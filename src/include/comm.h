#ifndef _COMM_H_
#define _COMM_H_

/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: comm.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define NUM_RESERVED_DESCS	8

struct creature;
struct room_data;
struct zone_data;
struct obj_data;
struct descriptor_data;
struct txt_q;

/* comm.c */
void send_to_all(const char *messg);
void send_to_char(struct creature *ch, const char *str, ...)
	__attribute__ ((format (printf, 2, 3)));
void send_to_desc(struct descriptor_data *d, const char *str, ...)
	__attribute__ ((format (printf, 2, 3)));
void send_to_room(const char *messg, struct room_data *room);
void send_to_clerics(int align, const char *messg);
void send_to_outdoor(const char *messg, int isecho);
void send_to_clan(const char *messg, int clan);
void send_to_zone(const char *messg, struct zone_data *zone, int outdoor);
void send_to_comm_channel(struct creature *ch, char *buff, int chan, int mode,
	int hide_invis);
void send_to_newbie_helpers(const char *messg);
void close_socket(struct descriptor_data *d);

// Act system
typedef bool (*act_if_predicate)(struct creature *ch, struct obj_data *obj, void *vict_obj, struct creature *to, int mode);
char *act_escape(const char *str);
void make_act_str(const char *orig, char *buf, struct creature *ch,
	struct obj_data *obj, void *vict_obj, struct creature *to);
void perform_act(const char *orig, struct creature *ch,
	struct obj_data *obj, void *vict_obj, struct creature *to, int mode);
void act_if(const char *str, int hide_invisible, struct creature *ch,
	struct obj_data *obj, void *vict_obj, int type, act_if_predicate pred);
void act(const char *str, int hide_invisible, struct creature *ch,
	struct obj_data *obj, void *vict_obj, int type);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define ACT_HIDECAR     8
#define TO_VICT_RM  64
#define TO_SLEEP	128			/* to char, even if sleeping */

#define SHUTDOWN_NONE   0
#define SHUTDOWN_DIE    1
#define SHUTDOWN_PAUSE  2
#define SHUTDOWN_REBOOT 3

struct account;

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

struct last_command_data {
	int idnum;
	char name[MAX_INPUT_LENGTH];
	int roomnum;
	char room[MAX_INPUT_LENGTH];
	char string[MAX_INPUT_LENGTH];
};

#define NUM_SAVE_CMDS 30

typedef void sigfunc(int);
void write_to_q(char *txt, struct txt_q *queue, int aliased);
void write_to_output(const char *txt, struct descriptor_data *d);
void page_string(struct descriptor_data *d, const char *str);
void show_file(struct creature *ch, const char *fname, int lines);
void show_account_chars(struct descriptor_data *d, struct account *acct, bool immort, bool brief);


extern bool suppress_output;
extern bool production_mode;

#endif
