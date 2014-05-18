#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>
#include <check.h>
#include <math.h>

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
#include "screen.h"
#include "players.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"
#include "testing.h"
#include "spells.h"

static struct creature *ch = NULL;

struct creature *load_player_from_file(const char *path);
void save_player_to_file(struct creature *ch, const char *path);
bool save_player_objects_to_file(struct creature *ch, const char *path);
bool load_player_objects_from_file(struct creature *ch, const char *path);

void boot_tongues(const char *path);
void boot_spells(const char *path);

static void
fixture_make_player(void)
{
    ch = make_test_player("foo", "Foo");
}

static void
fixture_destroy_player(void)
{
    destroy_test_player(ch);
}

int
make_random_object(void)
{
    struct obj_data *obj;
    CREATE(obj, struct obj_data, 1);
    CREATE(obj->shared, struct obj_shared_data, 1);
    obj->shared->vnum = 3000;
    obj->shared->number = 0;
    obj->shared->house_count = 0;
    obj->shared->func = NULL;
    obj->shared->proto = obj;
    obj->shared->owner_id = 0;

    obj->name = strdup("test name");
    obj->aliases = strdup("test aliases");
    obj->line_desc = strdup("test line desc");
    obj->action_desc = strdup("test action desc");
    obj->soilage = 84732;
    obj->unique_id = 34827429;
    obj->creation_time = time(NULL);
    obj->creation_method = CREATED_IMM;
    obj->creator = 732;

    obj->worn_on = -1;

    obj->obj_flags.weight = 1.0;

    if (!obj_prototypes) {
        obj_prototypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
    g_hash_table_insert(obj_prototypes, GINT_TO_POINTER(GET_OBJ_VNUM(obj)), obj);

    return obj->shared->vnum;
}

void
compare_objects(struct obj_data *obj_a, struct obj_data *obj_b)
{
    fail_unless(!strcmp(obj_a->name, obj_b->name));
    fail_unless(!strcmp(obj_a->aliases, obj_b->aliases));
    fail_unless(!strcmp(obj_a->line_desc, obj_b->line_desc));
    fail_unless(!strcmp(obj_a->action_desc, obj_b->action_desc));
    fail_unless(obj_a->plrtext_len == obj_b->plrtext_len);
    fail_unless(obj_a->worn_on == obj_b->worn_on, "obj_a->worn_on = %d, obj_b->worn_on = %d", obj_a->worn_on, obj_b->worn_on);
    fail_unless(obj_a->soilage == obj_b->soilage);
    fail_unless(obj_a->unique_id == obj_b->unique_id);
    fail_unless(obj_a->creation_time == obj_b->creation_time);
    fail_unless(obj_a->creation_method == obj_b->creation_method);
    fail_unless(obj_a->creator == obj_b->creator);
    fail_unless(obj_a->obj_flags.value[0] == obj_b->obj_flags.value[0]);
    fail_unless(obj_a->obj_flags.value[1] == obj_b->obj_flags.value[1]);
    fail_unless(obj_a->obj_flags.value[2] == obj_b->obj_flags.value[2]);
    fail_unless(obj_a->obj_flags.value[3] == obj_b->obj_flags.value[3]);
    fail_unless(obj_a->obj_flags.type_flag == obj_b->obj_flags.type_flag);
    fail_unless(obj_a->obj_flags.wear_flags == obj_b->obj_flags.wear_flags);
    fail_unless(obj_a->obj_flags.extra_flags == obj_b->obj_flags.extra_flags);
    fail_unless(obj_a->obj_flags.extra2_flags == obj_b->obj_flags.extra2_flags);
    fail_unless(obj_a->obj_flags.extra3_flags == obj_b->obj_flags.extra3_flags);
    fail_unless(obj_a->obj_flags.weight == obj_b->obj_flags.weight);
    fail_unless(obj_a->obj_flags.timer == obj_b->obj_flags.timer);
    fail_unless(obj_a->obj_flags.bitvector[0] == obj_b->obj_flags.bitvector[0]);
    fail_unless(obj_a->obj_flags.bitvector[1] == obj_b->obj_flags.bitvector[1]);
    fail_unless(obj_a->obj_flags.bitvector[2] == obj_b->obj_flags.bitvector[2]);
    fail_unless(obj_a->obj_flags.material == obj_b->obj_flags.material);
    fail_unless(obj_a->obj_flags.max_dam == obj_b->obj_flags.max_dam);
    fail_unless(obj_a->obj_flags.damage == obj_b->obj_flags.damage);
    fail_unless(obj_a->obj_flags.sigil_idnum == obj_b->obj_flags.sigil_idnum);
    fail_unless(obj_a->obj_flags.sigil_level == obj_b->obj_flags.sigil_level);

    for (int i = 0;i < MAX_OBJ_AFFECT;i++) {
        fail_unless(obj_a->affected[i].location == obj_a->affected[i].location);
        fail_unless(obj_a->affected[i].modifier == obj_a->affected[i].modifier);
    }

    // TODO: test temp object affects
}

START_TEST(test_load_save_creature)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_UNDEFINED);

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");

    compare_creatures(ch, tch);

    if (tch)
        free_creature(tch);
}
END_TEST

