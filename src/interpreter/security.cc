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
#include "tokenizer.h"
#include "screen.h"
#include "security.h"
#include "boards.h"

using namespace Security;


 /**
  *
  * The command struct for access commands
  * ( with 'pissers' being a group )
  * 
  * access list
  * access create pissers
  * access remove pissers
  * access description pissers The wierdos that piss a lot
  * access admingroup pissers PisserAdmins
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
    { "addmember",  "<group name> <member> [<member>...]" },
    { "addcmd",     "<group name> <command> [<command>...]" },
    { "admin",      "<group name> <admin group>"},
    { "cmdlist",    "<group name>"},
    { "create",     "<group name>" },
    { "describe",   "<group name> <description>"},
    { "grouplist",  "<player name>" },
    { "list",       "" },
    { "load",       ""},
    { "memberlist", "<group name>" },
    { "remmember",  "<group name> <member> [<member>...]" },
    { "remcmd",     "<group name> <command> [<command>...]" },
    { "destroy",     "<group name>" },
    { "stat",       "<group name>"}, 
    { "save",       ""},
    { NULL,         NULL }
};

static char out_buf[MAX_STRING_LENGTH + 2];

/**
 *  Sends usage info to the given character
 */
void send_access_options( Creature *ch ) {
    int i = 0; 
    strcpy(out_buf, "access usage :\r\n");
    while (1) {
        if (!access_cmds[i].command)
            break;
        sprintf(out_buf, "%s  %-15s %s\r\n", out_buf, access_cmds[i].command, access_cmds[i].usage);
        i++;
    }
    page_string(ch->desc, out_buf);
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
        case 0: // addmember
            
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot add members to this group.\r\n");
                    return;
                }
                
                if(! tokens.hasNext() )
                    send_to_char(ch, "Add what member?\r\n");

                while( tokens.next(token2) ) {
                    if( addMember( token2, token1 ) )
                        send_to_char(ch, "Member added : %s\r\n", token2);
                    else
                        send_to_char(ch, "Unable to add member: %s\r\n", token2);
                }
            } else
                send_to_char(ch, "Add what member to what group?\r\n");
            break;
        case 1: // addcmd
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot add commands to this group.\r\n");
                    return;
                }
                if(! tokens.hasNext() ) {
                    send_to_char(ch, "Add what command?\r\n");
                }
                while( tokens.next(token2) ) {
                    if( addCommand( token2, token1 ) ) {
                        send_to_char(ch, "Command added : %s\r\n", token2);
                    } else {
                        send_to_char(ch, "Unable to add command: %s\r\n", token2);
                    }
                }
            } else {
                send_to_char(ch, "Add what command to what group?\r\n");
            }
            break;
        case 2: // Admin
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot alter this group.\r\n");
                    return;
                }
                if( tokens.next(token2) && isGroup(token2) ) {
                    getGroup(token1).setAdminGroup( token2 );
                    send_to_char(ch, "Administrative group set.\r\n");
                } else {
                    send_to_char(ch, "Set admin group to what?\r\n",ch);        
                }
            } else {
                send_to_char(ch, "Set the admin group for which group?\r\n");
            }
            break;
        case 3: // cmdlist
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! sendCommandList( ch, token1 ) ) {
                    send_to_char(ch, "No such group.\r\n");
                }
            } else {
                send_to_char(ch, "List commands for what group?\r\n");
            }
            break;
        case 4: // create
            if(! isMember(ch, "GroupsAdmin") ) {
                send_to_char(ch, "You cannot create groups.\r\n");
                return;
            }
            if( tokens.next(token1) ) {
                if( Security::createGroup( token1 ) ) {
                    send_to_char(ch,  "Group created.\r\n");
                } else {
                    send_to_char(ch,  "Group creation failed.\r\n");
                }
            } else {
                send_to_char(ch, "Create a group with what name?\r\n");
            }
            break;
        case 5: // Describe
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot describe this group.\r\n");
                    return;
                }
                if( tokens.remaining(linebuf) ) {
                    getGroup(token1).setDescription( linebuf );
                    send_to_char(ch, "Description set.\r\n");
                } else {
                    send_to_char(ch, "Set what description?\r\n",ch);        
                }
            } else {
                send_to_char(ch, "Describe what group?\r\n");
            }
            break;
        case 6: // grouplist
            if( tokens.next(token1) ) {
                long id = get_id_by_name(token1);
                if( id <= 0 ) {
                    send_to_char(ch, "No such player.\r\n");
                    return;
                }
                //send_to_char(ch, "You are a member of the following groups.\r\n");
                send_to_char(ch, "%s is a member of the following groups:\r\n",token1);
                Security::sendMembership(ch, id);
            } else {
                send_to_char(ch, "You are a member of the following groups:\r\n");
                Security::sendMembership(ch, GET_IDNUM(ch) );
            }
            break;
        case 7: // list
            sendGroupList(ch);
            break;
        case 8: // Load
            if( loadGroups() ) {
                send_to_char(ch, "Access groups loaded.\r\n");
            } else {
                send_to_char(ch, "Error loading access groups.\r\n");
            }
            break;
        case 9: // memberlist
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! sendMemberList( ch, token1 ) ) {
                    send_to_char(ch, "No such group.\r\n");
                }
            } else {
                send_to_char(ch, "List members for what group?\r\n");
            }
            break;
        case 10: // remmember
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot remove members from this group.\r\n");
                    return;
                }

                if(! tokens.hasNext() ) {
                    send_to_char(ch, "Remove what member?\r\n");
                }
                while( tokens.next(token2) ) {
                    if( removeMember( token2, token1 ) ) {
                        send_to_char(ch, "Member removed : %s\r\n", token2);
                    } else {
                        send_to_char(ch, "Unable to remove member: %s\r\n", token2);
                    }
                }
            } else {
                send_to_char(ch, "Remove what member from what group?\r\n");
            }
            break;
        case 11: // remcmd
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! canAdminGroup( ch, token1 ) ) {
                    send_to_char(ch, "You cannot remove commands from this group.\r\n");
                    return;
                }
                if(! tokens.hasNext() ) {
                    send_to_char(ch, "Remove what command?\r\n");
                }
                while( tokens.next(token2) ) {
                    if( removeCommand( token2, token1 ) ) {
                        send_to_char(ch, "Command removed : %s\r\n", token2);
                    } else {
                        send_to_char(ch, "Unable to remove command: %s\r\n", token2);
                    }
                }
            } else {
                send_to_char(ch, "Remove what command from what group?\r\n");
            }
            break;
        case 12: // destroy
            if(! isMember(ch, "GroupsAdmin") ) {
                send_to_char(ch, "You cannot destroy groups.\r\n");
                return;
            }
            if( tokens.next(token1) && isGroup(token1) ) {
                if( removeGroup( token1 ) ) {
                    send_to_char(ch,  "Group destroyed.\r\n");
                } else {
                    send_to_char(ch,  "Group destruction failed.\r\n");
                }
            } else {
                send_to_char(ch, "Destroy which group?\r\n");
            }
            break;
        case 13: // Stat
            if( tokens.next(token1) && isGroup(token1) ) {
                getGroup(token1).sendStatus(ch);
            } else {
                send_to_char(ch, "Stat what group?\r\n");
            }
            break;
        case 14: // save
            if( saveGroups() ) {
                send_to_char(ch, "Access groups saved.\r\n");
            } else {
                send_to_char(ch, "Error saving access groups.\r\n");
            }
            break;
        default:
            send_access_options(ch);
            break;
    }
}

