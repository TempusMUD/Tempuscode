//
// File: mob_helper.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(mob_helper)
{
	struct Creature *vict = NULL;

	if (spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || FIGHTING(ch))
		return 0;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		vict = *it;
        // Being drawn into combat via a death cry will cause this
        // mob to attack a dead creature
        if( vict->getPosition() <= POS_DEAD ) {
            continue;
        }
		if (FIGHTING(vict) && IS_MOB(vict) && IS_MOB(FIGHTING(vict)))
			if (((IS_GOOD(ch) && IS_GOOD(vict)) || (IS_EVIL(ch)
						&& IS_EVIL(vict)))
				&& !number(0, 2)) {
				act("$n jumps to the aid of $N!", FALSE, ch, 0, vict,
					TO_NOTVICT);
				hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
				return 1;
			}
	}
	return 0;
}
