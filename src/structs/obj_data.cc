#define __obj_data_cc__

#include "structs.h"
#include "utils.h"

int obj_data::modifyWeight( int mod_weight ) {

    // if object is inside another object, recursively
    // go up and modify it
    if ( in_obj )
	in_obj->modifyWeight( mod_weight );

    else if ( worn_by ) {
	// implant, increase character weight
	if ( GET_IMPLANT( worn_by, worn_on ) == this )
	    worn_by->modifyWeight( mod_weight );
	// simply worn, increase character worn weight
	else 
	    worn_by->modifyWornWeight( mod_weight );
    }
    
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
