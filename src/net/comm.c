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
#endif

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <time.h>
#include <glib.h>

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
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "house.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "vehicle.h"
#include "obj_data.h"
#include "strutil.h"
#include "language.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"
#include "ban.h"

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
int buf_largecount = 0;         /* # of large buffers which exist */
int buf_overflows = 0;          /* # of overflows of output */
int buf_switches = 0;           /* # of switches from small to large buf */
int circle_shutdown = 0;        /* clean shutdown */
int circle_reboot = 0;          /* reboot the game after a shutdown */
int no_specials = 0;            /* Suppress ass. of special routines */
static int avail_descs = 0;            /* max descriptors available */
static int tics = 0;                   /* for extern checkpointing */
int log_cmds = 0;               /* log cmds */
bool stress_test = false;       /* stress test the codebase */
int shutdown_count = -1;        /* shutdown countdown */
int shutdown_idnum = -1;        /* idnum of person calling shutdown */
int shutdown_mode = SHUTDOWN_NONE;  /* what type of shutdown */
bool suppress_output = false;

extern int nameserver_is_slow;  /* see config.c */
extern int auto_save;           /* see config.c */
extern int autosave_time;       /* see config.c */
struct timeval null_time;       /* zero-valued time structure */
GMainLoop *main_loop = NULL;

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased, int length);
void flush_q(struct txt_q *queue);
void init_game(void);
void signal_setup(void);
void game_loop(int main_listener, int reader_listener);
int init_socket(int port);
int new_descriptor(int s, int port);
int get_avail_descs(void);
struct timespec timediff(struct timespec *a, struct timespec *b);
void flush_queues(struct descriptor_data *d);
void nonblock(int s);
char *perform_subst(struct descriptor_data *t, char *orig, const char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
void send_prompt(struct descriptor_data *point);
void bamf_quad_damage(void);
void descriptor_update(void);
gboolean process_input(GIOChannel *io,
                       GIOCondition condition,
                       gpointer data);
gboolean process_output(GIOChannel *io,
                        GIOCondition condition,
                        gpointer data);
gboolean handle_socket_hangup(GIOChannel *io,
                              GIOCondition condition,
                              gpointer data);
gboolean accept_new_connection(GIOChannel *io,
                               GIOCondition condition,
                               gpointer data);

/* extern fcnts */
void boot_world(void);
void affect_update(void);       /* In spells.c */
void obj_affect_update(void);
void mobile_activity(void);
void mobile_spec(void);
void burn_update(void);
void dynamic_object_pulse();
void flow_room(void);
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
void random_mob_activity(void);
gboolean update_room_affects(gpointer ignore);
gboolean update_alignment_ambience(gpointer ignore);

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

    extern PGconn *sql_cxn;

    my_srand(time(NULL));
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

    autosave_zones(ZONE_RESETSAVE);
    collect_housing_rent();
    save_all_players();
    save_houses();
    save_quests();
    xmlCleanupParser();
    PQfinish(sql_cxn);

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
    struct addrinfo *info, hints;
    int err, s, opt;
    char portstr[6];

    memset(&hints, 0, sizeof(hints));

#ifdef USE_IPV6
    hints.ai_family = AF_INET6;
#else
    hints.ai_family = AF_INET;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    sprintf(portstr, "%d", port);
    err = getaddrinfo(NULL, portstr, &hints, &info);
    if (err != 0) {
        fprintf(stderr, "init_socket(): %s\n", gai_strerror(err));
        safe_exit(EXIT_FAILURE);
    }

    struct addrinfo *cur = NULL;
    for (cur = info;cur;cur = cur->ai_next) {
        s = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (s == -1)
            continue;

#ifdef SO_SNDBUF
        opt = LARGE_BUFSIZE + GARBAGE_SPACE;
        err = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt));
        if (err < 0) {
            perror("setsockopt SNDBUF");
            safe_exit(EXIT_FAILURE);
        }
#endif

#ifdef SO_REUSEADDR
        opt = 1;
        err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
        if (err < 0) {
            perror("setsockopt REUSEADDR");
            safe_exit(EXIT_FAILURE);
        }
#endif

