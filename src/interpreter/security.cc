#include <vector>
#include <algorithm>
using namespace std;
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "security.h"

namespace Security {
    list<Group> groups;

    bool Group::addCommand( command_info *command ) {
        commands.push_back(command);
        sort( commands.begin(), commands.end() );
        return true;
    }

    bool Group::addMember( const char *name ) {
        long id = get_id_by_name(name);
        if( id < 0 ) 
            return false;
        return addMember(id);
    }

    bool Group::addMember( long player ) {
        members.push_back(player);
        sort( members.begin(), members.end() );
        return true;
    }

    // These membership checks should be binary searches.
    bool Group::member( long player ) { 
        return ( find(members.begin(), members.end(), player) != members.end() ); 
    }
    bool Group::member( const char_data *ch ) { 
        return member(GET_IDNUM(ch)); 
    }
    bool Group::member( const command_info *command ) { 
        return ( find(commands.begin(), commands.end(), command) != commands.end() ); 
    }
    bool Security::Group::givesAccess( const char_data *ch, const command_info *command ) {
        return ( member(ch) && member(command) );
    }
}
ACCMD(do_security) {
}
