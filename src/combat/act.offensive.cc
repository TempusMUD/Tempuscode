 /* ************************************************************************
    *   File: act.offensive.c                               Part of CircleMUD *
    *  Usage: player-level commands of an offensive nature                    *
    *                                                                         *
    *  All rights reserved.  See license.doc for complete information.        *
    *                                                                         *
    *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
    *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
    ************************************************************************ */

//
// File: act.offensive.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "guns.h"
#include "bomb.h"
#include "shop.h"
#include "events.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct obj_data *cur_weap;

struct follow_type *order_next_k;

ACMD(do_say);
/* extern functions */
int find_door(struct Creature *ch, char *type, char *dir,
	const char *cmdname);
int do_combat_fire(struct Creature *ch, struct Creature *vict, int weap_pos,
	int prob);
void send_to_queue(MobileEvent * e);


int
check_mob_reaction(struct Creature *ch, struct Creature *vict)
{
	int num = 0;

	if (!IS_MOB(vict))
		return 0;

	num = (GET_ALIGNMENT(vict) - GET_ALIGNMENT(ch)) / 20;
	num = MAX(num, (-num));
	num -= GET_CHA(ch);

	if (num > number(0, 101)) {
		act("$n gets pissed off!", FALSE, vict, 0, 0, TO_ROOM);
		return 1;
	} else
		return 0;
}


// #define RAW_EQ_DAM(ch, pos, var)

inline int
RAW_EQ_DAM(struct Creature *ch, int pos, int *var)
{

	if (ch->equipment[pos]) {
		if (IS_OBJ_TYPE(ch->equipment[pos], ITEM_ARMOR))
			*var += (ch->equipment[pos]->getWeight() +
				GET_OBJ_VAL(ch->equipment[pos], 0)) >> 2;
		else if (IS_OBJ_TYPE(ch->equipment[pos], ITEM_WEAPON))
			*var += dice(GET_OBJ_VAL(ch->equipment[pos], 1),
				GET_OBJ_VAL(ch->equipment[pos], 2));
	} else if (ch->implants[pos]) {
		if (IS_OBJ_TYPE(ch->implants[pos], ITEM_ARMOR))
			*var += (ch->implants[pos]->getWeight() +
				GET_OBJ_VAL(ch->implants[pos], 0)) >> 2;
		else if (IS_OBJ_TYPE(ch->implants[pos], ITEM_WEAPON))
			*var += dice(GET_OBJ_VAL(ch->implants[pos], 1),
				GET_OBJ_VAL(ch->implants[pos], 2));
	}

	return (*var);
}

#define ADD_EQ_DAM(ch, pos)     RAW_EQ_DAM(ch, pos, dam)

#define NOBEHEAD_EQ(obj) \
IS_SET(obj->obj_flags.bitvector[1], AFF2_NECK_PROTECTED)

int
calc_skill_prob(struct Creature *ch, struct Creature *vict, int skillnum,
	int *wait, int *vict_wait, int *move, int *mana, int *dam,
	int *fail_pos, int *vict_pos, int *loc,
	struct affected_type *af, int *return_flags)
{

	int prob = 0, eq_wt = 0, i;
	bool bad_sect = 0, need_hand = 0;
	struct obj_data *neck = NULL, *ovict = NULL;
	struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

	prob = CHECK_SKILL(ch, skillnum);
	if (CHECK_SKILL(ch, skillnum) < LEARNED(ch))
		prob -= (LEARNED(ch) - CHECK_SKILL(ch, skillnum));
	prob += GET_DEX(ch);
	prob += (str_app[STRENGTH_APPLY_INDEX(ch)].tohit + GET_HITROLL(ch)) >> 1;

	prob -= ((IS_WEARING_W(ch) + IS_CARRYING_W(ch)) * 15) / CAN_CARRY_W(ch);

	prob += (MAX(-200, GET_AC(vict)) / 10);

	if (vict->getPosition() < POS_FIGHTING) {
		prob += (POS_FIGHTING - vict->getPosition()) * 6;
		prob -= (GET_DEX(vict) >> 1);
	}

	if (IS_DROW(ch)) {
		if (OUTSIDE(ch) && PRIME_MATERIAL_ROOM(ch->in_room)) {
			if (ch->in_room->zone->weather->sunlight == SUN_LIGHT)
				prob -= 25;
			else if (ch->in_room->zone->weather->sunlight == SUN_DARK)
				prob += 10;
		} else if (IS_DARK(ch->in_room))
			prob += 10;
	}

	if (IS_AFFECTED_2(ch, AFF2_DISPLACEMENT) &&
		!IS_AFFECTED_2(vict, AFF2_TRUE_SEEING))
		prob += 5;
	if (IS_AFFECTED(ch, AFF_BLUR))
		prob += 5;
	if (!CAN_SEE(vict, ch))
		prob += 20;

