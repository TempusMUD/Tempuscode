#ifndef _SECURITY_H_
#define _SECURITY_H_

// Special roles
extern const char *SECURITY_EVERYONE;
extern const char *SECURITY_NOONE;

extern const char *SECURITY_ADMINBASIC;
extern const char *SECURITY_ADMINFULL;
extern const char *SECURITY_CLAN;
extern const char *SECURITY_CLANADMIN;
extern const char *SECURITY_CODER;
extern const char *SECURITY_CODERADMIN;
extern const char *SECURITY_DYNEDIT;
extern const char *SECURITY_ROLESADMIN;
extern const char *SECURITY_HELP;
extern const char *SECURITY_HOUSE;
extern const char *SECURITY_OLC;
extern const char *SECURITY_OLCADMIN;
extern const char *SECURITY_OLCAPPROVAL;
extern const char *SECURITY_OLCPROOFER;
extern const char *SECURITY_OLCWORLDWRITE;
extern const char *SECURITY_QUESTOR;
extern const char *SECURITY_QUESTORADMIN;
extern const char *SECURITY_TESTERS;
extern const char *SECURITY_WIZARDADMIN;
extern const char *SECURITY_WIZARDBASIC;
extern const char *SECURITY_WIZARDFULL;
extern const char *SECURITY_WORLDADMIN;

/**  Constant Command Flags **/
const int SECURITY_ROLE = (1 << 0);

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

GList *roles;

enum privilege {
    CREATE_ROLE,
    DESTROY_ROLE,
    EDIT_ROLE,
    CREATE_CLAN,
    DESTROY_CLAN,
    EDIT_CLAN,
    SEE_FULL_WHOLIST,
    FULL_IMMORT_WHERE,
    ENTER_GODROOM,
    COMMAND,
    SHOW,
    SET,
    ASET,
};

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

#endif
