SPECIAL( pit_keeper )
{
    char_data* vict = 0;

    if( spec_mode != SPECIAL_COMBAT && spec_mode != SPECIAL_TICK ) return 0;
    if ( cmd )
        return 0;

    if ( !( vict = FIGHTING( ch ) ) )
        return 0;

    if ( ch->in_room->number != 17027 &&
         ch->in_room->number != 17021 &&
         ch->in_room->number != 17022 &&
         ch->in_room->number != 17026 &&
         ch->in_room->number != 17020 )
        return 0;

    if ( !ch->in_room->dir_option[ DOWN ] || !ch->in_room->dir_option[ DOWN ]->to_room )
        return 0;

    
    if ( GET_STR( ch ) + number( 0, 10 ) > GET_STR( vict ) ) {
        act( "$n lifts you over his head and hurls you into the pit below!", FALSE, ch, 0, vict, TO_VICT );
        act( "$n lifts $N and hurls $M into the pit below!", FALSE, ch, 0, vict, TO_NOTVICT );
        WAIT_STATE( ch, 3 RL_SEC );
        stop_fighting( vict );
        stop_fighting( ch );
        char_from_room( vict,false );
        char_to_room( vict, ch->in_room->dir_option[ DOWN ]->to_room,false );
        act( "$n is hurled in from above!", FALSE, vict, 0, 0, TO_ROOM );
        look_at_room( vict, vict->in_room, 0 );
        WAIT_STATE( vict, 1 RL_SEC );
        return 1;
    }
    else {
        act( "$n attempts to grapple with you and fails!", FALSE,  ch, 0, vict, TO_VICT );
        WAIT_STATE( ch, 1 RL_SEC );
        return 1;
    }

    return 0;
}