	if (!CAN_SEE(ch, vict))
		prob -= 20;
	if (GET_COND(ch, DRUNK))
		prob -= (GET_COND(ch, DRUNK) * 2);
	if (IS_SICK(ch))
		prob -= 5;
	if (IS_AFFECTED_2(vict, AFF2_DISPLACEMENT) &&
		!IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))
		prob -= GET_LEVEL(vict) >> 1;
	if (IS_AFFECTED_2(vict, AFF2_EVADE))
		prob -= (GET_LEVEL(vict) >> 2) + 5;

	if (IS_MONK(ch)) {
		if (GET_EQ(ch, WEAR_WIELD))
			prob +=
				(LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) >> 2;

		for (i = 0; i < NUM_WEARS; i++)
			if ((ovict = GET_EQ(ch, i)) && GET_OBJ_TYPE(ovict) == ITEM_ARMOR &&
				(IS_METAL_TYPE(ovict) || IS_STONE_TYPE(ovict) ||
					IS_WOOD_TYPE(ovict)))
				eq_wt += ovict->getWeight();
	}

	if (ch->in_room->sector_type == SECT_UNDERWATER ||
		ch->in_room->sector_type == SECT_WATER_NOSWIM ||
		ch->in_room->sector_type == SECT_WATER_SWIM ||
		ch->in_room->sector_type == SECT_ASTRAL || ch->in_room->isOpenAir())
		bad_sect = TRUE;

	if (bad_sect)
		prob -= (prob * 20) / 100;

	/* individual skill parameters */
	switch (skillnum) {

	case SKILL_BASH:
		prob -= (str_app[STRENGTH_APPLY_INDEX(vict)].wield_w >> 1);
		prob -= (GET_WEIGHT(vict) - GET_WEIGHT(ch)) >> 4;

		if (ch->equipment[WEAR_WIELD])
			prob += ch->equipment[WEAR_WIELD]->getWeight();
		if (ch->equipment[WEAR_SHIELD])
			prob += ch->equipment[WEAR_SHIELD]->getWeight();


		if (bad_sect)
			prob = (int)(prob * 0.90);

		if ((MOB_FLAGGED(vict, MOB_NOBASH) ||
				vict->getPosition() < POS_FIGHTING)
			&& GET_LEVEL(ch) < LVL_AMBASSADOR)
			prob = 0;

		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*dam = dice(2, (GET_LEVEL(ch) >> 2));
		*fail_pos = POS_SITTING;
		*vict_pos = POS_SITTING;
		*vict_wait = 2 RL_SEC;
		*wait = 9 RL_SEC;
		if (IS_BARB(ch)) {
			// reduced wait state for barbs
			int w = 5;
			w *= ch->getLevelBonus(SKILL_BASH);
			w /= 100;
			*wait -= w;
			// Improved damage for barbs
			*dam += dice(2, ch->getLevelBonus(SKILL_BASH));
		}

		break;

	case SKILL_HEADBUTT:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*dam = dice(3, (GET_LEVEL(ch) >> 3));
		ADD_EQ_DAM(ch, WEAR_HEAD);

		*vict_wait = 1 RL_SEC;
		*wait = 4 RL_SEC;
		*loc = WEAR_HEAD;
		// Barb headbutt should be nastier
		if (IS_BARB(ch) && 0) {
			// Improved damage for barbs
			*dam += ch->getLevelBonus(SKILL_HEADBUTT);
			// Knock them down?
			int attack = GET_STR(ch) + GET_DEX(ch);
			int defence = GET_CON(vict) + GET_DEX(vict);
			int margin = attack - defence;
			if (number(1, 20) < margin)
				*vict_pos = POS_SITTING;
		}
		break;

	case SKILL_GOUGE:
		need_hand = 1;

		if (vict->equipment[WEAR_EYES] &&
			IS_OBJ_TYPE(vict->equipment[WEAR_EYES], ITEM_ARMOR))
			prob -= GET_OBJ_VAL(GET_EQ(vict, WEAR_EYES), 0);

		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*dam = dice(6, (GET_LEVEL(ch) >> 4));
		*wait = 6 RL_SEC;
		*vict_wait = 1 RL_SEC;
		*loc = WEAR_EYES;

		if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOBLIND)) {
			*dam = 0;
			prob = 0;
		} else {
			af->type = SKILL_GOUGE;
			af->duration = 1 + (GET_LEVEL(ch) > 40);
			af->modifier = -10;
			af->location = APPLY_HITROLL;
			af->bitvector = AFF_BLIND;
		}
		break;

	case SKILL_PILEDRIVE:
		need_hand = 1;

		if (GET_HEIGHT(vict) > (GET_HEIGHT(ch) << 1))
			prob -= (100 - GET_LEVEL(ch));

		if (vict->getPosition() < POS_STANDING)
			prob += (10 * (POS_STANDING - vict->getPosition()));

		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect)
			prob = 0;

		if ((GET_WEIGHT(vict) + ((IS_CARRYING_W(vict) +
						IS_WEARING_W(vict)) >> 1)) > CAN_CARRY_W(ch) * 1.5) {
			act("$N is too heavy for you to lift!", FALSE, ch, 0, vict,
				TO_CHAR);
			act("$n tries to pick you up and piledrive you!", FALSE, ch, 0,
				vict, TO_VICT);
			act("$n tries to pick $N up and piledrive $M!", FALSE, ch, 0, vict,
				TO_NOTVICT);
			prob = 0;
			if (check_mob_reaction(ch, vict)) {
				int retval = hit(vict, ch, TYPE_UNDEFINED);
				*return_flags = SWAP_DAM_RETVAL(retval);
				return -1;
			}
		}

		*move = 20;
		*dam = dice(10 + GET_LEVEL(ch), GET_STR(ch));
		if (!IS_BARB(ch) && GET_LEVEL(ch) < LVL_IMMORT)
			*dam = *dam >> 2;
		ADD_EQ_DAM(ch, WEAR_CROTCH);
		if (!MOB_FLAGGED(vict, MOB_NOBASH))
			*vict_pos = POS_SITTING;
		*fail_pos = POS_SITTING;
		*loc = WEAR_HEAD;
		*wait = 8 RL_SEC;
		*vict_wait = 3 RL_SEC;
		break;

	case SKILL_BODYSLAM:

		if ((GET_WEIGHT(vict) + ((IS_CARRYING_W(vict) +
						IS_WEARING_W(vict)) >> 1)) > CAN_CARRY_W(ch) * 1.5) {
			act("$N is too heavy for you to lift!", FALSE, ch, 0, vict,
				TO_CHAR);
			act("$n tries to pick you up and bodyslam you!", FALSE, ch, 0,
				vict, TO_VICT);
			act("$n tries to pick $N up and bodyslam $M!", FALSE, ch, 0, vict,
				TO_NOTVICT);
			prob = 0;
			if (check_mob_reaction(ch, vict)) {
				int retval = hit(vict, ch, TYPE_UNDEFINED);
				*return_flags = SWAP_DAM_RETVAL(retval);
			}
			return -1;
		}

		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect ||
			MOB_FLAGGED(vict, MOB_NOBASH))
			prob = 0;

		need_hand = 1;
		*dam = dice(10, GET_LEVEL(ch) >> 3);
		*vict_pos = POS_SITTING;
		*wait = 7 RL_SEC;
		*vict_wait = 2 RL_SEC;
		*move = 15;
		break;

	case SKILL_KICK:

		*dam = dice(2, (GET_LEVEL(ch) >> 2));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 3 RL_SEC;
		break;

	case SKILL_SPINKICK:

		*dam = dice(3, (GET_LEVEL(ch) >> 2));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 4 RL_SEC;
		break;

	case SKILL_ROUNDHOUSE:

		*dam = dice(4, (GET_LEVEL(ch) >> 2));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 5 RL_SEC;
		break;

	case SKILL_PELE_KICK:

		*dam = dice(8, GET_LEVEL(ch));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 6 RL_SEC;
		*fail_pos = POS_SITTING;
		break;

	case SKILL_SIDEKICK:

		*dam = dice(5, (GET_LEVEL(ch) >> 2));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 5 RL_SEC;
		break;

	case SKILL_GROINKICK:

		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*dam = dice(3, (GET_LEVEL(ch) >> 2));
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 5 RL_SEC;
		break;

	case SKILL_PUNCH:

		need_hand = 1;
		prob += GET_DEX(ch);
		if (GET_LEVEL(ch) >= 67) {
			*dam = 1000;
		} else {
			*dam = dice(1, GET_LEVEL(ch) >> 3);
			ADD_EQ_DAM(ch, WEAR_HANDS);
		}
		*wait = 3 RL_SEC;
		break;

	case SKILL_SPINFIST:
	case SKILL_CLAW:

		need_hand = 1;
		*dam = dice(2, GET_LEVEL(ch) >> 3);
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 3 RL_SEC;
		break;

	case SKILL_JAB:
	case SKILL_RABBITPUNCH:

		need_hand = 1;
		*dam = dice(1, GET_LEVEL(ch) >> 3) + 1;
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 3 RL_SEC;
		break;

	case SKILL_HOOK:

		need_hand = 1;
		*dam = dice(2, GET_LEVEL(ch) >> 3) + 1;
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 3 RL_SEC;
		break;

	case SKILL_UPPERCUT:

		need_hand = 1;
		*dam = dice(3, GET_LEVEL(ch) >> 3) + 10;
		if (IS_RANGER(ch))
			*dam <<= 1;
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 4 RL_SEC;
		*loc = WEAR_FACE;
		break;

	case SKILL_LUNGE_PUNCH:

		if (bad_sect)
			prob = (int)(prob * 0.90);

		if ((MOB_FLAGGED(vict, MOB_NOBASH) ||
				vict->getPosition() < POS_FIGHTING)
			&& GET_LEVEL(ch) < LVL_AMBASSADOR)
			prob = 0;

		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		need_hand = 1;
		*dam = dice(8, GET_LEVEL(ch) >> 3) + 1;
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 4 RL_SEC;
		*vict_pos = POS_SITTING;
		*vict_wait = 2 RL_SEC;
		*loc = WEAR_FACE;
		break;

	case SKILL_ELBOW:

		*dam = dice(2, GET_LEVEL(ch) >> 3);
		ADD_EQ_DAM(ch, WEAR_ARMS);
		*wait = 4 RL_SEC;
		break;

	case SKILL_KNEE:

		*dam = dice(3, GET_LEVEL(ch) >> 3);
		ADD_EQ_DAM(ch, WEAR_LEGS);
		*wait = 5 RL_SEC;
		break;

	case SKILL_STOMP:

		*loc = WEAR_FEET;
		*dam = dice(5, GET_LEVEL(ch) >> 2);
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 4 RL_SEC;
		*vict_wait = 1 RL_SEC;
		break;

	case SKILL_CLOTHESLINE:

		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect
			|| NON_CORPOREAL_MOB(vict))
			prob = 0;

		*loc = WEAR_NECK_1;
		*dam = dice(2, GET_LEVEL(ch) >> 3);
		ADD_EQ_DAM(ch, WEAR_ARMS);
		*wait = 4 RL_SEC;
		*vict_pos = POS_SITTING;
		break;

	case SKILL_SWEEPKICK:
	case SKILL_TRIP:

		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect
			|| NON_CORPOREAL_MOB(vict) || MOB_FLAGGED(vict, MOB_NOBASH))
			prob = 0;

		*dam = dice(2, GET_LEVEL(ch) >> 3);
		*wait = 5 RL_SEC;
		*vict_pos = POS_SITTING;
		*vict_wait = 2 RL_SEC;
		break;

	case SKILL_BEARHUG:

		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;
		need_hand = 1;
		*loc = WEAR_BODY;
		*dam = dice(4, GET_LEVEL(ch) >> 3);
		*wait = 6 RL_SEC;
		*vict_wait = 3 RL_SEC;
		break;

	case SKILL_BITE:

		*dam = dice(2, GET_LEVEL(ch) >> 3);
		*wait = 4 RL_SEC;
		break;

	case SKILL_CHOKE:
		if (IS_AFFECTED_2(vict, AFF2_NECK_PROTECTED) &&
			number(0, GET_LEVEL(vict) * 4) > number(0, GET_LEVEL(ch))) {
			if ((neck = GET_EQ(vict, WEAR_NECK_1)) ||
				(neck = GET_EQ(vict, WEAR_NECK_2))) {
				act("$n attempts to choke you, but $p has you covered!",
					FALSE, ch, neck, vict, TO_VICT);
				act("$n attempts to choke $N, but $p has $M covered!",
					FALSE, ch, neck, vict, TO_NOTVICT);
				act("You attempt to choke $N, but $p has $M covered!",
					FALSE, ch, neck, vict, TO_CHAR);
				prob = 0;
				return -1;
			}
		}

		need_hand = 1;
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;
		*loc = WEAR_NECK_1;
		*dam = dice(3, GET_LEVEL(ch) >> 3);
		*wait = 4 RL_SEC;
		*vict_wait = 2 RL_SEC;
		break;

	case SKILL_BEHEAD:

		*loc = WEAR_NECK_1;
		*move = 20;

		prob -= 20;

		if ((!weap || !SLASHING(weap)) &&
			(!(weap = GET_EQ(ch, WEAR_HANDS)) ||
				!IS_OBJ_TYPE(weap, ITEM_WEAPON) || !SLASHING(weap))) {
			send_to_char(ch, 
				"You need to wield a good slashing weapon to do this.\r\n");
			return -1;
		}

		*dam = (dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2)) * 6);
		*dam += dice(GET_LEVEL(ch), 5);
		if (IS_OBJ_STAT2(weap, ITEM2_TWO_HANDED)
			&& weap->worn_on == WEAR_WIELD)
			*dam <<= 1;
		else
			*dam = (int)(*dam * 0.80);

		if (isname("headless", vict->player.name)) {
			act("$N doesn't have a head!", FALSE, ch, 0, vict, TO_CHAR);
			return -1;
		}

		if ((GET_HEIGHT(ch) < GET_HEIGHT(vict) >> 1) &&
			vict->getPosition() > POS_SITTING) {
			if (AFF_FLAGGED(ch, AFF_INFLIGHT))
				*dam >>= 2;
			else {
				act("$N is over twice your height!  You can't reach $S head!",
					FALSE, ch, 0, vict, TO_CHAR);
				prob = 0;
				return -1;
			}
		}

		if (IS_AFFECTED_2(vict, AFF2_NECK_PROTECTED) &&
			number(0, GET_LEVEL(vict) * 4) > number(0, (GET_LEVEL(ch) >> 1))) {
			if (((neck = GET_EQ(vict, WEAR_NECK_1)) &&
					NOBEHEAD_EQ(neck)) ||
				((neck = GET_EQ(vict, WEAR_NECK_2)) && NOBEHEAD_EQ(neck))) {
				act("$n swings at your neck, but the blow is deflected by $p!",
					FALSE, ch, neck, vict, TO_VICT);
				act("$n swings at $N's neck, but the blow is deflected by $p!",
					FALSE, ch, neck, vict, TO_NOTVICT);
				act("You swing at $N's neck, but the blow is deflected by $p!",
					FALSE, ch, neck, vict, TO_CHAR);
				check_toughguy(ch, vict, 0);
				check_killer(ch, vict);
				damage_eq(ch, neck, *dam);
				WAIT_STATE(ch, 2 RL_SEC);
				prob = 0;
				return -1;
			}
			if (((neck = GET_IMPLANT(vict, WEAR_NECK_1)) &&
					NOBEHEAD_EQ(neck)) ||
				((neck = GET_IMPLANT(vict, WEAR_NECK_2)) &&
					NOBEHEAD_EQ(neck))) {
				damage_eq(ch, neck, *dam);
				*dam >>= 1;
			}
		}

		if (IS_PUDDING(vict) || IS_SLIME(vict) || IS_RACE(vict, RACE_BEHOLDER))
			prob = 0;

		*wait = 6 RL_SEC;
		cur_weap = weap;
		break;

	case SKILL_SHOOT:
		prob += (GET_DEX(ch) >> 1);
		prob -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;

		break;

	case SKILL_ARCHERY:
		prob += (GET_DEX(ch) >> 1);
		prob -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;

		break;

		/** monk skillz here **/
	case SKILL_PALM_STRIKE:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		need_hand = 1;
		*loc = WEAR_BODY;
		*dam = dice(GET_LEVEL(ch), GET_STR(ch) >> 2);
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 3 RL_SEC;
		*vict_wait = 1 RL_SEC;
		break;

	case SKILL_THROAT_STRIKE:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		need_hand = 1;
		*loc = WEAR_NECK_1;
		*dam = dice(GET_LEVEL(ch), 6);
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 4 RL_SEC;
		*vict_wait = 2 RL_SEC;
		break;

	case SKILL_CRANE_KICK:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*loc = WEAR_HEAD;;
		*dam = dice(GET_LEVEL(ch), 7);
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 6 RL_SEC;
		*vict_wait = 2 RL_SEC;
		*fail_pos = POS_SITTING;
		break;

	case SKILL_SCISSOR_KICK:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		*dam = dice(GET_LEVEL(ch), 9);
		ADD_EQ_DAM(ch, WEAR_FEET);
		*wait = 6 RL_SEC;
		*vict_wait = 2 RL_SEC;
		*fail_pos = POS_SITTING;
		*move = 10;
		break;

	case SKILL_RIDGEHAND:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		need_hand = 1;
		*loc = WEAR_NECK_1;
		*dam = dice(GET_LEVEL(ch), 11);
		ADD_EQ_DAM(ch, WEAR_HANDS);
		*wait = 4 RL_SEC;
		*vict_wait = 1 RL_SEC;
		*move = 10;
		break;


	case SKILL_DEATH_TOUCH:
		if (IS_PUDDING(vict) || IS_SLIME(vict))
			prob = 0;

		need_hand = 1;
		*loc = WEAR_NECK_1;
		*dam = dice(GET_LEVEL(ch), 15);
		*wait = 7 RL_SEC;
		*vict_wait = (2 + number(0, GET_LEVEL(ch) >> 3)) RL_SEC;
		*move = 35;
		break;

	case SKILL_HIP_TOSS:
		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect
			|| NON_CORPOREAL_MOB(vict) || MOB_FLAGGED(vict, MOB_NOBASH))
			prob = 0;

		need_hand = 1;
		*dam = dice(2, GET_LEVEL(ch));
		ADD_EQ_DAM(ch, WEAR_WAIST);
		*wait = 7 RL_SEC;
		*vict_wait = 3 RL_SEC;
		*vict_pos = POS_RESTING;
		*move = 10;
		break;

		 /** merc offensive skills **/
	case SKILL_SHOULDER_THROW:
		if (IS_PUDDING(vict) || IS_SLIME(vict) || bad_sect
			|| NON_CORPOREAL_MOB(vict) || MOB_FLAGGED(vict, MOB_NOBASH))
			prob = 0;

		need_hand = 1;
		*dam = dice(3, (GET_LEVEL(ch) >> 1) + GET_STR(ch));
		*wait = 6 RL_SEC;
		*vict_wait = 2 RL_SEC;
		*vict_pos = POS_RESTING;
		*move = 15;
		break;

		/** psionic skillz **/
	case SKILL_PSIBLAST:

		if (NULL_PSI(vict))
			prob = 0;

		*dam =
			dice(ch->getLevelBonus(SKILL_PSIBLAST) * 2 / 3, GET_INT(ch) + 1);

		*dam += CHECK_SKILL(ch, SKILL_PSIBLAST);

		if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_PSI))
			*dam >>= 1;
		*wait = 5 RL_SEC;
		*vict_wait = 2 RL_SEC;
		*mana = mag_manacost(ch, SKILL_PSIBLAST);
		*move = 10;
		break;

	default:
		slog("SYSERR: Illegal skillnum <%d> passed to calc_skill_prob().",
			skillnum);
		send_to_char(ch, "There was an error.\r\n");
		return -1;
	}

	if (need_hand) {
		if (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
			send_to_char(ch, 
				"You are using both hands to wield your weapon right now!\r\n");
			return -1;
		}
		if (GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_WIELD_2) ||
				GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_SHIELD))) {
			send_to_char(ch, "You need a hand free to do that!\r\n");
			return -1;
		}
	}

	if (*move && GET_MOVE(ch) < *move) {
		send_to_char(ch, "You are too exhausted!\r\n");
		return -1;
	}
	if (*mana && GET_MANA(ch) < *mana) {
		send_to_char(ch, "You lack the spiritual energies needed!\r\n");
		return -1;
	}

	*dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
	*dam += GET_DAMROLL(ch);

	*dam += (*dam * GET_REMORT_GEN(ch)) / 10;

	if (CHECK_SKILL(ch, skillnum) > LEARNED(ch))
		*dam *= (LEARNED(ch) +
			((CHECK_SKILL(ch, skillnum) - LEARNED(ch)) >> 1));
	else
		*dam *= CHECK_SKILL(ch, skillnum);
	*dam /= LEARNED(ch);

	if (NON_CORPOREAL_UNDEAD(vict))
		*dam = 0;

	prob = MIN(MAX(0, prob), 110);

	return (prob);
}

