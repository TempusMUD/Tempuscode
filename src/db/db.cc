/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: db.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __db_c__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <fstream>
using namespace std;

#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "screen.h"
#include "flow_room.h"
#include "clan.h"
#include "paths.h"
#include "quest.h"
#include "char_class.h"
#include "security.h"
#include "olc.h"
#include "shop.h"
#include "help.h"
#include "combat.h"
#include "iscript.h"
#include "tmpstr.h"
#include "flags.h"
#include "player_table.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

int top_of_world = 0;			/* ref to top element of world         */

int top_of_mobt = 0;			/* top of mobile index table         */
int current_mob_idnum = 0;

struct obj_data *object_list = NULL;	/* global linked list of objs         */
struct obj_data *obj_proto;		/* prototypes for objs                 */
struct obj_shared_data *null_obj_shared;
struct mob_shared_data *null_mob_shared;
int top_of_objt = 0;			/* top of object index table         */

struct zone_data *zone_table;	/* zone table                         */
int top_of_zone_table = 0;		/* top element of zone tab         */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages  */
struct player_index_element *player_table = NULL;	/* index to plr file */
extern HelpCollection *Help;
extern list <CIScript *>scriptList;

FILE *player_fl = NULL;			/* file desc of player file         */

int top_of_p_table = 0;			/* ref to top of table                 */
int top_of_p_file = 0;			/* ref of size of p file         */
long top_idnum = 0;				/* highest idnum in use                 */
int no_plrtext = 0;				/* player text disabled?         */

int no_mail = 0;				/* mail disabled?                 */
int mini_mud = 0;				/* mini-mud mode?                 */
int mud_moved = 0;
int no_rent_check = 0;			/* skip rent check on boot?         */
time_t boot_time = 0;			/* time of mud boot                 */
int restrict = 0;				/* level of game restriction         */
int olc_lock = 0;
int no_initial_zreset = 0;
int quest_status = 0;
int lunar_stage = 0;
int lunar_phase = MOON_NEW;

struct room_data *r_mortal_start_room;	/* rnum of mortal start room   */
struct room_data *r_electro_start_room;	/* Electro Centralis start room  */
struct room_data *r_immort_start_room;	/* rnum of immort start room   */
struct room_data *r_frozen_start_room;	/* rnum of frozen start room   */
struct room_data *r_new_thalos_start_room;
struct room_data *r_elven_start_room;
struct room_data *r_istan_start_room;
struct room_data *r_arena_start_room;
struct room_data *r_tower_modrian_start_room;
struct room_data *r_monk_start_room;
struct room_data *r_solace_start_room;
struct room_data *r_mavernal_start_room;
struct room_data *r_dwarven_caverns_start_room;
struct room_data *r_human_square_start_room;
struct room_data *r_skullport_start_room;
struct room_data *r_skullport_newbie_start_room;
struct room_data *r_drow_isle_start_room;
struct room_data *r_astral_manse_start_room;
struct room_data *r_zul_dane_start_room;
struct room_data *r_zul_dane_newbie_start_room;
struct room_data *r_newbie_school_start_room;


struct zone_data *default_quad_zone = NULL;

int *obj_index = NULL;			/* object index                  */
int *mob_index = NULL;			/* mobile index                  */
int *shp_index = NULL;			/* shop index                    */
int *wld_index = NULL;			/* world index                   */
int *ticl_index = NULL;			/* ticl index                    */
int *iscr_index = NULL;			/* iscript index                 */

char *credits = NULL;			/* game credits                         */
char *motd = NULL;				/* message of the day - mortals */
char *ansi_motd = NULL;			/* message of the day - mortals */
char *imotd = NULL;				/* message of the day - immorts */
char *ansi_imotd = NULL;		/* message of the day - immorts */
//char *help = NULL;                /* help screen                         */
char *info = NULL;				/* info page                         */
char *background = NULL;		/* background story                 */
char *handbook = NULL;			/* handbook for new immortals         */
char *policies = NULL;			/* policies page                 */
char *areas_low = NULL;			/* areas information                 */
char *areas_mid = NULL;			/* areas information                 */
char *areas_high = NULL;		/* areas information                 */
char *areas_remort = NULL;		/* areas information                 */
char *bugs = NULL;				/* bugs file                         */
char *ideas = NULL;				/* ideas file                         */
char *typos = NULL;				/* typos file                         */
char *olc_guide = NULL;			/* creation tips                 */
char *quest_guide = NULL;		/* quest guidelines             */

//FILE *help_fl = NULL;                /* file for help text                 */
//struct help_index_element *help_index = 0;        /* the help table         */
//int top_of_helpt;                /* top of help index table         */

struct time_info_data time_info;	/* the infomation about the time    */
/*struct weather_data weather_info;        the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs         */
struct reset_q_type reset_q;	/* queue of zones to be reset         */

/* local functions */
void setup_dir(FILE * fl, struct room_data *room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_room(FILE * fl, int vnum_nr);
void parse_mobile(FILE * mob_f, int nr);
void load_ticl(FILE * ticl_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void Load_paths(void);
int load_elevators();
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void assign_artisans(void);
void build_player_index(void);
void build_player_table(void);
void boot_dynamic_text(void);
void load_iscripts(char fname[MAX_INPUT_LENGTH]);

/*int is_empty(struct zone_data *zone); */
void reset_zone(struct zone_data *zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(struct zone_data *zone, struct reset_com *zonecmd,
	char *message);
void reset_time(void);
void reset_zone_weather(void);
void set_local_time(struct zone_data *zone, struct time_info_data *local_time);
void update_alias_dirs(void);
void purge_trails(struct Creature *ch);

/* external functions */
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);		/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void sort_skills(void);
void load_banned(void);
void Read_Invalid_List(void);
void Read_Nasty_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
//struct help_index_element *build_help_index(FILE * fl, int *num);
void add_alias(struct Creature *ch, struct alias_data *a);
void boot_clans(void);
void add_follower(struct Creature *ch, struct Creature *leader);
void xml_reload( Creature *ch );
extern int no_specials;
extern int scheck;

/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */



ACMD(do_reboot)
{
	one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(ANSI_MOTD_FILE, &ansi_motd);
		file_to_string_alloc(IMOTD_FILE, &imotd);
		file_to_string_alloc(ANSI_IMOTD_FILE, &ansi_imotd);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
		file_to_string_alloc(AREAS_LOW_FILE, &areas_low);
		file_to_string_alloc(AREAS_MID_FILE, &areas_mid);
		file_to_string_alloc(AREAS_HIGH_FILE, &areas_high);
		file_to_string_alloc(AREAS_REMORT_FILE, &areas_remort);
		file_to_string_alloc(OLC_GUIDE_FILE, &olc_guide);
		file_to_string_alloc(QUEST_GUIDE_FILE, &quest_guide);
	} else if (!str_cmp(arg, "credits")) {
		file_to_string_alloc(CREDITS_FILE, &credits);
	} else if (!str_cmp(arg, "motd")) {
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(ANSI_MOTD_FILE, &ansi_motd);
	} else if (!str_cmp(arg, "imotd")) {
		file_to_string_alloc(IMOTD_FILE, &imotd);
		file_to_string_alloc(ANSI_IMOTD_FILE, &ansi_imotd);
	} else if (!str_cmp(arg, "info")){
		file_to_string_alloc(INFO_FILE, &info);
	} else if (!str_cmp(arg, "handbook")) {
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	} else if (!str_cmp(arg, "background")){
		file_to_string_alloc(BACKGROUND_FILE, &background);
	} else if (!str_cmp(arg, "areas")) {
		file_to_string_alloc(AREAS_LOW_FILE, &areas_low);
		file_to_string_alloc(AREAS_MID_FILE, &areas_mid);
		file_to_string_alloc(AREAS_HIGH_FILE, &areas_high);
		file_to_string_alloc(AREAS_REMORT_FILE, &areas_remort);
		file_to_string_alloc(TYPO_FILE, &typos);
	} else if (!str_cmp(arg, "olc_guide")) {
		file_to_string_alloc(OLC_GUIDE_FILE, &olc_guide);
	} else if (!str_cmp(arg, "quest_guide")) {
		file_to_string_alloc(QUEST_GUIDE_FILE, &quest_guide);
	} else if (!str_cmp(arg, "paths")) {
		Load_paths();
	} else if (!str_cmp(arg, "trails")) {
		purge_trails(ch);
	} else if (!str_cmp(arg, "timewarps")) {
		boot_timewarp_data();
    } else if( !str_cmp(arg, "xml") ) {
        xml_reload(ch);
        return;
	} else if (!str_cmp(arg, "elevators")) {
		if (!load_elevators())
			send_to_char(ch, "There was an error.\r\n");
	} else {
		send_to_char(ch, "Unknown reboot option.\r\n");
        send_to_char(ch, "Options: all    *         credits     motd     imotd      info\r\n");
        send_to_char(ch, "         areas  olc_guide quest_guide handbook background paths\r\n");
        send_to_char(ch, "         trails timewarps elevators   xml\r\n");
		return;
	}
	send_to_char(ch, OK);
	mudlog(GET_INVIS_LVL(ch), NRM, false,
		"%s has reloaded %s text file.", GET_NAME(ch), arg);
}


void
boot_world(void)
{
	slog("Loading zone table.");
	index_boot(DB_BOOT_ZON);

	slog("Loading rooms.");
	index_boot(DB_BOOT_WLD);

	slog("Loading XML data.");
	xml_boot();

	slog("Renumbering rooms.");
	renum_world();

	slog("Checking start rooms.");
	check_start_rooms();

	slog("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);

	slog("Loading iscripts and generating index.");
	index_boot(DB_BOOT_ISCR);

	slog("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	slog("Loading TICL procs and generating index.");
	index_boot(DB_BOOT_TICL);

	slog("Renumbering zone table.");
	renum_zone_table();

	if (!no_specials) {
		slog("Loading shops.");
		index_boot(DB_BOOT_SHP);
	}

	/* for quad damage bamfing */
	if (!(default_quad_zone = real_zone(25)))
		default_quad_zone = zone_table;
}


/* body of the booting system */
void
boot_db(void)
{
	struct zone_data *zone;

	slog("Boot db -- BEGIN.");

	//boot_flagsets();

	slog("Resetting the game time:");
	reset_time();

	if( mini_mud ) {
		slog("Building player table.");
		build_player_table();
		slog("...%d records added.", playerIndex.size());
	}

	slog("Reading credits, bground, info & motds.");
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(ANSI_MOTD_FILE, &ansi_motd);
	file_to_string_alloc(IMOTD_FILE, &imotd);
	file_to_string_alloc(ANSI_IMOTD_FILE, &ansi_imotd);
//    file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);
	file_to_string_alloc(AREAS_LOW_FILE, &areas_low);
	file_to_string_alloc(AREAS_MID_FILE, &areas_mid);
	file_to_string_alloc(AREAS_HIGH_FILE, &areas_high);
	file_to_string_alloc(AREAS_REMORT_FILE, &areas_remort);
	file_to_string_alloc(OLC_GUIDE_FILE, &olc_guide);
	file_to_string_alloc(QUEST_GUIDE_FILE, &quest_guide);

	boot_dynamic_text();
	boot_world();

	reset_zone_weather();

	slog("Generating player index.");
	build_player_index();

	slog("Booting clans.");
	boot_clans();

	slog("Booting quests.");
	boot_quests();

	slog("Loading elevator data.");
	load_elevators();

	slog("Loading fight messages.");
	load_messages();

	slog("Loading social messages.");
	boot_social_messages();

	slog("Assigning function pointers:");

	if (!no_specials) {
		slog("   Mobiles.");
		assign_mobiles();
		slog("   Shopkeepers.");
		assign_the_shopkeepers();
		slog("   Objects.");
		assign_objects();
		slog("   Rooms.");
		assign_rooms();
		slog("   Artisans.");
		assign_artisans();
	}
	slog("   Spells.");
	mag_assign_spells();
	while (spells[max_spell_num][0] != '\n')
		max_spell_num++;


	slog("Sorting command list and spells.");
	sort_commands();
	sort_spells();
	sort_skills();
	Security::loadGroups();

	slog("Reading banned site, invalid-name, and NASTY word lists.");
	load_banned();
	Read_Invalid_List();
	Read_Nasty_List();

	slog("Reading paths.");
	Load_paths();

	slog("Booting timewarp data.");
	boot_timewarp_data();

	if (!no_rent_check) {
		slog("Deleting timed-out crash and rent files:");
		update_obj_file();
		slog("Done.");
		slog("Deleting unwanted alias files:");
		update_alias_dirs();
		slog("Done.");
	}

	if (!no_initial_zreset) {
		for (zone = zone_table; zone; zone = zone->next) {
			sprintf(buf2, "Resetting %s (rms %d-%d).",
				zone->name, zone->number * 100, zone->top);
			slog(buf2);
			reset_zone(zone);
		}
	}

	slog("Booting help system.");
	Help = new HelpCollection;
	if (Help->LoadIndex())
		slog("Help system boot succeded.");
	else
		slog("SYSERR: Help System Boot FAILED.");

	reset_q.head = reset_q.tail = NULL;

	if (TRUE) {					/*   this needs to be changed to !mini_mud   */
		slog("Booting houses.");
		House_boot();

		House_countobjs();

	}

	boot_time = time(0);

	slog("Boot db -- DONE.");
}


/* reset the time in the game from file */
void
reset_time(void)
{
	long beginning_of_time = 650336715;
	struct time_info_data mud_time_passed(time_t t2, time_t t1);

	time_info = mud_time_passed(time(0), beginning_of_time);

	slog("   Current Gametime (global): %dH %dD %dM %dY.",
		time_info.hours, time_info.day, time_info.month, time_info.year);
}

void
reset_zone_weather(void)
{
	struct zone_data *zone = NULL;
	struct time_info_data local_time;

	for (zone = zone_table; zone; zone = zone->next) {
		set_local_time(zone, &local_time);

		if (local_time.hours <= 4)
			zone->weather->sunlight = SUN_DARK;
		else if (local_time.hours == 5)
			zone->weather->sunlight = SUN_RISE;
		else if (local_time.hours <= 20)
			zone->weather->sunlight = SUN_LIGHT;
		else if (local_time.hours == 21)
			zone->weather->sunlight = SUN_SET;
		else
			zone->weather->sunlight = SUN_DARK;

		zone->weather->pressure = 960;

		if ((local_time.month >= 7) && (local_time.month <= 12))
			zone->weather->pressure += dice(1, 50);
		else
			zone->weather->pressure += dice(1, 80);

		zone->weather->change = 0;

		if (zone->weather->pressure <= 980)
			zone->weather->sky = SKY_LIGHTNING;
		else if (zone->weather->pressure <= 1000)
			zone->weather->sky = SKY_RAINING;
		else if (zone->weather->pressure <= 1020)
			zone->weather->sky = SKY_CLOUDY;
		else
			zone->weather->sky = SKY_CLOUDLESS;
	}

	slog("Zone weather set.");

}

/**
 * Parses name/id information out of every player file and stores it in
 * the playerIndex.
**/
void
build_player_table() 
{
	DIR* dir;
	dirent *file;
	char *dirname;
	char inbuf[131072];

	for( int i = 0; i <= 9; i++ ) {
		// If we don't have
		dirname = tmp_sprintf("players/%d", i);
		dir = opendir(dirname);
		if (!dir) {
			mkdir(dirname, 0644);
			dir = opendir(dirname);
			if (!dir) {
				slog("SYSERR: Couldn't open or create directory %s", dirname);
				exit(-1);
			}
		}
		while ((file = readdir(dir)) != NULL) {
			// Check for correct filename (*.dat)
			if (!rindex(file->d_name, '.'))
				continue;
			if (strcmp(rindex(file->d_name, '.'), ".dat"))
				continue;

			// Open up the file and scan through it, looking for name and idnum
			// parameters
			ifstream in(tmp_sprintf("%s/%s", dirname, file->d_name));
			in.read(inbuf, sizeof(inbuf));
			char *name = strstr(inbuf, "name=" );
			char *idnum = strstr(inbuf, "idnum=" );

			if(!name || !idnum)
				continue;

			name += 5;
			name = tmp_getquoted(&name);
			idnum += 6;
			idnum = tmp_getquoted(&idnum);

			long id = atol( idnum );
			playerIndex.add( id, name, false );
		}
		closedir(dir);
	}
	playerIndex.sort();
}

/* generate index table for the player file */
void
build_player_index(void)
{
	int nr = -1, i;
	long size, recs;
	struct char_file_u dummy;

	if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
		if (errno != ENOENT) {
			perror("fatal error opening playerfile");
			safe_exit(1);
		} else {
			slog("No playerfile.  Creating a new one.");
			touch(PLAYER_FILE);
			if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
				perror("fatal error opening new playerfile");
				safe_exit(1);
			}
		}
	}

	fseek(player_fl, 0L, SEEK_END);
	size = ftell(player_fl);
	rewind(player_fl);
	if (size % sizeof(struct char_file_u))
		fprintf(stderr, "\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!\n");
	recs = size / sizeof(struct char_file_u);
	if (recs) {
		slog("   %ld players in database.", recs);
		CREATE(player_table, struct player_index_element, recs);
	} else {
		player_table = NULL;
		top_of_p_file = top_of_p_table = -1;
		return;
	}

	for (i = 0; i < NUM_HOMETOWNS; i++)
		population_record[i] = 0;

	for (; !feof(player_fl);) {
		fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
		if (!feof(player_fl)) {	/* new record */
			nr++;
			CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
			for (i = 0;(*(player_table[nr].name + i) = tolower(*(dummy.name + i)));i++);
			player_table[nr].id = dummy.char_specials_saved.idnum;
			top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
			population_record[dummy.hometown]++;
		}
	}

	top_of_p_file = top_of_p_table = nr;
}



/* function to count how many hash-mark delimited records exist in a file */
int
count_hash_records(FILE * fl)
{
	char buf[128];
	int count = 0;

	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;

	return count;
}



void
index_boot(int mode)
{
	char *index_filename, *prefix = NULL;
	FILE *index, *db_file;
	int rec_count = 0, index_count = 0, number = 9, i;

	switch (mode) {
	case DB_BOOT_WLD:
		prefix = WLD_PREFIX;
		break;
	case DB_BOOT_MOB:
		prefix = MOB_PREFIX;
		break;
	case DB_BOOT_OBJ:
		prefix = OBJ_PREFIX;
		break;
	case DB_BOOT_ZON:
		prefix = ZON_PREFIX;
		break;
	case DB_BOOT_SHP:
		prefix = SHP_PREFIX;
		break;
	case DB_BOOT_TICL:
		prefix = TICL_PREFIX;
		break;
	case DB_BOOT_ISCR:
		prefix = ISCR_PREFIX;
		break;
	default:
		slog("SYSERR: Unknown subcommand to index_boot!");
		safe_exit(1);
		break;
	}

	if (mini_mud)
		index_filename = MINDEX_FILE;
	else
		index_filename = INDEX_FILE;

	sprintf(buf2, "%s/%s", prefix, index_filename);

	if (!(index = fopen(buf2, "r"))) {
		if (!(index = fopen(buf2, "a+"))) {
			sprintf(buf1, "Error opening index file '%s'", buf2);
			perror(buf1);
			safe_exit(1);
		}
	}

	/* first, count the number of records in the file so we can malloc */
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$') {
		sprintf(buf2, "%s/%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r"))) {
			//perror(buf2);
			//safe_exit(1);
			fprintf(stderr, "Unable to open: %s\r\n", buf2);
			index_count++;
			fscanf(index, "%s\n", buf1);
			continue;
		} else {
			if (mode == DB_BOOT_ZON)
				rec_count++;
			else
				rec_count += count_hash_records(db_file);
		}

		fclose(db_file);
		index_count++;
		fscanf(index, "%s\n", buf1);
	}

	if (!rec_count) {
		if (mode != DB_BOOT_ISCR) {
			slog("SYSERR: boot error - 0 records counted");
			safe_exit(1);
		} else
			slog("WARNING:  No IScripts loaded - 0 records counted");
	}
	rec_count++;

	switch (mode) {
	case DB_BOOT_WLD:
		break;
	case DB_BOOT_MOB:
		CREATE(null_mob_shared, struct mob_shared_data, 1);
		null_mob_shared->vnum = -1;
		null_mob_shared->number = 0;
		null_mob_shared->func = NULL;
		null_mob_shared->ticl_ptr = NULL;
		null_mob_shared->proto = NULL;
		null_mob_shared->move_buf = NULL;
		break;

	case DB_BOOT_OBJ:
		CREATE(null_obj_shared, struct obj_shared_data, 1);
		null_obj_shared->vnum = -1;
		null_obj_shared->number = 0;
		null_obj_shared->house_count = 0;
		null_obj_shared->func = NULL;
		null_obj_shared->ticl_ptr = NULL;
		null_obj_shared->proto = NULL;
		break;

	case DB_BOOT_ZON:
		break;

	case DB_BOOT_ISCR:
		break;
	}

	if (mode != DB_BOOT_ZON) {
		rewind(index);
		if (mode == DB_BOOT_OBJ)
			CREATE(obj_index, int, index_count + 1);
		else if (mode == DB_BOOT_MOB)
			CREATE(mob_index, int, index_count + 1);
		else if (mode == DB_BOOT_SHP)
			CREATE(shp_index, int, index_count + 1);
		else if (mode == DB_BOOT_WLD)
			CREATE(wld_index, int, index_count + 1);
		else if (mode == DB_BOOT_TICL)
			CREATE(ticl_index, int, index_count + 1);
		else if (mode == DB_BOOT_ISCR)
			CREATE(iscr_index, int, index_count + 1);

		for (i = 0; i < index_count; i++) {
			if (mode == DB_BOOT_OBJ) {
				fscanf(index, "%d.obj\n", &number);
				obj_index[i] = number;
			} else if (mode == DB_BOOT_MOB) {
				fscanf(index, "%d.mob\n", &number);
				mob_index[i] = number;
			} else if (mode == DB_BOOT_SHP) {
				fscanf(index, "%d.shp\n", &number);
				shp_index[i] = number;
			} else if (mode == DB_BOOT_WLD) {
				fscanf(index, "%d.wld\n", &number);
				wld_index[i] = number;
			} else if (mode == DB_BOOT_TICL) {
				fscanf(index, "%d.ticl\n", &number);
				ticl_index[i] = number;
			} else if (mode == DB_BOOT_ISCR) {
				fscanf(index, "%d.iscr\n", &number);
				iscr_index[i] = number;
			}
		}

		if (mode == DB_BOOT_OBJ)
			obj_index[index_count] = -1;
		else if (mode == DB_BOOT_MOB)
			mob_index[index_count] = -1;
		else if (mode == DB_BOOT_SHP)
			shp_index[index_count] = -1;
		else if (mode == DB_BOOT_WLD)
			wld_index[index_count] = -1;
		else if (mode == DB_BOOT_TICL)
			ticl_index[index_count] = -1;
		else if (mode == DB_BOOT_ISCR)
			iscr_index[index_count] = -1;
	}

	rewind(index);

	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$') {
		sprintf(buf2, "%s/%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r"))) {
			perror(buf2);
			safe_exit(1);
		}
		switch (mode) {
		case DB_BOOT_WLD:
		case DB_BOOT_OBJ:
		case DB_BOOT_MOB:
		case DB_BOOT_TICL:
			discrete_load(db_file, mode);
			break;
		case DB_BOOT_ZON:
			load_zones(db_file, buf2);
			break;
		case DB_BOOT_SHP:
			boot_the_shops(db_file, buf2, rec_count);
			break;
		case DB_BOOT_ISCR:
			load_iscripts(buf2);
			break;
		}

		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
}

