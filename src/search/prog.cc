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
void prog_do_silently(prog_env *env, prog_evt *evt, char *args);
void prog_do_force(prog_env *env, prog_evt *evt, char *args);
void prog_do_pause(prog_env *env, prog_evt *evt, char *args);
void prog_do_walkto(prog_env *env, prog_evt *evt, char *args);
void prog_do_driveto(prog_env *env, prog_evt *evt, char *args);
void prog_do_halt(prog_env *env, prog_evt *evt, char *args);
void prog_do_target(prog_env *env, prog_evt *evt, char *args);
void prog_do_nuke(prog_env *env, prog_evt *evt, char *args);
void prog_do_trans(prog_env *env, prog_evt *evt, char *args);
void prog_do_set(prog_env *env, prog_evt *evt, char *args);
void prog_do_oload(prog_env *env, prog_evt *evt, char *args);
void prog_do_mload(prog_env *env, prog_evt *evt, char *args);
void prog_do_opurge(prog_env *env, prog_evt *evt, char *args);
void prog_do_randomly(prog_env *env, prog_evt *evt, char *args);
void prog_do_or(prog_env *env, prog_evt *evt, char *args);
void prog_do_resume(prog_env *env, prog_evt *evt, char *args);
void prog_do_echo(prog_env *env, prog_evt *evt, char *args);
void prog_do_mobflag(prog_env *env, prog_evt *evt, char *args);

//external prototypes
struct Creature *real_mobile_proto(int vnum);
struct obj_data *real_object_proto(int vnum);

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
	{ "walkto",		true,	prog_do_walkto },
	{ "driveto",	true,	prog_do_driveto },
	{ "do",			true,	prog_do_do },
	{ "silently",	true,	prog_do_silently },
	{ "force",		true,	prog_do_force },
	{ "target",		true,	prog_do_target },
	{ "nuke",		true,	prog_do_nuke },
	{ "trans",		true,	prog_do_trans },
	{ "resume",		false,	prog_do_resume },
	{ "set",		true,	prog_do_set },
	{ "oload",		true,	prog_do_oload },
	{ "mload",		true,	prog_do_mload },
	{ "opurge",		true,	prog_do_opurge },
	{ "echo",		true,	prog_do_echo },
	{ "halt",		false,	prog_do_halt },
	{ "mobflag",	true,	prog_do_mobflag },
	{ NULL,		false,	prog_do_halt }
};

char *
advance_statements(char *str, int count)
{
	while (*str && count) {
		while (*str && *str != '\\' && *str != '\r' && *str != '\n')
			str++;
		// code duplicated for speed purposes
		if (*str == '\\') {
			str++;
			if (*str == '\r')
				str++;
			if (*str == '\n')
				str++;
		} else {
			if (*str == '\r')
				str++;
			if (*str == '\n')
				str++;
			count--;
		}
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
	case PROG_TYPE_OBJECT:
	  break;
	case PROG_TYPE_MOBILE:
		return GET_MOB_PROG(((Creature *)env->owner));
	case PROG_TYPE_ROOM:
		return ((room_data *)env->owner)->prog;
	}
	errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	return NULL;
}

