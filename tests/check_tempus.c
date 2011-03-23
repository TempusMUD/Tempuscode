#include <stdlib.h>
#include <check.h>

Suite *tmpstr_suite(void);

int
main(void)
{
    int number_failed;

    Suite *s = tmpstr_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
