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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fstream>
#include <stack>

#include "actions.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "clan.h"
#include "materials.h"
#include "player_table.h"

using namespace std;

list<obj_data *> load_mail(char *path);

void
show_mail_stats(Creature * ch)
{
    send_to_char(ch, "This has been removed.\r\n");
    return;
}

// returns 0 for no mail. 1 for mail.
int
has_mail(long id)
{
    fstream mail_file;

    if(! playerIndex.exists(id) )
        return 0;
    mail_file.open(get_mail_file_path(id), ios::in);

    if (!mail_file.is_open())
        return 0;
    return 1;
}

int
can_recieve_mail(long id)
{
    long length = 0;
    fstream mail_file;
    if(! playerIndex.exists(id) )
        return 0;
    mail_file.open(get_mail_file_path(id), ios::in);

    if (!mail_file.is_open())
        return 1;
    mail_file.seekg(0, ios::end);
    length = mail_file.tellg();
    mail_file.close();
    if (length >= MAX_MAILFILE_SIZE)
        return 0;
    return 1;
}

int
mail_box_status(long id)
{
    // 0 is normal
    // 1 is frozen
    // 2 is buried
    // 3 is deleted
    // 4 is failure

    struct Creature *victim;
    int flag = 0;

    victim = new Creature(true);
    if (!victim->loadFromXML(id)) {
        delete victim;
        return 4;
    }

    if (PLR_FLAGGED(victim, PLR_FROZEN))
        flag = 1;
    if (PLR2_FLAGGED(victim, PLR2_BURIED))
        flag = 2;
    if (PLR_FLAGGED(victim, PLR_DELETED))
        flag = 3;

    delete victim;
    return flag;
}

// Like it says, store the mail.  
// Returns 0 if mail not stored.
int
store_mail(long to_id, long from_id, char *txt, list<string> cc_list,
    time_t *cur_time)
{
    int buf_size = 0;
    char *mail_file_path;
    FILE *ofile;
    char *time_str;
    char buf[MAX_MAIL_SIZE];
    struct obj_data *obj;
    struct stat stat_buf;
    time_t now = time(NULL);
    list<obj_data*> mailBag;
    
    // NO zero length mail!
    // This should never happen.
    if (!txt || !strlen(txt)) {
        send_to_char(get_char_in_world_by_idnum(from_id), 
                     "Why would you send a blank message?\r\n");
        return 0;
    }
    
    // Recipient is frozen, buried, or deleted
    if (!can_recieve_mail(to_id)) {
        send_to_char(get_char_in_world_by_idnum(from_id), 
                     "%s doesn't seem to be able to recieve mail.\r\n",
            playerIndex.getName(to_id));
        return 0;
    }

    if(! playerIndex.exists(to_id) ) {
        slog("Toss_Mail Error, recipient idnum %ld invalid.", to_id);
        return 0;
    }

    mail_file_path = get_mail_file_path(to_id);

    // If we already have a mail file then we have to read in the mail and write it all back out.
    if (stat(mail_file_path, &stat_buf) == 0)
        mailBag = load_mail(mail_file_path);


    obj = read_object(MAIL_OBJ_VNUM);
    time_str = asctime(localtime(&now));
    *(time_str + strlen(time_str) - 1) = '\0';
     
    sprintf(buf, " * * * *  Tempus Mail System  * * * *\r\n"
                 "Date: %s\r\n"
                 "  To: %s\r\n"
                 "From: %s\r\n", time_str, playerIndex.getName(to_id), 
            playerIndex.getName(from_id));

    buf_size = strlen(txt) + strlen(buf);

    list<string>::iterator si;
    for (si = cc_list.begin(); si != cc_list.end(); si++)
        buf_size += si->length() + 1; // for the commas
        
    buf_size += 6; // for "  CC: "
    
    obj->action_description = (char*)malloc(sizeof(char) * buf_size + 2); // for the extra /n
    
    strcpy(obj->action_description, buf);

    if (!cc_list.empty())
        strcat(obj->action_description, "  CC: ");

    unsigned count = 1;
    for (si = cc_list.begin(); si != cc_list.end(); si++) {
        count++;
        strcat(obj->action_description, si->c_str());
        if (count <= cc_list.size())
            strcat(obj->action_description, ", ");
        else
            strcat(obj->action_description, "\n");
    }

    strcat(obj->action_description, "\n");
    strcat(obj->action_description, txt);

    obj->plrtext_len = strlen(obj->action_description) + 1;
    mailBag.push_back(obj);
    
    if ((ofile = fopen(mail_file_path, "w")) == NULL) {
        slog("SYSERR: Unable to open xml mail file '%s': %s", 
             mail_file_path, strerror(errno) );
        return 0;
    }
    else {
        list<obj_data*>::iterator oi;

        fprintf(ofile, "<objects>");
        for (oi = mailBag.begin(); oi != mailBag.end(); oi++) {
            (*oi)->saveToXML(ofile);
            extract_obj(*oi);
        }
        fprintf(ofile, "</objects>");
        fclose(ofile);
    }
    
    return 1;
}

