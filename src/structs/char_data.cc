#define __char_data_cc__

#include "structs.h"
#include <signal.h>
#include "utils.h"
#include "spells.h"

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}

int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getWornWeight() + mod_weight ) );
}

short char_player_data::modifyWeight( short mod_weight ) {
    return setWeight( getWeight() + mod_weight );
}

/**
   Compute level bonus factor.
   Currently, a gen 4 level 49 secondary should equate to level 49 mort primary.
   
   params: primary - Add in remort gen as a primary?
   return: a number from 1-100 based on level and primary/secondary)
*/
int char_data::getLevelBonus ( bool primary ) {
    int bonus = MIN(50,player.level + 1);

    if(player_specials->saved.remort_generation == 0) {
        return bonus;
    } else {
        if(primary) { // Primary. Give full remort bonus per gen.
            return bonus + (MIN(player_specials->saved.remort_generation,10)) * 5;
        } else { // Secondary. Give less level bonus and less remort bonus.
            return (bonus * 3 / 4) + (MIN(player_specials->saved.remort_generation,10) * 3);
        }
    }
}
/**
   Compute level bonus factor.
   Should be used for a particular skill in general.
   Returns 50 for max mort, 100 for max remort.
   params: skill - the skill # to check bonus for.
   return: a number from 1-100 based on level/gen/can learn skill.
*/
int char_data::getLevelBonus( int skill ) {
    unsigned short pclass = player.char_class % NUM_CLASSES; // Primary class
    short sclass = player.remort_char_class % NUM_CLASSES; // Secondary class
    short gen = player_specials->saved.remort_generation; // Player generation
    short pLevel = spell_info[skill].min_level[pclass]; // Level primary class gets "skill"
    short sLevel;
    bool primary;
    short spell_gen = spell_info[skill].gen[pclass]; // gen primary class gets "skill"
    // Note: If a class doesn't get a skill, the SPELL_LEVEL for that skill is 50.
    //       To compensate for this, level 50+ get the full bonus of 100.

    // Immorts get full bonus. 
    if(player.level >= 50) return 100; 
    // Irregular skill #s get 1
    if(skill > TOP_SPELL_DEFINE || skill < 0) return 1; 

    // Level secondary class gets "skill"
    if(sclass > 0) // mod'd by NUM_CLASSES to avoid overflow
        sLevel = spell_info[skill].min_level[sclass]; 
    else 
        sLevel = 50; // If the secondary class is UNDEFINED (-1) 
                      // set sLevel to 50 to avoid array underflow

    // is is primary or secondary
    // if neither, *SPLAT*
    if(pLevel < 50) // primary < 50
        primary = true;
    else if( sLevel < 50) // secondary < 50
        primary = false;
    else                 // Dont get the skill at all
        return (getLevelBonus(false))/2;

    // if its a primary skill and you're too low a gen
    // or its a remort skill of your secondary class
    if(primary && gen < spell_gen || spell_info[skill].gen[sclass] > 0)
        return (getLevelBonus(false))/2;

    return getLevelBonus(primary);
    return 1; // This should never happen, but just in case.
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
