//
// File: carrion_crawler.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(carrion_crawler)
{
    if (cmd || !is_fighting(ch)) {
        return 0;
    }
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }

    struct creature *vict = random_opponent(ch);
    if (!number(0, 3)) {
        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_PARA)) {
            if (number(0, 1)) {
                act("$n lashes at you with $s tentacles, but you leap aside!",
                    false, ch, NULL, vict, TO_VICT);
                act("$n lashes out at $N with $s tentacles!", false, ch, NULL,
                    vict, TO_NOTVICT);
            } else {
                act("$n lashes you with $s sticky tentacles!", false, ch, NULL,
                    vict, TO_VICT);
                act("$n lashes $N with $s sticky tentacles!", false, ch, NULL,
                    vict, TO_NOTVICT);
            }
        } else {
            act("$n lashes you with $s sticky tentacles!", false, ch, NULL,
                vict, TO_VICT);
            send_to_char(ch, "You feel paralyzed!\r\n");
            act("$n paralyzes $N with $s sticky tentacles!", false, ch, NULL,
                vict, TO_NOTVICT);
            WAIT_STATE(vict, PULSE_VIOLENCE * 2);
        }
        return 1;
    }
    return 0;
}
