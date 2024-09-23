#ifndef _OLC_H_
#define _OLC_H_

//
// File: olc.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define OLC_SET     0
#define OLC_SHOW    1
#define OLC_REPEAT  2
#define OLC_ROOM    3
#define OLC_MOB     4
#define OLC_OBJ     5

#define OLC_COPY    0
#define OLC_NAME    1
#define OLC_DESC    2
#define OLC_ALIASES 3

#define MAX_ROOM_NAME   75
#define MAX_NPC_NAME    50
#define MAX_OBJ_NAME    50
#define MAX_ROOM_DESC   1024
#define MAX_NPC_DESC    512
#define MAX_OBJ_DESC    512

#define SCMD_OLC        666

#define OLC_OSET    0
#define NORMAL_OSET 1

#define NPC_D1(lev)    (lev + 1)
#define NPC_D2(lev)    (6 + (lev / 2))
#define NPC_MOD(lev)   (((lev*lev*lev*lev*lev) / 32768) + lev + 6)
void set_physical_attribs(struct creature *ch);
// recalculates the given mob prototype's statistics based on it's current level.
void recalculate_based_on_level(struct creature *mob_p);
void recalc_all_mobs(struct creature *ch, const char *argument);

bool CAN_EDIT_ZONE(struct creature *ch, struct zone_data *zone);
bool OLC_EDIT_OK(struct creature *ch, struct zone_data *zone, int bits);

#define UPDATE_OBJLIST_NAMES(obj_p, tmp_obj, _item)                         \
    for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next) {   \
        if (tmp_obj->shared->vnum == obj_p->shared->vnum &&     \
            !IS_OBJ_STAT2(tmp_obj, ITEM2_RENAMED)) {                    \
            (tmp_obj)_item = (obj_p)_item; }}

#define UPDATE_OBJLIST(obj_p, tmp_obj, _item)                         \
    for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next) {    \
        if (tmp_obj->shared->vnum == obj_p->shared->vnum) {        \
            (tmp_obj)_item = (obj_p)_item; }}

#define UPDATE_OBJLIST_FULL(obj_p, obj)                               \
    for (obj = object_list; obj; obj = obj->next) {                   \
        if (obj->shared->vnum == obj_p->shared->vnum) {           \
            if (!IS_OBJ_STAT2(obj, ITEM2_RENAMED)) {                      \
                obj->aliases = obj_p->aliases;                                    \
                obj->name = obj_p->name;          \
            }                                                             \
            obj->line_desc = obj_p->line_desc;                        \
            obj->action_desc = obj_p->action_desc;                        \
            obj->ex_description = obj_p->ex_description;                  \
            obj->action_desc = obj_p->action_desc;          \
        }                                                               \
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

void format_search_data(struct str_builder *sb,
                        struct creature *ch,
                        struct room_data *room,
                        struct special_search_data *cur_search);
void print_search_data_to_buf(struct creature *ch,
                              struct room_data *room,
                              struct special_search_data *cur_search,
                              char *buf,
                              size_t buf_size);

void show_olc_help(struct creature *ch, char *arg);
int mobile_experience(struct creature *mob, FILE *outfile);

#endif                          // __OLC_H__
