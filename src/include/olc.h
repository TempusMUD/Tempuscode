#ifndef _OLC_H_
#define _OLC_H_

//
// File: olc.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

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
	long pos;
	char *text;
};


#define MOB_D1(lev)    (lev + 1)
#define MOB_D2(lev)    (6 + (lev >> 1))
#define MOB_MOD(lev)   (((lev*lev*lev*lev*lev) >> 15) + lev + 6)
void set_physical_attribs(struct Creature *ch);
//recalculates the given mob prototype's statistics based on it's current level.
void recalculate_based_on_level(Creature *mob_p);
void recalc_all_mobs(Creature *ch, const char *argument);


bool CAN_EDIT_ZONE(Creature *ch, struct zone_data *zone);
bool OLC_EDIT_OK(Creature *ch, struct zone_data *zone, int bits);

#define OLCGOD(ch) ( PLR_FLAGGED(ch, PLR_OLCGOD) )

bool OLCIMP(Creature * ch);

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
	  obj->aliases = obj_p->aliases;                                    \
	  obj->name = obj_p->name;          \
	}                                                             \
	obj->line_desc = obj_p->line_desc ;                        \
	obj->ex_description = obj_p->ex_description;                  \
        obj->action_desc = obj_p->action_desc;          \
      }                                                               \
    }

#define UPDATE_MOBLIST_NAMES(mob_p, tmp_mob, _item)                              \
    CreatureList::iterator cit = characterList.begin();                         \
    for( ; cit != characterList.end(); ++cit ) {                                 \
        tmp_mob = *cit;                                                          \
      if (IS_NPC(tmp_mob) && (tmp_mob->mob_specials.shared->vnum ==              \
      mob_p->mob_specials.shared->vnum) && !MOB2_FLAGGED(tmp_mob, MOB2_RENAMED)) \
        (tmp_mob)_item = (mob_p)_item;                                           \
    }

#define UPDATE_MOBLIST(mob_p, tmp_mob, _item)                            \
    CreatureList::iterator cit = characterList.begin();                 \
    for( ; cit != characterList.end(); ++cit ) {                         \
        tmp_mob = *cit;                                                  \
       if (IS_NPC(tmp_mob) && (tmp_mob->mob_specials.shared->vnum ==     \
           mob_p->mob_specials.shared->vnum))                            \
         (tmp_mob)_item = (mob_p)_item;                                  \
    }

#define OLC_RSET_USAGE "Usage:\r\n"                                  \
                  "olc rset title <title>\r\n"                       \
		  "olc rset desc\r\n"                                \
		  "olc rset sector <sector type>\r\n"                \
                  "olc rset flags [- | +] <flags>\r\n"               \
                  "olc rset sound [remove]\r\n"                      \
                  "olc rset flow [remove] <dir> <speed> <type>\r\n"  \
                  "olc rset occupancy <maximum>\r\n" \
				  "olc rset specparam <param>\r\n"

#define OLC_EXDESC_USAGE "olc <r|o>exdesc <create | remove | edit | addkey>" \
		   "<keywords> [new keywords]\r\n"


void
 print_search_data_to_buf(struct Creature *ch, struct room_data *room,
	struct special_search_data *cur_search, char *buf);

void show_olc_help(struct Creature *ch, char *arg);
int mobile_experience(struct Creature *mob, FILE *outfile = NULL);


#endif							// __OLC_H__
