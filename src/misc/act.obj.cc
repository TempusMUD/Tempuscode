/* ************************************************************************
*   File: act.obj1.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.obj.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "smokes.h"
#include "bomb.h"
#include "guns.h"
#include "fight.h"
#include "security.h"

/* extern variables */
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern int top_of_p_table;
int total_coins = 0;
int total_credits = 0;
extern struct obj_data *dam_object;

int char_hands_free(CHAR * ch);
int empty_to_obj(struct obj_data *obj, struct obj_data *container,
	struct char_data *ch);
bool junkable(struct obj_data *obj);
void gain_skill_prof(struct char_data *ch, int skillnum);
ACMD(do_stand);
ACMD(do_throw);
ACMD(do_activate);
ACMD(do_give);
ACMD(do_extinguish);
ACMD(do_say);
ACMD(do_split);

obj_data *
get_random_uncovered_implant(char_data * ch, int type = -1)
{
	int possibles = 0;
	int implant = 0;
	int pos_imp = 0;
	int i;
	obj_data *o = NULL;

	if (!ch)
		return NULL;
	for (i = 0; i < NUM_WEARS; i++) {
		if (IS_WEAR_EXTREMITY(i)) {
			if ((o = GET_IMPLANT(ch, i))) {
				if (type != -1 && !IS_OBJ_TYPE(o, type))
					continue;
				else
					possibles++;
			}
		}
	}
	if (possibles) {
		implant = number(1, possibles);
		pos_imp = 0;
		for (i = 0; pos_imp < implant && i < NUM_WEARS; i++) {
			if (IS_WEAR_EXTREMITY(i) && !GET_EQ(ch, i)) {
				if ((o = GET_IMPLANT(ch, i))) {
					if (type != -1 && !IS_OBJ_TYPE(o, type))
						continue;
					else
						pos_imp++;
				}
			}
		}
		if (pos_imp == implant) {
			o = GET_IMPLANT(ch, i - 1);
			return o;
		}
	}
	return NULL;
}

//
// returns value is same as damage()
//

int
explode_sigil(CHAR * ch, OBJ * obj)
{

	int ret = 0;
	int dam = 0;

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
		act("$p feels rather warm to the touch, and shudders violently.",
			FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	dam = number(GET_OBJ_SIGIL_LEVEL(obj), GET_OBJ_SIGIL_LEVEL(obj) << 2);
	if (mag_savingthrow(ch, GET_OBJ_SIGIL_LEVEL(obj), SAVING_SPELL))
		dam >>= 1;

	act("$p explodes when you pick it up!!", FALSE, ch, obj, 0, TO_CHAR);
	act("$p explodes when $n picks it up!!", FALSE, ch, obj, 0, TO_ROOM);

	dam_object = obj;

	ret = damage(NULL, ch, dam, TOP_SPELL_DEFINE, WEAR_HANDS);

	GET_OBJ_SIGIL_IDNUM(obj) = GET_OBJ_SIGIL_LEVEL(obj) = 0;

	dam_object = NULL;

	return SWAP_DAM_RETVAL(ret);
}

//
// return value is same as damage()
//
int
explode_all_sigils(struct char_data *ch)
{
	struct obj_data *obj, *next_obj;

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;

		if (GET_OBJ_SIGIL_IDNUM(obj)
			&& GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch)) {
			if (explode_sigil(ch, obj)) {
				return DAM_ATTACKER_KILLED;
			}
		}
	}
	return 0;
}

//
// this method removes all MONEY objects from a character's inventory
// increments his gold/credit counter, and emits messages
//

void
consolidate_char_money(struct char_data *ch)
{
	struct obj_data *obj = 0, *next_obj = 0;

	int num_gold = 0;
	int num_credits = 0;

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;

		if (IS_OBJ_TYPE(obj, ITEM_MONEY)) {

			if (GET_OBJ_VAL(obj, 1) == 1) {
				num_credits += GET_OBJ_VAL(obj, 0);
			} else {
				num_gold += GET_OBJ_VAL(obj, 0);
			}

			extract_obj(obj);
		}
	}

	if (num_gold) {
		GET_GOLD(ch) += num_gold;
		sprintf(buf, "There were %d coins.\r\n", num_gold);
		send_to_char(buf, ch);

		if (AFF_FLAGGED(ch, AFF_GROUP) && PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)) {
			sprintf(buf2, "%d", num_gold);
			do_split(ch, buf2, 0, 0);
		}
	}

	if (num_credits) {
		GET_CASH(ch) += num_credits;
		sprintf(buf, "There were %d credits.\r\n", num_credits);
		send_to_char(buf, ch);

		if (AFF_FLAGGED(ch, AFF_GROUP) && PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)) {
			sprintf(buf2, "%d credits", num_credits);
			do_split(ch, buf2, 0, 0);
		}
	}
}

//
// returns true if a quad is found and activated
//

bool
activate_char_quad(struct char_data *ch)
{

	struct obj_data *obj = 0, *next_obj = 0;

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;

		if (GET_OBJ_VNUM(obj) == QUAD_VNUM) {
			call_magic(ch, ch, NULL, SPELL_QUAD_DAMAGE, LVL_GRIMP, CAST_SPELL);
			extract_obj(obj);
			sprintf(buf, "%s got the Quad Damage at %d.", GET_NAME(ch),
				ch->in_room->number);
			slog(buf);

			struct room_data *room = 0;
			struct zone_data *zone = 0;

			//
			// notify the world of this momentous event
			//

			if (!ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
				if (ch->in_room->zone->plane > MAX_PRIME_PLANE ||
					!ZONE_FLAGGED(ch->in_room->zone, ZONE_SOUNDPROOF |
						ZONE_ISOLATED)) {
					for (zone = zone_table; zone; zone = zone->next) {
						if (zone != ch->in_room->zone &&
							zone->time_frame == ch->in_room->zone->time_frame
							&& ((zone->plane <= MAX_PRIME_PLANE
									&& ch->in_room->zone->plane <=
									MAX_PRIME_PLANE)
								|| ch->in_room->zone->plane == zone->plane)
							&& !ZONE_FLAGGED(zone,
								ZONE_ISOLATED | ZONE_SOUNDPROOF)) {
							for (room = zone->world; room; room = room->next) {
								if (room->people &&
									!ROOM_FLAGGED(room, ROOM_SOUNDPROOF))
									send_to_room
										("You hear a screaming roar in the distance.\r\n",
										room);
							}
						}
					}
				}
				for (room = ch->in_room->zone->world; room; room = room->next) {
					if (room->people && room != ch->in_room &&
						!ROOM_FLAGGED(room, ROOM_SOUNDPROOF))
						send_to_room("You hear a screaming roar nearby!\r\n",
							room);
				}
			}
			return true;
		}
	}
	return false;
}


void
perform_put(struct char_data *ch, struct obj_data *obj,
	struct obj_data *cont, int display)
{
	int capacity = 0;
	struct room_data *to_room = NULL;

	if (GET_OBJ_TYPE(cont) == ITEM_PIPE) {
		if (GET_OBJ_TYPE(obj) != ITEM_TOBACCO) {
			sprintf(buf, "SYSERR: obj %d '%s' attempted to pack.",
				GET_OBJ_VNUM(obj), obj->short_description);
			send_to_char("Sorry, there is an error here.\r\n", ch);
		} else if (CUR_DRAGS(cont) && SMOKE_TYPE(cont) != SMOKE_TYPE(obj))
			act("You need to clear out $p before packing it with $P.",
				FALSE, ch, cont, obj, TO_CHAR);
		else {
			capacity = GET_OBJ_VAL(cont, 1) - GET_OBJ_VAL(cont, 0);
			capacity = MIN(MAX_DRAGS(obj), capacity);
			if (MAX_DRAGS(obj) <= 0) {
				send_to_char("Tobacco error: please report.\r\n", ch);
				return;
			}
			if (capacity <= 0)
				act("Sorry, $p is fully packed.", FALSE, ch, cont, 0, TO_CHAR);
			else {

				GET_OBJ_VAL(cont, 0) += capacity;
				GET_OBJ_VAL(cont, 2) = GET_OBJ_VAL(obj, 0);	/* Type of tobacco */
				MAX_DRAGS(obj) -= capacity;

				act("You pack $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
				act("$n packs $p in $P.", TRUE, ch, obj, cont, TO_ROOM);

				if (MAX_DRAGS(obj) <= 0)
					extract_obj(obj);
			}
		}
	} else if (GET_OBJ_TYPE(cont) == ITEM_VEHICLE) {
		if (CAR_CLOSED(cont))
			act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
		else if ((to_room = real_room(ROOM_NUMBER(cont))) == NULL)
			act("Sorry, $p doesn't seem to have an interior right now.",
				FALSE, ch, cont, 0, TO_CHAR);
		else {
			act("You toss $P into $p.", FALSE, ch, cont, obj, TO_CHAR);
			act("$n tosses $P into $p.", FALSE, ch, cont, obj, TO_ROOM);
			obj_from_char(obj);
			obj_to_room(obj, to_room);
			act("$p is tossed into the car by $N.", FALSE, 0, obj, ch,
				TO_ROOM);
		}
	} else {

		if (cont->getContainedWeight() + obj->getWeight() > GET_OBJ_VAL(cont,
				0))
			act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
		else if (IS_OBJ_STAT(obj, ITEM_NODROP) && GET_LEVEL(ch) < LVL_TIMEGOD)
			act("$p must be cursed!  You can't seem to let go of it...", FALSE,
				ch, obj, 0, TO_CHAR);
		else {
			obj_from_char(obj);
			obj_to_obj(obj, cont);
			if (display == TRUE) {
				act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
				act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
			}
		}
	}
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char cntbuf[80];
	struct obj_data *obj, *next_obj, *cont, *save_obj = NULL;
	struct char_data *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0, counter = 0;
	bool bomb = false;

	two_arguments(argument, arg1, arg2);
	obj_dotmode = find_all_dots(arg1);
	cont_dotmode = find_all_dots(arg2);

	if (!*arg1)
		send_to_char("Put what in what?\r\n", ch);
	else if (cont_dotmode != FIND_INDIV)
		send_to_char
			("You can only put things into one container at a time.\r\n", ch);
	else if (!*arg2) {
		sprintf(buf, "What do you want to put %s in?\r\n",
			((obj_dotmode == FIND_INDIV) ? "it" : "them"));
		send_to_char(buf, ch);
	} else {
		generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP_CONT | FIND_OBJ_ROOM,
			ch, &tmp_char, &cont);
		if (!cont) {
			sprintf(buf, "You don't see %s %s here.\r\n", AN(arg2), arg2);
			send_to_char(buf, ch);
		} else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER &&
			GET_OBJ_TYPE(cont) != ITEM_VEHICLE &&
			GET_OBJ_TYPE(cont) != ITEM_PIPE)
			act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
		else if (GET_OBJ_TYPE(cont) != ITEM_PIPE &&
			IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED))
			send_to_char("You'd better open it first!\r\n", ch);
		else {
			if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
				if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
					sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg1),
						arg1);
					send_to_char(buf, ch);
				} else if (obj == cont)
					send_to_char
						("You attempt to fold it into itself, but fail.\r\n",
						ch);
				else if (GET_OBJ_TYPE(cont) == ITEM_PIPE
					&& GET_OBJ_TYPE(obj) != ITEM_TOBACCO)
					act("You can't pack $p with $P!", FALSE, ch, cont, obj,
						TO_CHAR);
				else if (IS_BOMB(obj) && obj->contains
					&& IS_FUSE(obj->contains) && FUSE_STATE(obj->contains))
					send_to_char
						("It would really be best if you didn't do that.\r\n",
						ch);
				else
					perform_put(ch, obj, cont, TRUE);
			} else {
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					if (obj != cont && INVIS_OK_OBJ(ch, obj) &&
						(obj_dotmode == FIND_ALL || isname(arg1, obj->name))) {
						if ((IS_BOMB(obj) && obj->contains
								&& IS_FUSE(obj->contains)
								&& FUSE_STATE(obj->contains))) {
							act("It would really be best if you didn't put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
							bomb = true;
							continue;
						}
						if (save_obj != NULL
							&& str_cmp(save_obj->short_description,
								obj->short_description) != 0) {
							if (counter == 1)
								sprintf(cntbuf, "You put $p in $P.");
							else
								sprintf(cntbuf, "You put $p in $P. (x%d)",
									counter);
							act(cntbuf, FALSE, ch, save_obj, cont, TO_CHAR);

							if (counter == 1)
								sprintf(cntbuf, "$n puts $p in $P.");
							else
								sprintf(cntbuf, "$n puts $p in $P. (x%d)",
									counter);
							act(cntbuf, TRUE, ch, save_obj, cont, TO_ROOM);
							counter = 0;
						}
						found = 1;
						counter++;
						save_obj = obj;
						perform_put(ch, obj, cont, FALSE);
					}
				}
				if (found == 1) {
					if (counter == 1)
						sprintf(cntbuf, "You put $p in $P.");
					else
						sprintf(cntbuf, "You put $p in $P. (x%d)", counter);
					act(cntbuf, FALSE, ch, save_obj, cont, TO_CHAR);
					if (counter == 1)
						sprintf(cntbuf, "$n puts $p in $P.");
					else
						sprintf(cntbuf, "$n puts $p in $P. (x%d)", counter);
					act(cntbuf, TRUE, ch, save_obj, cont, TO_ROOM);
				}
				if (!found && !bomb) {
					if (obj_dotmode == FIND_ALL)
						send_to_char
							("You don't seem to have anything to put in it.\r\n",
							ch);
					else {
						sprintf(buf, "You don't seem to have any %ss.\r\n",
							arg1);
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
}



bool
can_take_obj(struct char_data *ch, struct obj_data *obj, bool check_weight,
	bool print)
{

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		sprintf(buf, "$p: you can't carry that many items.");
	} else if (check_weight
		&& (IS_CARRYING_W(ch) + obj->getWeight()) > CAN_CARRY_W(ch)) {
		sprintf(buf, "$p: you can't carry that much weight.");
	} else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "$p: you can't take that!");
	} else {
		return true;
	}

	if (print)
		act(buf, FALSE, ch, obj, 0, TO_CHAR);

	return false;
}

//
// get_check_money extracts the money obj from the game and increments
// the character's cash or gold counter.
//
// returns 1 if item is extracted, 0 otherwise

