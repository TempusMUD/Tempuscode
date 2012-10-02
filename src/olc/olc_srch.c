//
// File: olc_srch.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "interpreter.h"
#include "clan.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "spells.h"
#include "obj_data.h"
#include "strutil.h"
#include "search.h"
#include "olc.h"

const char *olc_xset_keys[] = {
    "triggers",
    "keywords",
    "to_vict",
    "to_room",
    "command",
    "value",
    "to_remote",
    "flags",
    "fail_chance",
    "\n",
};

#define NUM_XSET_COMMANDS 8

#define srch_p GET_OLC_SRCH(ch)
void
do_olc_xset(struct creature *ch, char *argument)
{

    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i, command, cur_flags, tmp_flags, flag;
    int state = 0;

    if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_SEARCH_APPROVED)) {
        send_to_char(ch,
            "This zone has not been approved for search editing.\r\n");
        return;
    }
    if (!GET_OLC_SRCH(ch)) {
        send_to_char(ch, "You are not currently editing a search.\r\n");
        return;
    }

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*arg1) {
        sprintf(buf, "Valid xset commands:\r\n%s", CCYEL(ch, C_NRM));
        for (i = 0; i < NUM_XSET_COMMANDS; i++) {
            strcat(buf, olc_xset_keys[i]);
            strcat(buf, "\r\n");
        }
        strcat(buf, CCYEL(ch, C_NRM));
        page_string(ch->desc, buf);
        return;
    }

    if ((command = search_block(arg1, olc_xset_keys, 0)) < 0) {
        send_to_char(ch, "No such xset command '%s'.\r\n", arg1);
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Set search %s to what?\r\n", olc_xset_keys[command]);
        return;
    }
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    switch (command) {
    case 0:                    /* triggers */
        if (srch_p->command_keys)
            free(srch_p->command_keys);
        srch_p->command_keys = strdup(argument);
        send_to_char(ch, "Search command triggers set.\r\n");
        break;
    case 1:                    /* keywords */
        if (srch_p->keywords)
            free(srch_p->keywords);
        if (argument[0] == '~')
            srch_p->keywords = NULL;
        else
            srch_p->keywords = strdup(argument);
        send_to_char(ch, "Search argument keywords set.\r\n");
        break;
    case 2:                    /* to_vict */
        if (srch_p->to_vict)
            free(srch_p->to_vict);
        if (argument[0] == '~')
            srch_p->to_vict = NULL;
        else
            srch_p->to_vict = strdup(argument);
        send_to_char(ch, "To_vict message set.\r\n");
        break;
    case 3:                    /* to_room */
        if (srch_p->to_room)
            free(srch_p->to_room);
        if (argument[0] == '~')
            srch_p->to_room = NULL;
        else
            srch_p->to_room = strdup(argument);
        send_to_char(ch, "To_room message set.\r\n");
        break;
    case 4:                    /* command */
        if ((command = search_block(arg1, search_commands, 0)) < 0) {
            send_to_char(ch, "No such search command '%s'.\r\n", arg1);
            return;
        }
        srch_p->command = command;
        send_to_char(ch, "Search command set.\r\n");
        break;
    case 5:                    /* value */
        if ((i = atoi(arg1)) < 0 || i > 2) {
            send_to_char(ch, "xset val <0 | 1 | 2> <value>\r\n");
            return;
        }
        if (!*arg2) {
            send_to_char(ch, "Set the value to what?\r\n");
            return;
        }
        if (!isnumber(arg2)) {
            send_to_char(ch, "The value must be an integer.\r\n");
            return;
        }
        srch_p->arg[i] = atoi(arg2);
        send_to_char(ch, "Ok, value set.\r\n");
        break;
    case 6:                    /* to_remote */
        if (srch_p->to_remote)
            free(srch_p->to_remote);
        if (argument[0] == '~')
            srch_p->to_remote = NULL;
        else
            srch_p->to_remote = strdup(argument);
        send_to_char(ch, "To_remote message set.\r\n");
        break;

    case 7:                    /* flags */
        tmp_flags = 0;
        argument = one_argument(argument, arg1);

        if (*arg1 == '+')
            state = 1;
        else if (*arg1 == '-')
            state = 2;
        else {
            send_to_char(ch,
                "Usage: olc xset flags [+/-] [FLAG, FLAG, ...]\r\n");
            return;
        }

        argument = one_argument(argument, arg1);

        cur_flags = srch_p->flags;

        while (*arg1) {
            if ((flag = search_block(arg1, search_bits, false)) == -1) {
                send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
            } else
                tmp_flags = tmp_flags | (1 << flag);

            argument = one_argument(argument, arg1);
        }

        if (state == 1) {
            if (((cur_flags | tmp_flags) & SRCH_REPEATABLE) &&
                ((cur_flags | tmp_flags) & SRCH_FAIL_TRIP)) {
                send_to_char(ch, "A search can't be repeatable and "
                    "fail_trip, wackjob!  Where did we " "find you?\r\n");
                return;
            }
            cur_flags = cur_flags | tmp_flags;
        } else {
            tmp_flags = cur_flags & tmp_flags;
            cur_flags = cur_flags ^ tmp_flags;
        }

        srch_p->flags = cur_flags;

        if (tmp_flags == 0 && cur_flags == 0) {
            send_to_char(ch, "Search flags set\r\n");
        } else if (tmp_flags == 0)
            send_to_char(ch, "Search flags not altered.\r\n");
        else {
            send_to_char(ch, "Search flags set.\r\n");
        }
        break;
    case 8:
        i = atoi(argument);
        if (i < 0 || i > 100) {
            send_to_char(ch,
                "Fail chance for searches is a percentage (0 to 100)\r\n");
            return;
        }
        srch_p->fail_chance = i;
        send_to_char(ch,
            "This search will now have a %d%% chance of failure.\r\n", i);
        break;
    default:
        send_to_char(ch, "This option currently unavailable.\r\n");
        break;
    }
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}

