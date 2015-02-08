//
// File: ornery_goat.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(ornery_goat)
{
    struct obj_data *can;
    int can_vnum = 3138;

    if (spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (!is_fighting(ch)) {
        return 0;
    }

    if (!number(0, 40)) {
        can = read_object(can_vnum);
        if (!can) {
            return 0;
        }
        obj_to_room(can, ch->in_room);
        act("$n coughs up $p!", true, ch, can, NULL, TO_ROOM);
        return 1;
    }
    return 0;
}
