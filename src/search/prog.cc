#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "fight.h"
#include "security.h"
#include "actions.h"
#include "tmpstr.h"
#include "house.h"
#include "events.h"
#include "prog.h"

struct prog_env *prog_list = NULL;

// Prog command prototypes
void prog_do_before(prog_env *env, prog_evt *evt, char *args);
void prog_do_handle(prog_env *env, prog_evt *evt, char *args);
void prog_do_after(prog_env *env, prog_evt *evt, char *args);
void prog_do_when(prog_env *env, prog_evt *evt, char *args);
void prog_do_unless(prog_env *env, prog_evt *evt, char *args);
void prog_do_do(prog_env *env, prog_evt *evt, char *args);
void prog_do_pause(prog_env *env, prog_evt *evt, char *args);
void prog_do_halt(prog_env *env, prog_evt *evt, char *args);

struct prog_command {
	const char *str;
	bool count;
	void (*func)(prog_env *, prog_evt *, char *);
};

prog_command prog_cmds[] = {
	{ "before",	false,	prog_do_before },
	{ "handle",	false,	prog_do_handle },
	{ "after",	false,	prog_do_after },
	{ "when",	false,	prog_do_when },
	{ "unless",	false,	prog_do_unless },
	{ "do",		true,	prog_do_do },
	{ "pause",	true,	prog_do_pause },
	{ "halt",	true,	prog_do_halt },
	{ NULL,		false,	prog_do_halt }
};

char *
advance_lines(char *str, int lines)
{
	while (*str && lines) {
		while (*str && *str != '\r' && *str != '\n')
			str++;
		if (*str == '\r')
			str++;
		if (*str == '\n')
			str++;
		lines--;
	}

	return str;
}

void
prog_next_handler(prog_env *env)
{
	char *prog, *line, *cmd;

	// find our current execution point
	prog = advance_lines(env->prog, env->exec_pt);

	// skip over lines until we find another handler (or bust)
	while ((line = tmp_getline(&prog)) != NULL) {
		cmd = tmp_getword(&line);
		if (*cmd != '*') {
			env->exec_pt++;
			continue;
		}
		cmd++;
		if (!strcmp(cmd, "before")
				|| !strcmp(cmd, "handle")
				|| !strcmp(cmd, "after"))
			break;
		env->exec_pt++;
	}
	// if we reached the end of the prog, terminate the program
	if (!line)
		env->exec_pt = 0;
}

void
prog_trigger_handler(prog_env *env, prog_evt *evt, int phase, char *args)
{
	char *arg;
	bool matched = false;

	if (phase != evt->phase) {
		prog_next_handler(env);
		return;
	}
		
	arg = tmp_getword(&args);
	if (!strcmp("command", arg)) {
		arg = tmp_getword(&args);
		matched = (evt->kind == PROG_EVT_COMMAND)
			&& evt->cmd >= 0
			&& !strcasecmp(cmd_info[evt->cmd].command, arg);
	} else if (!strcmp("idle", arg))
		matched = (evt->kind == PROG_EVT_IDLE);

	if (!matched)
		prog_next_handler(env);
}

void
prog_do_before(prog_env *env, prog_evt *evt, char *args)
{
	prog_trigger_handler(env, evt, PROG_EVT_BEGIN, args);
}

void
prog_do_handle(prog_env *env, prog_evt *evt, char *args)
{
	prog_trigger_handler(env, evt, PROG_EVT_HANDLE, args);
}

void
prog_do_after(prog_env *env, prog_evt *evt, char *args)
{
	prog_trigger_handler(env, evt, PROG_EVT_AFTER, args);
}

bool
prog_eval_condition(prog_env *env, prog_evt *evt, char *args)
{
	char *arg;

	arg = tmp_getword(&args);
	if (!strcmp(arg, "argument")) {
		arg = tmp_getword(&args);
		if (!strcmp(arg, "is")) {
			return (evt->args && !strcasecmp(args, evt->args));
		} else if (!strcmp(arg, "contains")) {
			return (evt->args && !strstr(args, evt->args));
		}
	} else if (!strcmp(arg, "fighting")) {
		return (env->owner_type == PROG_TYPE_MOBILE
				&& ((Creature *)env->owner)->isFighting());
	} else if (!strcmp(arg, "randomly"))
		return number(0, 100) < atoi(args);

	return false;
}

void
prog_do_when(prog_env *env, prog_evt *evt, char *args)
{
	if (!prog_eval_condition(env, evt, args))
		prog_next_handler(env);
}

void
prog_do_unless(prog_env *env, prog_evt *evt, char *args)
{
	if (prog_eval_condition(env, evt, args))
		prog_next_handler(env);
}

void
prog_do_do(prog_env *env, prog_evt *evt, char *args)
{
	if (env->owner_type == PROG_TYPE_MOBILE) {
		if (env->target)
			args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *)env->owner, args);
	}
}

void
prog_do_pause(prog_env *env, prog_evt *evt, char *args)
{
	env->wait = MAX(1, atoi(args));
}

void
prog_do_halt(prog_env *env, prog_evt *evt, char *args)
{
	env->wait = -1;
}


struct prog_env *
find_prog_by_owner(void *owner)
{
	struct prog_env *cur_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next)
		if (cur_prog->owner == owner)
			return cur_prog;

	return NULL;
}

