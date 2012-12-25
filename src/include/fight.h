#ifndef _FIGHT_H_
#define _FIGHT_H_

//
// File: fight.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define DAM_OBJECT_IDNUM(obj) (IS_BOMB(obj) ? BOMB_IDNUM(obj) : GET_OBJ_SIGIL_IDNUM(obj))

#define IS_WEAPON(type) ((((type)>=TYPE_HIT) && ((type)<TOP_ATTACKTYPE)) || \
			 type == SKILL_SECOND_WEAPON || \
			 type == SKILL_ENERGY_WEAPONS || \
			 type == SKILL_ARCHERY        || \
             (((type) >= TYPE_EGUN_LASER) && ((type) <= TYPE_EGUN_TOP)) || \
			 type == SKILL_PROJ_WEAPONS)

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

bool cannot_damage(struct creature *ch, struct creature *vict, struct obj_data *weap, int attacktype)
    __attribute__ ((nonnull (2)));

struct CallerDiedException {
};

//
// internal functions
//
bool ok_damage_vendor(struct creature *ch, struct creature *victim)
    __attribute__ ((nonnull (2)));
void update_pos(struct creature *victim)
    __attribute__ ((nonnull));
struct obj_data *destroy_object(struct creature *ch, struct obj_data *obj, int type)
    __attribute__ ((nonnull (2)));
struct obj_data *damage_eq(struct creature *ch, struct obj_data *obj, int eq_dam, int type)
    __attribute__ ((nonnull (2)));

void dam_message(int dam, struct creature *ch, struct creature *victim,
                 struct obj_data *weapon, int w_type, int location)
    __attribute__ ((nonnull (2,3)));

bool do_gun_special(struct creature *ch, struct obj_data *obj)
    __attribute__ ((nonnull));

void death_cry(struct creature *ch)
    __attribute__ ((nonnull));
void appear(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
struct obj_data *make_corpse(struct creature *ch, struct creature *killer,
                     int attacktype)
    __attribute__ ((nonnull (1)));
void check_object_killer(struct obj_data *obj, struct creature *vict)
    __attribute__ ((nonnull));
void raw_kill(struct creature *ch, struct creature *killer, int attacktype)
    __attribute__ ((nonnull (1)));
bool is_arena_combat(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull (2)));
bool is_npk_combat(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
bool ok_to_damage(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void count_pkill(struct creature *killer, struct creature *vict)
    __attribute__ ((nonnull));
void check_attack(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void check_thief(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void die(struct creature *ch, struct creature *killer, int attacktype)
    __attribute__ ((nonnull (1)));
int calculate_thaco(struct creature *ch, struct creature *victim,
	struct obj_data *obj)
    __attribute__ ((nonnull (1,2)));
bool perform_offensive_skill(struct creature *ch, struct creature *vict, int skill)
    __attribute__ ((nonnull));
void perform_cleave(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void punish_killer_death(struct creature *ch)
    __attribute__ ((nonnull));

static const int wear_translator[] = {
	WEAR_LIGHT, WEAR_FINGER_R, WEAR_FINGER_R, WEAR_NECK_1, WEAR_NECK_1,
	WEAR_BODY, WEAR_HEAD, WEAR_LEGS, WEAR_FEET, WEAR_HANDS, WEAR_ARMS,
	WEAR_SHIELD, WEAR_BACK, WEAR_WAIST, WEAR_WRIST_R, WEAR_WRIST_R,
	WEAR_WIELD, WEAR_HOLD, WEAR_CROTCH, WEAR_EYES, WEAR_BACK, WEAR_WAIST,
	WEAR_FACE, WEAR_EAR_L, WEAR_EAR_L, WEAR_WIELD_2, WEAR_ASS
};

/* Weapon attack texts */
static const struct attack_hit_type attack_hit_text[] = {
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
static const struct gun_hit_type gun_hit_text[] = {
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

/* External structures */
extern struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *dam_object;
extern int max_exp_gain;		/* see config.c */
extern int mini_mud;

extern struct combat_data *battles;

/* External procedures */
char *fread_action(FILE * fl, int nr)
    __attribute__ ((nonnull));
ACMD(do_flee);
int char_class_race_hit_bonus(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
int apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
	int pos)
    __attribute__ ((nonnull (1)));

int choose_random_limb(struct creature *victim)
    __attribute__ ((nonnull));

/* prototypes from fight.c */
void set_defending(struct creature *ch, struct creature *target)
    __attribute__ ((nonnull));
void stop_follower(struct creature *ch)
    __attribute__ ((nonnull));
int hit(struct creature *ch, struct creature *victim, int type)
    __attribute__ ((nonnull));
void forget(struct creature *ch, struct creature *victim)
    __attribute__ ((nonnull));
void remember(struct creature *ch, struct creature *victim)
    __attribute__ ((nonnull));
int char_in_memory(struct creature *victim, struct creature *rememberer)
    __attribute__ ((nonnull));

bool damage(struct creature *ch, struct creature *victim,
            struct obj_data *weapon,
            int dam, int attacktype, int location)
    __attribute__ ((nonnull (2)));
int skill_message(int dam, struct creature *ch, struct creature *vict,
                  struct obj_data *weapon, int attacktype)
    __attribute__ ((nonnull (2,3)));
void best_initial_attack(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
bool check_infiltrate(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void add_blood_to_room(struct room_data *rm, int amount)
    __attribute__ ((nonnull));
bool ok_to_attack(struct creature *ch, struct creature *vict, bool emit)
    __attribute__ ((nonnull));
void perform_stun(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));

#endif							// __fight_h__
