//
// File: killer_hunter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(killer_hunter)
{
	ACMD(do_gen_comm);
	char buf2[MAX_STRING_LENGTH];
	struct Creature *hunter = (struct Creature *)me;
	struct descriptor_data *d = NULL;

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || !hunter->in_room)
		return 0;

	if (hunter->isHunting()) {
		if (!PLR_FLAGGED(hunter->isHunting(), PLR_KILLER)) {
			sprintf(buf2, "You're damn lucky, %s.", GET_NAME(hunter->isHunting()));
			do_gen_comm(ch, buf2, 0, SCMD_HOLLER, 0);
			hunter->stopHunting();
			return 1;
		}
		return 0;
	}

	if (number(0, 4))
		return 0;

	for (d = descriptor_list; d; d = d->next) {
		if (d->input_mode == CXN_PLAYING && !d->original && d->creature) {
			if (PLR_FLAGGED(d->creature, PLR_KILLER) &&
				d->creature->in_room && can_see_creature(hunter, d->creature)) {
				if (GET_LEVEL(d->creature) < LVL_IMMORT &&
					(GET_LEVEL(hunter) <= (GET_LEVEL(d->creature) + 10)) &&
					find_first_step(hunter->in_room, d->creature->in_room,
						STD_TRACK) >= 0) {
					ch->startHunting(d->creature);
					do_gen_comm(ch, "Okay.  Now I'm pissed.", 0, SCMD_HOLLER, 0);
					return 1;
				}
			}
		}
	}
	return 0;
}
