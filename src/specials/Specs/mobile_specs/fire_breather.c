//
// File: fire_breather.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fire_breather)
{

    if (cmd) {
        return false;
    }
    if (spec_mode != SPECIAL_TICK) {
        return false;
    }
    if (GET_POSITION(ch) != POS_FIGHTING || !is_fighting(ch)) {
        return false;
    }

    struct creature *vict = random_opponent(ch);
    if (vict && (vict->in_room == ch->in_room) && !number(0, 4)) {
        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_BREATH)) {
            damage(ch, vict, NULL, 0, SPELL_FIRE_BREATH, -1);
        } else {
            damage(ch, vict, NULL, GET_LEVEL(ch) + number(8, 30),
                   SPELL_FIRE_BREATH, -1);
        }
        return true;
    }
    return false;
}
