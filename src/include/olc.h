//
// File: olc.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __OLC_H__
#define __OLC_H__

#define OLC_SET		0
#define OLC_SHOW	1
#define OLC_REPEAT	2
#define OLC_ROOM	3
#define OLC_MOB		4
#define OLC_OBJ		5

#define OLC_COPY	0
#define OLC_NAME	1
#define OLC_DESC	2
#define OLC_ALIASES	3


#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	1024
#define MAX_MOB_DESC	512
#define MAX_OBJ_DESC	512

#define SCMD_OLC        666

#define OLC_OSET    0
#define NORMAL_OSET 1

struct olc_help_r {
  char *keyword;
  long  pos;
  char *text;
};


struct ticl_data {
  int    vnum;             /* VNUM of this proc */
  long   proc_id;          /* IDNum of this proc */
  long   creator;          /* IDNum of the creator */
  time_t date_created;     /* Date proc created */
  time_t last_modified;    /* Date last modified */
  long   last_modified_by; /* IDNum of char to last modify */
  int    times_executed;   /* Number of times proc run */
  int    flags;            /* MOB/OBJ/ROOM/ZONE/!APPROVED */
  int    compiled;         /* Indicates successfull interpretation */
  char  *title;            /* Name of proc */
  char  *code;             /* TICL instructions (code) */
  struct ticl_data *next;  /* Pointer to next TICL proc */
};


#define MOB_D1(lev)    (lev + 1)
#define MOB_D2(lev)    (6 + (lev >> 1))
#define MOB_MOD(lev)   (((lev*lev*lev*lev*lev) >> 15) + lev + 6)

int CAN_EDIT_ZONE(CHAR *ch, struct zone_data *zone);
int OLC_EDIT_OK( CHAR *ch, struct zone_data *zone, int bits );

#define OLCGOD(ch) ( PLR_FLAGGED(ch, PLR_OLCGOD) )
#define OLCIMP(ch) ( GET_LEVEL(ch) >= LVL_CREATOR )

#define UPDATE_OBJLIST_NAMES(obj_p, tmp_obj, _item)                         \
     for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)   \
       if (tmp_obj->shared->vnum == obj_p->shared->vnum &&     \
	   !IS_OBJ_STAT2(tmp_obj, ITEM2_RENAMED))                    \
         (tmp_obj)_item = (obj_p)_item;

#define UPDATE_OBJLIST(obj_p, tmp_obj, _item)                         \
     for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)    \
       if (tmp_obj->shared->vnum == obj_p->shared->vnum)        \
         (tmp_obj)_item = (obj_p)_item;

#define UPDATE_OBJLIST_FULL(obj_p, obj)                               \
    for (obj = object_list; obj; obj = obj->next) {                   \
      if (obj->shared->vnum == obj_p->shared->vnum) {           \
	if (!IS_OBJ_STAT2(obj, ITEM2_RENAMED)) {                      \
	  obj->name = obj_p->name;                                    \
	  obj->short_description = obj_p->short_description;          \
	}                                                             \
	obj->description = obj_p->description;                        \
	obj->ex_description = obj_p->ex_description;                  \
        obj->action_description = obj_p->action_description;          \
      }                                                               \
    }

#define UPDATE_MOBLIST_NAMES(mob_p, tmp_mob, _item)                         \
     for (tmp_mob = character_list; tmp_mob; tmp_mob = tmp_mob->next)\
       if (IS_NPC(tmp_mob) && (tmp_mob->mob_specials.shared->vnum ==\
           mob_p->mob_specials.shared->vnum) &&                    \
	   !MOB2_FLAGGED(tmp_mob, MOB2_RENAMED))                     \
         (tmp_mob)_item = (mob_p)_item;

#define UPDATE_MOBLIST(mob_p, tmp_mob, _item)                         \
     for (tmp_mob = character_list; tmp_mob; tmp_mob = tmp_mob->next) \
       if (IS_NPC(tmp_mob) && (tmp_mob->mob_specials.shared->vnum ==                   \
           mob_p->mob_specials.shared->vnum))                       \
         (tmp_mob)_item = (mob_p)_item;


#define OLC_RSET_USAGE "Usage:\r\n"                                  \
                  "olc rset title <title>\r\n"                       \
		  "olc rset desc\r\n"                                \
		  "olc rset sector <sector type>\r\n"                \
                  "olc rset flags [- | +] <flags>\r\n"               \
                  "olc rset sound [remove]\r\n"                      \
                  "olc rset flow [remove] <dir> <speed> <type>\r\n"  \
                  "olc rset occupancy <maximum>\r\n"

#define OLC_EXDESC_USAGE "olc <r|o>exdesc <create | remove | edit | addkey>" \
		   "<keywords> [new keywords]\r\n"


void
print_search_data_to_buf(struct char_data *ch, struct room_data *room,
			 struct special_search_data *cur_search, char *buf);

void    show_olc_help(struct char_data *ch, char *arg);
int mobile_experience(struct char_data *mob);


#endif // __OLC_H__