void
load_iscripts(char fname[MAX_INPUT_LENGTH])
{
	string line;

	ifstream ifile(buf2);

	while (line != "$") {
		getline(ifile, line);
		if (line.substr(0, 1) == "#") {
			CIScript *s = new CIScript(ifile, line);
			scriptList.push_back(s);
		}
	}
}

void
discrete_load(FILE * fl, int mode)
{
	int nr = -1, last = 0;
	char line[256];

	char *modes[] = { "world", "mob", "obj", "zon", "shp", "ticl" };

	for (;;) {
		/*
		 * we have to do special processing with the obj files because they have
		 * no end-of-record marker :(
		 */
		if (mode != DB_BOOT_OBJ || nr < 0)
			if (!get_line(fl, line)) {
				fprintf(stderr, "Format error after %s #%d\n", modes[mode],
					nr);
				safe_exit(1);
			}
		if (*line == '$')
			return;

		if (*line == '#') {
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				fprintf(stderr, "Format error after %s #%d\n", modes[mode],
					last);
				safe_exit(1);
			}
			if (nr >= 99999)
				return;
			else
				switch (mode) {
				case DB_BOOT_WLD:
					parse_room(fl, nr);
					break;
				case DB_BOOT_MOB:
					parse_mobile(fl, nr);
					break;
				case DB_BOOT_OBJ:
					strcpy(line, parse_object(fl, nr));
					break;
				case DB_BOOT_TICL:
					load_ticl(fl, nr);
				}
		} else {
			fprintf(stderr, "Format error in %s file near %s #%d\n",
				modes[mode], modes[mode], nr);
			fprintf(stderr, "Offending line: '%s'\n", line);
			safe_exit(1);
		}
	}
}


long
asciiflag_conv(char *flag)
{
	long flags = 0;
	int is_number = 1;
	register char *p;

	for (p = flag; *p; p++) {
		if (islower(*p))
			flags |= 1 << (*p - 'a');
		else if (isupper(*p))
			flags |= 1 << (26 + (*p - 'A'));

		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number)
		flags = atol(flag);

	return flags;
}


/* load the rooms */
void
parse_room(FILE * fl, int vnum_nr)
{
	static int room_nr = 0;
	int t[10];
	char line[256], flags[128];
	struct extra_descr_data *new_descr = NULL, *t_exdesc = NULL;
	struct special_search_data *new_search = NULL, *t_srch = NULL;
	struct zone_data *zone = NULL;
	struct room_data *room = NULL, *tmp_room = NULL;

	sprintf(buf2, "room #%d", vnum_nr);

	zone = zone_table;

	while (vnum_nr > zone->top) {
		if (!(zone = zone->next) || vnum_nr < (zone->number * 100)) {
			fprintf(stderr, "Room %d is outside of any zone.\n", vnum_nr);
			safe_exit(1);
		}
	}

	//CREATE(room, struct room_data, 1);
	room = new room_data(vnum_nr, zone);
	room->name = fread_string(fl, buf2);
	room->description = fread_string(fl, buf2);
	room->sounds = NULL;

	if (!get_line(fl, line)
		|| sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
		fprintf(stderr, "Format error in room #%d\n", vnum_nr);
		safe_exit(1);
	}

	room->room_flags = asciiflag_conv(flags);
	room->sector_type = t[2];
	/* Defaults set in constructor
	   room->zone = zone;
	   room->number = vnum_nr;
	   room->func = NULL;
	   room->ticl_ptr = NULL;
	   room->contents = NULL;
	   room->people = NULL;
	   room->light = 0;            // Zero light sources
	   room->max_occupancy = 256;  //Default value set here
	   room->find_first_step_index = 0;
	   for (i = 0; i < NUM_OF_DIRS; i++)
	   room->dir_option[i] = NULL;

	   room->ex_description = NULL;
	   room->search = NULL;
	   room->flow_dir = 0;
	   room->flow_speed = 0;
	   room->flow_type = 0;
	 */

	// t[0] is the zone number; ignored with the zone-file system
	zone = real_zone(t[0]);

	if (zone == NULL) {
		fprintf(stderr, "Room %d outside of any zone.\n", vnum_nr);
		safe_exit(1);
	}


	sprintf(buf, "Format error in room #%d (expecting D/E/S)", vnum_nr);

	for (;;) {
		if (!get_line(fl, line)) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, buf);
			safe_exit(1);
		}
		switch (*line) {
		case 'O':
			room->max_occupancy = atoi(line + 1);
			break;
		case 'D':
			setup_dir(fl, room, atoi(line + 1));
			break;
		case 'E':
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = fread_string(fl, buf2);
			new_descr->description = fread_string(fl, buf2);

			/* put the new exdesc at the end of the linked list, not head */

			new_descr->next = NULL;
			if (!room->ex_description)
				room->ex_description = new_descr;
			else {
				for (t_exdesc = room->ex_description; t_exdesc;
					t_exdesc = t_exdesc->next) {
					if (!(t_exdesc->next)) {
						t_exdesc->next = new_descr;
						break;
					}
				}

				if (!t_exdesc) {
					fprintf(stderr, "Error building exdesc list in room %d.",
						vnum_nr);
					safe_exit(1);
				}
			}
			break;
		case 'L':
			room->sounds = fread_string(fl, buf2);
			break;
		case 'F':
			if (!get_line(fl, line)
				|| sscanf(line, "%d %d %d", t, t + 1, t + 2) != 3) {
				fprintf(stderr, "Flow field incorrect in room #%d.\n",
					vnum_nr);
				safe_exit(1);
			}
			if (FLOW_SPEED(room)) {
				slog("SYSERR: Multiple flow states assigned to room #%d.\n",
					vnum_nr);
			}
			if (t[0] < 0 || t[0] >= NUM_DIRS) {
				fprintf(stderr,
					"Direction '%d' in room #%d flow field BUNK!\n", t[0],
					vnum_nr);
				safe_exit(1);
			}
			if (t[1] < 0) {
				fprintf(stderr, "Negative speed in room #%d flow field!\n",
					vnum_nr);
				safe_exit(1);
			}
			if (t[2] < 0 || t[2] >= NUM_FLOW_TYPES) {
				slog("SYSERR: Illegal flow type in room #%d.",
					vnum_nr);
				FLOW_TYPE(room) = F_TYPE_NONE;
			} else
				FLOW_TYPE(room) = t[2];
			FLOW_DIR(room) = t[0];
			FLOW_SPEED(room) = t[1];
			break;
		case 'Z':
			CREATE(new_search, struct special_search_data, 1);
			new_search->command_keys = fread_string(fl, buf2);
			new_search->keywords = fread_string(fl, buf2);
			new_search->to_vict = fread_string(fl, buf2);
			new_search->to_room = fread_string(fl, buf2);
			new_search->to_remote = fread_string(fl, buf2);

			if (!get_line(fl, line)) {
				fprintf(stderr, "Search error in room #%d.", vnum_nr);
				safe_exit(1);
			}

			if ((sscanf(line, "%d %d %d %d %d",
						t + 5, t + 1, t + 2, t + 3, t + 4) != 5)) {
				fprintf(stderr, "Search Field format error in room #%d\n",
					vnum_nr);
				safe_exit(1);
			}

			new_search->command = t[5];
			new_search->arg[0] = t[1];
			new_search->arg[1] = t[2];
			new_search->arg[2] = t[3];
			new_search->flags = t[4];

			/*** place the search at the top of the list ***/

			new_search->next = NULL;

			if (!(room->search))
				room->search = new_search;
			else {
				for (t_srch = room->search; t_srch; t_srch = t_srch->next)
					if (!(t_srch->next)) {
						t_srch->next = new_search;
						break;
					}

				if (!t_srch) {
					fprintf(stderr,
						"Error building main search list in room %d.",
						vnum_nr);
					safe_exit(1);
				}
			}
			break;

		case 'S':				/* end of room */
			top_of_world = room_nr++;

			room->next = NULL;

			if (zone->world) {
				for (tmp_room = zone->world; tmp_room;
					tmp_room = tmp_room->next)
					if (!tmp_room->next) {
						tmp_room->next = room;
						break;
					}
			} else
				zone->world = room;

			return;
			break;
		default:
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, buf);
			safe_exit(1);
			break;
		}
	}
	if (IS_SET(room->room_flags, ROOM_TUNNEL) && room->max_occupancy > 5) {
		room->max_occupancy = 2;
		REMOVE_BIT(room->room_flags, ROOM_TUNNEL);
	}
}


