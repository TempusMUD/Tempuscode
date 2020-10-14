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
#include "accstr.h"
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

// Testing tmp_tolower
START_TEST(tmp_tolower_1)
{
    ck_assert_str_eq(tmp_tolower("abcdef"), "abcdef");
}
END_TEST
START_TEST(tmp_tolower_2)
{
    ck_assert_str_eq(tmp_tolower("ABCDEF"), "abcdef");
}
END_TEST
START_TEST(tmp_tolower_3)
{
    ck_assert_str_eq(tmp_tolower("AbCdEf"), "abcdef");
}
END_TEST
START_TEST(tmp_tolower_4)
{
    ck_assert_str_eq(tmp_tolower("aBcDeF"), "abcdef");
}
END_TEST
START_TEST(tmp_tolower_5)
{
    ck_assert_str_eq(tmp_tolower("123456"), "123456");
}
END_TEST
START_TEST(tmp_tolower_6)
{
    ck_assert_str_eq(tmp_tolower(""), "");
}
END_TEST
// Testing tmp_strdupt
START_TEST(tmp_strdupt_1)
{
    tmp_strdup("xxxxxxxxxxxxxxxxxxxxxxxx");
    tmp_gc_strings();
    ck_assert_str_eq(tmp_strdupt("", "\n"), "");
}
END_TEST
START_TEST(tmp_strdupt_2)
{
    tmp_strdup("xxxxxxxxxxxxxxxxxxxxxxxx");
    tmp_gc_strings();
    ck_assert_str_eq(tmp_strdupt("foo\nbar", "\n"), "foo");
}
END_TEST
START_TEST(tmp_strdupt_3)
{
    tmp_strdup("xxxxxxxxxxxxxxxxxxxxxxxx");
    tmp_gc_strings();
    ck_assert_str_eq(tmp_strdupt("foo", "\n"), "foo");
}
END_TEST
// Testing tmp_gsub
START_TEST(tmp_gsub_1)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "", ""), "abcdef");
}
END_TEST
START_TEST(tmp_gsub_2)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "ghi", ""), "abcdef");
}
END_TEST
START_TEST(tmp_gsub_3)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "c", "ghi"), "abghidef");
}
END_TEST
START_TEST(tmp_gsub_4)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "cde", "g"), "abgf");
}
END_TEST
START_TEST(tmp_gsub_5)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "abc", ""), "def");
}
END_TEST
START_TEST(tmp_gsub_6)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "abc", "g"), "gdef");
}
END_TEST
START_TEST(tmp_gsub_7)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "def", ""), "abc");
}
END_TEST
START_TEST(tmp_gsub_8)
{
    ck_assert_str_eq(tmp_gsub("abcdef", "def", "g"), "abcg");
}
END_TEST
START_TEST(tmp_gsub_9)
{
    ck_assert_str_eq(tmp_gsub("", "", ""), "");
}
END_TEST
START_TEST(tmp_gsub_10)
{
    ck_assert_str_eq(tmp_gsub("ABCDEF", "def", "xxx"), "ABCDEF");
}
END_TEST
START_TEST(tmp_gsub_11)
{
    ck_assert_str_eq(tmp_gsub("ABCDEFEDCBA", "D", "xxx"), "ABCxxxEFExxxCBA");
}
END_TEST
// Testing tmp_gsubi
START_TEST(tmp_gsubi_1)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "", ""), "abcdef");
}
END_TEST
START_TEST(tmp_gsubi_2)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "ghi", ""), "abcdef");
}
END_TEST
START_TEST(tmp_gsubi_3)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "c", "ghi"), "abghidef");
}
END_TEST
START_TEST(tmp_gsubi_4)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "cde", "g"), "abgf");
}
END_TEST
START_TEST(tmp_gsubi_5)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "abc", ""), "def");
}
END_TEST
START_TEST(tmp_gsubi_6)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "abc", "g"), "gdef");
}
END_TEST
START_TEST(tmp_gsubi_7)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "def", ""), "abc");
}
END_TEST
START_TEST(tmp_gsubi_8)
{
    ck_assert_str_eq(tmp_gsubi("abcdef", "def", "g"), "abcg");
}
END_TEST
START_TEST(tmp_gsubi_9)
{
    ck_assert_str_eq(tmp_gsubi("", "", ""), "");
}
END_TEST
START_TEST(tmp_gsubi_10)
{
    ck_assert_str_eq(tmp_gsubi("ABCDEF", "def", "xxx"), "ABCxxx");
}
END_TEST
START_TEST(tmp_gsubi_11)
{
    ck_assert_str_eq(tmp_gsubi("ABCDEFEDCBA", "d", "xxx"), "ABCxxxEFExxxCBA");
}
END_TEST
// Testing tmp_sqlescape
START_TEST(tmp_sqlescape_1)
{
    ck_assert_str_eq(tmp_sqlescape("abcd'ef"), "abcd''ef");
}
END_TEST
START_TEST(tmp_sqlescape_2)
{
    ck_assert_str_eq(tmp_sqlescape("abcd\\'ef"), "abcd\\\\''ef");
}
END_TEST
START_TEST(tmp_sqlescape_3)
{
    ck_assert_str_eq(tmp_sqlescape("abcd\\ef"), "abcd\\\\ef");
}
END_TEST
// Testing tmp_printbits
START_TEST(tmp_printbits_1)
{
    ck_assert_str_eq(tmp_printbits(0x1, bit_descs), "BIT_00");
}
END_TEST
START_TEST(tmp_printbits_2)
{
    ck_assert_str_eq(tmp_printbits(0x2, bit_descs), "BIT_01");
}
END_TEST
START_TEST(tmp_printbits_3)
{
    ck_assert_str_eq(tmp_printbits(0x3, bit_descs), "BIT_00 BIT_01");
}
END_TEST
START_TEST(tmp_printbits_4)
{
    ck_assert_str_eq(tmp_printbits(0x4, bit_descs), "BIT_02");
}
END_TEST
START_TEST(tmp_printbits_5)
{
    ck_assert_str_eq(tmp_printbits(0x5, bit_descs), "BIT_00 BIT_02");
}
END_TEST
START_TEST(tmp_printbits_6)
{
    ck_assert_str_eq(tmp_printbits(0x6, bit_descs), "BIT_01 BIT_02");
}
END_TEST
START_TEST(tmp_printbits_7)
{
    ck_assert_str_eq(tmp_printbits(0x7, bit_descs), "BIT_00 BIT_01 BIT_02");
}
END_TEST
START_TEST(tmp_printbits_8)
{
    ck_assert_str_eq(tmp_printbits(0x20000, bit_descs), "");
}
END_TEST
// Testing tmp_substr
START_TEST(tmp_substr_1)
{
    ck_assert_str_eq(tmp_substr("abcdef", 0, -1), "abcdef");
}
END_TEST
START_TEST(tmp_substr_2)
{
    ck_assert_str_eq(tmp_substr("abcdef", 0, 2), "abc");
}
END_TEST
START_TEST(tmp_substr_3)
{
    ck_assert_str_eq(tmp_substr("abcdef", 1, 4), "bcde");
}
END_TEST
START_TEST(tmp_substr_4)
{
    ck_assert_str_eq(tmp_substr("abcdef", 3, -1), "def");
}
END_TEST
START_TEST(tmp_substr_5)
{
    ck_assert_str_eq(tmp_substr("abcdef", -1, -1), "f");
}
END_TEST
START_TEST(tmp_substr_6)
{
    ck_assert_str_eq(tmp_substr("abcdef", 0, 0), "a");
}
END_TEST
START_TEST(tmp_substr_7)
{
    ck_assert_str_eq(tmp_substr("abcdef", -5, -1), "bcdef");
}
END_TEST
// Testing tmp_trim
START_TEST(tmp_trim_1)
{
    ck_assert_str_eq(tmp_trim(""), "");
}
END_TEST
START_TEST(tmp_trim_2)
{
    ck_assert_str_eq(tmp_trim("abcdef"), "abcdef");
}
END_TEST
START_TEST(tmp_trim_3)
{
    ck_assert_str_eq(tmp_trim("abcdef"), "abcdef");
}
END_TEST
START_TEST(tmp_trim_4)
{
    ck_assert_str_eq(tmp_trim("   abcdef"), "abcdef");
}
END_TEST
START_TEST(tmp_trim_5)
{
    ck_assert_str_eq(tmp_trim("abcdef   "), "abcdef");
}
END_TEST
START_TEST(tmp_trim_6)
{
    ck_assert_str_eq(tmp_trim("   abcdef   "), "abcdef");
}
END_TEST
START_TEST(tmp_trim_7)
{
    ck_assert_str_eq(tmp_trim("   abc def   "), "abc def");
}
END_TEST
// Testing tmp_format
START_TEST(tmp_format_1)
{
    char *test_str = "       This is the main junction of the Holy City of Modrian."
                     "Citizens, travelers, adventurers, priests, and rogues all mingle here "
                     "as they go their separate ways.  The shining temple of Guiharia "
                     "towers to the south through a great silver archway.  Symbolically "
                     "positioned across the square to the north rises Town Hall, a "
                     "converted abbey with an arched facade of blue tile.  Goddess Street "
                     "stretches across the ancient city east to west.   ";
    char *expected_str = "   This is the main junction of the Holy City of Modrian.  Citizens,\r\n"
                         "travelers, adventurers, priests, and rogues all mingle here as they go\r\n"
                         "their separate ways.  The shining temple of Guiharia towers to the south\r\n"
                         "through a great silver archway.  Symbolically positioned across the\r\n"
                         "square to the north rises Town Hall, a converted abbey with an arched\r\n"
                         "facade of blue tile.  Goddess Street stretches across the ancient city\r\n"
                         "east to west.\r\n";
    ck_assert_str_eq(tmp_format(test_str, 72, 3, 3, 0), expected_str);
}
END_TEST
START_TEST(tmp_format_2)
{
    char *test_str = "Testing\r\nline feed\r\nhandling\r\n";
    char *expected_str = "Testing\r\nline feed\r\nhandling\r\n";
    ck_assert_str_eq(tmp_format(test_str, 72, 0, 0, 0), expected_str);
}
END_TEST
START_TEST(tmp_format_3)
{
    char *test_str = "These are flags: ABC DEF GHI JKL MNO PQR STU VWX\r\n";
    char *expected_str = "These are flags: ABC DEF\r\n"
                         "                 GHI JKL\r\n"
                         "                 MNO PQR\r\n"
                         "                 STU VWX\r\n";
    ck_assert_str_eq(tmp_format(test_str, 26, 0, 0, 17), expected_str);
}
END_TEST
START_TEST(tmp_format_4)
{
    char *test_str = "This should be across two different lines, indented at first by four, then by one...\r\nThis should be across a few lines, indented by five, and then by one.\r\n";
    char *expected_str = "    This should be across\r\n"
                         " two different lines,\r\n"
                         " indented at first by\r\n"
                         " four, then by one...\r\n"
                         "     This should be across\r\n"
                         " a few lines, indented by\r\n"
                         " five, and then by one.\r\n";
    ck_assert_str_eq(tmp_format(test_str, 26, 4, 5, 1), expected_str);
}
END_TEST
START_TEST(tmp_format_5)
{
    char *test_str = "This should be across\r\n\r\nTwo paragraphs.\r\n";
    char *expected_str = "   This should be across\r\n"
                         "   Two paragraphs.\r\n";
    ck_assert_str_eq(tmp_format(test_str, 26, 3, 3, 0), expected_str);
}
END_TEST
START_TEST(tmp_format_6)
{
    char *test_str = "These are numbers: 1.50 1,000,000 100.  One is 1. Two is 2, but three isn't.";
    char *expected_str = "These are numbers: 1.50 1,000,000 100.  One is 1.  Two is 2, but three isn't.";
    ck_assert_str_eq(tmp_format(test_str, 80, 0, 0, 0), expected_str);
}
END_TEST
START_TEST(tmp_format_7)
{
    char *test_str = "In a (parenthetical note). Test.";
    char *expected_str = "In a (parenthetical note).  Test.";
    ck_assert_str_eq(tmp_format(test_str, 80, 0, 0, 0), expected_str);
}
END_TEST
// Testing tmp_wrap
START_TEST(tmp_wrap_1)
{
    char *test_str = "       This is the main junction of the Holy City of Modrian."
                     "Citizens, travelers, adventurers, priests, and rogues all mingle here "
                     "as they go their separate ways.  The shining temple of Guiharia "
                     "towers to the south through a great silver archway.  Symbolically "
                     "positioned across the square to the north rises Town Hall, a "
                     "converted abbey with an arched facade of blue tile.  Goddess Street "
                     "stretches across the ancient city east to west.   ";
    char *expected_str = "   This is the main junction of the Holy City of Modrian.Citizens,\r\n"
                         "travelers, adventurers, priests, and rogues all mingle here as they go\r\n"
                         "their separate ways.  The shining temple of Guiharia towers to the south\r\n"
                         "through a great silver archway.  Symbolically positioned across the\r\n"
                         "square to the north rises Town Hall, a converted abbey with an arched\r\n"
                         "facade of blue tile.  Goddess Street stretches across the ancient city\r\n"
                         "east to west.\r\n";
    ck_assert_str_eq(tmp_wrap(test_str, 72, 3, 3, 0), expected_str);
}
END_TEST
START_TEST(tmp_wrap_2)
{
    char *test_str = "Testing\r\nline feed\r\nhandling\r\n";
    char *expected_str = "Testing\r\nline feed\r\nhandling\r\n";
    ck_assert_str_eq(tmp_wrap(test_str, 72, 0, 0, 0), expected_str);
}
END_TEST
START_TEST(tmp_wrap_3)
{
    char *test_str = "These are flags: ABC DEF GHI JKL MNO PQR STU VWX\r\n";
    char *expected_str = "These are flags: ABC DEF\r\n"
                         "                 GHI JKL\r\n"
                         "                 MNO PQR\r\n"
                         "                 STU VWX\r\n";
    ck_assert_str_eq(tmp_wrap(test_str, 26, 0, 0, 17), expected_str);
}
END_TEST
START_TEST(tmp_wrap_4)
{
    char *test_str = "This should be across two different lines, indented at first by four, then by one...\r\nThis should be across a few lines, indented by five, and then by one.\r\n";
    char *expected_str = "    This should be across\r\n"
                         " two different lines,\r\n"
                         " indented at first by\r\n"
                         " four, then by one...\r\n"
                         "     This should be across\r\n"
                         " a few lines, indented by\r\n"
                         " five, and then by one.\r\n";
    ck_assert_str_eq(tmp_wrap(test_str, 26, 4, 5, 1), expected_str);
}
END_TEST
START_TEST(tmp_wrap_5)
{
    char *test_str = "This should be across\r\n\r\nTwo paragraphs.\r\n";
    char *expected_str = "   This should be across\r\n"
                         "   Two paragraphs.\r\n";
    ck_assert_str_eq(tmp_wrap(test_str, 26, 3, 3, 0), expected_str);
}
END_TEST
START_TEST(tmp_wrap_6)
{
    char *test_str = "These are numbers: 1.50 1,000,000 100.  One is 1. Two is 2, but three isn't.";
    char *expected_str = "These are numbers: 1.50 1,000,000 100.  One is 1. Two is 2, but three isn't.";
    ck_assert_str_eq(tmp_wrap(test_str, 80, 0, 0, 0), expected_str);
}
END_TEST
START_TEST(tmp_wrap_7)
{
    char *test_str = "In a (parenthetical note). Test.";
    char *expected_str = "In a (parenthetical note). Test.";
    ck_assert_str_eq(tmp_wrap(test_str, 80, 0, 0, 0), expected_str);
}
END_TEST
START_TEST(tmp_strftime_1)
{
    time_t t = 1602685738;
    struct tm test_val;
    char *expected_str = "2020-10-14";

    localtime_r(&t, &test_val);
    ck_assert_str_eq(tmp_strftime("%Y-%m-%d", &test_val), expected_str);
}
END_TEST
START_TEST(tmp_strftime_2)
{
    time_t t = 1602685738;
    struct tm test_val;
    char *expected_str = "Wed Oct 14 10:28:58 2020";

    localtime_r(&t, &test_val);
    ck_assert_str_eq(tmp_strftime("%a %b %d %H:%M:%S %Y", &test_val), expected_str);
}
END_TEST

