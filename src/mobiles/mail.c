/* ************************************************************************
*   File: mail.c                                        Part of CircleMUD *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/******* MUD MAIL SYSTEM MAIN FILE ***************************************
   Written by Jeremy Elson (jelson@cs.jhu.edu)
   Rewritten by John Rothe (forget@tempusmud.com)
*************************************************************************/
//
// File: mail.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "clan.h"
#include "players.h"
#include "tmpstr.h"
#include "accstr.h"
#include "spells.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "actions.h"
#include "prog.h"
#include "mail.h"
#include "editor.h"
#include "strutil.h"

// From cityguard.cc
void call_for_help(struct creature *ch, struct creature *attacker);

GList *load_mail(char *path);

bool
has_mail(long id)
{
    if (!player_idnum_exists(id)) {
        return 0;
    }
    return access(get_mail_file_path(id), F_OK) == 0;
}

bool
can_receive_mail(long id)
{
    struct stat stat_buf;

    // Return false if player doesn't exist
    if (!player_idnum_exists(id)) {
        return false;
    }

    // Return true if file not found, false on any other error
    if (stat(get_mail_file_path(id), &stat_buf) < 0) {
        return errno == ENOENT;
    }

    // Purge mail if file size has gotten too large
    if (stat_buf.st_size > MAX_MAILFILE_SIZE) {
        purge_mail(id);
    }

    return true;
}

int
mail_box_status(long id)
{
    // 0 is normal
    // 1 is frozen
    // 2 is buried
    // 3 is deleted
    // 4 is failure

    struct creature *victim;
    int flag = 0;

    victim = load_player_from_xml(id);
    if (!victim) {
        return 4;
    }

    if (PLR_FLAGGED(victim, PLR_FROZEN)) {
        flag = 1;
    }
    if (PLR2_FLAGGED(victim, PLR2_BURIED)) {
        flag = 2;
    }

    free_creature(victim);

    return flag;
}

// Returns true if mail was successfully stored
static bool
store_mail(const char *from_name, long to_id, const char *txt, GList *cc_list,
           struct obj_data *obj_list, char **error)
{
    char *mail_file_path;
    FILE *ofile;
    char *time_str, *obj_string = NULL;
    struct obj_data *obj, *temp_o;
    time_t now = time(NULL);
    GList *mailBag = NULL;

    // NO zero length mail!
    // This should never happen.
    if (!txt || strlen(txt) == 0) {
        *error = "Why would you send a blank message?";
        return false;
    }
    // Recipient is frozen, buried, or deleted
    if (!can_receive_mail(to_id)) {
        *error = tmp_sprintf("%s doesn't seem to be able to receive mail.",
                             player_name_by_idnum(to_id));
        return false;
    }

    if (!player_idnum_exists(to_id)) {
        *error = "Sorry, you hit a bug in the mailing system.";
        errlog("Toss_Mail Error, recipient idnum %ld invalid.", to_id);
        return false;
    }

    mail_file_path = get_mail_file_path(to_id);

    // If we already have a mail file then we have to read in the mail and write it all back out.
    if (access(mail_file_path, F_OK) == 0) {
        mailBag = load_mail(mail_file_path);
    }

    obj = read_object(MAIL_OBJ_VNUM);
    time_str = asctime(localtime(&now));
    time_str[strlen(time_str) - 1] = '\0';

    acc_string_clear();
    acc_sprintf(" * * * *  Tempus Mail System  * * * *\r\n"
                "Date: %s\r\n  To: %s\r\nFrom: %s",
                time_str, player_name_by_idnum(to_id), from_name);

    if (cc_list != NULL) {
        GList *si;

        for (si = cc_list; si; si = si->next) {
            acc_strcat((si == cc_list) ? "\r\n  CC: " : ", ",
                       ((GString *)si->data)->str, NULL);
        }
    }
    if (obj_list) {
        acc_strcat("\r\nPackages attached to this mail:\r\n", NULL);
        temp_o = obj_list;
        while (temp_o) {
            obj_string = tmp_sprintf("  %s\r\n", temp_o->name);
            acc_strcat(obj_string, NULL);
            temp_o = temp_o->next_content;
        }
    }
    acc_strcat("\r\n\r\n", txt, NULL);

