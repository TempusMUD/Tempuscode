//
// File: morkoth.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define NUM_MORKOTH_OBJS 3

SPECIAL(morkoth)
{

    struct creature *morkoth = (struct creature *)me;

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }
    if (!cmd || morkoth->in_room->number != GET_NPC_LAIR(morkoth)) {
        return 0;
    }

    if (CMD_IS("down")) {
        act("$n prevents you from going down the tunnel.",
            false, morkoth, 0, ch, TO_VICT);
        act("$n prevents $N from going down the tunnel.",
            false, morkoth, 0, ch, TO_NOTVICT);
        return 1;
    }

    return 0;

}
