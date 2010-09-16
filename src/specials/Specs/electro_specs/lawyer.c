//
// File: lawyer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define KILLER 0
#define THIEF  1

SPECIAL(lawyer)
{
    struct creature *lawy = (struct creature *)me;
    if (!cmd || lawy->fighting || IS_NPC(ch))
        return 0;

    if (CMD_IS("buy") || CMD_IS("value")) {
        perform_say_to(lawy, ch, "Buzz off, I'm retired.");
        act("$n snickers.", false, lawy, 0, 0, TO_ROOM);
        return 1;
    }

    return 0;
}
