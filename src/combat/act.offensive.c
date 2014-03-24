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

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "libpq-fe.h"
#include "db.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "bomb.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "strutil.h"
#include "actions.h"
#include "guns.h"
#include "weather.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;

struct follow_type *order_next_k;

/* extern functions */
int find_door(struct creature *ch, char *type, char *dir, const char *cmdname);

int
check_mob_reaction(struct creature *ch, struct creature *vict)
{
    int num = 0;

    if (!IS_NPC(vict))
        return 0;

    num = (GET_ALIGNMENT(vict) - GET_ALIGNMENT(ch)) / 20;
    num = MAX(num, (-num));
    num -= GET_CHA(ch);

    if (num > number(0, 101)) {
        act("$n gets pissed off!", false, vict, NULL, NULL, TO_ROOM);
        return 1;
    } else
        return 0;
}

static inline int
RAW_EQ_DAM(struct creature *ch, int pos, int *var)
{

    if (ch->equipment[pos]) {
        if (IS_OBJ_TYPE(ch->equipment[pos], ITEM_ARMOR))
            *var += (GET_OBJ_WEIGHT(ch->equipment[pos]) +
                GET_OBJ_VAL(ch->equipment[pos], 0)) / 4;
        else if (IS_OBJ_TYPE(ch->equipment[pos], ITEM_WEAPON))
            *var += dice(GET_OBJ_VAL(ch->equipment[pos], 1),
                GET_OBJ_VAL(ch->equipment[pos], 2));
    } else if (ch->implants[pos]) {
        if (IS_OBJ_TYPE(ch->implants[pos], ITEM_ARMOR))
            *var += (GET_OBJ_WEIGHT(ch->implants[pos]) +
                GET_OBJ_VAL(ch->implants[pos], 0)) / 4;
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
calc_skill_prob(struct creature *ch, struct creature *vict, int skillnum,
                int *wait, int *vict_wait, int *move, int *mana, int *dam,
                int *fail_pos, int *vict_pos, struct obj_data **out_weap, int *loc,
                struct affected_type *af)
{

    int prob = 0;
    bool bad_sect = 0, need_hand = 0;
    struct obj_data *neck = NULL;
    struct obj_data *weap = NULL;

    prob = CHECK_SKILL(ch, skillnum);
    if (CHECK_SKILL(ch, skillnum) < LEARNED(ch))
        prob -= (LEARNED(ch) - CHECK_SKILL(ch, skillnum));
    prob += GET_DEX(ch);
    prob += (strength_hit_bonus(GET_STR(ch)) + GET_HITROLL(ch)) / 2;

    prob -= ((IS_WEARING_W(ch) + IS_CARRYING_W(ch)) * 15) / CAN_CARRY_W(ch);

    prob += (MAX(-200, GET_AC(vict)) / 10);

    if (GET_POSITION(vict) < POS_FIGHTING) {
        prob += (POS_FIGHTING - GET_POSITION(vict)) * 6;
        prob -= (GET_DEX(vict) / 2);
    }

    if (IS_DROW(ch)) {
        if (OUTSIDE(ch) && PRIME_MATERIAL_ROOM(ch->in_room)) {
            if (ch->in_room->zone->weather->sunlight == SUN_LIGHT)
                prob -= 25;
            else if (ch->in_room->zone->weather->sunlight == SUN_DARK)
                prob += 10;
        } else if (room_is_dark(ch->in_room))
            prob += 10;
    }

    if (AFF2_FLAGGED(ch, AFF2_DISPLACEMENT) &&
        !AFF2_FLAGGED(vict, AFF2_TRUE_SEEING))
        prob += 5;
    if (AFF_FLAGGED(ch, AFF_BLUR))
        prob += 5;
    if (!can_see_creature(vict, ch))
        prob += 20;

    if (!can_see_creature(ch, vict))
        prob -= 20;
    if (GET_COND(ch, DRUNK))
        prob -= (GET_COND(ch, DRUNK) * 2);
    if (IS_SICK(ch))
        prob -= 5;
    if (AFF2_FLAGGED(vict, AFF2_DISPLACEMENT) &&
        !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        prob -= GET_LEVEL(vict) / 2;
    if (AFF2_FLAGGED(vict, AFF2_EVADE))
        prob -= (GET_LEVEL(vict) / 4) + 5;

    if (IS_BARB(ch)) {
        if (GET_EQ(ch, WEAR_WIELD))
            prob +=
                (LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) / 4;
    }

    if (room_is_watery(ch->in_room) ||
        ch->in_room->sector_type == SECT_ASTRAL ||
        room_is_open_air(ch->in_room))
        bad_sect = true;

    if (bad_sect)
        prob -= (prob * 20) / 100;

    /* individual skill parameters */
    switch (skillnum) {

    case SKILL_BASH:
        prob -= (strength_wield_weight(GET_STR(vict)) / 2);
        prob -= (GET_WEIGHT(vict) - GET_WEIGHT(ch)) / 16;

        if (ch->equipment[WEAR_WIELD])
            prob += GET_OBJ_WEIGHT(ch->equipment[WEAR_WIELD]);
        if (ch->equipment[WEAR_SHIELD])
            prob += GET_OBJ_WEIGHT(ch->equipment[WEAR_SHIELD]);

        if (bad_sect)
            prob = (int)(prob * 0.90);

        if ((NPC_FLAGGED(vict, NPC_NOBASH) ||
                GET_POSITION(vict) < POS_FIGHTING)
            && GET_LEVEL(ch) < LVL_AMBASSADOR)
            prob = 0;

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        *dam = dice(2, (GET_LEVEL(ch) / 4));
        *fail_pos = POS_SITTING;
        *vict_pos = POS_SITTING;
        *vict_wait = 2 RL_SEC;
        *wait = 9 RL_SEC;
        if (IS_BARB(ch)) {
            // reduced wait state for barbs
            int w = 5;
            w *= skill_bonus(ch, SKILL_BASH);
            w /= 100;
            *wait -= w;
            // Improved damage for barbs
            *dam += dice(2, skill_bonus(ch, SKILL_BASH));
        }

        break;
    case SKILL_STRIKE:
        *dam = 0;
        if (GET_EQ(ch, WEAR_WIELD))
            ADD_EQ_DAM(ch, WEAR_WIELD);
        else if (GET_EQ(ch, WEAR_HANDS))
            ADD_EQ_DAM(ch, WEAR_HANDS);
        else {
            send_to_char(ch, "You need a weapon to strike out with!\r\n");
            return -1;
        }
        *dam *= 2;
        *wait = 2 RL_SEC;
        *loc = WEAR_RANDOM;
        *vict_wait = 1 RL_SEC;
        weap = GET_EQ(ch, WEAR_WIELD);
        break;
    case SKILL_HEADBUTT:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *dam = dice(3, (GET_LEVEL(ch) / 8));
        ADD_EQ_DAM(ch, WEAR_HEAD);

        *vict_wait = 1 RL_SEC;
        *wait = 4 RL_SEC;
        *loc = WEAR_HEAD;
        // Barb headbutt should be nastier
        if (IS_BARB(ch) && 0) {
            // Improved damage for barbs
            *dam += skill_bonus(ch, SKILL_HEADBUTT);
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

        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *dam = dice(6, (GET_LEVEL(ch) / 16));
        *wait = 6 RL_SEC;
        *vict_wait = 1 RL_SEC;
        *loc = WEAR_EYES;

        if (IS_NPC(vict) && NPC_FLAGGED(vict, NPC_NOBLIND)) {
            *dam = 0;
            prob = 0;
        } else {
            af->type = SKILL_GOUGE;
            af->duration = 1 + (GET_LEVEL(ch) > 40);
            af->modifier = -10;
            af->location = APPLY_HITROLL;
            af->owner = GET_IDNUM(ch);
            af->aff_index = 1;
            af->bitvector = AFF_BLIND;
        }
        break;

    case SKILL_PILEDRIVE:
        need_hand = 1;

        if (GET_HEIGHT(vict) > (GET_HEIGHT(ch) * 2))
            prob -= (100 - GET_LEVEL(ch));

        if (GET_POSITION(vict) < POS_STANDING)
            prob += (10 * (POS_STANDING - GET_POSITION(vict)));

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        if ((GET_WEIGHT(vict) + ((IS_CARRYING_W(vict) +
                        IS_WEARING_W(vict)) / 2)) > CAN_CARRY_W(ch) * 1.5) {
            act("$N is too heavy for you to lift!", false, ch, NULL, vict,
                TO_CHAR);
            act("$n tries to pick you up and piledrive you!", false, ch, NULL,
                vict, TO_VICT);
            act("$n tries to pick $N up and piledrive $M!", false, ch, NULL, vict,
                TO_NOTVICT);
            prob = 0;
            if (check_mob_reaction(ch, vict)) {
                hit(vict, ch, TYPE_UNDEFINED);
                return -1;
            }
        }

        *move = 20;
        *dam = dice(GET_LEVEL(ch), GET_STR(ch));
        if (!IS_BARB(ch) && GET_LEVEL(ch) < LVL_IMMORT)
            *dam = *dam / 4;
        ADD_EQ_DAM(ch, WEAR_CROTCH);
        if (!NPC_FLAGGED(vict, NPC_NOBASH))
            *vict_pos = POS_SITTING;
        *fail_pos = POS_SITTING;
        *loc = WEAR_HEAD;
        *wait = 8 RL_SEC;
        *vict_wait = 3 RL_SEC;
        break;

    case SKILL_BODYSLAM:

        if ((GET_WEIGHT(vict) + ((IS_CARRYING_W(vict) +
                        IS_WEARING_W(vict)) / 2)) > CAN_CARRY_W(ch) * 1.5) {
            act("$N is too heavy for you to lift!", false, ch, NULL, vict,
                TO_CHAR);
            act("$n tries to pick you up and bodyslam you!", false, ch, NULL,
                vict, TO_VICT);
            act("$n tries to pick $N up and bodyslam $M!", false, ch, NULL, vict,
                TO_NOTVICT);
            if (check_mob_reaction(ch, vict)) {
                hit(vict, ch, TYPE_UNDEFINED);
            }
            return -1;
        }

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        need_hand = 1;
        *dam = dice(10, GET_LEVEL(ch) / 8);
        *vict_pos = POS_SITTING;
        *wait = 7 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *move = 15;
        break;

    case SKILL_KICK:

        *dam = dice(2, (GET_LEVEL(ch) / 4));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 3 RL_SEC;
        break;

    case SKILL_SPINKICK:

        *dam = dice(3, (GET_LEVEL(ch) / 4));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 4 RL_SEC;
        break;

    case SKILL_ROUNDHOUSE:

        *dam = dice(4, (GET_LEVEL(ch) / 4));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 5 RL_SEC;
        break;

    case SKILL_PELE_KICK:

        *dam = dice(7, GET_LEVEL(ch));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 6 RL_SEC;
        *fail_pos = POS_SITTING;
        break;

    case SKILL_SIDEKICK:
        if ((NPC_FLAGGED(vict, NPC_NOBASH) ||
                GET_POSITION(vict) < POS_FIGHTING)
            && GET_LEVEL(ch) < LVL_AMBASSADOR)
            prob = 0;

        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *dam = dice(5, (GET_LEVEL(ch) / 4));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 5 RL_SEC;
        *fail_pos = POS_SITTING;
        *vict_pos = POS_SITTING;
        *vict_wait = 2 RL_SEC;
        break;

    case SKILL_GROINKICK:

        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *dam = dice(3, (GET_LEVEL(ch) / 4));
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 5 RL_SEC;
        break;

    case SKILL_PUNCH:

        need_hand = 1;
        prob += GET_DEX(ch);
        if (GET_LEVEL(ch) >= 67) {
            *dam = 1000;
        } else {
            *dam = dice(1, GET_LEVEL(ch) / 8);
            ADD_EQ_DAM(ch, WEAR_HANDS);
        }
        *wait = 3 RL_SEC;
        break;

    case SKILL_SPINFIST:
    case SKILL_CLAW:

        need_hand = 1;
        *dam = dice(2, GET_LEVEL(ch) / 8);
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 3 RL_SEC;
        break;

    case SKILL_JAB:
    case SKILL_RABBITPUNCH:

        need_hand = 1;
        *dam = dice(1, GET_LEVEL(ch) / 8) + 1;
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 3 RL_SEC;
        break;

    case SKILL_HOOK:

        need_hand = 1;
        *dam = dice(2, GET_LEVEL(ch) / 8) + 1;
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 3 RL_SEC;
        break;

    case SKILL_UPPERCUT:

        need_hand = 1;
        *dam = dice(3, GET_LEVEL(ch) / 8) + 10;
        if (IS_RANGER(ch))
            *dam *= 2;
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 4 RL_SEC;
        *loc = WEAR_FACE;
        break;

    case SKILL_LUNGE_PUNCH:

        if (bad_sect)
            prob = (int)(prob * 0.90);

        if ((NPC_FLAGGED(vict, NPC_NOBASH) ||
                GET_POSITION(vict) < POS_FIGHTING)
            && GET_LEVEL(ch) < LVL_AMBASSADOR)
            prob = 0;

        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *dam = dice(8, GET_LEVEL(ch) / 8) + 1;
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 4 RL_SEC;
        *vict_pos = POS_SITTING;
        *vict_wait = 2 RL_SEC;
        *loc = WEAR_FACE;
        break;

    case SKILL_ELBOW:

        *dam = dice(2, GET_LEVEL(ch) / 8);
        ADD_EQ_DAM(ch, WEAR_ARMS);
        *wait = 4 RL_SEC;
        break;

    case SKILL_KNEE:

        *dam = dice(3, GET_LEVEL(ch) / 8);
        ADD_EQ_DAM(ch, WEAR_LEGS);
        *wait = 5 RL_SEC;
        break;

    case SKILL_STOMP:

        *loc = WEAR_FEET;
        *dam = dice(5, GET_LEVEL(ch) / 4);
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 4 RL_SEC;
        *vict_wait = 1 RL_SEC;
        break;

    case SKILL_CLOTHESLINE:

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        *loc = WEAR_NECK_1;
        *dam = dice(2, GET_LEVEL(ch) / 8);
        ADD_EQ_DAM(ch, WEAR_ARMS);
        *wait = 4 RL_SEC;
        *vict_pos = POS_SITTING;
        break;

    case SKILL_SWEEPKICK:
    case SKILL_TRIP:

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        *dam = dice(2, GET_LEVEL(ch) / 8);
        *wait = 5 RL_SEC;
        *vict_pos = POS_SITTING;
        *vict_wait = 2 RL_SEC;
        break;

    case SKILL_SHIELD_SLAM:

        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        if (GET_EQ(ch, WEAR_SHIELD)) {

           *dam = dice(3, GET_OBJ_WEIGHT(ch->equipment[WEAR_SHIELD]));
           *dam += (*dam * GET_REMORT_GEN(ch)) / 4;

           *wait = 6 RL_SEC;
           *vict_pos = POS_SITTING;
           *vict_wait = 2 RL_SEC;
           *move = 10;
        } else {
           send_to_char(ch, "You need a shield equipped to shield slam.\r\n");
           return -1;
        }
        break;


    case SKILL_BEARHUG:

        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *loc = WEAR_BODY;
        *dam = dice(4, GET_LEVEL(ch) / 8);
        *wait = 6 RL_SEC;
        *vict_wait = 3 RL_SEC;
        break;

    case SKILL_BITE:

        *dam = dice(2, GET_LEVEL(ch) / 8);
        *wait = 4 RL_SEC;
        break;

    case SKILL_CHOKE:
        if (AFF2_FLAGGED(vict, AFF2_NECK_PROTECTED) &&
            number(0, GET_LEVEL(vict) * 4) > number(0, GET_LEVEL(ch))) {
            if ((neck = GET_EQ(vict, WEAR_NECK_1)) ||
                (neck = GET_EQ(vict, WEAR_NECK_2))) {
                act("$n attempts to choke you, but $p has you covered!",
                    false, ch, neck, vict, TO_VICT);
                act("$n attempts to choke $N, but $p has $M covered!",
                    false, ch, neck, vict, TO_NOTVICT);
                act("You attempt to choke $N, but $p has $M covered!",
                    false, ch, neck, vict, TO_CHAR);
                return -1;
            }
        }

        need_hand = 1;
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;
        *loc = WEAR_NECK_1;
        *dam = dice(3, GET_LEVEL(ch) / 8);
        *wait = 4 RL_SEC;
        *vict_wait = 2 RL_SEC;
        break;

    case SKILL_BEHEAD:

        *loc = WEAR_NECK_1;
        *move = 20;
        weap = GET_EQ(ch, WEAR_WIELD);

        prob -= 20;

        if ((!weap || !is_slashing_weapon(weap)) &&
            (!(weap = GET_EQ(ch, WEAR_HANDS)) || !is_slashing_weapon(weap))) {
            send_to_char(ch,
                "You need to wield a good slashing weapon to do this.\r\n");
            return -1;
        }

        *dam = (dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2)) * 6);
        *dam += dice((int)(GET_LEVEL(ch) * 0.75), 5);
        if (IS_OBJ_STAT2(weap, ITEM2_TWO_HANDED)
            && weap->worn_on == WEAR_WIELD)
            *dam *= 2;
        else
            *dam = (int)(*dam * 0.80);

        if (isname("headless", vict->player.name)) {
            act("$N doesn't have a head!", false, ch, NULL, vict, TO_CHAR);
            return -1;
        }

        if ((GET_HEIGHT(ch) < GET_HEIGHT(vict) / 2) &&
            GET_POSITION(vict) > POS_SITTING) {
            if (AFF_FLAGGED(ch, AFF_INFLIGHT))
                *dam /= 4;
            else {
                act("$N is over twice your height!  You can't reach $S head!",
                    false, ch, NULL, vict, TO_CHAR);
                return -1;
            }
        }

        if (AFF2_FLAGGED(vict, AFF2_NECK_PROTECTED)
            && number(0, GET_LEVEL(vict) * 4) > number(0,
                (GET_LEVEL(ch) / 2))) {
            // Try to find the nobehead eq.
            neck = GET_EQ(vict, WEAR_NECK_1);
            if (neck == NULL || !NOBEHEAD_EQ(neck)) {
                neck = GET_EQ(vict, WEAR_NECK_2);
            }
            if (neck == NULL || !NOBEHEAD_EQ(neck)) {
                neck = GET_IMPLANT(vict, WEAR_NECK_1);
            }
            if (neck == NULL || !NOBEHEAD_EQ(neck)) {
                neck = GET_IMPLANT(vict, WEAR_NECK_2);
            }

            if (neck != NULL) {
                // half the damage only if the eq survives.
                // ( damage_eq returns the mangled object, not the original )
                if (damage_eq(ch, neck, *dam, TYPE_HIT) != NULL)
                    *dam /= 2;
            }
        }

        if (IS_PUDDING(vict) || IS_SLIME(vict) || IS_RACE(vict, RACE_BEHOLDER))
            prob = 0;

        *wait = 6 RL_SEC;
        break;

    case SKILL_SHOOT:
        prob += (GET_DEX(ch) / 2);
        prob -= strength_hit_bonus(GET_STR(ch));

        break;

    case SKILL_ARCHERY:
        prob += (GET_DEX(ch) / 2);
        prob -= strength_hit_bonus(GET_STR(ch));

        break;

        /** monk skillz here **/
    case SKILL_PALM_STRIKE:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *loc = WEAR_BODY;
        *dam = dice(GET_LEVEL(ch), GET_STR(ch) / 4);
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 3 RL_SEC;
        *vict_wait = 1 RL_SEC;
        break;

    case SKILL_THROAT_STRIKE:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *loc = WEAR_NECK_1;
        *dam = dice(GET_LEVEL(ch), 5);
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 4 RL_SEC;
        *vict_wait = 2 RL_SEC;
        break;

    case SKILL_CRANE_KICK:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *loc = WEAR_HEAD;
        *dam = dice(GET_LEVEL(ch), 7);
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 6 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *fail_pos = POS_SITTING;
        break;

    case SKILL_SCISSOR_KICK:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        *dam = dice(GET_LEVEL(ch), 9);
        ADD_EQ_DAM(ch, WEAR_FEET);
        *wait = 6 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *fail_pos = POS_SITTING;
        *move = 10;
        break;

    case SKILL_RIDGEHAND:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *loc = WEAR_NECK_1;
        *dam = dice(GET_LEVEL(ch), 9);
        ADD_EQ_DAM(ch, WEAR_HANDS);
        *wait = 4 RL_SEC;
        *vict_wait = 1 RL_SEC;
        *move = 10;
        break;

    case SKILL_DEATH_TOUCH:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;

        need_hand = 1;
        *loc = WEAR_NECK_1;
        *dam = dice(GET_LEVEL(ch), 13);
        *wait = 7 RL_SEC;
        *vict_wait = (2 + number(0, GET_LEVEL(ch) / 8)) RL_SEC;
        *move = 35;
        break;

    case SKILL_HIP_TOSS:
        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
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
        if ((!affected_by_spell(ch, SKILL_KATA) &&
                (IS_PUDDING(vict) || IS_SLIME(vict)
                    || NON_CORPOREAL_MOB(vict)))
            || NPC_FLAGGED(vict, NPC_NOBASH) || bad_sect)
            prob = 0;

        need_hand = 1;
        *dam = dice(3, (GET_LEVEL(ch) / 4) + GET_STR(ch));
        *wait = 6 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *vict_pos = POS_RESTING;
        *move = 15;
        break;

        /** psionic skillz **/
    case SKILL_PSIBLAST:

        if (NULL_PSI(vict))
            prob = 0;
        if (GET_LEVEL(ch) < 51 && !IS_PSIONIC(ch))
            prob = 0;

        *dam = dice(skill_bonus(ch, SKILL_PSIBLAST) / 2, GET_INT(ch) + 1);

        *dam += CHECK_SKILL(ch, SKILL_PSIBLAST);

        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_PSI))
            *dam /= 2;
        *wait = 5 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *mana = mag_manacost(ch, SKILL_PSIBLAST);
        *move = 10;
        break;

    case SKILL_SCREAM:
        if (IS_UNDEAD(vict))
            prob = 0;
        if (GET_LEVEL(ch) < 51 && !IS_BARD(ch))
            prob = 0;

        *dam = GET_CON(ch) * ((10 + (skill_bonus(ch, SKILL_SCREAM)) / 4));
        *dam += dice(15, (CHECK_SKILL(ch, SKILL_SCREAM) / 4));

        //fortissimo makes scream more powerful
        struct affected_type *fortissimoAff;
        if ((fortissimoAff = affected_by_spell(ch, SONG_FORTISSIMO))) {
            // This should be +175 damage at gen 10/49
            *dam += skill_bonus(ch, SONG_FORTISSIMO) + fortissimoAff->level;
        }

        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_BREATH))
            *dam /= 2;

        *wait = 5 RL_SEC;
        *vict_wait = 2 RL_SEC;
        *mana = mag_manacost(ch, SKILL_SCREAM);
        *move = 25;
        break;

    case SKILL_GARROTE:
        if (!affected_by_spell(ch, SKILL_KATA) &&
            (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
            )
            prob = 0;
        if (!AFF_FLAGGED(ch, AFF_SNEAK) && !AFF3_FLAGGED(ch, AFF3_INFILTRATE))
            prob = 0;
        if (GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_HOLD) ||
            GET_EQ(ch, WEAR_SHIELD)) {
            send_to_char(ch, "You need both hands free to do that!\r\n");
            return -1;
        }

        *loc = WEAR_NECK_1;
        *dam = dice(GET_LEVEL(ch), 5);
        *wait = 4 RL_SEC;
        *vict_wait = 4 RL_SEC;
        break;

    default:
        errlog("Illegal skillnum <%d> passed to calc_skill_prob().", skillnum);
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

    *dam += strength_damage_bonus(GET_STR(ch));
    *dam += GET_DAMROLL(ch);

    *dam += (*dam * GET_REMORT_GEN(ch)) / 10;

    int skill_lvl = LEARNED(ch);

    if (CHECK_SKILL(ch, skillnum) > skill_lvl)
        *dam *= (LEARNED(ch) +
            ((CHECK_SKILL(ch, skillnum) - skill_lvl) / 2));
    else
        *dam *= CHECK_SKILL(ch, skillnum);
    *dam = (skill_lvl > 0) ? (*dam / skill_lvl):0;

    if (!affected_by_spell(ch, SKILL_KATA) && NON_CORPOREAL_MOB(vict))
        *dam = 0;

    prob = MIN(MAX(0, prob), 110);

    *out_weap = weap;

    return (prob);
}

