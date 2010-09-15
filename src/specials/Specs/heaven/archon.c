//
// File: archon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(archon)
{

    struct room_data *room = real_room(43252);

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd)
        return 0;
    if (!ch->fighting && ch->in_room->zone->plane != PLANE_HEAVEN) {
        for (GList * it = ch->in_room->people; it; it = it->next) {
            struct creature *tch = it->data;
            if (tch != ch && IS_ARCHON(tch) && tch->fighting) {
                do_rescue(ch, fname(tch->player.name), 0, 0, 0);
                return 1;
            }
        }

        act("$n disappears in a flash of light.", false, ch, 0, 0, TO_ROOM);
        if (room) {
            char_from_room(ch, false);
            char_to_room(ch, room, false);
            act("$n appears at the center of the room.", false, ch, 0, 0,
                TO_ROOM);
        } else {
            creature_purge(ch, true);
        }
        return 1;
    }
    return 0;
}