ACCMD(do_offensive_skill)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	struct affected_type af;
	int prob = -1, wait = 0, vict_wait = 0, dam = 0, vict_pos = 0, fail_pos =
		0, loc = -1, move = 0, mana = 0;
	int my_return_flags;

	ACMD_set_return_flags(0);

	one_argument(argument, arg);
	af.is_instant = 0;

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			sprintf(buf, "You fiercely %s $p!", CMD_NAME);
			act(buf, FALSE, ch, ovict, 0, TO_CHAR);
			sprintf(buf, "$n fiercely %ss $p!", CMD_NAME);
			act(buf, FALSE, ch, ovict, 0, TO_ROOM);
			return;
		} else {
			send_to_char(ch, "%s who?\r\n", CMD_NAME);
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Aren't we funny today...\r\n");
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	if (SPELL_IS_PSIONIC(subcmd) && ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)
		&& GET_LEVEL(ch) < LVL_GOD) {
		send_to_char(ch, "Psychic powers are useless here!\r\n");
		return;
	}
	if ((subcmd == SKILL_SWEEPKICK || subcmd == SKILL_TRIP) &&
		vict->getPosition() <= POS_SITTING) {
		act("$N is already on the ground.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	af.type = 0;

	//
	// calc_skill_prob returns -1 if you cannot perform that skill,
	// or if somethine exceptional happened and we need to return
	//

	if ((prob =
			calc_skill_prob(ch, vict, subcmd,
				&wait, &vict_wait, &move, &mana,
				&dam, &fail_pos, &vict_pos, &loc, &af,
				&my_return_flags)) < 0) {
		cur_weap = NULL;
		ACMD_set_return_flags(my_return_flags);
		return;
	}

	if (IS_MONK(ch) && !IS_NEUTRAL(ch) && GET_LEVEL(ch) < LVL_GOD) {
		prob -= (prob * (ABS(GET_ALIGNMENT(ch)))) / 1000;
		dam -= (dam * (ABS(GET_ALIGNMENT(ch)))) / 2000;
	}
	//
	// skill failure
	//

	if (prob < number(1, 120)) {
		if (damage(ch, vict, 0, subcmd, loc))
			return;
		if (fail_pos) {
			ch->setPosition(fail_pos);
			if (prob < 50) {
				// 0.1 sec for every point below 50, up to 7 sec
				int tmp_wait = 50 - prob;
				tmp_wait = MAX(tmp_wait, 70);
				wait += tmp_wait;
				slog("%s failed %s miserably, tacking on %d x0.1 sec",
					GET_NAME(ch), spell_to_str(subcmd), tmp_wait);
			}
		}

		if (move)
			GET_MOVE(ch) -= (move >> 1);
		if (mana)
			GET_MANA(ch) -= (mana >> 1);

		WAIT_STATE(ch, (wait >> 1));

	}
	//
	// skill success
	//

	else {
		if (move)
			GET_MOVE(ch) -= move;
		if (mana)
			GET_MANA(ch) -= mana;

		gain_skill_prof(ch, subcmd);
		WAIT_STATE(ch, wait);

		my_return_flags = damage(ch, vict, dam, subcmd, loc);

		//
		// set waits, position, and affects on victim if they are still alive
		//

		if ((!IS_SET(my_return_flags, DAM_ATTACK_FAILED))
			&& (!IS_SET(my_return_flags, DAM_VICT_KILLED))) {
			if (vict_pos)
				vict->setPosition(vict_pos);
			if (vict_wait)
				WAIT_STATE(vict, vict_wait);

			if (af.type && !affected_by_spell(vict, af.type))
				affect_to_char(vict, &af);
		}
	}
}


ACMD(do_assist)
{
	struct Creature *helpee;

	if (FIGHTING(ch)) {
		send_to_char(ch, 
			"You're already fighting!  How can you assist someone else?\r\n");
		return;
	}
	one_argument(argument, arg);

	if (!*arg)
		send_to_char(ch, "Whom do you wish to assist?\r\n");
	else if (!(helpee = get_char_room_vis(ch, arg))) {
		send_to_char(ch, NOPERSON);
		WAIT_STATE(ch, 4);
	} else if (helpee == ch) {
		send_to_char(ch, "You can't help yourself any more than this!\r\n");
	} else {
		CreatureList::iterator opponent = ch->in_room->people.begin();
		for (;
			opponent != ch->in_room->people.end()
			&& (FIGHTING((*opponent)) != helpee); ++opponent);

		if (opponent == ch->in_room->people.end()) {
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		} else if (!CAN_SEE(ch, (*opponent))) {
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee,
				TO_CHAR);
		} else if (!IS_NPC(ch) && !IS_NPC((*opponent))
			&& !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
			act("That rescue would entail attacking $N, but you are flagged NO PK.", FALSE, ch, 0, (*opponent), TO_CHAR);
			return;
		} else {
			send_to_char(ch, "You join the fight!\r\n");
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, (*opponent), TYPE_UNDEFINED);
			WAIT_STATE(ch, 1 RL_SEC);
		}
	}
}


ACMD(do_hit)
{
	struct Creature *vict;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char(ch, "Hit who?\r\n");
	else if (!(vict = get_char_room_vis(ch, arg))) {
		send_to_char(ch, "They don't seem to be here.\r\n");
		WAIT_STATE(ch, 4);
	} else if (vict == ch) {
		send_to_char(ch, "You hit yourself...OUCH!.\r\n");
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
	} else if (FIGHTING(ch) && FIGHTING(ch) == vict)
		send_to_char(ch, "You do the best you can!\r\n");
	else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
		act("$N is just such a good friend, you simply can't hit $M.", FALSE,
			ch, 0, vict, TO_CHAR);
	else {
		if (!peaceful_room_ok(ch, vict, true))
			return;

		if (FIGHTING(ch))
			stop_fighting(ch);

		GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 5);
		hit(ch, vict, TYPE_UNDEFINED);
		if (IS_NPC(vict) && IS_SET(MOB_FLAGS(vict), MOB_ISCRIPT)) {
			MobileEvent *e =
				new MobileEvent(ch, vict, 0, 0, 0, 0, "EVT_PHYSICAL_ATTACK");
			send_to_queue(e);
		}
		WAIT_STATE(ch, PULSE_VIOLENCE);
	}
}



ACMD(do_kill)
{
	struct Creature *vict;

	if ((GET_LEVEL(ch) < LVL_CREATOR) || subcmd != SCMD_SLAY || IS_NPC(ch)) {
		do_hit(ch, argument, cmd, subcmd);
		return;
	}
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Kill who?\r\n");
	} else {
		if (!(vict = get_char_room_vis(ch, arg))) {
			send_to_char(ch, "They aren't here.\r\n");
			WAIT_STATE(ch, 4);
		} else if (ch == vict)
			send_to_char(ch, "Your mother would be so sad.. :(\r\n");
		else if (GET_LEVEL(vict) >= GET_LEVEL(ch)) {
			act("That's a really bad idea.", FALSE, ch, 0, 0, TO_CHAR);
		} else {
			act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict,
				TO_CHAR);
			act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(vict)), NRM, true,
				"%s killed %s with a wiz-slay at %s.",
				GET_NAME(ch), GET_NAME(vict), ch->in_room->name);

			raw_kill(vict, ch, TYPE_SLASH);	// Wiz-slay
		}
	}
}


ACMD(do_order)
{
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH + 56];
	bool found = FALSE;
	struct room_data *org_room;
	struct Creature *vict;
	struct follow_type *k = NULL;

	half_chop(argument, name, message);

	if (!*name || !*message)
		send_to_char(ch, "Order who to do what?\r\n");
	else if (!(vict = get_char_room_vis(ch, name)) &&
		!is_abbrev(name, "followers"))
		send_to_char(ch, "That person isn't here.\r\n");
	else if (ch == vict)
		send_to_char(ch, "You obviously suffer from schizophrenia.\r\n");
	else {
		if (IS_AFFECTED(ch, AFF_CHARM)) {
			send_to_char(ch, 
				"Your superior would not aprove of you giving orders.\r\n");
			return;
		}
		if (vict) {
			send_to_char(vict, CCBLD(vict, C_SPR));
			send_to_char(vict, CCRED(vict, C_NRM));
			sprintf(buf, "$N orders you to '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR);
			send_to_char(vict, CCNRM(vict, C_SPR));
			act("$n gives $N an order.", FALSE, ch, 0, vict, TO_NOTVICT);
			send_to_char(ch, CCBLD(ch, C_SPR));
			send_to_char(ch, CCRED(ch, C_NRM));
			send_to_char(ch, "You order %s to '%s'.\r\n", PERS(vict, ch), message);
			send_to_char(ch, CCNRM(ch, C_SPR));

			if (((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM) ||
					GET_CHA(ch) < number(0, GET_INT(vict))) &&
				(GET_LEVEL(ch) < LVL_CREATOR ||
					GET_LEVEL(vict) >= GET_LEVEL(ch)) &&
				(!IS_VAMPIRE(ch) || !IS_EVIL(ch) || !IS_UNDEAD(vict) ||
					!IS_NPC(vict) ||
					(GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_CONTROL_UNDEAD)) <
					(number(40, 110) + GET_LEVEL(vict))))
				act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
			else {
				if (!CHECK_WAIT(vict) && !GET_MOB_WAIT(vict)) {

					if ((vict->master != ch || !IS_AFFECTED(vict, AFF_CHARM))
						&& GET_LEVEL(ch) < LVL_AMBASSADOR && IS_VAMPIRE(ch)) {
						gain_skill_prof(ch, SKILL_CONTROL_UNDEAD);
					}

					if (IS_NPC(vict) && GET_MOB_VNUM(vict) == 5318)
						do_say(vict, "As you command, master.", 0, 0);
					command_interpreter(vict, message);
				}

			}
		} else {				/* This is order "followers" */
			sprintf(buf, "$n issues the order '%s'.", message);
			act(buf, FALSE, ch, 0, vict, TO_ROOM);

			send_to_char(ch, CCBLD(ch, C_SPR));
			send_to_char(ch, CCRED(ch, C_NRM));
			send_to_char(ch, "You order your followers to '%s'.\r\n", message);
			send_to_char(ch, CCNRM(ch, C_SPR));

			org_room = ch->in_room;

			for (k = ch->followers; k; k = order_next_k) {
				order_next_k = k->next;
				if (org_room == k->follower->in_room)
					if ((IS_AFFECTED(k->follower, AFF_CHARM) &&
							GET_CHA(ch) > number(0, GET_INT(k->follower))) ||
						GET_LEVEL(ch) >= LVL_CREATOR) {
						found = TRUE;
						if (!CHECK_WAIT(k->follower)
							&& !GET_MOB_WAIT(k->follower)) {
							if (IS_NPC(k->follower)
								&& GET_MOB_VNUM(k->follower) == 5318)
								do_say(vict, "As you command, master.", 0, 0);
							command_interpreter(k->follower, message);
						}
					} else
						act("$n has an indifferent look.", TRUE, k->follower,
							0, 0, TO_CHAR);
			}
			order_next_k = NULL;

			if (!found)
				send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
		}
	}
}

//
// if this case, if return_flags has DAM_ATTACKER_KILLED set, it is not absolutely
// certain that he is dead, but you better damn well not mess with his pointer afterwards
//

ACMD(do_flee)
{
	int i, attempt, loss = 0;
	struct Creature *fighting = FIGHTING(ch);

	ACMD_set_return_flags(0);

	if (IS_AFFECTED_2(ch, AFF2_PETRIFIED)) {
		send_to_char(ch, "You are solid stone!\r\n");
		return;
	}
	if (IS_AFFECTED_2(ch, AFF2_BESERK) && FIGHTING(ch) &&
		!number(0, 1 + (GET_INT(ch) >> 2))) {
		send_to_char(ch, "You are too enraged to flee!\r\n");
		return;
	}
	if (ch->getPosition() < POS_FIGHTING) {
		send_to_char(ch, "You can't flee until you get on your feet!\r\n");
		return;
	}
	if (!IS_NPC(ch) && fighting) {
		loss = GET_LEVEL(FIGHTING(ch)) << 5;
		loss += (loss * GET_LEVEL(ch)) / (LVL_GRIMP + 1 - GET_LEVEL(ch));
		loss >>= 5;

		if (IS_REMORT(ch))
			loss -= loss * GET_REMORT_GEN(ch) / (GET_REMORT_GEN(ch) + 2);

		if (IS_THIEF(ch))
			loss >>= 1;
	}
	for (i = 0; i < 6; i++) {
		attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
		if (CAN_GO(ch, attempt) && !NOFLEE(EXIT(ch, attempt)->to_room) &&
			(!IS_NPC(ch) || !ROOM_FLAGGED(ch->in_room, ROOM_NOMOB)) &&
			!IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room),
				ROOM_DEATH | ROOM_GODROOM)) {
			act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
			if (ch->in_room->isOpenAir()
				|| EXIT(ch, attempt)->to_room->isOpenAir()) {
				if (IS_AFFECTED(ch, AFF_INFLIGHT))
					ch->setPosition(POS_FLYING);
				else
					continue;
			}

			int move_result = do_simple_move(ch, attempt, MOVE_FLEE, TRUE);

			//
			// return of 2 indicates critical failure of do_simple_move
			//

			if (move_result == 2)
				ACMD_set_return_flags(DAM_ATTACKER_KILLED);

			if (move_result == 0) {
				send_to_char(ch, "You flee head over heels.\r\n");
				if (loss && fighting) {
					gain_exp(ch, -loss);
					gain_exp(fighting, (loss >> 5));
				}
				if (FIGHTING(ch)) {
					if (FIGHTING(FIGHTING(ch)) == ch)
						stop_fighting(FIGHTING(ch));
					stop_fighting(ch);
				}
				if (ch->in_room->isOpenAir())
					ch->setPosition(POS_FLYING);
			} else if (move_result == 1) {
				act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
			}
			return;
		}
	}
	send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}