void
prog_next_handler(prog_env *env, bool use_resume)
{
	char *prog, *line, *cmd;

	// find our current execution point
	prog = advance_statements(prog_get_text(env), env->exec_pt);

	// skip over lines until we find another handler (or bust)
	while ((line = prog_get_statement(&prog, 0)) != NULL) {
		cmd = tmp_getword(&line);
		if (*cmd != '*') {
			env->exec_pt++;
			continue;
		}
		cmd++;
		if (!strcmp(cmd, "before")
				|| !strcmp(cmd, "handle")
				|| !strcmp(cmd, "after")
				|| (use_resume && !strcmp(cmd, "resume")))
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
		prog_next_handler(env, false);
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
	} else if (!strcmp("give", arg)) {
        arg = tmp_getword(&args);
        struct obj_data *theObj = (struct obj_data *)(evt->object);
        if (evt->kind != PROG_EVT_GIVE) {
            matched = false;
        }
        else if (!*arg) {
            matched = true;
        }
        else if (arg && !evt->object) {
            matched = false;
        }
        else if (atoi(arg) != theObj->getVnum()) {
            matched = false;
        }
        else {
            matched = true;
        }
	} else if (!strcmp("enter", arg)) {
		matched = (evt->kind == PROG_EVT_ENTER);
	} else if (!strcmp("leave", arg)) {
		matched = (evt->kind == PROG_EVT_LEAVE);
	} else if (!strcmp("load", arg)) {
		matched = (evt->kind == PROG_EVT_LOAD);
	} else if (!strcmp("tick", arg)) {
		matched = (evt->kind == PROG_EVT_TICK);
	}


	if (!matched)
		prog_next_handler(env, false);
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
prog_var_equal(prog_state_data *state, char *key, char *arg)
{
	struct prog_var *cur_var;

	for (cur_var = state->var_list;cur_var;cur_var = cur_var->next)
		if (!strcasecmp(cur_var->key, key))
			break;
	if (!cur_var || !cur_var->key)
		return !(*arg);
	return !strcasecmp(cur_var->value, arg);
}

bool
prog_eval_condition(prog_env *env, prog_evt *evt, char *args)
{
	obj_data *obj = NULL;
	int vnum;
	char *arg, *str;
	bool result = false, not_flag = false;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "not")) {
		not_flag = true;
		arg = tmp_getword(&args);
	}

	if (!strcmp(arg, "argument")) {
		result = (evt->args && !strcasecmp(args, evt->args));
    // Mobs using "alias"
    // 1200 3062 90800
	} else if (!strcmp(arg, "alias")) {
        char *alias_list = NULL;
        if (!(alias_list = prog_get_alias_list(args)))
            result = false;
		if (evt->args && alias_list) {
			str = evt->args;
			arg = tmp_getword(&str);
			while (*arg) {
				if (isname(arg, alias_list)) {
					result = true;
					break;
				}
				arg = tmp_getword(&str);
			}
		}
	} else if (!strcmp(arg, "keyword")) {
		if (evt->args) {
			str = evt->args;
			arg = tmp_getword(&str);
			while (*arg) {
				if (isname_exact(arg, args)) {
					result = true;
					break;
				}
				arg = tmp_getword(&str);
			}
		}
	} else if (!strcmp(arg, "abbrev")) {
		if (evt->args) {
			str = evt->args;
			arg = tmp_getword(&str);
			while (*arg) {
                char *len_ptr = NULL, *tmp_args = args, *saved_args = args;
                while (*args && (args = tmp_getword(&tmp_args))) {
                    int len = 0;
                    if ((len_ptr = strstr(args, "*"))) {
                        len = len_ptr - args;
                        memcpy(len_ptr, len_ptr + 1, strlen(args) - len - 1);
                        args[strlen(args) - 1] = 0;
                    }
				    if (is_abbrev(arg, args, len)) {
					    result = true;
					    break;
				    }
                }
                args = saved_args;
				arg = tmp_getword(&str);
			}
		}
    } else if (!strcmp(arg, "fighting")) {
		result = (env->owner_type == PROG_TYPE_MOBILE 
				&& ((Creature *)env->owner)->numCombatants());
	} else if (!strcmp(arg, "randomly")) {
		result = number(0, 100) < atoi(args);
	} else if (!strcmp(arg, "variable")) {
		if (env->state) {
			arg = tmp_getword(&args);
			result = prog_var_equal(env->state, arg, args);
		} else if (!*args)
			result = true;
	} else if (!strcasecmp(arg, "holding")) {
		vnum = atoi(tmp_getword(&args));
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			obj = ((Creature *)env->owner)->carrying; break;
		case PROG_TYPE_OBJECT:
			obj = ((obj_data *)env->owner)->contains; break;
		case PROG_TYPE_ROOM:
			obj = ((room_data *)env->owner)->contents; break;
		default:
			obj = NULL;
		}
		while (obj) {
			if (GET_OBJ_VNUM(obj) == vnum)
				result = true;
			obj = obj->next_content;
		}
	} else if (!strcasecmp(arg, "hour")) {
		result = time_info.hours == atoi(tmp_getword(&args));
	} else if (!strcasecmp(arg, "phase")) {
		str = tmp_getword(&args);
		vnum = get_lunar_phase(lunar_day);
		if (strcasecmp(str, "full"))
			result = vnum == MOON_FULL;
		else if (strcasecmp(str, "waning"))
			result = vnum == MOON_WANE_GIBBOUS
				|| vnum == MOON_WANE_CRESCENT
				|| vnum == MOON_LAST_QUARTER;
		else if (strcasecmp(str, "new"))
			result = vnum == MOON_NEW;
		else if (strcasecmp(str, "waxing"))
			result = vnum == MOON_WAX_GIBBOUS
				|| vnum == MOON_WAX_CRESCENT
				|| vnum == MOON_FIRST_QUARTER;
	}

	return (not_flag) ? (!result):result;
}

