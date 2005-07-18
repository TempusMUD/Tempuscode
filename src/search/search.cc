//
// File: search.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


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

/* extern variables */
extern struct room_data *world;
extern struct obj_data *object_list;

int search_nomessage = 0;

void print_search_data_to_buf(struct Creature *ch,
	struct room_data *room, struct special_search_data *cur_search, char *buf);
int general_search(struct Creature *ch, struct special_search_data *srch,
	int mode);
int clan_house_can_enter(struct Creature *ch, struct room_data *room);
int room_tele_ok(Creature *ch, struct room_data *room);
bool process_load_param( Creature *ch );

int
search_trans_character(Creature * ch,
	special_search_data * srch,
	room_data * targ_room, obj_data * obj, Creature * mob)
{
	room_data *was_in;

	if (!House_can_enter(ch, targ_room->number) ||
		!clan_house_can_enter(ch, targ_room) ||
		(ROOM_FLAGGED(targ_room, ROOM_GODROOM) &&
			!Security::isMember(ch, "WizardFull")))
		return 0;

	was_in = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, targ_room);
	ch->in_room->zone->enter_count++;
    if (!SRCH_FLAGGED(srch, SRCH_NO_LOOK))
	    look_at_room(ch, ch->in_room, 0);

	if (ch->followers) {
		struct follow_type *k, *next;

		for (k = ch->followers; k; k = next) {
			next = k->next;
			if (was_in == k->follower->in_room &&
					GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
					!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
					can_see_creature(k->follower, ch))
				perform_goto(k->follower, ch->in_room, true);
		}
	}
					

	if (GET_LEVEL(ch) < LVL_ETERNAL && !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
		SET_BIT(srch->flags, SRCH_TRIPPED);

	if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH)) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			log_death_trap(ch);
			death_cry(ch);
			ch->die();
			//Event::Queue(new DeathEvent(0, ch, false));
			return 2;
		} else {
			mudlog(LVL_GOD, NRM, true,
				"(GC) %s trans-searched into deathtrap %d.",
				GET_NAME(ch), ch->in_room->number);
		}
	}
	// can't double trans, to prevent loops
	for (srch = ch->in_room->search; srch; srch = srch->next) {
		if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER) && SRCH_OK(ch, srch) &&
			srch->command != SEARCH_COM_TRANSPORT)
			if (general_search(ch, srch, 0) == 2)
				return 2;
	}

	return 1;
}

inline void
SRCH_LOG(Creature *ch, special_search_data *srch)
{
	if (!ZONE_FLAGGED( ch->in_room->zone, ZONE_SEARCH_APPROVED)
			&& GET_LEVEL( ch ) < LVL_GOD )
		slog("SRCH: %s at %d: %c %d %d %d.",       
			GET_NAME( ch ), ch->in_room->number,         
			*search_commands[( int )srch->command],      
			srch->arg[0], srch->arg[1], srch->arg[2]);
}

#define SRCH_DOOR ( targ_room->dir_option[srch->arg[1]]->exit_info )
#define SRCH_REV_DOOR ( other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info )

//
//
//

