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

void postmaster_send_mail(struct char_data * ch, struct char_data *mailman,
			  int cmd, char *arg);
void postmaster_check_mail(struct char_data * ch, struct char_data *mailman,
			   int cmd, char *arg);
void postmaster_receive_mail(struct char_data * ch, struct char_data *mailman,
			     int cmd, char *arg);
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);

void string_add(struct descriptor_data *d, char *str);

extern int no_mail;
extern int no_plrtext;

mail_index_type *mail_index = 0;/* list of recs in the mail file  */
position_list_type *free_list = 0;	/* list of free positions in file */
long file_end_pos = 0;		/* length of file */


void 
push_free_list(long pos)
{
    position_list_type *new_pos;

    CREATE(new_pos, position_list_type, 1);
    new_pos->position = pos;
    new_pos->next = free_list;
    free_list = new_pos;
}



long 
pop_free_list(void)
{
    position_list_type *old_pos;
    long return_value;

    if ((old_pos = free_list) != 0) {
	return_value = free_list->position;
	free_list = old_pos->next;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(old_pos);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	return return_value;
    } else
	return file_end_pos;
}

void
show_mail_stats(char_data *ch)
{
	long num_free_positions = 0;
	long num_used_positions = 0;
	mail_index_type *cur_index = NULL;
	position_list_type *cur_pos = NULL;
	for ( cur_pos  = free_list; cur_pos; cur_pos = cur_pos->next ) {
		num_free_positions++;
	}
	for ( cur_index = mail_index; cur_index; cur_index = cur_index->next ) {
		for ( cur_pos = cur_index->list_start;cur_pos;cur_pos = cur_pos->next ) {
			num_used_positions++;
		}
	}
	sprintf(buf,"Unused Blocks: %ld  Total Unused Space: %ld bytes.\r\n",num_free_positions, num_free_positions * BLOCK_SIZE);
	send_to_char(buf,ch);
	sprintf(buf,"Letters currently in file: %ld.\r\n",num_used_positions);
	send_to_char(buf,ch);
}

mail_index_type *
find_char_in_index(long searchee)
{
    mail_index_type *tmp;

    if (searchee < 0) {
	slog("SYSERR: Mail system -- non fatal error #1.");
	return 0;
    }
    for (tmp = mail_index; (tmp && tmp->recipient != searchee); tmp = tmp->next);

    return tmp;
}

void 
write_to_file(void *buf, int size, long filepos)
{
    FILE *mail_file;

    mail_file = fopen(MAIL_FILE, "r+b");

    if (filepos % BLOCK_SIZE) {
	slog("SYSERR: Mail system -- fatal error #2!!!");
	no_mail = 1;
	return;
    }
    fseek(mail_file, filepos, SEEK_SET);
    fwrite(buf, size, 1, mail_file);

    /* find end of file */
    fseek(mail_file, 0L, SEEK_END);
    file_end_pos = ftell(mail_file);
    fclose(mail_file);
    return;
}


void 
read_from_file(void *buf, int size, long filepos)
{
    FILE *mail_file;

    mail_file = fopen(MAIL_FILE, "r+b");

    if (filepos % BLOCK_SIZE) {
	slog("SYSERR: Mail system -- fatal error #3!!!");
	no_mail = 1;
	return;
    }
    fseek(mail_file, filepos, SEEK_SET);
    fread(buf, size, 1, mail_file);
    fclose(mail_file);
    return;
}




void 
index_mail(long id_to_index, long pos)
{
    mail_index_type *new_index;
    position_list_type *new_position;

    if (id_to_index < 0) {
	slog("SYSERR: Mail system -- non-fatal error #4.");
	return;
    }
    if (!(new_index = find_char_in_index(id_to_index))) {
	/* name not already in index.. add it */
	CREATE(new_index, mail_index_type, 1);
	new_index->recipient = id_to_index;
	new_index->list_start = NULL;

	/* add to front of list */
	new_index->next = mail_index;
	mail_index = new_index;
    }
    /* now, add this position to front of position list */
    CREATE(new_position, position_list_type, 1);
    new_position->position = pos;
    new_position->next = new_index->list_start;
    new_index->list_start = new_position;
}


/* SCAN_FILE */
/* scan_file is called once during boot-up.  It scans through the mail file
   and indexes all entries currently in the mail file. */
