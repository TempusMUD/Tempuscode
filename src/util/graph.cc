/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: graph.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
// 

#define TRACK_THROUGH_DOORS
#define TRACK_THROUGH_DTS

/* You can define or not define TRACK_THOUGH_DOORS, above, depending on
   whether or not you want track to find paths which lead through closed
   or hidden doors.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "vehicle.h"
#include "fight.h"
#include "char_class.h"

/* Externals */
extern struct room_data *world;

int has_key(struct char_data *ch, obj_num key);

ACMD(do_say);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_fly);
ACMD(do_bash);
ACMD(do_cast);

struct bfs_queue_struct {
    struct room_data *room;
    char dir;
    struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;
static unsigned char find_first_step_index = 0;

/* Utility macros */
#define MARK(room) ( room->find_first_step_index = find_first_step_index )
#define UNMARK(room) ( room->find_first_step_index = 0 )
#define IS_MARKED(room) ( room->find_first_step_index == find_first_step_index )

#define TOROOM(x, y) ((x)->dir_option[(y)]->to_room)
#define IS_CLOSED(x, y) (IS_SET((x)->dir_option[(y)]->exit_info, EX_CLOSED))

#define MODE_NORM 0
#define MODE_GOD  1
#define MODE_PSI  2

#ifdef TRACK_THROUGH_DOORS
#define VALID_EDGE(x,y,mode) ( ( x )->dir_option[ ( y ) ] &&                         \
			      TOROOM( x, y )  &&                                     \
			      !IS_MARKED( TOROOM( x, y ) ) &&                        \
			      ( mode != MODE_PSI ||                                  \
			       !ROOM_FLAGGED( TOROOM( x,y ), ROOM_NOPSIONICS ) ) &&  \
			      ( !ROOM_FLAGGED( TOROOM( x, y ),                       \
					     ROOM_NOTRACK | ROOM_DEATH ) ||          \
			       mode == MODE_GOD ) )

#else
#define VALID_EDGE(x,y,mode) ((x)->dir_option[(y)] &&              \
			      TOROOM(x, y) &&                      \
			      !IS_MARKED(TOROOM(x, y)) &&          \
			      !IS_CLOSED(x, y) &&		   \
			      (mode != MODE_PSI ||                 \
			       !ROOM_FLAGGED(TOROOM(x,y),ROOM_NOPSIONICS)) &&\
			      (mode == MODE_GOD ||                 \
			       !ROOM_FLAGGED(TOROOM(x, y),        \
					     ROOM_NOTRACK | ROOM_DEATH)))

#endif

