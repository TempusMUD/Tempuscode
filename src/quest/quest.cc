//
// File: quest.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
//  File: quest.c -- Quest system for Tempus MUD 
//  by Fireball, December 1997
//
//  Copyright 1998 John Watson
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <fstream>
#include "xml_utils.h"

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "quest.h"
#include "handler.h"
#include "screen.h"
#include "tmpstr.h"
#include "utils.h"
#include "security.h"
#include "player_table.h"

// external funcs here
ACMD(do_switch);
// external vars here
extern struct descriptor_data *descriptor_list;

// internal funcs here
void do_qcontrol_help(struct Creature *ch, char *argument);
void do_qcontrol_switch(struct Creature *ch, char *argument, int com);
void do_qcontrol_oload_list(Creature * ch);
void do_qcontrol_oload(Creature *ch, char *argument, int com);
void send_to_quest(Creature *ch, char *str, Quest * quest, int level, int mode);

// internal vars here

const struct qcontrol_option {
	char *keyword;
	char *usage;
	int level;
} qc_options[] = {
	{
	"show", "[quest vnum]", LVL_AMBASSADOR}, {
	"create", "<type> <name>", LVL_AMBASSADOR}, {
	"end", "<quest vnum>", LVL_AMBASSADOR}, {
	"add", "<name> <vnum>", LVL_AMBASSADOR}, {
	"kick", "<player> <quest vnum>", LVL_AMBASSADOR},	// 5
	{
	"flags", "<quest vnum> <+/-> <flags>", LVL_AMBASSADOR}, {
	"comment", "<quest vnum> <comments>", LVL_AMBASSADOR}, {
	"describe", "<quest vnum>", LVL_AMBASSADOR}, {
	"update", "<quest vnum>", LVL_AMBASSADOR}, {
	"ban", "<player> <quest vnum>", LVL_AMBASSADOR},	// 10
	{
	"unban", "<player> <quest vnum>", LVL_AMBASSADOR}, {
	"mute", "<player> <quest vnum>", LVL_AMBASSADOR}, {
	"unmute", "<player> <quest vnum>", LVL_AMBASSADOR}, {
	"level", "<quest vnum> <access level>", LVL_AMBASSADOR}, {
	"minlev", "<quest vnum> <minlev>", LVL_AMBASSADOR},	// 15
	{
	"maxlev", "<quest vnum> <maxlev>", LVL_AMBASSADOR}, {
	"mingen", "<quest vnum> <min generation>", LVL_AMBASSADOR}, {
	"maxgen", "<quest vnum> <max generation>", LVL_AMBASSADOR}, {
	"mload", "<mobile vnum> <vnum>", LVL_IMMORT}, {
	"purge", "<quest vnum> <mobile name>", LVL_IMMORT},	// 20
	{
	"save", "", LVL_IMMORT }, {
	"help", "<topic>", LVL_AMBASSADOR}, {
	"switch", "<mobile name>", LVL_IMMORT}, {
	"rename", "<obj name> <new obj name>", LVL_GRIMP}, {
	"oload", "<item num> <vnum>", LVL_AMBASSADOR}, {
	"trans", "<quest vnum> [room number]", LVL_AMBASSADOR}, {
	"award", "<quest vnum> <player> <pts> [comments]", LVL_AMBASSADOR}, {
	"penalize", "<quest vnum> <player> <pts> [reason]", LVL_AMBASSADOR}, {
	NULL, NULL, 0}				// list terminator
};

const char *qtypes[] = {
	"trivia",
	"scavenger",
	"hide-and-seek",
	"roleplay",
	"pkill",
	"award/payment",
	"misc",
	"\n"
};

const char *qtype_abbrevs[] = {
	"trivia",
	"scav",
	"h&s",
	"RP",
	"pkill",
	"A/P",
	"misc",
	"\n"
};

const char *quest_bits[] = {
	"REVIEWED",
	"NOWHO",
	"NO_OUTWHO",
	"NOJOIN",
	"NOLEAVE",
	"HIDE",
	"WHOWHERE",
	"\n"
};

const char *quest_bit_descs[] = {
	"This quest has been reviewed.",
	"The \'quest who\' command does not work in this quest.",
	"Players in this quest cannot use the \'who\' command.",
	"Players may not join this quest.",
	"Players may not leave this quest.",
	"Players cannot see this quest until this flag is removed.",
	"\'quest who\' will show the locations of other questers.",
	"\n"	
};

const char *qlog_types[] = {
	"off",
	"brief",
	"normal",
	"complete",
	"\n"
};

const char *qp_bits[] = {
	"DEAF",
	"MUTE",
	"\n"
};

class QuestControl : protected vector<Quest> {
	public:
		QuestControl() : vector<Quest>() {
			filename = "etc/quest.xml";
			top_vnum = 0;
		}
		~QuestControl() {
			save();
		}
		void loadQuests();
		void add( Quest &q ) { push_back(q); }
		vector<Quest>::size;
		vector<Quest>::operator[];
		void save();
		int getNextVnum() { 
			return ++top_vnum;
		}
		void setNextVnum( int vnum ) {
			if( vnum >= top_vnum ) {
				top_vnum = vnum;
			}
		}
	private:
		int top_vnum;
		char *filename;
} quests;

void
QuestControl::loadQuests()
{
	erase(begin(),end());
	xmlDocPtr doc = xmlParseFile(filename);
	if (doc == NULL) {
		slog("SYSERR: Quesst load FAILED.");
		return;
	}
	// discard root node
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		xmlFreeDoc(doc);
		return;
	}

	cur = cur->xmlChildrenNode;
	// Load all the nodes in the file
	while (cur != NULL) {
		// But only question nodes
		if ((xmlMatches(cur->name, "Quest"))) {
			push_back(Quest(cur, doc));
		}
		cur = cur->next;
	}
	sort( begin(), end() );
	xmlFreeDoc(doc);
}

void
QuestControl::save()
{
	std::ofstream out(filename);
	if(! out ) {
		fprintf(stderr,"ERROR: Cannot open quest file: %s\r\n",filename);
	}
	out << "<Quests>" << endl;
	for( unsigned int i = 0; i < quests.size(); i++ ) {
		quests[i].save(out);
	}
	out << "</Quests>" << endl;
	out.flush();
	out.close();
}



char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
FILE *qlogfile = NULL;

ACMD(do_qcontrol)
{
	char arg1[MAX_INPUT_LENGTH];
	int com;

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);
	if (!*arg1) {
		do_qcontrol_options(ch);
		return;
	}
	/*
	   if(! Security::isMember( ch, "Questors" ) ) {
	   return;
	   } */
	for (com = 0;; com++) {
		if (!qc_options[com].keyword) {
			send_to_char(ch, "Unknown qcontrol option, '%s'.\r\n", arg1);
			return;
		}
		if (is_abbrev(arg1, qc_options[com].keyword))
			break;
	}
	if (qc_options[com].level > GET_LEVEL(ch)) {
		send_to_char(ch, "You are not godly enough to do this!\r\n");
		return;
	}

	switch (com) {
	case 0:					// show
		do_qcontrol_show(ch, argument);
		break;
	case 1:					// create
		do_qcontrol_create(ch, argument, com);
		break;
	case 2:					// end
		do_qcontrol_end(ch, argument, com);
		break;
	case 3:					// add
		do_qcontrol_add(ch, argument, com);
		break;
	case 4:					// kick
		do_qcontrol_kick(ch, argument, com);
		break;
	case 5:					// flags
		do_qcontrol_flags(ch, argument, com);
		break;
	case 6:					// comment
		do_qcontrol_comment(ch, argument, com);
		break;
	case 7:					// desc
		do_qcontrol_desc(ch, argument, com);
		break;
	case 8:					// update
		do_qcontrol_update(ch, argument, com);
		break;
	case 9:					// ban
		do_qcontrol_ban(ch, argument, com);
		break;
	case 10:					// unban
		do_qcontrol_unban(ch, argument, com);
		break;
	case 11:					// mute
		do_qcontrol_mute(ch, argument, com);
		break;
	case 12:					// unnute
		do_qcontrol_unmute(ch, argument, com);
		break;
	case 13:					// level
		do_qcontrol_level(ch, argument, com);
		break;
	case 14:					// minlev
		do_qcontrol_minlev(ch, argument, com);
		break;
	case 15:					// maxlev
		do_qcontrol_maxlev(ch, argument, com);
		break;
	case 16:					// maxgen
		do_qcontrol_mingen(ch, argument, com);
		break;
	case 17:					// mingen
		do_qcontrol_maxgen(ch, argument, com);
		break;
	case 18:					// Load Mobile
		do_qcontrol_mload(ch, argument, com);
		break;
	case 19:					// Purge Mobile
		do_qcontrol_purge(ch, argument, com);
		break;
	case 20:
		do_qcontrol_save(ch, argument, com);
		break;
	case 21:					// help - in help_collection.cc
		do_qcontrol_help(ch, argument);
		break;
	case 22:
		do_qcontrol_switch(ch, argument, com);
		break;
	case 23:// rename
		//do_qcontrol_rename( ch, argument ,com );
		send_to_char(ch, "Not Implemented\r\n");
		break;
	case 24:					// oload
		do_qcontrol_oload(ch, argument, com);
		break;
	case 25:
		do_qcontrol_trans(ch, argument, com);
		break;
	case 26:					// award
		do_qcontrol_award(ch, argument, com);
		break;
	case 27:					// penalize
		do_qcontrol_penalize(ch, argument, com);
		break;
	default:
		send_to_char(ch, "Sorry, this qcontrol option is not implemented.\r\n");
		break;
	}
}

