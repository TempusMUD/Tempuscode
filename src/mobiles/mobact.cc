
/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: mobact.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <signal.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "char_class.h"
#include "vehicle.h"
#include "flow_room.h"
#include "guns.h"
#include "bomb.h"
#include "mobact.h"
#include "fight.h"
#include "actions.h"
#include "creature.h"
#include "screen.h"
#include "tmpstr.h"
#include "prog.h"

/* external structs */
void npc_steal(struct Creature *ch, struct Creature *victim);
int hunt_victim(struct Creature *ch);
void perform_tell(struct Creature *ch, struct Creature *vict, char *messg);
int CLIP_COUNT(struct obj_data *clip);
int tarrasque_fight(struct Creature *tarr);
int general_search(struct Creature *ch, struct special_search_data *srch,
	int mode);
int smart_mobile_move(struct Creature *ch, int dir);
bool perform_offensive_skill(Creature *ch, Creature *vict, int skill, int *return_flags);

ACMD(do_flee);
ACMD(do_sleeper);
ACMD(do_stun);
ACMD(do_circle);
ACMD(do_backstab);
ACMD(do_say);
ACMD(do_feign);
ACMD(do_hide);
ACMD(do_gen_comm);
ACMD(do_remove);
ACCMD(do_drop);
ACMD(do_load);
ACMD(do_battlecry);
ACMD(do_psidrain);
ACMD(do_fly);
ACMD(do_extinguish);
ACMD(do_pinch);
ACMD(do_combo);
ACMD(do_berserk);

// for mobile_activity
ACMD(do_repair);
ACCMD(do_get);
ACMD(do_wear);
ACCMD(do_wield);
ACMD(do_shoot);
ACMD(do_holytouch);
ACMD(do_medic);
ACMD(do_firstaid);
ACMD(do_bandage);
ACMD(do_activate);
ACMD(do_discharge);
ACMD(do_de_energize);


SPECIAL(thief);
SPECIAL(cityguard);
SPECIAL(hell_hunter);

SPECIAL(mob_read_script);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)
#define IS_RACIALLY_AGGRO(ch) \
(GET_RACE(ch) == RACE_GOBLIN || \
 GET_RACE(ch) == RACE_ALIEN_1 || \
 GET_RACE(ch) == RACE_ORC)
#define RACIAL_ATTACK(ch, vict) \
    ((GET_RACE(ch) == RACE_GOBLIN && GET_RACE(vict) == RACE_DWARF) || \
     (GET_RACE(ch) == RACE_ALIEN_1 && GET_RACE(vict) == RACE_HUMAN) || \
     (GET_RACE(ch) == RACE_ORC && GET_RACE(vict) == RACE_DWARF))

int update_iaffects(Creature * ch);

void
burn_update(void)
{

	struct Creature *ch;
	struct obj_data *obj = NULL;
	struct room_data *fall_to = NULL;
	struct special_search_data *srch = NULL;
	int dam = 0;
	int found = 0;
	int idx = 0;
	struct affected_type *af;
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		ch = *cit;

		if (ch->in_room == NULL || ch->getPosition() == POS_DEAD) {
			errlog("Updating a corpse in burn_update.(%s)",
				GET_NAME(ch) == NULL ? "NULL" : GET_NAME(ch));
			continue;
		}

		if (IS_AFFECTED_3(ch, AFF3_INST_AFF)) {
			if (update_iaffects(ch))
				continue;
		}

		if (!ch->numCombatants() && GET_MOB_WAIT(ch))
			GET_MOB_WAIT(ch) = MAX(0, GET_MOB_WAIT(ch) - FIRE_TICK);

		if ((IS_NPC(ch) && ZONE_FLAGGED(ch->in_room->zone, ZONE_FROZEN)) ||
			IS_AFFECTED_2(ch, AFF2_PETRIFIED))
			continue;

		if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)
			&& ch->getPosition() == POS_FLYING
			&& (!ch->in_room->dir_option[DOWN]
				|| !ch->in_room->isOpenAir()
				|| IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED))
			&& !NOGRAV_ZONE(ch->in_room->zone)) {
			send_to_char(ch, 
				"You are slammed to the ground by the inexhorable force of gravity!\r\n");
			act("$n is slammed to the ground by the inexhorable force of gravity!\r\n", TRUE, ch, 0, 0, TO_ROOM);
			ch->setPosition(POS_RESTING);
			WAIT_STATE(ch, 1);
			if (damage(NULL, ch, dice(6, 5), TYPE_FALLING, WEAR_RANDOM))
				continue;
		}

		// char is flying but unable to continue
		if (ch->getPosition() == POS_FLYING && !AFF_FLAGGED(ch, AFF_INFLIGHT)
			&& GET_LEVEL(ch) < LVL_AMBASSADOR
			&& (CHECK_SKILL(ch, SKILL_FLYING) < 30)) {
			send_to_char(ch, "You can no longer fly!\r\n");
			ch->setPosition(POS_STANDING);
		}
		// character is in open air
		if (ch->in_room->dir_option[DOWN] &&
			ch->getPosition() < POS_FLYING &&
			!IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED) &&
			(!ch->numCombatants() || !AFF_FLAGGED(ch, AFF_INFLIGHT)) &&
			ch->in_room->isOpenAir() &&
			!NOGRAV_ZONE(ch->in_room->zone) &&
			(!MOUNTED(ch) || !AFF_FLAGGED(MOUNTED(ch), AFF_INFLIGHT)) &&
			(fall_to = ch->in_room->dir_option[DOWN]->to_room) &&
			fall_to != ch->in_room) {

			if (AFF_FLAGGED(ch, AFF_INFLIGHT) && AWAKE(ch)
				&& !IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)) {
				send_to_char(ch, 
					"You realize you are about to fall and resume your flight!\r\n");
				ch->setPosition(POS_FLYING);
			} else {

				act("$n falls downward through the air!", TRUE, ch, 0, 0,
					TO_ROOM);
				act("You fall downward through the air!", TRUE, ch, 0, 0,
					TO_CHAR);

				char_from_room(ch);
				char_to_room(ch, fall_to);
				look_at_room(ch, ch->in_room, 0);
				act("$n falls in from above!", TRUE, ch, 0, 0, TO_ROOM);
				GET_FALL_COUNT(ch)++;

				if (!ch->in_room->isOpenAir() ||
					!ch->in_room->dir_option[DOWN] ||
					!(fall_to = ch->in_room->dir_option[DOWN]->to_room) ||
					fall_to == ch->in_room ||
					IS_SET(ch->in_room->dir_option[DOWN]->exit_info,
						EX_CLOSED)) {
					/* hit the ground */

					dam =
						dice(GET_FALL_COUNT(ch) * (GET_FALL_COUNT(ch) + 2),
						12);
					if ((SECT_TYPE(ch->in_room) >= SECT_WATER_SWIM
							&& SECT_TYPE(ch->in_room) <= SECT_UNDERWATER)
						|| SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER
						|| SECT_TYPE(ch->in_room) == SECT_PITCH_PIT
						|| SECT_TYPE(ch->in_room) == SECT_PITCH_SUB) {
						dam >>= 1;
						if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL))
							dam <<= 2;

						act("$n makes an enormous splash!", TRUE, ch, 0, 0, TO_ROOM);
						act("You make an enormous splash!", TRUE, ch, 0, 0, TO_CHAR);
					} else {
						act("$n hits the ground hard!", TRUE, ch, 0, 0,
							TO_ROOM);
						act("You hit the ground hard!", TRUE, ch, 0, 0,
							TO_CHAR);
                    }

					for (srch = ch->in_room->search, found = 0; srch;
						srch = srch->next) {
						if (SRCH_FLAGGED(srch, SRCH_TRIG_FALL)
							&& SRCH_OK(ch, srch)) {
							found = general_search(ch, srch, found);
						}
					}

					if (found == 2)
						continue;

					if (IS_MONK(ch))
						dam -= (GET_LEVEL(ch) * dam) / 100;

					ch->setPosition(POS_RESTING);

					if (dam
						&& damage(NULL, ch, dam, TYPE_FALLING, WEAR_RANDOM))
						continue;

					GET_FALL_COUNT(ch) = 0;
				}
			}
		}

		// comfortable rooms
		if (ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_COMFORT)) {
			GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 1);
			GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 1);
			GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 1);
			ch->checkPosition();
		}

		// regen
		if (AFF_FLAGGED(ch, AFF_REGEN) || IS_TROLL(ch) || IS_VAMPIRE(ch)) {
			GET_HIT(ch) =
				MIN(GET_MAX_HIT(ch),
				GET_HIT(ch) + 1 +
				(random_percentage_zero_low() * GET_CON(ch) / 125));
			ch->checkPosition();
		}

		// mana tap
		if (AFF3_FLAGGED(ch, AFF3_MANA_TAP))
			GET_MANA(ch) =
				MIN(GET_MAX_MANA(ch),
				GET_MANA(ch) + 1 + random_number_zero_low(GET_WIS(ch) >> 2));

		// mana leak
		if (AFF3_FLAGGED(ch, AFF3_MANA_LEAK))
			GET_MANA(ch) = MAX(0, GET_MANA(ch) - 
                               (1 + random_number_zero_low(GET_WIS(ch) >> 2)));

		// energy tap
		if (AFF3_FLAGGED(ch, AFF3_ENERGY_TAP))
			GET_MOVE(ch) =
				MIN(GET_MAX_MOVE(ch),
				GET_MOVE(ch) + 1 + random_number_zero_low(GET_CON(ch) >> 2));

		// energy leak
		if (AFF3_FLAGGED(ch, AFF3_ENERGY_LEAK))
			GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 
                               (1 + random_number_zero_low(GET_CON(ch) >> 2)));

		// nanite reconstruction
		if (affected_by_spell(ch, SKILL_NANITE_RECONSTRUCTION)) {
			bool obj_found = false;
			bool repaired = false;

			for (idx = 0;idx < NUM_WEARS;idx++) {
				obj = GET_IMPLANT(ch, idx);
				if (obj) {
					obj_found = true;
					if (GET_OBJ_MAX_DAM(obj) != -1
							&& GET_OBJ_DAM(obj) != -1
							&& GET_OBJ_DAM(obj) < GET_OBJ_MAX_DAM(obj)) {
						repaired = true;
						GET_OBJ_DAM(obj) = GET_OBJ_DAM(obj) + number(0, 1);
					}
				}
			}

			if (!obj_found || !repaired) {
				if (!obj_found)
					send_to_char(ch, "NOTICE: Implants not found.  Nanite reconstruction halted.\r\n");
				else
					send_to_char(ch, "NOTICE: Implant reconstruction complete.  Nanite reconstruction halted.\r\n");
				affect_from_char(ch, SKILL_NANITE_RECONSTRUCTION);
			}
		}

		// Signed the Unholy Compact - Soulless
		if (PLR2_FLAGGED(ch, PLR2_SOULLESS) &&
			ch->getPosition() == POS_SLEEPING && !random_fractional_5()) {
			send_to_char(ch, 
				"The torturous cries of hell wake you from your dreams.\r\n");
			act("$n bolts upright, screaming in agony!", TRUE, ch, 0, 0,
				TO_ROOM);
			ch->setPosition(POS_SITTING);
		}
		// affected by sleep spell
		if (AFF_FLAGGED(ch, AFF_SLEEP) && ch->getPosition() > POS_SLEEPING
			&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
			send_to_char(ch, "You suddenly fall into a deep sleep.\r\n");
			act("$n suddenly falls asleep where $e stands.", TRUE, ch, 0, 0,
				TO_ROOM);
			ch->setPosition(POS_SLEEPING);
		}
		// self destruct
		if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
			if (MEDITATE_TIMER(ch)) {
				send_to_char(ch, "Self-destruct T-minus %d and counting.\r\n",
					MEDITATE_TIMER(ch));
				MEDITATE_TIMER(ch)--;
			} else {
				if (!IS_CYBORG(ch)) {
					errlog("%s tried to self destruct at [%d].",
						GET_NAME(ch), ch->in_room->number);
					REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
				} else {
					engage_self_destruct(ch);
					continue;
				}
			}
		}
		// character is poisoned (3)
		if (HAS_POISON_3(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
			if (damage(ch, ch, dice(4, 3) + (affected_by_spell(ch,
							SPELL_METABOLISM) ? dice(4, 11) : 0), SPELL_POISON,
					-1))
				continue;
		}
		// Gravity Well
		if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)
			&& !random_fractional_10()
			&& !NOGRAV_ZONE(ch->in_room->zone)) {
			af = affected_by_spell(ch, SPELL_GRAVITY_WELL);
			if (!af || mag_savingthrow(ch, af->level, SAVING_PHY))
				continue;
			if (damage(ch, ch, number(5, af->level / 5), TYPE_PRESSURE, -1))
				continue;
		}
		// psychic crush
		if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH)) {
			if (damage(ch, ch, mag_savingthrow(ch, 50,
						SAVING_PSI) ? 0 : dice(4, 20), SPELL_PSYCHIC_CRUSH,
					WEAR_HEAD))
				continue;
		}
		// character has a stigmata
		if ((af = affected_by_spell(ch, SPELL_STIGMATA))) {
			if (damage(ch, ch, mag_savingthrow(ch, af->level,
						SAVING_SPELL) ? 0 : dice(3, af->level), SPELL_STIGMATA,
					WEAR_FACE))
				continue;
		}

		// character has entropy field
		if ((af = affected_by_spell(ch, SPELL_ENTROPY_FIELD))
				&& !random_fractional_10()
				&& !mag_savingthrow(ch, af->level, SAVING_PHY)) {
			GET_MANA(ch) = MAX(0, GET_MANA(ch) - 
							   (13 - random_number_zero_low(GET_WIS(ch) >> 2)));
			GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 
							   (13 - random_number_zero_low(GET_STR(ch) >> 2)));
			if (damage(ch, ch, (13 - random_number_zero_low(GET_CON(ch) >> 2)),
					SPELL_ENTROPY_FIELD, -1))
				continue;

		}

		// character has acidity
		if (AFF3_FLAGGED(ch, AFF3_ACIDITY)) {
			if (damage(ch, ch, mag_savingthrow(ch, 50,
						SAVING_PHY) ? 0 : dice(2, 10), TYPE_ACID_BURN, -1))
				continue;
		}
		// motor spasm
		if ((af = affected_by_spell(ch, SPELL_MOTOR_SPASM))
			&& !MOB_FLAGGED(ch, MOB_NOBASH) && ch->getPosition() < POS_FLYING
			&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
			if ((random_number_zero_low(3 + (af->level >> 2)) + 3) >
				GET_DEX(ch) && (obj = ch->carrying)) {
				while (obj) {
					if (can_see_object(ch, obj) && !IS_OBJ_STAT(obj, ITEM_NODROP))
						break;
					obj = obj->next_content;
				}
				if (obj) {
					send_to_char(ch, 
						"Your muscles are seized in an uncontrollable spasm!\r\n");
					act("$n begins spasming uncontrollably.", TRUE, ch, 0, 0,
						TO_ROOM);
					do_drop(ch, fname(obj->aliases), 0, SCMD_DROP, 0);
				}
			}
			if (!obj
				&& random_number_zero_low(12 + (af->level >> 2)) > GET_DEX(ch)
				&& ch->getPosition() > POS_SITTING) {
				send_to_char(ch, 
					"Your muscles are seized in an uncontrollable spasm!\r\n"
					"You fall to the ground in agony!\r\n");
				act("$n begins spasming uncontrollably and falls to the ground.", TRUE, ch, 0, 0, TO_ROOM);
				ch->setPosition(POS_RESTING);
			}
			WAIT_STATE(ch, 4);
		}
		// hyperscanning increment
		if ((af = affected_by_spell(ch, SKILL_HYPERSCAN))) {
			if (GET_MOVE(ch) < 10) {
				send_to_char(ch, "Hyperscanning device shutting down.\r\n");
				affect_remove(ch, af);
			} else
				GET_MOVE(ch) -= 1;
		}
		// Soulless Goodie Two Shoes - Unholy Compact sellouts.
		if (PLR2_FLAGGED(ch, PLR2_SOULLESS) && IS_GOOD(ch)
			&& random_fractional_10()) {
			int thedam;
			thedam = dice(2, 3);
			if (GET_HIT(ch) + 1 > thedam)
				if (damage(ch, ch, thedam, TYPE_ANGUISH, -1))
					continue;
		}
		// burning character
		if (IS_AFFECTED_2(ch, AFF2_ABLAZE)) {
			if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
				SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN ||
				SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
				SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM) {
				send_to_char(ch, 
					"The flames on your body sizzle out and die, leaving you in a cloud of steam.\r\n");
				act("The flames on $n sizzle and die, leaving a cloud of steam.", FALSE, ch, 0, 0, TO_ROOM);
				REMOVE_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
			}
			// 
			// Sect types that don't have oxygen
			//

			if (				// SECT_TYPE( ch->in_room ) == SECT_ELEMENTAL_EARTH || 
				SECT_TYPE(ch->in_room) == SECT_FREESPACE) {
				send_to_char(ch, 
					"The flames on your body die in the absence of oxygen.\r\n");
				act("The flames on $n die in the absence of oxygen.", FALSE,
					ch, 0, 0, TO_ROOM);
				REMOVE_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
			} else if (!random_fractional_3() && !CHAR_WITHSTANDS_FIRE(ch)) {
				if (damage(ch, ch,
						CHAR_WITHSTANDS_FIRE(ch) ? 0 :
						ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) ? dice(8,
							7) : dice(5, 5), TYPE_ABLAZE, -1))
					continue;
				if (IS_MOB(ch) && ch->getPosition() >= POS_RESTING &&
					!GET_MOB_WAIT(ch) && !CHAR_WITHSTANDS_FIRE(ch))
					do_extinguish(ch, "", 0, 0, 0);
			}
		}
		// burning rooms
		else if ((ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) &&
				(!CHAR_WITHSTANDS_FIRE(ch) ||
					ROOM_FLAGGED(ch->in_room, ROOM_GODROOM))) ||
			(IS_VAMPIRE(ch) && OUTSIDE(ch) &&
				ch->in_room->zone->weather->sunlight == SUN_LIGHT &&
				GET_PLANE(ch->in_room) < PLANE_ASTRAL)) {
			send_to_char(ch, "Your body suddenly bursts into flames!\r\n");
			act("$n suddenly bursts into flames!", FALSE, ch, 0, 0, TO_ROOM);
			GET_MANA(ch) = 0;
			SET_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
			if (damage(ch, ch, dice(4, 5), TYPE_ABLAZE, -1))
				continue;
		}
		// freezing room
		else if (ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD)
			&& !CHAR_WITHSTANDS_COLD(ch)) {
			if (damage(NULL, ch, dice(4, 5), TYPE_FREEZING, -1))
				continue;
			if (IS_MOB(ch) && (IS_CLERIC(ch) || IS_RANGER(ch))
				&& GET_LEVEL(ch) > 15)
				cast_spell(ch, ch, 0, SPELL_ENDURE_COLD);

		}
		// holywater ocean
		else if (ROOM_FLAGGED(ch->in_room, ROOM_HOLYOCEAN) && IS_EVIL(ch)
			&& ch->getPosition() < POS_FLYING) {
			if (damage(ch, ch, dice(4, 5), TYPE_HOLYOCEAN, WEAR_RANDOM))
				continue;
			if (IS_MOB(ch) && !ch->numCombatants()) {
				do_flee(ch, "", 0, 0, 0);
			}
		}
		// boiling pitch
		else if ((ch->in_room->sector_type == SECT_PITCH_PIT
				|| ch->in_room->sector_type == SECT_PITCH_SUB)
			&& !CHAR_WITHSTANDS_HEAT(ch)) {
			if (damage(ch, ch, dice(4, 3), TYPE_BOILING_PITCH, WEAR_RANDOM))
				continue;
		}
		// radioactive room
		if (ROOM_FLAGGED(ch->in_room, ROOM_RADIOACTIVE)
			&& !CHAR_WITHSTANDS_RAD(ch)) {

			if (affected_by_spell(ch, SKILL_RADIONEGATION)) {

				GET_HIT(ch) = MAX(1, GET_HIT(ch) - dice(1, 7));

				if (GET_MOVE(ch) > 10) {
					GET_MOVE(ch) = MAX(10, GET_MOVE(ch) - dice(2, 7));
				}

				else {
					add_rad_sickness(ch, 10);
				}
			}

			else {

				GET_HIT(ch) = MAX(1, GET_HIT(ch) - dice(2, 7));

				if (GET_MOVE(ch) > 5)
					GET_MOVE(ch) = MAX(5, GET_MOVE(ch) - dice(2, 7));

				add_rad_sickness(ch, 20);

			}
		}

		if (SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN) {
			if (damage(ch, ch, dice(8, 10), TYPE_CRUSHING_DEPTH, -1))
				continue;
		}
		// underwater (underliquid)
		if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
				SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN ||
				SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
				SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM ||
				SECT_TYPE(ch->in_room) == SECT_FREESPACE
				// || SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_EARTH
			) &&
			!can_travel_sector(ch, SECT_TYPE(ch->in_room), 1) &&
			!ROOM_FLAGGED(ch->in_room, ROOM_DOCK) &&
			GET_LEVEL(ch) < LVL_AMBASSADOR) {

			int drown_factor = ch->getBreathCount() - ch->getBreathThreshold();
			int type = 0;

			drown_factor = MAX(0, drown_factor);


			if (SECT_TYPE(ch->in_room) == SECT_FREESPACE ||
				SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_EARTH) {
				type = TYPE_SUFFOCATING;
			}

			else {
				type = TYPE_DROWNING;
			}
			if (damage(ch, ch, dice(4, 5), type, -1))
				continue;

			if (AFF_FLAGGED(ch, AFF_INFLIGHT) &&
				ch->getPosition() < POS_FLYING &&
				SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)
				do_fly(ch, "", 0, 0, 0);
		}
		// sleeping gas
		if (ROOM_FLAGGED(ch->in_room, ROOM_SLEEP_GAS) &&
			ch->getPosition() > POS_SLEEPING
			&& !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
			send_to_char(ch, "You feel very sleepy...\r\n");
			if (!AFF_FLAGGED(ch, AFF_ADRENALINE)) {
				if (!mag_savingthrow(ch, 50, SAVING_CHEM)) {
					send_to_char(ch, 
						"You suddenly feel very sleepy and collapse where you stood.\r\n");
					act("$n suddenly falls asleep and collapses!", TRUE, ch, 0,
						0, TO_ROOM);
					ch->setPosition(POS_SLEEPING);
					WAIT_STATE(ch, 4 RL_SEC);
					continue;
				} else
					send_to_char(ch, "You fight off the effects.\r\n");
			}
		}
		// poison gas
		if (ROOM_FLAGGED(ch->in_room, ROOM_POISON_GAS) &&
			!PRF_FLAGGED(ch, PRF_NOHASSLE) && !IS_UNDEAD(ch) &&
			!AFF3_FLAGGED(ch, AFF3_NOBREATHE)) {

			if (!HAS_POISON_3(ch)) {
				send_to_char(ch, 
					"Your lungs burn as you inhale a poisonous gas!\r\n");
				act("$n begins choking and sputtering!", FALSE, ch, 0, 0,
					TO_ROOM);
			}

			found = 1;
			if (HAS_POISON_2(ch))
				call_magic(ch, ch, NULL, SPELL_POISON, 60, CAST_CHEM);
			else if (HAS_POISON_1(ch))
				call_magic(ch, ch, NULL, SPELL_POISON, 39, CAST_CHEM);
			else if (!HAS_POISON_3(ch))
				call_magic(ch, ch, NULL, SPELL_POISON, 10, CAST_CHEM);
			else
				found = 0;

			if (found)
				continue;
		}
		//
		// animated corpses decaying
		//

		if (IS_NPC(ch) && GET_MOB_VNUM(ch) == ZOMBIE_VNUM &&
			OUTSIDE(ch) && room_is_light(ch->in_room)
			&& PRIME_MATERIAL_ROOM(ch->in_room))
			if (damage(ch, ch, dice(4, 5), TOP_SPELL_DEFINE, -1))
				continue;

		/* Hunter Mobs */
		if (HUNTING(ch) && !AFF_FLAGGED(ch, AFF_BLIND) &&
			ch->getPosition() > POS_SITTING && !GET_MOB_WAIT(ch)) {
			if (MOB_FLAGGED(ch, MOB_WIMPY)
				&& (GET_HIT(ch) < MIN(500, GET_MAX_HIT(ch)) * 0.80)
				|| (100 - ((GET_HIT(ch) * 100) / GET_MAX_HIT(ch))) >
				GET_MORALE(ch) + number(-5, 10 + (GET_INT(ch) >> 2))) {
				if (ch->in_room == HUNTING(ch)->in_room)
					do_flee(ch, "", 0, 0, 0);
			} else
				hunt_victim(ch);
			continue;
		}

		if (GET_MOB_PROG(ch) && !ch->numCombatants())
			trigger_prog_idle(ch);
	}
}