    obj->action_desc = strdup(acc_get_string());

    obj->plrtext_len = strlen(obj->action_desc) + 1;
    mailBag = g_list_append(mailBag, obj);

    if ((ofile = fopen(mail_file_path, "w")) == NULL) {
        errlog("Unable to open xml mail file '%s': %s",
               mail_file_path, strerror(errno));
        return false;
    } else {
        GList *oi;

        fprintf(ofile, "<objects>");
        for (oi = mailBag; oi; oi = oi->next) {
            obj = (struct obj_data *)oi->data;
            save_object_to_xml(obj, ofile);
            extract_obj(obj);
        }
        if (obj_list) {
            temp_o = obj_list;
            while (temp_o) {
                save_object_to_xml(temp_o, ofile);
                extract_obj(temp_o);
                temp_o = temp_o->next_content;
            }
        }
        fprintf(ofile, "</objects>");
        fclose(ofile);
    }

    return true;
}

bool
send_mail(struct creature *from,
          GList *recipients,
          const char *txt,
          struct obj_data *obj_list,
          char **error)
{
    GList *cc_list = NULL;
    bool all_sent = true;

    for (GList *mail_rcpt = recipients; mail_rcpt; mail_rcpt = mail_rcpt->next) {
        const char *name = player_name_by_idnum(GPOINTER_TO_INT(mail_rcpt->data));
        cc_list = g_list_prepend(cc_list, g_string_new(name));
    }

    for (GList *it = recipients; it; it = it->next) {
        long to_id = GPOINTER_TO_INT(it->data);
        bool sent_this = store_mail(GET_NAME(from), to_id, txt, cc_list, obj_list, error);
        if (sent_this) {
            for (struct descriptor_data *r_d = descriptor_list; r_d; r_d = r_d->next) {
                if (IS_PLAYING(r_d) && r_d->creature &&
                    (r_d->creature != from) &&
                    (GET_IDNUM(r_d->creature) == to_id) &&
                    (!PLR_FLAGGED(r_d->creature, PLR_WRITING))) {
                    send_to_char(r_d->creature,
                                 "A strange voice in your head says, "
                                 "'You have new mail.'\r\n");
                }
            }
        } else {
            all_sent = false;
        }
    }
    g_list_foreach(cc_list, (GFunc)g_string_free, NULL);
    g_list_free(cc_list);

    return all_sent;
}

bool
purge_mail(long idnum)
{
    return unlink(get_mail_file_path(idnum)) == 0;
}

// Pull the mail out of the players mail file if he has one.
// Create the "letters" from the file, and plant them on him without
// telling him.  We'll let the spec say what it wants.
// Returns the number of mails received.
GList *
receive_mail(struct creature *ch, int *num_letters)
{
    int counter = 0;
    char *path = get_mail_file_path(GET_IDNUM(ch));
    struct obj_data *container = NULL;
    GList *olist = NULL;


    GList *mailBag = load_mail(path);

    if (g_list_length(mailBag) > MAIL_BAG_THRESH) {
        container = read_object(MAIL_BAG_OBJ_VNUM);
    }

    *num_letters = 0;
    for (GList *oi = mailBag; oi; oi = oi->next) {
        struct obj_data *obj = oi->data;
        counter++;

        if (GET_OBJ_VNUM(obj) == MAIL_OBJ_VNUM) {
            (*num_letters)++;
        } else {
            olist = g_list_append(olist, obj);
        }

        if ((counter > MAIL_BAG_OBJ_CONTAINS) && container) {
            obj_to_char(obj, ch);
            obj = read_object(MAIL_BAG_OBJ_VNUM);
            counter = 0;
        }
        if (container) {
            obj_to_obj(obj, container);
        } else {
            obj_to_char(obj, ch);
        }
    }

    if (container) {
        obj_to_char(container, ch);
    }

    unlink(path);

    return olist;
}