/*
 * The Security Namespace function definitions
 */
namespace Security {
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
     */
     bool canAccess( Creature *ch, const command_info *command ) {
	 	if (GET_LEVEL(ch) == LVL_GRIMP)
			return true;
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
    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
     **/
     bool canAccess( Creature *ch, const show_struct &command ) {
        if(command.level > GET_LEVEL(ch) )
            return false;
        if( *(command.group) =='\0')
            return true;
        if( isMember(ch, command.group) ) 
            return true;
        return false;
     }

    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
     **/
     bool canAccess( Creature *ch, const set_struct &command ) {
        if(command.level > GET_LEVEL(ch) )
            return false;
        if( *(command.group) =='\0')
            return true;
        if( isMember(ch, command.group) ) 
            return true;
        return false;
     }

    /**
     * Returns true if the character is the proper level AND is in
     * one of the required groups (if any)
     **/
     bool canAccess( Creature *ch, const board_info_type &board ) {
        if( board.read_lvl > GET_LEVEL(ch) )
            return false;
        if( *(board.groupname) =='\0')
            return true;
        if( isMember(ch, board.groupname) ) 
            return true;
        return false;
     }

    /**
     * Returns an iterator pointing to the named group
     * in the groups list or groups.end() if not found.
    **/
    list<Group>::iterator findGroup( const char* name ) {
        list<Group>::iterator it = groups.begin(); 
        for( ; it != groups.end(); ++it ) {
            if( (*it) == name )
                break;
        }
        return it;
    }

    /*
     * Check membership in a particular group by name.
     * Comma delimited names are also accepted.
     */
     bool isMember( Creature *ch, const char* group_name, bool substitute) {
        if( substitute && ch->getLevel() == LVL_GRIMP )
            return true;
        if( group_name == NULL || *group_name == '\0' )
            return false;

        Tokenizer tokens(group_name,',');
        char token[strlen(group_name) + 1];

        while( tokens.next(token) ) {
            list<Group>::iterator it = findGroup(token);
            if( it != groups.end() && (*it).member(ch) )
                return true;
        }
        return false;
    }

    /*
     * send a list of the current groups to a character
     */
     void sendGroupList( Creature *ch ) {
        const char *nrm = CCNRM(ch,C_NRM);
        const char *cyn = CCCYN(ch,C_NRM);
        const char *grn = CCGRN(ch,C_NRM);

        send_to_char(ch,
                "%s%15s %s[%scmds%s] [%smbrs%s]%s - Description \r\n",
                grn,
                "Group",
                cyn,
                nrm,
                cyn,
                nrm,
                cyn,
                nrm);

        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it ) {
            (*it).sendString(ch);
        }
    }