int
get_check_money(struct char_data *ch, struct obj_data **obj_p, int display)
{
	struct obj_data *obj = *obj_p;

	if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) {
		obj_from_char(obj);
		if (GET_OBJ_VAL(obj, 0) > 1 && display == 1) {
			sprintf(buf, "There were %d %s.\r\n", GET_OBJ_VAL(obj, 0),
				GET_OBJ_VAL(obj, 1) ? "credits" : "coins");
			send_to_char(buf, ch);
		}
		if (!IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED)) {
			if (GET_OBJ_VAL(obj, 0) > 1000000) {
				sprintf(buf, "MONEY: %s obtained %d %s at %d.",
					GET_NAME(ch), GET_OBJ_VAL(obj, 0),
					GET_OBJ_VAL(obj, 1) ? "credits" : "coins",
					ch->in_room->number);
				slog(buf);
			}
			if (GET_OBJ_VAL(obj, 1)) {
				GET_CASH(ch) += GET_OBJ_VAL(obj, 0);
				total_credits += GET_OBJ_VAL(obj, 0);
			} else {
				GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
				total_coins += GET_OBJ_VAL(obj, 0);
			}
		}
		extract_obj(obj);

		obj_p = 0;

		if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(ch) < LVL_GOD) {
			GET_GOLD(ch) = 0;
			GET_BANK_GOLD(ch) = 0;
			GET_CASH(ch) = 0;
			GET_ECONET(ch) = 0;
		}
		return 1;
	}
	return 0;
}

// 
// returns true if object is gotten from container
//

bool
perform_get_from_container(struct char_data * ch,
	struct obj_data * obj,
	struct obj_data * cont, bool already_has, bool display, int counter)
{

	bool retval = false;

	if (!can_take_obj(ch, obj, already_has, true)) {
		if (counter > 1) {
			display = true;
			--counter;
		} else {
			return false;
		}
	}

	else {
		obj_from_obj(obj);
		obj_to_char(obj, ch);
		retval = true;

		//
		// log corpse looting
		//

		if (GET_OBJ_VAL(cont, 3) && CORPSE_IDNUM(cont) > 0 &&
			CORPSE_IDNUM(cont) <= top_of_p_table &&
			CORPSE_IDNUM(cont) != GET_IDNUM(ch)) {
			sprintf(buf, "%s looted %s from %s.", GET_NAME(ch),
				obj->short_description, cont->short_description);
			mudlog(buf, CMP, LVL_DEMI, TRUE);
		}
	}

	if (display == true && counter > 0) {
		if (counter == 1) {
			strcpy(buf, "You get $p from $P.");
			strcpy(buf2, "$n gets $p from $P.");
		} else {
			sprintf(buf, "You get $p from $P. (x%d)", counter);
			sprintf(buf2, "$n gets $p from $P. (x%d)", counter);
		}
		act(buf, FALSE, ch, obj, cont, TO_CHAR);
		act(buf2, TRUE, ch, obj, cont, TO_ROOM);
	}

	return retval;
}

//
// return value same as damage()
//

int
get_from_container(struct char_data *ch, struct obj_data *cont, char *arg)
{

	struct obj_data *obj, *next_obj;

	int dotmode;
	int retval = 0;

	bool quad_found = false;
	bool sigil_found = false;
	bool money_found = false;

	char *match_name = 0;		// the char* keyword which matches on .<item>

	bool check_weight = false;

	if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED) && !GET_OBJ_VAL(cont, 3) &&
		GET_LEVEL(ch) < LVL_CREATOR) {
		act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
		return 0;
	}

	dotmode = find_all_dots(arg);

	if (cont->in_room)
		check_weight = true;

	if (dotmode == FIND_INDIV) {

		if (!(obj = get_obj_in_list_all(ch, arg, cont->contains))) {
			sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg),
				arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
			return 0;
		}
		if (IS_IMPLANT(obj) && IS_CORPSE(cont) &&
			!CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
			if (GET_LEVEL(ch) < LVL_GOD) {
				sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg),
					arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
				return 0;
			} else {
				SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
			}
		}

		if (!perform_get_from_container(ch, obj, cont, check_weight, true, 1))
			return 0;

		if (IS_OBJ_TYPE(obj, ITEM_MONEY))
			money_found = true;
		else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
			quad_found = true;
		else if (GET_OBJ_SIGIL_IDNUM(obj))
			sigil_found = true;
	}

	//
	// 'get all' or 'get all.item'
	//

	else {

		if (dotmode == FIND_ALLDOT) {
			match_name = arg;
		}

		bool found = false;
		bool display = false;
		int counter = 1;

		for (obj = cont->contains; obj; obj = next_obj) {
			next_obj = obj->next_content;

			if (!CAN_SEE_OBJ(ch, obj)) {
				continue;
			}
			// Gods can get all.corpse and get implants.
			// Players cannot.
			if (IS_IMPLANT(obj) && IS_CORPSE(cont)
				&& !CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
				if (GET_LEVEL(ch) < LVL_GOD) {
					continue;
				} else {
					SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
				}
			}
			//
			// match_name is set if this is a find alldot get
			//

			if (match_name && !isname(match_name, obj->name))
				continue;

			if (!next_obj ||
				next_obj->short_description != obj->short_description ||
				!CAN_SEE_OBJ(ch, next_obj)
				|| !can_take_obj(ch, next_obj, check_weight, false)) {
				display = true;
			}

			if (!perform_get_from_container(ch, obj, cont, check_weight,
					display, counter)) {
				counter = 1;
			}

			else {
				found = true;

				if (IS_OBJ_TYPE(obj, ITEM_MONEY))
					money_found = true;
				else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
					quad_found = true;
				else if (GET_OBJ_SIGIL_IDNUM(obj)
					&& GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
					sigil_found = true;

				if (!display) {
					++counter;
				} else {
					counter = 1;
				}
			}

			display = false;
		}

		if (!found) {

			if (match_name) {
				sprintf(buf,
					"You didn't find anything in $P to take that looks like a '%s'",
					match_name);
				act(buf, FALSE, ch, 0, cont, TO_CHAR);

			} else {
				sprintf(buf, "You didn't find anything in $P.");
				act(buf, FALSE, ch, 0, cont, TO_CHAR);
			}
			return 0;
		}
	}

	if (money_found)
		consolidate_char_money(ch);

	if (quad_found)
		activate_char_quad(ch);

	if (sigil_found) {
		retval = explode_all_sigils(ch);
	}

	return retval;
}

//
// perform_get_from_room cannot extract the obj
//
// returns true on successful get
//

bool
perform_get_from_room(struct char_data * ch,
	struct obj_data * obj, bool display, int counter)
{

	bool retval = false;

	if (!can_take_obj(ch, obj, true, true)) {
		if (counter > 1) {
			display = true;
			--counter;
		} else {
			return false;
		}
	}

	else {
		obj_from_room(obj);
		obj_to_char(obj, ch);
		retval = true;
	}

	if (display == true && counter > 0) {
		if (counter == 1) {
			strcpy(buf, "You get $p.");
			strcpy(buf2, "$n gets $p.");
		} else {
			sprintf(buf, "You get $p. (x%d)", counter);
			sprintf(buf2, "$n gets $p. (x%d)", counter);
		}
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act(buf2, TRUE, ch, obj, 0, TO_ROOM);

	}

	return retval;
}

//
// return value is same as damage()
//

int
get_from_room(struct char_data *ch, char *arg)
{

	struct obj_data *obj, *next_obj;

	int dotmode;
	int retval = 0;

	bool quad_found = false;
	bool sigil_found = false;
	bool money_found = false;

	char *match_name = 0;		// the char* keyword which matches

	dotmode = find_all_dots(arg);

	//
	// 'get <item>'
	//

	if (dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			sprintf(buf, "You don't see %s %s here.\r\n", AN(arg), arg);
			send_to_char(buf, ch);
			return 0;
		}


		if (!perform_get_from_room(ch, obj, true, 1))
			return 0;

		if (IS_OBJ_TYPE(obj, ITEM_MONEY))
			money_found = true;
		else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
			quad_found = true;
		else if (GET_OBJ_SIGIL_IDNUM(obj)
			&& GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
			sigil_found = true;

	}
	//
	// 'get all' or 'get all.item'
	//

	else {

		if (dotmode == FIND_ALLDOT) {
			match_name = arg;
		}

		bool found = false;
		bool display = false;
		int counter = 1;

		for (obj = ch->in_room->contents; obj; obj = next_obj) {
			next_obj = obj->next_content;

			if (!CAN_SEE_OBJ(ch, obj)) {
				continue;
			}
			//
			// match_name is set if this is a find alldot get
			//

			if (match_name && !isname(match_name, obj->name))
				continue;

			if (!next_obj ||
				next_obj->short_description != obj->short_description ||
				!CAN_SEE_OBJ(ch, next_obj)
				|| !can_take_obj(ch, next_obj, true, false)) {
				display = true;
			}

			if (!perform_get_from_room(ch, obj, display, counter)) {
				counter = 1;
			} else {
				found = true;

				if (IS_OBJ_TYPE(obj, ITEM_MONEY))
					money_found = true;
				else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
					quad_found = true;
				else if (GET_OBJ_SIGIL_IDNUM(obj)
					&& GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
					sigil_found = true;

				if (!display) {
					++counter;
				} else {
					counter = 1;
				}
			}

			display = false;
		}

		if (!found) {

			if (match_name) {
				sprintf(buf,
					"You didn't find anything to take that looks like a '%s'.\r\n",
					match_name);
				send_to_char(buf, ch);
			} else {
				send_to_char("You didn't find anything to take.\r\n", ch);
			}
			return 0;
		}
	}

	if (money_found)
		consolidate_char_money(ch);

	if (quad_found)
		activate_char_quad(ch);

	if (sigil_found) {
		retval = explode_all_sigils(ch);
	}

	return retval;
}



ACCMD(do_get)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	int cont_dotmode;
	struct obj_data *cont;
	struct char_data *tmp_char;

	ACMD_set_return_flags(0);

	total_coins = total_credits = 0;
	two_arguments(argument, arg1, arg2);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		send_to_char("Your arms are already full!\r\n", ch);
		return;
	}

	if (!*arg1) {
		send_to_char("Get what?\r\n", ch);
		return;
	}

	if (!*arg2) {

		int retval = get_from_room(ch, arg1);

		if (retval) {
			ACMD_set_return_flags(retval);
		}

		return;
	}

	cont_dotmode = find_all_dots(arg2);


	if (cont_dotmode == FIND_INDIV) {

		generic_find(arg2,
			FIND_OBJ_INV | FIND_OBJ_EQUIP_CONT | FIND_OBJ_ROOM,
			ch, &tmp_char, &cont);

		if (!cont) {
			sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
			send_to_char(buf, ch);
			return;
		}

		if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER) {
			act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
			return;
		}

		int retval = get_from_container(ch, cont, arg1);

		if (retval) {
			ACMD_set_return_flags(retval);
		}

		return;
	}

	char *match_container_name = 0;

	// cont_dotmode != FIND_INDIV 

	if (cont_dotmode == FIND_ALLDOT) {
		if (!*arg2) {
			send_to_char("Get from all of what?\r\n", ch);
			return;
		}
		match_container_name = arg2;
	}

	bool container_found = false;
	char container_args[MAX_INPUT_LENGTH];


	//
	// scan inventory for matching containers
	//

	for (cont = ch->carrying; cont; cont = cont->next_content) {

		if (CAN_SEE_OBJ(ch, cont) &&
			(!match_container_name
				|| isname(match_container_name, cont->name))) {

			if (!IS_OBJ_TYPE(cont, ITEM_CONTAINER)) {
				act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
				continue;
			}

			container_found = true;

			strcpy(container_args, arg1);
			int retval = get_from_container(ch, cont, container_args);

			if (retval) {
				ACMD_set_return_flags(retval);
				return;
			}
		}
	}

	//
	// scan room for matching containers
	//

	for (cont = ch->in_room->contents; cont; cont = cont->next_content) {

		if (CAN_SEE_OBJ(ch, cont) &&
			(!match_container_name
				|| isname(match_container_name, cont->name))) {

			if (!IS_OBJ_TYPE(cont, ITEM_CONTAINER)) {
				act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
				continue;
			}

			container_found = true;

			strcpy(container_args, arg1);
			int retval = get_from_container(ch, cont, container_args);

			if (retval) {
				ACMD_set_return_flags(retval);
				return;
			}
		}
	}

	if (!container_found) {
		if (match_container_name) {
			sprintf(buf, "You can't seem to find any %ss here.\r\n", arg2);
			send_to_char(buf, ch);
		} else {
			send_to_char("You can't seem to find any containers.\r\n", ch);
		}
	}
}


void
perform_drop_gold(struct char_data *ch, int amount,
	byte mode, struct room_data *RDR)
{
	struct obj_data *obj;

	if (amount <= 0)
		send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
	else if (GET_GOLD(ch) < amount)
		send_to_char("You don't have that many coins!\r\n", ch);
	else {
		if (mode != SCMD_JUNK) {
			WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
			obj = create_money(amount, 0);
			if (mode == SCMD_DONATE) {
				send_to_char
					("You throw some gold into the air where it disappears in a puff of smoke!\r\n",
					ch);
				act("$n throws some gold into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
				obj_to_room(obj, RDR);
				act("$p suddenly appears in a puff of orange smoke!", 0, 0,
					obj, 0, TO_ROOM);
			} else {
				send_to_char("You drop some gold.\r\n", ch);
				act("$n drops $p.", FALSE, ch, obj, 0, TO_ROOM);
				obj_to_room(obj, ch->in_room);
			}
		} else {
			sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
				money_desc(amount, 0));
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			send_to_char
				("You drop some gold which disappears in a puff of smoke!\r\n",
				ch);
		}
		GET_GOLD(ch) -= amount;
		if (mode != SCMD_DONATE && GET_LEVEL(ch) >= LVL_AMBASSADOR &&
			GET_LEVEL(ch) < LVL_GOD) {
			sprintf(buf, "MONEY: %s has dropped %d coins at %d.", GET_NAME(ch),
				amount, ch->in_room->number);
			slog(buf);
		}
	}
}
void
perform_drop_credits(struct char_data *ch, int amount,
	byte mode, struct room_data *RDR)
{
	struct obj_data *obj;

	if (amount <= 0)
		send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
	else if (GET_CASH(ch) < amount)
		send_to_char("You don't have that much cash!\r\n", ch);
	else {
		if (mode != SCMD_JUNK) {
			WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
			obj = create_money(amount, 1);
			if (mode == SCMD_DONATE) {
				send_to_char
					("You throw some cash into the air where it disappears in a puff of smoke!\r\n",
					ch);
				act("$n throws some cash into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
				obj_to_room(obj, RDR);
				act("$p suddenly appears in a puff of orange smoke!", 0, 0,
					obj, 0, TO_ROOM);
			} else {
				send_to_char("You drop some cash.\r\n", ch);
				act("$n drops $p.", FALSE, ch, obj, 0, TO_ROOM);
				obj_to_room(obj, ch->in_room);
			}
		} else {
			sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
				money_desc(amount, 1));
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			send_to_char
				("You drop some cash which disappears in a puff of smoke!\r\n",
				ch);
		}
		GET_CASH(ch) -= amount;
		if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(ch) < LVL_GOD) {
			sprintf(buf, "MONEY: %s has dropped %d credits at %d.",
				GET_NAME(ch), amount, ch->in_room->number);
			slog(buf);
		}
	}
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")
#define PROTO_SDESC(vnum) (real_object_proto(vnum)->short_description)
#define IS_CONTAINER(obj) (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)

