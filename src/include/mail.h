/* ************************************************************************
*   File: mail.h                                        Part of CircleMUD *
*  Usage: header file for mail system                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: mail.h                      -- Part of TempusMUD
// Author: John Rothe
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

// The vnum of the "letter" object
#define MAIL_OBJ_VNUM 1204

// minimum level a player must be to send mail
#define MIN_MAIL_LEVEL 1

// # of gold coins required to send mail
#define STAMP_PRICE 55

// Maximum size of mail in bytes (arbitrary)
#define MAX_MAIL_SIZE 4096

// How long to let mail sit in the file before purging it out.
// (Not actually used atm)
#define MAX_MAIL_AGE 15552000 // should be 6 months.

// Prototypes for postmaster specs
void postmaster_send_mail(struct char_data * ch, struct char_data *mailman,
              int cmd, char *arg);
void postmaster_check_mail(struct char_data * ch, struct char_data *mailman,
               int cmd, char *arg);
void postmaster_receive_mail(struct char_data * ch, struct char_data *mailman,
                 int cmd, char *arg);
// Redundant redeclarations for utility functions
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
void string_add(struct descriptor_data *d, char *str);

// Mail system internal functions.
int	has_mail(long recipient);
int has_mail(char_data *ch);
int store_mail(long to_id,long from_id,char *txt, time_t *cur_time = NULL);
int recieve_mail(char_data *ch);


// The actual mail file entry struct.
struct mail_data {
    long to;
    long from;
    time_t time;
	long spare;
    long msg_size;
};
