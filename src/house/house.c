#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libpq-fe.h>
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
#include "db.h"
#include "account.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "players.h"
#include "tmpstr.h"
#include "str_builder.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "mail.h"
#include "strutil.h"

// usage message
#define HCONTROL_FIND_FORMAT \
    "Usage: hcontrol find <'owner' | 'guest' | 'landlord'> <name|id>\r\n"
#define HCONTROL_DESTROY_FORMAT \
    "Usage: hcontrol destroy <house#>\r\n"
#define HCONTROL_ADD_FORMAT \
    "Usage: hcontrol add <house#> <room#>\r\n"
#define HCONTROL_DELETE_FORMAT \
    "Usage: hcontrol delete <house#> <room#>\r\n"
#define HCONTROL_SET_FORMAT \
    "Usage: hcontrol set <house#> <rate|owner|type|landlord> <'value'|public|private|rental>\r\n"
#define HCONTROL_SHOW_FORMAT \
    "Usage: hcontrol show [house#]\r\n"
#define HCONTROL_BUILD_FORMAT \
    "Usage: hcontrol build <player name|account#> <first room#> <last room#>\r\n"

#define HCONTROL_FORMAT \
    (HCONTROL_BUILD_FORMAT \
     HCONTROL_DESTROY_FORMAT \
     HCONTROL_ADD_FORMAT \
     HCONTROL_DELETE_FORMAT \
     HCONTROL_SET_FORMAT \
     HCONTROL_SHOW_FORMAT \
     "Usage: hcontrol save/recount\r\n" \
     "Usage: hcontrol where\r\n" \
     "Usage: hcontrol reload [house#] (Use with caution!)\r\n")

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern int no_plrtext;
void extract_norents(struct obj_data *obj);

time_t last_house_collection;
GList *houses;

static char *
get_house_file_path(int id)
{
    return tmp_sprintf("players/housing/%d/%04d.dat", (id % 10), id);
}

/**
 * mode:  true, recursively sums object costs.
 *       false, recursively sums object rent prices
 * obj: the object the add to the current sum
 * top_o: the first object totalled.
 **/
int
recurs_obj_cost(struct obj_data *obj, bool mode, struct obj_data *top_o)
{
    if (obj == NULL) {
        return 0;               // end of list

    }
    if (obj->in_obj && obj->in_obj != top_o) {
        if (!mode) { /** rent mode **/
            return ((IS_OBJ_STAT(obj, ITEM_NORENT) ? 0 : GET_OBJ_RENT(obj)) +
                    recurs_obj_cost(obj->contains, mode, top_o) +
                    recurs_obj_cost(obj->next_content, mode, top_o));
        } else {     /** cost mode **/
            return (GET_OBJ_COST(obj) + recurs_obj_cost(obj->contains, mode,
                                                        top_o)
                    + recurs_obj_cost(obj->next_content, mode, top_o));
        }
    } else if (!mode) {         // rent mode
        return ((IS_OBJ_STAT(obj, ITEM_NORENT) ? 0 : GET_OBJ_RENT(obj)) +
                recurs_obj_cost(obj->contains, mode, top_o));
    } else {
        return (GET_OBJ_COST(obj) + recurs_obj_cost(obj->contains, mode,
                                                    top_o));
    }
}

int
recurs_obj_contents(struct obj_data *obj, struct obj_data *top_o)
{
    if (!obj) {
        return 0;
    }

    if (obj->in_obj && obj->in_obj != top_o) {
        return (1 + recurs_obj_contents(obj->next_content, top_o) +
                recurs_obj_contents(obj->contains, top_o));
    }

    return (1 + recurs_obj_contents(obj->contains, top_o));
}

const char *
house_type_name(enum house_type type)
{
    switch (type) {
    case PRIVATE:
        return "Private";
    case PUBLIC:
        return "Public";
    case RENTAL:
        return "Rental";
    case CLAN:
        return "Clan";
    default:
        return "Invalid";
    }
}

const char *
house_type_short_name(enum house_type type)
{
    switch (type) {
    case PRIVATE:
        return "PRIV";
    case PUBLIC:
        return "PUB";
    case RENTAL:
        return "RENT";
    case CLAN:
        return "CLAN";
    default:
        return "ERR";
    }
}

enum house_type
house_type_from_name(const char *name)
{
    if (name == NULL) {
        return INVALID;
    }
    if (strcmp(name, "Private") == 0) {
        return PRIVATE;
    }
    if (strcmp(name, "Public") == 0) {
        return PUBLIC;
    }
    if (strcmp(name, "Rental") == 0) {
        return RENTAL;
    }
    if (strcmp(name, "Clan") == 0) {
        return CLAN;
    }
    return INVALID;
}

bool
is_house_guest(struct house *house, long idnum)
{
    switch (house->type) {
    case PRIVATE:
    case PUBLIC:
    case RENTAL:
        return g_list_find(house->guests, GINT_TO_POINTER(idnum));
    case CLAN: {
        // if there is no clan then check guests
        struct clan_data *clan = real_clan(house->owner_id);
        if (clan == NULL) {
            return g_list_find(house->guests, GINT_TO_POINTER(idnum));
        }
        // if they're not a member, check the guests
        struct clanmember_data *member = real_clanmember(idnum, clan);
        if (member == NULL) {
            return g_list_find(house->guests, GINT_TO_POINTER(idnum));
        }
        return true;
    }
    default:
        return false;
    }
}

bool
add_house_guest(struct house *house, long guest)
{
    if (is_house_guest(house, guest)) {
        return false;
    } else {
        house->guests = g_list_prepend(house->guests, GINT_TO_POINTER(guest));
        return true;
    }
}

bool
remove_house_guest(struct house *house, long guest)
{
    house->guests = g_list_remove_all(house->guests, GINT_TO_POINTER(guest));
    return true;
}

bool
house_has_room(struct house *house, room_num room)
{
    return g_list_find(house->rooms, GINT_TO_POINTER(room));
}

bool
add_house_room(struct house *house, room_num room)
{
    if (house_has_room(house, room)) {
        return false;
    } else {
        house->rooms = g_list_prepend(house->rooms, GINT_TO_POINTER(room));
        return true;
    }
}

bool
remove_house_room(struct house *house, room_num room)
{
    house->rooms = g_list_remove_all(house->rooms, GINT_TO_POINTER(room));
    return true;
}

struct house *
make_house(int idnum, int owner)
{
    struct house *house;

    CREATE(house, struct house, 1);
    house->id = idnum;
    house->created = time(NULL);
    house->type = PRIVATE;
    house->owner_id = owner;

    return house;
}

