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

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "quest.h"
#include "handler.h"
#include "screen.h"

// external funcs here
int find_name(char *name);
ACMD(do_switch);
// external vars here
extern struct char_data *character_list;
extern struct player_index_element *player_table; // index to plr file
extern int top_of_p_table;
extern struct descriptor_data *descriptor_list;

// internal funcs here
void do_qcontrol_help(struct char_data *ch, char *argument);
void do_qcontrol_switch( struct char_data *ch, char* argument, int com );
void do_qcontrol_oload_list(char_data *ch);
void do_qcontrol_oload(CHAR *ch, char *argument, int com);

// internal vars here

const struct qcontrol_option {
    char *keyword;
    char *usage;
    int level;
} qc_options[] = {
    { "show",     "[vnum]",                                     LVL_IMMORT },
    { "create",   "<type> <name>",                              LVL_IMMORT },
    { "end",      "<vnum>",                                     LVL_IMMORT },
    { "add",      "<name> <vnum>",                              LVL_IMMORT },
    { "kick",     "<player> <vnum>",                                   LVL_IMMORT }, // 5
    { "flags",    "<vnum> <+/-> <flags>",                       LVL_IMMORT },
    { "comment",  "<vnum> <comments>",                          LVL_IMMORT },
    { "desc",     "<vnum>",                                     LVL_IMMORT },
    { "update",   "<vnum>",                                     LVL_IMMORT },
    { "ban",      "<player> <vnum>",                            LVL_IMMORT }, // 10
    { "unban",    "<player> <vnum>",                            LVL_IMMORT },
    { "mute",     "<player> <vnum>",                            LVL_IMMORT },
    { "unmute",   "<player> <vnum>",                            LVL_IMMORT },
    { "level",    "<vnum> <access level>",                      LVL_IMMORT },
    { "minlev",   "<vnum> <minlev>",                            LVL_IMMORT }, // 15
    { "maxlev",   "<vnum> <maxlev>",                            LVL_IMMORT },
    { "award",    "<player> <vnum> <pts> [comments]",           LVL_IMMORT },
    { "penalize", "<player> <vnum> <pts> <reason>",             LVL_LUMINARY },
    { "load",     "<mobile vnum> <vnum>",                       LVL_POWER   },
    { "purge",    "<vnum> <mobile name>",                       LVL_POWER   }, // 20
    { "save",     "",                                           LVL_GRGOD  },
    { "help",     "<topic>",                                    LVL_IMMORT },
    { "switch",   "<mobile name>",                              LVL_IMMORT },
    { "rename",   "<obj name> <new obj name>",					LVL_POWER},
    { "oload",	  "<item num> <vnum>",							LVL_LUMINARY},
    { NULL, NULL, 0 }		// list terminator
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

quest_data *quests = NULL;
char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
int top_vnum = 0;
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
    for (com = 0;;com++) {
		if (!qc_options[com].keyword) {
			sprintf(buf, "Unknown qcontrol option, '%s'.\r\n", arg1);
			send_to_char(buf, ch);
			return;
		}
		if (is_abbrev(arg1, qc_options[com].keyword))
			break;
    }
  	if(qc_options[com].level > GET_LEVEL(ch)) {
		send_to_char("You are not godly enough to do this!\r\n",ch);
		return;
	}
    
	switch (com) {
    case 0:			// show
	do_qcontrol_show(ch, argument);
	break;
    case 1:			// create
	do_qcontrol_create(ch, argument, com);
	break;
    case 2:			// end
	do_qcontrol_end(ch, argument, com);
	break;
    case 3:			// add
	do_qcontrol_add(ch, argument, com);
	break;
    case 4:			// kick
	do_qcontrol_kick(ch, argument, com);
	break;
    case 5:			// flags
	do_qcontrol_flags(ch, argument, com);
	break;
    case 6:			// comment
	do_qcontrol_comment(ch, argument, com);
	break;
    case 7:			// desc
	do_qcontrol_desc(ch, argument, com);
	break;
    case 8:			// update
	do_qcontrol_update(ch, argument, com);
	break;
    case 9:			// ban
	do_qcontrol_ban(ch, argument, com);
	break;
    case 10:			// unban
	do_qcontrol_unban(ch, argument, com);
	break;
    case 11:			// mute
	do_qcontrol_mute(ch, argument, com);
	break;
    case 12:			// unnute
	do_qcontrol_unmute(ch, argument, com);
	break;
    case 13:			// level
	do_qcontrol_level(ch, argument, com);
	break;
    case 14:			// minlev
	do_qcontrol_minlev(ch, argument, com);
	break;
    case 15:			// maxlev
	do_qcontrol_maxlev(ch, argument, com);
	break;
    case 16:                    // award
	do_qcontrol_award( ch, argument, com);
	break;
    case 17:			// penalize
	do_qcontrol_penalize( ch, argument, com);
	break;
    case 18:			// Load Mobile
	do_qcontrol_load( ch, argument, com);
	break;
    case 19:			// Purge Mobile
	do_qcontrol_purge( ch, argument, com);
	break;
    case 21:			// help
	do_qcontrol_help(ch, argument);
	break;
    case 22:
	do_qcontrol_switch( ch, argument, com );
	break;
	case 23:			// rename
	//do_qcontrol_rename( ch, argument ,com );
	send_to_char("Not Implemented\r\n",ch);
	break;
	case 24:			// oload
	do_qcontrol_oload( ch, argument ,com );
	break;
	
    default:
	send_to_char("Sorry, this qcontrol option is not implemented.\r\n", ch);
	break;
    }
}   
void
do_qcontrol_help(struct char_data *ch, char *argument)
{
    int i;

    skip_spaces(&argument);
    if (!*argument) {
	send_to_char("Qcontrol help topics:\r\n"
		     "types\r\n"
		     "flags\r\n",
		     ch);
	return;
    }
    if (is_abbrev(argument, "types")) {
	strcpy(buf, "Quest types:\r\n");
	i = 0;
	while (1) {
	    if (*qtypes[i] == '\n')
		break;
	    sprintf(buf, "%s %2d. %s\r\n", buf, i++, qtypes[i]);
	}
	page_string(ch->desc, buf, 1);
	return;
    }
 
    else if (is_abbrev(argument, "flags")) {
	strcpy(buf, "Quest flags:\r\n");
	i = 0;
	while (1) {
	    if (*quest_bits[i] == '\n')
		break;
	    sprintf(buf, "%s %2d. %s\r\n", buf, i++, quest_bits[i]);
	}
	page_string(ch->desc, buf, 1);
	return;
    }
    
    send_to_char("No help on that topic.\r\n", ch);
}

