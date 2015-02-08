//
// File: malbolge_bridge.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(malbolge_bridge)
{
    struct room_data *bridge = (struct room_data *)me, *under = NULL;

    if (spec_mode != SPECIAL_CMD
        && spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_ENTER) {
        return 0;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT || GET_POSITION(ch) == POS_FLYING ||
        (MOUNTED_BY(ch) && (GET_POSITION(MOUNTED_BY(ch)) == POS_FLYING)) ||
        number(5, 25) < GET_DEX(ch)) {
        return 0;
    }

    act("A sudden earthquake shakes the bridge!!", false, ch, NULL, NULL, TO_ROOM);
    act("A sudden earthquake shakes the bridge!!",
        false, ch, NULL, NULL, TO_CHAR | TO_SLEEP);

    if (!(under = bridge->dir_option[DOWN]->to_room)) {
        return 1;
    }

    act("$n loses $s balance and falls off the bridge!",
        false, ch, NULL, NULL, TO_ROOM);
    act("You lose your balance and fall off the bridge!",
        false, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
    char_from_room(ch, false);
    char_to_room(ch, under, false);
    look_at_room(ch, ch->in_room, 0);
    int rc = damage(ch, ch, NULL,
                    dice(3, 5) + ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) / 32),
                    TYPE_FALLING, WEAR_RANDOM);
    if (rc) {
        return rc;
    }
    return 1;
}
