//
// File: olc_ticl.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/*******************************************************************
*  File: olc_ticl.c                              Part of TempusMUD *
* Usage: Tempus Interpreted Command Language                       *
*                                                                  *
* All rights reserved.                                             *
*                                                                  *
* Copyright (c) 1996 by Chris Austin           (Stryker@TempusMUD) *
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
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
#include "specs.h"

#define MAX_TICL_LENGTH 4000

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern int olc_lock;
extern int *ticl_index;

long asciiflag_conv(char *buf);
extern char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
	arg3[MAX_INPUT_LENGTH];

char *one_argument_no_lower(char *argument, char *first_arg);
int search_block_no_lower(char *arg, char **list, bool exact);
int fill_word_no_lower(char *argument);
void num2str(char *str, int num);

const char *olc_tset_keys[] = {
	"title",
	"code",
	"\n"
};

#define NUM_TSET_COMMANDS 2

struct ticl_data *
do_create_ticl(struct char_data *ch, int vnum)
{
	struct zone_data *zone = NULL;
	struct ticl_data *ticl = NULL, *new_ticl = NULL;

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum >= zone->number * 100 && vnum <= zone->top)
			break;

	if (!zone) {
		send_to_char
			("ERROR: A zone must be defined for the TICL proc first.\r\n", ch);
		return (NULL);
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char("You can't touch this zone, buttmunch.\r\n", ch);
		sprintf(buf, "OLC: %s failed attempt to CREATE TICL %d.", GET_NAME(ch),
			vnum);
		mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
		return (NULL);
	}

	if (!ZONE_FLAGGED(zone, ZONE_TICL_APPROVED)) {
		send_to_char("TICL OLC is not approved for this zone.\r\n", ch);
		return (NULL);
	}

	for (ticl = zone->ticl_list; ticl; ticl = ticl->next)
		if (ticl->vnum == vnum) {
			send_to_char("A TICL proc already exists with that vnum.\r\n", ch);
			return (NULL);
		}

	for (ticl = zone->ticl_list; ticl; ticl = ticl->next)
		if (vnum > ticl->vnum && (!ticl->next || vnum < ticl->next->vnum))
			break;

	CREATE(new_ticl, struct ticl_data, 1);

	new_ticl->vnum = vnum;
	new_ticl->proc_id = 0;
	new_ticl->creator = GET_IDNUM(ch);
	new_ticl->date_created = time(0);
	new_ticl->last_modified = time(0);
	new_ticl->last_modified_by = GET_IDNUM(ch);
	new_ticl->times_executed = 0;
	new_ticl->flags = 0;
	new_ticl->compiled = 0;
	new_ticl->title = str_dup("A new TICL proc.");
	new_ticl->code = NULL;

	if (ticl) {
		new_ticl->next = ticl->next;
		ticl->next = new_ticl;
	} else {
		new_ticl->next = NULL;
		zone->ticl_list = new_ticl;
	}

	return (new_ticl);
}

