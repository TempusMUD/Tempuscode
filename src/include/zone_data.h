//
// File: zone_data.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __zone_data_h__
#define __zone_data_h__


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

struct ticl_data;

/* zone definition structure. for the 'zone-table'   */
struct zone_data {
  char	*name;		    /* name of this zone                  */
  int	lifespan;           /* how long between resets (minutes)  */
  int	age;                /* current age of this zone (minutes) */
  int	top;                /* upper limit for rooms in this zone */
  
  int  reset_mode;          /* conditions for reset (see below)   */
  int  number;		    /* vnum number of this zone	  */
  int  time_frame;	    /* time-frame of zone		  */
  int  plane;
  int  owner_idnum;         /* Idnum of creator of zone           */
  int  enter_count;         /* tally of # of playrs to enter zone */
  int  flags;               /* Prevents writing to this zone      */
  int  hour_mod;            /* */
  int  year_mod; 
  int  lattitude;           /* geographic */
  unsigned char num_players; /* number of players in zone */
  unsigned short int idle_time; /* num tics idle */

  struct room_data *world; /* Pointer to first room in world      */
  struct reset_com *cmd;   /* command table for reset	          */
  struct weather_data *weather; /* zone weather                   */
  struct ticl_data *ticl_list; /* Pointer to linked list of TICLs */
  struct zone_data *next;  /* Pointer to next zone in list        */
  
  /*
   *  Reset mode:                              *
   *  0: Don't reset, and don't update age.    *
   *  1: Reset if no PC's are located in zone. *
   *  2: Just reset.                           *
   */
};


/* for queueing zones for update   */
struct reset_q_element {
   struct zone_data *zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};

/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};


#endif __zone_data_h__



