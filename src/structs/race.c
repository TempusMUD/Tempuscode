#include <stdint.h>
#include <glib.h>

#include "creature.h"
#include "utils.h"
#include "race.h"

GHashTable *races = NULL;

struct race *
make_race(void)
{
    struct race *race;

    CREATE(race, struct race, 1);
    race->age_adjust = 13;
    race->lifespan = 100;

    return race;
}

void
free_race(struct race *race)
{
    free(race->name);
    free(race);
}

struct race *
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
        }
        else if (xmlMatches(child->name, "age")) {
            race->age_adjust = xmlGetIntProp(child, "adjust", 13);
            race->lifespan = xmlGetIntProp(child, "lifespan", 100);
        }
    }

    return race;
}

struct race *
race_by_idnum(int idnum)
{
    return g_hash_table_lookup(races, GINT_TO_POINTER(idnum));
}

gboolean
race_name_matches(gpointer key, struct race *race, const char *name)
{
    return !strcasecmp(name, race->name);
}

gboolean
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
