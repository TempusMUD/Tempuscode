#ifndef _COMM_H_
#define _COMM_H_

/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: comm.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define NUM_RESERVED_DESCS	8

/* comm.c */
void send_to_all(char *messg);
void send_to_char(struct Creature *ch, const char *str, ...)
	__attribute__ ((format (printf, 2, 3)));
void send_to_desc(struct descriptor_data *d, const char *str, ...)
	__attribute__ ((format (printf, 2, 3)));
void send_to_room(char *messg, struct room_data *room);
void send_to_clerics(int align, char *messg);
void send_to_outdoor(char *messg, int isecho = 0);
void send_to_clan(char *messg, int clan);
void send_to_zone(char *messg, struct zone_data *zone, int outdoor);
void send_to_comm_channel(struct Creature *ch, char *buff, int chan, int mode,
	int hide_invis);
void send_to_newbie_helpers(char *messg);
void close_socket(struct descriptor_data *d);

// Act system
typedef bool (*act_if_predicate)(struct Creature *ch, struct obj_data *obj, void *vict_obj, struct Creature *to, int mode);
char *act_escape(const char *str);
void perform_act(const char *orig, struct Creature *ch,
	struct obj_data *obj, void *vict_obj, struct Creature *to, int mode);
void act_if(const char *str, int hide_invisible, struct Creature *ch,
	struct obj_data *obj, void *vict_obj, int type, act_if_predicate pred);
void act(const char *str, int hide_invisible, struct Creature *ch,
	struct obj_data *obj, void *vict_obj, int type);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define ACT_HIDECAR     8
#define TO_VICT_RM  64 
#define TO_SLEEP	128			/* to char, even if sleeping */

#define SHUTDOWN_NONE   0
#define SHUTDOWN_DIE    1
#define SHUTDOWN_PAUSE  2
#define SHUTDOWN_REBOOT 3

class Account;

void write_to_q(char *txt, struct txt_q *queue, int aliased);
void write_to_output(const char *txt, struct descriptor_data *d);
void page_string(struct descriptor_data *d, const char *str);
void show_file(struct Creature *ch, char *fname, int lines);
void show_account_chars(descriptor_data *d, Account *acct, bool immort, bool brief);

extern bool suppress_output;

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

typedef void sigfunc(int);

struct last_command_data {
	int idnum;
	char name[MAX_INPUT_LENGTH];
	int roomnum;
	char room[MAX_INPUT_LENGTH];
	char string[MAX_INPUT_LENGTH];
};

#define NUM_SAVE_CMDS 30

#ifdef __COMM_C__

/* system calls not prototyped in OS headers */
#if     defined(_AIX)
#define POSIX_NONBLOCK_BROKEN
#include <sys/select.h>
int accept(int s, struct sockaddr *addr, int *addrlen);
int bind(int s, struct sockaddr *name, int namelen);
int getpeername(int s, struct sockaddr *name, int *namelen);
int getsockname(int s, struct sockaddr *name, int *namelen);
int gettimeofday(struct timeval *tp, struct timezone *tzp);
int listen(int s, int backlog);
int setsockopt(int s, int level, int optname, void *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#if defined(__hpux)
int accept(int s, void *addr, int *addrlen);
int bind(int s, const void *addr, int addrlen);
int getpeername(int s, void *addr, int *addrlen);
int getsockname(int s, void *name, int *addrlen);
int gettimeofday(struct timeval *tp, struct timezone *tzp);
int listen(int s, int backlog);
int setsockopt(int s, int level, int optname, const void *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#if     defined(interactive)
#include <net/errno.h>
#include <sys/fcntl.h>
#endif

#if     defined(MIPS_OS)
extern int errno;
#endif

#if     defined(NeXT)
int close(int fd);
int chdir(const char *path);
int fcntl(int fd, int cmd, int arg);
#if     !defined(htons)
u_short htons(u_short hostshort);
#endif
#if     !defined(ntohl)
u_long ntohl(u_long hostlong);
u_long htonl(u_long hostlong);
#endif
int read(int fd, char *buf, int nbyte);
int select(int width, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval *timeout);
int write(int fd, char *buf, int nbyte);
#endif

#if     defined(sequent)
int accept(int s, struct sockaddr *addr, int *addrlen);
int bind(int s, struct sockaddr *name, int namelen);
int close(int fd);
int fcntl(int fd, int cmd, int arg);
int getpeername(int s, struct sockaddr *name, int *namelen);
int getsockname(int s, struct sockaddr *name, int *namelen);
int gettimeofday(struct timeval *tp, struct timezone *tzp);
#if     !defined(htons)
u_short htons(u_short hostshort);
#endif
int listen(int s, int backlog);
#if     !defined(ntohl)
u_long ntohl(u_long hostlong);
#endif
int read(int fd, char *buf, int nbyte);
int select(int width, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval *timeout);
int setsockopt(int s, int level, int optname, caddr_t optval, int optlen);
int socket(int domain, int type, int protocol);
int write(int fd, char *buf, int nbyte);
#endif

/*
 * This includes Solaris SYSV as well
 */
#if defined(sun)
int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);
int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
int accept(int s, struct sockaddr *addr, int *addrlen);
int bind(int s, struct sockaddr *name, int namelen);
int close(int fd);
int getpeername(int s, struct sockaddr *name, int *namelen);
int getsockname(int s, struct sockaddr *name, int *namelen);
int gettimeofday(struct timeval *tp, struct timezone *tzp);
int listen(int s, int backlog);
int select(int width, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval *timeout);
int setsockopt(int s, int level, int optname, const char *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#if defined(ultrix)
void bzero(char *b1, int length);
int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
int accept(int s, struct sockaddr *addr, int *addrlen);
int bind(int s, struct sockaddr *name, int namelen);
int close(int fd);
int getpeername(int s, struct sockaddr *name, int *namelen);
int getsockname(int s, struct sockaddr *name, int *namelen);
int gettimeofday(struct timeval *tp, struct timezone *tzp);
int listen(int s, int backlog);
int select(int width, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval *timeout);
int setsockopt(int s, int level, int optname, void *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#endif							/* #ifdef __COMM_C__ */
#endif