void
do_ticl_tedit(struct char_data *ch, char *argument)
{
	struct ticl_data *ticl = NULL, *tmp_ticl = NULL;
	struct zone_data *zone = NULL;
	struct descriptor_data *d = NULL;
	int j;

	ticl = GET_OLC_TICL(ch);

	if (!*argument) {
		if (!ticl)
			send_to_char("You are not currently editing a TICL proc.\r\n", ch);
		else {
			sprintf(buf, "Current olc TICL proc: [%5d] %s\r\n",
				ticl->vnum, ticl->title);
			send_to_char(buf, ch);
		}
		return;
	}
	if (!is_number(argument)) {
		if (is_abbrev(argument, "exit")) {
			send_to_char("Exiting TICL editor.\r\n", ch);
			GET_OLC_TICL(ch) = NULL;
			return;
		}
		send_to_char("The argument must be a number.\r\n", ch);
		return;
	} else {
		j = atoi(argument);

		for (zone = zone_table; zone; zone = zone->next)
			if (j < zone->top)
				break;

		if (!zone) {
			send_to_char("There is no such TICL proc, buttmunch.\r\n", ch);
			return;
		}

		for (tmp_ticl = zone->ticl_list; tmp_ticl; tmp_ticl = tmp_ticl->next)
			if (tmp_ticl->vnum == j)
				break;

		if (tmp_ticl == NULL)
			send_to_char("There is no such TICL proc.\r\n", ch);
		else {
			for (zone = zone_table; zone; zone = zone->next)
				if (j < zone->top)
					break;
			if (!zone) {
				send_to_char
					("That TICL proc does not belong to any zone!!\r\n", ch);
				slog("SYSERR: mobile not in any zone.");
				return;
			}

			if (!CAN_EDIT_ZONE(ch, zone)) {
				send_to_char
					("You do not have permission to edit those TICL procs.\r\n",
					ch);
				return;
			}

			if (!ZONE_FLAGGED(zone, ZONE_MOBS_APPROVED) && !OLCGOD(ch)) {
				send_to_char("TICL OLC is not approved for this zone.\r\n",
					ch);
				return;
			}

			for (d = descriptor_list; d; d = d->next) {
				if (d->character && GET_OLC_TICL(d->character) == tmp_ticl) {
					act("$N is already editing that TICL proc.", FALSE, ch, 0,
						d->character, TO_CHAR);
					return;
				}
			}

			GET_OLC_TICL(ch) = tmp_ticl;
			sprintf(buf, "Now editing TICL [%d] %s%s%s\r\n",
				tmp_ticl->vnum,
				CCGRN(ch, C_NRM), tmp_ticl->title, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}
}

void
do_ticl_tstat(struct char_data *ch)
{
	struct ticl_data *ticl = NULL;
	char tim1[30], tim2[30];

	ticl = GET_OLC_TICL(ch);

	if (!ticl)
		send_to_char("You are not currently editing a TICL proc.\r\n", ch);
	else {
		strcpy(tim1, (char *)asctime(localtime(&(ticl->date_created))));
		strcpy(tim2, (char *)asctime(localtime(&(ticl->last_modified))));
		tim1[10] = tim2[10] = '\0';
		sprintf(buf, "\r\n"
			"VNUM:  [%s%5d%s]    Title: %s%s%s\r\n"
			"Created By:       [%s%-15s%s] on [%s%s%s]\r\n"
			"Last Modified By: [%s%-15s%s] on [%s%s%s]\r\n\r\n"
			"-----------------------------------------------------------------\r\n\r\n",
			CCGRN(ch, C_NRM),
			ticl->vnum,
			CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM),
			ticl->title,
			CCNRM(ch, C_NRM),
			CCYEL(ch, C_NRM),
			get_name_by_id(ticl->creator),
			CCNRM(ch, C_NRM),
			CCGRN(ch, C_NRM),
			tim1,
			CCNRM(ch, C_NRM),
			CCYEL(ch, C_NRM),
			get_name_by_id(ticl->last_modified_by),
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), tim2, CCNRM(ch, C_NRM));

		send_to_char(buf, ch);

		if (ticl->code != NULL)
			page_string(ch->desc, ticl->code, 1);
	}
}


#define ticl_p GET_OLC_TICL(ch)
#define OLC_REPLY_USAGE "olc mset reply  <create | remove | edit | addkey>" \
"<keywords> [new keywords]\r\n"

