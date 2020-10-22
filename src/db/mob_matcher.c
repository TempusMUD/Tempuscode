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
#include "account.h"
#include "screen.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "strutil.h"
#include "comm.h"
#include "accstr.h"
#include "materials.h"
#include "specs.h"
#include "spells.h"

extern GHashTable *mob_prototypes;

struct mob_matcher;

enum matcher_varient {
    MATCHER_STD,
    MATCHER_GOLD,
    MATCHER_CASH,
    MATCHER_HITROLL,
    MATCHER_DAMROLL,
    MATCHER_ALIGNMENT,
    MATCHER_EXP,
};

enum operator {
    OP_EQ,
    OP_LT,
    OP_GT
};

typedef bool (*mob_matcher_func)(struct creature *mob,
                                 struct mob_matcher *matcher);
typedef char *(*mob_info_func)(struct creature *ch,
                               struct creature *mob,
                               struct mob_matcher *matcher);
typedef bool (*mob_matcher_constructor)(struct creature *ch,
                                        struct mob_matcher *matcher,
                                        enum matcher_varient varient,
                                        char *expr);


struct mob_matcher {
    mob_matcher_func pred;
    mob_info_func info;
    enum matcher_varient varient;
    char *str;
    int8_t idx;
    int64_t num;
    bool negated;
    enum operator op;
};

int parse_char_class(char *arg);
int parse_race(char *arg);

bool
mobile_matches_name(struct creature *mob, struct mob_matcher *matcher)
{
    return strcasestr(mob->player.short_descr, matcher->str);
}

