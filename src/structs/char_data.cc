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
1. from update_pos
2. from perform violence
*/
bool char_data::setPosition( int new_pos, int mode=0 ){
    if(new_pos == char_specials.getPosition())
        return false;
    if(new_pos < BOTTOM_POS || new_pos > TOP_POS)
        return false;
    if(IS_AFFECTED_2(this,AFF2_PETRIFIED) && new_pos > char_specials.getPosition())
        return false;
        /*
    // If they're standing up in update_pos, printf the name and the wait.
    if(mode == 1) {
            fprintf(stderr,"Update_POS: %s going to pos %d, from pos %d with wait %d\r\n",
                GET_NAME(this),new_pos, getPosition(),CHECK_WAIT(this));
    } else if(mode == 2) {
            fprintf(stderr,"Perform_Violence: %s going to pos %d, from pos %d with wait %d\r\n",
                GET_NAME(this),new_pos, getPosition(),CHECK_WAIT(this));
    } else {
    fprintf(stderr,"Default : %s going to pos %d, from pos %d with wait %d\r\n",
        GET_NAME(this),new_pos, getPosition(),CHECK_WAIT(this));
    }
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
    if(new_pos == POS_STANDING && FIGHTING(this))
        char_specials.setPosition(POS_FIGHTING);
    else
        char_specials.setPosition(new_pos);
    return true;
}
int char_data::getPosition( void ) {
    return char_specials.getPosition();
}



#undef __char_data_cc__
