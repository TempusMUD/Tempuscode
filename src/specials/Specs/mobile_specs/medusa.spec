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
		return FALSE;

    Creature *vict = ch->findRandomCombat();
	if (isname("medusa", ch->player.name) &&
		vict && (vict->in_room == ch->in_room) &&
		(number(0, 57 - GET_LEVEL(ch)) == 0)) {
		act("The snakes on $n bite $N!", 1, ch, 0, vict, TO_NOTVICT);
		act("You are bitten by the snakes on $n!", 1, ch, 0, vict,
			TO_VICT);
		call_magic(ch, vict, 0, SPELL_POISON, GET_LEVEL(ch),
			CAST_SPELL);

		return TRUE;
	} else if (vict && !number(0, 4)) {
		act("$n gazes into your eyes!", FALSE, ch, 0, vict, TO_VICT);
		act("$n gazes into $N's eyes!", FALSE, ch, 0, vict,
			TO_NOTVICT);
		call_magic(ch, vict, 0, SPELL_PETRIFY, GET_LEVEL(ch),
			CAST_PETRI);
		if (IS_AFFECTED_2(vict, AFF2_PETRIFIED))
            ch->removeAllCombat();
		return 1;
	}
	return FALSE;
}
