//
// File: olc_hlp.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "specs.h"

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
extern int top_of_helpt;
extern struct help_index_element *help_index;
extern FILE *help_fl;
extern struct descriptor_data *descriptor_list;

/* External Functions */

int get_line_count(char *buffer);
struct help_index_element *build_help_index(FILE *fl, int *num);


/* Local Functions */

void do_help_fedit(struct char_data *ch, char *argument);
void do_help_list(struct char_data *ch);
void do_help_hedit(struct char_data *ch, char *argument);
void do_help_hstat(struct char_data *ch);
void do_help_hset(struct char_data *ch, char *argument);
struct help_index_element *do_create_help(struct char_data *ch);


extern char arg1[MAX_INPUT_LENGTH];
extern char arg2[MAX_INPUT_LENGTH];



void do_help_hset(struct char_data *ch, char *argument)
{
    struct olc_help_r *help;
  
    help = GET_OLC_HELP(ch);
  
    if (help == NULL) {
	send_to_char("You are not currently editing a help index record.\r\n", ch);
	return;
    }
  
    if (!*argument) {
	send_to_char("Usage: olc hset <text>\r\n", ch);
	return;
    }
  
    if (!is_abbrev(argument, "text")) {
	send_to_char("Usage: olc hset <text>\r\n", ch);
	return;
    }
  
    half_chop(argument, arg1, arg2);
    skip_spaces(&argument);
  
    if (is_abbrev(arg1, "text")) {
	if (help->text == NULL) {
	    CREATE(help->text, char, MAX_STRING_LENGTH);
	    send_to_char(" Write the help text.  Terminate with a @ on a new line.\r\n"
			 " Enter a * on a new line to enter TED\r\n"
			 " The FIRST line in the help text MUST be the KEYWORD string!\r\n", ch);
	    send_to_char(" [+--------+---------+---------+--------"
			 "-+---------+---------+---------+------+]\r\n", ch);
	    act("$n starts to write some help text.", TRUE, ch, 0, 0, TO_ROOM);
	    ch->desc->str = &help->text;
	    ch->desc->max_str = MAX_STRING_LENGTH;
	    SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	}
	else {
	    send_to_char("Use TED to modify the help text.\r\n"
			 "The FIRST line in the help text MUST be the KEYWORD string!\r\n", ch);
	    ch->desc->str = &help->text;
	    ch->desc->max_str = MAX_STRING_LENGTH;
	    ch->desc->editor_mode = 1;
	    ch->desc->editor_cur_lnum = get_line_count(help->text);
	    SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	    act("$n begins to edit some help text.", TRUE, ch, 0, 0, TO_ROOM);
	}
    }
}

void do_help_hstat(struct char_data *ch)
{
    struct olc_help_r *help;
  
    help = GET_OLC_HELP(ch);
  
    if (!help)
	send_to_char("You are not currently editing a help index record.\r\n", ch);
    else {
	sprintf(buf, "Keyword : %s\r\n"
		"Position: %ld\r\n"
		"Text    :-\r\n"
		"%s\r\n", GET_OLC_HELP(ch)->keyword, GET_OLC_HELP(ch)->pos, GET_OLC_HELP(ch)->text);
    
	send_to_char(buf, ch);
    }
  
}

