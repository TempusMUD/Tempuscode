#ifndef _PROG_H_
#define _PROG_H_

class Creature;

enum prog_evt_type {
	PROG_TYPE_NONE,
	PROG_TYPE_MOBILE,
	PROG_TYPE_OBJECT,
	PROG_TYPE_ROOM
};

enum prog_evt_phase {
	PROG_EVT_BEGIN,
	PROG_EVT_HANDLE,
	PROG_EVT_AFTER
};

enum prog_evt_kind {
	PROG_EVT_COMMAND,
	PROG_EVT_IDLE,
	PROG_EVT_FIGHT,
	PROG_EVT_GIVE
};

struct prog_evt {
	prog_evt_phase phase;
	prog_evt_kind kind;
	int cmd;
	char *args;

	Creature *subject;
	void *object;
	int object_type;
};

struct prog_var {
	struct prog_var *next;
	char *key;
	char *value;
};

struct prog_state_data {
	struct prog_var *var_list;
};

struct prog_env {
	struct prog_env *next;	// next prog environment
	int exec_pt;				// the line number we're executing
	int executed;				// the number of non-handlers we've executed
	int speed;					// default wait between commands
	int wait;					// the number of seconds to wait
	int owner_type;				// type of the owner
	void *owner;				// pointer to the actual owner
	Creature *target;			// target of prog
	prog_evt evt;				// copy of event that caused prog to trigger
	prog_state_data *state;		// state record of owner
};

void destroy_attached_progs(void *self);
bool trigger_prog_cmd(Creature *self, Creature *ch, int cmd, char *argument);
void trigger_progs_after(Creature *ch, int cmd, char *argument);
void trigger_prog_idle(Creature *self);
void trigger_prog_fight(Creature *ch, Creature *self);
prog_env *prog_start(int owner_type, void *owner, Creature *target, char *prog, prog_evt *evt);
void prog_free(struct prog_env *prog);
void prog_update(void);
void prog_update_pending(void);
int prog_count(void);
void prog_state_free(prog_state_data *state);

#endif
