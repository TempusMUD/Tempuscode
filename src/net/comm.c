/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: comm.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "macros.h"
#include "structs.h"
#include "spells.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "security.h"
#include "handler.h"
#include "db.h"
#include "editor.h"
#include "house.h"
#include "screen.h"
#include "vehicle.h"
#include "help.h"
#include "desc_data.h"
#include "tmpstr.h"
#include "accstr.h"
#include "players.h"
#include "prog.h"
#include "quest.h"
#include "language.h"
#include "ban.h"
#include "char_class.h"
#include "quest.h"

/* externs */
extern struct help_collection *Help;
extern int mini_mud;
extern int olc_lock;
extern unsigned int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;
extern struct obj_data *cur_car;
extern struct zone_data *default_quad_zone;
extern struct obj_data *object_list;
int main_port;
int reader_port;
int restrict_logins = false;
bool production_mode = false;   // Run in production mode

/* local globals */
struct descriptor_data *descriptor_list = NULL; /* master desc list */
struct txt_block *bufpool = 0;  /* pool of large output buffers */
int buf_largecount = 0;         /* # of large buffers which exist */
int buf_overflows = 0;          /* # of overflows of output */
int buf_switches = 0;           /* # of switches from small to large buf */
int circle_shutdown = 0;        /* clean shutdown */
int circle_reboot = 0;          /* reboot the game after a shutdown */
int no_specials = 0;            /* Suppress ass. of special routines */
int avail_descs = 0;            /* max descriptors available */
int tics = 0;                   /* for extern checkpointing */
int log_cmds = 0;               /* log cmds */
bool stress_test = false;       /* stress test the codebase */
int shutdown_count = -1;        /* shutdown countdown */
int shutdown_idnum = -1;        /* idnum of person calling shutdown */
int shutdown_mode = SHUTDOWN_NONE;  /* what type of shutdown */
bool suppress_output = false;

struct last_command_data last_cmd[NUM_SAVE_CMDS];

extern int nameserver_is_slow;  /* see config.c */
extern int auto_save;           /* see config.c */
extern int autosave_time;       /* see config.c */
struct timeval null_time;       /* zero-valued time structure */

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased, int length);
void flush_q(struct txt_q *queue);
void init_game(void);
void signal_setup(void);
void game_loop(int main_listener, int reader_listener);
int init_socket(int port);
int new_descriptor(int s, int port);
int get_avail_descs(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
struct timeval timediff(struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(int s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
void send_prompt(struct descriptor_data *point);
void bamf_quad_damage(void);
void descriptor_update(void);
int write_to_descriptor(int desc, const char *txt);

/* extern fcnts */
void boot_world(void);
void affect_update(void);       /* In spells.c */
void obj_affect_update(void);
void mobile_activity(void);
void mobile_spec(void);
void burn_update(void);
void dynamic_object_pulse();
void flow_room(int pulse);
void path_activity();
void editor(struct descriptor_data *d, char *buffer);
void perform_violence(void);
void show_string(struct descriptor_data *d);
void weather_and_time(void);
void autosave_zones(int SAVE_TYPE);
void mem_cleanup(void);
void retire_trails(void);
void set_desc_state(int state, struct descriptor_data *d);
void save_quests();             // quests.cc - saves quest data
void save_all_players();

/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

/* Init sockets, run game, and cleanup sockets */
void
init_game(void)
{
    int main_listener, reader_listener;
    void my_srand(unsigned long initial_seed);
    void verify_tempus_integrity(struct creature *ch);

    my_srand(time(0));
    boot_db();

    slog("Testing internal integrity");
    verify_tempus_integrity(NULL);

    slog("Opening mother connection.");
    main_listener = init_socket(main_port);
    reader_listener = init_socket(reader_port);

    avail_descs = get_avail_descs();

    slog("Signal trapping.");
    signal_setup();

    slog("Entering game loop.");

    game_loop(main_listener, reader_listener);

    slog("Closing all sockets.");
    close(main_listener);
    close(reader_listener);
    while (descriptor_list)
        close_socket(descriptor_list);

    save_quests();

    if (circle_reboot) {
        slog("Rebooting.");
        exit(52);               /* what's so great about HHGTTG, anyhow? */
    }
    slog("Normal termination of game.");
}

/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int
init_socket(int port)
{
    int s, opt;
    struct sockaddr_in sa;

    /*
     * Should the first argument to socket() be AF_INET or PF_INET?  I don't
     * know, take your pick.  PF_INET seems to be more widely adopted, and
     * Comer (_Internetworking with TCP/IP_) even makes a point to say that
     * people erroneously use AF_INET with socket() when they should be using
     * PF_INET.  However, the man pages of some systems indicate that AF_INET
     * is correct; some such as ConvexOS even say that you can use either one.
     * All implementations I've seen define AF_INET and PF_INET to be the same
     * number anyway, so ths point is (hopefully) moot.
     */

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Create socket");
        safe_exit(EXIT_FAILURE);
    }
#if defined(SO_SNDBUF)
    opt = LARGE_BUFSIZE + GARBAGE_SPACE;
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt SNDBUF");
        safe_exit(EXIT_FAILURE);
    }
#endif

#if defined(SO_REUSEADDR)
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt REUSEADDR");
        safe_exit(EXIT_FAILURE);
    }
#endif

#if defined(SO_REUSEPORT)
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt REUSEPORT");
        safe_exit(EXIT_FAILURE);
    }
#endif

#if defined(SO_LINGER)
    {
        struct linger ld;

        ld.l_onoff = 0;
        ld.l_linger = 0;
        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0) {
            perror("setsockopt LINGER");
            safe_exit(EXIT_FAILURE);
        }
    }
#endif

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(s);
        safe_exit(EXIT_FAILURE);
    }
    nonblock(s);
    listen(s, 5);
    return s;
}

