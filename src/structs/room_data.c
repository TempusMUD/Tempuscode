#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "structs.h"
#include "room_data.h"
#include "prog.h"

void
free_room(struct room_data *room)
{
    free(room->name);
    free(room->description);
    free(room->sounds);
    free(room->prog);
    free(room->progobj);
    prog_state_free(room->prog_state);
    struct extra_descr_data *exd, *next_exd;
    for (exd = room->ex_description;exd;exd = next_exd) {
        next_exd = exd->next;
        free(exd->keyword);
        free(exd->description);
        free(exd);
    }

}

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
