#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include "tmpstr.h"
#include "accstr.h"
#include "utils.h"
#include "db.h"

extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int no_specials;
extern bool restrict_logins;
extern int mini_mud;
extern int olc_lock;
extern int no_rent_check;
extern int no_initial_zreset;
extern int log_cmds;
extern int nameserver_is_slow;  /* see config.c */
extern bool production_mode;

void verify_environment(void);
void clear_world(void);
void init_game(int port);

int
main(int argc, char **argv)
{
    int port;
    char *dir;
    const char *user = NULL, *group = NULL;
    const char *DATADIR_ENV_VAR = "CIRCLE_DATADIR";
    bool scheck = false;

    port = DFLT_PORT;

    tmp_string_init();
    acc_string_init();

    int option_idx = 0;
    struct option long_options[] = {
        {"wizlock", no_argument, NULL, 'b'},
        {"minimud", no_argument, NULL, 'm'},
        {"check", no_argument, NULL, 'c'},
        {"quickboot", no_argument, NULL, 'q'},
        {"restrict", no_argument, NULL, 'r'},
        {"nospecials", no_argument, NULL, 's'},
        {"noolc", no_argument, NULL, 'o'},
        {"nozreset", no_argument, NULL, 'z'},
        {"nonameserver", no_argument, NULL, 'n'},
        {"logall", no_argument, NULL, 'l'},
        {"production", no_argument, NULL, 'P'},
        {"user", required_argument, NULL, 'u'},
        {"group", required_argument, NULL, 'g'},
        {"port", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };

    char c;
    opterr = 1;
    while ((c = getopt_long(argc, argv, "bmcqrsoznlPp:u:g:",
                long_options, &option_idx)) != -1) {
        switch (c) {
        case 'b':
            restrict_logins = 50;
            slog("Wizlock 50");
            break;
        case 'm':
            mini_mud = 1;
            no_rent_check = 1;
            slog("Running in minimized mode & with no rent check and olc lock.");
            break;
        case 'c':
            scheck = 1;
            slog("Syntax check mode enabled.");
            break;
        case 'q':
            no_rent_check = 1;
            slog("Quick boot mode -- rent check supressed.");
            break;
        case 'r':
            restrict_logins = 1;
            slog("Restricting game -- no new players allowed.");
            break;
        case 's':
            no_specials = 1;
            slog("Suppressing assignment of special routines.");
            break;
        case 'o':
            olc_lock = 1;
            slog("Locking olc.");
            break;
        case 'z':
            no_initial_zreset = 1;
            slog("Bypassing initial zone resets.");
            break;
        case 'n':
            nameserver_is_slow = 1;
            slog("Disabling nameserver.");
            break;
        case 'l':
            log_cmds = true;
            slog("Enabling log_cmds.");
            break;
        case 'P':
            production_mode = true;
            slog("Running in production mode");
            break;
        case 'u':
            user = optarg;
            break;
        case 'g':
            group = optarg;
            break;
        case 'p':
            if (!isnumber(optarg)) {
                fprintf(stderr, "%s: port must be numeric\n", argv[0]);
                safe_exit(EXIT_FAILURE);
            }
            port = atoi(optarg);
            if (port < 1 || port > 65535) {
                fprintf(stderr, "%s: port %d out of range\n", argv[0], port);
                safe_exit(EXIT_FAILURE);
            }
            break;
        default:
            safe_exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        dir = getenv(DATADIR_ENV_VAR);

    } else {
        dir = argv[optind];
    }

    if (!dir) {
        fprintf(stderr,
                "%s: data directory must be specified by %s environment variable or on command line\n",
                argv[0], DATADIR_ENV_VAR);
        safe_exit(EXIT_FAILURE);
    }

    dir = canonicalize_file_name(dir);

    if (chdir(dir) < 0) {
        perror("Fatal error changing to data directory");
        safe_exit(EXIT_FAILURE);
    }

    struct passwd *pw = NULL;
    struct group *gr = NULL;

    if (group) {
        gr = getgrnam(group);
        if (!gr) {
            perror("Couldn't find group");
            exit(EXIT_FAILURE);
        }
    } else {
        gr = getgrgid(getegid());
    }

    if (user) {
        pw = getpwnam(user);
        if (!pw) {
            perror("Couldn't find user");
            exit(EXIT_FAILURE);
        }
    } else {
        pw = getpwuid(geteuid());
    }

    if (chroot(dir) < 0)
        perror("Warning: can't chroot to data directory");

    if (setgid(gr->gr_gid) < 0) {
        perror("Couldn't change gid");
        exit(EXIT_FAILURE);
    }

    if (setuid(pw->pw_uid) < 0) {
        perror("Couldn't change uid");
        exit(EXIT_FAILURE);
    }

    slog("Running as %s:%s in %s", pw->pw_name, gr->gr_name, dir);

    verify_environment();

    if (scheck) {
        void my_srand(unsigned long initial_seed);
        void verify_tempus_integrity(struct creature *ch);
        my_srand(time(0));
        boot_db();
        verify_tempus_integrity(NULL);
        slog("Press RETURN to continue.");
        while (getchar() != '\n')
            ;
        slog("Done.");
        clear_world();
        safe_exit(EXIT_SUCCESS);
    } else {
        slog("Running game on port %d.", port);
        init_game(port);
    }

    clear_world();

    return EXIT_SUCCESS;
}

