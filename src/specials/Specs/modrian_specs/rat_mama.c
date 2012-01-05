//
// File: rat_mama.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(rat_mama)
{
    struct creature *rat;
    int i;
    int rat_rooms[] = {
        2940,
        2941,
        2942,
        2943,
        -1
    };
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || ch->fighting || GET_POSITION(ch) == POS_FIGHTING)
        return (0);

    for (i = 0; i != -1; i++) {
        for (GList *it = first_living(real_room(rat_rooms[i])->people); it;
             it = next_living(it)) {
                 struct creature *tch = it->data;
             if (!number(0, 1) == 0 && IS_NPC(tch)
                 && isname("rat", tch->player.name)) {
                 act("$n climbs into a hole in the wall.", false, tch, NULL, NULL,
                     TO_ROOM);
                 char_from_room(tch, true);
                 creature_purge(tch, true);
                 return true;
            }
        }
        if (real_room(rat_rooms[i])->people && !number(0, 1)) {
            rat = read_mobile(number(4206, 4208));
            char_to_room(rat, real_room(rat_rooms[i]), false);
            act("$n climbs out of a hole in the wall.", false, rat, NULL, NULL,
                TO_ROOM);
            return true;
        }
    }
    return false;
}