void
do_ticl_tset(struct char_data *ch, char *argument)
{
	int i, tset_command;		/* tmp_flags, flag, cur_flags, state; */

	if (!ticl_p) {
		send_to_char("You are not currently editing a TICL proc.\r\n", ch);
		return;
	}

	if (!*argument) {
		strcpy(buf, "Valid tset commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (i < NUM_TSET_COMMANDS) {
			strcat(buf, olc_tset_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf, 1);
		return;
	}

	half_chop(argument, arg1, arg2);
	skip_spaces(&argument);

	if ((tset_command = search_block(arg1, olc_tset_keys, FALSE)) < 0) {
		sprintf(buf, "Invalid tset command '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}
	if (tset_command != 1 && !*arg2) {
		sprintf(buf, "Set %s to what??\r\n", olc_tset_keys[tset_command]);
		send_to_char(buf, ch);
		return;
	}


	switch (tset_command) {
	case 0:			  /** Title **/
		if (ticl_p->title != NULL) {
			free(ticl_p->title);
			ticl_p->title = str_dup(argument);
		} else
			ticl_p->title = str_dup(argument);
		sprintf(buf, "The title of this proc is now %s.\r\n", ticl_p->title);
		send_to_char(buf, ch);
		break;
	case 1:			 /** Code **/
		if (ticl_p->code == NULL) {
			CREATE(ticl_p->code, char, MAX_TICL_LENGTH);
			act("$n begins to code a TICL proc.", TRUE, ch, 0, 0, TO_ROOM);
		} else {
			act("$n begins to edit a TICL proc.", TRUE, ch, 0, 0, TO_ROOM);
		}
		start_text_editor(ch->desc, &ticl_p->code, true, MAX_TICL_LENGTH);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		break;
	default:
		break;
	}
}

int
write_ticl_index(struct char_data *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; ticl_index[i] != -1; i++) {
		count++;
		if (ticl_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (ticl_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (ticl_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = ticl_index[i];
			done = 1;
		} else
			new_index[j] = ticl_index[i];
		j++;
	}

	free(ticl_index);

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	ticl_index = new_index;

	sprintf(fname, "world/ticl/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char("Could not open index file, TICL save aborted.\r\n", ch);
		return (0);
	}

	for (i = 0; ticl_index[i] != -1; i++)
		fprintf(index, "%d.ticl\n", ticl_index[i]);

	fprintf(index, "$\n");

	send_to_char("TICL index file re-written.\r\n", ch);

	fclose(index);

	return (1);
}

int
save_ticls(struct char_data *ch)
{
	unsigned int tmp;
	int t_vnum;
	char fname[64];
	struct zone_data *zone;
	struct ticl_data *ticl;
	FILE *file;
	FILE *realfile;

	if (GET_OLC_TICL(ch)) {
		ticl = GET_OLC_TICL(ch);
		t_vnum = ticl->vnum;
		for (zone = zone_table; zone; zone = zone->next)
			if (t_vnum >= zone->number * 100 && t_vnum <= zone->top)
				break;
		if (!zone) {
			slog("OLC: ERROR finding zone for TICL %d.", t_vnum);
			send_to_char("Unable to match TICL with zone error..\r\n", ch);
			return 1;
		}
	} else
		zone = ch->in_room->zone;

	sprintf(fname, "world/ticl/olc/%d.ticl", zone->number);
	if (!(file = fopen(fname, "w")))
		return 1;

	if ((write_ticl_index(ch, zone)) != 1) {
		fclose(file);
		return (1);
	}

	for (ticl = zone->ticl_list; ticl; ticl = zone->ticl_list->next) {

		fprintf(file, "#%d\n", ticl->vnum);

		if (ticl->title)
			fprintf(file, "%s", ticl->title);
		fprintf(file, "~\n");

		fprintf(file, "%ld %ld %ld %ld %d\n", ticl->creator,
			ticl->date_created,
			ticl->last_modified, ticl->last_modified_by, ticl->compiled);

		if (ticl->code)
			fprintf(file, "%s", ticl->code);
		fprintf(file, "~\n");
	}

	fprintf(file, "$\n");

	sprintf(fname, "world/ticl/%d.ticl", zone->number);
	realfile = fopen(fname, "w");
	if (realfile) {
		fclose(file);
		sprintf(fname, "world/ticl/olc/%d.ticl", zone->number);
		if (!(file = fopen(fname, "r"))) {
			slog("SYSERR: Failure to reopen olc TICL file.");
			send_to_char
				("OLC Error: Failure to duplicate TICL file in main dir."
				"\r\n", ch);
			fclose(realfile);
			return 0;
		}
		do {
			tmp = fread(buf, 1, 512, file);
			if (fwrite(buf, 1, tmp, realfile) != tmp) {
				slog("SYSERR: Failure to duplicate olc TICL file in the main wld dir.");
				send_to_char
					("OLC Error: Failure to duplicate TICL file in main dir."
					"\r\n", ch);
				fclose(realfile);
				fclose(file);
				return 0;
			}
		} while (tmp == 512);

		fclose(realfile);

	}
	fclose(file);

	REMOVE_BIT(zone->flags, ZONE_MOBS_MODIFIED);
	return 0;
}