int
get_avail_descs(void)
{
    unsigned int max_descs = 0;
    struct rlimit limit;

    getrlimit(RLIMIT_NOFILE, &limit);
    max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
    limit.rlim_cur = max_descs;
    setrlimit(RLIMIT_NOFILE, &limit);

    max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

    if (max_descs <= 0) {
        slog("Non-positive max player limit!");
        safe_exit(EXIT_FAILURE);
    }
    slog("Setting player limit to %d.", max_descs);
    return max_descs;
}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void
game_loop(int main_listener, int reader_listener)
{
    fd_set input_set, output_set, exc_set;
    struct timeval last_time, now, timespent, timeout, opt_time;
    struct descriptor_data *d, *next_d;
    int pulse = 0, mins_since_crashsave = 0, maxdesc;

    /* initialize various time values */
    null_time.tv_sec = 0;
    null_time.tv_usec = 0;
    opt_time.tv_usec = OPT_USEC;
    opt_time.tv_sec = 0;
    gettimeofday(&last_time, (struct timezone *)0);

    /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
    while (!circle_shutdown) {
        /* Set up the input, output, and exception sets for select(). */
        FD_ZERO(&input_set);
        FD_ZERO(&output_set);
        FD_ZERO(&exc_set);
        FD_SET(main_listener, &input_set);
        FD_SET(reader_listener, &input_set);
        maxdesc = MAX(main_listener, reader_listener);
        for (d = descriptor_list; d; d = d->next) {
            if (d->descriptor > maxdesc)
                maxdesc = d->descriptor;
            FD_SET(d->descriptor, &input_set);
            FD_SET(d->descriptor, &output_set);
            FD_SET(d->descriptor, &exc_set);
        }

        // If we're not doing a stress test, we want to slow the mud
        // down to 1/10th of a second per pulse, if possible.  If we
        // ARE doing a stress test, this is skipped so we can load the
        // mud as fast as possible.
        if (!stress_test) {
            do {
                errno = 0;          // clear error condition

                // figure out for how long we have to sleep
                gettimeofday(&now, (struct timezone *)0);
                timespent = timediff(&now, &last_time);
                timeout = timediff(&opt_time, &timespent);

                if (!production_mode && !timeout.tv_sec && !timeout.tv_usec)
                    slog("WARNING: Last pulse %d took %lu.%06lu seconds",
                         pulse, timespent.tv_sec, timespent.tv_usec);

                // sleep until the next 0.1 second mark
                if (usleep(timeout.tv_sec * 1000000 + timeout.tv_usec) < 0)
                    if (errno != EINTR) {
                        perror("Select sleep");
                        safe_exit(EXIT_FAILURE);
                    }
            } while (errno);
        }
        /* record the time for the next pass */
        gettimeofday(&last_time, (struct timezone *)0);

        /* poll (without blocking) for new input, output, and exceptions */
        if (select(maxdesc + 1, &input_set, &output_set, &exc_set,
                &null_time) < 0) {
            if (errno == EINTR)
                continue;
            perror("Select poll");
            return;
        }
        /* New connection waiting for us? */
        if (FD_ISSET(main_listener, &input_set))
            new_descriptor(main_listener, main_port);
        if (FD_ISSET(reader_listener, &input_set))
            new_descriptor(reader_listener, reader_port);

        /* kick out the freaky folks in the exception set */
        for (d = descriptor_list; d; d = d->next) {
            if (FD_ISSET(d->descriptor, &exc_set)) {
                FD_CLR(d->descriptor, &input_set);
                FD_CLR(d->descriptor, &output_set);
                set_desc_state(CXN_DISCONNECT, d);
            }
        }

        /* process descriptors with input pending */
        for (d = descriptor_list; d; d = d->next) {
            if (d->input_mode != CXN_DISCONNECT &&
                FD_ISSET(d->descriptor, &input_set))
                if (process_input(d) < 0)
                    set_desc_state(CXN_DISCONNECT, d);
        }

        /* process commands we just read from process_input */
        for (d = descriptor_list; d; d = d->next) {
            if (d->input_mode != CXN_DISCONNECT) {
                if (--(d->wait) > 0)
                    continue;
                handle_input(d);
            }
        }

        /* save all players that need to be immediately */
        for (d = descriptor_list; d; d = d->next) {
            if (d->input_mode != CXN_DISCONNECT &&
                d->creature &&
                IS_PLAYING(d) &&
                IS_PC(d->creature) && PLR_FLAGGED(d->creature, PLR_CRASH))
                crashsave(d->creature);
        }

        /* handle heartbeat stuff */
        /* Note: pulse now changes every 0.10 seconds  */

        pulse++;

        if (stress_test) {
            void random_mob_activity(void);

            slog("Pulse %d", pulse);
            random_mob_activity();
        }

        /* Update progs triggered by user input that have not run yet */
        prog_update_pending();

        if (!(pulse % (PULSE_ZONE)))
            zone_update();
        if (!((pulse + 1) % PULSE_MOBILE))
            mobile_activity();
        if (!((pulse) % PULSE_MOBILE_SPEC))
            mobile_spec();
        if (!(pulse % SEG_VIOLENCE))
            perform_violence();
        if (!((pulse + 3) % FIRE_TICK))
            burn_update();

        if (!(pulse % PULSE_FLOWS))
            flow_room(pulse);
        else if (!((pulse + 1) % PULSE_FLOWS))
            dynamic_object_pulse();
        else if (!((pulse + 2) % PULSE_FLOWS))
            path_activity();
        else if (!((pulse + 3) % PULSE_FLOWS))
            prog_update();

        if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
            weather_and_time();
            affect_update();
            obj_affect_update();
            point_update();
            descriptor_update();
            help_collection_sync(help);
            save_quests();
        }
        if (auto_save) {
            if (!(pulse % (60 * PASSES_PER_SEC))) { /* 1 minute */
                if (++mins_since_crashsave >= autosave_time) {
                    mins_since_crashsave = 0;
                    save_all_players();
                }
                collect_housing_rent();
                save_houses();
                update_objects_housed_count();
            }
        }

        if (!(pulse % (300 * PASSES_PER_SEC)))  // 5 minutes
            record_usage();

        if (!(pulse % (130 * PASSES_PER_SEC)))  // 2.1 minutes
            retire_trails();
        if (!olc_lock && !(pulse % (900 * PASSES_PER_SEC))) // 15 minutes
            autosave_zones(ZONE_AUTOSAVE);

        if (!(pulse % (666 * PASSES_PER_SEC)))
            bamf_quad_damage();

        if (pulse >= (30 * 60 * PASSES_PER_SEC))    // 30 minutes
            pulse = 0;

        if (shutdown_count >= 0 && !(pulse % (PASSES_PER_SEC))) {
            shutdown_count--;   /* units of seconds */

            if (shutdown_count == 10)
                send_to_all(":: Tempus REBOOT in 10 seconds ::\r\n");
            else if (shutdown_count == 30)
                send_to_all(":: Tempus REBOOT in 30 seconds ::\r\n");
            else if (shutdown_count && !(shutdown_count % 60)) {
                sprintf(buf, ":: Tempus REBOOT in %d minute%s ::\r\n",
                    shutdown_count / 60, shutdown_count == 60 ? "" : "s");
                send_to_all(buf);
            } else if (shutdown_count <= 0) {
                save_all_players();
                collect_housing_rent();
                save_houses();
                xmlCleanupParser();

                autosave_zones(ZONE_RESETSAVE);
                send_to_all(":: Tempus REBOOTING ::\r\n\r\n"
                    "You feel your reality fading, as the universe spins away\r\n"
                    "before your eyes and the icy cold of nothingness settles\r\n"
                    "into your flesh.  With a jolt, you feel the thread snap,\r\n"
                    "severing your mind from the world known as Tempus.\r\n\r\n");
                if (shutdown_mode == SHUTDOWN_DIE
                    || shutdown_mode == SHUTDOWN_PAUSE) {
                    send_to_all
                        ("Shutting down for maintenance, try again in half an hour.\r\n");
                    if (shutdown_mode == SHUTDOWN_DIE)
                        touch("../.killscript");
                    else
                        touch("../pause");
                } else {
                    send_to_all
                        ("Rebooting now, we will be back online in a few minutes.\r\n");
                    touch("../.fastboot");
                }

                send_to_all
                    ("Please visit our website at http://tempusmud.com\r\n");

                slog("(GC) %s called by %s EXECUTING.",
                    (shutdown_mode == SHUTDOWN_DIE
                        || shutdown_mode ==
                        SHUTDOWN_DIE) ? "Shutdown" : "Reboot",
                    player_name_by_idnum(shutdown_idnum));
                circle_shutdown = true;
                for (d = descriptor_list; d; d = d->next)
                    if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
                        process_output(d);
            }
        }

        /* give each descriptor an appropriate prompt */
        for (d = descriptor_list; d; d = d->next) {
            // New output crlf
            if (d->creature && d->output[0]
                && d->account->compact_level < 2)
                SEND_TO_Q("\r\n", d);
            if (d->need_prompt) {
                send_prompt(d);
                // After prompt crlf
                if (d->creature && d->output[0]
                    && (d->account->compact_level == 0
                        || d->account->compact_level == 2))
                    SEND_TO_Q("\r\n", d);
                d->need_prompt = 0;
            }

        }

        /* garbage collect dead creatures */
        void extract_creature(struct creature *ch, enum cxn_state con_state);

        GList *next;
        for (GList *it = creatures;it;it = next) {
            next = it->next;
            struct creature *tch = it->data;
            if (is_dead(tch)) {
                if (IS_NPC(tch))
                    g_hash_table_remove(creature_map, GINT_TO_POINTER(-NPC_IDNUM(tch)));
                else
                    g_hash_table_remove(creature_map, GINT_TO_POINTER(GET_IDNUM(tch)));

                if (tch->in_room)
                    extract_creature(tch, CXN_AFTERLIFE);

                // pull the char from the various lists
                creatures = g_list_delete_link(creatures, it);
            }
        }

        /* send queued output out to the operating system (ultimately to user) */
        for (d = descriptor_list; d; d = d->next) {
            if (FD_ISSET(d->descriptor, &output_set) && *(d->output)) {
                if (process_output(d) < 0)
                    set_desc_state(CXN_DISCONNECT, d);
            }
        }

        // Enforce an autoban disconnection
        for (d = descriptor_list; d; d = d->next) {
            if (d->ban_dc_counter) {
                d->ban_dc_counter--;
                if (!d->ban_dc_counter)
                    set_desc_state(CXN_DISCONNECT, d);
            }
        }

        /* kick out folks in the CXN_DISCONNECT state */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (d->input_mode == CXN_DISCONNECT)
                close_socket(d);
        }

        update_unique_id();
        tics++;                 /* tics since last checkpoint signal */
        sql_gc_queries();
        tmp_gc_strings();
        suppress_output = false;    // failsafe
    }                           /* while (!circle_shutdown) */
}

