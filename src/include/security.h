#ifndef TEMPUS_SECURITY_H
#define TEMPUS_SECURITY_H


#include <vector>
#include <algorithm>
using namespace std;

namespace Security {
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
    const int GROUP = (1 << 0);

    class Group {
        public:
            Group( char *name, char *description ) : commands(), members() {
                this->name = name;
                this->description = description;
            }
            ~Group() {
                if( description != NULL ) delete description;
                if( name != NULL ) delete name;
            }

            bool addCommand( command_info *command );
            bool addMember( const char *name );
            bool addMember( long player );
            
            // These membership checks should be binary searches.
            bool member( long player );
            bool member( const char_data *ch );
            bool member( const command_info *command );
            bool givesAccess( const char_data *ch, const command_info *command );
            
            const char *getDescription() { return description; }
            const char *getName() { return name; }

            bool operator==( const char *name ) { return ( strcmp(this->name,name) == 0 ); }
        private:
            char *description;
            char *name;
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
                return false;
            }
        }

        list<Group>::iterator it = groups.begin(); 
        for( ; it != groups.end(); ++it ) {
            if( (*it).givesAccess(ch, command) )
                return true;
        }
        return false;
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
        return false;
    }
}
#endif
