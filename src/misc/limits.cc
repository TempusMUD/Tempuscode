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
#include "specs.h"
#include "fight.h"

#define READ_TITLE(ch) (GET_CLASS(ch) == CLASS_KNIGHT && IS_EVIL(ch) ?   \
			evil_knight_titles[(int)GET_LEVEL(ch)] :         \
			(GET_SEX(ch) == SEX_MALE ?                       \
			 titles[(int) MIN(GET_CLASS(ch), NUM_CLASSES-1)] \
			 [(int)GET_LEVEL(ch)].title_m :                  \
			 titles[(int)MIN(GET_CLASS(ch), NUM_CLASSES-1)]  \
			 [(int)GET_LEVEL(ch)].title_f))


extern struct obj_data *object_list;
extern struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
extern struct room_data *world;
extern struct zone_data *zone_table;
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
int
graf(int age, int race, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (race > RACE_TROLL)
		race = RACE_HUMAN;

	race = race_lifespan[race];

	if (age < (race * 0.2))
		return (p0);
	else if (age <= (race * 0.30))
		return (int)(p1 + (((age - (race * 0.2)) * (p2 - p1)) / (race * 0.2)));
	else if (age <= (race * 0.55))
		return (int)(p2 + (((age - (race * 0.35)) * (p3 -
						p2)) / (race * 0.2)));
	else if (age <= (race * 0.75))
		return (int)(p3 + (((age - (race * 0.55)) * (p4 -
						p3)) / (race * 0.2)));
	else if (age <= race)
		return (int)(p4 + (((age - (race * 0.75)) * (p5 -
						p4)) / (race * 0.2)));
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
mana_gain(struct Creature *ch)
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
	switch (ch->getPosition()) {
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
			gain *= ((16 + GET_LEVEL(ch)) / 22);
		else
			gain <<= 1;
	}

	if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
		gain += (GET_REMORT_GEN(ch) * gain) / 3;

	if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
		gain >>= 2;

	return (gain);
}


int
hit_gain(struct Creature *ch)
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
			gain -= (gain * af->level) / 50;	// important, divisor must be 50
	}

	/* Position calculations    */
	switch (ch->getPosition()) {
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 0);	/*  gain + gain  */
		else
			gain += (gain >> 1);	/* gain + gain/2 */
		break;
	case POS_RESTING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 1);	/* divide by 2 */
		else
			gain += (gain >> 2);	/* Divide by 4 */
		break;
	case POS_SITTING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 2);	/* divide by 4 */
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

	if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
		gain >>= 2;

	return (gain);
}



int
move_gain(struct Creature *ch)
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
	if IS_RANGER
		(ch) gain += (gain >> 2);
	gain += (GET_LEVEL(ch) >> 3);

	/* Skill/Spell calculations */
	gain += (GET_CON(ch) >> 2);

	/* Position calculations    */
	switch (ch->getPosition()) {
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 0);	/* divide by 1 */
		else
			gain += (gain >> 1);	/* Divide by 2 */
		break;
	case POS_RESTING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 1);	/* divide by 2 */
		else
			gain += (gain >> 2);	/* Divide by 4 */
		break;
	case POS_SITTING:
		if (IS_AFFECTED(ch, AFF_REJUV))
			gain += (gain >> 2);	/* divide by 4 */
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

	if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
		gain >>= 2;

	return (gain);
}


void
set_title(struct Creature *ch, char *title)
{
    if( title == NULL ) {
        title = strdup( "" );
    }

	if (strlen(title) > MAX_TITLE_LENGTH)
		title[MAX_TITLE_LENGTH] = '\0';

	if (GET_TITLE(ch) != NULL) {
		free(GET_TITLE(ch));
	}
	while (*title && ' ' == *title)
		title++;
	GET_TITLE(ch) = (char *)malloc(strlen(title) + 2);
	if (*title) {
		strcpy(GET_TITLE(ch), " ");
		strcat(GET_TITLE(ch), title);
	} else
		*GET_TITLE(ch) = '\0';
}