struct special_search_data *
do_create_search(struct creature *ch, char *arg)
{

    struct special_search_data *srch = NULL;
    char *trigger = tmp_getword(&arg);
    char *keyword = tmp_getword(&arg);

    if (!*trigger) {
        send_to_char(ch, "USAGE: create search <trigger word> <keywords>\r\n");
        return NULL;
    }

    for (srch = ch->in_room->search; srch; srch = srch->next) {
        if (isname_exact(trigger, srch->command_keys) &&
            *keyword && srch->keywords
            && isname_exact(keyword, srch->keywords)) {
            send_to_char(ch,
                "There is already a search here on that trigger.\r\n");
            return NULL;
        }
    }

    if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ROOMS_APPROVED)) {
        send_to_char(ch, "World olc is not approved for this zone.\r\n");
        return NULL;
    }

    CREATE(srch, struct special_search_data, 1);

    srch->command_keys = strdup(trigger);
    if (*keyword)
        srch->keywords = strdup(keyword);
    else
        srch->keywords = NULL;
    srch->to_room = NULL;
    srch->to_vict = NULL;
    srch->flags = 0;
    srch->command = SEARCH_COM_NONE;
    srch->arg[0] = -1;
    srch->arg[1] = -1;
    srch->arg[2] = -1;
    srch->next = ch->in_room->search;

    ch->in_room->search = srch;

    return (srch);
}

int
do_destroy_search(struct creature *ch, char *arg)
{
    struct special_search_data *srch = NULL, *temp = NULL;
    char triggers[MAX_INPUT_LENGTH], keywords[MAX_INPUT_LENGTH];

    two_arguments(arg, triggers, keywords);

    if (!*triggers) {
        send_to_char(ch, "USAGE: destroy search <trigger word> [keyword]\r\n");
        return 0;
    }

    for (srch = ch->in_room->search; srch; srch = srch->next) {
        if (isname_exact(triggers, srch->command_keys) &&
            (!keywords[0] || (!srch->keywords
                    || isname_exact(keywords, srch->keywords)))) {
            break;
        }
    }

    if (!srch) {
        send_to_char(ch, "There is no such search here.\r\n");
        return 0;
    }

    if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ROOMS_APPROVED)) {
        send_to_char(ch, "World olc is not approved for this zone.\r\n");
        return 0;
    }
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (GET_OLC_SRCH(tch) == srch)
            GET_OLC_SRCH(tch) = NULL;
    }
    REMOVE_FROM_LIST(srch, ch->in_room->search, next);
    if (srch->command_keys)
        free(srch->command_keys);
    if (srch->keywords)
        free(srch->keywords);
    if (srch->to_vict)
        free(srch->to_vict);
    if (srch->to_room)
        free(srch->to_room);
    free(srch);
    send_to_char(ch, "Search destroyed.\r\n");
    return 1;
}

int
set_char_xedit(struct creature *ch, char *argument)
{

    struct special_search_data *srch = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1)
        return 0;

    if (!*arg2 && is_abbrev(arg1, "exit")) {
        send_to_char(ch, "Exiting search editor.\r\n");
        GET_OLC_SRCH(ch) = srch;
        return 1;
    }

    for (srch = ch->in_room->search; srch; srch = srch->next)
        if (isname_exact(arg1, srch->command_keys) &&
            (!*arg2 || !srch->keywords
                || isname_exact(arg2, srch->keywords))) {
            GET_OLC_SRCH(ch) = srch;
            send_to_char(ch,
                "You are now editing a search that triggers on:\r\n"
                "%s (%s)\r\n", GET_OLC_SRCH(ch)->command_keys,
                GET_OLC_SRCH(ch)->keywords ?
                GET_OLC_SRCH(ch)->keywords : "NULL");
            return 1;
        }
    send_to_char(ch, "No such search in room.\r\n");
    return 0;
}

