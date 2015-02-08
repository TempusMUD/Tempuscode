//
// File: modrian_fountain_obj.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(modrian_fountain_obj)
{
    struct obj_data *fount = (struct obj_data *)me;
    struct room_data *fountain_room;
    char *arg = tmp_getword(&argument);

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    if ((fountain_room = real_room(2899)) == NULL) {
        return 0;
    }
    if (!CMD_IS("enter")) {
        return 0;
    }

    if (isname(arg, fount->aliases)) {
        act("$n leaps into $p.", false, ch, fount, NULL, TO_ROOM);
        act("You leaps into $p.", false, ch, fount, NULL, TO_CHAR);
        char_from_room(ch, false);
        char_to_room(ch, fountain_room, false);
        act("$n leaps into $p, splashig water all over you.", false, ch, fount,
            NULL, TO_ROOM);
        look_at_room(ch, ch->in_room, 0);
        return 1;
    }
    return 0;
}
