#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
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
#include "prog.h"
#include "clan.h"

struct prog_env *free_progs = NULL;
struct prog_env *prog_list = NULL;
int loop_fence = 0;
extern char locate_buf[256];

// Prog command prototypes
void prog_do_before(prog_env * env, prog_evt * evt, char *args);
void prog_do_handle(prog_env * env, prog_evt * evt, char *args);
void prog_do_after(prog_env * env, prog_evt * evt, char *args);
void prog_do_require(prog_env * env, prog_evt * evt, char *args);
void prog_do_unless(prog_env * env, prog_evt * evt, char *args);
void prog_do_do(prog_env * env, prog_evt * evt, char *args);
void prog_do_silently(prog_env * env, prog_evt * evt, char *args);
void prog_do_force(prog_env * env, prog_evt * evt, char *args);
void prog_do_pause(prog_env * env, prog_evt * evt, char *args);
void prog_do_walkto(prog_env * env, prog_evt * evt, char *args);
void prog_do_driveto(prog_env * env, prog_evt * evt, char *args);
void prog_do_halt(prog_env * env, prog_evt * evt, char *args);
void prog_do_target(prog_env * env, prog_evt * evt, char *args);
void prog_do_nuke(prog_env * env, prog_evt * evt, char *args);
void prog_do_trans(prog_env * env, prog_evt * evt, char *args);
void prog_do_set(prog_env * env, prog_evt * evt, char *args);
void prog_do_oload(prog_env * env, prog_evt * evt, char *args);
void prog_do_mload(prog_env * env, prog_evt * evt, char *args);
void prog_do_opurge(prog_env * env, prog_evt * evt, char *args);
void prog_do_randomly(prog_env * env, prog_evt * evt, char *args);
void prog_do_or(prog_env * env, prog_evt * evt, char *args);
void prog_do_resume(prog_env * env, prog_evt * evt, char *args);
void prog_do_echo(prog_env * env, prog_evt * evt, char *args);
void prog_do_mobflag(prog_env * env, prog_evt * evt, char *args);
void prog_do_ldesc(prog_env * env, prog_evt * evt, char *args);
void prog_do_damage(prog_env * env, prog_evt * evt, char *args);
void prog_do_spell(prog_env *env, prog_evt *evt, char *args);
void prog_do_doorset(prog_env * env, prog_evt * evt, char *args);
void prog_do_selfpurge(prog_env * env, prog_evt * evt, char *args);
void prog_do_hunt(prog_env * env, prog_evt * evt, char *args);
void prog_do_compare_cmd(prog_env *env, prog_evt *evt, char *args);
void prog_do_cond_next_handler(prog_env *env, prog_evt *evt, char *args);
void prog_do_compare_obj_vnum(prog_env *env, prog_evt *evt, char *args);
void prog_do_clear_cond(prog_env *env, prog_evt *evt, char *args);

//external prototypes
struct Creature *real_mobile_proto(int vnum);
struct obj_data *real_object_proto(int vnum);

prog_command prog_cmds[] = {
    {"!ENDOFPROG!", false, prog_do_halt },
	{"halt", false, prog_do_halt},
	{"resume", false, prog_do_resume},
	{"before", false, prog_do_before},
	{"handle", false, prog_do_handle},
	{"after", false, prog_do_after},
	{"or", false, prog_do_or},
	{"do", true, prog_do_do},
	{"!CLRCOND!", false, prog_do_clear_cond},
	{"!CMPCMD!", false, prog_do_compare_cmd},
	{"!CMPOBJVNUM!", false, prog_do_compare_obj_vnum},
	{"!CONDNEXTHANDLER!", false, prog_do_cond_next_handler},
	{"require", false, prog_do_require},
	{"unless", false, prog_do_unless},
	{"randomly", false, prog_do_randomly},
	{"pause", true, prog_do_pause},
	{"walkto", true, prog_do_walkto},
	{"driveto", true, prog_do_driveto},
	{"silently", true, prog_do_silently},
	{"force", true, prog_do_force},
	{"target", true, prog_do_target},
	{"nuke", true, prog_do_nuke},
	{"trans", true, prog_do_trans},
	{"set", true, prog_do_set},
	{"oload", true, prog_do_oload},
	{"mload", true, prog_do_mload},
	{"opurge", true, prog_do_opurge},
	{"echo", true, prog_do_echo},
	{"mobflag", true, prog_do_mobflag},
	{"ldesc", true, prog_do_ldesc},
	{"damage", true, prog_do_damage},
	{"spell", true, prog_do_spell},
	{"doorset", true, prog_do_doorset},
	{"selfpurge", true, prog_do_selfpurge},
	{NULL, false, prog_do_halt}
};

unsigned char *
prog_get_obj(void *owner, prog_evt_type owner_type)
{
	switch (owner_type) {
	case PROG_TYPE_OBJECT:
		break;
	case PROG_TYPE_MOBILE:
		if ((Creature *)owner) {
			return GET_MOB_PROGOBJ(((Creature *)owner));
		} else {
			errlog("Mobile Prog with no owner - Can't happen at %s:%d",
				__FILE__, __LINE__);
			return NULL;
		}
	case PROG_TYPE_ROOM:
		return ((room_data *)owner)->progobj;
    default:
        break;
	}
	errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	return NULL;
}

int
prog_event_handler(void *owner, prog_evt_type owner_type,
                   prog_evt_phase phase,
                   prog_evt_kind kind)
{
    unsigned char *obj = prog_get_obj(owner, owner_type);
    
    if (!obj)
        return 0;
    return *((short *)obj + phase * PROG_EVT_COUNT + kind);
}

void
prog_next_handler(prog_env * env, bool use_resume)
{
    unsigned char *prog;
    
    prog = prog_get_obj(env->owner, env->owner_type);
	// skip over lines until we find another handler (or bust)
    while (*((short *)&prog[env->exec_pt])) {
        short cmd;

        cmd = *((short *)&prog[env->exec_pt]);
        if (cmd == PROG_CMD_BEFORE ||
            cmd == PROG_CMD_HANDLE ||
            cmd == PROG_CMD_AFTER ||
            cmd == PROG_CMD_ENDOFPROG ||
            (use_resume && cmd == PROG_CMD_RESUME))
            return;
        env->exec_pt += sizeof(short) + 2;
    }

    // We reached the end of the prog, so we leave the exec_pt
    // where it is - the end of prog handler will terminate
    // the prog gracefully
}

void
prog_do_before(prog_env * env, prog_evt * evt, char *args)
{
}

