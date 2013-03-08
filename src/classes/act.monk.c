//
// File: act.monk.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
//   File: act.monk.c                Created for TempusMUD by Fireball
//

#ifdef HAS_CONFIG_H
#endif
#include <stdio.h>
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
#include "materials.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"

void
perform_monk_meditate(struct creature *ch)
{
    struct affected_type af;

    init_affect(&af);

    af.level = GET_LEVEL(ch) + (GET_REMORT_GEN(ch) * 4);
    MEDITATE_TIMER(ch)++;

    // meditating makes a monk's alignment correct itself
    if (GET_ALIGNMENT(ch) != 0) {
        if (GET_ALIGNMENT(ch) > 0)
            GET_ALIGNMENT(ch) -=
                number(0, affected_by_spell(ch, ZEN_DISPASSION) ? 10 : 1);
        else if (GET_ALIGNMENT(ch) < 0)
            GET_ALIGNMENT(ch) +=
                number(0, affected_by_spell(ch, ZEN_DISPASSION) ? 10 : 1);
        if (GET_ALIGNMENT(ch) == 0)
            send_to_char(ch, "You have achieved perfect balance.\r\n");
    }

    if (PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch, "%s[MEDITATE] timer:%d%s\r\n",
            CCCYN(ch, C_NRM), MEDITATE_TIMER(ch), CCNRM(ch, C_NRM));

    // oblivity
    if (!AFF2_FLAGGED(ch, AFF2_OBLIVITY)
        && CHECK_SKILL(ch, ZEN_OBLIVITY) >= LEARNED(ch)) {
        int target =
            MEDITATE_TIMER(ch) + (CHECK_SKILL(ch,
                ZEN_OBLIVITY) / 4) + GET_WIS(ch);
        int test = (mag_manacost(ch, ZEN_OBLIVITY) + number(20, 40));

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Oblivity   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch, "You begin to perceive the zen of oblivity.\r\n");
            af.type = ZEN_OBLIVITY;
            af.bitvector = AFF2_OBLIVITY;
            af.aff_index = 2;
            af.duration = af.level / 5;
            af.owner = GET_IDNUM(ch);
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            gain_skill_prof(ch, ZEN_OBLIVITY);
            return;
        }
    }
    // awareness
    if (!affected_by_spell(ch, ZEN_AWARENESS)
        && CHECK_SKILL(ch, ZEN_AWARENESS) >= LEARNED(ch)) {
        int target =
            MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_AWARENESS) / 4);
        int test =
            (mag_manacost(ch, ZEN_AWARENESS) + number(6, 40) - GET_WIS(ch));

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Awareness   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch, "You become one with the zen of awareness.\r\n");
            af.type = ZEN_AWARENESS;
            af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
            if (GET_LEVEL(ch) < 36) {
                af.bitvector = AFF_DETECT_INVIS;
                af.aff_index = 1;
            } else {
                af.bitvector = AFF2_TRUE_SEEING;
                af.aff_index = 2;
            }
            af.duration = af.level / 5;
            af.owner = GET_IDNUM(ch);
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_AWARENESS);
            return;
        }
    }
    // motion
    if (!affected_by_spell(ch, ZEN_MOTION)
        && CHECK_SKILL(ch, ZEN_MOTION) >= LEARNED(ch)) {
        int target = MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_MOTION) / 4);
        int test =
            (mag_manacost(ch, ZEN_MOTION) + number(10, 40) - GET_WIS(ch));

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Motion   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch, "You have attained the zen of motion.\r\n");
            af.type = ZEN_MOTION;
            af.duration = af.level / 4;
            af.owner = GET_IDNUM(ch);
            // make them able to fly at high power
            if (GET_CLASS(ch) == CLASS_MONK
                && skill_bonus(ch, ZEN_MOTION) >= 50) {
                af.bitvector = AFF_INFLIGHT;
                af.aff_index = 1;
            }
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_MOTION);

            return;
        }
    }
    // translocation
    if (!affected_by_spell(ch, ZEN_TRANSLOCATION)
        && CHECK_SKILL(ch, ZEN_TRANSLOCATION) >= LEARNED(ch)) {

        int target =
            MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_TRANSLOCATION) / 4);
        int test = number(20, 25);

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Translocation   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch,
                "You have achieved the zen of translocation.\r\n");
            af.type = ZEN_TRANSLOCATION;
            af.duration = af.level / 4;
            af.owner = GET_IDNUM(ch);
            affect_to_char(ch, &af);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_TRANSLOCATION);
            return;
        }
    }
    // celerity
    if (!affected_by_spell(ch, ZEN_CELERITY)
        && CHECK_SKILL(ch, ZEN_CELERITY) >= LEARNED(ch)) {
        int target = MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_CELERITY) / 4);
        int test = number(20, 25);

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Celerity   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch, "You have reached the zen of celerity.\r\n");
            af.type = ZEN_CELERITY;
            af.duration = af.level / 5;
            af.location = APPLY_SPEED;
            af.modifier = af.level / 4;
            af.owner = GET_IDNUM(ch);
            affect_to_char(ch, &af);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_CELERITY);
            return;
        }
    }

    if (!affected_by_spell(ch, ZEN_DISPASSION)
        && CHECK_SKILL(ch, ZEN_DISPASSION) >= LEARNED(ch)) {
        int target =
            MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_DISPASSION) / 4);
        int test = number(30, 50) - GET_WIS(ch) / 4;

        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch,
                "%s[MEDITATE] Dispassion   test:%d   target:%d%s\r\n",
                CCCYN(ch, C_NRM), test, target, CCNRM(ch, C_NRM));

        if (target > test) {
            send_to_char(ch, "You have grasped the zen of dispassion.\r\n");
            af.type = ZEN_DISPASSION;
            af.duration = af.level / 5;
            af.owner = GET_IDNUM(ch);
            affect_to_char(ch, &af);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_DISPASSION);
            return;
        }
    }
}