int
general_search(struct Creature *ch, struct special_search_data *srch,
	int mode)
{
	struct obj_data *obj = NULL;
	static struct Creature *mob = NULL;
	struct room_data *rm = ch->in_room, *other_rm = NULL;
	struct room_data *targ_room = NULL;
	int add = 0, killed = 0, found = 0;
	int bits = 0, i, j;
	int retval = 0;
	int maxlevel = 0;

	if (!mode) {
		obj = NULL;
		mob = NULL;
	}

	if (SRCH_FLAGGED(srch, SRCH_NEWBIE_ONLY) &&
		GET_LEVEL(ch) > 6 && !NOHASS(ch)) {
		send_to_char(ch, 
			"This can only be done here by players less than level 7.\r\n");
		return 1;
	}
	if (SRCH_FLAGGED(srch, SRCH_REMORT_ONLY) && !IS_REMORT(ch) && !NOHASS(ch)) {
		send_to_char(ch, "This can only be done here by remorts.\r\n");
		return 1;
	}

	if (srch->fail_chance && number(0, 100) < srch->fail_chance) {
        if (SRCH_FLAGGED(srch, SRCH_FAIL_TRIP))
		    SET_BIT(srch->flags, SRCH_TRIPPED);
		return 1;
    }

	switch (srch->command) {
	case (SEARCH_COM_OBJECT):
		if (!(obj = real_object_proto(srch->arg[0]))) {
			zerrlog(rm->zone, "search in room %d, object %d nonexistent.",
				rm->number, srch->arg[0]);
			return 0;
		}
		if ((targ_room = real_room(srch->arg[1])) == NULL) {
			zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
				rm->number, srch->arg[1]);
			return 0;
		}
		if (obj->shared->number - obj->shared->house_count >= srch->arg[2]) {
			return 0;
		}
		if (!(obj = read_object(srch->arg[0]))) {
			zerrlog(rm->zone, "search cannot load object #%d, room %d.",
				srch->arg[0], ch->in_room->number);
			return 0;
		}
		obj->creation_method = CREATED_SEARCH;
		obj->creator = ch->in_room->number;
		if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED))
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
		obj_to_room(obj, targ_room);
		if (srch->to_remote)
			act(srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM);

		//    SRCH_LOG( ch, srch );

		break;

	case SEARCH_COM_MOBILE:
		if (!(mob = real_mobile_proto(srch->arg[0]))) {
			zerrlog(rm->zone, "search in room %d, mobile %d nonexistent.",
				rm->number, srch->arg[0]);
			return 0;
		}
		if ((targ_room = real_room(srch->arg[1])) == NULL) {
			zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
				rm->number, srch->arg[1]);
			return 0;
		}
		if (mob->mob_specials.shared->number >= srch->arg[2]) {
			return 0;
		}
		if (!(mob = read_mobile(srch->arg[0]))) {
			zerrlog(rm->zone, "search cannot load mobile #%d, room %d.",
				srch->arg[0], ch->in_room->number);
			return 0;
		}
		char_to_room(mob, targ_room,false);
		if( process_load_param( mob ) ) {
			// Mobile Died in load_param
		} else {
			if (srch->to_remote)
				act(srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM);
			if (GET_MOB_PROGOBJ(mob))
				trigger_prog_load(mob);
		}


		//    SRCH_LOG( ch, srch );
		break;

	case SEARCH_COM_EQUIP:
		if (!(obj = real_object_proto(srch->arg[1]))) {
			zerrlog(rm->zone, "search in room %d, equip object %d nonexistent.",
				rm->number, srch->arg[1]);
			return 0;
		}
		if (srch->arg[2] < 0 || srch->arg[2] >= NUM_WEARS) {
			zerrlog(rm->zone, "search trying to equip obj %d to badpos.",
				obj->shared->vnum);
			return 0;
		}
		if (!(obj = read_object(srch->arg[1]))) {
			zerrlog(rm->zone, "search cannot load equip object #%d, room %d.",
				srch->arg[0], ch->in_room->number);
			return 0;
		}
		obj->creation_method = CREATED_SEARCH;
		obj->creator = ch->in_room->number;
		if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED))
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);

//    SRCH_LOG( ch, srch );
		if (IS_IMPLANT(obj)) {
			if (ch->implants[srch->arg[2]])
				obj_to_char(obj, ch);
			else if (equip_char(ch, obj, srch->arg[2], MODE_IMPLANT))
				return 2;
		} else {
			if (ch->equipment[srch->arg[2]])
				obj_to_char(obj, ch);
			else if (equip_char(ch, obj, srch->arg[2], MODE_EQ))
				return 2;
		}
		break;

	case SEARCH_COM_GIVE:
		if (!(obj = real_object_proto(srch->arg[1])))
			return 0;

		if (obj->shared->number - obj->shared->house_count >= srch->arg[2])
			return 0;

		if (!(obj = read_object(srch->arg[1])))
			return 0;

		obj->creation_method = CREATED_SEARCH;
		obj->creator = ch->in_room->number;
		if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED))
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
		obj_to_char(obj, ch);

