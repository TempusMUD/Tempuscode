//
// File: script_reader.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_RANDOM      ( 1 << 0 )
#define MODE_ALONE       ( 1 << 1 )

#define TOP_MESSAGE    ( GET_OBJ_VAL( obj, 0 ) )
#define MESSAGE_MODE   ( GET_OBJ_VAL( obj, 1 ) )
#define WAIT_TIME      ( GET_OBJ_VAL( obj, 2 ) )
#define SCRIPT_COUNTER ( GET_OBJ_VAL( obj, 3 ) )
#define CUR_WAIT       ( GET_OBJ_DAM( obj ) )
#define LAST_SCRIPT_ACTION       ( GET_OBJ_MAX_DAM( obj ) )
#define SCRIPT_FLAGGED( flag )  ( IS_SET( MESSAGE_MODE, flag ) )

SPECIAL(mob_read_script)
{
    struct obj_data *obj = GET_IMPLANT( ch, WEAR_HOLD );
    char *desc = NULL, *c, buf[EXDSCR_LENGTH];
    int which = 0;
    int found = 0;
    if ( !SCRIPT_FLAGGED( MODE_ALONE ) ) {
        CharacterList::iterator it = ch->in_room->people.begin();
        for( ; it != ch->in_room->people.end(); ++it ) {
            if ( (*it)->desc && CAN_SEE( ch, (*it) ) ) {
                found = 1;
                break;
            }
        }
    
        if ( !found ) {
            SCRIPT_COUNTER = 0;
            CUR_WAIT = 0;
            return 0;
        }
    }

    if ( CUR_WAIT > 0 ) {
        CUR_WAIT--;
        return 0;
    }
    
    if ( TOP_MESSAGE < 0 )
        return 0;

    if ( SCRIPT_FLAGGED( MODE_RANDOM ) ) {

        which = number( 0, TOP_MESSAGE );
        sprintf( buf, "%d", which );

        CUR_WAIT = WAIT_TIME;

        if ( which == LAST_SCRIPT_ACTION )
            return 0;

        if ( ( desc = find_exdesc( buf, obj->ex_description, 1 ) ) ) {
            strcpy( buf, desc );
            if ( ( c = strrchr( buf, '\n' ) ) )
                *c = '\0';
            if ( ( c = strrchr( buf, '\r' ) ) )
                *c = '\0';
            command_interpreter( ch, buf );
            LAST_SCRIPT_ACTION = which;
            return 1;
        }
        return 0;
    }

    /* ordered mode */
  
    if ( SCRIPT_COUNTER > TOP_MESSAGE ) {

        SCRIPT_COUNTER = 0;
        CUR_WAIT = WAIT_TIME << 1;

        return 0;
    }

    sprintf( buf, "%d", SCRIPT_COUNTER );

    CUR_WAIT = WAIT_TIME;
    SCRIPT_COUNTER++;

    if ( ( desc = find_exdesc( buf, obj->ex_description,1 ) ) ) {
        strcpy( buf, desc );
        if ( ( c = strrchr( buf, '\n' ) ) )
            *c = '\0';
        if ( ( c = strrchr( buf, '\r' ) ) )
            *c = '\0';
    
        command_interpreter( ch, buf );
        return 1;
    } 
  
    return 0;
}
    
    
  
  
