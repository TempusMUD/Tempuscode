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

#define __COMM_C__

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

#include "structs.h"
#include "spells.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "screen.h"
#include "intermud.h"
#include "vehicle.h"
#include "help.h"

/* externs */
extern HelpCollection *Help;
extern int restrict;
extern int mini_mud;
extern int olc_lock;
extern int no_rent_check;
extern int no_initial_zreset;
extern int log_cmds;
extern FILE *player_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;
extern struct obj_data *cur_car;
extern struct zone_data *d_quad_zone;
extern char help[];
extern struct obj_data *object_list;

/* local globals */
struct descriptor_data *descriptor_list = NULL;	/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int avail_descs = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
int log_cmds = 0;               /* log cmds */
int shutdown_count = -1;    /* shutdown countdown */
int shutdown_idnum = -1;        /* idnum of person calling shutdown */
int shutdown_mode = SHUTDOWN_NONE; /* what type of shutdown */


struct last_command_data last_cmd[NUM_SAVE_CMDS];

extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */


/* functions in this file */
int get_from_q(struct txt_q * queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_avail_descs(void);
int process_output(struct descriptor_data * t);
int process_input(struct descriptor_data * t);
void close_socket(struct descriptor_data * d);
struct timeval timediff(struct timeval * a, struct timeval * b);
void flush_queues(struct descriptor_data * d);
void nonblock(int s);
int perform_subst(struct descriptor_data * t, char *orig, char *subst);
int perform_alias(struct descriptor_data * d, char *orig);
void record_usage(void);
void make_prompt(struct descriptor_data * point);
void bamf_quad_damage(void);
void push_command_onto_list(int idnum, char *comm);
void descriptor_update(void);

/* extern fcnts */
void boot_db(void);
void boot_world(void);
void zone_update(void);
void affect_update(void);	/* In spells.c */
void point_update(void);	/* In limits.c */
void mobile_activity(void);
void burn_update(void);
void dynamic_object_pulse();
void flow_room(int pulse);
void path_activity();
void string_add(struct descriptor_data * d, char *str);
void editor(struct descriptor_data *d, char *buffer);
void perform_violence(void);
void show_string(struct descriptor_data * d, char *input);
int isbanned(char *hostname, char *blocking_hostname);
int intermud_desc;
int connected_to_intermud;
void init_intermud_socket(void);
void incoming_intermud_message(int intermud_desc);
void weather_and_time(int mode);
void House_checkrent(void);
char *diag_conditions(struct char_data *ch);
void autosave_zones(int SAVE_TYPE);
void mem_cleanup(void);
void retire_trails(void);
void qp_reload( int sig = 0 );

/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

int 
main(int argc, char **argv)
{
    int port;
    char buf[512];
    int pos = 1;
    char *dir;

    port = DFLT_PORT;
    dir = DFLT_DIR;

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
	    //      olc_lock = LVL_IMPL;
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
	    /*
	      if (!(cmd_log_fl = fopen(CMD_LOG_FILE, "w"))) {
	      log("SYSERR:  Unable to open CMD_LOG_FILE for write.");
	      log_cmds = FALSE;
	      }
	    */
	    break;
      
	default:
	    sprintf(buf, "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
	    slog(buf);
	    break;
	}
	pos++;
    }

    if (pos < argc) {
	if (!isdigit(*argv[pos])) {
	    fprintf(stderr, "Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
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

    sprintf(buf, "Using %s as data directory.", dir);
    slog(buf);

    if (scheck) {
	boot_world();
	slog("Done.");
	safe_exit(0);
    } else {
	sprintf(buf, "Running game on port %d.", port);
	slog(buf);
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

    my_srand(time(0));

    boot_db();

    slog("Opening mother connection.");
    mother_desc = init_socket(port);
  
/*  if (port == 2020) { */
    slog("Opening InterMUD connection.");
    init_intermud_socket();

    avail_descs = get_avail_descs();


    slog("Signal trapping.");
    signal_setup();

    slog("Entering game loop.");

    game_loop(mother_desc);

    slog("Closing all sockets.");
    while (descriptor_list)
	close_socket(descriptor_list);

    close(mother_desc);
    close(intermud_desc);
    fclose(player_fl);

    if (circle_reboot) {
	slog("Rebooting.");
	exit(52);			/* what's so great about HHGTTG, anyhow? */
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
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
	perror("setsockopt SNDBUF");
	safe_exit(1);
    }
#endif

#if defined(SO_REUSEADDR)
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
	perror("setsockopt REUSEADDR");
	safe_exit(1);
    }
#endif

#if defined(SO_REUSEPORT)
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof(opt)) < 0) {
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

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *) & sa, sizeof(sa)) < 0) {
	perror("bind");
	close(s);
	safe_exit(1);
    }
    nonblock(s);
    listen(s, 5);
    return s;
}


