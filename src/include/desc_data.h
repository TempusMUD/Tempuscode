//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//


#ifndef __desc_data_h__
#define __desc_data_h__

#include "structs.h"
#include "defs.h"
#include "editor.h"

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_QHOME        17             /* Hometown Query		*/
#define CON_RACE_PAST	 18		/* Racial Query			*/
#define CON_QALIGN       19		/* Alignment Query		*/
#define CON_QCOLOR       20		/* Start with color?            */
#define CON_QTIME_FRAME  21             /* Query for overall time frame */
#define CON_AFTERLIFE    22             /* After dies, before menu      */
#define CON_QHOME_PAST   23
#define CON_QHOME_FUTURE 24
#define CON_QREROLL      25             
#define CON_RACEHELP_P   26
#define CON_CLASSHELP_P  27
#define CON_HOMEHELP_P   28
#define CON_HOMEHELP_F   29
#define CON_RACE_FUTURE  30
#define CON_RACEHELP_F   31
#define CON_CLASSHELP_F  32
#define CON_REMORT_REROLL 33
#define CON_NET_MENU1    50             /* First net menu state         */
#define CON_NET_PROGMENU1 51            /* State which char_class of skill   */
#define CON_NET_PROG_CYB  52            /* State which char_class of skill   */
#define CON_NET_PROG_MNK  53            /* State which char_class of skill   */
#define CON_NET_PROG_HOOD 54
#define CON_PORT_OLC      55            /* Using port olc interface     */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};

struct mail_recipient_data {
  long recpt_idnum;                /* Idnum of char to recieve mail  */
  struct mail_recipient_data *next; /*pointer to next in recpt list. */
};

struct descriptor_data {
   int	descriptor;		/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   int	connected;		/* mode of 'connectedness'		*/
   int	wait;			/* wait for how many loops		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   time_t old_login_time;       // used for the dynamic text notification
   char	*showstr_head;		/* for paging through texts		*/
   char	*showstr_point;		/*		-			*/
   char	**str;			/* for the modify-str system		*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte	prompt_mode;		/* control of prompt-printing		*/
   int	max_str;		/*		-			*/
   int  repeat_cmd_count;       /* how many times has this command been */
   // We know if text_editor != NULL
   //int  editor_mode;            /* Flag if char is in editor            */
   int  editor_cur_lnum;        /* Current line number for editor       */
   char *editor_file;           /* Original line number for editor      */
   CTextEditor *text_editor;    /*  Pointer to text editor object. */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char last_argument[MAX_INPUT_LENGTH]; /* */
   char output_broken;
   char *output;		/* ptr to the current output buffer	*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   int  last_cmd;
   int idle;			// how long idle for
   pthread_t resolver_thread;	// thread to resolve hostname
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct mail_recipient_data *mail_to;	/* list of names for mailsystem	*/
};



#endif __desc_data_h__
