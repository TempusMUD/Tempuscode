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

#ifndef __utils_h__
#define __utils_h__

#include <iostream>

/* external declarations and prototypes **********************************/

/* public functions in utils.c */
char *str_dup(const char *source);
int str_cmp(const char *arg1, const char *arg2);
int strn_cmp(char *arg1, char *arg2, int n);
void slog(char *str);
int touch(char *path);
void mudlog(char *str, char type, sbyte level, byte file);
void log_death_trap(struct char_data *ch);
int number(int from, int to);
double float_number(double from, double to);
int dice(int number, int size);
void sprintbit(long vektor, const char *names[], char *result);
void sprinttype(int type, const char *names[], char *result);
int get_line(FILE * fl, char *buf);
int get_filename(char *orig_name, char *filename, int mode);
int get_line_count(char *buffer);
int remove_from_cstring(char *str, char c = '~', char c_to = '.');

struct time_info_data age(struct char_data *ch);
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern struct spell_info_type spell_info[];
void safe_exit(int mode);
int player_in_room(struct room_data *room);


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a > b ? b : a)

/* in magic.c */
bool circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
void look_at_room(struct char_data *ch, struct room_data *room, int mode);

/* in act.movmement.c */
int do_simple_move(struct char_data *ch, int dir, int mode, int following);
int perform_move(struct char_data *ch, int dir, int mode, int following);

/* in limits.c */
int mana_limit(struct char_data *ch);
int hit_limit(struct char_data *ch);
int move_limit(struct char_data *ch);
int mana_gain(struct char_data *ch);
int hit_gain(struct char_data *ch);
int move_gain(struct char_data *ch);
void advance_level(struct char_data *ch, byte keep_internal);
void set_title(struct char_data *ch, char *title);
void gain_exp(struct char_data *ch, int gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
int check_idling(struct char_data *ch);
void point_update(void);
void update_pos(struct char_data *victim);
char *GET_DISGUISED_NAME(struct char_data *ch, struct char_data *tch);
int CHECK_SKILL(struct char_data *ch, int i);
char *OBJS(obj_data * obj, char_data * vict);
char *OBJN(obj_data * obj, char_data * vict);
char *PERS(char_data * ch, char_data * sub);

void WAIT_STATE(struct char_data *ch, int cycle);
/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF        0
#define BRF        1
#define NRM        2
#define CMP        3

/* get_filename() */
#define CRASH_FILE        0
#define ETEXT_FILE        1
#define IMPLANT_FILE    2
#define PLAYER_MAIL_FILE 3

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


#define ABS(a)  MAX(a, -a)
/* string utils **********************************************************/



#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

char *YESNO(bool a);
char *ONOFF(bool a);
char *AN(char *str);

//#define AN(str) ( PLUR( str ) ? "some" : (strchr("aeiouAEIOU", *str) ? "an" : "a"))


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
        if (!((result) = (type *) calloc ((number), sizeof(type))))\
                { perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
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
#define REMOVE_FROM_LIST(item, head, next)        \
   if ((item) == (head))                \
      head = (item)->next;                \
   else {                                \
      temp = head;                        \
      while (temp && (temp->next != (item))) \
         temp = temp->next;                \
      if (temp)                                \
         temp->next = (item)->next;        \
   }                                        \


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
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
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS((loc)), (flag)))
#define MAX_OCCUPANTS(room)       ((room)->max_occupancy)

#define ZONE_FLAGS(loc)           ((loc)->flags)
#define ZONE_FLAGGED(loc, flag)   (IS_SET(ZONE_FLAGS((loc)), (flag)))

#define SRCH_FLAGGED(srch, flag)   (IS_SET(srch->flags, flag))
#define SRCH_OK(ch, srch) \
   ((!IS_EVIL(ch) || !SRCH_FLAGGED(srch, SRCH_NOEVIL)) && \
    (!IS_NEUTRAL(ch) || !SRCH_FLAGGED(srch, SRCH_NONEUTRAL)) && \
    (!IS_GOOD(ch) || !SRCH_FLAGGED(srch, SRCH_NOGOOD)) && \
    (ch->getPosition() < POS_FLYING || !SRCH_FLAGGED(srch, SRCH_NOTRIG_FLY)) && \
    (!IS_NPC(ch) || !SRCH_FLAGGED(srch, SRCH_NOMOB)) &&        \
    ( IS_NPC(ch) || !SRCH_FLAGGED(srch, SRCH_NOPLAYER)) &&        \
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
    (!IS_HOOD(ch)   || !SRCH_FLAGGED(srch, SRCH_NOHOOD)) &&   \
    !SRCH_FLAGGED(srch, SRCH_TRIPPED))

#define IN_ICY_HELL(ch)  (ch->in_room->zone->plane == PLANE_HELL_5)
#define HELL_PLANE(zone, num)  ((zone)->plane == (PLANE_HELL_1 - 1 + num))

/******************* SHADOW PLANE STUFF *******************/
#define SHADOW_ZONE(zone)     ((zone)->plane == PLANE_SHADOW)
#define ZONE_IS_SHADE(zone)   ((zone)->number == 198)
/***************** END  SHADOW PLANE STUFF *****************/

#define ZONE_IS_HELL(zone) \
                          (zone->plane >= PLANE_HELL_1 && zone->plane <= PLANE_HELL_9)

#define NOGRAV_ZONE(zone) ((zone->plane >= PLANE_ELEM_WATER && \
                             zone->plane <= PLANE_ELEM_NEG) || \
                            zone->plane == PLANE_ASTRAL || zone->plane == PLANE_PELEM_MAGMA \
                            || zone->plane == PLANE_PELEM_OOZE )