/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
struct timeval
timediff(struct timeval *a, struct timeval *b)
{
    struct timeval rslt;

    if (a->tv_sec < b->tv_sec)
        return null_time;
    else if (a->tv_sec == b->tv_sec) {
        if (a->tv_usec < b->tv_usec)
            return null_time;
        else {
            rslt.tv_sec = 0;
            rslt.tv_usec = a->tv_usec - b->tv_usec;
            return rslt;
        }
    } else {                    /* a->tv_sec > b->tv_sec */
        rslt.tv_sec = a->tv_sec - b->tv_sec;
        if (a->tv_usec < b->tv_usec) {
            rslt.tv_usec = a->tv_usec + 1000000 - b->tv_usec;
            rslt.tv_sec--;
        } else
            rslt.tv_usec = a->tv_usec - b->tv_usec;
        return rslt;
    }
}

void
record_usage(void)
{
    int sockets_input_mode = 0, sockets_playing = 0;
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next) {
        sockets_input_mode++;
        if (d->input_mode == CXN_PLAYING)
            sockets_playing++;
    }

    slog("nusage: %-3d sockets input_mode, %-3d sockets playing",
        sockets_input_mode, sockets_playing);

#ifdef RUSAGE
    {
        struct rusage ru;

        getrusage(0, &ru);
        slog("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld", ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
    }
#endif

}

void
write_to_q(char *txt, struct txt_q *queue, int aliased)
{
    struct txt_block *new_txt_block;

    CREATE(new_txt_block, struct txt_block, 1);
    CREATE(new_txt_block->text, char, strlen(txt) + 1);
    strcpy(new_txt_block->text, txt);
    new_txt_block->aliased = aliased;

    /* queue empty? */
    if (!queue->head) {
        new_txt_block->next = NULL;
        queue->head = queue->tail = new_txt_block;
    } else {
        queue->tail->next = new_txt_block;
        queue->tail = new_txt_block;
        new_txt_block->next = NULL;
    }
}

int
get_from_q(struct txt_q *queue, char *dest, int *aliased, int length)
{
    struct txt_block *tmp;

    /* queue empty? */
    if (!queue->head)
        return 0;

    tmp = queue->head;
    strncpy(dest, queue->head->text, length);
    *aliased = queue->head->aliased;
    queue->head = queue->head->next;
    free(tmp->text);
    free(tmp);
    return 1;
}

void
flush_q(struct txt_q *queue)
{
    struct txt_block *cur_blk, *next_blk;

    for (cur_blk = queue->head; cur_blk; cur_blk = next_blk) {
        next_blk = cur_blk->next;
        free(cur_blk->text);
        free(cur_blk);
    }
    queue->head = NULL;
}

/* Empty the queues before closing connection */
void
flush_queues(struct descriptor_data *d)
{
    if (d->large_outbuf) {
        d->large_outbuf->next = bufpool;
        bufpool = d->large_outbuf;
    }
    flush_q(&d->input);
}

void
write_to_output(const char *txt, struct descriptor_data *t)
{
    int size;

    size = strlen(txt);

    /* if we're in the overflow state already, ignore this new output */
    if (t->bufptr < 0)
        return;

    if (suppress_output)
        return;

    if (!t->need_prompt && (!t->creature
            || PRF2_FLAGGED(t->creature, PRF2_AUTOPROMPT))) {
        t->need_prompt = true;
        // New output crlf
        if (!t->account
            || t->account->compact_level == 1
            || t->account->compact_level == 3)
            write_to_output("\r\n", t);
    }

    /* if we have enough space, just write to buffer and that's it! */
    if (t->bufspace >= size) {
        strcpy(t->output + t->bufptr, txt);
        t->bufspace -= size;
        t->bufptr += size;
    } else {                    /* otherwise, try switching to a lrg buffer */
        if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
            /*
             * we're already using large buffer, or even the large buffer isn't big
             * enough -- switch to overflow state
             */
            t->bufptr = -1;
            buf_overflows++;
            return;
        }
        buf_switches++;

        /* if the pool has a buffer in it, grab it */
        if (bufpool != NULL) {
            t->large_outbuf = bufpool;
            bufpool = bufpool->next;
        } else {                /* else create a new one */
            CREATE(t->large_outbuf, struct txt_block, 1);
            CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
            buf_largecount++;
        }

        strcpy(t->large_outbuf->text, t->output);   /* copy to big buffer */
        t->output = t->large_outbuf->text;  /* make big buffer primary */
        strcat(t->output, txt); /* now add new text */

        /* calculate how much space is left in the buffer */
        t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

        /* set the pointer for the next write */
        t->bufptr = strlen(t->output);
    }
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */

int
new_descriptor(int s, int port)
{
    int desc, sockets_input_mode = 0;
    unsigned int i;
    static int last_desc = 0;   /* last descriptor number */
    struct descriptor_data *newd;
    struct sockaddr_in peer;
    struct hostent *from;
    extern const char *GREETINGS[];

    /* accept the new connection */
    i = sizeof(peer);
    if ((desc = accept(s, (struct sockaddr *)&peer, &i)) < 0) {
        perror("accept");
        return -1;
    }
    /* keep it from blocking */
    nonblock(desc);

    /* make sure we have room for it */
    for (newd = descriptor_list; newd; newd = newd->next)
        sockets_input_mode++;

    if (sockets_input_mode >= avail_descs) {
        write_to_descriptor(desc,
            "Sorry, Tempus is full right now... try again later!  :-)\r\n");
        close(desc);
        return 0;
    }
    /* create a new descriptor */
    CREATE(newd, struct descriptor_data, 1);

    /* find the sitename */
    if (nameserver_is_slow) {
        from = NULL;
    } else {
        from = gethostbyaddr((char *)&peer.sin_addr,
                             sizeof(peer.sin_addr),
                             AF_INET);
    }

    if (from) {
        strncpy(newd->host, from->h_name, HOST_LENGTH);
    } else {
        strncpy(newd->host, inet_ntoa(peer.sin_addr), HOST_LENGTH);
    }
    *(newd->host + HOST_LENGTH) = '\0';

    /* determine if the site is banned */
    if (check_ban_all(desc, newd->host)) {
        close(desc);
        free(newd);
        return 0;
    }

    int bantype = isbanned(newd->host, buf2);

    /* Log new connections - probably unnecessary, but you may want it */
    mlog(ROLE_ADMINBASIC, LVL_GOD, CMP, true,
         "New connection from [%s]%s%s on %d",
         newd->host,
         (bantype == BAN_SELECT) ? "(SELECT BAN)" : "",
         (bantype == BAN_NEW) ? "(NEWBIE BAN)" : "",
         port);

    /* initialize descriptor data */
    newd->descriptor = desc;
    STATE(newd) = CXN_ACCOUNT_LOGIN;
    newd->wait = 1;
    newd->output = newd->small_outbuf;
    newd->bufspace = SMALL_BUFSIZE - 1;
    newd->next = descriptor_list;
    newd->login_time = time(0);
    newd->text_editor = NULL;
    newd->idle = 0;
    newd->ban_dc_counter = 0;
    newd->is_blind = (port == reader_port);

    if (++last_desc == 10000)
        last_desc = 1;
    newd->desc_num = last_desc;

    /* prepend to list */
    descriptor_list = newd;
    if (mini_mud) {
        SEND_TO_Q("(testmud)", newd);
    } else if (newd->is_blind) {
        SEND_TO_Q("Welcome to Tempus MUD!\r\n", newd);
    } else {
        // This text is printed just before the screen clear, so most
        // people won't even see it.  Screen readers will read it out
        // loud, though.
        SEND_TO_Q(tmp_sprintf("If you use a screen reader, you'll want to use port %d",
                              reader_port),
                  newd);
        SEND_TO_Q("\033[H\033[J", newd);
        if (number(0, 99)) {
            if (number(0, 10))
                SEND_TO_Q(GREETINGS[1], newd);
            else
                SEND_TO_Q(GREETINGS[2], newd);
        } else {
            // send original greeting 1/100th of the time
            SEND_TO_Q(GREETINGS[0], newd);
        }
    }
    return 0;
}

