//
// File: loudspeaker.spec                                         -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
ACMD(do_gecho);

SPECIAL(loud_speaker)
{
    struct obj_data *speaker = (struct obj_data *)me;

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    if (!CMD_IS("yell")) {
        return 0;
    }

    skip_spaces(&argument);
    if (!*argument) {
        return 0;
    }

    argument = one_argument(argument, buf);
    if (!isname(buf, speaker->aliases)) {
        return 0;
    }

    if (!*argument) {
        return 0;
    }
    act("$n yells into $p.", true, ch, speaker, NULL, TO_ROOM);
    act("You yell into $p.", true, ch, speaker, NULL, TO_CHAR);
    snprintf(buf, sizeof(buf), "%s BOOMS '%s'", speaker->name, argument);
    do_gecho(ch, buf, 0, 0);
    return 1;
}
