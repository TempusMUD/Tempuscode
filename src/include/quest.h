#ifndef _QUEST_H_
#define _QUEST_H_

//
// File: quest.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
#include <vector>
using namespace std;
#include <fstream>
#include "xml_utils.h"

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

#define QTYPE_TRIVIA        1
#define QTYPE_SCAVENGER     2
#define QTYPE_HIDE_AND_SEEK 3
#define QTYPE_ROLEPLAY      4

/**
 * An entry representing a player in a quest and the flags set on him/her.
**/
struct qplayer_data {
		qplayer_data( long id ) {
            idnum = id;
            flags = deaths = mobkills = pkills = 0;
        }
		qplayer_data( const qplayer_data &q) { *this = q; }
		qplayer_data& operator=( const qplayer_data &q );
		bool operator==( const qplayer_data &q ) const { return idnum == q.idnum; }
		bool operator!=( const qplayer_data &q ) const { return idnum != q.idnum; }
		bool operator<( const qplayer_data &q ) const { return idnum < q.idnum; }
		bool operator>( const qplayer_data &q ) const { return idnum > q.idnum; }
		operator long() { return idnum; }
		operator int() { return (int)idnum; }
		bool isFlagged( int flag );
		void setFlag( int flag );
		void removeFlag( int flag );
		void toggleFlag( int flag );
		int getFlags() { return flags; }

		long idnum;
		int flags;
        int deaths;
        int mobkills;
        int pkills;
};

struct Quest {
		Quest( struct creature *ch, int type, const char* name );
		Quest( const Quest &q );
		Quest(xmlNodePtr n, xmlDocPtr doc);
		~Quest();
		bool operator==( const Quest &q ) const { return vnum == q.vnum; }
		bool operator!=( const Quest &q ) const { return vnum != q.vnum; }
		bool operator<( const Quest &q ) const { return vnum < q.vnum; }
		bool operator>( const Quest &q ) const { return vnum > q.vnum; }
		Quest& operator=( const Quest &q );
		bool canEdit( struct creature *ch );
		bool addPlayer( long id );
		bool removePlayer( long id );
		bool addBan( long id );
		bool removeBan( long id );
		bool isBanned( long id );
		bool isPlaying( long id );
		bool canLeave( struct creature *ch );
		bool canJoin( struct creature *ch );
		qplayer_data &getPlayer( long id );
		qplayer_data &getPlayer( int index ) { return players[index]; }
		qplayer_data &getBan( long id );
		void save( ostream &out );
		void addPenalized( int penalty ) { penalized += penalty; }
		void addAwarded( int award ) { awarded += award; }
		int getNumPlayers() { return (int)players.size(); }
		int getNumBans() { return (int)bans.size(); }
		int getAwarded() { return awarded; }
		int getPenalized() { return penalized; }
		int getMaxPlayers() { return max_players; }
		void setMaxPlayers( int max ) { max_players = max; }
		int getVnum() { return vnum; }
		time_t getEnded() { return ended; }
		void setEnded( time_t end ) { ended = end; }
		time_t getStarted() { return started; }
		long getOwner() { return owner_id; }
        void tallyDeath(int player);
        void tallyMobKill(int player);
        void tallyPlayerKill(int player);
		void clearDescs();
		bool levelOK( struct creature *ch );
		int max_players; // max number of players
		int awarded; // qps awarded
		int penalized; // qps taken
		int vnum;
		long owner_id;
		time_t started;
		time_t ended;
		vector<qplayer_data> players;
		vector<qplayer_data> bans;
		static int top_vnum;
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
		ubyte type;
};

// utility functions
Quest *find_quest(struct creature *ch, char *argument);
const char *list_active_quests(struct creature *ch);
Quest *quest_by_vnum(int vnum);
void qp_reload(int sig = 0);

void qlog(struct creature *ch, const char *str, int type, int level, int file);

struct creature *check_char_vis(struct creature *ch, char *name);
void list_quest_players(struct creature *ch, Quest * quest, char *outbuf);
int boot_quests(void);
int check_editors(struct creature *ch, char **buffer);
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

#endif