static inline int
FLEE_SPEED(Creature * ch)
{
	int speed = GET_DEX(ch);
	if (AFF2_FLAGGED(ch, AFF2_HASTE))
		speed += 20;
	if (AFF_FLAGGED(ch, AFF_ADRENALINE))
		speed += 10;
	if (AFF2_FLAGGED(ch, AFF2_SLOW))
		speed -= 20;
	if (SECT(ch->in_room) == SECT_ELEMENTAL_OOZE)
		speed -= 20;
	return speed;
}

//
// if this case, if return_flags has DAM_ATTACKER_KILLED set, it is not absolutely
// certain that he is dead, but you better damn well not mess with his pointer afterwards
//
ACMD(do_retreat)
{
	int dir;
	bool fighting = 0, found = 0;

	skip_spaces(&argument);
	if (GET_MOVE(ch) < 10) {
		send_to_char(ch, "You are too exhausted to make an effective retreat.\r\n");
		return;
	}
	if (!*argument) {
		send_to_char(ch, "Retreat in which direction?\r\n");
		return;
	}
	if ((dir = search_block(argument, dirs, 0)) < 0) {
		send_to_char(ch, "No such direction, fool.\r\n");
		return;
	}

	if (FIGHTING(ch)) {
		fighting = 1;
		if (CHECK_SKILL(ch, SKILL_RETREAT) + GET_LEVEL(ch) <
			number(60, 70 + GET_LEVEL(FIGHTING(ch)))) {
			send_to_char(ch, "You panic!\r\n");
			do_flee(ch, "", 0, 0);
			return;
		}
	}
	room_data *room = ch->in_room;
	CreatureList::iterator it = room->people.begin();
	for (; it != room->people.end(); ++it) {
		Creature *vict = *it;
		if (vict != ch && ch == FIGHTING(vict) &&
			CAN_SEE(vict, ch) &&
			((IS_NPC(vict) && GET_MOB_WAIT(vict) < 10) ||
				(vict->desc && vict->desc->wait < 10)) &&
			number(0, FLEE_SPEED(ch)) < number(0, FLEE_SPEED(vict))) {
			found = 1;
			int retval = hit(vict, ch, TYPE_UNDEFINED);

			if (retval & DAM_VICT_KILLED) {
				ACMD_set_return_flags(DAM_ATTACKER_KILLED);
				return;
			} else if (retval & DAM_ATTACKER_KILLED) {
				continue;
			}
			if (ch == FIGHTING(vict))
				stop_fighting(vict);
		}
	}

	int retval = perform_move(ch, dir, MOVE_RETREAT, TRUE);

	if (retval == 0) {

		if (fighting && !found)
			gain_skill_prof(ch, SKILL_RETREAT);
		if (FIGHTING(ch))
			stop_fighting(ch);
		if (ch->in_room->isOpenAir())
			ch->setPosition(POS_FLYING);
		GET_MOVE(ch) = (MAX(0, GET_MOVE(ch) - 10));
		return;
	} else if (retval == 1) {
		send_to_char(ch, "You try to retreat, but you can't!\r\n");
		act("$n attempts to retreat, but fails!", TRUE, ch, 0, 0, TO_ROOM);
	}
	// critical failure, possible ch death
	else if (retval == 2) {
		ACMD_set_return_flags(DAM_ATTACKER_KILLED);
		return;
	}
}

#undef FLEE_SPEED

ACMD(do_bash)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict;
	char arg2[MAX_INPUT_LENGTH];
	int percent, prob, door;
	struct room_data *room = NULL;

	two_arguments(argument, arg, arg2);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You bash $p!", FALSE, ch, ovict, 0, TO_CHAR);
			act("$n bashes $p!", FALSE, ch, ovict, 0, TO_ROOM);
			if (GET_OBJ_TYPE(ovict) == ITEM_VEHICLE &&
				(room = real_room(ROOM_NUMBER(ovict))) != NULL &&
				room->people) {
				act("$N bashes the outside of $p!",
					FALSE, room->people, ovict, ch, TO_ROOM);
				act("$N bashes the outside of $p!",
					FALSE, room->people, ovict, ch, TO_CHAR);
			}
			return;
		} else if ((door = find_door(ch, arg, arg2, "bash")) >= 0) {
			if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
				send_to_char(ch, "You cannot bash that!\r\n");
			else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
				send_to_char(ch, "It's already open!\r\n");
			else if (GET_MOVE(ch) < 20)
				send_to_char(ch, "You are to exhausted.\r\n");
			else {
				percent = CHECK_SKILL(ch, SKILL_BREAK_DOOR) +
					(str_app[STRENGTH_APPLY_INDEX(ch)].todam << 3) +
					GET_CON(ch);

				if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
					percent -= 15;
				if (IS_SET(EXIT(ch, door)->exit_info, EX_HEAVY_DOOR))
					percent -= 20;
				if (IS_SET(EXIT(ch, door)->exit_info, EX_REINFORCED))
					percent -= 25;
				if (!GET_COND(ch, FULL))
					percent -= 5;

				if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF) ||
					EXIT(ch, door)->to_room == NULL)
					percent = 0;

				// gods can bash down anything, dammit!!!
				if (GET_LEVEL(ch) > LVL_GRGOD)
					percent += 100;

				prob = dice(3, 8);

				if (percent < number(100, 170)) {
					prob += dice(4, 8);
					GET_HIT(ch) -= prob;
					if (GET_HIT(ch) < -10) {
						sprintf(buf,
							"$n throws $mself against the %s, $s last futile effort.",
							EXIT(ch, door)->keyword ?
							fname(EXIT(ch, door)->keyword) : "door");
						act(buf, FALSE, ch, 0, 0, TO_ROOM);
						send_to_char(ch,
							"You kill yourself as you hurl yourself against the %s!\r\n",
							EXIT(ch, door)->keyword ?
							fname(EXIT(ch, door)->keyword) : "door");
						if (!IS_NPC(ch)) {
							mudlog(GET_INVIS_LVL(ch), NRM, true,
								"%s killed self bashing door at %d.",
								GET_NAME(ch), ch->in_room->number);
						}
						raw_kill(ch, ch, SKILL_BASH);	// Bashing a door to death
						return;
					} else {
						sprintf(buf,
							"$n throws $mself against the %s, in an attempt to break it.",
							EXIT(ch, door)->keyword ?
							fname(EXIT(ch, door)->keyword) : "door");
						act(buf, FALSE, ch, 0, 0, TO_ROOM);
						send_to_char(ch,
							"You slam yourself against the %s in a futile effort.\r\n",
							EXIT(ch, door)->keyword ?
							fname(EXIT(ch, door)->keyword) : "door");
						update_pos(ch);
					}
				} else {
					sprintf(buf, "$n bashes the %s open with a powerful blow!",
						EXIT(ch, door)->keyword ?
						fname(EXIT(ch, door)->keyword) : "door");
					act(buf, FALSE, ch, 0, 0, TO_ROOM);
					send_to_char(ch,
						"The %s gives way under your powerful bash!\r\n",
						EXIT(ch, door)->keyword ? fname(EXIT(ch,
								door)->keyword) : "door");
					REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
					REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
					GET_HIT(ch) -= prob;

					if (number(0, 20) > GET_DEX(ch)) {
						act("$n staggers and falls down.", TRUE, ch, 0, 0,
							TO_ROOM);
						act("You stagger and fall down.", TRUE, ch, 0, 0,
							TO_CHAR);
						ch->setPosition(POS_SITTING);
					}

					if (GET_HIT(ch) < -10) {
						if (!IS_NPC(ch)) {
							mudlog(GET_INVIS_LVL(ch), NRM, true,
								"%s killed self bashing door at %d.",
								GET_NAME(ch), ch->in_room->number);
						}
						raw_kill(ch, ch, SKILL_BASH);	// Bash Door to death
						return;
					} else {
						update_pos(ch);
						gain_skill_prof(ch, SKILL_BREAK_DOOR);
					}

					if (EXIT(ch, door)->to_room->dir_option[rev_dir[door]] &&
						EXIT(ch,
							door)->to_room->dir_option[rev_dir[door]]->
						to_room == ch->in_room) {
						sprintf(buf,
							"The %s is bashed open from the other side!!\r\n",
							EXIT(ch,
								door)->to_room->dir_option[rev_dir[door]]->
							keyword ? fname(EXIT(ch,
									door)->to_room->dir_option[rev_dir[door]]->
								keyword) : "door");
						REMOVE_BIT(EXIT(ch,
								door)->to_room->dir_option[rev_dir[door]]->
							exit_info, EX_CLOSED);
						REMOVE_BIT(EXIT(ch,
								door)->to_room->dir_option[rev_dir[door]]->
							exit_info, EX_LOCKED);
						send_to_room(buf, EXIT(ch, door)->to_room);
					}
				}
				GET_MOVE(ch) -= 20;
			}
			return;
		} else {
			send_to_char(ch, "Bash who?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}

	do_offensive_skill(ch, fname(vict->player.name), 0, SKILL_BASH);

}

ACMD(do_stun)
{
	struct Creature *vict = NULL;
	int percent, prob, wait;

	one_argument(argument, arg);

	if (CHECK_SKILL(ch, SKILL_STUN) < 20) {
		send_to_char(ch, "You are not particularly stunning.\r\n");
		return;
	}
	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else {
			send_to_char(ch, "Who would you like to stun?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Aren't we stunningly funny today...\r\n");
		return;
	}
	if (FIGHTING(ch)) {
		send_to_char(ch, "You're pretty busy right now!\r\n");
		return;
	}
	if ((GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_HOLD) ||
				GET_EQ(ch, WEAR_WIELD_2))) ||
		(GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD)))) {
		send_to_char(ch, "You need at least one hand free to do that!\r\n");
		return;
	}
	if (FIGHTING(vict)) {
		send_to_char(ch, "You aren't able to get the right grip!\r\n");
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	if (!ok_damage_shopkeeper(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
		act("$N stuns you with a swift blow!", FALSE, ch, 0, vict, TO_CHAR);
		act("$N stuns $n with a swift blow to the neck!",
			FALSE, ch, 0, vict, TO_ROOM);
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
		ch->setPosition(POS_STUNNED);
		return;
	}

	appear(ch, vict);

	percent = number(1, 111) + GET_LEVEL(vict);	/* 101% is a complete failure */
	prob = CHECK_SKILL(ch, SKILL_STUN) + (GET_LEVEL(ch) >> 2) +
		(GET_DEX(ch) << 2);

	if (AFF_FLAGGED(vict, AFF_ADRENALINE))
		prob -= GET_LEVEL(vict);

	if (!CAN_SEE(vict, ch))
		prob += GET_LEVEL(ch) >> 1;
	if (AFF_FLAGGED(ch, AFF_SNEAK))
		prob += (CHECK_SKILL(ch, SKILL_SNEAK)) / 10;
	if (!AWAKE(vict))
		prob += 20;
	else
		prob -= GET_DEX(vict);

	if (!IS_NPC(ch))
		prob += (GET_REMORT_GEN(ch) << 3);

	if (IS_PUDDING(vict) || IS_SLIME(vict) || IS_UNDEAD(vict))
		prob = 0;

	if ((prob < percent || MOB2_FLAGGED(vict, MOB2_NOSTUN)) &&
		(GET_LEVEL(ch) < LVL_AMBASSADOR || GET_LEVEL(ch) < GET_LEVEL(vict))) {
		act("$N tried to stun you!", FALSE, vict, 0, ch, TO_CHAR);
		send_to_char(ch, "Uh-oh!  You failed.\r\n");
		set_fighting(ch, vict, TRUE);
		set_fighting(vict, ch, FALSE);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	act("$N grabs a nerve center in your neck!  You are stunned!",
		FALSE, vict, 0, ch, TO_CHAR);
	act("$n stuns $N with a swift blow!", FALSE, ch, 0, vict, TO_NOTVICT);
	send_to_char(ch, "You succeed!\r\n");
	if FIGHTING(vict) {
		stop_fighting(vict);
		stop_fighting(ch);
	}
	if (IS_MOB(vict)) {
		SET_BIT(MOB_FLAGS(vict), MOB_MEMORY);
		remember(vict, ch);
		if (IS_SET(MOB2_FLAGS(vict), MOB2_HUNT))
			HUNTING(vict) = ch;
	}
	wait = 1 + (number(0, GET_LEVEL(ch)) / 5);
	vict->setPosition(POS_STUNNED);
	WAIT_STATE(vict, wait RL_SEC);
	WAIT_STATE(ch, 4 RL_SEC);
	gain_skill_prof(ch, SKILL_STUN);
	check_toughguy(ch, vict, 0);
	check_killer(ch, vict);
}

ACMD(do_feign)
{
	int percent, prob;
	struct Creature *foe = NULL;

	percent = number(1, 101);	/* 101% is a complete failure */
	prob = CHECK_SKILL(ch, SKILL_FEIGN);

	if (prob < percent) {
		send_to_char(ch, "You fall over dead!\r\n");
		act("$n staggers and falls to the ground!", TRUE, ch, 0, 0, TO_ROOM);
	} else {
		if ((foe = FIGHTING(ch))) {
			act("You have killed $N!", FALSE, FIGHTING(ch), 0, ch, TO_CHAR);
			stop_fighting(ch);
			gain_skill_prof(ch, SKILL_FEIGN);

			percent = GET_INT(foe) + GET_LEVEL(foe) + number(-40, 40);
			if (percent < prob && FIGHTING(foe))
				stop_fighting(foe);

		}
		send_to_char(ch, "You fall over dead!\r\n");
		act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0,
			TO_ROOM);
	}
	ch->setPosition(POS_RESTING);

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_tag)
{
	struct Creature *vict = NULL, *tmp_ch = NULL;
	int percent, prob;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		send_to_char(ch, "Who do you want to tag in?\r\n");
		return;
	}
	if (!FIGHTING(ch)) {
		send_to_char(ch, "There is no need.  You aren't fighting!\r\n");
		return;
	}
	if (vict == ch) {
		send_to_char(ch, "Okay! You tag yourself in!...dummy.\r\n");
		return;
	}
	if (FIGHTING(ch) == vict) {
		send_to_char(ch, "They snatch their hand back, refusing the tag!\r\n");
		return;
	}

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end() && (*it)->getFighting() != ch; ++it)
		tmp_ch = *it;

	if (!tmp_ch) {
		act("But nobody is fighting you!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (IS_NPC(vict)) {
		act("You can't tag them in!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (CHECK_SKILL(vict, SKILL_TAG) <= 0) {
		act("$E don't know how to tag in!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	} else {
		percent = number(1, 101);	/* 101% is a complete failure */
		prob = CHECK_SKILL(ch, SKILL_TAG);

		if (percent > prob) {
			send_to_char(ch, "You fail the tag!\r\n");
			act("$n tries to tag you into the fight, but fails.", FALSE, vict,
				0, ch, TO_CHAR);
			return;
		}
		prob = CHECK_SKILL(vict, SKILL_TAG);
		if (percent > prob) {
			act("You try to tag $N, but $E misses the tag!", FALSE, ch, 0,
				vict, TO_CHAR);
			act("$n tries to tag you, but you miss it!", FALSE, vict, 0, ch,
				TO_CHAR);
			return;
		} else {
			act("You tag $N!  $E jumps into the fray!", FALSE, ch, 0, vict,
				TO_CHAR);
			act("$N tags you into the fight!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n tags $N into the fight!", FALSE, ch, 0, vict, TO_NOTVICT);

			if (FIGHTING(vict) == tmp_ch)
				stop_fighting(vict);
			if (FIGHTING(tmp_ch))
				stop_fighting(tmp_ch);
			if (FIGHTING(ch))
				stop_fighting(ch);

			set_fighting(vict, tmp_ch, TRUE);
			set_fighting(tmp_ch, vict, FALSE);
			gain_skill_prof(ch, SKILL_TAG);
		}
	}
}

ACMD(do_rescue)
{
	struct Creature *vict = NULL, *tmp_ch;
	int percent, prob;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		send_to_char(ch, "Who do you want to rescue?\r\n");
		return;
	}
	if (vict == ch) {
		send_to_char(ch, "What about fleeing instead?\r\n");
		return;
	}
	if (FIGHTING(ch) == vict) {
		send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
		return;
	}
	tmp_ch = NULL;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (FIGHTING((*it)) == vict) {
			tmp_ch = *it;
			break;
		}
	}
	if (!tmp_ch) {
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	// check for PKILLER flag
	if (!IS_NPC(ch) && !IS_NPC(tmp_ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
		act("That rescue would entail attacking $N, but you are flagged NO PK.", FALSE, ch, 0, tmp_ch, TO_CHAR);
		return;
	}

	if (GET_CLASS(ch) == CLASS_MAGIC_USER && (GET_REMORT_CLASS(ch) < 0
			|| GET_REMORT_CLASS(ch) == CLASS_MAGIC_USER))
		send_to_char(ch, "But only true warriors can do this!");
	else {
		percent = number(1, 101);	/* 101% is a complete failure */
		prob = CHECK_SKILL(ch, SKILL_RESCUE);

		if (percent > prob) {
			send_to_char(ch, "You fail the rescue!\r\n");
			return;
		}
		send_to_char(ch, "Banzai!  To the rescue...\r\n");
		act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch,
			TO_CHAR);
		act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

		if (FIGHTING(vict) == tmp_ch)
			stop_fighting(vict);
		if (FIGHTING(tmp_ch))
			stop_fighting(tmp_ch);
		if (FIGHTING(ch))
			stop_fighting(ch);

		set_fighting(ch, tmp_ch, TRUE);
		set_fighting(tmp_ch, ch, FALSE);

		WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
		gain_skill_prof(ch, SKILL_RESCUE);
	}
}


ACMD(do_tornado_kick)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	int percent, prob, dam;
	bool dead = 0;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You spin into the air, kicking $p!", FALSE, ch, ovict, 0,
				TO_CHAR);
			act("$n spins into the air, kicking $p!", FALSE, ch, ovict, 0,
				TO_ROOM);
			return;
		} else {
			send_to_char(ch, 
				"Upon who would you like to inflict your tornado kick?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Aren't we funny today...\r\n");
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;
	if (GET_MOVE(ch) < 30) {
		send_to_char(ch, "You are too exhausted!\r\n");
		return;
	}

	percent = ((40 - (GET_AC(vict) / 10)) >> 1) + number(1, 96);
	prob =
		CHECK_SKILL(ch,
		SKILL_TORNADO_KICK) + ((GET_DEX(ch) + GET_STR(ch)) >> 1);
	if (vict->getPosition() < POS_RESTING)
		prob += 30;
	prob -= GET_DEX(vict);

	dam = dice(GET_LEVEL(ch), 6) +
		(str_app[STRENGTH_APPLY_INDEX(ch)].todam) + GET_DAMROLL(ch);
	if (!IS_NPC(ch))
		dam += (dam * GET_REMORT_GEN(ch)) / 10;

	if (CHECK_SKILL(ch, SKILL_TORNADO_KICK) > LEARNED(ch))
		dam *= (CHECK_SKILL(ch, SKILL_TORNADO_KICK) +
			((CHECK_SKILL(ch, SKILL_TORNADO_KICK) - LEARNED(ch)) >> 1));
	else
		dam *= CHECK_SKILL(ch, SKILL_TORNADO_KICK);
	dam /= LEARNED(ch);

	RAW_EQ_DAM(ch, WEAR_FEET, &dam);

	if (NON_CORPOREAL_UNDEAD(vict))
		dam = 0;

	WAIT_STATE(ch, PULSE_VIOLENCE * 3);

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_TORNADO_KICK, -1);
		GET_MOVE(ch) -= 10;
	} else {
		GET_MOVE(ch) -= 10;
		if (!(dead = damage(ch, vict, dam, SKILL_TORNADO_KICK, -1)) &&
			GET_LEVEL(ch) > number(30, 55) &&
			(!(dead = damage(ch, vict,
						number(0, 1 + GET_LEVEL(ch) / 10) ?
						(int)(dam * 1.2) :
						0,
						SKILL_TORNADO_KICK, -1))) &&
			(GET_MOVE(ch) -= 10) &&
			GET_LEVEL(ch) > number(40, 55) &&
			(!(dead = damage(ch, vict,
						number(0, 1 + GET_LEVEL(ch) / 10) ?
						(int)(dam * 1.3) : 0, SKILL_TORNADO_KICK, -1))))
			GET_MOVE(ch) -= 10;
		gain_skill_prof(ch, SKILL_TORNADO_KICK);

	}
}