int 
scan_file(void)
{
    FILE *mail_file;
    header_block_type next_block;
    int total_messages = 0, block_num = 0;
    char buf[100];

    if (!(mail_file = fopen(MAIL_FILE, "r"))) {
	slog("Mail file non-existant... creating new file.");
	mail_file = fopen(MAIL_FILE, "w");
	fclose(mail_file);
	return 1;
    }
    while (fread(&next_block, sizeof(header_block_type), 1, mail_file)) {
	if (next_block.block_type == HEADER_BLOCK) {
	    index_mail(next_block.header_data.to, block_num * BLOCK_SIZE);
	    total_messages++;
	} else if (next_block.block_type == DELETED_BLOCK)
	    push_free_list(block_num * BLOCK_SIZE);
	block_num++;
    }

    file_end_pos = ftell(mail_file);
    fclose(mail_file);
    sprintf(buf, "   %ld bytes read.", file_end_pos);
    slog(buf);
    if (file_end_pos % BLOCK_SIZE) {
	slog("SYSERR: Error booting mail system -- Mail file corrupt!");
	slog("SYSERR: Mail disabled!");
	return 0;
    }
    sprintf(buf, "   Mail file read -- %d messages.", total_messages);
    slog(buf);
    return 1;
}				/* end of scan_file */


/* HAS_MAIL */
/* a simple little function which tells you if the guy has mail or not */
int 
has_mail(long recipient)
{
    if (find_char_in_index(recipient))
	return 1;
    return 0;
}



/* STORE_MAIL  */
/* call store_mail to store mail.  (hard, huh? :-) )  Pass 3 arguments:
   who the mail is to (long), who it's from (long), and a pointer to the
   actual message text (char *).
*/

void 
store_mail(long to, long from, char *message_pointer)
{
    header_block_type header;
    data_block_type data;
    long last_address, target_address;
    char *msg_txt = message_pointer;
    int bytes_written = 0;
    int total_length = strlen(message_pointer);

    if (sizeof(header_block_type) != sizeof(data_block_type))
	raise ( SIGSEGV );

    if (sizeof(header_block_type) != BLOCK_SIZE)
	raise( SIGSEGV );

    if (from < 0 || to < 0 || !*message_pointer) {
	slog("SYSERR: Mail system -- non-fatal error #5.");
	return;
    }
    memset((char *) &header, 0, sizeof(header));	/* clear the record */
    header.block_type = HEADER_BLOCK;
    header.header_data.next_block = LAST_BLOCK;
    header.header_data.from = from;
    header.header_data.to = to;
    header.header_data.mail_time = time(0);
    strncpy(header.txt, msg_txt, HEADER_BLOCK_DATASIZE);
    header.txt[HEADER_BLOCK_DATASIZE] = '\0';

    target_address = pop_free_list();	/* find next free block */
    index_mail(to, target_address);	/* add it to mail index in memory */
    write_to_file(&header, BLOCK_SIZE, target_address);

    if (strlen(msg_txt) <= HEADER_BLOCK_DATASIZE)
	return;			/* that was the whole message */

    bytes_written = HEADER_BLOCK_DATASIZE;
    msg_txt += HEADER_BLOCK_DATASIZE;	/* move pointer to next bit of text */

    /*
     * find the next block address, then rewrite the header to reflect where
     * the next block is.
     */
    last_address = target_address;
    target_address = pop_free_list();
    header.header_data.next_block = target_address;
    write_to_file(&header, BLOCK_SIZE, last_address);

    /* now write the current data block */
    memset((char *) &data, 0, sizeof(data));	/* clear the record */
    data.block_type = LAST_BLOCK;
    strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
    data.txt[DATA_BLOCK_DATASIZE] = '\0';
    write_to_file(&data, BLOCK_SIZE, target_address);
    bytes_written += strlen(data.txt);
    msg_txt += strlen(data.txt);

    /*
     * if, after 1 header block and 1 data block there is STILL part of the
     * message left to write to the file, keep writing the new data blocks and
     * rewriting the old data blocks to reflect where the next block is.  Yes,
     * this is kind of a hack, but if the block size is big enough it won't
     * matter anyway.  Hopefully, MUD players won't pour their life stories out
     * into the Mud Mail System anyway.
     * 
     * Note that the block_type data field in data blocks is either a number >=0,
     * meaning a link to the next block, or LAST_BLOCK flag (-2) meaning the
     * last block in the current message.  This works much like DOS' FAT.
     */

    while (bytes_written < total_length) {
	last_address = target_address;
	target_address = pop_free_list();

	/* rewrite the previous block to link it to the next */
	data.block_type = target_address;
	write_to_file(&data, BLOCK_SIZE, last_address);

	/* now write the next block, assuming it's the last.  */
	data.block_type = LAST_BLOCK;
	strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
	data.txt[DATA_BLOCK_DATASIZE] = '\0';
	write_to_file(&data, BLOCK_SIZE, target_address);

	bytes_written += strlen(data.txt);
	msg_txt += strlen(data.txt);
    }
}				/* store mail */




/* READ_DELETE */
/* read_delete takes 1 char pointer to the name of the person whose mail
   you're retrieving.  It returns to you a char pointer to the message text.
   The mail is then discarded from the file and the mail index. */