int
purge_mail(long idnum)
{
    fstream mail_file;
    mail_file.open(get_mail_file_path(idnum), ios::in);
    if (!mail_file.is_open()) {
        return 0;
    }
    mail_file.close();
    remove(get_mail_file_path(idnum));
    return 1;
}

// Pull the mail out of the players mail file if he has one.
// Create the "letters" from the file, and plant them on him without
//     telling him.  We'll let the spec say what it wants.
// Returns the number of mails recieved.
int
recieve_mail(Creature * ch)
{
    int num_letters = 0;
    int counter = 0;
    char *path = get_mail_file_path( GET_IDNUM(ch) );
    bool container = false;
    list<obj_data *> mailBag;
    

    mailBag = load_mail(path);

    if (mailBag.size() > MAIL_BAG_THRESH )
        container = true;
        
    list<obj_data *>::iterator oi;
    
    obj_data *obj = NULL;
    if (container)
        obj = read_object(MAIL_BAG_OBJ_VNUM);
        
    for (oi = mailBag.begin(); oi != mailBag.end(); oi++) {
        counter++;
        num_letters++;
        if ((counter > MAIL_BAG_OBJ_CONTAINS) && container) {
            obj_to_char(obj, ch);
            obj = read_object(MAIL_BAG_OBJ_VNUM);
            counter = 0;
        }
        if (obj) {
            obj_to_obj(*oi, obj);
        }
        else {
            obj_to_char(*oi, ch);
        }
    }

    if (obj)
        obj_to_char(obj, ch);

    unlink(path);
    
    return num_letters;
}


list<obj_data *> load_mail(char *path)
{
	int axs = access(path, W_OK);
    list<obj_data *> mailBag;

	if( axs != 0 ) {
		if( errno != ENOENT ) {
			slog("SYSERR: Unable to open xml mail file '%s': %s", 
				 path, strerror(errno) );
			return mailBag;
		} else {
			return mailBag; // normal no eq file
		}
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        slog("SYSERR: XML parse error while loading %s", path);
        return mailBag;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        slog("SYSERR: XML file %s is empty", path);
        return mailBag;
    }

	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "object") ) {
			obj_data *obj;
			CREATE(obj, obj_data, 1);
			obj->clear();
			if(! obj->loadFromXML(NULL, NULL, NULL, node) ) {
				extract_obj(obj);
			}
            else {
                mailBag.push_back(obj);
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
    if( spec_mode != SPECIAL_CMD ) 
        return 0;

    if (!ch || !ch->desc || IS_NPC(ch))
        return 0;                /* so mobs don't get caught here */

    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
        return 0;

    if (CMD_IS("mail")) {
        postmaster_send_mail(ch, (struct Creature *)me, cmd, argument);
        return 1;
    } else if (CMD_IS("check")) {
        postmaster_check_mail(ch, (struct Creature *)me, cmd, argument);
        return 1;
    } else if (CMD_IS("receive")) {
        postmaster_receive_mail(ch, (struct Creature *)me, cmd, argument);
        return 1;
    } else
        return 0;
}


void
postmaster_send_mail(struct Creature *ch, struct Creature *mailman,
    int cmd, char *arg)
{
    long recipient;
    char buf[MAX_STRING_LENGTH];
    struct mail_recipient_data *n_mail_to;
    int total_cost = 0;
    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    int status = 0;
    char **tmp_char = NULL;

    if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
        sprintf(buf2, "Sorry, you have to be level %d to send mail!",
            MIN_MAIL_LEVEL);
        perform_tell(mailman, ch, buf2);
        return;
    }
    arg = one_argument(arg, buf);

    if (!*buf) {                /* you'll get no argument from me! */
        strcpy(buf2, "You need to specify an addressee!");
        perform_tell(mailman, ch, buf2);
        return;
    }

    ch->desc->mail_to = NULL;

    if (!str_cmp(buf, "clan")) {
        if (!(clan = real_clan(GET_CLAN(ch)))) {
            perform_tell(mailman, ch, "You are not a member of any clan!");
            return;
        }
        for (member = clan->member_list; member; member = member->next) {
            total_cost += STAMP_PRICE;
            CREATE(n_mail_to, struct mail_recipient_data, 1);
            n_mail_to->next = ch->desc->mail_to;
            n_mail_to->recpt_idnum = member->idnum;
            ch->desc->mail_to = n_mail_to;
        }
    } else {

        while (*buf) {
            if ((recipient = playerIndex.getID(buf)) < 0) {
                sprintf(buf2, "No one by the name '%s' is registered here!",
                    buf);
                perform_tell(mailman, ch, buf2);
            } else if ((status = mail_box_status(recipient)) > 0) {
                // 0 is normal
                // 1 is frozen
                // 2 is buried
                // 3 is deleted
                switch (status) {
                case 1:
                    sprintf(buf2, "%s's mailbox is frozen shut!", buf);
                    break;
                case 2:
                    sprintf(buf2, "%s is buried! Go put it on their grave!",
                        buf);
                    break;
                case 3:
                    sprintf(buf2,
                        "No one by the name '%s' is registered here!", buf);
                    break;
                default:
                    sprintf(buf2,
                        "I don't have an address for %s. Try back later!",
                        buf);
                }
                perform_tell(mailman, ch, buf2);
            } else {
                if (recipient == 1)    // fireball
                    total_cost += 1000000;
                else
                    total_cost += STAMP_PRICE;

                CREATE(n_mail_to, struct mail_recipient_data, 1);
                n_mail_to->next = ch->desc->mail_to;
                n_mail_to->recpt_idnum = recipient;
                ch->desc->mail_to = n_mail_to;
            }
            arg = one_argument(arg, buf);
        }
    }
    if (!total_cost || !ch->desc->mail_to) {
        perform_tell(mailman, ch,
            "Sorry, you're going to have to specify some valid recipients!");
        return;
    }
    // deduct cost of mailing
    if (GET_LEVEL(ch) < LVL_AMBASSADOR) {

        // gold
        if (ch->in_room->zone->time_frame != TIME_ELECTRO) {
            if (GET_GOLD(ch) < total_cost) {
                sprintf(buf2, "The postage will cost you %d coins.",
                    total_cost);
                perform_tell(mailman, ch, buf2);
                strcpy(buf2, "...which I see you can't afford.");
                perform_tell(mailman, ch, buf2);
                while ((n_mail_to = ch->desc->mail_to)) {
                    ch->desc->mail_to = n_mail_to->next;
                    free(n_mail_to);
                }
                return;
            }
            GET_GOLD(ch) -= total_cost;
        } else {                // credits
            if (GET_CASH(ch) < total_cost) {
                sprintf(buf2, "The postage will cost you %d credits.",
                    total_cost);
                perform_tell(mailman, ch, buf2);
                strcpy(buf2, "...which I see you can't afford.");
                perform_tell(mailman, ch, buf2);
                while ((n_mail_to = ch->desc->mail_to)) {
                    ch->desc->mail_to = n_mail_to->next;
                    free(n_mail_to);
                }
                return;
            }
            GET_CASH(ch) -= total_cost;
        }
    }

    act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf2, "I'll take %d coins for the postage.", total_cost);
    perform_tell(mailman, ch, buf2);

    tmp_char = (char **)malloc(sizeof(char *));
    *(tmp_char) = NULL;

    SET_BIT(PLR_FLAGS(ch), PLR_MAILING | PLR_WRITING);
    start_text_editor(ch->desc, tmp_char, true, MAX_MAIL_SIZE - 1);
}

