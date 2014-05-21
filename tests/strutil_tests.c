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

static const char *bit_descs[32] = {
    "BIT_00", "BIT_01", "BIT_02", "BIT_03", "BIT_04",
    "BIT_05", "BIT_06", "BIT_07", "BIT_08", "BIT_09",
    "BIT_10", "BIT_11", "BIT_12", "BIT_13", "BIT_14",
    "BIT_15", "\n", NULL
};

START_TEST(test_is_newl_macro)
{
    fail_unless(ISNEWL('\n'));
    fail_unless(ISNEWL('\r'));
    fail_if(ISNEWL('a'));
}
END_TEST

START_TEST(test_if_str_macro)
{
    fail_unless(!strcmp(IF_STR("foo"), "foo"));
    fail_unless(!strcmp(IF_STR(NULL), ""));
}
END_TEST

START_TEST(test_cap_macro)
{
    char str[4] = "foo";
    CAP(str);
    fail_unless(!strcmp(str, "Foo"));
}
END_TEST

START_TEST(test_remove_from_cstring)
{
    char str[9] = "abcdefba";
    remove_from_cstring(str, 'b', '?');
    fail_unless(!strcmp(str, "a?cdef?a"), "remove_from_cstring() result was '%s'", str);
}
END_TEST

START_TEST(test_sprintbit)
{
    char str[1024] = "";
    sprintbit(0x0001, bit_descs, str, sizeof(str));
    fail_unless(!strcmp(str, "BIT_00 "), "sprintbit 0x1 gives \"%s\"", str);
    sprintbit(0x0003, bit_descs, str, sizeof(str));
    fail_unless(!strcmp(str, "BIT_00 BIT_01 "), "sprintbit 0x3 gives \"%s\"", str);
    sprintbit(0x0010, bit_descs, str, sizeof(str));
    fail_unless(!strcmp(str, "BIT_04 "), "sprintbit 0x10 gives \"%s\"", str);
    sprintbit(0, bit_descs, str, sizeof(str));
    fail_unless(!strcmp(str, "NOBITS "), "sprintbit 0 gives \"%s\"", str);
    sprintbit(0x10000, bit_descs, str, sizeof(str));
    fail_unless(!strcmp(str, "UNDEFINED "), "sprintbit 0x10000 gives \"%s\"", str);
}
END_TEST

START_TEST(test_strlist_aref)
{
    const char *strlist[] = {"alpha", "beta", "gamma", "delta", "\n"};

    fail_unless(!strcmp(strlist_aref(0, strlist), "alpha"));
    fail_unless(!strcmp(strlist_aref(1, strlist), "beta"));
    fail_unless(!strcmp(strlist_aref(5, strlist), "UNDEFINED(5)"));
}
END_TEST

START_TEST(test_sprinttype)
{
    const char *strlist[] = {"alpha", "beta", "gamma", "delta", "\n"};
    char str[1024] = "";

    sprinttype(0, strlist, str, sizeof(str));
    fail_unless(!strcmp(str, "alpha"));
    sprinttype(2, strlist, str, sizeof(str));
    fail_unless(!strcmp(strlist_aref(1, strlist), "beta"));
    sprinttype(5, strlist, str, sizeof(str));
    fail_unless(!strcmp(strlist_aref(5, strlist), "UNDEFINED(5)"));
}
END_TEST

START_TEST(test_an)
{
    fail_unless(!strcmp(AN("tests"), "some"));
    fail_unless(!strcmp(AN("fantasies"), "some"));
    fail_unless(!strcmp(AN("test"), "a"));
    fail_unless(!strcmp(AN("egg"), "an"));
    fail_unless(!strcmp(AN("idiot"), "an"));
    fail_unless(!strcmp(AN("arctic"), "an"));
    fail_unless(!strcmp(AN("olive"), "an"));
    fail_unless(!strcmp(AN("ukelele"), "an"));
}
END_TEST

START_TEST(test_yesno)
{
    fail_unless(!strcmp(YESNO(false), "NO"));
    fail_unless(!strcmp(YESNO(true), "YES"));
}
END_TEST

START_TEST(test_onoff)
{
    fail_unless(!strcmp(ONOFF(false), "OFF"));
    fail_unless(!strcmp(ONOFF(true), "ON"));
}
END_TEST

START_TEST(test_fname)
{
    fail_unless(!strcmp(fname("alpha"), "alpha"));
    fail_unless(!strcmp(fname("alpha beta"), "alpha"));
}
END_TEST

START_TEST(test_isname)
{
    fail_unless(isname("Alpha", "alpha beta gamma"));
    fail_unless(isname("Beta", "alpha beta gamma"));
    fail_unless(isname("Gamma", "alpha beta gamma"));
    fail_unless(isname("a", "alpha beta gamma"));
    fail_unless(isname("b", "alpha beta gamma"));
    fail_unless(isname("g", "alpha beta gamma"));
    fail_if(isname("delta", "alpha beta gamma"));
    fail_if(isname("d", "alpha beta gamma"));
}
END_TEST

