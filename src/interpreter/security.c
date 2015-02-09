#ifdef HAS_CONFIG_H
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "players.h"
#include "tmpstr.h"
#include "quest.h"
#include "strutil.h"

/**
 *
 * The command struct for access commands
 *  (with 'pissers' being a role)
 *
 * access list
 * access create pissers
 * access remove pissers
 * access description pissers The weirdos that piss a lot
 * access adminrole pissers PisserAdmins
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
    const char *command;
    const char *usage;
} access_cmds[] = {
    {
        "addmember", "<role name> <member> [<member>...]"
    }, {
        "addcmd", "<role name> <command> [<command>...]"
    }, {
        "admin", "<role name> <admin role>"
    }, {
        "cmdlist", "<role name>"
    }, {
        "create", "<role name>"
    }, {
        "describe", "<role name> <description>"
    }, {
        "rolelist", "<player name>"
    }, {
        "list", ""
    }, {
        "load", ""
    }, {
        "memberlist", "<role name>"
    }, {
        "remmember", "<role name> <member> [<member>...]"
    }, {
        "remcmd", "<role name> <command> [<command>...]"
    }, {
        "destroy", "<role name>"
    }, {
        "stat", "<role name>"
    }, {
        NULL, NULL
    }
};

static char out_buf[MAX_STRING_LENGTH + 2];

const char *ROLE_EVERYONE = "<ALL>";
const char *ROLE_NOONE = "<NONE>";

const char *ROLE_ADMINBASIC = "AdminBasic";
const char *ROLE_ADMINFULL = "AdminFull";
const char *ROLE_CLAN = "Clan";
const char *ROLE_CLANADMIN = "ClanAdmin";
const char *ROLE_CODER = "Coder";
const char *ROLE_CODERADMIN = "CoderAdmin";
const char *ROLE_DYNEDIT = "Dynedit";
const char *ROLE_ROLESADMIN = "RolesAdmin";
const char *ROLE_HELP = "Help";
const char *ROLE_HOUSE = "House";
const char *ROLE_OLC = "OLC";
const char *ROLE_OLCADMIN = "OLCAdmin";
const char *ROLE_OLCAPPROVAL = "OLCApproval";
const char *ROLE_OLCPROOFER = "OLCProofer";
const char *ROLE_OLCWORLDWRITE = "OLCWorldWrite";
const char *ROLE_QUESTOR = "Questor";
const char *ROLE_QUESTORADMIN = "QuestorAdmin";
const char *ROLE_TESTERS = "Testers";
const char *ROLE_WIZARDADMIN = "WizardAdmin";
const char *ROLE_WIZARDBASIC = "WizardBasic";
const char *ROLE_WIZARDFULL = "WizardFull";
const char *ROLE_WORLDADMIN = "WorldAdmin";

/**
 *  Sends usage info to the given character
 */
void
send_access_options(struct creature *ch)
{
    int i = 0;
    strcpy_s(out_buf, sizeof(out_buf), "access usage :\r\n");
    while (1) {
        if (!access_cmds[i].command) {
            break;
        }
        snprintf_cat(out_buf, sizeof(out_buf), "  %-15s %s\r\n", access_cmds[i].command,
                     access_cmds[i].usage);
        i++;
    }
    page_string(ch->desc, out_buf);
}

/**
 *  Find the proper 'access' subcommand
 */
int
find_access_command(char *command)
{
    if (command == NULL || *command == '\0') {
        return -1;
    }
    for (int i = 0; access_cmds[i].command != NULL; i++) {
        if (strncmp(access_cmds[i].command, command, strlen(command)) == 0) {
            return i;
        }
    }
    return -1;
}

struct role *role_by_name(const char *name);
bool authorized_to_edit_role(struct creature *ch, struct role *role);

bool
is_named_role_member(struct creature *ch, const char *role_name)
{
    struct role *role;

    if (IS_NPC(ch)) {
        return false;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
        return false;
    }

    role = role_by_name(role_name);
    if (!role) {
        return false;
    }
    return is_role_member(role, GET_IDNUM(ch));
}

