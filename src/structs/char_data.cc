#define __char_data_cc__

#include "structs.h"
#include <signal.h>
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "db.h"

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}
int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getWornWeight() + mod_weight ) );
}
short char_player_data::modifyWeight( short mod_weight ) {
    return setWeight( getWeight() + mod_weight );
}
// Utility function to determine if a char should be affected by sanctuary
// on a hit by hit level... --N
bool char_data::affBySanc(char_data *attacker = NULL) {

    char_data *ch = this;
    
    if (IS_AFFECTED(ch, AFF_SANCTUARY)) { 
        if (attacker && IS_EVIL(ch) && 
            affected_by_spell(attacker, SPELL_RIGHTEOUS_PENETRATION))
            return false;
        else if (attacker && IS_GOOD(ch) && 
                 affected_by_spell(attacker, SPELL_MALEFIC_VIOLATION))
            return false;
        else
            return true;
    }                           
    
    return false;                               
}

// Pass in the attacker for conditional reduction such as PROT_GOOD and 
// PROT_EVIL.  Or leave it blank for the characters base reduction --N
float char_data::getDamReduction(char_data *attacker = NULL)
{
    struct char_data *ch = this, *tmp_ch = NULL;
    struct affected_type *af = NULL;
    float dam_reduction = 0;
    
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch)) {
        if (lunar_stage == MOON_FULL) // full moon gives protection up to 30%
            dam_reduction += GET_ALIGNMENT(ch) / 30;
        else // good clerics get an alignment-based protection, up to 10%
            dam_reduction += GET_ALIGNMENT(ch) / 100;
    }
  
    /**************************** Sanctuary *****************************/
    /********************************************************************/
    if (ch->affBySanc(attacker)) {
        if (IS_VAMPIRE(ch))
            dam_reduction += 0;
        else if (GET_CLASS(ch) == CLASS_CYBORG || GET_CLASS(ch) == CLASS_PHYSIC)
            dam_reduction += 8;
        else if ((GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_KNIGHT)
                 && !IS_NEUTRAL(ch))
            dam_reduction += 25;
        else
            dam_reduction += 15;
    }

   /****************************** Oblivity *****************************/
   /*********************************************************************/
    // damage reduction ranges up to about 35%
    if (IS_AFFECTED_2(ch, AFF2_OBLIVITY)) {
        if (IS_NEUTRAL(ch) && CHECK_SKILL(ch, ZEN_OBLIVITY) > 60) {
            dam_reduction += (((GET_LEVEL(ch) + 
                               ch->getLevelBonus(ZEN_OBLIVITY)) * 10) +
                             (1000 - abs(GET_ALIGNMENT(ch))) + 
                               (CHECK_SKILL(ch, ZEN_OBLIVITY) * 10)) / 100;
        }
    } 
    
    /***************************** No Pain ******************************/
    /********************************************************************/
    if (IS_AFFECTED(ch, AFF_NOPAIN)) {
        dam_reduction += 25;         
    } 
    
    /***************************** Berserk ******************************/
    /********************************************************************/
    if (IS_AFFECTED_2(ch, AFF2_BESERK)) {
        if (IS_BARB(ch))
            dam_reduction += (ch->getLevelBonus(SKILL_BESERK)) / 5;
        else
            dam_reduction += 7;
    } 
    
    /*************************** Damage Control *************************/
    /********************************************************************/
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL)) {
        dam_reduction += (ch->getLevelBonus(SKILL_DAMAGE_CONTROL)) / 5;
    }
    
    /***************************** ALCOHOLICS!!! ************************/
    /********************************************************************/
    if (GET_COND(ch, DRUNK) > 5)
        dam_reduction += GET_COND(ch, DRUNK);
    
    /*********************** Shield of Righteousness ********************/
    /********************************************************************/
    if ((af = affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS)) && 
        IS_GOOD(ch) && !IS_NPC(ch)) {
        if (af->modifier == GET_IDNUM(ch)) {
            dam_reduction += (ch->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                   + (GET_ALIGNMENT(ch) / 100);
        } 
        else {
            for (tmp_ch = ch->in_room->people; tmp_ch; tmp_ch = tmp_ch->next_in_room) {
                if (IS_NPC(tmp_ch) && af->modifier == (short int)-MOB_IDNUM(tmp_ch)) {
                    dam_reduction += (tmp_ch->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                           + (GET_ALIGNMENT(ch) / 100);
                    break;
                } 
                else if (!IS_NPC(tmp_ch) && af->modifier == GET_IDNUM(tmp_ch )) {     
                    dam_reduction += (tmp_ch->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                           + (GET_ALIGNMENT(ch) / 100); 
                    break;
                }
            }
        }
    }

    /************************ Lattice Hardening **************************/
    /*********************************************************************/
    if (affected_by_spell(ch, SPELL_LATTICE_HARDENING))
        dam_reduction += (ch->getLevelBonus(SPELL_LATTICE_HARDENING)) / 6;
        
    /*************** Stoneskin Barkskin Dermal Hardening *****************/
    /*********************************************************************/
    if (affected_by_spell(ch, SPELL_STONESKIN)) 
        dam_reduction += (ch->getLevelBonus(SPELL_STONESKIN)) / 4;
    
    else if (affected_by_spell(ch, SPELL_BARKSKIN))
        dam_reduction += (ch->getLevelBonus(SPELL_BARKSKIN)) / 6;
    
    else if (affected_by_spell(ch, SPELL_DERMAL_HARDENING))
        dam_reduction += (ch->getLevelBonus(SPELL_DERMAL_HARDENING)) / 6; 
        
    /*************************** Petrification ***************************/
    /*********************************************************************/
    if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
        dam_reduction += 75;

            
    if (attacker) {    
        //******************* Various forms of protection ****************/
        if (IS_EVIL(attacker) && IS_AFFECTED(ch, AFF_PROTECT_EVIL)) 
            dam_reduction += 8;
        if (IS_GOOD(attacker) && IS_AFFECTED(ch, AFF_PROTECT_GOOD))
            dam_reduction += 8;
        if (IS_UNDEAD(attacker) && IS_AFFECTED_2(ch, AFF2_PROTECT_UNDEAD))
            dam_reduction += 8;
        if (IS_DEMON(attacker) && IS_AFFECTED_2(ch, AFF2_PROT_DEMONS))
            dam_reduction += 8;
        if (IS_DEVIL(attacker) && IS_AFFECTED_2(ch, AFF2_PROT_DEVILS))
            dam_reduction += 8;
    }

    dam_reduction = MIN(dam_reduction, 75);

    return (dam_reduction / 100);
}

/**
   Compute level bonus factor.
   Currently, a gen 4 level 49 secondary should equate to level 49 mort primary.
   
   params: primary - Add in remort gen as a primary?
   return: a number from 1-100 based on level and primary/secondary)
*/
int char_data::getLevelBonus ( bool primary ) {
    int bonus = MIN(50,player.level + 1);
    short gen;

    if(IS_NPC(this)) {
        if( (player.remort_char_class % NUM_CLASSES) == 0) {
            gen = 0;
        } else {
            gen = (aff_abils.intel + aff_abils.str + aff_abils.wis) / 3;
            gen = MAX( 0, gen - 18 );
       }
    } else {
        gen = player_specials->saved.remort_generation; // Player generation
    }

    if(gen == 0) {
        return bonus;
    } else {
        if(primary) { // Primary. Give full remort bonus per gen.
            return bonus + (MIN(gen,10)) * 5;
        } else { // Secondary. Give less level bonus and less remort bonus.
            return (bonus * 3 / 4) + (MIN(gen,10) * 3);
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

    // Immorts get full bonus. 
    if(player.level >= 50) return 100; 
    // Irregular skill #s get 1
    if(skill > TOP_SPELL_DEFINE || skill < 0) return 1; 

    unsigned short pclass = player.char_class % NUM_CLASSES; // Primary class
    short sclass = player.remort_char_class % NUM_CLASSES; // Secondary class
    short gen;
    // If a mob is an NPC, assume that it's attributes figure it's gen
    if(IS_NPC(this)) {
        if(sclass == 0) {
            gen = 0;
        } else {
            gen = (aff_abils.intel + aff_abils.str + aff_abils.wis) / 3;
            gen = MAX( 0, gen - 18 );
       }
    } else {
        gen = player_specials->saved.remort_generation; // Player generation
    }
    short pLevel = spell_info[skill].min_level[pclass]; // Level primary class gets "skill"
    short sLevel; // Level secondary class gets "skill"
    bool primary; // whether skill is learnable by primary class. (false == secondary)
    short spell_gen = spell_info[skill].gen[pclass]; // gen primary class gets "skill"
    // Note: If a class doesn't get a skill, the SPELL_LEVEL for that skill is 50.
    //       To compensate for this, level 50+ get the full bonus of 100.

    // Level secondary class gets "skill"
    if(sclass > 0) // mod'd by NUM_CLASSES to avoid overflow
        sLevel = spell_info[skill].min_level[sclass]; 
    else 
        sLevel = 50; // If the secondary class is UNDEFINED (-1) 
                      // set sLevel to 50 to avoid array underflow

    // is is primary or secondary
    // if neither, *SPLAT*
    if(pLevel < 50) {         // primary gets skill
        primary = true;
    } else if( sLevel < 50) { // secondary gets skill
        primary = false;
    } else {                  // Dont get the skill at all
        return (getLevelBonus(false))/2;
    }
    // if its a primary skill and you're too low a gen
    // or its a remort skill of your secondary class
    if(primary && gen < spell_gen || spell_info[skill].gen[sclass] > 0)
        return (getLevelBonus(false))/2;

    return getLevelBonus(primary);
}

/*
 *  attempts to set character's position.
 *
 *  return success or failure
 *  @param mode: 1 == from update_pos, 2 == from perform violence (not used for anything really)
 *  @param new_position the enumerated int position to be set to.
 */
bool char_data::setPosition( int new_pos, int mode=0 ){
    if(new_pos == char_specials.getPosition())
        return false;
    if(new_pos < BOTTOM_POS || new_pos > TOP_POS)
        return false;
    if(IS_AFFECTED_2(this,AFF2_PETRIFIED) && new_pos > char_specials.getPosition())
        return false;
    if(new_pos == POS_STANDING && FIGHTING(this)) {
        char_specials.setPosition(POS_FIGHTING);
    } else {
        char_specials.setPosition(new_pos);
    }
    return true;
}
/*
 * Returns current position
 */
int char_data::getPosition( void ) {
    return char_specials.getPosition();
}

#undef __char_data_cc__