void
prog_do_handle(prog_env * env, prog_evt * evt, char *args)
{
}

void
prog_do_after(prog_env * env, prog_evt * evt, char *args)
{
}

bool
prog_var_equal(prog_state_data * state, char *key, char *arg)
{
	struct prog_var *cur_var;

	for (cur_var = state->var_list; cur_var; cur_var = cur_var->next)
		if (!strcasecmp(cur_var->key, key))
			break;
	if (!cur_var || !cur_var->key)
		return !(*arg);
	return !strcasecmp(cur_var->value, arg);
}

char *
prog_get_alias_list(char *args)
{
	char *type, *vnum_str;
	int vnum = 0;
	struct Creature *mob = NULL;
	struct obj_data *obj = NULL;

	type = tmp_getword(&args);
	vnum_str = tmp_getword(&args);

	if (!is_number(vnum_str))
		return NULL;

	vnum = atoi(vnum_str);

	if (tolower(*type) == 'm') {
		mob = real_mobile_proto(vnum);
		if (mob)
			return tmp_strdup(mob->player.name);
	} else if (tolower(*type) == 'o') {
		obj = real_object_proto(vnum);
		if (obj)
			return tmp_strdup(obj->aliases);
	}

	return NULL;
}

bool 
prog_eval_alias(prog_env *env, prog_evt *evt, char *args) {
    char *alias_list = NULL;
    bool result = false;
    char *str, *arg;

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

    return result;
}

bool 
prog_eval_keyword(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *str, *arg;

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

    return result;
}

bool 
prog_eval_abbrev(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *str, *arg;

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

    return result;
}

bool 
prog_eval_holding(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
	int vnum;
	obj_data *obj = NULL;

    vnum = atoi(tmp_getword(&args));
    switch (env->owner_type) {
        case PROG_TYPE_MOBILE:
            obj = ((Creature *) env->owner)->carrying;
            break;
        case PROG_TYPE_OBJECT:
            obj = ((obj_data *) env->owner)->contains;
            break;
        case PROG_TYPE_ROOM:
            obj = ((room_data *) env->owner)->contents;
            break;
        default:
            obj = NULL;
    }
    while (obj) {
        if (GET_OBJ_VNUM(obj) == vnum)
            result = true;
        obj = obj->next_content;
    }

    return result;
}

bool 
prog_eval_phase(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *str;
    int phase;

    str = tmp_getword(&args);
    phase = get_lunar_phase(lunar_day);
    if (strcasecmp(str, "full"))
        result = phase == MOON_FULL;
    else if (strcasecmp(str, "waning"))
        result = phase == MOON_WANE_GIBBOUS
            || phase == MOON_WANE_CRESCENT || phase == MOON_LAST_QUARTER;
    else if (strcasecmp(str, "new"))
        result = phase == MOON_NEW;
    else if (strcasecmp(str, "waxing"))
        result = phase == MOON_WAX_GIBBOUS
            || phase == MOON_WAX_CRESCENT || phase == MOON_FIRST_QUARTER;

    return result;
}

bool 
prog_eval_class(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    // Required class
    char *rclass = tmp_tolower(args);
    // Actual character class
    char *cclass;

	extern const char *pc_char_class_types[];

    if (!env->target)
        return false;

    cclass = tmp_tolower(pc_char_class_types[GET_CLASS(env->target)]);
    if (strstr(rclass, cclass))
        result = true;

	if (IS_REMORT(env->target) &&
        GET_REMORT_CLASS(env->target) != CLASS_NONE) {
		cclass = tmp_tolower(pc_char_class_types[GET_REMORT_CLASS(env->target)]);
		if (strstr(rclass, cclass))
			result = true;
	}

    return result;
}

bool 
prog_eval_vnum(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *arg;

    if (env->target && IS_NPC(env->target)) {
        arg = tmp_getword(&args);
        while (*arg) {
            if (is_number(arg)
                && atoi(arg) == GET_MOB_VNUM(env->target)) {
                result = true;
                break;
            }
            arg = tmp_getword(&args);
        }
    }

    return result;
}

bool 
prog_eval_level(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *arg;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "greater")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
            && GET_LEVEL(env->target) > atoi(arg);
    } else if (!strcasecmp(arg, "less")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
            && GET_LEVEL(env->target) < atoi(arg);
    }

    return result;
}

bool 
prog_eval_gen(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    char *arg;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "greater")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
            && GET_REMORT_GEN(env->target) > atoi(arg);
    } else if (!strcasecmp(arg, "less")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
            && GET_REMORT_GEN(env->target) < atoi(arg);
    }

    return result;
}

bool 
prog_eval_tar_holding(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    obj_data *obj;
    int vnum;

    vnum = atoi(tmp_getword(&args));
    if (env->target)
        obj = env->target->carrying;
    else
        obj = NULL;

    while (obj) {
        if (GET_OBJ_VNUM(obj) == vnum)
            result = true;
        obj = obj->next_content;
    }

    return result;
}

bool 
prog_eval_wearing(prog_env *env, prog_evt *evt, char *args) {
    bool result = false;
    int vnum;

    vnum = atoi(tmp_getword(&args));
    if (env->target) {
        for (int i = 0; i < NUM_WEARS; i++) {
            if ((env->target->equipment[i]
                    && GET_OBJ_VNUM(env->target->equipment[i]) == vnum)
                || (env->target->implants[i]
                    && GET_OBJ_VNUM(env->target->implants[i]) == vnum))
                result = true;
        }
    }

    return result;
}

room_data *
prog_get_owner_room(prog_env *env)
{
    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        return ((Creature *)env->owner)->in_room;
    case PROG_TYPE_ROOM:
        return ((room_data *)env->owner);
    case PROG_TYPE_OBJECT:
        return ((obj_data *)env->owner)->find_room();
    default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    }

    return NULL;
}

