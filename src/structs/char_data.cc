#define __char_data_cc__

#include <signal.h>
#include "structs.h"
#include "comm.h"
#include "character_list.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "help.h"
#include "paths.h"
#include "login.h"

int set_desc_state( int state,struct descriptor_data *d );

extern struct descriptor_data *descriptor_list;

bool char_data::isFighting(){
    return (char_specials.fighting != NULL);
}
char_data *char_data::getFighting() {
    return (char_specials.fighting);
}
void char_data::setFighting(char_data *ch) {
    char_specials.fighting = ch;
}

int char_data::modifyCarriedWeight( int mod_weight ) {
    return ( setCarriedWeight( getCarriedWeight() + mod_weight ) );
}
int char_data::modifyWornWeight( int mod_weight ) {
    return ( setWornWeight( getWornWeight() + mod_weight ) );
}
short char_player_data::modifyWeight( short mod_weight ) {
    return setWeight( getWeight() + mod_weight );
}
int char_data::getSpeed( void ) {
    // if(IS_NPC(this))
    if(char_specials.saved.act & MOB_ISNPC)
        return 0;
    return (int)player_specials->saved.speed;
}
void char_data::setSpeed( int speed ) {
    // if(IS_NPC(this))
    if(char_specials.saved.act & MOB_ISNPC)
        return;
    speed = MAX(speed, 0);
    speed = MIN(speed, 100);
    player_specials->saved.speed = (char)(speed);
}
bool char_data::isNewbie() {
    if(char_specials.saved.act & MOB_ISNPC)
        return false;
    if((player_specials->saved.remort_generation) > 0)
        return false;
    if(player.level > 40)
        return false;
    return true;
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
    struct char_data *ch = this;
    struct affected_type *af = NULL;
    float dam_reduction = 0;
    
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch)) {
        if (lunar_stage == MOON_FULL) // full moon gives protection up to 30%
            dam_reduction += GET_ALIGNMENT(ch) / 30;
        else // good clerics get an alignment-based protection, up to 10%
            dam_reduction += GET_ALIGNMENT(ch) / 100;
    }
  
    //*************************** Sanctuary ****************************
    //******************************************************************
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

   //***************************** Oblivity ****************************
   //*******************************************************************
    // damage reduction ranges up to about 35%
    if (IS_AFFECTED_2(ch, AFF2_OBLIVITY)) {
        if (IS_NEUTRAL(ch) && CHECK_SKILL(ch, ZEN_OBLIVITY) > 60) {
            dam_reduction += (((GET_LEVEL(ch) + 
                               ch->getLevelBonus(ZEN_OBLIVITY)) * 10) +
                             (1000 - abs(GET_ALIGNMENT(ch))) + 
                               (CHECK_SKILL(ch, ZEN_OBLIVITY) * 10)) / 100;
        }
    } 
    
    //**************************** No Pain *****************************
    //******************************************************************
    if (IS_AFFECTED(ch, AFF_NOPAIN)) {
        dam_reduction += 25;         
    } 
    
    //**************************** Berserk *****************************
    //******************************************************************
    if (IS_AFFECTED_2(ch, AFF2_BESERK)) {
        if (IS_BARB(ch))
            dam_reduction += (ch->getLevelBonus(SKILL_BESERK)) / 6;
        else
            dam_reduction += 7;
    } 
    
    //************************** Damage Control ************************
    //******************************************************************
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL)) {
        dam_reduction += (ch->getLevelBonus(SKILL_DAMAGE_CONTROL)) / 5;
    }
    
    //**************************** ALCOHOLICS!!! ***********************
    //******************************************************************
    if (GET_COND(ch, DRUNK) > 5)
        dam_reduction += GET_COND(ch, DRUNK);
    
    //********************** Shield of Righteousness *******************
    //******************************************************************
    if ((af = affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS)) && 
        IS_GOOD(ch) && !IS_NPC(ch)) {
        if (af->modifier == GET_IDNUM(ch)) {
            dam_reduction += (ch->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                   + (GET_ALIGNMENT(ch) / 100);
        } else if( ch->in_room != NULL ) {
            
            CharacterList::iterator it = ch->in_room->people.begin();
            for( ; it != ch->in_room->people.end(); ++it ) {
                if (IS_NPC((*it)) && af->modifier == (short int)-MOB_IDNUM((*it))) {
                    dam_reduction += ((*it)->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                           + (GET_ALIGNMENT(ch) / 100);
                    break;
                } 
                else if (!IS_NPC((*it)) && af->modifier == GET_IDNUM((*it) )) {     
                    dam_reduction += ((*it)->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                                           + (GET_ALIGNMENT(ch) / 100); 
                    break;
                }
            }
        }
    }

    //*********************** Lattice Hardening *************************
    //*******************************************************************
    if (affected_by_spell(ch, SPELL_LATTICE_HARDENING))
        dam_reduction += (ch->getLevelBonus(SPELL_LATTICE_HARDENING)) / 6;
        
    //************** Stoneskin Barkskin Dermal Hardening ****************
    //*******************************************************************
    if (affected_by_spell(ch, SPELL_STONESKIN)) 
        dam_reduction += (ch->getLevelBonus(SPELL_STONESKIN)) / 4;
    
    else if (affected_by_spell(ch, SPELL_BARKSKIN))
        dam_reduction += (ch->getLevelBonus(SPELL_BARKSKIN)) / 6;
    
    else if (affected_by_spell(ch, SPELL_DERMAL_HARDENING))
        dam_reduction += (ch->getLevelBonus(SPELL_DERMAL_HARDENING)) / 6; 
        
    //************************** Petrification **************************
    //*******************************************************************
    if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
        dam_reduction += 75;

            
    if (attacker) {    
        ///****************** Various forms of protection ***************
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

//
// Compute level bonus factor.
// Currently, a gen 4 level 49 secondary should equate to level 49 mort primary.
//   
//   params: primary - Add in remort gen as a primary?
//   return: a number from 1-100 based on level and primary/secondary)

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
//
// Compute level bonus factor.
// Should be used for a particular skill in general.
// Returns 50 for max mort, 100 for max remort.
// params: skill - the skill # to check bonus for.
// return: a number from 1-100 based on level/gen/can learn skill.

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

//
//  attempts to set character's position.
//
//  return success or failure
//  @param mode: 1 == from update_pos, 2 == from perform violence (not used for anything really)
//  @param new_position the enumerated int position to be set to.

bool char_data::setPosition( int new_pos, int mode=0 ){
    if(new_pos == char_specials.getPosition())
        return false;
    if(new_pos < BOTTOM_POS || new_pos > TOP_POS)
        return false;
    // Petrified
    if(IS_AFFECTED_2(this,AFF2_PETRIFIED) ){
        // Stoners can stop fighting
        if( char_specials.getPosition() == POS_FIGHTING && new_pos == POS_STANDING )
            return true;
        if( new_pos > char_specials.getPosition())
            return false;
    }
    if(new_pos == POS_STANDING && FIGHTING(this)) {
        char_specials.setPosition(POS_FIGHTING);
    } else {
        char_specials.setPosition(new_pos);
    }
    return true;
}
//
// Returns current position (standing sitting etc.)

int char_data::getPosition( void ) {
    return char_specials.getPosition();
}
// Extract a ch completely from the world, and leave his stuff behind
// mode = 0 -> menu, 1 -> afterlife, 2 -> remort reroll
void char_data::extract( char mode ) {
    void stop_fighting( struct char_data * ch );
    struct char_data *k;
    struct descriptor_data *t_desc;
    struct obj_data *obj;
    int i, freed = 0;
    ACMD(do_return);

    void die_follower(struct char_data * ch);

    if (!IS_NPC(this) && !desc) {
        for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
            if (t_desc->original == this)
                do_return(t_desc->character, "", 0, SCMD_FORCED);
    }
    if (in_room == NULL) {
        slog("SYSERR: NOWHERE extracting char. (handler.c, extract_char)");
        sprintf(buf, "...extract char = %s", GET_NAME(this));
        slog(buf);
        exit(1);
    }

    if (followers || master)
        die_follower(this);

    // remove hunters
    CharacterList::iterator cit = characterList.begin();
    for( ; cit != characterList.end(); ++cit ) {
        if (this == HUNTING((*cit)))
            HUNTING((*cit)) = NULL;
    }
    // Make sure they aren't editing a help topic.
    if(GET_OLC_HELP(this)) {
        GET_OLC_HELP(this)->editor = NULL;
        GET_OLC_HELP(this) = NULL;
    }
    // Forget snooping, if applicable
    if (desc) {
        if (desc->snooping) {
            desc->snooping->snoop_by = NULL;
            desc->snooping = NULL;
        }
        if (desc->snoop_by) {
            SEND_TO_Q("Your victim is no longer among us.\r\n",
                      desc->snoop_by);
            desc->snoop_by->snooping = NULL;
            desc->snoop_by = NULL;
        }
    }
    // transfer objects to room, if any
    while (carrying) {
        obj = carrying;
        obj_from_char(obj);
        obj_to_room(obj, in_room);
    }

    // transfer equipment to room, if any
    for (i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(this, i))
            obj_to_room(unequip_char(this, i, MODE_EQ), in_room);
        if (GET_IMPLANT(this, i))
            obj_to_room(unequip_char(this, i, MODE_IMPLANT), in_room);
    }

	// transfer gold to room
	if ( GET_GOLD(this) > 0 )
		obj_to_room(create_money(GET_GOLD(this), 0), in_room);
	if ( GET_CASH(this) > 0 )
		obj_to_room(create_money(GET_CASH(this), 1), in_room);
	GET_GOLD(this) = GET_CASH(this) = 0;

    if (FIGHTING(this))
        stop_fighting(this);

    // stop all fighting
    CharacterList::iterator coit = combatList.begin();
    for( ; coit != combatList.end(); ++coit ) {
        if (this == FIGHTING((*coit)))
            stop_fighting(*coit);
    }

    if (MOUNTED(this)) {
        REMOVE_BIT(AFF2_FLAGS(MOUNTED(this)), AFF2_MOUNTED);
        MOUNTED(this) = NULL;
    }

    if (AFF2_FLAGGED(this, AFF2_MOUNTED)) {
        CharacterList::iterator cit = characterList.begin();
        for( ; cit != characterList.end(); ++cit ) {
            k = *cit;
            if (this == MOUNTED(k)) {
                MOUNTED(k) = NULL;
                if (k->getPosition() == POS_MOUNTED) {
                    if (k->in_room->sector_type == SECT_FLYING)
                        k->setPosition( POS_FLYING );
                    else 
                        k->setPosition( POS_STANDING );
                }
            }
        }
    }

    if (desc && desc->original) {
        do_return(this, "", 0,  SCMD_NOEXTRACT);
    }

    char_from_room(this);

    // pull the char from the list
    characterList.remove(this);
    combatList.remove(this);
    // remove any paths
    path_remove_object(this);

    if (!IS_NPC(this)) {
        save_char(this, NULL);
        Crash_delete_crashfile(this);
    } else {
        if (GET_MOB_VNUM(this) > -1)                // if mobile
            mob_specials.shared->number--;
        clearMemory();                // Only NPC's can have memory
        free_char(this);
        freed = 1;
        return;
    }

    if (desc) { // PC's have descriptors. Take care of them
        if (mode) { // Afterlife or Remort-Reroll
			set_desc_state( CON_AFTERLIFE,desc );
            desc->wait = 0;
        } else { // Menu 
			set_desc_state( CON_MENU,desc );
        }
    } else {  // if a player gets purged from within the game
        if (!freed)
            free_char(this);
    }
}
// erase ch's memory
void char_data::clearMemory() {
    memory_rec *curr, *next;

    curr = MEMORY(this);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(this) = NULL;
}

#undef __char_data_cc__
