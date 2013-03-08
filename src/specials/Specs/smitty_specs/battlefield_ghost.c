//
// File: battlefield_ghost.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(battlefield_ghost)
{
    struct room_data *r_ghost_hole = real_room(20809), *r_bones_room = NULL;

    const room_num v_bones_room[] = {
        20831,
        20866,
        20884,
        20898
    };

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (r_ghost_hole == NULL)
        return 0;

    if (cmd || is_fighting(ch))
        return 0;

    if (OUTSIDE(ch) && ch->in_room != r_ghost_hole &&
        ch->in_room->zone->weather->sunlight == SUN_LIGHT) {
        if (number(0, 2)) {
            if (!number(0, 3))
                act("$n looks to the sun with sorrow.", true, ch, NULL, NULL,
                    TO_ROOM);
            else if (!number(0, 2))
                act("$n seems to recoil from the sun.", true, ch, NULL, NULL,
                    TO_ROOM);
            else
                act("$n begins to fade out of reality as the sun rises.",
                    true, ch, NULL, NULL, TO_ROOM);
            return 1;
        }

        act("$n lies down on the pile of bones and disappears!",
            true, ch, NULL, NULL, TO_ROOM);
        char_from_room(ch, true);
        char_to_room(ch, r_ghost_hole, true);
        act("$n has disappeared into the netherworld.", false, ch, NULL, NULL,
            TO_ROOM);
        return 1;

    } else if (ch->in_room == r_ghost_hole &&
        (ch->in_room->zone->weather->sunlight == SUN_SET ||
            ch->in_room->zone->weather->sunlight == SUN_DARK)) {

        if (GET_NPC_VNUM(ch) == 20801)
            r_bones_room = real_room(20874);
        else
            r_bones_room = real_room(v_bones_room[number(0, 3)]);

        if (r_bones_room == NULL) {
            if (GET_NPC_VNUM(ch) == 20801)
                slog("r_bones_room == NULL in battlefield_ghost()...general");
            else
                slog("r_bones_room == NULL in battlefield_ghost().");
            return 0;
        }

        act("$n seems to rise out of the pile of bones.", false, ch, NULL, NULL,
            TO_ROOM);
        char_from_room(ch, true);
        char_to_room(ch, r_bones_room, true);
        act("$n seems to rise out of the pile of bones.", false, ch, NULL, NULL,
            TO_ROOM);
        return 1;
    } else
        return 0;
}
