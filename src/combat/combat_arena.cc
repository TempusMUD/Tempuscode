
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "structs.h"
#include "character_list.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "combat.h"
#include "handler.h"
#include "screen.h"
#include "bomb.h"
#include "house.h"

combat_data *battles = NULL;
carena_data *the_arenas = NULL;
int combat_number = 0;
int combat_on = 1;
int num_combats = 0;
FILE *combatfile = NULL;
int num_arenas = 0;
long holding_room = 0;
long starting_room = 0;


#define BOOTY_ROOM 1205
#define BATTLE_START_ROOM 1204
#define HOLDING_ROOM 1202
#define MAX_COMBATS 30

const struct ctypes {
	char *combattype;
	int fee;
} ctypes[] = {
	{
	"One on One", 0}, {
	"Free for All", 0}, {
	"\n", 0}
};

const char *combat_bits[] = {
	"Started",
	"Open",
	"Invite",
	"Draw",
	"Ladder",
	"\n"
};

const char *clog_types[] = {
	"off",
	"brief",
	"normal",
	"complete",
	"\n"
};

const struct cc_options {
	char *keyword;
	char *usage;
	int level;
	int in_room;				//must be in BATTLE_START_ROOM to use this command
} cc_options[] = {
	{
	"create", "<type><name>", 30, 1}, {
	"show", "", 30, 0}, {
	"join", "<number>", 30, 1}, {
	"open", "", 30, 1}, {
	"start", "", 30, 1}, {
	"close", "", 30, 1}, {
	"invite", "", 30, 1}, {
	"sacrifice", "<object name>/view/undo", 30, 1}, {
	"arena", "<arena number/random>", 30, 1}, {
	"say", "<message>", 30, 0}, {
	"describe", "", 30, 1}, {
	"end", "", 30, 1}, {
	"leave", "", 30, 0}, {
	"approve", "", 30, 1}, {
	"show_arenas", "", 30, 0}, {
	"reimburse", "", 30, 0},
//    { "fee",       "<number>",                                          30, 1 },
	{
	NULL, NULL, 0, 0}			// list terminator
};


const struct cc_wizoptions {
	char *keyword;
	char *usage;
	int level;
} cc_wizoptions[] = {
	{
	"lock", "", LVL_GOD}, {
	"destroy", "<number>", LVL_GOD}, {
	"reload", "", LVL_GOD}, {
	"stats", "", LVL_GOD}, {
	"clear_booty", "", LVL_GOD}, {
	NULL, NULL, 0}
};


ACMD(do_ccontrol)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int com;

	strcpy(arg2, argument);

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);


	if (!combat_on && (GET_LEVEL(ch) < LVL_GOD)) {
		send_to_char(ch, "Sorry combat is currently wizlocked.\r\n");
		return;
	}

	if (!*arg1) {
		do_ccontrol_options(ch);
		return;
	}
	for (com = 0;; com++) {
		if (!cc_options[com].keyword) {
			do_ccontrol_wizoptions(ch, arg2);
			return;
		}
		if (is_abbrev(arg1, cc_options[com].keyword))
			break;
	}

	if ((IN_ROOM(ch)->number != starting_room) && (cc_options[com].in_room)) {
		send_to_char(ch, 
			"You have to be in the battle starting room to use that command.\r\n");
		return;
	}

	switch (com) {
	case 0:					// create
		do_ccontrol_create(ch, argument, com);
		break;
	case 1:					// show
		do_ccontrol_show(ch, argument);
		break;
	case 2:					// join 
		do_ccontrol_join(ch, argument);
		break;
	case 3:
		do_ccontrol_open(ch);
		break;
	case 4:
		do_ccontrol_start(ch);
		break;
	case 5:
		do_ccontrol_close(ch);
		break;
	case 7:
		do_ccontrol_sacrifice(ch, argument);
		break;
	case 8:
		do_ccontrol_arena(ch, argument);
		break;
	case 9:
		do_ccontrol_say(ch, argument);
		break;
	case 10:
		do_ccontrol_describe(ch);
		break;
	case 11:
		do_ccontrol_end(ch);
		break;
	case 12:
		do_ccontrol_leave(ch);
		break;
	case 13:
		do_ccontrol_approve(ch);
		break;
	case 14:
		show_arenas(ch);
		break;
	case 15:
		do_ccontrol_reimburse(ch);
		break;
	case 6:
	default:
		send_to_char(ch, "Combat option not implemented.\r\n");
		break;
	}
}

void
do_ccontrol_options(CHAR * ch)
{
	int i = 0;
	int j = 0;

	strcpy(buf, "combat options:\r\n");
	while (1) {
		if (!cc_options[i].keyword)
			break;
		sprintf(buf, "%s  %s%-15s %s%s%s\r\n",
			buf, CCCYN(ch, C_NRM), cc_options[i].keyword, CCYEL(ch, C_NRM),
			cc_options[i].usage, CCNRM(ch, C_NRM));
		i++;
	}

	if (GET_LEVEL(ch) >= LVL_GOD) {
		strcat(buf, "Wiz combat options:\r\n");

		while (1) {

			if (!cc_wizoptions[j].keyword) {
				break;
			}

			if (GET_LEVEL(ch) >= cc_wizoptions[j].level) {
				sprintf(buf, "%s  %s%-15s %s%s%s\r\n",
					buf, KCYN, cc_wizoptions[j].keyword, KYEL,
					cc_wizoptions[j].usage, KNRM);
			}

			j++;
		}

	}
	page_string(ch->desc, buf);
}

void
do_ccontrol_wizoptions(CHAR * ch, char *argument)
{

	int com = 0;
	char arg1[MAX_INPUT_LENGTH];

	skip_spaces(&argument);
	argument = one_argument(argument, arg1);

	if (GET_LEVEL(ch) < LVL_GOD) {
		send_to_char(ch, "Unknown combat option, '%s'.\r\n", arg1);
		return;
	}

	for (com = 0;; com++) {
		if (!cc_wizoptions[com].keyword) {
			send_to_char(ch, "Unknown combat option, '%s'.\r\n", arg1);
			return;
		}
		if (is_abbrev(arg1, cc_wizoptions[com].keyword))
			break;
	}

	switch (com) {
	case 0:
		do_ccontrol_lock(ch);
		break;
	case 1:
		do_ccontrol_destroy(ch, argument);
		break;
	case 2:
		num_arenas = build_arena_list();
		send_to_char(ch, "%d arenas reloaded.\r\n", num_arenas);
		break;
	case 3:
		do_ccontrol_stats(ch);
		break;
	case 4:
		clear_booty_rooms();
		break;
	default:
		send_to_char(ch, "That wiz combat option is not implemented.\r\n");
		break;
	}


}

