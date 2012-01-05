#ifndef _QUEST_H_
#define _QUEST_H_

//
// File: quest.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
#define QUEST_DIR "quest"
#define QLOGFILENAME "log/quest.log"

#define QUEST_REVIEWED  (1 << 0)	// quest has been reviewed
#define QUEST_NOWHO     (1 << 1)	// quest who does not work
#define QUEST_NO_OUTWHO (1 << 2)	// only paricipants may quest who
#define QUEST_NOJOIN    (1 << 3)	// quest is 'invite only'
#define QUEST_NOLEAVE   (1 << 4)	// no-one here gets out alive
#define QUEST_HIDE      (1 << 5)	// make quest invisible to mortals
#define QUEST_WHOWHERE  (1 << 6)	// quest who shows where players are
#define QUEST_ARENA     (1 << 7)	// questors only have arena deaths

#define QP_IGNORE       (1 << 0)	// ignore quest comm
#define QP_MUTE         (1 << 1)	// cannot send to quest

#define QP_FLAGGED(qp, bits) (qp->isFlagged(bits))

#define QCOMM_SAY      0
#define QCOMM_ECHO     1

#define QLOG_OFF    0
#define QLOG_BRIEF  1
#define QLOG_NORM   2
#define QLOG_COMP   3

#define QUEST_FLAGGED(quest, bits)  (IS_SET(quest->flags, bits))

#define MAX_QUEST_NAME   31
#define MAX_QUEST_DESC   32768
#define MAX_QUEST_UPDATE 32768
// Qcontrol OLOAD ranges
#define MAX_QUEST_OBJ_VNUM 92299
#define MIN_QUEST_OBJ_VNUM 92270

/**
 * An entry representing a player in a quest and the flags set on him/her.
**/
struct qplayer_data {
		long idnum;
		int flags;
        int deaths;
        int mobkills;
        int pkills;
};

extern int top_quest_vnum;

struct quest {
		int max_players; // max number of players
		int awarded; // qps awarded
		int penalized; // qps taken
		int vnum;
		long owner_id;
		time_t started;
		time_t ended;
		GList *players;
		GList *bans;
		int flags;
		char *name;
		char *description;
		char *updates;
		int owner_level;
		int minlevel;
		int maxlevel;
		int mingen;
		int maxgen;
        int loadroom;
		uint8_t type;
};

// utility functions
struct quest *find_quest(struct creature *ch, char *argument);
const char *list_active_quests(struct creature *ch);
struct quest *quest_by_vnum(int vnum);
void qp_reload(int sig);

void qlog(struct creature *ch, const char *str, int type, int level, int file);

struct creature *check_char_vis(struct creature *ch, char *name);
void list_quest_players(struct creature *ch, struct quest *quest, char *outbuf);
int boot_quests(void);
void save_quests();

// quest subfunctions and utils
void do_quest_list(struct creature *ch);
void do_quest_join(struct creature *ch, char *argument);
void do_quest_info(struct creature *ch, char *argument);
void do_quest_status(struct creature *ch);
void do_quest_who(struct creature *ch, char *argument);
void do_quest_leave(struct creature *ch, char *argument);
void do_quest_current(struct creature *ch, char *argument);
void do_quest_ignore(struct creature *ch, char *argument);

bool is_playing_quest(struct quest *quest, int id);
bool remove_quest_player(struct quest *quest, int id);
void tally_quest_death(struct quest *quest, int idnum);
void tally_quest_mobkill(struct quest *quest, int idnum);
void tally_quest_pkill(struct quest *quest, int idnum);

#endif