START_TEST(test_load_save_cyborg)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_CYBORG);

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");

    compare_creatures(ch, tch);

    fail_unless(GET_OLD_CLASS(ch) == GET_OLD_CLASS(tch));
    fail_unless(GET_TOT_DAM(ch) == GET_TOT_DAM(tch));
    fail_unless(GET_BROKE(ch) == GET_BROKE(tch));
}
END_TEST

START_TEST(test_load_save_mage)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_MAGE);

    SET_SKILL(ch, SPELL_MANA_SHIELD, 1);
    ch->player_specials->saved.mana_shield_low = number(0, 100);
    ch->player_specials->saved.mana_shield_pct = number(0, 100);

    save_player_to_file(ch, "/tmp/test_player_mage.xml");
    tch = load_player_from_file("/tmp/test_player_mage.xml");

    compare_creatures(ch, tch);

    fail_unless(ch->player_specials->saved.mana_shield_low ==
                tch->player_specials->saved.mana_shield_low,
                "mana_shield_low mismatch: %d != %d",
                ch->player_specials->saved.mana_shield_low,
                tch->player_specials->saved.mana_shield_low);
    fail_unless(ch->player_specials->saved.mana_shield_pct ==
                tch->player_specials->saved.mana_shield_pct);
}
END_TEST

START_TEST(test_load_save_immort)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_UNDEFINED);
    GET_LEVEL(ch) = LVL_IMMORT;
    POOFIN(ch) = strdup("poofs in.");
    POOFOUT(ch) = strdup("poofs out.");

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");

    compare_creatures(ch, tch);

    fail_unless(!strcmp(BADGE(ch), BADGE(tch)));
    fail_unless(!strcmp(POOFIN(ch), POOFIN(tch)));
    fail_unless(!strcmp(POOFOUT(ch), POOFOUT(tch)));
    fail_unless(GET_QLOG_LEVEL(ch) == GET_QLOG_LEVEL(tch));
    fail_unless(GET_INVIS_LVL(ch) == GET_INVIS_LVL(tch));
    fail_unless(GET_IMMORT_QP(ch) == GET_IMMORT_QP(tch));
    fail_unless(GET_QUEST_ALLOWANCE(ch) == GET_QUEST_ALLOWANCE(tch));
}
END_TEST

START_TEST(test_load_save_title)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_UNDEFINED);
    set_title(ch, " test title");

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");

    compare_creatures(ch, tch);
}
END_TEST

START_TEST(test_load_save_frozen)
{
    struct creature *tch = NULL;

    randomize_creature(ch, CLASS_UNDEFINED);
    PLR_FLAGS(ch) |= PLR_FROZEN;
    ch->player_specials->thaw_time = time(NULL) + number(0, 65535);
    ch->player_specials->freezer_id = number(0, 65535);

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");

    compare_creatures(ch, tch);

    fail_unless(ch->player_specials->thaw_time == tch->player_specials->thaw_time);
    fail_unless(ch->player_specials->freezer_id == tch->player_specials->freezer_id);
}
END_TEST

START_TEST(test_load_save_objects_carried)
{
    bool save_player_objects_to_file(struct creature *ch, const char *path);

    struct creature *tch = NULL;
    int carried_vnum = make_random_object();
    struct obj_data *carried_item = read_object(carried_vnum);

    obj_to_char(carried_item, ch);
    fail_unless(ch->char_specials.carry_weight == carried_item->obj_flags.weight);

    save_player_to_file(ch, "/tmp/test_player.xml");
    save_player_objects_to_file(ch, "/tmp/test_items.xml");

    tch = load_player_from_file("/tmp/test_player.xml");
    load_player_objects_from_file(tch, "/tmp/test_items.xml");

    struct obj_data *obj_a = ch->carrying;
    struct obj_data *obj_b = tch->carrying;
    while (obj_a || obj_b) {
        if (!obj_a || !obj_b) {
            fail("different carried counts");
            return;
        }
        compare_objects(obj_a, obj_b);
        obj_a = obj_a->next_content;
        obj_b = obj_b->next_content;
    }

    fail_unless(ch->char_specials.carry_weight == tch->char_specials.carry_weight);
    fail_unless(ch->char_specials.carry_items == tch->char_specials.carry_items);

    ch->carrying = NULL;
}
END_TEST

