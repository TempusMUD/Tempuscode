/*************************************************************************
*   File: combat_utils.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: combat_utils.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __combat_code__
#define __combat_utils__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "fight.h"
#include "bomb.h"
#include "shop.h"
#include "guns.h"
#include "specs.h"

#include <iostream>

extern int corpse_state;

/* The Fight related routines */
obj_data * get_random_uncovered_implant ( char_data *ch, int type = -1 );
int calculate_weapon_probability( struct char_data *ch, int prob, struct obj_data *weap);


int 
calculate_weapon_probability( struct char_data *ch, int prob, struct obj_data *weap) {
    int i, weap_weight;
    i = weap_weight = 0;

    if(!ch || !weap)
        return prob;

    // Add in weapon specialization bonus.   
    if ( GET_OBJ_VNUM(weap) > 0 ) {
        for ( i = 0; i < MAX_WEAPON_SPEC; i++ ) {
            if ( GET_WEAP_SPEC( ch, i ).vnum == GET_OBJ_VNUM( weap ) ) {
                prob += GET_WEAP_SPEC( ch, i ).level << 2;
                break;
            }   
        }   
    }  

	// Below this check applies to WIELD and WIELD_2 only!
    if(weap->worn_on != WEAR_WIELD && weap->worn_on != WEAR_WIELD_2) {
		return prob;
	}

	// Weapon speed
	for ( i = 0, weap_weight = 0; i < MAX_OBJ_AFFECT; i++ ) {
		if ( weap->affected[i].location == APPLY_WEAPONSPEED ) {
			weap_weight -= weap->affected[i].modifier;
			break;
		}   
	}
	// 1/4 actual weapon weight or effective weapon weight, whichever is higher.
	weap_weight += weap->getWeight();
	weap_weight = MAX( ( weap->getWeight() >> 2 ), weap_weight );

	if(weap->worn_on == WEAR_WIELD_2) {
		prob -= ( prob * weap_weight ) / 
				( str_app[STRENGTH_APPLY_INDEX( ch )].wield_w >> 1 );
		prob += CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) - 60;
	} else {
		prob -= ( prob * weap_weight ) / 
				( str_app[STRENGTH_APPLY_INDEX( ch )].wield_w << 1 );
		if(IS_MONK( ch ) )
			prob += ( LEARNED( ch ) - weapon_prof( ch, weap) ) >> 3;
	}
    return prob;
}


void 
update_pos( struct char_data * victim )
{
    if ( GET_HIT( victim ) > 0 && victim->getPosition() == POS_SLEEPING )
		victim->setPosition( POS_RESTING,1 );
    else if ( ( GET_HIT( victim ) > 0 ) 
        && ( victim->getPosition() > POS_STUNNED ) 
        && victim->getPosition() < POS_FIGHTING &&
	    FIGHTING( victim ) ) {
		if ( ( victim->desc && victim->desc->wait <= 0 ) ||
			 ( IS_NPC( victim ) && GET_MOB_WAIT( victim ) <= 0 ) ) {
            if ( victim->getPosition() < POS_FIGHTING) {
                if(!IS_AFFECTED_3(victim,AFF3_GRAVITY_WELL) ||
                    number(1,20) < GET_STR(victim)) {
                    act( "$n scrambles to $s feet!", TRUE, victim, 0, 0, TO_ROOM );
                    victim->setPosition( POS_FIGHTING, 1 );
                }
                WAIT_STATE( victim, PULSE_VIOLENCE );
            } else {
                victim->setPosition( POS_FIGHTING, 1 );
            }
        } else {
			return;
        }
    } else if ( GET_HIT( victim ) > 0 ) {
            if ( victim->in_room->isOpenAir() 
            && !IS_AFFECTED_3(victim, AFF3_GRAVITY_WELL)
            && victim->getPosition() != POS_FLYING)
                victim->setPosition( POS_FLYING, 1 );
            else if(!IS_AFFECTED_3(victim,AFF3_GRAVITY_WELL) 
            && victim->getPosition() < POS_FIGHTING ) {
                victim->setPosition( POS_STANDING, 1 );
                act( "$n scrambles to $s feet!", TRUE, victim, 0, 0, TO_ROOM );
                WAIT_STATE( victim, PULSE_VIOLENCE );
            } else if ( number(1,20) < GET_STR(victim)
            && victim->getPosition() < POS_FIGHTING ) {
                victim->setPosition( POS_STANDING, 1 );
                act( "$n scrambles to $s feet!", TRUE, victim, 0, 0, TO_ROOM );
                WAIT_STATE( victim, PULSE_VIOLENCE );
            }
    }
    else if ( GET_HIT( victim ) <= -11 )
        victim->setPosition( POS_DEAD, 1 );
    else if ( GET_HIT( victim ) <= -6 )
        victim->setPosition( POS_MORTALLYW, 1);
    else if ( GET_HIT( victim ) <= -3 )
        victim->setPosition( POS_INCAP, 1 );
    else
        victim->setPosition( POS_STUNNED,1 );
}

void
check_killer( struct char_data * ch, struct char_data * vict, const char *debug_msg=0 )
{
    if ( !PLR_FLAGGED( vict, PLR_KILLER | PLR_THIEF ) &&
	 !PLR_FLAGGED( ch, PLR_KILLER ) &&
	 ( !PLR_FLAGGED( vict, PLR_TOUGHGUY ) ||  
	   ( IS_REMORT( ch ) && !IS_REMORT( vict ) && 
	     !PLR_FLAGGED( vict, PLR_REMORT_TOUGHGUY ) ) ) &&
	 !IS_NPC( ch ) && !IS_NPC( vict ) &&
	 ( GET_LEVEL( ch ) > GET_LEVEL( vict ) ||
	   ( IS_REMORT( ch ) && !IS_REMORT( vict ) ) ) &&
	 !affected_by_spell( vict, SKILL_DISGUISE ) &&
	 ( ch != vict ) && !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) ) {
	char buf[256];
    if( GET_LEVEL(ch) >= LVL_POWER )
		return;
	SET_BIT( PLR_FLAGS( ch ), PLR_KILLER );
	sprintf( buf, "PC KILLER set on %s for attack on %s at %d.",
		 GET_NAME( ch ), GET_NAME( vict ), vict->in_room->number );
	mudlog( buf, BRF, LVL_AMBASSADOR, TRUE );
	send_to_char( "If you want to be a PLAYER KILLER, so be it...\r\n", ch );
	
	sprintf( buf, "KILLER set from: %s", debug_msg ? debug_msg : "unknown" );
	slog( buf );
    }
}