// If have a new condition to add and you can do it in
// 3 lines or less, you can add it inline here.  Otherwise
// factor it out into a function
bool
prog_eval_condition(prog_env * env, prog_evt * evt, char *args)
{
	char *arg;
	bool result = false, not_flag = false;

    bool prog_eval_alias(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_keyword(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_abbrev(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_holding(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_phase(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_class(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_vnum(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_level(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_gen(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_tar_holding(prog_env *env, prog_evt *evt, char *args);
    bool prog_eval_wearing(prog_env *env, prog_evt *evt, char *args);

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
        result = prog_eval_alias(env, evt, args);
	} else if (!strcmp(arg, "keyword")) {
        result = prog_eval_keyword(env, evt, args);
	} else if (!strcmp(arg, "abbrev")) {
        result = prog_eval_abbrev(env, evt, args);
	} else if (!strcmp(arg, "fighting")) {
		result = (env->owner_type == PROG_TYPE_MOBILE
			&& ((Creature *) env->owner)->numCombatants());
	} else if (!strcmp(arg, "randomly")) {
		result = number(0, 100) < atoi(args);
	} else if (!strcmp(arg, "variable")) {
		if (env->state) {
			arg = tmp_getword(&args);
			result = prog_var_equal(env->state, arg, args);
		} else if (!*args)
			result = true;
	} else if (!strcasecmp(arg, "holding")) {
        result = prog_eval_holding(env, evt, args);
	} else if (!strcasecmp(arg, "hour")) {
		result = time_info.hours == atoi(tmp_getword(&args));
	} else if (!strcasecmp(arg, "phase")) {
        result = prog_eval_phase(env, evt, args);
    } else if (!strcasecmp(arg, "room")) {
        room_data *room = prog_get_owner_room(env);

        arg = tmp_getword(&args);
        result = (*arg && room) ? (room->number == atoi(arg)):false;
	} else if (!strcasecmp(arg, "target")) {
        // These are all subsets of the *require target <attribute> directive
		arg = tmp_getword(&args);
		if (!strcasecmp(arg, "class")) {
            result = prog_eval_class(env, evt, args);
		} 
        else if (!strcasecmp(arg, "player")) {
			result = env->target && IS_PC(env->target);
		} 
        else if (!strcasecmp(arg, "vnum")) {
            result = prog_eval_vnum(env, evt, args);
		} 
        else if (!strcasecmp(arg, "level")) {
            result = prog_eval_level(env, evt, args);
		} 
        else if (!strcasecmp(arg, "gen")) {
            result = prog_eval_gen(env, evt, args);
		} 
        else if (!strcasecmp(arg, "holding")) {
            result = prog_eval_tar_holding(env, evt, args);
		} 
        else if (!strcasecmp(arg, "wearing")) {
            result = prog_eval_wearing(env, evt, args);
		} 
        else if (!strcasecmp(arg, "self")) {
			result = (env->owner == env->target);
		}
		else if (!strcasecmp(arg, "visible")) {
			if (env->owner_type == PROG_TYPE_MOBILE)
				result = can_see_creature((Creature *)env->owner, env->target);
			else
				result = true;
		}
	}

	return (not_flag) ? (!result) : result;
}

void
prog_do_require(prog_env * env, prog_evt * evt, char *args)
{
	if (!prog_eval_condition(env, evt, args))
		prog_next_handler(env, true);
}

void
prog_do_unless(prog_env * env, prog_evt * evt, char *args)
{
	if (prog_eval_condition(env, evt, args))
		prog_next_handler(env, true);
}

void
prog_do_randomly(prog_env * env, prog_evt * evt, char *args)
{
	unsigned char *exec;
	int cur_pt, last_pt, num_paths;

	// We save the execution point and find the next handler.
    cur_pt = env->exec_pt;
	prog_next_handler(env, true);
	last_pt = env->exec_pt;
	env->exec_pt = cur_pt;

	// now we run through, setting randomly which code path to take
	exec = prog_get_obj(env->owner, env->owner_type);
	num_paths = 0;
	while (cur_pt < last_pt) {
        if (*((short *)&exec[cur_pt]) == PROG_CMD_OR) {
            num_paths += 1;
            if (!number(0, num_paths))
                env->exec_pt = cur_pt + sizeof(short) * 2;
        }
        cur_pt += sizeof(short) * 2;
	}

	// At this point, exec_pt should be on a randomly selected code path
	// within the current handler
}

void
prog_do_or(prog_env * env, prog_evt * evt, char *args)
{
	prog_next_handler(env, true);
}

void
prog_do_do(prog_env * env, prog_evt * evt, char *args)
{
	if (env->owner_type == PROG_TYPE_MOBILE) {
		if (env->target)
			args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *) env->owner, args);
	}
}

void
prog_do_silently(prog_env * env, prog_evt * evt, char *args)
{
	suppress_output = true;
	if (env->owner_type == PROG_TYPE_MOBILE) {
		if (env->target)
			args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *) env->owner, args);
	}
	suppress_output = false;
}

void
prog_do_force(prog_env * env, prog_evt * evt, char *args)
{
	if (!env->target)
		return;

	if (env->owner_type == PROG_TYPE_MOBILE) {
		args = tmp_gsub(args, "$N", fname(env->target->player.name));
		command_interpreter((Creature *) env->target, args);
	}
}

void
prog_do_target(prog_env * env, prog_evt * evt, char *args)
{
	Creature *ch_self;
	obj_data *obj_self;
	char *arg;

	arg = tmp_getword(&args);
	if (!strcasecmp(arg, "random")) {
		switch (env->owner_type) {
            case PROG_TYPE_MOBILE:
                ch_self = (Creature *) env->owner;
                env->target = get_char_random_vis(ch_self, ch_self->in_room);
                break;
            case PROG_TYPE_OBJECT:
                obj_self = (obj_data *) env->owner;
                if (obj_self->in_room)
                    env->target = get_char_random(obj_self->in_room);
                else if (obj_self->worn_by)
                    env->target = get_char_random(obj_self->worn_by->in_room);
                else if (obj_self->carried_by)
                    env->target = get_char_random(obj_self->carried_by->in_room);
                break;
            case PROG_TYPE_ROOM:
                env->target = get_char_random((room_data *) env->owner);
                break;
        default:
            break;
		}
	} else if (!strcasecmp(arg, "opponent")) {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			env->target = ((Creature *) env->owner)->findRandomCombat();
			break;
		default:
			env->target = NULL;
		}
	}
}

void
prog_do_pause(prog_env * env, prog_evt * evt, char *args)
{
	env->wait = MAX(1, atoi(args));
}

void
prog_do_walkto(prog_env * env, prog_evt * evt, char *args)
{
	Creature *ch;
	room_data *room;
	int dir, pause;

	if (env->owner_type != PROG_TYPE_MOBILE)
		return;

	ch = (Creature *) env->owner;

    // Make sure the creature can walk
    if (ch->getPosition() < POS_STANDING)
        return;

	room = real_room(atoi(tmp_getword(&args)));

	if (room && room != ch->in_room) {
		dir = find_first_step(ch->in_room, room, STD_TRACK);
		if (dir >= 0)
			smart_mobile_move(ch, dir);

		// we have to wait at least one second
		pause = atoi(tmp_getword(&args));
		env->wait = MAX(1, pause);

		// we stay on the same line until we get to the destination
		env->exec_pt -= sizeof(short) * 2;
	}
}

void
prog_do_driveto(prog_env * env, prog_evt * evt, char *args)
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

	ch = (Creature *) env->owner;

    // Make sure the creature can drive
    if (ch->getPosition() < POS_SITTING)
        return;

	target_room = real_room(atoi(tmp_getword(&args)));

	// Find the console in the room.  Do nothing if there's no console
	for (console = ch->in_room->contents; console;
		console = console->next_content)
		if (GET_OBJ_TYPE(console) == ITEM_V_CONSOLE)
			break;
	if (!console)
		return;

	// Now find the vehicle vnum that the console points to
	for (vehicle = object_list; vehicle; vehicle = vehicle->next)
		if (GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(console) && vehicle->in_room)
			break;
	if (!vehicle)
		return;

	// Check for engine status
	engine = vehicle->contains;
	if (!engine || !IS_ENGINE(engine) || !ENGINE_ON(engine))
		return;

	if (target_room != vehicle->in_room) {
		dir = find_first_step(vehicle->in_room, target_room, GOD_TRACK);

		// Validate exit the vehicle is going to take
        if (dir >= 0) {
            exit = vehicle->in_room->dir_option[dir];
            if (!exit ||
                !exit->to_room ||
                ROOM_FLAGGED(exit->to_room, ROOM_DEATH | ROOM_INDOORS) ||
                IS_SET(exit->exit_info, EX_CLOSED | EX_ISDOOR))
                return;

            move_car(ch, vehicle, dir);
        }

		// we have to wait at least one second
		pause = atoi(tmp_getword(&args));
		env->wait = MAX(1, pause);

		// we stay on the same line until we get to the destination
		env->exec_pt -= sizeof(short) * 2;
	}
}

void
prog_do_halt(prog_env * env, prog_evt * evt, char *args)
{
	env->exec_pt = -1;
}

void
prog_do_mobflag(prog_env * env, prog_evt * evt, char *args)
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
	while (*(arg = tmp_getword(&args))) {
		flag_idx = search_block(arg, action_bits_desc, false);
		if (flag_idx != -1)
			flags |= (1 << flag_idx);
	}
	// some flags can't change
	flags &= ~(MOB_SPEC | MOB_ISNPC | MOB_PET);

	if (op)
		MOB_FLAGS(((Creature *) env->owner)) |= flags;
	else
		MOB_FLAGS(((Creature *) env->owner)) &= ~flags;
}

void
prog_do_ldesc(prog_env * env, prog_evt * evt, char *args)
{
	Creature *mob;
	obj_data *obj;

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		mob = (Creature *) env->owner;
		if (mob->player.long_descr) {
			if (mob->player.long_descr !=
				mob->mob_specials.shared->proto->player.long_descr)
				free(mob->player.long_descr);
		}
		mob->player.long_descr = str_dup(args);
		break;
	case PROG_TYPE_OBJECT:
		obj = (obj_data *) env->owner;
		if (obj->line_desc && obj->line_desc != obj->shared->proto->line_desc)
			free(obj->line_desc);
		obj->line_desc = str_dup(args);
		break;
	default:
		// do nothing
		break;
	}
}

