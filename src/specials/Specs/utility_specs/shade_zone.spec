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

extern struct room_data *real_room(int vnum);
extern void set_local_time(struct zone_data *zone, struct time_info_data *local_time);
extern void send_to_zone(char *messg, struct zone_data *zone, int outdoor);

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
	struct room_data *link_room  = NULL;
    
    // Don't do anything.  Why is this?  What is cmd for?
  	if (cmd)
		return 0;

    set_local_time(zone, &local_time);

    //reconnect the zone at the appropriate hours
    if ((local_time.hours == 7) || (local_time.hours == 13)) {
        REMOVE_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The shadows deepen.\r\n", zone, 0);
        
        if ((link_room = real_room(19716)) && real_room(19718)) {
            if (link_room->dir_option[1] != NULL) {
                for (ch = link_room->dir_option[1]->to_room->people; ch;) {
                    tempch = ch->next_in_room;
                    char_from_room(ch);
                    char_to_room(ch, real_room(19718));
                    ch = tempch;
                }    
                link_room->dir_option[1]->to_room = real_room(19718);
            }  
        }
        
        if ((link_room = real_room(19686)) && real_room(19688)) {
            if (link_room->dir_option[0] != NULL) {
                for (ch = link_room->dir_option[0]->to_room->people; ch;) {
                    tempch = ch->next_in_room;
                    char_from_room(ch);
                    char_to_room(ch, real_room(19688));
                    ch = tempch;
                }
                link_room->dir_option[0]->to_room = real_room(19688);
            }
        }
        return 1;
    }
    else if ((local_time.hours == 19) || (local_time.hours == 12)){ 
        //disconnect the zone
        SET_BIT(zone->flags, ZONE_ISOLATED);
        send_to_zone("The world seems more gray.\r\n", zone, 0);
        
        if ((link_room = real_room(19716)) && real_room(19717)) {
            if (link_room->dir_option[1] != NULL) {
                for (ch = link_room->dir_option[1]->to_room->people; ch;) {
                    tempch = ch->next_in_room;
                    char_from_room(ch);
                    char_to_room(ch, real_room(19717));
                    ch = tempch;
                }
                link_room->dir_option[1]->to_room = real_room(19717);
            }
        }
    
        if ((link_room = real_room(19686)) && real_room(19687)) {
            if (link_room->dir_option[0] != NULL) {
                for (ch = link_room->dir_option[0]->to_room->people; ch;) {
                    tempch = ch->next_in_room;
                    char_from_room(ch);
                    char_to_room(ch, real_room(19687));
                    ch = tempch;
                }
                link_room->dir_option[0]->to_room = real_room(19687);
            }
        }
        return 1;
    }
    return 0;
}