//
// return a measure of how willing ch is to help vict in a fight
// <= 0 return value means ch will not help vict
//

inline int
helper_help_probability(struct Creature *ch, struct Creature *vict)
{

	int prob = GET_MORALE(ch);

	if (!IS_MOB(ch) || !vict)
		return 0;

	//
	// newbie bonus!
	//

	if (!IS_MOB(vict) && GET_LEVEL(vict) < 6) {
		prob += GET_LEVEL(ch);
	}
	//
	// don't help ppl on our shitlist
	//

	if (char_in_memory(vict, ch))
		return 0;

	//
	// bonus if vict is leader
	//

	if (IS_MOB(vict) && GET_MOB_LEADER(ch) == GET_MOB_VNUM(vict)) {
		prob += GET_LEVEL(vict) * 2;
	}
	//
	// bonus if vict is follower
	//

	if (IS_MOB(vict) && GET_MOB_LEADER(vict) == GET_MOB_VNUM(ch)) {
		prob += GET_LEVEL(vict);
	}
	//
	// bonus for same race
	//

	if (GET_RACE(ch) == GET_RACE(vict)) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// bonus for same class
	//

	if (GET_CLASS(ch) == GET_CLASS(vict)) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// bonus for same alignment for good and neutral chars
	//

	if ((IS_GOOD(ch) && IS_GOOD(vict)) || (IS_NEUTRAL(ch) && IS_NEUTRAL(vict))) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// penalty for good chars helping evil, or evil helping good
	//

	if ((IS_GOOD(ch) && IS_EVIL(vict)) || (IS_EVIL(ch) && IS_GOOD(vict))) {
		prob -= GET_LEVEL(vict);
	}
	//
	// aggro align penalty
	//

	if ((MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
		(MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)) ||
		(MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict))) {
		prob -= GET_LEVEL(vict);
	}
	//
	// racial hatred penalty
	//

	if (RACIAL_ATTACK(ch, vict)) {
		prob -= GET_LEVEL(vict);
	}

	if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
		prob += GET_LEVEL(vict);

	return prob;
}


//
// return a measure of how willing ch is to attack vict
// <= 0 return value means ch will not attack vict
//

inline int
helper_attack_probability(struct Creature *ch, struct Creature *vict)
{

	int prob = GET_MORALE(ch);

	if (!IS_MOB(ch))
		return 0;

	//
	// don't attack newbies
	//

	if (!IS_MOB(vict) && GET_LEVEL(vict) < 6)
		return 0;

	//
	// don't attack other mobs unless flagged
	//

	if (!MOB2_FLAGGED(ch, MOB2_ATK_MOBS) && IS_MOB(vict))
		return 0;

	//
	// dont attack same race if prohibited
	//

	if (MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) && GET_RACE(ch) == GET_RACE(vict))
		return 0;

	//
	// don't attack your leader
	//

	if (IS_MOB(vict) && GET_MOB_LEADER(ch) == GET_MOB_VNUM(vict))
		return 0;

	//
	// don't attack your followers
	//

	if (IS_MOB(vict) && GET_MOB_LEADER(vict) == GET_MOB_VNUM(ch))
		return 0;

	//
	// memory bonus
	//

	if (char_in_memory(vict, ch)) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// racial hatred bonus
	//

	if (RACIAL_ATTACK(ch, vict)) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// aggro bonus
	//

	if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
		prob += GET_MORALE(ch) / 2;

	//
	// aggro align bonus
	//

	if ((MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
		(MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)) ||
		(MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict))) {
		prob += GET_MORALE(ch) / 2;
	}
	//
	// level penalty
	//
	// range from -100 -> 0 with level difference
	//

	prob += (GET_LEVEL(ch) - GET_LEVEL(vict)) * 2;

	//
	// penalty for same race
	//

	if (GET_RACE(vict) == GET_RACE(ch))
		prob -= GET_LEVEL(vict);

	//
	// penalty for same class
	//

	if (GET_CLASS(vict) == GET_CLASS(ch))
		prob -= GET_LEVEL(vict);

	//
	// penalty for same align if positive alignment
	//

	if (GET_ALIGNMENT(ch) > 0) {
		prob -= GET_ALIGNMENT(vict) / 10;
	}
	//
	// bonus for opposite align if negative alignment
	//

	else if (GET_ALIGNMENT(vict) > 0) {
		// bonus ranges from 0 -> 50 with alignment 0 -> 1000
		prob += (GET_ALIGNMENT(vict) / 20);
	}

	if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
		prob -= GET_LEVEL(vict);

	return prob;
}

//
// make ch help fvict by attacking vict
//
// return values are the same as damage()
//

int
helper_assist(struct Creature *ch, struct Creature *vict,
	struct Creature *fvict)
{

	int my_return_flags = 0;

	int prob = 0;
	struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

	if (!ch || !vict || !fvict) {
		errlog("Illegal value(s) passed to helper_assist().");
		return 0;
	}

	if (GET_RACE(ch) == GET_RACE(fvict))
		prob += 10 + (GET_LEVEL(ch) >> 1);
	else if (GET_RACE(ch) != GET_RACE(vict))
		prob -= 10;
	else if (IS_DEVIL(ch) || IS_KNIGHT(ch) || IS_WARRIOR(ch) || IS_RANGER(ch))
		prob += GET_LEVEL(ch);

	if (IS_ANIMAL(ch) && affected_by_spell(fvict, SPELL_ANIMAL_KIN))
		prob += GET_LEVEL(vict);

	if (weap) {
		if (IS_THIEF(ch) && GET_LEVEL(ch) > 43 &&
			(GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT ||
				GET_OBJ_VAL(weap, 3) == TYPE_STAB - TYPE_HIT)) {
			do_circle(ch, GET_NAME(vict), 0, 0, &my_return_flags);
			return my_return_flags;
		}

		if (IS_ENERGY_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
			EGUN_CUR_ENERGY(weap) > 10) {
			CUR_R_O_F(weap) = MAX_R_O_F(weap);
			do_shoot(ch, tmp_sprintf("%s %s", fname(weap->aliases),
					fname(vict->player.name)),
				0, 0, &my_return_flags);
			return my_return_flags;
		}

		if (IS_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
			GUN_LOADED(weap)) {
			CUR_R_O_F(weap) = MAX_R_O_F(weap);
			sprintf(buf, "%s ", fname(weap->aliases));
			strcat(buf, fname(vict->player.name));
			do_shoot(ch, buf, 0, 0, &my_return_flags);
			return my_return_flags;
		}
	}

	act("$n jumps to the aid of $N!", TRUE, ch, 0, fvict, TO_NOTVICT);
	act("$n jumps to your aid!", TRUE, ch, 0, fvict, TO_VICT);

	if (prob > random_percentage())
		vict->removeCombat(fvict);

	return hit(ch, vict, TYPE_UNDEFINED);
}

void
mob_load_unit_gun(struct Creature *ch, struct obj_data *clip,
	struct obj_data *gun, bool internal)
{
	char loadbuf[1024];
	sprintf(loadbuf, "%s %s", fname(clip->aliases), internal ? "internal " : "");
	strcat(loadbuf, fname(gun->aliases));
	do_load(ch, loadbuf, 0, SCMD_LOAD, 0);
	if (GET_MOB_VNUM(ch) == 1516 && IS_CLIP(clip) && random_binary())
		do_say(ch, "Let's Rock.", 0, 0, 0);
	return;
}

struct obj_data *
find_bullet(struct Creature *ch, int gun_type, struct obj_data *list)
{
	struct obj_data *bul = NULL;

	for (bul = list; bul; bul = bul->next_content) {
		if (can_see_object(ch, bul) &&
			IS_BULLET(bul) && GUN_TYPE(bul) == gun_type)
			return (bul);
	}
	return NULL;
}

void
mob_reload_gun(struct Creature *ch, struct obj_data *gun)
{
	bool internal = false;
	int count = 0, i;
	struct obj_data *clip = NULL, *bul = NULL, *cont = NULL;

	if (GET_INT(ch) < (random_number_zero_low(4) + 3))
		return;

	if (gun->worn_by && gun != GET_EQ(ch, gun->worn_on))
		internal = true;

	if (IS_GUN(gun)) {
		if (!MAX_LOAD(gun)) {	/** a gun that uses a clip **/
			if (gun->contains) {
				sprintf(buf, "%s%s", internal ? "internal " : "",
					fname(gun->aliases));
				do_load(ch, buf, 0, SCMD_UNLOAD, 0);
			}

			/** look for a clip that matches the gun **/
			for (clip = ch->carrying; clip; clip = clip->next_content)
				if (IS_CLIP(clip) && GUN_TYPE(clip) == GUN_TYPE(gun))
					break;

			if (!clip)
				return;

			if ((count = CLIP_COUNT(clip)) >= MAX_LOAD(clip)) {	 /** a full clip **/
				mob_load_unit_gun(ch, clip, gun, internal);
				return;
			}

			/** otherwise look for a bullet to load into the clip **/
			if ((bul = find_bullet(ch, GUN_TYPE(gun), ch->carrying))) {
				mob_load_unit_gun(ch, bul, clip, FALSE);
				count++;
				if (count >= MAX_LOAD(clip))	  /** load full clip in gun **/
					mob_load_unit_gun(ch, clip, gun, internal);
				return;
			}

			/* no bullet found, look for one in bags * */
			for (cont = ch->carrying; cont; cont = cont->next_content) {
				if (!can_see_object(ch, cont)
					|| !IS_OBJ_TYPE(cont, ITEM_CONTAINER) || CAR_CLOSED(cont))
					continue;
				if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
					sprintf(buf, "%s ", fname(bul->aliases));
					strcat(buf, fname(cont->aliases));
					do_get(ch, buf, 0, 0, 0);
					mob_load_unit_gun(ch, bul, clip, FALSE);
					count++;
					if (count >= MAX_LOAD(clip))	  /** load full clip in gun **/
						mob_load_unit_gun(ch, clip, gun, internal);
					return;
				}
			}
			for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
				if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER)
					&& !CAR_CLOSED(cont)
					&& (bul =
						find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
					sprintf(buf, "%s ", fname(bul->aliases));
					strcat(buf, fname(cont->aliases));
					do_get(ch, buf, 0, 0, 0);
					mob_load_unit_gun(ch, bul, clip, FALSE);
					count++;
					if (count >= MAX_LOAD(clip))	  /** load full clip in gun **/
						mob_load_unit_gun(ch, clip, gun, internal);
					return;
				}
			}

			/** might as well load it now **/
			if (count > 0)
				mob_load_unit_gun(ch, clip, gun, internal);

			return;
		}
		/************************************/
		/** a gun that does not use a clip **/
		if ((bul = find_bullet(ch, GUN_TYPE(gun), ch->carrying))) {
			mob_load_unit_gun(ch, bul, gun, internal);
			return;
		}

		/* no bullet found, look for one in bags * */
		for (cont = ch->carrying; cont; cont = cont->next_content) {
			if (!can_see_object(ch, cont) || !IS_OBJ_TYPE(cont, ITEM_CONTAINER) ||
				CAR_CLOSED(cont))
				continue;
			if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
				sprintf(buf, "%s ", fname(bul->aliases));
				strcat(buf, fname(cont->aliases));
				do_get(ch, buf, 0, 0, 0);
				mob_load_unit_gun(ch, bul, gun, internal);
			}
		}
		for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
			if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER) &&
				!CAR_CLOSED(cont) &&
				(bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
				sprintf(buf, "%s ", fname(bul->aliases));
				strcat(buf, fname(cont->aliases));
				do_get(ch, buf, 0, 0, 0);
				mob_load_unit_gun(ch, bul, gun, internal);
				return;
			}
		}
		return;
	}
	/** energy guns **/
}

//
// return values are the same as damage()
//
//

bool
check_infiltrate(struct Creature *ch, struct Creature *vict)
{
	if (!ch || !vict) {
		errlog("NULL char in check_infiltrate()!");
		return false;
	}

	int prob = ch->getLevelBonus(SKILL_INFILTRATE);
	int percent = number(1, 115);

    if (PLR_FLAGGED(ch, PLR_KILLER)) {
        return false;
    }
    
	if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_SPIRIT_TRACKER) &&
		char_in_memory(ch, vict))
		return false;

	if (!IS_AFFECTED_3(ch, AFF3_INFILTRATE) && !IS_AFFECTED(ch, AFF_HIDE))
		return false;

	if (affected_by_spell(vict, ZEN_AWARENESS) ||
		IS_AFFECTED_2(vict, AFF2_TRUE_SEEING))
		percent += 17;

	if (IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))
		prob += 17;

	if (vict->getPosition() <= POS_FIGHTING)
		prob += 10;

	if (IS_AFFECTED_2(ch, AFF2_HASTE))
		prob += 15;

	if (IS_AFFECTED_2(vict, AFF2_HASTE))
		percent += 15;

	percent += (vict->getLevelBonus(SKILL_INFILTRATE) / 2);

	if (ch && PRF2_FLAGGED(ch, PRF2_DEBUG))
		send_to_char(ch, "%s[INFILTRATE] chance:%d   roll:%d%s\r\n",
			CCCYN(ch, C_NRM), prob, percent, CCNRM(ch, C_NRM));
	
	return prob > percent;
}