void
prog_do_damage(prog_env * env, prog_evt * evt, char *args)
{
	room_data *room;
	Creature *mob;
	obj_data *obj;
	int damage_amt;
	char *target_arg;
	int damage_type;

	target_arg = tmp_getword(&args);

	damage_amt = atoi(tmp_getword(&args));
	if (!*args)
		return;
	damage_type = str_to_spell(args);
	if (damage_type == -1)
		return;

	search_nomessage = true;
	if (!strcmp(target_arg, "self")) {
		// Trans the owner of the prog
		switch (env->owner_type) {
		case PROG_TYPE_OBJECT:
			obj = (obj_data *) env->owner;
			damage_eq((obj->worn_by) ? obj->worn_by : obj->carried_by,
				obj, damage_amt, damage_type);
			break;
		case PROG_TYPE_MOBILE:
			mob = (Creature *) env->owner;
            if (mob->getPosition() > POS_DEAD)
                damage(NULL, mob, damage_amt, damage_type, WEAR_RANDOM);
			break;
		case PROG_TYPE_ROOM:
			break;
        default:
            break;
		}
		search_nomessage = false;
		return;
	} else if (!strcmp(target_arg, "target")) {
		// Transport the target, which is always a creature
		if (!env->target) {
			search_nomessage = false;
			return;
		}
        if (env->target->getPosition() > POS_DEAD)
            damage(NULL, env->target, damage_amt, damage_type, WEAR_RANDOM);
		search_nomessage = false;
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

    room = prog_get_owner_room(env);
    if (!room)
        return;

	for (CreatureList::iterator it = room->people.begin();
		it != room->people.end(); ++it)
		if ((!players || IS_PC(*it)) &&
            (!mobs || IS_NPC(*it)) &&
            (*it)->getPosition() > POS_DEAD)
			damage(NULL, *it, damage_amt, damage_type, WEAR_RANDOM);
	search_nomessage = false;
}

void
prog_do_spell(prog_env *env, prog_evt *evt, char *args)
{
	room_data *room;
	Creature *caster = NULL;
	obj_data *obj;
	int spell_num, spell_lvl, spell_type;
	char *target_arg;

    // Parse out arguments
	target_arg = tmp_getword(&args);
	if (!*args)
		return;

    spell_lvl = atoi(tmp_getword(&args));
    if (spell_lvl > 49)
        return;
	if (!*args)
		return;
	spell_num = str_to_spell(args);
	if (spell_num == -1)
		return;

    // Set locate_buf to avoid accidents
    locate_buf[0] = '\0';

    // Determine the spell type
    if (SPELL_IS_MAGIC(spell_num) || SPELL_IS_DIVINE(spell_num))
        spell_type = CAST_SPELL;
    else if (SPELL_IS_PHYSICS(spell_num))
        spell_type = CAST_PHYSIC;
    else if (SPELL_IS_PSIONIC(spell_num))
        spell_type = CAST_PSIONIC;
    else
        spell_type = CAST_ROD;

    // Determine the caster of the spell, so to speak
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        obj = (obj_data *) env->owner;
        caster = (obj->worn_by) ? obj->worn_by : obj->carried_by;
        break;
    case PROG_TYPE_MOBILE:
        caster = (Creature *)env->owner;
        break;
    case PROG_TYPE_ROOM:
        caster = *(((room_data *)env->owner)->people.begin());
        break;
    default:
        break;
    }

	search_nomessage = true;
	if (!strcmp(target_arg, "self")) {
		// Cast a spell on the owner of the prog
		switch (env->owner_type) {
		case PROG_TYPE_OBJECT:
			obj = (obj_data *) env->owner;
			call_magic(caster, caster, obj, NULL,
                       spell_num, spell_lvl, spell_type, NULL);
			break;
		case PROG_TYPE_MOBILE:
            if (caster->getPosition() > POS_DEAD
                && !affected_by_spell(caster, spell_num))
                call_magic(caster, caster, NULL, NULL,
                           spell_num, spell_lvl, spell_type,
                           NULL);
			break;
		case PROG_TYPE_ROOM:
			break;
        default:
            break;
		}
		search_nomessage = false;
		return;
	} else if (!strcmp(target_arg, "target")) {
		// Cast a spell on the target
		if (!env->target) {
			search_nomessage = false;
			return;
		}
        if (env->target->getPosition() > POS_DEAD
            && !affected_by_spell(env->target, spell_num))
                call_magic(caster, env->target, NULL, NULL,
                           spell_num, spell_lvl, spell_type,
                           NULL);
		search_nomessage = false;
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

    room = prog_get_owner_room(env);
    if (!room)
        return;

	for (CreatureList::iterator it = room->people.begin();
		it != room->people.end(); ++it)
		if ((!players || IS_PC(*it)) &&
            (!mobs || IS_NPC(*it)) &&
            (*it)->getPosition() > POS_DEAD &&
            !affected_by_spell(*it, spell_num))
            call_magic(caster, *it, NULL, NULL,
                       spell_num, spell_lvl, spell_type,
                       NULL);
	search_nomessage = false;
}

void
prog_do_doorset(prog_env * env, prog_evt * evt, char *args)
{
	// *doorset <room vnum> <direction> +/- <doorflags>
	bool op;
	char *arg;
	room_data *room;
	int dir, flag_idx = 0, flags = 0;

	arg = tmp_getword(&args);
	room = real_room(atoi(arg));
	if (!room)
		return;

	arg = tmp_getword(&args);
	dir = search_block(arg, dirs, false);
	if (dir < 0)
		return;

	if (*args == '+')
		op = true;
	else if (*args == '-')
		op = false;
	else
		return;

	args += 1;
	skip_spaces(&args);
	while (*(arg = tmp_getword(&args))) {
		flag_idx = search_block(arg, exit_bits, false);
		if (flag_idx != -1)
			flags |= (1 << flag_idx);
	}

	if (!ABS_EXIT(room, dir)) {
		CREATE(ABS_EXIT(room, dir), struct room_direction_data, 1);
		ABS_EXIT(room, dir)->to_room = NULL;
	}

	if (op)
		ABS_EXIT(room, dir)->exit_info |= flags;
	else
		ABS_EXIT(room, dir)->exit_info &= ~flags;
}

void
prog_do_selfpurge(prog_env * env, prog_evt * evt, char *args)
{
	if (env->owner_type == PROG_TYPE_MOBILE) {
		prog_do_nuke(env, evt, args);
		env->exec_pt = -1;
		((Creature *) env->owner)->purge(true);
		env->owner = NULL;
	}
}

void
prog_do_hunt(prog_env * env, prog_evt * evt, char *args)
{
	if (env->owner_type == PROG_TYPE_MOBILE && env->target) {
		((Creature *) env->owner)->startHunting(env->target);
	}
}

void
prog_do_clear_cond(prog_env *env, prog_evt *evt, char *args)
{
	env->condition = 0;
}

void
prog_do_compare_cmd(prog_env *env, prog_evt *evt, char *args)
{
	// FIXME: nasty hack
	if (!env->condition)
		env->condition = (evt->cmd == *((int *)args));
}

void
prog_do_compare_obj_vnum(prog_env *env, prog_evt *evt, char *args)
{
	// FIXME: nasty hack
	if (!env->condition)
		env->condition = (evt->object_type == PROG_TYPE_OBJECT
						&& evt->object
						&& ((obj_data *)evt->object)->getVnum() == *((int *)args));
}

void
prog_do_cond_next_handler(prog_env *env, prog_evt *evt, char *args)
{
	if (!env->condition)
		prog_next_handler(env, false);
}

void
prog_do_nuke(prog_env * env, prog_evt * evt, char *args)
{
	struct prog_env *cur_prog;

	for (cur_prog = prog_list; cur_prog; cur_prog = cur_prog->next)
		if (cur_prog != env && cur_prog->owner == env->owner)
			cur_prog->exec_pt = -1;
}

void
prog_trans_creature(Creature * ch, room_data * targ_room)
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
				!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING)
				&& can_see_creature(k->follower, ch))
				perform_goto(k->follower, targ_room, true);
		}
	}

	if (IS_SET(ROOM_FLAGS(targ_room), ROOM_DEATH)) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			log_death_trap(ch);
			death_cry(ch);
			ch->die();
		} else {
			mudlog(LVL_GOD, NRM, true,
				"(GC) %s trans-searched into deathtrap %d.",
				GET_NAME(ch), targ_room->number);
		}
	}
}