bool
is_authorized(struct creature *ch, enum privilege priv, void *target)
{
    struct role *role = target;
    struct command_info *command = target;
    struct show_struct *show_cmd = target;
    struct set_struct *set_cmd = target;
    struct zone_data *zone = target;
    struct house *house = target;
    struct room_data *room = target;
    struct quest *quest = target;

    if (GET_LEVEL(ch) == LVL_GRIMP) {
        return true;
    }

    switch (priv) {
    case CREATE_ROLE:
    case DESTROY_ROLE:
        return is_named_role_member(ch, "RolesAdmin");

    case EDIT_ROLE:
        return is_named_role_member(ch, "RolesAdmin")
               || is_named_role_member(ch, role->admin_role);

    case CREATE_CLAN:
    case EDIT_CLAN:
    case DESTROY_CLAN:
        return is_named_role_member(ch, "Clan");

    case LOGIN_WITH_TESTER:
        return is_named_role_member(ch, "OLC");

    case CONTROL_FATE:
    case DESTROY_NOTAKE:
    case EMPTY_CORPSES:
    case SNOOP_IN_GODROOMS:
        return is_named_role_member(ch, "WizardFull");

    case STAT_FATE:
    case STAT_PLAYERS:
    case STAT_PLAYER_FILE:
    case FORCE_PLAYERS:
        return is_named_role_member(ch, "AdminFull");

    case EDIT_HELP:
        return is_named_role_member(ch, "Help");

    case SEE_FULL_WHOLIST:
        return is_named_role_member(ch, "AdminBasic");

    case FULL_IMMORT_WHERE:
        return is_named_role_member(ch, "Questor")
               || is_named_role_member(ch, "AdminBasic")
               || is_named_role_member(ch, "WizardBasic");

    case TESTER:
        return is_named_role_member(ch, "Testers");

    case DEBUGGING:
    case FORCE_ALL:
        return is_named_role_member(ch, "Coder");

    case ENTER_HOUSES:
        return (is_named_role_member(ch, "House")
                || is_named_role_member(ch, "AdminBasic")
                || is_named_role_member(ch, "WizardFull"));

    case ENTER_ROOM:
        return (can_enter_house(ch, room->number)
                && clan_house_can_enter(ch, room)
                && (!ROOM_FLAGGED(room, ROOM_GODROOM)
                    || is_named_role_member(ch, "WizardFull")));

    case WORLDWRITE:
    case SET_FULLCONTROL:
    case SET_RESERVED_SPECIALS:
        return is_named_role_member(ch, "OLCWorldWrite");

    case HEAR_ALL_CHANNELS:
    case PARDON:
        return is_named_role_member(ch, "AdminBasic");

    case DESTROY_CORPSES:
    case EAT_ANYTHING:
        return is_named_role_member(ch, "WizardBasic");

    case MULTIPLAY:
        return (is_named_role_member(ch, "WizardFull")
                || is_named_role_member(ch, "AdminFull"));

    case LIST_SEARCHES:
        if (IS_NPC(ch)) {
            return false;
        }
        return (zone->owner_idnum == GET_IDNUM(ch)
                || zone->co_owner_idnum == GET_IDNUM(ch)
                || is_named_role_member(ch, "OLCWorldWrite"));

    case EDIT_ZONE:
        if (IS_NPC(ch)) {
            return false;
        }
        return (zone->owner_idnum == GET_IDNUM(ch)
                || zone->co_owner_idnum == GET_IDNUM(ch)
                || (is_named_role_member(ch, "OLCProofer") && !IS_APPR(zone))
                || (is_named_role_member(ch, "OLCWorldWrite")
                    && PRF2_FLAGGED(ch, PRF2_WORLDWRITE)));

    case CREATE_ZONE:
    case APPROVE_ZONE:
    case UNAPPROVE_ZONE:
    case OLC_LOCK:
        return is_named_role_member(ch, "OLCApproval");

    case EDIT_HOUSE:
        if (IS_NPC(ch)) {
            return false;
        }
        return (is_named_role_member(ch, "House")
                || (house && house->owner_id == ch->account->id));

    case QUEST_BAN:
    case EDIT_QUEST:
        return GET_IDNUM(ch) == quest->owner_id
               || is_named_role_member(ch, "Questor");

    case COMMAND:
        if (GET_LEVEL(ch) < command->minimum_level) {
            return false;
        }

        if (!command->role_count) {
            return true;
        }

        if (IS_PC(ch)) {
            for (GList *it = roles; it; it = it->next) {
                struct role *role = (struct role *)it->data;
                if (is_role_command(role, command)
                    && is_role_member(role, GET_IDNUM(ch))) {
                    return true;
                }
            }
        }
        return false;

    case SET:
        if (IS_NPC(ch)) {
            return false;
        }
        if (set_cmd->level > GET_LEVEL(ch)) {
            return false;
        }
        if (*(set_cmd->role) == '\0') {
            return true;
        }
        if (set_cmd->role == ROLE_EVERYONE) {
            return true;
        }
        if (is_named_role_member(ch, set_cmd->role)) {
            return true;
        }
        return false;

    case SHOW:
        if (IS_NPC(ch)) {
            return false;
        }
        if (show_cmd->level > GET_LEVEL(ch)) {
            return false;
        }
        if (*(show_cmd->role) == '\0') {
            return true;
        }
        if (show_cmd->role == ROLE_EVERYONE) {
            return true;
        }
        if (is_named_role_member(ch, show_cmd->role)) {
            return true;
        }
        return false;

    case ASET:
        if (IS_NPC(ch)) {
            return false;
        }
        if (set_cmd->level > GET_LEVEL(ch)) {
            return false;
        }
        if (*(set_cmd->role) == '\0') {
            return true;
        }
        if (set_cmd->role == ROLE_EVERYONE) {
            return true;
        }
        if (is_named_role_member(ch, set_cmd->role)) {
            return true;
        }
    }

    return false;
}

