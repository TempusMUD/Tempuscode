#define __char_data_cc__

#include "structs.h"
#include <signal.h>
#include "utils.h"

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}

int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getWornWeight() + mod_weight ) );
}

short char_player_data::modifyWeight( short mod_weight ) {
    return setWeight( getWeight() + mod_weight );
}

// Set position and Get position.
// Set returns success or failure
// Get returns current pos
// Modes... hrm....
/*
1. adjust for damage.
2. 
*/
bool char_data::setPosition( int new_pos, int mode=0 ){
    if(new_pos == char_specials.getPosition())
        return false;
    if(new_pos < BOTTOM_POS || new_pos > TOP_POS)
        return false;
    /*
    // If they're goin from sitting or resting to standing or fighting
    // while they have a wait state....
    if (new_pos == POS_STANDING || new_pos == POS_FIGHTING ) 
        if (char_specials.getPosition() == POS_RESTING || 
        char_specials.getPosition() == POS_SITTING )
            if( ( desc && desc->wait > 0 ) ||
            ( IS_NPC( this ) && GET_MOB_WAIT( this ) > 0 ) )
                if(FIGHTING(this))
                    raise(SIGINT);
    */
    char_specials.setPosition(new_pos);
    return true;
}
int char_data::getPosition( void ) {
    return char_specials.getPosition();
}



#undef __char_data_cc__
