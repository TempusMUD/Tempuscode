#ifndef _PROG_H_
#define _PROG_H_

#define PROG_TYPE_MOBILE		0
#define PROG_TYPE_OBJECT		1
#define PROG_TYPE_ROOM			2

class Creature;

struct prog_engine {
	struct prog_engine *next;
	char *prog;
	int linenum;
	int speed;
	int wait;
	int cmd_var;
	int owner_type;
	void *owner;
	Creature *target;
};

void destroy_attached_progs(void *self);
bool trigger_prog_cmd(Creature *self, Creature *ch, int cmd, char *argument);
void trigger_prog_idle(Creature *self);
void prog_start(int owner_type, void *owner, Creature *target, char *prog, int linenum);
void prog_free(struct prog_engine *prog);
void prog_update(void);

#endif
