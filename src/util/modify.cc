/* ************************************************************************
*   File: modify.c                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: modify.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "login.h"
#include "screen.h"

/* External Variables */

extern char *news;
extern char *motd;
extern char *imotd;
extern char *info;
extern char *wizlist;
extern char *immlist;
extern char *background;
extern char *handbook;
extern char *policies;
extern char *areas;


void show_string(struct descriptor_data *d);
extern struct descriptor_data *descriptor_list;

char *string_fields[] = {
	"name",
	"short",
	"long",
	"description",
	"title",
	"delete-description",
	"\n"
};


/* maximum length for text field x+1 */
int length[] = {
	15,
	60,
	256,
	240,
	60
};


/* **********************************************************************
*  Modification of character skills                                     *
********************************************************************** */
void
perform_skillset(Creature *ch, Creature *vict, char *skill_str, int value)
{
	int skill;

	if ((skill = find_skill_num(skill_str)) <= 0) {
		send_to_char(ch, "Unrecognized skill.\r\n");
		return;
	}
	if (value < 0) {
		send_to_char(ch, "Minimum value for a skill is 0.\r\n");
		return;
	}
	if (value > 120) {
		send_to_char(ch, "Max value for a skill is is 120.\r\n");
		return;
	}
	if (IS_NPC(vict)) {
		send_to_char(ch, "You can't set skills on an NPC.\r\n");
		return;
	}
	mudlog(0, BRF, true,
		"%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
		spell_to_str(skill), value);

	SET_SKILL(vict, skill, value);

	send_to_char(ch, "You change %s's %s to %d.\r\n", GET_NAME(vict),
		spell_to_str(skill), value);

}

ACMD(do_skillset)
{
	Creature *vict;
	char *vict_name, *skill, *val_str;

	vict_name = tmp_getword(&argument);
	skill = tmp_getquoted(&argument);
	val_str = tmp_getword(&argument);
	if (!vict_name || !skill || !val_str) {
		send_to_char(ch, "skillset <name> '<skill>' <value>\n");
		return;
	}

	vict = get_char_vis(ch, vict_name);
	if (!vict) {
		send_to_char(ch, NOPERSON);
		return;
	}
	
	perform_skillset(ch, vict, skill, atoi(val_str));
}

/* db stuff *********************************************** */


/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *
one_word(char *argument, char *first_arg)
{
	int found, begin, look_at;

	found = begin = 0;

	do {
		for (; isspace(*(argument + begin)); begin++);

		if (*(argument + begin) == '\"') {	/* is it a quote */

			begin++;

			for (look_at = 0; (*(argument + begin + look_at) >= ' ') &&
				(*(argument + begin + look_at) != '\"'); look_at++)
				*(first_arg + look_at) = tolower(*(argument + begin + look_at));

			if (*(argument + begin + look_at) == '\"')
				begin++;

		} else {

			for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
				*(first_arg + look_at) = tolower(*(argument + begin + look_at));

		}

		*(first_arg + look_at) = '\0';
		begin += look_at;
	} while (fill_word(first_arg));

	return (argument + begin);
}


struct help_index_element *
build_help_index(FILE * fl, int *num)
{
	int nr = -1, issorted, i;
	struct help_index_element *list = 0, mem;
	char buf[128], tmp[128], *scan;
	long pos;
	int count_hash_records(FILE * fl);

	i = count_hash_records(fl) * 5;
	rewind(fl);
	CREATE(list, struct help_index_element, i);

	for (;;) {
		pos = ftell(fl);
		fgets(buf, 128, fl);
		*(buf + strlen(buf) - 1) = '\0';
		scan = buf;
		for (;;) {
			/* extract the keywords */
			scan = one_word(scan, tmp);

			if (!*tmp)
				break;

			nr++;

			list[nr].pos = pos;
			CREATE(list[nr].keyword, char, strlen(tmp) + 1);
			strcpy(list[nr].keyword, tmp);
		}
		/* skip the text */
		do
			fgets(buf, 128, fl);
		while (*buf != '#');
		if (*(buf + 1) == '~')
			break;
	}
	/* we might as well sort the stuff */
	do {
		issorted = 1;
		for (i = 0; i < nr; i++)
			if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0) {
				mem = list[i];
				list[i] = list[i + 1];
				list[i + 1] = mem;
				issorted = 0;
			}
	} while (!issorted);

	*num = nr;
	return (list);
}

