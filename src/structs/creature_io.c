#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "xmlc.h"
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
#include "zone_data.h"
#include "race.h"
#include "account.h"
#include "creature.h"
#include "db.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "actions.h"
#include "language.h"
#include "strutil.h"

void add_alias(struct creature *ch, struct alias_data *a);
void affect_to_char(struct creature *ch, struct affected_type *af);
void extract_object_list(struct obj_data *head);
bool save_player_objects(struct creature *ch);
struct creature *load_player_from_xml(int id);
void save_player_to_xml(struct creature *ch);

void
autocryo(bool backfill)
{
    const char *cond = "where age(entry_time) between '90 days' and '91 days'";
    if (backfill) {
        cond = "where age(entry_time) >= '90 days'";
    }
    PGresult *res = sql_query("select p.idnum from accounts a join players p on p.account=a.idnum %s", cond);
    long count = PQntuples(res);
    for (int idx = 0;idx < count;idx++) {
        int idnum = atol(PQgetvalue(res, idx, 0));
        struct creature *ch = load_player_from_xml(idnum);
        if (!ch) {
            slog("couldn't read player %d for autocryo", idnum);
            continue;
        }
        if (ch->player_specials->rentcode != RENT_CRYO) {
            slog("Auto-cryoing player %s(%d) for dormancy.", GET_NAME(ch), idnum);
            ch->player_specials->rentcode = RENT_CRYO;
            save_player_to_xml(ch);
        }
    }
}

struct obj_data *
findCostliestObj(struct creature *ch)
{
    struct obj_data *cur_obj, *result;

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        return NULL;
    }

    result = NULL;

    cur_obj = ch->carrying;
    while (cur_obj) {
        if (cur_obj &&
            (!result || GET_OBJ_COST(result) < GET_OBJ_COST(cur_obj))) {
            result = cur_obj;
        }

        if (cur_obj->contains) {
            cur_obj = cur_obj->contains;    // descend into obj
        } else if (!cur_obj->next_content && cur_obj->in_obj) {
            cur_obj = cur_obj->in_obj->next_content;    // ascend out of obj
        } else {
            cur_obj = cur_obj->next_content;    // go to next obj
        }
    }

    return result;
}

// struct creature_payRent will pay the player's rent, selling off items to meet the
// bill, if necessary.
int
pay_player_rent(struct creature *ch, time_t last_time, int code, int currency)
{
    float day_count;
    int factor;
    long cost;

    // Immortals don't pay rent
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        return 0;
    }
    if (is_authorized(ch, TESTER, NULL)) {
        return 0;
    }

    // Cryoed chars already paid their rent, quit chars don't have any rent
    if (code == RENT_NEW_CHAR || code == RENT_CRYO || code == RENT_QUIT) {
        return 0;
    }

    // Calculate total cost
    day_count = (float)(time(NULL) - last_time) / SECS_PER_REAL_DAY;
    if (code == RENT_FORCED) {
        factor = 3;
    } else {
        factor = 1;
    }
    cost = (int)(calc_daily_rent(ch, factor, NULL, NULL) * day_count);
    slog("Charging %ld for %.2f days of rent", cost, day_count);

    // First we get as much as we can out of their hand
    if (currency == TIME_ELECTRO) {
        if (cost < GET_CASH(ch) + GET_FUTURE_BANK(ch)) {
            withdraw_future_bank(ch->account, cost - GET_CASH(ch));
            GET_CASH(ch) = MAX(GET_CASH(ch) - cost, 0);
            cost = 0;
        } else {
            cost -= GET_CASH(ch) + GET_FUTURE_BANK(ch);
            account_set_future_bank(ch->account, 0);
            GET_CASH(ch) = 0;
        }
    } else {
        if (cost < GET_GOLD(ch) + GET_PAST_BANK(ch)) {
            withdraw_past_bank(ch->account, cost - GET_GOLD(ch));
            GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
            cost = 0;
        } else {
            cost -= GET_GOLD(ch) + GET_PAST_BANK(ch);
            account_set_past_bank(ch->account, 0);
            GET_GOLD(ch) = 0;
        }
    }

    // If they didn't have enough, try the cross-time money
    if (cost > 0) {
        if (currency == TIME_ELECTRO) {
            if (cost < GET_GOLD(ch) + GET_PAST_BANK(ch)) {
                withdraw_past_bank(ch->account, cost - GET_GOLD(ch));
                GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
                cost = 0;
            } else {
                cost -= GET_GOLD(ch) + GET_PAST_BANK(ch);
                account_set_past_bank(ch->account, 0);
                GET_GOLD(ch) = 0;
            }
        } else {
            if (cost < GET_CASH(ch) + GET_FUTURE_BANK(ch)) {
                withdraw_future_bank(ch->account, cost - GET_CASH(ch));
                GET_CASH(ch) = MAX(GET_CASH(ch) - cost, 0);
                cost = 0;
            } else {
                cost -= GET_CASH(ch) + GET_FUTURE_BANK(ch);
                account_set_future_bank(ch->account, 0);
                GET_CASH(ch) = 0;
            }
        }
    }

    // If they still don't have enough, put them in JAIL
    if (cost > 0) {
        return 3;
    }
    return 0;
}

bool
reportUnrentables(struct creature *ch, struct obj_data *obj_list,
                  const char *pos)
{
    bool same_obj(struct obj_data *obj1, struct obj_data *obj2);

    struct obj_data *cur_obj, *last_obj;
    bool result = false;

    last_obj = NULL;
    cur_obj = obj_list;
    while (cur_obj) {
        if (!last_obj || !same_obj(last_obj, cur_obj)) {
            if (obj_is_unrentable(cur_obj) || ((cur_obj->shared->owner_id) && cur_obj->shared->owner_id != GET_IDNUM(ch))) {
                act(tmp_sprintf("You cannot rent while %s $p!", pos),
                    true, ch, cur_obj, NULL, TO_CHAR);
                result = true;
            }
        }
        last_obj = cur_obj;

        pos = "carrying";
        if (cur_obj->contains) {
            cur_obj = cur_obj->contains;    // descend into obj
        } else if (!cur_obj->next_content && cur_obj->in_obj) {
            cur_obj = cur_obj->in_obj->next_content;    // ascend out of obj
        } else {
            cur_obj = cur_obj->next_content;    // go to next obj
        }
    }

    return result;
}