#ifdef SO_REUSEPORT
        opt = 1;
        err = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt));
        if (err < 0) {
            perror("setsockopt REUSEPORT");
            safe_exit(EXIT_FAILURE);
        }
#endif

#ifdef SO_LINGER
        {
            struct linger ld;

            ld.l_onoff = 0;
            ld.l_linger = 0;
            err = setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld));
            if (err < 0) {
                perror("setsockopt LINGER");
                safe_exit(EXIT_FAILURE);
            }
        }
#endif
        err = bind(s, cur->ai_addr, cur->ai_addrlen);
        if (err == 0)
            break;

        close(s);
    }

    freeaddrinfo(info);

    if (cur == NULL) {
        fprintf(stderr, "Could not bind.\n");
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

gboolean
mud_wide_tick(__attribute__ ((unused)) gpointer ignore)
{
    zone_update();
    weather_and_time();
    affect_update();
    obj_affect_update();
    point_update();
    descriptor_update();
    help_collection_sync(help);
    save_quests();
    return true;
}

gboolean
autosave(__attribute__ ((unused)) gpointer ignore)
{
    save_all_players();
    collect_housing_rent();
    save_houses();
    update_objects_housed_count();
    return true;
}

gboolean
update_ticks(__attribute__ ((unused)) gpointer ignore)
{
    tics++;
    return true;
}

gboolean
update_suppress_output(__attribute__ ((unused)) gpointer ignore)
{
    suppress_output = false;
    return true;
}

gboolean
temp_autosave_zones(__attribute__ ((unused)) gpointer ignore)
{
    autosave_zones(ZONE_AUTOSAVE);
    return true;
}

gboolean
reap_dead_creatures(__attribute__ ((unused)) gpointer ignore)
{
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
    return true;
}

gboolean
update_shutdown_timer(__attribute__ ((unused)) gpointer data)
{
    if (shutdown_count < 0)
        return false;

    shutdown_count--;

    if (shutdown_count == 10)
        send_to_all(":: Tempus REBOOT in 10 seconds ::\r\n");
    else if (shutdown_count == 30)
        send_to_all(":: Tempus REBOOT in 30 seconds ::\r\n");
    else if (shutdown_count && !(shutdown_count % 60)) {
        sprintf(buf, ":: Tempus REBOOT in %d minute%s ::\r\n",
                shutdown_count / 60, shutdown_count == 60 ? "" : "s");
        send_to_all(buf);
    } else if (shutdown_count <= 0) {
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
        g_main_loop_quit(main_loop);
    }
    return true;
}

gboolean
autoban_disconnect(struct descriptor_data *desc)
{
    close_socket(desc);
    return false;
}

gboolean
repeating_func_wrapper(gpointer func_ptr)
{
    void (*func)(void) = func_ptr;
    func();
    return true;
}

/**
 * g_io_channel_write_buffer_empty:
 * @channel: A #GIOChannel
 *
 * This function returns whether the write buffer of @channel has
 * pending data.
 *
 * Return value: %TRUE when @channel has no buffered data to write.
 *               Returns %FALSE when @channel has pending writes.
 **/
gboolean
g_io_channel_write_buffer_empty(GIOChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);
    g_return_val_if_fail (channel->use_buffer, FALSE);
    return !channel->write_buf || (channel->write_buf->len == 0);
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
    GIOChannel *main_io = g_io_channel_unix_new(main_listener);
    GIOChannel *reader_io = g_io_channel_unix_new(reader_listener);

    main_loop = g_main_loop_new(NULL, false);

    /* Setup network connections */
    g_io_add_watch(main_io, G_IO_IN, accept_new_connection,
                   GINT_TO_POINTER(main_port));
    g_io_add_watch(reader_io, G_IO_IN, accept_new_connection,
                   GINT_TO_POINTER(reader_port));

    /* Set up repeating events */
    g_timeout_add(100, repeating_func_wrapper, prog_update_pending);
    g_timeout_add(100, repeating_func_wrapper, update_unique_id);
    g_timeout_add(100, repeating_func_wrapper, update_ticks);
    g_timeout_add(100, repeating_func_wrapper, sql_gc_queries);
    g_timeout_add(100, repeating_func_wrapper, tmp_gc_strings);
    g_timeout_add(100, update_suppress_output, NULL);
    g_timeout_add(1000, reap_dead_creatures, NULL);
    g_timeout_add(100 * PULSE_MOBILE, repeating_func_wrapper, mobile_activity);
    g_timeout_add(100 * PULSE_MOBILE_SPEC, repeating_func_wrapper, mobile_spec);
    g_timeout_add(100 * SEG_VIOLENCE, repeating_func_wrapper, perform_violence);
    g_timeout_add(100 * FIRE_TICK, repeating_func_wrapper, burn_update);
    g_timeout_add(100 * PULSE_FLOWS, repeating_func_wrapper, flow_room);
    g_timeout_add(5000, update_room_affects, NULL);
    g_timeout_add(4000, update_alignment_ambience, NULL);
    g_timeout_add(100 * PULSE_FLOWS, repeating_func_wrapper, dynamic_object_pulse);
    g_timeout_add(100 * PULSE_FLOWS, repeating_func_wrapper, path_activity);
    g_timeout_add(100 * PULSE_FLOWS, repeating_func_wrapper, prog_update);
    g_timeout_add(100 * 130 * PASSES_PER_SEC, repeating_func_wrapper, retire_trails);
    g_timeout_add(100 * SECS_PER_MUD_HOUR * PASSES_PER_SEC, mud_wide_tick, NULL);
    g_timeout_add(100 * 300 * PASSES_PER_SEC, repeating_func_wrapper, record_usage);
    g_timeout_add(100 * 900 * PASSES_PER_SEC, temp_autosave_zones, NULL);
    g_timeout_add(100 * 666 * PASSES_PER_SEC, repeating_func_wrapper, bamf_quad_damage);
    if (auto_save)
        g_timeout_add(100 * 60 * PASSES_PER_SEC, autosave, NULL);
    if (stress_test) {
        g_timeout_add(100, repeating_func_wrapper, random_mob_activity);
    }

    /* Start the game */
    g_main_loop_run(main_loop);
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
struct timespec
timediff(struct timespec *a, struct timespec *b)
{
    struct timespec rslt = { .tv_sec = 0, .tv_nsec = 0 };

    if (a->tv_sec < b->tv_sec)
        return rslt;
    else if (a->tv_sec == b->tv_sec) {
        if (a->tv_nsec < b->tv_nsec)
            return rslt;
        else {
            rslt.tv_sec = 0;
            rslt.tv_nsec = a->tv_nsec - b->tv_nsec;
            return rslt;
        }
    } else {                    /* a->tv_sec > b->tv_sec */
        rslt.tv_sec = a->tv_sec - b->tv_sec;
        if (a->tv_nsec < b->tv_nsec) {
            rslt.tv_nsec = a->tv_nsec + 1000000000 - b->tv_nsec;
            rslt.tv_sec--;
        } else
            rslt.tv_nsec = a->tv_nsec - b->tv_nsec;
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
d_send(struct descriptor_data *d, const char *txt)
{
    GError *error = NULL;
    gsize bytes_written;

    if (suppress_output)
        return;

    if (!IS_SET(g_io_channel_get_flags(d->io), G_IO_FLAG_IS_WRITEABLE)) {
        return;
    }

    if (!d->need_prompt
        && (!d->creature || PRF2_FLAGGED(d->creature, PRF2_AUTOPROMPT))) {
        d->need_prompt = true;
        // New output crlf
        if (!d->account
            || d->account->compact_level == 1
            || d->account->compact_level == 3)
            d_send(d, "\r\n");
    }

    /* handle snooping: prepend "% " and send to snooper */
    if (d->snoop_by && d->creature && !IS_NPC(d->creature)) {
        for (GList * x = d->snoop_by; x; x = x->next) {
            struct descriptor_data *td = x->data;
            d_printf(td, "&r{ &n%s&r } &n", txt);
        }
    }
    error = NULL;
    g_io_channel_write_chars(d->io, txt, -1, &bytes_written, &error);
    if (error) {
        slog("g_io_channel_write_chars: %s", error->message);
        g_error_free(error);
     }
    if (!d->out_watcher)
        d->out_watcher = g_io_add_watch(d->io, G_IO_OUT, process_output, d);
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */

void destroy_socket(struct descriptor_data *d);

gboolean
handle_socket_error(__attribute__ ((unused)) GIOChannel *io,
                    GIOCondition condition,
                    gpointer data)
{
    struct descriptor_data *d = data;

    if (condition == G_IO_ERR)
        slog("Received socket error event");
    else
        slog("Received socket invalid event");
    close_socket(d);
    return false;
}

gboolean
accept_new_connection(GIOChannel *listener_io,
                      __attribute__ ((unused)) GIOCondition condition,
                      gpointer data)
{
    int desc, sockets_input_mode = 0;
    static int last_desc = 0;   /* last descriptor number */
    struct descriptor_data *newd;
    struct sockaddr_storage peer;
    socklen_t addrlen;
    extern const char *GREETINGS[];
    int s = g_io_channel_unix_get_fd(listener_io);
    int port = GPOINTER_TO_INT(data);
    GIOChannel *io;

    /* accept the new connection */
    addrlen = sizeof(peer);
    if ((desc = accept(s, (struct sockaddr *)&peer, &addrlen)) < 0) {
        perror("accept");
        return true;
    }

    /* make sure we have room for it */
    for (newd = descriptor_list; newd; newd = newd->next)
        sockets_input_mode++;

    io = g_io_channel_unix_new(desc);

    if (sockets_input_mode >= avail_descs) {
        g_io_channel_write_chars(io, "Sorry, Tempus is full right now... try again later!  :-)\r\n", -1, NULL, NULL);
        g_io_channel_shutdown(io, true, NULL);
        g_io_channel_unref(io);
        return true;
    }

    /* create a new descriptor */
    CREATE(newd, struct descriptor_data, 1);

    /* find the site name */
    int info_flags = NI_NUMERICSERV;
    int err;
    if (nameserver_is_slow)
        info_flags |= NI_NUMERICHOST;
    err = getnameinfo((struct sockaddr *)&peer, addrlen,
                      newd->host, HOST_LENGTH,
                      NULL, 0, info_flags);
    if (err != 0) {
        errlog("new_descriptor(): %s\n", gai_strerror(err));
        g_io_channel_shutdown(io, true, NULL);
        g_io_channel_unref(io);
        free(newd);
        return true;
    }

    /* determine if the site is banned */
    if (check_ban_all(io, newd->host)) {
        g_io_channel_shutdown(io, true, NULL);
        g_io_channel_unref(io);
        free(newd);
        return true;
    }

    int bantype = isbanned(newd->host, buf2, sizeof(buf2));

    /* Log new connections - probably unnecessary, but you may want it */
    mlog(ROLE_ADMINBASIC, LVL_GOD, CMP, true,
         "New connection from [%s]%s%s on %d",
         newd->host,
         (bantype == BAN_SELECT) ? "(SELECT BAN)" : "",
         (bantype == BAN_NEW) ? "(NEWBIE BAN)" : "",
         port);

    /* Set up descriptor I/O channels */
    GError *error = NULL;

    newd->io = g_io_channel_unix_new(desc);

    g_io_channel_set_flags(newd->io, G_IO_FLAG_NONBLOCK, &error);
    if (error) {
        slog("g_io_channel_set_flags(): %s", error->message);
        g_error_free(error);
    }
    g_io_channel_set_encoding(newd->io, NULL, NULL);
    g_io_channel_set_line_term(newd->io, NULL, 0);
    g_io_channel_set_buffer_size(newd->io, SMALL_BUFSIZE);
    newd->in_watcher = g_io_add_watch(newd->io, G_IO_IN | G_IO_HUP, process_input, newd);
    newd->err_watcher = g_io_add_watch(newd->io, G_IO_ERR | G_IO_NVAL, handle_socket_error, newd);
    newd->input_handler = g_timeout_add(100, handle_input, newd);

    newd->input = g_queue_new();

    /* initialize descriptor data */
    STATE(newd) = CXN_ACCOUNT_LOGIN;
    newd->wait = 1;
    newd->next = descriptor_list;
    newd->login_time = time(NULL);
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
        d_send(newd, "(testmud)");
    } else if (newd->is_blind) {
        d_send(newd, "Welcome to Tempus MUD!\r\n");
    } else {
        // This text is printed just before the screen clear, so most
        // people won't even see it.  Screen readers will read it out
        // loud, though.
        d_printf(newd,"If you use a screen reader, you'll want to use port %d&@", reader_port);
        if (number(0, 99)) {
            if (number(0, 10))
                d_send(newd, GREETINGS[1]);
            else
                d_send(newd, GREETINGS[2]);
        } else {
            // send original greeting 1/100th of the time
            d_send(newd, GREETINGS[0]);
        }
    }
    return true;
}

gboolean
process_output(__attribute__ ((unused)) GIOChannel *io,
               __attribute__ ((unused)) GIOCondition condition,
               gpointer data)
{
    struct descriptor_data *d = data;
    GError *error = NULL;

    if (!IS_SET(g_io_channel_get_flags(d->io), G_IO_FLAG_IS_WRITEABLE)) {
        d->out_watcher = 0;
        return false;
    }

    // New output crlf
    if (d->creature
        && !g_io_channel_write_buffer_empty(d->io)
        && d->account->compact_level < 2) {
        d_send(d, "\r\n");
    }
    if (d->need_prompt) {
        send_prompt(d);
        // After prompt crlf
        if (d->creature
            && !g_io_channel_write_buffer_empty(d->io)
            && (d->account->compact_level == 0
                || d->account->compact_level == 2))
            d_send(d, "\r\n");
        d->need_prompt = false;
    }

    g_io_channel_flush(d->io, &error);
    if (error) {
        slog("g_io_channel_flush: %s", error->message);
        g_error_free(error);
    }

    if (g_io_channel_write_buffer_empty(d->io)) {
        d->out_watcher = 0;
        return false;
    }

    return true;
}

void
enqueue_line_input(struct descriptor_data *d, const char *line)
{
    d->need_prompt = true;
    
    if (d->snoop_by && d->creature && !IS_NPC(d->creature)) {
        for (GList * x = d->snoop_by; x; x = x->next) {
            struct descriptor_data *td = x->data;
            d_printf(td, "&r[ &n%s&r ]\r\n&n", line);
        }
    }

    if (*line == '!' && IS_PLAYING(d)) {
        line = d->last_input;
    } else if (*line == '^') {
        char *sub = perform_subst(d, d->last_input, line);
        if (sub == NULL) {
            return;
        }
        strcpy_s(d->last_input, sizeof(d->last_input), sub);
        line = d->last_input;
    } else {
        strcpy_s(d->last_input, sizeof(d->last_input), line);
    }

    if (d->repeat_cmd_count > 300 &&
        (!d->creature || GET_LEVEL(d->creature) < LVL_ETERNAL)) {
        if (d->creature && d->creature->in_room) {
            act("SAY NO TO SPAM.\r\n"
                "Begone oh you waster of electrons,"
                " ye vile profaner of CPU time!", true, d->creature, NULL, NULL,
                TO_ROOM);
            slog("SPAM-death on the queue!");
            close_socket(d);
            return;
        }
    }

    if (IS_PLAYING(d) && !strncmp("revo", line, 4)) {
        // We want all commands in the queue to be dumped immediately
        // This has to be here so we can bypass the normal order of
        // commands
        if (g_queue_is_empty(d->input)) {
            d_printf(d,
                     "You don't have any commands to revoke!\r\n");
        } else {
            g_queue_foreach(d->input, (GFunc)g_free, NULL);
            g_queue_clear(d->input);
            d_printf(d, "You reconsider your rash plans.\r\n");
            WAIT_STATE(d->creature, 1 RL_SEC);
        }
    } else {
        g_queue_push_tail(d->input, strdup(line));
    }
}

gboolean
process_input(__attribute__ ((unused)) GIOChannel *io,
              GIOCondition condition,
              gpointer data)
{
    struct descriptor_data *d = data;
    GError *error = NULL;
    gsize bytes_read;
    GIOStatus status;

    if ((condition & G_IO_HUP) != 0) {
        close_socket(d);
        return false;
    }

    status = g_io_channel_read_chars(d->io,
                                     &d->inbuf[d->inbuf_len],
                                     MAX_RAW_INPUT_LENGTH - d->inbuf_len,
                                     &bytes_read,
                                     &error);
    if (status == G_IO_STATUS_EOF) {
        close_socket(d);
        return false;
    } else if (error) {
         slog("g_io_channel_read_line(): %s", error->message);
         g_error_free(error);
        close_socket(d);
        return false;
    }

    d->inbuf_len += bytes_read;
    char *end_pt = d->inbuf + d->inbuf_len;
    char *last_line_start = d->inbuf;
    GString *line = g_string_sized_new(80);
    bool consume_linebreak = false;
    int ignore_count = 0;
    for (char *read_pt = d->inbuf;read_pt != end_pt;read_pt++) {
        if (ignore_count > 0) {
            ignore_count--;
            continue;
        }
        if (consume_linebreak) {
            consume_linebreak = false;
            if (*read_pt == '\n') {
                last_line_start = read_pt + 1;
            }
            continue;
        }
        switch (*read_pt) {
        case '\xff':
            // telnet handling - telnet codes are always three bytes
            ignore_count = 2;
            break;
        case '\b':
            // backspacing
            g_string_truncate(line, line->len - 1);
            break;
        case '\r':
            // CRLF handling
            consume_linebreak = true;
            enqueue_line_input(d, line->str);
            g_string_truncate(line, 0);
            last_line_start = read_pt + 1;
            break;
        case '\n':
            // bare linefeed handling
            enqueue_line_input(d, line->str);
            g_string_truncate(line, 0);
            last_line_start = read_pt + 1;
            break;
        default:
            g_string_append_c(line, *read_pt);
        }
    }
    g_string_free(line, true);

    if (d->inbuf_len >= MAX_RAW_INPUT_LENGTH) {
        // Guard against the line buffer overflowing.
        d_printf(d, "WARNING: line too long.  Ignoring.\r\n");
        d->inbuf_len = 0;
    } else {
        // Copy last line fragment to start of input buffer
        d->inbuf_len = end_pt - last_line_start;
        memmove(d->inbuf, last_line_start, d->inbuf_len);
    }

    return true;
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
char *
perform_subst(struct descriptor_data *t, char *orig, const char *subst)
{
    char *needle = tmp_strdupt(subst + 1, "^");

    if (strstr(orig, needle) == NULL) {
        return NULL;
    }

    return tmp_gsub(orig, needle, subst + strlen(needle) + 2);
}

void
destroy_socket(struct descriptor_data *d)
{
    struct descriptor_data *temp;

    // Forget those this descriptor is snooping
    if (d->snooping)
        d->snooping->snoop_by = g_list_remove(d->snooping->snoop_by, d);

    // Forget those snooping on this descriptor
    for (GList *x = d->snoop_by; x; x = x->next) {
        struct descriptor_data *td = x->data;
        d_send(td, "Your victim is no longer among us.\r\n");
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
        d->creature->player.time.logon = time(NULL);
        crashsave(d->creature);
        account_logout(d->account, d, true);
        d->creature->desc = NULL;
        GET_OLC_OBJ(d->creature) = NULL;
        mlog(ROLE_ADMINBASIC,
            MAX(LVL_AMBASSADOR, GET_INVIS_LVL(d->creature)),
            NRM, false, "Closing link to: %s [%s] ", GET_NAME(d->creature),
            d->host);
        act("$n has lost $s link.", true, d->creature, NULL, NULL, TO_ROOM);
    } else if (d->account) {
        if (d->creature) {
            save_player_to_xml(d->creature);
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

    g_source_remove(d->in_watcher);
    g_source_remove(d->err_watcher);
    if (d->out_watcher)
        g_source_remove(d->out_watcher);
    g_source_remove(d->input_handler);

    g_io_channel_unref(d->io);

    
    free(d);
}

void
close_socket(struct descriptor_data *d)
{
    GError *error = NULL;

    set_desc_state(CXN_DISCONNECT, d);
    g_io_channel_shutdown(d->io, (g_io_channel_get_flags(d->io) & G_IO_FLAG_IS_WRITEABLE) != 0, &error);
    destroy_socket(d);
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
    g_timeout_add(1000, update_shutdown_timer, NULL);
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
    d_send(ch->desc, msg_str);
}

void
d_printf(struct descriptor_data *d, const char *str, ...)
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
            d_send(d, msg_str);
            if (d->account && d->account->ansi_level > 0) {
                if (isupper(*read_pt)) {
                    *read_pt = tolower(*read_pt);
                    // A few extra normal tags never hurt anyone...
                    if (d->account->ansi_level > 2)
                        d_send(d, KBLD);
                } else if (d->account->ansi_level > 2)
                    d_send(d, KNRM);
                switch (*read_pt) {
                case '@':
                    d_send(d, "\e[H\e[J");
                    break;
                case 'n':
                    d_send(d, KNRM);
                    break;
                case 'r':
                    d_send(d, KRED);
                    break;
                case 'g':
                    d_send(d, KGRN);
                    break;
                case 'y':
                    d_send(d, KYEL);
                    break;
                case 'm':
                    d_send(d, KMAG);
                    break;
                case 'c':
                    d_send(d, KCYN);
                    break;
                case 'b':
                    d_send(d, KBLU);
                    break;
                case 'w':
                    d_send(d, KWHT);
                    break;
                case '&':
                    d_send(d, "&");
                    break;
                default:
                    d_send(d, "&&");
                }
            }
            read_pt++;
            msg_str = read_pt;
        }
    }
    d_send(d, msg_str);
}

void
send_to_all(const char *messg)
{
    struct descriptor_data *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->input_mode)
                d_send(i, messg);
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
            d_send(i, messg);
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
            d_send(i, messg);
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
            d_printf(i, "&Y%s&n", messg);
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
            act("$p makes some noises.", false, NULL, obj, NULL, TO_ROOM);
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
            d_send(i, messg);
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
            d_send(i->desc, messg);
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
                            d_send(i->desc, str);
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
                        d_send(i->desc, str);
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
                d_printf(i, "&c%s&n", messg);
            }
}

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
    const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression)                     \
    if ((pointer) == NULL) i = ACTNULL; else i = (expression);

    const char *s = orig;
    const char *i = NULL;
    char *first_printed_char = NULL;

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
        snprintf(outbuf, sizeof(outbuf), "(outside) %s", lbuf);
    } else if (mode == 2) {
        snprintf(outbuf, sizeof(outbuf), "(remote) %s", lbuf);
    } else if (mode == 3) {
        struct room_data *toroom = NULL;
        if (ch != NULL && ch->in_room != NULL) {
            toroom = ch->in_room;
        } else if (obj != NULL && obj->in_room != NULL) {
            toroom = obj->in_room;
        }
        snprintf(outbuf, sizeof(outbuf), "(%s) %s", (toroom) ? toroom->name : "remote", lbuf);
    } else {
        strcpy_s(outbuf, sizeof(outbuf), lbuf);
    }

    d_send(to->desc, outbuf);
    if (COLOR_LEV(to) > 0)
        d_send(to->desc, KNRM);
}

#define SENDOK(ch) (AWAKE(ch) || sleep)

void
act_if(const char *str, bool hide_invisible, struct creature *ch,
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
act(const char *str, bool hide_invisible, struct creature *ch,
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
            act("$p starts spinning faster and faster, and disappears in a flash!", false, NULL, quad, NULL, TO_ROOM);
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
            false, NULL, quad, NULL, TO_ROOM);

}

void
descriptor_update(void)
{
    struct descriptor_data *d = NULL, *next_d = NULL;

    for (d = descriptor_list; d; d = next_d) {
        next_d = d->next;

        // skip the folks that will get hit with point_update()
        if (d->creature && d->input_mode == CXN_PLAYING)
            continue;

        d->idle++;

        if (d->idle >= 10 && STATE(d) != CXN_PLAYING
            && STATE(d) != CXN_NETWORK) {
            mlog(ROLE_ADMINBASIC, LVL_IMMORT, CMP, true,
                "Descriptor idling out after 10 minutes");
            d_send(d, "Idle time limit reached, disconnecting.\r\n");
            if (d->creature) {
                save_player_to_xml(d->creature);
                d->creature = NULL;
            }
            close_socket(d);
        }
    }
}
