//
// File: hobbes.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cheeky_monkey)
{
	struct Creature *vict = NULL;

	if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_ENTER) {
		return 0;
	}

	if (cmd || !AWAKE(ch) || FIGHTING(ch) || number(0, 3))
		return FALSE;

	CreatureList::iterator it = ch->in_room->people.begin();
	CreatureList::iterator nit = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		++nit;
		vict = *it;
		if (vict != ch && CAN_SEE(ch, vict) &&
			(!number(0, 5) || nit == ch->in_room->people.end()))
			break;
	}
	if (!vict || vict == ch)
		return 0;

	switch (number(0, 10)) {
	case 0:
		act("$n grabs your ass!", TRUE, ch, 0, vict, TO_VICT);
		act("$n reaches around and grabs $N's ass!", TRUE, ch, 0, vict,
			TO_NOTVICT);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}


SPECIAL(hobbes_speaker)
{
	if (cmd)
		return 0;

	switch (number(0, 10)) {

	case 0:
		do_say(ch, "THIS IS THE TEXT HIS MOB WILL SAY.", 0, 0, 0);
		return TRUE;
	case 1:
		do_say(ch, "THIS IS MORE TEXT HIS MOB WILL SAY.", 0, 0, 0);
		return TRUE;
	default:
		return FALSE;
	}
}
