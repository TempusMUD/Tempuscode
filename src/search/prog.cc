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

struct prog_engine *prog_list = NULL;

struct prog_engine *
find_prog_by_owner(void *owner)
{
	struct prog_engine *cur_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next)
		if (cur_prog->owner == owner)
			return cur_prog;

	return NULL;
}

// Returns the next line number after the appropriate command handler
int
prog_find_cmd_handler(char *prog, const char *directive, int cmd)
{
	char *line, *str;
	int linenum;

	for (line = tmp_getline(&prog), linenum = 1;
			line;
			line = tmp_getline(&prog), linenum++) {
		if (*line != '*')
			continue;
		str = tmp_getword(&line);
		if (strcmp(directive, str))
			continue;
		str = tmp_getword(&line);
		if (strcmp("command", str))
			continue;
		str = tmp_getword(&line);
		if (strcmp(cmd_info[cmd].command, str))
			continue;

		return linenum;
	}
	return -1;
}

int
prog_find_idle_handler(char *prog)
{
	char *line, *str;
	int linenum;

	for (line = tmp_getline(&prog), linenum = 1;
			line;
			line = tmp_getline(&prog), linenum++) {
		if (*line != '*')
			continue;
		str = tmp_getword(&line);
		if (strcmp("*handle", str))
			continue;
		str = tmp_getword(&line);
		if (strcmp("idle", str))
			continue;

		return linenum;
	}
	return -1;
}

void
prog_do_command(struct prog_engine *prog, char *exec)
{
	char *str;

	str = tmp_getword(&exec) + 1;
	if (!strcmp("before", str)
			|| !strcmp("handle", str)
			|| !strcmp("after", str)) {
		// The beginning of another handler terminates this one
		prog->linenum = 0;
	} else if (!strcmp("echo", str)) {
		room_data *room = NULL;

		switch (prog->owner_type) {
		case PROG_TYPE_MOBILE:
			room = ((Creature *)prog->owner)->in_room; break;
		case PROG_TYPE_OBJECT:
			room = ((obj_data *)prog->owner)->in_room; break;
		case PROG_TYPE_ROOM:
			room = ((room_data *)prog->owner); break;
		}
		if (room)
			send_to_room(exec, room);
	} else if (!strcmp("pause", str)) {
		prog->wait = atoi(exec);
	} else if (!strcmp("wait", str)) {
		prog->wait = -1;
	}
}

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
prog_execute(struct prog_engine *prog)
{
	char *exec, *line;

	// blocking indefinitely
	if (prog->wait < 0)
		return;

	// waiting until the right moment
	if (prog->wait > 0) {
		prog->wait -= 1;
		return;
	}

	// we've waited long enough!
	prog->wait = prog->speed;

	exec = advance_lines(prog->prog, prog->linenum);
	line = tmp_getline(&exec);
	while (line) {
		if (*line == '*') {
			prog_do_command(prog, line);
		} else if (prog->owner_type == PROG_TYPE_MOBILE) {
			if (prog->target)
				line = tmp_gsub(line, "$N", fname(prog->target->player.name));
			command_interpreter((Creature *)prog->owner, line);
		} else {
			// error
		}

		// prog exit
		if (!prog->linenum)
			return;

		// increment line number of currently executing prog
		prog->linenum++;

		if (prog->wait > 0) {
			prog->wait -= 1;
			return;
		}

		line = tmp_getline(&exec);
	}

	prog->linenum = 0;
}

void
prog_start(int owner_type, void *owner, Creature *target, char *prog, int linenum)
{
	struct prog_engine *new_prog, *cur_prog;

	// Find any progs run by the same owner
	cur_prog = find_prog_by_owner(owner);
	if (cur_prog) {
		// The same object was running a prog, so we just re-initialize
		// its state
		new_prog = cur_prog;
	} else {
		// The same object wasn't already running a prog, so we have to
		// make a new one
		CREATE(new_prog, struct prog_engine, 1);
		new_prog->next = prog_list;
		prog_list = new_prog;
	}

	new_prog->owner_type = owner_type;
	new_prog->owner = owner;
	new_prog->prog = prog;
	new_prog->linenum = linenum;
	new_prog->wait = 0;
	new_prog->speed = 1;
	new_prog->cmd_var = 0;
	new_prog->target = target;

	prog_execute(new_prog);
}

void
prog_free(struct prog_engine *prog)
{
	struct prog_engine *prev_prog;

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
	struct prog_engine *cur_prog, *next_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (cur_prog->owner == owner)
			prog_free(cur_prog);
		else if (cur_prog->target == owner)
			prog_free(cur_prog);
	}
}

bool
trigger_prog_cmd(Creature *owner, Creature *ch, int cmd, char *argument)
{
	int linenum;

	if (!GET_MOB_PROG(owner) || ch == owner)
		return false;
	
	linenum = prog_find_cmd_handler(GET_MOB_PROG(owner), "*before", cmd);
	if (linenum > 0)
		prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), linenum);
	
	linenum = prog_find_cmd_handler(GET_MOB_PROG(owner), "*handle", cmd);
	if (linenum > 0) {
		prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), linenum);
		return true;
	}

	linenum = prog_find_cmd_handler(GET_MOB_PROG(owner), "*after", cmd);
	if (linenum > 0) {
		cmd_info[cmd].command_pointer(ch, argument, cmd, cmd_info[cmd].subcmd, 0);
		prog_start(PROG_TYPE_MOBILE, owner, ch, GET_MOB_PROG(owner), linenum);
		return true;
	}
	
	return false;
}

void
trigger_prog_idle(Creature *owner)
{
	int linenum;

	// Do we have a mobile program?
	if (!GET_MOB_PROG(owner))
		return;
	
	// Are we already running a prog?
	if (find_prog_by_owner(owner))
		return;
	
	// We start an idle mobprog here
	linenum = prog_find_idle_handler(GET_MOB_PROG(owner));
	if (!linenum)
		return;
	prog_start(PROG_TYPE_MOBILE, owner, NULL, GET_MOB_PROG(owner), linenum);
}

void
prog_update(void)
{
	struct prog_engine *cur_prog, *next_prog;

	if (!prog_list)
		return;
	
	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next)
		prog_execute(cur_prog);
	
	for (cur_prog = prog_list;cur_prog;cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (!cur_prog->linenum)
			prog_free(cur_prog);
	}
}
