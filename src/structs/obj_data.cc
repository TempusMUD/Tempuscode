#define __obj_data_cc__

#include "structs.h"

int obj_data::modifyWeight( int mod_weight ) {
    if ( in_obj )
	in_obj->modifyWeight( mod_weight );
    else if ( worn_by )
	worn_by->modifyWornWeight( mod_weight );
    else if ( carried_by )
	carried_by->modifyCarriedWeight( mod_weight );
    
    return ( obj_flags.setWeight( getWeight() + mod_weight ) );
}
    
int obj_data::setWeight( int new_weight ) {
    
    return ( modifyWeight( new_weight - getWeight() ) );
}    

int obj_flag_data::setWeight( int new_weight ) {
    return ( (weight = new_weight) );
}

#undef __obj_data_cc__
