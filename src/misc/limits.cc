/**************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: limits.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "vehicle.h"
#include "clan.h"

#define READ_TITLE(ch) (GET_CLASS(ch) == CLASS_KNIGHT && IS_EVIL(ch) ?   \
			evil_knight_titles[(int)GET_LEVEL(ch)] :         \
			(GET_SEX(ch) == SEX_MALE ?                       \
			 titles[(int) MIN(GET_CLASS(ch), NUM_CLASSES-1)] \
			 [(int)GET_LEVEL(ch)].title_m :                  \
			 titles[(int)MIN(GET_CLASS(ch), NUM_CLASSES-1)]  \
			 [(int)GET_LEVEL(ch)].title_f))
			

    extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;
extern int exp_scale[];
extern int race_lifespan[];
extern char *evil_knight_titles[];

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int race, 
	 int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

    if (race > RACE_TROLL)
	race = RACE_HUMAN;

    race = race_lifespan[race];

    if (age < (race * 0.2))
	return (p0);	
    else if (age <= (race * 0.30))
	return (int) (p1 + (((age - (race * 0.2)) * (p2 - p1)) / (race * 0.2))); 
    else if (age <= (race * 0.55))
	return (int) (p2 + (((age - (race * 0.35)) * (p3 - p2)) / (race * 0.2)));
    else if (age <= (race * 0.75))
	return (int) (p3 + (((age - (race * 0.55)) * (p4 - p3)) / (race * 0.2)));
    else if (age <= race)
	return (int) (p4 + (((age - (race * 0.75)) * (p5 - p4)) / (race * 0.2)));
    else
	return (p6);
}

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int 
mana_gain(struct char_data * ch)
{
    int gain;

    if (IS_NPC(ch)) {
	/* Neat and fast */
	gain = GET_LEVEL(ch);
    } else {
	gain = graf(GET_AGE(ch), GET_RACE(ch), 4, 8, 12, 16, 12, 10, 8);

	/* Class calculations */
	gain += (GET_LEVEL(ch) >> 3);
    }
  
    /* Skill/Spell calculations */
    gain += (GET_WIS(ch) >> 2);

    // evil clerics get a gain bonus, up to 10%
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_EVIL(ch))
	gain -= (gain * GET_ALIGNMENT(ch)) / 10000;
  
    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
	gain <<= 1;
	break;
    case POS_RESTING:
	gain += (gain >> 1);	/* Divide by 2 */
	break;
    case POS_SITTING:
	gain += (gain >> 2);	/* Divide by 4 */
	break;
    case POS_FLYING:
	gain += (gain >> 1);
	break;
    }

    if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_PSYCHIC(ch) || IS_PHYSIC(ch))
	gain <<= 1;

    if (IS_AFFECTED(ch, AFF_POISON))
	gain >>= 2;
  
    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
	gain >>= 2;

    if (ch->in_room && ch->in_room->sector_type == SECT_DESERT && 
	!ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) &&
	ch->in_room->zone->weather->sunlight == SUN_LIGHT)
	gain >>= 1;

    if (IS_AFFECTED_2(ch, AFF2_MEDITATE)) {
	if (CHECK_SKILL(ch, ZEN_HEALING) > number(40, 100) ||
	    (IS_NPC(ch) && IS_MONK(ch) && GET_LEVEL(ch) > 40))
	    gain *= ((16 + GET_LEVEL(ch))/22);
	else
	    gain <<= 1;
    }

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
	gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && 
	(IS_KNIGHT(ch) || IS_CLERIC(ch)))
	gain >>= 2;
  
    return (gain);
}


