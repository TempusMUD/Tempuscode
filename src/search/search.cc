//
// File: search.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "fight.h"

/* extern variables */
extern struct room_data *world;
extern struct obj_data *object_list;

int search_nomessage = 0;

void print_search_data_to_buf( struct char_data *ch, 
			       struct room_data *room,
			       struct special_search_data *cur_search, 
			       char *buf );
int House_can_enter( struct char_data *ch, room_num real_room );
int clan_house_can_enter( struct char_data *ch, struct room_data *room );
void death_cry( struct char_data * ch );
	
#define SRCH_LOG( ch, srch ) \
{ if ( !ZONE_FLAGGED( ch->in_room->zone, ZONE_SEARCH_APPROVED ) && \
       GET_LEVEL( ch ) < LVL_GOD ) {                     \
							     sprintf( buf, "SRCH: %s at %d: %c %d %d %d.",       \
								      GET_NAME( ch ), ch->in_room->number,         \
								      *search_commands[( int )srch->command],      \
								      srch->arg[0], srch->arg[1], srch->arg[2] );slog( buf );}}
#define SRCH_DOOR ( targ_room->dir_option[srch->arg[1]]->exit_info )
#define SRCH_REV_DOOR ( other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info )

//
//
//

int
general_search( struct char_data *ch, struct special_search_data *srch,int mode )
{
    struct obj_data *obj = NULL;
    static struct char_data *mob = NULL, *next_mob = NULL;
    struct room_data *rm = ch->in_room, *other_rm = NULL;
    struct room_data *targ_room = NULL;
    int add = 0, killed = 0, found = 0;
    int bits = 0, i, j;
    int retval = 0;

    if ( !mode ) {
	obj = NULL;
	mob = NULL;
    }

    if ( SRCH_FLAGGED( srch, SRCH_NEWBIE_ONLY ) &&
	 GET_LEVEL( ch ) > 6 && !NOHASS( ch ) ) {
	send_to_char( "This can only be done here by players less than level 7.\r\n",ch );
	return 1;
    }
    switch ( srch->command ) {
    case ( SEARCH_COM_OBJECT ):
	if ( !( obj = real_object_proto( srch->arg[0] ) ) ) {
	    sprintf( buf, "SYSERR: search in room %d, object %d nonexistant.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	}
	if ( ( targ_room = real_room( srch->arg[1] ) ) == NULL ) {
	    sprintf( buf, "SYSERR: search in room %d, targ room %d nonexistant.",
		     rm->number, srch->arg[1] );
	    slog( buf );
	    return 0;
	}
	if ( obj->shared->number - obj->shared->house_count >= srch->arg[2] ) {
	    return 0;
	}
	if ( !( obj = read_object( srch->arg[0] ) ) ) {
	    sprintf( buf, "SYSERR: search cannot load object #%d, room %d.",
		     srch->arg[0], ch->in_room->number );
	    slog( buf );
	    return 0;
	}
	if ( ZONE_FLAGGED( ch->in_room->zone, ZONE_ZCMDS_APPROVED ) )
	    SET_BIT( GET_OBJ_EXTRA2( obj ), ITEM2_UNAPPROVED );
	obj_to_room( obj, targ_room );
	if ( srch->to_remote )
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );
    
	//    SRCH_LOG( ch, srch );

	break;

    case SEARCH_COM_MOBILE:
	if ( !( mob = real_mobile_proto( srch->arg[0] ) ) ) {
	    sprintf( buf, "SYSERR: search in room %d, mobile %d nonexistant.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	}
	if ( ( targ_room = real_room( srch->arg[1] ) ) == NULL ) {
	    sprintf( buf, "SYSERR: search in room %d, targ room %d nonexistant.",
		     rm->number, srch->arg[1] );
	    slog( buf );
	    return 0;
	}
	if ( mob->mob_specials.shared->number >= srch->arg[2] ) {
	    return 0;
	}
	if ( !( mob = read_mobile( srch->arg[0] ) ) ) {
	    sprintf( buf, "SYSERR: search cannot load mobile #%d, room %d.",
		     srch->arg[0], ch->in_room->number );
	    slog( buf );
	    return 0;
	}
	char_to_room( mob, targ_room );
	if ( srch->to_remote )
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );

	//    SRCH_LOG( ch, srch );
	break;

    case SEARCH_COM_EQUIP:
	if ( !( obj = real_object_proto( srch->arg[1] ) ) ) {
	    sprintf( buf, "SYSERR: search in room %d, equip object %d nonexistant.",
		     rm->number, srch->arg[1] );
	    slog( buf );
	    return 0; 
	}
	if ( srch->arg[2] < 0 || srch->arg[2] >= NUM_WEARS ) {
	    sprintf( buf, 
		     "SYSERR: search trying to equip obj %d to badpos.", 
		     obj->shared->vnum );
	    slog( buf );
	    return 0;
	}
	if ( !( obj = read_object( srch->arg[1] ) ) ) {
	    sprintf( buf, "SYSERR: search cannot load equip object #%d, room %d.",
		     srch->arg[0], ch->in_room->number );
	    slog( buf );
	    return 0;
	}
	if ( ZONE_FLAGGED( ch->in_room->zone, ZONE_ZCMDS_APPROVED ) )
	    SET_BIT( GET_OBJ_EXTRA2( obj ), ITEM2_UNAPPROVED );

//    SRCH_LOG( ch, srch );
	if ( ch->equipment[srch->arg[2]] )
	    obj_to_char( obj, ch );
	else if ( equip_char( ch, obj, srch->arg[2], MODE_EQ ) )
	    return 2;

	break;

    case SEARCH_COM_GIVE:
	if ( !( obj = real_object_proto( srch->arg[1] ) ) )
	    return 0;

	if ( obj->shared->number - obj->shared->house_count >= srch->arg[2] )
	    return 0;

	if ( !( obj = read_object( srch->arg[1] ) ) ) 
	    return 0; 

	if ( ZONE_FLAGGED( ch->in_room->zone, ZONE_ZCMDS_APPROVED ) )
	    SET_BIT( GET_OBJ_EXTRA2( obj ), ITEM2_UNAPPROVED );
	obj_to_char( obj, ch );

//    SRCH_LOG( ch, srch );

	break;

    case SEARCH_COM_TRANSPORT:
	if ( ( targ_room = real_room( srch->arg[0] ) ) == NULL ) {
	    sprintf( buf, "SYSERR: search in room %d, targ room %d nonexistant.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	}
    
	if ( !House_can_enter( ch, targ_room->number ) ||
	     !clan_house_can_enter( ch, targ_room ) ||
	     ( ROOM_FLAGGED( targ_room, ROOM_GODROOM ) &&
	       GET_LEVEL( ch ) < LVL_GRGOD ) )
	    return 0;

	if ( srch->to_room )
	    act( srch->to_room, FALSE, ch, obj, mob, TO_ROOM );
	if ( srch->to_vict )
	    act( srch->to_vict, FALSE, ch, obj, mob, TO_CHAR );
	else
	    send_to_char( "Okay.\r\n", ch );

	SRCH_LOG( ch, srch );     // don't log trans searches for now

	char_from_room( ch );
	char_to_room( ch, targ_room );
	look_at_room( ch, ch->in_room, 0 );

	if ( srch->to_remote )
	    act( srch->to_remote, FALSE, ch, obj, mob, TO_ROOM );

	if ( GET_LEVEL( ch ) < LVL_ETERNAL && !SRCH_FLAGGED( srch, SRCH_REPEATABLE ) )
	    SET_BIT( srch->flags, SRCH_TRIPPED );
    
	if ( IS_SET( ROOM_FLAGS( ch->in_room ),ROOM_DEATH ) ) {
	    if ( GET_LEVEL( ch ) < LVL_AMBASSADOR ) {
		log_death_trap( ch );
		death_cry( ch );
		extract_char( ch, 1 );
		return 2;
	    }
	    else {
		sprintf( buf, "( GC ) %s trans-searched goto into deathtrap %d.", 
			 GET_NAME( ch ), ch->in_room->number );
		mudlog( buf, NRM, LVL_GOD, TRUE );
	    }
	}
	// can't double trans, to prevent loops
	for ( srch = ch->in_room->search; srch; srch = srch->next ) {
	    if ( SRCH_FLAGGED( srch, SRCH_TRIG_ENTER ) && SRCH_OK( ch, srch ) &&
		 srch->command != SEARCH_COM_TRANSPORT )
		if ( general_search( ch, srch, 0 ) == 2 )
		    return 2;
	}
    
	return 1;
			 
    case SEARCH_COM_DOOR:
	/************  Targ Room nonexistant ************/
	if ( ( targ_room = real_room( srch->arg[0] ) ) == NULL ) {
	    sprintf( buf, "SYSERR: search in room %d, targ room %d nonexistant.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	}
	if ( srch->arg[1] >= NUM_DIRS || !targ_room->dir_option[srch->arg[1]] ) {
	    sprintf( buf, "SYSERR: search in room %d, direction nonexistant.",
		     rm->number );
	    slog( buf );
	    return 0;
	}
	switch ( srch->arg[2] ) {
	case 0:
	    add = 0;
	    bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
	    break;
	case 1:
	    add = 1;
	    bits = EX_CLOSED;
	    break;
	case 2:   /** close and lock door **/
	    add = 1;
	    bits = EX_CLOSED | EX_LOCKED;
	    break;
	case 3:   /** remove hidden flag **/
	    add = 0;
	    bits = EX_HIDDEN;
	    break;
        case 4:                 // close, lock, and hide the door
            add = 1;
            bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
            break;

	default:
	    sprintf( buf, "SYSERR: search bunk doorcmd %d in rm %d.",
		     srch->arg[2], rm->number );
	    slog( buf );
	    return 0;
	}
	if ( !( other_rm = targ_room->dir_option[srch->arg[1]]->to_room ) ||
	     !other_rm->dir_option[rev_dir[srch->arg[1]]] ||
	     other_rm->dir_option[rev_dir[srch->arg[1]]]->to_room!=targ_room )
	    other_rm = NULL;

	if ( add ) {
	    for ( found = 0, i = 0; i < 32; i++ ) {
		if ( IS_SET( bits, ( j = ( 1 << i ) ) ) && 
		     ( !IS_SET( SRCH_DOOR, j ) ||
		       ( other_rm && !IS_SET( SRCH_REV_DOOR, j ) ) ) ) {
		    found = 1;
		    break;
		}
	    }
	    if ( !found ) {
		if ( ! SRCH_FLAGGED( srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL ) )
		    send_to_char( "You can't do that right now.\r\n", ch );
		return 1;
	    }
	    SET_BIT( targ_room->dir_option[srch->arg[1]]->exit_info, bits );
	    if ( other_rm )
		SET_BIT( other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info, bits );
	} else {
	    for ( found = 0, i = 0; i < 32; i++ ) {
		if ( IS_SET( bits, ( j = ( 1 << i ) ) ) && 
		     ( IS_SET( SRCH_DOOR, j ) ||
		       ( other_rm && IS_SET( SRCH_REV_DOOR, j ) ) ) ) {
		    found = 1;
		    break;
		}
	    }
	    if ( !found ) {
		if ( ! SRCH_FLAGGED( srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL ) )
		    send_to_char( "You can't do that right now.\r\n", ch );
		return 1;
	    }

	    REMOVE_BIT( targ_room->dir_option[srch->arg[1]]->exit_info, bits );
	    if ( other_rm )
		REMOVE_BIT( other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info, bits );
	}

	//    SRCH_LOG( ch, srch );

	if ( srch->to_remote ) {
	    if ( targ_room != ch->in_room && targ_room->people ) {
		act( srch->to_remote, FALSE, targ_room->people, obj, ch, TO_NOTVICT );
	    }
	    if ( other_rm && other_rm != ch->in_room && other_rm->people ) {
		act( srch->to_remote, FALSE, other_rm->people, obj, ch, TO_NOTVICT );
	    }
	}
	break;

    case SEARCH_COM_SPELL:

	targ_room = real_room( srch->arg[1] );

	if ( srch->arg[0] < 0 || srch->arg[0] > LVL_GRIMP ) {
	    sprintf( buf, "SYSERR: search in room %d, spell level %d invalid.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	}      

	if ( srch->arg[2] <= 0 || srch->arg[2] > TOP_NPC_SPELL ) {
	    sprintf( buf, "SYSERR: search in room %d, spell number %d invalid.",
		     rm->number, srch->arg[2] );
	    slog( buf );
	    return 0;
	}

	if ( srch->to_room )
	    act( srch->to_room, FALSE, ch, obj, mob, TO_ROOM );
	if ( srch->to_vict )
	    act( srch->to_vict, FALSE, ch, obj, mob, TO_CHAR );
	else
	    send_to_char( "Okay.\r\n", ch );
    
	if ( GET_LEVEL( ch ) < LVL_ETERNAL && !SRCH_FLAGGED( srch, SRCH_REPEATABLE ) )
	    SET_BIT( srch->flags, SRCH_TRIPPED );

	// SRCH_LOG( ch, srch );

	// turn messaging off for damage(  ) call
	if ( SRCH_FLAGGED( srch, SRCH_NOMESSAGE ) )
	    search_nomessage = 1;

	if ( targ_room ) {

	    if ( srch->to_remote && ch->in_room != targ_room && targ_room->people ) {
            act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );
            act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_CHAR );
	    }

	    for ( mob = targ_room->people; mob; mob = next_mob ) {
            next_mob = mob->next_in_room;

            if ( affected_by_spell( ch, srch->arg[2] ) )
                continue;

            if ( mob == ch ) {
                call_magic( ch, ch, 0, srch->arg[2], srch->arg[0], 
                          ( SPELL_IS_MAGIC( srch->arg[2] ) ||
                        SPELL_IS_DIVINE( srch->arg[2] ) ) ? CAST_SPELL :
                          ( SPELL_IS_PHYSICS( srch->arg[2] ) ) ? CAST_PHYSIC :
                          ( SPELL_IS_PSIONIC( srch->arg[2] ) ) ? CAST_PSIONIC :
                          CAST_ROD , &retval);
                if ( IS_SET( retval, DAM_ATTACKER_KILLED ) ) 
                    break;
            } else {
                call_magic( ch, mob, 0, srch->arg[2], srch->arg[0], 
                    ( SPELL_IS_MAGIC( srch->arg[2] ) ||
                      SPELL_IS_DIVINE( srch->arg[2] ) ) ? CAST_SPELL :
                    ( SPELL_IS_PHYSICS( srch->arg[2] ) ) ? CAST_PHYSIC :
                    ( SPELL_IS_PSIONIC( srch->arg[2] ) ) ? CAST_PSIONIC :
                    CAST_ROD );
            }
	    }
	} else if ( !targ_room && !affected_by_spell( ch, srch->arg[2] ) )
	    call_magic( ch, ch, 0, srch->arg[2], srch->arg[0], 
			( SPELL_IS_MAGIC( srch->arg[2] ) ||
			  SPELL_IS_DIVINE( srch->arg[2] ) ) ? CAST_SPELL :
			( SPELL_IS_PHYSICS( srch->arg[2] ) ) ? CAST_PHYSIC :
			( SPELL_IS_PSIONIC( srch->arg[2] ) ) ? CAST_PSIONIC :
			CAST_ROD );
    
	search_nomessage = 0;

	return 2;
	/*
	  if ( !IS_SET( srch->flags, SRCH_IGNORE ) )
	  return 1;
	  else
	  return 0;
	*/
    
	break;

    case SEARCH_COM_DAMAGE:  /** damage a room or person */

	killed = 0;
	targ_room = real_room( srch->arg[1] );

	if ( srch->arg[0] < 0 || srch->arg[0] > 500 ) {
	    sprintf( buf, "SYSERR: search in room %d, damage level %d invalid.",
		     rm->number, srch->arg[0] );
	    slog( buf );
	    return 0;
	} 

	if ( srch->arg[2] <= 0 || srch->arg[2] >= TOP_NPC_SPELL ) {
	    sprintf( buf, "SYSERR: search in room %d, damage number %d invalid.",
		     rm->number, srch->arg[2] );
	    slog( buf );
	    return 0;
	}

	if ( srch->to_room )
	    act( srch->to_room, FALSE, ch, obj, mob, TO_ROOM );
	if ( srch->to_vict )
	    act( srch->to_vict, FALSE, ch, obj, mob, TO_CHAR );
	else
	    send_to_char( "Okay.\r\n", ch );
    
	if ( GET_LEVEL( ch ) < LVL_ETERNAL && !SRCH_FLAGGED( srch, SRCH_REPEATABLE ) )
	    SET_BIT( srch->flags, SRCH_TRIPPED );

	// SRCH_LOG( ch, srch );

	// turn messaging off for damage(  ) call
	if ( SRCH_FLAGGED( srch, SRCH_NOMESSAGE ) )
	    search_nomessage = 1;

	if ( targ_room ) {
      
	    if ( srch->to_remote && ch->in_room != targ_room && targ_room->people ) {
		act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );
		act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_CHAR );
	    }
      
	    for ( mob = targ_room->people; mob; mob = next_mob ) {
		next_mob = mob->next_in_room;
	
		if ( mob == ch )
		    killed = damage( NULL, mob, 
				     dice( srch->arg[0], srch->arg[0] ), srch->arg[2],
				     WEAR_RANDOM );
		else 
		    damage( NULL, mob, 
			    dice( srch->arg[0], srch->arg[0] ), srch->arg[2],
			    WEAR_RANDOM );
	
	    }
	} else 
	    killed =  damage( NULL, ch, 
			      dice( srch->arg[0], srch->arg[0] ), srch->arg[2],
			      WEAR_RANDOM );

	// turn messaging back on
	search_nomessage = 0;

	if ( killed )
	    return 2;
    
	if ( !IS_SET( srch->flags, SRCH_IGNORE ) )
	    return 1;
	else
	    return 0;
    
	break;

    case SEARCH_COM_SPAWN:  /* spawn a room full of mobs to another room */
	if ( !( other_rm = real_room( srch->arg[0] ) ) )
	    return 0;
	if ( !( targ_room = real_room( srch->arg[1] ) ) )
	    targ_room = ch->in_room;

	if ( srch->to_room )
	    act( srch->to_room, FALSE, ch, obj, mob, TO_ROOM );
	if ( srch->to_remote && targ_room->people ) {
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_CHAR );
	}
	if ( srch->to_vict )
	    act( srch->to_vict, FALSE, ch, obj, mob, TO_CHAR );
	else if ( !SRCH_FLAGGED( srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL ) )
	    send_to_char( "Okay.\r\n", ch );
    
	if ( GET_LEVEL( ch ) < LVL_ETERNAL && !SRCH_FLAGGED( srch, SRCH_REPEATABLE ) )
	    SET_BIT( srch->flags, SRCH_TRIPPED );


	for ( mob = other_rm->people; mob; mob = next_mob ) {
	    next_mob = mob->next_in_room;
	    if ( !IS_NPC( mob ) )
		continue;
	    if ( other_rm != targ_room ) {
		act( "$n suddenly disappears.", TRUE, mob, 0, 0, TO_ROOM );
		char_from_room( mob );
		char_to_room( mob, targ_room );
		act( "$n suddenly appears.", TRUE, mob, 0, 0, TO_ROOM );
	    }
	    if ( srch->arg[2] )
		HUNTING( mob ) = ch;
	}

	// SRCH_LOG( ch, srch );
    
	if ( !IS_SET( srch->flags, SRCH_IGNORE ) )
	    return 1;
	else
	    return 0;
	break;

    case SEARCH_COM_NONE: /* simple echo search */

	if ( ( targ_room = real_room( srch->arg[1] ) ) &&
	     srch->to_remote && ch->in_room != targ_room && targ_room->people ) {
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_ROOM );
	    act( srch->to_remote, FALSE, targ_room->people, obj, mob, TO_CHAR );
	}
    
	break;

    default:
	sprintf( buf, "Unknown rsearch command in do_search(  )..room %d.", 
		 rm->number );
	slog( buf );

    }

    if ( srch->to_room )
	act( srch->to_room, FALSE, ch, obj, mob, TO_ROOM );
    if ( srch->to_vict )
	act( srch->to_vict, FALSE, ch, obj, mob, TO_CHAR );
    else
	send_to_char( "Okay.\r\n", ch );

    if ( GET_LEVEL( ch ) < LVL_ETERNAL && !SRCH_FLAGGED( srch, SRCH_REPEATABLE ) )
	SET_BIT( srch->flags, SRCH_TRIPPED );

    if ( !IS_SET( srch->flags, SRCH_IGNORE ) )
	return 1;
    else
	return 0;
}

void
show_searches( struct char_data *ch, char *value, char *argument )
{

    struct special_search_data *srch = NULL;
    struct room_data *room = NULL;
    struct zone_data *zone = NULL, *zn = NULL;
    int count = 0, srch_type = -1;
    char outbuf[MAX_STRING_LENGTH], arg1[MAX_INPUT_LENGTH];
    bool overflow = 0, found = 0;

    strcpy( argument, strcat( value, argument ) );
    argument = one_argument( argument, arg1 );
    while ( ( *arg1 ) ) {
	if ( is_abbrev( arg1, "zone" ) ) {
	    argument = one_argument( argument, arg1 );
	    if ( !*arg1 ) {
		send_to_char( "No zone specified.\r\n", ch );
		return;
	    }
	    if ( !( zn = real_zone( atoi( arg1 ) ) ) ) {
		sprintf( buf, "No such zone ( %s ).\r\n", arg1 );
		send_to_char( buf, ch );
		return;
	    }
	}
	if ( is_abbrev( arg1, "type" ) ) {
	    argument = one_argument( argument, arg1 );
	    if ( !*arg1 ) {
		send_to_char( "No type specified.\r\n", ch );
		return;
	    }
	    if ( ( srch_type = search_block( arg1, search_commands, FALSE ) ) < 0 ) {
		sprintf( buf, "No such search type ( %s ).\r\n", arg1 );
		send_to_char( buf, ch );
		return;
	    }
	}
	argument = one_argument( argument, arg1 );
    }
    
    strcpy( outbuf, "Searches:\r\n" );

    for ( zone = zone_table; zone && !overflow; zone = zone->next ) {

	if ( zn ) {
	    if ( zone == zn->next )
		break;
	    if ( zn != zone )
		continue;
	}

	for ( room = zone->world, found = FALSE; room && !overflow; 
	      found = FALSE, room = room->next ) {

	    for ( srch = room->search, count = 0; srch && !overflow; 
		  srch = srch->next, count++ ) {

		if ( srch_type >= 0 && srch_type != srch->command )
		    continue;

		if ( !found ) {
		    sprintf( buf, "Room [%s%5d%s]:\n", CCCYN( ch, C_NRM ),
			     room->number, CCNRM( ch, C_NRM ) );
		    strcat( outbuf, buf );
		    found = TRUE;
		}

		print_search_data_to_buf( ch, room, srch, buf );

		if ( strlen( outbuf ) + strlen( buf ) > MAX_STRING_LENGTH - 128 ) {
		    overflow = 1;
		    strcat( outbuf, "**OVERFLOW**\r\n" );
		    break;
		}
		strcat( outbuf, buf );
	    }
	}
    }
    page_string( ch->desc, outbuf, 1 );
}

int 
triggers_search( struct char_data *ch, int cmd, char *arg, 
		 struct special_search_data *srch )
{
  
    char arg1[MAX_STRING_LENGTH];
    char *buf = NULL;

    skip_spaces( &arg );

    if ( srch->keywords ) {
	if ( SRCH_FLAGGED( srch, SRCH_CLANPASSWD | SRCH_NOABBREV ) ) {
	    if ( !isname_exact( arg, srch->keywords ) ) {
		return 0;
	    }
	} 
	else if ( !isname( arg, srch->keywords ) ) {
	    return 0;
	}
    }

    if ( IS_SET( srch->flags, SRCH_TRIPPED ) )
	return 0;

    if ( GET_LEVEL( ch ) < LVL_IMMORT && !SRCH_OK( ch, srch ) )
	return 0;

    if ( !srch->command_keys )
	return 1;
    buf = str_dup( srch->command_keys );
    buf = one_argument( buf, arg1 );

    while ( *arg1 ) {
	if ( CMD_IS( arg1 ) )
	    return 1;
	buf = one_argument( buf, arg1 );
    }

    return 0;
}

#undef __search_c__  
