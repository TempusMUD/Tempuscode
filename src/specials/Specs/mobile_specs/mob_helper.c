//
// File: mob_helper.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(mob_helper)
{
    struct creature *helpee = NULL;
	struct creature *vict = NULL;

	if (spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || ch->isFighting())
		return 0;
	struct creatureList_iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		helpee = *it;
        // Being drawn into combat via a death cry will cause this
        // mob to attack a dead creature
        if (!helpee->isFighting())
            continue;

        vict = helpee->findRandomCombat();
		if (vict->getPosition() > POS_DEAD
				&& IS_MOB(helpee)
				&& IS_MOB(vict)
				&& ((IS_GOOD(ch) && IS_GOOD(helpee)) ||
						(IS_EVIL(ch) && IS_EVIL(helpee)))
				&& !number(0, 2)) {
			act("$n jumps to the aid of $N!", false, ch, 0, helpee,
				TO_NOTVICT);
			hit(ch, vict, TYPE_UNDEFINED);
			return 1;
		}
	}
	return 0;
}