bool
junkable(struct obj_data *obj)
{
	if (obj->shared && obj->shared->vnum >= 0 &&
		strcmp(obj->short_description, PROTO_SDESC(obj->shared->vnum)))
		return false;

	if (IS_CONTAINER(obj)) {
		for (obj = obj->contains; obj; obj = obj->next_content) {
			if (!junkable(obj))
				return false;
		}
	}
	return true;
}

int
perform_drop(struct char_data *ch, struct obj_data *obj,
	byte mode, char *sname, struct room_data *RDR, int display)
{
	int value;
	string sbuf;

	if (!obj)
		return 0;

	if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
		if (GET_LEVEL(ch) < LVL_TIMEGOD) {
			sprintf(buf, "You can't %s $p, it must be CURSED!", sname);
			act(buf, FALSE, ch, obj, 0, TO_CHAR);
			return 0;
		} else
			act("You peel $p off your hand...", FALSE, ch, obj, 0, TO_CHAR);
	}

	if ((mode == SCMD_DONATE || mode == SCMD_JUNK) &&
		GET_LEVEL(ch) < LVL_SPIRIT) {
		if (!junkable(obj)) {
			if (IS_CONTAINER(obj)) {
				string containerName(obj->short_description);
				sbuf = AN(obj->name) + string(" ") + containerName +
					" contains, or is, a renamed object.\r\n";
			} else
				sbuf = "You can't " + (mode == SCMD_JUNK ? string("junk") :
					string("donate")) + " a renamed object.\r\n";

			if (display == TRUE)
				send_to_char(sbuf.c_str(), ch);
			return 0;
		}
	}

	if (display == TRUE) {
		sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
		act(buf, TRUE, ch, obj, 0, TO_ROOM);
	}

	obj_from_char(obj);

	if ((mode == SCMD_DONATE) && (IS_OBJ_STAT(obj, ITEM_NODONATE) ||
			!OBJ_APPROVED(obj)))
		mode = SCMD_JUNK;

	switch (mode) {
	case SCMD_DROP:
		obj_to_room(obj, ch->in_room);
		if (ch->in_room->isOpenAir() &&
			EXIT(ch, DOWN) &&
			EXIT(ch, DOWN)->to_room &&
			!IS_SET(EXIT(ch, DOWN)->exit_info, EX_CLOSED)) {
			act("$p falls downward through the air, out of sight!",
				FALSE, ch, obj, 0, TO_ROOM);
			act("$p falls downward through the air, out of sight!",
				FALSE, ch, obj, 0, TO_CHAR);
			obj_from_room(obj);
			obj_to_room(obj, EXIT(ch, DOWN)->to_room);
			act("$p falls from the sky and lands by your feet.",
				FALSE, 0, obj, 0, TO_ROOM);
		}
		return 1;
		break;
	case SCMD_DONATE:
		obj_to_room(obj, RDR);
		act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0,
			TO_ROOM);
		if (!IS_OBJ_TYPE(obj, ITEM_OTHER) && !IS_OBJ_TYPE(obj, ITEM_TREASURE)
			&& !IS_OBJ_TYPE(obj, ITEM_METAL))
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
		return 1;
		break;
	case SCMD_JUNK:
		value = MAX(1, MIN(200, GET_OBJ_COST(obj) >> 4));
		extract_obj(obj);
		return value;
		break;
	default:
		slog("SYSERR: Incorrect argument passed to perform_drop");
		break;
	}

	return 0;
}



ACCMD(do_drop)
{
	extern room_num donation_room_1;	/* Modrian */
	extern room_num donation_room_2;	/* EC */
	extern room_num donation_room_3;	/* New Thalos */
	extern room_num donation_room_istan;	/* Istan */
	extern room_num donation_room_solace;
	extern room_num donation_room_skullport_common;
	extern room_num donation_room_skullport_dwarven;

	extern int guild_donation_info[][4];
	int i;
	struct obj_data *obj, *next_obj;
	struct room_data *RDR = NULL;
	byte oldmode;
	byte mode = SCMD_DROP;
	int dotmode, amount = 0, counter = 0;
	char *sname = NULL;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

	if (subcmd == SCMD_GUILD_DONATE) {
		for (i = 0; guild_donation_info[i][0] != -1; i++) {
			if (guild_donation_info[i][0] == GET_CLASS(ch) &&
				!((guild_donation_info[i][1] == EVIL && IS_GOOD(ch)) ||
					(guild_donation_info[i][1] == GOOD && IS_EVIL(ch))) &&
				(guild_donation_info[i][2] == GET_HOME(ch))) {
				RDR = real_room(guild_donation_info[i][3]);
				break;
			}
		}
		subcmd = SCMD_DONATE;
		mode = subcmd;
		sname = "donate";
	}

	if (RDR == NULL) {
		switch (subcmd) {
		case SCMD_JUNK:
			sname = "junk";
			mode = SCMD_JUNK;
			break;
		case SCMD_DONATE:
			sname = "donate";
			mode = SCMD_DONATE;
			switch (number(0, 18)) {
			case 0:
			case 1:
				mode = SCMD_JUNK;
				RDR = NULL;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				RDR = real_room(donation_room_1);
				break;			/* Modrian */
			case 6:
			case 7:
				RDR = real_room(donation_room_3);
				break;			/* New Thalos */
			case 8:
			case 9:
				RDR = real_room(donation_room_istan);
				break;			/* Istan */
			case 10:
			case 11:
				RDR = real_room(donation_room_solace);
				break;			/* Solace Cove */
			case 12:
			case 13:
			case 14:
				RDR = real_room(donation_room_skullport_common);
				break;			/* sk */
			case 15:
				RDR = real_room(donation_room_skullport_dwarven);
				break;
			case 16:
			case 17:
			case 18:
				RDR = real_room(donation_room_2);
				break;
			}
			if (RDR == NULL && mode != SCMD_JUNK) {
				send_to_char("Sorry, you can't donate anything right now.\r\n",
					ch);
				return;
			}
			break;
		default:
			sname = "drop";
			break;
		}
	}

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		sprintf(buf, "What do you want to %s?\r\n", sname);
		send_to_char(buf, ch);
		return;
	} else if (is_number(arg1)) {
		amount = atoi(arg1);

		if (!str_cmp("coins", arg2) || !str_cmp("coin", arg2))
			perform_drop_gold(ch, amount, mode, RDR);
		else if (!str_cmp("credits", arg2) || !str_cmp("credit", arg2))
			perform_drop_credits(ch, amount, mode, RDR);
		else {
			/* code to drop multiple items.  anyone want to write it? -je */
			send_to_char
				("Sorry, you can't do that to more than one item at a time.\r\n",
				ch);
		}
		return;
	} else {
		dotmode = find_all_dots(arg1);

		/* Can't junk or donate all */
		if ((dotmode == FIND_ALL) &&
			(subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
			if (subcmd == SCMD_JUNK)
				send_to_char
					("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
			else
				send_to_char
					("Go do the donation room if you want to donate EVERYTHING!\r\n",
					ch);
			return;
		}
		if (dotmode == FIND_ALL) {
			if (!ch->carrying) {
				send_to_char("You don't seem to be carrying anything.\r\n",
					ch);
				return;
			}

			for (obj = ch->carrying, counter = 1; obj; obj = next_obj) {
				next_obj = obj->next_content;
				oldmode = mode;
				if (GET_OBJ_TYPE(obj) == ITEM_KEY && !GET_OBJ_VAL(obj, 1) &&
					mode != SCMD_DROP)
					mode = SCMD_JUNK;
				if (IS_BOMB(obj) && obj->contains && IS_FUSE(obj->contains))
					mode = SCMD_DROP;

				int found = 0;
				found = perform_drop(ch, obj, mode, sname, RDR, FALSE);
				mode = oldmode;
				if (next_obj
					&& (next_obj->short_description == obj->short_description)
					&& found) {
					counter++;
					found = 0;
				} else {
					if (counter == 1) {
						sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
						sprintf(buf2, "$n %ss $p.%s", sname, VANISH(mode));
					} else {
						sprintf(buf, "You %s $p.%s (x%d)", sname, VANISH(mode),
							counter);
						sprintf(buf2, "$n %ss $p.%s (x%d)", sname,
							VANISH(mode), counter);
					}
					act(buf, FALSE, ch, obj, 0, TO_CHAR);
					act(buf2, TRUE, ch, obj, 0, TO_ROOM);
					counter = 1;
				}
			}
		}

		else if (dotmode == FIND_ALLDOT) {
			if (!*arg1) {
				sprintf(buf, "What do you want to %s all of?\r\n", sname);
				send_to_char(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
				sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
				send_to_char(buf, ch);
				return;
			}

			counter = 1;
			oldmode = mode;

			while (obj) {
				next_obj = get_obj_in_list_all(ch, arg1, obj->next_content);
				if (GET_OBJ_TYPE(obj) == ITEM_KEY && !GET_OBJ_VAL(obj, 1) &&
					mode != SCMD_DROP)
					mode = SCMD_JUNK;

				// cannot junk a lit bomb!
				if (IS_BOMB(obj) && obj->contains && IS_FUSE(obj->contains)
					&& FUSE_STATE(obj->contains))
					mode = SCMD_DROP;

				if (next_obj
					&& next_obj->short_description == obj->short_description) {
					counter++;
					amount += perform_drop(ch, obj, mode, sname, RDR, FALSE);
				} else {
					if ((mode == SCMD_DONATE || mode == SCMD_JUNK)
						&& !junkable(obj)) {
						sprintf(buf, "$p is or contains a renamed object.");
						counter--;
						send_to_char(buf, ch);
						if (counter > 1) {
							sprintf(buf, "You %s $p.%s (x%d)", sname,
								VANISH(mode), counter);
							sprintf(buf2, "$n %ss $p.%s (x%d)", sname,
								VANISH(mode), counter);
						}
					} else if (counter == 1) {
						sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
						sprintf(buf2, "$n %ss $p.%s", sname, VANISH(mode));
					} else {
						sprintf(buf, "You %s $p.%s (x%d)", sname, VANISH(mode),
							counter);
						sprintf(buf2, "$n %ss $p.%s (x%d)", sname,
							VANISH(mode), counter);
					}
					act(buf, FALSE, ch, obj, 0, TO_CHAR);
					act(buf2, TRUE, ch, obj, 0, TO_ROOM);
					amount += perform_drop(ch, obj, mode, sname, RDR, FALSE);
					counter = 1;
				}
				mode = oldmode;
				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1),
					arg1);
				send_to_char(buf, ch);
			} else {
				oldmode = mode;
				if (GET_OBJ_TYPE(obj) == ITEM_KEY && !GET_OBJ_VAL(obj, 1) &&
					mode != SCMD_DROP)
					mode = SCMD_JUNK;
				amount += perform_drop(ch, obj, mode, sname, RDR, TRUE);
				mode = oldmode;
			}
		}
	}

	if (amount && (subcmd == SCMD_JUNK)) {
		send_to_char("You have been rewarded by the gods!\r\n", ch);
		act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
		GET_GOLD(ch) += amount;
	}
}


void
perform_give(struct char_data *ch, struct char_data *vict,
	struct obj_data *obj, int display)
{
	int i;

	if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
		if (GET_LEVEL(ch) < LVL_TIMEGOD) {
			act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0,
				TO_CHAR);
			return;
		} else
			act("You peel $p off your hand...", FALSE, ch, obj, 0, TO_CHAR);
	}
	if (GET_LEVEL(ch) < LVL_IMMORT && vict->getPosition() <= POS_SLEEPING) {
		act("$E is currently unconscious.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (obj->getWeight() + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	obj_from_char(obj);
	obj_to_char(obj, vict);
	if (display == TRUE) {
		act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
		act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
		act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
	}

	if (!IS_NPC(ch) && IS_NPC(vict) && MOB2_FLAGGED(vict, MOB2_MUGGER) &&
		vict->mob_specials.mug &&
		GET_OBJ_VNUM(obj) == vict->mob_specials.mug->vnum) {

		if (GET_IDNUM(ch) == vict->mob_specials.mug->idnum) {
			if (FIGHTING(vict) && !IS_NPC(FIGHTING(vict)) &&
				GET_IDNUM(FIGHTING(vict)) == vict->mob_specials.mug->idnum) {
				sprintf(buf, "Ha!  Let this be a lesson to you, %s!",
					GET_NAME(FIGHTING(vict)));
				do_say(vict, buf, 0, 0);
				if (FIGHTING(FIGHTING(vict)))
					stop_fighting(FIGHTING(vict));
				stop_fighting(vict);
			} else {
				sprintf(buf, "Good move, %s!", GET_DISGUISED_NAME(vict, ch));
				do_say(vict, buf, 0, 0);
			}
			free(vict->mob_specials.mug);
			vict->mob_specials.mug = NULL;
		} else
			do_say(ch, "Ok, that will work.", 0, 0);

		free(vict->mob_specials.mug);
		vict->mob_specials.mug = NULL;
		return;
	}
	if (IS_OBJ_TYPE(obj, ITEM_MONEY) &&
		GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) < LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "MONEY: %s has given %s (%d) to %s.", GET_NAME(ch),
			obj->short_description, GET_OBJ_VAL(obj, 0), GET_NAME(vict));
		slog(buf);
	}

	if (IS_NPC(vict) && AWAKE(vict) && !AFF_FLAGGED(vict, AFF_CHARM) &&
		CAN_SEE_OBJ(vict, obj)) {
		if (IS_BOMB(obj)) {
			if ((isname("bomb", obj->name) || isname("grenade", obj->name) ||
					isname("explosives", obj->name)
					|| isname("explosive", obj->name))
				&& (CHECK_SKILL(vict, SKILL_DEMOLITIONS) < number(10, 50)
					|| (obj->contains && IS_FUSE(obj->contains)
						&& FUSE_STATE(obj->contains)))) {
				if (!FIGHTING(vict)
					&& CHECK_SKILL(vict, SKILL_DEMOLITIONS) > number(40, 60))
					if (FUSE_IS_BURN(obj->contains))
						do_extinguish(vict, fname(obj->name), 0, 0);
					else
						do_activate(vict, fname(obj->name), 0, 1);
				else {
					if (vict->getPosition() < POS_FIGHTING)
						do_stand(vict, 0, 0, 0);
					for (i = 0; i < NUM_DIRS; i++) {
						if (ch->in_room->dir_option[i] &&
							ch->in_room->dir_option[i]->to_room && i != UP &&
							!IS_SET(ch->in_room->dir_option[i]->exit_info,
								EX_CLOSED)) {
							sprintf(buf, "%s %s", fname(obj->name), dirs[i]);
							do_throw(vict, buf, 0, 0);
							return;
						}
					}
					if (CAN_SEE(vict, ch)) {
						sprintf(buf, "%s %s", fname(obj->name),
							fname(ch->player.name));
						do_give(vict, buf, 0, 0);
					} else
						do_throw(vict, fname(obj->name), 0, 0);
				}
			}
			return;
		}
		if ((IS_CARRYING_W(vict) + IS_WEARING_W(vict)) > (CAN_CARRY_W(vict) >> 1))	// i don't want that heavy shit
			do_drop(vict, fname(obj->name), 0, 0);
	}
}
void
perform_plant(struct char_data *ch, struct char_data *vict,
	struct obj_data *obj)
{
	if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
		act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (obj->getWeight() + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	obj_from_char(obj);
	obj_to_char(obj, vict);
	act("You plant $p on $N.", FALSE, ch, obj, vict, TO_CHAR);
	if ((CHECK_SKILL(ch, SKILL_PLANT) + GET_DEX(ch)) <
		(number(0, 83) + GET_WIS(vict)) ||
		IS_AFFECTED_2(vict, AFF2_TRUE_SEEING))
		act("$n puts $p in your pocket.", FALSE, ch, obj, vict, TO_VICT);
	else
		gain_skill_prof(ch, SKILL_PLANT);
}

/* utility function for give */
struct char_data *
give_find_vict(struct char_data *ch, char *arg)
{
	struct char_data *vict;

	if (!*arg) {
		send_to_char("To who?\r\n", ch);
		return NULL;
	} else if (!(vict = get_char_room_vis(ch, arg))) {
		send_to_char(NOPERSON, ch);
		return NULL;
	} else if (vict == ch) {
		send_to_char("What's the point of that?\r\n", ch);
		return NULL;
	} else
		return vict;
}

void
perform_give_gold(struct char_data *ch, struct char_data *vict, int amount)
{
	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
		send_to_char("You don't have that many coins!\r\n", ch);
		return;
	}
	if (GET_LEVEL(ch) < LVL_IMMORT && vict->getPosition() <= POS_SLEEPING) {
		act("$E is currently unconscious.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "You are given %d gold coins by $n.", amount);
	act(buf, FALSE, ch, 0, vict, TO_VICT);
	sprintf(buf, "$n gives %s to $N.", money_desc(amount, 0));
	act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
	if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
		GET_GOLD(ch) -= amount;
	GET_GOLD(vict) += amount;

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) < LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "MONEY: %s has given %d coins to %s.", GET_NAME(ch),
			amount, GET_NAME(vict));
		slog(buf);
	}
}

