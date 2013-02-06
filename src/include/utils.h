#ifndef _UTILS_H_
#define _UTILS_H_

/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: utils.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/* external declarations and prototypes **********************************/
struct creature;
struct zone_data;
struct room_data;
struct obj_data;
struct descriptor_data;
enum mob_flag;
enum mob2_flag;

/* public functions in utils.c */
int touch(const char *path);

enum log_type
{
	OFF = 0,
	BRF = 1,
	NRM = 2,
	CMP = 3
};

// mudlog() and slog() are shorter interfaces to mlog()
void mudlog(int8_t level, enum log_type type, bool file, const char *fmt, ...)
	__attribute__ ((format (printf, 4, 5)));
void slog(const char *str, ...)
	__attribute__ ((format (printf, 1, 2)));
void errlog(const char *str, ...)
	__attribute__ ((format (printf, 1, 2)));
void zerrlog(struct zone_data *zone, const char *str, ...)
	__attribute__ ((format (printf, 2, 3)));

void mlog(const char *group,
		int8_t level,
		enum log_type type,
		bool file,
		const char *fmt, ...)
	__attribute__ ((format (printf, 5, 6)));

void log_death_trap(struct creature *ch);
void show_string(struct descriptor_data *desc);
int number(int from, int to);
double float_number(double from, double to);
int dice(int number, int size);
int get_line(FILE * fl, char *buf);
void perform_skillset(struct creature *ch, struct creature *vict, char *skill_str, int value);
float total_obj_weight(struct obj_data *obj);

enum track_mode
{
	STD_TRACK = 0,
	GOD_TRACK = 1,
	PSI_TRACK = 2
};
int find_first_step(struct room_data *start, struct room_data *dest, enum track_mode mode);
int find_distance(struct room_data *start, struct room_data *dest);

struct time_info_data age(struct creature *ch);
struct time_info_data mud_time_passed(time_t t2, time_t t1);
extern struct zone_data *zone_table;
extern struct creature *mob_proto;
__attribute__((noreturn)) void safe_exit(int mode);
bool player_in_room(struct room_data *room);
void check_bits_32(int bitv, int *newbits);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

/* in magic.c */
bool circle_follow(struct creature *ch, struct creature *victim);
bool can_charm_more(struct creature *ch);

/* in act.informative.c */
void look_at_room(struct creature *ch, struct room_data *room, int mode);

/* in act.movmement.c */
int do_simple_move(struct creature *ch, int dir, int mode, int following);
int perform_move(struct creature *ch, int dir, int mode, int following);

/* in limits.c */
int mana_gain(struct creature *ch);
int hit_gain(struct creature *ch);
int move_gain(struct creature *ch);
void advance_level(struct creature *ch, int8_t keep_internal);
void set_title(struct creature *ch, const char *title);
void gain_exp(struct creature *ch, int gain);
void gain_exp_regardless(struct creature *ch, int gain);
void gain_condition(struct creature *ch, int condition, int value);
int check_idling(struct creature *ch);
void point_update(void);

void new_follower(struct creature *ch, struct creature *leader);
void add_follower(struct creature *ch, struct creature *leader);
void remove_follower(struct creature *ch);
void stop_follower(struct creature *ch);

char *GET_DISGUISED_NAME(struct creature *ch, struct creature *tch);
int CHECK_SKILL(struct creature *ch, int i);
int CHECK_TONGUE(struct creature *ch, int i);
const char *OBJS(struct obj_data *obj, struct creature *vict);
const char *OBJN(struct obj_data *obj, struct creature *vict);
const char *PERS(struct creature *ch, struct creature *sub);

void WAIT_STATE(struct creature *ch, int cycle);
/* various constants *****************************************************/

/* breadth-first searching */
#define BFS_ERROR                -1
#define BFS_ALREADY_THERE        -2
#define BFS_NO_PATH                -3

/* mud-life time */
#define SECS_PER_MUD_HOUR        60
#define SECS_PER_MUD_DAY        (24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH        (35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR        (16*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN        60
#define SECS_PER_REAL_HOUR        (60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY        (24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR        (365*SECS_PER_REAL_DAY)

/* memory utils **********************************************************/

