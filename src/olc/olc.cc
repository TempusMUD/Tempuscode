//
// File: olc.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/* ************************************************************************
*  FILE:  olc.c                                         Part of TempusMUD *
*  Usage: online creation                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "spells.h"
#include "materials.h"
#include "vehicle.h"
#include "specs.h"
#include "shop.h"
#include "smokes.h"
#include "paths.h"
#include "bomb.h"
#include "guns.h"
#include "char_class.h"

#define NUM_POSITIONS 11

#define OLC_USAGE "Usage:\r\n"                                   \
"olc rsave\r\n"                                \
"olc rmimic <room number> [all, sound, desc, exdesc, flags, sector, title]\r\n"                 \
"olc rset <parameter> <arguments>\r\n"           \
"olc exit <direction> <parameter> <value> ['one-way']\r\n" \
"olc rexdesc <create | remove | edit | addkey> <keywords> [new keywords\r\n"  \
"olc clear <room | obj>\r\n"           \
"olc create/destroy <room|zone|obj|mob|shop|search|ticl> <number>\r\n" \
"olc help [keyword]\r\n"               \
"olc osave\r\n"                        \
"olc oedit [number | 'exit']\r\n"               \
"olc ostat [number]\r\n"               \
"olc oset <option> <value>\r\n"        \
"olc oexdesc <create | remove | edit | addkey> <keywords> [new keywords\r\n"                      \
"olc oload [number]\r\n" \
"olc omimic <obj number>\r\n" \
"olc zset [zone] <option> <value>\r\n" \
"olc zcmd [zone] [cmdrenumber] [list] [cmdremove <number>] \r\n" \
"                 <M|O|P|R|D|E|G> <if_flag> [arg1] [arg2] [arg3]\r\n" \
"olc zsave [zone]\r\n" \
"olc zmob <mobile vnum> <max loaded> [prob]\r\n" \
"olc zequip <mob name> <item number> <max> <position> [prob]\r\n" \
"olc zimplant <mob name> <item number> <max> <position> [prob]\r\n"\
"olc zobj <object vnum> <max loaded> [prob]\r\n" \
"olc zdoor <direction> [+/-] [OPEN|CLOSED|LOCKED|HIDDEN]\r\n"\
"olc zput <obj name> <obj vnum> <max loaded> [prob]\r\n"\
"olc zgive <mob name> <obj vnum> <max loaded> [prob]\r\n"\
"olc zpath <'mob'|'obj'> <name> <path name>\r\n" \
"olc zreset/zpurge\r\n" \
"olc medit [number | 'exit']\r\n" \
"olc mload <vnum>\r\n" \
"olc mstat\r\n" \
"olc mset\r\n" \
"olc mmimic <vnum>\r\n" \
"olc msave\r\n" \
"olc sedit [number | 'exit']\r\n" \
"olc sstat\r\n" \
"olc sset\r\n"  \
"olc ssave\r\n" \
"olc hlist\r\n" \
"olc hedit [number | 'exit']\r\n" \
"olc hstat\r\n" \
"olc hset <text>\r\n" \
"olc xedit <search trigger> <search keyword>\r\n" \
"olc xset <arg> <val>\r\n"  \
"olc xstat\r\n" \
"olc show\r\n"


#define OLC_ZSET_USAGE "Usage:\r\n"                                  \
"olc zset [zone] name <name>\r\n"                  \
"olc zset [zone] lifespan <lifespan>\r\n"          \
"olc zset [zone] top <max room vnum in zone>\r\n"  \
"olc zset [zone] reset <reset mode>\r\n"           \
"olc zset [zone] tframe <time frame>\r\n"          \
"olc zset [zone] plane <plane>\r\n"                \
"olc zset [zone] owner <player name>\r\n"          \
"olc zset [zone] flags <+/-> [FLAG FLAG ...]\r\n"  \
"olc zset [zone] <hours | years> <mod>\r\n"        \
"olc zset [zone] blanket_exp <percent>\r\n"        \
"Usage: olc zset [zone] command <cmd num> [if|max|prob] <value>\r\n"

#define OLC_ZCMD_USAGE "Usage:\r\n"                                      \
"olc zcmd [zone] list [range # #] [obj|mob|eq|give|door|etc...]\r\n" \
"olc zcmd [zone] cmdremove <number>\r\n" \
"olc zcmd [zone] cmdrenumber\r\n" \
"olc zcmd move   <original num> <target num>\r\n" \
"olc zcmd [zone] <M> <if_flag> <mob> <num> <room> <prob>\r\n" \
"olc zcmd [zone] <O> <if_flag> <obj> <num> <room> <prob>\r\n" \
"olc zcmd [zone] <P> <if_flag> <obj> <num> <obj> <prob>\r\n"  \
"olc zcmd [zone] <R> <if_flag> <obj> <room>\r\n"       \
"olc zcmd [zone] <E> <if_flag> <obj> <num> <pos> <mob> <prob>\r\n" \
"olc zcmd [zone] <G> <if_flag> <obj> <num> <mob> <prob>\r\n"  \
"olc zcmd [zone] <D> <if_flag> <room> <door> <state>\r\n" \
"olc zcmd [zone] <I> <if_flag> <obj> <num> <pos> <mob> <prob>\r\n" 


#define SEARCH_USAGE "Values:\r\n"  \
"Command  Values: 0              1            2\r\n" \
"OBJECT       object vnum    room vnum     max exist\r\n" \
"MOBILE        mob vnum      room vnum     max exist\r\n" \
"EQUIP         -unused-      obj vnum      load pos\r\n"  \
"GIVE          -unused-      obj vnum      max exist\r\n" \
"DOOR          room vnum     direction     door flag\r\n" \
"NONE            unused      room vnum     unused\r\n" \
"TRANS         room vnum     unused        unused\r\n" \
"SPELL           level       room vnum     spellnum\r\n"  \
"DAMAGE        dam dice      room vnum     damtype\r\n" \
"SPAWN         spawn room    targ room     hunter?\r\n"

#define OLC_SHOW_USAGE "Usage:\n" \
"olc show nosound                              -- displays rooms with no sound\n"  \
"olc show noexitdesc ['all'|'toroom']          -- displays exits with no desc (default !toroom only)\n"

extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct obj_data *object_list;
extern int olc_lock;

extern char *olc_guide;
extern const char *flow_types[];
extern const char *char_flow_msg[NUM_FLOW_TYPES+1][3];
extern const char *obj_flow_msg[NUM_FLOW_TYPES+1][2];

long asciiflag_conv(char *buf);

void    num2str(char *str, int num);
void    show_olc_help(struct char_data *ch, char *arg);
void    do_stat_object(struct char_data *ch, struct obj_data *obj);

ACMD(do_zonepurge);
ACMD(do_zreset);

int get_line_count(char *buffer);
int prototype_obj_value(struct obj_data *obj);
int save_objs (struct char_data *ch);
int save_wld (struct char_data *ch);
int save_mobs(struct char_data *ch);
int save_shops(struct char_data *ch);
struct room_data *do_create_room(struct char_data *ch, int vnum);
struct obj_data *do_create_obj(struct char_data *ch, int vnum);
struct char_data *do_create_mob(struct char_data *ch, int vnum);
struct shop_data *do_create_shop(struct char_data *ch, int vnum);
struct help_index_element *do_create_help(struct char_data *ch);
struct ticl_data *do_create_ticl(struct char_data *ch, int vnum);
int do_destroy_room(struct char_data *ch, int vnum);
int do_destroy_object(struct char_data *ch, int vnum);
int do_destroy_mobile(struct char_data *ch, int vnum);
int do_destroy_shop(struct char_data *ch, int vnum);
int do_create_zone(struct char_data *ch, int num);
int olc_mimic_mob(struct char_data *ch, struct char_data *orig, struct char_data *targ, int mode);
void olc_mimic_room(struct char_data *ch, struct room_data *targ, char *arg);
void do_olc_rexdesc(struct char_data *ch, char *a);
void perform_oset(struct char_data *ch, struct obj_data *obj_p, 
		  char *argument, byte subcmd);
void do_zset_command(struct char_data *ch, char *argument);
void do_zcmd(struct char_data *ch, char *argument);
void do_zone_cmdlist(struct char_data *ch, struct zone_data *zone);
void do_zmob_cmd(struct char_data *ch, char *argument);
void do_zobj_cmd(struct char_data *ch, char *argument);
void do_zdoor_cmd(struct char_data *ch, char *argument);
void do_zgive_cmd(struct char_data *ch, char *argument);
void do_zput_cmd(struct char_data *ch, char *argument);
void do_zequip_cmd(struct char_data *ch, char *argument);
void do_zimplant_cmd(struct char_data *ch, char *argument);
void do_zpath_cmd(struct char_data *ch, char *argument);
void do_mob_medit(struct char_data *ch, char *argument);
void do_mob_mstat(struct char_data *ch);
void do_mob_mset(struct char_data *ch, char *argument);
void do_shop_sedit(struct char_data *ch, char *argument);
void do_shop_sstat(struct char_data *ch);
void do_shop_sset(struct char_data *ch, char *argument);
void do_help_fedit(struct char_data *ch, char *argument);
void do_help_list(struct char_data *ch);
void do_help_hedit(struct char_data *ch, char *argument);
void do_help_hstat(struct char_data *ch);
void do_help_hset(struct char_data *ch, char *argument);
int save_zone(struct char_data *ch, struct zone_data *zone);
void do_olc_xset(struct char_data *ch, char *argument);
void do_olc_rset(struct char_data *ch, char *argument);
void do_olc_xstat(struct char_data *ch);
struct special_search_data * do_create_search(struct char_data *ch, char *arg);
int do_destroy_search(struct char_data *ch, char *arg);
int set_char_xedit(struct char_data *ch, char *argument);
void do_clear_room(struct char_data *ch);
void do_clear_olc_object(struct char_data *ch);
void do_ticl_tedit(struct char_data *ch, char *argument);
void do_ticl_tstat(struct char_data *ch);

char *find_exdesc(char *word, struct extra_descr_data *list,int find_exact=0);
extern struct attack_hit_type attack_hit_text[];