void
prog_do_trans(prog_env * env, prog_evt * evt, char *args)
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
			obj = (obj_data *) env->owner;
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
			prog_trans_creature((Creature *) env->owner, targ_room);
			break;
		case PROG_TYPE_ROOM:
			break;
        default:
            break;
		}
		return;
	} else if (!strcmp(target_arg, "target")) {
		// Transport the target, which is always a creature
		if (!env->target)
			return;
		prog_trans_creature((Creature *) env->target, targ_room);
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

    room = prog_get_owner_room(env);
    if (!room)
        return;

	for (CreatureList::iterator it = room->people.begin();
		it != room->people.end(); ++it)
		if ((!players || IS_PC(*it)) && (!mobs || IS_NPC(*it)))
			prog_trans_creature(*it, targ_room);
}

void
prog_do_set(prog_env * env, prog_evt * evt, char *args)
{
	prog_state_data *state;
	prog_var *cur_var;
	char *key;

	switch (env->owner_type) {
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
	for (cur_var = state->var_list; cur_var; cur_var = cur_var->next)
		if (!strcasecmp(cur_var->key, key))
			break;
	if (!cur_var) {
		CREATE(cur_var, prog_var, 1);
		strcpy(cur_var->key, key);
		cur_var->next = state->var_list;
		state->var_list = cur_var;
	}

	strcpy(cur_var->value, args);
}

void
prog_do_oload(prog_env * env, prog_evt * evt, char *args)
{
	obj_data *obj;
	room_data *room = NULL;
	int vnum, max_load = -1, target_num = -1;
	char *target_str, *arg;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;
	target_str = tmp_getword(&args);

	arg = tmp_getword(&args);
	if (is_number(arg)) {
		target_num = atoi(arg);
		arg = tmp_getword(&args);
	}

	if (!strcasecmp(arg, "max"))
		max_load = atoi(tmp_getword(&args));

	obj = real_object_proto(vnum);
	if (!obj ||
		(max_load != -1 &&
			obj->shared->number - obj->shared->house_count >= max_load))
		return;

	obj = read_object(vnum);
	if (!obj)
		return;
	obj->creation_method = CREATED_PROG;
	switch (env->owner_type) {
	case PROG_TYPE_OBJECT:
		obj->creator = GET_OBJ_VNUM((obj_data *)env->owner); break;
	case PROG_TYPE_ROOM:
		obj->creator = ((room_data *)env->owner)->number; break;
	case PROG_TYPE_MOBILE:
		obj->creator = GET_MOB_VNUM((Creature *)env->owner); break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}

	if (!*target_str || !strcasecmp(target_str, "room")) {
		if (target_num == -1) {
			// They mean the current room
            room = prog_get_owner_room(env);
		} else {
			// They're specifying a room number
			room = real_room(target_num);
		}
		if (!room) {
			extract_obj(obj);
			return;
		}
		obj_to_room(obj, room);
	} else if (!strcasecmp(target_str, "target")) {
		if (env->target)
			obj_to_char(obj, env->target);
		else
			extract_obj(obj);
	} else if (!strcasecmp(target_str, "self")) {
		switch (env->owner_type) {
		case PROG_TYPE_MOBILE:
			obj_to_char(obj, (Creature *) env->owner);
			break;
		case PROG_TYPE_OBJECT:
			obj_to_obj(obj, (obj_data *) env->owner);
			break;
		case PROG_TYPE_ROOM:
			obj_to_room(obj, (room_data *) env->owner);
			break;
		default:
			errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		}
	} else {
		// Bad target str, so we need to free the object
		extract_obj(obj);
	}
}

void
prog_do_mload(prog_env * env, prog_evt * evt, char *args)
{
	Creature *mob;
	room_data *room = NULL;
	int vnum, max_load = -1;
	char *arg;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;
	mob = real_mobile_proto(vnum);
	if (!mob)
		return;

	arg = tmp_getword(&args);
	if (*arg && strcasecmp(arg, "max")) {
		room = real_room(atoi(arg));
		if (!room)
			return;
		arg = tmp_getword(&args);
	} else {
        room = prog_get_owner_room(env);
	}
	if (*arg && !strcasecmp(arg, "max"))
		max_load = atoi(tmp_getword(&args));

	if (max_load == -1 || mob->mob_specials.shared->number < max_load) {
		mob = read_mobile(vnum);
		char_to_room(mob, room);
		if (GET_MOB_PROG(mob))
			trigger_prog_load(mob);
	}
}

void
prog_do_opurge(prog_env * env, prog_evt * evt, char *args)
{
	obj_data *obj, *obj_list, *next_obj;
	int vnum;

	vnum = atoi(tmp_getword(&args));
	if (vnum <= 0)
		return;

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		obj_list = ((Creature *) env->owner)->carrying;
		break;
	case PROG_TYPE_OBJECT:
		obj_list = ((obj_data *) env->owner)->contains;
		break;
	case PROG_TYPE_ROOM:
		obj_list = ((room_data *) env->owner)->contents;
		break;
	default:
		obj_list = NULL;
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}

	if (!obj_list)
		return;

	// Eliminate the selected objects at the head of the list
	while (obj_list && GET_OBJ_VNUM(obj_list) == vnum) {
		next_obj = obj_list->next_content;
		extract_obj(obj_list);
		obj_list = next_obj;
	}

	if (obj_list) {
		// There are still objects in the list, so iterate through
		// them, extracting the ones of the proper vnum
		for (obj = obj_list->next_content; obj; obj = next_obj) {
			next_obj = obj->next_content;
			if (GET_OBJ_VNUM(obj) == vnum)
				extract_obj(obj);
		}
	}

	switch (env->owner_type) {
	case PROG_TYPE_MOBILE:
		((Creature *) env->owner)->carrying = obj_list;
		break;
	case PROG_TYPE_OBJECT:
		((obj_data *) env->owner)->contains = obj_list;
		break;
	case PROG_TYPE_ROOM:
		((room_data *) env->owner)->contents = obj_list;
		break;
    default:
        break;
	}
}