void //Load mobile.
do_qcontrol_load    (CHAR *ch, char *argument, int com) {
    struct char_data *mob;
    struct quest_data *quest = NULL;
	char arg1[MAX_INPUT_LENGTH];
    int number;

	argument = two_arguments(argument,buf,arg1);
    
	if (!*buf || !isdigit(*buf) || !*arg1 || !isdigit(*arg1)) {
	do_qcontrol_usage(ch, com);
	return;
    }

	if ( !(quest = find_quest( ch, arg1) ) ){
		return;
	}
	if ( !quest_edit_ok( ch, quest) ){
		return;
	}
	if( quest->ended ){
		send_to_char( "Pay attentionu dummy! That quest is over!\r\n", ch);
		return;
	}

    if ((number = atoi(buf)) < 0) {
	send_to_char("A NEGATIVE number??\r\n", ch);
	return;
    }
    if (!real_mobile_proto(number)) {
	send_to_char("There is no mobile thang with that number.\r\n", ch);
	return;
    }
    mob = read_mobile(number);
    char_to_room(mob, ch->in_room);
	act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);

    sprintf(buf, "loaded %s at %d.", GET_NAME(mob), ch->in_room->number);
	qlog(ch,buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(ch),LVL_DEMI), TRUE);

}
void 
do_qcontrol_oload_list(char_data *ch) {
	int i=0;
	char main_buf[MAX_STRING_LENGTH];
	obj_data *obj;
	strcpy(main_buf,"Valid Quest Objects:\r\n");
	for(i = MIN_QUEST_OBJ_VNUM; i <= MAX_QUEST_OBJ_VNUM; i++) {
		if(!(obj = read_object(i)))
			continue;
		sprintf(buf,"    %s%d. %s%s %s: %d qps ",CCNRM(ch,C_NRM),
			i - MIN_QUEST_OBJ_VNUM, CCGRN(ch,C_NRM),obj->short_description,
			CCNRM(ch,C_NRM), (obj->shared->cost/100000));
		if(IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
			strcat(buf,"(!ap)\r\n");
		else
			strcat(buf,"\r\n");
		strcat(main_buf,buf);
		extract_obj(obj);
	}
	send_to_char(main_buf,ch);
}
// Load Quest Object
void
do_qcontrol_oload(CHAR *ch, char *argument, int com) {
    struct obj_data *obj;
    struct quest_data *quest = NULL;
    int number;
	char arg2[MAX_INPUT_LENGTH];

    argument = two_arguments(argument, buf, arg2);


    if (!*buf || !isdigit(*buf)) {
		do_qcontrol_oload_list(ch);
		do_qcontrol_usage(ch, com);
		return;
    }

	if ( !(quest = find_quest( ch, arg2) ) ){
		return;
	}
	if ( !quest_edit_ok( ch, quest) ){
		return;
	}
	if( quest->ended ){
		send_to_char( "Pay attentionu dummy! That quest is over!\r\n", ch);
		return;
	}

    if ((number = atoi(buf)) < 0) {
		send_to_char("A NEGATIVE number??\r\n", ch);
		return;
    }
	if(	number > MAX_QUEST_OBJ_VNUM - MIN_QUEST_OBJ_VNUM) {
		send_to_char("Invalid item number.\r\n",ch);
		do_qcontrol_oload_list(ch);
		return;
	}
	obj = read_object(number + MIN_QUEST_OBJ_VNUM);
	
	if(!obj) {
		send_to_char("Error, no object loaded\r\n",ch);
		return;
	}
	if(obj->shared->cost < 0) {
		send_to_char("This object is messed up.\r\n",ch);
		return;
	}

    if( ( (obj->shared->cost/100000) > GET_QUEST_POINTS( ch ) ) ){
		send_to_char( "You do not have the required quest points.\r\n", ch);
		return;
    }
     
	GET_QUEST_POINTS( ch ) -= (obj->shared->cost/100000);
	obj_to_char(obj,ch);
	save_char( ch, NULL );
	act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj,0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj,0, TO_CHAR);

    sprintf(buf, "loaded %s at %d.", obj->short_description, ch->in_room->number);
	qlog(ch,buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(ch),LVL_DEMI), TRUE);

}

void //Purge mobile.
do_qcontrol_purge   (CHAR *ch, char *argument, int com) {

    struct char_data *vict;
    struct quest_data *quest = NULL;
	char arg1[MAX_INPUT_LENGTH];


    argument = two_arguments(argument, arg1, buf);
	printf("arg1 %s\r\nbuf %s\r\n",arg1,buf);
	if (!*buf) {
		send_to_char("Purge what?\r\n",ch);
		return;
	}
	if ( !(quest = find_quest( ch, arg1) ) ){
		return;
	}
	if ( !quest_edit_ok( ch, quest) ){
		return;
	}
	if( quest->ended ){
		send_to_char( "Pay attentionu dummy! That quest is over!\r\n", ch);
		return;
	}
	
	if ((vict = get_char_room_vis(ch, buf))) {
        if (!IS_NPC(vict)) {
			send_to_char("You don't need a quest to purge them!\r\n", ch);
			return;
        }
        act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		sprintf(buf, "has purged %s at %d.",
		GET_NAME(vict), vict->in_room->number);
		qlog(ch,buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(ch),LVL_DEMI), TRUE);
		if (vict->desc) {
			close_socket(vict->desc);
			vict->desc = NULL;
		}
		extract_char(vict, TRUE);
		send_to_char(OK, ch);
	} else {
		send_to_char("Purge what?\r\n",ch);
		return;
	}

}


void
do_qcontrol_show(CHAR *ch, char *argument)
{

    int timediff;
    quest_data *quest = NULL;
    char *timestr_e, *timestr_s;
    char timestr_a[16];

    // show all quests
    if (!*argument) {

	list_active_quests(ch, buf);
	list_inactive_quests(ch, buf2);
	strcat(buf, buf2);

	page_string(ch->desc, buf, 1);
	return;
    }


    // list q specific quest
    if (!(quest = find_quest(ch, argument)))
	return;

    timestr_s = asctime(localtime(&quest->started));
    *(timestr_s + strlen(timestr_s) - 1) = '\0';
  

    // quest is over, show summary information
    if (quest->ended) {

	timediff = quest->ended - quest->started;
	sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);
    
	timestr_e = asctime(localtime(&quest->started));
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
		"  Min Level:   Gen %-2d, Level %2d  (%d)\r\n"
		"  Max Level:   Gen %-2d, Level %2d  (%d)\r\n"
		"  Max Players:    %d\r\n"
		"  Pts. Awarded:   %d\r\n",
		get_name_by_id(quest->owner_id), quest->owner_level, 
		quest->name,
		quest->description ? quest->description : "None.\r\n",
		qtypes[(int)quest->type], timestr_s, 
		timestr_e, timestr_a,
		quest->minlev/50, quest->minlev%50, quest->minlev,
		quest->maxlev/50, quest->maxlev%50, quest->maxlev,
		quest->max_players, quest->awarded);
	page_string(ch->desc, buf, 1);
	return;

    }

    // quest is still active

    timediff = time(0) - quest->started;
    sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);
  
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
	    "  Min Level:   Gen %-2d, Level %2d  (%d)\r\n"
	    "  Max Level:   Gen %-2d, Level %2d  (%d)\r\n"
	    "  Num Players:     %d\r\n"
	    "  Max Players:     %d\r\n"
	    "  Pts. Awarded:    %d\r\n",
	    get_name_by_id(quest->owner_id), quest->owner_level,
	    quest->name, 
	    quest->description ? quest->description : "None.\r\n",
	    quest->updates ? quest->updates : "None.\r\n",
	    qtypes[(int)quest->type], buf2, timestr_s, 
	    timestr_a,
	    quest->minlev/50, quest->minlev%50, quest->minlev,
	    quest->maxlev/50, quest->maxlev%50, quest->maxlev,
	    quest->num_players, quest->max_players, quest->awarded);

    if (quest->num_players) {
	list_quest_players(ch, quest, buf2);
	strcat(buf, buf2);
    }
  
    if (quest->num_bans) {
	list_quest_bans(ch, quest, buf2);
	strcat(buf, buf2);
    }

    page_string(ch->desc, buf, 1);
  
}

void
do_qcontrol_options(CHAR *ch)
{
    int i = 0;

    strcpy(buf, "qcontrol options:\r\n");
    while (1) {
	if (!qc_options[i].keyword)
	    break;
	sprintf(buf, "%s  %-15s %s\r\n", buf, qc_options[i].keyword, qc_options[i].usage);
	i++;
    }
    page_string(ch->desc, buf, 1);
}