/*  character utils ******************************************************/
#define MOB_FLAGS(ch)  ((ch)->char_specials.saved.act)
#define MOB2_FLAGS(ch) ((ch)->char_specials.saved.act2)
#define PLR_FLAGS(ch)  ((ch)->char_specials.saved.act)
#define PLR2_FLAGS(ch) ((ch)->player_specials->saved.plr2_bits)
#define PRF_FLAGS(ch)  ((ch)->player_specials->saved.pref)
#define PRF2_FLAGS(ch) ((ch)->player_specials->saved.pref2)
#define AFF_FLAGS(ch)  ((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected2_by)
#define AFF3_FLAGS(ch) ((ch)->char_specials.saved.affected3_by)

#define CHAR_SOILAGE(ch, pos) (ch->player_specials->soilage[pos])
#define CHAR_SOILED(ch, pos, soil) (IS_SET(CHAR_SOILAGE(ch, pos), soil))

#define ILLEGAL_SOILPOS(pos) \
     (pos == WEAR_LIGHT || pos == WEAR_SHIELD || pos == WEAR_ABOUT || \
      pos == WEAR_WIELD || pos == WEAR_HOLD || pos == WEAR_BELT ||    \
      pos == WEAR_WIELD_2 || pos == WEAR_ASS || pos == WEAR_NECK_2 || \
      pos == WEAR_RANDOM)

#define IS_WEAR_EXTREMITY(pos) \
    (        pos == WEAR_FINGER_L || pos == WEAR_FINGER_R \
     || pos == WEAR_LEGS     || pos == WEAR_FEET     || pos == WEAR_BELT \
     || pos == WEAR_EAR_L    || pos == WEAR_EAR_R    || pos == WEAR_ARMS \
     || pos == WEAR_HANDS    || pos == WEAR_HOLD     || pos == WEAR_WIELD \
     || pos == WEAR_SHIELD   || pos == WEAR_WRIST_L  || pos == WEAR_WRIST_R \
         || pos == WEAR_WAIST         || pos == WEAR_CROTCH)

#define IS_WEAR_STRIKER(pos) \
    (        pos == WEAR_FINGER_L || pos == WEAR_FINGER_R \
     || pos == WEAR_LEGS     || pos == WEAR_FEET \
     || pos == WEAR_EAR_L    || pos == WEAR_EAR_R    || pos == WEAR_ARMS \
     || pos == WEAR_HANDS    || pos == WEAR_WRIST_L  || pos == WEAR_WRIST_R \
         || pos == WEAR_WAIST          || pos == WEAR_HEAD)

#define ILLEGAL_IMPLANTPOS(pos) \
     (pos == WEAR_LIGHT || pos == WEAR_SHIELD || pos == WEAR_ABOUT || \
      pos == WEAR_WIELD || pos == WEAR_BELT ||    \
      pos == WEAR_WIELD_2 || pos == WEAR_RANDOM)

#define GET_ROWS(ch)    ((ch)->player_specials->saved.page_length)
#define GET_COLS(ch)    ((ch)->player_specials->saved.columns)

#define NOHASS(ch)       (PRF_FLAGGED(ch, PRF_NOHASSLE))

#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch))
#define IS_PC(ch)   (!IS_NPC(ch))

#define MOB_FLAGGED(ch, flag)   (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define MOB2_FLAGGED(ch, flag)  (IS_NPC(ch) && IS_SET(MOB2_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag)   (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define PLR2_FLAGGED(ch, flag)   (!IS_NPC(ch) && IS_SET(PLR2_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag)   (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag)  (IS_SET(AFF2_FLAGS(ch), (flag)))
#define AFF3_FLAGGED(ch, flag)  (IS_SET(AFF3_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag)   (IS_SET(PRF_FLAGS(ch), (flag)))
#define PRF2_FLAGGED(ch, flag)  (IS_SET(PRF2_FLAGS(ch), (flag)))

#define WIS_APP(k)         (k >> 1)
#define INT_APP(k)         (k << 1)
#define BACKSTAB_MULT(ch)  (((GET_LEVEL(ch) + 1) / 5) + GET_REMORT_GEN(ch))
#define GET_MORALE(ch)     (ch->mob_specials.shared->morale)
#define MOB_SHARED(ch)     (ch->mob_specials.shared)

#define IS_PET(ch)       (MOB_FLAGGED(ch, MOB_PET))
#define IS_SOULLESS(ch) (MOB_FLAGGED(ch, MOB_SOULLESS) || PLR2_FLAGGED(ch, PLR2_SOULLESS))
#define HAS_SYMBOL(ch) (IS_SOULLESS(ch) || affected_by_spell(ch, SPELL_STIGMATA) \
                        || IS_AFFECTED_3(ch, AFF3_SYMBOL_OF_PAIN) \
                        || IS_AFFECTED_3(ch, AFF3_TAINTED))
// IS_AFFECTED for backwards compatibility
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))
#define IS_AFFECTED_2(ch, skill) (AFF2_FLAGGED((ch), (skill)))
#define IS_AFFECTED_3(ch, skill) (AFF3_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF2_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))

#define CHAR_WITHSTANDS_RAD(ch)   (PRF_FLAGGED(ch, PRF_NOHASSLE) || \
                                   AFF2_FLAGGED(ch, AFF2_PROT_RAD))

