/* ************************************************************************
*   File: boards.c                                      Part of CircleMUD *
*  Usage: handling of multiple bulletin boards                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: boards.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/* FEATURES & INSTALLATION INSTRUCTIONS ***********************************

Written by Jeremy "Ras" Elson (jelson@cs.jhu.edu)

This board code has many improvements over the infamously buggy standard
Diku board code.  Features include:

- Arbitrary number of boards handled by one set of generalized routines.
  Adding a new board is as easy as adding another entry to an array.
- Safe removal of messages while other messages are being written.
- Does not allow messages to be removed by someone of a level less than
  the poster's level.


TO ADD A NEW BOARD, simply follow our easy 4-step program:

1 - Create a new board object in the object files

2 - Increase the NUM_OF_BOARDS constant in board.h

3 - Add a new line to the board_info array below.  The fields, in order, are:

	Board's vnum number.
	Min level one must be to look at this board or read messages on it.
	Min level one must be to post a message to the board.
	Min level one must be to remove other people's messages from this
		board (but you can always remove your own message).
	Filename of this board, in quotes.
	Last field must always be 0.

4 - In spec_assign.c, find the section which assigns the special procedure
    gen_board to the other bulletin boards, and add your new one in a
    similar fashion.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "clan.h"

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern int mini_mud;

/*
format:	vnum, read lvl, write lvl, remove lvl, filename, 0 at end
Be sure to also change NUM_OF_BOARDS in board.h
*/
struct board_info_type board_info[NUM_OF_BOARDS] = {
    {{3099, 22920, 22613, 72333,-1,-1,-1,-1,-1,-1},  0, LVL_CAN_POST, LVL_GOD, "etc/board.mort"},
    {{3098, -1, -1, -1, -1, -1, -1, -1, -1, -1},LVL_AMBASSADOR, LVL_AMBASSADOR, LVL_GRGOD, "etc/board.immort"},
    {{3097, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, LVL_FREEZE, LVL_CREATOR, "etc/board.freeze"},
    {{3096, 22921, 22614, 72334,-1,-1,-1,-1,-1,-1},  0, LVL_CAN_POST, LVL_DEMI, "etc/board.social"},
    {{3094, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, LVL_CAN_POST, LVL_ELEMENT, "etc/board.obscene"},
    {{3095, 53322, -1, -1,-1,-1,-1,-1,-1,-1},        0, 0, LVL_ELEMENT, "etc/board.newbie"},
    {{3192, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, 0, LVL_ETERNAL, "etc/board.guild_cleric"},
    {{3193, 22893, -1, -1,-1,-1,-1,-1,-1,-1},        0, 0, LVL_ETERNAL, "etc/board.guild_pain"},
    {{3194, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, 0, LVL_ETERNAL, "etc/board.guild_justice"},
    {{3195, 22894, -1, -1,-1,-1,-1,-1,-1,-1},        0, 0, LVL_ETERNAL, "etc/board.guild_ares"},
    {{3196, 22895, -1, -1,-1,-1,-1,-1,-1,-1},        0, 0, LVL_ETERNAL, "etc/board.guild_thief"},
    {{3197, 22618, 22614, -1,-1,-1,-1,-1,-1,-1},     0, 0, LVL_ETERNAL, "etc/board.guild_mage"},
    {{3198, 22892, -1, -1,-1,-1,-1,-1,-1,-1},        0, 0, LVL_ETERNAL, "etc/board.guild_barb"},
    {{3199, 72220, -1, -1,-1,-1,-1,-1,-1,-1},           0, 0, LVL_ETERNAL, "etc/board.guild_ranger"},
    {{1294, 22919, -1, -1,-1,-1,-1,-1,-1,-1},        0, LVL_CAN_POST, LVL_DEMI,  "etc/board.clan_public"},
    {{1293, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, LVL_CAN_POST, LVL_ETERNAL,  "etc/board.quest"},
    {{30091, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_hoodlum"}, 
    {{30092, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_monk"}, 
    {{30093, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_psychic"}, 
    {{30094, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_physic"},
    {{30095, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_cyborg"},
    {{30088, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_ETERNAL, "etc/board.guild_merc"}, 
    {{1296, -1, -1, -1,-1,-1,-1,-1,-1,-1},           LVL_IMMORT, LVL_IMMORT, LVL_GOD,     "etc/board.world_general"},
    {{1295, 22651, -1, -1,-1,-1,-1,-1,-1,-1},        0, LVL_CAN_POST, LVL_GOD,     "etc/board.ideas"},
    {{1298, -1, -1, -1,-1,-1,-1,-1,-1,-1},           0, LVL_AMBASSADOR, LVL_ETERNAL, "etc/board.to_do_list"}, 
    {{72309,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR,"etc/board.dagger_clan"},
    {{72401,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.fa_clan"},
    {{1299, -1, -1, -1,-1,-1,-1,-1,-1,-1},           LVL_AMBASSADOR, LVL_DEMI, LVL_TIMEGOD, "etc/board.imp"},
    {{63047, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, 0, LVL_GOD, "etc/board.council"}, 
    {{30089, -1, -1, -1,-1,-1,-1,-1,-1,-1},          0, LVL_CAN_POST, LVL_DEMI, "etc/board.char_classified"},
    {{1250, -1, -1, -1,-1,-1,-1,-1,-1,-1},           LVL_IMMORT, LVL_IMMORT, LVL_LUCIFER, "etc/board.rulings"},
    {{72801,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.toreador_clan"},
    {{72099,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.jerrytown_clan"},
    {{72517,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.deathleague_clan"},
    {{73010,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.quiet_storm_clan"},
    {{73400,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN, LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.regime_clan"},
    {{73603,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.coven_clan"},
    {{72601,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.venom_clan"},
    {{74102,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.blacksun_clan"},
    {{74007,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.family_clan"},
    {{72212,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.brethren_clan"},
    {{75010,-1,-1, -1,-1,-1,-1,-1,-1,-1},            LVL_CAN_CLAN,LVL_CAN_CLAN,LVL_AMBASSADOR, "etc/board.blade_clan"},

    {{42503,-1,-1,-1,-1,-1,-1,-1,-1,-1},             0, 0, LVL_IMMORT, "etc/board.astral_mase"},
    {{1292,-1,-1,-1,-1,-1,-1,-1,-1,-1},              0, LVL_CAN_POST, LVL_GRGOD, "etc/board.real_estate"},
    {{1291,-1,-1,-1,-1,-1,-1,-1,-1,-1},              0, LVL_CAN_POST, LVL_IMMORT, "etc/board.story"},
    {{1200,-1,-1,-1,-1,-1,-1,-1,-1,-1},              LVL_GOD, LVL_GOD, LVL_GOD, "etc/board.admin"}
};


char *msg_storage[INDEX_SIZE];
int msg_storage_taken[INDEX_SIZE];
int num_of_msgs[NUM_OF_BOARDS];
int CMD_READ, CMD_LOOK, CMD_EXAMINE, CMD_WRITE, CMD_REMOVE;
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];


int 
find_slot(void)
{
    int i;

    for (i = 0; i < INDEX_SIZE; i++)
	if (!msg_storage_taken[i]) {
	    msg_storage_taken[i] = 1;
	    return i;
	}
    return -1;
}


/* search the room ch is standing in to find which board he's looking at */
int 
find_board(struct obj_data *obj)
{
    int i, j;

    for (i = 0; i < NUM_OF_BOARDS; i++) {
		for (j = 0; j < NUM_OF_BOARD_VNUMS; j++) {
			if (BOARD_VNUM(i, j) != -1 && BOARD_VNUM(i, j) == GET_OBJ_VNUM(obj)) {
				return i;
			}
		}
    }
  
    return -1;
}


void 
init_boards(void)
{
    int i, j, fatal_error = 0;

    for (i = 0; i < INDEX_SIZE; i++) {
	msg_storage[i] = 0;
	msg_storage_taken[i] = 0;
    }

    for (i = 0; i < NUM_OF_BOARDS; i++) {
	/*    if (!real_object_proto(BOARD_VNUM(i, 0))) {
	      sprintf(buf, "SYSERR: Fatal board error: board vnum %d does not exist!",
	      BOARD_VNUM(i, 0));
	      slog(buf);
	      fatal_error = 1;
	      }*/
	num_of_msgs[i] = 0;
	for (j = 0; j < MAX_BOARD_MESSAGES; j++) {
	    memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
	    msg_index[i][j].slot_num = -1;
	}
	Board_load_board(i);
    }

    CMD_READ = find_command("read");
    CMD_WRITE = find_command("write");
    CMD_REMOVE = find_command("remove");
    CMD_LOOK = find_command("look");
    CMD_EXAMINE = find_command("examine");

    if (fatal_error)
	safe_exit(1);
}


SPECIAL(gen_board)
{
    int board_type;
    static int loaded = 0;
    struct obj_data *obj = (struct obj_data *) me;

    if (!loaded) {
	init_boards();
	loaded = 1;
    }
    if (!ch->desc)
	return 0;

    if (cmd != CMD_WRITE && cmd != CMD_LOOK && cmd != CMD_EXAMINE &&
	cmd != CMD_READ && cmd != CMD_REMOVE)
	return 0;

    if ((board_type = find_board(obj)) == -1) {
	sprintf(buf, "SYSERR: board vnum %d is degenerate.", GET_OBJ_VNUM(obj));
	slog(buf);
	return 0;
    }
    if (cmd == CMD_WRITE) {
	Board_write_message(board_type, ch, obj, argument);
	return 1;
    } else if (cmd == CMD_LOOK || cmd == CMD_EXAMINE)
	return (Board_show_board(board_type, ch, obj, argument));
    else if (cmd == CMD_READ)
	return (Board_display_msg(board_type, ch, obj, argument));
    else if (cmd == CMD_REMOVE)
	return (Board_remove_msg(board_type, ch, obj, argument));
    else
	return 0;
}


void 
Board_write_message(int board_type, struct char_data * ch, struct obj_data *obj, char *arg)
{
    char *tmstr;
    int len;
    time_t ct;
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    struct mail_recipient_data *n_mail_to;

    if (PLR_FLAGGED(ch, PLR_NOPOST)) {
	send_to_char("You cannot post.\r\n", ch);
	return;
    }

    if (!obj->in_room) {
	act("You cannot write on $p unless it is in a room.", FALSE, ch, obj, 0, TO_CHAR);
	return;
    }

    if (GET_LEVEL(ch) < WRITE_LVL(board_type)) {
	send_to_char("You cannot write on this board yet.\r\n", ch);
	return;
    }
    if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES) {
	send_to_char("The board is full.\r\n", ch);
	return;
    }
    if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1) {
	send_to_char("The board is malfunctioning - sorry.\r\n", ch);
	slog("SYSERR: Board: failed to find empty slot on write.");
	return;
    }
    /* skip blanks */
    skip_spaces(&arg);
    delete_doubledollar(arg);

    /* Truncate to 80 characters. */
    arg[81] = '0';

    if (!*arg) {
	send_to_char("We must have a headline!\r\n", ch);
	return;
    }
    ct = time(0);
    tmstr = (char *) asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    sprintf(buf2, "(%s)", GET_NAME(ch));
    sprintf(buf, "%6.10s %-12s :: %s", tmstr, buf2, arg);
    len = strlen(buf) + 1;
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    if (!(NEW_MSG_INDEX(board_type).heading = (char *) malloc(sizeof(char) * len))) {
	send_to_char("The board is malfunctioning - sorry.\r\n", ch);
	return;
    }
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    strcpy(NEW_MSG_INDEX(board_type).heading, buf);
    NEW_MSG_INDEX(board_type).heading[len - 1] = '\0';
    NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

    start_text_editor(ch->desc,
        &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]),
        true,
        MAX_MESSAGE_LENGTH);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

    act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

    CREATE(n_mail_to, struct mail_recipient_data, 1);
    n_mail_to->next = NULL;
    n_mail_to->recpt_idnum = board_type + BOARD_MAGIC;
    ch->desc->mail_to = n_mail_to; 

    num_of_msgs[board_type]++;
}


int 
Board_show_board(int board_type, struct char_data * ch, struct obj_data *obj, char *arg)
{
    int i;
    char tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

    if (!ch->desc)
	return 0;

    one_argument(arg, tmp);

    if (!*tmp || !isname(tmp, "board bulletin"))
	return 0;

    if (!obj->in_room) {
	act("You cannot look at $p unless it is in a room.", FALSE, ch, obj, 0, TO_CHAR);
	return 1;
    }

    if (GET_LEVEL(ch) < READ_LVL(board_type)) {
	send_to_char("You try but fail to understand the words.\r\n", ch);
	return 1;
    }
    act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM);
  
    strcpy(buf,
	   "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n"
	   "You will need to look at the board to save your message.\r\n");
    if (!num_of_msgs[board_type]) 
	sprintf(buf, "%s%s%sThe board is empty.%s\r\n", buf, CCRED(ch, C_NRM), 
		CCBLD(ch, C_NRM), CCNRM(ch, C_NRM));
    else {
	sprintf(buf + strlen(buf), "%sThere are %d messages on the board.%s\r\n",
		CCGRN(ch, C_NRM), num_of_msgs[board_type], CCNRM(ch, C_NRM));
	/*    for (i = 0; i < num_of_msgs[board_type]; i++) { */
	for (i = num_of_msgs[board_type] - 1; i >= 0; i--) {
	    if (MSG_HEADING(board_type, i))
		sprintf(buf + strlen(buf), "%s%-2d %s:%s %s\r\n", CCGRN(ch, C_NRM), 
			i + 1, CCRED(ch, C_NRM), CCNRM(ch, C_NRM), MSG_HEADING(board_type, i));
	    else {
		slog("SYSERR: The board is fubar'd.");
		send_to_char("Sorry, the board isn't working.\r\n", ch);
		return 1;
	    }
	}
    }
    page_string(ch->desc, buf, 1);

    return 1;
}


int 
Board_display_msg(int board_type, struct char_data * ch, struct obj_data *obj, char *arg)
{
    char number[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
    int msg, ind;

    one_argument(arg, number);
    if (!*number)
	return 0;
    if (isname(number, "board bulletin"))	/* so "read board" works */
	return (Board_show_board(board_type, ch, obj, arg));
    if (!isdigit(*number) || (!(msg = atoi(number))))
	return 0;

    if (!obj->in_room) {
	act("You cannot look at $p unless it is in a room.", FALSE, ch, obj, 0, TO_CHAR);
	return 1;
    }

    if (GET_LEVEL(ch) < READ_LVL(board_type)) {
	send_to_char("You try but fail to understand the words.\r\n", ch);
	return 1;
    }
    if (!num_of_msgs[board_type]) {
	send_to_char("The board is empty!\r\n", ch);
	return (1);
    }
    if (msg < 1 || msg > num_of_msgs[board_type]) {
	send_to_char("That message exists only in your imagination.\r\n",
		     ch);
	return (1);
    }
    ind = msg - 1;
    if (MSG_SLOTNUM(board_type, ind) < 0 ||
	MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE) {
	send_to_char("Sorry, the board is not working.\r\n", ch);
	slog("SYSERR: Board is screwed up.");
	return 1;
    }
    if (!(MSG_HEADING(board_type, ind))) {
	send_to_char("That message appears to be screwed up.\r\n", ch);
	return 1;
    }
    if (!(msg_storage[MSG_SLOTNUM(board_type, ind)])) {
	send_to_char("That message seems to be empty.\r\n", ch);
	return 1;
    }
    sprintf(buffer, "%sMessage %d : %s%s\r\n\r\n%s\r\n", CCBLD(ch, C_CMP), msg,
	    MSG_HEADING(board_type, ind), CCNRM(ch, C_CMP),
	    msg_storage[MSG_SLOTNUM(board_type, ind)]); 

    page_string(ch->desc, buffer, 1);

    return 1;
}


int 
Board_remove_msg(int board_type, struct char_data * ch, struct obj_data *obj, char *arg)
{
    int ind, msg, slot_num;
    char number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    struct descriptor_data *d;

    one_argument(arg, number);

    if (!*number || !isdigit(*number))
	return 0;
    if (!(msg = atoi(number)))
	return (0);

    if (!obj->in_room) {
	act("You cannot edit $p unless it is in a room.", FALSE, ch, obj, 0, TO_CHAR);
	return 1;
    }

    if (!num_of_msgs[board_type]) {
	send_to_char("The board is empty!\r\n", ch);
	return 1;
    }
    if (msg < 1 || msg > num_of_msgs[board_type]) {
	send_to_char("That message exists only in your imagination.\r\n", ch);
	return 1;
    }
    ind = msg - 1;
    if (!MSG_HEADING(board_type, ind)) {
	send_to_char("That message appears to be screwed up.\r\n", ch);
	return 1;
    }
    sprintf(buf, "(%s)", GET_NAME(ch));
    if (GET_LEVEL(ch) < REMOVE_LVL(board_type) &&
	!(strstr(MSG_HEADING(board_type, ind), buf))) {
	send_to_char("You are not holy enough to remove other people's messages.\r\n", ch);
	return 1;
    }
    if (GET_LEVEL(ch) < LVL_LUCIFER &&
	!(strstr(MSG_HEADING(board_type, ind), buf)) &&
	GET_LEVEL(ch) < MSG_LEVEL(board_type, ind)) {
	send_to_char("You can't remove a message holier than yourself.\r\n", ch);
	return 1;
    }
    slot_num = MSG_SLOTNUM(board_type, ind);
    if (slot_num < 0 || slot_num >= INDEX_SIZE) {
	slog("SYSERR: The board is seriously screwed up.");
	send_to_char("That message is majorly screwed up.\r\n", ch);
	return 1;
    }
    for (d = descriptor_list; d; d = d->next)
	if (!d->connected && d->str == &(msg_storage[slot_num])) {
	    send_to_char("At least wait until the author is finished before removing it!\r\n", ch);
	    return 1;
	}
    if (msg_storage[slot_num]) {
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(msg_storage[slot_num]);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
    }
    msg_storage[slot_num] = 0;
    msg_storage_taken[slot_num] = 0;
    if (MSG_HEADING(board_type, ind)) {
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(MSG_HEADING(board_type, ind));
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
    }

    for (; ind < num_of_msgs[board_type] - 1; ind++) {
	MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
	MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
	MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
    }
    num_of_msgs[board_type]--;
    send_to_char("Message removed.\r\n", ch);
    sprintf(buf, "$n just removed message %d.", msg);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    Board_save_board(board_type);

    return 1;
}


void 
Board_save_board(int board_type)
{
    FILE *fl;
    int i;
    char *tmp1 = 0, *tmp2 = 0;

    if (!num_of_msgs[board_type]) {
	unlink(FILENAME(board_type));
	return;
    }
    if (!(fl = fopen(FILENAME(board_type), "wb"))) {
	perror("Error writing board");
	return;
    }
    fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

    for (i = 0; i < num_of_msgs[board_type]; i++) {
	if ((tmp1 = MSG_HEADING(board_type, i)))
	    msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
	else
	    msg_index[board_type][i].heading_len = 0;

	if (MSG_SLOTNUM(board_type, i) < 0 ||
	    MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
	    (!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
	    msg_index[board_type][i].message_len = 0;
	else
	    msg_index[board_type][i].message_len = strlen(tmp2) + 1;

	fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
	if (tmp1)
	    fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
	if (tmp2)
	    fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
    }

    fclose(fl);
}


void 
Board_load_board(int board_type)
{
    FILE *fl;
    int i, len1 = 0, len2 = 0;
    char *tmp1 = NULL, *tmp2 = NULL;


    if (!(fl = fopen(FILENAME(board_type), "rb"))) {
	if (!mini_mud) {
	    sprintf(buf, "Error reading board '%s'", FILENAME(board_type));
	    perror(buf);
	}
	return;
    }
    fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
    if (num_of_msgs[board_type] < 1 || num_of_msgs[board_type] > MAX_BOARD_MESSAGES) {
	slog("SYSERR: Board file corrupt.  Resetting.");
	Board_reset_board(board_type);
	return;
    }
    for (i = 0; i < num_of_msgs[board_type]; i++) {
	fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
	if (!(len1 = msg_index[board_type][i].heading_len)) {
	    slog("SYSERR: Board file corrupt!  Resetting.");
	    Board_reset_board(board_type);
	    return;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	if (!(tmp1 = (char *) malloc(sizeof(char) * len1))) {
	    slog("SYSERR: Error - malloc failed for board header");
	    safe_exit(1);
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	fread(tmp1, sizeof(char), len1, fl);
	MSG_HEADING(board_type, i) = tmp1;

	if ((len2 = msg_index[board_type][i].message_len)) {
	    if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1) {
		slog("SYSERR: Out of slots booting board!  Resetting...");
		Board_reset_board(board_type);
		return;
	    }
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    if (!(tmp2 = (char *) malloc(sizeof(char) * len2))) {
		slog("SYSERR: malloc failed for board text");
		safe_exit(1);
	    }
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    fread(tmp2, sizeof(char), len2, fl);
	    msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
	}
    }

    fclose(fl);
}


void 
Board_reset_board(int board_type)
{
    int i;

    for (i = 0; i < MAX_BOARD_MESSAGES; i++) {
	if (MSG_HEADING(board_type, i)) {
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free(MSG_HEADING(board_type, i));
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	}
	if (msg_storage[MSG_SLOTNUM(board_type, i)]) {
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	    free(msg_storage[MSG_SLOTNUM(board_type, i)]);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	}
	msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
	memset((char *)&(msg_index[board_type][i]),0,sizeof(struct board_msginfo));
	msg_index[board_type][i].slot_num = -1;
    }
    num_of_msgs[board_type] = 0;
    unlink(FILENAME(board_type));
}
