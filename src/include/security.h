#ifndef TEMPUS_SECURITY_H
#define TEMPUS_SECURITY_H

#include <vector>
#include <algorithm>
using namespace std;

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
struct board_info_type;
struct command_info;
struct show_struct;
struct set_struct;

/*
 * A namespace for isolating the access security functionality of
 * the interpreter.
 *
**/
namespace Security {
    /** The name of the file to store security data in. **/
    static const char SECURITY_FILE[] = "etc/security.dat";
    static const bool TRACE = false;
    static inline void trace( const char *msg, const char *name = "system" ) {
        if( TRACE )
            fprintf( stderr,"TRACE: %s : %s\r\n", msg, name );
    }

    /*
     * Preliminary group list:
     *   Questor
     *   Builder
     *   Proofer
     *   Approver
     *   Lower Admin ( mute, maybe freeze, etc)
     *   Upper Admin ( purge, dc, ban, set file. )
     *   Coder
     *   Security ( add/alter security groups )
     *
     * Considerations:
     *   Using group membership to check access for zone writes
     *     for non-owners. (Such as for architects etc.)
     */


    /**  Constant Command Flags **/
    const int GROUP = (1 << 0);

    class Group {
        public:
            /* does not make a copy of name or desc.  */
            Group( char *name, char *description );
            /* Makes a copy of name */
            Group( const char *name );
            /* Makes a complete copy of the Group */
            Group( const Group &g );
            /* Loads a group from it's xmlnode */
            Group( xmlNodePtr node );
            
            /* Adds a command to this group. Fails if already added. */
            bool addCommand( command_info *command );
            int getCommandCount() { return commands.size(); }
            /* Adds a member to this group by name. Fails if already added. */
            bool addMember( const char *name );
            /* Adds a member to this group by player id. Fails if already added. */
            bool addMember( long player );
            
            /* Removes a command from this group. Fails if not a member. */
            bool removeCommand( command_info *command );
            /* Removes a member from this group by player name. Fails if not a member. */ 
            bool removeMember( const char *name );
            /* Removes a member from this group by player id. Fails if not a member. */
            bool removeMember( long player );

            /* membership check for players by player id */
            bool member( long player );
            /* membership check for a given player */
            bool member( Creature *ch );
            /* membership check for a given command */
            bool member( const command_info *command );
            
            /* membership check for players by player id */
            bool givesAccess( Creature *ch, const command_info *command );
            
            /* Retrieves this group's description */
            const char *getDescription() { return _description; }
            /* sets this group's description */
            void setDescription(const char *desc);
            /* Retrieves this group's name */
            const char *getName() { return _name; }
            /* retrieves the name of the group that can admin this group. */
            const char *getAdminGroup() { return _adminGroup; }
            /* sets the name of the group that can admin this group. */
            void setAdminGroup(const char *group);
            
            /* assignment operator. Used by copy constructor and sorting. */
            Group &operator=( const Group &g );
            /* 
             * Equivilance of this groups name with the given name.
             * Used for sorting and searching.
             */
            bool operator==( const char *name ) 
                { return ( strcasecmp(_name,name) == 0 ); }
            /* 
             * Inequivilance of this groups name with the given name.
             * Used for sorting and searching.
             */
            bool operator!=( const char *name ) 
                { return !(*this == name); }
            /* 
             * Less than as defined by strcasecmp() on this._name vs g._name.
             * Used for sorting and searching.
             */
            bool operator<( const Group& g )  
                { return ( strcasecmp(_name, g._name) < 0 ); }
            /* 
             * Greater than as defined by strcasecmp() on this._name vs g._name.
             * Used for sorting and searching.
             */
            bool operator>( const Group& g )  
                { return ( strcasecmp(_name, g._name) > 0 ); }

