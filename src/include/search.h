//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __search_h__
#define __search_h__


/* Room Search Commands */
#define SEARCH_COM_NONE      0
#define SEARCH_COM_DOOR      1
#define SEARCH_COM_MOBILE    2
#define SEARCH_COM_OBJECT    3
#define SEARCH_COM_REMOVE    4
#define SEARCH_COM_GIVE      5
#define SEARCH_COM_EQUIP     6
#define SEARCH_COM_TRANSPORT 7
#define SEARCH_COM_SPELL     8
#define SEARCH_COM_DAMAGE    9
#define SEARCH_COM_SPAWN     10
#define SEARCH_COM_LOADROOM  11
#define NUM_SEARCH_COM       12

	 /** room search flags **/
#define SRCH_REPEATABLE         (1 << 0)
#define SRCH_TRIPPED            (1 << 1)
#define SRCH_IGNORE             (1 << 2)
#define SRCH_CLANPASSWD         (1 << 3)
#define SRCH_TRIG_ENTER         (1 << 4)
#define SRCH_TRIG_FALL          (1 << 5)	// triggers when a player falls into the room (e.g. a spike pit)
#define SRCH_NOTRIG_FLY         (1 << 6)
#define SRCH_NOMOB              (1 << 7)
#define SRCH_NEWBIE_ONLY        (1 << 8)
#define SRCH_NOMESSAGE          (1 << 9)
#define SRCH_NOEVIL             (1 << 10)
#define SRCH_NONEUTRAL          (1 << 11)
#define SRCH_NOGOOD             (1 << 12)
#define SRCH_NOMAGE             (1 << 13)
#define SRCH_NOCLERIC           (1 << 14)
#define SRCH_NOTHIEF            (1 << 15)
#define SRCH_NOBARB             (1 << 16)
#define SRCH_NORANGER           (1 << 17)
#define SRCH_NOKNIGHT           (1 << 18)
#define SRCH_NOMONK             (1 << 19)
#define SRCH_NOPSIONIC          (1 << 20)
#define SRCH_NOPHYSIC           (1 << 21)
#define SRCH_NOMERC             (1 << 22)
#define SRCH_NOHOOD             (1 << 23)
#define SRCH_NOABBREV           (1 << 24)
#define SRCH_NOAFFMOB           (1 << 25)
#define SRCH_NOPLAYER           (1 << 26)
#define SRCH_REMORT_ONLY        (1 << 27)


#define NUM_SRCH_BITS           28

struct special_search_data {
	char *command_keys;			/* which command activates          */
	char *keywords;				/* Keywords which activate the command */
	char *to_vict;
	char *to_room;
	char *to_remote;
	byte command;
	int flags;
	int arg[3];
	struct special_search_data *next;
};

#endif
