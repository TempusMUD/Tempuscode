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
#include "clan.h"

struct prog_env *prog_list = NULL;
int loop_fence = 0;

// Prog command prototypes
void prog_do_before(prog_env *env, prog_evt *evt, char *args);
void prog_do_handle(prog_env *env, prog_evt *evt, char *args);
void prog_do_after(prog_env *env, prog_evt *evt, char *args);
void prog_do_require(prog_env *env, prog_evt *evt, char *args);
void prog_do_unless(prog_env *env, prog_evt *evt, char *args);
void prog_do_do(prog_env *env, prog_evt *evt, char *args);
void prog_do_force(prog_env *env, prog_evt *evt, char *args);
void prog_do_pause(prog_env *env, prog_evt *evt, char *args);
void prog_do_halt(prog_env *env, prog_evt *evt, char *args);
void prog_do_target(prog_env *env, prog_evt *evt, char *args);
void prog_do_nuke(prog_env *env, prog_evt *evt, char *args);
void prog_do_trans(prog_env *env, prog_evt *evt, char *args);
void prog_do_randomly(prog_env *env, prog_evt *evt, char *args);
void prog_do_or(prog_env *env, prog_evt *evt, char *args);

char *prog_get_statement(char **prog, int linenum);

struct prog_command {
	const char *str;
	bool count;
	void (*func)(prog_env *, prog_evt *, char *);
};

prog_command prog_cmds[] = {
	{ "before",		false,	prog_do_before },
	{ "handle",		false,	prog_do_handle },
	{ "after",		false,	prog_do_after },
	{ "require",	false,	prog_do_require },
	{ "unless",		false,	prog_do_unless },
	{ "randomly",	false,	prog_do_randomly },
	{ "or",			false,	prog_do_or },
	{ "pause",		true,	prog_do_pause },
	{ "do",			true,	prog_do_do },
	{ "force",		true,	prog_do_force },
	{ "target",		true,	prog_do_target },
	{ "nuke",		true,	prog_do_nuke },
	{ "trans",		true,	prog_do_trans },
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
prog_get_text(prog_env *env)
{
	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		return GET_MOB_PROG(((Creature *)env->owner));
	case PROG_TYPE_OBJECT:
	case PROG_TYPE_ROOM:
		break;
	}
	slog("Can't happen at %s:%d", __FILE__, __LINE__);
	return NULL;
}

void
prog_next_handler(prog_env *env)
{
	char *prog, *line, *cmd;

	// find our current execution point
	prog = prog_get_text(env);
	prog = advance_lines(prog, env->exec_pt);

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
		env->exec_pt = -1;
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
		while (*arg) {
			if (evt->kind == PROG_EVT_COMMAND
					&& evt->cmd >= 0
					&& !strcasecmp(cmd_info[evt->cmd].command, arg)) {
				matched = true;
				break;
			}
			arg = tmp_getword(&args);
		}
	} else if (!strcmp("idle", arg)) {
		matched = (evt->kind == PROG_EVT_IDLE);
	} else if (!strcmp("fight", arg)) {
		matched = (evt->kind == PROG_EVT_FIGHT);
	}

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
	char *arg, *str;
	bool result = false, not_flag = false;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "not")) {
		not_flag = true;
		arg = tmp_getword(&args);
	}

	if (!strcmp(arg, "argument")) {
		result = (evt->args && !strcasecmp(args, evt->args));
	} else if (!strcmp(arg, "keyword")) {
		if (evt->args) {
			str = evt->args;
			arg = tmp_getword(&str);
			while (*arg) {
				if (isname(arg, args)) {
					result = true;
					break;
				}
				arg = tmp_getword(&str);
			}
		}
	} else if (!strcmp(arg, "fighting")) {
		result = (env->owner_type == PROG_TYPE_MOBILE
				&& ((Creature *)env->owner)->isFighting());
	} else if (!strcmp(arg, "randomly"))
		result = number(0, 100) < atoi(args);

	return (not_flag) ? (!result):result;
}