char *
prog_get_statement(char **prog, int linenum)
{
	char *statement;

	if (linenum)
		*prog = advance_lines(*prog, linenum);

	statement = tmp_getline(prog);
	if (!statement)
		return NULL;
	
	while (statement[strlen(statement) - 1] == '\\')
		statement = tmp_strcat(statement, tmp_getline(prog), NULL);
	
	return statement;
}

void
prog_handle_command(prog_env *env, prog_evt *evt, char *statement)
{
	prog_command *cmd;
	char *cmd_str;

	cmd_str = tmp_getword(&statement) + 1;
	cmd = prog_cmds;
	while (cmd->str && strcasecmp(cmd->str, cmd_str))
		cmd++;
	if (!cmd->str)
		return;
	if (cmd->count)
		env->executed++;
	cmd->func(env, evt, statement);
}

void
prog_execute(prog_env *env)
{
	char *exec, *line;
	int cur_line;

	// blocking indefinitely
	if (env->wait < 0)
		return;

	// waiting until the right moment
	if (env->wait > 0) {
		env->wait -= 1;
		return;
	}

	// we've waited long enough!
	env->wait = env->speed;

	exec = env->prog;
	line = prog_get_statement(&exec, env->exec_pt);
	while (line) {
		// increment line number of currently executing prog
		cur_line = env->exec_pt;
		env->exec_pt++;

		if (*line == '*') {
			prog_handle_command(env, &env->evt, line);
		} else if (env->owner_type == PROG_TYPE_MOBILE) {
			env->executed += 1;
			prog_do_do(env, &env->evt, line);
		} else {
			// error
		}

		// prog exit
		if (!env->exec_pt)
			return;

		if (env->wait > 0)
			return;

		if (env->exec_pt > cur_line + 1)
			exec = advance_lines(exec, env->exec_pt - (cur_line + 1));
		line = prog_get_statement(&exec, 0);
	}

	env->exec_pt = 0;
}

prog_env *
prog_start(int owner_type, void *owner, Creature *target, char *prog, prog_evt *evt)
{
	prog_env *new_prog, *cur_prog;

	// Find any progs run by the same owner
	cur_prog = find_prog_by_owner(owner);
	if (cur_prog) {
		// The same object was running a prog, so we just re-initialize
		// its state
		new_prog = cur_prog;
	} else {
		// The same object wasn't already running a prog, so we have to
		// make a new one
		CREATE(new_prog, struct prog_env, 1);
		new_prog->next = prog_list;
		prog_list = new_prog;
	}

	new_prog->owner_type = owner_type;
	new_prog->owner = owner;
	new_prog->prog = prog;
	new_prog->exec_pt = 0;
	new_prog->executed = 0;
	new_prog->wait = 0;
	new_prog->speed = 0;
	new_prog->target = target;
	new_prog->evt = *evt;

	prog_execute(new_prog);

	return new_prog;
}

void
prog_free(struct prog_env *prog)
{
	struct prog_env *prev_prog;

	if (prog_list == prog) {
		prog_list = prog->next;
	} else {
		prev_prog = prog_list;
		while (prev_prog && prev_prog->next != prog)
			prev_prog = prev_prog->next;
		if (!prev_prog) {
			// error
		}
		prev_prog->next = prog->next;
	}

	free(prog);
}

void
destroy_attached_progs(void *owner)
{
	struct prog_env *cur_prog, *next_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (cur_prog->owner == owner)
			prog_free(cur_prog);
		else if (cur_prog->target == owner)
			prog_free(cur_prog);
		else if (cur_prog->evt.subject == owner)
			prog_free(cur_prog);
		else if (cur_prog->evt.object == owner)
			prog_free(cur_prog);
	}
}

bool
trigger_prog_cmd(Creature *owner, Creature *ch, int cmd, char *argument)
{
	prog_env *env;
	prog_evt evt;
	bool handled = false;

	if (!GET_MOB_PROG(owner) || ch == owner)
		return false;
	
	evt.phase = PROG_EVT_BEGIN;
	evt.kind = PROG_EVT_COMMAND;
	evt.cmd = cmd;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = strdup(argument);
	env = prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), &evt);
	
	evt.phase = PROG_EVT_HANDLE;
	env = prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), &evt);
	if (env->executed)
		return true;

	return handled;
}

void
trigger_progs_after(Creature *ch, int cmd, char *argument)
{
	prog_env *env;
	prog_evt evt;

	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_COMMAND;
	evt.cmd = cmd;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = strdup(argument);

	CreatureList::iterator cit = ch->in_room->people.begin();
	while (cit != ch->in_room->people.end()) {
		if (ch && ch->in_room && ch != (*cit) && GET_MOB_PROG((*cit)))
			env = prog_start(PROG_TYPE_MOBILE, *cit, ch, GET_MOB_PROG((*cit)), &evt);
		cit++;
	}
}

void
trigger_prog_idle(Creature *owner)
{
	prog_evt evt;

	// Do we have a mobile program?
	if (!GET_MOB_PROG(owner))
		return;
	
	// Are we already running a prog?
	if (find_prog_by_owner(owner))
		return;
	
	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_IDLE;
	evt.cmd = -1;
	evt.subject = owner;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = NULL;

	// We start an idle mobprog here
	prog_start(PROG_TYPE_MOBILE, owner, NULL, GET_MOB_PROG(owner), &evt);
}

void
prog_update(void)
{
	struct prog_env *cur_prog, *next_prog;

	if (!prog_list)
		return;
	
	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next)
		prog_execute(cur_prog);
	
	for (cur_prog = prog_list;cur_prog;cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (!cur_prog->exec_pt)
			prog_free(cur_prog);
	}
}