void do_help_hedit(struct char_data *ch, char *argument)
{
    struct olc_help_r *help;
    FILE *bhelp_fl;
    char fname[64], *tmpbuf;
    int j, i, found = 0, pos = 0, tmp;
  
    help = GET_OLC_HELP(ch);

    if (GET_LEVEL(ch) < LVL_GOD)
	return;
  
    if (!*argument) {
	if (!help)
	    send_to_char("You are not currently editing a help index.\r\n", ch);
	else {
	    sprintf(buf, "Current olc help: [%s]\r\n", help->keyword);
	    send_to_char(buf, ch);
	}
	return;
    }
    if (!is_number(argument)) {
	if (is_abbrev(argument, "save")) {
	    if (help == NULL) {
		send_to_char("You have to edit a help index record first, pretzel boy.\r\n", ch);
		return;
	    }
      
	    tmpbuf = help->text;
      
	    do {
		*tmpbuf = toupper(*tmpbuf);
		tmpbuf++;
	    }
	    while (*tmpbuf != '\r' && *tmpbuf != '\n');
      
	    strcpy(fname, "text/bhelp.tmp");
	    bhelp_fl = fopen(fname, "w");
      
	    rewind(help_fl);
	    for (;;) {
		pos = ftell(help_fl);
		if (pos == help->pos) {
		    found = 1;
		    tmp = strlen(help->text);
		    for(i=0;i<tmp;i++)
			if (help->text[i] != '\r')
			    fputc(help->text[i], bhelp_fl);
		    fputs("#\n", bhelp_fl);
		    do
			fgets(buf, 128, help_fl);
		    while(*buf != '#');
		    if (*(buf + 1) == '~')
			break;
		    continue;
		}
		do {
		    fgets(buf, 128, help_fl);
		    if (*(buf + 1) != '~')
			fputs(buf, bhelp_fl);
		}
		while (*buf != '#');
		if (*(buf + 1) == '~')
		    break;
	    }
      
	    if (found != 1) {
		fputs("#\n", bhelp_fl);
		tmp = strlen(help->text);
		for(i=0;i<tmp;i++)
		    if (help->text[i] != '\r')
			fputc(help->text[i],bhelp_fl);
		fputs("\n#~\n", bhelp_fl);
	    }
      
	    fclose(bhelp_fl);
	    fclose(help_fl);
      
	    remove(HELP_KWRD_FILE);
	    rename("text/bhelp.tmp", HELP_KWRD_FILE);
      
	    help_fl = fopen(HELP_KWRD_FILE, "r");
      
	    send_to_char("Help file saved.\r\n", ch);
      
	    top_of_helpt++;
      
	    help_index = build_help_index(help_fl, &top_of_helpt);
      
	    send_to_char("On-line help index updated.\r\n", ch);
      
	    free(GET_OLC_HELP(ch)->keyword);
	    free(GET_OLC_HELP(ch)->text);
	    free(GET_OLC_HELP(ch));
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif      
	    GET_OLC_HELP(ch) = NULL;
      
	    return;
	}
	if (is_abbrev(argument, "exit")) {
	    send_to_char("Exiting help editor.\r\n", ch);
	    free(GET_OLC_HELP(ch)->keyword);
	    free(GET_OLC_HELP(ch)->text);
	    free(GET_OLC_HELP(ch));
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	    GET_OLC_HELP(ch) = NULL;
	    return;
	}
	send_to_char("The argument must be a help index number.\r\n", ch);
	return;
    } else {
    
	if (help != NULL) {
	    send_to_char("You are already editing an index record, use olc hedit exit to clear it.\r\n", ch);
	    return;
	}
    
	j = atoi(argument);
    
	for (i = 0; i <= top_of_helpt && found != 1; i++)
	    if ((i + 1) == j) {
		found = 1;
		CREATE(ch->player_specials->olc_help, struct olc_help_r, 1);
		GET_OLC_HELP(ch)->keyword = strdup(help_index[i].keyword);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		GET_OLC_HELP(ch)->pos = help_index[i].pos;
		if (help_index[i].pos == -999999)
		    break;
		fseek(help_fl, help_index[i].pos, SEEK_SET);
		*buf2 = '\0';
		for (;;) {
		    fgets(buf, 128, help_fl);
		    if (*buf == '#')
			break;
		    buf[strlen(buf) - 1] = '\0';	/* cleave off the trailing \n */
		    strcat(buf2, strcat(buf, "\r\n"));
		}
		CREATE(GET_OLC_HELP(ch)->text, char, strlen(buf2));
		strcpy(GET_OLC_HELP(ch)->text, buf2);
	    }
    
	if (found == 0) 
	    send_to_char("No such help index\r\n", ch);
	else {
	    sprintf(buf, "Now editing help index [%s]\r\n", GET_OLC_HELP(ch)->keyword);
	    send_to_char(buf, ch);
	}
    }
}

