/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: db.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//


#ifndef __db_h__
#define __db_h__

#include <stdio.h>
#include "defs.h"
#include "char_data.h"

#ifndef _NEWDYNCONTROL_			// used by a util

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	 0
#define DB_BOOT_MOB	 1
#define DB_BOOT_OBJ	 2
#define DB_BOOT_ZON	 3
#define DB_BOOT_SHP	 4
#define DB_BOOT_TICL 5
#define DB_BOOT_ISCR 6

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files        */
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode   */
#define TINDEX_FILE	"index.test"	/* ... and for test-mud-mode   */
#define WLD_PREFIX	"world/wld"	/* room definitions        */
#define MOB_PREFIX	"world/mob"	/* monster prototypes      */
#define OBJ_PREFIX	"world/obj"	/* object prototypes       */
#define ZON_PREFIX	"world/zon"	/* zon defs & command tables   */
#define SHP_PREFIX	"world/shp"	/* shop definitions        */
#define TICL_PREFIX "world/ticl"	/* ticl procs                   */
#define ISCR_PREFIX "world/iscr"	/* iscripts */
#define XML_PREFIX  "world/xml"

#define CREDITS_FILE	"text/credits"	/* for the 'credits' command    */
#define MOTD_FILE	"text/motd"	/* messages of the day / mortal */
#define ANSI_MOTD_FILE	"text/ansi_motd"	/* messages of the day / mortal */
#define IMOTD_FILE	"text/imotd"	/* messages of the day / immort */
#define ANSI_IMOTD_FILE	"text/ansi_imotd"	/* messages of the day / imm */
#define HELP_KWRD_FILE	"text/help_table"	/* for HELP <keywrd>       */
#define HELP_KWRD_WIZ	"text/help_table_wiz"	/* for HELP <keywrd>   */
#define HELP_PAGE_FILE	"text/help"	/* for HELP <CR>        */
#define INFO_FILE	"text/info"	/* for INFO         */
#define WIZLIST_FILE	"text/wizlist"	/* for WIZLIST          */
#define ANSI_WIZLIST_FILE  "text/ansi_wizlist"	/* for WIZLIST  */
#define IMMLIST_FILE	"text/immlist"	/* for IMMLIST          */
#define ANSI_IMMLIST_FILE  "text/ansi_immlist"	/* for IMMLIST  */
#define BACKGROUND_FILE	"text/background"	/* for the background story   */
#define POLICIES_FILE	"text/policies"	/* player policies/rules  */
#define HANDBOOK_FILE	"text/handbook"	/* handbook for new immorts   */
#define AREAS_LOW_FILE	"text/areas_low"	/* list of areas  */
#define AREAS_MID_FILE	"text/areas_mid"
#define AREAS_HIGH_FILE	"text/areas_high"
#define AREAS_REMORT_FILE  "text/areas_remort"
#define OLC_GUIDE_FILE  "text/olc_creation_guide"	/* tips for creators  */
#define QUEST_GUIDE_FILE "text/quest_guide"	/* quest guidelines         */
#define QUEST_LIST_FILE  "text/quest_list"	/* list of quests           */

#define IDEA_FILE	"misc/ideas"	/* for the 'idea'-command   */
#define TYPO_FILE	"misc/typos"	/*         'typo'       */
#define BUG_FILE	"misc/bugs"	/*         'bug'        */
#define MESS_FILE	"misc/messages"	/* damage messages      */
#define SOCMESS_FILE	"misc/socials"	/* messgs for social acts   */
#define XNAME_FILE	"misc/xnames"	/* invalid name substrings  */
#define NASTY_FILE	"misc/nasty"	/* nasty words for public comm  */

#define PLAYER_FILE	"etc/players"	/* the player database      */
#define MAIL_FILE	"etc/plrmail"	/* for the mudmail system   */
#define BAN_FILE	"etc/badsites"	/* for the siteban system   */
#define HCONTROL_FILE	"etc/hcontrol"	/* for the house system     */
#define ALIAS_DIR       "plralias/"

#define CMD_LOG_FILE    "cmd_log"

/* public procedures in db.c */
void boot_db(void);
int create_entry(char *name);
void zone_update(void);
struct room_data *real_room(int vnum);
struct zone_data *real_zone(int number);
class CIScript *real_iscript(int vnum);
char *fread_string(FILE * fl, char *error);
int pread_string(FILE * fl, char *str, char *error);
long get_id_by_name(const char *name);
char *get_name_by_id(long id);

