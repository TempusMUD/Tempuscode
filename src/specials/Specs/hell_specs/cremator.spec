SPECIAL( cremator )
{
    if ( cmd || !FIGHTING( ch ) )
	return 0;

    switch ( number( 0, 5 ) ) {
    case 0:
	act( "$n attempts to throw you into the furnace!", FALSE, ch, 0, FIGHTING( ch ), TO_VICT );
	break;
    case 1:
	act( "$n lifts you and almosts hurls you into the furnace!", FALSE, ch, 0, FIGHTING( ch ), TO_VICT );
	break;
    case 2:
	act( "$n shoves you toward the blazing furnace!", FALSE, ch, 0, FIGHTING( ch ), TO_VICT );
	break;
    default:
	return 0;
    }

    return 1;
}
