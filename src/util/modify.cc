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


void show_string(struct descriptor_data *d, char *input);
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

ACMD(do_skillset)
{
	struct char_data *vict;
	char name[MAX_INPUT_LENGTH], buf2[100], buf[100], help[MAX_STRING_LENGTH];
	int skill, value, i, qend;

	argument = one_argument(argument, name);

	if (!*name) {				/* no arguments. print an informative text */
		send_to_char("Syntax: skillset <name> '<skill>' <value>\r\n", ch);
		strcpy(help, "Skill being one of the following:\r\n");
		for (i = 0; *spells[i] != '\n'; i++) {
			if (*spells[i] == '!')
				continue;
			sprintf(help + strlen(help), "%18s", spells[i]);
			if (i % 4 == 3) {
				strcat(help, "\r\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\r\n", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, name))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	/* If there is no chars in argument */
	if (!*argument) {
		send_to_char("Skill name expected.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		send_to_char("Skill must be enclosed in: ''\r\n", ch);
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	if (*(argument + qend) != '\'') {
		send_to_char("Skill must be enclosed in: ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = find_skill_num(help)) <= 0) {
		send_to_char("Unrecognized skill.\r\n", ch);
		return;
	}
	argument += qend + 1;		/* skip to next parameter */
	argument = one_argument(argument, buf);

	if (!*buf) {
		send_to_char("Learned value expected.\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0) {
		send_to_char("Minimum value for learned is 0.\r\n", ch);
		return;
	}
	if (value > 120) {
		send_to_char("Max value for learned is 120.\r\n", ch);
		return;
	}
	if (IS_NPC(vict)) {
		send_to_char("You can't set NPC skills.\r\n", ch);
		return;
	}
	sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
		spells[skill], value);
	mudlog(buf2, BRF, -1, TRUE);

	SET_SKILL(vict, skill, value);

	sprintf(buf2, "You change %s's %s to %d.\r\n", GET_NAME(vict),
		spells[skill], value);
	send_to_char(buf2, ch);
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
				*(first_arg + look_at) = LOWER(*(argument + begin + look_at));

			if (*(argument + begin + look_at) == '\"')
				begin++;

		} else {

			for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
				*(first_arg + look_at) = LOWER(*(argument + begin + look_at));

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
show_file(struct char_data *ch, char *fname, int lines)
{
	char *logbuf = NULL;
	int size;
	fstream file;

	file.open(fname, ios::in);
	if (!file) {
		send_to_char("It seems to be empty.\r\n", ch);
		return;
	}

	file.seekg(0, ios::end);
	if (!file.tellg()) {
		send_to_char("It seems to be empty.\r\n", ch);
		return;
	}
	file.seekg(0, ios::beg);
	if (lines > 0) {
		if (lines > 100) {
			send_to_char
				("If you want that many lines, you might as well read the whole thing.\r\n",
				ch);
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
		size = file.tellg() + 1;
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
	page_string(ch->desc, buf, 0);
}

void
page_string(struct descriptor_data *d, char *str, int keep_internal)
{
	if (!d || !str)
		return;

	if (keep_internal) {
		CREATE(d->showstr_head, char, strlen(str) + 1);
		strcpy(d->showstr_head, str);
		d->showstr_point = d->showstr_head;
	} else
		d->showstr_point = str;

	show_string(d, "");
}



void
show_string(struct descriptor_data *d, char *input)
{
	char buffer[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH];
	register char *scan, *chk;
	int lines = 0, toggle = 1, page_length;

	one_argument(input, buf);
	if (IS_NPC(d->character))
		page_length = 22;
	else
		page_length = GET_PAGE_LENGTH(d->character);

	if (*buf) {
		if (d->showstr_head) {
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			free(d->showstr_head);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			d->showstr_head = 0;
		}
		d->showstr_point = 0;
		return;
	}
	/* show a chunk */
	for (scan = buffer;; scan++, d->showstr_point++) {
		if ((((*scan = *d->showstr_point) == '\n') || (*scan == '\r')) &&
			((toggle = -toggle) < 0))
			lines++;
		else if (!*scan || (lines >= page_length)) {
			if (lines >= page_length) {
				/* We need to make sure that if we're breaking the input here, we
				 * must set showstr_point to the right place and null terminate the
				 * character after the last '\n' or '\r' so we don't lose it.
				 * -- Michael Buselli
				 */
				d->showstr_point++;
				*(++scan) = '\0';
			}
			SEND_TO_Q(buffer, d);

			/* see if this is the end (or near the end) of the string */
			for (chk = d->showstr_point; isspace(*chk); chk++);
			if (!*chk) {
				if (d->showstr_head) {
#ifdef DMALLOC
					dmalloc_verify(0);
#endif
					free(d->showstr_head);
#ifdef DMALLOC
					dmalloc_verify(0);
#endif
					d->showstr_head = 0;
				}
				d->showstr_point = 0;
			}
			return;
		}
	}
}