bool
perform_offensive_skill(struct creature *ch,
                        struct creature *vict,
                        int skill)
{
    struct affected_type af;
    struct obj_data *weap = NULL;
    int prob = -1, wait = 0, vict_wait = 0, dam = 0, vict_pos = 0, fail_pos =
        0, loc = -1, move = 0, mana = 0;

    init_affect(&af);

    if (!ok_to_attack(ch, vict, false)) {
        send_to_char(ch, "You seem to be unable to attack %s.\r\n",
            GET_NAME(vict));
        act("$n shakes with rage as $e tries to attack $N!",
            false, ch, NULL, vict, TO_NOTVICT);
        act("$n shakes with rage as $e tries to attack you!",
            false, ch, NULL, vict, TO_VICT);
        return false;
    }

    if (SPELL_IS_PSIONIC(skill) && ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)
        && GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "Psychic powers are useless here!\r\n");
        return false;
    }
    if ((skill == SKILL_SWEEPKICK || skill == SKILL_TRIP) &&
        GET_POSITION(vict) <= POS_SITTING) {
        act("$N is already on the ground.", false, ch, NULL, vict, TO_CHAR);
        return false;
    }

    af.owner = GET_IDNUM(ch);
    //
    // calc_skill_prob returns -1 if you cannot perform that skill,
    // or if somethine exceptional happened and we need to return
    //
    prob = calc_skill_prob(ch, vict, skill,
                           &wait, &vict_wait, &move, &mana,
                           &dam, &fail_pos, &vict_pos, &weap, &loc, &af);
    if (prob < 0) {
        return false;
    }

    if (IS_MONK(ch) && !IS_NEUTRAL(ch) && GET_LEVEL(ch) < LVL_GOD) {
        prob -= (prob * (ABS(GET_ALIGNMENT(ch)))) / 1000;
        dam -= (dam * (ABS(GET_ALIGNMENT(ch)))) / 2000;
    }
    //
    // skill failure
    //

    if (prob < number(1, 120)) {
        if (!damage(ch, vict, weap, 0, skill, loc))
            return false;
        if (fail_pos) {
            GET_POSITION(ch) = fail_pos;
            if (prob < 50) {
                // 0.1 sec for every point below 50, up to 7 sec
                int tmp_wait = 50 - prob;
                tmp_wait = MAX(tmp_wait, 70);
                wait += tmp_wait;
            }
        }

        if (move)
            GET_MOVE(ch) -= (move / 2);
        if (mana)
            GET_MANA(ch) -= (mana / 2);

        WAIT_STATE(ch, (wait / 2));

        return false;
    }
    //
    // skill success
    //
    if (move)
        GET_MOVE(ch) -= move;
    if (mana)
        GET_MANA(ch) -= mana;

    gain_skill_prof(ch, skill);
    WAIT_STATE(ch, wait);

    // On success, we always do at least one point of damage
    if (dam < 1)
        dam = 1;
    if (!damage(ch, vict, weap, dam, skill, loc))
        return false;

    //
    // set waits, position, and affects on victim if they are still alive
    //

    if (!is_dead(ch) && !is_dead(vict)) {
        if (vict_pos)
            GET_POSITION(vict) = vict_pos;
        if (vict_wait)
            WAIT_STATE(vict, vict_wait);

        if (af.type && !affected_by_spell(vict, af.type))
            affect_to_char(vict, &af);

        return true;
    }

    return false;
}