#define CREATE(result, type, number)  do {\
        if (!((result) = (type *)(calloc ((number), sizeof(type)))))    \
        { perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
        if (!((result) = (type *)(realloc ((result), sizeof(type) * (number))))) \
                { perror("realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)          \
    do {                                            \
        if ((item) == (head))                       \
            head = (item)->next;                    \
        else {                                      \
            temp = head;                            \
            while (temp && (temp->next != (item)))  \
                temp = temp->next;                  \
            if (temp)                               \
                temp->next = (item)->next;          \
        }                                           \
    } while (false)

/* basic bitvector utils *************************************************/

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~((unsigned long)(bit)))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

/* room utils ************************************************************/
#define ROOM_FLAGS(loc)           ((loc)->room_flags)
#define NOFLEE(loc)          (ROOM_FLAGGED(loc,ROOM_NOFLEE) \
                                && !(random_fractional_10()))
#define SECT_TYPE(room)           ((room)->sector_type)
#define GET_PLANE(room)           ((room)->zone->plane)
#define GET_ZONE(room)            ((room)->zone->number)
#define PRIME_MATERIAL_ROOM(room) (GET_PLANE(room) <= MAX_PRIME_PLANE)
#define GET_TIME_FRAME(room)      ((room)->zone->time_frame)
#define IS_TIMELESS(room)         (GET_TIME_FRAME((room)) == TIME_TIMELESS)
#define FLOW_DIR(room)            ((room)->flow_dir)
#define FLOW_SPEED(room)          ((room)->flow_speed)
#define FLOW_TYPE(room)           ((room)->flow_type)
#define FLOW_PULSE(room)          ((room)->flow_pulse)
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS((loc)), (flag)))
#define MAX_OCCUPANTS(room)       ((room)->max_occupancy)
#define ZONE_FLAGS(loc)           ((loc)->flags)
#define ZONE_FLAGGED(loc, flag)   (IS_SET(ZONE_FLAGS((loc)), (flag)))

#define SRCH_FLAGGED(srch, flag)   (IS_SET(srch->flags, flag))
#define SRCH_OK(ch, srch) \
   ((!IS_EVIL(ch) || !SRCH_FLAGGED(srch, SRCH_NOEVIL)) && \
    (!IS_NEUTRAL(ch) || !SRCH_FLAGGED(srch, SRCH_NONEUTRAL)) && \
    (!IS_GOOD(ch) || !SRCH_FLAGGED(srch, SRCH_NOGOOD)) && \
    (GET_POSITION(ch) < POS_FLYING || !SRCH_FLAGGED(srch, SRCH_NOTRIG_FLY)) && \
    (!IS_NPC(ch) || !SRCH_FLAGGED(srch, SRCH_NOMOB)) &&        \
    ( IS_NPC(ch) || !SRCH_FLAGGED(srch, SRCH_NOPLAYER)) &&        \
	(!AFF_FLAGGED(ch, AFF_CHARM) || !SRCH_FLAGGED(srch, SRCH_NOPLAYER)) &&	  \
    (!IS_MAGE(ch)   || !SRCH_FLAGGED(srch, SRCH_NOMAGE)) &&    \
    (!IS_CLERIC(ch) || !SRCH_FLAGGED(srch, SRCH_NOCLERIC)) && \
    (!IS_THIEF(ch)  || !SRCH_FLAGGED(srch, SRCH_NOTHIEF)) &&  \
    (!IS_BARB(ch)   || !SRCH_FLAGGED(srch, SRCH_NOBARB)) &&   \
    (!IS_RANGER(ch) || !SRCH_FLAGGED(srch, SRCH_NORANGER)) && \
    (!IS_KNIGHT(ch) || !SRCH_FLAGGED(srch, SRCH_NOKNIGHT)) && \
    (!IS_MONK(ch)   || !SRCH_FLAGGED(srch, SRCH_NOMONK)) &&   \
    (!IS_PSIONIC(ch) || !SRCH_FLAGGED(srch, SRCH_NOPSIONIC)) && \
    (!IS_PHYSIC(ch) || !SRCH_FLAGGED(srch, SRCH_NOPHYSIC)) && \
    (!IS_MERC(ch)   || !SRCH_FLAGGED(srch, SRCH_NOMERC)) &&   \
    (!IS_BARD(ch)   || !SRCH_FLAGGED(srch, SRCH_NOBARD)) &&   \
    !SRCH_FLAGGED(srch, SRCH_TRIPPED))