void
perform_plant_gold(struct char_data *ch, struct char_data *vict, int amount)
{
	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
		send_to_char("You don't have that many coins!\r\n", ch);
		return;
	}
	send_to_char(OK, ch);
	if ((GET_SKILL(ch, SKILL_PLANT) + GET_DEX(ch)) < (number(0,
				83) + GET_WIS(vict))
		|| IS_AFFECTED_2(vict, AFF2_TRUE_SEEING)) {
		sprintf(buf, "%d gold coin%s planted in your pocket by $n.", amount,
			amount == 1 ? " is" : "s are");
		act(buf, FALSE, ch, 0, vict, TO_VICT);
	}
	if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
		GET_GOLD(ch) -= amount;
	GET_GOLD(vict) += amount;
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) < LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "MONEY: %s has planted %d coins on %s.", GET_NAME(ch),
			amount, GET_NAME(vict));
		slog(buf);
	}

}

void
perform_give_credits(struct char_data *ch, struct char_data *vict, int amount)
{
	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_CASH(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
		send_to_char("You don't have that many credits!\r\n", ch);
		return;
	}
	if (GET_LEVEL(ch) < LVL_IMMORT && vict->getPosition() <= POS_SLEEPING) {
		act("$E is currently unconscious.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "You are given %d credits by $n.", amount);
	act(buf, FALSE, ch, 0, vict, TO_VICT);
	sprintf(buf, "$n gives %s to $N.", money_desc(amount, 1));
	act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
	if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
		GET_CASH(ch) -= amount;
	GET_CASH(vict) += amount;

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) < LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "MONEY: %s has given %d credits to %s.", GET_NAME(ch),
			amount, GET_NAME(vict));
		slog(buf);
	}
}

void
perform_plant_credits(struct char_data *ch, struct char_data *vict, int amount)
{
	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_CASH(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
		send_to_char("You don't have that many credits!\r\n", ch);
		return;
	}
	send_to_char(OK, ch);
	if ((GET_SKILL(ch, SKILL_PLANT) + GET_DEX(ch)) <
		(number(0, 83) + GET_WIS(vict)) ||
		IS_AFFECTED_2(vict, AFF2_TRUE_SEEING)) {
		sprintf(buf, "%d credit%s planted in your pocket by $n.",
			amount, amount == 1 ? " is" : "s are");
		act(buf, FALSE, ch, 0, vict, TO_VICT);
	}
	if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
		GET_CASH(ch) -= amount;
	GET_CASH(vict) += amount;
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) < LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD) {
		sprintf(buf, "MONEY: %s has planted %d credits on %s.", GET_NAME(ch),
			amount, GET_NAME(vict));
		slog(buf);
	}

}

ACMD(do_give)
{
	int amount, dotmode, found, counter = 0;
	struct char_data *vict;
	struct obj_data *obj, *next_obj, *save_obj = NULL;
	char cntbuf[80];

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Give what to who?\r\n", ch);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
			argument = one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)))
				perform_give_gold(ch, vict, amount);
			return;
		} else if (!str_cmp("credits", arg) || !str_cmp("cash", arg) ||
			!str_cmp("credit", arg)) {
			argument = one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)))
				perform_give_credits(ch, vict, amount);
			return;
		} else {
			/* code to give multiple items.  anyone want to write it? -je */
			send_to_char("You can't give more than one item at a time.\r\n",
				ch);
			return;
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV) {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg),
					arg);
				send_to_char(buf, ch);
			} else
				perform_give(ch, vict, obj, TRUE);
		} else {
			if (dotmode == FIND_ALLDOT && !*arg) {
				send_to_char("All of what?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				send_to_char("You don't seem to be holding anything.\r\n", ch);
			else {
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					if (INVIS_OK_OBJ(ch, obj) &&
						((dotmode == FIND_ALL || isname(arg, obj->name)))) {
						perform_give(ch, vict, obj, FALSE);
						if (save_obj != NULL &&
							str_cmp(save_obj->short_description,
								obj->short_description) != 0) {
							if (counter == 1)
								sprintf(cntbuf, "You give $p to $N.");
							else
								sprintf(cntbuf, "You give $p to $N. (x%d)",
									counter);
							act(cntbuf, FALSE, ch, save_obj, vict, TO_CHAR);
							if (counter == 1)
								sprintf(cntbuf, "$n gives you $p.");
							else
								sprintf(cntbuf, "$n gives you $p. (x%d)",
									counter);
							act(cntbuf, FALSE, ch, save_obj, vict, TO_VICT);
							if (counter == 1)
								sprintf(cntbuf, "$n gives $p to $N.");
							else
								sprintf(cntbuf, "$n gives $p to $N. (x%d)",
									counter);
							act(cntbuf, TRUE, ch, save_obj, vict, TO_NOTVICT);
							counter = 0;
						}
						found = 1;
						save_obj = obj;
						counter++;
					}
				}
				if (counter == 1)
					sprintf(cntbuf, "You give $p to $N.");
				else
					sprintf(cntbuf, "You give $p to $N. (x%d)", counter);
				act(cntbuf, FALSE, ch, save_obj, vict, TO_CHAR);
				if (counter == 1)
					sprintf(cntbuf, "$n gives you $p.");
				else
					sprintf(cntbuf, "$n gives you $p. (x%d)", counter);
				act(cntbuf, FALSE, ch, save_obj, vict, TO_VICT);
				if (counter == 1)
					sprintf(cntbuf, "$n gives $p to $N.");
				else
					sprintf(cntbuf, "$n gives $p to $N. (x%d)", counter);
				act(cntbuf, TRUE, ch, save_obj, vict, TO_NOTVICT);
			}
		}
	}
}


ACMD(do_plant)
{
	int amount, dotmode;
	struct char_data *vict;
	struct obj_data *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Plant what on who?\r\n", ch);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
			argument = one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)))
				perform_plant_gold(ch, vict, amount);
			return;
		} else if (!str_cmp("credits", arg) || !str_cmp("credit", arg)) {
			argument = one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)))
				perform_plant_credits(ch, vict, amount);
			return;
		} else {
			/* code to give multiple items.  anyone want to write it? -je */
			send_to_char("You can't give more than one item at a time.\r\n",
				ch);
			return;
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV) {
			if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg),
					arg);
				send_to_char(buf, ch);
			} else
				perform_plant(ch, vict, obj);
		} else {
			if (dotmode == FIND_ALLDOT && !*arg) {
				send_to_char("All of what?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				send_to_char("You don't seem to be holding anything.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					if (INVIS_OK_OBJ(ch, obj) &&
						((dotmode == FIND_ALL || isname(arg, obj->name))))
						perform_plant(ch, vict, obj);
				}
		}
	}
}

/* Everything from here down is what was formerly act.obj2.c */