#define CHAR_WITHSTANDS_FIRE(ch)  (GET_LEVEL(ch) >= LVL_IMMORT      ||  \
                                  IS_AFFECTED_2(ch, AFF2_PROT_FIRE) ||  \
                                        GET_CLASS(ch) == CLASS_FIRE ||  \
                      (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_RED) ||  \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_4 || \
                               GET_PLANE(ch->in_room) == PLANE_HELL_6)))

#define CHAR_WITHSTANDS_COLD(ch)  (GET_LEVEL(ch) >= LVL_IMMORT        || \
                                   IS_VAMPIRE(ch) || \
                                  IS_AFFECTED_2(ch, AFF2_ENDURE_COLD) || \
                                  IS_AFFECTED_2(ch, AFF2_ABLAZE)      || \
                                   (ch->getPosition() == POS_SLEEPING &&       \
                                    AFF3_FLAGGED(ch, AFF3_STASIS))    || \
                      (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_WHITE) || \
                        IS_UNDEAD(ch) || GET_CLASS(ch) == CLASS_FROST || \
                                                         IS_SLAAD(ch) || \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_5 || \
                               GET_PLANE(ch->in_room) == PLANE_HELL_8)))

#define CHAR_WITHSTANDS_HEAT(ch)  (GET_LEVEL(ch) >= LVL_IMMORT        || \
                                   IS_AFFECTED_3(ch, AFF3_PROT_HEAT)   || \
                                   (ch->getPosition() == POS_SLEEPING &&       \
                                    AFF3_FLAGGED(ch, AFF3_STASIS))    || \
                        (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_RED) || \
                         IS_UNDEAD(ch) || GET_CLASS(ch) == CLASS_FIRE || \
                          IS_SLAAD(ch) || \
                           (GET_MOB_VNUM(ch) >= 16100 && \
                            GET_MOB_VNUM(ch) <= 16699) || \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_4 || \
                                   GET_PLANE(ch->in_room) == PLANE_HELL_6)))

#define CHAR_WITHSTANDS_ELECTRIC(ch)   \
                                (IS_AFFECTED_2(ch, AFF2_PROT_LIGHTNING) || \
                                 IS_VAMPIRE(ch) || \
                           (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_BLUE))

#define NEEDS_TO_BREATHE(ch) \
     (!AFF3_FLAGGED(ch, AFF3_NOBREATHE) && \
      !IS_UNDEAD(ch) && (GET_MOB_VNUM(ch) <= 16100 || \
                         GET_MOB_VNUM(ch) > 16999))

#define NULL_PSI(ch) \
     (IS_UNDEAD(vict) || IS_SLIME(vict) || IS_PUDDING(vict) || \
      IS_ROBOT(vict) || IS_PLANT(vict))

#define COMM_NOTOK_ZONES(ch, tch) \
     (GET_LEVEL(ch) < LVL_IMMORT &&                                        \
      ch->in_room != tch->in_room &&                                       \
      (ch->in_room->zone->plane != tch->in_room->zone->plane ||            \
       (ch->in_room->zone != tch->in_room->zone &&                         \
        (ZONE_FLAGGED(ch->in_room->zone,                                   \
                      ZONE_ISOLATED | ZONE_SOUNDPROOF) ||                  \
         ZONE_FLAGGED(tch->in_room->zone,                                  \
                      ZONE_ISOLATED | ZONE_SOUNDPROOF))) ||                \
       (ch->in_room->zone->time_frame != tch->in_room->zone->time_frame && \
        ch->in_room->zone->time_frame != TIME_TIMELESS &&                  \
        tch->in_room->zone->time_frame != TIME_TIMELESS) ||                \
       ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) ||                       \
       ROOM_FLAGGED(tch->in_room, ROOM_SOUNDPROOF)))


	  /* room utils *********************************************************** */


static inline int
SECT(room_data * room)
{
	return room->sector_type;
}

/*
#define IS_DARK(room)  (!room->light && \
                        (ROOM_FLAGGED(room, ROOM_DARK) || \
                          ((!ROOM_FLAGGED(room, ROOM_INDOORS) && \
                            PRIME_MATERIAL_ROOM(room) && \
                            SECT(room) != SECT_INSIDE && \
                              SECT(room) != SECT_VEHICLE && \
                              SECT(room) != SECT_CITY ) && \
                            ((room)->zone->weather->sunlight == SUN_SET || \
                             (room)->zone->weather->sunlight == SUN_DARK)) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))
*/
static inline bool
IS_DARK(room_data * room)
{
	if (room->light)
		return false;
	if (SECT(room) == SECT_ELEMENTAL_OOZE)
		return true;
	if (ROOM_FLAGGED(room, ROOM_DARK))
		return true;
	if (!PRIME_MATERIAL_ROOM(room))
		return false;
	if (ROOM_FLAGGED(room, ROOM_INDOORS))
		return false;
	if (SECT(room) == SECT_INSIDE)
		return false;
	if (SECT(room) == SECT_VEHICLE)
		return false;
	if (SECT(room) == SECT_CITY)
		return false;
	int sunlight = (room)->zone->weather->sunlight;
	return (sunlight == SUN_SET || sunlight == SUN_DARK);
}
static inline bool
IS_LIGHT(room_data * room)
{
	return !(IS_DARK(room));
}

#define GET_ROOM_SPEC(room) ((room) != NULL ? (room)->func : NULL)

/* char utils ************************************************************/


#define IN_ROOM(ch)        ((ch)->in_room)
#define GET_WAS_IN(ch)        ((ch)->player_specials->was_in_room)
#define GET_AGE(ch)     (MAX(age(ch).year + ch->player.age_adjust, 17))

