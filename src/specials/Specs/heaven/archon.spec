//
// File: archon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(archon)
{

	struct room_data *room = real_room(43252);

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd)
		return 0;
	if (!FIGHTING(ch) && ch->in_room->zone->plane != PLANE_HEAVEN) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it)
			if ((*it) != ch && IS_ARCHON((*it)) && FIGHTING((*it))) {
				do_rescue(ch, fname((*it)->player.name), 0, 0, 0);
				return 1;
			}

		act("$n disappears in a flash of light.", FALSE, ch, 0, 0, TO_ROOM);
		if (room) {
			char_from_room(ch, false);
			char_to_room(ch, room, false);
			act("$n appears at the center of the room.", FALSE, ch, 0, 0,
				TO_ROOM);
		} else {
			ch->purge(true);
		}
		return 1;
	}
	return 0;
}