GList *
load_mail(char *path)
{
    int axs = access(path, W_OK | R_OK);
    GList *mailBag = NULL;

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml mail file '%s': %s",
                   path, strerror(errno));
            return mailBag;
        } else {
            return mailBag;     // normal no eq file
        }
    }
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return mailBag;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return mailBag;
    }

    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "object")) {
            struct obj_data *obj =
                load_object_from_xml(NULL, NULL, NULL, node);
            if (obj) {
                mailBag = g_list_append(mailBag, obj);
            }
        }
    }

    xmlFreeDoc(doc);

    return mailBag;
}

/*****************************************************************
** Below is the spec_proc for a postmaster using the above       **
** routines.  Written by Jeremy Elson (jelson@server.cs.jhu.edu) **
** Fixed by John Rothe (forget@tempusmud.com) and changes owned  **
** John Watson.                                                  **
*****************************************************************/

SPECIAL(postmaster)
{
    struct creature *self = (struct creature *)me;

    if (spec_mode == SPECIAL_TICK) {
        if (is_fighting(self) && !number(0, 4)) {
            call_for_help(self, random_opponent(self));
            return 1;
        }
        return 0;
    }

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    if (!ch || !ch->desc || IS_NPC(ch)) {
        return 0;               /* so mobs don't get caught here */

    }
    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive"))) {
        return 0;
    }

    if (CMD_IS("mail")) {
        postmaster_send_mail(ch, self, argument);
        return 1;
    } else if (CMD_IS("check")) {
        postmaster_check_mail(ch, self);
        return 1;
    } else if (CMD_IS("receive")) {
        postmaster_receive_mail(ch, self);
        return 1;
    } else {
        return 0;
    }
}

void
postmaster_send_mail(struct creature *ch, struct creature *mailman, char *args)
{
    long recipient;
    GList *mail_list = NULL;
    int total_cost = 0;
    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    int status = 0;

    if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
        perform_tell(mailman, ch,
                     tmp_sprintf("Sorry, you have to be level %d to send mail!",
                                 MIN_MAIL_LEVEL));
        return;
    }

    char *arg = tmp_getword(&args);
    if (!*arg) {                /* you'll get no argument from me! */
        perform_tell(mailman, ch, "You need to specify an addressee!");
        return;
    }

    if (!strcasecmp(arg, "clan")) {
        if (!(clan = real_clan(GET_CLAN(ch)))) {
            perform_tell(mailman, ch, "You are not a member of any clan!");
            return;
        }
        for (member = clan->member_list; member; member = member->next) {
            if (!member->no_mail) {
                total_cost += STAMP_PRICE;
                mail_list = g_list_append(mail_list, GINT_TO_POINTER(member->idnum));
            }
        }
    } else {
        GHashTable *unique_names = g_hash_table_new(g_str_hash, g_str_equal);
        for (; *arg; arg = tmp_getword(&args)) {
            if (g_hash_table_contains(unique_names, arg)) {
                perform_tell(mailman, ch,
                             tmp_sprintf("You seem to have listed %s more than once...", arg));
                continue;
            }
            g_hash_table_add(unique_names, arg);
            recipient = player_idnum_by_name(arg);
            if (recipient <= 0) {
                perform_tell(mailman, ch,
                             tmp_sprintf("No one by the name '%s' is registered here!", arg));
                continue;
            }
            status = mail_box_status(recipient);
            if (status > 0) {
                // 0 is normal
                // 1 is frozen
                // 2 is buried
                // 3 is deleted
                switch (status) {
                case 1:
                    perform_tell(mailman, ch, tmp_sprintf("%s's mailbox is frozen shut!", arg));
                    break;
                case 2:
                    perform_tell(mailman, ch, tmp_sprintf("%s is buried! Go put it on their grave!", arg));
                    break;
                case 3:
                    perform_tell(mailman, ch, tmp_sprintf("No one by the name '%s' is registered here!", arg));
                    break;
                default:
                    perform_tell(mailman, ch, tmp_sprintf("I don't have an address for %s. Try back later!", arg));
                }
                continue;
            }
            if (recipient == 1) { // fireball
                total_cost += 1000000;
            } else {
                total_cost += STAMP_PRICE;
            }

            mail_list = g_list_append(mail_list, GINT_TO_POINTER(recipient));
        }
        g_hash_table_destroy(unique_names);
    }

    if (!mail_list) {
        perform_tell(mailman, ch,
                     "Sorry, you're going to have to specify some valid recipients!");
        return;
    }

    // deduct cost of mailing
    if (total_cost > 0) {
        if (!adjust_creature_money(ch, -total_cost)) {
            perform_tell(mailman, ch,
                         tmp_sprintf("The postage will cost you %'d %ss.",
                                     total_cost, CURRENCY(ch)));
            perform_tell(mailman, ch, "...which I see you can't afford.");
            g_list_free(mail_list);
            return;
        }

        perform_tell(mailman, ch,
                     tmp_sprintf("I'll take %'d coins for the postage.",
                                 total_cost));
    }

    act("$n starts to write some mail.", true, ch, NULL, NULL, TO_ROOM);

    start_editing_mail(ch->desc, mail_list);
}