void							//Load mobile.
do_qcontrol_mload(Creature *ch, char *argument, int com)
{
	struct Creature *mob;
	struct Quest *quest = NULL;
	char arg1[MAX_INPUT_LENGTH];
	int number;

	argument = two_arguments(argument, buf, arg1);

	if (!*buf || !isdigit(*buf) || !*arg1 || !isdigit(*arg1)) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1))) {
		return;
	}
	if (!quest->canEdit(ch)) {
		return;
	}
	if (quest->getEnded() ) {
		send_to_char(ch, "Pay attentionu dummy! That quest is over!\r\n");
		return;
	}

	if ((number = atoi(buf)) < 0) {
		send_to_char(ch, "A NEGATIVE number??\r\n");
		return;
	}
	if (!real_mobile_proto(number)) {
		send_to_char(ch, "There is no mobile thang with that number.\r\n");
		return;
	}
	mob = read_mobile(number);
	char_to_room(mob, ch->in_room,false);
	act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
		0, 0, TO_ROOM);
	act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
	act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);

	sprintf(buf, "mloaded %s at %d.", GET_NAME(mob), ch->in_room->number);
	qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), TRUE);

}

void
do_qcontrol_oload_list(Creature * ch)
{
	int i = 0;
	char main_buf[MAX_STRING_LENGTH];
	obj_data *obj;
	strcpy(main_buf, "Valid Quest Objects:\r\n");
	for (i = MIN_QUEST_OBJ_VNUM; i <= MAX_QUEST_OBJ_VNUM; i++) {
		if (!(obj = read_object(i)))
			continue;
		sprintf(buf, "    %s%d. %s%s %s: %d qps ", CCNRM(ch, C_NRM),
			i - MIN_QUEST_OBJ_VNUM, CCGRN(ch, C_NRM), obj->short_description,
			CCNRM(ch, C_NRM), (obj->shared->cost / 100000));
		if (IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
			strcat(buf, "(!ap)\r\n");
		else
			strcat(buf, "\r\n");
		strcat(main_buf, buf);
		extract_obj(obj);
	}
	send_to_char(ch, main_buf);
}

// Load Quest Object
void
do_qcontrol_oload(Creature *ch, char *argument, int com)
{
	struct obj_data *obj;
	struct Quest *quest = NULL;
	int number;
	char arg2[MAX_INPUT_LENGTH];

	argument = two_arguments(argument, buf, arg2);


	if (!*buf || !isdigit(*buf)) {
		do_qcontrol_oload_list(ch);
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2))) {
		return;
	}
	if (!quest->canEdit(ch)) {
		return;
	}
	if (quest->getEnded() ) {
		send_to_char(ch, "Pay attentionu dummy! That quest is over!\r\n");
		return;
	}

	if ((number = atoi(buf)) < 0) {
		send_to_char(ch, "A NEGATIVE number??\r\n");
		return;
	}
	if (number > MAX_QUEST_OBJ_VNUM - MIN_QUEST_OBJ_VNUM) {
		send_to_char(ch, "Invalid item number.\r\n");
		do_qcontrol_oload_list(ch);
		return;
	}
	obj = read_object(number + MIN_QUEST_OBJ_VNUM);

	if (!obj) {
		send_to_char(ch, "Error, no object loaded\r\n");
		return;
	}
	if (obj->shared->cost < 0) {
		send_to_char(ch, "This object is messed up.\r\n");
		extract_obj(obj);
		return;
	}
	if (GET_LEVEL(ch) == LVL_AMBASSADOR && obj->shared->cost > 0) {
		send_to_char(ch, "You can only load objects with a 0 cost.\r\n");
		extract_obj(obj);
		return;
	}

	if (((obj->shared->cost / 100000) > GET_QUEST_POINTS(ch))) {
		send_to_char(ch, "You do not have the required quest points.\r\n");
		extract_obj(obj);
		return;
	}

	GET_QUEST_POINTS(ch) -= (obj->shared->cost / 100000);
	obj_to_char(obj, ch);
	ch->crashSave();
	act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
		0, 0, TO_ROOM);
	act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
	act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);

	sprintf(buf, "loaded %s at %d.", obj->short_description,
		ch->in_room->number);
	qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), TRUE);

}


void							//Purge mobile.
do_qcontrol_trans(Creature *ch, char *argument, int com)
{

	Creature *vict;
	Quest *quest = NULL;
	room_data *room = NULL;

	argument = two_arguments(argument, arg1, arg2);
	if (!*arg1) {
		send_to_char(ch, "Usage: qcontrol trans <quest number> [room number]\r\n");
		return;
	}
	if (!(quest = find_quest(ch, arg1))) {
		return;
	}
	if (!quest->canEdit(ch)) {
		return;
	}
	if (quest->getEnded() ) {
		send_to_char(ch, "Pay attention dummy! That quest is over!\r\n");
		return;
	}
	if( *arg2 ) {
		if(! is_number(arg2) ) {
			send_to_char(ch, "No such room: '%s'\r\n",arg2);
			return;
		}
		room = real_room(atoi(arg2));
		if( room == NULL ) {
			send_to_char(ch, "No such room: '%s'\r\n",arg2);
			return;
		}
	}

	if( room == NULL )
		room = ch->in_room;

	int transCount = 0;
	for (int i = 0; i < quest->getNumPlayers(); i++) {
		long id = quest->getPlayer(i).idnum;
		vict = get_char_in_world_by_idnum(id);
		if ( vict == NULL || vict == ch )
			continue;
		if ((GET_LEVEL(ch) < GET_LEVEL(vict)) ) {
			send_to_char(ch, "%s ignores your summons.\r\n", GET_NAME(vict));
			continue;
		}
		++transCount;
		act("$n disappears in a mushroom cloud.", FALSE, vict, 0, 0, TO_ROOM);
		char_from_room(vict,false);
		char_to_room(vict, room,false);
		act("$n arrives from a puff of smoke.", FALSE, vict, 0, 0, TO_ROOM);
		act("$n has transferred you!", FALSE, ch, 0, vict, TO_VICT);
		look_at_room(vict, room, 0);
	}
	char *buf = tmp_sprintf("has transferred %d questers to %s[%d] for quest %d.", 
		 					transCount, room->name,room->number, quest->getVnum());
	qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), TRUE);

	send_to_char(ch,"%d players transferred.\r\n",transCount);
}


void							//Purge mobile.
do_qcontrol_purge(Creature *ch, char *argument, int com)
{

	struct Creature *vict;
	struct Quest *quest = NULL;
	char arg1[MAX_INPUT_LENGTH];


	argument = two_arguments(argument, arg1, buf);
	if (!*buf) {
		send_to_char(ch, "Purge what?\r\n");
		return;
	}
	if (!(quest = find_quest(ch, arg1))) {
		return;
	}
	if (!quest->canEdit(ch)) {
		return;
	}
	if (quest->getEnded() ) {
		send_to_char(ch, "Pay attentionu dummy! That quest is over!\r\n");
		return;
	}

	if ((vict = get_char_room_vis(ch, buf))) {
		if (!IS_NPC(vict)) {
			send_to_char(ch, "You don't need a quest to purge them!\r\n");
			return;
		}
		act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		sprintf(buf, "has purged %s at %d.",
			GET_NAME(vict), vict->in_room->number);
		qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), TRUE);
		if (vict->desc) {
			close_socket(vict->desc);
			vict->desc = NULL;
		}
		vict->purge(false);
		send_to_char(ch, OK);
	} else {
		send_to_char(ch, "Purge what?\r\n");
		return;
	}

}


