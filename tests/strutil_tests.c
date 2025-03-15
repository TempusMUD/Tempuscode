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

static const char *bit_descs[32] = {
    "BIT_00", "BIT_01", "BIT_02", "BIT_03", "BIT_04",
    "BIT_05", "BIT_06", "BIT_07", "BIT_08", "BIT_09",
    "BIT_10", "BIT_11", "BIT_12", "BIT_13", "BIT_14",
    "BIT_15", "\n", NULL
};

START_TEST(test_is_newl_macro)
{
    ck_assert(ISNEWL('\n'));
    ck_assert(ISNEWL('\r'));
    ck_assert(!ISNEWL('a'));
}
END_TEST
START_TEST(test_if_str_macro)
{
    ck_assert_str_eq(IF_STR("foo"), "foo");
    ck_assert_str_eq(IF_STR(NULL), "");
}
END_TEST
START_TEST(test_cap_macro)
{
    char str[4] = "foo";
    CAP(str);
    ck_assert_str_eq(str, "Foo");
}
END_TEST
START_TEST(test_remove_from_cstring)
{
    char str[9] = "abcdefba";
    remove_from_cstring(str, 'b', '?');
    ck_assert_msg(streq(str, "a?cdef?a"),
                  "remove_from_cstring() result was '%s'", str);
}
END_TEST
START_TEST(test_sprintbit)
{
    char str[1024] = "";
    sprintbit(0x0001, bit_descs, str, sizeof(str));
    ck_assert_msg(streq(str, "BIT_00 "), "sprintbit 0x1 gives \"%s\"", str);
    sprintbit(0x0003, bit_descs, str, sizeof(str));
    ck_assert_msg(streq(str, "BIT_00 BIT_01 "), "sprintbit 0x3 gives \"%s\"",
                  str);
    sprintbit(0x0010, bit_descs, str, sizeof(str));
    ck_assert_msg(streq(str, "BIT_04 "), "sprintbit 0x10 gives \"%s\"", str);
    sprintbit(0, bit_descs, str, sizeof(str));
    ck_assert_msg(streq(str, "NOBITS "), "sprintbit 0 gives \"%s\"", str);
    sprintbit(0x10000, bit_descs, str, sizeof(str));
    ck_assert_msg(streq(str, "UNDEFINED "), "sprintbit 0x10000 gives \"%s\"",
                  str);
}
END_TEST
START_TEST(test_strlist_aref)
{
    const char *strlist[] = {"alpha", "beta", "gamma", "delta", "\n"};

    ck_assert_str_eq(strlist_aref(0, strlist), "alpha");
    ck_assert_str_eq(strlist_aref(1, strlist), "beta");
    ck_assert_str_eq(strlist_aref(5, strlist), "UNDEFINED(5)");
}
END_TEST
START_TEST(test_an)
{
    ck_assert_str_eq(AN("tests"), "some");
    ck_assert_str_eq(AN("fantasies"), "some");
    ck_assert_str_eq(AN("test"), "a");
    ck_assert_str_eq(AN("egg"), "an");
    ck_assert_str_eq(AN("idiot"), "an");
    ck_assert_str_eq(AN("arctic"), "an");
    ck_assert_str_eq(AN("olive"), "an");
    ck_assert_str_eq(AN("ukelele"), "an");
}
END_TEST
START_TEST(test_yesno)
{
    ck_assert_str_eq(YESNO(false), "NO");
    ck_assert_str_eq(YESNO(true), "YES");
}
END_TEST
START_TEST(test_onoff)
{
    ck_assert_str_eq(ONOFF(false), "OFF");
    ck_assert_str_eq(ONOFF(true), "ON");
}
END_TEST
START_TEST(test_fname)
{
    ck_assert_str_eq(fname("alpha"), "alpha");
    ck_assert_str_eq(fname("alpha beta"), "alpha");
}
END_TEST
START_TEST(test_isname)
{
    ck_assert(isname("Alpha", "alpha beta gamma"));
    ck_assert(isname("Beta", "alpha beta gamma"));
    ck_assert(isname("Gamma", "alpha beta gamma"));
    ck_assert(isname("a", "alpha beta gamma"));
    ck_assert(isname("b", "alpha beta gamma"));
    ck_assert(isname("g", "alpha beta gamma"));
    ck_assert(!isname("delta", "alpha beta gamma"));
    ck_assert(!isname("d", "alpha beta gamma"));
}
END_TEST
START_TEST(test_isname_exact)
{
    ck_assert(isname_exact("Alpha", "alpha beta gamma"));
    ck_assert(isname_exact("Beta", "alpha beta gamma"));
    ck_assert(isname_exact("Gamma", "alpha beta gamma"));
    ck_assert(!isname_exact("a", "alpha beta gamma"));
    ck_assert(!isname_exact("b", "alpha beta gamma"));
    ck_assert(!isname_exact("g", "alpha beta gamma"));
    ck_assert(!isname_exact("delta", "alpha beta gamma"));
    ck_assert(!isname_exact("d", "alpha beta gamma"));
}
END_TEST
START_TEST(test_namelist_match)
{
    ck_assert(namelist_match("Alpha", "alpha beta gamma"));
    ck_assert(namelist_match("alpha Beta", "alpha beta gamma"));
    ck_assert(namelist_match("beta Gamma", "alpha beta gamma"));
    ck_assert(namelist_match("a b", "alpha beta gamma"));
    ck_assert(!namelist_match("delta", "alpha beta gamma"));
    ck_assert(!namelist_match("alpha delta", "alpha beta gamma"));
    ck_assert(!namelist_match("a d", "alpha beta gamma"));
}
END_TEST
START_TEST(test_get_number)
{
    char buf[10];
    char *str = buf;

    strcpy(buf, "object");
    ck_assert_int_eq(get_number(&str), 1);
    ck_assert_str_eq(str, "object");
    str = buf;
    strcpy(buf, ".object");
    ck_assert_int_eq(get_number(&str), 0);
    ck_assert_str_eq(str, "object");
    strcpy(buf, "3.object");
    ck_assert_int_eq(get_number(&str), 3);
    ck_assert_str_eq(str, "object");
}
END_TEST
START_TEST(test_find_all_dots)
{
    char buf[15];

    strcpy(buf, "object");
    ck_assert_int_eq(find_all_dots(buf), FIND_INDIV);
    ck_assert_str_eq(buf, "object");
    strcpy(buf, "all.object");
    ck_assert_int_eq(find_all_dots(buf), FIND_ALLDOT);
    ck_assert_str_eq(buf, "object");
    strcpy(buf, "all");
    ck_assert_int_eq(find_all_dots(buf), FIND_ALL);
}
END_TEST
START_TEST(test_one_word)
{
    char buf[1024];
    char first_arg[1024];
    char *str;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_word(buf, first_arg);
    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(str, " \"beta gamma\" Delta");

    str = one_word(str, first_arg);
    ck_assert_str_eq(first_arg, "beta gamma");
    ck_assert_str_eq(str, " Delta");
}
END_TEST
START_TEST(test_search_block)
{
    const char *strlist[] = {"Alpha", "Beta", "Gamma", "Delta", "\n"};

    ck_assert_int_eq(search_block("alpha", strlist, true), 0);
    ck_assert_int_eq(search_block("al", strlist, true), -1);
    ck_assert_int_eq(search_block("al", strlist, false), 0);

    ck_assert_int_eq(search_block("beta", strlist, true), 1);
    ck_assert_int_eq(search_block("gamma", strlist, true), 2);
    ck_assert_int_eq(search_block("delta", strlist, true), 3);

    ck_assert_int_eq(search_block("", strlist, true), -1);
}
END_TEST
START_TEST(test_is_number)
{
    ck_assert(!is_number("alpha"));
    ck_assert(!is_number(""));
    ck_assert(is_number("5"));
    ck_assert(is_number("-5"));
    ck_assert(is_number("+5"));
    ck_assert(!is_number("+alpha"));
    ck_assert(!is_number("-alpha"));
}
END_TEST
START_TEST(test_skip_spaces_const)
{
    const char *str = "testing";

    skip_spaces_const(&str);
    ck_assert_str_eq(str, "testing");

    str = "    testing";
    skip_spaces_const(&str);
    ck_assert_str_eq(str, "testing");

    str = "    testing   ";
    skip_spaces_const(&str);
    ck_assert_str_eq(str, "testing   ");

    str = "    test  ing   ";
    skip_spaces_const(&str);
    ck_assert_str_eq(str, "test  ing   ");
}
END_TEST
START_TEST(test_skip_spaces)
{
    char buf[1024];
    char *str = buf;

    strcpy(buf, "testing");
    skip_spaces(&str);
    ck_assert_str_eq(str, "testing");

    str = strcpy(buf, "   testing");
    skip_spaces(&str);
    ck_assert_str_eq(str, "testing");

    str = strcpy(buf, "   testing   ");
    skip_spaces(&str);
    ck_assert_str_eq(str, "testing   ");

    str = strcpy(buf, "   test  ing   ");
    skip_spaces(&str);
    ck_assert_str_eq(str, "test  ing   ");
}
END_TEST
START_TEST(test_fill_word)
{
    ck_assert(fill_word("the"));
    ck_assert(fill_word("from"));
    ck_assert(fill_word("to"));
    ck_assert(!fill_word("alpha"));
}
END_TEST
START_TEST(test_one_argument)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_argument(str, first_arg);
    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(str, " \"beta gamma\" Delta");

    str = one_argument(str, first_arg);
    ck_assert_str_eq(first_arg, "\"beta");
    ck_assert_str_eq(str, " gamma\" Delta");
}
END_TEST
START_TEST(test_any_one_arg)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = any_one_arg(str, first_arg);
    ck_assert_str_eq(first_arg, "the");
    ck_assert_str_eq(str, " Alpha \"beta gamma\" Delta");

    str = any_one_arg(str, first_arg);
    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(str, " \"beta gamma\" Delta");

    str = any_one_arg(str, first_arg);
    ck_assert_str_eq(first_arg, "\"beta");
    ck_assert_str_eq(str, " gamma\" Delta");
}
END_TEST
START_TEST(test_two_arguments)
{
    char buf[1024];
    char first_arg[1024];
    char second_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha beta gamma");

    str = two_arguments(str, first_arg, second_arg);
    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(second_arg, "beta");
    ck_assert_str_eq(str, "gamma");

    str = strcpy(buf, "   the Alpha");
    str = two_arguments(str, first_arg, second_arg);
    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(second_arg, "");
    ck_assert_str_eq(str, "");
}
END_TEST
START_TEST(test_is_abbrev)
{
    ck_assert_int_eq(is_abbrev("a", "alpha"), 1);
    ck_assert_int_eq(is_abbrev("al", "alpha"), 1);
    ck_assert_int_eq(is_abbrev("alph", "alpha"), 1);
    ck_assert_int_eq(is_abbrev("alpha", "alpha"), 2);
    ck_assert(!is_abbrev("b", "alpha"));
}
END_TEST
START_TEST(test_is_abbrevn)
{
    ck_assert_int_eq(is_abbrevn("a", "alpha", 3), 0);
    ck_assert_int_eq(is_abbrevn("al", "alpha", 3), 0);
    ck_assert_int_eq(is_abbrevn("alph", "alpha", 3), 1);
    ck_assert_int_eq(is_abbrevn("alpha", "alpha", 3), 2);
    ck_assert(!is_abbrevn("b", "alpha", 3));
}
END_TEST
START_TEST(test_half_chop)
{
    char buf[1024];
    char first_arg[1024];
    char rest[1024];

    strcpy(buf, "alpha beta gamma delta");
    half_chop(buf, first_arg, rest);

    ck_assert_str_eq(first_arg, "alpha");
    ck_assert_str_eq(rest, "beta gamma delta");
}
END_TEST
START_TEST(test_search_block_no_lower)
{
    const char *strlist[] = {"Alpha", "Beta", "Gamma", "Delta", "\n"};

    ck_assert_int_eq(search_block_no_lower("Alpha", strlist, true), 0);
    ck_assert_int_eq(search_block_no_lower("Al", strlist, true), -1);
    ck_assert_int_eq(search_block_no_lower("Al", strlist, false), 0);
    ck_assert_int_eq(search_block_no_lower("al", strlist, false), -1);

    ck_assert_int_eq(search_block_no_lower("Beta", strlist, true), 1);
    ck_assert_int_eq(search_block_no_lower("Gamma", strlist, true), 2);
    ck_assert_int_eq(search_block_no_lower("Delta", strlist, true), 3);

    ck_assert_int_eq(search_block_no_lower("", strlist, true), -1);
}
END_TEST
START_TEST(test_fill_word_no_lower)
{
    ck_assert(fill_word_no_lower("the"));
    ck_assert(fill_word_no_lower("from"));
    ck_assert(fill_word_no_lower("to"));
    ck_assert(!fill_word_no_lower("alpha"));
    ck_assert(!fill_word_no_lower("THE"));

}
END_TEST
START_TEST(test_one_argument_no_lower)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_argument_no_lower(str, first_arg);
    ck_assert_str_eq(first_arg, "Alpha");
    ck_assert_str_eq(str, " \"beta gamma\" Delta");

    str = one_argument_no_lower(str, first_arg);
    ck_assert_str_eq(first_arg, "\"beta");
    ck_assert_str_eq(str, " gamma\" Delta");
}
END_TEST
START_TEST(test_snprintf_cat)
{
    char buf[1024] = "";
    int result = 0;

    result = snprintf_cat(buf, sizeof(buf), "ab%c", 'c');
    ck_assert_int_eq(result, 3);
    ck_assert_str_eq(buf, "abc");

    result = snprintf_cat(buf, sizeof(buf), "def");
    ck_assert_int_eq(result, 6);
    ck_assert_str_eq(buf, "abcdef");
}
END_TEST

