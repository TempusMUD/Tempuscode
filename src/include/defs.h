//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __defs_h__
#define __defs_h__

/* preamble *************************************************************/

#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL_NONE   0
#define SPECIAL_DEATH  1

#define SPECIAL(name) \
int (name)(struct char_data *ch, void *me, int cmd, char *argument, int spec_mode)

#define GET_SCRIPT_VNUM(mob)   (IS_MOB(mob) ? \
                              mob->mob_specials.shared->svnum : -1)

  /* Zone flags */
#define ZONE_AUTOSAVE           (1 << 0)
#define ZONE_RESETSAVE          (1 << 1)
#define ZONE_NOTIFYOWNER        (1 << 2)
#define ZONE_LOCKED             (1 << 3)
#define ZONE_NOMAGIC            (1 << 4)
#define ZONE_NOLAW              (1 << 5)
#define ZONE_NOWEATHER          (1 << 6)
#define ZONE_NOCRIME            (1 << 7)
#define ZONE_FROZEN             (1 << 8)
#define ZONE_ISOLATED           (1 << 9)
#define ZONE_SOUNDPROOF         (1 << 10)
#define ZONE_NOIDLE             (1 << 11)
#define ZONE_FULLCONTROL        (1 << 12)
#define NUM_ZONE_FLAGS          13
#define ZONE_SEARCH_APPROVED    (1 << 19)
#define ZONE_MOBS_APPROVED      (1 << 20)
#define ZONE_OBJS_APPROVED      (1 << 21)
#define ZONE_ROOMS_APPROVED     (1 << 22)
#define ZONE_ZCMDS_APPROVED     (1 << 23)
#define ZONE_SHOPS_APPROVED     (1 << 24)
#define ZONE_MOBS_MODIFIED      (1 << 25)
#define ZONE_OBJS_MODIFIED      (1 << 26)
#define ZONE_ROOMS_MODIFIED     (1 << 27)
#define ZONE_ZONE_MODIFIED      (1 << 28)
#define ZONE_TICL_APPROVED      (1 << 29)
#define ZONE_TICL_MODIFIED      <1 << 30)
#define TOT_ZONE_FLAGS          29


#define PLANE_PRIME_1        0
#define PLANE_PRIME_2        1
#define PLANE_NEVERWHERE     2
#define PLANE_UNDERDARK      3
#define PLANE_WESTERN        4
#define PLANE_MORBIDIAN      5
#define MAX_PRIME_PLANE      9
#define PLANE_ASTRAL	    10
#define PLANE_HELL_1	    11
#define PLANE_HELL_2	    12
#define PLANE_HELL_3	    13
#define PLANE_HELL_4	    14
#define PLANE_HELL_5	    15
#define PLANE_HELL_6	    16
#define PLANE_HELL_7	    17
#define PLANE_HELL_8	    18
#define PLANE_HELL_9	    19
#define PLANE_GHENNA	    20
#define PLANE_ABYSS         25
#define PLANE_OLC           39
#define PLANE_OLYMPUS       40
#define PLANE_COSTAL        41
#define PLANE_HEAVEN        43
#define PLANE_DOOM          50
#define PLANE_ELEM_WATER    70
#define PLANE_ELEM_FIRE     71
#define PLANE_ELEM_AIR      72
#define PLANE_ELEM_EARTH    73
#define PLANE_ELEM_POS      74
#define PLANE_ELEM_NEG      75

#define TIME_TIMELESS       0
#define TIME_MODRIAN        1
#define TIME_ELECTRO        2


/* other miscellaneous defines *******************************************/

/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (4 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define SEG_VIOLENCE    (7)
#define SEG_QUEUE    (7)
#define FIRE_TICK       (3 RL_SEC)
#define PULSE_FLOWS     (1 RL_SEC)

#define SMALL_BUFSIZE		1024
#define LARGE_BUFSIZE		(12 * 1024)
#define GARBAGE_SPACE		32

#define MAX_STRING_LENGTH	65536
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		150 
#define MAX_POOF_LENGTH         256 /*Used in char_file_u *DO*NOT*CHANGE**/
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	60  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		63  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_CHAR_DESC           1023 // used in char_file_u
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		700 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		96  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6 /* Used in obj_file_elem *DO*NOT*CHANGE* */

/***********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef struct Link {
  int          type;
  int          flags;
  void         *object;
  struct Link  *prev;
  struct Link  *next;
} Link;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
  char	*keyword;                 /* Keyword in look/examine          */
  char	*description;             /* What to see                      */
  struct extra_descr_data *next; /* Next in list                      */
};


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   byte hours, day, month;
   sh_int year;
};


/* ====================================================================== */

/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};

#endif __defs_h__


