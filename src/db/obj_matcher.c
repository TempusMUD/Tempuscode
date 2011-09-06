/* ************************************************************************
*   File: obj_matcher.cc                            NOT Part of CircleMUD *
*  Usage: Matcher objects intended to be used to match struct obj_data           *
*         structures in the virtual object list                           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: obj_matcher.cc                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
// Copyright 2002 by John Rothe, all rights reserved.
//
#ifdef HAS_CONFIG_H
#endif

#define _GNU_SOURCE

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "obj_data.h"
#include "strutil.h"
#include "comm.h"
#include "accstr.h"
#include "account.h"
#include "materials.h"
#include "specs.h"
#include "spells.h"

extern GHashTable *obj_prototypes;

struct obj_matcher;
typedef bool (*obj_matcher_func)(struct obj_data *obj,
                                 struct obj_matcher *matcher);
typedef char *(*obj_info_func)(struct creature *ch,
                               struct obj_data *obj,
                               struct obj_matcher *matcher);
typedef bool (*obj_matcher_constructor)(struct creature *ch,
                                        struct obj_matcher *matcher,
                                        char *expr);

enum operator {
    OP_EQ,
    OP_LT,
    OP_GT
};

struct obj_matcher {
    obj_matcher_func pred;
    obj_info_func info;
    char *str;
    int8_t idx;
    int num;
    bool negated;
    enum operator op;
};

bool
object_matches_name(struct obj_data *obj, struct obj_matcher *matcher)
{
    return strcasestr(obj->name, matcher->str);
}

bool
make_object_name_matcher(struct creature *ch,
                              struct obj_matcher *matcher,
                              char *expr)
{
    matcher->pred = object_matches_name;
    if (*expr == '\0') {
        send_to_char(ch, "name matcher requires a name to match against\r\n");
        return false;
    }
    matcher->str = expr;
    return true;
}

bool
object_matches_type(struct obj_data *obj, struct obj_matcher *matcher)
{
    return GET_OBJ_TYPE(obj) == matcher->num;
}

bool
make_object_type_matcher(struct creature *ch,
                         struct obj_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify an object type to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_type;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = search_block(expr, item_types, 0);
    if (matcher->num < 0 || matcher->num > NUM_ITEM_TYPES) {
        send_to_char(ch, "Type olc help otypes for a valid list of types.\r\n");
        return false;
    }
    return true;
}

bool
object_matches_material(struct obj_data *obj, struct obj_matcher *matcher)
{
    return GET_OBJ_MATERIAL(obj) == matcher->num;
}

bool
make_object_material_matcher(struct creature *ch,
                         struct obj_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify an object material to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_material;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = search_block(expr, material_names, 0);
    if (matcher->num < 0 || matcher->num > TOP_MATERIAL) {
        send_to_char(ch, "Type olc help materials for a valid list of materials.\r\n");
        return false;
    }
    return true;
}

bool
object_matches_apply(struct obj_data *obj, struct obj_matcher *matcher)
{
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == matcher->num)
            return true;
    }
    return false;
}

char *
object_apply_info(struct creature *ch,
                  struct obj_data *obj,
                  struct obj_matcher *matcher)
{
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == matcher->num) {
            return tmp_sprintf("[%3d]", obj->affected[i].modifier);
        }
    }
    return "";
}

bool
make_object_apply_matcher(struct creature *ch,
                         struct obj_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify an object apply to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_apply;
    matcher->info = object_apply_info;

    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = search_block(expr, apply_types, 0);
    if (matcher->num < 0 || matcher->num > TOP_MATERIAL) {
        send_to_char(ch, "Type olc help apply for a valid list.\r\n");
        return false;
    }
    return true;
}

bool
object_matches_special(struct obj_data *obj, struct obj_matcher *matcher)
{
    return obj->shared->func && obj->shared->func == spec_list[matcher->num].func;
}

bool
make_object_special_matcher(struct creature *ch,
                            struct obj_matcher *matcher,
                            char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify an object special to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_special;
    matcher->num = find_spec_index_arg(expr);
    if ( matcher->num < 0 ) {
        send_to_char(ch, "Type show specials for a valid list.\r\n");
        return false;
    }
    return true;
}

bool
object_matches_affect(struct obj_data *obj, struct obj_matcher *matcher)
{
    return IS_SET(obj->obj_flags.bitvector[matcher->idx - 1], matcher->num);
}

bool
make_object_affect_matcher(struct creature *ch,
                            struct obj_matcher *matcher,
                            char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify object affects for matching.\r\n");
        return false;
    }

    int index, affect;
    matcher->pred = object_matches_affect;
    if( (!(index = 1) || (affect = search_block(expr, affected_bits_desc,  0)) < 0)
        &&  (!(index = 2) || (affect = search_block(expr, affected2_bits_desc, 0)) < 0)
        &&  (!(index = 3) || (affect = search_block(expr, affected3_bits_desc, 0)) < 0)) {
        send_to_char(ch, "There is no affect '%s'.\r\n", expr);
        return false;
    }

    matcher->idx = index;
    matcher->num = 1 << affect;
    return true;
}

bool
object_matches_cost(struct obj_data *obj, struct obj_matcher *matcher)
{
    bool result = false;
    if (matcher->op == OP_EQ)
        result = (obj->shared->cost == matcher->num);
    if (matcher->op == OP_LT)
        result = (obj->shared->cost < matcher->num);
    if (matcher->op == OP_GT)
        result = (obj->shared->cost > matcher->num);
    return result;
}

char *
object_cost_info(struct creature *ch,
                 struct obj_data *obj,
                 struct obj_matcher *matcher)
{
    return tmp_sprintf("%s[%s%11d%s]%s",
                       CCYEL(ch,C_NRM), CCGRN(ch, C_NRM),
                       obj->shared->cost,
                       CCYEL(ch, C_NRM), CCNRM(ch,C_NRM));
}

bool
make_object_cost_matcher(struct creature *ch,
                            struct obj_matcher *matcher,
                            char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify object costs for matching.\r\n");
        return false;
    }

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
    } else if (isnumber(opstr)) {
        matcher->op = OP_EQ;
        expr = opstr;
    }

    matcher->pred = object_matches_cost;
    matcher->info = object_cost_info;
    if (!isnumber(expr)) {
        send_to_char(ch, "You must specify a number to match the cost against.\r\n");
        return false;
    }
    matcher->num = atoi(expr);
    return true;
}

bool
object_matches_spell(struct obj_data *obj, struct obj_matcher *matcher)
{
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:	// val 3
        case ITEM_STAFF:	// val 3
            return (GET_OBJ_VAL(obj, 3) == matcher->num);
        case ITEM_WEAPON:	// val 0
            return (GET_OBJ_VAL(obj, 0) == matcher->num);
        case ITEM_SCROLL:	// val 1,2,3
        case ITEM_POTION:	// val 1,2,3
        case ITEM_PILL:	// ""
            return (GET_OBJ_VAL(obj, 1) == matcher->num
                    || GET_OBJ_VAL(obj, 2) == matcher->num
                    || GET_OBJ_VAL(obj, 3) == matcher->num);
        case ITEM_FOOD:	// Val 2
            return (GET_OBJ_VAL(obj, 2) == matcher->num);
    }
    return false;
}

bool
make_object_spell_matcher(struct creature *ch,
                         struct obj_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify spells to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_spell;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = search_block(expr, (const char **)spells, 0);
    if (matcher->num < 0 || matcher->num > TOP_SPELL_DEFINE) {
        send_to_char(ch, "Type olc help spells for a list of spells.\r\n");
        return false;
    }
    return true;
}

bool
object_matches_worn(struct obj_data *obj, struct obj_matcher *matcher)
{
    return CAN_WEAR(obj, matcher->num);
}

bool
make_object_worn_matcher(struct creature *ch,
                         struct obj_matcher *matcher,
                         char *expr)
{
    if (*expr == '\0') {
        send_to_char(ch, "You must specify wear positions to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_worn;
    if (is_number(expr))
        matcher->num = atoi(expr);
    else
        matcher->num = search_block(expr, wear_bits, 0);
    if (matcher->num < 0 || matcher->num > NUM_WEAR_FLAGS) {
        send_to_char(ch, "Type olc help wear for a list of wear positions.\r\n");
        return false;
    }
    matcher->num = 1 << matcher->num;

    return true;
}

bool
object_matches_extra(struct obj_data *obj, struct obj_matcher *matcher)
{
    switch (matcher->idx) {
    case 1:
        return IS_SET(GET_OBJ_EXTRA(obj), matcher->num);
    case 2:
        return IS_SET(GET_OBJ_EXTRA2(obj), matcher->num);
    case 3:
        return IS_SET(GET_OBJ_EXTRA3(obj), matcher->num);
    }
    return false;
}

bool
make_object_extra_matcher(struct creature *ch,
                          struct obj_matcher *matcher,
                          char *expr)
{
    int index, flag;

    if (*expr == '\0') {
        send_to_char(ch, "You must specify extra flags to match.\r\n");
        return false;
    }

    matcher->pred = object_matches_extra;
    if( (!(index = 1) || (flag = search_block(expr, extra_names,  0)) < 0)
        &&  (!(index = 2) || (flag = search_block(expr, extra2_names, 0)) < 0)
        &&  (!(index = 3) || (flag = search_block(expr, extra3_names, 0)) < 0)) {
        send_to_char(ch, "There is no extra flag '%s'.\r\n", expr);
        return false;
    }

    matcher->idx = index;
    matcher->num = 1 << flag;
    return true;
}

struct {
    const char *matcher;
    obj_matcher_constructor constructor;
} match_table[] = {
    { "name", make_object_name_matcher },
    { "type", make_object_type_matcher },
    { "material", make_object_material_matcher },
    { "apply", make_object_apply_matcher },
    { "special", make_object_special_matcher },
    { "affect", make_object_affect_matcher },
    { "cost", make_object_cost_matcher },
    { "spell", make_object_spell_matcher },
    { "worn", make_object_worn_matcher },
    { "extra", make_object_extra_matcher },
    { NULL, NULL }
};

struct obj_matcher *
make_object_matcher(struct creature *ch, char *expr)
{
    struct obj_matcher new_matcher;
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

    struct obj_matcher *result;
    if (found) {
        CREATE(result, struct obj_matcher, 1);
        *result = new_matcher;
    } else {
        result = NULL;
    }
    return result;
}

void
do_show_objects(struct creature *ch, char *value, char *argument)
{
    gchar **exprv;
    GList *matchers = NULL;

    if (!*value) {
        send_to_char(ch,
                     "Usage: show object [!] <term>[, [!]<term> ...]\r\n"
                     "Precede the term with an exclamation point (!) to select\r\n"
                     "objects that don't match.\r\n"
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
        struct obj_matcher *new_matcher = make_object_matcher(ch, expr);
        if (new_matcher)
            matchers = g_list_prepend(matchers, new_matcher);
        else
            goto cleanup;
    }

    GHashTableIter iter;
    struct obj_data *obj;
    int vnum, found;

    acc_string_clear();
    found = 0;

    g_hash_table_iter_init(&iter, obj_prototypes);
    while (g_hash_table_iter_next(&iter, (gpointer)&vnum, (gpointer)&obj)) {
        bool matches = false;
        for (GList *cur_matcher = matchers;cur_matcher;cur_matcher = cur_matcher->next) {
            struct obj_matcher *matcher = cur_matcher->data;
            matches = matcher->pred(obj, matcher);
            if (matcher->negated)
                matches = !matches;
            if (!matches)
                break;
        }
        if (matches) {
            acc_sprintf("%3d. %s[%s%5d%s]%s %-50s%s", ++found,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                        obj->shared->vnum,
                        CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                        obj->name, CCNRM(ch, C_NRM));
            for (GList *cur_matcher = matchers;cur_matcher;cur_matcher = cur_matcher->next) {
                struct obj_matcher *matcher = cur_matcher->data;
                if (matcher->info)
                    acc_strcat(" ", matcher->info(ch, obj, matcher), NULL);
            }
            acc_strcat("\r\n", NULL);
        }
    }

    if (found)
        page_string(ch->desc, acc_get_string());
    else
        send_to_char(ch, "No matching objects found.\r\n");

cleanup:
    g_strfreev(exprv);
    g_list_foreach(matchers, (GFunc)free, NULL);
    g_list_free(matchers);
}
