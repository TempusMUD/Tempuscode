/* ************************************************************************
*  File: editor.c                                                         *
*                                                                         * 
*  Usage: New and improved editor for Tempus                              *
*                                                                         *
*  All rights reserved.                                                   *
*                                                                         *
*  Copyright (C) 1996 by Chris Austin  (Stryker@TempusMUD)                *
************************************************************************ */

//
// File: editor.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"

static char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

static const char *editor_cmds[] = {
    "list",
    "exit",
    "help",
    "delete",
    "insert",
    "change",
    "replace",
    "purge",
    "\n"
};


/* External Functions */

void string_add(struct descriptor_data *d, char *str);
extern int log_cmds;

 /* Local Functions */

void process_editor_command(struct descriptor_data *d, char *buffer);
void do_list_command(struct descriptor_data *d, int start, int end);
void do_delete_command(struct descriptor_data *d, int start, int end);
void do_insert_command(struct descriptor_data *d, int start, char *text);
void do_change_command(struct descriptor_data *d, int start, char *text);
void do_purge_command(struct descriptor_data *d);
void do_help_command(struct descriptor_data *d, char *buffer);

static char oldbuf[MAX_MESSAGE_LENGTH], newbuf[MAX_MESSAGE_LENGTH];

void 
editor(struct descriptor_data *d, char *buffer)
{

    if (d->editor_mode == 1) {
	if (d->character && 
	    (log_cmds || PLR_FLAGGED(d->character, PLR_LOG))) {
	    sprintf(buf, "CMD: %s (editor) ::'%s'", GET_NAME(d->character), buffer);
	    slog(buf);
	}
	process_editor_command(d, buffer);
    }
    else if (*buffer == '*') {
	send_to_char("\r\nEntering command mode.\r\n", d->character);
	d->editor_mode = 1;
    }
    else {
	if (d->character && 
	    (log_cmds || PLR_FLAGGED(d->character, PLR_LOG))) {
	    sprintf(buf, "CMD: %s (writin) ::'%s'", GET_NAME(d->character), buffer);
	    slog(buf);
	}
	string_add(d, buffer);
	d->editor_cur_lnum++;
    }
}

