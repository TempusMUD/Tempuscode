#include <vector>
#include <algorithm>
using namespace std;
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "security.h"
#include "tokenizer.h"


namespace Security {
    list<Group> groups;

    bool Group::addCommand( command_info *command ) {
        if( member( command ) )
            return false;
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
        if( member(player) )
            return false;
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
    bool Group::givesAccess( const char_data *ch, const command_info *command ) {
        return ( member(ch) && member(command) );
    }

    bool Group::sendMemberList( char_data *ch ) {
        int pos = 1;
        vector<long>::iterator it = members.begin();
        strcpy(buf,"");
        for( ; it != members.end(); ++it ) {
            sprintf(buf,"%s %20ld",buf,(*it));
            if( pos % 4 == 0 ) {
                pos = 1;
                strcat(buf,"\r\n");
            } else {
                pos++;
                strcat(buf,",");
            }
        }
        if( pos != 1 )
            strcat(buf,"\r\n");
        send_to_char(buf,ch);
        return true;
    }

    bool Group::sendCommandList( char_data *ch ) {
        int pos = 1;
        vector<command_info*>::iterator it = commands.begin();
        for( ; it != commands.end(); ++it ) {
            sprintf(buf,"%s %20s",buf,(*it)->command);
            if( pos % 4 == 0 ) {
                pos = 1;
                strcat(buf,"\r\n");
            } else {
                pos++;
                strcat(buf,",");
            }
        }
        if( pos != 1 )
            strcat(buf,"\r\n");
        send_to_char(buf,ch);
        return true;
    }

    
}
 /**
  * access list
  * access create pissers
  * access remove pissers
  *
  * access memberlist pissers
  * access addmember pissers forget ashe
  * access remmember pissers forget ashe
  *
  * access cmdlist pissers
  * access addcmd pissers wizpiss immpiss
  * access remcmd pissers wizpiss
  *
  **/
const struct {
    char *command;
    char *usage;
} access_cmds[] = {
    { "list",      "" },
    { "create",    "<group name>" },
    { "remove",    "<group name>" },
    { "memberlist" "<group name>" },
    { "addmember", "<group name> <member> [<member>...]" },
    { "remmember", "<group name> <member> [<member>...]" },
    { "cmdlist",   "<group name>"},
    { "addcmd",    "<group name> <command> [<command>...]" },
    { "remcmd",    "<group name> <command> [<command>...]" },
    { NULL, NULL }
};

void send_access_options( char_data *ch ) {
    int i = 0; 
        
    strcpy(buf, "access usage :\r\n");
    while (1) {
        if (!access_cmds[i].command)
            break;
        sprintf(buf, "%s  %-15s %s\r\n", buf, access_cmds[i].command, access_cmds[i].usage);
        i++;
    }
    page_string(ch->desc, buf, 1);
}

int find_access_command( char *command ) {
    if( command == NULL || *command == '\0' )
        return -1;
    for( int i = 0; access_cmds[i].command != NULL ; i++ ) {
        if( strcmp( access_cmds[i].command, command ) == 0 ) 
            return i;
    }
    return -1;
}

//   void (name)(struct char_data *ch, char *argument, int cmd, int subcmd, int *r
ACCMD(do_access) {
    if( argument == NULL || *argument == '\0' ) {
        send_access_options(ch);
        return;
    }
    
    Tokenizer tokens(argument, ' ');
    char token1[256];
    char token2[256];

    tokens.next(token1);

    switch( find_access_command(token1) ) {
        case 0: // list
            Security::sendGroupList(ch);
            break;
        case 1: // create
            if( tokens.next(token1) ) {
                if( Security::createGroup( token1 ) ) {
                    send_to_char( "Group created.\r\n",ch);
                } else {
                    send_to_char( "Group creation failed.\r\n",ch);
                }
            } else {
                send_to_char("Create a group with what name?\r\n",ch);
            }
            break;
        case 2: // remove
            send_to_char("Unimplemented.\r\n",ch);
            break;
        case 3: // memberlist
            if( tokens.next(token1) ) {
                if(! Security::sendMemberList( ch, token1 ) ) {
                    send_to_char("No such group.\r\n",ch);
                }
            } else {
                send_to_char("List members for what group?\r\n",ch);
            }
            break;
        case 4: // addmember
            if( tokens.next(token1) && tokens.next(token2) ) {
                if( Security::addCommand( token2, token1 ) ) {
                    send_to_char( "Member added.\r\n",ch);
                } else {
                    send_to_char( "Member addition failed.\r\n",ch);
                }
            } else {
                send_to_char("Add who to what group?\r\n",ch);
            }
            break;
        case 5: // remmember
            send_to_char("Unimplemented.\r\n",ch);
            break;
        case 6: // cmdlist
            if( tokens.next(token1) ) {
                if(! Security::sendCommandList( ch, token1 ) ) {
                    send_to_char("No such group.\r\n",ch);
                }
            } else {
                send_to_char("List commands for what group?\r\n",ch);
            }
            break;
        case 7: // addcmd
            if( tokens.next(token1) && tokens.next(token2) ) {
                if( Security::addCommand( token2, token1 ) ) {
                    send_to_char( "Command added.\r\n",ch);
                } else {
                    send_to_char( "Command addition failed.\r\n",ch);
                }
            } else {
                send_to_char("Add what command to what group?\r\n",ch);
            }
            break;
        case 8: // remcmd
            send_to_char("Unimplemented.\r\n",ch);
            break;
        default:
            send_access_options(ch);
            break;
    }
}