//    SRCH_LOG( ch, srch );

		break;

	case SEARCH_COM_TRANSPORT:
		if ((targ_room = real_room(srch->arg[0])) == NULL) {
			zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
				rm->number, srch->arg[0]);
			return 0;
		}
		if (srch->arg[1] == -1 || srch->arg[1] == 0) {
			if (!House_can_enter(ch, targ_room->number)
				|| !clan_house_can_enter(ch, targ_room)
				|| (ROOM_FLAGGED(targ_room, ROOM_GODROOM)
					&& !Security::isMember(ch, "WizardFull"))) {
				return 0;
			}

			if (srch->to_room)
				act(srch->to_room, FALSE, ch, obj, mob, TO_ROOM);
			if (srch->to_vict)
				act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
			else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
				send_to_char(ch, "Okay.\r\n");

			SRCH_LOG(ch, srch);	// don't log trans searches for now

			char_from_room(ch);
			char_to_room(ch, targ_room);
			ch->in_room->zone->enter_count++;
            if (!SRCH_FLAGGED(srch, SRCH_NO_LOOK))
			    look_at_room(ch, ch->in_room, 0);
					
			if (srch->to_remote)
				act(srch->to_remote, FALSE, ch, obj, mob, TO_ROOM);

			if (GET_LEVEL(ch) < LVL_ETERNAL
				&& !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
				SET_BIT(srch->flags, SRCH_TRIPPED);

			if (ch->followers) {
				struct follow_type *k, *next;

				for (k = ch->followers; k; k = next) {
					next = k->next;
					if (rm == k->follower->in_room &&
							GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
							!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
							can_see_creature(k->follower, ch))
						perform_goto(k->follower, targ_room, true);
				}
			}

			if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH)) {
				if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
					log_death_trap(ch);
					death_cry(ch);
					ch->die();
					//Event::Queue(new DeathEvent(0, ch, false));
					return 2;
				} else {
					mudlog(LVL_GOD, NRM, true,
						"(GC) %s trans-searched into deathtrap %d.",
						GET_NAME(ch), ch->in_room->number);
				}
			}
			// can't double trans, to prevent loops
			for (srch = ch->in_room->search; srch; srch = srch->next) {
				if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER) && SRCH_OK(ch, srch) &&
					srch->command != SEARCH_COM_TRANSPORT)
					if (general_search(ch, srch, 0) == 2)
						return 2;
			}

			return 1;
		} else if (srch->arg[1] == 1) {

			if (srch->to_vict)
				act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
			else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
				send_to_char(ch, "Okay.\r\n");

			if (srch->to_remote && targ_room->people) {
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_ROOM);
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_CHAR);
			}

			int rc = 1;
			room_data *src_room = ch->in_room;
			rc = search_trans_character(ch, srch, targ_room, obj, mob);

			CreatureList::iterator it = src_room->people.begin();
			for (; it != src_room->people.end(); ++it) {
				mob = *it;
				if (srch->to_room)
					act(srch->to_room, FALSE, ch, obj, mob, TO_VICT);
				if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob))
					continue;
				int r = search_trans_character(mob, srch, targ_room, obj, mob);
				if (rc != 2)
					rc = r;
			}
			return rc;
		}
	case SEARCH_COM_DOOR:
		/************  Targ Room nonexistent ************/
		if ((targ_room = real_room(srch->arg[0])) == NULL) {
			zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
				rm->number, srch->arg[0]);
			return 0;
		}
		if (srch->arg[1] >= NUM_DIRS || !targ_room->dir_option[srch->arg[1]]) {
			zerrlog(rm->zone, "search in room %d, direction nonexistent.",
				rm->number);
			return 0;
		}
		switch (srch->arg[2]) {
		case 0:
			add = 0;
			bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
			break;
		case 1:
			add = 1;
			bits = EX_CLOSED;
			break;
		case 2:  /** close and lock door **/
			add = 1;
			bits = EX_CLOSED | EX_LOCKED;
			break;
		case 3:  /** remove hidden flag **/
			add = 0;
			bits = EX_HIDDEN;
			break;
		case 4:				// close, lock, and hide the door
			add = 1;
			bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
			break;

		default:
			zerrlog(rm->zone, "search bunk doorcmd %d in rm %d.",
				srch->arg[2], rm->number);
			return 0;
		}
		if (!(other_rm = targ_room->dir_option[srch->arg[1]]->to_room) ||
			!other_rm->dir_option[rev_dir[srch->arg[1]]] ||
			other_rm->dir_option[rev_dir[srch->arg[1]]]->to_room != targ_room)
			other_rm = NULL;

		if (add) {
			for (found = 0, i = 0; i < 32; i++) {
				if (IS_SET(bits, (j = (1 << i))) &&
					(!IS_SET(SRCH_DOOR, j) ||
						(other_rm && !IS_SET(SRCH_REV_DOOR, j)))) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL))
					send_to_char(ch, "You can't do that right now.\r\n");
				return 1;
			}
			SET_BIT(targ_room->dir_option[srch->arg[1]]->exit_info, bits);
			if (other_rm)
				SET_BIT(other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info,
					bits);
		} else {
			for (found = 0, i = 0; i < 32; i++) {
				if (IS_SET(bits, (j = (1 << i))) &&
					(IS_SET(SRCH_DOOR, j) ||
						(other_rm && IS_SET(SRCH_REV_DOOR, j)))) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL))
					send_to_char(ch, "You can't do that right now.\r\n");
				return 1;
			}

			REMOVE_BIT(targ_room->dir_option[srch->arg[1]]->exit_info, bits);
			if (other_rm)
				REMOVE_BIT(other_rm->dir_option[rev_dir[srch->arg[1]]]->
					exit_info, bits);
		}

		//    SRCH_LOG( ch, srch );

		if (srch->to_remote) {
			if (targ_room != ch->in_room && targ_room->people.size() > 0) {
				act(srch->to_remote, FALSE, targ_room->people, obj, ch,
					TO_NOTVICT);
			}
			if (other_rm && other_rm != ch->in_room && other_rm->people) {
				act(srch->to_remote, FALSE, other_rm->people, obj, ch,
					TO_NOTVICT);
			}
		}
		break;

	case SEARCH_COM_SPELL:

		targ_room = real_room(srch->arg[1]);

		if (srch->arg[0] < 0 || srch->arg[0] > LVL_GRIMP) {
			zerrlog(rm->zone, "search in room %d, spell level %d invalid.",
				rm->number, srch->arg[0]);
			return 0;
		}

		if (srch->arg[2] <= 0 || srch->arg[2] > TOP_NPC_SPELL) {
			zerrlog(rm->zone, "search in room %d, spell number %d invalid.",
				rm->number, srch->arg[2]);
			return 0;
		}

		if (srch->to_room)
			act(srch->to_room, FALSE, ch, obj, mob, TO_ROOM);
		if (srch->to_vict)
			act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
		else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
			send_to_char(ch, "Okay.\r\n");

		if (GET_LEVEL(ch) < LVL_ETERNAL
			&& !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
			SET_BIT(srch->flags, SRCH_TRIPPED);

		// SRCH_LOG( ch, srch );

		// turn messaging off for damage(  ) call
		if (SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
			search_nomessage = 1;

		if (targ_room) {

			if (srch->to_remote && ch->in_room != targ_room
				&& targ_room->people.size() > 0) {
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_ROOM);
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_CHAR);
			}
			CreatureList::iterator it = targ_room->people.begin();
			for (; it != targ_room->people.end(); ++it) {
				mob = *it;

				if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob))
					continue;
				if (affected_by_spell(ch, srch->arg[2]))
					continue;

				if (mob == ch) {
					call_magic(ch, ch, 0, NULL, srch->arg[2], srch->arg[0],
						(SPELL_IS_MAGIC(srch->arg[2]) ||
							SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
						(SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
						(SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC :
						CAST_ROD, &retval);
					if (IS_SET(retval, DAM_ATTACKER_KILLED))
						break;
				} else {
					call_magic(ch, mob, 0, NULL, srch->arg[2], srch->arg[0],
						(SPELL_IS_MAGIC(srch->arg[2]) ||
							SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
						(SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
						(SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC :
						CAST_ROD);
				}
			}
		} else if (!targ_room && !affected_by_spell(ch, srch->arg[2]))
			call_magic(ch, ch, 0, NULL, srch->arg[2], srch->arg[0],
				(SPELL_IS_MAGIC(srch->arg[2]) ||
					SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
				(SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
				(SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC : CAST_ROD);

		search_nomessage = 0;

		return 2;
		/*
		   if ( !IS_SET( srch->flags, SRCH_IGNORE ) )
		   return 1;
		   else
		   return 0;
		 */

		break;

	case SEARCH_COM_DAMAGE: /** damage a room or person */

		killed = 0;
		targ_room = real_room(srch->arg[1]);

		if (srch->arg[0] < 0 || srch->arg[0] > 500) {
			zerrlog(rm->zone, "search in room %d, damage level %d invalid.",
				rm->number, srch->arg[0]);
			return 0;
		}

		if (srch->arg[2] <= 0 || srch->arg[2] >= TOP_NPC_SPELL) {
			zerrlog(rm->zone, "search in room %d, damage number %d invalid.",
				rm->number, srch->arg[2]);
			return 0;
		}

		if (srch->to_room)
			act(srch->to_room, FALSE, ch, obj, mob, TO_ROOM);
		if (srch->to_vict)
			act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
		else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
			send_to_char(ch, "Okay.\r\n");

		if (GET_LEVEL(ch) < LVL_ETERNAL
			&& !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
			SET_BIT(srch->flags, SRCH_TRIPPED);

		// SRCH_LOG( ch, srch );

		// turn messaging off for damage(  ) call
		if (SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
			search_nomessage = 1;

		if (targ_room) {

			if (srch->to_remote && ch->in_room != targ_room
				&& targ_room->people.size() > 0) {
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_ROOM);
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_CHAR);
			}
			CreatureList::iterator it = targ_room->people.begin();
			for (mob = NULL; it != targ_room->people.end(); ++it) {
				mob = *it;

				if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob))
					continue;

				if (mob == ch)
					killed = damage(NULL, mob,
						dice(srch->arg[0], srch->arg[0]), srch->arg[2],
						WEAR_RANDOM);
				else
					damage(NULL, mob,
						dice(srch->arg[0], srch->arg[0]), srch->arg[2],
						WEAR_RANDOM);

			}
		} else
			killed = damage(NULL, ch,
				dice(srch->arg[0], srch->arg[0]), srch->arg[2], WEAR_RANDOM);

		// turn messaging back on
		search_nomessage = 0;

		if (killed)
			return 2;

		if (!IS_SET(srch->flags, SRCH_IGNORE))
			return 1;
		else
			return 0;

		break;

	case SEARCH_COM_SPAWN:{	/* spawn a room full of mobs to another room */
			if (!(other_rm = real_room(srch->arg[0])))
				return 0;
			if (!(targ_room = real_room(srch->arg[1])))
				targ_room = ch->in_room;

			if (srch->to_room)
				act(srch->to_room, FALSE, ch, obj, mob, TO_ROOM);
			if (srch->to_remote && targ_room->people.size() > 0) {
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_ROOM);
				act(srch->to_remote, FALSE, targ_room->people, obj, mob,
					TO_CHAR);
			}
			if (srch->to_vict)
				act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
			else if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL))
				send_to_char(ch, "Okay.\r\n");

			if (GET_LEVEL(ch) < LVL_ETERNAL
				&& !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
				SET_BIT(srch->flags, SRCH_TRIPPED);

			CreatureList::iterator it = other_rm->people.begin();
			for (; it != other_rm->people.end(); ++it) {
				mob = *it;
				if (!IS_NPC(mob))
					continue;
				if (other_rm != targ_room) {
					act("$n suddenly disappears.", TRUE, mob, 0, 0, TO_ROOM);
					char_from_room(mob,false);
					char_to_room(mob, targ_room,false);
					act("$n suddenly appears.", TRUE, mob, 0, 0, TO_ROOM);
				}
				if (srch->arg[2])
					mob->startHunting(ch);
			}

			// SRCH_LOG( ch, srch );

			if (!IS_SET(srch->flags, SRCH_IGNORE))
				return 1;
			else
				return 0;
			break;

		}
	case SEARCH_COM_LOADROOM:
		maxlevel = srch->arg[1];
		targ_room = real_room(srch->arg[0]);


		if ((GET_LEVEL(ch) < maxlevel) && (targ_room)) {

			if (room_tele_ok(ch, targ_room)) {
				GET_LOADROOM(ch) = srch->arg[0];
			}

		}

		else {
			return 0;
		}

		break;

	case SEARCH_COM_NONE:		/* simple echo search */

		if ((targ_room = real_room(srch->arg[1])) &&
			srch->to_remote && ch->in_room != targ_room
			&& targ_room->people.size() > 0) {
			act(srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM);
			act(srch->to_remote, FALSE, targ_room->people, obj, mob, TO_CHAR);
		}

		break;

	default:
		slog("Unknown rsearch command in do_search(  )..room %d.",
			rm->number);

	}

	if (srch->to_room)
		act(srch->to_room, FALSE, ch, obj, mob, TO_ROOM);
	if (srch->to_vict)
		act(srch->to_vict, FALSE, ch, obj, mob, TO_CHAR);
	else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE))
		send_to_char(ch, "Okay.\r\n");

	if (GET_LEVEL(ch) < LVL_ETERNAL && !SRCH_FLAGGED(srch, SRCH_REPEATABLE))
		SET_BIT(srch->flags, SRCH_TRIPPED);

	if (!IS_SET(srch->flags, SRCH_IGNORE))
		return 1;
	else
		return 0;
}

