//
// File: act.merc.c                     -- Part of TempusMUD
//
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

#define PISTOL(gun)  ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && !IS_TWO_HAND(gun))
#define LARGE_GUN(gun) ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && IS_TWO_HAND(gun))

int apply_soil_to_char(struct char_data *ch,struct obj_data *obj,int type,int pos);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);

ACMD(do_pistolwhip)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;

    one_argument(argument, arg);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (ch->isFighting()) {
            vict = ch->getFighting();
        } else if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You pistolwhip $p!", FALSE, ch, ovict, 0, TO_CHAR);
            return;
        } else {
            send_to_char("Pistolwhip who?\r\n", ch);
            return;
        }
    }
    if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && PISTOL(weap)) ||
    ((weap = GET_EQ(ch, WEAR_WIELD_2)) && PISTOL(weap)) ||
    ((weap = GET_EQ(ch, WEAR_HANDS)) && PISTOL(weap)))) {
        send_to_char("You need to be using a pistol.\r\n", ch);
        return;
    }
    if (vict == ch) {
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            act("You fear that your death will grieve $N.",
            FALSE, ch, 0, ch->master, TO_CHAR);
            return;
        }
        act("You slam $p into your head!", 
            FALSE, ch, weap, 0, TO_CHAR);
        act("$n beats $mself senseless with $p!", TRUE, ch, weap, 0, TO_ROOM);
        return;
    }
    if (!peaceful_room_ok(ch, vict, true))
        return;

    percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_PISTOLWHIP);

    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    cur_weap = weap;
    if (percent > prob) {
        damage(ch, vict, 0, SKILL_IMPALE, WEAR_BODY);
    } else {
        dam = dice(GET_LEVEL(ch), str_app[STRENGTH_APPLY_INDEX(ch)].todam) +
            dice(4, weap->getWeight() );
        dam /= 4;
        damage(ch, vict, dam, SKILL_PISTOLWHIP, WEAR_HEAD);
        gain_skill_prof(ch, SKILL_PISTOLWHIP);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_crossface)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL, *wear = NULL;
    int str_mod, dex_mod, percent = 0, prob = 0, dam = 0;
    int retval = 0, diff = 0, wear_num;
    bool prime_merc = false;
    short prev_pos = 0;
    
    if (GET_CLASS(ch) == CLASS_MERCENARY)
        prime_merc = true;
        
    one_argument(argument, arg);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (ch->getFighting()) {
            vict = ch->getFighting();
        } 
        else if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You fiercely crossface $p!", FALSE, ch, ovict, 0, TO_CHAR);
            return;
        } 
        else {
            send_to_char("Crossface who?\r\n", ch);
            return;
        }
    }
    
    if (!((weap = GET_EQ(ch, WEAR_WIELD)) && LARGE_GUN(weap))) { 
        send_to_char("You need to be using a two handed gun.\r\n", ch);
        return;
    }
    
    if (vict == ch) {
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            act("You fear that your death will grieve $N.",
                FALSE, ch, 0, ch->master, TO_CHAR);
            return;
        }
        act("You slam $p into your head!", 
            FALSE, ch, weap, 0, TO_CHAR);
        act("$n beats $mself senseless with $p!", TRUE, ch, weap, 0, TO_ROOM);
        return;
    }
    
    if (!peaceful_room_ok(ch, vict, true))
        return;

    if(!ok_damage_shopkeeper(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {    
        WAIT_STATE(ch, PULSE_VIOLENCE * 8);
       return;
    }

    cur_weap = weap;
    
    str_mod = 3;
    dex_mod = 3;
    // This beastly function brought to you by Cat, the letter F, and Nothing more
    prob = ((GET_LEVEL(ch) + ch->getLevelBonus(prime_merc)) - (GET_LEVEL(vict) * 2))
    + (CHECK_SKILL(ch, SKILL_CROSSFACE) >> 2)
    + (dex_mod * (GET_DEX(ch) - GET_DEX(vict)))
    + (str_mod * (GET_STR(ch) - GET_STR(vict)));
     percent = number( 1, 100);
    if(PRF2_FLAGGED(ch, PRF2_FIGHT_DEBUG)) {
        sprintf(buf,"Roll: %d, Chance: %d\r\n", prob, percent);
        send_to_char(buf,ch);
    }
    
    // You can't crossface pudding you fool!
    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;
    
    if (percent >= prob) {
        damage(ch, vict, 0, SKILL_CROSSFACE, WEAR_HEAD);
    } 
    else {
        
        dam = dice(GET_LEVEL(ch), str_app[STRENGTH_APPLY_INDEX(ch)].todam) +
              dice(9, weap->getWeight());
        
        if ((wear = GET_EQ(vict, WEAR_FACE)))
          wear_num = WEAR_FACE;
        else { 
          wear = GET_EQ(vict, WEAR_HEAD);
          wear_num = WEAR_HEAD;
        }
        
        diff = prob - percent;
        
        // Wow!  vict really took one hell of a shot.  Stun that bastard!
        if (diff >= 70 && !wear) {
            prev_pos = vict->getPosition();
            retval = damage(ch, vict, dam, SKILL_CROSSFACE, wear_num);
            if (prev_pos != POS_STUNNED && !IS_SET(retval, DAM_VICT_KILLED) &&
                !IS_SET(retval, DAM_ATTACKER_KILLED)) {
                if (!IS_NPC(vict) || (IS_NPC(vict) && 
                    !MOB2_FLAGGED(vict, MOB2_NOSTUN)) && ch->getFighting()) {
                    stop_fighting(ch);
                    stop_fighting(vict);
                    vict->setPosition(POS_STUNNED);
                    act("Your crossface has knocked $N senseless!", 
                        TRUE, ch, NULL, vict, TO_CHAR);
                    act("$n stuns $N with a vicious crossface!", 
                        TRUE, ch, NULL, vict, TO_ROOM);
                    act("Your jaw cracks as $n whips his gun across your face.  
                        Your vision fades...", 
                        TRUE, ch, NULL, vict, TO_VICT);
                }
            }
        }
        // vict just took a pretty good shot, knock him down...
        else if (diff >= 55) {
            dam = (int)(dam * 0.75);
            prev_pos = vict->getPosition();
            retval = damage(ch, vict, dam, SKILL_CROSSFACE, wear_num);
            if ((prev_pos != POS_RESTING  && prev_pos != POS_STUNNED) 
                && !IS_SET(retval, DAM_VICT_KILLED) && 
                !IS_SET(retval, DAM_ATTACKER_KILLED) && ch->getFighting()) {
                vict->setPosition(POS_RESTING);
                act("Your crossface has knocked $N on his ass!", 
                    TRUE, ch, NULL, vict, TO_CHAR);
                act("$n's nasty crossface just knocked $N on his ass!", 
                    TRUE, ch, NULL, vict, TO_ROOM);
                act("Your jaw cracks as $n whips his gun across your face.
                    You stagger and fall to the ground", 
                    TRUE, ch, NULL, vict, TO_VICT);
            }
        }
        // vict pretty much caught a grazing blow, knock off some eq
        else if (diff >= 20) {
            retval = damage(ch, vict, dam >> 1, SKILL_CROSSFACE, wear_num);
            if (wear && !IS_SET(retval, DAM_VICT_KILLED) && 
            !IS_SET(retval, DAM_ATTACKER_KILLED) && ch->getFighting()) {
                act("Your crossface has knocked $N's $p from his head!", 
                    TRUE, ch, wear, vict, TO_CHAR);
                act("$n's nasty crossface just knocked $p from $N's head!", 
                    TRUE, ch, wear, vict, TO_ROOM);
                act("Your jaw cracks as $n whips his gun across your face.
                    Your $p flies from your head and lands a short distance away.",
                    TRUE, ch, wear, vict, TO_VICT);

                damage_eq(vict, wear, dam >> 4);
                obj_to_room(unequip_char(vict, wear_num, MODE_EQ), vict->in_room);
                wear = NULL;    
            }
        }
        else
            retval = damage(ch, vict, dam >> 1, SKILL_CROSSFACE, wear_num);
       
        if (wear)
            damage_eq(vict, wear, dam >> 4);
        
        gain_skill_prof(ch, SKILL_CROSSFACE);
    }

    if (!IS_SET(retval, DAM_ATTACKER_KILLED))
        WAIT_STATE(ch, 3 RL_SEC);
    if (!IS_SET(retval, DAM_VICT_KILLED))
        WAIT_STATE(vict, 2 RL_SEC);
}

#define NOBEHEAD_EQ(obj) \
IS_SET(obj->obj_flags.bitvector[1], AFF2_NECK_PROTECTED)

// Sniper skill for mercs...I've tried to comment all of
// my thought processes.  Don't be too hard on me, this
// has been my first modification of anything functional

// --Nothing
ACMD(do_snipe)
{
  struct char_data *vict, *temp = NULL;
  struct obj_data *gun = NULL;
  struct affected_type af;
  obj_data *bullet, *armor;
  int retval, prob, percent, damage_loc, dam = 0;
  int snipe_dir = -1;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], buf[75];

  two_arguments(argument, arg1, arg2);


  // ch is blind?
  if(IS_AFFECTED(ch, AFF_BLIND) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
    send_to_char("You can't snipe anyone! you're blind!\r\n", ch);
    return;
  }
  //ch in smoky room?
  if(ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
    GET_LEVEL(ch) < LVL_AMBASSADOR) {
    send_to_char("The room is too smoky to see very far.\r\n", ch);
    return;
  }
  // ch wielding a rifle?
  if((gun = GET_EQ(ch, WEAR_WIELD))) {
     if(!IS_RIFLE(gun)) {
       if((gun = GET_EQ(ch, WEAR_WIELD_2))) {
         if(!IS_RIFLE(gun)) {
           send_to_char("But you aren't wielding a rifle!\r\n", ch);
           return;
         }
       }
       else {
         send_to_char("But you aren't wielding a rifle!\r\n", ch);
         return;
       }
     }
  }
  else {
    send_to_char("You aren't wielding anything fool!\r\n", ch);
    return;
  }
  // does ch have snipe leared at all?
  // I don't know if a skill can be less than 0
  // but I don't think it can hurt to check for it
  if(CHECK_SKILL(ch, SKILL_SNIPE) <= 0){
    send_to_char("You have no idea how!", ch);
  }
  // is ch's gun loaded?
  if(!GUN_LOADED(gun)) {
    send_to_char("But your gun isn't loaded!\r\n", ch);
    return;
  }
  //in what direction is ch attempting to snipe?
  if((snipe_dir = search_block(arg2, dirs, FALSE)) < 0) {
    send_to_char("Snipe in which direction?!\r\n", ch);
    return;
  }
  // is the victim in sight in that direction?
  // note:  line of sight stops at a DT or a closed door
  // and if the exit from a room leads to a NULL room we
  // have a serious problem...
  // This is a NASTY ass way to check for this...if anyone
  // can think of a better way to implement it, be my guest...
  if(EXIT(ch, snipe_dir) && EXIT(ch, snipe_dir)->to_room != NULL &&
    EXIT(ch, snipe_dir)->to_room != ch->in_room &&
     !ROOM_FLAGGED(EXIT(ch, snipe_dir)->to_room, ROOM_DEATH) &&
     !IS_SET(EXIT(ch, snipe_dir)->exit_info, EX_CLOSED)) {
    if(!(vict = get_char_in_remote_room_vis(ch, arg1, EXIT(ch, snipe_dir)->to_room))) {
      if(_2ND_EXIT(ch, snipe_dir) && _2ND_EXIT(ch, snipe_dir)->to_room != NULL &&
        _2ND_EXIT(ch, snipe_dir)->to_room != ch->in_room &&
        !ROOM_FLAGGED(_2ND_EXIT(ch, snipe_dir)->to_room, ROOM_DEATH) &&
        !IS_SET(_2ND_EXIT(ch, snipe_dir)->exit_info, EX_CLOSED)) {
        if(!(vict = get_char_in_remote_room_vis(ch, arg1, _2ND_EXIT(ch, snipe_dir)->to_room))) {
          if(_3RD_EXIT(ch, snipe_dir) && _3RD_EXIT(ch, snipe_dir)->to_room != NULL &&
            _3RD_EXIT(ch, snipe_dir)->to_room != ch->in_room &&
            !ROOM_FLAGGED(_3RD_EXIT(ch, snipe_dir)->to_room, ROOM_DEATH) &&
            !IS_SET(_3RD_EXIT(ch, snipe_dir)->exit_info, EX_CLOSED)) {
            if(!(vict = get_char_in_remote_room_vis(ch, arg1, _3RD_EXIT(ch, snipe_dir)->to_room))) {
              send_to_char("Your target is not in sight in that direction!\r\n", ch);
              return;
            }
          }
          else {
            send_to_char("Your target is not in sight in that direction!\r\n", ch);
            return;
          }
        }
      }
      else {
        send_to_char("Your target is not in sight in that direction!\r\n", ch);
        return;
      }
    }
  }
  else {
    send_to_char("You can't snipe in that direction!\r\n", ch);
    return;
  }
  // is vict an imm?
  if((GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
    send_to_char("Are you crazy man!?!  You'll piss off superfly!!\r\n", ch);
    return;
  }
  //is the player trying to snipe himself?
  if(vict == ch) {
    send_to_char("Yeah...real funny.\r\n", ch);
    return;
  }
  // if vict is fighting someone you have a 50% chance of hitting the person
  // vict is fighting
  if((vict->isFighting()) && (number(0, 1))) {
    vict = (vict->getFighting());
  }
  // Has vict been sniped once and is vict a sentinel mob?
  if((MOB_FLAGGED(vict, MOB_SENTINEL)) && IS_SNIPED(vict)) {
    act( "$N has taken cover!\r\n", TRUE, ch, NULL, vict, TO_CHAR);
    return;
  } 
  //make sure we have a victim...
  if(vict) {
    // is ch in a peaceful room?
    if(!peaceful_room_ok(ch, vict, true)) {
      return;
    }
    //is vict in a peaceful room?
    else if(IS_SET(ROOM_FLAGS(vict->in_room), ROOM_PEACEFUL)) {
      act("$N is in an NVZ!", FALSE, ch, NULL, vict, TO_CHAR);
      return;
    }
    //Ok, last check...is some asshole trying to damage a shop keeper
    if(!ok_damage_shopkeeper(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
      WAIT_STATE(ch, PULSE_VIOLENCE * 3);
      send_to_char("NO! The gods will DESTROY you for that!\r\n", ch);
      return;
    }

    // Ok, the victim is in sight, ch is not blind or in a smoky room
    // ch is wielding a rifle, and neither the victim nor ch is in a
    // peaceful room, and all other misc. checks have been passed...
    // begin the hit prob calculations
    prob = CHECK_SKILL(ch, SKILL_SNIPE) + GET_REMORT_GEN(ch);
    // start percent at 40 to make it impossible for a merc to hit
    // someone if his skill is less than 40
    percent = number(40, 125);
    if(affected_by_spell(vict, ZEN_AWARENESS) || 
       IS_AFFECTED(vict, AFF2_TRUE_SEEING)) {
      percent += 25;
    }

    if(vict->getPosition() < POS_FIGHTING) {
      percent += 15;
    }

    if(IS_AFFECTED_2(vict, AFF2_HASTE) && !IS_AFFECTED_2(ch, AFF2_HASTE)) {
      percent += 25;
    }
    // if the victim has already been sniped (wears off in 3 ticks)
    // then the victim is aware of a sniper and is assumed
    // to be taking the necessary precautions, and therefore is
    // much harder to hit
    if(IS_SNIPED(vict)) {
      percent += 50;
    }
    // just some level checks.  The victims level matters more
    // because it should be almost impossible to hit a high
    // level character who has been sniped once already
    prob += (GET_LEVEL(ch) >> 2) + GET_REMORT_GEN(ch);
    percent += GET_LEVEL(vict) + (GET_REMORT_GEN(vict) >> 2);
    // get a random damage location.  I know there is a WEAR_RANDOM
    // constant but I wasn't sure how to use it and this works just
    // as well
    damage_loc = number(0, 26);
    // we need to extract the bullet so we need an object pointer to
    // it.  However we musn't over look the possibility that gun->contains
    // could be a clip rather than a bullet
    bullet = gun->contains;
    if(IS_CLIP(bullet)) {
      bullet = bullet->contains;
    }
    extract_obj(bullet);
    // if percent is greater than prob it's a miss
    if(percent > prob) {
      // call damage with 0 dam to check for killers, TG's
      // newbie protection and other such stuff automagically ;)
      retval = damage(ch, vict, 0, SKILL_SNIPE, damage_loc);
      if(retval == DAM_ATTACK_FAILED) {
        return;
      }
      // ch and vict really shouldn't be fighting if they aren't in
      // the same room...
      stop_fighting(ch);
      stop_fighting(vict);
      send_to_char("Damn!  You missed!\r\n", ch);
      act("$n tries to snipe $N, but misses!", TRUE, ch, NULL, vict, TO_ROOM);
      sprintf(buf, "A bullet screams past your head from the %s!", dirs[snipe_dir]);
      act(buf, TRUE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "A bullet screams past $n's head from the %s!", dirs[snipe_dir]);
      act(buf, TRUE, vict, NULL, ch, TO_ROOM);
      WAIT_STATE(ch, 3 RL_SEC);
      return;
    }   
    else {
      // it a hit!
      // grab the damage for the gun...this way this skill is
      // scalable
      dam = dice(gun_damage[GUN_TYPE(gun)][0],
                 gun_damage[GUN_TYPE(gun)][1] + BUL_DAM_MOD(bullet));
      // as you can see, armor makes a huge difference, it's hard to imagine
      // a bullet doing much more than brusing someone through a T. carapace
      if((armor = GET_EQ(vict, damage_loc)) && OBJ_TYPE(armor, ITEM_ARMOR)) {
        if(IS_STONE_TYPE(armor) || IS_METAL_TYPE(armor))
          dam -= GET_OBJ_VAL(armor, 0) << 4;
        else
          dam -= GET_OBJ_VAL(armor, 0) << 2;
      }

      add_blood_to_room(vict->in_room, 1);
      apply_soil_to_char(ch, GET_EQ(vict, damage_loc), SOIL_BLOOD, damage_loc);
      if(!affected_by_spell(vict, SKILL_SNIPE)) {
        af.type = SKILL_SNIPE;
        af.is_instant = 0;
        af.bitvector = AFF3_SNIPED;
        af.aff_index = 3;
        af.level = GET_LEVEL(ch);
        af.duration = 3;
        af.location = damage_loc;
        af.modifier = 0;
        affect_to_char(vict, &af);
        WAIT_STATE(vict, 2 RL_SEC);
      }
      else
       WAIT_STATE(vict, 2 RL_SEC);
      // double damage for a head shot...1 in 27 chance
      if(damage_loc == WEAR_HEAD) {
        dam = dam << 1;
      }
      // 1.5x damage for a neck shot...2 in 27 chance
      else if(damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
        dam += dam >> 1;
      }
      // seems to crash the mud if you call damage_eq() on a location
      // that doesn't have any eq...hmmm
      if(GET_EQ(vict, damage_loc)) {
        damage_eq(vict, GET_EQ(vict, damage_loc), dam >> 1);
      }
      if(damage_loc == WEAR_HEAD) {
        send_to_char("HEAD SHOT!!\r\n", ch);
      }
      else if(damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
        send_to_char("NECK SHOT!!\r\n", ch);
      }
      if(GET_LEVEL(vict) > 6) {
        act("You smirk with satisfaction as your bullet rips into $N.",
            FALSE, ch, NULL, vict, TO_CHAR);
        sprintf(buf, "A bullet rips into your flesh from the %s!\r\n", dirs[snipe_dir]);
        act(buf, TRUE, ch, NULL, vict, TO_VICT);
        act("A bullet rips into $n's flesh!", TRUE, vict, NULL, ch, TO_ROOM);
        act("$n takes aim and snipes $N!", TRUE, ch, NULL, vict, TO_ROOM);
      }
      // it's my understanding that damage() frees the memory for vict if
      // vict has been killed.  Therefore if I want to print the message
      // "You have killed $N" I'm going to have to make a copy of victs
      // char_data struct.  If there's another way that I'm unaware of
      // please change it and/or let me know
      if(!(temp = (struct char_data *)malloc(sizeof(struct char_data) +1))) {
        mudlog("SYSERR: Not enough memory to allocate for struct char_data *\r\n        in function do_snipe!",
               NRM, LVL_AMBASSADOR, TRUE);
      }
      memcpy(temp, vict, sizeof(struct char_data));
      retval = damage(ch, vict, dam, SKILL_SNIPE, damage_loc);
      // I hope this works...this is supposed to handle the case
      // of ch hitting vict succesfully but for some outside
      // reason (newbie protection) the attack fails
      if(retval == DAM_ATTACK_FAILED) {
        return;
      }
      if(IS_SET(retval, DAM_VICT_KILLED)) {
        act("You have killed $N!", TRUE, ch, NULL, temp, TO_CHAR);
        act("$n has killed $N!", TRUE, ch, NULL, temp, TO_ROOM);
      }
      free(temp);
      gain_skill_prof(ch, SKILL_SNIPE);
      // again, if ch and vict aren't in the same room they
      // shouldn't be fighting each other
      stop_fighting(ch);
      if(!IS_SET(retval, DAM_VICT_KILLED)) {
        stop_fighting(vict);
      }
      WAIT_STATE(ch, 5 RL_SEC);
    }
  }
  else {
    mudlog("ERROR: Null vict at end of do_snipe!", NRM, LVL_AMBASSADOR, TRUE);
  }
}

ACMD(do_wrench)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL;
    struct obj_data *neck = NULL;
    int two_handed = 0;
    int prob, percent, dam, genmult;

    genmult = GET_REMORT_GEN( ch );

    one_argument( argument, arg );

    if ( ! ( vict = get_char_room_vis( ch, arg ) ) ) {
        if ( ch->isFighting() ) {
            vict = ( ch->getFighting() );
        } else if ( ( ovict = get_obj_in_list_vis( ch, arg, ch->in_room->contents ) ) ) {
            act( "You fiercly wrench $p!", FALSE, ch, ovict, 0, TO_CHAR );
            return;
        } else {
            send_to_char( "Wrench who?\r\n", ch );
            return;
        }
    }

     if ( GET_EQ( ch, WEAR_WIELD ) && IS_TWO_HAND( GET_EQ( ch, WEAR_WIELD ) ) ) {
         send_to_char( "You are using both hands to wield your weapon right now!\r\n", ch );
         return;
     }

     if ( GET_EQ( ch, WEAR_WIELD ) && ( GET_EQ( ch, WEAR_WIELD_2 ) || 
          GET_EQ( ch, WEAR_HOLD ) || GET_EQ( ch, WEAR_SHIELD ) ) ) {
         send_to_char( "You need a hand free to do that!\r\n", ch );   
         return;
     }

    if (!peaceful_room_ok(ch, vict, true))
    return;
     //
     // give a bonus if both hands are free
     //

     if( !GET_EQ( ch, WEAR_WIELD ) && 
         !( GET_EQ( ch, WEAR_WIELD_2 ) || GET_EQ( ch, WEAR_HOLD ) 
            || GET_EQ( ch, WEAR_SHIELD ) ) ) {
         two_handed = 1;
     }

     percent = ( ( 10 - ( GET_AC( vict ) / 50 ) ) << 1) + number( 1, 101 );
     prob = CHECK_SKILL( ch, SKILL_WRENCH );

     if ( ! CAN_SEE( ch, vict ) ) {
         prob += 10;
     }

     dam = dice( genmult + GET_LEVEL( ch ), dice(2, GET_STR( ch )  ) );

     if ( two_handed ) {
         dam += dam/2;
     }
     
     if ( ! ( ch->isFighting() ) && ! ( vict->isFighting() ) ) {
         dam += dam/3;
     }

     if ( ( ( neck = GET_IMPLANT( vict, WEAR_NECK_1 ) ) && NOBEHEAD_EQ( neck ) ) ||
          ( ( neck = GET_IMPLANT( vict, WEAR_NECK_2 ) ) && NOBEHEAD_EQ( neck ) ) ) {
         dam >>= 1;
         damage_eq( ch, neck, dam );  
     }

     if ( ( ( neck = GET_EQ( vict, WEAR_NECK_1 ) ) && NOBEHEAD_EQ( neck ) ) || 
          ( ( neck = GET_EQ( vict, WEAR_NECK_2 ) ) && NOBEHEAD_EQ( neck ) ) ) {
         act( "$n grabs you around the neck, but you are covered by $p!", FALSE, ch, neck, vict, TO_VICT );
         act( "$n grabs $N's neck, but $N is covered by $p!", FALSE, ch, neck, vict, TO_NOTVICT );
         act( "You grab $N's neck, but $e is covered by $p!", FALSE, ch, neck, vict, TO_CHAR );
         check_toughguy( ch, vict, 0 );
         check_killer( ch, vict );
         damage_eq( ch, neck, dam );
         WAIT_STATE( ch, 2 RL_SEC );
         return;
     }

     if ( prob > percent  && ( CHECK_SKILL(ch, SKILL_WRENCH) >= 30 ) ) {
         WAIT_STATE( ch, PULSE_VIOLENCE * 4 );
         WAIT_STATE( vict, PULSE_VIOLENCE * 2);
         damage( ch, vict, dam, SKILL_WRENCH, WEAR_NECK_1 );
         gain_skill_prof( ch, SKILL_WRENCH );
         return;
     } else {
         WAIT_STATE( ch, PULSE_VIOLENCE * 2 );
         damage( ch, vict, 0, SKILL_WRENCH, WEAR_NECK_1 );
     }
}

ACMD(do_infiltrate)
{
    struct affected_type af;

    if (IS_AFFECTED_3(ch, AFF3_INFILTRATE)) {
       send_to_char("Okay, you are no longer attempting to infiltrate.\r\n",ch);
       affect_from_char(ch, SKILL_INFILTRATE);
       affect_from_char(ch, SKILL_SNEAK);
       return;
    }

    if (CHECK_SKILL(ch, SKILL_INFILTRATE) < number(20, 70)) {
       send_to_char("You don't feel particularly sneaky...\n", ch);
       return;
    }

    send_to_char("Okay, you'll try to infiltrate until further notice.\r\n", ch);
    
    af.type = SKILL_SNEAK;
    af.is_instant = 0;
    af.duration = GET_LEVEL(ch);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_SNEAK;
    af.level = GET_LEVEL(ch) + ch->getLevelBonus(SKILL_INFILTRATE);
    affect_to_char(ch, &af);

    af.type = SKILL_INFILTRATE;
    af.aff_index = 3;
    af.is_instant = 0;
    af.duration = GET_LEVEL(ch);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF3_INFILTRATE;
    af.level = GET_LEVEL(ch) + ch->getLevelBonus(SKILL_INFILTRATE);
    affect_to_char(ch, &af);
}







