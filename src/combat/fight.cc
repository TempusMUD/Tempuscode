/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: fight.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __fight_c__

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

#include <iostream>

int corpse_state = 0;

/* The Fight related routines */
obj_data * get_random_uncovered_implant ( char_data *ch, int type = -1 );


void 
appear( struct char_data * ch, struct char_data *vict )
{
    char *to_char = NULL;
    int found = 0;
    //  int could_see = CAN_SEE( vict, ch );

    if ( !AFF_FLAGGED( vict, AFF_DETECT_INVIS ) ) {
	if ( affected_by_spell( ch, SPELL_INVISIBLE ) ) {
	    affect_from_char( ch, SPELL_INVISIBLE );
	    send_to_char( "Your invisibility spell has expired.\r\n", ch );
	    found = 1;
	}
	if ( affected_by_spell( ch, SPELL_TRANSMITTANCE ) ) {
	    affect_from_char( ch, SPELL_TRANSMITTANCE );
	    send_to_char( "Your transparency has expired.\r\n", ch );
	    found = 1;
	}
    }

    if ( IS_ANIMAL( vict ) &&
	 affected_by_spell( ch, SPELL_INVIS_TO_ANIMALS ) ) {
	affect_from_char( ch, SPELL_INVIS_TO_ANIMALS );
	send_to_char( "Your animal invisibility has expired.\r\n", ch );
	found = 1;
    }
    if ( IS_UNDEAD( vict ) &&
	 affected_by_spell( ch, SPELL_INVIS_TO_UNDEAD ) ) {
	affect_from_char( ch, SPELL_INVIS_TO_UNDEAD );
	send_to_char( "Your invisibility to undead has expired.\r\n", ch );
	found = 1;
    }

    if ( AFF_FLAGGED( ch, AFF_HIDE ) ) {
	REMOVE_BIT( AFF_FLAGS( ch ), AFF_HIDE );
	found = 1;
    }
  
    if ( !IS_NPC( vict ) && !IS_NPC( ch ) &&
	 GET_REMORT_GEN( ch ) > GET_REMORT_GEN( vict ) &&
	 GET_REMORT_INVIS( ch ) > GET_LEVEL( vict ) ) {
	GET_REMORT_INVIS( ch ) = GET_LEVEL( vict );
	sprintf( buf, "Your remort invisibility has dropped to level %d.\n",
		 GET_REMORT_INVIS( ch ) );
	send_to_char( buf, ch );
	found = 1;
    }
      
    if ( found ) {
	if ( GET_LEVEL( ch ) < LVL_AMBASSADOR ) {
	    act( "$n suddenly appears, seemingly from nowhere.", 
		 TRUE, ch, 0, 0, TO_ROOM );
	    if ( to_char )
		send_to_char( to_char, ch );
	    else
		send_to_char( "You fade into visibility.\r\n", ch );
	} else
	    act( "You feel a strange presence as $n appears, seemingly from nowhere.",
		 FALSE, ch, 0, 0, TO_ROOM );
    }
}


void 
load_messages( void )
{
    FILE *fl;
    int i, type;
    struct message_type *messages;
    char chk[128];

    if ( !( fl = fopen( MESS_FILE, "r" ) ) ) {
	sprintf( buf2, "Error reading combat message file %s", MESS_FILE );
	perror( buf2 );
	safe_exit( 1 );
    }
    for ( i = 0; i < MAX_MESSAGES; i++ ) {
	fight_messages[i].a_type = 0;
	fight_messages[i].number_of_attacks = 0;
	fight_messages[i].msg = 0;
    }


    fgets( chk, 128, fl );
    while ( !feof( fl ) && ( *chk == '\n' || *chk == '*' ) )
	fgets( chk, 128, fl );

    while ( *chk == 'M' ) {
	fgets( chk, 128, fl );
	sscanf( chk, " %d\n", &type );
	for ( i = 0; ( i < MAX_MESSAGES ) && ( fight_messages[i].a_type != type ) &&
		  ( fight_messages[i].a_type ); i++ );
	if ( i >= MAX_MESSAGES ) {
	    fprintf( stderr, "Too many combat messages.  Increase MAX_MESSAGES and recompile." );
	    safe_exit( 1 );
	}
	CREATE( messages, struct message_type, 1 );
	fight_messages[i].number_of_attacks++;
	fight_messages[i].a_type = type;
	messages->next = fight_messages[i].msg;
	fight_messages[i].msg = messages;

	messages->die_msg.attacker_msg = fread_action( fl, i );
	messages->die_msg.victim_msg = fread_action( fl, i );
	messages->die_msg.room_msg = fread_action( fl, i );
	messages->miss_msg.attacker_msg = fread_action( fl, i );
	messages->miss_msg.victim_msg = fread_action( fl, i );
	messages->miss_msg.room_msg = fread_action( fl, i );
	messages->hit_msg.attacker_msg = fread_action( fl, i );
	messages->hit_msg.victim_msg = fread_action( fl, i );
	messages->hit_msg.room_msg = fread_action( fl, i );
	messages->god_msg.attacker_msg = fread_action( fl, i );
	messages->god_msg.victim_msg = fread_action( fl, i );
	messages->god_msg.room_msg = fread_action( fl, i );
	fgets( chk, 128, fl );
	while ( !feof( fl ) && ( *chk == '\n' || *chk == '*' ) )
	    fgets( chk, 128, fl );
    }

    sprintf( buf, "Top message number loaded: %d.", type );
    slog( buf );
    fclose( fl );
}