void 
process_editor_command(struct descriptor_data *d, char *buffer)
{
    int cmd, start, end;
  
    half_chop(buffer, arg1, arg2);
    skip_spaces(&buffer);
  
    if ((cmd = search_block(arg1, editor_cmds, FALSE)) < 0) { 
	sprintf(buf, "Unknown command [%s] - Type help for usage.\r\n", arg1);
	send_to_char(buf, d->character);
	return;
    }
  
    switch(cmd) {
    case 0:            /** List **/
	buffer = two_arguments(buffer, arg2, arg1);
	buffer = one_argument(buffer, arg2);
      
	if (*arg1 && is_number(arg1)) {
	    if (!(*arg2)) {
		start = end = atoi(arg1);
		do_list_command(d, start, end);
		return;
	    }
	
	    if (!is_number(arg2)) {
		send_to_char("List usage:- list [[start line #] [end line #]]\r\n", d->character);
		return;
	    }
	
	    start = atoi(arg1);
	    end   = atoi(arg2);
	
	    if (start > end || start > 9999) {
		send_to_char("Funny today, aren't we?\r\n", d->character);
		return;
	    }
	
	    do_list_command(d, start, end);
	}
	else
	    do_list_command(d, 1, 9999);
	break;
    case 1:             /** Exit **/
	send_to_char("\r\n Exiting command mode.  Enter a * on a new line to re-enter.\r\n", d->character);
	send_to_char(" Continue with your message.  Terminate with a @ on a new line.\r\n", d->character);
	send_to_char(" [+--------+---------+---------+--------"
		     "-+---------+---------+---------+------+]\r\n", d->character);
	d->editor_mode = 0;
	break;
    case 2:             /** Help **/
	buffer = two_arguments(buffer, arg1, arg2);
	if (!*arg2) {
	    send_to_char("\r\nTED v0.75          - HELP -\r\n\r\n"
			 "Commands :-\r\n"
			 "   help      list      delete     insert\r\n"
			 "   change    replace   exit       purge\r\n\r\n"
			 "Enter help <command name> for an explanation.\r\n",
			 d->character);
	}
	else
	    do_help_command(d, arg2);
	break;
    case 3:             /** Delete **/
	buffer = two_arguments(buffer, arg2, arg1);
	buffer = one_argument(buffer, arg2);
      
	if (*arg1 && is_number(arg1)) {
	    if (!(*arg2)) {
		start = end = atoi(arg1);
		do_delete_command(d, start, end);
		return;
	    }
	
	    if (!is_number(arg2)) {
		send_to_char("Delete usage:- delete [[start line #] [end line #]]\r\n", d->character);
		return;
	    }
	
	    start = atoi(arg1);
	    end   = atoi(arg2);
	
	    if (start > end || start > 99) {
		send_to_char("Funny today, aren't we?\r\n", d->character);
		return;
	    }
	    do_delete_command(d, start, end);
	}
	else
	    send_to_char("Delete usage:- delete [[start line #] [end line #]]\r\n", d->character);
	break;
    case 4:             /** Insert **/
	buffer = two_arguments(buffer, arg2, arg1);
      
	if ((!*arg1 || !*buffer) || !is_number(arg1))
	    send_to_char("Insert usage:- insert <starting line #> <text>\r\n", d->character);
	else {
	    start = atoi(arg1);
	    do_insert_command(d, start, buffer);
	}
	break;
    case 5:             /** Change **/
	buffer = two_arguments(buffer, arg2, arg1);
      
	if ((!*arg1 || !*buffer) || !is_number(arg1))
	    send_to_char("Change usage:- change <line #> <text>\r\n", d->character);
	else {
	    start = atoi(arg1);
	    do_change_command(d, start, buffer);
	}
	break;
    case 6:               /** Replace **/
	send_to_char("Sorry, not yet implemented.\r\n", d->character);
	break;
    case 7:               /** Purge **/
	do_purge_command(d);
	break;
    }
}


void 
do_list_command(struct descriptor_data *d, int start, int end)
{
    char *line, *inbuf, *safebuf;
    int   i = 0;

    if (*d->str == NULL) {
	send_to_char("Nothing in the buffer!\r\n", d->character);
	return;
    }
  
    safebuf = inbuf = str_dup(*d->str);

    while ((line = strsep(&inbuf, "\n")) && i < end) {
	i++;
	if (i >= start) {
	    if (i > d->editor_cur_lnum)
		send_to_char("--past cur_lnum error--\r\n", d->character);
	    sprintf(buf, "%-2d] %s\r\n", i, line);
	    send_to_char(buf, d->character);
	}
    }
    if (safebuf)
	free (safebuf);
}

void 
do_delete_command(struct descriptor_data *d, int start, int end)
{
    char *line = NULL, *inbuf = NULL, newbuf[MAX_MESSAGE_LENGTH];
    int   i = 0, found = 0, top = 0;
  
    if (*d->str == NULL) {
	send_to_char("Nothing in the buffer!\r\n", d->character);
	return;
    }

    strcpy(oldbuf, *d->str);
    inbuf = oldbuf;
    *newbuf = '\0';
    top = d->editor_cur_lnum;
    while ((line = strsep(&inbuf, "\n")) && i < top-1) {
	i++;
	if (i >= start && i <= end) {
	    d->editor_cur_lnum--;
	    found++;
	    continue;
	}
	strcat(strcat(newbuf, line), "\n");
    }
  
    if (d->editor_cur_lnum < 1)
	d->editor_cur_lnum = 1;

    sprintf(buf, "[%d] lines deleted.\r\n", found);
    send_to_char(buf, d->character);

    sprintf(buf, "%s TED-deleted lines %d through %d at %d.",
	    GET_NAME(d->character), start, end,
	    d->character->in_room ? d->character->in_room->number : -1);
    slog(buf);
    fflush(stderr);

    free(*d->str);
    *d->str = str_dup(newbuf);
}