void
free_house(struct house *house)
{
    for (GList *it = house->rooms; it; it = it->next) {
        room_num room_idnum = GPOINTER_TO_INT(it->data);

        struct room_data *room = real_room(GPOINTER_TO_INT(room_idnum));
        if (room) {
            REMOVE_BIT(ROOM_FLAGS(room), ROOM_HOUSE | ROOM_HOUSE_CRASH);
        }
    }

    g_list_free(house->rooms);
    g_list_free(house->guests);
    free(house);
}

bool
house_is_owner(struct house *house, struct creature *ch)
{
    switch (house->type) {
    case PRIVATE:
    case PUBLIC:
    case RENTAL:
        return house->owner_id == player_account_by_idnum(GET_IDNUM(ch));
    case CLAN: {
        struct clan_data *clan = real_clan(house->owner_id);
        if (clan == NULL) {
            return false;
        }
        struct clanmember_data *member =
            real_clanmember(GET_IDNUM(ch), clan);
        if (member == NULL) {
            return false;
        }
        return PLR_FLAGGED(ch, PLR_CLAN_LEADER);
    }
    default:
        return false;
    }
}

unsigned int
house_count(void)
{
    return g_list_length(houses);
}

struct house *
get_house_by_index(int index)
{
    return g_list_nth_data(houses, index);
}

gint
this_house_has_room(struct house *house, gpointer user_data)
{
    room_num room_idnum = GPOINTER_TO_INT(user_data);

    return (house_has_room(house, room_idnum)) ? 0 : -1;
}

struct house *
find_house_by_room(room_num room_idnum)
{
    struct room_data *room = real_room(room_idnum);
    if (!room) {
        return NULL;
    }

    GList *it = g_list_find_custom(houses,
                                   GINT_TO_POINTER(room_idnum),
                                   (GCompareFunc) this_house_has_room);
    return (it) ? it->data : NULL;
}

gint
house_has_idnum(struct house *house, gpointer user_data)
{
    int idnum = GPOINTER_TO_INT(user_data);
    return (house->id == idnum) ? 0 : -1;
}

struct house *
find_house_by_idnum(int idnum)
{
    GList *it =
        g_list_find_custom(houses, GINT_TO_POINTER(idnum), (GCompareFunc)house_has_idnum);
    return (it) ? it->data : NULL;
}

gint
house_has_owner(struct house *house, gpointer user_data)
{
    int idnum = GPOINTER_TO_INT(user_data);
    return (house->type == PRIVATE && house->owner_id == idnum) ? 0 : -1;
}

struct house *
find_house_by_owner(int idnum)
{
    GList *it =
        g_list_find_custom(houses, GINT_TO_POINTER(idnum), (GCompareFunc) house_has_owner);
    return (it) ? it->data : NULL;
}

gint
house_has_clan_owner(struct house *house, gpointer user_data)
{
    int idnum = GPOINTER_TO_INT(user_data);
    return (house->type == CLAN && house->owner_id == idnum) ? 0 : -1;
}

struct house *
find_house_by_clan(int idnum)
{
    GList *it =
        g_list_find_custom(houses, GINT_TO_POINTER(idnum), (GCompareFunc) house_has_clan_owner);
    return (it) ? it->data : NULL;
}

/* note: arg passed must be house vnum, so there. */
bool
can_enter_house(struct creature *ch, room_num room_vnum)
{
    struct house *house = find_house_by_room(room_vnum);
    if (!house) {
        return true;
    }

    if (is_authorized(ch, ENTER_HOUSES, house)) {
        return true;
    }

    if (IS_NPC(ch)) {
        // so charmies can walk around in the master's house
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            return can_enter_house(ch->master, room_vnum);
        }
        return false;
    }

    switch (house->type) {
    case PUBLIC:
        return true;
    case RENTAL:
    case PRIVATE:
        if (ch->account && ch->account->id == house->owner_id) {
            return true;
        }
        return is_house_guest(house, GET_IDNUM(ch));
    case CLAN: {
        struct clan_data *clan = real_clan(house->owner_id);
        if (clan == NULL) {
            return true;
        }
        struct clanmember_data *member =
            real_clanmember(GET_IDNUM(ch), clan);
        if (member != NULL) {
            return true;
        }
        return false;
    }
    case INVALID:
        return false;
    }

    return false;
}

bool
can_hedit_room(struct creature *ch, struct room_data *room)
{
    struct house *h = find_house_by_room(room->number);
    if (h == NULL) {
        return false;
    }
    return is_authorized(ch, EDIT_HOUSE, h);
}

bool
destroy_house(struct house *house)
{
    houses = g_list_remove(houses, house);

    unlink(get_house_file_path(house->id));

    free_house(house);
    return true;
}

int
repo_note_count(struct house *house)
{
    int result = 0;

    for (struct txt_block *note = house->repo_notes; note; note = note->next) {
        result++;
    }

    return result;
}

void
clear_repo_notes(struct house *house)
{
    struct txt_block *next;

    for (struct txt_block *note = house->repo_notes; note; note = next) {
        next = note->next;
        free(note->text);
        free(note);
    }
    house->repo_notes = NULL;
}

void
update_object_counts(struct obj_data *obj)
{
    if (!obj) {
        return;
    }

    update_object_counts(obj->contains);
    update_object_counts(obj->next_content);

    // don't count NORENT items as being in house
    if (obj->shared->proto && !IS_OBJ_STAT(obj, ITEM_NORENT)) {
        obj->shared->house_count++;
    }
}

void
update_objects_in_house(struct house *house, gpointer ignore __attribute__((unused)))
{
    for (GList *it = house->rooms; it; it = it->next) {
        room_num room_idnum = GPOINTER_TO_INT(it->data);
        struct room_data *room = real_room(room_idnum);
        if (room) {
            update_object_counts(room->contents);
        }
    }
}

void
zero_housed_objects(gpointer vnum __attribute__((unused)), struct obj_data *obj, gpointer ignore __attribute__((unused)))
{
    obj->shared->house_count = 0;
}

void
update_objects_housed_count(void)
{
    g_hash_table_foreach(obj_prototypes, (GHFunc) zero_housed_objects, NULL);

    g_list_foreach(houses, (GFunc) update_objects_in_house, NULL);
}

int
count_objects_in_room(struct room_data *room)
{
    int count = 0;
    for (struct obj_data *obj = room->contents; obj; obj = obj->next_content) {
        count += recurs_obj_contents(obj, NULL);
    }
    return count;
}

int
count_housed_objects(struct house *house, gpointer ignore __attribute__((unused)))
{
    int count = 0;

    for (GList *it = house->rooms; it; it = it->next) {
        room_num room_idnum = GPOINTER_TO_INT(it->data);

        struct room_data *room = real_room(room_idnum);
        if (room) {
            count += count_objects_in_room(room);
        }
    }

    return count;
}

