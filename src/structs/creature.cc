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
#include "player_table.h"

void extract_norents(struct obj_data *obj);
extern struct descriptor_data *descriptor_list;
struct player_special_data dummy_mob;	/* dummy spec area for mobs         */

Creature::Creature(bool pc)
{
	memset((char *)this, 0, sizeof(Creature));
	if (pc) {
		player_specials = new player_special_data;
		memset((char *)player_specials, 0, sizeof(player_special_data));
	} else {
		player_specials = &dummy_mob;
		SET_BIT(MOB_FLAGS(this), MOB_ISNPC);
	}
	clear();
}

Creature::~Creature(void)
{
	clear();
	if (player_specials != &dummy_mob) {
		delete player_specials;
		free(player.title);
	}
}

bool
Creature::isFighting()
{
	return (char_specials.fighting != NULL);
}

void
Creature::checkPosition(void)
{
	if (GET_HIT(this) > 0) {
		if (getPosition() < POS_STUNNED)
			setPosition(POS_RESTING);
		return;
	}

	if (GET_HIT(this) > -3)
		setPosition(POS_STUNNED);
	else if (GET_HIT(this) > -6)
		setPosition(POS_INCAP);
	else
		setPosition(POS_MORTALLYW);
}

/**
 * Returns true if this character is in the Testers access group.
**/
bool Creature::isTester(){
	if( IS_NPC(this) )
		return false;
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
	if (ch == this) {
		slog("SYSERR: Attempt to make %s fight itself!", GET_NAME(this));
		raise(SIGSEGV);
		return;
	}
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
	if (player.level > 24)
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
		// good clerics get an alignment-based protection, up to 30% in the
		// full moon, up to 10% otherwise
		if (get_lunar_phase(lunar_day) == MOON_FULL)
			dam_reduction += GET_ALIGNMENT(ch) / 30;
		else
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
	short gen = char_specials.saved.remort_generation;

	if( gen == 0 && IS_NPC(this) ) {
		if ((player.remort_char_class % NUM_CLASSES) == 0) {
			gen = 0;
		} else {
			gen = (aff_abils.intel + aff_abils.str + aff_abils.wis) / 3;
			gen = MAX(0, gen - 18);
		}
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
	if( player.level >= 50 )
		return 100;

	// Irregular skill #s get 1
	if( skill > TOP_SPELL_DEFINE || skill < 0 )
		return 1;

	if( IS_NPC(this) && GET_CLASS(this) >= NUM_CLASSES ) {
		// Check to make sure they have the skill
		int skill_lvl = CHECK_SKILL(this, skill);
		if (!skill_lvl)
			return 1;
		// Average the basic level bonus and the skill level
		return MIN(100, (getLevelBonus(true) + skill_lvl) / 2);
	} else {
		int pclass = GET_CLASS(this);
		int sclass = GET_REMORT_CLASS(this);
		int spell_lvl = SPELL_LEVEL(skill, pclass);
		int spell_gen = SPELL_GEN( skill, pclass );

		if( pclass < 0 || pclass >= NUM_CLASSES )
			pclass = CLASS_WARRIOR;

		if( sclass >= NUM_CLASSES )
			sclass = CLASS_WARRIOR;

		if( spell_lvl <= player.level && spell_gen <= GET_REMORT_GEN(this) ) {
			return getLevelBonus(true);
		} else if( sclass >= 0 && SPELL_LEVEL(skill, sclass <= player.level ) && 
				   SPELL_GEN( skill, sclass ) < 0 ) {
			return getLevelBonus(false);
		} else {
			return player.level/2;
		}
	}
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
    for (int j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(this, j))
			extract_norents(GET_EQ(this, j));
		if (GET_IMPLANT(this, j))
			extract_norents(GET_EQ(this, j));
	}

	extract_norents(carrying);
}

