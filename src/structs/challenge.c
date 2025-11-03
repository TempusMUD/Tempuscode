#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "race.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "language.h"
#include "strutil.h"
#include "xmlc.h"

#include "challenge.h"

GHashTable *challenges = NULL;

static struct challenge *
make_challenge(void)
{
    struct challenge *challenge;

    CREATE(challenge, struct challenge, 1);

    return challenge;
}

void
free_challenge(struct challenge *challenge)
{
    free(challenge->label);
    free(challenge->name);
    free(challenge->desc);
    free(challenge);
}

static gboolean
challenge_label_matches(gpointer key, struct challenge *challenge, const char *label)
{
    return !strcasecmp(label, challenge->label);
}

static gboolean
challenge_label_abbrev(gpointer key, struct challenge *challenge, const char *label)
{
    return is_abbrev(label, challenge->label);
}

struct challenge *
challenge_by_label(const char *label, bool exact)
{
    // This is stupid, but g_hash_table_find doesn't take a const
    // user_data even if we pinky-swear that we won't modify it.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-qual"

    if (exact) {
        return g_hash_table_find(challenges, (GHRFunc)challenge_label_matches, (gpointer)label);
    } else {
        return g_hash_table_find(challenges, (GHRFunc)challenge_label_abbrev, (gpointer)label);
    }
    #pragma GCC diagnostic pop
}

static struct challenge *
load_challenge(xmlNodePtr node)
{
    struct challenge *challenge = make_challenge();
    xmlNodePtr child;

    challenge->idnum = xmlGetIntProp(node, "id", 0);
    challenge->label = xmlGetStrProp(node, "label", NULL);
    challenge->name = xmlGetStrProp(node, "name", NULL);
    challenge->stages = xmlGetIntProp(node, "stages", 1);

    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "description")) {
            char *txt = (char *)xmlNodeGetContent(child);
            challenge->desc = strdup(tmp_gsub(txt, "\n", "\r\n"));
            xmlFree(txt);
        } else if (xmlMatches(child->name, "secret")) {
            challenge->secret = true;
        }
    }

    return challenge;
}

void
boot_challenges(const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    if (challenges) {
        g_hash_table_remove_all(challenges);
    } else {
        challenges = g_hash_table_new(g_direct_hash, g_direct_equal);
    }

    doc = xmlParseFile(path);
    if (!doc) {
        errlog("Couldn't load %s", path);
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("%s is empty", path);
        return;
    }

    if (!xmlMatches(node->name, "challenges")) {
        xmlFreeDoc(doc);
        errlog("%s root node is not challenges!", path);
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "challenge")) {
            char *label = xmlGetStrProp(node, "label", NULL);
            struct challenge *challenge;
            if (!label) {
                errlog("Label not set in challenge!");
                exit(-1);
            }

            challenge = load_challenge(node);
            if (challenge) {
                g_hash_table_insert(challenges,
                                    GINT_TO_POINTER(challenge->idnum),
                                    challenge);
            }
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%d challenges loaded", g_hash_table_size(challenges));
}

static struct xmlc_node *
xml_collect_challenges(GHashTable *challenges)
{
    if (g_hash_table_size(challenges) == 0) {
        return xml_null_node();
    }
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    GHashTableIter iter;
    gpointer key;
    struct challenge *val;

    g_hash_table_iter_init(&iter, challenges);
    while (g_hash_table_iter_next(&iter, &key, (gpointer)&val)) {
        struct xmlc_node *new_node = xml_node(
            "challenge",
            xml_int_attr("id", GPOINTER_TO_INT(key)),
            xml_str_attr("label", val->label),
            xml_str_attr("name", val->name),
            xml_int_attr("stages", val->stages),
            xml_if(val->desc && *val->desc,
                   xml_node("description",
                            xml_text(val->desc),
                            NULL)),
            xml_if(val->secret, xml_node("secret", NULL)),
            NULL);
        if (last_node) {
            last_node->next = new_node;
            last_node = last_node->next;
        } else {
            node_list = last_node = new_node;
        }
    }
    return node_list;
}

void
save_challenges(const char *path)
{
    xml_output(
        path,
        xml_node("challenges",
                 xml_splice(xml_collect_challenges(challenges)),
                 NULL));

}
