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
		Quest( Creature *ch, int type, const char* name );
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
		bool canEdit( Creature *ch );
		bool addPlayer( long id );
		bool removePlayer( long id );
		bool addBan( long id );
		bool removeBan( long id );
		bool isBanned( long id );
		bool isPlaying( long id );
		bool canLeave( Creature *ch );
		bool canJoin( Creature *ch );
		qplayer_data &getPlayer( long id );
		qplayer_data &getPlayer( int index ) { return players[index]; }
		qplayer_data &getBan( long id );
		void save( ostream &out );
	public: // accessors
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

	private: // utils
		void clearDescs();
		bool levelOK( Creature *ch );
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
void do_qcontrol_show(Creature * ch, char *argument);
void do_qcontrol_options(Creature *ch);
void do_qcontrol_create(Creature *ch, char *argument, int com);
void do_qcontrol_end(Creature *ch, char *argument, int com);
void do_qcontrol_add(Creature *ch, char *argument, int com);
void do_qcontrol_kick(Creature *ch, char *argument, int com);
void do_qcontrol_flags(Creature *ch, char *argument, int com);
void do_qcontrol_comment(Creature *ch, char *argument, int com);
void do_qcontrol_desc(Creature *ch, char *argument, int com);
void do_qcontrol_update(Creature *ch, char *argument, int com);
void do_qcontrol_ban(Creature *ch, char *argument, int com);
void do_qcontrol_unban(Creature *ch, char *argument, int com);
void do_qcontrol_level(Creature *ch, char *argument, int com);
void do_qcontrol_minlev(Creature *ch, char *argument, int com);
void do_qcontrol_maxlev(Creature *ch, char *argument, int com);
void do_qcontrol_mingen(Creature *ch, char *argument, int com);
void do_qcontrol_maxgen(Creature *ch, char *argument, int com);
void do_qcontrol_mute(Creature *ch, char *argument, int com);
void do_qcontrol_unmute(Creature *ch, char *argument, int com);
void do_qcontrol_award(Creature *ch, char *argument, int com);
void do_qcontrol_penalize(Creature *ch, char *argument, int com);
void do_qcontrol_save(Creature *ch, char *argument, int com);
void do_qcontrol_mload(Creature *ch, char *argument, int com);	//Load mobile.
void do_qcontrol_purge(Creature *ch, char *argument, int com);	//Purge mobile.
void do_qcontrol_trans(Creature *ch, char *argument, int com);	//trans whole quest

// utility functions
void do_qcontrol_usage(Creature *ch, int com);
Quest *find_quest(Creature *ch, char *argument);
char *list_active_quests(Creature *ch);
char *list_inactive_quests(Creature *ch);
Quest *quest_by_vnum(int vnum);
void qp_reload(int sig = 0);

void qlog(Creature *ch, char *str, int type, int level, int file);

Creature *check_char_vis(Creature *ch, char *name);
void list_quest_players(Creature *ch, Quest * quest, char *outbuf);
void list_quest_bans(Creature *ch, Quest * quest, char *outbuf);
int boot_quests(void);
int check_editors(Creature *ch, char **buffer);
void save_quests();

// quest subfunctions and utils
void do_quest_list(Creature *ch);
void do_quest_join(Creature *ch, char *argument);
void do_quest_info(Creature *ch, char *argument);
void do_quest_status(Creature *ch, char *argument);
void do_quest_who(Creature *ch, char *argument);
void do_quest_leave(Creature *ch, char *argument);
void do_quest_current(Creature *ch, char *argument);
void do_quest_ignore(Creature *ch, char *argument);

// external FILES
extern FILE *player_fl;
#endif
