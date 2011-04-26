#include <stdlib.h>
#include <check.h>
#include "libpq-fe.h"

#include "desc_data.h"
#include "creature.h"
#include "account.h"
#include "strutil.h"
#include "tmpstr.h"
#include "utils.h"
#include "db.h"
#include "language.h"
#include "char_class.h"
#include "handler.h"

static struct descriptor_data *desc = NULL;
static struct account *test_acct = NULL;
static struct creature *ch = NULL;
extern PGconn *sql_cxn;

struct creature *load_player_from_file(const char *path);
void save_player_to_file(struct creature *ch, const char *path);
bool save_player_objects_to_file(struct creature *ch, const char *path);
bool load_player_objects_from_file(struct creature *ch, const char *path);

void boot_tongues(const char *path);
void boot_spells(const char *path);

int
random_char_class(void)
{
    int choices[] = { CLASS_MAGIC_USER, CLASS_CLERIC, CLASS_BARB, CLASS_THIEF,
                      CLASS_KNIGHT, CLASS_RANGER, CLASS_MONK, CLASS_CYBORG,
                      CLASS_PSIONIC, CLASS_PHYSIC, CLASS_MONK,
                      CLASS_MERCENARY, CLASS_BARD, -1 };
    int len = 0;

    while (choices[len] != -1)
        len++;

    return choices[number(0, len - 1)];
}

void
randomize_creature(struct creature *ch, int char_class)
{
    time_t now = time(0);

    GET_CLASS(ch) = (char_class == CLASS_UNDEFINED) ? random_char_class():char_class;

    GET_MAX_HIT(ch) = number(100, 1000);
    GET_MAX_MANA(ch) = number(100, 1000);
    GET_MAX_MOVE(ch) = number(100, 1000);
    GET_HIT(ch) = number(1, GET_MAX_HIT(ch));
    GET_MANA(ch) = number(1, GET_MAX_MANA(ch));
    GET_MOVE(ch) = number(1, GET_MAX_MOVE(ch));
    GET_AC(ch) = number(-300, 100);
    GET_GOLD(ch) = number(0, 1000000);
    GET_CASH(ch) = number(0, 1000000);
    GET_LEVEL(ch) = number(1, 49);
    GET_EXP(ch) = exp_scale[GET_LEVEL(ch)];
    GET_HITROLL(ch) = number(0, 125);
    GET_DAMROLL(ch) = number(0, 125);
    GET_SEX(ch) = number(0, 2);
    GET_RACE(ch) = number(0,7);
    GET_HEIGHT(ch) = number(1,200);
    GET_WEIGHT(ch) = number(1,200);
    GET_ALIGNMENT(ch) = number(-1000,1000);
    GET_REMORT_GEN(ch) = number(0, 10);
    if (GET_REMORT_GEN(ch) > 0)
        GET_REMORT_CLASS(ch) = random_char_class();
    if (IS_CYBORG(ch)) {
        GET_OLD_CLASS(ch) = number(0, 2);
        GET_TOT_DAM(ch) = number(0, 10);
        GET_BROKE(ch) = number(0, 65535);
    }
    ch->player.time.birth = now - number(100000, 1000000);
    ch->player.time.death = now - number(0, 100000);
    ch->player.time.logon = now - number(0, 100000);
    ch->player.time.played = number(0, 100000);

    ch->real_abils.str = number(3, 25);
    ch->real_abils.intel = number(3, 25);
    ch->real_abils.wis = number(3, 25);
    ch->real_abils.dex = number(3, 25);
    ch->real_abils.con = number(3, 25);
    ch->real_abils.cha = number(3, 25);
    if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
    GET_COND(ch, FULL) = number(0, 20);
    GET_COND(ch, THIRST) = number(0, 20);
    GET_COND(ch, DRUNK) = number(0, 20);
    GET_WIMP_LEV(ch) = number(0, GET_MAX_HIT(ch) / 2);
    GET_LIFE_POINTS(ch) = number(0, 100);
    GET_CLAN(ch) = number(0, 10);
    GET_HOME(ch) = number(0, 10);
    GET_HOMEROOM(ch) = number(3000, 5000);
    GET_LOADROOM(ch) = number(3000, 5000);
    GET_QUEST(ch) = number(1, 60);
    GET_IMMORT_QP(ch) = number(1, 10);
    GET_QUEST_ALLOWANCE(ch) = number(1, 10);
    ch->char_specials.saved.act = number(0, 65535) & ~NPC_ISNPC;
    ch->player_specials->saved.plr2_bits = number(0, 65535);
    ch->player_specials->saved.pref = number(0, 65535);
    ch->player_specials->saved.pref2 = number(0, 65535);
    GET_TONGUE(ch) = number(0, 34);

    for (int i = 0; i < number(0, MAX_WEAPON_SPEC); i++) {
        GET_WEAP_SPEC(ch, i).vnum = number(1, 30000);
        GET_WEAP_SPEC(ch, i).level = number(1, 5);
    }
    GET_TITLE(ch) = strdup("");
}

