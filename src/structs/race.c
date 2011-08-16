#include <stdint.h>
#include <glib.h>

#include "creature.h"
#include "utils.h"
#include "language.h"
#include "race.h"

GHashTable *races = NULL;

static struct race *
make_race(void)
{
    struct race *race;

    CREATE(race, struct race, 1);
    race->age_adjust = 13;
    race->lifespan = 100;
    race->tongue = TONGUE_NONE;

    return race;
}

static void
parse_racial_sex(struct race *race, xmlNodePtr node)
{
    int sex_id = xmlGetIntProp(node, "idnum", 0);

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "weight")) {
            race->weight_min[sex_id] = xmlGetIntProp(child, "weight_min", 130);
            race->weight_max[sex_id] = xmlGetIntProp(child, "weight_max", 180);
        } else if (xmlMatches(child->name, "height")) {
            race->height_min[sex_id] = xmlGetIntProp(child, "height_min", 140);
            race->height_max[sex_id] = xmlGetIntProp(child, "height_max", 190);
            race->weight_add[sex_id] = xmlGetIntProp(child, "weight_add", 8);
        }
    }
 }

static struct race *
load_race(xmlNodePtr node)
{
    struct race *race = make_race();
    xmlNodePtr child;

    race = make_race();

    race->idnum = xmlGetIntProp(node, "idnum", 0);
    race->name = (char *)xmlGetProp(node, (xmlChar *) "name");

    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "attributes")) {
            race->str_mod = xmlGetIntProp(child, "str", 0);
            race->int_mod = xmlGetIntProp(child, "int", 0);
            race->wis_mod = xmlGetIntProp(child, "wis", 0);
            race->dex_mod = xmlGetIntProp(child, "dex", 0);
            race->con_mod = xmlGetIntProp(child, "con", 0);
            race->cha_mod = xmlGetIntProp(child, "cha", 0);
        } else if (xmlMatches(child->name, "age")) {
            race->age_adjust = xmlGetIntProp(child, "adjust", 13);
            race->lifespan = xmlGetIntProp(child, "lifespan", 100);
        } else if (xmlMatches(child->name, "tongue")) {
            char *tongue_name = (char *)xmlGetProp(child, (xmlChar *)"name");
            if (tongue_name) {
                race->tongue = find_tongue_idx_by_name(tongue_name);
                free(tongue_name);
            }
        } else if (xmlMatches(child->name, "intrinsic")) {
            char *aff = (char *)xmlGetProp(child, (xmlChar *)"affect");
            if (aff) {
                int idx;
                if ((idx = search_block(aff, affected_bits_desc, true)) >= 0) {
                    race->aff1 |= (1 << idx);
                } else if ((idx = search_block(aff, affected2_bits_desc, true)) >= 0) {
                    race->aff2 |= (1 << idx);
                } else if ((idx = search_block(aff, affected3_bits_desc, true)) >= 0) {
                    race->aff3 |= (1 << idx);
                }
                free(aff);
            }
        } else if (xmlMatches(child->name, "sex")) {
            parse_racial_sex(race, child);
        }
    }

    return race;
}

struct race *
race_by_idnum(int idnum)
{
    return g_hash_table_lookup(races, GINT_TO_POINTER(idnum));
}

static gboolean
race_name_matches(gpointer key, struct race *race, const char *name)
{
    return !strcasecmp(name, race->name);
}

static gboolean
race_name_abbrev(gpointer key, struct race *race, const char *name)
{
    return is_abbrev(name, race->name);
}

struct race *
race_by_name(char *name, bool exact)
{
    if (exact)
        return g_hash_table_find(races, (GHRFunc)race_name_matches, name);
    else
        return g_hash_table_find(races, (GHRFunc)race_name_abbrev, name);
}

const char *
race_name_by_idnum(int idnum)
{
    struct race *race = race_by_idnum(idnum);
    if (race)
        return race->name;
    return "ILLEGAL";
}

void
boot_races(const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    races = g_hash_table_new(g_direct_hash, g_direct_equal);

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

    if (!xmlMatches(node->name, "races")) {
        xmlFreeDoc(doc);
        errlog("%s root node is not races!", path);
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "race")) {
            int idnum = xmlGetIntProp(node, "idnum", -1);
            struct race *race;

            race = load_race(node);
            if (race)
                g_hash_table_insert(races, GINT_TO_POINTER(idnum), race);
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%d races loaded", g_hash_table_size(races));
}
