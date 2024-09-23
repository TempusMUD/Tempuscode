#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <libpq-fe.h>
#include <unistd.h>
#include <glib.h>
#include <sys/stat.h>
#include <locale.h>

#include "utils.h"
#include "constants.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"
#include "str_builder.h"
#include "strutil.h"

extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int no_specials;
extern int restrict_logins;
extern int mini_mud;
extern int olc_lock;
extern int no_rent_check;
extern int no_initial_zreset;
extern int log_cmds;
extern bool nameserver_is_slow;  /* see config.c */
extern bool stress_test;
extern bool production_mode;
extern int main_port;
extern int reader_port;
extern int irc_port;
extern int proxy_port;

void ensure_environment(void);
void init_game(void);

int
main(int argc, char **argv)
{
    char *dir;
    const char *user = NULL, *group = NULL;
    const char *DATADIR_ENV_VAR = "CIRCLE_DATADIR";
    bool scheck = false;

    main_port = DFLT_PORT;

    tmp_string_init();

    setlocale(LC_ALL, "");

    int option_idx = 0;
    struct option long_options[] = {
        {"wizlock", no_argument, NULL, 'b'},
        {"stress", no_argument, NULL, 's'},
        {"minimud", no_argument, NULL, 'm'},
        {"check", no_argument, NULL, 'c'},
        {"quickboot", no_argument, NULL, 'q'},
        {"restrict", no_argument, NULL, 'r'},
        {"nozreset", no_argument, NULL, 'z'},
        {"nonameserver", no_argument, NULL, 'n'},
        {"logall", no_argument, NULL, 'l'},
        {"production", no_argument, NULL, 'P'},
        {"user", required_argument, NULL, 'u'},
        {"group", required_argument, NULL, 'g'},
        {"port", required_argument, NULL, 'p'},
        {NULL, 0, NULL, 0}
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
        case 's':
            stress_test = true;
            slog("Running in stress test mode");
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
            if (!is_number(optarg)) {
                fprintf(stderr, "%s: port must be numeric\n", argv[0]);
                safe_exit(EXIT_FAILURE);
            }
            main_port = atoi(optarg);
            if (main_port < 1 || main_port > 65535) {
                fprintf(stderr, "%s: port %d out of range\n", argv[0], main_port);
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

    if (chroot(dir) < 0) {
        perror("Warning: can't chroot to data directory");
    }

    if (setgid(gr->gr_gid) < 0) {
        perror("Couldn't change gid");
        exit(EXIT_FAILURE);
    }

    if (setuid(pw->pw_uid) < 0) {
        perror("Couldn't change uid");
        exit(EXIT_FAILURE);
    }

    slog("Running as %s:%s in %s", pw->pw_name, gr->gr_name, dir);

    free(dir);

    ensure_environment();

    reader_port = main_port + 1;
    irc_port = main_port + 2;
    proxy_port = main_port + 4;

    if (scheck) {
        void my_srand(unsigned long initial_seed);
        void verify_tempus_integrity(struct creature *ch);
        my_srand(time(NULL));
        boot_db();
        verify_tempus_integrity(NULL);
        slog("Press RETURN to continue.");
        while (getchar() != '\n') {
            ;
        }
        slog("Done.");
        safe_exit(EXIT_SUCCESS);
    } else {
        slog("Running game on port %d.", main_port);
        init_game();
    }

    return EXIT_SUCCESS;
}

void
ensure_environment(void)
{
    char path[PATH_MAX + 1];
    int idx;

    mkdir("players", 0755);
    mkdir("players/character", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, sizeof(path), "players/character/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/equipment", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, sizeof(path), "players/equipment/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/housing", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, sizeof(path), "players/housing/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/corpses", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, sizeof(path), "players/corpses/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("world", 0755);
    mkdir("world/mob", 0755);
    mkdir("world/mob/olc", 0755);
    mkdir("world/obj", 0755);
    mkdir("world/obj/olc", 0755);
    mkdir("world/wld", 0755);
    mkdir("world/wld/olc", 0755);
    mkdir("world/zon", 0755);
    mkdir("text", 0755);
    mkdir("text/dyn", 0755);
    mkdir("text/dyn/control", 0755);
    mkdir("log", 0755);
    mkdir("log/parsed", 0755);
}
