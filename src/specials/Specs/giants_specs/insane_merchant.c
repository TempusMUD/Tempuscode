//
// File: insane_merchant.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(insane_merchant)
{
    if (cmd) {
        return 0;
    }
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (!ch->in_room->people) {
        return 0;
    }

    switch (number(0, 50)) {
    case 0:
        act("$n laughs maniacally, chilling you to the bone.", false, ch, NULL, NULL,
            TO_ROOM);
        break;
    case 1:
        act("$n begins to giggle uncontrollably.", false, ch, NULL, NULL, TO_ROOM);
        break;
    case 2:
        act("$n begins to sob loudly.", false, ch, NULL, NULL, TO_ROOM);
        break;
    case 3:
        act("$n laments the destruction of the fine city Istan.", false, ch, NULL,
            NULL, TO_ROOM);
        break;
    default:
        return 0;
    }
    return 1;
}