int 
hit_gain(struct char_data * ch)
/* Hitpoint gain pr. game hour */
{
    int gain;
    struct affected_type *af = NULL;

    if (IS_NPC(ch)) {
	gain = GET_LEVEL(ch);
    } else {
	gain = graf(GET_AGE(ch), GET_RACE(ch), 10, 14, 22, 34, 18, 12, 6);

	/* Class/Level calculations */
	gain += (GET_LEVEL(ch) >> 3);

    }

    /* Skill/Spell calculations */
    gain += (GET_CON(ch) << 1);
    gain += (CHECK_SKILL(ch, SKILL_SPEED_HEALING) >> 3);
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL))
	gain += (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_DAMAGE_CONTROL)) >> 2;
  
    // good clerics get a gain bonus, up to 10%
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch))
	gain += (gain * GET_ALIGNMENT(ch)) / 10000;

    // radiation sickness prevents healing
    if ((af = affected_by_spell(ch, TYPE_RAD_SICKNESS))) {
	if (af->level <= 50)
	    gain -= (gain * af->level) / 50;  // important, divisor must be 50
    }

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 0);    /*  gain + gain  */
	else
	    gain += (gain >> 1);	/* gain + gain/2 */
	break;
    case POS_RESTING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 1);    /* divide by 2 */
	else
	    gain += (gain >> 2);	/* Divide by 4 */
	break;
    case POS_SITTING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 2);    /* divide by 4 */
	else
	    gain += (gain >> 3);	/* Divide by 8 */
	break;
    }

    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
	gain >>= 1;

    if (IS_POISONED(ch))
	gain >>= 2;
    if (IS_SICK(ch))
	gain >>= 1;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
	gain >>= 2;

    else if (IS_AFFECTED_2(ch, AFF2_MEDITATE))
	gain += (gain >> 1);

    if (affected_by_spell(ch, SPELL_METABOLISM))
	gain += (gain >> 2);

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
	gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && 
	(IS_KNIGHT(ch) || IS_CLERIC(ch)))
	gain >>= 2;

    return (gain);
}



int 
move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
    int gain;

    if (IS_NPC(ch)) {
	gain = GET_LEVEL(ch);
	if (MOB2_FLAGGED(ch, MOB2_MOUNT))
	    gain <<= 1;
    } else
	gain = graf(GET_AGE(ch), GET_RACE(ch), 18, 22, 26, 22, 18, 14, 12);

    /* Class/Level calculations */
    if IS_RANGER(ch)               	 gain += (gain >> 2);
    gain += (GET_LEVEL(ch) >> 3);

    /* Skill/Spell calculations */
    gain += (GET_CON(ch) >>2);

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 0);    /* divide by 1 */
	else
	    gain += (gain >> 1);	/* Divide by 2 */
	break;
    case POS_RESTING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 1);    /* divide by 2 */
	else
	    gain += (gain >> 2);	/* Divide by 4 */
	break;
    case POS_SITTING:
	if (IS_AFFECTED(ch, AFF_REJUV))
	    gain += (gain >> 2);    /* divide by 4 */
	else
	    gain += (gain >> 3);	/* Divide by 8 */
	break;
    }

    if (IS_POISONED(ch))
	gain >>= 2;
    if (IS_SICK(ch))
	gain >>= 1;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
	gain >>= 2;

    if (IS_AFFECTED_2(ch, AFF2_MEDITATE))
	gain += (gain >> 1);

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
	gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && 
	(IS_KNIGHT(ch) || IS_CLERIC(ch)))
	gain >>= 2;

    return (gain);
}


void 
set_title(struct char_data * ch, char *title)
{
    struct clan_data *clan = NULL;
    struct clanmember_data *memb = NULL;

    if (title == NULL) {
	title = READ_TITLE(ch);

	if (PRF2_FLAGGED(ch, PRF2_CLAN_TITLE) &&
	    (clan = real_clan(GET_CLAN(ch))) &&
	    (memb = real_clanmember(GET_IDNUM(ch), clan)) &&
	    memb->rank < clan->top_rank &&
	    clan->ranknames[(int)memb->rank])
	    title = clan->ranknames[(int)memb->rank];
    }
    if (strlen(title) > MAX_TITLE_LENGTH)
	title[MAX_TITLE_LENGTH] = '\0';

    if (GET_TITLE(ch) != NULL) {
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(GET_TITLE(ch));
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
    }
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    GET_TITLE(ch) = str_dup(title);
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}


void 
check_autowiz(struct char_data * ch)
{
    char buf[100];
    extern int use_autowiz;
    extern int min_wizlist_lev;
//    pid_t getpid(void);

    if (use_autowiz && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
	sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
		WIZLIST_FILE, LVL_AMBASSADOR, IMMLIST_FILE, (int) getpid());
	mudlog("Initiating autowiz.", CMP, LVL_AMBASSADOR, FALSE);
	system(buf);
    }
}



