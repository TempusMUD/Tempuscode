#define __char_data_cc__

#include "structs.h"

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}

int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getCarriedWeight() + mod_weight ) );
}

short char_player_data::modifyWeight( short mod_weight ) {
    return setWeight( getWeight() + mod_weight );
}



#undef __char_data_cc__
