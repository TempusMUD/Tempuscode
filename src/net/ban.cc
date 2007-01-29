/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: ban.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

struct ban_list_element *ban_list = NULL;



char *ban_types[] = {
	"no",
	"new",
	"select",
	"all",
	"ERROR"
};


void
load_banned(void)
{
	FILE *fl;
	int i;
	char *read_pt, *ban_type;
	struct ban_list_element *next_node;

	ban_list = 0;

	if (!(fl = fopen(BAN_FILE, "r"))) {
		perror("Unable to open banfile");
		return;
	}
	while (fgets(buf, MAX_STRING_LENGTH, fl)) {
		if (!buf[0] || buf[0] == '#')
			continue;
		buf[strlen(buf) - 1] = '\0';
		read_pt = buf;
		CREATE(next_node, struct ban_list_element, 1);
		ban_type = tmp_getword(&read_pt);

		strncpy(next_node->site, tmp_getword(&read_pt), BANNED_SITE_LENGTH);
		next_node->site[BANNED_SITE_LENGTH] = '\0';

		next_node->date = atol(tmp_getword(&read_pt));

		strncpy(next_node->name, tmp_getword(&read_pt), MAX_NAME_LENGTH);
		next_node->name[MAX_NAME_LENGTH] = '\0';
		next_node->name[0] = toupper(next_node->name[0]);

		strncpy(next_node->reason, read_pt, 79);
		next_node->name[79] = '\0';

		for (i = BAN_NOT; i <= BAN_ALL; i++)
			if (!strcmp(ban_type, ban_types[i]))
				next_node->type = i;

		next_node->next = ban_list;
		ban_list = next_node;
	}

	fclose(fl);
}


int
isbanned(char *hostname, char *blocking_hostname)
{
	int i;
	struct ban_list_element *banned_node;
	char *nextchar;

	if (!hostname || !*hostname)
		return (0);

	i = 0;
	for (nextchar = hostname; *nextchar; nextchar++)
		*nextchar = tolower(*nextchar);

	for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
		if (strstr(hostname, banned_node->site)) {	/* if hostname is a substring */
			i = MAX(i, banned_node->type);
			strcpy(blocking_hostname, banned_node->site);
		}

	return i;
}


void
_write_one_node(FILE * fp, struct ban_list_element *node)
{
	if (node) {
		_write_one_node(fp, node->next);
		fprintf(fp, "%s %s %ld %s %s\n", ban_types[node->type],
			node->site, (long)node->date, node->name, node->reason);
	}
}



void
write_ban_list(void)
{
	FILE *fl;

	if (!(fl = fopen(BAN_FILE, "w"))) {
		perror("write_ban_list");
		return;
	}
	_write_one_node(fl, ban_list);	/* recursively write from end to start */
	fclose(fl);
	return;
}


ACMD(do_ban)
{
	char *nextchar, timestr[30];
	int i;
	struct ban_list_element *ban_node;
	char *write_pt, *site, *flag;

	*buf = '\0';

	if (!*argument || GET_LEVEL(ch) < LVL_CAN_BAN) {
		if (!ban_list) {
			send_to_char(ch, "No sites are banned.\r\n");
			return;
		}

		strcpy(buf, "* Site             Date            Banned by   Reason\r\n");
		strcat(buf, "- ---------------  --------------  ----------  -------------------------------\r\n");
		write_pt = buf + strlen(buf);

		for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
			if (ban_node->date) {
				strftime(timestr, 29, "%a %m/%d/%Y", localtime(&ban_node->date));
			} else
				strcpy(timestr, "Unknown");
			sprintf(buf2, "%c %-15s  %-14s  %-10s  %-30s\r\n",
				toupper(ban_types[ban_node->type][0]), ban_node->site,
				timestr, ban_node->name, ban_node->reason);
			strcpy(write_pt, buf2);
			write_pt += strlen(buf2);
		}
		page_string(ch->desc, buf);
		return;
	}
	flag = tmp_getword(&argument);
	site = tmp_getword(&argument);
	if (!*site || !*flag) {
		send_to_char(ch, "Usage: ban {all | select | new} site_name\r\n");
		return;
	}
	if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all")
			|| !str_cmp(flag, "new"))) {
		send_to_char(ch, "Flag must be ALL, SELECT, or NEW.\r\n");
		return;
	}
	for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
		if (!str_cmp(ban_node->site, site)) {
			send_to_char(ch, 
				"That site has already been banned -- unban it to change the ban type.\r\n");
			return;
		}
	}

	if (isbanned(site, buf)) {
		send_to_char(ch, "That site is already blocked as '%s'.\r\n", buf);
		return;
	}

	CREATE(ban_node, struct ban_list_element, 1);
	strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
	for (nextchar = ban_node->site; *nextchar; nextchar++)
		*nextchar = tolower(*nextchar);
	ban_node->site[BANNED_SITE_LENGTH] = '\0';
	strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
	ban_node->name[MAX_NAME_LENGTH] = '\0';
	ban_node->date = time(0);
	strncpy(ban_node->reason, argument, MAX_NAME_LENGTH);
	ban_node->reason[MAX_NAME_LENGTH] = '\0';
	
	for (i = BAN_NEW; i <= BAN_ALL; i++)
		if (!str_cmp(flag, ban_types[i]))
			ban_node->type = i;

	ban_node->next = ban_list;
	ban_list = ban_node;

	mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
		"%s has banned %s for %s players.", GET_NAME(ch), site,
		ban_types[ban_node->type]);
	send_to_char(ch, "Site banned.\r\n");
	write_ban_list();
}


