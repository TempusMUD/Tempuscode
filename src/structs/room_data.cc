#define __room_data_cc__
#include "structs.h"
#include "room_data.h"
// 
//
//

bool
room_data::isOpenAir(void)
{

	//
	// sector types must not only be added here, but also in
	// act.movement.cc can_travel_sector()
	//

	if (sector_type == SECT_FLYING ||
		sector_type == SECT_ELEMENTAL_AIR ||
		sector_type == SECT_ELEMENTAL_RADIANCE ||
		sector_type == SECT_ELEMENTAL_LIGHTNING ||
		sector_type == SECT_ELEMENTAL_VACUUM)
		return true;

	return false;
}

room_data::room_data(room_num n, zone_data *z)
:	people(true)
{

	affects = NULL;
	contents = NULL;
	description = NULL;
	ex_description = NULL;
	find_first_step_index = 0;
	flow_dir = 0;
	flow_speed = 0;
	flow_type = 0;
	func = NULL;
	func_param = NULL;
	light = 0;
	max_occupancy = 256;
	name = NULL;
	next = NULL;
	number = n;
	//people = NULL;
	room_flags = 0;
	search = NULL;
	sector_type = 0;
	sounds = NULL;
	trail = NULL;
	zone = z;

	for (int i = 0; i < NUM_OF_DIRS; i++)
		dir_option[i] = NULL;
}

int
room_data::countExits(void)
{
	int idx, result = 0;

	for (idx = 0;idx < NUM_OF_DIRS; idx++)
		if (dir_option[idx] &&
				dir_option[idx]->to_room &&
				dir_option[idx]->to_room != this)
			result++;

	return result;
}


#undef __room_data_cc__
