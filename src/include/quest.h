//
// File: quest.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
#ifndef _QUEST_H
#define _QUEST_H

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
class qplayer_data {
	public:
		qplayer_data( long id ) { idnum = id; flags = 0; }
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
	public:
		long idnum;
	private:
		int flags;
};

class Quest {
	public:
		Quest( char_data *ch, int type, const char* name );
		Quest( const Quest &q );
		Quest(xmlNodePtr n, xmlDocPtr doc);
		~Quest();
	public: // operators
		bool operator==( const Quest &q ) const { return vnum == q.vnum; }
		bool operator!=( const Quest &q ) const { return vnum != q.vnum; }
		bool operator<( const Quest &q ) const { return vnum < q.vnum; }
		bool operator>( const Quest &q ) const { return vnum > q.vnum; }
		Quest& operator=( const Quest &q );
	public: // utils
		bool canEdit( char_data *ch );
		bool addPlayer( long id );
		bool removePlayer( long id );
		bool addBan( long id );
		bool removeBan( long id );
		bool isBanned( long id );
		bool isPlaying( long id );
		bool canLeave( CHAR *ch );
		bool canJoin( CHAR *ch );
		qplayer_data &getPlayer( long id );
		qplayer_data &getPlayer( int index ) { return players[index]; }
		qplayer_data &getBan( long id );
		void save( ostream &out );
	public: // accessors
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

	private: // utils
		void clearDescs();
		bool levelOK( CHAR *ch );
	private: // data
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
	public: // data
		int flags;
		char *name;
		char *description;
		char *updates;
		int owner_level;
		int minlevel;
		int maxlevel;
		int mingen;
		int maxgen;
		ubyte type;
};

// qcontrol subfunctions  
void do_qcontrol_show(CHAR * ch, char *argument);
void do_qcontrol_options(CHAR * ch);
void do_qcontrol_create(CHAR * ch, char *argument, int com);
void do_qcontrol_end(CHAR * ch, char *argument, int com);
void do_qcontrol_add(CHAR * ch, char *argument, int com);
void do_qcontrol_kick(CHAR * ch, char *argument, int com);
void do_qcontrol_flags(CHAR * ch, char *argument, int com);
void do_qcontrol_comment(CHAR * ch, char *argument, int com);
void do_qcontrol_desc(CHAR * ch, char *argument, int com);
void do_qcontrol_update(CHAR * ch, char *argument, int com);
void do_qcontrol_ban(CHAR * ch, char *argument, int com);
void do_qcontrol_unban(CHAR * ch, char *argument, int com);
void do_qcontrol_level(CHAR * ch, char *argument, int com);
void do_qcontrol_minlev(CHAR * ch, char *argument, int com);
void do_qcontrol_maxlev(CHAR * ch, char *argument, int com);
void do_qcontrol_mingen(CHAR * ch, char *argument, int com);
void do_qcontrol_maxgen(CHAR * ch, char *argument, int com);
void do_qcontrol_mute(CHAR * ch, char *argument, int com);
void do_qcontrol_unmute(CHAR * ch, char *argument, int com);
void do_qcontrol_award(CHAR * ch, char *argument, int com);
void do_qcontrol_penalize(CHAR * ch, char *argument, int com);
void do_qcontrol_save(CHAR * ch, char *argument, int com);
void do_qcontrol_mload(CHAR * ch, char *argument, int com);	//Load mobile.
void do_qcontrol_purge(CHAR * ch, char *argument, int com);	//Purge mobile.
void do_qcontrol_trans(CHAR * ch, char *argument, int com);	//trans whole quest

// utility functions
void do_qcontrol_usage(CHAR * ch, int com);
Quest *find_quest(CHAR * ch, char *argument);
char *list_active_quests(CHAR * ch, char *outbuf);
char *list_inactive_quests(CHAR * ch, char *outbuf);
Quest *quest_by_vnum(int vnum);
void qp_reload(int sig = 0);

void qlog(CHAR * ch, char *str, int type, int level, int file);

CHAR *check_char_vis(CHAR * ch, char *name);
void list_quest_players(CHAR * ch, Quest * quest, char *outbuf);
void list_quest_bans(CHAR * ch, Quest * quest, char *outbuf);
int boot_quests(void);
int check_editors(CHAR * ch, char **buffer);
void save_quests();

// quest subfunctions and utils
void do_quest_list(CHAR * ch);
void do_quest_join(CHAR * ch, char *argument);
void do_quest_info(CHAR * ch, char *argument);
void do_quest_status(CHAR * ch, char *argument);
void do_quest_who(CHAR * ch, char *argument);
void do_quest_leave(CHAR * ch, char *argument);
void do_quest_current(CHAR * ch, char *argument);
void do_quest_ignore(CHAR * ch, char *argument);

// external FILES
extern FILE *player_fl;
#endif