void
prog_do_require(prog_env *env, prog_evt *evt, char *args)
{
	if (!prog_eval_condition(env, evt, args))
		prog_next_handler(env, true);
}

void
prog_do_unless(prog_env *env, prog_evt *evt, char *args)
{
	if (prog_eval_condition(env, evt, args))
		prog_next_handler(env, true);
}

void
prog_do_randomly(prog_env *env, prog_evt *evt, char *args)
{
	char *exec, *line, *cmd;
	int cur_line, last_line, num_paths;

	// We save the execution point and find the next handler.
	cur_line = env->exec_pt;
	prog_next_handler(env, true);
	last_line = env->exec_pt;
	env->exec_pt = cur_line;

	// now we run through, setting randomly which code path to take
	exec = prog_get_text(env);
	line = prog_get_statement(&exec, env->exec_pt);
	num_paths = 0;
	while (line) {
		if (last_line > 0 && cur_line >= last_line)
			break;
		if (*line == '*') {
			cmd = tmp_getword(&line) + 1;
			if (!strcasecmp(cmd, "or")) {
				num_paths += 1;
				if (!number(0, num_paths))
					env->exec_pt = cur_line + 1;
			}
		}
		cur_line++;
		line = prog_get_statement(&exec, 0);
	}

	// At this point, exec_pt should be on a randomly selected code path
	// within the current handler
}

void
prog_do_or(prog_env *env, prog_evt *evt, char *args)
{
	prog_next_handler(env, true);
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
prog_do_silently(prog_env *env, prog_evt *evt, char *args)
{
	suppress_output = true;
	if (env->owner_type == PROG_TYPE_MOBILE) {
		if (env->target)
			args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *)env->owner, args);
	}
	suppress_output = false;
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
			env->target = ((Creature *)env->owner)->findRandomCombat();
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
prog_do_walkto(prog_env *env, prog_evt *evt, char *args)
{
	Creature *ch;
	room_data *room;
	int dir, pause;
	
	if (env->owner_type != PROG_TYPE_MOBILE)
		return;

	ch = (Creature *)env->owner;
	room = real_room(atoi(tmp_getword(&args)));

	if (room && room != ch->in_room) {
		dir = find_first_step(ch->in_room, room, STD_TRACK);
		if (dir != -1)
			smart_mobile_move(ch, dir);
		
		// we have to wait at least one second
		pause = atoi(tmp_getword(&args));
		env->wait = MAX(1, pause);

		// we stay on the same line until we get to the destination
		env->exec_pt--;
	}
}

void
prog_do_driveto(prog_env *env, prog_evt *evt, char *args)
{
	// move_car courtesy of vehicle.cc
	int move_car(struct Creature *ch, struct obj_data *car, int dir);

	obj_data *console, *vehicle, *engine;
	Creature *ch;
	room_data *target_room;
	room_direction_data *exit;
	int dir, pause;
	
	if (env->owner_type != PROG_TYPE_MOBILE)
		return;

	ch = (Creature *)env->owner;
	target_room = real_room(atoi(tmp_getword(&args)));

	// Find the console in the room.  Do nothing if there's no console
	for (console = ch->in_room->contents;console;console = console->next_content)
		if (GET_OBJ_TYPE(console) == ITEM_V_CONSOLE)
			break;
	if (!console)
		return;
	
	// Now find the vehicle vnum that the console points to
	for (vehicle = object_list;vehicle;vehicle = vehicle->next)
		if (GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(console) && vehicle->in_room)
			break;
	if (!vehicle)
		return;
	
	// Check for engine status
	engine = vehicle->contains;
	if (!engine || !IS_ENGINE(engine) || !ENGINE_ON(engine))
		return;

	if (target_room != vehicle->in_room) {
		dir = find_first_step(vehicle->in_room, target_room, PSI_TRACK);

		// Validate exit the vehicle is going to take
		exit = vehicle->in_room->dir_option[dir];
		if (!exit ||
				!exit->to_room ||
				ROOM_FLAGGED(exit->to_room, ROOM_DEATH | ROOM_INDOORS) ||
				IS_SET(exit->exit_info, EX_CLOSED | EX_ISDOOR))
			return;

		move_car(ch, vehicle, dir);

		// we have to wait at least one second
		pause = atoi(tmp_getword(&args));
		env->wait = MAX(1, pause);

		// we stay on the same line until we get to the destination
		env->exec_pt--;
	}
}