void
prog_do_resume(prog_env * env, prog_evt * evt, char *args)
{
	// literally does nothing
}

void
prog_do_echo(prog_env * env, prog_evt * evt, char *args)
{
	char *arg;
	Creature *ch = NULL, *target = NULL;
	obj_data *obj = NULL;
	room_data *room = NULL;

	arg = tmp_getword(&args);
    room = prog_get_owner_room(env);
    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        ch = ((Creature *)env->owner); break;
    case PROG_TYPE_OBJECT:
        obj = ((obj_data *)env->owner); break;
    case PROG_TYPE_ROOM:
		// if there's noone in the room and it's not a zecho,
        // no point in echoing
		if (room->people.empty() && strcasecmp(arg, "zone"))
			return;
		// we just pick the top guy off the people list for rooms.
		ch = *(((room_data *) env->owner)->people.begin());
        break;
    default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__); break;
    }

	target = env->target;
	if (!strcasecmp(arg, "room")) {
		act(args, false, ch, obj, target, TO_CHAR);
		act(args, false, ch, obj, target, TO_ROOM);
	} else if (!strcasecmp(arg, "target")) {
		act(args, false, ch, obj, target, TO_VICT);
    } else if (!strcasecmp(arg, "!target")) {
		act(args, false, ch, obj, target, TO_NOTVICT);
        if (ch != target) {
            act(args, false, ch, obj, target, TO_CHAR);
        }
    } else if (!strcasecmp(arg, "zone")) {
		for (struct descriptor_data * pt = descriptor_list; pt; pt = pt->next) {
			if (pt->input_mode == CXN_PLAYING &&
				pt->creature && pt->creature->in_room
				&& pt->creature->in_room->zone == room->zone
				&& !PLR_FLAGGED(pt->creature, PLR_OLC | PLR_WRITING)) {
				act(args, false, pt->creature, obj, target, TO_CHAR);
			}
		}
	}
}