// remember that ch->in_room can be null!!
void 
check_toughguy( struct char_data * ch, struct char_data * vict, int mode )
{
    if ( IS_NPC( ch ) || IS_NPC( vict ) || affected_by_spell( vict, SKILL_DISGUISE ) ||
	 ( ch == vict ) || ROOM_FLAGGED( vict->in_room, ROOM_ARENA ) )
	return;

    if ( ( !PLR_FLAGGED( ch, PLR_TOUGHGUY ) ||
	   ( !PLR_FLAGGED( ch, PLR_REMORT_TOUGHGUY ) && IS_REMORT( vict ) ) ) ) {
	char buf[256];
	
	if ( !PLR_FLAGGED( ch, PLR_TOUGHGUY ) ) {
	    SET_BIT( PLR_FLAGS( ch ), PLR_TOUGHGUY );
	    sprintf( buf,
		     "PC Tough bit set on %s for %s %s at %d.",
		     GET_NAME( ch ), mode ? "robbing" : "attack on",
		     GET_NAME( vict ), vict->in_room->number );
	    mudlog( buf, BRF, LVL_AMBASSADOR, TRUE );
	}
	if ( IS_REMORT( vict ) && !IS_REMORT( ch ) && 
	     !PLR_FLAGGED( ch, PLR_REMORT_TOUGHGUY ) ) {
	    SET_BIT( PLR_FLAGS( ch ), PLR_REMORT_TOUGHGUY );
	    sprintf( buf,
		     "PC remort_Tough set on %s for %s %s at %d.",
		     GET_NAME( ch ), mode ? "robbing" : "attack on",
		     GET_NAME( vict ), vict->in_room->number );
	    mudlog( buf, BRF, LVL_AMBASSADOR, TRUE );
	}
    }
}

void 
check_object_killer( struct obj_data * obj, struct char_data * vict )
{
    CHAR cbuf;
    struct char_data *killer;
    int player_i=0;
    struct char_file_u tmp_store;
    int obj_id;
    int is_file = 0;
    int is_desc = 0;
    struct descriptor_data *d = NULL;

    if ( ROOM_FLAGGED( vict->in_room, ROOM_PEACEFUL ) )
	return;

    sprintf( buf, "Checking object killer %s -> %s.", obj->short_description, GET_NAME( vict ) );;;
    slog( buf );

    if ( IS_BOMB( obj ) )
	obj_id = BOMB_IDNUM( obj );
    else if ( GET_OBJ_SIGIL_IDNUM( obj ) )
	obj_id = GET_OBJ_SIGIL_IDNUM( obj );
    else {
	slog( "SYSERR: unknown damager in check_object_killer." );
	return;
    }
	    
    if ( !obj_id )
	return;

    for ( killer = character_list; killer; killer = killer->next )
	if ( !IS_NPC( killer ) && GET_IDNUM( killer ) == obj_id )
	    break;

    // see if the sonuvabitch is still connected
    if ( !killer ) {

	for ( d = descriptor_list; d; d = d->next ) {
	    if ( d->connected && d->character && GET_IDNUM( d->character ) == obj_id ) {
		is_desc = 1;
		killer = d->character;
		break;
	    }
	}
    }

    // load the bastich from file.
    if ( !killer ) {

	clear_char( &cbuf );

	if ( ( player_i = load_char( get_name_by_id( obj_id ), &tmp_store ) ) > -1 ) {
	    store_to_char( &tmp_store, &cbuf );
	    is_file = 1;
	    killer = &cbuf;
	}
    }

    // the piece o shit has a bogus killer idnum on it!x
    if ( !killer ) {
	sprintf( buf, "SYSERR: bogus idnum %d on object %s damaging %s.", obj_id, obj->short_description, GET_NAME( vict ) );
	slog( buf );
	return;
    }

    check_toughguy( killer, vict, 0 );

    if ( GET_LEVEL( killer ) < GET_LEVEL( vict ) || ( !IS_REMORT( killer ) && IS_REMORT( vict ) ) )
	return;
    
    if ( !PLR_FLAGGED( vict, PLR_KILLER ) && !PLR_FLAGGED( vict, PLR_THIEF ) &&
	 !PLR_FLAGGED( killer, PLR_KILLER ) &&
	 ( !PLR_FLAGGED( vict, PLR_TOUGHGUY ) ||  
	   ( IS_REMORT( killer ) && !IS_REMORT( vict ) && 
	     !PLR_FLAGGED( vict, PLR_REMORT_TOUGHGUY ) ) ) &&
	 !IS_NPC( killer ) && !IS_NPC( vict ) &&
	 !affected_by_spell( vict, SKILL_DISGUISE ) &&
	 ( killer != vict ) && !ROOM_FLAGGED( vict->in_room, ROOM_ARENA ) ) {
	char buf[256];
	if( GET_LEVEL(killer) >= LVL_POWER ) {
		return;
	}
	SET_BIT( PLR_FLAGS( killer ), PLR_KILLER );
	sprintf( buf, "PC KILLER bit set on %s %sfor damaging %s at %s with %s.",
		 GET_NAME( killer ), is_file ? "( file ) " : is_desc ? "( desc ) " :  "", GET_NAME( vict ), ( vict->in_room ) ? 
		 vict->in_room->name : "DEAD", obj->short_description );
	mudlog( buf, BRF, LVL_AMBASSADOR, TRUE );

	if ( is_desc ) {
	    sprintf( buf, "KILLER bit set for damaging %s with %s.!\r\n", GET_NAME( vict ), obj->short_description );
	    SEND_TO_Q( buf, killer->desc );
	}

	else if ( !is_file )
	    act( "KILLER bit set for damaging $N with $p!", FALSE, killer, obj, vict, TO_CHAR );
    }

    // save the sonuvabitch to file if the killer was offline
    if ( is_file ) {
	char_to_store( killer, &tmp_store );
	fseek( player_fl, ( player_i ) * sizeof( struct char_file_u ), SEEK_SET );
	fwrite( &tmp_store, sizeof( struct char_file_u ), 1, player_fl );
    }
}
char *
replace_string( char *str, char *weapon_singular, char *weapon_plural, 
		const char *location )
{
    static char buf[256];
    char *cp;

    cp = buf;

    for ( ; *str; str++ ) {
	if ( *str == '#' ) {
	    switch ( *( ++str ) ) {
	    case 'W':
		for ( ; *weapon_plural; *( cp++ ) = *( weapon_plural++ ) );
		break;
	    case 'w':
		for ( ; *weapon_singular; *( cp++ ) = *( weapon_singular++ ) );
		break;
	    case 'p':
		if ( *location )
		    for ( ; *location; *( cp++ ) = *( location++ ) );
		break;
	    default:
		*( cp++ ) = '#';
		break;
	    }
	} else
	    *( cp++ ) = *str;

	*cp = 0;
    }				/* For */

    return ( buf );
}

