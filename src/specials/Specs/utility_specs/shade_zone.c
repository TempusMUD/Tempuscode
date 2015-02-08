//
// File: shade_zone.spec                     -- Part of TempusMUD
//
// Copyright 2000 by John Watson, all rights reserved.
// This code written by Lenon Kitchens, all rights reserved
//

#include "room_data.h"
#include "zone_data.h"
#include "utils.h"

extern void set_local_time(struct zone_data *zone,
                           struct time_info_data *local_time);

// moves all characters from "from" to "to"
// Does not deal with nulls in either place.
void
move_chars(struct room_data *from, struct room_data *to)
{
    struct creature *ch = NULL;
    GList *old_people = g_list_copy(from->people);

    for (GList *it = old_people; it; it = it->next) {
        ch = it->data;
        char_from_room(ch, false);
        char_to_room(ch, to, false);
    }
    g_list_free(old_people);
}

// Connects the "dir" exit of link room to dest_room
// if either aren't found, it complains
void
connect_rooms(int link_room, int dest_room, int dir)
{
    struct room_data *link = real_room(link_room);
    struct room_data *dest = real_room(dest_room);
    if (link == NULL) {
        errlog("shade_spec cannot find link room [%d]", link_room);
        return;
    }
    if (dest == NULL) {
        errlog("shade_spec cannot find destination room [%d]", dest_room);
        return;
    }
    if (link->dir_option[dir] != NULL) {
        move_chars(link->dir_option[dir]->to_room, dest);
        link->dir_option[dir]->to_room = dest;
    }
}

// Shadow zones unlink themselves from the rest of the world
// when there aren't any shadows.  i.e. 12am and between the
// hours of 7pm and 6am there's no shadows so the entire plane
// of shadow disconnects from the rest of the world.
// Shadow zones are also isolated when they are disconnected.
// This is kind of cool in that any player trapped there at
// night is actually trapped there until morning.
// Thought:
// Spawn a given number of creatures in a shadow zone when it
// becomes disconnected?
// Shadow zones are also !weather

SPECIAL(shade_zone)
{
    struct time_info_data local_time;
    struct zone_data *zone = (struct zone_data *)me;

    if (spec_mode != SPECIAL_TICK) {
        return 0;
    }

    set_local_time(zone, &local_time);

    // reconnect the zone at the appropriate hours
    if ((local_time.hours == 7) || (local_time.hours == 13)) {
        REMOVE_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The shadows deepen.\r\n", zone, 0);
        connect_rooms(19716, 19718, EAST);
        connect_rooms(19686, 19688, NORTH);
        return 1;
    } else if ((local_time.hours == 19) || (local_time.hours == 12)) {
        // disconnect the zone
        SET_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The world seems more gray.\r\n", zone, 0);
        connect_rooms(19716, 19717, EAST);
        connect_rooms(19686, 19687, NORTH);
        return 1;
    }
    return 0;
}