void
prog_execute(prog_env *env)
{
	unsigned char *exec;
    int cmd, arg_addr;

	// Called with NULL environment
	if (!env)
		return;

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

    exec = prog_get_obj(env->owner, env->owner_type);
    if (!exec) {
        // Damn prog disappeared on us
        env->exec_pt = -1;
        return;
    }

    while (env->exec_pt >= 0 &&
           *((short *)&exec[env->exec_pt]) &&
           env->wait == 0) {
        // Get the command and the arg address
        cmd = *((short *)(exec + env->exec_pt));
        arg_addr = *((short *)(exec + env->exec_pt + sizeof(short)));
        // Set the execution point to the next command by default
        env->exec_pt += sizeof(short) * 2;
        // Call the handler for the command
        prog_cmds[cmd].func(env, &env->evt, (char *)exec + arg_addr);
        // If the command did something, count it
        if (prog_cmds[cmd].count)
            env->executed +=1 ;
    }

    if (!env->wait)
        env->exec_pt = -1;
}

prog_env *
prog_start(prog_evt_type owner_type, void *owner, Creature * target, prog_evt * evt)
{
	prog_env *new_prog;
    int initial_exec_pt;

    initial_exec_pt = prog_event_handler(owner, owner_type, evt->phase, evt->kind);
    if (!initial_exec_pt)
		return NULL;

	if (free_progs) {
		new_prog = free_progs;
		free_progs = free_progs->next;
	} else {
		CREATE(new_prog, struct prog_env, 1);
	}
	new_prog->next = prog_list;
	prog_list = new_prog;

	new_prog->owner_type = owner_type;
	new_prog->owner = owner;
    new_prog->exec_pt = initial_exec_pt;
	new_prog->executed = 0;
	new_prog->wait = 0;
	new_prog->speed = 0;
	new_prog->target = target;
	new_prog->evt = *evt;

	switch (owner_type) {
	case PROG_TYPE_MOBILE:
		new_prog->state = GET_MOB_STATE(owner);
		break;
	case PROG_TYPE_OBJECT:
		new_prog->state = NULL;
		break;
	case PROG_TYPE_ROOM:
		new_prog->state = GET_ROOM_STATE(owner);
		break;
    default:
        break;
	}

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
            errlog("prog_free() called on prog not in prog_list!");
            return;
		}
		prev_prog->next = prog->next;
	}

	prog->next = free_progs;
	free_progs = prog;
}

void
destroy_attached_progs(void *owner)
{
	struct prog_env *cur_prog;

	for (cur_prog = prog_list; cur_prog; cur_prog = cur_prog->next) {
		if (cur_prog->owner == owner ||
            cur_prog->target == owner ||
            cur_prog->evt.subject == owner ||
            cur_prog->evt.object == owner)
			cur_prog->exec_pt = -1;
	}
}

void
prog_unreference_object(obj_data *obj)
{
	struct prog_env *cur_prog;

	for (cur_prog = prog_list; cur_prog; cur_prog = cur_prog->next) {
		if (cur_prog->evt.object_type == PROG_TYPE_OBJECT
				&& cur_prog->evt.object == obj) {
			cur_prog->evt.object_type = PROG_TYPE_NONE;
			cur_prog->evt.object = NULL;
		}
	}
}

bool
trigger_prog_cmd(void *owner, prog_evt_type owner_type, Creature * ch, int cmd,
	char *argument)
{
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (ch == owner)
		return false;
    if (!prog_get_obj(owner, owner_type))
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
	strcpy(evt.args, argument);
	env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(env);

    // Check for death or destruction of the owner
    if (env && !env->owner)
        return true;

	evt.phase = PROG_EVT_HANDLE;
	strcpy(evt.args, argument);
	handler_env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(handler_env);

    // Check for death or destruction of the owner
    if (handler_env && !handler_env->owner)
        return true;

	evt.phase = PROG_EVT_AFTER;
	strcpy(evt.args, argument);
	env = prog_start(owner_type, owner, ch, &evt);
	// note that we don't start executing yet...

	loop_fence -= 1;

	if (handler_env && handler_env->executed)
		return true;

	return handled;
}

bool
trigger_prog_spell(void *owner, prog_evt_type owner_type, Creature * ch, int cmd)
{
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (ch == owner)
		return false;

    if (!prog_get_obj(owner, owner_type))
        return false;

	// We don't want an infinite loop with triggered progs that
	// trigger a prog, etc.
	if (loop_fence >= 20) {
		mudlog(LVL_IMMORT, NRM, true, "Infinite prog loop halted.");
		return false;
	}

	loop_fence++;

	evt.phase = PROG_EVT_BEGIN;
	evt.kind = PROG_EVT_SPELL;
	evt.cmd = cmd;
	evt.subject = ch;
	evt.object = NULL;			//this should perhaps be updated to hold the target of the spell
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");
	env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(env);

	evt.phase = PROG_EVT_HANDLE;
	strcpy(evt.args, "");
	handler_env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(handler_env);

	evt.phase = PROG_EVT_AFTER;
	strcpy(evt.args, "");
	env = prog_start(owner_type, owner, ch, &evt);
	// note that we don't start executing yet...

	loop_fence -= 1;

	if (handler_env && handler_env->executed)
		return true;

	return handled;
}


bool
trigger_prog_move(void *owner, prog_evt_type owner_type, Creature * ch,
	special_mode mode)
{
	prog_env *env, *handler_env;
	prog_evt evt;
	bool handled = false;

	if (ch == owner)
		return false;

    if (!prog_get_obj(owner, owner_type))
        return false;

	// We don't want an infinite loop with mobs triggering progs that
	// trigger a prog, etc.
	if (loop_fence >= 20) {
		mudlog(LVL_IMMORT, NRM, true, "Infinite prog loop halted.");
		return false;
	}

	loop_fence++;

	evt.phase = PROG_EVT_BEGIN;
	evt.kind = (mode == SPECIAL_ENTER) ? PROG_EVT_ENTER : PROG_EVT_LEAVE;
	evt.cmd = 0;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");
	env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(env);

	evt.phase = PROG_EVT_HANDLE;
	strcpy(evt.args, "");
	handler_env = prog_start(owner_type, owner, ch, &evt);
	prog_execute(handler_env);

	evt.phase = PROG_EVT_AFTER;
	strcpy(evt.args, "");
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
trigger_prog_fight(Creature * ch, Creature * self)
{
	prog_env *env;
	prog_evt evt;


	if (!self || !self->in_room || !GET_MOB_PROG(self))
		return;
    if (!GET_MOB_PROGOBJ(self))
        return;
	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_FIGHT;
	evt.cmd = -1;
	evt.subject = ch;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");

	env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
	prog_execute(env);
}

void
trigger_prog_death(void *owner, prog_evt_type owner_type)
{
	prog_env *env;
	prog_evt evt;

    if (!prog_get_obj(owner, owner_type))
        return;

	// We don't want an infinite loop with triggered progs that
	// trigger a prog, etc.
	if (loop_fence >= 20) {
		mudlog(LVL_IMMORT, NRM, true, "Infinite prog loop halted.");
		return;
	}

	loop_fence++;

	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_DEATH;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");
	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);

	loop_fence -= 1;
}

