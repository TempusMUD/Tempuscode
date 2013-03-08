//
// File: cemetary_ghoul.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cemetary_ghoul)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    struct room_data *r_ghoul_hole = real_room(4299), *r_cemetary_room = NULL;

    if (cmd || is_fighting(ch))
        return 0;

    if (OUTSIDE(ch) && ch->in_room->zone->weather->sunlight == SUN_LIGHT) {

        if (number(0, 2)) {
            if (!number(0, 3))
                act("$n cowers under the light of the suns.", true, ch, NULL, NULL,
                    TO_ROOM);
            else if (!number(0, 2))
                act("$n recoils from the light of the sun", true, ch, NULL, NULL,
                    TO_ROOM);
            else
                act("$n screams in agony as the sun burns $s flesh!", true, ch,
                    NULL, NULL, TO_ROOM);
            return 1;
        }

        act("$n claws $s way into the ground and disappears!",
            true, ch, NULL, NULL, TO_ROOM);
        char_from_room(ch, false);
        char_to_room(ch, r_ghoul_hole, false);
        act("$n has climbed into the hole.", false, ch, NULL, NULL, TO_ROOM);
        return 1;

    } else if (ch->in_room == r_ghoul_hole &&
        ch->in_room->zone->weather->sunlight == SUN_DARK) {

        if (number(0, 2)) {
            act("$n twitches hungrily.", true, ch, NULL, NULL, TO_ROOM);
            return 1;
        }

        r_cemetary_room = real_room(number(4231, 4236));
        if (r_cemetary_room == NULL)
            return 1;

        act("$n claws $s way out of the hole.", false, ch, NULL, NULL, TO_ROOM);
        char_from_room(ch, false);
        char_to_room(ch, r_cemetary_room, false);
        act("$n claws $s way out of the ground.", false, ch, NULL, NULL, TO_ROOM);
        return 1;
    } else
        return 0;
}
