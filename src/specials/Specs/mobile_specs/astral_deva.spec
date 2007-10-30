//
// File: astral_deva.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(astral_deva)
{
	if (spec_mode != SPECIAL_TICK)
		return 0;
	if (!ch->isFighting() || cmd)
		return 0;

    Creature *vict = ch->findRandomCombat();
	if (affected_by_spell(vict, SPELL_GREATER_INVIS) &&
		!GET_MOB_WAIT(ch) && GET_MANA(ch) > 50) {
		act("$n stares at $N and utters a strange incantation.", FALSE, ch, 0,
			vict, TO_NOTVICT);
		act("$n stares at you and utters a strange incantation.", FALSE, ch, 0,
			vict, TO_VICT);
		affect_from_char(vict, SPELL_GREATER_INVIS);
		GET_MANA(ch) -= 50;
		return 1;
	}

	if (!IS_AFFECTED_2(ch, AFF2_BLADE_BARRIER) &&
		!GET_MOB_WAIT(ch) && GET_MANA(ch) > 100) {
		act("$n concentrates for a moment...\r\n"
			"...a flurry of whirling blades appears in the air before $m!",
			FALSE, ch, 0, 0, TO_ROOM);
		SET_BIT(AFF2_FLAGS(ch), AFF2_BLADE_BARRIER);
		GET_MANA(ch) -= 100;
		return 1;
	}

	return 0;
}