#define GET_NAME(ch)    (IS_NPC(ch) ? \
                         (ch)->player.short_descr : (ch)->player.name)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)        ((ch)->player.passwd)
#define GET_PFILEPOS(ch)((ch)->pfilepos)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.char_class)
#define GET_REMORT_CLASS(ch)  ((ch)->player.remort_char_class)
#define CHECK_REMORT_CLASS(ch)  ((ch)->player.remort_char_class)
#define GET_RACE(ch)        ((ch)->player.race)
#define GET_HOME(ch)        ((ch)->player.hometown)
#define GET_HEIGHT(ch)        ((ch)->player.height)
#define GET_WEIGHT(ch)        ((ch)->player.weight)
#define GET_SEX(ch)        ((ch)->player.sex)
#define IS_MALE(ch)     ((ch)->player.sex == SEX_MALE)
#define IS_FEMALE(ch)   ((ch)->player.sex == SEX_FEMALE)
#define IS_REMORT(ch)   ((ch) && (CHECK_REMORT_CLASS(ch) >= 0))

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)          ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)          ((ch)->points.hit)
#define GET_MAX_HIT(ch)          ((ch)->points.max_hit)
#define GET_MOVE(ch)          ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)          ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)          ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_ECONET(ch)    ((ch)->points.credits)
#define GET_CASH(ch)      ((ch)->points.cash)

#define BANK_MONEY(ch) (ch->in_room->zone->time_frame == TIME_ELECTRO ? \
                        GET_ECONET(ch) : GET_BANK_GOLD(ch))
#define CASH_MONEY(ch) (ch->in_room->zone->time_frame == TIME_ELECTRO ? \
                    GET_CASH(ch) : GET_GOLD(ch))
char *CURRENCY(char_data * ch);

#define GET_HITROLL(ch)          ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

#define GET_REAL_HITROLL(ch)   \
  ((GET_HITROLL(ch) <= 5) ?    \
   GET_HITROLL(ch) :           \
   ((GET_HITROLL(ch) <= 50) ?  \
    (5 + (((GET_HITROLL(ch) - 5)) / 3)) : 20))

#define GET_IDNUM(ch)          ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define IS_WEARING_W(ch)  ((ch)->char_specials.worn_weight)
#define TOTAL_ENCUM(ch)   (IS_CARRYING_W(ch) + IS_WEARING_W(ch))
#define GET_WEAPON_PROF(ch)  ((ch)->char_specials.weapon_proficiency)
#define CHECK_WEAPON_PROF(ch)  ((ch)->char_specials.weapon_proficiency)
#define MEDITATE_TIMER(ch)  (ch->char_specials.meditate_timer)
#define CHAR_CUR_PULSE(ch)  (ch->char_specials.cur_flow_pulse)
#define GET_FALL_COUNT(ch)     ((ch)->char_specials.fall_count)

#define FIGHTING(ch)          (ch->getFighting())
#define HUNTING(ch)          ((ch)->char_specials.hunting)
#define MOUNTED(ch)          ((ch)->char_specials.mounted)
#define DRIVING(ch)       ((ch)->char_specials.driving)
#define GET_SAVE(ch, i)          ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COND(ch, i)                ((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)        ((ch)->player_specials->saved.load_room)
#define GET_PRACTICES(ch)        ((ch)->player_specials->saved.spells_to_learn)
#define GET_LIFE_POINTS(ch)     ((ch)->player_specials->saved.life_points)
#define GET_MOBKILLS(ch)        ((ch)->player_specials->saved.mobkills)
#define GET_MSHIELD_LOW(ch)     ((ch)->player_specials->saved.mana_shield_low)
#define GET_MSHIELD_PCT(ch)     ((ch)->player_specials->saved.mana_shield_pct)
#define GET_PKILLS(ch)          ((ch)->player_specials->saved.pkills)
#define GET_PC_DEATHS(ch)       ((ch)->player_specials->saved.deaths)
#define GET_PAGE_LENGTH(ch)     ((ch)->player_specials->saved.page_length)
#define GET_INVIS_LEV(ch)        ((ch)->player_specials->saved.invis_level)
#define GET_BROKE(ch)           ((ch)->player_specials->saved.broken_component)
#define GET_REMORT_INVIS(ch)  ((ch)->player_specials->saved.remort_invis_level)
#define GET_OLD_CLASS(ch)       ((ch)->player_specials->saved.old_char_class)
#define GET_TOT_DAM(ch)         ((ch)->player_specials->saved.total_dam)
#define GET_WIMP_LEV(ch)        ((ch)->player_specials->saved.wimp_level)
#define GET_WEAP_SPEC(ch, i)    ((ch)->player_specials->saved.weap_spec[(i)])
#define GET_DEITY(ch)                ((ch)->player_specials->saved.deity)
#define GET_CLAN(ch)                ((ch)->player_specials->saved.clan)
#define GET_HOLD_HOME(ch)        ((ch)->player_specials->saved.hold_home)
#define GET_HOLD_LOADROOM(ch)        ((ch)->player_specials->saved.hold_load_room)
#define GET_REMORT_GEN(ch)     ((ch)->player_specials->saved.remort_generation)
#define GET_QUEST_POINTS(ch)    ((ch)->player_specials->saved.quest_points)
#define GET_QUEST_ALLOWANCE(ch) ((ch)->player_specials->saved.qp_allowance)
#define GET_QLOG_LEVEL(ch)     ((ch)->player_specials->saved.qlog_level)
#define GET_QUEST(ch)           ((ch)->player_specials->saved.quest_id)
#define GET_FREEZE_LEV(ch)        ((ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)                ((ch)->player_specials->saved.bad_pws)
#define POOFIN(ch)                ((ch)->player_specials->poofin)
#define POOFOUT(ch)                ((ch)->player_specials->poofout)
#define GET_IMPRINT_ROOM(ch,i)  ((ch)->player_specials->imprint_rooms[i])
#define GET_LAST_OLC_TARG(ch)        ((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)        ((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)                ((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)        ((ch)->player_specials->last_tell)
#define GET_OLC_OBJ(ch)         ((ch)->player_specials->olc_obj)
#define GET_OLC_MOB(ch)         ((ch)->player_specials->olc_mob)
#define GET_OLC_SHOP(ch)        ((ch)->player_specials->olc_shop)
#define GET_OLC_HELP(ch)        ((ch)->player_specials->olc_help_item)
#define GET_OLC_SRCH(ch)        ((ch)->player_specials->olc_srch)
#define GET_OLC_TICL(ch)        ((ch)->player_specials->olc_ticl)
#define GET_OLC_ISCR(ch)        ((ch)->player_specials->olc_iscript)
#define GET_OLC_HANDLER(ch)     ((ch)->player_specials->olc_handler)