bool
save_house(struct house *house)
{
    char *path = get_house_file_path(house->id);
    FILE *ouf = fopen(path, "w");

    if (!ouf) {
        fprintf(stderr, "Unable to open XML house file for save.[%s] (%s)\n",
                path, strerror(errno));
        return false;
    }
    fprintf(ouf, "<housefile>\n");
    fprintf(ouf, "<house id=\"%d\" type=\"%s\" owner=\"%d\" created=\"%ld\"",
            house->id,
            house_type_name(house->type), house->owner_id, house->created);
    fprintf(ouf, " landlord=\"%ld\" rate=\"%d\" overflow=\"%ld\" >\n",
            house->landlord, house->rental_rate, house->rent_overflow);
    for (GList *i = house->rooms; i; i = i->next) {
        struct room_data *room = real_room(GPOINTER_TO_INT(i->data));
        if (!room) {
            continue;
        }
        fprintf(ouf, "    <room number=\"%d\">\n", GPOINTER_TO_INT(i->data));
        for (struct obj_data *obj = room->contents; obj != NULL;
             obj = obj->next_content) {
            save_object_to_xml(obj, ouf);
        }
        fprintf(ouf, "    </room>\n");
        REMOVE_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
    }
    for (GList *i = house->guests; i; i = i->next) {
        fprintf(ouf, "    <guest id=\"%d\"></guest>\n",
                GPOINTER_TO_INT(i->data));
    }
    for (struct txt_block *i = house->repo_notes; i; i = i->next) {
        fprintf(ouf, "    <repossession note=\"%s\"></repossession>\n",
                xmlEncodeSpecialTmp(i->text));
    }
    fprintf(ouf, "</house>");
    fprintf(ouf, "</housefile>\n");
    fclose(ouf);

    return true;
}

bool
load_house_room(struct house *house, xmlNodePtr roomNode)
{
    room_num number = xmlGetIntProp(roomNode, "number", -1);
    struct room_data *room = real_room(number);

    if (room == NULL) {
        errlog("House %d has invalid room: %d", house->id, number);
        return false;
    }

    SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE);
    add_house_room(house, number);
    for (xmlNodePtr node = roomNode->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "object")) {
            load_object_from_xml(NULL, NULL, room, node);
        }
    }
    extract_norents(room->contents);
    return true;
}

struct house *
load_house(const char *filename)
{
    struct house *house;
    int axs = access(filename, W_OK);

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml house file '%s': %s",
                   filename, strerror(errno));
            return NULL;
        } else {
            return NULL;       // normal no eq file
        }
    }
    xmlDocPtr doc = xmlParseFile(filename);
    if (!doc) {
        errlog("XML parse error while loading %s", filename);
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", filename);
        return NULL;
    }

    xmlNodePtr houseNode;
    for (houseNode = root->xmlChildrenNode; houseNode;
         houseNode = houseNode->next) {
        if (xmlMatches(houseNode->name, "house")) {
            break;
        }
    }
    if (houseNode == NULL) {
        xmlFreeDoc(doc);
        errlog("XML house file %s has no house node.", filename);
        return NULL;
    }
    // read house node stuff
    int id = xmlGetIntProp(houseNode, "id", -1);
    if (id == -1 || find_house_by_idnum(id) != NULL) {
        errlog("Duplicate house id %d loaded from file %s.", id, filename);
        return NULL;
    }
    int owner_id = xmlGetIntProp(houseNode, "owner", -1);

    house = make_house(id, owner_id);

    char *typeName = (char *)xmlGetProp(houseNode, (const xmlChar *)"type");
    house->type = house_type_from_name(typeName);
    if (typeName != NULL) {
        free(typeName);
    }

    house->created = xmlGetLongProp(houseNode, "created", 0);
    house->landlord = xmlGetLongProp(houseNode, "landlord", -1);
    house->rental_rate = xmlGetIntProp(houseNode, "rate", 0);
    house->rent_overflow = xmlGetLongProp(houseNode, "rentOverflow", 0);

    for (xmlNodePtr node = houseNode->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "room")) {
            load_house_room(house, node);
        } else if (xmlMatches(node->name, "guest")) {
            int id = xmlGetIntProp(node, "id", -1);
            add_house_guest(house, id);
        } else if (xmlMatches(node->name, "repossession")) {
            char *note = (char *)xmlGetProp(node, (const xmlChar *)"note");
            struct txt_block *blk;

            CREATE(blk, struct txt_block, 1);
            blk->next = house->repo_notes;
            blk->text = strdup(tmp_trim(note));
            free(note);
            house->repo_notes = blk;
        }
    }

    xmlFreeDoc(doc);
    return house;
}

void
house_notify_repossession(struct house *house, struct creature *ch)
{
    struct obj_data *note;

    if (repo_note_count(house) == 0) {
        return;
    }

    note = read_object(MAIL_OBJ_VNUM);
    if (!note) {
        errlog("Failed to read object MAIL_OBJ_VNUM (#%d)", MAIL_OBJ_VNUM);
        return;
    }
    // Build the repossession note
    struct str_builder sb = str_builder_default;
    sb_strcat(&sb, "The following items were sold at auction to cover your back rent:\r\n\r\n",
        NULL);
    for (struct txt_block *note = house->repo_notes; note; note = note->next) {
        sb_strcat(&sb, note->text, "\r\n", NULL);
    }

    sb_strcat(&sb, "\r\n\r\nSincerely,\r\n    The Management\r\n", NULL);

    note->action_desc = strdup(sb.str);
    note->plrtext_len = strlen(note->action_desc) + 1;
    obj_to_char(note, ch);
    send_to_char(ch,
                 "The TempusMUD Landlord gives you a letter detailing your bill.\r\n");
    clear_repo_notes(house);
    save_house(house);
}

struct house *
create_house(int owner, room_num firstRoom, room_num lastRoom)
{
    int id = 0;
    int i = 0;
    struct room_data *room = real_room(firstRoom);

    if (!room) {
        return NULL;
    }

    for (GList *it = houses; it; it = it->next) {
        struct house *house = it->data;

        if (house->id > id) {
            id = house->id;
        }
    }
    id++;

    struct house *house = make_house(id, owner);

    SET_BIT(ROOM_FLAGS((room)), ROOM_HOUSE);
    for (i = firstRoom; i <= lastRoom; i++) {
        struct room_data *room = real_room(i);

        if (room != NULL) {
            add_house_room(house, room->number);
            SET_BIT(ROOM_FLAGS((room)), ROOM_HOUSE);
        }
    }
    houses = g_list_prepend(houses, house);
    save_house(house);

    return house;
}

void
save_houses(void)
{
    if (!houses) {
        return;
    }

    slog("HOUSE: Saving %d houses.", g_list_length(houses));
    for (GList *i = houses; i; i = i->next) {
        struct house *house = (struct house *)i->data;
        if (!save_house(house)) {
            errlog("Failed to save house %d.", house->id);
        }
    }
}