char *
read_delete(long recipient)
/* recipient is the name as it appears in the index.
   recipient_formatted is the name as it should appear on the mail
   header (i.e. the text handed to the player) */
{
    header_block_type header;
    data_block_type data;
    mail_index_type *mail_pointer, *prev_mail;
    position_list_type *position_pointer;
    long mail_address, following_block;
    char *message, *tmstr, buf[200];
    size_t string_size;

    if (recipient < 0) {
	slog("SYSERR: Mail system -- non-fatal error #6.");
	return 0;
    }
    if (!(mail_pointer = find_char_in_index(recipient))) {
	slog("SYSERR: Mail system -- post office spec_proc error?  Error #7.");
	return 0;
    }
    if (!(position_pointer = mail_pointer->list_start)) {
	slog("SYSERR: Mail system -- non-fatal error #8.");
	return 0;
    }
    if (!(position_pointer->next)) {	/* just 1 entry in list. */
	mail_address = position_pointer->position;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(position_pointer);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	/* now free up the actual name entry */
	if (mail_index == mail_pointer) {	/* name is 1st in list */
	    mail_index = mail_pointer->next;
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free(mail_pointer);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif  
	} else {
	    /* find entry before the one we're going to del */
	    for (prev_mail = mail_index;
		 prev_mail->next != mail_pointer;
		 prev_mail = prev_mail->next);
	    prev_mail->next = mail_pointer->next;
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free(mail_pointer);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	}
    } else {
	/* move to next-to-last record */
	while (position_pointer->next->next)
	    position_pointer = position_pointer->next;
	mail_address = position_pointer->next->position;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(position_pointer->next);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	position_pointer->next = 0;
    }

    /* ok, now lets do some readin'! */
    read_from_file(&header, BLOCK_SIZE, mail_address);

    if (header.block_type != HEADER_BLOCK) {
	slog("SYSERR: Oh dear.");
	no_mail = 1;
	slog("SYSERR: Mail system disabled!  -- Error #9.");
	return 0;
    }
    tmstr = asctime(localtime(&header.header_data.mail_time));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    sprintf(buf, " * * * *  Tempus Mail System  * * * *\r\n"
	    "Date: %s\r\n"
	    "  To: %s\r\n"
	    "From: %s\r\n", tmstr, get_name_by_id(recipient),
	    get_name_by_id(header.header_data.from));

    string_size = (sizeof(char) * (strlen(buf) + strlen(header.txt) + 1));
    CREATE(message, char, string_size);
    strcpy(message, buf);
    message[strlen(buf)] = '\0';
    strcat(message, header.txt);
    message[string_size - 1] = '\0';
    following_block = header.header_data.next_block;

    /* mark the block as deleted */
    header.block_type = DELETED_BLOCK;
    write_to_file(&header, BLOCK_SIZE, mail_address);
    push_free_list(mail_address);

    while (following_block != LAST_BLOCK) {
	read_from_file(&data, BLOCK_SIZE, following_block);

	string_size = (sizeof(char) * (strlen(message) + strlen(data.txt) + 1));
	RECREATE(message, char, string_size);
	strcat(message, data.txt);
	message[string_size - 1] = '\0';
	mail_address = following_block;
	following_block = data.block_type;
	data.block_type = DELETED_BLOCK;
	write_to_file(&data, BLOCK_SIZE, mail_address);
	push_free_list(mail_address);
    }

    return message;
}


/*****************************************************************
** Below is the spec_proc for a postmaster using the above       **
** routines.  Written by Jeremy Elson (jelson@server.cs.jhu.edu) **
*****************************************************************/

SPECIAL(postmaster)
{
    if (!ch->desc || IS_NPC(ch))
	return 0;			/* so mobs don't get caught here */

    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
	return 0;

    if (no_mail) {
	send_to_char("Sorry, the mail system is having technical difficulties.\r\n", ch);
	return 0;
    }

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

    if (!*buf) {			/* you'll get no argument from me! */
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


#define MAIL_OBJ_VNUM 1204
void 
postmaster_receive_mail(struct char_data * ch, struct char_data *mailman,
			int cmd, char *arg)
{
    char buf2[256];
    struct obj_data *obj;

    if (!has_mail(GET_IDNUM(ch))) {
	strcpy(buf2, "Sorry, you don't have any mail waiting.");
	perform_tell(mailman, ch, buf2);
	return;
    }
    while (has_mail(GET_IDNUM(ch))) {
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	if (!(obj = read_object(MAIL_OBJ_VNUM))) {
	    slog("SYSERR: Unable to load MAIL_OBJ_VNUM in postmaster_receive_mail");
	    return;
	}

	obj->action_description = read_delete(GET_IDNUM(ch));

	if (obj->action_description == NULL)
	    obj->action_description =
		str_dup("Mail system error - please report.  Error #11.\r\n");

	if (!no_plrtext && obj->action_description) {
	    obj->plrtext_len = strlen(obj->action_description) + 1;
	}

	obj_to_char(obj, ch);

	act("$n gives you a piece of mail.", FALSE, mailman, 0, ch, TO_VICT);
	act("$N gives $n a piece of mail.", FALSE, ch, 0, mailman, TO_ROOM);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
    }
    save_char(ch, NULL);
}