void
do_ccontrol_usage(CHAR * ch, int com)
{
	if (com < 0)
		do_ccontrol_options(ch);
	else {
		send_to_char(ch, "Usage: combat %s%s %s%s%s\r\n",
			CCCYN(ch, C_NRM), cc_options[com].keyword, CCYEL(ch, C_NRM),
			(cc_options[com].usage), CCNRM(ch, C_NRM));
	}
}

void
do_ccontrol_create(CHAR * ch, char *argument, int com)
{
	char arg1[MAX_INPUT_LENGTH];
	int type;
	combat_data *combat = NULL;

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (!ch) {
		return;
	}

	if (IN_COMBAT(ch)) {
		send_to_char(ch, "You are already in a combat jerky.\r\n");
		return;
	}

	if (!*arg1 || !*argument) {
		do_ccontrol_usage(ch, com);
		return;
	}


	if ((type = find_combat_type(arg1)) < 0) {
		send_to_char(ch,
			"Invalid combat type '%s'.\r\n"
			"Use 'combat help types'.\r\n", arg1);
		return;
	}

	if (GET_GOLD(ch) < ctypes[type].fee) {
		send_to_char(ch, "You don't have the required fee to start a %s\r\n",
			ctypes[type].combattype);
		return;
	}


	if (strlen(argument) >= MAX_COMBAT_NAME) {
		send_to_char(ch, "Battle name too long.  Max length %d characters.\r\n",
			MAX_COMBAT_NAME - 1);
		return;
	}

	GET_GOLD(ch) -= ctypes[type].fee;
	combat = create_combat(ch, type, argument);
	if (!combat) {
		return;
	}
	combat->next = battles;
	battles = combat;


	send_to_char(ch, "Combat '%s' created.\r\n", combat->name);
	sprintf(buf, "%d", combat->vnum);	// Cheesy
	do_ccontrol_join(ch, buf);
	num_combats++;

}

void
do_ccontrol_show(CHAR * ch, char *argument)
{

	int timediff;
	combat_data *combat = NULL;
	struct zone_data *zn = NULL;
	char *timestr_s;
	char timestr_a[16];
	char bitbuf[100];
	char arena_name[MAX_INPUT_LENGTH];

	combat = battles;

	if (combat == NULL) {
		send_to_char(ch, "No battles are currently active.\r\n");
		return;
	}

	for (combat = battles;;) {
		if (combat == NULL) {
			return;
		}
		timestr_s = asctime(localtime(&combat->started));
		*(timestr_s + strlen(timestr_s) - 1) = '\0';

		for (zn = zone_table; zn; zn = zn->next) {
			if (combat->arena->zone == 0) {
				sprintf(arena_name, "None.");
			}

			else if (zn->number == combat->arena->zone) {
				sprintf(arena_name, "%s", zn->name);
			}
		}

		// combat is still active

		sprintbit(combat->flags, combat_bits, bitbuf);

		timediff = time(0) - combat->started;
		sprintf(timestr_a, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);
		send_to_char(ch, "------------------------\r\n");
		sprintf(buf,
			"%s%-10s%10s%s\r\n"
			"%sCreator:%s %-30s%s\r\n"
			"%sDescription:%s %-30s%s\r\n"
			"  %sType:%s            %s%s\r\n"
			"  %sStarted:%s         %s%s\r\n"
			"  %sAge:%s             %s%s\r\n"
			"  %sNum Players:%s     %d%s\r\n"
			"  %sMax Players:%s     %d%s\r\n"
			"  %sArena:%s           %s%s\r\n"
			"  %sID Number:%s       %d%s\r\n"
			"  %sSacrificed:%s      %s%s\r\n",
			KYEL, CAP(combat->name), bitbuf, KNRM,
			KCYN, KYEL, CAP(get_name_by_id(combat->creator)), KNRM,
			KCYN, KYEL, combat->description ? combat->description : "None.",
			KNRM, KCYN, KYEL, ctypes[combat->type].combattype, KNRM, KCYN,
			KYEL, timestr_s, KNRM, KCYN, KYEL, timestr_a, KNRM, KCYN, KYEL,
			combat->num_players, KNRM, KCYN, KYEL, combat->max_combatants,
			KNRM, KCYN, KYEL, arena_name ? arena_name : "No Arena", KNRM, KCYN,
			KYEL, combat->vnum, KNRM, KCYN, KYEL,
			combat->sacrificed ? "Yes" : "No", KNRM);

		if (combat->num_players) {
			list_combat_players(ch, combat, buf2);
			strcat(buf, buf2);
		}

		strcat(buf, "\r\n");
		page_string(ch->desc, buf);

		send_to_char(ch, "------------------------\r\n");

		combat = combat->next;
	}
}

void
do_ccontrol_join(CHAR * ch, char *argument)
{

	int battle_number = 0;
	int i = 0;
	struct combat_data *the_combat = NULL;
	char buf[MAX_INPUT_LENGTH];




	if (IN_COMBAT(ch)) {
		send_to_char(ch, "You are already in a combat.\r\n");
		return;
	}


	if (battles == NULL) {
		send_to_char(ch, "There is not a battle to join.\r\n");
		return;
	}


	if (!argument || !strcmp(argument, "")) {
		for (i = 0; i <= 1000; i++) {
			the_combat = combat_by_vnum(i);
			if (the_combat) {
				battle_number = the_combat->vnum;
				break;
			}
		}
	} else {
		battle_number = atoi(argument);
		the_combat = combat_by_vnum(battle_number);
	}

	if (!the_combat) {
		send_to_char(ch, "Cannot join combat #%d.\r\n", battle_number);
		return;
	}


	if (COMBAT_FLAGGED(the_combat, COMBAT_STARTED)) {
		send_to_char(ch, "Battle '%s' has already started.\r\n", the_combat->name);
		return;
	}

	if (the_combat->type == CTYPE_ONE_ONE
		&& !idnum_in_combat(GET_IDNUM(ch), the_combat)) {
		if (the_combat->num_players >= 2) {
			send_to_char(ch,
				"Battle '%s' is a one-on-one battle and is currently full.\r\n",
				the_combat->name);
			return;
		}
	}

	if (COMBAT_FLAGGED(the_combat, COMBAT_INVITE)
		&& GET_IDNUM(ch) != the_combat->creator) {
		send_to_char(ch, "Battle '%s' is an invite only battle.\r\n",
			the_combat->name);
		return;
	}

	if (GET_GOLD(ch) < the_combat->fee) {
		send_to_char(ch, "You do not have enough gold to enter the battle.\r\n");
		return;
	}


	if (COMBAT_FLAGGED(the_combat, COMBAT_OPEN)
		|| GET_IDNUM(ch) == the_combat->creator) {
		if (!(idnum_in_combat(GET_IDNUM(ch), the_combat))) {
			send_to_char(ch, "You have joined combat '%s'\r\n", the_combat->name);
			sprintf(buf, "%s has joined the combat!", GET_NAME(ch));
			send_to_combat(buf, the_combat);
			add_idnum_to_combat(GET_IDNUM(ch), the_combat);
			SET_BIT(PLR2_FLAGS(ch), PLR2_COMBAT);
			GET_GOLD(ch) -= the_combat->fee;
			return;

		} else {
			send_to_char(ch, "You are already in a battle.\r\n");
			return;
		}
	}

	return;

}