struct role *
acquire_role_for_edit(struct creature *ch, char *name)
{
    struct role *role;

    if (!name) {
        send_to_char(ch, "What role do you wish to edit?\r\n");
        return NULL;
    }
    role = role_by_name(name);
    if (!role) {
        send_to_char(ch, "That's not actually a role.\r\n");
        return NULL;
    }

    if (!is_authorized(ch, EDIT_ROLE, role)) {
        send_to_char(ch, "You cannot alter this role.\r\n");
        return NULL;
    }

    return role;
}

/*
 * send a list of the current roles to a character
 */
void
send_role_list(struct creature *ch)
{
    const char *nrm = CCNRM(ch, C_NRM);
    const char *cyn = CCCYN(ch, C_NRM);
    const char *grn = CCGRN(ch, C_NRM);

    send_to_char(ch,
                 "%s%15s %s[%scmds%s] [%smbrs%s]%s - Description \r\n",
                 grn, "Role", cyn, nrm, cyn, nrm, cyn, nrm);


    for (GList *it = roles; it; it = it->next) {
        struct role *role = (struct role *)it->data;
        send_role_linedesc(role, ch);
    }
}

bool
destroy_role(struct role *role)
{
    sql_exec("update sgroups set admin=NULL where admin=%d", role->id);
    sql_exec("delete from sgroup_members where sgroup=%d", role->id);
    sql_exec("delete from sgroup_commands where sgroup=%d", role->id);
    sql_exec("delete from sgroups where idnum=%d", role->id);

    roles = g_list_remove(roles, role);
    free_role(role);
    return true;
}

void
send_role_membership(struct creature *ch, long id)
{
    int n = 0;

    for (GList *it = roles; it; it = it->next) {
        struct role *role = (struct role *)it->data;
        if (is_role_member(role, id)) {
            send_role_linedesc(role, ch);
            n++;
        }
    }

    if (!n) {
        send_to_char(ch, "That player is not in any roles.\r\n");
    }
}

/* sends a list of the commands a char has access to and the
 * roles that contain them.
 **/
bool
send_available_commands(struct creature *ch, long id)
{
    int n = 0;

    for (GList *it = roles; it; it = it->next) {
        struct role *role = (struct role *)it->data;
        if (is_role_member(role, id) && role->commands) {
            ++n;
            send_to_char(ch, "%s%s%s\r\n", CCYEL(ch, C_NRM),
                         role->name, CCNRM(ch, C_NRM));
            send_role_commands(role, ch, NULL);
        }
    }
    if (n <= 0) {
        send_to_char(ch, "That player is not in any roles.\r\n");
        return false;
    }
    return true;
}

/**
 * clears security related memory and shuts the system down.
 */
void
free_roles(void)
{
    g_list_foreach(roles, (GFunc) free_role, NULL);
    g_list_free(roles);
    roles = NULL;
}

