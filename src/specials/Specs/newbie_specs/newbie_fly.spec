//
// File: newbie_fly.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_fly)
{
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || FIGHTING(ch))
		return 0;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (IS_AFFECTED((*it), AFF_INFLIGHT) || !can_see_creature(ch, (*it)))
			continue;
		cast_spell(ch, (*it), 0, SPELL_FLY);
		return 1;
	}
	return 0;
}