int
process_output(struct descriptor_data *d)
{
    static int result;

    result = write_to_descriptor(d->descriptor, d->output);

    /* if we're in the overflow state, notify the user */
    if (!result && d->bufptr < 0)
        result = write_to_descriptor(d->descriptor, "**OVERFLOW**");

    /* handle snooping: prepend "% " and send to snooper */
    if (d->snoop_by && d->creature && !IS_NPC(d->creature)) {
        for (GList * x = d->snoop_by; x; x = x->next) {
            struct descriptor_data *td = x->data;
            SEND_TO_Q(CCRED(td->creature, C_NRM), td);
            SEND_TO_Q("{ ", td);
            SEND_TO_Q(CCNRM(td->creature, C_NRM), td);
            SEND_TO_Q(d->output, td);
            SEND_TO_Q(CCRED(td->creature, C_NRM), td);
            SEND_TO_Q(" } ", td);
            SEND_TO_Q(CCNRM(td->creature, C_NRM), td);
        }
    }
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (d->large_outbuf) {
        d->large_outbuf->next = bufpool;
        bufpool = d->large_outbuf;
        d->large_outbuf = NULL;
        d->output = d->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    d->bufspace = SMALL_BUFSIZE - 1;
    d->bufptr = 0;
    *(d->output) = '\0';

    return result;
}

int
write_to_descriptor(int desc, const char *txt)
{
    int total, bytes_written;

    total = strlen(txt);

    do {
        if ((bytes_written = write(desc, txt, total)) < 0) {
#ifdef EWOULDBLOCK
            if (errno == EWOULDBLOCK)
                errno = EAGAIN;
#endif
            if (errno == EAGAIN)
                slog("process_output: socket write would block, about to close");
            else
                perror("Write to socket");
            return -1;
        } else {
            txt += bytes_written;
            total -= bytes_written;
        }
    } while (total > 0);

    return 0;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int
process_input(struct descriptor_data *t)
{
    int buf_length, bytes_read, space_left, failed_subst;
    char *ptr, *read_point, *write_point, *nl_pos = NULL;
    char tmp[MAX_INPUT_LENGTH + 8];

    /* first, find the point where we left off reading data */
    buf_length = strlen(t->inbuf);
    read_point = t->inbuf + buf_length;
    space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

    do {
        if (space_left <= 0) {
            slog("process_input: about to close connection: input overflow");
            return -1;
        }

        if ((bytes_read = read(t->descriptor, read_point, space_left)) < 0) {

#ifdef EWOULDBLOCK
            if (errno == EWOULDBLOCK)
                errno = EAGAIN;
#endif
            if (errno != EAGAIN) {
                perror("process_input: about to lose connection");
                return -1;      /* some error condition was encountered on
                                 * read */
            } else
                return 0;       /* the read would have blocked: just means no
                                 * data there */
        } else if (bytes_read == 0) {
            slog("EOF on socket read (connection broken by peer)");
            return -1;
        }
        /* at this point, we know we got some data from the read */

        *(read_point + bytes_read) = '\0';  /* terminate the string */

        /* search for a newline in the data we just read */
        for (ptr = read_point; *ptr && !nl_pos; ptr++)
            if (ISNEWL(*ptr))
                nl_pos = ptr;

        read_point += bytes_read;
        space_left -= bytes_read;
    } while (nl_pos == NULL);

    /*
     * okay, at this point we have at least one newline in the string; now we
     * can copy the formatted data to a new array for further processing.
     */

    read_point = t->inbuf;

    while (nl_pos != NULL) {
        write_point = tmp;
        space_left = MAX_INPUT_LENGTH - 1;

        for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
            if (*ptr == '\b') { /* handle backspacing */
                if (write_point > tmp) {
                    if (*(--write_point) == '$') {
                        write_point--;
                        space_left += 2;
                    } else
                        space_left++;
                }
            } else if (isascii(*ptr) && isprint(*ptr)) {
                *write_point++ = *ptr;
                space_left--;
            }
        }

        *write_point = '\0';

        if ((space_left <= 0) && (ptr < nl_pos)) {
            char buffer[MAX_INPUT_LENGTH + 64];

            sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
            if (write_to_descriptor(t->descriptor, buffer) < 0)
                return -1;
        }
        if (t->snoop_by && t->creature && !IS_NPC(t->creature)) {
            for (GList * x = t->snoop_by; x; x = x->next) {
                struct descriptor_data *td = x->data;
                SEND_TO_Q(CCRED(td->creature, C_NRM), td);
                SEND_TO_Q("[ ", td);
                SEND_TO_Q(CCNRM(td->creature, C_NRM), td);
                SEND_TO_Q(tmp, td);
                SEND_TO_Q(CCRED(td->creature, C_NRM), td);
                SEND_TO_Q(" ]\r\n", td);
                SEND_TO_Q(CCNRM(td->creature, C_NRM), td);
            }
        }

        failed_subst = 0;

        if (*tmp == '!' && STATE(t) != CXN_PW_VERIFY) {
            strcpy(tmp, t->last_input);
        } else if (*tmp == '^') {
            if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
                strcpy(t->last_input, tmp);
        } else
            strcpy(t->last_input, tmp);

        if (t->repeat_cmd_count > 300 &&
            (!t->creature || GET_LEVEL(t->creature) < LVL_ETERNAL)) {
            if (t->creature && t->creature->in_room) {
                act("SAY NO TO SPAM.\r\n"
                    "Begone oh you waster of electrons,"
                    " ye vile profaner of CPU time!", true, t->creature, 0, 0,
                    TO_ROOM);
                slog("SPAM-death on the queue!");
                return (-1);
            }
        }

        if (!failed_subst) {
            if (IS_PLAYING(t) && !strncmp("revo", tmp, 4)) {
                // We want all commands in the queue to be dumped immediately
                // This has to be here so we can bypass the normal order of
                // commands
                t->need_prompt = true;
                if (t->input.head) {
                    flush_q(&t->input);
                    send_to_desc(t, "You reconsider your rash plans.\r\n");
                    WAIT_STATE(t->creature, 1 RL_SEC);
                } else {
                    send_to_desc(t,
                        "You don't have any commands to revoke!\r\n");
                }
            } else
                write_to_q(tmp, &t->input, 0);
        }

        /* find the end of this line */
        while (ISNEWL(*nl_pos))
            nl_pos++;

        /* see if there's another newline in the input buffer */
        read_point = ptr = nl_pos;
        for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
            if (ISNEWL(*ptr))
                nl_pos = ptr;
    }

/* now move the rest of the buffer up to the beginning for the next pass */
    write_point = t->inbuf;
    while (*read_point)
        *(write_point++) = *(read_point++);
    *write_point = '\0';

    return 1;
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int
perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
    char new_str[MAX_INPUT_LENGTH + 5];

    char *first, *second, *strpos;

/*
 * first is the position of the beginning of the first string (the one
 * to be replaced
 */
    first = subst + 1;

/* now find the second '^' */
    if (!(second = strchr(first, '^'))) {
        SEND_TO_Q("Invalid substitution.\r\n", t);
        return 1;
    }

/* terminate "first" at the position of the '^' and make 'second' point
 * to the beginning of the second string */
    *(second++) = '\0';

/* now, see if the contents of the first string appear in the original */
    if (!(strpos = strstr(orig, first))) {
        SEND_TO_Q("Invalid substitution.\r\n", t);
        return 1;
    }

/* now, we construct the new string for output. */

/* first, everything in the original, up to the string to be replaced */
    strncpy(new_str, orig, (strpos - orig));
    new_str[(strpos - orig)] = '\0';

/* now, the replacement string */
    strncat(new_str, second, (MAX_INPUT_LENGTH - strlen(new_str) - 1));

/* now, if there's anything left in the original after the string to
 * replaced, copy that too. */
    if (((strpos - orig) + strlen(first)) < strlen(orig))
        strncat(new_str, strpos + strlen(first),
            (MAX_INPUT_LENGTH - strlen(new_str) - 1));

/* terminate the string in case of an overflow from strncat */
    new_str[MAX_INPUT_LENGTH - 1] = '\0';
    strcpy(subst, new_str);

    return 0;
}

void
close_socket(struct descriptor_data *d)
{
    struct descriptor_data *temp;

    close(d->descriptor);
    flush_queues(d);

    // Forget those this descriptor is snooping
    if (d->snooping)
        d->snooping->snoop_by = g_list_remove(d->snooping->snoop_by, d);

    // Forget those snooping on this descriptor
    for (GList * x = d->snoop_by; x; x = x->next) {
        struct descriptor_data *td = x->data;
        SEND_TO_Q("Your victim is no longer among us.\r\n", td);
        td->snooping = NULL;
    }

    if (d->original) {
        d->creature->desc = NULL;
        d->creature = d->original;
        d->original = NULL;
        d->creature->desc = d;
    }
    // Cancel any text editing
    if (d->text_editor)
        editor_finish(d->text_editor, false);

    if (d->creature && d->creature->in_room) {
        // Lost link in-game
        d->creature->player.time.logon = time(0);
        crashsave(d->creature);
        act("$n has lost $s link.", true, d->creature, 0, 0, TO_ROOM);
        mlog(ROLE_ADMINBASIC,
            MAX(LVL_AMBASSADOR, GET_INVIS_LVL(d->creature)),
            NRM, false, "Closing link to: %s [%s] ", GET_NAME(d->creature),
            d->host);
        account_logout(d->account, d, true);
        d->creature->desc = NULL;
        GET_OLC_OBJ(d->creature) = NULL;
    } else if (d->account) {
        if (d->creature) {
            crashsave(d->creature);
            free_creature(d->creature);
            d->creature = NULL;
        }
        mlog(ROLE_ADMINBASIC, LVL_AMBASSADOR, NRM, false,
            "%s[%d] logging off from %s",
            d->account->name, d->account->id, d->host);
        account_logout(d->account, d, false);
    } else {
        slog("Losing descriptor without account");
    }

    REMOVE_FROM_LIST(d, descriptor_list, next);

    if (d->showstr_head)
        free(d->showstr_head);

    free(d);
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void
nonblock(int s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(s, F_SETFL, flags) < 0) {
        perror("Fatal error executing nonblock (comm.c)");
        safe_exit(EXIT_FAILURE);
    }
}

/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */

void
checkpointing(int sig __attribute__ ((unused)))
{
    if (!tics) {
        errlog("CHECKPOINT shutdown: tics not updated");
        slog("Last command: %s %s.", player_name_by_idnum(last_cmd[0].idnum),
            last_cmd[0].string);
        raise(SIGSEGV);
    } else
        tics = 0;
}

void
unrestrict_game(int sig __attribute__ ((unused)))
{
    extern struct ban_list_element *ban_list;
    extern int num_invalid;

    mudlog(LVL_AMBASSADOR, BRF, true,
        "Received SIGUSR2 - completely unrestricting game (emergent)");
    ban_list = NULL;
    restrict_logins = 0;
    num_invalid = 0;
}

void
hupsig(int sig __attribute__ ((unused)))
{
    slog("Received SIGHUP or SIGTERM.  Shutting down...");

    mudlog(LVL_AMBASSADOR, BRF, true,
        "Received external signal - shutting down for reboot in 60 sec..");

    send_to_all("\007\007:: Tempus REBOOT in 60 seconds ::\r\n");
    shutdown_idnum = -1;
    shutdown_count = 60;
    shutdown_mode = SHUTDOWN_REBOOT;

}

void
intsig(int sig __attribute__ ((unused)))
{
    mudlog(LVL_AMBASSADOR, BRF, true,
        "Received external signal - shutting down for reboot now.");

    send_to_all("\007\007:: Tempus REBOOT NOW! ::\r\n");
    circle_shutdown = circle_reboot = 1;
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

sigfunc *
my_signal(int signo, sigfunc * func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(signo, &act, &oact) < 0)
        return SIG_ERR;

    return oact.sa_handler;
}

void
signal_setup(void)
{
    struct itimerval itime;
    struct timeval interval;

    /* user signal 1: reread wizlists.  Used by autowiz system. */
    my_signal(SIGUSR1, qp_reload);

    /*
     * user signal 2: unrestrict game.  Used for emergencies if you lock
     * yourself out of the MUD somehow.  (Duh...)
     */
    my_signal(SIGUSR2, unrestrict_game);
    /*
     * set up the deadlock-protection so that the MUD aborts itself if it gets
     * caught in an infinite loop for more than 3 seconds
     */
    interval.tv_sec = 600;
    interval.tv_usec = 0;
    itime.it_interval = interval;
    itime.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &itime, NULL);
    my_signal(SIGVTALRM, checkpointing);

    /* just to be on the safe side: */
    my_signal(SIGHUP, hupsig);
    my_signal(SIGTERM, hupsig);
    my_signal(SIGINT, intsig);
    my_signal(SIGPIPE, SIG_IGN);
    my_signal(SIGALRM, SIG_IGN);

}