inline void bfs_enqueue(struct room_data *room, char dir) {

    struct bfs_queue_struct *curr;

    CREATE(curr, struct bfs_queue_struct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = 0;

    if (queue_tail) {
	queue_tail->next = curr;
	queue_tail = curr;
    } else
	queue_head = queue_tail = curr;
}


inline void bfs_dequeue(void) {

    struct bfs_queue_struct *curr;

    curr = queue_head;

    if (!(queue_head = queue_head->next))
	queue_tail = 0;
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    free(curr);
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}


inline void bfs_clear_queue(void) {

    while (queue_head)
	bfs_dequeue();
}


/* find_first_step: given a source room and a target room, find the first
   step on the shortest path from the source to the target.

   Intended usage: in mobile_activity, give a mob a dir to go if they're
   tracking another mob or a PC.  Or, a 'track' skill for PCs.

   mode: TRUE -> go thru DT's, doorz, !track roomz 
*/


int 
find_first_step( struct room_data *src, struct room_data *target, byte mode ) {
    int curr_dir;
    struct room_data *curr_room;
    struct zone_data *zone;
  
    if (src == NULL && target == NULL) {
	slog("Illegal value passed to find_first_step (graph.c)");
	return BFS_ERROR;
    }
    if (src == target)
	return BFS_ALREADY_THERE;

    // increment the static index
    ++find_first_step_index;

    // check if find_first_step_index has rolled over to zero
    if ( ! find_first_step_index ) {
	// put index at 1;
	++find_first_step_index;
	
	// reset all rooms' indices to zero
	
	for (zone = zone_table; zone; zone = zone->next)
	    for (curr_room = zone->world; curr_room; curr_room = curr_room->next)
		UNMARK(curr_room);
    }
    
    MARK(src);

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	if (VALID_EDGE(src, curr_dir, mode)) {
	    MARK(TOROOM(src, curr_dir));
	    bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
	}
    /* now, do the char_classic BFS. */
    while (queue_head) {
	if (queue_head->room == target) {
	    curr_dir = queue_head->dir;
	    bfs_clear_queue();
	    return curr_dir;
	} else {
	    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
		if (VALID_EDGE(queue_head->room, curr_dir, mode)) {
		    MARK(TOROOM(queue_head->room, curr_dir));
		    bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
		}
	    bfs_dequeue();
	}
    }

    return BFS_NO_PATH;
}

int 
find_distance(struct room_data *start, struct room_data *location)
{
    int dir = -1, steps = 0;
    struct room_data *tmp = start;

    while (tmp && (dir=find_first_step(tmp, location, 1)) >= 0 && steps < 600) {
	tmp = ABS_EXIT(tmp, dir)->to_room;
	steps++;
    }

    if (location != tmp || steps >= 600)
	return (-1);

    return (steps);
}


/************************************************************************
*  Functions and Commands which use the above fns		        *
************************************************************************/

ACMD(do_track)
{
    struct char_data *vict;
    int dir, num, bonus = 0;
    byte count = 0;

    if (!GET_SKILL(ch, SKILL_TRACK)) {
	send_to_char("You have no idea how.\r\n", ch);
	return;
    }
    one_argument(argument, arg);
    if (!*arg) {
	send_to_char("Whom are you trying to track?\r\n", ch);
	return;
    }
    if ((vict = get_char_vis(ch, arg))) {
        if (!(CAN_SEE(ch, vict) && (GET_LEVEL(vict) < LVL_AMBASSADOR ||
                        GET_LEVEL(ch) > GET_LEVEL(vict))))  {
            send_to_char("No-one around by that name.\r\n", ch);
            return;
        }
    } else {
        send_to_char("No-one around by that name.\r\n", ch);
        return;
    }
    if (vict->in_room == ch->in_room) {
        send_to_char("You're already in the same room!\r\n", ch);
        return;
    }
    if (IS_AFFECTED(vict, AFF_NOTRACK) || (affected_by_spell(vict, SKILL_ELUSION) && !(IS_NPC(ch) && MOB_FLAGGED(ch,MOB_SPIRIT_TRACKER)) &&
	 number(0, CHECK_SKILL(ch, SKILL_TRACK)) > (number(0, CHECK_SKILL(vict, SKILL_ELUSION)) + (IS_RANGER(vict) && SECT_TYPE(vict->in_room) == SECT_FOREST) ? 20 : 0))) {
        send_to_char("You sense no trail.\r\n", ch);
	if (GET_LEVEL(ch) < LVL_IMPL)
	    return;
    }
    if (GET_LEVEL(ch) < LVL_AMBASSADOR && ch->in_room->isOpenAir() ) {
        send_to_char("Track through the open air?\r\n", ch);
        return;
    }
    if (GET_LEVEL(ch) < LVL_AMBASSADOR && (SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
					   SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)) {
        send_to_char("You find it difficult to follow the ripples of the water.\r\n", ch);
        return;
    }
    dir = find_first_step(ch->in_room, vict->in_room, 
			  GET_LEVEL(ch) > LVL_TIMEGOD ? 1 : 0);

    switch (dir) {
    case BFS_ERROR:
        send_to_char("Hmm.. something seems to be wrong.\r\n", ch);
        break;
    case BFS_ALREADY_THERE:
        send_to_char("You're already in the same room!!\r\n", ch);
        break;
    case BFS_NO_PATH:
        sprintf(buf, "You can't sense a trail to %s from here.\r\n",
            HMHR(vict));
        send_to_char(buf, ch);
        break;
    default:
	num = number(0, 101);	/* 101% is a complete failure */
	if (GET_CLASS(ch) == CLASS_RANGER) {
	    if ((ch->in_room->sector_type == SECT_FOREST) ||
		(ch->in_room->sector_type == SECT_FIELD) ||
		(ch->in_room->sector_type == SECT_HILLS) ||
		(ch->in_room->sector_type == SECT_MOUNTAIN)) 
		bonus = 20;
	    else bonus = -10;
	    if (!OUTSIDE(ch)) 
		bonus -= 40;
	}
	if (IS_TABAXI(ch))
	    bonus += GET_LEVEL(ch) >> 1;
	if ((GET_SKILL(ch, SKILL_TRACK) + bonus) < num)
	    do {
            dir = number(0, NUM_OF_DIRS - 1);
            count++;
	    } while ((!CAN_GO(ch, dir) || 
		      ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DEATH)) && 
		     count < 30);
	else if (GET_LEVEL(vict) > GET_LEVEL(ch) + number(0, 10))
	    gain_skill_prof(ch, SKILL_TRACK);
	sprintf(buf, "You sense a trail %s from here!\r\n", dirs[dir]);
	send_to_char(buf, ch);
	WAIT_STATE(ch, 6);
	break;
    }
}

