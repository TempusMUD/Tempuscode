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

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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
#include "help.h"
#include "tmpstr.h"
#include "player_table.h"
#include "account.h"
#include "specs.h"
#include "language.h"
#include "prog.h"
#include "mobile_map.h"
#include "object_map.h"

#define ZONE_ERROR(message) \
{ zerrlog(zone, "%s (cmd %c, num %d)", message, zonecmd->command, zonecmd->line); last_cmd = 0; }

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
map<int,room_data*> rooms;

struct zone_data *zone_table;	/* zone table                         */
int top_of_zone_table = 0;		/* top element of zone tab         */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages  */
extern HelpCollection *Help;

int no_plrtext = 0;				/* player text disabled?         */

int no_mail = 0;				/* mail disabled?                 */
int mini_mud = 0;				/* mini-mud mode?                 */
int mud_moved = 0;
int no_rent_check = 0;			/* skip rent check on boot?         */
time_t boot_time = 0;			/* time of mud boot                 */
time_t last_sunday_time = 0;	/* time of last sunday, for qp regen */
int restrict = 0;				/* level of game restriction         */
int olc_lock = 0;
int no_initial_zreset = 0;
int quest_status = 0;
int lunar_day = 0;
long top_unique_id = 0;
bool unique_id_changed = true;

struct room_data *r_mortal_start_room;	/* rnum of mortal start room   */
struct room_data *r_electro_start_room;	/* Electro Centralis start room  */
struct room_data *r_immort_start_room;	/* rnum of immort start room   */
struct room_data *r_frozen_start_room;	/* rnum of frozen start room   */
struct room_data *r_new_thalos_start_room;
struct room_data *r_kromguard_start_room;
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
int *wld_index = NULL;			/* world index                   */

char *credits = NULL;			/* game credits                         */
char *motd = NULL;				/* message of the day - mortals */
char *ansi_motd = NULL;			/* message of the day - mortals */
char *imotd = NULL;				/* message of the day - immorts */
char *ansi_imotd = NULL;		/* message of the day - immorts */
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

struct time_info_data time_info;	/* the infomation about the time    */
/*struct weather_data weather_info;        the infomation about the weather */
extern struct player_special_data dummy_mob;	/* dummy spec area for mobs         */
extern bool production_mode;
struct reset_q_type reset_q;	/* queue of zones to be reset         */
PGconn *sql_cxn = NULL;
struct sql_query_data *sql_query_list = NULL;