ACMD(do_unban)
{
	char site[MAX_INPUT_LENGTH];
	struct ban_list_element *ban_node, *temp;
	int found = 0;

	one_argument(argument, site);
	if (!*site) {
		send_to_char(ch, "A site to unban might help.\r\n");
		return;
	}
	ban_node = ban_list;
	while (ban_node && !found) {
		if (!str_cmp(ban_node->site, site))
			found = 1;
		else
			ban_node = ban_node->next;
	}

	if (!found) {
		send_to_char(ch, "That site is not currently banned.\r\n");
		return;
	}
	REMOVE_FROM_LIST(ban_node, ban_list, next);
	send_to_char(ch, "Site unbanned.\r\n");
	mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
		"%s removed the %s-player ban on %s.",
		GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(ban_node);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	write_ban_list();
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)                  *
 *  Written by Sharon P. Goza                                                  *
 **************************************************************************/

namestring *invalid_list = NULL;
namestring *nasty_list = NULL;
int num_invalid = 0;
int num_nasty = 0;

int
Valid_Name(char *newname)
{
	int alpha_hist[256];
	int i, len;
	char tempname[MAX_NAME_LENGTH];

	len = strlen(newname);
    if( len > (int)MAX_NAME_LENGTH )
        return 0;

	if (len < 3)
		return 0;

	/* return valid if list doesn't exist */
	if (!invalid_list || num_invalid < 1)
		return 1;

	/* change to lowercase */
	strncpy(tempname, newname,MAX_NAME_LENGTH);
	for (i = 0; i < len; i++) {
		if (!isalpha(tempname[i]) && tempname[i] != '\'')
			return 0;
		tempname[i] = tolower(tempname[i]);
	}

	/* Does the desired name contain a string in the invalid list? */
	for (i = 0; i < num_invalid; i++)
		if (strstr(tempname, invalid_list[i]))
			return 0;

	// Build histogram of characters used
	for (i = 0;i < 256;i++)
		alpha_hist[i] = 0;
	for (i = 0; i < len; i++)
		alpha_hist[(int)tempname[i]] += 1;

	// All names must have at least one vowel
	if (!(alpha_hist[(int)'a'] || alpha_hist[(int)'e'] || alpha_hist[(int)'i'] ||
			alpha_hist[(int)'o'] || alpha_hist[(int)'u'] || alpha_hist[(int)'y']))
		return 0;

	// Check that no character is used more than half the length of the string,
	// rounded up, excluding the first character of the name
	// Oog is ok, Ooog is ok, Oooog is not
	for (i = 0; i < 256; i++)
		if (alpha_hist[i] - ((i == tempname[0]) ? 1:0) > len / 2)
			return 0;

	if (alpha_hist[(int)'\''] > 1)
		return 0;

	// no 's at end of name
	if (tempname[len - 1] == 's' && tempname[len - 2] == '\'')
		return 0;

	return 1;
}

bool
Nasty_Words(const char *words)
{
	int i;
	char tempword[MAX_STRING_LENGTH];

	if (!nasty_list || num_nasty < 1)
		return false;

	/* change to lowercase */
	strcpy(tempword, words);
	for (i = 0; tempword[i]; i++)
		tempword[i] = tolower(tempword[i]);

	for (i = 0; i < num_nasty; i++)
		if (strstr(tempword, nasty_list[i]))
			return true;

	return false;
}


void
Read_Invalid_List(void)
{
	FILE *fp;
	int i = 0;
	char string[128];

	if (!(fp = fopen(XNAME_FILE, "r"))) {
		perror("Unable to open invalid name file");
		return;
	}
	/* count how many records */
	while (fgets(string, 80, fp) != NULL && strlen(string) > 1)
		num_invalid++;

	rewind(fp);

	CREATE(invalid_list, namestring, num_invalid);

	for (i = 0; i < num_invalid; i++) {
		fgets(invalid_list[i], MAX_NAME_LENGTH, fp);	/* read word */
		invalid_list[i][strlen(invalid_list[i]) - 1] = '\0';	/* cleave off \n */
	}
	fclose(fp);
}

void
Read_Nasty_List(void)
{
	FILE *fp;
	int i = 0;
	char string[128];

	if (!(fp = fopen(NASTY_FILE, "r"))) {
		perror("Unable to open nasty words file");
		return;
	}
	/* count how many records */
	while (fgets(string, 80, fp) != NULL && strlen(string) > 1)
		num_nasty++;

	rewind(fp);

	CREATE(nasty_list, namestring, num_nasty);

	for (i = 0; i < num_nasty; i++) {
		fgets(nasty_list[i], MAX_NAME_LENGTH, fp);	/* read word */
		nasty_list[i][strlen(nasty_list[i]) - 1] = '\0';	/* cleave off \n */
	}
	fclose(fp);
}