void
prog_do_halt(prog_env *env, prog_evt *evt, char *args)
{
	env->exec_pt = -1;
}

void
prog_do_mobflag(prog_env *env, prog_evt *evt, char *args)
{
	bool op;
	char *arg;
	int flag_idx = 0, flags = 0;

	if (env->owner_type != PROG_TYPE_MOBILE)
		return;
	
	if (*args == '+')
		op = true;
	else if (*args == '-')
		op = false;
	else
		return;

	args += 1;
	skip_spaces(&args);
	while ((arg = tmp_getword(&args)) != NULL) {
		flag_idx = search_block(arg, action_bits_desc, false);
		if (flag_idx != -1)
			flags |= (1 << flag_idx);
	}
	// some flags can't change
	flags &= ~(MOB_SPEC | MOB_ISNPC | MOB_PET);

	if (op)
		MOB_FLAGS(((Creature *)env->owner)) |= flags;
	else
		MOB_FLAGS(((Creature *)env->owner)) &= ~flags;
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
prog_trans_creature(Creature *ch, room_data *targ_room)
{
	room_data *was_in;

	if (!House_can_enter(ch, targ_room->number)
		|| !clan_house_can_enter(ch, targ_room)
		|| (ROOM_FLAGGED(targ_room, ROOM_GODROOM)
			&& !Security::isMember(ch, "WizardFull"))) {
		return;
	}

	was_in = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, targ_room);
	targ_room->zone->enter_count++;

	// Immortal following
	if (ch->followers) {
		struct follow_type *k, *next;

		for (k = ch->followers; k; k = next) {
			next = k->next;
			if (targ_room == k->follower->in_room &&
					GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
					!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
					can_see_creature(k->follower, ch))
				perform_goto(k->follower, targ_room, true);
		}
	}

	if (IS_SET(ROOM_FLAGS(targ_room), ROOM_DEATH)) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			log_death_trap(ch);
			death_cry(ch);
			ch->die();
			//Event::Queue(new DeathEvent(0, ch, false));
		} else {
			mudlog(LVL_GOD, NRM, true,
				"(GC) %s trans-searched into deathtrap %d.",
				GET_NAME(ch), targ_room->number);
		}
	}
}

void
prog_do_trans(prog_env *env, prog_evt *evt, char *args)
{
	room_data *room, *targ_room;
	obj_data *obj;
	int targ_num;
	char *target_arg;

	target_arg = tmp_getword(&args);

	targ_num = atoi(tmp_getword(&args));
	if ((targ_room = real_room(targ_num)) == NULL) {
		errlog("prog trans targ room %d nonexistent.", targ_num);
		return;
	}

	if (!strcmp(target_arg, "self")) {
		// Trans the owner of the prog
		switch (env->owner_type) {
		case PROG_TYPE_OBJECT:
			obj = (obj_data *)env->owner;
			if (obj->carried_by)
				obj_from_char(obj);
			else if (obj->in_room)
				obj_from_room(obj);
			else if (obj->worn_by)
				unequip_char(obj->worn_by, obj->worn_on,
					(obj == GET_EQ(obj->worn_by, obj->worn_on) ?
						MODE_EQ : MODE_IMPLANT), false);
			else if (obj->in_obj)
				obj_from_obj(obj);

			obj_to_room(obj, targ_room);
			break;
		case PROG_TYPE_MOBILE:
			prog_trans_creature((Creature *)env->owner, targ_room);
		  break;
		case PROG_TYPE_ROOM:
		  break;
		}
		return;
	} else if (!strcmp(target_arg, "target")) {
		// Transport the target, which is always a creature
		if (!env->target)
			return;
		prog_trans_creature((Creature *)env->target, targ_room);
		return;
	}

	// The rest of the options deal with multiple creatures

	bool players = false, mobs = false;
	
	if (!strcmp(target_arg, "mobiles"))
		mobs = true;
	else if (!strcmp(target_arg, "players"))
		players = true;
	else if (strcmp(target_arg, "all"))
		errlog("Bad trans argument 1");

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		room = ((Creature *)env->owner)->in_room; break;
	case PROG_TYPE_OBJECT:
		room = ((obj_data *)env->owner)->find_room(); break;
	case PROG_TYPE_ROOM:
		room = ((room_data *)env->owner); break;
	default:
		room = NULL;
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}

	for (CreatureList::iterator it = room->people.begin();
			it != room->people.end();
			++it)
		if ((!players || IS_PC(*it)) &&
				(!mobs || IS_NPC(*it)))
			prog_trans_creature(*it, targ_room);
}