void
do_ccontrol_open(CHAR * ch)
{

	int num = 0;
	struct combat_data *combat = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!combat) {
		send_to_char(ch, "You aren't even in a combat.\r\n");
		return;
	}

	if (GET_IDNUM(ch) != combat->creator) {
		send_to_char(ch, 
			"Only the battle creator can open a battle to the public.\r\n");
		return;
	}

	REMOVE_BIT(combat->flags, COMBAT_INVITE);
	SET_BIT(combat->flags, COMBAT_OPEN);
	send_to_char(ch, "The battle is now open to the general public.\r\n");


	return;
}

void
do_ccontrol_close(CHAR * ch)
{
	int num = 0;
	struct combat_data *combat = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);


	if (!combat) {
		send_to_char(ch, "You aren't even in a combat.\r\n");
		return;
	}

	if (GET_IDNUM(ch) != combat->creator) {
		send_to_char(ch, "Only the battle creator can close a battle.\r\n");
		return;
	}

	REMOVE_BIT(combat->flags, COMBAT_OPEN);
	SET_BIT(combat->flags, COMBAT_INVITE);
	send_to_char(ch, "The battle is now closed.\r\n");


	return;
}

void
do_ccontrol_start(CHAR * ch)
{

	char buf[MAX_INPUT_LENGTH];
	int num = 0;
	int i = 0;
	struct combat_data *combat = NULL;
	struct char_data *unapproved = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);



	if (!ch || !combat) {
		return;
	}

	if (GET_IDNUM(ch) != combat->creator) {
		send_to_char(ch, "Only the battle creator can start a battle.\r\n");
		return;
	}

	if (combat->arena->zone == 0) {
		send_to_char(ch, 
			"You have to choose an arena before you start genius.\r\n");
		return;
	}


	for (i = 0; i < combat->num_players; i++) {
		if (!combat->players[i].approved) {
			unapproved = get_char_in_world_by_idnum(combat->players[i].idnum);
			if (unapproved) {
				send_to_char(ch, "%s has not submitted approval.\r\n",
					GET_NAME(unapproved));
				return;
			}
			continue;
		}
	}

	switch (combat->type) {

	case CTYPE_ONE_ONE:
		if (combat->num_players != 2) {
			send_to_char(ch, 
				"You need two combatants for a one on one battle.\r\n");
			return;
		}
		break;
	case CTYPE_FREE_FOR_ALL:
		if (combat->num_players < 2) {
			send_to_char(ch, 
				"You need at least two combatants for a free for all.\r\n");
			return;
		}
		break;
	default:
		break;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, 
			"The battle is already started...better check yourself before you wreck yourself.\r\n");
		return;
	} else {
		REMOVE_BIT(combat->flags, COMBAT_OPEN);
		REMOVE_BIT(combat->flags, COMBAT_INVITE);
		SET_BIT(combat->flags, COMBAT_STARTED);
		sprintf(buf, "Battle '%s' has started.", combat->name);
		send_to_combat(buf, combat);

		// Here is where we will trans all the players to random locations in the arena
		trans_combatants(combat);
		return;
	}

	return;
}

void
do_ccontrol_sacrifice(CHAR * ch, char *argument)
{
	int num = 0;
	struct obj_data *obj = NULL;
	struct combat_data *combat = NULL;
	struct room_data *rm = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);



	if (!combat) {
		send_to_char(ch, "You aren't even in combat.\r\n");
		return;
	}

	rm = real_room(combat->arena->booty_room);
	if (!rm) {
		slog("Error with booty room.");
		send_to_char(ch, "Error with booty room....please report.\r\n");
		return;
	}

	if (!idnum_in_combat(GET_IDNUM(ch), combat)) {
		send_to_char(ch, "You have to be in the combat to sacrifice.r\n");
		return;
	}

	if (!strncmp(argument, "view", 4)) {
		look_at_room(ch, rm, 0);
		return;
	}

	if (!strncmp(argument, "undo", 4)) {
		return_sacrifices(combat);
		return;
	}

	if (combat->arena->zone == 0) {
		send_to_char(ch, "Choose an arena first.\r\n");
		return;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, "It's too late to sacrifice.\r\n");
		return;
	}


// Take an object from a character's inventory and send it to the booty room
	obj = get_obj_in_list_vis(ch, argument, ch->carrying);
	if (!obj) {
		send_to_char(ch, "You can't find that to sacrifice.\r\n");
		return;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
		if (GET_OBJ_VAL(obj, 3)) {
			send_to_char(ch, "Hey, no sacrificing CORPSES!!!!\r\n");
			return;
		}
	}

	if (IS_BOMB(obj)) {
		send_to_char(ch, "No sacrificing bombs!!!!\r\n");
		return;
	}

	else {
		obj_from_char(obj);
		obj_to_room(obj, rm);

		// Kill 2 birds with 1 stone.  We don't want sigil'd objects being sac'd so we'll use the sigil to show who sac'd the eq 
		GET_OBJ_SIGIL_IDNUM(obj) = GET_IDNUM(ch);
		GET_OBJ_SIGIL_LEVEL(obj) = -1;
		act("$n sacrifices $p for the combat.", FALSE, ch, obj, 0, TO_ROOM);
		act("You sacrifice $p for the combat.", FALSE, ch, obj, 0, TO_CHAR);
		combat->sacrificed = 1;
	}


}

