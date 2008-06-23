#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "defs.h"
/* ************************************************************************
*   File: interpreter.h                                 Part of CircleMUD *
*  Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: interpreter.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
#define ACMD(name)  \
   void (name)(__attribute__ ((unused)) struct Creature *ch, \
   	__attribute__ ((unused)) char *argument, \
	__attribute__ ((unused)) int cmd, \
	__attribute__ ((unused)) int subcmd, \
	__attribute__ ((unused)) int *return_flags)

#define ACCMD(name)  \
   void (name)(__attribute__ ((unused)) struct Creature *ch, \
   __attribute__ ((unused)) char *argument, \
   __attribute__ ((unused)) int cmd, \
   __attribute__ ((unused)) int subcmd, \
   __attribute__ ((unused)) int *return_flags)

void command_interpreter(struct Creature *ch, char *argument);
int search_block(const char *arg, const char **list, bool exact);
char lower(char c);
char *one_argument(char *argument, char *first_arg);
void one_argument(const char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
void two_arguments(const char *argument, char *first_arg, char *second_arg);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void handle_input(struct descriptor_data *d);
int is_abbrev(const char *arg1, const char *arg2, int count = 0);
bool is_number(const char *str);
int find_command(const char *command, bool abbrev = false);
void skip_spaces(char **string);
void skip_spaces(const char **string);
void send_unknown_cmd(Creature *ch);

// from search.c
int triggers_search(struct Creature *ch, int cmd, char *arg,
	struct special_search_data *srch);

//
// used by ACMD functinos to set return_flags if they exist
//

#define ACMD_set_return_flags( val ) {\
    if ( return_flags ) { \
        *return_flags = val; \
    } \
}

//
// used by any functions to set their return_flags if they exist
//

struct command_info {
	const char *command;
	byte minimum_position;
    ACMD(*command_pointer);
	sh_int minimum_level;
	int subcmd;
	int security;
	unsigned long usage;
};

struct sort_struct {
	int sort_pos;
	bool is_social;
	bool is_mood;
};

extern struct sort_struct *cmd_sort_info;

extern int num_of_cmds;

/* necessary for CMD_IS macro */
#ifndef __interpreter_c__
extern struct command_info cmd_info[];
#endif

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (cmdnum >= 1 && cmdnum <= 6)

