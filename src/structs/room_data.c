#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include "room_data.h"

bool
room_is_open_air(struct room_data *room)
{

	//
	// sector types must not only be added here, but also in
	// act.movement.cc can_travel_sector()
	//

	if (room->sector_type == SECT_FLYING ||
		room->sector_type == SECT_ELEMENTAL_AIR ||
		room->sector_type == SECT_ELEMENTAL_RADIANCE ||
		room->sector_type == SECT_ELEMENTAL_LIGHTNING ||
		room->sector_type == SECT_ELEMENTAL_VACUUM)
		return true;

	return false;
}

int
count_room_exits(struct room_data *room)
{
	int idx, result = 0;

	for (idx = 0;idx < NUM_OF_DIRS; idx++)
		if (room->dir_option[idx] &&
            room->dir_option[idx]->to_room &&
            room->dir_option[idx]->to_room != room)
			result++;

	return result;
}