void
fixture_make_player(void)
{
    sql_cxn = PQconnectdb("user=realm dbname=devtempus");
    sql_exec("delete from players where account=99999");
    sql_exec("delete from accounts where idnum=99999");
    account_boot();
    boot_tongues("../../lib/etc/tongues.xml");
    boot_spells("../../lib/etc/spells.xml");
    CREATE(desc, struct descriptor_data, 1);
    memset(desc, 0, sizeof(struct descriptor_data));
    strcpy(desc->host, "127.0.0.1");
    desc->output = desc->small_outbuf;
    desc->bufspace = SMALL_BUFSIZE - 1;
    desc->login_time = time(0);

    CREATE(test_acct, struct account, 1);
    account_initialize(test_acct, "foo", desc, 99999);
    g_hash_table_insert(account_cache, GINT_TO_POINTER(account_top_id), test_acct);

    ch = account_create_char(test_acct, "Foo");
    GET_EXP(ch) = 1;
}

void
fixture_destroy_player(void)
{
    if (ch) {
        account_delete_char(test_acct, ch);
        free_creature(ch);
    }
    g_hash_table_remove(account_cache, GINT_TO_POINTER(test_acct->id));
    free(test_acct->name);
    free(test_acct->password);
    free(test_acct->creation_addr);
    free(test_acct->login_addr);
    free(test_acct);
    sql_exec("delete from accounts where idnum=%d", test_acct->id);
}