/* ****************************************************************
*       Public routines for system-to-player-communication        *
*******************************************************************/

void
send_to_char(struct creature *ch, const char *str, ...)
{
    char *msg_str;
    va_list args;

    if (!ch || !ch->desc || !str || !*str)
        return;

    va_start(args, str);
    msg_str = tmp_vsprintf(str, args);
    va_end(args);

    // Everything gets capitalized
    msg_str[0] = toupper(msg_str[0]);
    SEND_TO_Q(msg_str, ch->desc);
}

void
send_to_desc(struct descriptor_data *d, const char *str, ...)
{
    char *msg_str, *read_pt;
    va_list args;

    if (!d || !str || !*str)
        return;

    va_start(args, str);
    msg_str = tmp_vsprintf(str, args);
    va_end(args);

    // Everything gets capitalized
    msg_str[0] = toupper(msg_str[0]);

    // Now iterate through string, adding color coding
    read_pt = msg_str;
    while (*read_pt) {
        while (*read_pt && *read_pt != '&')
            read_pt++;
        if (*read_pt == '&') {
            *read_pt++ = '\0';
            SEND_TO_Q(msg_str, d);
            if (d->account && d->account->ansi_level > 0) {
                if (isupper(*read_pt)) {
                    *read_pt = tolower(*read_pt);
                    // A few extra normal tags never hurt anyone...
                    if (d->account->ansi_level > 2)
                        SEND_TO_Q(KBLD, d);
                } else if (d->account->ansi_level > 2)
                    SEND_TO_Q(KNRM, d);
                switch (*read_pt) {
                case '@':
                    SEND_TO_Q("\e[H\e[J", d);
                    break;
                case 'n':
                    SEND_TO_Q(KNRM, d);
                    break;
                case 'r':
                    SEND_TO_Q(KRED, d);
                    break;
                case 'g':
                    SEND_TO_Q(KGRN, d);
                    break;
                case 'y':
                    SEND_TO_Q(KYEL, d);
                    break;
                case 'm':
                    SEND_TO_Q(KMAG, d);
                    break;
                case 'c':
                    SEND_TO_Q(KCYN, d);
                    break;
                case 'b':
                    SEND_TO_Q(KBLU, d);
                    break;
                case 'w':
                    SEND_TO_Q(KWHT, d);
                    break;
                case '&':
                    SEND_TO_Q("&", d);
                    break;
                default:
                    SEND_TO_Q("&&", d);
                }
            }
            read_pt++;
            msg_str = read_pt;
        }
    }
    SEND_TO_Q(msg_str, d);
}

void
send_to_all(const char *messg)
{
    struct descriptor_data *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->input_mode)
                SEND_TO_Q(messg, i);
}

void
send_to_clerics(int align, const char *messg)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next) {
        if (!i->input_mode && i->creature && AWAKE(i->creature) &&
            !PLR_FLAGGED(i->creature, PLR_WRITING) &&
            PRIME_MATERIAL_ROOM(i->creature->in_room)
            && IS_CLERIC(i->creature)
            && !IS_NEUTRAL(i->creature)
            && (align != EVIL || IS_EVIL(i->creature))
            && (align != GOOD || IS_GOOD(i->creature))) {
            SEND_TO_Q(messg, i);
        }
    }
}

