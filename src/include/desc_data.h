#ifndef _DESC_DATA_H_
#define _DESC_DATA_H_

//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//


#include "defs.h"
#include "editor.h"

struct Creature;
class Account;

/* Modes of connectedness: used by descriptor_data.state */
enum cxn_state {
	CXN_PLAYING,				// Playing - Nominal state  
	CXN_DISCONNECT,				// Disconnecting        
	// Account states
	CXN_ACCOUNT_LOGIN,			// Initial login to account
	CXN_ACCOUNT_PW,				// Password to account
	CXN_ACCOUNT_PROMPT,			// Query for new account name
	CXN_ACCOUNT_VERIFY,			// Verify new account name
	CXN_PW_PROMPT,				// Enter new password
	CXN_PW_VERIFY,				// Verify new password
	CXN_ANSI_PROMPT,			// Do you have color?
	CXN_EMAIL_PROMPT,			// Optional email address
	CXN_OLDPW_PROMPT,			// Get old password for pw change
	CXN_NEWPW_PROMPT,			// Get new password for pw change
	CXN_NEWPW_VERIFY,			// Verify new password for pw change
	// Character creation
	CXN_NAME_PROMPT,			// Enter a name for new character
	CXN_NAME_VERIFY,			// Did I get that right, x? 
	CXN_TIME_PROMPT,			// What timeframe?
	CXN_SEX_PROMPT,				// Sex?             
	CXN_RACE_PAST,				// Race? (timeframe-based)
	CXN_RACE_FUTURE,			// Race? (timeframe-based)
	CXN_CLASS_PAST,				// Class? (timeframe-based)
	CXN_CLASS_FUTURE,			// Class? (timeframe-based)
	CXN_ALIGN_PROMPT,			// Align? (race/class may restrict)
	CXN_STATISTICS_ROLL,		// Statistics rolling
	// Other, miscellaneous states
	CXN_MENU,					// Your choice: (main menu) 
	CXN_WAIT_MENU,				// Press return to go back to the main menu
	CXN_CLASS_REMORT,			// Select your remort class
	CXN_DELETE_PROMPT,			// What character to delete?
	CXN_DELETE_PW,				// Enter password to delete
	CXN_DELETE_VERIFY,			// Are you sure you want to delete?
	CXN_AFTERLIFE,				// After death, before menu
	CXN_REMORT_AFTERLIFE,		// After death, before remort class
	CXN_VIEW_BG,				// View background
	CXN_STATS_PROMPT,			// View character statistics
	CXN_EDIT_PROMPT,			// Which character do you want to describe
	CXN_EDIT_DESC,				// Describe your char
	CXN_NETWORK,				// Cyborg interfaced to network
};

#define IS_PLAYING(desc)	((desc)->input_mode == CXN_PLAYING || \
							(desc)->input_mode == CXN_NETWORK)

/* descriptor-related structures ******************************************/


struct txt_block {
	char *text;
	int aliased;
	struct txt_block *next;
};


struct txt_q {
	struct txt_block *head;
	struct txt_block *tail;
};

struct mail_recipient_data {
	long recpt_idnum;			/* Idnum of char to recieve mail  */
	struct mail_recipient_data *next;	/*pointer to next in recpt list. */
};

struct descriptor_data {
	int descriptor;				/* file descriptor for socket       */
	char host[HOST_LENGTH + 1];	/* hostname             */
	cxn_state input_mode;					/* mode of 'connectedness'      */
	int wait;					/* wait for how many loops      */
	int desc_num;				/* unique num assigned to desc      */
	time_t login_time;			/* when the person connected        */
	time_t old_login_time;		// used for the dynamic text notification
	char *showstr_head;			/* for paging through texts     */
	char *showstr_point;		/*      -           */
	byte bad_pws;				/* number of bad pw attemps this login  */
	byte need_prompt;			/* control of prompt-printing       */
	int max_str;				/*      -           */
	int repeat_cmd_count;		/* how many times has this command been */
	// We know if text_editor != NULL
	//int  editor_mode;            /* Flag if char is in editor            */
	int editor_cur_lnum;		/* Current line number for editor       */
	char *editor_file;			/* Original line number for editor      */
	CTextEditor *text_editor;	/*  Pointer to text editor object. */
	char inbuf[MAX_RAW_INPUT_LENGTH];	/* buffer for raw input       */
	char last_input[MAX_INPUT_LENGTH];	/* the last input         */
	char small_outbuf[SMALL_BUFSIZE];	/* standard output buffer     */
	char last_argument[MAX_INPUT_LENGTH];	/* */
	char output_broken;
	char *output;				/* ptr to the current output buffer */
	int bufptr;					/* ptr to end of current output     */
	int bufspace;				/* space left in the output buffer  */
	int last_cmd;
	int idle;					// how long idle for
	pthread_t resolver_thread;	// thread to resolve hostname
	struct txt_block *large_outbuf;	/* ptr to large buffer, if we need it */
	struct txt_q input;			/* q of unprocessed input       */
	Account *account;
	struct Creature *creature;	/* linked to char           */
	struct Creature *original;	/* original char if switched        */
	struct descriptor_data *snooping;	/* Who is this char snooping   */
	struct descriptor_data *snoop_by;	/* And who is snooping this char   */
	struct descriptor_data *next;	/* link to next descriptor     */
	struct mail_recipient_data *mail_to;	/* list of names for mailsystem */
};

void set_desc_state(cxn_state state, descriptor_data *d);

extern struct descriptor_data *descriptor_list;

#endif
