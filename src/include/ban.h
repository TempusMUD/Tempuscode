#ifndef _BAN_H_
#define _BAN_H_

#include <string.h>

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
#define BANNED_REASON_LENGTH  80

typedef char namestring[MAX_NAME_LENGTH];

class ban_entry {
 public:
    ban_entry(void) {
        _site[0] = _name[0] = _reason[0] = '\0';
        _type = _date = 0;
    }
    ban_entry(const ban_entry &o) {
        strcpy(_site, o._site);
        strcpy(_name, o._name);
        strcpy(_reason, o._reason);
        _type = o._type;
        _date = o._date;
    }
    ban_entry(const char *site,
              int type,
              time_t date,
              const char *name,
              const char *reason) {
        strcpy(_site, site);
        strcpy(_name, name);
        strcpy(_reason, reason);
        _type = type;
        _date = date;
    }

	char _site[BANNED_SITE_LENGTH + 1];
	int _type;
	time_t _date;
	char _name[MAX_NAME_LENGTH + 1];
	char _reason[BANNED_REASON_LENGTH + 1];
};

extern namestring *nasty_list;
extern int num_nasty;

int isbanned(char *hostname, char *blocking_hostname);
void perform_ban(int flag,
                 const char *site,
                 const char *name,
                 const char *reason);

int Valid_Name(char *newname);

#endif
