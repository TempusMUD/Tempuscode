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

#define NUM_RESERVED_DESCS  8

struct creature;
struct room_data;
struct zone_data;
struct obj_data;
struct descriptor_data;
struct txt_q;

/* comm.c */
void send_to_all(const char *messg)
__attribute__ ((nonnull));
void send_to_char(struct creature *ch, const char *str, ...)
__attribute__ ((format (printf, 2, 3)))
__attribute__ ((nonnull (1,2)));
void send_to_room(const char *messg, struct room_data *room)
__attribute__ ((nonnull));
void send_to_clerics(int align, const char *messg)
__attribute__ ((nonnull));
void send_to_outdoor(const char *messg, int isecho)
__attribute__ ((nonnull));
void send_to_clan(const char *messg, int clan)
__attribute__ ((nonnull));
void send_to_zone(const char *messg, struct zone_data *zone, int outdoor)
__attribute__ ((nonnull));
void send_to_comm_channel(struct creature *ch, char *buff, int chan, int mode,
                          int hide_invis)
__attribute__ ((nonnull));
void send_to_newbie_helpers(const char *messg)
__attribute__ ((nonnull));
void close_socket(struct descriptor_data *d)
__attribute__ ((nonnull));

// Act system
typedef bool (*act_if_predicate)(struct creature *ch, struct obj_data *obj, void *vict_obj, struct creature *to, int mode)
__attribute__ ((nonnull (4)));
char *act_escape(const char *str)
__attribute__ ((nonnull));
void make_act_str(const char *orig, char *buf, struct creature *ch,
                  struct obj_data *obj, void *vict_obj, struct creature *to)
__attribute__ ((nonnull (1,2,6)));
void perform_act(const char *orig, struct creature *ch,
                 struct obj_data *obj, void *vict_obj, struct creature *to, int mode)
__attribute__ ((nonnull (1,5)));
void act_if(const char *str, bool hide_invisible, struct creature *ch,
            struct obj_data *obj, void *vict_obj, int type, act_if_predicate pred);
void act(const char *str, bool hide_invisible, struct creature *ch,
         /*@null@*/ struct obj_data *obj, /*@null@*/ void *vict_obj, int type);

#define TO_ROOM     1
#define TO_VICT     2
#define TO_NOTVICT  3
#define TO_CHAR     4
#define ACT_HIDECAR     8
#define TO_VICT_RM  64
#define TO_SLEEP    128         /* to char, even if sleeping */

#define SHUTDOWN_NONE   0
#define SHUTDOWN_DIE    1
#define SHUTDOWN_PAUSE  2
#define SHUTDOWN_REBOOT 3

struct account;

#define USING_SMALL(d)  ((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

// printf to descriptor with color code expansion
void d_send(struct descriptor_data *d, const char *txt);
void d_printf(struct descriptor_data *d, const char *str, ...)
__attribute__ ((format (printf, 2, 3)));

struct last_command_data {
    int idnum;
    char name[MAX_INPUT_LENGTH];
    int roomnum;
    char room[MAX_INPUT_LENGTH];
    char string[MAX_INPUT_LENGTH];
};

#define NUM_SAVE_CMDS 30

extern struct last_command_data last_cmd[NUM_SAVE_CMDS];

typedef void sigfunc (int);
void write_to_q(char *txt, struct txt_q *queue, int aliased)
__attribute__ ((nonnull));
void write_to_output(const char *txt, struct descriptor_data *d)
__attribute__ ((nonnull));
void page_string(struct descriptor_data *d, const char *str);
void show_file(struct creature *ch, const char *fname, int lines)
__attribute__ ((nonnull));
void show_account_chars(struct descriptor_data *d, struct account *acct, bool immort, bool brief)
__attribute__ ((nonnull));


extern bool suppress_output;
extern bool production_mode;

#endif