/**
 * Extract a ch completely from the world, and destroy his stuff
 * @param con_state the connection state to change the descriptor to, if one exists
**/
void
Creature::extract(cxn_state con_state)
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

	if (FIGHTING(this))
		stop_fighting(this);

	// stop all fighting
	for (cit = combatList.begin(); cit != combatList.end(); ++cit) {
		if (this == FIGHTING((*cit)))
			stop_fighting(*cit);
	}


	if (desc && desc->original)
		do_return(this, "", 0, SCMD_NOEXTRACT, 0);

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
	} else if (GET_LOADROOM(this)) {
		if ((load_room = real_room(GET_LOADROOM(this))) &&
			(!House_can_enter(this, load_room->number) ||
			!clan_house_can_enter(this, load_room))) 
		{
			load_room = NULL;
		}
	} else if (GET_HOMEROOM(this)) {
		if ((load_room = real_room(GET_HOMEROOM(this))) &&
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
	bool is_pc;

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
	if (mob_specials.shared && GET_MOB_VNUM(this) > -1) {

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
		if (this->mob_specials.func_data)
			free(this->mob_specials.func_data);
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

	is_pc = !IS_NPC(this);

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
	GET_CLASS(this) = -1;
	GET_REMORT_CLASS(this) = -1;

	GET_AC(this) = 100;			/* Basic Armor */
	if (this->points.max_mana < 100)
		this->points.max_mana = 100;

	if (is_pc) {
		player_specials = new player_special_data;
		memset((char *)player_specials, 0, sizeof(player_special_data));
		player_specials->desc_mode = CXN_UNKNOWN;
		set_title(this, "");
	} else {
		player_specials = &dummy_mob;
		GET_TITLE(this) = NULL;
	}
}

void
Creature::restore(void)
{
	int i;

	GET_HIT(this) = GET_MAX_HIT(this);
	GET_MANA(this) = GET_MAX_MANA(this);
	GET_MOVE(this) = GET_MAX_MOVE(this);

	if (GET_COND(this, FULL) >= 0)
		GET_COND(this, FULL) = 24;
	if (GET_COND(this, THIRST) >= 0)
		GET_COND(this, THIRST) = 24;

	if ((GET_LEVEL(this) >= LVL_GRGOD)
			&& (GET_LEVEL(this) >= LVL_AMBASSADOR)) {
		for (i = 1; i <= MAX_SKILLS; i++)
			SET_SKILL(this, i, 100);

		if (GET_LEVEL(this) >= LVL_IMMORT) {
			real_abils.intel = 25;
			real_abils.wis = 25;
			real_abils.dex = 25;
			real_abils.str = 25;
			real_abils.con = 25;
			real_abils.cha = 25;
		}
		aff_abils = real_abils;
	}
	update_pos(this);
}

bool
Creature::rent(void)
{
	player_specials->rentcode = RENT_RENTED;
	player_specials->rent_per_day =
		(GET_LEVEL(this) < LVL_IMMORT) ? calc_daily_rent(this, 1, NULL, NULL):0;
	player_specials->desc_mode = CXN_UNKNOWN;
	player_specials->rent_currency = in_room->zone->time_frame;
	GET_LOADROOM(this) = in_room->number;
	player.time.logon = time(0);
	saveObjects();
	saveToXML();
	if (GET_LEVEL(this) < 50)
		mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(this)), NRM, true,
			"%s has rented (%d/day, %lld %s)", GET_NAME(this),
			player_specials->rent_per_day, CASH_MONEY(this) + BANK_MONEY(this),
			(player_specials->rent_currency == TIME_ELECTRO) ? "gold":"creds");
	extract(CXN_MENU);

	return true;
}

bool
Creature::cryo(void)
{
	player_specials->rentcode = RENT_CRYO;
	player_specials->rent_per_day = 0;
	player_specials->desc_mode = CXN_UNKNOWN;
	player_specials->rent_currency = in_room->zone->time_frame;
	GET_LOADROOM(this) = in_room->number;
	player.time.logon = time(0);
	saveObjects();
	saveToXML();
	
	mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(this)), NRM, true,
		"%s has cryo-rented", GET_NAME(this));
	extract(CXN_MENU);
	return true;
}

bool
Creature::quit(void)
{
	obj_data *obj, *next_obj, *next_contained_obj;
	int pos;

	if (IS_NPC(this))
		return false;

	for (pos = 0;pos < NUM_WEARS;pos++) {
		// Drop all non-cursed equipment worn
		if (GET_EQ(this, pos)) {
			obj = GET_EQ(this, pos);
			if (IS_OBJ_STAT(obj, ITEM_NODROP) ||
					IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) {
				for (obj = obj->contains;obj;obj = next_contained_obj) {
					next_contained_obj = obj->next_content;
					obj_from_obj(obj);
					obj_to_room(obj, in_room);
				}
			} else {
				obj = unequip_char(this, pos, MODE_EQ);
				obj_to_room(obj, in_room);
			}
		}

		// Drop all implanted items, breaking them
		if (GET_IMPLANT(this, pos)) {
			obj = unequip_char(this, pos, MODE_IMPLANT);
			GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj) >> 3 - 1;
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_BROKEN);
			obj_to_room(obj, in_room);
			act("$p drops to the ground!", false, 0, obj, 0, TO_ROOM);
		}
	}

	// Drop all uncursed inventory items
	for (obj = carrying;obj;obj = next_obj) {
		next_obj = obj->next_content;
		if (IS_OBJ_STAT(obj, ITEM_NODROP) ||
				IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) {
			for (obj = obj->contains;obj;obj = next_contained_obj) {
				next_contained_obj = obj->next_content;
				obj_from_obj(obj);
				obj_to_room(obj, in_room);
			}
		} else {
			obj_from_char(obj);
			obj_to_room(obj, in_room);
		}
	}

	player_specials->rentcode = RENT_QUIT;
	player_specials->rent_per_day = calc_daily_rent(this, 3, NULL, NULL);
	player_specials->desc_mode = CXN_UNKNOWN;
	player_specials->rent_currency = in_room->zone->time_frame;
	GET_LOADROOM(this) = 0;
	player.time.logon = time(0);
	saveObjects();
	saveToXML();
	extract(CXN_MENU);

	return true;
}

