/* ************************************************************************
*  File: mob_matcher.cc                             NOT Part of CircleMUD *
*  Usage: Match mobiles intended to be used to match struct mob_data      *
*         structures in the virtual mobile list                           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: mob_matcher.cc                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 2014 by Daniel Lowe, all rights reserved.
//
#ifdef HAS_CONFIG_H
#endif

#define _GNU_SOURCE

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "strutil.h"
#include "comm.h"
#include "accstr.h"
#include "account.h"
#include "materials.h"
#include "specs.h"
#include "spells.h"

extern GHashTable *mob_prototypes;

struct mob_matcher;
typedef bool (*mob_matcher_func)(struct creature *mob,
                                 struct mob_matcher *matcher);
typedef char *(*mob_info_func)(struct creature *ch,
                               struct creature *mob,
                               struct mob_matcher *matcher);
typedef bool (*mob_matcher_constructor)(struct creature *ch,
                                        struct mob_matcher *matcher,
                                        char *expr);

enum operator {
    OP_EQ,
    OP_LT,
    OP_GT
};

struct mob_matcher {
    mob_matcher_func pred;
    mob_info_func info;
    char *str;
    int8_t idx;
    int num;
    bool negated;
    enum operator op;
};

int parse_char_class(char *arg);
int parse_race(char *arg);

bool
parse_mob_matcher_num_expr(char *expr, struct mob_matcher *matcher)
{
    char *opstr = tmp_getword(&expr);

    if (!strcmp(opstr, "=") || !strcmp(opstr, "=="))
        matcher->op = OP_EQ;
    else if (!strcmp(opstr, "<"))
        matcher->op = OP_LT;
    else if (!strcmp(opstr, ">"))
        matcher->op = OP_GT;
    else if (!strcmp(opstr, "<=")) {
        matcher->op = OP_GT;
        matcher->negated = !matcher->negated;
    } else if (!strcmp(opstr, ">=")) {
        matcher->op = OP_LT;
        matcher->negated = !matcher->negated;
    } else if (is_number(opstr)) {
        matcher->op = OP_EQ;
        expr = opstr;
    }

    if (!is_number(expr)) {
        return false;
    }
    matcher->num = atoi(expr);

    return true;
}

bool
mobile_matches_name(struct creature *mob, struct mob_matcher *matcher)
{
    return strcasestr(mob->player.short_descr, matcher->str);
}

bool
make_mobile_name_matcher(struct creature *ch,
                              struct mob_matcher *matcher,
                              char *expr)
{
    matcher->pred = mobile_matches_name;
    if (*expr == '\0') {
        send_to_char(ch, "name matcher requires a name to match against\r\n");
        return false;
    }
    matcher->str = expr;
    return true;
}

bool
mobile_matches_class(struct creature *mob, struct mob_matcher *matcher)
{
    return IS_CLASS(mob, matcher->num);
}

bool
make_mobile_class_matcher(struct creature *ch,
                          struct mob_matcher *matcher,
                          char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify a class to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_class;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = parse_char_class(expr);
    if (matcher->num < 0 || matcher->num > NUM_CLASSES) {
        send_to_char(ch, "Type olc help class for a valid list of classes.\r\n");
        return false;
    }
    return true;
}

bool
mobile_matches_race(struct creature *mob, struct mob_matcher *matcher)
{
    return IS_RACE(mob, matcher->num);
}

bool
make_mobile_race_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify a race to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_race;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = parse_race(expr);
    if (matcher->num < 0 || matcher->num > NUM_RACES) {
        send_to_char(ch, "Type olc help race for a valid list of classes.\r\n");
        return false;
    }
    return true;
}

bool
mobile_matches_flags(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->idx) {
    case 1:
        return IS_SET(NPC_FLAGS(mob), matcher->num);
    case 2:
        return IS_SET(NPC2_FLAGS(mob), matcher->num);
    }
    return false;
}

bool
make_mobile_flags_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    int index, flag;

    if (*expr == '\0') {
        send_to_char(ch, "You must specify NPC flags to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_flags;
    if ((!(index = 1) || (flag = search_block(expr, action_bits_desc,  0)) < 0)
        &&  (!(index = 2) || (flag = search_block(expr, action2_bits_desc, 0)) < 0)) {
        send_to_char(ch, "There is no NPC flag '%s'.\r\n", expr);
        return false;
    }
    matcher->idx = index;
    matcher->num = 1 << flag;

    return true;
}

bool
mobile_matches_special(struct creature *mob, struct mob_matcher *matcher)
{
    return mob->mob_specials.shared->func && mob->mob_specials.shared->func == spec_list[matcher->num].func;
}

bool
make_mobile_special_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify a mob special to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_special;
    matcher->num = find_spec_index_arg(expr);
    if (matcher->num < 0) {
        send_to_char(ch, "Type show specials for a valid list.\r\n");
        return false;
    }

    return true;
}

bool
mobile_matches_gold(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->op) {
    case OP_EQ:
        return (GET_GOLD(mob) == matcher->num);
    case OP_LT:
        return (GET_GOLD(mob) < matcher->num);
    case OP_GT:
        return (GET_GOLD(mob) > matcher->num);
    }
    return false;
}

char *
mobile_gold_info(struct creature *ch,
                 struct creature *mob,
                 struct mob_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11" PRId64 "%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       GET_GOLD(mob),
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_mobile_gold_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify mobile gold for matching.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_gold;
    matcher->info = mobile_gold_info;
    if (!parse_mob_matcher_num_expr(expr, matcher)) {
        send_to_char(ch, "You must specify a number to match the gold against.\r\n");
        return false;
    }
    return true;
}

bool
mobile_matches_cash(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->op) {
    case OP_EQ:
        return (GET_CASH(mob) == matcher->num);
    case OP_LT:
        return (GET_CASH(mob) < matcher->num);
    case OP_GT:
        return (GET_CASH(mob) > matcher->num);
    }
    return false;
}

char *
mobile_cash_info(struct creature *ch,
                 struct creature *mob,
                 struct mob_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11" PRId64 "%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       GET_CASH(mob),
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_mobile_cash_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify mobile cash for matching.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_cash;
    matcher->info = mobile_cash_info;
    if (!parse_mob_matcher_num_expr(expr, matcher)) {
        send_to_char(ch, "You must specify a number to match the cash against.\r\n");
        return false;
    }
    return true;
}

bool
mobile_matches_hitroll(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->op) {
    case OP_EQ:
        return (GET_HITROLL(mob) == matcher->num);
    case OP_LT:
        return (GET_HITROLL(mob) < matcher->num);
    case OP_GT:
        return (GET_HITROLL(mob) > matcher->num);
    }
    return false;
}

char *
mobile_hitroll_info(struct creature *ch,
                 struct creature *mob,
                 struct mob_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11d%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       GET_HITROLL(mob),
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_mobile_hitroll_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify mobile exp for matching.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_hitroll;
    matcher->info = mobile_hitroll_info;
    if (!parse_mob_matcher_num_expr(expr, matcher)) {
        send_to_char(ch, "You must specify a number to match the hitroll against.\r\n");
        return false;
    }
    return true;
}

bool
mobile_matches_damroll(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->op) {
    case OP_EQ:
        return (GET_DAMROLL(mob) == matcher->num);
    case OP_LT:
        return (GET_DAMROLL(mob) < matcher->num);
    case OP_GT:
        return (GET_DAMROLL(mob) > matcher->num);
    }
    return false;
}

char *
mobile_damroll_info(struct creature *ch,
                 struct creature *mob,
                 struct mob_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11d%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       GET_DAMROLL(mob),
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_mobile_damroll_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify mobile exp for matching.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_damroll;
    matcher->info = mobile_damroll_info;
    if (!parse_mob_matcher_num_expr(expr, matcher)) {
        send_to_char(ch, "You must specify a number to match the damroll against.\r\n");
        return false;
    }
    return true;
}

static struct {
    const char *matcher;
    mob_matcher_constructor constructor;
} match_table[] = {
    { "name", make_mobile_name_matcher },
    { "class", make_mobile_class_matcher },
    { "race", make_mobile_race_matcher },
    { "flags", make_mobile_flags_matcher },
    { "special", make_mobile_special_matcher },
    { "gold", make_mobile_gold_matcher },
    { "cash", make_mobile_cash_matcher },
    { "hitroll", make_mobile_hitroll_matcher },
    { "damroll", make_mobile_damroll_matcher },
    { NULL, NULL }
};

struct mob_matcher *
make_mobile_matcher(struct creature *ch, char *expr)
{
    struct mob_matcher new_matcher;
    char *term;
    bool found = false;

    memset(&new_matcher, 0, sizeof(new_matcher));
    term = tmp_getword(&expr);
    new_matcher.negated = (*term == '!');
    if (new_matcher.negated) {
        term++;
        if (!*term)
            term = tmp_getword(&expr);
    }

    for (int i = 0;match_table[i].matcher && !found;i++) {
        if (!strcasecmp(term, match_table[i].matcher)) {
            if (!match_table[i].constructor(ch, &new_matcher, expr))
                return NULL;
            found = true;
        }
    }

    struct mob_matcher *result;
    if (found) {
        CREATE(result, struct mob_matcher, 1);
        *result = new_matcher;
    } else {
        send_to_char(ch, "'%s' is not a valid match term.\r\n", term);
        result = NULL;
    }
    return result;
}

void
do_show_mobiles(struct creature *ch, char *value, char *argument)
{
    gchar **exprv;
    GList *matchers = NULL;

    if (!*value) {
        send_to_char(ch,
                     "Usage: show mobile [!] <term>[, [!]<term> ...]\r\n"
                     "Precede the term with an exclamation point (!) to select\r\n"
                     "mobiles that don't match.\r\n"
                     "Valid terms can be:\r\n");
        for (int i = 0;match_table[i].matcher;i++) {
            send_to_char(ch, "%s\r\n", match_table[i].matcher);
        }
        return;
    }

    exprv = g_strsplit(tmp_strcat(value, " ", argument, NULL), ",", 0);
    for (gchar **cur_expr = exprv;*cur_expr;cur_expr++) {
        char *expr = *cur_expr;
        // all expressions are either of the format [!] <field> <op> <value>
        // or [!] <field> <value>, separated by commas
        struct mob_matcher *new_matcher = make_mobile_matcher(ch, expr);
        if (new_matcher)
            matchers = g_list_prepend(matchers, new_matcher);
        else
            goto cleanup;
    }

    GHashTableIter iter;
    struct creature *mob;
    int vnum, found;

    acc_string_clear();
    found = 0;

    g_hash_table_iter_init(&iter, mob_prototypes);
    while (g_hash_table_iter_next(&iter, (gpointer)&vnum, (gpointer)&mob)) {
        bool matches = false;
        for (GList *cur_matcher = matchers;cur_matcher;cur_matcher = cur_matcher->next) {
            struct mob_matcher *matcher = cur_matcher->data;
            matches = matcher->pred(mob, matcher);
            if (matcher->negated)
                matches = !matches;
            if (!matches)
                break;
        }
        if (matches) {
            acc_sprintf("%3d. %s[%s%5d%s]%s %-50s%s", ++found,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                        mob->mob_specials.shared->vnum,
                        CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                        GET_NAME(mob), CCNRM(ch, C_NRM));
            for (GList *cur_matcher = matchers;cur_matcher;cur_matcher = cur_matcher->next) {
                struct mob_matcher *matcher = cur_matcher->data;
                if (matcher->info)
                    acc_strcat(" ", matcher->info(ch, mob, matcher), NULL);
            }
            acc_strcat("\r\n", NULL);
        }
    }

    if (found)
        page_string(ch->desc, acc_get_string());
    else
        send_to_char(ch, "No matching mobiles found.\r\n");

cleanup:
    g_strfreev(exprv);
    g_list_foreach(matchers, (GFunc)free, NULL);
    g_list_free(matchers);
}