void
do_qcontrol_usage(CHAR *ch, int com)
{
    if (com < 0)
	do_qcontrol_options(ch);
    else {
	sprintf(buf, "Usage: qcontrol %s %s\r\n", 
		qc_options[com].keyword, qc_options[com].usage);
	send_to_char(buf, ch);
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
do_qcontrol_create(CHAR *ch, char *argument, int com)
{
    int type;
    quest_data *quest = NULL;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*arg1 || !*argument) {
	do_qcontrol_usage(ch, com);
	return;
    }
  
    if ((type = find_quest_type(arg1)) < 0) {
	sprintf(buf, 
		"Invalid quest type '%s'.\r\n"
		"Use 'qcontrol help types'.\r\n", arg1);
	send_to_char(buf, ch);
	return;
    }

    if (strlen(argument) >= MAX_QUEST_NAME) {
	sprintf(buf, "Quest name too long.  Max length %d characters.\r\n",
		MAX_QUEST_NAME-1);
	send_to_char(buf, ch);
	return;
    }

    quest = create_quest(ch, type, argument);
  
    quest->next = quests;
    quests = quest;

    sprintf(buf, "Quest %d created.\r\n", quest->vnum);
    send_to_char(buf, ch);
}
void
do_qcontrol_end(CHAR *ch, char *argument, int com)
{
    CHAR *vict;
    quest_data *quest = NULL;

    if (!*argument) {
	do_qcontrol_usage(ch, com);
	return;
    }
  
    if (!(quest = find_quest(ch, argument)))
	return;

    if (quest->ended) {
	send_to_char("That quest has already ended... duh.\r\n", ch);
	return;
    }

    send_to_quest(ch, "Quest ending...", quest, 0, QCOMM_ECHO);

    qlog(ch, "Purging players from quest...", QLOG_COMP, 0, TRUE);

    while ( quest->num_players ) {
        // Go back when you get time and make this set in the player file.
        vict = get_char_in_world_by_idnum(quest->players[0].idnum);
        if (vict) {
            if( GET_LEVEL(vict) < LVL_AMBASSADOR && PRF_FLAGGED(vict, PRF_QUEST)) {
                REMOVE_BIT(PRF_FLAGS(vict), PRF_QUEST);
            }
        }
        if ( ! remove_idnum_from_quest( quest->players[0].idnum, quest) ) {
            send_to_char("Error removing char from quest.\r\n", ch);
            break;
        }

    }
	
    quest->ended = time(0);
    sprintf(buf, "ended quest '%s'", quest->name);
    qlog(ch, buf, QLOG_BRIEF, 0, TRUE);
    send_to_char("Quest ended.\r\n", ch);
}

void
do_qcontrol_add(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;
    CHAR       *vict  = NULL;

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
	send_to_char("You cannot add mobiles to quests.\r\n", ch);
	return;
    }

    if (quest->ended) {
	send_to_char("That quest has already ended, you wacko.\r\n", ch);
	return;
    }

    if (idnum_in_quest(GET_IDNUM(vict), quest)) {
	send_to_char("That person is already part of this quest.\r\n", ch);
	return;
    }

    if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch) {
	send_to_char("You are not powerful enough to do this.\r\n", ch);
	return;
    }

    if (!quest_edit_ok(ch, quest))
	return;

    if (!add_idnum_to_quest(GET_IDNUM(vict), quest)) {
	send_to_char("Error adding char to quest.\r\n", ch);
	return;
    }
    sprintf(buf, "added %s to quest '%s'.", GET_NAME(vict), quest->name);
    qlog(ch, buf, QLOG_COMP, GET_INVIS_LEV(vict), TRUE);

    sprintf(buf, "%s added to quest %d.\r\n", GET_NAME(vict), quest->vnum);
    send_to_char(buf, ch);

    sprintf(buf, "%s has added you to quest %d.\r\n", GET_NAME(ch), quest->vnum);
    send_to_char(buf, vict);

    sprintf(buf, "%s is now part of the quest.", GET_NAME(vict));
    send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LEV(vict),LVL_IMMORT), QCOMM_ECHO);
    
    if( GET_LEVEL(vict) < LVL_AMBASSADOR && !PRF_FLAGGED(vict, PRF_QUEST)) {
        SET_BIT(PRF_FLAGS(vict), PRF_QUEST);
    }
    
}

void
do_qcontrol_kick(CHAR *ch, char *argument, int com)
{

    quest_data *quest = NULL;
    CHAR       *vict  = NULL;
    unsigned int idnum;
    int         level = 0;
    struct char_file_u tmp_store;

    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2)))
        return;

    if ((idnum = get_id_by_name(arg1)) < 0) {
        send_to_char("There is no character named '%s'\r\n", ch);
        return;
    }
  
    if (quest->ended) {
        send_to_char("That quest has already ended.. there are no players in it.\r\n", ch);
        return;
    }

    if (!quest_edit_ok(ch, quest))
        return;

    if (!idnum_in_quest(idnum, quest)) {
        send_to_char("That person not participating in this quest.\r\n", ch);
        return;
    }

    if (!(vict = get_char_in_world_by_idnum(idnum))) {
        // load the char from file
        CREATE(vict, CHAR, 1);
        clear_char(vict);
        if (load_char(arg1, &tmp_store) > -1) {
            store_to_char(&tmp_store, vict);
            level = GET_LEVEL(vict);
            free_char(vict);
            vict = NULL;
        } else {
            send_to_char("Error loading char from file.\r\n", ch);
            return;
        }
    
    } else {
        level = GET_LEVEL(vict);
    }
  

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
	send_to_char("You are not powerful enough to do this.\r\n", ch);
	return;
    }

    if (!remove_idnum_from_quest(idnum, quest)) {
	send_to_char("Error removing char from quest.\r\n", ch);
	return;
    }


    sprintf(buf, "%s kicked from quest %d.\r\n", arg1, quest->vnum);
    send_to_char(buf, ch);
    if(vict) {
        sprintf(buf, "kicked %s from quest '%s'.", arg1, quest->name);
        qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(vict),LVL_IMMORT), TRUE);

        sprintf(buf, "%s kicked you from quest %d.\r\n", 
            GET_NAME(ch), quest->vnum);
        send_to_char(buf, vict);

        sprintf(buf, "%s has been kicked from the quest.", arg1);
        send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LEV(vict),LVL_IMMORT), 
            QCOMM_ECHO);
    } else {
        sprintf(buf, "kicked %s from quest '%s'.", arg1, quest->name);
        qlog(ch, buf, QLOG_BRIEF, LVL_IMMORT, TRUE);

        sprintf(buf, "%s has been kicked from the quest.", arg1);
        send_to_quest(NULL, buf, quest, LVL_IMMORT, QCOMM_ECHO);
    }


    // Go back and set this in the player file when you get time!
    if (vict) {
        if( GET_LEVEL(vict) < LVL_AMBASSADOR && PRF_FLAGGED(vict, PRF_QUEST)) {
            REMOVE_BIT(PRF_FLAGS(vict), PRF_QUEST);
        }
    }

}

void
do_qcontrol_flags(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;
    int state, cur_flags = 0, tmp_flags = 0, flag = 0, old_flags = 0;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !*argument) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg1)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if (*arg2 == '+')
	state = 1;
    else if (*arg2 == '-')
	state = 2;
    else {
	do_qcontrol_usage(ch, com);
	return;
    }
 
    argument = one_argument(argument, arg1);
  
    old_flags = cur_flags = quest->flags;
  
    while (*arg1) {
	if ((flag = search_block(arg1, quest_bits,FALSE)) == -1) {
	    sprintf(buf, "Invalid flag %s, skipping...\r\n", arg1);
	    send_to_char(buf, ch);
	}
	else 
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
	sprintf(buf, "Flags for quest %d not altered.\r\n", quest->vnum);
	send_to_char(buf, ch);
    }
    else {
	sprintf(buf, "[%s] flags %s for quest %d.\r\n", buf2, 
		state == 1 ? "added" : "removed", quest->vnum);
	send_to_char(buf, ch);

	sprintf(buf, "%s [%s] flags for quest '%s'.", 
		state == 1 ? "added" : "removed", buf2, quest->name);
	qlog(ch, buf, QLOG_COMP, LVL_IMMORT, TRUE);
    }
}  
 
void
do_qcontrol_comment(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*argument || !*arg1) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg1)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    sprintf(buf, "comment on quest '%s': %s", quest->name, argument);
    qlog(ch, buf, QLOG_NORM, LVL_IMMORT, TRUE);

}
  