ACMD(do_offensive_skill)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    struct affected_type af;
    char *arg;

    init_affect(&af);

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act(tmp_sprintf("You fiercely %s $p!", CMD_NAME),
                false, ch, ovict, NULL, TO_CHAR);
            act(tmp_sprintf("$n fiercely %s%ss $p!", CMD_NAME,
                    (CMD_NAME[strlen(CMD_NAME) - 1] == 's') ? "e" : ""),
                false, ch, ovict, NULL, TO_ROOM);
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

    perform_offensive_skill(ch, vict, subcmd);
}

ACMD(do_assist)
{
    struct creature *helpee;
    char *arg;

    if (is_fighting(ch)) {
        send_to_char(ch,
            "You're already fighting!  How can you assist someone else?\r\n");
        return;
    }
    arg = tmp_getword(&argument);

    if (!*arg)
        send_to_char(ch, "Whom do you wish to assist?\r\n");
    else if (!(helpee = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "%s", NOPERSON);
        WAIT_STATE(ch, 4);
    } else if (helpee == ch)
        send_to_char(ch, "You can't help yourself any more than this!\r\n");
    else if (!is_fighting(helpee))
        act("But nobody is fighting $M!", false, ch, NULL, helpee, TO_CHAR);
    else {
        struct creature *opponent = random_opponent(helpee);
        if (!can_see_creature(ch, (opponent))) {
            act("You can't see who is fighting $M!", false, ch, NULL, helpee,
                TO_CHAR);
        } else if (!IS_NPC(ch) && !IS_NPC((opponent))
            && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
            act("That rescue would entail attacking $N, but you are "
                "flagged NO PK.", false, ch, NULL, (opponent), TO_CHAR);
            return;
        } else {
            send_to_char(ch, "You join the fight!\r\n");
            act("$N assists you!", 0, helpee, NULL, ch, TO_CHAR);
            act("$n assists $N.", false, ch, NULL, helpee, TO_NOTVICT);
            hit(ch, (opponent), TYPE_UNDEFINED);
            WAIT_STATE(ch, 1 RL_SEC);
        }
    }
}

ACMD(do_hit)
{
    struct creature *vict;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg)
        send_to_char(ch, "Hit who?\r\n");
    else if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "They don't seem to be here.\r\n");
        WAIT_STATE(ch, 4);
    } else if (vict == ch) {
        send_to_char(ch, "You hit yourself...OUCH!\r\n");
        act("$n hits $mself, and says OUCH!", false, ch, NULL, vict, TO_ROOM);
    } else if (g_list_find(ch->fighting, vict)) {
        act("Okay, you will now concentrate your attacks on $N!",
            0, ch, NULL, vict, TO_CHAR);
        ch->fighting = g_list_prepend(g_list_remove(ch->fighting, vict), vict);
    } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
        act("$N is just such a good friend, you simply can't hit $M.", false,
            ch, NULL, vict, TO_CHAR);
    else {
        if (!ok_to_attack(ch, vict, true))
            return;

        GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 5);
        hit(ch, vict, TYPE_UNDEFINED);
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_kill)
{
    struct creature *vict;
    char *arg;

    if ((GET_LEVEL(ch) < LVL_CREATOR) || subcmd != SCMD_SLAY || IS_NPC(ch)) {
        do_hit(ch, argument, cmd, subcmd);
        return;
    }

    arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Kill who?\r\n");
    } else {
        if (!(vict = get_char_room_vis(ch, arg))) {
            send_to_char(ch, "They aren't here.\r\n");
            WAIT_STATE(ch, 4);
        } else if (ch == vict)
            send_to_char(ch, "Your mother would be so sad.. :(\r\n");
        else if (GET_LEVEL(vict) >= GET_LEVEL(ch)) {
            act("That's a really bad idea.", false, ch, NULL, NULL, TO_CHAR);
        } else {
            act("You chop $M to pieces!  Ah!  The blood!", false, ch, NULL, vict,
                TO_CHAR);
            act("$N chops you to pieces!", false, vict, NULL, ch, TO_CHAR);
            act("$n brutally slays $N!", false, ch, NULL, vict, TO_NOTVICT);
            mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(vict)), NRM, true,
                "%s killed %s with a wiz-slay at %s.",
                GET_NAME(ch), GET_NAME(vict), ch->in_room->name);

            raw_kill(vict, ch, TYPE_SLASH); // Wiz-slay
        }
    }
}

ACMD(do_order)
{
    bool detect_opponent_master(struct creature *ch, struct creature *opp);

    const char *name, *message;
    bool found = false;
    struct room_data *org_room;
    struct creature *vict;
    struct follow_type *k = NULL;

    name = tmp_getword(&argument);
    message = argument;

    if (!*name || !*message)
        send_to_char(ch, "Order who to do what?\r\n");
    else if (!(vict = get_char_room_vis(ch, name)) &&
        !is_abbrev(name, "followers"))
        send_to_char(ch, "That person isn't here.\r\n");
    else if (ch == vict)
        send_to_char(ch, "You obviously suffer from schizophrenia.\r\n");
    else {
        if (AFF_FLAGGED(ch, AFF_CHARM)) {
            send_to_char(ch,
                "Your superior would not approve of you giving orders.\r\n");
            return;
        }
        if (vict) {
            act(tmp_sprintf("%s%s$N orders you to '%s'.%s",
                    CCBLD(vict, C_SPR),
                    CCRED(vict, C_NRM),
                    message, CCNRM(vict, C_SPR)), false, vict, NULL, ch, TO_CHAR);
            act("$n gives $N an order.", false, ch, NULL, vict, TO_NOTVICT);
            send_to_char(ch, "%s%sYou order %s to '%s'.%s\r\n",
                CCBLD(ch, C_SPR),
                CCRED(ch, C_NRM), PERS(vict, ch), message, CCNRM(ch, C_SPR));

            if (((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM) ||
                    GET_CHA(ch) < number(0, GET_INT(vict))) &&
                (GET_LEVEL(ch) < LVL_CREATOR ||
                    GET_LEVEL(vict) >= GET_LEVEL(ch))
                && (!NPC2_FLAGGED(vict, NPC2_FAMILIAR) || vict->master != ch))
                act("$n has an indifferent look.", false, vict, NULL, NULL, TO_ROOM);
            else {
                if (!CHECK_WAIT(vict) && !GET_NPC_WAIT(vict)) {
                    if (IS_NPC(vict) && GET_NPC_VNUM(vict) == 5318)
                        perform_say(vict, "intone", "As you command, master.");
                    if (is_fighting(vict)) {
                        for (GList * cit = first_living(vict->fighting); cit;
                            cit = next_living(cit)) {
                            struct creature *tch = cit->data;
                            detect_opponent_master(tch, vict);
                        }
                    }
                    command_interpreter(vict, message);
                }

            }
        } else {                /* This is order "followers" */
            act(tmp_sprintf("$n issues the order '%s'.", message),
                false, ch, NULL, vict, TO_ROOM);

            send_to_char(ch, "%s%sYou order your followers to '%s'.%s\r\n",
                CCBLD(ch, C_SPR), CCRED(ch, C_NRM), message, CCNRM(ch, C_SPR));

            org_room = ch->in_room;

            for (k = ch->followers; k; k = order_next_k) {
                order_next_k = k->next;
                if (org_room == k->follower->in_room) {
                    if ((AFF_FLAGGED(k->follower, AFF_CHARM) &&
                            GET_CHA(ch) > number(0, GET_INT(k->follower)))
                        || GET_LEVEL(ch) >= LVL_CREATOR
                        || NPC2_FLAGGED(k->follower, NPC2_FAMILIAR)) {
                        found = true;
                        if (!CHECK_WAIT(k->follower)
                            && !GET_NPC_WAIT(k->follower)) {
                            if (IS_NPC(k->follower)
                                && GET_NPC_VNUM(k->follower) == 5318)
                                perform_say(vict, "intone",
                                    "As you command, master.");
                            if (is_fighting(k->follower)) {
                                for (GList * cit = first_living(k->follower->fighting);
                                    cit; cit = next_living(cit)) {
                                    struct creature *tch = cit->data;
                                    detect_opponent_master(tch, k->follower);
                                }
                            }
                            command_interpreter(k->follower, message);
                        }
                    } else
                        act("$n has an indifferent look.", true, k->follower,
                            NULL, NULL, TO_CHAR);
                }
            }
            order_next_k = NULL;

            if (!found)
                send_to_char(ch,
                    "Nobody here is a loyal subject of yours!\r\n");
        }
    }
}