void
do_qcontrol_show(Creature *ch, char *argument)
{

	int timediff;
	Quest *quest = NULL;
	char *timestr_e, *timestr_s;
	char timestr_a[16];

	if (!quests.size()) {
		send_to_char(ch, "There are no quests to show.\r\n");
		return;
	}

	// show all quests
	if (!*argument) {
		char *msg;

		msg = list_active_quests(ch);
		msg = tmp_strcat(msg, list_inactive_quests(ch));
		page_string(ch->desc, msg);
		return;
	}

	// list q specific quest
	if (!(quest = find_quest(ch, argument)))
		return;

	time_t started = quest->getStarted();
	timestr_s = asctime(localtime(&started));
	*(timestr_s + strlen(timestr_s) - 1) = '\0';


	// quest is over, show summary information
	if (quest->getEnded() ) {

		timediff = quest->getEnded() - quest->getStarted();
		sprintf(timestr_a, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

		time_t started = quest->getStarted();
		timestr_e = asctime(localtime(&started));
		*(timestr_e + strlen(timestr_e) - 1) = '\0';

		sprintf(buf,
			"Owner:  %-30s [%2d]\r\n"
			"Name:   %s\r\n"
			"Status: COMPLETED\r\n"
			"Description:\r\n%s"
			"  Type:           %s\r\n"
			"  Started:        %s\r\n"
			"  Ended:          %s\r\n"
			"  Age:            %s\r\n"
			"  Min Level:   Gen %-2d, Level %2d\r\n"
			"  Max Level:   Gen %-2d, Level %2d\r\n"
			"  Max Players:    %d\r\n"
			"  Pts. Awarded:   %d\r\n",
			playerIndex.getName(quest->getOwner()), quest->owner_level,
			quest->name,
			quest->description ? quest->description : "None.\r\n",
			qtypes[(int)quest->type], timestr_s,
			timestr_e, timestr_a,
			quest->mingen , quest->minlevel,
			quest->maxgen , quest->maxlevel,
			quest->getMaxPlayers(), quest->getAwarded());
		page_string(ch->desc, buf);
		return;

	}
	// quest is still active

	timediff = time(0) - quest->getStarted();
	sprintf(timestr_a, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

	sprintbit(quest->flags, quest_bits, buf2);

	sprintf(buf,
		"Owner:  %-30s [%2d]\r\n"
		"Name:   %s\r\n"
		"Status: ACTIVE\r\n"
		"Description:\r\n%s"
		"Updates:\r\n%s"
		"  Type:            %s\r\n"
		"  Flags:           %s\r\n"
		"  Started:         %s\r\n"
		"  Age:             %s\r\n"
		"  Min Level:   Gen %-2d, Level %2d\r\n"
		"  Max Level:   Gen %-2d, Level %2d\r\n"
		"  Num Players:     %d\r\n"
		"  Max Players:     %d\r\n"
		"  Pts. Awarded:    %d\r\n",
		playerIndex.getName(quest->getOwner()), quest->owner_level,
		quest->name,
		quest->description ? quest->description : "None.\r\n",
		quest->updates ? quest->updates : "None.\r\n",
		qtypes[(int)quest->type], buf2, timestr_s,
		timestr_a,
		quest->mingen , quest->minlevel,
		quest->maxgen , quest->maxlevel,
		quest->getNumPlayers(), quest->getMaxPlayers(), quest->getAwarded());

	if (quest->getNumPlayers()) {
		list_quest_players(ch, quest, buf2);
		strcat(buf, buf2);
	}

	if (quest->getNumBans()) {
		list_quest_bans(ch, quest, buf2);
		strcat(buf, buf2);
	}

	page_string(ch->desc, buf);

}

void
do_qcontrol_options(Creature *ch)
{
	int i = 0;

	strcpy(buf, "qcontrol options:\r\n");
	while (1) {
		if (!qc_options[i].keyword)
			break;
		sprintf(buf, "%s  %-15s %s\r\n", buf, qc_options[i].keyword,
			qc_options[i].usage);
		i++;
	}
	page_string(ch->desc, buf);
}

void
do_qcontrol_usage(Creature *ch, int com)
{
	if (com < 0)
		do_qcontrol_options(ch);
	else {
		send_to_char(ch, "Usage: qcontrol %s %s\r\n",
			qc_options[com].keyword, qc_options[com].usage);
	}
}

int
find_quest_type(char *argument)
{
	int i = 0;

	while (1) {
		if (*qtypes[i] == '\n')
			break;
		if (is_abbrev(argument, qtypes[i]))
			return i;
		i++;
	}
	return (-1);
}

void
qcontrol_show_valid_types( Creature *ch ) {
	char *msg = tmp_sprintf("  Valid Types:\r\n");
	int i = 0;
	while (1) {
		if (*qtypes[i] == '\n')
			break;
		char *line = tmp_sprintf("    %2d. %s\r\n", i, qtypes[i]);
		msg = tmp_strcat(msg,line);
		i++;
	}
	page_string(ch->desc, msg);
	return;
}

void
do_qcontrol_create(Creature *ch, char *argument, int com)
{
	int type;
	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (!*arg1 || !*argument) {
		do_qcontrol_usage(ch, com);
		qcontrol_show_valid_types(ch);
		return;
	}

	if ((type = find_quest_type(arg1)) < 0) {
		send_to_char(ch, "Invalid quest type '%s'.\r\n",arg1);
		qcontrol_show_valid_types(ch);
		return;
	}

	if (strlen(argument) >= MAX_QUEST_NAME) {
		send_to_char(ch, "Quest name too long.  Max length %d characters.\r\n",
			MAX_QUEST_NAME - 1);
		return;
	}

	Quest quest(ch,type,argument);
	quests.add(quest);

	char *msg = tmp_sprintf( "created quest [%d] type %s, '%s'", 
							 quest.getVnum(), qtypes[type], argument );
	qlog(ch, msg, QLOG_BRIEF, LVL_AMBASSADOR, TRUE);
	send_to_char(ch, "Quest %d created.\r\n", quest.getVnum());
	save_quests();
}

/*
 * Callisto tells you, 'we had to join a different quest... get kicked from it, and then he closed that quest and it was fine'
 */
void
do_qcontrol_end(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	if (!*argument) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already ended... duh.\r\n");
		return;
	}

	send_to_quest(ch, "Quest ending...", quest, 0, QCOMM_ECHO);

	qlog(ch, "Purging players from quest...", QLOG_COMP, 0, TRUE);

	while (quest->getNumPlayers()) {
		// TODO: Go back when you get time and make this set in the player file.
		if (!quest->removePlayer(quest->getPlayer((int)0).idnum)) {
			send_to_char(ch, "Error removing char from quest.\r\n");
			break;
		}

	}

	quest->setEnded(time(0));
	sprintf(buf, "ended quest %d '%s'", quest->getVnum(),quest->name);
	qlog(ch, buf, QLOG_BRIEF, 0, TRUE);
	send_to_char(ch, "Quest ended.\r\n");
	save_quests();
}

void
do_qcontrol_add(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;
	Creature *vict = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if (!(vict = check_char_vis(ch, arg1)))
		return;

	if (IS_NPC(vict)) {
		send_to_char(ch, "You cannot add mobiles to quests.\r\n");
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already ended, you wacko.\r\n");
		return;
	}

	if( quest->isPlaying(GET_IDNUM(vict) ) ) {
		send_to_char(ch, "That person is already part of this quest.\r\n");
		return;
	}

	if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch) {
		send_to_char(ch, "You are not powerful enough to do this.\r\n");
		return;
	}

	if (!quest->canEdit(ch)) 
		return;

	if ( !quest->addPlayer(GET_IDNUM(vict)) ) {
		send_to_char(ch, "Error adding char to quest.\r\n");
		return;
	}
	GET_QUEST(vict) = quest->getVnum();

	sprintf(buf, "added %s to quest %d '%s'.", 
			GET_NAME(vict), quest->getVnum(),quest->name);
	qlog(ch, buf, QLOG_COMP, GET_INVIS_LVL(vict), TRUE);

	send_to_char(ch, "%s added to quest %d.\r\n", GET_NAME(vict), quest->getVnum());

	send_to_char(vict, "%s has added you to quest %d.\r\n", GET_NAME(ch),
		quest->getVnum());

	sprintf(buf, "%s is now part of the quest.", GET_NAME(vict));
	send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(vict), LVL_AMBASSADOR),
		QCOMM_ECHO);
}

void
do_qcontrol_kick(Creature *ch, char *argument, int com)
{

	Quest *quest = NULL;
	Creature *vict = NULL;
	unsigned int idnum;
	int pid;
	int level = 0;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if ((idnum = playerIndex.getID(arg1)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg1);
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, 
			"That quest has already ended.. there are no players in it.\r\n");
		return;
	}

	if (!quest->canEdit(ch)) 
		return;

	if(! quest->isPlaying(idnum) ) {
		send_to_char(ch, "That person not participating in this quest.\r\n");
		return;
	}

	if (!(vict = get_char_in_world_by_idnum(idnum))) {
		// load the char from file
		vict = new Creature(true);
		pid = playerIndex.getID(arg1);
		if (pid > 0) {
			vict->loadFromXML(pid);
			level = GET_LEVEL(vict);
			delete vict;
			vict = NULL;
		} else {
			send_to_char(ch, "Error loading char from file.\r\n");
			return;
		}

	} else {
		level = GET_LEVEL(vict);
	}


	if (level >= GET_LEVEL(ch) && vict && ch != vict) {
		send_to_char(ch, "You are not powerful enough to do this.\r\n");
		return;
	}

	if (!quest->removePlayer(idnum)) {
		send_to_char(ch, "Error removing char from quest.\r\n");
		return;
	}


	send_to_char(ch, "%s kicked from quest %d.\r\n", arg1, quest->getVnum());
	if (vict) {
		sprintf(buf, "kicked %s from quest %d '%s'.", 
				arg1, quest->getVnum(),quest->name);
		qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(vict), LVL_AMBASSADOR),
			TRUE);

		send_to_char(vict, "%s kicked you from quest %d.\r\n",
			GET_NAME(ch), quest->getVnum());

		sprintf(buf, "%s has been kicked from the quest.", arg1);
		send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(vict),
				LVL_AMBASSADOR), QCOMM_ECHO);
	} else {
		sprintf(buf, "kicked %s from quest %d '%s'.", 
				arg1, quest->getVnum(),quest->name);
		qlog(ch, buf, QLOG_BRIEF, LVL_AMBASSADOR, TRUE);

		sprintf(buf, "%s has been kicked from the quest.", arg1);
		send_to_quest(NULL, buf, quest, LVL_AMBASSADOR, QCOMM_ECHO);
	}

	save_quests();
}

