#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <check.h>
#include <time.h>

void my_srand(unsigned long initial_seed);

Suite *movement_suite(void);
Suite *tmpstr_suite(void);
Suite *strutil_suite(void);
Suite *editor_suite(void);
Suite *object_suite(void);
Suite *player_io_suite(void);
Suite *prog_suite(void);
Suite *quest_suite(void);

int
main(void)
{
    int number_failed = 0;

    if (chdir("../../lib") < 0) {
        fprintf(stderr, "Couldn't change directory to lib: %s\n", strerror(errno));
    }

    my_srand(time(NULL));

    Suite *s = tmpstr_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = strutil_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = editor_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = movement_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = object_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = player_io_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = prog_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    s = quest_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