ACMD(do_flee)
{
    int i, attempt, loss = 0;
    struct creature *fighting = random_opponent(ch);

    if (AFF2_FLAGGED(ch, AFF2_BERSERK) && is_fighting(ch) &&
        !number(0, 1 + (GET_INT(ch) / 4))) {
        send_to_char(ch, "You are too enraged to flee!\r\n");
        return;
    }
    if (GET_POSITION(ch) < POS_FIGHTING) {
        send_to_char(ch, "You can't flee until you get on your feet!\r\n");
        return;
    }
    if (!IS_NPC(ch) && fighting) {
        loss = GET_LEVEL(random_opponent(ch)) * 32;
        loss += (loss * GET_LEVEL(ch)) / (LVL_GRIMP + 1 - GET_LEVEL(ch));
        loss /= 32;

        if (IS_REMORT(ch))
            loss -= loss * GET_REMORT_GEN(ch) / (GET_REMORT_GEN(ch) + 2);

        if (IS_THIEF(ch))
            loss /= 2;
    }
    for (i = 0; i < 6; i++) {
        attempt = number(0, NUM_OF_DIRS - 1);   /* Select a random direction */
        if (CAN_GO(ch, attempt) && !NOFLEE(EXIT(ch, attempt)->to_room) &&
            (!IS_NPC(ch) || !ROOM_FLAGGED(ch->in_room, ROOM_NOMOB)) &&
            !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room),
                ROOM_DEATH | ROOM_GODROOM)) {
            act("$n panics, and attempts to flee!", true, ch, NULL, NULL, TO_ROOM);
            if (room_is_open_air(ch->in_room)
                || room_is_open_air(EXIT(ch, attempt)->to_room)) {
                if (AFF_FLAGGED(ch, AFF_INFLIGHT))
                    GET_POSITION(ch) = POS_FLYING;
                else
                    continue;
            }

            int move_result = do_simple_move(ch, attempt, MOVE_FLEE, true);

            //
            // return of 2 indicates critical failure of do_simple_move
            //

            if (move_result == 2)
                return;

            if (move_result == 0) {
                send_to_char(ch, "You flee head over heels.\r\n");
                if (loss && fighting) {
                    gain_exp(ch, -loss);
                    gain_exp(fighting, (loss / 32));
                }
                if (is_fighting(ch)) {
                    remove_all_combat(ch);
                }
                if (room_is_open_air(ch->in_room))
                    GET_POSITION(ch) = POS_FLYING;
            } else if (move_result == 1) {
                act("$n tries to flee, but can't!", true, ch, NULL, NULL, TO_ROOM);
            }
            return;
        }
    }
    send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}

static inline int
FLEE_SPEED(struct creature *ch)
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

ACMD(do_retreat)
{
    int dir;
    bool fighting = false, found = false;

    skip_spaces(&argument);
    if (GET_MOVE(ch) < 10) {
        send_to_char(ch,
            "You are too exhausted to make an effective retreat.\r\n");
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

    if (is_fighting(ch)) {
        fighting = true;
        if (CHECK_SKILL(ch, SKILL_RETREAT) + GET_LEVEL(ch) <
            number(60, 70 + GET_LEVEL(random_opponent(ch)))) {
            send_to_char(ch, "You panic!\r\n");
            do_flee(ch, tmp_strdup(""), 0, 0);
            return;
        }
    }
    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *vict = it->data;

        if (vict != ch
            && g_list_find(ch->fighting, vict)
            && can_see_creature(vict, ch)
            && ((IS_NPC(vict) && GET_NPC_WAIT(vict) < 10)
                || (vict->desc && vict->desc->wait < 10))
            && number(0, FLEE_SPEED(ch)) < number(0, FLEE_SPEED(vict))) {
            found = true;
            hit(vict, ch, TYPE_UNDEFINED);
            if (is_dead(ch))
                return;
        }
    }

    int retval = perform_move(ch, dir, MOVE_RETREAT, true);

    if (is_dead(ch))
        return;

    if (retval == 0) {
        if (fighting && !found)
            gain_skill_prof(ch, SKILL_RETREAT);
        if (is_fighting(ch))
            remove_all_combat(ch);
        if (room_is_open_air(ch->in_room))
            GET_POSITION(ch) = POS_FLYING;
        GET_MOVE(ch) = (MAX(0, GET_MOVE(ch) - 10));
        return;
    } else if (retval == 1) {
        send_to_char(ch, "You try to retreat, but you can't!\r\n");
        act("$n attempts to retreat, but fails!", true, ch, NULL, NULL, TO_ROOM);
    }
    // critical failure, possible ch death
    else if (retval == 2) {
        return;
    }
}

#undef FLEE_SPEED

void
bash_door(struct creature *ch, int door)
{
    const char *door_str;

    // We know they're looking for a door at this point
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "You cannot bash that!\r\n");
        return;
    }

    if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        send_to_char(ch, "It's already open!\r\n");
        return;
    }

    if (GET_MOVE(ch) < 20) {
        send_to_char(ch, "You are too exhausted.\r\n");
        return;
    }
    GET_MOVE(ch) -= 20;

    int door_damage = dice(2, GET_LEVEL(ch) / 4);
    door_damage += strength_damage_bonus(GET_STR(ch)) * 4;
    if (CHECK_SKILL(ch, SKILL_BREAK_DOOR) < number(1,99)) {
        door_damage *= 2;
    }

    if (door_damage < 0 ||
        IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF) ||
        EXIT(ch, door)->damage < 0 ||
        EXIT(ch, door)->to_room == NULL)
        door_damage = 0;

    door_damage = MIN(door_damage, EXIT(ch, door)->damage);

    door_str = EXIT(ch, door)->keyword ?
        fname(EXIT(ch, door)->keyword) : "door";

    // Damage the door
    EXIT(ch, door)->damage -= door_damage;

    if (PRF2_FLAGGED(ch, PRF2_DEBUG)) {
        send_to_char(ch, "%s[BASH] door_damage: %d, durability left: %d%s\r\n",
                     CCCYN(ch, C_CMP),
                     door_damage,
                     EXIT(ch, door)->damage,
                     CCNRM(ch, C_CMP));
    }
    // Report success
    if (EXIT(ch, door)->damage > 0) {
        GET_HIT(ch) -= dice(4, 8);
        if (GET_HIT(ch) > -10) {
            act(tmp_sprintf
                ("$n throws $mself against the %s in an attempt to break it.",
                    door_str), false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch,
                "You slam yourself against the %s.\r\n",
                EXIT(ch, door)->keyword ? fname(EXIT(ch,
                        door)->keyword) : "door");
            update_pos(ch);
            int percent = EXIT(ch, door)->damage * 100 / EXIT(ch, door)->maxdam;
            const char *damage_desc = "untouched";
            if (percent > 99)
                damage_desc = "untouched";
            else if (percent > 90)
                damage_desc = "virtually unharmed";
            else if (percent > 75)
                damage_desc = "pretty scratched up";
            else if (percent > 50)
                damage_desc = "in poor shape";
            else if (percent > 25)
                damage_desc = "completely battered";
            else if (percent > 5)
                damage_desc = "on the verge of breaking";

            act(tmp_sprintf("The %s looks %s.", door_str, damage_desc),
                false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch, "The %s looks %s.\r\n", door_str, damage_desc);
        } else {
            act(tmp_sprintf
                ("$n throws $mself against the %s, $s last futile effort.",
                    door_str), false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch,
                "You kill yourself as you hurl yourself against the %s!\r\n",
                door_str);
            if (!IS_NPC(ch))
                mudlog(GET_INVIS_LVL(ch), NRM, true,
                    "%s killed self bashing door at %d.",
                    GET_NAME(ch), ch->in_room->number);

            raw_kill(ch, ch, SKILL_BASH);   // Bashing a door to death
        }
    } else {
        // Success
        act(tmp_sprintf("$n bashes the %s open with a powerful blow!",
                door_str), false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "The %s gives way under your powerful bash!\r\n",
            door_str);

        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        GET_HIT(ch) -= dice(4,8);

        if (number(0, 20) > GET_DEX(ch)) {
            act("$n staggers and falls down.", true, ch, NULL, NULL, TO_ROOM);
            act("You stagger and fall down.", true, ch, NULL, NULL, TO_CHAR);
            GET_POSITION(ch) = POS_SITTING;
        }

        if (GET_HIT(ch) < -10) {
            if (!IS_NPC(ch)) {
                mudlog(GET_INVIS_LVL(ch), NRM, true,
                    "%s killed self bashing door at %d.",
                    GET_NAME(ch), ch->in_room->number);
            }
            raw_kill(ch, ch, SKILL_BASH);   // Bash Door to death
            return;
        } else {
            update_pos(ch);
            gain_skill_prof(ch, SKILL_BREAK_DOOR);
        }

        struct room_direction_data *other_side =
            EXIT(ch, door)->to_room->dir_option[rev_dir[door]];
        if (other_side && other_side->to_room == ch->in_room) {
            REMOVE_BIT(other_side->exit_info, EX_CLOSED);
            REMOVE_BIT(other_side->exit_info, EX_LOCKED);
            send_to_room(tmp_sprintf
                ("The %s is bashed open from the other side!!\r\n",
                    other_side->keyword ? fname(other_side->keyword) : "door"),
                EXIT(ch, door)->to_room);
        }
    }
}

ACMD(do_bash)
{
    struct creature *vict = NULL;
    struct obj_data *ovict;
    char *arg1, *arg2;
    struct room_data *room = NULL;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (*arg1)
        vict = get_char_room_vis(ch, arg1);
    else
        vict = random_opponent(ch);

    // If we found our victim, it's a combat move
    if (vict) {
        perform_offensive_skill(ch, vict, SKILL_BASH);
        return;
    }
    // If it's an object in the room, it's just a scary social
    ovict = get_obj_in_list_vis(ch, arg1, ch->in_room->contents);
    if (ovict) {
        act("You bash $p!", false, ch, ovict, NULL, TO_CHAR);
        act("$n bashes $p!", false, ch, ovict, NULL, TO_ROOM);
        if (IS_OBJ_TYPE(ovict, ITEM_VEHICLE) &&
            (room = real_room(ROOM_NUMBER(ovict))) != NULL && room->people) {
            act("$N bashes the outside of $p!",
                false, room->people->data, ovict, ch, TO_ROOM);
            act("$N bashes the outside of $p!",
                false, room->people->data, ovict, ch, TO_CHAR);
        }
        return;
    }
    // If it's a door, it's a non-combat skill
    int door = find_door(ch, arg1, arg2, "bash");
    if (door < 0) {
        WAIT_STATE(ch, 4);
        return;
    }
    bash_door(ch, door);
}

void
slam_door(struct creature *ch, int door)
{
    const char *door_str;
    int wait_state = 0;

    // We know they're looking for a door at this point
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "You cannot slam that!\r\n");
        return;
    }

    if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        send_to_char(ch, "It's already shut!\r\n");
        return;
    }

    door_str = EXIT(ch, door)->keyword ?
        fname(EXIT(ch, door)->keyword) : "door";
    if (IS_SET(EXIT(ch, door)->exit_info, EX_HEAVY_DOOR)) {
        wait_state = 30; // 3 sec
        act(tmp_sprintf("$n grunts as $e slams the %s shut!",
                        door_str), false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You slam the %s shut with a heavy crash!\r\n",
                     door_str);
    } else {
        wait_state = 15; // 15 sec
        act(tmp_sprintf("$n swiftly slams the %s shut!",
                        door_str), false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You swiftly slam the %s shut with a crash!\r\n",
                     door_str);
    }

    SET_BIT(EXIT(ch,door)->exit_info, EX_CLOSED);

    struct room_direction_data *other_side =
        EXIT(ch, door)->to_room->dir_option[rev_dir[door]];
    if (other_side && other_side->to_room == ch->in_room) {
        SET_BIT(other_side->exit_info, EX_CLOSED);
        send_to_room(tmp_sprintf
                     ("The %s slams shut from the other side!!\r\n",
                      other_side->keyword ? fname(other_side->keyword) : "door"),
                     EXIT(ch, door)->to_room);
    }

    WAIT_STATE(ch, wait_state);
}

