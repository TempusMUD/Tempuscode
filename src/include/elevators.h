#ifndef __elevator_h__
#define __elevator_h__
//
// File: elevators.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "paths.h"

#define ELEVATOR_FILE "etc/elevators"

typedef struct path_Q_link {
	PHead *phead;
	struct path_Q_link *next;
} path_Q_link;

struct elevator_elem {
	int rm_vnum;
	int key;
	char *name;
	struct elevator_elem *next;
};

struct elevator_data {
	int vnum;
	struct elevator_elem *list;
	path_Q_link *pQ;
	struct elevator_data *next;
};

struct elevator_data *real_elevator(int vnum);
int path_to_elevator_queue(int vnum, struct elevator_data *elev);
struct elevator_elem *elevator_elem_by_name(char *str,
	struct elevator_data *elev);
int target_room_on_queue(int vnum, struct elevator_data *elev);
int handle_elevator_position(struct obj_data *car, struct room_data *targ_rm);

#ifdef __elevators_c__

struct elevator_data *elevators = NULL;

#else

extern struct elevator_data *elevators;

#endif

#endif
