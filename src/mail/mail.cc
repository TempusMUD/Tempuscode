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
    mail_file.open(fname, ios::in || ios::ate);

    if (!mail_file || mail_file.eof())
        return 0;
    return 1;
}

// Like it says, store the mail.  
// Returns 0 if mail not stored.
int
store_mail( long to_id, long from_id, char *txt , time_t *cur_time = NULL) {
    fstream mail_file;
    mail_data *letter;
    char fname[256];
    /*
	Add in something here to avoid sending length 0 messages.
	if (!txt || !strlen(txt)) {
        sprintf(buf,"Why would you send a blank message?");
        send_to_char(buf, get_char_in_world_by_idnum(from_id));
	}
	*/
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
    if(!mail_file) {
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
    int num_letters = 0;
    fstream mail_file;
    char fname[256];
    char *text,*time_str;
    mail_data *letter = NULL;

    get_filename(GET_NAME(ch), fname, PLAYER_MAIL_FILE);

    mail_file.open(fname, ios::in || ios::ate);

    if (!mail_file || mail_file.eof()) {
        return 0;
    }
    
    // Seek to the beginning and setup for reading.
    mail_file.seekp(0,ios::beg);
    letter = new mail_data;
    while(!mail_file.eof()) {
        mail_file.read(letter, sizeof(mail_data));
        text = NULL;
        if(letter->msg_size && !mail_file.eof()) { 
            num_letters++;
            text = new char[letter->msg_size + 1];
            mail_file.read(text, letter->msg_size + 1);
        } else {
            mail_file.close();
            delete letter;
            remove(fname);
            return num_letters;
		}
        if (!(obj = read_object(MAIL_OBJ_VNUM))) {
            slog("SYSERR: Unable to load MAIL_OBJ_VNUM in postmaster_receive_mail");
            delete letter;
            return num_letters;
        }
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

        obj_to_char(obj, ch);
        delete text;
    }
    delete letter;
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

/*
if (no_mail) {
    send_to_char("Sorry, the mail system is having technical difficulties.\r\n", ch);
    return 0;
    }
*/
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
        total_cost += STAMP_PRICE;
      
        CREATE(n_mail_to, struct mail_recipient_data, 1);
        n_mail_to->next = ch->desc->mail_to;
        n_mail_to->recpt_idnum = member->idnum;
        ch->desc->mail_to = n_mail_to;
    }
    } else {
    
    while (*buf) {
        if ((recipient = get_id_by_name(buf)) < 0) {
        sprintf(buf2, "No one by the name '%s' is registered here!", buf);
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
    }
    
    // credits
    else {
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

    if (total_cost > STAMP_PRICE) {  /* multiple recipients */
    strcpy(buf, "  CC: ");
    for(n_mail_to = ch->desc->mail_to; n_mail_to; n_mail_to = n_mail_to->next){
        strcat(buf, get_name_by_id(n_mail_to->recpt_idnum));
        if (n_mail_to->next)
        strcat(buf, ", ");
        else
        strcat(buf, "\r\n");
    }
    string_add(ch->desc, buf);
    }
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


#define MIN_MAIL_LEVEL 1
#define STAMP_PRICE 50
#define MAX_MAIL_SIZE 4096
#define BLOCK_SIZE 100
#define HEADER_BLOCK  -1
#define LAST_BLOCK    -2
#define DELETED_BLOCK -3

#define MAX_MAIL_AGE 15552000 // should be 6 months.
#define PURGE_OLD_MAIL 1

/*
 * note: next_block is part of header_blk in a data block; we can't combine
 * them here because we have to be able to differentiate a data block from a
 * header block when booting mail system.
 */

struct header_data_type {
   long next_block;     /* if header block, link to next block  */
   long from;           /* idnum of the mail's sender       */
   long to;         /* idnum of mail's recipient        */
   time_t mail_time;        /* when was the letter mailed?      */
};

/* size of the data part of a header block */
#define HEADER_BLOCK_DATASIZE \
    (BLOCK_SIZE - sizeof(long) - sizeof(struct header_data_type) - 1)

/* size of the data part of a data block */
#define DATA_BLOCK_DATASIZE (BLOCK_SIZE - sizeof(long) - 1)
struct header_block_type_d {
   long block_type;     /* is this a header or data block?  */
   struct header_data_type header_data; /* other header data        */
   char txt[HEADER_BLOCK_DATASIZE+1]; /* actual text plus 1 for null    */
};

struct data_block_type_d {
   long block_type;     /* -1 if header block, -2 if last data block
                       in mail, otherwise a link to the next */
   char txt[DATA_BLOCK_DATASIZE+1]; /* actual text plus 1 for null  */
};

typedef struct header_block_type_d header_block_type;
typedef struct data_block_type_d data_block_type;

struct position_list_type_d {
   long position;
   struct position_list_type_d *next;
};

typedef struct position_list_type_d position_list_type;

struct mail_index_type_d {
   long recipient;          /* who is this mail for?    */
   position_list_type *list_start;  /* list of mail positions   */
   struct mail_index_type_d *next;  /* link to next one     */
};

typedef struct mail_index_type_d mail_index_type;


ACMD(do_toss_mail)
{
    FILE *mail_file;
    header_block_type next_block;
    int block_num = 0;
    data_block_type data;
    time_t current_time = 0;
    long following_block;
    long position;
    time_t mail_time;
    char *txt = NULL;
    long to_id=0,from_id=0;
    int letters_tossed = 0;
    int letters_ignored = 0;
    int letters_failed = 0;

    if (!(mail_file = fopen(MAIL_FILE, "r"))) {
        slog("Mail file non-existant... creating new file.");
        mail_file = fopen(MAIL_FILE, "w");
        fclose(mail_file);
        return;
    }
    current_time = time(NULL);
    while (fread(&next_block, sizeof(header_block_type), 1, mail_file)) {
        position = ftell(mail_file) - sizeof(header_block_type);
        if (next_block.block_type == HEADER_BLOCK) {
            if( PURGE_OLD_MAIL &&  
                next_block.header_data.mail_time + MAX_MAIL_AGE < current_time) {
                following_block = next_block.header_data.next_block;
                fseek(mail_file, position + BLOCK_SIZE, SEEK_SET);
                letters_ignored++;
            } else {
                to_id = next_block.header_data.to;
                from_id = next_block.header_data.from;
                mail_time = next_block.header_data.mail_time;
                txt = new char[MAX_MAIL_SIZE + 1];
                strcpy(txt,next_block.txt);
                
                following_block = next_block.header_data.next_block;
                while (following_block != LAST_BLOCK) {
                    fseek(mail_file, following_block, SEEK_SET);
                    fread(&data, BLOCK_SIZE, 1, mail_file);
                    fseek(mail_file, following_block, SEEK_SET);
                    following_block = data.block_type;
                    strcat(txt,data.txt);
                }
                fseek(mail_file, position + BLOCK_SIZE, SEEK_SET);
                if(!store_mail(to_id, from_id, txt, &mail_time)) {
                //    send_to_char("ERROR: Unable to store mail!\r\n",ch);
                    letters_failed++;
                    delete txt;
                    continue;
                }
                delete txt;
                letters_tossed++;
            }
        } 
        block_num++;
    }

    fclose(mail_file);
    sprintf(buf, "Mail tossed.\r\n    %d letters tossed.\r\n    %d letters ignored.\r\n    %d letters failed\r\n",
        letters_tossed, letters_ignored,letters_failed);
    send_to_char(buf,ch);
    return;
}