int
best_attack(struct Creature *ch, struct Creature *vict)
{

	struct obj_data *gun = GET_EQ(ch, WEAR_WIELD);
	int cur_class = 0;

	int return_flags = 0;

	if (ch->getPosition() < POS_STANDING) {
		if (!IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)
			|| number(1, 20) < GET_STR(ch)) {
			act("$n jumps to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
			ch->setPosition(POS_STANDING);
		}
		GET_MOB_WAIT(ch) += PULSE_VIOLENCE;
	}
	if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_fractional_3())
		cur_class = GET_REMORT_CLASS(ch);
	else
		cur_class = GET_CLASS(ch);

	//
	//
	//

	if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {

		if (((gun = GET_EQ(ch, WEAR_WIELD)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_EQ(ch, WEAR_WIELD_2)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_IMPLANT(ch, WEAR_WIELD)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_IMPLANT(ch, WEAR_WIELD_2)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun))))) {

			CUR_R_O_F(gun) = MAX_R_O_F(gun);

			sprintf(buf, "%s ", fname(gun->aliases));
			strcat(buf, fname(vict->player.name));
			do_shoot(ch, buf, 0, 0, &return_flags);
			return return_flags;
		}
	}
	//
	// thief mobs
	//
	if (cur_class == CLASS_THIEF) {
		if (GET_LEVEL(ch) >= 35 && vict->getPosition() > POS_STUNNED)
			do_stun(ch, fname(vict->player.name), 0, 0, 0);

		else if (((gun = GET_EQ(ch, WEAR_WIELD)) && STAB_WEAPON(gun)) ||
			((gun = GET_EQ(ch, WEAR_WIELD_2)) && STAB_WEAPON(gun)) ||
			((gun = GET_EQ(ch, WEAR_HANDS)) && STAB_WEAPON(gun))) {

			if (!vict->numCombatants())
				do_backstab(ch, fname(vict->player.name), 0, 0, &return_flags);
			else if (GET_LEVEL(ch) > 43)
				do_circle(ch, fname(vict->player.name), 0, 0, &return_flags);
			else if (GET_LEVEL(ch) >= 30)
				perform_offensive_skill(ch, vict, SKILL_GOUGE, &return_flags);
			else {
				return_flags = hit(ch, vict, TYPE_UNDEFINED);
			}
		} else {
			if (GET_LEVEL(ch) >= 30) {
				perform_offensive_skill(ch, vict, SKILL_GOUGE, &return_flags);
			} else {
				return_flags = hit(ch, vict, TYPE_UNDEFINED);
			}
		}
		return return_flags;
	}
	//
	// psionic mobs
	//

	if (cur_class == CLASS_PSIONIC && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
		if (GET_LEVEL(ch) >= 33 &&
			AFF3_FLAGGED(vict, AFF3_PSISHIELD) &&
			GET_MANA(ch) > mag_manacost(ch, SPELL_PSIONIC_SHATTER)) {
			cast_spell(ch, vict, NULL, SPELL_PSIONIC_SHATTER, &return_flags);
			return return_flags;
		}

		else if (vict->getPosition() > POS_SLEEPING) {
			if (GET_LEVEL(ch) >= 40 &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_PSYCHIC_SURGE)) {
				cast_spell(ch, vict, NULL, SPELL_PSYCHIC_SURGE, &return_flags);
				return return_flags;
			} else if (GET_LEVEL(ch) >= 18 &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_MELATONIC_FLOOD)) {
				cast_spell(ch, vict, NULL, SPELL_MELATONIC_FLOOD,
					&return_flags);
				return return_flags;
			}
		}

		if (GET_LEVEL(ch) >= 36 &&
			(IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
				IS_KNIGHT(vict) || IS_PHYSIC(vict)) &&
			!IS_CONFUSED(vict) &&
			GET_MANA(ch) > mag_manacost(ch, SPELL_CONFUSION)) {
			cast_spell(ch, vict, NULL, SPELL_CONFUSION, &return_flags);
			return return_flags;
		} else if (GET_LEVEL(ch) >= 31 &&
			!AFF2_FLAGGED(ch, AFF2_VERTIGO) &&
			GET_MANA(ch) > mag_manacost(ch, SPELL_VERTIGO)) {
			cast_spell(ch, vict, NULL, SPELL_VERTIGO, &return_flags);
			return return_flags;
		}

	}
	//
	// mage mobs
	//

	if (cur_class == CLASS_MAGE && GET_MANA(ch) > 100 &&
		!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
		if (GET_LEVEL(ch) >= 37 && vict->getPosition() > POS_SLEEPING)
			cast_spell(ch, vict, NULL, SPELL_WORD_STUN, &return_flags);
		else if (GET_LEVEL(ch) < 5)
			cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, &return_flags);
		else if (GET_LEVEL(ch) < 9 && !CHAR_WITHSTANDS_COLD(vict))
			cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, &return_flags);
		else if (GET_LEVEL(ch) < 11)
			cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, &return_flags);
		else if (GET_LEVEL(ch) < 15 && !CHAR_WITHSTANDS_FIRE(vict))
			cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, &return_flags);
		else if (GET_LEVEL(ch) < 18 || IS_CYBORG(vict))
			cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, &return_flags);
		else if (GET_LEVEL(ch) < 31)
			cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, &return_flags);
		else if (GET_LEVEL(ch) < 36)
			cast_spell(ch, vict, NULL, SPELL_FIREBALL, &return_flags);
		else if (GET_LEVEL(ch) < 42)
			cast_spell(ch, vict, NULL, SPELL_PRISMATIC_SPRAY, &return_flags);
		else {
			return_flags = hit(ch, vict, TYPE_UNDEFINED);
		}
		return return_flags;
	}
	//
	// barb and warrior mobs
	//

	if (cur_class == CLASS_BARB || cur_class == CLASS_WARRIOR) {
		if (GET_LEVEL(ch) >= 25 && vict->getPosition() > POS_SITTING)
			perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
		else
			return_flags = hit(ch, vict, TYPE_UNDEFINED);
		return return_flags;
	}
	//
	// monk mobs
	//

	if (cur_class == CLASS_MONK) {
		if (GET_LEVEL(ch) >= 39 && vict->getPosition() > POS_STUNNED) {
			sprintf(buf, "%s omega", fname(vict->player.name));
			do_pinch(ch, buf, 0, 0, &return_flags);
			return return_flags;
		}
		// Screw them up.
		if (vict->getPosition() <= POS_STUNNED) {
			if (!affected_by_spell(vict, SKILL_PINCH_GAMMA)
				&& (GET_LEVEL(ch) > 35)) {
				sprintf(buf, "%s gamma", fname(vict->player.name));
				do_pinch(ch, buf, 0, 0, &return_flags);
			} else if (!affected_by_spell(vict, SKILL_PINCH_EPSILON)
				&& (GET_LEVEL(ch) > 26)) {
				sprintf(buf, "%s epsilon", fname(vict->player.name));
				do_pinch(ch, buf, 0, 0, &return_flags);
			} else if (!affected_by_spell(vict, SKILL_PINCH_DELTA)
				&& (GET_LEVEL(ch) > 19)) {
				sprintf(buf, "%s delta", fname(vict->player.name));
				do_pinch(ch, buf, 0, 0, &return_flags);
			} else if (!affected_by_spell(vict, SKILL_PINCH_BETA)
				&& (GET_LEVEL(ch) > 12)) {
				sprintf(buf, "%s beta", fname(vict->player.name));
				do_pinch(ch, buf, 0, 0, &return_flags);
			} else if (!affected_by_spell(vict, SKILL_PINCH_ALPHA)
				&& (GET_LEVEL(ch) > 6)) {
				sprintf(buf, "%s alpha", fname(vict->player.name));
				do_pinch(ch, buf, 0, 0, &return_flags);
			} else if (GET_LEVEL(ch) >= 33) {
				do_combo(ch, fname(vict->player.name), 0, 0, &return_flags);
			} else if (GET_LEVEL(ch) >= 30) {
				perform_offensive_skill(ch, vict, SKILL_RIDGEHAND, &return_flags);
			} else {
				return_flags = hit(ch, vict, TYPE_UNDEFINED);
			}
		}
		return return_flags;
	}
	//
	// dragon mobs
	//

	if (IS_DRAGON(ch) && random_fractional_4()) {
		switch (GET_CLASS(ch)) {
		case CLASS_GREEN:
			if (random_fractional_3()) {
				call_magic(ch, vict, 0, SPELL_GAS_BREATH, GET_LEVEL(ch),
					CAST_BREATH, &return_flags);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;
		case CLASS_BLACK:
			if (random_fractional_3()) {
				call_magic(ch, vict, 0, SPELL_ACID_BREATH, GET_LEVEL(ch),
					CAST_BREATH, &return_flags);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;
		case CLASS_BLUE:
			if (random_fractional_3()) {
				call_magic(ch, vict, 0,
					SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH,
					&return_flags);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;
		case CLASS_WHITE:
		case CLASS_SILVER:
			if (random_fractional_3()) {
				call_magic(ch, vict, 0, SPELL_FROST_BREATH, GET_LEVEL(ch),
					CAST_BREATH, &return_flags);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;
		case CLASS_RED:
			if (random_fractional_3()) {
				call_magic(ch, vict, 0, SPELL_FIRE_BREATH, GET_LEVEL(ch),
					CAST_BREATH, &return_flags);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;
		default:
			return hit(ch, vict, TYPE_UNDEFINED);
			break;
		}
		return return_flags;
	}

	return hit(ch, vict, TYPE_UNDEFINED);

}

#define ELEMENTAL_LIKES_ROOM(ch, room)  \
(!IS_ELEMENTAL(ch) ||                \
 (GET_CLASS(ch) == CLASS_WATER &&            \
  (SECT_TYPE(room) == SECT_WATER_NOSWIM  ||        \
   SECT_TYPE(room) == SECT_WATER_SWIM ||        \
   SECT_TYPE(room) == SECT_UNDERWATER || \
   SECT_TYPE(room) == SECT_DEEP_OCEAN)) ||        \
 GET_CLASS(ch) != CLASS_WATER)

inline bool
CHAR_LIKES_ROOM(struct Creature * ch, struct room_data * room)
{
	if (!ELEMENTAL_LIKES_ROOM(ch, room))
		return false;

	if (ROOM_FLAGGED(room, ROOM_FLAME_FILLED) && !CHAR_WITHSTANDS_FIRE(ch))
		return false;

	if (ROOM_FLAGGED(room, ROOM_ICE_COLD) && !CHAR_WITHSTANDS_COLD(ch))
		return false;

	if (ROOM_FLAGGED(room, ROOM_HOLYOCEAN) && IS_EVIL(ch))
		return false;

	if (!can_travel_sector(ch, room->sector_type, 0))
		return false;

	if (MOB2_FLAGGED(ch, MOB2_STAY_SECT) &&
			ch->in_room->sector_type != room->sector_type)
		return false;

	if (!can_see_room(ch, ch->in_room) &&
			(!GET_EQ(ch, WEAR_LIGHT) ||
				GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0)))
		return false;

	return true;
}

void
mobile_activity(void)
{

	struct Creature *ch, *vict = NULL;
	struct obj_data *obj, *best_obj, *i;
	struct affected_type *af_ptr = NULL;
	CreatureList::iterator cit, it;
	int dir, found, max, k;
	static unsigned int count = 0;
	struct room_data *room = NULL;
    int cur_class = 0;

	extern int no_specials;

	cit = characterList.begin();
	for (++count; cit != characterList.end(); ++cit) {
		ch = *cit;
		found = FALSE;
 
     if (!ch->in_room && !ch->player.name && !ch->player.short_descr
         && !ch->player.description) {
         errlog("SYSERR: Skipping null mobile in mobile_activity");
		//
		// Check for mob spec
		//
		if (!no_specials && MOB_FLAGGED(ch, MOB_SPEC) &&
			GET_MOB_WAIT(ch) <= 0 && !ch->desc && (count % 2)) {
			if (ch->mob_specials.shared->func == NULL) {
				errlog("%s (#%d): Attempting to call non-existing mob func",
					GET_NAME(ch), GET_MOB_VNUM(ch));
				REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
			} else {
				if ((ch->mob_specials.shared->func) (ch, ch, 0, "", SPECIAL_TICK)) {
					continue;	/* go to next char */
				}
			}
		}


		//
		// Non-special mobs in idle zones don't do anything
		//
		if( IS_NPC(ch) && ch->in_room->zone->idle_time >= ZONE_IDLE_TIME)
			continue;

		//
		// poison 2 tick
		//

		if (HAS_POISON_2(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR && !(count % 2)) {
			if (damage(ch, ch, dice(4, 3) +
					(affected_by_spell(ch, SPELL_METABOLISM) ? dice(4,
							11) : 0), SPELL_POISON, -1))
				continue;
		}

		// Remorts will act like their secondary class 1/3rd of the time
	    if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_fractional_3())
            cur_class = GET_REMORT_CLASS(ch);
        else
            cur_class = GET_CLASS(ch);

		
		//
		// bleed
		//

		if (GET_HIT(ch) &&
			CHAR_HAS_BLOOD(ch) &&
			GET_HIT(ch) < ((GET_MAX_HIT(ch) >> 3) +
				random_number_zero_low(MAX(1, GET_MAX_HIT(ch) >> 4))))
			add_blood_to_room(ch->in_room, 1);

		//
		// Zen of Motion effect
		//

		if (IS_NEUTRAL(ch) && affected_by_spell(ch, ZEN_MOTION))
			GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch),
				GET_MOVE(ch) +
				random_number_zero_low(MAX(1, CHECK_SKILL(ch,
							ZEN_MOTION) >> 3)));

		//
		// Deplete scuba tanks
		//

		if ((obj = ch->equipment[WEAR_FACE]) &&
			GET_OBJ_TYPE(obj) == ITEM_SCUBA_MASK &&
			!CAR_CLOSED(obj) &&
			obj->aux_obj && GET_OBJ_TYPE(obj->aux_obj) == ITEM_SCUBA_TANK &&
			(GET_OBJ_VAL(obj->aux_obj, 1) > 0 ||
				GET_OBJ_VAL(obj->aux_obj, 0) < 0)) {
			if (GET_OBJ_VAL(obj->aux_obj, 0) > 0) {
				GET_OBJ_VAL(obj->aux_obj, 1)--;
				if (!GET_OBJ_VAL(obj->aux_obj, 1))
					act("A warning indicator reads: $p fully depleted.",
						FALSE, ch, obj->aux_obj, 0, TO_CHAR);
				else if (GET_OBJ_VAL(obj->aux_obj, 1) == 5)
					act("A warning indicator reads: $p air level low.",
						FALSE, ch, obj->aux_obj, 0, TO_CHAR);
			}
		}
		//
		// TODO: fix this so it doesnt potentially cause a crash
		// mob_read_script() can potentially cause the mob to kill the character
		// which happens to be pointed to by next_ch, which will be disastrous
		// at the top of the loop
		//

		if (IS_NPC(ch)
				&& MOB2_FLAGGED(ch, MOB2_SCRIPT)
				&& GET_IMPLANT(ch, WEAR_HOLD)
				&& OBJ_TYPE(GET_IMPLANT(ch, WEAR_HOLD), ITEM_SCRIPT)) {

			if (mob_read_script(ch, NULL, 0, NULL, SPECIAL_TICK))
				continue;
		}

		//
		// nothing below this conditional affects FIGHTING characters
		//

		if (ch->numCombatants() || ch->getPosition() == POS_FIGHTING) {
			continue;
		}

		//
		// meditate
		// 
		
		if (IS_NEUTRAL(ch) && ch->getPosition() == POS_SITTING
		&& IS_AFFECTED_2(ch, AFF2_MEDITATE)) 
		{
			perform_monk_meditate(ch);
		} 

		//
		// Check if we've gotten knocked down.
		//

		if (IS_NPC(ch) && !MOB2_FLAGGED(ch, MOB2_MOUNT) &&
			!AFF_FLAGGED(ch, AFF_SLEEP) &&
			GET_MOB_WAIT(ch) < 30 &&
			!AFF_FLAGGED(ch, AFF_SLEEP) &&
			ch->getPosition() >= POS_SLEEPING &&
			(GET_DEFAULT_POS(ch) <= POS_STANDING ||
				ch->getPosition() < POS_STANDING) &&
			(ch->getPosition() < GET_DEFAULT_POS(ch))
			&& random_fractional_3()) {
			if (ch->getPosition() == POS_SLEEPING)
				act("$n wakes up.", TRUE, ch, 0, 0, TO_ROOM);

			switch (GET_DEFAULT_POS(ch)) {
			case POS_SITTING:
				act("$n sits up.", TRUE, ch, 0, 0, TO_ROOM);
				ch->setPosition(POS_SITTING);
				break;
			default:
				if (!IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)
					|| number(1, 20) < GET_STR(ch)) {
					act("$n stands up.", TRUE, ch, 0, 0, TO_ROOM);
					ch->setPosition(POS_STANDING);
				}
				break;
			}
			continue;
		}
		//
		// nothing below this conditional affects characters who are asleep or in a wait state
		//

		if (!AWAKE(ch) || GET_MOB_WAIT(ch) > 0 || CHECK_WAIT(ch))
			continue;

		//
		// barbs go BERSERK (berserk)
		//

		if (GET_LEVEL(ch) < LVL_AMBASSADOR && AFF2_FLAGGED(ch, AFF2_BERSERK)) {

			int return_flags = 0;
			if (perform_barb_berserk(ch, 0, &return_flags))
				continue;
		}
		//
		// drunk effects
		//

		if (GET_COND(ch, DRUNK) > GET_CON(ch) && random_fractional_10()) {
			found = FALSE;
			act("$n burps loudly.", FALSE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You burp loudly.\r\n");
			found = TRUE;
		} else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 2 && random_fractional_10()) {
			found = FALSE;
			act("$n hiccups.", FALSE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You hiccup.\r\n");
			found = TRUE;
		}
		//
		// nothing below this conditional affects PCs
		//

		if (!IS_MOB(ch) || ch->desc)
			continue;

		// If players can't do anything while petrified, neither can mobs
		if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
			continue;


		/** implicit awake && !fighting **/

		/** mobiles re-hiding **/
		if (!AFF_FLAGGED(ch, AFF_HIDE) &&
			AFF_FLAGGED(MOB_SHARED(ch)->proto, AFF_HIDE))
			SET_BIT(AFF_FLAGS(ch), AFF_HIDE);

		/** mobiles reloading guns **/
		if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {
			for (obj = ch->carrying; obj; obj = obj->next_content) {
				if ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
					(IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
					(IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))) {
					mob_reload_gun(ch, obj);
					break;
				}
			}

			if ((obj = GET_EQ(ch, WEAR_WIELD)) &&
				((IS_GUN(obj) && !GUN_LOADED(obj)) ||
					(IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
					(IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
				mob_reload_gun(ch, obj);
			if ((obj = GET_EQ(ch, WEAR_WIELD_2)) &&
				((IS_GUN(obj) && !GUN_LOADED(obj)) ||
					(IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
					(IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
				mob_reload_gun(ch, obj);
			if ((obj = GET_IMPLANT(ch, WEAR_WIELD)) &&
				((IS_GUN(obj) && !GUN_LOADED(obj)) ||
					(IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
					(IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
				mob_reload_gun(ch, obj);
			if ((obj = GET_IMPLANT(ch, WEAR_WIELD_2)) &&
				((IS_GUN(obj) && !GUN_LOADED(obj)) ||
					(IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
					(IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
				mob_reload_gun(ch, obj);
		}

		/* Mobiles looking at chars */
		if (random_fractional_20()) {
			it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				vict = *it;
				if (vict == ch)
					continue;
				if (can_see_creature(ch, vict) && random_fractional_50()) {
					if ((IS_EVIL(ch) && IS_GOOD(vict)) || (IS_GOOD(ch)
							&& IS_EVIL(ch))) {
						if (GET_LEVEL(ch) < (GET_LEVEL(vict) - 10)) {
							act("$n looks warily at $N.", FALSE, ch, 0, vict,
								TO_NOTVICT);
							act("$n looks warily at you.", FALSE, ch, 0, vict,
								TO_VICT);
						} else {
							act("$n growls at $N.", FALSE, ch, 0, vict,
								TO_NOTVICT);
							act("$n growls at you.", FALSE, ch, 0, vict,
								TO_VICT);
						}
					} else if (cur_class == CLASS_PREDATOR) {
						act("$n growls at $N.", FALSE, ch, 0, vict,
							TO_NOTVICT);
						act("$n growls at you.", FALSE, ch, 0, vict, TO_VICT);
					} else if (cur_class == CLASS_THIEF) {
						act("$n glances sidelong at $N.", FALSE, ch, 0, vict,
							TO_NOTVICT);
						act("$n glances sidelong at you.", FALSE, ch, 0, vict,
							TO_VICT);
					} else if (((IS_MALE(ch) && IS_FEMALE(vict))
							|| (IS_FEMALE(ch) && IS_MALE(vict)))
						&& random_fractional_4()) {
						act("$n stares dreamily at $N.", FALSE, ch, 0, vict,
							TO_NOTVICT);
						act("$n stares dreamily at you.", FALSE, ch, 0, vict,
							TO_VICT);
					} else if (random_binary()) {
						act("$n looks at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
						act("$n looks at you.", FALSE, ch, 0, vict, TO_VICT);
					}
					break;
				}
			}
		}

		/* Scavenger (picking up objects) */

		if (MOB_FLAGGED(ch, MOB_SCAVENGER)) {
			if (ch->in_room->contents && random_fractional_10()) {
				max = 1;
				best_obj = NULL;
				for (obj = ch->in_room->contents; obj; obj = obj->next_content)
					if (can_see_object(ch, obj) &&
						// don't pick up sigil-ized objs if we know better
						(!GET_OBJ_SIGIL_IDNUM(obj) ||
							(!AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
								&& !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING)))
						&& CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
						best_obj = obj;
						max = GET_OBJ_COST(obj);
					}
				if (best_obj != NULL) {
					strcpy(buf, fname(best_obj->aliases));
					do_get(ch, buf, 0, 0, 0);
				}
			}
		}

		/* Drink from fountains */
		if (GET_RACE(ch) != RACE_UNDEAD && GET_RACE(ch) != RACE_DRAGON &&
			GET_RACE(ch) != RACE_GOLEM && GET_RACE(ch) != RACE_ELEMENTAL) {
			if (ch->in_room->contents && random_fractional_100()) {
				for (obj = ch->in_room->contents; obj; obj = obj->next_content)
					if (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN &&
						GET_OBJ_VAL(obj, 1) > 0)
						break;
				if (obj && GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN &&
					GET_OBJ_VAL(obj, 1) > 0) {
					act("$n drinks from $p.", TRUE, ch, obj, 0, TO_ROOM);
					act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
					continue;
				}
				continue;
			}
		}

		/* Animals devouring corpses and food */
		if (cur_class == CLASS_PREDATOR) {
			if (ch->in_room->contents &&
				(random_fractional_4() || IS_TARRASQUE(ch))) {
				found = FALSE;
				for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
					if (GET_OBJ_TYPE(obj) == ITEM_FOOD && !GET_OBJ_VAL(obj, 3)) {
						found = TRUE;
						act("$n devours $p, growling and drooling all over.",
							FALSE, ch, obj, 0, TO_ROOM);
						extract_obj(obj);
						break;
					} else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER &&
							GET_OBJ_VAL(obj, 3)) {
						room_data *stuff_rm;

						act("$n devours $p, growling and drooling all over.",
							FALSE, ch, obj, 0, TO_ROOM);
						if (IS_TARRASQUE(ch))
							stuff_rm = real_room(24919);
						else
							stuff_rm = NULL;
						
						if (!stuff_rm)
							stuff_rm = ch->in_room;
						for (i = obj->contains; i; i = best_obj) {
							best_obj = i->next_content;
							if (IS_IMPLANT(i)) {
								SET_BIT(GET_OBJ_WEAR(i), ITEM_WEAR_TAKE);
								if (GET_OBJ_DAM(i) > 0)
									GET_OBJ_DAM(i) >>= 1;
							}
							obj_from_obj(i);
							obj_to_room(i, stuff_rm);

						}
						extract_obj(obj);
						break;
					}
				}
				if (found)
					continue;
			}
		}

		/* Wearing Objects */
		if (!MOB2_FLAGGED(ch, MOB2_WONT_WEAR) && !IS_ANIMAL(ch) &&
			!ch->numCombatants() && !IS_DRAGON(ch) && !IS_ELEMENTAL(ch)) {
			if (ch->carrying && random_fractional_4()) {
				for (obj = ch->carrying; obj; obj = best_obj) {
					best_obj = obj->next_content;
					if (ANTI_ALIGN_OBJ(ch, obj)
						&& (random_number_zero_low(15) + 6) < GET_INT(ch)
						&& !IS_NODROP(obj)) {
						// Check to see if the obj is renamed Or Imm Enchanted
						// Log it if it is.
						struct obj_data *original =
							real_object_proto(GET_OBJ_VNUM(obj));
						int renamed = 0;
						if (original)
							renamed =
								strcmp(obj->name,
								original->name);
						if (renamed || isname_exact("imm", obj->aliases)) {
							mudlog(LVL_IMMORT, CMP, true,
								"%s [%d] junked by %s at %s [%d]. ( %s %s )",
								obj->name, GET_OBJ_VNUM(obj),
								GET_NAME(ch), ch->in_room->name,
								ch->in_room->number, renamed ? "R3nAm3" : "",
								isname_exact("imm",
									obj->aliases) ? "|mM3nChAnT" : "");
						}
						act("$n junks $p.", TRUE, ch, obj, 0, TO_ROOM);
						extract_obj(obj);
						break;
					}
					if (can_see_object(ch, obj) &&
						!IS_IMPLANT(obj) &&
						(GET_OBJ_TYPE(obj) == ITEM_WEAPON ||
							IS_ENERGY_GUN(obj) || IS_GUN(obj)) &&
						!invalid_char_class(ch, obj) &&
						(!GET_EQ(ch, WEAR_WIELD) ||
							!IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD),
								ITEM2_NOREMOVE))) {
						if (GET_EQ(ch, WEAR_WIELD)
							&& (obj->getWeight() <=
								str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
							&& GET_OBJ_COST(obj) > GET_OBJ_COST(GET_EQ(ch,
									WEAR_WIELD))) {
							strcpy(buf, fname(obj->aliases));
							do_remove(ch, buf, 0, 0, 0);
						}
						if (!GET_EQ(ch, WEAR_WIELD)) {
							do_wield(ch, OBJN(obj, ch), 0, 0, 0);
							if (IS_GUN(obj) && GET_MOB_VNUM(ch) == 1516)
								do_say(ch, "Let's Rock.", 0, 0, 0);
						}
					}
				}

				//
				// "wear all" can cause the death of ch, but we don't care since we will continue
				// immediately afterwards
				//

				do_wear(ch, "all", 0, 0, 0);
				continue;
			}
		}

		/* Looter */

		if (MOB2_FLAGGED(ch, MOB2_LOOTER)) {
			struct obj_data *o = NULL;
			if (ch->in_room->contents && random_fractional_3()) {
				for (i = ch->in_room->contents; i; i = i->next_content) {
					if ((GET_OBJ_TYPE(i) == ITEM_CONTAINER
							&& GET_OBJ_VAL(i, 3))) {
						if (!i->contains)
							continue;
						for (obj = i->contains; obj; obj = obj->next_content) {
							if (o) {
								extract_obj(o);
								o = NULL;
							}
							if (!(obj->in_obj))
								continue;

							// skip sigil-ized items, simplest way to deal with it
							if (GET_OBJ_SIGIL_IDNUM(obj))
								continue;

							if (CAN_GET_OBJ(ch, obj)) {
								obj_from_obj(obj);
								obj_to_char(obj, ch);
								act("$n gets $p from $P.", FALSE, ch, obj, i,
									TO_ROOM);
								if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
									if (GET_OBJ_VAL(obj, 1))	// credits
										GET_CASH(ch) += GET_OBJ_VAL(obj, 0);
									else
										GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
									o = obj;
								}
							}
						}
						if (o) {
							extract_obj(o);
							o = NULL;
						}
					}
				}
			}
		}

		/* Helper Mobs */

		if (MOB_FLAGGED(ch, MOB_HELPER) && random_binary()) {
			found = FALSE;
			int fvict_retval = 0;
			int vict_retval = 0;
			vict = NULL;
			//struct Creature *tmp_next_ch = 0;

			if (IS_AFFECTED(ch, AFF_CHARM))
				continue;

			if (IS_AFFECTED(ch, AFF_BLIND))
				continue;

			it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && !found; ++it) {
				vict = *it;
				if (ch != vict && vict->numCombatants() && !vict->findCombat(ch) &&
					can_see_creature(ch, vict)) {

					int fvict_help_prob =
						helper_help_probability(ch, vict->findRandomCombat());
					int fvict_attack_prob =
						helper_attack_probability(ch, vict->findRandomCombat());

					int vict_help_prob = helper_help_probability(ch, vict);
					int vict_attack_prob = helper_attack_probability(ch, vict);

					//
					// if we're not willing to help anybody here, then just continue
					//

					if (fvict_help_prob <= 0 && fvict_attack_prob <= 0)
						continue;
					if (vict_help_prob <= 0 && vict_attack_prob <= 0)
						continue;

					int fvict_composite_prob =
						fvict_help_prob + vict_attack_prob;
					int vict_composite_prob =
						vict_help_prob + fvict_attack_prob;

					//
					// attack vict, helping fvict
					//

					if (fvict_composite_prob > vict_composite_prob &&
						fvict_help_prob > 0 && vict_attack_prob > 0) {

						//
						// store a pointer past next_ch if next_ch _happens_ to be vict
						//

						vict_retval = helper_assist(ch, vict, vict->findRandomCombat());
						found = 1;
						break;
					}
					//
					// attack fvict, helping fict
					//

					else if (vict_composite_prob > fvict_composite_prob &&
						vict_help_prob > 0 && fvict_attack_prob > 0) {
						//
						// store a pointer past next_ch if next_ch _happens_ to be fvict
						//


						fvict_retval = helper_assist(ch, vict->findRandomCombat(), 
                                                     vict);
						found = 1;
						break;
					}

				}
				// continue
			}

			if (found)
				continue;
		}

		/*Racially aggressive Mobs */

		if (IS_RACIALLY_AGGRO(ch) && random_fractional_4()) {
			found = FALSE;
			vict = NULL;
			room_data *room = ch->in_room;
			it = room->people.begin();
			for (; it != room->people.end() && !found; ++it) {
				vict = *it;
				if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS))
					|| !can_see_creature(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
					IS_AFFECTED_2(vict, AFF2_PETRIFIED))
					continue;

				if (check_infiltrate(vict, ch))
					continue;

				if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
					continue;

				// DIVIDE BY ZERO ERROR! FPE!
				if (GET_MORALE(ch) + GET_LEVEL(ch) <
					number(GET_LEVEL(vict), (GET_LEVEL(vict) << 2) +
						(MAX(1, (GET_HIT(vict)) * GET_LEVEL(vict))
							/ MAX(1, GET_MAX_HIT(vict)))) && AWAKE(vict))
					continue;
				else if (RACIAL_ATTACK(ch, vict) &&
					(!IS_NPC(vict) || MOB2_FLAGGED(ch, MOB2_ATK_MOBS))) {

					//int retval = 
					best_attack(ch, vict);

					found = TRUE;
					break;
				}
			}

			if (found)
				continue;
		}

		/* Aggressive Mobs */

		if (MOB_FLAGGED(ch, MOB_AGGRESSIVE)
				|| MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN)) {
			found = FALSE;
			vict = NULL;
			it = ch->in_room->people.begin();
			CreatureList::iterator nit = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && !found; ++it) {
				++nit;
				vict = *it;
				if (vict == ch || (nit != ch->in_room->people.end()
						&& random_fractional_4()) || (!IS_NPC(vict)
						&& !vict->desc)) {
					continue;
				}
				if (check_infiltrate(vict, ch))
					continue;

				if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS))
					|| !can_see_creature(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
					IS_AFFECTED_2(vict, AFF2_PETRIFIED)) {
					continue;
				}
				if (AWAKE(vict) &&
					GET_MORALE(ch) <
					(random_number_zero_low(GET_LEVEL(vict) << 1) + 35 +
						(GET_LEVEL(vict) >> 1)))
					continue;

				if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master
					&& ch->master == vict
					&& (GET_CHA(vict) + GET_INT(vict)) >
					GET_INT(ch) + !random_fractional_20())
					continue;

				if (MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) &&
					GET_RACE(ch) == GET_RACE(vict))
					continue;

				if (!MOB_FLAGGED(ch, MOB_AGGRESSIVE))
					if ((IS_EVIL(vict) && !MOB_FLAGGED(ch, MOB_AGGR_EVIL)) ||
						(IS_GOOD(vict) && !MOB_FLAGGED(ch, MOB_AGGR_GOOD)) ||
						(IS_NEUTRAL(vict)
							&& !MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL)))
						continue;

				if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
					continue;

				best_attack(ch, vict);

				found = 1;
				break;
			}

			// end for vict

			if (found)
				continue;

			/** scan surrounding rooms **/
			if (!found && !HUNTING(ch) && ch->getPosition() > POS_FIGHTING && 
			!MOB_FLAGGED(ch, MOB_SENTINEL) && 
			(GET_LEVEL(ch) + GET_MORALE(ch) > (random_number_zero_low(120) + 50) 
			 || IS_TARRASQUE(ch)))
			{	
				found = 0;

				for (dir = 0; dir < NUM_DIRS && !found; dir++) {
					if(! CAN_GO(ch, dir) )
						continue;
					room_data *tmp_room = EXIT(ch, dir)->to_room;
					if ( !ROOM_FLAGGED(tmp_room, ROOM_DEATH | ROOM_NOMOB) 
						&& tmp_room != ch->in_room 
						&& CHAR_LIKES_ROOM(ch, tmp_room) 
						&& tmp_room->people.size() > 0 
						&& can_see_creature( ch, (*(tmp_room->people.begin()) ) )
						&& tmp_room->people.size() < (unsigned)tmp_room->max_occupancy ) 
					{
						break;
					}
				}

				if (dir < NUM_DIRS) {
					vict = NULL;
					room_data *tmp_room = EXIT(ch, dir)->to_room;
					it = tmp_room->people.begin();
					for (; it != tmp_room->people.end() && !found; ++it) {
						vict = *it;
						if (can_see_creature(ch, vict)
							&& !PRF_FLAGGED(vict, PRF_NOHASSLE)
							&& (!AFF_FLAGGED(vict, AFF_SNEAK)
								|| AFF_FLAGGED(vict, AFF_GLOWLIGHT)
								|| AFF_FLAGGED(vict,
									AFF2_FLUORESCENT |
									AFF2_DIVINE_ILLUMINATION))
							&& (MOB_FLAGGED(ch, MOB_AGGRESSIVE)
								|| (MOB_FLAGGED(ch, MOB_AGGR_EVIL)
									&& IS_EVIL(vict))
								|| (MOB_FLAGGED(ch, MOB_AGGR_GOOD)
									&& IS_GOOD(vict))
								|| (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL)
									&& IS_NEUTRAL(vict)))
							&& (!MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE)
								|| GET_RACE(ch) != GET_RACE(vict))
							&& (!IS_NPC(vict)
								|| MOB2_FLAGGED(ch, MOB2_ATK_MOBS))) {
							found = 1;
							break;
						}
					}
					if (found) {
						if (IS_PSIONIC(ch) && GET_LEVEL(ch) > 23 &&
							GET_MOVE(ch) > 100 && GET_MANA(vict) > 100) {
							int retval = 0;
							do_psidrain(ch, fname(vict->player.name), 0, 0, &retval);
						} else {
							perform_move(ch, dir, MOVE_NORM, 1);
						}
					}
				}
			}
		}

		if (found)
			continue;

		/* Mob Memory */

		if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch) &&
			!AFF_FLAGGED(ch, AFF_CHARM)) {
			found = FALSE;
			room_data *room = ch->in_room;
			it = room->people.begin();
			for (; it != room->people.end() && !found; ++it) {
				vict = *it;
				if (check_infiltrate(vict, ch))
					continue;

				if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS)) ||
					!can_see_creature(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
					((af_ptr = affected_by_spell(vict, SKILL_DISGUISE)) &&
						!CAN_DETECT_DISGUISE(ch, vict, af_ptr->duration)))
					continue;
				if (char_in_memory(vict, ch)) {
					found = TRUE;

					if (ch->getPosition() != POS_FIGHTING) {
						switch (random_number_zero_low(20)) {
						case 0:
							if ((!IS_ANIMAL(ch) && !IS_DEVIL(ch)
									&& !IS_DEMON(ch) && !IS_UNDEAD(ch)
									&& !IS_DRAGON(ch))
								|| random_fractional_4())
								act("'You wimp $N!  Your ass is grass.', exclaims $n.", FALSE, ch, 0, vict, TO_ROOM);
							else {
								act("$n growls at you menacingly.",
									TRUE, ch, 0, vict, TO_VICT);
								act("$n growls menacingly at $N.",
									TRUE, ch, 0, vict, TO_NOTVICT);
							}
							break;
						case 1:
							act("$n sighs loudly.", FALSE, ch, 0, 0, TO_ROOM);
							break;
						case 2:
							act("$n gazes at you coldly.", TRUE, ch, 0, vict,
								TO_VICT);
							act("$n gazes coldly at $N.", TRUE, ch, 0, vict,
								TO_NOTVICT);
							break;
						case 3:
							act("$n prepares for battle.", TRUE, ch, 0, 0,
								TO_ROOM);
							break;
						case 4:
							perform_tell(ch, vict, "Let's rumble.");
							break;
						case 5:	// look for help
							break;
						case 6:
							if (AWAKE(vict) && !IS_UNDEAD(ch) && !IS_DRAGON(ch) &&
								!IS_DEVIL(ch) && GET_HIT(ch) > GET_HIT(vict) &&
								((GET_LEVEL(vict) + ((50 * GET_HIT(vict)) /
											MAX(1, GET_MAX_HIT(vict)))) >
									GET_LEVEL(ch) + (GET_MORALE(ch) >> 1) +
									random_percentage())) {
								if (!IS_ANIMAL(ch) && !IS_SLIME(ch)
									&& !IS_PUDDING(ch)) {
									if (random_fractional_4())
										do_say(ch, "Oh, shit!", 0, 0, 0);
									else if (random_fractional_3())
										act("$n screams in terror!", FALSE, ch, 0,
											0, TO_ROOM);
									else if (random_binary())
										do_gen_comm(ch, "Run away!  Run away!", 0,
											SCMD_SHOUT, 0);
								}
								do_flee(ch, "", 0, 0, 0);
								break;
							}
							if (IS_ANIMAL(ch)) {
								act("$n snarls and attacks $N!!", FALSE, ch, 0,
									vict, TO_NOTVICT);
								act("$n snarls and attacks you!!", FALSE, ch, 0,
									vict, TO_VICT);
							} else {
								if (random_binary())
									act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
								else
									act("'Hey!  You're the punk I've been looking for!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
							}
							best_attack(ch, vict);
							break;
						}
					}
				}
			}
		}

		if (found)
			continue;

		/* Mob Movement -- Lair */
		if (GET_MOB_LAIR(ch) > 0 && ch->in_room->number != GET_MOB_LAIR(ch) &&
			!HUNTING(ch) &&
			(room = real_room(GET_MOB_LAIR(ch))) &&
			((dir = find_first_step(ch->in_room, room, STD_TRACK)) >= 0) &&
			MOB_CAN_GO(ch, dir) &&
			!ROOM_FLAGGED(ch->in_room->dir_option[dir]->to_room,
				ROOM_NOMOB | ROOM_DEATH)) {
			smart_mobile_move(ch, dir);
			continue;
		}
		// mob movement -- follow the leader

		if (GET_MOB_LEADER(ch) > 0 && ch->master &&
			ch->in_room != ch->master->in_room) {
			if (smart_mobile_move(ch, find_first_step(ch->in_room,
						ch->master->in_room, STD_TRACK)))
				continue;
		}

		/* Mob Movement */
		if (!MOB_FLAGGED(ch, MOB_SENTINEL)
			&& !((MOB_FLAGGED(ch, MOB_PET) || MOB2_FLAGGED(ch, MOB2_FAMILIAR))
				&& ch->master)
			&& ch->getPosition() >= POS_STANDING 
			&& !IS_AFFECTED_2(ch, AFF2_MOUNTED) ){

			int door;
			
			if (IS_TARRASQUE(ch) || ch->in_room->people.size() > 10)
				door = random_number_zero_low(NUM_OF_DIRS - 1);
			else
				door = random_number_zero_low(20);

			if ((door < NUM_OF_DIRS) &&
				(MOB_CAN_GO(ch, door)) &&
				(rev_dir[door] != ch->mob_specials.last_direction ||
					ch->in_room->countExits() < 2 ||
					random_binary())
				&& (EXIT(ch, door)->to_room != ch->in_room)
				&& (!ROOM_FLAGGED(EXIT(ch, door)->to_room,
						ROOM_NOMOB | ROOM_DEATH)
					&& !IS_SET(EXIT(ch, door)->exit_info, EX_NOMOB))
				&& (CHAR_LIKES_ROOM(ch, EXIT(ch, door)->to_room))
				&& (!MOB2_FLAGGED(ch, MOB2_STAY_SECT)
					|| (EXIT(ch, door)->to_room->sector_type ==
						ch->in_room->sector_type))
				&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE)
					|| (EXIT(ch, door)->to_room->zone == ch->in_room->zone))
				&& EXIT(ch, door)->to_room->people.size() < 10) {
				if (perform_move(ch, door, MOVE_NORM, 1))
					continue;
			}
		} 

		//
		// thief tries to steal from others
		//

		if (cur_class == CLASS_THIEF && random_binary()) {
			if (thief(ch, ch, 0, "", SPECIAL_TICK))
				continue;
		}
		//
		// clerics spell up
		//

		else if (cur_class == CLASS_CLERIC && random_binary()) {
			if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
				if (GET_LEVEL(ch) > 23)
					cast_spell(ch, ch, 0, SPELL_HEAL);
				else if (GET_LEVEL(ch) > 11)
					cast_spell(ch, ch, 0, SPELL_CURE_CRITIC);
				else
					cast_spell(ch, ch, 0, SPELL_CURE_LIGHT);
			} else if (room_is_dark(ch->in_room) &&
				!has_dark_sight(ch) && GET_LEVEL(ch) > 8) {
				cast_spell(ch, ch, 0, SPELL_DIVINE_ILLUMINATION);
			} else if ((affected_by_spell(ch, SPELL_BLINDNESS) ||
					affected_by_spell(ch, SKILL_GOUGE)) && GET_LEVEL(ch) > 6) {
				cast_spell(ch, ch, 0, SPELL_CURE_BLIND);
			} else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 7) {
				cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
			} else if (IS_AFFECTED(ch, AFF_CURSE) && GET_LEVEL(ch) > 26) {
				cast_spell(ch, ch, 0, SPELL_REMOVE_CURSE);
			} else if (GET_MANA(ch) > (GET_MAX_MANA(ch) * 0.75) &&
				GET_LEVEL(ch) >= 28 &&
				!AFF_FLAGGED(ch, AFF_SANCTUARY) &&
				!AFF_FLAGGED(ch, AFF_NOPAIN))
				cast_spell(ch, ch, 0, SPELL_SANCTUARY);
		}
		//
		// knights spell up
		//

		else if (cur_class == CLASS_KNIGHT && random_binary()) {
            knight_activity(ch);
		}
		//
		// ranger spell up
		//


		else if (cur_class == CLASS_RANGER && random_binary()) {
            ranger_activity(ch);
		}
		//
		// cyborg spell up
		//

		else if (cur_class == CLASS_CYBORG && random_binary()) {
			if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80 && GET_MOVE(ch) > 100) {
				do_repair(ch, "", 0, 0, 0);
			}
			if (!random_percentage_zero_low()) {
				if (GET_LEVEL(ch) > 11) {
					if (!affected_by_spell(ch, SKILL_DAMAGE_CONTROL)) {
						if (GET_LEVEL(ch) >= 12)
							do_activate(ch, "damage control", 0, 1, 0);	// 12
						if (GET_LEVEL(ch) >= 17
							&& GET_REMORT_CLASS(ch) != CLASS_UNDEFINED)
							do_activate(ch, "radionegation", 0, 1, 0);	// 17
						if (GET_LEVEL(ch) >= 18)
							do_activate(ch, "power boost", 0, 1, 0);	// 18
						do_activate(ch, "reflex boost", 0, 1, 0);	// 18
						if (GET_LEVEL(ch) >= 24)
							do_activate(ch, "melee weapons tactics", 0, 1, 0);	// 24
						if (GET_LEVEL(ch) >= 32)
							do_activate(ch, "energy field", 0, 1, 0);	// 32
						if (GET_LEVEL(ch) >= 37)
							do_activate(ch, "shukutei adrenal maximizations", 1, 0, 0);	// 37
					}
				}
			}
		}
		//
		// mage spell up
		//

		else if (cur_class == CLASS_MAGE && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			&& random_binary()) {
			if (GET_LEVEL(ch) > 3 && !affected_by_spell(ch, SPELL_ARMOR)) {
				cast_spell(ch, ch, 0, SPELL_ARMOR);
			} else if (room_is_dark(ch->in_room) &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_ARMOR) &&
				!has_dark_sight(ch) && GET_LEVEL(ch) > 5) {
				cast_spell(ch, ch, 0, SPELL_INFRAVISION);
			} else if (GET_LEVEL(ch) > 12 && !IS_AFFECTED(ch, AFF_BLUR) &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_BLUR)) {
				cast_spell(ch, ch, 0, SPELL_BLUR);
			} else if (GET_LEVEL(ch) > 27
				&& !IS_AFFECTED_2(ch, AFF2_TELEKINESIS)
				&& GET_MANA(ch) > mag_manacost(ch, SPELL_TELEKINESIS)) {
				cast_spell(ch, ch, 0, SPELL_TELEKINESIS);
			} else if (GET_LEVEL(ch) > 43 && !IS_AFFECTED_2(ch, AFF2_HASTE) &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_HASTE)) {
				cast_spell(ch, ch, 0, SPELL_HASTE);
			} else if (GET_LEVEL(ch) > 45
				&& !IS_AFFECTED_2(ch, AFF2_DISPLACEMENT)
				&& GET_MANA(ch) > mag_manacost(ch, SPELL_DISPLACEMENT)) {
				cast_spell(ch, ch, 0, SPELL_DISPLACEMENT);
			} else if (GET_LEVEL(ch) > 16
				&& !IS_AFFECTED_2(ch, AFF2_FIRE_SHIELD)
				&& GET_MANA(ch) > mag_manacost(ch, SPELL_FIRE_SHIELD)
				&& SECT(ch->in_room) != SECT_UNDERWATER
				&& SECT(ch->in_room) != SECT_DEEP_OCEAN
				&& SECT(ch->in_room) != SECT_ELEMENTAL_WATER) {
				cast_spell(ch, ch, 0, SPELL_FIRE_SHIELD);
			} else if (GET_LEVEL(ch) > 48
				&& !IS_AFFECTED_3(ch, AFF3_PRISMATIC_SPHERE)
				&& GET_MANA(ch) > mag_manacost(ch, SPELL_FIRE_SHIELD)) {
				cast_spell(ch, ch, 0, SPELL_PRISMATIC_SPHERE);
			} else if (IS_REMORT(ch)) {
				if (GET_LEVEL(ch) > 20
					&& !affected_by_spell(ch, SPELL_ANTI_MAGIC_SHELL)
					&& GET_MANA(ch) > mag_manacost(ch,
						SPELL_ANTI_MAGIC_SHELL)) {
					cast_spell(ch, ch, 0, SPELL_ANTI_MAGIC_SHELL);

				}
			}

		}
		//
		// psionic spell up
		//

		else if (cur_class == CLASS_PSIONIC && !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)
			&& random_binary()) {
			if (room_is_dark(ch->in_room) && !has_dark_sight(ch)
				&& GET_LEVEL(ch) >= 6)
				cast_spell(ch, ch, 0, SPELL_INFRAVISION);
			else if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80
				&& GET_LEVEL(ch) >= 10) {
				if (GET_LEVEL(ch) >= 34)
					cast_spell(ch, ch, 0, SPELL_CELL_REGEN);
				else
					cast_spell(ch, ch, 0, SPELL_WOUND_CLOSURE);
			} else if (GET_LEVEL(ch) >= 42 && !AFF_FLAGGED(ch, AFF_NOPAIN) &&
				!AFF_FLAGGED(ch, AFF_SANCTUARY))
				cast_spell(ch, ch, 0, SPELL_NOPAIN);
			else if (GET_LEVEL(ch) >= 21 &&
				(ch->in_room->sector_type == SECT_UNDERWATER ||
					ch->in_room->sector_type == SECT_DEEP_OCEAN ||
					ch->in_room->sector_type == SECT_WATER_NOSWIM ||
					ch->in_room->sector_type == SECT_PITCH_SUB) &&
				!can_travel_sector(ch, ch->in_room->sector_type, 0) &&
				!AFF3_FLAGGED(ch, AFF3_NOBREATHE))
				cast_spell(ch, ch, 0, SPELL_BREATHING_STASIS);
			else if (GET_LEVEL(ch) >= 42 &&
				!affected_by_spell(ch, SPELL_DERMAL_HARDENING))
				cast_spell(ch, ch, 0, SPELL_DERMAL_HARDENING);
			else if (GET_LEVEL(ch) >= 30 &&
				!AFF3_FLAGGED(ch, AFF3_PSISHIELD) &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_PSISHIELD))
				cast_spell(ch, ch, 0, SPELL_PSISHIELD);
			else if (GET_LEVEL(ch) >= 20 &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_PSYCHIC_RESISTANCE) &&
				!affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
				cast_spell(ch, ch, 0, SPELL_PSYCHIC_RESISTANCE);
			else if (GET_LEVEL(ch) >= 15 &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_POWER) &&
				!affected_by_spell(ch, SPELL_POWER))
				cast_spell(ch, ch, 0, SPELL_POWER);
			else if (GET_LEVEL(ch) >= 15 &&
				GET_MANA(ch) > mag_manacost(ch, SPELL_CONFIDENCE) &&
				!AFF_FLAGGED(ch, AFF_CONFIDENCE))
				cast_spell(ch, ch, 0, SPELL_CONFIDENCE);
		}
		//
		// barbs acting barbaric
		//

		else if (cur_class == CLASS_BARB || cur_class == CLASS_HILL || GET_RACE(ch) == RACE_DEMON ) {
            barbarian_activity(ch); 
		}

        else if(GET_RACE(ch) == RACE_CELESTIAL || GET_RACE(ch) == RACE_ARCHON || GET_RACE(ch) == RACE_DEVIL){
            knight_activity(ch);
        }

        else if(GET_RACE(ch) == RACE_GUARDINAL){
            ranger_activity(ch);
        }
		//
		// elementals heading home
		//

		if (GET_RACE(ch) == RACE_ELEMENTAL) {
			int sect;
			sect = ch->in_room->sector_type;

			if (((GET_MOB_VNUM(ch) >= 1280 &&
						GET_MOB_VNUM(ch) <= 1283) ||
					GET_MOB_VNUM(ch) == 5318) && !ch->master)
				k = 1;
			else
				k = 0;
			found = 0;

			switch (GET_CLASS(ch)) {
			case CLASS_EARTH:
				if (k
					|| sect == SECT_WATER_SWIM
					|| sect == SECT_WATER_NOSWIM
					|| ch->in_room->isOpenAir()
					|| sect == SECT_UNDERWATER
					|| sect == SECT_DEEP_OCEAN) {
					found = 1;
					act("$n dissolves, and returns to $s home plane!",
						TRUE, ch, 0, 0, TO_ROOM);
					//extract_char(ch, FALSE);
					ch->purge(true);
				}
				break;
			case CLASS_FIRE:
				if ((k
						|| sect == SECT_WATER_SWIM
						|| sect == SECT_WATER_NOSWIM
						|| ch->in_room->isOpenAir()
						|| sect == SECT_UNDERWATER
						|| sect == SECT_DEEP_OCEAN
						|| (OUTSIDE(ch)
							&& ch->in_room->zone->weather->sky == SKY_RAINING))
					&& !ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)) {
					found = 1;
					act("$n dissipates, and returns to $s home plane!",
						TRUE, ch, 0, 0, TO_ROOM);
					//extract_char(ch, FALSE);
					ch->purge(true);
				}
				break;
			case CLASS_WATER:
				if (k ||
					(sect != SECT_WATER_SWIM &&
						sect != SECT_WATER_NOSWIM &&
						sect != SECT_UNDERWATER &&
						sect != SECT_DEEP_OCEAN &&
						ch->in_room->zone->weather->sky != SKY_RAINING)) {
					found = 1;
					act("$n dissipates, and returns to $s home plane!",
						TRUE, ch, 0, 0, TO_ROOM);
					//extract_char(ch, FALSE);
					ch->purge(true);
				}
				break;
			case CLASS_AIR:
				if (k
						|| sect == SECT_UNDERWATER
						|| sect == SECT_DEEP_OCEAN
						|| sect == SECT_PITCH_SUB) {
					found = 1;
					act("$n dissipates, and returns to $s home plane!",
						TRUE, ch, 0, 0, TO_ROOM);
					//extract_char(ch, FALSE);
					ch->purge(true);
				}
				break;
			default:
				if (k) {
					found = 1;
					act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
					//extract_char(ch, FALSE);
					ch->purge(true);
				}
			}

			if (found)
				continue;
		}
		//
		// birds fluttering around
		//

		if (GET_RACE(ch) == RACE_ANIMAL &&
			GET_CLASS(ch) == CLASS_BIRD &&
			IS_AFFECTED(ch, AFF_INFLIGHT) &&
			!ch->in_room->isOpenAir() && random_fractional_3()) {
			if (GET_CLASS(ch) == CLASS_BIRD && IS_AFFECTED(ch, AFF_INFLIGHT) &&
				!ch->in_room->isOpenAir()) {
				if (ch->getPosition() == POS_FLYING && random_fractional_5()) {
					act("$n flutters to the ground.", TRUE, ch, 0, 0, TO_ROOM);
					ch->setPosition(POS_STANDING);
				} else if (ch->getPosition() == POS_STANDING
					&& !IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL)) {
					act("$n flaps $s wings and takes flight.", TRUE, ch, 0, 0,
						TO_ROOM);
					ch->setPosition(POS_FLYING);
				}
				continue;
			}
		}
		//
		// non-birds flying around
		//

		if (IS_AFFECTED(ch, AFF_INFLIGHT) && random_fractional_10()) {
			if (!ch->in_room->isOpenAir()) {
				if (ch->getPosition() == POS_FLYING && random_fractional_10()) {
					ch->setPosition(POS_STANDING);
					if (!can_travel_sector(ch, ch->in_room->sector_type, 1)) {
						ch->setPosition(POS_FLYING);
						continue;
					} else if (FLOW_TYPE(ch->in_room) != F_TYPE_SINKING_SWAMP)
						act("$n settles to the ground.", TRUE, ch, 0, 0,
							TO_ROOM);

				} else if (ch->getPosition() == POS_STANDING
					&& random_fractional_4()) {

					act("$n begins to hover in midair.", TRUE, ch, 0, 0,
						TO_ROOM);
					ch->setPosition(POS_FLYING);
				}
				continue;
			}
		}
		/* Add new mobile actions here */
		/* end for() */


	}
}