#define IN_ICY_HELL(ch)  (ch->in_room->zone->plane == PLANE_HELL_5)
#define HELL_PLANE(zone, num)  ((zone)->plane == (PLANE_HELL_1 - 1 + num))

/******************* SHADOW PLANE STUFF *******************/
#define SHADOW_ZONE(zone)     ((zone)->plane == PLANE_SHADOW)
#define ZONE_IS_SHADE(zone)   ((zone)->number == 198)

/******************* ASLEEP ZONE STUFF ********************/
#define ZONE_IS_ASLEEP(zone)  ((zone)->number == 445 || \
                               (zone)->number == 446 || \
							   (zone)->number == 447)

/***************** END  SHADOW PLANE STUFF *****************/

#define ZONE_IS_HELL(zone) \
                          (zone->plane >= PLANE_HELL_1 && zone->plane <= PLANE_HELL_9)

#define NOGRAV_ZONE(zone) ((zone->plane >= PLANE_ELEM_WATER && \
                             zone->plane <= PLANE_ELEM_NEG) || \
                            zone->plane == PLANE_ASTRAL || zone->plane == PLANE_PELEM_MAGMA \
                            || zone->plane == PLANE_PELEM_OOZE )

/*  character utils ******************************************************/
	  /* room utils *********************************************************** */

#define GET_ROOM_SPEC(room) ((room)->func)
#define GET_ROOM_PARAM(room) ((room)->func_param)
#define GET_ROOM_PROG(room) ((room)->prog)
#define GET_ROOM_PROGOBJ(room) ((room)->progobj)

/* char utils ************************************************************/

/* descriptor-based utils ************************************************/

#define CHECK_WAIT(ch)        (((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)        ((d)->input_mode)

/* object utils **********************************************************/

#define GET_OBJ_TYPE(obj)        ((obj)->obj_flags.type_flag)
#define IS_OBJ_TYPE(obj, type)  (GET_OBJ_TYPE(obj) == type)
#define GET_OBJ_COST(obj)        ((obj)->shared->cost)
// can only be used to get the rent, not set
#define GET_OBJ_RENT(obj)        ((obj)->plrtext_len ? \
                                  ((int)(obj)->plrtext_len << 3) :  \
                                  (obj)->shared->cost_per_day)

#define GET_OBJ_EXTRA(obj)        ((obj)->obj_flags.extra_flags)
#define GET_OBJ_EXTRA2(obj)        ((obj)->obj_flags.extra2_flags)
#define GET_OBJ_EXTRA3(obj)        ((obj)->obj_flags.extra3_flags)
#define GET_OBJ_WEIGHT(obj)        ((obj)->obj_flags.weight)
#define GET_OBJ_WEAR(obj)        ((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)        ((obj)->obj_flags.value[(val)])
#define GET_OBJ_TIMER(obj)        ((obj)->obj_flags.timer)
#define GET_OBJ_MATERIAL(obj)        ((obj)->obj_flags.material)
#define GET_OBJ_MAX_DAM(obj)        ((obj)->obj_flags.max_dam)
#define GET_OBJ_SIGIL_IDNUM(obj) ((obj)->obj_flags.sigil_idnum)
#define GET_OBJ_SIGIL_LEVEL(obj) ((obj)->obj_flags.sigil_level)
#define GET_OBJ_DAM(obj)        ((obj)->obj_flags.damage)
#define CORPSE_IDNUM(obj)        ((obj)->obj_flags.value[2])
#define CORPSE_KILLER(obj)        ((obj)->obj_flags.value[1])
#define GET_OBJ_VNUM(obj)        ((obj)->shared->vnum)
#define IS_OBJ_STAT(obj,stat)        (IS_SET((obj)->obj_flags.extra_flags,stat))
#define IS_OBJ_STAT2(obj,stat)        (IS_SET((obj)->obj_flags.extra2_flags,stat))
#define IS_OBJ_STAT3(obj,stat)        (IS_SET((obj)->obj_flags.extra3_flags,stat))

#define GET_OBJ_SPEC(obj) ((obj) ? (obj)->shared->func : NULL)
#define GET_OBJ_PARAM(obj) ((obj) ? (obj)->shared->func_param : NULL)
#define GET_OBJ_DATA(obj) ((obj) ? (obj)->func_data : NULL)