ACMD(do_slam)
{
    struct creature *vict = NULL;
    struct obj_data *ovict;
    char *arg1, *arg2;
    struct room_data *room = NULL;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (*arg1)
        vict = get_char_room_vis(ch, arg1);
    else
        vict = random_opponent(ch);

    // If we found our victim, it's a combat move
    if (vict) {
        perform_offensive_skill(ch, vict, SKILL_SHIELD_SLAM);
        return;
    }
    // If it's an object in the room, it's just a scary social
    ovict = get_obj_in_list_vis(ch, arg1, ch->in_room->contents);
    if (ovict) {
        act("You slam $p!", false, ch, ovict, NULL, TO_CHAR);
        act("$n slams $p!", false, ch, ovict, NULL, TO_ROOM);
        if (IS_OBJ_TYPE(ovict, ITEM_VEHICLE) &&
            (room = real_room(ROOM_NUMBER(ovict))) != NULL && room->people) {
            act("$N slams the outside of $p!",
                false, room->people->data, ovict, ch, TO_ROOM);
            act("$N slams the outside of $p!",
                false, room->people->data, ovict, ch, TO_CHAR);
        }
        return;
    }
    // If it's a door, it's a non-combat skill
    int door = find_door(ch, arg1, arg2, "slam");
    if (door < 0) {
        WAIT_STATE(ch, 4);
        return;
    }
    slam_door(ch, door);
}


void
perform_stun(struct creature *ch, struct creature *vict)
{
    int percent, prob, wait;

    if (vict == ch) {
        send_to_char(ch, "Aren't we stunningly funny today...\r\n");
        return;
    }
    if (is_fighting(ch)) {
        send_to_char(ch, "You're pretty busy right now!\r\n");
        return;
    }
    if (is_fighting(vict)) {
        send_to_char(ch, "You aren't able to get the right grip!");
        return;
    }
    if ((GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_HOLD) ||
                GET_EQ(ch, WEAR_WIELD_2))) ||
        (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD)))) {
        send_to_char(ch, "You need at least one hand free to do that!\r\n");
        return;
    }

    if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
        act("$N stuns you with a swift blow!", false, ch, NULL, vict, TO_CHAR);
        act("You stun $n with a swift blow!", false, ch, NULL, vict, TO_VICT);
        act("$N stuns $n with a swift blow to the neck!",
            false, ch, NULL, vict, TO_NOTVICT);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        GET_POSITION(ch) = POS_STUNNED;
        return;
    }

    appear(ch, vict);

    percent = number(1, 111) + GET_LEVEL(vict); /* 101% is a complete failure */
    prob = CHECK_SKILL(ch, SKILL_STUN) + (GET_LEVEL(ch) / 4) +
        (GET_DEX(ch) * 4);

    if (AFF_FLAGGED(vict, AFF_ADRENALINE))
        prob -= GET_LEVEL(vict);

    if (!can_see_creature(vict, ch))
        prob += GET_LEVEL(ch) / 2;
    if (AFF_FLAGGED(ch, AFF_SNEAK))
        prob += (CHECK_SKILL(ch, SKILL_SNEAK)) / 10;
    if (!AWAKE(vict))
        prob += 20;
    else
        prob -= GET_DEX(vict);

    if (!IS_NPC(ch))
        prob += (GET_REMORT_GEN(ch) * 8);

    if (IS_PUDDING(vict) || IS_SLIME(vict) || IS_UNDEAD(vict))
        prob = 0;

    if ((prob < percent || NPC2_FLAGGED(vict, NPC2_NOSTUN)) &&
        (GET_LEVEL(ch) < LVL_AMBASSADOR || GET_LEVEL(ch) < GET_LEVEL(vict))) {
        act("Uh-oh!  You fumbled while trying to stun $N!", false, ch, NULL, vict,
            TO_CHAR);
        act("$n pokes your neck trying to stun you!  Ow!", false, ch, NULL, vict,
            TO_VICT);
        act("$n pokes at $N's neck, but nothing happens!", false, ch, NULL, vict,
            TO_NOTVICT);
        add_combat(ch, vict, true);
        add_combat(vict, ch, false);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    act("You stun $N with a swift blow!", false, ch, NULL, vict, TO_CHAR);
    act("$n strikes a nerve center in your neck!  You are stunned!",
        false, ch, NULL, vict, TO_VICT);
    act("$n stuns $N with a swift blow!", false, ch, NULL, vict, TO_NOTVICT);
    if (is_fighting(vict)) {
        remove_all_combat(vict);
        remove_all_combat(ch);
    }
    if (IS_NPC(vict)) {
        SET_BIT(NPC_FLAGS(vict), NPC_MEMORY);
        remember(vict, ch);
        if (IS_SET(NPC2_FLAGS(vict), NPC2_HUNT))
            start_hunting(vict, ch);
    }
    wait = 1 + (number(0, GET_LEVEL(ch)) / 5);
    GET_POSITION(vict) = POS_STUNNED;
    WAIT_STATE(vict, wait RL_SEC);
    WAIT_STATE(ch, 4 RL_SEC);
    gain_skill_prof(ch, SKILL_STUN);
    check_attack(ch, vict);
}

ACMD(do_stun)
{
    struct creature *vict = NULL;
    char *arg;

    arg = tmp_getword(&argument);

    if (CHECK_SKILL(ch, SKILL_STUN) < 20) {
        send_to_char(ch, "You are not particularly stunning.\r\n");
        return;
    }
    if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "Who would you like to stun?\r\n");
        WAIT_STATE(ch, 4);
        return;
    }
    if (is_fighting(ch)) {
        send_to_char(ch, "You're pretty busy right now!\r\n");
        return;
    }
    if (is_fighting(vict)) {
        send_to_char(ch, "You aren't able to get the right grip!\r\n");
        return;
    }
    if (!ok_to_attack(ch, vict, false)) {
        send_to_char(ch, "You seem to be unable to attack %s.\r\n",
            GET_NAME(vict));
        act("$n shakes with rage as $e tries to attack $N!",
            false, ch, NULL, vict, TO_NOTVICT);
        act("$n shakes with rage as $e tries to attack you!",
            false, ch, NULL, vict, TO_VICT);
        return;
    }

    perform_stun(ch, vict);
}

ACMD(do_feign)
{
    int percent, prob;
    struct creature *foe = NULL;

    percent = number(1, 101);   /* 101% is a complete failure */
    prob = CHECK_SKILL(ch, SKILL_FEIGN);

    if (prob < percent) {
        send_to_char(ch, "You fall over dead!\r\n");
        act("$n staggers and falls to the ground!", true, ch, NULL, NULL, TO_ROOM);
    } else {
        if ((foe = random_opponent(ch))) {
            act("You have killed $N!", false, foe, NULL, ch, TO_CHAR);
            remove_all_combat(ch);
            gain_skill_prof(ch, SKILL_FEIGN);
        }
        send_to_char(ch, "You fall over dead!\r\n");
        death_cry(ch);
    }
    GET_POSITION(ch) = POS_RESTING;

    WAIT_STATE(ch, SEG_VIOLENCE * 2);
}