void
do_qcontrol_desc(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    skip_spaces(&argument);
  
    if (!*argument) {
	do_qcontrol_usage(ch, com);
	return;
    }
  
    if (!(quest = find_quest(ch, argument)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if (check_editors(ch, &(quest->description)))
	return;


    act("$n begins to edit a quest description.\r\n",TRUE,ch,0,0,TO_ROOM);

    if (quest->description) {
        sprintf(buf, "began editing description of quest '%s'",quest->name);
    } else {
        sprintf(buf, "began writing description of quest '%s'",quest->name);
    }

    start_text_editor(ch->desc, &quest->description, true,MAX_QUEST_DESC);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}  

void
do_qcontrol_update(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    skip_spaces(&argument);
  
    if (!*argument) {
	do_qcontrol_usage(ch, com);
	return;
    }
  
    if (!(quest = find_quest(ch, argument)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if (check_editors(ch, &(quest->updates)))
	return;

    if (quest->description) {
        sprintf(buf, "began editing update of quest '%s'",quest->name);
    } else {
        sprintf(buf, "began writing the update of quest '%s'",quest->name);
    }

    start_text_editor(ch->desc,&quest->updates, true,MAX_QUEST_UPDATE);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
  
    act("$n begins to edit a quest update.\r\n",TRUE,ch,0,0,TO_ROOM);
    qlog(ch, buf, QLOG_COMP, LVL_IMMORT, TRUE);
}  
  
void
do_qcontrol_ban(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;
    CHAR       *vict  = NULL;
    unsigned int idnum;
    struct char_file_u tmp_store;
    int level = 0;

    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg2)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if ((idnum = get_id_by_name(arg1)) < 0) {
	send_to_char("There is no character named '%s'\r\n", ch);
	return;
    }
  
    if (quest->ended) {
	send_to_char("That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
  
    if (!(vict = get_char_in_world_by_idnum(idnum))) {
    
	// load the char from file
	CREATE(vict, CHAR, 1);
	clear_char(vict);
	if (load_char(arg1, &tmp_store) > -1) {
	    store_to_char(&tmp_store, vict);
	    level = GET_LEVEL(vict);
	    free_char(vict);
	    vict = NULL;
	}
	else {
	    send_to_char("Error loading char from file.\r\n", ch);
	    return;
	}
    
    } 
    else
	level = GET_LEVEL(vict);

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
	send_to_char("You are not powerful enough to do this.\r\n", ch);
	return;
    }

    if (idnum_banned_from_quest(idnum, quest)) {
	send_to_char("That character is already banned from this quest.\r\n", ch);
	return;
    }

    if (idnum_in_quest(idnum, quest)) {
	if (!remove_idnum_from_quest(idnum, quest)) {
	    send_to_char("Unable to auto-kick victim from quest!\r\n", ch);
	} else {
	    sprintf(buf, "%s auto-kicked from quest.\r\n", arg1);
	    send_to_char(buf, ch);
      
	    sprintf(buf, "auto-kicked %s from quest '%s'.", 
		    vict ? GET_NAME(vict) : arg1, quest->name);
	    qlog(ch, buf, QLOG_COMP, 0, TRUE);
	}
    }
  
    if (!ban_idnum_from_quest(idnum, quest)) {
	send_to_char("Error banning char from quest.\r\n", ch);
	return;
    }
  
    if (vict) {
	sprintf(buf, "You have been banned from quest '%s'.\r\n", quest->name);
	send_to_char(buf, ch);
    }

    sprintf(buf, "banned %s from quest '%s'.",
	    vict ? GET_NAME(vict) : arg1, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);
  
    sprintf(buf, "%s banned from quest %d.\r\n", 
	    vict ? GET_NAME(vict) : arg1, quest->vnum);
    send_to_char(buf, ch);
  
}

void 
do_qcontrol_unban(CHAR *ch, char *argument, int com)
{

    quest_data *quest = NULL;
    CHAR       *vict  = NULL;
    unsigned int idnum;
    struct char_file_u tmp_store;
    int level = 0;

    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg2)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if ((idnum = get_id_by_name(arg1)) < 0) {
	send_to_char("There is no character named '%s'\r\n", ch);
	return;
    }
  
    if (quest->ended) {
	send_to_char("That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
  
    if (!(vict = get_char_in_world_by_idnum(idnum))) {
    
	// load the char from file
	CREATE(vict, CHAR, 1);
	clear_char(vict);
	if (load_char(arg1, &tmp_store) > -1) {
	    store_to_char(&tmp_store, vict);
	    level = GET_LEVEL(vict);
	    free_char(vict);
	    vict = NULL;
	}
	else {
	    send_to_char("Error loading char from file.\r\n", ch);
	    return;
	}
    
    } 
    else
	level = GET_LEVEL(vict);

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
	send_to_char("You are not powerful enough to do this.\r\n", ch);
	return;
    }

    if (!idnum_banned_from_quest(idnum, quest)) {
	send_to_char("That player is not banned... maybe you should ban him!\r\n",ch);
	return;
    }

    if (!unban_idnum_from_quest(idnum, quest)) {
	send_to_char("Error unbanning char from quest.\r\n", ch);
	return;
    }

    sprintf(buf, "unbanned %s from quest '%s'.",
	    vict ? GET_NAME(vict) : arg1, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);
  
    sprintf(buf, "%s unbanned from quest %d.\r\n", 
	    vict ? GET_NAME(vict) : arg1, quest->vnum);
    send_to_char(buf, ch);

}
void
do_qcontrol_level(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*argument || !*arg1) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg1)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    quest->owner_level = atoi(arg2);
  
    sprintf(buf, "set quest '%s' access level to %d", 
	    quest->name, quest->owner_level);
    qlog(ch, buf, QLOG_NORM, LVL_IMMORT, TRUE);
  
}
void
do_qcontrol_minlev(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg1)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    quest->minlev = MAX(0, atoi(arg2));
  
    sprintf(buf, "set quest '%s' minimum level to gen %-2d level %2d", 
	    quest->name, quest->minlev/50, quest->minlev%50);
    qlog(ch, buf, QLOG_NORM, LVL_IMMORT, TRUE);
  
}
void
do_qcontrol_maxlev(CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg1)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    quest->maxlev = MAX(0, atoi(arg2));
  
    sprintf(buf, "set quest '%s' maximum level to gen %-2d level %2d", 
	    quest->name, quest->maxlev/50, quest->maxlev%50);
    qlog(ch, buf, QLOG_NORM, LVL_IMMORT, TRUE);
  
}