#define SET_SKILL(ch, i, pct)        \
                             {(ch)->player_specials->saved.skills[i] = pct; }
#define SET_SKILL(ch, i, pct)        \
                             {(ch)->player_specials->saved.skills[i] = pct; }

#define GET_EQ(ch, i)                ((ch)->equipment[i])
#define GET_IMPLANT(ch, i)      ((ch)->implants[i])

#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (ch->mob_specials.shared->func) : NULL)
#define GET_MOB_VNUM(mob)        (IS_MOB(mob) ? \
                                      mob->mob_specials.shared->vnum : -1)

#define GET_MOB_WAIT(ch)        ((ch)->mob_specials.wait_state)
#define GET_MOB_LAIR(ch)        ((ch)->mob_specials.shared->lair)
#define GET_MOB_LEADER(ch)        ((ch)->mob_specials.shared->leader)
#define GET_DEFAULT_POS(ch)        ((ch)->mob_specials.shared->default_pos)
#define MEMORY(ch)                ((ch)->mob_specials.memory)
#define MOB_IDNUM(ch)           ((ch)->mob_specials.mob_idnum)

#define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 10) ? 26 :( \
          (GET_ADD(ch) <= 20) ? 27 :( \
          (GET_ADD(ch) <= 30) ? 28 :( \
          (GET_ADD(ch) <= 40) ? 29 :( \
          (GET_ADD(ch) <= 50) ? 30 :( \
          (GET_ADD(ch) <= 60) ? 31 :( \
          (GET_ADD(ch) <= 70) ? 32 :( \
          (GET_ADD(ch) <= 80) ? 33 :( \
          (GET_ADD(ch) <= 90) ? 34 :( \
          (GET_ADD(ch) <= 99) ? 35 :  36 ) ) ) ) ) ) ) ) )                    \
       )

#define CAN_CARRY_W(ch) (MAX(10, RAW_CARRY_W(ch)))

#define RAW_CARRY_W(ch) (IS_AFFECTED_2(ch, AFF2_TELEKINESIS) ? \
                         (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w << 1) : \
                         str_app[STRENGTH_APPLY_INDEX(ch)].carry_w + \
                         ((GET_LEVEL(ch) >= LVL_GOD) ? 10000 : 0))
#define CAN_CARRY_N(ch) (1 + GET_DEX(ch) + \
                         ((GET_LEVEL(ch) >= LVL_GOD) ? 1000 : 0) + \
                         (IS_AFFECTED_2(ch, AFF2_TELEKINESIS) ? \
                          (GET_LEVEL(ch) >> 2) : 0))

#define AWAKE(ch) (ch->getPosition() > POS_SLEEPING && !IS_AFFECTED_2(ch, AFF2_MEDITATE))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_SICK(ch)    (IS_AFFECTED_3(ch, AFF3_SICKNESS))
#define IS_HAMSTRUNG(ch)    (IS_AFFECTED_3(ch, AFF3_HAMSTRUNG))
#define IS_SNIPED(ch)    (IS_AFFECTED_3(ch, AFF3_SNIPED))
#define HAS_POISON_1(ch) (IS_AFFECTED(ch, AFF_POISON))
#define HAS_POISON_2(ch) (IS_AFFECTED_3(ch, AFF3_POISON_2))
#define HAS_POISON_3(ch) (IS_AFFECTED_3(ch, AFF3_POISON_3))
#define IS_POISONED(ch) (HAS_POISON_1(ch) || HAS_POISON_2(ch) || \
                         HAS_POISON_3(ch))
#define IS_CONFUSED(ch)  (IS_AFFECTED(ch, AFF_CONFUSION) \
                          || IS_AFFECTED_3(ch, AFF3_SYMBOL_OF_PAIN))
#define THACO(char_class, level) \
     ((char_class < 0) ? 20 :                             \
      ((char_class < NUM_CLASSES) ?                       \
       (20 - (level * thaco_factor[char_class])) :        \
       (20 - (level * thaco_factor[CLASS_WARRIOR]))))

/* descriptor-based utils ************************************************/


#define CHECK_WAIT(ch)        (((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)        ((d)->connected)


/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)        ((obj)->obj_flags.type_flag)
#define IS_OBJ_TYPE(obj, type)  (GET_OBJ_TYPE(obj) == type)
#define GET_OBJ_COST(obj)        ((obj)->shared->cost)
// can only be used to get the rent, not set
#define GET_OBJ_RENT(obj)        ((obj)->plrtext_len ? \
                                 ((obj)->plrtext_len << 3) : \
                                 (obj)->shared->cost_per_day)

