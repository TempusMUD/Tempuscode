/* ************************************************************************
*   File: gnetsys.cc                                    Part of CircleMUD *
*  Usage: handle functions dealing with the slobal network systems        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: gnetsys.cc                         -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 2001 by Daniel Lowe, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "strutil.h"

extern int skill_sort_info[MAX_SKILLS - MAX_SPELLS + 1];
extern struct descriptor_data *descriptor_list;
void set_desc_state(int state, struct descriptor_data *d);

void
perform_net_help(struct descriptor_data *d)
{
    d_printf(d, "       System Help\r\n");
    d_printf(d, "------------------------------------------------------------------\r\n");
    d_printf(d, "            logout - disconnects from network\r\n");
    d_printf(d, "              exit - synonym for logout\r\n");
    d_printf(d, "               who - lists users logged into network\r\n");
    d_printf(d, "     finger <user> - displays information about user\r\n");
    d_printf(d, "write <user> <msg> - sends a private message to a user\r\n");
    d_printf(d, "       wall <user> - sends a message to all users logged in\r\n");
    d_printf(d, "              help - displays this menu\r\n");
    d_printf(d, "                 ? - synonym for help\r\n");
    if (IS_CYBORG(d->creature)) {
        d_printf(d, "              list - lists programs executing locally\r\n");
        d_printf(d, "    load <program> - loads and executes programs into local memory\r\n");
    }
}

void
perform_net_write(struct descriptor_data *d, char *arg)
{
    char targ[MAX_INPUT_LENGTH];
    char msg[MAX_INPUT_LENGTH];
    struct creature *vict;

    half_chop(arg, targ, msg);

    if (!*targ || !*msg) {
        d_printf(d, "Usage: write <user> <message>\r\n");
        return;
    }

    vict = get_player_vis(d->creature, targ, 1);
    if (!vict)
        vict = get_player_vis(d->creature, targ, 0);

    if (!vict || STATE(vict->desc) != CXN_NETWORK) {
        d_printf(d, "Error: user not logged in\r\n");
        return;
    }

    d_printf(d, "Message sent to %s: %s\r\n", GET_NAME(vict), msg);
    d_printf(vict->desc, "Message from %s: %s\r\n", GET_NAME(d->creature), msg);
}

void
perform_net_wall(struct descriptor_data *d, char *arg)
{
    struct descriptor_data *r_d;

    if (!*arg) {
        d_printf(d, "Usage: wall <message>\r\n");
        return;
    }

    for (r_d = descriptor_list; r_d; r_d = r_d->next) {
        if (STATE(r_d) != CXN_NETWORK)
            continue;
        d_printf(r_d, "%s walls:%s\r\n", GET_NAME(d->creature), arg);
    }
}

void
perform_net_load(struct descriptor_data *d, char *arg)
{
    int skill_num, percent;
    long int cost;

    skip_spaces(&arg);
    if (!*arg) {
        d_printf(d, "Usage: load <program>\r\n");
        return;
    }

    skill_num = find_skill_num(arg);
    if (skill_num < 1) {
        d_printf(d, "Error: program '%s' not found\r\n", arg);
        return;
    }

    if ((SPELL_GEN(skill_num, CLASS_CYBORG) > 0
            && GET_CLASS(d->creature) != CLASS_CYBORG)
        || (GET_REMORT_GEN(d->creature) < SPELL_GEN(skill_num, CLASS_CYBORG))
        || (GET_LEVEL(d->creature) < SPELL_LEVEL(skill_num, CLASS_CYBORG))) {
        d_printf(d, "Error: resources unavailable to load '%s'\r\n", arg);
        return;
    }

    if (GET_SKILL(d->creature, skill_num) >= LEARNED(d->creature)) {
        d_printf(d, "Program fully installed on local system.\r\n");
        return;
    }

    cost = GET_SKILL_COST(d->creature, skill_num);
    d_printf(d, "Program cost: %10ld  Account balance; %'" PRId64 "\r\n",
        cost, d->account->bank_future);

    if (d->account->bank_future < cost) {
        d_printf(d, "Error: insufficient funds in your account\r\n");
        return;
    }

    withdraw_future_bank(d->account, cost);
    percent = MIN(MAXGAIN(d->creature),
        MAX(MINGAIN(d->creature), GET_INT(d->creature) * 2));
    percent = MIN(LEARNED(d->creature) -
        GET_SKILL(d->creature, skill_num), percent);
    SET_SKILL(d->creature, skill_num, GET_SKILL(d->creature,
            skill_num) + percent);
    d_printf(d,
        "Program download: %s terminating, %d percent transfer.\r\n",
        spell_to_str(skill_num), percent);
    if (GET_SKILL(d->creature, skill_num) >= LEARNED(d->creature))
        d_printf(d, "Program fully installed on local system.\r\n");
    else
        d_printf(d, "Program %d%% installed on local system.\r\n",
            GET_SKILL(d->creature, skill_num));
}

void
perform_net_who(struct creature *ch, const char *arg __attribute__ ((unused)))
{
    struct descriptor_data *d = NULL;
    int count = 0;

    strcpy_s(buf, sizeof(buf), "Visible users of the global net:\r\n");
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CXN_NETWORK)
            continue;
        if (!can_see_creature(ch, d->creature))
            continue;

        count++;
        snprintf_cat(buf, sizeof(buf), "   (%03d)     %s\r\n", count,
            GET_NAME(d->creature));
        continue;
    }
    snprintf_cat(buf, sizeof(buf), "\r\n%d users detected.\r\n", count);
    page_string(ch->desc, buf);
}

void
perform_net_finger(struct creature *ch, const char *arg)
{
    struct creature *vict = NULL;

    skip_spaces_const(&arg);
    if (!*arg) {
        send_to_char(ch, "Usage: finger <user>\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg)) || !vict->desc ||
        STATE(vict->desc) != CXN_NETWORK || (GET_LEVEL(ch) < LVL_AMBASSADOR
            && GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
        send_to_char(ch, "No such user detected.\r\n");
        return;
    }
    send_to_char(ch, "Finger results:\r\n"
        "Name:  %s, Level %d %s %s.\r\n"
        "Logged in at: %s.\r\n",
        GET_NAME(vict), GET_LEVEL(vict),
                 race_name_by_idnum(GET_RACE(vict)),
        class_names[(int)GET_CLASS(vict)],
        vict->in_room != NULL ? ch->in_room->name : "NOWHERE");
}

void
perform_net_list(struct creature *ch)
{
    int i, sortpos;

    strcpy_s(buf2, sizeof(buf2), "Directory listing for local programs.\r\n\r\n");

    for (sortpos = 1; sortpos < MAX_SKILLS - MAX_SPELLS; sortpos++) {
        i = skill_sort_info[sortpos];
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
            strcat_s(buf2, sizeof(buf2), "**OVERFLOW**\r\n");
            return;
        }
        if ((CHECK_SKILL(ch, i) || is_able_to_learn(ch, i)) &&
            SPELL_LEVEL(i, 0) <= LVL_GRIMP) {
            snprintf(buf, sizeof(buf), "%-30s [%3d] percent installed.\r\n",
                spell_to_str(i), GET_SKILL(ch, i));
            strcat_s(buf2, sizeof(buf2), buf);
        }
    }

    page_string(ch->desc, buf2);
}

void
handle_network(struct descriptor_data *d, char *arg)
{
    char arg1[MAX_INPUT_LENGTH];

    if (!*arg)
        return;
    arg = one_argument(arg, arg1);
    if (is_abbrev(arg1, "who")) {
        perform_net_who(d->creature, "");
    } else if (*arg1 == '?' || is_abbrev(arg1, "help")) {
        perform_net_help(d);
    } else if (is_abbrev(arg1, "finger")) {
        perform_net_finger(d->creature, arg);
    } else if (is_abbrev(arg1, "write")) {
        perform_net_write(d, arg);
    } else if (is_abbrev(arg1, "wall")) {
        perform_net_wall(d, arg);
    } else if (is_abbrev(arg1, "more")) {
        if (d->showstr_head)
            show_string(d);
        else
            d_printf(d, "Error: Resource not available\r\n");
    } else if (*arg1 == '@' || is_abbrev(arg1, "exit")
        || is_abbrev(arg1, "logout")) {
        slog("User %s disconnecting from net.", GET_NAME(d->creature));
        set_desc_state(CXN_PLAYING, d);
        d_printf(d, "Connection closed.\r\n");
        act("$n disconnects from the network.", true, d->creature, NULL, NULL,
            TO_ROOM);
    } else if (IS_CYBORG(d->creature) && is_abbrev(arg1, "list")) {
        perform_net_list(d->creature);
    } else if (IS_CYBORG(d->creature) && (is_abbrev(arg1, "load")
            || is_abbrev(arg, "download"))) {
        perform_net_load(d, arg);
    } else {
        d_printf(d, "%s: command not found\r\n", arg1);
    }
}

#undef __gnetsys_c__