void
weight_change_object(struct obj_data *obj, int weight)
{
	struct obj_data *tmp_obj;
	struct char_data *tmp_ch;

	if (obj->in_room != NULL) {
		obj->modifyWeight(weight);
	}

	else if ((tmp_ch = obj->carried_by)) {
		obj_from_char(obj);
		obj->modifyWeight(weight);
		obj_to_char(obj, tmp_ch);
	}

	else if ((tmp_obj = obj->in_obj)) {
		obj_from_obj(obj);
		obj->modifyWeight(weight);
		obj_to_obj(obj, tmp_obj);
	}

	else {
		slog("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}



void
name_from_drinkcon(struct obj_data *obj)
{
	int i;
	char *new_name;
	struct obj_data *tmp_obj = NULL;

	for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0');
		i++);

	if (*((obj->name) + i) == ' ') {
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		new_name = str_dup((obj->name) + i + 1);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		tmp_obj = real_object_proto(GET_OBJ_VNUM(obj));
		if (GET_OBJ_VNUM(obj) < 0 || obj->name != tmp_obj->name) {
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			free(obj->name);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
		}
		obj->name = new_name;
	}
}

void
name_to_drinkcon(struct obj_data *obj, int type)
{
	char *new_name;
	struct obj_data *tmp_obj = NULL;

	CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
	sprintf(new_name, "%s %s", drinknames[type], obj->name);
	tmp_obj = real_object_proto(GET_OBJ_VNUM(obj));
	if (GET_OBJ_VNUM(obj) < 0 || obj->name != tmp_obj->name) {
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		free(obj->name);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
	}
	obj->name = new_name;
}



ACMD(do_drink)
{
	struct obj_data *temp;
	struct affected_type af;
	int amount, weight, drunk;
	int on_ground = 0;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Drink from what?\r\n", ch);
		return;
	}
	if (!(temp = get_obj_in_list_all(ch, arg, ch->carrying))) {
		if (!(temp = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			return;
		} else
			on_ground = 1;
	}
	if (GET_OBJ_TYPE(temp) == ITEM_POTION) {
		send_to_char("You must QUAFF that!\r\n", ch);
		return;
	}
	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
		(GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
		send_to_char("You can't drink from that!\r\n", ch);
		return;
	}
	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
		send_to_char("You have to be holding that to drink from it.\r\n", ch);
		return;
	}
	if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
		/* The pig is drunk */
		send_to_char("You can't seem to get close enough to your mouth.\r\n",
			ch);
		act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}
	if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
		send_to_char("Your stomach can't contain anymore!\r\n", ch);
		return;
	}
	if (!GET_OBJ_VAL(temp, 1)) {
		send_to_char("It's empty.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_DRINK) {
		if (!ch->desc || !ch->desc->repeat_cmd_count) {
			sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp,
						2)]);
			act(buf, TRUE, ch, temp, 0, TO_ROOM);
		}

		sprintf(buf, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);

		if (IS_VAMPIRE(ch) && GET_OBJ_VAL(temp, 2) == LIQ_BLOOD)
			send_to_char("Ack!  It is cold!\r\n", ch);

		if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
			amount =
				(25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp,
					2)][DRUNK];
		else
			amount = number(3, 10);

	} else {
		act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);
		amount = 1;
	}

	if (GET_OBJ_VAL(temp, 1) != -1)
		amount = MIN(amount, GET_OBJ_VAL(temp, 1));

	/* You can't subtract more than the object weighs */
	weight = MIN(amount, temp->getWeight());

	if (GET_OBJ_VAL(temp, 1) != -1)
		weight_change_object(temp, -weight);	/* Subtract amount */

	if (PRF2_FLAGGED(ch, PRF2_FIGHT_DEBUG)) {
		sprintf(buf, "(%d amount) D%d-F%d-H%d\n", amount,
			(int)drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK],
			(int)drink_aff[GET_OBJ_VAL(temp, 2)][FULL],
			(int)drink_aff[GET_OBJ_VAL(temp, 2)][THIRST]);
		send_to_char(buf, ch);
	}

	if (!IS_VAMPIRE(ch)) {
		drunk = (int)drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount;
		drunk /= 4;
		if (IS_MONK(ch))
			drunk >>= 2;

		gain_condition(ch, DRUNK, drunk);

		gain_condition(ch, FULL,
			(int)((int)drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

		gain_condition(ch, THIRST,
			(int)((int)drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);
	}

	if (GET_COND(ch, DRUNK) > 10)
		send_to_char("You feel pretty damn drunk.\r\n", ch);

	if (GET_COND(ch, THIRST) > 20)
		send_to_char("Your thirst has been quenched.\r\n", ch);

	if (GET_COND(ch, FULL) > 20)
		send_to_char("You belly is satiated.\r\n", ch);

	if (GET_OBJ_VAL(temp, 2) == LIQ_STOUT)
		GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + number(0, 3));

	if (OBJ_IS_RAD(temp)) {		// The shit was radioactive!!
	}
	if (GET_OBJ_VAL(temp, 3)) {	// The shit was poisoned !
		send_to_char("Oops, it tasted rather strange!\r\n", ch);
		if (!number(0, 1))
			act("$n chokes and utters some strange sounds.", FALSE, ch, 0, 0,
				TO_ROOM);
		else
			act("$n sputters and coughs.", FALSE, ch, 0, 0, TO_ROOM);

		af.location = APPLY_STR;
		af.modifier = -2;
		af.duration = amount * 3;
		af.bitvector = 0;
		af.type = SPELL_POISON;
		af.level = 30;

		if (GET_OBJ_VAL(temp, 3) == 2) {
			if (HAS_POISON_3(ch))
				af.duration = amount;
			else
				af.bitvector = AFF3_POISON_2;
			af.aff_index = 3;
		} else if (GET_OBJ_VAL(temp, 3) == 3) {
			af.bitvector = AFF3_POISON_3;
			af.aff_index = 3;
		} else if (GET_OBJ_VAL(temp, 3) == 4) {
			af.type = SPELL_SICKNESS;
			af.bitvector = AFF3_SICKNESS;
			af.aff_index = 3;
		} else {
			if (HAS_POISON_3(ch) || HAS_POISON_2(ch))
				af.duration = amount;
			else
				af.bitvector = AFF_POISON;
			af.aff_index = 1;
		}

		affect_join(ch, &af, TRUE, TRUE, TRUE, FALSE);
	}
	/* empty the container, and no longer poison. */

	if (GET_OBJ_VAL(temp, 1) != -1) {
		GET_OBJ_VAL(temp, 1) -= amount;
		if (!GET_OBJ_VAL(temp, 1)) {	/* The last bit */
			GET_OBJ_VAL(temp, 2) = 0;
			GET_OBJ_VAL(temp, 3) = 0;
			name_from_drinkcon(temp);
		}
	}
	return;
}



ACMD(do_eat)
{
	struct obj_data *food;
	struct affected_type af;
	int amount;
	bool extract_ok = TRUE;
	int my_return_flags = 0;

	if (return_flags == NULL)
		return_flags = &my_return_flags;
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Eat what?\r\n", ch);
		return;
	}
	if (!(food = get_obj_in_list_all(ch, arg, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
		return;
	}
	if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}
	if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_GOD)) {
		send_to_char("You can't eat THAT!\r\n", ch);
		return;
	}
	if (GET_COND(ch, FULL) > 20) {	/* Stomach full */
		act("Your belly is stuffed to capacity!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (subcmd == SCMD_EAT) {
		if (GET_LEVEL(ch) < LVL_GOD)
			act("You eat the $o.", FALSE, ch, food, 0, TO_CHAR);
		else
			act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
		act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
	} else {
		act("You nibble a little bit of the $o.", FALSE, ch, food, 0, TO_CHAR);
		act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
	}

	amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

	gain_condition(ch, FULL, amount);

	if ((GET_LEVEL(ch) >= LVL_AMBASSADOR
			|| ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) && !IS_MOB(ch))
		extract_ok = FALSE;		// sometimes a death extracts the char's objs too

	if (IS_OBJ_TYPE(food, ITEM_FOOD)) {
		if (IS_OBJ_STAT(food, ITEM_EVIL_BLESS)) {
			if (!call_magic(ch, ch, 0, SPELL_ESSENCE_OF_EVIL, GET_OBJ_VAL(food,
						1), CAST_SPELL)) {
				if (extract_ok)
					extract_obj(food);
				return;
			} else {
				if (*return_flags && extract_ok) {
					extract_obj(food);
					return;
				}
			}
		} else if (IS_OBJ_STAT(food, ITEM_BLESS)) {
			if (!call_magic(ch, ch, 0, SPELL_ESSENCE_OF_GOOD, GET_OBJ_VAL(food,
						1), CAST_SPELL)) {
				if (extract_ok)
					extract_obj(food);
				return;
			} else {
				if (*return_flags && extract_ok) {
					extract_obj(food);
					return;
				}
			}
		}
		if ((GET_OBJ_VAL(food, 1) != 0) && (GET_OBJ_VAL(food, 2) != 0) &&
			(GET_OBJ_TYPE(food) == ITEM_FOOD) && subcmd == SCMD_EAT) {
			if (!mag_objectmagic(ch, food, buf)) {
				if (extract_ok)
					extract_obj(food);
				return;
			} else {
				if (*return_flags && extract_ok) {
					extract_obj(food);
					return;
				}
			}
		}
	}
	if (IS_OBJ_TYPE(food, ITEM_FOOD) && GET_COND(ch, FULL) > 20)
		act("You are satiated.", FALSE, ch, 0, 0, TO_CHAR);

	if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		/* The shit was poisoned ! */
		send_to_char("Oops, that tasted rather strange!\r\n", ch);
		if (!number(0, 1))
			act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0,
				TO_ROOM);
		else
			act("$n sputters and chokes.", FALSE, ch, 0, 0, TO_ROOM);

		af.location = APPLY_STR;
		af.modifier = -2;
		af.duration = amount * 3;
		af.bitvector = 0;
		af.type = SPELL_POISON;

		if (GET_OBJ_VAL(food, 3) == 2) {
			if (HAS_POISON_3(ch))
				af.duration = amount;
			else
				af.bitvector = AFF3_POISON_2;
			af.aff_index = 3;
		} else if (GET_OBJ_VAL(food, 3) == 3) {
			af.bitvector = AFF3_POISON_3;
			af.aff_index = 3;
		} else if (GET_OBJ_VAL(food, 3) == 4) {
			af.type = SPELL_SICKNESS;
			af.bitvector = AFF3_SICKNESS;
			af.aff_index = 3;
			send_to_char("You feel sick.\r\n", ch);
		} else {
			if (HAS_POISON_3(ch) || HAS_POISON_2(ch))
				af.duration = amount;
			else
				af.bitvector = AFF_POISON;
			af.aff_index = 1;
		}

		if (subcmd != SCMD_EAT)
			af.duration >>= 1;
		affect_join(ch, &af, TRUE, TRUE, TRUE, FALSE);
	}

	if (subcmd == SCMD_EAT)
		extract_obj(food);
	else {
		if (!(--GET_OBJ_VAL(food, 0))) {
			send_to_char("There's nothing left now.\r\n", ch);
			extract_obj(food);
		}
	}
}


ACMD(do_pour)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *from_obj = NULL;
	struct obj_data *to_obj = NULL;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR) {
		if (!*arg1) {			/* No arguments */
			act("What do you want to pour from?", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
			act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
	}
	if (subcmd == SCMD_FILL) {
		if (!*arg1) {			/* no arguments */
			send_to_char
				("What do you want to fill?  And what are you filling it from?\r\n",
				ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
			act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (GET_OBJ_VAL(to_obj, 0) == -1) {
			act("No need to fill $p, because it never runs dry!",
				FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!*arg2) {			/* no 2nd argument */
			act("What do you want to fill $p from?", FALSE, ch, to_obj, 0,
				TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
			sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2),
				arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
			act("You can't fill something from $p.", FALSE, ch, from_obj, 0,
				TO_CHAR);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("$p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}

	if (subcmd == SCMD_POUR) {	/* pour */
		if (!*arg2) {
			act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0,
				TO_CHAR);
			return;
		}
		if (!str_cmp(arg2, "out")) {
			if (GET_OBJ_VAL(from_obj, 0) == -1) {
				send_to_char("Now that would be an exercise in futility!\r\n",
					ch);
				return;
			}
			act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));	/* Empty */
			GET_OBJ_VAL(from_obj, 1) = 0;
			GET_OBJ_VAL(from_obj, 2) = 0;
			GET_OBJ_VAL(from_obj, 3) = 0;

			name_from_drinkcon(from_obj);

			return;
		}
		if (!(to_obj = get_obj_in_list_all(ch, arg2, ch->carrying))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
			act("You can't pour anything into that.", FALSE, ch, 0, 0,
				TO_CHAR);
			return;
		}
	}
	if (to_obj == from_obj) {
		act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
		(GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
		act("There is already another liquid in it!", FALSE, ch, 0, 0,
			TO_CHAR);
		return;
	}
	if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)) ||
		GET_OBJ_VAL(to_obj, 0) == -1) {
		act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 0) == -1) {
		act("You cannot seem to pour from $p.", FALSE, ch, from_obj, 0,
			TO_CHAR);
		return;
	}
	if (subcmd == SCMD_POUR) {
		sprintf(buf, "You pour the %s into the %s.\r\n",
			drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
		send_to_char(buf, ch);
	}
	if (subcmd == SCMD_FILL) {
		act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj,
			TO_CHAR);
		act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj,
			TO_ROOM);
	}
	/* New alias */
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

	/* First same type liq. */
	GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

	/* Then how much to pour */
	GET_OBJ_VAL(from_obj, 1) -= (amount =
		(GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

	GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

	if (GET_OBJ_VAL(from_obj, 1) < 0) {	/* There was too little */
		GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
		amount += GET_OBJ_VAL(from_obj, 1);
		GET_OBJ_VAL(from_obj, 1) = 0;
		GET_OBJ_VAL(from_obj, 2) = 0;
		GET_OBJ_VAL(from_obj, 3) = 0;
		name_from_drinkcon(from_obj);
	}
	/* Then the poison boogie */
	GET_OBJ_VAL(to_obj, 3) =
		(GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

	/* And the weight boogie */
	weight_change_object(from_obj, -amount);
	weight_change_object(to_obj, amount);	/* Add weight */

	return;
}



void
wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
	char *wear_messages[][2] = {
		{"$n lights $p and holds it.",
			"You light $p and hold it."},

		{"$n slides $p on to $s right ring finger.",
			"You slide $p on to your right ring finger."},

		{"$n slides $p on to $s left ring finger.",
			"You slide $p on to your left ring finger."},

		{"$n wears $p around $s neck.",
			"You wear $p around your neck."},

		{"$n wears $p around $s neck.",
			"You wear $p around your neck."},

		{"$n wears $p on $s body.",
			"You wear $p on your body.",},

		{"$n wears $p on $s head.",
			"You wear $p on your head."},

		{"$n puts $p on $s legs.",
			"You put $p on your legs."},

		{"$n wears $p on $s feet.",
			"You wear $p on your feet."},

		{"$n puts $p on $s hands.",
			"You put $p on your hands."},

		{"$n wears $p on $s arms.",
			"You wear $p on your arms."},

		{"$n straps $p around $s arm as a shield.",
			"You start to use $p as a shield."},

		{"$n wears $p about $s body.",
			"You wear $p around your body."},

		{"$n wears $p around $s waist.",
			"You wear $p around your waist."},

		{"$n puts $p on around $s right wrist.",
			"You put $p on around your right wrist."},

		{"$n puts $p on around $s left wrist.",
			"You put $p on around your left wrist."},

		{"$n wields $p.",
			"You wield $p."},

		{"$n grabs $p.",
			"You grab $p."},

		{"$n puts $p on $s crotch.",
			"You put $p on your crotch."},

		{"$n puts $p on $s eyes.",
			"You put $p on your eyes."},

		{"$n wears $p on $s back.",
			"You wear $p on your back."},

		{"$n attaches $p to $s belt.",
			"You attach $p to your belt."},

		{"$n wears $p on $s face.",
			"You wear $p on your face."},

		{"$n puts $p in $s left ear lobe.",
			"You wear $p in your left ear."},

		{"$n puts $p in $s right ear lobe.",
			"You wear $p in your right ear."},

		{"$n wields $p in $s off hand.",
			"You wield $p in your off hand."},

		{"$n RAMS $p up $s ass!!!",
			"You RAM $p up your ass!!!"}

	};

	act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
	act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

/* returns same as equip_char(), TRUE = killed da ho */
int
perform_wear(struct char_data *ch, struct obj_data *obj, int where)
{
	int i;
	char *already_wearing[] = {
		"You're already using a light.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something on both of your ring fingers.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You can't wear anything else around your neck.\r\n",
		"You're already wearing something on your body.\r\n",
		"You're already wearing something on your head.\r\n",
		"You're already wearing something on your legs.\r\n",
		"You're already wearing something on your feet.\r\n",
		"You're already wearing something on your hands.\r\n",
		"You're already wearing something on your arms.\r\n",
		"You're already using a shield.\r\n",
		"You're already wearing something about your body.\r\n",
		"You already have something around your waist.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something around both of your wrists.\r\n",
		"You're already wielding a weapon.\r\n",
		"You're already holding something.\r\n",
		"You've already got something on your crotch.\r\n",
		"You already have something on your eyes.\r\n",
		"You already have something on your back.\r\n",
		"There is already something attached to your belt.\r\n",
		"You are already wearing something on your face.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You are wearing something in both ears already.\r\n",
		"You are already wielding something in your off hand.\r\n",
		"You already have something stuck up yer butt!\r\n"
	};

	/* first, make sure that the wear position is valid. */
	if (!CAN_WEAR(obj, wear_bitvectors[where])) {
		act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}
	if (IS_IMPLANT(obj) && where != WEAR_HOLD) {
		act("You cannot wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}
	/* for neck, finger, and wrist, ears try pos 2 if pos 1 is already full */
	if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
		(where == WEAR_WRIST_R) || (where == WEAR_EAR_L))
		if (GET_EQ(ch, where))
			where++;

	if (GET_OBJ_TYPE(obj) == ITEM_TATTOO) {
		act("You cannot wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	if (GET_EQ(ch, where)) {
		send_to_char(already_wearing[where], ch);
		return 0;
	}
	if ((where == WEAR_BELT) && (!GET_EQ(ch, WEAR_WAIST))) {
		act("You need to be WEARING a belt to put $p on it.",
			FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}
	if (where == WEAR_HANDS && IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED)) {
		if (GET_EQ(ch, WEAR_WIELD)) {
			act("You can't be using your hands for anything else to use $p.",
				FALSE, ch, obj, 0, TO_CHAR);
			return 0;
		}
	}
	if (where == WEAR_SHIELD || where == WEAR_HOLD) {
		if (GET_EQ(ch, WEAR_WIELD)) {
			if (IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
				act("You can't use $p while wielding your two handed weapon.",
					FALSE, ch, obj, 0, TO_CHAR);
				return 0;
			} else if (GET_EQ(ch, WEAR_WIELD_2)) {
				act("You can't use $p while dual wielding.",
					FALSE, ch, obj, 0, TO_CHAR);
				return 0;
			} else if (GET_EQ(ch, WEAR_HOLD)) {
				act("You are currently wielding $p and holding $P.",
					FALSE, ch, GET_EQ(ch, WEAR_WIELD), GET_EQ(ch, WEAR_HOLD),
					TO_CHAR);
				return 0;
			} else if (GET_EQ(ch, WEAR_SHIELD)) {
				act("You are currently wielding $p and using $P.",
					FALSE, ch, GET_EQ(ch, WEAR_WIELD), GET_EQ(ch, WEAR_SHIELD),
					TO_CHAR);
				return 0;
			}
		}
		if (GET_EQ(ch, WEAR_HANDS)) {
			if (IS_OBJ_STAT2(GET_EQ(ch, WEAR_HANDS), ITEM2_TWO_HANDED)) {
				act("You can't use $p while wearing $P on your hands.",
					FALSE, ch, obj, GET_EQ(ch, WEAR_HANDS), TO_CHAR);
				return 0;
			}
		}
	}
	// If the shield's too heavy, they cant make good use of it.
	if (where == WEAR_SHIELD
		&& obj->getWeight() >
		1.5 * str_app[STRENGTH_APPLY_INDEX(ch)].wield_w) {
		send_to_char("It's too damn heavy.\r\n", ch);
		return 0;
	}
	if (!OBJ_APPROVED(obj) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
		!ch->isTester()) {
		act("$p has not been approved for mortal use.",
			FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}
	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN) && GET_LEVEL(ch) < LVL_GOD) {
		act("$p is broken, and unusable until it is repaired.",
			FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}
	if ((where == WEAR_WIELD || where == WEAR_WIELD_2) &&
		GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
		!IS_ENERGY_GUN(obj) && !IS_GUN(obj)) {
		send_to_char("You cannot wield non-weapons.\r\n", ch);
		return 0;
	}
	if (IS_OBJ_STAT2(obj, ITEM2_NO_MORT) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
		!IS_REMORT(ch)) {
		act("You feel a strange sensation as you attempt to use $p.\r\n"
			"The universe slowly spins around you, and the fabric of space\r\n"
			"and time appears to warp.", FALSE, ch, 0, obj, TO_CHAR);
		return 0;
	}

	if (IS_OBJ_STAT2(obj, ITEM2_SINGULAR)) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(ch, i) &&
				(GET_OBJ_VNUM(GET_EQ(ch, i)) == GET_OBJ_VNUM(obj))) {
				act("$p:  You cannot use more than one of this item.",
					FALSE, ch, obj, 0, TO_CHAR);
				return 0;
			}
		}
	}
	if (IS_OBJ_STAT2(obj, ITEM2_GODEQ) && GET_LEVEL(ch) < LVL_IMPL) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(ch, i) && !IS_OBJ_STAT2(GET_EQ(ch, i), ITEM2_GODEQ)) {
				act("$p swiftly slides off your body, into your hands.",
					FALSE, ch, GET_EQ(ch, i), 0, TO_CHAR);
				obj_to_char(unequip_char(ch, i, MODE_EQ), ch);
			}
		}
	} else if (GET_LEVEL(ch) < LVL_IMPL) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(ch, i) && IS_OBJ_STAT2(GET_EQ(ch, i), ITEM2_GODEQ)) {
				act("$p swiftly leaps off your body, into your hands.",
					FALSE, ch, GET_EQ(ch, i), 0, TO_CHAR);
				obj_to_char(unequip_char(ch, i, MODE_EQ), ch);
			}
		}
	}

	wear_message(ch, obj, where);
	if ((IS_OBJ_TYPE(obj, ITEM_WORN) ||
			IS_OBJ_TYPE(obj, ITEM_ARMOR) ||
			IS_OBJ_TYPE(obj, ITEM_WEAPON)) && obj->action_description) {
		act(obj->action_description, FALSE, ch, obj, 0, TO_CHAR);
	}
	obj_from_char(obj);
	return (equip_char(ch, obj, where, MODE_EQ));
}