void
prog_do_set(prog_env *env, prog_evt *evt, char *args)
{
	prog_state_data *state;
	prog_var *cur_var;
	char *key;

	switch(env->owner_type) {
	case PROG_TYPE_OBJECT:
	  return;
	case PROG_TYPE_MOBILE:
	  state = GET_MOB_STATE(env->owner);
	  if (!state) {
		CREATE(GET_MOB_STATE(env->owner), prog_state_data, 1);
		state = GET_MOB_STATE(env->owner);
	  }
	  break;
	case PROG_TYPE_ROOM:
	  state = GET_ROOM_STATE(env->owner);
	  if (!state) {
		CREATE(GET_ROOM_STATE(env->owner), prog_state_data, 1);
		state = GET_ROOM_STATE(env->owner);
	  }
	  break;
	default:
	  errlog("Can't happen");
	  return;
	}

	// Now find the variable record.  If they don't have one
	// with the right key, create one
	key = tmp_getword(&args);
	for (cur_var = state->var_list;cur_var;cur_var = cur_var->next)
		if (!strcasecmp(cur_var->key, key))
			break;
	if (!cur_var) {
		CREATE(cur_var, prog_var, 1);
		cur_var->key = strdup(key);
		cur_var->next = state->var_list;
		state->var_list = cur_var;
	}

	if (cur_var->value)
		free(cur_var->value);
	cur_var->value = strdup(args);
}

void
prog_do_oload(prog_env *env, prog_evt *evt, char *args)
{
	obj_data *obj;
	room_data *room = NULL;
	int vnum;
	char *arg;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;
	obj = read_object(vnum);
	if (!obj)
		return;
	obj->creation_method = CREATED_PROG;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "room")) {
		arg = tmp_getword(&args);
		if (arg) {
			// They're specifying a room number
			room = real_room(atoi(arg));
			if (!room)
				return;
		} else {
			// They mean the current room
			switch (env->owner_type) {
			case PROG_TYPE_MOBILE:
				room = ((Creature *)env->owner)->in_room; break;
			case PROG_TYPE_OBJECT:
				room = ((obj_data *)env->owner)->find_room(); break;
			case PROG_TYPE_ROOM:
				room = ((room_data *)env->owner); break;
			default:
				room = NULL;
				errlog("Can't happen at %s:%d", __FILE__, __LINE__);
			}
		}
		obj_to_room(obj, room);
	} else if (!strcasecmp(arg, "target")) {
		if (env->target)
			obj_to_char(obj, env->target);
		else
			extract_obj(obj);
	} else if (!strcasecmp(arg, "self")) {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			obj_to_char(obj, (Creature *)env->owner); break;
		case PROG_TYPE_OBJECT:
			obj_to_obj(obj, (obj_data *)env->owner); break;
		case PROG_TYPE_ROOM:
			obj_to_room(obj, room); break;
		default:
			errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		}
	}
}

void
prog_do_mload(prog_env *env, prog_evt *evt, char *args)
{
	Creature *mob;
	room_data *room = NULL;
	int vnum;
	char *arg;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;
	mob = read_mobile(vnum);
	if (!mob)
		return;

	arg = tmp_getword(&args);
	if (*arg) {
		room = real_room(atoi(arg));
		if (!room)
			return;
	} else {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			room = ((Creature *)env->owner)->in_room; break;
		case PROG_TYPE_OBJECT:
			room = ((obj_data *)env->owner)->find_room(); break;
		case PROG_TYPE_ROOM:
			room = ((room_data *)env->owner); break;
		default:
			room = NULL;
			errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		}
	}
	char_to_room(mob, room);
}