void 
gain_exp(struct char_data * ch, int gain)
{
    int is_altered = FALSE;
    int num_levels = 0;
    char buf[128];

    if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_AMBASSADOR)))
	return;

    if (IS_NPC(ch)) {
	GET_EXP(ch) = MIN(2000000000, 
			  ((unsigned int)GET_EXP(ch) + (unsigned int)gain));
	return;
    }
    if (gain > 0) {
	gain = MIN(max_exp_gain, gain);  /* put a cap on the max gain per kill */
	if ((gain + GET_EXP(ch)) > exp_scale[GET_LEVEL(ch) + 2])
	    gain = 
		(((exp_scale[GET_LEVEL(ch) + 2] -	exp_scale[GET_LEVEL(ch) + 1]) >> 1) +
		 exp_scale[GET_LEVEL(ch) +1]) - GET_EXP(ch);

	if (PLR_FLAGGED(ch, PLR_KILLER) || PLR_FLAGGED(ch, PLR_THIEF))
	    gain = 0;

	GET_EXP(ch) += gain;
	while (GET_LEVEL(ch) < (LVL_AMBASSADOR - 1) &&
	       GET_EXP(ch) >= exp_scale[GET_LEVEL(ch) + 1]) {
	    GET_LEVEL(ch) += 1;
	    num_levels++;
	    advance_level(ch, 0);
	    is_altered = TRUE;
	}

	if (is_altered) {
	    if (num_levels == 1)
		send_to_char("You rise a level!\r\n", ch);
	    else {
		sprintf(buf, "You rise %d levels!\r\n", num_levels);
		send_to_char(buf, ch);
	    }
	    save_char(ch, NULL);
	    set_title(ch, NULL);
	    check_autowiz(ch);
	}
    } else if (gain < 0) {
	gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
	    GET_EXP(ch) = 0;
    }
}


void gain_exp_regardless(struct char_data * ch, int gain)
{
    int is_altered = FALSE;
    int num_levels = 0;

    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
	GET_EXP(ch) = 0;

    if (!IS_NPC(ch)) {
	while (GET_LEVEL(ch) < LVL_GRIMP &&
	       GET_EXP(ch) >= exp_scale[GET_LEVEL(ch) + 1]) {
	    GET_LEVEL(ch) += 1;
	    num_levels++;
	    advance_level(ch, 1);
	    is_altered = TRUE;
	}

	if (is_altered) {
	    if (num_levels == 1)
		send_to_char("You rise a level!\r\n", ch);
	    else {
		sprintf(buf, "You rise %d levels!\r\n", num_levels);
		send_to_char(buf, ch);
	    }
	    set_title(ch, NULL);
	    check_autowiz(ch);
	}
    }
}


void 
gain_condition(struct char_data * ch, int condition, int value)
{
    int intoxicated = 0;

    if (GET_COND(ch, condition) == -1 || !value)	/* No change */
	return;

    intoxicated = (GET_COND(ch, DRUNK) > 0);

    GET_COND(ch, condition) += value;

    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

    if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING) || 
	PLR_FLAGGED(ch, PLR_OLC))
	return;

    if (ch->desc && !STATE(ch->desc)) {
	switch (condition) {
	case FULL:
	    if (!number(0, 3))
		send_to_char("You feel quite hungry.\r\n", ch);
	    else if (!number(0, 2))
		send_to_char("You are famished.\r\n", ch);
	    else
		send_to_char("You are hungry.\r\n", ch);
	    return;
	case THIRST:
	    if (IS_VAMPIRE(ch))
		send_to_char("You feel a thirst for blood...\r\n", ch);
	    else if (!number(0, 1))
		send_to_char("Your throat is parched.\r\n", ch);
	    else
		send_to_char("You are thirsty.\r\n", ch);
	    return;
	case DRUNK:
	    if (intoxicated)
		send_to_char("You are now sober.\r\n", ch);
	    return;
	default:
	    break;
	}
    }
}


