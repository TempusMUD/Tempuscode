//
// File: taunting_frenchman.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(taunting_frenchman)
{
	struct creature *vict = NULL;

	if (spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || !AWAKE(ch) || ch->fighting || number(0, 10))
		return (false);
    for (GList *it = ch->in_room->people;it;it = it->next) {
        vict = it->data;
		if (vict != ch && can_see_creature(ch, vict) &&
			GET_MOB_VNUM(ch) != GET_MOB_VNUM(vict) && !number(0, 3))
			break;
	}
	if (!vict || vict == ch)
		return 0;

	switch (number(0, 4)) {
	case 0:
		act("$n insults your mother!", true, ch, 0, vict, TO_VICT);
		act("$n just insulted $N's mother!", true, ch, 0, vict, TO_NOTVICT);
		break;
	case 1:
		act("$n smacks you with a rubber chicken!",
			true, ch, 0, vict, TO_VICT);
		act("$n hits $N in the head with a rubber chicken!",
			true, ch, 0, vict, TO_NOTVICT);
		break;
	case 2:
		act("$n spits in your face.", true, ch, 0, vict, TO_VICT);
		act("$n spits in $N's face.", true, ch, 0, vict, TO_NOTVICT);
		break;
	case 3:
		act("$n says, 'GO AWAY, I DONT WANT TO TALK TO YOU NO MORE!",
			true, ch, 0, 0, TO_ROOM);
		break;
	case 4:
		act("$n ***ANNIHILATES*** you with his ultra powerfull taunt!!",
			true, ch, 0, vict, TO_VICT);
		act("$n ***ANNIHILATES*** $N with his ultra powerfull taunt!!",
			true, ch, 0, vict, TO_NOTVICT);
		break;
	default:
		return false;
	}
	return true;
}