void
send_to_outdoor(const char *messg, int isecho)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next)
        if (!i->input_mode && i->creature && AWAKE(i->creature) &&
            (!isecho || !PRF2_FLAGGED(i->creature, PRF2_NOGECHO)) &&
            !PLR_FLAGGED(i->creature, PLR_WRITING) &&
            OUTSIDE(i->creature)
            && PRIME_MATERIAL_ROOM(i->creature->in_room))
            SEND_TO_Q(messg, i);
}

void
send_to_newbie_helpers(const char *messg)
{
    struct descriptor_data *i;
    extern int level_can_shout;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next)
        if (!i->input_mode && i->creature &&
            PRF2_FLAGGED(i->creature, PRF2_NEWBIE_HELPER) &&
            GET_LEVEL(i->creature) > level_can_shout &&
            !(PRF_FLAGGED(i->creature, PRF_LOG1) ||
                PRF_FLAGGED(i->creature, PRF_LOG2))) {
            SEND_TO_Q(CCBLD(i->creature, C_CMP), i);
            SEND_TO_Q(CCYEL(i->creature, C_SPR), i);
            SEND_TO_Q(messg, i);
            SEND_TO_Q(CCNRM(i->creature, C_SPR), i);
        }
}

/* mode == true -> hide from sender.  false -> show to all */
void
send_to_comm_channel(struct creature *ch, char *buf, int chan, int mode,
    int hide_invis)
{
    SPECIAL(master_communicator);
    struct creature *receiver;
    struct obj_data *obj = NULL;

    for (obj = object_list; obj; obj = obj->next) {
        if (obj->in_obj || !IS_COMMUNICATOR(obj))
            continue;

        if (!ENGINE_STATE(obj))
            continue;

        if (GET_OBJ_SPEC(obj) != master_communicator &&
            COMM_CHANNEL(obj) != chan)
            continue;

        if (obj->in_room) {
            act("$p makes some noises.", false, 0, obj, 0, TO_ROOM);
            continue;
        }

        receiver = obj->carried_by ? obj->carried_by : obj->worn_by;
        if (!receiver || !receiver->desc)
            continue;

        if (!IS_PLAYING(receiver->desc) || !receiver->desc ||
            PLR_FLAGGED(receiver, PLR_WRITING))
            continue;

        if (!COMM_UNIT_SEND_OK(ch, receiver))
            continue;

        if (mode && receiver == ch)
            continue;

        if (hide_invis && !can_see_creature(receiver, ch))
            continue;

        if (GET_OBJ_SPEC(obj) == master_communicator)
            send_to_char(receiver, "%s_%s [%d]::%s ", CCYEL(receiver, C_NRM),
                OBJS(obj, receiver), chan, CCNRM(receiver, C_NRM));
        else
            send_to_char(receiver, "%s_%s::%s ", CCYEL(receiver, C_NRM),
                OBJS(obj, receiver), CCNRM(receiver, C_NRM));
        act(buf, true, ch, obj, receiver, TO_VICT);
    }
}

void
send_to_zone(const char *messg, struct zone_data *zn, int outdoor)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next)
        if (!i->input_mode && i->creature && AWAKE(i->creature) &&
            !PLR_FLAGGED(i->creature, PLR_WRITING) &&
            i->creature->in_room->zone == zn &&
            (!outdoor ||
                (OUTSIDE(i->creature) &&
                    PRIME_MATERIAL_ROOM(i->creature->in_room))))
            SEND_TO_Q(messg, i);
}

void
send_to_room(const char *messg, struct room_data *room)
{
    struct creature *i;
    struct room_data *to_room;
    struct obj_data *o, *obj = NULL;
    char *str;
    int j;

    if (!room || !messg)
        return;

    for (GList * it = first_living(room->people); it; it = next_living(it)) {
        i = it->data;
        if (i->desc && !PLR_FLAGGED(i, PLR_WRITING))
            SEND_TO_Q(messg, i->desc);
    }
    /** check for vehicles in the room **/
    str = tmp_sprintf("(outside) %s", messg);
    for (o = room->contents; o; o = o->next_content) {
        if (IS_OBJ_TYPE(o, ITEM_VEHICLE)) {
            for (obj = object_list; obj; obj = obj->next) {
                if (((IS_OBJ_TYPE(obj, ITEM_V_DOOR) && !CAR_CLOSED(o)) ||
                        (IS_V_WINDOW(obj) && !CAR_CLOSED(obj))) &&
                    ROOM_NUMBER(obj) == ROOM_NUMBER(o) &&
                    GET_OBJ_VNUM(o) == V_CAR_VNUM(obj) && obj->in_room) {
                    for (GList * it = first_living(obj->in_room->people); it; it = next_living(it)) {
                        i = it->data;
                        if (i->desc && !PLR_FLAGGED(i, PLR_WRITING))
                            SEND_TO_Q(str, i->desc);
                    }
                }
            }
        }
    }

    /* see if there is a podium in the room */
    for (o = room->contents; o; o = o->next_content)
        if (IS_OBJ_TYPE(o, ITEM_PODIUM))
            break;

    if (o) {
        str = tmp_sprintf("(remote) %s", messg);
        for (j = 0; j < NUM_OF_DIRS; j++)
            if (ABS_EXIT(room, j) && ABS_EXIT(room, j)->to_room &&
                room != ABS_EXIT(room, j)->to_room &&
                !IS_SET(ABS_EXIT(room, j)->exit_info, EX_ISDOOR | EX_CLOSED)) {
                for (GList * it = first_living(ABS_EXIT(room, j)->to_room->people);
                    it; it = next_living(it)) {
                    i = it->data;
                    if (i->desc && !PLR_FLAGGED(i, PLR_OLC))
                        SEND_TO_Q(str, i->desc);
                }
            }
    }

    for (o = room->contents; o; o = o->next_content) {
        if (IS_OBJ_TYPE(o, ITEM_CAMERA) && o->in_room) {
            to_room = real_room(GET_OBJ_VAL(o, 0));
            if (to_room) {
                send_to_room(tmp_sprintf("(%s) %s", o->in_room->name, messg),
                    to_room);
            }
        }
    }
}

void
send_to_clan(const char *messg, int clan)
{
    struct descriptor_data *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->input_mode && i->creature
                && (GET_CLAN(i->creature) == clan)
                && !PLR_FLAGGED(i->creature, PLR_OLC)) {
                SEND_TO_Q(CCCYN(i->creature, C_NRM), i);
                SEND_TO_Q(messg, i);
                SEND_TO_Q(CCNRM(i->creature, C_NRM), i);
            }
}

const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
if ((pointer) == NULL) i = ACTNULL; else i = (expression);

/* higher-level communication: the act() function */
char *
act_escape(const char *str)
{
    str = tmp_gsub(str, "\\", "\\\\");
    str = tmp_gsub(str, "&", "\\&");
    str = tmp_gsub(str, "$", "\\$");
    return tmp_gsub(str, "]", "\\]");
}

char *
act_translate(struct creature *ch, struct creature *to, const char **s)
{
    bool Nasty_Words(const char *words);
    const char *random_curses(void);
    const char *c;
    char *i;

    (*s)++;
    c = *s;
    while (*c && *c != ']')
        if (*c == '\\' && *(c + 1) != '\0')
            c += 2;
        else
            c++;

    // Check for empty translation string
    if (c == *s) {
        errlog("act_translate called with empty string");
        return tmp_strdup("");
    }
    // Select substr
    i = tmp_substr(*s, 0, (c - *s) - 1);

    // Advance pointer
    *s = c;

    // Remove any act-escaping
    char *read_pt, *write_pt;
    read_pt = write_pt = i;
    while (*read_pt) {
        if (*read_pt == '\\')
            read_pt++;
        if (write_pt != read_pt)
            *write_pt = *read_pt;
        write_pt++;
        read_pt++;
    }
    *write_pt = '\0';

    if (!PRF_FLAGGED(to, PRF_NASTY) && ch != to && Nasty_Words(i))
        for (int idx = 0; idx < num_nasty; idx++)
            i = tmp_gsubi(i, nasty_list[idx], random_curses());

    if (ch != to)
        return translate_tongue(ch, to, i);

    return i;
}