#define SAME_ALIGN(a, b) ( ( IS_GOOD(a) && IS_GOOD(b) ) || ( IS_EVIL(a) && IS_EVIL(b) ) )

struct Creature *
choose_opponent(struct Creature *ch, struct Creature *ignore_vict)
{

	struct Creature *vict = NULL;
	struct Creature *best_vict = NULL;

	// first look for someone who is fighting us
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		vict = *it;
		// ignore bystanders
		if (!vict->findCombat(ch))
			continue;

		//
		// ignore the specified ignore_vict
		//

		if (vict == ignore_vict)
			continue;

		// look for opposite/neutral aligns only
		if (SAME_ALIGN(ch, vict))
			continue;

		// pick the weakest victim in hopes of killing one
		if (!best_vict || GET_HIT(vict) < GET_HIT(best_vict)) {
			best_vict = vict;
			continue;
		}
		// if victim's AC is worse, prefer to attack him
		if (!best_vict || GET_AC(vict) > GET_AC(best_vict)) {
			best_vict = vict;
			continue;
		}
	}

	// default to fighting opponent
	if (!best_vict)
		best_vict = ch->findRandomCombat();

	return (best_vict);
}

bool
detect_opponent_master(Creature *ch, Creature *opp)
{
	if (!opp->findCombat(ch))
		return false;
	if (IS_PC(ch) || IS_PC(opp))
		return false;
	if (!opp->master)
		return false;
	if (ch->master == opp->master)
		return false;
	if (GET_MOB_WAIT(ch) >= 10)
		return false;
	if (!can_see_creature(ch, opp))
		return false;
	if (opp->master->in_room != ch->in_room)
		return false;
	if (number(0, 50 - GET_INT(ch) - GET_WIS(ch)))
		return false;

	ch->removeCombat(opp);
	set_fighting(ch, opp->master, false);
	return true;
}

