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

ACMD(do_pistolwhip)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;

    one_argument(argument, arg);

    if (!(vict = get_char_room_vis(ch, arg))) {
	if (FIGHTING(ch)) {
	    vict = FIGHTING(ch);
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
	act("You ruthlessly beat yourself to death with $p!", 
	    FALSE, ch, weap, 0, TO_CHAR);
	act("$n beats $mself to death with $p!", TRUE, ch, weap, 0, TO_ROOM);
	sprintf(buf, "%s killed self with an pistolwhip at %d.",
		GET_NAME(ch), ch->in_room->number);
	mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
	gain_exp(ch, -(GET_LEVEL(ch) * 1000));
	raw_kill(ch, ch, SKILL_PISTOLWHIP);
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

#define NOBEHEAD_EQ(obj) \
IS_SET(obj->obj_flags.bitvector[1], AFF2_NECK_PROTECTED)

ACMD(do_wrench)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL;
    struct obj_data *neck = NULL;
    int two_handed = 0;
    int prob, percent, dam, genmult;


    genmult = GET_REMORT_GEN( ch );
    genmult = genmult/2;


    one_argument( argument, arg );


    if ( ! ( vict = get_char_room_vis( ch, arg ) ) ) {
        
	if ( FIGHTING( ch ) ) {
            vict = FIGHTING( ch );
        } 
	
	else if ( ( ovict = get_obj_in_list_vis( ch, arg, ch->in_room->contents ) ) ) {
	    act( "You fiercly wrench $p!", FALSE, ch, ovict, 0, TO_CHAR );
            return;
        }
	
	else {
            send_to_char( "Wrench who?\r\n", ch );
            return;
        }
    }




     if ( GET_EQ( ch, WEAR_WIELD ) && IS_TWO_HAND( GET_EQ( ch, WEAR_WIELD ) ) ) {
	 send_to_char( "You are using both hands to wield your weapon right now!\r\n", ch );
	 return;
     }
	
     if ( GET_EQ( ch, WEAR_WIELD ) && ( GET_EQ( ch, WEAR_WIELD_2 ) ||
					GET_EQ( ch, WEAR_HOLD ) ||
					GET_EQ( ch, WEAR_SHIELD ) ) ) {
	 send_to_char( "You need a hand free to do that!\r\n", ch );   
	 return;
     }

     // 
     // give a bonus if both hands are free
     //

     if( ! GET_EQ( ch, WEAR_WIELD ) && ! ( GET_EQ( ch, WEAR_WIELD_2 ) ||
					   GET_EQ( ch, WEAR_HOLD ) ||
					   GET_EQ( ch, WEAR_SHIELD ) ) ) {
	 
	 two_handed = 1;
     }


     percent = ( ( 10 - ( GET_AC( vict ) / 50 ) ) << 1) + number( 1, 101 );
     prob = CHECK_SKILL( ch, SKILL_WRENCH );

     if ( ! CAN_SEE( ch, vict ) ) {
	 prob += 10;
     }


     dam = dice( 5 + genmult + GET_LEVEL( ch ), dice(2, GET_STR( ch )  ) );

     if ( two_handed ) {

	 dam += dam/2;
     }
     
     if ( ! FIGHTING( ch ) && ! FIGHTING( vict ) ) {

	 dam += dam/3;
     }

     if ( ( ( neck = GET_IMPLANT( vict, WEAR_NECK_1 ) ) &&
	  NOBEHEAD_EQ( neck ) ) ||
	 ( ( neck = GET_IMPLANT( vict, WEAR_NECK_2 ) ) &&
	  NOBEHEAD_EQ( neck ) ) ) {

	 dam >>= 1;
	 damage_eq( ch, neck, dam );  
	 
     }



     if ( ( ( neck = GET_EQ( vict, WEAR_NECK_1 ) ) &&
	  NOBEHEAD_EQ( neck ) ) || 
	 ( ( neck = GET_EQ( vict, WEAR_NECK_2 ) ) &&
	  NOBEHEAD_EQ( neck ) ) ) {
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
	 
	 WAIT_STATE( ch, PULSE_VIOLENCE * 2 );
         WAIT_STATE( vict, PULSE_VIOLENCE );
	 damage( ch, vict, dam, SKILL_WRENCH, WEAR_NECK_1 );
	 gain_skill_prof( ch, SKILL_WRENCH );
	 return;
     }

     else {
	 WAIT_STATE( ch, PULSE_VIOLENCE * 2 );
	 damage( ch, vict, 0, SKILL_WRENCH, WEAR_NECK_1 );
     }



}









