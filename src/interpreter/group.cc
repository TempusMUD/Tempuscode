#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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
#include "screen.h"
#include "player_table.h"
#include "accstr.h"

/*
 * The Security Namespace Group function definitions.
 */
namespace Security {
    /** The global container of Group objects. **/
    list<Group> groups;

    char buf[MAX_STRING_LENGTH + 1];

    void log( const char* msg, const char* name ) {
        slog("<SECURITY> %s : %s", msg, name);
    }

    void log( const char* msg, const long id ) {
        slog("<SECURITY> %s : %ld", msg, id );
    }

    /* sets this group's description */
    void Group::setDescription(const char *desc) {
        if( _description != NULL )
            free(_description);
        _description = strdup(desc);
    }

    // These membership checks should be binary searches.
    bool Group::member( long player ) {
        return binary_search(members.begin(), members.end(), player);
    }
    bool Group::member(  Creature *ch ) {
        return member(GET_IDNUM(ch));
    }
    bool Group::member( const command_info *command ) {
        return binary_search( commands.begin(), commands.end(), command );
    }
    bool Group::givesAccess(  Creature *ch, const command_info *command ) {
        return ( member(ch) && member(command) );
    }

    /* sets the name of the group that can admin this group. */
    void Group::setAdminGroup(const char *group) {
		if (_adminGroup != NULL)
			free(_adminGroup);
        _adminGroup = strdup(group);
    }

    /* sprintf's a one line desc of this group into out */
    void Group::sendString(Creature *ch) {
        const char *nrm = CCNRM(ch,C_NRM);
        const char *cyn = CCCYN(ch,C_NRM);
        const char *grn = CCGRN(ch,C_NRM);

        send_to_char(ch,
                "%s%15s %s[%s%4zd%s] [%s%4zd%s]%s - %s\r\n",
                grn, _name, cyn,
                nrm, commands.size(), cyn,
                nrm, members.size(), cyn,
                nrm, _description );
    }

    /* sends a multi-line status of this group to ch */
    void Group::sendStatus( Creature *ch ) {
        const char *nrm = CCNRM(ch,C_NRM);
        const char *cyn = CCCYN(ch,C_NRM);
        const char *grn = CCGRN(ch,C_NRM);

        send_to_char(ch,
                "Name: %s%s%s [%s%4zd%s][%s%4zd%s]%s\r\n",
                grn, _name, cyn,
                nrm, commands.size(), cyn,
                nrm, members.size(), cyn,
                nrm);
        send_to_char(ch,
                "Admin Group: %s%s%s\r\n",
                grn,
                _adminGroup ? _adminGroup : "None",
                nrm);
        send_to_char(ch,
                "Description: %s\r\n",
                _description);
        sendCommandList( ch );
        sendMemberList( ch );
    }

    /* Adds a command to this group. Fails if already added. */
    bool Group::addCommand( command_info *command ) {
        if( member( command ) )
            return false;
        command->group_count += 1;
        commands.push_back(command);
        sort( commands.begin(), commands.end() );
        return true;
    }

    /* Removes a command from this group. Fails if not a member. */
    bool Group::removeCommand( command_info *command ) {
        vector<command_info *>::iterator it;
        it = find(commands.begin(), commands.end(), command);
        if( it == commands.end() )
            return false;

        // Remove the command
        commands.erase(it);

        // Remove the group bit from the command
        command->group_count -= 1;
        return true;
    }

    /* Removes a member from this group by player name. Fails if not a member. */
    bool Group::removeMember( const char *name ) {
        long id = playerIndex.getID(name);
        if( id < 0 )
            return false;
        return removeMember(id);
    }

    /* Removes a member from this group by player id. Fails if not a member. */
    bool Group::removeMember( long player ) {
        vector<long>::iterator it;
        it = find( members.begin(), members.end(), player );
        if( it == members.end() )
            return false;
        members.erase(it);

        sort( members.begin(), members.end() );
        return true;
    }

    /* Adds a member to this group by name. Fails if already added. */
    bool Group::addMember( const char *name ) {
        long id = playerIndex.getID(name);
        if( id < 0 )
            return false;

        return addMember(id);
    }

