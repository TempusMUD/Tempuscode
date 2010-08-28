#ifndef _HELP_H_
#define _HELP_H_

#define HFLAG_UNAPPROVED (1 << 0)	// Help topic cannot be found by searching
#define HFLAG_IMMHELP    (1 << 1)	// Help topic can only be found by gods
#define HFLAG_MODIFIED   (1 << 2)	// Help topic has been changed or is being
								   //    edited now.
#define HGROUP_OLC       (1 << 0)
#define HGROUP_MISC      (1 << 1)
#define HGROUP_NEWBIE    (1 << 2)
#define HGROUP_SKILL     (1 << 3)
#define HGROUP_SPELL     (1 << 4)
#define HGROUP_CLASS     (1 << 5)
#define HGROUP_CITY      (1 << 6)
#define HGROUP_PLAYER    (1 << 7)
#define HGROUP_MAGE      (1 << 8)
#define HGROUP_CLERIC    (1 << 9)
#define HGROUP_THIEF     (1 << 10)
#define HGROUP_WARRIOR   (1 << 11)
#define HGROUP_BARB      (1 << 12)
#define HGROUP_PSIONIC   (1 << 13)
#define HGROUP_PHYSIC    (1 << 14)
#define HGROUP_CYBORG    (1 << 15)
#define HGROUP_KNIGHT    (1 << 16)
#define HGROUP_RANGER    (1 << 17)
#define HGROUP_BARD      (1 << 18)
#define HGROUP_MONK      (1 << 19)
#define HGROUP_MERCENARY (1 << 20)
#define HGROUP_HELP_EDIT (1 << 21)
#define HGROUP_HELP_GODS (1 << 22)
#define HGROUP_IMMHELP   (1 << 23)
#define HGROUP_QCONTROL  (1 << 24)
#define HGROUP_MAX        25

#define HELP_DIRECTORY "text/help_data/"
#define MAX_HELP_NAME_LENGTH 128
#define MAX_HELP_TEXT_LENGTH 16384

struct creature;

struct help_item {
	int idnum;					// Unique Identifier
	int counter;				// How many times has it been looked at
	// Groups and flags are both bitvectors.
	int groups;					//  The groups the item is in
	int flags;					// Approved/Unapproved etc.
	long owner;					// Last person to edit the topic
	char *keys;					// Key Words
	char *name;					// The listed name of the help topic
	char *text;					// The body of the help topic
	struct creature *editor;
	struct help_item *next;
	struct help_item *next_show;
};

struct help_collection {
	// Data
	struct help_item *items;			// The top of the list of help topics
	struct help_item *bottom;			// The bottom of the list of help topics

	// Returns a show list of items it found
	int top_id;					// The highest id in use..
	bool need_save;				// Weather something has been changed or not.
};

struct help_item *make_helpitem(void);
void free_helpitem(struct help_item *item);
void helpitem_show(struct help_item *item,
                   struct creature *ch,
                   char *buffer,
                   int mode);

#endif