void
make_act_str(const char *orig,
             char *buf,
             struct creature *ch,
             struct obj_data *obj,
             void *vict_obj,
             struct creature *to)
{
    const char *s = orig;
    const char *i = 0;
    char *first_printed_char = 0;

    while (true) {
        if (*s == '$') {
            switch (*(++s)) {
            case 'n':
                i = PERS(ch, to);
                break;
            case 'N':
                CHECK_NULL(vict_obj, PERS((struct creature *)vict_obj, to));
                break;
            case 't':
                i = (ch == to) ? "you" : PERS(ch, to);
                break;
            case 'T':
                if (vict_obj == NULL) {
                    i = ACTNULL;
                } else if (ch == vict_obj) {
                    if (vict_obj == to)
                        i = "yourself";
                    else if (IS_MALE((struct creature *)vict_obj))
                        i = "himself";
                    else if (IS_FEMALE((struct creature *)vict_obj))
                        i = "herself";
                    else
                        i = "itself";
                } else if (to == vict_obj) {
                    i = "you";
                } else {
                    i = PERS((struct creature *)vict_obj, to);
                }
                break;
            case 'm':
                i = HMHR(ch);
                break;
            case 'M':
                CHECK_NULL(vict_obj, HMHR((struct creature *)vict_obj));
                break;
            case 's':
                i = HSHR(ch);
                break;
            case 'S':
                CHECK_NULL(vict_obj, HSHR((struct creature *)vict_obj));
                break;
            case 'e':
                i = HSSH(ch);
                break;
            case 'E':
                CHECK_NULL(vict_obj, HSSH((struct creature *)vict_obj));
                break;
            case 'o':
                CHECK_NULL(obj, OBJN(obj, to));
                break;
            case 'O':
                CHECK_NULL(vict_obj, OBJN((struct obj_data *)vict_obj, to));
                break;
            case 'p':
                CHECK_NULL(obj, OBJS(obj, to));
                break;
            case 'P':
                CHECK_NULL(vict_obj, OBJS((struct obj_data *)vict_obj, to));
                break;
            case 'a':
                i = GET_MOOD(ch) ? GET_MOOD(ch) : "";
                break;
            case '{':
                if (GET_MOOD(ch)) {
                    i = GET_MOOD(ch);
                    while (*s && *s != '}')
                        s++;
                } else {
                    s++;
                    i = tmp_strdupt(s, "}");
                    s += strlen(i);
                }
                break;
            case 'A':
                CHECK_NULL(vict_obj, SANA((struct obj_data *)vict_obj));
                break;
            case '%':
                i = (ch == to) ? "" : "s";
                break;
            case '^':
                i = (ch == to) ? "" : "es";
                break;
            case 'l':
                i = make_tongue_str(ch, to);
                break;
            case '[':
                i = act_translate(ch, to, &s);
                break;
            default:
                errlog("Illegal $-code to act(): %s", s);
                i = "---";
                break;
            }
            if (!first_printed_char)
                first_printed_char = buf;
            while ((*buf = *(i++)))
                buf++;
            s++;
        } else if (*s == '&') {
            if (COLOR_LEV(to) > 0) {
                char c = *(++s);

                if (isupper(c)) {
                    c = tolower(c);
                    if (clr(to, C_NRM)) {
                        strcpy(buf, KBLD);
                        buf += strlen(KBLD);
                    }
                } else if (c != '&') {
                    strcpy(buf, KNRM);
                    buf += strlen(KNRM);
                }
                switch (c) {
                case 'n':
                    // Normal tag has already been copied
                    i = "";
                    break;
                case 'r':
                    i = KRED;
                    break;
                case 'g':
                    i = clr(to, C_NRM) ? KGRN : KNRM;
                    break;
                case 'y':
                    i = clr(to, C_NRM) ? KYEL : KNRM;
                    break;
                case 'm':
                    i = KMAG;
                    break;
                case 'c':
                    i = clr(to, C_NRM) ? KCYN : KNRM;
                    break;
                case 'b':
                    i = KBLU;
                    break;
                case 'w':
                    i = KWHT;
                    break;
                case '&':
                    i = "&";
                    break;
                default:
                    i = "&&";
                }
                while ((*buf = *(i++)))
                    buf++;
                s++;
            } else {
                s += 2;
            }
        } else {
            // backslashes are literal chars
            if (*s == '\\')
                s++;
            if (!first_printed_char)
                first_printed_char = buf;
            if (!(*(buf++) = *(s++)))
                break;
        }
    }
    if (first_printed_char)
        *first_printed_char = toupper(*first_printed_char);

    *(--buf) = '\r';
    *(++buf) = '\n';
    *(++buf) = '\0';
}

void
perform_act(const char *orig, struct creature *ch, struct obj_data *obj,
    void *vict_obj, struct creature *to, int mode)
{
    static char lbuf[MAX_STRING_LENGTH];
    char outbuf[MAX_STRING_LENGTH];

    if (!to || !to->desc || PLR_FLAGGED((to), PLR_WRITING))
        return;

    if (!to->in_room) {
        errlog("to->in_room NULL in perform_act.");
        return;
    }

    make_act_str(orig, lbuf, ch, obj, vict_obj, to);

    if (mode == 1) {
        sprintf(outbuf, "(outside) %s", lbuf);
    } else if (mode == 2) {
        sprintf(outbuf, "(remote) %s", lbuf);
    } else if (mode == 3) {
        struct room_data *toroom = NULL;
        if (ch != NULL && ch->in_room != NULL) {
            toroom = ch->in_room;
        } else if (obj != NULL && obj->in_room != NULL) {
            toroom = obj->in_room;
        }
        sprintf(outbuf, "(%s) %s", (toroom) ? toroom->name : "remote", lbuf);
    } else {
        strcpy(outbuf, lbuf);
    }

    SEND_TO_Q(outbuf, to->desc);
    if (COLOR_LEV(to) > 0)
        SEND_TO_Q(KNRM, to->desc);
}

#define SENDOK(ch) (AWAKE(ch) || sleep)

void
act_if(const char *str, int hide_invisible, struct creature *ch,
    struct obj_data *obj, void *vict_obj, int type, act_if_predicate pred)
{
    struct obj_data *o, *o2 = NULL;
    static int sleep;
    struct room_data *room = NULL;
    int j;
    bool hidecar = false;

    if (!str || !*str)
        return;

    /*
     * Warning: the following TO_SLEEP code is a hack.
     *
     * I wanted to be able to tell act to deliver a message regardless of sleep
     * without adding an additional argument.  TO_SLEEP is 128 (a single bit
     * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
     * command.  It's not legal to combine TO_x's with each other otherwise.
     */

    /*
     * Same warning goes for TO_VICT_RM.  Ugly, ugly hack.  I'm not prepared
     * to rewrite act to use a bitvector right now though.  TO_VICT_RM will
     * be ignored if not used with TO_ROOM or TO_NOTVICT.  At any rate, it
     * should cause the message to be sent to the victims room instead of
     * ch's room.
     */

    /* check if TO_SLEEP is there, and remove it if it is. */
    if ((sleep = (type & TO_SLEEP)))
        type &= ~TO_SLEEP;

    if (vict_obj && (type & TO_VICT_RM)) {
        room = ((struct creature *)vict_obj)->in_room;
        type &= ~TO_VICT_RM;
    }

    if (IS_SET(type, ACT_HIDECAR)) {
        hidecar = true;
        REMOVE_BIT(type, ACT_HIDECAR);
    }

    if (type == TO_CHAR) {
        if (ch && SENDOK(ch))
            perform_act(str, ch, obj, vict_obj, ch, 0);
        return;
    }
    if (type == TO_VICT) {
        if (vict_obj && SENDOK((struct creature *)vict_obj))
            perform_act(str, ch, obj, vict_obj, (struct creature *)vict_obj,
                0);
        return;
    }
    /* ASSUMPTION: at this point we know type must be TO_NOTVICT TO_ROOM,
       or TO_VICT_RM */

    if (!room && ch && ch->in_room != NULL) {
        room = ch->in_room;
    } else if (obj && obj->in_room != NULL) {
        room = obj->in_room;
    }

