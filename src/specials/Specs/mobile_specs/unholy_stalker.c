//
// File: unholy_stalker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(unholy_stalker)
{
	if (spec_mode != SPECIAL_TICK)
		return 0;

	struct creature *mob = (struct creature *) me;

	if (!MOB_HUNTING(mob) && !isFighting(mob)) {
		act("$n dematerializes, returning to the negative planes.", true, mob,
			0, 0, TO_ROOM);
		purge(mob, true);
		return 1;
	}

	if (isFighting(mob)) {
		if (!number(0, 3)) {
			call_magic(mob, findRandomCombat(mob), NULL, 0, SPELL_CHILL_TOUCH,
                       GET_LEVEL(mob) + 10, CAST_SPELL, NULL);
		}

        struct creature *vict = findRandomCombat(mob);
		if (GET_HIT(mob) < 100 && GET_HIT(vict) > GET_HIT(mob) &&
			!ROOM_FLAGGED(mob->in_room, ROOM_NOMAGIC | ROOM_NORECALL) &&
			GET_LEVEL(mob) > number(20, 35)) {
			call_magic(mob, mob, 0, NULL, SPELL_LOCAL_TELEPORT, 90, CAST_SPELL, NULL);
		}
	}

	return 0;
}
