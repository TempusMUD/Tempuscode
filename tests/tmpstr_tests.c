#include <stdlib.h>
#include <check.h>
#include "tmpstr.h"

static const char *bit_descs[32] = {
    "BIT_00", "BIT_01", "BIT_02", "BIT_03", "BIT_04",
    "BIT_05", "BIT_06", "BIT_07", "BIT_08", "BIT_09",
    "BIT_10", "BIT_11", "BIT_12", "BIT_13", "BIT_14",
    "BIT_15", "\n", 0
};

// Testing tmp_tolower
START_TEST(tmp_tolower_1)
{
    fail_unless(!strcmp(tmp_tolower("abcdef"), "abcdef"));
}
END_TEST

START_TEST(tmp_tolower_2)
{
    fail_unless(!strcmp(tmp_tolower("ABCDEF"), "abcdef"));
}
END_TEST
START_TEST(tmp_tolower_3)
{
    fail_unless(!strcmp(tmp_tolower("AbCdEf"), "abcdef"));
}
END_TEST
START_TEST(tmp_tolower_4)
{
    fail_unless(!strcmp(tmp_tolower("aBcDeF"), "abcdef"));
}
END_TEST
START_TEST(tmp_tolower_5)
{
    fail_unless(!strcmp(tmp_tolower("123456"), "123456"));
}
END_TEST
START_TEST(tmp_tolower_6)
{
    fail_unless(!strcmp(tmp_tolower(""), ""));
}
END_TEST

// Testing tmp_gsub
START_TEST(tmp_gsub_1)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "", ""), "abcdef"));
}
END_TEST
START_TEST(tmp_gsub_2)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "ghi", ""), "abcdef"));
}
END_TEST
START_TEST(tmp_gsub_3)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "c", "ghi"), "abghidef"));
}
END_TEST
START_TEST(tmp_gsub_4)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "cde", "g"), "abgf"));
}
END_TEST
START_TEST(tmp_gsub_5)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "abc", ""), "def"));
}
END_TEST
START_TEST(tmp_gsub_6)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "abc", "g"), "gdef"));
}
END_TEST
START_TEST(tmp_gsub_7)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "def", ""), "abc"));
}
END_TEST
START_TEST(tmp_gsub_8)
{
    fail_unless(!strcmp(tmp_gsub("abcdef", "def", "g"), "abcg"));
}
END_TEST
START_TEST(tmp_gsub_9)
{
    fail_unless(!strcmp(tmp_gsub("", "", ""), ""));
}
END_TEST
START_TEST(tmp_gsub_10)
{
    fail_unless(!strcmp(tmp_gsub("ABCDEF", "def", "xxx"), "ABCDEF"));
}
END_TEST
START_TEST(tmp_gsub_11)
{
    fail_unless(!strcmp(tmp_gsub("ABCDEFEDCBA", "D", "xxx"), "ABCxxxEFExxxCBA"));
}
END_TEST

// Testing tmp_gsubi
START_TEST(tmp_gsubi_1)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "", ""), "abcdef"));
}
END_TEST
START_TEST(tmp_gsubi_2)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "ghi", ""), "abcdef"));
}
END_TEST
START_TEST(tmp_gsubi_3)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "c", "ghi"), "abghidef"));
}
END_TEST
START_TEST(tmp_gsubi_4)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "cde", "g"), "abgf"));
}
END_TEST
START_TEST(tmp_gsubi_5)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "abc", ""), "def"));
}
END_TEST
START_TEST(tmp_gsubi_6)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "abc", "g"), "gdef"));
}
END_TEST
START_TEST(tmp_gsubi_7)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "def", ""), "abc"));
}
END_TEST
START_TEST(tmp_gsubi_8)
{
    fail_unless(!strcmp(tmp_gsubi("abcdef", "def", "g"), "abcg"));
}
END_TEST
START_TEST(tmp_gsubi_9)
{
    fail_unless(!strcmp(tmp_gsubi("", "", ""), ""));
}
END_TEST
START_TEST(tmp_gsubi_10)
{
    fail_unless(!strcmp(tmp_gsubi("ABCDEF", "def", "xxx"), "ABCxxx"));
}
END_TEST
START_TEST(tmp_gsubi_11)
{
    fail_unless(!strcmp(tmp_gsubi("ABCDEFEDCBA", "d", "xxx"), "ABCxxxEFExxxCBA"));
}
END_TEST

