#define __Creature_cc__

#include <signal.h>
#include "structs.h"
#include "comm.h"
#include "creature_list.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "help.h"
#include "paths.h"
#include "login.h"
#include "house.h"
#include "clan.h"
#include "security.h"
#include "fight.h"

void Crash_extract_norents(struct obj_data *obj);
extern struct descriptor_data *descriptor_list;
struct player_special_data dummy_mob;	/* dummy spec area for mobs         */

Creature::Creature(void)
{
	memset((char *)this, 0, sizeof(Creature));
	player_specials = new player_special_data;
	memset((char *)player_specials, 0, sizeof(player_special_data));
	clear();
}

Creature::~Creature(void)
{
	clear();
	delete player_specials;
	free(player.title);
}

bool
Creature::isFighting()
{
	return (char_specials.fighting != NULL);
}

/**
 * Returns true if this character is in the Testers access group.
**/
bool Creature::isTester(){
	return Security::isMember( this, "Testers", false );
}

// Returns this creature's account id.
long Creature::getAccountID() const { 
    if( account == NULL ) 
        return 0; 
    return account->get_idnum(); 
}

/**
 * Modifies the given experience to be appropriate for this character's
 *  level/gen and class.
 * if victim != NULL, assume that this char is fighting victim to gain
 *  experience.
 *
**/
int Creature::getPenalizedExperience( int experience, Creature *victim) 
{

	// Mobs are easily trained
	if( IS_NPC(this) )
		return experience;
	// Immortals are not
	if( getLevel() >= LVL_AMBASSADOR ) {
		return 0;
	}

	if ( victim != NULL ) {
		if( victim->getLevel() >= LVL_AMBASSADOR )
			return 0;

		// good clerics & knights penalized for killing good
		if( IS_GOOD(victim) && IS_GOOD(this) && 
			(IS_CLERIC(this) || IS_KNIGHT(this)) ) {
			experience /= 2;
		}
	}

	// Slow remorting down a little without slowing leveling completely.
	// This penalty works out to:
	// gen lvl <=15 16->39  40>
	//  1     23.3%	 33.3%  43.3%
	//  2     40.0%  50.0%  60.0%
	//  3     50.0%  60.0%  70.0%
	//  4     56.6%  66.6%  76.6%
	//  5     61.4%  71.4%  81.4%
	//  6     65.0%  75.0%  85.0%
	//  7     67.7%  77.7%  87.7%
	//  8     70.0%  80.0%  90.0%
	//  9     71.8%  81.8%  91.8%
	// 10     73.3%  83.3%  93.3%
	if( IS_REMORT(this) ) {
		float gen = MIN( 10, GET_REMORT_GEN(this) );
		float multiplier = (gen / ( gen + 2 ));

		if( getLevel() <= 15 )
			multiplier -= 0.10;
		else if( getLevel() >= 40 )
			multiplier += 0.10;

		experience -= (int)(experience * multiplier);
	}

	return experience;
}

void
Creature::setFighting(Creature * ch)
{
	char_specials.fighting = ch;
}

int
Creature::modifyCarriedWeight(int mod_weight)
{
	return (setCarriedWeight(getCarriedWeight() + mod_weight));
}

int
Creature::modifyWornWeight(int mod_weight)
{
	return (setWornWeight(getWornWeight() + mod_weight));
}

short
char_player_data::modifyWeight(short mod_weight)
{
	return setWeight(getWeight() + mod_weight);
}

int
Creature::getSpeed(void)
{
	// if(IS_NPC(this))
	if (char_specials.saved.act & MOB_ISNPC)
		return 0;
	return (int)player_specials->saved.speed;
}

void
Creature::setSpeed(int speed)
{
	// if(IS_NPC(this))
	if (char_specials.saved.act & MOB_ISNPC)
		return;
	speed = MAX(speed, 0);
	speed = MIN(speed, 100);
	player_specials->saved.speed = (char)(speed);
}

bool
Creature::isNewbie()
{
	if (char_specials.saved.act & MOB_ISNPC)
		return false;
	if ((char_specials.saved.remort_generation) > 0)
		return false;
	if (player.level > 40)
		return false;
	return true;
}

