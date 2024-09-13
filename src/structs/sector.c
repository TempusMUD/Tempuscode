#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "sector.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "language.h"
#include "strutil.h"

GHashTable *sectors = NULL;

static struct sector *
make_sector(void)
{
    struct sector *sector;

    CREATE(sector, struct sector, 1);

    return sector;
}

static struct sector *
load_sector(xmlNodePtr node)
{
    struct sector *sector = make_sector();
    xmlNodePtr child;
    char *txt;

    sector->idnum = xmlGetIntProp(node, "idnum", 0);
    sector->name = (char *)xmlGetProp(node, (xmlChar *) "name");
    sector->moveloss = xmlGetIntProp(node, "moveloss", 0);

    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "groundless")) {
            sector->groundless = true;
        } else if (xmlMatches(child->name, "airless")) {
            sector->airless = true;
        } else if (xmlMatches(child->name, "watery")) {
            sector->watery = true;
        } else if (xmlMatches(child->name, "opensky")) {
            sector->opensky = true;
        } else if (xmlMatches(child->name, "descriptions")) {
            for (xmlNodePtr descNode = child->children; descNode; descNode = descNode->next) {
                if (xmlMatches(descNode->name, "up")) {
                    txt = (char *)xmlNodeGetContent(descNode);
                    sector->up_desc = strdup(txt);
                    xmlFree(txt);
                } else if (xmlMatches(descNode->name, "side")) {
                    txt = (char *)xmlNodeGetContent(descNode);
                    sector->side_desc = strdup(txt);
                    xmlFree(txt);
                } else if (xmlMatches(descNode->name, "down")) {
                    txt = (char *)xmlNodeGetContent(descNode);
                    sector->down_desc = strdup(txt);
                    xmlFree(txt);
                }
            }
        }
    }

    return sector;
}

struct sector *
sector_by_idnum(int idnum)
{
    return g_hash_table_lookup(sectors, GINT_TO_POINTER(idnum));
}

static gboolean
sector_name_matches(gpointer key, struct sector *sector, const char *name)
{
    return !strcasecmp(name, sector->name);
}

static gboolean
sector_name_abbrev(gpointer key, struct sector *sector, const char *name)
{
    return is_abbrev(name, sector->name);
}

struct sector *
sector_by_name(char *name, bool exact)
{
    if (exact) {
        return g_hash_table_find(sectors, (GHRFunc)sector_name_matches, name);
    } else {
        return g_hash_table_find(sectors, (GHRFunc)sector_name_abbrev, name);
    }
}

const char *
sector_name_by_idnum(int idnum)
{
    struct sector *sector = sector_by_idnum(idnum);
    if (sector) {
        return sector->name;
    }
    return tmp_sprintf("ILLEGAL(%d)", idnum);
}

void
boot_sectors(const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    sectors = g_hash_table_new(g_direct_hash, g_direct_equal);

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

    if (!xmlMatches(node->name, "sectors")) {
        xmlFreeDoc(doc);
        errlog("%s root node is not sectors!", path);
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "sector")) {
            int idnum = xmlGetIntProp(node, "idnum", -1);
            struct sector *sector;

            sector = load_sector(node);
            if (sector) {
                g_hash_table_insert(sectors, GINT_TO_POINTER(idnum), sector);
            }
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%d sectors loaded", g_hash_table_size(sectors));
}