//
// whirlwind attack
//

ACMD(do_whirlwind)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent = 0, prob = 0, i;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg) {
        vict = random_opponent(ch);
    } else {
        vict = get_char_room_vis(ch, arg);
        ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents);
    }

    //do we have a target?
    if (!vict && !ovict) {
        send_to_char(ch, "Whirlwind who?\r\n");
        WAIT_STATE(ch, 4);
        return;
    }
    //hit an object
    if (ovict && !vict) {
        act("You whirl through the air hitting $p!", false, ch, ovict, NULL,
            TO_CHAR);
        act("$n whirls through the air hitting $p!", false, ch, ovict, NULL,
            TO_ROOM);
        return;
    }
    //can we perform an attack?
    if (vict == ch) {
        send_to_char(ch, "Aren't we funny today...\r\n");
        return;
    }
    if (!ok_to_attack(ch, vict, true))
        return;
    if (GET_MOVE(ch) < 28) {
        send_to_char(ch, "You are too exhausted!\r\n");
        return;
    }
    //101% is a complete failure
    percent = ((40 - (GET_AC(vict) / 10)) / 2) + number(1, 86);

    //adjust for equipment
    for (i = 0; i < NUM_WEARS; i++) {
        struct obj_data *obj = GET_EQ(ch, i);
        if (obj == NULL)
            continue;
        if (IS_OBJ_TYPE(obj, ITEM_ARMOR)) {
            if (IS_METAL_TYPE(obj) || IS_STONE_TYPE(obj) || IS_WOOD_TYPE(obj)) {
                percent += GET_OBJ_WEIGHT(obj);
            }
        }
    }

    //adjust for worn wield
    if (GET_EQ(ch, WEAR_WIELD))
        percent += (LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) / 2;

    //check skill and stat modifiers for prob
    prob =
        CHECK_SKILL(ch, SKILL_WHIRLWIND) + ((GET_DEX(ch) + GET_STR(ch)) / 2);

    //adjust prob based on victims position
    if (GET_POSITION(vict) < POS_STANDING)
        prob += 30;
    else
        prob -= GET_DEX(vict);

    //if alignment is messed up adjust
    if (IS_MONK(ch) && !IS_NEUTRAL(ch))
        prob -= (prob * (ABS(GET_ALIGNMENT(ch)))) / 1000;

    //if we don't have kata and victim is of a non-solid nature we can't hit
    if (!affected_by_spell(ch, SKILL_KATA) &&
        (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
        )
        prob = 0;

    //do we fail outright?
    if (percent > prob) {
        damage(ch, vict, NULL, 0, SKILL_WHIRLWIND, -1);
        //do we fall down too?
        if (GET_LEVEL(ch) + GET_DEX(ch) < number(0, 77)) {
            send_to_char(ch, "You fall on your ass!\r\n");
            act("$n falls smack on $s ass!\r\n", true, ch, NULL, NULL, TO_ROOM);
            GET_POSITION(ch) = POS_SITTING;
        }
        GET_MOVE(ch) -= 5;
    } else {
        //lets kick ass

        //how many times will we hit?
        int hits = 0;
        if (GET_CLASS(ch) == CLASS_MONK) {
            hits = number(3, 6);
        } else {
            hits = number(3, 4);
        }

        //attack our chosen victim first
        int dam = 0;
        if (CHECK_SKILL(ch, SKILL_WHIRLWIND) > number(40, 80) + GET_DEX(vict)) {
            dam = dice(GET_LEVEL(ch), 5) + GET_DAMROLL(ch);
        }

        damage(ch, vict, NULL, dam, SKILL_WHIRLWIND, -1);
        if (is_dead(ch))
            return;

        GET_MOVE(ch) -= 3;

        //attack up to hits-1 more victims at random
        int i = 1;

        for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
            struct creature *tch = it->data;

            if (g_list_find(tch->fighting, ch) && random_percentage() <= 75) {
                dam = 0;
                if (CHECK_SKILL(ch, SKILL_WHIRLWIND) > number(40,
                        80) + GET_DEX(tch))
                    dam = dice(GET_LEVEL(ch), 5) + GET_DAMROLL(ch);
                damage(ch, tch, NULL, dam, SKILL_WHIRLWIND, -1);
                if (is_dead(ch))
                    return;

                i++;
                GET_MOVE(ch) -= 3;
            }
        }

        //if we still haven't attacked hits times send the rest of them too
        //our initially chosen victim as long as he's alive
        if (!is_dead(vict)) {
            for (; i < hits; i++) {
                if (vict && vict->in_room == ch->in_room) {
                    dam = 0;
                    if (CHECK_SKILL(ch, SKILL_WHIRLWIND) > number(40,
                            80) + GET_DEX(vict)) {
                        dam = dice(GET_LEVEL(ch), 5) + GET_DAMROLL(ch);
                    }
                    GET_MOVE(ch) -= 3;
                    damage(ch, vict, NULL, dam, SKILL_WHIRLWIND, -1);
                    if (is_dead(ch))
                        return;
                    if (is_dead(vict))
                        break;
                }
            }
        }

        gain_skill_prof(ch, SKILL_WHIRLWIND);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

//
// combo attack
//

#define HOW_MANY    19
ACMD(do_combo)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent = 0, prob = 0, count = 0, i, dam = 0;
    int dead = 0;
    char *arg;

    const int which_attack[] = {
        SKILL_PALM_STRIKE,
        SKILL_THROAT_STRIKE,
        SKILL_SCISSOR_KICK,
        SKILL_CRANE_KICK,
        SKILL_KICK,
        SKILL_ROUNDHOUSE,
        SKILL_SIDEKICK,
        SKILL_SPINKICK,
        SKILL_TORNADO_KICK,
        SKILL_PUNCH,
        SKILL_KNEE,
        SKILL_SPINFIST,
        SKILL_JAB,
        SKILL_HOOK,
        SKILL_UPPERCUT,
        SKILL_LUNGE_PUNCH,
        SKILL_ELBOW,
        SKILL_HEADBUTT,
        SKILL_GROINKICK
    };

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You perform a devastating combo to the $p!", false, ch, ovict,
                NULL, TO_CHAR);
            act("$n performs a devastating combo on the $p!", false, ch, ovict,
                NULL, TO_ROOM);
            return;
        } else {
            send_to_char(ch, "Combo who?\r\n");
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

    if (GET_MOVE(ch) < 48) {
        send_to_char(ch, "You are too exhausted!\r\n");
        return;
    }

    percent = ((40 - (GET_AC(vict) / 10)) / 2) + number(1, 86);    /* 101% is a complete
                                                                     * failure */
    for (i = 0; i < NUM_WEARS; i++) {
        struct obj_data *obj = GET_EQ(ch, i);
        if (obj == NULL)
            continue;
        if (IS_OBJ_TYPE(obj, ITEM_ARMOR)) {
            if (IS_METAL_TYPE(obj) || IS_STONE_TYPE(obj) || IS_WOOD_TYPE(obj)) {
                percent += GET_OBJ_WEIGHT(obj);
            }
        }
    }

    if (GET_EQ(ch, WEAR_WIELD))
        percent += (LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) / 2;

    prob = CHECK_SKILL(ch, SKILL_COMBO) + ((GET_DEX(ch) + GET_STR(ch)) / 2);

    if (GET_POSITION(vict) < POS_STANDING)
        prob += 30;
    else
        prob -= GET_DEX(vict);

    if (IS_MONK(ch) && !IS_NEUTRAL(ch))
        prob -= (prob * (ABS(GET_ALIGNMENT(ch)))) / 1000;

    //if we don't have kata and victim is of a non-solid nature we can't hit
    if (!affected_by_spell(ch, SKILL_KATA) &&
        (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_MOB(vict))
        )
        prob = 0;

    dam = dice(6, GET_LEVEL(ch) / 2) + GET_DAMROLL(ch);
    dam = (dam * CHECK_SKILL(ch, SKILL_COMBO)) / 100;
    GET_MOVE(ch) -= 20;

    //
    // failure
    //

    if (percent > prob) {
        WAIT_STATE(ch, 4 RL_SEC);
        damage(ch, vict, NULL, 0, which_attack[number(0, HOW_MANY - 1)], -1);
        return;
    }
    //
    // success
    //

    else {
        gain_skill_prof(ch, SKILL_COMBO);

        //
        // lead with a throat strike
        //


        if (is_dead(ch))
            return;

        if (is_dead(vict)) {
            WAIT_STATE(ch, (1 + count) RL_SEC);
            return;
        }

        //
        // try to throw up to 8 more attacks
        //

        for (i = 0, count = 0; i < 8 && !dead && vict->in_room == ch->in_room;
            i++, count++) {
            if (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_COMBO) > number(100,
                    120 + (count * 8))) {
                damage(ch, vict, NULL, dam + (count * 8), which_attack[number(0,
                            HOW_MANY - 1)], -1);
                if (is_dead(ch))
                    return;
                if (is_dead(vict)) {
                    WAIT_STATE(ch, (1 + count) RL_SEC);
                    return;
                }
            }
        }
        WAIT_STATE(ch, (1 + count) RL_SEC);
    }
}

