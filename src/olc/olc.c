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

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "bomb.h"
#include "fight.h"
#include "obj_data.h"
#include "actions.h"
#include "guns.h"
#include "language.h"
#include "search.h"
#include "prog.h"
#include "paths.h"
#include "olc.h"
#include "editor.h"
#include "smokes.h"
#include "strutil.h"

extern const char *language_names[];
extern const char *race_language[][2];

#define NUM_POSITIONS 11

#define OLC_USAGE "Usage:\r\n"                                   \
"olc rsave\r\n"                                \
"olc rmimic <room number> [all, sound, desc, exdesc, flags, sector, title]\r\n"                 \
"olc rset <parameter> <arguments>\r\n"           \
"olc exit <direction> <parameter> <value> ['one-way']\r\n" \
"olc rexdesc <create | remove | edit | addkey> <keywords> [new keywords\r\n"  \
"olc clear <room | obj | mob>\r\n"           \
"olc create/destroy <room|zone|obj|mob|search> <number>\r\n" \
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
"olc show\r\n" \
"olc recalculate { obj | mob } <vnum>\r\n"

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
"olc zset [zone] pk_style <no_pk | neutral_pk | chaotic_pk>\r\n"        \
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
"olc show nodesc                     -- displays rooms with no description\n"  \
"olc show nosound                    -- displays rooms with no sound\n"  \
"olc show noexitdesc ['all'|'toroom'] -- displays exits with no desc\n" \
"  (default !toroom only)\n"

extern struct room_data *world;
extern struct creature *mob_proto;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct obj_data *object_list;
extern int olc_lock;
extern char *olc_guide;
extern const char *flow_types[];
extern const char *char_flow_msg[NUM_FLOW_TYPES + 1][3];
extern const char *obj_flow_msg[NUM_FLOW_TYPES + 1][2];

long asciiflag_conv(char *buf);

void num2str(char *str, int num);
void do_stat_object(struct creature *ch, struct obj_data *obj);

ACMD(do_zonepurge);
ACMD(do_zreset);

int prototype_obj_value(struct obj_data *obj);
bool save_objs(struct creature *ch, struct zone_data *zone);
bool save_wld(struct creature *ch, struct zone_data *zone);
bool save_mobs(struct creature *ch, struct zone_data *zone);
bool save_zone(struct creature *ch, struct zone_data *zone);

struct room_data *do_create_room(struct creature *ch, int vnum);
struct obj_data *do_create_obj(struct creature *ch, int vnum);
struct creature *do_create_mob(struct creature *ch, int vnum);

int do_destroy_room(struct creature *ch, int vnum);
int do_destroy_object(struct creature *ch, int vnum);
int do_destroy_mobile(struct creature *ch, int vnum);
int do_create_zone(struct creature *ch, int num);
int olc_mimic_mob(struct creature *ch, struct creature *orig,
    struct creature *targ, int mode);
void olc_mimic_room(struct creature *ch, struct room_data *targ, char *arg);
void do_olc_rexdesc(struct creature *ch, char *a, bool is_hedit);
void perform_oset(struct creature *ch, struct obj_data *obj_p,
    char *argument, int8_t subcmd);
void do_zset_command(struct creature *ch, char *argument);
void do_zcmd(struct creature *ch, char *argument);
void do_zone_cmdlist(struct creature *ch, struct zone_data *zone);
void do_zmob_cmd(struct creature *ch, char *argument);
void do_zobj_cmd(struct creature *ch, char *argument);
void do_zdoor_cmd(struct creature *ch, char *argument);
void do_zgive_cmd(struct creature *ch, char *argument);
void do_zput_cmd(struct creature *ch, char *argument);
void do_zequip_cmd(struct creature *ch, char *argument);
void do_zimplant_cmd(struct creature *ch, char *argument);
void do_zpath_cmd(struct creature *ch, char *argument);
void do_mob_medit(struct creature *ch, char *argument);
void do_mob_mstat(struct creature *ch, char *argument);
void do_mob_mset(struct creature *ch, char *argument);
void do_olc_xset(struct creature *ch, char *argument);
void do_olc_rset(struct creature *ch, char *argument);
void do_olc_xstat(struct creature *ch);
struct special_search_data *do_create_search(struct creature *ch, char *arg);
int do_destroy_search(struct creature *ch, char *arg);
int set_char_xedit(struct creature *ch, char *argument);
void do_clear_room(struct creature *ch);
void do_clear_olc_object(struct creature *ch);
int do_clear_olc_mob(struct creature *ch);

char *find_exdesc(char *word, struct extra_descr_data *list, bool find_exact);

const char *olc_commands[] = {
    "rsave",                    /* save wld file */
    "rmimic",
    "rset",                     /* 2 -- set utility */
    "exit",
    "owner",                    /* 4 -- set owner  */
    "rexdesc",
    "unlock",                   /* 6 -- zone/global unlock */
    "lock",
    "guide",
    "rsearch",
    "help",                     /* 10 -- help function */
    "osave",
    "oedit",
    "ostat",
    "oset",
    "oexdesc",                  /* 15 -- object exdescs  */
    "oload",
    "omimic",
    "clear",
    "supersave",
    "create",                   /* 20 -- make new rooms/objs/zones etc. */
    "destroy",
    "zset",
    "zcmd",
    "zsave",
    "zmob",                     /* 25 -- load mob */
    "zobj",
    "zdoor",
    "zreset",
    "zpurge",
    "zequip",                   /* 30 zequip  */
    "zput",
    "zgive",
    "medit",                    /* 33 -- do_mob_medit() */
    "mstat",
    "mset",
    "msave",
    "sedit",
    "sstat",
    "sset",
    "ssave",                    /* 40 */
    "zimplant",
    "mmimic",
    "zpath",
    "xedit",
    "xset",                     /* 45 */
    "xstat",
    "mload",
    "show",
    "recalculate",
    "progmimic",                /* 50 */
    "\n"                        /* many more to be added */
};

struct extra_descr_data *
locate_exdesc(char *word, struct extra_descr_data *list, bool exact)
{

    struct extra_descr_data *i;

    if (!exact) {
        for (i = list; i; i = i->next)
            if (isname(word, i->keyword))
                return (i);
    } else {
        for (i = list; i; i = i->next)
            if (isname_exact(word, i->keyword))
                return (i);
    }

    return NULL;
}

#define obj_p     GET_OLC_OBJ(ch)
#define mob_p     GET_OLC_MOB(ch)

/* The actual do_olc command for the interpreter.  Determines the target
   entity, checks permissions, and passes contrl to olc_interpreter */