/* local functions */
void setup_dir(FILE * fl, struct room_data *room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_room(FILE * fl, int vnum_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void Load_paths(void);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_artisans(void);
void boot_dynamic_text(void);
void boot_tongues(void);

/*int is_empty(struct zone_data *zone); */
void reset_zone(struct zone_data *zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void compile_all_progs(void);
void reset_time(void);
void reset_zone_weather(void);
void set_local_time(struct zone_data *zone, struct time_info_data *local_time);
void purge_trails(struct Creature *ch);
void build_old_player_index();
void randomize_object(struct obj_data *obj);

/* external functions */
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void boot_spells(void);
void boot_social_messages(void);
void free_socials(void);
void sort_commands(void);
void sort_spells(void);
void sort_skills(void);
void load_banned(void);
void Read_Invalid_List(void);
void Read_Nasty_List(void);
void add_alias(struct Creature *ch, struct alias_data *a);
void boot_clans(void);
void add_follower(struct Creature *ch, struct Creature *leader);
void xml_reload( Creature *ch );
void load_bounty_data(void);
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
		free_socials();
		boot_social_messages();
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
	} else if (!str_cmp(arg, "socials")) {
		free_socials();
		boot_social_messages();
	} else if (!str_cmp(arg, "spells")) {
        boot_spells();
    } else if( !str_cmp(arg, "xml") ) {
        xml_reload(ch);
        return;
	} else {
		send_to_char(ch, "Unknown reboot option.\r\n");
        send_to_char(ch, "Options: all    *         credits     motd     imotd      info\r\n");
        send_to_char(ch, "         areas  olc_guide quest_guide handbook background paths\r\n");
        send_to_char(ch, "         trails timewarps xml      socials\r\n");
		return;
	}
	send_to_char(ch, OK);
	mudlog(GET_INVIS_LVL(ch), NRM, true,
		"%s has reloaded %s file", GET_NAME(ch), arg);
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

	slog("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	slog("Renumbering zone table.");
	renum_zone_table();

	/* for quad damage bamfing */
	if (!(default_quad_zone = real_zone(25)))
		default_quad_zone = zone_table;
}


/* body of the booting system */
void
boot_db(void)
{
	struct zone_data *zone;
	PGresult *res;

	slog("Boot db -- BEGIN.");

	slog("Resetting the game time:");
	reset_time();

	slog("Connecting to postgres.");
	if (production_mode)
		sql_cxn = PQconnectdb("user=realm dbname=tempus");
	else
		sql_cxn = PQconnectdb("hostaddr=206.41.250.2 user=realm dbname=devtempus password=tarrasque");
	if (!sql_cxn) {
		slog("Couldn't allocate postgres connection!");
		safe_exit(1);
	}
	if (PQstatus(sql_cxn) != CONNECTION_OK) {
		slog("Couldn't connect to postgres!: %s", PQerrorMessage(sql_cxn));
		safe_exit(1);
	}

	if (production_mode) {
		slog("Vacuuming old database transactions");
		sql_exec("vacuum full analyze");
	}

	res = sql_query("select last_value from unique_id");
	top_unique_id = atol(PQgetvalue(res, 0, 0));
	slog("Top unique object id = %ld", top_unique_id);

	Account::boot();
	load_bounty_data();
	slog("Reading credits, bground, info & motds.");
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(ANSI_MOTD_FILE, &ansi_motd);
	file_to_string_alloc(IMOTD_FILE, &imotd);
	file_to_string_alloc(ANSI_IMOTD_FILE, &ansi_imotd);
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

    slog("Loading tongues.");
    boot_tongues();

	boot_dynamic_text();
	boot_world();

	reset_zone_weather();

	slog("Booting clans.");
	boot_clans();

	slog("Booting quests.");
	boot_quests();

	slog("Loading fight messages.");
	load_messages();

	slog("Loading social messages.");
	boot_social_messages();

	slog("Assigning function pointers:");

	if (!no_specials) {
		slog("   Mobiles.");
		assign_mobiles();
		slog("   Objects.");
		assign_objects();
		slog("   Rooms.");
		assign_rooms();
		slog("   Artisans.");
		assign_artisans();
	}
	slog("   Spells.");
    boot_spells();
	while (spells[max_spell_num][0] != '\n')
		max_spell_num++;


	slog("Sorting command list and spells.");
	sort_commands();
	sort_spells();
	sort_skills();
	Security::loadGroups();

	slog("Compiling progs.");
	compile_all_progs();

	slog("Reading banned site, invalid-name, and NASTY word lists.");
	load_banned();
	Read_Invalid_List();
	Read_Nasty_List();

	slog("Reading paths.");
	Load_paths();

	slog("Booting timewarp data.");
	boot_timewarp_data();

    if( mini_mud ) {
        slog("HOUSE: Mini-mud detected. Houses not loading.");
	} else {
		slog("HOUSE: Booting houses.");
		Housing.load();
		Housing.countObjects();
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
		slog("Help system boot succeeded.");
	else
		errlog("Help System Boot FAILED.");

	reset_q.head = reset_q.tail = NULL;

	boot_time = time(0);

	slog("Boot db -- DONE.");
}


/* reset the time in the game from file */
void
reset_time(void)
{
	long epoch = 650336715;
	time_t now = time(0);
	struct tm *sun_tm;
	char sun_str[30];

	sun_tm = localtime(&now);
	sun_tm->tm_mday -= sun_tm->tm_wday;
	sun_tm->tm_sec = sun_tm->tm_min = sun_tm->tm_hour = 0;
	last_sunday_time = mktime(sun_tm);
	strftime(sun_str, 55, "%Y-%m-%d %H-%M-%S", localtime(&last_sunday_time));

	slog("   Last Realtime Sunday: %s", sun_str);

	time_info = mud_time_passed(now, epoch);

	slog("   Current Gametime (global): %dH %dD %dM %dY.",
		time_info.hours, time_info.day, time_info.month, time_info.year);

	lunar_day = ((now - epoch) / SECS_PER_MUD_DAY) % 24;;
	slog("   Current lunar day: %d (%s)",
		lunar_day, lunar_phases[get_lunar_phase(lunar_day)]);
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
	default:
		errlog("Unknown subcommand to index_boot!");
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
		errlog("boot error - 0 records counted");
		safe_exit(1);
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
		null_mob_shared->proto = NULL;
		null_mob_shared->move_buf = NULL;
		break;

	case DB_BOOT_OBJ:
		CREATE(null_obj_shared, struct obj_shared_data, 1);
		null_obj_shared->vnum = -1;
		null_obj_shared->number = 0;
		null_obj_shared->house_count = 0;
		null_obj_shared->func = NULL;
		null_obj_shared->proto = NULL;
		break;

	case DB_BOOT_ZON:
		break;
	}

	if (mode != DB_BOOT_ZON) {
		rewind(index);
		if (mode == DB_BOOT_OBJ)
			CREATE(obj_index, int, index_count + 1);
		else if (mode == DB_BOOT_MOB)
			CREATE(mob_index, int, index_count + 1);
		else if (mode == DB_BOOT_WLD)
			CREATE(wld_index, int, index_count + 1);

		for (i = 0; i < index_count; i++) {
			if (mode == DB_BOOT_OBJ) {
				fscanf(index, "%d.obj\n", &number);
				obj_index[i] = number;
			} else if (mode == DB_BOOT_MOB) {
				fscanf(index, "%d.mob\n", &number);
				mob_index[i] = number;
			} else if (mode == DB_BOOT_WLD) {
				fscanf(index, "%d.wld\n", &number);
				wld_index[i] = number;
			}
		}

		if (mode == DB_BOOT_OBJ)
			obj_index[index_count] = -1;
		else if (mode == DB_BOOT_MOB)
			mob_index[index_count] = -1;
		else if (mode == DB_BOOT_WLD)
			wld_index[index_count] = -1;
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
			discrete_load(db_file, mode);
			break;
		case DB_BOOT_ZON:
			load_zones(db_file, buf2);
			break;
		}

		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
}

void
discrete_load(FILE * fl, int mode)
{
	int nr = -1, last = 0;
	char line[256];

	char *modes[] = { "world", "mob", "obj", "zon", "shp" };

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
		case 'R':
		  room->prog = fread_string(fl, buf2);
          break;
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
				errlog("Multiple flow states assigned to room #%d.\n",
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
				errlog("Illegal flow type in room #%d.",
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

			if ((sscanf(line, "%d %d %d %d %d %d",
						t + 5, t + 1, t + 2, t + 3, t + 4, t + 6) != 6)) {
				if ((sscanf(line, "%d %d %d %d %d",
							t + 5, t + 1, t + 2, t + 3, t + 4) != 5)) {
					fprintf(stderr, "Search Field format error in room #%d\n",
						vnum_nr);
					safe_exit(1);
				}
				t[6] = 0;
			}

			new_search->command = t[5];
			new_search->arg[0] = t[1];
			new_search->arg[1] = t[2];
			new_search->arg[2] = t[3];
			new_search->flags = t[4];
			new_search->fail_chance = t[6];

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

		case 'P':
			room->func_param = fread_string(fl, buf2); break;
		case 'S':				/* end of room */
			top_of_world = room_nr++;

			room->next = NULL;

			// Eliminate duplicates
			// FIXME: This leaks all the memory the room takes up.  Doing it
			// this way because a) it shouldn't ever happen b) if it does
			// happen, it'll get fixed the next time the zone is saved and
			// c) rooms don't take up that much space anyway
			if (real_room(vnum_nr)) {
			  errlog("Duplicate room %d detected.  Ignoring second instance.", vnum_nr);
			  return;
			}

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
		errlog("Room direction > NUM_DIRS in room #%d", room->number);
		safe_exit(1);
	}

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
	extern room_num kromguard_start_room;
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
		errlog(" Mortal start room does not exist.  Change in config.c.");
		safe_exit(1);
	}
	if ((r_electro_start_room = real_room(electro_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Electro Centralis start room does not exist.");
		r_electro_start_room = r_mortal_start_room;
	}
	if ((r_immort_start_room = real_room(immort_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Immort start room does not exist.");
		r_immort_start_room = r_mortal_start_room;
	}
	if ((r_frozen_start_room = real_room(frozen_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Frozen start room does not exist.");
		r_frozen_start_room = r_mortal_start_room;
	}
	if ((r_new_thalos_start_room = real_room(new_thalos_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: New Thalos start room does not exist.");
		r_new_thalos_start_room = r_mortal_start_room;
	}
	if ((r_kromguard_start_room = real_room(kromguard_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Kromguard start room does not exist.");
		r_kromguard_start_room = r_mortal_start_room;
	}
	if ((r_elven_start_room = real_room(elven_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Elven Village start room does not exist.");
		r_elven_start_room = r_mortal_start_room;
	}
	if ((r_istan_start_room = real_room(istan_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Istan start room does not exist.");
		r_istan_start_room = r_mortal_start_room;
	}
	if ((r_arena_start_room = real_room(arena_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Arena start room does not exist.");
		r_arena_start_room = r_mortal_start_room;
	}
	if ((r_tower_modrian_start_room =
			real_room(tower_modrian_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Arena start room does not exist.");
		r_tower_modrian_start_room = r_mortal_start_room;
	}
	if ((r_monk_start_room = real_room(monk_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Monk start room does not exist.");
		r_monk_start_room = r_mortal_start_room;
	}
	if ((r_skullport_newbie_start_room =
			real_room(skullport_newbie_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Skullport Newbie Academy start room does not exist.");
		r_skullport_newbie_start_room = r_mortal_start_room;
	}
	if ((r_solace_start_room = real_room(solace_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Solace Cove start room does not exist.");
		r_solace_start_room = r_mortal_start_room;
	}
	if ((r_mavernal_start_room = real_room(mavernal_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Mavernal start room does not exist.");
		r_mavernal_start_room = r_mortal_start_room;
	}
	if ((r_dwarven_caverns_start_room =
			real_room(dwarven_caverns_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Dwarven Caverns  start room does not exist.");
		r_dwarven_caverns_start_room = r_mortal_start_room;
	}
	if ((r_human_square_start_room =
			real_room(human_square_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Human Square start room does not exist.");
		r_human_square_start_room = r_mortal_start_room;
	}
	if ((r_skullport_start_room = real_room(skullport_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Skullport start room does not exist.");
		r_skullport_start_room = r_mortal_start_room;
	}
	if ((r_drow_isle_start_room = real_room(drow_isle_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Drow Isle start room does not exist.");
		r_drow_isle_start_room = r_mortal_start_room;
	}
	if ((r_astral_manse_start_room =
			real_room(astral_manse_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Astral Manse start room does not exist.");
		r_astral_manse_start_room = r_mortal_start_room;
	}

	if ((r_zul_dane_start_room = real_room(zul_dane_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Zul Dane start room does not exist.");
		r_zul_dane_start_room = r_mortal_start_room;
	}

	if ((r_zul_dane_newbie_start_room =
			real_room(zul_dane_newbie_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Zul Dane newbie start room does not exist.");
		// set it to the normal zul dane room ...
		r_zul_dane_newbie_start_room = r_zul_dane_start_room;
	}
	if ((r_newbie_school_start_room =
			real_room(newbie_school_start_room)) == NULL) {
		if (!mini_mud)
			errlog(" Warning: Zul Dane newbie start room does not exist.");
		// set it to the normal zul dane room ...
		r_newbie_school_start_room = r_mortal_start_room;
	}


}

void
renum_world(void)
{
    // store the rooms in a map temoporarily for use in lookups
	for( zone_data* zone = zone_table; zone; zone = zone->next) {
		for( room_data* room = zone->world; room; room = room->next) {
            rooms[room->number] = room;
        }
    }
	slog("%ld rooms loaded.", rooms.size());
    // lookup each room's doors and reconnect the to_room pointers
    for( zone_data* zone = zone_table; zone; zone = zone->next) {
		for( room_data* room = zone->world; room; room = room->next) {
			for (int door = 0; door < NUM_OF_DIRS; ++door) {
				if (room->dir_option[door]) {
                    // the to_room pointer has the room # stored in it during bootup
                    int vnum = (long int)room->dir_option[door]->to_room;
                    room->dir_option[door]->to_room = NULL;
                    // if it points somewhere and is in the map
					if( vnum != NOWHERE && rooms.count(vnum) > 0 ) {
                        room->dir_option[door]->to_room = rooms[vnum];
                    }
                }
            }
        }
    }
}

/* resolve vnums into rnums in the zone reset tables */
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
			if (!mini_mud && !scheck) {
				if (a < 0)
					zerrlog(zone, "Invalid vnum %d - cmd disabled", a);
				if (b < 0)
					zerrlog(zone, "Invalid vnum %d - cmd disabled", b);
				if (a < 0 || b < 0)
					zonecmd->command = '*';
			}
		}
}

void
compile_all_progs(void)
{
	// Compile all room progs
    for (zone_data *zone = zone_table; zone; zone = zone->next)
		for (room_data *room = zone->world; room; room = room->next)
          if (room->prog)
            prog_compile(NULL, room, PROG_TYPE_ROOM);

	// Compile all mob progs
	MobileMap::iterator mit = mobilePrototypes.begin();
	Creature *mobile;

	for (; mit != mobilePrototypes.end(); ++mit) {
		mobile = mit->second;
		if (MOB_SHARED(mobile)->prog)
		  prog_compile(NULL, mobile, PROG_TYPE_MOBILE);
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
	if (IS_CLERIC(ch) || IS_MAGE(ch) || IS_LICH(ch) || IS_PHYSIC(ch) || IS_PSYCHIC(ch))
		GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 4);
	else if (IS_KNIGHT(ch) || IS_RANGER(ch))
		GET_MAX_MANA(ch) += (GET_LEVEL(ch) << 1);

	if (IS_RANGER(ch))
		GET_MAX_MOVE(ch) += (GET_LEVEL(ch) << 3);

    GET_MAX_MOVE(ch) += ( GET_REMORT_GEN(ch) * GET_MAX_MOVE(ch))/10;
    GET_MAX_MANA(ch) += ( GET_REMORT_GEN(ch) * GET_MAX_MANA(ch))/10;

	ch->aff_abils = ch->real_abils;
}


void recalculate_based_on_level( Creature *mob_p ) 
{
    int level = GET_LEVEL(mob_p);
    int doubleLevel = level + (level * GET_REMORT_GEN(mob_p))/10;
    int gen = GET_REMORT_GEN(mob_p);

    set_physical_attribs(mob_p);

    GET_HIT(mob_p) = MOB_D1(doubleLevel); // hitd_num
    GET_MANA(mob_p) = MOB_D2(level);// hitd_size
    GET_MOVE(mob_p) = MOB_MOD(level);// hitp_mod
    GET_MOVE(mob_p) += (int) ( 3.26 * ( gen * gen ) * level );


    GET_AC(mob_p) = (100 - (doubleLevel * 3));

    mob_p->mob_specials.shared->damnodice = ((level * level + 3) >> 7) + 1;
    mob_p->mob_specials.shared->damsizedice = ((level + 1) >> 3) + 3;
    GET_DAMROLL(mob_p) = (level/2) + ((level/2) * GET_REMORT_GEN(mob_p)) / 10;
    GET_HITROLL(mob_p) = (int)(GET_DAMROLL(mob_p) * 1.25);
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
	long long num_arg, matched = 0;

	num_arg = atoll(value);

	CASE("BareHandAttack") {
		RANGE(0, 99);
		mobile->mob_specials.shared->attack_type = (int)num_arg;
	}

	CASE("Move_buf") {
		MOB_SHARED(mobile)->move_buf = str_dup(value);
	}

	CASE("Str") {
		RANGE(3, 25);
		mobile->real_abils.str = (int)num_arg;
	}

	CASE("StrAdd") {
		RANGE(0, 100);
		mobile->real_abils.str_add = (int)num_arg;
	}

	CASE("Int") {
		RANGE(3, 25);
		mobile->real_abils.intel = (int)num_arg;
	}

	CASE("Wis") {
		RANGE(3, 25);
		mobile->real_abils.wis = (int)num_arg;
	}

	CASE("Dex") {
		RANGE(3, 25);
		mobile->real_abils.dex = (int)num_arg;
	}

	CASE("Con") {
		RANGE(3, 25);
		mobile->real_abils.con = (int)num_arg;
	}

	CASE("Cha") {
		RANGE(3, 25);
		mobile->real_abils.cha = (int)num_arg;
	}

	CASE("MaxMana") {
		RANGE(0, 4000);
		mobile->points.max_mana = (int)num_arg;
	}
	CASE("MaxMove") {
		RANGE(0, 2000);
		mobile->points.max_move = (int)num_arg;
	}
	CASE("Height") {
		RANGE(1, 10000);
		mobile->player.height = (int)num_arg;
	}
	CASE("Weight") {
		RANGE(1, 10000);
		mobile->player.weight = (int)num_arg;
	}
	CASE("RemortClass") {
		RANGE(0, 1000);
		mobile->player.remort_char_class = (int)num_arg;
        if( GET_REAL_GEN(mobile) == 0 )
            GET_REAL_GEN(mobile) = 1;
	}
	CASE("Class") {
		RANGE(0, 1000);
		mobile->player.char_class = (int)num_arg;
	}
	CASE("Race") {
		RANGE(0, 1000);
		mobile->player.race = (int)num_arg;
	}
	CASE("Credits") {
		RANGE(0, 1000000);
		mobile->points.cash = (int)num_arg;
	}
	CASE("Cash") {
		RANGE(0, 1000000);
		mobile->points.cash = (int)num_arg;
	}
	CASE("Econet") {
		RANGE(0, 1000000);
	}
	CASE("Morale") {
		RANGE(0, 120);
		mobile->mob_specials.shared->morale = (int)num_arg;
	}
	CASE("Lair") {
		RANGE(-99999, 99999);
		mobile->mob_specials.shared->lair = (int)num_arg;
	}
	CASE("Leader") {
		RANGE(-99999, 99999);
		mobile->mob_specials.shared->leader = (int)num_arg;
	}
    CASE("Generation") {
        RANGE(0,10);
        GET_REAL_GEN(mobile) = (int)num_arg;
    }
    CASE("KnownLang") {
        // deprecated conversion
        for (int i = 0;i < 32;i++)
            if ((1 << i) & num_arg)
                SET_TONGUE(mobile, i + 1, 100);
    }
    CASE("CurLang") {
        // deprecated conversion
        GET_TONGUE(mobile) = num_arg + 1;
    }
    CASE("CurTongue") {
        GET_TONGUE(mobile) = num_arg;
    }
    CASE("KnownTongue") {
        SET_TONGUE(mobile, num_arg, 100);
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
		if (!strcmp(line, "SpecParam:")) { /* multi-line specparam */
			MOB_SHARED(mobile)->func_param = fread_string(mob_f, buf2);
		} else if (!strcmp(line, "LoadParam:")) { /* multi-line load param*/
			MOB_SHARED(mobile)->load_param = fread_string(mob_f, buf2);
		} else if (!strcmp(line, "Prog:")) { /* multi-line prog */
			MOB_SHARED(mobile)->prog = fread_string(mob_f, buf2);
		} else if (!strcmp(line, "E"))	/* end of the ehanced section */
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
parse_mobile(FILE * mob_f, int nr)
{
	bool done = false;
	static int i = 0;
	int j, t[10];
	char line[256], *tmpptr, letter;
	char f1[128], f2[128], f3[128], f4[128], f5[128];
	struct Creature *mobile = NULL, *tmp_mob = NULL;

	mobile = new Creature(false);

	tmp_mob = real_mobile_proto(nr);

	CREATE(mobile->mob_specials.shared, struct mob_shared_data, 1);
	mobile->mob_specials.shared->vnum = nr;
	mobile->mob_specials.shared->number = 0;
	mobile->mob_specials.shared->func = NULL;
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
		case 'R':				/* reply text - no longer used */
			free(fread_string(mob_f, buf2));
            free(fread_string(mob_f, buf2));
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
    set_initial_tongue(mobile);

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
	struct obj_data *obj = NULL;

	CREATE(obj, struct obj_data, 1);
	
	obj->clear();

	CREATE(obj->shared, struct obj_shared_data, 1);

	obj->shared->vnum = nr;
	obj->shared->number = 0;
	obj->shared->house_count = 0;
	obj->shared->func = NULL;
	obj->shared->proto = obj;
	obj->shared->owner_id = 0;

	obj->in_room = NULL;

	sprintf(buf2, "object #%d", nr);

	/* *** string data *** */
	if ((obj->aliases = fread_string(obj_f, buf2)) == NULL) {
		fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
		safe_exit(1);
	}
	tmpptr = obj->name = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
			!str_cmp(fname(tmpptr), "the"))
			*tmpptr = tolower(*tmpptr);

	tmpptr = obj->line_desc = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
		*tmpptr = toupper(*tmpptr);
	obj->action_desc = fread_string(obj_f, buf2);

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
			"Format error in first numeric line (expecting 4 or 5 args, got %d), %s\n",
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

    t[3] = 0;
	if (!get_line(obj_f, line) ||
		(retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) < 3) {
		fprintf(stderr,
			"Format error in fourth numeric line (expecting 3 or 4 args, got %d), %s\n",
			retval, buf2);
		safe_exit(1);
	}
	obj->setWeight(t[0]);
	obj->shared->cost = t[1];
	obj->shared->cost_per_day = t[2];
    if (t[3] < 0)
	    t[3] = 0;
    obj->obj_flags.timer = t[3];

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
		case 'O':
			sscanf(line+1, " %ld ", &(obj->shared->owner_id) );
			break;
		case 'V':
			get_line(obj_f, line);
			sscanf(line, " %d %s ", t, f1);
			if (t[0] < 1 || t[0] > 3)
				break;
			obj->obj_flags.bitvector[t[0] - 1] = asciiflag_conv(f1);
			break;
		case 'P':
			obj->shared->func_param = fread_string(obj_f, buf2); break;
		case '$':
		case '#':
			top_of_objt = i++;
			obj->next = NULL;
            objectPrototypes.add(obj);
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
	char *arg1, *arg2;

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
	
	if (!strncmp(buf, "RP ", 3)) {
		new_zone->respawn_pt = atoi(buf + 3);
		line_num += get_line(fl, buf);
	} else
		new_zone->respawn_pt = 0;
	
	// New format reading starts now.
	while (true) {
		arg2 = buf;
		arg1 = tmp_getword(&arg2);
		if (!strcmp(arg1, "owner:"))
			new_zone->owner_idnum = atoi(arg2);
		else if (!strcmp(arg1, "co-owner:"))
			new_zone->co_owner_idnum = atoi(arg2);
		else if (!strcmp(arg1, "respawn-pt:"))
			new_zone->respawn_pt = atoi(arg2);
		else if (!strcmp(arg1, "minimum-level:"))
			new_zone->min_lvl = atoi(arg2);
		else if (!strcmp(arg1, "minimum-gen:"))
			new_zone->min_gen = atoi(arg2);
		else if (!strcmp(arg1, "maximum-level:"))
			new_zone->max_lvl = atoi(arg2);
		else if (!strcmp(arg1, "maximum-gen:"))
			new_zone->max_gen = atoi(arg2);
		else if (!strcmp(arg1, "public-desc:"))
			new_zone->public_desc = fread_string(fl, buf2);
		else if (!strcmp(arg1, "private-desc:"))
			new_zone->private_desc = fread_string(fl, buf2);
		else
			break;

		line_num += get_line(fl, buf);
	}

    int result = sscanf(buf, " %d %d %d %d %d %s %d %d %d", &new_zone->top,
                        &new_zone->lifespan, &new_zone->reset_mode,
                        &new_zone->time_frame, &new_zone->plane, flags,
                        &new_zone->hour_mod, &new_zone->year_mod, 
                        &new_zone->pk_style);
	if (result == 8) {
        new_zone->pk_style = ZONE_CHAOTIC_PK;
    }
    else if (result != 9) {
		fprintf(stderr, "Format error in 9-constant line of %s\n", zname);
		fprintf(stderr, "Line was: %s", buf);
		safe_exit(0);
	}

	new_zone->flags = asciiflag_conv(flags);
	new_zone->num_players = 0;
	new_zone->idle_time = 0;
//    REMOVE_BIT(new_zone->flags, (1 << 12));

	CREATE(weather, struct weather_data, 1);
    if (weather) {
        weather->pressure = 0;
        weather->change = 0;
        weather->sky = 0;
        weather->sunlight = 0;
        weather->humid = 0;

        new_zone->weather = weather;
    }
    else {
        errlog("WTF?? No weather in db.cc??");
        new_zone->weather = NULL;
    }

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
			if (!zone->next || (zone->next->number > new_zone->number)) {
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

	MobileMap::iterator mit = mobilePrototypes.begin();
	for (; mit != mobilePrototypes.end(); ++mit) {
		mobile = mit->second;
		if (namelist_match(searchname, mobile->player.name)) {
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
    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;
		if (namelist_match(searchname, obj->aliases)) {
			sprintf(buf, "%s%3d. %s[%s%5d%s]%s %s%s\r\n", buf, ++found,
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->shared->vnum,
				CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
				obj->name, CCNRM(ch, C_NRM));
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
	struct Creature *mob = NULL, *tmp_mob;

	if (!(tmp_mob = real_mobile_proto(vnum))) {
		sprintf(buf, "Mobile (V) %d does not exist in database.", vnum);
		return (NULL);
	}
    mob = new Creature(*tmp_mob);
    tmp_mob->mob_specials.shared->number++;
    tmp_mob->mob_specials.shared->loaded++;
    
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
	if (MOB2_FLAGGED(mob, MOB2_UNAPPROVED))
		GET_GOLD(mob) = GET_CASH(mob) = GET_EXP(mob) = 0;
    else { // randomize mob gold and cash
        if (GET_GOLD(mob) > 0)
            GET_GOLD(mob) = rand_value(GET_GOLD(mob), 
                                       (int)(GET_GOLD(mob) * 0.15), -1, -1);
        if (GET_CASH(mob) > 0)
            GET_CASH(mob) = rand_value(GET_CASH(mob), 
                                       (int)(GET_CASH(mob) * 0.15), -1, -1);
    }

	return mob;
}

int on_load_equip( Creature *ch, int vnum, char* position, int maxload, int percent );

// Processes the given mobile's load_parameter, executing the commands in it.
// returns true if the creature dies
bool 
process_load_param( Creature *ch )
{
	char* str = GET_LOAD_PARAM(ch);
	int mob_id = ch->mob_specials.shared->vnum;
	int lineno = 0;
	int last_cmd = 0;
	if( str == NULL )
		return false;

	for( char* line = tmp_getline(&str); line; line = tmp_getline(&str) ) {
		++lineno;
		char *param_key = tmp_getword(&line);
		if( strcasecmp(param_key, "LOAD") == 0 ) {
			char* vnum = tmp_getword(&line); // vnum
			char* pos = tmp_getword(&line); // position
			char* max = tmp_getword(&line); //max loaded
			char* p = tmp_getword(&line); // percent
			int if_flag = atoi( tmp_getword(&line) ); // if flag
			// if_flag 
			// 1 == "Do if previous succeded"
			if( if_flag == 1 && last_cmd == 1 )
				continue;
			//-1 == "Do if previous failed" by percentage
			if( if_flag == -1 && last_cmd != 1 )
				continue;
			// 0 == "Do regardless of previous"

			if( *vnum && *pos && *max && *p ) {
				last_cmd = on_load_equip(ch, atoi(vnum), pos, atoi(max), atoi(p));
				if( last_cmd == 100 ) {
					slog("Mob number %d killed during load param processing line %d.\r\n",
							mob_id, lineno );
					return true;
				}
			} else {
				char* msg = tmp_sprintf("Line %d of my load param has the wrong format!",lineno);
				perform_say(ch, "yell", msg);
			}
		}
	}
    return true;
 }


// returns:
// 0, Success
// 1, Not loaded, percentage failure.
// 2, Not loaded, already equipped.
// 3, No such object
// 4, Invalid position
// 5, read_obj failed
// 6, Not loaded, maxload failure
// 100, Creature died in equip process
int 
on_load_equip( Creature *ch, int vnum, char* position, int maxload, int percent )
{
    obj_data *obj = real_object_proto(vnum);
    if( obj == NULL ) {
        errlog("Mob num %d: equip object %d nonexistent.",
			ch->mob_specials.shared->vnum, vnum );
		if( MOB2_FLAGGED(ch, MOB2_UNAPPROVED) )
			perform_say(ch, "yell",
                        tmp_sprintf("Object %d doesn't exist!",vnum));
        return 3;
    }
    if( obj->shared->number - obj->shared->house_count >= maxload) {
        return 6;
	}

    if( random_percentage() > percent ){
        return 1;
    }
	int pos = -1;
	if( strcasecmp( position, "take" ) == 0 ) {
		pos = ITEM_WEAR_TAKE;
	} else if( IS_OBJ_STAT2(obj, ITEM2_IMPLANT) ) {
		pos = search_block(position, wear_implantpos, FALSE);
	} else {
		pos = search_block(position, wear_eqpos, FALSE);
	}

    if (pos < 0 || pos >= NUM_WEARS) {
		if( MOB2_FLAGGED(ch, MOB2_UNAPPROVED) )
			perform_say(ch, "yell",
                        tmp_sprintf("%s is not a valid position!",position));

        errlog("Mob num %d trying to equip obj %d to badpos: '%s'",
			ch->mob_specials.shared->vnum, obj->shared->vnum, position);
        return 4;
    }
	obj = read_object(vnum);
	obj->creation_method = CREATED_MOB;
	obj->creator = vnum;

	randomize_object(obj);
    if( obj == NULL ) {
        errlog("Mob num %d cannot load equip object #%d.",
            ch->mob_specials.shared->vnum, vnum);
		if( MOB2_FLAGGED(ch, MOB2_UNAPPROVED) )
			perform_say(ch, "yell",
                        tmp_sprintf("Loading object %d failed!", vnum ));
        return 5;
    }

	// Unapproved mobs should load unapproved eq.
	if( MOB2_FLAGGED(ch, MOB2_UNAPPROVED) ) {
	    SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
	}
	if( pos == ITEM_WEAR_TAKE ) {
		obj_to_char( obj, ch, true );
	} else {
		int mode = EQUIP_WORN;
		if( IS_OBJ_STAT2(obj, ITEM2_IMPLANT) ) {
			mode = EQUIP_IMPLANT;
			if( ch->implants[pos] ) {
				extract_obj(obj);
				return 2;
			}
		} else {
			if( ch->equipment[pos] ) {
				extract_obj(obj);
				return 2;
			}
		}

		if( equip_char(ch, obj, pos, mode ) ) {
			return 100;
		}
	}
    return 0;
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

void
randomize_object(struct obj_data *obj)
{
	int idx, bit, total_affs = 0;

	// Applies
	for (idx = 0;idx < MAX_OBJ_AFFECT;idx++)
		if (obj->affected[idx].location != APPLY_NONE
				&& obj->affected[idx].location != APPLY_CLASS
				&& obj->affected[idx].location != APPLY_RACE
				&& obj->affected[idx].location != APPLY_SEX
				&& obj->affected[idx].location != APPLY_CASTER
				&& obj->affected[idx].location != APPLY_NOTHIRST
				&& obj->affected[idx].location != APPLY_NOHUNGER
				&& obj->affected[idx].location != APPLY_NODRUNK) {
			if (obj->affected[idx].modifier > 0)
				obj->affected[idx].modifier = number(0, obj->affected[idx].modifier);
			else if (obj->affected[idx].modifier < 0)
				obj->affected[idx].modifier = -number(0, -obj->affected[idx].modifier);
			if (!obj->affected[idx].modifier)
				obj->affected[idx].location = APPLY_NONE;
			else
				total_affs++;
		}
	obj->normalizeApplies();

	// Affects
	for (idx = 0;idx < 32;idx++) {
		bit = (1 << idx);
		if (IS_SET(obj->obj_flags.bitvector[0], bit)) {
			if (!number(0, 10))
				REMOVE_BIT(obj->obj_flags.bitvector[0], bit);
			else
				total_affs++;
		}
		if (IS_SET(obj->obj_flags.bitvector[1], bit)) {
			if (!number(0, 10))
				REMOVE_BIT(obj->obj_flags.bitvector[1], bit);
			else
				total_affs++;
		}
		if (IS_SET(obj->obj_flags.bitvector[2], bit)) {
			if (!number(0, 10))
				REMOVE_BIT(obj->obj_flags.bitvector[2], bit);
			else
				total_affs++;
		}
	}

	// Remove magicalness if no affects are left
	if (!total_affs)
		REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC | ITEM_BLESS | ITEM_DAMNED);
	
	switch (GET_OBJ_TYPE(obj)) {
	// Spell level in first value
	case ITEM_SCROLL:
	case ITEM_PILL:
	case ITEM_POTION:
	case ITEM_SYRINGE:
		GET_OBJ_VAL(obj, 0) = rand_value(GET_OBJ_VAL(obj, 0), 10, 1, 49);
		break;
	// Spell level in first, max charges in second
	case ITEM_WAND:
	case ITEM_STAFF:
		GET_OBJ_VAL(obj, 0) = rand_value(GET_OBJ_VAL(obj, 0), 10, 1, 49);
		GET_OBJ_VAL(obj, 1) = rand_value(
			GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 1) / 4,
			1,
			-1);
		GET_OBJ_VAL(obj, 2) = rand_value(
			GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 2) / 4,
			1,
			GET_OBJ_VAL(obj, 1));
		break;
	// First value just needs some variation
	case ITEM_CHIT:
	case ITEM_ARMOR:
	case ITEM_CONTAINER:
	case ITEM_MONEY:
		GET_OBJ_VAL(obj, 0) = rand_value(
			GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 0) / 4,
			1,
			-1);
		break;
	// First value is max charges, second is current
	case ITEM_BATTERY:
	case ITEM_DEVICE:
	case ITEM_ENERGY_CELL:
	case ITEM_ENGINE:
	case ITEM_COMMUNICATOR:
	case ITEM_TRANSPORTER:
		GET_OBJ_VAL(obj, 0) = rand_value(
			GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 0) / 4,
			1,
			-1);
		GET_OBJ_VAL(obj, 1) = rand_value(
			GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 1) / 4,
			1,
			GET_OBJ_VAL(obj, 0));
		break;
	// Weapon damage dice
	case ITEM_ENERGY_GUN:
        GET_OBJ_VAL(obj, 0) = rand_value( //randomize the drain rate
			GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 0) / 4,
			1, -1);
            //no break here because we want to randomize the damage dice as well
    case ITEM_WEAPON:
		GET_OBJ_VAL(obj, 1) = rand_value(
			GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 1) / 4,
			1, -1);
        GET_OBJ_VAL(obj, 2) = rand_value(
			GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 2) / 4,
			1, -1);
		break;
	// Vstone and portal charges
	case ITEM_VSTONE:
		GET_OBJ_VAL(obj, 2) = rand_value(
			GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 2) / 4,
			1, -1);
        break;
	case ITEM_PORTAL:
        if (GET_OBJ_VAL(obj, 3) > 0) {
		    GET_OBJ_VAL(obj, 3) = rand_value(
			    GET_OBJ_VAL(obj, 3),
			    GET_OBJ_VAL(obj, 3) / 4,
			    1, -1);
        }
		break;
	}
}

/* create a new object from a prototype */
struct obj_data *
read_object(int vnum)
{
	struct obj_data *obj = NULL, *tmp_obj = NULL;

	if (!(tmp_obj = objectPrototypes.find(vnum))) {
		slog("Object (V) %d does not exist in database.", vnum);
		return NULL;
	}
    CREATE(obj, struct obj_data, 1);
    *obj = *tmp_obj;
    tmp_obj->shared->number++;

#ifdef TRACK_OBJS
	// temp debugging
	obj->obj_flags.tracker.lost_time = time(0);
	obj->obj_flags.tracker.string[TRACKER_STR_LEN - 1] = '\0';

#endif

	unique_id_changed = true;
	top_unique_id += 1;
	obj->unique_id = top_unique_id;
	obj->creation_time = time(NULL);
	obj->creator = 0;
	obj->creation_method = CREATED_UNKNOWN;

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

	// Send SPECIAL_RESET notification to all mobiles with specials
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		// Wrong zone
		if ((*cit)->in_room->zone != zone)
			continue;
		// No special
		if (!MOB_FLAGGED((*cit), MOB_SPEC))
			continue;
		// Mobile with special gets a reset notice
		if (GET_MOB_SPEC((*cit))) {
			GET_MOB_SPEC((*cit))((*cit), (*cit), 0, "", SPECIAL_RESET);
			continue;
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
						if( process_load_param( mob ) ) { // true on death
							last_cmd = 0; 
						} else {
							last_cmd = 1;
						}
						if (GET_MOB_PROGOBJ(mob))
							trigger_prog_load(mob);
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
						obj->creation_method = CREATED_ZONE;
						obj->creator = zone->number;
						randomize_object(obj);
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
				obj->creation_method = CREATED_ZONE;
				obj->creator = zone->number;
				randomize_object(obj);
				if (!(obj_to = get_obj_num(zonecmd->arg3))) {
					ZONE_ERROR("target obj not found");
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					extract_obj(obj);
					break;
				}
				if (obj == obj_to) {
					ZONE_ERROR("target object cannot be put into itself");
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
					ZONE_ERROR("attempt to give obj to nonexistent mob");
				break;
			}
			tobj = real_object_proto(zonecmd->arg1);
			if (tobj != NULL &&
				tobj->shared->number - tobj->shared->house_count <
				zonecmd->arg2) {
				obj = read_object(zonecmd->arg1);
				obj->creation_method = CREATED_ZONE;
				obj->creator = zone->number;
				if (GET_MOB_SPEC(mob) != vendor)
					randomize_object(obj);
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
					ZONE_ERROR("trying to equip nonexistent mob");
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
					obj->creation_method = CREATED_ZONE;
					obj->creator = zone->number;
					randomize_object(obj);
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					if (equip_char(mob, obj, zonecmd->arg3, EQUIP_WORN)) {
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
					ZONE_ERROR("trying to implant nonexistent mob");
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
					obj->creation_method = CREATED_ZONE;
					obj->creator = zone->number;
					randomize_object(obj);
					if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED)) {
						SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
						GET_OBJ_TIMER(obj) = 60;
					}
					if (equip_char(mob, obj, zonecmd->arg3, EQUIP_IMPLANT)) {
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
					ZONE_ERROR("trying to assign path to nonexistent mob");
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
			errlog("fread_string: format error at or near %s\n",
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
			errlog("fread_string: string too large (db.c)");
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
			errlog("pread_string: string too large: %s\n", error);
			return 0;
		} else {
			strcat(str + length, tmp);
			length += templength;
		}
	} while (!done);

	return 1;
}

/* release memory allocated for an obj struct */
void
free_obj(struct obj_data *obj)
{
	struct extra_descr_data *this_desc, *next_one;

	if (!obj->shared || GET_OBJ_VNUM(obj) == -1 || !obj->shared->proto) {
		if (obj->aliases) {
			free(obj->aliases);
			obj->aliases = NULL;
		}
		if (obj->line_desc) {
			free(obj->line_desc);
			obj->line_desc = NULL;
		}
		if (obj->name) {
			free(obj->name);
			obj->name = NULL;
		}
		if (obj->action_desc) {
			free(obj->action_desc);
			obj->action_desc = NULL;
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
		if (obj->aliases && obj->aliases != obj->shared->proto->aliases) {
			free(obj->aliases);
			obj->aliases = NULL;
		}
		if (obj->line_desc &&
			obj->line_desc != obj->shared->proto->line_desc) {
			free(obj->line_desc);
			obj->line_desc = NULL;
		}
		if (obj->name &&
			obj->name != obj->shared->proto->name) {
			free(obj->name);
			obj->name = NULL;
		}
		if (obj->action_desc &&
			obj->action_desc !=
			obj->shared->proto->action_desc) {
			free(obj->action_desc);
			obj->action_desc = NULL;
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
        while (obj->tmp_affects) {
            tmp_obj_affect *aff;

            aff = obj->tmp_affects;
            obj->removeAffect(aff);
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
				errlog("fl->strng: string too big (db.c, file_to_string)");
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}




/* returns the real number of the room with given vnum number */
struct room_data *
real_room(int vnum)
{
	if (rooms.count(vnum) < 1)
        return NULL;
    return rooms[vnum];
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
/*	struct Creature *mobile = NULL;

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
	return (NULL); */

    return mobilePrototypes.find(vnum);
}

struct obj_data *
real_object_proto(int vnum)
{
/*	struct obj_data *object = NULL;

	for (object = obj_proto; object; object = object->next)
		if (object->shared->vnum >= vnum) {
			if (object->shared->vnum == vnum)
				return (object);
			else
				return NULL;
		}
	return (NULL);*/

    return objectPrototypes.find(vnum);
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

void
update_unique_id(void)
{
	if (unique_id_changed) {
		sql_exec("select setval('unique_id', %ld)", top_unique_id);
		unique_id_changed = false;
	}
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
get_mail_file_path( long id )
{
    return tmp_sprintf( "players/mail/%0ld/%ld.dat", (id % 10), id );
}

char*
get_player_file_path( long id ) 
{
    return tmp_sprintf( "players/character/%0ld/%ld.dat", (id % 10), id );
}

char*
get_account_file_path( long id ) 
{
    return tmp_sprintf( "players/accounts/%0ld/%ld.acct", (id % 10), id );
}

char*
get_equipment_file_path( long id ) 
{
    return tmp_sprintf( "players/equipment/%0ld/%ld.dat", (id % 10), id );
}

char*
get_corpse_file_path( long id ) 
{
    return tmp_sprintf( "players/corpses/%0ld/%ld.dat", (id % 10), id );
}

bool
sql_exec(const char *str, ...)
{
	PGresult *res;
	char *query;
	va_list args;
	bool result;

	if (!str || !*str)
		return false;

	va_start(args, str);
	query = tmp_vsprintf(str, args);
	va_end(args);

	res = PQexec(sql_cxn, query);
	if (!res) {
		errlog("FATAL: Couldn't allocate sql result");
		safe_exit(1);
	}
	result = PQresultStatus(res) == PGRES_COMMAND_OK
		|| PQresultStatus(res) == PGRES_TUPLES_OK;
	if (!result) {
		errlog("FATAL: sql command generated error: %s",
			PQresultErrorMessage(res));
		errlog("FROM SQL: %s", query);
		raise(SIGSEGV);
	}
	PQclear(res);
	return result;
}

Oid
sql_insert(const char *str, ...)
{
	PGresult *res;
	char *query;
	va_list args;
	Oid result;

	if (!str || !*str)
		return InvalidOid;

	va_start(args, str);
	query = tmp_vsprintf(str, args);
	va_end(args);

	res = PQexec(sql_cxn, query);
	if (!res) {
		errlog("FATAL: Couldn't allocate sql result");
		safe_exit(1);
	}
	result = PQoidValue(res);
	PQclear(res);
	return result;
}

PGresult *
sql_query(const char *str, ...)
{
	PGresult *res;
	char *query;
	va_list args;

	if (!str || !*str)
		return NULL;

	va_start(args, str);
	query = tmp_vsprintf(str, args);
	va_end(args);

	res = PQexec(sql_cxn, query);
	if (!res) {
		errlog("FATAL: Couldn't allocate sql result");
		safe_exit(1);
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		slog("WARNING: sql expression generated error: %s",
			PQresultErrorMessage(res));
		slog("FROM SQL: %s", query);
	}

	sql_query_data *rec = new sql_query_data;
	rec->next = sql_query_list;
	rec->res = res;
	sql_query_list = rec;

	return res;
}

void
sql_gc_queries(void)
{
	sql_query_data *cur_query, *next_query;

	for (cur_query = sql_query_list;cur_query;cur_query = next_query) {
		next_query = cur_query->next;
		PQclear(cur_query->res);
		delete cur_query;
	}

	sql_query_list = NULL;
}

#undef __db_c__
