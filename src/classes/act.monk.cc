//
// File: act.monk.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
//   File: act.monk.c                Created for TempusMUD by Fireball
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "house.h"
#include "char_class.h"
#include "fight.h"
#include "shop.h"

void appear(struct char_data *ch, struct char_data *vict);

void perform_monk_meditate(struct char_data *ch)
{
    struct affected_type af;

    af.level = GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 2);
    MEDITATE_TIMER(ch)++;
    
    // oblivity
    if ( !IS_AFFECTED_2(ch, AFF2_OBLIVITY) && CHECK_SKILL(ch, ZEN_OBLIVITY) >= LEARNED(ch) ) {
        if (MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_OBLIVITY) >> 2) + GET_WIS(ch)> 
            (mag_manacost(ch, ZEN_OBLIVITY) + number(20, 40))) {
            send_to_char("You begin to perceive the zen of oblivity.\r\n", ch);
            af.type = ZEN_OBLIVITY;
        af.is_instant = 0;
            af.bitvector = AFF2_OBLIVITY;
            af.aff_index     = 2;
            af.duration = af.level / 5;
            af.location = APPLY_NONE;
            af.modifier = 0;
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            gain_skill_prof(ch, ZEN_OBLIVITY);
            return;
        }
    } 
      
    // awareness
    if (!affected_by_spell(ch, ZEN_AWARENESS) && 
        CHECK_SKILL(ch, ZEN_AWARENESS) >= LEARNED(ch)) {
        if (MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_AWARENESS) >> 2) > 
            (mag_manacost(ch, ZEN_AWARENESS) + number(6, 40) - GET_WIS(ch))) {
            send_to_char("You become one with the zen of awareness.\r\n", ch);
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
            af.modifier = 0;
            af.location = APPLY_NONE;
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_AWARENESS);
            return;
        }
    } 

    // motion
    if (!affected_by_spell(ch, ZEN_MOTION) && 
        CHECK_SKILL(ch, ZEN_MOTION) >= LEARNED(ch)) {
        if (MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_MOTION) >> 2)> 
            (mag_manacost(ch, ZEN_MOTION) + number(10, 40) - GET_WIS(ch))) {
            send_to_char("You have attained the zen of motion.\r\n", ch);
            af.type = ZEN_MOTION;
            af.bitvector = 0;
            af.aff_index = 0;
            af.duration = af.level / 4;
            af.location = APPLY_NONE;
            af.modifier = 0;
            affect_to_char(ch, &af);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_MOTION);
            return;
        }
    } 
    // translocation
    if (!affected_by_spell(ch, ZEN_TRANSLOCATION) && 
        CHECK_SKILL(ch, ZEN_TRANSLOCATION) >= LEARNED(ch)) {
        if (MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_TRANSLOCATION) >> 2)> 
            number(20, 25)) {
            send_to_char("You have achieved the zen of translocation.\r\n", ch);
            af.type = ZEN_TRANSLOCATION;
            af.bitvector = 0;
            af.aff_index = 0;
            af.duration = af.level / 4;
            af.location = APPLY_NONE;
            af.modifier = 0;
            affect_to_char(ch, &af);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_TRANSLOCATION);
            return;
        }
    }

    // celerity
    if ( !affected_by_spell(ch, ZEN_CELERITY) && CHECK_SKILL(ch, ZEN_CELERITY) >= LEARNED(ch)) {
        if (MEDITATE_TIMER(ch) + (CHECK_SKILL(ch, ZEN_CELERITY) >> 2)> 
            number(20, 25)) {
            send_to_char("You have reached the zen of celerity.\r\n", ch);
            af.type = ZEN_CELERITY;
            af.bitvector = 0;
            af.aff_index = 0;
            af.duration = af.level / 5;
            af.location = APPLY_SPEED;
            af.modifier = af.level / 4;
            affect_to_char(ch, &af);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            gain_skill_prof(ch, ZEN_CELERITY);
            return;
        }
    }

}   

//
// whirlwind attack
//