Suite *
tmpstr_suite(void)
{
    Suite *s = suite_create("tmpstr");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, tmp_string_init, NULL);
    tcase_add_test(tc_core, tmp_tolower_1);
    tcase_add_test(tc_core, tmp_tolower_2);
    tcase_add_test(tc_core, tmp_tolower_3);
    tcase_add_test(tc_core, tmp_tolower_4);
    tcase_add_test(tc_core, tmp_tolower_5);
    tcase_add_test(tc_core, tmp_tolower_6);
    tcase_add_test(tc_core, tmp_strdupt_1);
    tcase_add_test(tc_core, tmp_strdupt_2);
    tcase_add_test(tc_core, tmp_strdupt_3);
    tcase_add_test(tc_core, tmp_gsub_1);
    tcase_add_test(tc_core, tmp_gsub_2);
    tcase_add_test(tc_core, tmp_gsub_3);
    tcase_add_test(tc_core, tmp_gsub_4);
    tcase_add_test(tc_core, tmp_gsub_5);
    tcase_add_test(tc_core, tmp_gsub_6);
    tcase_add_test(tc_core, tmp_gsub_7);
    tcase_add_test(tc_core, tmp_gsub_8);
    tcase_add_test(tc_core, tmp_gsub_9);
    tcase_add_test(tc_core, tmp_gsub_10);
    tcase_add_test(tc_core, tmp_gsub_11);
    tcase_add_test(tc_core, tmp_gsubi_1);
    tcase_add_test(tc_core, tmp_gsubi_2);
    tcase_add_test(tc_core, tmp_gsubi_3);
    tcase_add_test(tc_core, tmp_gsubi_4);
    tcase_add_test(tc_core, tmp_gsubi_5);
    tcase_add_test(tc_core, tmp_gsubi_6);
    tcase_add_test(tc_core, tmp_gsubi_7);
    tcase_add_test(tc_core, tmp_gsubi_8);
    tcase_add_test(tc_core, tmp_gsubi_9);
    tcase_add_test(tc_core, tmp_gsubi_10);
    tcase_add_test(tc_core, tmp_gsubi_11);
    tcase_add_test(tc_core, tmp_sqlescape_1);
    tcase_add_test(tc_core, tmp_sqlescape_2);
    tcase_add_test(tc_core, tmp_sqlescape_3);
    tcase_add_test(tc_core, tmp_printbits_1);
    tcase_add_test(tc_core, tmp_printbits_2);
    tcase_add_test(tc_core, tmp_printbits_3);
    tcase_add_test(tc_core, tmp_printbits_4);
    tcase_add_test(tc_core, tmp_printbits_5);
    tcase_add_test(tc_core, tmp_printbits_6);
    tcase_add_test(tc_core, tmp_printbits_7);
    tcase_add_test(tc_core, tmp_printbits_8);
    tcase_add_test(tc_core, tmp_substr_1);
    tcase_add_test(tc_core, tmp_substr_2);
    tcase_add_test(tc_core, tmp_substr_3);
    tcase_add_test(tc_core, tmp_substr_4);
    tcase_add_test(tc_core, tmp_substr_5);
    tcase_add_test(tc_core, tmp_substr_6);
    tcase_add_test(tc_core, tmp_substr_7);
    tcase_add_test(tc_core, tmp_trim_1);
    tcase_add_test(tc_core, tmp_trim_2);
    tcase_add_test(tc_core, tmp_trim_3);
    tcase_add_test(tc_core, tmp_trim_4);
    tcase_add_test(tc_core, tmp_trim_5);
    tcase_add_test(tc_core, tmp_trim_6);
    tcase_add_test(tc_core, tmp_trim_7);
    tcase_add_test(tc_core, tmp_format_1);
    tcase_add_test(tc_core, tmp_format_2);
    tcase_add_test(tc_core, tmp_format_3);
    tcase_add_test(tc_core, tmp_format_4);
    tcase_add_test(tc_core, tmp_format_5);
    tcase_add_test(tc_core, tmp_format_6);
    tcase_add_test(tc_core, tmp_format_7);
    tcase_add_test(tc_core, tmp_wrap_1);
    tcase_add_test(tc_core, tmp_wrap_2);
    tcase_add_test(tc_core, tmp_wrap_3);
    tcase_add_test(tc_core, tmp_wrap_4);
    tcase_add_test(tc_core, tmp_wrap_5);
    tcase_add_test(tc_core, tmp_wrap_6);
    tcase_add_test(tc_core, tmp_wrap_7);
    tcase_add_test(tc_core, tmp_strftime_1);
    tcase_add_test(tc_core, tmp_strftime_2);
    suite_add_tcase(s, tc_core);

    return s;
}
