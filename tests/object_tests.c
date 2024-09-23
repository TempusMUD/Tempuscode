#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>
#include <check.h>

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
#include "players.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"
#include "zone_data.h"

extern int current_mob_idnum;
extern GHashTable *rooms;
extern GHashTable *mob_prototypes;
extern GHashTable *obj_prototypes;

extern GList *creatures;
extern GHashTable *creature_map;

static struct creature *ch = NULL;
static struct zone_data *zone = NULL;
static struct room_data *room_a = NULL, *room_b = NULL;

void
fixture_object_setup(void)
{
    rooms = g_hash_table_new(g_direct_hash, g_direct_equal);
    mob_prototypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    obj_prototypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    creature_map = g_hash_table_new(g_direct_hash, g_direct_equal);

    ch = make_creature(false);

    ch->points.hit = ch->points.max_hit = 100;
    ch->player.name = strdup("testmob");
    ch->player.short_descr = strdup("testmob");

    CREATE(ch->mob_specials.shared, struct mob_shared_data, 1);
    ch->mob_specials.shared->vnum = 1;
    ch->mob_specials.shared->number = 0;
    ch->mob_specials.shared->func = NULL;
    ch->mob_specials.shared->move_buf = NULL;
    ch->mob_specials.shared->proto = ch;

    NPC_IDNUM(ch) = (++current_mob_idnum);
    creatures = g_list_prepend(creatures, ch);
    g_hash_table_insert(creature_map, GINT_TO_POINTER(-NPC_IDNUM(ch)), ch);

    zone = make_zone(1);
    room_a = make_room(zone, 1);
    room_b = make_room(zone, 2);
    link_rooms(room_a, room_b, NORTH);
    char_to_room(ch, room_a, false);
}

START_TEST(test_obj_to_from_obj)
{
    struct obj_data *obj_a = make_object();
    struct obj_data *obj_b = make_object();
    GET_OBJ_WEIGHT(obj_a) = 10;
    GET_OBJ_WEIGHT(obj_b) = 15;

    obj_to_obj(obj_a, obj_b);
    ck_assert(obj_a->in_obj == obj_b);
    ck_assert(obj_b->contains == obj_a);
    ck_assert(GET_OBJ_WEIGHT(obj_b) == 25);

    obj_from_obj(obj_a);
    ck_assert(obj_a->in_obj == NULL);
    ck_assert(obj_b->contains == NULL);
    ck_assert(GET_OBJ_WEIGHT(obj_b) == 15);
}
END_TEST
START_TEST(test_obj_to_from_char)
{
    struct obj_data *obj_a = make_object();
    GET_OBJ_WEIGHT(obj_a) = 10;
    obj_to_char(obj_a, ch);
    ck_assert(obj_a->carried_by == ch);
    ck_assert(ch->carrying == obj_a);
    ck_assert(IS_CARRYING_W(ch) == 10);
    ck_assert(IS_CARRYING_N(ch) == 1);

    obj_from_char(obj_a);
    ck_assert(obj_a->carried_by == NULL);
    ck_assert(ch->carrying == NULL);
    ck_assert(IS_CARRYING_W(ch) == 0);
    ck_assert(IS_CARRYING_N(ch) == 0);
}
END_TEST
START_TEST(test_obj_to_from_carried)
{
    struct obj_data *obj_a = make_object();
    struct obj_data *obj_b = make_object();

    GET_OBJ_WEIGHT(obj_a) = 10;
    GET_OBJ_WEIGHT(obj_b) = 15;
    obj_to_char(obj_a, ch);

    obj_to_obj(obj_b, obj_a);
    ck_assert(obj_a->carried_by == ch);
    ck_assert(obj_b->in_obj == obj_a);
    ck_assert(ch->carrying == obj_a);
    ck_assert(IS_CARRYING_W(ch) == 25);
    ck_assert(IS_CARRYING_N(ch) == 1);

    obj_from_obj(obj_b);
    ck_assert(obj_a->carried_by == ch);
    ck_assert(obj_b->in_obj == NULL);
    ck_assert(ch->carrying == obj_a);
    ck_assert(IS_CARRYING_W(ch) == 10);
    ck_assert(IS_CARRYING_N(ch) == 1);
}
END_TEST

Suite *
object_suite(void)
{
    Suite *s = suite_create("object");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, tmp_string_cleanup);
    tcase_add_checked_fixture(tc_core, fixture_object_setup, NULL);
    tcase_add_test(tc_core, test_obj_to_from_obj);
    tcase_add_test(tc_core, test_obj_to_from_char);
    tcase_add_test(tc_core, test_obj_to_from_carried);
    suite_add_tcase(s, tc_core);

    return s;
}
