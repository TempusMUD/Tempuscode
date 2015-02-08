//
// File: black_rose.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(black_rose)
{
    int8_t num;
    struct obj_data *trash = (struct obj_data *)me;

    if (spec_mode != SPECIAL_CMD || !CMD_IS("smell")) {
        return 0;
    }

    skip_spaces(&argument);

    if (!*argument) {
        return 0;
    }

    if (!isname(argument, trash->aliases)) {
        return 0;
    }

    send_to_char(ch,
                 "You smell the scent of the black rose and feel oddly invigorated!\r\n");
    act("$n smells $p.", true, ch, trash, NULL, TO_ROOM);

    num = number(10, 30);
    GET_MANA(ch) = MIN(GET_MANA(ch) + num, GET_MAX_MANA(ch));
    WAIT_STATE(ch, 2 RL_SEC);
    return 1;
}