#define GET_OBJ_EXTRA(obj)        ((obj)->obj_flags.extra_flags)
#define GET_OBJ_EXTRA2(obj)        ((obj)->obj_flags.extra2_flags)
#define GET_OBJ_EXTRA3(obj)        ((obj)->obj_flags.extra3_flags)
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
#define OBJ_CUR_PULSE(obj)      ((obj)->cur_flow_pulse)
#define GET_OBJ_VNUM(obj)        ((obj)->shared->vnum)
#define IS_OBJ_STAT(obj,stat)        (IS_SET((obj)->obj_flags.extra_flags,stat))
#define IS_OBJ_STAT2(obj,stat)        (IS_SET((obj)->obj_flags.extra2_flags,stat))
#define IS_OBJ_STAT3(obj,stat)        (IS_SET((obj)->obj_flags.extra3_flags,stat))

#define GET_OBJ_SPEC(obj) ((obj) ? (obj)->shared->func : NULL)

#define OBJ_SOILAGE(obj)        (obj->soilage)
#define OBJ_SOILED(obj, soil)    (IS_SET(OBJ_SOILAGE(obj), soil))

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

#define IS_TWO_HAND(obj) (IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED))
#define IS_NODROP(obj)   (IS_OBJ_STAT(obj, ITEM_NODROP))
#define IS_THROWN(obj)   (IS_OBJ_STAT2(obj, ITEM2_THROWN_WEAPON))

#define STAB_WEAPON(obj) (IS_OBJ_TYPE(obj, ITEM_WEAPON) && \
                          (GET_OBJ_VAL(obj, 3) + TYPE_HIT == TYPE_STAB || \
                           GET_OBJ_VAL(obj, 3) + TYPE_HIT == TYPE_PIERCE))

#define IS_CORPSE(obj)  (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
                         GET_OBJ_VAL(obj, 3))

#define IS_BODY_PART(obj)  (IS_OBJ_STAT2(obj, ITEM2_BODY_PART))

#define OBJ_TYPE(obj, type) (GET_OBJ_TYPE(obj) == type)

#define OBJ_APPROVED(obj) (!IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
#define P_OBJ_APPROVED(p_obj) (!IS_SET(p_obj->obj_flags.extra2_flags, \
                                      ITEM2_UNAPPROVED))

#define OBJ_IS_RAD(obj) (IS_OBJ_STAT2(obj, ITEM2_RADIOACTIVE))

#define QUAD_VNUM  1578

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
      (IS_OBJ_STAT(obj,ITEM_ANTI_GOOD | ITEM_EVIL_BLESS) && IS_GOOD(ch)) ||\
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))

/* compound utilities and other macros **********************************/


#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

// add special plural words that don't end in S to the SPECIAL_PLUR macro
#define SPECIAL_PLUR( buf )    ( !strcasecmp( buf, "teeth" ) || \
                                 !strcasecmp( buf, "cattle" ) ||  \
                                 !strcasecmp( buf, "data" ) )
#define SPECIAL_SING( buf )    ( !strcasecmp( buf, "portcullis" ) )
#define PLUR(buf)              ( !SPECIAL_SING( buf ) && ( SPECIAL_PLUR( buf ) || buf[strlen(buf) - 1] == 's' ) )

#define ISARE(buf)             (PLUR(buf) ? "are" : "is")
#define IT_THEY(buf)           (PLUR(buf) ? "they" : "it")
#define IT_THEM(buf)           (PLUR(buf) ? "them" : "it")

/* Various macros building up to CAN_SEE */

#define GET_SKILL(ch, i)        ((ch)->player_specials->saved.skills[i])
#define KNOCKDOWN_SKILL(i) \
          (i == SPELL_EARTHQUAKE  || i == SPELL_PSYCHIC_SURGE || \
           i == SPELL_EGO_WHIP    || i == SKILL_BASH ||          \
           i == SKILL_PILEDRIVE   || i == SKILL_BODYSLAM ||      \
           i == SKILL_LUNGE_PUNCH || i == SKILL_CLOTHESLINE ||   \
           i == SKILL_SWEEPKICK   || i == SKILL_TRIP ||          \
           i == SKILL_HIP_TOSS    || i == SKILL_SHOULDER_THROW)

/*#define SKILL_NIGHT_VISION          605   (in spells.h) */
#define NIGHT_VIS(ch)         (CHECK_SKILL(ch, 605) >= 70)

#define IS_RACE_INFRA(ch) \
                ((GET_RACE(ch) == RACE_ELF) ||     \
                (GET_RACE(ch) == RACE_DROW) ||    \
                (GET_RACE(ch) == RACE_DWARF) ||    \
                (GET_RACE(ch) == RACE_HALF_ORC) || \
                (GET_RACE(ch) == RACE_TABAXI) ||   \
                (GET_RACE(ch) == RACE_UNDEAD) ||   \
                (GET_RACE(ch) == RACE_DRAGON) ||   \
                (GET_RACE(ch) == RACE_ORC) ||      \
                (GET_RACE(ch) == RACE_OGRE) ||      \
                (GET_RACE(ch) == RACE_GOBLIN) ||   \
                (GET_RACE(ch) == RACE_TROLL) ||    \
                 IS_BUGBEAR(ch) ||           \
                (GET_CLASS(ch) == CLASS_VAMPIRE && IS_EVIL(ch)))

