#ifndef _SECURITY_H_
#define _SECURITY_H_

// Special roles
extern const char *ROLE_EVERYONE;
extern const char *ROLE_NOONE;

extern const char *ROLE_ADMINBASIC;
extern const char *ROLE_ADMINFULL;
extern const char *ROLE_CLAN;
extern const char *ROLE_CLANADMIN;
extern const char *ROLE_CODER;
extern const char *ROLE_CODERADMIN;
extern const char *ROLE_DYNEDIT;
extern const char *ROLE_ROLESADMIN;
extern const char *ROLE_HELP;
extern const char *ROLE_HOUSE;
extern const char *ROLE_OLC;
extern const char *ROLE_OLCADMIN;
extern const char *ROLE_OLCAPPROVAL;
extern const char *ROLE_OLCPROOFER;
extern const char *ROLE_OLCWORLDWRITE;
extern const char *ROLE_QUESTOR;
extern const char *ROLE_QUESTORADMIN;
extern const char *ROLE_TESTERS;
extern const char *ROLE_WIZARDADMIN;
extern const char *ROLE_WIZARDBASIC;
extern const char *ROLE_WIZARDFULL;
extern const char *ROLE_WORLDADMIN;

struct role {
        /* A one line description of this role */
    char *description;
    /* The one word name for this role */
    char *name;
    /* The name of the role whose members can administrate this role */
    char *admin_role;

    int id;
    /**
     * resolved on role load/creation.
     * Command names stored in file.
     * Pointers sorted as if they're ints.
     **/
    GList *commands;
    // player ids, sorted
    GList *members;
};

extern GList *roles;

enum privilege {
    CREATE_ROLE,
    DESTROY_ROLE,
    EDIT_ROLE,
    CREATE_CLAN,
    DESTROY_CLAN,
    EDIT_CLAN,
    SEE_FULL_WHOLIST,
    FULL_IMMORT_WHERE,
    HEAR_ALL_CHANNELS,
    EAT_ANYTHING,
    LIST_SEARCHES,
    SET_RESERVED_SPECIALS,
    SET_FULLCONTROL,
    WORLDWRITE,
    ECONVERT_CORPSES,
    LOGIN_WITH_TESTER,
    ENTER_HOUSES,
    PARDON,
    MULTIPLAY,
    OLC_LOCK,
    TESTER,
    ENTER_ROOM,
    CREATE_ZONE,
    EDIT_ZONE,
    APPROVE_ZONE,
    UNAPPROVE_ZONE,
    EDIT_HOUSE,
    EDIT_QUEST,
    QUEST_BAN,
    CONTROL_FATE,
    EDIT_HELP,
    COMMAND,
    SHOW,
    SET,
    ASET,
};

bool load_roles_from_db(void);
bool is_authorized(struct creature *ch, enum privilege priv, void *target);
void send_role_linedesc(struct role *role,struct creature *ch);
bool is_role_member(struct role *role, long player);
bool is_role_command(struct role *role, const struct command_info *command);
struct role *make_role(const char *name,
                       const char *description,
                       const char *admin_role);
bool add_role_command(struct role *role, struct command_info *command);
bool add_role_member(struct role *role, long player);
void set_role_admin_role(struct role *role, const char *role_name);
void set_role_description(struct role *role, const char *desc);
bool send_role_members(struct role *role, struct creature *ch);
bool send_role_commands(struct role *role, struct creature *ch, bool prefix);
bool remove_role_command(struct role *role, struct command_info *command);
bool remove_role_member(struct role *role, long player);
void send_role_status(struct role *role, struct creature *ch);
void free_role(struct role *role);
struct role *role_by_name(const char *name);
void send_role_member_list(struct role *role,
                           struct creature *ch,
                           const char *title,
                           const char *admin_role_name);
bool is_tester(struct creature *ch);

#endif