#define OBJ_SOILAGE(obj)        (obj->soilage)
#define OBJ_SOILED(obj, soil)    (IS_SET(OBJ_SOILAGE(obj), soil))

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

#define IS_TWO_HAND(obj) (IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED))
#define IS_NODROP(obj)   (IS_OBJ_STAT(obj, ITEM_NODROP))
#define IS_THROWN(obj)   (IS_OBJ_STAT2(obj, ITEM2_THROWN_WEAPON))

#define STAB_WEAPON(obj) (IS_OBJ_TYPE(obj, ITEM_WEAPON) && \
                          (GET_OBJ_VAL(obj, 3) + TYPE_HIT == TYPE_STAB || \
                           GET_OBJ_VAL(obj, 3) + TYPE_HIT == TYPE_PIERCE))

#define IS_CORPSE(obj)  (IS_OBJ_TYPE(obj, ITEM_CONTAINER) && \
                         GET_OBJ_VAL(obj, 3))

#define IS_BODY_PART(obj)  (IS_OBJ_STAT2(obj, ITEM2_BODY_PART))

#define OBJ_APPROVED(obj) (!IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
#define P_OBJ_APPROVED(p_obj) (!IS_SET(p_obj->obj_flags.extra2_flags, \
                                      ITEM2_UNAPPROVED))

#define OBJ_IS_RAD(obj) (IS_OBJ_STAT2(obj, ITEM2_RADIOACTIVE))

#define QUAD_VNUM  1578
#define MIXED_POTION_VNUM 15

#define BLOOD_VNUM 1579
#define ICE_VNUM   1576

#define OBJ_IS_SOILAGE(obj) (GET_OBJ_VNUM(obj) == BLOOD_VNUM || \
							 GET_OBJ_VNUM(obj) == ICE_VNUM)

#define CHAR_HAS_BLOOD(ch)  (!IS_UNDEAD(ch) && !IS_ELEMENTAL(ch) && \
                             !IS_GOLEM(ch) && !IS_ROBOT(ch) && \
                             !IS_PLANT(ch) && !IS_ALIEN_1(ch) && \
                             !IS_PUDDING(ch) && !IS_SLIME(ch))

#define OBJ_REINFORCED(obj) (IS_OBJ_STAT2(obj, ITEM2_REINFORCED))
#define OBJ_ENHANCED(obj) (IS_OBJ_STAT2(obj, ITEM2_ENHANCED))

#define ANTI_ALIGN_OBJ(ch, obj) \
     ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL | ITEM_BLESS) && IS_EVIL(ch)) ||    \
      (IS_OBJ_STAT(obj,ITEM_ANTI_GOOD | ITEM_DAMNED) && IS_GOOD(ch)) ||\
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))

/* compound utilities and other macros **********************************/

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->aliases) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->aliases) ? "an" : "a")

#define GET_SKILL(ch, i)        ((ch)->player_specials->saved.skills[i])
long GET_SKILL_COST(struct creature *ch, int skill);
#define KNOCKDOWN_SKILL(i) \
          (i == SPELL_EARTHQUAKE  || i == SPELL_PSYCHIC_SURGE || \
           i == SPELL_EGO_WHIP    || i == SKILL_BASH ||          \
           i == SKILL_PILEDRIVE   || i == SKILL_BODYSLAM ||      \
           i == SKILL_LUNGE_PUNCH || i == SKILL_CLOTHESLINE ||   \
           i == SKILL_SWEEPKICK   || i == SKILL_TRIP ||          \
           i == SKILL_HIP_TOSS    || i == SKILL_SHOULDER_THROW ||\
		   i == SKILL_SIDEKICK)

#define CLASS_ABBR(ch) (char_class_abbrevs[(int)GET_CLASS(ch)])
#define LEV_ABBR(ch) (IS_NPC(ch) ? "--" : level_abbrevs[(int)GET_LEVEL(ch)-50])

#define IS_CLASS(ch, char_class)     (GET_CLASS(ch) == char_class || \
                                 GET_REMORT_CLASS(ch) == char_class)

