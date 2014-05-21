//
// File: prac_manual.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(prac_manual)
{
    struct obj_data *manual = (struct obj_data *)me;
    if (!CMD_IS("read") || manual->carried_by != ch)
        return 0;
    skip_spaces(&argument);
    if (!isname(manual->aliases, argument))
        return 0;

    send_to_char(ch, "The writing on the manual has been smudged out.\r\n");
    return 1;
}
