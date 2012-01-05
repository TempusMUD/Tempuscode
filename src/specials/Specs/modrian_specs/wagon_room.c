//
// File: wagon_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(wagon_room)
{
    struct obj_data *i;
    int wagon_obj_rnum = 10;
    if (spec_mode != SPECIAL_CMD)
        return 0;

    if (!CMD_IS("disembark") && !CMD_IS("leave") && !CMD_IS("look"))
        return 0;

    for (i = object_list; i; i = i->next)
        if (wagon_obj_rnum == GET_OBJ_VNUM(i) && i->in_room != NULL)
            break;

    if (!i) {
        mudlog(LVL_DEMI, BRF, true,
            "WARNING: Chars may be trapped in wagon (room 10)!");
        act("You suddenly realize that reality is not what it seems to be...",
            false, ch, NULL, NULL, TO_CHAR);
        return 1;
    }

    if (CMD_IS("look")) {
        skip_spaces(&argument);
        if (!*argument)
            return 0;
        if (strncasecmp(argument, "out", 3))
            return 0;
        send_to_char(ch, "You look off the side of the wagon...\r\n\r\n");
        look_at_room(ch, i->in_room, 1);
        return true;
    }

    if (CMD_IS("disembark") || CMD_IS("leave")) {
        send_to_char(ch, "You leap off the side of the wagon.\r\n\r\n");
        act("$n leaps off the side of the wagon.", true, ch, NULL, NULL, TO_ROOM);

        char_from_room(ch, false);
        char_to_room(ch, i->in_room, false);
        look_at_room(ch, ch->in_room, 0);

        act("$n leaps off the wagon and lands near you.", true, ch, NULL, NULL,
            TO_ROOM);
        return 1;
    }
    return 0;
}
