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
enum
	{
	CON_PLAYING,		// Playing - Nominal state	
	CON_CLOSE,			// Disconnecting		
	CON_GET_NAME,		// By what name ..?		
	CON_NAME_CNFRM,		// Did I get that right, x?	
	CON_PASSWORD,		// Password:			
	CON_NEWPASSWD,		// Give me a password for x	
	CON_CNFPASSWD,		// Please retype password:	
	CON_QSEX,			// Sex?				
	CON_QCLASS_PAST,	// Class past?			
	CON_QCLASS_FUTURE,	// Class future?			
	CON_RMOTD,			// PRESS RETURN after MOTD	
	CON_MENU,			// Your choice: (main menu)	
	CON_EXDESC,			// Enter a new description:	
	CON_CHPWD_GETOLD,	// Changing passwd: get old	
	CON_CHPWD_GETNEW,	// Changing passwd: get new	
	CON_CHPWD_VRFY,		// Verify new password		
	CON_DELCNF1,		// Delete confirmation 1	
	CON_DELCNF2,		// Delete confirmation 2	
	CON_RACE_PAST,		// Racial Query			
	CON_QALIGN,			// Alignment Query		
	CON_QCOLOR,			// Start with color?
	CON_QTIME_FRAME,	// Query for overall time frame
	CON_AFTERLIFE,		// After dies, before menu
	CON_QREROLL,		// Reroll statistics
	CON_RACEHELP_P,	
	CON_CLASSHELP_P,	
	CON_HOMEHELP_P,	
	CON_HOMEHELP_F,	
	CON_RACE_FUTURE,	
	CON_RACEHELP_F,	
	CON_CLASSHELP_F,	
	CON_REMORT_REROLL,	
	CON_NETWORK,		// interfaced to network
	CON_PORT_OLC,		// Using port olc interface
	};

#define IS_PLAYING(desc)	((desc)->connected == CON_PLAYING || \
							(desc)->connected == CON_NETWORK)

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
   byte need_prompt;		/* control of prompt-printing		*/
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

#endif 