void
do_ccontrol_arena(CHAR * ch, char *argument)
{
	int num = 0;
	int arena_num = 0;
	char arena_name[MAX_INPUT_LENGTH];
	struct carena_data *the_arena = NULL;
	struct zone_data *zn = NULL;

	struct combat_data *combat = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!combat) {
		send_to_char(ch, "You aren't even in combat.\r\n");
		return;
	}

	if (GET_IDNUM(ch) != combat->creator) {
		send_to_char(ch, "Only the battle creator can choose the arena.\r\n");
		return;
	}

	if (is_abbrev(argument, "random")) {
		random_arena(ch, combat);

		for (zn = zone_table; zn; zn = zn->next) {
			if (combat->arena->zone == 0) {
				sprintf(arena_name, "None.");
			}

			else if (zn->number == combat->arena->zone) {
				sprintf(arena_name, "%s", zn->name);
			}
		}

		send_to_char(ch, "Ok using %s as the arena.\r\n", arena_name);

		return;
	}

	arena_num = atoi(argument);

	the_arena = arena_by_num(arena_num);
	if (!the_arena) {
		send_to_char(ch, "No such arena.\r\n");
		return;
	}

	if (the_arena->used) {
		send_to_char(ch, "That arena is already in use.\r\n");
		return;
	}

	if (combat->arena) {
		combat->arena->used = 0;
	}

	if (combat->sacrificed) {
		//  Return the sacrifices
		return_sacrifices(combat);
	}




	combat->arena = the_arena;

	for (zn = zone_table; zn; zn = zn->next) {
		if (combat->arena->zone == 0) {
			sprintf(arena_name, "None.");
		}

		else if (zn->number == combat->arena->zone) {
			sprintf(arena_name, "%s", zn->name);
		}


	}

	send_to_char(ch, "Ok using %s as the arena.\r\n", arena_name);

	the_arena->used = 1;


}

void
do_ccontrol_say(CHAR * ch, char *argument)
{
	int num = -1;
	struct combat_data *combat = NULL;

	num = char_in_combat_vnum(ch);

	if (num >= 0) {
		combat = combat_by_vnum(num);

		if (combat) {
			say_to_combat(argument, ch, combat);
			return;
		}
	}
}

void
do_ccontrol_describe(CHAR * ch)
{
	int num;
	struct combat_data *combat = NULL;


	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!combat) {
		send_to_char(ch, "You need to be in a combat to describe it.\r\n");
		return;
	}

	if (combat->creator != GET_IDNUM(ch)) {
		send_to_char(ch, "Only the combat creator can write the description.\r\n");
		return;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, 
			"The combat has already started...no need to describe it now.\r\n");
		return;
	}



	if (combat) {
		if (combat->description) {
			act("$n begins to edit a combat description.", TRUE, ch, 0, 0,
				TO_ROOM);
		} else {
			act("$n begins to write a combat description.", TRUE, ch, 0, 0,
				TO_ROOM);
		}

		start_text_editor(ch->desc, &combat->description, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	}


}

void
do_ccontrol_end(CHAR * ch)
{
	int num;
	struct combat_data *combat = NULL;
	char buf[MAX_INPUT_LENGTH];

	num = char_in_combat_vnum(ch);
	if (num >= 1) {
		combat = combat_by_vnum(num);
	} else {
		send_to_char(ch, "You need to be in the combat to end it.\r\n");
		return;
	}

	if (!combat || !ch) {
		return;
	}

	if (combat->creator != GET_IDNUM(ch)) {
		send_to_char(ch, "Only the combat creator can end a combat.\r\n");
		return;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, "The combat has already started it must be WON!\r\n");
		return;
	}

	sprintf(buf, "%s has ended '%s'", GET_NAME(ch), combat->name);
	send_to_combat(buf, combat);

	if (combat->sacrificed) {
		return_sacrifices(combat);
	}

	if (combat->arena) {
		combat->arena->used = 0;
	}



	remove_players(combat);
	remove_combat(combat);
	num_combats--;


}

void
do_ccontrol_leave(CHAR * ch)
{
	int num = 0;
	struct combat_data *combat = NULL;
	char buf[MAX_INPUT_LENGTH];

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!ch) {
		return;
	}

	if (!IN_COMBAT(ch) || (!combat)) {
		send_to_char(ch, "You aren't in a combat.\r\n");
		return;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, 
			"The battle has already begun....it's too late to chicken out now.\r\n");
		return;
	}


	if (combat->sacrificed) {
		return_sacrifices(combat);
	}

	if (combat->creator == GET_IDNUM(ch)) {
		send_to_combat("Combat creator has left...combat ending.", combat);
		if (combat->arena) {
			combat->arena->used = 0;
		}

		remove_players(combat);
		remove_combat(combat);
		num_combats--;
		return;
	}

	remove_player(ch);
	sprintf(buf, "%s has left the combat.", GET_NAME(ch));
	send_to_combat(buf, combat);
	send_to_char(ch, "You have left the combat.\r\n");

	if (combat->num_players < 1) {

		if (combat->arena) {
			combat->arena->used = 0;
		}

		remove_players(combat);
		remove_combat(combat);
		num_combats--;
	}

}

void
do_ccontrol_reimburse(CHAR * ch)
{
	struct combat_data *combat = NULL;
	struct room_data *rm = NULL;
	struct obj_data *obj = NULL;
	struct obj_data *next_o = NULL;
	int items = 0;

	send_to_char(ch, "Searching for reimbursements.\r\n");

	for (combat = battles; combat; combat = combat->next) {
		combat_reimburse(ch, combat);
	}

	rm = real_room(holding_room);
	if (rm) {
		for (obj = rm->contents; obj; obj = next_o) {
			next_o = obj->next_content;
			if (GET_OBJ_SIGIL_IDNUM(obj) == GET_IDNUM(ch)
				&& GET_OBJ_SIGIL_LEVEL(obj) == -1) {
				slog("!");
				GET_OBJ_SIGIL_IDNUM(obj) = 0;
				GET_OBJ_SIGIL_LEVEL(obj) = 0;
				obj_from_room(obj);
				obj_to_char(obj, ch);
				items++;
			}
		}

		if (items > 0) {
			send_to_char(ch, "%d items have been reimbursed.\r\n", items);
		} else {
			send_to_char(ch, "No reimbursements found");
		}

	}

	return;
}

void
do_ccontrol_destroy(CHAR * ch, char *argument)
{

	int num = 0;
	struct combat_data *combat = NULL;
	char buf[MAX_INPUT_LENGTH];

	num = atoi(argument);
	combat = combat_by_vnum(num);


	if (!combat) {
		send_to_char(ch, "Combat %d does not exist, check combat show.\r\n", num);
		return;
	}

	sprintf(buf, "%s has destroyed '%s'", GET_NAME(ch), combat->name);
	slog(buf);
	send_to_combat("Combat destroyed!", combat);

	send_to_char(ch, "Ok combat idnum %d has been destroyed.\r\n", num);

	if (combat->sacrificed) {
		return_sacrifices(combat);
	}

	if (combat->arena) {
		combat->arena->used = 0;
	}

	remove_players(combat);
	remove_combat(combat);
	num_combats--;
}