int 
check_idling(struct char_data * ch)
{
    void Crash_rentsave(struct char_data *ch, int cost);

    if (++(ch->char_specials.timer) > 1 && ch->desc)
	ch->desc->repeat_cmd_count = 0;

    if ((ch->char_specials.timer) > 10 && !PLR_FLAGGED(ch, PLR_OLC))
	if (GET_WAS_IN(ch) == NULL && ch->in_room != NULL) {
	    GET_WAS_IN(ch) = ch->in_room;
	    if (FIGHTING(ch)) {
		stop_fighting(FIGHTING(ch));
		stop_fighting(ch);
	    }
	    act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
	    send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
	    save_char(ch, NULL);
	    Crash_crashsave(ch);
	    char_from_room(ch);
	    char_to_room(ch, real_room(1));
	} else if (ch->char_specials.timer > 60) {
	    if (ch->in_room != NULL)
		char_from_room(ch);
	    char_to_room(ch, real_room(3));
	    if (ch->desc)
		close_socket(ch->desc);
	    ch->desc = NULL;
	    Crash_idlesave(ch);
	    sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
	    mudlog(buf, CMP, LVL_GOD, TRUE);
	    extract_char(ch, FALSE);
	    return TRUE;
	}
    return FALSE;
}



/* Update PCs, NPCs, and objects */
void 
point_update(void)
{
    void update_char_objects(struct char_data * ch);	/* handler.c */
    void extract_obj(struct obj_data * obj);	/* handler.c */
    register struct char_data *i, *next_char;
    register struct obj_data *j, *next_thing, *jj, *next_thing2;
    struct room_data *rm;
    int full = 0, thirst = 0, drunk = 0, z;

    /* characters */
    for (i = character_list; i; i = next_char) {
	next_char = i->next;
    
	if (GET_POS(i) >= POS_STUNNED) {
	    GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
	    GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
	    GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

	    if (IS_AFFECTED(i, AFF_POISON) &&	
		damage(i, i, dice(4, 11) +
		       (affected_by_spell(i, SPELL_METABOLISM) ? dice (4, 3) : 0),  
		       SPELL_POISON, -1))
		continue;
	    if (GET_POS(i) <= POS_STUNNED)
		update_pos(i);

	    if (IS_SICK(i)&&affected_by_spell(i, SPELL_SICKNESS)&&!number(0, 4)) {
		switch (number(0, 6)) {
		case 0:
		    act("$n pukes all over the place.", FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You puke all over the place.\r\n", i);
		    break;
		case 1:
		    act("$n vomits uncontrollably.", FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You vomit uncontrollably.\r\n", i);
		    break;
		case 2:
		    act("$n begins to regurgitate steaming bile.",FALSE,i,0,0,TO_ROOM);
		    send_to_char("You begin to regurgitate bile.\r\n", i);
		    break;
		case 3:
		    act("$n is violently overcome with a fit of dry heaving.", 
			FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You are violently overcome with a fit of dry heaving.\r\n", i);
		    break;
		case 4:
		    act("$n begins to retch.", FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You begin to retch.\r\n", i);
		    break;
		case 5:
		    act("$n violently ejects $s lunch!", FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You violently eject your lunch!\r\n", i);
		    break;
		default:
		    act("$n begins an extended session of tossing $s cookies.", 
			FALSE, i, 0, 0, TO_ROOM);
		    send_to_char("You begin an extended session of tossing your cookies.\r\n", i); 
		    break;
		}
	    }
	} else if ((GET_POS(i)==POS_INCAP && damage(i, i, 1, TYPE_SUFFERING, -1)) ||
		   (GET_POS(i)==POS_MORTALLYW && damage(i,i,2,TYPE_SUFFERING, -1)))
	    continue;

	if (!IS_NPC(i)) {
	    update_char_objects(i);
	    if (GET_LEVEL(i) < LVL_GOD && check_idling(i))
		continue;
	} else if (i->char_specials.timer) 
	    i->char_specials.timer -= 1;


	full = 1;
	if (affected_by_spell(i, SPELL_METABOLISM))
	    full += 1;
	if (IS_CYBORG(i)) {
	    if (AFF3_FLAGGED(i, AFF3_STASIS))
		full >>= 2;
	    else if (GET_LEVEL(i) > number(10, 60))
		full >>= 1;
	}
	gain_condition(i, FULL, -full);

	thirst = 1;
	if (SECT_TYPE(i->in_room) == SECT_DESERT)
	    thirst += 2;
	if (ROOM_FLAGGED(i->in_room, ROOM_FLAME_FILLED))
	    thirst += 2;
	if (affected_by_spell(i, SPELL_METABOLISM))
	    thirst += 1;
	if (IS_CYBORG(i)) {
	    if (AFF3_FLAGGED(i, AFF3_STASIS))
		thirst >>= 2;
	    else if (GET_LEVEL(i) > number(10, 60))
		thirst >>= 1;
	}
	gain_condition(i, THIRST, -thirst);

	drunk = 1;
	if (IS_MONK(i))
	    drunk += 1;
	if (IS_CYBORG(i))
	    drunk >>= 1;
	if (affected_by_spell(i, SPELL_METABOLISM))
	    drunk += 1;
	gain_condition(i, DRUNK, -drunk);
    }

    /* objects */
    for (j = object_list; j; j = next_thing) {
	next_thing = j->next;	/* Next in object list */

	/* If this is a corpse */
	if (IS_CORPSE(j)) {
	    /* timer count down */
	    if (GET_OBJ_TIMER(j) > 0)
		GET_OBJ_TIMER(j)--;

	    if (!GET_OBJ_TIMER(j)) {

		if (j->carried_by)
		    act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
		if (j->worn_by)
		    act("$p disintigrates as you are wearing it.", FALSE, j->worn_by, j, 0, TO_CHAR);
		else if ((j->in_room != NULL) && (j->in_room->people)) {
		    if (ROOM_FLAGGED(j->in_room, ROOM_FLAME_FILLED) ||
			SECT_TYPE(j->in_room) == SECT_FIRE_RIVER ||
			GET_PLANE(j->in_room) == PLANE_HELL_4 ||
			GET_PLANE(j->in_room) == PLANE_HELL_1 || !number(0, 50)) {
			act("$p spontaneously combusts and is devoured by flames.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("$p spontaneously combusts and is devoured by flames.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (ROOM_FLAGGED(j->in_room, ROOM_ICE_COLD) ||
			       GET_PLANE(j->in_room)==PLANE_HELL_5|| !number(0, 250)) {
			act("$p freezes and shatters into dust.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("$p freezes and shatters into dust.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (GET_PLANE(j->in_room)==PLANE_ASTRAL || !number(0, 250)) {
			act("A sudden psychic wind rips through $p.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("A sudden psychic wind rips through $p.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (GET_TIME_FRAME(j->in_room)==TIME_TIMELESS || 
			       !number(0, 250)) {
			act("$p is pulled into a timeless void and nullified.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("$p is pulled into a timeless void and nullified.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (SECT_TYPE(j->in_room) == SECT_WATER_SWIM ||
			       SECT_TYPE(j->in_room) == SECT_WATER_NOSWIM) {
			act("$p sinks beneath the surface and is gone.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("$p sinks beneath the surface and is gone.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (SECT_TYPE(j->in_room) == SECT_UNDERWATER) {
			act("A school of small fish appears and devours $p.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("A school of small fish appears and devours $p.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (!ROOM_FLAGGED(j->in_room,ROOM_INDOORS)&& !number(0, 2)) {
			act("A flock of carrion birds hungrily devours $p.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("A flock of carrion birds hungrily devours $p.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else if (number(0, 3)) {
			act("A quivering horde of maggots consumes $p.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("A quivering horde of maggots consumes $p.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    } else {
			act("$p decays into nothing before your eyes.",
			    TRUE, j->in_room->people, j, 0, TO_ROOM);
			act("$p decays into nothing before your eyes.",
			    TRUE, j->in_room->people, j, 0, TO_CHAR);
		    }
		}
		for (jj = j->contains; jj; jj = next_thing2) {
		    next_thing2 = jj->next_content;	/* Next in inventory */
		    obj_from_obj(jj);

		    if (j->in_obj)
			obj_to_obj(jj, j->in_obj);
		    else if (j->carried_by)
			obj_to_char(jj, j->carried_by);
		    else if (j->worn_by)
			obj_to_char(jj, j->worn_by);
		    else if (j->in_room != NULL)
			obj_to_room(jj, j->in_room);
		    else 
			raise( SIGSEGV );

		    if (IS_IMPLANT(jj) && !CAN_WEAR(jj, ITEM_WEAR_TAKE)) {

			SET_BIT(jj->obj_flags.wear_flags, ITEM_WEAR_TAKE);

			if (!IS_OBJ_TYPE(jj, ITEM_ARMOR))
			    damage_eq(NULL, jj, (GET_OBJ_DAM(jj) >> 2));
			else	    
			    damage_eq(NULL, jj, (GET_OBJ_DAM(jj) >> 1));

		    }

		}
		extract_obj(j);
	    }
	} else if (GET_OBJ_VNUM(j) < 0 &&
		   ((IS_OBJ_TYPE(j, ITEM_DRINKCON) && isname("head", j->name)) ||
		    (IS_OBJ_TYPE(j, ITEM_FOOD) && isname("heart", j->name)))) {
	    // body parts
	    if (GET_OBJ_TIMER(j) > 0)
		GET_OBJ_TIMER(j)--;
	    if (!GET_OBJ_TIMER(j)) {
		if (j->carried_by)
		    act("$p collapses into mush in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
		else if ((j->in_room != NULL) && (j->in_room->people)) {
		    act("$p collapses into nothing.",
			TRUE, j->in_room->people, j, 0, TO_ROOM);
		    act("$p collapses into nothing.",
			TRUE, j->in_room->people, j, 0, TO_CHAR);
		}

		// drop out the (damaged) implants
		for (jj = j->contains; jj; jj = next_thing2) {
		    next_thing2 = jj->next_content;	/* Next in inventory */
		    obj_from_obj(jj);

		    if (j->carried_by)
			obj_to_char(jj, j->carried_by);
		    else if (j->worn_by)
			obj_to_char(jj, j->worn_by);
		    else if (j->in_obj)
			obj_to_obj(jj, j->in_obj);
		    else if (j->in_room)
			obj_to_room(jj, j->in_room);
		    else
			raise ( SIGSEGV );

		    // fix up the implants, and damage them
		    if (IS_IMPLANT(jj) && !CAN_WEAR(jj, ITEM_WEAR_TAKE)) {

			SET_BIT(jj->obj_flags.wear_flags, ITEM_WEAR_TAKE);

			if (!IS_OBJ_TYPE(jj, ITEM_ARMOR))
			    damage_eq(NULL, jj, (GET_OBJ_DAM(jj) >> 1));
			else 
			    damage_eq(NULL, jj, (GET_OBJ_DAM(jj) >> 2));
	    
		    }
		}
		extract_obj(j);
	    }
	} else if (GET_OBJ_VNUM(j) == BLOOD_VNUM) { /* blood pools */
	    GET_OBJ_TIMER(j)--;
	    if (GET_OBJ_TIMER(j) <= 0) {
		extract_obj(j);
	    }
	}      
	else if (IS_OBJ_STAT2(j, ITEM2_UNAPPROVED) ||
		 (IS_OBJ_TYPE(j, ITEM_KEY) && GET_OBJ_TIMER(j))) { // keys && unapp
	    if (IS_OBJ_TYPE(j, ITEM_KEY)) { // skip keys still in zone
		z = zone_number(GET_OBJ_VNUM(j));
		if (((rm = where_obj(j)) && rm->zone->number == z) ||
		    !obj_owner(j)) {
		    continue;
		}
	    }
	    /* timer count down */
	    if (GET_OBJ_TIMER(j) > 0)
		GET_OBJ_TIMER(j)--;

	    if (!GET_OBJ_TIMER(j)) {

		if (j->carried_by)
		    act("$p slowly fades out of existance.", 
			FALSE, j->carried_by, j, 0, TO_CHAR);
		if (j->worn_by)
		    act("$p disintigrates as you are wearing it.", 
			FALSE, j->worn_by, j, 0, TO_CHAR);
		else if ((j->in_room != NULL) && (j->in_room->people)) {
		    act("$p slowly fades out of existance.",
			TRUE, j->in_room->people, j, 0, TO_ROOM);
		    act("$p slowly fades out of existance.",
			TRUE, j->in_room->people, j, 0, TO_CHAR);
		}
		for (jj = j->contains; jj; jj = next_thing2) {
		    next_thing2 = jj->next_content;	/* Next in inventory */
		    obj_from_obj(jj);

		    if (j->in_obj)
			obj_to_obj(jj, j->in_obj);
		    else if (j->carried_by)
			obj_to_room(jj, j->carried_by->in_room);
		    else if (j->worn_by)
			obj_to_room(jj, j->worn_by->in_room);
		    else if (j->in_room != NULL)
			obj_to_room(jj, j->in_room);
		    else
			raise( SIGSEGV );
		}
		extract_obj(j);
	    }
	}
    }
}