void
prog_do_opurge(prog_env *env, prog_evt *evt, char *args)
{
	obj_data *obj, *obj_list, *next_obj;
	int vnum;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;
	
	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		obj_list = ((Creature *)env->owner)->carrying; break;
	case PROG_TYPE_OBJECT:
		obj_list = ((obj_data *)env->owner)->contains; break;
	case PROG_TYPE_ROOM:
		obj_list = ((room_data *)env->owner)->contents; break;
	default:
		obj_list = NULL;
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}
	
	if (!obj_list)
		return;

	while (obj_list && GET_OBJ_VNUM(obj_list) == vnum) {
		next_obj = obj_list->next_content;
		extract_obj(obj_list);
		obj_list = next_obj;
	}

	if (obj_list) {
		for (obj = obj_list->next_content;obj->next_content;obj = next_obj) {
			if (GET_OBJ_VNUM(obj->next_content) == vnum) {
				next_obj = obj->next_content->next_content;
				extract_obj(obj->next_content);
			} else
				next_obj = obj->next_content;
		}
	}

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		((Creature *)env->owner)->carrying = obj_list; break;
	case PROG_TYPE_OBJECT:
		((obj_data *)env->owner)->contains = obj_list; break;
	case PROG_TYPE_ROOM:
		((room_data *)env->owner)->contents = obj_list; break;
	}
}

void
prog_do_resume(prog_env *env, prog_evt *evt, char *args)
{
	// literally does nothing
}

void
prog_do_echo(prog_env *env, prog_evt *evt, char *args)
{
	char *arg;
	Creature *ch = NULL, *target = NULL;
	obj_data *obj = NULL;

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		ch = ((Creature *)env->owner);
		break;
	case PROG_TYPE_OBJECT:
		obj = ((obj_data *)env->owner);
	case PROG_TYPE_ROOM:
	  // if there's noone in the room, no point in echoing
	  if (((room_data *)env->owner)->people.empty())
		return;
	  // we just pick the top guy off the people list for rooms.
	  ch = *(((room_data *)env->owner)->people.begin());
	  break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}
	target = env->target;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "room")) {
	  act(args, false, ch, obj, target, TO_CHAR);
		act(args, false, ch, obj, target, TO_ROOM);
	} else if (!strcasecmp(arg, "target"))
		act(args, false, ch, obj, target, TO_VICT);
	else if (!strcasecmp(arg, "!target"))
		act(args, false, ch, obj, target, TO_NOTVICT);
}

char *
prog_get_statement(char **prog, int linenum)
{
	char *statement;

	if (linenum)
		*prog = advance_statements(*prog, linenum);

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

	// Terminated, but not freed
	if (env->exec_pt < 0)
		return;

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
	if (!exec) {
		// damn prog disappeared on us
		env->exec_pt = -1;
		return;
	}
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
			exec = advance_statements(exec, env->exec_pt - (cur_line + 1));
		line = prog_get_statement(&exec, 0);
	}

	env->exec_pt = -1;
}

prog_env *
prog_start(int owner_type, void *owner, Creature *target, prog_evt *evt)
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

	if (!prog_get_text(new_prog)) {
	  free(new_prog);
	  return NULL;
	}

	switch (owner_type) {
	case PROG_TYPE_MOBILE:
		new_prog->state = GET_MOB_STATE(owner); break;
	case PROG_TYPE_OBJECT:
	  new_prog->state = NULL; break;
	case PROG_TYPE_ROOM:
		new_prog->state = GET_ROOM_STATE(owner); break;
	}

	prog_next_handler(new_prog, false);

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

	if (prog->evt.args)
		free(prog->evt.args);
	free(prog);
}

void
destroy_attached_progs(void *owner)
{
	struct prog_env *cur_prog;

	for (cur_prog = prog_list;cur_prog;cur_prog = cur_prog->next) {
		if (cur_prog->owner == owner
				|| cur_prog->target == owner
				|| cur_prog->evt.subject == owner
				|| cur_prog->evt.object == owner)
			cur_prog->exec_pt = -1;
	}
}