//
// nerve pinches
//

ACMD(do_pinch)
{
    struct affected_type af;
    struct obj_data *ovict = NULL;
    struct creature *vict = NULL;
    int prob, percent, which_pinch, i;
    char *pinch_str, *vict_str;
    const char *to_vict = NULL, *to_room = NULL;
    bool happened;

    init_affect(&af);

    pinch_str = tmp_getword(&argument);
    vict_str = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, vict_str))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict = get_obj_in_list_vis(ch, vict_str,
                    ch->in_room->contents))) {
            act("You can't pinch that.", false, ch, ovict, NULL, TO_CHAR);
            return;
        } else {
            send_to_char(ch, "Pinch who?\r\n");
            WAIT_STATE(ch, 4);
            return;
        }
    }
    if (vict == ch) {
        send_to_char(ch,
            "Using this skill on yourself is probably a bad idea...\r\n");
        return;
    }
    if (GET_EQ(ch, WEAR_WIELD)) {
        if (IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
            send_to_char(ch,
                "You are using both hands to wield your weapon right now!\r\n");
            return;
        } else if (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_WIELD_2)) {
            send_to_char(ch,
                "You need at least one hand free to do that!\r\n");
            return;
        }
    }

    if (is_abbrev(pinch_str, "alpha"))
        which_pinch = SKILL_PINCH_ALPHA;
    else if (is_abbrev(pinch_str, "beta"))
        which_pinch = SKILL_PINCH_BETA;
    else if (is_abbrev(pinch_str, "gamma"))
        which_pinch = SKILL_PINCH_GAMMA;
    else if (is_abbrev(pinch_str, "delta"))
        which_pinch = SKILL_PINCH_DELTA;
    else if (is_abbrev(pinch_str, "epsilon"))
        which_pinch = SKILL_PINCH_EPSILON;
    else if (is_abbrev(pinch_str, "omega"))
        which_pinch = SKILL_PINCH_OMEGA;
    else if (is_abbrev(pinch_str, "zeta"))
        which_pinch = SKILL_PINCH_ZETA;
    else {
        send_to_char(ch, "You know of no such nerve.\r\n");
        return;
    }

    if (which_pinch != SKILL_PINCH_ZETA && !ok_to_attack(ch, vict, true))
        return;

    if (!CHECK_SKILL(ch, which_pinch)) {
        send_to_char(ch,
            "You have absolutely no idea how to perform this pinch.\r\n");
        return;
    }

    percent = number(1, 101) + GET_LEVEL(vict);
    prob =
        CHECK_SKILL(ch,
        which_pinch) + GET_LEVEL(ch) + GET_DEX(ch) + GET_HITROLL(ch);
    prob += (GET_AC(vict) / 5);

    for (i = 0; i < NUM_WEARS; i++)
        if ((ovict = GET_EQ(ch, i)) && IS_OBJ_TYPE(ovict, ITEM_ARMOR) &&
            (IS_METAL_TYPE(ovict) || IS_STONE_TYPE(ovict) ||
                IS_WOOD_TYPE(ovict)))
            percent += GET_OBJ_WEIGHT(ovict);

    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    if (!IS_NPC(vict) && !vict->desc)
        prob = 0;

    act("$n grabs a nerve on your body!", false, ch, NULL, vict, TO_VICT);
    act("$n suddenly grabs $N!", false, ch, NULL, vict, TO_NOTVICT);

    if (percent > prob || IS_UNDEAD(vict)) {
        send_to_char(ch, "You fail the pinch!\r\n");
        act("You quickly shake off $n's attack!", false, ch, NULL, vict, TO_VICT);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (IS_NPC(vict) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
            hit(vict, ch, TYPE_UNDEFINED);
        return;
    }

    af.type = which_pinch;
    af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
    af.owner = GET_IDNUM(ch);

    switch (which_pinch) {
    case SKILL_PINCH_ALPHA:
        af.duration = 3;
        af.location = APPLY_HITROLL;
        af.modifier = -(2 + GET_LEVEL(ch) / 15);
        to_vict = "You suddenly feel very inept!";
        to_room = "$n gets a strange look on $s face.";
        break;
    case SKILL_PINCH_BETA:
        af.duration = 3;
        af.location = APPLY_DAMROLL;
        af.modifier = -(2 + GET_LEVEL(ch) / 15);
        to_vict = "You suddenly feel very inept!";
        to_room = "$n gets a strange look on $s face.";
        break;
    case SKILL_PINCH_DELTA:
        af.duration = 3;
        af.location = APPLY_DEX;
        af.modifier = -(2 + GET_LEVEL(ch) / 15);
        to_vict = "You begin to feel very clumsy!";
        to_room = "$n stumbles across the floor.";
        break;
    case SKILL_PINCH_EPSILON:
        af.duration = 3;
        af.location = APPLY_STR;
        af.modifier = -(2 + GET_LEVEL(ch) / 15);
        to_vict = "You suddenly feel very weak!";
        to_room = "$n staggers weakly.";
        break;
    case SKILL_PINCH_OMEGA:
        if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
            act("$N stuns you with a swift blow!", false, ch, NULL, vict,
                TO_CHAR);
            act("$N stuns $n with a swift blow to the neck!", false, ch, NULL,
                vict, TO_ROOM);
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            GET_POSITION(ch) = POS_STUNNED;
            return;
        }
        if (is_fighting(ch) || is_fighting(vict) || NPC2_FLAGGED(vict, NPC2_NOSTUN)
            || (AFF_FLAGGED(vict, AFF_ADRENALINE)
                && number(0, 60) < GET_LEVEL(vict))) {
            send_to_char(ch, "You fail.\r\n");
            send_to_char(vict, "%s", NOEFFECT);
            return;
        }
        GET_POSITION(vict) = POS_STUNNED;
        WAIT_STATE(vict, PULSE_VIOLENCE * 3);
        to_vict = "You feel completely stunned!";
        to_room = "$n gets a stunned look on $s face and falls over.";
        af.type = 0;
        break;
    case SKILL_PINCH_GAMMA:
        if (is_fighting(ch) || is_fighting(vict)) {
            send_to_char(ch, "You fail.\r\n");
            send_to_char(vict, "%s", NOEFFECT);
            return;
        }
        af.bitvector = AFF_CONFUSION;
        af.duration = (1 + (GET_LEVEL(ch) / 24));
        remember(vict, ch);
        to_vict = "You suddenly feel very confused.";
        to_room = "$n suddenly becomes confused and stumbles around.";
        break;
    case SKILL_PINCH_ZETA:
        happened = false;
        af.type = 0;
        // Remove all biological affects
        if (vict->affected) {
            struct affected_type *doomed_aff, *next_aff;
            int level;

            level = GET_LEVEL(ch) + (GET_REMORT_GEN(ch) * 2);
            for (doomed_aff = vict->affected; doomed_aff;
                doomed_aff = next_aff) {
                next_aff = doomed_aff->next;
                if (SPELL_IS_BIO(doomed_aff->type)) {
                    if (doomed_aff->level < number(level / 2, level * 2)) {
                        affect_remove(vict, doomed_aff);
                        happened = true;
                    }
                }
            }
        }
        if (happened) {
            to_vict = "You feel hidden tensions fade.";
            to_room = "$n looks noticably more relaxed.";
        }

        if (GET_POSITION(vict) == POS_STUNNED
            || GET_POSITION(vict) == POS_SLEEPING) {
            // Revive from sleeping
            REMOVE_BIT(AFF_FLAGS(vict), AFF_SLEEP);
            // Wake them up
            GET_POSITION(vict) = POS_RESTING;
            // stun also has a wait-state which must be removed
            if (ch->desc)
                ch->desc->wait = 0;
            else if (IS_NPC(ch))
                GET_NPC_WAIT(ch) = 0;
            to_vict = "You feel a strange sensation as $N wakes you.";
            to_room = "$n is revived.";
            happened = true;
        }

        if (!happened) {
            to_room = NOEFFECT;
            to_vict = NOEFFECT;
        }
        break;
    default:
        af.type = 0;
        to_room = NOEFFECT;
        to_vict = NOEFFECT;
    }

    if (affected_by_spell(vict, which_pinch)) {
        send_to_char(ch, "%s", NOEFFECT);
        send_to_char(vict, "%s", NOEFFECT);
        return;
    }
    if (af.type)
        affect_to_char(vict, &af);

    if (to_vict)
        act(to_vict, false, vict, NULL, ch, TO_CHAR);
    if (to_room)
        act(to_room, false, vict, NULL, ch, TO_ROOM);

    gain_skill_prof(ch, which_pinch);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);

    //
    // NOTE: pinches always make it through these magical protections before
    // the monk feels the effect
    //

    //
    // victim has fire on or around him
    //

    if (!CHAR_WITHSTANDS_FIRE(ch) && !AFF2_FLAGGED(ch, AFF2_ABLAZE) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {

        //
        // victim has fire shield
        //

        if (AFF2_FLAGGED(vict, AFF2_FIRE_SHIELD)) {
            damage(vict, ch, NULL, dice(3, 8), SPELL_FIRE_SHIELD, -1);
            if (is_dead(ch))
                return;

            ignite_creature(ch, vict);
        }
        //
        // victim is simply on fire
        //

        else if (AFF2_FLAGGED(vict, AFF2_ABLAZE) && !number(0, 3)) {
            act("You burst into flames on contact with $N!",
                false, ch, NULL, vict, TO_CHAR);
            act("$n bursts into flames on contact with $N!",
                false, ch, NULL, vict, TO_NOTVICT);
            act("$n bursts into flames on contact with you!",
                false, ch, NULL, vict, TO_VICT);

            ignite_creature(ch, vict);
        }
    }
    //
    // victim has blade barrier
    //

    if (AFF2_FLAGGED(vict, AFF2_BLADE_BARRIER)) {
        damage(vict, ch, NULL, GET_LEVEL(vict), SPELL_BLADE_BARRIER, -1);
    }
    //
    // the victim should attack the monk if they can
    //

    if (which_pinch != SKILL_PINCH_ZETA) {
        check_attack(ch, vict);
        if (IS_NPC(vict) && !is_fighting(vict)
            && GET_POSITION(vict) >= POS_FIGHTING) {
            hit(vict, ch, TYPE_UNDEFINED);
        }
    }

}