#define IS_MAGIC_USER(ch)           IS_CLASS(ch, CLASS_MAGIC_USER)
#define IS_MAGE(ch)                IS_MAGIC_USER(ch)
#define IS_CLERIC(ch)             IS_CLASS(ch, CLASS_CLERIC)
#define IS_THIEF(ch)                  IS_CLASS(ch, CLASS_THIEF)
#define IS_WARRIOR(ch)                  IS_CLASS(ch, CLASS_WARRIOR)
#define IS_BARB(ch)               IS_CLASS(ch, CLASS_BARB)
#define IS_PSYCHIC(ch)            IS_CLASS(ch, CLASS_PSIONIC)
#define IS_PSIONIC(ch)          IS_PSYCHIC(ch)
#define IS_PHYSIC(ch)             IS_CLASS(ch, CLASS_PHYSIC)
#define IS_CYBORG(ch)             IS_CLASS(ch, CLASS_CYBORG)
#define IS_KNIGHT(ch)             IS_CLASS(ch, CLASS_KNIGHT)
#define IS_RANGER(ch)             IS_CLASS(ch, CLASS_RANGER)
#define IS_BARD(ch)               IS_CLASS(ch, CLASS_BARD)
#define IS_MONK(ch)               IS_CLASS(ch, CLASS_MONK)
#define IS_MERC(ch)               IS_CLASS(ch, CLASS_MERCENARY)
#define IS_SPARE1(ch)             IS_CLASS(ch, CLASS_SPARE1)
#define IS_SPARE2(ch)             IS_CLASS(ch, CLASS_SPARE2)
#define IS_SPARE3(ch)             IS_CLASS(ch, CLASS_SPARE3)
#define IS_SKELETON(ch)           IS_CLASS(ch, CLASS_SKELETON)
#define IS_GHOUL(ch)              IS_CLASS(ch, CLASS_GHOUL)
#define IS_SHADOW(ch)             IS_CLASS(ch, CLASS_SHADOW)
#define IS_WIGHT(ch)              IS_CLASS(ch, CLASS_WIGHT)
#define IS_WRAITH(ch)             IS_CLASS(ch, CLASS_WRAITH)
#define IS_MUMMY(ch)              IS_CLASS(ch, CLASS_MUMMY)
#define IS_SPECTRE(ch)            IS_CLASS(ch, CLASS_SPECTRE)
#define IS_VAMPIRE(ch)            IS_CLASS(ch, CLASS_VAMPIRE)
#define IS_NPC_VAMPIRE(ch)          (IS_CLASS(ch, CLASS_VAMPIRE) && IS_NPC(ch))
#define IS_GHOST(ch)                IS_CLASS(ch, CLASS_GHOST)
#define IS_LICH(ch)              IS_CLASS(ch, CLASS_LICH)
#define IS_ZOMBIE(ch)                  IS_CLASS(ch, CLASS_ZOMBIE)
#define IS_UNDEAD(ch)            (IS_RACE(ch, RACE_UNDEAD) || \
                                 IS_SKELETON(ch) || \
                                 IS_GHOUL(ch) || \
                                 IS_SHADOW(ch) || \
                                 IS_WIGHT(ch) || \
                                 IS_WRAITH(ch) || \
                                 IS_MUMMY(ch) || \
                                 IS_SPECTRE(ch) || \
                                 IS_VAMPIRE(ch) || \
                                 IS_GHOST(ch) || \
                                 IS_LICH(ch) || \
                                 IS_ZOMBIE(ch))

#define NON_CORPOREAL_MOB(ch) \
            ((IS_SHADOW(ch) || IS_WIGHT(ch) ||   \
            IS_WRAITH(ch) || IS_SPECTRE(ch) || \
            IS_GHOST(ch))                      \
            || (GET_RACE(ch) == RACE_ELEMENTAL &&       \
                (                                       \
                GET_CLASS(ch) == CLASS_AIR ||           \
                GET_CLASS(ch) == CLASS_WATER ||         \
                GET_CLASS(ch) == CLASS_FIRE             \
                )))

