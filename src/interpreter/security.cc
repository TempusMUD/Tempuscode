#include <vector>
#include <algorithm>
using namespace std;
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
// Undefine CHAR to avoid collisions
#undef CHAR 
#include "xml_utils.h"
// Tempus includes
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "security.h"
#include "tokenizer.h"


namespace Security {
    list<Group> groups;

    void updateSecurity(command_info *command) {

        list<Group>::iterator it = groups.begin(); 
        for( ; it != groups.end(); ++it ) {
            if( (*it).member( command ) )
                return;
        }
        command->security &= ~(GROUP);
    }

    bool Group::addCommand( command_info *command ) {
        if( member( command ) )
            return false;
        command->security |= GROUP;
        commands.push_back(command);
        sort( commands.begin(), commands.end() );
        return true;
    }

    bool Group::removeCommand( command_info *command ) {
        vector<command_info *>::iterator it;
        it = find(commands.begin(), commands.end(), command);
        if( it == commands.end() )
            return false;

        // Remove the command
        commands.erase(it);

        // Remove the group bit from the command
        updateSecurity(command);

        sort( commands.begin(), commands.end() );
        return true;
    }

    bool Group::removeMember( const char *name ) {
        long id = get_id_by_name(name);
        if( id < 0 ) 
            return false;
        return removeMember(id);
    }

    bool Group::removeMember( long player ) {
        vector<long>::iterator it;
        it = find( members.begin(), members.end(), player );
        if( it == members.end() )
            return false;
        members.erase(it);
        sort( members.begin(), members.end() );
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
        strcpy(buf,"Members:\r\n");
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
        strcpy(buf,"Commands:\r\n");
        for( ; it != commands.end(); ++it ) {
            sprintf(buf,"%s   %10s",buf,(*it)->command);
            if( pos++ % 3 == 0 ) {
                pos = 1;
                strcat(buf,"\r\n");
            }
        }
        if( pos != 1 )
            strcat(buf,"\r\n");
        send_to_char(buf,ch);
        return true;
    }
    
    /*
     * does not make a copy of name or desc.
     */
    Group::Group( char *name, char *description ) : commands(), members() {
        _name = name;
        _description = description;
    }

    /*
     *  Loads a group from the given xmlnode;
     *  Intended for reading from a file
     */
    Group::Group( xmlNodePtr node ) {
        // properties
        _name = xmlGetProp(node, "Name");
        _description = xmlGetProp(node, "Description");
        long member;
        char *command;
        // commands & members
        node = node->xmlChildrenNode;
        while (node != NULL) {
            if ((!xmlStrcmp(node->name, (const xmlChar*)"Member"))) {
                member = xmlGetLongProp(node, "ID");
                if( member == 0 )
                    continue;
                members.push_back(member);
            }
            if ((!xmlStrcmp(node->name, (const xmlChar*)"Command"))) {
                command = xmlGetProp(node, "Name");
                int index = find_command( command );
                if( index == -1 ) {
                    trace("Group(xmlNodePtr): command not found", command);
                } else {
                    addCommand( &cmd_info[index] );
                }
            }
            node = node->next;
        }
    }
    
    /*
     * Makes a copy of name
     */
    Group::Group( const char *name ) : commands(), members() {
        _name = new char[strlen(name) + 1];
        strcpy(_name, name);

        _description = new char[40];
        strcpy(_description, "No Description");
    }

    /*
     * Makes a complete copy of teh Group
     */
    Group::Group( const Group &g ) {
        this->_name = strdup(g._name);
        this->_description = strdup(g._description);
        this->members = g.members;
        this->commands = g.commands;
    }