//
// return value like damage()
// if DAM_VICT_KILLED is set, then the victim was FIGHTING(ch)
//
// IMPORTANT: mobile_battle_activity can kill other chars than just FIGHTING(ch)!!
// use precious_vict if you must protect one in a loop or something
//
// TODO: finish implementing return values
//

int
mobile_battle_activity(struct Creature *ch, struct Creature *precious_vict)
{

	register struct Creature *vict = NULL, *new_mob = NULL;
	int num = 0, prob = 0, dam = 0;
	struct obj_data *weap = GET_EQ(ch, WEAR_WIELD), *gun = NULL;
	int return_flags = 0;

	ACCMD(do_disarm);
	ACMD(do_feign);

	if (!ch->numCombatants()) {
		errlog("mobile_battle_activity called on non-fighting ch.");
		raise(SIGSEGV);
	}

	if (ch->findCombat(precious_vict)) {
		errlog("FIGHTING(ch) == precious_vict in mobile_battle_activity()");
		return 0;
	}

	if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
		return 0;

	if (IS_TARRASQUE(ch)) {	/* tarrasque */
		tarrasque_fight(ch);
		return 0;
	}

	if (!IS_ANIMAL(ch)) {
        Creature *target = ch->findRandomCombat();
		// Speaking mobiles
		if (GET_HIT(ch) < GET_MAX_HIT(ch) >> 7 && random_fractional_20()) {
			int pct = random_percentage_zero_low();

			if (pct < 20) {
				act("$n grits $s teeth as $e begins to weaken.", false,
					ch, 0, 0, TO_ROOM);
			} else {
				if (target && can_see_creature(target, ch))
					do_say(ch,
						tmp_sprintf("%s you bastard!", PERS(target, ch)),
						0, 0, 0);
				else
					do_say(ch, "You stinking bastard!", 0, 0, 0);
			}

			return 0;
		}

		if (target && GET_HIT(target) < GET_MAX_HIT(target) >> 7 &&
			GET_HIT(ch) > GET_MAX_HIT(ch) >> 4 && random_fractional_20()) {

			int pct = random_percentage_zero_low();

			if (pct < 20) {
				do_say(ch, "Let this be a lesson to you!", 0, 0, 0);
				return 0;
			} else if (pct < 50) {
				do_say(ch, "So you thought you were tough, eh?", 0, 0, 0);
				return 0;
			} else if (pct < 60) {
				do_say(ch,
					tmp_sprintf("Kiss my ass, %s!", PERS(ch, target)),
					0, 0, 0);
				return 0;
			}
		}
	}

    list <CharCombat>::iterator li;
    li = ch->getCombatList()->begin();
    for (; li != ch->getCombatList()->end(); ++li)
	    if (detect_opponent_master(ch, li->getOpponent()))
		    return 0;

	/* Here go the fighting Routines */

	if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {

		if (((gun = GET_EQ(ch, WEAR_WIELD)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_EQ(ch, WEAR_WIELD_2)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_IMPLANT(ch, WEAR_WIELD)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
			((gun = GET_IMPLANT(ch, WEAR_WIELD_2)) &&
				((IS_GUN(gun) && GUN_LOADED(gun)) ||
					(IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun))))) {

			CUR_R_O_F(gun) = MAX_R_O_F(gun);

            if (ch->numCombatants())
			    do_shoot(ch, tmp_sprintf("%s %s",
				         fname(gun->aliases), ch->findRandomCombat()->player.name),
				         0, 0, &return_flags);
			return return_flags;
		}
	}

	if (GET_CLASS(ch) == CLASS_SNAKE) {
        Creature *target = ch->findRandomCombat();
		if (target && target->in_room == ch->in_room &&
			(random_number_zero_low(42 - GET_LEVEL(ch)) == 0)) {
			act("$n bites $N!", 1, ch, 0, target, TO_NOTVICT);
			act("$n bites you!", 1, ch, 0, target, TO_VICT);
			if (random_fractional_10()) {
				call_magic(ch, target, 0, SPELL_POISON, GET_LEVEL(ch),
					CAST_CHEM, &return_flags);
			}
			return return_flags;
		}
	}

	/** RACIAL ATTACKS FIRST **/

	// wemics 'leap' out of battle, only to return via the hunt for the kill
	if (IS_RACE(ch, RACE_WEMIC) && GET_MOVE(ch) > (GET_MAX_MOVE(ch) >> 1)) {

		// leap
		if (random_fractional_3()) {
			if (GET_HIT(ch) > (GET_MAX_HIT(ch) >> 1) &&
				!ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK)) {

				for (int dir = 0; dir < NUM_DIRS; ++dir) {
					if (CAN_GO(ch, dir) &&
						!ROOM_FLAGGED(EXIT(ch, dir)->to_room,
							ROOM_DEATH | ROOM_NOMOB |
							ROOM_NOTRACK) &&
						EXIT(ch, dir)->to_room != ch->in_room &&
						CHAR_LIKES_ROOM(ch, EXIT(ch, dir)->to_room)
						&& random_binary()) {

						Creature *was_fighting = ch->findRandomCombat();

                        ch->removeAllCombat();
                        CreatureList::iterator ci = ch->in_room->people.begin();
                        for (; ci != ch->in_room->people.end(); ++ci) {
                            if ((*ci)->findCombat(ch)) {
                                (*ci)->removeCombat(ch);
                            }
                        }

						//
						// leap in this direction
						//
						act(tmp_sprintf("$n leaps over you to the %s!",
								dirs[dir]),
							false, ch, 0, 0, TO_ROOM);

						struct room_data *to_room = EXIT(ch, dir)->to_room;
						char_from_room(ch,false);
						char_to_room(ch, to_room,false);

						act(tmp_sprintf("$n leaps in from %s!",
								from_dirs[dir]),
							false, ch, 0, 0, TO_ROOM);

                        if (was_fighting)
						    HUNTING(ch) = was_fighting;
						return 0;
					}
				}
			}
		}
		// paralyzing scream

		else if (random_binary()) {
			act("$n releases a deafening scream!!", FALSE, ch, 0, 0, TO_ROOM);
            list<CharCombat>::iterator li = ch->getCombatList()->end();
            for (; li != ch->getCombatList()->end(); ++li) {
			    call_magic(ch, li->getOpponent(), 0, SPELL_FEAR, GET_LEVEL(ch),
				    CAST_BREATH, &return_flags);
            }
			return return_flags;
		}
	}


	if (IS_RACE(ch, RACE_UMBER_HULK)) {
        Creature *target = ch->findRandomCombat();
		if (target && random_fractional_3() && !AFF_FLAGGED(target, AFF_CONFUSION)) {
			call_magic(ch, target, 0, SPELL_CONFUSION, GET_LEVEL(ch),
				CAST_ROD, &return_flags);
			return return_flags;
		}
	}

	if (IS_RACE(ch, RACE_DAEMON)) {
		if (GET_CLASS(ch) == CLASS_DAEMON_PYRO) {
            Creature *target = ch->findRandomCombat();
			if (target && random_fractional_3()) {
				WAIT_STATE(ch, 3 RL_SEC);
				call_magic(ch, ch->findRandomCombat(), 0, SPELL_FIRE_BREATH,
					GET_LEVEL(ch), CAST_BREATH, &return_flags);
				return return_flags;
			}
		}
	}

	if (IS_RACE(ch, RACE_MEPHIT)) {
		if (GET_CLASS(ch) == CLASS_MEPHIT_LAVA) {
            Creature *target = ch->findRandomCombat();
			if (target && random_binary()) {
				return damage(ch, ch->findRandomCombat(), 
                              dice(20, 20), TYPE_LAVA_BREATH,
					          WEAR_RANDOM);
			}
		}
	}

	if (IS_MANTICORE(ch)) {

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		dam = dice(GET_LEVEL(ch), 5);
		if (GET_LEVEL(ch) + random_number_zero_low(50) >
			GET_LEVEL(vict) - GET_AC(ch) / 2)
			dam = 0;

		WAIT_STATE(ch, 2 RL_SEC);
		return damage(ch, vict, dam, SPELL_MANTICORE_SPIKES, WEAR_RANDOM);
	}

    /* Devils: Denezens of the 9 hells */
    /* Devils act like knights when not acting like devils */
	if (IS_DEVIL(ch)) {
        if( random_number_zero_low(8) < 1 ){
            if( knight_battle_activity(ch, precious_vict) == 0 ){
            }
        } else if (mob_fight_devil(ch, precious_vict)){
            return 0;
        }
	}

    /* Celestials and archons:  Denezens of the  7 heavens */
    /* Archons act like knights unless otherwize told */
    if(GET_RACE(ch) == RACE_CELESTIAL || GET_RACE(ch) == RACE_ARCHON){
        if(random_number_zero_low(8) < 1){
            if(knight_battle_activity(ch, precious_vict) == 0){
                return 0;
            }
        } else if(mob_fight_celestial(ch, precious_vict)){
            return 0;
        }
    }

    /* Guardinal : Denezins of the Elysian Planes */
    /* Guardinal's act like rangers unless otherwise told */
    if(GET_RACE(ch) == RACE_GUARDINAL){
        if(random_number_zero_low(8) < 1){
            if(ranger_battle_activity(ch, precious_vict) == 0){
                return 0;
            }
        } else if(mob_fight_guardinal(ch, precious_vict)){
            return 0;
        }
    }

    if( GET_RACE(ch) == RACE_DEMON ){
        if( random_number_zero_low(8) < 1 ){
            if( barbarian_battle_activity(ch, precious_vict) == 0 ){
                return 0;
            }
        }else if( mob_fight_demon( ch, precious_vict )){
            return 0;
        }
    }

	/* Slaad */
	if (IS_SLAAD(ch)) {
		num = 12;
		new_mob = NULL;

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it)
			if (IS_SLAAD((*it)))
				num++;

		switch (GET_CLASS(ch)) {
		case CLASS_SLAAD_BLUE:
			if (!IS_PET(ch)) {
				if (!random_number_zero_low(3 * num)
					&& ch->mob_specials.shared->number > 1)
					new_mob = read_mobile(GET_MOB_VNUM(ch));
				else if (!random_number_zero_low(3 * num))	/* red saad */
					new_mob = read_mobile(42000);
			}
			break;
		case CLASS_SLAAD_GREEN:
			if (!IS_PET(ch)) {
				if (!random_number_zero_low(4 * num)
					&& ch->mob_specials.shared->number > 1)
					new_mob = read_mobile(GET_MOB_VNUM(ch));
				else if (!random_number_zero_low(3 * num))	/* blue slaad */
					new_mob = read_mobile(42001);
				else if (!random_number_zero_low(2 * num))	/* red slaad */
					new_mob = read_mobile(42000);
			}
			break;
		case CLASS_SLAAD_GREY:
			if (GET_EQ(ch, WEAR_WIELD) &&
				(IS_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD), ITEM_WEAPON) &&
					(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) ==
						TYPE_SLASH - TYPE_HIT) && random_fractional_4()) &&
				GET_MOVE(ch) > 20) {
				perform_offensive_skill(ch, vict, SKILL_BEHEAD, &return_flags);
				return 0;
			}
			if (!IS_PET(ch)) {
				if (!random_number_zero_low(4 * num)
					&& ch->mob_specials.shared->number > 1)
					new_mob = read_mobile(GET_MOB_VNUM(ch));
				else if (!random_number_zero_low(4 * num))	/* green slaad */
					new_mob = read_mobile(42002);
				else if (!random_number_zero_low(3 * num))	/* blue slaad */
					new_mob = read_mobile(42001);
				else if (!random_number_zero_low(2 * num))	/* red slaad */
					new_mob = read_mobile(42000);
			}
			break;
		case CLASS_SLAAD_DEATH:
		case CLASS_SLAAD_LORD:
			if (!IS_PET(ch)) {
				if (!random_number_zero_low(4 * num))	/* grey slaad */
					new_mob = read_mobile(42003);
				else if (!random_number_zero_low(3 * num))	/* green slaad */
					new_mob = read_mobile(42002);
				else if (!random_number_zero_low(3 * num))	/* blue slaad */
					new_mob = read_mobile(42001);
				else if (!random_number_zero_low(4 * num))	/* red slaad */
					new_mob = read_mobile(42000);
			}
			break;
		}
		if (new_mob) {
			WAIT_STATE(ch, 5 RL_SEC);
			GET_MOVE(ch) -= 100;
			char_to_room(new_mob, ch->in_room,false);
			act("$n gestures, a glowing portal appears with a whine!",
				FALSE, ch, 0, 0, TO_ROOM);
			act("$n steps out of the portal with a crack of lightning!",
				FALSE, new_mob, 0, 0, TO_ROOM);
            Creature *target = ch->findRandomCombat();
			if (target && IS_MOB(target))
				hit(new_mob, target, TYPE_UNDEFINED);
			return 0;
		}
	}
	if (IS_TROG(ch)) {
		if (random_fractional_5()) {
			act("$n begins to secrete a disgustingly malodorous oil!",
				FALSE, ch, 0, 0, TO_ROOM);
			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it)
				if (!IS_TROG((*it)) &&
					!IS_UNDEAD(vict) && !IS_ELEMENTAL((*it))
					&& !IS_DRAGON((*it)))
					call_magic(ch, (*it), 0, SPELL_TROG_STENCH, GET_LEVEL(ch),
						CAST_POTION);
			return 0;
		}
	}
	if (IS_UNDEAD(ch)) {
		if (GET_CLASS(ch) == CLASS_GHOUL) {
			if (random_fractional_10())
				act(" $n emits a terrible shriek!!", FALSE, ch, 0, 0, TO_ROOM);
			else if (random_fractional_5())
				call_magic(ch, vict, 0, SPELL_CHILL_TOUCH, GET_LEVEL(ch),
					CAST_SPELL);
		}
	}

	if (IS_DRAGON(ch)) {
		if (random_number_zero_low(GET_LEVEL(ch)) > 40) {
			act("You feel a wave of sheer terror wash over you as $n approaches!", FALSE, ch, 0, 0, TO_ROOM);
            list<CharCombat>::iterator li = ch->getCombatList()->begin();
            for (; li != ch->getCombatList()->end(); ++li) {
                if ((vict = li->getOpponent())) {
                    if (!mag_savingthrow(vict, GET_LEVEL(ch), SAVING_SPELL) &&
                        !IS_AFFECTED(vict, AFF_CONFIDENCE))
                        do_flee(vict, "", 0, 0, 0);
                }
            }
			return 0;
		}

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		if (random_fractional_4()) {
			damage(ch, vict, random_number_zero_low(GET_LEVEL(ch)), TYPE_CLAW,
				WEAR_RANDOM);
			return 0;
		} else if (random_fractional_4() && ch->findCombat(vict)) {
			damage(ch, vict, random_number_zero_low(GET_LEVEL(ch)), TYPE_CLAW,
				WEAR_RANDOM);
			return 0;
		} else if (random_fractional_3()) {
			damage(ch, vict, random_number_zero_low(GET_LEVEL(ch)), TYPE_BITE,
				WEAR_RANDOM);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return 0;
		} else if (random_fractional_3()) {
			switch (GET_CLASS(ch)) {
			case CLASS_GREEN:
				call_magic(ch, vict, 0, SPELL_GAS_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;

			case CLASS_BLACK:
				call_magic(ch, vict, 0, SPELL_ACID_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			case CLASS_BLUE:
				call_magic(ch, vict, 0,
					SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			case CLASS_WHITE:
			case CLASS_SILVER:
				call_magic(ch, vict, 0, SPELL_FROST_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			case CLASS_RED:
				call_magic(ch, vict, 0, SPELL_FIRE_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			case CLASS_SHADOW_D:
				call_magic(ch, vict, 0, SPELL_SHADOW_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			case CLASS_TURTLE:
				call_magic(ch, vict, 0, SPELL_STEAM_BREATH, GET_LEVEL(ch),
					CAST_BREATH);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
				break;
			}
			return 0;
		}
	}
	if (GET_RACE(ch) == RACE_ELEMENTAL && random_fractional_4()) {

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		prob =
			GET_LEVEL(ch) - GET_LEVEL(vict) + GET_AC(vict) + GET_HITROLL(ch);
		if (prob > (random_percentage() - 50))
			dam = random_number_zero_low(GET_LEVEL(ch)) + (GET_LEVEL(ch) >> 1);
		else
			dam = 0;
		switch (GET_CLASS(ch)) {
		case CLASS_EARTH:
			if (random_fractional_3()) {
				damage(ch, vict, dam, SPELL_EARTH_ELEMENTAL, WEAR_RANDOM);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			} else if (random_fractional_5()) {
				damage(ch, vict, dam, SKILL_BODYSLAM, WEAR_BODY);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
			break;
		case CLASS_FIRE:
			if (random_fractional_3()) {
				damage(ch, vict, dam, SPELL_FIRE_ELEMENTAL, WEAR_RANDOM);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
			break;
		case CLASS_WATER:
			if (random_fractional_3()) {
				damage(ch, vict, dam, SPELL_WATER_ELEMENTAL, WEAR_RANDOM);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
			break;
		case CLASS_AIR:
			if (random_fractional_3()) {
				damage(ch, vict, dam, SPELL_AIR_ELEMENTAL, WEAR_RANDOM);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
			break;
		}
		return 0;
	}


	/** CLASS ATTACKS HERE **/
	int cur_class = 0;
	if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_binary())
		cur_class = GET_REMORT_CLASS(ch);
	else
		cur_class = GET_CLASS(ch);
	if (cur_class == CLASS_PSIONIC
		&& !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)) {
		if (mob_fight_psionic(ch, precious_vict))
			return 0;
	}

	if (cur_class == CLASS_CYBORG) {

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		if (random_fractional_3() == 1) {
			if (GET_MOVE(ch) < 200 && random_binary()) {
				do_de_energize(ch, GET_NAME(vict), 0, 0, 0);
				return 0;
			}
			if (GET_LEVEL(ch) >= 14 && random_binary()) {
				if (GET_MOVE(ch) > 50 && random_binary()) {
					do_discharge(ch, tmp_sprintf("%d %s", GET_LEVEL(ch) / 3,
							fname(vict->player.name)),
						0, 0, 0);
					return 0;
				}
			} else if (GET_LEVEL(ch) >= 9) {
				perform_offensive_skill(ch, vict, SKILL_KICK, &return_flags);
				return 0;
			}
		}

	}


	if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {

		if ((cur_class == CLASS_MAGE || IS_LICH(ch))) {

			if (!(vict = choose_opponent(ch, precious_vict)))
				return 0;

			if (ch->findCombat(vict) &&
				GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2) &&
				GET_HIT(vict) > (GET_MAX_HIT(vict) >> 1)) {
				if (GET_LEVEL(ch) >= 33 && random_fractional_3() &&
					GET_MANA(ch) > mag_manacost(ch, SPELL_TELEPORT)) {
					cast_spell(ch, vict, NULL, SPELL_TELEPORT);
					return 0;
				} else if (GET_LEVEL(ch) >= 18 &&
					GET_MANA(ch) > mag_manacost(ch, SPELL_LOCAL_TELEPORT)) {
					cast_spell(ch, vict, NULL, SPELL_LOCAL_TELEPORT);
					return 0;
				}
			}

			if ((GET_LEVEL(ch) > 18) && random_fractional_5() &&
				!IS_AFFECTED_2(ch, AFF2_FIRE_SHIELD)) {
				cast_spell(ch, ch, NULL, SPELL_FIRE_SHIELD);
				return 0;
			} else if ((GET_LEVEL(ch) > 14) && random_fractional_5() &&
				!IS_AFFECTED(ch, AFF_BLUR)) {
				cast_spell(ch, ch, NULL, SPELL_BLUR);
				return 0;
			} else if ((GET_LEVEL(ch) > 5) && random_fractional_5() &&
				!affected_by_spell(ch, SPELL_ARMOR)) {
				cast_spell(ch, ch, NULL, SPELL_ARMOR);
				return 0;
			} else if ((GET_LEVEL(ch) >= 38) && !AFF2_FLAGGED(vict, AFF2_SLOW)
				&& random_binary()) {
				cast_spell(ch, vict, NULL, SPELL_SLOW);
				return 0;
			} else if ((GET_LEVEL(ch) > 12) && random_fractional_10()) {
				if (IS_EVIL(ch)) {
					cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
					return 0;
				} else if (IS_GOOD(ch) && IS_EVIL(vict)) {
					cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
					return 0;
				}
			} else if (random_fractional_4()) {
				command_interpreter(ch, "cackle");

				if (random_fractional_5()) {

					if (GET_LEVEL(ch) < 6) {
						cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
					} else if (GET_LEVEL(ch) < 8) {
						cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
					} else if (GET_LEVEL(ch) < 12) {
						cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
					} else if (GET_LEVEL(ch) < 15) {
						cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
					} else if (GET_LEVEL(ch) < 20) {
						cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
					} else if (GET_LEVEL(ch) < 33) {
						cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
					} else if (GET_LEVEL(ch) < 43) {
						cast_spell(ch, vict, NULL, SPELL_FIREBALL);
					} else {
						cast_spell(ch, vict, NULL, SPELL_PRISMATIC_SPRAY);
					}
					return 0;
				}
			}
		}


		if (cur_class == CLASS_CLERIC &&
			!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {

			if (!(vict = choose_opponent(ch, precious_vict)))
				return 0;

			if ((GET_LEVEL(ch) > 2) && random_fractional_10() &&
				!affected_by_spell(ch, SPELL_ARMOR)) {
				cast_spell(ch, ch, NULL, SPELL_ARMOR);
				return 0;
			} else if ((GET_HIT(ch) / MAX(1,
						GET_MAX_HIT(ch))) < (GET_MAX_HIT(ch) >> 2)) {
				if ((GET_LEVEL(ch) < 12) && random_fractional_10()) {
					cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
					return 0;
				} else if ((GET_LEVEL(ch) < 24) && random_fractional_10()) {
					cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
					return 0;
				} else if ((GET_LEVEL(ch) <= 34) && random_fractional_10()) {
					cast_spell(ch, ch, NULL, SPELL_HEAL);
					return 0;
				} else if ((GET_LEVEL(ch) > 34) && random_fractional_10()) {
					cast_spell(ch, ch, NULL, SPELL_GREATER_HEAL);
					return 0;
				}
			}
			if ((GET_LEVEL(ch) > 12) && random_fractional_10()) {
				if (IS_EVIL(ch) && IS_GOOD(vict))
					cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD);
				else if (IS_GOOD(ch) && IS_EVIL(vict))
					cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
				return 0;
			}

			if (random_fractional_5()) {

				if (GET_LEVEL(ch) < 10)
					return 0;
				if (GET_LEVEL(ch) < 26) {
					cast_spell(ch, vict, NULL, SPELL_SPIRIT_HAMMER);
				} else if (GET_LEVEL(ch) < 31) {
					cast_spell(ch, vict, NULL, SPELL_CALL_LIGHTNING);
				} else if (GET_LEVEL(ch) < 36) {
					cast_spell(ch, vict, NULL, SPELL_HARM);
				} else {
					cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
				}
				return 0;
			}
		}
	}

	if (cur_class == CLASS_BARB || IS_GIANT(ch)) {
       if( barbarian_battle_activity( ch, precious_vict) == 0 ){
           return 0;
       }
	}

	if (cur_class == CLASS_WARRIOR) {

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		if (can_see_creature(ch, vict) &&
			(IS_MAGE(vict) || IS_CLERIC(vict))
			&& vict->getPosition() > POS_SITTING) {
			perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
			return return_flags;
		}

		if ((GET_LEVEL(ch) > 37) && vict->getPosition() > POS_SITTING &&
			random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) >= 20) &&
			GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
			random_fractional_5()) {
			do_disarm(ch, PERS(vict, ch), 0, 0, 0);
			return 0;
		} else if ((GET_LEVEL(ch) > 9) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_ELBOW, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 5) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_STOMP, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 2) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_PUNCH, &return_flags);
			return return_flags;
		}

		if (random_fractional_5()) {

			if (GET_LEVEL(ch) < 3) {
				perform_offensive_skill(ch, vict, SKILL_PUNCH, &return_flags);
			} else if (GET_LEVEL(ch) < 5) {
				perform_offensive_skill(ch, vict, SKILL_KICK, &return_flags);
			} else if (GET_LEVEL(ch) < 7) {
				perform_offensive_skill(ch, vict, SKILL_STOMP, &return_flags);
			} else if (GET_LEVEL(ch) < 9) {
				perform_offensive_skill(ch, vict, SKILL_KNEE, &return_flags);
			} else if (GET_LEVEL(ch) < 14) {
				perform_offensive_skill(ch, vict, SKILL_UPPERCUT, &return_flags);
			} else if (GET_LEVEL(ch) < 16) {
				perform_offensive_skill(ch, vict, SKILL_ELBOW, &return_flags);
			} else if (GET_LEVEL(ch) < 24) {
				perform_offensive_skill(ch, vict, SKILL_HOOK, &return_flags);
			} else if (GET_LEVEL(ch) < 36) {
				perform_offensive_skill(ch, vict, SKILL_SIDEKICK, &return_flags);
			}
			return return_flags;
		}
	}

	if (cur_class == CLASS_THIEF) {

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		if (GET_LEVEL(ch) > 18 && vict->getPosition() >= POS_FIGHTING
			&& random_fractional_4()) {
			perform_offensive_skill(ch, vict, SKILL_TRIP, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) >= 20) &&
			GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
			random_fractional_5()) {
			do_disarm(ch, PERS(vict, ch), 0, 0, 0);
			return 0;
		} else if (GET_LEVEL(ch) > 29 && random_fractional_4()) {
			perform_offensive_skill(ch, vict, SKILL_GOUGE, &return_flags);
			return return_flags;
		} else if (GET_LEVEL(ch) > 24 && random_fractional_4()) {
			do_feign(ch, "", 0, 0, 0);
			do_hide(ch, "", 0, 0, 0);
			SET_BIT(MOB_FLAGS(ch), MOB_MEMORY);
			remember(ch, vict);
			return 0;
		} else if (weap &&
			((GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT) ||
				(GET_OBJ_VAL(weap, 3) == TYPE_STAB - TYPE_HIT)) &&
			CHECK_SKILL(ch, SKILL_CIRCLE) > 40) {
			do_circle(ch, fname(vict->player.name), 0, 0, 0);
			return 0;
		} else if ((GET_LEVEL(ch) > 9) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_ELBOW, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 5) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_STOMP, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 2) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_PUNCH, &return_flags);
			return return_flags;
		}

		if (random_fractional_5()) {

			if (GET_LEVEL(ch) < 3) {
				perform_offensive_skill(ch, vict, SKILL_PUNCH, &return_flags);
			} else if (GET_LEVEL(ch) < 7) {
				perform_offensive_skill(ch, vict, SKILL_STOMP, &return_flags);
			} else if (GET_LEVEL(ch) < 9) {
				perform_offensive_skill(ch, vict, SKILL_KNEE, &return_flags);
			} else if (GET_LEVEL(ch) < 16) {
				perform_offensive_skill(ch, vict, SKILL_ELBOW, &return_flags);
			} else if (GET_LEVEL(ch) < 24) {
				perform_offensive_skill(ch, vict, SKILL_HOOK, &return_flags);
			} else if (GET_LEVEL(ch) < 36) {
				perform_offensive_skill(ch, vict, SKILL_SIDEKICK, &return_flags);
			}
			return return_flags;
		}
	}
	if (cur_class == CLASS_RANGER) {
        if(ranger_battle_activity(ch, precious_vict) == 0){
            return 0;
        }
	}

    ///Knights act offesive, through a function! (bwahahaha)
    //defined below
	if (cur_class == CLASS_KNIGHT) {
        if(knight_battle_activity(ch, precious_vict) == 0){
            return 0;
        }
    }


	/* add new mobile fight routines here. */
	if (cur_class == CLASS_MONK) {
		if (GET_LEVEL(ch) >= 23 &&
			GET_MOVE(ch) < (GET_MAX_MOVE(ch) >> 1) && GET_MANA(ch) > 100
			&& random_fractional_5()) {
			do_battlecry(ch, "", 1, SCMD_KIA, 0);
			return 0;
		}
		// Monks are way to nasty. Make them twiddle thier thumbs a tad bit.
		if (random_fractional_5())
			return 0;

		if (!(vict = choose_opponent(ch, precious_vict)))
			return 0;

		if (random_fractional_3() && (GET_CLASS(vict) == CLASS_MAGIC_USER ||
				GET_CLASS(vict) == CLASS_CLERIC)) {
			if (GET_LEVEL(ch) >= 27)
				perform_offensive_skill(ch, vict, SKILL_HIP_TOSS, &return_flags);
			else if (GET_LEVEL(ch) >= 25)
				perform_offensive_skill(ch, vict, SKILL_SWEEPKICK, &return_flags);
			return return_flags;
		}
		if ((GET_LEVEL(ch) >= 49) && (random_fractional_5() ||
				vict->getPosition() < POS_FIGHTING)) {
			perform_offensive_skill(ch, vict, SKILL_DEATH_TOUCH, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 33) && random_fractional_5()) {
			do_combo(ch, GET_NAME(vict), 0, 0, 0);
			return 0;
		} else if ((GET_LEVEL(ch) > 27) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_HIP_TOSS, &return_flags);
			return return_flags;
		} else if (random_fractional_5()
				&& !affected_by_spell(vict, SKILL_PINCH_EPSILON)
				&& (GET_LEVEL(ch) > 26)) {
			do_pinch(ch, tmp_sprintf("%s epsilon", fname(vict->player.name)), 0, 0, 0);
			return 0;
		} else if (random_fractional_5()
				&& !affected_by_spell(vict, SKILL_PINCH_DELTA)
				&& (GET_LEVEL(ch) > 19)) {
			do_pinch(ch, tmp_sprintf("%s delta", fname(vict->player.name)), 0, 0, 0);
			return 0;
		} else if (random_fractional_5()
				&& !affected_by_spell(vict, SKILL_PINCH_BETA)
				&& (GET_LEVEL(ch) > 12)) {
			do_pinch(ch, tmp_sprintf("%s beta", fname(vict->player.name)), 0, 0, 0);
			return 0;
		} else if (random_fractional_5()
				&& !affected_by_spell(vict, SKILL_PINCH_ALPHA)
				&& (GET_LEVEL(ch) > 6)) {
			do_pinch(ch, tmp_sprintf("%s alpha", fname(vict->player.name)), 0, 0, 0);
			return 0;
		} else if ((GET_LEVEL(ch) > 30) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_RIDGEHAND, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 25) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_SWEEPKICK, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 36) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_GOUGE, &return_flags);
			return return_flags;
		} else if ((GET_LEVEL(ch) > 10) && random_fractional_5()) {
			perform_offensive_skill(ch, vict, SKILL_THROAT_STRIKE, &return_flags);
			return return_flags;
		}
	}
	return 0;
}


