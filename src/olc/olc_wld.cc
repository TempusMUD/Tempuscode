//
// File: olc_wld.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
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
#include "clan.h"
#include "specs.h"
#include "house.h"

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern int top_of_world;
extern int olc_lock;
extern int *wld_index;
extern struct room_data *r_mortal_start_room;
extern struct clan_data *clan_list;

long asciiflag_conv(char *buf);

void num2str(char *str, int num);
void do_stat_object(struct Creature *ch, struct obj_data *obj);

char *find_exdesc(char *word, struct extra_descr_data *list, int find_exact =
	0);
extern struct extra_descr_data *locate_exdesc(char *word,
	struct extra_descr_data *list);

int
write_wld_index(struct Creature *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; wld_index[i] != -1; i++) {
		count++;
		if (wld_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (wld_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (wld_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = wld_index[i];
			done = 1;
		} else
			new_index[j] = wld_index[i];
		j++;
	}

	free(wld_index);
	wld_index = new_index;

	sprintf(fname, "world/wld/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char(ch, "Could not open index file, world save aborted.\r\n");
		return (0);
	}

	for (i = 0; wld_index[i] != -1; i++)
		fprintf(index, "%d.wld\n", wld_index[i]);

	fprintf(index, "$\n");

	send_to_char(ch, "World index file re-written.\r\n");

	fclose(index);

	return (1);
}

//
// check_room_cstrings()
// calls remove_from_cstring() on each char *
// in the room
//

int
check_room_cstrings(struct room_data *room)
{
	remove_from_cstring(room->name);
	remove_from_cstring(room->description);

	for (int i = 0; i < NUM_DIRS; i++) {
		if (room->dir_option[i]) {
			remove_from_cstring(room->dir_option[i]->general_description);
			remove_from_cstring(room->dir_option[i]->keyword);
		}
	}

	for (struct extra_descr_data * desc = room->ex_description; desc;
		desc = desc->next) {
		remove_from_cstring(desc->description);
		remove_from_cstring(desc->keyword);
	}

	for (struct special_search_data * search = room->search; search;
		search = search->next) {
		remove_from_cstring(search->command_keys);
		remove_from_cstring(search->to_room);
		remove_from_cstring(search->to_vict);
		remove_from_cstring(search->to_remote);
	}

	remove_from_cstring(room->sounds);

	return 0;
}

//
// save_room()
// must call check_room_cstrings() to clean up the room strings for saving
// then writes the single room to the specified file pointer
//

int
save_room(struct Creature *ch, struct room_data *room, FILE * file)
{
	unsigned int i, j;
	unsigned int tmp;
	struct extra_descr_data *desc;
	struct special_search_data *search;
	struct room_affect_data *rm_aff;

	if (!file) {
		slog("SYSERR: null fl in save_room().");
		return 1;
	}

	if (check_room_cstrings(room)) {
		slog("SYSERR: save_room() check_room_cstrings() failed.");
		return 1;
	}

	fprintf(file, "#%d\n", room->number);

	if (room->name)
		fprintf(file, "%s", room->name);

	fprintf(file, "~\n");

	if (room->description) {
		tmp = strlen(room->description);
		for (i = 0; i < tmp; i++)
			if (room->description[i] != '\r' && room->description[i] != '~')
				fputc(room->description[i], file);
	}

	fprintf(file, "~\n");
	tmp = room->room_flags;
	REMOVE_BIT(tmp, ROOM_HOUSE | ROOM_HOUSE_CRASH | ROOM_ATRIUM);

	for (rm_aff = room->affects; rm_aff; rm_aff = rm_aff->next)
		if (rm_aff->type == RM_AFF_FLAGS)
			REMOVE_BIT(tmp, rm_aff->flags);

	num2str(buf, tmp);
	fprintf(file, "%d %s %d\n", room->zone->number, buf, room->sector_type);

	for (i = 0; i < NUM_DIRS; i++) {
		if (room->dir_option[i]) {
			fprintf(file, "D%d\n", i);
			if (room->dir_option[i]->general_description) {
				tmp = strlen(room->dir_option[i]->general_description);
				for (j = 0; j < tmp; j++)
					if (room->dir_option[i]->general_description[j] != '\r')
						fputc(room->dir_option[i]->general_description[j],
							file);
			}
			fprintf(file, "~\n");
			if (room->dir_option[i]->keyword)
				fprintf(file, "%s", room->dir_option[i]->keyword);
			fprintf(file, "~\n");

			tmp = room->dir_option[i]->exit_info;
			REMOVE_BIT(tmp, EX_CLOSED | EX_LOCKED);
			for (rm_aff = room->affects; rm_aff; rm_aff = rm_aff->next)
				if ((unsigned int)rm_aff->type == i)
					REMOVE_BIT(tmp, rm_aff->flags);

			num2str(buf, tmp);
			strcat(buf, " ");
			fprintf(file, buf);

			fprintf(file, "%d %d\n", room->dir_option[i]->key,
				room->dir_option[i]->to_room ? room->dir_option[i]->to_room->
				number : (-1));
		}
	}

	desc = room->ex_description;
	while (desc != NULL) {
		if (!desc->keyword || !desc->description) {
			slog("OLCERROR: ExDesc with null %s, room #%d.",
				!desc->keyword ? "keyword" : "desc", room->number);
			send_to_char(ch, "I didn't save your bogus extra desc in room %d.\r\n",
				room->number);
			desc = desc->next;
			continue;
		}
		fprintf(file, "E\n");
		fprintf(file, "%s~\n", desc->keyword);
		tmp = strlen(desc->description);
		for (i = 0; i < tmp; i++)
			if (desc->description[i] != '\r')
				fputc(desc->description[i], file);
		fprintf(file, "~\n");
		desc = desc->next;
	}

	search = room->search;
	while (search) {
		fprintf(file, "Z\n");
		fprintf(file, "%s~\n", search->command_keys);
		fprintf(file, "%s~\n", search->keywords ? search->keywords : "");
		if (search->to_vict) {
			tmp = strlen(search->to_vict);
			for (i = 0; i < tmp; i++)
				if (search->to_vict[i] != '\r' && search->to_vict[i] != '~')
					fputc(search->to_vict[i], file);
		}
		fprintf(file, "~\n");
		if (search->to_room) {
			tmp = strlen(search->to_room);
			for (i = 0; i < tmp; i++)
				if (search->to_room[i] != '\r' && search->to_room[i] != '~')
					fputc(search->to_room[i], file);
		}
		fprintf(file, "~\n");
		if (search->to_remote) {
			tmp = strlen(search->to_remote);
			for (i = 0; i < tmp; i++)
				if (search->to_remote[i] != '\r'
					&& search->to_remote[i] != '~')
					fputc(search->to_remote[i], file);
		}
		fprintf(file, "~\n");

		fprintf(file, "%d %d %d %d %d\n", search->command,
			search->arg[0], search->arg[1], search->arg[2], search->flags);

		search = search->next;
		continue;
	}

	if (room->sounds) {
		fprintf(file, "L\n");
		tmp = strlen(room->sounds);
		for (i = 0; i < tmp; i++)
			if (room->sounds[i] != '\r')
				fputc(room->sounds[i], file);
		fprintf(file, "~\n");
	}
	if (FLOW_SPEED(room)) {
		if (FLOW_DIR(room) < 0 || FLOW_DIR(room) >= NUM_DIRS) {
			send_to_char(ch, "OLCERROR: Bunk flow direction in room %d.\r\n",
				room->number);
		} else if (FLOW_SPEED(room) < 0) {
			send_to_char(ch, "OLCERROR: Negative flow speed in room %d.\r\n",
				room->number);
		} else {
			fprintf(file, "F\n");
			fprintf(file, "%d %d %d\n", FLOW_DIR(room), FLOW_SPEED(room),
				FLOW_TYPE(room));
			if (FLOW_TYPE(room) < 0 || FLOW_TYPE(room) >= NUM_FLOW_TYPES) {
				send_to_char(ch, "Error in flow type, room #%d.\r\n",
					room->number);
			}
		}
	}

	if (MAX_OCCUPANTS(room) != 256)
		fprintf(file, "O %d\n", MAX_OCCUPANTS(room));


	fprintf(file, "S\n");
	return 0;
}

int
save_wld(struct Creature *ch)
{
	FILE *file = 0;
	char temp_fname[1024];
	char real_fname[1024];
	struct zone_data *zone = ch->in_room->zone;

	sprintf(temp_fname, "world/wld/%d.wld", zone->number);
	if ((access(temp_fname, F_OK) >= 0) && (access(temp_fname, W_OK) < 0)) {
		mudlog(0, BRF, true,
			"OLC: ERROR - Main world file for zone %d is read-only.",
			zone->number);
	}

	sprintf(real_fname, "world/wld/%d.wld", zone->number);
	sprintf(temp_fname, "%s.tmp", real_fname);

	if (!(file = fopen(temp_fname, "w")))
		return 1;

	if ((write_wld_index(ch, zone)) != 1) {
		fclose(file);
		return (1);
	}

	for (struct room_data * room = zone->world; room; room = room->next) {

		if (save_room(ch, room, file)) {
			send_to_char(ch, "Error saving room #%d.\r\n", room->number);
			return 1;
		}
	}

	fprintf(file, "$~\n");
	fclose(file);

	if (rename(temp_fname, real_fname)) {
		slog("SYSERR: Error copying %s -> %s in save_wld: %s.",
			temp_fname, real_fname, strerror(errno));
		send_to_char(ch, 
			"There was an error copying the temporary .wld file during the save.\r\n"
			"World Save Failed.\r\n");
		return 1;
	}

	slog("OLC: %s rsaved %d.", GET_NAME(ch), zone->number);

	return 0;
}


struct room_data *
do_create_room(struct Creature *ch, int vnum)
{

	struct room_data *rm = NULL, *new_rm = NULL;
	struct zone_data *zone = NULL;
	//int i;

	if ((rm = real_room(vnum))) {
		send_to_char(ch, "ERROR: Room already exists.\r\n");
		return NULL;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum >= zone->number * 100 && vnum <= zone->top)
			break;

	if (!zone) {
		send_to_char(ch, "ERROR: A zone must be defined for the room first.\r\n");
		return NULL;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Try creating rooms in your own zone, luser.\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to CREATE room %d.",
			GET_NAME(ch), vnum);
		return NULL;
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_ROOMS_APPROVED)) {
		send_to_char(ch, "World OLC is not approved for this zone.\r\n");
		return NULL;
	}

	if (zone->world && vnum > zone->world->number) {
		for (rm = zone->world; rm; rm = rm->next) {
			if (vnum > rm->number && (!rm->next || vnum < rm->next->number))
				break;
		}
	}
	//CREATE( new_rm, struct room_data, 1 );
	new_rm = new room_data(vnum, zone);
	new_rm->name = str_dup("A Freshly Made Room");

	/*
	   new_rm->zone = zone;
	   new_rm->number = vnum;
	   new_rm->description = NULL;
	   new_rm->sounds = NULL;
	   new_rm->room_flags = 0;
	   new_rm->sector_type = 0;
	   new_rm->func = NULL;
	   new_rm->contents = NULL;
	   new_rm->people = NULL;
	   new_rm->light = 0;
	   new_rm->max_occupancy = 256;
	   new_rm->find_first_step_index = 0;

	   for ( i = 0; i < NUM_OF_DIRS; i++ )
	   new_rm->dir_option[i] = NULL;

	   new_rm->ex_description = NULL;
	   new_rm->search = NULL;
	   new_rm->flow_dir = 0;
	   new_rm->flow_speed = 0;
	   new_rm->flow_type = 0;
	 */
	if (rm) {
		new_rm->next = rm->next;
		rm->next = new_rm;
	} else {
		new_rm->next = zone->world;
		zone->world = new_rm;
	}

	top_of_world++;

	return (new_rm);
}

