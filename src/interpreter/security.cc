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

using namespace Security;


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
    { "grouplist",  "<player name>" },
    { "load",       ""},
    { "save",       ""},
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
            sendGroupList(ch);
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
            if( tokens.next(token1) && isGroup(token1) ) {
                if( removeGroup( token1 ) ) {
                    send_to_char( "Group removed.\r\n",ch);
                } else {
                    send_to_char( "Group removal failed.\r\n",ch);
                }
            } else {
                send_to_char("Remove which group?\r\n",ch);
            }
            break;
        case 3: // memberlist
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! sendMemberList( ch, token1 ) ) {
                    send_to_char("No such group.\r\n",ch);
                }
            } else {
                send_to_char("List members for what group?\r\n",ch);
            }
            break;
        case 4: // addmember
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Add what member?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( addMember( token2, token1 ) ) {
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
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Remove what member?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( removeMember( token2, token1 ) ) {
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
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! sendCommandList( ch, token1 ) ) {
                    send_to_char("No such group.\r\n",ch);
                }
            } else {
                send_to_char("List commands for what group?\r\n",ch);
            }
            break;
        case 7: // addcmd
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Add what command?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( addCommand( token2, token1 ) ) {
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
            if( tokens.next(token1) && isGroup(token1) ) {
                if(! tokens.hasNext() ) {
                    send_to_char("Remove what command?\r\n",ch);
                }
                while( tokens.next(token2) ) {
                    if( removeCommand( token2, token1 ) ) {
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
        case 9: // grouplist
            if( tokens.next(token1) ) {
                send_to_char("Unimplemented.",ch);
            } else {
                send_to_char("List group membership for what player?",ch);
            }
        case 10: // Load
            if( loadGroups() ) {
                send_to_char("Access groups loaded.\r\n",ch);
            } else {
                send_to_char("Error loading access groups.\r\n",ch);
            }
            break;
        case 11: // save
            if( saveGroups() ) {
                send_to_char("Access groups saved.\r\n",ch);
            } else {
                send_to_char("Error saving access groups.\r\n",ch);
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
     bool canAccess( const char_data *ch, const command_info *command ) {
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
     bool isMember( char_data *ch, const char* group_name ) {
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
     void sendGroupList( char_data *ch ) {
        static char linebuf[256];

        sprintf(linebuf,"%15s [cmds] [mbrs]\r\n","Group");
        strcat(buf,linebuf);

        list<Group>::iterator it = groups.begin();
        for( ; it != groups.end(); ++it ) {
            (*it).toString(linebuf);
            fprintf(stderr,linebuf);
            strcat(buf,linebuf);
        }
        send_to_char(buf,ch);
    }

     bool createGroup( char *name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        if( it != groups.end() ) {
            trace("createGroup: group exists");
            return false;
        }
        groups.push_back(name);
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

     bool sendMemberList( char_data *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendMemberList : group not found");
            return false;
        }
        return (*it).sendMemberList(ch);
    }
    
     bool sendCommandList( char_data *ch, char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
        if( it == groups.end() ) {
            trace("sendCommandList : group not found");
            return false;
        }
        return (*it).sendCommandList(ch);
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
    
    bool saveGroups( const char *filename = SECURITY_FILE ) {
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

    bool loadGroups( const char *filename = SECURITY_FILE ) {
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
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Group"))) {
                groups.push_back(Group(cur));
            }
            cur = cur->next;
        }
        xmlFreeDoc(doc);
        slog("Security:  Access group data loaded.");
        return true;
    }
}