    /* Adds a member to this group by player id. Fails if already added. */
    bool Group::addMember( long player ) {
        if( member(player) )
            return true;
        members.push_back(player);

        sort( members.begin(), members.end() );
        return true;
    }

	bool Group::sendPublicMember( Creature *ch, char* prefix ) {
		if( members.size() == 0 )
			return false;
		const char* name = playerIndex.getName(members[0]);
		if(!name)
			return false;
        acc_strcat(prefix, CCYEL_BLD(ch, C_NRM), name, CCNRM(ch, C_NRM), NULL);
		return true;
	}

    /* Sends a list of this group's members to the given character. */
    bool Group::sendPublicMemberList( Creature *ch, const char *title, const char *adminGroup ) {
        vector<long>::iterator it;
        int pos = 0;
		const char *name;
		Group* group = NULL;
		bool admin = false;

		if (members.empty())
			return false;

		if( Security::isGroup(adminGroup) )
			group = &( Security::getGroup(adminGroup) );

		acc_sprintf("\r\n\r\n        %s%s%s\r\n",
				CCYEL(ch,C_NRM), title, CCNRM(ch,C_NRM) );
		acc_sprintf("    %so~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~%s\r\n",

            CCCYN(ch,C_NRM), CCNRM(ch,C_NRM) );
        acc_strcat("        ", NULL);

		it = members.begin();
        for( ; it != members.end(); ++it ) {
			name = playerIndex.getName(*it);
			if (!name)
				continue;
			admin = (group != NULL) && (group->member(*it));
			if(! admin )
				continue;
            acc_sprintf("%s%-15s%s",
                        admin ? CCYEL_BLD(ch,C_NRM) : "",
                        name,
                        admin ? CCNRM(ch,C_NRM) : "");
			pos++;
            if (pos > 3 ) {
                pos = 0;
                acc_strcat("\r\n        ", NULL);
            }
        }

		it = members.begin();
        for( ; it != members.end(); ++it ) {
			name = playerIndex.getName(*it);
			if (!name)
				continue;
			admin = (group != NULL) && (group->member(*it));
			if( admin )
				continue;
            acc_sprintf("%s%-15s%s",
                        admin ? CCYEL_BLD(ch,C_NRM) : "",
                        name,
                        admin ? CCNRM(ch,C_NRM) : "");
			pos++;
            if (pos > 3 ) {
                pos = 0;
                acc_strcat("\r\n        ", NULL);
            }
        }
        return true;
    }

