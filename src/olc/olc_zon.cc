//
// File: olc_zon.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "security.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "paths.h"
#include "player_table.h"

#define NUM_ZSET_COMMANDS 13

const char *door_flags[] = {
	"OPEN",
	"CLOSED",
	"LOCKED",
	"HIDDEN",
	"\n"
};


const char *olc_zset_keys[] = {
	"name",
	"lifespan",
	"top",
	"reset",
	"tframe",
	"plane",				/** 5**/
	"owner",
	"flags",
	"command",				/** 8 **/
	"hours",
	"years",
	"blanket_exp",			 /** 11 **/
	"co-owner",
	"blanket_flags",
	"respawn_pt",
	"min_lvl",		 /** 15 **/
	"min_gen",
	"max_lvl",
	"max_gen",
	"public_desc",
	"private_desc", /** 20 **/
	"\n"
};

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern int top_of_zone_table;
extern int olc_lock;

extern const char *fill_words[];

long asciiflag_conv(char *buf);
static char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

char *one_argument_no_lower(char *argument, char *first_arg);
int search_block_no_lower(char *arg, const char **list, bool exact);
int fill_word_no_lower(char *argument);
void num2str(char *str, int num);
void do_stat_object(struct Creature *ch, struct obj_data *obj);
void do_zone_cmdlist(struct Creature *ch, struct zone_data *zone, char *arg);
void do_zone_cmdrem(struct Creature *ch, struct zone_data *zone, int num);
void do_zone_cmdmove(struct Creature *ch, struct zone_data *zone,
	char *argument);

void
do_zcmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone = NULL, *tmp_zone;
	struct reset_com *zonecmd, *tmp_zonecmd, *zcmd = NULL;
	struct room_data *room;
	char command[2];
	int cur_door_flags, tmp_door_flags, tmp_flag;
	int i, line, found, if_flag, int_arg1, int_arg2, int_arg3, int_arg4;

	argument = one_argument(argument, arg1);

	if (is_number(arg1)) {
		i = atoi(arg1);

		for (found = 0, tmp_zone = zone_table; tmp_zone && found != 1;
			tmp_zone = tmp_zone->next)
			if (tmp_zone->number == i) {
				zone = tmp_zone;
				found = 1;
			}

		if (found != 1) {
			send_to_char(ch, "Invalid zone number.\r\n");
			return;
		}
		argument = one_argument(argument, arg2);
	} else {
		zone = ch->in_room->zone;
		strcpy(arg2, arg1);
	}

	if (is_abbrev(arg2, "list")) {
		skip_spaces(&argument);
		do_zone_cmdlist(ch, zone, argument);
		return;
	}

	if (is_abbrev(arg2, "cmdrenumber")) {
		for (i = 0, zonecmd = zone->cmd; zonecmd; i++, zonecmd = zonecmd->next)
			zonecmd->line = i;
		send_to_char(ch, "Zonecmds renumbered.\r\n");
		return;
	}


	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, 
			"You looking to getting a BEAT-DOWN?!  Permission denied.\r\n");
		return;
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	strcpy(command, arg2);

	if (is_abbrev(arg2, "cmdremove")) {
		skip_spaces(&argument);
		if (!*argument || !is_number(argument)) {
			send_to_char(ch, "Usage: olc zcmd cmdrem <NUMBER>\r\n");
			return;
		}
		i = atoi(argument);
		do_zone_cmdrem(ch, zone, i);
		return;
	} else if (!strncmp(arg2, "move", 4)) {
		do_zone_cmdmove(ch, zone, argument);
		return;
	} else
		switch (*arg2) {
		case 'm':
		case 'M':
#define ZCMD_M_USAGE "Usage: olc zcmd [zone] M <if_flag> <mob vnum> <max loaded> <room vnum> <prob>\r\n"
			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_mobile_proto(int_arg1)) {
					send_to_char(ch, "Mobile (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if ((room = real_room(int_arg3)) == NULL) {
					send_to_char(ch, "Room (V) %d does not exist, buttmunch.\r\n",
						int_arg3);
					return;
				}
				if (!CAN_EDIT_ZONE(ch, room->zone)) {
					send_to_char(ch, 
						"Let's not load mobs in other ppl's zones, shall we?\r\n");
					return;
				}
			} else {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_M_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'M';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;
			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					++line, tmp_zonecmd = tmp_zonecmd->next) {
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
				}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'o':
		case 'O':
			argument = two_arguments(argument, arg1, arg2);

#define ZCMD_O_USAGE "Usage: olc zcmd [zone] O <if_flag> <obj vnum> <max loaded> <room vnum> <prob>\r\n"
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if ((room = real_room(int_arg3)) == NULL) {
					send_to_char(ch, "Room (V) %d does not exist, buttmunch.\r\n",
						int_arg3);
					return;
				}
				if (!CAN_EDIT_ZONE(ch, room->zone)) {
					send_to_char(ch, 
						"Let's not load objs in other ppl's zones, shall we?\r\n");
					return;
				}
			} else {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_O_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'O';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					++line, tmp_zonecmd = tmp_zonecmd->next) {
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
				}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'p':
		case 'P':
			argument = two_arguments(argument, arg1, arg2);

#define ZCMD_P_USAGE "Usage: olc zcmd [zone] P <if_flag> <obj vnum> <max loaded> <obj vnum> <prob>\r\n"
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if (!real_object_proto(int_arg3)) {
					send_to_char(ch, 
						"Object (V) %d does not exist, buttmunch.\r\n",
						int_arg3);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_P_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'P';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd; ++line,
					tmp_zonecmd = tmp_zonecmd->next)
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'g':
		case 'G':
			argument = two_arguments(argument, arg1, arg2);

#define ZCMD_G_USAGE "Usage: olc zcmd [zone] G <if_flag> <obj vnum> <max loaded> <mob vnum> <prob>\r\n"
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if (!real_mobile_proto(int_arg3)) {
					send_to_char(ch,
						"Mobile (V) %d does not exist, buttmunch.\r\n",
						int_arg3);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_G_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'G';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					++line, tmp_zonecmd = tmp_zonecmd->next) {
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
				}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'e':
		case 'E':
#define ZCMD_E_USAGE "Usage: olc zcmd [zone] E <if_flag> <obj vnum> <max loaded> <wear pos> <mob vnum> <prob>\r\n"
			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			// Maxload and Wear Position 
			argument = two_arguments(argument, arg1, arg2);
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if (int_arg3 < 0 || int_arg3 > NUM_WEARS) {
					send_to_char(ch, "Invalid wear position, %d, must be 0-27\r\n",
						int_arg3);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}

			// Target Mobile
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (!real_mobile_proto(i)) {
					send_to_char(ch, "Mobile (V) %d does not exist.\r\n", i);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_E_USAGE);
				return;
			}
			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'E';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					++line, tmp_zonecmd = tmp_zonecmd->next) {
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
				}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'i':
		case 'I':
			argument = two_arguments(argument, arg1, arg2);