/* Calculate the raw armor including magic armor.  Lower AC is better. */
int 
calculate_thaco( struct char_data *ch, struct char_data *victim,
		 struct obj_data *weap )
{
    int calc_thaco, wpn_wgt, i;

    calc_thaco = ( int ) MIN( THACO( GET_CLASS( ch ), GET_LEVEL( ch ) ),
			      THACO( GET_REMORT_CLASS( ch ), GET_LEVEL( ch ) ) );
    
    calc_thaco -= str_app[STRENGTH_APPLY_INDEX( ch )].tohit;

    if ( GET_HITROLL( ch ) <= 5 )
	calc_thaco -= GET_HITROLL( ch );
    else if ( GET_HITROLL( ch ) <= 50 )
	calc_thaco -= 5 + ( ( ( GET_HITROLL( ch ) - 5 ) ) / 3 );
    else
	calc_thaco -= 20;

    calc_thaco -= ( int ) ( ( GET_INT( ch ) - 12 ) >> 1 );     /* Intelligence helps! */
    calc_thaco -= ( int ) ( ( GET_WIS( ch ) - 10 ) >> 2 );     /* So does wisdom */

    if ( AWAKE( victim ) )
	calc_thaco -= dex_app[GET_DEX( victim )].defensive;

    if ( IS_DROW( ch ) ) {
	if ( OUTSIDE( ch ) && PRIME_MATERIAL_ROOM( ch->in_room ) ) {
	    if ( ch->in_room->zone->weather->sunlight == SUN_LIGHT )
		calc_thaco += 10;
	    else if ( ch->in_room->zone->weather->sunlight == SUN_DARK )
		calc_thaco -= 5;
	} else if ( IS_DARK( ch->in_room ) ) 
	    calc_thaco -= 5;
    }

    if ( weap ) {
	if ( ch != weap->worn_by ) {
	    slog( "SYSERR: inconsistent weap->worn_by ptr in calculate_thaco." );
	    sprintf( buf, "weap: ( %s ), ch: ( %s ), weap->worn->by: ( %s )",
		     weap->short_description, GET_NAME( ch ), weap->worn_by ?
		     GET_NAME( weap->worn_by ) : "NULL" );
	    slog( buf );
	    return 0;
	}
    
	if ( GET_OBJ_VNUM( weap ) > 0 ) {
	    for ( i = 0; i < MAX_WEAPON_SPEC; i++ )
		if ( GET_WEAP_SPEC( ch, i ).vnum == GET_OBJ_VNUM( weap ) ) {
		    calc_thaco -= GET_WEAP_SPEC( ch, i ).level;
		    break;
		}
	} 
	  
	wpn_wgt = weap->getWeight();
	if ( wpn_wgt > str_app[STRENGTH_APPLY_INDEX( ch )].wield_w )
	    calc_thaco += 2;
	if ( IS_MAGE( ch ) && 
	     ( wpn_wgt > 
	       ( ( GET_LEVEL( ch )*str_app[STRENGTH_APPLY_INDEX( ch )].wield_w/100 )
		 + ( str_app[STRENGTH_APPLY_INDEX( ch )].wield_w>>1 ) ) ) )
	    calc_thaco += ( wpn_wgt >> 2 );
	else if ( IS_THIEF( ch ) && ( wpn_wgt > 12 + ( GET_STR( ch ) >> 2 ) ) )
	    calc_thaco += ( wpn_wgt >> 3 ); 

	if ( IS_MONK( ch ) )
	    calc_thaco += ( LEARNED( ch ) - weapon_prof( ch, weap ) ) / 8;

	if ( GET_EQ( ch, WEAR_WIELD_2 ) ) {
	    if ( CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) < LEARNED( ch ) ) {
		if ( weap == GET_EQ( ch, WEAR_WIELD_2 ) )
		    calc_thaco -= ( LEARNED( ch )-CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) )/5;
		else
		    calc_thaco -= ( LEARNED( ch )-CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) )/10;
	    }
	}
    } /* end if ( weap ) */

    if ( ( IS_EVIL( ch ) && IS_AFFECTED( victim, AFF_PROTECT_EVIL ) ) ||
	 ( IS_GOOD( ch ) && IS_AFFECTED( victim, AFF_PROTECT_GOOD ) ) ||
	 ( GET_RACE( ch ) == RACE_UNDEAD && 
	   IS_AFFECTED( victim, AFF2_PROTECT_UNDEAD ) ) )
	calc_thaco += 2;
  
    if ( IS_CARRYING_N( ch ) > ( CAN_CARRY_N( ch )*0.80 ) )
	calc_thaco += 1;

    if ( CAN_CARRY_W( ch ) )
	calc_thaco += ( ( TOTAL_ENCUM( ch ) << 1 ) / CAN_CARRY_W( ch ) );
    else
	calc_thaco += 10;
  
    if ( IS_AFFECTED_2( ch, AFF2_DISPLACEMENT ) && 
	 !IS_AFFECTED_2( victim, AFF2_TRUE_SEEING ) )
	calc_thaco -= 2;
    if ( IS_AFFECTED( ch, AFF_BLUR ) )
	calc_thaco -= 1;
    if ( !CAN_SEE( victim, ch ) )
	calc_thaco -= 3;

    if ( !CAN_SEE( ch, victim ) )
	calc_thaco += 2;
    if ( GET_COND( ch, DRUNK ) )
	calc_thaco += 2;
    if ( IS_SICK( ch ) )
	calc_thaco += 2;
    if ( IS_AFFECTED_2( victim, AFF2_DISPLACEMENT ) && 
	 !IS_AFFECTED_2( ch, AFF2_TRUE_SEEING ) )
	calc_thaco += 2;
    if ( IS_AFFECTED_2( victim, AFF2_EVADE ) )
	calc_thaco += ( GET_LEVEL( victim )/6 ) + 1;

    if ( SECT_TYPE( ch->in_room ) == SECT_UNDERWATER && !IS_MOB( ch ) )
	calc_thaco += 4;

    calc_thaco -= MIN( 5, MAX( 0, ( POS_FIGHTING - victim->getPosition() ) ) );

    calc_thaco -= char_class_race_hit_bonus( ch, victim );

    return ( calc_thaco );
}

void
add_blood_to_room( struct room_data *rm, int amount )
{
    struct obj_data *blood;
    int new_blood = FALSE;

    if ( amount <= 0 )
	return;

    for ( blood = rm->contents; blood; blood = blood->next_content )
	if ( GET_OBJ_VNUM( blood ) == BLOOD_VNUM )
	    break;

    if ( !blood && ( new_blood = TRUE ) && !( blood = read_object( BLOOD_VNUM ) ) ) {
	slog( "SYSERR: Unable to load blood." );
	return;
    }

    if ( GET_OBJ_TIMER( blood ) > 50 )
	return;

    GET_OBJ_TIMER( blood ) += amount;

    if ( new_blood )
	obj_to_room( blood, rm );

}

