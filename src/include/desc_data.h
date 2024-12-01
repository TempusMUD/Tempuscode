#ifndef _DESC_DATA_H_
#define _DESC_DATA_H_

//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

struct creature;    // forward declaration from creature.h
struct account;      // forward declaration from account.h
struct editor;  // forward declaration from editor.h

// Modes of connectedness: used by descriptor_data.state
// make sure changes to this are synced with desc_modes[] in db/constants.cc
enum cxn_state {
    CXN_UNKNOWN = -1,
    CXN_PLAYING,                // Playing - Nominal state
    CXN_DISCONNECT,             // Disconnecting
    // struct account states
    CXN_ACCOUNT_LOGIN,          // Initial login to account
    CXN_ACCOUNT_PW,             // Password to account
    CXN_ACCOUNT_PROMPT,         // Query for new account name
    CXN_ACCOUNT_VERIFY,         // Verify new account name
    CXN_PW_PROMPT,              // Enter new password
    CXN_PW_VERIFY,              // Verify new password
    CXN_ANSI_PROMPT,            // Do you have color?
    CXN_COMPACT_PROMPT,         // What compactness do you prefer?
    CXN_EMAIL_PROMPT,           // Optional email address
    CXN_VIEW_POLICY,            // View policy before char creation
    CXN_OLDPW_PROMPT,           // Get old password for pw change
    CXN_NEWPW_PROMPT,           // Get new password for pw change
    CXN_NEWPW_VERIFY,           // Verify new password for pw change
    // Character creation
    CXN_NAME_PROMPT,            // Enter a name for new character
    CXN_SEX_PROMPT,             // Sex?
    CXN_HARDCORE_PROMPT,        // Hardcore?
    CXN_CLASS_PROMPT,           // Class?
    CXN_RACE_PROMPT,            // Race?
    CXN_ALIGN_PROMPT,           // Align? (race/class may restrict)
    CXN_STATISTICS_ROLL,        // Statistics rolling
    // Other, miscellaneous states
    CXN_MENU,                   // Your choice: (main menu)
    CXN_WAIT_MENU,              // Press return to go back to the main menu
    CXN_CLASS_REMORT,           // Select your remort class
    CXN_DELETE_PROMPT,          // What character to delete?
    CXN_DELETE_PW,              // Enter password to delete
    CXN_DELETE_VERIFY,          // Are you sure you want to delete?
    CXN_AFTERLIFE,              // After death, before menu
    CXN_REMORT_AFTERLIFE,       // After death, before remort class
    CXN_VIEW_BG,                // View background
    CXN_DETAILS_PROMPT,         // View character statistics
    CXN_EDIT_PROMPT,            // Which character do you want to describe
    CXN_EDIT_DESC,              // Describe your char
    CXN_NETWORK,                // Cyborg interfaced to network
    CXN_CLASS_HELP,             // Help on different classes
    CXN_RACE_HELP,              // Help on different races
    CXN_EMAIL_VERIFY,        // Verify password before changing e-mail
    CXN_NEWEMAIL_PROMPT,     // Enter new e-mail address
    CXN_RECOVER_EMAIL,       // Enter email for account recovery
    CXN_PROXY                // Awaiting proxy information
};

enum display_mode {
    NORMAL,                     // Standard interfaces
    BLIND,                      // Text interface optimized for screen readers
    IRC,                        // Text interface optimized for IRC
};

struct telnet_endpoint_option {
    bool enabled;
    bool requesting;
};

struct telnet_option {
    struct telnet_endpoint_option host[256];  // options negotiated for MUD-side
    struct telnet_endpoint_option peer[256];  // options negotiated for client-side
};

#define IS_PLAYING(desc)    ((desc)->input_mode == CXN_PLAYING || \
                             (desc)->input_mode == CXN_NETWORK)

/* descriptor-related structures ******************************************/

struct txt_q {
    struct txt_block *head;
    struct txt_block *tail;
};

struct descriptor_data {
    GIOChannel *io;             /* file descriptor for socket       */

    char host[HOST_LENGTH + 1]; /* hostname             */
    enum cxn_state input_mode;  /* mode of 'connectedness'      */
    struct telnet_option telnet; /* Telnet protocol options */
    void *mode_data;            // pointer for misc data needed for input_mode
    int wait;                   /* wait for how many loops      */
    int desc_num;               /* unique num assigned to desc      */
    time_t login_time;          /* when the person connected        */
    char *showstr_head;         /* for paging through texts     */
    char *showstr_point;        /*      -           */
    int8_t bad_pws;             /* number of bad pw attempts this login  */
    bool need_prompt;           /* control of prompt-printing       */
    bool eor_enabled;           /* Negotiated EOR at end of prompt */
    int max_str;                /*      -           */
    int repeat_cmd_count;       /* how many times has this command been */
    struct editor *text_editor; /*  Pointer to text editor object. */
    uint8_t inbuf[MAX_RAW_INPUT_LENGTH];   /* buffer for raw input       */
    size_t inbuf_len;
    GString *line;              /* buffer for current line of text */
    GString *last_input;        /* last line entered */
    GQueue *input;          /* q of unprocessed input       */
    guint in_watcher;
    guint out_watcher;
    guint err_watcher;
    guint input_handler;
    char last_argument[MAX_INPUT_LENGTH];   /* */
    int last_cmd;
    int idle;                   // how long idle for
    enum display_mode display;
    int ban_dc_counter;         // countdown to disconnection due to ban
    struct account *account;
    struct creature *creature;  /* linked to char           */
    struct creature *original;  /* original char if switched        */
    struct descriptor_data *snooping;   /* Who is this char snooping   */
    GList *snoop_by;
    struct descriptor_data *next;   /* link to next descriptor     */
};

void set_desc_state(enum cxn_state state, struct descriptor_data *d);

extern struct descriptor_data *descriptor_list;

#endif