void
do_ccontrol_lock(CHAR * ch)
{

	char buf2[100];
	struct combat_data *the_combat = NULL;

	if (combat_on) {
		combat_on = 0;
		sprintf(buf2, "%sLOCKED%s", KRED, KNRM);
	} else {
		combat_on = 1;
		sprintf(buf2, "%sunlocked%s", KCYN, KNRM);
	}

	sprintf(buf, "Combat System %s by %s", combat_on ? "unlocked" : "LOCKED",
		GET_NAME(ch));
	slog(buf);

	send_to_char(ch, "Combat is now %s.\r\n", buf2);

	if (!combat_on) {
		for (the_combat = battles;;) {
			if (the_combat == NULL) {
				return;
			}

			if (the_combat->arena) {
				the_combat->arena->used = 0;
			}

			remove_players(the_combat);
			remove_combat(the_combat);
			send_to_combat
				("The combat system has temporarily been locked.\r\n",
				the_combat);
			the_combat = the_combat->next;
		}
	}
	num_combats = 0;
}

void
do_ccontrol_approve(struct char_data *ch)
{
	struct combat_data *combat = NULL;
	struct cplayer_data *cplayer = NULL;
	int num = 0;
	int i = 0;
	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);


	if (!IN_COMBAT(ch)) {
		send_to_char(ch, "You aren't in a combat dork.\r\n");
		return;
	}

	if (COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		send_to_char(ch, "The combat has already started LOSER!.\r\n");
		return;
	}

	for (i = 0; i <= combat->num_players; i++) {
		if (GET_IDNUM(ch) == combat->players[i].idnum) {
			cplayer = &combat->players[i];
		}
	}

	if (cplayer == NULL) {
		slog("Cplayer is Null....darnit");
		return;
	}


	if (cplayer->approved == 0) {
		send_to_char(ch, "You have given your approval.\r\n");
		cplayer->approved = 1;
		return;
	} else {
		send_to_char(ch, "You have withdrawn your approval of the combat.\r\n");
		cplayer->approved = 0;
		return;
	}


}


void
do_ccontrol_fee(struct char_data *ch, char *arg)
{
	struct combat_data *combat = NULL;
	int num = 0;
	long f = 0;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!arg) {
		send_to_char(ch, "What do you want to set the fee to?");
		return;
	}

	if (!IN_COMBAT(ch) || !combat) {
		send_to_char(ch, "You aren't in a combat dork.\r\n");
		return;
	}

	if (!COMBAT_CREATOR(ch, combat)) {
		send_to_char(ch, "Only the combat creator can set the fee.");
		return;
	}

	if (combat->num_players > 1) {
		send_to_char(ch, 
			"The combat must have only one combatant to set the fee.\r\n");
		return;
	}

	f = atol(arg);
	if (f >= 0 && f <= 3000000) {
		combat->fee = f;
	}

	sprintf(buf, "Combat fee set to %ld.", combat->fee);
	send_to_combat(buf, combat);

}

void
do_ccontrol_stats(CHAR * ch)
{
	send_to_char(ch, "Combat Starting Room: %ld \r\n"
		"Combat Holding  Room: %ld \r\n"
		"Number of Combats: %d \r\n",
		starting_room, holding_room, num_combats);
}

int
boot_combat(void)
{
	if (!(combatfile = fopen(COMBATFILENAME, "a+"))) {
		slog("SYSERR: unable to open combat file.");
		return 0;
	}

	num_arenas = build_arena_list();
	clear_booty_rooms();


	comlog(NULL, "Combat System Rebooted.", TRUE, FALSE);
	return 1;
}

combat_data *
create_combat(CHAR * ch, int type, char *name)
{

	struct carena_data *dummy_arena = NULL;

	combat_data *combat = NULL;

	dummy_arena = create_arena(0, 0);
	CREATE(combat, combat_data, 1);

	combat_number++;
	combat->vnum = combat_number;
	combat->creator = GET_IDNUM(ch);
	combat->owner_level = GET_LEVEL(ch);
	combat->flags = COMBAT_INVITE;
	combat->fee = 0;
	combat->arena = dummy_arena;
	combat->max_combatants = 0;
	combat->type = type;
	combat->started = time(0);
	combat->ended = 0;
	combat->name = str_dup(name);
	combat->description = NULL;
	combat->players = NULL;
	combat->num_players = 0;
	combat->winner = 0;
	combat->booty_room = real_room(BOOTY_ROOM);
	combat->sacrificed = 0;

	sprintf(buf, "created %s combat, '%s'", ctypes[type].combattype, name);
	comlog(ch, buf, FALSE, FALSE);
	return combat;

}

void
comlog(CHAR * ch, char *str, int file, int to_char)
{
	time_t ct;
	char *tmstr;
	//CHAR *vict = NULL;
	//char buf[MAX_STRING_LENGTH];
	CharacterList::iterator vict = characterList.begin();
	for (; vict != characterList.end(); ++vict) {
		if ((*vict == ch) && !to_char) {
			continue;
		}
		if (IN_COMBAT(*vict)) {
			sprintf(buf,
				"%s~*~%s COMBATLOG: %s %s %s~*~%s\r\n",
				CCYEL_BLD((*vict), C_NRM), CCNRM_GRN((*vict), C_NRM),
				ch ? PERS(ch, (*vict)) : "",
				str, CCYEL_BLD((*vict), C_NRM), CCNRM((*vict), C_NRM));
			send_to_char(*vict, "%s", buf);

		}
	}
	if (file) {
		sprintf(buf, "%s %s\n", ch ? GET_NAME(ch) : "", str);
		ct = time(0);
		tmstr = asctime(localtime(&ct));
		*(tmstr + strlen(tmstr) - 1) = '\0';

		if (combatfile) {
			fprintf(combatfile, "%-19.19s :: %s", tmstr, buf);
			fflush(combatfile);
		}

	}
}


cplayer_data *
idnum_in_combat(int idnum, combat_data * combat)
{
	int i;

	if (combat) {

		for (i = 0; i < combat->num_players; i++)
			if (combat->players[i].idnum == idnum)
				return (&combat->players[i]);
	}

	return NULL;
}

int
char_in_combat_vnum(CHAR * ch)
{
	int i;
	struct combat_data *combat = NULL;

	for (combat = battles;;) {

		if (!combat) {
			return -1;
		}

		for (i = 0; i < combat->num_players; i++) {
			if (combat->players[i].idnum == GET_IDNUM(ch)) {
				return combat->vnum;
			}
		}

		combat = combat->next;
	}
	return -1;
}

int
add_idnum_to_combat(int idnum, combat_data * combat)
{

	cplayer_data *newplayers = NULL;

	// we need a bigger array
	if (combat->max_combatants <= combat->num_players) {
		if (!(newplayers = (cplayer_data *) realloc(combat->players,
					sizeof(cplayer_data) * (combat->num_players + 1)))) {
			slog("SYSERR: Error allocating new player array.\r\n");
			return 0;
		}
		combat->players = newplayers;
		combat->max_combatants++;
	}

	combat->players[combat->num_players].idnum = idnum;
	combat->players[combat->num_players].flags = 0;
	combat->players[combat->num_players].approved = 0;

	combat->num_players++;

	return (combat->num_players);
}