ACMD(do_meditate)
{
    int8_t percent;

    if (AFF2_FLAGGED(ch, AFF2_MEDITATE)) {
        REMOVE_BIT(AFF2_FLAGS(ch), AFF2_MEDITATE);
        send_to_char(ch, "You cease to meditate.\r\n");
        act("$n comes out of a trance.", true, ch, NULL, NULL, TO_ROOM);
        MEDITATE_TIMER(ch) = 0;
    } else if (is_fighting(ch))
        send_to_char(ch, "You cannot meditate while in battle.\r\n");
    else if (GET_POSITION(ch) != POS_SITTING || !AWAKE(ch))
        send_to_char(ch,
            "You are not in the proper position to meditate.\r\n");
    else if (AFF_FLAGGED(ch, AFF_POISON))
        send_to_char(ch, "You cannot meditate while you are poisoned!\r\n");
    else if (AFF2_FLAGGED(ch, AFF2_BERSERK))
        send_to_char(ch, "You cannot meditate while BERSERK!\r\n");
    else {
        send_to_char(ch, "You begin to meditate.\r\n");
        MEDITATE_TIMER(ch) = 0;
        if (CHECK_SKILL(ch, ZEN_HEALING) > number(40, 140))
            send_to_char(ch, "You have attained the zen of healing.\r\n");

        percent = number(1, 101);   /* 101% is a complete failure */

        if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.75)
            && GET_LEVEL(ch) < LVL_AMBASSADOR) {
            send_to_char(ch,
                "You find it difficult with all your heavy equipment.\r\n");
            percent += 30;
        }
        if (GET_COND(ch, DRUNK) > 5)
            percent += 20;
        if (GET_COND(ch, DRUNK) > 15)
            percent += 20;
        percent -= GET_WIS(ch);

        if (CHECK_SKILL(ch, SKILL_MEDITATE) > percent)
            SET_BIT(AFF2_FLAGS(ch), AFF2_MEDITATE);

        if (GET_LEVEL(ch) < LVL_AMBASSADOR)
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        act("$n goes into a trance.", true, ch, NULL, NULL, TO_ROOM);
    }
}

