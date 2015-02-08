#include <stdlib.h>
#include <stdbool.h>
#include <check.h>


START_TEST(test_parse_optional_range)
{
    struct {
        const char *str;
        bool valid;
        int start;
        int finish;
    } cases[] = {
        { "10", true, 10, 10 },
        { "10  ", true, 10, 10 },
        { "5-10", true, 5, 10 },
        { "10-5", true, 5, 10 },
        { "5 - 10", true, 5, 10 },
        { "-5", false, 0, 0 },
        { "5--10", false, 0, 0 },
        { "5a", false, 0, 0 },
        { "5-10a", false, 0, 0 },
        { "5 - a", false, 0, 0 },
        { NULL, false, 0, 0 },
    };
    bool parse_optional_range(const char *arg, int *start, int *finish);

    int start;
    int finish;

    for (int i = 0; cases[i].str != NULL; i++) {
        if (cases[i].valid) {
            ck_assert(parse_optional_range(cases[i].str, &start, &finish));
            ck_assert_int_eq(start, cases[i].start);
            ck_assert_int_eq(finish, cases[i].finish);
        } else {
            ck_assert(!parse_optional_range(cases[i].str, &start, &finish));
        }
    }
}
END_TEST

Suite *
editor_suite(void)
{
    Suite *s = suite_create("editor");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_parse_optional_range);
    suite_add_tcase(s, tc_core);

    return s;
}
