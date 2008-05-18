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

/* handling the affected-structures */
bool obj_gives_affects(obj_data *obj, Creature *ch, int internal);
void affect_total(struct Creature *ch);
void affect_modify(struct Creature *ch, sh_int loc, sh_int mod, long bitv,
	int index, bool add);
void affect_to_char(struct Creature *ch, struct affected_type *af);
int affect_remove(struct Creature *ch, struct affected_type *af);
int affect_from_char(struct Creature *ch, sh_int type);
struct affected_type *affected_by_spell(struct Creature *ch, sh_int type);
int count_affect(struct Creature *ch, sh_int type);
void affect_join(struct Creature *ch, struct affected_type *af,
	bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void check_interface(struct Creature *ch, struct obj_data *obj, int mode);

void check_flying(struct Creature *ch);
bool can_travel_sector(struct Creature *ch, int sect_type, bool active);


/* utility */
char *money_desc(int amount, int mode);
struct obj_data *create_money(int amount, int mode);
int isname(const char *str, const char *namelist);
int isname_exact(const char *str, const char *namelist);
bool namelist_match(const char *sub_list, const char *super_list);
char *fname(const char *namelist);
int get_number(char **name);

/* ******** objects *********** */

int equip_char(struct Creature *ch, struct obj_data *obj, int pos, int mode);
struct obj_data *unequip_char(struct Creature *ch, int pos, int mode, bool disable_checks = false);
int check_eq_align(struct Creature *ch);
bool same_obj(obj_data *obj1, obj_data *obj2);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(int nr);

void obj_to_char(struct obj_data *object, struct Creature *ch, bool sorted = true);
void obj_from_char(struct obj_data *object);
void obj_to_room(struct obj_data *object, struct room_data *room, bool sorted = true);
void obj_from_room(struct obj_data *object);
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to, bool sorted = true);
void obj_from_obj(struct obj_data *obj);

void extract_obj(struct obj_data *obj);

/* ******* characters ********* */

struct Creature *get_char_room(char *name, struct room_data *room);
struct Creature *get_char(char *name);
struct Creature *get_char_in_world_by_idnum(int nr);

bool char_from_room( Creature *ch, bool check_specials = true );
bool char_to_room( Creature *ch, room_data *room, bool check_specials = true );

/* find if character can see */
struct Creature *get_char_room_vis(struct Creature *ch, char *name);
struct Creature *get_char_random(room_data *room);
struct Creature *get_char_random_vis(struct Creature *ch, room_data *room);
struct Creature *get_player_random(room_data *room);
struct Creature *get_player_random_vis(struct Creature *ch, room_data *room);
struct Creature *get_char_in_remote_room_vis(struct Creature *ch, char *name,
	struct room_data *inroom);
struct Creature *get_player_vis(struct Creature *ch, char *name, int inroom);
struct Creature *get_char_vis(struct Creature *ch, char *name);
struct obj_data *get_obj_in_list_vis(struct Creature *ch, char *name,
	struct obj_data *list);
struct obj_data *get_obj_in_list_all(struct Creature *ch, char *name,
	struct obj_data *list);
struct obj_data *get_obj_vis(struct Creature *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct Creature *ch,
	char *arg, struct obj_data *equipment[], int *j);
struct obj_data *get_object_in_equip_pos(struct Creature *ch, char *arg,
	int pos);
struct obj_data *get_object_in_equip_all(struct Creature *ch, char *arg,
	struct obj_data *equipment[], int *j);

int weapon_prof(struct Creature *ch, struct obj_data *obj);

/* find all dots */

int find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int generic_find(char *arg, int bitvector, struct Creature *ch,
	struct Creature **tar_ch, struct obj_data **tar_obj);

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

/* prototypes from crash save system */

#endif