ACMD(do_whirlwind)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL;
    int  percent = 0, prob = 0, i;
    bool all = false;

    one_argument(argument, arg);

    if ((!*arg && (all = true) && !(vict = ch->getFighting())) ||
        (*arg && !(all = false) && !(vict = get_char_room_vis(ch, arg)) &&
         !(ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents)))) {
        send_to_char("Whirlwind who?\r\n", ch);
        WAIT_STATE(ch, 4);
        return;
    }

    if (ovict) {
        act("You whirl through the air hitting $p!",FALSE,ch,ovict, 0, TO_CHAR);
        act("$n whirls through the air hitting $p!",FALSE,ch,ovict, 0, TO_ROOM);
        return;
    }

    if (vict == ch) {
        send_to_char("Aren't we funny today...\r\n", ch);
        return;
    }
    if (!peaceful_room_ok(ch, vict, true))
        return;
    if (GET_MOVE(ch) < 28) {
        send_to_char("You are too exhausted!\r\n", ch);
        return;
    } 

    percent = ((40 - (GET_AC(vict) / 10)) >> 1) + number(1, 86); 

    for (i = 0; i < NUM_WEARS; i++)
        if ((ovict = GET_EQ(ch, i)) && GET_OBJ_TYPE(ovict) == ITEM_ARMOR &&
            (IS_METAL_TYPE(ovict) || IS_STONE_TYPE(ovict) || 
             IS_WOOD_TYPE(ovict)))
            percent += ovict->getWeight();

    if (GET_EQ(ch, WEAR_WIELD))
        percent += (LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) / 2;


    prob = CHECK_SKILL(ch, SKILL_WHIRLWIND) + ((GET_DEX(ch) + GET_STR(ch)) >> 1);
    if (vict->getPosition() < POS_RESTING)
        prob += 30;
    prob -= GET_DEX(vict);

    if (percent > prob)  {
        damage(ch, vict, 0, SKILL_WHIRLWIND, -1);
        if (GET_LEVEL(ch) + GET_DEX(ch) < number(0, 77)) {
            send_to_char("You fall on your ass!", ch);
            act("$n falls smack on $s ass!", TRUE, ch, 0, 0, TO_ROOM);
            ch->setPosition( POS_SITTING );
        }
        GET_MOVE(ch) -=10;
    }  else {
        if (!all) {
            if (!damage(ch, vict, dice(GET_LEVEL(ch), 6), SKILL_WHIRLWIND, -1)) {
                for (i = 0; i < 3; i++) {
                    GET_MOVE(ch) -= 7;
                    if (CHECK_SKILL(ch, SKILL_WHIRLWIND) > number(50, 50 + (10 << i))) {
                        if (damage(ch, vict, number(0, 1 + GET_LEVEL(ch)/10) ? 
                                   (dice(GET_LEVEL(ch), 4) + GET_DAMROLL(ch)) : 0,
                                   SKILL_WHIRLWIND,-1))
                            break;
                    }
                }
            } 
        }
        else {  /** hit all **/
      
            i = 0;  /* will get 4 hits total */
            CharacterList::iterator it = ch->in_room->people.begin();
            for (; it != ch->in_room->people.end(); ++it) {
                if ((*it)== ch || ch != (*it)->getFighting() || !CAN_SEE(ch, (*it)))
                    continue;
                damage(ch, (*it), (CHECK_SKILL(ch, SKILL_WHIRLWIND) > 
                                  number(50, 100) + GET_DEX((*it))) ?
                       (dice(GET_LEVEL(ch), 4) + GET_DAMROLL(ch)) : 0,
                       SKILL_WHIRLWIND,-1);
                i++;
            }

            if (i < 4 && (vict = ch->getFighting())) {
                for (; i < 4; i++) {
                    GET_MOVE(ch) -= 7;
                    if (CHECK_SKILL(ch, SKILL_WHIRLWIND) > number(50, 50 + (8 << i))) {
                        if (damage(ch, vict, number(0, 1 + GET_LEVEL(ch)/10) ? 
                                   (dice(GET_LEVEL(ch), 4) + GET_DAMROLL(ch)) : 0,
                                   SKILL_WHIRLWIND,-1))
                            break;
                    }
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
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL;
    int  percent = 0, prob = 0, count = 0, i, dam = 0;
    int dead = 0;
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

    ACMD_set_return_flags( 0 );
    one_argument(argument, arg);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (ch->isFighting()) {
            vict = ch->getFighting();
        } else if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You perform a devastating combo to the $p!", 
                FALSE, ch, ovict, 0, TO_CHAR);
            act("$n performs a devastating combo on the $p!", 
                FALSE, ch, ovict, 0, TO_ROOM);
            return;
        } else {
            send_to_char("Combo who?\r\n", ch);
            WAIT_STATE(ch, 4);
            return;
        }
    }
    if (vict == ch) {
        send_to_char("Aren't we funny today...\r\n", ch);
        return;
    }
    if (!peaceful_room_ok(ch, vict, true))
        return;

    if (GET_MOVE(ch) < 48) {
        send_to_char("You are too exhausted!\r\n", ch);
        return;
    } 

    percent = ((40 - (GET_AC(vict) / 10)) >> 1) + number(1, 86); /* 101% is a complete
                                                                  * failure */
    for (i = 0; i < NUM_WEARS; i++)
        if ((ovict = GET_EQ(ch, i)) && GET_OBJ_TYPE(ovict) == ITEM_ARMOR &&
            (IS_METAL_TYPE(ovict) || IS_STONE_TYPE(ovict) || 
             IS_WOOD_TYPE(ovict)))
            percent += ovict->getWeight();

    if (GET_EQ(ch, WEAR_WIELD))
        percent += (LEARNED(ch) - weapon_prof(ch, GET_EQ(ch, WEAR_WIELD))) / 2;


    prob = CHECK_SKILL(ch, SKILL_COMBO) + ((GET_DEX(ch) + GET_STR(ch)) >> 1);

    if (vict->getPosition() < POS_STANDING)
        prob += 30;
    else
        prob -= GET_DEX(vict);
  
    if (IS_MONK(ch) && !IS_NEUTRAL(ch))
        prob -= (prob * (ABS(GET_ALIGNMENT(ch)))) / 1000;

    if (IS_PUDDING(vict) || IS_SLIME(vict) || NON_CORPOREAL_UNDEAD(vict))
        prob = 0;
  
    if (NON_CORPOREAL_UNDEAD(vict))
        prob = 0;

    dam = dice(4, (GET_LEVEL(ch) + GET_REMORT_GEN(ch))) + 
        CHECK_SKILL(ch, SKILL_COMBO) - LEARNED(ch) + 
        GET_DAMROLL(ch);

    GET_MOVE(ch) -=20;

    //
    // failure
    //

    if (percent > prob)  {
        WAIT_STATE(ch,4 RL_SEC);
        int retval = damage(ch, vict, 0, which_attack[number(0, HOW_MANY - 1)], -1);
        ACMD_set_return_flags( retval );
        return;
    }  
    
    //
    // success
    //

    else {
        int retval = 0;

        gain_skill_prof(ch, SKILL_COMBO);
        
        //
        // lead with a throat strike
        //

        retval = damage(ch, vict, dam, SKILL_THROAT_STRIKE,WEAR_NECK_1);

        if ( retval ) {
            if ( !IS_SET( retval, DAM_ATTACKER_KILLED ) )
                WAIT_STATE(ch, (1 + count) RL_SEC);
            ACMD_set_return_flags( retval );
            return;
        }
        
        //
        // try to throw up to 8 more attacks
        //

        for (i = 0, count = 0; i < 8 && !dead && vict->in_room == ch->in_room; i++, count++) {
            if (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_COMBO) > number(100, 120 + (count << 3))) {
                    retval = damage(ch, vict, dam + (count << 3), which_attack[number(0, HOW_MANY -1)], -1);
                    if ( retval ) {
                        if ( !IS_SET( retval, DAM_ATTACKER_KILLED ) )
                            WAIT_STATE(ch, (1 + count) RL_SEC);
                        ACMD_set_return_flags( retval );
                        return;
                    }
            }
        }
        if ( !IS_SET( retval, DAM_ATTACKER_KILLED ) )
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
    struct char_data *vict = NULL;
    int prob, percent, which_pinch, i;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char *to_vict = NULL, *to_room = NULL;

    ACMD_set_return_flags( 0 );

    half_chop(argument, arg1, arg2);

    if (!(vict = get_char_room_vis(ch, arg1))) {
        if (ch->isFighting()) {
            vict = ch->getFighting();
        } else if ((ovict = get_obj_in_list_vis(ch, arg1, 
                                                ch->in_room->contents))) {
            act("You can't pinch that.", FALSE, ch, ovict, 0, TO_CHAR);
            return;
        } else {
            send_to_char("Pinch who?\r\n", ch);
            WAIT_STATE(ch, 4);
            return;
        }
    }
    if (vict == ch) {
        send_to_char("Using this skill on yourself is probably a bad idea...\r\n",
                     ch);
        return;
    }
    if (GET_EQ(ch, WEAR_WIELD)) {
        if (IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
            send_to_char("You are using both hands to wield your weapon right now!\r\n", ch);
            return;
        } else if (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_WIELD_2)) {
            send_to_char("You need at least one hand free to do that!\r\n", ch);
            return;
        }
    }

    if (!peaceful_room_ok(ch, vict, true))
        return;

    if (is_abbrev(arg2, "alpha"))
        which_pinch = SKILL_PINCH_ALPHA;
    else if (is_abbrev(arg2, "beta"))
        which_pinch = SKILL_PINCH_BETA;
    else if (is_abbrev(arg2, "gamma"))
        which_pinch = SKILL_PINCH_GAMMA;
    else if (is_abbrev(arg2, "delta"))
        which_pinch = SKILL_PINCH_DELTA;
    else if (is_abbrev(arg2, "epsilon"))
        which_pinch = SKILL_PINCH_EPSILON;
    else if (is_abbrev(arg2, "omega"))
        which_pinch = SKILL_PINCH_OMEGA;
    else if (is_abbrev(arg2, "zeta"))
        which_pinch = SKILL_PINCH_ZETA;
    else {
        send_to_char("You know of no such nerve.\r\n", ch);
        return;
    }
  
    if (!CHECK_SKILL(ch, which_pinch)) {
        send_to_char("You have absolutely no idea how to perform this pinch.\r\n", ch);
        return;
    }

    percent = number(1, 101) + GET_LEVEL(vict);
    prob = CHECK_SKILL(ch, which_pinch) + GET_LEVEL(ch) + GET_DEX(ch) + GET_HITROLL(ch);
    prob += (GET_AC(vict)/5);
 
    for (i = 0; i < NUM_WEARS; i++)
        if ((ovict = GET_EQ(ch, i)) && GET_OBJ_TYPE(ovict) == ITEM_ARMOR &&
            (IS_METAL_TYPE(ovict) || IS_STONE_TYPE(ovict) || 
             IS_WOOD_TYPE(ovict)))
            percent += ovict->getWeight();

    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    act("$n grabs a nerve on your body!", FALSE, ch, 0, vict, TO_VICT);
    act("$n suddenly grabs $N!", FALSE, ch, 0, vict, TO_NOTVICT);

    if (which_pinch != SKILL_PINCH_ZETA) {
        check_toughguy(ch, vict, 0);
        check_killer(ch, vict);
    }

    if (!IS_NPC(vict) && !vict->desc)
        prob = 0;

    if (percent > prob || IS_UNDEAD(vict)) {
        send_to_char("You fail the pinch!\r\n", ch);
        act("You quickly shake off $n's attack!", FALSE, ch, 0, vict, TO_VICT);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (IS_MOB(vict) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
            hit(vict, ch, TYPE_UNDEFINED);
        return;
    }

    af.type = which_pinch;  
    af.location = APPLY_NONE;
    af.is_instant = 0;
    af.modifier = 0;
    af.duration = 0;
    af.bitvector = 0;  
    af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

    switch (which_pinch) {
    case SKILL_PINCH_ALPHA:
        af.duration = 3;
        af.location = APPLY_HITROLL;
        af.modifier = - (2 + GET_LEVEL(ch)/15);
        to_vict = "You suddenly feel very inept!";
        to_room = "$n gets a strange look on $s face.";
        break;
    case SKILL_PINCH_BETA:
        af.duration = 3;
        af.location = APPLY_DAMROLL;
        af.modifier = - (2 + GET_LEVEL(ch)/15);
        to_vict = "You suddenly feel very inept!";
        to_room = "$n gets a strange look on $s face.";
        break;
    case SKILL_PINCH_DELTA:
        af.duration = 3;
        af.location = APPLY_DEX;
        af.modifier = - (2 + GET_LEVEL(ch)/15);
        to_vict = "You begin to feel very clumsy!";
        to_room = "$n stumbles across the floor.";
        break;
    case SKILL_PINCH_EPSILON:
        af.duration = 3;
        af.location = APPLY_STR;
        af.modifier = - (2 + GET_LEVEL(ch)/15);
        to_vict = "You suddenly feel very weak!";
        to_room = "$n staggers weakly.";
        break;
    case SKILL_PINCH_OMEGA:
        if (!ok_damage_shopkeeper(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
            act("$N stuns you with a swift blow!", FALSE, ch, 0, vict, TO_CHAR);
            act("$N stuns $n with a swift blow to the neck!", 
                FALSE, ch, 0, vict, TO_ROOM);
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            ch->setPosition( POS_STUNNED );
            return;
        }
        if (ch->isFighting() || vict->isFighting() || MOB2_FLAGGED(vict, MOB2_NOSTUN) ||
            (AFF_FLAGGED(vict, AFF_ADRENALINE) && 
             number(0, 60) < GET_LEVEL(vict))) {
            send_to_char("You fail.\r\n", ch);
            send_to_char(NOEFFECT, vict);
            return;
        }
        vict->setPosition( POS_STUNNED );
        WAIT_STATE(vict, PULSE_VIOLENCE * 3);
        to_vict = "You feel completely stunned!";
        to_room = "$n gets a stunned look on $s face and falls over.";
        af.type = 0;
        break;
    case SKILL_PINCH_GAMMA:
        if (ch->isFighting() || vict->isFighting()) {
            send_to_char("You fail.\r\n", ch);
            send_to_char(NOEFFECT, vict);
            return;
        }
        af.bitvector = AFF_CONFUSION;
        af.duration = (1 + (GET_LEVEL(ch) / 24));
        remember(vict, ch);
        to_vict = "You suddenly feel very confused.";
        to_room = "$n suddenly becomes confused and stumbles around.";
        break;
    case SKILL_PINCH_ZETA:
        if (vict->getPosition() == POS_STUNNED || vict->getPosition() == POS_SLEEPING) {
            REMOVE_BIT(AFF_FLAGS(vict), AFF_SLEEP);
            vict->setPosition( POS_RESTING );
            to_vict = "You feel a strange sensation as $N wakes you.";
            to_room = "$n is revived.";
        } else {
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
        send_to_char("Nothing seems to happen.\r\n", ch);
        send_to_char("Nothing seems to happen.\r\n", vict);
        return;
    }
    if (af.type)
        affect_to_char(vict, &af);

    if (to_vict)
        act(to_vict, FALSE, vict, 0, ch, TO_CHAR);
    if (to_room)
        act(to_room, FALSE, vict, 0, ch, TO_ROOM);


    gain_skill_prof(ch, which_pinch);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
        WAIT_STATE(ch, PULSE_VIOLENCE*2);

    //
    // NOTE: pinches always make it through these magical protections before
    // the monk feels the effect
    //
    
    //
    // victim has fire on or around him
    //

    if (!CHAR_WITHSTANDS_FIRE(ch) && !IS_AFFECTED_2(ch, AFF2_ABLAZE) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {
        
        //
        // victim has fire shield
        //

        if (IS_AFFECTED_2(vict, AFF2_FIRE_SHIELD)) {
            int retval = damage(vict, ch, dice(3, 8), SPELL_FIRE_SHIELD, -1);
            retval = SWAP_DAM_RETVAL( retval );
            
            ACMD_set_return_flags( retval );

            if ( retval )
                return;
            
            SET_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
            
        } 
        
        //
        // victim is simply on fire
        //

        else if (IS_AFFECTED_2(vict, AFF2_ABLAZE) && !number(0, 3)) {
            act("You burst into flames on contact with $N!", 
                FALSE, ch, 0, vict, TO_CHAR);
            act("$n bursts into flames on contact with $N!", 
                FALSE, ch, 0, vict, TO_NOTVICT);
            act("$n bursts into flames on contact with you!", 
                FALSE, ch, 0, vict, TO_VICT);
            SET_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
        }
    }

    //
    // victim has blade barrier
    //

    if (IS_AFFECTED_2(vict, AFF2_BLADE_BARRIER)) {
        int retval = damage(vict, ch, GET_LEVEL(vict), SPELL_BLADE_BARRIER, -1);
        retval = SWAP_DAM_RETVAL( retval );
        ACMD_set_return_flags( retval );
        if ( retval )
            return;
    }
    
    //
    // the victim should attack the monk if they can
    //

    if (IS_NPC(vict) && !vict->isFighting() && vict->getPosition() >= POS_FIGHTING) {
        int retval = hit(vict, ch, TYPE_UNDEFINED);
        retval = SWAP_DAM_RETVAL( retval );
        ACMD_set_return_flags( retval );
    }        
}

ACMD(do_meditate)
{
    byte percent;

    if (IS_AFFECTED_2(ch, AFF2_MEDITATE)) {
        REMOVE_BIT(AFF2_FLAGS(ch), AFF2_MEDITATE);
        send_to_char("You cease to meditate.\r\n", ch);
        act("$n comes out of a trance.", TRUE, ch, 0, 0, TO_ROOM);
        MEDITATE_TIMER(ch) = 0;
    } else if (FIGHTING(ch))
        send_to_char("You cannot meditate while in battle.\r\n", ch);
    else if (ch->getPosition() != POS_SITTING || !AWAKE(ch))
        send_to_char("You are not in the proper position to meditate.\r\n", ch);
    else if (IS_AFFECTED(ch, AFF_POISON))
        send_to_char("You cannot meditate while you are poisoned!\r\n", ch);
    else if (IS_AFFECTED_2(ch, AFF2_BESERK))
        send_to_char("You cannot meditate while BESERK!\r\n", ch);
    else {
        send_to_char("You begin to meditate.\r\n", ch);
        MEDITATE_TIMER(ch) = 0;
        if (CHECK_SKILL(ch, ZEN_HEALING) > number(40, 140))
            send_to_char("You have attained the zen of healing.\r\n", ch);

        percent = number(1, 101);        /* 101% is a complete failure */

        if (IS_WEARING_W(ch) >(CAN_CARRY_W(ch)*0.75) && GET_LEVEL(ch)<LVL_AMBASSADOR){
            send_to_char("You find it difficult with all your heavy equipment.\r\n", 
                         ch);
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
            WAIT_STATE(ch, PULSE_VIOLENCE*3);
        act("$n goes into a trance.", TRUE, ch, 0, 0, TO_ROOM);
    }
}

ACMD(do_kata)
{
    struct affected_type af;
  
    if (GET_LEVEL(ch) < LVL_AMBASSADOR && 
        (CHECK_SKILL(ch, SKILL_KATA) < 50 || !IS_MONK(ch)))
        send_to_char("You have not learned any kata.\r\n", ch);
    else if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 2))
        send_to_char("You are too wounded to perform the kata.\r\n", ch);
    else if (GET_MANA(ch) < 40)
        send_to_char("You do not have the spiritual energy to do this.\r\n", ch);
    else if (GET_MOVE(ch) < 10)
        send_to_char("You are too spiritually exhausted.\r\n", ch);
    else {
        send_to_char("You carefully perform your finest kata.\r\n", ch);
        act("$n begins to perform a kata with fluid motions.", TRUE, ch, 0, 0, TO_ROOM);
    
        GET_MANA(ch) -= 40;
        GET_MOVE(ch) -= 10;
        WAIT_STATE(ch, PULSE_VIOLENCE*(GET_LEVEL(ch) / 12));
    
        if (affected_by_spell(ch, SKILL_KATA) || !IS_NEUTRAL(ch))
            return;
    
        af.type = SKILL_KATA;
    af.is_instant =0;
        af.bitvector = 0;
        af.duration = 1 + (GET_LEVEL(ch) / 12);
        af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

        af.location = APPLY_HITROLL;
        af.modifier = 1 + (GET_LEVEL(ch) / 6) + (GET_REMORT_GEN(ch) * 2)/4;
        affect_to_char(ch, &af);

        af.location = APPLY_DAMROLL;
        af.modifier = 1 + (GET_LEVEL(ch) / 12) + (GET_REMORT_GEN(ch) * 2)/4;
        affect_to_char(ch, &af);
    }
}

ACMD(do_evade)
{
  
    int prob, percent;
    struct obj_data *obj = NULL;

    send_to_char("You attempt to evade all attacks.\r\n", ch);

    prob = CHECK_SKILL(ch, SKILL_EVASION);

    if ((obj = GET_EQ(ch, WEAR_BODY)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
        prob -= obj->getWeight() / 2;
        prob -= GET_OBJ_VAL(obj, 0)*3;
    }
    if ((obj = GET_EQ(ch, WEAR_LEGS)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
        prob -= obj->getWeight();
        prob -= GET_OBJ_VAL(obj, 0)*2;
    }
    if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch)*0.6))
        prob -= (10 + IS_WEARING_W(ch)/8);

    prob += GET_DEX(ch);
  
    percent = number(0, 101) - (GET_LEVEL(ch) >> 2);

    if (percent < prob)
        SET_BIT(AFF2_FLAGS(ch), AFF2_EVADE);

}