void
load_houses(void)
{
    DIR *dir;
    struct dirent *file;
    char *dirname;

    slog("Loading player houses");
    for (int i = 0; i <= 9; i++) {
        // If we don't have
        dirname = tmp_sprintf("players/housing/%d", i);
        dir = opendir(dirname);
        if (!dir) {
            mkdir(dirname, 0644);
            dir = opendir(dirname);
            if (!dir) {
                errlog("Couldn't open or create directory %s", dirname);
                safe_exit(-1);
            }
        }
        while ((file = readdir(dir)) != NULL) {
            // Check for correct filename (*.dat)
            if (!rindex(file->d_name, '.')) {
                continue;
            }
            if (strcmp(rindex(file->d_name, '.'), ".dat")) {
                continue;
            }

            char *filename = tmp_sprintf("%s/%s", dirname, file->d_name);
            struct house *house = load_house(filename);
            if (house) {
                houses = g_list_prepend(houses, house);
            } else {
                errlog("Failed to load house file: %s ", filename);
            }

        }
        closedir(dir);
    }

    // Now we preload the accounts that are attached to the house
    extern bool production_mode;

    if (production_mode && houses != NULL) {
        struct str_builder sb = str_builder_default;
        sb_sprintf(&sb, "idnum in (");
        bool first = true;
        for (GList *i = houses; i; i = i->next) {
            struct house *house = (struct house *)i->data;
            if (house->owner_id &&
                (house->type == PRIVATE || house->type == RENTAL)) {
                if (first) {
                    first = false;
                } else {
                    sb_strcat(&sb, ",", NULL);
                }
                sb_sprintf(&sb, "%d", house->owner_id);
            }
        }
        sb_strcat(&sb, ")", NULL);

        slog("Preloading accounts with houses...");
        preload_accounts(sb.str);
    }
}

int
room_rent_cost(struct house *house __attribute__((unused)), struct room_data *room)
{
    if (room == NULL) {
        return 0;
    }
    int room_count = count_objects_in_room(room);
    int room_sum = 0;

    for (struct obj_data *obj = room->contents; obj; obj = obj->next_content) {
        room_sum += recurs_obj_cost(obj, false, NULL);
    }
    if (room_count > MAX_HOUSE_ITEMS) {
        room_sum *= (room_count / MAX_HOUSE_ITEMS) + 1;
    }
    return room_sum;
}

gint
creature_is_pc(struct creature *ch, gpointer ignore __attribute__((unused)))
{
    return (IS_PC(ch)) ? 0 : -1;
}

int
house_rent_cost(struct house *house)
{
    int room_count = 0;
    int sum = 0;
    for (GList *i = house->rooms; i; i = i->next) {
        struct room_data *room = real_room(GPOINTER_TO_INT(i->data));

        if (!g_list_find_custom(room->people, NULL, (GCompareFunc) creature_is_pc)) {
            sum += room_rent_cost(house, room);
        }

        room_count++;
    }

    if (house->type == RENTAL) {
        sum += house->rental_rate * room_count;
    }

    return sum;
}

int
reconcile_clan_collection(struct house *house, int cost)
{
    struct clan_data *clan = real_clan(house->owner_id);
    if (clan == NULL) {
        return 0;
    }
    // First we get as much gold as we can
    if (cost < clan->bank_account) {
        clan->bank_account -= cost;
        cost = 0;
    } else {
        cost -= clan->bank_account;
        clan->bank_account = 0;
    }
    sql_exec("update clans set bank=%" PRId64 " where idnum=%d",
             clan->bank_account, clan->number);
    return cost;
}

int
reconcile_private_collection(struct house *house, int cost)
{
    struct account *account = account_by_idnum(house->owner_id);

    if (!account) {
        return false;
    }

    // First we get as much gold as we can
    if (cost < account->bank_past) {
        withdraw_past_bank(account, cost);
        cost = 0;
    } else {
        cost -= account->bank_past;
        account_set_past_bank(account, 0);
    }

    // If they didn't have enough, try credits
    if (cost > 0) {
        if (cost < account->bank_future) {
            withdraw_future_bank(account, cost);
            cost = 0;
        } else {
            cost -= account->bank_future;
            account_set_future_bank(account, 0);
        }
    }
    return cost;
}

int
reconcile_rent_collection(struct house *house, int cost)
{
    switch (house->type) {
    case PUBLIC:
        return 0;
    case RENTAL:
    case PRIVATE:
        return reconcile_private_collection(house, cost);
    case CLAN:
        return reconcile_clan_collection(house, cost);
    default:
        return 0;
    }
}

