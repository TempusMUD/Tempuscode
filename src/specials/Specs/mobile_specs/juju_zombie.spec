//
// File: juju_zombie.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(juju_zombie)
{
	int prob;
	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (ch->getPosition() != POS_FIGHTING || !FIGHTING(ch))
		return 0;

	if (!number(0, 1)) {
		prob = MAX(GET_LEVEL(ch) - GET_LEVEL(FIGHTING(ch)), 10) +
			number(20,
			60) + (GET_AC(FIGHTING(ch)) / 10) - GET_DEX(FIGHTING(ch));
		if (prob < number(20, 70))
			return 0;

		act("$n reaches out and touches you!  You feel paralyzed!", FALSE, ch,
			0, FIGHTING(ch), TO_VICT);
		act("$n reaches out and paralyzes $N!", FALSE, ch, 0, FIGHTING(ch),
			TO_NOTVICT);
		WAIT_STATE(FIGHTING(ch), number(1, 3) * PULSE_VIOLENCE);
		return 1;
	}
	return 0;
}
