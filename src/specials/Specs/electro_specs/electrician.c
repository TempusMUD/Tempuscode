//
// File: electrician.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_flee);

SPECIAL(electrician)
{
    if (cmd || is_fighting(ch) || !AWAKE(ch) || number(0, 150))
        return 0;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK &&
        spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_LEAVE) {
        return 0;
    }

    struct creature *mob = (struct creature *)me;
    int num;
    if (spec_mode == SPECIAL_ENTER || spec_mode == SPECIAL_LEAVE) {
        num = number(0, 4);
    } else {
        num = number(0, 5);
    }

    switch (num) {
    case 0:
    case 4:
        act("$n shocks the hell out of $mself!!", false, mob, NULL, NULL, TO_ROOM);
        break;
    case 1:
        act("$n fumbles and drops a screwdriver.", false, mob, NULL, NULL, TO_ROOM);
        break;
    case 2:
        act("$n jabs $s finger with a wire.", false, mob, NULL, NULL, TO_ROOM);
        break;
    case 3:
        act("$n's hair suddenly stands on end.", false, mob, NULL, NULL, TO_ROOM);
        break;
    case 5:
        perform_say(mob, "say", "Uh-oh.");
        do_flee(mob, tmp_strdup(""), 0, 0);
        break;
    }
    return 1;
}
