//
// File: basher.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(basher)
{
	struct Creature *vict = NULL;
	ACMD(do_bash);
	if (spec_mode != SPECIAL_TICK)
		return false;
	if (ch->getPosition() != POS_FIGHTING || !ch->numCombatants())
		return 0;

	if (number(0, 81) > GET_LEVEL(ch))
		return 0;

    list<CharCombat>::iterator li = ch->getCombatList()->begin();
    for (; li != ch->getCombatList()->end(); ++li) {
        if (IS_MAGE(li->getOpponent()) && number(0, 1)) {
            vict = li->getOpponent();
            break;
        }
    }
	if (vict == NULL)
		vict = ch->findRandomCombat();

	do_bash(ch, "", 0, 0, 0);
	return 1;
}