START_TEST(test_load_save_objects_equipped)
{
    bool save_player_objects_to_file(struct creature *ch, const char *path);

    struct creature *tch = NULL;
    int equipped_vnum = make_random_object();
    struct obj_data *equipped_item = read_object(equipped_vnum);
    int equipped_pos = number(0, NUM_WEARS - 1);

    equip_char(ch, equipped_item, equipped_pos, EQUIP_WORN);
    fail_unless(ch->char_specials.worn_weight == equipped_item->obj_flags.weight);

    save_player_to_file(ch, "/tmp/test_player.xml");
    save_player_objects_to_file(ch, "/tmp/test_items.xml");

    tch = load_player_from_file("/tmp/test_player.xml");
    load_player_objects_from_file(tch, "/tmp/test_items.xml");

    for (int i = 0;i < NUM_WEARS;i++) {
        struct obj_data *obj_a = GET_EQ(ch, i);
        struct obj_data *obj_b = GET_EQ(tch, i);
        if (!obj_a && obj_b) {
            fail("wear pos %d on original has eq, loaded doesn't", i);
            return;
        } else if (obj_a && !obj_b) {
            fail("wear pos %d on loaded has eq, original doesn't", i);
            return;
        } else if (obj_a && obj_b) {
            compare_objects(obj_a, obj_b);
        }
    }

    fail_unless(ch->char_specials.carry_weight == tch->char_specials.carry_weight);
    fail_unless(ch->char_specials.carry_items == tch->char_specials.carry_items);
    fail_unless(ch->char_specials.worn_weight == tch->char_specials.worn_weight);

    for (int i = 0;i < NUM_WEARS;i++) {
        GET_EQ(ch, i) = NULL;
    }
}
END_TEST

START_TEST(test_load_save_objects_implanted)
{
    bool save_player_objects_to_file(struct creature *ch, const char *path);

    struct creature *tch = NULL;
    int implanted_vnum = make_random_object();
    struct obj_data *implanted_item = read_object(implanted_vnum);
    int equipped_pos = number(0, NUM_WEARS - 1);

    equip_char(ch, implanted_item, equipped_pos, EQUIP_IMPLANT);

    save_player_to_file(ch, "/tmp/test_player.xml");
    save_player_objects_to_file(ch, "/tmp/test_items.xml");

    tch = load_player_from_file("/tmp/test_player.xml");
    load_player_objects_from_file(tch, "/tmp/test_items.xml");

    for (int i = 0;i < NUM_WEARS;i++) {
        struct obj_data *obj_a = GET_IMPLANT(ch, i);
        struct obj_data *obj_b = GET_IMPLANT(tch, i);
        if (!obj_a && obj_b) {
            fail("implant pos %d on original has eq, loaded doesn't", i);
            return;
        } else if (obj_a && !obj_b) {
            fail("implant pos %d on loaded has eq, original doesn't", i);
            return;
        } else if (obj_a && obj_b) {
            compare_objects(obj_a, obj_b);
        }
    }

    fail_unless(ch->char_specials.carry_weight == tch->char_specials.carry_weight);
    fail_unless(ch->char_specials.carry_items == tch->char_specials.carry_items);
    fail_unless(ch->char_specials.worn_weight == tch->char_specials.worn_weight);

    for (int i = 0;i < NUM_WEARS;i++) {
        GET_IMPLANT(ch, i) = NULL;
    }
}
END_TEST

START_TEST(test_load_save_objects_tattooed)
{
    struct creature *tch = NULL;
    int tattooed_vnum = make_random_object();
    struct obj_data *tattooed_item = read_object(tattooed_vnum);
    int equipped_pos = number(0, NUM_WEARS - 1);

    equip_char(ch, tattooed_item, equipped_pos, EQUIP_TATTOO);

    save_player_to_file(ch, "/tmp/test_player.xml");
    save_player_objects_to_file(ch, "/tmp/test_items.xml");

    tch = load_player_from_file("/tmp/test_player.xml");
    load_player_objects_from_file(tch, "/tmp/test_items.xml");

    for (int i = 0;i < NUM_WEARS;i++) {
        struct obj_data *obj_a = GET_TATTOO(ch, i);
        struct obj_data *obj_b = GET_TATTOO(tch, i);
        if (!obj_a && obj_b) {
            fail("tattoo pos %d on original has eq, loaded doesn't", i);
            return;
        } else if (obj_a && !obj_b) {
            fail("tattoo pos %d on loaded has eq, original doesn't", i);
            return;
        } else if (obj_a && obj_b) {
            compare_objects(obj_a, obj_b);
        }
    }

    fail_unless(ch->char_specials.carry_weight == tch->char_specials.carry_weight);
    fail_unless(ch->char_specials.carry_items == tch->char_specials.carry_items);
    fail_unless(ch->char_specials.worn_weight == tch->char_specials.worn_weight);

    for (int i = 0;i < NUM_WEARS;i++) {
        GET_TATTOO(ch, i) = NULL;
    }
}
END_TEST

