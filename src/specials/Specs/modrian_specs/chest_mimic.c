//
// File: chest_mimic.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(chest_mimic)
{
    struct creature *mimic = (struct creature *)me;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    skip_spaces(&argument);

    if (!(CMD_IS("open") || CMD_IS("pick") || CMD_IS("unlock") ||
          CMD_IS("look") || CMD_IS("examine"))) {
        return 0;
    }

    if (!*argument) {
        return 0;
    }

    if (!strncasecmp(argument, "in chest", 8)) {
        send_to_char(ch, "It seems to be closed.");
        act("$n looks at the bulging chest by the wall.", true, ch, NULL, NULL,
            TO_ROOM);
        return 1;
    }

    if (strncasecmp(argument, "chest", 5) &&
        strncasecmp(argument, "at bulging", 10) &&
        strncasecmp(argument, "at chest", 8) &&
        strncasecmp(argument, "at mimic", 8) &&
        strncasecmp(argument, "bulging", 7) &&
        strncasecmp(argument, "mimic", 5)) {
        return 0;
    }

    if (GET_POSITION(ch) == POS_FIGHTING) {
        return 0;
    }
    if (CMD_IS("open") || CMD_IS("pick") || CMD_IS("unlock")) {
        if (strncasecmp(argument, "chest", 5) &&
            strncasecmp(argument, "bulging", 7) &&
            strncasecmp(argument, "mimic", 5)) {
            return 0;
        }
        send_to_char(ch,
                     "The chest lurches forward suddenly!  It has sharp teeth!!\r\n");
        act("$n attempts to open the bulging chest.", false, ch, NULL, NULL,
            TO_ROOM);
        act("The chest lurches forward suddenly and snaps at $n!", false, ch,
            NULL, NULL, TO_ROOM);
        hit(mimic, ch, TYPE_BITE);
        return true;

    } else if (CMD_IS("look") || CMD_IS("examine")) {
        send_to_char(ch,
                     "The chest appears to be full of jewels and precious metals.\r\n");
        act("$n looks at the bulging chest by the wall.", true, ch, NULL, NULL,
            TO_ROOM);
        return true;
    } else {
        return false;
    }
}