int
find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
	int where = -1;

	if (!arg || !*arg) {
		if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
			where = WEAR_FINGER_R;
		if (CAN_WEAR(obj, ITEM_WEAR_NECK))
			where = WEAR_NECK_1;
		if (CAN_WEAR(obj, ITEM_WEAR_BODY))
			where = WEAR_BODY;
		if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
			where = WEAR_HEAD;
		if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
			where = WEAR_LEGS;
		if (CAN_WEAR(obj, ITEM_WEAR_FEET))
			where = WEAR_FEET;
		if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
			where = WEAR_HANDS;
		if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
			where = WEAR_ARMS;
		if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
			where = WEAR_SHIELD;
		if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
			where = WEAR_ABOUT;
		if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
			where = WEAR_WAIST;
		if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
			where = WEAR_WRIST_R;
		if (CAN_WEAR(obj, ITEM_WEAR_CROTCH))
			where = WEAR_CROTCH;
		if (CAN_WEAR(obj, ITEM_WEAR_EYES))
			where = WEAR_EYES;
		if (CAN_WEAR(obj, ITEM_WEAR_BACK))
			where = WEAR_BACK;
		if (CAN_WEAR(obj, ITEM_WEAR_BELT))
			where = WEAR_BELT;
		if (CAN_WEAR(obj, ITEM_WEAR_FACE))
			where = WEAR_FACE;
		if (CAN_WEAR(obj, ITEM_WEAR_EAR))
			where = WEAR_EAR_L;
		if (CAN_WEAR(obj, ITEM_WEAR_ASS))
			where = WEAR_ASS;
	} else {
		if ((where = search_block(arg, wear_keywords, FALSE)) < 0) {
			sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
			send_to_char(buf, ch);
		}
	}

	return where;
}



ACMD(do_wear)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *obj, *next_obj;
	int where, dotmode, items_worn = 0;

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char("Wear what?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);

	if (*arg2 && (dotmode != FIND_INDIV)) {
		send_to_char
			("You can't specify the same body location for more than one item!\r\n",
			ch);
		return;
	}
	if (dotmode == FIND_ALL) {
		for (obj = ch->carrying; obj; obj = next_obj) {
			next_obj = obj->next_content;
			if (INVIS_OK_OBJ(ch, obj)
				&& (where = find_eq_pos(ch, obj, 0)) >= 0) {
				items_worn++;
				if (perform_wear(ch, obj, where))
					return;
			}
		}
		if (!items_worn)
			send_to_char("You don't seem to have anything wearable.\r\n", ch);
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg1) {
			send_to_char("Wear all of what?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
			send_to_char(buf, ch);
		} else
			while (obj) {
				next_obj = get_obj_in_list_all(ch, arg1, obj->next_content);
				if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
					if (perform_wear(ch, obj, where))
						return;
				} else
					act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
				obj = next_obj;
			}
	} else {
		if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
			sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
			send_to_char(buf, ch);
		} else {
			if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
				perform_wear(ch, obj, where);
			} else if (!*arg2)
				act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
}


ACCMD(do_wield)
{
	struct obj_data *obj;
	int hands_free = 2;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Wield what?\r\n", ch);
		return;
	}

	if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
		return;
	}

	if (!CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
		send_to_char("You can't wield that.\r\n", ch);
		return;
	}

	if (obj->getWeight() > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w) {
		send_to_char("It's too damn heavy.\r\n", ch);
		return;
	}

	if (IS_CLERIC(ch) && !IS_EVIL(ch) &&
		(GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
		((GET_OBJ_VAL(obj, 3) == TYPE_SLASH - TYPE_HIT) ||
			(GET_OBJ_VAL(obj, 3) == TYPE_PIERCE - TYPE_HIT) ||
			(GET_OBJ_VAL(obj, 3) == TYPE_STAB - TYPE_HIT) ||
			(GET_OBJ_VAL(obj, 3) == TYPE_RIP - TYPE_HIT) ||
			(GET_OBJ_VAL(obj, 3) == TYPE_CHOP - TYPE_HIT) ||
			(GET_OBJ_VAL(obj, 3) == TYPE_CLAW - TYPE_HIT))) {
		send_to_char("You can't wield that as a cleric.\r\n", ch);
		return;
	}

	hands_free = char_hands_free(ch);

	if (!hands_free) {
		send_to_char("You don't have a hand free to wield it with.\r\n", ch);
		return;
	}

	if (hands_free != 2 && IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED)) {
		act("You need both hands free to wield $p.", FALSE, ch, obj, 0,
			TO_CHAR);
		return;
	}

	if (GET_EQ(ch, WEAR_HANDS) &&
		IS_OBJ_STAT2(GET_EQ(ch, WEAR_HANDS), ITEM2_TWO_HANDED)) {
		act("You can't wield anything while wearing $p on your hands.",
			FALSE, ch, GET_EQ(ch, WEAR_HANDS), 0, TO_CHAR);
		return;
	}

	if (GET_EQ(ch, WEAR_WIELD)) {
		// Guns and Mercs have to be handled a bit differently, but I hate they
		// way I've done this.  However, I can't think of a better way ATM...
		// feel free to clean this up if you like
		if (IS_MERC(ch) && IS_ANY_GUN(GET_EQ(ch, WEAR_WIELD))
			&& IS_ANY_GUN(obj))
			perform_wear(ch, obj, WEAR_WIELD_2);

		else if (GET_EQ(ch, WEAR_WIELD)->getWeight() <= 6 ?
			(obj->getWeight() > GET_EQ(ch, WEAR_WIELD)->getWeight()) :
			(obj->getWeight() > (GET_EQ(ch, WEAR_WIELD)->getWeight() >> 1))
			)
			send_to_char
				("Your secondary weapon must weigh less than half of your primary weapon,\r\nif your primary weighs more than 6 lbs.\r\n",
				ch);
		else
			perform_wear(ch, obj, WEAR_WIELD_2);
	} else
		perform_wear(ch, obj, WEAR_WIELD);
}



ACMD(do_grab)
{
	struct obj_data *obj;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Hold what?\r\n", ch);
	else if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	} else {
		if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
			if (!GET_OBJ_VAL(obj, 2))
				act("$p is no longer usable as a light.", FALSE, ch, obj, 0,
					TO_CHAR);
			else
				perform_wear(ch, obj, WEAR_LIGHT);
		} else {
			if (!CAN_WEAR(obj, ITEM_WEAR_HOLD)
				&& GET_OBJ_TYPE(obj) != ITEM_WAND
				&& GET_OBJ_TYPE(obj) != ITEM_STAFF
				&& GET_OBJ_TYPE(obj) != ITEM_SCROLL
				&& GET_OBJ_TYPE(obj) != ITEM_POTION
				&& GET_OBJ_TYPE(obj) != ITEM_SYRINGE)
				send_to_char("You can't hold that.\r\n", ch);
			else
				perform_wear(ch, obj, WEAR_HOLD);
		}
	}
}

void
perform_remove(struct char_data *ch, int pos)
{
	struct obj_data *obj;

	if (!(obj = GET_EQ(ch, pos))) {
		slog("Error in perform_remove: bad pos passed.");
		return;
	}
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items!", FALSE, ch, obj, 0,
			TO_CHAR);
	else if ((ch->getPosition() == POS_FLYING)
		&& (GET_OBJ_TYPE(ch->equipment[pos]) == ITEM_WINGS)
		&& (!IS_AFFECTED(ch, AFF_INFLIGHT))) {
		act("$p: you probably shouldn't remove those while flying!", FALSE, ch,
			obj, 0, TO_CHAR);
	} else if (GET_OBJ_TYPE(obj) == ITEM_TATTOO)
		act("$p: you must have this removed by a professional.",
			FALSE, ch, obj, 0, TO_CHAR);
	else if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)
		&& GET_LEVEL(ch) < LVL_AMBASSADOR)
		act("$p: you cannot convince yourself to stop using it.", FALSE, ch,
			obj, 0, TO_CHAR);
	else {
		obj_to_char(unequip_char(ch, pos, MODE_EQ), ch);
		if (pos == WEAR_WAIST && GET_EQ(ch, WEAR_BELT))
			obj_to_char(unequip_char(ch, WEAR_BELT, MODE_EQ), ch);

		if ((pos == WEAR_WIELD) && GET_EQ(ch, WEAR_WIELD_2)) {
			equip_char(ch, unequip_char(ch, WEAR_WIELD_2, MODE_EQ),
				WEAR_WIELD, MODE_EQ);
			act("You stop using $p, and wield $P.",
				FALSE, ch, obj, GET_EQ(ch, WEAR_WIELD), TO_CHAR);
			act("$n stops using $p, and wields $P.",
				TRUE, ch, obj, GET_EQ(ch, WEAR_WIELD), TO_ROOM);
		} else {
			act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
		}

	}
}


int
char_hands_free(CHAR * ch)
{
	struct obj_data *obj;
	int hands_free;

	hands_free = 2;

	obj = GET_EQ(ch, WEAR_WIELD);
	if (obj)
		hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
	obj = GET_EQ(ch, WEAR_WIELD_2);
	if (obj)
		hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
	obj = GET_EQ(ch, WEAR_SHIELD);
	if (obj)
		hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
	obj = GET_EQ(ch, WEAR_HOLD);
	if (obj)
		hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;

	if (hands_free < 0) {
		char buf[2048];

		// Report it to the immortals
		snprintf(buf, 2047, "SYSERR: %s has negative hands free",
			GET_NAME(ch));
		slog(buf);
		// Now fix the problem
		act("Oops... You dropped everything that was in your hands.",
			FALSE, ch, NULL, NULL, TO_CHAR);
		if (GET_EQ(ch, WEAR_WIELD))
			perform_remove(ch, WEAR_WIELD);
		if (GET_EQ(ch, WEAR_WIELD_2))
			perform_remove(ch, WEAR_WIELD_2);
		if (GET_EQ(ch, WEAR_SHIELD))
			perform_remove(ch, WEAR_SHIELD);
		if (GET_EQ(ch, WEAR_HOLD))
			perform_remove(ch, WEAR_HOLD);
		hands_free = 2;
	}
	return hands_free;
}

ACMD(do_remove)
{
	struct obj_data *obj;
	int i, dotmode, found;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Remove what?\r\n", ch);
		return;
	}
	if (AFF3_FLAGGED(ch, AFF3_ATTRACTION_FIELD) && !NOHASS(ch)) {
		send_to_char
			("You cannot remove anything while generating an attraction field!\r\n",
			ch);
		return;
	}

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_ALL) {
		found = 0;
		for (i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i)) {
				perform_remove(ch, i);
				found = 1;
				if (GET_EQ(ch, i))
					perform_remove(ch, i);
			}
		if (!found)
			send_to_char("You're not using anything.\r\n", ch);
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg)
			send_to_char("Remove all of what?\r\n", ch);
		else {
			found = 0;
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i) && INVIS_OK_OBJ(ch, GET_EQ(ch, i)) &&
					isname(arg, GET_EQ(ch, i)->name)) {
					perform_remove(ch, i);
					found = 1;
				}
			if (!found) {
				sprintf(buf, "You don't seem to be using any %ss.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	} else {
		if (!(obj = get_object_in_equip_all(ch, arg, ch->equipment, &i))) {
			sprintf(buf, "You don't seem to be using %s %s.\r\n", AN(arg),
				arg);
			send_to_char(buf, ch);
		} else if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE) &&
			GET_LEVEL(ch) < LVL_AMBASSADOR)
			act("Ack!  You cannot seem to let go of $p!", FALSE, ch, obj, 0,
				TO_CHAR);
		else
			perform_remove(ch, i);
	}
}