START_TEST(test_isname_exact)
{
    fail_unless(isname_exact("Alpha", "alpha beta gamma"));
    fail_unless(isname_exact("Beta", "alpha beta gamma"));
    fail_unless(isname_exact("Gamma", "alpha beta gamma"));
    fail_if(isname_exact("a", "alpha beta gamma"));
    fail_if(isname_exact("b", "alpha beta gamma"));
    fail_if(isname_exact("g", "alpha beta gamma"));
    fail_if(isname_exact("delta", "alpha beta gamma"));
    fail_if(isname_exact("d", "alpha beta gamma"));
}
END_TEST

START_TEST(test_namelist_match)
{
    fail_unless(namelist_match("Alpha", "alpha beta gamma"));
    fail_unless(namelist_match("alpha Beta", "alpha beta gamma"));
    fail_unless(namelist_match("beta Gamma", "alpha beta gamma"));
    fail_unless(namelist_match("a b", "alpha beta gamma"));
    fail_if(namelist_match("delta", "alpha beta gamma"));
    fail_if(namelist_match("alpha delta", "alpha beta gamma"));
    fail_if(namelist_match("a d", "alpha beta gamma"));
}
END_TEST

START_TEST(test_get_number)
{
    char buf[10];
    char *str = buf;

    strcpy(buf, "object");
    fail_unless(get_number(&str) == 1);
    fail_unless(!strcmp(str, "object"));
    str = buf;
    strcpy(buf, ".object");
    fail_unless(get_number(&str) == 0);
    fail_unless(!strcmp(str, "object"));
    strcpy(buf, "3.object");
    fail_unless(get_number(&str) == 3);
    fail_unless(!strcmp(str, "object"));
}
END_TEST

START_TEST(test_find_all_dots)
{
    char buf[15];

    strcpy(buf, "object");
    fail_unless(find_all_dots(buf) == FIND_INDIV);
    fail_unless(!strcmp(buf, "object"));
    strcpy(buf, "all.object");
    fail_unless(find_all_dots(buf) == FIND_ALLDOT);
    fail_unless(!strcmp(buf, "object"));
    strcpy(buf, "all");
    fail_unless(find_all_dots(buf) == FIND_ALL);
}
END_TEST

START_TEST(test_one_word)
{
    char buf[1024];
    char first_arg[1024];
    char *str;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_word(buf, first_arg);
    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(str, " \"beta gamma\" Delta"));

    str = one_word(str, first_arg);
    fail_unless(!strcmp(first_arg, "beta gamma"));
    fail_unless(!strcmp(str, " Delta"));
}
END_TEST

START_TEST(test_search_block)
{
    const char *strlist[] = {"Alpha", "Beta", "Gamma", "Delta", "\n"};

    fail_unless(search_block("alpha", strlist, true) == 0);
    fail_unless(search_block("al", strlist, true) == -1);
    fail_unless(search_block("al", strlist, false) == 0);

    fail_unless(search_block("beta", strlist, true) == 1);
    fail_unless(search_block("gamma", strlist, true) == 2);
    fail_unless(search_block("delta", strlist, true) == 3);

    fail_unless(search_block("", strlist, true) == -1);
}
END_TEST

START_TEST(test_is_number)
{
    fail_if(is_number("alpha"));
    fail_if(is_number(""));
    fail_unless(is_number("5"));
    fail_unless(is_number("-5"));
    fail_unless(is_number("+5"));
    fail_if(is_number("+alpha"));
    fail_if(is_number("-alpha"));
}
END_TEST

START_TEST(test_skip_spaces_const)
{
    const char *str = "testing";

    skip_spaces_const(&str);
    fail_unless(!strcmp(str, "testing"));

    str = "    testing";
    skip_spaces_const(&str);
    fail_unless(!strcmp(str, "testing"));

    str = "    testing   ";
    skip_spaces_const(&str);
    fail_unless(!strcmp(str, "testing   "));

    str = "    test  ing   ";
    skip_spaces_const(&str);
    fail_unless(!strcmp(str, "test  ing   "));
}
END_TEST

START_TEST(test_skip_spaces)
{
    char buf[1024];
    char *str = buf;

    strcpy(buf, "testing");
    skip_spaces(&str);
    fail_unless(!strcmp(str, "testing"));

    str = strcpy(buf, "   testing");
    skip_spaces(&str);
    fail_unless(!strcmp(str, "testing"));

    str = strcpy(buf, "   testing   ");
    skip_spaces(&str);
    fail_unless(!strcmp(str, "testing   "));

    str = strcpy(buf, "   test  ing   ");
    skip_spaces(&str);
    fail_unless(!strcmp(str, "test  ing   "));
}
END_TEST

START_TEST(test_fill_word)
{
    fail_unless(fill_word("the"));
    fail_unless(fill_word("from"));
    fail_unless(fill_word("to"));
    fail_if(fill_word("alpha"));
}
END_TEST