int
apply_soil_to_char( struct char_data *ch,struct obj_data *obj,int type,int pos )
{

    int count = 0;

    if ( pos == WEAR_RANDOM ) {
        while ( count < 100 &&
            ( ( !GET_EQ( ch, pos ) && ( ILLEGAL_SOILPOS( pos ) ||
                            CHAR_SOILED( ch, pos, type ) ) ) ||
              ( GET_EQ( ch, pos ) 
                && OBJ_SOILED( GET_EQ( ch, pos ), type ) ) ) ) {
            pos = number( 0, NUM_WEARS-2 );
            count++;
        }
    }
    if ( count >= 100 ) /* didnt find pos.  rare, but possible */
        return 0;
	   
    if ( GET_EQ( ch, pos ) && (GET_EQ( ch, pos ) == obj || !obj) ) {
        if ( OBJ_SOILED( GET_EQ( ch, pos ), type ) )
            return 0;

        SET_BIT( OBJ_SOILAGE( GET_EQ( ch, pos ) ), type );
    }
    else if ( ILLEGAL_SOILPOS( pos ) || CHAR_SOILED( ch, pos, type ) ) {
        return 0;
    } else {
        SET_BIT( CHAR_SOILAGE( ch, pos ), type );
    }

    if ( type == SOIL_BLOOD && obj && GET_OBJ_VNUM( obj ) == BLOOD_VNUM )
	GET_OBJ_TIMER( obj ) = MAX( 1, GET_OBJ_TIMER( obj ) - 5 );
  
    return pos;
}


int
choose_random_limb( CHAR *victim )
{

    int prob;
    int i;
    static int limb_probmax = 0;

    if ( !limb_probmax ) {
	for ( i = 0; i < NUM_WEARS; i++ ) {
	    limb_probmax += limb_probs[i];

	    if ( i >= 1 )
		limb_probs[i] += limb_probs[i-1];
	}

    }

    prob = number( 1, limb_probmax );

    for ( i = 1; i < NUM_WEARS; i++ ) {
	if ( prob > limb_probs[i-1] &&
	     prob <= limb_probs[i] )
	    break;
    }

    if ( i >= NUM_WEARS ) {
	slog( "SYSERR: exceeded NUM_WEARS-1 in choose_random_limb." );
	return WEAR_BODY;
    }

    // shield will be the only armor check we do here, since it is a special position
    if ( i == WEAR_SHIELD ) {
	if ( !GET_EQ( victim, WEAR_SHIELD ) ) {
	    if ( !number( 0, 2 ) )
		i = WEAR_ARMS;
	    else
		i = WEAR_BODY;
	}
    }
  
    if ( !POS_DAMAGE_OK( i ) ) {
	sprintf( buf, "SYSERR improper pos %d leaving choose_random_limb.", i );
	slog( buf );
	return WEAR_BODY;
    }

    return i;
}  
// 
// the check for PRF2_KILLER is rolled into here for increased ( un )coherency.
// since this is called before any attack/damage/etc... its a good place to check the flag  
int peaceful_room_ok( struct char_data *ch, struct char_data *vict, bool mssg )
{
    if ( vict && IS_SET( ROOM_FLAGS( ch->in_room ), ROOM_PEACEFUL ) && 
	 !PLR_FLAGGED( vict, PLR_KILLER ) && GET_LEVEL( ch ) < LVL_GRGOD && 
     !( PLR_FLAGGED(ch, PLR_KILLER) && FIGHTING(vict) == ch)
     ) {
	send_to_char( "The universal forces of order prevent violence here!\r\n",
		      ch );
	if ( mssg ) {
	    if ( !number( 0, 1 ) )
		act( "$n seems to be violently disturbed.", FALSE, ch, 0, 0, TO_ROOM );
	    else
		act( "$n becomes violently agitated for a moment.",
		     FALSE,ch,0,0,TO_ROOM );
	}

	return 0;
    }
    if ( IS_AFFECTED( ch, AFF_CHARM ) && ( ch->master == vict ) ) {
	if ( mssg )
	    act( "$N is just such a good friend, you simply can't hurt $M.", 
		 FALSE, ch, 0, vict, TO_CHAR );
	return 0;
    }

    if ( !IS_NPC( ch ) && !IS_NPC( vict ) && !PRF2_FLAGGED( ch, PRF2_PKILLER ) ) {
	act( "In order to attack $N or another player, you must toggle your\n"
	     "Pkiller status with the 'pkiller' command.", FALSE, ch, 0, vict, TO_CHAR );
	return 0;
    }

    return 1;
} 
          