void
postmaster_check_mail(struct creature *ch, struct creature *mailman)
{
    char buf2[256];

    if (has_mail(GET_IDNUM(ch))) {
        strcpy_s(buf2, sizeof(buf2), "You have mail waiting.");
    } else {
        strcpy_s(buf2, sizeof(buf2), "Sorry, you don't have any mail waiting.");
    }
    perform_tell(mailman, ch, buf2);
}

void
postmaster_receive_mail(struct creature *ch, struct creature *mailman)
{
    char *to_char = NULL, *to_room = NULL;
    int num_mails = 0;

    if (!has_mail(GET_IDNUM(ch))) {
        to_char = tmp_sprintf("Sorry, you don't have any mail waiting.");
        perform_tell(mailman, ch, to_char);
        return;
    }

    GList *olist = receive_mail(ch, &num_mails);

    if (num_mails == 0) {
        to_char = tmp_sprintf("Sorry, you don't have any mail waiting.");
        perform_tell(mailman, ch, to_char);
        return;
    } else if (num_mails > MAIL_BAG_THRESH) {
        num_mails = num_mails / MAIL_BAG_OBJ_CONTAINS + 1;
        to_char = tmp_sprintf("$n gives you %d bag%s of mail", num_mails,
                              (num_mails > 1 ? "s" : ""));
        to_room = tmp_sprintf("$n gives $N %d bag%s of mail", num_mails,
                              (num_mails > 1 ? "s" : ""));
    } else {
        to_char = tmp_sprintf("$n gives you %d piece%s of mail", num_mails,
                              (num_mails > 1 ? "s" : ""));
        to_room = tmp_sprintf("$n gives $N %d piece%s of mail", num_mails,
                              (num_mails > 1 ? "s" : ""));
    }

    if (olist) {
        if (olist->next) {
            to_room = tmp_strcat(to_room, " and some packages.", NULL);
        } else {
            to_room = tmp_strcat(to_room, " and a package.", NULL);
        }

        for (GList *li = olist; li; li = li->next) {
            struct obj_data *obj = li->data;
            if (!li->next) {
                to_char = tmp_strcat(to_char, " and ", obj->name, ".", NULL);
            } else {
                to_char = tmp_strcat(to_char, ", ", obj->name, NULL);
            }
        }
    } else {
        to_char = tmp_strcat(to_char, ".", NULL);
        to_room = tmp_strcat(to_room, ".", NULL);
    }

    act(to_char, false, mailman, NULL, ch, TO_VICT);
    act(to_room, false, mailman, NULL, ch, TO_NOTVICT);

    crashsave(ch);
}