bool
trigger_prog_cmd(void *owner, int owner_type, Creature *ch, int cmd, char *argument)
{
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (ch == owner)
		return false;
	
	// We don't want an infinite loop with triggered progs that
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
	env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(env);
	
	evt.phase = PROG_EVT_HANDLE;
	evt.args = strdup(argument);
	handler_env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(handler_env);

	evt.phase = PROG_EVT_AFTER;
	evt.args = strdup(argument);
	env = prog_start(owner_type, owner, ch, &evt);
	// note that we don't start executing yet...

	loop_fence -= 1;

	if (handler_env && handler_env->executed)
		return true;

	return handled;
}

bool
trigger_prog_move(void *owner, int owner_type, Creature *ch, special_mode mode)
{
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (ch == owner)
		return false;
	
	// We don't want an infinite loop with mobs triggering progs that
	// trigger a prog, etc.
	if (loop_fence >= 20) {
		mudlog(LVL_IMMORT, NRM, true, "Infinite prog loop halted.");
		return false;
	}
	
	loop_fence++;

	evt.phase = PROG_EVT_BEGIN;
	evt.kind = (mode == SPECIAL_ENTER) ? PROG_EVT_ENTER:PROG_EVT_LEAVE;
	evt.cmd = 0;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = NULL;
	env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(env);
	
	evt.phase = PROG_EVT_HANDLE;
	evt.args = NULL;
	handler_env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(handler_env);

	evt.phase = PROG_EVT_AFTER;
	evt.args = NULL;
	env = prog_start(owner_type, owner, ch, &evt);
	// note that we don't start executing yet...  prog_update_pending()
	// gets called by interpret_command(), after all command processing
	// takes place

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

	env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
	prog_execute(env);
}

void
trigger_prog_give(Creature *ch, Creature *self, struct obj_data *obj)
{
	prog_env *env;
	prog_evt evt;

	if (!self || !self->in_room || !GET_MOB_PROG(self))
		return;
	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_GIVE;
	evt.cmd = -1;
	evt.subject = ch;
	obj ? evt.object = obj : evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = strdup("");

	env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
	prog_execute(env);
}

void
trigger_prog_idle(void *owner, int owner_type)
{
	prog_env *env;
	prog_evt evt;

	// Are we already running a prog?  We're not idle if we are.
	if (find_prog_by_owner(owner))
		return;
	
	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_IDLE;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = NULL;

	// We start an idle mobprog here
	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);
}

void
trigger_prog_load(Creature *owner)
{
	prog_env *env;
	prog_evt evt;

	// Do we have a mobile program?
	if (!GET_MOB_PROG(owner))
		return;
	
	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_LOAD;
	evt.cmd = -1;
	evt.subject = owner;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = NULL;

	env = prog_start(PROG_TYPE_MOBILE, owner, NULL, &evt);
	prog_execute(env);
}

void
trigger_prog_tick(void *owner, int owner_type)
{
	prog_env *env;
	prog_evt evt;

	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_TICK;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	evt.args = NULL;

	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);
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

void
prog_state_free(prog_state_data *state)
{
	struct prog_var *cur_var, *next_var;

	for (cur_var = state->var_list;cur_var;cur_var = next_var) {
		next_var = cur_var->next;
		free(cur_var->key);
		free(cur_var->value);
		free(cur_var);
	}
	free(state);
}

char *prog_get_alias_list(char *args)
{
    char *str;
    int vnum = 0, type = 0; // 0 is a mob, 1 is an object
    struct Creature *mob = NULL;
    struct obj_data *obj = NULL;
    
    str = tmp_getword(&args);
    if ((*str != 'm') && (*str != 'o')) {
        return NULL;
    }
    
    type = (*str == 'm') ? 0 : 1;

    str = tmp_getword(&args);
    if (is_number(str)) {
       vnum = atoi(str); 
    } else {
        return NULL;
    }
    if (type == 0) {
        mob = real_mobile_proto(vnum);
        if (!mob) {
            return NULL;
        }

        str = mob->player.name;
    }
    else {
        obj = real_object_proto(vnum);
        if (!obj) {
            return NULL;
        }

        str = obj->aliases;
    }

    return tmp_strdup(str);
}
