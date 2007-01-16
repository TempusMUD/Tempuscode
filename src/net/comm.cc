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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>

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
#include "player_table.h"
#include "prog.h"
#include "quest.h"

/* externs */
extern HelpCollection *Help;
extern int restrict;
extern int mini_mud;
extern int mud_moved;
extern int olc_lock;
extern int no_rent_check;
extern int no_initial_zreset;
extern int log_cmds;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern unsigned int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;
extern struct obj_data *cur_car;
extern struct zone_data *default_quad_zone;
extern char help[];
extern struct obj_data *object_list;
bool production_mode = false;	// Run in production mode

/* local globals */
struct descriptor_data *descriptor_list = NULL;	/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;			/* # of large buffers which exist */
int buf_overflows = 0;			/* # of overflows of output */
int buf_switches = 0;			/* # of switches from small to large buf */
int circle_shutdown = 0;		/* clean shutdown */
int circle_reboot = 0;			/* reboot the game after a shutdown */
int no_specials = 0;			/* Suppress ass. of special routines */
int avail_descs = 0;			/* max descriptors available */
int tics = 0;					/* for extern checkpointing */
int scheck = 0;					/* for syntax checking mode */
int log_cmds = 0;				/* log cmds */
int shutdown_count = -1;		/* shutdown countdown */
int shutdown_idnum = -1;		/* idnum of person calling shutdown */
int shutdown_mode = SHUTDOWN_NONE;	/* what type of shutdown */
bool suppress_output = false;


struct last_command_data last_cmd[NUM_SAVE_CMDS];

extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;			/* see config.c */
extern int autosave_time;		/* see config.c */
struct timeval null_time;		/* zero-valued time structure */

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased, int length = MAX_INPUT_LENGTH );
void flush_q(struct txt_q *queue);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
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
int write_to_descriptor(int desc, char *txt);

/* extern fcnts */
void boot_world(void);
void affect_update(void);		/* In spells.c */
void obj_affect_update(void);
void mobile_activity(void);
void mobile_spec(void);
void burn_update(void);
void dynamic_object_pulse();
void flow_room(int pulse);
void path_activity();
void editor(struct descriptor_data *d, char *buffer);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname, char *blocking_hostname);
void verify_environment(void);
void weather_and_time(int mode);
void autosave_zones(int SAVE_TYPE);
void mem_cleanup(void);
void retire_trails(void);
void set_desc_state(int state, struct descriptor_data *d);
void save_quests(); // quests.cc - saves quest data
void save_all_players();

/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

int
main(int argc, char **argv)
{
	int port;
	int pos = 1;
	char *dir;

	port = DFLT_PORT;
	dir = DFLT_DIR;
	
	tmp_string_init();
	acc_string_init();

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
		case 'b':
			restrict = 50;
			slog("Wizlock 50");
			break;
		case 'd':
			if (*(argv[pos] + 2))
				dir = argv[pos] + 2;
			else if (++pos < argc)
				dir = argv[pos];
			else {
				slog("Directory arg expected after option -d.");
				safe_exit(1);
			}
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
			restrict = 1;
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
			log_cmds = TRUE;
			slog("Enabling log_cmds.");
			break;
		case 'p':
			production_mode = true;
			slog("Running in production mode");
			break;
		default:
			errlog("Unknown option -%c in argument string.",
				*(argv[pos] + 1));
			break;
		}
		pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			fprintf(stderr,
				"Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n",
				argv[0]);
			safe_exit(1);
		} else if ((port = atoi(argv[pos])) <= 1024) {
			fprintf(stderr, "Illegal port number.\n");
			safe_exit(1);
		}
	}

	if (chdir(dir) < 0) {
		perror("Fatal error changing to data directory");
		safe_exit(1);
	}

	slog("Using %s as data directory.", dir);
	verify_environment();

	if (scheck) {
		boot_world();
		slog("Done.");
		safe_exit(0);
	} else {
		slog("Running game on port %d.", port);
		init_game(port);
	}

	return 0;
}



/* Init sockets, run game, and cleanup sockets */
void
init_game(int port)
{
	int mother_desc;
	void my_srand(unsigned long initial_seed);
    void verify_tempus_integrity(Creature *ch);

	my_srand(time(0));
	boot_db();

    slog("Testing internal integrity");
    const char *result = tmp_string_test();
    if (result) {
        fprintf(stderr, "%s\n", result);
        return;
    }
    verify_tempus_integrity(NULL);

	slog("Opening mother connection.");
	mother_desc = init_socket(port);

	avail_descs = get_avail_descs();


	slog("Signal trapping.");
	signal_setup();

	slog("Entering game loop.");

	game_loop(mother_desc);

	slog("Closing all sockets.");
	while (descriptor_list)
		close_socket(descriptor_list);

	close(mother_desc);
	Security::shutdown();

	if (circle_reboot) {
		slog("Rebooting.");
		exit(52);				/* what's so great about HHGTTG, anyhow? */
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
		safe_exit(1);
	}
#if defined(SO_SNDBUF)
	opt = LARGE_BUFSIZE + GARBAGE_SPACE;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt SNDBUF");
		safe_exit(1);
	}