void
qcontrol_show_valid_flags( Creature *ch ) {

	char *msg = tmp_sprintf("  Valid Quest Flags:\r\n");
	int i = 0;
	while (1) {
		if (*quest_bits[i] == '\n')
			break;
		char *line = tmp_sprintf("    %2d. %s - %s\r\n", 
								 i, 
								 quest_bits[i],
								 quest_bit_descs[i]);
		msg = tmp_strcat(msg,line);
		i++;
	}
	page_string(ch->desc, msg);
	return;
}
void
do_qcontrol_flags(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;
	int state, cur_flags = 0, tmp_flags = 0, flag = 0, old_flags = 0;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !*argument) {
		do_qcontrol_usage(ch, com);
		qcontrol_show_valid_flags(ch);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch)) 
		return;

	if (*arg2 == '+')
		state = 1;
	else if (*arg2 == '-')
		state = 2;
	else {
		do_qcontrol_usage(ch, com);
		qcontrol_show_valid_flags(ch);
		return;
	}

	argument = one_argument(argument, arg1);

	old_flags = cur_flags = quest->flags;

	while (*arg1) {
		if ((flag = search_block(arg1, quest_bits, FALSE)) == -1) {
			send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
		} else
			tmp_flags = tmp_flags | (1 << flag);

		argument = one_argument(argument, arg1);
	}

	if (state == 1)
		cur_flags = cur_flags | tmp_flags;
	else {
		tmp_flags = cur_flags & tmp_flags;
		cur_flags = cur_flags ^ tmp_flags;
	}

	quest->flags = cur_flags;

	tmp_flags = old_flags ^ cur_flags;
	sprintbit(tmp_flags, quest_bits, buf2);

	if (tmp_flags == 0) {
		send_to_char(ch, "Flags for quest %d not altered.\r\n", quest->getVnum());
	} else {
		send_to_char(ch, "[%s] flags %s for quest %d.\r\n", buf2,
			state == 1 ? "added" : "removed", quest->getVnum());

		sprintf(buf, "%s [%s] flags for quest %d '%s'.",
			state == 1 ? "added" : "removed", 
			buf2, quest->getVnum(), quest->name);
		qlog(ch, buf, QLOG_COMP, LVL_AMBASSADOR, TRUE);
	}
	save_quests();
}

void
do_qcontrol_comment(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (!*argument || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch)) 
		return;

	sprintf(buf, "comments on quest %d '%s': %s", 
			quest->getVnum(), quest->name, argument);
	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);
	send_to_char(ch, "Comment logged as '%s'", argument);

	save_quests();
}

void
do_qcontrol_desc(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, argument)))
		return;

	if (!quest->canEdit(ch))
		return;

	if (check_editors(ch, &(quest->description)))
		return;


	act("$n begins to edit a quest description.\r\n", TRUE, ch, 0, 0, TO_ROOM);

	if (quest->description) {
		sprintf(buf, "began editing description of quest '%s'", quest->name);
	} else {
		sprintf(buf, "began writing description of quest '%s'", quest->name);
	}

	start_text_editor(ch->desc, &quest->description, true, MAX_QUEST_DESC);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}

void
do_qcontrol_update(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, argument)))
		return;

	if (!quest->canEdit(ch))
		return;

	if (check_editors(ch, &(quest->updates)))
		return;

	if (quest->description) {
		sprintf(buf, "began editing update of quest '%s'", quest->name);
	} else {
		sprintf(buf, "began writing the update of quest '%s'", quest->name);
	}

	start_text_editor(ch->desc, &quest->updates, true, MAX_QUEST_UPDATE);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

	act("$n begins to edit a quest update.\r\n", TRUE, ch, 0, 0, TO_ROOM);
	qlog(ch, buf, QLOG_COMP, LVL_AMBASSADOR, TRUE);
}

void
do_qcontrol_ban(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;
	Creature *vict = NULL;
	unsigned int idnum;
	int pid;
	int level = 0;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if (!quest->canEdit(ch))
		return;

	if ((idnum = playerIndex.getID(arg1)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg1);
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already , you psychopath!\r\n");
		return;
	}

	if (!(vict = get_char_in_world_by_idnum(idnum))) {
		// load the char from file
		vict = new Creature(true);
		pid = playerIndex.getID(arg1);
		if (pid > 0) {
			vict->loadFromXML(pid);
			level = GET_LEVEL(vict);
			delete vict;
			vict = NULL;
		} else {
			send_to_char(ch, "Error loading char from file.\r\n");
			return;
		}

	} else {
		level = GET_LEVEL(vict);
	}

	if (level >= GET_LEVEL(ch) && vict && ch != vict) {
		send_to_char(ch, "You are not powerful enough to do this.\r\n");
		return;
	}

	if (quest->isBanned(idnum)) {
		send_to_char(ch, "That character is already banned from this quest.\r\n");
		return;
	}

	if (quest->isPlaying(idnum)) {
		if (!quest->removePlayer(idnum)) {
			send_to_char(ch, "Unable to auto-kick victim from quest!\r\n");
		} else {
			send_to_char(ch, "%s auto-kicked from quest.\r\n", arg1);

			sprintf(buf, "auto-kicked %s from quest %d '%s'.",
				vict ? GET_NAME(vict) : arg1, quest->getVnum(), quest->name);
			qlog(ch, buf, QLOG_COMP, 0, TRUE);
		}
	}

	if (!quest->addBan(idnum)) {
		send_to_char(ch, "Error banning char from quest.\r\n");
		return;
	}

	if (vict) {
		send_to_char(ch, "You have been banned from quest '%s'.\r\n", quest->name);
	}

	sprintf(buf, "banned %s from quest %d '%s'.",
		vict ? GET_NAME(vict) : arg1, quest->getVnum(), quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "%s banned from quest %d.\r\n",
		vict ? GET_NAME(vict) : arg1, quest->getVnum());

}

void
do_qcontrol_unban(Creature *ch, char *argument, int com)
{

	Quest *quest = NULL;
	Creature *vict = NULL;
	unsigned int idnum;
	int level = 0;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if (!quest->canEdit(ch))
		return;

	if ((idnum = playerIndex.getID(arg1)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg1);
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
		return;
	}

	if (!(vict = get_char_in_world_by_idnum(idnum))) {
		// load the char from file
		vict = new Creature(true);
		if (idnum > 0) {
			vict->loadFromXML(idnum);
			level = GET_LEVEL(vict);
			delete vict;
			vict = NULL;
		} else {
			send_to_char(ch, "Error loading char from file.\r\n");
			return;
		}

	} else {
		level = GET_LEVEL(vict);
	}

	if (level >= GET_LEVEL(ch) && vict && ch != vict) {
		send_to_char(ch, "You are not powerful enough to do this.\r\n");
		return;
	}

	if (!quest->isBanned(idnum)) {
		send_to_char(ch, 
			"That player is not banned... maybe you should ban him!\r\n");
		return;
	}

	if (!quest->removeBan(idnum)) {
		send_to_char(ch, "Error unbanning char from quest.\r\n");
		return;
	}

	sprintf(buf, "unbanned %s from %d quest '%s'.",
		vict ? GET_NAME(vict) : arg1,quest->getVnum(), quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "%s unbanned from quest %d.\r\n",
		vict ? GET_NAME(vict) : arg1, quest->getVnum());

}

void
do_qcontrol_level(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (!*argument || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch))
		return;

	quest->owner_level = atoi(arg2);

	sprintf(buf, "set quest %d '%s' access level to %d",
		quest->getVnum(),quest->name, quest->owner_level);
	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);

}

void
do_qcontrol_minlev(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg2 || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch))
		return;

	quest->minlevel = MIN(LVL_GRIMP, MAX(0, atoi(arg2)));

	sprintf(buf, "set quest %d '%s' minimum level to %d",
		quest->getVnum(),quest->name, quest->minlevel);

	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);

}

void
do_qcontrol_maxlev(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg2 || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch))
		return;

	quest->maxlevel = MIN(LVL_GRIMP, MAX(0, atoi(arg2)));

	sprintf(buf, "set quest %d '%s' maximum level to %d",
		quest->getVnum(),quest->name, quest->maxlevel);
	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);
}


void
do_qcontrol_mingen(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg2 || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch))
		return;

	quest->mingen = MIN(10, MAX(0, atoi(arg2)));

	sprintf(buf, "set quest %d '%s' minimum gen to %d",
		quest->getVnum(),quest->name, quest->mingen );
	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);
}

void
do_qcontrol_maxgen(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg2 || !*arg1) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1)))
		return;

	if (!quest->canEdit(ch))
		return;

	quest->maxgen = MIN(10, MAX(0, atoi(arg2)));

	sprintf(buf, "set quest %d '%s' maximum gen to %d",
		quest->getVnum(),quest->name, quest->maxgen);
	qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, TRUE);
}


void
do_qcontrol_mute(Creature *ch, char *argument, int com)
{

	Quest *quest = NULL;
	long idnum;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if (!quest->canEdit(ch))
		return;

	if ((idnum = playerIndex.getID(arg1)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg1);
		return;
	}

	if (quest->getEnded()) {
		send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
		return;
	}

	if (! quest->isPlaying(idnum)) {
		send_to_char(ch, "That player is not in the quest.\r\n");
		return;
	}

	if (quest->getPlayer(idnum).isFlagged(QP_MUTE)) {
		send_to_char(ch, "That player is already muted.\r\n");
		return;
	}

	quest->getPlayer(idnum).setFlag(QP_MUTE);

	sprintf(buf, "muted %s in %d quest '%s'.", arg1, quest->getVnum(),quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "%s muted for quest %d.\r\n", arg1, quest->getVnum());

}


void
do_qcontrol_unmute(Creature *ch, char *argument, int com)
{

	Quest *quest = NULL;
	long idnum;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg2)))
		return;

	if (!quest->canEdit(ch))
		return;

	if ((idnum = playerIndex.getID(arg1)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg1);
		return;
	}

	if (quest->getEnded()) {
		send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
		return;
	}

	if(! quest->isPlaying(idnum) ) {
		send_to_char(ch, "That player is not in the quest.\r\n");
		return;
	}

	if (!quest->getPlayer(idnum).isFlagged(QP_MUTE)) {
		send_to_char(ch, "That player not muted.\r\n");
		return;
	}

	quest->getPlayer(idnum).removeFlag(QP_MUTE);

	sprintf(buf, "unmuted %s in quest %d '%s'.", arg1, quest->getVnum(), quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "%s unmuted for quest %d.\r\n", arg1, quest->getVnum());

}