int
prototype_obj_value(struct obj_data *obj)
{
	int value = 0, prev_value = 0, j;

	if (!obj) {
		slog("SYSERR: NULL obj passed to prototype_obj_value.");
		return 0;
	}


	/***** First the item-type specifics. ********/

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_LIGHT:
		if (GET_OBJ_VAL(obj, 2) < 0)
			value += 10000;
		else
			value += (GET_OBJ_VAL(obj, 2));

		if (IS_METAL_TYPE(obj))
			value *= 2;
		break;

	case ITEM_CONTAINER:
		value += GET_OBJ_VAL(obj, 0);
		value <<= 2;
		value += ((30 - obj->getWeight()) >> 2);
		if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
			value += 100;
		if (GET_OBJ_VAL(obj, 2)) {
			value += 100;
			if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF))
				value += 100;
		}
		if (GET_OBJ_VAL(obj, 3))			   /** corpse **/
			value = 0;
		break;

	case ITEM_WEAPON:
		value += GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 1) *
			GET_OBJ_VAL(obj, 1) *
			GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 2) * 4;

		if (GET_OBJ_VAL(obj, 0) && IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON)) {
			prev_value = value;
			value += MIN(value << 2, 200000);
			value = (prev_value + value) >> 1;
		}
		break;

	case ITEM_GUN:
		if (GUN_TYPE(obj) >= 0 && GUN_TYPE(obj) < NUM_GUN_TYPES)
			value += (gun_damage[GUN_TYPE(obj)][0] *
				gun_damage[GUN_TYPE(obj)][1]) << 4;
		if (!MAX_LOAD(obj))
			value *= 15;
		else
			value *= MAX_LOAD(obj);
		break;

	case ITEM_BULLET:
		if (GUN_TYPE(obj) >= 0 && GUN_TYPE(obj) < NUM_GUN_TYPES)
			value +=
				(gun_damage[GUN_TYPE(obj)][0] * gun_damage[GUN_TYPE(obj)][1]);
		value += (BUL_DAM_MOD(obj) * 100);
		break;

	case ITEM_ENERGY_GUN:
		value += GUN_DISCHARGE(obj) * GUN_DISCHARGE(obj) *
			GUN_DISCHARGE(obj) * MAX_R_O_F(obj) * MAX_R_O_F(obj) *
			MAX_R_O_F(obj);

		break;

	case ITEM_BATTERY:
		value += MAX_ENERGY(obj) * 10;
		break;

	case ITEM_ENERGY_CELL:
		value += MAX_ENERGY(obj) * 20;
		break;

	case ITEM_ARMOR:
		value += (GET_OBJ_VAL(obj, 0) * GET_OBJ_VAL(obj, 0) *
			GET_OBJ_VAL(obj, 0) * GET_OBJ_VAL(obj, 0));
		if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
			value *= 3;
			prev_value = value;
			value += ((50 - obj->getWeight()) << 3);
			value = (prev_value + value) >> 1;
		} else if (CAN_WEAR(obj, ITEM_WEAR_HEAD)
			|| CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
			value *= 2;
			prev_value = value;
			value += ((25 - obj->getWeight()) << 3);
			value = (prev_value + value) >> 1;
		} else {
			prev_value = value;
			value += ((25 - obj->getWeight()) << 3);
			value = (prev_value + value) >> 1;
		}
		if (IS_IMPLANT(obj))
			value <<= 1;
		break;

	default:
		value = GET_OBJ_COST(obj);	/* resets value for un-def'd item types ** */
		return value;
		break;
	}

	/***** Make sure its not negative... if it is maybe make it pos. **/
	value = MAX(value, -(value >> 1));

	/**** Next the values of the item's affections *****/
	for (j = 0; j < MAX_OBJ_AFFECT; j++) {

		switch (obj->affected[j].location) {
		case APPLY_STR:
		case APPLY_DEX:
		case APPLY_INT:
		case APPLY_WIS:
		case APPLY_CON:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 4;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_CHA:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 5;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_HIT:
		case APPLY_MOVE:
		case APPLY_MANA:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 25;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_AC:
			prev_value = value;
			value -= (obj->affected[j].modifier) * value / 10;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_HITROLL:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 5;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_DAMROLL:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 4;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_SAVING_PARA:
		case APPLY_SAVING_ROD:
		case APPLY_SAVING_PETRI:
		case APPLY_SAVING_BREATH:
		case APPLY_SAVING_SPELL:
			prev_value = value;
			value -= (obj->affected[j].modifier) * value / 4;
			value = (prev_value + value) >> 1;
			break;

		case APPLY_SNEAK:
		case APPLY_HIDE:
		case APPLY_BACKSTAB:
		case APPLY_PICK_LOCK:
		case APPLY_PUNCH:
		case APPLY_SHOOT:
		case APPLY_KICK:
		case APPLY_TRACK:
		case APPLY_IMPALE:
		case APPLY_BEHEAD:
		case APPLY_THROWING:
			prev_value = value;
			value += (obj->affected[j].modifier) * value / 15;
			value = (prev_value + value) >> 1;
			break;

		}
	}

	/***** Make sure its not negative... if it is maybe make it pos. **/
	value = MAX(value, -(value >> 1));

	/***** The value of items which set bitvectors ***** */
	for (j = 0; j < 3; j++)
		if (obj->obj_flags.bitvector[j]) {
			prev_value = value;
			value += 120000;
			value = (prev_value + value) >> 1;
		}


	switch (GET_OBJ_MATERIAL(obj)) {
	case MAT_GOLD:
		value *= 3;
		break;
	case MAT_SILVER:
		value *= 2;
		break;
	case MAT_PLATINUM:
		value *= 4;
		break;
	case MAT_KEVLAR:
		value *= 2;
		break;
	}

	return value;
}

int
set_maxdamage(struct obj_data *obj)
{

	int dam = 0;

	dam = (obj->getWeight() >> 1);

	if (IS_LEATHER_TYPE(obj))
		dam *= 3;
	else if (IS_CLOTH_TYPE(obj))
		dam *= 2;
	else if (IS_WOOD_TYPE(obj))
		dam *= 4;
	else if (IS_METAL_TYPE(obj))
		dam *= 5;
	else if (IS_PLASTIC_TYPE(obj))
		dam *= 3;
	else if (IS_STONE_TYPE(obj))
		dam *= 6;

	if (GET_OBJ_MATERIAL(obj) == MAT_HARD_LEATHER)
		dam *= 2;
	else if (GET_OBJ_MATERIAL(obj) == MAT_SCALES)
		dam *= 3;

	switch (obj->obj_flags.type_flag) {
	case ITEM_WEAPON:
		dam *= 30;
		break;
	case ITEM_ARMOR:
		dam *= 30;
		dam *= MAX(1, GET_OBJ_VAL(obj, 0));
		break;
	case ITEM_ENGINE:
		dam *= 20;
		break;
	case ITEM_VEHICLE:
		dam *= 18;
		break;
	case ITEM_SCUBA_TANK:
		dam *= 5;
		break;
	case ITEM_BOAT:
		dam *= 10;
		break;
	case ITEM_BATTERY:
		dam *= 3;
		break;
	case ITEM_ENERGY_GUN:
		dam *= 7;
		break;
	case ITEM_METAL:
		dam *= 30;
		break;
	case ITEM_CHIT:
		dam *= 5;
		break;
	case ITEM_CONTAINER:
		dam *= 5;
		break;
	case ITEM_HOLY_SYMB:
		dam *= (MAX(1, GET_OBJ_VAL(obj, 2) / 10));
		dam *= (MAX(1, GET_OBJ_VAL(obj, 3) / 10));
		break;
	case ITEM_FOUNTAIN:
		dam *= 6;
		break;
	}

	if (IS_OBJ_STAT(obj, ITEM_MAGIC))
		dam <<= 1;
	if (IS_OBJ_STAT(obj, ITEM_MAGIC_NODISPEL))
		dam <<= 2;
	if (IS_OBJ_STAT(obj, ITEM_BLESS))
		dam <<= 1;
	if (IS_OBJ_STAT(obj, ITEM_EVIL_BLESS))
		dam <<= 1;
	if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
		dam <<= 1;
	if (IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM))
		dam <<= 1;
	if (IS_OBJ_STAT2(obj, ITEM2_GODEQ))
		dam <<= 1;

	dam = MAX(100, dam);

	return (dam);

}


int
choose_material(struct obj_data *obj)
{
	int i;
	char name[128], aliases[512], *ptr;

	for (i = 0; i < TOP_MATERIAL; i++)
		if (isname(material_names[i], obj->name))
			return (i);

	strcpy(aliases, obj->short_description);
	ptr = aliases;

	ptr = one_argument(ptr, name);
	while (*name) {
		if (str_cmp(name, "a") && str_cmp(name, "an") && str_cmp(name, "the")
			&& str_cmp(name, "some") && str_cmp(name, "of")
			&& str_cmp(name, "black") && str_cmp(name, "white")
			&& str_cmp(name, "green") && str_cmp(name, "brown")) {
			for (i = 0; i < TOP_MATERIAL; i++)
				if (isname(material_names[i], name))
					return (i);
		}
		ptr = one_argument(ptr, name);
	}

	if (obj->description) {
		strcpy(aliases, obj->description);
		ptr = aliases;

		ptr = one_argument(ptr, name);
		while (*name) {
			if (str_cmp(name, "a") && str_cmp(name, "an")
				&& str_cmp(name, "the") && str_cmp(name, "some")
				&& str_cmp(name, "of") && str_cmp(name, "black")
				&& str_cmp(name, "white") && str_cmp(name, "green")
				&& str_cmp(name, "brown") && str_cmp(name, "lying")
				&& str_cmp(name, "has") && str_cmp(name, "been")
				&& str_cmp(name, "is") && str_cmp(name, "left")
				&& str_cmp(name, "here") && str_cmp(name, "set")
				&& str_cmp(name, "lies") && str_cmp(name, "looking")
				&& str_cmp(name, "pair") && str_cmp(name, "someone")
				&& str_cmp(name, "there") && str_cmp(name, "has")
				&& str_cmp(name, "on") && str_cmp(name, "dropped")) {
				for (i = 0; i < TOP_MATERIAL; i++)
					if (isname(material_names[i], name))
						return (i);
			}
			ptr = one_argument(ptr, name);
		}
	}

	if (isname("bottle", obj->name) || isname("glasses", obj->name) ||
		isname("jar", obj->name) || isname("mirror", obj->name))
		return (MAT_GLASS);

	if (isname("meat", obj->name) || isname("beef", obj->name)) {
		if (isname("raw", obj->name))
			return (MAT_MEAT_RAW);
		else
			return (MAT_MEAT_COOKED);
	}

	if (isname("burger", obj->name) || isname("hamburger", obj->name) ||
		isname("cheeseburger", obj->name))
		return (MAT_MEAT_COOKED);

	if (isname("oaken", obj->name) || isname("oak", obj->name))
		return (MAT_OAK);

	if (isname("tree", obj->name) || isname("branch", obj->name) ||
		isname("board", obj->name) || isname("wooden", obj->name) ||
		isname("barrel", obj->name) || isname("log", obj->name) ||
		isname("logs", obj->name) || isname("stick", obj->name) ||
		isname("pencil", obj->name))
		return (MAT_WOOD);

	if (isname("jean", obj->name) || isname("jeans", obj->name))
		return (MAT_DENIM);

	if (isname("hide", obj->name) || isname("pelt", obj->name) ||
		isname("skins", obj->name) || isname("viscera", obj->name))
		return (MAT_SKIN);

	if (isname("golden", obj->name) || isname("coin", obj->name) ||
		isname("coins", obj->name))
		return (MAT_GOLD);

	if (isname("silvery", obj->name))
		return (MAT_SILVER);

	if (isname("computer", obj->name))
		return (MAT_PLASTIC);

	if (isname("candle", obj->name))
		return (MAT_WAX);

	if (isname("corpse", obj->name) || isname("body", obj->name) ||
		isname("bodies", obj->name) || isname("corpses", obj->name) ||
		isname("carcass", obj->name) || isname("cadaver", obj->name))
		return (MAT_FLESH);

	if (isname("can", obj->name) || isname("bucket", obj->name))
		return (MAT_ALUMINUM);

	if (isname("claw", obj->name) || isname("claws", obj->name) ||
		isname("bones", obj->name) || isname("spine", obj->name) ||
		isname("skull", obj->name) || isname("tusk", obj->name) ||
		isname("tusks", obj->name) || isname("talons", obj->name) ||
		isname("talon", obj->name) || isname("teeth", obj->name) ||
		isname("tooth", obj->name))
		return (MAT_BONE);

	if (isname("jewel", obj->name) || isname("gem", obj->name))
		return (MAT_STONE);

	if (isname("khaki", obj->name) || isname("fabric", obj->name))
		return (MAT_CLOTH);

	if (isname("rock", obj->name) || isname("boulder", obj->name))
		return (MAT_STONE);

	if (isname("brick", obj->name))
		return (MAT_CONCRETE);

	if (isname("adamantite", obj->name))
		return (MAT_ADAMANTIUM);

	if (isname("rusty", obj->name) || isname("rusted", obj->name))
		return (MAT_IRON);

	if (isname("wicker", obj->name))
		return (MAT_RATTAN);

	if (isname("rope", obj->name))
		return (MAT_HEMP);

	if (isname("mug", obj->name))
		return (MAT_PORCELAIN);

	if (isname("tire", obj->name) || isname("tires", obj->name))
		return (MAT_RUBBER);

	if (isname("rug", obj->name))
		return (MAT_CARPET);

	if (GET_OBJ_TYPE(obj) == ITEM_FOOD && (isname("loaf", obj->name) ||
			isname("pastry", obj->name) ||
			isname("muffin", obj->name) ||
			isname("cracker", obj->name) ||
			isname("bisquit", obj->name) || isname("cake", obj->name)))
		return (MAT_BREAD);

	if (GET_OBJ_TYPE(obj) == ITEM_FOOD && (isname("fish", obj->name) ||
			isname("trout", obj->name) ||
			isname("steak", obj->name) ||
			isname("roast", obj->name) ||
			isname("fried", obj->name) ||
			isname("smoked", obj->name) ||
			isname("jerky", obj->name) ||
			isname("sausage", obj->name) || isname("fillet", obj->name)))
		return (MAT_MEAT_COOKED);

	if (IS_ENGINE(obj) || IS_VEHICLE(obj))
		return (MAT_STEEL);

	if (GET_OBJ_TYPE(obj) == ITEM_SCROLL || GET_OBJ_TYPE(obj) == ITEM_NOTE ||
		GET_OBJ_TYPE(obj) == ITEM_CIGARETTE || isname("papers", obj->name))
		return (MAT_PAPER);

	if (GET_OBJ_TYPE(obj) == ITEM_KEY)
		return (MAT_IRON);

	if (GET_OBJ_TYPE(obj) == ITEM_POTION)
		return (MAT_GLASS);

	if (GET_OBJ_TYPE(obj) == ITEM_TOBACCO)
		return (MAT_LEAF);

	if (GET_OBJ_TYPE(obj) == ITEM_WINDOW)
		return (MAT_GLASS);

	if (GET_OBJ_TYPE(obj) == ITEM_TATTOO)
		return (MAT_SKIN);

	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF)
		return (MAT_WOOD);

	if (GET_OBJ_TYPE(obj) == ITEM_SYRINGE) {
		if (isname("disposable", obj->name))
			return (MAT_PLASTIC);
		else
			return (MAT_GLASS);
	}

	if (GET_OBJ_TYPE(obj) == ITEM_WORN)
		return (MAT_CLOTH);

	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
		if (isname("plate", obj->name) || isname("chain", obj->name) ||
			isname("plates", obj->name) || isname("plated", obj->name) ||
			isname("helmet", obj->name) || isname("breastplate", obj->name) ||
			isname("shield", obj->name) || isname("mail", obj->name) ||
			((isname("scale", obj->name) || isname("scales", obj->name)) &&
				obj->short_description &&
				!isname("dragon", obj->short_description) &&
				!isname("dragons", obj->short_description) &&
				!isname("dragon's", obj->short_description) &&
				!isname("tiamat", obj->short_description)))
			return (MAT_STEEL);
		if (isname("boots", obj->name))
			return (MAT_LEATHER);
	}

	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
		if (isname("blade", obj->name) || isname("sword", obj->name) ||
			isname("dagger", obj->name) || isname("knife", obj->name) ||
			isname("axe", obj->name) || isname("longsword", obj->name) ||
			isname("scimitar", obj->name) || isname("halberd", obj->name) ||
			isname("sabre", obj->name) || isname("cutlass", obj->name) ||
			isname("rapier", obj->name) || isname("pike", obj->name) ||
			isname("spear", obj->name))
			return (MAT_STEEL);
		if (isname("club", obj->name))
			return (MAT_WOOD);
		if (isname("whip", obj->name))
			return (MAT_LEATHER);
	}

	if (isname("chain", obj->name) || isname("chains", obj->name))
		return (MAT_METAL);

	if (isname("cloak", obj->name))
		return (MAT_CLOTH);

	if (isname("torch", obj->name))
		return (MAT_WOOD);

	return (MAT_NONE);
}