void 
do_qcontrol_mute(CHAR *ch, char *argument, int com)
{

    quest_data *quest = NULL;
    unsigned int idnum;
    qplayer_data *qp = NULL;

    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg2)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if ((idnum = get_id_by_name(arg1)) < 0) {
	send_to_char("There is no character named '%s'\r\n", ch);
	return;
    }
  
    if (quest->ended) {
	send_to_char("That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
  
    if (!(qp = idnum_in_quest(idnum, quest))) {
	send_to_char("That player is not in the quest.\r\n", ch);
	return;
    }

    if (IS_SET(qp->flags, QP_MUTE)) {
	send_to_char("That player is already muted.\r\n", ch);
	return;
    }

    SET_BIT(qp->flags, QP_MUTE);

    sprintf(buf, "muted %s in quest '%s'.", arg1, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);
  
    sprintf(buf, "%s muted for quest %d.\r\n", arg1, quest->vnum);
    send_to_char(buf, ch);

}


void 
do_qcontrol_unmute(CHAR *ch, char *argument, int com)
{

    quest_data *quest = NULL;
    unsigned int idnum;
    qplayer_data *qp = NULL;

    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
	do_qcontrol_usage(ch, com);
	return;
    }

    if (!(quest = find_quest(ch, arg2)))
	return;

    if (!quest_edit_ok(ch, quest))
	return;

    if ((idnum = get_id_by_name(arg1)) < 0) {
	send_to_char("There is no character named '%s'\r\n", ch);
	return;
    }
  
    if (quest->ended) {
	send_to_char("That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
  
    if (!(qp = idnum_in_quest(idnum, quest))) {
	send_to_char("That player is not in the quest.\r\n", ch);
	return;
    }

    if (!IS_SET(qp->flags, QP_MUTE)) {
	send_to_char("That player not muted.\r\n", ch);
	return;
    }

    REMOVE_BIT(qp->flags, QP_MUTE);

    sprintf(buf, "unmuted %s in quest '%s'.", arg1, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);
  
    sprintf(buf, "%s unmuted for quest %d.\r\n", arg1, quest->vnum);
    send_to_char(buf, ch);

}

void 
do_qcontrol_switch( CHAR *ch, char* argument, int com )
{
    struct quest_data *quest = NULL;
    struct qplayer_data *qp = NULL;
    

    if ( ( ! ( quest = quest_by_vnum( GET_QUEST( ch ) ) ) ||
	   ! ( qp = idnum_in_quest( GET_IDNUM( ch ), quest ) ) ) && ( GET_LEVEL( ch ) < LVL_ELEMENT ) ) {
	send_to_char( "You are not currently active on any quest.\r\n", ch );
	return;
    }
    do_switch( ch, argument, 0, SCMD_QSWITCH );
}



/*************************************************************************
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * UTILITY FUNCTIONS                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *************************************************************************/

qplayer_data *
idnum_in_quest(unsigned int idnum, quest_data *quest)
{
    int i;

    for (i = 0; i < quest->num_players; i++)
	if (quest->players[i].idnum == idnum)
	    return (&quest->players[i]);

    return NULL;
}
int
add_idnum_to_quest(unsigned int idnum, quest_data *quest)
{
  
    //  unsigned int *
    qplayer_data *newplayers = NULL;

    // we need a bigger array
    if (quest->max_players <= quest->num_players) {
	if (!(newplayers = (qplayer_data *) realloc(quest->players, 
				   sizeof(qplayer_data)*(quest->num_players+1)))) {
	    slog("SYSERR: Error allocating new player array.\r\n");
	    return 0;
	}
	quest->players = newplayers;
	quest->max_players++;
    }

    quest->players[quest->num_players].idnum = idnum;
    quest->players[quest->num_players].flags = 0;

    quest->num_players++;
  
    return (quest->num_players);
}

int
remove_idnum_from_quest(unsigned int idnum, quest_data *quest)
{
    int i;

    for (i = 0; i < quest->num_players; i++)
	if (quest->players[i].idnum == idnum)
	    break;
  
    if (i >= quest->num_players) {
	slog("SYSERR: error finding player idnum in remove_idnum_from_quest.");
	return 0;
    }
  
    for (++i; i < quest->num_players; i++) {
	quest->players[i-1].idnum = quest->players[i].idnum;
	quest->players[i-1].flags = quest->players[i].flags;
    }
  
    quest->num_players--;

    return 1;
}

qplayer_data *
idnum_banned_from_quest(unsigned int idnum, quest_data *quest)
{
    int i;

    for (i = 0; i < quest->num_bans; i++)
	if (quest->bans[i].idnum == idnum)
	    return (&quest->bans[i]);

    return NULL;
}

int 
ban_idnum_from_quest(unsigned int idnum, quest_data *quest)
{
  
    qplayer_data *newbans = NULL;

    if (!(newbans = (qplayer_data *) realloc(quest->bans, 
			    sizeof(qplayer_data)*(quest->num_bans+1)))) {
	slog("SYSERR: Error allocating new bans array.\r\n");
	return 0;
    }

    quest->bans = newbans;

    quest->bans[quest->num_bans].idnum = idnum;
    quest->bans[quest->num_bans].flags = 0;

    quest->num_bans++;
  
    return (quest->num_bans);
}


int
unban_idnum_from_quest(unsigned int idnum, quest_data *quest)
{
    int i;

    for (i = 0; i < quest->num_bans; i++)
	if (quest->bans[i].idnum == idnum)
	    break;
  
    if (i >= quest->num_bans) {
	slog("SYSERR: error finding player idnum in unban_idnum_from_quest.");
	return 0;
    }
  
    for (++i; i < quest->num_bans; i++) {
	quest->bans[i-1].idnum = quest->bans[i].idnum;
	quest->bans[i-1].flags = quest->bans[i].flags;
    }
  
    quest->num_bans--;

    return 1;
}

int
quest_edit_ok(CHAR *ch, quest_data *quest)
{
    if (GET_LEVEL(ch) <= quest->owner_level &&
	GET_IDNUM(ch) != quest->owner_id && GET_IDNUM(ch) != 1) {
	send_to_char("You cannot do that to this quest.\r\n", ch);
	return 0;
    }
    return 1;
}
  

quest_data *
create_quest(CHAR *ch, int type, char *name)
{
    quest_data *quest = NULL;

    CREATE(quest, quest_data, 1);
  
    top_vnum++;
    quest->vnum          = top_vnum;
    quest->owner_id      = GET_IDNUM(ch);
    quest->owner_level   = GET_LEVEL(ch);
    quest->flags         = QUEST_HIDE;
    quest->started       = time(0);
    quest->ended         = 0;
    quest->name          = str_dup(name);
    quest->description   = NULL;
    quest->updates       = NULL;
    quest->players       = NULL;
    quest->bans          = NULL;
    quest->num_bans      = 0;
    quest->type          = type;
    quest->num_players   = 0;
    quest->max_players   = 0;
    quest->awarded       = 0;
    quest->penalized     = 0;
    quest->kicked        = 0;
    quest->banned        = 0;
    quest->loaded        = 0;
    quest->purged        = 0;
    quest->minlev        = 0;
    quest->maxlev        = 549;

    quest->next          = NULL;

    sprintf(buf, "created quest type %s, '%s'", qtypes[type], name);
    qlog(ch, buf, QLOG_BRIEF, LVL_IMMORT, TRUE);


    return quest;
}

/*************************************************************************
 * function to find a quest                                              *
 * argument is the vnum of the quest as a string                         *
 *************************************************************************/
quest_data *
find_quest(CHAR *ch, char *argument)
{
    int vnum;
    quest_data *quest = NULL;

    vnum = atoi(argument);


    if ((quest = quest_by_vnum(vnum)))
	return quest;

  
    sprintf(buf, "There is no quest number %d.\r\n", vnum);
    send_to_char(buf, ch);

    return NULL;

}

/*************************************************************************
 * low level function to return a quest                                  *
 *************************************************************************/
quest_data *
quest_by_vnum(int vnum)
{
    quest_data *quest = NULL;

    for (quest = quests; quest; quest = quest->next)
	if (quest->vnum == vnum)
	    return quest;

    return NULL;
}
/*************************************************************************
 * function to list active quests to both mortals and gods               *
 * if outbuf is NULL, the list is sent to the character, otherwise the   *
 * list is copied to outbuf                                              *
 *************************************************************************/

void
list_active_quests(CHAR *ch, char *outbuf)
{  
    char buf[MAX_STRING_LENGTH];
    quest_data *quest = NULL;
    int i;
    int timediff;
    char timestr_a[16];

    strcpy(buf, 
	   "Active quests:\r\n"
	   "-Vnum--Owner-------Type------Name----------------------Age------Players\r\n");
    for (i = 0, quest = quests; quest; quest = quest->next) {
	if (quest->ended)
	    continue;

	if (QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	    continue;

	timediff = time(0) - quest->started;
	sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);

	sprintf(buf, "%s  %3d  %-10s  %-8s  %-24s %6s    %d\r\n", buf,
		quest->vnum,
		get_name_by_id(quest->owner_id), qtype_abbrevs[(int)quest->type],
		quest->name, timestr_a, quest->num_players);
	i++;
    }
    sprintf(buf, "%s%d visible quest%s active.\r\n\r\n", buf, 
	    i, i == 1 ? "" : "s");
  
    if (outbuf)
	strcpy(outbuf, buf);
    else
	page_string(ch->desc, buf, 1);
}

void
list_inactive_quests(CHAR *ch, char *outbuf)
{
    char buf[MAX_STRING_LENGTH];
    quest_data *quest = NULL;
    int i;
    int timediff;
    char timestr_a[16];

    strcpy(buf, 
	   "Finished quests:\r\n"
	   "-Vnum--Owner-------Type------Name----------------------Run------Max Plrs\r\n");
  
    for (i = 0, quest = quests; quest; quest = quest->next) {
	if (!quest->ended)
	    continue;
    
	timediff = quest->ended - quest->started;
	sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);
    
	sprintf(buf, "%s  %3d  %-10s  %-8s  %-24s %6s    %d\r\n", buf,
		quest->vnum,
		get_name_by_id(quest->owner_id), qtype_abbrevs[(int)quest->type],
		quest->name, timestr_a, quest->max_players);
	i++;
    }
    sprintf(buf, "%s%d quests finished.\r\n", buf, i);

    if (outbuf)
	strcpy(outbuf, buf);
    else
	page_string(ch->desc, buf, 1);

}

void
list_quest_players(CHAR *ch, quest_data *quest, char *outbuf)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], name[128];
    int i, num_online, num_offline;
    CHAR *vict = NULL;
    char bitbuf[1024];

    strcpy(buf, "  -Online Players------------------------------------\r\n");
    strcpy(buf2, "  -Offline Players-----------------------------------\r\n");

    for (i = num_online = num_offline = 0; i < quest->num_players; i++) {

	sprintf(name, "%s", get_name_by_id(quest->players[i].idnum));
	if (!*name) {
	    strcat(buf, "BOGUS player idnum!\r\n");
	    strcat(buf2, "BOGUS player idnum!\r\n");
	    slog("SYSERR: bogus player idnum in list_quest_players.");
	    break;
	}
	strcpy(name, CAP(name));

	if (quest->players[i].flags)
	    sprintbit(quest->players[i].flags, qp_bits, bitbuf);
	else
	    strcpy(bitbuf, "");

	// player is in world and visible
	if ((vict = get_char_in_world_by_idnum(quest->players[i].idnum)) &&
	    CAN_SEE(ch, vict)) {
      
	    // see if we can see the locations of the players
	    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
		QUEST_FLAGGED(quest, QUEST_WHOWHERE)) {
		sprintf(buf, "%s  %2d. %-15s - %-10s - [%5d] %s %s\r\n", buf,
			++num_online, name,  bitbuf, vict->in_room->number,
			vict->in_room->name,
			vict->desc ? "" : "   (linkless)");
	    }
	    else
		sprintf(buf, "%s  %2d. %-15s - %-10s\r\n", buf, ++num_online,
			name, bitbuf);
     
	} 
	// player is either offline or invisible
	else if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	    sprintf(buf2, "%s  %2d. %-15s - %-10s\r\n", buf2, ++num_offline,name,bitbuf);
    }
  
    // only gods may see the offline players
    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	strcat(buf, buf2);

    if (outbuf)
	strcpy(outbuf, buf);
    else
	page_string(ch->desc, buf, 1);

}