int get_avail_descs(void)
{
    int max_descs = 0;

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
    sprintf(buf, "Setting player limit to %d.", max_descs);
    slog(buf);
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
    char comm[MAX_INPUT_LENGTH];
    struct descriptor_data *d, *next_d;
    int pulse = 0, mins_since_crashsave = 0, maxdesc, aliased;

    /* initialize various time values */
    null_time.tv_sec = 0;
    null_time.tv_usec = 0;
    opt_time.tv_usec = OPT_USEC;
    opt_time.tv_sec = 0;
    gettimeofday(&last_time, (struct timezone *) 0);

    /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
    while (!circle_shutdown) {

	/* Sleep if we don't have any connections */
	if (descriptor_list == NULL) {
	    slog("No connections.  Going to sleep.");
	    FD_ZERO(&input_set);
	    FD_SET(mother_desc, &input_set);
	    if (connected_to_intermud == 1)
		FD_SET(intermud_desc, &input_set);
	    if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
		if (errno == EINTR)
		    slog("Waking up to process signal.");
		else
		    perror("Select coma");
	    } else
		slog("New connection.  Waking up.");
	    gettimeofday(&last_time, (struct timezone *) 0);
	}
	/* Set up the input, output, and exception sets for select(). */
	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
	FD_ZERO(&exc_set);
	FD_SET(mother_desc, &input_set);
	if (connected_to_intermud == 1) {
	    FD_SET(intermud_desc, &input_set);
	    maxdesc = intermud_desc;
	} else
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
	    errno = 0;		/* clear error condition */

	    /* figure out for how long we have to sleep */
	    gettimeofday(&now, (struct timezone *) 0);
	    timespent = timediff(&now, &last_time);
	    timeout = timediff(&opt_time, &timespent);

	    /* sleep until the next 0.1 second mark */
	    if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
		if (errno != EINTR) {
		    perror("Select sleep");
		    safe_exit(1);
		}
	} while (errno);

	/* record the time for the next pass */
	gettimeofday(&last_time, (struct timezone *) 0);

	/* poll (without blocking) for new input, output, and exceptions */
	if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time)<0) {
	    perror("Select poll");
	    return;
	}
	/* New connection waiting for us? */
	if (FD_ISSET(mother_desc, &input_set))
	    new_descriptor(mother_desc);

	if (FD_ISSET(intermud_desc, &input_set) && connected_to_intermud == 1)
	    incoming_intermud_message(intermud_desc);

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

	    if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
		if (d->character) {
		    d->character->char_specials.timer = 0;
		    d->idle = 0;
		    if (!d->connected && *comm)
			push_command_onto_list(GET_IDNUM(d->character), comm);
		    if (!d->connected && GET_WAS_IN(d->character) != NULL) {
			if (d->character->in_room != NULL)
			    char_from_room(d->character);
			char_to_room(d->character, GET_WAS_IN(d->character));
			GET_WAS_IN(d->character) = NULL;
			act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
		    }
		}
      
		d->wait = 1;
		d->prompt_mode = 1;


		if (d->str)			/* writing boards, mail, etc.	*/
		    editor(d, comm);
		else if (d->showstr_point)	/* reading something w/ pager	*/
		    show_string(d, comm);
		else if (d->connected != CON_PLAYING)	/* in menus, etc.	*/
		    nanny(d, comm);
		else {				/* else: we're playing normally */
		    if (aliased) /* to prevent recursive aliases */
			d->prompt_mode = 0;
		    else {
			if (perform_alias(d, comm))	/* run it through aliasing system */
			    get_from_q(&d->input, comm, &aliased);
		    }
		    command_interpreter(d->character, comm); /* send it to interpreter */
		}
	    }
	}

	/* send queued output out to the operating system (ultimately to user) */
	for (d = descriptor_list; d; d = next_d) {
	    next_d = d->next;
	    if (FD_ISSET(d->descriptor, &output_set) && *(d->output)) {
		if (process_output(d) < 0)
		    close_socket(d);
		else if (!d->prompt_mode)
		    d->prompt_mode = 2;
	    }
	}

	/* kick out folks in the CON_CLOSE state */
	for (d = descriptor_list; d; d = next_d) {
	    next_d = d->next;
	    if (STATE(d) == CON_CLOSE)
		close_socket(d);
	}

	/* give each descriptor an appropriate prompt */
	for (d = descriptor_list; d; d = d->next) {
	    if (d->prompt_mode) {
		make_prompt(d);
		d->prompt_mode = 0;
	    }
	}

	/* handle heartbeat stuff */
	/* Note: pulse now changes every 0.10 seconds  */

	pulse++;

	if (!(pulse % (PULSE_ZONE)))
	    zone_update();
	if (!((pulse + 1) % PULSE_MOBILE))
	    mobile_activity();
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

	if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
	    weather_and_time(1);
	    affect_update();
	    point_update();
	    fflush(player_fl);
	    descriptor_update();
        Help->Sync();
	}
	if (auto_save) {
	    if (!(pulse % (60 * PASSES_PER_SEC))) {	/* 1 minute */
		if (++mins_since_crashsave >= autosave_time) {
		    mins_since_crashsave = 0;
		    Crash_save_all();
		    House_save_all(0);
		}
		House_checkrent();
		House_countobjs();
	    }
	}

	if (!(pulse % (300 * PASSES_PER_SEC)))	/* 5 minutes */
	    record_usage();

	if (!(pulse % (130 * PASSES_PER_SEC))) {      // 2.1 minutes
	    retire_trails();
	}
	if (!olc_lock && !(pulse % (900 * PASSES_PER_SEC)))      /* 15 minutes */
	    autosave_zones(ZONE_AUTOSAVE);
    
	if (!(pulse % (666 * PASSES_PER_SEC))) 
	    bamf_quad_damage();

	if (pulse >= (30 * 60 * PASSES_PER_SEC)) {	/* 30 minutes */
	    pulse = 0;
	}

	if (shutdown_count >= 0 && !(pulse % (PASSES_PER_SEC))) {
	    shutdown_count--;   /* units of seconds */
      
	    if (shutdown_count == 10)
		send_to_all(":: Tempus REBOOT in 10 seconds ::\r\n");
	    else if (shutdown_count == 30)
		send_to_all(":: Tempus REBOOT in 30 seconds ::\r\n");
	    else if (shutdown_count && !(shutdown_count % 60)) {
		sprintf(buf, ":: Tempus REBOOT in %d minute%s ::\r\n", shutdown_count / 60,
			shutdown_count == 60 ? "" : "s");
		send_to_all(buf);
	    } else if (shutdown_count <= 0) {
		Crash_save_all();
		House_save_all(TRUE);
		autosave_zones(ZONE_RESETSAVE);
		send_to_all(":: Tempus REBOOTING ::\r\n\r\n"
			    "You feel your reality fading, as the universe spins away\r\n"
			    "before your eyes and the icy cold of nothingness settles\r\n"
			    "into your flesh.  With a jolt, you feel the thread snap,\r\n"
			    "severing your mind from the world known as Tempus.\r\n\r\n");
		if (shutdown_mode == SHUTDOWN_DIE || shutdown_mode == SHUTDOWN_PAUSE) {
		    send_to_all("Shutting down for maintenance, try again in half an hour.\r\n");
		    if (shutdown_mode == SHUTDOWN_DIE)
			touch("../.killscript");
		    else
			touch("../pause");
		} else {
		    send_to_all("Rebooting now, we will be back online in a few minutes.\r\n");
		    touch("../.fastboot");
		}

                send_to_all( "Please visit our website at http://tempusmud.com\r\n" );
                
		sprintf(buf, "(GC) %s called by %s EXECUTING.", 
			(shutdown_mode == SHUTDOWN_DIE || shutdown_mode == SHUTDOWN_DIE) ?
			"Shutdown" : "Reboot", get_name_by_id(shutdown_idnum));
		slog(buf);
		circle_shutdown = TRUE;
		for (d = descriptor_list; d; d = next_d) {
		    next_d = d->next;
		    if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
			process_output(d);
		}
	    }
	}
	tics++;			/* tics since last checkpoint signal */
    } /* while (!circle_shutdown) */
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
timediff(struct timeval * a, struct timeval * b)
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
    } else { /* a->tv_sec > b->tv_sec */
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
    int sockets_connected = 0, sockets_playing = 0;
    struct descriptor_data *d;
    char buf[256];

    for (d = descriptor_list; d; d = d->next) {
	sockets_connected++;
	if (!d->connected)
	    sockets_playing++;
    }

    sprintf(buf, "nusage: %-3d sockets connected, %-3d sockets playing",
	    sockets_connected, sockets_playing);
    slog(buf);

#ifdef RUSAGE
    {
	struct rusage ru;

	getrusage(0, &ru);
	sprintf(buf, "rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
		ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	slog(buf);
    }
#endif

}


void 
make_prompt(struct descriptor_data * d)
{
    char lnum[5];
    char colorbuf[ 100 ];	
  
    if (d->str)
	if (d->editor_mode == 1)
	    write_to_descriptor(d->descriptor, "[TED] ");
	else {
	    sprintf(lnum, "%-2d] ", d->editor_cur_lnum);
	    write_to_descriptor(d->descriptor, lnum);
	}
    else if (d->showstr_point) {
	sprintf(buf, " %s%s******%s  Press return to continue, q to quit  %s******%s",
		CCBLD(d->character, C_CMP), 
		CCRED(d->character, C_NRM), CCWHT(d->character, C_NRM),
		CCRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
	write_to_descriptor(d->descriptor, buf);
    } else if (!d->connected && (d->prompt_mode != 2 ||
				 PRF2_FLAGGED(d->character, PRF2_AUTOPROMPT))) {
	char prompt[MAX_INPUT_LENGTH];

	*prompt = '\0';

	if (GET_INVIS_LEV(d->character))
	    sprintf(prompt, "%s(%si%d%s)%s ", CCMAG(d->character, C_NRM),
		    CCRED(d->character, C_NRM), GET_INVIS_LEV(d->character), 
		    CCMAG(d->character, C_NRM), CCNRM(d->character, C_NRM));
	else if (GET_REMORT_INVIS(d->character) && 
		 GET_LEVEL(d->character) < LVL_AMBASSADOR)
	    sprintf(prompt, "%s(%sr%d%s)%s ", CCBLU(d->character, C_NRM),
		    CCMAG(d->character, C_NRM), GET_REMORT_INVIS(d->character), 
		    CCBLU(d->character, C_NRM), CCNRM(d->character, C_NRM));

	if (PRF_FLAGGED(d->character, PRF_DISPHP))
	    sprintf(prompt, "%s%s%s< %s%d%s%sH%s ", prompt, 
		    CCWHT(d->character, C_SPR), CCBLD(d->character, C_CMP), 
		    CCGRN(d->character, C_SPR), GET_HIT(d->character), 
		    CCNRM(d->character, C_SPR),
		    CCYEL_BLD(d->character, C_CMP), CCNRM(d->character, C_SPR));

	if (PRF_FLAGGED(d->character, PRF_DISPMANA))
	    sprintf(prompt, "%s%s%s%d%s%sM%s ", prompt, 
		    CCBLD(d->character, C_CMP), CCMAG(d->character, C_SPR),
		    GET_MANA(d->character), CCNRM(d->character, C_SPR),
		    CCYEL_BLD(d->character, C_CMP), CCNRM(d->character, C_SPR));
    
	if (PRF_FLAGGED(d->character, PRF_DISPMOVE))
	    sprintf(prompt, "%s%s%s%d%s%sV%s ", prompt,
		    CCCYN(d->character, C_SPR), CCBLD(d->character, C_CMP), 
		    GET_MOVE(d->character), CCNRM(d->character, C_SPR),
		    CCYEL_BLD(d->character, C_CMP), CCNRM(d->character, C_SPR));

	if ( PRF2_FLAGGED( d->character, PRF2_DISPALIGN ) ) {
             
	    if( IS_GOOD( d->character ) ) {
            sprintf( colorbuf, "%s", CCCYN( d->character, C_SPR ) );
        } else if ( IS_EVIL( d->character ) ) {
            sprintf( colorbuf, "%s", CCRED( d->character, C_SPR ) );
	    } else {
            sprintf( colorbuf, "%s", CCWHT(d->character, C_SPR ) );
	    } 

	    sprintf( prompt, "%s%s%s%d%s%sA%s ", prompt,
                    colorbuf, CCBLD( d->character,C_CMP ),
                    GET_ALIGNMENT( d->character ), CCNRM( d->character, C_SPR ),
                    CCYEL_BLD( d->character, C_CMP ), CCNRM( d->character,C_SPR ) );    
        }

	if (FIGHTING(d->character) && 
	    PRF2_FLAGGED(d->character, PRF2_AUTO_DIAGNOSE)) 
	    sprintf(prompt, "%s%s(%s)%s ", prompt, CCRED(d->character, C_NRM),
		    diag_conditions(FIGHTING(d->character)),
		    CCNRM(d->character, C_NRM));
    
	sprintf(prompt, "%s%s%s>%s ", prompt, CCWHT(d->character, C_NRM),
		CCBLD(d->character, C_CMP), CCNRM(d->character, C_NRM));
	write_to_descriptor(d->descriptor, prompt);
	d->output_broken = FALSE;
    }
}


void 
write_to_q(char *txt, struct txt_q * queue, int aliased)
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
get_from_q(struct txt_q * queue, char *dest, int *aliased)
{
    struct txt_block *tmp;

    /* queue empty? */
    if (!queue->head)
	return 0;

    tmp = queue->head;
    strcpy(dest, queue->head->text);
    *aliased = queue->head->aliased;
    queue->head = queue->head->next;
    /*#ifdef DMALLOC
      dmalloc_verify(0);
      #endif */
    free(tmp->text);
    free(tmp);
    /*#ifdef DMALLOC
      dmalloc_verify(0);
      #endif */
    return 1;
}



/* Empty the queues before closing connection */
void 
flush_queues(struct descriptor_data * d)
{
    int dummy;

    if (d->large_outbuf) {
	d->large_outbuf->next = bufpool;
	bufpool = d->large_outbuf;
    }
    while (get_from_q(&d->input, buf2, &dummy));
}


void 
write_to_output(const char *txt, struct descriptor_data * t)
{
    int size;

    size = strlen(txt);

    /* if we're in the overflow state already, ignore this new output */
    if (t->bufptr < 0)
	return;

    /* if we have enough space, just write to buffer and that's it! */
    if (t->bufspace >= size) {
	strcpy(t->output + t->bufptr, txt);
	t->bufspace -= size;
	t->bufptr += size;
    } else {			/* otherwise, try switching to a lrg buffer */
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
	} else {			/* else create a new one */
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
    int desc, sockets_connected = 0;
    unsigned long addr;
    unsigned int i;
    static int last_desc = 0;	/* last descriptor number */
    struct descriptor_data *newd;
    struct sockaddr_in peer;
    struct hostent *from;
    extern char *GREETINGS;

    /* accept the new connection */
    i = sizeof(peer);
    if ((desc = accept(s, (struct sockaddr *) & peer, &i)) < 0) {
	perror("accept");
	return -1;
    }
    /* keep it from blocking */
    nonblock(desc);

    /* make sure we have room for it */
    for (newd = descriptor_list; newd; newd = newd->next)
	sockets_connected++;

    if (sockets_connected >= avail_descs) {
	write_to_descriptor(desc, "Sorry, Tempus is full right now... try again later!  :-)\r\n");
	close(desc);
	return 0;
    }
    /* create a new descriptor */
    CREATE(newd, struct descriptor_data, 1);
    memset((char *) newd, 0, sizeof(struct descriptor_data));

    /* find the sitename */

    if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
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
    if (isbanned(newd->host, buf2) == BAN_ALL) {
	close(desc);
	sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
	mudlog(buf2, CMP, LVL_GOD, TRUE);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(newd);
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	return 0;
    }

    /* Log new connections - probably unnecessary, but you may want it */
    sprintf(buf2, "New connection from [%s]", newd->host);
    slog(buf2);

    /* initialize descriptor data */
    newd->descriptor = desc;
    newd->connected = CON_GET_NAME;
    newd->wait = 1;
    newd->output = newd->small_outbuf;
    newd->bufspace = SMALL_BUFSIZE - 1;
    newd->next = descriptor_list;
    newd->login_time = time(0);
    newd->editor_mode = 0;
    newd->editor_cur_lnum = 1;
    newd->editor_file = NULL;
    newd->idle = 0;
  
    if (++last_desc == 10000)
	last_desc = 1;
    newd->desc_num = last_desc;

    /* prepend to list */
    descriptor_list = newd;

    if (!mini_mud) {
	SEND_TO_Q("\033[H\033[J", newd);  
	SEND_TO_Q(GREETINGS, newd);
    } else
	SEND_TO_Q("(tempus testmud) name: ", newd);
    return 0;
}


int 
process_output(struct descriptor_data * t)
{
    static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
    static int result;

    /* we may need this \r\n for later -- see below */
    strcpy(i, "\r\n");

    /* now, append the 'real' output */
    strcpy(i + 2, t->output);

    /* if we're in the overflow state, notify the user */
    if (t->bufptr < 0)
	strcat(i, "**OVERFLOW**");

    /* add the extra CRLF if the person isn't in compact mode */
    if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
	strcat(i + 2, "\r\n");

    /*
     * now, send the output.  If this is an 'interruption', use the prepended
     * CRLF, otherwise send the straight output sans CRLF.
     */
    if (!t->prompt_mode && 
	(!t->character || !t->output_broken ||
	 PRF2_FLAGGED(t->character, PRF2_AUTOPROMPT)))
	/* && !t->connected) */
	result = write_to_descriptor(t->descriptor, i);
    else
	result = write_to_descriptor(t->descriptor, i + 2);
    t->output_broken = TRUE;

    /* handle snooping: prepend "% " and send to snooper */
    if (t->snoop_by && t->snoop_by->character) {
	SEND_TO_Q(CCRED(t->snoop_by->character, C_NRM), t->snoop_by);
	SEND_TO_Q("% ", t->snoop_by);
	SEND_TO_Q(CCNRM(t->snoop_by->character, C_NRM), t->snoop_by);
	SEND_TO_Q(t->output, t->snoop_by);
	SEND_TO_Q(CCRED(t->snoop_by->character, C_NRM), t->snoop_by);
	SEND_TO_Q("%%", t->snoop_by);
	SEND_TO_Q(CCNRM(t->snoop_by->character, C_NRM), t->snoop_by);
    }
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (t->large_outbuf) {
	t->large_outbuf->next = bufpool;
	bufpool = t->large_outbuf;
	t->large_outbuf = NULL;
	t->output = t->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    t->bufspace = SMALL_BUFSIZE - 1;
    t->bufptr = 0;
    *(t->output) = '\0';

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
process_input(struct descriptor_data * t)
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
		// no route to host, probably a spoofed ip
//		if ( errno == EHOSTUNREACH ) {
//		    check_autoban( t->host );
//		}
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
	
	/*
	 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
	 * causing the MUD to hang when it encounters input not terminated by a
	 * newline.  This was causing hangs at the Password: prompt, for example.
	 * I attempt to compensate by always returning after the _first_ read, instead
	 * of looping forever until a read returns -1.  This simulates non-blocking
	 * I/O because the result is we never call read unless we know from select()
	 * that data is ready (process_input is only called if select indicates that
	 * this descriptor is in the read set).  JE 2/23/95.
	 */
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
		if ((*(write_point++) = *ptr) == '$') {	/* copy one character */
		    *(write_point++) = '$';	/* if it's a $, double it */
		    space_left -= 2;
		} else
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
	if (t->snoop_by) {
	    SEND_TO_Q(CCRED(t->snoop_by->character, C_NRM), t->snoop_by);
	    SEND_TO_Q("% ", t->snoop_by);
	    SEND_TO_Q(CCNRM(t->snoop_by->character, C_NRM), t->snoop_by);
	    SEND_TO_Q(tmp, t->snoop_by);
	    SEND_TO_Q("\r\n", t->snoop_by);
	}

	failed_subst = 0;

	if (*tmp == '!' && STATE(t) != CON_CNFPASSWD) {
	    strcpy(tmp, t->last_input);
	} else if (*tmp == '^') {
	    if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
		strcpy(t->last_input, tmp);
	} else
	    strcpy(t->last_input, tmp);

	if (t->repeat_cmd_count > 300 && 
	    (!t->character || GET_LEVEL(t->character) < LVL_ETERNAL)) {
	    if (t->character && t->character->in_room) {
		act("SAY NO TO SPAM.\r\n"
		    "Begone oh you waster of electrons,"
		    " ye vile profaner of CPU time!", TRUE, t->character,0,0,TO_ROOM);
		slog("SPAM-death on the queue!");
		return (-1);	
	    }
	}

	if (!failed_subst)
	    write_to_q(tmp, &t->input, 0);

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
perform_subst(struct descriptor_data * t, char *orig, char *subst)
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
	strncat(new_str, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new_str) - 1));

/* terminate the string in case of an overflow from strncat */
    new_str[MAX_INPUT_LENGTH-1] = '\0';
    strcpy(subst, new_str);

    return 0;
}



void 
close_socket(struct descriptor_data * d)
{
    struct descriptor_data *temp;
    char buf[128];
    long target_idnum = -1;

    close(d->descriptor);
    flush_queues(d);

    /* Forget snooping */
    if (d->snooping)
	d->snooping->snoop_by = NULL;

    if (d->snoop_by) {
	SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
	d->snoop_by->snooping = NULL;
    }
    if (d->character) {
	target_idnum = GET_IDNUM(d->character);
	if (d->connected == CON_PLAYING || d->connected > CON_NET_MENU1) {
	    save_char(d->character, NULL);
	    act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
	    sprintf(buf, "Closing link to: %s. [%s] ", GET_NAME(d->character),d->host);
	    mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(d->character)), TRUE);
	    d->character->desc = NULL;
	    GET_OLC_OBJ(d->character) = NULL;
	} else {
	    sprintf(buf, "Losing player: %s. [%s]",
		    GET_NAME(d->character) ? GET_NAME(d->character) : "<null>",
            d->host);
	    mudlog(buf, CMP, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(d->character)), TRUE);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free_char(d->character);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif    
	}
    } else
	mudlog("Losing descriptor without char.", CMP, LVL_AMBASSADOR, TRUE);
    /* JE 2/22/95 -- part of my enending quest to make switch stable */
    if (d->original && d->original->desc)
	d->original->desc = NULL;

    REMOVE_FROM_LIST(d, descriptor_list, next);