bool
make_mobile_name_matcher(struct creature *ch,
                         struct mob_matcher *matcher,
                         __attribute__ ((unused)) enum matcher_varient varient,
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
                          __attribute__ ((unused)) enum matcher_varient varient,
                          char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify a class to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_class;
    if (is_number(expr)) {
        matcher->num = atoi(expr);
    } else {
        matcher->num = parse_char_class(expr);
    }
    if (matcher->num < 0 || matcher->num >= NUM_CLASSES) {
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
                         __attribute__ ((unused)) enum matcher_varient varient,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify a race to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_race;
    if (is_number(expr)) {
        matcher->num = atoi(expr);
    } else {
        matcher->num = parse_race(expr);
    }
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
                          __attribute__ ((unused)) enum matcher_varient varient,
                          char *expr)
{
    int index, flag;

    if (*expr == '\0') {
        send_to_char(ch, "You must specify NPC flags to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_flags;
    index = 1;
    flag = search_block(expr, action_bits_desc,  0);
    if (flag < 0) {
        index = 2;
        flag = search_block(expr, action2_bits_desc,  0);
    }
    if (flag < 0) {
        send_to_char(ch, "There is no NPC flag '%s'.\r\n", expr);
        return false;
    }

    matcher->idx = index;
    matcher->num = 1U << flag;

    return true;
}


bool
mobile_matches_affect(struct creature *mob, struct mob_matcher *matcher)
{
    switch (matcher->idx) {
    case 1:
        return IS_SET(AFF_FLAGS(mob), matcher->num);
    case 2:
        return IS_SET(AFF2_FLAGS(mob), matcher->num);
    case 3:
        return IS_SET(AFF3_FLAGS(mob), matcher->num);
    }
    return false;
}

bool
make_mobile_affect_matcher(struct creature *ch,
                           struct mob_matcher *matcher,
                           __attribute__ ((unused)) enum matcher_varient varient,
                           char *expr)
{
    int index, flag;

    if (*expr == '\0') {
        send_to_char(ch, "You must specify an affect to match.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_affect;
    index = 1;
    flag = search_block(expr, affected_bits_desc,  0);
    if (flag < 0) {
        index = 2;
        flag = search_block(expr, affected2_bits_desc,  0);
    }
    if (flag < 0) {
        index = 3;
        flag = search_block(expr, affected3_bits_desc,  0);
    }
    if (flag < 0) {
        send_to_char(ch, "There is no NPC affect '%s'.\r\n", expr);
        return false;
    }

    matcher->idx = index;
    matcher->num = 1U << flag;

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
                            __attribute__ ((unused)) enum matcher_varient varient,
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

int64_t
get_mob_value(struct creature *mob, enum matcher_varient varient)
{
    switch (varient) {
    case MATCHER_GOLD:
        return GET_GOLD(mob);
    case MATCHER_CASH:
        return GET_CASH(mob);
    case MATCHER_HITROLL:
        return GET_HITROLL(mob);
    case MATCHER_DAMROLL:
        return GET_DAMROLL(mob);
    case MATCHER_ALIGNMENT:
        return GET_ALIGNMENT(mob);
    case MATCHER_EXP:
        return GET_EXP(mob);
    default:
        return 0;
    }
}

bool
mobile_matches_num(struct creature *mob, struct mob_matcher *matcher)
{
    int64_t value = get_mob_value(mob, matcher->varient);

    switch (matcher->op) {
    case OP_EQ:
        return (value == matcher->num);
    case OP_LT:
        return (value < matcher->num);
    case OP_GT:
        return (value > matcher->num);
    }
    return false;
}

char *
mobile_num_info(struct creature *ch,
                struct creature *mob,
                struct mob_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11" PRId64 "%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       get_mob_value(mob, matcher->varient),
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_mobile_num_matcher(struct creature *ch,
                        struct mob_matcher *matcher,
                        enum matcher_varient varient,
                        char *expr)
{

    if (*expr == '\0') {
        send_to_char(ch, "You must specify a number for matching.\r\n");
        return false;
    }

    matcher->pred = mobile_matches_num;
    matcher->info = mobile_num_info;
    matcher->varient = varient;

    char *opstr = tmp_getword(&expr);

    if (!strcmp(opstr, "=") || !strcmp(opstr, "==")) {
        matcher->op = OP_EQ;
    } else if (!strcmp(opstr, "<")) {
        matcher->op = OP_LT;
    } else if (!strcmp(opstr, ">")) {
        matcher->op = OP_GT;
    } else if (!strcmp(opstr, "<=")) {
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
        send_to_char(ch, "You must specify a number to match the gold against.\r\n");
        return false;
    }
    matcher->num = atoi(expr);

    return true;
}

static struct {
    const char *matcher;
    mob_matcher_constructor constructor;
    enum matcher_varient varient;
} match_table[] = {
    { "name", make_mobile_name_matcher, MATCHER_STD },
    { "class", make_mobile_class_matcher, MATCHER_STD },
    { "race", make_mobile_race_matcher, MATCHER_STD },
    { "flags", make_mobile_flags_matcher, MATCHER_STD },
    { "affect", make_mobile_affect_matcher, MATCHER_STD },
    { "special", make_mobile_special_matcher, MATCHER_STD },
    { "gold", make_mobile_num_matcher, MATCHER_GOLD },
    { "cash", make_mobile_num_matcher, MATCHER_CASH },
    { "hitroll", make_mobile_num_matcher, MATCHER_HITROLL },
    { "damroll", make_mobile_num_matcher, MATCHER_DAMROLL },
    { "align", make_mobile_num_matcher, MATCHER_ALIGNMENT },
    { "exp", make_mobile_num_matcher, MATCHER_EXP },
    { NULL, NULL, MATCHER_STD }
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
        if (!*term) {
            term = tmp_getword(&expr);
        }
    }

    for (int i = 0; match_table[i].matcher && !found; i++) {
        if (!strcasecmp(term, match_table[i].matcher)) {
            if (!match_table[i].constructor(ch, &new_matcher, match_table[i].varient, expr)) {
                return NULL;
            }
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
        for (int i = 0; match_table[i].matcher; i++) {
            send_to_char(ch, "%s\r\n", match_table[i].matcher);
        }
        return;
    }

    exprv = g_strsplit(tmp_strcat(value, " ", argument, NULL), ",", 0);
    for (gchar **cur_expr = exprv; *cur_expr; cur_expr++) {
        char *expr = *cur_expr;
        // all expressions are either of the format [!] <field> <op> <value>
        // or [!] <field> <value>, separated by commas
        struct mob_matcher *new_matcher = make_mobile_matcher(ch, expr);
        if (new_matcher) {
            matchers = g_list_prepend(matchers, new_matcher);
        } else {
            goto cleanup;
        }
    }

    // Ensure only one of each kind of info function
    for (GList *needle = matchers; needle; needle = needle->next) {
        if (((struct mob_matcher *)(needle->data))->info == NULL) {
            continue;
        }
        for (GList *cur = needle->next; cur; cur = cur->next) {
            if (((struct mob_matcher *)(cur->data))->info == ((struct mob_matcher *)(needle->data))->info) {
                ((struct mob_matcher *)(cur->data))->info = NULL;
            }
        }
    }

    GHashTableIter iter;
    struct creature *mob;
    int vnum, found;

    acc_string_clear();
    found = 0;

    g_hash_table_iter_init(&iter, mob_prototypes);
    while (g_hash_table_iter_next(&iter, (gpointer)&vnum, (gpointer)&mob)) {
        bool matches = false;
        for (GList *cur_matcher = matchers; cur_matcher; cur_matcher = cur_matcher->next) {
            struct mob_matcher *matcher = cur_matcher->data;
            matches = matcher->pred(mob, matcher);
            if (matcher->negated) {
                matches = !matches;
            }
            if (!matches) {
                break;
            }
        }
        if (matches) {
            acc_sprintf("%3d. %s[%s%5d%s]%s %-50s%s", ++found,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                        mob->mob_specials.shared->vnum,
                        CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                        GET_NAME(mob), CCNRM(ch, C_NRM));
            for (GList *cur_matcher = matchers; cur_matcher; cur_matcher = cur_matcher->next) {
                struct mob_matcher *matcher = cur_matcher->data;
                if (matcher->info) {
                    acc_strcat(" ", matcher->info(ch, mob, matcher), NULL);
                }
            }
            acc_strcat("\r\n", NULL);
        }
    }

    if (found) {
        page_string(ch->desc, acc_get_string());
    } else {
        send_to_char(ch, "No matching mobiles found.\r\n");
    }

cleanup:
    g_strfreev(exprv);
    g_list_foreach(matchers, (GFunc)free, NULL);
    g_list_free(matchers);
}
