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

	if (!HUNTING(mob) && !FIGHTING(mob)) {
		act("$n dematerializes, returning to the negative planes.", TRUE, mob,
			0, 0, TO_ROOM);
		mob->extract(true, false, CXN_MENU);
		return 1;
	}

	if (FIGHTING(mob)) {
		if (!number(0, 3)) {
			call_magic(mob, FIGHTING(mob), 0, SPELL_CHILL_TOUCH,
				GET_LEVEL(mob) + 10, CAST_SPELL);
		}

		if (GET_HIT(mob) < 100 && GET_HIT(FIGHTING(mob)) > GET_HIT(mob) &&
			!ROOM_FLAGGED(mob->in_room, ROOM_NOMAGIC | ROOM_NORECALL) &&
			GET_LEVEL(mob) > number(20, 35)) {
			call_magic(mob, mob, 0, SPELL_LOCAL_TELEPORT, 90, CAST_SPELL);
		}
	}

	return 0;
}