void
list_quest_bans(CHAR *ch, quest_data *quest, char *outbuf)
{
    char buf[MAX_STRING_LENGTH], name[128];
    int i, num;

    strcpy(buf, "  -Banned Players------------------------------------\r\n");

    for (i = num = 0; i < quest->num_bans; i++) {

	sprintf(name, "%s", get_name_by_id(quest->bans[i].idnum));
	if (!*name) {
	    strcat(buf, "BOGUS player idnum!\r\n");
	    slog("SYSERR: bogus player idnum in list_quest_bans.");
	    break;
	}
	strcpy(name, CAP(name));
    
	sprintf(buf, "%s  %2d. %-20s\r\n", buf, ++num,name);
    }
  
    if (outbuf)
	strcpy(outbuf, buf);
    else
	page_string(ch->desc, buf, 1);

}

void 
qlog(CHAR *ch, char *str, int type, int level, int file)
{
    time_t ct;
    char *tmstr;
    CHAR *vict = NULL;
    char buf[MAX_STRING_LENGTH];

    if (type) {
	for (vict = character_list; vict; vict = vict->next) {
      
	    if (GET_LEVEL(vict) >= level &&
		GET_QLOG_LEVEL(vict) >= type) {
	
		sprintf(buf, 
			"%s[%s QLOG: %s %s %s]%s\r\n",
			CCYEL_BLD(vict, C_NRM), CCNRM_GRN(vict, C_NRM),
			ch ? PERS(ch, vict) : "",
			str, CCYEL_BLD(vict, C_NRM), CCNRM(vict, C_NRM));
		send_to_char(buf, vict);
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

CHAR *
check_char_vis(CHAR *ch, char *name)
{
    CHAR *vict;

    if (!(vict = get_char_vis(ch, name))) {
	sprintf(buf, "No-one by the name of '%s' around.\r\n", name);
	send_to_char(buf, ch);
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
  
    qlog(NULL, "Quests REBOOTED.", QLOG_OFF, 0, TRUE);
    return 1;
}

int
check_editors(CHAR *ch, char **buffer)
{
    struct descriptor_data *d = NULL;

    for (d = descriptor_list; d; d = d->next) {
	if (d->str == buffer) {
	    sprintf(buf, "%s is already editing that buffer.\r\n",
		    d->character ? PERS(d->character, ch) : "BOGUSMAN");
	    send_to_char(buf, ch);
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
	sprintf(buf, "You current qlog level is: %s.\r\n", 
		qlog_types[(int)GET_QLOG_LEVEL(ch)]);
	send_to_char(buf, ch);
	return;
    }

    if ((i = search_block(argument, qlog_types, FALSE)) < 0) {
	sprintf(buf, "Unknown qlog type '%s'.  Options are:\r\n", argument);
	i = 0;
	while (1) {
	    if (*qlog_types[i] == '\n')
		break;
	    strcat(buf, qlog_types[i]);
	    strcat(buf, "\n");
	    i++;
	}
	send_to_char(buf, ch);
	return;
    }
  
    GET_QLOG_LEVEL(ch) = i;
    sprintf(buf, "Qlog level set to: %s.\r\n", qlog_types[i]);
    send_to_char(buf, ch);
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

char *quest_commands[][2] = {
    { "list",   "shows currently active quests"},
    { "info",   "get info about a specific quest"},
    { "join",   "join an active quest"},
    { "leave",  "leave a quest"},


    // Go back and set this in the player file when you get time!
    { "status", "list the quests you are participating in"},
    { "who",    "list all players in a specific quest"},
    { "current","specify which quest you are currently active in"},
    { "ignore", "ignore most qsays from specified quest."},
    { "\n",     "\n"}
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
	strcpy(buf, "Quest options:\r\n");
    
	while (1) {
	    if (*quest_commands[i][0] == '\n')
		break;
	    sprintf(buf, "%s  %-10s -- %s\r\n", buf, 
		    quest_commands[i][0], quest_commands[i][1]);
	    i++;
	}
    
	send_to_char(buf, ch);
	return;
    }
  
    for (i = 0; ; i++) {
	if (*quest_commands[i][0] == '\n') {
	    sprintf(buf, "No such quest option, '%s'.  Type 'quest' for usage.\r\n",
		    arg1);
	    send_to_char(buf, ch);
	    return;
	}
	if (is_abbrev(arg1, quest_commands[i][0]))
	    break;
    }

    switch (i) {
    case 0:  // list
	do_quest_list(ch);
	break;
    case 1:  // info
	do_quest_info(ch, argument);
	break;
    case 2:  // join
	do_quest_join(ch, argument);
	break;
    case 3:  // leave
	do_quest_leave(ch, argument);
	break;
    case 4:  // status
	do_quest_status(ch, argument);
	break;
    case 5:  // who
	do_quest_who(ch, argument);
	break;
    case 6: // current
	do_quest_current(ch, argument);
	break;
    case 7: // ignore
	do_quest_ignore(ch, argument);
	break;
    default:
	break;
    }
}

void
do_quest_list(CHAR *ch)
{
    list_active_quests(ch, NULL);
}

void
do_quest_join(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
	send_to_char("Join which quest?\r\n", ch);
	return;
    }

    if (!(quest = find_quest(ch, argument)))
	return;

    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }

    if (idnum_in_quest(GET_IDNUM(ch), quest)) {
	send_to_char("You are already in that quest, fool.\r\n", ch);
	return;
    }

    if (!quest_join_ok(ch, quest))
	return;

    if (!add_idnum_to_quest(GET_IDNUM(ch), quest)) {
	send_to_char("Error adding char to quest.\r\n", ch);
	return;
    }

    GET_QUEST(ch) = quest->vnum;

    sprintf(buf, "joined quest '%s'.", quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);

    sprintf(buf, "You have joined quest '%s'.\r\n", quest->name);
    send_to_char(buf, ch);

    sprintf(buf, "%s has joined the quest.", GET_NAME(ch));
    send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LEV(ch),0), QCOMM_ECHO);
    if( GET_LEVEL(ch) < LVL_AMBASSADOR && !PRF_FLAGGED(ch, PRF_QUEST)) {
        SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
    }

}
void
do_quest_leave(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
	if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
	    send_to_char("Leave which quest?\r\n", ch);
	    return;
	}
    }

    else if (!(quest = find_quest(ch, argument)))
	return;

    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }

    if (!idnum_in_quest(GET_IDNUM(ch), quest)) {
	send_to_char("You are not in that quest, fool.\r\n", ch);
	return;
    }

    if (!quest_leave_ok(ch, quest))
	return;

    if (!remove_idnum_from_quest(GET_IDNUM(ch), quest)) {
	send_to_char("Error removing char from quest.\r\n", ch);
	return;
    }

    sprintf(buf, "left quest '%s'.", quest->name);
    qlog(ch, buf, QLOG_COMP, 0, TRUE);

    sprintf(buf, "You have left quest '%s'.\r\n", quest->name);
    send_to_char(buf, ch);

    sprintf(buf, "%s has left the quest.", GET_NAME(ch));
    send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LEV(ch),0), QCOMM_ECHO);

    if( GET_LEVEL(ch) < LVL_AMBASSADOR && PRF_FLAGGED(ch, PRF_QUEST)) {
        REMOVE_BIT(PRF_FLAGS(ch), PRF_QUEST);
    }

}