ACMD(do_objupdate)
{
	return;

	struct obj_data *obj = NULL;

	if (GET_IDNUM(ch) != 1)
		return;

	for (obj = obj_proto; obj; obj = obj->next) {

		// reduce cost/value by half
		GET_OBJ_COST(obj) >>= 1;

		/*
		   if (GET_OBJ_TYPE(obj)==ITEM_LIGHT || GET_OBJ_TYPE(obj)==ITEM_ARMOR ||
		   GET_OBJ_TYPE(obj)==ITEM_CONTAINER) {

		   obj->obj_flags.cost = prototype_obj_value(obj);
		   obj->obj_flags.cost += number(-(obj->obj_flags.cost>>6), 
		   (obj->obj_flags.cost>>6));
		   i = obj->obj_flags.cost % 10;
		   obj->obj_flags.cost -= i;
		   obj->obj_flags.cost_per_day = obj->obj_flags.cost / 50;
		   }
		   obj->obj_flags.material = choose_material(obj);

		   GET_OBJ_MAX_DAM(obj) = set_maxdamage(obj);
		   GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);
		 */
	}
}


ACMD(do_attach)
{
	struct obj_data *obj1 = NULL, *obj2 = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int i;

	skip_spaces(&argument);
	half_chop(argument, arg1, arg2);

	if (!*argument) {
		sprintf(buf, "%stach what %s what?\r\n", subcmd ? "De" : "At",
			subcmd ? "from" : "to");
		send_to_char(buf, ch);
		return;
	}

	if (!(obj1 = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		!(obj1 = get_object_in_equip_all(ch, arg1, ch->equipment, &i))) {
		sprintf(buf, "You don't have %s '%s'.\r\n", AN(arg1), arg1);
		send_to_char(buf, ch);
		return;
	}

	if (!*arg2) {
		sprintf(buf, "%statch $p %s what?", subcmd ? "De" : "At",
			subcmd ? "from" : "to");
		act(buf, FALSE, ch, obj1, 0, TO_CHAR);
		return;
	}

	if (!(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
		!(obj2 = get_object_in_equip_all(ch, arg2, ch->equipment, &i))) {
		sprintf(buf, "You don't have %s '%s'.\r\n", AN(arg2), arg2);
		send_to_char(buf, ch);
		return;
	}

	if (!subcmd) {
		switch (GET_OBJ_TYPE(obj1)) {
		case ITEM_SCUBA_MASK:
		case ITEM_SCUBA_TANK:
			if ((GET_OBJ_TYPE(obj1) == ITEM_SCUBA_MASK &&
					GET_OBJ_TYPE(obj2) != ITEM_SCUBA_TANK) ||
				(GET_OBJ_TYPE(obj2) == ITEM_SCUBA_MASK &&
					GET_OBJ_TYPE(obj1) != ITEM_SCUBA_TANK)) {
				act("You cannot attach $p to $P.", FALSE, ch, obj1, obj2,
					TO_CHAR);
				return;
			}
			if (obj1->aux_obj)
				act("$p is already attached to $P.",
					FALSE, ch, obj1, obj1->aux_obj, TO_CHAR);
			else if (obj2->aux_obj)
				act("$p is already attached to $P.",
					FALSE, ch, obj2, obj2->aux_obj, TO_CHAR);
			else {
				obj1->aux_obj = obj2;
				obj2->aux_obj = obj1;
				act("You attach $p to $P.", FALSE, ch, obj1, obj2, TO_CHAR);
				act("$n attaches $p to $P.", FALSE, ch, obj1, obj2, TO_ROOM);
			}

			break;
		case ITEM_FUSE:
			if (!IS_BOMB(obj2)) {
				act("You cannot attach $p to $P.", FALSE, ch, obj1, obj2,
					TO_CHAR);
			} else if (obj2->contains) {
				if (IS_FUSE(obj2->contains))
					act("$p is already attached to $P.",
						FALSE, ch, obj2->contains, obj2, TO_CHAR);
				else
					act("$P is jammed with $p.",
						FALSE, ch, obj2->contains, obj2, TO_CHAR);
			} else if (FUSE_STATE(obj1)) {
				act("Better not do that -- $p is active!",
					FALSE, ch, obj1, 0, TO_CHAR);
			} else if (!obj1->carried_by) {
				act("You can't attach $p while you are using it.",
					FALSE, ch, obj1, 0, TO_CHAR);
			} else {
				obj_from_char(obj1);
				obj_to_obj(obj1, obj2);
				act("You attach $p to $P.", FALSE, ch, obj1, obj2, TO_CHAR);
				act("$n attaches $p to $P.", FALSE, ch, obj1, obj2, TO_ROOM);
			}
			break;
		default:
			act("You cannot attach $p to $P.", FALSE, ch, obj1, obj2, TO_CHAR);
			break;
		}
	} else {

		if (!obj1->aux_obj || obj2 != obj1->aux_obj) {
			act("$p is not attached to $P.", FALSE, ch, obj1, obj2, TO_CHAR);
			return;
		}

		switch (GET_OBJ_TYPE(obj1)) {
		case ITEM_SCUBA_MASK:
		case ITEM_SCUBA_TANK:
			if (obj1->aux_obj->aux_obj && obj1->aux_obj->aux_obj == obj1)
				obj1->aux_obj->aux_obj = NULL;
			obj1->aux_obj = NULL;

			act("You detach $p from $P.", FALSE, ch, obj1, obj2, TO_CHAR);
			act("$n detaches $p from $P.", FALSE, ch, obj1, obj2, TO_ROOM);
			break;

		default:
			send_to_char("...\n", ch);
			break;
		}
	}
}

ACMD(do_conceal)
{

	struct obj_data *obj = NULL;
	int i;

	skip_spaces(&argument);
	if (!*argument) {
		send_to_char("Conceal what?\r\n", ch);
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
		!(obj = get_object_in_equip_vis(ch, argument, ch->equipment, &i)) &&
		!(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
		sprintf(buf, "You can't find %s '%s' here.\r\n", AN(argument),
			argument);
		send_to_char(buf, ch);
		return;
	}

	act("You conceal $p.", FALSE, ch, obj, 0, TO_CHAR);
	if (obj->in_room)
		act("$n conceals $p.", TRUE, ch, obj, 0, TO_ROOM);
	else
		act("$n conceals something.", TRUE, ch, obj, 0, TO_ROOM);

	WAIT_STATE(ch, 1 RL_SEC);
	if (obj->in_room && !CAN_GET_OBJ(ch, obj))
		return;

	if (CHECK_SKILL(ch, SKILL_CONCEAL) + GET_LEVEL(ch) > number(60, 120)) {
		SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_HIDDEN);
		gain_skill_prof(ch, SKILL_CONCEAL);
	}
}

ACMD(do_sacrifice)
{
	struct obj_data *obj;
	int exp;
	string sbuf;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Sacrifice what object?\r\n", ch);
		return;
	}
	if (!(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
		sprintf(buf, "You can't find any '%s' in the room.\r\n", argument);
		send_to_char(buf, ch);
		return;
	}
	if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && GET_LEVEL(ch) < LVL_GOD) {
		send_to_char("You can't sacrifice that.\r\n", ch);
		return;
	}
	if (GET_LEVEL(ch) < LVL_SPIRIT) {
		if (!junkable(obj)) {
			if (IS_CONTAINER(obj)) {
				string containerName(obj->name);
				sbuf =
					AN(obj->short_description) + string(" ") + containerName +
					" contains, or is, a renamed object.\r\n";
			} else
				sbuf = "You can't sacrifice a renamed object.\r\n";

			send_to_char(sbuf.c_str(), ch);
			return;
		}
	}

	act("$n sacrifices $p.", FALSE, ch, obj, 0, TO_ROOM);
	act("You sacrifice $p.", FALSE, ch, obj, 0, TO_CHAR);
	if (IS_CORPSE(obj))
		exp = (GET_LEVEL(ch) << 1);
	else
		exp = MAX(0, GET_OBJ_COST(obj) >> 10);

	if (exp) {
		send_to_char("You sense your deity's favor upon you.\r\n", ch);
		gain_exp(ch, exp);
	}
	extract_obj(obj);
}

ACMD(do_empty)
{
	struct obj_data *obj = NULL;	// the object that will be emptied 
	struct obj_data *next_obj = NULL;
	struct obj_data *o = NULL;
	struct obj_data *container = NULL;
	struct char_data *dummy = NULL;
	int bits;
	int bits2;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int objs_moved = 0;

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char("What do you want to empty?\r\n", ch);
		return;
	}

	if (!(bits = generic_find(arg1, FIND_OBJ_INV, ch, &dummy, &obj))) {
		sprintf(buf, "You can't find any %s to empty\r\n.", arg1);
		send_to_char(buf, ch);
		return;
	}

	if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
		send_to_char("You can't empty that.\r\n", ch);
		return;
	}
	if (!IS_CORPSE(obj) && IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE) &&
		IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
		send_to_char("It seems to be closed.\r\n", ch);
		return;

	}

	if (*arg2 && obj) {

		if (!(bits2 =
				generic_find(arg2,
					FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy,
					&container))) {
			sprintf(buf, "Empty %s into what?\r\n", obj->short_description);
			send_to_char(buf, ch);
			return;
		} else {
			empty_to_obj(obj, container, ch);
			return;
		}
	}


	if (obj->contains) {
		for (next_obj = obj->contains; next_obj; next_obj = o) {
			if (next_obj->in_obj) {
				o = next_obj->next_content;
				if (!(IS_OBJ_STAT(next_obj, ITEM_NODROP)) &&
					(IS_SET(GET_OBJ_WEAR(next_obj), ITEM_WEAR_TAKE))) {
					obj_from_obj(next_obj);
					obj_to_room(next_obj, ch->in_room);
					objs_moved++;
				}
			} else {
				o = NULL;
			}
		}
	}
	if (objs_moved) {
		act("$n empties the contents of $p.", FALSE, ch, obj, 0, TO_ROOM);
		act("You carefully empty the contents of $p.", FALSE, ch, obj, 0,
			TO_CHAR);
	} else {
		sprintf(buf, "%s is already empty idiot!\r\n", obj->short_description);
		send_to_char(buf, ch);
	}
}



int
empty_to_obj(struct obj_data *obj, struct obj_data *container,
	struct char_data *ch)
{

	//
	// To clarify, obj is the object that contains the equipment originally and container is
	// the object that the eq is being transferred into.
	//

	struct obj_data *o = NULL;
	struct obj_data *next_obj = NULL;
	bool can_fit = true;
	int objs_moved = 0;

	if (!(obj) || !(container) || !(ch)) {
		sprintf(buf, "Null value passed into empty_to_obj.\r\n");
		slog(buf);
		return 0;
	}

	if (obj == container) {
		send_to_char("Why would you want to empty something into itself?\r\n",
			ch);
		return 0;
	}

	if (GET_OBJ_TYPE(container) != ITEM_CONTAINER) {
		send_to_char("You can't put anything in that.\r\n", ch);
		return 0;
	}

	if (!obj->contains) {
		send_to_char("There is nothing to empty!\r\n", ch);
		return 0;
	}

	if (obj->contains) {
		for (next_obj = obj->contains; next_obj; next_obj = o) {
			if (next_obj->in_obj && next_obj) {
				o = next_obj->next_content;
				if (!(IS_OBJ_STAT(next_obj, ITEM_NODROP)) &&
					(IS_SET(GET_OBJ_WEAR(next_obj), ITEM_WEAR_TAKE))) {
					if (container->getWeight() + next_obj->getWeight() >
						GET_OBJ_VAL(container, 0)) {
						can_fit = false;
					}
					if (can_fit) {
						obj_from_obj(next_obj);
						obj_to_obj(next_obj, container);
						objs_moved++;
					}
					can_fit = true;
				}
			}
		}
		if (objs_moved) {
			sprintf(buf, "$n carefully empties the contents of $p into %s.",
				container->short_description);
			act(buf, FALSE, ch, obj, 0, TO_ROOM);
			sprintf(buf, "You carefully empty the contents of $p into %s.",
				container->short_description);
			act(buf, FALSE, ch, obj, 0, TO_CHAR);
		} else {
			act("There's nothing in it!", FALSE, ch, obj, 0, TO_CHAR);
			return 0;
		}

	}

	return 1;
}