/* Mob Memory Routines */

/* make ch remember victim */
void
remember(struct Creature *ch, struct Creature *victim)
{
	memory_rec *tmp;
	int present = FALSE;

	if (!IS_NPC(ch) || (!MOB2_FLAGGED(ch, MOB2_ATK_MOBS) && IS_NPC(victim)))
		return;

	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
		if (tmp->id == GET_IDNUM(victim))
			present = TRUE;

	if (!present) {
		CREATE(tmp, memory_rec, 1);
		tmp->next = MEMORY(ch);
		tmp->id = GET_IDNUM(victim);
		MEMORY(ch) = tmp;
	}
}


/* make ch forget victim */
void
forget(struct Creature *ch, struct Creature *victim)
{
	memory_rec *curr, *prev = NULL;

	if (!(curr = MEMORY(ch)))
		return;

	while (curr && curr->id != GET_IDNUM(victim)) {
		prev = curr;
		curr = curr->next;
	}

	if (!curr)
		return;					/* person wasn't there at all. */

	if (curr == MEMORY(ch))
		MEMORY(ch) = curr->next;
	else
		prev->next = curr->next;
	free(curr);
}



int
char_in_memory(struct Creature *victim, struct Creature *rememberer)
{
	memory_rec *names;

	for (names = MEMORY(rememberer); names; names = names->next)
		if (names->id == GET_IDNUM(victim))
			return TRUE;

	return FALSE;
}