ACMD(do_sleeper)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	int percent, prob;

	ACMD_set_return_flags(0);

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You try to put the sleeper on $p!", FALSE, ch, ovict, 0,
				TO_CHAR);
			act("$n tries to put the sleeper on $p!", FALSE, ch, ovict, 0,
				TO_ROOM);
			return;
		} else {
			send_to_char(ch, "Who do you want to put to sleep?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Aren't we funny today...\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
		send_to_char(ch, 
			"You are using both hands to wield your weapon right now!\r\n");
		return;
	}
	if (IS_NPC(vict) && IS_UNDEAD(vict)) {
		act("$N is undead! You can't put it to sleep!",
			TRUE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (vict->getPosition() <= POS_SLEEPING) {
		send_to_char(ch, "Yeah.  Right.\r\n");
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	percent = ((10 + GET_LEVEL(vict)) >> 1) + number(1, 101);
	prob = CHECK_SKILL(ch, SKILL_SLEEPER);

	if (AFF_FLAGGED(vict, AFF_ADRENALINE))
		prob -= GET_LEVEL(vict);

	if (IS_PUDDING(vict) || IS_SLIME(vict))
		prob = 0;

	WAIT_STATE(ch, 3 RL_SEC);

	//
	// failure
	//

	if (percent > prob || MOB_FLAGGED(vict, MOB_NOSLEEP) ||
		GET_LEVEL(vict) > LVL_CREATOR) {
		int retval = damage(ch, vict, 0, SKILL_SLEEPER, WEAR_NECK_1);
		ACMD_set_return_flags(retval);
	}
	//
	// success
	//

	else {
		gain_skill_prof(ch, SKILL_SLEEPER);
		int retval = damage(ch, vict, 18, SKILL_SLEEPER, WEAR_NECK_1);

		ACMD_set_return_flags(retval);

		//
		// put the victim to sleep if he's still alive
		//
		if (IS_SET(retval, DAM_ATTACK_FAILED))
			return;

		if (!IS_SET(retval, DAM_VICT_KILLED)) {

			if (FIGHTING(vict))
				stop_fighting(vict);
			WAIT_STATE(vict, 4 RL_SEC);
			vict->setPosition(POS_SLEEPING);

			if (IS_SET(retval, DAM_ATTACKER_KILLED))
				return;

			if (FIGHTING(ch))
				stop_fighting(ch);
			remember(vict, ch);
		}
	}
}

ACMD(do_turn)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	int percent, prob;

	if (GET_CLASS(ch) != CLASS_CLERIC && GET_CLASS(ch) != CLASS_KNIGHT &&
		GET_REMORT_CLASS(ch) != CLASS_CLERIC
		&& GET_REMORT_CLASS(ch) != CLASS_KNIGHT
		&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "Heathens are not able to turn the undead!\r\n");
		return;
	}
	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You turn $p!", FALSE, ch, ovict, 0, TO_CHAR);
			act("$n turns $p!", FALSE, ch, ovict, 0, TO_ROOM);
			return;
		} else {
			send_to_char(ch, "Turn who?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "You try to flee from yourself, to no avail.\r\n");
		return;
	}
	if (!IS_NPC(vict)) {
		send_to_char(ch, "You cannot turn other players!\r\n");
		return;
	}
	if (!IS_UNDEAD(vict)) {
		send_to_char(ch, "You can only turn away the undead.\r\n");
		return;
	}

	if (!peaceful_room_ok(ch, vict, true))
		return;
	percent = ((GET_LEVEL(vict)) + number(1, 101));	/* 101% is a complete
													 * failure */
	if (MOB_FLAGGED(vict, MOB_NOTURN))
		percent += GET_LEVEL(vict);

	prob = CHECK_SKILL(ch, SKILL_TURN);

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_TURN, -1);
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	} else {
		if ((GET_LEVEL(ch) - GET_LEVEL(vict) - number(0, 10)) > 20) {
			act("$n calls upon the power of $s deity and DESTROYS $N!!",
				FALSE, ch, 0, vict, TO_ROOM);
			act("You call upon the power of your deity and DESTROY $N!!",
				TRUE, ch, 0, vict, TO_CHAR);
			gain_exp(ch, GET_EXP(vict));

			slog("%s killed %s with a turn at %d.",
				GET_NAME(ch), GET_NAME(vict), ch->in_room->number);
			gain_skill_prof(ch, SKILL_TURN);

			raw_kill(vict, ch, SKILL_TURN);	// Destroying a victime with turn
		} else if ((GET_LEVEL(ch) - GET_LEVEL(vict)) > 10) {
			act("$n calls upon the power of $s deity and forces $N to flee!",
				FALSE, ch, 0, vict, TO_ROOM);
			act("You call upon the power of your deity and force $N to flee!",
				TRUE, ch, 0, vict, TO_CHAR);
			do_flee(vict, "", 0, 0);
			gain_skill_prof(ch, SKILL_TURN);
		} else {
			damage(ch, vict, GET_LEVEL(ch) >> 1, SKILL_TURN, -1);
			WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			gain_skill_prof(ch, SKILL_TURN);
		}
	}
}