#define IS_HUMAN(ch)                     (GET_RACE(ch) == RACE_HUMAN)
#define IS_ELF(ch)                     (GET_RACE(ch) == RACE_ELF)
#define IS_DROW(ch)                     (GET_RACE(ch) == RACE_DROW)
#define IS_DWARF(ch)                     (GET_RACE(ch) == RACE_DWARF)
#define IS_TABAXI(ch)                     (GET_RACE(ch) == RACE_TABAXI)
#define IS_HALF_ORC(ch)                     (GET_RACE(ch) == RACE_HALF_ORC)
#define IS_ANIMAL(ch)                     (GET_RACE(ch) == RACE_ANIMAL)
#define IS_ELEMENTAL(ch)        (GET_RACE(ch) == RACE_ELEMENTAL)
#define IS_DRAGON(ch)           (GET_RACE(ch) == RACE_DRAGON)
#define IS_HUMANOID(ch)         (GET_RACE(ch) == RACE_HUMANOID)
#define IS_HALFLING(ch)         (GET_RACE(ch) == RACE_HALFLING)
#define IS_GIANT(ch)                 (GET_RACE(ch) == RACE_GIANT)
#define IS_ORC(ch)                 (GET_RACE(ch) == RACE_ORC)
#define IS_GOBLIN(ch)                 (GET_RACE(ch) == RACE_GOBLIN)
#define IS_MINOTAUR(ch)         (GET_RACE(ch) == RACE_MINOTAUR)
#define IS_TROLL(ch)                (GET_RACE(ch) == RACE_TROLL)
#define IS_GOLEM(ch)                (GET_RACE(ch) == RACE_GOLEM)
#define IS_OGRE(ch)                 (GET_RACE(ch) == RACE_OGRE)
#define IS_DEVIL(ch)                (GET_RACE(ch) == RACE_DEVIL)
#define IS_DEMON(ch)                (GET_RACE(ch) == RACE_DEMON)
#define IS_SLAAD(ch)                (GET_RACE(ch) == RACE_SLAAD)
#define IS_TROG(ch)                (GET_RACE(ch) == RACE_TROGLODYTE)
#define IS_MANTICORE(ch)        (GET_RACE(ch) == RACE_MANTICORE)
#define IS_ROBOT(ch)            (GET_RACE(ch) == RACE_ROBOT)
#define IS_PLANT(ch)            (GET_RACE(ch) == RACE_PLANT)
#define IS_ARCHON(ch)           (GET_RACE(ch) == RACE_ARCHON)
#define IS_GUARDINAL(ch)           (GET_RACE(ch) == RACE_GUARDINAL)
#define IS_PUDDING(ch)          (GET_RACE(ch) == RACE_PUDDING)
#define IS_SLIME(ch)            (GET_RACE(ch) == RACE_SLIME)
#define IS_BUGBEAR(ch)          (GET_RACE(ch) == RACE_BUGBEAR)
#define IS_ALIEN_1(ch)          (GET_RACE(ch) == RACE_ALIEN_1)
#define IS_FISH(ch)             (GET_RACE(ch) == RACE_FISH)
#define IS_RAKSHASA(ch)         (GET_RACE(ch) == RACE_RAKSHASA)
#define IS_ROWLAHR(ch)         (GET_RACE(ch) == RACE_ROWLAHR)
#define IS_RACE(ch, race)       (GET_RACE(ch) == race)

#define IS_CELESTIAL(ch)		(IS_ARCHON(ch) || IS_GUARDINAL(ch))
#define IS_GREATER_DEVIL(ch) (IS_DEVIL(ch) \
                              && (GET_CLASS(ch) == CLASS_GREATER \
                                  || GET_CLASS(ch) == CLASS_ARCH \
                                  || GET_CLASS(ch) == CLASS_DUKE))

#define HUMANOID_TYPE(ch)       ((GET_RACE(ch) <= RACE_DEVIL && \
                                  !IS_RACE(ch, RACE_ANIMAL) &&     \
                                  !IS_RACE(ch, RACE_DRAGON) &&     \
                                  !IS_RACE(ch, RACE_ELEMENTAL)) || \
                                 (GET_RACE(ch) >= RACE_BUGBEAR &&  \
                                  GET_RACE(ch) <= RACE_DEVA) ||    \
                                 IS_RACE(ch, RACE_ARCHON) ||       \
                                 IS_RACE(ch, RACE_ILLITHID) ||     \
                                 IS_RACE(ch, RACE_GITHYANKI) ||    \
                                 IS_RACE(ch, RACE_GITHZERAI) ||    \
                                 IS_RACE(ch, RACE_KOBOLD) ||    \
                                 IS_RACE(ch, RACE_MEPHIT) ||    \
                                 IS_RACE(ch, RACE_DAEMON) ||    \
                                 IS_RACE(ch, RACE_RAKSHASA) || \
                                 IS_RACE(ch, RACE_ROWLAHR))