int
do_destroy_room(struct Creature *ch, int vnum)
{

	struct room_data *rm = NULL, *t_rm = NULL;
	struct zone_data *zone = NULL;
	struct Creature *vict = NULL;
	struct obj_data *obj = NULL, *next_obj = NULL;
	struct special_search_data *srch = NULL;
	struct extra_descr_data *desc = NULL;
	struct clan_data *clan = NULL;
	struct room_list_elem *rm_list = NULL;
	int i;
	void REMOVE_ROOM_FROM_CLAN(struct room_list_elem *rm_list,
		struct clan_data *clan);

	if (!(rm = real_room(vnum))) {
		send_to_char(ch, "ERROR: That room does not exist.\r\n");
		return 1;
	}

	if (!(zone = rm->zone)) {
		send_to_char(ch, "ERROR: ( very bad ) This room has NULL zone!!\r\n");
		return 1;
	}

	if (GET_IDNUM(ch) != zone->owner_idnum && GET_LEVEL(ch) < LVL_LUCIFER) {
		send_to_char(ch, "Oh, no you dont!!!\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to DESTROY room %d.",
			GET_NAME(ch), rm->number);
		return 1;
	}

	if (rm == zone->world) {
		zone->world = rm->next;
		t_rm = zone->world;
	} else {
		for (t_rm = zone->world; t_rm; t_rm = t_rm->next) {
			if (t_rm->next && t_rm->next == rm) {
				t_rm->next = rm->next;
				break;
			}
			if (!t_rm->next) {
				send_to_char(ch, "!( t_rm->next ) error.\r\n");
				return 1;
			}
		}
	}
	CreatureList::iterator it = rm->people.begin();
	for (; it != rm->people.end(); ++it) {
		vict = *it;
		send_to_char(vict, 
			"The room in which you exist is suddenly removed from reality!\r\n");
		char_from_room(vict,false);
		if (rm->next) {
			char_to_room(vict, rm->next,false);
		} else if (t_rm) {
			char_to_room(vict, t_rm,false);
		} else if (zone->world) {
			char_to_room(vict, zone->world,false);
		} else
			char_to_room(vict, r_mortal_start_room,false);

		look_at_room(vict, vict->in_room, 0);
		act("$n appears from a void in reality.", TRUE, ch, 0, 0, TO_ROOM);

	}

	for (obj = rm->contents; obj; obj = next_obj) {
		next_obj = obj->next_content;
		extract_obj(obj);
	}

	for (clan = clan_list; clan; clan = clan->next)
		for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
			if (rm == rm_list->room) {
				REMOVE_ROOM_FROM_CLAN(rm_list, clan);
				break;
			}

	if (rm->name)
		free(rm->name);
	if (rm->description)
		free(rm->description);
	if (rm->sounds)
		free(rm->sounds);

	while ((desc = rm->ex_description)) {
		rm->ex_description = desc->next;
		if (desc->keyword)
			free(desc->keyword);
		if (desc->description)
			free(desc->description);
		free(desc);
	}

	while ((srch = rm->search)) {
		rm->search = srch->next;
		if (srch->command_keys)
			free(srch->command_keys);
		if (srch->keywords)
			free(srch->keywords);
		if (srch->to_room)
			free(srch->to_room);
		if (srch->to_vict)
			free(srch->to_vict);
		free(srch);
	}

	for (i = 0; i < NUM_DIRS; i++) {
		if (!rm->dir_option[i])
			continue;
		if (rm->dir_option[i]->general_description)
			free(rm->dir_option[i]->general_description);
		if (rm->dir_option[i]->keyword)
			free(rm->dir_option[i]->keyword);
	}

	while (rm->affects)
		affect_from_room(rm, rm->affects);

	for (zone = zone_table; zone; zone = zone->next)
		for (t_rm = zone->world; t_rm; t_rm = t_rm->next)
			for (i = 0; i < NUM_DIRS; i++)
				if (t_rm->dir_option[i] && t_rm->dir_option[i]->to_room == rm)
					t_rm->dir_option[i]->to_room = NULL;

	top_of_world--;
	//free( rm );
	delete rm;
	return 0;
}