struct obj_data *
find_costliest_obj_in_room(struct room_data *room)
{
    struct obj_data *result = NULL;
    struct obj_data *cur_obj = room->contents;
    while (cur_obj) {
        if (cur_obj && (!result
                        || GET_OBJ_COST(result) < GET_OBJ_COST(cur_obj))) {
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

// collects the house's rent, selling off items to meet the
// bill, if necessary.
bool
collect_house_rent(struct house *house, int cost)
{

    if (cost < house->rent_overflow) {
        slog("HOUSE: [%d] Previous repossessions covering %d rent.", house->id,
             cost);
        house->rent_overflow -= cost;
        return false;
    }

    cost -= house->rent_overflow;
    reconcile_rent_collection(house, cost);
    return false;
}

void
collect_housing_rent(void)
{
    extern bool production_mode;

    if (production_mode) {
        last_house_collection = time(NULL);

        for (GList *i = houses; i; i = i->next) {
            struct house *house = (struct house *)i->data;
            if (house->type == PUBLIC) {
                continue;
            }
            if (house->type == PRIVATE || house->type == RENTAL) {
                // If the player is online, do not charge rent.
                struct account *account = account_by_idnum(house->owner_id);
                if (account && account_is_logged_in(account)) {
                    continue;
                }
            }
            // Cost per minute
            int cost = (int)((house_rent_cost(house) / 24.0) / 60);

            collect_house_rent(house, cost);
            save_house(house);
        }
    }
}

void
list_house_guests(struct house *house, struct creature *ch)
{
    if (!house->guests) {
        send_to_char(ch, "No guests defined.\r\n");
        return;
    }

    struct str_builder sb = str_builder_default;
    sb_strcat(&sb, "     Guests: ", NULL);
    for (GList *i = house->guests; i; i = i->next) {
        sb_sprintf(&sb, "%s ", player_name_by_idnum(GPOINTER_TO_INT(i->data)));
    }
    sb_strcat(&sb, "\r\n", NULL);
    page_string(ch->desc, sb.str);
}

void
list_house_rooms(struct house *house, struct creature *ch, bool show_contents)
{
    struct str_builder sb = str_builder_default;
    for (GList *i = house->rooms; i; i = i->next) {
        struct room_data *room = real_room(GPOINTER_TO_INT(i->data));

        if (room) {
            sb_strcat(&sb, print_room_contents(ch, room, show_contents), NULL);
        } else {
            errlog("Room [%5d] of House [%5d] does not exist.",
                   GPOINTER_TO_INT(i->data), house->id);
        }
    }
    page_string(ch->desc, sb.str);
}

void
display_house(struct house *house, struct creature *ch)
{

    const char *landlord;
    char created_buf[30];

    landlord = player_name_by_idnum(house->landlord);
    if (!landlord) {
        landlord = "NONE";
    }

    d_printf(ch->desc,
             "&yHouse[&n%4d&y]  Type:&n %s  &yLandlord:&n %s\r\n", house->id,
             house_type_name(house->type), landlord);

    if (house->type == PRIVATE || house->type == RENTAL
        || house->type == PUBLIC) {
        struct account *account = account_by_idnum(house->owner_id);
        if (account != NULL) {
            const char *email = "";
            if (account->email && *account->email) {
                email = account->email;
            }
            d_printf(ch->desc, "&yOwner:&n %s [%d] &c%s&n\r\n",
                     account->name, account->id, email);
        }
    } else if (house->type == CLAN) {
        struct clan_data *clan = real_clan(house->owner_id);
        if (clan == NULL) {
            d_printf(ch->desc, "&yOwned by Clan:&n NONE\r\n");
        } else {
            d_printf(ch->desc, "&yOwned by Clan:&c %s&n [%d]\r\n",
                     clan->name, clan->number);
        }
    } else {
        d_printf(ch->desc, "&yOwner:&n NONE\r\n");
    }

    strftime(created_buf, 29, "%a %b %d, %Y %H:%M:%S",
             localtime(&house->created));
    d_printf(ch->desc, "&yCreated:&n %s\r\n", created_buf);
    list_house_guests(house, ch);
    list_house_rooms(house, ch, false);
    if (house->repo_notes) {
        d_printf(ch->desc, "&cRepossession Notifications:&n \r\n");
        for (struct txt_block *i = house->repo_notes; i; i = i->next) {
            d_printf(ch->desc, "    %s\r\n", i->text);
        }
    }
}

char *
print_room_contents(struct creature *ch, struct room_data *real_house_room,
                    bool showContents)
{
    int count;
    char *buf = NULL;
    const char *buf2 = "";

    struct obj_data *obj, *cont;
    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
        buf2 = tmp_sprintf(" %s[%s%5d%s]%s", CCGRN(ch, C_NRM),
                           CCNRM(ch, C_NRM), real_house_room->number,
                           CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    }

    buf = tmp_sprintf("Room%s: %s%s%s\r\n", buf2, CCCYN(ch, C_NRM),
                      real_house_room->name, CCNRM(ch, C_NRM));
    if (!showContents) {
        return buf;
    }
    for (obj = real_house_room->contents; obj; obj = obj->next_content) {
        count = recurs_obj_contents(obj, NULL) - 1;
        buf2 = "\r\n";
        if (count > 0) {
            buf2 = tmp_sprintf("     (contains %d)\r\n", count);
        }
        buf = tmp_sprintf("%s   %s%-35s%s  %10d Au%s", buf, CCGRN(ch, C_NRM),
                          obj->name, CCNRM(ch, C_NRM),
                          recurs_obj_cost(obj, false, NULL), buf2);
        if (obj->contains) {
            for (cont = obj->contains; cont; cont = cont->next_content) {
                count = recurs_obj_contents(cont, obj) - 1;
                buf2 = "\r\n";
                if (count > 0) {
                    buf2 = tmp_sprintf("     (contains %d)\r\n", count);
                }
                buf =
                    tmp_sprintf("%s     %s%-33s%s  %10d Au%s", buf, CCGRN(ch,
                                                                          C_NRM), cont->name, CCNRM(ch, C_NRM),
                                recurs_obj_cost(cont, false, obj), buf2);
            }
        }
    }
    return buf;
}

void
hcontrol_build_house(struct creature *ch, char *arg)
{
    char *str;
    room_num virt_top_room, virt_atrium;
    struct room_data *real_atrium;
    int owner;

    // FIRST arg: player's name
    str = tmp_getword(&arg);
    if (!*str) {
        send_to_char(ch, HCONTROL_FORMAT);
        return;
    }
    if (is_number(str)) {
        int id = atoi(str);
        if (id == 0) {
            send_to_char(ch, "Warning, creating house with no owner.\r\n");
        } else if (!account_by_idnum(id)) {
            send_to_char(ch, "Invalid account id '%d'.\r\n", id);
            return;
        }
        owner = id;
    } else {
        if (!player_idnum_by_name(str)) {
            send_to_char(ch, "Unknown player '%s'.\r\n", str);
            return;
        }
        owner = player_account_by_name(str);
    }

    // SECOND arg: the first room of the house
    str = tmp_getword(&arg);
    if (!*str) {
        send_to_char(ch,
                     "You have to give a beginning room for the range.\r\n");
        send_to_char(ch, HCONTROL_FORMAT);
        return;
    } else if (!is_number(str)) {
        send_to_char(ch, "The beginning room must be a number: %s\r\n", str);
        return;
    }
    virt_atrium = atoi(str);
    struct house *house = find_house_by_room(virt_atrium);
    if (house != NULL) {
        send_to_char(ch, "Room [%d] already exists as room of house [%d]\r\n",
                     virt_atrium, house->id);
        return;
    }
    if ((real_atrium = real_room(virt_atrium)) == NULL) {
        send_to_char(ch, "No such room exists.\r\n");
        return;
    }
    // THIRD arg: Top room of the house.
    // Inbetween rooms will be added
    str = tmp_getword(&arg);
    if (!*str) {
        send_to_char(ch, "You have to give an ending room for the range.\r\n");
        send_to_char(ch, HCONTROL_FORMAT);
        return;
    } else if (!is_number(str)) {
        send_to_char(ch, "The ending room must be a number: %s\r\n", str);
        return;
    }
    virt_top_room = atoi(str);
    if (virt_top_room < virt_atrium) {
        send_to_char(ch,
                     "Top room number is less than Atrium room number.\r\n");
        return;
    } else if (virt_top_room - virt_atrium > 20) {
        send_to_char(ch, "Don't be greedy! Who needs more than 20 rooms?\r\n");
        return;
    }
    struct house *h = find_house_by_owner(owner);
    if (h != NULL) {
        send_to_char(ch, "Account %d already owns house %d.\r\n", owner,
                     h->id);
    } else if ((h = create_house(owner, virt_atrium, virt_top_room)) != NULL) {
        send_to_char(ch, "House built.  Mazel tov!\r\n");
        h->landlord = GET_IDNUM(ch);
        save_house(h);
        slog("HOUSE: %s created house %d for account %d with first room %d.",
             GET_NAME(ch), h->id, owner, GPOINTER_TO_INT(h->rooms->data));
    } else {
        send_to_char(ch, "House build failed.\r\n");
    }
}

void
hcontrol_destroy_house(struct creature *ch, char *arg)
{
    if (!*arg) {
        send_to_char(ch, HCONTROL_FORMAT);
        return;
    }
    struct house *house = find_house_by_idnum(atoi(arg));
    if (house == NULL) {
        send_to_char(ch, "Someone already destroyed house %d.\r\n", atoi(arg));
        return;
    }

    if (!is_authorized(ch, EDIT_HOUSE, house)) {
        send_to_char(ch, "You cannot edit that house.\r\n");
        return;
    }

    if (destroy_house(house)) {
        send_to_char(ch, "House %d destroyed.\r\n", atoi(arg));
        slog("HOUSE: %s destroyed house %d.", GET_NAME(ch), atoi(arg));
    } else {
        send_to_char(ch, "House %d failed to die.\r\n", atoi(arg));
    }
}

void
set_house_clan_owner(struct creature *ch, struct house *house, char *arg)
{
    int clanID = 0;
    if (is_number(arg)) {       // to clan id
        struct clan_data *clan = real_clan(atoi(arg));
        if (clan != NULL) {
            clanID = atoi(arg);
        } else {
            send_to_char(ch, "Clan %d doesn't exist.\r\n", atoi(arg));
            return;
        }
    } else {
        send_to_char(ch,
                     "When setting a clan owner, the clan ID must be used.\r\n");
        return;
    }
    // An account may only own one house
    struct house *h = find_house_by_owner(clanID);
    if (h != NULL) {
        send_to_char(ch, "Clan %d already owns house %d.\r\n", clanID, h->id);
        return;
    }

    house->owner_id = clanID;
    send_to_char(ch, "Owner set to clan %d.\r\n", house->owner_id);
    slog("HOUSE: Owner of house %d set to clan %d by %s.",
         house->id, house->owner_id, GET_NAME(ch));
}

void
set_house_account_owner(struct creature *ch, struct house *house, char *arg)
{
    int accountID = 0;
    if (isdigit(*arg)) {        // to an account id
        if (account_by_idnum(atoi(arg))) {
            accountID = atoi(arg);
        } else {
            send_to_char(ch, "Account %d doesn't exist.\r\n",
                         atoi(arg));
            return;
        }
    } else {                    // to a player name
        if (player_name_exists(arg)) {
            accountID = player_account_by_name(arg);
        } else {
            send_to_char(ch, "There is no such player, '%s'\r\n", arg);
            return;
        }
    }
    // An account may only own one house
    struct house *h = find_house_by_owner(accountID);
    if (h != NULL) {
        send_to_char(ch, "Account %d already owns house %d.\r\n",
                     accountID, h->id);
        return;
    }

    house->owner_id = accountID;
    send_to_char(ch, "Owner set to account %d.\r\n", house->owner_id);
    slog("HOUSE: Owner of house %d set to account %d by %s.",
         house->id, house->owner_id, GET_NAME(ch));

}

void
hcontrol_set_house(struct creature *ch, char *arg)
{
    char *house_id_str = tmp_getword(&arg);
    char *field = tmp_getword(&arg);

    if (!*house_id_str || !*field) {
        send_to_char(ch, HCONTROL_FORMAT);
        return;
    }

    if (!isdigit(*house_id_str)) {
        send_to_char(ch, "The house id must be a number.\r\n");
        return;
    }

    struct house *house = find_house_by_idnum(atoi(house_id_str));
    if (house == NULL) {
        send_to_char(ch, "House id %d not found.\r\n", atoi(house_id_str));
        return;
    }
    if (!is_authorized(ch, EDIT_HOUSE, house)) {
        send_to_char(ch, "You cannot edit that house.\r\n");
        return;
    }

    if (is_abbrev(field, "rate")) {
        if (!*arg) {
            send_to_char(ch, "Set rental rate to what?\r\n");
        } else {
            char *rate_str = tmp_getword(&arg);
            house->rental_rate = atoi(rate_str);
            send_to_char(ch, "House %d rental rate set to %d/day.\r\n",
                         house->id, house->rental_rate);
            slog("HOUSE: Rental rate of house %d set to %d by %s.",
                 house->id, house->rental_rate, GET_NAME(ch));
        }
    } else if (is_abbrev(field, "type")) {
        if (!*arg) {
            send_to_char(ch, "Set mode to public, private, or rental?\r\n");
            return;
        } else if (is_abbrev(arg, "public")) {
            if (house->type != PUBLIC) {
                house->type = PUBLIC;
                house->owner_id = 0;
            }
        } else if (is_abbrev(arg, "private")) {
            if (house->type != PRIVATE) {
                if (house->type != RENTAL) {
                    house->owner_id = 0;
                }
                house->type = PRIVATE;
            }
        } else if (is_abbrev(arg, "rental")) {
            if (house->type != RENTAL) {
                if (house->type != PRIVATE) {
                    house->owner_id = 0;
                }
                house->type = RENTAL;
            }
        } else if (is_abbrev(arg, "clan")) {
            if (house->type != CLAN) {
                house->type = CLAN;
            }
        } else {
            send_to_char(ch,
                         "You must specify public, private, clan, or rental!!\r\n");
            return;
        }
        send_to_char(ch, "House %d type set to %s.\r\n",
                     house->id, house_type_name(house->type));
        slog("HOUSE: Type of house %d set to %s by %s.",
             house->id, house_type_name(house->type), GET_NAME(ch));

    } else if (is_abbrev(field, "landlord")) {
        if (!*arg) {
            send_to_char(ch, "Set who as the landlord?\r\n");
            return;
        }
        char *landlord = tmp_getword(&arg);
        if (!player_name_exists(landlord)) {
            send_to_char(ch, "There is no such player, '%s'\r\n", arg);
            return;
        }

        house->landlord = player_idnum_by_name(landlord);

        send_to_char(ch, "Landlord of house %d set to %s.\r\n",
                     house->id, player_name_by_idnum(house->landlord));
        slog("HOUSE: Landlord of house %d set to %s by %s.",
             house->id, player_name_by_idnum(house->landlord), GET_NAME(ch));
        return;
    } else if (is_abbrev(field, "owner")) {
        if (!*arg) {
            send_to_char(ch, "To whom would you like to set the house owner?\r\n");
            return;
        }
        char *owner = tmp_getword(&arg);
        switch (house->type) {
        case PRIVATE:
        case PUBLIC:
        case RENTAL:
            set_house_account_owner(ch, house, owner);
            break;
        case CLAN:
            set_house_clan_owner(ch, house, owner);
            break;
        case INVALID:
            send_to_char(ch, "Invalid house type. Nothing set.\r\n");
            break;
        }

        return;
    }
    save_house(house);
}

void
hcontrol_where_house(struct creature *ch)
{
    struct house *h = find_house_by_room(ch->in_room->number);

    if (h == NULL) {
        send_to_char(ch, "You are not in a house.\r\n");
        return;
    }

    send_to_char(ch,
                 "You are in house id: %s[%s%6d%s]%s\n"
                 "              Owner: %d\n",
                 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                 h->id, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), h->owner_id);
}

bool
reload_house(struct house *house)
{
    if (house == NULL) {
        return false;
    }

    char *path = tmp_strcat(get_house_file_path(house->id), ".reload", NULL);
    int axs = access(path, R_OK);
    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml house file '%s' for reload: %s",
                   path, strerror(errno));
            return false;
        } else {
            errlog("Unable to open xml house file '%s' for reload.", path);
            return false;
        }
    }

    for (GList *i = house->rooms; i; i = i->next) {
        int room_num = GPOINTER_TO_INT(i->data);
        struct room_data *room = real_room(room_num);
        for (struct obj_data *obj = room->contents; obj; ) {
            struct obj_data *next_o = obj->next_content;
            extract_obj(obj);
            obj = next_o;
        }
    }

    destroy_house(house);
    house = load_house(path);
    if (house) {
        houses = g_list_prepend(houses, house);
        slog("HOUSE: Reloaded house %d", house->id);
        return true;
    } else {
        errlog("Failed to reload house file: %s ", path);
        return false;
    }
}

