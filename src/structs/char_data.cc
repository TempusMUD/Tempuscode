#define __char_data_cc__

#include "structs.h"

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}

int char_data::setCarriedWeight( int new_weight ) {
    return ( char_specials.setCarriedWeight( new_weight ) );
}

int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getCarriedWeight() + mod_weight ) );
}

int char_data::setWornWeight( int new_weight ) {
    return ( char_specials.setWornWeight( new_weight ) );
}

int char_special_data::setCarriedWeight( int new_weight ) {
    return ( carry_weight = new_weight );
}

int char_special_data::setWornWeight( int new_weight ) {
    return ( worn_weight = new_weight );
}




#undef __char_data_cc__
