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

	if (HUNTING(hunter)) {
		if (!PLR_FLAGGED(HUNTING(hunter), PLR_KILLER)) {
			sprintf(buf2, "You're damn lucky, %s.", GET_NAME(HUNTING(hunter)));
			do_gen_comm(ch, buf2, 0, SCMD_HOLLER);
			HUNTING(hunter) = 0;
			return 1;
		}
		return 0;
	}

	if (number(0, 4))
		return 0;

	for (d = descriptor_list; d; d = d->next) {
		if (!d->connected && !d->original && d->character) {
			if (PLR_FLAGGED(d->character, PLR_KILLER) &&
				d->character->in_room && CAN_SEE(hunter, d->character)) {
				if (GET_LEVEL(d->character) < LVL_IMMORT &&
					(GET_LEVEL(hunter) <= (GET_LEVEL(d->character) + 10)) &&
					find_first_step(hunter->in_room, d->character->in_room,
						STD_TRACK) >= 0) {
					HUNTING(ch) = d->character;
					do_gen_comm(ch, "Okay.  Now I'm pissed.", 0, SCMD_HOLLER);
					return 1;
				}
			}
		}
	}
	return 0;
}