void
hcontrol_reload_house(struct creature *ch, char *arg)
{
    const char *arg1 = tmp_getword(&arg);
    struct house *h = NULL;

    if (arg1 == NULL || *arg1 == '\0') {
        if (!(h = find_house_by_room(ch->in_room->number))) {
            send_to_char(ch, "You are not in a house.\r\n");
            return;
        }
    } else {
        if (!atoi(arg1)) {
            send_to_char(ch, HCONTROL_FORMAT);
            return;
        }
        if (!(h = find_house_by_idnum(atoi(arg1)))) {
            send_to_char(ch, "House id: [%d] does not exist\r\n", atoi(arg1));
            return;
        }
    }
    if (reload_house(h)) {
        send_to_char(ch, "Reload complete. It might even have worked.\r\n");
    } else {
        send_to_char(ch,
                     "Reload failed. Figure it out yourself oh great coder.\r\n");
    }
}

/* Misc. administrative functions */

void
hcontrol_add_to_house(struct creature *ch, char *arg)
{
    // first arg is the atrium vnum
    char *str = tmp_getword(&arg);
    if (!*str || !*arg) {
        send_to_char(ch, HCONTROL_ADD_FORMAT);
        return;
    }

    struct house *house = find_house_by_idnum(atoi(str));

    if (house == NULL) {
        send_to_char(ch, "No such house exists with id %d.\r\n", atoi(str));
        return;
    }

    if (!is_authorized(ch, EDIT_HOUSE, house)) {
        send_to_char(ch, "You cannot edit that house.\r\n");
        return;
    }
    // turn arg into the final argument and arg1 into the room/guest/owner arg
    str = tmp_getword(&arg);

    if (!*str) {
        send_to_char(ch, HCONTROL_ADD_FORMAT);
        return;
    }

    int roomID = atoi(str);
    struct room_data *room = real_room(roomID);
    if (room == NULL) {
        send_to_char(ch, "No such room exists.\r\n");
        return;
    }
    struct house *h = find_house_by_room(roomID);
    if (h != NULL) {
        send_to_char(ch, "Room [%d] is already part of house [%d]\r\n",
                     roomID, h->id);
        return;
    }
    add_house_room(house, roomID);
    SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE);
    send_to_char(ch, "Room %d added to house %d.\r\n", roomID, house->id);
    slog("HOUSE: Room %d added to house %d by %s.",
         roomID, house->id, GET_NAME(ch));
    save_house(house);
}