void
do_clear_room(struct Creature *ch)
{
	struct extra_descr_data *desc = NULL;
	struct special_search_data *srch = NULL;
	struct room_affect_data *aff = NULL;


	if (ch->in_room->name)
		free(ch->in_room->name);

	ch->in_room->name = strdup("A Blank Room");

	if (ch->in_room->description)
		free(ch->in_room->description);
	ch->in_room->description = NULL;

	if (ch->in_room->sounds)
		free(ch->in_room->sounds);
	ch->in_room->sounds = NULL;

	ch->in_room->room_flags = 0;
	ch->in_room->sector_type = SECT_INSIDE;

	while ((desc = ch->in_room->ex_description)) {
		ch->in_room->ex_description = desc->next;
		if (desc->keyword)
			free(desc->keyword);
		if (desc->description)
			free(desc->description);
		free(desc);
	}
	ch->in_room->ex_description = NULL;

	while ((srch = ch->in_room->search)) {
		ch->in_room->search = srch->next;
		if (srch->command_keys)
			free(srch->command_keys);
		if (srch->keywords)
			free(srch->keywords);
		if (srch->to_room)
			free(srch->to_room);
		if (srch->to_vict)
			free(srch->to_vict);
		free(srch);
	}
	ch->in_room->search = NULL;

	while ((aff = ch->in_room->affects)) {
		ch->in_room->affects = aff->next;
		if (aff->description)
			free(aff->description);
		free(aff);
	}
	ch->in_room->affects = NULL;

	ch->in_room->light = 0;
	ch->in_room->flow_dir = 0;
	ch->in_room->flow_speed = 0;
	ch->in_room->flow_type = 0;
	ch->in_room->max_occupancy = 256;

	send_to_char(ch, "Room fully cleared..\r\n");
}

