/*************************************************************************
 *  File: language.cc                                                    *
 *  Usage: Source file for multi-language support                        *
 *                                                                       *
 *  All rights reserved.  See license.doc for complete information.      *
 *                                                                       *
 *********************************************************************** */

//
// File: tongue.cc                           -- Part of TempusMUD
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libxml/parser.h>
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
#include "screen.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "xml_utils.h"
#include "language.h"
#include "strutil.h"

struct tongue *
make_tongue(void)
{
    struct tongue *tongue;

    CREATE(tongue, struct tongue, 1);
    for (int c = 0; c < 256; c++)
        tongue->letters[c] = c;

    return tongue;
}

void
free_tongue(struct tongue *tongue)
{
    free(tongue->name);
    free(tongue->nospeak_msg);
    free(tongue->syllables);
    free(tongue);
}

struct tongue *
load_tongue(xmlNodePtr node)
{
    struct tongue *tongue;
    xmlNodePtr child;

    tongue = make_tongue();

    tongue->idnum = xmlGetIntProp(node, "idnum", 0);
    tongue->name = (char *)xmlGetProp(node, (xmlChar *) "name");

    for (child = node->children; child; child = child->next)
        if (xmlMatches(child->name, "syllable"))
            tongue->syllable_count++;

    if (tongue->syllable_count) {
        CREATE(tongue->syllables, struct trans_pair, tongue->syllable_count);
    }

    int syllable_idx = 0;
    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "syllable")) {
            char *s;
            s = (char *)xmlGetProp(child, (xmlChar *) "pattern");
            strcpy_s(tongue->syllables[syllable_idx].pattern, sizeof(tongue->syllables[syllable_idx].pattern), s);
            free(s);
            s = (char *)xmlGetProp(child, (xmlChar *) "replacement");
            strcpy_s(tongue->syllables[syllable_idx].replacement, sizeof(tongue->syllables[syllable_idx].replacement), s);
            free(s);
            syllable_idx++;
        } else if (xmlMatches(child->name, "letter")) {
            char *pattern = (char *)xmlGetProp(child, (xmlChar *) "pattern");
            char *replace =
                (char *)xmlGetProp(child, (xmlChar *) "replacement");

            tongue->letters[tolower(*pattern)] = tolower(*replace);
            tongue->letters[toupper(*pattern)] = toupper(*replace);
            free(pattern);
            free(replace);
        } else if (xmlMatches(child->name, "nospeak")) {
            tongue->nospeak_msg = (char *)xmlNodeGetContent(child);
        }
    }

    return tongue;
}

char *
translate_word(struct tongue *tongue, char *word)
{
    char *arg;
    bool found = false;

    arg = tmp_strdup(word);

    for (int x = 0; x < tongue->syllable_count; x++) {
        found = false;
        if (!strcmp(arg, tongue->syllables[x].pattern)) {
            found = true;
            arg = tmp_strdup(tongue->syllables[x].replacement);
            break;
        }
    }

    if (!found) {
        int length = strlen(word);
        for (int x = 0; x < length; x++)
            arg[x] = tongue->letters[(int)arg[x]];
        arg[length] = 0;
    }

    return arg;
}

char *
translate_with_tongue(struct tongue *tongue, const char *phrase, int amount)
{
    char *arg = NULL, *outbuf = NULL;

    if (!strcmp(phrase, "") || !tongue->syllables || amount == 100)
        return tmp_strdup(phrase);

    while (*phrase) {
        arg = tmp_gettoken_const(&phrase);

        if (number(1, 100) > amount)
            arg = translate_word(tongue, arg);

        if (outbuf)
            outbuf = tmp_strcat(outbuf, " ", arg, NULL);
        else
            outbuf = tmp_strcat(arg, NULL);
    }

    return outbuf;
}

GHashTable *tongues;

void
boot_tongues(const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    tongues = g_hash_table_new(g_direct_hash, g_direct_equal);

    doc = xmlParseFile(path);
    if (!doc) {
        errlog("Couldn't load %s", path);
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("etc/tongues.xml is empty");
        return;
    }

    if (!xmlMatches(node->name, "tongues")) {
        xmlFreeDoc(doc);
        errlog("/etc/tongues.xml root node is not tongues!");
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "tongue")) {
            int idnum = xmlGetIntProp(node, "idnum", -1);
            struct tongue *lang;

            lang = load_tongue(node);
            if (lang)
                g_hash_table_insert(tongues, GINT_TO_POINTER(idnum), lang);
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%d tongues loaded", g_hash_table_size(tongues));
}

