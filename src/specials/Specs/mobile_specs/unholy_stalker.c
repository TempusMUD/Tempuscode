//
// File: unholy_stalker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(unholy_stalker)
{
    if (spec_mode != SPECIAL_TICK)
        return 0;

    struct creature *mob = (struct creature *)me;

    if (!NPC_HUNTING(mob) && !mob->fighting) {
        act("$n dematerializes, returning to the negative planes.", true, mob,
            NULL, NULL, TO_ROOM);
        creature_purge(mob, true);
        return 1;
    }

    if (mob->fighting) {
        if (!number(0, 3)) {
            call_magic(mob, random_opponent(mob), NULL, NULL, SPELL_CHILL_TOUCH,
                GET_LEVEL(mob) + 10, CAST_SPELL);
        }

        struct creature *vict = random_opponent(mob);
        if (GET_HIT(mob) < 100 && GET_HIT(vict) > GET_HIT(mob) &&
            !ROOM_FLAGGED(mob->in_room, ROOM_NOMAGIC | ROOM_NORECALL) &&
            GET_LEVEL(mob) > number(20, 35)) {
            call_magic(mob, mob, NULL, NULL, SPELL_LOCAL_TELEPORT, 90, CAST_SPELL);
        }
    }

    return 0;
}