void
olc_mimic_room(struct Creature *ch, struct room_data *rnum, char *argument)
{

	char arg1[MAX_INPUT_LENGTH];
	byte mode_sounds = 0, mode_desc = 0, mode_sector = 0, mode_flags = 0,
		mode_exdesc = 0, mode_all = 0, mode_title = 0;
	struct extra_descr_data *desc = NULL, *ndesc = NULL;

	argument = one_argument(argument, arg1);
	if (!*arg1)
		mode_all = 1;
	else
		while (*arg1) {
			if (is_abbrev(arg1, "sounds"))
				mode_sounds = 1;
			else if (is_abbrev(arg1, "description"))
				mode_desc = 1;
			else if (is_abbrev(arg1, "sector"))
				mode_sector = 1;
			else if (is_abbrev(arg1, "flags"))
				mode_flags = 1;
			else if (is_abbrev(arg1, "exdesc"))
				mode_exdesc = 1;
			else if (is_abbrev(arg1, "title"))
				mode_title = 1;
			else {
				send_to_char(ch, "'%s' is not a valid argument.\r\n", arg1);
			}
			argument = one_argument(argument, arg1);
		}
	if (mode_all || mode_title) {	/* Room Title Mode */
		if (ch->in_room->name)
			free(ch->in_room->name);
		if (rnum->name)
			ch->in_room->name = strdup(rnum->name);
		else
			ch->in_room->name = NULL;
	}
	if (mode_all || mode_desc) {	/* Room Description Mode */
		if (ch->in_room->description)
			free(ch->in_room->description);
		if (rnum->description)
			ch->in_room->description = strdup(rnum->description);
		else
			ch->in_room->description = NULL;
	}
	if (mode_all || mode_sounds) {	/*  Room Sounds Mode   */
		if (ch->in_room->sounds)
			free(ch->in_room->sounds);
		if (rnum->sounds)
			ch->in_room->sounds = strdup(rnum->sounds);
		else
			ch->in_room->sounds = NULL;
	}
	if (mode_all || mode_flags)	/*  Room Flags Mode   */
		ch->in_room->room_flags = rnum->room_flags;
	if (mode_all || mode_sector)	/*  Room Sector Mode  */
		ch->in_room->sector_type = rnum->sector_type;
	if (mode_all || mode_exdesc) {	/*  Room Exdesc Mode  */
		desc = ch->in_room->ex_description;
		ch->in_room->ex_description = NULL;
		while (desc != NULL) {
			if (desc->keyword)
				free(desc->keyword);
			if (desc->description)
				free(desc->description);
			desc = desc->next;
		}
		desc = rnum->ex_description;
		while (desc != NULL) {
			CREATE(ndesc, struct extra_descr_data, 1);
			ndesc->keyword = strdup(desc->keyword);
			ndesc->description = strdup(desc->description);
			ndesc->next = ch->in_room->ex_description;
			ch->in_room->ex_description = ndesc;
			desc = desc->next;
		}
	}

	ch->in_room->flow_dir = rnum->flow_dir;
	ch->in_room->flow_type = rnum->flow_type;
	ch->in_room->flow_speed = rnum->flow_speed;
	ch->in_room->max_occupancy = rnum->max_occupancy;

	send_to_char(ch, "Okay, done mimicing.\r\n");
}