#define ZCMD_I_USAGE "Usage: olc zcmd [zone] I <if_flag> <obj vnum> <max loaded> <implant pos> <mob vnum><prob>\r\n"
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or 1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				int_arg2 = atoi(arg1);
				if (int_arg2 < 1 || int_arg2 > 1000) {
					send_to_char(ch, 
						"Number loaded must be between 1 and 1000.\r\n");
					return;
				}
				int_arg3 = atoi(arg2);
				if (int_arg3 < 0 || int_arg3 > NUM_WEARS) {
					send_to_char(ch,
						"Invalid implant position, %d, must be 0-27\r\n",
						int_arg3);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			argument = one_argument(argument, arg1);

			if (!*arg1) {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			if (is_number(arg1)) {
				i = atoi(arg1);
				if (!real_mobile_proto(i)) {
					send_to_char(ch, "Mobile (V) %d does not exist.\r\n", i);
					return;
				}

			} else {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}
			// Probability
			argument = one_argument(argument, arg1);
			if (!*arg1) {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}
			if (is_number(arg1)) {
				i = atoi(arg1);
				if (i > 100 || i < 0) {
					send_to_char(ch, "Invalid probability: %d\r\n", i);
					return;
				}
				int_arg4 = i;
			} else {
				send_to_char(ch, ZCMD_I_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'I';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = int_arg3;
			zonecmd->prob = int_arg4;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					++line, tmp_zonecmd = tmp_zonecmd->next) {
					if (!tmp_zonecmd->next) {
						zonecmd->line = ++line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
				}
			} else {
				zonecmd->line = 0;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'r':
		case 'R':
			argument = two_arguments(argument, arg1, arg2);

#define ZCMD_R_USAGE "Usage: olc zcmd [zone] R <if_flag> <obj vnum> <room vnum>\r\n"
			if (!*arg1 || !*arg2) {
				send_to_char(ch, ZCMD_R_USAGE);
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if (!real_object_proto(int_arg1)) {
					send_to_char(ch, "Object (V) %d does not exist.\r\n",
						int_arg1);
					return;
				}
			} else {
				send_to_char(ch, ZCMD_R_USAGE);
				return;
			}

			argument = one_argument(argument, arg1);

			if (!*arg1) {
				send_to_char(ch, ZCMD_R_USAGE);
				return;
			}

			if (is_number(arg1)) {
				int_arg2 = atoi(arg1);
				if ((room = real_room(int_arg1)) == NULL) {
					send_to_char(ch, "Room (V) %d does not exist, buttmunch.\r\n",
						int_arg1);
					return;
				}
				if (!CAN_EDIT_ZONE(ch, room->zone)) {
					send_to_char(ch, 
						"Let's not remove objs from other ppl's zones, asshole.\r\n");
					return;
				}
			} else {
				send_to_char(ch, ZCMD_R_USAGE);
				return;
			}

			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'R';
			zonecmd->if_flag = if_flag;
			zonecmd->arg1 = int_arg1;
			zonecmd->arg2 = int_arg2;
			zonecmd->arg3 = 0;
			zonecmd->prob = 100;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd; line++,
					tmp_zonecmd = tmp_zonecmd->next)
					if (!tmp_zonecmd->next) {
						zonecmd->line = line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
			} else {
				zonecmd->line = 1;
				zone->cmd = zonecmd;
			}

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Command completed ok.\r\n");

			break;
		case 'd':
		case 'D':
			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, 
					"Usage: olc zcmd [zone] D <if_flag> <room vnum> <direction> [+/-] <FLAG FLAG ...>\r\n");
				return;
			}

			if (is_number(arg1) && is_number(arg2)) {
				if_flag = atoi(arg1);
				if (if_flag != 0 && if_flag != 1 && if_flag != -1) {
					send_to_char(ch, 
						"if_flag dependancy flag must be 0, 1 or -1\r\n");
					return;
				}
				int_arg1 = atoi(arg2);
				if ((room = real_room(int_arg1)) == NULL) {
					send_to_char(ch, "Room (V) %d does not exist.\r\n", int_arg1);
					return;
				}
				if (!CAN_EDIT_ZONE(ch, room->zone)) {
					send_to_char(ch, 
						"Let's not close doors in other ppl's zones, shall we?\r\n");
					return;
				}
			} else {
				send_to_char(ch, 
					"Usage: olc zcmd [zone] D <if_flag> <room vnum> <direction> <FLAG FLAG ...>\r\n");
				return;
			}

			argument = two_arguments(argument, arg1, arg2);

			if (!*arg1 || !*arg2) {
				send_to_char(ch, 
					"Usage: olc zcmd [zone] D <if_flag> <room vnum> <direction> <FLAG FLAG ...>\r\n");
				return;
			}

			if ((int_arg2 = search_block(arg1, dirs, FALSE)) < 0) {
				send_to_char(ch, 
					"You must supply a valid direction of the door.\r\n");
				return;
			}

			if (room->dir_option[int_arg2] == NULL) {
				send_to_char(ch, "Room #%d does not have a door leading %s\r\n",
					int_arg1, arg1);
				return;
			}

			tmp_door_flags = 0;

			while (*arg2) {
				if ((tmp_flag = search_block(arg2, door_flags, FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg2);
				} else
					tmp_door_flags = tmp_door_flags | (1 << tmp_flag);

				argument = one_argument(argument, arg2);
			}

			for (found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd && found != 1;
				tmp_zonecmd = tmp_zonecmd->next)
				if (tmp_zonecmd->command == 'D'
					&& tmp_zonecmd->arg1 == int_arg1
					&& tmp_zonecmd->arg2 == int_arg2) {
					cur_door_flags = tmp_zonecmd->arg3;
					found = 1;
					zcmd = tmp_zonecmd;
				}

			cur_door_flags = tmp_door_flags;

			if (found != 1) {
				CREATE(zonecmd, struct reset_com, 1);

				zonecmd->command = 'D';
				zonecmd->if_flag = if_flag;
				zonecmd->arg1 = int_arg1;
				zonecmd->arg2 = int_arg2;
				zonecmd->arg3 = cur_door_flags;
				zonecmd->prob = 100;

				zonecmd->next = NULL;

				if (zone->cmd) {
					for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
						line++, tmp_zonecmd = tmp_zonecmd->next)
						if (!tmp_zonecmd->next) {
							zonecmd->line = line;
							tmp_zonecmd->next = zonecmd;
							break;
						}
				} else {
					zonecmd->line = 1;
					zone->cmd = zonecmd;
				}
			} else
				zcmd->arg3 = cur_door_flags;

			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Door flags set.\r\n");
			break;

		default:
			send_to_char(ch, "Invalid zone command %s.\r\n", arg2);
			return;
		}
}

void
do_zone_cmdmove(struct Creature *ch, struct zone_data *zone, char *argument)
{
	struct reset_com *first = NULL, *tmp_zonecmd = NULL, *savecmd1 = NULL,
		*savecmd2 = NULL;
	int i, num, where;

	argument = two_arguments(argument, arg1, arg2);

	if (is_number(arg1) && is_number(arg2)) {
		num = atoi(arg1);
		where = atoi(arg2);

		if (!zone->cmd) {
			send_to_char(ch, 
				"Why not get some zcmds before trying to move them... SHEESH!!\r\n");
			return;
		}

		if (num == where) {
			send_to_char(ch, "You're pretty funny.\r\n");
			return;
		}

		if (where == num + 1) {
			send_to_char(ch, 
				"Moving the command one down will have no effect.\r\n");
			return;
		}

		if (!zone->cmd->next) {
			send_to_char(ch, 
				"WHAT??  You've only got one command, and its number is ZERO.\r\n");
			return;
		}

		for (first = zone->cmd, savecmd1 = zone->cmd, i = 0;
			first; savecmd1 = first, first = first->next, i++)
			if (i == num)
				break;

		if (!first) {
			send_to_char(ch, "There is no command number %d in zone %d, fool.\r\n",
				num, zone->number);
			return;
		}

		for (savecmd2 = zone->cmd, tmp_zonecmd = zone->cmd, i = 0;
			tmp_zonecmd;
			i++, savecmd2 = tmp_zonecmd, tmp_zonecmd = tmp_zonecmd->next)
			if (i == where)
				break;

		if (!tmp_zonecmd) {
			if (i == where)
				send_to_char(ch, "Moving command to the end of the list.\r\n");
			else {
				send_to_char(ch,
					"There is no command number %d in zone %d.  Moving cmd %d to pos %d.\r\n",
					where, zone->number, num, i);
				where = i;
			}
		}

		if (first == zone->cmd)
			zone->cmd = first->next;
		else
			savecmd1->next = first->next;

		first->next = tmp_zonecmd;

		if (tmp_zonecmd == zone->cmd)
			zone->cmd = first;
		else
			savecmd2->next = first;

		for (tmp_zonecmd = zone->cmd, i = 0; tmp_zonecmd;
			i++, tmp_zonecmd = tmp_zonecmd->next)
			tmp_zonecmd->line = i;

		send_to_char(ch, "Move completed, zone command %d to position %d\r\n", num,
			where);
	} else {
		send_to_char(ch, "Usage olc zcmd move <zone cmd> <position>\r\n");
		return;
	}
}

#define ZMOB_USAGE "Usage: olc zmob <mob vnum> <max existing>\n"

void
do_zmob_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct reset_com *zonecmd, *tmp_zonecmd;
	struct Creature *mob;
	int int_arg1, int_arg2, line, prob = 100;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZMOB_USAGE);
		return;
	}

	if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, ZMOB_USAGE);
		return;
	}

	if (is_number(arg1) && is_number(arg2)) {
		int_arg1 = atoi(arg1);
		if (!real_mobile_proto(int_arg1)) {
			send_to_char(ch, "Mobile (V) %d does not exist, buttmunch.\r\n",
				int_arg1);
			return;
		}
		int_arg2 = atoi(arg2);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZMOB_USAGE);
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'M';
	zonecmd->if_flag = 0;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = ch->in_room->number;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	zone = ch->in_room->zone;

	if (zone->cmd) {
		for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			line++, tmp_zonecmd = tmp_zonecmd->next)
			if (!tmp_zonecmd->next) {
				zonecmd->line = line;
				tmp_zonecmd->next = zonecmd;
				break;
			}
	} else {
		zonecmd->line = 1;
		zone->cmd = zonecmd;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	send_to_char(ch, "Command completed ok.\r\n");
	mob = read_mobile(int_arg1);
	char_to_room(mob, ch->in_room,false);

}

#define ZPUT_USAGE "Usage: olc zput <obj name> <obj vnum> <max loaded> [prob]\r\n"