void 
update_pos( struct char_data * victim )
{

    if ( GET_HIT( victim ) > 0 && GET_POS( victim ) == POS_SLEEPING )
	GET_POS( victim ) = POS_RESTING;
    else if ( ( GET_HIT( victim ) > 0 ) && ( GET_POS( victim ) > POS_STUNNED ) &&
	      FIGHTING( victim ) ) {
	if ( ( victim->desc && victim->desc->wait <= 0 ) ||
	     ( IS_NPC( victim ) && GET_MOB_WAIT( victim ) <= 0 ) )
	    GET_POS( victim ) = POS_FIGHTING;
	else
	    return;
    } else if ( GET_HIT( victim ) > 0 ) {
	if ( victim->in_room->isOpenAir() )
	    GET_POS( victim ) = POS_FLYING;
	else
	    GET_POS( victim ) = POS_STANDING;
    }
    else if ( GET_HIT( victim ) <= -11 )
	GET_POS( victim ) = POS_DEAD;
    else if ( GET_HIT( victim ) <= -6 )
	GET_POS( victim ) = POS_MORTALLYW;
    else if ( GET_HIT( victim ) <= -3 )
	GET_POS( victim ) = POS_INCAP;
    else
	GET_POS( victim ) = POS_STUNNED;
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


/* start one char fighting another ( yes, it is horrible, I know... )  */
void 
set_fighting( struct char_data * ch, struct char_data * vict, int aggr ) 
{
    if ( ch == vict )
	return;

    if ( FIGHTING( ch ) ) {
	slog( "SYSERR: FIGHTING( ch ) != NULL in set_fighting(  )." );
	return;
    }

    ch->next_fighting = combat_list;
    combat_list = ch;

    FIGHTING( ch ) = vict;
    GET_POS( ch ) = POS_FIGHTING;
  
    if ( aggr == TRUE && !IS_NPC( vict ) ) {

	if ( IS_NPC( ch ) ) {
	    if ( AFF_FLAGGED( ch, AFF_CHARM ) && ch->master && !IS_NPC( ch->master ) &&
		 ( !MOB_FLAGGED( ch, MOB_MEMORY ) || !char_in_memory( vict, ch ) ) ) {
		if ( !PLR_FLAGGED( ch->master, PLR_TOUGHGUY ) )
		    check_toughguy( ch->master, vict, 0 );
		else if ( ( GET_LEVEL( ch->master ) > GET_LEVEL( vict ) ) ||
			  ( GET_LEVEL( vict ) < 4 ) || 
			  ( IS_REMORT( ch->master ) && !IS_REMORT( vict ) ) )
		    check_killer( ch->master, vict, "charmie" );
	    }
	}
	else { /*if ( !IS_NPC( ch ) ) {*/
	    if ( PLR_FLAGGED( ch, PLR_NOPK ) ) {
		send_to_char( "A small dark shape flies in from the future and sticks to your tongue.\r\n", ch );
		return;
	    }
	    if ( PLR_FLAGGED( vict, PLR_NOPK ) ) {
		send_to_char( "A small dark shape flies in from the future and sticks to your eye.\r\n", ch );
		return;
	    }
	    if ( GET_LEVEL( ch ) <= LVL_PROTECTED && !PLR_FLAGGED( ch, PLR_TOUGHGUY ) ) {
		send_to_char( "You are currently under new player protection, which expires at level 6.\r\nYou cannot attack other players while under this protection.\r\n", ch );
		return;
	    }
      
	    if ( !PLR_FLAGGED( ch, PLR_TOUGHGUY ) || 
		 !PLR_FLAGGED( ch, PLR_REMORT_TOUGHGUY ) )
		check_toughguy( ch, vict, 0 );
	    if ( ( GET_LEVEL( ch ) > GET_LEVEL( vict ) ) ||
		 ( GET_LEVEL( vict ) < 4 ) || ( IS_REMORT( ch ) && !IS_REMORT( vict ) ) )
		check_killer( ch, vict, "normal" );
      
	    if ( GET_LEVEL( vict ) <= LVL_PROTECTED && 
		 !PLR_FLAGGED( vict, PLR_TOUGHGUY ) &&
		 GET_LEVEL( ch ) < LVL_IMMORT ) {
		act( "$N is currently under new character protection.",
		     FALSE, ch, 0, vict, TO_CHAR );
		act( "You are protected by the gods against $n's attack!",
		     FALSE, ch, 0, vict, TO_VICT );	  
		sprintf( buf, "%s protected against %s ( set_fighting ) at %d\n",
			 GET_NAME( vict ), GET_NAME( ch ), vict->in_room->number );
		slog( buf );
		FIGHTING( ch ) = NULL;
		GET_POS( ch ) = POS_STANDING;
		return;
	    }
	}
    }
}

/* remove a char from the list of fighting chars */
void 
stop_fighting( struct char_data * ch )
{
    struct char_data *temp;

    if ( ch == next_combat_list )
	next_combat_list = ch->next_fighting;

    REMOVE_FROM_LIST( ch, combat_list, next_fighting );
    ch->next_fighting = NULL;
    FIGHTING( ch ) = NULL;

    if ( ch->in_room && ch->in_room->isOpenAir() )
	GET_POS( ch ) = POS_FLYING;
    else if ( !IS_NPC( ch ) )
	GET_POS( ch ) = POS_STANDING;
    else {
	if ( IS_AFFECTED( ch, AFF_CHARM ) && IS_UNDEAD( ch ) )
	    GET_POS( ch ) = POS_STANDING;
	else if ( !HUNTING( ch ) )
	    GET_POS( ch ) = GET_DEFAULT_POS( ch );
	else 
	    GET_POS( ch ) = POS_STANDING;
    }

    REMOVE_BIT( AFF3_FLAGS( ch ), AFF3_FEEDING );
    update_pos( ch );
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
    }
    else if ( attacktype == SKILL_DRAIN ) {
	strcpy( typebuf, "husk" );
	strcpy( namestr, typebuf );
    }
    else {
	strcpy( typebuf, "corpse" );
	strcpy( namestr, typebuf );
    }
  
    if ( AFF2_FLAGGED( ch, AFF2_PETRIFIED ) ) {
	strcat( namestr, " stone" );
	sprintf( typebuf, "stone %s", typebuf );
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
	    if ( GET_IMPLANT( ch, WEAR_HEAD ) ) {
		obj_to_obj( unequip_char( ch, WEAR_HEAD, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( head ), ITEM_WEAR_TAKE );
	    }
	    if ( GET_IMPLANT( ch, WEAR_FACE ) ) {
		obj_to_obj( unequip_char( ch, WEAR_FACE, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( head ), ITEM_WEAR_TAKE );
	    }
	    if ( GET_IMPLANT( ch, WEAR_EAR_L ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EAR_L, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( head ), ITEM_WEAR_TAKE );
	    }
	    if ( GET_IMPLANT( ch, WEAR_EAR_R ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EAR_R, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( head ), ITEM_WEAR_TAKE );
	    }
	    if ( GET_IMPLANT( ch, WEAR_EYES ) ) {
		obj_to_obj( unequip_char( ch, WEAR_EYES, MODE_IMPLANT ), head );
		REMOVE_BIT( GET_OBJ_WEAR( head ), ITEM_WEAR_TAKE );
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
    if ( NON_CORPOREAL_UNDEAD( ch ) ) {
	while ( ( o = corpse->contains ) ) {
	    obj_from_obj( o );
	    obj_to_room( o, ch->in_room );
	}
	extract_obj( corpse );
    } else
	obj_to_room( corpse, ch->in_room );
}


/* When ch kills victim */
void 
change_alignment( struct char_data * ch, struct char_data * victim )
{
    GET_ALIGNMENT( ch ) += -( GET_ALIGNMENT( victim ) / 100 );
    GET_ALIGNMENT( ch ) = MAX( -1000, GET_ALIGNMENT( ch ) );
    GET_ALIGNMENT( ch ) = MIN( 1000, GET_ALIGNMENT( ch ) );
}

void 
death_cry( struct char_data * ch )
{
    struct char_data *helper = NULL;
    struct room_data * adjoin_room = NULL;
    int door;
    struct room_data *was_in = NULL;
    int found = 0;

    if (IS_NPC( ch ) && MOB_SHARED( ch )->death_cry) {
            sprintf(buf, "$n %s", MOB_SHARED( ch )->death_cry);
	    act( buf, FALSE, ch, 0, 0, TO_ROOM );
    }

    else if ( IS_GHOUL( ch ) || IS_WIGHT( ch ) || IS_MUMMY( ch ) )
	act( "$n falls lifeless to the floor with a shriek.", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else if ( IS_SKELETON( ch ) )
	act( "$n clatters noisily into a lifeless heap.", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else if ( IS_GHOST( ch ) || IS_SHADOW( ch ) || IS_WRAITH( ch ) || IS_SPECTRE( ch ) )
	act( "$n vanishes into the void with a terrible shriek.", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else if ( IS_VAMPIRE( ch ) )
	act( "$n screams as $e is consumed in a blazing fire!", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else if ( IS_LICH( ch ) )
	act( "A roar fills your ears as $n is ripped into the void!", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else if ( IS_UNDEAD( ch ) )
	act( "Your skin crawls as you hear $n's final shriek.", 
	     FALSE, ch, 0, 0, TO_ROOM );
    else {
	for ( helper = ch->in_room->people; helper; helper = helper->next_in_room ) {
	    if ( helper == ch )
		continue;
	    found = 0;

	    if ( !found && IS_BARB( helper ) && !number( 0, 1 ) ) {
		found = 1;
		act( "You feel a rising bloodlust as you hear $n's death cry.",
		     FALSE, ch, 0, helper, TO_VICT );
	    }

	    if ( !found )
		act( "Your blood freezes as you hear $n's death cry.", 
		     FALSE, ch, 0, helper, TO_VICT );
	}
    }

    for ( helper = ch->in_room->people; helper; helper = helper->next_in_room ) {
	if ( ch != helper && GET_POS( helper ) == POS_SLEEPING && 
	     !PLR_FLAGGED( helper, PLR_OLC | PLR_WRITING ) && 
	     !AFF_FLAGGED( helper, AFF_SLEEP ) ) {
	    GET_POS( helper ) = POS_RESTING;
	    send_to_char( "You are awakened by a loud scream.\r\n", helper );
	}
    }

    was_in = ch->in_room;

    for ( door = 0; door < NUM_OF_DIRS && !found; door++ ) {
	if ( CAN_GO( ch, door ) && ch->in_room != EXIT( ch, door )->to_room ) {
	    ch->in_room = was_in->dir_option[door]->to_room;
	    sprintf( buf, "Your blood freezes as you hear someone's death cry from %s.",
		     from_dirs[door] );
	    act( buf, FALSE, ch, 0, 0, TO_ROOM );
	    adjoin_room = ch->in_room;
	    ch->in_room = was_in;
	    if ( adjoin_room->dir_option[rev_dir[door]] &&
		 adjoin_room->dir_option[rev_dir[door]]->to_room == was_in ) {
		for ( helper = adjoin_room->people; helper; 
		      helper=helper->next_in_room ) {
		    if ( IS_MOB( helper ) && !MOB_FLAGGED( helper, MOB_SENTINEL ) && 
			 !FIGHTING( helper ) && AWAKE( helper ) &&
			 ( MOB_FLAGGED( helper, MOB_HELPER )  ||
			   helper->mob_specials.shared->func ==
			   cityguard ) &&
			 number( 0, 40 ) < GET_LEVEL( helper ) ) {
			if ( ( !ROOM_FLAGGED( ch->in_room, ROOM_FLAME_FILLED ) ||
			       CHAR_WITHSTANDS_FIRE( helper ) ) &&
			     ( !ROOM_FLAGGED( ch->in_room, ROOM_ICE_COLD ) ||
			       CHAR_WITHSTANDS_COLD( helper ) ) &&
			     ( !IS_DARK( ch->in_room ) || CAN_SEE_IN_DARK( helper ) ) ) {

			    int move_result = do_simple_move( helper, rev_dir[door], MOVE_RUSH, 1 );

			    if ( move_result == 0 ) {
				found = number( 0, 1 );
				break;
			    }
			    else if ( move_result == 2 ) {
				found = 1;
				break;
			    }
			}
		    }
		}
	    }
	}
    }
}



void 
raw_kill( struct char_data * ch, struct char_data *killer, int attacktype )
{

    if ( FIGHTING( ch ) )
	stop_fighting( ch );

    death_cry( ch );

    make_corpse( ch, killer, attacktype );

    while ( ch->affected )
	affect_remove( ch, ch->affected );

    REMOVE_BIT( AFF2_FLAGS( ch ), AFF2_PETRIFIED );

    if ( IS_NPC( ch ) )
	ch->mob_specials.shared->kills++;
  
    extract_char( ch, 1 );
}



void 
die( struct char_data *ch, struct char_data *killer,
     int attacktype, int is_humil )
{
  
    /*
      if ( IS_NPC( ch ) &&
      GET_MOB_SPEC( ch ) ) {
      if ( GET_MOB_SPEC( ch ) ( killer, ch, 0, NULL, SPECIAL_DEATH ) ) {
      return;
      }
      }
    */
  
    if ( !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) && killer &&
	 !PLR_FLAGGED( killer, PLR_KILLER ) )
	gain_exp( ch, -( GET_EXP( ch ) >> 3 ) );
  
    if ( PLR_FLAGGED( ch, PLR_KILLER ) && GET_LEVEL( ch ) < LVL_AMBASSADOR ) {
	GET_EXP( ch ) = MAX( 0, MIN( GET_EXP( ch ) - ( GET_LEVEL( ch ) * GET_LEVEL( ch ) ),
				     exp_scale[GET_LEVEL( ch ) - 2] ) );
    
	GET_LEVEL( ch ) = MAX( 1, GET_LEVEL( ch ) - 1 );
	GET_CHA( ch ) = MAX( 3, GET_CHA( ch ) -2 );
	GET_MAX_HIT( ch ) = MAX( 0, GET_MAX_HIT( ch ) - GET_LEVEL( ch ) );
    }
  
    if ( !IS_NPC( ch ) && !ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) ) {
	if ( ch != killer )
	    REMOVE_BIT( PLR_FLAGS( ch ), PLR_KILLER | PLR_THIEF );
    
	if ( GET_LEVEL( ch ) > 10 ) {
	    if ( GET_LIFE_POINTS( ch ) > 0 ) {
		GET_LIFE_POINTS( ch ) = 
		    MAX( 0, GET_LIFE_POINTS( ch ) - number( 1, ( GET_LEVEL( ch ) >> 3 ) ) );
	    }
	    else if ( !number( 0, 3 ) )
		GET_CON( ch ) = MAX( 3, GET_CON( ch ) - 1 );
	    else if ( GET_LEVEL( ch ) > number( 20, 50 ) )
		GET_MAX_HIT( ch ) = MAX( 0, GET_MAX_HIT( ch ) - dice( 3, 5 ) );
	}
	if ( IS_CYBORG( ch ) ) {
	    GET_TOT_DAM( ch ) = 0;
	    GET_BROKE( ch ) = 0;
	}
	GET_PC_DEATHS( ch )++;
    }

    REMOVE_BIT( AFF2_FLAGS( ch ), AFF2_ABLAZE );
    REMOVE_BIT( AFF3_FLAGS( ch ), AFF3_SELF_DESTRUCT );
    raw_kill( ch, killer, attacktype );
}

void perform_gain_kill_exp( struct char_data *ch, struct char_data *victim, float multiplier );

void
group_gain( struct char_data *ch, struct char_data *victim )
{
    struct char_data *member, *leader;
    int total_levs = 0;
    int total_pc_mems = 0;
    float mult = 0;
    float mult_mod = 0;

    if ( ! ( leader = ch->master ) )
	leader = ch;

    for ( member = ch->in_room->people; member; member = member->next_in_room ) {
	if ( AFF_FLAGGED( member, AFF_GROUP ) && ( member == leader || leader == member->master ) ) {
	    total_levs += GET_LEVEL( member ) + ( IS_NPC( member ) ? 0 : GET_REMORT_GEN( member ) << 3 );
	    if ( !IS_NPC( member ) )
		total_pc_mems++;
	}
    }

    for ( member = ch->in_room->people; member; member = member->next_in_room ) {
	if ( AFF_FLAGGED( member, AFF_GROUP ) && ( member == leader || leader == member->master ) ) {
	    mult = ( float ) GET_LEVEL( member ) + ( IS_NPC( member ) ? 0 : GET_REMORT_GEN( member ) << 3 );
	    mult /= ( float ) total_levs;
	    
	    if ( total_pc_mems ) {
		mult_mod = 1 - mult;
		mult_mod *= mult;
		sprintf( buf, "Your group gain is %d%% + bonus %d%%.\n",
			 ( int ) ( ( float ) mult * 100 ),
			 ( int ) ( ( float ) mult_mod * 100 ) );
		send_to_char( buf, member );
	    }

	    perform_gain_kill_exp( member, victim, mult );
	}
    }
}
    

void
perform_gain_kill_exp( struct char_data *ch, struct char_data *victim, float multiplier )
{

    int exp = 0;


    if ( !IS_NPC( victim ) )
	exp = ( int ) MIN( max_exp_gain, ( GET_EXP( victim ) * multiplier ) / 8 );
    else
	exp = ( int ) MIN( max_exp_gain, ( GET_EXP( victim ) * multiplier ) / 3 );
  
    /* Calculate level-difference bonus */
    if ( IS_NPC( ch ) )
	exp += MAX( 0, ( exp * MIN( 4, ( GET_LEVEL( victim ) - 
					 GET_LEVEL( ch ) ) ) ) >> 3 );
    else 
	exp += MAX( 0, ( exp * MIN( 8, ( GET_LEVEL( victim ) - 
					 GET_LEVEL( ch ) ) ) ) >> 3 );
    exp = MAX( exp, 1 );
    exp = MIN( max_exp_gain, exp );
  
    if ( !IS_NPC( ch ) ) {
	exp = MIN( ( ( exp_scale[( int )( GET_LEVEL( ch ) + 1 )] - 
		       exp_scale[( int )GET_LEVEL( ch )] ) >> 3 ), exp );
	/* exp is limited to 12.5% of needed from level to ( level + 1 ) */
    
	if ( ( exp + GET_EXP( ch ) ) > exp_scale[GET_LEVEL( ch ) + 2] )
	    exp = 
		( ( ( exp_scale[GET_LEVEL( ch ) + 2] - exp_scale[GET_LEVEL( ch ) + 1] )
		    >> 1 ) + exp_scale[GET_LEVEL( ch ) + 1] ) - GET_EXP( ch );
    
	if ( !IS_NPC( victim ) ) {
	    if ( ROOM_FLAGGED( ch->in_room, ROOM_ARENA ) )
		exp = 1;
	    else 
		exp >>= 8;
	} 
    }
    if ( IS_THIEF( ch ) )  // thieves gain more exp
	exp += ( exp * 25 ) / 100;
    if ( IS_CLERIC( ch ) && !IS_GOOD( ch ) )
	exp -= ( exp * 15 ) / 100;
    if ( IS_KNIGHT( ch ) && !IS_GOOD( ch ) ) 
	exp -= ( exp * 25 ) / 100;

  
    if ( !IS_NPC( ch ) && IS_REMORT( ch ) )
	exp -= ( exp * GET_REMORT_GEN( ch ) ) / ( GET_REMORT_GEN( ch ) + 2 );
  
    if ( IS_GOOD( ch ) && ( IS_CLERIC( ch ) || IS_KNIGHT( ch ) ) && IS_GOOD( victim ) ) {    // good clerics & knights penalized
	exp = -exp;
	act( "You feel a sharp pang of remorse for $N's death.", FALSE, ch, 0, victim, TO_CHAR );
    }
  
    if ( GET_LEVEL( victim ) >= LVL_AMBASSADOR ||
	 GET_LEVEL( ch ) >= LVL_AMBASSADOR )
	exp = 0;
  
    if ( IS_NPC( victim ) && !IS_NPC( ch ) && 
	 ( GET_EXP( victim ) < 0 || exp > 5000000 ) ) {
	sprintf( buf, "%s Killed %s( %d ) for exp: %d.", GET_NAME( ch ), 
		 GET_NAME( victim ), GET_EXP( victim ), exp );
	slog( buf );
    }
    if ( exp > ( ( exp_scale[GET_LEVEL( ch )+1] - GET_EXP( ch ) ) / 10 ) ) {
	sprintf( buf2, "%s%sYou have gained much experience.%s\r\n", 
		 CCYEL( ch, C_NRM ), CCBLD( ch, C_CMP ), CCNRM( ch, C_SPR ) );
	send_to_char( buf2, ch );
    } else if ( exp > 1 ) {
	sprintf( buf2, "%s%sYou have gained experience.%s\r\n", 
		 CCYEL( ch, C_NRM ), CCBLD( ch, C_CMP ), CCNRM( ch, C_SPR ) );
	send_to_char( buf2, ch );
    } else if ( exp < 0 ) {
	sprintf( buf2, "%s%sYou have lost experience.%s\r\n", 
		 CCYEL( ch, C_NRM ), CCBLD( ch, C_CMP ), CCNRM( ch, C_SPR ) );
	send_to_char( buf2, ch );
    } else 
	send_to_char( "You have gained trivial experience.\r\n", ch );
  
    gain_exp( ch, exp );
    change_alignment( ch, victim );

}

void
gain_kill_exp( struct char_data *ch, struct char_data *victim )
{
  
    if ( ch == victim )
	return;

    if ( IS_NPC( victim ) && MOB2_FLAGGED( victim, MOB2_UNAPPROVED ) && !PLR_FLAGGED( ch, PLR_TESTER ) )
		return;
 	
	if ((IS_NPC(ch) && IS_PET(ch)) || IS_NPC(victim) && IS_PET(victim))
 		return; 

    if ( IS_AFFECTED( ch, AFF_GROUP ) ) {
	group_gain( ch, victim );
	return;
    }

    perform_gain_kill_exp( ch, victim, 1 );
}

void
blood_spray( struct char_data *ch, struct char_data *victim, 
	     int dam, int attacktype )
{
    char *to_char, *to_vict, *to_notvict;
    int pos, found = 0;
    struct char_data *nvict;
  
    switch ( number( 0, 6 ) ) {
    case 0:
	to_char = 
	    "Your terrible %s sends a spray of $N's blood into the air!";
	to_notvict =
	    "$n's terrible %s sends a spray of $N's blood into the air!";
	to_vict =
	    "$n's terrible %s sends a spray of your blood into the air!";
	break;
    case 1:
	to_char =
	    "A stream of blood erupts from $N as a result of your %s!";
	to_notvict =
	    "A stream of blood erupts from $N as a result of $n's %s!";
	to_vict =
	    "A stream of your blood erupts as a result of $n's %s!";
	break;
    case 2:
	to_char =
	    "Your %s opens a wound in $N which erupts in a torrent of blood!";
	to_notvict =
	    "$n's %s opens a wound in $N which erupts in a torrent of blood!";
	to_vict =
	    "$n's %s opens a wound in you which erupts in a torrent of blood!";
	break;
    case 3:
	to_char =
	    "Your mighty %s sends a spray of $N's blood raining down all over!";
	to_notvict =
	    "$n's mighty %s sends a spray of $N's blood raining down all over!";
	to_vict =
	    "$n's mighty %s sends a spray of your blood raining down all over!";
	break;
    case 4:
	to_char =
	    "Your devastating %s sends $N's blood spraying into the air!";
	to_notvict =
	    "$n's devastating %s sends $N's blood spraying into the air!";
	to_vict =
	    "$n's devastating %s sends your blood spraying into the air!";
	break;
    case 5:
	to_char =
	    "$N's blood sprays all over as a result of your terrible %s!";
	to_notvict =
	    "$N's blood sprays all over as a result of $n's terrible %s!";
	to_vict =
	    "Your blood sprays all over as a result of $n's terrible %s!";
	break;
    default:
	to_char =
	    "A gout of $N's blood spews forth in response to your %s!";
	to_notvict =
	    "A gout of $N's blood spews forth in response to $n's %s!";
	to_vict =
	    "A gout of your blood spews forth in response to $n's %s!";
	break;
    }	
    
    sprintf( buf, 
	     to_char,
	     attacktype >= TYPE_HIT ? 
	     attack_hit_text[attacktype - TYPE_HIT].singular :
	     spells[attacktype] );
    send_to_char( CCRED( ch, C_NRM ), ch );
    act( buf, FALSE, ch, 0, victim, TO_CHAR );
    send_to_char( CCNRM( ch, C_NRM ), ch );

    sprintf( buf,
	     to_vict,
	     attacktype >= TYPE_HIT ? 
	     attack_hit_text[attacktype - TYPE_HIT].singular :
	     spells[attacktype] );
    send_to_char( CCRED( victim, C_NRM ), victim );
    act( buf, FALSE, ch, 0, victim, TO_VICT );
    send_to_char( CCNRM( victim, C_NRM ), victim );

    for ( nvict = ch->in_room->people; nvict; nvict = nvict->next_in_room ) {
	if ( nvict == ch || nvict == victim || !nvict->desc || !AWAKE( nvict ) )
	    continue;
	send_to_char( CCRED( ch, C_NRM ), nvict );
    }

    sprintf( buf,
	     to_notvict,
	     attacktype >= TYPE_HIT ? 
	     attack_hit_text[attacktype - TYPE_HIT].singular :
	     spells[attacktype] );

    act( buf, FALSE, ch, 0, victim, TO_NOTVICT );

    for ( nvict = ch->in_room->people; nvict; nvict = nvict->next_in_room ) {
	if ( nvict == ch || nvict == victim || !nvict->desc || !AWAKE( nvict ) )
	    continue;
	send_to_char( CCNRM( ch, C_NRM ), nvict );
    }

    //
    // evil clerics get a bonus for sending blood flying!
    //
    
    if ( IS_CLERIC( ch ) && IS_EVIL( ch ) ) {
	GET_HIT( ch ) = MIN( GET_MAX_HIT( ch ), GET_HIT( ch ) + 20 );
	GET_MANA( ch ) = MIN( GET_MAX_MANA( ch ), GET_MANA( ch ) + 20 );
	GET_MOVE( ch ) = MIN( GET_MAX_MOVE( ch ), GET_MOVE( ch ) + 20 );
    }
	
    for ( nvict = ch->in_room->people; nvict; nvict = nvict->next_in_room ) {
	if ( nvict == victim || IS_NPC( nvict ) )
	    continue;

	if ( number( 5, 30 ) > GET_DEX( nvict ) && ( nvict != ch || !number( 0, 2 ) ) ) {
	    if ( ( pos = apply_soil_to_char( nvict,NULL,SOIL_BLOOD,WEAR_RANDOM ) ) ) {
		sprintf( buf, "$N's blood splatters all over your %s!",
			 GET_EQ( nvict, pos ) ? GET_EQ( nvict, pos )->short_description :
			 wear_description[pos] );
		act( buf, FALSE, nvict, 0, victim, TO_CHAR );
		found = 1;

		if ( nvict == ch && IS_CLERIC( ch ) && IS_EVIL( ch ) ) {
		    GET_HIT( ch ) = MIN( GET_MAX_HIT( ch ), GET_HIT( ch ) + 20 );
		    GET_MANA( ch ) = MIN( GET_MAX_MANA( ch ), GET_MANA( ch ) + 20 );
		    GET_MOVE( ch ) = MIN( GET_MAX_MOVE( ch ), GET_MOVE( ch ) + 20 );
		}
		
		break;
	    }
	}
    }
  
    if ( !found )
	add_blood_to_room( ch->in_room, 1 );

    if ( cur_weap && cur_weap->worn_by && 
	 cur_weap == GET_EQ( ch, cur_weap->worn_on ) &&
	 !OBJ_SOILED( cur_weap, SOIL_BLOOD ) ) {
	apply_soil_to_char( ch, NULL, SOIL_BLOOD, cur_weap->worn_on );
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


/* message for doing damage with a weapon */
void 
dam_message( int dam, struct char_data * ch, struct char_data * victim,
	     int w_type, int location )
{
    char *buf;
    int msgnum;

    struct obj_data *weap = cur_weap;

    static struct dam_weapon_type {
	char *to_room;
	char *to_char;
	char *to_victim;
    } dam_weapons[] = {

	/* use #w for singular ( i.e. "slash" ) and #W for plural ( i.e. "slashes" ) */

	{
	    "$n tries to #w $N, but misses.",	/* 0: 0     */
	    "You try to #w $N, but miss.",
	    "$n tries to #w you, but misses."
	},

	{
	    "$n tickles $N as $e #W $M.",	/* 1: 1..4  */
	    "You tickle $N as you #w $M.",
	    "$n tickles you as $e #W you."
	},

	{
	    "$n barely #W $N.",		/* 2: 3..6  */
	    "You barely #w $N.",
	    "$n barely #W you."
	},

	{
	    "$n #W $N.",			/* 3: 5..10 */
	    "You #w $N.",
	    "$n #W you."
	},

	{
	    "$n #W $N hard.",			/* 4: 7..14  */
	    "You #w $N hard.",
	    "$n #W you hard."
	},

	{
	    "$n #W $N very hard.",		/* 5: 11..19  */
	    "You #w $N very hard.",
	    "$n #W you very hard."
	},

	{
	    "$n #W $N extremely hard.",	/* 6: 15..23  */
	    "You #w $N extremely hard.",
	    "$n #W you extremely hard."
	},

	{
	    "$n massacres $N to small fragments with $s #w.",	/* 7: 19..27 */
	    "You massacre $N to small fragments with your #w.",
	    "$n massacres you to small fragments with $s #w."
	},

	{
	    "$n devastates $N with $s incredible #w!!",    /* 8: 23..32 */
	    "You devastate $N with your incredible #w!!",
	    "$n devastates you with $s incredible #w!!"
	},

	{
	    "$n OBLITERATES $N with $s deadly #w!!",	/* 9: 32..37   */
	    "You OBLITERATE $N with your deadly #w!!",
	    "$n OBLITERATES you with $s deadly #w!!"
	},

	{
	    "$n utterly DEMOLISHES $N with $s unbelievable #w!!", /* 10: 37..45 */
	    "You utterly DEMOLISH $N with your unbelievable #w!!",
	    "$n utterly DEMOLISHES you with $s unbelievable #w!!"
	},

	{
	    "$n PULVERIZES $N with $s viscious #w!!", /* 11: 46..69 */
	    "You PULVERIZE $N with your viscious #w!!",
	    "$n PULVERIZES you with $s viscious #w!!"
	},

	{
	    "$n *DECIMATES* $N with $s horrible #w!!", /* 12: 70..99 */
	    "You *DECIMATE* $N with your horrible #w!!",
	    "$n *DECIMATES* you with $s horrible #w!!"
	},

	{
	    "$n *LIQUIFIES* $N with $s incredibly viscious #w!!", /* 13: 100-139 */
	    "You **LIQUIFY** $N with your incredibly viscious #w!!",
	    "$n *LIQUIFIES* you with $s incredibly viscious #w!!"
	},

	{
	    "$n **VAPORIZES** $N with $s terrible #w!!", /* 14: 14-189 */
	    "You **VAPORIZE** $N with your terrible #w!!",
	    "$n **VAPORIZES** you with $s terrible #w!!"
	},

	{
	    "$n **ANNIHILATES** $N with $s ultra powerful #w!!", /* 15: >189 */
	    "You **ANNIHILATE** $N with your ultra powerful #w!!",
	    "$n **ANNIHILATES** you with $s ultra powerful #w!!"
	}

    };

    /* second set of possible mssgs, chosen ramdomly. */
    static struct dam_weapon_type dam_weapons_2[] = {

	{
	    "$n tries to #w $N with $p, but misses.",	/* 0: 0     */
	    "You try to #w $N with $p, but miss.",
	    "$n tries to #w you with $p, but misses."
	},

	{
	    "$n tickles $N as $e #W $M with $p.",	/* 1: 1..4  */
	    "You tickle $N as you #w $M with $p.",
	    "$n tickles you as $e #W you with $p."
	},

	{
	    "$n barely #W $N with $p.",		/* 2: 3..6  */
	    "You barely #w $N with $p.",
	    "$n barely #W you with $p."
	},

	{
	    "$n #W $N with $p.",			/* 3: 5..10 */
	    "You #w $N with $p.",
	    "$n #W you with $p."
	},

	{
	    "$n #W $N hard with $p.",			/* 4: 7..14  */
	    "You #w $N hard with $p.",
	    "$n #W you hard with $p."
	},

	{
	    "$n #W $N very hard with $p.",		/* 5: 11..19  */
	    "You #w $N very hard with $p.",
	    "$n #W you very hard with $p."
	},

	{
	    "$n #W $N extremely hard with $p.",	/* 6: 15..23  */
	    "You #w $N extremely hard with $p.",
	    "$n #W you extremely hard with $p."
	},

	{
	    "$n massacres $N to small fragments with $p.",	/* 7: 19..27 */
	    "You massacre $N to small fragments with $p.",
	    "$n massacres you to small fragments with $p."
	},

	{
	    "$n devastates $N with $s incredible #w!!",    /* 8: 23..32 */
	    "You devastate $N with a #w from $p!!",
	    "$n devastates you with $s incredible #w!!"
	},

	{
	    "$n OBLITERATES $N with $p!!",	/* 9: 32..37   */
	    "You OBLITERATE $N with $p!!",
	    "$n OBLITERATES you with $p!!"
	},

	{
	    "$n deals a DEMOLISHING #w to $N with $p!!", /* 10: 37..45 */
	    "You deal a DEMOLISHING #w to $N with $p!!",
	    "$n deals a DEMOLISHING #w to you with $p!!"
	},

	{
	    "$n PULVERIZES $N with $p!!", /* 11: 46..79 */
	    "You PULVERIZE $N with $p!!",
	    "$n PULVERIZES you with $p!!"
	},

	{
	    "$n *DECIMATES* $N with $p!!", /* 12: 70..99 */
	    "You *DECIMATE* $N with $p!!",
	    "$n *DECIMATES* you with $p!!"
	},

	{
	    "$n *LIQUIFIES* $N with a #w from $p!!", /* 13: 100-139 */
	    "You **LIQUIFY** $N with a #w from $p!!",
	    "$n *LIQUIFIES* you with a #w from $p"
	},

	{
	    "$n **VAPORIZES** $N with $p!!", /* 14: 14-189 */
	    "You **VAPORIZE** $N with a #w from $p!!",
	    "$n **VAPORIZES** you with a #w from $p!!"
	},

	{
	    "$n **ANNIHILATES** $N with $p!!", /* 15: >189 */
	    "You **ANNIHILATE** $N with your #w from $p!!",
	    "$n **ANNIHILATES** you with $s #w from $p!!"
	}

    };

    /* messages for body part specifics */
    static struct dam_weapon_type dam_weapons_location[] = {

	{
	    "$n tries to #w $N's #p, but misses.",	/* 0: 0     */
	    "You try to #w $N's #p, but miss.",
	    "$n tries to #w your #p, but misses."
	},

	{
	    "$n tickles $N's #p as $e #W $M.",	/* 1: 1..4  */
	    "You tickle $N's #p as you #w $M.",
	    "$n tickles you as $e #W your #p."
	},

	{
	    "$n barely #W $N's #p.",		/* 2: 3..6  */
	    "You barely #w $N's #p.",
	    "$n barely #W your #p."
	},

	{
	    "$n #W $N's #p.",			/* 3: 5..10 */
	    "You #w $N's #p.",
	    "$n #W your #p."
	},

	{
	    "$n #W $N's #p hard.",			/* 4: 7..14  */
	    "You #w $N's #p hard.",
	    "$n #W your #p hard."
	},

	{
	    "$n #W $N's #p very hard.",		/* 5: 11..19  */
	    "You #w $N's #p very hard.",
	    "$n #W your #p very hard."
	},

	{
	    "$n #W $N's #p extremely hard.",	/* 6: 15..23  */
	    "You #w $N's #p extremely hard.",
	    "$n #W your #p extremely hard."
	},

	{
	    "$n massacres $N's #p to fragments with $s #w.",	/* 7: 19..27 */
	    "You massacre $N's #p to small fragments with your #w.",
	    "$n massacres your #p to small fragments with $s #w."
	},

	{
	    "$n devastates $N's #p with $s incredible #w!!",    /* 8: 23..32 */
	    "You devastate $N's #p with your incredible #w!!",
	    "$n devastates your #p with $s incredible #w!!"
	},

	{
	    "$n OBLITERATES $N's #p with $s #w!!",	/* 9: 32..37   */
	    "You OBLITERATE $N's #p with your #w!!",
	    "$n OBLITERATES your #p with $s #w!!"
	},

	{
	    "$n deals a DEMOLISHING #w to $N's #p!!", /* 10: 37..45 */
	    "You deal a DEMOLISHING #w to $N's #p!!",
	    "$n deals a DEMOLISHING #w to your #p!!"
	},

	{
	    "$n PULVERIZES $N's #p with $s viscious #w!!", /* 11: 46..69 */
	    "You PULVERIZE $N's #p with your viscious #w!!",
	    "$n PULVERIZES your #p with $s viscious #w!!"
	},

	{
	    "$n *DECIMATES* $N's #p with $s horrible #w!!", /* 12: 70..99 */
	    "You *DECIMATE* $N's #p with your horrible #w!!",
	    "$n *DECIMATES* your #p with $s horrible #w!!"
	},

	{
	    "$n *LIQUIFIES* $N's #p with $s viscious #w!!", /* 13: 100-139 */
	    "You **LIQUIFY** $N's #p with your viscious #w!!",
	    "$n *LIQUIFIES* your #p with $s viscious #w!!"
	},

	{
	    "$n **VAPORIZES** $N's #p with $s terrible #w!!", /* 14: 14-189 */
	    "You **VAPORIZE** $N's #p with your terrible #w!!",
	    "$n **VAPORIZES** your #p with $s terrible #w!!"
	},

	{
	    "$n **ANNIHILATES** $N's #p with $s ultra #w!!", /* 15: >189 */
	    "You **ANNIHILATE** $N's #p with your ultra #w!!",
	    "$n **ANNIHILATES** your #p with $s ultra #w!!"
	}


    };

    /* fourth set of possible mssgs, IF RAYGUN. */
    static struct dam_weapon_type dam_guns[] = {

	{
	    "$n tries to #w $N with $p, but misses.",	/* 0: 0     */
	    "You try to #w $N with $p, but miss.",
	    "$n tries to #w you with $p, but misses."
	},

	{
	    "$n grazes $N with a #w from $p.",	/* 1: 1..4  */
	    "You graze $N as you #w at $M with $p.",
	    "$n grazes you as $e #W you with $p."
	},

	{
	    "$n barely #W $N with $p.",		/* 2: 3..6  */
	    "You barely #w $N with $p.",
	    "$n barely #W you with $p."
	},

	{
	    "$n #W $N with $p.",			/* 3: 5..10 */
	    "You #w $N with $p.",
	    "$n #W you with $p."
	},

	{
	    "$n #W $N hard with $p.",			/* 4: 7..14  */
	    "You #w $N hard with $p.",
	    "$n #W you hard with $p."
	},

	{
	    "$n #W $N very hard with $p.",		/* 5: 11..19  */
	    "You #w $N very hard with $p.",
	    "$n #W you very hard with $p."
	},

	{
	    "$n #W the hell out of $N with $p.",	/* 6: 15..23  */
	    "You #w the hell out of $N with $p.",
	    "$n #W the hell out of you with $p."
	},

	{
	    "$n #W $N to small fragments with $p.",	/* 7: 19..27 */
	    "You #w $N to small fragments with $p.",
	    "$n #W you to small fragments with $p."
	},

	{
	    "$n devastates $N with a #w from $p!!",    /* 8: 23..32 */
	    "You devastate $N with a #W from $p!!",
	    "$n devastates you with $s #w from $p!!"
	},

	{
	    "$n OBLITERATES $N with a #w from $p!!",	/* 9: 32..37   */
	    "You OBLITERATE $N with a #w from $p!!",
	    "$n OBLITERATES you with a #w from $p!!"
	},

	{
	    "$n DEMOLISHES $N with a dead on #w!!", /* 10: 37..45 */
	    "You DEMOLISH $N with a dead on blast from $p!!",
	    "$n DEMOLISHES you with a dead on #w from $p!!"
	},

	{
	    "$n PULVERIZES $N with a #w from $p!!", /* 11: 46..79 */
	    "You PULVERIZE $N with a #w from $p!!",
	    "$n PULVERIZES you with a #w from $p!!"
	},

	{
	    "$n *DECIMATES* $N with a #w from $p!!", /* 12: 70..99 */
	    "You *DECIMATE* $N with a #w from $p!!",
	    "$n *DECIMATES* you with a #w from $p!!"
	},

	{
	    "$n *LIQUIFIES* $N with a #w from $p!!", /* 13: 100-139 */
	    "You **LIQUIFY** $N with a #w from $p!!",
	    "$n *LIQUIFIES* you with a #w from $p"
	},

	{
	    "$n **VAPORIZES** $N with $p!!", /* 14: 14-189 */
	    "You **VAPORIZE** $N with a #w from $p!!",
	    "$n **VAPORIZES** you with a #w from $p!!"
	},

	{
	    "$n **ANNIHILATES** $N with $p!!", /* 15: >189 */
	    "You **ANNIHILATE** $N with your #w from $p!!",
	    "$n **ANNIHILATES** you with $s #w from $p!!"
	}

    };

    if ( search_nomessage )
	return;

    if ( w_type == SKILL_ENERGY_WEAPONS )
	w_type = TYPE_ENERGY_GUN;
  
    if ( w_type == SKILL_PROJ_WEAPONS )
	w_type = TYPE_BLAST;

    if ( w_type == SKILL_ARCHERY )
	return;

    if ( dam == 0 )		msgnum = 0;
    else if ( dam <= 4 )    msgnum = 1;
    else if ( dam <= 6 )    msgnum = 2;
    else if ( dam <= 10 )   msgnum = 3;
    else if ( dam <= 14 )   msgnum = 4;
    else if ( dam <= 19 )   msgnum = 5;
    else if ( dam <= 23 )   msgnum = 6;
    else if ( dam <= 27 )   msgnum = 7;   /* fragments  */
    else if ( dam <= 32 )   msgnum = 8;   /* devastate  */
    else if ( dam <= 37 )   msgnum = 9;   /* obliterate */
    else if ( dam <= 45 )   msgnum = 10;  /* demolish   */
    else if ( dam <= 69 )   msgnum = 11;  /* pulverize  */
    else if ( dam <= 94 )   msgnum = 12;  /* decimate   */
    else if ( dam <= 129 )  msgnum = 13;  /* liquify    */
    else if ( dam <= 169 )  msgnum = 14;  /* vaporize   */
    else			msgnum = 15;  /* annihilate */

    w_type -= TYPE_HIT;

    /* damage message to onlookers */
    if ( weap && ( ( IS_ENERGY_GUN( weap ) && w_type == ( TYPE_ENERGY_GUN-TYPE_HIT ) ) ||
		   ( IS_GUN( weap ) && w_type == ( TYPE_BLAST-TYPE_HIT ) ) ) )
	buf = replace_string( dam_guns[msgnum].to_room,
			      attack_hit_text[w_type].singular,
			      attack_hit_text[w_type].plural, NULL );
    else if ( location >= 0 && !number( 0, 2 ) && ( !weap || weap->worn_on == WEAR_WIELD ) )
	buf = replace_string( dam_weapons_location[msgnum].to_room,
			      attack_hit_text[w_type].singular, 
			      attack_hit_text[w_type].plural,
			      wear_keywords[wear_translator[location]] );
    else if ( weap && ( number( 0, 2 ) || weap->worn_on != WEAR_WIELD ) )
	buf = replace_string( dam_weapons_2[msgnum].to_room,
			      attack_hit_text[w_type].singular,
			      attack_hit_text[w_type].plural, NULL );
    else
	buf = replace_string( dam_weapons[msgnum].to_room,
			      attack_hit_text[w_type].singular,
			      attack_hit_text[w_type].plural, NULL );
    act( buf, FALSE, ch, weap, victim, TO_NOTVICT );
    /* damage message to damager */
    if ( ( msgnum || !PRF_FLAGGED( ch, PRF_GAGMISS ) ) && ch->desc ) {
	if ( weap && ( ( IS_ENERGY_GUN( weap ) && w_type==( TYPE_ENERGY_GUN-TYPE_HIT ) ) ||
		       ( IS_GUN( weap ) && w_type == ( TYPE_BLAST-TYPE_HIT ) ) ) )
	    buf = replace_string( dam_guns[msgnum].to_char,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	else if ( location >= 0 && !number( 0, 2 ) ) {
	    buf = replace_string( dam_weapons_location[msgnum].to_char,
				  attack_hit_text[w_type].singular, 
				  attack_hit_text[w_type].plural,
				  wear_keywords[wear_translator[location]] );
	} else if ( weap && ( number( 0, 4 ) || weap->worn_on != WEAR_WIELD ) )
	    buf = replace_string( dam_weapons_2[msgnum].to_char,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	else
	    buf = replace_string( dam_weapons[msgnum].to_char,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	send_to_char( CCYEL( ch, C_NRM ), ch );
	act( buf, FALSE, ch, weap, victim, TO_CHAR | TO_SLEEP );
	send_to_char( CCNRM( ch, C_NRM ), ch );
    }
    /* damage message to damagee */
    if ( ( msgnum || !PRF_FLAGGED( victim, PRF_GAGMISS ) ) && victim->desc ) {
	if ( weap && ( ( IS_ENERGY_GUN( weap ) && w_type==( TYPE_ENERGY_GUN-TYPE_HIT ) ) ||
		       ( IS_GUN( weap ) && w_type == ( TYPE_BLAST-TYPE_HIT ) ) ) )
	    buf = replace_string( dam_guns[msgnum].to_victim,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	else if ( location >= 0 && !number( 0, 2 ) ) {
	    buf = replace_string( dam_weapons_location[msgnum].to_victim, 
				  attack_hit_text[w_type].singular, 
				  attack_hit_text[w_type].plural,
				  wear_keywords[wear_translator[location]] );
	} else if ( weap && ( number( 0, 3 ) || weap->worn_on != WEAR_WIELD ) )
	    buf = replace_string( dam_weapons_2[msgnum].to_victim,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	else
	    buf = replace_string( dam_weapons[msgnum].to_victim,
				  attack_hit_text[w_type].singular,
				  attack_hit_text[w_type].plural, NULL );
	send_to_char( CCRED( victim, C_NRM ), victim );
	act( buf, FALSE, ch, weap, victim, TO_VICT | TO_SLEEP );
	send_to_char( CCNRM( victim, C_NRM ), victim );
    }
  
    if ( CHAR_HAS_BLOOD( victim ) && BLOODLET( victim,  dam, w_type + TYPE_HIT ) )
	blood_spray( ch, victim, dam, w_type + TYPE_HIT );
}  


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int 
skill_message( int dam, struct char_data * ch, struct char_data * vict,
	       int attacktype )
{
    int i, j, nr;
    struct message_type *msg;
    struct obj_data *weap = cur_weap;

    if ( search_nomessage )
	return 1;

    for ( i = 0; i < MAX_MESSAGES; i++ ) {
	if ( fight_messages[i].a_type == attacktype ) {
	    nr = dice( 1, fight_messages[i].number_of_attacks );
	    for ( j = 1, msg = fight_messages[i].msg; ( j < nr ) && msg; j++ )
		msg = msg->next;
      
	    /*      if ( attacktype == TYPE_SLASH ) {
		    sprintf( buf, "slash [%d]\r\n", nr );
		    send_to_char( buf, ch );
		    }*/
	    if ( attacktype == TYPE_SLASH && nr == 1 && !isname( "headless", vict->player.name ) )
		corpse_state = SKILL_BEHEAD;
	    else
		corpse_state = 0;

	    if ( !IS_NPC( vict ) && GET_LEVEL( vict ) >= LVL_AMBASSADOR && 
		 ( !PLR_FLAGGED( vict, PLR_MORTALIZED ) || dam == 0 ) ) {
		act( msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR );
		act( msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT );
		act( msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT );
	    } else if ( dam != 0 ) {
		if ( GET_POS( vict ) == POS_DEAD ) {
		    if ( ch ) {
			send_to_char( CCYEL( ch, C_NRM ), ch );
			act( msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR );
			send_to_char( CCNRM( ch, C_NRM ), ch );

			act( msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT );
		    }

		    send_to_char( CCRED( vict, C_NRM ), vict );
		    act( msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP );
		    send_to_char( CCNRM( vict, C_NRM ), vict );

		} else {
		    if ( ch ) {
			send_to_char( CCYEL( ch, C_NRM ), ch );
			act( msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR );
			send_to_char( CCNRM( ch, C_NRM ), ch );

			act( msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT );
		    }

		    send_to_char( CCRED( vict, C_NRM ), vict );
		    act( msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP );
		    send_to_char( CCNRM( vict, C_NRM ), vict );

		}
	    } else if ( ch != vict ) {	/* Dam == 0 */
		if ( ch && !PRF_FLAGGED( ch, PRF_GAGMISS ) ) {
		    send_to_char( CCYEL( ch, C_NRM ), ch );
		    act( msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR );
		    send_to_char( CCNRM( ch, C_NRM ), ch );

		    act( msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT );
		}

		if ( !PRF_FLAGGED( vict, PRF_GAGMISS ) ) {
		    send_to_char( CCRED( vict, C_NRM ), vict );
		    act( msg->miss_msg.victim_msg, FALSE, ch, weap, vict, 
			 TO_VICT | TO_SLEEP );
		    send_to_char( CCNRM( vict, C_NRM ), vict );
		}

	    }
	    if ( BLOODLET( vict, dam, attacktype ) )
		blood_spray( ch, vict, dam, attacktype );

	    return 1;
	}
    }
    return 0;
}

void
eqdam_extract_obj( struct obj_data *obj )
{

    struct obj_data *inobj = NULL, *next_obj = NULL;

    for ( inobj = obj->contains; inobj; inobj = next_obj ) {
	next_obj = inobj->next_content;
    
	obj_from_obj( inobj );

	if ( IS_IMPLANT( inobj ) )
	    SET_BIT( GET_OBJ_WEAR( inobj ), ITEM_WEAR_TAKE );
    
	if ( obj->in_room )
	    obj_to_room( inobj, obj->in_room );
	else if ( obj->worn_by )
	    obj_to_char( inobj, obj->worn_by );
	else if ( obj->carried_by )
	    obj_to_char( inobj, obj->carried_by );
	else if ( obj->in_obj )
	    obj_to_obj( inobj, obj->in_obj );
	else {
	    slog( "SYSERR: wierd bogus shit." );
	    extract_obj( inobj );
	}
    }
    extract_obj( obj );
}


struct obj_data * 
damage_eq( struct char_data *ch, struct obj_data *obj, int eq_dam )
{
    struct char_data *vict = NULL;
    struct obj_data *new_obj = NULL, *inobj = NULL, *next_obj = NULL;
    struct room_data *room = NULL;
    int tmp;

    if ( GET_OBJ_DAM( obj ) < 0 || GET_OBJ_MAX_DAM( obj ) < 0 ||
	 ( ch && GET_LEVEL( ch ) < LVL_IMMORT && !CAN_WEAR( obj, ITEM_WEAR_TAKE ) ) ||
     ( ch && ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_ARENA) )
        )
	return NULL;

    /** damage has destroyed object */
    if ( ( GET_OBJ_DAM( obj ) - eq_dam ) < ( GET_OBJ_MAX_DAM( obj ) >> 5 ) ) {
	new_obj = create_obj(  );
	new_obj->shared = null_obj_shared;
	GET_OBJ_MATERIAL( new_obj ) = GET_OBJ_MATERIAL( obj );
	new_obj->setWeight( obj->getWeight() );
	GET_OBJ_TYPE( new_obj ) = ITEM_TRASH;
	GET_OBJ_WEAR( new_obj ) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA( new_obj ) = ITEM_NODONATE + ITEM_NOSELL;
	GET_OBJ_VAL( new_obj, 0 ) = obj->shared->vnum;
	GET_OBJ_MAX_DAM( new_obj ) = 100;
	GET_OBJ_DAM( new_obj ) = 100;

	/* damage interior items */
	for ( inobj = obj->contains; inobj; inobj = next_obj ) {
	    next_obj = inobj->next_content;

	    damage_eq( NULL, inobj, ( eq_dam >> 1 ) );
	}
#ifdef DMALLOC
	dmalloc_verify( 0 );
#endif      
	if ( IS_METAL_TYPE( obj ) ) {
	    strcpy( buf2, "$p is reduced to a mangled pile of scrap!!" );

	    sprintf( buf, "%s heap mangled %s", 
		     material_names[GET_OBJ_MATERIAL( obj )], obj->name );
	    new_obj->name = str_dup( buf );
	    sprintf( buf, "a mangled heap of %s", 
		     material_names[GET_OBJ_MATERIAL( obj )] );
	    new_obj->short_description = str_dup( buf );
	    strcat( CAP( buf ), " is lying here." );
	    new_obj->description = str_dup( buf );
      
	} else if ( IS_STONE_TYPE( obj ) || IS_GLASS_TYPE( obj ) ) {
	    strcpy( buf2, "$p shatters into a thousand fragments!!" );

	    sprintf( buf, "%s shattered fragments %s", 
		     material_names[GET_OBJ_MATERIAL( obj )], obj->name );
	    new_obj->name = str_dup( buf );
	    sprintf( buf, "shattered fragments of %s", 
		     material_names[GET_OBJ_MATERIAL( obj )] );
	    new_obj->short_description = str_dup( buf );
	    strcat( CAP( buf ), " are lying here." );
	    new_obj->description = str_dup( buf );

	} else {
	    strcpy( buf2, "$p has been destroyed!" );
      
	    sprintf( buf, "%s mutilated heap %s%s", 
		     material_names[GET_OBJ_MATERIAL( obj )], 
		     GET_OBJ_MATERIAL( obj ) ? "" : " material ", obj->name );
	    new_obj->name = str_dup( buf );
	    sprintf( buf, "a mutilated heap of %s%s", 
		     material_names[GET_OBJ_MATERIAL( obj )],
		     !GET_OBJ_MATERIAL( obj ) ? " material" : "" );
	    new_obj->short_description = str_dup( buf );
	    strcat( CAP( buf ), " is lying here." );
	    new_obj->description = str_dup( buf );
	    if ( IS_CORPSE( obj ) ) {
		GET_OBJ_TYPE( new_obj )   = ITEM_CONTAINER;
		GET_OBJ_VAL( new_obj, 0 ) = GET_OBJ_VAL( obj, 0 );
		GET_OBJ_VAL( new_obj, 1 ) = GET_OBJ_VAL( obj, 1 );
		GET_OBJ_VAL( new_obj, 2 ) = GET_OBJ_VAL( obj, 2 );
		GET_OBJ_VAL( new_obj, 3 ) = GET_OBJ_VAL( obj, 3 );
		GET_OBJ_TIMER( new_obj )  = GET_OBJ_TIMER( obj );
	    }
	}
#ifdef DMALLOC
	dmalloc_verify( 0 );
#endif
	if ( IS_OBJ_STAT2( obj, ITEM2_IMPLANT ) )
	    SET_BIT( GET_OBJ_EXTRA2( new_obj ), ITEM2_IMPLANT );

	if ( ( room = obj->in_room ) && ( vict = obj->in_room->people ) ) {
	    act( buf2, FALSE, vict, obj, 0, TO_CHAR );
	    act( buf2, FALSE, vict, obj, 0, TO_ROOM );
	} else if ( ( vict = obj->worn_by ) )
	    act( buf2, FALSE, vict, obj, 0, TO_CHAR );
	else
	    act( buf2, FALSE, ch, obj, 0, TO_CHAR );

	if ( !obj->shared->proto ) {
	    eqdam_extract_obj( obj );
	    extract_obj( new_obj );
	    return NULL;
	}
    
	if ( ( vict = obj->worn_by ) || ( vict = obj->carried_by ) ) {
	    if ( obj->worn_by && ( obj != GET_EQ( obj->worn_by, obj->worn_on ) ) ) {
		tmp = obj->worn_on;
		eqdam_extract_obj( obj );
		if ( equip_char( vict, new_obj, tmp, MODE_IMPLANT ) )
		    return ( new_obj );
	    } else {
		eqdam_extract_obj( obj );
		obj_to_char( new_obj, vict );
	    }
	} else if ( room ) {
	    eqdam_extract_obj( obj );
	    obj_to_room( new_obj, room );
	} else if ( ( inobj = obj->in_obj ) ) {
	    eqdam_extract_obj( obj );
	    obj_to_obj( new_obj, inobj );
	} else {
	    slog( "SYSERR: Improper location of obj and new_obj in damage_eq." );
	    eqdam_extract_obj( obj );
	}
	return ( new_obj );
    } 

  
    /* object has reached broken state */
    if ( ( GET_OBJ_DAM( obj ) - eq_dam ) < ( GET_OBJ_MAX_DAM( obj ) >> 3 ) ) {

	SET_BIT( GET_OBJ_EXTRA2( obj ), ITEM2_BROKEN );
	strcpy( buf2, "$p has been severely damaged!!" );

	/* object looking rough ( 25% ) */
    } else if ( ( GET_OBJ_DAM( obj ) > ( GET_OBJ_MAX_DAM( obj ) >> 2 ) ) &&
		( ( GET_OBJ_DAM( obj ) - eq_dam ) < ( GET_OBJ_MAX_DAM( obj ) >> 2 ) ) ) {
  
	sprintf( buf2, "$p is starting to look pretty %s.",
		 IS_METAL_TYPE( obj ) ? "mangled" :
		 ( IS_LEATHER_TYPE( obj ) || IS_CLOTH_TYPE( obj ) ) ? "ripped up" :
		 "bad" );

	/* object starting to wear ( 50% ) */
    } else if ( ( GET_OBJ_DAM( obj ) > ( GET_OBJ_MAX_DAM( obj ) >> 1 ) ) &&
		( ( GET_OBJ_DAM( obj ) - eq_dam ) < ( GET_OBJ_MAX_DAM( obj ) >> 1 ) ) ) {

	strcpy( buf2, "$p is starting to show signs of wear." );

	/* just tally the damage and end */
    } else {
	GET_OBJ_DAM( obj ) -= eq_dam;
	return NULL;
    }

    /* send out messages and unequip if needed */
    if ( obj->in_room && ( vict = obj->in_room->people ) ) {
	act( buf2, FALSE, vict, obj, 0, TO_CHAR );
	act( buf2, FALSE, vict, obj, 0, TO_ROOM );
    } else if ( ( vict = obj->worn_by ) ) {
	act( buf2, FALSE, vict, obj, 0, TO_CHAR );
	if ( IS_OBJ_STAT2( obj, ITEM2_BROKEN ) ) {
	    if ( obj == GET_EQ( vict, obj->worn_on ) )
		obj_to_char( unequip_char( vict, obj->worn_on, MODE_EQ ), vict );
	    else {
		if ( IS_DEVICE( obj ) )
		    ENGINE_STATE( obj ) = 0;
	    }
	}
    } else
	act( buf2, FALSE, ch, obj, 0, TO_CHAR );
  
    GET_OBJ_DAM( obj ) -= eq_dam;
    return NULL;
}

//
// damage( ) returns TRUE on a kill, FALSE otherwise
// damage(  ) MUST return with DAM_RETURN(  ) macro !!!
//

int 
damage( struct char_data * ch, struct char_data * victim, int dam,
	int attacktype, int location )
{
    int hard_damcap, is_humil = 0, eq_dam = 0, weap_dam = 0, i,
	impl_dam = 0, mana_loss;
    struct char_data *tmp_ch = NULL, *next_ch = NULL;
    struct obj_data *obj = NULL, *weap = cur_weap, *impl = NULL;
    struct room_affect_data rm_aff;
    struct affected_type *af = NULL;


    if ( GET_POS( victim ) <= POS_DEAD ) {
	sprintf( buf,"SYSERR: Attempt to damage a corpse--ch=%s,vict=%s,type=%d.",
		 ch ? GET_NAME( ch ) : "NULL", GET_NAME( victim ), attacktype );
	slog( buf );
	DAM_RETURN( FALSE );
    }
    if ( ch && ( PLR_FLAGGED( victim, PLR_MAILING )  || 
		 PLR_FLAGGED( victim, PLR_WRITING )  || 
		 PLR_FLAGGED( victim, PLR_OLC ) ) &&
	 ch != victim ) {
	sprintf( buf, "%s has attacked %s while writing at %d.", GET_NAME( ch ),
		 GET_NAME( victim ), ch->in_room->number );
	mudlog( buf, BRF, GET_INVIS_LEV( ch ), TRUE );
	stop_fighting( ch );
	stop_fighting( victim );
	send_to_char( "NO!  Do you want to be ANNIHILATED by the gods?!\r\n", ch );
	DAM_RETURN( FALSE );
    }

    if ( ch ) {
		if ( affected_by_spell( ch, SPELL_QUAD_DAMAGE ) )
			dam <<= 2;
		else if ( AFF3_FLAGGED( ch, AFF3_DOUBLE_DAMAGE ) )
			dam <<= 1;
		
		if ( ( af = affected_by_spell( ch, SPELL_SANCTIFICATION ) ) ) {
			if ( IS_EVIL( victim ) && !IS_SOULLESS(victim) )
				dam += ( dam * GET_REMORT_GEN( ch ) ) / 20;
			else if (ch != victim &&  IS_GOOD( victim ) ) {
				send_to_char( "You have been de-sanctified!\r\n", ch );
				affect_remove( ch, af );
			}
		}
    }

    /* NV */
    if ( ch && ch != victim && !PLR_FLAGGED( victim, PLR_KILLER | PLR_THIEF ) &&
	 GET_LEVEL( ch ) < LVL_VIOLENCE &&
	 ( ROOM_FLAGGED( victim->in_room, ROOM_PEACEFUL ) ||
	   ROOM_FLAGGED( ch->in_room, ROOM_PEACEFUL ) ) )
	dam = 0;

    /* You can't damage an immortal! */
    if ( !IS_NPC( victim ) && ( GET_LEVEL( victim ) >= LVL_AMBASSADOR ) && 
	 ( !ch || !PLR_FLAGGED( victim, PLR_MORTALIZED ) ) )
	dam = 0;

    /* need magic weapons to damage undead */
    if ( IS_WEAPON( attacktype ) && weap && IS_OBJ_TYPE( weap, ITEM_WEAPON ) &&
	 NON_CORPOREAL_UNDEAD( victim ) && !IS_OBJ_STAT( weap, ITEM_MAGIC ) )
	dam = 0;

    /* shopkeeper protection */
    if ( !ok_damage_shopkeeper( ch, victim ) ) {
	DAM_RETURN( FALSE );
    }

    /* newbie protection and PLR_NOPK check*/
    if ( ch && ch != victim && !IS_NPC( ch ) && !IS_NPC( victim ) ) {
	if ( PLR_FLAGGED( ch, PLR_NOPK ) ) {
	    send_to_char( "A small dark shape flies in from the future and sticks to your eyebrow.\r\n", ch );
	    DAM_RETURN( FALSE );
	}
	if ( PLR_FLAGGED( victim, PLR_NOPK ) ) {
	    send_to_char( "A small dark shape flies in from the future and sticks to your nose.\r\n", ch );
	    DAM_RETURN( FALSE );
	}
    
	if ( GET_LEVEL( victim ) <= LVL_PROTECTED && 
	     !PLR_FLAGGED( victim, PLR_TOUGHGUY ) &&
	     GET_LEVEL( ch ) < LVL_IMMORT ) {
	    act( "$N is currently under new character protection.",
		 FALSE, ch, 0, victim, TO_CHAR );
	    act( "You are protected by the gods against $n's attack!",
		 FALSE, ch, 0, victim, TO_VICT );	
	    sprintf( buf, "%s protected against %s ( damage ) at %d\n",
		     GET_NAME( victim ), GET_NAME( ch ), victim->in_room->number );
	    slog( buf );
  
	    if ( victim == FIGHTING( ch ) )
		stop_fighting( ch );
	    if ( ch == FIGHTING( victim ) )
		stop_fighting( victim );
	    DAM_RETURN( FALSE );
	}

	if ( GET_LEVEL( ch ) <= LVL_PROTECTED && !PLR_FLAGGED( ch, PLR_TOUGHGUY ) ) {
	    send_to_char( "You are currently under new player protection, which expires at level 6.\r\nYou cannot attack other players while under this protection.\r\n", ch );
	    DAM_RETURN( FALSE );
	}
    }

    if ( GET_POS( victim ) < POS_FIGHTING )
	dam += ( dam * ( POS_FIGHTING - GET_POS( victim ) ) ) / 3;

    if ( ch ) {
	if ( MOB2_FLAGGED( ch, MOB2_UNAPPROVED ) && !PLR_FLAGGED( victim, PLR_TESTER ) )
	    dam = 0;

	if ( PLR_FLAGGED( ch, PLR_TESTER ) && !IS_MOB( victim ) && 
	     !PLR_FLAGGED( victim, PLR_TESTER ) )
	    dam = 0;
  
	if ( IS_MOB( victim ) && GET_LEVEL( ch ) >= LVL_AMBASSADOR && 
	     GET_LEVEL( ch ) < LVL_TIMEGOD && !mini_mud )
	    dam = 0;


	if ( victim != ch && IS_NPC( ch ) && IS_NPC( victim ) && victim->master &&
	     GET_MOB_WAIT( ch ) < 10 &&
	     !number( 0, 10 ) && IS_AFFECTED( victim, AFF_CHARM ) &&
	     ( victim->master->in_room == ch->in_room ) ) {
	    if ( FIGHTING( ch ) )
		stop_fighting( ch );
	    hit( ch, victim->master, TYPE_UNDEFINED );
	    DAM_RETURN( FALSE );
	}

	if ( victim->master == ch )
	    stop_follower( victim );

	appear( ch, victim );
    }

    if ( attacktype < MAX_SPELLS ) {
	if ( SPELL_IS_PSIONIC( attacktype ) &&
	     affected_by_spell( victim, SPELL_PSYCHIC_RESISTANCE ) )
	    dam >>= 1;
	if ( ( SPELL_IS_MAGIC( attacktype ) || SPELL_IS_DIVINE( attacktype ) ) &&
	     affected_by_spell( victim, SPELL_MAGICAL_PROT ) )
	    dam -= dam >> 2;
    }

    if ( attacktype == SPELL_PSYCHIC_CRUSH )
	location = WEAR_HEAD;

    if ( attacktype == TYPE_ACID_BURN && location == -1 ) {
		for ( i = 0; i < NUM_WEARS; i++ ) {
			if ( GET_EQ( ch, i ) )
			damage_eq( ch, GET_EQ( ch, i ), ( dam >> 1 ) );
			if ( GET_IMPLANT( ch, i ) )
			damage_eq( ch, GET_IMPLANT( ch, i ), ( dam >> 1 ) );
		}
    }

    /** check for armor **/
    if ( location != -1 ) {

	if ( location == WEAR_RANDOM ) {
	    location = choose_random_limb( victim );
	}

	if ( ( location == WEAR_FINGER_R ||
	       location == WEAR_FINGER_L ) && GET_EQ( victim, WEAR_HANDS ) ) {
	    location = WEAR_HANDS;
	}
    
	if ( ( location == WEAR_EAR_R ||
	       location == WEAR_EAR_L ) && GET_EQ( victim, WEAR_HEAD ) ) {
	    location = WEAR_HEAD;
	}
    
	obj = GET_EQ( victim, location );
	impl = GET_IMPLANT( victim, location );
    
	// implants are protected by armor
	if ( obj && impl && IS_OBJ_TYPE( obj, ITEM_ARMOR ) &&
	     !IS_OBJ_TYPE( impl, ITEM_ARMOR ) )
	    impl = NULL;
    
	if ( obj || impl ) {
      
	    if ( obj && OBJ_TYPE( obj, ITEM_ARMOR ) ) {
		eq_dam = ( GET_OBJ_VAL( obj, 0 ) * dam ) / 100;
		if ( location == WEAR_SHIELD )
		    eq_dam <<= 1;
	    }
	    if ( impl && OBJ_TYPE( impl, ITEM_ARMOR ) )
		impl_dam = ( GET_OBJ_VAL( impl, 0 ) * dam ) / 100;
      
	    weap_dam = eq_dam + impl_dam;
      
	    if ( obj && !eq_dam )
		eq_dam = ( dam >> 4 );
      
	    if ( ( !obj || !OBJ_TYPE( obj, ITEM_ARMOR ) ) && 
		 impl && !impl_dam )
		impl_dam = ( dam >> 5 );
      
	    /* here are the damage absorbing characteristics */
	    if ( ( attacktype == TYPE_BLUDGEON || attacktype == TYPE_HIT ||
		   attacktype == TYPE_POUND || attacktype == TYPE_PUNCH ) ) {
		if ( obj ) {
		    if ( IS_LEATHER_TYPE( obj ) || IS_CLOTH_TYPE( obj ) ||
			 IS_FLESH_TYPE( obj ) )
			eq_dam = ( int ) ( eq_dam * 0.7 );
		    else if ( IS_METAL_TYPE( obj ) )
			eq_dam = ( int ) ( eq_dam * 1.3 );
		}
		if ( impl ) {
		    if ( IS_LEATHER_TYPE( impl ) || IS_CLOTH_TYPE( impl ) ||
			 IS_FLESH_TYPE( impl ) )
			eq_dam = ( int ) ( eq_dam * 0.7 );
		    else if ( IS_METAL_TYPE( impl ) )
			eq_dam = ( int ) ( eq_dam * 1.3 );
		}
	    }
	    else if ( ( attacktype == TYPE_SLASH || attacktype == TYPE_PIERCE ||
			attacktype == TYPE_CLAW  || attacktype == TYPE_STAB ||
			attacktype == TYPE_RIP   || attacktype == TYPE_CHOP ) ) {
		if ( obj && ( IS_METAL_TYPE( obj ) || IS_STONE_TYPE( obj ) ) ) {
		    eq_dam = ( int ) ( eq_dam * 0.7 );
		    weap_dam = ( int ) ( weap_dam * 1.3 );
		}
		if ( impl && ( IS_METAL_TYPE( impl ) || IS_STONE_TYPE( impl ) ) ) {
		    impl_dam = ( int ) ( impl_dam * 0.7 );
		    weap_dam = ( int ) ( weap_dam * 1.3 );
		}
	    }
	    else if ( attacktype == SKILL_PROJ_WEAPONS ) {
		if ( obj ) {
		    if ( IS_MAT( obj, MAT_KEVLAR ) )
			eq_dam = ( int ) ( eq_dam * 1.6 );
		    else if ( IS_METAL_TYPE( obj ) )
			eq_dam = ( int ) ( eq_dam * 1.3 );
		}
	    }
      
	    dam = MAX( 0, ( dam - eq_dam - impl_dam ) );
      
	    if ( obj ) {
		if ( IS_OBJ_STAT( obj, ITEM_MAGIC_NODISPEL ) )
		    eq_dam >>= 1;
		if ( IS_OBJ_STAT( obj, ITEM_MAGIC | ITEM_BLESS | ITEM_EVIL_BLESS ) )
		    eq_dam = ( int ) ( eq_dam * 0.8 );
		if ( IS_OBJ_STAT2( obj, ITEM2_GODEQ | ITEM2_CURSED_PERM ) )
		    eq_dam = ( int ) ( eq_dam * 0.7 );
	    }
	    if ( impl ) {
		if ( IS_OBJ_STAT( impl, ITEM_MAGIC_NODISPEL ) )
		    impl_dam >>= 1;
		if ( IS_OBJ_STAT( impl, ITEM_MAGIC | ITEM_BLESS | ITEM_EVIL_BLESS ) )
		    impl_dam = ( int ) ( impl_dam * 0.8 );
		if ( IS_OBJ_STAT2( impl, ITEM2_GODEQ | ITEM2_CURSED_PERM ) )
		    impl_dam = ( int ) ( impl_dam * 0.7 );
	    }
      
	    //    struct obj_data *the_obj = impl ? impl : obj;

	    /* here are the object oriented damage specifics */
	    if ( ( attacktype == TYPE_SLASH || attacktype == TYPE_PIERCE ||
		   attacktype == TYPE_CLAW  || attacktype == TYPE_STAB ||
		   attacktype == TYPE_RIP   || attacktype == TYPE_CHOP ) ) {
		if ( obj && ( IS_METAL_TYPE( obj ) || IS_STONE_TYPE( obj ) ) ) {
		    eq_dam = ( int ) ( eq_dam * 0.7 );
		    weap_dam = ( int ) ( weap_dam * 1.3 );
		}
		if ( impl && ( IS_METAL_TYPE( impl ) || IS_STONE_TYPE( impl ) ) ) {
		    impl_dam = ( int ) ( impl_dam * 0.7 );
		    weap_dam = ( int ) ( weap_dam * 1.3 );
		}
	    }
	    else if ( attacktype == SPELL_PSYCHIC_CRUSH )
		eq_dam <<= 7;
	    else if ( attacktype == SPELL_OXIDIZE ) {
		if ( ( obj && IS_METAL_TYPE( obj ) ) ||
		     ( impl && IS_METAL_TYPE( impl ) ) ) {
		    eq_dam <<= 5;
		}
	    }
	    if ( weap ) {
		if ( IS_OBJ_STAT( weap, ITEM_MAGIC_NODISPEL ) )
		    weap_dam >>= 1;
		if ( IS_OBJ_STAT( weap, ITEM_MAGIC | ITEM_BLESS | ITEM_EVIL_BLESS ) )
		    weap_dam = ( int ) ( weap_dam * 0.8 );
		if ( IS_OBJ_STAT2( weap, ITEM2_CAST_WEAPON | ITEM2_GODEQ | 
				   ITEM2_CURSED_PERM ) )
		    weap_dam = ( int ) ( weap_dam * 0.7 );
	    }
	}
    }

    if ( GET_CLASS( victim ) == CLASS_CLERIC && IS_GOOD( victim ) ) {

	// full moon gives protection up to 30%
	
	if ( lunar_stage == MOON_FULL ) {
	    dam -= ( dam * GET_ALIGNMENT( victim ) ) / 3000;
	}  
	
	// good clerics get an alignment-based protection, up to 10%

	else {
	    dam -= ( dam * GET_ALIGNMENT( victim ) ) / 10000;
	}
    }
  
    if ( ch && GET_CLASS( ch ) == CLASS_CLERIC && IS_EVIL( ch ) ) {

	// new moon gives extra damage up to 30%

	if ( lunar_stage == MOON_NEW ) {
	    dam += ( dam * GET_ALIGNMENT( ch ) ) / 10000;
	}

	// evil clerics get an alignment-based damage bonus, up to 10%
	
	else {
	    dam += ( dam * GET_ALIGNMENT( ch ) ) / 10000;
	}
    }

    /*************** Sanctuary ******************/
    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) &&
	 ( !ch || !IS_EVIL( victim ) || 
	   !affected_by_spell( ch, SPELL_RIGHTEOUS_PENETRATION ) ) &&
	 ( !ch || !IS_GOOD( victim ) || 
	   !affected_by_spell( ch, SPELL_MALEFIC_VIOLATION ) ) && 
	 ( !(  attacktype == TYPE_BLEED ) && ! ( attacktype == SPELL_POISON ) ) ) {
	if ( IS_VAMPIRE( victim ) || IS_CYBORG( victim ) || IS_PHYSIC( victim ) )
	    dam = ( int ) ( dam * 0.80 );
	else if ( IS_CLERIC( victim ) || IS_KNIGHT( victim ) ) {
	    if ( IS_NEUTRAL( victim ) )    /***** weaker affects while neutral *****/
		dam = ( int ) ( dam * 0.60 );
	    else     
		dam >>= 1;
	} else
	    dam = ( int ) ( dam * 0.60 );
    }
    else if ( IS_AFFECTED( victim, AFF_NOPAIN ) )
	dam >>= 1;          /* 1/2 damage when NoPain */
    else if ( IS_AFFECTED_2( victim, AFF2_BESERK ) ) {
	if ( IS_BARB( victim ) ) 
	    dam -= ( dam * ( 10 + GET_REMORT_GEN( victim ) ) ) / 50;
	else
	    dam = ( int ) ( dam * 0.80 );
    }
    else if ( IS_AFFECTED_2( victim, AFF2_OBLIVITY ) &&
	      CHECK_SKILL( victim, ZEN_OBLIVITY ) > 60 ) {
    
	// damage reduction ranges from about 35 to 50%
	dam -= ( dam * 
		 ( ( GET_LEVEL( victim ) << 3 ) +
		   ( CHECK_SKILL( victim, ZEN_OBLIVITY ) - 60 ) +
		   ( GET_REMORT_GEN( victim ) << 2 ) ) ) / 1000;
    
    } else if ( AFF3_FLAGGED( victim, AFF3_DAMAGE_CONTROL ) ) {
	if ( GET_LEVEL( victim ) < 30 )
	    dam = ( int ) ( dam * 0.90 );
	else if ( GET_LEVEL( victim ) < 35 ) 
	    dam = ( int ) ( dam * 0.85 );
	else if ( GET_LEVEL( victim ) < 40 ) 
	    dam = ( int ) ( dam * 0.80 );
	else if ( GET_LEVEL( victim ) < 45 ) 
	    dam = ( int ) ( dam * 0.75 );
	else if ( GET_LEVEL( victim ) < 47 ) 
	    dam = ( int ) ( dam * 0.70 );
	else if ( GET_LEVEL( victim ) < 49 ) 
	    dam = ( int ) ( dam * 0.65 );
	else 
	    dam = ( int ) ( dam * 0.60 ); 
    }
    else if ( GET_COND( victim, DRUNK ) > 5 )
	dam = ( int ) ( dam * 0.86 );

    
    if ( ( af = affected_by_spell( victim, SPELL_SHIELD_OF_RIGHTEOUSNESS ) ) &&
	 IS_GOOD( victim ) ) {
	if ( !IS_NPC( victim ) ) {
	    if ( af->modifier == GET_IDNUM( victim ) ) {
		dam -= dam * ( ( 5 + ( GET_REMORT_GEN( victim ) >> 1 ) ) / 30 );
	    } else {
		for ( tmp_ch = victim->in_room->people; tmp_ch; tmp_ch = tmp_ch->next_in_room ) {
		    if ( IS_NPC( tmp_ch ) ) {
			if ( af->modifier == ( short int ) -MOB_IDNUM( tmp_ch ) ) {
			    dam -= dam * ( ( 5 + ( GET_REMORT_GEN( tmp_ch ) >> 1 ) ) / 30 );
			    break;
			}
		    }
		    else if ( !IS_NPC( tmp_ch ) ) {
			if ( af->modifier == GET_IDNUM( tmp_ch ) ) {	
			    dam -= dam * ( ( 5 + ( GET_REMORT_GEN( tmp_ch ) >> 1 ) ) / 30 );
			    break;
			}
		    }
		}
	    }
	}
    }

	if ( (af = affected_by_spell( victim, SPELL_STONESKIN ) ) )
		dam -= ( dam * af->level ) / 150;
    else if ( ( af = affected_by_spell( victim, SPELL_BARKSKIN ) ) ||
	 ( af = affected_by_spell( victim, SPELL_DERMAL_HARDENING ) ) )
		dam -= ( dam * af->level ) / 200;


    if ( IS_AFFECTED_2( victim, AFF2_PETRIFIED ) )
	dam = ( int ) ( dam * 0.2 );


    if ( ch ) {
	if ( ( IS_KNIGHT( ch ) || IS_CLERIC( ch ) ) && IS_NEUTRAL( ch ) && !IS_NPC( ch ) )
	    dam = MIN( ( dam * abs( GET_ALIGNMENT( ch ) ) ) / 2000, dam>>2 );  // limit to 1/4 damage
	else if ( IS_MONK( ch ) && !IS_NPC( ch ) && !IS_NEUTRAL( ch ) )
	    dam -= ( dam * abs( GET_ALIGNMENT( ch ) ) ) / 2000;
    
	if ( ( IS_EVIL( ch ) && IS_AFFECTED( victim, AFF_PROTECT_EVIL ) ) ||
	     ( IS_GOOD( ch ) && IS_AFFECTED( victim, AFF_PROTECT_GOOD ) ) ||
	     ( IS_UNDEAD( ch ) && IS_AFFECTED_2( victim, AFF2_PROTECT_UNDEAD ) ) ||
	     ( IS_DEMON( ch ) && IS_AFFECTED_2( victim, AFF2_PROT_DEMONS ) ) ||
	     ( IS_DEVIL( ch ) && IS_AFFECTED_2( victim, AFF2_PROT_DEVILS ) ) )
	    dam = ( int ) ( dam * 0.85 );         /*reduces the damage */
    
    }

    if ( IS_PUDDING( victim ) || IS_SLIME( victim ) || IS_TROLL( victim ) )
	dam >>= 2;

    switch ( attacktype ) {
	case TYPE_OVERLOAD:
	break;
    case SPELL_POISON:
	if ( IS_UNDEAD( victim ) )
	    dam = 0;
	break;
    case SPELL_STIGMATA:
    case TYPE_HOLYOCEAN:
	if ( IS_UNDEAD( victim ) || IS_EVIL( victim ) ) // is_evil added for stigmata
	    dam <<= 1;
	if ( AFF_FLAGGED( victim, AFF_PROTECT_GOOD ) )
	    dam >>= 1;
	break;
    case SPELL_OXIDIZE:
	if ( IS_CYBORG( victim ) )
	    dam <<= 1;
	break;
    case SPELL_DISRUPTION:
	if ( IS_UNDEAD( victim ) || IS_CYBORG( victim ) || IS_ROBOT( victim ) )
	    dam <<= 1;
	break;
    case SPELL_CALL_LIGHTNING:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_LIGHTNING_BREATH:
    case JAVELIN_OF_LIGHTNING:
    case SKILL_ENERGY_FIELD:
    case SKILL_DISCHARGE:
	if ( IS_VAMPIRE( victim ) )
	    dam >>= 1;
	if ( OUTSIDE( victim ) && victim->in_room->zone->weather->sky==SKY_LIGHTNING )
	    dam <<= 1;
	if ( IS_AFFECTED_2( victim, AFF2_PROT_LIGHTNING ) )
	    dam >>= 1;
	if ( IS_CYBORG( victim ) )
	    dam <<= 1;
	break;
    case SPELL_HAILSTORM:
	if ( OUTSIDE( victim ) && victim->in_room->zone->weather->sky >= SKY_RAINING )
	    dam <<= 1;
	break;
    case SPELL_HELL_FIRE:
    case SPELL_BURNING_HANDS:
    case SPELL_FIREBALL:
    case SPELL_FLAME_STRIKE:
    case SPELL_FIRE_ELEMENTAL:
    case SPELL_FIRE_BREATH:
    case TYPE_ABLAZE:
    case SPELL_FIRE_SHIELD:
    case TYPE_FLAMETHROWER:
	if ( IS_VAMPIRE( victim ) && OUTSIDE( victim ) &&
	     victim->in_room->zone->weather->sunlight == SUN_LIGHT && 
	     GET_PLANE( victim->in_room ) < PLANE_ASTRAL )
	    dam <<= 2;
    
	if ( ( victim->in_room->sector_type == SECT_PITCH_PIT ||
	       ROOM_FLAGGED( victim->in_room, ROOM_EXPLOSIVE_GAS ) ) && 
	     !ROOM_FLAGGED( victim->in_room, ROOM_FLAME_FILLED ) ) {
	    send_to_room( "The room bursts into flames!!!\r\n", victim->in_room );
	    if ( victim->in_room->sector_type == SECT_PITCH_PIT )
		rm_aff.description = 
		    str_dup( "   The pitch is ablaze with raging flames!\r\n" );
	    else
		rm_aff.description =
		    str_dup( "   The room is ablaze with raging flames!\r\n" );

	    rm_aff.level = GET_LEVEL( ch );
	    rm_aff.duration = number( 3, 8 );
	    rm_aff.type = RM_AFF_FLAGS;
	    rm_aff.flags = ROOM_FLAME_FILLED;
	    affect_to_room( victim->in_room, &rm_aff );
	    sound_gunshots( victim->in_room, SPELL_FIREBALL, 8, 1 );
	} 
      
	if ( CHAR_WITHSTANDS_FIRE( victim ) )
	    dam >>= 1;
	if ( IS_TROLL( victim ) || IS_PUDDING( victim ) || IS_SLIME( victim ) )
	    dam <<= 2;
	break;
    case SPELL_CONE_COLD:
    case SPELL_CHILL_TOUCH:
    case SPELL_ICY_BLAST:
    case SPELL_FROST_BREATH:
    case TYPE_FREEZING:
	if ( CHAR_WITHSTANDS_COLD( victim ) )
	    dam >>= 1; 
	break;
	case TYPE_BLEED:
		if( !CHAR_HAS_BLOOD(victim) )
			dam = 0;
	break;
    case SPELL_STEAM_BREATH:
    case TYPE_BOILING_PITCH:
	if ( AFF3_FLAGGED( victim, AFF3_PROT_HEAT ) )
	    dam >>= 1;
    case TYPE_ACID_BURN:
    case SPELL_ACIDITY:
    case SPELL_ACID_BREATH:
	if ( affected_by_spell( victim, SPELL_CHEMICAL_STABILITY ) )
	    dam >>= 1;
	break;
    }

    if ( affected_by_spell( victim, SPELL_MANA_SHIELD ) &&  ( !  ( attacktype == TYPE_BLEED ) && 
	 ! ( attacktype == SPELL_POISON ) ) ) { 
	mana_loss = ( dam * GET_MSHIELD_PCT( victim ) ) / 100;
	mana_loss = MAX( MIN( GET_MANA( victim ) - GET_MSHIELD_LOW( victim ), mana_loss ), 0 );
	GET_MANA( victim ) -= mana_loss;
	
	if ( GET_MANA( victim ) <= GET_MSHIELD_LOW( victim ) ) {
	    send_to_char( "Your mana shield has expired.\r\n", victim );
	    affect_from_char( victim, SPELL_MANA_SHIELD );
	}
	
	dam = MAX( 0, dam - mana_loss );
    }
	

    if ( dam_object && dam )
	check_object_killer( dam_object, victim );

    if ( ch ) {
    
	if ( ! cur_weap && ch != victim && dam &&
	     ( attacktype < MAX_SKILLS || attacktype >= TYPE_HIT ) &&
	     ( attacktype > MAX_SPELLS || IS_SET( spell_info[attacktype].routines, MAG_TOUCH ) ) && 
	     ! SPELL_IS_PSIONIC( attacktype ) ) {
	    
	    if ( IS_AFFECTED_3( victim, AFF3_PRISMATIC_SPHERE ) &&
		 attacktype < MAX_SKILLS && 
		 ( CHECK_SKILL( ch, attacktype )+GET_LEVEL( ch ) )
		 <  ( GET_INT( victim ) + number( 70, 130 + GET_LEVEL( victim ) ) ) ) {

		act( "You are deflected by $N's prismatic sphere!",
		     FALSE, ch, 0, victim, TO_CHAR );
		act( "$n is deflected by $N's prismatic sphere!",
		     FALSE, ch, 0, victim, TO_NOTVICT );
		act( "$n is deflected by your prismatic sphere!",
		     FALSE, ch, 0, victim, TO_VICT );
		damage( victim,ch,dice( 30, 3 )+( dam >> 2 ), SPELL_PRISMATIC_SPHERE, -1 );
		gain_skill_prof( victim, SPELL_PRISMATIC_SPHERE );
		DAM_RETURN( TRUE );
	    }

	    // electrostatic field
	    if ( ( af = affected_by_spell( victim, SPELL_ELECTROSTATIC_FIELD ) ) ) {
		
		if ( ! CHAR_WITHSTANDS_ELECTRIC( ch ) && ! mag_savingthrow( ch, af->level, SAVING_ROD ) ) {
		    
		    // reduces duration if it hits
		    if ( af->duration > 1 ) {
			af->duration--;
		    }
		    
		    if ( damage( victim, ch, dice( 3, af->level ), SPELL_ELECTROSTATIC_FIELD, -1 ) ) {
			gain_skill_prof( victim, SPELL_ELECTROSTATIC_FIELD );
			DAM_RETURN( FALSE );
		    }
		}
	    }

	    if ( attacktype < MAX_SKILLS &&
		 attacktype != SKILL_BEHEAD && attacktype != SKILL_SECOND_WEAPON && attacktype != SKILL_IMPALE ) {
		
		if ( IS_AFFECTED_3( victim, AFF3_PRISMATIC_SPHERE ) &&
		     !mag_savingthrow( ch, GET_LEVEL( victim ), SAVING_ROD ) ) {
		    if ( damage( victim, ch, dice( 30, 3 ) + 
				 ( IS_MAGE( victim ) ? ( dam >> 2 ) : 0 ),
				 SPELL_PRISMATIC_SPHERE,-1 ) ) {
			DAM_RETURN( TRUE );
		    }
		    WAIT_STATE( ch, PULSE_VIOLENCE );
		} else if ( IS_AFFECTED_2( victim, AFF2_BLADE_BARRIER ) ) {
		    if ( damage( victim,ch,GET_LEVEL( victim ) +( dam >> 4 ),
				 SPELL_BLADE_BARRIER,-1 ) ) {
			DAM_RETURN( TRUE );
		    }
		} else if ( IS_AFFECTED_2( victim, AFF2_FIRE_SHIELD ) && 
			    attacktype != SKILL_BACKSTAB &&
			    !mag_savingthrow( ch, GET_LEVEL( victim ), SAVING_BREATH ) &&  
			    !IS_AFFECTED_2( ch, AFF2_ABLAZE ) &&  
			    !CHAR_WITHSTANDS_FIRE( ch ) ) {
		    if ( damage( victim, ch, dice( 8, 8 ) + 
				 ( IS_MAGE( victim ) ? ( dam >> 3 ) : 0 ),
				 SPELL_FIRE_SHIELD,-1 ) ) {
			DAM_RETURN( FALSE );
		    }
		    SET_BIT( AFF2_FLAGS( ch ), AFF2_ABLAZE );
		} else if ( IS_AFFECTED_2( victim, AFF2_ENERGY_FIELD ) ) {
		    af = affected_by_spell( victim, SKILL_ENERGY_FIELD );
		    if ( !mag_savingthrow( ch, 
					   af ? af->level : GET_LEVEL( victim ), 
					   SAVING_ROD ) &&  
			 !CHAR_WITHSTANDS_ELECTRIC( ch ) ) {
			if ( damage( victim, ch, 
				     af ? dice( 3, MAX( 10, af->level ) ) : dice( 3, 8 ),
				     SKILL_ENERGY_FIELD, -1 ) ) {
			    DAM_RETURN( FALSE );
			}
			GET_POS(ch) = POS_SITTING;
			WAIT_STATE(ch, 2 RL_SEC);
			//DAM_RETURN( TRUE );
		    }
		}
	    }
	}
    }

    // rangers' critical hit
    if ( ch && IS_RANGER( ch ) && dam > 10 &&
	 IS_WEAPON( attacktype ) &&
	 number( 0, 74 ) <= GET_REMORT_GEN( ch ) ) {
	send_to_char( "CRITICAL HIT!\r\n", ch );
	act( "$n has scored a CRITICAL HIT!\r\n", FALSE, ch, 0, victim, TO_VICT );
	dam += ( dam * ( GET_REMORT_GEN( ch )+4 ) ) / 14;
    }

    if ( ch ) 
	hard_damcap = MAX( 20 + GET_LEVEL( ch ) + ( GET_REMORT_GEN( ch ) * 2 ),
			   GET_LEVEL( ch ) * 20 + 
			   ( GET_REMORT_GEN( ch ) * GET_LEVEL( ch ) * 2 ) );
    else
	hard_damcap = 7000;

    dam = MAX( MIN( dam, hard_damcap ), 0 );

    //
    // characters under the effects of vampiric regeneration
    //
    
    if ( ( af = affected_by_spell( victim, SPELL_VAMPIRIC_REGENERATION ) ) ) {
	
	// pc caster
	
	if ( !IS_NPC( ch ) && GET_IDNUM( ch ) == af->modifier ) {

	    sprintf( buf, "You drain %d hitpoints from $N!", dam );
	    act( buf, FALSE, ch, 0, victim, TO_CHAR );
	    GET_HIT( ch ) = MIN( GET_MAX_HIT( ch ), GET_HIT( ch ) + dam );

	    af->duration--;

	    if ( af->duration <= 0 ) {
		affect_remove( victim, af );
	    }
	}
    }
	    

    GET_HIT( victim ) -= dam;

    if ( !IS_NPC( victim ) )
	GET_TOT_DAM( victim ) += dam;

    if ( ch 
		&& ch != victim 
		&& !(
			MOB2_FLAGGED( victim, MOB2_UNAPPROVED ) || 
	 		PLR_FLAGGED( ch, PLR_TESTER ) ||
			IS_PET(ch) || IS_PET(victim)
			) 
		)
	gain_exp( ch, MIN( GET_LEVEL( ch ) * GET_LEVEL( ch ) * GET_LEVEL( ch ),
			   GET_LEVEL( victim ) * dam ) );

    // check for killer flags and remove sleep/etc...
    if ( ch && ch != victim ) {

	// remove sleep/flood on a damaging hit
	if ( dam ) {
	    if ( IS_AFFECTED( victim, AFF_SLEEP ) ) {
		if ( affected_by_spell( victim, SPELL_SLEEP ) )
		    affect_from_char( victim, SPELL_SLEEP );
		if ( affected_by_spell( victim, SPELL_MELATONIC_FLOOD ) )
		    affect_from_char( victim, SPELL_MELATONIC_FLOOD );
	    }
	}

	// remove stasis even on a miss
	if ( AFF3_FLAGGED( victim, AFF3_STASIS ) ) {
	    send_to_char( "Emergency restart of system procesess...\r\n", victim );
	    REMOVE_BIT( AFF3_FLAGS( victim ), AFF3_STASIS );
	    WAIT_STATE( victim, ( 5 - ( CHECK_SKILL( victim, SKILL_FASTBOOT ) >> 5 ) ) RL_SEC );
	}

	// check for killer flags right here
	if ( ch != FIGHTING( victim) && !IS_DEFENSE_ATTACK( attacktype ) ) {
	    
	    check_killer( ch, victim, "secondary in damage()" );
	    check_toughguy( ch, victim, 0 );
	}
    }

    update_pos( victim );

    /*
     * skill_message sends a message from the messages file in lib/misc.
     * dam_message just sends a generic "You hit $n extremely hard.".
     * skill_message is preferable to dam_message because it is more
     * descriptive.
     * 
     * If we are _not_ attacking with a weapon ( i.e. a spell ), always use
     * skill_message. If we are attacking with a weapon: If this is a miss or a
     * death blow, send a skill_message if one exists; if not, default to a
     * dam_message. Otherwise, always send a dam_message.
     */

    if ( !IS_WEAPON( attacktype ) ) {
	if ( ch ) {
	    skill_message( dam, ch, victim, attacktype );
	}
    
	// some "non-weapon" attacks involve a weapon, e.g. backstab
	if ( weap )
	    damage_eq( ch, weap, MAX( weap_dam, dam >> 6 ) );
	    
	if ( obj ) {
	    damage_eq( ch, obj, eq_dam );
	}
	if ( impl && impl_dam )
	    damage_eq( ch, impl, impl_dam );

	// ignite the victim if applicable
	if ( !IS_AFFECTED_2( victim, AFF2_ABLAZE ) && 
	     ( attacktype== SPELL_FIREBALL || 
	       attacktype == SPELL_FIRE_BREATH || 
	       attacktype == SPELL_HELL_FIRE || 
	       attacktype == SPELL_FLAME_STRIKE || 
	       attacktype == SPELL_METEOR_STORM ||
	       attacktype == TYPE_FLAMETHROWER ||
	       attacktype == SPELL_FIRE_ELEMENTAL ) ) {
	    if ( !mag_savingthrow( victim, 50, SAVING_BREATH ) && 
		 !CHAR_WITHSTANDS_FIRE( victim ) ) {
		act( "$n's body suddenly ignites into flame!", 
		     FALSE, victim, 0, 0, TO_ROOM );
		act( "Your body suddenly ignites into flame!", 
		     FALSE, victim, 0, 0, TO_CHAR );
		SET_BIT( AFF2_FLAGS( victim ), AFF2_ABLAZE );
	    }
	}

	// transfer sickness if applicable
	if ( ch && ch != victim &&
	     ( ( attacktype > MAX_SPELLS && attacktype < MAX_SKILLS &&
		 attacktype != SKILL_BEHEAD && attacktype != SKILL_IMPALE ) ||
	       ( attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING ) ) ) {
	    if ( ( IS_SICK( ch ) || IS_ZOMBIE( ch ) ||
		   ( ch->in_room->zone->number == 163 && !IS_NPC( victim ) ) ) &&
		 !IS_SICK( victim ) && !IS_UNDEAD( victim ) ) {
		call_magic( ch, victim, 0, SPELL_SICKNESS, GET_LEVEL( ch ), CAST_PARA ); 
	    } 
	}
    
    } 

    // it is a weapon attack
    else if ( ch ) {
    
	if ( GET_POS( victim ) == POS_DEAD || dam == 0 ) {
	    if ( !skill_message( dam, ch, victim, attacktype ) )
		dam_message( dam, ch, victim, attacktype, location );
	} else
	    dam_message( dam, ch, victim, attacktype, location );

	if ( obj && eq_dam )
	    damage_eq( ch, obj, eq_dam );
	if ( impl && impl_dam )
	    damage_eq( ch, impl, impl_dam );

	if ( weap && ( attacktype != SKILL_PROJ_WEAPONS ) && attacktype != SKILL_ENERGY_WEAPONS )
	    damage_eq( ch, weap, MAX( weap_dam, dam >> 6 ) );

	if ( IS_ALIEN_1( victim ) && ( attacktype == TYPE_SLASH  || 
				       attacktype == TYPE_RIP    ||
				       attacktype == TYPE_BITE   || 
				       attacktype == TYPE_CLAW   ||
				       attacktype == TYPE_PIERCE || 
				       attacktype == TYPE_STAB   ||
				       attacktype == TYPE_CHOP   || 
				       attacktype == SPELL_BLADE_BARRIER ) ) {
	    for ( tmp_ch = victim->in_room->people; tmp_ch; tmp_ch = next_ch ) {
		next_ch = tmp_ch->next_in_room;
		if ( tmp_ch == victim || number( 0, 8 ) )
		    continue;
		damage( victim, tmp_ch, dice( 4, GET_LEVEL( victim ) ),TYPE_ALIEN_BLOOD,-1 );
	    }
	}
    }

    // Use send_to_char -- act(  ) doesn't send message if you are DEAD.
    switch ( GET_POS( victim ) ) {

	// Mortally wounded
    case POS_MORTALLYW:
	act( "$n is mortally wounded, and will die soon, if not aided.", 
	     TRUE, victim, 0, 0, TO_ROOM );
	send_to_char( "You are mortally wounded, and will die soon, if not aided.\r\n", victim );
	if ( ch && IS_VAMPIRE( ch ) && IS_AFFECTED_3( ch, AFF3_FEEDING ) ) {
	    stop_fighting( ch );
	}
	break;

	// Incapacitated
    case POS_INCAP:
	act( "$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM );
	send_to_char( "You are incapacitated and will slowly die, if not aided.\r\n", victim );
	if ( ch && IS_VAMPIRE( ch ) && IS_AFFECTED_3( ch, AFF3_FEEDING ) ) {
	    stop_fighting( ch );
	}

	break;

	// Stunned
    case POS_STUNNED:
	act( "$n is stunned, but will probably regain consciousness again.", 
	     TRUE, victim, 0, 0, TO_ROOM );
	send_to_char( "You're stunned, but will probably regain consciousness again.\r\n", victim );
	if ( ch && IS_VAMPIRE( ch ) && IS_AFFECTED_3( ch, AFF3_FEEDING ) ) {
	    stop_fighting( ch );
	}
	break;

	// Dead
    case POS_DEAD:
	act( "$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM );
	send_to_char( "You are dead!  Sorry...\r\n", victim );
	break;

	// pos >= Sleeping ( Fighting, Standing, Flying, Mounted... )
    default:
    {
	if ( dam > ( GET_MAX_HIT( victim ) >> 2 ) )
	    act( "That really did HURT!", FALSE, victim, 0, 0, TO_CHAR );

	if ( GET_HIT( victim ) < ( GET_MAX_HIT( victim ) >> 2 ) &&
	     GET_HIT( victim ) < 200 ) {
	    sprintf( buf2, "%sYou wish that your wounds would stop %sBLEEDING%s%s so much!%s\r\n",
		     CCRED( victim, C_SPR ), CCBLD( victim, C_SPR ), CCNRM( victim, C_SPR ),
		     CCRED( victim, C_SPR ), CCNRM( victim, C_SPR ) );
	    send_to_char( buf2, victim );
	}
	if ( ch && IS_NPC( victim ) && ch != victim && 
	     !KNOCKDOWN_SKILL( attacktype ) &&
	     GET_POS( victim ) > POS_SITTING && !MOB_FLAGGED( victim, MOB_SENTINEL ) &&
	     !IS_DRAGON( victim ) && !IS_UNDEAD( victim ) &&
	     GET_CLASS( victim ) != CLASS_ARCH &&
	     GET_CLASS( victim ) != CLASS_DEMON_PRINCE &&
	     GET_MOB_WAIT( ch ) <= 0 && !MOB_FLAGGED( ch, MOB_SENTINEL ) &&
	     ( 100 - ( ( GET_HIT( victim ) * 100 ) / GET_MAX_HIT( victim ) ) ) > 
	     GET_MORALE( victim ) + number( -5, 10 + ( GET_INT( victim ) >> 2 ) )
	   ) {
		if(GET_HIT(victim) > 0) {
			do_flee( victim, "", 0, 0 );
		} else {
			sprintf(buf,"ERROR: %s was at position %d with %d hit points and tried to flee.",GET_NAME(victim),GET_POS(victim),GET_HIT(victim));
			mudlog( buf, BRF, LVL_DEMI, TRUE );
		}
	}
	if ( IS_CYBORG( victim ) && !IS_NPC( victim ) && 
	     GET_TOT_DAM( victim ) >= max_component_dam( victim ) ){
	    if ( GET_BROKE( victim ) ) {
		if ( !AFF3_FLAGGED( victim, AFF3_SELF_DESTRUCT ) ) {
		    sprintf( buf, "Your %s has been severely damaged.\r\n"
			     "Systems cannot support excessive damage to this and %s.\r\n"
			     "%sInitiating Self-Destruct Sequence.%s\r\n",
			     component_names[number( 1, NUM_COMPS )][GET_OLD_CLASS( victim )],
			     component_names[( int )GET_BROKE( victim )][GET_OLD_CLASS( victim )],
			     CCRED_BLD( victim, C_NRM ), CCNRM( victim, C_NRM ) );
		    send_to_char( buf, victim );
		    act( "$n has auto-initiated self destruct sequence!\r\n"
			 "CLEAR THE AREA!!!!", FALSE, victim, 0, 0, TO_ROOM | TO_SLEEP );
		    MEDITATE_TIMER( victim ) = 4; 
		    SET_BIT( AFF3_FLAGS( victim ), AFF3_SELF_DESTRUCT );
		    WAIT_STATE( victim, PULSE_VIOLENCE*7 );      /* Just enough to die */
		}
	    } else {
		GET_BROKE( victim ) = number( 1, NUM_COMPS );
		act( "$n staggers and begins to smoke!", FALSE, victim, 0, 0, TO_ROOM );
		sprintf( buf, "Your %s has been damaged!\r\n", 
			 component_names[( int )GET_BROKE( victim )][GET_OLD_CLASS( victim )] );
		send_to_char( buf, victim );
		GET_TOT_DAM( victim ) = 0;
	    }
	
	}
   
	// this is the block that handles things that happen when someone is damaging
	// someone else.  ( not damaging self or being damaged by a null char, e.g. bomb )
    
	if ( victim != ch ) {
	    
	    // see if the victim needs to run screaming in terror!
	    if ( !IS_NPC( victim ) &&
		 GET_HIT( victim ) < GET_WIMP_LEV( victim ) ) {
		send_to_char( "You wimp out, and attempt to flee!\r\n", victim );
		if ( KNOCKDOWN_SKILL( attacktype ) && damage )
		    GET_POS( victim ) = POS_SITTING;
		do_flee( victim, "", 0, 0 );
		break;
	    } 
	    else if ( affected_by_spell( victim, SPELL_FEAR ) && 
		      !number( 0, ( GET_LEVEL( victim ) >> 3 ) + 1 ) ) {
		if ( KNOCKDOWN_SKILL( attacktype ) && damage )
		    GET_POS( victim ) = POS_SITTING;
		do_flee( victim, "", 0, 0 );
		break;
	    }

	    // this block handles things that happen IF and ONLY IF the ch has
	    // initiated the attack.  We assume that ch is attacking unless the
	    // damage is from a DEFENSE ATTACK
	    if ( ch && !IS_DEFENSE_ATTACK( attacktype ) ) {
	    
		// ch is initiating an attack ?
		// only if ch is not "attacking" with a fireshield/energy shield/etc...
		if ( !FIGHTING( ch ) ) {
		
		    // mages casting spells and shooters should be able to attack
		    // without initiating melee ( on their part at least )
		    if ( attacktype != SKILL_ENERGY_WEAPONS && 
			 attacktype != SKILL_PROJ_WEAPONS &&
			 ( !IS_MAGE( ch ) ||
			   attacktype > MAX_SPELLS ||
			   !SPELL_IS_MAGIC( attacktype ) ) ) {
			set_fighting( ch, victim, TRUE );
		    }
		
		}
	    
		// add ch to victim's shitlist( s )
		if ( !IS_NPC( ch ) && !PRF_FLAGGED( ch, PRF_NOHASSLE ) ) {
		    if ( MOB_FLAGGED( victim, MOB_MEMORY ) )
			remember( victim, ch );
		    if ( MOB2_FLAGGED( victim, MOB2_HUNT ) )
			HUNTING( victim ) = ch;
		}
	    
		// make the victim retailiate against the attacker
		if ( !FIGHTING( victim ) ) {
		    set_fighting( victim, ch, FALSE );
		}
	    }
	}
	break;
    }
    }   /* end switch ( GET_POS( victim ) ) */
    

    // Victim is Linkdead
    // Send him to the void

    if ( !IS_NPC( victim ) && !( victim->desc ) ) {
	do_flee( victim, "", 0, 0 );
	act( "$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM );
	GET_WAS_IN( victim ) = victim->in_room;
	if ( ch ) {
	    stop_fighting( victim );
	    stop_fighting( ch );
	}
	char_from_room( victim );
	char_to_room( victim, zone_table->world );
	REMOVE_BIT( AFF2_FLAGS( victim ), AFF2_ABLAZE );
    }

    /* debugging message */
    if ( ch && PRF2_FLAGGED( ch, PRF2_FIGHT_DEBUG ) ) {
	sprintf( buf, "<%s> ( dam:%4d ) ( Wait: %2d ) ( Pos: %d )\r\n", GET_NAME( victim ),
		 dam, IS_NPC( victim ) ? GET_MOB_WAIT( victim ) :
		 victim->desc ? victim->desc->wait : 0, GET_POS( victim ) );
	send_to_char( buf, ch );
    }

    // If victim is asleep, incapacitated, etc.. stop fighting.
    if ( !AWAKE( victim ) )
	if ( FIGHTING( victim ) )
	    stop_fighting( victim );

    // Victim has been slain, handle all the implications
    // Exp, Kill counter, etc...
    if ( GET_POS( victim ) == POS_DEAD ) {
	if ( ch ) {
	    gain_kill_exp( ch, victim );

	    if ( !IS_NPC( victim ) ) {
		if ( victim != ch ) {
		    GET_PKILLS( ch ) += 1;
		    sprintf( buf2, "%s %skilled by %s at %s ( %d )", GET_NAME( victim ), 
			     !IS_NPC( ch ) ? "p" : "", GET_NAME( ch ),
			     victim->in_room->name, victim->in_room->number );

		    if ( ROOM_FLAGGED( victim->in_room, ROOM_ARENA ) ) {
			strcat( buf2, " [ARENA]" );
		    }

		} else
		    sprintf( buf2, "%s died%s%s at %s ( %d )", GET_NAME( ch ),
			     ( attacktype <= TOP_NPC_SPELL ) ? " by " : "",
			     ( attacktype <= TOP_NPC_SPELL ) ? spells[attacktype] : "",
			     ch->in_room->name, ch->in_room->number );
		mudlog( buf2, BRF, GET_INVIS_LEV( victim ), TRUE );
		if ( MOB_FLAGGED( ch, MOB_MEMORY ) )
		    forget( ch, victim );
		if ( !IS_NPC( ch ) && ch != victim )
		    is_humil = TRUE;
	    } else
		GET_MOBKILLS( ch ) += 1;

	    if ( HUNTING( ch ) && HUNTING( ch ) == victim )
		HUNTING( ch ) = NULL; 

	    die( victim, ch, attacktype, is_humil );
	    DAM_RETURN( TRUE );

	} 

	if ( !IS_NPC( victim ) ) {
	    sprintf( buf, "%s killed by NULL-char ( type %d ( %s ) ) at %d.", 
		     GET_NAME( victim ), attacktype,
		     ( attacktype > 0 && attacktype < TOP_NPC_SPELL ) ?
		     spells[attacktype]  :
		     ( attacktype >= TYPE_HIT && attacktype <= TOP_ATTACKTYPE ) ?
		     attack_type[attacktype-TYPE_HIT] : "bunk",
		     victim->in_room->number );
	    mudlog( buf, BRF, LVL_AMBASSADOR, TRUE );
	}
	die( victim, NULL, attacktype, is_humil );
	DAM_RETURN( TRUE );
    } /* if GET_POS( victim ) == POS_DEAD */
  
    DAM_RETURN( FALSE );
}

int
hit( struct char_data * ch, struct char_data * victim, int type )
{

    struct char_data *tch;
    int w_type = 0, victim_ac, calc_thaco, dam, tmp_dam, diceroll, skill = 0;
    int i, metal_wt, dual_prob = 0;
    byte limb;
    struct obj_data *weap = NULL, *weap2 = NULL;

    if ( ch->in_room != victim->in_room ) {
	if ( FIGHTING( ch ) && FIGHTING( ch ) == victim )
	    stop_fighting( ch );
	return 0;
    }

    if ( ROOM_FLAGGED( ch->in_room, ROOM_PEACEFUL ) && 
	 !PLR_FLAGGED( victim, PLR_KILLER ) && GET_LEVEL( ch ) < LVL_CREATOR ) {
	send_to_char( "This room just has such a peaceful, easy feeling...\r\n",ch );
	return 0;
    }
    if ( LVL_AMBASSADOR <= GET_LEVEL( ch ) && GET_LEVEL( ch ) < LVL_GOD  && 
	 IS_NPC( victim ) && !mini_mud ) {
	send_to_char( "You are not allowed to attack mobiles!\r\n", ch );
	return 0;
    }
    if ( IS_AFFECTED_2( ch, AFF2_PETRIFIED ) && GET_LEVEL( ch ) < LVL_ELEMENT ) {
	if ( !number( 0, 2 ) )
	    act( "You want to fight back against $N's attack, but cannot!",
		 FALSE, ch, 0, victim, TO_CHAR|TO_SLEEP );
	else if ( !number( 0, 1 ) )
	    send_to_char( "You have been turned to stone, and cannot fight!\r\n",ch );
	else
	    send_to_char( "You cannot fight back!!  You are petrified!\r\n", ch );
    
	return 0;
    }
  
    if ( GET_LEVEL( victim ) <= LVL_PROTECTED && !IS_NPC( ch ) && !IS_NPC( victim ) &&
	 !PLR_FLAGGED( victim, PLR_TOUGHGUY ) &&
	 GET_LEVEL( ch ) < LVL_IMMORT ) {
	act( "$N is currently under new character protection.",
	     FALSE, ch, 0, victim, TO_CHAR );
	act( "You are protected by the gods against $n's attack!",
	     FALSE, ch, 0, victim, TO_VICT );	  
	sprintf( buf, "%s protected against %s ( hit ) at %d\n",
		 GET_NAME( victim ), GET_NAME( ch ), victim->in_room->number );
	slog( buf );
    
	if ( victim == FIGHTING( ch ) )
	    stop_fighting( ch );
	if ( ch == FIGHTING( victim ) )
	    stop_fighting( victim );
	return 0;
    }

    if ( MOUNTED( ch ) ) {
	if ( MOUNTED( ch )->in_room != ch->in_room ) {
	    REMOVE_BIT( AFF2_FLAGS( MOUNTED( ch ) ), AFF2_MOUNTED );
	    MOUNTED( ch ) = NULL;
	} else
	    send_to_char( "You had better dismount first.\r\n", ch );
	return 0;
    }
    if ( MOUNTED( victim ) ) {
	REMOVE_BIT( AFF2_FLAGS( MOUNTED( victim ) ), AFF2_MOUNTED );
	MOUNTED( victim ) = NULL;
	act( "You are knocked from your mount by $N's attack!",
	     FALSE, victim, 0, ch, TO_CHAR );
    } 
    if ( IS_AFFECTED_2( victim, AFF2_MOUNTED ) ) {
	REMOVE_BIT( AFF2_FLAGS( victim ), AFF2_MOUNTED );
	for ( tch = ch->in_room->people;tch;tch = tch->next_in_room )
	    if ( MOUNTED( tch ) && MOUNTED( tch ) == victim ) {
		act( "You are knocked from your mount by $N's attack!", 
		     FALSE, tch, 0, ch, TO_CHAR );
		MOUNTED( tch ) = NULL;
		GET_POS( tch ) = POS_STANDING;
	    }
    }
  
    for ( i = 0, metal_wt = 0; i < NUM_WEARS; i++ )
	if ( ch->equipment[i] && 
	     GET_OBJ_TYPE( ch->equipment[i] ) == ITEM_ARMOR &&
	     ( IS_METAL_TYPE( ch->equipment[i] ) ||
	       IS_STONE_TYPE( ch->equipment[i] ) ) )
	    metal_wt += ch->equipment[i]->getWeight();


    if ( ( type != SKILL_BACKSTAB && type != SKILL_CIRCLE &&
	   type != SKILL_BEHEAD ) || !cur_weap ) {
    
	if ( GET_EQ( ch, WEAR_WIELD_2 ) && GET_EQ( ch, WEAR_WIELD ) ) {
	    dual_prob = ( GET_EQ( ch, WEAR_WIELD )->getWeight() - 
			  GET_EQ( ch, WEAR_WIELD_2 )->getWeight() ) * 2;
	}
	if (type == SKILL_IMPLANT_W || type == SKILL_ADV_IMPLANT_W) {
		cur_weap = get_random_uncovered_implant(ch, ITEM_WEAPON);
		if(!cur_weap)
			return 0;
	} else if ( !( ( ( cur_weap = GET_EQ( ch, WEAR_WIELD_2 ) ) && 
		  IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) &&
		  ( CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) + dual_prob ) > 
		  number( 50, 150 ) ) ||
		( ( cur_weap = GET_EQ( ch, WEAR_WIELD ) ) && !number( 0, 1 ) ) ||
		( ( cur_weap = GET_EQ( ch, WEAR_HANDS ) ) && 
		  ( !cur_weap || !number( 0, 5 ) ) && 
		  IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) ) ||
		( ( cur_weap = GET_IMPLANT( ch, WEAR_HANDS ) ) &&
		  IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) && 
		  !GET_EQ( ch, WEAR_HANDS ) &&
		  ( !cur_weap || !number( 0, 2 ) ) ) ||
		( ( cur_weap = GET_EQ( ch, WEAR_FEET ) ) && 
		  CHECK_SKILL( ch, SKILL_KICK ) > number( 10, 170 ) &&
		  IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) ) ||
		( ( cur_weap = GET_EQ( ch, WEAR_HEAD ) ) &&
		  IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) && !number( 0, 2 ) ) ) &&
	     !( cur_weap = GET_EQ( ch, WEAR_WIELD ) ) )
	    cur_weap = GET_EQ( ch, WEAR_HANDS );
    
	if ( cur_weap ) {
      
	    if ( IS_ENERGY_GUN( cur_weap ) || IS_GUN( cur_weap ) ) {
		w_type = TYPE_BLUDGEON;
	    } else if ( IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) )
		w_type = GET_OBJ_VAL( cur_weap, 3 ) + TYPE_HIT;
	    else
		cur_weap = NULL;
	}
    }

    if ( !cur_weap ) {
	if ( IS_NPC( ch ) && ( ch->mob_specials.shared->attack_type != 0 ) )
	    w_type = ch->mob_specials.shared->attack_type + TYPE_HIT;
	else if ( IS_TABAXI( ch ) && number( 0, 1 ) )
	    w_type = TYPE_CLAW;
	else 
	    w_type = TYPE_HIT;
    }

    weap = cur_weap;
    calc_thaco = calculate_thaco( ch, victim, weap );

    victim_ac = MAX( GET_AC( victim ), -( GET_LEVEL( ch ) << 2 ) ) / 10;

    diceroll = number( 1, 20 );

    /* decide whether this is a hit or a miss */
    if ( ( 
	( diceroll < 20 ) && AWAKE( victim ) &&
	( ( diceroll == 1 ) || ( ( calc_thaco - diceroll ) ) > victim_ac ) ) ) { 

	if ( type == SKILL_BACKSTAB || type == SKILL_CIRCLE || 
	     type == SKILL_SECOND_WEAPON )
	    return ( damage( ch, victim, 0, type, -1 ) );
	else
	    return ( damage( ch, victim, 0, w_type, -1 ) );
    }

    /* IT's A HIT! */

    if ( CHECK_SKILL( ch, SKILL_DBL_ATTACK ) > 60 )
	gain_skill_prof( ch, SKILL_DBL_ATTACK );
    if ( CHECK_SKILL( ch, SKILL_TRIPLE_ATTACK ) > 60 )
	gain_skill_prof( ch, SKILL_TRIPLE_ATTACK );
  
    /* okay, it's a hit.  calculate limb */

    limb = choose_random_limb( victim );

    if ( type == SKILL_BACKSTAB || type == SKILL_CIRCLE )
	limb = WEAR_BACK;
    else if ( type == SKILL_IMPALE || type == SKILL_LUNGE_PUNCH )
	limb = WEAR_BODY;
    else if ( type == SKILL_BEHEAD )
	limb = WEAR_NECK_1;

    /* okay, we know the guy has been hit.  now calculate damage. */
    dam = str_app[STRENGTH_APPLY_INDEX( ch )].todam;
    dam += GET_DAMROLL( ch );
    tmp_dam = dam;

    if ( cur_weap ) {
	if ( IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) ) {
	    dam += dice( GET_OBJ_VAL( cur_weap, 1 ), GET_OBJ_VAL( cur_weap, 2 ) );
	    if ( invalid_char_class( ch, cur_weap ) )
            dam >>= 1;
	    if ( IS_NPC( ch ) ) {
            tmp_dam += dice( ch->mob_specials.shared->damnodice, 
                     ch->mob_specials.shared->damsizedice );
            dam = MAX( tmp_dam, dam );
	    }
	    if ( GET_OBJ_VNUM( cur_weap ) > 0 ) {
            for ( i = 0; i < MAX_WEAPON_SPEC; i++ )
                if ( GET_WEAP_SPEC( ch, i ).vnum == GET_OBJ_VNUM( cur_weap ) ) {
                    dam += GET_WEAP_SPEC( ch, i ).level;
                    break;
                }
	    } 
        if(IS_TWO_HAND(cur_weap) && IS_BARB(ch)) {
            int dam_add;
            dam_add = cur_weap->getWeight() / 2;
            if (CHECK_SKILL(ch,SKILL_DISCIPLINE_OF_STEEL) > 30) {
                dam_add += (cur_weap->getWeight() * (GET_LEVEL(ch) + 1) / 100);
                dam_add += (cur_weap->getWeight() * (GET_REMORT_GEN(ch) + 1) / 11);
                dam_add = dam_add * CHECK_SKILL(ch,SKILL_DISCIPLINE_OF_STEEL) / 100;
            }
            dam += dam_add;
        }
	} else if ( IS_OBJ_TYPE( cur_weap, ITEM_ARMOR ) ) {
	    dam += ( GET_OBJ_VAL( cur_weap, 0 ) / 3 );
	} else
	    dam += dice( 1, 4 ) + number( 0, cur_weap->getWeight() );
    } else if ( IS_NPC( ch ) ) {
        tmp_dam += dice( ch->mob_specials.shared->damnodice, 
                 ch->mob_specials.shared->damsizedice );
        dam = MAX( tmp_dam, dam );
    } else {
        dam += number( 0, 3 );	/* Max. 3 dam with bare hands */
	if ( IS_MONK( ch ) || IS_VAMPIRE( ch ) )
	    dam += number( ( GET_LEVEL( ch ) >> 2 ), GET_LEVEL( ch ) );
	else if ( IS_BARB( ch ) )
	    dam += number( 0, ( GET_LEVEL( ch ) >> 1 ) );
    }

    dam = MAX( 1, dam );		/* at least 1 hp damage min per hit */
    dam = MIN( dam, 30 + ( GET_LEVEL( ch ) << 3 ) );       /* level limit */

    if ( type == SKILL_BACKSTAB ) {
	gain_skill_prof( ch, type );
	if ( IS_THIEF( ch ) ) {
	    dam *= BACKSTAB_MULT( ch );
	    dam += number( 0, ( CHECK_SKILL( ch, SKILL_BACKSTAB ) - LEARNED( ch ) ) >> 1 );
	}
	if ( damage( ch, victim, dam, SKILL_BACKSTAB, WEAR_BACK ) )
	    return 1;
    } else if ( type == SKILL_CIRCLE ) {
	if ( IS_THIEF( ch ) ) {
	    dam *= MAX( 2, ( BACKSTAB_MULT( ch ) >> 1 ) );
	    dam += number( 0, ( CHECK_SKILL( ch, SKILL_CIRCLE ) - LEARNED( ch ) ) >> 1 );
	}
	gain_skill_prof( ch, type );
	if ( damage( ch, victim, dam, SKILL_CIRCLE, WEAR_BACK ) ) 
	    return 1;
    } else {
	if ( cur_weap && IS_OBJ_TYPE( cur_weap, ITEM_WEAPON ) && 
	     GET_OBJ_VAL( cur_weap, 3 ) >= 0 &&
	     GET_OBJ_VAL( cur_weap, 3 ) < TOP_ATTACKTYPE - TYPE_HIT &&
	     IS_MONK( ch ) ) {
	    skill = weapon_proficiencies[GET_OBJ_VAL( cur_weap, 3 )];
	    if ( skill )
		gain_skill_prof( ch, skill );
	}
	if ( damage( ch, victim, dam, w_type, limb ) )
	    return 1;

	if ( weap && IS_FERROUS(weap) && IS_NPC( victim ) && GET_MOB_SPEC( victim ) == rust_monster ) {

	    if ( ( !IS_OBJ_STAT( weap, ITEM_MAGIC ) || 
		   mag_savingthrow( ch, GET_LEVEL( victim ), SAVING_ROD ) ) &&
		 ( !IS_OBJ_STAT( weap, ITEM_MAGIC_NODISPEL ) || 
		   mag_savingthrow( ch, GET_LEVEL( victim ), SAVING_ROD ) ) ) {
	  
		act( "$p spontaneously oxidizes and crumbles into a pile of rust!",
		     FALSE, ch, weap, 0, TO_CHAR);
		act( "$p crumbles into rust on contact with $N!",
		     FALSE, ch, weap, victim, TO_ROOM );
	  
		extract_obj( weap );
	  
		if ( ( weap = read_object( RUSTPILE ) ) )
		    obj_to_room( weap, ch->in_room );
		return 0;
	    }
	}

	if ( weap && IS_OBJ_TYPE( weap, ITEM_WEAPON ) &&
	     IS_OBJ_STAT2( weap, ITEM2_CAST_WEAPON ) &&
	     weap == GET_EQ( ch, weap->worn_on ) &&  victim != ch->next &&
	     GET_OBJ_VAL( weap, 0 ) > 0 &&
	     GET_OBJ_VAL( weap, 0 ) < MAX_SPELLS &&
	     ( ( ( !SPELL_IS_MAGIC( GET_OBJ_VAL( weap, 0 ) ) &&
		   !SPELL_IS_DIVINE( GET_OBJ_VAL( weap, 0 ) ) ) ||
		 !IS_OBJ_STAT( weap, ITEM_MAGIC ) ||
		 !ROOM_FLAGGED( ch->in_room, ROOM_NOMAGIC ) ) &&
	       ( !SPELL_IS_PHYSICS( GET_OBJ_VAL( weap, 0 ) ) ||
		 !ROOM_FLAGGED( ch->in_room, ROOM_NOSCIENCE ) ) &&
	       ( !SPELL_IS_PSIONIC( GET_OBJ_VAL( weap, 0 ) ) ||
		 !ROOM_FLAGGED( ch->in_room, ROOM_NOPSIONICS ) ) ) &&
	     !number( 0, MAX( 2, 
			      LVL_GRIMP + 28 - GET_LEVEL( ch ) - GET_INT( ch ) -
			      ( CHECK_SKILL( ch, GET_OBJ_VAL( weap, 0 ) ) >> 3 ) ) ) ) {
	    if ( weap->action_description )
		strcpy( buf, weap->action_description );
	    else
		sprintf( buf, "$p begins to hum and shake%s!",
			 weap->worn_on == WEAR_WIELD ||
			 weap->worn_on == WEAR_WIELD_2 ? " in your hand" : "" );
	    sprintf( buf2, "$p begins to hum and shake%s!",
		     weap->worn_on == WEAR_WIELD ||
		     weap->worn_on == WEAR_WIELD_2 ? " in $n's hand" : "" );
	    send_to_char( CCCYN( ch, C_NRM ), ch );
	    act( buf, FALSE, ch, weap, 0, TO_CHAR );
	    send_to_char( CCNRM( ch, C_NRM ), ch );
	    act( buf2, TRUE, ch, weap, 0, TO_ROOM );
	    if ( ( ( ( !IS_DWARF( ch ) && !IS_CYBORG( ch ) ) || 
		     !IS_OBJ_STAT( weap, ITEM_MAGIC ) ||
		     !SPELL_IS_MAGIC( GET_OBJ_VAL( weap, 0 ) ) ) &&
		   number( 0, GET_INT( ch ) + 3 ) ) || GET_LEVEL( ch ) > LVL_GRGOD ) {
		if ( IS_SET( spell_info[GET_OBJ_VAL( weap, 0 )].routines, MAG_DAMAGE )
		     || spell_info[GET_OBJ_VAL( weap, 0 )].violent )
		    call_magic( ch, victim, 0, GET_OBJ_VAL( weap, 0 ), GET_LEVEL( ch ), 
				CAST_WAND ); 
		else if ( !affected_by_spell( ch, GET_OBJ_VAL( weap, 0 ) ) ) 
		    call_magic( ch, ch, 0, GET_OBJ_VAL( weap, 0 ), GET_LEVEL( ch ), 
				CAST_WAND ); 
	    } else {
		// drop the weapon
		if ( ( weap->worn_on == WEAR_WIELD ||
		       weap->worn_on == WEAR_WIELD_2 ) &&
		     GET_EQ( ch, weap->worn_on ) == weap ) {
		    act( "$p shocks you!  You are forced to drop it!", 
			 FALSE, ch,weap, 0, TO_CHAR );

		    // weapon is the 1st of 2 wielded
		    if ( weap->worn_on == WEAR_WIELD && 
			 GET_EQ( ch, WEAR_WIELD_2 ) ) {
			weap2 = unequip_char( ch, WEAR_WIELD_2, MODE_EQ );
			obj_to_room( unequip_char( ch, weap->worn_on, MODE_EQ ), 
				     ch->in_room );
			if ( equip_char( ch, weap2, WEAR_WIELD, MODE_EQ ) )
			    return 1;
		    } 
		    // weapon should fall to ground
		    else if ( number( 0, 20 ) > GET_DEX( ch ) )
			obj_to_room( unequip_char( ch, weap->worn_on, MODE_EQ ), 
				     ch->in_room );
		    // weapon should drop to inventory
		    else 
			obj_to_char( unequip_char( ch, weap->worn_on, MODE_EQ ), ch );
	  
		} 
		// just shock the victim
		else {
		    act( "$p blasts the hell out of you!  OUCH!!",
			 FALSE,ch,weap,0,TO_CHAR );
		    GET_HIT( ch ) -= dice( 3, 4 );
		}
		act( "$n cries out as $p shocks $m!", FALSE,ch,weap,0,TO_ROOM );
	    }
	}
    }
    if ( !IS_NPC( ch ) && GET_MOVE( ch ) > 20 ) {
	GET_MOVE( ch )--;
	if ( IS_DROW( ch ) && OUTSIDE( ch ) && PRIME_MATERIAL_ROOM( ch->in_room ) &&
	     ch->in_room->zone->weather->sunlight == SUN_LIGHT )
	    GET_MOVE( ch )--;
	if ( IS_CYBORG( ch ) && GET_BROKE( ch ) )
	    GET_MOVE( ch )--;
    }

    return 0;
}

/* control the fights.  Called every 0.SEG_VIOLENCE sec from comm.c. */
void 
perform_violence( void )
{
    register struct char_data *ch;
    int prob, i, weap_weight,die_roll;
    void mobile_battle_activity( struct char_data *ch );

    for ( ch = combat_list; ch; ch = next_combat_list ) {
	next_combat_list = ch->next_fighting;

	if ( !ch->in_room || !FIGHTING( ch ) )
	    continue;

	if ( ch == FIGHTING( ch ) )  {       // intentional crash here.
	    slog( "SYSERR: ch == FIGHTING( ch ) in perform_violence." );
	    send_to_char( NULL, NULL );
	}

	if ( FIGHTING( ch ) == NULL || ch->in_room != FIGHTING( ch )->in_room ||
	     AFF2_FLAGGED( ch, AFF2_PETRIFIED ) ||
	     ( IS_NPC( ch ) && ch->in_room->zone->idle_time >= ZONE_IDLE_TIME ) ) {
	    stop_fighting( ch );
	    continue;
	}

	if ( IS_NPC( ch ) ) {
	    if ( GET_MOB_WAIT( ch ) > 0 ) {
		GET_MOB_WAIT( ch ) -= SEG_VIOLENCE;
	    } else if ( GET_MOB_WAIT( ch ) < SEG_VIOLENCE ) {
		GET_MOB_WAIT( ch ) = 0;
		if ( GET_POS( ch ) < POS_FIGHTING && GET_POS( ch ) > POS_STUNNED ) {
		    GET_POS( ch ) = POS_FIGHTING;
		    act( "$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM );
		    GET_MOB_WAIT( ch ) += PULSE_VIOLENCE;
		}
	    }
	    if ( GET_POS( ch ) <= POS_SITTING )
		continue;
	}

	prob = 1 + ( GET_LEVEL( ch ) / 7 ) + ( GET_DEX( ch ) << 1 );

	if ( IS_RANGER( ch ) && ( !GET_EQ( ch, WEAR_BODY ) || 
				  !OBJ_TYPE( GET_EQ( ch, WEAR_BODY ), ITEM_ARMOR ) ||
				  !IS_METAL_TYPE( GET_EQ( ch, WEAR_BODY ) ) ) )
	    prob -= ( GET_LEVEL( ch ) >> 2 );
      
	if ( GET_EQ( ch, WEAR_WIELD_2 ) ) {
	    if ( GET_OBJ_VNUM( GET_EQ( ch, WEAR_WIELD_2 ) ) > 0 ) {
		for ( i = 0; i < MAX_WEAPON_SPEC; i++ )
		    if ( GET_WEAP_SPEC( ch, i ).vnum == 
			 GET_OBJ_VNUM( GET_EQ( ch, WEAR_WIELD_2 ) ) ) {
			prob += GET_WEAP_SPEC( ch, i ).level << 2;
			break;
		    }
	    } 
	    for ( i = 0, weap_weight = 0; i < MAX_OBJ_AFFECT; i++ ) {
		if ( GET_EQ( ch, WEAR_WIELD_2 )->affected[i].location == 
		     APPLY_WEAPONSPEED ) {
		    weap_weight -= GET_EQ( ch, WEAR_WIELD_2 )->affected[i].modifier;
		    break;
		}
	    }
	    weap_weight += GET_EQ( ch, WEAR_WIELD_2 )->getWeight();
	    weap_weight = MAX( ( GET_EQ( ch, WEAR_WIELD_2 )->getWeight() >> 2 ), 
			       weap_weight );
      
	    prob -=  ( prob * weap_weight ) / 
		( str_app[STRENGTH_APPLY_INDEX( ch )].wield_w >> 1 );
      
	    prob += CHECK_SKILL( ch, SKILL_SECOND_WEAPON ) - 60;
      
	}
	if ( GET_EQ( ch, WEAR_WIELD ) ) {
	    if ( GET_OBJ_VNUM( GET_EQ( ch, WEAR_WIELD ) ) > 0 ) {
		for ( i = 0; i < MAX_WEAPON_SPEC; i++ )
		    if ( GET_WEAP_SPEC( ch, i ).vnum == 
			 GET_OBJ_VNUM( GET_EQ( ch, WEAR_WIELD ) ) ) {
			prob += GET_WEAP_SPEC( ch, i ).level << 2;
			break;
		    }
	    } 
	    if ( IS_MONK( ch ) )
		prob += ( LEARNED( ch ) - weapon_prof( ch, GET_EQ( ch, WEAR_WIELD ) ) ) >> 3;
      
	    for ( weap_weight = 0, i = 0; i < MAX_OBJ_AFFECT; i++ ) {
		if ( GET_EQ( ch, WEAR_WIELD )->affected[i].location == 
		     APPLY_WEAPONSPEED ) {
		    weap_weight = - GET_EQ( ch, WEAR_WIELD )->affected[i].modifier;
		    break;
		}
	    }
	    weap_weight += GET_EQ( ch, WEAR_WIELD )->getWeight();
	    weap_weight = MAX( (GET_EQ( ch, WEAR_WIELD )->getWeight() >> 1 ), weap_weight );
      
	    prob -=  ( prob * weap_weight ) / 
		( str_app[STRENGTH_APPLY_INDEX( ch )].wield_w << 1 );
	}
    
	prob += ( ( POS_FIGHTING - GET_POS( FIGHTING( ch ) ) ) << 1 );

	if ( CHECK_SKILL( ch, SKILL_DBL_ATTACK ) ) 
	    prob += ( int ) ( ( CHECK_SKILL( ch, SKILL_DBL_ATTACK ) * 0.15 ) +
			      ( CHECK_SKILL( ch, SKILL_TRIPLE_ATTACK ) * 0.17 ) );
	if ( CHECK_SKILL(ch, SKILL_MELEE_COMBAT_TAC) &&
		affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC))
		prob += (int) ( CHECK_SKILL(ch, SKILL_MELEE_COMBAT_TAC) * 0.10);
	if ( IS_AFFECTED( ch, AFF_ADRENALINE ) )
	    prob = ( int ) ( prob * 1.10 );
	if ( IS_AFFECTED_2( ch, AFF2_HASTE ) )
	    prob = ( int ) ( prob * 1.30 );
	if ( GET_CHAR_SPEED( ch ) )
	    prob += ( prob * GET_CHAR_SPEED( ch ) ) / 100;
	if ( IS_AFFECTED_2( ch, AFF2_SLOW ) )
	    prob = ( int ) ( prob * 0.70 );
	if ( IS_AFFECTED_2( ch, AFF2_BESERK ) )
	    prob += ( GET_LEVEL( ch ) + ( GET_REMORT_GEN( ch ) << 2 ) ) >> 1;
	if ( IS_MONK( ch ) )
	    prob += GET_LEVEL( ch ) >> 2;

	if ( ch->desc )
	    prob -= ( ( MAX( 0, ch->desc->wait >> 1 ) ) * prob ) / 100;
	else
	    prob -= ( ( MAX( 0, GET_MOB_WAIT( ch ) >> 1 ) ) * prob ) / 100;

	prob -=
	    ( ( ( ( IS_CARRYING_W( ch ) + IS_WEARING_W( ch ) ) << 5 ) * prob ) / 
	      ( CAN_CARRY_W( ch ) * 85 ) );
		  
	die_roll = number( 0, 300);

	if ( PRF2_FLAGGED( ch, PRF2_FIGHT_DEBUG ) ) {
	    sprintf( buf, "Attack speed: %d. Die roll: %d.\r\n", prob,die_roll );
	    send_to_char( buf, ch );
	}
	if ( PRF2_FLAGGED( FIGHTING( ch ), PRF2_FIGHT_DEBUG ) ) {
	    sprintf( buf, "Enemy Attack speed: %d. Enemy Die roll %d.\r\n", prob,die_roll );
	    send_to_char( buf, FIGHTING( ch ) );
	}
    
	if ( MIN( 100, prob+15 ) >= die_roll ) {
	    bool stop = false;

	    for ( i = 0; i < 4; i++ ) {
		if ( !FIGHTING( ch ) || GET_LEVEL( ch ) < ( i << 3 ) )
		    break;
		if ( GET_POS( ch ) < POS_FIGHTING ) {
		    if ( CHECK_WAIT( ch ) < 10 )
			send_to_char( "You can't fight while sitting!!\r\n", ch );
		    break;
		}
		if ( prob >= number( ( i << 4 ) + ( i << 3 ), ( i << 5 ) + ( i << 3 ) ) ) {
		    if ( hit( ch, FIGHTING( ch ), TYPE_UNDEFINED ) ) {
			stop = true;
			break;
		    }
		}
	    }

	    if ( stop )
		continue;

	    if ( IS_CYBORG( ch ) ) {
		int implant_prob;

		if ( !FIGHTING( ch ) )
		    continue;
		if ( GET_POS( ch ) < POS_FIGHTING ) {
		    if ( CHECK_WAIT( ch ) < 10 )
			send_to_char( "You can't fight while sitting!!\r\n", ch );
		    continue;
		}
		if ( number(1,100) < CHECK_SKILL(ch, SKILL_ADV_IMPLANT_W ) ) {
		    implant_prob = 25;
		    if (CHECK_SKILL(ch, SKILL_ADV_IMPLANT_W) > 100) {
			implant_prob += GET_REMORT_GEN(ch) + (CHECK_SKILL(ch, SKILL_ADV_IMPLANT_W) - 100)/2;
		    }
		    if(  number( 0 ,100 ) < implant_prob ) {
			if ( PRF2_FLAGGED( ch, PRF2_FIGHT_DEBUG ) ) 
			    send_to_char("Attempting advanced implant weapon attack.\r\n",ch);
			if ( hit( ch, FIGHTING(ch), SKILL_ADV_IMPLANT_W) )
			    continue;
		    }
		}
		if ( !FIGHTING( ch ) )
		    continue;
		if ( IS_NPC(ch) && ( GET_REMORT_CLASS(ch) == CLASS_UNDEFINED || GET_CLASS(ch) != CLASS_CYBORG ) )
			continue;
		if ( number(1,100) < CHECK_SKILL(ch, SKILL_IMPLANT_W ) ) {
		    implant_prob = 25;
		    if (CHECK_SKILL(ch, SKILL_IMPLANT_W) > 100) {
			implant_prob += GET_REMORT_GEN(ch) + (CHECK_SKILL(ch, SKILL_IMPLANT_W) - 100)/2;
		    }
		    if(  number( 0 ,100 ) < implant_prob ) {
			if ( PRF2_FLAGGED( ch, PRF2_FIGHT_DEBUG ) ) 
			    send_to_char("Attempting implant weapon attack.\r\n",ch);
			if ( hit( ch, FIGHTING(ch), SKILL_IMPLANT_W) )
			    continue;
		    } 
		}
	    }
	}

	else if ( IS_NPC( ch ) && ch->in_room && 
		  GET_POS( ch ) == POS_FIGHTING && 
		  GET_MOB_WAIT( ch ) <= 0 &&
		  ( MIN( 100, prob ) >= number( 0, 300 ) ) ) {
	    if ( MOB_FLAGGED( ch, MOB_SPEC ) && ch->in_room &&
		 ch->mob_specials.shared->func && !number( 0, 2 ) &&
		 ( ch->mob_specials.shared->func ) ( ch, ch, 0, "", 0 ) )
		continue;
	    if ( ch->in_room && GET_MOB_WAIT( ch ) <= 0 && FIGHTING( ch ) )
		mobile_battle_activity( ch ); 
	}
    }
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

    calc_thaco -= MIN( 5, MAX( 0, ( POS_FIGHTING - GET_POS( victim ) ) ) );

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
		  ( GET_EQ( ch, pos ) && OBJ_SOILED( GET_EQ( ch, pos ), type ) ) ) ) {
	    pos = number( 0, NUM_WEARS-2 );
	    count++;
	}
    }
    if ( count >= 100 ) /* didnt find pos.  rare, but possible */
	return 0;
	   
    if ( GET_EQ( ch, pos ) && GET_EQ( ch, pos ) != obj ) {
	if ( OBJ_SOILED( GET_EQ( ch, pos ), type ) )
	    return 0;

	SET_BIT( OBJ_SOILAGE( GET_EQ( ch, pos ) ), type );
    }
    else if ( ILLEGAL_SOILPOS( pos ) || CHAR_SOILED( ch, pos, type ) )
	return 0;
    else {
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
	 !PLR_FLAGGED( vict, PLR_KILLER ) && GET_LEVEL( ch ) < LVL_GRGOD ) {
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


#undef __fight_c__
