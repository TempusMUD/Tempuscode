//
// File: falling_tower_dt.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(falling_tower_dt)
{
    struct room_data *under_room;

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    if ((under_room = real_room(5241)) == NULL) {
        return 0;
    }
    if (GET_POSITION(ch) == POS_FLYING) {
        return 0;
    }
    if (CMD_IS("down")) {
        send_to_char(ch,
                     "As you begin to descend the ladder, a rung breaks, sending you\r\n"
                     "plummeting downwards to the street!\r\n");
        char_from_room(ch, false);
        char_to_room(ch, under_room, false);
        send_to_char(ch, "You hit the ground hard!!\r\n");
        act("$n falls out of the tower above, and slams into the street hard!",
            false, ch, NULL, NULL, TO_ROOM);
        look_at_room(ch, ch->in_room, 0);
        GET_HIT(ch) = MAX(-8, GET_HIT(ch) -
                          dice(10, 100 - GET_DEX(ch) - 40 * AFF_FLAGGED(ch, AFF_INFLIGHT)));
        if (GET_HIT(ch) > 0) {
            GET_POSITION(ch) = POS_SITTING;
        } else if (GET_HIT(ch) < -6) {
            GET_POSITION(ch) = POS_MORTALLYW;
        } else if (GET_HIT(ch) < -3) {
            GET_POSITION(ch) = POS_INCAP;
        } else {
            GET_POSITION(ch) = POS_STUNNED;
        }
        return 1;
    }
    return 0;
}