     bool createGroup( char *name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        if( it != groups.end() ) {
            trace("createGroup: group exists");
            return false;
        }
        groups.push_back(name);
        groups.sort();
        return true;
    }

     bool removeGroup( char *name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        if( it == groups.end() ) {
            return false;
        }
        groups.erase(it);
        return true;
    }

     bool sendMemberList( Creature *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendMemberList : group not found");
            return false;
        }
        return (*it).sendMemberList(ch);
    }
    
     bool sendCommandList( Creature *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendCommandList : group not found");
            return false;
        }
        return (*it).sendCommandList(ch);
    }
    /** sends a list of the groups that the id is a member if. **/
    bool sendMembership( Creature *ch, long id ) {
        int n = 0;
        out_buf[0] = '\0';
        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it ) {
            if( (*it).member(id) ) {
                (*it).sendString(ch);
                n++;
            }
        }
        if( n <= 0 ) {
            send_to_char(ch, "That player is not in any groups.\r\n");
            return false;
        }
        send_to_char(ch, out_buf);
        return true;
    }
    
    /* sends a list of the commands a char has access to and the
     * groups that contain them.
    **/
    bool sendAvailableCommands( Creature *ch, long id ) {
        int n = 0;
        out_buf[0] = '\0';
        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it,++n ) {
            if( (*it).member(id) && (*it).getCommandCount() > 0 ) {
                send_to_char(ch, "%s%s%s\r\n", CCYEL(ch,C_NRM),
                        (*it).getName(), CCNRM(ch,C_NRM) );
                (*it).sendCommandList( ch, false );
            }
        }
        if( n <= 0 ) {
            send_to_char(ch, "That player is not in any groups.\r\n");
            return false;
        }
        return true;
    }
    
    bool addCommand( char *command, char *group_name ) {
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
    
    bool addMember( const char *member, const char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("addMember: group not found");
            return false;
        }

        return (*it).addMember( member );
    }

    bool removeCommand( char *command, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("removeCommand: group not found");
            return false;
        }
        int index = find_command( command );
        if( index == -1 ) {
            trace("removeCommand: command not found");
            return false;
        }
        return (*it).removeCommand( &cmd_info[index] );
    }
    
    bool removeMember( const char *member, const char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("removeMember: group not found");
            return false;
        }
        return (*it).removeMember( member );
    }
    
    /** returns true if the named group exists. **/
    bool isGroup( const char* name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        return( it != groups.end() );
    }
    
    
    /** retrieves the named group. **/
    Group& getGroup( const char* name) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        return *it;
    }
        
    bool saveGroups( const char *filename) {
        xmlDocPtr doc;
        doc = xmlNewDoc((const xmlChar*)"1.0");
        doc->children = xmlNewDocNode(doc, NULL, (const xmlChar *)"Security", NULL);
        
        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it ) {
            (*it).save(doc->children);
        }
        int rc = xmlSaveFormatFile( filename, doc, 1 );
        xmlFreeDoc(doc);
        slog("Security:  Access group data saved.");
        return( rc != -1 );
    }

    bool loadGroups( const char *filename) {
        groups.erase(groups.begin(),groups.end());
        xmlDocPtr doc = xmlParseFile(filename);
        if( doc == NULL ) {
            return true;
        }
        // discard root node
        xmlNodePtr cur = xmlDocGetRootElement(doc);
        if( cur == NULL ) {
            xmlFreeDoc(doc);
            return false;
        }
        cur = cur->xmlChildrenNode;
        // Load all the nodes in the file
        while (cur != NULL) {
            // But only question nodes
            if ((xmlMatches(cur->name, "Group"))) {
                groups.push_back(Group(cur));
            }
            cur = cur->next;
        }
        xmlFreeDoc(doc);
        slog("Security:  Access group data loaded.");
        return true;
    }
    
    bool canAdminGroup( Creature *ch, const char* groupName ) {
        // The name of the administrative group
        const char* admin = NULL;
        
        if(! isGroup(groupName) )
            return false;
        if( isMember(ch, "GroupsAdmin") )
            return true;
        admin = getGroup(groupName).getAdminGroup();
        if( admin == NULL || !isGroup(admin) )
            return false;
        return getGroup(admin).member(ch);
    }
     /**
      * clears security related memory and shuts the system down.
      */
    void shutdown() {
        list<Group>::iterator it = groups.begin();
        for(; it != groups.end(); it++) {
            (*it).clear();
        }
        groups.erase(groups.begin(),groups.end());
    }

}