#ifdef DMALLOC
    dmalloc_verify(0);
#endif  
    if (d->showstr_head) 
	free(d->showstr_head);

    free(d);
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    /*
      if (target_idnum >= 0) {
      for (temp = descriptor_list; temp; temp = next_d) {
      next_d = temp->next;
      if (temp->character && (GET_IDNUM(temp->character) == target_idnum)
      && (temp->connected != CON_PLAYING))
      close_socket(temp);
      }
      }
    */
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
checkpointing( int sig = 0 )
{
    if (!tics) {
	slog("SYSERR: CHECKPOINT shutdown: tics not updated");
	sprintf(buf, "Last command: %s %s.", get_name_by_id(last_cmd[0].idnum),
		last_cmd[0].string);
	slog(buf);
	raise(SIGSEGV);
    } else
	tics = 0;
}


void 
reread_wizlists( int sig = 0 )
{
    void reboot_wizlists(void);

    mudlog("Signal received - rereading wizlists.", NRM, LVL_AMBASSADOR, TRUE);
    reboot_wizlists();
}


void 
unrestrict_game( int sig = 0 )
{
    extern struct ban_list_element *ban_list;
    extern int num_invalid;

    mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	   BRF, LVL_AMBASSADOR, TRUE);
    ban_list = NULL;
    restrict = 0;
    num_invalid = 0;
}


