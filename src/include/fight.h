#ifndef _FIGHT_H_
#define _FIGHT_H_

//
// File: fight.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//#define DEBUG_POSITION 1
#include "handler.h"
#include "utils.h"


#define DAM_OBJECT_IDNUM(obj) (IS_BOMB(obj) ? BOMB_IDNUM(obj) : GET_OBJ_SIGIL_IDNUM(obj))

#define IS_WEAPON(type) ((((type)>=TYPE_HIT) && ((type)<TOP_ATTACKTYPE)) || \
			 type == SKILL_SECOND_WEAPON || \
			 type == SKILL_ENERGY_WEAPONS || \
			 type == SKILL_ARCHERY        || \
             (((type) >= TYPE_EGUN_LASER) && ((type) <= TYPE_EGUN_TOP)) || \
			 type == SKILL_PROJ_WEAPONS)

#define SLASHING(weap) (GET_OBJ_VAL(weap, 3) == (TYPE_SLASH-TYPE_HIT))

#define BLOODLET_FACTOR(i) \
(i == SPELL_SPIRIT_HAMMER ? 2 :  i == SKILL_BACKSTAB ? 3 :   \
 i == SKILL_KICK ? 2 :  i == SKILL_PUNCH ? 1 :              \
 i == SKILL_PILEDRIVE ? 3 :  i == SKILL_ELBOW ? 4 :\
 i == SKILL_KNEE ? 3 : i == SKILL_BITE ? 3 :\
 i == SKILL_HEADBUTT ? 4 : i == SKILL_GOUGE ? 1 :\
 i == SKILL_BEHEAD ? 7 : i == SKILL_SPINKICK ? 4 :\
 i == SKILL_ROUNDHOUSE ? 2 : i == SKILL_SIDEKICK ? 2 :\
 i == SKILL_SPINFIST ? 3 : i == SKILL_JAB ? 1 :\
 i == SKILL_HOOK ? 2 : i == SKILL_UPPERCUT ? 4 :\
 i == SKILL_CLAW ? 6 : i == SKILL_IMPALE ? 5 :\
 i == SKILL_PELE_KICK ? 4 : i == SKILL_TORNADO_KICK ? 3 :\
 i == SKILL_WHIRLWIND ? 2 : i == SKILL_COMBO ? 2 :\
 i == SKILL_CRANE_KICK ? 4 : i == SKILL_RIDGEHAND ? 1 :\
 i == SKILL_SCISSOR_KICK ? 4 : i == TYPE_HIT ? 1 :\
 i == TYPE_STING ? 2 : i == TYPE_WHIP  ? 3 :\
 i == TYPE_SLASH ? 8 : i == TYPE_BITE  ? 5 :\
 i == TYPE_BLUDGEON ? 4 : i == TYPE_CRUSH ? 3 :\
 i == TYPE_POUND ? 3 : i == TYPE_CLAW ? 7 :\
 i == TYPE_MAUL ? 7 : i == TYPE_PIERCE ? 3 :\
 i == TYPE_BLAST ? 2 : i == TYPE_PUNCH ? 1 :\
 i == TYPE_STAB ? 3 : i == TYPE_RIP ? 8 :\
 i == TYPE_CHOP ? 7 : i == TYPE_EGUN_PARTICLE ? 3 : 0)\

#define BLOODLET(ch, dam, attacktype) \
 (MIN(BLOODLET_FACTOR(attacktype) * dam, 10000) > number(0, 20000))


#define POS_DAMAGE_OK(location) \
(location != WEAR_LIGHT && location != WEAR_ABOUT && \
 location != WEAR_HOLD  && location != WEAR_BELT &&  \
 location != WEAR_WIELD && location != WEAR_WIELD_2 && \
 location != WEAR_ASS)

#define IS_DEFENSE_ATTACK(attacktype)   (attacktype == SPELL_FIRE_SHIELD || attacktype == SPELL_BLADE_BARRIER  || attacktype == SPELL_PRISMATIC_SPHERE || attacktype == SKILL_ENERGY_FIELD || attacktype == SPELL_THORN_SKIN || attacktype == SONG_WOUNDING_WHISPERS)

