//
// File: tiamat.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(tiamat)
{

    int type = 0;
    struct room_data *lair = real_room(16182);

    if (cmd || !is_fighting(ch) || CHECK_WAIT(ch) || number(0, 4)) {
        return 0;
    }
    if (spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (GET_HIT(ch) < 200 && ch->in_room != lair && lair != NULL) {
        act("$n vanishes in a prismatic blast of light!",
            false, ch, NULL, NULL, TO_ROOM);
        remove_all_combat(ch);
        char_from_room(ch, false);
        char_to_room(ch, lair, false);
        act("$n appears in a prismatic blast of light!",
            false, ch, NULL, NULL, TO_ROOM);
        return 1;
    }
    switch (number(0, 4)) {
    case 0:
        type = SPELL_GAS_BREATH;
        break;
    case 1:
        type = SPELL_ACID_BREATH;
        break;
    case 2:
        type = SPELL_LIGHTNING_BREATH;
        break;
    case 3:
        type = SPELL_FROST_BREATH;
        break;
    case 4:
        type = SPELL_FIRE_BREATH;
        break;
    }

    struct creature *vict = random_opponent(ch);
    call_magic(ch, vict, NULL, NULL, type, GET_LEVEL(ch), SAVING_BREATH);
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    return 1;
}
