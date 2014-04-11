#ifndef _HANDLER_H_
#define _HANDLER_H_

/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: handler.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

struct creature;

/* handling the affected-structures */
void init_affect(struct affected_type *af)
    __attribute__ ((nonnull));
void apply_object_affects(struct creature *ch, struct obj_data *obj, bool add)
    __attribute__ ((nonnull));
void affect_total(struct creature *ch)
    __attribute__ ((nonnull));
void affect_modify(struct creature *ch, int16_t loc, int16_t mod, long bitv,
	int index, bool add)
    __attribute__ ((nonnull));
void affect_to_char(struct creature *ch, struct affected_type *af)
    __attribute__ ((nonnull));
bool affect_remove(struct creature *ch, struct affected_type *af)
    __attribute__ ((nonnull));
bool affect_from_char(struct creature *ch, int16_t type)
    __attribute__ ((nonnull));
struct affected_type *affected_by_spell(struct creature *ch, int16_t type)
    __attribute__ ((nonnull));
int count_affect(struct creature *ch, int16_t type)
    __attribute__ ((nonnull));
void affect_join(struct creature *ch, struct affected_type *af,
	bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
    __attribute__ ((nonnull));
void check_interface(struct creature *ch, struct obj_data *obj, int mode)
    __attribute__ ((nonnull));

void check_flying(struct creature *ch)
    __attribute__ ((nonnull));
bool can_travel_sector(struct creature *ch, int sect_type, bool active)
    __attribute__ ((nonnull));

/* utility */
char *money_desc(int amount, int mode);
struct obj_data *create_money(int amount, int mode);
int isname(const char *str, const char *namelist)
    __attribute__ ((nonnull));
int isname_exact(const char *str, const char *namelist)
    __attribute__ ((nonnull));
bool namelist_match(const char *sub_list, const char *super_list)
    __attribute__ ((nonnull));
char *fname(const char *namelist)
    __attribute__ ((nonnull));
int get_number(char **name)
    __attribute__ ((nonnull));

/* ******** objects *********** */

int equip_char(struct creature *ch, struct obj_data *obj, int pos, int mode)
    __attribute__ ((nonnull));
struct obj_data *unequip_char(struct creature *ch, int pos, int mode)
    __attribute__ ((nonnull));
struct obj_data *raw_unequip_char(struct creature *ch, int pos, int mode)
    __attribute__ ((nonnull));
int check_eq_align(struct creature *ch)
    __attribute__ ((nonnull));
bool same_obj(struct obj_data *obj1, struct obj_data *obj2)
    __attribute__ ((nonnull));

struct obj_data *get_obj_in_list(char *name, struct obj_data *list)
    __attribute__ ((nonnull (1)));
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name)
    __attribute__ ((nonnull));
struct obj_data *get_obj_num(int nr);

void obj_to_char(struct obj_data *object, struct creature *ch)
    __attribute__ ((nonnull));
void unsorted_obj_to_char(struct obj_data *object, struct creature *ch)
    __attribute__ ((nonnull));
void obj_from_char(struct obj_data *object)
    __attribute__ ((nonnull));
void obj_to_room(struct obj_data *object, struct room_data *room)
    __attribute__ ((nonnull));
void unsorted_obj_to_room(struct obj_data *object, struct room_data *room)
    __attribute__ ((nonnull));
void obj_from_room(struct obj_data *object)
    __attribute__ ((nonnull));
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
    __attribute__ ((nonnull));
void unsorted_obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
    __attribute__ ((nonnull));
void obj_from_obj(struct obj_data *obj)
    __attribute__ ((nonnull));

void extract_obj(struct obj_data *obj)
    __attribute__ ((nonnull));

/* ******* characters ********* */

/*@keep@*/ struct creature *get_char_room(char *name, struct room_data *room)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char(char *name)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char_in_world_by_idnum(long nr);

bool char_from_room(struct creature *ch, bool check_specials)
    __attribute__ ((nonnull));
bool char_to_room(struct creature *ch, struct room_data *room, bool check_specials)
    __attribute__ ((nonnull));

/* find if character can see */
/*@keep@*/ struct creature *get_char_room_vis(struct creature *ch, const char *name)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char_random(struct room_data *room)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char_random_vis(struct creature *ch, struct room_data *room)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_player_random(struct room_data *room)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_player_random_vis(struct creature *ch, struct room_data *room)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char_in_remote_room_vis(struct creature *ch, const char *name,
	struct room_data *inroom)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_player_vis(struct creature *ch, const char *name, int inroom)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_mobile_vis(struct creature *ch, const char *name, int inroom)
    __attribute__ ((nonnull));
/*@keep@*/ struct creature *get_char_vis(struct creature *ch, const char *name)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_obj_in_list_vis(struct creature *ch, const char *name,
	struct obj_data *list)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_obj_in_list_all(struct creature *ch, const char *name,
	struct obj_data *list)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_obj_vis(struct creature *ch, const char *name)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_object_in_equip_vis(struct creature *ch,
	const char *arg, struct obj_data *equipment[], int *j)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_object_in_equip_pos(struct creature *ch, const char *arg,
	int pos)
    __attribute__ ((nonnull));
/*@keep@*/ struct obj_data *get_object_in_equip_all(struct creature *ch, const char *arg,
	struct obj_data *equipment[], int *j)
    __attribute__ ((nonnull));

int weapon_prof(struct creature *ch, struct obj_data *obj)
    __attribute__ ((nonnull));

/* Generic Find */

int generic_find(char *arg, int bitvector, struct creature *ch,
	struct creature **tar_ch, struct obj_data **tar_obj)
    __attribute__ ((nonnull (1,3)));

#define FIND_CHAR_ROOM      (1 << 0)
#define FIND_CHAR_WORLD     (1 << 1)
#define FIND_OBJ_INV        (1 << 2)
#define FIND_OBJ_ROOM       (1 << 3)
#define FIND_OBJ_WORLD      (1 << 4)
#define FIND_OBJ_EQUIP      (1 << 5)
#define FIND_OBJ_EQUIP_CONT (1 << 6)
#define FIND_IGNORE_WIERD   (1 << 7)

#define EQUIP_WORN    0
#define EQUIP_IMPLANT 1
#define EQUIP_TATTOO  2

#define SHOW_OBJ_ROOM    0
#define SHOW_OBJ_INV     1
#define SHOW_OBJ_CONTENT 2
#define SHOW_OBJ_NOBITS  3
#define SHOW_OBJ_UNUSED  4
#define SHOW_OBJ_EXTRA   5
#define SHOW_OBJ_BITS    6

enum decision_t {
	UNDECIDED,
	ALLOW,
	DENY,
};

struct reaction {
    char *reaction;
};

struct reaction *make_reaction(void);
void free_reaction(struct reaction *reaction);
bool add_reaction(struct reaction *reaction, char *arg)
    __attribute__ ((nonnull));
enum decision_t react(struct reaction *reaction, struct creature *ch)
    __attribute__ ((nonnull));

#endif