int
remove_idnum_from_combat(int idnum, combat_data * combat)
{
	int i;

	for (i = 0; i < combat->num_players; i++)
		if (combat->players[i].idnum == idnum)
			break;

	if (i >= combat->num_players) {
		slog("SYSERR: error finding player idnum in remove_idnum_from_combat.");
		return 0;
	}

	for (++i; i < combat->num_players; i++) {
		combat->players[i - 1].idnum = combat->players[i].idnum;
		combat->players[i - 1].flags = combat->players[i].flags;
	}


	combat->num_players--;

	return 1;
}

int
find_combat_type(char *argument)
{
	int i = 0;

	while (1) {
		if (*ctypes[i].combattype == '\n')
			break;
		if (is_abbrev(argument, ctypes[i].combattype))
			return i;
		i++;
	}
	return (-1);
}

void
list_combat_players(CHAR * ch, combat_data * combat, char *outbuf)
{
	char buf[MAX_STRING_LENGTH], name[1024];
	int i, num_online, num_offline;
	CHAR *vict = NULL;

	strcpy(buf, "  -Online Players-------\r\n");
	for (i = num_online = num_offline = 0; i < combat->num_players; i++) {

		sprintf(name, "%s%s%s -- %s(%s)%s", KCYN,
			CAP(get_name_by_id(combat->players[i].idnum)), KNRM, KYEL,
			((combat->players[i].approved) ? "approved" : "unapproved"), KNRM);
		if (!*name) {
			strcat(buf, "BOGUS player idnum!\r\n");
			slog("SYSERR: bogus player idnum in list_combat_players.");
			break;
		}
		strcpy(name, CAP(name));

		// player is in world and visible
		if ((vict = get_char_in_world_by_idnum(combat->players[i].idnum)) &&
			CAN_SEE(ch, vict)) {

			sprintf(buf, "%s  %2d. %-15s\r\n", buf, ++num_online, name);

		}
	}
	if (outbuf)
		strcpy(outbuf, buf);
	else
		page_string(ch->desc, buf);

}

int
check_battles(combat_data * combat)
{

	// First make sure that the battle was started
	if (!COMBAT_FLAGGED(combat, COMBAT_STARTED)) {
		return 0;
	}
	// This is where we check to see if the battle has a decisive winner
	switch (combat->type) {

	case 0:					// Clan War
		end_battle(combat);
		break;
	case 1:					// One on One
		if (combat->num_players == 1) {	// There can be only ONE!
			combat->winner = combat->players[0].idnum;
			sprintf(buf, "Battle '%s' won by %s", combat->name,
				get_name_by_id(combat->winner));
			comlog(NULL, buf, TRUE, TRUE);

			end_battle(combat);
			return 1;
		}
		break;
	case 2:					// Free for All
		if (combat->num_players == 1) {	// Last man standing
			combat->winner = combat->players[0].idnum;
			sprintf(buf, "Battle '%s' won by %s", combat->name,
				get_name_by_id(combat->winner));
			comlog(NULL, buf, TRUE, TRUE);
			end_battle(combat);
			return 1;
		}
		break;

	default:
		break;
	}
	return 0;
}

int
check_teams(combat_data * combat)
{
	// So what we want to do is loop through all the teams and make sure that
	// there are at least 2 teams that still have participants
	int j, team;
	int team_score[MAX_TEAMS];
	int players = combat->num_players;
	for (j = 0; j <= players; j++) {
		if (combat->players != NULL) {
			team = combat->players[j].team;
			team_score[team]++;
		}
	}

	if (team_score[0] == 0) {
		comlog(NULL, "Team 0  has been eliminated.\r\n", TRUE, TRUE);
		return 0;
	}

	if (team_score[1] == 0) {
		comlog(NULL, "Team 1 has been eliminated.\r\n", TRUE, TRUE);
		return 0;
	}

	return 1;

}

int
end_battle(combat_data * combat)
{
	struct char_data *ch = NULL;
	struct room_data *booty_room = NULL;
	struct obj_data *obj = NULL;

	booty_room = real_room(combat->arena->booty_room);

	// loop through contents of the room and remove sigils
	if (booty_room) {
		for (obj = booty_room->contents; obj; obj = obj->next_content) {
			slog("@");
			GET_OBJ_SIGIL_IDNUM(obj) = 0;
			GET_OBJ_SIGIL_LEVEL(obj) = 0;
		}
	}
	// Trans winner(s) to the booty room


	switch (combat->type) {
	case 0:					// CLAN WAR loop through the winning clans players still in battle and trans them to the booty room
		check_teams(combat);
		break;
	case 1:
	case 2:
		ch = get_char_in_world_by_idnum(combat->winner);
		if (ch) {

			sprintf(buf, "%s%s is the victor!%s", KNRM_BLD, GET_NAME(ch),
				KNRM);
			send_to_combat(buf, combat);
			send_to_char(ch, "There can be only one!\r\n");
			char_from_room(ch,false);
			char_to_room(ch, real_room(combat->arena->booty_room),false);
			look_at_room(ch, ch->in_room, 0);
		}
		break;
	default:
		break;
	}

	if (combat->arena) {
		combat->arena->used = 0;
	}
	remove_players(combat);
	remove_combat(combat);

	return 0;
}

struct room_data *
random_arena_room(struct combat_data *combat, CHAR * ch)
{
	int rando = 0;				// The ones place
	int randt = 0;				// The tens place

	int room_num = -1;			// This is the room they will end up being transd to
	int num_tries = 0;
	struct room_data *trans_room = NULL;

	while (!trans_room && (num_tries < 50)) {
		num_tries++;
		rando = random_number_zero_low(9);
		randt = random_number_zero_low(9);

		sprintf(buf, "%ld%d%d", combat->arena->zone, randt, rando);
		room_num = atoi(buf);

		if (room_num == combat->arena->booty_room
			|| room_num == BATTLE_START_ROOM) {
			random_arena_room(combat, ch);
		}

		trans_room = real_room(room_num);


		if (trans_room) {

			if (ROOM_FLAGGED(trans_room, ROOM_NOTEL)
				|| ROOM_FLAGGED(trans_room, ROOM_GODROOM)) {
				random_arena_room(combat, ch);
			}

			return trans_room;
		}

	}
	slog("SYSERR: random_arena_room bailing after 50 tries...bummer.");
	return NULL;
}

