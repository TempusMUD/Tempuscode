/* ***********************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: structs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __structs_h__
#define __structs_h__


/* preamble *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#include "defs.h"
#include "macros.h"
#include "constants.h"
#include "obj_data.h"
#include "character_list.h"
#include "creature.h"
#include "desc_data.h"
#include "room_data.h"
#include "search.h"
#include "weather.h"
#include "zone_data.h"
#include "ban.h"

/***********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef struct Link {
	int type;
	int flags;
	void *object;
	struct Link *prev;
	struct Link *next;
} Link;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
	char *keyword;				/* Keyword in look/examine          */
	char *description;			/* What to see                      */
	struct extra_descr_data *next;	/* Next in list                      */
};


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
	char hours, day, month;
	short year;
};


/* ====================================================================== */

/* other miscellaneous structures ***************************************/


struct msg_type {
	char *attacker_msg;			/* message to attacker */
	char *victim_msg;			/* message to victim   */
	char *room_msg;				/* message to room     */
};


struct message_type {
	struct msg_type die_msg;	/* messages when death                        */
	struct msg_type miss_msg;	/* messages when miss                        */
	struct msg_type hit_msg;	/* messages when hit                        */
	struct msg_type god_msg;	/* messages when hit on god                */
	struct message_type *next;	/* to next messages of this kind.        */
};


struct message_list {
	int a_type;					/* Attack type                                */
	int number_of_attacks;		/* How many attack messages to chose from. */
	struct message_type *msg;	/* List of messages.                        */
};

#endif