            /* 
			 * Sends a list of this group's members to the given character. 
			 * without player id's etc.
			 */
            bool sendPublicMemberList( Creature *ch, char *str );
            /* Sends a list of this group's members to the given character. */
            bool sendMemberList( Creature *ch );
            /* Sends a list of this group's members to the given character. */
            bool sendCommandList( Creature *ch, bool prefix = true );
            
            /* Create the required xmlnodes to recreate this group; */
            bool save( xmlNodePtr parent );
            
            /* sends a multi-line status of this group to ch */
            void sendStatus( Creature *ch ); 
            /* sprintf's a one line desc of this group into out */
            void sendString(Creature *ch );

            /* Clear out this group's data for shutdown. */
            void clear();

            ~Group();
        private:
            /* A one line description of this group */
            char *_description;
            /* The one word name for this group */
            char *_name;
            /* The name of the group whos members can administrate this group */
            char *_adminGroup;
            /** 
             * resolved on group load/creation. 
             * Command names stored in file.
             * Pointers sorted as if they're ints.
             **/
             vector<command_info *> commands;
             // player ids, sorted
             vector<long> members;
    };
    /* The list of existing groups (group.cc) */
    extern list<Group> groups;
    
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
    **/
    bool canAccess( Creature *ch, const command_info *command );
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
    **/
    bool canAccess( Creature *ch, const show_struct &command );
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
    **/
    bool canAccess( Creature *ch, const set_struct &command );
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
    **/
    bool canAccess( Creature *ch, const board_info_type &board );
    /* Check membership in a particular group by name.**/
	bool isMember( Creature *ch, const char* group_name, bool substitute=true );

    /* can this character add/remove characters from this group. **/
    bool canAdminGroup( Creature *ch, const char* groupName );
    
    /* send a list of the current groups to a character */
    void sendGroupList( Creature *ch );
    /** 
     * creates a new group with the given name.  
     * returns false if the name is taken. 
     **/
    bool createGroup( char *name );
    /** 
     * removes the group with the given name. 
     * returns false if there is no such group. 
     **/
    bool removeGroup( char *name );
    
    /** returns true if the named group exists. **/
    bool isGroup( const char* name );
    /** retrieves the named group. **/
    Group& getGroup( const char* name);
    /** sends the member list of the named group to the given character. **/
    bool sendMemberList( Creature *ch, char *group_name );
    /** sends the command list of the named group to the given character. **/
    bool sendCommandList( Creature *ch, char *group_name );
    /** sends a list of the groups that the id is a member if. **/
    bool sendMembership( Creature *ch, long id );
    /* 
     * sends a list of the commands a char has access to and the
     * groups that contain them.
     **/
    bool sendAvailableCommands( Creature *ch, long id );
    /** 
     * adds the named command to the named group.  
     * returns false if either doesn't exist. 
     **/
    bool addCommand( char *command, char *group_name );
    /** 
     * adds the named character to the named group.  
     * returns false if either doesn't exist. 
     **/
    bool addMember( const char *member, const char *group_name );
    
    /** 
     * removes the named command from the named group.  
     * returns false if either doesn't exist. 
     **/
    bool removeCommand( char *command, char *group_name );
    /** 
     * removes the named character from the named group.  
     * returns false if either doesn't exist. 
     **/
    bool removeMember( const char *member, const char *group_name );
    
    /** stores the security groups in the given file in XML format. **/
    bool saveGroups( const char *filename = SECURITY_FILE );
    /** 
     * clears the current security groups and loads new groups and 
     * membership from the given filename 
     **/
    bool loadGroups( const char *filename = SECURITY_FILE );

    /**
     *
     * clears security related memory and shuts the system down.
     */
    void shutdown();
    
    void log( const char* msg, const char* name );
    void log( const char* msg, const long id );
}

/*  <Security> 
 *    <Group  Name="test"  Description="Testing Group" >
 *      <Member ID="1"/>
 *      <Command Name="look"/>
 *    </Group>
 *  </Security>
 */

#endif