void do_help_list(struct char_data *ch)
{
    int i;
  
    strcpy(buf, "Num  Keyword\r\n"
	   "------------\r\n");
  
    for (i = 0; i <= top_of_helpt; i++)
	sprintf(buf, "%s%-2d] %s - %ld\r\n", buf, i+1, help_index[i].keyword, help_index[i].pos);
  
    page_string(ch->desc, buf, 0);
}


struct help_index_element *do_create_help(struct char_data *ch)
{

    if (GET_LEVEL(ch) < LVL_GOD)
	return NULL;

    top_of_helpt++;
  
    RECREATE(help_index, struct help_index_element, top_of_helpt);
  
    help_index[top_of_helpt].keyword = strdup("NEW HELP ITEM");
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    help_index[top_of_helpt].pos = -999999;
  
    sprintf(buf, "Created new help index %d\r\n", (top_of_helpt + 1));
    send_to_char(buf, ch);
  
    return(help_index);
}


void do_help_fedit(struct char_data *ch, char *argument)
{
    char *buffer, message[80];
    struct descriptor_data *d;
  
    if (GET_LEVEL(ch) < LVL_GOD)
	return;

#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    if (is_abbrev(argument, "news")) {
	buffer = news;
	ch->desc->editor_file = strdup(NEWS_FILE);
    }
    else if (is_abbrev(argument,"motd")) {
	buffer = motd;
	ch->desc->editor_file = strdup(MOTD_FILE);
    }
    else if (is_abbrev(argument, "imotd")) {
	buffer = imotd;
	ch->desc->editor_file = strdup(IMOTD_FILE);
    }
    else if (is_abbrev(argument, "info")) {
	buffer = info;
	ch->desc->editor_file = strdup(INFO_FILE);
    }
    else if (is_abbrev(argument, "wizlist")) {
	buffer = wizlist;
	ch->desc->editor_file = strdup(WIZLIST_FILE);
    }
    else if (is_abbrev(argument, "immlist")) {
	buffer = immlist;
	ch->desc->editor_file = strdup(IMMLIST_FILE);
    }
    else if (is_abbrev(argument, "background")) {
	buffer = background;
	ch->desc->editor_file = strdup(BACKGROUND_FILE);
    }
    else if (is_abbrev(argument, "handbook")) {
	buffer = handbook;
	ch->desc->editor_file = strdup(HANDBOOK_FILE);
    }
    else if (is_abbrev(argument, "policy")) {
	buffer = policies;
	ch->desc->editor_file = strdup(POLICIES_FILE);
    }
    else {
	send_to_char("Usage: olc fedit <news|motd|imotd|info|wizlist|immlist|background|handbook|policy|areas>\r\n", ch);
	return;
    }
#ifdef DMALLOC
    dmalloc_verify(0);
#endif  
    for (d = descriptor_list; d; d = d->next)
	if (d->str != NULL && GET_IDNUM(d->character) != GET_IDNUM(ch) && *d->str == buffer) {
	    sprintf(buf, "Sorry, %s is already editing that file.\r\n", GET_NAME(d->character));
	    send_to_char(buf, ch);
	    return;
	}
  
    sprintf(message, "Use TED to modify the %s file\r\n", argument);
    send_to_char(message, ch);
  
    ch->desc->str = &buffer;
    ch->desc->max_str = MAX_STRING_LENGTH;
    ch->desc->editor_mode = 1;
    ch->desc->editor_cur_lnum = get_line_count(buffer)+1;
    SET_BIT(PLR_FLAGS(ch), PLR_OLC);
    act("$n begins to modify a help file.", TRUE, ch, 0, 0, TO_ROOM);
}