#define IS_TIAMAT(ch)           (GET_NPC_VNUM(ch) == 61119)
#define IS_TARRASQUE(ch)           (GET_NPC_VNUM(ch) == 24800)

#define IS_LEMURE(ch)           (GET_NPC_VNUM(ch) == 16121 || \
                                 GET_NPC_VNUM(ch) == 16110 || \
                                 GET_NPC_VNUM(ch) == 16652)

#define ICY_DEVIL(ch)           (GET_NPC_VNUM(ch) == 16117 || \
                                 GET_NPC_VNUM(ch) == 16146 || \
                                 GET_NPC_VNUM(ch) == 16132)

#define LIFE_FORM(ch)           (!IS_ROBOT(ch) && !IS_UNDEAD(ch))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS) && \
                                        (ch)->in_room->sector_type != SECT_INSIDE )
bool has_dark_sight(struct creature *self);
bool check_sight_self(struct creature *self);
bool check_sight_room(struct creature *self, struct room_data *room);
bool check_sight_object(struct creature *self, struct obj_data *obj);
bool check_sight_vict(struct creature *self, struct creature *vict);

bool can_see_creature(struct creature *self, struct creature *vict);
bool can_see_object(struct creature *self, struct obj_data *obj);
bool can_see_room(struct creature *self, struct room_data *room);

#define CAN_CARRY_OBJ(ch,obj)  \
    (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) && \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   ((CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    can_see_object((ch),(obj))) || GET_LEVEL(ch) > LVL_CREATOR)

#define CAN_DETECT_DISGUISE(ch, vict, level) \
                          (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || \
                           AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) ||\
                           (GET_INT(ch)+GET_WIS(ch)) > (level+GET_CHA(vict)))

bool CAN_GO(struct creature * ch, int door);
bool OCAN_GO(struct obj_data * obj, int door);

struct extra_descr_data *exdesc_list_dup(struct extra_descr_data *list);
int smart_mobile_move(struct creature *ch, int dir);
int drag_char_to_jail(struct creature *ch, struct creature *vict, struct room_data *jail_room);

/* OS compatibility ******************************************************/

#if !defined(false)
#define false 0
#endif

#if !defined(true)
#define true  (!false)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#endif

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
//
// my_rand() returns 0 to 2147483647
//

unsigned long my_rand(void);
double rand_float(void);
// returns a random boolean value
bool random_binary();
// returns a random boolean value, true 1/num of returns
bool random_fractional(unsigned int num);
// returns a random boolean value, true 1/3 of returns (33% tru)
bool random_fractional_3();
// returns a random boolean value, true 1/4 of returns (25% true)
bool random_fractional_4();
// returns a random boolean value, true 1/5 of returns (20% true)
bool random_fractional_5();
// returns a random boolean value, true 1/10 of returns (10% true)
bool random_fractional_10();
// returns a random boolean value, true 1/20 of returns (5% true)
bool random_fractional_20();
// returns a random boolean value, true 1/50 of returns (2% true)
bool random_fractional_50();
// returns a random boolean value, true 1/100 of returns (1% true)
bool random_fractional_100();
// returns a random value between and including 1-100
int random_percentage();
// returns a random value between and including 0-99
int random_percentage_zero_low();
// return a random value of val +/- variance not < min or > max
int rand_value(int val, int variance, int min, int max);
// return a random value between 0 and num
int random_number_zero_low(unsigned int num);
// creates a random number in interval [from;to]'
int number(int from, int to);
double float_number(double from, double to);
// simulates dice roll
int dice(int number, int size);

static inline const char *
SAFETY(const char *str)
{
	if (!str) {
		errlog("Attempt to print null string");
		return "<NULLS>";
	}
	return str;
}

static inline unsigned int
hex2dec(const char *s)
{
	unsigned int i = 0;

	while (isxdigit(*s)) {
		int n = toupper(*s);
		i = i << 4 | (n - ((n >= (int)'A') ? (int)'7':(int)'0'));
		s++;
	}
	return i;
}

char *format_weight(float lbs, bool metric);
char *format_distance(int cm, bool metric);
float parse_weight(char *str, bool metric);
int parse_distance(char *str, bool metric);

#endif