#define CAN_SEE_IN_DARK(ch) \
     (AFF_FLAGGED(ch, AFF_INFRAVISION) || PRF_FLAGGED(ch, PRF_HOLYLIGHT) || \
      AFF_FLAGGED(ch, AFF_RETINA) || \
      IS_RACE_INFRA(ch) || NIGHT_VIS(ch))


#define LIGHT_OK(sub)        ((!IS_AFFECTED(sub, AFF_BLIND) && \
                          (IS_LIGHT((sub)->in_room) ||  \
                           CAN_SEE_IN_DARK(sub))) || \
                         AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY))

#define LIGHT_OK_ROOM(sub, room) ((!IS_AFFECTED(sub, AFF_BLIND) && \
                                 (IS_LIGHT(room) ||  \
                                 CAN_SEE_IN_DARK(sub))) || \
                                 AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY))

#define ROOM_OK(sub)    (!sub->in_room ||                   \
                         !ROOM_FLAGGED(sub->in_room, ROOM_SMOKE_FILLED))
inline bool INVIS_OK(char_data * sub, char_data * obj);
#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && ROOM_OK(sub) && \
                                INVIS_OK(sub, obj) &&     \
                                (!PLR_FLAGGED(obj, PLR_TESTER) || \
                                 PLR_FLAGGED(sub, PLR_TESTER) || \
                                 IS_NPC(sub)))

#define IMM_CAN_SEE(sub, obj) \
     (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */

#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
                           ((GET_LEVEL(sub) >= GET_INVIS_LEV(obj)) && \
                            IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define APPROVED_OK_OBJ(sub, obj)   \
     (OBJ_APPROVED(obj) ||                     \
      GET_LEVEL(sub) >= LVL_IMMORT ||          \
      PLR_FLAGGED(sub, PLR_TESTER))

#define INVIS_OK_OBJ(sub, obj) \
     ((((!IS_OBJ_STAT((obj), ITEM_INVISIBLE) ||   \
         IS_AFFECTED((sub), AFF_DETECT_INVIS)) && \
        (!IS_OBJ_STAT((obj), ITEM_TRANSPARENT) || \
         IS_AFFECTED((sub), AFF_DETECT_INVIS))) ||\
       (PRF_FLAGGED(sub, PRF_HOLYLIGHT) || \
        IS_AFFECTED_2(sub, AFF2_TRUE_SEEING))) && \
      APPROVED_OK_OBJ(sub, obj))


#define MORT_CAN_SEE_OBJ(sub, obj) (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj))


#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || PRF_FLAGGED((sub), PRF_HOLYLIGHT))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + obj->getWeight()) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   ((CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj))) || GET_LEVEL(ch) > LVL_CREATOR)

#define CAN_DETECT_DISGUISE(ch, vict, level) \
                          (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || \
                           AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) ||\
                           (GET_INT(ch)+GET_WIS(ch)) > (level+GET_CHA(vict)))

#define EXIT(ch, door)  ((ch)->in_room->dir_option[(door)])
#define _2ND_EXIT(ch, door) (EXIT((ch), (door))->to_room->dir_option[door])
#define _3RD_EXIT(ch, door) (_2ND_EXIT((ch),(door))->to_room->dir_option[door])

#define ABS_EXIT(room, door)  ((room)->dir_option[door])

bool CAN_GO(char_data * ch, int door);
bool CAN_GO(obj_data * obj, int door);


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
#define IS_HOOD(ch)               IS_CLASS(ch, CLASS_HOOD)
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
#define IS_UNDEAD(ch)                     (GET_RACE(ch) == RACE_UNDEAD || \
                                 IS_CLASS(ch, CLASS_VAMPIRE))
#define NON_CORPOREAL_MOB(ch) NON_CORPOREAL_UNDEAD(ch)  \
            || (GET_RACE(ch) == RACE_ELEMENTAL &&       \
                (                                       \
                GET_CLASS(ch) == CLASS_AIR ||           \
                GET_CLASS(ch) == CLASS_WATER ||         \
                GET_CLASS(ch) == CLASS_FIRE             \
                ))
#define NON_CORPOREAL_UNDEAD(ch) \
                                (IS_SHADOW(ch) || IS_WIGHT(ch) ||   \
                                 IS_WRAITH(ch) || IS_SPECTRE(ch) || \
                                 IS_GHOST(ch))                      \

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
#define IS_PUDDING(ch)          (GET_RACE(ch) == RACE_PUDDING)
#define IS_SLIME(ch)            (GET_RACE(ch) == RACE_SLIME)
#define IS_BUGBEAR(ch)          (GET_RACE(ch) == RACE_BUGBEAR)
#define IS_ALIEN_1(ch)          (GET_RACE(ch) == RACE_ALIEN_1)
#define IS_FISH(ch)             (GET_RACE(ch) == RACE_FISH)
#define IS_RAKSHASA(ch)         (GET_RACE(ch) == RACE_RAKSHASA)
#define IS_RACE(ch, race)       (GET_RACE(ch) == race)

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
                                 IS_RACE(ch, RACE_KOBOLD) ||    \
                                 IS_RACE(ch, RACE_MEPHIT) ||    \
                                 IS_RACE(ch, RACE_DAEMON) ||    \
                                 IS_RACE(ch, RACE_RAKSHASA))


#define IS_TIAMAT(ch)           (GET_MOB_VNUM(ch) == 61119)
#define IS_TARRASQUE(ch)           (GET_MOB_VNUM(ch) == 24800)

#define IS_LEMURE(ch)           (GET_MOB_VNUM(ch) == 16121 || \
                                 GET_MOB_VNUM(ch) == 16110 || \
                                 GET_MOB_VNUM(ch) == 16652)