void
prog_do_require(prog_env *env, prog_evt *evt, char *args)
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
prog_do_randomly(prog_env *env, prog_evt *evt, char *args)
{
	char *exec, *line, *cmd;
	int cur_line, last_line, num_paths;

	// We save the execution point and find the next handler.
	cur_line = env->exec_pt;
	prog_next_handler(env);
	last_line = env->exec_pt;
	env->exec_pt = cur_line;

	// now we run through, setting randomly which code path to take
	exec = prog_get_text(env);
	line = prog_get_statement(&exec, env->exec_pt);
	num_paths = 0;
	while (line) {
		cur_line++;
		if (last_line > 0 && cur_line >= last_line)
			break;
		if (*line == '*') {
			cmd = tmp_getword(&line) + 1;
			if (!strcasecmp(cmd, "or")) {
				num_paths += 1;
				if (!number(0, num_paths))
					env->exec_pt = cur_line;
			}
		}
		line = prog_get_statement(&exec, 0);
	}

	// At this point, exec_pt should be on a randomly selected code path
	// within the current handler
}

void
prog_do_or(prog_env *env, prog_evt *evt, char *args)
{
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
prog_do_force(prog_env *env, prog_evt *evt, char *args)
{
	if (!env->target)
		return;

	if (env->owner_type == PROG_TYPE_MOBILE) {
		args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *)env->target, args);
	}
}

void
prog_do_target(prog_env *env, prog_evt *evt, char *args)
{
	Creature *ch_self;
	obj_data *obj_self;
	char *arg;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "random")) {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			ch_self = (Creature *)env->owner;
			env->target = get_char_random_vis(ch_self, ch_self->in_room);
			break;
		case PROG_TYPE_OBJECT:
			obj_self = (obj_data *)env->owner;
			if (obj_self->in_room)
				env->target = get_char_random(obj_self->in_room);
			else if (obj_self->worn_by)
				env->target = get_char_random(obj_self->worn_by->in_room);
			else if (obj_self->carried_by)
				env->target = get_char_random(obj_self->carried_by->in_room);
			break;
		case PROG_TYPE_ROOM:
			env->target = get_char_random((room_data *)env->owner);
			break;
		}
	} else if (!strcasecmp(arg, "opponent")) {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			env->target = FIGHTING(((Creature *)env->owner));
			break;
		default:
			env->target = NULL;
		}
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

void
prog_do_nuke(prog_env *env, prog_evt *evt, char *args)
{
	struct prog_env *cur_prog, *next_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (cur_prog != env && cur_prog->owner == env->owner)
			prog_free(cur_prog);
	}
}

