//
// File: fight.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __fight_h__
#define __fight_h__
//#define DEBUG_POSITION 1


#define DAM_OBJECT_IDNUM(obj) (IS_BOMB(obj) ? BOMB_IDNUM(obj) : GET_OBJ_SIGIL_IDNUM(obj))

#define IS_WEAPON(type) ((((type)>=TYPE_HIT) && ((type)<TOP_ATTACKTYPE)) || \
			 type == SKILL_SECOND_WEAPON || \
			 type == SKILL_ENERGY_WEAPONS || \
			 type == SKILL_ARCHERY        || \
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
 i == TYPE_CHOP ? 7 : 0)\

#define BLOODLET(ch, dam, attacktype) \
(!IS_UNDEAD(ch) && \
 (MIN(BLOODLET_FACTOR(attacktype) * dam, 10000) > number(0, 20000)))


#define POS_DAMAGE_OK(location) \
(location != WEAR_LIGHT && location != WEAR_ABOUT && \
 location != WEAR_HOLD  && location != WEAR_BELT &&  \
 location != WEAR_WIELD && location != WEAR_WIELD_2 && \
 location != WEAR_ASS)

#define IS_DEFENSE_ATTACK(attacktype)   (attacktype == SPELL_FIRE_SHIELD || attacktype == SPELL_BLADE_BARRIER  || attacktype == SPELL_PRISMATIC_SPHERE || attacktype == SKILL_ENERGY_FIELD)

class CallerDiedException {
};

//
// internal functions
//
void update_pos(struct char_data *victim);
struct obj_data *damage_eq(struct char_data *ch, struct obj_data *obj, int eq_dam, int type = -1);

void dam_message(int dam, struct char_data *ch, struct char_data *victim,
	int w_type, int location);

void death_cry(struct char_data *ch);
void appear(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch, struct char_data *killer,
	int attacktype);
void check_object_killer(struct obj_data *obj, struct char_data *vict);
void raw_kill(struct char_data *ch, struct char_data *killer, int attacktype);	// prototype
int peaceful_room_ok(struct char_data *ch, struct char_data *vict, bool mssg);
void check_toughguy(struct char_data *ch, struct char_data *vict, int mode);
void check_killer(struct char_data *ch, struct char_data *vict,
	const char *debug_msg = 0);
void die(struct char_data *ch, struct char_data *killer, int attacktype,
	int is_humil);
int calculate_thaco(struct char_data *ch, struct char_data *victim,
	struct obj_data *obj);

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
#endif							// __combat_messages__
/* External structures */
extern struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *dam_object;
extern int max_exp_gain;		/* see config.c */
extern int mini_mud;
extern struct descriptor_data *descriptor_list;

extern int search_nomessage;
extern struct combat_data *battles;

/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int char_class_race_hit_bonus(struct char_data *ch, struct char_data *vict);
int apply_soil_to_char(struct char_data *ch, struct obj_data *obj, int type,
	int pos);

int choose_random_limb(CHAR * victim);

extern FILE *player_fl;

#endif
#ifdef __fight_c__
/* Structures */
ACCMD(do_offensive_skill);
struct char_data *combat_list = NULL;	/* head of list of fighting chars */
struct char_data *next_combat_list = NULL;
struct obj_data *cur_weap = NULL;
#else
extern struct char_data *combat_list;	/* head of list of fighting chars */
extern struct char_data *next_combat_list;
extern struct obj_data *cur_weap;
#endif

/* prototypes from fight.c */
void set_fighting(struct char_data *ch, struct char_data *victim, int aggr);
void stop_fighting(struct char_data *ch);
void stop_follower(struct char_data *ch);
int hit(struct char_data *ch, struct char_data *victim, int type);
void forget(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim);
int char_in_memory(struct char_data *victim, struct char_data *rememberer);

const int DAM_VICT_KILLED = 0x0001;	// the victim of damage() died
const int DAM_ATTACKER_KILLED = 0x0002;	// the caller of damage() died
const int DAM_ATTACK_FAILED = 0x0003;	// the caller of damage() died

int SWAP_DAM_RETVAL(int val);
int damage(struct char_data *ch, struct char_data *victim, int dam,
	int attacktype, int location);
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
	int attacktype);
int best_attack(struct char_data *ch, struct char_data *vict);
int check_infiltrate(struct char_data *ch, struct char_data *vict);
void add_blood_to_room(struct room_data *rm, int amount);


#endif							// __fight_h__