static inline bool
CANNOT_DAMAGE(Creature *ch, Creature *vict, obj_data *weap, int attacktype) {
	if (IS_PC(vict) && GET_LEVEL(vict) >= LVL_AMBASSADOR &&
			!PLR_FLAGGED(vict, PLR_MORTALIZED))
		return true;

	if (NON_CORPOREAL_UNDEAD(vict) ||
			IS_RAKSHASA(vict) ||
			IS_GREATER_DEVIL(vict)) {

		// Spells can hit them too
		if (!IS_WEAPON(attacktype))
			return false;

		// Magical items can hit them
		if (IS_WEAPON(attacktype) && weap && IS_OBJ_STAT(weap, ITEM_MAGIC))
			return false;

		// bare-handed attacks with kata can hit magical stuff
		if (IS_WEAPON(attacktype) && !weap && ch &&
				ch->getLevelBonus(SKILL_KATA) >= 50 &&
					affected_by_spell(ch, SKILL_KATA)) 
			return false;	

		// nothing else can
		return true;
	}

	return false;
}

class CallerDiedException {
};

//
// internal functions
//
void update_pos(struct Creature *victim);
struct obj_data *destroy_object(Creature *ch, struct obj_data *obj, int type);
struct obj_data *damage_eq(struct Creature *ch, struct obj_data *obj, int eq_dam, int type = -1);

void dam_message(int dam, struct Creature *ch, struct Creature *victim,
	int w_type, int location);

void death_cry(struct Creature *ch);
void appear(struct Creature *ch, struct Creature *vict);
void make_corpse(struct Creature *ch, struct Creature *killer,
	int attacktype);
void check_object_killer(struct obj_data *obj, struct Creature *vict);
void raw_kill(struct Creature *ch, struct Creature *killer, int attacktype);	// prototype
bool peaceful_room_ok(struct Creature *ch, struct Creature *vict, bool mssg);
bool is_arena_combat(struct Creature *ch, struct Creature *vict);
bool ok_to_damage(struct Creature *ch, struct Creature *vict);
void count_pkill(struct Creature *killer, struct Creature *vict);
void check_killer(struct Creature *ch, struct Creature *vict,
	const char *debug_msg = 0);
void check_thief(struct Creature *ch, struct Creature *vict,
	const char *debug_msg = 0);
void die(struct Creature *ch, struct Creature *killer, int attacktype,
	int is_humil);
int calculate_thaco(struct Creature *ch, struct Creature *victim,
	struct obj_data *obj);
bool perform_offensive_skill(Creature *ch, Creature *vict, int skill, int *return_flags);
void perform_cleave(Creature *ch, Creature *vict, int *return_flags);

#ifdef __combat_code__
#ifdef __combat_utils__
static int limb_probs[] = {
	0,							//#define WEAR_LIGHT      0
	3,							//#define WEAR_FINGER_R   1
	3,							//#define WEAR_FINGER_L   2
	4,							//#define WEAR_NECK_1     3
	4,							//#define WEAR_NECK_2     4
	35,							//#define WEAR_BODY       5
	12,							//#define WEAR_HEAD       6
	20,							//#define WEAR_LEGS       7
	5,							//#define WEAR_FEET       8
	10,							//#define WEAR_HANDS      9
	25,							//#define WEAR_ARMS      10
	50,							//#define WEAR_SHIELD    11
	0,							//#define WEAR_ABOUT     12
	10,							//#define WEAR_WAIST     13
	5,							//#define WEAR_WRIST_R   14
	5,							//#define WEAR_WRIST_L   15
	0,							//#define WEAR_WIELD     16
	0,							//#define WEAR_HOLD      17
	15,							//#define WEAR_CROTCH    18
	5,							//#define WEAR_EYES      19
	5,							//#define WEAR_BACK      20
	0,							//#define WEAR_BELT      21
	10,							//#define WEAR_FACE      22
	4,							//#define WEAR_EAR_L     23
	4,							//#define WEAR_EAR_R     24
	0,							//#define WEAR_WIELD_2   25
	0,							//#define WEAR_ASS       26
};
//#define NUM_WEARS      27 /* This must be the # of eq positions!! */
#endif