// Utility function to determine if a char should be affected by sanctuary
// on a hit by hit level... --N
bool
Creature::affBySanc(Creature *attacker)
{

	Creature *ch = this;

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
float
Creature::getDamReduction(Creature *attacker)
{
	struct Creature *ch = this;
	struct affected_type *af = NULL;
	float dam_reduction = 0;

	if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch)) {
		if (lunar_stage == MOON_FULL)	// full moon gives protection up to 30%
			dam_reduction += GET_ALIGNMENT(ch) / 30;
		else					// good clerics get an alignment-based protection, up to 10%
			dam_reduction += GET_ALIGNMENT(ch) / 100;
	}
	//*************************** Sanctuary ****************************
	//******************************************************************
	if (ch->affBySanc(attacker)) {
		if (IS_VAMPIRE(ch))
			dam_reduction += 0;
		else if (IS_CLERIC(ch) || IS_KNIGHT(ch) && !IS_NEUTRAL(ch))
			dam_reduction += 25;
		else if (GET_CLASS(ch) == CLASS_CYBORG || GET_CLASS(ch) == CLASS_PHYSIC)
			dam_reduction += 8;
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
	if (IS_AFFECTED_2(ch, AFF2_BERSERK)) {
		if (IS_BARB(ch))
			dam_reduction += (ch->getLevelBonus(SKILL_BERSERK)) / 6;
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
			dam_reduction +=
				(ch->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
				+ (GET_ALIGNMENT(ch) / 100);
		} else if (ch->in_room != NULL) {

			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (IS_NPC((*it))
					&& af->modifier == (short int)-MOB_IDNUM((*it))) {
					dam_reduction +=
						((*it)->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) /
						20)
						+ (GET_ALIGNMENT(ch) / 100);
					break;
				} else if (!IS_NPC((*it)) && af->modifier == GET_IDNUM((*it))) {
					dam_reduction +=
						((*it)->getLevelBonus(SPELL_SHIELD_OF_RIGHTEOUSNESS) /
						20)
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
	struct affected_type *taf;
	if ((taf = affected_by_spell(ch, SPELL_STONESKIN)))
		dam_reduction += (taf->level) / 4;
	else if ((taf = affected_by_spell(ch, SPELL_BARKSKIN)))
		dam_reduction += (taf->level) / 6;
	else if ((taf = affected_by_spell(ch, SPELL_DERMAL_HARDENING)))
		dam_reduction += (taf->level) / 6;

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

int
Creature::getLevelBonus(bool primary)
{
	int bonus = MIN(50, player.level + 1);
	short gen;

	if (IS_NPC(this)) {
		if ((player.remort_char_class % NUM_CLASSES) == 0) {
			gen = 0;
		} else {
			gen = (aff_abils.intel + aff_abils.str + aff_abils.wis) / 3;
			gen = MAX(0, gen - 18);
		}
	} else {
		gen = char_specials.saved.remort_generation;	// Player generation
	}

	if (gen == 0) {
		return bonus;
	} else {
		if (primary) {			// Primary. Give full remort bonus per gen.
			return bonus + (MIN(gen, 10)) * 5;
		} else {				// Secondary. Give less level bonus and less remort bonus.
			return (bonus * 3 / 4) + (MIN(gen, 10) * 3);
		}
	}
}

//
// Compute level bonus factor.
// Should be used for a particular skill in general.
// Returns 50 for max mort, 100 for max remort.
// params: skill - the skill # to check bonus for.
// return: a number from 1-100 based on level/gen/can learn skill.

int
Creature::getLevelBonus(int skill)
{

	// Immorts get full bonus. 
	if (player.level >= 50)
		return 100;
	// Irregular skill #s get 1
	if (skill > TOP_SPELL_DEFINE || skill < 0)
		return 1;

	unsigned short pclass = player.char_class % NUM_CLASSES;	// Primary class
	short sclass = player.remort_char_class % NUM_CLASSES;	// Secondary class
	short gen;
	// If a mob is an NPC, assume that it's attributes figure it's gen
	if (IS_NPC(this)) {
		if (sclass == 0) {
			gen = 0;
		} else {
			gen = (aff_abils.intel + aff_abils.str + aff_abils.wis) / 3;
			gen = MAX(0, gen - 18);
		}
	} else {
		gen = char_specials.saved.remort_generation;	// Player generation
	}
	short pLevel = spell_info[skill].min_level[pclass];	// Level primary class gets "skill"
	short sLevel;				// Level secondary class gets "skill"
	bool primary;				// whether skill is learnable by primary class. (false == secondary)
	short spell_gen = spell_info[skill].gen[pclass];	// gen primary class gets "skill"
	// Note: If a class doesn't get a skill, the SPELL_LEVEL for that skill is 50.
	//       To compensate for this, level 50+ get the full bonus of 100.

	// Level secondary class gets "skill"
	if (sclass > 0)				// mod'd by NUM_CLASSES to avoid overflow
		sLevel = spell_info[skill].min_level[sclass];
	else
		sLevel = 50;			// If the secondary class is UNDEFINED (-1) 
	// set sLevel to 50 to avoid array underflow

	// is is primary or secondary
	// if neither, *SPLAT*
	if (pLevel < 50) {			// primary gets skill
		primary = true;
	// secondary gets skill or wierdo mob class
	} else if( sLevel < 50 || (IS_NPC(this) && player.char_class > NUM_CLASSES ) ) {	
		primary = false;
	} else {					// Dont get the skill at all
		return (getLevelBonus(false)) / 2;
	}
	// if its a primary skill and you're too low a gen
	// or its a remort skill of your secondary class
	if (primary && gen < spell_gen || spell_info[skill].gen[sclass] > 0)
		return (getLevelBonus(false)) / 2;

	return getLevelBonus(primary);
}

/**
 *  attempts to set character's position.
 *
 *  @return success or failure
 *  @param new_position the enumerated int position to be set to.
 *  @param mode: 1 == from update_pos;
 *  			 2 == from perform violence;
 *  			 NOTE: Previously used for debugging. No longer used.
**/
bool
Creature::setPosition(int new_pos, int mode)
{
	if (new_pos == char_specials.getPosition())
		return false;
	if (new_pos < BOTTOM_POS || new_pos > TOP_POS)
		return false;
	// Petrified
	if (IS_AFFECTED_2(this, AFF2_PETRIFIED)) {
		// Stoners can stop fighting
		if (char_specials.getPosition() == POS_FIGHTING && new_pos == POS_STANDING ) {
			char_specials.setPosition(new_pos);
			return true;
		} else if (new_pos > char_specials.getPosition()) {
			return false;
		}
	}
	if (new_pos == POS_STANDING && FIGHTING(this)) {
		char_specials.setPosition(POS_FIGHTING);
	} else {
		char_specials.setPosition(new_pos);
	}
	return true;
}

/**
 * Extracts all unrentable objects worn on or carried by this creature.
**/
void
Creature::extractUnrentables()
{
    for (int j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(this, j))
			Crash_extract_norents(GET_EQ(this, j));

	Crash_extract_norents(carrying);
}


/**
 * Extract a ch completely from the world, and leave his stuff behind
 * 
 * @param destroy_objs if false, will dump all objects into room.  
 * 					   if true,  will destroy all the objects.
 * @param save if true and IS_PC(this), will save the character after 
 * 			   removing everything
 * @param con_state the connection state to change the descriptor to, if one exists
**/
void
Creature::extract(bool destroy_objs, bool save, cxn_state con_state)
{
	ACMD(do_return);
	void stop_fighting(struct Creature *ch);
	void die_follower(struct Creature *ch);

	struct obj_data* obj;
	struct descriptor_data* t_desc;
	int idx;
	CreatureList::iterator cit;

	if (!IS_NPC(this) && !desc) {
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
			if (t_desc->original == this)
				do_return(t_desc->creature, "", 0, SCMD_FORCED, 0);
	}
	if (in_room == NULL) {
		slog("SYSERR: NOWHERE extracting char. (handler.c, extract_char)");
		slog("...extract char = %s", GET_NAME(this));
		exit(1);
	}

	if (followers || master)
		die_follower(this);


	if (FIGHTING(this))
		stop_fighting(this);

	// remove hunters and mounters
	for (cit = characterList.begin(); cit != characterList.end(); ++cit) {
		if (this == HUNTING((*cit)))
			HUNTING((*cit)) = NULL;
		if (this == MOUNTED((*cit))) {
			MOUNTED((*cit)) = NULL;
			if ((*cit)->getPosition() == POS_MOUNTED) {
				if ((*cit)->in_room->sector_type == SECT_FLYING)
					(*cit)->setPosition(POS_FLYING);
				else
					(*cit)->setPosition(POS_STANDING);
			}
		}
	}


	if (MOUNTED(this)) {
		REMOVE_BIT(AFF2_FLAGS(MOUNTED(this)), AFF2_MOUNTED);
		MOUNTED(this) = NULL;
	}

	// Make sure they aren't editing a help topic.
	if (GET_OLC_HELP(this)) {
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
	if (destroy_objs) {
		// destroy all that equipment
		for (idx = 0; idx < NUM_WEARS; idx++) {
			if (GET_EQ(this, idx))
				extract_obj(unequip_char(this, idx, MODE_EQ, true));
			if (GET_IMPLANT(this, idx))
				extract_obj(unequip_char(this, idx, MODE_IMPLANT, true));
		}
		// transfer inventory to room, if any
		while (carrying) {
			obj = carrying;
			obj_from_char(obj);
			extract_obj(obj);
		}
		// gold and cash aren't actually objects, so we just let them
		// dissipate in a mist of deallocated bits
	} else {
		// transfer equipment to room, if any
		for (idx = 0; idx < NUM_WEARS; idx++) {
			if (GET_EQ(this, idx))
				obj_to_room(unequip_char(this, idx, MODE_EQ, true), in_room);
			if (GET_IMPLANT(this, idx))
				obj_to_room(unequip_char(this, idx, MODE_IMPLANT, true), in_room);
		}

		// transfer inventory to room, if any
		while (carrying) {
			obj = carrying;
			obj_from_char(obj);
			obj_to_room(obj, in_room);
		}
/*
		// transfer gold to room
		if (GET_GOLD(this) > 0)
			obj_to_room(create_money(GET_GOLD(this), 0), in_room);
		if (GET_CASH(this) > 0)
			obj_to_room(create_money(GET_CASH(this), 1), in_room);
		GET_GOLD(this) = GET_CASH(this) = 0;
*/
	}

	if (FIGHTING(this))
		stop_fighting(this);

	// stop all fighting
	for (cit = combatList.begin(); cit != combatList.end(); ++cit) {
		if (this == FIGHTING((*cit)))
			stop_fighting(*cit);
	}


	if (IS_PC(this) && save) {
		this->player.time.logon = time(0);
		this->crashSave();
	}
	if (desc && desc->original) {
		do_return(this, "", 0, SCMD_NOEXTRACT, 0);
	}

	char_from_room(this,false);

	// pull the char from the list
	characterList.remove(this);
	combatList.remove(this);
	// remove any paths
	path_remove_object(this);

	if (IS_NPC(this)) {
		if (GET_MOB_VNUM(this) > -1)	// if mobile
			mob_specials.shared->number--;
		clearMemory();			// Only NPC's can have memory
		delete this;
		return;
	}

	if (desc) {					// PC's have descriptors. Take care of them
		set_desc_state(con_state, desc);
	} else {					// if a player gets purged from within the game
		delete this;
	}
}

// erase ch's memory
void
Creature::clearMemory()
{
	memory_rec *curr, *next;

	curr = MEMORY(this);

	while (curr) {
		next = curr->next;
		free(curr);
		curr = next;
	}

	MEMORY(this) = NULL;
}


// Retrieves the characters appropriate loadroom.
room_data *Creature::getLoadroom() {
    room_data *load_room = NULL;

	if (PLR_FLAGGED(this, PLR_FROZEN)) {
		load_room = r_frozen_start_room;
	} else if (PLR_FLAGGED(this, PLR_LOADROOM)) {
		if ((load_room = real_room(GET_LOADROOM(this))) &&
			(!House_can_enter(this, load_room->number) ||
			!clan_house_can_enter(this, load_room))) 
		{
			load_room = NULL;
		}
	}

	if( load_room != NULL )
		return load_room;

	
	if ( GET_LEVEL(this) >= LVL_AMBASSADOR ) {
		load_room = r_immort_start_room;
	} else {
		if( GET_HOME(this) == HOME_NEWBIE_SCHOOL ) {
			if (GET_LEVEL(this) > 5) {
				population_record[HOME_NEWBIE_SCHOOL]--;
				GET_HOME(this) = HOME_MODRIAN;
				population_record[HOME_MODRIAN]--;
				load_room = r_mortal_start_room;
			} else {
				load_room = r_newbie_school_start_room;
			}
		} else if (GET_HOME(this) == HOME_ELECTRO) {
			load_room = r_electro_start_room;
		} else if (GET_HOME(this) == HOME_NEWBIE_TOWER) {
			if (GET_LEVEL(this) > 5) {
				population_record[HOME_NEWBIE_TOWER]--;
				GET_HOME(this) = HOME_MODRIAN;
				population_record[HOME_MODRIAN]--;
				load_room = r_mortal_start_room;
			} else
				load_room = r_tower_modrian_start_room;
		} else if (GET_HOME(this) == HOME_NEW_THALOS) {
			load_room = r_new_thalos_start_room;
		} else if (GET_HOME(this) == HOME_KROMGUARD) {
			load_room = r_kromguard_start_room;
		} else if (GET_HOME(this) == HOME_ELVEN_VILLAGE){
			load_room = r_elven_start_room;
		} else if (GET_HOME(this) == HOME_ISTAN){
			load_room = r_istan_start_room;
		} else if (GET_HOME(this) == HOME_ARENA){
			load_room = r_arena_start_room;
		} else if (GET_HOME(this) == HOME_MONK){
			load_room = r_monk_start_room;
		} else if (GET_HOME(this) == HOME_SKULLPORT_NEWBIE) {
			load_room = r_skullport_newbie_start_room;
		} else if (GET_HOME(this) == HOME_SOLACE_COVE){
			load_room = r_solace_start_room;
		} else if (GET_HOME(this) == HOME_MAVERNAL){
			load_room = r_mavernal_start_room;
		} else if (GET_HOME(this) == HOME_DWARVEN_CAVERNS){
			load_room = r_dwarven_caverns_start_room;
		} else if (GET_HOME(this) == HOME_HUMAN_SQUARE){
			load_room = r_human_square_start_room;
		} else if (GET_HOME(this) == HOME_SKULLPORT){
			load_room = r_skullport_start_room;
		} else if (GET_HOME(this) == HOME_DROW_ISLE){
			load_room = r_drow_isle_start_room;
		} else if (GET_HOME(this) == HOME_ASTRAL_MANSE) {
			load_room = r_astral_manse_start_room;
		// zul dane
		} else if (GET_HOME(this) == HOME_ZUL_DANE) {
			// newbie start room for zul dane
			if (GET_LEVEL(this) > 5)
				load_room = r_zul_dane_newbie_start_room;
			else
				load_room = r_zul_dane_start_room;
		} else {
			load_room = r_mortal_start_room;
		}
	}
    return load_room;
}

// Free all structures and return to a virginal state
void
Creature::clear(void)
{
	struct Creature *tmp_mob;
	struct alias_data *a;

	void free_alias(struct alias_data *a);

	//
	// first make sure the char is no longer in the world
	//
	if (this->in_room != NULL || this->carrying != NULL ||
		this->getFighting() != NULL || this->followers != NULL
		|| this->master != NULL) {
		slog("SYSERR: attempted clear of creature who is still connected to the world.");
		raise(SIGSEGV);
	}

	//
	// first remove and free all alieases
	//

	while ((a = GET_ALIASES(this)) != NULL) {
		GET_ALIASES(this) = (GET_ALIASES(this))->next;
		free_alias(a);
	}

	//
	// now remove all affects
	//

	while (this->affected)
		affect_remove(this, this->affected);


	//
	// free mob strings:
	// free strings only if the string is not pointing at proto
	//
	if (GET_MOB_VNUM(this) > -1) {

		tmp_mob = real_mobile_proto(GET_MOB_VNUM(this));

		if (this->player.name && this->player.name != tmp_mob->player.name)
			free(this->player.name);
		if (this->player.title && this->player.title != tmp_mob->player.title)
			free(this->player.title);
		if (this->player.short_descr &&
			this->player.short_descr != tmp_mob->player.short_descr)
			free(this->player.short_descr);
		if (this->player.long_descr &&
			this->player.long_descr != tmp_mob->player.long_descr)
			free(this->player.long_descr);
		if (this->player.description &&
			this->player.description != tmp_mob->player.description)
			free(this->player.description);
		if (this->mob_specials.mug)
			free(this->mob_specials.mug);
	} else {
		//
		// otherwise this is a player, so free all
		//

		if (GET_NAME(this))
			free(GET_NAME(this));
		if (this->player.title)
			free(this->player.title);
		if (this->player.short_descr)
			free(this->player.short_descr);
		if (this->player.long_descr)
			free(this->player.long_descr);
		if (this->player.description)
			free(this->player.description);
	}

	//
	// remove player_specials:
	// - poofin
	// - poofout
	//

	if (this->player_specials != NULL && this->player_specials != &dummy_mob) {
		if (this->player_specials->poofin)
			free(this->player_specials->poofin);
		if (this->player_specials->poofout)
			free(this->player_specials->poofout);

		delete this->player_specials;

		if (IS_NPC(this)) {
			slog("SYSERR: Mob had player_specials allocated!");
			raise(SIGSEGV);
		}
	}

	// At this point, everything should be freed, so we null the entire
	// structure just to be sure
	memset((char *)this, 0, sizeof(Creature));

	// And we reset all the values to their initial settings
	this->setPosition(POS_STANDING);
	GET_REMORT_CLASS(this) = -1;

	GET_AC(this) = 100;			/* Basic Armor */
	if (this->points.max_mana < 100)
		this->points.max_mana = 100;

	player_specials = new player_special_data;
	memset((char *)player_specials, 0, sizeof(player_special_data));

	set_title(this, "");
}

#undef __Creature_cc__
