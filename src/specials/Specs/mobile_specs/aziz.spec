//
// File: aziz.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_bash);

SPECIAL(Aziz)
{
	struct Creature *vict = NULL;

	if (spec_mode != SPECIAL_TICK)
		return 0;
	if (!ch->numCombatants())
		return 0;

	/* pseudo-randomly choose a mage in the room who is fighting me */
    CombatDataList::iterator li = ch->getCombatList()->begin();
    for (; li != ch->getCombatList()->end(); ++li) {
        if (IS_MAGE(li->getOpponent()) && number(0, 1)) {
            vict = li->getOpponent();
            break;
        }
    }


	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
		vict = ch->findRandomCombat();

	do_bash(ch, "", 0, 0, 0);

	return 0;
}
