//
// File: rat_mama.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(rat_mama)
{
    int rat_rooms[] = {
        2940,
        2941,
        2942,
        2943,
        -1
    };
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (cmd || is_fighting(ch) || GET_POSITION(ch) == POS_FIGHTING) {
        return 0;
    }

    for (int i = 0; rat_rooms[i] != -1; i++) {
        struct room_data *room = real_room(rat_rooms[i]);
        if (room == NULL) {
            errlog("Invalid room %d in rat_mama special", rat_rooms[i]);
            continue;
        }
        for (GList *it = first_living(room->people); it; it = next_living(it)) {
            struct creature *tch = it->data;
            if (number(0, 1) == 0
                || IS_PC(tch)
                || !isname("rat", tch->player.name)) {
                continue;
            }

            act("$n climbs into a hole in the wall.", false, tch, NULL, NULL,
                TO_ROOM);
            char_from_room(tch, true);
            creature_purge(tch, true);
        }

        if (room->people && !number(0, 1)) {
            struct creature *rat = read_mobile(number(4206, 4208));
            if (rat == NULL) {
                continue;
            }
            char_to_room(rat, room, false);
            act("$n climbs out of a hole in the wall.", false, rat, NULL, NULL,
                TO_ROOM);
        }
    }
    return false;
}
