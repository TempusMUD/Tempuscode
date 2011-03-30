#include <stdlib.h>
#include <check.h>
#include "desc_data.h"
#include "creature.h"
#include "account.h"
#include "strutil.h"
#include "tmpstr.h"
#include "utils.h"
#include "libpq-fe.h"

static struct descriptor_data *desc = NULL;
static struct account *test_acct = NULL;
static struct creature *ch = NULL;
extern PGconn *sql_cxn;

void
fixture_make_player(void)
{
    sql_cxn = PQconnectdb("user=realm dbname=devtempus");
    sql_exec("delete from players where account=99999");
    sql_exec("delete from accounts where idnum=99999");
    account_boot();
    boot_tongues("../../lib/etc/tongues.xml");
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

START_TEST(test_load_save_check)
{
    struct creature *tch;

    save_player_to_file(ch, "/tmp/test_player.xml");
    tch = load_player_from_file("/tmp/test_player.xml");
    fail_unless(tch != NULL);
    if (tch)
        free_creature(tch);
}
END_TEST



Suite *
player_io_suite(void)
{
    Suite *s = suite_create("player_io");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, NULL);
    tcase_add_checked_fixture(tc_core, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_core, test_load_save_check);
    suite_add_tcase(s, tc_core);

    return s;
}