void
hcontrol_delete_from_house(struct creature *ch, char *arg)
{

    // first arg is the atrium vnum
    char *str = tmp_getword(&arg);
    if (!*str || !*arg) {
        send_to_char(ch, HCONTROL_DELETE_FORMAT);
        return;
    }

    struct house *house = find_house_by_idnum(atoi(str));

    if (house == NULL) {
        send_to_char(ch, "No such house exists with id %d.\r\n", atoi(str));
        return;
    }

    if (!is_authorized(ch, EDIT_HOUSE, house)) {
        send_to_char(ch, "You cannot edit that house.\r\n");
        return;
    }
    // turn arg into the final argument and arg1 into the room/guest/owner arg
    str = tmp_getword(&arg);

    if (!*str) {
        send_to_char(ch, HCONTROL_DELETE_FORMAT);
        return;
    }
    // delete room
    int roomID = atoi(str);
    struct room_data *room = real_room(roomID);
    if (room == NULL) {
        send_to_char(ch, "No such room exists.\r\n");
        return;
    }
    struct house *h = find_house_by_room(roomID);
    if (h == NULL) {
        send_to_char(ch, "Room [%d] isn't a room of any house!\r\n",
                     roomID);
        return;
    } else if (house != h) {
        send_to_char(ch, "Room [%d] belongs to house [%d]!\r\n",
                     roomID, h->id);
        return;
    } else if (!house->rooms->next) {
        send_to_char(ch, "Room %d is the last room in house [%d]. Destroy it instead.\r\n",
                     roomID, h->id);
        return;
    }

    remove_house_room(house, roomID);
    REMOVE_BIT(ROOM_FLAGS(room), ROOM_HOUSE | ROOM_HOUSE_CRASH);
    send_to_char(ch, "Room %d removed from house %d.\r\n",
                 roomID, house->id);
    slog("HOUSE: Room %d removed from house %d by %s.",
         roomID, house->id, GET_NAME(ch));
    save_house(house);
}

