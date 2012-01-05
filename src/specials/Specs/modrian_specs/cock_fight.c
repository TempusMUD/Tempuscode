//
// File: cock_fight.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cock_fight)
{
    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (!is_fighting(ch)) {
        switch (number(0, 40)) {
        case 0:
            act("$n scratches the ground with $s claw.", true, ch, NULL, NULL,
                TO_ROOM);
            send_to_char(ch, "You scratch.\r\n");
            return 1;
        case 1:
            act("$n squawks loudly!", false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch, "You squawk!\r\n");
            return 1;
        case 2:
            act("$n struts around proudly.", true, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch, "You strut.\r\n");
            return 1;
        default:
            return 0;
        }
    }
    struct creature *target = random_opponent(ch);
    switch (number(0, 16)) {
    case 0:
        act("$n leaps into the air, feathers flying!", true, ch, NULL, NULL,
            TO_ROOM);
        send_to_char(ch, "You leap.\r\n");
        return 1;
    case 1:
        send_to_room("Feathers fly everywhere!\r\n", ch->in_room);
        return 1;
    case 2:
        act("$N screams as $E attacks you!", false, target, NULL, ch, TO_CHAR);
        act("$N screams as $E attacks $n!", false, target, NULL, ch, TO_ROOM);
        return 1;
    default:
        return 0;
    }
    return 0;
}