void
do_zput_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone = NULL;
	struct reset_com *zonecmd, *tmp_zonecmd;
	int int_arg1, int_arg2, int_arg3, line, found = 0, prob = 100;
	struct obj_data *to_obj = NULL, *obj = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZPUT_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if ((to_obj =
			get_obj_in_list_vis(ch, arg1, ch->in_room->contents)) == NULL) {
		send_to_char(ch, "Cannot find that object in this room.\r\n");
		return;
	}

	int_arg3 = GET_OBJ_VNUM(to_obj);

	if (is_number(arg2)) {
		int_arg1 = atoi(arg2);
		if (!(obj = real_object_proto(int_arg3))) {
			send_to_char(ch, "Object (V) %d does not exist.\r\n", int_arg3);
			return;
		}
	} else {
		send_to_char(ch, ZPUT_USAGE);
		return;
	}

	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, ZPUT_USAGE);
		return;
	}

	if (is_number(arg1)) {
		int_arg2 = atoi(arg1);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZPUT_USAGE);
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'P';
	zonecmd->if_flag = 1;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = int_arg3;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	zone = ch->in_room->zone;

	if (zone->cmd) {
		for (line = 0, found = 0, tmp_zonecmd = zone->cmd;
			tmp_zonecmd; line++, tmp_zonecmd = tmp_zonecmd->next) {
			if (found)
				tmp_zonecmd->line++;
			if (!found && tmp_zonecmd->command == 'O' &&
				tmp_zonecmd->arg1 == int_arg3) {
				zonecmd->line = line;
				zonecmd->next = tmp_zonecmd->next;
				tmp_zonecmd->next = zonecmd;
				found = 1;
			}
		}
		if (found == 0) {
			send_to_char(ch,
				"Zone command O required for %d before this can be set.\r\n",
				int_arg3);
			free(zonecmd);
			return;
		}
	} else {
		send_to_char(ch,
			"Zone command O required for %d before this can be set.\r\n",
			int_arg3);
		free(zonecmd);
		return;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	if (obj->shared->number - obj->shared->house_count < zonecmd->arg2) {
		if ((obj = read_object(GET_OBJ_VNUM(obj)))) {
			if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED))
				SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
			obj_to_obj(obj, to_obj);
		} else
			slog("SYSERR: Freaky-ass error in zput!");
	}
	send_to_char(ch, "Command completed ok.\r\n");
}

#define ZGIVE_USAGE "Usage: olc zgive <mob name> <obj vnum> <max loaded> [prob]\r\n"

void
do_zgive_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct reset_com *zonecmd, *tmp_zonecmd;
	int int_arg1, int_arg2, int_arg3, line, found = 0, prob = 100;
	struct Creature *mob;
	struct obj_data *obj = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZGIVE_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if ((mob = get_char_room_vis(ch, arg1)) == NULL) {
		send_to_char(ch, "Cannot find that mobile in this room.\r\n");
		return;
	}

	if (mob == ch || !IS_NPC(mob)) {
		send_to_char(ch, "You're pretty funny.\r\n");
		return;
	}

	int_arg3 = mob->mob_specials.shared->vnum;

	if (is_number(arg2)) {
		int_arg1 = atoi(arg2);
		if (!(obj = real_object_proto(int_arg1))) {
			send_to_char(ch, "Object (V) %d does not exist.\r\n", int_arg1);
			return;
		}
	} else {
		send_to_char(ch, ZGIVE_USAGE);
		return;
	}

	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, ZGIVE_USAGE);
		return;
	}

	if (is_number(arg1)) {
		int_arg2 = atoi(arg1);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZGIVE_USAGE);
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'G';
	zonecmd->if_flag = 1;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = int_arg3;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	if (zone->cmd) {
		for (line = 0, found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			line++, tmp_zonecmd = tmp_zonecmd->next) {
			if (found)
				tmp_zonecmd->line++;
			else if (tmp_zonecmd->command == 'M'
				&& tmp_zonecmd->arg1 == int_arg3) {
				zonecmd->line = tmp_zonecmd->line;
				zonecmd->next = tmp_zonecmd->next;
				tmp_zonecmd->next = zonecmd;
				found = 1;
			}
		}
		if (found == 0) {
			send_to_char(ch,
				"Zone command M required for %d before this can be set.\r\n",
				int_arg3);
			free(zonecmd);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			return;
		}
	} else {
		send_to_char(ch,
			"Zone command M required for %d before this can be set.\r\n",
			int_arg3);
		free(zonecmd);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		return;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	if (obj->shared->number - obj->shared->house_count < zonecmd->arg3) {
		obj = read_object(int_arg1);
		if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED))
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
		obj_to_char(obj, mob);
	}
	send_to_char(ch, "Command completed ok.\r\n");
}



#define ZIMPLANT_USAGE "Usage: olc zimplant <mob name> <obj vnum> <max loaded> <implant pos> [prob]\r\n"

void
do_zimplant_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct reset_com *zonecmd, *tmp_zonecmd;
	int int_arg1, int_arg2, int_arg3, line, found = 0, prob = 100;
	struct Creature *mob;
	struct obj_data *obj;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZIMPLANT_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if ((mob = get_char_room_vis(ch, arg1)) == NULL) {
		send_to_char(ch, "Cannot find that mobile in this room.\r\n");
		return;
	}

	if (mob == ch || !IS_NPC(mob)) {
		send_to_char(ch, "You're pretty funny.\r\n");
		return;
	}

	if (is_number(arg2)) {
		int_arg1 = atoi(arg2);
		if (!(obj = real_object_proto(int_arg1))) {
			send_to_char(ch, "Object (V) %d does not exist.\r\n", int_arg1);
			return;
		}
	} else {
		send_to_char(ch, ZIMPLANT_USAGE);
		return;
	}

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZIMPLANT_USAGE);
		return;
	}

	if (is_number(arg1)) {
		int_arg2 = atoi(arg1);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZIMPLANT_USAGE);
		return;
	}

	if ((int_arg3 = search_block(arg2, wear_implantpos, FALSE)) < 0) {
		send_to_char(ch, "You must supply a valid implant position.\r\n");
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'I';
	zonecmd->if_flag = 1;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = int_arg3;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	if (zone->cmd) {
		for (line = 0, found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			line++, tmp_zonecmd = tmp_zonecmd->next) {
			if (found)
				tmp_zonecmd->line++;
			else if (tmp_zonecmd->command == 'M' &&
				tmp_zonecmd->arg1 == mob->mob_specials.shared->vnum) {
				zonecmd->line = tmp_zonecmd->line;
				zonecmd->next = tmp_zonecmd->next;
				tmp_zonecmd->next = zonecmd;
				found = 1;
			}
		}
		if (found == 0) {
			send_to_char(ch,
				"zmob command required for mobile (V) %d before this can be set.\r\n",
				mob->mob_specials.shared->vnum);
			free(zonecmd);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			return;
		}
	} else {
		send_to_char(ch,
			"zmob command required for mobile (V) %d before this can be set.\r\n",
			mob->mob_specials.shared->vnum);
		free(zonecmd);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		return;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	if (obj->shared->number - obj->shared->house_count <
		zonecmd->arg2 && !mob->equipment[zonecmd->arg3]) {
		obj = read_object(int_arg1);
		equip_char(mob, obj, zonecmd->arg3, MODE_IMPLANT);
	}
	send_to_char(ch, "Command completed ok.\r\n");
}


#define ZEQUIP_USAGE "Usage: olc zequip <mob name> <obj vnum> <max loaded>"\
" <wear pos> [prob]\r\n"

void
do_zequip_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct reset_com *zonecmd, *tmp_zonecmd;
	int int_arg1, int_arg2, int_arg3, line, found = 0, prob = 100;
	struct Creature *mob;
	struct obj_data *obj;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZEQUIP_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if ((mob = get_char_room_vis(ch, arg1)) == NULL) {
		send_to_char(ch, "Cannot find that mobile in this room.\r\n");
		return;
	}

	if (mob == ch || !IS_NPC(mob)) {
		send_to_char(ch, "You're pretty funny.\r\n");
		return;
	}

	if (is_number(arg2)) {
		int_arg1 = atoi(arg2);
		if (!(obj = real_object_proto(int_arg1))) {
			send_to_char(ch, "Object (V) %d does not exist.\r\n", int_arg1);
			return;
		}
	} else {
		send_to_char(ch, ZEQUIP_USAGE);
		return;
	}

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZEQUIP_USAGE);
		return;
	}

	if (is_number(arg1)) {
		int_arg2 = atoi(arg1);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZEQUIP_USAGE);
		return;
	}

	if ((int_arg3 = search_block(arg2, wear_eqpos, FALSE)) < 0) {
		send_to_char(ch, "You must supply a valid wear position.\r\n");
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'E';
	zonecmd->if_flag = 1;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = int_arg3;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	if (zone->cmd) {
		for (line = 0, found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			line++, tmp_zonecmd = tmp_zonecmd->next) {
			if (found)
				tmp_zonecmd->line++;
			else if (tmp_zonecmd->command == 'M' &&
				tmp_zonecmd->arg1 == mob->mob_specials.shared->vnum) {
				zonecmd->line = tmp_zonecmd->line;
				zonecmd->next = tmp_zonecmd->next;
				tmp_zonecmd->next = zonecmd;
				found = 1;
			}
		}
		if (found == 0) {
			send_to_char(ch,
				"zmob command required for mobile (V) %d before this can be set.\r\n",
				mob->mob_specials.shared->vnum);
			free(zonecmd);
			return;
		}
	} else {
		send_to_char(ch,
			"zmob command required for mobile (V) %d before this can be set.\r\n",
			mob->mob_specials.shared->vnum);
		free(zonecmd);
		return;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	if (obj->shared->number - obj->shared->house_count <
		zonecmd->arg2 && !mob->equipment[zonecmd->arg3]) {
		obj = read_object(int_arg1);
		equip_char(mob, obj, zonecmd->arg3, MODE_EQ);
	}
	send_to_char(ch, "Command completed ok.\r\n");
}

#define ZOBJ_USAGE "Usage: olc zobj <obj vnum> <max loaded> [prob]\r\n"

void
do_zobj_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct obj_data *obj;
	struct reset_com *zonecmd, *tmp_zonecmd, *rzonecmd;
	int int_arg1, int_arg2, line, prob = 100;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, ZOBJ_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if (is_number(arg1) && is_number(arg2)) {
		int_arg1 = atoi(arg1);
		if (!real_object_proto(int_arg1)) {
			send_to_char(ch, "Object (V) %d does not exist, buttmunch.\r\n",
				int_arg1);
			return;
		}
		int_arg2 = atoi(arg2);
		if (int_arg2 < 1 || int_arg2 > 1000) {
			send_to_char(ch, "Number loaded must be between 1 and 1000.\r\n");
			return;
		}
	} else {
		send_to_char(ch, ZOBJ_USAGE);
		return;
	}

	if (*argument) {
		skip_spaces(&argument);
		prob = atoi(argument);
		prob = MIN(MAX(0, prob), 100);
	}

	CREATE(zonecmd, struct reset_com, 1);

	zonecmd->command = 'O';
	zonecmd->if_flag = 0;
	zonecmd->arg1 = int_arg1;
	zonecmd->arg2 = int_arg2;
	zonecmd->arg3 = ch->in_room->number;
	zonecmd->prob = prob;

	zonecmd->next = NULL;

	CREATE(rzonecmd, struct reset_com, 1);

	rzonecmd->command = 'R';
	rzonecmd->if_flag = 0;
	rzonecmd->arg1 = int_arg1;
	rzonecmd->arg2 = ch->in_room->number;
	rzonecmd->arg3 = 0;
	rzonecmd->prob = prob;

	rzonecmd->next = zonecmd;


	zone = ch->in_room->zone;

	if (zone->cmd) {
		for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			line++, tmp_zonecmd = tmp_zonecmd->next)
			if (!tmp_zonecmd->next) {
				rzonecmd->line = line;
				rzonecmd->next->line = line + 1;
				tmp_zonecmd->next = rzonecmd;
				break;
			}
	} else {
		rzonecmd->line = 1;
		rzonecmd->next->line = 2;
		zone->cmd = rzonecmd;
	}

	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	send_to_char(ch, "Command completed ok.\r\n");
	obj = read_object(int_arg1);
	if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED))
		SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
	obj_to_room(obj, ch->in_room);
}