void
acc_format_search_data(struct creature *ch,
    struct room_data *room, struct special_search_data *cur_search)
{
    struct obj_data *obj = NULL;
    struct creature *mob = NULL;

    acc_sprintf("%sCommand triggers:%s %s, %skeywords:%s %s\r\n",
        CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
        cur_search->command_keys ? cur_search->command_keys : "None.",
        CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
        (SRCH_FLAGGED(cur_search, SRCH_CLANPASSWD) && room &&
            !clan_house_can_enter(ch, room)) ? "*******" :
        (cur_search->keywords ? cur_search->keywords : "None."));
    acc_sprintf(" To_vict  : %s\r\n To_room  : %s\r\n To_remote: %s\r\n",
        cur_search->to_vict ? cur_search->to_vict : "None",
        cur_search->to_room ? cur_search->to_room : "None",
        cur_search->to_remote ? cur_search->to_remote : "None");

    acc_sprintf("Fail_chance: %d\r\n", cur_search->fail_chance);

    switch (cur_search->command) {
    case SEARCH_COM_DOOR:
        acc_sprintf("DOOR  Room #: %d, Direction: %d, Mode: %d.\r\n",
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_MOBILE:
        mob = real_mobile_proto(cur_search->arg[0]);
        acc_sprintf("MOB  Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
            cur_search->arg[0], CCYEL(ch, C_NRM),
            mob ? GET_NAME(mob) : "NULL", CCNRM(ch, C_NRM), cur_search->arg[1],
            cur_search->arg[2]);
        break;
    case SEARCH_COM_OBJECT:
        obj = real_object_proto(cur_search->arg[0]);
        acc_sprintf("OBJECT Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
            cur_search->arg[0],
            CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM),
            cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_REMOVE:
        obj = real_object_proto(cur_search->arg[0]);
        acc_sprintf
            ("REMOVE  Obj Vnum : %d (%s%s%s), Room # : %d, Val 2: %d.\r\n",
            cur_search->arg[0], CCGRN(ch, C_NRM), obj ? obj->name : "NULL",
            CCNRM(ch, C_NRM), cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_EQUIP:
        obj = real_object_proto(cur_search->arg[1]);
        acc_sprintf("EQUIP  ----- : %d, Obj Vnum : %d (%s%s%s), Pos : %d.\r\n",
            cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM), cur_search->arg[2]);
        break;
    case SEARCH_COM_GIVE:
        obj = real_object_proto(cur_search->arg[1]);
        acc_sprintf("GIVE  ----- : %d, Obj Vnum : %d (%s%s%s), Max : %d.\r\n",
            cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM), cur_search->arg[2]);
        break;
    case SEARCH_COM_NONE:
        acc_sprintf("NONE       %5d        %5d        %5d\r\n",
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_TRANSPORT:
        acc_sprintf("TRANS      %5d        %5d        %5d\r\n",
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_SPELL:
        acc_sprintf("SPELL      %5d        %5d        %5d (%s)\r\n",
            cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2],
            (cur_search->arg[2] > 0 && cur_search->arg[2] < TOP_NPC_SPELL) ?
            spell_to_str(cur_search->arg[2]) : "NULL");
        break;
    case SEARCH_COM_DAMAGE:
        acc_sprintf("DAMAGE      %5d        %5d        %5d (%s)\r\n",
            cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2],
            (cur_search->arg[2] > 0 && cur_search->arg[2] < TYPE_SUFFERING) ?
            spell_to_str(cur_search->arg[2]) : "NULL");
        break;
    case SEARCH_COM_SPAWN:
        acc_sprintf("SPAWN  Spawn_rm: %5d   Targ_rm:%5d   Hunt: %5d\r\n",
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_LOADROOM:
        acc_sprintf("LOADROOM  NewLoad: %5d    MaxLevel:%5d    %5d\r\n",
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;

    default:
        acc_sprintf("ERROR (%d)  %5d        %5d        %5d\r\n",
            cur_search->command, cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2]);
        break;
    }
    acc_sprintf("Flags: %s%s%s\r\n",
        CCBLU_BLD(ch, C_NRM),
        tmp_printbits(cur_search->flags, search_bits), CCNRM(ch, C_NRM));
}

void
print_search_data_to_buf(struct creature *ch, struct room_data *room,
    struct special_search_data *cur_search, char *buf)
{

