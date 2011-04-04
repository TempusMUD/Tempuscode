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
#include <regex.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "ban.h"
#include "accstr.h"
#include "tmpstr.h"
#include "security.h"

GList *ban_list = NULL;

const char *ban_types[] = {
    "no",
    "new",
    "select",
    "all",
    "ERROR"
};

void
load_banned_entry(xmlNodePtr node)
{
    struct ban_entry *ban;
    xmlNodePtr child;
    char *text;

    CREATE(ban, struct ban_entry, 1);
    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "site")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban->site, text);
            free(text);
        } else if (xmlMatches(child->name, "name")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban->name, text);
            free(text);
        } else if (xmlMatches(child->name, "reason")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban->reason, text);
            free(text);
        } else if (xmlMatches(child->name, "message")) {
            text = (char *)xmlNodeGetContent(child);
            strcpy(ban->message, text);
            free(text);
        } else if (xmlMatches(child->name, "type")) {
            text = (char *)xmlNodeGetContent(child);
            if (!strcmp(text, "all"))
                ban->type = BAN_ALL;
            else if (!strcmp(text, "select"))
                ban->type = BAN_SELECT;
            else if (!strcmp(text, "new"))
                ban->type = BAN_NEW;
            free(text);
        } else if (xmlMatches(child->name, "date")) {
            text = (char *)xmlNodeGetContent(child);
            ban->date = atol(text);
            free(text);
        }
    }

    ban_list = g_list_prepend(ban_list, ban);
}

void
load_banned(void)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    g_list_foreach(ban_list, (GFunc) free, 0);
    g_list_free(ban_list);
    ban_list = NULL;

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

    for (node = node->children; node; node = node->next) {
        if (xmlMatches(node->name, "banned"))
            load_banned_entry(node);
    }

    xmlFreeDoc(doc);
}

int
isbanned(char *hostname, char *blocking_hostname)
{
    int i = BAN_NOT;

    if (!hostname || !*hostname)
        return BAN_NOT;

    hostname = tmp_tolower(hostname);

    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *node = it->data;
        if (!strncmp(hostname, node->site, strlen(node->site))) {
            i = MAX(i, node->type);
            strcpy(blocking_hostname, node->reason);
        }
    }

    return i;
}

bool
check_ban_all(int desc, char *hostname)
{
    int write_to_descriptor(int desc, const char *txt);
    struct ban_entry *node = NULL;

    if (!hostname || !*(hostname))
        return false;

    hostname = tmp_tolower(hostname);

    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *ban = it->data;
        if (ban->type == BAN_ALL
            && !strncmp(hostname, ban->site, strlen(ban->site))) {
            node = ban;
            break;
        }
    }

    if (!node)
        return false;

    write_to_descriptor(desc,
        "*******************************************************************************\r\n");
    write_to_descriptor(desc,
        "                               T E M P U S  M U D\r\n");
    write_to_descriptor(desc,
        "*******************************************************************************\r\n");
    if (node->message[0]) {
        write_to_descriptor(desc, tmp_gsub(node->message, "\n", "\r\n"));
    } else {
        write_to_descriptor(desc,
            "       We're sorry, we have been forced to ban your IP address.\r\n"
            "    If you have never played here before, or you feel we have made\r\n"
            "    a mistake, or perhaps you just got caught in the wake of\r\n"
            "    someone else's trouble making, please mail unban@tempusmud.com.\r\n"
            "    Please include your account name and your character name(s)\r\n"
            "    so we can siteok your IP.  We apologize for the inconvenience,\r\n"
            "    and we hope to see you soon!\r\n" "\r\n");
    }

    mlog(ROLE_ADMINBASIC, LVL_GOD, CMP, true,
        "Connection attempt denied from [%s]%s",
        hostname, (node->reason[0]) ? tmp_sprintf(" (%s)", node->reason) : "");

    return true;
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
    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *node = it->data;
        fprintf(fl, "  <banned>\n");
        fprintf(fl, "    <site>%s</site>\n", node->site);
        fprintf(fl, "    <name>%s</name>\n", node->name);
        if (node->reason[0])
            fprintf(fl, "    <reason>%s</reason>\n", node->reason);
        if (node->message[0])
            fprintf(fl, "    <message>%s</message>\n", node->message);
        fprintf(fl, "    <date>%ld</date>\n", (long)node->date);
        fprintf(fl, "    <type>%s</type>\n", ban_types[node->type]);
        fprintf(fl, "  </banned>\n");
    }
    fprintf(fl, "</banlist>\n");
    fclose(fl);
    return;
}

void
perform_ban(int flag, const char *site, const char *name, const char *reason)
{
    struct ban_entry *ban;
    CREATE(ban, struct ban_entry, 1);
    strcpy(ban->site, site);
    ban->type = flag;
    ban->date = time(0);
    strcpy(ban->name, name);
    strcpy(ban->reason, reason);
    ban_list = g_list_append(ban_list, ban);
    write_ban_list();
}