ACMD(do_tag)
{
    struct creature *vict = NULL, *tmp_ch = NULL;
    int percent, prob;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "Who do you want to tag in?\r\n");
        return;
    }
    if (!is_fighting(ch)) {
        send_to_char(ch, "There is no need.  You aren't fighting!\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "Okay! You tag yourself in!...dummy.\r\n");
        return;
    }
    if (g_list_find(ch->fighting, vict)) {
        send_to_char(ch, "They snatch their hand back, refusing the tag!\r\n");
        return;
    }

    tmp_ch = random_opponent(ch);

    if (!tmp_ch) {
        act("But nobody is fighting you!", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (IS_NPC(vict)) {
        act("You can't tag them in!", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (CHECK_SKILL(vict, SKILL_TAG) <= 0) {
        act("$E don't know how to tag in!", false, ch, NULL, vict, TO_CHAR);
        return;
    } else {
        percent = number(1, 101);   /* 101% is a complete failure */
        prob = CHECK_SKILL(ch, SKILL_TAG);

        if (percent > prob) {
            send_to_char(ch, "You fail the tag!\r\n");
            act("$n tries to tag you into the fight, but fails.", false, vict,
                NULL, ch, TO_CHAR);
            return;
        }
        prob = CHECK_SKILL(vict, SKILL_TAG);
        if (percent > prob) {
            act("You try to tag $N, but $E misses the tag!", false, ch, NULL,
                vict, TO_CHAR);
            act("$n tries to tag you, but you miss it!", false, vict, NULL, ch,
                TO_CHAR);
            return;
        } else {
            act("You tag $N!  $E jumps into the fray!", false, ch, NULL, vict,
                TO_CHAR);
            act("$N tags you into the fight!", false, vict, NULL, ch, TO_CHAR);
            act("$n tags $N into the fight!", false, ch, NULL, vict, TO_NOTVICT);

            remove_combat(ch, tmp_ch);
            remove_combat(tmp_ch, ch);

            add_combat(vict, tmp_ch, true);
            add_combat(tmp_ch, vict, false);
            gain_skill_prof(ch, SKILL_TAG);
        }
    }
}

ACMD(do_rescue)
{
    struct creature *vict = NULL, *tmp_ch;
    int percent, prob;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "Who do you want to rescue?\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "What about fleeing instead?\r\n");
        return;
    }
    if (g_list_find(ch->fighting, vict)) {
        send_to_char(ch,
            "How can you rescue someone you are trying to kill?\r\n");
        return;
    }
    tmp_ch = random_opponent(vict);

    if (!tmp_ch) {
        act("But nobody is fighting $M!", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    // check for PKILLER flag
    if (!IS_NPC(ch) && !IS_NPC(tmp_ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
        act("That rescue would entail attacking $N, but you are flagged NO PK.", false, ch, NULL, tmp_ch, TO_CHAR);
        return;
    }

    if (GET_CLASS(ch) == CLASS_MAGIC_USER && (GET_REMORT_CLASS(ch) < 0
            || GET_REMORT_CLASS(ch) == CLASS_MAGIC_USER))
        send_to_char(ch, "But only true warriors can do this!");
    else {
        percent = number(1, 101);   /* 101% is a complete failure */
        // Temp hack until mobs actually have skills
        if (ch->char_specials.saved.act & NPC_ISNPC)
            prob = 101;
        else
            prob = CHECK_SKILL(ch, SKILL_RESCUE);

        if (percent > prob) {
            send_to_char(ch, "You fail the rescue!\r\n");
            return;
        }
        send_to_char(ch, "Banzai!  To the rescue...\r\n");
        act("You are rescued by $N, you are confused!", false, vict, NULL, ch,
            TO_CHAR);
        act("$n heroically rescues $N!", false, ch, NULL, vict, TO_NOTVICT);

        remove_combat(tmp_ch, vict);
        remove_combat(vict, tmp_ch);

        add_combat(ch, tmp_ch, true);
        add_combat(tmp_ch, ch, false);
        WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
        gain_skill_prof(ch, SKILL_RESCUE);
    }
}

ACMD(do_tornado_kick)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent, prob, dam;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You spin into the air, kicking $p!", false, ch, ovict, NULL,
                TO_CHAR);
            act("$n spins into the air, kicking $p!", false, ch, ovict, NULL,
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
    if (!ok_to_attack(ch, vict, true))
        return;
    if (GET_MOVE(ch) < 30) {
        send_to_char(ch, "You are too exhausted!\r\n");
        return;
    }

    percent = ((40 - (GET_AC(vict) / 10)) / 2) + number(1, 96);
    prob =
        CHECK_SKILL(ch,
        SKILL_TORNADO_KICK) + ((GET_DEX(ch) + GET_STR(ch)) / 2);
    if (GET_POSITION(vict) < POS_RESTING)
        prob += 30;
    prob -= GET_DEX(vict);

    dam = dice(GET_LEVEL(ch), 5) +
        strength_damage_bonus(GET_STR(ch)) + GET_DAMROLL(ch);
    if (!IS_NPC(ch))
        dam += (dam * GET_REMORT_GEN(ch)) / 10;

    int skill_lvl = LEARNED(ch);

    if (CHECK_SKILL(ch, SKILL_TORNADO_KICK) > skill_lvl)
        dam *= (CHECK_SKILL(ch, SKILL_TORNADO_KICK) +
            ((CHECK_SKILL(ch, SKILL_TORNADO_KICK) - skill_lvl) / 2));
    else
        dam *= CHECK_SKILL(ch, SKILL_TORNADO_KICK);
    dam = (skill_lvl > 0) ? (dam / skill_lvl):0;

    RAW_EQ_DAM(ch, WEAR_FEET, &dam);

    if (NON_CORPOREAL_MOB(vict))
        dam = 0;

    WAIT_STATE(ch, PULSE_VIOLENCE * 3);

    if (percent > prob) {
        damage(ch, vict, NULL, 0, SKILL_TORNADO_KICK, -1);
        GET_MOVE(ch) -= 10;
    } else {
        GET_MOVE(ch) -= 10;
        if (!(damage(ch, vict, NULL, dam, SKILL_TORNADO_KICK, -1)) &&
            GET_LEVEL(ch) > number(30, 55) &&
            (!(damage(ch, vict, NULL,
                      number(0, 1 + GET_LEVEL(ch) / 10) ?
                      (int)(dam * 1.2) :
                      0,
                      SKILL_TORNADO_KICK, -1))) &&
            (GET_MOVE(ch) -= 10) &&
            GET_LEVEL(ch) > number(40, 55) &&
            (!(damage(ch, vict, NULL,
                      number(0, 1 + GET_LEVEL(ch) / 10) ?
                      (int)(dam * 1.3) : 0, SKILL_TORNADO_KICK, -1))))
            GET_MOVE(ch) -= 10;
        gain_skill_prof(ch, SKILL_TORNADO_KICK);

    }
}

ACMD(do_sleeper)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent, prob;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You try to put the sleeper on $p!", false, ch, ovict, NULL,
                TO_CHAR);
            act("$n tries to put the sleeper on $p!", false, ch, ovict, NULL,
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
            true, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (GET_POSITION(vict) <= POS_SLEEPING) {
        send_to_char(ch, "Yeah.  Right.\r\n");
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;

    percent = ((10 + GET_LEVEL(vict)) / 2) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_SLEEPER);

    if (AFF_FLAGGED(vict, AFF_ADRENALINE))
        prob -= GET_LEVEL(vict);

    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    WAIT_STATE(ch, 3 RL_SEC);

    //
    // failure
    //

    if (percent > prob || NPC_FLAGGED(vict, NPC_NOSLEEP) ||
        GET_LEVEL(vict) > LVL_CREATOR) {
        damage(ch, vict, NULL, 0, SKILL_SLEEPER, WEAR_NECK_1);
    }
    //
    // success
    //

    else {
        gain_skill_prof(ch, SKILL_SLEEPER);
        damage(ch, vict, NULL, 18, SKILL_SLEEPER, WEAR_NECK_1);
        // FIXME: don't do anything if the attack failed

        //
        // put the victim to sleep if he's still alive
        //
        if (!is_dead(vict)) {
            struct affected_type af;

            init_affect(&af);

            remove_all_combat(vict);

            WAIT_STATE(vict, 4 RL_SEC);
            GET_POSITION(vict) = POS_SLEEPING;

            af.is_instant = false;
            af.duration = 2;
            af.bitvector = AFF_SLEEP;
            af.type = SKILL_SLEEPER;
            af.aff_index = 1;
            af.level = GET_LEVEL(ch);
            af.owner = GET_IDNUM(ch);
            affect_join(vict, &af, false, false, false, false);

            if (is_dead(ch))
                return;

            remove_combat(ch, vict);
            remember(vict, ch);
        }
    }
}

ACMD(do_turn)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent, prob;
    char *arg;

    if (GET_CLASS(ch) != CLASS_CLERIC && GET_CLASS(ch) != CLASS_KNIGHT &&
        GET_REMORT_CLASS(ch) != CLASS_CLERIC
        && GET_REMORT_CLASS(ch) != CLASS_KNIGHT
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "Heathens are not able to turn the undead!\r\n");
        return;
    }

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You turn $p!", false, ch, ovict, NULL, TO_CHAR);
            act("$n turns $p!", false, ch, ovict, NULL, TO_ROOM);
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

    if (!ok_to_attack(ch, vict, true))
        return;
    percent = ((GET_LEVEL(vict)) + number(1, 101)); /* 101% is a complete
                                                     * failure */
    if (NPC_FLAGGED(vict, NPC_NOTURN))
        percent += GET_LEVEL(vict);

    prob = CHECK_SKILL(ch, SKILL_TURN);

    if (percent > prob) {
        damage(ch, vict, NULL, 0, SKILL_TURN, -1);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    } else {
        if ((GET_LEVEL(ch) - GET_LEVEL(vict) - number(0, 10)) > 20) {
            act("$n calls upon the power of $s deity and DESTROYS $N!!",
                false, ch, NULL, vict, TO_ROOM);
            act("You call upon the power of your deity and DESTROY $N!!",
                true, ch, NULL, vict, TO_CHAR);
            gain_exp(ch, GET_EXP(vict));

            slog("%s killed %s with a turn at %d.",
                GET_NAME(ch), GET_NAME(vict), ch->in_room->number);
            gain_skill_prof(ch, SKILL_TURN);

            raw_kill(vict, ch, SKILL_TURN); // Destroying a victime with turn
        } else if ((GET_LEVEL(ch) - GET_LEVEL(vict)) > 10) {
            act("$n calls upon the power of $s deity and forces $N to flee!",
                false, ch, NULL, vict, TO_ROOM);
            act("You call upon the power of your deity and force $N to flee!",
                true, ch, NULL, vict, TO_CHAR);
            do_flee(vict, tmp_strdup(""), 0, 0);
            gain_skill_prof(ch, SKILL_TURN);
        } else {
            damage(ch, vict, NULL, GET_LEVEL(ch) / 2, SKILL_TURN, -1);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, SKILL_TURN);
        }
    }
}

struct creature *
random_fighter(struct room_data *room,
               struct creature *ch,
               struct creature *vict)
{
    for (GList *it = first_living(room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch != ch
            && tch != vict && g_list_find(tch->fighting, vict)
            && !number(0, 2))
            return tch;
    }
    return NULL;
}

struct creature *
random_bystander(struct room_data *room,
               struct creature *ch,
               struct creature *vict)
{
    for (GList *it = first_living(room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch != ch && tch != vict && !number(0, 2))
            return tch;
    }
    return NULL;
}

void
shoot_energy_gun(struct creature *ch,
    struct creature *vict,
    struct obj_data *target, struct obj_data *gun)
{
    int16_t prob, dam, cost;
    int dum_ptr = 0, dum_move = 0;
    struct obj_data *dum_obj;
    struct affected_type *af = NULL;

    if (!gun->contains || !IS_ENERGY_CELL(gun->contains)) {
        act("$p is not loaded with an energy cell.", false, ch, gun, NULL,
            TO_CHAR);
        return;
    }
    if (CUR_ENERGY(gun->contains) <= 0) {
        act("$p is out of energy.", false, ch, gun, NULL, TO_CHAR);
        if (vict)
            act("$n points $p at $N!", true, ch, gun, vict, TO_ROOM);
        act("$p hums faintly in $n's hand.", false, ch, gun, NULL, TO_ROOM);
        return;
    }

    cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));
    if (target) {
        dam = dice(GUN_DISCHARGE(gun), (cost / 2));

        CUR_ENERGY(gun->contains) -= cost;
        act(tmp_sprintf("$n blasts %s with $p!", target->name),
            false, ch, gun, NULL, TO_ROOM);
        act("You blast $p!", false, ch, target, NULL, TO_CHAR);
        damage_eq(ch, target, dam, TYPE_BLAST);
        return;
    }

    prob = calc_skill_prob(ch, vict, SKILL_SHOOT,
                           &dum_ptr, &dum_ptr, &dum_move, &dum_move,
                           &dum_ptr, &dum_ptr, &dum_ptr, &dum_obj, &dum_ptr, af);

    if (is_dead(ch) || is_dead(vict))
        return;

    prob += CHECK_SKILL(ch, SKILL_ENERGY_WEAPONS) / 4;

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (tch != ch && g_list_find(tch->fighting, ch))
            prob -= GET_LEVEL(tch) / 8;
    }

    struct creature *miss_vict = NULL;

    if (is_fighting(vict)
        && !g_list_find(vict->fighting, ch)
        && number(1, 121) > prob) {
        miss_vict = random_opponent(vict);
    } else if (is_fighting(vict) && number(1, 101) > prob) {
        miss_vict = random_fighter(ch->in_room, ch, vict);
    } else if (number(1, 81) > prob) {
        miss_vict = random_bystander(ch->in_room, ch, vict);
    }
    if (miss_vict)
        vict = miss_vict;

    cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));

    dam = dice(GET_OBJ_VAL(gun, 1), GET_OBJ_VAL(gun, 2));
    dam += dam * skill_bonus(ch, SKILL_SHOOT) / 100;    //damage bonus for taking the time to aim and shoot
    dam += GET_HITROLL(ch);

    CUR_ENERGY(gun->contains) -= cost;

    //
    // miss
    //

    if (number(0, 121) > prob) {
        check_attack(ch, vict);
        damage(ch, vict, gun, 0, GUN_TYPE(gun) + TYPE_EGUN_LASER, number(0,
                NUM_WEARS - 1));
    }
    //
    // hit
    //

    else {
        check_attack(ch, vict);
        damage(ch, vict, gun, dam, GUN_TYPE(gun) + TYPE_EGUN_LASER, number(0,
                NUM_WEARS - 1));
    }
    //
    // if the attacker was somehow killed, return immediately
    //

    if (is_dead(ch))
        return;

    if (!CUR_ENERGY(gun->contains)) {
        act("$p has been depleted of fuel.  You must replace the energy cell before firing again.", false, ch, gun, NULL, TO_CHAR);
    }

    WAIT_STATE(ch, 4 RL_SEC);
    //don't do the extra stuff because we're only shooting one blast at a time
    //ROF doesn't exist anymore
}

int
bullet_damage(struct obj_data *gun, struct obj_data *bullet)
{
    int16_t dam;

    dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
    dam += BUL_DAM_MOD(bullet);
    return dam;
}

void
fire_projectile_round(struct creature *ch,
    struct creature *vict,
    struct obj_data *gun,
    struct obj_data *bullet, int bullet_num, int prob)
{
    int16_t dam;
    const char *arrow_name;

    if (!bullet) {
        act("$p is out of ammo.",
            false, ch, gun->contains ? gun->contains : gun, NULL, TO_CHAR);

        if (IS_ARROW(gun) && IS_ELF(ch))
            WAIT_STATE(ch, (((bullet_num * 2) + 6) / 4) RL_SEC);
        else
            WAIT_STATE(ch, (((bullet_num * 2) + 6) / 2) RL_SEC);

        return;
    }

    prob -= (bullet_num * 4);

    dam = bullet_damage(gun, bullet);
    if (IS_ARROW(gun)) {
        obj_from_obj(bullet);
        obj_to_room(bullet, ch->in_room);
        arrow_name = tmp_strdup(bullet->name);
        damage_eq(NULL, bullet, dam / 4, TYPE_HIT);
    } else {
        if (!bullet_num && !IS_FLAMETHROWER(gun))
            sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS,
                /*GUN_TYPE(gun), */ dam, CUR_R_O_F(gun));
        arrow_name = "";
    }

    if (number(0, 121) > prob) {
        //
        // miss
        //
        if (IS_ARROW(bullet)) {
            act(tmp_sprintf("$n fires %s at you from $p!", arrow_name),
                false, ch, gun, vict, TO_VICT);
            act(tmp_sprintf("You fire %s at $N from $p!", arrow_name),
                false, ch, gun, vict, TO_CHAR);
            act(tmp_sprintf("$n fires %s at $N from $p!", arrow_name),
                false, ch, gun, vict, TO_NOTVICT);
        }
        damage(ch, vict, gun, 0,
            IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER : (IS_ARROW(gun) ?
                SKILL_ARCHERY : SKILL_PROJ_WEAPONS), number(0, NUM_WEARS - 1));
    } else {
        //
        // hit
        //
        if (IS_ARROW(bullet)) {
            act(tmp_sprintf("$n fires %s into you from $p!  OUCH!!",
                    arrow_name), false, ch, gun, vict, TO_VICT);
            act(tmp_sprintf("You fire %s into $N from $p!", arrow_name), false,
                ch, gun, vict, TO_CHAR);
            act(tmp_sprintf("$n fires %s into $N from $p!", arrow_name), false,
                ch, gun, vict, TO_NOTVICT);
        }
        damage(ch, vict, gun, dam,
            (IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER :
                (IS_ARROW(gun) ? SKILL_ARCHERY :
                    SKILL_PROJ_WEAPONS)), number(0, NUM_WEARS - 1));
    }
}

