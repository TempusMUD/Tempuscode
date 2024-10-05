//
// File: astral_deva.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(astral_deva)
{
    if (spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (!is_fighting(ch) || cmd) {
        return 0;
    }

    struct creature *vict = random_opponent(ch);
    if (affected_by_spell(vict, SPELL_GREATER_INVIS) &&
        !CHECK_WAIT(ch) && GET_MANA(ch) > 50) {
        act("$n stares at $N and utters a strange incantation.", false, ch, NULL,
            vict, TO_NOTVICT);
        act("$n stares at you and utters a strange incantation.", false, ch, NULL,
            vict, TO_VICT);
        affect_from_char(vict, SPELL_GREATER_INVIS);
        GET_MANA(ch) -= 50;
        return 1;
    }

    if (!AFF2_FLAGGED(ch, AFF2_BLADE_BARRIER) &&
        !CHECK_WAIT(ch) && GET_MANA(ch) > 100) {
        act("$n concentrates for a moment...\r\n"
            "...a flurry of whirling blades appears in the air before $m!",
            false, ch, NULL, NULL, TO_ROOM);
        SET_BIT(AFF2_FLAGS(ch), AFF2_BLADE_BARRIER);
        GET_MANA(ch) -= 100;
        return 1;
    }

    return 0;
}