//
// devils
//
// return value like damage() but it is impossible to be sure who the victim was
//

int
mob_fight_devil(struct Creature *ch, struct Creature *precious_vict)
{


	int prob = 0;
	Creature *new_mob = NULL;
	Creature *vict = NULL;
	int num = 0;
	int return_flags = 0;




	if (IS_PET(ch)) {			// pets should only fight who they're told to
		vict = ch->findRandomCombat();
	} else {					// find a suitable victim
		vict = choose_opponent(ch, precious_vict);
	}
	// if you have noone to fight, don't fight
	// what a waste.
	if (vict == NULL)
		return 0;

	// prob determines devil's chance of drawing a bead on his victim for blasting
	prob =
		10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
		GET_HITROLL(ch);

	if (prob > (random_percentage() - 25)) {
		WAIT_STATE(ch, 2 RL_SEC);
		call_magic(ch, vict, NULL,
			(IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
			SPELL_HELL_FROST : SPELL_HELL_FIRE,
			GET_LEVEL(ch), CAST_BREATH, &return_flags);
		return return_flags;
	}
	// see if we're fighting more than 1 person, if so, blast the room
    num = ch->numCombatants();
    
	if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
		WAIT_STATE(ch, 3 RL_SEC);
		if (call_magic(ch, NULL, NULL,
				(IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
				SPELL_HELL_FROST_STORM : SPELL_HELL_FIRE_STORM,
				GET_LEVEL(ch), CAST_BREATH, &return_flags)) {
			return return_flags;
		}
	}
	// pets shouldnt port, ever, not even once. not on a train. not on a plane
	// not even for green eggs and spam.
	if (IS_PET(ch)) {
		return 0;
	}
	// 100 move flat rate to gate, removed when the gating actually occurs
	if (GET_MOVE(ch) < 100) {
		return 0;
	}
	// hell hunters only gate IN hell, where they should not be anyway...
	if (hell_hunter == GET_MOB_SPEC(ch) && PRIME_MATERIAL_ROOM(ch->in_room))
		return 0;


	// see how many devils are already in the room
	CreatureList::iterator it = ch->in_room->people.begin();
	for (num = 0; it != ch->in_room->people.end(); ++it)
		if (IS_DEVIL((*it)))
			num++;

	// less chance of gating for psionic devils with mana
	if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
		num += 3;

	// gating results depend on devil char_class
	switch (GET_CLASS(ch)) {
	case CLASS_LESSER:
		if (random_number_zero_low(8) > num) {
			if (ch->mob_specials.shared->number > 1 && random_binary())
				new_mob = read_mobile(GET_MOB_VNUM(ch));
			else if (random_fractional_3())	// nupperibo-spined
				new_mob = read_mobile(16111);
			else {				// Abishais
				if (HELL_PLANE(ch->in_room->zone, 3))
					new_mob = read_mobile(16133);
				else if (HELL_PLANE(ch->in_room->zone, 4) ||
					HELL_PLANE(ch->in_room->zone, 6))
					new_mob = read_mobile(16135);
				else if (HELL_PLANE(ch->in_room->zone, 5))
					new_mob = read_mobile(16132);
				else if (HELL_PLANE(ch->in_room->zone, 1) ||
					HELL_PLANE(ch->in_room->zone, 2))
					new_mob = read_mobile(number(16131, 16135));
			}
		}
		break;
	case CLASS_GREATER:
		if (random_number_zero_low(10) > num) {
			if (ch->mob_specials.shared->number > 1 && random_binary())
				new_mob = read_mobile(GET_MOB_VNUM(ch));
			else if (GET_MOB_VNUM(ch) == 16118 ||	// Pit Fiends
				GET_MOB_VNUM(ch) == 15252) {
				if (random_binary())
					new_mob = read_mobile(16112);	// Barbed
				else
					new_mob = read_mobile(16116);	// horned
			} else if (HELL_PLANE(ch->in_room->zone, 5) || ICY_DEVIL(ch)) {	//  Ice Devil
				if (random_fractional_3())
					new_mob = read_mobile(16115);	// Bone
				else if (random_binary() && GET_LEVEL(ch) > 40)
					new_mob = read_mobile(16146);
				else
					new_mob = read_mobile(16132);	// white abishai
			}
		}
		break;
	case CLASS_DUKE:
	case CLASS_ARCH:
		if (random_number_zero_low(GET_LEVEL(ch)) > 30) {
			act("You feel a wave of sheer terror wash over you as $n approaches!", FALSE, ch, 0, 0, TO_ROOM);
			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && *it != ch; ++it) {
				vict = *it;
				if (vict->findCombat(ch) &&
					GET_LEVEL(vict) < LVL_AMBASSADOR &&
					!mag_savingthrow(vict, GET_LEVEL(ch) << 2, SAVING_SPELL) &&
					!IS_AFFECTED(vict, AFF_CONFIDENCE))
					do_flee(vict, "", 0, 0, 0);
			}

        }else if( GET_MOB_VNUM(ch) == 16142 ){ // Sekolah
            if( random_number_zero_low(12) > num ){
                new_mob = read_mobile( 16165 ); // DEVIL FISH
            }

		} else if (random_number_zero_low(12) > num) {
			if (random_binary())
				new_mob = read_mobile(16118);	// Pit Fiend 
			else
				new_mob = read_mobile(number(16114, 16118));	// barbed bone horned ice pit 

		}
		break;
	}
	if (new_mob) {
		if (IS_PET(ch))
			SET_BIT(MOB_FLAGS(new_mob), MOB_PET);
		WAIT_STATE(ch, 5 RL_SEC);
		GET_MOVE(ch) -= 100;
		char_to_room(new_mob, ch->in_room,false);
		WAIT_STATE(new_mob, 3 RL_SEC);
		act("$n gestures, a glowing portal appears with a whine!",
			FALSE, ch, 0, 0, TO_ROOM);

        if( GET_MOB_VNUM(ch) == 16142 ){ // SEKOLAH
            act("$n swims out from the submerged portal in a jet of bubbles.", 
                FALSE, new_mob, 0, 0, TO_ROOM );
        } else{
		    act("$n steps out of the portal with a crack of lightning!",
			    FALSE, new_mob, 0, 0, TO_ROOM);
        }
        Creature *target = ch->findRandomCombat();
		if (target && IS_MOB(target))
			return (hit(new_mob, target,
					TYPE_UNDEFINED) & DAM_VICT_KILLED);
		return 0;
	}

	return -1;
}