    /*
     * Create the required xmlnodes to recreate this group
     */
    bool Group::save( xmlNodePtr parent ) {
        xmlNodePtr node = NULL;
        
        xmlNewChild( parent, NULL, (const xmlChar *)"Group", NULL );
        xmlSetProp( parent, "Name", _name ); 
       
        vector<command_info*>::iterator cit = commands.begin();
        for( ; cit != commands.end(); ++cit ) {
            node = xmlNewChild( parent, NULL, (const xmlChar *)"Command", NULL );
            xmlSetProp( parent, "Name", (*cit)->command );
        }
        vector<long>::iterator mit = members.begin();
        for( ; mit != members.end(); ++mit ) {
            node = xmlNewChild( parent, NULL, (const xmlChar *)"Member", NULL );
            xmlSetProp( parent, "ID", *mit );
        }
        return true;
    }


    /*
     *
     */
    Group::~Group() {
        while( commands.begin() != commands.end() ) {
            removeCommand( *( commands.begin() ) );
        }
        if( _description != NULL ) delete _description;
        if( _name != NULL ) delete _name;
    }
}
 /**
  *
  * The command struct for access commands
  * 
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
    { "list",       "" },
    { "create",     "<group name>" },
    { "remove",     "<group name>" },
    { "memberlist", "<group name>" },
    { "addmember",  "<group name> <member> [<member>...]" },
    { "remmember",  "<group name> <member> [<member>...]" },
    { "cmdlist",    "<group name>"},
    { "addcmd",     "<group name> <command> [<command>...]" },
    { "remcmd",     "<group name> <command> [<command>...]" },
    { NULL,         NULL }
};

/**
 *  Sends usage info to the given character
 */
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

/**
 *  Find the proper 'access' subcommand
 */
int find_access_command( char *command ) {
    if( command == NULL || *command == '\0' )
        return -1;
    for( int i = 0; access_cmds[i].command != NULL ; i++ ) {
        if( strncmp( access_cmds[i].command, command, strlen(command) ) == 0 ) 
            return i;
    }
    return -1;
}

/**
 *  The access command itself. 
 *  Used as an interface for the modification of security settings
 *  from within the mud.
 */
ACCMD(do_access) {
    if( argument == NULL || *argument == '\0' ) {
        send_access_options(ch);
        return;
    }
    
    Tokenizer tokens(argument, ' ');
    char token1[256];
    char token2[256];
    static char linebuf[8192];

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
            if( tokens.next(token1) ) {
                if( Security::removeGroup( token1 ) ) {
                    send_to_char( "Group removed.\r\n",ch);
                } else {
                    send_to_char( "Group removal failed.\r\n",ch);
                }
            } else {
                send_to_char("Remove which group?\r\n",ch);
            }
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
            if( tokens.next(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Add what member?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( Security::addMember( token2, token1 ) ) {
                        sprintf(linebuf, "Member added : %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    } else {
                        sprintf(linebuf, "Unable to add member: %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    }
                }
            } else {
                send_to_char("Add what member to what group?\r\n",ch);
            }
            break;
        case 5: // remmember
            if( tokens.next(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Remove what member?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( Security::removeMember( token2, token1 ) ) {
                        sprintf(linebuf, "Member removed : %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    } else {
                        sprintf(linebuf, "Unable to remove member: %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    }
                }
            } else {
                send_to_char("Remove what member from what group?\r\n",ch);
            }
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
            if( tokens.next(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Add what command?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( Security::addCommand( token2, token1 ) ) {
                        sprintf(linebuf, "Command added : %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    } else {
                        sprintf(linebuf, "Unable to add command: %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    }
                }
            } else {
                send_to_char("Add what command to what group?\r\n",ch);
            }
            break;
        case 8: // remcmd
            if( tokens.next(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Remove what command?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( Security::removeCommand( token2, token1 ) ) {
                        sprintf(linebuf, "Command removed : %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    } else {
                        sprintf(linebuf, "Unable to remove command: %s\r\n", token2);
                        send_to_char( linebuf, ch );
                    }
                }
            } else {
                send_to_char("Remove what command from what group?\r\n",ch);
            }
            break;
        default:
            send_access_options(ch);
            break;
    }
}
