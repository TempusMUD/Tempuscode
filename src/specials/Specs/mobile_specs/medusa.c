//
// File: medusa.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(medusa)
{
	if (spec_mode != SPECIAL_TICK)
		return 0;
	if (ch->getPosition() != POS_FIGHTING)
		return false;

    struct creature *vict = ch->findRandomCombat();
	if (isname("medusa", ch->player.name) &&
		vict && (vict->in_room == ch->in_room) &&
		(number(0, 57 - GET_LEVEL(ch)) == 0)) {
		act("The snakes on $n bite $N!", 1, ch, 0, vict, TO_NOTVICT);
		act("You are bitten by the snakes on $n!", 1, ch, 0, vict,
			TO_VICT);
		call_magic(ch, vict, 0, NULL, SPELL_POISON, GET_LEVEL(ch),
			CAST_SPELL);

		return true;
	} else if (vict && !number(0, 4)) {
		act("$n gazes into your eyes!", false, ch, 0, vict, TO_VICT);
		act("$n gazes into $N's eyes!", false, ch, 0, vict,
			TO_NOTVICT);
		call_magic(ch, vict, 0, NULL, SPELL_PETRIFY, GET_LEVEL(ch),
			CAST_PETRI);
		if (AFF2_FLAGGED(vict, AFF2_PETRIFIED))
            ch->removeAllCombat();
		return 1;
	}
	return false;
}