int
find_tongue_idx_by_name(const char *tongue_name)
{
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        int vnum = GPOINTER_TO_INT(key);
        struct tongue *tongue = val;

        if (is_abbrev(tongue_name, tongue->name))
            return vnum;
    }
    return TONGUE_NONE;
}

struct tongue *
find_tongue_by_name(const char *tongue_name)
{
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct tongue *tongue = val;

        if (is_abbrev(tongue_name, tongue->name))
            return tongue;
    }
    return NULL;
}

struct tongue *
find_tongue_by_idnum(int tongue_id)
{
    return g_hash_table_lookup(tongues, GINT_TO_POINTER(tongue_id));
}

int
racial_tongue(int race_idx)
{
    struct race *race = race_by_idnum(race_idx);

    if (race)
        return race->tongue;

    return TONGUE_NONE;
}

ACMD(do_speak_tongue)
{
    struct tongue *tongue;
    char *tongue_name;

    if (!*argument) {
        tongue = g_hash_table_lookup(tongues,
                                     GINT_TO_POINTER((intptr_t)GET_TONGUE(ch)));
        send_to_char(ch, "You are currently speaking %s.\r\n", tongue->name);
        return;
    }

    tongue_name = tmp_getword(&argument);
    if (isname(tongue_name, "common")) {
        GET_TONGUE(ch) = TONGUE_COMMON;
        send_to_char(ch, "Ok, you're now speaking common.\r\n");
        return;
    }
    // Find the tongue they want
    tongue = find_tongue_by_name(tongue_name);
    if (!tongue) {
        send_to_char(ch, "That's not a tongue!\r\n");
        return;
    }

    if (tongue->nospeak_msg) {
        send_to_char(ch, "%s\r\n", tongue->nospeak_msg);
        return;
    }

    if (CHECK_TONGUE(ch, tongue->idnum) > 75) {
        GET_TONGUE(ch) = tongue->idnum;
        send_to_char(ch, "Ok, you're now speaking %s.\r\n", tongue->name);
    } else if (CHECK_TONGUE(ch, tongue->idnum) > 50) {
        GET_TONGUE(ch) = tongue->idnum;
        send_to_char(ch, "Ok, you're now trying to speak %s.\r\n",
            tongue->name);
    } else if (CHECK_TONGUE(ch, tongue->idnum) > 25) {
        GET_TONGUE(ch) = tongue->idnum;
        send_to_char(ch, "Ok, you're now trying to speak %s (badly).\r\n",
            tongue->name);
    } else
        send_to_char(ch,
            "You don't know that tongue well enough to speak it.\r\n");
}

const char *
fluency_desc(struct creature *ch, int tongue_idx)
{
    int fluency = CHECK_TONGUE(ch, tongue_idx);

    if (fluency < 0)
        return "(terrible)";
    if (fluency == 0)
        return "(not learned)";
    if (fluency <= 10)
        return "(awful)";
    if (fluency <= 20)
        return "(bad)";
    if (fluency <= 40)
        return "(poor)";
    if (fluency <= 55)
        return "(average)";
    if (fluency <= 70)
        return "(average)";
    if (fluency <= 80)
        return "(good)";
    if (fluency <= 90)
        return "(very good)";

    return "(fluent)";
}

ACMD(do_show_languages)
{
    struct tongue *tongue;

    tongue = g_hash_table_lookup(tongues, GINT_TO_POINTER((intptr_t)GET_TONGUE(ch)));

    acc_string_clear();
    acc_sprintf("%sYou are currently speaking:  %s%s\r\n\r\n",
        CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), tongue->name);

    acc_sprintf("%s%sYou know of the following languages:%s\r\n",
        CCYEL(ch, C_CMP), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR));

    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        int vnum = GPOINTER_TO_INT(key);
        struct tongue *tongue = val;
        if (CHECK_TONGUE(ch, vnum)) {
            if (IS_IMMORT(ch)) {
                acc_sprintf("%s%3d. %-30s %s%-17s%s%s\r\n",
                    CCCYN(ch, C_NRM),
                    vnum,
                    tongue->name,
                    CCBLD(ch, C_SPR),
                    fluency_desc(ch, vnum),
                    tmp_sprintf("%s[%3d]%s", CCYEL(ch, C_NRM),
                        CHECK_TONGUE(ch, vnum),
                        CCNRM(ch, C_NRM)), CCNRM(ch, C_SPR));
            } else {
                acc_sprintf("%s%-30s %s%s%s\r\n",
                    CCCYN(ch, C_NRM), tongue->name, CCBLD(ch, C_SPR),
                    fluency_desc(ch, vnum), CCNRM(ch, C_SPR));
            }
        }
    }

    page_string(ch->desc, acc_get_string());
}