void
do_qcontrol_switch(Creature *ch, char *argument, int com)
{
	struct Quest *quest = NULL;


	if ((!(quest = quest_by_vnum(GET_QUEST(ch))) ||
		!quest->isPlaying(GET_IDNUM(ch))  )
		&& (GET_LEVEL(ch) < LVL_ELEMENT)) {
		send_to_char(ch, "You are not currently active on any quest.\r\n");
		return;
	}
	do_switch(ch, argument, 0, SCMD_QSWITCH, 0);
}



/*************************************************************************
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * UTILITY FUNCTIONS                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *************************************************************************/




/*************************************************************************
 * function to find a quest                                              *
 * argument is the vnum of the quest as a string                         *
 *************************************************************************/
Quest *
find_quest(Creature *ch, char *argument)
{
	int vnum;
	Quest *quest = NULL;
	vnum = atoi(argument);

	if ((quest = quest_by_vnum(vnum)))
		return quest;

	send_to_char(ch, "There is no quest number %d.\r\n", vnum);

	return NULL;
}

/*************************************************************************
 * low level function to return a quest                                  *
 *************************************************************************/
Quest *
quest_by_vnum(int vnum)
{
	for( unsigned int i = 0; i < quests.size(); i++ ) {
		if( quests[i].getVnum() == vnum )
			return &quests[i];
	}
	return NULL;
}

/*************************************************************************
 * function to list active quests to both mortals and gods               *
 *************************************************************************/

char *
list_active_quests(Creature *ch)
{
	int timediff;
	int questCount = 0;
	char timestr_a[32];
	char *msg = 
"Active quests:\r\n"
"-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

	if (!quests.size())
		return "There are no active quests.\r\n";

	for (unsigned int i = 0; i < quests.size(); i++) {
		Quest *quest = &(quests[i]);
		if (quest->getEnded())
			continue;
		if (QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
			continue;
		questCount++;

		timediff = time(0) - quest->getStarted();
		snprintf(timestr_a, 16, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

		msg = tmp_sprintf( "%s %3d  %-10s  %-8s  %-24s %6s    %d\r\n", msg,
			quest->getVnum(), playerIndex.getName(quest->getOwner()), 
			qtype_abbrevs[(int)quest->type], quest->name, timestr_a,
			quest->getNumPlayers());
	}
	if (!questCount)
		return "There are no visible quests.\r\n";

	return tmp_sprintf("%s%d visible quest%s active.\r\n\r\n", msg,
		questCount, questCount == 1 ? "" : "s");
}

char *
list_inactive_quests(Creature *ch)
{
	int timediff;
	char timestr_a[128];
	int questCount = 0;
	char *msg =
"Finished Quests:\r\n"
"-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

	if (!quests.size())
		return "There are no finished quests.\r\n";

	for (int i = quests.size() - 1; i >= 0; --i) {
		Quest *quest = &(quests[i]);
		if (!quest->getEnded())
			continue;
		questCount++;
		timediff = quest->getEnded() - quest->getStarted();
		snprintf(timestr_a, 127, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

		msg = tmp_sprintf( "%s %3d  %-10s  %-8s  %-24s %6s    %d\r\n", msg,
			quest->getVnum(), playerIndex.getName(quest->getOwner()), 
			qtype_abbrevs[(int)quest->type], quest->name, timestr_a,
			quest->getNumPlayers());
	}

	if (!questCount)
		return "There are no finished quests.\r\n";

	return tmp_sprintf("%s%d visible quest%s finished.\r\n", msg, 
						  questCount, questCount == 1 ? "" : "s");
}

void
list_quest_players(Creature *ch, Quest * quest, char *outbuf)
{
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], name[128];
	int i, num_online, num_offline;
	Creature *vict = NULL;
	char bitbuf[1024];

	strcpy(buf, "  -Online Players------------------------------------\r\n");
	strcpy(buf2, "  -Offline Players-----------------------------------\r\n");

	for (i = num_online = num_offline = 0; i < quest->getNumPlayers(); i++) {


		sprintf(name, "%s", playerIndex.getName(quest->getPlayer(i).idnum));

		if (!*name) {
			strcat(buf, "BOGUS player idnum!\r\n");
			strcat(buf2, "BOGUS player idnum!\r\n");
			slog("SYSERR: bogus player idnum in list_quest_players.");
			break;
		}
		strcpy(name, CAP(name));

		if (quest->getPlayer(i).getFlags()) {
			sprintbit(quest->getPlayer(i).getFlags(), qp_bits, bitbuf);
		} else {
			strcpy(bitbuf, "");
		}

		// player is in world and visible
		if ((vict = get_char_in_world_by_idnum(quest->getPlayer(i).idnum)) &&
			can_see_creature(ch, vict)) {

			// see if we can see the locations of the players
			if (PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
				QUEST_FLAGGED(quest, QUEST_WHOWHERE)) {
				sprintf(buf, "%s  %2d. %-15s - %-10s - [%5d] %s %s\r\n", buf,
					++num_online, name, bitbuf, vict->in_room->number,
					vict->in_room->name, vict->desc ? "" : "   (linkless)");
			} else {
				sprintf(buf, "%s  %2d. %-15s - %-10s\r\n", buf, ++num_online,
					name, bitbuf);
			}

		}
		// player is either offline or invisible
		else if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
			sprintf(buf2, "%s  %2d. %-15s - %-10s\r\n", buf2, ++num_offline,
				name, bitbuf);
		}
	}

	// only gods may see the offline players
	if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		strcat(buf, buf2);

	if (outbuf)
		strcpy(outbuf, buf);
	else
		page_string(ch->desc, buf);

}

void
list_quest_bans(Creature *ch, Quest * quest, char *outbuf)
{
	char buf[MAX_STRING_LENGTH], name[128];
	int i, num;

	strcpy(buf, "  -Banned Players------------------------------------\r\n");

	for (i = num = 0; i < quest->getNumBans(); i++) {

		sprintf(name, "%s", playerIndex.getName(quest->getBan(i).idnum));
		if (!*name) {
			strcat(buf, "BOGUS player idnum!\r\n");
			slog("SYSERR: bogus player idnum in list_quest_bans.");
			break;
		}
		strcpy(name, CAP(name));

		sprintf(buf, "%s  %2d. %-20s\r\n", buf, ++num, name);
	}

	if (outbuf)
		strcpy(outbuf, buf);
	else
		page_string(ch->desc, buf);

}

void
qlog(Creature *ch, char *str, int type, int level, int file)
{
	time_t ct;
	char *tmstr;
	Creature *vict = NULL;
	char buf[MAX_STRING_LENGTH];

	// Mortals don't need to be seeing logs
	if (level < LVL_IMMORT)
		level = LVL_IMMORT;

	if (type) {
		CreatureList::iterator cit = characterList.begin();
		for (; cit != characterList.end(); ++cit) {
			vict = *cit;
			if (GET_LEVEL(vict) >= level && GET_QLOG_LEVEL(vict) >= type) {

				sprintf(buf,
					"%s[%s QLOG: %s %s %s]%s\r\n",
					CCYEL_BLD(vict, C_NRM), CCNRM_GRN(vict, C_NRM),
					ch ? PERS(ch, vict) : "",
					str, CCYEL_BLD(vict, C_NRM), CCNRM(vict, C_NRM));
				send_to_char(vict, "%s", buf);
			}
		}
	}

	if (file) {
		sprintf(buf, "%s %s\n", ch ? GET_NAME(ch) : "", str);
		ct = time(0);
		tmstr = asctime(localtime(&ct));
		*(tmstr + strlen(tmstr) - 1) = '\0';
		fprintf(qlogfile, "%-19.19s :: %s", tmstr, buf);
		fflush(qlogfile);
	}
}

Creature *
check_char_vis(Creature *ch, char *name)
{
	Creature *vict;

	if (!(vict = get_char_vis(ch, name))) {
		send_to_char(ch, "No-one by the name of '%s' around.\r\n", name);
	}
	return (vict);
}

int
boot_quests(void)
{
	if (!(qlogfile = fopen(QLOGFILENAME, "a"))) {
		slog("SYSERR: unable to open qlogfile.");
		safe_exit(1);
	}
	quests.loadQuests();
	qlog(NULL, "Quests REBOOTED.", QLOG_OFF, 0, TRUE);
	return 1;
}

int
check_editors(Creature *ch, char **buffer)
{
	struct descriptor_data *d = NULL;

	for (d = descriptor_list; d; d = d->next) {
		if (d->text_editor && d->text_editor->IsEditing(*buffer)) {
			send_to_char(ch, "%s is already editing that buffer.\r\n",
				d->creature ? PERS(d->creature, ch) : "BOGUSMAN");
			return 1;
		}
	}
	return 0;
}

ACMD(do_qlog)
{
	int i;

	skip_spaces(&argument);

	GET_QLOG_LEVEL(ch) = MIN(MAX(GET_QLOG_LEVEL(ch), 0), QLOG_COMP);

	if (!*argument) {
		send_to_char(ch, "You current qlog level is: %s.\r\n",
			qlog_types[(int)GET_QLOG_LEVEL(ch)]);
		return;
	}

	if ((i = search_block(argument, qlog_types, FALSE)) < 0) {
		buf[0] = '\0';
		for (i = 0;*qlog_types[i] != '\n';i++) {
			strcat(buf, qlog_types[i]);
			strcat(buf, " ");
		}
		send_to_char(ch, "Unknown qlog type '%s'.  Options are: %s\r\n",
			argument, buf);
		return;
	}

	GET_QLOG_LEVEL(ch) = i;
	send_to_char(ch, "Qlog level set to: %s.\r\n", qlog_types[i]);
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

char *quest_commands[][2] = {
	{"list", "shows currently active quests"},
	{"info", "get info about a specific quest"},
	{"join", "join an active quest"},
	{"leave", "leave a quest"},


	// Go back and set this in the player file when you get time!
	{"status", "list the quests you are participating in"},
	{"who", "list all players in a specific quest"},
	{"current", "specify which quest you are currently active in"},
	{"ignore", "ignore most qsays from specified quest."},
	{"\n", "\n"}
	// "\n" terminator must be here
};

ACMD(do_quest)
{
	int i = 0;

	if (IS_NPC(ch))
		return;

	skip_spaces(&argument);
	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, "Quest options:\r\n");

		while (1) {
			if (*quest_commands[i][0] == '\n')
				break;
			send_to_char(ch, "  %-10s -- %s\r\n",
							 quest_commands[i][0], 
							 quest_commands[i][1]);
			i++;
		}

		return;
	}

	for (i = 0;; i++) {
		if (*quest_commands[i][0] == '\n') {
			send_to_char(ch,
				"No such quest option, '%s'.  Type 'quest' for usage.\r\n",
				arg1);
			return;
		}
		if (is_abbrev(arg1, quest_commands[i][0]))
			break;
	}

	switch (i) {
	case 0:					// list
		do_quest_list(ch);
		break;
	case 1:					// info
		do_quest_info(ch, argument);
		break;
	case 2:					// join
		do_quest_join(ch, argument);
		break;
	case 3:					// leave
		do_quest_leave(ch, argument);
		break;
	case 4:					// status
		do_quest_status(ch, argument);
		break;
	case 5:					// who
		do_quest_who(ch, argument);
		break;
	case 6:					// current
		do_quest_current(ch, argument);
		break;
	case 7:					// ignore
		do_quest_ignore(ch, argument);
		break;
	default:
		break;
	}
}

