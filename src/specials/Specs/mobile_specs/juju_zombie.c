//
// File: juju_zombie.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(juju_zombie)
{
    int prob;
    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (GET_POSITION(ch) != POS_FIGHTING || !is_fighting(ch))
        return 0;

    struct creature *vict = random_opponent(ch);
    if (!number(0, 1)) {
        prob = MAX(GET_LEVEL(ch) - GET_LEVEL(vict), 10) +
            number(20, 60) + (GET_AC(vict) / 10) - GET_DEX(vict);
        if (prob < number(20, 70))
            return 0;

        act("$n reaches out and touches you!  You feel paralyzed!", false, ch,
            NULL, vict, TO_VICT);
        act("$n reaches out and paralyzes $N!", false, ch, NULL, vict,
            TO_NOTVICT);
        WAIT_STATE(vict, number(1, 3) * PULSE_VIOLENCE);
        return 1;
    }
    return 0;
}
