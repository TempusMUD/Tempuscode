#ifndef _CLAN_H_
#define _CLAN_H_

//
// File: clan.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define CLAN_NONE   0

#define MAX_CLAN_NAME      24
#define MAX_CLAN_BADGE     16
#define MAX_CLAN_PASSWORD  16
#define MAX_CLAN_MEMBERS  200
#define MAX_CLAN_ROOMS    200
#define MAX_CLAN_RANKNAME  48
#define NUM_CLAN_RANKS     11
#define LVL_CAN_CLAN       10

struct clan_data;
struct room_data;
struct room_list_elem;

bool boot_clans(void);
/*@keep@*/ struct clan_data *clan_by_owner(int idnum);
/*@keep@*/ struct clan_data *real_clan(int vnum);
/*@keep@*/ struct clan_data *clan_by_name(char *arg)
__attribute__ ((nonnull));
int clan_owning_room(struct room_data *room)
__attribute__ ((nonnull));
bool clan_house_can_enter(struct creature *ch, struct room_data *room)
__attribute__ ((nonnull));
void do_show_clan(struct creature *ch, struct clan_data *clan)
__attribute__ ((nonnull (1)));
bool save_clans(void);
/*@only@*/ struct clan_data *create_clan(int vnum);
int delete_clan(/*@only@*/ struct clan_data *clan)
__attribute__ ((nonnull));
/*@keep@*/ struct clanmember_data *real_clanmember(long idnum, struct clan_data *clan)
__attribute__ ((nonnull));
void sort_clanmembers(struct clan_data *clan)
__attribute__ ((nonnull));
void remove_member_from_clan(/*@only@*/ struct clanmember_data *member,
                                        struct clan_data *clan)
__attribute__ ((nonnull));
void remove_room_from_clan(/*@only@*/ struct room_list_elem *rm_list,
                                      struct clan_data *clan)
__attribute__ ((nonnull));
void remove_clan_member(struct creature *ch)
__attribute__ ((nonnull));
void clear_clan_owner(long idnum);
void remove_char_clan(int clan_idnum, long ch_idnum);
int clan_id(struct clan_data *clan)
__attribute__ ((nonnull));

struct clanmember_data {
    long idnum;
    int8_t rank;
    bool no_mail;
    /*@owned@*/ struct clanmember_data *next;
};

struct room_list_elem {
    /*@dependent@*/ struct room_data *room;
    /*@owned@*/ struct room_list_elem *next;
};

struct clan_data {
    int number;
    money_t bank_account;
    int8_t top_rank;
    long owner;
    char *name;                 /* official clan name */
    char *badge;                /* title of clan for who list, etc. */
    char *password;             /* password of clan */
    /*@owned@*/ char *ranknames[NUM_CLAN_RANKS];
    /*@owned@*/ struct clanmember_data *member_list;    /* list of idnums */
    /*@owned@*/ struct room_list_elem *room_list;   /* list of clan house rooms */
    /*@owned@*/ struct clan_data *next;
};

ACMD(do_enroll);
ACMD(do_join);
ACMD(do_dismiss);
ACMD(do_resign);
ACMD(do_promote);
ACMD(do_demote);
ACMD(do_clanmail);
ACMD(do_clanlist);
ACMD(do_cinfo);
ACMD(do_clanpasswd);
ACMD(do_cedit);

#endif
