//
// File: unspecializer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define _u_mode_buy_   1
#define _u_mode_offer_ 2

SPECIAL(unspecializer)
{
    struct char_data *theDr = ( struct char_data * ) me;
    int mode = 0;
    struct obj_data *o_proto = NULL;
    char arg[ MAX_INPUT_LENGTH ];
    int cash_cost = 0;
    int prac_cost = 0;
    int i;

    if ( CMD_IS( "buy" ) )
	mode = _u_mode_buy_;
    else if ( CMD_IS( "offer" ) )
	mode = _u_mode_offer_;
    else
	return 0;

    argument = one_argument( argument, arg );

    skip_spaces( & argument );

    if ( ! *arg || strcasecmp( arg, "unspecialization" ) ) {
	sprintf( buf, "You must type '%s unspecialization <weapon name>'\r\n", _u_mode_buy_ ? "buy" : "offer" );
	send_to_char( buf, ch );
	return 1;
    }

    if ( ! *argument ) {
	send_to_char( "Which weapon specialization?\r\n", ch );
	return 1;
    }

    for ( i = 0; i < MAX_WEAPON_SPEC; i++ ) {
	if ( ! GET_WEAP_SPEC( ch, i ).vnum ||
	     ! GET_WEAP_SPEC( ch, i ).level ||
	     ! ( o_proto = real_object_proto( GET_WEAP_SPEC( ch, i ).vnum ) ) )
	    continue;
	if ( isname( argument, o_proto->name ) )
	    break;
    }
    
    if ( i >= MAX_WEAPON_SPEC ) {
	send_to_char( "You are not specialized in that weapon.\r\n", ch );
	return 1;
    }

    prac_cost = GET_WEAP_SPEC( ch, i ).level;
    cash_cost = GET_WEAP_SPEC( ch, i ).level * 200000;

    sprintf( buf,
	     "The service of complete neural erasure of the weapon specialization of\r\n"
	     "'%s' will cost you %d pracs and %d credits....\r\n", 
	     o_proto->short_description, prac_cost, cash_cost );
    send_to_char( buf, ch );

    if ( GET_PRACTICES( ch ) < prac_cost ) {
	send_to_char( "You do not have enough practice points to endure the procedure.\r\n", ch );
	return 1;
    }

    if ( GET_CASH( ch ) < cash_cost ) {
	send_to_char( "You do not have enough cash on hand.\r\n", ch );
	return 1;
    }

    if ( mode == _u_mode_offer_ )
	return 1;
    
    GET_PRACTICES( ch ) -= prac_cost;
    GET_CASH( ch )      -= cash_cost;

    GET_WEAP_SPEC( ch, i ).level =
	GET_WEAP_SPEC( ch, i ).vnum = 0;

    act(
	"\r\n$n takes your money and straps you into the chair.\r\n"
	"He attaches a cold, vaseline-coated probe to each of your temples and walks\r\n"
	"to one corner of the room.\r\n"
	"You see him pour some ether from a small glass vial onto a small\r\n"
	"white rag, which he then proceeds to press to his nose and inhale deeply.\r\n"
	"He stomps slowly back across the room on rubbery, wavering legs, and clamps\r\n"
	"a heavy metal helmet onto your head.  He then produces a large set of jumper\r\n"
	"cables.  He clips one end to your nipples, and the other end to his.  He then\r\n"
	"opens the door of a large, vented, metal case with a bang.  Inside you see\r\n"
	"a shiny metal rod, coated with conductive grease.  He grasps this rod firmly\r\n"
	"with his left hand.  He lays his right hand on a large double bladed knife\r\n"
	"switch on the side of your chair.\r\n\r\n"
	"He grins at you and says, 'This won't hurt a bit.\r\n\r\n"
	"You black out...\r\n\r\n", FALSE, theDr, 0, ch, TO_VICT );

    act(
	"$n straps $N into the chair at the center of the room.\r\n"
	"$n proceeds to inhale heavily on an ether-laden rag, and then\r\n"
	"connects a series of cables to $N.\r\n"
	"$n then throws a large knife switch, apparently sending a strong\r\n"
	"current of electricity through $N's body.\r\n"
	"$N twitches and writhes for a while, and finally, seemingly\r\n"
	"prompted by some information on a glowing monitor,\r\n"
	"$n turns the switch off and walks away.\r\n\r\n"
	"$N lies on the chair motionless...\r\n\r\n", FALSE, theDr, 0, ch, TO_NOTVICT );

    WAIT_STATE( ch, 10 RL_SEC );

    return 1;
}	     
	     
#undef _u_mode_buy_
#undef _u_mode_offer_