#define ICY_DEVIL(ch)           (GET_MOB_VNUM(ch) == 16117 || \
                                 GET_MOB_VNUM(ch) == 16146 || \
                                 GET_MOB_VNUM(ch) == 16132)

#define LIFE_FORM(ch)           (!IS_ROBOT(ch) && !IS_UNDEAD(ch))

#define MOB_UNAPPROVED(ch)       (MOB2_FLAGGED(ch, MOB2_UNAPPROVED))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS) && \
                                        (ch)->in_room->sector_type != SECT_INSIDE )


inline bool
INVIS_OK(char_data * sub, char_data * obj)
{
	// Holy is the light that shines on the chosen
	if (PRF_FLAGGED(sub, PRF_HOLYLIGHT))
		return true;

	// Invis/Transparent
	if (IS_AFFECTED(obj, AFF_INVISIBLE)
		|| IS_AFFECTED_2(obj, AFF2_TRANSPARENT)) {
		if (!IS_AFFECTED(sub, AFF_DETECT_INVIS)
			&& !IS_AFFECTED_2(sub, AFF2_TRUE_SEEING))
			return false;
	}
	// Invis to Undead
	if (IS_UNDEAD(sub) && IS_AFFECTED_2(obj, AFF2_INVIS_TO_UNDEAD)) {
		if (!AFF2_FLAGGED(sub, AFF2_TRUE_SEEING)) {
			return false;
		}
	}
	// Invis to animals
	if (IS_ANIMAL(sub) && IS_AFFECTED_2(obj, AFF2_INVIS_TO_ANIMALS)) {
		if (!AFF_FLAGGED(sub, AFF_DETECT_INVIS)
			&& !AFF2_FLAGGED(sub, AFF2_TRUE_SEEING))
			return false;
	}
	// Remort invis. 
	// (mobs don't have it and aren't affected by it.)
	if (IS_NPC(sub) || IS_NPC(obj))
		return true;
	if (GET_LEVEL(sub) >= GET_REMORT_INVIS(obj)
		|| GET_REMORT_GEN(sub) >= GET_REMORT_GEN(obj))
		return true;

	// If none of this counts for you, you just suck.
	return false;
}

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
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
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#ifdef NOCRYPT
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

inline bool
MOB_CAN_GO(struct char_data * ch, int door)
{
	if (EXIT(ch, door) &&
		EXIT(ch, door)->to_room &&
		(!IS_SET(EXIT(ch, door)->exit_info,
				EX_CLOSED | EX_NOPASS | EX_HIDDEN) ||
			GET_LEVEL(ch) >= LVL_IMMORT || NON_CORPOREAL_UNDEAD(ch))) {
		return true;
	}
	return false;
}

//
// my_rand() returns 0 to 2147483647
//

unsigned long my_rand(void);

//
// returns a random boolean value
//
inline bool
random_binary()
{
	return (my_rand() > 1073741823);
}

//
// returns a random boolean value, true 1/num of returns
//
inline bool
random_fractional(unsigned int num)
{
	if (num == 0)
		return true;
	return (my_rand() < (2147483647 / num));
}

//
// returns a random boolean value, true 1/3 of returns (33% tru)
//
inline bool
random_fractional_3()
{
	return (my_rand() < 715827882);
}

//
// returns a random boolean value, true 1/4 of returns (25% true)
//
inline bool
random_fractional_4()
{
	return (my_rand() < 536870911);
}

//
// returns a random boolean value, true 1/5 of returns (20% true)
//
inline bool
random_fractional_5()
{
	return (my_rand() < 429496729);
}

//
// returns a random boolean value, true 1/10 of returns (10% true)
//
inline bool
random_fractional_10()
{
	return (my_rand() < 214748365);
}

//
// returns a random boolean value, true 1/20 of returns (5% true)
//
inline bool
random_fractional_20()
{
	return (my_rand() < 107374182);
}

//
// returns a random boolean value, true 1/20 of returns (5% true)
//
inline bool
random_fractional_50()
{
	return (my_rand() < 42949672);
}

//
// returns a random boolean value, true 1/20 of returns (5% true)
//
inline bool
random_fractional_100()
{
	return (my_rand() < 21474836);
}

//
// returns a random value between and including 1-100
//
inline int
random_percentage()
{
	return ((my_rand() / 21474837) + 1);
}

//
// returns a random value between and including 0-99
//
inline int
random_percentage_zero_low()
{
	return (my_rand() / 21474837);
}

//
// return a random value between 0 and num
//
inline int
random_number_zero_low(unsigned int num)
{
	return (my_rand() % (num + 1));
}

//
// creates a random number in interval [from;to]'
//
inline int
number(int from, int to)
{
	if (to <= from)
		return (from);
	return ((my_rand() % (to - from + 1)) + from);
}

//
//
//

inline double
float_number(double from, double to)
{
	double pct;
	if (to <= from)
		return (from);
	pct = (double)((double)my_rand() / 2147483647);
	if (pct > 1) {
		slog("SYSERR: float_number pct > 1.");
		return (from);
	}
	return ((double)(pct * (to - from)) + from);
}

//
// simulates dice roll
//
inline int
dice(int number, int size)
{
	int sum = 0;

	if (size <= 0 || number <= 0)
		return 0;

	while (number-- > 0)
		sum += ((my_rand() % size) + 1);

	return sum;
}

inline bool
isnumber(const char *str)
{
	while (*str)
		if (!isdigit(*str))
			return false;
		else
			str++;
	return true;
}


#endif
