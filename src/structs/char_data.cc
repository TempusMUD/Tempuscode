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
   Do NOT pass true for "use_remort" if gen == 0
   Do NOT pass a gen < 0
   params: use_remort - Add in remort gen to factor?
            primary - Add in remort gen as a primary?
   return: a number from 1-100 based on level and params.
           (!use_remort for a level 49 returns 100)
           (use_remort + primary for level 49 gen 10 returns 100)
*/
int char_data::getLevelBonus ( bool use_remort, bool primary ) {
    int bonus = player.level + 1;

    if(! use_remort ) {// Without remort calc, simply use the mort calc in its' place.
        return 2 * bonus;
    } else {
        if(primary) { // Primary. Give full remort bonus per gen.
            return bonus + (MIN(player_specials->saved.remort_generation,10)) * 5;
        } else { // Secondary. Give miniscule remort bonus.
            return bonus + MIN(player_specials->saved.remort_generation,10);
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
    sh_int pclass = player.char_class;
    sh_int sclass = player.remort_char_class;
    sh_int gen = player_specials->saved.remort_generation;
    int pLevel = spell_info[skill].min_level[pclass];
    int sLevel = spell_info[skill].min_level[sclass];
    int spell_gen = spell_info[skill].gen[pclass];
    // Note: If a class doesn't get a skill, the SPELL_LEVEL for that skill is 50.
    //       To compensate for this, level 50+ get the full bonus of 100.

    if(player.level == 50) return 100; // Immorts get full bonus.

    // If you dont get it, you dont get jack
    // (The following is an optimized ABLE_TO_LEARN)
    if(!(((gen >= spell_gen && player.level >= sLevel ) || \
    (sclass >= 0 && player.level >= sLevel && spell_info[skill].gen[pclass] == 0))))
    //if(!ABLE_TO_LEARN(skill,this)) 
        return (getLevelBonus(false,false))/4;

    if(player_specials->saved.remort_generation == 0)  { // Mortal. Normal bonus.
        return getLevelBonus(false,false);
    } else { // Remort, check primary vs secondary
        // Primary Skill.
        if( pLevel < sLevel) {
            return getLevelBonus(true,true);
        } else { // Secondary Skill
            return getLevelBonus(true,false);
        }
    }
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
