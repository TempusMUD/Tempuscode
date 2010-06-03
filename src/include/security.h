#ifndef _SECURITY_H_
#define _SECURITY_H_

// XML Includes
#include <libxml/parser.h>
#include <libxml/tree.h>
// Undefine CHAR to avoid collisions
#undef CHAR
#include "xml_utils.h"
#include "screen.h"

// Interpreter command structure
extern struct command_info cmd_info[];
// Forward references to various clients.
struct command_info;
struct show_struct;
struct set_struct;

/*
 * A namespace for isolating the access security functionality of
 * the interpreter.
 *
**/
/** The name of the file to store security data in. **/
static const bool SECURITY_TRACE = false;
static inline void security_trace( const char *msg, const char *name) {
    if( SECURITY_TRACE )
        fprintf( stderr,"TRACE: %s : %s\r\n", msg, name );
}

// Special groups
extern const char *SECURITY_EVERYONE;
extern const char *SECURITY_NOONE;

extern const char *SECURITY_ADMINBASIC;
extern const char *SECURITY_ADMINFULL;
extern const char *SECURITY_CLAN;
extern const char *SECURITY_CLANADMIN;
extern const char *SECURITY_CODER;
extern const char *SECURITY_CODERADMIN;
extern const char *SECURITY_DYNEDIT;
extern const char *SECURITY_GROUPSADMIN;
extern const char *SECURITY_HELP;
extern const char *SECURITY_HOUSE;
extern const char *SECURITY_OLC;
extern const char *SECURITY_OLCADMIN;
extern const char *SECURITY_OLCAPPROVAL;
extern const char *SECURITY_OLCPROOFER;
extern const char *SECURITY_OLCWORLDWRITE;
extern const char *SECURITY_QUESTOR;
extern const char *SECURITY_QUESTORADMIN;
extern const char *SECURITY_TESTERS;
extern const char *SECURITY_WIZARDADMIN;
extern const char *SECURITY_WIZARDBASIC;
extern const char *SECURITY_WIZARDFULL;
extern const char *SECURITY_WORLDADMIN;

/**  Constant Command Flags **/
const int SECURITY_GROUP = (1 << 0);

struct group {
        /* A one line description of this group */
    char *description;
    /* The one word name for this group */
    char *name;
    /* The name of the group whose members can administrate this group */
    char *adminGroup;

    int id;
    /**
     * resolved on group load/creation.
     * Command names stored in file.
     * Pointers sorted as if they're ints.
     **/
    struct command_info * commands;
    // player ids, sorted
    struct int_list *members;
};
/* The list of existing groups (group.cc) */
struct group groups;

/**
 * Returns true if the character is the proper level AND is in
 * one of the required groups (if any)
 **/
bool group_canAccessCmd(struct group *group, struct creature *ch, struct command_info *command );
/**
 * Returns true if the character is the proper level AND is in
 * one of the required groups (if any)
 **/
bool group_canAccessShow(struct group *group, struct creature *ch, const struct show_struct *command );
/**
 * Returns true if the character is the proper level AND is in
 * one of the required groups (if any)
 **/
bool group_canAccessSet(struct group *group, struct creature *ch, const struct set_struct *command );
/* Check membership in a particular group by name.**/
bool group_isMember(struct group *group, struct creature *ch, const char* group_name, bool substitute );

/* can this character add/remove characters from this group. **/
bool group_canAdminGroup(struct group *group, struct creature *ch, const char* groupName );

/* send a list of the current groups to a character */
void sendGroupList(struct group *group, struct creature *ch );
/**
 * creates a new group with the given name.
 * returns false if the name is taken.
 **/
bool group_createGroup(struct group *group, char *name );
/**
 * removes the group with the given name.
 * returns false if there is no such group.
 **/
bool group_removeGroup(struct group *group, char *name );

/** returns true if the named group exists. **/
bool group_isGroup(struct group *group, const char* name );
/** retrieves the named group. **/
struct group *getGroup(struct group *group, const char* name);
/** sends the member list of the named group to the given character. **/
bool group_sendMemberList(struct group *group, struct creature *ch, char *group_name );
/** sends the command list of the named group to the given character. **/
bool group_sendCommandList(struct group *group, struct creature *ch, char *group_name );
/** sends a list of the groups that the id is a member if. **/
bool group_sendMembership(struct group *group, struct creature *ch, long id );
/*
 * sends a list of the commands a char has access to and the
 * groups that contain them.
 **/
bool group_sendAvailableCommands(struct group *group, struct creature *ch, long id );
/**
 * adds the named command to the named group.
 * returns false if either doesn't exist.
 **/
bool group_addCommand(struct group *group, char *command, char *group_name );
/**
 * adds the named character to the named group.
 * returns false if either doesn't exist.
 **/
bool group_addMember(struct group *group, const char *member, const char *group_name );

/**
 * removes the named command from the named group.
 * returns false if either doesn't exist.
 **/
bool group_removeCommand(struct group *group, char *command, char *group_name );
/**
 * removes the named character from the named group.
 * returns false if either doesn't exist.
 **/
bool group_removeMember(struct group *group, const char *member, const char *group_name );

/**
 * clears the current security groups and loads new groups and
 * membership from the database
 **/
bool group_loadGroups(void);

/**
 *
 * clears security related memory and shuts the system down.
 */
void security_shutdown();

void log_by_name(struct group *group, const char* msg, const char* name );
void log_by_id(struct group *group, const char* msg, const long id );

/*  <Security>
 *    <Group  Name="test"  Description="Testing Group" >
 *      <Member ID="1"/>
 *      <Command Name="look"/>
 *    </Group>
 *  </Security>
 */

#endif