void
postmaster_check_mail(struct Creature *ch, struct Creature *mailman,
    int cmd, char *arg)
{
    char buf2[256];

    if (has_mail(GET_IDNUM(ch))) {
        strcpy(buf2, "You have mail waiting.");
    } else
        strcpy(buf2, "Sorry, you don't have any mail waiting.");
    perform_tell(mailman, ch, buf2);
}

void
postmaster_receive_mail(struct Creature *ch, struct Creature *mailman,
    int cmd, char *arg)
{
    char buf2[256];
    int num_mails = 0;

    if (!has_mail(GET_IDNUM(ch))) {
        strcpy(buf2, "Sorry, you don't have any mail waiting.");
        perform_tell(mailman, ch, buf2);
        return;
    }

    num_mails = recieve_mail(ch);

    if (num_mails == 0) {
        strcpy(buf2, "Sorry, you don't have any mail waiting.");
        perform_tell(mailman, ch, buf2);
    }
    else if (num_mails > MAIL_BAG_THRESH) {
        num_mails = num_mails / MAIL_BAG_OBJ_CONTAINS + 1;
        sprintf(buf2, "$n gives you %d bag%s of mail.", num_mails,
               (num_mails > 1 ? "s" : ""));
        act(buf2, FALSE, mailman, 0, ch, TO_VICT);
        sprintf(buf2, "$N gives $n %d bag%s of mail.", num_mails,
               (num_mails > 1 ? "s" : ""));
        act(buf2, FALSE, ch, 0, mailman, TO_ROOM);
    }
    else {
        sprintf(buf2, "$n gives you %d piece%s of mail.", num_mails,
            (num_mails > 1 ? "s" : ""));
        act(buf2, FALSE, mailman, 0, ch, TO_VICT);
        sprintf(buf2, "$N gives $n %d piece%s of mail.", num_mails,
            (num_mails > 1 ? "s" : ""));
        act(buf2, FALSE, ch, 0, mailman, TO_ROOM);
    }
    ch->saveToXML();
}