ACMD(do_kata)
{
    struct affected_type af;

    init_affect(&af);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR &&
        (CHECK_SKILL(ch, SKILL_KATA) < 50 || !IS_MONK(ch)))
        send_to_char(ch, "You have not learned any kata.\r\n");
    else if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 2))
        send_to_char(ch, "You are too wounded to perform the kata.\r\n");
    else if (GET_MANA(ch) < 40)
        send_to_char(ch,
            "You do not have the spiritual energy to do this.\r\n");
    else if (GET_MOVE(ch) < 10)
        send_to_char(ch, "You are too spiritually exhausted.\r\n");
    else {
        send_to_char(ch, "You carefully perform your finest kata.\r\n");
        act("$n begins to perform a kata with fluid motions.", true, ch, NULL, NULL,
            TO_ROOM);

        GET_MANA(ch) -= 40;
        GET_MOVE(ch) -= 10;
        WAIT_STATE(ch, PULSE_VIOLENCE * (GET_LEVEL(ch) / 12));

        if (affected_by_spell(ch, SKILL_KATA) || !IS_NEUTRAL(ch))
            return;

        af.type = SKILL_KATA;
        af.duration = 1 + (GET_LEVEL(ch) / 12);
        af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
        af.owner = GET_IDNUM(ch);

        af.location = APPLY_HITROLL;
        af.modifier = 1 + (GET_LEVEL(ch) / 6) + (GET_REMORT_GEN(ch) * 2) / 4;
        affect_to_char(ch, &af);

        af.location = APPLY_DAMROLL;
        af.modifier = 1 + (GET_LEVEL(ch) / 12) + (GET_REMORT_GEN(ch) * 2) / 4;
        affect_to_char(ch, &af);
    }
}

ACMD(do_evade)
{

    int prob, percent;
    struct obj_data *obj = NULL;

    send_to_char(ch, "You attempt to evade all attacks.\r\n");

    prob = CHECK_SKILL(ch, SKILL_EVASION);

    if ((obj = GET_EQ(ch, WEAR_BODY)) && IS_OBJ_TYPE(obj, ITEM_ARMOR)) {
        prob -= GET_OBJ_WEIGHT(obj) / 2;
        prob -= GET_OBJ_VAL(obj, 0) * 3;
    }
    if ((obj = GET_EQ(ch, WEAR_LEGS)) && IS_OBJ_TYPE(obj, ITEM_ARMOR)) {
        prob -= GET_OBJ_WEIGHT(obj);
        prob -= GET_OBJ_VAL(obj, 0) * 2;
    }
    if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.6))
        prob -= (10 + IS_WEARING_W(ch) / 8);

    prob += GET_DEX(ch);

    percent = number(0, 101) - (GET_LEVEL(ch) / 4);

    if (percent < prob)
        SET_BIT(AFF2_FLAGS(ch), AFF2_EVADE);

}