ACMD(do_psilocate)
{

    struct char_data *vict = NULL;
    int dist, dir;
    byte error;

    skip_spaces(&argument);
  
    if (CHECK_SKILL(ch, SKILL_PSILOCATE) < 10) {
        send_to_char("You have no idea how.\r\n", ch);
        return;
    }
    if (!*argument) {
        send_to_char("Locate who's psyche?\r\n", ch);
        return;
    }
    if (!(vict = get_char_vis(ch, argument))) {
        sprintf(buf, "You cannot locate %s '%s'.\r\n", AN(argument), argument);
        send_to_char(buf, ch);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS) && GET_LEVEL(ch) < LVL_GOD) {
        send_to_char("Psychic powers are useless here!\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(vict->in_room,ROOM_NOPSIONICS)&&GET_LEVEL(ch) < LVL_GOD) {
        act("Psychic powers are useless where $E is!",FALSE,ch,0,vict,TO_CHAR);
        return;
    }
    if (vict && (IS_UNDEAD(vict) || IS_SLIME(vict) || IS_PUDDING(vict) ||
		 IS_ROBOT(vict) || IS_PLANT(vict))) {
        act("It is pointless to attempt this on $M.", FALSE,ch,0,vict, TO_CHAR);
        return;
    }
    if (GET_MANA(ch) < mag_manacost(ch, SKILL_PSILOCATE)) {
        send_to_char("You are too psychically exhausted.\r\n", ch);
        return;
    }

    GET_MANA(ch) -= mag_manacost(ch, SKILL_PSILOCATE);
    act("$n begins concentrating deeply, on a distant psyche.",
	TRUE, ch, 0, 0, TO_ROOM);

    if (AFF3_FLAGGED(vict, AFF3_SHROUD_OBSCUREMENT)) {
        act("$N is surrounded by a shroud of obscurement.",
            FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if ((dist = find_distance(ch->in_room, vict->in_room)) + 
	(AFF3_FLAGGED(vict, AFF3_PSISHIELD) ?( GET_LEVEL(vict) >> 1) : 0) > 
	GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 4) + GET_INT(ch)) {
        act("$N is out of your psychic range.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if ((dir = find_first_step(ch->in_room, vict->in_room, 2)) == 
	BFS_ALREADY_THERE) {
        send_to_char("You are in the same room!\r\n", ch);
        return;
    }
    if (dir < 0) {
        act("You cannot sense $S psi.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (CHECK_SKILL(vict, SKILL_PSILOCATE) > 
	number(10, CHECK_SKILL(ch, SKILL_PSILOCATE) + GET_LEVEL(ch)))
        act("You feel $n's psyche connect with your mind briefly.",
            FALSE, ch, 0, vict, TO_VICT);
    else if (GET_INT(vict) > number(0, GET_INT(ch) << 1))
        send_to_char("You feel a strange sensation on the periphery of your psyche.\r\n", vict);

    if (number(0, 121) + (AFF_FLAGGED(vict, AFF_HIDE) ? 10 : 0) > 
	CHECK_SKILL(ch, SKILL_PSILOCATE) + GET_INT(ch))
        dir = number(0, NUM_DIRS-1);

    error = number(0, 140 - CHECK_SKILL(ch, SKILL_PSILOCATE) - GET_INT(ch));
    error = MAX(0, error);
    error >>= 3;

    if (error)
        dist += number(-error, error);

    dist = MAX(1, dist);

    sprintf(buf, "$N seems to be about %d rooms away %s.", dist, to_dirs[dir]);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);

}

//
// smart_mobile_move()
// returns -1 on a critical failure
//

int smart_mobile_move(struct char_data *ch, int dir) {
  
    char doorbuf[128];

    if (dir < 0)
	return 0;

    if (EXIT(ch, dir) && EXIT(ch, dir)->to_room != NULL) {
	if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
	    if (IS_SET(EXIT(ch, dir)->exit_info, EX_SPECIAL)) // can't open here
		return 0;
	    if (EXIT(ch, dir)->keyword)
		strcpy(doorbuf, fname(EXIT(ch, dir)->keyword));
	    else
		sprintf(doorbuf, "door %s", dirs[dir]);

	    if (IS_SET(EXIT(ch, dir)->exit_info, EX_LOCKED)) {
		if (has_key(ch, EXIT(ch, dir)->key))
		    do_gen_door(ch, doorbuf, 0, SCMD_UNLOCK);
		else if ((IS_THIEF(ch) || IS_HOOD(ch)) &&
			 CHECK_SKILL(ch, SKILL_PICK_LOCK) > 30)
		    do_gen_door(ch, doorbuf, 0, SCMD_PICK);
		else if (IS_MAGE(ch) && CHECK_SKILL(ch, SPELL_KNOCK) &&
			 GET_MANA(ch) > (GET_MAX_MANA(ch) >> 1)) {
		    sprintf(doorbuf, "'knock' %s", doorbuf);
		    do_cast(ch, doorbuf, 0, 0);
		}
		else if (CHECK_SKILL(ch, SKILL_BREAK_DOOR) > 30 &&
			 GET_HIT(ch) > (GET_MAX_HIT(ch) >> 1))
		    do_bash(ch, doorbuf, 0, 0);
	    } else
		do_gen_door(ch, doorbuf, 0, SCMD_OPEN);

	} else if (EXIT(ch, dir)->to_room->isOpenAir() &&
		   ch->getPosition() != POS_FLYING) {
	    if (can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0))
		do_fly(ch, "", 0, 0);
	    else if (IS_MAGE(ch) && GET_LEVEL(ch) >= 33)
		cast_spell(ch, ch, 0, SPELL_FLY);
	    else if (IS_CLERIC(ch) && GET_LEVEL(ch) >= 32)
		cast_spell(ch, ch, 0, SPELL_AIR_WALK);
	    else if (IS_PHYSIC(ch))
		cast_spell(ch, ch, 0, SPELL_TIDAL_SPACEWARP);
	    else if (!number(0, 10)) {
		do_say(ch, "Well, SHIT!  I need to be able to fly!", 0, 0);
		return 0;
	    }
	} else if (SECT_TYPE(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM && 
		   ch->getPosition() != POS_FLYING  && 
		   can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0)) {
	    if (IS_AFFECTED(ch, AFF_INFLIGHT))
		do_fly(ch, "", 0, 0);
	    else if (IS_MAGE(ch) && GET_LEVEL(ch) >= 32)
		cast_spell(ch, ch, 0, SPELL_WATERWALK);
	    else if (!number(0, 10)) {
		do_say(ch, "Damn this water!  Can anybody help me cross?", 0, 0);  
		return 0;
	    }
	} else
	    perform_move(ch, dir, MOVE_NORM, 1);
    }
    return 0;
}