void 
do_insert_command(struct descriptor_data *d, int start, char *text)
{
    char *line = NULL, *inbuf = NULL;
    int   i = 1, found = 0;
  
    if (*d->str == NULL) {
	send_to_char("Nothing in the buffer!\r\n", d->character);
	return;
    }
    if (strlen(*d->str)+strlen(text)+1 > MAX_MESSAGE_LENGTH) {
	send_to_char("Maximum message size exceeded!", d->character);
	return;
    }

    text++;
    strcpy(oldbuf, *d->str);
    inbuf = oldbuf;
    *newbuf = '\0';

    while ((line = strsep(&inbuf, "\n"))) {
	if (i == start) {
	    strcat(strcat(newbuf, text), "\r\n");
	    if (i != d->editor_cur_lnum) {
		strcat(strcat(newbuf, line), "\n");
		if (inbuf) {
		    strcat(newbuf, inbuf);
		}
	    }
	    d->editor_cur_lnum++;
	    found = TRUE;
	    break;
	}
	i++;
	strcat(strcat(newbuf, line), "\n");
    }

    if (!found) {
	SEND_TO_Q("No such line number!!\r\n", d);
	return;
    }

    SEND_TO_Q("Line inserted OK.\r\n", d);

    sprintf(buf, "%s TED-deleted line %d at %d.",
	    GET_NAME(d->character), start,
	    d->character->in_room ? d->character->in_room->number : -1);
    slog(buf);
    fflush(stderr);


    free(*d->str);
    *d->str = str_dup(newbuf);
}
void 
do_change_command(struct descriptor_data *d, int start, char *text)
{
    char *line, *inbuf, *newbuf, *safebuf;
    int   i = 1;
  
    if (*d->str == NULL) {
	send_to_char("Nothing in the buffer!\r\n", d->character);
	return;
    }

    text++;
  
    safebuf = inbuf = strdup(*d->str);
  
    if (strlen(inbuf)+strlen(text)+1 > MAX_MESSAGE_LENGTH) {
	send_to_char("Maximum message size exceeded!", d->character);
	return;
    }
  
    CREATE(newbuf, char, strlen(inbuf)+strlen(text)+1);

    line = strsep(&inbuf, "\n");
  
    if (line == NULL) {
	send_to_char("No such line number!!\r\n", d->character);
	return;
    }
  
    line[strlen(line)-1] = '\0';
  
    strcat(text, "\r\n");
  
    do {
	if (i == start) { 
	    strcat(newbuf,text);
	    /* sprintf(newbuf, "%s%s", newbuf, text);*/
	    line = strsep(&inbuf, "\n");
	    if (line != NULL)
		line[strlen(line)-1] = '\0';
	    i++;
	    continue;
	}
	i++;
	strcat(newbuf,line);
	strcat(newbuf,"\r\n");
	/* sprintf(newbuf, "%s%s\r\n", newbuf, line); */    
	line = strsep(&inbuf, "\n");
	if (line != NULL)
	    line[strlen(line)-1] = '\0';
    } while (line != NULL && strcmp(line, "") != 0);
  
    sprintf(buf, "Changed line %d ok.\r\n", start);
    send_to_char(buf, d->character);

    sprintf(buf, "%s TED-changed line %d at %d.",
	    GET_NAME(d->character), start, 
	    d->character->in_room ? d->character->in_room->number : -1);
    slog(buf);
    fflush(stderr);
 
    free(*d->str);
    *d->str = newbuf;

    if (safebuf)
	free(safebuf);

#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}


void 
do_purge_command(struct descriptor_data *d)
{
    char *newbuf;
  
    CREATE(newbuf, char, MAX_STRING_LENGTH);
  
    free(*d->str);
    *d->str = newbuf = NULL;
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    d->max_str = MAX_STRING_LENGTH;
    d->editor_cur_lnum = 1;
  
    send_to_char("Buffer purged.\r\n", d->character);
}
  
void 
do_help_command(struct descriptor_data *d, char *buffer)
{
    int help_cmd;
  
    skip_spaces(&buffer);
  
    if ((help_cmd = search_block(buffer, editor_cmds, FALSE)) < 0) { 
	sprintf(buf, "Unknown command [%s] - Type help for command listing.\r\n", buffer);
	send_to_char(buf, d->character);
	return;
    }
  
    switch(help_cmd) {
    case 0:             /** List **/
	send_to_char("COMMAND:\r\n"
		     "    List - List will display the text you have already enterd so far.\r\n\r\n"
		     "USAGE:\r\n"
		     "    list [start line #] [end line #]\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The list command used by itself will list all of the text in your buffer.\r\n"
		     "    You can also list ranges of lines, ie list 1 5 will list lines 1 to 5 out.\r\n"
		     "    List 2, for example,  is also acceptable to list just 1 line.\r\n\r\n",
		     d->character);
	break;
    case 1:             /** Exit **/
	send_to_char("COMMAND:\r\n"
		     "    Exit - Exits TED and returns you to the line you were last on\r\n\r\n"
		     "USAGE:\r\n"
		     "    exit\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The exit command quits TED and returns you to inputting lines.  You are\r\n"
		     "    returned to the point where you left off.  The modifications that you\r\n"
		     "    did while in TED will be reflected after you exit.\r\n\r\n", d->character);
	break;
    case 2:             /** Help **/
	send_to_char("COMMAND:\r\n"
		     "    Help - Help will display the TED help screen.\r\n\r\n"
		     "USAGE:\r\n"
		     "    help\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The help command will display the TED help screen.  From there you can\r\n"
		     "    type help <command name> to display more detailed help per command.\r\n\r\n",
		     d->character);
	break;
    case 3:             /** Delete **/
	send_to_char("COMMAND:\r\n"
		     "    Delete - The delete command will delete lines of text from the buffer.\r\n\r\n"
		     "USAGE:\r\n"
		     "    delete [start line #] [end line #]\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The delete command will delete single lines or ranges of lines of text\r\n"
		     "    from the buffer.  Delete 5 will delete line 5.  Delete 3 6 will delete\r\n"
		     "    lines 3 4 5 and 6 from your buffer.\r\n\r\n", d->character);
	break;
    case 4:             /** Insert **/
	send_to_char("COMMAND:\r\n"
		     "    Insert - The insert command will insert a line of text.\r\n\r\n"
		     "USAGE:\r\n"
		     "    insert <line #> <text>\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The insert command will insert a line of text at a given line.  Note that\r\n"
		     "    the line number specified will be used to insert the text and the existing\r\n"
		     "    line at that position will be moved down 1 along with all of the other text.\r\n\r\n",
		     d->character);
	break;
    case 5:             /** Change **/
	send_to_char("COMMAND:\r\n"
		     "    Change - The change command will replace a line of text.\r\n\r\n"
		     "USAGE:\r\n"
		     "    change <line #> <text>\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The change command will replace a line of text at a given line position.\r\n"
		     "    the line number specified will be totaly replaced by the text specified.\r\n\r\n",
		     d->character);
	break;
    case 6:               /** Replace **/
	send_to_char("Sorry, not yet implemented.\r\n", d->character);
	break;
    case 7:               /** Purge **/
	send_to_char("COMMAND:\r\n"
		     "    Purge - The purge command will purge your buffer.\r\n\r\n"
		     "USAGE:\r\n"
		     "    purge\r\n\r\n"
		     "DESCRIPTION:\r\n"
		     "    The purge command will purge the contents of your buffer.\r\n",
		     d->character);
	break;
    
    }
}