#endif

#if defined(SO_REUSEADDR)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt REUSEADDR");
		safe_exit(1);
	}
#endif

#if defined(SO_REUSEPORT)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt REUSEPORT");
		safe_exit(1);
	}
#endif

#if defined(SO_LINGER)
	{
		struct linger ld;

		ld.l_onoff = 0;
		ld.l_linger = 0;
		if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0) {
			perror("setsockopt LINGER");
			safe_exit(1);
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
		safe_exit(1);
	}
	nonblock(s);
	listen(s, 5);
	return s;
}


int
get_avail_descs(void)
{
	unsigned int max_descs = 0;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.
 */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
	{
		struct rlimit limit;

		getrlimit(RLIMIT_NOFILE, &limit);
		max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
		limit.rlim_cur = max_descs;
		setrlimit(RLIMIT_NOFILE, &limit);
	}
#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
	max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
								   * OPEN_MAX */
#else
	/*
	 * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
	 * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
	 * in the UNIX Environment_).
	 */
	errno = 0;
	if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
		if (errno == 0)
			max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
		else {
			perror("Error calling sysconf");
			safe_exit(1);
		}
	}
#endif

	max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

	if (max_descs <= 0) {
		slog("Non-positive max player limit!");
		safe_exit(1);
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
game_loop(int mother_desc)
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

		/* Sleep if we don't have any connections */
		if (descriptor_list == NULL) {
			slog("No connections.  Going to sleep.");
			FD_ZERO(&input_set);
			FD_SET(mother_desc, &input_set);
			if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0,
					NULL) < 0) {
				if (errno == EINTR)
					slog("Waking up to process signal.");
				else
					perror("Select coma");
			} else
				slog("New connection.  Waking up.");
			gettimeofday(&last_time, (struct timezone *)0);
		}
		/* Set up the input, output, and exception sets for select(). */
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(mother_desc, &input_set);
		maxdesc = mother_desc;
		for (d = descriptor_list; d; d = d->next) {
			if (d->descriptor > maxdesc)
				maxdesc = d->descriptor;
			FD_SET(d->descriptor, &input_set);
			FD_SET(d->descriptor, &output_set);
			FD_SET(d->descriptor, &exc_set);
		}

		/*
		 * At this point, the original Diku code set up a signal mask to avoid
		 * block all signals from being delivered.  I believe this was done in
		 * order to prevent the MUD from dying with an "interrupted system call"
		 * error in the event that a signal be received while the MUD is dormant.
		 * However, I think it is easier to check for an EINTR error return from
		 * this select() call rather than to block and unblock signals.
		 */
		do {
			errno = 0;			/* clear error condition */

			/* figure out for how long we have to sleep */
			gettimeofday(&now, (struct timezone *)0);
			timespent = timediff(&now, &last_time);
			timeout = timediff(&opt_time, &timespent);

			/* sleep until the next 0.1 second mark */
			if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0,
					&timeout) < 0)
				if (errno != EINTR) {
					perror("Select sleep");
					safe_exit(1);
				}
		} while (errno);

		/* record the time for the next pass */
		gettimeofday(&last_time, (struct timezone *)0);

		/* poll (without blocking) for new input, output, and exceptions */
		if (select(maxdesc + 1, &input_set, &output_set, &exc_set,
				&null_time) < 0) {
			perror("Select poll");
			return;
		}
		/* New connection waiting for us? */
		if (FD_ISSET(mother_desc, &input_set))
			new_descriptor(mother_desc);

		/* kick out the freaky folks in the exception set */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (FD_ISSET(d->descriptor, &exc_set)) {
				FD_CLR(d->descriptor, &input_set);
				FD_CLR(d->descriptor, &output_set);
				close_socket(d);
			}
		}

		/* process descriptors with input pending */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (FD_ISSET(d->descriptor, &input_set))
				if (process_input(d) < 0)
					close_socket(d);
		}

		/* process commands we just read from process_input */
		for (d = descriptor_list; d; d = next_d) {

			next_d = d->next;

			if (--(d->wait) > 0)
				continue;
			handle_input(d);
		}

		/* save all players that need to be immediately */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (d->creature &&
					IS_PLAYING(d) &&
					IS_PC(d->creature) &&
					PLR_FLAGGED(d->creature, PLR_CRASH))
				d->creature->crashSave();
		}

        /* Update progs triggered by user input that have not run yet */
        prog_update_pending();

		/* handle heartbeat stuff */
		/* Note: pulse now changes every 0.10 seconds  */

		pulse++;

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
			weather_and_time(1);
			affect_update();
            obj_affect_update();
			point_update();
			descriptor_update();
			Help->Sync();
			save_quests();
		}
		if (auto_save) {
			if (!(pulse % (60 * PASSES_PER_SEC))) {	/* 1 minute */
				if (++mins_since_crashsave >= autosave_time) {
					mins_since_crashsave = 0;
					save_all_players();
				}
				Housing.collectRent();
				Housing.save();
				Housing.countObjects();
			}
		}

		if (!(pulse % (300 * PASSES_PER_SEC))) {	/* 5 minutes */
			record_usage();
        }

		if (!(pulse % (130 * PASSES_PER_SEC))) {	// 2.1 minutes
			retire_trails();
            // This is just some debug code. Hopefully it
            // will confirm a suspicion that I have that one more
            // more rooms are getting disconnected from their
            // zones at some point.
            for (zone_data *zone = zone_table; zone; zone = zone->next) {
                for (room_data *room = zone->world; room; room = room->next) {
                    if (!room->zone) {
	                    mudlog(LVL_AMBASSADOR, BRF, true,
		                       "WARNING:  room #%d belongs to no zone.",
                               room->number);
                    }
                }
            }
		}
		if (!olc_lock && !(pulse % (900 * PASSES_PER_SEC)))	/* 15 minutes */
			autosave_zones(ZONE_AUTOSAVE);

		if (!(pulse % (666 * PASSES_PER_SEC)))
			bamf_quad_damage();

		if (pulse >= (30 * 60 * PASSES_PER_SEC)) {	/* 30 minutes */
			pulse = 0;
		}

		if (shutdown_count >= 0 && !(pulse % (PASSES_PER_SEC))) {
			shutdown_count--;	/* units of seconds */

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
				Housing.collectRent();
				Housing.save();
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
					playerIndex.getName(shutdown_idnum));
				circle_shutdown = TRUE;
				for (d = descriptor_list; d; d = next_d) {
					next_d = d->next;
					if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
						process_output(d);
				}
			}
		}

		/* give each descriptor an appropriate prompt */
		for (d = descriptor_list; d; d = d->next) {
			// New output crlf
			if (d->creature
					&& d->output[0]
					&& d->account->get_compact_level() < 2)
				SEND_TO_Q("\r\n", d);
			if (d->need_prompt) {
				send_prompt(d);
				// After prompt crlf
				if (d->creature
						&& d->output[0]
						&& (d->account->get_compact_level() == 0
							|| d->account->get_compact_level() == 2))
					SEND_TO_Q("\r\n", d);
				d->need_prompt = 0;
			}

		}

		/* send queued output out to the operating system (ultimately to user) */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (FD_ISSET(d->descriptor, &output_set) && *(d->output)) {
				if (process_output(d) < 0)
					close_socket(d);
			}
		}

		/* kick out folks in the CXN_DISCONNECT state */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (d->input_mode == CXN_DISCONNECT)
				close_socket(d);
		}


		update_unique_id();
		tics++;					/* tics since last checkpoint signal */
		sql_gc_queries();
		tmp_gc_strings();
		suppress_output = false; // failsafe
	}							/* while (!circle_shutdown) */
	/*  mem_cleanup(); */
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
	} else {					/* a->tv_sec > b->tv_sec */
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
		slog("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
			ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
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
get_from_q(struct txt_q *queue, char *dest, int *aliased, int length )
{
	struct txt_block *tmp;

	/* queue empty? */
	if (!queue->head)
		return 0;

	tmp = queue->head;
	strncpy(dest, queue->head->text,length);
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

	for (cur_blk = queue->head;cur_blk;cur_blk = next_blk) {
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
				|| t->account->get_compact_level() == 1
				|| t->account->get_compact_level() == 3)
			write_to_output("\r\n", t);
	}

	/* if we have enough space, just write to buffer and that's it! */
	if (t->bufspace >= size) {
		strcpy(t->output + t->bufptr, txt);
		t->bufspace -= size;
		t->bufptr += size;
	} else {					/* otherwise, try switching to a lrg buffer */
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
		} else {				/* else create a new one */
			CREATE(t->large_outbuf, struct txt_block, 1);
			CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
			buf_largecount++;
		}

		strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
		t->output = t->large_outbuf->text;	/* make big buffer primary */
		strcat(t->output, txt);	/* now add new text */

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
new_descriptor(int s)
{
	int desc, sockets_input_mode = 0;
	unsigned long addr;
	unsigned int i;
	static int last_desc = 0;	/* last descriptor number */
	struct descriptor_data *newd;
	struct sockaddr_in peer;
	struct hostent *from;
	extern char *GREETINGS;
	extern char *MUD_MOVED_MSG;

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
	memset((char *)newd, 0, sizeof(struct descriptor_data));

	/* find the sitename */

	if (nameserver_is_slow || !(from = gethostbyaddr((char *)&peer.sin_addr,
				sizeof(peer.sin_addr), AF_INET))) {
		if (!nameserver_is_slow)
			perror("gethostbyaddr");
		addr = ntohl(peer.sin_addr.s_addr);
		sprintf(newd->host, "%d.%d.%d.%d", (int)((addr & 0xFF000000) >> 24),
			(int)((addr & 0x00FF0000) >> 16), (int)((addr & 0x0000FF00) >> 8),
			(int)((addr & 0x000000FF)));
	} else {
		strncpy(newd->host, from->h_name, HOST_LENGTH);
		*(newd->host + HOST_LENGTH) = '\0';
	}

	/* determine if the site is banned */
	int bantype = isbanned(newd->host, buf2);
	if (bantype == BAN_ALL) {
        write_to_descriptor(desc, "**************************************************"
                                  "******************************\r\n");
        write_to_descriptor(desc, "                               T E M P U S  M U D\r\n");
        write_to_descriptor(desc, "**************************************************"
                                  "******************************\r\n");
        write_to_descriptor(desc, "\t\tWe're sorry, we have been forced to ban your "
                                  "IP address.\r\n\tIf you have never played here "
                                  "before, or you feel we have made\r\n\ta mistake, or "
                                  "perhaps you just got caught in the wake of\r\n\tsomeone "
                                  "elses trouble making, please mail "
                                  "unban@tempusmud.com.\r\n\tPlease include your account "
                                  "name and your character name(s)\r\n\tso we can siteok "
                                  "your IP.  We apologize for the inconvenience,\r\n\t and "
                                  "we hope to see you soon!");
		close(desc);
		mlog(Security::ADMINBASIC, LVL_GOD, CMP, true,
			"Connection attempt denied from [%s]", newd->host);
		free(newd);
		return 0;
	}

	/* Log new connections - probably unnecessary, but you may want it */
	mlog(Security::ADMINBASIC, LVL_GOD, CMP, true,
		"New connection from [%s]%s%s",
		newd->host,
		(bantype == BAN_SELECT) ? "(SELECT BAN)" : "",
		(bantype == BAN_NEW) ? "(NEWBIE BAN)" : "");

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

	if (++last_desc == 10000)
		last_desc = 1;
	newd->desc_num = last_desc;

	/* prepend to list */
	descriptor_list = newd;
	if (mud_moved) {
		SEND_TO_Q("\033[H\033[J", newd);
		SEND_TO_Q(MUD_MOVED_MSG, newd);
	} else if (!mini_mud) {
		SEND_TO_Q("\033[H\033[J", newd);
		SEND_TO_Q(GREETINGS, newd);
	} else
		SEND_TO_Q("(testmud)", newd);
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
	if (d->snoop_by.size() && d->creature && !IS_NPC(d->creature)) {
        for (unsigned x = 0; x < d->snoop_by.size(); x++) {
            SEND_TO_Q(CCRED(d->snoop_by[x]->creature, C_NRM), d->snoop_by[x]);
            SEND_TO_Q("{ ", d->snoop_by[x]);
            SEND_TO_Q(CCNRM(d->snoop_by[x]->creature, C_NRM), d->snoop_by[x]);
            SEND_TO_Q(d->output, d->snoop_by[x]);
            SEND_TO_Q(CCRED(d->snoop_by[x]->creature, C_NRM), d->snoop_by[x]);
            SEND_TO_Q(" } ", d->snoop_by[x]);
            SEND_TO_Q(CCNRM(d->snoop_by[x]->creature, C_NRM), d->snoop_by[x]);
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
write_to_descriptor(int desc, char *txt)
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
				return -1;		/* some error condition was encountered on
								   * read */
			} else
				return 0;		/* the read would have blocked: just means no
								   * data there */
		} else if (bytes_read == 0) {
			slog("EOF on socket read (connection broken by peer)");
			return -1;
		}
		/* at this point, we know we got some data from the read */

		*(read_point + bytes_read) = '\0';	/* terminate the string */

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
			if (*ptr == '\b') {	/* handle backspacing */
				if (write_point > tmp) {
					if (*(--write_point) == '$') {
						write_point--;
						space_left += 2;
					} else
						space_left++;
				}
			} else if (isascii(*ptr) && isprint(*ptr)) {
				*write_point = *ptr;
				if (*write_point == '$') {	/* copy one character */
					*(++write_point) = '$';	/* if it's a $, double it */
					space_left -= 1;
				} else if (*write_point == '&') {
					*(++write_point) = '&';
					space_left -= 1;
				}
				write_point++;
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
		if (t->snoop_by.size() && t->creature && !IS_NPC(t->creature)) {
            for (unsigned x = 0; x < t->snoop_by.size(); x++) {
                SEND_TO_Q(CCRED(t->snoop_by[x]->creature, C_NRM), t->snoop_by[x]);
                SEND_TO_Q("[ ", t->snoop_by[x]);
                SEND_TO_Q(CCNRM(t->snoop_by[x]->creature, C_NRM), t->snoop_by[x]);
                SEND_TO_Q(tmp, t->snoop_by[x]);
                SEND_TO_Q(CCRED(t->snoop_by[x]->creature, C_NRM), t->snoop_by[x]);
                SEND_TO_Q(" ]\r\n", t->snoop_by[x]);
                SEND_TO_Q(CCNRM(t->snoop_by[x]->creature, C_NRM), t->snoop_by[x]);
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
					" ye vile profaner of CPU time!", TRUE, t->creature, 0, 0,
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
					send_to_desc(t, "You don't have any commands to revoke!\r\n");
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
	vector<descriptor_data *>::iterator vi;

	close(d->descriptor);
	flush_queues(d);

	// Forget those this descriptor is snooping
	if (d->snooping) {
	  vi = find(d->snooping->snoop_by.begin(), d->snooping->snoop_by.end(), d);
	  if (vi != d->snooping->snoop_by.end())
		d->snooping->snoop_by.erase(vi);
    }

	// Forget those snooping on this descriptor
    for (unsigned x = 0; x < d->snoop_by.size(); x++) {
        SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by[x]);
		d->snoop_by[x]->snooping = NULL;
    }

	if (d->original) {
		d->creature->desc = NULL;
		d->creature = d->original;
		d->original = NULL;
		d->creature->desc = d;
	}

	if (d->creature && IS_PLAYING(d)) {
	  // Lost link in-game
		d->creature->player.time.logon = time(0);
		d->creature->saveToXML();
		act("$n has lost $s link.", true, d->creature, 0, 0, TO_ROOM);
		mlog(Security::ADMINBASIC,
			 MAX(LVL_AMBASSADOR, GET_INVIS_LVL(d->creature)),
			 NRM, false, "Closing link to: %s [%s] ", GET_NAME(d->creature),
			d->host);
		d->account->logout(d, true);
		d->creature->desc = NULL;
		GET_OLC_OBJ(d->creature) = NULL;
	} else if (d->account) {
		if (d->creature) {
			d->creature->saveToXML();
			delete d->creature;
			d->creature = NULL;
		}
		mlog(Security::ADMINBASIC, LVL_AMBASSADOR, NRM, false,
                "%s[%d] logging off from %s", 
                d->account->get_name(), d->account->get_idnum(), d->host);
		d->account->logout(d, false);
	} else {
		slog("Losing descriptor without account");
    }

	REMOVE_FROM_LIST(d, descriptor_list, next);

	if (d->showstr_head)
		free(d->showstr_head);

	// Free text editor object.
	if (d->text_editor)
		delete d->text_editor;

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
		safe_exit(1);
	}
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */


void
checkpointing(int sig = 0)
{
	if (!tics) {
		errlog("CHECKPOINT shutdown: tics not updated");
		slog("Last command: %s %s.", playerIndex.getName(last_cmd[0].idnum),
			last_cmd[0].string);
		raise(SIGSEGV);
	} else
		tics = 0;
}



void
unrestrict_game(int sig = 0)
{
	extern struct ban_list_element *ban_list;
	extern int num_invalid;

	mudlog(LVL_AMBASSADOR, BRF, true,
		"Received SIGUSR2 - completely unrestricting game (emergent)");
	ban_list = NULL;
	restrict = 0;
	num_invalid = 0;
}


void
hupsig(int sig = 0)
{
	slog("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");

	mudlog(LVL_AMBASSADOR, BRF, true,
		"Received external signal - shutting down for reboot in 60 sec..");

	send_to_all("\007\007:: Tempus REBOOT in 60 seconds ::\r\n");
	shutdown_idnum = -1;
	shutdown_count = 60;
	shutdown_mode = SHUTDOWN_REBOOT;

}

void
pipesig(int sig = 0)
{
	errlog(" Ignoring SIGPIPE signal.");
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
	 * caught in an infinite loop for more than 3 minutes
	 */
	interval.tv_sec = 180;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	my_signal(SIGVTALRM, checkpointing);

	/* just to be on the safe side: */
	my_signal(SIGHUP, hupsig);
	my_signal(SIGINT, hupsig);
	my_signal(SIGTERM, hupsig);
	my_signal(SIGPIPE, SIG_IGN);
	my_signal(SIGALRM, SIG_IGN);

}



/* ****************************************************************
*       Public routines for system-to-player-communication        *
*******************************************************************/

void
send_to_char(struct Creature *ch, const char *str, ...)
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
send_to_desc(descriptor_data *d, const char *str, ...)
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
			if (d->account && d->account->get_ansi_level() > 0) {
				if (isupper(*read_pt)) {
					*read_pt = tolower(*read_pt);
						// A few extra normal tags never hurt anyone...
					if (d->account->get_ansi_level() > 2)
						SEND_TO_Q(KBLD, d);
				}
				switch (*read_pt) {
				case 'n':
					SEND_TO_Q(KNRM, d); break;
				case 'r':
					SEND_TO_Q(KRED, d); break;
				case 'g':
					SEND_TO_Q(KGRN, d); break;
				case 'y':
					SEND_TO_Q(KYEL, d); break;
				case 'm':
					SEND_TO_Q(KMAG, d); break;
				case 'c':
					SEND_TO_Q(KCYN, d); break;
				case 'b':
					SEND_TO_Q(KBLU, d); break;
				case 'w':
					SEND_TO_Q(KWHT, d); break;
				case '&':
					SEND_TO_Q("&", d); break;
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
send_to_all(char *messg)
{
	struct descriptor_data *i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (!i->input_mode)
				SEND_TO_Q(messg, i);
}

void
send_to_clerics(char *messg)
{
	struct descriptor_data *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next) {
		if (!i->input_mode && i->creature && AWAKE(i->creature) &&
			!PLR_FLAGGED(i->creature, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
			PRIME_MATERIAL_ROOM(i->creature->in_room)
			&& IS_CLERIC(i->creature)) {
			SEND_TO_Q(messg, i);
		}
	}
}

void
send_to_outdoor(char *messg, int isecho)
{
	struct descriptor_data *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next)
		if (!i->input_mode && i->creature && AWAKE(i->creature) &&
			(!isecho || !PRF2_FLAGGED(i->creature, PRF2_NOGECHO)) &&
			!PLR_FLAGGED(i->creature, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
			OUTSIDE(i->creature)
			&& PRIME_MATERIAL_ROOM(i->creature->in_room))
			SEND_TO_Q(messg, i);
}

void
send_to_newbie_helpers(char *messg)
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

/* mode == TRUE -> hide from sender.  FALSE -> show to all */
void
send_to_comm_channel(struct Creature *ch, char *buf, int chan, int mode,
	int hide_invis)
{
	SPECIAL(master_communicator);
	Creature *receiver;
	obj_data *obj = NULL;

	for (obj = object_list; obj; obj = obj->next) {
		if (obj->in_obj || !IS_COMMUNICATOR(obj))
			continue;

		if (!ENGINE_STATE(obj))
			continue;

		if (GET_OBJ_SPEC(obj) != master_communicator &&
				COMM_CHANNEL(obj) != chan)
			continue;

		if (obj->in_room) {
			act("$p makes some noises.", FALSE, 0, obj, 0, TO_ROOM);
			continue;
		}

		receiver = obj->carried_by ? obj->carried_by:obj->worn_by;
		if (!receiver || !receiver->desc)
			continue;
		
		if (!IS_PLAYING(receiver->desc) || !receiver->desc ||
				PLR_FLAGGED(receiver, PLR_WRITING) ||
				PLR_FLAGGED(receiver, PLR_OLC))
			continue;
			
		if (!COMM_UNIT_SEND_OK(ch, receiver))
			continue;

		if (mode && receiver == ch)
			continue;

		if (hide_invis && !can_see_creature(receiver, ch))
			continue;

		if (GET_OBJ_SPEC(obj) == master_communicator)
			send_to_char(receiver, "%s::%s [%d]::%s ", CCYEL(receiver, C_NRM),
				OBJS(obj, receiver), chan, CCNRM(receiver, C_NRM));
		else
			send_to_char(receiver, "%s::%s::%s ", CCYEL(receiver, C_NRM),
				OBJS(obj, receiver), CCNRM(receiver, C_NRM));
		act(buf, true, ch, obj, receiver, TO_VICT);
	}
}

void
send_to_zone(char *messg, struct zone_data *zn, int outdoor)
{
	struct descriptor_data *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next)
		if (!i->input_mode && i->creature && AWAKE(i->creature) &&
			!PLR_FLAGGED(i->creature, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
			i->creature->in_room->zone == zn &&
			(!outdoor ||
				(OUTSIDE(i->creature) &&
					PRIME_MATERIAL_ROOM(i->creature->in_room))))
			SEND_TO_Q(messg, i);
}

void
send_to_room(char *messg, struct room_data *room)
{
	struct Creature *i;
	room_data *to_room;
	struct obj_data *o, *obj = NULL;
	char *str;
	int j;

	if (!room || !messg)
		return;
	CreatureList::iterator it = room->people.begin();
	for (; it != room->people.end(); ++it) {
		i = *it;
		if (i->desc && !PLR_FLAGGED(i, PLR_OLC | PLR_WRITING | PLR_MAILING))
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
					it = obj->in_room->people.begin();
					for (; it != obj->in_room->people.end(); ++it) {
						i = *it;
						if (i->desc
							&& !PLR_FLAGGED(i,
								PLR_OLC | PLR_WRITING | PLR_MAILING))
							SEND_TO_Q(str, i->desc);
					}
				}
			}
		}
	}

	/* see if there is a podium in the room */
	for (o = room->contents; o; o = o->next_content)
		if (GET_OBJ_TYPE(o) == ITEM_PODIUM)
			break;

	if (o) {
		str = tmp_sprintf("(remote) %s", messg);
		for (j = 0; j < NUM_OF_DIRS; j++)
			if (ABS_EXIT(room, j) && ABS_EXIT(room, j)->to_room &&
				room != ABS_EXIT(room, j)->to_room &&
				!IS_SET(ABS_EXIT(room, j)->exit_info, EX_ISDOOR | EX_CLOSED)) {
				it = ABS_EXIT(room, j)->to_room->people.begin();
				for (; it != ABS_EXIT(room, j)->to_room->people.end(); ++it) {
					i = *it;
					if (i->desc && !PLR_FLAGGED(i, PLR_OLC))
						SEND_TO_Q(str, i->desc);
				}
			}
	}

	for (o = room->contents; o; o = o->next_content) {
		if (GET_OBJ_TYPE(o) == ITEM_CAMERA && o->in_room) {
			to_room = real_room(GET_OBJ_VAL(o, 0));
			if (to_room) {
				send_to_room(tmp_sprintf("(%s) %s", o->in_room->name, messg),
					to_room);
			}
		}
	}
}

void
send_to_clan(char *messg, int clan)
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


char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
if ((pointer) == NULL) i = ACTNULL; else i = (expression);

/* higher-level communication: the act() function */
void
perform_act(const char *orig, struct Creature *ch, struct obj_data *obj,
	void *vict_obj, struct Creature *to, int mode)
{
	register const char *i = 0;
	register const char *s = orig;
	register char *buf;
	static char lbuf[MAX_STRING_LENGTH];
	char outbuf[MAX_STRING_LENGTH];
	char *first_printed_char = 0;

	if (!to || !to->desc || PLR_FLAGGED((to), PLR_WRITING | PLR_OLC))
		return;

	if (!to->in_room) {
		errlog("to->in_room NULL in perform_act.");
		return;
	}
	buf = lbuf;

	for (;;) {
		if (*s == '$') {
			switch (*(++s)) {
			case 'n':
				i = PERS(ch, to);
				break;
			case 'N':
				CHECK_NULL(vict_obj, PERS((struct Creature *)vict_obj, to));
				break;
			case 't':
				i = (ch==to)?"you":PERS(ch, to);
				break;
			case 'T':
				if (ch==vict_obj) {
					if (vict_obj==to)
						i = "yourself";
					else if (IS_MALE((Creature *)vict_obj))
						i = "himself";
					else if (IS_FEMALE((Creature *)vict_obj))
						i = "herself";
					else
						i = "itself";
				} else if (to==vict_obj) {
					i = "you";
				} else {
					CHECK_NULL(vict_obj, PERS((struct Creature *)vict_obj, to));
				}
				break;
			case 'm':
				i = HMHR(ch);
				break;
			case 'M':
				CHECK_NULL(vict_obj, HMHR((struct Creature *)vict_obj));
				break;
			case 's':
				i = HSHR(ch);
				break;
			case 'S':
				CHECK_NULL(vict_obj, HSHR((struct Creature *)vict_obj));
				break;
			case 'e':
				i = HSSH(ch);
				break;
			case 'E':
				CHECK_NULL(vict_obj, HSSH((struct Creature *)vict_obj));
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
				i = GET_MOOD(ch) ? GET_MOOD(ch):"";
				break;
			case '{':
				if (GET_MOOD(ch)) {
					i = GET_MOOD(ch);
					while (*s && *s != '}')
						s++;
				} else {
					s++;
					i = tmp_strdup(s, "}");
					s += strlen(i);
				}
				break;
			case 'A':
				CHECK_NULL(vict_obj, SANA((struct obj_data *)vict_obj));
				break;
			case 'F':
				CHECK_NULL(vict_obj, fname((char *)vict_obj));
				break;
            case '%':
                i = (ch == to) ? "":"s";
                break;
            case '^':
                i = (ch == to) ? "":"es";
                break;
			case '$':
				i = "$";
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
					i = KRED; break;
				case 'g':
					i = clr(to, C_NRM)?KGRN:KNRM; break;
				case 'y':
					i = clr(to, C_NRM)?KYEL:KNRM; break;
				case 'm':
					i = KMAG; break;
				case 'c':
					i = clr(to, C_NRM)?KCYN:KNRM; break;
				case 'b':
					i = KBLU; break;
				case 'w':
					i = KWHT; break;
				case '&':
					i = "&"; break;
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

	if (mode == 1) {
		sprintf(outbuf, "(outside) %s", lbuf);
    } else if (mode == 2) {
		sprintf(outbuf, "(remote) %s", lbuf);
    } else if (mode == 3) {
        room_data *toroom = NULL;
        if( ch != NULL && ch->in_room != NULL ) {
            toroom = ch->in_room;
        } else if( obj != NULL && obj->in_room != NULL ) {
            toroom = obj->in_room;
        }
		sprintf(outbuf, "(%s) %s", (toroom) ? toroom->name:"remote", lbuf);
    } else {
		strcpy(outbuf, lbuf);
    }

	SEND_TO_Q(outbuf, to->desc);
	if (COLOR_LEV(to) > 0)
		SEND_TO_Q(KNRM, to->desc);
}

#define SENDOK(ch) (AWAKE(ch) || sleep)

void
act_if(const char *str, int hide_invisible, struct Creature *ch,
	struct obj_data *obj, void *vict_obj, int type, act_if_predicate pred)
{
	struct Creature *to;
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
        room = ((Creature *)vict_obj)->in_room;
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
		if ((to = (struct Creature *)vict_obj) && SENDOK(to))
			perform_act(str, ch, obj, vict_obj, to, 0);
		return;
	}
	/* ASSUMPTION: at this point we know type must be TO_NOTVICT TO_ROOM,
       or TO_VICT_RM */

	if (!room && ch && ch->in_room != NULL) {
		room = ch->in_room;
    }
	else if (obj && obj->in_room != NULL) {
		room = obj->in_room;
	} else if (!room && (to = (struct Creature *)vict_obj) && to->in_room != NULL) { //needed for bombs
        room = to->in_room;
    }

    if (!room) {
		errlog("no valid target to act()!");
		raise(SIGSEGV);
		return;
	}
	CreatureList::iterator it = room->people.begin();
	for (; it != room->people.end(); ++it) {
		if (!pred(ch, obj, vict_obj, (*it), 0))
			continue;
		if (SENDOK((*it)) &&
			!(hide_invisible && ch && !can_see_creature((*it), ch)) &&
			((*it) != ch) && (type == TO_ROOM || ((*it) != vict_obj)))
			perform_act(str, ch, obj, vict_obj, (*it), 0);
	}

	/** check for vehicles in the room **/
	for (o = room->contents; o; o = o->next_content) {
		if (IS_OBJ_TYPE(o, ITEM_VEHICLE) && (!hidecar || o != cur_car)) {
			for (o2 = object_list; o2; o2 = o2->next) {
				if (((IS_OBJ_TYPE(o2, ITEM_V_DOOR) && !CAR_CLOSED(o)) ||
						(IS_OBJ_TYPE(o2, ITEM_V_WINDOW) && !CAR_CLOSED(o2))) &&
					ROOM_NUMBER(o2) == ROOM_NUMBER(o) &&
					GET_OBJ_VNUM(o) == V_CAR_VNUM(o2) && o2->in_room) {

					it = o2->in_room->people.begin();
					for (; it != o2->in_room->people.end(); ++it) {
						if (!pred(ch, obj, vict_obj, (*it), 1))
							continue;
						if (SENDOK((*it)) &&
							!(hide_invisible && ch && !can_see_creature((*it), ch)) &&
							((*it) != ch) && (type == TO_ROOM
								|| ((*it) != vict_obj))) {
							perform_act(str, ch, obj, vict_obj, (*it), 1);
						}
					}
					break;
				}
			}
		}
	}

	/* see if there is a podium in the room */
	for (o = room->contents; o; o = o->next_content)
		if (GET_OBJ_TYPE(o) == ITEM_PODIUM)
			break;

	if (o) {
		for (j = 0; j < NUM_OF_DIRS; j++) {
			if (ABS_EXIT(room, j) && ABS_EXIT(room, j)->to_room &&
				room != ABS_EXIT(room, j)->to_room &&
				!IS_SET(ABS_EXIT(room, j)->exit_info, EX_ISDOOR | EX_CLOSED)) {


				it = ABS_EXIT(room, j)->to_room->people.begin();
				for (; it != ABS_EXIT(room, j)->to_room->people.end(); ++it) {
					if (!pred(ch, obj, vict_obj, (*it), 2))
						continue;
					if (SENDOK((*it)) &&
						!(hide_invisible && ch && !can_see_creature((*it), ch)) &&
						((*it) != ch) && (type == TO_ROOM || ((*it) != vict_obj)))
						perform_act(str, ch, obj, vict_obj, (*it), 2);
				}
			}
		}
	}

	for (o = room->contents; o; o = o->next_content) {
		if (GET_OBJ_TYPE(o) == ITEM_CAMERA && o->in_room) {
			room = real_room(GET_OBJ_VAL(o, 0));
			if (room) {
				it = room->people.begin();
				for (; it != room->people.end(); ++it) {
					if (!pred(ch, obj, vict_obj, (*it), 3))
						continue;
					if (SENDOK((*it)) &&
						!(hide_invisible && ch && !can_see_creature((*it), ch)) &&
						((*it) != ch) && ((*it) != vict_obj))
						perform_act(str, ch, obj, vict_obj, (*it), 3);
				}
			}
		}
	}
}

bool
standard_act_predicate(struct Creature *ch, struct obj_data *obj, void *vict_obj, struct Creature *to, int mode)
{
	return true;
}

void
act(const char *str, int hide_invisible, struct Creature *ch,
	struct obj_data *obj, void *vict_obj, int type)
{
	act_if(str, hide_invisible, ch, obj, vict_obj, type, standard_act_predicate);
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
			act("$p starts spinning faster and faster, and disappears in a flash!", FALSE, 0, quad, 0, TO_ROOM);
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
			FALSE, 0, quad, 0, TO_ROOM);

}


void
push_command_onto_list(Creature *ch, char *string)
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
	last_cmd[0].roomnum = (ch->in_room) ? ch->in_room->number:-1;
	strncpy(last_cmd[0].room, 
                (ch->in_room && ch->in_room->name) ? 
                    ch->in_room->name : "<NULL>", MAX_INPUT_LENGTH );
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
			mlog(Security::ADMINBASIC, LVL_IMMORT, CMP, true, "Descriptor idling out after 10 minutes");
			SEND_TO_Q("Idle time limit reached, disconnecting.\r\n", d);
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
	for (idx = 0;idx < 10;idx++) {
		snprintf(path, 255, "players/character/%d", idx);
		mkdir(path, 0755);
	}
	mkdir("players/equipment", 0755);
	for (idx = 0;idx < 10;idx++) {
		snprintf(path, 255, "players/equipment/%d", idx);
		mkdir(path, 0755);
	}
	mkdir("players/housing", 0755);
	for (idx = 0;idx < 10;idx++) {
		snprintf(path, 255, "players/housing/%d", idx);
		mkdir(path, 0755);
	}
    mkdir("players/corpses", 0755);
	for (idx = 0;idx < 10;idx++) {
		snprintf(path, 255, "players/corpses/%d", idx);
		mkdir(path, 0755);
	}
}