void
do_quest_list(Creature *ch)
{
	send_to_char(ch, "%s", list_active_quests(ch));
}

void
do_quest_join(Creature *ch, char *argument)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "Join which quest?\r\n");
		return;
	}

	if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	if (quest->isPlaying(GET_IDNUM(ch))) {
		send_to_char(ch, "You are already in that quest, fool.\r\n");
		return;
	}

	if (!quest->canJoin(ch))
		return;

	if (! quest->addPlayer(GET_IDNUM(ch))) {
		send_to_char(ch, "Error adding char to quest.\r\n");
		return;
	}

	GET_QUEST(ch) = quest->getVnum();
	ch->crashSave();

	sprintf(buf, "joined quest %d '%s'.", quest->getVnum(),quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "You have joined quest '%s'.\r\n", quest->name);

	sprintf(buf, "%s has joined the quest.", GET_NAME(ch));
	send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(ch), 0), QCOMM_ECHO);
}

void
do_quest_leave(Creature *ch, char *argument)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
			send_to_char(ch, "Leave which quest?\r\n");
			return;
		}
	}

	else if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	if (!quest->isPlaying(GET_IDNUM(ch))) {
		send_to_char(ch, "You are not in that quest, fool.\r\n");
		return;
	}

	if (!quest->canLeave(ch))
		return;

	if (!quest->removePlayer(GET_IDNUM(ch))) {
		send_to_char(ch, "Error removing char from quest.\r\n");
		return;
	}

	sprintf(buf, "left quest %d '%s'.", quest->getVnum(),quest->name);
	qlog(ch, buf, QLOG_COMP, 0, TRUE);

	send_to_char(ch, "You have left quest '%s'.\r\n", quest->name);

	sprintf(buf, "%s has left the quest.", GET_NAME(ch));
	send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(ch), 0), QCOMM_ECHO);
}

void
do_quest_info(Creature *ch, char *argument)
{
	Quest *quest = NULL;
	int timediff;
	char timestr_a[128];
	char *timestr_s;

	skip_spaces(&argument);

	if (!*argument) {
		if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
			send_to_char(ch, "Get info on which quest?\r\n");
			return;
		}
	} else if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	timediff = time(0) - quest->getStarted();
	sprintf(timestr_a, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

	time_t started = quest->getStarted();
	timestr_s = asctime(localtime(&started));
	*(timestr_s + strlen(timestr_s) - 1) = '\0';

	sprintf(buf,
		"Quest [%d] info:\r\n"
		"Owner:  %s\r\n"
		"Name:   %s\r\n"
		"Description:\r\n%s"
		"Updates:\r\n%s"
		"  Type:            %s\r\n"
		"  Started:         %s\r\n"
		"  Age:             %s\r\n"
		"  Min Level:   Gen %-2d, Level %2d\r\n"
		"  Max Level:   Gen %-2d, Level %2d\r\n"
		"  Num Players:     %d\r\n"
		"  Max Players:     %d\r\n",
		quest->getVnum(),
		playerIndex.getName(quest->getOwner()), quest->name,
		quest->description ? quest->description : "None.\r\n",
		quest->updates ? quest->updates : "None.\r\n",
		qtypes[(int)quest->type], timestr_s, timestr_a,
		quest->mingen, quest->minlevel,
		quest->maxgen, quest->maxlevel,
		quest->getNumPlayers(), quest->getMaxPlayers());
	page_string(ch->desc, buf);

}

void
do_quest_status(Creature *ch, char *argument)
{
	char timestr_a[128];
	int timediff;
	bool found = false;

	char *msg = "You are participating in the following quests:\r\n"
		"-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

	for( unsigned int i = 0; i < quests.size(); i++ ) {
		Quest *quest = &(quests[i]);
		if (quest->getEnded())
			continue;
		if( quest->isPlaying( GET_IDNUM(ch) ) ) {
			timediff = time(0) - quest->getStarted();
			snprintf(timestr_a,128, "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

			char *line = tmp_sprintf(" %s%3d  %-10s  %-8s  %-24s %6s    %d\r\n", 
									quest->getVnum()== GET_QUEST(ch) ? "*" : " ",
									quest->getVnum(), playerIndex.getName(quest->getOwner()),
									qtype_abbrevs[(int)quest->type],
									quest->name, timestr_a, quest->getNumPlayers());
			msg = tmp_strcat(msg,line,NULL);
			found = true;
		}
	}
	if (!found)
		msg = tmp_strcat(msg,"None.\r\n",NULL);
	page_string(ch->desc, msg);
}

void
do_quest_who(Creature *ch, char *argument)
{
	Quest *quest = NULL;
	skip_spaces(&argument);

	if (!*argument) {
		if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
			send_to_char(ch, "List the players for which quest?\r\n");
			return;
		}
	} else if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded()||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	if (QUEST_FLAGGED(quest, QUEST_NOWHO)) {
		send_to_char(ch, "Sorry, you cannot get a who listing for this quest.\r\n");
		return;
	}

	if (QUEST_FLAGGED(quest, QUEST_NO_OUTWHO) &&
	!quest->isPlaying(GET_IDNUM(ch))) {
		send_to_char(ch, 
			"Sorry, you cannot get a who listing from outside this quest.\r\n");
		return;
	}

	list_quest_players(ch, quest, NULL);

}

void
do_quest_current(Creature *ch, char *argument)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
			send_to_char(ch, "You are not current on any quests.\r\n");
			return;
		}
		send_to_char(ch, "You are current on quest %d, '%s'\r\n", quest->getVnum(),
			quest->name);
		return;
	}

	if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	if ( !quest->isPlaying(GET_IDNUM(ch))) {
		send_to_char(ch, "You are not even in that quest.\r\n");
		return;
	}

	GET_QUEST(ch) = quest->getVnum();
	ch->crashSave();

	send_to_char(ch, "Ok, you are now currently active in '%s'.\r\n", quest->name);
}

void
do_quest_ignore(Creature *ch, char *argument)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!*argument) {
		if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
			send_to_char(ch, "Ignore which quest?\r\n");
			return;
		}
	}

	else if (!(quest = find_quest(ch, argument)))
		return;

	if (quest->getEnded() ||
		(QUEST_FLAGGED(quest, QUEST_HIDE)
			&& !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		send_to_char(ch, "No such quest is running.\r\n");
		return;
	}

	if(! quest->isPlaying(GET_IDNUM(ch) ) ) {
		send_to_char(ch, "You are not even in that quest.\r\n");
		return;
	}
	
	
	quest->getPlayer(GET_IDNUM(ch)).toggleFlag(QP_IGNORE);

	send_to_char(ch, "Ok, you are %s ignoring '%s'.\r\n",
		quest->getPlayer(GET_IDNUM(ch)).isFlagged(QP_IGNORE) ? "now" : "no longer", 
		quest->name);
}




ACMD(do_qsay)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!(quest = quest_by_vnum(GET_QUEST(ch))) ||
			!quest->isPlaying(GET_IDNUM(ch)) ) {
		send_to_char(ch, "You are not currently active on any quest.\r\n");
		return;
	}

	if( quest->getPlayer(GET_IDNUM(ch)).isFlagged(QP_MUTE) ) {
		send_to_char(ch, "You have been quest-muted.\r\n");
		return;
	}

	if( quest->getPlayer(GET_IDNUM(ch)).isFlagged(QP_IGNORE) ) {
		send_to_char(ch, "You can't quest-say while ignoring the quest.\r\n");
		return;
	}

	if (!*argument) {
		send_to_char(ch, "Qsay what?\r\n");
		return;
	}

	send_to_quest(ch, argument, quest, 0, QCOMM_SAY);
}

