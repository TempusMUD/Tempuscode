//
// File: kata.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(kata)
{
    struct creature *kata = (struct creature *)me;

    if (kata->master || AFF_FLAGGED(kata, AFF_CHARM)) {
        return 0;
    }

    if (!CMD_IS("rescue")) {
        return 0;
    }

    skip_spaces(&argument);

    if (!isname(argument, kata->player.name)) {
        return 0;
    }

    if (IS_EVIL(ch) && IS_GOOD(kata)) {
        perform_say(kata, "say", "No! I will not follow you.");
        return 1;
    }

    snprintf(buf, sizeof(buf),
             "Thank you for rescuing me, %s!  I will be a loyal companion.",
             GET_NAME(ch));
    perform_say(kata, "say", buf);
    add_follower(kata, ch);
    SET_BIT(AFF_FLAGS(kata), AFF_CHARM);
    return 1;
}
