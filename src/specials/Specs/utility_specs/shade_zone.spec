//
// File: shade_zone.spec					 -- Part of TempusMUD
//
// Copyright 2000 by John Watson, all rights reserved.
//    This code written by Lenon Kitchens, all rights reserved
//

#include "room_data.h"
#include "zone_data.h"
#include "utils.h"

struct char_data *ch, *tempch = NULL;

extern void set_local_time(struct zone_data *zone, struct time_info_data *local_time);

// moves all characters from "from" to "to"
// Does not deal with nulls in either place.
void move_chars(room_data *from, room_data *to) {
     char_data *ch = NULL;
     CharacterList::iterator it = from->people.begin();
     for( ; it != from->people.end(); ++it ) {
         ch = *it;
         char_from_room(ch);
         char_to_room(ch, to);
     }
}
// Connects the "dir" exit of link room to dest_room
// if either aren't found, it complains
void connect_rooms(int link_room, int dest_room, int dir ) {
    room_data *link = real_room(link_room);
    room_data *dest = real_room(dest_room);
    if( link == NULL ) {
        sprintf(buf,"SYSERR: shade_spec cannot find link room [%d]",link_room);
        return;
    }
    if( dest == NULL ) {
        sprintf(buf,"SYSERR: shade_spec cannot find destination room [%d]",dest_room);
        return;
    }
    if (link->dir_option[dir] != NULL) {
        move_chars(link->dir_option[dir]->to_room,dest);
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
    
    // Don't do anything.  Why is this?  What is cmd for?
    // F: specs are run when commands are typed within reference of them
    // ie. when somedone is in a room that a room spec is on.
    
    
    if (cmd)
		return 0;

    set_local_time(zone, &local_time);

    //reconnect the zone at the appropriate hours
    if ((local_time.hours == 7) || (local_time.hours == 13)) {
        REMOVE_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The shadows deepen.\r\n", zone, 0);
        connect_rooms(19716,19718,EAST);
        connect_rooms(19686,19688,NORTH);
        return 1;
    }
    else if ((local_time.hours == 19) || (local_time.hours == 12)){ 
        //disconnect the zone
        SET_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The world seems more gray.\r\n", zone, 0);
        connect_rooms(19716,19717,EAST);
        connect_rooms(19686,19687,NORTH);
        return 1;
    }
    return 0;
}

