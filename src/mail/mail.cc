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
#include <fstream.h>

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

void show_mail_stats(char_data *ch) {
    send_to_char("This has been removed.\r\n",ch);
    return;
}
// returns 0 for no mail. 1 for mail.
int
has_mail ( long id ) {
    char fname[256];
    fstream mail_file;
    if (!get_name_by_id(id))
        return 0;
    get_filename( get_name_by_id(id), fname, PLAYER_MAIL_FILE);
    mail_file.open(fname, ios::in | ios::nocreate);

    if (!mail_file.is_open())
        return 0;
    return 1;
}

int 
mail_box_status( long id ) {
	// 0 is normal
    // 1 is frozen
    // 2 is buried
    // 3 is deleted
    // 4 is failure

	struct char_data *victim = NULL;
	struct char_file_u tmp_store;
	int flag = 0;

	if (load_char(get_name_by_id(id), &tmp_store) < 0) 
	    return 4; // Failed to load char.

	CREATE(victim, struct char_data, 1);
	clear_char(victim);

	store_to_char(&tmp_store, victim);
	if(PLR_FLAGGED(victim,PLR_FROZEN))
        flag = 1;
    if(PLR2_FLAGGED(victim,PLR2_BURIED))
		flag = 2;
    if(PLR_FLAGGED(victim,PLR_DELETED))
        flag = 3;

	// Free the victim
	free_char(victim);
	return flag;
}

// Like it says, store the mail.  
// Returns 0 if mail not stored.
int
store_mail( long to_id, long from_id, char *txt , char *cc_list, time_t *cur_time = NULL) {
    fstream mail_file;
    mail_data *letter;
    char fname[256];
    // NO zero length mail!
	if (!txt || !strlen(txt)) {
        sprintf(buf,"Why would you send a blank message?\r\n");
        send_to_char(buf, get_char_in_world_by_idnum(from_id));
        return 0;
	}
    if(cc_list) {
        strcpy(buf,cc_list);
		strcat(buf,"\r\n");
        strcat(buf,txt);
        txt = buf;
    }
    letter = new mail_data;
    letter->to = to_id;
    letter->from = from_id;
    letter->time = time(cur_time);
    letter->msg_size = strlen(txt);
    letter->spare = 0;

    char *to_name = get_name_by_id( to_id );

    if ( to_name == 0 ) {
        sprintf( buf, "Toss_Mail Error, recipient idnum %ld invalid.", to_id );
        slog( buf );
        delete letter;
        return 0;
    }
    
    get_filename( to_name, fname, PLAYER_MAIL_FILE);
    mail_file.open(fname,ios::out | ios::app | ios::ate );
    if(!mail_file.is_open()) {
        sprintf(buf,"Error, mailfile (%s) not opened.",fname);
        slog(buf);
        send_to_char(buf, get_char_in_world_by_idnum(from_id));
        delete letter;
        return 0;
    }

    mail_file.seekp(0,ios::end);
    mail_file.write((char *)letter,sizeof(mail_data));
    mail_file.write(txt, letter->msg_size + 1);
    mail_file.close();
    delete letter;
    return 1;
}


// Pull the mail out of the players mail file if he has one.
// Create the "letters" from the file, and plant them on him without
//     telling him.  We'll let the spec say what it wants.
// Returns the number of mails recieved.
int 
recieve_mail(char_data *ch) {
    obj_data *obj = NULL;
    obj_data *list = NULL;
    int num_letters = 0;
    fstream mail_file;
    char fname[256];
    char *text,*time_str;
    mail_data *letter = NULL;

    get_filename(GET_NAME(ch), fname, PLAYER_MAIL_FILE);

    mail_file.open(fname, ios::in | ios::nocreate);

    if (!mail_file.is_open()) {
        return 0;
    }

    // Seek to the beginning and setup for reading.
    mail_file.seekp(0,ios::beg);
    letter = new mail_data;
    while(!mail_file.eof()) {
        mail_file.read(letter, sizeof(mail_data));
        text = NULL;
        if(letter->msg_size && !mail_file.eof() && (obj = read_object(MAIL_OBJ_VNUM))) { 
            num_letters++;
            text = new char[letter->msg_size + 1];
            mail_file.read(text, letter->msg_size + 1);
            // Actually build the mail object and give it to the player.    
            time_str = asctime(localtime(&letter->time));
            *(time_str + strlen(time_str) - 1) = '\0';

            sprintf(buf, " * * * *  Tempus Mail System  * * * *\r\n"
                "Date: %s\r\n"
                "  To: %s\r\n"
                "From: %s\r\n", time_str, GET_NAME(ch),
                get_name_by_id(letter->from));

            obj->action_description = new char[strlen(text) + strlen(buf) + 1];
            strcpy(obj->action_description,buf);
            strcat(obj->action_description,text);
            obj->plrtext_len = strlen(obj->action_description) + 1;

            if(list) {
                obj->next_content = list;
                list = obj;
            } else {
                list = obj;
            }
            delete text;
        } else {
            break;
		}
    }
    mail_file.close();
    remove(fname);
    delete letter;
    while (list) {
        obj = list;
        list = obj->next_content;
        obj->next_content = NULL;
        obj_to_char(obj,ch);
    }
    return num_letters;
}


