//
// File: aziz_canon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(aziz_canon)
{
	struct creature *vict = NULL;

	if (cmd)
		return 0;
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (!ch->fighting && GET_POSITION(ch) != POS_FIGHTING) {
		for (GList *cit = ch->in_room->people;cit;cit = cit->next) {
            vict = cit->data;
			if (!number(0, 2))
				break;
		}
		if (!vict)
			return 0;

		switch (number(0, 30)) {
		case 1:
			act("$n looks at you and winks.'.", false, ch, 0, vict, TO_VICT);
			act("$n looks at $N and winks.'.", false, ch, 0, vict, TO_NOTVICT);
			break;
		case 2:
			act("$n struts around you like a horny rooster.", true, ch, 0,
				vict, TO_VICT);
			act("$n struts around $N like a horny rooster.", true, ch, 0, vict,
				TO_NOTVICT);
			break;
		case 3:
			act("$n farts loudly and smiles sheepishly.", false, ch, 0, 0,
				TO_ROOM);
			break;
		case 4:
			act("$n pounds his shoulder against the wall.", true, ch, 0, 0,
				TO_ROOM);
			break;
		case 5:
			act("$n eats a small piece of paper.", true, ch, 0, 0, TO_ROOM);
			break;
		case 6:
			act("$n rolls a joint and licks it real good.", true, ch, 0, 0,
				TO_ROOM);
			break;
		case 7:
			act("$n guzzles a beer and smashes the can against $s forehead.",
				true, ch, 0, 0, TO_ROOM);
			break;
		}
		return 1;
	}
	if (ch->fighting) {
        vict = random_opponent(ch);
		if (!number(0, 3)) {
			act("$n gets in a three point stance and plows his shoulder into you!", false, ch, 0, vict, TO_VICT);
			act("$n gets in a three point stance and plows his shoulder into $N!", false, ch, 0, vict, TO_NOTVICT);
			if (GET_LEVEL(vict) < LVL_IMMORT) {
				GET_HIT(vict) -= dice(1, 20);
				GET_POSITION(vict) = POS_SITTING;
				WAIT_STATE(ch, PULSE_VIOLENCE * 4);
				WAIT_STATE(vict, PULSE_VIOLENCE * 2);
			}
			return 1;
		}
	}
	return 0;
}