void
do_zdoor_cmd(struct Creature *ch, char *argument)
{
	struct zone_data *zone;
	struct reset_com *zonecmd, *tmp_zonecmd, *zcmd = NULL;
	struct room_data *other_rm = NULL;
	int cur_door_flags, tmp_door_flags, int_arg1, int_arg2, tmp_flag;
	int found, line, rev_room_vnum = -1;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, "Usage: olc zdoor <direction> <FLAG FLAG ...>\r\n");
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if ((int_arg2 = search_block(arg1, dirs, FALSE)) < 0) {
		send_to_char(ch, "You must supply a valid direction of the door.\r\n");
		return;
	}

	if (ch->in_room->dir_option[int_arg2] == NULL) {
		send_to_char(ch, "This room does not have a door leading %s.\r\n", arg1);
		return;
	}

	if (!*arg2) {
		send_to_char(ch, "Usage: olc zdoor <direction> <FLAG FLAG ...>\r\n");
		return;
	}

	if ((other_rm = ch->in_room->dir_option[int_arg2]->to_room) &&
		other_rm->dir_option[rev_dir[int_arg2]] &&
		ch->in_room == other_rm->dir_option[rev_dir[int_arg2]]->to_room)
		rev_room_vnum = other_rm->number;


	int_arg1 = ch->in_room->number;

	tmp_door_flags = 0;

	while (*arg2) {
		if ((tmp_flag = search_block(arg2, door_flags, FALSE)) == -1) {
			send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg2);
		} else
			tmp_door_flags = tmp_door_flags | (1 << tmp_flag);

		argument = one_argument(argument, arg2);
	}

	zone = ch->in_room->zone;

	for (found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd && found != 1;
		tmp_zonecmd = tmp_zonecmd->next)
		if (tmp_zonecmd->command == 'D' && tmp_zonecmd->arg1 == int_arg1 &&
			tmp_zonecmd->arg2 == int_arg2) {
			found = 1;
			zcmd = tmp_zonecmd;
		}

	cur_door_flags = tmp_door_flags;

	if (found != 1) {
		CREATE(zonecmd, struct reset_com, 1);

		zonecmd->command = 'D';
		zonecmd->if_flag = 0;;
		zonecmd->arg1 = int_arg1;
		zonecmd->arg2 = int_arg2;
		zonecmd->arg3 = cur_door_flags;
		zonecmd->prob = 100;

		zonecmd->next = NULL;

		if (zone->cmd) {
			for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
				line++, tmp_zonecmd = tmp_zonecmd->next)
				if (!tmp_zonecmd->next) {
					zonecmd->line = line;
					tmp_zonecmd->next = zonecmd;
					break;
				}
		} else {
			zonecmd->line = 1;
			zone->cmd = zonecmd;
		}
	} else
		zcmd->arg3 = cur_door_flags;

	/* Rev door */
	if (rev_room_vnum != -1) {
		for (found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd && found != 1;
			tmp_zonecmd = tmp_zonecmd->next)
			if (tmp_zonecmd->command == 'D'
				&& tmp_zonecmd->arg1 == rev_room_vnum
				&& tmp_zonecmd->arg2 == rev_dir[int_arg2]) {
				found = 1;
				zcmd = tmp_zonecmd;
			}

		if (found != 1) {
			zonecmd = NULL;
			CREATE(zonecmd, struct reset_com, 1);

			zonecmd->command = 'D';
			zonecmd->if_flag = 1;
			zonecmd->arg1 = rev_room_vnum;
			zonecmd->arg2 = rev_dir[int_arg2];
			zonecmd->arg3 = cur_door_flags;
			zonecmd->prob = 100;

			zonecmd->next = NULL;

			if (zone->cmd) {
				for (line = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
					line++, tmp_zonecmd = tmp_zonecmd->next)
					if (!tmp_zonecmd->next) {
						zonecmd->line = line;
						tmp_zonecmd->next = zonecmd;
						break;
					}
			} else {
				zonecmd->line = 1;
				zone->cmd = zonecmd;
			}
		} else
			zcmd->arg3 = cur_door_flags;
	}


	SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
	send_to_char(ch, "Door flags set.\r\n");

}

