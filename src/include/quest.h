//
// File: quest.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define QUEST_DIR "quest"
#define QLOGFILENAME "quest/qlog"

#define QUEST_REVIEWED  (1 << 0)   // quest has been reviewed
#define QUEST_NOWHO     (1 << 1)   // quest who does not work
#define QUEST_NO_OUTWHO (1 << 2)   // only paricipants may quest who
#define QUEST_NOJOIN    (1 << 3)   // quest is 'invite only'
#define QUEST_NOLEAVE   (1 << 4)   // no-one here gets out alive
#define QUEST_HIDE      (1 << 5)   // make quest invisible to mortals
#define QUEST_WHOWHERE  (1 << 6)   // quest who shows where players are

#define QP_IGNORE       (1 << 0)   // ignore quest comm
#define QP_MUTE         (1 << 1)   // cannot send to quest

#define QP_FLAGGED(qp, bits) (IS_SET(qp->flags, bits))

#define QCOMM_SAY      0
#define QCOMM_ECHO     1

#define QLOG_OFF    0
#define QLOG_BRIEF  1
#define QLOG_NORM   2
#define QLOG_COMP   3

#define QUEST_FLAGGED(quest, bits)  (IS_SET(quest->flags, bits))

#define MAX_QUEST_NAME   31
#define MAX_QUEST_DESC   1024
#define MAX_QUEST_UPDATE 2048

#define QTYPE_TRIVIA        1
#define QTYPE_SCAVENGER     2
#define QTYPE_HIDE_AND_SEEK 3
#define QTYPE_ROLEPLAY      4

typedef struct qplayer_data {
    unsigned int idnum;
    int          flags;
} qplayer_data;

typedef struct quest_data {
    int     vnum;
    int     owner_id;
    int     flags;
    time_t  started;
    time_t  ended;
    char   *name;
    char   *description;
    char   *updates;
    qplayer_data *players;
    qplayer_data *bans;
    ubyte   num_bans;
    //  unsigned int *players;
    ubyte   owner_level;
    short   minlev;
    short   maxlev;
    ubyte   type;
    ubyte   num_players;
    ubyte   max_players;
    ubyte   awarded;
    ubyte   penalized;
    ubyte   kicked;
    ubyte   banned;
    ubyte   loaded;
    ubyte   purged;
    struct quest_data *next;
} quest_data;

typedef struct quest_file_u {
    int     owner_id;
    time_t  started;
    time_t  ended;
    int     flags;
    char    name[MAX_QUEST_NAME+1];
    char    description[MAX_QUEST_DESC+1];
    ubyte   type;
    ubyte   num_players;
    ubyte   max_players;
    ubyte   pts_awarded;
    ubyte   pts_penalized;
    ubyte   players_kicked;
    ubyte   players_banned;
    ubyte   mobs_loaded;
    ubyte   mobs_purged;
    char    spares[20];
} quest_file_u;  

// qcontrol subfunctions  
void do_qcontrol_show    (CHAR *ch, char *argument);
void do_qcontrol_options (CHAR *ch);
void do_qcontrol_create  (CHAR *ch, char *argument, int com);
void do_qcontrol_end     (CHAR *ch, char *argument, int com);
void do_qcontrol_add     (CHAR *ch, char *argument, int com);
void do_qcontrol_kick    (CHAR *ch, char *argument, int com);
void do_qcontrol_flags   (CHAR *ch, char *argument, int com);
void do_qcontrol_comment (CHAR *ch, char *argument, int com);
void do_qcontrol_desc    (CHAR *ch, char *argument, int com);
void do_qcontrol_update  (CHAR *ch, char *argument, int com);
void do_qcontrol_ban     (CHAR *ch, char *argument, int com);
void do_qcontrol_unban   (CHAR *ch, char *argument, int com);
void do_qcontrol_level   (CHAR *ch, char *argument, int com);
void do_qcontrol_minlev  (CHAR *ch, char *argument, int com);
void do_qcontrol_maxlev  (CHAR *ch, char *argument, int com);
void do_qcontrol_mute    (CHAR *ch, char *argument, int com);
void do_qcontrol_unmute  (CHAR *ch, char *argument, int com);
void do_qcontrol_award   (CHAR *ch, char *argument, int com);
void do_qcontrol_penalize (CHAR *ch, char *argument, int com);
void do_qcontrol_save    (CHAR *ch, char *argument, int com);
void do_qcontrol_load    (CHAR *ch, char *argument, int com);//Load mobile.
void do_qcontrol_purge   (CHAR *ch, char *argument, int com);//Purge mobile.

// utility functions
void do_qcontrol_usage(CHAR *ch, int com);
quest_data *create_quest (CHAR *ch, int type, char *name);
quest_data *find_quest   (CHAR *ch, char *argument);
void list_active_quests  (CHAR *ch, char *outbuf);
void list_inactive_quests(CHAR *ch, char *outbuf);
quest_data *quest_by_vnum(int vnum);
void qp_reload( int sig = 0 );

void qlog(CHAR *ch, char *str, int type, int level, int file);

CHAR *check_char_vis     (CHAR *ch, char *name);
int add_idnum_to_quest   (unsigned int idnum, quest_data *quest);
int remove_idnum_from_quest(unsigned int idnum, quest_data *quest);
void list_quest_players  (CHAR *ch, quest_data *quest, char *outbuf);
void list_quest_bans     (CHAR *ch, quest_data *quest, char *outbuf);
qplayer_data *idnum_in_quest       (unsigned int idnum, quest_data *quest);
int boot_quests          (void);
int check_editors        (CHAR *ch, char **buffer);
qplayer_data *idnum_banned_from_quest(unsigned int idnum, quest_data *quest);
int ban_idnum_from_quest (unsigned int idnum, quest_data *quest);
int unban_idnum_from_quest(unsigned int idnum, quest_data *quest);
int quest_edit_ok        (CHAR *ch, quest_data *quest);

// quest subfunctions and utils
void do_quest_list       (CHAR *ch);
void do_quest_join       (CHAR *ch, char *argument);
void do_quest_info       (CHAR *ch, char *argument);
void do_quest_status     (CHAR *ch, char *argument);
void do_quest_who        (CHAR *ch, char *argument);
void do_quest_leave      (CHAR *ch, char *argument);
void do_quest_current    (CHAR *ch, char *argument);
void do_quest_ignore     (CHAR *ch, char *argument);

int quest_leave_ok(CHAR *ch, quest_data *quest);
int quest_join_ok(CHAR *ch, quest_data *quest);
int quest_level_ok(CHAR *ch, quest_data *quest);

// external functions
int get_line_count(char *buffer);
void send_to_quest(CHAR *ch, char *buf, quest_data *quest,int level,int mode);
void compose_qcomm_string(CHAR *ch, CHAR *vict, quest_data *quest, int mode, char *str, char *outbuf);

// external FILES
extern FILE *player_fl;