void 
do_quest_info(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;
    int timediff;
    char timestr_a[128];
    char *timestr_s;

    skip_spaces(&argument);
  
    if (!*argument) {
	if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
	    send_to_char("Get info on which quest?\r\n", ch);
	    return;
	}
    } 
    else if (!(quest = find_quest(ch, argument)))
	return;
  
    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }
 
    timediff = time(0) - quest->started;
    sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);

    timestr_s = asctime(localtime(&quest->started));
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
	    quest->vnum,
	    get_name_by_id(quest->owner_id), quest->name, 
	    quest->description ? quest->description : "None.\r\n",
	    quest->updates ? quest->updates : "None.\r\n",
	    qtypes[(int)quest->type], timestr_s, timestr_a,
	    quest->minlev/50, quest->minlev%50,
	    quest->maxlev/50, quest->maxlev%50,
	    quest->num_players, quest->max_players);
    page_string(ch->desc, buf, 1);

}

void 
do_quest_status(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;
    int i;
    char timestr_a[128];
    int timediff;
    int found = 0;

    strcpy(buf, 
	   "You are participating in the following quests:\r\n"
	   "-Vnum--Owner-------Type------Name----------------------Age------Players\r\n");
  
    for (quest = quests; quest; quest = quest->next) {

	if (quest->ended)
	    continue;
    
	for (i = 0; i < quest->num_players; i++) {
	    if (quest->players[i].idnum == (unsigned int) GET_IDNUM(ch)) {
	
		timediff = time(0) - quest->started;
		sprintf(timestr_a, "%02d:%02d", timediff/3600, (timediff/60)%60);
	
		sprintf(buf, "%s %s%3d  %-10s  %-8s  %-24s %6s    %d\r\n", buf,
			quest->vnum == GET_QUEST(ch) ? "*" : " ",
			quest->vnum,
			get_name_by_id(quest->owner_id), 
			qtype_abbrevs[(int)quest->type],
			quest->name, timestr_a, quest->num_players);
		found = 1;
		break;
	    }
	}
    }
    if (!found)
	strcat(buf, "None.\r\n");
    page_string(ch->desc, buf, 1);
}

void 
do_quest_who(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;
    skip_spaces(&argument);
  
    if (!*argument) {
	if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
	    send_to_char("List the players for which quest?\r\n", ch);
	    return;
	}
    } 
    else if (!(quest = find_quest(ch, argument)))
	return;
  
    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }

    if (QUEST_FLAGGED(quest, QUEST_NOWHO)) {
	send_to_char("Sorry, you cannot get a who listing for this quest.\r\n", ch);
	return;
    }
 
    if (QUEST_FLAGGED(quest, QUEST_NO_OUTWHO) &&
	!idnum_in_quest(GET_IDNUM(ch), quest)) {
	send_to_char("Sorry, you cannot get a who listing from outside this quest.\r\n", ch);
	return;
    }

    list_quest_players(ch, quest, NULL);

}

void
do_quest_current(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
	if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
	    send_to_char("You are not current on any quests.\r\n",ch);
	    return;
	}
	sprintf(buf, "You are current on quest %d, '%s'\r\n", quest->vnum,
		quest->name);
	send_to_char(buf, ch);
	return;
    }

    if (!(quest = find_quest(ch, argument)))
	return;

    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }

    if (!idnum_in_quest(GET_IDNUM(ch), quest)) {
	send_to_char("You are not even in that quest.\r\n", ch);
	return;
    }

    GET_QUEST(ch) = quest->vnum;
  
    sprintf(buf, "Ok, you are now currently active in '%s'.\r\n", quest->name);
    send_to_char(buf, ch);
}

void
do_quest_ignore(CHAR *ch, char *argument)
{
    quest_data *quest = NULL;
    qplayer_data *qp = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char("Ignore which quest?\r\n", ch);
            return;
        }
    }

    else if (!(quest = find_quest(ch, argument)))
	return;
  
    if (quest->ended || 
	(QUEST_FLAGGED(quest, QUEST_HIDE) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
	send_to_char("No such quest is running.\r\n", ch);
	return;
    }

    if (!(qp = idnum_in_quest(GET_IDNUM(ch), quest))) {
	send_to_char("You are not even in that quest.\r\n", ch);
	return;
    }

    TOGGLE_BIT(qp->flags, QP_IGNORE);
  
    sprintf(buf, "Ok, you are %s ignoring '%s'.\r\n", 
	    IS_SET(qp->flags, QP_IGNORE) ? "now" : "no longer", quest->name);
    send_to_char(buf, ch);
}

int
quest_join_ok(CHAR *ch, quest_data *quest)
{
    if (QUEST_FLAGGED(quest, QUEST_NOJOIN)) {
	send_to_char("This quest is open by invitation only.\r\n"
		     "Contact the wizard in charge of the quest for an invitation.\r\n", ch);
	return 0;
    }

    if (!quest_level_ok(ch, quest))
	return 0;
  
    if (idnum_banned_from_quest(GET_IDNUM(ch), quest)) {
	send_to_char("Sorry, you have been banned from this quest.\r\n", ch);
	return 0;
    }

    return 1;
}

int
quest_leave_ok(CHAR *ch, quest_data *quest)
{
    if (QUEST_FLAGGED(quest, QUEST_NOLEAVE)) {
	send_to_char("Sorry, you cannot leave the quest right now.\r\n", ch);
	return 0;
    }
    return 1;
}

int
quest_level_ok(CHAR *ch, quest_data *quest)
{
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
        return 1;
    if (GET_REMORT_GEN(ch)*50 + GET_LEVEL(ch) > quest->maxlev) {
        send_to_char("Your level is too high for this quest.\r\n", ch);
        return 0;
    }
    if (GET_REMORT_GEN(ch)*50 + GET_LEVEL(ch) < quest->minlev) {
        send_to_char("Your level is too low for this quest.\r\n", ch);
        return 0;
    }
    return 1;
}

void
send_to_quest(CHAR *ch, char *str, quest_data *quest, int level, int mode)
{
    struct char_data *vict = NULL;
    int i;
    char buf[MAX_STRING_LENGTH];

    for (i = 0; i < quest->num_players; i++) {
        if (QP_FLAGGED((quest->players+i), QP_IGNORE) && (level < LVL_IMMORT))
            continue;
    
        if ((vict = get_char_in_world_by_idnum(quest->players[i].idnum))) {
            if (!PLR_FLAGGED(vict, PLR_MAILING | PLR_WRITING | PLR_OLC) &&
            vict->desc &&
            (!vict->desc->showstr_point || 
             PRF2_FLAGGED(vict, PRF2_LIGHT_READ))
             && GET_LEVEL(vict) >= level) {
            compose_qcomm_string(ch, vict, quest, mode, str, buf);
            send_to_char(buf, vict);
            }
        }
    }
}

void
compose_qcomm_string(CHAR *ch, CHAR *vict, quest_data *quest,
		     int mode, char *str, char *outbuf)
{
  
    if (mode == QCOMM_SAY && ch) {
	if (ch == vict) {
	    sprintf(outbuf, "%s %2d] You quest-say,%s '%s'\r\n",
		    CCYEL_BLD(vict, C_NRM), quest->vnum, 
		    CCNRM(vict, C_NRM), str);
	} else {
	    sprintf(outbuf, "%s %2d] %s quest-says,%s '%s'\r\n",
		    CCYEL_BLD(vict, C_NRM), quest->vnum, 
		    PERS(ch, vict), CCNRM(vict, C_NRM), str);
	}	    
    } 
    // quest echo
    else {
	sprintf(outbuf, "%s %2d] %s%s\r\n",
		CCYEL_BLD(vict, C_NRM), quest->vnum, 
		str, CCNRM(vict, C_NRM));
    }
}