/* read direction data */
void
setup_dir(FILE * fl, struct room_data *room, int dir)
{
	int t[5];
	char line[256], flags[128];

	sprintf(buf2, "room #%d, direction D%d", room->number, dir);

	if (dir >= NUM_DIRS) {
		slog("ERROR!! 666!! (%d)", room->number);
		safe_exit(1);
	}
	/*
	   if (dir >= FORWARD && dir <= LEFT) {
	   if (!room->dir_option[dir-FORWARD])
	   dir -= FORWARD;
	   } else if (dir == FUTURE || dir == PAST) {
	   if (!room->dir_option[dir-4])
	   dir -= 4;
	   } */

	CREATE(room->dir_option[dir], struct room_direction_data, 1);
	room->dir_option[dir]->general_description = fread_string(fl, buf2);
	room->dir_option[dir]->keyword = fread_string(fl, buf2);

	if (!get_line(fl, line)) {
		fprintf(stderr, "Format error, %s\n", buf2);
		safe_exit(1);
	}
	if (sscanf(line, " %s %d %d ", flags, t + 1, t + 2) != 3) {
		fprintf(stderr, "Format error, %s\n", buf2);
		safe_exit(1);
	}

	room->dir_option[dir]->exit_info = asciiflag_conv(flags);

	room->dir_option[dir]->key = t[1];
	room->dir_option[dir]->to_room = (struct room_data *)t[2];
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void
check_start_rooms(void)
{
	extern room_num mortal_start_room;
	extern room_num immort_start_room;
	extern room_num frozen_start_room;
	extern room_num new_thalos_start_room;
	extern room_num elven_start_room;
	extern room_num istan_start_room;
	extern room_num arena_start_room;
	extern room_num tower_modrian_start_room;
	extern room_num electro_start_room;
	extern room_num monk_start_room;
	extern room_num solace_start_room;
	extern room_num mavernal_start_room;
	extern room_num dwarven_caverns_start_room;
	extern room_num human_square_start_room;
	extern room_num skullport_start_room;
	extern room_num skullport_newbie_start_room;
	extern room_num drow_isle_start_room;
	extern room_num astral_manse_start_room;
	extern room_num zul_dane_start_room;
	extern room_num zul_dane_newbie_start_room;
	extern room_num newbie_school_start_room;

	if ((r_mortal_start_room = real_room(mortal_start_room)) == NULL) {
		slog("SYSERR:  Mortal start room does not exist.  Change in config.c.");
		safe_exit(1);
	}
	if ((r_electro_start_room = real_room(electro_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Electro Centralis start room does not exist.");
		r_electro_start_room = r_mortal_start_room;
	}
	if ((r_immort_start_room = real_room(immort_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Immort start room does not exist.");
		r_immort_start_room = r_mortal_start_room;
	}
	if ((r_frozen_start_room = real_room(frozen_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Frozen start room does not exist.");
		r_frozen_start_room = r_mortal_start_room;
	}
	if ((r_new_thalos_start_room = real_room(new_thalos_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: New Thalos start room does not exist.");
		r_new_thalos_start_room = r_mortal_start_room;
	}
	if ((r_elven_start_room = real_room(elven_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Elven Village start room does not exist.");
		r_elven_start_room = r_mortal_start_room;
	}
	if ((r_istan_start_room = real_room(istan_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Istan start room does not exist.");
		r_istan_start_room = r_mortal_start_room;
	}
	if ((r_arena_start_room = real_room(arena_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Arena start room does not exist.");
		r_arena_start_room = r_mortal_start_room;
	}
	if ((r_tower_modrian_start_room =
			real_room(tower_modrian_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Arena start room does not exist.");
		r_tower_modrian_start_room = r_mortal_start_room;
	}
	if ((r_monk_start_room = real_room(monk_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Monk start room does not exist.");
		r_monk_start_room = r_mortal_start_room;
	}
	if ((r_skullport_newbie_start_room =
			real_room(skullport_newbie_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Skullport Newbie Academy start room does not exist.");
		r_skullport_newbie_start_room = r_mortal_start_room;
	}
	if ((r_solace_start_room = real_room(solace_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Solace Cove start room does not exist.");
		r_solace_start_room = r_mortal_start_room;
	}
	if ((r_mavernal_start_room = real_room(mavernal_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Mavernal start room does not exist.");
		r_mavernal_start_room = r_mortal_start_room;
	}
	if ((r_dwarven_caverns_start_room =
			real_room(dwarven_caverns_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Dwarven Caverns  start room does not exist.");
		r_dwarven_caverns_start_room = r_mortal_start_room;
	}
	if ((r_human_square_start_room =
			real_room(human_square_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Human Square start room does not exist.");
		r_human_square_start_room = r_mortal_start_room;
	}
	if ((r_skullport_start_room = real_room(skullport_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Skullport start room does not exist.");
		r_skullport_start_room = r_mortal_start_room;
	}
	if ((r_drow_isle_start_room = real_room(drow_isle_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Drow Isle start room does not exist.");
		r_drow_isle_start_room = r_mortal_start_room;
	}
	if ((r_astral_manse_start_room =
			real_room(astral_manse_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Astral Manse start room does not exist.");
		r_astral_manse_start_room = r_mortal_start_room;
	}

	if ((r_zul_dane_start_room = real_room(zul_dane_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Zul Dane start room does not exist.");
		r_zul_dane_start_room = r_mortal_start_room;
	}

	if ((r_zul_dane_newbie_start_room =
			real_room(zul_dane_newbie_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Zul Dane newbie start room does not exist.");
		// set it to the normal zul dane room ...
		r_zul_dane_newbie_start_room = r_zul_dane_start_room;
	}
	if ((r_newbie_school_start_room =
			real_room(newbie_school_start_room)) == NULL) {
		if (!mini_mud)
			slog("SYSERR:  Warning: Zul Dane newbie start room does not exist.");
		// set it to the normal zul dane room ...
		r_newbie_school_start_room = r_mortal_start_room;
	}


}

void
renum_world(void)
{
	register int door, found;
	int vnum;
	struct room_data *room = NULL, *room1 = NULL;
	struct zone_data *zone = NULL, *zone1 = NULL;

	for (zone = zone_table; zone; zone = zone->next)
		for (room = zone->world; room; room = room->next)
			for (door = 0; door < NUM_OF_DIRS; door++)
				if (room->dir_option[door])
					if ((room_num) room->dir_option[door]->to_room != NOWHERE) {
						vnum = (room_num) room->dir_option[door]->to_room;
						room->dir_option[door]->to_room = NULL;
						for (found = 0, room1 = zone->world;
							room1 && found != 1; room1 = room1->next)
							if (room1->number == vnum) {
								room->dir_option[door]->to_room =
									(struct room_data *)room1;
								found = 1;
							}
						if (found == 0) {
							zone1 = real_zone(vnum / 100);
							if (zone1 == NULL) {
								for (zone1 = zone_table; zone1;
									zone1 = zone1->next)
									for (found = 0, room1 = zone1->world;
										room1 && found != 1;
										room1 = room1->next)
										if (room1->number == vnum) {
											room->dir_option[door]->to_room =
												(struct room_data *)room1;
											found = 1;
										}
							} else
								for (found = 0, room1 = zone1->world;
									room1 && found != 1; room1 = room1->next)
									if (room1->number == vnum) {
										room->dir_option[door]->to_room =
											(struct room_data *)room1;
										found = 1;
									}
							if (found == 0) {
								for (zone1 = zone_table; zone1;
									zone1 = zone1->next)
									for (found = 0, room1 = zone1->world;
										room1 && found != 1;
										room1 = room1->next)
										if (room1->number == vnum) {
											room->dir_option[door]->to_room =
												(struct room_data *)room1;
											found = 1;
										}
							}
						}
					} else
						room->dir_option[door]->to_room = NULL;

}


/*************
void renum_world(void)
{
  register int door;
  register struct room_data *room;
  register struct zone_data *zone;
  
  for (zone = zone_table; zone; zone = zone->next)
    for (room = zone->world; room; room = room->next)
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (room->dir_option[door]&&room->dir_option[door]->to_room!=NOWHERE)
            room->dir_option[door]->to_room = 
              real_room(room->dir_option[door]->to_room);

}
***/

#define ZCMD         zone->cmd
#define new_ZCMD new_zone->cmd

/* resulve vnums into rnums in the zone reset tables */
void
renum_zone_table(void)
{
	int a, b;
	struct zone_data *zone;
	struct reset_com *zonecmd;
	struct room_data *room;

	for (zone = zone_table; zone; zone = zone->next)
		for (zonecmd = zone->cmd; zonecmd && zonecmd->command != 'S';
			zonecmd = zonecmd->next) {
			a = b = 0;
			switch (zonecmd->command) {
			case 'M':
				a = zonecmd->arg1;
				room = real_room(zonecmd->arg3);
				if (room != NULL)
					b = zonecmd->arg3 = room->number;
				else
					b = NOWHERE;
				break;
			case 'O':
				a = zonecmd->arg1;
				room = real_room(zonecmd->arg3);
				if (room != NULL)
					b = room->number;
				else
					b = NOWHERE;
				break;
			case 'G':
				a = zonecmd->arg1;
				break;
			case 'I':
				a = zonecmd->arg1;
				break;
			case 'E':
				a = zonecmd->arg1;
				break;
			case 'P':
				a = zonecmd->arg1;
				b = zonecmd->arg3;
				break;
			case 'D':
				room = real_room(zonecmd->arg1);
				if (room != NULL)
					a = zonecmd->arg1 = room->number;
				else
					a = NOWHERE;
				break;
			case 'R':			/* rem obj from room */
				a = zonecmd->arg1;
				room = real_room(zonecmd->arg2);
				if (room != NULL)
					b = zonecmd->arg2 = room->number;
				else
					b = NOWHERE;
				break;
			}
			if (a < 0 || b < 0) {
				if (!mini_mud && !scheck)
					log_zone_error(zone, zonecmd,
						"Invalid vnum, cmd disabled");
				zonecmd->command = '*';
			}
		}
}


void
set_physical_attribs(struct Creature *ch)
{
	GET_MAX_MANA(ch) = MAX(100, (GET_LEVEL(ch) << 3));
	GET_MAX_MOVE(ch) = MAX(100, (GET_LEVEL(ch) << 4));

	if (GET_RACE(ch) == RACE_HUMAN || IS_HUMANOID(ch) ||
		GET_RACE(ch) == RACE_MOBILE) {
		ch->player.weight = number(130, 180) + (GET_STR(ch) << 1);
		ch->player.height = number(140, 180) + (GET_WEIGHT(ch) >> 3);
    } else if (GET_RACE(ch) == RACE_ROTARIAN) {
        ch->player.weight = number(300, 450);
        ch->player.height = number(200, 325);
    } else if (GET_RACE(ch) == RACE_GRIFFIN) {
        ch->player.weight = number(1500, 2300);
        ch->player.height = number(400, 550);
	} else if (GET_RACE(ch) == RACE_DWARF) {
		ch->player.weight = number(120, 160) + (GET_STR(ch) << 1);
		ch->player.height = number(100, 115) + (GET_WEIGHT(ch) >> 4);
		ch->real_abils.str = 15;
	} else if (IS_ELF(ch) || IS_DROW(ch)) {
		ch->player.weight = number(120, 180) + (GET_STR(ch) << 1);
		ch->player.height = number(140, 155) + (GET_WEIGHT(ch) >> 3);
		ch->real_abils.intel = 15;
	} else if (GET_RACE(ch) == RACE_HALF_ORC || IS_ORC(ch)) {
		ch->player.weight = number(120, 180) + (GET_STR(ch) << 1);
		ch->player.height = number(120, 190) + (GET_WEIGHT(ch) >> 3);
	} else if (GET_RACE(ch) == RACE_HALFLING || IS_GOBLIN(ch)) {
		ch->player.weight = number(110, 150) + (GET_STR(ch) << 1);
		ch->player.height = number(100, 125) + (GET_WEIGHT(ch) >> 3);
		ch->real_abils.dex = 15;
	} else if (GET_RACE(ch) == RACE_WEMIC) {
		ch->player.weight = number(500, 560) + (GET_STR(ch) << 1);
	}

	if (ch->player.sex == SEX_FEMALE) {
		ch->player.weight = (int)(ch->player.weight * 0.75);
		ch->player.height = (int)(ch->player.weight * 0.75);
	}

	if (GET_RACE(ch) == RACE_GIANT) {
		if (GET_CLASS(ch) == CLASS_STONE) {
			ch->player.weight = 18 * 40;
			ch->player.height = 18 * 30;
			ch->real_abils.str = 20;
		} else if (GET_CLASS(ch) == CLASS_FROST) {
			ch->player.weight = 21 * 30;
			ch->player.height = 21 * 31;
			ch->real_abils.str = 21;
			GET_MAX_MANA(ch) += GET_LEVEL(ch);
		} else if (GET_CLASS(ch) == CLASS_FIRE) {
			ch->player.weight = 18 * 30;
			ch->player.height = 18 * 31;
			ch->real_abils.str = 22;
		} else if (GET_CLASS(ch) == CLASS_CLOUD) {
			ch->player.weight = 24 * 30;
			ch->player.height = 24 * 31;
			ch->real_abils.str = 23;
		} else if (GET_CLASS(ch) == CLASS_STORM) {
			ch->player.weight = 26 * 30;
			ch->player.height = 26 * 31;
			ch->real_abils.str = 24;
		} else {
			ch->player.weight = 16 * 30;
			ch->player.height = 16 * 31;
			ch->real_abils.str = 19;
		}
	} else if (IS_OGRE(ch) || IS_TROLL(ch)) {
		ch->player.weight = 9 * 30 + number(10, 100);
		ch->player.height = 9 * 31 + (GET_WEIGHT(ch) >> 3);
		ch->real_abils.str = number(16, 18);
	} else if (IS_MINOTAUR(ch)) {
		ch->player.weight = number(400, 640) + GET_STR(ch);
		ch->player.height = number(200, 425) + (GET_WEIGHT(ch) >> 3);
		ch->real_abils.str = number(15, 19);
	} else if (IS_DRAGON(ch)) {
		if (GET_CLASS(ch) == CLASS_WHITE) {
			ch->player.height = (24 + GET_LEVEL(ch)) * 31;
			ch->player.weight = (24 + GET_LEVEL(ch)) * 30;
			ch->real_abils.str = 18;
		} else {
			ch->player.height = (30 + GET_LEVEL(ch)) * 31;
			ch->player.weight = (30 + GET_LEVEL(ch)) * 30;
			ch->real_abils.str = 18;
		}
	} else if (IS_ANIMAL(ch)) {
		if (GET_CLASS(ch) == CLASS_BIRD) {
			ch->player.weight = number(10, 60) + GET_STR(ch);
			ch->player.height = number(30, 90) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_CLASS(ch) == CLASS_SNAKE) {
			ch->player.weight = number(10, 30) + GET_LEVEL(ch);
			ch->player.height = number(30, 90) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_CLASS(ch) == CLASS_SMALL) {
			ch->player.weight = number(10, 30) + GET_STR(ch);
			ch->player.height = number(20, 40) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_CLASS(ch) == CLASS_MEDIUM) {
			ch->player.weight = number(20, 80) + GET_STR(ch);
			ch->player.height = number(50, 100) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_CLASS(ch) == CLASS_LARGE) {
			ch->player.weight = number(130, 360) + GET_STR(ch);
			ch->player.height = number(120, 240) + (GET_WEIGHT(ch) >> 3);
		} else {
			ch->player.weight = number(60, 160) + GET_STR(ch);
			ch->player.height = number(100, 190) + (GET_WEIGHT(ch) >> 3);
		}
	} else if (IS_DEVIL(ch)) {
		if (GET_CLASS(ch) == CLASS_ARCH) {
			ch->player.weight = 11 * 40;
			ch->player.height = 11 * 31;
			ch->real_abils.str = 23;
			ch->real_abils.intel = 22;
			ch->real_abils.cha = 22;
			ch->real_abils.wis = 23;
			ch->real_abils.dex = 23;
			ch->real_abils.con = 23;
			GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 3);
		} else if (GET_CLASS(ch) == CLASS_DUKE) {
			ch->player.weight = 10 * 40;
			ch->player.height = 10 * 31;
			ch->real_abils.str = 22;
			ch->real_abils.intel = 20;
			ch->real_abils.cha = 20;
			ch->real_abils.wis = 21;
			ch->real_abils.dex = 21;
			ch->real_abils.con = 21;
			GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 2);
		} else if (GET_CLASS(ch) == CLASS_GREATER) {
			ch->player.weight = 10 * 40;
			ch->player.height = 10 * 31;
			ch->real_abils.str = 21;
			ch->real_abils.intel = 18;
			ch->real_abils.wis = 20;
			ch->real_abils.dex = 20;
			ch->real_abils.con = 20;
			GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 2);
		} else if (GET_CLASS(ch) == CLASS_LESSER) {
			ch->player.weight = 6 * 30;
			ch->player.height = 6 * 31;
			ch->real_abils.str = 19;
			ch->real_abils.wis = 18;
			ch->real_abils.dex = 18;
			ch->real_abils.con = 18;
		} else if (GET_CLASS(ch) == CLASS_LEAST) {
			ch->player.weight = 4 * 30;
			ch->player.height = 4 * 31;
			ch->real_abils.str = 16;
		} else {
			ch->player.weight = 6 * 40;
			ch->player.height = 6 * 31;
			ch->real_abils.str = 18;
		}
	}
	if (IS_CLERIC(ch) || IS_MAGE(ch) || IS_LICH(ch) || IS_PHYSIC(ch) ||
		IS_PSYCHIC(ch))
		GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 2);
	else if (IS_KNIGHT(ch) || IS_RANGER(ch))
		GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 1);

	if (IS_RANGER(ch))
		GET_MAX_MOVE(ch) += (GET_LEVEL(ch) << 3);

	ch->aff_abils = ch->real_abils;
}

void
parse_simple_mob(FILE * mob_f, struct Creature *mobile, int nr)
{
	int j, t[10];
	char line[256];

	mobile->real_abils.str = 11;
	mobile->real_abils.intel = 11;
	mobile->real_abils.wis = 11;
	mobile->real_abils.dex = 11;
	mobile->real_abils.con = 11;
	mobile->real_abils.cha = 11;

	get_line(mob_f, line);
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
			t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
		fprintf(stderr, "Format error in mob #%d, first line after S flag\n"
			"...expecting line of form '# # # #d#+# #d#+#'\n", nr);
		safe_exit(1);
	}
	GET_LEVEL(mobile) = t[0];
	mobile->points.hitroll = 20 - t[1];
	mobile->points.armor = 10 * t[2];

	/* max hit = 0 is a flag that H, M, V is xdy+z */
	mobile->points.max_hit = 0;
	mobile->points.hit = t[3];
	mobile->points.mana = t[4];
	mobile->points.move = t[5];

	mobile->points.max_mana = 10;
	mobile->points.max_move = 50;

	mobile->mob_specials.shared->damnodice = t[6];
	mobile->mob_specials.shared->damsizedice = t[7];
	mobile->points.damroll = t[8];

	if (MOB_FLAGGED(mobile, MOB_WIMPY))
		mobile->mob_specials.shared->morale = MAX(30, GET_LEVEL(mobile));
	else
		mobile->mob_specials.shared->morale = 100;

	mobile->mob_specials.shared->lair = -1;
	mobile->mob_specials.shared->leader = -1;

	// for accounting statistics
	mobile->mob_specials.shared->kills = 0;
	mobile->mob_specials.shared->loaded = 0;

	get_line(mob_f, line);
	if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) == 4) {
		mobile->player.char_class = t[3];
		mobile->player.race = t[2];
	} else {
		mobile->player.char_class = CLASS_NORMAL;
		mobile->player.race = RACE_MOBILE;
	}
	GET_GOLD(mobile) = t[0];
	GET_EXP(mobile) = t[1];

	get_line(mob_f, line);
	if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
		mobile->mob_specials.shared->attack_type = t[3];
	else
		mobile->mob_specials.shared->attack_type = 0;

	mobile->char_specials.position = t[0];
	mobile->mob_specials.shared->default_pos = t[1];
	mobile->player.sex = t[2];

	mobile->player.weight = 200;
	mobile->player.height = 198;

	mobile->player.remort_char_class = -1;
	set_physical_attribs(mobile);

	for (j = 0; j < 3; j++)
		GET_COND(mobile, j) = -1;

	/*
	 * these are now save applies; base save numbers for MOBs are now from
	 * the warrior save table.
	 */
	for (j = 0; j < 5; j++)
		GET_SAVE(mobile, j) = 0;
}



/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void
interpret_espec(char *keyword, char *value, struct Creature *mobile, int nr)
{
	int num_arg, matched = 0;

	num_arg = atoi(value);

	CASE("BareHandAttack") {
		RANGE(0, 99);
		mobile->mob_specials.shared->attack_type = num_arg;
	}

	CASE("Move_buf") {
		MOB_SHARED(mobile)->move_buf = str_dup(value);
	}

	CASE("Str") {
		RANGE(3, 25);
		mobile->real_abils.str = num_arg;
	}

	CASE("StrAdd") {
		RANGE(0, 100);
		mobile->real_abils.str_add = num_arg;
	}

	CASE("Int") {
		RANGE(3, 25);
		mobile->real_abils.intel = num_arg;
	}

	CASE("Wis") {
		RANGE(3, 25);
		mobile->real_abils.wis = num_arg;
	}

	CASE("Dex") {
		RANGE(3, 25);
		mobile->real_abils.dex = num_arg;
	}

	CASE("Con") {
		RANGE(3, 25);
		mobile->real_abils.con = num_arg;
	}

	CASE("Cha") {
		RANGE(3, 25);
		mobile->real_abils.cha = num_arg;
	}

	CASE("MaxMana") {
		RANGE(0, 4000);
		mobile->points.max_mana = num_arg;
	}
	CASE("MaxMove") {
		RANGE(0, 2000);
		mobile->points.max_move = num_arg;
	}
	CASE("Height") {
		RANGE(1, 10000);
		mobile->player.height = num_arg;
	}
	CASE("Weight") {
		RANGE(1, 10000);
		mobile->player.weight = num_arg;
	}
	CASE("RemortClass") {
		RANGE(0, 1000);
		mobile->player.remort_char_class = num_arg;
	}
	CASE("Class") {
		RANGE(0, 1000);
		mobile->player.char_class = num_arg;
	}
	CASE("Race") {
		RANGE(0, 1000);
		mobile->player.race = num_arg;
	}
	CASE("Credits") {
		RANGE(0, 1000000);
		mobile->points.cash = num_arg;
	}
	CASE("Cash") {
		RANGE(0, 1000000);
		mobile->points.cash = num_arg;
	}
	CASE("Econet") {
		RANGE(0, 1000000);
		mobile->points.credits = num_arg;
	}
	CASE("Morale") {
		RANGE(0, 120);
		mobile->mob_specials.shared->morale = num_arg;
	}
	CASE("Lair") {
		RANGE(-99999, 99999);
		mobile->mob_specials.shared->lair = num_arg;
	}
	CASE("Leader") {
		RANGE(-99999, 99999);
		mobile->mob_specials.shared->leader = num_arg;
	}
	CASE("IScript") {
		RANGE(-99999, 99999);
		mobile->mob_specials.shared->svnum = num_arg;
	}
	if (!matched) {
		fprintf(stderr, "Warning: unrecognized espec keyword %s in mob #%d\n",
			keyword, nr);
	}
}

#undef CASE
#undef RANGE

void
parse_espec(char *buf, struct Creature *mobile, int nr)
{
	char *ptr;

	if ((ptr = strchr(buf, ':')) != NULL) {
		*(ptr++) = '\0';
		while (isspace(*ptr))
			ptr++;
	} else
		ptr = "";

	interpret_espec(buf, ptr, mobile, nr);
}


void
parse_enhanced_mob(FILE * mob_f, struct Creature *mobile, int nr)
{
	char line[256];

	parse_simple_mob(mob_f, mobile, nr);

	while (get_line(mob_f, line)) {
		if (!strcmp(line, "E"))	/* end of the ehanced section */
			return;
		else if (*line == '#') {	/* we've hit the next mob, maybe? */
			fprintf(stderr, "Unterminated E section in mob #%d\n", nr);
			safe_exit(1);
		} else
			parse_espec(line, mobile, nr);
	}

	fprintf(stderr, "Unexpected end of file reached after mob #%d\n", nr);
	safe_exit(1);
}

void
load_ticl(FILE * ticl_f, int nr)
{
	int retval = 0, j[1];
	long t[4];
	struct ticl_data *ticl = NULL, *tmp_ticl = NULL;
	struct zone_data *zone = NULL;
	static char line[256];

	zone = zone_table;

	while (nr > zone->top) {
		if (!(zone = zone->next) || nr < (zone->number * 100)) {
			fprintf(stderr, "TICL proc %d is outside of any zone.\n", nr);
			safe_exit(1);
		}
	}

	CREATE(ticl, struct ticl_data, 1);

	sprintf(buf2, "TICL vnum %d", nr);

	ticl->vnum = nr;
	ticl->proc_id = 0;
	ticl->times_executed = 0;
	ticl->flags = 0;

	if ((ticl->title = fread_string(ticl_f, buf2)) == NULL) {
		fprintf(stderr, "Null TICL title or format error at or near %s\n",
			buf2);
		safe_exit(1);
	}

	if (!get_line(ticl_f, line) ||
		(retval =
			sscanf(line, "%ld %ld %ld %ld %d", t, t + 1, t + 2, t + 3,
				j)) != 5) {
		fprintf(stderr,
			"Format error in ticl numeric line (expecting 5 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}

	ticl->creator = t[0];
	ticl->date_created = t[1];
	ticl->last_modified = t[2];
	ticl->last_modified_by = t[3];
	ticl->compiled = j[0];

	ticl->code = fread_string(ticl_f, buf2);

	ticl->next = NULL;

	if (zone->ticl_list) {
		for (tmp_ticl = zone->ticl_list; tmp_ticl; tmp_ticl = tmp_ticl->next)
			if (!tmp_ticl->next) {
				tmp_ticl->next = ticl;
				break;
			}
	} else
		zone->ticl_list = ticl;

}

void
parse_mobile(FILE * mob_f, int nr)
{
	struct extra_descr_data *new_descr = NULL;
	bool done = false;
	static int i = 0;
	int j, t[10];
	char line[256], *tmpptr, letter;
	char f1[128], f2[128], f3[128], f4[128], f5[128];
	struct Creature *mobile = NULL, *tmp_mob = NULL;

	CREATE(mobile, struct Creature, 1);

	clear_char(mobile);

	tmp_mob = real_mobile_proto(nr);

	CREATE(mobile->mob_specials.shared, struct mob_shared_data, 1);
	mobile->mob_specials.shared->vnum = nr;
	mobile->mob_specials.shared->number = 0;
	mobile->mob_specials.shared->func = NULL;
	mobile->mob_specials.shared->ticl_ptr = NULL;
	mobile->mob_specials.shared->move_buf = NULL;
	mobile->mob_specials.shared->proto = mobile;

	mobile->player_specials = &dummy_mob;
	sprintf(buf2, "mob vnum %d", nr);

	/***** String data *** */
	mobile->player.name = fread_string(mob_f, buf2);
	tmpptr = mobile->player.short_descr = fread_string(mob_f, buf2);
	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
			!str_cmp(fname(tmpptr), "the"))
			*tmpptr = tolower(*tmpptr);
	mobile->player.long_descr = fread_string(mob_f, buf2);
	if (mobile->player.long_descr) {
		char *read_pt;

		read_pt = mobile->player.long_descr;
		while (*read_pt)
			read_pt++;
		if ('\n' == read_pt[-1] && '\r' == read_pt[-2])
			read_pt[-2] = '\0';
	}
	mobile->player.description = fread_string(mob_f, buf2);
	mobile->player.title = NULL;

	/* *** Numeric data *** */
	get_line(mob_f, line);
	sscanf(line, "%s %s %s %s %s %d %c", f1, f2, f3, f4, f5, t + 5, &letter);
	MOB_FLAGS(mobile) = asciiflag_conv(f1);
	MOB2_FLAGS(mobile) = asciiflag_conv(f2);
	REMOVE_BIT(MOB2_FLAGS(mobile), MOB2_RENAMED);
	SET_BIT(MOB_FLAGS(mobile), MOB_ISNPC);
	AFF_FLAGS(mobile) = asciiflag_conv(f3);
	AFF2_FLAGS(mobile) = asciiflag_conv(f4);
	AFF3_FLAGS(mobile) = asciiflag_conv(f5);
	GET_ALIGNMENT(mobile) = t[5];



	switch (letter) {
	case 'S':					/* Simple monsters */
		parse_simple_mob(mob_f, mobile, nr);
		break;
	case 'E':					/* Circle3 Enhanced monsters */
		parse_enhanced_mob(mob_f, mobile, nr);
		break;
		/* add new mob types here.. */
	default:
		fprintf(stderr, "Unsupported mob type '%c' in mob #%d\n", letter, nr);
		safe_exit(1);
		break;
	}

/*      load reply structors till 'S' incountered */
	do {
		get_line(mob_f, line);
		switch (line[0]) {
		case 'R':				/* reply text */
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = fread_string(mob_f, buf2);
			new_descr->description = fread_string(mob_f, buf2);
			new_descr->next = mobile->mob_specials.response;
			mobile->mob_specials.response = new_descr;
			break;
		case '$':
		case '#':				/* we hit a # so there is no end of replys */
			fseek(mob_f, 0 - (strlen(line) + 1), SEEK_CUR);
			done = true;
			break;
		}
	} while (!done);

	mobile->aff_abils = mobile->real_abils;

	for (j = 0; j < NUM_WEARS; j++)
		mobile->equipment[j] = NULL;

	mobile->desc = NULL;

	mobilePrototypes.add(mobile);
	top_of_mobt = i++;
}

/* read all objects from obj file; generate index and prototypes */
char *
parse_object(FILE * obj_f, int nr)
{
	static int i = 0, retval;
	static char line[256];
	int t[10], j;
	char *tmpptr;
	char f1[256], f2[256], f3[256], f4[256];
	struct extra_descr_data *new_descr;
	struct obj_data *obj = NULL, *tmp_obj = NULL;

	CREATE(obj, struct obj_data, 1);
	
	obj->clear();

	CREATE(obj->shared, struct obj_shared_data, 1);

	obj->shared->vnum = nr;
	obj->shared->number = 0;
	obj->shared->house_count = 0;
	obj->shared->func = NULL;
	obj->shared->ticl_ptr = NULL;
	obj->shared->proto = obj;

	obj->in_room = NULL;

	sprintf(buf2, "object #%d", nr);

	/* *** string data *** */
	if ((obj->name = fread_string(obj_f, buf2)) == NULL) {
		fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
		safe_exit(1);
	}
	tmpptr = obj->short_description = fread_string(obj_f, buf2);
	if (*tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
			!str_cmp(fname(tmpptr), "the"))
			*tmpptr = tolower(*tmpptr);

	tmpptr = obj->description = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
		*tmpptr = toupper(*tmpptr);
	obj->action_description = fread_string(obj_f, buf2);

	/* *** numeric data *** */
	retval = 0;
	if (!get_line(obj_f, line)) {
		fprintf(stderr, "Unable to read first numeric line for object %d.\n",
			nr);
		safe_exit(1);
	}

	retval = sscanf(line, " %d %s %s %s %s", t, f1, f2, f3, f4);

	obj->obj_flags.type_flag = t[0];
	obj->obj_flags.extra_flags = asciiflag_conv(f1);
	obj->obj_flags.extra2_flags = asciiflag_conv(f2);
	obj->obj_flags.wear_flags = asciiflag_conv(f3);

	// old format numeric line
	if (retval == 4) {
		obj->obj_flags.extra3_flags = 0;
	}							// new format numeric line
	else if (retval == 5) {
		obj->obj_flags.extra3_flags = asciiflag_conv(f4);
	}
	// wrong number of fields
	else {
		fprintf(stderr,
			"Format error in first nmric line (expctng 4 or 5 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}

	if (!get_line(obj_f, line) ||
		(retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
		fprintf(stderr,
			"Format error in second numeric line (expecting 4 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}
	obj->obj_flags.value[0] = t[0];
	obj->obj_flags.value[1] = t[1];
	obj->obj_flags.value[2] = t[2];
	obj->obj_flags.value[3] = t[3];

	if (!get_line(obj_f, line) ||
		(retval = sscanf(line, "%d %d %d", t, t + 1, t + 2)) != 3) {
		fprintf(stderr,
			"Format error in third numeric line (expecting 3 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}
	obj->obj_flags.material = t[0];
	obj->obj_flags.max_dam = t[1];
	obj->obj_flags.damage = t[2];

	if (!get_line(obj_f, line) ||
		(retval = sscanf(line, "%d %d %d", t, t + 1, t + 2)) != 3) {
		fprintf(stderr,
			"Format error in fourth numeric line (expecting 3 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}
	obj->setWeight(t[0]);
	obj->shared->cost = t[1];
	obj->shared->cost_per_day = t[2];


	/* check to make sure that weight of containers exceeds curr. quantity */
	if (obj->obj_flags.type_flag == ITEM_DRINKCON ||
		obj->obj_flags.type_flag == ITEM_FOUNTAIN) {
		if (obj->getWeight() < obj->obj_flags.value[1])
			obj->setWeight(obj->obj_flags.value[1] + 5);
	}

	/* *** extra descriptions and affect fields *** */

	for (j = 0; j < MAX_OBJ_AFFECT; j++) {
		obj->affected[j].location = APPLY_NONE;
		obj->affected[j].modifier = 0;
	}
	obj->obj_flags.bitvector[0] = 0;
	obj->obj_flags.bitvector[1] = 0;
	obj->obj_flags.bitvector[2] = 0;

	strcat(buf2, ", after numeric constants (expecting E/A/#xxx)");
	j = 0;

	for (;;) {
		if (!get_line(obj_f, line)) {
			fprintf(stderr, "Format error in %s\n", buf2);
			safe_exit(1);
		}
		switch (*line) {
		case 'E':
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = fread_string(obj_f, buf2);
			new_descr->description = fread_string(obj_f, buf2);
			new_descr->next = obj->ex_description;
			obj->ex_description = new_descr;
			break;
		case 'A':
			if (j >= MAX_OBJ_AFFECT) {
				fprintf(stderr, "Too many A fields (%d max), %s\n",
					MAX_OBJ_AFFECT, buf2);
				safe_exit(1);
			}
			get_line(obj_f, line);
			sscanf(line, " %d %d ", t, t + 1);
			obj->affected[j].location = t[0];
			obj->affected[j].modifier = t[1];
			j++;
			break;
		case 'V':
			get_line(obj_f, line);
			sscanf(line, " %d %s ", t, f1);
			if (t[0] < 1 || t[0] > 3)
				break;
			obj->obj_flags.bitvector[t[0] - 1] = asciiflag_conv(f1);
			break;
		case '$':
		case '#':
			top_of_objt = i++;

			obj->next = NULL;

			if (obj_proto) {
				for (tmp_obj = obj_proto; tmp_obj; tmp_obj = tmp_obj->next)
					if (!tmp_obj->next) {
						tmp_obj->next = obj;
						break;
					}
			} else
				obj_proto = obj;

			return line;
			break;
		default:
			fprintf(stderr, "Format error in %s\n", buf2);
			safe_exit(1);
			break;
		}
	}
}



/* load the zone table and command tables */
void
load_zones(FILE * fl, char *zonename)
{
	int num_of_cmds = 0, line_num = 0, tmp, tmp2, error, cmd_num = 0;
	char *ptr, buf[256], zname[256], flags[128];
	struct zone_data *new_zone, *zone = NULL;
	struct reset_com *zonecmd, *new_zonecmd = NULL;
	struct weather_data *weather = NULL;

	/* Let's allocate the memory for the new zone... */

	CREATE(new_zone, struct zone_data, 1);

	strcpy(zname, zonename);

	while (get_line(fl, buf))
		num_of_cmds++;			/* this should be correct within 3 or so */
	rewind(fl);

	if (num_of_cmds == 0) {
		fprintf(stderr, "%s is empty!\n", zname);
		safe_exit(0);
	}

	line_num += get_line(fl, buf);

	if (sscanf(buf, "#%d", &new_zone->number) != 1) {
		fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
		safe_exit(0);
	}
	sprintf(buf2, "beginning of zone #%d", new_zone->number);

	line_num += get_line(fl, buf);
	if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
		*ptr = '\0';

	new_zone->name = str_dup(buf);

	line_num += get_line(fl, buf);
	if (!strncmp(buf, "C ", 2)) {
		new_zone->owner_idnum = atoi(buf + 2);
		line_num += get_line(fl, buf);
	} else
		new_zone->owner_idnum = -1;

	if (!strncmp(buf, "C2 ", 3)) {
		new_zone->co_owner_idnum = atoi(buf + 3);
		line_num += get_line(fl, buf);
	} else
		new_zone->co_owner_idnum = -1;

	if (sscanf(buf, " %d %d %d %d %d %s %d %d", &new_zone->top,
			&new_zone->lifespan, &new_zone->reset_mode,
			&new_zone->time_frame, &new_zone->plane, flags,
			&new_zone->hour_mod, &new_zone->year_mod) != 8) {
		fprintf(stderr, "Format error in 8-constant line of %s\n", zname);
		safe_exit(0);
	}

	new_zone->flags = asciiflag_conv(flags);
	new_zone->num_players = 0;
	new_zone->idle_time = 0;
//    REMOVE_BIT(new_zone->flags, (1 << 12));

	CREATE(weather, struct weather_data, 1);
	weather->pressure = 0;
	weather->change = 0;
	weather->sky = 0;
	weather->sunlight = 0;
	weather->humid = 0;

	new_zone->weather = weather;

	new_zone->ticl_list = NULL;

	for (;;) {
		if ((tmp = get_line(fl, buf)) == 0) {
			fprintf(stderr, "Format error in %s - premature end of file\n",
				zname);
			safe_exit(0);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(&ptr);

		CREATE(new_zonecmd, struct reset_com, 1);

		if ((new_zonecmd->command = *ptr) == '*')
			continue;

		ptr++;

		if (new_zonecmd->command == 'S' || new_zonecmd->command == '$') {
			new_zonecmd->command = 'S';
			{
				free(new_zonecmd);
				break;
			}
		}
		error = 0;
		if (strchr("MOEPDIVW", new_zonecmd->command) == NULL) {
			if (sscanf(ptr, " %d %d %d %d ", &tmp, &tmp2,
					&new_zonecmd->arg1, &new_zonecmd->arg2) != 4)
				error = 1;
		} else {
			if (new_zonecmd->command == 'D') {
				if (sscanf(ptr, " %d %d %d %d %s ", &tmp, &tmp2,
						&new_zonecmd->arg1, &new_zonecmd->arg2, flags) != 5)
					error = 1;
				if (error != 1)
					new_zonecmd->arg3 = asciiflag_conv(flags);

			} else {
				if (sscanf(ptr, " %d %d %d %d %d ", &tmp, &tmp2,
						&new_zonecmd->arg1,
						&new_zonecmd->arg2, &new_zonecmd->arg3) != 5)
					error = 1;
			}
		}
		new_zonecmd->if_flag = tmp;
		new_zonecmd->prob = tmp2;

		if (error) {
			fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname,
				line_num, buf);
			safe_exit(0);
		}
		new_zonecmd->line = cmd_num;

		if (new_zone->cmd) {
			for (zonecmd = new_zone->cmd; zonecmd; zonecmd = zonecmd->next)
				if (!zonecmd->next) {
					zonecmd->next = new_zonecmd;
					break;
				}
		} else
			new_zone->cmd = new_zonecmd;

		cmd_num++;
	}

	/* Now we add the new zone to the zone_table linked list */

	new_zone->next = NULL;

	if (zone_table) {
		for (zone = zone_table; zone; zone = zone->next)
			if (!zone->next) {
				zone->next = new_zone;
				break;
			}
	} else
		zone_table = new_zone;


	top_of_zone_table++;
}

#undef Z


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                  *
*********************************************************************** */



int
vnum_mobile(char *searchname, struct Creature *ch)
{
	struct Creature *mobile;
	int found = 0;

	strcpy(buf, "");

	CreatureList::iterator mit = mobilePrototypes.begin();
	for (; mit != mobilePrototypes.end(); ++mit) {
		//for (mobile = mob_proto; mobile; mobile = mobile->next) {
		mobile = *mit;
		if (isname(searchname, mobile->player.name)) {
			sprintf(buf, "%s%3d. %s[%s%5d%s]%s %s%s\r\n", buf, ++found,
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				mobile->mob_specials.shared->vnum,
				CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
				mobile->player.short_descr, CCNRM(ch, C_NRM));
			if (strlen(buf) + 128 > MAX_STRING_LENGTH) {
				strcat(buf, "**OVERFLOW**\r\n");
				break;
			}
		}
	}
	if (found)
		page_string(ch->desc, buf);
	return (found);
}



int
vnum_object(char *searchname, struct Creature *ch)
{
	struct obj_data *obj = NULL;
	int found = 0;

	strcpy(buf, "");
	for (obj = obj_proto; obj; obj = obj->next) {
		if (isname(searchname, obj->name)) {
			sprintf(buf, "%s%3d. %s[%s%5d%s]%s %s%s\r\n", buf, ++found,
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->shared->vnum,
				CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
				obj->short_description, CCNRM(ch, C_NRM));
			if (strlen(buf) + 128 > MAX_STRING_LENGTH) {
				strcat(buf, "**OVERFLOW**\r\n");
				break;
			}
		}
	}
	if (found)
		page_string(ch->desc, buf);

	return (found);
}


/* create a new mobile from a prototype */
struct Creature *
read_mobile(int vnum)
{
	int found = 0;
	struct Creature *mob = NULL, *tmp_mob;

	/*  CREATE(mob, struct Creature, 1);
	   clear_char(mob); */
	CreatureList::iterator mit = mobilePrototypes.begin();
	for (; mit != mobilePrototypes.end(); ++mit) {
		tmp_mob = *mit;
		//for (tmp_mob = mob_proto; tmp_mob; tmp_mob = tmp_mob->next) {
		if (tmp_mob->mob_specials.shared->vnum >= vnum) {
			if (tmp_mob->mob_specials.shared->vnum == vnum) {
				CREATE(mob, struct Creature, 1);
				*mob = *tmp_mob;
				tmp_mob->mob_specials.shared->number++;
				tmp_mob->mob_specials.shared->loaded++;
				found = 1;
				break;
			} else {
				sprintf(buf, "Mobile (V) %d does not exist in database.",
					vnum);
				return (NULL);
			}
		}
	}

	if (!found) {
		sprintf(buf, "Mobile (V) %d does not exist in database.", vnum);
		return (NULL);
	}
	characterList.add(mob);
	if (!mob->points.max_hit) {
		mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
			mob->points.move;
	} else
		mob->points.max_hit = number(mob->points.hit, mob->points.mana);

	mob->points.hit = mob->points.max_hit;
	mob->points.mana = mob->points.max_mana;
	mob->points.move = mob->points.max_move;

	mob->player.time.birth = time(0);
	mob->player.time.death = 0;
	mob->player.time.played = 0;
	mob->player.time.logon = mob->player.time.birth;

	MOB_IDNUM(mob) = (++current_mob_idnum);

	// unapproved mobs load without money or exp
	if (MOB_UNAPPROVED(mob))
		GET_GOLD(mob) = GET_CASH(mob) = GET_EXP(mob) = 0;

	return mob;
}


/* create an object, and add it to the object list */
struct obj_data *
create_obj(void)
{
	struct obj_data *obj;

	CREATE(obj, struct obj_data, 1);

	obj->clear();

	obj->next = object_list;
	object_list = obj;

	return obj;
}


/* create a new object from a prototype */
struct obj_data *
read_object(int vnum)
{
	struct obj_data *obj = NULL, *tmp_obj = NULL;
	int found = 0;

	for (tmp_obj = obj_proto; tmp_obj; tmp_obj = tmp_obj->next) {
		if (tmp_obj->shared->vnum >= vnum) {
			if (tmp_obj->shared->vnum == vnum) {
				CREATE(obj, struct obj_data, 1);
				*obj = *tmp_obj;
				tmp_obj->shared->number++;
				found = 1;
				break;
			} else {
				sprintf(buf, "Object (V) %d does not exist in database.",
					vnum);
				return NULL;
			}
		}
	}
	if (!found) {
		sprintf(buf, "Object (V) %d does not exist in database.", vnum);
		return NULL;
	}
#ifdef TRACK_OBJS
	// temp debugging
	obj->obj_flags.tracker.lost_time = time(0);
	obj->obj_flags.tracker.string[TRACKER_STR_LEN - 1] = '\0';

#endif

	obj->next = object_list;
	object_list = obj;

	if (IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
		GET_OBJ_TIMER(obj) = 60;

	return obj;
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void
zone_update(void)
{
	struct reset_q_element *update_u, *temp;
	struct zone_data *zone;
	static int timer = 0;

	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
		/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		/* since one minute has passed, increment zone ages */

		for (zone = zone_table; zone; zone = zone->next) {

			if (!zone->num_players && !ZONE_FLAGGED(zone, ZONE_NOIDLE))
				zone->idle_time++;

			if (zone->age < zone->lifespan && zone->reset_mode)
				(zone->age)++;

			if (zone->age >= zone->lifespan && zone->age > zone->idle_time &&	/* <-- new mod here */
				zone->age < ZO_DEAD && zone->reset_mode) {
				/* enqueue zone */

				CREATE(update_u, struct reset_q_element, 1);

				update_u->zone_to_reset = zone;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else {
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone->age = ZO_DEAD;
			}
		}
	}



	/* end - one minute has passed */
	/* dequeue zones (if possible) and reset */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (update_u->zone_to_reset->reset_mode == 2 ||
			!update_u->zone_to_reset->num_players) {
			/*        is_empty(update_u->zone_to_reset)) { */
			reset_zone(update_u->zone_to_reset);
			slog("Auto zone reset: %s", update_u->zone_to_reset->name);
			/* dequeue */
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else {
				for (temp = reset_q.head; temp->next != update_u;
					temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}
			free(update_u);
			break;
		}
}

void
log_zone_error(struct zone_data *zone, struct reset_com *zonecmd,
	char *message)
{
	mudlog(LVL_IMMORT, (mini_mud) ? CMP : NRM, true,
		"SYSERR: error in zone file: %s", message);
	mudlog(LVL_IMMORT, (mini_mud) ? CMP : NRM, true,
		"SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
		zonecmd->command, zone->number, zonecmd->line);
}

#define ZONE_ERROR(message) \
{ log_zone_error(zone, zonecmd, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void
reset_zone(struct zone_data *zone)
{
	int cmd_no, last_cmd = 0, prob_override = 0;
	struct Creature *mob = NULL, *tmob = NULL;
	struct obj_data *obj = NULL, *obj_to = NULL, *tobj = NULL;
	struct reset_com *zonecmd;
	struct room_data *room;
	struct special_search_data *srch = NULL;
	PHead *p_head = NULL;
	struct shop_data *shop;
	struct Creature *vkeeper = NULL;

	SPECIAL(shop_keeper);
	extern struct shop_data *shop_index;

	// Find all the shops in this zone and reset them.
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		// Wrong zone
		if ((*cit)->in_room->zone != zone)
			continue;
		// No special
		if (!MOB_FLAGGED((*cit), MOB_SPEC))
			continue;
		// Wrong special
		if (GET_MOB_SPEC((*cit)) != shop_keeper)
			continue;
		// Run through the shop list and find the right one
		for (shop = shop_index; shop; shop = shop->next) {
			// Do they have revenue set on the shop?
			if (SHOP_REVENUE(shop) <= 0)
				continue;
			// Make sure its the right shop keeper.
			if (GET_MOB_VNUM((*cit)) != shop->keeper)
				continue;
			// Make sure the shop is in this zone;
			if (shop->vnum < (zone->number * 100) || shop->vnum > zone->top)
				continue;
			// Assume at this point that we've found the proper keeper
			// and shop pair.

			// if the vnum's gold is less than his current gold
			// add the revenue up to the vnum's gold.
			vkeeper = real_mobile_proto(shop->keeper);
			if (GET_MONEY((*cit), shop) < GET_MONEY(vkeeper, shop))
				GET_MONEY((*cit), shop) =
					MIN(GET_MONEY(vkeeper, shop), GET_MONEY((*cit),
						shop) + SHOP_REVENUE(shop));
			break;				// should break out of shop for loop.
		}
	}

	for (zonecmd = zone->cmd; zonecmd && zonecmd->command != 'S';
		zonecmd = zonecmd->next, cmd_no++) {
		// if_flag 
		// 0 == "Do regardless of previous"
		// 1 == "Do if previous succeded"
		// 2 == "Do if previous failed"
		// last_cmd
		// 1 == "Last command succeded"
		// 0 == "Last command had an error"
		//-1 == "Last command's percentage failed"

		if (zonecmd->if_flag == 1 && last_cmd != 1)
			continue;
		else if (zonecmd->if_flag == -1 && last_cmd != -1)
			continue;

		if (!prob_override && number(1, 100) > zonecmd->prob) {
			last_cmd = -1;
			continue;
		} else {
			prob_override = 0;
		}
// WARNING: FUNKY INDENTATION
		switch (zonecmd->command) {
		case '*':				/* ignore command */
			last_cmd = -1;
			break;
		case 'M':{				/* read a mobile */
				tmob = real_mobile_proto(zonecmd->arg1);
				if (tmob != NULL
					&& tmob->mob_specials.shared->number < zonecmd->arg2) {
					room = real_room(zonecmd->arg3);
					if (room) {
						mob = read_mobile(zonecmd->arg1);
					} else {
						last_cmd = 0;
						break;
					}
					if (mob) {
						char_to_room(mob, room,false);
						if (GET_MOB_LEADER(mob) > 0) {
							CreatureList::iterator it =
								mob->in_room->people.begin();
							for (; it != mob->in_room->people.end(); ++it) {
								if ((*it) != mob && IS_NPC((*it))
									&& GET_MOB_VNUM((*it)) ==
									GET_MOB_LEADER(mob)) {
									if (!circle_follow(mob, (*it))) {
										add_follower(mob, (*it));
										break;
									}
								}
							}
						}
						last_cmd = 1;
					} else {
						last_cmd = 0;
					}
				} else {
					last_cmd = 0;
				}
				break;
			}
		case 'O':				/* read an object */
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->house_count <
				zonecmd->arg2) {
				if (zonecmd->arg3 >= 0) {
					room = real_room(zonecmd->arg3);
					if (room && !ROOM_FLAGGED(room, ROOM_HOUSE)) {
						obj = read_object(zonecmd->arg1);
						if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
							SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
							GET_OBJ_TIMER(obj) = 60;
						}
					} else {
						last_cmd = 0;
						break;
					}
					if (obj) {
						obj_to_room(obj, room);
						last_cmd = 1;
					} else
						last_cmd = 0;
				} else
					last_cmd = 0;
			} else
				last_cmd = 0;
			break;

		case 'P':				/* object to object */
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->house_count <
				zonecmd->arg2) {
				obj = read_object(zonecmd->arg1);
				if (!(obj_to = get_obj_num(zonecmd->arg3))) {
					ZONE_ERROR("target obj not found");
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					extract_obj(obj);
					break;
				}
				obj_to_obj(obj, obj_to);
				last_cmd = 1;
			} else
				last_cmd = 0;
			break;

		case 'V':				/* add path to vehicle */
			last_cmd = 0;
			if (!(tobj = get_obj_num(zonecmd->arg3))) {
				ZONE_ERROR("target obj not found");
				break;
			}
			if (!(p_head = real_path_by_num(zonecmd->arg1))) {
				ZONE_ERROR("path not found");
				break;
			}
			if (add_path_to_vehicle(tobj, p_head->name))
				last_cmd = 1;
			break;

		case 'G':				/* obj_to_char */
			if (!mob) {
				if (last_cmd == 1)
					ZONE_ERROR("attempt to give obj to non-existant mob");
				break;
			}
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->house_count <
				zonecmd->arg2) {
				obj = read_object(zonecmd->arg1);
				if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
					SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
					GET_OBJ_TIMER(obj) = 60;
				}
				obj_to_char(obj, mob);
				last_cmd = 1;
			} else
				last_cmd = 0;
			break;

		case 'E':				/* object to equipment list */
			if (!mob) {
				if (last_cmd)
					ZONE_ERROR("trying to equip non-existant mob");
				break;
			}
			last_cmd = 0;
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->house_count <
				zonecmd->arg2) {
				if (zonecmd->arg3 < 0 || zonecmd->arg3 >= NUM_WEARS) {
					ZONE_ERROR("invalid equipment pos number");
				} else if (!CAN_WEAR(tobj, wear_bitvectors[zonecmd->arg3])) {
					ZONE_ERROR("invalid eq pos for obj");
				} else if (GET_EQ(mob, zonecmd->arg3)) {
					ZONE_ERROR("char already equipped in position");
				} else {
					obj = read_object(zonecmd->arg1);
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					if (equip_char(mob, obj, zonecmd->arg3, MODE_EQ)) {
						mob = NULL;
						last_cmd = 0;
					} else
						last_cmd = 1;
				}
			} else
				last_cmd = 0;
			break;
		case 'I':				/* object to equipment list */
			if (!mob) {
				if (last_cmd)
					ZONE_ERROR("trying to implant non-existant mob");
				break;
			}
			last_cmd = 0;
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->number < zonecmd->arg2) {
				if (zonecmd->arg3 < 0 || zonecmd->arg3 >= NUM_WEARS) {
					ZONE_ERROR("invalid implant pos number");
				} else if (!CAN_WEAR(tobj, wear_bitvectors[zonecmd->arg3])) {
					ZONE_ERROR("invalid implant pos for obj");
				} else if (GET_IMPLANT(mob, zonecmd->arg3)) {
					ZONE_ERROR("char already implanted in position");
				} else {
					obj = read_object(zonecmd->arg1);
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					if (equip_char(mob, obj, zonecmd->arg3, MODE_IMPLANT)) {
						mob = NULL;
						last_cmd = 0;
					} else
						last_cmd = 1;
				}
			} else
				last_cmd = 0;
			break;
		case 'W':
			if (!mob) {
				if (last_cmd)
					ZONE_ERROR("trying to assign path to non-existant mob");
				last_cmd = 0;
				break;
			}
			last_cmd = 0;
			if (!(p_head = real_path_by_num(zonecmd->arg1))) {
				ZONE_ERROR("invalid path vnum");
				break;
			}
			if (!add_path_to_mob(mob, p_head->name)) {
				ZONE_ERROR("unable to attach path to mob");
			} else
				last_cmd = 1;
			break;

		case 'R':				/* rem obj from room */
			room = real_room(zonecmd->arg2);
			if (room &&
				(obj = get_obj_in_list_num(zonecmd->arg1, room->contents)) &&
				!ROOM_FLAGGED(room, ROOM_HOUSE)) {
				obj_from_room(obj);
				extract_obj(obj);
				last_cmd = 1;
				prob_override = 1;
			} else
				last_cmd = 0;

			break;

		case 'D':				/* set state of door */
			room = real_room(zonecmd->arg1);
			if (!room || zonecmd->arg2 < 0 || zonecmd->arg2 >= NUM_OF_DIRS ||
				(room->dir_option[zonecmd->arg2] == NULL)) {
				ZONE_ERROR("door does not exist");
			} else {
				if (IS_SET(zonecmd->arg3, DOOR_OPEN)) {
					REMOVE_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_LOCKED);
					REMOVE_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_CLOSED);
				}
				if (IS_SET(zonecmd->arg3, DOOR_CLOSED)) {
					SET_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_CLOSED);
					REMOVE_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_LOCKED);
				}
				if (IS_SET(zonecmd->arg3, DOOR_LOCKED)) {
					SET_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_LOCKED);
					SET_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_CLOSED);
				}
				if (IS_SET(zonecmd->arg3, DOOR_HIDDEN)) {
					SET_BIT(room->dir_option[zonecmd->arg2]->exit_info,
						EX_HIDDEN);
				}
				last_cmd = 1;
			}
			break;

		default:
			ZONE_ERROR("unknown cmd in reset table! cmd disabled");
			zonecmd->command = '*';
			break;
		}
	}

	zone->age = 0;

	/* reset all search status */
	for (room = zone->world; room; room = room->next)
		for (srch = room->search; srch; srch = srch->next)
			REMOVE_BIT(srch->flags, SRCH_TRIPPED);

}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
/*int 
  is_empty(struct zone_data *zone)
  {
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
  if (!i->connected)
  if (i->character->in_room->zone == zone)
  return 0;

  return 1;
  }
*/

/*************************************************************************
*  stuff related to the save/load player system                                 *
*********************************************************************** */

long
get_id_by_name(const char *name)
{
	int i;

	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!strcasecmp((player_table + i)->name, arg))
			return ((player_table + i)->id);

	return -1;
}


char *
get_name_by_id(long id)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if ((player_table + i)->id == id)
			return ((player_table + i)->name);

	return NULL;
}

void
export_player_table( Creature *ch ) 
{
	char_file_u file_e;
	Creature *vict;
	CREATE(vict, Creature, 1);
	clear_char(vict);

	int i;
	for( i = 0; i <= top_of_p_table; i++ ) {
		load_char( player_table[i].name, &file_e );
		store_to_char( &file_e, vict );
		vict->saveToXML();
		clear_char(vict);
	}
	send_to_char(ch, "Exported %d files.\r\n", i);
}

// Load a char, pfile index if loaded, -1 if not
int
load_char(char *name, struct char_file_u *char_element)
{

	int player_i;
	int find_name(char *name);

	if (!name)
		return -1;

	if ((player_i = find_name(name)) >= 0) {
		fseek(player_fl, (long)(player_i * sizeof(struct char_file_u)),
			SEEK_SET);
		fread(char_element, sizeof(struct char_file_u), 1, player_fl);
		return (player_i);
	} else
		return (-1);
}

void
save_aliases(struct Creature *ch)
{
	struct alias_data *a;
	FILE *file_handle = NULL;

	strcpy(buf, GET_NAME(ch));
	buf[0] = toupper(buf[0]);
	if (buf[0] < 'F')
		sprintf(buf, "plralias/A-E/%s.al", GET_NAME(ch));
	else if (buf[0] < 'K')
		sprintf(buf, "plralias/F-J/%s.al", GET_NAME(ch));
	else if (buf[0] < 'P')
		sprintf(buf, "plralias/K-O/%s.al", GET_NAME(ch));
	else if (buf[0] < 'U')
		sprintf(buf, "plralias/P-T/%s.al", GET_NAME(ch));
	else if (buf[0] <= 'Z')
		sprintf(buf, "plralias/U-Z/%s.al", GET_NAME(ch));
	else
		sprintf(buf, "plralias/ZZZ/%s.al", GET_NAME(ch));

	file_handle = fopen(buf, "w");
	if (!file_handle) {
		sprintf(buf2, "SYSERR: Unable to open alias file %s for write.", buf);
		slog(buf2);
		return;
	}
	for (a = ch->player_specials->aliases; a; a = a->next) {
		sprintf(buf, "%d %s %s\n", a->type, a->alias, a->replacement);
		fputs(buf, file_handle);
	}
	fputs("S\n", file_handle);
	fclose(file_handle);
}

void
read_alias(struct Creature *ch)
{
	struct alias_data *a;
	char buf[MAX_STRING_LENGTH];
	FILE *file_handle = NULL;
	char *t, *a_, *r;

	strcpy(buf, GET_NAME(ch));
	buf[0] = toupper(buf[0]);
	if (buf[0] < 'F')
		sprintf(buf, "plralias/A-E/%s.al", GET_NAME(ch));
	else if (buf[0] < 'K')
		sprintf(buf, "plralias/F-J/%s.al", GET_NAME(ch));
	else if (buf[0] < 'P')
		sprintf(buf, "plralias/K-O/%s.al", GET_NAME(ch));
	else if (buf[0] < 'U')
		sprintf(buf, "plralias/P-T/%s.al", GET_NAME(ch));
	else if (buf[0] <= 'Z')
		sprintf(buf, "plralias/U-Z/%s.al", GET_NAME(ch));
	else
		sprintf(buf, "plralias/ZZZ/%s.al", GET_NAME(ch));

	file_handle = fopen(buf, "r");
	if (!file_handle) {
		if (errno && errno != 2) {
			sprintf(buf2, "SYSERR: Unable to open alias file %s for read.",
				buf);
			slog(buf2);
		}
		return;
	}

	while (!feof(file_handle)) {
		if (!fgets(buf, MAX_INPUT_LENGTH, file_handle))
			break;
		if (buf[0] == 'S')
			break;

		t = strtok(buf, " ");
		a_ = strtok(NULL, " ");
		r = strtok(NULL, "\n");

		if ((t != NULL) && (isdigit(t[0])) && (a_ != NULL) && (r != NULL)) {
			CREATE(a, struct alias_data, 1);
			a->type = atoi(t);
			a->alias = strdup(a_);
			a->replacement = strdup(r);
			add_alias(ch, a);
		}
	}
	if (file_handle != NULL) {
		fclose(file_handle);
	}
}

/* write the vital data of a player to the player file */
void
save_char(struct Creature *ch, struct room_data *load_room)
{
	struct char_file_u st;
	if (IS_NPC(ch) || !ch->desc || GET_PFILEPOS(ch) < 0)
		return;

	ch->player.time.played += time(0) - ch->player.time.logon;
	ch->player.time.logon = time(0);

	char_to_store(ch, &st);

	// This must be updated here because the last host isn't stored anywhere
	// in the creature record
	strncpy(st.host, ch->desc->host, HOST_LENGTH);
	st.host[HOST_LENGTH] = '\0';

	if (STATE(ch->desc) == CON_PLAYING)
		save_aliases(ch);

	if (load_room) {
		if (PLR_FLAGGED(ch, PLR_LOADROOM) &&
			GET_LOADROOM(ch) != load_room->number) {
			REMOVE_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
			REMOVE_BIT(st.char_specials_saved.act, PLR_LOADROOM);
			GET_HOLD_LOADROOM(ch) = GET_LOADROOM(ch);
			st.player_specials_saved.hold_load_room = GET_LOADROOM(ch);
		}
		GET_LOADROOM(ch) = load_room->number;
		st.player_specials_saved.load_room = load_room->number;
	} else if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
		GET_LOADROOM(ch) = -1;
		st.player_specials_saved.load_room = -1;
	}

	fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
	fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
    ch->saveToXML();
}



/* copy data from the file structure to a char struct */
void
store_to_char(struct char_file_u *st, struct Creature *ch)
{
	int i;

	/* to save memory, only PC's -- not MOB's -- have player_specials */
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);

	GET_SEX(ch) = st->sex;
	GET_CLASS(ch) = st->char_class;
	GET_REMORT_CLASS(ch) = st->remort_char_class;
	GET_RACE(ch) = st->race;
	GET_LEVEL(ch) = st->level;

	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;
	set_title(ch, st->title);
	ch->player.description = str_dup(st->description);
	ch->player.hometown = st->hometown;
	ch->player.time.birth = st->birth;
	ch->player.time.death = st->death;
	ch->player.time.played = st->played;
	ch->player.time.logon = time(0);

	ch->player.weight = st->weight;
	ch->player.height = st->height;

	ch->real_abils = st->abilities;
	ch->aff_abils = st->abilities;
	ch->points = st->points;
	ch->char_specials.saved = st->char_specials_saved;

	if (IS_SET(ch->char_specials.saved.act, MOB_ISNPC)) {
		REMOVE_BIT(ch->char_specials.saved.act, MOB_ISNPC);
		slog("SYSERR: store_to_char %s loaded with MOB_ISNPC bit set!",
			GET_NAME(ch));
	}

	ch->player_specials->saved = st->player_specials_saved;

	if (ch->points.max_mana < 100)
		ch->points.max_mana = 100;

	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;
	ch->char_specials.worn_weight = 0;
	ch->points.armor = 100;
	ch->points.hitroll = 0;
	ch->points.damroll = 0;
	ch->setSpeed(0);

	st->name[MAX_NAME_LENGTH] = '\0';
	CREATE(ch->player.name, char, strlen(st->name) + 1);
	strcpy(ch->player.name, st->name);
	st->pwd[MAX_PWD_LENGTH] = '\0';
	strcpy(ch->player.passwd, st->pwd);

	if (*st->poofin) {
		CREATE(POOFIN(ch), char, strlen(st->poofin) + 1);
		strcpy(POOFIN(ch), st->poofin);
	} else
		POOFIN(ch) = NULL;

	if (*st->poofout) {
		CREATE(POOFOUT(ch), char, strlen(st->poofout) + 1);
		strcpy(POOFOUT(ch), st->poofout);
	} else
		POOFOUT(ch) = NULL;

	// reset all imprint rooms
	for (i = 0; i < MAX_IMPRINT_ROOMS; i++)
		GET_IMPRINT_ROOM(ch, i) = -1;

	/* Add all spell effects */
	for (i = 0; i < MAX_AFFECT; i++) {
		if (st->affected[i].type)
			affect_to_char(ch, &st->affected[i]);
	}
	/*  ch->in_room = real_room(GET_LOADROOM(ch)); */

/*   affect_total(ch); also - unnecessary?? */

	/*
	 * If you're not poisioned and you've been away for more than an hour,
	 * we'll set your HMV back to full
	 */

	if (!IS_POISONED(ch) &&
		(((long)(time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(ch) = GET_MAX_HIT(ch);
		GET_MOVE(ch) = GET_MAX_MOVE(ch);
		GET_MANA(ch) = GET_MAX_MANA(ch);
	}

	read_alias(ch);
}								/* store_to_char */



/* copy vital data from a players char-structure to the file structure */
void
char_to_store(struct Creature *ch, struct char_file_u *st)
{
	int i;
	struct affected_type *af;
	struct obj_data *char_eq[NUM_WEARS], *char_implants[NUM_WEARS];
	struct room_data *room = ch->in_room;
	sh_int hit = GET_HIT(ch), mana = GET_MANA(ch), move = GET_MOVE(ch);
	int dead = 0;

	memset(st, 0x00, sizeof(struct char_file_u));

	/* Unaffect everything a character can be affected by */

	for (i = 0; i < NUM_WEARS; i++) {
		if (ch->equipment[i])
			char_eq[i] = unequip_char(ch, i, MODE_EQ, true);
		else
			char_eq[i] = NULL;

		if (ch->implants[i])
			char_implants[i] = unequip_char(ch, i, MODE_IMPLANT, true);
		else
			char_implants[i] = NULL;
	}

	for (af = ch->affected, i = 0; i < MAX_AFFECT; i++) {
		if (af) {
			//      st->affected[i] = *af;  buggy circle shit, read it and weep
			memcpy(st->affected + i, af, sizeof(struct affected_type));
			st->affected[i].next = NULL;
			af = af->next;
		} else {
			st->affected[i].type = 0;	/* Zero signifies not used */
			st->affected[i].duration = 0;
			st->affected[i].modifier = 0;
			st->affected[i].location = 0;
			st->affected[i].bitvector = 0;
			st->affected[i].level = 0;
			st->affected[i].is_instant = 0;
			st->affected[i].aff_index = 0;
			st->affected[i].next = NULL;
		}
	}

	/*
	 * remove the affections so that the raw values are stored; otherwise the
	 * effects are doubled when the char logs back in.
	 */

	while (ch->affected)
		affect_remove(ch, ch->affected);

	if ((i >= MAX_AFFECT) && af && af->next)
		slog("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

	ch->aff_abils = ch->real_abils;

	st->birth = ch->player.time.birth;
	st->death = ch->player.time.death;
	st->played = ch->player.time.played;
	st->last_logon = ch->player.time.logon;

	ch->player.time.played = st->played;
	ch->player.time.logon = time(0);

	st->hometown = ch->player.hometown;
	st->weight = GET_WEIGHT(ch);
	st->height = GET_HEIGHT(ch);
	st->sex = GET_SEX(ch);
	st->char_class = GET_CLASS(ch);
	st->remort_char_class = GET_REMORT_CLASS(ch);
	st->race = GET_RACE(ch);
	st->level = GET_LEVEL(ch);
	st->abilities = ch->real_abils;
	st->points = ch->points;
	st->char_specials_saved = ch->char_specials.saved;
	st->char_specials_saved.affected_by = 0;
	st->char_specials_saved.affected2_by = 0;
	st->char_specials_saved.affected3_by = 0;
	st->player_specials_saved = ch->player_specials->saved;

	st->points.armor = 100;
	st->points.hitroll = 0;
	st->points.damroll = 0;

	if (GET_TITLE(ch)) {
		strncpy(st->title, GET_TITLE(ch),MAX_TITLE_LENGTH);
		st->title[MAX_TITLE_LENGTH] = '\0'; // allocated size + 1
	} else {
		*st->title = '\0';
	}

	if (ch->player.description) {
		strncpy(st->description, ch->player.description, MAX_CHAR_DESC);
		st->description[MAX_CHAR_DESC] = '\0'; // allocated size +1 
	} else {
		*st->description = '\0';
	}

	if (POOFIN(ch)) {
		strncpy( st->poofin, POOFIN(ch), MAX_POOF_LENGTH );
		st->poofin[MAX_POOF_LENGTH - 1] = '\0';
	} else {
		*st->poofin = '\0';
	}

	if (POOFOUT(ch)) {
		strncpy(st->poofout, POOFOUT(ch), MAX_POOF_LENGTH);
		st->poofout[MAX_POOF_LENGTH - 1] = '\0';
	} else {
		*st->poofout = '\0';
	}

	strncpy(st->name, GET_NAME(ch), MAX_NAME_LENGTH);
	st->name[MAX_NAME_LENGTH] = '\0';
	strncpy(st->pwd, GET_PASSWD(ch), MAX_PWD_LENGTH);
	st->pwd[MAX_PWD_LENGTH] = '\0';


	/* add spell and eq affections back in now */
	for (i = 0; i < MAX_AFFECT; i++) {
		if (st->affected[i].type)
			affect_to_char(ch, &st->affected[i]);
	}

	for (i = 0, dead = 0; i < NUM_WEARS; i++) {
		if (char_eq[i]) {
			if (dead && room)
				obj_to_room(char_eq[i], room);
			else
				dead = equip_char(ch, char_eq[i], i, MODE_EQ);
		}
		if (char_implants[i]) {
			if (dead && room)
				obj_to_room(char_eq[i], room);
			else
				dead = equip_char(ch, char_implants[i], i, MODE_IMPLANT);
		}
	}
	if (!dead)
		dead = check_eq_align(ch);

	if (dead)
		return;

	GET_HIT(ch) = MIN(GET_MAX_HIT(ch), hit);
	GET_MANA(ch) = MIN(GET_MAX_MANA(ch), mana);
	GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), move);

}								/* Char to store */

/* create a new entry in the in-memory index table for the player file */
int
create_entry(char *name)
{
	int i, len = strlen(name);

	if (top_of_p_table == -1) {
		CREATE(player_table, struct player_index_element, 1);
		top_of_p_table = 0;
		return 0;
	}

	if (!(player_table = (struct player_index_element *)
			realloc(player_table, sizeof(struct player_index_element) *
				(++top_of_p_table + 1)))) {
		perror("create entry");
		safe_exit(1);
	}
	CREATE(player_table[top_of_p_table].name, char, len + 1);

	/* copy lowercase equivalent of name to table field */
	for (i = 0;
		(*(player_table[top_of_p_table].name + i) = tolower(*(name + i))); i++);

	return (top_of_p_table);
}



/************************************************************************
*  funcs of a (more or less) general utility nature                        *
********************************************************************** */


/* read and allocate space for a '~'-terminated string from a given file */
char *
fread_string(FILE * fl, char *error)
{
	char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
	register char *point;
	unsigned int length = 0, templength = 0;
	bool done = false;

	*buf = '\0';

	do {
		if (!fgets(tmp, 512, fl)) {
			slog("SYSERR: fread_string: format error at or near %s\n",
				error);
			safe_exit(1);
		}
		/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL) {
			*point = '\0';
			done = true;
		} else {
			point = tmp + strlen(tmp) - 1;
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			slog("SYSERR: fread_string: string too large (db.c)");
			safe_exit(1);
		} else {
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);

	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0) {
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	} else
		rslt = NULL;

	return rslt;
}

/* read and copy for a '~'-terminated string from a given file
   copying it into str, returning 1 on success or 0 on failure  */
int
pread_string(FILE * fl, char *str, char *error)
{
	char tmp[512];
	register char *point;
	unsigned int length = 0, templength = 0;
	bool done = false;

	*str = '\0';

	do {
		if (!fgets(tmp, 512, fl)) {
			return 0;
		}
		/* lines that start with # are comments  */
		if (*tmp == '#')
			continue;

		/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL) {
			*point = '\0';
			done = true;
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			slog("SYSERR: pread_string: string too large: %s\n", error);
			return 0;
		} else {
			strcat(str + length, tmp);
			length += templength;
		}
	} while (!done);

	return 1;
}

//
// release memory allocated for a char struct
// char must be remove from the world _FIRST_
//

void
free_char(struct Creature *ch)
{

	int i;
	struct Creature *tmp_mob;
	struct alias_data *a;

	void free_alias(struct alias_data *a);

	//
	// first make sure the char is no longer in the world
	//

	if (ch->in_room != NULL || ch->carrying != NULL ||
		ch->getFighting() != NULL || ch->followers != NULL
		|| ch->master != NULL) {
		slog("SYSERR: free_char() attempted to free a char who is still connected to the world.");
		raise(SIGSEGV);
	}
	//
	// first remove and free all alieases
	//

	while ((a = GET_ALIASES(ch)) != NULL) {
		GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
		free_alias(a);
	}

	//
	// now remove all affects
	//

	while (ch->affected)
		affect_remove(ch, ch->affected);


	//
	// free mob strings:
	// free strings only if the string is not pointing at proto
	//

	if ((i = GET_MOB_VNUM(ch)) > -1) {

		tmp_mob = real_mobile_proto(GET_MOB_VNUM(ch));

		if (ch->player.name && ch->player.name != tmp_mob->player.name)
			free(ch->player.name);
		if (ch->player.title && ch->player.title != tmp_mob->player.title)
			free(ch->player.title);
		if (ch->player.short_descr &&
			ch->player.short_descr != tmp_mob->player.short_descr)
			free(ch->player.short_descr);
		if (ch->player.long_descr &&
			ch->player.long_descr != tmp_mob->player.long_descr)
			free(ch->player.long_descr);
		if (ch->player.description &&
			ch->player.description != tmp_mob->player.description)
			free(ch->player.description);
		if (ch->mob_specials.mug)
			free(ch->mob_specials.mug);

		ch->mob_specials.mug = 0;
	}
	//
	// otherwise this is a player, so free all
	//

	else {

		if (GET_NAME(ch))
			free(GET_NAME(ch));
		if (ch->player.title)
			free(ch->player.title);
		if (ch->player.short_descr)
			free(ch->player.short_descr);
		if (ch->player.long_descr)
			free(ch->player.long_descr);
		if (ch->player.description)
			free(ch->player.description);
	}

	//
	// null all the fields so if (heaven forbid!) the free'd ch is used again, 
	// it will hopefully cause a segv
	//

	ch->player.name = 0;
	ch->player.title = 0;
	ch->player.short_descr = 0;
	ch->player.long_descr = 0;
	ch->player.description = 0;

	//
	// remove player_specials:
	// - poofin
	// - poofout
	//

	if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
		if (ch->player_specials->poofin)
			free(ch->player_specials->poofin);
		if (ch->player_specials->poofout)
			free(ch->player_specials->poofout);

		free(ch->player_specials);

		if (IS_NPC(ch)) {
			slog("SYSERR: Mob had player_specials allocated!");
			raise(SIGSEGV);
		}
	}
	//
	// null the free'd player_specials field
	//

	ch->player_specials = 0;

	free(ch);
}

/* release memory allocated for an obj struct */
void
free_obj(struct obj_data *obj)
{
	struct extra_descr_data *this_desc, *next_one;

	if ((GET_OBJ_VNUM(obj)) == -1 || !obj->shared->proto) {
		if (obj->name) {
			free(obj->name);
			obj->name = NULL;
		}
		if (obj->description) {
			free(obj->description);
			obj->description = NULL;
		}
		if (obj->short_description) {
			free(obj->short_description);
			obj->short_description = NULL;
		}
		if (obj->action_description) {
			free(obj->action_description);
			obj->action_description = NULL;
		}
		if (obj->ex_description) {
			for (this_desc = obj->ex_description; this_desc;
				this_desc = next_one) {
				next_one = this_desc->next;
				if (this_desc->keyword)
					free(this_desc->keyword);
				if (this_desc->description)
					free(this_desc->description);
				free(this_desc);
			}
			obj->ex_description = NULL;
		}
	} else {
		if (obj->name && obj->name != obj->shared->proto->name) {
			free(obj->name);
			obj->name = NULL;
		}
		if (obj->description &&
			obj->description != obj->shared->proto->description) {
			free(obj->description);
			obj->description = NULL;
		}
		if (obj->short_description &&
			obj->short_description != obj->shared->proto->short_description) {
			free(obj->short_description);
			obj->short_description = NULL;
		}
		if (obj->action_description &&
			obj->action_description !=
			obj->shared->proto->action_description) {
			free(obj->action_description);
			obj->action_description = NULL;
		}
		if (obj->ex_description &&
			obj->ex_description != obj->shared->proto->ex_description) {
			for (this_desc = obj->ex_description; this_desc;
				this_desc = next_one) {
				next_one = this_desc->next;
				if (this_desc->keyword)
					free(this_desc->keyword);
				if (this_desc->description)
					free(this_desc->description);
				free(this_desc);
			}
			obj->ex_description = NULL;
		}
	}
	free(obj);
}


/* read contets of a text file, alloc space, point buf to it */
int
file_to_string_alloc(char *name, char **buf)
{
	char temp[MAX_STRING_LENGTH];


	if (*buf)
		free(*buf);

	if (file_to_string(name, temp) < 0) {
		*buf = NULL;
		return -1;
	} else {
		*buf = str_dup(temp);
		return 0;
	}
}


#define READ_SIZE 128

/* read contents of a text file, and place in buf */
int
file_to_string(char *name, char *buf)
{
	FILE *fl;
	char tmp[129];

	*buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		sprintf(tmp, "Error reading %s", name);
		perror(tmp);
		return (-1);
	}
	do {
		fgets(tmp, READ_SIZE, fl);
		tmp[strlen(tmp) - 1] = '\0';	/* remove trailing \n */
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
				slog("SYSERR: fl->strng: string too big (db.c, file_to_string)");
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}




/* clear some of the the working variables of a char */
void
reset_char(struct Creature *ch)
{
	int i;

	for (i = 0; i < NUM_WEARS; i++) {
		ch->equipment[i] = NULL;
		ch->implants[i] = NULL;
	}

	ch->followers = NULL;
	ch->master = NULL;
	/* ch->in_room = NOWHERE; Used for start in room */
	ch->carrying = NULL;
	ch->setFighting(NULL);
	ch->char_specials.position = POS_STANDING;
	if (ch->mob_specials.shared)
		ch->mob_specials.shared->default_pos = POS_STANDING;
	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;
	ch->player_specials->olc_obj = NULL;
	ch->player_specials->olc_mob = NULL;
	ch->player_specials->olc_ticl = NULL;

	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) <= 0)
		GET_MANA(ch) = 1;

	GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void
clear_char(struct Creature *ch)
{
	memset((char *)ch, 0, sizeof(struct Creature));

	ch->in_room = NULL;
	GET_PFILEPOS(ch) = -1;

	if (ch->player_specials) {
		GET_WAS_IN(ch) = NULL;
	}
	ch->setPosition(POS_STANDING);
	GET_REMORT_CLASS(ch) = -1;

	GET_AC(ch) = 100;			/* Basic Armor */
	if (ch->points.max_mana < 100)
		ch->points.max_mana = 100;
}



/* initialize a new character only if char_class is set */
void
init_char(struct Creature *ch)
{
	int i;

	/* create a player_special structure */
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);

	/* *** if this is our first player --- he be God *** */

	if (top_of_p_table == 0) {
		GET_EXP(ch) = 160000000;
		GET_LEVEL(ch) = LVL_GRIMP;

		ch->points.max_hit = 666;
		ch->points.max_mana = 555;
		ch->points.max_move = 444;
	}
	set_title(ch, NULL);

	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;
	ch->player.description = NULL;

	ch->player.hometown = GET_HOME(ch);

	ch->player.time.birth = time(0);
	ch->player.time.death = 0;
	ch->player.time.played = 0;
	ch->player.time.logon = time(0);

	for (i = 0; i < MAX_SKILLS; i++)
		ch->player_specials->saved.skills[i] = 0;

	for (i = 0; i < MAX_WEAPON_SPEC; i++) {
		ch->player_specials->saved.weap_spec[i].vnum = -1;
		ch->player_specials->saved.weap_spec[i].level = 0;
	}
	ch->player_specials->saved.quest_points = 0;
	ch->player_specials->saved.quest_id = 0;
	ch->player_specials->saved.qlog_level = 0;

	GET_REMORT_CLASS(ch) = -1;
	/* make favors for sex ... and race */
	if (ch->player.sex == SEX_MALE) {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(110, 160) + GET_STR(ch);
			ch->player.height = number(160, 200) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(120, 160) + GET_STR(ch);
			ch->player.height = number(100, 125) + (GET_WEIGHT(ch) >> 4);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(140, 165) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(120, 200) + (GET_WEIGHT(ch) >> 4);
		} else if (GET_RACE(ch) == RACE_MINOTAUR) {
			ch->player.weight = number(200, 360) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		} else {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		}
	} else {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(80, 120) + GET_STR(ch);
			ch->player.height = number(160, 190) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(100, 140) + GET_STR(ch);
			ch->player.height = number(90, 115) + (GET_WEIGHT(ch) >> 4);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(90, 130) + GET_STR(ch);
			ch->player.height = number(120, 155) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(110, 170) + GET_STR(ch);
			ch->player.height = number(110, 190) + (GET_WEIGHT(ch) >> 3);
		} else {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) >> 3);
		}
	}

	ch->points.max_mana = 100;
	ch->points.mana = GET_MAX_MANA(ch);
	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->points.armor = 100;

	PRF_FLAGS(ch) = PRF_COMPACT | PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE |
		PRF_AUTOEXIT | PRF_COLOR_1;
	PRF2_FLAGS(ch) = PRF2_AUTO_DIAGNOSE | PRF2_AUTOPROMPT | PRF2_DISPALIGN;

	player_table[GET_PFILEPOS(ch)].id = GET_IDNUM(ch) = ++top_idnum;

	for (i = 1; i <= MAX_SKILLS; i++) {
		if (GET_LEVEL(ch) < LVL_GRIMP) {
			SET_SKILL(ch, i, 0);
		} else {
			SET_SKILL(ch, i, 100);
		}
	}

	GET_ROWS(ch) = 22;			/* default page length */
	GET_COLS(ch) = -1;

	ch->char_specials.saved.affected_by = 0;
	ch->char_specials.saved.affected2_by = 0;
	ch->char_specials.saved.affected3_by = 0;

	for (i = 0; i < 5; i++)
		GET_SAVE(ch, i) = 0;

	for (i = 0; i < 3; i++)
		GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 24);
	GET_COND(ch, DRUNK) = 0;

	POOFIN(ch) = NULL;
	POOFOUT(ch) = NULL;
}



/* returns the real number of the room with given vnum number */
struct room_data *
real_room(int vnum)
{
	struct room_data *room;
	struct zone_data *zone;
	int num = (vnum / 10);

	for (zone = zone_table; zone; zone = zone->next) {

		if (num >= zone->number && vnum <= zone->top) {
			for (room = zone->world; room; room = room->next) {

				if (room->number == vnum)
					return (room);
			}
			return NULL;
		}
	}

	return (NULL);
}


struct zone_data *
real_zone(int number)
{
	struct zone_data *zone;

	for (zone = zone_table; zone; zone = zone->next) {
		if (zone->number >= number) {
			if (zone->number == number)
				return (zone);
			else
				return NULL;
		}
	}
	return (NULL);
}

struct Creature *
real_mobile_proto(int vnum)
{
	struct Creature *mobile = NULL;

	CreatureList::iterator mit = mobilePrototypes.begin();
	for (; mit != mobilePrototypes.end(); ++mit) {
		mobile = *mit;
		if (mobile->mob_specials.shared->vnum >= vnum) {
			if (mobile->mob_specials.shared->vnum == vnum)
				return (mobile);
			else
				return NULL;
		}
	}
	return (NULL);
}

class CIScript *
real_iscript(int vnum)
{
	list <CIScript *>::iterator si;

	for (si = scriptList.begin(); si != scriptList.end(); si++) {
		if ((*si)->getVnum() >= vnum) {
			if ((*si)->getVnum() == vnum)
				return *si;
			else
				return NULL;
		}
	}
	return NULL;
}

struct obj_data *
real_object_proto(int vnum)
{
	struct obj_data *object = NULL;

	for (object = obj_proto; object; object = object->next)
		if (object->shared->vnum >= vnum) {
			if (object->shared->vnum == vnum)
				return (object);
			else
				return NULL;
		}
	return (NULL);
}

#include <dirent.h>
void
update_alias_dirs(void)
{
	DIR *dir;
	struct dirent *dirp;
	char dlist[7][4] = { "A-E", "F-J", "K-O", "P-T", "U-Z", "ZZZ" };
	char name[1024], name2[1024];
	int i;

	for (i = 0; i < 6; i++) {
		sprintf(buf, "plralias/%s", dlist[i]);
		if (!(dir = opendir(buf))) {
			slog("SYSERR: Incorrect plralias directory structure.");
			safe_exit(1);
		}
		while ((dirp = readdir(dir))) {
			if ((strlen(dirp->d_name) > 3) &&
				!strcmp(dirp->d_name + strlen(dirp->d_name) - 3, ".al")) {
				strncpy(name, dirp->d_name, strlen(dirp->d_name) - 3);
				name[strlen(dirp->d_name) - 3] = 0;
				if (get_id_by_name(name) < 0) {
					sprintf(name2, "%s/%s", buf, dirp->d_name);
					unlink(name2);
					sprintf(buf2, "    Deleting %s's alias file.", name);
					slog(buf2);
				}
			}
		}
		closedir(dir);
	}
}

void
purge_trails(struct Creature *ch)
{
	struct room_trail_data *trl;
	struct room_data *rm;
	struct zone_data *zn;
	int i = 0;

	for (zn = zone_table; zn; zn = zn->next) {
		for (rm = zn->world; rm; rm = rm->next) {
			while ((trl = rm->trail)) {
				rm->trail = trl->next;
				i++;
				if (trl->name)
					free(trl->name);
				free(trl);
			}
		}
	}
	send_to_char(ch, "%d trails removed from the world.\r\n", i);
}

int
zone_number(int nr)
{
	struct zone_data *zone;
	for (zone = zone_table; zone; zone = zone->next)
		if (zone->number * 100 <= nr && zone->top >= nr)
			return (zone->number);
	return -1;
}

struct room_data *
where_obj(struct obj_data *obj)
{
	if (obj->in_room)
		return (obj->in_room);
	if (obj->in_obj)
		return (where_obj(obj->in_obj));
	if (obj->carried_by)
		return (obj->carried_by->in_room);
	if (obj->worn_by)
		return (obj->worn_by->in_room);
	return NULL;
}

struct Creature *
obj_owner(struct obj_data *obj)
{
	if (obj->carried_by)
		return (obj->carried_by);
	if (obj->worn_by)
		return (obj->worn_by);
	if (obj->in_obj)
		return (obj_owner(obj->in_obj));
	return NULL;
}

char*
get_player_file_path( long id ) 
{
    return tmp_sprintf( "players/%0ld/%ld.dat", (id % 10), id );
}

char*
get_equipment_file_path( long id ) 
{
    return tmp_sprintf( "equipment/%0ld/%ld.dat", (id % 10), id );
}


#undef __db_c__