void
projectile_blast_corpse(struct creature *ch, struct obj_data *gun,
    struct obj_data *bullet)
{
    //
    // vict is dead, blast the corpse
    //
    int16_t dam = bullet_damage(gun, bullet);

    if (ch->in_room->contents && IS_CORPSE(ch->in_room->contents) &&
        CORPSE_KILLER(ch->in_room->contents) ==
        (IS_NPC(ch) ? -GET_NPC_VNUM(ch) : GET_IDNUM(ch))) {
        if (IS_ARROW(gun)) {
            act("You shoot $p with $P!!",
                false, ch, ch->in_room->contents, gun, TO_CHAR);
            act("$n shoots $p with $P!!",
                false, ch, ch->in_room->contents, gun, TO_ROOM);
        } else {
            act("You blast $p with $P!!",
                false, ch, ch->in_room->contents, gun, TO_CHAR);
            act("$n blasts $p with $P!!",
                false, ch, ch->in_room->contents, gun, TO_ROOM);
        }
        if (damage_eq(ch, ch->in_room->contents, dam, TYPE_BLAST))
            return;
    } else {
        act("You fire off a round from $P.",
            false, ch, ch->in_room->contents, gun, TO_ROOM);
        act("$n fires off a round from $P.",
            false, ch, ch->in_room->contents, gun, TO_ROOM);
    }
}

void
shoot_projectile_gun(struct creature *ch,
    struct creature *vict,
    struct obj_data *target, struct obj_data *gun)
{
    struct obj_data *bullet = NULL;
    struct obj_data *dum_obj;
    int16_t prob, dam;
    int bullet_num = 0, dum_ptr = 0, dum_move = 0;
    struct affected_type *af = NULL;

    if (GUN_TYPE(gun) < 0 || GUN_TYPE(gun) >= NUM_GUN_TYPES) {
        act("$p is a bogus gun.  extracting.", false, ch, gun, NULL, TO_CHAR);
        extract_obj(gun);
        return;
    }

    if (MAX_LOAD(gun)) {
        bullet = gun->contains;
        if (!bullet) {
            act("$p is not loaded.", false, ch, gun, NULL, TO_CHAR);
            return;
        }
    } else {
        bullet = gun->contains->contains;
        if (!bullet) {
            act("$P is not loaded.", false, ch, gun, gun->contains, TO_CHAR);
            return;
        }
    }

    if (target) {
        dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
        dam += BUL_DAM_MOD(bullet);

        if (IS_ARROW(gun)) {
            act(tmp_sprintf("$n fires %s from $p into $P!", bullet->name),
                false, ch, gun, target, TO_ROOM);
            act("You fire $P into $p!", false, ch, target, bullet, TO_CHAR);
            obj_from_obj(bullet);
            obj_to_room(bullet, ch->in_room);
            damage_eq(NULL, bullet, dam / 4, TYPE_HIT);
        } else {
            act(tmp_sprintf("$n blasts %s with $p!", target->name),
                false, ch, gun, NULL, TO_ROOM);
            act("You blast $p!", false, ch, target, NULL, TO_CHAR);
            extract_obj(bullet);
        }
        damage_eq(ch, target, dam, TYPE_BLAST);
        return;
    }

    prob = calc_skill_prob(ch, vict,
                           (IS_ARROW(gun) ? SKILL_ARCHERY : SKILL_SHOOT),
                           &dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
                           &dum_ptr, &dum_ptr, &dum_obj, &dum_ptr, af);

    if (!IS_ARROW(gun))
        prob += CHECK_SKILL(ch, SKILL_PROJ_WEAPONS) / 8;
    else if (IS_ELF(ch))
        prob += number(GET_LEVEL(ch) / 4,
            GET_LEVEL(ch) / 2) + (GET_REMORT_GEN(ch) * 4);

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (tch != ch && g_list_find(tch->fighting, ch))
            prob -= GET_LEVEL(tch) / 8;
    }

    if (is_fighting(ch))
        prob -= 10;

    struct creature *miss_vict = NULL;

    if (is_fighting(vict)
        && !g_list_find(vict->fighting, ch)
        && number(1, 121) > prob) {
        miss_vict = random_opponent(vict);
    } else if (is_fighting(vict) && number(1, 101) > prob) {
        miss_vict = random_fighter(ch->in_room, ch, vict);
    } else if (number(1, 81) > prob) {
        miss_vict = random_bystander(ch->in_room, ch, vict);
    }
    if (miss_vict)
        vict = miss_vict;

    if (CUR_R_O_F(gun) <= 0)
        CUR_R_O_F(gun) = 1;

    // loop through ROF of the gun for burst fire

    for (bullet_num = 0; bullet && bullet_num < CUR_R_O_F(gun); bullet_num++) {
        struct obj_data *next_bullet = bullet->next_content;

        // Search for next bullet/arrow
        while (next_bullet && !IS_BULLET(next_bullet)) {
            next_bullet = next_bullet->next_content;
        }

        if (is_dead(vict)) {
            // if victim was killed, fire at their corpse
            projectile_blast_corpse(ch, gun, bullet);
        } else {
            fire_projectile_round(ch, vict, gun, bullet,
                bullet_num, prob);
        }
        if (!IS_ARROW(gun))
            extract_obj(bullet);

        bullet = next_bullet;

        // if the attacker was somehow killed, return immediately
        if (is_dead(ch))
            return;
    }

    if (IS_ARROW(gun) && IS_ELF(ch))
        WAIT_STATE(ch, (((bullet_num * 2) + 6) / 4) RL_SEC);
    else
        WAIT_STATE(ch, (((bullet_num * 2) + 6) / 2) RL_SEC);
}

ACMD(do_shoot)
{
    struct creature *vict = NULL;
    struct obj_data *gun = NULL, *target = NULL, *bullet = NULL;
    int i;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Shoot what at who?\r\n");
        return;
    }

    if (!strncmp(arg, "internal", 8)) {

        arg = tmp_getword(&argument);

        if (!*arg) {
            send_to_char(ch, "Discharge which implant at who?\r\n");
            return;
        }

        if (!(gun = get_object_in_equip_vis(ch, arg, ch->implants, &i))) {
            send_to_char(ch, "You are not implanted with %s '%s'.\r\n",
                AN(arg), arg);
            return;
        }

    } else if ((!(gun = GET_EQ(ch, WEAR_WIELD)) || !isname(arg, gun->aliases))
        && (!(gun = GET_EQ(ch, WEAR_WIELD_2)) || !isname(arg, gun->aliases))) {
        send_to_char(ch, "You are not wielding %s '%s'.\r\n", AN(arg), arg);
        return;
    }

    if (!IS_ENERGY_GUN(gun) && !IS_GUN(gun)) {
        act("$p is not a gun.", false, ch, gun, NULL, TO_CHAR);
        return;
    }

    if (is_fighting(ch)
        && g_list_find(random_opponent(ch)->fighting, ch)
        && !number(0, 3)
        && (!skill_bonus(ch, SKILL_ENERGY_WEAPONS > 60))
        && (GET_LEVEL(ch) < LVL_GRGOD)) {
        send_to_char(ch, "You are in too close to get off a shot!\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {

        if (!(vict = random_opponent(ch))) {
            if (IS_GUN(gun) && !IS_ARROW(gun) && GUN_TYPE(gun) >= 0
                && GUN_TYPE(gun) < NUM_GUN_TYPES) {

                if (!(bullet = gun->contains) || (!MAX_LOAD(gun)
                        && !(bullet = bullet->contains)))
                    act("$p is not loaded.", false, ch, gun, NULL, TO_CHAR);
                else {
                    act("$n fires $p into the air.", false, ch, gun, NULL,
                        TO_ROOM);
                    act("You fire $p into the air.", false, ch, gun, NULL,
                        TO_CHAR);
                    sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS, /*GUN_TYPE(gun), */
                        dice(gun_damage[GUN_TYPE(gun)][0],
                            gun_damage[GUN_TYPE(gun)][1]) +
                        BUL_DAM_MOD(bullet), 1);
                    extract_obj(bullet);
                }
            } else
                act("Shoot $p at what?", false, ch, gun, NULL, TO_CHAR);
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
        if (!ok_to_attack(ch, vict, true))
            return;

    }

    if (IS_ENERGY_GUN(gun))
        shoot_energy_gun(ch, vict, target, gun);
    else
        shoot_projectile_gun(ch, vict, target, gun);
}

ACMD(do_ceasefire)
{
    struct creature *f = NULL;

    if (!is_fighting(ch)) {
        send_to_char(ch, "But you aren't fighting anyone.\r\n");
        return;
    }

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch != ch && g_list_find(tch->fighting, ch))
            f = tch;
    }

    if (f)
        send_to_char(ch, "You can't ceasefire while your enemy is actively "
            "attacking you!\r\n");
    else {
        act("You stop attacking your opponents.", false, ch, NULL, NULL, TO_CHAR);
        act("$n stops attacking $s opponents.", false, ch, NULL,
            NULL, TO_NOTVICT);
        act("$n stops attacking you.", false, ch, NULL, f, TO_VICT);
        g_list_free(ch->fighting);
        ch->fighting = NULL;
        WAIT_STATE(ch, 2 RL_SEC);
    }
}

ACMD(do_disarm)
{
    struct creature *vict = NULL;
    struct obj_data *weap = NULL, *weap2 = NULL;
    int percent, prob;
    char *arg;
    ACMD(do_drop);
    ACMD(do_get);

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
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

    if (!ok_to_attack(ch, vict, true))
        return;

    if (!(weap = GET_EQ(vict, WEAR_WIELD))) {
        send_to_char(ch, "They aren't wielding anything, fool.\r\n");
        return;
    }

    percent = ((GET_LEVEL(vict)) + number(1, 101)); /* 101% is a complete
                                                     * failure */
    prob = MAX(0, GET_LEVEL(ch) +
        CHECK_SKILL(ch, SKILL_DISARM) - GET_OBJ_WEIGHT(weap));

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
        act("You knock $p out of $N's hand!", false, ch, weap, vict, TO_CHAR);
        act("$n knocks $p out of $N's hand!", false, ch, weap, vict,
            TO_NOTVICT);
        act("$n knocks $p out of your hand!", false, ch, weap, vict, TO_VICT);
        unequip_char(vict, WEAR_WIELD, EQUIP_WORN);
        obj_to_char(weap, vict);
        if (GET_EQ(vict, WEAR_WIELD_2)) {
            if ((weap2 = unequip_char(vict, WEAR_WIELD_2, EQUIP_WORN)))
                if (equip_char(vict, weap2, WEAR_WIELD, EQUIP_WORN))
                    return;
        }

        if (GET_STR(ch) + number(0, 20) > GET_STR(vict) + GET_DEX(vict)) {
            do_drop(vict, fname(weap->aliases), 0, 0);
            if (IS_NPC(vict) && !GET_NPC_WAIT(vict) && AWAKE(vict) &&
                number(0, GET_LEVEL(vict)) > (GET_LEVEL(vict) / 2))
                do_get(vict, fname(weap->aliases), 0, 0);
        }

        GET_EXP(ch) += MIN(100, GET_OBJ_WEIGHT(weap));
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (IS_NPC(vict) && !is_fighting(vict) && can_see_creature(vict, ch)) {
            add_combat(ch, vict, true);
            add_combat(vict, ch, false);
        }
    } else {
        send_to_char(ch, "You fail the disarm!\r\n");
        act("$n tries to disarm you!", false, ch, NULL, vict, TO_VICT);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (IS_NPC(vict) && !is_fighting(vict)) {
            add_combat(ch, vict, true);
            add_combat(vict, ch, false);
        }
    }
}

/***********************************************************************/

