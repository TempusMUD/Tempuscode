//
// File: basher.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(basher)
{
	struct Creature *vict = NULL;
	ACMD(do_bash);
	if (spec_mode != SPECIAL_COMBAT)
		return 0;
	if (cmd || ch->getPosition() != POS_FIGHTING || !FIGHTING(ch))
		return 0;

	if (number(0, 81) > GET_LEVEL(ch))
		return 0;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (FIGHTING((*it)) == ch && !number(0, 2) && IS_MAGE((*it))) {
			vict = *it;
			break;
		}
	}

	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
		vict = FIGHTING(ch);

	do_bash(ch, "", 0, 0, 0);
	return 1;
}