GList *
match_houses(GList *houses, int mode, const char *arg)
{
    GList *result = NULL;
    long id;
    switch (mode) {
    case HC_OWNER:
        if (isdigit(*arg)) {
            id = atoi(arg);
        } else {
            id = player_account_by_name(arg);
        }
        for (GList *i = houses; i; i = i->next) {
            if (((struct house *)i->data)->owner_id == id) {
                result = g_list_prepend(result, i->data);
            }
        }
        break;
    case HC_LANDLORD:
        if (isdigit(*arg)) {
            id = atoi(arg);
        } else {
            id = player_account_by_name(arg);
        }
        for (GList *i = houses; i; i = i->next) {
            if (((struct house *)i->data)->landlord == id) {
                result = g_list_prepend(result, i->data);
            }
        }
        break;
    case HC_GUEST:
        if (isdigit(*arg)) {
            id = atoi(arg);
        } else {
            id = player_idnum_by_name(arg);
        }
        for (GList *i = houses; i; i = i->next) {
            if (is_house_guest(((struct house *)i->data), id)) {
                result = g_list_prepend(result, i->data);
            }
        }
        break;
    default:
        raise(SIGSEGV);
    }
    return result;

}
void
display_houses(GList *houses, struct creature *ch)
{
    struct str_builder sb = str_builder_default;

    sb_strcat(&sb, "ID   Size Owner  Landlord   Type Rooms\r\n"
               "---- ---- ------ ---------- ---- -----"
               "-----------------------------------------\r\n", NULL);

    for (GList *cur = houses; cur; cur = cur->next) {
        struct house *house = (struct house *)cur->data;
        const char *landlord = "none";
        if (player_idnum_exists(house->landlord)) {
            landlord = player_name_by_idnum(house->landlord);
        }
        sb_sprintf(&sb, "%4d %4d %6d %-10s %4s ",
                    house->id,
                    g_list_length(house->rooms),
                    house->owner_id, landlord, house_type_short_name(house->type));
        if (house->rooms) {
            sb_sprintf(&sb, "%d", GPOINTER_TO_INT(house->rooms->data));
            for (GList *i = house->rooms; i; i = i->next) {
                sb_sprintf(&sb, ", %d", GPOINTER_TO_INT(i->data));
            }
        }
        sb_strcat(&sb, "\r\n", NULL);
    }
    page_string(ch->desc, sb.str);
}

void
hcontrol_find_houses(struct creature *ch, char *arg)
{
    char *token;

    if (arg == NULL || *arg == '\0') {
        send_to_char(ch, HCONTROL_FIND_FORMAT);
        return;
    }

    GList *houses_left = g_list_copy(houses);
    GList *matched = NULL;

    while (*arg && houses_left) {
        token = tmp_getword(&arg);
        if (strcmp(token, "owner") == 0 && *arg) {
            token = tmp_getword(&arg);
            matched = match_houses(houses_left, HC_OWNER, token);
        } else if (strcmp(token, "landlord") == 0 && *arg) {
            token = tmp_getword(&arg);
            matched = match_houses(houses_left, HC_LANDLORD, token);
        } else if (strcmp(token, "guest") == 0 && *arg) {
            token = tmp_getword(&arg);
            matched = match_houses(houses_left, HC_GUEST, token);
        } else {
            send_to_char(ch, HCONTROL_FIND_FORMAT);
            return;
        }
        g_list_free(houses_left);
        houses_left = matched;
    }
    if (!houses_left) {
        send_to_char(ch, "No houses found.\r\n");
        return;
    }
    display_houses(houses, ch);
}

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
    char *action_str;

    if (!is_authorized(ch, EDIT_HOUSE, NULL)) {
        send_to_char(ch, "You aren't able to edit houses!\r\n");
        return;
    }

    action_str = tmp_getword(&argument);

    if (is_abbrev(action_str, "save")) {
        save_houses();
        send_to_char(ch, "Saved.\r\n");
        slog("HOUSE: Saved by %s.", GET_NAME(ch));
    } else if (is_abbrev(action_str, "build")) {
        hcontrol_build_house(ch, argument);
    } else if (is_abbrev(action_str, "destroy")) {
        hcontrol_destroy_house(ch, argument);
    } else if (is_abbrev(action_str, "add")) {
        hcontrol_add_to_house(ch, argument);
    } else if (is_abbrev(action_str, "delete")) {
        hcontrol_delete_from_house(ch, argument);
    } else if (is_abbrev(action_str, "set")) {
        hcontrol_set_house(ch, argument);
    } else if (is_abbrev(action_str, "find")) {
        hcontrol_find_houses(ch, argument);
    } else if (is_abbrev(action_str, "where")) {
        hcontrol_where_house(ch);
    } else if (is_abbrev(action_str, "show")) {
        if (!*argument) {
            display_houses(houses, ch);
        } else {
            char *str = tmp_getword(&argument);
            if (isdigit(*str)) {
                struct house *house = find_house_by_idnum(atoi(str));
                if (house == NULL) {
                    send_to_char(ch, "No such house %d.\r\n", atoi(str));
                    return;
                }
                display_house(house, ch);
            } else {
                send_to_char(ch, HCONTROL_SHOW_FORMAT);
            }
        }
    } else {
        send_to_char(ch, HCONTROL_FORMAT);
    }
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
    int found = false;
    char *action_str;
    struct house *house = NULL;
    action_str = tmp_getword(&argument);

    if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_HOUSE)) {
        send_to_char(ch, "You must be in your house to set guests.\r\n");
        return;
    }
    house = find_house_by_room(ch->in_room->number);

    if (house == NULL) {
        send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
        return;
    }

    if (!is_authorized(ch, EDIT_HOUSE, house)) {
        send_to_char(ch, "Only the owner can set guests.\r\n");
        return;
    }
    // No arg, list the guests
    if (!*action_str) {
        send_to_char(ch, "Guests of your house:\r\n");
        if (!house->guests) {
            send_to_char(ch, "  None.\r\n");
        } else {
            GList *j;
            int col;
            for (j = house->guests, col = 0; j; j = j->next, col++) {
                send_to_char(ch, "%-19s",
                             player_name_by_idnum(GPOINTER_TO_INT(j->data)));
                if (!((col + 1) % 4)) {
                    send_to_char(ch, "\r\n");
                }
            }
            if (col % 4) {
                send_to_char(ch, "\r\n");
            }
        }
        return;
    }

    if (!player_name_exists(action_str)) {
        send_to_char(ch, "No such player.\r\n");
        return;
    }
    int playerID = player_idnum_by_name(action_str);

    if (is_house_guest(house, playerID)) {
        remove_house_guest(house, playerID);
        send_to_char(ch, "Guest deleted.\r\n");
        return;
    }

    int accountID = player_account_by_idnum(playerID);
    if (house->owner_id == accountID) {
        send_to_char(ch, "They already own the house.\r\n");
        return;
    }

    if (g_list_length(house->guests) == MAX_HOUSE_GUESTS) {
        send_to_char(ch,
                     "Sorry, you have the maximum number of guests already.\r\n");
        return;
    }

    if (add_house_guest(house, playerID)) {
        send_to_char(ch, "Guest added.\r\n");
    }
    // delete any bogus ids
    for (GList *i = house->guests; i; i = i->next) {
        if (!player_idnum_exists(GPOINTER_TO_INT(i->data))) {
            remove_house_guest(house, GPOINTER_TO_INT(i->data));
            i = house->guests;
            found++;
        }
    }

    if (found > 0) {
        send_to_char(ch,
                     "%d bogus guest%s deleted.\r\n", found, found == 1 ? "" : "s");
    }
    save_house(house);
}