ACMD(do_olc)
{
    char mode_arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int olc_command;
    room_num vnum = -1;
    struct room_data *rnum, *room;
    int edir;
    int i = 0, j, k;
    int8_t one_way = false;
    struct extra_descr_data *desc, *ndesc, *temp;
    struct obj_data *tmp_obj = NULL, *obj = NULL;
    struct descriptor_data *d = NULL;
    struct zone_data *zone = NULL;
    struct creature *mob = NULL;
    struct creature *tmp_mob = NULL;
    struct special_search_data *tmp_search;

    if (olc_lock || (IS_SET(ch->in_room->zone->flags, ZONE_LOCKED))) {
        if (GET_LEVEL(ch) >= LVL_IMPL) {
            send_to_char(ch,
                "\007\007\007\007%sWARNING.%s  Overriding olc %s lock.\r\n",
                CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM),
                olc_lock ? "global" : "discrete zone");
        } else {
            send_to_char(ch, "OLC is currently locked.  Try again later.\r\n");
            return;
        }
    }

    /* first, figure out the first (mode) argument */
    half_chop(argument, mode_arg, argument);
    skip_spaces(&argument);

    if (!*mode_arg) {
        page_string(ch->desc, OLC_USAGE);
        return;
    }
    if ((olc_command = search_block(mode_arg, olc_commands, false)) < 0) {
        send_to_char(ch, "Invalid command '%s'.\r\n", mode_arg);
        return;
    }
    if (ch->in_room == NULL) {
        send_to_char(ch, "What the hell? Where are you!?!?\r\n");
        return;
    }

    if (olc_command != 8 && olc_command != 10) {    /* help? */
        if (!is_authorized(ch, EDIT_ZONE, ch->in_room->zone)) {
            if (is_authorized(ch, WORLDWRITE, NULL)
                && !PRF2_FLAGGED(ch, PRF2_WORLDWRITE)) {
                send_to_char(ch, "You seem to have forgotten something.\r\n");
            } else {
                send_to_char(ch, "Piss off Beanhead.  Permission DENIED.\r\n");
                mudlog(GET_INVIS_LVL(ch), NRM, true,
                    "Failed attempt for %s to edit zone %d.",
                    GET_NAME(ch), ch->in_room->zone->number);
            }
            return;
        }
    }

    if ((olc_command <= 3 || olc_command == 5) &&
        !OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ROOMS_APPROVED)) {
        send_to_char(ch, "World OLC is not approved for this zone.\r\n");
        return;
    }

    switch (olc_command) {
    case 0:                    /* rsave */
        zone = ch->in_room->zone;
        if (save_wld(ch, zone))
            send_to_char(ch, "You save the rooms for zone #%d (%s).\r\n",
                zone->number, zone->name);
        else
            send_to_char(ch,
                "The rooms for zone #%d (%s) could not be saved.\r\n",
                zone->number, zone->name);
        break;
    case 1:                    /* rmimic */
        if (!*argument) {
            send_to_char(ch,
                "Usage: olc rmimic <room num> [all] [sounds] [flags] [desc] [exdesc]\r\n");
            break;
        }
        argument = one_argument(argument, arg1);
        vnum = atoi(arg1);
        if (!(rnum = real_room(vnum))) {
            send_to_char(ch, "That's not a valid room, genius.\r\n");
            break;
        }
        if (rnum == ch->in_room) {
            send_to_char(ch, "Real funny.\r\n");
            break;
        }
        olc_mimic_room(ch, rnum, argument);
        break;
    case 2:                    /* rset */
        do_olc_rset(ch, argument);
        break;

    case 3:                    /* exit */
        if (!*argument) {
            send_to_char(ch,
                "Usage: olc exit <direction> <parameter> <value>.\r\n");
            return;
        }
        half_chop(argument, buf, argument);
        skip_spaces(&argument);

        if ((!*buf) || ((edir = search_block(buf, dirs, false)) < 0)) {
            send_to_char(ch, "What exit?\r\n");
            return;
        }

        half_chop(argument, buf, argument);
        skip_spaces(&argument);
        if (!*buf) {
            send_to_char(ch,
                "Options are: description, doorflags, toroom, keynumber, keywords, remove.\r\n");
            return;
        }
        if (!EXIT(ch, edir)) {
            CREATE(ch->in_room->dir_option[edir], struct room_direction_data,
                1);
            EXIT(ch, edir)->to_room = NULL;
        } else if (is_abbrev(buf, "remove")) {
            if (EXIT(ch, edir)->general_description)
                free(EXIT(ch, edir)->general_description);
            if (EXIT(ch, edir)->keyword)
                free(EXIT(ch, edir)->keyword);
            free(EXIT(ch, edir));
            ch->in_room->dir_option[edir] = NULL;
            send_to_char(ch, "Exit removed.\r\n");
            return;
        }

        if (is_abbrev(buf, "description")) {
            skip_spaces(&argument);

            if (!*argument && is_abbrev(argument, "remove")) {
                if (EXIT(ch, edir)->general_description) {
                    free(EXIT(ch, edir)->general_description);
                    EXIT(ch, edir)->general_description = NULL;
                }
                send_to_char(ch, "Exit desc removed.\r\n");
                return;
            }

            if (EXIT(ch, edir)->general_description == NULL) {
                act("$n begins to create an exit description.", true, ch, NULL, NULL,
                    TO_ROOM);
            } else {
                act("$n begins to edit an exit description.", true, ch, NULL, NULL,
                    TO_ROOM);
            }
            start_editing_text(ch->desc, &EXIT(ch, edir)->general_description,
                4096);
            SET_BIT(PLR_FLAGS(ch), PLR_OLC);
            return;
        } else if (is_abbrev(buf, "keywords")) {
            if (EXIT(ch, edir)->keyword) {
                free(EXIT(ch, edir)->keyword);
                EXIT(ch, edir)->keyword = NULL;
            }
            if (argument && *argument) {
                EXIT(ch, edir)->keyword = strdup(argument);
                send_to_char(ch, "Keywords set.\r\n");
            } else
                send_to_char(ch, "What keywords?!\r\n");

            return;
        } else if (is_abbrev(buf, "doorflags")) {
            if (!argument || !*argument) {
                EXIT(ch, edir)->exit_info = 0;
                show_olc_help(ch, buf);
            } else {
                EXIT(ch, edir)->exit_info = asciiflag_conv(argument);
                send_to_char(ch, "Doorflags set.\r\n");
            }

            return;
        } else if (is_abbrev(buf, "keynumber")) {
            if (!argument || !*argument || !is_number(argument))
                EXIT(ch, edir)->key = 0;
            else
                EXIT(ch, edir)->key = atoi(argument);

            send_to_char(ch, "Keynumber set.\r\n");

            return;
        } else if (is_abbrev(buf, "toroom")) {
            half_chop(argument, arg1, arg2);
            if (!*arg1 || !is_number(arg1)
                || (!(room = real_room(atoi(arg1))))) {
                if (!(room = do_create_room(ch, atoi(arg1)))) {
                    EXIT(ch, edir)->to_room = NULL;
                    send_to_char(ch,
                        "That destination room does not exist.\r\n");
                    return;
                } else
                    send_to_char(ch,
                        "The destination room has been created.\r\n");
            }

            if (*arg2 && is_abbrev(arg2, "one-way"))
                one_way = true;
            EXIT(ch, edir)->to_room = room;
            if (!one_way && !ABS_EXIT(room, rev_dir[edir])) {
                if (!is_authorized(ch, EDIT_ZONE, room->zone)) {
                    send_to_char(ch,
                        "To room set.  Unable to create return exit.\r\n");
                } else {
                    CREATE(room->dir_option[rev_dir[edir]],
                        struct room_direction_data, 1);
                    ABS_EXIT(room, rev_dir[edir])->to_room = ch->in_room;
                    send_to_char(ch,
                        "To room set.  Return exit created from to room.\r\n");
                }
            } else if (!one_way &&
                ABS_EXIT(room, rev_dir[edir])->to_room == NULL) {
                if (!is_authorized(ch, EDIT_ZONE, room->zone)) {
                    send_to_char(ch,
                        "To room set.  Unable to set return exit.\r\n");
                } else {
                    ABS_EXIT(room, rev_dir[edir])->to_room = ch->in_room;
                    send_to_char(ch,
                        "To room set.  Return exit set in to room.\r\n");
                }
            } else
                send_to_char(ch, "To room set.\r\n");

            return;
        } else {
            send_to_char(ch,
                "Options are: description, doorflags, toroom, keynumber.\r\n");
            return;
        }
        break;
    case 4:                    /* Owner */
        send_to_char(ch, "Use olc zset owner, instead.\r\n");
        break;
    case 5:                    /* rexdesc */
        do_olc_rexdesc(ch, argument, false);
        break;
    case 6:                    /*  unlock */
        if (!*argument || is_abbrev(argument, "all")) {
            if (!olc_lock)
                send_to_char(ch, "Olc is currently globally unlocked.\r\n");
            else if (GET_LEVEL(ch) >= olc_lock) {
                send_to_char(ch, "Unlocking olc (global).\r\n");
                olc_lock = 0;
                mudlog(MAX(LVL_IMMORT, GET_INVIS_LVL(ch)), NRM, true,
                    "OLC: %s has unlocked global access.", GET_NAME(ch));
            } else
                send_to_char(ch, "Olc unlock:  Permission denied.\r\n");
        } else if (!is_number(argument))
            send_to_char(ch, "The argument must be a number, or 'all'.\r\n");
        else {
            one_argument(argument, arg1);
            for (j = atoi(arg1), zone = zone_table; zone; zone = zone->next) {
                if (zone->number == j) {
                    if (is_authorized(ch, EDIT_ZONE, zone)) {
                        if (!IS_SET(zone->flags, ZONE_LOCKED))
                            send_to_char(ch,
                                "That zone is currently unlocked.\r\n");
                        else {
                            REMOVE_BIT(zone->flags, ZONE_LOCKED);
                            send_to_char(ch,
                                "Zone unlocked for online creation.\r\n");
                            mudlog(GET_INVIS_LVL(ch), NRM, true,
                                "OLC: %s has unlocked zone %d (%s).",
                                GET_NAME(ch), zone->number, zone->name);
                        }
                        return;
                    } else {
                        send_to_char(ch,
                            "Olc zone unlock: Permission denied.\r\n");
                        return;
                    }
                }
            }
            send_to_char(ch, "That is an invalid zone.\r\n");
        }
        break;
    case 7:                    /* lock */
        if (!*argument || is_abbrev(argument, "all")) {
            if (olc_lock)
                send_to_char(ch, "Olc is currently locked.\r\n");
            else if (is_authorized(ch, OLC_LOCK, NULL)) {
                send_to_char(ch, "Locking olc.\r\n");
                olc_lock = GET_LEVEL(ch);
                mudlog(GET_INVIS_LVL(ch), BRF, true,
                    "OLC: %s has locked global access to olc.", GET_NAME(ch));
            } else
                send_to_char(ch, "Olc lock:  Permission denied.\r\n");
        } else if (!is_number(argument))
            send_to_char(ch, "The argument must be a number, or 'all'.\r\n");
        else {
            one_argument(argument, arg1);
            for (j = atoi(arg1), zone = zone_table; zone; zone = zone->next) {
                if (zone->number == j) {
                    if (is_authorized(ch, EDIT_ZONE, zone)) {
                        if (IS_SET(zone->flags, ZONE_LOCKED))
                            send_to_char(ch,
                                "That zone is already locked.\r\n");
                        else {
                            SET_BIT(zone->flags, ZONE_LOCKED);
                            send_to_char(ch,
                                "Zone locked to online creation.\r\n");
                            mudlog(GET_INVIS_LVL(ch), BRF, true,
                                "OLC: %s has locked zone %d (%s).",
                                GET_NAME(ch), zone->number, zone->name);
                        }
                        return;
                    } else {
                        send_to_char(ch,
                            "Olc zone unlock: Permission denied.\r\n");
                        return;
                    }
                }
            }
            send_to_char(ch, "That is an invalid zone.\r\n");
        }
        break;
    case 8:/************* guide ***************/
        if (!olc_guide)
            send_to_char(ch,
                "Sorry, the olc guide is not loaded into memory.\r\n");
        else
            page_string(ch->desc, olc_guide);
        break;
    case 9:       /*************** rsearch ******************/
        break;
    case 10:          /*********** OLC HELP  *********************/
        show_olc_help(ch, argument);
        break;
    case 11:          /*************** osave *********************/
        zone = ch->in_room->zone;
        if (GET_OLC_OBJ(ch)) {
            int o_vnum = GET_OLC_OBJ(ch)->shared->vnum;
            for (zone = zone_table; zone; zone = zone->next)
                if (o_vnum >= zone->number * 100 && o_vnum <= zone->top)
                    break;
            if (!zone) {
                send_to_char(ch, "Couldn't find a zone for object %d", o_vnum);
                return;
            }
        }

        if (save_objs(ch, zone))
            send_to_char(ch, "You save the objects in zone #%d (%s).\r\n",
                zone->number, zone->name);
        else
            send_to_char(ch,
                "The objects for zone #%d (%s) could not be saved.\r\n",
                zone->number, zone->name);
        break;
    case 12:          /*************** oedit *********************/
        if (!*argument) {
            if (!obj_p)
                send_to_char(ch,
                    "You are not currently editing an object.\r\n");
            else {
                send_to_char(ch, "Current olc object: [%5d] %s\r\n",
                    obj_p->shared->vnum, obj_p->name);
            }
            return;
        }
        if (!is_number(argument)) {
            if (is_abbrev(argument, "exit")) {
                send_to_char(ch, "Exiting object editor.\r\n");
                GET_OLC_OBJ(ch) = NULL;
                return;
            }
            send_to_char(ch, "The argument must be a number.\r\n");
            return;
        } else {
            j = atoi(argument);
            if ((tmp_obj = real_object_proto(j)) == NULL)
                send_to_char(ch, "There is no such object.\r\n");
            else {

                for (zone = zone_table; zone; zone = zone->next)
                    if (j <= zone->top)
                        break;

                if (!zone) {
                    send_to_char(ch,
                        "That object does not belong to any zone!!\r\n");
                    errlog("object not in any zone.");
                    return;
                }

                if (!is_authorized(ch, EDIT_ZONE, zone)) {
                    send_to_char(ch,
                        "You do not have permission to edit those objects.\r\n");
                    return;
                }

                if (!OLC_EDIT_OK(ch, zone, ZONE_OBJS_APPROVED)) {
                    send_to_char(ch,
                        "Object OLC is not approved for this zone.\r\n");
                    return;
                }

                for (d = descriptor_list; d; d = d->next) {
                    if (d->creature && GET_OLC_OBJ(d->creature) == tmp_obj) {
                        act("$N is already editing that object.", false, ch, NULL,
                            d->creature, TO_CHAR);
                        return;
                    }
                }

                GET_OLC_OBJ(ch) = tmp_obj;
                send_to_char(ch, "Now editing object [%d] %s%s%s\r\n",
                    tmp_obj->shared->vnum,
                    CCGRN(ch, C_NRM), tmp_obj->name, CCNRM(ch, C_NRM));
            }
        }

        break;
    case 13:          /*************** ostat *********************/
        if (!*argument) {
            if (!obj_p)
                send_to_char(ch,
                    "You are not currently editing an object.\r\n");
            else {
                do_stat_object(ch, obj_p);
            }
        } else if (!is_number(argument)) {
            send_to_char(ch, "The argument must be a number.\r\n");
            return;
        } else {
            j = atoi(argument);
            if (!(tmp_obj = read_object(j)))
                send_to_char(ch, "There is no such object.\r\n");
            else {
                do_stat_object(ch, tmp_obj);
                extract_obj(tmp_obj);
            }
        }
        break;

    case 14:       /*****************  oset *********************/
        if (!obj_p) {
            send_to_char(ch, "You are not currently editing an object.\r\n");
            return;
        }

        perform_oset(ch, obj_p, argument, OLC_OSET);
        break;

    case 15:                   /* oexdesc */
        if (!*argument) {
            send_to_char(ch, OLC_EXDESC_USAGE);
            return;
        }
        half_chop(argument, buf, argument);
        if (!obj_p)
            send_to_char(ch,
                "Hey punk, you need an object in your editor first!!!\r\n");
        else if (!*argument)
            send_to_char(ch,
                "Which extra description would you like to deal with?\r\n");
        else if (!*buf)
            send_to_char(ch,
                "Valid commands are: create, remove, edit, addkey.\r\n");
        else if (is_abbrev(buf, "remove")) {
            if ((desc = locate_exdesc(argument, obj_p->ex_description, 1))) {
                REMOVE_FROM_LIST(desc, obj_p->ex_description, next);
                if (desc->keyword)
                    free(desc->keyword);
                else
                    errlog("WTF?? !desc->keyword??");

                if (desc->description)
                    free(desc->description);
                else
                    errlog("WTF?? !desc->description??");

                free(desc);
                send_to_char(ch, "Description removed.\r\n");
                UPDATE_OBJLIST(obj_p, tmp_obj,->ex_description);
            } else
                send_to_char(ch, "No such extra description.\r\n");

            return;
        } else if (is_abbrev(buf, "create")) {
            if (find_exdesc(argument, obj_p->ex_description, false)) {
                send_to_char(ch,
                    "An extra description already exists with that keyword.\r\n"
                    "Use the 'olc rexdesc remove' command to remove it, or the\r\n"
                    "'olc rexdesc edit' command to change it, punk.\r\n");
                return;
            }
            CREATE(ndesc, struct extra_descr_data, 1);
            ndesc->keyword = strdup(argument);
            ndesc->next = obj_p->ex_description;
            obj_p->ex_description = ndesc;

            start_editing_text(ch->desc, &obj_p->ex_description->description,
                4096);
            SET_BIT(PLR_FLAGS(ch), PLR_OLC);

            act("$n begins to write an object description.",
                true, ch, NULL, NULL, TO_ROOM);
            UPDATE_OBJLIST(obj_p, tmp_obj,->ex_description);
            for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
                if (GET_OBJ_VNUM(tmp_obj) == GET_OBJ_VNUM(obj_p))
                    tmp_obj->ex_description = NULL;

            return;
        } else if (is_abbrev(buf, "edit")) {
            if ((desc = locate_exdesc(argument, obj_p->ex_description, 1))) {
                start_editing_text(ch->desc, &desc->description, 4096);
                SET_BIT(PLR_FLAGS(ch), PLR_OLC);
                UPDATE_OBJLIST(obj_p, tmp_obj,->ex_description);
                act("$n begins to write an object description.",
                    true, ch, NULL, NULL, TO_ROOM);
                for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
                    if (GET_OBJ_VNUM(tmp_obj) == GET_OBJ_VNUM(obj_p))
                        tmp_obj->ex_description = NULL;
            } else
                send_to_char(ch,
                    "No such description.  Use 'create' to make a new one.\r\n");

            return;
        } else if (is_abbrev(buf, "addkeyword")) {
            half_chop(argument, arg1, arg2);
            if ((desc = locate_exdesc(arg1, obj_p->ex_description, 1))) {
                if (!*arg2) {
                    send_to_char(ch,
                        "What??  How about giving me some keywords to add...\r\n");
                    return;
                } else {
                    strcpy(buf, desc->keyword);
                    strcat(buf, " ");
                    strcat(buf, arg2);
                    free(desc->keyword);
                    desc->keyword = strdup(buf);
                    UPDATE_OBJLIST(obj_p, tmp_obj,->ex_description);
                    send_to_char(ch, "Keywords added.\r\n");
                    return;
                }
            } else
                send_to_char(ch,
                    "There is no such description on this object.\r\n");
        } else
            send_to_char(ch, OLC_EXDESC_USAGE);

        break;

    case 16:     /********** oload **************/
        if (!*argument) {
            if (!obj_p) {
                send_to_char(ch, "Which object?\r\n");
                return;
            } else {
                j = obj_p->shared->vnum;
                tmp_obj = obj_p;
            }
        } else {
            skip_spaces(&argument);
            if (!is_number(argument)) {
                send_to_char(ch, "The argument must be a vnum.\r\n");
                return;
            }
            j = atoi(argument);
            if (!(tmp_obj = real_object_proto(j))) {
                send_to_char(ch, "No such object exists.\r\n");
                return;
            }
            if (j < (GET_ZONE(ch->in_room) * 100)
                || j > ch->in_room->zone->top) {
                send_to_char(ch,
                    "You cannot olc oload objects from other zones.\r\n");
                return;
            }
        }
        if (!is_authorized(ch, WORLDWRITE, NULL)
            && !IS_SET(tmp_obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
            send_to_char(ch, "You cannot olc oload approved items.\r\n");
            return;
        }
        if (!(tmp_obj = read_object(j)))
            send_to_char(ch, "Unable to load object.\r\n");
        else {
            obj_to_char(tmp_obj, ch);
            GET_OBJ_TIMER(tmp_obj) = GET_LEVEL(ch);
            act("$p appears in your hands.", false, ch, tmp_obj, NULL, TO_CHAR);
            act("$n creates $p in $s hands.", true, ch, tmp_obj, NULL, TO_ROOM);
            slog("OLC: %s oloaded [%d] %s.", GET_NAME(ch),
                GET_OBJ_VNUM(tmp_obj), tmp_obj->name);
        }
        break;
    case 17:    /********** omimic ************/
        if (!*argument) {
            send_to_char(ch, "Usage: olc omimic <obj number>\r\n");
            break;
        }
        vnum = atoi(argument);
        tmp_obj = real_object_proto(vnum);
        if (!tmp_obj) {
            send_to_char(ch, "That's not a valid object, genius.\r\n");
            break;
        }
        if (!obj_p) {
            send_to_char(ch,
                "You need to have an object in your editing buffer first.\r\n");
            return;
        }
        if (tmp_obj == obj_p) {
            send_to_char(ch, "Real funny.\r\n");
            break;
        }
        if (obj_p->aliases)
            free(obj_p->aliases);
        if (tmp_obj->aliases)
            obj_p->aliases = strdup(tmp_obj->aliases);
        else
            obj_p->aliases = NULL;

        if (obj_p->name)
            free(obj_p->name);
        if (tmp_obj->name)
            obj_p->name = strdup(tmp_obj->name);
        else
            obj_p->name = NULL;

        if (obj_p->line_desc)
            free(obj_p->line_desc);
        if (tmp_obj->line_desc)
            obj_p->line_desc = strdup(tmp_obj->line_desc);
        else
            obj_p->line_desc = NULL;

        if (obj_p->action_desc)
            free(obj_p->action_desc);
        if (tmp_obj->action_desc)
            obj_p->action_desc = strdup(tmp_obj->action_desc);
        else
            obj_p->action_desc = NULL;
        obj_p->obj_flags.type_flag = tmp_obj->obj_flags.type_flag;
        obj_p->obj_flags.extra_flags = tmp_obj->obj_flags.extra_flags;
        obj_p->obj_flags.extra2_flags = tmp_obj->obj_flags.extra2_flags;
        obj_p->obj_flags.extra3_flags = tmp_obj->obj_flags.extra3_flags;
        obj_p->obj_flags.wear_flags = tmp_obj->obj_flags.wear_flags;
        obj_p->obj_flags.value[0] = tmp_obj->obj_flags.value[0];
        obj_p->obj_flags.value[1] = tmp_obj->obj_flags.value[1];
        obj_p->obj_flags.value[2] = tmp_obj->obj_flags.value[2];
        obj_p->obj_flags.value[3] = tmp_obj->obj_flags.value[3];
        obj_p->obj_flags.material = tmp_obj->obj_flags.material;
        obj_p->obj_flags.max_dam = tmp_obj->obj_flags.max_dam;
        obj_p->obj_flags.damage = tmp_obj->obj_flags.damage;
        obj_p->obj_flags.weight = tmp_obj->obj_flags.weight;
        obj_p->shared->cost = tmp_obj->shared->cost;
        obj_p->shared->cost_per_day = tmp_obj->shared->cost_per_day;
        obj_p->shared->func = NULL;
        obj_p->shared->func_param = NULL;

        if (!is_authorized(ch, WORLDWRITE, NULL)) {
            SET_BIT(obj_p->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
        }

        for (k = 0; k < 3; k++)
            obj_p->obj_flags.bitvector[k] = tmp_obj->obj_flags.bitvector[k];

        for (k = 0; k < MAX_OBJ_AFFECT; k++) {
            obj_p->affected[k].location = tmp_obj->affected[k].location;
            obj_p->affected[k].modifier = tmp_obj->affected[k].modifier;
        }

        desc = obj_p->ex_description;
        obj_p->ex_description = NULL;
        while (desc) {
            if (desc->keyword)
                free(desc->keyword);
            if (desc->description)
                free(desc->description);
            desc = desc->next;
        }
        desc = tmp_obj->ex_description;
        while (desc) {
            CREATE(ndesc, struct extra_descr_data, 1);
            ndesc->keyword = strdup(desc->keyword);
            if (ndesc->description)
                ndesc->description = strdup(desc->description);
            ndesc->next = obj_p->ex_description;
            obj_p->ex_description = ndesc;
            desc = desc->next;
        }
        UPDATE_OBJLIST_FULL(obj_p, obj);

        send_to_char(ch, "Okay, done mimicing.\r\n");
        break;

    case 18:    /************** clear ****************/
        if (!*argument) {
            send_to_char(ch, "Usage: olc clear <room | obj | mob>\r\n");
            return;
        }
        if (is_abbrev(argument, "room")) {
            do_clear_room(ch);
        } else if (is_abbrev(argument, "object")) {
            if (!obj_p) {
                send_to_char(ch,
                    "You are not currently editing an object.\r\n");
            } else
                do_clear_olc_object(ch);
        } else if (is_abbrev(argument, "mobile")) {
            if (GET_OLC_MOB(ch) == NULL) {
                send_to_char(ch,
                    "You are not currently editing a mobile.\r\n");
            } else {
                do_clear_olc_mob(ch);
            }
        } else {
            send_to_char(ch, "Olc clear what?!?!!\r\n");
        }
        break;

    case 19:/*** supersave ***/
        if (GET_LEVEL(ch) < LVL_LUCIFER)
            return;
        if (!*argument)
            send_to_char(ch, "Usage: olc supersave <world|objects|zones>\r\n");
        else if (is_abbrev(argument, "world")) {
            for (i = 1, zone = zone_table; zone; zone = zone->next, i++)
                save_wld(ch, zone);
            slog("SUPERSAVE: %d wld files saved.\r\n", i);
            send_to_char(ch, "Done. %d wld file%s saved.\r\n", i,
                (i == 1) ? "" : "s");
            return;
        } else if (is_abbrev(argument, "zones")) {
            for (i = 1, zone = zone_table; zone; zone = zone->next, i++)
                save_zone(ch, zone);
            slog("SUPERSAVE: %d zon files saved.\r\n", i);
            send_to_char(ch, "Done. %d zone%s saved.\r\n", i,
                (i == 1) ? "" : "s");
            return;

        } else if (is_abbrev(argument, "objects")) {
            for (i = 1, zone = zone_table; zone; zone = zone->next, i++)
                save_objs(ch, zone);

            slog("SUPERSAVE: %d obj files saved.\r\n", i);
            send_to_char(ch, "Done. %d obj file%s saved.\r\n", i,
                (i == 1) ? "" : "s");
            return;

        } else if (is_abbrev(argument, "mobiles")) {
            for (zone = zone_table, i = 1; zone; zone = zone->next, i++)
                save_mobs(ch, zone);
            slog("SUPERSAVE: %d mob files saved.\r\n", i);
            send_to_char(ch, "Done. %d mob file%s saved.\r\n", i,
                (i == 1) ? "" : "s");
            return;

        } else
            send_to_char(ch, "Not Implemented.\r\n");

        break;

    case 20: /*** create ***/
        if (!*argument)
            send_to_char(ch,
                "Usage: olc create <room|zone|obj|mob|help> <vnum|next>\r\n");
        else {
            int tmp_vnum = 0;
            argument = two_arguments(argument, arg1, arg2);
            if (is_abbrev(arg1, "room")) {
                if (!*arg2) {
                    send_to_char(ch, "Create a room with what vnum?\r\n");
                } else if (is_abbrev(arg2, "next")) {
                    for (i = ch->in_room->zone->number * 100;
                        i < ch->in_room->zone->top; i++) {
                        if (!real_room(i)) {
                            tmp_vnum = i;
                            break;
                        }
                    }
                } else {
                    i = atoi(arg2);
                    tmp_vnum = i;
                }
                if (tmp_vnum && do_create_room(ch, tmp_vnum)) {
                    send_to_char(ch, "Room %d successfully created.\r\n",
                        tmp_vnum);
                } else if (!tmp_vnum && *arg2) {
                    send_to_char(ch,
                        "No allocatable rooms found in zone.\r\n");
                }
            } else if (is_abbrev(arg1, "zone")) {
                if (!is_authorized(ch, CREATE_ZONE, NULL)) {
                    send_to_char(ch, "You cannot create zones.\r\n");
                    return;
                }
                if (!*arg2)
                    send_to_char(ch, "Create a zone with what number?\r\n");
                else {
                    i = atoi(arg2);
                    if (do_create_zone(ch, i))
                        send_to_char(ch, "Zone successfully created.\r\n");
                }
            } else if (is_abbrev(arg1, "object")) {
                if (!*arg2) {
                    send_to_char(ch, "Create an obj with what vnum?\r\n");
                } else if (is_abbrev(arg2, "next")) {
                    for (i = ch->in_room->zone->number * 100;
                        i < ch->in_room->zone->top; i++) {
                        if (!real_object_proto(i)) {
                            tmp_vnum = i;
                            break;
                        }
                    }
                } else {
                    tmp_vnum = atoi(arg2);
                }
                if (tmp_vnum && (tmp_obj = do_create_obj(ch, tmp_vnum))) {
                    GET_OLC_OBJ(ch) = tmp_obj;
                    send_to_char(ch,
                        "Object %d successfully created.\r\nNow editing object %d\r\n",
                        tmp_obj->shared->vnum, tmp_obj->shared->vnum);
                } else if (!tmp_vnum && *arg2) {
                    send_to_char(ch,
                        "No allocatable objects found in zone.\r\n");
                }
            } else if (is_abbrev(arg1, "mobile")) {
                if (!*arg2) {
                    send_to_char(ch, "Create a mob with what vnum?\r\n");
                } else if (is_abbrev(arg2, "next")) {
                    for (i = ch->in_room->zone->number * 100;
                        i < ch->in_room->zone->top; i++) {
                        if (!real_mobile_proto(i)) {
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
                    send_to_char(ch,
                        "Mobile %d successfully created.\r\nNow editing mobile %d\r\n",
                        tmp_vnum, tmp_vnum);
                } else if (!tmp_vnum && *arg2) {
                    send_to_char(ch,
                        "No allocatable mobiles found in zone.\r\n");
                }
            } else if (is_abbrev(arg1, "search")) {
                tmp_search = do_create_search(ch, tmp_strcat(arg2, " ", argument, NULL));
                if (tmp_search) {
                    GET_OLC_SRCH(ch) = tmp_search;
                    send_to_char(ch, "Search creation successful.\r\n");
                    send_to_char(ch, "Now editing search (%s)/(%s)\r\n",
                        tmp_search->command_keys, tmp_search->keywords);
                }
            } else if (is_abbrev(arg1, "path")
                && is_authorized(ch, WORLDWRITE, NULL)) {
                if (!add_path(strcat(arg2, argument), true))
                    send_to_char(ch, "Path added.\r\n");
            } else
                send_to_char(ch,
                    "Usage: olc create <room|zone|obj|mob> <vnum>\r\n");
        }
        break;

    case 21:
            /**** destroy ****/
        if (!*argument) {
            send_to_char(ch,
                "Usage: olc destroy <room|zone|obj|mob> <vnum>\r\n");
            return;
        }
        argument = two_arguments(argument, arg1, arg2);

        if (is_abbrev(arg1, "search")) {
            do_destroy_search(ch, tmp_strcat(arg2, " ", argument, NULL));
            return;
        }

        if (GET_LEVEL(ch) < LVL_IMPL) {
            send_to_char(ch,
                "You are not authorized to destroy, hosehead.\r\n");
            return;
        }

        if (is_abbrev(arg1, "room")) {
            if (!*arg2)
                i = ch->in_room->number;
            else
                i = atoi(arg2);

            if (!do_destroy_room(ch, i))
                send_to_char(ch, "Room eliminated.\r\n");

        } else if (is_abbrev(arg1, "object")) {
            if (!*arg2) {
                if (!GET_OLC_OBJ(ch)) {
                    send_to_char(ch, "Destroy what object prototype?\r\n");
                    return;
                }
                i = GET_OBJ_VNUM(GET_OLC_OBJ(ch));
            } else
                i = atoi(arg2);

            if (!do_destroy_object(ch, i))
                send_to_char(ch, "Object eliminated.\r\n");

        } else if (is_abbrev(arg1, "mobile")) {
            if (!*arg2) {
                if (!GET_OLC_MOB(ch)) {
                    send_to_char(ch, "Destroy what mobile prototype?\r\n");
                    return;
                }
                i = GET_NPC_VNUM(GET_OLC_MOB(ch));
            } else
                i = atoi(arg2);

            if (!do_destroy_mobile(ch, i))
                send_to_char(ch, "Mobile eliminated.\r\n");

        } else if (is_abbrev(arg1, "zone")) {
            send_to_char(ch, "Command not imped yet.\r\n");
        } else
            send_to_char(ch, "Unknown.\r\n");
        break;
    case 22:
        if (!*argument) {
            page_string(ch->desc, OLC_ZSET_USAGE);
            return;
        } else
            do_zset_command(ch, argument);
        break;
    case 23:                   /* zcmd */
        if (!*argument) {
            page_string(ch->desc, OLC_ZCMD_USAGE);
            return;
        } else
            do_zcmd(ch, argument);
        break;
    case 24:        /******** zsave ********/
        zone = ch->in_room->zone;
        if (*argument) {
            i = atoi(argument);
            for (zone = zone_table; zone; zone = zone->next)
                if (zone->number == i)
                    break;
            if (!zone) {
                send_to_char(ch, "That zone could not be found.\r\n");
                return;
            }
        }

        if (save_zone(ch, zone))
            send_to_char(ch,
                "You save the zone information of zone #%d (%s).\r\n",
                zone->number, zone->name);
        else
            send_to_char(ch,
                "The zone information of zone #%d (%s) could not be saved.\r\n",
                zone->number, zone->name);
        break;
    case 25:                   /* zmob */
        if (!*argument)
            send_to_char(ch, "Type olc for usage.\r\n");
        else
            do_zmob_cmd(ch, argument);
        break;
    case 26:                   /* zobj */
        if (!*argument)
            send_to_char(ch, "Type olc for usage.\r\n");
        else
            do_zobj_cmd(ch, argument);
        break;
    case 27:                   /* zdoor */
        if (!*argument)
            send_to_char(ch, "Type olc for usage.\r\n");
        else
            do_zdoor_cmd(ch, argument);
        break;
    case 28:                   /* zreset */
        do_zreset(ch, tmp_strdup("."), 0, SCMD_OLC);
        break;
    case 29:                   /* zonepurge */
        sprintf(buf, " %d", ch->in_room->zone->number);
        do_zonepurge(ch, buf, 0, SCMD_OLC);
        break;
    case 30:                   /* zequip */
        do_zequip_cmd(ch, argument);
        break;
    case 31:                   /* zput */
        do_zput_cmd(ch, argument);
        break;
    case 32:                   /* zgive */
        do_zgive_cmd(ch, argument);
        break;
    case 33:                   /* medit */
        do_mob_medit(ch, argument);
        break;
    case 34:                   /* mstat */
        do_mob_mstat(ch, argument);
        break;
    case 35:                   /* mset */
        do_mob_mset(ch, argument);
        break;
    case 36:                   /* msave */
        zone = ch->in_room->zone;
        if (GET_OLC_MOB(ch)) {
            int m_vnum = GET_OLC_MOB(ch)->mob_specials.shared->vnum;
            for (zone = zone_table; zone; zone = zone->next)
                if (m_vnum >= zone->number * 100 && m_vnum <= zone->top)
                    break;
            if (!zone) {
                send_to_char(ch, "Couldn't find a zone for mobile #%d",
                    m_vnum);
                return;
            }
        }

        if (save_mobs(ch, zone))
            send_to_char(ch, "You save the mobiles in zone #%d (%s).\r\n",
                zone->number, zone->name);
        else
            send_to_char(ch,
                "The mobiles of zone #%d (%s) could not be saved.\r\n",
                zone->number, zone->name);
        break;
    case 37:                   /* sedit */
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 38:                   /* sstat */
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 39:                   /* sset */
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 40:
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 41:                   /* zimplant */
        do_zimplant_cmd(ch, argument);
        break;
    case 42:
        if (!GET_OLC_MOB(ch))
            send_to_char(ch, "yOu beTteR oLC MedIt a mObiLe fIRsT, baBy.\r\n");
        else if (!*argument)
            send_to_char(ch, "Mmimic wHiCH mOb?!\r\n");
        else if (!(mob = real_mobile_proto(atoi(argument))))
            send_to_char(ch, "No sUcH mObiLE eSiSTs, foOl!\r\n");
        else if (mob == GET_OLC_MOB(ch))
            send_to_char(ch, "wHooah.. geT a griP, aYe?\r\n");
        else if (olc_mimic_mob(ch, mob, GET_OLC_MOB(ch), true))
            send_to_char(ch, "dUde...  cOulDn't dO it.\r\n");
        else
            send_to_char(ch, "aLriGHtY thEn.\r\n");
        break;
    case 43:
        do_zpath_cmd(ch, argument);
        break;
    case 44:
        if (!*argument) {
            if (!GET_OLC_SRCH(ch))
                send_to_char(ch,
                    "Usage: olc xedit <command trigger> <keyword>\r\n");
            else {
                send_to_char(ch,
                    "You are currently editing a search that triggers on:\r\n"
                    "%s (%s)\r\n", GET_OLC_SRCH(ch)->command_keys,
                    GET_OLC_SRCH(ch)->keywords);
            }
            return;
        }
        set_char_xedit(ch, argument);
        break;
    case 45:                   /* xset */
        do_olc_xset(ch, argument);
        break;
    case 46:                   /* xstat */
        do_olc_xstat(ch);
        break;
    case 47:
        if (!*argument) {
            if (!mob_p) {
                send_to_char(ch, "Which mobile?\r\n");
                return;
            } else {
                tmp_mob = mob_p;
                j = GET_NPC_VNUM(tmp_mob);
            }
        } else {
            skip_spaces(&argument);
            if (!is_number(argument)) {
                send_to_char(ch, "The argument must be a vnum.\r\n");
                return;
            }
            j = atoi(argument);

            if (!(tmp_mob = real_mobile_proto(j))) {
                send_to_char(ch, "No such mobile exists.\r\n");
                return;
            }

            if (j < (GET_ZONE(ch->in_room) * 100)
                || j > ch->in_room->zone->top) {
                send_to_char(ch,
                    "You cannot olc mload mobiles from other zones.\r\n");
                return;
            }
        }

        if (!is_authorized(ch, WORLDWRITE, NULL)
            && !NPC2_FLAGGED(tmp_mob, NPC2_UNAPPROVED)) {
            send_to_char(ch, "You cannot olc mload approved mobiles.\r\n");
            return;
        }

        if (!(tmp_mob = read_mobile(j))) {
            send_to_char(ch, "Unable to load mobile.\r\n");
        } else {
            char_to_room(tmp_mob, ch->in_room, false);
            act("$N appears next to you.", false, ch, NULL, tmp_mob, TO_CHAR);
            act("$n creates $N in $s hands.", true, ch, NULL, tmp_mob, TO_ROOM);
            slog("OLC: %s mloaded [%d] %s.", GET_NAME(ch),
                GET_NPC_VNUM(tmp_mob), GET_NAME(tmp_mob));
        }

        break;

        //
        // olc show
        //

    case 48:
        if (!*argument) {
            send_to_char(ch, OLC_SHOW_USAGE);
            return;
        }

        argument = one_argument(argument, arg1);

        skip_spaces(&argument);

        // search for rooms with no sound
        if (is_abbrev(arg1, "nosound")) {
            sprintf(buf, "Rooms with no sound in zone %d:\n",
                ch->in_room->zone->number);
            for (struct room_data * room = ch->in_room->zone->world; room;
                room = room->next) {
                if (!room->sounds) {
                    sprintf(buf, "%s[ %6d ] %s\n", buf, room->number,
                        room->name);
                }
            }

            page_string(ch->desc, buf);
            return;
        }
        if (is_abbrev(arg1, "nodesc")) {
            sprintf(buf, "Rooms with no description in zone %d:\n",
                ch->in_room->zone->number);
            for (struct room_data * room = ch->in_room->zone->world; room;
                room = room->next) {
                if (!room->description) {
                    sprintf(buf, "%s[ %6d ] %s\n", buf, room->number,
                        room->name);
                }
            }

            page_string(ch->desc, buf);
            return;
        }

        if (is_abbrev(arg1, "noexitdesc")) {

            //
            // modes
            // mode 1 = ONLY exits with no toroom
            // mode 2 = ONLY exits with a toroom
            // mode 3 = ALL exits
            //

            int mode = 1;

            if (!strcmp(argument, "toroom"))
                mode = 2;
            else if (!strcmp(argument, "all"))
                mode = 3;

            sprintf(buf, "Exits with no description in zone %d:\n",
                ch->in_room->zone->number);
            for (struct room_data * room = ch->in_room->zone->world; room;
                room = room->next) {
                sprintf(buf2, " [ %6d ] %-55s : [ ", room->number, room->name);
                bool found = false;

                for (int i = 0; i < FUTURE; ++i) {
                    struct room_direction_data *exit = room->dir_option[i];

                    if ((mode == 3 && (!exit || !exit->general_description)) ||
                        (mode == 1 && (!exit || (!exit->to_room
                                    && !exit->general_description)))
                        || (mode == 2 && exit && exit->to_room
                            && !exit->general_description)) {

                        found = true;
                        if (exit && exit->to_room)
                            sprintf(buf2, "%s%s%c%s ", buf2, CCGRN(ch, C_NRM),
                                dirs[i][0], CCNRM(ch, C_NRM));
                        else
                            sprintf(buf2, "%s%s%c%s ", buf2, CCYEL(ch, C_NRM),
                                dirs[i][0], CCNRM(ch, C_NRM));
                    } else
                        strcat(buf2, "  ");
                }

                if (found) {
                    strcat(buf2, "]\n");
                    strcat(buf, buf2);
                }
            }

            page_string(ch->desc, buf);
            return;
        }

        send_to_char(ch, "Unknown option: %s\n", arg1);
        break;

    case 49:{                  // recalculate
            struct creature *mob;
            int number;
            char *buf1 = tmp_getword(&argument);
            char *buf2 = tmp_getword(&argument);

            if (!*buf1 || !*buf2 || !isdigit(*buf2)) {
                send_to_char(ch,
                    "Usage: olc recalculate { obj | mob } <number>\r\n");
                return;
            }
            if ((number = atoi(buf2)) < 0) {
                send_to_char(ch, "A NEGATIVE number??\r\n");
                return;
            }
            for (zone = zone_table; zone; zone = zone->next)
                if (number >= zone->number * 100 && number <= zone->top)
                    break;

            if (!zone) {
                send_to_char(ch, "Hmm.  There's no zone attached to it.\r\n");
                break;
            }
            if (!is_authorized(ch, EDIT_ZONE, zone)) {
                send_to_char(ch, "You can't edit that zone.\r\n");
                break;
            }
            if (is_abbrev(buf1, "mobile")) {
                if (!(mob = real_mobile_proto(number))) {
                    send_to_char(ch,
                        "There is no monster with that number.\r\n");
                } else {
                    recalculate_based_on_level(mob);
					set_physical_attribs(mob);
                    send_to_char(ch,
                        "Mobile %d statistics recalculated based on level.\r\n",
                        number);
                }
            } else if (is_abbrev(buf1, "object")) {
                if (!real_object_proto(number)) {
                    send_to_char(ch,
                        "There is no object with that number.\r\n");
                } else {
                    //do something
                    send_to_char(ch, "Unimplemented.\r\n");
                }
            } else {
                send_to_char(ch,
                    "That'll have to be either 'obj' or 'mob'.\r\n");
            }
            break;
        }
    case 50:{                  //olc progmimic {room|obj|mob} <vnum>
            int number;

            char *arg1 = tmp_getword(&argument);    //room|obj|mob
            char *arg2 = tmp_getword(&argument);    //vnum

            if (!*arg1 || !*arg2 || !isdigit(*arg2)) {
                send_to_char(ch,
                    "Usage: olc progmimic {room|obj|mob} <vnum>\r\n");
                return;
            }

            if ((number = atoi(arg2)) < 0) {
                send_to_char(ch, "A NEGATIVE number??\r\n");
                return;
            }

            if (is_abbrev(arg1, "mobile")) {
                struct creature *mob = real_mobile_proto(number);
                if (!mob) {
                    send_to_char(ch,
                        "There is no monster with that number.\r\n");
                } else if (!GET_OLC_MOB(ch)) {
                    send_to_char(ch, "You aren't editing a mobile.\r\n");
                } else if (!GET_NPC_PROG(mob)) {
                    send_to_char(ch, "There is no prog to mimic on %s.\r\n",
                        GET_NAME(mob));
                } else {
                    char *prog = GET_NPC_PROG(mob);
                    if (prog)
                        NPC_SHARED(GET_OLC_MOB(ch))->prog = strdup(prog);
                    send_to_char(ch, "You have mimiced %s's prog.\r\n",
                        GET_NAME(mob));
                }
            } else if (is_abbrev(arg1, "object")) {
                send_to_char(ch,
                    "This feature has not been implemented yet.\r\n");
            } else if (is_abbrev(arg1, "room")) {
                struct room_data *room = real_room(number);
                if (!room) {
                    send_to_char(ch, "There is no room with that number.\r\n");
                } else if (!GET_ROOM_PROG(room)) {
                    send_to_char(ch, "There is no prog to mimic in %s.\r\n",
                        room->name);
                } else {
                    char *prog = GET_ROOM_PROG(room);
                    if (prog)
                        ch->in_room->prog = strdup(prog);
                    send_to_char(ch, "You have mimiced the prog from %s.\r\n",
                        room->name);
                }
            } else {
                send_to_char(ch,
                    "Usage: olc progmimic {room|obj|mob} <vnum>\r\n");
            }
            break;
        }
    default:
        send_to_char(ch, "This action is not supported yet.\r\n");
    }
}

void
recalc_all_mobs(struct creature *ch, const char *argument)
{
    int oldmobile_experience(struct creature *mob);

    struct creature *mob;
    struct zone_data *zone;
    int count = 0;
    FILE *outfile = NULL;
    bool exptest = false;

    if (strcmp(argument, "exptest") == 0) {
        exptest = true;
        send_to_char(ch,
            "Performing EXP test. Check log file for result.\r\n");
    } else if (strcmp(argument, "aA43215nfiss")) {
        // Safe version reports non-recalculated zones and returns
        for (zone = zone_table; zone; zone = zone->next)
            if (ZONE_FLAGGED(zone, ZONE_NORECALC))
                send_to_char(ch, "Zone #%d would not be recalculated.\r\n",
                    zone->number);
        return;
    }
    // Warn about unincluded zones
    for (zone = zone_table; zone; zone = zone->next)
        if (ZONE_FLAGGED(zone, ZONE_NORECALC))
            send_to_char(ch, "Zone #%d is not being recalculated.\r\n",
                zone->number);

    if (exptest) {
        outfile = fopen("recalc.log", "w");
        if (outfile == NULL) {
            errlog("Unable to open recalculation log file.");
            send_to_char(ch, "Cannot open recalc log file. Failing.\r\n");
            return;
        }
        fprintf(outfile, "%6s %22s lvl gen %10s %10s %10s %7s\r\n",
            "id", "name", "old", "new", "base", "fact");
    }
    // Iterate through mobiles
    GHashTableIter iter;
    int vnum;
    g_hash_table_iter_init(&iter, mob_prototypes);
    while (g_hash_table_iter_next(&iter, (gpointer) & vnum, (gpointer) & mob)) {
        for (zone = zone_table; zone; zone = zone->next)
            if (GET_NPC_VNUM(mob) >= zone->number * 100 &&
                GET_NPC_VNUM(mob) <= zone->top)
                break;
        if (!zone) {
            send_to_char(ch, "WARNING: No zone found for mobile %d.\r\n",
                GET_NPC_VNUM(mob));
            continue;
        }

        if (ZONE_FLAGGED(zone, ZONE_NORECALC))
            continue;
        if (exptest) {
            mobile_experience(mob, outfile);
        } else {
            recalculate_based_on_level(mob);
			set_physical_attribs(mob);
            GET_EXP(mob) = mobile_experience(mob, NULL);
        }
        count++;
    }

    if (exptest) {
        fclose(outfile);
        send_to_char(ch, "%d mobiles tested.\r\n", count);
    } else {
        // Save all the zones
        for (zone = zone_table; zone; zone = zone->next)
            if (!ZONE_FLAGGED(zone, ZONE_NORECALC))
                if (!save_mobs(ch, zone))
                    send_to_char(ch, "WARNING: Zone %d was not saved.\r\n",
                        zone->number);

        send_to_char(ch, "%d mobiles recalculated.\r\n", count);
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
    "languages",
    "\n"
};

#define NUM_OLC_HELPS   42
#define NUM_SHOP_TEMPER 6

void
show_olc_help(struct creature *ch, char *arg)
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
        page_string(ch->desc, buf);
        return;
    } else if ((which_help = search_block(arg1, olc_help_keys, false)) < 0) {
        send_to_char(ch,
            "No such keyword.  Type 'olc help' for a list of valid keywords.\r\n");
        return;
    }
    switch (which_help) {
    case 0:       /******* rflags ********/
        strcpy(buf, "ROOM FLAGS:\r\n");
        for (i = 0; i < NUM_ROOM_FLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%-10s %-10s%s\r\n", smallbuf,
                CCCYN(ch, C_NRM), room_bits[i], roomflag_names[i], CCNRM(ch,
                    C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 1:      /********* rsector **********/
        strcpy(buf, "ROOM SECTOR TYPES:\r\n");
        for (i = 0; i < NUM_SECT_TYPES; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n", i, CCCYN(ch, C_NRM),
                sector_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 2:
        strcpy(buf, "ROOM FLOW TYPES:\r\n");
        for (i = 0; i < NUM_FLOW_TYPES; i++) {
            sprintf(buf2, "%2d         %s%-20s%s  '%s'\r\n", i, CCCYN(ch,
                    C_NRM), flow_types[i], CCNRM(ch, C_NRM),
                char_flow_msg[i][2]);
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 3:
        strcpy(buf, "DOOR FLAGS:\r\n");
        for (i = 0; i < NUM_DOORFLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch,
                    C_NRM), exit_bits[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 4:
        strcpy(buf, "ITEM TYPES:\r\n");
        for (i = 0; i < NUM_ITEM_TYPES; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n", i, CCCYN(ch, C_NRM),
                item_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
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
        page_string(ch->desc, buf);
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
        page_string(ch->desc, buf);
        break;
    case 7:
        strcpy(buf, "OBJ WEAR FLAGS:\r\n");
        for (i = 0; i < NUM_WEAR_FLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch,
                    C_NRM), wear_bits[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
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
        page_string(ch->desc, buf);
        break;
    case 9:
        strcpy(buf, "ITEM APPLIES:\r\n");
        for (i = 0; i < NUM_APPLIES; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), apply_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 10:
        strcpy(buf, "RACES:\r\n");
        for (i = 0; i < NUM_RACES; i++) {
            struct race *race = race_by_idnum(i);
            if (race) {
                sprintf(buf2, "%2d         %s%s%s\r\n",
                        i, CCCYN(ch, C_NRM), race->name, CCNRM(ch, C_NRM));
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;
    case 11:
        strcpy(buf, "AFF FLAGS:\r\n");
        for (i = 0; i < NUM_AFF_FLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch,
                    C_NRM), affected_bits_desc[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 12:
        strcpy(buf, "AFF2 FLAGS:\r\n");
        for (i = 0; i < NUM_AFF2_FLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch,
                    C_NRM), affected2_bits_desc[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 13:
        strcpy(buf, "AFF3 FLAGS:\r\n");
        for (i = 0; i < NUM_AFF3_FLAGS; i++) {
            num2str(smallbuf, (1 << i));
            sprintf(buf2, "  %s         %s%s%s\r\n", smallbuf, CCCYN(ch,
                    C_NRM), affected3_bits_desc[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 14:
        if (*arg2) {
            if (!is_number(arg2)) {
                if ((i = search_block(arg2, item_types, 0)) < 0) {
                    send_to_char(ch,
                        "Type olc help otypes for a valid list.\r\n");
                    return;
                }
            } else
                i = atoi(arg2);
            if (i < 0 || i > NUM_ITEM_TYPES) {
                send_to_char(ch, "Object type out of range.\r\n");
                return;
            }
            sprintf(buf, "##          Type       %sValue 0      Value 1"
                "      Value 2      Value 3%s\r\n",
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
            send_to_char(ch, "%s%2d  %s%12s%s  %12s %12s %12s %12s\r\n", buf,
                i, CCCYN(ch, C_NRM), item_types[i], CCNRM(ch, C_NRM),
                item_value_types[i][0], item_value_types[i][1],
                item_value_types[i][2], item_value_types[i][3]);
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
        page_string(ch->desc, buf);
        break;
    case 15:      /** spells **/
        if (*arg2) {
            if (!is_number(arg2)) {
                if ((i = search_block(arg2, (const char * const *)spells, 0)) < 0) {
                    send_to_char(ch,
                        "Type olc help spells for a valid list.\r\n");
                    return;
                }
            } else
                i = atoi(arg2);
            if (i < 0 || i > TOP_SPELL_DEFINE) {
                send_to_char(ch, "Spell num out of range.\r\n");
                return;
            }
            send_to_char(ch, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), spell_to_str(i), CCNRM(ch, C_NRM));
            return;
        }

        strcpy(buf, "SPELLS:\r\n");
        for (i = 1; i < TOP_NPC_SPELL; i++) {
            if (strcmp(spell_to_str(i), "!UNUSED!")) {
                sprintf(buf2, "%3d         %s%s%s\r\n",
                    i, CCCYN(ch, C_NRM), spell_to_str(i), CCNRM(ch, C_NRM));
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;

    case 16:
        strcpy(buf, "ATTACKTYPES:\r\n");
        for (i = 0; i < (TOP_ATTACKTYPE - TOP_SPELL_DEFINE - 1); i++) {
            sprintf(buf2, "%2d         %s%10s  %-10s%s\r\n",
                i, CCCYN(ch, C_NRM), attack_hit_text[i].singular,
                attack_hit_text[i].plural, CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }

        page_string(ch->desc, buf);
        break;

    case 17:
            /** chemicals **/
        send_to_char(ch, "There are no more chemical types.\r\n");
        break;

    case 18:
        if (is_authorized(ch, WORLDWRITE, NULL))
            j = TOT_ZONE_FLAGS;
        else
            j = NUM_ZONE_FLAGS;
        strcpy(buf, "ZONE FLAGS:\r\n");
        for (i = 0; i < j; i++) {
            if (!is_number(zone_flags[i])) {
                sprintf(buf2, "%2d         %s%s%s\r\n",
                    i, CCCYN(ch, C_NRM), zone_flags[i], CCNRM(ch, C_NRM));
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;
    case 19:                   /* materials */
        if (*arg2) {
            if (!is_number(arg2)) {
                if ((i = search_block(arg2, material_names, 0)) < 0) {
                    send_to_char(ch,
                        "Type olc help materials for a valid list.\r\n");
                    return;
                }
            } else
                i = atoi(arg2);
            if (i < 0 || i > TOP_MATERIAL) {
                send_to_char(ch, "Material out of range.\r\n");
                return;
            }
            send_to_char(ch, "%2d         %s%s%s\r\n",
                i, CCYEL(ch, C_NRM), material_names[i], CCNRM(ch, C_NRM));
            return;
        }

        strcpy(buf, "MATERIALS:\r\n");
        for (i = 0; i < TOP_MATERIAL; i++) {
            if (strcasecmp(material_names[i], "*")) {
                sprintf(buf2, "%2d         %s%s%s\r\n",
                    i, CCYEL(ch, C_NRM), material_names[i], CCNRM(ch, C_NRM));
                if ((strlen(buf2) + strlen(buf) + 128) > MAX_STRING_LENGTH) {
                    strcat(buf2, "***OVERFLOW***\r\n");
                    break;
                }
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;

    case 20:          /** bomb types **/
        strcpy(buf, "BOMB TYPES:\r\n");
        for (i = 0; i < MAX_BOMB_TYPES; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), bomb_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 21:          /** fuse types **/
        strcpy(buf, "FUSE TYPES:\r\n");
        for (i = 0; i <= FUSE_MOTION; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), fuse_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 22:          /** mob flags **/
        strcpy(buf, "MOB FLAGS:\r\n");
        for (i = 0; i < NUM_NPC_FLAGS; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), action_bits_desc[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 23:          /** mob flags2 **/
        strcpy(buf, "MOB FLAGS2:\r\n");
        for (i = 0; i < NUM_NPC2_FLAGS; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), action2_bits_desc[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 24:          /** position **/
        strcpy(buf, "POSITIONS:\r\n");
        for (i = 0; i <= NUM_POSITIONS; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), position_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 25:          /** sex **/
        strcpy(buf, "GENDERS:\r\n");
        for (i = 0; i <= 2; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), genders[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;
    case 26:          /** char_class **/
        strcpy(buf, "CLASSES:\r\n");
        for (i = 0; i < TOP_CLASS; i++) {
            if (strcasecmp(class_names[i], "ILL")) {
                sprintf(buf2, "%2d         %s%s%s\r\n",
                    i, CCCYN(ch, C_NRM), class_names[i], CCNRM(ch, C_NRM));
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;
    case 27:          /** itemtypes **/
        strcpy(buf, "ITEM TYPES:\r\n");
        for (i = 0; i < NUM_ITEM_TYPES; i++) {
            if (strcasecmp(item_types[i], "ILL")) {
                sprintf(buf2, "%2d         %s%s%s\r\n",
                    i, CCCYN(ch, C_NRM), item_types[i], CCNRM(ch, C_NRM));
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        break;
    case 28:          /** sflags **/
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 29:          /** tradewith **/
        send_to_char(ch, "Disabled.\r\n");
    case 30:/** implantpos **/
        strcpy(buf, "IMPLANT POSITIONS:\r\n");
        for (i = 0; i < NUM_WEARS; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), wear_implantpos[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 31:/** smokes **/
        strcpy(buf, "SMOKE TYPES:\r\n");
        for (i = 0; i < NUM_SMOKES; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), smoke_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 32:/** temper **/
        strcpy(buf, "SHOP TEMPER:\r\n");
        for (i = 0; i < NUM_SHOP_TEMPER; i++) {
            sprintf(buf2, "%2d         %s%s%s\r\n",
                i, CCCYN(ch, C_NRM), temper_str[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 33:/** vehicles **/
        strcpy(buf, "VEHICLE TYPE BITZ:\r\n");
        for (i = 0; i < NUM_VEHICLE_TYPES; i++) {
            sprintf(buf2, "%4d         %s%s%s\r\n",
                (1 << i), CCCYN(ch, C_NRM), vehicle_types[i], CCNRM(ch,
                    C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 34:/** engines **/
        strcpy(buf, "ENGINE BITZ:\r\n");
        for (i = 0; i < NUM_ENGINE_FLAGS; i++) {
            sprintf(buf2, "%4d         %s%s%s\r\n",
                (1 << i), CCCYN(ch, C_NRM), engine_flags[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 35: /** searches **/
        send_to_char(ch, SEARCH_USAGE);
        break;

    case 36:/** guntypes **/
        strcpy(buf, "GUN TYPES:\r\n");
        for (i = 0; i < NUM_GUN_TYPES; i++) {
            sprintf(buf2,
                "%2d     %s%15s%s ----  %2d d %-3d  (avg. %3d, max %3d)\r\n",
                i, CCCYN(ch, C_NRM), gun_types[i], CCNRM(ch, C_NRM),
                gun_damage[i][0], gun_damage[i][1],
                (gun_damage[i][0] * (gun_damage[i][1] + 1)) / 2,
                gun_damage[i][0] * gun_damage[i][1]);
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 37:/** interfaces **/
        strcpy(buf, "INTERFACE TYPES:\r\n");
        for (i = 0; i < NUM_INTERFACES; i++) {
            sprintf(buf2,
                "%2d     %s%10s%s\r\n",
                i, CCCYN(ch, C_NRM), interface_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 38:/**  microchips **/
        strcpy(buf, "MICROCHIP TYPES:\r\n");
        for (i = 0; i < NUM_CHIPS; i++) {
            sprintf(buf2,
                "%2d     %s%10s%s\r\n",
                i, CCCYN(ch, C_NRM), microchip_types[i], CCNRM(ch, C_NRM));
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
        break;

    case 39:/** searchflags **/
        strcpy(buf, "SEARCH BITS:\r\n");
        for (i = 0; i < NUM_SRCH_BITS; i++) {
            sprintf(buf2, " %s%-15s%s  -  %s\r\n",
                CCCYN(ch, C_NRM), search_bits[i], CCNRM(ch, C_NRM),
                searchflag_help[i]);
            strcat(buf, buf2);
        }
        page_string(ch->desc, buf);
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
        page_string(ch->desc, buf);
        break;
    case 41:{
            show_language_help(ch);
            break;
        }
    default:
        send_to_char(ch,
            "There is no help on this word yet.  Maybe you should write it.\r\n");
    }
    return;
}

#define UNAPPR_USE       "Usage: approve <obj | mob | zone> <vnum>\r\n"

ACMD(do_unapprove)
{

    int rnum = NOTHING;
    int first, last = 0;
    char *arg1, *arg2, *arg3;
    int8_t o_m = 0, zn = 0;
    struct obj_data *obj = NULL;
    struct zone_data *zone = NULL;
    struct creature *mob = NULL;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);
    arg3 = tmp_getword(&argument);

    if (*arg1) {
        if (is_abbrev(arg1, "object"))
            o_m = 0;
        else if (is_abbrev(arg1, "mobile"))
            o_m = 1;
        else if (is_abbrev(arg1, "zone"))
            zn = 1;
        else {
            send_to_char(ch, UNAPPR_USE);
            return;
        }
    } else {
        send_to_char(ch, UNAPPR_USE);
        return;
    }

    if (*arg2)
        rnum = atoi(arg2);
    else {
        send_to_char(ch, UNAPPR_USE);
        return;
    }

    if (zn) {       /********* unapprove zone *********/
        if (!strncmp(arg2, ".", 1))
            zone = ch->in_room->zone;
        else if (!(zone = real_zone(rnum))) {
            send_to_char(ch, "No such zone.\r\n");
            return;
        }

        send_to_char(ch, "Zone approved for olc.\r\n");
        slog("%s approved zone [%d] %s for OLC.", GET_NAME(ch),
            zone->number, zone->name);

        SET_BIT(zone->flags,
            ZONE_MOBS_APPROVED |
            ZONE_OBJS_APPROVED |
            ZONE_ROOMS_APPROVED | ZONE_ZCMDS_APPROVED | ZONE_SEARCH_APPROVED);
        save_zone(ch, zone);

        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;

        if (*arg3 && (is_abbrev(arg3, "mob") || is_abbrev(arg3, "all"))) {
            for (int vnum = first; vnum <= last; vnum++) {
                mob = (struct creature *)g_hash_table_lookup(mob_prototypes,
                    GINT_TO_POINTER(vnum));
                if (mob) {
                    SET_BIT(NPC2_FLAGS(mob), NPC2_UNAPPROVED);
                }
            }
            save_mobs(ch, zone);
            send_to_char(ch, "Mobs approved for olc.\r\n");
            slog("%s approved mobs in zone [%d] %s for OLC.", GET_NAME(ch),
                zone->number, zone->name);
        }
        if (*arg3 && (is_abbrev(arg3, "object") || is_abbrev(arg3, "all"))) {
            for (int vnum = first; vnum <= last; vnum++) {
                obj = (struct obj_data *)g_hash_table_lookup(obj_prototypes,
                    GINT_TO_POINTER(vnum));
                if (obj) {
                    SET_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
                }
            }
            save_objs(ch, zone);
            send_to_char(ch, "Objects approved for olc.\r\n");
            slog("%s approved objects zone [%d] %s for OLC.", GET_NAME(ch),
                zone->number, zone->name);
        }
        return;
    }

    if (!o_m) {     /********* unapprove objects ******/
        obj = real_object_proto(rnum);

        if (!obj) {
            send_to_char(ch,
                "There exists no object with that number, slick.\r\n");
            return;
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (obj->shared->vnum >= (zone->number * 100) &&
                obj->shared->vnum <= zone->top)
                break;
        }

        if (!zone) {
            send_to_char(ch,
                "ERROR: That object does not belong to any zone.\r\n");
            return;
        }

        if (!is_authorized(ch, UNAPPROVE_ZONE, zone)) {
            send_to_char(ch, "You can't unapprove this, BEANHEAD!\r\n");
            return;
        }

        if (IS_SET(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
            send_to_char(ch, "That item is already unapproved.\r\n");
            return;
        }

        SET_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
        send_to_char(ch, "Object unapproved.\r\n");
        slog("%s unapproved object [%d] %s.", GET_NAME(ch),
            obj->shared->vnum, obj->name);

        GET_OLC_OBJ(ch) = obj;
        save_objs(ch, zone);
        GET_OLC_OBJ(ch) = NULL;

    } else {     /** approve mobs */
        mob = real_mobile_proto(rnum);

        if (!mob) {
            send_to_char(ch,
                "There exists no mobile with that number, slick.\r\n");
            return;
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (GET_NPC_VNUM(mob) >= (zone->number * 100) &&
                GET_NPC_VNUM(mob) <= zone->top)
                break;
        }

        if (!zone) {
            send_to_char(ch,
                "ERROR: That object does not belong to any zone.\r\n");
            return;
        }

        if (!is_authorized(ch, UNAPPROVE_ZONE, zone)) {
            send_to_char(ch, "You can't unapprove this, BEANHEAD!\r\n");
            return;
        }

        if (IS_SET(NPC2_FLAGS(mob), NPC2_UNAPPROVED)) {
            send_to_char(ch, "That mobile is already unapproved.\r\n");
            return;
        }

        SET_BIT(NPC2_FLAGS(mob), NPC2_UNAPPROVED);
        send_to_char(ch, "Mobile unapproved.\r\n");
        slog("%s unapproved mobile [%d] %s.", GET_NAME(ch),
            rnum, GET_NAME(mob));

        GET_OLC_MOB(ch) = mob;
        save_mobs(ch, zone);
        GET_OLC_MOB(ch) = NULL;
    }
}

#define APPR_USE  "Usage: approve <zone | mob | obj> <vnum> [all | mob | object]\r\n"
ACMD(do_approve)
{

    int rnum = NOTHING;
    int first, last = 0;
    char *arg1, *arg2, *arg3;
    int8_t o_m = 0, zn = 0;
    struct obj_data *obj = NULL;
    struct zone_data *zone = NULL;
    struct creature *mob = NULL;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);
    arg3 = tmp_getword(&argument);

    if (*arg1) {
        if (is_abbrev(arg1, "object"))
            o_m = 0;
        else if (is_abbrev(arg1, "mobile"))
            o_m = 1;
        else if (is_abbrev(arg1, "zone"))
            zn = 1;
        else {
            send_to_char(ch, APPR_USE);
            return;
        }
    } else {
        send_to_char(ch, APPR_USE);
        return;
    }

    if (*arg2)
        rnum = atoi(arg2);
    else {
        send_to_char(ch, APPR_USE);
        return;
    }

    if (zn) {       /********* approve zone *********/
        if (!strncmp(arg2, ".", 1))
            zone = ch->in_room->zone;
        else if (!(zone = real_zone(rnum))) {
            send_to_char(ch, "No such zone.\r\n");
            return;
        }

        send_to_char(ch, "Zone approved for full inclusion in the game.\r\n"
            "Zone modification from this point must be approved by an olc god.\r\n");
        slog("%s approved zone [%d] %s.", GET_NAME(ch), zone->number,
            zone->name);

        REMOVE_BIT(zone->flags,
            ZONE_MOBS_APPROVED |
            ZONE_OBJS_APPROVED |
            ZONE_ROOMS_APPROVED | ZONE_ZCMDS_APPROVED | ZONE_SEARCH_APPROVED);
        save_zone(ch, zone);

        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;

        if (*arg3 && (is_abbrev(arg3, "mob") || is_abbrev(arg3, "all"))) {
            for (int vnum = first; vnum <= last; vnum++) {
                mob = (struct creature *)g_hash_table_lookup(mob_prototypes,
                    GINT_TO_POINTER(vnum));
                if (mob) {
                    REMOVE_BIT(NPC2_FLAGS(mob), NPC2_UNAPPROVED);
                }
            }

            save_mobs(ch, zone);
            send_to_char(ch,
                "Mobiles approved for full inclusion in the game.\r\n");
            slog("%s approved mobs in zone [%d] %s.", GET_NAME(ch),
                zone->number, zone->name);
        }
        if (*arg3 && (is_abbrev(arg3, "object") || is_abbrev(arg3, "all"))) {
            for (int vnum = first; vnum <= last; vnum++) {
                obj = (struct obj_data *)g_hash_table_lookup(obj_prototypes,
                    GINT_TO_POINTER(vnum));
                if (obj) {
                    REMOVE_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
                }
            }
            save_objs(ch, zone);
            send_to_char(ch,
                "Objects approved for full inclusion in the game.\r\n");
            slog("%s approved objects in zone [%d] %s.", GET_NAME(ch),
                zone->number, zone->name);
        }
        return;
    }

    if (!o_m) {     /********* approve objects ******/
        obj = real_object_proto(rnum);

        if (!obj) {
            send_to_char(ch,
                "There exists no object with that number, slick.\r\n");
            return;
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (obj->shared->vnum >= (zone->number * 100) &&
                obj->shared->vnum <= zone->top)
                break;
        }

        if (!zone) {
            send_to_char(ch,
                "ERROR: That object does not belong to any zone.\r\n");
            return;
        }

        if (!is_authorized(ch, APPROVE_ZONE, zone)) {
            send_to_char(ch, "You can't approve your own objects, silly.\r\n");
            return;
        }

        if (!IS_SET(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED)) {
            send_to_char(ch, "That item is already approved.\r\n");
            return;
        }

        REMOVE_BIT(obj->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
        send_to_char(ch,
            "Object approved for full inclusion in the game.\r\n");
        slog("%s approved object [%d] %s.", GET_NAME(ch), obj->shared->vnum,
            obj->name);

        GET_OLC_OBJ(ch) = obj;
        save_objs(ch, zone);
        GET_OLC_OBJ(ch) = NULL;

    } else {     /** approve mobs */
        mob = real_mobile_proto(rnum);

        if (!mob) {
            send_to_char(ch,
                "There exists no mobile with that number, slick.\r\n");
            return;
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (GET_NPC_VNUM(mob) >= (zone->number * 100) &&
                GET_NPC_VNUM(mob) <= zone->top)
                break;
        }

        if (!zone) {
            send_to_char(ch,
                "ERROR: That object does not belong to any zone.\r\n");
            return;
        }

        if (!is_authorized(ch, APPROVE_ZONE, zone)) {
            send_to_char(ch, "You can't approve your own mobiles, silly.\r\n");
            return;
        }

        if (!IS_SET(NPC2_FLAGS(mob), NPC2_UNAPPROVED)) {
            send_to_char(ch, "That mobile is already approved.\r\n");
            return;
        }

        REMOVE_BIT(NPC2_FLAGS(mob), NPC2_UNAPPROVED);
        send_to_char(ch,
            "Mobile approved for full inclusion in the game.\r\n");
        slog("%s approved mobile [%d] %s.", GET_NAME(ch), rnum, GET_NAME(mob));

        GET_OLC_MOB(ch) = mob;
        save_mobs(ch, zone);
        GET_OLC_MOB(ch) = NULL;
    }
}

/** Can this person edit this zone given these bits set on it. **/
bool
OLC_EDIT_OK(struct creature * ch, struct zone_data * zone, int bits)
{

    if (is_authorized(ch, WORLDWRITE, NULL) && PRF2_FLAGGED(ch, PRF2_WORLDWRITE))
        return true;

    if (ZONE_FLAGGED(zone, ZONE_FULLCONTROL))
        return true;

    if (ZONE_FLAGGED(zone, bits))
        return true;

    return false;
}
