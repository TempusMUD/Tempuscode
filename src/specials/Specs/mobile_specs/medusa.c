//
// File: medusa.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(medusa)
{
    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (GET_POSITION(ch) != POS_FIGHTING)
        return false;

    struct creature *vict = random_opponent(ch);
    if (isname("medusa", ch->player.name) &&
        vict && (vict->in_room == ch->in_room) &&
        (number(0, 57 - GET_LEVEL(ch)) == 0)) {
        act("The snakes on $n bite $N!", 1, ch, NULL, vict, TO_NOTVICT);
        act("You are bitten by the snakes on $n!", 1, ch, NULL, vict, TO_VICT);
        call_magic(ch, vict, NULL, NULL, SPELL_POISON, GET_LEVEL(ch),
            CAST_SPELL);

        return true;
    } else if (vict && !AFF_FLAGGED(vict, AFF_BLIND) && !number(0, 4) ) {
        act("$n gazes into your eyes!", false, ch, NULL, vict, TO_VICT);
        act("$n gazes into $N's eyes!", false, ch, NULL, vict, TO_NOTVICT);
        call_magic(ch, vict, NULL, NULL, SPELL_PETRIFY, GET_LEVEL(ch),
            CAST_PETRI);
        return 1;
    }
    return false;
}