void char_to_store(struct char_data *ch, struct char_file_u *st);
void store_to_char(struct char_file_u *st, struct char_data *ch);
int load_char(char *name, struct char_file_u *char_element);
void save_char(struct char_data *ch, struct room_data *load_room);
void init_char(struct char_data *ch);
struct char_data *read_mobile(int vnum);
int real_mobile(int vnum);
struct char_data *real_mobile_proto(int vnum);
int vnum_mobile(char *searchname, struct char_data *ch);
void clear_char(struct char_data *ch);
void reset_char(struct char_data *ch);
void free_char(struct char_data *ch);

struct obj_data *create_obj(void);
void clear_object(struct obj_data *obj);
void free_obj(struct obj_data *obj);
int real_object(int vnum);
struct obj_data *real_object_proto(int vnum);
struct obj_data *read_object(int vnum);
int vnum_object(char *searchname, struct char_data *ch);
int zone_number(int nr);
struct room_data *where_obj(struct obj_data *obj);
struct char_data *obj_owner(struct obj_data *obj);

#define REAL 0
#define VIRTUAL 1

#define ZONE_IDLE_TIME 5

/* structure for the reset commands */
struct reset_com {
	char command;				/* current command                      */

	int if_flag;				/* if TRUE: exe only if preceding exe'd */
	int arg1;					/*                                      */
	int arg2;					/* Arguments to the command             */
	int arg3;					/*                                      */
	int line;					/* line number this command appears on  */
	int prob;
	struct reset_com *next;

	/* 
	 *  Commands:              *
	 *  'M': Read a mobile     *
	 *  'O': Read an object    *
	 *  'G': Give obj to mob   *
	 *  'P': Put obj in obj    *
	 *  'G': Obj to char       *
	 *  'E': Obj to char equip *
	 *  'D': Set state of door *
	 *  'V': Set Path on obj   *
	 */
};

struct player_index_element {
	char *name;
	long id;
};

struct help_index_element {
	char *keyword;
	long pos;
};

/* global buffering system */

#ifdef __db_c__
char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];
int population_record[NUM_HOMETOWNS];
#else
extern char buf[MAX_STRING_LENGTH];
extern char buf1[MAX_STRING_LENGTH];
extern char buf2[MAX_STRING_LENGTH];
extern char arg[MAX_STRING_LENGTH];
extern int population_record[NUM_HOMETOWNS];
#endif

#ifndef __CONFIG_C__
extern char *OK;
extern char *NOPERSON;
extern char *NOEFFECT;
#endif

#endif							// _NEWDYNCONTROL_

#define DYN_TEXT_HIST_SIZE 10
#define DYN_TEXT_PERM_SIZE 5
#define DYN_TEXT_BACKUP_DIR "text/dyn/text_backup"
#define DYN_TEXT_CONTROL_DIR "text/dyn/control"


typedef struct dynamic_edit_data {
	long idnum;
	time_t tEdit;
} dynamic_edit_data;

typedef struct dynamic_text_file {
	char filename[1024];		// name of the file to load/save
	dynamic_edit_data last_edit[DYN_TEXT_HIST_SIZE];	// history of edits
	long perms[DYN_TEXT_PERM_SIZE];	// who has special access perms
	int level;					// edit access level
	long lock;					// lock while editing (char's idnum)
	char *buffer;				// the buffer that people see
	char *tmp_buffer;			// the buffer you edit
	struct dynamic_text_file *next;
} dynamic_text_file;

#ifndef _NEWDYNCONTROL_

#ifndef __DYNTEXT_C__
extern dynamic_text_file *dyntext_list;
#endif

void check_dyntext_updates(CHAR * ch, int mode);
#define CHECKDYN_UNRENT    0
#define CHECKDYN_RECONNECT 1

#endif							// _NEWDYNCONTROL_

#ifndef __db_c__

extern struct time_info_data time_info;
extern const int daylight_mod[];
extern const char *lunar_phases[];
extern int lunar_stage;
extern int lunar_phase;
extern struct obj_shared_data *null_obj_shared;
extern struct shop_data *shop_index;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;

#endif							// __db_c__

#endif							// #ifndef __db_h__