void
show_searches(struct Creature *ch, char *value, char *argument)
{

	struct special_search_data *srch = NULL;
	struct room_data *room = NULL;
	struct zone_data *zone = NULL, *zn = NULL;
	int count = 0, srch_type = -1;
	char outbuf[MAX_STRING_LENGTH], arg1[MAX_INPUT_LENGTH];
	bool overflow = 0, found = 0;

	strcpy(argument, strcat(value, argument));
	argument = one_argument(argument, arg1);
	while ((*arg1)) {
		if (is_abbrev(arg1, "zone")) {
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, "No zone specified.\r\n");
				return;
			}
			if (!(zn = real_zone(atoi(arg1)))) {
				send_to_char(ch, "No such zone ( %s ).\r\n", arg1);
				return;
			}
		}
		if (is_abbrev(arg1, "type")) {
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, "No type specified.\r\n");
				return;
			}
			if ((srch_type = search_block(arg1, search_commands, FALSE)) < 0) {
				send_to_char(ch, "No such search type ( %s ).\r\n", arg1);
				return;
			}
		}
		argument = one_argument(argument, arg1);
	}

	strcpy(outbuf, "Searches:\r\n");

	for (zone = zone_table; zone && !overflow; zone = zone->next) {

		if (zn) {
			if (zone == zn->next)
				break;
			if (zn != zone)
				continue;
		}

		for (room = zone->world, found = FALSE; room && !overflow;
			found = FALSE, room = room->next) {

			for (srch = room->search, count = 0; srch && !overflow;
				srch = srch->next, count++) {

				if (srch_type >= 0 && srch_type != srch->command)
					continue;

				if (!found) {
					sprintf(buf, "Room [%s%5d%s]:\n", CCCYN(ch, C_NRM),
						room->number, CCNRM(ch, C_NRM));
					strcat(outbuf, buf);
					found = TRUE;
				}

				print_search_data_to_buf(ch, room, srch, buf);

				if (strlen(outbuf) + strlen(buf) > MAX_STRING_LENGTH - 128) {
					overflow = 1;
					strcat(outbuf, "**OVERFLOW**\r\n");
					break;
				}
				strcat(outbuf, buf);
			}
		}
	}
	page_string(ch->desc, outbuf);
}

int
triggers_search(struct Creature *ch, int cmd, char *arg,
	struct special_search_data *srch)
{

	char *cur_arg, *next_arg;

	skip_spaces(&arg);

	if (srch->keywords) {
		if (SRCH_FLAGGED(srch, SRCH_CLANPASSWD | SRCH_NOABBREV)) {
			if (!isname_exact(arg, srch->keywords)) {
				return 0;
			}
		} else if (!isname(arg, srch->keywords)) {
			return 0;
		}
	}

	if (IS_SET(srch->flags, SRCH_TRIPPED))
		return 0;

	if (GET_LEVEL(ch) < LVL_IMMORT && !SRCH_OK(ch, srch))
		return 0;

	if (!srch->command_keys)
		return 1;
	next_arg = srch->command_keys;
	cur_arg= tmp_getword(&next_arg);

	while (*cur_arg) {
		if (IS_SET(srch->flags, SRCH_MATCH_ALL)) {
			if (!CMD_IS(cur_arg))
				return 0;
		} else {
			if (CMD_IS(cur_arg))
				return 1;
		}
		cur_arg = tmp_getword(&next_arg);
	}

	return IS_SET(srch->flags, SRCH_MATCH_ALL) ? 1:0;
}

#undef __search_c__
