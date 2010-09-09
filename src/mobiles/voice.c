#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <time.h>
#include <tmpstr.h>
#include <ctype.h>
#include <stdlib.h>
#include "interpreter.h"
#include "constants.h"
#include "utils.h"
#include "creature.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"
#include "voice.h"
#include "accstr.h"

// Note: These must be in the same order as the voice_situation enum
// in voice.h
static const char *voice_emit_categories[] = {
    "taunt",
    "attack",
    "panic",
    "hunt_found",
    "hunt_lost",
    "hunt_gone",
    "hunt_taunt",
    "hunt_unseen",
    "hunt_openair",
    "hunt_water",
    "fight_winning",
    "fight_losing",
    "fight_helping",
    "obeying",
    "\n"
};

GHashTable *voices = NULL;

void
free_voice(struct voice *voice)
{
    GList *emits;
    GHashTableIter it;
    int key;

    free(voice->name);

    g_hash_table_iter_init(&it, voice->emits);
    while (g_hash_table_iter_next(&it, (gpointer)&key, (gpointer)&emits)) {
        g_list_foreach(emits, (GFunc)free, 0);
        g_list_free(emits);
    }
    
    g_hash_table_destroy(voice->emits);
    free(voice);
}

struct voice *
load_voice(xmlNodePtr node)
{
    struct voice *voice;
    xmlNodePtr child;

    CREATE(voice, struct voice, 1);
    voice->idnum = xmlGetIntProp(node, "idnum", 0);
    voice->name = (char *)xmlGetProp(node, (xmlChar *)"name");
    voice->emits = g_hash_table_new(g_int_hash, g_int_equal);
    for (child = node->children;child;child = child->next) {
        if (xmlMatches(child->name, "text"))
            continue;
        int emit_idx = search_block((const char *)child->name,
                                    voice_emit_categories,
                                    true);
        if (emit_idx < 0) {
            errlog("Invalid node %s in voices.xml", child->name);
            free_voice(voice);
            return NULL;
        }
        char *str = (char *)xmlNodeGetContent(child);
        GList *new_emits;
        new_emits = g_list_prepend(
            g_hash_table_lookup(voice->emits, GINT_TO_POINTER(emit_idx)),
            strdup(tmp_trim(str)));

        g_hash_table_insert(voice->emits, GINT_TO_POINTER(emit_idx), new_emits);
        free(str);
    }
    return voice;
}

void
voice_perform(struct voice *voice,
              struct creature *ch,
              void *vict,
              enum voice_situation situation)
{
    GList *emits = g_hash_table_lookup(voice->emits,
                                       GINT_TO_POINTER(situation));
    if (!emits)
        return;

    int cmd_idx = number(0, g_list_length(emits) - 1);
    char buf[MAX_STRING_LENGTH];

    make_act_str(g_list_nth_data(emits, cmd_idx),
                 buf,
                 ch, NULL, vict, ch);
    command_interpreter(ch, tmp_trim(buf));
}

void
emit_voice(struct creature *ch, void *vict, enum voice_situation situation)
{
    if (!IS_NPC(ch))
        return;

    int voice_idx = GET_VOICE(ch);

    if (!voice_idx)
        voice_idx = IS_ANIMAL(ch) ? VOICE_ANIMAL:VOICE_MOBILE;

    struct voice *voice = g_hash_table_lookup(voices,
                                              GINT_TO_POINTER(voice_idx));
    if (voice)
        voice_perform(voice, ch, vict, situation);
}

void
boot_voices(void)
{
	xmlDocPtr doc;
	xmlNodePtr node;

    voices = g_hash_table_new(g_int_hash, g_int_equal);

    doc = xmlParseFile("etc/voices.xml");
    if (!doc) {
        errlog("Couldn't load etc/voices.xml");
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("etc/voices.xml is empty");
        return;
    }

    if (!xmlMatches(node->name, "voices")) {
        xmlFreeDoc(doc);
        errlog("/etc/voices.xml root node is not voices!");
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "voice")) {
            int idnum = xmlGetIntProp(node, "idnum", 0);
            struct voice *voice;

            voice = load_voice(node);
            if (voice)
                g_hash_table_insert(voices, GINT_TO_POINTER(idnum), voice);
            else
                safe_exit(1);
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%d voices loaded", g_hash_table_size(voices));
}

int
find_voice_idx_by_name(const char *voice_name)
{
    int result;
    struct voice *voice;

    if (is_number(voice_name)) {
        result = atoi(voice_name);
        if (g_hash_table_lookup(voices, GINT_TO_POINTER(result)))
            return result;
        return VOICE_NONE;
    }

    GHashTableIter it;

    g_hash_table_iter_init(&it, voices);
    while (g_hash_table_iter_next(&it, (gpointer)&result, (gpointer)&voice)) {
        if (is_abbrev(voice_name, voice->name))
            return result;
    }

	return VOICE_NONE;
}

const char *
voice_name(int voice_idx)
{
    if (voice_idx == VOICE_NONE)
        return "<none>";
    struct voice *voice = g_hash_table_lookup(voices,
                                              GINT_TO_POINTER(voice_idx));
    if (!voice)
        return tmp_sprintf("<#%d>", voice_idx);
    return voice->name;
}

void
show_voices(struct creature *ch)
{
    acc_string_clear();

    acc_sprintf("VOICES:\r\n");

    GHashTableIter it;
    struct voice *voice;
    int idnum;

    g_hash_table_iter_init(&it, voices);
    while (g_hash_table_iter_next(&it, (gpointer)&idnum, (gpointer)&voice)) {
        acc_sprintf("%2d         %s%-10s%s\r\n",
                    idnum,
                    CCCYN(ch, C_NRM),
                    voice->name,
                    CCNRM(ch, C_NRM));
    }
    page_string(ch->desc, acc_get_string());
}
