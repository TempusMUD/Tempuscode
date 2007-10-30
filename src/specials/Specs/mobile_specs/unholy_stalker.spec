//
// File: unholy_stalker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(unholy_stalker)
{
	if (spec_mode != SPECIAL_TICK)
		return 0;

	Creature *mob = (Creature *) me;

	if (!mob->isHunting() && !mob->isFighting()) {
		act("$n dematerializes, returning to the negative planes.", TRUE, mob,
			0, 0, TO_ROOM);
		mob->purge(true);
		return 1;
	}

	if (mob->isFighting()) {
		if (!number(0, 3)) {
			call_magic(mob, mob->findRandomCombat(), NULL, 0, SPELL_CHILL_TOUCH,
				GET_LEVEL(mob) + 10, CAST_SPELL);
		}

        Creature *vict = mob->findRandomCombat();
		if (GET_HIT(mob) < 100 && GET_HIT(vict) > GET_HIT(mob) &&
			!ROOM_FLAGGED(mob->in_room, ROOM_NOMAGIC | ROOM_NORECALL) &&
			GET_LEVEL(mob) > number(20, 35)) {
			call_magic(mob, mob, 0, NULL, SPELL_LOCAL_TELEPORT, 90, CAST_SPELL);
		}
	}

	return 0;
}
