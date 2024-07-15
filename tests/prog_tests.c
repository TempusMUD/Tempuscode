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

unsigned char *
prog_compile_prog(struct creature *ch,
                  char *prog_text, void *owner, enum prog_evt_type owner_type);
int
prog_event_handler(void *owner, enum prog_evt_type owner_type,
                   enum prog_evt_phase phase,
                   enum prog_evt_kind kind);

struct cmd_arg {
    enum prog_cmd_kind cmd;
    char *arg;
};
    
START_TEST(test_prog_compile_prog)
{
    int vnum = make_random_mobile();
    struct creature *owner = read_mobile(vnum);
    char *test_prog =
        "*after command whap\n"
        "say hey!";
    struct cmd_arg want[] = {
        { PROG_CMD_AFTER, "" },
        { PROG_CMD_CLRCOND, "" },
        { PROG_CMD_CMPCMD, "\xd3\x03" },
        { PROG_CMD_CONDNEXTHANDLER, "" },
        { PROG_CMD_DO, "say hey!" },
        { PROG_CMD_ENDOFPROG, "" },
    };
    unsigned char *obj = prog_compile_prog(NULL, test_prog, owner, PROG_TYPE_MOBILE);
    ck_assert(obj != NULL);
    owner->mob_specials.shared->progobj = obj;
    int addr = prog_event_handler(owner, PROG_TYPE_MOBILE, PROG_EVT_AFTER, PROG_EVT_COMMAND);
    ck_assert(addr != 0);
    int want_idx = 0;
    prog_word_t cmd;
    do {
        cmd = *((prog_word_t *)(obj + addr));
        int arg_addr = *((prog_word_t *)(obj + addr) + 1);
        fprintf(stderr, "%-20s %x %s\n", prog_cmds[cmd].str, arg_addr, (char *)obj + arg_addr);
        ck_assert_int_eq(cmd, want[want_idx].cmd);
        ck_assert_str_eq((char *)obj + arg_addr, want[want_idx].arg);
        addr += 2 * sizeof(prog_word_t);
        want_idx++;
    } while (cmd != PROG_CMD_ENDOFPROG);
    
    free_creature(owner);
    free(obj);
}
END_TEST

Suite *
prog_suite(void)
{
    Suite *s = suite_create("player_io");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, test_tempus_boot, test_tempus_cleanup);
    tcase_add_test(tc_core, test_prog_compile_prog);
    suite_add_tcase(s, tc_core);

    return s;
}