#define ZSET_COMMAND_USAGE  "Usage: olc zset [zone] command <cmd num> [if|max|prob] <value>\r\n"
extern const char *zone_flag_names[];
void
do_zset_command(struct Creature *ch, char *argument)
{
	struct zone_data *zone = NULL, *tmp_zone;
	struct reset_com *tmp_zonecmd;
	int cmd, i, j, k, found, timeframe, plane, zset_command, state;
	int tmp_zone_flags, cur_zone_flags, tmp_flag;
	struct Creature *vict = NULL;
	int num;

	argument = one_argument(argument, arg1);

	if (is_number(arg1)) {
		i = atoi(arg1);

		for (found = 0, tmp_zone = zone_table; tmp_zone && found != 1;
			tmp_zone = tmp_zone->next)
			if (tmp_zone->number == i) {
				zone = tmp_zone;
				found = 1;
			}

		if (found != 1) {
			send_to_char(ch, "Invalid zone number.\r\n");
			return;
		}
		argument = one_argument(argument, arg2);
	} else {
		zone = ch->in_room->zone;
		strcpy(arg2, arg1);
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "You are currently unable to set flags on this zone.\r\n");
		return;
	}


	if ((zset_command = search_block(arg2, olc_zset_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid zset command '%s'.\r\n", arg2);
		return;
	}

	skip_spaces(&argument);

	if (!*argument && zset_command != 19 && zset_command != 20) {
		send_to_char(ch, "You must supply a value to %s\r\n",
			olc_zset_keys[zset_command]);
		if (zset_command == 8)
			send_to_char(ch, ZSET_COMMAND_USAGE);
		return;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	switch (zset_command) {
	case 0:					/*     name  */
		if (zone->name)
			free(zone->name);
		zone->name = strdup(argument);
		send_to_char(ch, "Zone %d name set to: %s\r\n", zone->number, zone->name);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 1:					/*   life   */
		i = atoi(argument);
		zone->lifespan = i;
		send_to_char(ch, "Zone %d lifespan set to: %d\r\n",
			zone->number, zone->lifespan);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 2:					/*  top   */
		if(! Security::isMember(ch, "OLCWorldWrite") ) {
			send_to_char(ch, "You cannot alter zones in this way.\r\n");
			return;
		}
		i = atoi(argument);
		if ((zone->next && zone->next->number * 100 <= i) ||
			i < zone->number * 100) {
			send_to_char(ch, "Invalid value for top of zone.\r\n");
			return;
		}
		zone->top = i;
		send_to_char(ch, "Zone %d top of zone set to: %d\r\n",
			zone->number, zone->top);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 3:					/* reset */
		i = atoi(argument);
		if (i < 0 || i > 2 || !is_number(argument)) {
			send_to_char(ch, 
				"Zone reset mode must either be 0, 1, or 2.  Buttmunch.\r\n");
			return;
		}

		zone->reset_mode = i;
		send_to_char(ch, "Zone %d reset mode set to: %d\r\n",
			zone->number, zone->reset_mode);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 4:
		if ((timeframe = search_block(argument, time_frames, FALSE)) < 0) {
			send_to_char(ch, "Invalid time frame '%s'.\r\n", argument);
			return;
		}

		zone->time_frame = timeframe;
		send_to_char(ch, "Zone %d timeframe set to: %s\r\n", zone->number,
			argument);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 5:
		if ((plane = search_block(argument, planes, FALSE)) < 0) {
			send_to_char(ch, "Invalid plane: %s\r\n", argument);
			return;
		}

		zone->plane = plane;
		send_to_char(ch, "Zone %d plane set to: %s\r\n", zone->number, argument);
		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		return;
		break;
	case 6:
		if (strcmp(argument, "none") == 0) {
			zone->owner_idnum = -1;
			send_to_char(ch, "Zone owner set to: None\r\n");
			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		} else {
			if ((zone->owner_idnum = playerIndex.getID(argument)) < 0) {
				send_to_char(ch, "No such player in the file.\r\n");
				return;
			} else {
				send_to_char(ch, "Zone %d owner set to: %s\r\n", zone->number,
					argument);
				SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			}
		}
		break;
	case 7:					// flags
		tmp_zone_flags = 0;
		argument = one_argument(argument, arg1);
		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, 
				"Usage: olc zset [zone] flags [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_zone_flags = zone->flags;

		while (*arg1) {
			if ((tmp_flag = search_block(arg1, zone_flag_names, FALSE)) == -1 ||
				(tmp_flag >= NUM_ZONE_FLAGS && !OLCIMP(ch)) ||
				(tmp_flag == ZONE_FULLCONTROL && !OLCIMP(ch))) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_zone_flags = tmp_zone_flags | (1 << tmp_flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_zone_flags = cur_zone_flags | tmp_zone_flags;
		else {
			tmp_zone_flags = cur_zone_flags & tmp_zone_flags;
			cur_zone_flags = cur_zone_flags ^ tmp_zone_flags;
		}

		zone->flags = cur_zone_flags;

		if (tmp_zone_flags == 0 && cur_zone_flags == 0) {
			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Zone flags set to: None\r\n");
		} else if (tmp_zone_flags == 0)
			send_to_char(ch, "Zone flags not altered.\r\n");
		else {
			SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			send_to_char(ch, "Zone flags set.\r\n");
		}
		break;
	case 8:					/* zset command */

		argument = two_arguments(argument, arg1, arg2);

		if (!*arg1 || !*arg2) {
			send_to_char(ch, ZSET_COMMAND_USAGE);
			return;
		}

		if (!is_number(arg1)) {
			send_to_char(ch, ZSET_COMMAND_USAGE);
			return;
		}

		cmd = atoi(arg1);

		if (!str_cmp(arg2, "if") && !str_cmp(arg2, "max") &&
			!str_cmp(arg2, "prob")) {
			send_to_char(ch, ZSET_COMMAND_USAGE);
			return;
		}

		one_argument(argument, arg1);

		if (!is_number(arg1)) {
			send_to_char(ch, ZSET_COMMAND_USAGE);
			return;
		}

		state = atoi(arg1);

		if ((str_cmp(arg2, "if") == 0) && (state != 0 && state != 1
				&& state != -1)) {
			send_to_char(ch, "Value for if_flag must be either -1, 0 or 1.\r\n");
			return;
		}

		for (i = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
			i++, tmp_zonecmd = tmp_zonecmd->next)
			if (i == cmd) {
				if (str_cmp(arg2, "if") == 0) {
					tmp_zonecmd->if_flag = state;
					send_to_char(ch, "IF flag of command %d set to %d.\r\n", cmd,
						state);
				} else if (str_cmp(arg2, "prob") == 0) {
					if (state < 0 || state > 100) {
						send_to_char(ch, "'%d' is a silly probability.\r\n",
							state);
					} else {
						tmp_zonecmd->prob = state;
						send_to_char(ch,
							"Probability of execution of command %d set to %d.\r\n",
							cmd, state);
					}
				} else if (tmp_zonecmd->command == 'P' ||
					tmp_zonecmd->command == 'O' ||
					tmp_zonecmd->command == 'E' ||
					tmp_zonecmd->command == 'G' ||
					tmp_zonecmd->command == 'I' ||
					tmp_zonecmd->command == 'M') {
					tmp_zonecmd->arg2 = state;
					send_to_char(ch, "MAX loaded of command %d set to %d.\r\n",
						cmd, state);
				} else
					send_to_char(ch, "That command does not use a max.\r\n");
				break;
			}
		break;
	case 9:					/* hours */
		if (!is_number(argument))
			send_to_char(ch, "You must supply a numerical argument.\r\n");
		else {
			zone->hour_mod = atoi(argument);
			send_to_char(ch, "Hour modifier (longitude) set.\r\n");
		}
		break;
	case 10:					/* years */
		if (!is_number(argument))
			send_to_char(ch, "You must supply a numerical argument.\r\n");
		else {
			zone->year_mod = atoi(argument);
			send_to_char(ch, "Year modifier set.\r\n");
		}
		break;
	case 11:
		{
			int totexp;
			if (!is_number(argument))
				send_to_char(ch, "You must supply a numerical argument.\r\n");
			else if ((i = atoi(argument)) < 0 || i > 900)
				send_to_char(ch, "Modifier out of range [0, 900].\r\n");
			else {
				j = zone->number * 100;
				k = zone->top;
				CreatureList::iterator mit = mobilePrototypes.begin();
				for (; mit != mobilePrototypes.end(); ++mit) {
					vict = *mit;
					if (GET_MOB_VNUM(vict) > k)
						break;
					if (GET_MOB_VNUM(vict) < j)
						continue;
					totexp = mobile_experience(vict);
					totexp += (int)(totexp * (float)i / 100);

					GET_EXP(vict) = totexp;
				}
				send_to_char(ch, 
					"Blanket exp modified.  Don't forget to save.\r\n");
			}
			break;
	case 12:
			if (strcmp(argument, "none") == 0) {
				zone->co_owner_idnum = -1;
				send_to_char(ch, "Zone co-owner set to: None\r\n");
				SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
			} else {
				if ((zone->co_owner_idnum = playerIndex.getID(argument)) < 0) {
					send_to_char(ch, "No such player in the file.\r\n");
					return;
				} else {
					send_to_char(ch, "Zone %d c-owner set to: %s\r\n",
						zone->number, argument);
					SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
				}
			}
			break;
		}

	case 13:					// Blanket Zone flags
		{
			int tmp_flags = 0;
			int tmp_room_flags = 0;
			int top = 0, bottom = 0, count = 0, add_flags = 0;
			struct room_data *room = NULL;

			argument = one_argument(argument, arg1);
			top = zone->top;
			bottom = (zone->number) * 100;



			if (!ZONE_FLAGGED(zone, ZONE_ROOMS_APPROVED)
				&& !ZONE_FLAGGED(zone, ZONE_FULLCONTROL) && !OLCIMP(ch)) {
				send_to_char(ch, 
					"You do not have the appropriate permissions biznitch.\r\n");
				return;
			}

			if (*arg1 == '+')
				add_flags = 1;
			else if (*arg1 == '-')
				add_flags = 0;
			else {
				send_to_char(ch, 
					"Usage: olc zset [zone] blanket_flags [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);
			while (*arg1) {
				if ((tmp_flags =
						search_block(arg1, roomflag_names, FALSE)) == -1
					|| (tmp_flags >= NUM_ROOM_FLAGS && !OLCIMP(ch))) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else
					tmp_room_flags = tmp_room_flags | (1 << tmp_flags);

				if (tmp_room_flags == 0) {
					send_to_char(ch, 
						"No valid flags specified...hard drive crash imminent.\r\n");
					return;
				}
				argument = one_argument(argument, arg1);
			}


			for (count = bottom; count <= top; ++count) {
				room = real_room(count);

				if (room) {
					if (add_flags) {
						room->room_flags = (room->room_flags |=
							tmp_room_flags);
					} else {
						tmp_flags = room->room_flags;
						tmp_room_flags = (tmp_flags & tmp_room_flags);
						room->room_flags = (tmp_flags ^ tmp_room_flags);
					}
					SET_BIT(zone->flags, ZONE_ROOMS_MODIFIED);

				}

			}
			send_to_char(ch, "Cha-Ching flags set!\r\n");
			break;

		}

	case 14:	// respawn point
		if (is_abbrev(argument, "none")) {
			zone->respawn_pt = 0;
			send_to_char(ch, "Zone respawn point cleared\r\n");
		} else if (is_number(argument)) {
			zone->respawn_pt = atoi(argument);
			if (real_room(zone->respawn_pt))
				send_to_char(ch, "Zone respawn point set to room #%d (%s)\r\n",
					zone->respawn_pt, real_room(zone->respawn_pt)->name);
			else
				send_to_char(ch, "Zone respawn point set to nonexistant room #%d\r\n",
					zone->respawn_pt);
		} else
			send_to_char(ch, "You must supply a room number or 'none'.\r\n");
		break;
	default:
		send_to_char(ch, "Unsupported olc zset command.\r\n");
		break;
	case 15:	// minimum target level
		if (!is_number(argument)) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 49.\r\n");
			return;
		}
		num = atoi(argument);
		if (num < 0 || num > 49) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 49.\r\n");
			return;
		}
		zone->min_lvl = atoi(argument);
		send_to_char(ch, "Minimum target player level of zone set.\r\n");
		break;
	case 16:	// minimum target gen
		if (!is_number(argument)) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 10.\r\n");
			return;
		}
		num = atoi(argument);
		if (num < 0 || num > 10) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 10.\r\n");
			return;
		}
		zone->min_gen = atoi(argument);
		send_to_char(ch, "Minimum target gen of zone set.\r\n");
		break;
	case 17:	// maximum target level
		if (!is_number(argument)) {
			send_to_char(ch, "You must supply a numerical argument from 1 to 49.\r\n");
			return;
		}
		num = atoi(argument);
		if (num < 1 || num > 49) {
			send_to_char(ch, "You must supply a numerical argument from 1 to 49.\r\n");
			return;
		}
		zone->max_lvl = atoi(argument);
		send_to_char(ch, "Maximum target player level of zone set.\r\n");
		break;
	case 18:	// maximum target gen
		if (!is_number(argument)) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 10.\r\n");
			return;
		}
		num = atoi(argument);
		if (num < 0 || num > 10) {
			send_to_char(ch, "You must supply a numerical argument from 0 to 10.\r\n");
			return;
		}
		zone->max_gen = atoi(argument);
		send_to_char(ch, "Maximum target gen of zone set.\r\n");
		break;
	case 19:	// public description
		if (zone->public_desc)
			act("$n begins to edit a zone description.", TRUE, ch, 0, 0,
				TO_ROOM);
		else
			act("$n starts to write a zone description.", TRUE, ch, 0, 0,
				TO_ROOM);
		start_text_editor(ch->desc, &zone->public_desc, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		break;
	case 20:	// private description
		if (zone->private_desc)
			act("$n begins to edit a zone description.", TRUE, ch, 0, 0,
				TO_ROOM);
		else
			act("$n starts to write a zone description.", TRUE, ch, 0, 0,
				TO_ROOM);
		start_text_editor(ch->desc, &zone->private_desc, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	}
}

/* Create zone structure        */
/* Create <num>.zon file        */
/* Add zone <num> to index file */

int
do_create_zone(struct Creature *ch, int num)
{
	struct zone_data *zone = NULL, *new_zone = NULL;
	struct weather_data *weather = NULL;

	char fname[64];
	FILE *index;
	FILE *zone_file;


	/* Check to see if the zone already exists */

	for (zone = zone_table; zone; zone = zone->next) {
		if (zone->number == num) {
			send_to_char(ch, "ERROR: Zone already exists.\r\n");
			return (1);
		}
		if (zone->number < num && zone->top > num * 100) {
			send_to_char(ch, "ERROR: Zone overlaps existing zone.\r\n");
			return (1);
		}
	}

    if (num < 0 || num > 999) {
        send_to_char (ch, "ERROR:  Zone number must be between 0 and 999.\r\n");
        return (1);
    }

	/* Open the index file */

	sprintf(fname, "world/zon/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char(ch, "Could not open index file, zone creation aborted.\r\n");
		return (1);
	}

	/* Open the zone file */

	sprintf(fname, "world/zon/%d.zon", num);
	if (!(zone_file = fopen(fname, "w"))) {
		send_to_char(ch, "Cound not open %d.zon file, zone creation aborted.\r\n",
			num);
		return (1);
	}


	/* Create the new zone and set the defualts */

	CREATE(new_zone, struct zone_data, 1);

	new_zone->name = strdup("A Freshly made Zone");
	new_zone->lifespan = 30;
	new_zone->age = 0;
	new_zone->top = num * 100 + 99;
	new_zone->reset_mode = 0;
	new_zone->number = num;
	new_zone->time_frame = 0;
	new_zone->plane = PLANE_OLC;
	new_zone->owner_idnum = -1;
	new_zone->enter_count = 0;
	new_zone->hour_mod = 0;
	new_zone->year_mod = 0;
	new_zone->flags = ZONE_MOBS_APPROVED + ZONE_OBJS_APPROVED +
		ZONE_ROOMS_APPROVED + ZONE_ZCMDS_APPROVED + ZONE_SEARCH_APPROVED;
	new_zone->world = NULL;
	new_zone->cmd = NULL;
	new_zone->next = NULL;

	CREATE(weather, struct weather_data, 1);
	weather->pressure = 0;
	weather->change = 0;
	weather->sky = 0;
	weather->sunlight = 0;
	weather->humid = 0;

	new_zone->weather = weather;

	/* Add new zone to zone_table */

	if (zone_table) {
		for (zone = zone_table; zone; zone = zone->next)
			if (new_zone->number > zone->number &&
				(!zone->next || new_zone->number < zone->next->number)) {
				if (zone->next != NULL)
					new_zone->next = zone->next;
				zone->next = new_zone;
				break;
			}
	} else
		zone_table = new_zone;

	top_of_zone_table++;

	send_to_char(ch, "Zone %d structure created OK.\r\n", num);

	/* Write the zone file */

	fprintf(zone_file, "#%d\n", new_zone->number);
	fprintf(zone_file, "%s~\n", new_zone->name);
	num2str(buf, new_zone->flags);
	fprintf(zone_file, "%d %d %d %d %d %s\n", new_zone->top,
		new_zone->lifespan, new_zone->reset_mode, new_zone->time_frame,
		new_zone->plane, buf);
	fprintf(zone_file, "*\n");
	fprintf(zone_file, "$\n");

	fclose(zone_file);

	send_to_char(ch, "Zone file, %d.zon, created\r\n", num);

	/* Rewrite index file */

	for (zone = zone_table; zone; zone = zone->next)
		fprintf(index, "%d.zon\n", zone->number);

	fprintf(index, "$\n");

	send_to_char(ch, "Zone index file re-written.\r\n");

	fclose(index);

	slog("Zone %d created by %s.", new_zone->number, GET_NAME(ch));
	return (0);
}


bool
save_zone(struct Creature *ch, struct zone_data *zone)
{
	char fname[64], comment[MAX_TITLE_LENGTH];
	unsigned int tmp;
	struct obj_data *obj;
	struct Creature *mob;
	struct reset_com *zcmd = NULL;
	FILE *zone_file, *realfile;
	PHead *p_head = NULL;

	/* Open the zone file */

	REMOVE_BIT(zone->flags, ZONE_ZONE_MODIFIED);

	sprintf(fname, "world/zon/olc/%d.zon", zone->number);
	if (!(zone_file = fopen(fname, "w")))
		return false;

	fprintf(zone_file, "#%d\n", zone->number);
	fprintf(zone_file, "%s~\n", zone->name);

	if (zone->owner_idnum != -1)
		fprintf(zone_file, "owner: %d\n", zone->owner_idnum);

	if (zone->co_owner_idnum != -1)
		fprintf(zone_file, "co-owner: %d\n", zone->co_owner_idnum);
	
	if (zone->respawn_pt)
		fprintf(zone_file, "respawn-pt: %d\n", zone->respawn_pt);

	if (zone->min_lvl)
		fprintf(zone_file, "minimum-level: %d\n", zone->min_lvl);
	if (zone->min_gen)
		fprintf(zone_file, "minimum-gen: %d\n", zone->min_gen);
	if (zone->max_lvl)
		fprintf(zone_file, "maximum-level: %d\n", zone->max_lvl);
	if (zone->max_gen)
		fprintf(zone_file, "maximum-gen: %d\n", zone->max_gen);
		
	if (zone->public_desc)
		fprintf(zone_file, "public-desc:\n%s~\n",
			tmp_gsub(zone->public_desc, "\r", ""));

	if (zone->private_desc)
		fprintf(zone_file, "private-desc:\n%s~\n",
			tmp_gsub(zone->private_desc, "\r", ""));

	tmp = zone->flags;

	num2str(buf, tmp);
	fprintf(zone_file, "%d %d %d %d %d %s %d %d\n",
		zone->top, zone->lifespan,
		zone->reset_mode, zone->time_frame, zone->plane, buf,
		zone->hour_mod, zone->year_mod);

	for (zcmd = zone->cmd; zcmd; zcmd = zcmd->next) {
		if (zcmd->command == 'D') {
			num2str(buf, zcmd->arg3);
			fprintf(zone_file, "%c %d %3d %5d %5d %5s\n",
				zcmd->command, zcmd->if_flag, zcmd->prob,
				zcmd->arg1, zcmd->arg2, buf);
		} else {
			switch (zcmd->command) {
			case 'M':
				if ((mob = real_mobile_proto(zcmd->arg1))) {
					if (mob && mob->player.short_descr)
						strcpy(comment, mob->player.short_descr);
					else
						strcpy(comment, " ");
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'O':
				if ((obj = real_object_proto(zcmd->arg1))) {
					if (obj && obj->name)
						strcpy(comment, obj->name);
					else
						strcpy(comment, " ");
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'E':
			case 'I':
				if ((obj = real_object_proto(zcmd->arg1))) {
					if (obj && obj->name)
						strcpy(comment, obj->name);
					else
						strcpy(comment, " ");
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'P':
				if ((obj = real_object_proto(zcmd->arg1))) {
					if (obj && obj->name)
						strcpy(comment, obj->name);
					else
						strcpy(comment, " ");
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'V':
				if ((obj = real_object_proto(zcmd->arg3)) &&
					(p_head = real_path_by_num(zcmd->arg1))) {
					strcpy(comment, p_head->name);
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'W':
				if ((mob = real_mobile_proto(zcmd->arg3)) &&
					(p_head = real_path_by_num(zcmd->arg1))) {
					strcpy(comment, p_head->name);
				} else
					strcpy(comment, " BOGUS");
				break;
			case 'G':
				if ((obj = real_object_proto(zcmd->arg1))) {
					if (obj->name)
						strcpy(comment, obj->name);
					else
						strcpy(comment, " ");
				} else
					strcpy(comment, " BOGUS");
				break;
			default:
				strcpy(comment, " ---");
				break;
			}

			fprintf(zone_file, "%c %d %3d %5d %5d %5d        %s\n",
				zcmd->command, zcmd->if_flag, zcmd->prob,
				zcmd->arg1, zcmd->arg2, zcmd->arg3, comment);
		}
	}

	fprintf(zone_file, "*\n");
	fprintf(zone_file, "$\n");

	slog("OLC: %s zsaved %d.", GET_NAME(ch), zone->number);

	sprintf(fname, "world/zon/%d.zon", zone->number);
	realfile = fopen(fname, "w");
	if (realfile) {
		fclose(zone_file);
		sprintf(fname, "world/zon/olc/%d.zon", zone->number);
		if (!(zone_file = fopen(fname, "r"))) {
			slog("SYSERR: Failure to reopen olc zon file.");
			fclose(realfile);
			return false;
		}
		do {
			tmp = fread(buf, 1, 512, zone_file);
			if (fwrite(buf, 1, tmp, realfile) != tmp) {
				fclose(realfile);
				fclose(zone_file);
				slog("SYSERR: Failure to duplicate olc zon file in the main wld dir.");
				return false;
			}
		} while (tmp == 512);
		fclose(realfile);
	}

	fclose(zone_file);

	return true;
}


void
autosave_zones(int SAVE_TYPE)
{
	struct zone_data *zone;

	for (zone = zone_table; zone; zone = zone->next)
		if (!ZONE_FLAGGED(zone, ZONE_LOCKED) &&
			ZONE_FLAGGED(zone, SAVE_TYPE & ZONE_ZONE_MODIFIED))
			if (save_zone(NULL, zone) == 0) {
				mudlog(LVL_IMMORT, NRM, true,
					"SYSERR: Could not save zone : %s\n", zone->name);
			}
/*
  if (ZONE_FLAGGED(zone, SAVE_TYPE&ZONE_ROOMS_MODIFIED))
  if (save_wld(NULL, zone) == 0) {
  sprintf(buf, "SYSERR: Could not save world for zone : %s\n", zone->name);
  mudlog(buf, NRM, LVL_IMMORT, TRUE);
  }
  if (ZONE_FLAGGED(zone, SAVE_TYPE&ZONE_OBJS_MODIFIED))
  if (save_objs(NULL, zone) == 0) {
  sprintf(buf, "SYSERR: Cound not save objs for zone : %s\n", zone->name);
  mudlog(buf, NRM, LVL_IMMORT, TRUE);
  }
*/
}


void
do_zone_cmdrem(struct Creature *ch, struct zone_data *zone, int num)
{
	struct reset_com *zcmd, *next_zcmd;
	int i, found = 0;

	if (!(zcmd = zone->cmd)) {
		send_to_char(ch, "Hmm...\r\n");
		return;
	}
	if (!num) {
		zone->cmd = zcmd->next;
		free(zcmd);
		found = TRUE;
	}

	for (i = 0, zcmd = zone->cmd; zcmd; i++, zcmd = next_zcmd) {
		next_zcmd = zcmd->next;
		if (!found) {
			if (zcmd->next && zcmd->next->line == num) {
				next_zcmd = zcmd->next->next;
				free(zcmd->next);
				zcmd->next = next_zcmd;
				found = TRUE;
			}
			continue;
		}
		zcmd->line = i;
	}
	if (found == 1) {
		send_to_char(ch, "Removed zone command #%d from zone #%d\r\n",
			num, zone->number);
	} else {
		send_to_char(ch, "Could not find zone command #%d in zone #%d\r\n",
			num, zone->number);
	}
}


void
do_zone_cmdlist(struct Creature *ch, struct zone_data *zone, char *arg)
{

	char out_buf[MAX_STRING_LENGTH];
	char door_flg[150];
	struct obj_data *tmp_obj;
	struct Creature *tmp_mob;
	struct room_data *tmp_rom;
	struct reset_com *zcmd;
	char arg1[MAX_INPUT_LENGTH];
	int
	 mode_obj = 0, mode_rem = 0, mode_mob = 0, mode_eq = 0, mode_give =
		0, mode_put = 0, mode_door = 0, mode_error = 0, mode_all =
		0, mode_implant = 0, mode_path = 0, mode_range = 0;
	int i;
	int startcmd = 0, endcmd = 0;
	PHead *p_head = NULL;

	if (!zone) {
		slog("SYSERR: Improper zone passed to do_zone_cmdlist.");
		send_to_char(ch, "Improper zone error.\r\n");
		return;
	}
	arg1[0] = '\0';
	out_buf[0] = '\0';

	arg = one_argument(arg, arg1);
	if (!*arg1) {
		mode_all = 1;
	} else {
		while (*arg1) {
			if (is_abbrev(arg1, "all")) {
				mode_all = 1;
			} else if (is_abbrev(arg1, "objects"))
				mode_obj = 1;
			else if (is_abbrev(arg1, "removes"))
				mode_rem = 1;
			else if (is_abbrev(arg1, "mobiles"))
				mode_mob = 1;
			else if (is_abbrev(arg1, "equips"))
				mode_eq = 1;
			else if (is_abbrev(arg1, "implants"))
				mode_implant = 1;
			else if (is_abbrev(arg1, "gives"))
				mode_give = 1;
			else if (is_abbrev(arg1, "puts"))
				mode_put = 1;
			else if (is_abbrev(arg1, "doors"))
				mode_door = 1;
			else if (is_abbrev(arg1, "paths"))
				mode_path = 1;
			else if (is_abbrev(arg1, "range")) {
				mode_range = 1;
				mode_all = 1;
				arg = two_arguments(arg, arg1, arg2);
				if (*arg1 && *arg2 && isdigit(*arg1) && isdigit(*arg2)) {
					startcmd = atoi(arg1);
					endcmd = atoi(arg2);
				} else {
					send_to_char(ch, "Invalid zone command segment.\r\n");
					return;
				}
				if (startcmd > endcmd) {
					send_to_char(ch, "Invalid zone command segment.\r\n");
					return;
				}
			} else if (is_abbrev(arg1, "errors") 
				|| is_abbrev(arg1, "comments")) {
				mode_error = 1;
			} else {
				send_to_char(ch, "'%s' is not a valid argument.\r\n", arg1);
			}
			arg = one_argument(arg, arg1);
		}
	}
	// Yeah, ugly. I know.
	// If mode all is on and anything other than range is on, 
	//  turn "mode_all" off.
	if (mode_all &&
		(mode_obj || mode_rem || mode_mob || mode_eq
			|| mode_give || mode_put || mode_door
			|| mode_error || mode_implant || mode_path))
		mode_all = 0;



	sprintf(out_buf, "Command list for zone %d :\r\n\r\n", zone->number);

	for (i = 0, zcmd = zone->cmd; zcmd && zcmd->command != 'S';
		i++, zcmd = zcmd->next) {

		if (mode_range && (i < startcmd || i > endcmd))
			continue;

		strcpy(buf, "");
		switch (zcmd->command) {
		case 'M':
			if (!mode_all && !mode_mob)
				break;
			tmp_mob = real_mobile_proto(zcmd->arg1);
			tmp_rom = real_room(zcmd->arg3);
			sprintf(buf,
				"%3d. %sMobile%s: % d [%3d] %5d to   %5d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, zcmd->arg1, zcmd->arg3, zcmd->arg2, CCYEL(ch,
					C_NRM), (tmp_mob
					&& tmp_mob->player.short_descr) ? tmp_mob->player.
				short_descr : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'O':
			if (!mode_all && !mode_obj)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			tmp_rom = real_room(zcmd->arg3);
			sprintf(buf,
				"%3d. %sObject%s: % d [%3d] %5d to   %5d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, zcmd->arg1, tmp_rom ? tmp_rom->number : (-1),
				zcmd->arg2, CCGRN(ch, C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'P':
			if (!mode_all && !mode_put)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			sprintf(buf,
				"%3d.    %sPut%s: % d [%3d] %5d to   %5d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCBLU_BLD(ch, C_NRM), CCNRM(ch, C_NRM),
				zcmd->if_flag, zcmd->prob, zcmd->arg1, zcmd->arg3, zcmd->arg2,
				CCGRN(ch, C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'V':
			if (!mode_all && !mode_path)
				break;
			p_head = real_path_by_num(zcmd->arg1);
			sprintf(buf,
				"%3d.  %sPath%s : % d [%3d] %5d   to obj %5d     : (%s%s%s)\r\n",
				zcmd->line, CCYEL_REV(ch, C_NRM), CCNRM(ch, C_NRM),
				zcmd->if_flag, zcmd->prob, zcmd->arg1, zcmd->arg3, CCCYN(ch,
					C_NRM), p_head->name, CCNRM(ch, C_NRM));
			break;
		case 'W':
			if (!mode_all && !mode_path)
				break;
			p_head = real_path_by_num(zcmd->arg1);
			sprintf(buf,
				"%3d.   %sPath%s: % d [%3d] %5d to mob %5d   : (%s%s%s)\r\n",
				zcmd->line, CCYEL_REV(ch, C_NRM), CCNRM(ch, C_NRM),
				zcmd->if_flag, zcmd->prob, zcmd->arg1, zcmd->arg3, CCCYN(ch,
					C_NRM), p_head->name, CCNRM(ch, C_NRM));
			break;
		case 'G':
			if (!mode_all && !mode_give)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			sprintf(buf,
				"%3d.   %sGive%s: % d [%3d] %5d to   %5d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCBLU_BLD(ch, C_NRM), CCNRM(ch, C_NRM),
				zcmd->if_flag, zcmd->prob, zcmd->arg1, zcmd->arg3, zcmd->arg2,
				CCGRN(ch, C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'E':
			if (!mode_all && !mode_eq)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			sprintf(buf,
				"%3d.  %sEquip%s: % d [%3d] %5d to   %5d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCMAG(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, zcmd->arg1, zcmd->arg3, zcmd->arg2, CCGRN(ch,
					C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'I':
			if (!mode_all && !mode_implant)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			sprintf(buf,
				"%3d.%sImplant%s: % d [%3d] %5d to      %2d, max %3d: (%s%s%s)\r\n",
				zcmd->line, CCMAG(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, zcmd->arg1, zcmd->arg3, zcmd->arg2, CCGRN(ch,
					C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'R':
			if (!mode_all && !mode_rem)
				break;
			tmp_obj = real_object_proto(zcmd->arg1);
			sprintf(buf,
				"%3d. %sRem Obj%s % d [%3d] %5d from %5d,          (%s%s%s)\r\n",
				zcmd->line, CCRED(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, zcmd->arg1, zcmd->arg2, CCGRN(ch, C_NRM), (tmp_obj
					&& tmp_obj->name) ? tmp_obj->
				name : "null-desc", CCNRM(ch, C_NRM));
			break;
		case 'D':
			if (!mode_all && !mode_door)
				break;
			sprintbit(zcmd->arg3, door_flags, door_flg);
			tmp_rom = real_room(zcmd->arg1);
			sprintf(buf,
				"%3d. %sDoor%s  : % d [%3d] %5d dir  %5s,          (%s)\r\n",
				zcmd->line, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), zcmd->if_flag,
				zcmd->prob, tmp_rom ? tmp_rom->number : (-1), dirs[zcmd->arg2],
				door_flg);
			break;
		default:
			if (!mode_all && !mode_error)
				break;
			sprintf(buf, "%3d. %c     : % d [%3d] %5d    %5d    %5d\r\n",
				zcmd->line, zcmd->command, zcmd->if_flag, zcmd->prob,
				zcmd->arg1, zcmd->arg2, zcmd->arg3);
			break;
		}

		if (strlen(out_buf) + strlen(buf) > MAX_STRING_LENGTH - 128) {
			strcat(out_buf, "**OVERFLOW**\r\n");
			break;
		} else
			strcat(out_buf, buf);
	}

	page_string(ch->desc, out_buf);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *
one_argument_no_lower(char *argument, char *first_arg)
{
	char *begin = first_arg;

	do {
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = *argument;
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word_no_lower(begin));

	return argument;
}


/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int
search_block_no_lower(char *arg, const char **list, bool exact)
{
	register int i, l;

	/* Get length of string Only... do NOT lowercase */
	for (l = 0; *(arg + l); l++);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strcmp(arg, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;				/* Avoid "" to match the first available
								   * string */
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strncmp(arg, *(list + i), l))
				return (i);
	}

	return -1;
}


int
fill_word_no_lower(char *argument)
{
	return (search_block_no_lower(argument, fill_words, FALSE) >= 0);
}

#define ZPATH_USAGE "Usage: olc zpath <'mob'|'obj'> <name> <path name>\r\n"

void
do_zpath_cmd(struct Creature *ch, char *argument)
{

	PHead *p_head = NULL;
	struct obj_data *obj = NULL;
	struct Creature *mob = NULL;
	struct zone_data *zone = ch->in_room->zone;
	struct reset_com *zonecmd, *tmp_zonecmd;
	bool obj_mode = 0, mob_mode = 0, found = 0;
	int line = 0;

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "You cannot edit this zone.\r\n");
		return;
	}

	skip_spaces(&argument);
	argument = one_argument(argument, buf);

	if (!*argument || !*buf) {
		send_to_char(ch, ZPATH_USAGE);
		return;
	}

	zone = ch->in_room->zone;

	if (!OLC_EDIT_OK(ch, zone, ZONE_ZCMDS_APPROVED)) {
		send_to_char(ch, "Zone commands are not approved for this zone.\r\n");
		return;
	}

	if (is_abbrev(buf, "object"))
		obj_mode = 1;
	else if (is_abbrev(buf, "mobile"))
		mob_mode = 1;
	else {
		send_to_char(ch, ZPATH_USAGE);
		return;
	}

	two_arguments(argument, buf, buf2);

	if (!*buf2) {
		send_to_char(ch, ZPATH_USAGE);
		return;
	}

	if (!(p_head = real_path(buf2))) {
		send_to_char(ch, "There is no path called '%s'.\r\n", buf2);
		return;
	}

	if (mob_mode) {
		if (!(mob = get_char_room_vis(ch, buf))) {
			send_to_char(ch, "You see no mob by the name of '%s' here.\r\n", buf);
			return;
		}

		if (mob == ch || !IS_NPC(mob)) {
			send_to_char(ch, "You're pretty funny.\r\n");
			return;
		}

		if (!add_path_to_mob(mob, p_head->name)) {
			send_to_char(ch, "Cannot add that path to that mobile.\r\n");
			return;
		}
		path_remove_object(mob);

		CREATE(zonecmd, struct reset_com, 1);

		zonecmd->command = 'W';
		zonecmd->if_flag = 1;
		zonecmd->arg1 = p_head->number;
		zonecmd->arg2 = 0;
		zonecmd->arg3 = GET_MOB_VNUM(mob);
		zonecmd->prob = 100;

		zonecmd->next = NULL;

		if (zone->cmd) {
			for (line = 0, found = 0, tmp_zonecmd = zone->cmd; tmp_zonecmd;
				line++, tmp_zonecmd = tmp_zonecmd->next) {
				if (found)
					tmp_zonecmd->line++;
				else if (tmp_zonecmd->command == 'M' &&
					tmp_zonecmd->arg1 == mob->mob_specials.shared->vnum) {
					zonecmd->line = tmp_zonecmd->line;
					zonecmd->next = tmp_zonecmd->next;
					tmp_zonecmd->next = zonecmd;
					found = 1;
				}
			}
			if (found == 0) {
				send_to_char(ch,
					"zmob command required for mobile (V) %d before this can be set.\r\n",
					mob->mob_specials.shared->vnum);
				free(zonecmd);
				return;
			}
		} else {
			send_to_char(ch,
				"zmob command required for mobile (V) %d before this can be set.\r\n",
				mob->mob_specials.shared->vnum);
			free(zonecmd);
			return;
		}

		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		add_path_to_mob(mob, p_head->name);
		send_to_char(ch, "Command completed ok.\r\n");
		return;
	}

	if (obj_mode) {

		if (!(obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
			send_to_char(ch, "You see no mob by the name of '%s' here.\r\n", buf);
			return;
		}

		if (!add_path_to_vehicle(obj, p_head->name)) {
			send_to_char(ch, "Cannot add that path to that object.\r\n");
			return;
		}
		path_remove_object(obj);

		CREATE(zonecmd, struct reset_com, 1);

		zonecmd->command = 'V';
		zonecmd->if_flag = 1;
		zonecmd->arg1 = p_head->number;
		zonecmd->arg2 = 0;
		zonecmd->arg3 = GET_OBJ_VNUM(obj);
		zonecmd->prob = 100;

		zonecmd->next = NULL;

		if (zone->cmd) {
			for (line = 0, found = 0, tmp_zonecmd = zone->cmd;
				tmp_zonecmd && found != 1;
				line++, tmp_zonecmd = tmp_zonecmd->next)
				if (tmp_zonecmd->command == 'O' &&
					tmp_zonecmd->arg1 == GET_OBJ_VNUM(obj)) {
					zonecmd->line = ++line;
					zonecmd->next = tmp_zonecmd->next;
					tmp_zonecmd->next = zonecmd;
					found = 1;
					break;
				}
			if (found == 0) {
				send_to_char(ch,
					"Zone command O required for %d before this can be set.\r\n",
					GET_OBJ_VNUM(obj));
				free(zonecmd);
				return;
			}
		} else {
			send_to_char(ch,
				"Zone command O required for %d before this can be set.\r\n",
				GET_OBJ_VNUM(obj));
			free(zonecmd);
			return;
		}

		SET_BIT(zone->flags, ZONE_ZONE_MODIFIED);
		add_path_to_vehicle(obj, p_head->name);
		send_to_char(ch, "Command completed ok.\r\n");
		return;
	}
}
