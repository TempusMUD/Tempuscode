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
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data *ch, sh_int loc, sh_int mod, long bitv, int index, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
int	affect_from_char(struct char_data *ch, sh_int type);
struct affected_type *affected_by_spell(struct char_data *ch, sh_int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
        bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void check_interface(struct char_data *ch, struct obj_data *obj, int mode);

void    check_flying(struct char_data *ch);
bool     can_travel_sector(struct char_data *ch, int sect_type, bool active);


/* utility */
char *money_desc(int amount, int mode);
struct obj_data *create_money(int amount, int mode);
int	isname(const char *str, const char *namelist);
int	isname_exact(const char *str, const char *namelist);
char	*fname(const char *namelist);
int	get_number(char **name);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch);
void	obj_from_char(struct obj_data *object);

int  equip_char(struct char_data *ch,struct obj_data *obj,int pos,int mode);
struct obj_data *unequip_char(struct char_data *ch, int pos, int mode);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(int nr);

void	obj_to_room(struct obj_data *object, struct room_data *room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);

/* ******* characters ********* */

struct char_data *get_char_room(char *name, struct room_data *room);
struct char_data *get_char_num(int nr);
struct char_data *get_char(char *name);
struct char_data *get_char_in_world_by_idnum(int nr);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, struct room_data *room);
void	extract_char(struct char_data *ch, byte mode);

/* find if character can see */
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
struct char_data *get_char_in_remote_room_vis(struct char_data *ch, char *name, struct room_data *inroom);
struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom);
struct char_data *get_char_vis(struct char_data *ch, char *name);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, 
struct obj_data *list);
struct obj_data *get_obj_in_list_all(struct char_data *ch, char *name, 
struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
                             char *arg, struct obj_data *equipment[], int *j);
struct obj_data *get_object_in_equip_pos(struct char_data *ch, char *arg, int pos);
struct obj_data *get_object_in_equip_all(struct char_data *ch,
                            char *arg, struct obj_data *equipment[], int *j);

int weapon_prof(struct char_data *ch, struct obj_data *obj);

/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(char *arg, int bitvector, struct char_data *ch,
struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM      (1 << 0)
#define FIND_CHAR_WORLD     (1 << 1)
#define FIND_OBJ_INV        (1 << 2)
#define FIND_OBJ_ROOM       (1 << 3)
#define FIND_OBJ_WORLD      (1 << 4)
#define FIND_OBJ_EQUIP      (1 << 5)
#define FIND_OBJ_EQUIP_CONT (1 << 6)
#define FIND_IGNORE_WIERD   (1 << 7)


#define MODE_EQ      0
#define MODE_IMPLANT 1


/* prototypes from crash save system */

int	Crash_get_filename(char *orig_name, char *filename);
int	Crash_delete_file(char *name, int mode);
int	Crash_delete_crashfile(struct char_data *ch);
int	Crash_clean_file(char *name);
void	Crash_listrent(struct char_data *ch, char *name);
int	Crash_load(struct char_data *ch);
void	Crash_crashsave(struct char_data *ch);
void	Crash_idlesave(struct char_data *ch);
void	Crash_save_all(void);

/* prototypes from fight.c */
void	set_fighting(struct char_data *ch, struct char_data *victim, int aggr);
void	stop_fighting(struct char_data *ch);
void	stop_follower(struct char_data *ch);
int	hit(struct char_data *ch, struct char_data *victim, int type);
void	forget(struct char_data *ch, struct char_data *victim);
void	remember(struct char_data *ch, struct char_data *victim);
int char_in_memory(struct char_data *victim, struct char_data *rememberer);
int	damage(struct char_data *ch, struct char_data *victim, int dam, 
	       int attacktype, int location);
int	skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype);

#define TED_MESSAGE " Write the text.  Terminate with @ on a new line.\r\n"\
      " Enter a * on a new line to enter TED\r\n"  \
	" [+--------+---------+---------+--------"  \
	"-+---------+---------+---------+------+]\r\n"