bool
load_roles_from_db(void)
{
    PGresult *res;
    int idx, count, cmd;
    struct role *role;

    free_roles();

    res =
        sql_query
            ("select sgroups.idnum, sgroups.name, sgroups.descrip, admin.name from sgroups left outer join sgroups as admin on admin.idnum=sgroups.admin order by sgroups.name");
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++) {
        role = make_role(PQgetvalue(res, idx, 1),
                         PQgetvalue(res, idx, 2), PQgetvalue(res, idx, 3));
        role->id = atoi(PQgetvalue(res, idx, 0));
        roles = g_list_prepend(roles, role);
    }
    roles = g_list_reverse(roles);

    res =
        sql_query
            ("select name, command from sgroups, sgroup_commands where sgroups.idnum=sgroup_commands.sgroup order by command");
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++) {
        cmd = find_command(PQgetvalue(res, idx, 1));
        if (cmd >= 0) {
            role = role_by_name(PQgetvalue(res, idx, 0));
            if (role) {
                add_role_command(role, &cmd_info[cmd]);
            } else {
                errlog("Invalid security role '%s' using command '%s'",
                       PQgetvalue(res, idx, 0), PQgetvalue(res, idx, 1));
            }
        } else {
            errlog("Invalid command '%s' in security role '%s'",
                   PQgetvalue(res, idx, 1), PQgetvalue(res, idx, 0));
        }
    }

    res =
        sql_query
            ("select name, player from sgroups, sgroup_members where sgroups.idnum=sgroup_members.sgroup order by player");
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++) {
        role = role_by_name(PQgetvalue(res, idx, 0));
        if (role) {
            add_role_member(role, atol(PQgetvalue(res, idx, 1)));
        } else {
            errlog("Invalid security role '%s' with member %s",
                   PQgetvalue(res, idx, 0), PQgetvalue(res, idx, 1));
        }
    }

    slog("Security:  Access role data loaded.");
    return true;
}

/**
 *  The access command itself.
 *  Used as an interface for the modification of security settings
 *  from within the mud.
 */