Suite *
strutil_suite(void)
{
    Suite *s = suite_create("strutils");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, tmp_string_cleanup);
    tcase_add_test(tc_core, test_is_newl_macro);
    tcase_add_test(tc_core, test_if_str_macro);
    tcase_add_test(tc_core, test_cap_macro);
    tcase_add_test(tc_core, test_remove_from_cstring);
    tcase_add_test(tc_core, test_sprintbit);
    tcase_add_test(tc_core, test_strlist_aref);
    tcase_add_test(tc_core, test_an);
    tcase_add_test(tc_core, test_yesno);
    tcase_add_test(tc_core, test_onoff);
    tcase_add_test(tc_core, test_fname);
    tcase_add_test(tc_core, test_isname);
    tcase_add_test(tc_core, test_isname_exact);
    tcase_add_test(tc_core, test_namelist_match);
    tcase_add_test(tc_core, test_get_number);
    tcase_add_test(tc_core, test_find_all_dots);
    tcase_add_test(tc_core, test_one_word);
    tcase_add_test(tc_core, test_search_block);
    tcase_add_test(tc_core, test_is_number);
    tcase_add_test(tc_core, test_skip_spaces_const);
    tcase_add_test(tc_core, test_skip_spaces);
    tcase_add_test(tc_core, test_fill_word);
    tcase_add_test(tc_core, test_one_argument);
    tcase_add_test(tc_core, test_any_one_arg);
    tcase_add_test(tc_core, test_two_arguments);
    tcase_add_test(tc_core, test_is_abbrev);
    tcase_add_test(tc_core, test_is_abbrevn);
    tcase_add_test(tc_core, test_half_chop);
    tcase_add_test(tc_core, test_search_block_no_lower);
    tcase_add_test(tc_core, test_fill_word_no_lower);
    tcase_add_test(tc_core, test_one_argument_no_lower);
    tcase_add_test(tc_core, test_snprintf_cat);
    suite_add_tcase(s, tc_core);

    return s;
}