void
check_autowiz(struct Creature *ch)
{
	char buf[100];
	extern int use_autowiz;
	extern int min_wizlist_lev;
//    pid_t getpid(void);

	if (use_autowiz && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
			WIZLIST_FILE, LVL_AMBASSADOR, IMMLIST_FILE, (int)getpid());
		mudlog(LVL_AMBASSADOR, CMP, false, "Initiating autowiz");
		system(buf);
	}
}



void
gain_exp(struct Creature *ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;

	if (ch && ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
		return;

	if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1
				|| GET_LEVEL(ch) >= LVL_AMBASSADOR)))
		return;

	if (IS_NPC(ch)) {
		GET_EXP(ch) = MIN(2000000000,
			((unsigned int)GET_EXP(ch) + (unsigned int)gain));
		return;
	}
	if (gain > 0) {
		gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */
		if ((gain + GET_EXP(ch)) > exp_scale[GET_LEVEL(ch) + 2])
			gain =
				(((exp_scale[GET_LEVEL(ch) + 2] - exp_scale[GET_LEVEL(ch) +
							1]) >> 1) + exp_scale[GET_LEVEL(ch) + 1]) -
				GET_EXP(ch);

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
				send_to_char(ch, "You rise a level!\r\n");
			else {
				send_to_char(ch, "You rise %d levels!\r\n", num_levels);
			}
			ch->saveToXML();
			check_autowiz(ch);
		}
	} else if (gain < 0) {
		gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
		GET_EXP(ch) += gain;
		if (GET_EXP(ch) < 0)
			GET_EXP(ch) = 0;
	}
}


void
gain_exp_regardless(struct Creature *ch, int gain)
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
				send_to_char(ch, "You rise a level!\r\n");
			else {
				send_to_char(ch, "You rise %d levels!\r\n", num_levels);
			}
			check_autowiz(ch);
		}
	}
}


void
gain_condition(struct Creature *ch, int condition, int value)
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
				send_to_char(ch, "You feel quite hungry.\r\n");
			else if (!number(0, 2))
				send_to_char(ch, "You are famished.\r\n");
			else
				send_to_char(ch, "You are hungry.\r\n");
			return;
		case THIRST:
			if (IS_VAMPIRE(ch))
				send_to_char(ch, "You feel a thirst for blood...\r\n");
			else if (!number(0, 1))
				send_to_char(ch, "Your throat is parched.\r\n");
			else
				send_to_char(ch, "You are thirsty.\r\n");
			return;
		case DRUNK:
			if (intoxicated)
				send_to_char(ch, "You are now sober.\r\n");
			return;
		default:
			break;
		}
	}
}


int
check_idling(struct Creature *ch)
{
	void set_desc_state(int state, struct descriptor_data *d);

	if (++(ch->char_specials.timer) > 1 && ch->desc)
		ch->desc->repeat_cmd_count = 0;

	if ((ch->char_specials.timer) > 10 && !PLR_FLAGGED(ch, PLR_OLC)) {
		if (ch->desc && STATE(ch->desc) == CXN_NETWORK) {
			send_to_char(ch, "Idle limit reached.  Connection reset by peer.\r\n");
			set_desc_state(CXN_PLAYING, ch->desc);
		} else if (ch->char_specials.timer > 60) {
			if (GET_LEVEL(ch) > 49) {
				// Immortals have their afk flag set when idle
				if (!PLR_FLAGGED(ch, PLR_AFK)) {
					send_to_char(ch,
						"You have been idle, and are now marked afk.\r\n");
					SET_BIT(PLR_FLAGS(ch), PLR_AFK);
				}
			} else {
				if (ch->in_room != NULL)
					char_from_room(ch,false);
				char_to_room(ch, real_room(3),true);
				if (ch->desc)
					close_socket(ch->desc);
				ch->desc = NULL;
				ch->idle();
				return true;
			}
		}
	}
	return FALSE;
}


void
save_all_players(void)
{
	Creature *i;

	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		i = *cit;
		if (!IS_NPC(i))
			i->crashSave();
	}
}