START_TEST(test_load_save_objects_contained)
{
    bool save_player_objects_to_file(struct creature *ch, const char *path);

    struct creature *tch = NULL;
    int carried_vnum = make_random_object();
    struct obj_data *carried_item = read_object(carried_vnum);
    int contained_vnum = make_random_object();
    struct obj_data *contained_item = read_object(contained_vnum);

    obj_to_char(carried_item, ch);
    obj_to_obj(contained_item, carried_item);
    // TODO: manage multiple contained or carried items

    fail_unless(ch->char_specials.carry_weight == GET_OBJ_WEIGHT(carried_item));

    save_player_to_file(ch, "/tmp/test_player.xml");
    save_player_objects_to_file(ch, "/tmp/test_items.xml");

    tch = load_player_from_file("/tmp/test_player.xml");
    load_player_objects_from_file(tch, "/tmp/test_items.xml");

    struct obj_data *obj_a = ch->carrying;
    struct obj_data *obj_b = tch->carrying;
    while (obj_a || obj_b) {
        if (!obj_a || !obj_b) {
            fail("different carried counts");
            return;
        }
        compare_objects(obj_a, obj_b);
        obj_a = obj_a->next_content;
        obj_b = obj_b->next_content;
    }

    obj_a = ch->carrying->contains;
    obj_b = tch->carrying->contains;
    while (obj_a || obj_b) {
        if (!obj_a || !obj_b) {
            fail("different contained counts");
            return;
        }
        compare_objects(obj_a, obj_b);
        obj_a = obj_a->next_content;
        obj_b = obj_b->next_content;
    }

    fail_unless(ch->char_specials.carry_weight == tch->char_specials.carry_weight);
    fail_unless(ch->char_specials.carry_items == tch->char_specials.carry_items);
    fail_unless(ch->char_specials.worn_weight == tch->char_specials.worn_weight);

    ch->carrying = NULL;
}
END_TEST

START_TEST(test_load_save_objects_affected)
{
    bool save_player_objects_to_file(struct creature *ch, const char *path);

    int vnum = make_random_object();
    struct obj_data *obj_a = read_object(vnum);
    FILE *ouf;
    struct tmp_obj_affect aff;
    float orig_weight = GET_OBJ_WEIGHT(obj_a);

    memset(&aff, 0, sizeof(aff));
    aff.type = SPELL_ELEMENTAL_BRAND;
    aff.level = 64;
    aff.duration = 5;
    aff.dam_mod = 4200;
    aff.maxdam_mod = 4200;
    aff.type_mod = ITEM_WEAPON;
    aff.weight_mod = 5.0;

    obj_affect_join(obj_a, &aff, AFF_ADD, AFF_ADD, AFF_ADD);

    fail_unless(GET_OBJ_WEIGHT(obj_a) == orig_weight + 5);

    ouf = fopen("/tmp/test_items.xml", "w");
    if (!ouf) {
        fail("Couldn't open file to save object");
        return;
    }
    fprintf(ouf, "<objects>\n");
    save_object_to_xml(obj_a, ouf);
    fprintf(ouf, "</objects>\n");
    fclose(ouf);

    fail_unless(GET_OBJ_WEIGHT(obj_a) == orig_weight + 5);

    xmlDocPtr doc = xmlParseFile("/tmp/test_items.xml");
    if (!doc) {
        fail("Couldn't open file to load object");
        return;
    }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!doc) {
        fail("XML file is empty");
        return;
    }

    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "object")) {
            struct obj_data *obj_b = load_object_from_xml(NULL, NULL, NULL, node);
            compare_objects(obj_a, obj_b);
            free_object(obj_b);
        }
    }
}
END_TEST

Suite *
player_io_suite(void)
{
    Suite *s = suite_create("player_io");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, test_tempus_boot, NULL);
    tcase_add_checked_fixture(tc_core, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_core, test_load_save_creature);
    tcase_add_test(tc_core, test_load_save_cyborg);
    tcase_add_test(tc_core, test_load_save_immort);
    tcase_add_test(tc_core, test_load_save_title);
    tcase_add_test(tc_core, test_load_save_mage);
    tcase_add_test(tc_core, test_load_save_frozen);
    tcase_add_test(tc_core, test_load_save_objects_carried);
    tcase_add_test(tc_core, test_load_save_objects_equipped);
    tcase_add_test(tc_core, test_load_save_objects_implanted);
    tcase_add_test(tc_core, test_load_save_objects_tattooed);
    tcase_add_test(tc_core, test_load_save_objects_contained);
    tcase_add_test(tc_core, test_load_save_objects_affected);
    suite_add_tcase(s, tc_core);

    return s;
}