#ifdef __combat_messages__
static int wear_translator[] = {
	WEAR_LIGHT, WEAR_FINGER_R, WEAR_FINGER_R, WEAR_NECK_1, WEAR_NECK_1,
	WEAR_BODY, WEAR_HEAD, WEAR_LEGS, WEAR_FEET, WEAR_HANDS, WEAR_ARMS,
	WEAR_SHIELD, WEAR_BACK, WEAR_WAIST, WEAR_WRIST_R, WEAR_WRIST_R,
	WEAR_WIELD, WEAR_HOLD, WEAR_CROTCH, WEAR_EYES, WEAR_BACK, WEAR_WAIST,
	WEAR_FACE, WEAR_EAR_L, WEAR_EAR_L, WEAR_WIELD_2, WEAR_ASS
};

/* Weapon attack texts */
extern const struct attack_hit_type attack_hit_text[] = {
	{"hit", "hits"},			/* 0 */
	{"sting", "stings"},
	{"whip", "whips"},
	{"slash", "slashes"},
	{"bite", "bites"},
	{"bludgeon", "bludgeons"},	/* 5 */
	{"crush", "crushes"},
	{"pound", "pounds"},
	{"claw", "claws"},
	{"maul", "mauls"},
	{"thrash", "thrashes"},		/* 10 */
	{"pierce", "pierces"},
	{"blast", "blasts"},
	{"punch", "punches"},
	{"stab", "stabs"},
	{"zap", "zaps"},			/* Raygun blasting */
	{"rip", "rips"},
	{"chop", "chops"},
	{"shoot", "shoots"},
};

/* Energy weapon attack texts */
extern const struct gun_hit_type gun_hit_text[] = {
    {"fire", "fires", "laser beam"},
    {"ignite", "ignites", "plasma eruption"},
    {"discharge", "discharges", "ionic pulse"},
    {"energize", "energizes", "photon burst"},
    {"trigger", "triggers", "sonic shock wave"},
    {"fire", "fires", "particle stream"},
    {"activate", "activates", "gamma ray"},
    {"discharge", "discharges", "lightning bolt"},
    {"shoot", "shoots", "unknown force"} //leave this as the top type if adding new types
};

#endif							// __combat_messages__
/* External structures */
extern struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *dam_object;
extern int max_exp_gain;		/* see config.c */
extern int mini_mud;

extern struct combat_data *battles;

/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int char_class_race_hit_bonus(struct Creature *ch, struct Creature *vict);
int apply_soil_to_char(struct Creature *ch, struct obj_data *obj, int type,
	int pos);

int choose_random_limb(Creature *victim);

#endif
#ifdef __fight_c__
/* Structures */
struct Creature *combat_list = NULL;	/* head of list of fighting chars */
struct Creature *next_combat_list = NULL;
struct obj_data *cur_weap = NULL;
#else
extern struct Creature *combat_list;	/* head of list of fighting chars */
extern struct Creature *next_combat_list;
extern struct obj_data *cur_weap;
#endif

/* prototypes from fight.c */
//void set_fighting(struct Creature *ch, struct Creature *victim, int aggr);
void set_defending(struct Creature *ch, struct Creature *target);
void stop_defending(struct Creature *ch);
void stop_follower(struct Creature *ch);
int hit(struct Creature *ch, struct Creature *victim, int type);
void forget(struct Creature *ch, struct Creature *victim);
void remember(struct Creature *ch, struct Creature *victim);
int char_in_memory(struct Creature *victim, struct Creature *rememberer);

const int DAM_VICT_KILLED = 0x0001;	// the victim of damage() died
const int DAM_ATTACKER_KILLED = 0x0002;	// the caller of damage() died
const int DAM_ATTACK_FAILED = 0x0003;	// the caller of damage() died

int SWAP_DAM_RETVAL(int val);
int damage(struct Creature *ch, struct Creature *victim, int dam,
	int attacktype, int location);
int skill_message(int dam, struct Creature *ch, struct Creature *vict,
	int attacktype);
int best_attack(struct Creature *ch, struct Creature *vict);
bool check_infiltrate(struct Creature *ch, struct Creature *vict);
void add_blood_to_room(struct room_data *rm, int amount);


#endif							// __fight_h__