    if (!room) {
        errlog("no valid target to act()!");
        raise(SIGSEGV);
        return;
    }
    for (GList * it = first_living(room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (!pred(ch, obj, vict_obj, tch, 0))
            continue;
        if (SENDOK(tch) &&
            !(hide_invisible && ch && !can_see_creature(tch, ch)) &&
            (tch != ch) && (type == TO_ROOM || (tch != vict_obj)))
            perform_act(str, ch, obj, vict_obj, tch, 0);
    }

    /** check for vehicles in the room **/
    for (o = room->contents; o; o = o->next_content) {
        if (IS_OBJ_TYPE(o, ITEM_VEHICLE) && (!hidecar || o != cur_car)) {
            for (o2 = object_list; o2; o2 = o2->next) {
                if (((IS_OBJ_TYPE(o2, ITEM_V_DOOR) && !CAR_CLOSED(o)) ||
                        (IS_OBJ_TYPE(o2, ITEM_V_WINDOW) && !CAR_CLOSED(o2))) &&
                    ROOM_NUMBER(o2) == ROOM_NUMBER(o) &&
                    GET_OBJ_VNUM(o) == V_CAR_VNUM(o2) && o2->in_room) {
                    for (GList * it = first_living(o2->in_room->people); it; it = next_living(it)) {
                        struct creature *tch = it->data;
                        if (!pred(ch, obj, vict_obj, tch, 1))
                            continue;
                        if (SENDOK(tch) &&
                            !(hide_invisible && ch
                                && !can_see_creature(tch, ch)) && (tch != ch)
                            && (type == TO_ROOM || (tch != vict_obj))) {
                            perform_act(str, ch, obj, vict_obj, tch, 1);
                        }
                    }
                    break;
                }
            }
        }
    }

    /* see if there is a podium in the room */
    for (o = room->contents; o; o = o->next_content)
        if (IS_OBJ_TYPE(o, ITEM_PODIUM))
            break;

    if (o) {
        for (j = 0; j < NUM_OF_DIRS; j++) {
            if (ABS_EXIT(room, j) && ABS_EXIT(room, j)->to_room &&
                room != ABS_EXIT(room, j)->to_room &&
                !IS_SET(ABS_EXIT(room, j)->exit_info, EX_ISDOOR | EX_CLOSED)) {

                for (GList * it = first_living(ABS_EXIT(room, j)->to_room->people);
                    it; it = next_living(it)) {
                    struct creature *tch = it->data;

                    if (!pred(ch, obj, vict_obj, tch, 2))
                        continue;
                    if (SENDOK(tch) &&
                        !(hide_invisible && ch && !can_see_creature(tch, ch))
                        && (tch != ch) && (type == TO_ROOM
                            || (tch != vict_obj)))
                        perform_act(str, ch, obj, vict_obj, tch, 2);
                }
            }
        }
    }

    for (o = room->contents; o; o = o->next_content) {
        if (IS_OBJ_TYPE(o, ITEM_CAMERA) && o->in_room) {
            room = real_room(GET_OBJ_VAL(o, 0));
            if (room) {
                for (GList * it = room->people; it; it = next_living(it)) {
                    struct creature *tch = it->data;

                    if (!pred(ch, obj, vict_obj, tch, 3))
                        continue;
                    if (SENDOK(tch) &&
                        !(hide_invisible && ch && !can_see_creature(tch, ch))
                        && (tch != ch) && (tch != vict_obj))
                        perform_act(str, ch, obj, vict_obj, tch, 3);
                }
            }
        }
    }
}

bool
standard_act_predicate(struct creature *ch __attribute__ ((unused)),
    struct obj_data *obj __attribute__ ((unused)),
    void *vict_obj __attribute__ ((unused)),
    struct creature *to __attribute__ ((unused)),
    int mode __attribute__ ((unused)))
{
    return true;
}

void
act(const char *str, int hide_invisible, struct creature *ch,
    struct obj_data *obj, void *vict_obj, int type)
{
    act_if(str, hide_invisible, ch, obj, vict_obj, type,
        standard_act_predicate);
}

//
// ramdomly move the quad damage, creating it if needed
// bamf as in *>BAMF<* nightcrawler from X-Men
//
void
bamf_quad_damage(void)
{

    struct obj_data *quad = NULL;
    struct zone_data *zone = NULL;
    struct room_data *room = NULL, *orig_room = NULL;

    for (quad = object_list; quad; quad = quad->next)
        if (quad->in_room && (GET_OBJ_VNUM(quad) == QUAD_VNUM))
            break;

    if (quad) {
        if (quad->in_room->people)
            act("$p starts spinning faster and faster, and disappears in a flash!", false, 0, quad, 0, TO_ROOM);
        orig_room = quad->in_room;
        obj_from_room(quad);
    } else if (!(quad = read_object(QUAD_VNUM))) {
        errlog(" Unable to load Quad Damage.");
        return;
    }

    if (orig_room)
        zone = orig_room->zone;
    else
        zone = default_quad_zone;

    for (; zone; zone = zone->next) {
        if (zone->number >= 700)
            zone = default_quad_zone;
        if (zone->world && zone->plane <= MAX_PRIME_PLANE && !number(0, 2))
            break;
    }

    if (!zone) {
        errlog(" Quad didn't find valid zone to bamf to in bamf_quad_damage.");
        extract_obj(quad);
        return;
    }

    for (room = zone->world; room; room = room->next) {
        if (ROOM_FLAGGED(room, ROOM_PEACEFUL | ROOM_DEATH | ROOM_HOUSE |
                ROOM_CLAN_HOUSE) || room == orig_room || number(0, 9))
            continue;
        else
            break;
    }

    if (!room) {
        extract_obj(quad);
        return;
    }

    obj_to_room(quad, room);

    if (room->people)
        act("$p appears slowly spinning at the center of the room.",
            false, 0, quad, 0, TO_ROOM);

}

void
push_command_onto_list(struct creature *ch, char *string)
{

    int i;

    for (i = NUM_SAVE_CMDS - 2; i >= 0; i--) {
        last_cmd[i + 1].idnum = last_cmd[i].idnum;
        strcpy(last_cmd[i + 1].name, last_cmd[i].name);
        last_cmd[i + 1].roomnum = last_cmd[i].roomnum;
        strcpy(last_cmd[i + 1].room, last_cmd[i].room);
        strcpy(last_cmd[i + 1].string, last_cmd[i].string);
    }

    last_cmd[0].idnum = GET_IDNUM(ch);
    strncpy(last_cmd[0].name, GET_NAME(ch), MAX_INPUT_LENGTH);
    last_cmd[0].roomnum = (ch->in_room) ? ch->in_room->number : -1;
    strncpy(last_cmd[0].room,
        (ch->in_room && ch->in_room->name) ?
        ch->in_room->name : "<NULL>", MAX_INPUT_LENGTH);
    strncpy(last_cmd[0].string, string, MAX_INPUT_LENGTH);
}

void
descriptor_update(void)
{
    struct descriptor_data *d = NULL;

    for (d = descriptor_list; d; d = d->next) {

        // skip the folks that will get hit with point_update()
        if (d->creature && d->input_mode == CXN_PLAYING)
            continue;

        d->idle++;

        if (d->idle >= 10 && STATE(d) != CXN_PLAYING
            && STATE(d) != CXN_NETWORK) {
            mlog(ROLE_ADMINBASIC, LVL_IMMORT, CMP, true,
                "Descriptor idling out after 10 minutes");
            SEND_TO_Q("Idle time limit reached, disconnecting.\r\n", d);
            if (d->creature) {
                save_player_to_xml(d->creature);
                d->creature = NULL;
            }
            set_desc_state(CXN_DISCONNECT, d);
        }
    }
}

void
verify_environment(void)
{
    char path[256];
    int idx;

    mkdir("players", 0755);
    mkdir("players/character", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, 255, "players/character/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/equipment", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, 255, "players/equipment/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/housing", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, 255, "players/housing/%d", idx);
        mkdir(path, 0755);
    }
    mkdir("players/corpses", 0755);
    for (idx = 0; idx < 10; idx++) {
        snprintf(path, 255, "players/corpses/%d", idx);
        mkdir(path, 0755);
    }
}