struct alias_data {
	char *alias;
	char *replacement;
	int type;
	struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

struct show_struct {
	char *cmd;
	char level;
	char *group;
};
struct set_struct {
	char *cmd;
	char level;
	char pcnpc;
	char type;
	char *group;
};

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH	1
#define SCMD_EAST	2
#define SCMD_SOUTH	3
#define SCMD_WEST	4
#define SCMD_UP		5
#define SCMD_DOWN	6
#define SCMD_FUTURE     7
#define SCMD_PAST       8
#define SCMD_MOVE       9
#define SCMD_JUMP       10
#define SCMD_CRAWL      11

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_CREDITS    1
#define SCMD_NEWS       2
#define SCMD_WIZLIST    3
#define SCMD_VERSION    4
#define SCMD_IMMLIST    5
#define SCMD_MOTD	6
#define SCMD_IMOTD	7
#define SCMD_CLEAR	8
#define SCMD_WHOAMI	9

/* do_gen_tog */
#define CMD_TESTER 1
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_UNUSED     3
#define SCMD_NOTELL	4
#define SCMD_NOAUCTION	5
#define SCMD_DEAF	6
#define SCMD_NOGOSSIP	7
#define SCMD_NOGRATZ	8
#define SCMD_NOWIZ	9
#define SCMD_ROOMFLAGS	10
#define SCMD_HOLYLIGHT	11
#define SCMD_SLOWNS	12
#define SCMD_AUTOEXIT	13
#define SCMD_AFK        14
#define SCMD_NOMUSIC    15
#define SCMD_NOSPEW	16
#define SCMD_GAGMISS    17
#define SCMD_AUTOPAGE   18
#define SCMD_NOCLANSAY  19
#define SCMD_NOIDENTIFY 20
#define SCMD_DEBUG      21
#define SCMD_NEWBIE_HELP 22
#define SCMD_AUTO_DIAGNOSE 23
#define SCMD_NODREAM    24
#define SCMD_NOPROJECT  25
#define SCMD_HALT       26
#define SCMD_MORTALIZE  27
#define SCMD_NOAFFECTS  28
#define SCMD_NOHOLLER   29
#define SCMD_NOIMMCHAT  30
#define SCMD_CLAN_HIDE  31
#define SCMD_AUTOPROMPT 32
#define SCMD_NOWHO      33
#define SCMD_ANON       34
#define SCMD_NOTRAILERS 35
#define SCMD_LOGALL     36
#define SCMD_JET_STREAM 37
#define SCMD_WEATHER    38
#define SCMD_AUTOSPLIT  39
#define SCMD_AUTOLOOT   40
#define SCMD_PKILLER    41
#define SCMD_NOGECHO	42
#define SCMD_AUTOWRAP   43
#define SCMD_WORLDWRITE 44
#define SCMD_DEATHLOG   45
#define SCMD_NOGUILDSAY 46
#define SCMD_NOPETITION 47
#define SCMD_NASTY      48
#define SCMD_NOHAGGLE   49

/* do_wizutil */
#define SCMD_REROLL	0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE	4
#define SCMD_THAW	5
#define SCMD_UNAFFECT	6
#define SCMD_NOPOST     7

/* do_set */
#define SCMD_TESTER_SET 1

/* do_gen_com */
#define SCMD_HOLLER		0
#define SCMD_SHOUT		1
#define SCMD_GOSSIP		2
#define SCMD_AUCTION	3
#define SCMD_GRATZ		4
#define SCMD_MUSIC      5
#define SCMD_SPEW		6
#define SCMD_DREAM      7
#define SCMD_PROJECT    8
#define SCMD_NEWBIE     9
#define SCMD_CLANSAY	10
#define SCMD_GUILDSAY	11
#define SCMD_CLANEMOTE	12
#define SCMD_PETITION	13
#define SCMD_HAGGLE 	14

/* do_shutdown */
#define SCMD_SHUTDOW	0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI	0
#define SCMD_QUIT	1

/* do_date */
#define SCMD_DATE	0
#define SCMD_UPTIME	1

/* do_commands */
#define SCMD_COMMANDS	0
#define SCMD_SOCIALS	1
#define SCMD_WIZHELP	2
#define SCMD_MOODS  	3

/* do_drop */
#define SCMD_DROP	0
#define SCMD_JUNK	1
#define SCMD_DONATE	2
#define SCMD_GUILD_DONATE 3

/* do_gen_write */
#define SCMD_BUG	0
#define SCMD_TYPO	1
#define SCMD_IDEA	2
#define SCMD_BAD_IDEA	3

/* do_look */
#define SCMD_LOOK	0

/* do_qcomm */
#define SCMD_QSAY	0
#define SCMD_QECHO	1

/* do_clan_comm */
#define SCMD_CLAN_SAY   0
#define SCMD_CLAN_ECHO  1

/* do_pour */
#define SCMD_POUR	0
#define SCMD_FILL	1

/* do_poof */
#define SCMD_POOFIN	0
#define SCMD_POOFOUT	1

/* do_hit */
#define SCMD_HIT	0
#define SCMD_MURDER	1

/* do_eat */
#define SCMD_EAT	0
#define SCMD_TASTE	1
#define SCMD_DRINK	2
#define SCMD_SIP	3

/* do_use */
#define SCMD_USE	0
#define SCMD_QUAFF	1
#define SCMD_RECITE	2
#define SCMD_INJECT     3
#define SCMD_SWALLOW    4
#define SCMD_READ	5

/* do_echo */
#define SCMD_ECHO	0
#define SCMD_EMOTE	1

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4
#define SCMD_HACK       5

/* do_help	*/
#define SCMD_MODRIAN    1
#define SCMD_SKILLS     2
#define SCMD_CITIES     3
#define SCMD_HANDBOOK   4
#define SCMD_POLICIES   5

/* do_send_color  */
#define SCMD_KRED	1
#define SCMD_KGRN	2
#define SCMD_KYEL	3
#define SCMD_KBLU       4
#define SCMD_KMAG       5
#define SCMD_KCYN	6
#define SCMD_KWHT	7
#define SCMD_KBLD 	8
#define SCMD_KUND	9
#define SCMD_KBLK	10
#define SCMD_KREV	11
#define SCMD_KNRM	12

/* command do_kill */
#define SCMD_SLAY       1

/* command do_activate */
#define SCMD_OFF        0
#define SCMD_ON         1


/* command do_switch */
#define SCMD_NORMAL     0
#define SCMD_QSWITCH    1

/* command do_wiznet */
#define SCMD_WIZNET      0
#define SCMD_IMMCHAT     1

/* command do_attach */
#define SCMD_ATTACH      0
#define SCMD_DETACH      1

/** commmand do_equipment **/
#define SCMD_EQ          0
#define SCMD_IMPLANTS    1
#define SCMD_TATTOOS     2

/** command do_battle_cry **/
#define SCMD_BATTLE_CRY        0
#define SCMD_KIA               1
#define SCMD_CRY_FROM_BEYOND   2

/** command do_load **/
#define SCMD_LOAD       0
#define SCMD_UNLOAD     1

#define MOVE_NORM    0
#define MOVE_JUMP    1
#define MOVE_BURGLE  2
#define MOVE_XXX     3
#define MOVE_RUSH    4
#define MOVE_FLEE    5
#define MOVE_RETREAT 6
#define MOVE_DRAG    7
#define MOVE_CRAWL   8

/* do_return */
#define SCMD_RETURN 0
#define SCMD_RECORP 1
#define SCMD_FORCED 2
#define SCMD_NOEXTRACT 3

/* do_skills */
#define SCMD_SKILLS_ONLY 0
#define SCMD_SPELLS_ONLY 1

#define SPELL_BIT 1
#define TRIG_BIT  2
#define ZEN_BIT   4
#define ALTER_BIT 8
#define SONG_BIT 16

// do_dyntext_show
#define SCMD_DYNTEXT_NEWS  1
#define SCMD_DYNTEXT_INEWS 2
#define SCMD_DYNTEXT_FAIT_1 3
#define SCMD_DYNTEXT_FAIT_2 4
#define SCMD_DYNTEXT_FAIT_3 5

#endif							//TEMPUSMUD_INTERPRETER_H