const char *olc_commands[] = {
    "rsave",			/* save wld file */
    "rmimic",
    "rset",                       /* 2 -- set utility */
    "exit",
    "owner",                      /* 4 -- set owner  */
    "rexdesc",
    "unlock",                     /* 6 -- zone/global unlock */
    "lock",
    "guide",
    "rsearch",
    "help",                       /* 10 -- help function */
    "osave",
    "oedit",
    "ostat",
    "oset",
    "oexdesc",                    /* 15 -- object exdescs  */
    "oload",
    "omimic",
    "clear",                    
    "supersave",
    "create",                     /* 20 -- make new rooms/objs/zones etc. */
    "destroy",
    "zset",
    "zcmd",
    "zsave",
    "zmob",                       /* 25 -- load mob */
    "zobj",
    "zdoor",
    "zreset",
    "zpurge",
    "zequip",
    "zput",
    "zgive",
    "medit",                      /* 33 -- do_mob_medit() */
    "mstat",
    "mset",
    "msave",
    "sedit",
    "sstat",
    "sset",
    "ssave",                      /* 40 */
    "zimplant",
    "fedit",
    "mmimic",
    "hlist",
    "hedit",
    "hstat",
    "hset",
    "zpath",
    "xedit",
    "xset",                       /* 50 */
    "xstat",
    "tedit",
    "tstat",
    "tset",
    "tsave",
    "mload", 
    "show",
     "\n"				/* many more to be added */
};


struct extra_descr_data *locate_exdesc(char *word, 
				       struct extra_descr_data *list)
{

    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
	if (isname(word, i->keyword))
	    return (i);
    
    return NULL;
}


#define obj_p     GET_OLC_OBJ(ch)
#define mob_p     GET_OLC_MOB(ch)

/* The actual do_olc command for the interpreter.  Determines the target
   entity, checks permissions, and passes contrl to olc_interpreter */
