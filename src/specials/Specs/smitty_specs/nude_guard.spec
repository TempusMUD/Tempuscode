//
// File: nude_guard.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(nude_guard)
{
	struct Creature *guard = (struct Creature *)me;
	int i;
	int found = 0;

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (!CMD_IS("west") || GET_LEVEL(ch) >= LVL_IMMORT)
		return 0;

	for (i = 0; i < NUM_WEARS && !found; i++)
		if (GET_EQ(ch, i))
			found = 1;

	if (found || ch->carrying) {
		perform_tell(guard, ch, "You can't take a bath with your clothes on!");
		act("$n blocks $N.", FALSE, guard, 0, ch, TO_NOTVICT);
		return 1;
	}
	return 0;
}
