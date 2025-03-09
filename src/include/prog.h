#ifndef _PROG_H_
#define _PROG_H_

struct creature;

typedef unsigned long prog_word_t;

enum prog_evt_type {
    PROG_TYPE_NONE,
    PROG_TYPE_MOBILE,
    PROG_TYPE_OBJECT,
    PROG_TYPE_ROOM
};

enum prog_evt_phase {
    PROG_EVT_BEGIN,
    PROG_EVT_HANDLE,
    PROG_EVT_AFTER,
    PROG_PHASE_COUNT,
};

enum prog_evt_kind {
    PROG_EVT_COMMAND,
    PROG_EVT_IDLE,
    PROG_EVT_FIGHT,
    PROG_EVT_GIVE,
    PROG_EVT_CHAT,
    PROG_EVT_ENTER,
    PROG_EVT_LEAVE,
    PROG_EVT_LOAD,
    PROG_EVT_TICK,
    PROG_EVT_SPELL,
    PROG_EVT_COMBAT,
    PROG_EVT_DEATH,
    PROG_EVT_DYING,
    PROG_EVT_COUNT,             /* Maximum event number + 1 = count */
};

enum prog_cmd_kind {
    PROG_CMD_ENDOFPROG,
    PROG_CMD_HALT,
    PROG_CMD_RESUME,
    PROG_CMD_BEFORE,
    PROG_CMD_HANDLE,
    PROG_CMD_AFTER,
    PROG_CMD_OR,
    PROG_CMD_DO,
    PROG_CMD_CLRCOND,
    PROG_CMD_CMPCMD,
    PROG_CMD_CMPOBJVNUM,
    PROG_CMD_CONDNEXTHANDLER,
};

struct prog_evt {
    enum prog_evt_phase phase;
    enum prog_evt_kind kind;
    int cmd;
    char args[MAX_INPUT_LENGTH];

    struct creature *subject;
    void *object;
    int object_type;
};

struct prog_var {
    struct prog_var *next;
    char key[255];
    char value[255];
};

struct prog_state_data {
    struct prog_var *var_list;
};

struct prog_env {
    int exec_pt;                // the line number we're executing
    int executed;               // the number of non-handlers we've executed
    int speed;                  // default wait between commands
    int next_tick;              // the tick number to continue execution
    int condition;              // T/F depending on last compare
    enum prog_evt_type owner_type;  // type of the owner
    void *owner;                // pointer to the actual owner
    struct creature *target;            // target of prog
    struct prog_evt evt;                // copy of event that caused prog to trigger
    bool tracing;               // prog is being traced
    struct prog_state_data *state; // thread-local state
};

struct prog_command {
    const char *str;
    bool count;
    void (*func)(struct prog_env *, struct prog_evt *, char *);
};

extern struct prog_command prog_cmds[];

void destroy_attached_progs(void *self);
bool trigger_prog_cmd(void *owner, enum prog_evt_type owner_type, struct creature *ch, int cmd, char *argument);
bool trigger_prog_spell(void *owner, enum prog_evt_type owner_type, struct creature *ch, int cmd);
bool trigger_prog_move(void *owner, enum prog_evt_type owner_type, struct creature *ch, enum special_mode mode);
void trigger_prog_idle(void *owner, enum prog_evt_type owner_type);
void trigger_prog_combat(void *owner, enum prog_evt_type owner_type);
void trigger_prog_tick(void *owner, enum prog_evt_type owner_type);
void trigger_prog_room_load(struct room_data *self);
void trigger_prog_load(struct creature *self);
void trigger_prog_fight(struct creature *ch, struct creature *self);
void trigger_prog_give(struct creature *ch, struct creature *self, struct obj_data *obj);
bool trigger_prog_chat(struct creature *ch, struct creature *self);
void trigger_prog_dying(struct creature *owner, struct creature *killer);
void trigger_prog_death(void *owner, enum prog_evt_type owner_type, struct creature *doomed);
struct prog_env *prog_start(enum prog_evt_type owner_type, void *owner, struct creature *target, struct prog_evt *evt);
void prog_update(void);
void prog_update_pending(void);
size_t prog_count(bool total);
size_t free_prog_count(void);
void prog_state_free(struct prog_state_data *state);
void prog_compile(struct creature *ch, void *owner, enum prog_evt_type owner_type);
char *prog_get_text(void *owner, enum prog_evt_type owner_type);
void prog_unreference_object(struct obj_data *obj);

#endif