ACMD(do_qecho)
{
	Quest *quest = NULL;

	skip_spaces(&argument);

	if (!(quest = quest_by_vnum(GET_QUEST(ch))) ||
		!quest->isPlaying(GET_IDNUM(ch)) ){
		send_to_char(ch, "You are not currently active on any quest.\r\n");
		return;
	}

	if( quest->getPlayer(GET_IDNUM(ch)).isFlagged(QP_MUTE)) {
		send_to_char(ch, "You have been quest-muted.\r\n");
		return;
	}

	if( quest->getPlayer(GET_IDNUM(ch)).isFlagged(QP_IGNORE)) {
		send_to_char(ch, "You can't quest-echo while ignoring the quest.\r\n");
		return;
	}

	if (!*argument) {
		send_to_char(ch, "Qecho what?\r\n");
		return;
	}

	send_to_quest(ch, argument, quest, 0, QCOMM_ECHO);
}

void
qp_reload(int sig)
{
	int x;
	struct Creature *immortal;
	int online = 0, offline = 0;

	immortal = new Creature(true);
	for (x = 0; x <= playerIndex.getTopIDNum(); ++x) {
		if (playerIndex.exists(x) && !get_char_in_world_by_idnum(x)) {
			immortal->clear();
			immortal->loadFromXML(x);

			if (GET_LEVEL(immortal) >= LVL_AMBASSADOR &&
					GET_QUEST_ALLOWANCE(immortal) > 0) {
				slog("QP_RELOAD: Reset %s to %d QPs from %d. ( file )",
					GET_NAME(immortal),
					GET_QUEST_ALLOWANCE(immortal),
					GET_QUEST_POINTS(immortal));

				GET_QUEST_POINTS(immortal) = GET_QUEST_ALLOWANCE(immortal);
				immortal->crashSave();
				offline++;
			}
		}
	}

	delete immortal;

	//
	// Check if the imm is logged on
	//
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		immortal = *cit;
		if (GET_LEVEL(immortal) >= LVL_AMBASSADOR && (!IS_NPC(immortal)
				&& GET_QUEST_ALLOWANCE(immortal) > 0)) {
			slog("QP_RELOAD: Reset %s to %d QPs from %d. ( online )",
				GET_NAME(immortal), GET_QUEST_ALLOWANCE(immortal),
				GET_QUEST_POINTS(immortal));

			GET_QUEST_POINTS(immortal) = GET_QUEST_ALLOWANCE(immortal);
			send_to_char(immortal, "Your quest points have been restored!\r\n");
			immortal->crashSave();
			online++;
		}
	}
	mudlog(LVL_GRGOD, NRM, true,
		"QP's have been reloaded.  %d offline and %d online reset.",
		offline, online);
}


void
do_qcontrol_award(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;
	Creature *vict = NULL;
	char arg3[MAX_INPUT_LENGTH];	// Awarded points
	int award;
	int idnum;


	argument = two_arguments(argument, arg1, arg2);
	argument = one_argument(argument, arg3);
	award = atoi(arg3);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1))) {
		return;
	}

	if (!quest->canEdit(ch)) {
		return;
	}

	if ((idnum = playerIndex.getID(arg2)) < 0) {
		send_to_char(ch, "There is no character named '%s'.\r\n", arg2);
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
		return;
	}

	if ((vict = get_char_in_world_by_idnum(idnum))) {
		if(! quest->isPlaying(idnum) ) {
			send_to_char(ch, "No such player in the quest.\r\n");
			return;
		}
		if (!vict->desc) {
			send_to_char(ch, 
				"You can't award quest points to a linkless player.\r\n");
			return;
		}
	}

	if ((award <= 0)) {
		send_to_char(ch, "The award must be greater than zero.\r\n");
		return;
	}

	if ((award > GET_QUEST_POINTS(ch))) {
		send_to_char(ch, "You do not have the required quest points.\r\n");
		return;
	}

	if (!(vict)) {
		send_to_char(ch, "No such player in the quest.\r\n");
		return;
	}

	if ((ch) && (vict)) {
		GET_QUEST_POINTS(ch) -= award;
		GET_QUEST_POINTS(vict) += award;
		quest->addAwarded(award);
		ch->crashSave();
		vict->crashSave();
		sprintf(buf, "awarded player %s %d qpoints.", GET_NAME(vict), award);
		qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
			TRUE);
		if (*argument) {
			sprintf(buf, "'s Award Comments: %s", argument);
			qlog(ch, buf, QLOG_COMP, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
				TRUE);
		}
	}

}
void
do_qcontrol_penalize(Creature *ch, char *argument, int com)
{
	Quest *quest = NULL;
	Creature *vict = NULL;
	char arg3[MAX_INPUT_LENGTH];	// Penalized points
	int penalty;
	int idnum;


	argument = two_arguments(argument, arg1, arg2);
	argument = one_argument(argument, arg3);
	penalty = atoi(arg3);

	if (!*arg1 || !*arg2) {
		do_qcontrol_usage(ch, com);
		return;
	}

	if (!(quest = find_quest(ch, arg1))) {
		return;
	}

	if (!quest->canEdit(ch)) {
		return;
	}

	if ((idnum = playerIndex.getID(arg2)) < 0) {
		send_to_char(ch, "There is no character named '%s'.\r\n", arg2);
		return;
	}

	if (quest->getEnded() ) {
		send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
		return;
	}

	if ((vict = get_char_in_world_by_idnum(idnum))) {
		if(! quest->isPlaying(idnum) ) {
			send_to_char(ch, "No such player in the quest.\r\n");
			return;
		}
	}

	if (!(vict)) {
		send_to_char(ch, "No such player in the quest.\r\n");
		return;
	}

	if ((penalty <= 0)) {
		send_to_char(ch, "The penalty must be greater than zero.\r\n");
		return;
	}

	if ((penalty > GET_QUEST_POINTS(vict))) {
		send_to_char(ch, "They do not have the required quest points.\r\n");
		return;
	}

	if ((ch) && (vict)) {
		GET_QUEST_POINTS(vict) -= penalty;
		GET_QUEST_POINTS(ch) += penalty;
		quest->addPenalized(penalty);
		vict->crashSave();
		ch->crashSave();
		send_to_char(vict, "%d of your quest points have been taken by %s!\r\n",
			penalty, GET_NAME(ch));
		send_to_char(ch, "%d quest points transferred from %s.\r\n", penalty,
			GET_NAME(vict));
		sprintf(buf, "penalized player %s %d qpoints.", GET_NAME(vict),
			penalty);
		qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
			TRUE);
		if (*argument) {
			sprintf(buf, "'s Penalty Comments: %s", argument);
			qlog(ch, buf, QLOG_COMP, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
				TRUE);
		}
	}
}
void 
do_qcontrol_save(Creature *ch, char *argument, int com)
{
	quests.save();
	send_to_char(ch,"Quests saved.\r\n");
}

void
save_quests() {
	quests.save();
}

char *
compose_qcomm_string(Creature *ch, Creature *vict, Quest * quest, int mode, char *str)
{
	if (mode == QCOMM_SAY && ch) {
		if (ch == vict) {
			return tmp_sprintf("%s %2d] You quest-say,%s '%s'\r\n",
								CCYEL_BLD(vict, C_NRM), quest->getVnum(), 
								CCNRM(vict, C_NRM), str);
		} else {
			return tmp_sprintf("%s %2d] %s quest-says,%s '%s'\r\n",
								CCYEL_BLD(vict, C_NRM), quest->getVnum(),
								PERS(ch, vict), CCNRM(vict, C_NRM), str);
		}
	} else {// quest echo
		return tmp_sprintf("%s %2d] %s%s\r\n", 
							CCYEL_BLD(vict, C_NRM), quest->getVnum(), 
							str, CCNRM(vict, C_NRM));
	}
}
void
send_to_quest(Creature *ch, char *str, Quest * quest, int level, int mode)
{
	struct Creature *vict = NULL;
	int i;

	for (i = 0; i < quest->getNumPlayers(); i++) {
		if (quest->getPlayer(i).isFlagged(QP_IGNORE) && (level < LVL_AMBASSADOR))
			continue;

		if ((vict = get_char_in_world_by_idnum(quest->getPlayer(i).idnum))) {
			if (!PLR_FLAGGED(vict, PLR_MAILING | PLR_WRITING | PLR_OLC) &&
			vict->desc && GET_LEVEL(vict) >= level) 
			{
				send_to_char(vict, compose_qcomm_string(ch, vict, quest, mode, str) );
			}
		}
	}
}

Quest::Quest( Creature *ch, int type, const char* name ) 
	: players(), bans()
{
	this->vnum = quests.getNextVnum();
	this->type = type;
	this->name = str_dup(name);
	this->owner_id = GET_IDNUM(ch);
	this->owner_level = GET_LEVEL(ch);

	flags = QUEST_HIDE;
	started = time(0);
	ended = 0;

	description = (char*) malloc(sizeof(char) * 2);
	description[0] = ' ';
	description[1] = '\0';

	updates = (char*) malloc(sizeof(char) * 2);
	updates[0] = ' ';
	updates[1] = '\0';

	max_players = 0;
	awarded = 0;
	penalized = 0;
	minlevel = 0;
	maxlevel = 49;
	mingen = 0;
	maxgen = 10;
}