ACMD(do_shoot)
{

	struct Creature *vict = NULL, *tmp_vict = NULL;
	struct obj_data *gun = NULL, *target = NULL, *bullet = NULL;
	sh_int prob, dam, cost;
	int i, dum_ptr = 0, dum_move = 0;
	bool dead = false;
	struct affected_type *af = NULL;
	char arg[MAX_INPUT_LENGTH];
	int my_return_flags = 0;

	ACMD_set_return_flags(0);

	argument = one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Shoot what at who?\r\n");
		return;
	}

	if (!strncmp(arg, "internal", 8)) {

		argument = one_argument(argument, arg);

		if (!*arg) {
			send_to_char(ch, "Discharge which implant at who?\r\n");
			return;
		}

		if (!(gun = get_object_in_equip_vis(ch, arg, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg),
				arg);
			return;
		}

	} else if ((!(gun = GET_EQ(ch, WEAR_WIELD)) || !isname(arg, gun->name)) &&
		(!(gun = GET_EQ(ch, WEAR_WIELD_2)) || !isname(arg, gun->name))) {
		send_to_char(ch, "You are not wielding %s '%s'.\r\n", AN(arg), arg);
		return;
	}

	if (!IS_ENERGY_GUN(gun) && !IS_GUN(gun)) {
		act("$p is not a gun.", FALSE, ch, gun, 0, TO_CHAR);
		return;
	}

	if (FIGHTING(ch) && ch == FIGHTING(FIGHTING(ch)) && !number(0, 3) &&
		(!IS_MERC(ch)) && (GET_LEVEL(ch) < LVL_GRGOD)) {
		send_to_char(ch, "You are in too close to get off a shot!\r\n");
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {

		if (!(vict = FIGHTING(ch))) {

			if (IS_GUN(gun) && !IS_ARROW(gun) && GUN_TYPE(gun) >= 0
				&& GUN_TYPE(gun) < NUM_GUN_TYPES) {

				if (!(bullet = gun->contains) || (!MAX_LOAD(gun)
						&& !(bullet = bullet->contains)))
					act("$p is not loaded.", FALSE, ch, gun, 0, TO_CHAR);
				else {
					act("$n fires $p into the air.", FALSE, ch, gun, 0,
						TO_ROOM);
					act("You fire $p into the air.", FALSE, ch, gun, 0,
						TO_CHAR);
					sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS,	/*GUN_TYPE(gun), */
						dice(gun_damage[GUN_TYPE(gun)][0],
							gun_damage[GUN_TYPE(gun)][1]) +
						BUL_DAM_MOD(bullet), 1);
					extract_obj(bullet);
				}
			} else
				act("Shoot $p at what?", FALSE, ch, gun, 0, TO_CHAR);
			return;
		}
	} else if (!(vict = get_char_room_vis(ch, argument)) &&
		!(target = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
		send_to_char(ch, "You don't see %s '%s' here.\r\n", AN(argument),
			argument);
		WAIT_STATE(ch, 4);
		return;
	}
	if (vict) {
		if (vict == ch) {
			send_to_char(ch, "Aren't we funny today...\r\n");
			return;
		}
		if (!peaceful_room_ok(ch, vict, true))
			return;
	}
	//
	// The Energy Gun block 
	//

	if (IS_ENERGY_GUN(gun)) {

		if (!gun->contains || !IS_ENERGY_CELL(gun->contains)) {
			act("$p is not loaded with an energy cell.", FALSE, ch, gun, 0,
				TO_CHAR);
			return;
		}
		if (CUR_ENERGY(gun->contains) <= 0) {
			act("$p is out of energy.", FALSE, ch, gun, 0, TO_CHAR);
			if (vict)
				act("$n points $p at $N!", TRUE, ch, gun, vict, TO_ROOM);
			act("$p hums faintly in $n's hand.", FALSE, ch, gun, 0, TO_ROOM);
			return;
		}

		cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));
		if (target) {

			dam = dice(GUN_DISCHARGE(gun), (cost >> 1));

			CUR_ENERGY(gun->contains) -= cost;
			sprintf(buf, "$n blasts %s with $p!", target->short_description);
			act(buf, FALSE, ch, gun, 0, TO_ROOM);
			act("You blast $p!", FALSE, ch, target, 0, TO_CHAR);

			damage_eq(ch, target, dam);
			return;
		}

		prob = calc_skill_prob(ch, vict, SKILL_SHOOT,
			&dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
			&dum_ptr, &dum_ptr, &dum_ptr, af, &my_return_flags);

		if (my_return_flags) {
			ACMD_set_return_flags(my_return_flags);
			return;
		}

		prob += CHECK_SKILL(ch, SKILL_ENERGY_WEAPONS) >> 2;
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it)
			if (*it != ch && ch == FIGHTING((*it)))
				prob -= (GET_LEVEL(*it) >> 3);

		if (FIGHTING(vict) && FIGHTING(vict) != ch && number(1, 121) > prob)
			vict = FIGHTING(vict);
		else if (FIGHTING(vict) && number(1, 101) > prob) {
			it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if ((*it) != ch && (*it) != vict && vict == FIGHTING((*it)) &&
					!number(0, 2)) {
					vict = (*it);
					break;
				}
			}
		} else if (number(1, 81) > prob) {
			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if ((*it) != ch && (*it) != vict && !number(0, 2)) {
					vict = (*it);
					break;
				}
			}
		}

		if (CUR_R_O_F(gun) <= 0)
			CUR_R_O_F(gun) = 1;

		for (i = 0, dead = false; i < CUR_R_O_F(gun); i++) {

			prob -= (i * 4);
			cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));

			dam = dice(GUN_DISCHARGE(gun), (cost >> 1));

			CUR_ENERGY(gun->contains) -= cost;

			cur_weap = gun;

			if (dead == false) {

				//
				// miss
				//

				if (number(0, 121) > prob) {
					check_toughguy(ch, vict, 0);
					check_killer(ch, vict);
					my_return_flags =
						damage(ch, vict, 0, SKILL_ENERGY_WEAPONS, number(0,
							NUM_WEARS - 1));
				}
				//
				// hit
				//

				else {
					check_toughguy(ch, vict, 0);
					check_killer(ch, vict);
					my_return_flags =
						damage(ch, vict, dam, SKILL_ENERGY_WEAPONS, number(0,
							NUM_WEARS - 1));
				}
			}
			//
			// vict is dead, blast the corpse
			//

			else {
				if (ch->in_room->contents && IS_CORPSE(ch->in_room->contents)
					&& CORPSE_KILLER(ch->in_room->contents) ==
					(IS_NPC(ch) ? -GET_MOB_VNUM(ch) : GET_IDNUM(ch))) {
					act("You blast $p with $P!!", FALSE, ch,
						ch->in_room->contents, gun, TO_CHAR);
					act("$n blasts $p with $P!!", FALSE, ch,
						ch->in_room->contents, gun, TO_ROOM);
					if (damage_eq(ch, ch->in_room->contents, dam))
						break;
				} else {
					act("You fire off a round from $P.",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
					act("$n fires off a round from $P.",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				}
			}

			//
			// if the attacker was somehow killed, return immediately
			//

			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
				ACMD_set_return_flags(my_return_flags);
				return;
			}

			if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				ACMD_set_return_flags(my_return_flags);
				dead = true;
			}

			if (!CUR_ENERGY(gun->contains)) {
				act("$p has been depleted of fuel.  Auto switching off.",
					FALSE, ch, gun, 0, TO_CHAR);
				act("$p makes a clicking noise in the hard of $n.",
					TRUE, ch, gun, 0, TO_ROOM);
				break;
			}
		}
		cur_weap = NULL;
		WAIT_STATE(ch, (((i << 1) + 6) >> 1) RL_SEC);
		return;
	}
	//
	// The Projectile Gun block
	//

	if (GUN_TYPE(gun) < 0 || GUN_TYPE(gun) >= NUM_GUN_TYPES) {
		act("$p is a bogus gun.  extracting.", FALSE, ch, gun, 0, TO_CHAR);
		extract_obj(gun);
		return;
	}

	if (!(bullet = gun->contains)) {
		act("$p is not loaded.", FALSE, ch, gun, 0, TO_CHAR);
		return;
	}

	if (!MAX_LOAD(gun) && !(bullet = gun->contains->contains)) {
		act("$P is not loaded.", FALSE, ch, gun, gun->contains, TO_CHAR);
		return;
	}

	if (target) {

		dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
		dam += BUL_DAM_MOD(bullet);

		if (IS_ARROW(gun)) {
			sprintf(buf, "$n fires %s from $p into $P!",
				bullet->short_description);
			act(buf, FALSE, ch, gun, target, TO_ROOM);
			act("You fire $P into $p!", FALSE, ch, target, bullet, TO_CHAR);
			obj_from_obj(bullet);
			obj_to_room(bullet, ch->in_room);
			damage_eq(NULL, bullet, dam >> 2);
			damage_eq(ch, target, dam);
			return;
		} else {
			sprintf(buf, "$n blasts %s with $p!", target->short_description);
			act(buf, FALSE, ch, gun, 0, TO_ROOM);
			act("You blast $p!", FALSE, ch, target, 0, TO_CHAR);
			extract_obj(bullet);
			damage_eq(ch, target, dam);
			return;
		}
	}

	prob = calc_skill_prob(ch, vict,
		(IS_ARROW(gun) ? SKILL_ARCHERY : SKILL_SHOOT),
		&dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
		&dum_ptr, &dum_ptr, &dum_ptr, af, &my_return_flags);


	if (my_return_flags) {
		ACMD_set_return_flags(my_return_flags);
		return;
	}


	if (IS_ARROW(gun)) {
		if (IS_ELF(ch))
			prob +=
				number(GET_LEVEL(ch) >> 2,
				GET_LEVEL(ch) >> 1) + (GET_REMORT_GEN(ch) << 2);
	} else
		prob += CHECK_SKILL(ch, SKILL_PROJ_WEAPONS) >> 3;

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((*it) != ch && ch == FIGHTING((*it)))
			prob -= (GET_LEVEL((*it)) >> 3);
	}

	if (FIGHTING(ch)) {
		if (ch == FIGHTING(FIGHTING(ch)))
			prob -= 20;
		else
			prob -= 10;
	}

	if (FIGHTING(vict) && FIGHTING(vict) != ch && number(1, 121) > prob)
		vict = FIGHTING(vict);
	else if (FIGHTING(vict) && number(1, 101) > prob) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (*it != ch && tmp_vict != vict && vict == FIGHTING((*it)) &&
				!number(0, 2)) {
				vict = *it;
				break;
			}
		}
	} else if (number(1, 81) > prob) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (*it != ch && tmp_vict != vict && vict == FIGHTING((*it)) &&
				!number(0, 2)) {
				vict = *it;
				break;
			}
		}
	}

	if (CUR_R_O_F(gun) <= 0)
		CUR_R_O_F(gun) = 1;

	// loop through ROF of the gun for burst fire

	for (i = 0, dead = 0; i < CUR_R_O_F(gun); i++) {

		if (!bullet) {
			act("$p is out of ammo.",
				FALSE, ch, gun->contains ? gun->contains : gun, 0, TO_CHAR);
			cur_weap = NULL;
			return;
		}

		prob -= (i * 4);

		dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
		dam += BUL_DAM_MOD(bullet);

		if (IS_ARROW(gun)) {
			obj_from_obj(bullet);
			obj_to_room(bullet, ch->in_room);
			strcpy(buf2, bullet->short_description);
			damage_eq(NULL, bullet, dam >> 2);
		} else {
			if (!i && !IS_FLAMETHROWER(gun))
				sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS,
					/*GUN_TYPE(gun), */ dam, CUR_R_O_F(gun));

			extract_obj(bullet);
		}
		/* we /must/ have a clip in a clipped gun at this point! */
		bullet = MAX_LOAD(gun) ? gun->contains : gun->contains->contains;

		cur_weap = gun;

		if (dead == false) {

			//
			// miss
			//

			if (number(0, 121) > prob) {
				if (IS_ARROW(gun)) {
					sprintf(buf, "$n fires %s at you from $p!", buf2);
					act(buf, FALSE, ch, gun, vict, TO_VICT);
					sprintf(buf, "You fire %s at $N from $p!", buf2);
					act(buf, FALSE, ch, gun, vict, TO_CHAR);
					sprintf(buf, "$n fires %s at $N from $p!", buf2);
					act(buf, FALSE, ch, gun, vict, TO_NOTVICT);
				}
				my_return_flags =
					damage(ch, vict, 0,
					IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER : (IS_ARROW(gun) ?
						SKILL_ARCHERY : SKILL_PROJ_WEAPONS), number(0,
						NUM_WEARS - 1));
			}
			//
			// hit
			//

			else if (!dead) {
				if (IS_ARROW(gun)) {
					sprintf(buf, "$n fires %s into you from $p!  OUCH!!",
						buf2);
					act(buf, FALSE, ch, gun, vict, TO_VICT);
					sprintf(buf, "You fire %s into $N from $p!", buf2);
					act(buf, FALSE, ch, gun, vict, TO_CHAR);
					sprintf(buf, "$n fires %s into $N from $p!", buf2);
					act(buf, FALSE, ch, gun, vict, TO_NOTVICT);
				}
				my_return_flags =
					damage(ch, vict, dam,
					IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER : (IS_ARROW(gun) ?
						SKILL_ARCHERY : SKILL_PROJ_WEAPONS), number(0,
						NUM_WEARS - 1));
			}
		}
		//
		// vict is dead, blast the corpse
		//

		else {
			if (ch->in_room->contents && IS_CORPSE(ch->in_room->contents) &&
				CORPSE_KILLER(ch->in_room->contents) ==
				(IS_NPC(ch) ? -GET_MOB_VNUM(ch) : GET_IDNUM(ch))) {
				if (IS_ARROW(gun)) {
					act("You shoot $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_CHAR);
					act("$n shoots $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				} else {
					act("You blast $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_CHAR);
					act("$n blasts $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				}
				if (damage_eq(ch, ch->in_room->contents, dam))
					break;
			} else {
				act("You fire off a round from $P.",
					FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				act("$n fires off a round from $P.",
					FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
			}
		}

		//
		// if the attacker was somehow killed, return immediately
		//

		if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
			if (return_flags) {
				*return_flags = my_return_flags;
			}
			return;
		}

		if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
			if (return_flags) {
				*return_flags = my_return_flags;
			}
			dead = true;
		}

	}
	if (IS_ARROW(gun) && IS_ELF(ch))
		WAIT_STATE(ch, (((i << 1) + 6) >> 2) RL_SEC);
	else
		WAIT_STATE(ch, (((i << 1) + 6) >> 1) RL_SEC);

}

ACMD(do_ceasefire)
{
	struct Creature *f = NULL;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (ch == FIGHTING((*it))) {
			f = *it;
			break;
		}
	}
	if (!FIGHTING(ch))
		send_to_char(ch, "You aren't fighting anyone.\r\n");
	else if ((FIGHTING(ch) == FIGHTING(FIGHTING(ch))))
		send_to_char(ch, 
			"You can't ceasefire while your enemy is actively attacking you.\r\n");
	else {
		act("You stop attacking $N.", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
		act("$n stops attacking $N.", FALSE, ch, 0, FIGHTING(ch), TO_NOTVICT);
		act("$n stops attacking you.", FALSE, ch, 0, FIGHTING(ch), TO_VICT);
		WAIT_STATE(ch, 2 RL_SEC);
		if (f) {
			act("You start defending yourself against $N.", FALSE, ch, 0, f,
				TO_CHAR);
			ch->setFighting(f);
		} else
			stop_fighting(ch);
	}

	ch->setPosition(POS_STANDING);
}

ACCMD(do_disarm)
{
	struct Creature *vict = NULL;
	struct obj_data *weap = NULL, *weap2 = NULL;
	int percent, prob;
	ACCMD(do_drop);
	ACCMD(do_get);

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else {
			send_to_char(ch, "Who do you want to disarm?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Use remove.\r\n");
		return;
	}

	if (!peaceful_room_ok(ch, vict, true))
		return;

	if (!(weap = GET_EQ(vict, WEAR_WIELD))) {
		send_to_char(ch, "They aren't wielding anything, fool.\r\n");
		return;
	}

	percent = ((GET_LEVEL(vict)) + number(1, 101));	/* 101% is a complete
													 * failure */
	prob = MAX(0, GET_LEVEL(ch) +
		CHECK_SKILL(ch, SKILL_DISARM) - weap->getWeight());

	prob += GET_DEX(ch) - GET_DEX(vict);
	if (IS_OBJ_STAT2(weap, ITEM2_TWO_HANDED))
		prob -= 60;
	if (IS_OBJ_STAT(weap, ITEM_MAGIC))
		prob -= 20;
	if (IS_OBJ_STAT2(weap, ITEM2_CAST_WEAPON))
		prob -= 10;
	if (IS_OBJ_STAT(weap, ITEM_NODROP))
		prob = 0;
	if (AFF3_FLAGGED(vict, AFF3_ATTRACTION_FIELD))
		prob = 0;

	if (prob > percent) {
		act("You knock $p out of $N's hand!", FALSE, ch, weap, vict, TO_CHAR);
		act("$n knocks $p out of $N's hand!", FALSE, ch, weap, vict,
			TO_NOTVICT);
		act("$n knocks $p out of your hand!", FALSE, ch, weap, vict, TO_VICT);
		unequip_char(vict, WEAR_WIELD, MODE_EQ);
		obj_to_char(weap, vict);
		if (GET_EQ(vict, WEAR_WIELD_2)) {
			if ((weap2 = unequip_char(vict, WEAR_WIELD_2, MODE_EQ)))
				if (equip_char(vict, weap2, WEAR_WIELD, MODE_EQ))
					return;
		}

		if (GET_STR(ch) + number(0, 20) > GET_STR(vict) + GET_DEX(vict)) {
			do_drop(vict, OBJN(weap, vict), 0, 0);
			if (IS_NPC(vict) && !GET_MOB_WAIT(vict) && AWAKE(vict) &&
				number(0, GET_LEVEL(vict)) > (GET_LEVEL(vict) >> 1))
				do_get(vict, OBJN(weap, vict), 0, 0);
		}

		GET_EXP(ch) += MIN(100, weap->getWeight());
		WAIT_STATE(ch, PULSE_VIOLENCE);

		return;
	} else {
		send_to_char(ch, "You fail the disarm!\r\n");
		act("$n tries to disarm you!", FALSE, ch, 0, vict, TO_VICT);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (IS_NPC(vict))
			hit(vict, ch, TYPE_UNDEFINED);
		return;
	}
}

/***********************************************************************/

ACMD(do_impale)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL, *weap = NULL;
	int percent, prob, dam;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You try to impale $p!", FALSE, ch, ovict, 0, TO_CHAR);
			return;
		} else {
			send_to_char(ch, "Impale who?\r\n");
			return;
		}
	}
	if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && STAB_WEAPON(weap)) ||
			((weap = GET_EQ(ch, WEAR_WIELD_2)) && STAB_WEAPON(weap)) ||
			((weap = GET_EQ(ch, WEAR_HANDS)) && STAB_WEAPON(weap)))) {
		send_to_char(ch, "You need to be using a stabbing weapon.\r\n");
		return;
	}
	if (vict == ch) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
		if (!strcmp(arg, "self") || !strcmp(arg, "me")) {
			if (!(ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))) {
				act("You impale yourself with $p!", FALSE, ch, weap, 0,
					TO_CHAR);
				act("$n suddenly impales $mself with $p!", TRUE, ch, weap, 0,
					TO_ROOM);
				mudlog(GET_INVIS_LVL(ch), NRM, true,
					"%s killed self with an impale at %d.",
					GET_NAME(ch), ch->in_room->number);
				gain_exp(ch, -(GET_LEVEL(ch) * 1000));
				raw_kill(ch, ch, SKILL_IMPALE);	// Impaling yourself
				return;
			} else {
				send_to_char(ch, "Suicide is not the answer...\r\n");
				return;
			}
		} else {
			act("Are you sure $p is supposed to go there?", FALSE, ch, weap, 0,
				TO_CHAR);
			return;
		}

	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);
	prob = CHECK_SKILL(ch, SKILL_IMPALE);

	if (IS_PUDDING(vict) || IS_SLIME(vict))
		prob = 0;

	cur_weap = weap;
	dam = (dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2)) << 1) +
		(weap->getWeight() << 2) +
		(str_app[STRENGTH_APPLY_INDEX(ch)].todam << 2) + GET_DAMROLL(ch);
	if (!IS_NPC(ch))
		dam += (dam * GET_REMORT_GEN(ch)) / 10;

	if (CHECK_SKILL(ch, SKILL_IMPALE) > LEARNED(ch))
		dam *= (CHECK_SKILL(ch, SKILL_IMPALE) +
			((CHECK_SKILL(ch, SKILL_IMPALE) - LEARNED(ch)) >> 1));
	else
		dam *= CHECK_SKILL(ch, SKILL_IMPALE);
	dam /= LEARNED(ch);

	if (IS_OBJ_STAT2(weap, ITEM2_TWO_HANDED) && weap->worn_on == WEAR_WIELD)
		dam <<= 1;

	if (NON_CORPOREAL_UNDEAD(vict))
		dam = 0;

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_IMPALE, WEAR_BODY);
	} else {
		damage(ch, vict, dam, SKILL_IMPALE, WEAR_BODY);
		gain_skill_prof(ch, SKILL_IMPALE);
	}
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_intimidate)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	struct affected_type af;
	int prob = 0;

	one_argument(argument, arg);
	af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			if (subcmd == SKILL_TERRORIZE) {
				act("You attempt to terrorize $p!", FALSE, ch, ovict, 0,
					TO_CHAR);
				act("$n attempts to terrorize $p!", FALSE, ch, ovict, 0,
					TO_ROOM);
			} else {
				act("You attempt to intimidate $p!", FALSE, ch, ovict, 0,
					TO_CHAR);
				act("$n attempts to intimidate $p!", FALSE, ch, ovict, 0,
					TO_ROOM);
			}
			return;
		} else {
			send_to_char(ch, "%s who?\r\n",
				subcmd == SKILL_TERRORIZE ? "Terrorize" : "Intimidate");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (!CAN_SEE(vict, ch)) {
		act("$N doesn't seem to be able to see you.", FALSE, ch, 0, vict,
			TO_CHAR);
		return;
	}
	if (vict == ch) {
		if (subcmd == SKILL_TERRORIZE) {
			send_to_char(ch, "You cannot succeed at this.\r\n");
			return;
		}

		send_to_char(ch, "You attempt to intimidate yourself.\r\n"
			"You feel intimidated!\r\n");
		act("$n intimidates $mself!", TRUE, ch, 0, 0, TO_ROOM);
		if (affected_by_spell(ch, SKILL_INTIMIDATE))
			return;
		af.type = SKILL_INTIMIDATE;
		af.is_instant = 0;
		af.duration = 2 + 2 * (GET_LEVEL(ch) > 40);
		af.modifier = -5;
		af.location = APPLY_HITROLL;
		af.bitvector = 0;
		affect_to_char(vict, &af);
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	prob = GET_LEVEL(vict) + number(1, 51) +
		(IS_NPC(vict) ? (GET_MORALE(vict) >> 1) : GET_LEVEL(vict)) +
		(AFF_FLAGGED(vict, AFF_CONFIDENCE) ? GET_LEVEL(vict) : 0);

	if (!ok_damage_shopkeeper(ch, vict))
		return;

	if (subcmd == SKILL_TERRORIZE) {
		act("You peer into $S soul with terrible malice.",
			FALSE, ch, 0, vict, TO_CHAR);
		act("$n peers into $N's soul with terrible malice.",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n peers into your soul with terrible malice.",
			FALSE, ch, 0, vict, TO_VICT);

		if (affected_by_spell(vict, SKILL_TERRORIZE)) {
			act("$n PANICS and attempt to escape!", FALSE, vict, 0, 0,
				TO_ROOM);
			send_to_char(vict, "You PANIC and attempt to escape!\r\n");
			if (!MOB_FLAGGED(vict, MOB_SENTINEL))
				do_flee(vict, "", 0, 0);

			act("$n cowers in fear!", TRUE, vict, 0, 0, TO_ROOM);
			send_to_char(vict, "You cower in paralyzing fear!\r\n");
		} else if ((affected_by_spell(vict, SPELL_FEAR) ||
				GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_TERRORIZE) > prob) &&
			!IS_UNDEAD(vict)) {
			af.type = SKILL_TERRORIZE;
			af.duration = 2 + 2 * (GET_LEVEL(ch) > 40);
			af.modifier = -5;
			af.location = APPLY_HITROLL;
			af.bitvector = 0;
			affect_to_char(vict, &af);

		} else {
			act("$N glarse at $n with contempt.", TRUE, ch, 0, vict,
				TO_NOTVICT);
			act("$N glares at you with contempt!", TRUE, ch, 0, vict, TO_CHAR);
			send_to_char(vict, "You glare back with contempt!\r\n");
			if (IS_NPC(vict))
				hit(vict, ch, TYPE_UNDEFINED);
		}
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;

	} else {
		act("You attempt to intimidate $N.", FALSE, ch, 0, vict, TO_CHAR);
		act("$n attempts to intimidate $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n attempts to intimidate you.", FALSE, ch, 0, vict, TO_VICT);

		if (affected_by_spell(vict, SKILL_INTIMIDATE)) {
			act("$n cringes in terror!", FALSE, vict, 0, 0, TO_ROOM);
			send_to_char(vict, "You cringe in terror!\r\n");
		} else if ((affected_by_spell(vict, SPELL_FEAR) ||
				GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_INTIMIDATE) > prob) &&
			!IS_UNDEAD(vict)) {
			act("$n looks intimidated!", TRUE, vict, 0, 0, TO_ROOM);
			send_to_char(vict, "You feel intimidated!\r\n");

			af.type = SKILL_INTIMIDATE;
			af.duration = 2 + 2 * (GET_LEVEL(ch) > 40);
			af.modifier = -5;
			af.location = APPLY_HITROLL;
			af.bitvector = 0;
			affect_to_char(vict, &af);

		} else {
			act("$N snickers at $n", TRUE, ch, 0, vict, TO_NOTVICT);
			act("$N snickers at you!", TRUE, ch, 0, vict, TO_CHAR);
			send_to_char(vict, "You snicker!\r\n");
			if (IS_NPC(vict))
				hit(vict, ch, TYPE_UNDEFINED);
		}
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}
}