ACMD(do_breathe)
{								// Breath Weapon Attack
    struct affected_type *fire = affected_by_spell( ch, SPELL_FIRE_BREATHING );
    struct affected_type *frost = affected_by_spell( ch, SPELL_FROST_BREATHING );

	if (IS_PC(ch) && fire == NULL && frost == NULL ) {
        act("You breathe heavily.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n seems to be out of breath.", FALSE, ch, 0, 0, TO_ROOM);
        return;
	}
    // Find the victim
	skip_spaces(&argument);
	Creature *vict = get_char_room_vis(ch, argument);
	if (vict == NULL)
		vict = ch->findRandomCombat();
	if (vict == NULL) {
		act("Breathe on whom?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

    if( IS_PC(ch) ) {
        if( fire != NULL ) {
            call_magic(ch, vict, 0, SPELL_FIRE_BREATH, GET_LEVEL(ch), CAST_BREATH);
            fire->duration -= 5;
            if( fire->duration <= 0 )
                affect_remove( ch, fire );
        } else if( frost != NULL ) {
            call_magic(ch, vict, 0, SPELL_FROST_BREATH, GET_LEVEL(ch), CAST_BREATH);
            frost->duration -= 5;
            if( frost->duration <= 0 )
                affect_remove( ch, frost );
        } else {
            send_to_char(ch, "ERROR: No breath type found.\r\n" );
        }
        return;
    }

	switch (GET_CLASS(ch)) {
	case CLASS_GREEN:
		call_magic(ch, vict, 0, SPELL_GAS_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;

	case CLASS_BLACK:
		call_magic(ch, vict, 0, SPELL_ACID_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	case CLASS_BLUE:
		call_magic(ch, vict, 0,
			SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	case CLASS_WHITE:
	case CLASS_SILVER:
		call_magic(ch, vict, 0, SPELL_FROST_BREATH, GET_LEVEL(ch),
			CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	case CLASS_RED:
		call_magic(ch, vict, 0, SPELL_FIRE_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	case CLASS_SHADOW_D:
		call_magic(ch, vict, 0, SPELL_SHADOW_BREATH, GET_LEVEL(ch),
			CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	case CLASS_TURTLE:
		call_magic(ch, vict, 0, SPELL_STEAM_BREATH, GET_LEVEL(ch),
			CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		break;
	default:
		act("You don't seem to have a breath weapon.", FALSE, ch, 0, 0,
			TO_CHAR);
		break;
	}

}


int
mob_fight_celestial(struct Creature *ch, struct Creature *precious_vict)
{

    const int LANTERN_ARCHON = 43000;
    const int HOUND_ARCHON = 43001;
    const int WARDEN_ARCHON = 43002;
    const int SWORD_ARCHON = 43003;
    const int TOME_ARCHON = 43004;
    
	int prob = 0;
	Creature *new_mob = NULL;
	Creature *vict = NULL;
	int num = 0;
	int return_flags = 0;

	if (IS_PET(ch)) {			// pets should only fight who they're told to
		vict = ch->findRandomCombat();
	} else {					// find a suitable victim
		vict = choose_opponent(ch, precious_vict);
	}
	// if you have noone to fight, don't fight
	// what a waste.
	if (vict == NULL)
		return 0;

	// prob determines celestials's chance of drawing a bead on his victim for blasting
	prob =
		10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
		GET_HITROLL(ch);

	if (prob > (random_percentage() - 25)) {
		WAIT_STATE(ch, 2 RL_SEC);
		call_magic(ch, vict, NULL, SPELL_FLAME_STRIKE,
			GET_LEVEL(ch), CAST_BREATH, &return_flags);
		return return_flags;
	}
	// see if we're fighting more than 1 person, if so, blast the room
	num = ch->numCombatants();

	if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
		WAIT_STATE(ch, 3 RL_SEC);
		if (call_magic(ch, NULL, NULL, SPELL_FLAME_STRIKE,
				GET_LEVEL(ch), CAST_BREATH, &return_flags)) {
			return return_flags;
		}
	}
	// pets shouldnt port, ever, not even once. not on a train. not on a plane
	// not even for green eggs and spam.
	if (IS_PET(ch)) {
		return 0;
	}
	// 100 move flat rate to gate, removed when the gating actually occurs
	if (GET_MOVE(ch) < 100) {
		return 0;
	}

	// see how many celestials are already in the room
	CreatureList::iterator it = ch->in_room->people.begin();
	for (num = 0; it != ch->in_room->people.end(); ++it)
		if (GET_RACE(*it) == RACE_CELESTIAL || GET_RACE(*it) == RACE_ARCHON)
			num++;

	// less chance of gating for psionic celeestials with mana
	if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
		num += 3;

	// gating results depend on celestials char_class
	switch (GET_CLASS(ch)) {
	case CLASS_LESSER:
		if (random_number_zero_low(8) > num) {
			if (random_fractional_4()){	// Warden Archon
				new_mob = read_mobile(WARDEN_ARCHON);
            } else if (random_fractional_3()) {  //Hound Archon
                new_mob = read_mobile(HOUND_ARCHON);
			} else {                    // Lantern Archon
                new_mob = read_mobile(LANTERN_ARCHON);
            }
		}
		break;
	case CLASS_GREATER:
		if (random_number_zero_low(10) > num) {
            if(random_fractional_3()){
                if(random_binary()){
                    new_mob = read_mobile(TOME_ARCHON);
                } else {
                    new_mob = read_mobile(SWORD_ARCHON);
                }
                
            } else if( random_binary()){
                new_mob = read_mobile(WARDEN_ARCHON);
            } else {
                new_mob = read_mobile(HOUND_ARCHON);
            }
		}
		break;
    case CLASS_GODLING:
		if (random_number_zero_low(12) > num) {
			if (random_binary())
				new_mob = read_mobile(TOME_ARCHON);	// Tome Archon
			else
				new_mob = read_mobile(SWORD_ARCHON);	// Sword Archon

		}
		break;
	case CLASS_DIETY:
		if (random_number_zero_low(15) > num) {
			if (random_binary())
				new_mob = read_mobile(TOME_ARCHON);	// Tome Archon
			else
				new_mob = read_mobile(SWORD_ARCHON);	// Sword Archon

		}
		break;
	}
	if (new_mob) {
		if (IS_PET(ch))
			SET_BIT(MOB_FLAGS(new_mob), MOB_PET);
		WAIT_STATE(ch, 5 RL_SEC);
		GET_MOVE(ch) -= 100;
		char_to_room(new_mob, ch->in_room,false);
		WAIT_STATE(new_mob, 3 RL_SEC);
		act("$n gestures, a glowing silver portal appears with a hum!",
			FALSE, ch, 0, 0, TO_ROOM);
		act("$n steps out of the portal with a flash of white light!",
			FALSE, new_mob, 0, 0, TO_ROOM);
        Creature *target = ch->findRandomCombat();
		if (target && IS_MOB(target))
			return (hit(new_mob, target,
					TYPE_UNDEFINED) & DAM_VICT_KILLED);
		return 0;
	}

	return -1;
}

/**mob_fight_Guardinal:  /
*  This is the port and combat code for the guardinal mobs that enhabit elysium
*/

int
mob_fight_guardinal(struct Creature *ch, struct Creature *precious_vict)
{

    const int AVORIAL_GUARDINAL = 48160;
    const int HAWCINE_GUARDINAL = 48161;
    const int PANTHRAL_GUARDINAL = 48162;
    const int LEONAL_GUARDINAL = 48163;
    
	int prob = 0;
	Creature *new_mob = NULL;
	Creature *vict = NULL;
	int num = 0;
	int return_flags = 0;

	if (IS_PET(ch)) {			// pets should only fight who they're told to
		vict = ch->findRandomCombat();
	} else {					// find a suitable victim
		vict = choose_opponent(ch, precious_vict);
	}
	// if you have noone to fight, don't fight
	// what a waste.
	if (vict == NULL)
		return 0;

	// prob determines guardinal's chance of drawing a bead on his victim for blasting
	prob =
		10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
		GET_HITROLL(ch);

	if (prob > (random_percentage() - 25)) {
		WAIT_STATE(ch, 2 RL_SEC);
		call_magic(ch, vict, NULL, SPELL_FLAME_STRIKE,
			GET_LEVEL(ch), CAST_BREATH, &return_flags);
		return return_flags;
	}
	// see if we're fighting more than 1 person, if so, blast the room
	num = ch->numCombatants();

	if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
		WAIT_STATE(ch, 3 RL_SEC);
		if (call_magic(ch, NULL, NULL, SPELL_FLAME_STRIKE,
				GET_LEVEL(ch), CAST_BREATH, &return_flags)) {
			return return_flags;
		}
	}
	// pets shouldnt port, ever, not even once. not on a train. not on a plane
	// not even for green eggs and spam.
	if (IS_PET(ch)) {
		return 0;
	}
	// 100 move flat rate to gate, removed when the gating actually occurs
	if (GET_MOVE(ch) < 100) {
		return 0;
	}

	// see how many guardinal are already in the room
	CreatureList::iterator it = ch->in_room->people.begin();
	for (num = 0; it != ch->in_room->people.end(); ++it)
		if (GET_RACE(*it) == RACE_GUARDINAL)
			num++;

	// less chance of gating for psionic celeestials with mana
	if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
		num += 3;

	// gating results depend on guardinal char_class
	switch (GET_CLASS(ch)) {
	case CLASS_LESSER:
		if (random_number_zero_low(8) > num) {
			if (random_binary()){	
				new_mob = read_mobile(AVORIAL_GUARDINAL);
            } else if (!random_fractional_5()) { 
                new_mob = read_mobile(HAWCINE_GUARDINAL);
			} else {                  
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            }
		}
		break;
	case CLASS_GREATER:
		if (random_number_zero_low(10) > num) {
            if(!random_fractional_3()){
                if(random_binary()){
                    new_mob = read_mobile(AVORIAL_GUARDINAL);
                } else {
                    new_mob = read_mobile(HAWCINE_GUARDINAL);
                }
                
            } else if( !random_fractional_3() || ch->in_room->zone->number == 481){
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            } else {
                new_mob = read_mobile(LEONAL_GUARDINAL);
            }
		}
		break;
    case CLASS_GODLING:
		if (random_number_zero_low(12) > num) {
			if (random_binary() || ch->in_room->zone->number == 481 )
				new_mob = read_mobile(PANTHRAL_GUARDINAL);
			else
				new_mob = read_mobile(LEONAL_GUARDINAL);

		}
		break;
	case CLASS_DIETY:
		if (random_number_zero_low(15) > num) {
			if (random_binary() || ch->in_room->zone->number == 481)
				new_mob = read_mobile(PANTHRAL_GUARDINAL);
			else
				new_mob = read_mobile(LEONAL_GUARDINAL);

		}
		break;
	}
	if (new_mob) {
		if (IS_PET(ch))
			SET_BIT(MOB_FLAGS(new_mob), MOB_PET);
		WAIT_STATE(ch, 5 RL_SEC);
		GET_MOVE(ch) -= 100;
		char_to_room(new_mob, ch->in_room,false);
		WAIT_STATE(new_mob, 3 RL_SEC);
		act("$n gestures, a glowing golden portal appears with a hum!",
			FALSE, ch, 0, 0, TO_ROOM);
		act("$n steps out of the portal with a flash of blue light!",
			FALSE, new_mob, 0, 0, TO_ROOM);
        Creature *target = ch->findRandomCombat();
		if (target && IS_MOB(target))
			return (hit(new_mob, target,
					TYPE_UNDEFINED) & DAM_VICT_KILLED);
		return 0;
	}

	return -1;
}



/**mob_fight_demon:  /
*  This is the port and combat code for the demonic mobs that enhabit the abyss
*/

int
mob_fight_demon(struct Creature *ch, struct Creature *precious_vict)
{

    //Uncoment when world updated
    const int DRETCH_DEMON = 28200;
    const int BABAU_DEMON = 28201;
    const int VROCK_DEMON = 28202;
    const int HEZROU_DEMON = 28203;
    const int GLABREZU_DEMON = 28204;
    const int ARMANITE_DEMON = 28205;
    const int KNECHT_DEMON = 28206;
    const int SUCCUBUS_DEMON = 28207;
    const int GORISTRO_DEMON = 28208;
    const int BALOR_DEMON  = 28209;
    const int NALFESHNEE_DEMON = 28210;
    
	int prob = 0;
    //Uncomment when world updated
	Creature *new_mob = NULL;
	Creature *vict = NULL;
	int num = 0;
	int return_flags = 0;

	if (IS_PET(ch)) {			// pets should only fight who they're told to
		vict = ch->findRandomCombat();
	} else {					// find a suitable victim
		vict = choose_opponent(ch, precious_vict);
	}
	// if you have noone to fight, don't fight
	// what a waste.
	if (vict == NULL)
		return 0;

	// prob determines demon chance of drawing a bead on his victim for blasting
	prob =
		10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
		GET_HITROLL(ch);

	if (prob > (random_percentage() - 25)) {
		WAIT_STATE(ch, 2 RL_SEC);
		call_magic(ch, vict, NULL, SPELL_FLAME_STRIKE,
			GET_LEVEL(ch), CAST_BREATH, &return_flags);
		return return_flags;
	}
	// see if we're fighting more than 1 person, if so, blast the room
    num = ch->numCombatants();

	if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
		WAIT_STATE(ch, 3 RL_SEC);
		if (call_magic(ch, NULL, NULL, SPELL_HELL_FIRE,
				GET_LEVEL(ch), CAST_BREATH, &return_flags)) {
			return return_flags;
		}
	}
	// pets shouldnt port, ever, not even once. not on a train. not on a plane
	// not even for green eggs and spam.
	if (IS_PET(ch)) {
		return 0;
	}
	// Mobs with specials set shouldn't port either
	if (GET_MOB_SPEC(ch)) {
		return 0;
	}
	// 100 move flat rate to gate, removed when the gating actually occurs
	if (GET_MOVE(ch) < 100) {
		return 0;
	}

	// see how many demon are already in the room
	CreatureList::iterator it = ch->in_room->people.begin();
	for (num = 0; it != ch->in_room->people.end(); ++it)
		if (GET_RACE(*it) == RACE_DEMON)
			num++;

	// less chance of gating for psionic celeestials with mana
	if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
		num += 3;
	// gating results depend on demon char_class
    
	switch (GET_CLASS(ch)) {
	case CLASS_DEMON_II:
		if (random_number_zero_low(8) > num) {
			if (!random_fractional_3()){	
                if( random_binary() ){
                    new_mob = read_mobile(DRETCH_DEMON);
                }else{
				    new_mob = read_mobile(BABAU_DEMON);
                }
            } else if ( random_binary() ){
                new_mob = read_mobile(VROCK_DEMON);
			} else {                  
                new_mob = read_mobile(HEZROU_DEMON);
            }
		}
		break;
	case CLASS_DEMON_III:
		if (random_number_zero_low(10) > num) {
			if (random_fractional_3()){	
                if( random_binary() ){
                    new_mob = read_mobile(DRETCH_DEMON);
                }else{
				    new_mob = read_mobile(BABAU_DEMON);
                }
            } else if ( !random_fractional_3() ){
                if( random_binary() ){
                    new_mob = read_mobile(HEZROU_DEMON);
                }else{
                    new_mob = read_mobile(VROCK_DEMON);
                }
            } else if ( random_binary() ){
                new_mob = read_mobile(GLABREZU_DEMON);
			} else {                  
                new_mob = read_mobile(ARMANITE_DEMON);
            }
		}
		break;
	case CLASS_DEMON_IV:
		if (random_number_zero_low(12) > num) {
            if (random_fractional_3() ){
                if( random_binary() ){
                    new_mob = read_mobile(HEZROU_DEMON);
                }else{
                    new_mob = read_mobile(VROCK_DEMON);
                }
            } else if ( random_binary()){
                if( random_binary() ){
                    new_mob = read_mobile(GLABREZU_DEMON);
                }else{
                    new_mob = read_mobile(ARMANITE_DEMON);
                }
            } else if ( random_fractional_3() ){
				new_mob = read_mobile( KNECHT_DEMON );
			} else if ( random_binary() ) {                  
				new_mob = read_mobile( SUCCUBUS_DEMON );
            } else {
				new_mob = read_mobile( NALFESHNEE_DEMON );
            }
		}
		break;
	case CLASS_DEMON_V:
		if (random_number_zero_low(13) > num) {
            if ( random_binary()){
                if( random_binary() ){
                    new_mob = read_mobile(GLABREZU_DEMON);
                }else{
                    new_mob = read_mobile(ARMANITE_DEMON);
                }
            } else if ( !random_fractional_3() ){
                if( random_fractional_3() ){
                    new_mob = read_mobile(KNECHT_DEMON);
                }else if( random_binary() ){
                    new_mob = read_mobile(SUCCUBUS_DEMON);
                }else{
                    new_mob = read_mobile( NALFESHNEE_DEMON );
                }
			} else if ( random_binary() ){                  
				new_mob = read_mobile( GORISTRO_DEMON );
            } else {
				new_mob = read_mobile( BALOR_DEMON );
            }
		}
		break;
    case CLASS_DEMON_LORD:
	case CLASS_SLAAD_LORD:
        if( GET_MOB_VNUM(ch) == 42819 ){ // Pigeon God
            if( random_number_zero_low(14) > num ){
                if( random_binary() ){
                    new_mob = read_mobile( 42875 ); // grey pigeion
                } else {
                    new_mob = read_mobile( 42888 ); // roosting pigeon
                }
            }
		} else if (random_number_zero_low(14) > num) {
            if ( !random_fractional_3() ){
                if( random_fractional_3() ){
                    new_mob = read_mobile(KNECHT_DEMON);
                }else if( random_binary() ){
                    new_mob = read_mobile(SUCCUBUS_DEMON);
                }else{
                    new_mob = read_mobile( NALFESHNEE_DEMON );
                }
			} else if ( random_binary() ){                  
				new_mob = read_mobile( GORISTRO_DEMON );
            } else {
				new_mob = read_mobile( BALOR_DEMON );
            }
		}
		break;
	case CLASS_DEMON_PRINCE:
		if (random_number_zero_low(15) > num) {
			if ( random_binary() ){                  
				new_mob = read_mobile( GORISTRO_DEMON );
            } else {
				new_mob = read_mobile( BALOR_DEMON );
            }
		}
		break;
	} 
	if (new_mob) {
		if (IS_PET(ch))
			SET_BIT(MOB_FLAGS(new_mob), MOB_PET);
		WAIT_STATE(ch, 5 RL_SEC);
		GET_MOVE(ch) -= 100;
		char_to_room(new_mob, ch->in_room,false);
		WAIT_STATE(new_mob, 3 RL_SEC);
		act("$n gestures, a glowing yellow portal appears with a hum!",
			FALSE, ch, 0, 0, TO_ROOM);

        if( GET_MOB_VNUM(ch) == 42819 ){ // Pigeon god!
		    act("$n flys out of the portal with a clap of thunder!",
			    FALSE, new_mob, 0, 0, TO_ROOM);
        }else{
		    act("$n steps out of the portal with a clap of thunder!",
			    FALSE, new_mob, 0, 0, TO_ROOM);
        }
        Creature *target = ch->findRandomCombat();
		if (target && IS_MOB(target))
			return (hit(new_mob, target,
					TYPE_UNDEFINED) & DAM_VICT_KILLED);
		return 0;
	}
	return -1;
}



/***********************************************************************************
 *
 *                        Knight Activity
 *   First in a series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/

void knight_activity(struct Creature *ch){
    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (GET_LEVEL(ch) > 27 && random_binary() && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
            cast_spell(ch, ch, 0, SPELL_HEAL);
        else if (GET_LEVEL(ch) > 13 && random_binary() && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
            cast_spell(ch, ch, 0, SPELL_CURE_CRITIC);
        else if (random_binary() && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
            cast_spell(ch, ch, 0, SPELL_CURE_LIGHT);
        else if (IS_GOOD(ch)){
            do_holytouch(ch, "self", 0, 0, 0);
        }
    } else if (room_is_dark(ch->in_room) &&
               !has_dark_sight(ch) && GET_LEVEL(ch) > 6 &&
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        cast_spell(ch, ch, 0, SPELL_DIVINE_ILLUMINATION);
    } else if ( !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
                GET_LEVEL(ch) > 20 &&
                (affected_by_spell(ch, SPELL_BLINDNESS) ||
                affected_by_spell(ch, SKILL_GOUGE)) ) {
        cast_spell(ch, ch, 0, SPELL_CURE_BLIND);
    } else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 18 &&
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)){
        cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
    } else if (IS_AFFECTED(ch, AFF_CURSE) && GET_LEVEL(ch) > 30 &&
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        cast_spell(ch, ch, 0, SPELL_REMOVE_CURSE);
    } else if (IS_GOOD(ch) && GET_LEVEL(ch) > 32 &&
               GET_REMORT_CLASS(ch) != CLASS_UNDEFINED &&
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
               !affected_by_spell(ch, SPELL_SANCTIFICATION)) {
        cast_spell(ch, ch, 0, SPELL_SANCTIFICATION);
    } else if( GET_LEVEL(ch) > 4 && 
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
               !affected_by_spell(ch, SPELL_ARMOR)){
        cast_spell(ch, ch, 0, SPELL_ARMOR);
    } else if( IS_GOOD(ch) && GET_LEVEL(ch) > 9 && 
               !affected_by_spell(ch, SPELL_BLESS) && 
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)){
        cast_spell(ch, ch, 0, SPELL_BLESS);
    } else if( GET_LEVEL(ch) > 30 &&
               !affected_by_spell(ch, SPELL_PRAY) && 
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)){
        cast_spell(ch, ch, 0, SPELL_PRAY);
    }
}

/***********************************************************************************
 *
 *                        Knight Battle Activity
 *   member of  a series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/

int knight_battle_activity(struct Creature *ch, struct Creature *precious_vict){
    ACCMD(do_disarm);
    struct Creature * vict;
	int return_flags = 0;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return 0;

    if (can_see_creature(ch, vict) &&
        (IS_MAGE(vict) || IS_CLERIC(vict))
        && vict->getPosition() > POS_SITTING) {
		perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
		return return_flags;
    }

    if (GET_LEVEL(ch) > 4 && random_fractional_5() &&
        !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
        !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, NULL, SPELL_ARMOR);
        return 0;
    } else if ((GET_HIT(ch) / MAX(1,
                GET_MAX_HIT(ch))) < (GET_MAX_HIT(ch) >> 2)) {
        if ((GET_LEVEL(ch) < 14) && (number(0, 10) == 0) &&
            !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
            cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
            return 0;
        } else if ((GET_LEVEL(ch) < 28) && random_fractional_10() &&
                    !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
            return 0;
        } else if (random_fractional_5() &&
                   !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
            cast_spell(ch, ch, NULL, SPELL_HEAL);
            return 0;
        }
    } else if (IS_GOOD(ch) && IS_EVIL(ch->findRandomCombat()) &&
        random_fractional_3()  &&
        !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
        !affected_by_spell(ch, SPELL_PROT_FROM_EVIL)) {
        cast_spell(ch, ch, NULL, SPELL_PROT_FROM_EVIL);
        return 0;
    } else if (IS_EVIL(ch) && IS_GOOD(ch->findRandomCombat()) &&
        random_fractional_3() &&
        !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
        && !affected_by_spell(ch, SPELL_PROT_FROM_GOOD)) {
        cast_spell(ch, ch, NULL, SPELL_PROT_FROM_GOOD);
        return 0;
    } else if ((GET_LEVEL(ch) > 21) &&
        GET_RACE(vict) == RACE_UNDEAD &&
        random_fractional_10() &&
        !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
        !affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
        cast_spell(ch, ch, NULL, SPELL_INVIS_TO_UNDEAD);
        return 0;
    } else if ((GET_LEVEL(ch) > 15) && random_fractional_5()
               && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        cast_spell(ch, vict, NULL, SPELL_SPIRIT_HAMMER);
        return 0;
    } else if ((GET_LEVEL(ch) > 35) && random_fractional_4()
               && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
        return 0;
    } else if( IS_EVIL(ch) && !IS_EVIL(vict) && GET_LEVEL(ch) > 9 && 
               !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) &&
               random_fractional_4() &&
               !affected_by_spell(vict, SPELL_DAMN) ){
        cast_spell(ch, vict, NULL, SPELL_DAMN);
    } else if ((GET_LEVEL(ch) >= 20) &&
        GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
        random_fractional_3()) {
        do_disarm(ch, PERS(vict, ch), 0, 0, 0);
        return 0;
    }

    if (random_fractional_4()) {

        if (GET_LEVEL(ch) < 7) {
			perform_offensive_skill(ch, vict, SKILL_SPINFIST, &return_flags);
        } else if (GET_LEVEL(ch) < 16) {
			perform_offensive_skill(ch, vict, SKILL_UPPERCUT, &return_flags);
        } else if (GET_LEVEL(ch) < 23) {
			perform_offensive_skill(ch, vict, SKILL_HOOK, &return_flags);
        } else if (GET_LEVEL(ch) < 35) {
			perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH, &return_flags);
        } else if (GET_EQ(ch, WEAR_WIELD) &&
            GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) == 3 && 
            random_binary()) {
			perform_offensive_skill(ch, vict, SKILL_BEHEAD, &return_flags);
        } else if(IS_EVIL(ch) && IS_GOOD(vict)){
            do_holytouch(ch, PERS(vict, ch), 0, 0, 0);
        }
        return return_flags;
    }
    return 1;
}


/***********************************************************************************
 *
 *                        Ranger Activity
 *   Continuing series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/

void ranger_activity(struct Creature *ch){

    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (GET_LEVEL(ch) > 39)
            do_medic(ch, "self", 0, 0, 0);
        else if (GET_LEVEL(ch) > 18)
            do_firstaid(ch, "self", 0, 0, 0);
        else if (GET_LEVEL(ch) > 9)
            do_bandage(ch, "self", 0, 0, 0);
    } else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 11) {
        cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
    } else if (!affected_by_spell(ch, SPELL_STONESKIN) && GET_LEVEL(ch) > 19 &&
                GET_REMORT_CLASS(ch) != CLASS_RANGER &&
                GET_REMORT_CLASS(ch) != CLASS_UNDEFINED){
                cast_spell(ch,ch,0,SPELL_STONESKIN);
    } else if (!affected_by_spell(ch, SPELL_BARKSKIN) &&
               !affected_by_spell(ch, SPELL_STONESKIN) ){
                cast_spell(ch, ch, 0, SPELL_BARKSKIN);
    } 
}


/***********************************************************************************
 *
 *                        Ranger battle  Activity
 *   Continuing series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/


int ranger_battle_activity(struct Creature *ch, struct Creature *precious_vict){
    ACCMD(do_disarm);

    struct Creature * vict;
	int return_flags;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return 0;


    if ((GET_LEVEL(ch) > 4) && random_fractional_5() &&
        !affected_by_spell(ch, SPELL_BARKSKIN) &&
        !affected_by_spell(ch, SPELL_STONESKIN)) {
        cast_spell(ch, ch, NULL, SPELL_BARKSKIN);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return 0;
    } else if ((GET_LEVEL(ch) > 21) && random_fractional_5() &&
        GET_RACE(vict) == RACE_UNDEAD &&
		!ch->numCombatants() &&
        !affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
        cast_spell(ch, ch, NULL, SPELL_INVIS_TO_UNDEAD);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return 0;
    }

    if (GET_LEVEL(ch) >= 27 &&
        (GET_CLASS(vict) == CLASS_MAGIC_USER ||
            GET_CLASS(vict) == CLASS_CLERIC) && random_fractional_3()) {
		perform_offensive_skill(ch, vict, SKILL_SWEEPKICK, &return_flags);
        return return_flags;
    }
    if ((GET_LEVEL(ch) > 42) && random_fractional_5()) {
		perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH, &return_flags);
        return return_flags;
    } else if ((GET_LEVEL(ch) > 25) && random_fractional_5()) {
		perform_offensive_skill(ch, vict, SKILL_BEARHUG, &return_flags);
        return return_flags;
    } else if ((GET_LEVEL(ch) >= 20) &&
        GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
        random_fractional_5()) {
        do_disarm(ch, PERS(vict, ch), 0, 0, 0);
        return 0;
    }

    if (random_fractional_4()) {

        if (GET_LEVEL(ch) < 3) {
            perform_offensive_skill(ch, vict, SKILL_KICK, &return_flags);
        } else if (GET_LEVEL(ch) < 6) {
            perform_offensive_skill(ch, vict, SKILL_BITE, &return_flags);
        } else if (GET_LEVEL(ch) < 14) {
            perform_offensive_skill(ch, vict, SKILL_UPPERCUT, &return_flags);
        } else if (GET_LEVEL(ch) < 17) {
            perform_offensive_skill(ch, vict, SKILL_HEADBUTT, &return_flags);
        } else if (GET_LEVEL(ch) < 22) {
            perform_offensive_skill(ch, vict, SKILL_ROUNDHOUSE, &return_flags);
        } else if (GET_LEVEL(ch) < 24) {
            perform_offensive_skill(ch, vict, SKILL_HOOK, &return_flags);
        } else if (GET_LEVEL(ch) < 36) {
            perform_offensive_skill(ch, vict, SKILL_SIDEKICK, &return_flags);
        } else {
            perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH, &return_flags);
        }
        return return_flags;
    }
    return 1;
}


/***********************************************************************************
 *
 *                        Barbarian Activity
 *   Continuing series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/

void barbarian_activity(struct Creature *ch){
    if (AFF2_FLAGGED(ch, AFF2_BERSERK)){
        do_berserk( ch, "", 0, 0, 0 );
        return;
    }
    if (ch->getPosition() != POS_FIGHTING && random_fractional_20()) {
        if (random_fractional_50())
            act("$n grunts and scratches $s ear.", FALSE, ch, 0, 0,
                TO_ROOM);
        else if (random_fractional_50())
            act("$n drools all over $mself.", FALSE, ch, 0, 0,
                TO_ROOM);
        else if (random_fractional_50())
            act("$n belches loudly.", FALSE, ch, 0, 0, TO_ROOM);
        else if (random_fractional_50())
            act("$n swats at an annoying gnat.", FALSE, ch, 0, 0,
                TO_ROOM);
        else if (random_fractional_100()) {
            if (GET_SEX(ch) == SEX_MALE)
                act("$n scratches $s nuts and grins.", FALSE, ch, 0, 0,
                    TO_ROOM);
            else if (GET_SEX(ch) == SEX_FEMALE)
                act("$n belches loudly and grins.", FALSE, ch, 0, 0,
                    TO_ROOM);
        }
    } else if (IS_BARB(ch) && GET_LEVEL(ch) >= 42 &&
        GET_HIT(ch) < (GET_MAX_HIT(ch) >> 1) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, "", 0, SCMD_CRY_FROM_BEYOND, 0);
        return;
    } else if (IS_BARB(ch) && GET_LEVEL(ch) >= 30 &&
        GET_MOVE(ch) < (GET_MAX_MOVE(ch) >> 2) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, "", 0, SCMD_BATTLE_CRY, 0);
        return;
    }
}

/***********************************************************************************
 *
 *                        Barbarian battle  Activity
 *   Continuing series of functions designed to extract activity and fight code 
 * into many small functions as opposed to one large one.  Also allows for easeir 
 * modifications to mob ai.
 *******************************************************************************/



int barbarian_battle_activity(struct Creature *ch, struct Creature *precious_vict){
    ACCMD(do_disarm);

    struct Creature * vict;
	int return_flags = 0;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return 0;

    if (IS_BARB(ch) && GET_LEVEL(ch) >= 42 &&
        GET_HIT(ch) < (GET_MAX_HIT(ch) >> 1) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, "", 0, SCMD_CRY_FROM_BEYOND, 0);
        return 0;
    }

    if (random_fractional_3() && (GET_CLASS(vict) == CLASS_MAGIC_USER ||
            GET_CLASS(vict) == CLASS_CLERIC)) {
        perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
        return return_flags;
    }
    if ((GET_LEVEL(ch) > 27) && random_fractional_5()
        && vict->getPosition() > POS_RESTING) {
        perform_offensive_skill(ch, vict, SKILL_BASH, &return_flags);
        return return_flags;
    }

		//
		// barbs go BERSERK (berserk)
		//

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && !AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        do_berserk(ch, "", 0, 0, 0);

		if (!perform_barb_berserk(ch, 0, &return_flags)){
            return return_flags;
        }
	}
    if (random_fractional_4()) {
        if( GET_LEVEL(ch) >= 30 && GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD)) ){
            perform_cleave(ch, vict, &return_flags);
        } else if (GET_LEVEL(ch) >= 17) {
            perform_offensive_skill(ch, vict, SKILL_STRIKE, &return_flags);
        } else if (GET_LEVEL(ch) >= 13) {
            perform_offensive_skill(ch, vict, SKILL_HEADBUTT, &return_flags);
        } else if (GET_LEVEL(ch) >= 4 ) {
            perform_offensive_skill(ch, vict, SKILL_STOMP, &return_flags);
        } else{
            perform_offensive_skill(ch, vict, SKILL_KICK, 0 );
        }
        return return_flags;
    }
    return -1;
}
