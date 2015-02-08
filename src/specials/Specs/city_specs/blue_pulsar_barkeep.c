//
// File: blue_pulsar_barkeep.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(blue_pulsar)
{

    struct creature *bartender = (struct creature *)me;
    struct obj_data *beer = NULL;
    int beer_vnum = (number(0, 1) ? 50300 : 50301);

    if (!CMD_IS("yell")) {
        return 0;
    }
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    skip_spaces(&argument);

    if (!*argument || strcasecmp(argument, "beer")) {
        return 0;
    }

    if (!(beer = read_object(beer_vnum))) {
        errlog("Blue Pulsar beer not in database.");
    } else {
        act("$N yells, 'BEER!!!!'", false, ch, NULL, NULL, TO_NOTVICT);
        act("$n throws $p across the room to $N!", false, bartender, beer, ch,
            TO_NOTVICT);
        act("$n throws $p to you.", false, bartender, beer, ch, TO_VICT);
        return 1;
    }
    return 0;
}
