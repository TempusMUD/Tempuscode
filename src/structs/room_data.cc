#define __room_data_cc__

#include "structs.h"

// 
//
//

bool room_data::isOpenAir( void ) {
    
    //
    // sector types must not only be added here, but also in
    // act.movement.cc can_travel_sector()
    //

    if ( sector_type == SECT_FLYING ||
	 sector_type == SECT_ELEMENTAL_AIR ||
	 sector_type == SECT_ELEMENTAL_RADIANCE ||
	 sector_type == SECT_ELEMENTAL_LIGHTNING ||
	 sector_type == SECT_ELEMENTAL_VACUUM )
	return true;

    return false;
}

#undef __room_data_cc__
