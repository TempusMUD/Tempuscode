#ifndef _BAN_H_
#define _BAN_H_

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
	char site[BANNED_SITE_LENGTH + 1];
	int type;
	time_t date;
	char name[MAX_NAME_LENGTH + 1];
	struct ban_list_element *next;
};


#endif
