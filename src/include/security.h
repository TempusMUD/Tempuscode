#ifndef TEMPUS_SECURITY_H
#define TEMPUS_SECURITY_H

#include <vector>
#include <algorithm>
using namespace std;


extern struct command_info cmd_info[];
namespace Security {
    static const bool TRACE = true;
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
            /*
             * does not make a copy of name or desc.
             */
            Group( char *name, char *description ) : commands(), members() {
                _name = name;
                _description = description;
            }
            /*
             * Makes a copy of name
             */
            Group( const char *name ) : commands(), members() {
                trace("Group: Creating new group with name", name);
                _name = new char[strlen(name) + 1];
                strcpy(_name, name);

                _description = new char[40];
                strcpy(_description, "No Description");
                trace("Group: Created new group with name", _name);
            }
            Group( const Group &g ) {
                this->_name = strdup(g._name);
                this->_description = strdup(g._description);
                this->members = g.members;
                this->commands = g.commands;
                trace("Group: Created via copy constructor.", _name);
            }
            ~Group() {
                if( _description != NULL ) delete _description;
                if( _name != NULL ) delete _name;
            }

            bool addCommand( command_info *command );
            bool addMember( const char *name );
            bool addMember( long player );
            
            // These membership checks should be binary searches.
            bool member( long player );
            bool member( const char_data *ch );
            bool member( const command_info *command );
            bool givesAccess( const char_data *ch, const command_info *command );
            
            const char *getDescription() { return _description; }
            const char *getName() { return _name; }

            bool operator==( const char *name ) { return ( strcmp(_name,name) == 0 ); }
            bool operator!=( const char *name ) { return !(*this == name); }

            bool sendMemberList( char_data *ch );
            bool sendCommandList( char_data *ch );
        private:
            char *_description;
            char *_name;
            // resolved on group load/creation. 
            // Command names stored in file.
            // Pointers sorted as if they're ints.
            vector<command_info *> commands;
            // player ids, sorted
            vector<long> members;
    };
    extern list<Group> groups;
    
    /*
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
     */
    inline bool canAccess( const char_data *ch, const command_info *command ) {
        if(! command->security & GROUP ) {
            return (GET_LEVEL(ch) >= command->minimum_level || GET_IDNUM(ch) == 1);
        } else {
            if( GET_LEVEL(ch) < command->minimum_level ) {
                trace("canAcess failed: level < min_level");
                return false;
            }
        }

        list<Group>::iterator it = groups.begin(); 
        for( ; it != groups.end(); ++it ) {
            if( (*it).givesAccess(ch, command) )
                return true;
        }
        return false;
        trace("canAcess failed: no group gives access");
    }

    /*
     * Check membership in a particular group by name.
     */
    inline bool isMember( char_data *ch, const char* group_name ) {
        list<Group>::iterator it = groups.begin(); 
        for( ; it != groups.end(); ++it ) {
            if( (*it) == group_name )
                return (*it).member(ch);
        }
        trace("isMember check: false");
        return false;
    }
    /*
     * send a list of the current groups to a character
     */
    inline void sendGroupList( char_data *ch ) {
        static char linebuf[256];
        int i = 0;
        buf[0] = '\0';

        sprintf(buf, "Security Groups: \r\n");
        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it ) {
            sprintf(linebuf," %3d, %s\r\n",i++, (*it).getName());
            fprintf(stderr,(*it).getName());
            strcat(buf,linebuf);
        }
        send_to_char(buf,ch);
    }

    inline bool createGroup( char *name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        if( it != groups.end() ) {
            trace("createGroup: group exists");
            return false;
        }
        groups.push_back(name);
        return true;
    }

    inline bool sendMemberList( char_data *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendMemberList : group not found");
            return false;
        }
        return (*it).sendMemberList(ch);
    }
    inline bool sendCommandList( char_data *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendCommandList : group not found");
            return false;
        }
        return (*it).sendCommandList(ch);
    }
    inline bool addCommand( char *command, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("addCommand: group not found");
            return false;
        }
        int index = find_command( command );
        if( index == -1 ) {
            trace("addCommand: command not found");
            return false;
        }
        /* otherwise, find the command */

        return (*it).addCommand( &cmd_info[index] );
    }
    inline bool addMember( const char *member, const char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("addMember: group not found");
            return false;
        }

        return (*it).addMember( member );
    }

}
#endif
