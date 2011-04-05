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

static struct descriptor_data *desc = NULL;
static struct account *test_acct = NULL;
static struct creature *ch = NULL;
extern PGconn *sql_cxn;

struct creature *load_player_from_file(const char *path);
void save_player_to_file(struct creature *ch, const char *path);
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
    struct creature *tch;

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
    suite_add_tcase(s, tc_core);

    return s;
}