void
show_bans(struct creature *ch)
{
    if (ban_list) {
        send_to_char(ch, "No sites are banned.\r\n");
        return;
    }

    acc_string_clear();

    acc_strcat("* Site             Date            Banned by   Reason\r\n",
        "- ---------------  --------------  ----------  -------------------------------\r\n",
        NULL);

    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *node = it->data;
        char timestr[30];

        if (node->date) {
            strftime(timestr, 29, "%a %m/%d/%Y", localtime(&node->date));
        } else
            strcpy(timestr, "Unknown");
        acc_sprintf("%c %-15s  %-14s  %-10s  %-30s\r\n",
            toupper(ban_types[node->type][0]), node->site,
            timestr, node->name, node->reason);
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

    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *node = it->data;
        if (!strcasecmp(node->site, site)) {
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
    struct ban_entry *node = NULL;

    one_argument(argument, site);
    if (!*site) {
        send_to_char(ch, "A site to unban might help.\r\n");
        return;
    }
    for (GList * it = ban_list; it; it = it->next) {
        struct ban_entry *node = it->data;
        if (!strcasecmp(node->site, site))
            break;
    }

    if (node) {
        send_to_char(ch, "That site is not currently banned.\r\n");
        return;
    }
    send_to_char(ch, "Site unbanned.\r\n");
    mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
        "%s removed the %s-player ban on %s.",
        GET_NAME(ch), ban_types[node->type], node->site);

    ban_list = g_list_remove(ban_list, node);
    free(node);

    write_ban_list();
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)                  *
 *  Written by Sharon P. Goza                                                  *
 **************************************************************************/

regex_t *invalid_list = NULL;
namestring *nasty_list = NULL;
int num_invalid = 0;
int num_nasty = 0;

bool
is_valid_name(char *newname)
{
    int alpha_hist[256];
    int i, len;
    char tempname[MAX_NAME_LENGTH];

    len = strlen(newname);
    if (len > (int)MAX_NAME_LENGTH)
        return 0;

    if (len < 3)
        return 0;

    /* return valid if list doesn't exist */
    if (!invalid_list || num_invalid < 1)
        return 1;

    /* change to lowercase */
    strncpy(tempname, newname, MAX_NAME_LENGTH);
    for (i = 0; i < len; i++) {
        if (!isalpha(tempname[i]) && tempname[i] != '\'')
            return 0;
        tempname[i] = tolower(tempname[i]);
    }

    /* Does the desired name contain a string in the invalid list? */
    for (i = 0; i < num_invalid; i++)
        if (regexec(&invalid_list[i], tempname, 0, 0, 0) == 0)
            return false;

    // Build histogram of characters used
    for (i = 0; i < 256; i++)
        alpha_hist[i] = 0;
    for (i = 0; i < len; i++)
        alpha_hist[(int)tempname[i]] += 1;

    // All names must have at least one vowel
    if (!(alpha_hist[(int)'a'] || alpha_hist[(int)'e'] || alpha_hist[(int)'i']
            || alpha_hist[(int)'o'] || alpha_hist[(int)'u']
            || alpha_hist[(int)'y']))
        return 0;

    // Check that no character is used more than half the length of the string,
    // rounded up, excluding the first character of the name
    // Oog is ok, Ooog is ok, Oooog is not
    for (i = 0; i < 256; i++)
        if (alpha_hist[i] - ((i == tempname[0]) ? 1 : 0) > len / 2)
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
    namestring buf;

    if (!(fp = fopen(XNAME_FILE, "r"))) {
        perror("Unable to open invalid name file");
        return;
    }
    /* count how many records */
    while (fgets(buf, sizeof(buf), fp) != NULL && strlen(buf) > 1)
        num_invalid++;

    rewind(fp);

    CREATE(invalid_list, regex_t, num_invalid);

    for (i = 0; i < num_invalid; i++) {
        int err;

        if (!fgets(buf, MAX_NAME_LENGTH, fp)) {
            perror(tmp_sprintf("Couldn't read %s", XNAME_FILE));
            safe_exit(1);
        }
        buf[strlen(buf) - 1] = '\0';
        err = regcomp(&invalid_list[i], buf, REG_EXTENDED | REG_NOSUB);
        if (err) {
            char errbuf[1024];
            regerror(err, &invalid_list[i], errbuf, sizeof(errbuf));
            slog("Invalid bad name %s: %s", buf, errbuf);
            safe_exit(1);
        }
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
        if (!fgets(nasty_list[i], MAX_NAME_LENGTH, fp)) {   /* read word */
            perror(tmp_sprintf("Couldn't read %s", NASTY_FILE));
            safe_exit(1);
        }
        nasty_list[i][strlen(nasty_list[i]) - 1] = '\0';    /* cleave off \n */
    }
    fclose(fp);
}