// Displays all unrentable items and returns true if any are found
bool
display_unrentables(struct creature *ch)
{
    struct obj_data *cur_obj;
    int pos;
    bool result = false;

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        return false;
    }

    for (pos = 0; pos < NUM_WEARS; pos++) {
        cur_obj = GET_EQ(ch, pos);
        if (cur_obj) {
            result = result || reportUnrentables(ch, cur_obj, "wearing");
        }
        cur_obj = GET_IMPLANT(ch, pos);
        if (cur_obj) {
            result = result
                     || reportUnrentables(ch, cur_obj, "implanted with");
        }
    }

    result = result || reportUnrentables(ch, ch->carrying, "carrying");

    return result;
}

bool
save_player_objects_to_file(struct creature *ch, const char *path)
{
    FILE *ouf;
    char *tmp_path;
    int idx;

    tmp_path = tmp_sprintf("%s.new", path);
    ouf = fopen(tmp_path, "w");

    if (!ouf) {
        fprintf(stderr,
                "Unable to open XML equipment file for save.[%s] (%s)\n", path,
                strerror(errno));
        return false;
    }
    fprintf(ouf, "<objects>\n");
    // Save the inventory
    for (struct obj_data *obj = ch->carrying; obj != NULL;
         obj = obj->next_content) {
        save_object_to_xml(obj, ouf);
    }
    // Save the equipment
    for (idx = 0; idx < NUM_WEARS; idx++) {
        if (GET_EQ(ch, idx)) {
            save_object_to_xml(GET_EQ(ch, idx), ouf);
        }
        if (GET_IMPLANT(ch, idx)) {
            save_object_to_xml(GET_IMPLANT(ch, idx), ouf);
        }
        if (GET_TATTOO(ch, idx)) {
            save_object_to_xml(GET_TATTOO(ch, idx), ouf);
        }
    }
    fprintf(ouf, "</objects>\n");
    fclose(ouf);

    // on success, move the new object file onto the old one
    rename(tmp_path, path);

    return true;
}

bool
save_player_objects(struct creature *ch)
{
    return save_player_objects_to_file(ch, get_equipment_file_path(GET_IDNUM(ch)));
}

/**
 * return values:
 * -1 - dangerous failure - don't allow char to enter
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple. (or noeq)
 *  2 - rented equipment lost ( no $ )
 **/
int
unrent(struct creature *ch)
{
    int err;

    err = load_player_objects(ch);
    if (err) {
        return 0;
    }

    return pay_player_rent(ch,
                           ch->player.time.logon,
                           ch->player_specials->rentcode, ch->player_specials->rent_currency);
}

bool
load_player_objects_from_file(struct creature *ch, const char *path)
{
    int axs = access(path, W_OK);

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml equipment file '%s': %s",
                   path, strerror(errno));
            return -1;
        } else {
            return 1;           // normal no eq file
        }
    }
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return -1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return 1;
    }

    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "object")) {
            (void)load_object_from_xml(NULL, ch, NULL, node);
        }
    }

    xmlFreeDoc(doc);

    return 0;
}

bool
load_player_objects(struct creature *ch)
{
    return load_player_objects_from_file(ch, get_equipment_file_path(GET_IDNUM(ch)));
}

bool
checkLoadCorpse(struct creature *ch)
{
    char *path = get_corpse_file_path(GET_IDNUM(ch));
    int axs = access(path, W_OK);
    struct stat file_stat;
    extern time_t boot_time;

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml corpse file '%s' : %s",
                   path, strerror(errno));
            return false;
        } else {
            return false;
        }
    }

    stat(path, &file_stat);

    if (file_stat.st_ctime < boot_time) {
        return true;
    }

    return false;
}

int
loadCorpse(struct creature *ch)
{

    char *path = get_corpse_file_path(GET_IDNUM(ch));
    int axs = access(path, W_OK);
    struct obj_data *corpse_obj;

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml corpse file '%s': %s",
                   path, strerror(errno));
            return -1;
        } else {
            return 1;           // normal no eq file
        }
    }
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return -1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return 1;
    }

    xmlNodePtr node = root->xmlChildrenNode;
    while (!xmlMatches(node->name, "object")) {
        node = node->next;
        if (node == NULL) {
            xmlFreeDoc(doc);
            errlog("First child in XML file (%s) not an object", path);
            return 1;
        }
    }

    corpse_obj = load_object_from_xml(NULL, ch, NULL, node);
    if (!corpse_obj) {
        xmlFreeDoc(doc);
        errlog("Could not create corpse object from file %s", path);
        return 1;
    }

    if (!IS_CORPSE(corpse_obj)) {
        xmlFreeDoc(doc);
        extract_obj(corpse_obj);
        errlog("First object in corpse file %s not a corpse", path);
        return 1;
    }

    xmlFreeDoc(doc);

    remove(path);
    return 0;
}

void
stash_creature_affects(struct creature *ch, struct aff_stash *stash)
{
    // Save vital statistics
    struct affected_type *cur_aff;
    int pos;

    // Remove all spell affects without deleting them
    stash->saved_affs = ch->affected;
    ch->affected = NULL;

    for (cur_aff = stash->saved_affs; cur_aff; cur_aff = cur_aff->next) {
        affect_modify(ch, cur_aff->location, cur_aff->modifier,
                      cur_aff->bitvector, cur_aff->aff_index, false);
    }

    for (pos = 0; pos < NUM_WEARS; pos++) {
        if (GET_EQ(ch, pos)) {
            stash->saved_eq[pos] = raw_unequip_char(ch, pos, EQUIP_WORN);
        }
        if (GET_IMPLANT(ch, pos)) {
            stash->saved_impl[pos] = raw_unequip_char(ch, pos, EQUIP_IMPLANT);
        }
        if (GET_TATTOO(ch, pos)) {
            stash->saved_tattoo[pos] = raw_unequip_char(ch, pos, EQUIP_TATTOO);
        }
    }
}