void 
hupsig( int sig = 0 )
{
    slog("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
    
    mudlog("Received external signal - shutting down for reboot in 60 sec..", BRF, LVL_AMBASSADOR, TRUE);
    
    send_to_all("\007\007:: Tempus REBOOT in 60 seconds ::\r\n");
    shutdown_idnum = -1;
    shutdown_count = 60;
    shutdown_mode = SHUTDOWN_REBOOT;

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

#if defined(NeXT) || defined(sun386)
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

    if (sigaction(signo, &act, &oact) < 0)
	return SIG_ERR;

    return oact.sa_handler;
}
#endif /* NeXT */

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
send_to_char(const char *messg, struct char_data * ch)
{
    if (ch->desc && messg)
	SEND_TO_Q(messg, ch->desc);
}


void 
send_to_all(char *messg)
{
    struct descriptor_data *i;

    if (messg)
	for (i = descriptor_list; i; i = i->next)
	    if (!i->connected)
		SEND_TO_Q(messg, i);
}

void
send_to_clerics(char *messg)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
	return;

    for (i = descriptor_list; i; i = i->next) {
	if ( !i->connected && i->character && AWAKE(i->character) &&
	     !PLR_FLAGGED(i->character, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
	     PRIME_MATERIAL_ROOM(i->character->in_room) && IS_CLERIC(i->character) ) {
	    SEND_TO_Q(messg, i);
	}
    }
}    

void 
send_to_outdoor(char *messg,int isecho=0)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
	return;

    for (i = descriptor_list; i; i = i->next)
	if (!i->connected && i->character && AWAKE(i->character) &&
		( !isecho || !PRF2_FLAGGED(i->character, PRF2_NOGECHO)) &&
	    !PLR_FLAGGED(i->character, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
	    OUTSIDE(i->character) && PRIME_MATERIAL_ROOM(i->character->in_room))
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
	if (!i->connected && i->character && 
	    PRF2_FLAGGED(i->character, PRF2_NEWBIE_HELPER) &&
	    GET_LEVEL(i->character) > level_can_shout &&
	    !(PRF_FLAGGED(i->character, PRF_LOG1) ||
	      PRF_FLAGGED(i->character, PRF_LOG2))) {
	    SEND_TO_Q(CCBLD(i->character, C_CMP), i);
	    SEND_TO_Q(CCYEL(i->character, C_SPR), i);
	    SEND_TO_Q(messg, i);
	    SEND_TO_Q(CCNRM(i->character, C_SPR), i);
	}
}
/* mode == TRUE -> hide from sender.  FALSE -> show to all */
void
send_to_comm_channel(struct char_data *ch, char *buf, int chan, int mode, int hide_invis)
{

    struct obj_data *obj = NULL;

    for (obj = object_list; obj; obj = obj->next) {
	if (IS_COMMUNICATOR(obj) && COMM_CHANNEL(obj) == chan &&
	    !obj->in_obj && ENGINE_STATE(obj)) {
	    if (obj->carried_by && (!mode || obj->carried_by != ch) &&
		(!hide_invis || CAN_SEE(obj->carried_by, ch))) {
		if (COMM_UNIT_SEND_OK(ch, obj->carried_by)) {
		    sprintf(buf2, "%s::%s::%s ", CCYEL(obj->carried_by, C_NRM),
			    OBJS(obj, obj->carried_by), CCNRM(obj->carried_by, C_NRM));
		    send_to_char(buf2, obj->carried_by);
		    act(buf, TRUE, ch, obj, obj->carried_by, TO_VICT);
		}
	    }
	    else if (obj->worn_by && (!mode || obj->worn_by != ch) &&
		     (!hide_invis || CAN_SEE(obj->worn_by, ch))) {
		if (COMM_UNIT_SEND_OK(ch, obj->worn_by)) {
		    sprintf(buf2, "%s::%s::%s ", CCYEL(obj->worn_by, C_NRM),
			    OBJS(obj, obj->worn_by), CCNRM(obj->worn_by, C_NRM));
		    send_to_char(buf2, obj->worn_by);
		    act(buf, TRUE, ch, obj, obj->worn_by, TO_VICT);
		}
	    }
	    else if (obj->in_room) {
		act("$p makes some noises.", FALSE, 0, obj, 0, TO_ROOM);
	    }
	}
    }
}

void 
send_to_zone(char *messg, struct zone_data *zn, int outdoor)
{
    struct descriptor_data *i;

    if (!messg || !*messg)
	return;

    for (i = descriptor_list; i; i = i->next)
	if (!i->connected && i->character && AWAKE(i->character) &&
	    !PLR_FLAGGED(i->character, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
	    i->character->in_room->zone == zn &&
	    (!outdoor ||
	     (OUTSIDE(i->character) && 
	      PRIME_MATERIAL_ROOM(i->character->in_room))))
	    SEND_TO_Q(messg, i);
}

void 
send_to_room(char *messg, struct room_data *room)
{
    struct char_data *i;
    struct obj_data *o, *obj = NULL;
    char buf[MAX_STRING_LENGTH];
    int j;

    if (!room || !messg)
	return;

    for (i = room->people; i; i = i->next_in_room)
	if (i->desc && !PLR_FLAGGED(i, PLR_OLC | PLR_WRITING | PLR_MAILING))
	    SEND_TO_Q(messg, i->desc);

    /** check for vehicles in the room **/
    sprintf(buf, "(outside) %s", messg);
    for (o = room->contents; o; o = o->next_content) {
	if (IS_OBJ_TYPE(o, ITEM_VEHICLE)) {
	    for (obj = object_list; obj; obj = obj->next) {
		if (((IS_OBJ_TYPE(obj, ITEM_V_DOOR) && !CAR_CLOSED(o)) ||
		     (IS_V_WINDOW(obj) && !CAR_CLOSED(obj))) &&
		    ROOM_NUMBER(obj) == ROOM_NUMBER(o) &&
		    GET_OBJ_VNUM(o) == V_CAR_VNUM(obj) &&
		    obj->in_room) {
		    for (i = obj->in_room->people; i; i = i->next_in_room)
			if (i->desc && !PLR_FLAGGED(i, PLR_OLC|PLR_WRITING|PLR_MAILING))
			    SEND_TO_Q(buf, i->desc);
		}
	    }
	}
    }

    /* see if there is a podium in the room */
    sprintf(buf, "(remote) %s", messg);  
    for (o = room->contents; o; o = o->next_content)
	if (GET_OBJ_TYPE(o) == ITEM_PODIUM)
	    break;
  
    if (!o)
	return;
  
    for (j=0;j<NUM_OF_DIRS;j++)
	if (ABS_EXIT(room,j) && ABS_EXIT(room, j)->to_room &&
	    room != ABS_EXIT(room, j)->to_room &&
	    !IS_SET(ABS_EXIT(room,j)->exit_info, EX_ISDOOR | EX_CLOSED)) {

	    for (i = ABS_EXIT(room,j)->to_room->people; i; i = i->next_in_room)
		if (i->desc && !PLR_FLAGGED(i, PLR_OLC))
		    SEND_TO_Q(buf, i->desc);
	}
}

void 
send_to_clan(char *messg, int clan)
{
    struct descriptor_data *i;

    if (messg)
	for (i = descriptor_list; i; i = i->next)
	    if (!i->connected && i->character && (GET_CLAN(i->character) == clan) &&
		!PLR_FLAGGED(i->character, PLR_OLC)) {
		SEND_TO_Q(CCCYN(i->character, C_NRM), i);
		SEND_TO_Q(messg, i);
		SEND_TO_Q(CCNRM(i->character, C_NRM), i);
	    }
}



char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void 
perform_act(const char *orig, struct char_data * ch, struct obj_data * obj,
	    void *vict_obj, struct char_data * to, int mode)
{
    register const char *i=0;
    register const char *s=orig;
    register char *buf;
    static char lbuf[MAX_STRING_LENGTH];
    char outbuf[MAX_STRING_LENGTH];
  
    if (!to->in_room) {
	slog("SYSERR: to->in_room NULL in perform_act.");
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
		CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
		break;
	    case 'm':
		i = HMHR(ch);
		break;
	    case 'M':
		CHECK_NULL(vict_obj, HMHR((struct char_data *) vict_obj));
		break;
	    case 's':
		i = HSHR(ch);
		break;
	    case 'S':
		CHECK_NULL(vict_obj, HSHR((struct char_data *) vict_obj));
		break;
	    case 'e':
		i = HSSH(ch);
		break;
	    case 'E':
		CHECK_NULL(vict_obj, HSSH((struct char_data *) vict_obj));
		break;
	    case 'o':
		CHECK_NULL(obj, OBJN(obj, to));
		break;
	    case 'O':
		CHECK_NULL(vict_obj, OBJN((struct obj_data *) vict_obj, to));
		break;
	    case 'p':
		CHECK_NULL(obj, OBJS(obj, to));
		break;
	    case 'P':
		CHECK_NULL(vict_obj, OBJS((struct obj_data *) vict_obj, to));
		break;
	    case 'a':
		CHECK_NULL(obj, SANA(obj));
		break;
	    case 'A':
		CHECK_NULL(vict_obj, SANA((struct obj_data *) vict_obj));
		break;
	    case 'T':
		CHECK_NULL(vict_obj, (char *) vict_obj);
		break;
	    case 'F':
		CHECK_NULL(vict_obj, fname((char *) vict_obj));
		break;
	    case '$':
		i = "$";
		break;
	    default:
		slog("SYSERR: Illegal $-code to act():");
		strcpy(buf1, "SYSERR: ");
		strcat(buf1, s);
		slog(buf1);
		i = "---";
		break;
	    }
	    while ((*buf = *(i++)))
		buf++;
	    s++;
	} else if (!(*(buf++) = *(s++)))
	    break;
    }

    *(--buf) = '\r';
    *(++buf) = '\n';
    *(++buf) = '\0';

    if (mode == 1)
	sprintf(outbuf, "(outside) %s", CAP(lbuf));
    else if (mode == 2)
	sprintf(outbuf, "(remote) %s", CAP(lbuf));
    else
	strcpy(outbuf, CAP(lbuf));

    SEND_TO_Q(outbuf, to->desc);
}


