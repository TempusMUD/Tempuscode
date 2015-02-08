//
// File: newbie_portal_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_portal_rm)
{

    struct room_data *room = (struct room_data *)me;

    if (CMD_IS("north") || CMD_IS("enter")) {
        act("$n steps into the portal.", false, ch, NULL, NULL, TO_ROOM);
        char_from_room(ch, false);
        if (IS_REMORT(ch)) {
            char_to_room(ch, room, false);
            call_magic(ch, ch, NULL, NULL, SPELL_WORD_OF_RECALL, LVL_GRIMP,
                       CAST_SPELL);
            return 1;
        } else if (GET_EXP(ch) < 10) {
            char_to_room(ch, real_room(2329), false);
        } else if (GET_EXP(ch) < 200) {
            char_to_room(ch, real_room(2330), false);
        } else if (GET_LEVEL(ch) < 3) {
            char_to_room(ch, real_room(2332), false);
        } else if (GET_LEVEL(ch) < 4) {
            char_to_room(ch, real_room(2336), false);
        } else if (GET_LEVEL(ch) < 6) {
            char_to_room(ch, real_room(2358), false);
        } else {
            char_to_room(ch, real_room(3001), false);
            if (GET_HOME(ch) == HOME_NEWBIE_TOWER) {
                population_record[GET_HOME(ch)]--;
                GET_HOME(ch) = HOME_MODRIAN;
                population_record[GET_HOME(ch)]++;
            }
        }
        look_at_room(ch, ch->in_room, 0);
        act("$n has stepped through the portal into the arena.",
            true, ch, NULL, NULL, TO_ROOM);
        return 1;
    }
    return 0;
}