    struct obj_data *obj = NULL;
    struct creature *mob = NULL;

    sprintf(buf, "%sCommand triggers:%s %s, %skeywords:%s %s\r\n",
        CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
        cur_search->command_keys ? cur_search->command_keys : "None.",
        CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
        (SRCH_FLAGGED(cur_search, SRCH_CLANPASSWD) && room &&
            !clan_house_can_enter(ch, room)) ? "*******" :
        (cur_search->keywords ? cur_search->keywords : "None."));
    sprintf(buf, "%s To_vict  : %s\r\n To_room  : %s\r\n To_remote: %s\r\n",
        buf, cur_search->to_vict ? cur_search->to_vict : "None",
        cur_search->to_room ? cur_search->to_room : "None",
        cur_search->to_remote ? cur_search->to_remote : "None");

    sprintf(buf, "%s Fail_chance: %d\r\n", buf, cur_search->fail_chance);

    switch (cur_search->command) {
    case SEARCH_COM_DOOR:
        sprintf(buf, "%s DOOR  Room #: %d, Direction: %d, Mode: %d.\r\n", buf,
            cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_MOBILE:
        mob = real_mobile_proto(cur_search->arg[0]);
        sprintf(buf, "%s MOB  Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
            buf, cur_search->arg[0], CCYEL(ch, C_NRM),
            mob ? GET_NAME(mob) : "NULL", CCNRM(ch, C_NRM), cur_search->arg[1],
            cur_search->arg[2]);
        break;
    case SEARCH_COM_OBJECT:
        obj = real_object_proto(cur_search->arg[0]);
        sprintf(buf, "%s OBJECT Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
            buf, cur_search->arg[0],
            CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM),
            cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_REMOVE:
        obj = real_object_proto(cur_search->arg[0]);
        sprintf(buf,
            "%s REMOVE  Obj Vnum : %d (%s%s%s), Room # : %d, Val 2: %d.\r\n",
            buf, cur_search->arg[0], CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM),
            cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_EQUIP:
        obj = real_object_proto(cur_search->arg[1]);
        sprintf(buf,
            "%s EQUIP  ----- : %d, Obj Vnum : %d (%s%s%s), Pos : %d.\r\n", buf,
            cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM), cur_search->arg[2]);
        break;
    case SEARCH_COM_GIVE:
        obj = real_object_proto(cur_search->arg[1]);
        sprintf(buf,
            "%s GIVE  ----- : %d, Obj Vnum : %d (%s%s%s), Max : %d.\r\n", buf,
            cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
            obj ? obj->name : "NULL", CCNRM(ch, C_NRM), cur_search->arg[2]);
        break;
    case SEARCH_COM_NONE:
        sprintf(buf, "%s NONE       %5d        %5d        %5d\r\n",
            buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_TRANSPORT:
        sprintf(buf, "%s TRANS      %5d        %5d        %5d\r\n",
            buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_SPELL:
        sprintf(buf, "%s SPELL      %5d        %5d        %5d (%s)\r\n",
            buf, cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2],
            (cur_search->arg[2] > 0 && cur_search->arg[2] < TOP_NPC_SPELL) ?
            spell_to_str(cur_search->arg[2]) : "NULL");
        break;
    case SEARCH_COM_DAMAGE:
        sprintf(buf, "%s DAMAGE      %5d        %5d        %5d (%s)\r\n",
            buf, cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2],
            (cur_search->arg[2] > 0 && cur_search->arg[2] < TYPE_SUFFERING) ?
            spell_to_str(cur_search->arg[2]) : "NULL");
        break;
    case SEARCH_COM_SPAWN:
        sprintf(buf, "%s SPAWN  Spawn_rm: %5d   Targ_rm:%5d   Hunt: %5d\r\n",
            buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;
    case SEARCH_COM_LOADROOM:
        sprintf(buf, "%s LOADROOM  NewLoad: %5d    MaxLevel:%5d    %5d\r\n",
            buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
        break;

    default:
        sprintf(buf, "%s ERROR (%d)  %5d        %5d        %5d\r\n",
            buf, cur_search->command, cur_search->arg[0],
            cur_search->arg[1], cur_search->arg[2]);
        break;
    }
    sprintbit(cur_search->flags, search_bits, buf2);
    sprintf(buf, "%s Flags: %s%s%s\r\n",
        buf, CCBLU_BLD(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
}

void
do_olc_xstat(struct creature *ch)
{
    if (!GET_OLC_SRCH(ch)) {
        send_to_char(ch, "You are not currently editing a search.\r\n");
        return;
    }
    print_search_data_to_buf(ch, ch->in_room, GET_OLC_SRCH(ch), buf);
    send_to_char(ch, "%s", buf);

}