void
prog_do_trans(prog_env *env, prog_evt *evt, char *args)
{
	room_data *targ_room;
	int targ_num;

	if (!env->target)
		return;
	
	targ_num = atoi(tmp_getword(&args));
	if ((targ_room = real_room(targ_num)) == NULL) {
		slog("SYSERR: prog trans targ room %d nonexistant.", targ_num);
		return;
	}

	if (targ_room == env->target->in_room)
		return;

	if (!House_can_enter(env->target, targ_room->number)
		|| !clan_house_can_enter(env->target, targ_room)
		|| (ROOM_FLAGGED(targ_room, ROOM_GODROOM)
			&& !Security::isMember(env->target, "WizardFull"))) {
		return;
	}

	char_from_room(env->target);
	char_to_room(env->target, targ_room);
	targ_room->zone->enter_count++;

	if (env->target->followers) {
		struct follow_type *k, *next;

		for (k = env->target->followers; k; k = next) {
			next = k->next;
			if (targ_room == k->follower->in_room &&
					GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
					!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
					can_see_creature(k->follower, env->target))
				perform_goto(k->follower, targ_room, true);
		}
	}

	if (IS_SET(ROOM_FLAGS(targ_room), ROOM_DEATH)) {
		if (GET_LEVEL(env->target) < LVL_AMBASSADOR) {
			log_death_trap(env->target);
			death_cry(env->target);
			env->target->die();
			//Event::Queue(new DeathEvent(0, ch, false));
			return;
		} else {
			mudlog(LVL_GOD, NRM, true,
				"(GC) %s trans-searched into deathtrap %d.",
				GET_NAME(env->target), targ_room->number);
		}
	}
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
	
	while (statement[strlen(statement) - 1] == '\\') {
		statement[strlen(statement) - 1] = '\0';
		statement = tmp_strcat(statement, tmp_getline(prog), NULL);
	}
	
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

	exec = prog_get_text(env);
	line = prog_get_statement(&exec, env->exec_pt);
	while (line) {
		// increment line number of currently executing prog
		cur_line = env->exec_pt;
		env->exec_pt++;

		if (*line == '*') {
			prog_handle_command(env, &env->evt, line);
		} else if (*line == '\0' || *line == '-') {
			// Do nothing.  comment or blank.
		} else if (env->owner_type == PROG_TYPE_MOBILE) {
			env->executed += 1;
			prog_do_do(env, &env->evt, line);
		} else {
			// error
		}

		// prog exit
		if (env->exec_pt < 0)
			return;

		if (env->wait > 0)
			return;

		if (env->exec_pt > cur_line + 1)
			exec = advance_lines(exec, env->exec_pt - (cur_line + 1));
		line = prog_get_statement(&exec, 0);
	}

	env->exec_pt = -1;
}

prog_env *
prog_start(int owner_type, void *owner, Creature *target, char *prog, prog_evt *evt)
{
	prog_env *new_prog;

	CREATE(new_prog, struct prog_env, 1);
	new_prog->next = prog_list;
	prog_list = new_prog;

	new_prog->owner_type = owner_type;
	new_prog->owner = owner;
	new_prog->exec_pt = 0;
	new_prog->executed = 0;
	new_prog->wait = 0;
	new_prog->speed = 0;
	new_prog->target = target;
	new_prog->evt = *evt;

	prog_next_handler(new_prog);

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
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (!GET_MOB_PROG(owner) || ch == owner)
		return false;
	
	// We don't want an infinite loop with mobs triggering progs that
	// trigger a prog, etc.
	if (loop_fence >= 20) {
		mudlog(LVL_IMMORT, NRM, true, "Infinite prog loop halted.");
		return false;
	}
	
	loop_fence++;

	evt.phase = PROG_EVT_BEGIN;
	evt.kind = PROG_EVT_COMMAND;
	evt.cmd = cmd;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = strdup(argument);
	env = prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), &evt);
	prog_execute(env);
	
	evt.phase = PROG_EVT_HANDLE;
	handler_env = prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), &evt);
	prog_execute(handler_env);

	evt.phase = PROG_EVT_AFTER;
	env = prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), &evt);

	loop_fence -= 1;

	if (handler_env && handler_env->executed)
		return true;

	return handled;
}

void
trigger_prog_fight(Creature *ch, Creature *self)
{
	prog_env *env;
	prog_evt evt;

	if (!self || !self->in_room || !GET_MOB_PROG(self))
		return;
	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_FIGHT;
	evt.cmd = -1;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = strdup("");

	env = prog_start(PROG_TYPE_MOBILE, self, ch, GET_MOB_PROG(self), &evt);
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
		if (cur_prog->exec_pt < 0)
			prog_free(cur_prog);
	}
}

void
prog_update_pending(void)
{
	struct prog_env *cur_prog;

	if (!prog_list)
		return;
	
	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next)
		if (cur_prog->exec_pt == 0 && cur_prog->executed == 0)
			prog_execute(cur_prog);
}

int
prog_count(void)
{
	int result = 0;
	prog_env *cur_env;

	for (cur_env = prog_list;cur_env;cur_env = cur_env->next)
		if (cur_env->exec_pt > 0)
			result++;
	return result;
}
