SPECIAL(cremator)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (cmd || !is_fighting(ch)) {
        return 0;
    }

    struct creature *vict = random_opponent(ch);
    switch (number(0, 5)) {
    case 0:
        act("$n attempts to throw you into the furnace!", false, ch, NULL,
            vict, TO_VICT);
        act("$n attempts to throw $N into the furnace!", false, ch, NULL,
            vict, TO_NOTVICT);
        break;
    case 1:
        act("$n lifts you and almosts hurls you into the furnace!", false, ch,
            NULL, vict, TO_VICT);
        act("$n lifts $N and almosts hurls $M into the furnace!", false, ch, NULL,
            vict, TO_NOTVICT);
        break;
    case 2:
        act("$n shoves you toward the blazing furnace!", false, ch, NULL,
            vict, TO_VICT);
        act("$n shoves $N toward the blazing furnace!", false, ch, NULL,
            vict, TO_NOTVICT);
        break;
    default:
        return 0;
    }

    return 1;
}