ACMD(do_drain)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	int percent, prob, mana;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			send_to_char(ch, "You can't drain objects!\r\n");
			return;
		} else {
			send_to_char(ch, "Drain whos life force?\r\n");
			WAIT_STATE(ch, 4);
			return;
		}
	}
	if (vict == ch) {
		send_to_char(ch, "Aren't we funny today...\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
		send_to_char(ch, 
			"You are using both hands to wield your weapon right now!\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_WIELD_2) ||
			GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_SHIELD))) {
		send_to_char(ch, "You need a hand free to do that!\r\n");
		return;
	}

	if (!peaceful_room_ok(ch, vict, true))
		return;

	percent = ((10 - (GET_AC(vict) / 10)) >> 1) + number(1, 91);

	prob = CHECK_SKILL(ch, SKILL_DRAIN);

	if (IS_PUDDING(vict) || IS_SLIME(vict))
		prob = 0;

	WAIT_STATE(ch, PULSE_VIOLENCE * 3);

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_DRAIN, -1);
	} else {
		gain_skill_prof(ch, SKILL_DRAIN);

		WAIT_STATE(vict, PULSE_VIOLENCE);
		mana = MIN(GET_MANA(vict), GET_LEVEL(ch) * 5);
		GET_MANA(vict) -= mana;
		GET_MANA(ch) = MAX(GET_MAX_MANA(ch), GET_MANA(ch) + mana);

		damage(ch, vict, ((GET_LEVEL(ch) + GET_STR(ch)) >> 1), SKILL_DRAIN,
			-1);
	}
}