void
trigger_prog_give(Creature * ch, Creature * self, struct obj_data *obj)
{
	prog_env *env;
	prog_evt evt;

	if (!self || !self->in_room || !GET_MOB_PROGOBJ(self))
		return;

	if (!obj) {
		errlog("trigger_prog_give() called with no object!");
		return;
	}

	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_GIVE;
	evt.cmd = -1;
	evt.subject = ch;
	evt.object = obj;
	evt.object_type = PROG_TYPE_OBJECT;
	strcpy(evt.args, "");

	env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
	prog_execute(env);
}

void
trigger_prog_idle(void *owner, prog_evt_type owner_type)
{
	prog_env *env;
	prog_evt evt;

    if (!prog_get_obj(owner, owner_type))
        return;

	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_IDLE;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");

	// We start an idle mobprog here
	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);
}

//handles idle combat actions
void
trigger_prog_combat(void *owner, prog_evt_type owner_type)
{
	prog_env *env;
	prog_evt evt;

    if (!prog_get_obj(owner, owner_type))
        return;

	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_COMBAT;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");

	// We start a combat prog here
	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);
}

void
trigger_prog_load(Creature * owner)
{
	prog_env *env;
	prog_evt evt;

	// Do we have a mobile program?
	if (!GET_MOB_PROGOBJ(owner))
		return;

	evt.phase = PROG_EVT_AFTER;
	evt.kind = PROG_EVT_LOAD;
	evt.cmd = -1;
	evt.subject = owner;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");

	env = prog_start(PROG_TYPE_MOBILE, owner, NULL, &evt);
	prog_execute(env);
}

void
trigger_prog_tick(void *owner, prog_evt_type owner_type)
{
	prog_env *env;
	prog_evt evt;

    if (!prog_get_obj(owner, owner_type))
        return;

	evt.phase = PROG_EVT_HANDLE;
	evt.kind = PROG_EVT_TICK;
	evt.cmd = -1;
	evt.subject = NULL;
	evt.object = NULL;
	evt.object_type = PROG_TYPE_NONE;
	strcpy(evt.args, "");

	env = prog_start(owner_type, owner, NULL, &evt);
	prog_execute(env);
}

void
prog_update(void)
{
	struct prog_env *cur_prog, *next_prog;
	CreatureList::iterator cit, end;
	zone_data *zone;
	room_data *room;

	// Unmark mobiles
    end = characterList.end();
	for (cit = characterList.begin();cit != end;++cit)
		(*cit)->mob_specials.prog_marker = 0;
	// Unmark rooms
	for (zone = zone_table; zone; zone = zone->next)  {
		if (ZONE_FLAGGED(zone, ZONE_FROZEN)
				|| zone->idle_time >= ZONE_IDLE_TIME)
			continue;

		for (room = zone->world; room; room = room->next)
			if (GET_ROOM_PROG(room))
				room->prog_marker = 0;
	}


	// Execute progs and mark them as non-idle
	for (cur_prog = prog_list; cur_prog; cur_prog = cur_prog->next) {
		if (cur_prog->exec_pt == -1)
			continue;

		if (!cur_prog->owner) {
			errlog("Prog without owner");
			continue;
		}
			
		switch (cur_prog->owner_type) {
		case PROG_TYPE_OBJECT:
			break;
		case PROG_TYPE_MOBILE:
			((Creature *)cur_prog->owner)->mob_specials.prog_marker = 1; break;
		case PROG_TYPE_ROOM:
			((room_data *)cur_prog->owner)->prog_marker = 1; break;
        default:
            break;
		}

		prog_execute(cur_prog);
	}
	// Free threads that have terminated
	for (cur_prog = prog_list; cur_prog; cur_prog = next_prog) {
		next_prog = cur_prog->next;
		if (cur_prog->exec_pt < 0 || !cur_prog->owner)
			prog_free(cur_prog);
	}

	// Trigger mobile idle and combat progs
    end = characterList.end();
	for (cit = characterList.begin();cit != end;++cit) {
		if ((*cit)->mob_specials.prog_marker || !GET_MOB_PROGOBJ(*cit))
			continue;
		else if (!(*cit)->numCombatants())
			trigger_prog_idle((*cit), PROG_TYPE_MOBILE);
		else
			trigger_prog_combat((*cit), PROG_TYPE_MOBILE);
	}

	// Trigger room idle progs
	for (zone = zone_table; zone; zone = zone->next)  {
		if (ZONE_FLAGGED(zone, ZONE_FROZEN)
				|| zone->idle_time >= ZONE_IDLE_TIME)
			continue;

		for (room = zone->world; room; room = room->next)
			if (GET_ROOM_PROGOBJ(room) && !room->prog_marker)
				trigger_prog_idle(room, PROG_TYPE_ROOM);
	}

}

void
prog_update_pending(void)
{
	struct prog_env *cur_prog;

	if (!prog_list)
		return;
    
	for (cur_prog = prog_list; cur_prog; cur_prog = cur_prog->next)
		if (cur_prog->executed == 0)
			prog_execute(cur_prog);

}

int
prog_count(bool total)
{
	int result = 0;
	prog_env *cur_env;

	for (cur_env = prog_list; cur_env; cur_env = cur_env->next)
		if (total || cur_env->exec_pt > 0)
			result++;
	return result;
}

int
free_prog_count(void)
{
	int result = 0;
	prog_env *cur_env;

	for (cur_env = free_progs; cur_env; cur_env = cur_env->next)
		result++;
	return result;
}

void
prog_state_free(prog_state_data * state)
{
	struct prog_var *cur_var, *next_var;

	for (cur_var = state->var_list; cur_var; cur_var = next_var) {
		next_var = cur_var->next;
		free(cur_var);
	}
	free(state);
}