ACMD(do_impale)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You try to impale $p!", false, ch, ovict, NULL, TO_CHAR);
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
                false, ch, NULL, ch->master, TO_CHAR);
            return;
        }
        if (!strcmp(arg, "self")) {
            if (!(ch->in_room && is_arena_combat(ch, ch))) {
                act("You impale yourself with $p!", false, ch, weap, NULL,
                    TO_CHAR);
                act("$n suddenly impales $mself with $p!", true, ch, weap, NULL,
                    TO_ROOM);
                mudlog(GET_INVIS_LVL(ch), NRM, true,
                    "%s killed self with an impale at %d.",
                    GET_NAME(ch), ch->in_room->number);
                gain_exp(ch, -(GET_LEVEL(ch) * 1000));
                raw_kill(ch, ch, SKILL_IMPALE); // Impaling yourself
                return;
            } else {
                send_to_char(ch, "Suicide is not the answer...\r\n");
                return;
            }
        } else {
            act("Are you sure $p is supposed to go there?", false, ch, weap, NULL,
                TO_CHAR);
            return;
        }

    }
    if (!ok_to_attack(ch, vict, true))
        return;

    percent = ((10 - (GET_AC(vict) / 10)) * 2) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_IMPALE);

    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    dam = dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2)) * 2 +
        GET_OBJ_WEIGHT(weap) * 4 + GET_DAMROLL(ch);
    dam += dam * skill_bonus(ch, SKILL_IMPALE) / 200;

    if (IS_OBJ_STAT2(weap, ITEM2_TWO_HANDED) && weap->worn_on == WEAR_WIELD)
        dam *= 2;

    if (NON_CORPOREAL_MOB(vict))
        dam = 0;

    if (percent > prob) {
        damage(ch, vict, weap, 0, SKILL_IMPALE, WEAR_BODY);
    } else {
        damage(ch, vict, weap, dam, SKILL_IMPALE, WEAR_BODY);
        gain_skill_prof(ch, SKILL_IMPALE);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_intimidate)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    struct affected_type af;
    int prob = 0;
    char *arg;

    init_affect(&af);

    arg = tmp_getword(&argument);
    af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You attempt to intimidate $p!", false, ch, ovict, NULL, TO_CHAR);
            act("$n attempts to intimidate $p!", false, ch, ovict, NULL, TO_ROOM);
            return;
        } else {
            send_to_char(ch, "Intimidate who?\r\n");
            WAIT_STATE(ch, 4);
            return;
        }
    }

    if (!can_see_creature(vict, ch)) {
        act("$N doesn't seem to be able to see you.", false, ch, NULL, vict,
            TO_CHAR);
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "You attempt to intimidate yourself.\r\n"
            "You feel intimidated!\r\n");
        act("$n intimidates $mself!", true, ch, NULL, NULL, TO_ROOM);
        if (affected_by_spell(ch, SKILL_INTIMIDATE))
            return;
        af.type = SKILL_INTIMIDATE;
        af.duration = 2 + 2 * (GET_LEVEL(ch) > 40);
        af.modifier = -5;
        af.location = APPLY_HITROLL;
        af.owner = GET_IDNUM(ch);
        affect_to_char(vict, &af);
        return;
    }
    if (!ok_to_attack(ch, vict, true))
        return;

    prob = GET_LEVEL(vict) + number(1, 51) +
        (IS_NPC(vict) ? (GET_MORALE(vict) / 2) : GET_LEVEL(vict)) +
        (AFF_FLAGGED(vict, AFF_CONFIDENCE) ? GET_LEVEL(vict) : 0);

    if (!ok_damage_vendor(ch, vict))
        return;

    act("You attempt to intimidate $N.", false, ch, NULL, vict, TO_CHAR);
    act("$n attempts to intimidate $N.", false, ch, NULL, vict, TO_NOTVICT);
    act("$n attempts to intimidate you.", false, ch, NULL, vict, TO_VICT);

    if (affected_by_spell(vict, SKILL_INTIMIDATE)) {
        act("$n cringes in terror!", false, vict, NULL, NULL, TO_ROOM);
        send_to_char(vict, "You cringe in terror!\r\n");
    } else if ((affected_by_spell(vict, SPELL_FEAR) ||
            GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_INTIMIDATE) > prob) &&
        !IS_UNDEAD(vict)) {
        act("$n looks intimidated!", true, vict, NULL, NULL, TO_ROOM);
        send_to_char(vict, "You feel intimidated!\r\n");

        af.type = SKILL_INTIMIDATE;
        af.duration = 2 + 2 * (GET_LEVEL(ch) > 40);
        af.modifier = -5;
        af.location = APPLY_HITROLL;
        af.owner = GET_IDNUM(ch);
        affect_to_char(vict, &af);

    } else {
        act("$N snickers at $n", true, ch, NULL, vict, TO_NOTVICT);
        act("$N snickers at you!", true, ch, NULL, vict, TO_CHAR);
        send_to_char(vict, "You snicker!\r\n");
        if (IS_NPC(vict))
            hit(vict, ch, TYPE_UNDEFINED);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
}

ACMD(do_beguile)
{

    struct creature *vict = NULL;
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
        true, ch, NULL, vict, TO_VICT);
    act("You look deeply into $S eyes with an enigmatic look.",
        false, ch, NULL, vict, TO_CHAR);
    act("$n looks deeply into $N's eyes with an enigmatic look.",
        true, ch, NULL, vict, TO_NOTVICT);

    if (!can_see_creature(vict, ch))
        return;

    if (!ok_to_attack(ch, vict, true))
        return;

    if (GET_INT(vict) < 4) {
        act("$N is too stupid to be beguiled.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    check_attack(ch, vict);
    if (can_see_creature(vict, ch) &&
        (CHECK_SKILL(ch, SKILL_BEGUILE) + GET_CHA(ch)) >
        (number(0, 50) + GET_LEVEL(vict) + GET_INT(vict)))
        spell_charm(GET_LEVEL(ch), ch, vict, NULL, NULL);
    else
        send_to_char(ch, "There appears to be no effect.\r\n");

    WAIT_STATE(ch, 2 RL_SEC);
}

struct creature *
randomize_target(struct creature *ch, struct creature *vict, short prob)
{
    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (tch != ch && g_list_find(tch->fighting, ch))
            prob -= GET_LEVEL(tch) / 8;
    }

    if (!g_list_find(vict->fighting, ch)) {
        if (number(1, 121) > prob)
            vict = random_opponent(vict);
    } else if (is_fighting(vict) && number(1, 101) > prob) {
        vict = random_fighter(ch->in_room, ch, vict);

    } else if (number(1, 81) > prob) {
        vict = random_bystander(ch->in_room, ch, vict);
    }

    return vict;
}

//This function assumes that ch is a merc.  It provides for  mercs
//shooting wielded guns in combat instead of bludgeoning with them
int
do_combat_fire(struct creature *ch, struct creature *vict)
{
    struct obj_data *bullet = NULL, *gun = NULL;
    struct obj_data *dum_obj;
    int16_t prob, dam, cost;
    int dum_ptr = 0, dum_move = 0;
    struct affected_type *af = NULL;
    struct obj_data *weap1 = NULL, *weap2 = NULL;

    weap1 = GET_EQ(ch, WEAR_WIELD);
    weap2 = GET_EQ(ch, WEAR_WIELD_2);

    if ((weap1 && (IS_GUN(weap1) || IS_ENERGY_GUN(weap1))) &&
        (weap2 && (IS_GUN(weap2) || IS_ENERGY_GUN(weap2)))) {
        prob = number(1, 101);
        if (prob % 2)
            gun = weap1;
        else
            gun = weap2;
    } else if (weap1 && (IS_GUN(weap1) || IS_ENERGY_GUN(weap1))) {
        gun = weap1;
    } else if (weap2 && (IS_GUN(weap2) || IS_ENERGY_GUN(weap2))) {
        gun = weap2;
    } else
        return -1;

    //if our victim is NULL we simply return
    if (vict == NULL) {
        errlog("NULL vict in %s().", __FUNCTION__);
        return 0;
    }
    // This should never happen
    if (vict == ch) {
        mudlog(LVL_AMBASSADOR, NRM, true,
            "ERROR: ch == vict in %s()!", __FUNCTION__);
        return 0;
    }

    if (GET_POSITION(ch) < POS_FIGHTING) {
        send_to_char(ch, "You can't fight while sitting!\r\n");
        return 0;
    }
    // And since ch and vict should already be fighting this should never happen either
    if (!ok_to_attack(ch, vict, true))
        return 0;

    //
    // The Energy Gun block
    //

    if (IS_ENERGY_GUN(gun)) {

        if (!gun->contains || !IS_ENERGY_CELL(gun->contains)) {
            act("$p doesn't contain an energy cell!",
                false, ch, gun, NULL, TO_CHAR);
            return -1;
        }
        if (CUR_ENERGY(gun->contains) <= 0) {
            return -1;
        }

        prob = calc_skill_prob(ch, vict, SKILL_SHOOT,
                               &dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
                               &dum_ptr, &dum_ptr, &dum_obj, &dum_ptr, af);

        prob += CHECK_SKILL(ch, SKILL_ENERGY_WEAPONS) / 4;
        prob += dexterity_hit_bonus(GET_DEX(ch));

        vict = randomize_target(ch, vict, prob);

        cost = MIN(CUR_ENERGY(gun->contains), GUN_DISCHARGE(gun));

        dam = dice(GET_OBJ_VAL(gun, 1), GET_OBJ_VAL(gun, 2));
        dam += GET_HITROLL(ch);
        dam += dexterity_damage_bonus(GET_DEX(ch));

        CUR_ENERGY(gun->contains) -= cost;

        if (number(0, 121) > prob) {    //miss
            check_attack(ch, vict);
            damage(ch, vict, gun, 0, SKILL_ENERGY_WEAPONS,
                number(0, NUM_WEARS - 1));
        } else {                // hit
            check_attack(ch, vict);
            damage(ch, vict, gun, dam, SKILL_ENERGY_WEAPONS,
                number(0, NUM_WEARS - 1));
        }

        // if the attacker was somehow killed, return immediately
        if (is_dead(ch) || is_dead(vict)) {
            return true;
        }

        if (!CUR_ENERGY(gun->contains)) {
            act("$p has been depleted of fuel.  Replace cell before further use.", false, ch, gun, NULL, TO_CHAR);
        }
        return false;
    }
    //
    // The Projectile Gun block
    //

    if (GUN_TYPE(gun) < 0 || GUN_TYPE(gun) >= NUM_GUN_TYPES) {
        act("$p is a bogus gun.  extracting.", false, ch, gun, NULL, TO_CHAR);
        extract_obj(gun);
        return 0;
    }
    if (!(bullet = gun->contains)) {
        act("$p is not loaded.", false, ch, gun, NULL, TO_CHAR);
        return -1;
    }

    if (!MAX_LOAD(gun) && !(bullet = gun->contains->contains)) {
        act("$P is not loaded.", false, ch, gun, gun->contains, TO_CHAR);
        return -1;
    }

    prob = calc_skill_prob(ch, vict, SKILL_SHOOT,
                           &dum_ptr, &dum_ptr, &dum_move, &dum_move, &dum_ptr,
                           &dum_ptr, &dum_ptr, &dum_obj, &dum_ptr, af);

    prob += CHECK_SKILL(ch, SKILL_PROJ_WEAPONS) / 8;

    vict = randomize_target(ch, vict, prob);

    if (!bullet) {
        return -1;
    }

    dam = dice(gun_damage[GUN_TYPE(gun)][0], gun_damage[GUN_TYPE(gun)][1]);
    dam += BUL_DAM_MOD(bullet);

    if (IS_ARROW(gun)) {
        obj_from_obj(bullet);
        obj_to_room(bullet, ch->in_room);
        damage_eq(NULL, bullet, dam / 4, TYPE_HIT);
    } else {
        if (!IS_FLAMETHROWER(gun))
            sound_gunshots(ch->in_room, SKILL_PROJ_WEAPONS,
                dam, CUR_R_O_F(gun));

        extract_obj(bullet);
    }
    // we /must/ have a clip in a clipped gun at this point!
    // TODO: fix this
    bullet = MAX_LOAD(gun) ? gun->contains : gun->contains->contains;

    if (number(0, 121) > prob) {    //miss
        damage(ch, vict, gun, 0,
            IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER : SKILL_PROJ_WEAPONS,
            number(0, NUM_WEARS - 1));
    }

    // hit
    return damage(ch, vict, gun, dam,
                  IS_FLAMETHROWER(gun) ? TYPE_FLAMETHROWER : SKILL_PROJ_WEAPONS,
                  number(0, NUM_WEARS - 1));
}