Quest::~Quest() 
{
	clearDescs();
}
void Quest::clearDescs() {
	if( name != NULL ) {
		free(name);
		name = NULL;
	}
	if( description != NULL ) {
		free(description);
		description = NULL;
	}
	if( updates != NULL ) {
		free(updates);
		updates = NULL;
	}
}

Quest::Quest( const Quest &q ) : players(), bans() 
{
	name = description = updates = NULL;
	*this = q;
}

Quest::Quest( xmlNodePtr n, xmlDocPtr doc )
{
	xmlChar *s;
	vnum = xmlGetIntProp(n, "VNUM");
	quests.setNextVnum( vnum );
	owner_id = xmlGetLongProp(n, "OWNER");
	started = (time_t) xmlGetLongProp(n, "STARTED");
	ended = (time_t) xmlGetLongProp(n, "ENDED");
	max_players = xmlGetIntProp(n, "MAX_PLAYERS");
	maxlevel = xmlGetIntProp(n, "MAX_LEVEL");
	minlevel = xmlGetIntProp(n, "MIN_LEVEL");
	maxgen = xmlGetIntProp(n, "MAX_GEN");
	mingen = xmlGetIntProp(n, "MIN_GEN");
	awarded = xmlGetIntProp(n, "AWARDED");
	penalized = xmlGetIntProp(n, "PENALIZED");
	owner_level = xmlGetIntProp(n, "OWNER_LEVEL");
	flags = xmlGetIntProp(n, "FLAGS");

	char *typest = xmlGetProp(n, "TYPE");
	type = search_block(typest, qtype_abbrevs, true);
	free(typest);

	name = xmlGetProp(n, "NAME");
	
	description = updates = NULL;

	xmlNodePtr cur = n->xmlChildrenNode;
	while (cur != NULL) {
		if ((xmlMatches(cur->name, "Description"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				description = (char *)s;
			}
		} else if ((xmlMatches(cur->name, "Update"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				updates = (char *)s;
			}
		} else if ((xmlMatches(cur->name, "Player"))) {
			long id = xmlGetLongProp(cur, "ID");
			int flags = xmlGetIntProp(cur, "FLAGS");
			qplayer_data player(id);
			player.setFlag(flags);
			players.push_back(player);
		} else if ((xmlMatches(cur->name, "Ban"))) {
			long id = xmlGetLongProp(cur, "ID");
			int flags = xmlGetIntProp(cur, "FLAGS");
			qplayer_data player(id);
			player.setFlag(flags);
			bans.push_back(player);
		}
		cur = cur->next;
	}
}

Quest& Quest::operator=( const Quest &q )
{
	clearDescs();

	vnum = q.vnum;
	type = q.type;
	name = str_dup(q.name);
	owner_id = q.owner_id;
	owner_level = q.owner_level;
	players = q.players;
	bans = q.bans;

	flags = q.flags;
	started = q.started;
	ended = q.ended;
	if( q.description != NULL )
		description = str_dup(q.description);
	if( q.updates != NULL )
		updates = str_dup(q.updates);
	max_players = q.max_players;
	awarded = q.awarded;
	penalized = q.penalized;
	minlevel = q.minlevel;
	maxlevel = q.maxlevel;
	maxgen = q.maxgen;
	mingen = q.mingen;
	return *this;
}

bool Quest::removePlayer( long id ) { 
	Creature *vict = NULL;
	vector<qplayer_data>::iterator it;

	it = find(players.begin(),players.end(),qplayer_data(id) );
	if( it == players.end() )
		return false;

	if (!(vict = get_char_in_world_by_idnum(id))) {
		// load the char from file
		vict = new Creature(true);
		if (vict->loadFromXML(id)) {
			//HERE
			GET_QUEST(vict) = 0;
			vict->saveToXML();
			delete vict;
		} else {
			//send_to_char(ch, "Error loading char from file.\r\n");
			slog("Error loading player id %ld from file for removal from quest %d.\r\n",
					id, vnum );
			return false;
		}
	} else {
		GET_QUEST(vict) = 0;
		vict->crashSave();
	}
	
	players.erase(it);
	return true;
}

bool Quest::removeBan( long id ) { 
	vector<qplayer_data>::iterator it;

	it = find(bans.begin(),bans.end(),qplayer_data(id) );
	if( it == bans.end() )
		return false;

	bans.erase(it);
	return true;
}


bool Quest::addBan( long id ) { 
	if( find(bans.begin(),bans.end(),qplayer_data(id) ) == bans.end() ){
		bans.push_back(qplayer_data(id)); 
		return true;
	} 
	return false;
}

bool Quest::isBanned( long id ) { 
	return find(bans.begin(),bans.end(),qplayer_data(id) ) != bans.end();
}

bool Quest::isPlaying( long id ) { 
	return find(players.begin(),players.end(),qplayer_data(id) ) != players.end();
}

qplayer_data &Quest::getPlayer( long id ) { 
	vector<qplayer_data>::iterator it;
	it = find(players.begin(),players.end(),qplayer_data(id) );
	return *it;
}

qplayer_data &Quest::getBan( long id ) { 
	vector<qplayer_data>::iterator it;
	it = find(bans.begin(),bans.end(),qplayer_data(id) );
	return *it;
}

bool Quest::addPlayer( long id ) { 
	if( find(players.begin(),players.end(),qplayer_data(id) ) == players.end() ){
		players.push_back(qplayer_data(id)); 
		return true;
	} 
	return false;
}


void qplayer_data::removeFlag( int flag ) {
	REMOVE_BIT(flags,flag);
}

void qplayer_data::toggleFlag( int flag ) {
	TOGGLE_BIT(flags,flag);
}

void qplayer_data::setFlag( int flag ) {
	SET_BIT(flags,flag);
}

bool qplayer_data::isFlagged( int flag ) {
	return IS_SET(flags, flag);
}

qplayer_data& qplayer_data::operator=( const qplayer_data &q )
{
	idnum = q.idnum; 
	flags = q.flags; 
	return *this;
}

bool Quest::canEdit(Creature *ch)
{
	if( Security::isMember(ch, "QuestorAdmin") )
		return true;

	if (GET_LEVEL(ch) <= owner_level &&
		GET_IDNUM(ch) != owner_id && GET_IDNUM(ch) != 1) {
		send_to_char(ch, "You cannot do that to this quest.\r\n");
		return false;
	}
	return true;
}
bool
Quest::canJoin(Creature *ch)
{
	if (QUEST_FLAGGED(this, QUEST_NOJOIN)) {
		send_to_char(ch, "This quest is open by invitation only.\r\n"
			"Contact the wizard in charge of the quest for an invitation.\r\n");
		return false;
	}

	if (!levelOK(ch))
		return false;

	if (isBanned(GET_IDNUM(ch))) {
		send_to_char(ch, "Sorry, you have been banned from this quest.\r\n");
		return false;
	}

	return true;
}

bool
Quest::canLeave(Creature *ch)
{
	if (QUEST_FLAGGED(this, QUEST_NOLEAVE)) {
		send_to_char(ch, "Sorry, you cannot leave the quest right now.\r\n");
		return false;
	}
	return true;
}

bool
Quest::levelOK(Creature *ch)
{
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
		return true;

	if (GET_REMORT_GEN(ch) > maxgen) {
		send_to_char(ch, "Your generation is too high for this quest.\r\n");
		return false;
	}

	if ( GET_LEVEL(ch) > maxlevel ) {
		send_to_char(ch, "Your level is too high for this quest.\r\n");
		return false;
	}
	if (GET_REMORT_GEN(ch) < mingen ) {
		send_to_char(ch, "Your generation is too low for this quest.\r\n");
		return false;
	}

	if ( GET_LEVEL(ch) < minlevel ) {
		send_to_char(ch, "Your level is too low for this quest.\r\n");
		return false;
	}

	return true;
}

void
Quest::save(std::ostream &out)
{
	const char *indent = "    ";
	
	out << indent << "<Quest VNUM=\"" << vnum << "\" NAME=\"" << xmlEncodeTmp(name) 
				  << "\" OWNER=\"" << owner_id << "\" STARTED=\"" << started
				  << "\" ENDED=\"" << ended << "\"" 
				  << endl;
	out << indent << indent 
				  << "MAX_PLAYERS=\"" << max_players << "\" MAX_LEVEL=\"" << maxlevel
				  << "\" MIN_LEVEL=\"" << minlevel << "\" MAX_GEN=\"" << maxgen 
				  << "\" MIN_GEN=\"" << mingen << "\"" << endl;
				  
	out << indent << indent 
				  << "AWARDED=\"" << awarded << "\""
				  << " PENALIZED=\"" << penalized 
				  << "\" TYPE=\"" << xmlEncodeTmp(tmp_strdup(qtype_abbrevs[type])) 
				  << "\" OWNER_LEVEL=\"" << owner_level << "\" FLAGS=\"" << flags 
				  << "\" >" << endl;

	if (description)
		out << indent << "  <Description>" 
					  << xmlEncodeTmp(description)
					  << "</Description>" << endl;
	
	out << indent << "  <Update>" << xmlEncodeTmp(updates) << "</Update>" << endl;

	for( unsigned int i = 0; i < players.size(); i++ ) {
		out << indent << "  <Player ID=\"" << players[i].idnum 
					  << "\" FLAGS=\"" << players[i].getFlags() << "\" />" << endl;
	}
	for( unsigned int i = 0; i < bans.size(); i++ ) {
		out << indent << "  <Ban ID=\"" << bans[i].idnum 
					  << "\" FLAGS=\"" << bans[i].getFlags() << "\" />" << endl;
	}
	out << indent << "</Quest>" << endl;
}