START_TEST(test_one_argument)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_argument(str, first_arg);
    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(str, " \"beta gamma\" Delta"));

    str = one_argument(str, first_arg);
    fail_unless(!strcmp(first_arg, "\"beta"));
    fail_unless(!strcmp(str, " gamma\" Delta"));
}
END_TEST

START_TEST(test_any_one_arg)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = any_one_arg(str, first_arg);
    fail_unless(!strcmp(first_arg, "the"));
    fail_unless(!strcmp(str, " Alpha \"beta gamma\" Delta"));

    str = any_one_arg(str, first_arg);
    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(str, " \"beta gamma\" Delta"));

    str = any_one_arg(str, first_arg);
    fail_unless(!strcmp(first_arg, "\"beta"));
    fail_unless(!strcmp(str, " gamma\" Delta"));
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
    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(second_arg, "beta"));
    fail_unless(!strcmp(str, "gamma"));

    str = strcpy(buf, "   the Alpha");
    str = two_arguments(str, first_arg, second_arg);
    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(second_arg, ""));
    fail_unless(!strcmp(str, ""));
}
END_TEST

START_TEST(test_is_abbrev)
{
    fail_unless(is_abbrev("a", "alpha") == 1);
    fail_unless(is_abbrev("al", "alpha") == 1);
    fail_unless(is_abbrev("alph", "alpha") == 1);
    fail_unless(is_abbrev("alpha", "alpha") == 2);
    fail_if(is_abbrev("b", "alpha"));
}
END_TEST

START_TEST(test_is_abbrevn)
{
    fail_unless(is_abbrevn("a", "alpha", 3) == 0);
    fail_unless(is_abbrevn("al", "alpha", 3) == 0);
    fail_unless(is_abbrevn("alph", "alpha", 3) == 1);
    fail_unless(is_abbrevn("alpha", "alpha", 3) == 2);
    fail_if(is_abbrevn("b", "alpha", 3));
}
END_TEST

START_TEST(test_half_chop)
{
    char buf[1024];
    char first_arg[1024];
    char rest[1024];

    strcpy(buf, "alpha beta gamma delta");
    half_chop(buf, first_arg, rest);

    fail_unless(!strcmp(first_arg, "alpha"));
    fail_unless(!strcmp(rest, "beta gamma delta"));
}
END_TEST

START_TEST(test_search_block_no_lower)
{
    const char *strlist[] = {"Alpha", "Beta", "Gamma", "Delta", "\n"};

    fail_unless(search_block_no_lower("Alpha", strlist, true) == 0);
    fail_unless(search_block_no_lower("Al", strlist, true) == -1);
    fail_unless(search_block_no_lower("Al", strlist, false) == 0);
    fail_unless(search_block_no_lower("al", strlist, false) == -1);

    fail_unless(search_block_no_lower("Beta", strlist, true) == 1);
    fail_unless(search_block_no_lower("Gamma", strlist, true) == 2);
    fail_unless(search_block_no_lower("Delta", strlist, true) == 3);

    fail_unless(search_block_no_lower("", strlist, true) == -1);
}
END_TEST

START_TEST(test_fill_word_no_lower)
{
    fail_unless(fill_word_no_lower("the"));
    fail_unless(fill_word_no_lower("from"));
    fail_unless(fill_word_no_lower("to"));
    fail_if(fill_word_no_lower("alpha"));
    fail_if(fill_word_no_lower("THE"));

}
END_TEST

START_TEST(test_one_argument_no_lower)
{
    char buf[1024];
    char first_arg[1024];
    char *str = buf;

    strcpy(buf, "   the Alpha \"beta gamma\" Delta");

    str = one_argument_no_lower(str, first_arg);
    fail_unless(!strcmp(first_arg, "Alpha"), "first_arg was '%s'", first_arg);
    fail_unless(!strcmp(str, " \"beta gamma\" Delta"));

    str = one_argument_no_lower(str, first_arg);
    fail_unless(!strcmp(first_arg, "\"beta"));
    fail_unless(!strcmp(str, " gamma\" Delta"));
}
END_TEST

START_TEST(test_snprintf_cat)
{
    char buf[1024] = "";
    int result = 0;

    result = snprintf_cat(buf, sizeof(buf), "ab%c", 'c');
    fail_unless(result == 3);
    fail_unless(!strcmp(buf, "abc"));

    result = snprintf_cat(buf, sizeof(buf), "def");
    fail_unless(result == 6);
    fail_unless(!strcmp(buf, "abcdef"));
}
END_TEST

Suite *
strutil_suite(void)
{
    Suite *s = suite_create("strutils");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, NULL);
    tcase_add_test(tc_core, test_is_newl_macro);
    tcase_add_test(tc_core, test_if_str_macro);
    tcase_add_test(tc_core, test_cap_macro);
    tcase_add_test(tc_core, test_remove_from_cstring);
    tcase_add_test(tc_core, test_sprintbit);
    tcase_add_test(tc_core, test_strlist_aref);
    tcase_add_test(tc_core, test_sprinttype);
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
