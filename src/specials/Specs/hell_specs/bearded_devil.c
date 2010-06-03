//
// File: bearded_devil.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(bearded_devil)
{
	if (spec_mode != SPECIAL_TICK)
		return false;
	if (!ch->isFighting() || !AWAKE(ch) || GET_MOB_WAIT(ch) > 0)
		return false;

	if (!number(0, 3)) {
        struct creature *vict = ch->findRandomCombat();
		act("$n thrusts $s wirelike beard at you!",
			false, ch, 0, vict, TO_VICT);
		act("$n thrusts $s wirelike beard at $N!",
			false, ch, 0, vict, TO_NOTVICT);
		if (GET_DEX(vict) > number(0, 25))
			damage(ch, vict, 0, TYPE_RIP, WEAR_FACE);
		else
			damage(ch, vict, dice(8, 8), TYPE_RIP, WEAR_FACE);
		WAIT_STATE(ch, 2 RL_SEC);
		return 1;
	}
	return 0;
}