#define SENDOK(ch) ((ch)->desc && (AWAKE(ch) || sleep) &&           \
		    !PLR_FLAGGED((ch), PLR_WRITING | PLR_OLC) &&    \
		    (!ch->desc->showstr_point ||                 \
		     PRF2_FLAGGED(ch, PRF2_LIGHT_READ)))

void 
act(const char *str, int hide_invisible, struct char_data * ch,
    struct obj_data * obj, void *vict_obj, int type)
{
    struct char_data *to;
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

    /* check if TO_SLEEP is there, and remove it if it is. */
    if ((sleep = (type & TO_SLEEP)))
	type &= ~TO_SLEEP;

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
	if ((to = (struct char_data *) vict_obj) && SENDOK(to))
	    perform_act(str, ch, obj, vict_obj, to, 0);
	return;
    }
    /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

    if (ch && ch->in_room != NULL)
	room = ch->in_room;
    else if (obj && obj->in_room != NULL) {
	room = obj->in_room;
    } else {
	slog("SYSERR: no valid target to act()!");
	return;
    }

    to = room->people;

    for (; to; to = to->next_in_room)
	if (SENDOK(to) && 
	    !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
	    (to != ch) && (type == TO_ROOM || (to != vict_obj)))
	    perform_act(str, ch, obj, vict_obj, to, 0);


    /** check for vehicles in the room **/
    for (o = room->contents; o; o = o->next_content) {
	if (IS_OBJ_TYPE(o, ITEM_VEHICLE) && (!hidecar || o != cur_car)) {
	    for (o2 = object_list; o2; o2 = o2->next) {
		if (((IS_OBJ_TYPE(o2, ITEM_V_DOOR) && !CAR_CLOSED(o)) ||
		     (IS_OBJ_TYPE(o2, ITEM_V_WINDOW) && !CAR_CLOSED(o2))) &&
		    ROOM_NUMBER(o2) == ROOM_NUMBER(o) &&
		    GET_OBJ_VNUM(o) == V_CAR_VNUM(o2) &&
		    o2->in_room) {
		    for (to = o2->in_room->people; to; to = to->next_in_room) {
			if (SENDOK(to) && 
			    !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
			    (to != ch) && (type == TO_ROOM || (to != vict_obj))) {
			    perform_act(str, ch, obj, vict_obj, to, 1);
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

    if (!o)
	return;

    for (j=0;j<NUM_OF_DIRS;j++) {
	if (ABS_EXIT(room,j) && ABS_EXIT(room, j)->to_room &&
	    room != ABS_EXIT(room, j)->to_room &&
	    !IS_SET(ABS_EXIT(room,j)->exit_info, EX_ISDOOR | EX_CLOSED)) {

	    for (to = ABS_EXIT(room, j)->to_room->people; to; to = to->next_in_room)
		if (SENDOK(to) && 
		    !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
		    (to != ch) && (type == TO_ROOM || (to != vict_obj)))
		    perform_act(str, ch, obj, vict_obj, to, 2);
	}
    }
}

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
	    act("$p starts spinning faster and faster, and disappears in a flash!",
		FALSE, 0, quad, 0, TO_ROOM);
	orig_room = quad->in_room;
	obj_from_room(quad);
    } else if (!(quad = read_object(QUAD_VNUM))) {
	slog("SYSERR:  Unable to load Quad Damage.");
	return;
    }

    if (orig_room)
	zone = orig_room->zone;
    else 
	zone = d_quad_zone;

    for (; zone; zone = zone->next) {
	if (zone->number >= 700)
	    zone = d_quad_zone;
	if (zone->world && zone->plane <= MAX_PRIME_PLANE && !number(0, 2))
	    break;
    }
  
    for (room = zone->world; room; room = room->next) {
	if (ROOM_FLAGGED(room, ROOM_PEACEFUL | ROOM_DEATH | ROOM_HOUSE |
			 ROOM_CLAN_HOUSE) ||
	    room == orig_room ||
	    number(0, 9))
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
	    FALSE,0,quad,0,TO_ROOM);

    /*  sprintf(buf, "Quad damage moved to room %d.", room->number);
	slog(buf); */

}
  

void
push_command_onto_list(int idnum, char *string)
{

    int i;

    for (i = NUM_SAVE_CMDS-2; i >= 0; i--) {
	last_cmd[i+1].idnum = last_cmd[i].idnum;
	strcpy(last_cmd[i+1].string, last_cmd[i].string);
    }

    last_cmd[0].idnum = idnum;
    strcpy(last_cmd[0].string, string);

}
    
void
descriptor_update(void)
{
    struct descriptor_data *d = NULL;
    
    for (d = descriptor_list; d; d = d->next) {

	// skip the folks that will get hit with point_update()
	if (d->character && !d->connected)
	    continue;
    
	d->idle++;

	if (d->idle >= 30 || (d->connected < CON_NET_MENU1 && d->idle >= 10)) {
	    mudlog("Descriptor idling out after 10 minutes.", CMP, LVL_IMMORT, TRUE);
	    SEND_TO_Q("Idle time limit reached, disconnecting.\r\n", d);
	    d->connected = CON_CLOSE;
	}
    }
}