/* Update PCs, NPCs, objects, and shadow zones */
void
point_update(void)
{
	void update_char_objects(struct Creature *ch);	/* handler.c */
	void extract_obj(struct obj_data *obj);	/* handler.c */
	register struct Creature *i;
	register struct obj_data *j, *next_thing, *jj, *next_thing2;
	struct room_data *rm;
	struct zone_data *zone;
	int full = 0, thirst = 0, drunk = 0, z, out_of_zone = 0;

	/* shadow zones */
	for (zone = zone_table; zone; zone = zone->next) {
		if (SHADOW_ZONE(zone)) {
			if (ZONE_IS_SHADE(zone)) {
				shade_zone(NULL, zone, 0, NULL, SPECIAL_TICK);
			}
			// Add future shadow zone specs here shadow zone spec should work for all
			// shadow zones so long as you create an integer array with the number for
			// all the rooms which link the zone to the rest of the world.
		}
	}

	/* characters */
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		i = *cit;

		if (i->getPosition() >= POS_STUNNED) {
			GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
			GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
			GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

			if (IS_AFFECTED(i, AFF_POISON) &&
				damage(i, i, dice(4, 11) + (affected_by_spell(i,
							SPELL_METABOLISM) ? dice(4, 3) : 0), SPELL_POISON,
					-1))
				continue;

			if (i->getPosition() <= POS_STUNNED)
				update_pos(i);

			if (IS_SICK(i) && affected_by_spell(i, SPELL_SICKNESS)
				&& !number(0, 4)) {
				switch (number(0, 6)) {
				case 0:
					act("$n pukes all over the place.", FALSE, i, 0, 0,
						TO_ROOM);
					send_to_char(i, "You puke all over the place.\r\n");
					break;

				case 1:
					act("$n vomits uncontrollably.", FALSE, i, 0, 0, TO_ROOM);
					send_to_char(i, "You vomit uncontrollably.\r\n");
					break;

				case 2:
					act("$n begins to regurgitate steaming bile.", FALSE, i, 0,
						0, TO_ROOM);
					send_to_char(i, "You begin to regurgitate bile.\r\n");
					break;

				case 3:
					act("$n is violently overcome with a fit of dry heaving.",
						FALSE, i, 0, 0, TO_ROOM);
					send_to_char(i, 
						"You are violently overcome with a fit of dry heaving.\r\n");
					break;

				case 4:
					act("$n begins to retch.", FALSE, i, 0, 0, TO_ROOM);
					send_to_char(i, "You begin to retch.\r\n");
					break;

				case 5:
					act("$n violently ejects $s lunch!", FALSE, i, 0, 0,
						TO_ROOM);
					send_to_char(i, "You violently eject your lunch!\r\n");
					break;

				default:
					act("$n begins an extended session of tossing $s cookies.",
						FALSE, i, 0, 0, TO_ROOM);
					send_to_char(i, 
						"You begin an extended session of tossing your cookies.\r\n");
					break;
				}
			}
		} else if ((i->getPosition() == POS_INCAP
				|| i->getPosition() == POS_MORTALLYW)) {
			// If they've been healed since they were incapacitated,
			//  Update thier position appropriately.
			if (GET_HIT(i) > -11) {
				if (GET_HIT(i) > 0) {
					i->setPosition(POS_RESTING);
				} else if (GET_HIT(i) > -3) {
					i->setPosition(POS_STUNNED);
				} else if (GET_HIT(i) > -6) {
					i->setPosition(POS_INCAP);
				} else {
					i->setPosition(POS_MORTALLYW);
				}
			}
			// Blood! Blood makes the grass grow drill seargent!
			if ((i->getPosition() == POS_INCAP
					&& damage(i, i, 1, TYPE_SUFFERING, -1))
				|| (i->getPosition() == POS_MORTALLYW
					&& damage(i, i, 2, TYPE_SUFFERING, -1)))
				continue;
		}

		if (!IS_NPC(i)) {
			update_char_objects(i);
			if (check_idling(i))
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

        /* player frozen */
		if (PLR_FLAGGED(i, PLR_FROZEN) && i->player_specials->thaw_time > -1) {
			time_t now;

			now = time(NULL);

			if (now > i->player_specials->thaw_time) {
				REMOVE_BIT(PLR_FLAGS(i), PLR_FROZEN);
				i->player_specials->thaw_time = 0;
				mudlog(MAX(LVL_POWER, GET_INVIS_LVL(i)), BRF, true,
					   "(GC) %s un-frozen by timeout.", GET_NAME(i));
				send_to_char(i, "You thaw out and can move again.\r\n");
				act("$n has thawed out and can move again.",
					false, i, 0, 0, TO_ROOM);
			}
		}
	}

	/* objects */
	for (j = object_list; j; j = next_thing) {
		next_thing = j->next;	/* Next in object list */

		// First check for a tick special
		if (GET_OBJ_SPEC(j) && GET_OBJ_SPEC(j)(NULL, j, 0, NULL, SPECIAL_TICK))
			continue;

		/* If this is a corpse */
		if (IS_CORPSE(j)) {
			/* timer count down */
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;

			if (!GET_OBJ_TIMER(j)) {
				if (j->carried_by)
					act("$p decays in your hands.", FALSE, j->carried_by, j, 0,
						TO_CHAR);
				if (j->worn_by)
					act("$p disintegrates as you are wearing it.", FALSE,
						j->worn_by, j, 0, TO_CHAR);
				else if ((j->in_room != NULL) && (j->in_room->people)) {
					if (ROOM_FLAGGED(j->in_room, ROOM_FLAME_FILLED) ||
						SECT_TYPE(j->in_room) == SECT_FIRE_RIVER ||
						GET_PLANE(j->in_room) == PLANE_HELL_4 ||
						GET_PLANE(j->in_room) == PLANE_HELL_1
						|| !number(0, 50)) {
						act("$p spontaneously combusts and is devoured by flames.", TRUE, j->in_room->people, j, 0, TO_ROOM);
						act("$p spontaneously combusts and is devoured by flames.", TRUE, j->in_room->people, j, 0, TO_CHAR);
					} else if (ROOM_FLAGGED(j->in_room, ROOM_ICE_COLD) ||
						GET_PLANE(j->in_room) == PLANE_HELL_5
						|| !number(0, 250)) {
						act("$p freezes and shatters into dust.", TRUE,
							j->in_room->people, j, 0, TO_ROOM);
						act("$p freezes and shatters into dust.", TRUE,
							j->in_room->people, j, 0, TO_CHAR);
					} else if (GET_PLANE(j->in_room) == PLANE_ASTRAL
						|| !number(0, 250)) {
						act("A sudden psychic wind rips through $p.", TRUE,
							j->in_room->people, j, 0, TO_ROOM);
						act("A sudden psychic wind rips through $p.", TRUE,
							j->in_room->people, j, 0, TO_CHAR);
					} else if (GET_TIME_FRAME(j->in_room) == TIME_TIMELESS
						|| !number(0, 250)) {
						act("$p is pulled into a timeless void and nullified.",
							TRUE, j->in_room->people, j, 0, TO_ROOM);
						act("$p is pulled into a timeless void and nullified.",
							TRUE, j->in_room->people, j, 0, TO_CHAR);
					} else if (SECT_TYPE(j->in_room) == SECT_WATER_SWIM
						|| SECT_TYPE(j->in_room) == SECT_WATER_NOSWIM) {
						act("$p sinks beneath the surface and is gone.", TRUE,
							j->in_room->people, j, 0, TO_ROOM);
						act("$p sinks beneath the surface and is gone.", TRUE,
							j->in_room->people, j, 0, TO_CHAR);
					} else if (SECT_TYPE(j->in_room) == SECT_UNDERWATER) {
						act("A school of small fish appears and devours $p.",
							TRUE, j->in_room->people, j, 0, TO_ROOM);
						act("A school of small fish appears and devours $p.",
							TRUE, j->in_room->people, j, 0, TO_CHAR);
					} else if (!ROOM_FLAGGED(j->in_room, ROOM_INDOORS)
						&& !number(0, 2)) {
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
						raise(SIGSEGV);

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
				((IS_OBJ_TYPE(j, ITEM_WEAPON) && isname("leg", j->name)) &&
					(j->worn_on != WEAR_WIELD && j->worn_on != WEAR_WIELD_2))
				|| (IS_OBJ_TYPE(j, ITEM_FOOD) && isname("heart", j->name)))) {
			// body parts
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
			if (GET_OBJ_TIMER(j) == 0) {
				if (j->carried_by)
					act("$p collapses into mush in your hands.",
						FALSE, j->carried_by, j, 0, TO_CHAR);
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
						raise(SIGSEGV);

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
		} else if (GET_OBJ_VNUM(j) == BLOOD_VNUM) {	/* blood pools */
			GET_OBJ_TIMER(j)--;
			if (GET_OBJ_TIMER(j) <= 0) {
				extract_obj(j);
			}
		} else if (GET_OBJ_VNUM(j) == QUANTUM_RIFT_VNUM) {
			GET_OBJ_TIMER(j)--;
			if (GET_OBJ_TIMER(j) <= 0) {
				if (j->action_description && j->in_room) {
					act("$p collapses in on itself.",
						TRUE, j->in_room->people, j, 0, TO_CHAR);
					act("$p collapses in on itself.",
						TRUE, j->in_room->people, j, 0, TO_ROOM);
				}
				extract_obj(j);
			}
		} else if (GET_OBJ_VNUM(j) == ICE_VNUM) {
			if (j->in_room) {
				if (SECT_TYPE(j->in_room) == SECT_DESERT ||
					SECT_TYPE(j->in_room) == SECT_FIRE_RIVER ||
					SECT_TYPE(j->in_room) == SECT_PITCH_PIT ||
					SECT_TYPE(j->in_room) == SECT_PITCH_SUB ||
					SECT_TYPE(j->in_room) == SECT_ELEMENTAL_FIRE) {
					GET_OBJ_TIMER(j) = 0;
				}

				if (!ROOM_FLAGGED(j->in_room, ROOM_ICE_COLD)) {
					GET_OBJ_TIMER(j)--;
				}

				if (GET_OBJ_TIMER(j) <= 0) {
					if (j->action_description) {
						act("$p melts and is gone.",
							TRUE, j->in_room->people, j, 0, TO_CHAR);
						act("$p melts and is gone.",
							TRUE, j->in_room->people, j, 0, TO_ROOM);
					}
					extract_obj(j);
				}
			}
		} else if (IS_OBJ_STAT2(j, ITEM2_UNAPPROVED) ||
			(IS_OBJ_TYPE(j, ITEM_KEY) && GET_OBJ_TIMER(j)) ||
			(GET_OBJ_SPEC(j) == fate_portal) ||
			(IS_OBJ_STAT3(j, ITEM3_STAY_ZONE))) {

			// keys, unapp, zone only objects && fate portals
			if (IS_OBJ_TYPE(j, ITEM_KEY)) {	// skip keys still in zone
				z = zone_number(GET_OBJ_VNUM(j));
				if (((rm = where_obj(j)) && rm->zone->number == z)
					|| !obj_owner(j)) {
					continue;
				}
			}

			if (IS_OBJ_STAT3(j, ITEM3_STAY_ZONE)) {
				z = zone_number(GET_OBJ_VNUM(j));
				if ((rm = where_obj(j)) && rm->zone->number != z) {
					out_of_zone = 1;
				}
			}

			/* timer count down */
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;

			if (!GET_OBJ_TIMER(j) || out_of_zone) {

				if (j->carried_by)
					act("$p slowly fades out of existence.",
						FALSE, j->carried_by, j, 0, TO_CHAR);
				if (j->worn_by)
					act("$p disintegrates as you are wearing it.",
						FALSE, j->worn_by, j, 0, TO_CHAR);
				else if ((j->in_room != NULL) && (j->in_room->people)) {
					act("$p slowly fades out of existence.",
						TRUE, j->in_room->people, j, 0, TO_ROOM);
					act("$p slowly fades out of existence.",
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
						raise(SIGSEGV);
				}
				extract_obj(j);
			}
		}
	}
}