ACMD(do_olc)
{
    char                    mode_arg[MAX_INPUT_LENGTH];
    char                    arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int                     olc_command;
    room_num                vnum;
    struct room_data       *rnum, *room;
    int                     edir;
    int                     i, j, k, found;
    byte one_way = FALSE;
    struct extra_descr_data *desc, *ndesc, *temp;
    struct obj_data *tmp_obj = NULL, *obj = NULL;
    struct descriptor_data *d = NULL;
    struct zone_data *zone = NULL;
    struct shop_data *shop = NULL;
    struct char_data *mob = NULL;
    struct char_data *tmp_mob = NULL;
	struct special_search_data *tmp_search;

  
    if (olc_lock || (IS_SET(ch->in_room->zone->flags, ZONE_LOCKED)) ) {
	if (GET_LEVEL(ch) >= LVL_IMPL) {
	    sprintf(buf,"\007\007\007\007%sWARNING.%s  Overriding olc %s lock.\r\n", 
		    CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM), olc_lock ? "global" :
                    "discrete zone");
	    send_to_char(buf, ch);
	} else {
	    send_to_char("OLC is currently locked.  Try again later.\r\n", ch);
	    return;
	}
    } 

    /* first, figure out the first (mode) argument */
    half_chop(argument, mode_arg, argument);
    skip_spaces(&argument);

    if (!*mode_arg) {
	page_string(ch->desc, OLC_USAGE, 1);
	return;
    }
    if ((olc_command = search_block(mode_arg, olc_commands, FALSE))<0) {
	sprintf(buf, "Invalid command '%s'.\r\n", mode_arg);
	send_to_char(buf, ch);
	return;
    } 
    if (ch->in_room == NULL)  {
	send_to_char("What the hell? Where are you!?!?\r\n",ch);
	return;
    }

    if (olc_command != 8 && olc_command != 10) {  /* help? */
	if (!CAN_EDIT_ZONE(ch, ch->in_room->zone) ) {
	    send_to_char("Piss off Beanhead.  Permission DENIED.\r\n", ch);
	    sprintf(buf,"Failed attempt for %s to edit zone %d.",GET_NAME(ch),
		    ch->in_room->zone->number);
	    mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
	    return;
	}
    }
  
    if ( ( olc_command <= 3 || olc_command == 5 ) && 
	 ! OLC_EDIT_OK( ch, ch->in_room->zone, ZONE_ROOMS_APPROVED ) ) {
	send_to_char("World OLC is not approved for this zone.\r\n", ch);
	return;
    }
  
    switch (olc_command) {
    case 0: /* rsave */
	if(!save_wld (ch))
	    send_to_char("World file saved.\r\n",ch);
	else
	    send_to_char("An error occured while saving.\r\n",ch);
	break;
    case 1: /* rmimic */
	if (!*argument)  {
	    send_to_char("Usage: olc rmimic <room num> [all] [sounds] [flags] [desc] [exdesc]\r\n",ch);
	    break;
	}
	argument = one_argument(argument, arg1);
	vnum = atoi(arg1);
	if (!(rnum = real_room(vnum))) {
	    send_to_char("That's not a valid room, genius.\r\n",ch);
	    break;
	}
	if (rnum == ch->in_room)  {
	    send_to_char("Real funny.\r\n",ch);
	    break;
	}
	olc_mimic_room(ch, rnum, argument);
	break;
    case 2: /* rset */
	do_olc_rset(ch, argument);
	break;

    case 3:                         /* exit */
	if (!*argument)  {
	    send_to_char ("Usage: olc exit <direction> <parameter> <value>.\r\n",ch);
	    return;
	}
	half_chop(argument, buf, argument);
	skip_spaces(&argument);

	if ((!*buf) || ((edir = search_block (buf, dirs, FALSE)) < 0))   {
	    send_to_char ("What exit?\r\n",ch);
	    return;
	}

	half_chop(argument, buf, argument);
	skip_spaces(&argument);
	if (!*buf)  {
	    send_to_char("Options are: description, doorflags, toroom, keynumber, keywords, remove.\r\n",ch);
	    return;
	}
	if (!EXIT(ch,edir)) {
	    CREATE(EXIT(ch,edir), struct room_direction_data, 1);
	    EXIT(ch, edir)->to_room = NULL;
	} else if (is_abbrev(buf, "remove")) {
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free(EXIT(ch, edir)->general_description);
	    free(EXIT(ch, edir)->keyword);
	    free(EXIT(ch, edir));
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    EXIT(ch, edir) = NULL;
	    send_to_char("Exit removed.\r\n", ch);
	    return;
	}

	if (is_abbrev(buf,"description"))  {
	    skip_spaces(&argument);

	    if (!*argument && is_abbrev(argument, "remove")) {
		if (EXIT(ch,edir)->general_description) { 
		    free(EXIT(ch,edir)->general_description);
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    EXIT(ch,edir)->general_description = NULL;
		}
		send_to_char("Exit desc removed.\r\n", ch);
		return;
	    }
  
	    if (EXIT(ch, edir)->general_description == NULL) {
		send_to_char(TED_MESSAGE, ch);
		ch->desc->str = &EXIT(ch, edir)->general_description;
		ch->desc->max_str = MAX_STRING_LENGTH;
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to create an exit description.", TRUE, ch, 0, 0, TO_ROOM);
	    }
	    else {
		send_to_char("Use TED to modify the exit description.\r\n", ch);
		ch->desc->str = &EXIT(ch, edir)->general_description;
		ch->desc->max_str = MAX_STRING_LENGTH;
		ch->desc->editor_mode = 1;
		ch->desc->editor_cur_lnum = get_line_count(EXIT(ch, edir)->general_description);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to edit an exit description.", TRUE, ch, 0, 0, TO_ROOM);
	    }
	    return;
	} 
	else if (is_abbrev(buf,"keywords"))  {
	    if (EXIT(ch,edir)->keyword)  {
		free(EXIT(ch,edir)->keyword);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		EXIT(ch,edir)->keyword = NULL;
	    }
	    if (argument && *argument) {
		EXIT(ch,edir)->keyword = str_dup(argument);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		send_to_char("Keywords set.\r\n", ch);
	    } else 
		send_to_char("What keywords?!\r\n", ch);

	    return;
	}
	else if (is_abbrev(buf,"doorflags"))  {
	    if (!argument || !*argument) {
		EXIT(ch,edir)->exit_info = 0;
		show_olc_help(ch, buf);
	    } else {
		EXIT(ch,edir)->exit_info = asciiflag_conv(argument);
		send_to_char("Doorflags set.\r\n", ch);
	    }

	    return;
	}
	else if (is_abbrev(buf,"keynumber"))  {
	    if (!argument || !*argument || !is_number(argument))
		EXIT(ch,edir)->key = 0;
	    else
		EXIT(ch,edir)->key = atoi(argument);

	    send_to_char("Keynumber set.\r\n", ch);

	    return;
	}
	else if (is_abbrev(buf,"toroom"))  {
	    half_chop(argument, arg1, arg2);
	    if (!*arg1 || !is_number(arg1) || (!(room = real_room(atoi(arg1))))) {
		if (!(room = do_create_room(ch, atoi(arg1)))) {
		    EXIT(ch,edir)->to_room = NULL;
		    send_to_char("That destination room does not exist.\r\n", ch);
		    return;
		} else
		    send_to_char("The destination room has been created.\r\n", ch);
	    } 
      
	    if (*arg2 && arg2 && is_abbrev(arg2, "one-way"))
		one_way = TRUE;
	    EXIT(ch,edir)->to_room = room;
	    if (!one_way && !ABS_EXIT(room, rev_dir[edir])) {
		if (!CAN_EDIT_ZONE(ch, room->zone)) {
		    send_to_char("To room set.  Unable to create return exit.\r\n",ch);
		} else {
		    CREATE(ABS_EXIT(room, rev_dir[edir]), struct room_direction_data, 1);
		    ABS_EXIT(room, rev_dir[edir])->to_room = ch->in_room;
		    send_to_char("To room set.  Return exit created from to room.\r\n", ch);
		}
	    } else if (!one_way && 
		       ABS_EXIT(room, rev_dir[edir])->to_room == NULL) {
		if (!CAN_EDIT_ZONE(ch, room->zone)) {
		    send_to_char("To room set.  Unable to set return exit.\r\n",ch);
		} else {
		    ABS_EXIT(room, rev_dir[edir])->to_room = ch->in_room;
		    send_to_char("To room set.  Return exit set in to room.\r\n", ch);
		}
	    } else
		send_to_char("To room set.\r\n", ch);

	    return;
	}
	else  {
	    send_to_char("Options are: description, doorflags, toroom, keynumber.\r\n",ch);
	    return;
	}
	break;
    case 4:    /* Owner */
	send_to_char("Use olc zset owner, instead.\r\n", ch);
	break;
    case 5:   /* rexdesc*/
	do_olc_rexdesc(ch, argument);
	break;
    case 6:          /*  unlock */
	if (!*argument || is_abbrev(argument, "all")) {
	    if (!olc_lock)
		send_to_char("Olc is currently globally unlocked.\r\n", ch);
	    else if (GET_LEVEL(ch) >= olc_lock) {
		send_to_char("Unlocking olc (global).\r\n", ch);
		olc_lock = 0;
		sprintf(buf, "OLC: %s has unlocked global access.", GET_NAME(ch));
		mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	    } else
		send_to_char("Olc unlock:  Permission denied.\r\n", ch);
	} else if (!is_number(argument))
	    send_to_char("The argument must be a number, or 'all'.\r\n", ch);
	else {
	    one_argument(argument, arg1);
	    for (j = atoi(arg1), zone = zone_table; zone; zone = zone->next) {
		if (zone->number == j) {  
		    if (CAN_EDIT_ZONE(ch, zone)) {
			if (!IS_SET(zone->flags, ZONE_LOCKED))
			    send_to_char("That zone is currently unlocked.\r\n", ch);
			else {
			    REMOVE_BIT(zone->flags, ZONE_LOCKED);
			    send_to_char("Zone unlocked for online creation.\r\n", ch);
			    sprintf(buf,"OLC: %s has unlocked zone %d (%s).", GET_NAME(ch), 
				    zone->number, zone->name);
			    mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
			}
			return;
		    } else {
			send_to_char("Olc zone unlock: Permission denied.\r\n", ch);
			return;
		    }
		}
	    }
	    send_to_char("That is an invalid zone.\r\n", ch);
	}
	break;
    case 7:    /* lock */
	if (!*argument || is_abbrev(argument, "all")) {
	    if (olc_lock)
		send_to_char("Olc is currently locked.\r\n", ch);
	    else if (OLCIMP(ch)) {
		send_to_char("Locking olc.\r\n", ch);
		olc_lock = GET_LEVEL(ch);
		sprintf(buf, "OLC: %s has locked global access to olc.", GET_NAME(ch));
		mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	    } else
		send_to_char("Olc lock:  Permission denied.\r\n", ch);
	} else if (!is_number(argument))
	    send_to_char("The argument must be a number, or 'all'.\r\n", ch);
	else {
	    one_argument(argument, arg1);
	    for (j = atoi(arg1), zone = zone_table; zone; zone = zone->next) {
		if (zone->number == j) {
		    if (CAN_EDIT_ZONE(ch, zone)) {
			if (IS_SET(zone->flags, ZONE_LOCKED)) 
			    send_to_char("That zone is already locked.\r\n", ch);
			else {
			    SET_BIT(zone->flags, ZONE_LOCKED);
			    send_to_char("Zone locked to online creation.\r\n", ch);
			    sprintf(buf,"OLC: %s has locked zone %d (%s).", GET_NAME(ch), 
				    zone->number, zone->name);
			    mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
			}
			return;
		    } else {
			send_to_char("Olc zone unlock: Permission denied.\r\n", ch);
			return;
		    }
		}
	    }
	    send_to_char("That is an invalid zone.\r\n", ch);
	}
	break;
    case 8:     /************* guide ***************/
	if (!olc_guide)
	    send_to_char("Sorry, the olc guide is not loaded into memory.\r\n", ch);
	else
	    page_string(ch->desc, olc_guide, 1);
	break;
    case 9:            /*************** rsearch ******************/
	break;
    case 10:           /*********** OLC HELP  *********************/
	show_olc_help(ch, argument);
	break;
    case 11:           /*************** osave *********************/
	if(!save_objs (ch))
	    send_to_char("Object file saved.\r\n",ch);
	else
	    send_to_char("An error occured while saving.\r\n",ch);
	break;
    case 12:           /*************** oedit *********************/
	if (!*argument) {
	    if (!obj_p)
		send_to_char("You are not currently editing an object.\r\n", ch);
	    else {
		sprintf(buf, "Current olc object: [%5d] %s\r\n",
			obj_p->shared->vnum, obj_p->short_description);
		send_to_char(buf, ch);
	    }
	    return;
	}
	if (!is_number(argument)) {
	    if (is_abbrev(argument, "exit")) {
		send_to_char("Exiting object editor.\r\n", ch);
		GET_OLC_OBJ(ch) = NULL;
		return;
	    }
	    send_to_char("The argument must be a number.\r\n", ch);
	    return;
	} else {
	    j = atoi(argument);
	    if ((tmp_obj = real_object_proto(j)) == NULL)
		send_to_char("There is no such object.\r\n", ch);
	    else {

		for (zone = zone_table; zone; zone = zone->next)
		    if (j <= zone->top)
			break;

		if (!zone) {
		    send_to_char("That object does not belong to any zone!!\r\n", ch);
		    slog("SYSERR: object not in any zone.");
		    return;
		}
	
		if (!CAN_EDIT_ZONE(ch, zone)) {
		    send_to_char("You do not have permission to edit those objects.\r\n",ch);
		    return;
		}

		if ( ! OLC_EDIT_OK( ch, zone, ZONE_OBJS_APPROVED ) ) {
		    send_to_char("Object OLC is not approved for this zone.\r\n", ch);
		    return;
		}
	
		for (d = descriptor_list; d; d = d->next) {
		    if (d->character && GET_OLC_OBJ(d->character) == tmp_obj) {
			act("$N is already editing that object.",FALSE,ch,0,d->character,
			    TO_CHAR);
			return;
		    }
		}

		GET_OLC_OBJ(ch) = tmp_obj;
		sprintf(buf, "Now editing object [%d] %s%s%s\r\n",
			tmp_obj->shared->vnum,
			CCGRN(ch, C_NRM), tmp_obj->short_description,
			CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	    }
	}

	break;
    case 13:           /*************** ostat *********************/
	if (!*argument) {
	    if (!obj_p)
		send_to_char("You are not currently editing an object.\r\n", ch);
	    else {
		do_stat_object(ch, obj_p);
	    }
	} else if (!is_number(argument)) {
	    send_to_char("The argument must be a number.\r\n", ch);
	    return;
	} else {
	    j = atoi(argument);
	    if (!(tmp_obj = read_object(j)))
		send_to_char("There is no such object.\r\n", ch);
	    else {
		do_stat_object(ch, tmp_obj);
		extract_obj(tmp_obj);
	    }
	}
	break;
    
    case 14:        /*****************  oset *********************/
	if (!obj_p) {
	    send_to_char("You are not currently editing an object.\r\n", ch);
	    return;
	}
    
	perform_oset(ch, obj_p, argument, OLC_OSET);
	break;

    case 15:   /* oexdesc */
	if (!*argument) {
	    send_to_char(OLC_EXDESC_USAGE, ch);
	    return;
	}
	half_chop(argument, buf, argument);
	if (!obj_p)
	    send_to_char("Hey punk, you need an object in your editor first!!!\r\n",
			 ch);
	else if (!*argument)
	    send_to_char("Which extra description would you like to deal with?\r\n",
			 ch);
	else if (!*buf)
	    send_to_char("Valid commands are: create, remove, edit, addkey.\r\n",
			 ch);
	else if (is_abbrev(buf, "remove")) {
	    if ((desc = locate_exdesc(argument,obj_p->ex_description))) {
		REMOVE_FROM_LIST(desc, obj_p->ex_description, next);
		if (desc->keyword)
		    free(desc->keyword);
		else
		    slog("WTF?? !desc->keyword??");

		if (desc->description)
		    free(desc->description);
		else
		    slog("WTF?? !desc->description??");

		free(desc);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		send_to_char("Description removed.\r\n", ch);
		UPDATE_OBJLIST(obj_p, tmp_obj, ->ex_description);
	    } else
		send_to_char("No such extra description.\r\n", ch);
       
	    return;
	} else if (is_abbrev(buf, "create")) {
	    if (find_exdesc(argument, obj_p->ex_description)) {
		send_to_char("An extra description already exists with that keyword.\r\n"
			     "Use the 'olc rexdesc remove' command to remove it, or the\r\n"
			     "'olc rexdesc edit' command to change it, punk.\r\n", ch);
		return;
	    }
	    CREATE(ndesc, struct extra_descr_data, 1); 
	    ndesc->keyword  = str_dup(argument);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    ndesc->next = obj_p->ex_description;
	    obj_p->ex_description = ndesc;
	    send_to_char(TED_MESSAGE, ch);
	    ch->desc->str = &obj_p->ex_description->description;
	    ch->desc->max_str = MAX_STRING_LENGTH;
	    UPDATE_OBJLIST(obj_p, tmp_obj, ->ex_description);
	    SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	    act("$n begins to write an object description.", 
		TRUE, ch, 0, 0, TO_ROOM);
	    for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
		if (GET_OBJ_VNUM(tmp_obj) == GET_OBJ_VNUM(obj_p))
		    tmp_obj->ex_description = NULL;
      
	    return;
	} else if (is_abbrev(buf, "edit")) {
	    if ((desc = locate_exdesc(argument,obj_p->ex_description))) {
		send_to_char("Use TED to modify the object description.\r\n", ch);
		ch->desc->str = &desc->description;
		ch->desc->max_str = MAX_STRING_LENGTH;
		if (desc->description) {
		    ch->desc->editor_mode = 1;
		    ch->desc->editor_cur_lnum = get_line_count(desc->description);
		}
		UPDATE_OBJLIST(obj_p, tmp_obj, ->ex_description);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to write an object description.", 
		    TRUE, ch, 0, 0, TO_ROOM);
		for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
		    if (GET_OBJ_VNUM(tmp_obj) == GET_OBJ_VNUM(obj_p))
			tmp_obj->ex_description = NULL;
	    } else
		send_to_char("No such description.  Use 'create' to make a new one.\r\n", ch);

	    return;
	} else if (is_abbrev(buf, "addkeyword")) {
	    half_chop(argument, arg1, arg2);
	    if ((desc = locate_exdesc(arg1,obj_p->ex_description))) {
		if (!*arg2) {
		    send_to_char("What??  How about giving me some keywords to add...\r\n", ch);
		    return;
		} else {
		    strcpy(buf, desc->keyword);
		    strcat(buf, " ");
		    strcat(buf, arg2);
		    free(desc->keyword);
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    desc->keyword = str_dup(buf);
		    UPDATE_OBJLIST(obj_p, tmp_obj, ->ex_description);
		    send_to_char("Keywords added.\r\n", ch);
		    return;
		}
	    } else
		send_to_char("There is no such description on this object.\r\n", ch);
	} else
	    send_to_char(OLC_EXDESC_USAGE, ch);

	break;

    case 16:      /********** oload **************/
	if (!*argument) {
	    if (!obj_p) {
		send_to_char("Which object?\r\n", ch);
		return;
	    } else {
		j = obj_p->shared->vnum;
		tmp_obj = obj_p;
	    }
	} else {
	    skip_spaces(&argument);
	    if (!is_number(argument)) {
		send_to_char("The argument must be a vnum.\r\n", ch);
		return;
	    }
	    j = atoi(argument);
	    if (!(tmp_obj = real_object_proto(j))) {
		send_to_char("No such object exists.\r\n", ch);
		return;
	    }
	    if (j < (GET_ZONE(ch->in_room) * 100) || j > ch->in_room->zone->top) {
		send_to_char("You cannot olc oload objects from other zones.\r\n",ch);
		return;
	    }
	}
	if (!OLCIMP(ch) && !IS_SET(tmp_obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
	    send_to_char("You cannot olc oload approved items.\r\n", ch);
	    return;
	}
	if (!(tmp_obj = read_object(j)))
	    send_to_char("Unable to load object.\r\n", ch);
	else {
	    obj_to_char(tmp_obj, ch);
	    GET_OBJ_TIMER(tmp_obj) = GET_LEVEL(ch);
	    act("$p appears in your hands.", FALSE, ch, tmp_obj, 0, TO_CHAR);
	    act("$n creates $p in $s hands.",TRUE,ch,tmp_obj,0,TO_ROOM);
	    sprintf(buf, "OLC: %s oloaded [%d] %s.", GET_NAME(ch), 
		    GET_OBJ_VNUM(tmp_obj), tmp_obj->short_description);
	    slog(buf);
	}
	break;
    case 17:     /********** omimic ************/
	if (!*argument)  {
	    send_to_char("Usage: olc omimic <obj number>\r\n",ch);
	    break;
	}
	vnum = atoi(argument);
	tmp_obj = real_object_proto(vnum);
	if (!tmp_obj) {
	    send_to_char("That's not a valid object, genius.\r\n",ch);
	    break;
	}
	if (!obj_p) {
	    send_to_char("You need to have an object in your editing buffer first.\r\n", ch);
	    return;
	}
	if (tmp_obj == obj_p) {
	    send_to_char("Real funny.\r\n",ch);
	    break;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	if (obj_p->name)
	    free(obj_p->name);
	if (tmp_obj->name)
	    obj_p->name =                   str_dup(tmp_obj->name);
	else
	    obj_p->name =                   NULL;

	if (obj_p->short_description)
	    free(obj_p->short_description);
	if (tmp_obj->short_description)
	    obj_p->short_description =     str_dup(tmp_obj->short_description);
	else 
	    obj_p->short_description =     NULL;

	if (obj_p->description)
	    free(obj_p->description);
	if (tmp_obj->description)
	    obj_p->description =          str_dup(tmp_obj->description);
	else 
	    obj_p->description =          NULL;

	if (obj_p->action_description)
	    free(obj_p->action_description);
	if (tmp_obj->action_description)
	    obj_p->action_description =   str_dup(tmp_obj->action_description);
	else 
	    obj_p->action_description =   NULL;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	obj_p->obj_flags.type_flag =    tmp_obj->obj_flags.type_flag;
	obj_p->obj_flags.extra_flags =  tmp_obj->obj_flags.extra_flags;
	obj_p->obj_flags.extra2_flags = tmp_obj->obj_flags.extra2_flags;
	obj_p->obj_flags.wear_flags =   tmp_obj->obj_flags.wear_flags;
	obj_p->obj_flags.value[0] =     tmp_obj->obj_flags.value[0];
	obj_p->obj_flags.value[1] =     tmp_obj->obj_flags.value[1];
	obj_p->obj_flags.value[2] =     tmp_obj->obj_flags.value[2];
	obj_p->obj_flags.value[3] =     tmp_obj->obj_flags.value[3];
	obj_p->obj_flags.material =     tmp_obj->obj_flags.material;
	obj_p->obj_flags.max_dam =      tmp_obj->obj_flags.max_dam;
	obj_p->obj_flags.damage =       tmp_obj->obj_flags.damage;
	obj_p->setWeight( tmp_obj->getWeight() );
	obj_p->shared->cost =           tmp_obj->shared->cost;
	obj_p->shared->cost_per_day = tmp_obj->shared->cost_per_day;

	for (k = 0; k < 4; k++)
	    obj_p->obj_flags.bitvector[k] = tmp_obj->obj_flags.bitvector[k];

	for (k = 0; k < MAX_OBJ_AFFECT; k++) {
	    obj_p->affected[k].location = tmp_obj->affected[k].location;
	    obj_p->affected[k].modifier = tmp_obj->affected[k].modifier;
	}

	desc = obj_p->ex_description;
	obj_p->ex_description = NULL;
	while (desc)  {
	    if (desc->keyword)
		free(desc->keyword);
	    if (desc->description)
		free(desc->description);
	    desc = desc->next;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	desc = tmp_obj->ex_description;
	while (desc) {
	    CREATE(ndesc, struct extra_descr_data, 1);
	    ndesc->keyword = str_dup(desc->keyword);
	    ndesc->description = strdup(desc->description);
	    ndesc->next = obj_p->ex_description;
	    obj_p->ex_description = ndesc;
	    desc = desc->next;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	UPDATE_OBJLIST_FULL(obj_p, obj);
	
	send_to_char("Okay, done mimicing.\r\n",ch);
	break;

    case 18:     /************** clear ****************/
	if (!*argument) {
	    send_to_char("Usage: olc clear <room | obj>\r\n", ch);
	    return;
	}
	if (is_abbrev(argument, "room")) { 
	    do_clear_room(ch);
	} else if (is_abbrev(argument, "object")) {
	    if (!obj_p) {
		send_to_char("You are not currently editing an object.\r\n", ch);
	    } else 
		do_clear_olc_object(ch);
	} else
	    send_to_char("Olc clear what???!!\r\n", ch);
	break;

    case 19:  /*** supersave ***/
	if (GET_LEVEL(ch) < LVL_LUCIFER)
	    return;
	if (!*argument)
	    send_to_char("Usage: olc supersave <world | objects>\r\n", ch);
	else if (is_abbrev(argument, "world")) {
	    for (k = 0, zone = zone_table; zone; zone = zone->next) {
		if ((room = zone->world)) {
		    char_from_room(ch);
		    char_to_room(ch, room);
		    save_wld(ch);
		    sprintf( buf, "SAVEWLD Super: Zone %d, rooms %d-%d.\n",
			     zone->number, zone->number*100, zone->top);
		    slog( buf );
		    k++;
		}
	    }
	    sprintf(buf, "Done. %d zone saved.\r\n", k);
	    send_to_char(buf, ch);
	    return;
	} else if (is_abbrev(argument, "zones")) {
	    for (zone = zone_table, i = 0; zone; zone = zone->next, i++)
		save_zone(ch, zone);
	    sprintf(buf, "SAVEZON Super: %d zones saved.\r\n", i+1);
	    send_to_char(buf, ch);
	    return;

	} else if (is_abbrev(argument, "objects")) {
	    room = ch->in_room;
	    for (zone = zone_table, i = 0; zone; zone = zone->next, i++) {
		if ( zone->world ) {
		    ch->in_room = zone->world;
		    save_objs(ch);
		}
	    }

	    ch->in_room = room;
	    send_to_char("objs saved.\r\n", ch);
	    return;

	} else if (is_abbrev(argument, "shops")) {
	    room = ch->in_room;
	    for (zone = zone_table, i = 0; zone; zone = zone->next, i++)
		if (zone->world) {
		    ch->in_room = zone->world;
		    save_shops(ch);
		}
	    ch->in_room = room;
	    send_to_char("shops saved.\r\n", ch);
	    return;

	} else if (is_abbrev(argument, "mobiles")) {
	    room = ch->in_room;
	    for (zone = zone_table, i = 0; zone; zone = zone->next, i++)
		if (zone->world) {
		    ch->in_room = zone->world;
		    save_mobs(ch);
		}
	    ch->in_room = room;
	    send_to_char("mobs saved.\r\n", ch);
	    return;

	} else 
	    send_to_char("Not Implemented.\r\n", ch);

	break;

    case 20:   /*** create ***/
	if (!*argument)
	    send_to_char("Usage: olc create <room|zone|obj|mob|shop|help|ticl> <vnum|next>\r\n", ch);
	else {
	int tmp_vnum = 0;
	    argument = two_arguments(argument, arg1, arg2);
	    if (is_abbrev(arg1, "room")) {
			if (!*arg2) {
				send_to_char("Create a room with what vnum?\r\n", ch);
			} else if (is_abbrev(arg2, "next")) {
				for (i = ch->in_room->zone->number*100; i < ch->in_room->zone->top; i++) {
					if(!real_room(i)) {
						tmp_vnum = i;
						break;
					}
				}
			} else {
				i = atoi(arg2);
				tmp_vnum = i;
			}
			if (tmp_vnum && do_create_room(ch, tmp_vnum)) {
				sprintf(buf,"Room %d succesfully created.\r\n",tmp_vnum);
				send_to_char(buf, ch);
			} else if(!tmp_vnum && *arg2) {
					send_to_char("No allocatable rooms found in zone.\r\n",ch);
			}
	    } else if (is_abbrev(arg1, "zone")) {
		if (GET_LEVEL(ch) < LVL_IMPL) {
		    send_to_char("You cannot create zones.\r\n", ch);
		    return;
		}
		if (!*arg2)
		    send_to_char("Create a zone with what number?\r\n", ch);
		else {
		    i = atoi(arg2);
		    if (do_create_zone(ch, i))
			send_to_char("Zone succesfully created.\r\n", ch);
		}
	    } else if (is_abbrev(arg1, "object")) {
			if (!*arg2) {
				send_to_char("Create an obj with what vnum?\r\n", ch);
			} else if (is_abbrev(arg2, "next")) {
				for (i = ch->in_room->zone->number*100; i < ch->in_room->zone->top; i++) {
					if(!real_object_proto(i)) {
						tmp_vnum = i;
						break;
					}
				}
			} else {
				tmp_vnum = atoi(arg2);
			}
			if (tmp_vnum && (tmp_obj = do_create_obj(ch, tmp_vnum))) {
				GET_OLC_OBJ(ch) = tmp_obj;
				sprintf(buf,"Object %d succesfully created.\r\nNow editing object %d\r\n",
					tmp_obj->shared->vnum,tmp_obj->shared->vnum);
				send_to_char(buf,ch);
			} else if (!tmp_vnum && *arg2) {
				send_to_char("No allocatable objects found in zone.\r\n",ch);
			}
	    } else if (is_abbrev(arg1, "mobile")) {
			if (!*arg2) {
				send_to_char("Create a mob with what vnum?\r\n", ch);
			} else if (is_abbrev(arg2, "next")) {
				for (i = ch->in_room->zone->number*100; i < ch->in_room->zone->top; i++) {
					if(!real_mobile_proto(i)) {
						tmp_vnum = i;
						break;
					}
				}
			} else {
				tmp_vnum = atoi(arg2);
			        i = tmp_vnum;
                        }
			if (tmp_vnum && (tmp_mob = do_create_mob(ch, i))) {
				GET_OLC_MOB(ch) = tmp_mob;
				sprintf(buf,"Mobile %d succesfully created.\r\nNow editing mobile %d\r\n",
					tmp_vnum,tmp_vnum);
				send_to_char(buf,ch);
			} else if (!tmp_vnum && *arg2) {
				send_to_char("No allocatable mobiles found in zone.\r\n",ch);
			}
	    } else if (is_abbrev(arg1, "shop")) {
		if (!*arg2)
		    send_to_char("Create a shop with what vnum?\r\n", ch);
		else {
		    i = atoi(arg2);
		    if (do_create_shop(ch, i))
			send_to_char("Shop succesfully created.\r\n", ch);
		}
	    } else if (is_abbrev(arg1, "search")) {
			if ((tmp_search = do_create_search(ch, strcat(arg2, argument)))) {
				GET_OLC_SRCH(ch) = tmp_search;
				send_to_char("Search creation successful.\r\n", ch);
				sprintf(buf,"Now editing search (%s)/(%s)\r\n",tmp_search->command_keys,tmp_search->keywords);
				send_to_char(buf,ch);
			}
	    } else if (is_abbrev(arg1, "path") && OLCIMP(ch)) {
		if (!add_path(strcat(arg2, argument), TRUE))
		    send_to_char("Path added.\r\n", ch);
	    } else if (is_abbrev(arg1, "help")) {
		do_create_help(ch);
	    } else if (is_abbrev(arg1, "ticl")) {
		if (!*arg2)
		    send_to_char("Create a TICL with what vnum?\r\n", ch);
		else {
		    i = atoi(arg2);
		    if (do_create_ticl(ch, i))
			send_to_char("TICL succesfully created.\r\n", ch);
		}
	    } else
		send_to_char("Usage: olc create <room|zone|obj|mob|shop|help|ticl> <vnum>\r\n", 
			     ch);	
	}
	break;
    
    case 21: /**** destroy ****/
	if (!*argument) {
	    send_to_char("Usage: olc destroy <room|zone|obj|mob|shop> <vnum>\r\n", 
			 ch);
	    return;
	}
	argument = two_arguments(argument, arg1, arg2);

	if (is_abbrev(arg1, "search")) {
	    do_destroy_search(ch, strcat(arg2, argument));
	    return;
	}

	if (GET_LEVEL(ch) < LVL_IMPL) {
	    send_to_char("You are not authorized to destroy, hosehead.\r\n", ch);
	    return;
	}

	if (is_abbrev(arg1, "room")) {
	    if (!*arg2)
		i = ch->in_room->number;
	    else 
		i = atoi(arg2);
	  
	    if (!do_destroy_room(ch, i))
		send_to_char("Room eliminated.\r\n", ch);
	
	} else if (is_abbrev(arg1, "object")) {
	    if (!*arg2) {
		if (!GET_OLC_OBJ(ch)) {
		    send_to_char("Destroy what object prototype?\r\n", ch);
		    return;
		}
		i = GET_OBJ_VNUM(GET_OLC_OBJ(ch));
	    } else 
		i = atoi(arg2);

	    if (!do_destroy_object(ch, i))
		send_to_char("Object eliminated.\r\n", ch);
	
	} else if (is_abbrev(arg1, "mobile")) {
	    if (!*arg2) {
		if (!GET_OLC_MOB(ch)) {
		    send_to_char("Destroy what mobile prototype?\r\n", ch);
		    return;
		}
		i = GET_MOB_VNUM(GET_OLC_MOB(ch));
	    } else
		i = atoi(arg2);
	
	    if (!do_destroy_mobile(ch, i))
		send_to_char("Mobile eliminated.\r\n", ch);

	} else if (is_abbrev(arg1, "shop")) {
	    if (!*arg2) {
		if (!GET_OLC_SHOP(ch)) {
		    send_to_char("Destroy what shop prototype?\r\n", ch);
		    return;
		}
		shop = GET_OLC_SHOP(ch);
		i = shop->vnum;
	    } else
		i = atoi(arg2);
	
	    if (!do_destroy_shop(ch, i))
		send_to_char("Shop eliminated.\r\n", ch);

	} else if (is_abbrev(arg1, "zone")) {
	    send_to_char("Command not imped yet.\r\n", ch);
	} else
	    send_to_char("Unknown.\r\n", ch);
	break;
    case 22:
	if (!*argument) {
	    page_string(ch->desc, OLC_ZSET_USAGE, 1);
	    return;
	}
	else
	    do_zset_command(ch, argument);
	break;
    case 23: /* zcmd */
	if (!*argument) {
	    page_string(ch->desc, OLC_ZCMD_USAGE, 1);
	    return;
	}
	else
	    do_zcmd(ch, argument);
	break;
    case 24:         /******** zsave ********/
	if (!*argument)
	    if (!save_zone(ch, ch->in_room->zone))
		send_to_char("Zone saved to disk.\r\n", ch);
	    else
		send_to_char("SYSERR: Could not save zone!!\r\n", ch);
	else if (is_number(argument)) {
	    i = atoi(argument);
	    for (found = 0, zone = zone_table; zone && found != 1; zone = zone->next)
		if (zone->number == i) {
		    if (!save_zone(ch, zone))
			send_to_char("Zone saved to disk.\r\n", ch);
		    else
			send_to_char("SYSERR: Could not save zone to disk!!\r\n", ch);
		    found = 1;
		}
	    if (found != 1)
		send_to_char("Save which zone?\r\n", ch);
	}
	else
	    send_to_char("Save which zone?\r\n", ch);
	break;
    case 25:              /* zmob */
	if (!*argument)
	    send_to_char("Type olc for usage.\r\n", ch);
	else
	    do_zmob_cmd(ch, argument);
	break;         
    case 26:              /* zobj */
	if (!*argument)
	    send_to_char("Type olc for usage.\r\n", ch);
	else
	    do_zobj_cmd(ch, argument);
	break;
    case 27:              /* zdoor */
	if (!*argument)
	    send_to_char("Type olc for usage.\r\n", ch);
	else
	    do_zdoor_cmd(ch, argument);
	break;
    case 28:              /* zreset */
	do_zreset(ch, ".", 0, SCMD_OLC);
	break;
    case 29:              /* zonepurge */
	sprintf(buf, " %d", ch->in_room->zone->number);
	do_zonepurge(ch, buf, 0, SCMD_OLC);
	break;
    case 30:              /* zequip */
	do_zequip_cmd(ch, argument);
	break;
    case 31:              /* zput */
	do_zput_cmd(ch, argument);
	break;
    case 32:              /* zgive */
	do_zgive_cmd(ch, argument);
	break;
    case 33:              /* medit */
	do_mob_medit(ch, argument);
	break;
    case 34:              /* mstat */
	do_mob_mstat(ch);
	break;
    case 35:              /* mset */
	do_mob_mset(ch, argument);
	break;
    case 36:              /* msave */
	if(!save_mobs (ch))
	    send_to_char("Mobile file saved.\r\n",ch);
	else
	    send_to_char("An error occured while saving.\r\n",ch);
	break;
    case 37:              /* sedit */
	do_shop_sedit(ch, argument);
	break;
    case 38:              /* sstat */
	do_shop_sstat(ch);
	break;
    case 39:              /* sset */
	do_shop_sset(ch, argument);
	break;
    case 40:
	if(!save_shops(ch))
	    send_to_char("Shop file saved.\r\n",ch);
	else
	    send_to_char("An error occured while saving.\r\n",ch);
	break;
    case 41:              /* zimplant */
	do_zimplant_cmd(ch, argument);
	break;
    case 42:
	do_help_fedit(ch, argument);
	break;
    case 43:
	if (!GET_OLC_MOB(ch))
	    send_to_char("yOu beTteR oLC MedIt a mObiLe fIRsT, baBy.\r\n", ch);
	else if (!*argument)
	    send_to_char("Mmimic wHiCH mOb?!\r\n", ch);
	else if (!(mob = real_mobile_proto(atoi(argument))))
	    send_to_char("No sUcH mObiLE eSiSTs, foOl!\r\n", ch);
	else if (mob == GET_OLC_MOB(ch))
	    send_to_char("wHooah.. geT a griP, aYe?\r\n", ch);
	else if (olc_mimic_mob(ch, mob, GET_OLC_MOB(ch), TRUE))
	    send_to_char("dUde...  cOulDn't dO it.\r\n", ch);
	else
	    send_to_char("aLriGHtY thEn.\r\n", ch);
	break;
    case 44:
	do_help_list(ch);
	break;
    case 45:
	do_help_hedit(ch, argument);
	break;
    case 46:
	do_help_hstat(ch);
	break; 
    case 47:
	do_help_hset(ch, argument);
	break;
    case 48:
	do_zpath_cmd(ch, argument);
	break;
    case 49:
	if (!*argument) {
	    if (!GET_OLC_SRCH(ch))
		send_to_char("Usage: olc xedit <command trigger> <keyword>\r\n", ch);
	    else {
		sprintf(buf,
			"You are currently editing a search that triggers on:\r\n"
			"%s (%s)\r\n", GET_OLC_SRCH(ch)->command_keys,
			GET_OLC_SRCH(ch)->keywords);
		send_to_char(buf, ch);
	    }
	    return;
	} 
	set_char_xedit(ch, argument);
	break;
    case 50:  /* xset */
	do_olc_xset(ch, argument);
	break;
    case 51:  /* xstat */
	do_olc_xstat(ch);
	break;
    case 52:              /* tedit */
	do_ticl_tedit(ch, argument);
	break;
    case 53:              /* tstat */
	do_ticl_tstat(ch);
	break;
    case 54:              /* tset */
/*    do_ticl_tset(ch, argument); */
	break;
    case 55:              /* tsave */
/*    if(!save_ticls (ch))
      send_to_char("Mobile file saved.\r\n",ch);
      else
      send_to_char("An error occured while saving.\r\n",ch);
      break;  */
    case 56:
	if ( ! *argument ) {
            if ( ! mob_p ) {
                send_to_char( "Which mobile?\r\n", ch );
                return;
            } else {
                tmp_mob = mob_p;
                j = GET_MOB_VNUM( tmp_mob );
            } 
        } else {
	    skip_spaces( &argument );
	    if ( ! is_number( argument ) ) {
		send_to_char( "The argument must be a vnum.\r\n", ch );
		return;
	    }
	    j = atoi( argument );
				
	    if ( ! ( tmp_mob = real_mobile_proto( j ) ) ) {
		send_to_char( "No such mobile exists.\r\n", ch );
		return;
	    }   
			
	    if ( j < ( GET_ZONE( ch->in_room ) * 100 ) || j > ch->in_room->zone->top ) {
		send_to_char( "You cannot olc mload mobiles from other zones.\r\n",ch );
		return;
	    }
	}


	if ( ! OLCIMP( ch ) && ! MOB2_FLAGGED( tmp_mob, MOB2_UNAPPROVED ) ) {
	    send_to_char( "You cannot olc mload approved mobiles.\r\n", ch );
	    return;
	}

	if ( ! ( tmp_mob = read_mobile( j ) ) ){
	    send_to_char( "Unable to load mobile.\r\n", ch );
	} else {
	    char_to_room( tmp_mob, ch->in_room );
	    act( "$N appears next to you.", FALSE, ch, 0, tmp_mob, TO_CHAR );
	    act( "$n creates $N in $s hands.",TRUE, ch, 0, tmp_mob, TO_ROOM );
	    sprintf( buf, "OLC: %s mloaded [%d] %s.", GET_NAME( ch ), GET_MOB_VNUM( tmp_mob ), GET_NAME( tmp_mob ) );
	    slog( buf );
	}

	break;

	//
	// olc show
	//

    case 57:
	if ( !*argument ) {
	    send_to_char( OLC_SHOW_USAGE, ch );
	    return;
	}

	argument = one_argument( argument, arg1 );

	skip_spaces( &argument );

	// search for rooms with no sound
	if ( is_abbrev( arg1, "nosound" ) ) {
	    sprintf( buf, "Rooms with no sound in zone %d:\n", ch->in_room->zone->number );
	    for ( struct room_data *room = ch->in_room->zone->world; room; room = room->next ) {
		if ( !room->sounds ) {
		    sprintf( buf, "%s[ %6d ] %s\n", buf, room->number, room->name );
		}
	    }
	    
	    page_string( ch->desc, buf, 1 );
	    return;
	}

	if ( is_abbrev( arg1, "noexitdesc" ) ) {

	    //
	    // modes
	    // mode 1 = ONLY exits with no toroom 
	    // mode 2 = ONLY exits with a toroom
	    // mode 3 = ALL exits
	    //

	    int mode = 1;

	    if ( !strcmp( argument, "toroom" ) )
		mode = 2;
	    else if ( !strcmp( argument, "all" ) )
		mode = 3;

	    sprintf( buf, "Exits with no description in zone %d:\n", ch->in_room->zone->number );
	    for ( struct room_data *room = ch->in_room->zone->world; room; room = room->next ) {
		sprintf( buf2, " [ %6d ] %-55s : [ ", room->number, room->name );
		bool found = false;

		for ( int i = 0; i < FUTURE; ++i ) {
		    struct room_direction_data *exit = room->dir_option[ i ];
		    
		    if ( ( mode == 3 && ( !exit || !exit->general_description ) ) ||
			 ( mode == 1 && ( !exit || ( !exit->to_room && !exit->general_description ) ) ) ||
			 ( mode == 2 && exit && exit->to_room && !exit->general_description ) ) {

			found = true;
			if ( exit && exit->to_room )
			    sprintf( buf2, "%s%s%c%s ", buf2, CCGRN( ch, C_NRM ), dirs[i][0], CCNRM( ch, C_NRM ) );
			else
			    sprintf( buf2, "%s%s%c%s ", buf2, CCYEL( ch, C_NRM ), dirs[i][0], CCNRM( ch, C_NRM ) );
		    }
		    else strcat( buf2, "  " );
		}

		if ( found ) {
		    strcat( buf2,  "]\n" );
		    strcat( buf, buf2 );
		}
	    }
	    
	    page_string( ch->desc, buf, 1 );
	    return;
	}

	sprintf( buf, "Unknown option: %s\n", arg1 );
	send_to_char( buf, ch );
	break;

    default:
	send_to_char("This action is not supported yet.\r\n",ch);
    }
}


const char *olc_help_keys[] = {
    "rflags",
    "rsector",
    "rflow",
    "doorflags",
    "otypes",
    "oextra1",
    "oextra2",
    "wearflags",
    "liquids",
    "apply",
    "races",
    "aff",
    "aff2",
    "aff3",
    "values",
    "spells",
    "attacktypes",
    "chemicals",
    "zflags",
    "materials",
    "bombs",
    "fuses",
    "mflags",
    "mflags2",
    "position",
    "sex",
    "char_class",
    "itemtypes",
    "sflags",
    "tradewith",
    "implants",
    "smokes",
    "temper",
    "vehicles",
    "engines",
    "searches",
    "guntypes",
    "interfaces",
    "microchips",
    "searchflags",
    "oextra3",
    "\n"
};

#define NUM_OLC_HELPS   41
#define NUM_SHOP_TEMPER 6
#define NUM_SHOP_FLAGS 4

void show_olc_help(struct char_data *ch, char *arg)
{

    int i = 0, which_help, j = 0;
    char smallbuf[30];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 
    half_chop(arg, arg1, arg2);
    if (!*arg1) {
	strcpy(buf, "Valid help keywords:\r\n");
	strcat(buf, CCYEL(ch, C_NRM));
	while (i < NUM_OLC_HELPS) {
	    strcat(buf, olc_help_keys[i]);
	    strcat(buf, "\r\n");
	    i++;
	}
	strcat(buf, CCNRM(ch, C_NRM));
	page_string(ch->desc, buf, 1);
	return;
    } else if ((which_help = search_block(arg1, olc_help_keys, FALSE)) <0) {
	send_to_char("No such keyword.  Type 'olc help' for a list of valid keywords.\r\n", ch);
	return;
    }
    switch (which_help) {
    case 0:            /******* rflags ********/
	strcpy(buf, "ROOM FLAGS:\r\n");
	for (i = 0; i < NUM_ROOM_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%-10s %-10s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    room_bits[i], roomflag_names[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 1:           /********* rsector **********/
	strcpy(buf, "ROOM SECTOR TYPES:\r\n");
	for (i = 0; i < NUM_SECT_TYPES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", i, CCCYN(ch, C_NRM), 
		    sector_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 2:
	strcpy(buf, "ROOM FLOW TYPES:\r\n");
	for (i = 0; i < NUM_FLOW_TYPES; i++) {
	    sprintf(buf2, "%2d         %s%-20s%s  '%s'\r\n", i, CCCYN(ch, C_NRM), 
		    flow_types[i], CCNRM(ch, C_NRM), char_flow_msg[i][2]);
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 3:
	strcpy(buf, "DOOR FLAGS:\r\n");
	for (i = 0; i < NUM_DOORFLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    exit_bits[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 4:
	strcpy(buf, "ITEM TYPES:\r\n");
	for (i = 0; i < NUM_ITEM_TYPES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", i, CCCYN(ch, C_NRM), 
		    item_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 5:
	strcpy(buf, "OBJ EXTRA1 FLAGS:\r\n");
	for (i = 0; i < NUM_EXTRA_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%-10s %-10s %s\r\n", 
		    smallbuf, CCCYN(ch, C_NRM),
		    extra_bits[i], extra_names[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 6:
	strcpy(buf, "OBJ EXTRA2 FLAGS:\r\n");
	for (i = 0; i < NUM_EXTRA2_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%-10s %-10s%s\r\n", 
		    smallbuf, CCCYN(ch, C_NRM),
		    extra2_bits[i], extra2_names[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 7:
	strcpy(buf, "OBJ WEAR FLAGS:\r\n");
	for (i = 0; i < NUM_WEAR_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    wear_bits[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 8:
	strcpy(buf, "##  LIQUID            Drunk     Hunger   Thirst\r\n");
	for (i = 0; i < NUM_LIQUID_TYPES; i++) {
	    sprintf(buf2, "%2d  %s%-20s%s  %3d   %3d  %3d\r\n", 
		    i, CCCYN(ch, C_NRM), drinks[i], CCNRM(ch, C_NRM),
		    (int)drink_aff[i][0], (int)drink_aff[i][1], 
		    (int)drink_aff[i][2]);
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 9:
	strcpy(buf, "ITEM APPLIES:\r\n");
	for (i = 0; i < NUM_APPLIES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), apply_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 10:
	strcpy(buf, "RACES:\r\n");
	for (i = 0; i < NUM_RACES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), player_race[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 11:
	strcpy(buf, "AFF FLAGS:\r\n");
	for (i = 0; i < NUM_AFF_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    affected_bits_desc[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 12:
	strcpy(buf, "AFF2 FLAGS:\r\n");
	for (i = 0; i < NUM_AFF2_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    affected2_bits_desc[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 13:
	strcpy(buf, "AFF3 FLAGS:\r\n");
	for (i = 0; i < NUM_AFF3_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch, C_NRM),
		    affected3_bits_desc[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 14:
	if (*arg2) {
	    if (!is_number(arg2)) {
		if ((i = search_block(arg2, item_types, 0)) < 0) {
		    send_to_char("Type olc help otypes for a valid list.\r\n", ch);
		    return;
		}
	    } else 
		i = atoi(arg2);
	    if (i < 0 || i > NUM_ITEM_TYPES) {
		send_to_char("Object type out of range.\r\n", ch);
		return;
	    } 
	    sprintf(buf, "##          Type       %sValue 0      Value 1"
		    "      Value 2      Value 3%s\r\n", 
		    CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	    sprintf(buf, "%s%2d  %s%12s%s  %12s %12s %12s %12s\r\n", buf,
		    i, CCCYN(ch, C_NRM), item_types[i], CCNRM(ch, C_NRM), 
		    item_value_types[i][0], item_value_types[i][1], 
		    item_value_types[i][2], item_value_types[i][3]);
	    send_to_char(buf, ch);
	    return;
	}

	sprintf(buf, "##          Type       %sValue 0      Value 1"
		"      Value 2      Value 3%s\r\n", 
		CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	for (i = 0; i < NUM_ITEM_TYPES; i++) {
	    sprintf(buf, "%s%2d  %s%12s%s  %12s %12s %12s %12s\r\n", buf,
		    i, CCCYN(ch, C_NRM), item_types[i], CCNRM(ch, C_NRM), 
		    item_value_types[i][0], item_value_types[i][1], 
		    item_value_types[i][2], item_value_types[i][3]);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 15:       /** spells **/
	if (*arg2) {
	    if (!is_number(arg2)) {
		if ((i = search_block(arg2, spells, 0)) < 0) {
		    send_to_char("Type olc help spells for a valid list.\r\n", ch);
		    return;
		}
	    } else 
		i = atoi(arg2);
	    if (i < 0 || i > TOP_SPELL_DEFINE) {
		send_to_char("Spell num out of range.\r\n", ch);
		return;
	    } 
	    sprintf(buf, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), spells[i], CCNRM(ch, C_NRM));
	    send_to_char(buf, ch);
	    return;
	}

	strcpy(buf, "SPELLS:\r\n");
	for (i = 1; i < TOP_NPC_SPELL; i++) {
	    if ( strcmp( spells[i], "!UNUSED!") ) {
		sprintf(buf2, "%3d         %s%s%s\r\n", 
			i, CCCYN(ch, C_NRM), spells[i], CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}/*
	   for (i = MAX_SKILLS+1; i < TOP_NPC_SPELL; i++) {
	   if (str_cmp(spells[i], "!UNUSED!")) {
	   sprintf(buf2, "%2d         %s%s%s\r\n", 
	   i, CCCYN(ch, C_NRM), spells[i], CCNRM(ch, C_NRM));
	   strcat(buf, buf2);
	   }
	   }*/
	page_string(ch->desc, buf, 1);
	break;

    case 16:
	strcpy(buf, "ATTACKTYPES:\r\n");
	for (i = 0; i < (TOP_ATTACKTYPE - TOP_SPELL_DEFINE - 1); i++) {
	    sprintf(buf2, "%2d         %s%10s  %-10s%s\r\n", 
		    i, CCCYN(ch, C_NRM), attack_hit_text[i].singular, 
		    attack_hit_text[i].plural, CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}

	page_string(ch->desc, buf, 1);
	break;

    case 17: /** chemicals **/
	send_to_char("There are no more chemical types.\r\n", ch);
	break;

    case 18:
	if (OLCIMP(ch))
	    j = TOT_ZONE_FLAGS;
	else
	    j = NUM_ZONE_FLAGS;
	strcpy(buf, "ZONE FLAGS:\r\n");
	for (i = 0; i < j; i++) {
	    if (!is_number( zone_flags[i])) {
		sprintf(buf2, "%2d         %s%s%s\r\n", 
			i, CCCYN(ch, C_NRM), zone_flags[i], CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}
	page_string(ch->desc, buf, 1);
	break;    
    case 19:   /* materials */
	if (*arg2) {
	    if (!is_number(arg2)) {
		if ((i = search_block(arg2, material_names, 0)) < 0) {
		    send_to_char("Type olc help materials for a valid list.\r\n", ch);
		    return;
		}
	    } else 
		i = atoi(arg2);
	    if (i < 0 || i > TOP_MATERIAL) {
		send_to_char("Material out of range.\r\n", ch);
		return;
	    } 
	    sprintf(buf, "%2d         %s%s%s\r\n", 
		    i, CCYEL(ch, C_NRM), material_names[i], CCNRM(ch, C_NRM));
	    send_to_char(buf, ch);
	    return;
	}

	strcpy(buf, "MATERIALS:\r\n");
	for (i = 0; i < TOP_MATERIAL; i++) {
	    if (str_cmp(material_names[i], "*")) {
		sprintf(buf2, "%2d         %s%s%s\r\n", 
			i, CCYEL(ch, C_NRM), material_names[i], CCNRM(ch, C_NRM));
		if ((strlen(buf2) + strlen(buf) + 128) > MAX_STRING_LENGTH) {
		    strcat(buf2, "***OVERFLOW***\r\n");
		    break;
		}
		strcat(buf, buf2);
	    }
	}
	page_string(ch->desc, buf, 1);
	break;

    case 20:           /** bomb types **/
	strcpy(buf, "BOMB TYPES:\r\n");
	for (i = 0; i < MAX_BOMB_TYPES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), bomb_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;    
    case 21:           /** fuse types **/
	strcpy(buf, "FUSE TYPES:\r\n");
	for (i = 0; i <= FUSE_MOTION; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), fuse_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;    
    case 22:           /** mob flags **/
	strcpy(buf, "MOB FLAGS:\r\n");
	for (i = 0; i < NUM_MOB_FLAGS; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n",
		    i, CCCYN(ch, C_NRM), action_bits_desc[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 23:           /** mob flags2 **/
	strcpy(buf, "MOB FLAGS2:\r\n");
	for (i = 0; i < NUM_MOB2_FLAGS; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n",
		    i, CCCYN(ch, C_NRM), action2_bits_desc[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 24:           /** position **/
	strcpy(buf, "POSITIONS:\r\n");
	for (i = 0; i <= NUM_POSITIONS; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n",
		    i, CCCYN(ch, C_NRM), position_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 25:           /** sex **/
	strcpy(buf, "GENDERS:\r\n");
	for (i = 0; i <= 2; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n",
		    i, CCCYN(ch, C_NRM), genders[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 26:           /** char_class **/
	strcpy(buf, "CLASSES:\r\n");
	for (i = 0; i < TOP_CLASS; i++) {
	    if (str_cmp(pc_char_class_types[i], "ILL")) {
		sprintf(buf2, "%2d         %s%s%s\r\n", 
			i, CCCYN(ch, C_NRM), pc_char_class_types[i], CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}
	page_string(ch->desc, buf, 1);
	break;
    case 27:           /** itemtypes **/
	strcpy(buf, "ITEM TYPES:\r\n");
	for (i = 0; i < NUM_ITEM_TYPES; i++) {
	    if (str_cmp(item_types[i], "ILL")) {
		sprintf(buf2, "%2d         %s%s%s\r\n", 
			i, CCCYN(ch, C_NRM), item_types[i], CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}
	page_string(ch->desc, buf, 1);
	break;
    case 28:           /** sflags **/
	strcpy(buf, "SHOP FLAGS:\r\n");
	for (i = 0; i < NUM_SHOP_FLAGS; i++) {
	    if (str_cmp(shop_bits[i], "ILL")) {
		sprintf(buf2, "%2d         %s%s%s\r\n", 
			i, CCCYN(ch, C_NRM), shop_bits[i], CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}
	page_string(ch->desc, buf, 1);
	break;
    case 29:           /** tradewith **/
	strcpy(buf, "TRADE WITH:\r\n");
	for (i = 0; i < NUM_TRADE_WITH; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), trade_letters[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 30:  /** implantpos **/
	strcpy(buf, "IMPLANT POSITIONS:\r\n");
	for (i = 0; i < NUM_WEARS; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), wear_implantpos[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 31:  /** smokes **/
	strcpy(buf, "SMOKE TYPES:\r\n");
	for (i = 0; i < NUM_SMOKES; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), smoke_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 32:  /** temper **/
	strcpy(buf, "SHOP TEMPER:\r\n");
	for (i = 0; i < NUM_SHOP_TEMPER; i++) {
	    sprintf(buf2, "%2d         %s%s%s\r\n", 
		    i, CCCYN(ch, C_NRM), temper_str[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 33:  /** vehicles **/
	strcpy(buf, "VEHICLE TYPE BITZ:\r\n");
	for (i = 0; i < NUM_VEHICLE_TYPES; i++) {
	    sprintf(buf2, "%4d         %s%s%s\r\n", 
		    (1 << i), CCCYN(ch, C_NRM), vehicle_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 34:  /** engines **/
	strcpy(buf, "ENGINE BITZ:\r\n");
	for (i = 0; i < NUM_ENGINE_FLAGS; i++) {
	    sprintf(buf2, "%4d         %s%s%s\r\n", 
		    (1 << i), CCCYN(ch, C_NRM), engine_flags[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    
    case 35:   /** searches **/
	send_to_char(SEARCH_USAGE, ch);
	break;

    case 36:  /** guntypes **/
	strcpy(buf, "GUN TYPES:\r\n");
	for (i = 0; i < NUM_GUN_TYPES; i++) {
	    sprintf(buf2, 
		    "%2d     %s%15s%s ----  %2d d %-3d  (avg. %3d, max %3d)\r\n", 
		    i, CCCYN(ch, C_NRM), gun_types[i], CCNRM(ch, C_NRM),
		    gun_damage[i][0], gun_damage[i][1],
		    (gun_damage[i][0]*(gun_damage[i][1] + 1)) / 2,
		    gun_damage[i][0]*gun_damage[i][1]);
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 37:  /** interfaces **/
	strcpy(buf, "INTERFACE TYPES:\r\n");
	for (i = 0; i < NUM_INTERFACES; i++) {
	    sprintf(buf2, 
		    "%2d     %s%10s%s\r\n",
		    i, CCCYN(ch, C_NRM), interface_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 38:  /**  microchips **/
	strcpy(buf, "MICROCHIP TYPES:\r\n");
	for (i = 0; i < NUM_CHIPS; i++) {
	    sprintf(buf2, 
		    "%2d     %s%10s%s\r\n",
		    i, CCCYN(ch, C_NRM), microchip_types[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    case 39:  /** searchflags **/
	strcpy(buf, "SEARCH BITS:\r\n");
	for (i = 0; i < NUM_SRCH_BITS; i++) {
	    sprintf(buf2, "%8d         %s%s%s\r\n", 
		    (1 << i), CCCYN(ch, C_NRM), search_bits[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;
    case 40:
	strcpy(buf, "OBJ EXTRA3 FLAGS:\r\n");
	for (i = 0; i < NUM_EXTRA3_FLAGS; i++) {
	    num2str(smallbuf, (1 << i));
	    sprintf(buf2, "  %s         %s%-10s %-10s%s\r\n", 
		    smallbuf, CCCYN(ch, C_NRM),
		    extra3_bits[i], extra3_names[i], CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}
	page_string(ch->desc, buf, 1);
	break;

    default:
	send_to_char("There is no help on this word yet.  Maybe you should write it.\r\n", ch);
    }
    return;
}

#define UNAPPR_USE       "Usage: approve <obj | mob | zone> <vnum>\r\n"

ACMD(do_unapprove)
{

    int rnum = NOTHING;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    byte o_m = 0, zn = 0;
    struct obj_data *obj = NULL;
    struct zone_data *zone = NULL;
    struct char_data *mob = NULL;

    half_chop(argument, arg1, arg2);

    if (*arg1) {
	if (is_abbrev(arg1, "object"))
	    o_m = 0;
	else if (is_abbrev(arg1, "mobile"))
	    o_m = 1;
	else if (is_abbrev(arg1, "zone"))
	    zn = 1;
	else {
	    send_to_char(UNAPPR_USE, ch);
	    return;
	}
    } else {
	send_to_char(UNAPPR_USE, ch);
	return;
    }
  
    if (*arg2) 
	rnum = atoi(arg2);
    else {
	send_to_char(UNAPPR_USE, ch);
	return;
    }

    if (zn) {       /********* unapprove zone *********/
	if (!strncmp(arg2, ".", 1))
	    zone = ch->in_room->zone;
	else if (!(zone = real_zone(rnum))) {
	    send_to_char("No such zone.\r\n", ch);
	    return;
	}

	send_to_char("Zone approved for olc.\r\n", ch);
	sprintf(buf, "%s approved zone [%d] %s for OLC.", GET_NAME(ch),
		zone->number, zone->name);
	slog(buf);

	SET_BIT(zone->flags, ZONE_MOBS_APPROVED |
		   ZONE_OBJS_APPROVED |
		   ZONE_ROOMS_APPROVED |
		   ZONE_ZCMDS_APPROVED |
		   ZONE_SEARCH_APPROVED |
		   ZONE_SHOPS_APPROVED);
	save_zone(ch, zone);
	return;
    }
    
    if (!o_m) {     /********* unapprove objects ******/
	obj = real_object_proto(rnum);
    
	if (!obj) {
	    send_to_char("There exists no object with that number, slick.\r\n", ch);
	    return;
	}

	for (zone = zone_table; zone; zone = zone->next) {
	    if (obj->shared->vnum >= (zone->number * 100) &&
		obj->shared->vnum <= zone->top)
		break;
	}

	if (!zone) {
	    send_to_char("ERROR: That object does not belong to any zone.\r\n", ch);
	    return;
	}

	if (!CAN_EDIT_ZONE(ch, zone) && !OLCIMP(ch) && !OLCGOD( ch )) {
	    send_to_char("You can't unapprove this, BEANHEAD!\r\n", ch);
	    return;
	}
    
	if (IS_SET(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
	    send_to_char("That item is already unapproved.\r\n", ch);
	    return;
	}

	SET_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
	send_to_char("Object unapproved.\r\n", ch);
	sprintf(buf, "%s unapproved object [%d] %s.", GET_NAME(ch),
		obj->shared->vnum, obj->short_description);
	slog(buf);

	GET_OLC_OBJ(ch) = obj;
	save_objs(ch);
	GET_OLC_OBJ(ch) = NULL;

    } else {     /** approve mobs */
	mob = real_mobile_proto(rnum);
    
	if (!mob) {
	    send_to_char("There exists no mobile with that number, slick.\r\n", ch);
	    return;
	}

	for (zone = zone_table; zone; zone = zone->next) {
	    if (GET_MOB_VNUM(mob) >= (zone->number * 100) &&
		GET_MOB_VNUM(mob) <= zone->top)
		break;
	}

	if (!zone) {
	    send_to_char("ERROR: That object does not belong to any zone.\r\n", ch);
	    return;
	}

	if (!CAN_EDIT_ZONE(ch, zone) && !OLCIMP(ch) && !OLCGOD( ch ) ) {
	    send_to_char("You can't unapprove this, BEANHEAD!\r\n", ch);
	    return;
	}
    
	if (IS_SET(MOB2_FLAGS(mob), MOB2_UNAPPROVED)) {
	    send_to_char("That mobile is already unapproved.\r\n", ch);
	    return;
	}

	SET_BIT(MOB2_FLAGS(mob), MOB2_UNAPPROVED);
	send_to_char("Mobile unapproved.\r\n", ch);
	sprintf(buf, "%s unapproved mobile [%d] %s.", GET_NAME(ch),
		rnum, GET_NAME(mob));
	slog(buf);

	GET_OLC_MOB(ch) = mob;
	save_mobs(ch);
	GET_OLC_MOB(ch) = NULL;
    }
}


#define APPR_USE       "Usage: approve <obj | mob | zone> <vnum>\r\n"
ACMD(do_approve)
{

    int rnum = NOTHING;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    byte o_m = 0, zn = 0;
    struct obj_data *obj = NULL;
    struct zone_data *zone = NULL;
    struct char_data *mob = NULL;

    half_chop(argument, arg1, arg2);

    if (*arg1) {
	if (is_abbrev(arg1, "object"))
	    o_m = 0;
	else if (is_abbrev(arg1, "mobile"))
	    o_m = 1;
	else if (is_abbrev(arg1, "zone"))
	    zn = 1;
	else {
	    send_to_char(APPR_USE, ch);
	    return;
	}
    } else {
	send_to_char(APPR_USE, ch);
	return;
    }
  
    if (*arg2) 
	rnum = atoi(arg2);
    else {
	send_to_char(APPR_USE, ch);
	return;
    }

    if (zn) {       /********* approve zone *********/
	if (!strncmp(arg2, ".", 1))
	    zone = ch->in_room->zone;
	else if (!(zone = real_zone(rnum))) {
	    send_to_char("No such zone.\r\n", ch);
	    return;
	}

	send_to_char("Zone approved for full inclusion in the game.\r\n"
		     "Zone modification from this point must be approved by an olc god.\r\n", ch);
	sprintf(buf, "%s approved zone [%d] %s.", GET_NAME(ch),
		zone->number, zone->name);
	slog(buf);

	REMOVE_BIT(zone->flags, ZONE_MOBS_APPROVED |
		   ZONE_OBJS_APPROVED |
		   ZONE_ROOMS_APPROVED |
		   ZONE_ZCMDS_APPROVED |
		   ZONE_SEARCH_APPROVED |
		   ZONE_SHOPS_APPROVED);
	save_zone(ch, zone);
	return;
    }
    
    if (!o_m) {     /********* approve objects ******/
	obj = real_object_proto(rnum);
    
	if (!obj) {
	    send_to_char("There exists no object with that number, slick.\r\n", ch);
	    return;
	}

	for (zone = zone_table; zone; zone = zone->next) {
	    if (obj->shared->vnum >= (zone->number * 100) &&
		obj->shared->vnum <= zone->top)
		break;
	}

	if (!zone) {
	    send_to_char("ERROR: That object does not belong to any zone.\r\n", ch);
	    return;
	}

	if (CAN_EDIT_ZONE(ch, zone) && !OLCIMP(ch) && !OLCGOD( ch )) {
	    send_to_char("You can't approve your own objects, silly.\r\n", ch);
	    return;
	}
    
	if (!IS_SET(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
	    send_to_char("That item is already approved.\r\n", ch);
	    return;
	}

	REMOVE_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
	send_to_char("Object approved for full inclusion in the game.\r\n", ch);
	sprintf(buf, "%s approved object [%d] %s.", GET_NAME(ch),
		obj->shared->vnum, obj->short_description);
	slog(buf);

	GET_OLC_OBJ(ch) = obj;
	save_objs(ch);
	GET_OLC_OBJ(ch) = NULL;

    } else {     /** approve mobs */
	mob = real_mobile_proto(rnum);
    
	if (!mob) {
	    send_to_char("There exists no mobile with that number, slick.\r\n", ch);
	    return;
	}

	for (zone = zone_table; zone; zone = zone->next) {
	    if (GET_MOB_VNUM(mob) >= (zone->number * 100) &&
		GET_MOB_VNUM(mob) <= zone->top)
		break;
	}

	if (!zone) {
	    send_to_char("ERROR: That object does not belong to any zone.\r\n", ch);
	    return;
	}

	if (CAN_EDIT_ZONE(ch, zone) && !OLCIMP(ch) && !OLCGOD( ch ) ) {
	    send_to_char("You can't approve your own mobiles, silly.\r\n", ch);
	    return;
	}
    
	if (!IS_SET(MOB2_FLAGS(mob), MOB2_UNAPPROVED)) {
	    send_to_char("That mobile is already approved.\r\n", ch);
	    return;
	}

	REMOVE_BIT(MOB2_FLAGS(mob), MOB2_UNAPPROVED);
	send_to_char("Mobile approved for full inclusion in the game.\r\n", ch);
	sprintf(buf, "%s approved mobile [%d] %s.", GET_NAME(ch),
		rnum, GET_NAME(mob));
	slog(buf);

	GET_OLC_MOB(ch) = mob;
	save_mobs(ch);
	GET_OLC_MOB(ch) = NULL;
    }
}

int
CAN_EDIT_ZONE(CHAR *ch, struct zone_data *zone)
{

    char *name = NULL;
    CHAR *vict = NULL;
    struct char_file_u tmp_store;

    if ( OLCIMP(ch) )
	return 1;

    if (zone->owner_idnum == GET_IDNUM(ch))
	return 1;

    if (zone->co_owner_idnum == GET_IDNUM(ch))
	return 1;
    
    if (!(name = get_name_by_id(zone->owner_idnum))) {
	return 0;
    }
        
    if ( !OLCGOD(ch) ) {
	return 0;
    }

    // load the owner from file and compare levels
    CREATE(vict, CHAR, 1);
    clear_char(vict);
    if (load_char(name, &tmp_store) > -1) {
	store_to_char(&tmp_store, vict);
	if (GET_LEVEL(vict) >= GET_LEVEL(ch)) {
	    free_char(vict);
	    return 0;
	}
	else {
	    free_char(vict);
	    return 1;
	}
	
    }
    else {
	return 1;
    }
    
}
int OLC_EDIT_OK( CHAR *ch, struct zone_data *zone, int bits )
{

    if ( OLCIMP( ch ) )
	return 1;
    
    if ( ZONE_FLAGGED( zone, ZONE_FULLCONTROL ) )
	return 1;

    if ( ZONE_FLAGGED( zone, bits ) )
	return 1;

    return 0;
}
    