// Testing tmp_sqlescape
START_TEST(tmp_sqlescape_1)
{
    fail_unless(!strcmp(tmp_sqlescape("abcd'ef"), "abcd''ef"));
}
END_TEST
START_TEST(tmp_sqlescape_2)
{
    fail_unless(!strcmp(tmp_sqlescape("abcd\\'ef"), "abcd\\\\''ef"));
}
END_TEST
START_TEST(tmp_sqlescape_3)
{
    fail_unless(!strcmp(tmp_sqlescape("abcd\\ef"), "abcd\\\\ef"));
}
END_TEST

// Testing tmp_printbits
START_TEST(tmp_printbits_1)
{
    fail_unless(!strcmp(tmp_printbits(0x1, bit_descs), "BIT_00"));
}
END_TEST
START_TEST(tmp_printbits_2)
{
    fail_unless(!strcmp(tmp_printbits(0x2, bit_descs), "BIT_01"));
}
END_TEST
START_TEST(tmp_printbits_3)
{
    fail_unless(!strcmp(tmp_printbits(0x3, bit_descs), "BIT_00 BIT_01"));
}
END_TEST
START_TEST(tmp_printbits_4)
{
    fail_unless(!strcmp(tmp_printbits(0x4, bit_descs), "BIT_02"));
}
END_TEST
START_TEST(tmp_printbits_5)
{
    fail_unless(!strcmp(tmp_printbits(0x5, bit_descs), "BIT_00 BIT_02"));
}
END_TEST
START_TEST(tmp_printbits_6)
{
    fail_unless(!strcmp(tmp_printbits(0x6, bit_descs), "BIT_01 BIT_02"));
}
END_TEST
START_TEST(tmp_printbits_7)
{
    fail_unless(!strcmp(tmp_printbits(0x7, bit_descs), "BIT_00 BIT_01 BIT_02"));
}
END_TEST
START_TEST(tmp_printbits_8)
{
    fail_unless(!strcmp(tmp_printbits(0x20000, bit_descs), ""));
}
END_TEST

// Testing tmp_substr
START_TEST(tmp_substr_1)
{
    fail_unless(!strcmp(tmp_substr("abcdef", 0, -1), "abcdef"));
}
END_TEST
START_TEST(tmp_substr_2)
{
    fail_unless(!strcmp(tmp_substr("abcdef", 0, 2), "abc"));
}
END_TEST
START_TEST(tmp_substr_3)
{
    fail_unless(!strcmp(tmp_substr("abcdef", 1, 4), "bcde"));
}
END_TEST
START_TEST(tmp_substr_4)
{
    fail_unless(!strcmp(tmp_substr("abcdef", 3, -1), "def"));
}
END_TEST
START_TEST(tmp_substr_5)
{
    fail_unless(!strcmp(tmp_substr("abcdef", -1, -1), "f"));
}
END_TEST
START_TEST(tmp_substr_6)
{
    fail_unless(!strcmp(tmp_substr("abcdef", 0, 0), "a"));
}
END_TEST
START_TEST(tmp_substr_7)
{
    fail_unless(!strcmp(tmp_substr("abcdef", -5, -1), "bcdef"));
}
END_TEST

// Testing tmp_trim
START_TEST(tmp_trim_1)
{
    fail_unless(!strcmp(tmp_trim(""), ""));
}
END_TEST
START_TEST(tmp_trim_2)
{
    fail_unless(!strcmp(tmp_trim("abcdef"), "abcdef"));
}
END_TEST
START_TEST(tmp_trim_3)
{
    fail_unless(!strcmp(tmp_trim("abcdef"), "abcdef"));
}
END_TEST
START_TEST(tmp_trim_4)
{
    fail_unless(!strcmp(tmp_trim("   abcdef"), "abcdef"));
}
END_TEST
START_TEST(tmp_trim_5)
{
    fail_unless(!strcmp(tmp_trim("abcdef   "), "abcdef"));
}
END_TEST
START_TEST(tmp_trim_6)
{
    fail_unless(!strcmp(tmp_trim("   abcdef   "), "abcdef"));
}
END_TEST
START_TEST(tmp_trim_7)
{
    fail_unless(!strcmp(tmp_trim("   abc def   "), "abc def"));
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
    suite_add_tcase(s, tc_core);

    return s;
}
