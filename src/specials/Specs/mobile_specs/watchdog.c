//
// File: watchdog.spec                                         -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// As people stand in the room, watchdog gets more and more irritated.  When
// he sees someone ugly enough, he snaps.

SPECIAL(watchdog)
{
    struct creature *dog = (struct creature *)me;
    struct creature *vict = NULL;
    static int8_t indignation = 0;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }

    if (cmd || !AWAKE(dog) || is_fighting(dog)) {
        return 0;
    }

    for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch != dog && can_see_creature(dog, tch)
            && GET_LEVEL(tch) < LVL_IMMORT) {
            vict = tch;
            break;
        }
    }

    if (!vict || vict == dog) {
        return 0;
    }

    indignation++;

    switch (number(0, 4)) {
    case 0:
        act("$n growls menacingly at $N.", false, dog, NULL, vict,
            TO_NOTVICT);
        act("$n growls menacingly at you.", false, dog, NULL, vict, TO_VICT);
        break;
    case 1:
        act("$n barks loudly at $N.", false, dog, NULL, vict, TO_NOTVICT);
        act("$n barks loudly at you.", false, dog, NULL, vict, TO_VICT);
        break;
    case 2:
        act("$n growls at $N.", false, dog, NULL, vict, TO_NOTVICT);
        act("$n growls at you.", false, dog, NULL, vict, TO_VICT);
        break;
    case 3:
        act("$n snarls at $N.", false, dog, NULL, vict, TO_NOTVICT);
        act("$n snarls at you.", false, dog, NULL, vict, TO_VICT);
        break;
    default:
        break;
    }

    if (indignation > (GET_CHA(vict) / 4)) {
        hit(dog, vict, TYPE_UNDEFINED);
        indignation = 0;
        vict = NULL;
    }
    return 1;
}