ACMD(do_qsay)
{
    quest_data *quest = NULL;
    qplayer_data *qp = NULL;

    skip_spaces(&argument);

    if (!(quest = quest_by_vnum(GET_QUEST(ch))) ||
        !(qp = idnum_in_quest(GET_IDNUM(ch), quest))) {
	send_to_char("You are not currently active on any quest.\r\n", ch);
	return;
    }

    if (QP_FLAGGED(qp, QP_MUTE)) {
	send_to_char("You have been quest-muted.\r\n", ch);
	return;
    }

    if (QP_FLAGGED(qp, QP_IGNORE)) {
	send_to_char("You can't quest-say while ignoring the quest.\r\n", ch);
	return;
    }

    if (!*argument) {
	send_to_char("Qsay what?\r\n", ch);
	return;
    }

    send_to_quest(ch, argument, quest, 0, QCOMM_SAY);
}
ACMD(do_qecho)
{
    quest_data *quest = NULL;
    qplayer_data *qp = NULL;

    skip_spaces(&argument);

    if (!(quest = quest_by_vnum(GET_QUEST(ch))) ||
	!(qp = idnum_in_quest(GET_IDNUM(ch), quest))) {
	send_to_char("You are not currently active on any quest.\r\n", ch);
	return;
    }
  
    if (QP_FLAGGED(qp, QP_MUTE)) {
	send_to_char("You have been quest-muted.\r\n", ch);
	return;
    }

    if (QP_FLAGGED(qp, QP_IGNORE)) {
	send_to_char("You can't quest-echo while ignoring the quest.\r\n", ch);
	return;
    }

    if (!*argument) {
	send_to_char("Qecho what?\r\n", ch);
	return;
    }

    send_to_quest(ch, argument, quest, 0, QCOMM_ECHO);
}

void
qp_reload( int sig = 0 )
{
    int x;
    int player_i = 0;
    struct char_data *immortal = NULL;
    struct char_file_u tmp_store;
    int online = 0, offline = 0;

    if ( ! immortal ){
	for(x = 0; x <= top_of_p_table; ++x) {
       
	    if ( (player_i = load_char( player_table[x].name, &tmp_store) ) > -1) {
		
		// its an immort with an allowance, set 'em up
		if ( tmp_store.level >= LVL_IMMORT && tmp_store.player_specials_saved.qp_allowance > 0 ) {
		    sprintf( buf, "QP_RELOAD: Reset %s to %d QPs from %d. ( file )",
			     tmp_store.name,
			     tmp_store.player_specials_saved.qp_allowance,
			     tmp_store.player_specials_saved.quest_points );
		    slog( buf );

		    tmp_store.player_specials_saved.quest_points = tmp_store.player_specials_saved.qp_allowance;
		    fseek( player_fl, ( player_i ) * sizeof( struct char_file_u ), SEEK_SET );
		    fwrite( &tmp_store, sizeof( struct char_file_u ), 1, player_fl);
		    offline++;
		}
		
	    }
	}
   
    }

    //
    // Check if the imm is logged on
    //
  
    for( immortal = character_list; immortal; immortal = immortal->next) {
	if(GET_LEVEL( immortal ) >= LVL_IMMORT && (! IS_NPC( immortal ) && GET_QUEST_ALLOWANCE( immortal ) > 0 ) ) {
	    sprintf( buf, "QP_RELOAD: Reset %s to %d QPs from %d. ( online )",
		     GET_NAME( immortal ),
		     GET_QUEST_ALLOWANCE( immortal ),
		     GET_QUEST_POINTS( immortal ) );
	    slog( buf );
	    
	    GET_QUEST_POINTS( immortal ) = GET_QUEST_ALLOWANCE( immortal );
	    send_to_char( "You quest points have been restored!\r\n", immortal );
	    save_char( immortal, NULL );
	    online++;
	    //
	    //  sprintf( buf, "Online Mode: %s's QP reset to %d", GET_NAME( immortal ), GET_QUEST_POINTS( immortal ) );
	    //  mudlog( buf, CMP, LVL_GOD, TRUE);
	    //

	}
    }
    sprintf( buf, "QP's have been reloaded.  %d offline and %d online reset.", offline, online );
    mudlog( buf, NRM, LVL_GRGOD, TRUE );


}


void
do_qcontrol_award( CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;
    CHAR       *vict  = NULL;
    char       arg3[MAX_INPUT_LENGTH]; // Awarded points
    int        award;
    int        idnum;
        
     
    argument = two_arguments( argument, arg1, arg2);
    argument = one_argument( argument, arg3);
    award = atoi( arg3 );
   
    if (!*arg1 || !*arg2){
	do_qcontrol_usage( ch, com);
	return;
    }
    
    if ( !(quest = find_quest( ch, arg2) ) ){
	return;
    }

    if ( !quest_edit_ok( ch, quest) ){
	return;
    }

    if( ( idnum = get_id_by_name( arg1 ) ) < 0){
	sprintf( buf, "There is no character named '%s'.\r\n", arg1);
	send_to_char( buf, ch);
	return;
    }
  
    if( quest->ended ){
	send_to_char( "That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
   
    if ( (vict = get_char_in_world_by_idnum( idnum ) ) ) {
        if( !idnum_in_quest( idnum, quest) ){
            send_to_char( "No such player in the quest.\r\n", ch);
            return;
        }
        if(!vict->desc) {
            send_to_char("You can't award quest points to a linkless player.\r\n",ch);
            return;
        }
    }

    if( ( award <= 0 ) ){
	send_to_char( "The award must be greater than zero.\r\n", ch);
	return;
    }
       
    if( ( award > GET_QUEST_POINTS( ch ) ) ){
	send_to_char( "You do not have the required quest points.\r\n", ch);
	return;
    }
     
    if(!(vict)){
	send_to_char( "No such player in the quest.\r\n", ch);
        return;
    }

    if ( (ch) && (vict)){
        GET_QUEST_POINTS( ch ) -= award;
	GET_QUEST_POINTS( vict ) += award;
	save_char( ch, NULL );
	save_char( vict, NULL );
	sprintf( buf, "awarded player %s %d qpoints.",GET_NAME( vict ), award);
	qlog( ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(ch),LVL_IMMORT), TRUE);
	if ( *argument ) {
	    sprintf( buf, "'s Award Comments: %s", argument);
	    qlog( ch, buf, QLOG_COMP, MAX(GET_INVIS_LEV(ch),LVL_IMMORT), TRUE);
	}
    }    

}   
void
do_qcontrol_penalize( CHAR *ch, char *argument, int com)
{
    quest_data *quest = NULL;
    CHAR       *vict  = NULL;
    char       arg3[MAX_INPUT_LENGTH]; // Penalized points
    int        penalty;
    int        idnum;
        
     
    argument = two_arguments( argument, arg1, arg2);
    argument = one_argument( argument, arg3);
    penalty = atoi( arg3 );
   
    if (!*arg1 || !*arg2){
	do_qcontrol_usage( ch, com);
	return;
    }
    
    if ( !(quest = find_quest( ch, arg2) ) ){
	return;
    }

    if ( !quest_edit_ok( ch, quest) ){
	return;
    }

    if( ( idnum = get_id_by_name( arg1 ) ) < 0){
	sprintf( buf, "There is no character named '%s'.\r\n", arg1);
	send_to_char( buf, ch);
	return;
    }
  
    if( quest->ended ){
	send_to_char( "That quest has already ended, you psychopath!\r\n", ch);
	return;
    }
   
    if ( (vict = get_char_in_world_by_idnum( idnum ) ) ) {
	if( !idnum_in_quest( idnum, quest) ){
	    send_to_char( "No such player in the quest.\r\n", ch);
	    return;
	}
    }

    if(!(vict)){
	send_to_char( "No such player in the quest.\r\n", ch);
        return;
    }

    if( ( penalty <= 0 ) ){
	send_to_char( "The penalty must be greater than zero.\r\n", ch);
	return;
    }
       
    if( ( penalty > GET_QUEST_POINTS( vict ) ) ){
	send_to_char( "They do not have the required quest points.\r\n", ch);
	return;
    }
     
    if ( (ch) && (vict)){
		GET_QUEST_POINTS( vict ) -= penalty;
		save_char( vict, NULL );
		sprintf( buf, "penalized player %s %d qpoints.",GET_NAME( vict ), penalty);
		qlog( ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LEV(ch),LVL_IMMORT), TRUE);
		if ( *argument ) {
			sprintf( buf, "'s Penalty Comments: %s", argument);
			qlog( ch, buf, QLOG_COMP, MAX(GET_INVIS_LEV(ch),LVL_IMMORT), TRUE);
		}
    }    

}   
