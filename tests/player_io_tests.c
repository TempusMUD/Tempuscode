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
#include "account.h"
#include "screen.h"
#include "players.h"
#include "tmpstr.h"
#include "accstr.h"
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

START_TEST(test_fixture_addresses)
{
fprintf(stderr, "fixture\n");
}
END_TEST

START_TEST(test_load_save_creature)
{
    struct creature *tch;

    fprintf(stderr, "creature\n");
    randomize_creature(ch, CLASS_UNDEFINED);

    save_player_to_file(ch, test_path("test_player.xml"));
    tch = load_player_from_file(test_path("test_player.xml"));

    compare_creatures(ch, tch);

    if (tch) {
        free_creature(tch);
    }
}
END_TEST
START_TEST(test_load_save_cyborg)
{
    struct creature *tch;

    fprintf(stderr, "cyborg\n");
    randomize_creature(ch, CLASS_CYBORG);

    save_player_to_file(ch, test_path("test_player.xml"));
    tch = load_player_from_file(test_path("test_player.xml"));

    compare_creatures(ch, tch);

    fail_unless(GET_OLD_CLASS(ch) == GET_OLD_CLASS(tch));
    fail_unless(GET_TOT_DAM(ch) == GET_TOT_DAM(tch));
    fail_unless(GET_BROKE(ch) == GET_BROKE(tch));

    free_creature(tch);
}
END_TEST
START_TEST(test_load_save_mage)
{
    struct creature *tch;

    fprintf(stderr, "mage\n");
    randomize_creature(ch, CLASS_MAGE);

    SET_SKILL(ch, SPELL_MANA_SHIELD, 1);
    ch->player_specials->saved.mana_shield_low = number(0, 100);
    ch->player_specials->saved.mana_shield_pct = number(0, 100);

    save_player_to_file(ch, test_path("test_player_mage.xml"));
    tch = load_player_from_file(test_path("test_player_mage.xml"));

    compare_creatures(ch, tch);

    fail_unless(ch->player_specials->saved.mana_shield_low ==
                tch->player_specials->saved.mana_shield_low,
                "mana_shield_low mismatch: %d != %d",
                ch->player_specials->saved.mana_shield_low,
                tch->player_specials->saved.mana_shield_low);
    fail_unless(ch->player_specials->saved.mana_shield_pct ==
                tch->player_specials->saved.mana_shield_pct);

    free_creature(tch);
}
END_TEST
START_TEST(test_load_save_immort)
{
    struct creature *tch;

    fprintf(stderr, "immort\n");
    randomize_creature(ch, CLASS_UNDEFINED);
    GET_LEVEL(ch) = LVL_IMMORT;
    POOFIN(ch) = strdup("poofs in.");
    POOFOUT(ch) = strdup("poofs out.");

    save_player_to_file(ch, test_path("test_player.xml"));
    tch = load_player_from_file(test_path("test_player.xml"));

    compare_creatures(ch, tch);

    fail_unless(!strcmp(BADGE(ch), BADGE(tch)));
    fail_unless(!strcmp(POOFIN(ch), POOFIN(tch)));
    fail_unless(!strcmp(POOFOUT(ch), POOFOUT(tch)));
    fail_unless(GET_QLOG_LEVEL(ch) == GET_QLOG_LEVEL(tch));
    fail_unless(GET_INVIS_LVL(ch) == GET_INVIS_LVL(tch));
    fail_unless(GET_IMMORT_QP(ch) == GET_IMMORT_QP(tch));
    fail_unless(GET_QUEST_ALLOWANCE(ch) == GET_QUEST_ALLOWANCE(tch));

    free_creature(tch);
}
END_TEST
START_TEST(test_load_save_title)
{
    struct creature *tch;

    randomize_creature(ch, CLASS_UNDEFINED);
    set_title(ch, " test title");

    save_player_to_file(ch, test_path("test_player.xml"));
    tch = load_player_from_file(test_path("test_player.xml"));

    compare_creatures(ch, tch);
    
    free_creature(tch);
}
END_TEST
START_TEST(test_load_save_frozen)
{
    struct creature *tch = NULL;

    randomize_creature(ch, CLASS_UNDEFINED);
    PLR_FLAGS(ch) |= PLR_FROZEN;
    ch->player_specials->thaw_time = time(NULL) + number(0, 65535);
    ch->player_specials->freezer_id = number(0, 65535);

    save_player_to_file(ch, test_path("test_player.xml"));
    tch = load_player_from_file(test_path("test_player.xml"));

    compare_creatures(ch, tch);

    fail_unless(ch->player_specials->thaw_time == tch->player_specials->thaw_time);
    fail_unless(ch->player_specials->freezer_id == tch->player_specials->freezer_id);
    
    free_creature(tch);
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

    save_player_to_file(ch, test_path("test_player.xml"));
    save_player_objects_to_file(ch, test_path("test_items.xml"));

    tch = load_player_from_file(test_path("test_player.xml"));
    load_player_objects_from_file(tch, test_path("test_items.xml"));

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

    
    while (ch->carrying) {
        struct obj_data *obj = ch->carrying;
        obj_from_char(obj);
        extract_obj(obj);
    }

    while (tch->carrying) {
        struct obj_data *obj = tch->carrying;
        obj_from_char(obj);
        extract_obj(obj);
    }

    free_creature(tch);
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

    save_player_to_file(ch, test_path("test_player.xml"));
    save_player_objects_to_file(ch, test_path("test_items.xml"));

    tch = load_player_from_file(test_path("test_player.xml"));
    load_player_objects_from_file(tch, test_path("test_items.xml"));

    for (int i = 0; i < NUM_WEARS; i++) {
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

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, i)) {
            extract_obj(GET_EQ(ch, i));
            GET_EQ(ch, i) = NULL;
        }
        if (GET_EQ(tch, i)) {
            extract_obj(GET_EQ(tch, i));
            GET_EQ(tch, i) = NULL;
        }
    }
    free_creature(tch);
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

    save_player_to_file(ch, test_path("test_player.xml"));
    save_player_objects_to_file(ch, test_path("test_items.xml"));

    tch = load_player_from_file(test_path("test_player.xml"));
    load_player_objects_from_file(tch, test_path("test_items.xml"));

    for (int i = 0; i < NUM_WEARS; i++) {
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

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_IMPLANT(ch, i)) {
            extract_obj(GET_IMPLANT(ch, i));
            GET_IMPLANT(ch, i) = NULL;
        }
        if (GET_IMPLANT(tch, i)) {
            extract_obj(GET_IMPLANT(tch, i));
            GET_IMPLANT(tch, i) = NULL;
        }
    }

    free_creature(tch);
}
END_TEST
START_TEST(test_load_save_objects_tattooed)
{
    struct creature *tch = NULL;
    int tattooed_vnum = make_random_object();
    struct obj_data *tattooed_item = read_object(tattooed_vnum);
    int equipped_pos = number(0, NUM_WEARS - 1);

    equip_char(ch, tattooed_item, equipped_pos, EQUIP_TATTOO);

    save_player_to_file(ch, test_path("test_player.xml"));
    save_player_objects_to_file(ch, test_path("test_items.xml"));

    tch = load_player_from_file(test_path("test_player.xml"));
    load_player_objects_from_file(tch, test_path("test_items.xml"));

    for (int i = 0; i < NUM_WEARS; i++) {
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

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_TATTOO(ch, i)) {
            extract_obj(GET_TATTOO(ch, i));
            GET_TATTOO(ch, i) = NULL;
        }
        if (GET_TATTOO(tch, i)) {
            extract_obj(GET_TATTOO(tch, i));
            GET_TATTOO(tch, i) = NULL;
        }
    }

    free_creature(tch);
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

    save_player_to_file(ch, test_path("test_player.xml"));
    save_player_objects_to_file(ch, test_path("test_items.xml"));

    tch = load_player_from_file(test_path("test_player.xml"));
    load_player_objects_from_file(tch, test_path("test_items.xml"));

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

    while (ch->carrying) {
        struct obj_data *obj = ch->carrying;
        obj_from_char(obj);
        extract_obj(obj);
    }

    while (tch->carrying) {
        struct obj_data *obj = tch->carrying;
        obj_from_char(obj);
        extract_obj(obj);
    }

    
    free_creature(tch);
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

    ouf = fopen(test_path("test_items.xml"), "w");
    if (!ouf) {
        fail("Couldn't open file to save object");
        return;
    }
    fprintf(ouf, "<objects>\n");
    save_object_to_xml(obj_a, ouf);
    fprintf(ouf, "</objects>\n");
    fclose(ouf);

    fail_unless(GET_OBJ_WEIGHT(obj_a) == orig_weight + 5);

    xmlDocPtr doc = xmlParseFile(test_path("test_items.xml"));
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
    xmlFreeDoc(doc);
    free_object(obj_a);
}
END_TEST

Suite *
player_io_suite(void)
{
    Suite *s = suite_create("player_io");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, test_tempus_boot, NULL);
    tcase_add_checked_fixture(tc_core, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_core, test_fixture_addresses);
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
