//
// File: spirit_priestess.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL( spirit_priestess )
{
    struct char_data *pri = ( struct char_data * ) me;
    struct obj_data *am = NULL, *staff = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if ( !CMD_IS( "give" ) && !CMD_IS( "plant" ) )
	return 0;

    for ( am = ch->carrying; am; am = am->next_content ) {
	if ( GET_OBJ_VNUM( am ) == 34307 )
	    break;
    }

    if (  !am  )
	return 0;

    two_arguments( argument, arg1, arg2 );

    if ( !*arg2 || !*arg1 )
	return 0;

    if ( !isname( arg1, am->name ) || !isname( arg2, pri->player.name ) )
	return 0;

    if ( !( staff = read_object( 34306 ) ) )
	return 0;

    act( "$n presents $N with $p.", TRUE, ch, am, pri, TO_ROOM );
    act( "You present $N with $p.", FALSE, ch, am, pri, TO_CHAR );

    do_say( pri, "Thank you, you have done well.  I now give you this healing staff.", 0, 0 );

    act( "$N gives $p to $n.", TRUE, ch, staff, pri, TO_ROOM );
    act( "$N gives $p to you.", FALSE, ch, staff, pri, TO_CHAR );

    extract_obj( am );
    obj_to_char( staff, ch );

    return 1;
}
  
  
    