void
set_initial_tongue(struct creature *ch)
{
    // Only set initial tongues if this is the first time their
    // language is being set.  If they change race, they shouldn't
    // automatically get the language
    if (CHECK_TONGUE(ch, TONGUE_COMMON) != 100) {
        int tongue_idx = racial_tongue(GET_RACE(ch));

        // Everyone knows common
        SET_TONGUE(ch, TONGUE_COMMON, 100);
        if (tongue_idx != TONGUE_NONE)
            SET_TONGUE(ch, tongue_idx, 100);
        if (tongue_idx == TONGUE_NONE) {
            GET_TONGUE(ch) = TONGUE_COMMON;
        } else {
            // Speakers know their native language
            GET_TONGUE(ch) = tongue_idx;
        }
    }
}

const char *
tongue_name(int tongue_idx)
{
    struct tongue *tongue;

    tongue = g_hash_table_lookup(tongues, GINT_TO_POINTER(tongue_idx));
    return (tongue) ? tongue->name : NULL;
}

char *
make_tongue_str(struct creature *ch, struct creature *to)
{
    int lang = GET_TONGUE(ch);

    if (lang == TONGUE_COMMON || !CHECK_TONGUE(to, lang))
        return tmp_strdup("");
    if (CHECK_TONGUE(ch, lang) < 50 && CHECK_TONGUE(to, lang) > 50)
        return tmp_sprintf(" in broken %s", tongue_name(lang));
    return tmp_sprintf(" in %s", tongue_name(lang));
}

char *
translate_tongue(struct creature *speaker, struct creature *listener,
    const char *message)
{
    int lang = GET_TONGUE(speaker);

    // Immortals understand all
    if (IS_IMMORT(listener))
        return tmp_strdup(message);

    // Perfect fluency for both speaker and listener
    if (CHECK_TONGUE(speaker, lang) == 100 &&
        CHECK_TONGUE(listener, lang) == 100)
        return tmp_strdup(message);

    // Listener might gain in fluency by listening to a sufficiently
    // skilled speaker
    if (CHECK_TONGUE(speaker, lang) > 75 &&
        CHECK_TONGUE(listener, lang) < 100 && speaker != listener) {
        GList *result =
            g_list_find(GET_LANG_HEARD(listener), GINT_TO_POINTER(lang));

        if (!result)
            GET_LANG_HEARD(listener) = g_list_prepend(GET_LANG_HEARD(listener),
                GINT_TO_POINTER(lang));
    }

    struct tongue *tongue;

    tongue = g_hash_table_lookup(tongues, GINT_TO_POINTER(lang));
    return translate_with_tongue(tongue,
        message,
        MIN(CHECK_TONGUE(speaker, lang), CHECK_TONGUE(listener, lang)));
}

void
write_tongue_xml(struct creature *ch, FILE * ouf)
{
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        int vnum = GPOINTER_TO_INT(key);
        struct tongue *tongue = val;
        if (CHECK_TONGUE(ch, vnum))
            fprintf(ouf, "<tongue name=\"%s\" level=\"%d\"/>\n",
                tongue->name, CHECK_TONGUE(ch, vnum));
    }
}

void
show_language_help(struct creature *ch)
{
    GHashTableIter iter;
    gpointer key, val;

    acc_string_clear();

    acc_sprintf("LANGUAGES:\r\n");

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct tongue *tongue = val;

        acc_sprintf("%2d         %s%-10s%s",
            tongue->idnum, CCCYN(ch, C_NRM), tongue->name, CCNRM(ch, C_NRM));
    }
    page_string(ch->desc, acc_get_string());
}
