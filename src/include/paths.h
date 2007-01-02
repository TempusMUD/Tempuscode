#ifndef _PATHS_H_
#define _PATHS_H_

//
// File: paths.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define PATH_FILE "etc/paths"

#define PATH_WAIT  0
#define PATH_ROOM  1
#define PATH_DIR   2
#define PATH_CMD   3
#define PATH_EXIT  4
#define PATH_ECHO  5

typedef struct {
	int type;
	int data;
	char *str;
} Path;

#define PATH_LOCKED      (1 << 0)
#define PATH_REVERSIBLE  (1 << 1)
#define PATH_SAVE        (1 << 2)

typedef struct {
	int number;
	char name[64];
	long owner;
	int wait_time;
	int flags;
	int length;
	unsigned int find_first_step_calls;
	Path *path;
	void *next;
} PHead;

#define POBJECT_STALLED  (1 << 0)

#define PMOBILE  1
#define PVEHICLE 2
typedef struct {
	int type;
	int wait_time;
	int time;
	int pos;
	int flags;
	int step;
	void *object;
	PHead *phead;
} PObject;

PHead *real_path(char *str);
void show_path(struct Creature *ch, char *arg);
void show_pathobjs(struct Creature *ch);
void print_path(PHead * phead, char *str);
int add_path_to_vehicle(struct obj_data *obj, char *name);
PHead *real_path_by_num(int vnum);
void path_remove_object(void *object);
int add_path_to_mob(struct Creature *mob, char *name);
int add_path(char *spath, int save);

#endif
