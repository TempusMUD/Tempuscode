//
// File: newbie_tower_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
#include "help.h"
extern struct help_collection *help;
SPECIAL(newbie_tower_rm)
{
    struct help_item *cur = NULL;

    ACMD(do_hcollect_help);
    char *arg = tmp_getword(&argument);

    if (!CMD_IS("look") && !CMD_IS("examine")) {
        return 0;
    }

    if (CMD_IS("look")) {
        if (!strncasecmp(arg, "plate", 5) ||
            !strncasecmp(arg, "at plate", 8) ||
            !strncasecmp(arg, "map", 3) || !strncasecmp(arg, "at map", 6)) {
            cur = help_collection_find_item_by_id(help, 196);
            if (cur) {
                help_item_show(cur, ch, buf, sizeof(buf), 2);
                page_string(ch->desc, buf);
            }
            /*
               send_to_char(ch, "This map may be viewed at any time by typing 'help modrian'.\r\n");
               send_to_char(ch, "You may also look out the windows of the tower by using the\r\n"
               "look command to look in the cardinal directions.  For example,\r\n"
               "'look north'.\r\n");
             */
            return 1;
        }
    }
    if (CMD_IS("examine")) {
        if (!strncasecmp(arg, "plate", 5)) {
            cur = help_collection_find_item_by_id(help, 196);
            if (cur) {
                help_item_show(cur, ch, buf, sizeof(buf), 2);
                page_string(ch->desc, buf);
            }
            return 1;
        }
    }
    return 0;
}