    /* Sends a list of this group's members to the given character. */
    bool Group::sendMemberList( Creature *ch ) {
        int pos = 1;
        vector<long>::iterator it = members.begin();
        send_to_char(ch, "Members:\r\n");
        for( ; it != members.end(); ++it ) {
            send_to_char(ch,
                    "%s[%s%6ld%s] %s%-15s%s",
                    CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),
                    *it,
                    CCCYN(ch,C_NRM),
                    CCGRN(ch,C_NRM),
                    playerIndex.getName(*it),
                    CCNRM(ch,C_NRM)
                    );
            if( pos++ % 3 == 0 ) {
                pos = 1;
                send_to_char(ch, "\r\n");
            }
        }
        if( pos != 1 )
            send_to_char(ch, "\r\n");
        return true;
    }

    /* Sends a list of this group's members to the given character. */
    bool Group::sendCommandList( Creature *ch, bool prefix) {
        int pos = 1;
        vector<command_info*>::iterator it = commands.begin();
        if( prefix )
            send_to_char(ch, "Commands:\r\n");
        for( int i=1 ; it != commands.end(); ++it, ++i ) {
            send_to_char(ch,
                    "%s[%s%4d%s] %s%-15s",
                    CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),
                    i,
                    CCCYN(ch,C_NRM),
                    CCGRN(ch,C_NRM),
                    (*it)->command);
            if( pos++ % 3 == 0 ) {
                pos = 1;
                send_to_char(ch, "\r\n");
            }
        }
        if( pos != 1 )
            send_to_char(ch, "\r\n");
        send_to_char(ch, "%s", CCNRM(ch,C_NRM));
        return true;
    }

    /*
     * Makes a copy of name
     */
    Group::Group( const char *name ) : commands(), members() {
        _name = strdup(name);

        _description = strdup("No description");
        _adminGroup = NULL;
    }

    /*
     * Makes a complete copy of the Group
     */
    Group::Group( const Group &g ) : commands(), members() {
        _name = NULL;
        _description = NULL;
        _adminGroup = NULL;
        (*this) = g;
    }

    /*
     * does not make a copy of name or desc.
     */
    Group::Group( char *name, char *description ) : commands(), members() {
        _name = name;
        _description = description;
        _adminGroup = NULL;
    }

    /*
     *  Loads a group from the given xmlnode;
     *  Intended for reading from a file
     */
    Group::Group( xmlNodePtr node ) {
        // properties
        _name = xmlGetProp(node, "Name");
        _description = xmlGetProp(node, "Description");
        _adminGroup = xmlGetProp(node, "Admin");
        long member;
        char *command;
        // commands & members
        node = node->xmlChildrenNode;
        while (node != NULL) {
            if ((xmlMatches(node->name, "Member"))) {
                member = xmlGetLongProp(node, "ID");
                if( member == 0 || !playerIndex.exists(member) ) {
                    log("Invalid PID not loaded.",member);
                } else {
                    addMember(member);
                }
            }
            if ((xmlMatches(node->name, "Command"))) {
                command = xmlGetProp(node, "Name");
                int index = find_command( command );
                if( index == -1 ) {
                    trace("Group(xmlNodePtr): command not found", command);
                } else {
                    addCommand( &cmd_info[index] );
                }
				free(command);
            }
            node = node->next;
        }
    }

    /*
     * Assignment operator
     */
    Group &Group::operator=( const Group &g ) {
        // Clean out old data
        if( _name != NULL )
            free(_name);
        if( _description != NULL )
            free(_description);
        if( _adminGroup != NULL )
            free(_adminGroup);
        // Copy in new data
        _name = strdup(g._name);
        if( g._description != NULL )
            _description = strdup(g._description);
        if( g._adminGroup != NULL )
            _adminGroup = strdup(g._adminGroup);
        members = g.members;
        commands = g.commands;
        return *this;
    }

    /*
     * Create the required xmlnodes to recreate this group
     */
    bool Group::save( xmlNodePtr parent ) {
        xmlNodePtr node = NULL;

        parent = xmlNewChild( parent, NULL, (const xmlChar *)"Group", NULL );
        xmlSetProp( parent , "Name", _name );
        xmlSetProp( parent , "Description", _description );
        xmlSetProp( parent , "Admin", _adminGroup );

        vector<command_info*>::iterator cit = commands.begin();
        for( ; cit != commands.end(); ++cit ) {
            node = xmlNewChild( parent, NULL, (const xmlChar *)"Command", NULL );
            xmlSetProp( node, "Name", (*cit)->command );
        }
        vector<long>::iterator mit = members.begin();
        for( ; mit != members.end(); ++mit ) {
            node = xmlNewChild( parent, NULL, (const xmlChar *)"Member", NULL );
            const char* name = playerIndex.getName(*mit);
            if( name == NULL ) {
                log("Invalid PID not saved.",*mit);
                continue;
            }
            xmlSetProp( node, "Name", playerIndex.getName(*mit) );
            xmlSetProp( node, "ID", *mit );
        }
        return true;
    }

    /**
     * Clear out this group's data for shutdown.
     */
    void Group::clear() {
        commands.erase(commands.begin(),commands.end());
        members.erase( members.begin(), members.end() );
    }

    /*
     *
     */
    Group::~Group() {
        while( commands.begin() != commands.end() ) {
            removeCommand( *( commands.begin() ) );
        }
        members.erase( members.begin(), members.end() );

        if( _description != NULL )
            free(_description);
        _description = NULL;

        if( _name != NULL )
            free(_name);
        _name = NULL;

        if( _adminGroup != NULL )
            free(_adminGroup);
        _adminGroup = NULL;
    }
}
