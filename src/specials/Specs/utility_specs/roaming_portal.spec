//
// File: roaming_portal.spec					 -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//

#include "room_data.h"
#include "obj_data.h"
#include "utils.h"

// Don't leave the zone
// Don't go into Inside rooms
// Don't go to rooms with players
// Don't leave while players in rooms


SPECIAL(roaming_portal)
{
	struct obj_data *portal= (struct obj_data *) me; 
	struct room_data *dest = NULL;
    int num_dests = 0;

    if( spec_mode != SPECIAL_TICK )
        return 0;
    // Don't do nuffin.
  	if (cmd)
		return 0;

    // Just in case some idiot is carrying it.
	if(!portal->in_room)
		return 0;

    if(!OBJ_APPROVED(portal))
        return 0;

	if(portal->obj_flags.type_flag != ITEM_PORTAL)
        return 0;

	// If there is a player in the room, don't do anything.
	if(player_in_room(portal->in_room))
		return 0;

	// Is it time to leave?
    if(portal->obj_flags.timer > 0) {
        (portal->obj_flags.timer)--;
        return 0;
    }
        
	// It's time to leave.
	// Start the timer for the next jump.
	portal->obj_flags.timer = 20 + dice(4,10);

    // Find the destination
    for (dest = portal->in_room->zone->world; dest; dest = dest->next)
        if(dest != portal->in_room && dest->sector_type != SECT_INSIDE) // Not indoors.
            num_dests++;

    for (dest = portal->in_room->zone->world; dest; dest = dest->next)
        if(dest != portal->in_room && dest->sector_type != SECT_INSIDE) // Not indoors.
            if (!number(0, --num_dests))
                break;
    if( dest != NULL ) {
        // Only immortals see this.
        act("$p disappears suddenly.",FALSE, 0, portal, 0, TO_ROOM);
    	obj_from_room(portal);
    	obj_to_room(portal, dest);
        act("$p appears suddenly.",FALSE, 0, portal, 0, TO_ROOM);
    }
	return 1;
}