static const char *olc_rset_keys[] = {
	"title",
	"description",				/* 1 -- room desc */
	"sector",
	"flags",
	"sound",					/* 4 -- room sounds */
	"flow",
	"occupancy",
	"special",
	"specparam",
	"\n"						/* many more to be added */
};


void
do_olc_rset(struct Creature *ch, char *argument)
{

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
		arg3[MAX_INPUT_LENGTH];
	int rset_command, i, j, tmp_flags, flag, cur_flags, edir;
	int state = 0;

	if (!*argument) {
		page_string(ch->desc, OLC_RSET_USAGE);
		return;
	}

	half_chop(argument, arg1, arg2);
	skip_spaces(&argument);

	if ((rset_command = search_block(arg1, olc_rset_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid rset command '%s'.\r\n", arg1);
		return;
	}

	switch (rset_command) {
	case 0:					/* rtitle */
		if (!*arg2) {
			send_to_char(ch, "You have to set it to something moron!\r\n");
			return;
		}
		if (ch->in_room->name)
			free(ch->in_room->name);
		ch->in_room->name = strdup(arg2);
		send_to_char(ch, "Okay, room title changed.\r\n");
		break;

	case 1:					/* rdescription */
		if (ch->in_room->description) {
			act("$n begins to edit a room description.", TRUE, ch, 0, 0,
				TO_ROOM);
		} else {
			act("$n begins to write a room description.", TRUE, ch, 0, 0,
				TO_ROOM);
		}
		start_text_editor(ch->desc, &ch->in_room->description, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		break;

	case 2:					/* rsector */
		if (!*arg2) {
			send_to_char(ch, "Set sector to what?\r\n");
			return;
		}
		if (!is_number(arg2)) {
			if ((i = search_block(arg2, sector_types, 0)) < 0) {
				send_to_char(ch, "No such sector type.  Type olc h rsect.\r\n");
				return;
			} else
				ch->in_room->sector_type = i;
		} else
			ch->in_room->sector_type = atoi(arg2);

		send_to_char(ch, "Room sector type set to: %s.\r\n",
			sector_types[(int)ch->in_room->sector_type]);
		break;

	case 3:					/* rflags */
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, "Usage: olc rset flags [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = ROOM_FLAGS(ch->in_room);

		while (*arg1) {
			if ((flag = search_block(arg1, roomflag_names, FALSE)) == -1) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		ROOM_FLAGS(ch->in_room) = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char(ch, "Room flags set\r\n");
		} else if (tmp_flags == 0)
			send_to_char(ch, "Room flags not altered.\r\n");
		else {
			send_to_char(ch, "Room flags set.\r\n");
			if (IS_SET(tmp_flags, ROOM_INDOORS) &&
				MAX_OCCUPANTS(ch->in_room) == 256)
				MAX_OCCUPANTS(ch->in_room) = 50;
		}
		break;

	case 4:					/*   sound   */
		if (*arg2 && is_abbrev(arg2, "remove")) {
			if (ch->in_room->sounds) {
				free(ch->in_room->sounds);
				ch->in_room->sounds = NULL;
			}
			send_to_char(ch, "Sounds removed from room.\r\n");
			break;
		}

		if (ch->in_room->sounds == NULL) {
			act("$n begins to create a sound.", TRUE, ch, 0, 0, TO_ROOM);
		} else {
			act("$n begins to edit a sound.", TRUE, ch, 0, 0, TO_ROOM);
		}
		start_text_editor(ch->desc, &ch->in_room->sounds, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		break;
	case 5:					/*  flow  */
		if (!*arg2) {
			strcpy(buf, "Current flow state:");
			if (!FLOW_SPEED(ch->in_room))
				strcat(buf, " None.\r\n");
			else {
				send_to_char(ch, "Direction: %s, Speed: %d, Type: %s ( %d ).\r\n",
					dirs[(int)FLOW_DIR(ch->in_room)], FLOW_SPEED(ch->in_room),
					flow_types[(int)FLOW_TYPE(ch->in_room)],
					(int)FLOW_TYPE(ch->in_room));
			}
			send_to_char(ch, "Usage: olc rset flow <dir> <speed> <type>\r\n");
			return;
		}
		half_chop(arg2, arg1, arg2);	/* sneaky trix */
		strcpy(argument, arg2);
		half_chop(argument, arg2, arg3);
		if (!*arg2) {
			if (*arg1 && arg1 && is_abbrev(arg1, "remove")) {
				FLOW_SPEED(ch->in_room) = 0;
				send_to_char(ch, "Flow removed from room.\r\n");
				return;
			} else
				send_to_char(ch, 
					"You must specify the flow speed as the second argument.\r\n");
		} else if (!*arg1)
			send_to_char(ch, "Usage: olc rset flow <dir> <speed> <type>\r\n");
		else if (!is_number(arg2) || ((j = atoi(arg2)) < 0))
			send_to_char(ch, "The second argument must be a positive number.\r\n");
		else if ((edir = search_block(arg1, dirs, FALSE)) < 0)
			send_to_char(ch, "What direction to flow in??\r\n");
		else {
			if (*arg3) {
				if (!is_number(arg3)) {
					if ((i = search_block(arg3, flow_types, 0)) < 0) {
						send_to_char(ch, 
							"Invalid flow type... type olc h rflow.\r\n");
						return;
					}
				} else
					i = atoi(arg3);

				FLOW_TYPE(ch->in_room) = i;
				if (FLOW_TYPE(ch->in_room) >= NUM_FLOW_TYPES ||
					FLOW_TYPE(ch->in_room) < 0) {
					send_to_char(ch, "Illegal flow type.\r\n");
					FLOW_TYPE(ch->in_room) = F_TYPE_NONE;
					return;
				}
			} else {
				send_to_char(ch, "Usage: olc rset flow <dir> <speed> <type>\r\n");
				return;
			}

			FLOW_DIR(ch->in_room) = edir;
			FLOW_SPEED(ch->in_room) = j;
			send_to_char(ch, "Flow state set.  HA!\r\n");
		}
		break;
	case 6:	/** occupancy **/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument should be a number.\r\n");
			return;
		}
		if ((i = atoi(arg2)) < 0) {
			send_to_char(ch, "Haha hahaHOoHOhah hah.  Funny.\r\n");
			return;
		}
		ch->in_room->max_occupancy = i;
		send_to_char(ch, "Occupancy set!  Watch yourself.\r\n");
		break;

	case 7:  /** special **/
		if (!*arg2 || (i = find_spec_index_arg(arg2)) < 0)
			send_to_char(ch, "That is not a valid special.\r\n"
				"Type show special room to view a list.\r\n");
		else if (!IS_SET(spec_list[i].flags, SPEC_RM))
			send_to_char(ch, "This special is not for rooms.\r\n");
		else if (IS_SET(spec_list[i].flags, SPEC_RES) && !OLCIMP(ch))
			send_to_char(ch, "This special is reserved.\r\n");
		else {
			ch->in_room->func = spec_list[i].func;
			do_specassign_save(ch, SPEC_RM);
			send_to_char(ch, "Room special set.\r\n");
		}

		break;
	case 8:
		if (!*arg2) {
			send_to_char(ch, "You should set the specparam to something.  Try using ~ to clear it.\r\n");
		} else if (!GET_ROOM_SPEC(ch->in_room)) {
			send_to_char(ch, "You should set a special first!\r\n");
		} else {
			if (GET_ROOM_PARAM(ch->in_room))
				free(GET_ROOM_PARAM(ch->in_room));
			if (*arg2 == '~')
				ch->in_room->func_param = NULL;
			else
				ch->in_room->func_param = strdup(arg2);
			do_specassign_save(ch, SPEC_RM);
			send_to_char(ch, "Room special parameters set.\r\n");
		}

		break;

	default:
		send_to_char(ch, 
			"Sorry, you have attempted to access an unimplemented command.\r\nUnfortunately, you will now be killed.\r\n");
		return;
	}
}

void
do_olc_rexdesc(struct Creature *ch, char *argument, bool is_hedit)
{

	struct extra_descr_data *desc = NULL, *ndesc = NULL, *temp = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	const char *cmd;

	cmd = (is_hedit) ? "hedit extradesc":"olc rexdesc";

	if (!*argument) {
		send_to_char(ch, "Usage: %s <create | remove | edit | addkey> <keyword> [new keywords]\r\n", cmd);
		return;
	}

	half_chop(argument, buf, argument);
	if (!*argument)
		send_to_char(ch, 
			"Which extra description would you like to deal with?\r\n");
	else if (!*buf)
		send_to_char(ch, "Valid commands are: create, remove, edit, addkey.\r\n");
	else if (is_abbrev(buf, "remove")) {
		if ((desc = locate_exdesc(argument, ch->in_room->ex_description))) {
			REMOVE_FROM_LIST(desc, ch->in_room->ex_description, next);
			if (desc->keyword)
				free(desc->keyword);
			else
				slog("WTF?? !desc->keyword??");

			if (desc->description)
				free(desc->description);
			else
				slog("WTF?? !desc->description??");

			free(desc);
			send_to_char(ch, "Description removed.\r\n");
		} else
			send_to_char(ch, "No such extra description.\r\n");

		return;
	} else if (is_abbrev(buf, "create")) {
		if (find_exdesc(argument, ch->in_room->ex_description)) {
			send_to_char(ch, "An extra description already exists with that keyword.\r\n"
				"Use the '%s remove' command to remove it, or the\r\n"
				"'%s edit' command to change it, punk.\r\n", cmd, cmd);
			return;
		}
		CREATE(ndesc, struct extra_descr_data, 1);
		ndesc->keyword = str_dup(argument);
		ndesc->next = ch->in_room->ex_description;
		ch->in_room->ex_description = ndesc;

		start_text_editor(ch->desc, &ch->in_room->ex_description->description,
			true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);

		act("$n begins to write an extra description.", TRUE, ch, 0, 0,
			TO_ROOM);
		return;
	} else if (is_abbrev(buf, "edit")) {
		if ((desc = locate_exdesc(argument, ch->in_room->ex_description))) {
			start_text_editor(ch->desc, &desc->description, true);
			SET_BIT(PLR_FLAGS(ch), PLR_OLC);
			act("$n begins to write an extra description.", TRUE, ch, 0, 0,
				TO_ROOM);
		} else
			send_to_char(ch, 
				"No such description.  Use 'create' to make a new one.\r\n");

		return;
	} else if (is_abbrev(buf, "addkeyword")) {
		half_chop(argument, arg1, arg2);
		if ((desc = locate_exdesc(arg1, ch->in_room->ex_description))) {
			if (!*arg2) {
				send_to_char(ch, 
					"What??  How about giving me some keywords to add...\r\n");
				return;
			} else {
				strcpy(buf, desc->keyword);
				strcat(buf, " ");
				strcat(buf, arg2);
				free(desc->keyword);
				desc->keyword = str_dup(buf);
				send_to_char(ch, "Keywords added.\r\n");
				return;
			}
		} else
			send_to_char(ch, "There is no such description in this room.\r\n");
	} else
		send_to_char(ch, OLC_EXDESC_USAGE);

}




#define HEDIT_USAGE   "HEDIT ( house edit ) usage:\r\n" \
"hedit title <title>\r\n" \
"hedit desc\r\n"          \
"hedit extradesc <create|edit|addkey> <keywords>\r\n"\
"hedit sound\r\n"         \
"hedit save\r\n"          \
"hedit show  [brief] [.] ( lists rooms and contents )\r\n" \
"hedit owner <name>\r\n"

ACMD(do_hedit)
{
	char arg[MAX_INPUT_LENGTH];
	int command, j, cost = 0, num = 0, tot_cost = 0, tot_num = 0;
	bool local = false, brief = false;
	struct room_data *rm = NULL;
	struct obj_data *o = NULL;
	struct house_control_rec *h = 0;

	const char *hedit_options[] = {
		"title",
		"desc",
		"extradesc",
		"sound",
		"save",
		"show",
		"owner",
		"\n"
	};

	if (olc_lock || (IS_SET(ch->in_room->zone->flags, ZONE_LOCKED))) {
		if (GET_LEVEL(ch) >= LVL_IMPL) {
			send_to_char(ch,
				"\007\007\007\007%sWARNING.%s  Overriding olc %s lock.\r\n",
				CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM),
				olc_lock ? "global" : "discrete zone");
		} else {
			send_to_char(ch, "OLC is currently locked.  Try again later.\r\n");
			return;
		}
	}

	if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_HOUSE)) {
		send_to_char(ch, "You have to be in your house to edit it.\r\n");
		return;
	}
	if (!(h = real_house(ch->in_room->number))) {
		send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
		return;
	}
	if ((GET_IDNUM(ch) != h->owner1) &&
		(GET_IDNUM(ch) != h->owner2) && GET_LEVEL(ch) < LVL_GRGOD) {
		send_to_char(ch, "Only the owner can edit the house.\r\n");
		return;
	}
	if (h->mode == HOUSE_RENTAL) {
		send_to_char(ch, "You cannot edit rental houses.\r\n");
		return;
	}

	argument = one_argument(argument, arg);
	skip_spaces(&argument);

	if ((command = search_block(arg, hedit_options, 0)) < 0) {
		send_to_char(ch, HEDIT_USAGE);
		return;
	}

	switch (command) {
	case 0:					/* title */
		if (strlen(argument) > 80) {
			send_to_char(ch, "That's a bit long. Dont you think?\r\n");
			return;
		}
		sprintf(buf, "title %s", argument);
		do_olc_rset(ch, buf);
		break;
	case 1:					/* desc */
		sprintf(buf, "desc %s", argument);
		do_olc_rset(ch, buf);
		break;
	case 3:					/* sound */
		sprintf(buf, "sound %s", argument);
		do_olc_rset(ch, buf);
		break;
	case 2:					/* extra */
		do_olc_rexdesc(ch, argument, true);
		break;
	case 4:					/* save  */
		if (!save_wld(ch))
			send_to_char(ch, "World file saved.\r\n");
		else {
			send_to_char(ch, "An error occured while saving.\r\n");
			slog("SYSERR: Error hedit save in house room %d.",
				ch->in_room->number);
		}
		WAIT_STATE(ch, 8 RL_SEC);
		break;
	case 5:					/* show */
		char tmpbuf[1024];
		tmpbuf[0] = '\0';
		buf[0] = '\0';
		argument = one_argument(argument, arg);
		skip_spaces(&argument);
		if ((*arg && is_abbrev(arg, "brief")) ||
			(*argument && is_abbrev(argument, "brief"))) {
			brief = true;
		}
		if ((*arg && !strncmp(arg, ".", 1)) ||
			(*argument && !strncmp(argument, ".", 1))) {
			local = true;
		}
		if (brief) {
			strcpy(buf,
				"-- House Room --------------------- Items ------- Cost\r\n");
			for (j = 0; j < h->num_of_rooms; j++, num = 0, cost = 0) {
				if (local && h->house_rooms[j] != ch->in_room->number)
					continue;
				if (!(rm = real_room(h->house_rooms[j]))) {
					slog("SYSERR:  house room does not exist!");
					continue;
				}

				for (o = rm->contents; o; o = o->next_content) {
					num += recurs_obj_contents(o, NULL);
					cost += recurs_obj_cost(o, false, NULL);
				}
				tot_cost += cost;
				tot_num += num;
				sprintf(tmpbuf, "%s%-30s%s       %s%3d%s        %5d\r\n",
					CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM),
					(num > MAX_HOUSE_ITEMS) ? CCRED(ch, C_NRM) : "",
					num, CCNRM(ch, C_NRM), cost);

				if (strlen(buf) + strlen(tmpbuf) > MAX_STRING_LENGTH - 256) {
					sprintf(tmpbuf, "%sOVERFLOW\r\n%s", CCRED(ch, C_NRM),
						CCNRM(ch, C_NRM));
					strcat(buf, tmpbuf);
					break;
				} else {
					strcat(buf, tmpbuf);
				}
			}
			if (!local)
				sprintf(buf,
					"%s"
					"- Totals -------------------------  %4d        %5d\r\n",
					buf, tot_num, tot_cost);
			page_string(ch->desc, buf);
			return;
		}
		// not brief mode
		if (local) {
			*buf = 0;
			print_room_contents_to_buf(ch, buf, ch->in_room);
			page_string(ch->desc, buf);
		} else
			hcontrol_list_house_rooms(ch, h->house_rooms[0]);
		break;

		// owner
	case 6:
		{
			long idnum;
			argument = one_argument(argument, arg);
			idnum = get_id_by_name(arg);

			if (GET_IDNUM(ch) != h->owner1) {
				send_to_char(ch, "Sorry, only the primary owner can do this.\r\n");
				return;
			}
			if (idnum <= 0) {
				send_to_char(ch, "There is no player named '%s'.\r\n", arg);
				return;
			}

			if (GET_IDNUM(ch) == idnum) {
				send_to_char(ch, "You cannot Co-own with yourself.\r\n");
				return;
			}

			if (h->owner2 == idnum) {
				h->owner2 = 0;
				House_save_control();
				send_to_char(ch, "Co-owner removed.\r\n");
				WAIT_STATE(ch, 2 RL_SEC);
				return;
			}

			h->owner2 = idnum;
			House_save_control();
			send_to_char(ch, "Co-owner added.\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			return;
			break;
		}

	default:
		send_to_char(ch, "Error code <69> -- fatal allocation in do_hedit.\r\n");
		break;
	}
}