void
compare_creatures(struct creature *ch, struct creature *tch)
{
    fail_unless(!strcmp(GET_NAME(ch), GET_NAME(tch)));
    fail_unless(GET_IDNUM(ch) == GET_IDNUM(tch));
    fail_unless(GET_HIT(ch) == GET_HIT(tch));
    fail_unless(GET_MANA(ch) == GET_MANA(tch));
    fail_unless(GET_MOVE(ch) == GET_MOVE(tch));
    fail_unless(GET_MAX_HIT(ch) == GET_MAX_HIT(tch));
    fail_unless(GET_MAX_MANA(ch) == GET_MAX_MANA(tch));
    fail_unless(GET_MAX_MOVE(ch) == GET_MAX_MOVE(tch));
    fail_unless(GET_GOLD(ch) == GET_GOLD(tch));
    fail_unless(GET_CASH(ch) == GET_CASH(tch));
    fail_unless(GET_EXP(ch) == GET_EXP(tch));
    fail_unless(GET_HITROLL(ch) == GET_HITROLL(tch));
    fail_unless(GET_DAMROLL(ch) == GET_DAMROLL(tch));
    fail_unless(GET_LEVEL(ch) == GET_LEVEL(tch));
    fail_unless(GET_SEX(ch) == GET_SEX(tch));
    fail_unless(GET_RACE(ch) == GET_RACE(tch));
    fail_unless(GET_HEIGHT(ch) == GET_HEIGHT(tch));
    fail_unless(GET_WEIGHT(ch) == GET_WEIGHT(tch));
    fail_unless(GET_ALIGNMENT(ch) == GET_ALIGNMENT(tch));
    fail_unless(GET_CLASS(ch) == GET_CLASS(tch));
    fail_unless(GET_REMORT_CLASS(ch) == GET_REMORT_CLASS(tch));
    fail_unless(GET_REMORT_GEN(ch) == GET_REMORT_GEN(tch));
    fail_unless(!memcmp(&ch->player.time, &tch->player.time, sizeof(ch->player.time)));
    fail_unless(!memcmp(&ch->real_abils, &tch->real_abils, sizeof(ch->real_abils)));
    fail_unless(GET_COND(ch, FULL) == GET_COND(tch, FULL));
    fail_unless(GET_COND(ch, THIRST) == GET_COND(tch, THIRST));
    fail_unless(GET_COND(ch, DRUNK) == GET_COND(tch, DRUNK));
    fail_unless(GET_WIMP_LEV(ch) == GET_WIMP_LEV(tch));
    fail_unless(GET_LIFE_POINTS(ch) == GET_LIFE_POINTS(tch));
    fail_unless(GET_CLAN(ch) == GET_CLAN(tch));
    fail_unless(GET_HOME(ch) == GET_HOME(tch));
    fail_unless(GET_HOMEROOM(ch) == GET_HOMEROOM(tch));
    fail_unless(GET_LOADROOM(ch) == GET_LOADROOM(tch));
    fail_unless(GET_QUEST(ch) == GET_QUEST(tch));
    fail_unless(ch->char_specials.saved.act == tch->char_specials.saved.act);
    fail_unless(ch->player_specials->saved.plr2_bits == tch->player_specials->saved.plr2_bits);
    fail_unless(ch->player_specials->saved.pref == tch->player_specials->saved.pref);
    fail_unless(ch->player_specials->saved.pref2 == tch->player_specials->saved.pref2);
    fail_unless(GET_TONGUE(ch) == GET_TONGUE(tch));
    for (int i = 0; i < MAX_WEAPON_SPEC; i++) {
        fail_unless(GET_WEAP_SPEC(ch, i).vnum == GET_WEAP_SPEC(tch, i).vnum);
        fail_unless(GET_WEAP_SPEC(ch, i).level == GET_WEAP_SPEC(tch, i).level);
    }
    fail_unless(!strcmp(GET_TITLE(ch), GET_TITLE(tch)));
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

    GET_SKILL(ch, SPELL_MANA_SHIELD) = 1;
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
    ch->player_specials->thaw_time = time(0) + number(0, 65535);
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
    int equipped_pos = number(0, NUM_WEARS);

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
    int equipped_pos = number(0, NUM_WEARS);

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
    int equipped_pos = number(0, NUM_WEARS);

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
    fail_unless(ch->char_specials.carry_weight
                == carried_item->obj_flags.weight + contained_item->obj_flags.weight);

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

Suite *
player_io_suite(void)
{
    Suite *s = suite_create("player_io");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, NULL);
    tcase_add_checked_fixture(tc_core, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_core, test_load_save_creature);
    tcase_add_test(tc_core, test_load_save_cyborg);
    tcase_add_test(tc_core, test_load_save_immort);
    tcase_add_test(tc_core, test_load_save_title);
    tcase_add_test(tc_core, test_load_save_mage);
    tcase_add_test(tc_core, test_load_save_objects_carried);
    tcase_add_test(tc_core, test_load_save_objects_equipped);
    tcase_add_test(tc_core, test_load_save_objects_implanted);
    tcase_add_test(tc_core, test_load_save_objects_tattooed);
    tcase_add_test(tc_core, test_load_save_objects_contained);
    suite_add_tcase(s, tc_core);

    return s;
}