/*****************************************************************
** Below is the spec_proc for a postmaster using the above       **
** routines.  Written by Jeremy Elson (jelson@server.cs.jhu.edu) **
** Fixed by John Rothe (forget@tempusmud.com) and changes owned  **
** John Watson.                                                  **
*****************************************************************/

SPECIAL(postmaster)
{
    if (!ch->desc || IS_NPC(ch))
    return 0;            /* so mobs don't get caught here */

    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
    return 0;

    if (CMD_IS("mail")) {
    postmaster_send_mail(ch, ( struct char_data *) me, cmd, argument);
    return 1;
    } else if (CMD_IS("check")) {
    postmaster_check_mail(ch, ( struct char_data *) me, cmd, argument);
    return 1;
    } else if (CMD_IS("receive")) {
    postmaster_receive_mail(ch, ( struct char_data *) me, cmd, argument);
    return 1;
    } else
    return 0;
}


void 
postmaster_send_mail(struct char_data * ch, struct char_data *mailman,
             int cmd, char *arg)
{
    long recipient;
    char buf[MAX_STRING_LENGTH];
    struct mail_recipient_data *n_mail_to;
    int total_cost = 0;
    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    int status = 0;
  
    if (GET_LEVEL(ch) < MIN_MAIL_LEVEL) {
		sprintf(buf2, "Sorry, you have to be level %d to send mail!", 
			MIN_MAIL_LEVEL);
		perform_tell(mailman, ch, buf2);
		return;
    }
    arg = one_argument(arg, buf);

    if (!*buf) {            /* you'll get no argument from me! */
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
            if(member->idnum != GET_IDNUM(ch)) {
                total_cost += STAMP_PRICE;
                CREATE(n_mail_to, struct mail_recipient_data, 1);
                n_mail_to->next = ch->desc->mail_to;
                n_mail_to->recpt_idnum = member->idnum;
                ch->desc->mail_to = n_mail_to;
            }
		}
    } else {
    
		while (*buf) {
			if ((recipient = get_id_by_name(buf)) < 0) {
				sprintf(buf2, "No one by the name '%s' is registered here!", buf);
				perform_tell(mailman, ch, buf2);
			} else if ((status = mail_box_status(recipient)) > 0) {
                // 0 is normal
                // 1 is frozen
                // 2 is buried
                // 3 is deleted
                switch (status) {
                    case 1:
                        sprintf(buf2, "%s's mailbox is frozen shut!",buf);
                        break;
                    case 2:
                        sprintf(buf2, "%s is buried! Go put it on thier grave!",buf);
                        break;
                    case 3:
                        sprintf(buf2, "No one by the name '%s' is registered here!", buf);
                        break;
                    default:
                        sprintf(buf2, "I don't have an address for %s. Try back later!",buf);
                }
                perform_tell(mailman, ch, buf2);
			} else {
				if (recipient == 1) // fireball
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
				sprintf(buf2, "The postage will cost you %d coins.", total_cost);
				perform_tell(mailman, ch, buf2);
				strcpy(buf2, "...which I see you can't afford.");
				perform_tell(mailman, ch, buf2);
				while ((n_mail_to = ch->desc->mail_to)) {
					ch->desc->mail_to = n_mail_to->next;
					free (n_mail_to);
				}
				return;
			}
			GET_GOLD(ch) -= total_cost;
		} else {	// credits
			if (GET_CASH(ch) < total_cost) {
				sprintf(buf2, "The postage will cost you %d credits.", total_cost);
				perform_tell(mailman, ch, buf2);
				strcpy(buf2, "...which I see you can't afford.");
				perform_tell(mailman, ch, buf2);
				while ((n_mail_to = ch->desc->mail_to)) {
					ch->desc->mail_to = n_mail_to->next;
					free (n_mail_to);
				}
				return;
			}
			GET_CASH(ch) -= total_cost;
		}
    }

    act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf2, "I'll take %d coins for the postage.", total_cost);
    perform_tell(mailman, ch, buf2);
    send_to_char(" Write your message.  Terminate with a @ on a new line.\r\n"
         " Enter a * on a new line to enter TED\r\n", ch);
    send_to_char(" [+--------+---------+---------+--------"
         "-+---------+---------+---------+------+]\r\n", ch);
    
    SET_BIT(PLR_FLAGS(ch), PLR_MAILING | PLR_WRITING);

#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    ch->desc->str = (char **) malloc(sizeof(char *));
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    *(ch->desc->str) = NULL;
    ch->desc->max_str = MAX_MAIL_SIZE;
}

void 
postmaster_check_mail(struct char_data * ch, struct char_data *mailman,
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
postmaster_receive_mail(struct char_data * ch, struct char_data *mailman,
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
    
    if(num_mails) {
        sprintf(buf2,"$n gives you %d piece%s of mail.",num_mails, (num_mails > 1 ? "s" : "" ) );
        act(buf2, FALSE, mailman, 0, ch, TO_VICT);
        sprintf(buf2,"$N gives $n %d piece%s of mail.",num_mails, (num_mails > 1 ? "s" : "" ) );
        act(buf2, FALSE, ch, 0, mailman, TO_ROOM);
    } else {
        strcpy(buf2, "Sorry, you don't have any mail waiting.");
        perform_tell(mailman, ch, buf2);
    }
    save_char(ch, NULL);
}
