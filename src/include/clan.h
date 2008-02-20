#ifndef _CLAN_H_
#define _CLAN_H_

//
// File: clan.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define CLAN_NONE	0

#define MAX_CLAN_NAME      24
#define MAX_CLAN_BADGE     16
#define MAX_CLAN_MEMBERS  200
#define MAX_CLAN_ROOMS    200
#define MAX_CLAN_RANKNAME  48
#define NUM_CLAN_RANKS     11
#define LVL_CAN_CLAN       10

struct clan_data *real_clan(int vnum);
struct clan_data *clan_by_name(char *arg);
int clan_house_can_enter(struct Creature *ch, struct room_data *room);
void do_show_clan(struct Creature *ch, struct clan_data *clan);
bool save_clans();
struct clan_data *create_clan(int vnum);
int delete_clan(struct clan_data *clan);
struct clanmember_data *real_clanmember(long idnum, struct clan_data *clan);
void sort_clanmembers(struct clan_data *clan);
void REMOVE_MEMBER_FROM_CLAN(struct clanmember_data *member,
	struct clan_data *clan);

struct clanmember_data {
	long idnum;
	byte rank;
	struct clanmember_data *next;
};

struct room_list_elem {
	struct room_data *room;
	struct room_list_elem *next;
};

struct clan_data {
	int number;
	long long int bank_account;
	byte top_rank;
	long owner;
	char *name;					/* official clan name */
	char *badge;				/* title of clan for who list, etc. */
	char *ranknames[NUM_CLAN_RANKS];
	struct clanmember_data *member_list;	/* list of idnums */
	struct room_list_elem *room_list;	/* list of clan house rooms */
	struct clan_data *next;
};

extern struct clan_data *clan_list;
#endif