void
random_arena(CHAR * ch, combat_data * combat)
{
	int arena_num = 0;
	struct carena_data *the_arena = NULL;

	CREATE(the_arena, carena_data, 1);

	arena_num = random_number_zero_low(num_arenas - 1);
	the_arena = arena_by_num(arena_num);

	if (!the_arena) {
		if (!num_arenas) {
			return;
		}
		random_arena(ch, combat);
	}

	if (combat->arena) {
		combat->arena->used = 0;
	}

	if (the_arena) {
		combat->arena = the_arena;
		the_arena->used = 1;
	}

	return;
}


int
trans_combatants(struct combat_data *combat)
{
	// We're gonna loop through the combatants ids, convert them to players
	//  and then trans em to a random room
	int i = 0;
	struct room_data *trans_room = NULL;
	struct char_data *ch = NULL;

	for (i = 0; i < combat->num_players; i++) {

		if ((ch = get_char_in_world_by_idnum(combat->players[i].idnum))) {

			trans_room = random_arena_room(combat, ch);

			if (trans_room && ch) {
				char_from_room(ch,false);
				char_to_room(ch, trans_room,false);
				look_at_room(ch, ch->in_room, 0);
			} else {
				slog("SYSERR: Caught bad trans room or character");
				return 0;
			}
		}
	}
	return 1;
}

struct combat_data *
combat_by_vnum(int num)
{
	struct combat_data *the_combat = NULL;

	for (the_combat = battles;;) {
		if (the_combat == NULL) {
			return NULL;
		}

		if ((the_combat->vnum == num)) {
			return the_combat;
		}

		the_combat = the_combat->next;
	}

	return NULL;
}

void
say_to_combat(char *argument, CHAR * ch, combat_data * combat)
{

	int i = 0;
	struct char_data *ch2 = NULL;

	if (!argument) {
		return;
	}

	send_to_char(ch, "%sYou grunt,%s '%s'%s\r\n'", KCYN_BLD, KNRM_BLD, argument,
		KNRM);

	send_to_char(ch2, "%s%s grunts,%s '%s'%s\r\n", KCYN_BLD, GET_NAME(ch),
		KNRM_BLD, argument, KNRM);

	for (i = 0; i < combat->num_players; i++) {

		if ((ch2 = get_char_in_world_by_idnum(combat->players[i].idnum))
			&& !(ch2 == ch)) {
		}
	}
}



//Sends a message(argument) to everyone in a specified combat
void
send_to_combat(char *argument, combat_data * combat)
{
	int i = 0;
	struct char_data *ch2 = NULL;

	if (!argument) {
		return;
	}


	send_to_char(ch2, "%s%s%s\r\n", KNRM_BLD, argument, KNRM);

	for (i = 0; i < combat->num_players; i++) {

		if ((ch2 = get_char_in_world_by_idnum(combat->players[i].idnum))) {
		}
	}
}

// Removes a combat from the "battles" list
void
remove_combat(struct combat_data *combat)
{
	struct combat_data *previous_combat = NULL;
	struct combat_data *the_combat = NULL;

	for (the_combat = battles;;) {
		if (the_combat == NULL) {
			return;
		}

		if ((the_combat->vnum == combat->vnum)) {
			if (previous_combat == NULL) {
				battles = the_combat->next;
			} else {
				previous_combat->next = the_combat->next;
			}
		}

		previous_combat = the_combat;
		the_combat = the_combat->next;
	}

	return;
}

void
remove_players(combat_data * combat)
{

	int i = 0;
	CHAR *ch2 = NULL;

	for (i = 0; i < combat->num_players; i++) {

		if ((ch2 = get_char_in_world_by_idnum(combat->players[i].idnum))) {
			REMOVE_BIT(PLR2_FLAGS(ch2), PLR2_COMBAT);
			remove_player_from_list(combat, combat->players[i].idnum);
		}
	}

	// add code here to cycle through offline players

}

void
remove_player(CHAR * ch)
{
	int i = 0;
	int num;
	struct combat_data *combat = NULL;
	CHAR *ch2 = NULL;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (combat) {
		for (i = 0; i < combat->num_players; i++) {

			if ((ch2 = get_char_in_world_by_idnum(combat->players[i].idnum))) {

				if (ch2 == ch) {
					REMOVE_BIT(PLR2_FLAGS(ch2), PLR2_COMBAT);
					remove_player_from_list(combat, combat->players[i].idnum);
					combat->num_players--;
				}

			}
		}
	}
	// add code here to cycle through offline players
}

int
build_arena_list(void)
{
	char dyn_arena[100];
	char *arena_buf_top = NULL;
	char *arena_buf = NULL;
	char s[1024];
	int arenas = 0;
	int counter = 0;
	long zone = 0, rm = 0;

	carena_data *the_arena = NULL;
	dynamic_text_file *dyntext = NULL;


	strcpy(dyn_arena, "arenalist");

	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
		if (!strcasecmp(dyn_arena, dyntext->filename)) {
			break;
		}
	}

	if (!dyntext) {
		slog("Unable to read arena list file.");
		return -1;
	}

	if (!dyntext->buffer) {
		return -1;
	}

	arena_buf_top = new char[strlen(dyntext->buffer) + 1];
	arena_buf = arena_buf_top;
	strcpy(arena_buf, dyntext->buffer);
	the_arenas = NULL;
	while (*arena_buf) {
		if (!isdigit(*arena_buf)) {
			*arena_buf = ' ';
		}
		arena_buf++;
	}

	arena_buf = arena_buf_top;

	skip_spaces(&arena_buf);
	while (*arena_buf) {

		if (counter == 0) {

			//
			// First two arguments of the file are the combat starting room and holding room respectively
			// the holding room should also be the atrium for the other booty rooms
			// 

			arena_buf = one_argument(arena_buf, s);
			starting_room = atol(s);
			arena_buf = one_argument(arena_buf, s);
			holding_room = atol(s);
			counter++;
		}

		//
		// Then read in the arenas by zone number and then booty room
		//

		arena_buf = one_argument(arena_buf, s);
		zone = atol(s);
		arena_buf = one_argument(arena_buf, s);
		rm = atol(s);
		the_arena = create_arena(zone, rm);
		add_arena(the_arena);
		arenas++;
	}

	return (arenas - 1);
}

