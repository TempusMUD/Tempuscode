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

// Returns the next line number after the appropriate command handler
int
prog_find_cmd_handler(char *prog, const char *directive, int cmd, char *arg)
{
	char *line, *str;
	int linenum;

	linenum = 1;
	arg = tmp_getword(&arg);
	for (line = tmp_getline(&prog);
			line;
			linenum++, line = tmp_getline(&prog)) {
		str = tmp_getword(&line);
		if (strcmp(directive, str))
			continue;
		str = tmp_getword(&line);
		if (strcmp("command", str))
			continue;
		str = tmp_getword(&line);
		if (strcmp(cmd_info[cmd].command, str))
			continue;
		if (*line && !isname(arg, line))
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

	linenum = 1;
	for (line = tmp_getline(&prog);
			line;
			linenum++, line = tmp_getline(&prog)) {
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
execute_prog(Creature *ch)
{
	char *prog, *line, *str;
	int linenum;

	if (ch->mob_specials.prog_wait> 0) {
		ch->mob_specials.prog_wait -= 1;
		return;
	}

	prog = GET_MOB_PROG(ch);
	line = tmp_getline(&prog);
	linenum = 0;
	while (line && linenum < ch->mob_specials.prog_exec) {
		linenum++;
		line = tmp_getline(&prog);
	}

	while (line) {
		if (*line == '*') {
			str = tmp_getword(&line);
			if (!strcmp("*before", str)
					|| !strcmp("*handle", str)
					|| !strcmp("*after", str))
				break;
			else if (!strcmp("*echo", str))
				send_to_room(line, ch->in_room);
			else if (!strcmp("*pause", str)) {
				ch->mob_specials.prog_exec++;
				ch->mob_specials.prog_wait = atoi(line);
				if (ch->mob_specials.prog_wait > 0) {
					ch->mob_specials.prog_wait -= 1;
					return;
				}
				ch->mob_specials.prog_wait = 0;
			}
				
		} else {
			if (ch->mob_specials.prog_target)
				line = tmp_gsub(line, "$N", fname(ch->mob_specials.prog_target->player.name));
			command_interpreter(ch, line);
		}

		ch->mob_specials.prog_exec++;
		line = tmp_getline(&prog);
	}

	ch->mob_specials.prog_exec = 0;
	ch->mob_specials.prog_target = NULL;
}

void
start_subprog(Creature *self, Creature *targ, int line)
{
	self->mob_specials.prog_wait = 0;
	self->mob_specials.prog_exec = line;
	self->mob_specials.prog_target = targ;
	execute_prog(self);
}

bool
trigger_prog_cmd(Creature *ch, Creature *self, int cmd, char *argument)
{
	int line;

	if (!GET_MOB_PROG(self))
		return false;
	
	line = prog_find_cmd_handler(GET_MOB_PROG(self), "*before", cmd, argument);
	if (line > 0)
		start_subprog(self, ch, line);
	
	line = prog_find_cmd_handler(GET_MOB_PROG(self), "*handle", cmd, argument);
	if (line > 0) {
		start_subprog(self, ch, line);
		return true;
	}

	line = prog_find_cmd_handler(GET_MOB_PROG(self), "*after", cmd, argument);
	if (line > 0) {
		cmd_info[cmd].command_pointer(ch, argument, cmd, cmd_info[cmd].subcmd, 0);
		start_subprog(self, ch, line);
		return true;
	}
	
	return false;
}

void
trigger_prog_idle(Creature *self)
{
	int line;

	// Do we have a mobile program?
	if (!GET_MOB_PROG(self))
		return;
	
	// Do we have a mobile program already running?
	if (self->mob_specials.prog_exec)
		return;
	
	// We start an idle mobprog here
	line = prog_find_idle_handler(GET_MOB_PROG(self));
	if (!line)
		return;
	self->mob_specials.prog_exec = line;
	self->mob_specials.prog_wait = 0;
	self->mob_specials.prog_target = NULL;
	execute_prog(self);
}
