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
#include "player_table.h"

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
    { NULL,         NULL }
};

static char out_buf[MAX_STRING_LENGTH + 2];

const char *Security::EVERYONE = "<ALL>";
const char *Security::NOONE = "<NONE>";

const char *Security::ADMINBASIC = "AdminBasic";
const char *Security::ADMINFULL = "AdminFull";
const char *Security::CLAN = "Clan";
const char *Security::CLANADMIN = "ClanAdmin";
const char *Security::CODER = "Coder";
const char *Security::CODERADMIN = "CoderAdmin";
const char *Security::DYNEDIT = "Dynedit";
const char *Security::GROUPSADMIN = "GroupsAdmin";
const char *Security::HELP = "Help";
const char *Security::HOUSE = "House";
const char *Security::OLC = "OLC";
const char *Security::OLCADMIN = "OLCAdmin";
const char *Security::OLCAPPROVAL = "OLCApproval";
const char *Security::OLCPROOFER = "OLCProofer";
const char *Security::OLCWORLDWRITE = "OLCWorldWrite";
const char *Security::QUESTOR = "Questor";
const char *Security::QUESTORADMIN = "QuestorAdmin";
const char *Security::TESTERS = "Testers";
const char *Security::WIZARDADMIN = "WizardAdmin";
const char *Security::WIZARDBASIC = "WizardBasic";
const char *Security::WIZARDFULL = "WizardFull";
const char *Security::WORLDADMIN = "WorldAdmin";

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
                    if( addMember( token2, token1 ) ) {
						
                        send_to_char(ch, "Member added : %s\r\n", token2);
						slog("Security:  %s added to group '%s' by %s.", 
						      token2, token1, GET_NAME(ch) );
					} else {
                        send_to_char(ch, "Unable to add member: %s\r\n", token2);
					}
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
						slog("Security:  Command '%s' added to '%s' by %s.", 
						      token2, token1, GET_NAME(ch) );
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
					sql_exec("update sgroups set admin=%d where idnum=%d",
						getGroup(token2).getID(), getGroup(token1).getID());
                    send_to_char(ch, "Administrative group set.\r\n");
                } else {
                    send_to_char(ch, "Set admin group to what?\r\n");
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
					slog("Security:  Group '%s' created by %s.", 
						  token1, GET_NAME(ch) );
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
					sql_exec("update sgroups set descrip='%s' where idnum=%d",
						tmp_sqlescape(linebuf), getGroup(token1).getID());
					slog("Security:  Group '%s' described by %s.", 
						  token1, GET_NAME(ch) );
                } else {
                    send_to_char(ch, "Set what description?\r\n");
                }
            } else {
                send_to_char(ch, "Describe what group?\r\n");
            }
            break;
        case 6: // grouplist
            if( tokens.next(token1) ) {
                long id = playerIndex.getID(token1);
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
						slog("Security:  %s removed from group '%s' by %s.", 
						      token2, token1, GET_NAME(ch) );
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
						slog("Security:  Command '%s' removed from '%s' by %s.", 
						      token2, token1, GET_NAME(ch) );
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
	 	if (group_name == Security::EVERYONE)
			return true;
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
		PGresult *res;
		int group_id;

        if( it != groups.end() ) {
            trace("createGroup: group exists");
            return false;
        }
		res = sql_query("select MAX(idnum) from sgroups");
		group_id = atoi(PQgetvalue(res, 0, 0)) + 1;

        groups.push_back(name);
		getGroup(name).setID(group_id);
        groups.sort();

		sql_exec("insert into sgroups (idnum, name, descrip) values (%d, '%s', 'No description.')",
			group_id, name);
        return true;
    }

     bool removeGroup( char *name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        if( it == groups.end() ) {
            return false;
        }
		sql_exec("update sgroups set admin=NULL where admin=%d", it->getID());
		sql_exec("delete from sgroup_members where sgroup=%d", it->getID());
		sql_exec("delete from sgroup_commands where sgroup=%d", it->getID());
		sql_exec("delete from sgroups where idnum=%d", it->getID());

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
        for( ; it != groups.end(); ++it ) {
            if( (*it).member(id) && (*it).getCommandCount() > 0 ) {
                ++n;
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

		sql_exec("insert into sgroup_commands (sgroup, command) values (%d, '%s')",
			it->getID(), cmd_info[index].command);
        return (*it).addCommand( &cmd_info[index] );
    }
    
    bool addMember( const char *member, const char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
		long player_id;

        if( it == groups.end() ) {
            trace("addMember: group not found");
            return false;
        }

		if (!playerIndex.exists(member)) {
			trace("addMember: player not found");
			return false;
		}

		player_id = playerIndex.getID(member);
		if (it->member(player_id)) {
			trace("addMember: player already exists in group");
			return false;
		}

		sql_exec("insert into sgroup_members (sgroup, player) values (%d, %ld)",
			it->getID(), player_id);

        return (*it).addMember(player_id);
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
		sql_exec("delete from sgroup_commands where sgroup=%d and command='%s'",
			it->getID(), cmd_info[index].command);
        return (*it).removeCommand( &cmd_info[index] );
    }
    
    bool removeMember( const char *member, const char *group_name ) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), group_name );
		long player_id;

        if( it == groups.end() ) {
            trace("removeMember: group not found");
            return false;
        }
		player_id = playerIndex.getID(member);
		if (player_id < 1) {
			trace("removeMember: player not found");
			return false;
		}
			
		sql_exec("delete from sgroup_members where sgroup=%d and player=%ld",
			it->getID(), player_id);
        return (*it).removeMember( member );
    }
    
    /** returns true if the named group exists. **/
    bool isGroup( const char* name ) {
		if( name == NULL )
			return false;
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        return( it != groups.end() );
    }
    
    
    /** retrieves the named group. **/
    Group& getGroup( const char* name) {
        list<Group>::iterator it = find( groups.begin(), groups.end(), name );
        return *it;
    }

    bool loadGroups(void) {
		PGresult *res;
		char *name;
		int idx, count, cmd;

        groups.erase(groups.begin(),groups.end());
		res = sql_query("select sgroups.idnum, sgroups.name, sgroups.descrip, admin.name from sgroups left outer join sgroups as admin on admin.idnum=sgroups.admin order by sgroups.name");
		count = PQntuples(res);
		for (idx = 0;idx < count;idx++) {
			name = PQgetvalue(res, idx, 1);
			groups.push_back(Group(str_dup(name),
					str_dup(PQgetvalue(res, idx, 2))));
			getGroup(name).setAdminGroup(PQgetvalue(res, idx, 3));
			getGroup(name).setID(atoi(PQgetvalue(res, idx, 0)));
		}

		res = sql_query("select name, command from sgroups, sgroup_commands where sgroups.idnum=sgroup_commands.sgroup order by command");
		count = PQntuples(res);
		for (idx = 0;idx < count;idx++) {
			cmd = find_command(PQgetvalue(res, idx, 1));
			if( cmd >= 0 )
				getGroup(PQgetvalue(res, idx, 0)).addCommand(&cmd_info[cmd]);
			else
				errlog("Invalid command '%s' in security group '%s'",
					PQgetvalue(res, idx, 1), PQgetvalue(res, idx, 0));
		}

		res = sql_query("select name, player from sgroups, sgroup_members where sgroups.idnum=sgroup_members.sgroup order by player");
		count = PQntuples(res);
		for (idx = 0;idx < count;idx++) {
			getGroup(PQgetvalue(res, idx, 0)).addMember(atol(PQgetvalue(res, idx, 1)));
		}

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