void
show_file(struct Creature *ch, char *fname, int lines)
{
	char *logbuf = NULL;
	int size;
	fstream file;

	file.open(fname, ios::in);
	if (!file) {
		send_to_char(ch, "It seems to be empty.\r\n");
		return;
	}

	file.seekg(0, ios::end);
	if (!file.tellg()) {
		send_to_char(ch, "It seems to be empty.\r\n");
		return;
	}
	file.seekg(0, ios::beg);
	if (lines > 0) {
		if (lines > 100) {
			send_to_char(ch, 
				"If you want that many lines, you might as well read the whole thing.\r\n");
			return;
		}
		int tot, i;
		int x = MAX_RAW_INPUT_LENGTH + 1;
		tot = i = 0;
		strcpy(buf, "");

		logbuf = new char[(lines + 1) * x];
		if (!logbuf) {
			slog("Memory not allocated in show_file.");
			return;
		}


		for (i = 0; i < lines; i++)
			strcpy((logbuf + (i * x)), "");

		i = 0;
		while (!file.eof()) {
			file.getline((logbuf + (i * x)), x - 1, '\n');
			tot++;
			i == lines ? i = 0 : i++;
		}
		if (tot < lines) {
			for (i = 0; i < tot; i++) {
				strcat(buf, (logbuf + (i * x)));
				strcat(buf, "\r\n");
			}
		} else {
			for (tot = lines; tot; i == lines ? i = 0 : i++) {
				strcat(buf, (logbuf + (i * x)));
				strcat(buf, "\r\n");
				tot--;
			}
		}
		delete[]logbuf;
	} else {
		file.seekg(0, ios::end);
		size = (int)file.tellg() + 1;
		logbuf = new char[MAX_RAW_INPUT_LENGTH + 1];
		file.seekg(0, ios::beg);
		strcpy(buf, "");
		while (!file.eof()) {
			file.getline(logbuf, MAX_RAW_INPUT_LENGTH, '\n');
			strcat(buf, logbuf);
			strcat(buf, "\r\n");
		}
		delete[]logbuf;
	}
	file.close();
	page_string(ch->desc, buf);
}

void
page_string(struct descriptor_data *d, char *str)
{
	if (!d || !str)
		return;

	if (d->showstr_head)
		free(d->showstr_head);

	CREATE(d->showstr_head, char, strlen(str) + 1);
	strcpy(d->showstr_head, str);
	d->showstr_point = d->showstr_head;

	show_string(d);
}

void
show_string(struct descriptor_data *d)
{
	register char *read_pt;
	int page_length;
	int pt_save;

	if (IS_NPC(d->character))
		page_length = 22;
	else
		page_length = GET_PAGE_LENGTH(d->character);

	read_pt = d->showstr_point;
	while (*read_pt && page_length) {
		while (*read_pt && '\n' != *read_pt && '\r' != *read_pt)
			read_pt++;
		if (*read_pt) {
			page_length--;
			read_pt++;
			if ('\n' == *read_pt || '\r' == *read_pt)
				read_pt++;
		}
	}

	pt_save = *read_pt;
	*read_pt = '\0';

	SEND_TO_Q(d->showstr_point, d);

	if (pt_save) {
		*read_pt = pt_save;
		d->showstr_point = read_pt;
		if(d->character)
			send_to_char(d->character,
				"%s**** %sUse the 'more' command to continue. %s****%s\r\n",
				CCRED(d->character, C_NRM), CCNRM(d->character, C_NRM),
				CCRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
		else
			SEND_TO_Q("**** Press return to continue, 'q' to quit ****\r\n", d);
	} else {
		free(d->showstr_head);
		d->showstr_head = d->showstr_point = NULL;
	}
}