bool
Creature::idle(void)
{
	if (IS_NPC(this))
		return false;
	player_specials->rentcode = RENT_FORCED;
	player_specials->rent_per_day = calc_daily_rent(this, 3, NULL, NULL);
	player_specials->desc_mode = CXN_UNKNOWN;
	player_specials->rent_currency = in_room->zone->time_frame;
	GET_LOADROOM(this) = 0;
	player.time.logon = time(0);
	saveObjects();
	saveToXML();

	mudlog(LVL_GOD, CMP, true, "%s force-rented and extracted (idle).",
		GET_NAME(this));

	extract(CXN_MENU);
	return true;
}

bool
Creature::die(void)
{
	obj_data *obj, *next_obj;
	int pos;

	// If their stuff hasn't been moved out, they dt'd, so we need to dump
	// their stuff to the room
	for (pos = 0;pos < NUM_WEARS;pos++) {
		if (GET_EQ(this, pos)) {
			obj = unequip_char(this, pos, MODE_EQ);
			obj_to_room(obj, in_room);
		}
		if (GET_IMPLANT(this, pos)) {
			obj = unequip_char(this, pos, MODE_IMPLANT);
			obj_to_room(obj, in_room);
		}
	}
	for (obj = carrying;obj;obj = next_obj) {
		next_obj = obj->next_content;
		obj_from_char(obj);
		obj_to_room(obj, in_room);
	}
	
	if (!IS_NPC(this)) {
		player_specials->rentcode = RENT_QUIT;
		player_specials->rent_per_day = 0;
		player_specials->desc_mode = CXN_AFTERLIFE;
		player_specials->rent_currency = 0;
		GET_LOADROOM(this) = in_room->zone->respawn_pt;
		player.time.logon = time(0);
		saveObjects();
		saveToXML();
	}
	extract(CXN_AFTERLIFE);
	return true;
}

bool
Creature::arena_die(void)
{
	// Rent them out
	if (!IS_NPC(this)) {
		player_specials->rentcode = RENT_RENTED;
		player_specials->rent_per_day =
			(GET_LEVEL(this) < LVL_IMMORT) ? calc_daily_rent(this, 1, NULL, NULL):0;
		player_specials->desc_mode = CXN_UNKNOWN;
		player_specials->rent_currency = in_room->zone->time_frame;
		GET_LOADROOM(this) = in_room->zone->respawn_pt;
		player.time.logon = time(0);
		saveObjects();
		saveToXML();
		if (GET_LEVEL(this) < 50)
			mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(this)), NRM, true,
				"%s has died in arena (%d/day, %lld %s)", GET_NAME(this),
				player_specials->rent_per_day, CASH_MONEY(this) + BANK_MONEY(this),
				(player_specials->rent_currency == TIME_ELECTRO) ? "gold":"creds");
	}
	// But extract them to afterlife
	extract(CXN_AFTERLIFE);
	return true;
}

bool
Creature::purge(bool destroy_obj)
{
	obj_data *obj, *next_obj;

	if (!destroy_obj) {
		int pos;

		for (pos = 0;pos < NUM_WEARS;pos++) {
			if (GET_EQ(this, pos)) {
				obj = unequip_char(this, pos, MODE_EQ);
				obj_to_room(obj, in_room);
			}
			if (GET_IMPLANT(this, pos)) {
				obj = unequip_char(this, pos, MODE_IMPLANT);
				obj_to_room(obj, in_room);
			}
		}

		for (obj = carrying;obj;obj = next_obj) {
			next_obj = obj->next_content;
			obj_from_char(obj);
			obj_to_room(obj, in_room);
		}
	}

	if (!IS_NPC(this)) {
		player_specials->rentcode = RENT_QUIT;
		player_specials->rent_per_day = 0;
		player_specials->desc_mode = CXN_UNKNOWN;
		player_specials->rent_currency = 0;
		GET_LOADROOM(this) = 0;
		player.time.logon = time(0);
		saveObjects();
		saveToXML();
	}

	extract(CXN_DISCONNECT);
	return true;
}

bool
Creature::remort(void)
{
	if (IS_NPC(this))
		return false;
	player_specials->rentcode = RENT_QUIT;
	player_specials->rent_per_day = 0;
	player_specials->desc_mode = CXN_UNKNOWN;
	player_specials->rent_currency = 0;
	GET_LOADROOM(this) = 0;
	player.time.logon = time(0);
	saveObjects();
	saveToXML();
	extract(CXN_REMORT_AFTERLIFE);
	return true;
}

bool
Creature::trusts(long idnum)
{
	if (IS_NPC(this))
		return false;
	
	return account->isTrusted(idnum);
}

bool
Creature::distrusts(long idnum)
{
	return !trusts(idnum);
}

bool
Creature::trusts(Creature *ch)
{
	if (IS_NPC(this))
		return false;
	
	if (IS_AFFECTED(this, AFF_CHARM) && master == ch)
		return true;

	return trusts(GET_IDNUM(ch));
}

bool
Creature::distrusts(Creature *ch)
{
	return !trusts(ch);
}

#undef __Creature_cc__
