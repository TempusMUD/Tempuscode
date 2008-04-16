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

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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
#include "ban.h"
#include "accstr.h"

std::list<ban_entry> ban_list;

char *ban_types[] = {
	"no",
	"new",
	"select",
	"all",
	"ERROR"
};

void
load_banned_entry(xmlNodePtr node)
{
    ban_entry ban;
    xmlNodePtr child;
    char *text;

    for (child = node->children;child;child = child->next) {
        if (xmlMatches(child->name, "site")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban._site, text);
            free(text);
        } else if (xmlMatches(child->name, "name")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban._name, text);
            free(text);
        } else if (xmlMatches(child->name, "reason")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban._reason, text);
            free(text);
        } else if (xmlMatches(child->name, "type")) {
            text = (char *)xmlNodeGetContent(child);
            if (!strcmp(text, "all"))
                ban._type = BAN_ALL;
            else if (!strcmp(text, "select"))
                ban._type = BAN_SELECT;
            else if (!strcmp(text, "new"))
                ban._type = BAN_NEW;
            free(text);
        } else if (xmlMatches(child->name, "date")) {
            text = (char *)xmlNodeGetContent(child);
            ban._date = atol(text);
            free(text);
        }
    }

    ban_list.push_back(ban);
}

void
load_banned(void)
{
	xmlDocPtr doc;
	xmlNodePtr node;

    ban_list.clear();

    doc = xmlParseFile(BAN_FILE);
    if (!doc) {
        errlog("Couldn't load %s", BAN_FILE);
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("%s is empty", BAN_FILE);
        return;
    }

    for (node = node->children;node;node = node->next) {
        if (xmlMatches(node->name, "banned"))
            load_banned_entry(node);
    }
    
    xmlFreeDoc(doc);
}

int
isbanned(char *hostname, char *blocking_hostname)
{
    std::list<ban_entry>::iterator node = ban_list.begin();
	int i = BAN_NOT;

	if (!hostname || !*hostname)
		return BAN_NOT;

    hostname = tmp_tolower(hostname);

    for (;node != ban_list.end();++node) {
        if (!strncmp(hostname, node->_site, strlen(node->_site))) {
            i = MAX(i, node->_type);
            strcpy(blocking_hostname, node->_site);
        }
    }

	return i;
}

void
write_ban_list(void)
{
	FILE *fl;

	if (!(fl = fopen(BAN_FILE, "w"))) {
		perror("write_ban_list");
		return;
	}
    fprintf(fl, "<banlist>\n");
    std::list<ban_entry>::iterator node = ban_list.begin();
    for (;node != ban_list.end();++node) {
        fprintf(fl, "  <banned>\n");
        fprintf(fl, "    <site>%s</site>\n", node->_site);
        fprintf(fl, "    <name>%s</name>\n", node->_name);
        fprintf(fl, "    <reason>%s</reason>\n", node->_reason);
        fprintf(fl, "    <date>%ld</date>\n", (long)node->_date);
        fprintf(fl, "    <type>%s</type>\n", ban_types[node->_type]);
        fprintf(fl, "  </banned>\n");
    }
    fprintf(fl, "</banlist>\n");
	fclose(fl);
	return;
}


void
perform_ban(int flag, const char *site, const char *name, const char *reason)
{
    ban_list.push_back(ban_entry(site, flag, time(0), name, reason));
	write_ban_list();
}

void
show_bans(Creature *ch)
{
    if (ban_list.empty()) {
        send_to_char(ch, "No sites are banned.\r\n");
        return;
    }

    acc_string_clear();

    acc_strcat("* Site             Date            Banned by   Reason\r\n",
               "- ---------------  --------------  ----------  -------------------------------\r\n", NULL);

    std::list<ban_entry>::iterator node = ban_list.begin();
    for (;node != ban_list.end();++node) {
        char timestr[30];

        if (node->_date) {
            strftime(timestr, 29, "%a %m/%d/%Y", localtime(&node->_date));
        } else
            strcpy(timestr, "Unknown");
        acc_sprintf("%c %-15s  %-14s  %-10s  %-30s\r\n",
				toupper(ban_types[node->_type][0]), node->_site,
				timestr, node->_name, node->_reason);
    }
    page_string(ch->desc, acc_get_string());
}

ACMD(do_ban)
{
	int type = BAN_NEW;
	char *site, *flag;

	*buf = '\0';

	if (!*argument || GET_LEVEL(ch) < LVL_CAN_BAN) {
        show_bans(ch);
        return;
    }

	flag = tmp_getword(&argument);
	site = tmp_getword(&argument);
	if (!*site || !*flag) {
		send_to_char(ch, "Usage: ban {all | select | new} site_name\r\n");
		return;
	}
	if (!(!strcasecmp(flag, "select") || !strcasecmp(flag, "all")
			|| !strcasecmp(flag, "new"))) {
		send_to_char(ch, "Flag must be ALL, SELECT, or NEW.\r\n");
		return;
	}

    std::list<ban_entry>::iterator node = ban_list.begin();
    for (;node != ban_list.end();++node) {
		if (!strcasecmp(node->_site, site)) {
			send_to_char(ch, 
				"That site has already been banned -- unban it to change the ban type.\r\n");
			return;
		}
	}

	if (isbanned(site, buf)) {
		send_to_char(ch, "That site is already blocked as '%s'.\r\n", buf);
		return;
	}

	for (int i = BAN_NEW; i <= BAN_ALL; i++)
		if (!strcasecmp(flag, ban_types[i]))
			type = i;

    perform_ban(type, site, GET_NAME(ch), argument);

	mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
		"%s has banned %s for %s players.", GET_NAME(ch), site,
		ban_types[type]);
	send_to_char(ch, "Site banned.\r\n");
}


ACMD(do_unban)
{
	char site[MAX_INPUT_LENGTH];

	one_argument(argument, site);
	if (!*site) {
		send_to_char(ch, "A site to unban might help.\r\n");
		return;
	}
    std::list<ban_entry>::iterator node = ban_list.begin();
    for (;node != ban_list.end();++node)
		if (!strcasecmp(node->_site, site))
            break;

	if (node == ban_list.end()) {
		send_to_char(ch, "That site is not currently banned.\r\n");
		return;
	}
	send_to_char(ch, "Site unbanned.\r\n");
	mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
		"%s removed the %s-player ban on %s.",
		GET_NAME(ch), ban_types[node->_type], node->_site);

    ban_list.erase(node);

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