void
restore_creature_affects(struct creature *ch, struct aff_stash *aff_stash)
{
    struct affected_type *cur_aff;
    int pos;

    for (pos = 0; pos < NUM_WEARS; pos++) {
        if (aff_stash->saved_eq[pos]) {
            equip_char(ch, aff_stash->saved_eq[pos], pos, EQUIP_WORN);
        }
        if (aff_stash->saved_impl[pos]) {
            equip_char(ch, aff_stash->saved_impl[pos], pos, EQUIP_IMPLANT);
        }
        if (aff_stash->saved_tattoo[pos]) {
            equip_char(ch, aff_stash->saved_tattoo[pos], pos, EQUIP_TATTOO);
        }
    }

    for (cur_aff = aff_stash->saved_affs; cur_aff; cur_aff = cur_aff->next) {
        affect_modify(ch, cur_aff->location, cur_aff->modifier,
                      cur_aff->bitvector, cur_aff->aff_index, true);
    }
    ch->affected = aff_stash->saved_affs;
    affect_total(ch);
}


static struct xmlc_node *
xml_collect_weaponspecs(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    for (int i = 0;i < MAX_WEAPON_SPEC;i++) {
        if (GET_WEAP_SPEC(ch, i).level == 0) {
            continue;
        }

        struct xmlc_node *new_node = xml_node(
            "weaponspec",
            xml_int_attr("vnum", GET_WEAP_SPEC(ch, i).vnum),
            xml_int_attr("level", GET_WEAP_SPEC(ch, i).level),
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

static struct xmlc_node *
xml_collect_aliases(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    for (struct alias_data *cur_alias = ch->player_specials->aliases;cur_alias;cur_alias = cur_alias->next) {
        struct xmlc_node *new_node = xml_node(
            "alias",
            xml_int_attr("type", cur_alias->type),
            xml_str_attr("alias", cur_alias->alias),
            xml_str_attr("replace", cur_alias->replacement),
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

static struct xmlc_node *
xml_collect_skills(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    for (int idx = 0;idx < MAX_SKILLS;idx++) {
        if (GET_SKILL(ch, idx) == 0) {
            continue;
        }
        struct xmlc_node *new_node = xml_node(
            "skill",
            xml_str_attr("name", spell_to_str(idx)),
            xml_int_attr("level", GET_SKILL(ch, idx)),
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

static struct xmlc_node *
xml_collect_tongues(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        int vnum = GPOINTER_TO_INT(key);
        struct tongue *tongue = val;
        if (CHECK_TONGUE(ch, vnum) == 0) {
            continue;
        }

        struct xmlc_node *new_node = xml_node(
            "tongue",
            xml_str_attr("name", tongue->name),
            xml_int_attr("level", CHECK_TONGUE(ch, vnum)),
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

static struct xmlc_node *
xml_collect_recent_kills(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    for (GList *it = GET_RECENT_KILLS(ch); it; it = it->next) {
        struct kill_record *kill = it->data;

        struct xmlc_node *new_node = xml_node(
            "recentkill",
            xml_int_attr("vnum", kill->vnum),
            xml_int_attr("times", kill->times),
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

static struct xmlc_node *
xml_collect_grievances(struct creature *ch)
{
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    for (GList *it = GET_GRIEVANCES(ch); it; it = it->next) {
        struct grievance *grievance = it->data;

        struct xmlc_node *new_node = xml_node(
            "grievance",
            xml_int_attr("time", grievance->time),
            xml_int_attr("player", grievance->player_id),
            xml_int_attr("reputation", grievance->rep),
            xml_str_attr("kind", grievance_kind_descs[grievance->grievance]),
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

static struct xmlc_node *
xml_collect_tags(struct creature *ch)
{
    if (!ch->player_specials->tags) {
        return xml_null_node();
    }
    struct xmlc_node *node_list = NULL, *last_node = NULL;
    GHashTableIter iter;
    char *key;

    g_hash_table_iter_init(&iter, ch->player_specials->tags);
    while (g_hash_table_iter_next(&iter, (gpointer *)&key, NULL)) {
        struct xmlc_node *new_node = xml_node(
            "tag",
            xml_str_attr("tag", key),
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

static struct xmlc_node *
xml_collect_affects(struct aff_stash *aff_stash)
{
    struct xmlc_node *aff_list = NULL, *last_aff = NULL;
    for (struct affected_type *aff = aff_stash->saved_affs;aff;aff = aff->next) {
        struct xmlc_node *new_aff =
            xml_node("affect",
                     xml_int_attr("type", aff->type),
                     xml_int_attr("duration", aff->duration),
                     xml_int_attr("modifier", aff->modifier),
                     xml_int_attr("location", aff->location),
                     xml_int_attr("level", aff->level),
                     xml_bool_attr("instant", aff->is_instant),
                     xml_hex_attr("affbits", aff->bitvector),
                     xml_int_attr("index", aff->aff_index),
                     xml_int_attr("owner", aff->owner),
                     NULL);
        if (last_aff) {
            last_aff->next = new_aff;
            last_aff = last_aff->next;
        } else {
            aff_list = last_aff = new_aff;
        }
    }

    return aff_list;
}

void
save_player_to_file(struct creature *ch, const char *path)
{
    int hit = GET_HIT(ch), mana = GET_MANA(ch), move = GET_MOVE(ch);

    struct aff_stash aff_stash = { 0 };

    stash_creature_affects(ch, &aff_stash);

    xml_output(
        path,
        xml_node("creature",
                 xml_str_attr("name", GET_NAME(ch)),
                 xml_int_attr("idnum", GET_IDNUM(ch)),
                 xml_node("points",
                          xml_int_attr("hit", ch->points.hit),
                          xml_int_attr("mana", ch->points.mana),
                          xml_int_attr("move", ch->points.move),
                          xml_int_attr("maxhit", ch->points.max_hit),
                          xml_int_attr("maxmana", ch->points.max_mana),
                          xml_int_attr("maxmove", ch->points.max_move),
                          NULL),
                 xml_node("money",
                          xml_int_attr("gold", ch->points.gold),
                          xml_int_attr("cash", ch->points.cash),
                          xml_int_attr("xp", ch->points.exp),
                          NULL),
                 xml_node("stats",
                          xml_int_attr("level", GET_LEVEL(ch)),
                          xml_str_attr("sex", genders[(int)GET_SEX(ch)]),
                          xml_str_attr("race", race_name_by_idnum(GET_RACE(ch))),
                          xml_int_attr("height", GET_HEIGHT(ch)),
                          xml_float_attr("weight", GET_WEIGHT(ch)),
                          xml_int_attr("align", GET_ALIGNMENT(ch)),
                          NULL),
                 xml_node("class",
                          xml_str_attr("name", class_names[GET_CLASS(ch)]),
                          xml_if(GET_REMORT_CLASS(ch) != CLASS_UNDEFINED,
                                 xml_str_attr("remort", class_names[GET_REMORT_CLASS(ch)])),
                          xml_if(GET_REMORT_GEN(ch) > 0,
                                 xml_int_attr("gen", GET_REMORT_GEN(ch))),
                          xml_if(IS_CYBORG(ch) && GET_OLD_CLASS(ch) != -1,
                                 xml_str_attr("subclass", borg_subchar_class_names[GET_OLD_CLASS(ch)])),
                          xml_if(IS_CYBORG(ch) && GET_TOT_DAM(ch) != 0,
                                 xml_int_attr("total_dam", GET_TOT_DAM(ch))),
                          xml_if(GET_BROKE(ch) && GET_BROKE(ch) != 0,
                                 xml_int_attr("broken", GET_BROKE(ch))),
                          xml_if(IS_MAGE(ch) && GET_SKILL(ch, SPELL_MANA_SHIELD) > 0,
                                 xml_int_attr("manash_low", ch->player_specials->saved.mana_shield_low)),
                          xml_if(IS_MAGE(ch) && GET_SKILL(ch, SPELL_MANA_SHIELD) > 0,
                                 xml_int_attr("manash_pct", ch->player_specials->saved.mana_shield_pct)),
                          NULL),
                 xml_node("time",
                          xml_int_attr("birth", ch->player.time.birth),
                          xml_int_attr("death", ch->player.time.death),
                          xml_int_attr("played", ch->player.time.played),
                          xml_int_attr("last", ch->player.time.logon),
                          NULL),
                 xml_node("carnage",
                          xml_int_attr("pkills", GET_PKILLS(ch)),
                          xml_int_attr("akills", GET_ARENAKILLS(ch)),
                          xml_int_attr("mkills", GET_MOBKILLS(ch)),
                          xml_int_attr("deaths", GET_PC_DEATHS(ch)),
                          xml_int_attr("reputation", ch->player_specials->saved.reputation),
                          NULL),
                 xml_node("attr",
                          xml_int_attr("str", ch->real_abils.str),
                          xml_int_attr("int", ch->real_abils.intel),
                          xml_int_attr("wis", ch->real_abils.wis),
                          xml_int_attr("dex", ch->real_abils.dex),
                          xml_int_attr("con", ch->real_abils.con),
                          xml_int_attr("cha", ch->real_abils.cha),
                          NULL),
                 xml_node("condition",
                          xml_int_attr("hunger", GET_COND(ch, FULL)),
                          xml_int_attr("thirst", GET_COND(ch, THIRST)),
                          xml_int_attr("drunk", GET_COND(ch, DRUNK)),
                          NULL),
                 xml_node("player",
                          xml_int_attr("wimpy", GET_WIMP_LEV(ch)),
                          xml_int_attr("lp", GET_LIFE_POINTS(ch)),
                          xml_int_attr("clan", GET_CLAN(ch)),
                          NULL),
                 xml_node("rent",
                          xml_int_attr("code", ch->player_specials->rentcode),
                          xml_int_attr("perdiem", ch->player_specials->rent_per_day),
                          xml_int_attr("currency", ch->player_specials->rent_currency),
                          xml_if(ch->player_specials->rentcode == RENT_CREATING
                                 || ch->player_specials->rentcode == RENT_REMORTING,
                                 xml_str_attr("state", desc_modes[(int)ch->player_specials->desc_mode])),
                          NULL),
                 xml_node("home",
                          xml_int_attr("town", GET_HOME(ch)),
                          xml_int_attr("homeroom", GET_HOMEROOM(ch)),
                          xml_int_attr("loadroom", GET_LOADROOM(ch)),
                          NULL),
                 xml_node("quest",
                          xml_if(GET_QUEST(ch),
                                 xml_int_attr("current", GET_QUEST(ch))),
                          xml_if(GET_LEVEL(ch) >= LVL_IMMORT,
                                 xml_int_attr("allowance", GET_QUEST_ALLOWANCE(ch))),
                          xml_if(GET_IMMORT_QP(ch),
                                 xml_int_attr("points", GET_IMMORT_QP(ch))),
                          NULL),
                 xml_node("bits",
                          xml_hex_attr("flag1", ch->char_specials.saved.act),
                          xml_hex_attr("flag2", ch->player_specials->saved.plr2_bits),
                          NULL),
                 xml_if(PLR_FLAGGED(ch, PLR_FROZEN),
                        xml_node("frozen",
                                 xml_int_attr("thaw_time", ch->player_specials->thaw_time),
                                 xml_int_attr("freezer_id", ch->player_specials->freezer_id),
                                 NULL)),
                 xml_node("prefs",
                          xml_hex_attr("flag1", ch->player_specials->saved.pref),
                          xml_hex_attr("flag2", ch->player_specials->saved.pref2),
                          xml_str_attr("tongue", tongue_name(GET_TONGUE(ch))),
                          NULL),
                 xml_node("affects",
                          xml_hex_attr("flag1", ch->char_specials.saved.affected_by),
                          xml_hex_attr("flag2", ch->char_specials.saved.affected2_by),
                          xml_hex_attr("flag3", ch->char_specials.saved.affected3_by),
                          NULL),
                 xml_splice(xml_collect_weaponspecs(ch)),
                 xml_if(GET_TITLE(ch) && *GET_TITLE(ch),
                        xml_node("title",
                                 xml_text(GET_TITLE(ch)),
                                 NULL)),
                 xml_if(GET_LEVEL(ch) >= 50,
                        xml_node("immort",
                                 xml_str_attr("badge", BADGE(ch)),
                                 xml_int_attr("qlog", GET_QLOG_LEVEL(ch)),
                                 xml_int_attr("invis", GET_INVIS_LVL(ch)),
                                 NULL)),
                 xml_if(GET_LEVEL(ch) >= 50 && POOFIN(ch) && *POOFIN(ch),
                        xml_node("poofin",
                                 xml_text(POOFIN(ch)),
                                 NULL)),
                 xml_if(GET_LEVEL(ch) >= 50 && POOFOUT(ch) && *POOFOUT(ch),
                        xml_node("poofout",
                                 xml_text(POOFOUT(ch)),
                                 NULL)),
                 xml_if(ch->player.description && *ch->player.description,
                        xml_node("description",
                                 xml_text(ch->player.description),
                                 NULL)),
                 xml_splice(xml_collect_aliases(ch)),
                 xml_splice(xml_collect_affects(&aff_stash)),
                 xml_if(!IS_IMMORT(ch),
                        xml_splice(xml_collect_skills(ch))),
                 xml_if(!IS_IMMORT(ch),
                        xml_splice(xml_collect_tongues(ch))),
                 xml_if(!IS_IMMORT(ch),
                        xml_splice(xml_collect_recent_kills(ch))),
                 xml_if(!IS_IMMORT(ch),
                        xml_splice(xml_collect_grievances(ch))),
                 xml_splice(xml_collect_tags(ch)),
                 NULL));

    restore_creature_affects(ch, &aff_stash);

    // Modifying creature affects can mess with hit/mana/move - restore them.
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), hit);
    GET_MANA(ch) = MIN(GET_MAX_MANA(ch), mana);
    GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), move);
}


void
save_player_to_xml(struct creature *ch)
{
    void expire_old_grievances(struct creature *);

    char *path = get_player_file_path(GET_IDNUM(ch));

    if (GET_IDNUM(ch) == 0) {
        slog("Attempt to save creature with idnum==0");
        raise(SIGSEGV);
    }

    expire_old_grievances(ch);

    save_player_to_file(ch, path);
}

// Saves the given characters equipment to a file. Intended for use while
// the character is still in the game.
bool
crashsave(struct creature *ch)
{
    time_t now = time(NULL);

    if (IS_NPC(ch)) {
        return false;
    }
    if (!ch->in_room) {
        return false;
    }
    ch->player_specials->rentcode = RENT_CRASH;
    ch->player_specials->rent_currency = ch->in_room->zone->time_frame;

    ch->player.time.played += now - ch->player.time.logon;
    ch->player.time.logon = now;

    if (!save_player_objects(ch)) {
        return false;
    }

    REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
    save_player_to_xml(ch);
    return true;
}

struct creature *
load_player_from_file(const char *path)
{
    struct creature *ch = NULL;
    char *txt;

    if (access(path, W_OK)) {
        errlog("Unable to open xml player file '%s': %s", path,
               strerror(errno));
        return NULL;
    }
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return NULL;
    }

    /* to save memory, only PC's -- not MOB's -- have player_specials */
    ch = make_creature(true);

    ch->player.name = xmlGetStrProp(root, "name", "");
    ch->char_specials.saved.idnum = xmlGetIntProp(root, "idnum", 0);
    set_title(ch, "");

    ch->player.short_descr = NULL;
    ch->player.long_descr = NULL;

    if (ch->points.max_mana < 100) {
        ch->points.max_mana = 100;
    }

    ch->char_specials.carry_weight = 0;
    ch->char_specials.carry_items = 0;
    ch->char_specials.worn_weight = 0;
    ch->points.armor = 100;
    ch->points.hitroll = 0;
    ch->points.damroll = 0;
    ch->player_specials->saved.speed = 0;

    // Read in the subnodes
    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "points")) {
            ch->points.mana = xmlGetIntProp(node, "mana", 100);
            ch->points.max_mana = xmlGetIntProp(node, "maxmana", 100);
            ch->points.hit = xmlGetIntProp(node, "hit", 100);
            ch->points.max_hit = xmlGetIntProp(node, "maxhit", 100);
            ch->points.move = xmlGetIntProp(node, "move", 100);
            ch->points.max_move = xmlGetIntProp(node, "maxmove", 100);
        } else if (xmlMatches(node->name, "money")) {
            ch->points.gold = xmlGetIntProp(node, "gold", 0);
            ch->points.cash = xmlGetIntProp(node, "cash", 0);
            ch->points.exp = xmlGetIntProp(node, "xp", 0);
        } else if (xmlMatches(node->name, "stats")) {
            ch->player.level = xmlGetIntProp(node, "level", 0);
            ch->player.height = xmlGetIntProp(node, "height", 0);
            ch->player.weight = xmlGetIntProp(node, "weight", 0);
            GET_ALIGNMENT(ch) = xmlGetIntProp(node, "align", 0);
            /***
                Temp fix for negative weights
             ***/

            if (ch->player.weight < 0) {
                calculate_height_weight(ch);
            }

            GET_SEX(ch) = 0;
            char *sex = (char *)xmlGetProp(node, (xmlChar *) "sex");
            if (sex != NULL) {
                GET_SEX(ch) = search_block(sex, genders, false);
            }
            xmlFree(sex);

            GET_RACE(ch) = 0;
            char *race_name = (char *)xmlGetProp(node, (xmlChar *) "race");
            if (race_name != NULL) {
                struct race *race = race_by_name(race_name, true);
                if (race != NULL) {
                    GET_RACE(ch) = race->idnum;
                }
            }
            xmlFree(race_name);

        } else if (xmlMatches(node->name, "class")) {
            GET_OLD_CLASS(ch) = GET_REMORT_CLASS(ch) = GET_CLASS(ch) = -1;

            char *trade = (char *)xmlGetProp(node, (xmlChar *) "name");
            if (trade != NULL) {
                GET_CLASS(ch) = search_block(trade, class_names, false);
                xmlFree(trade);
            }

            trade = (char *)xmlGetProp(node, (xmlChar *) "remort");
            if (trade != NULL) {
                GET_REMORT_CLASS(ch) = search_block(trade, class_names, false);
                xmlFree(trade);
            }

            if (IS_CYBORG(ch)) {
                char *subclass =
                    (char *)xmlGetProp(node, (xmlChar *) "subclass");
                if (subclass != NULL) {
                    GET_OLD_CLASS(ch) = search_block(subclass,
                                                     borg_subchar_class_names, false);
                    xmlFree(subclass);
                }
            }

            if (GET_CLASS(ch) == CLASS_MAGE) {
                ch->player_specials->saved.mana_shield_low =
                    xmlGetLongProp(node, "manash_low", 0);
                ch->player_specials->saved.mana_shield_pct =
                    xmlGetLongProp(node, "manash_pct", 0);
            }

            GET_REMORT_GEN(ch) = xmlGetIntProp(node, "gen", 0);
            GET_TOT_DAM(ch) = xmlGetIntProp(node, "total_dam", 0);
            GET_BROKE(ch) = xmlGetIntProp(node, "broken", 0);
        } else if (xmlMatches(node->name, "time")) {
            ch->player.time.birth = xmlGetLongProp(node, "birth", 0);
            ch->player.time.death = xmlGetLongProp(node, "death", 0);
            ch->player.time.played = xmlGetLongProp(node, "played", 0);
            ch->player.time.logon = xmlGetLongProp(node, "last", 0);
        } else if (xmlMatches(node->name, "carnage")) {
            GET_PKILLS(ch) = xmlGetIntProp(node, "pkills", 0);
            GET_ARENAKILLS(ch) = xmlGetIntProp(node, "akills", 0);
            GET_MOBKILLS(ch) = xmlGetIntProp(node, "mkills", 0);
            GET_PC_DEATHS(ch) = xmlGetIntProp(node, "deaths", 0);
            RAW_REPUTATION_OF(ch) = xmlGetIntProp(node, "reputation", 0);
            GET_SEVERITY(ch) = xmlGetIntProp(node, "severity", 0);
        } else if (xmlMatches(node->name, "attr")) {
            ch->aff_abils.str = ch->real_abils.str =
                                    xmlGetIntProp(node, "str", 0);
            ch->aff_abils.str = ch->real_abils.str +=
                                    xmlGetIntProp(node, "stradd", 0) / 10;
            ch->aff_abils.intel = ch->real_abils.intel =
                                      xmlGetIntProp(node, "int", 0);
            ch->aff_abils.wis = ch->real_abils.wis =
                                    xmlGetIntProp(node, "wis", 0);
            ch->aff_abils.dex = ch->real_abils.dex =
                                    xmlGetIntProp(node, "dex", 0);
            ch->aff_abils.con = ch->real_abils.con =
                                    xmlGetIntProp(node, "con", 0);
            ch->aff_abils.cha = ch->real_abils.cha =
                                    xmlGetIntProp(node, "cha", 0);
        } else if (xmlMatches(node->name, "condition")) {
            GET_COND(ch, THIRST) = xmlGetIntProp(node, "thirst", 0);
            GET_COND(ch, FULL) = xmlGetIntProp(node, "hunger", 0);
            GET_COND(ch, DRUNK) = xmlGetIntProp(node, "drunk", 0);
        } else if (xmlMatches(node->name, "player")) {
            GET_WIMP_LEV(ch) = xmlGetIntProp(node, "wimpy", 0);
            GET_LIFE_POINTS(ch) = xmlGetIntProp(node, "lp", 0);
            GET_CLAN(ch) = xmlGetIntProp(node, "clan", 0);
        } else if (xmlMatches(node->name, "home")) {
            GET_HOME(ch) = xmlGetIntProp(node, "town", 0);
            GET_HOMEROOM(ch) = xmlGetIntProp(node, "homeroom", 0);
            GET_LOADROOM(ch) = xmlGetIntProp(node, "loadroom", 0);
        } else if (xmlMatches(node->name, "quest")) {
            GET_QUEST(ch) = xmlGetIntProp(node, "current", 0);
            GET_IMMORT_QP(ch) = xmlGetIntProp(node, "points", 0);
            GET_QUEST_ALLOWANCE(ch) = xmlGetIntProp(node, "allowance", 0);
        } else if (xmlMatches(node->name, "bits")) {
            char *flag = (char *)xmlGetProp(node, (xmlChar *) "flag1");
            ch->char_specials.saved.act = hex2dec(flag);
            xmlFree(flag);

            flag = (char *)xmlGetProp(node, (xmlChar *) "flag2");
            ch->player_specials->saved.plr2_bits = hex2dec(flag);
            xmlFree(flag);
        } else if (xmlMatches(node->name, "frozen")) {
            ch->player_specials->thaw_time =
                xmlGetIntProp(node, "thaw_time", 0);
            ch->player_specials->freezer_id =
                xmlGetIntProp(node, "freezer_id", 0);
        } else if (xmlMatches(node->name, "prefs")) {
            char *flag = (char *)xmlGetProp(node, (xmlChar *) "flag1");
            ch->player_specials->saved.pref = hex2dec(flag);
            xmlFree(flag);

            flag = (char *)xmlGetProp(node, (xmlChar *) "flag2");
            ch->player_specials->saved.pref2 = hex2dec(flag);
            xmlFree(flag);

            flag = (char *)xmlGetProp(node, (xmlChar *) "tongue");
            if (flag) {
                GET_TONGUE(ch) = find_tongue_idx_by_name(flag);
                xmlFree(flag);
            }
        } else if (xmlMatches(node->name, "weaponspec")) {
            int vnum = xmlGetIntProp(node, "vnum", -1);
            int level = xmlGetIntProp(node, "level", 0);
            if (vnum > 0 && level > 0) {
                for (int i = 0; i < MAX_WEAPON_SPEC; i++) {
                    if (GET_WEAP_SPEC(ch, i).level <= 0) {
                        GET_WEAP_SPEC(ch, i).vnum = vnum;
                        GET_WEAP_SPEC(ch, i).level = level;
                        break;
                    }
                }
            }
        } else if (xmlMatches(node->name, "title")) {
            char *txt;

            txt = (char *)xmlNodeGetContent(node);
            set_title(ch, txt);
            xmlFree(txt);
        } else if (xmlMatches(node->name, "affect")) {
            struct affected_type af;
            init_affect(&af);
            af.type = xmlGetIntProp(node, "type", 0);
            af.duration = xmlGetIntProp(node, "duration", 0);
            af.modifier = xmlGetIntProp(node, "modifier", 0);
            af.location = xmlGetIntProp(node, "location", 0);
            af.level = xmlGetIntProp(node, "level", 0);
            af.aff_index = xmlGetIntProp(node, "index", 0);
            af.owner = xmlGetIntProp(node, "owner", 0);
            af.next = NULL;
            char *instant = (char *)xmlGetProp(node, (xmlChar *) "instant");
            if (instant != NULL) {
                af.is_instant = (streq(instant, "yes"));
                xmlFree(instant);
            }

            char *bits = (char *)xmlGetProp(node, (xmlChar *) "affbits");
            af.bitvector = hex2dec(bits);
            xmlFree(bits);

            affect_to_char(ch, &af);

        } else if (xmlMatches(node->name, "affects")) {
            // PCs shouldn't have ANY perm affects
            if (IS_NPC(ch)) {
                char *flag = (char *)xmlGetProp(node, (xmlChar *) "flag1");
                AFF_FLAGS(ch) = hex2dec(flag);
                xmlFree(flag);

                flag = (char *)xmlGetProp(node, (xmlChar *) "flag2");
                AFF2_FLAGS(ch) = hex2dec(flag);
                xmlFree(flag);

                flag = (char *)xmlGetProp(node, (xmlChar *) "flag3");
                AFF3_FLAGS(ch) = hex2dec(flag);
                xmlFree(flag);
            } else {
                AFF_FLAGS(ch) = 0;
                AFF2_FLAGS(ch) = 0;
                AFF3_FLAGS(ch) = 0;
            }

        } else if (xmlMatches(node->name, "skill")) {
            char *spellName = (char *)xmlGetProp(node, (xmlChar *) "name");
            int index = str_to_spell(spellName);
            if (index >= 0) {
                SET_SKILL(ch, index, xmlGetIntProp(node, "level", 0));
            }
            xmlFree(spellName);
        } else if (xmlMatches(node->name, "tongue")) {
            char *tongue = (char *)xmlGetProp(node, (xmlChar *) "name");
            int index = find_tongue_idx_by_name(tongue);
            if (index >= 0) {
                SET_TONGUE(ch, index,
                           MIN(100, xmlGetIntProp(node, "level", 0)));
            }
            xmlFree(tongue);
        } else if (xmlMatches(node->name, "alias")) {
            struct alias_data *alias;
            CREATE(alias, struct alias_data, 1);
            alias->type = xmlGetIntProp(node, "type", 0);
            alias->alias = xmlGetStrProp(node, "alias", NULL);
            alias->replacement = xmlGetStrProp(node, "replace", NULL);
            if (alias->alias == NULL || alias->replacement == NULL) {
                free(alias->alias);
                free(alias->replacement);
                free(alias);
            } else {
                add_alias(ch, alias);
            }
        } else if (xmlMatches(node->name, "description")) {
            txt = (char *)xmlNodeGetContent(node);
            ch->player.description = strdup(tmp_gsub(txt, "\n", "\r\n"));
            xmlFree(txt);
        } else if (xmlMatches(node->name, "poofin")) {
            POOFIN(ch) = (char *)xmlNodeGetContent(node);
        } else if (xmlMatches(node->name, "poofout")) {
            POOFOUT(ch) = (char *)xmlNodeGetContent(node);
        } else if (xmlMatches(node->name, "immort")) {
            txt = (char *)xmlGetProp(node, (xmlChar *) "badge");
            strncpy(BADGE(ch), txt, 7);
            BADGE(ch)[7] = '\0';
            xmlFree(txt);

            GET_QLOG_LEVEL(ch) = xmlGetIntProp(node, "qlog", 0);
            GET_INVIS_LVL(ch) = xmlGetIntProp(node, "invis", 0);
        } else if (xmlMatches(node->name, "rent")) {
            char *txt;

            ch->player_specials->rentcode = xmlGetIntProp(node, "code", 0);
            ch->player_specials->rent_per_day =
                xmlGetIntProp(node, "perdiem", 0);
            ch->player_specials->rent_currency =
                xmlGetIntProp(node, "currency", 0);
            txt = (char *)xmlGetProp(node, (xmlChar *) "state");
            if (txt) {
                ch->player_specials->desc_mode =
                    (enum cxn_state)search_block(txt, desc_modes, false);
                xmlFree(txt);
            }
        } else if (xmlMatches(node->name, "recentkill")) {
            struct kill_record *kill;

            CREATE(kill, struct kill_record, 1);
            kill->vnum = xmlGetIntProp(node, "vnum", 0);
            kill->times = xmlGetIntProp(node, "times", 0);
            GET_RECENT_KILLS(ch) = g_list_prepend(GET_RECENT_KILLS(ch), kill);
        } else if (xmlMatches(node->name, "grievance")) {
            struct grievance *grievance;

            txt = (char *)xmlGetProp(node, (xmlChar *) "kind");
            if (txt) {
                CREATE(grievance, struct grievance, 1);
                grievance->time = xmlGetIntProp(node, "time", 0);
                grievance->player_id = xmlGetIntProp(node, "player", 0);
                grievance->rep = xmlGetIntProp(node, "reputation", 0);
                grievance->grievance =
                    search_block(txt, grievance_kind_descs, false);
                GET_GRIEVANCES(ch) =
                    g_list_prepend(GET_GRIEVANCES(ch), grievance);
                xmlFree(txt);
            }
        } else if (xmlMatches(node->name, "tag")) {
            char *tag = (char *)xmlGetProp(node, (xmlChar *) "tag");
            add_player_tag(ch, tag);
            xmlFree(tag);
        }
    }

    xmlFreeDoc(doc);
    return ch;
}

struct creature *
load_player_from_xml(int id)
{
    char *path = get_player_file_path(id);
    struct creature *ch = load_player_from_file(path);
    int idx;

    if (!ch) {
        return NULL;
    }

    // reset all imprint rooms
    for (int i = 0; i < MAX_IMPRINT_ROOMS; i++) {
        GET_IMPRINT_ROOM(ch, i) = -1;
    }

    // Make sure the NPC flag isn't set
    if (IS_SET(ch->char_specials.saved.act, NPC_ISNPC)) {
        REMOVE_BIT(ch->char_specials.saved.act, NPC_ISNPC);
        errlog("loadFromXML %s loaded with NPC_ISNPC bit set!", GET_NAME(ch));
    }
    // Check for freezer expiration
    if (PLR_FLAGGED(ch, PLR_FROZEN)
        && time(NULL) >= ch->player_specials->thaw_time) {
        REMOVE_BIT(PLR_FLAGS(ch), PLR_FROZEN);
    }
    // If you're not poisioned and you've been away for more than an hour,
    // we'll set your HMV back to full
    if (!IS_POISONED(ch)
        && (((long)(time(NULL) - ch->player.time.logon)) >= SECS_PER_REAL_HOUR)) {
        GET_HIT(ch) = GET_MAX_HIT(ch);
        GET_MOVE(ch) = GET_MAX_MOVE(ch);
        GET_MANA(ch) = GET_MAX_MANA(ch);
    }

    if (GET_LEVEL(ch) >= 50) {
        for (idx = 0; idx < MAX_SKILLS; idx++) {
            ch->player_specials->saved.skills[idx] = 100;
        }
        for (idx = 0; idx < MAX_TONGUES; idx++) {
            ch->language_data.tongues[idx] = 100;
        }
    }

    return ch;
}

void
set_player_field(struct creature *ch, const char *key, const char *val)
{
    if (streq(key, "race")) {
        GET_RACE(ch) = atoi(val);
    } else if (streq(key, "class")) {
        GET_CLASS(ch) = atoi(val);
    } else if (streq(key, "remort")) {
        GET_REMORT_CLASS(ch) = atoi(val);
    } else if (streq(key, "name")) {
        ch->player.name = strdup(val);
    } else if (streq(key, "title")) {
        GET_TITLE(ch) = strdup(val);
    } else if (streq(key, "poofin")) {
        POOFIN(ch) = strdup(val);
    } else if (streq(key, "poofout")) {
        POOFOUT(ch) = strdup(val);
    } else if (streq(key, "immbadge")) {
        strcpy_s(BADGE(ch), sizeof(BADGE(ch)), val);
    } else if (streq(key, "sex")) {
        GET_SEX(ch) = atoi(val);
    } else if (streq(key, "hitp")) {
        GET_HIT(ch) = atoi(val);
    } else if (streq(key, "mana")) {
        GET_MANA(ch) = atoi(val);
    } else if (streq(key, "move")) {
        GET_MOVE(ch) = atoi(val);
    } else if (streq(key, "maxhitp")) {
        GET_MAX_HIT(ch) = atoi(val);
    } else if (streq(key, "maxmana")) {
        GET_MAX_MANA(ch) = atoi(val);
    } else if (streq(key, "maxmove")) {
        GET_MAX_MOVE(ch) = atoi(val);
    } else if (streq(key, "gold")) {
        GET_GOLD(ch) = atol(val);
    } else if (streq(key, "cash")) {
        GET_CASH(ch) = atol(val);
    } else if (streq(key, "exp")) {
        GET_EXP(ch) = atol(val);
    } else if (streq(key, "level")) {
        GET_LEVEL(ch) = atol(val);
    } else if (streq(key, "height")) {
        GET_HEIGHT(ch) = atol(val);
    } else if (streq(key, "weight")) {
        GET_WEIGHT(ch) = atol(val);
    } else if (streq(key, "align")) {
        GET_ALIGNMENT(ch) = atoi(val);
    } else if (streq(key, "gen")) {
        GET_REMORT_GEN(ch) = atoi(val);
    } else if (streq(key, "birth_time")) {
        ch->player.time.birth = atol(val);
    } else if (streq(key, "death_time")) {
        ch->player.time.death = atol(val);
    } else if (streq(key, "played_time")) {
        ch->player.time.played = atol(val);
    } else if (streq(key, "login_time")) {
        ch->player.time.logon = atol(val);
    } else if (streq(key, "pkills")) {
        GET_PKILLS(ch) = atol(val);
    } else if (streq(key, "mkills")) {
        GET_MOBKILLS(ch) = atol(val);
    } else if (streq(key, "akills")) {
        GET_ARENAKILLS(ch) = atol(val);
    } else if (streq(key, "deaths")) {
        GET_PC_DEATHS(ch) = atol(val);
    } else if (streq(key, "reputation")) {
        ch->player_specials->saved.reputation = atoi(val);
    } else if (streq(key, "str")) {
        ch->aff_abils.str = ch->real_abils.str = atoi(val);
    } else if (streq(key, "int")) {
        ch->aff_abils.intel = ch->real_abils.intel = atoi(val);
    } else if (streq(key, "wis")) {
        ch->aff_abils.wis = ch->real_abils.wis = atoi(val);
    } else if (streq(key, "dex")) {
        ch->aff_abils.dex = ch->real_abils.dex = atoi(val);
    } else if (streq(key, "con")) {
        ch->aff_abils.con = ch->real_abils.con = atoi(val);
    } else if (streq(key, "cha")) {
        ch->aff_abils.cha = ch->real_abils.cha = atoi(val);
    } else if (streq(key, "hunger")) {
        GET_COND(ch, FULL) = atoi(val);
    } else if (streq(key, "thirst")) {
        GET_COND(ch, THIRST) = atoi(val);
    } else if (streq(key, "drunk")) {
        GET_COND(ch, DRUNK) = atoi(val);
    } else {
        slog("Invalid player field %s set to %s", key, val);
    }
}