void 
make_corpse( struct char_data *ch,struct char_data *killer,int attacktype )
{
    struct obj_data *corpse = NULL, *head = NULL, *heart = NULL, 
	*spine = NULL, *o = NULL, *next_o = NULL, *leg = NULL;
    struct obj_data *money = NULL;
    int i;
    int rentcost = 0;
    char typebuf[256];
    char adj[256];
    char namestr[256];
    char isare[16];

    extern int max_npc_corpse_time, max_pc_corpse_time;

    *adj = '\0';
	// The Fate's corpses are portals to the remorter.
	if ( GET_MOB_SPEC(ch) == fate ) { // GetMobSpec checks IS_NPC
		struct obj_data *portal = NULL;
        extern int fate_timers[];
	    if ( ( portal = read_object( FATE_PORTAL_VNUM ) ) ) {
			GET_OBJ_TIMER( portal ) = max_npc_corpse_time;
			if(GET_MOB_VNUM(ch) == FATE_VNUM_LOW )  {
				GET_OBJ_VAL( portal, 2 ) = 1;
                fate_timers[0] = 0;
			} else if(GET_MOB_VNUM(ch) == FATE_VNUM_MID ) {
				GET_OBJ_VAL( portal, 2 ) = 2;
                fate_timers[1] = 0;
			} else if(GET_MOB_VNUM(ch) == FATE_VNUM_HIGH) {
				GET_OBJ_VAL( portal, 2 ) = 3;
                fate_timers[2] = 0;
			} else {
				GET_OBJ_VAL( portal, 2 ) = 12;
            }
			obj_to_room( portal, ch->in_room );
		}
	}
	// End Fate
    if ( corpse_state ) {
	attacktype = corpse_state;
	corpse_state = 0;
    }
    corpse = create_obj(  );
    corpse->shared = null_obj_shared;
    corpse->in_room = NULL;
    corpse->worn_on = -1;

    if ( AFF2_FLAGGED( ch, AFF2_PETRIFIED ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_STONE;
    else if ( IS_ROBOT( ch ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_FLESH;
    else if ( IS_SKELETON( ch ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_BONE;
    else if ( IS_PUDDING( ch ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_PUDDING;
    else if ( IS_SLIME( ch ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_SLIME;
    else if ( IS_PLANT( ch ) )
	GET_OBJ_MATERIAL( corpse ) = MAT_VEGETABLE;
    else 
	GET_OBJ_MATERIAL( corpse ) = MAT_FLESH;

    strcpy( isare, "is" );
  
    if ( GET_RACE( ch ) == RACE_ROBOT || GET_RACE( ch ) == RACE_PLANT ||
        attacktype == TYPE_FALLING ) {
        strcpy( isare, "are" );
        strcpy( typebuf, "remains" );
        strcpy( namestr, typebuf );
    } else if ( attacktype == SKILL_DRAIN ) {
        strcpy( typebuf, "husk" );
        strcpy( namestr, typebuf );
    } else {
        strcpy( typebuf, "corpse" );
        strcpy( namestr, typebuf );
    }
  
    if ( AFF2_FLAGGED( ch, AFF2_PETRIFIED ) ) {
        strcat( namestr, " stone" );
        strcpy( adj, "stone ");
        strcat( adj, typebuf);
        strcpy(typebuf,adj);
        adj[0] = '\0';
    }
  
#ifdef DMALLOC
    malloc_verify( 0 );
#endif

    if ( ( attacktype == SKILL_BEHEAD ||
	   attacktype == SKILL_PELE_KICK ||
	   attacktype == SKILL_CLOTHESLINE ) && isname( "headless", ch->player.name ) ) {
	attacktype = TYPE_HIT;
    }

    switch ( attacktype ) {

    case TYPE_HIT:
    case SKILL_BASH:
    case SKILL_PISTOLWHIP:
	sprintf( buf2, "The bruised up %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "bruised" );
	break;

    case TYPE_STING:
	sprintf( buf2, "The bloody, swollen %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "stung" );
	break;

    case TYPE_WHIP:
	sprintf( buf2, "The whelted %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "whipped" );
	break;

    case TYPE_SLASH:
    case TYPE_CHOP:
    case SPELL_BLADE_BARRIER:
	sprintf( buf2, "The chopped up %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "chopped up" );
	break;

    case SKILL_HAMSTRING:
    sprintf( buf2, "The legless %s of %s %s lying here.",
         typebuf,GET_NAME( ch ), isare );
    corpse->description = str_dup( buf2 );
    strcpy( adj, "legless" );

    if ( IS_RACE( ch, RACE_BEHOLDER ) )
        break;

    leg = create_obj(  );
    leg->shared = null_obj_shared;
    leg->in_room = NULL;
    if ( IS_AFFECTED_2( ch, AFF2_PETRIFIED ) )
        leg->name = str_dup( "blood leg stone" );
    else
        leg->name = str_dup( "leg severed" );
  
    sprintf( buf2, "The severed %sleg of %s %s lying here.",
         IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ? "stone " : "",GET_NAME( ch ), isare );
    leg->description = str_dup( buf2 );
    sprintf( buf2, "the severed %sleg of %s",
         IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ? "stone " : "",GET_NAME( ch ) );
    leg->short_description = str_dup( buf2 );
    GET_OBJ_TYPE( leg ) = ITEM_WEAPON;
    GET_OBJ_WEAR( leg ) = ITEM_WEAR_TAKE + ITEM_WEAR_WIELD;
    GET_OBJ_EXTRA( leg ) = ITEM_NODONATE + ITEM_NOSELL;
    GET_OBJ_EXTRA2( leg ) = ITEM2_BODY_PART;
    GET_OBJ_VAL( leg, 0 ) = 0;  
    GET_OBJ_VAL( leg, 1 ) = 2;
	GET_OBJ_VAL( leg, 2 ) = 9;
    GET_OBJ_VAL( leg, 3 ) = 7;
    leg->setWeight( 7 );
    leg->worn_on = -1;
    if ( IS_NPC( ch ) )
        GET_OBJ_TIMER( leg ) = max_npc_corpse_time;
    else {
        GET_OBJ_TIMER( leg ) = max_pc_corpse_time;
    }   
    obj_to_room( leg, ch->in_room );
    if ( !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) &&
         GET_LEVEL( ch ) <= LVL_AMBASSADOR ) {                                          /* transfer character's leg EQ to room, if applicable */
        if ( GET_EQ( ch, WEAR_LEGS) )
        obj_to_room( unequip_char( ch, WEAR_LEGS, MODE_EQ ), ch->in_room );
        if ( GET_EQ( ch, WEAR_FEET) )
        obj_to_room( unequip_char( ch, WEAR_FEET, MODE_EQ ), ch->in_room );
        
        /** transfer implants to leg or corpse randomly**/
        if ( GET_IMPLANT( ch, WEAR_LEGS) && number(0,1)) {
        obj_to_obj( unequip_char( ch, WEAR_LEGS, MODE_IMPLANT ), leg );
        REMOVE_BIT( GET_OBJ_WEAR( leg ), ITEM_WEAR_TAKE );
        }
        if ( GET_IMPLANT( ch, WEAR_FEET) && number(0,1)) {
        obj_to_obj( unequip_char( ch, WEAR_FACE, MODE_IMPLANT ), leg );
        REMOVE_BIT( GET_OBJ_WEAR( leg ), ITEM_WEAR_TAKE );
        }
    } // end if !arena room
    break;

  
 
	case SKILL_BITE:
	case TYPE_BITE:
	sprintf( buf2, "The chewed up looking %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "chewed up" );
	break;

    case TYPE_BLUDGEON:
    case TYPE_POUND:
    case TYPE_PUNCH:
	sprintf( buf2, "The battered %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "battered" );
	break;

    case TYPE_CRUSH:
    case SPELL_PSYCHIC_CRUSH:
	sprintf( buf2, "The crushed %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "crushed" );
	break;

    case TYPE_CLAW:
	sprintf( buf2, "The shredded %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "shredded" );
	break;

    case TYPE_MAUL:
	sprintf( buf2, "The mauled %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "mauled" );
	break;

    case TYPE_THRASH:
	sprintf( buf2, "The %s of %s %s lying here, badly thrashed.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "thrashed" );
	break;

    case SKILL_BACKSTAB:
	sprintf( buf2, "The backstabbed %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "backstabbed" );
	break;

    case TYPE_PIERCE:
    case TYPE_STAB:
	sprintf( buf2, "The bloody %s of %s %s lying here, full of holes.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "stabbed" );
	break;

    case TYPE_GORE_HORNS:
	sprintf( buf2, "The gored %s of %s %s lying here in a pool of blood.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "gored" );
	break;

    case TYPE_TRAMPLING:
	sprintf( buf2, "The trampled %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "trampled" );
	break;

    case TYPE_TAIL_LASH:
	sprintf( buf2, "The lashed %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "lashed" );
	break;

    case SKILL_FEED:
	sprintf( buf2, "The drained %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "drained" );
	break;

    case TYPE_BLAST:
    case SPELL_MAGIC_MISSILE:
    case SKILL_ENERGY_WEAPONS:
    case SPELL_SYMBOL_OF_PAIN:
    case SPELL_DISRUPTION:
    case SPELL_PRISMATIC_SPRAY:
    case SKILL_DISCHARGE:
	sprintf( buf2, "The blasted %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "blasted" );
	break;

    case SKILL_PROJ_WEAPONS:
	sprintf( buf2, "The shot up %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "shot up" );
	break;

    case SKILL_ARCHERY:
	sprintf( buf2, "The pierced %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "pierced" );
	break;

    case SPELL_BURNING_HANDS:
    case SPELL_CALL_LIGHTNING:
    case SPELL_FIREBALL:
    case SPELL_FLAME_STRIKE:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_MICROWAVE:
    case SPELL_FIRE_BREATH:
    case SPELL_LIGHTNING_BREATH:
    case SPELL_FIRE_ELEMENTAL:
    case TYPE_ABLAZE:
    case SPELL_METEOR_STORM:
    case SPELL_FIRE_SHIELD:
    case SPELL_HELL_FIRE:
    case TYPE_FLAMETHROWER:
	sprintf( buf2, "The charred %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "charred" );
	break;

    case SKILL_ENERGY_FIELD:
    case SKILL_SELF_DESTRUCT:
	sprintf( buf2, "The smoking %s of %s %s lying here,", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "smoking" );
	break;

    case TYPE_BOILING_PITCH:
	sprintf( buf2, "The scorched %s of %s %s here.", typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "scorched" );
	break;

    case SPELL_STEAM_BREATH:
	sprintf( buf2, "The scalded %s of %s %s here.", typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "scalded" );
	break;

    case JAVELIN_OF_LIGHTNING:
	sprintf( buf2, "The %s of %s %s lying here, blasted and smoking.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "blasted" );
	break;

    case SPELL_CONE_COLD:
    case SPELL_CHILL_TOUCH:
    case TYPE_FREEZING:
    case SPELL_HELL_FROST:
    case SPELL_FROST_BREATH:
	sprintf( buf2, "The frozen %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "frozen" );
	break;

    case SPELL_SPIRIT_HAMMER:
    case SPELL_EARTH_ELEMENTAL:
	sprintf( buf2, "The smashed %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "smashed" );
	break;

    case SPELL_AIR_ELEMENTAL:
    case TYPE_RIP:
	sprintf( buf2, "The ripped apart %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "ripped apart" );
	break;

    case SPELL_WATER_ELEMENTAL:
	sprintf( buf2, "The drenched %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "drenched" );
	break;

    case SPELL_GAMMA_RAY:
    case SPELL_HALFLIFE:
	sprintf( buf2, "The radioactive %s of %s %s lying here.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "radioactive" );
	break;

    case SPELL_ACIDITY:
	sprintf( buf2, "The sizzling %s of %s %s lying here, dripping acid.", 
		 typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "sizzling" );
	break;

    case SPELL_GAS_BREATH:
	sprintf( buf2, "The %s of %s lie%s here, stinking of chlorine gas.", 
		 typebuf, GET_NAME( ch ), ISARE( typebuf ) );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "chlorinated" );
	break;

    case SKILL_TURN:
	sprintf( buf2, "The burned up %s of %s %s lying here, finally still.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "burned" );
	break;

    case TYPE_FALLING:
	sprintf( buf2, "The splattered %s of %s %s lying here.",
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "splattered" );
	break;

	// attack that tears the victim's spine out
    case SKILL_PILEDRIVE:
	sprintf( buf2, "The shattered, twisted %s of %s %s lying here.",
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "shattered" );
	if ( GET_MOB_VNUM( ch ) == 1511 ) {
	    if ( ( spine = read_object( 1541 ) ) )
		obj_to_room( spine, ch->in_room );
	} else {
	    spine = create_obj(  );
	    spine->shared = null_obj_shared;
	    spine->in_room = NULL;
	    if ( IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ) {
		spine->name = str_dup( "spine spinal column stone" );
		strcpy( buf2, "A stone spinal column is lying here." );
		spine->description = str_dup( buf2 );
		strcpy( buf2, "a stone spinal column" );
		spine->short_description = str_dup( buf2 );
		GET_OBJ_VAL( spine, 1 ) = 4;	
	    } else {
		spine->name = str_dup( "spine spinal column bloody" );
		strcpy( buf2, "A bloody spinal column is lying here." );
		spine->description = str_dup( buf2 );
		strcpy( buf2, "a bloody spinal column" );
		spine->short_description = str_dup( buf2 );
		GET_OBJ_VAL( spine, 1 ) = 2;	
	    }
      
	    GET_OBJ_TYPE( spine ) = ITEM_WEAPON;
	    GET_OBJ_WEAR( spine ) = ITEM_WEAR_TAKE + ITEM_WEAR_WIELD;
	    GET_OBJ_EXTRA( spine ) = ITEM_NODONATE + ITEM_NOSELL;
	    GET_OBJ_EXTRA2( spine ) = ITEM2_BODY_PART;
	    if ( GET_LEVEL( ch ) > number( 30, 59 ) )
		SET_BIT( GET_OBJ_EXTRA( spine ), ITEM_HUM );
	    GET_OBJ_VAL( spine, 0 ) = 0;
	    GET_OBJ_VAL( spine, 2 ) = 6;
	    GET_OBJ_VAL( spine, 3 ) = 5;
	    spine->setWeight(5);
	    GET_OBJ_MATERIAL( spine ) = MAT_BONE;
	    spine->worn_on = -1;
	    obj_to_room( spine, ch->in_room );
	}
	break;

	// attack that rips the victim's head off
    case SKILL_BEHEAD:
    case SKILL_PELE_KICK:
    case SKILL_CLOTHESLINE:
	sprintf( buf2, "The headless %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	sprintf( adj, "headless" );

	if ( IS_RACE( ch, RACE_BEHOLDER ) )
	    break;

	head = create_obj(  );
	head->shared = null_obj_shared;
	head->in_room = NULL;
	if ( IS_AFFECTED_2( ch, AFF2_PETRIFIED ) )
	    head->name = str_dup( "blood head skull stone" );
	else
	    head->name = str_dup( "blood head skull" );
    
	sprintf( buf2, "The severed %shead of %s %s lying here.", 
		 IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ? "stone " : "",GET_NAME( ch ), isare );
	head->description = str_dup( buf2 );
	sprintf( buf2, "the severed %shead of %s", 
		 IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ? "stone " : "",GET_NAME( ch ) );
	head->short_description = str_dup( buf2 );
	GET_OBJ_TYPE( head ) = ITEM_DRINKCON;
	GET_OBJ_WEAR( head ) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA( head ) = ITEM_NODONATE;
	GET_OBJ_EXTRA2( head ) = ITEM2_BODY_PART;
	GET_OBJ_VAL( head, 0 ) = 5;  /* Head full of blood */	
	GET_OBJ_VAL( head, 1 ) = 5;	
	GET_OBJ_VAL( head, 2 ) = 13;

	head->worn_on = -1;

	if(IS_AFFECTED_2( ch, AFF2_PETRIFIED )) {
		GET_OBJ_MATERIAL( head ) = MAT_STONE;
		head->setWeight( 25 );
	} else {
		GET_OBJ_MATERIAL( head ) = MAT_FLESH;
		head->setWeight( 10 );
	}

	if ( IS_NPC( ch ) )
	    GET_OBJ_TIMER( head ) = max_npc_corpse_time;
	else {
	    GET_OBJ_TIMER( head ) = max_pc_corpse_time;
	}
	obj_to_room( head, ch->in_room );
	if ( !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) && 
	     GET_LEVEL( ch ) <= LVL_AMBASSADOR ) {
         obj_data *o;
	    /* transfer character's head EQ to room, if applicable */
	    if ( GET_EQ( ch, WEAR_HEAD ) )
		obj_to_room( unequip_char( ch, WEAR_HEAD, MODE_EQ ), ch->in_room );
	    if ( GET_EQ( ch, WEAR_FACE ) )
		obj_to_room( unequip_char( ch, WEAR_FACE, MODE_EQ ), ch->in_room );
	    if ( GET_EQ( ch, WEAR_EAR_L ) )
		obj_to_room( unequip_char( ch, WEAR_EAR_L, MODE_EQ ), ch->in_room ); 
	    if ( GET_EQ( ch, WEAR_EAR_R ) )
		obj_to_room( unequip_char( ch, WEAR_EAR_R, MODE_EQ ), ch->in_room );
	    if ( GET_EQ( ch, WEAR_EYES ) )
		obj_to_room( unequip_char( ch, WEAR_EYES, MODE_EQ ), ch->in_room );
	    /** transfer implants to head **/
	    if ( ( o = GET_IMPLANT( ch, WEAR_HEAD ) ) ) {
		obj_to_obj( unequip_char( ch, WEAR_HEAD, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( o ), ITEM_WEAR_TAKE );
	    }
	    if ( ( o = GET_IMPLANT( ch, WEAR_FACE ) ) ) {
		obj_to_obj( unequip_char( ch, WEAR_FACE, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( o ), ITEM_WEAR_TAKE );
	    }
	    if ( ( o = GET_IMPLANT( ch, WEAR_EAR_L ) ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EAR_L, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( o ), ITEM_WEAR_TAKE );
	    }
	    if ( ( o = GET_IMPLANT( ch, WEAR_EAR_R ) ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EAR_R, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( o ), ITEM_WEAR_TAKE );
	    }
	    if ( (o = GET_IMPLANT( ch, WEAR_EYES ) ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EYES, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( o ), ITEM_WEAR_TAKE );
	    }
	} // end if !arena room
	break;

	// attack that rips the victim's heart out
    case SKILL_LUNGE_PUNCH:
	sprintf( buf2, "The maimed %s of %s %s lying here.", typebuf,GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "maimed" );

	heart = create_obj(  );
	heart->shared = null_obj_shared;
	heart->in_room = NULL;
	if ( IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ) {
	    GET_OBJ_TYPE( heart ) = ITEM_OTHER;
	    heart->name = str_dup( "heart stone" );
	    sprintf( buf2, "the stone heart of %s", GET_NAME( ch ) );
	    heart->short_description = str_dup( buf2 );
	} else {
	    GET_OBJ_TYPE( heart ) = ITEM_FOOD;
	    heart->name = str_dup( "heart bloody" );
	    sprintf( buf2, "the bloody heart of %s", GET_NAME( ch ) );
	    heart->short_description = str_dup( buf2 );
	}

	sprintf( buf2, "The %sheart of %s %s lying here.", 
		 IS_AFFECTED_2( ch, AFF2_PETRIFIED ) ? "stone " : "",GET_NAME( ch ), isare );
	heart->description = str_dup( buf2 );

	GET_OBJ_WEAR( heart ) = ITEM_WEAR_TAKE + ITEM_WEAR_HOLD;
	GET_OBJ_EXTRA( heart ) = ITEM_NODONATE;
	GET_OBJ_EXTRA2( heart ) = ITEM2_BODY_PART;
	GET_OBJ_VAL( heart, 0 ) = 10;
	if ( IS_DEVIL( ch ) || IS_DEMON( ch ) || IS_LICH( ch ) ) {
	    if ( GET_CLASS( ch ) == CLASS_GREATER || GET_CLASS( ch ) == CLASS_ARCH ||
		 GET_CLASS( ch ) == CLASS_DUKE || GET_CLASS( ch ) == CLASS_DEMON_PRINCE ||
		 GET_CLASS( ch ) == CLASS_DEMON_LORD || GET_CLASS( ch ) == CLASS_LESSER ||
		 GET_LEVEL( ch ) > 30 ) {
		GET_OBJ_VAL( heart, 1 ) = GET_LEVEL( ch );	
		if ( GET_CLASS( ch ) == CLASS_LESSER )
		    GET_OBJ_VAL( heart, 1 )  >>= 1;	
		GET_OBJ_VAL( heart, 2 ) = SPELL_ESSENCE_OF_EVIL;
	    } else {
		GET_OBJ_VAL( heart, 1 ) = 0;	
		GET_OBJ_VAL( heart, 2 ) = 0;	
	    }
	} else {
	    GET_OBJ_VAL( heart, 1 ) = 0;	
	    GET_OBJ_VAL( heart, 2 ) = 0;	
	}
	heart->setWeight( 0 );
	heart->worn_on = -1;

	if ( IS_NPC( ch ) )
	    GET_OBJ_TIMER( heart ) = max_npc_corpse_time;
	else {
	    GET_OBJ_TIMER( heart ) = max_pc_corpse_time;
	}
	obj_to_room( heart, ch->in_room );
	break;

    case SKILL_IMPALE:
	sprintf( buf2, "The run through %s of %s %s lying here.",
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "impaled" );
	break;

    case TYPE_DROWNING:
	if ( SECT_TYPE( ch->in_room ) == SECT_WATER_NOSWIM ||
	     SECT_TYPE( ch->in_room ) == SECT_WATER_SWIM ||
	     SECT_TYPE( ch->in_room ) == SECT_UNDERWATER )
	    sprintf( buf2, "The waterlogged %s of %s %s lying here.", typebuf,
		     GET_NAME( ch ), ISARE( typebuf ) );
	else
	    sprintf( buf2, "The %s of %s %s lying here.", typebuf,
		     GET_NAME( ch ), ISARE( typebuf ) );
    
	corpse->description = str_dup( buf2 );
	strcpy( adj, "drowned" );
	break;

    case SKILL_DRAIN:
	sprintf( buf2, "The withered %s of %s %s lying here.", typebuf,
		 GET_NAME( ch ), ISARE( typebuf ) );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "withered" );
	break;

    default:
	sprintf( buf2, "The %s of %s %s lying here.", 
		 typebuf, GET_NAME( ch ), isare );
	corpse->description = str_dup( buf2 );
	strcpy( adj, "" );
	break;
    }

    //  make the short description
    sprintf( buf2, "the %s%s%s of %s", adj, *adj ? " " : "", typebuf, GET_NAME( ch ) );
    corpse->short_description = str_dup( buf2 );

    // make the alias list
    strcat( strcat( strcat( strcat( namestr, " " ), adj ), " " ), ch->player.name );
    if ( namestr[strlen( namestr )] == ' ' )
	namestr[strlen( namestr )] = '\0';
    corpse->name = str_dup( namestr );
  
    // now flesh out the other vairables on the corpse
    GET_OBJ_TYPE( corpse ) = ITEM_CONTAINER;
    GET_OBJ_WEAR( corpse ) = ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA( corpse ) = ITEM_NODONATE;
    if ( PLR_FLAGGED( ch, PLR_TESTER ) )
	SET_BIT( GET_OBJ_EXTRA( corpse ), ITEM2_UNAPPROVED );
    GET_OBJ_VAL( corpse, 0 ) = 0;	/* You can't store stuff in a corpse */
    GET_OBJ_VAL( corpse, 3 ) = 1;	/* corpse identifier */
    corpse->setWeight( GET_WEIGHT( ch ) + IS_CARRYING_W( ch ) );
    corpse->contains = NULL;

    if ( IS_NPC( ch ) ) {
	GET_OBJ_TIMER( corpse ) = max_npc_corpse_time;
	CORPSE_IDNUM( corpse ) = - ch->mob_specials.shared->vnum;
    } else {
	GET_OBJ_TIMER( corpse ) = max_pc_corpse_time;
	CORPSE_IDNUM( corpse ) =GET_IDNUM( ch );
    }

    if ( killer ) {
	if ( IS_NPC( killer ) )
	    CORPSE_KILLER( corpse ) = - GET_MOB_VNUM( killer );
	else
	    CORPSE_KILLER( corpse ) = GET_IDNUM( killer );
    } else if ( dam_object )
	CORPSE_KILLER( corpse ) = DAM_OBJECT_IDNUM( dam_object );
  
    // if non-arena room, transfer eq to corpse
    if ( ( !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) || IS_MOB( ch ) ) &&
	 GET_LEVEL( ch ) < LVL_AMBASSADOR ) {

	/* transfer character's inventory to the corpse */
	corpse->contains = ch->carrying;
	for ( o = corpse->contains; o != NULL; o = o->next_content )
	    o->in_obj = corpse;
	object_list_new_owner( corpse, NULL );
    
	/* transfer character's equipment to the corpse */
	for ( i = 0; i < NUM_WEARS; i++ ) {
	    if ( GET_EQ( ch, i ) )
		obj_to_obj( unequip_char( ch, i, MODE_EQ ), corpse );
	    if ( GET_IMPLANT( ch, i ) ) {
		REMOVE_BIT( GET_OBJ_WEAR( GET_IMPLANT( ch, i ) ), ITEM_WEAR_TAKE );
		if ( ch->in_room->zone->number == 400 ||
		     ch->in_room->zone->number == 320 )
		    extract_obj( unequip_char( ch, i, MODE_IMPLANT ) );
		else
		    obj_to_obj( unequip_char( ch, i, MODE_IMPLANT ), corpse );
	    }
	}
    
	/* transfer gold */
	if ( GET_GOLD( ch ) > 0 ) {
	    /* following 'if' clause added to fix gold duplication loophole */
	    if ( IS_NPC( ch ) || ( !IS_NPC( ch ) && ch->desc ) ) {
		if ( ( money = create_money( GET_GOLD( ch ), 0 ) ) )
		    obj_to_obj( money, corpse );
	    }
	    GET_GOLD( ch ) = 0;
	}
	if ( GET_CASH( ch ) > 0 ) {
	    /* following 'if' clause added to fix gold duplication loophole */
	    if ( IS_NPC( ch ) || ( !IS_NPC( ch ) && ch->desc ) ) {
		if ( ( money = create_money( GET_CASH( ch ), 1 ) ) )
		    obj_to_obj( money, corpse );
	    }
	    GET_CASH( ch ) = 0;
	}
	ch->carrying = NULL;
	IS_CARRYING_N( ch ) = 0;
	IS_CARRYING_W( ch ) = 0;
    
	if ( !IS_NPC( ch ) && ch->in_room->zone->number != 400 &&
	     ch->in_room->zone->number != 320 )
	    Crash_delete_file( GET_NAME( ch ), IMPLANT_FILE );

	for ( o = corpse->contains; o; o = next_o ) {
	    next_o = o->next_content;
	    if ( OBJ_TYPE( o, ITEM_SCRIPT ) )
		extract_obj( o );
	}
	// Save the char to prevent duping of eq.
	save_char(ch, NULL);
	Crash_crashsave(ch);

    } else { // arena kills do not drop EQ
	rentcost = Crash_rentcost( ch, FALSE, 1 );
	if ( rentcost < 0 )
	    rentcost = -rentcost;

	Crash_rentsave( ch, rentcost, RENT_RENTED );
    }

    // leave no corpse behind
    if ( NON_CORPOREAL_UNDEAD( ch ) || GET_MOB_SPEC(ch) == fate) {
	while ( ( o = corpse->contains ) ) {
	    obj_from_obj( o );
	    obj_to_room( o, ch->in_room );
	}
	extract_obj( corpse );
    } else
	obj_to_room( corpse, ch->in_room );
}

#undef __combat_code__
#undef __combat_utils__