ACMD(do_access)
{
    struct role *role, *admin_role;
    char *token;

    if (argument == NULL || *argument == '\0') {
        send_access_options(ch);
        return;
    }

    token = tmp_getword(&argument);

    switch (find_access_command(token)) {
    case 0:                    // addmember
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Add what member?\r\n");
            return;
        }
        do {
            int player_id;
            player_id = player_idnum_by_name(token);
            if (player_id) {
                add_role_member(role, player_id);
                sql_exec("insert into sgroup_members (sgroup, player) "
                         "values (%d, %d)", role->id, player_id);

                send_to_char(ch, "Member added : %s\r\n", token);
                slog("Security:  %s added to role '%s' by %s.",
                     token, role->name, GET_NAME(ch));
            } else {
                send_to_char(ch, "Player doesn't exist: %s\r\n", token);
            }
            token = tmp_getword(&argument);
        } while (*token);

        break;
    case 1:                    // addcmd
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Add what command?\r\n");
            return;
        }
        do {
            int command_idx;
            command_idx = find_command(token);
            if (command_idx != -1) {
                add_role_command(role, &cmd_info[command_idx]);
                send_to_char(ch, "Command added : %s\r\n",
                             cmd_info[command_idx].command);
                sql_exec("insert into sgroup_commands (sgroup, command) "
                         "values (%d, '%s')",
                         role->id, cmd_info[command_idx].command);

                slog("Security:  command %s added to role '%s' by %s.",
                     cmd_info[command_idx].command, role->name, GET_NAME(ch));
            } else {
                send_to_char(ch, "Invalid command: %s\r\n", token);
            }
            token = tmp_getword(&argument);
        } while (*token);
        break;
    case 2:                    // Admin
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        token = tmp_getword(&argument);
        admin_role = role_by_name(token);
        if (!admin_role) {
            send_to_char(ch, "The admin role you specified isn't a role.\r\n");
            return;
        }

        set_role_admin_role(role, token);
        sql_exec("update sgroups set admin=%d where idnum=%d",
                 role->id, admin_role->id);
        send_to_char(ch, "Administrative role set.\r\n");
        break;
    case 3:                    // cmdlist
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "List commands for what role?\r\n");
            return;
        }
        role = role_by_name(token);
        if (!role) {
            send_to_char(ch, "That isn't a role.\r\n");
            return;
        }

        send_role_commands(role, ch, NULL);
        break;
    case 4:                    // create
        if (!is_authorized(ch, CREATE_ROLE, NULL)) {
            send_to_char(ch, "You cannot create roles.\r\n");
            return;
        }
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Create a role with what name?\r\n");
            return;
        }
        if (role_by_name(token)) {
            send_to_char(ch, "That role already exists.\r\n");
            return;
        }
        role = make_role(token, NULL, NULL);
        if (role) {
            PGresult *res;
            int role_id;

            res = sql_query("select MAX(idnum) from sgroups");
            role_id = atoi(PQgetvalue(res, 0, 0)) + 1;

            role->id = role_id;

            sql_exec("insert into sgroups (idnum, name, descrip) "
                     "values (%d, '%s', 'No description.')", role_id, token);

            roles = g_list_prepend(roles, role);

            send_to_char(ch, "Role created.\r\n");
            slog("Security:  Role '%s' created by %s.", token, GET_NAME(ch));
        } else {
            send_to_char(ch, "Role creation failed.\r\n");
        }
        break;
    case 5:                    // Describe
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        if (!*argument) {
            send_to_char(ch, "No description given.\r\n");
            return;
        }
        set_role_description(role, argument);
        send_to_char(ch, "Description set.\r\n");
        sql_exec("update sgroups set descrip='%s' where idnum=%d",
                 tmp_sqlescape(argument), role->id);
        slog("Security:  Role '%s' described by %s.", token, GET_NAME(ch));
        break;
    case 6:                    // rolelist
        token = tmp_getword(&argument);
        if (token) {
            long id = player_idnum_by_name(token);
            if (id <= 0) {
                send_to_char(ch, "No such player.\r\n");
                return;
            }
            send_to_char(ch, "%s is a member of the following roles:\r\n",
                         token);
            send_role_membership(ch, id);
        } else {
            send_to_char(ch, "You are a member of the following roles:\r\n");
            send_role_membership(ch, GET_IDNUM(ch));
        }
        break;
    case 7:                    // list
        send_role_list(ch);
        break;
    case 8:                    // Load
        if (load_roles_from_db()) {
            send_to_char(ch, "Access roles loaded.\r\n");
        } else {
            send_to_char(ch, "Error loading access roles.\r\n");
        }
        break;
    case 9:                    // memberlist
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "List members for what role?\r\n");
            return;
        }
        role = role_by_name(token);
        if (!role) {
            send_to_char(ch, "No such role.\r\n");
            return;
        }
        send_role_members(role, ch);

        break;
    case 10:                   // remmember
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Remove which members?\r\n");
            return;
        }
        do {
            int player_id;
            player_id = player_idnum_by_name(token);
            if (player_id) {
                remove_role_member(role, player_id);
                sql_exec("delete from sgroup_members "
                         "where sgroup=%d and player=%d", role->id, player_id);

                send_to_char(ch, "Member removed : %s\r\n", token);
                slog("Security:  %s removed from role '%s' by %s.",
                     token, role->name, GET_NAME(ch));
            } else {
                send_to_char(ch, "Player doesn't exist: %s\r\n", token);
            }
            token = tmp_getword(&argument);
        } while (*token);
        break;
    case 11:                   // remcmd
        token = tmp_getword(&argument);
        role = acquire_role_for_edit(ch, token);
        if (!role) {
            return;
        }

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Remove which command?\r\n");
            return;
        }
        do {
            int command_idx;
            command_idx = find_command(token);
            if (command_idx != -1) {
                add_role_command(role, &cmd_info[command_idx]);
                send_to_char(ch, "Command removed : %s\r\n",
                             cmd_info[command_idx].command);
                sql_exec("delete from sgroup_commands "
                         "where sgroup = %d and command='%s') ",
                         role->id, cmd_info[command_idx].command);

                slog("Security:  command %s removed from role '%s' by %s.",
                     cmd_info[command_idx].command, role->name, GET_NAME(ch));
            } else {
                send_to_char(ch, "Invalid command: %s\r\n", token);
            }
            token = tmp_getword(&argument);
        } while (*token);
        break;
    case 12:                   // destroy
        if (!is_authorized(ch, DESTROY_ROLE, NULL)) {
            send_to_char(ch, "You cannot destroy roles.\r\n");
            return;
        }
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Destroy which role?\r\n");
            return;
        }
        role = role_by_name(token);
        if (!role) {
            send_to_char(ch, "That's not a role.\r\n");
            return;
        }
        destroy_role(role);
        break;
    case 13:                   // Stat
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Destroy which role?\r\n");
            return;
        }
        role = role_by_name(token);
        if (!role) {
            send_to_char(ch, "That's not a role.\r\n");
            return;
        }

        send_role_status(role, ch);
        break;
    default:
        send_access_options(ch);
        break;
    }
}
