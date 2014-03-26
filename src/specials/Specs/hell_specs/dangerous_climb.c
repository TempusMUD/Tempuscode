//
// File: dangerous_climb.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(dangerous_climb)
{

    int dam;
    struct room_data *toroom = NULL;

    if (cmd && cmd < NUM_DIRS && GET_LEVEL(ch) < LVL_IMMORT &&
        GET_POSITION(ch) < POS_FLYING && number(5, 25) > GET_DEX(ch) &&
        EXIT(ch, DOWN) && (toroom = EXIT(ch, DOWN)->to_room)) {
        act("A rock shifts and $n goes crashing down the steep path!",
            false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch,
            "A rock shifts under your feet sending you tumbling down the path!\r\n"
            "Your body crashes painfully against the rocks below!!\r\n");
        dam = dice(5, 8) + (GET_WEIGHT(ch) / 16);
        dam -= (dam * (100 - GET_AC(ch))) / 400;
        if (damage(ch, ch, NULL, dam, TYPE_FALLING, -1))
            return 1;

        char_from_room(ch, false);
        char_to_room(ch, toroom, false);
        look_at_room(ch, ch->in_room, 0);
        act("$n comes crashing down the rocks from above!",
            false, ch, NULL, NULL, TO_ROOM);
        return 1;
    }
    return 0;
}