ACMD(do_beguile)
{

	struct Creature *vict = NULL;
	skip_spaces(&argument);

	if (!(vict = get_char_room_vis(ch, argument))) {
		send_to_char(ch, "Beguile who??\r\n");
		WAIT_STATE(ch, 4);
		return;
	}
	if (vict == ch) {
		send_to_char(ch, "You find yourself amazingly beguiling.\r\n");
		return;
	}
	act("$n looks deeply into your eyes with an enigmatic look.",
		TRUE, ch, 0, vict, TO_VICT);
	act("You look deeply into $S eyes with an enigmatic look.",
		FALSE, ch, 0, vict, TO_CHAR);
	act("$n looks deeply into $N's eyes with an enigmatic look.",
		TRUE, ch, 0, vict, TO_NOTVICT);

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || !CAN_SEE(vict, ch))
		return;

	if (GET_INT(vict) < 4) {
		act("$N is too stupid to be beguiled.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	check_toughguy(ch, vict, 0);
	check_killer(ch, vict);
	if (CAN_SEE(vict, ch) &&
		(CHECK_SKILL(ch, SKILL_BEGUILE) + GET_CHA(ch)) >
		(number(0, 50) + GET_LEVEL(vict) + GET_INT(vict)))
		spell_charm(GET_LEVEL(ch), ch, vict, NULL);
	else
		send_to_char(ch, "There appears to be no effect.\r\n");

	WAIT_STATE(ch, 2 RL_SEC);

}

//This function assumes that ch is a merc.  It provides for  mercs
//shooting wielded guns in combat instead of bludgeoning with them

int
do_combat_fire(struct Creature *ch, struct Creature *vict, int weap_pos)
{
	struct Creature *tmp_vict = NULL;
	struct obj_data *bullet = NULL, *gun = NULL;
	sh_int prob, dam, cost;
	int i, dum_ptr = 0, dum_move = 0;
	bool dead = false;
	struct affected_type *af = NULL;
	int my_return_flags = 0;

	if (!(gun = GET_EQ(ch, weap_pos)))
		return 0;
	else if (!IS_ENERGY_GUN(gun) && !IS_GUN(gun))
		return 0;

	//if our victim is NULL we simply return;
	if (vict == NULL) {
		return 0;
	}
	// This should never happen 
	if (vict == ch) {
		mudlog(LVL_AMBASSADOR, NRM, true,
			"ERROR: ch == vict in do_combat_fire()!");
		return 0;
	}

	if (ch->getPosition() < POS_FIGHTING) {
		send_to_char(ch, "You can't fight while sitting!\r\n");
		return 1;
	}
	// And since ch and vict should already be fighting this should never happen either
	if (!peaceful_room_ok(ch, vict, true))
		return 0;

	//
	// The Energy Gun block
	//

	if (IS_ENERGY_GUN(gun)) {

		if (!gun->contains || !IS_ENERGY_CELL(gun->contains)) {
			return 0;
		}
		if (CUR_ENERGY(gun->contains) <= 0) {
			return 0;
		}

		cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));

		prob = calc_skill_prob(ch, vict, SKILL_SHOOT,
			&dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
			&dum_ptr, &dum_ptr, &dum_ptr, af, &my_return_flags);

		prob += CHECK_SKILL(ch, SKILL_ENERGY_WEAPONS) >> 2;


		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (*it != ch && ch == FIGHTING((*it)))
				prob -= (GET_LEVEL((*it)) >> 3);
		}
		if (FIGHTING(vict) && FIGHTING(vict) != ch && number(1, 121) > prob)
			vict = FIGHTING(vict);
		else if (FIGHTING(vict) && number(1, 101) > prob) {
			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (*it != ch && tmp_vict != vict && vict == FIGHTING((*it)) &&
					!number(0, 2)) {
					vict = *it;
					break;
				}
			}

		} else if (number(1, 81) > prob) {
			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (*it != ch && tmp_vict != vict && vict == FIGHTING((*it)) &&
					!number(0, 2)) {
					vict = *it;
					break;
				}
			}
		}

		if (CUR_R_O_F(gun) <= 0)
			CUR_R_O_F(gun) = 1;

		for (i = 0, dead = false; i < CUR_R_O_F(gun); i++) {

			prob -= (i * 4);
			cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));

			dam = dice(GUN_DISCHARGE(gun), (cost >> 1));

			CUR_ENERGY(gun->contains) -= cost;

			cur_weap = gun;

			if (dead == false) {

				//
				// miss
				//

				if (number(0, 121) > prob) {
					check_toughguy(ch, vict, 0);
					check_killer(ch, vict);
					my_return_flags =
						damage(ch, vict, 0, SKILL_ENERGY_WEAPONS, number(0,
							NUM_WEARS - 1));
				}
				//
				// hit
				//

				else {
					check_toughguy(ch, vict, 0);
					check_killer(ch, vict);
					my_return_flags =
						damage(ch, vict, dam, SKILL_ENERGY_WEAPONS, number(0,
							NUM_WEARS - 1));
				}
			}
			//
			// vict is dead, blast the corpse
			//

			else {
				if (ch->in_room->contents && IS_CORPSE(ch->in_room->contents)
					&& CORPSE_KILLER(ch->in_room->contents) ==
					(IS_NPC(ch) ? -GET_MOB_VNUM(ch) : GET_IDNUM(ch))) {
					act("You blast $p with $P!!", FALSE, ch,
						ch->in_room->contents, gun, TO_CHAR);
					act("$n blasts $p with $P!!", FALSE, ch,
						ch->in_room->contents, gun, TO_ROOM);
					if (damage_eq(ch, ch->in_room->contents, dam))
						break;
				} else {
					act("You fire off a round from $P.",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
					act("$n fires off a round from $P.",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				}
			}

			//
			// if the attacker was somehow killed, return immediately
			//

			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
				return 1;
			}

			if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				dead = true;
				return DAM_VICT_KILLED;
			}

			if (!CUR_ENERGY(gun->contains)) {
				act("$p has been depleted of fuel.  Auto switching off.",
					FALSE, ch, gun, 0, TO_CHAR);
				act("$p makes a clicking noise in the hard of $n.",
					TRUE, ch, gun, 0, TO_ROOM);
				break;
			}
		}
		cur_weap = NULL;
		return 1;
	}
	//
	// The Projectile Gun block
	//

	if (GUN_TYPE(gun) < 0 || GUN_TYPE(gun) >= NUM_GUN_TYPES) {
		act("$p is a bogus gun.  extracting.", FALSE, ch, gun, 0, TO_CHAR);
		extract_obj(gun);
		return 0;
	}
	if (!(bullet = gun->contains)) {
		act("$p is not loaded.", FALSE, ch, gun, 0, TO_CHAR);
		return 0;
	}

	if (!MAX_LOAD(gun) && !(bullet = gun->contains->contains)) {
		act("$P is not loaded.", FALSE, ch, gun, gun->contains, TO_CHAR);
		return 0;
	}

	prob = calc_skill_prob(ch, vict,
		(SKILL_SHOOT),
		&dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
		&dum_ptr, &dum_ptr, &dum_ptr, af, &my_return_flags);


	prob += CHECK_SKILL(ch, SKILL_PROJ_WEAPONS) >> 3;

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((*it) != ch && ch == FIGHTING((*it)))
			prob -= (GET_LEVEL((*it)) >> 3);
	}
	if (FIGHTING(vict) && FIGHTING(vict) != ch && number(1, 121) > prob)
		vict = FIGHTING(vict);
	else if (FIGHTING(vict) && number(1, 101) > prob) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if ((*it) != ch && (*it) != vict && vict == FIGHTING((*it)) &&
				!number(0, 2)) {
				vict = (*it);
				break;
			}
		}
	} else if (number(1, 81) > prob) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if ((*it) != ch && (*it) != vict && !number(0, 2)) {
				vict = (*it);
				break;
			}
		}
	}

	if (CUR_R_O_F(gun) <= 0)
		CUR_R_O_F(gun) = 1;

	// loop through ROF of the gun for burst fire

	for (i = 0, dead = 0; i < CUR_R_O_F(gun); i++) {

		if (!bullet) {
			return 0;
		}

		prob -= (i * 4);

		dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
		dam += BUL_DAM_MOD(bullet);

		if (IS_ARROW(gun)) {
			obj_from_obj(bullet);
			obj_to_room(bullet, ch->in_room);
			strcpy(buf2, bullet->short_description);
			damage_eq(NULL, bullet, dam >> 2);
		} else {
			if (!i && !IS_FLAMETHROWER(gun))
				sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS,
					dam, CUR_R_O_F(gun));

			extract_obj(bullet);
		}
		// we /must/ have a clip in a clipped gun at this point!
		bullet = MAX_LOAD(gun) ? gun->contains : gun->contains->contains;

		cur_weap = gun;

		if (dead == false) {

			//
			// miss
			//

			if (number(0, 121) > prob) {
				my_return_flags =
					damage(ch, vict, 0,
					IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER :
					SKILL_PROJ_WEAPONS, number(0, NUM_WEARS - 1));
			}
			//
			// hit
			//

			else if (!dead) {
				my_return_flags =
					damage(ch, vict, dam,
					IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER :
					SKILL_PROJ_WEAPONS, number(0, NUM_WEARS - 1));
			}
		}
		//
		// vict is dead, blast the corpse
		//

		else {
			if (ch->in_room->contents && IS_CORPSE(ch->in_room->contents) &&
				CORPSE_KILLER(ch->in_room->contents) ==
				(IS_NPC(ch) ? -GET_MOB_VNUM(ch) : GET_IDNUM(ch))) {
				if (IS_ARROW(gun)) {
					act("You shoot $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_CHAR);
					act("$n shoots $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				} else {
					act("You blast $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_CHAR);
					act("$n blasts $p with $P!!",
						FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				}
				if (damage_eq(ch, ch->in_room->contents, dam))
					break;
			} else {
				act("You fire off a round from $P.",
					FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
				act("$n fires off a round from $P.",
					FALSE, ch, ch->in_room->contents, gun, TO_ROOM);
			}
		}

		//
		// if the attacker was somehow killed, return immediately
		//

		if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
			return 1;
		}

		if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
			dead = true;
			return DAM_VICT_KILLED;
		}

	}
	return 1;
}