//
// hunt_victim()
//
// has a return value like damage()
//

int hunt_victim( struct char_data * ch ) {

    char buf2[MAX_STRING_LENGTH];
    extern struct char_data *character_list;
    void perform_tell(struct char_data *ch, struct char_data *vict, char *buf);
    struct affected_type *af_ptr = NULL;
    int dir;
    byte found;
    struct char_data *tmp;

    if (!ch || !HUNTING(ch))
	return 0;

    if ( ! HUNTING(ch)->in_room ) {
        slog("SYSERR:  hunting ! HUNTING(ch)->in_room !!");
        return 0;
    }

    /* make sure the char still exists */
    for (found = 0, tmp = character_list; tmp && !found; tmp = tmp->next)
        if (HUNTING(ch) == tmp)
            found = 1;

    if (!found) {
        if (!FIGHTING(ch)) {
            do_say(ch, "Damn!  My prey is gone!!", 0, 0);
            HUNTING(ch) = 0;
        }
        return 0;
    }
    if (GET_LEVEL(HUNTING(ch)) >= LVL_AMBASSADOR) {
        HUNTING(ch) = NULL;
        return 0;
    }
    if (HUNTING(ch) == FIGHTING(ch))
        return 0;

    if (ch->in_room == HUNTING(ch)->in_room &&
	!FIGHTING(ch) && CAN_SEE(ch, HUNTING(ch)) &&
	!PLR_FLAGGED(HUNTING(ch), PLR_WRITING | PLR_OLC) &&
	(!(af_ptr = affected_by_spell(HUNTING(ch), SKILL_DISGUISE)) ||
	 CAN_DETECT_DISGUISE(ch, HUNTING(ch), af_ptr->duration))) {
	if (peaceful_room_ok(ch, HUNTING(ch), false) && !check_infiltrate(HUNTING(ch), ch)) {
	    return best_attack(ch, HUNTING(ch));
        }
	return 0;
    }
    if (IS_CLERIC(ch) || IS_MAGE(ch)) {
        if (HUNTING(ch)->in_room && CAN_SEE(ch, HUNTING(ch)) && 
            peaceful_room_ok(ch, HUNTING(ch), false)) {
            if ((IS_CLERIC(ch) && GET_LEVEL(ch) > 16) ||
            (IS_MAGE(ch) && GET_LEVEL(ch) > 27)) { 
                if (GET_MANA(ch) < mag_manacost(ch, SPELL_SUMMON)) {
                    return cast_spell(ch, HUNTING(ch), 0, SPELL_SUMMON);
                }
            }
        }
    }

    if(!IS_AFFECTED(HUNTING(ch),AFF_NOTRACK) || 
       (IS_NPC(ch) && MOB_FLAGGED(ch,MOB_SPIRIT_TRACKER)))
        dir = find_first_step(ch->in_room, HUNTING(ch)->in_room, 0);
    else
        dir = -1;
    if (dir < 0) {
        act("$n says, 'Damn! Lost $M!'", FALSE, ch, 0, HUNTING(ch), TO_ROOM);
        HUNTING(ch) = 0;
        return 0;
    } else {
        if (smart_mobile_move(ch, dir) < 0) {
            HUNTING(ch) = NULL;
            return 0;
	}

	if ((ch->in_room == HUNTING(ch)->in_room) && CAN_SEE(ch, HUNTING(ch)) &&
	    (!(af_ptr = affected_by_spell(HUNTING(ch), SKILL_DISGUISE)) ||
	     CAN_DETECT_DISGUISE(ch, HUNTING(ch), af_ptr->duration)) &&
         !check_infiltrate(HUNTING(ch), ch)) {
	    if (peaceful_room_ok(ch, HUNTING(ch), false) &&
		    !PLR_FLAGGED(HUNTING(ch), PLR_OLC | PLR_WRITING)) {
            if (ch->getPosition() >= POS_STANDING && !FIGHTING(ch)) {
                if (IS_ANIMAL(ch)) {
                    act("$n snarls and attacks $N!!!",
                        FALSE, ch, 0, HUNTING(ch), TO_NOTVICT);
                    act("$n snarls and attacks you!!!", 
                        FALSE, ch, 0, HUNTING(ch), TO_VICT);
                } else if (IS_RACE(ch, RACE_ARCHON)) {
                    act("$n shouts, '$N you vile profaner of goodness!",
                        FALSE, ch, 0, HUNTING(ch), TO_ROOM);		
                } else if ( GET_MOB_VNUM(ch) == UNHOLY_STALKER_VNUM ) {
                    do_say(ch, "Time to die.", 0, SCMD_INTONE);
                } else {
                    if (!number(0, 3))
                        act("$n screams, 'Gotcha, punk ass $N!!'.", 
                        FALSE, ch, 0, HUNTING(ch), TO_ROOM);
                    else if (!number(0, 2)) {
                        sprintf(buf2, "Well, well well... if it isn't %s!", 
                            GET_NAME(HUNTING(ch)));
                        do_say(ch, buf2, 0, 0);
                    } else if (!number(0, 1)) {
                        sprintf(buf2, "You can run, but you can't hide %s!", 
                            GET_NAME(HUNTING(ch)));
                        do_say(ch, buf2, 0, 0);
                    } else { 
                        sprintf(buf2, "Now I have you, %s!", GET_NAME(HUNTING(ch)));
                        do_say(ch, buf2, 0, 0);
                    } 
                }
                return best_attack(ch, HUNTING(ch));
            }
	    } 
	} else if (ch->in_room == HUNTING(ch)->in_room) {
	    if (!number(0, 10))
		act("$n says, 'I know that jerk $N is around here somewhere!", 
		    FALSE, ch, 0, HUNTING(ch), TO_ROOM);
	    return 0;
	} else {
	    if (!number(0, 64))
            act("$n sniffs the ground.", FALSE, ch, 0, 0, TO_ROOM);
	    else if (!number(0, 64))
            act("$n says 'Im gonna get that freak $N!'", FALSE, ch, 0, 
                HUNTING(ch), TO_ROOM);
	    else if (!IS_ANIMAL(ch) && !MOB2_FLAGGED(ch, MOB2_SILENT_HUNTER) &&
		     (GET_LEVEL(ch) + number(1, 12)) > GET_LEVEL(HUNTING(ch))) {
            switch (number(0, 2048)) {
            case 0:
                sprintf(buf2, "You're toast, %s!", GET_NAME(HUNTING(ch)));
                break;
            case 1:
                sprintf(buf2, "I'm gonna find you, %s!", GET_NAME(HUNTING(ch)));
                perform_tell(ch, HUNTING(ch), buf2);
                return 0;
            case 2:
                sprintf(buf2, "You can run, but you can't hide, %s you sissy!", 
                    GET_NAME(HUNTING(ch)));
                break;
            case 3:
                sprintf(buf2, 
                    "You better run for the inn, cause I'm coming for you %s!", 
                    GET_NAME(HUNTING(ch)));
                break;
            case 4:
                sprintf(buf2, "You're gonna learn better than to mess with me, %s!",
                    GET_NAME(HUNTING(ch)));
                perform_tell(ch, HUNTING(ch), buf2);
                return 0;
            case 5:
                strcpy(buf2, "One of these days, I'm gonna get a little respect!");
                break;
            case 6:
                sprintf(buf2, "Well, looks like I'm gonna hafta teach %s a lesson!",
                    GET_NAME(HUNTING(ch)));
                break;
            case 7:
                sprintf(buf2, "Anybody know where that punk %s is?!", 
                    GET_NAME(HUNTING(ch)));
                break;
            case 8:
                sprintf(buf2, "Now you've pissed me off, %s!", 
                    GET_NAME(HUNTING(ch)));
                break;
            case 9:
                sprintf(buf2, "I've had it up to HERE with punks like %s!", 
                    GET_NAME(HUNTING(ch)));
                break;
            case 10:
                sprintf(buf2, "Don't worry %s... I'll find you!", 
                    GET_NAME(HUNTING(ch)));
                perform_tell(ch, HUNTING(ch), buf2);
                return 0;
            case 11:
                sprintf(buf2, "I'm practically on you, %s!", GET_NAME(HUNTING(ch)));
                do_gen_comm(ch, buf2, 0, SCMD_SHOUT);
                return 0;
            case 12:
                sprintf(buf2, "Hey you momma's %s!  I'm coming for you.", 
                    IS_FEMALE(HUNTING(ch)) ? "girl" : "boy");
                perform_tell(ch, HUNTING(ch), buf2);
                return 0;
            case 13:
                sprintf(buf2, "Looks like I'm gonna have to hunt you down, %s!", 
                    GET_NAME(HUNTING(ch)));
                if (number(0, 2)) {
                perform_tell(ch, HUNTING(ch), buf2);
                return 0;
                } 
            default:
                return 0;
            }
                
            if (!number(0, 4))
                do_gen_comm(ch, buf2, 0, SCMD_SHOUT);
            if (!number(0, 5))
                do_gen_comm(ch, buf2, 0, SCMD_HOLLER);
            else if (!number(0, 2))
                do_gen_comm(ch, buf2, 0, SCMD_GOSSIP);
            else if (!number(0, 1))
                do_gen_comm(ch, buf2, 0, SCMD_SPEW);
            else
                do_gen_comm(ch, buf2, 0, SCMD_MUSIC);
	    }
            
	    if (!number(0, 32))
            do_say(ch, "One of these days..", 0, 0);
	}
    }
    return 0;
}