void
combat_reimburse(CHAR * ch, combat_data * combat)
{

	struct obj_data *obj = NULL;
	int returned = 0;

	if (!combat || !ch) {
		slog("Returning...shit.");
		return;
	}

	if (!COMBAT_FLAGGED(combat, COMBAT_DRAW)) {
		return;
	}

	if (!combat->booty_room) {
		slog("Error with combat booty room.");
		send_to_char(ch, "You shouldn't see this...please report.\r\n");
		return;
	}

	for (obj = combat->booty_room->contents; obj; obj = obj->next_content) {
		if (GET_IDNUM(ch) == GET_OBJ_SIGIL_IDNUM(obj)
			&& (GET_OBJ_SIGIL_LEVEL(obj) == -1)) {
			slog("#");
			GET_OBJ_SIGIL_IDNUM(obj) = 0;
			GET_OBJ_SIGIL_LEVEL(obj) = 0;
			obj_from_room(obj);
			obj_to_char(obj, ch);


		}
	}

	if (returned) {
		send_to_char(ch, "Your equipment has been returned.\r\n");
		return;
	}


}

void
add_arena(struct carena_data *arena)
{

	struct carena_data *tmp_arena = NULL;
	struct zone_data *zn = NULL;

	for (zn = zone_table; zn; zn = zn->next) {
		if (zn->number == arena->zone) {
			break;
		}
	}

	if (!zn) {
		return;
	}

	CREATE(tmp_arena, carena_data, 1);

	if (the_arenas == NULL) {
		the_arenas = arena;
		return;
	}

	if ((arena->zone == 0) || (arena->booty_room == 0)) {
		return;
	}

	for (tmp_arena = the_arenas;;) {
		if (tmp_arena == NULL) {
			return;
		}

		if (tmp_arena->next == NULL) {
			tmp_arena->next = arena;
			return;
		}

		tmp_arena = tmp_arena->next;

	}
}

void
return_sacrifice(CHAR * ch)
{
	struct combat_data *combat = NULL;
	struct obj_data *obj = NULL;
	struct obj_data *next_o = NULL;
	struct room_data *rm = NULL;
	int num = 0;
	int returned = 0;

	if (!IN_COMBAT(ch)) {
		send_to_char(ch, "You aren't even in a combat.\r\n");
		return;
	}

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);

	if (!combat->booty_room) {
		slog("Error with combat booty room.");
		send_to_char(ch, "You shouldn't see this...please report.\r\n");
		return;
	}

	rm = real_room(combat->arena->booty_room);
	if (!rm) {
		slog("Bad booty room.");
		return;
	}

	for (obj = rm->contents; obj; obj = next_o) {
		next_o = obj->next_content;
		if (GET_IDNUM(ch) == GET_OBJ_SIGIL_IDNUM(obj)
			&& (GET_OBJ_SIGIL_LEVEL(obj) == -1)) {
			slog("$");
			GET_OBJ_SIGIL_IDNUM(obj) = 0;
			GET_OBJ_SIGIL_LEVEL(obj) = 0;
			if (obj->in_room) {
				obj_from_room(obj);
			}
			obj_to_char(obj, ch);
			returned = 1;
		}
	}

	if (returned) {
		send_to_char(ch, "Your sacrifice has been returned.\r\n");
	}


}

void
return_sacrifices(struct combat_data *combat)
{
	struct char_data *ch2 = NULL;
	int i = 0;

	if (!combat) {
		return;
	}

	for (i = 0; i < combat->num_players; i++) {

		if ((ch2 = get_char_in_world_by_idnum(combat->players[i].idnum))) {

			return_sacrifice(ch2);
		}


	}
	combat->sacrificed = 0;

}



struct carena_data *
create_arena(long the_zone, long the_room)
{
	struct carena_data *the_arena = NULL;

	CREATE(the_arena, carena_data, 1);

	the_arena->zone = the_zone;
	the_arena->booty_room = the_room;
	the_arena->used = 0;

	return the_arena;
}




struct carena_data *
arena_by_num(int num)
{
	struct carena_data *the_arena = NULL;
	int i = 1;

	for (the_arena = the_arenas;;) {
		if (the_arena == NULL) {
			return NULL;
		}

		if ((i == num)) {
			return the_arena;
		}
		i++;
		the_arena = the_arena->next;
	}

	return NULL;
}


void
show_arenas(CHAR * ch)
{

	struct carena_data *the_arena = NULL;
	struct zone_data *zn = NULL;
	int i = 1;

	send_to_char(ch, "Current arenas are:\r\n");

	for (the_arena = the_arenas; the_arena; the_arena = the_arena->next) {
		if (the_arena) {
			for (zn = zone_table; zn; zn = zn->next) {
				if (zn->number == the_arena->zone) {
					send_to_char(ch, "%s%4d) %s%30s      %20s%s\r\n", KCYN, i,
						KYEL, zn->name,
						(the_arena->used ? "(unavailable)" : "(available)"),
						KNRM);
				}
			}
		}
		i++;
	}
}

void
combat_loop(CHAR * ch, CHAR * killer)
{
	struct combat_data *combat = NULL;
	int num = 0;
	int check;

	num = char_in_combat_vnum(ch);
	combat = combat_by_vnum(num);
	if (combat) {

		// take him out of the battle
		if (!killer) {
			check_battles(combat);
			if (combat->type == CTYPE_ONE_ONE) {
				SET_BIT(combat->flags, COMBAT_DRAW);
			}

			else if (combat->num_players <= 1) {
				SET_BIT(combat->flags, COMBAT_DRAW);
			}

			remove_idnum_from_combat(GET_IDNUM(ch), combat);
		}

		else {
			remove_idnum_from_combat(GET_IDNUM(ch), combat);
			sprintf(buf, "%s killed by %s in battle '%s'", GET_NAME(ch),
				GET_NAME(killer), battles->name);
			send_to_combat(buf, combat);
			check = check_battles(combat);
		}

	}

}

void
clear_booty_rooms(void)
{

	struct room_data *rm = NULL;
	struct room_data *holding = NULL;
	struct carena_data *arena = NULL;
	struct obj_data *obj = NULL;
	struct obj_data *next_o = NULL;
	int count = 0;
	int counter = 0;

	slog("Cleaning booty rooms.");

	for (arena = the_arenas; arena; arena = arena->next) {
		rm = real_room(arena->booty_room);
		holding = real_room(holding_room);

		if (rm && holding) {
			for (obj = rm->contents; obj; obj = next_o) {
				next_o = obj->next_content;
				obj_from_room(obj);
				obj_to_room(obj, holding);
				counter++;
			}
		} else {
			slog("Error with holding room.");
		}
		count++;
	}

}

// Removes a combat from the "battles" list
void
remove_player_from_list(struct combat_data *combat, long idnum)
{

	struct cplayer_data *previous_player = NULL;
	struct cplayer_data *the_player = NULL;


	for (the_player = combat->players;;) {
		if (the_player == NULL) {
			return;
		}

		if ((the_player->idnum == idnum)) {
			if (previous_player == NULL) {
				combat->players = the_player->next;
			} else {
				previous_player->next = the_player->next;
			}
		}

		previous_player = the_player;
		the_player = the_player->next;
	}

	return;
}
