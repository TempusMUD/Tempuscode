//
// File: paths.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "vehicle.h"
#include "interpreter.h"
#include "handler.h"
#include "paths.h"
#include "comm.h"
#include "screen.h"
#include "player_table.h"

const char *PATH_FILE = "etc/paths";

const int PATH_WAIT = 0;
const int PATH_ROOM = 1;
const int PATH_DIR = 2;
const int PATH_CMD = 3;
const int PATH_EXIT = 4;
const int PATH_ECHO = 5;

const int PATH_LOCKED = (1 << 0);
const int PATH_REVERSIBLE = (1 << 1);
const int PATH_SAVE = (1 << 2);

const int POBJECT_STALLED = (1 << 0);

const int PMOBILE = 1;
const int PVEHICLE = 2;

struct Link {
	int type;
	int flags;
	void *object;
	struct Link *prev;
	struct Link *next;
};

struct Path {
	int type;
	int data;
	char *str;
};

struct PHead {
	int number;
	char name[64];
	long owner;
	int wait_time;
	int flags;
	int length;
	unsigned int find_first_step_calls;
	Path *path;
	struct PHead *next;
};

struct PObject {
	int type;
	int wait_time;
	int time;
	int pos;
	int flags;
	int step;
	thing *object;
	PHead *phead;
};

PHead *first_path = NULL;
Link *path_object_list = NULL;
Link *path_command_list = NULL;
int path_command_length = 0;
int path_locked = 1;

int move_car(struct Creature *ch, struct obj_data *car, int dir);

inline void
PATH_MOVE(PObject *o)
{
	o->pos += o->step;

	if (o->pos >= o->phead->length) {
		if (IS_SET(o->phead->flags, PATH_REVERSIBLE)) {
			o->pos--;
			o->step = -1;
		} else
			o->pos = 0;
	}

	if (o->pos < 0) {
		o->pos = 0;
		o->step = 1;
	}
}

PHead *
real_path(const char *str)
{
	PHead *path_head = NULL;

	if (!str)
		return (NULL);

	for (path_head = first_path; path_head;
		path_head = (PHead *) path_head->next)
		if (isname(str, path_head->name))
			return (path_head);

	return (NULL);
}

PHead *
real_path_by_num(int vnum)
{
	PHead *path_head = NULL;

	for (path_head = first_path; path_head;
		path_head = (PHead *) path_head->next)
		if (path_head->number == vnum)
			return (path_head);

	return (NULL);
}

bool
path_vnum_exists(int vnum)
{
    return real_path_by_num(vnum) != NULL;
}

bool
path_name_exists(const char *name)
{
    return real_path(name) != NULL;
}

int
path_vnum_by_name(const char *name)
{
    PHead *phead = real_path(name);
    return (phead) ? phead->number:0;
}

char *
path_name_by_vnum(int vnum)
{
    PHead *phead = real_path_by_num(vnum);
    return (phead) ? tmp_strdup(phead->name):0;
}

void
show_pathobjs(struct Creature *ch)
{

	Link *lnk = NULL;
	PObject *p_obj = NULL;
	int count = 0;

	strcpy(buf, "Assigned paths:\r\n");
	for (lnk = path_object_list; lnk; lnk = lnk->next, count++) {
		p_obj = (PObject *) lnk->object;
		if (p_obj->type == PMOBILE && p_obj->object)
			sprintf(buf, "%s%3d. MOB <%5d> %25s - %12s (%2d) %s\r\n", buf,
				count,
				((struct Creature *)p_obj->object)->mob_specials.shared->vnum,
				((struct Creature *)p_obj->object)->player.short_descr,
				p_obj->phead->name, p_obj->pos,
				IS_SET(p_obj->flags, POBJECT_STALLED) ? "stalled" : "");
		else if (p_obj->type == PVEHICLE && p_obj->object)
			sprintf(buf, "%s%3d. OBJ <%5d> %25s - %12s (%2d) %s\r\n", buf,
				count, ((struct obj_data *)p_obj->object)->shared->vnum,
				((struct obj_data *)p_obj->object)->name,
				p_obj->phead->name, p_obj->pos,
				IS_SET(p_obj->flags, POBJECT_STALLED) ? "stalled" : "");
		else
			strcat(buf, "ERROR!\r\n");
	}
	page_string(ch->desc, buf);
}

void
print_path(PHead * phead, char *str)
{
	char buf[MAX_STRING_LENGTH];
	int i, j, ll;
	int cmds = 0, fcmd = 0, cmdl;
	Link *cmd;

	if (!phead)
		return;

	sprintf(str, "%d %s %ld %d %d ", phead->number, phead->name, phead->owner,
		phead->wait_time, phead->length);
	ll = strlen(str);

	for (i = 0; i < phead->length; i++) {
		switch (phead->path[i].type) {
		case PATH_ROOM:
			sprintf(buf, "%d ", phead->path[i].data);
			break;
		case PATH_WAIT:
			sprintf(buf, "W%d ", phead->path[i].data);
			break;
		case PATH_DIR:
			sprintf(buf, "D%c ", *(dirs[phead->path[i].data]));
			break;
		case PATH_EXIT:
			strcpy(buf, "X ");
			break;
		case PATH_CMD:
			cmd = path_command_list;
			if (phead->path[i].data >= (fcmd + cmds)) {
				cmdl = phead->path[i].data;
				if (!cmds)
					fcmd = cmdl;
				cmds++;

				for (j = 1; j < cmdl; j++, cmd = cmd->next);
				sprintf(buf, "C\"%s\" ", (char *)(cmd->object));
			} else {
				cmdl = phead->path[i].data - fcmd + 1;
				sprintf(buf, "C%d ", cmdl);
			}
			break;
		}

		if ((ll + strlen(buf)) > 79) {
			strcat(str, "\n");
			ll = 0;
		}
		ll += strlen(buf);
		strcat(str, buf);
	}

	if (IS_SET(phead->flags, PATH_REVERSIBLE))
		strcat(str, "R");

	strcat(str, "\n~\n");
}

void
show_path(struct Creature *ch, char *arg)
{
	PHead *path_head = NULL;
	int i = 0;
	char outbuf[MAX_STRING_LENGTH];

	if (!*arg) {				/* show all paths */

		strcpy(outbuf, "Full path listing:\r\n");

		for (path_head = first_path; path_head;
			path_head = (PHead *) path_head->next, i++) {
			sprintf(buf,
				"%3d. %-15s  Own:[%-12s]  Wt:[%3d]  Flags:[%3d]  Len:[%3d]  BFS:[%9d]\r\n",
				path_head->number, path_head->name,
				playerIndex.getName(path_head->owner) ? playerIndex.getName(path_head->
					owner) : "NULL", path_head->wait_time, path_head->flags,
				path_head->length, path_head->find_first_step_calls);
			strcat(outbuf, buf);
		}
	} else if (!(path_head = real_path(arg)))
		sprintf(outbuf, "No such path, '%s'.\r\n", arg);
	else {
		/*
		   sprintf(outbuf, "PATH: %-20s  Wait:[%3d]  Flags:[%3d]  Length:[%3d]\r\n",
		   path_head->name, path_head->wait_time,
		   path_head->flags, path_head->length);
		 */
		strcpy(outbuf, "VNUM NAME OWNER <wait> <length> list...\r\n");
		print_path(path_head, buf);
		strcat(outbuf, buf);
	}

	page_string(ch->desc, outbuf);
}

int
add_path(char *spath, int save)
{
	char buf[MAX_INPUT_LENGTH];
	PHead *phead = NULL;
	Link *cmd, *ncmd;
	int i, j, start_len = path_command_length, cmds = 0, vnum;
	char *tmpc;

	if (!spath)
		return 1;

	/* Get the path vnum */
	spath = one_argument(spath, buf);
	if (!*buf)
		return 1;
	if (!is_number(buf) || (vnum = atoi(buf)) < 0)
		return 1;

	if (real_path_by_num(vnum))
		return 1;

	CREATE(phead, PHead, 1);
	phead->number = vnum;

	/* Get the path name */
	spath = one_argument(spath, buf);
	if (!*buf) {
		free(phead);
		return 2;
	}
	strncpy(phead->name, buf, 64);

	/* Get the path owner */
	spath = one_argument(spath, buf);
	if (!*buf) {
		free(phead);
		return 3;
	}
	phead->owner = atoi(buf);

	/* Get the default wait length */
	spath = one_argument(spath, buf);
	if (!*buf) {
		free(phead);
		return 4;
	}
	phead->wait_time = strtol(buf, NULL, 0);

	/* Get the number of entries in the path */
	spath = one_argument(spath, buf);
	if (!*buf) {
		free(phead);
		return 5;
	}
	phead->length = strtol(buf, NULL, 0);
	CREATE(phead->path, Path, phead->length);
	if (phead->length < 1) {
		free(phead);
		return 5;
	}

	for (i = 0; i < phead->length; i++) {
		spath = one_argument(spath, buf);
		switch (*buf) {
		case 0:
			free(phead->path);
			free(phead);
			return (i + 6);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			phead->path[i].type = PATH_ROOM;
			phead->path[i].data = strtol(buf, NULL, 0);
			break;
		case 'X':
		case 'x':
			phead->path[i].type = PATH_EXIT;
			phead->path[i].data = 0;
			break;
		case 'D':
		case 'd':
			phead->path[i].type = PATH_DIR;
			switch (buf[1]) {
			case 'n':
			case 'N':
				phead->path[i].data = 0;
				break;
			case 'e':
			case 'E':
				phead->path[i].data = 1;
				break;
			case 's':
			case 'S':
				phead->path[i].data = 2;
				break;
			case 'w':
			case 'W':
				phead->path[i].data = 3;
				break;
			case 'u':
			case 'U':
				phead->path[i].data = 4;
				break;
			case 'd':
			case 'D':
				phead->path[i].data = 5;
				break;
			case 'f':
			case 'F':
				phead->path[i].data = 6;
				break;
			case 'p':
			case 'P':
				phead->path[i].data = 7;
				break;
			default:
				free(phead->path);
				free(phead);
				return (i + 6);
			}
			break;
		case 'w':
		case 'W':
			if (!buf[1]) {
				free(phead->path);
				free(phead);
				return (i + 6);
			}
			phead->path[i].type = PATH_WAIT;
			phead->path[i].data = strtol(buf + 1, NULL, 0);
			break;

		case 'e':
		case 'E':
			if (!buf[1] || (buf[1] != '"')
				|| !(tmpc = strchr((char *)spath, '"'))) {
				free(phead->path);
				free(phead);
				return (i + 6);
			}
			phead->path[i].type = PATH_ECHO;

			strncat(buf, spath, (tmpc - spath));
			spath = tmpc + 1;
			*tmpc = 0;
			phead->path[i].str = strdup(buf + 2);
			break;

		case 'c':
		case 'C':
			if (!buf[1]) {
				free(phead->path);
				free(phead);
				return (i + 6);
			}
			phead->path[i].type = PATH_CMD;

			if ((buf[1] == '"') && (buf[strlen(buf) - 1] != '"')) {
				tmpc = strchr(spath, '"');
				if (!tmpc) {
					free(phead->path);
					free(phead);
					return (i + 6);
				}
				strncat(buf, spath, (tmpc - spath) + 1);	// +1 added to get last?
				spath = tmpc + 1;
				if (strlen(buf) < 4) {
					free(phead->path);
					free(phead);
					return (i + 6);
				}
			}
			if (buf[1] == '"') {
				if (strlen(buf) < 4) {
					free(phead->path);
					free(phead);
					return (i + 6);
				}
				buf[strlen(buf) - 1] = '\0';
				CREATE(ncmd, Link, 1);
				CREATE(ncmd->object, char, strlen(buf) - 1);
				strcpy((char *)ncmd->object, buf + 2);
				if (path_command_length == 0) {
					path_command_list = ncmd;
					path_command_length++;
				} else {
					cmd = path_command_list;
					for (j = 1; j < path_command_length; j++, cmd = cmd->next);
					path_command_length++;
					cmd->next = ncmd;
					ncmd->prev = cmd;
				}
				phead->path[i].data = path_command_length;
				cmds++;
			} else {
				phead->path[i].data = strtol(buf + 1, NULL, 0) + start_len;
				if (phead->path[i].data > (start_len + cmds)) {
					free(phead->path);
					free(phead);
					return (i + 6);
				}
			}
			break;
		default:
			free(phead->path);
			free(phead);
			return (i + 6);
		}
	}

	spath = one_argument(spath, buf);
	if (*buf) {
		if ((*buf == 'r') || (*buf == 'R'))
			SET_BIT(phead->flags, PATH_REVERSIBLE);
	}

	if (save)
		SET_BIT(phead->flags, PATH_SAVE);

	phead->next = first_path;
	first_path = phead;
	return 0;
}

void
clear_path_objects(PHead * phead)
{
	Link *lnk, *next_lnk;

	for (lnk = path_object_list; lnk; lnk = next_lnk) {
		next_lnk = lnk->next;
		if (lnk->object && ((PObject *) lnk->object)->phead == phead) {
			path_remove_object(((PObject *) lnk->object)->object);
		}
	}
}

void
load_paths(void)
{
	FILE *pathfile;
	int line = 0, fail, ret;
	Link *obj_link = NULL;
	PHead *path_head = NULL;
	static int virgin = 1;


	path_locked = 0;

	if (!(pathfile = fopen(PATH_FILE, "r")))
		return;
	while ((obj_link = path_object_list)) {
		path_object_list = path_object_list->next;
		free(obj_link->object);
		free(obj_link);
	}

	while ((path_head = first_path)) {
		first_path = (PHead *) first_path->next;
		free(path_head->path);
		free(path_head);
	}

	while ((obj_link = path_command_list)) {
		path_command_list = path_command_list->next;
		free(obj_link->object);
		free(obj_link);
	}
	path_command_length = 0;

	while ((pread_string(pathfile, buf, "paths."))) {
		line++;
        for (char *tc = strchr(buf, '\n');tc;tc = strchr(tc, '\n'))
            *tc = ' ';

		fail = 0;
		switch ((ret = add_path(buf, TRUE))) {
		case 0:
			break;
		case 1:
			sprintf(buf, "Path %d in vnum position.", line);
			fail = 1;
			break;
		case 2:
			sprintf(buf, "Path %d in title position.", line);
			fail = 1;
			break;
		case 3:
			sprintf(buf, "Path %d in owner position.", line);
			fail = 1;
			break;
		case 4:
			sprintf(buf, "Path %d in wait time position.", line);
			fail = 1;
			break;
		case 5:
			sprintf(buf, "Path %d in length position.", line);
			fail = 1;
			break;
		default:
			sprintf(buf, "Path %d in path position %d.", line,
				ret - 5);
			fail = 1;
		}

		if (fail && virgin) {
			fprintf(stderr, "paths error:%s\n", buf);
			safe_exit(0);
		} else if (fail) {
			errlog("%s", buf);
		}
	}

	virgin = 0;
}

void
path_do_echo(struct Creature *ch, struct obj_data *o, char *echo)
{
	char *tmp;
	int vnum;

	if (!echo || !*echo || (*echo != 'r' && *echo != 'R') ||
		!(vnum = strtol(echo + 1, &tmp, 10)) || *tmp != ' ')
		return;

	errlog("error in path_do_echo.");
}

void
path_activity(void)
{
	PObject *o;
	Link *i, *cmd, *next_i;
	int length, dir, j, k;
	struct room_data *room;
	struct Creature *ch;
	struct obj_data *obj;


	if (path_locked)
		return;

	for (i = path_object_list; i; i = next_i) {
		next_i = i->next;
		o = (PObject *) i->object;
		if (IS_SET(o->phead->flags, PATH_LOCKED) ||
			IS_SET(o->flags, POBJECT_STALLED))
			continue;

		length = o->wait_time;
		if (o->phead->path[o->pos].type == PATH_WAIT)
			length += o->phead->path[o->pos].data;

		if (o->time < length) {
			o->time++;
			continue;
		}

		if (o->type == PMOBILE && (ch = (Creature *) o->object) &&
			((ch->isFighting() || GET_MOB_WAIT(ch) > 0)))
			continue;

		o->time = 0;

		switch (o->phead->path[o->pos].type) {
		case PATH_ROOM:

			room = real_room(o->phead->path[o->pos].data);
			if ((o->type == PMOBILE) && room) {
				ch = (Creature *) o->object;
				if ((room == ch->in_room) && (o->phead->length != 1)) {
					PATH_MOVE(o);
					break;
				}
				if (ch->getPosition() < POS_STANDING)
					ch->setPosition(POS_STANDING);

				o->phead->find_first_step_calls++;

				if ((dir = find_first_step(ch->in_room, room, GOD_TRACK)) >= 0)
					perform_move(ch, dir, MOVE_NORM, 1);
				if ((ch->in_room == room) && (o->phead->length != 1))
					PATH_MOVE(o);
			} else if ((o->type == PVEHICLE) && room &&
				(obj = (struct obj_data *)o->object)->in_room) {

				o->phead->find_first_step_calls++;

				if ((dir = find_first_step(obj->in_room, room, GOD_TRACK)) >= 0)
					move_car(NULL, obj, dir);
				if ((obj->in_room == room) && (o->phead->length != 1))
					PATH_MOVE(o);
			}
			break;
		case PATH_DIR:

			dir = o->phead->path[o->pos].data;
			if (o->type == PMOBILE) {
				ch = (struct Creature *)o->object;

				if (ch->getPosition() < POS_STANDING)
					ch->setPosition(POS_STANDING);

				perform_move(ch, dir, MOVE_NORM, 1);
				if (o->phead->length != 1)
					PATH_MOVE(o);
			} else if ((o->type == PVEHICLE) &&
				(obj = (struct obj_data *)o->object)->in_room) {
				move_car(NULL, obj, dir);
				if (o->phead->length != 1)
					PATH_MOVE(o);
			}
			break;
		case PATH_CMD:

			if (o->type == PMOBILE) {
				ch = (struct Creature *)o->object;
				cmd = path_command_list;
				j = o->phead->path[o->pos].data;
				for (k = 1; k != j; k++, cmd = cmd->next);
				command_interpreter(ch, (char *)cmd->object);
			}
			if (o->phead->length != 1)
				PATH_MOVE(o);
			break;
		case PATH_ECHO:

			if (o->type == PMOBILE) {
				path_do_echo((struct Creature *)o->object, NULL,
					o->phead->path[o->pos].str);
			} else {
				path_do_echo(NULL, (struct obj_data *)o->object,
					o->phead->path[o->pos].str);
			}
			if (o->phead->length != 1)
				PATH_MOVE(o);
			break;
		case PATH_WAIT:

			if (o->phead->length != 1) {
				PATH_MOVE(o);
			} else
				SET_BIT(o->flags, POBJECT_STALLED);
			break;
		case PATH_EXIT:

			path_remove_object(o->object);
			break;
		default:

			break;

		}
	}
}
void
path_remove_object(thing *object)
{
	Link *i;

	for (i = path_object_list; i; i = i->next) {
		if (((PObject *) i->object)->object == object) {
			break;
		}
	}

	if (!i)
		return;

	if (i == path_object_list)
		path_object_list = i->next;

	if (i->next)
		i->next->prev = i->prev;

	if (i->prev)
		i->prev->next = i->next;

	free(i);
}

int
add_path_to_mob(struct Creature *mob, int vnum)
{
	PHead *phead;
	Link *i;
	PObject *o;


	if (!vnum || !mob)
		return 0;

	/* Find the requested path */
	for (phead = first_path; phead; phead = (PHead *) phead->next)
		if (phead->number == vnum)
			break;

	if (!phead)
		return 0;

	/* See if the mob already has a path */
	for (i = path_object_list; i; i = i->next) {
		o = (PObject *) i->object;
		if ((o->type == PMOBILE) && (o->object == mob))
			return 0;
	}

	CREATE(i, Link, 1);
	CREATE(o, PObject, 1);
	i->object = o;
	if (path_object_list) {
		path_object_list->prev = i;
		i->next = path_object_list;
	}
	path_object_list = i;

	o->type = PMOBILE;
	o->object = mob;
	o->phead = phead;
	o->wait_time = phead->wait_time;
	o->step = 1;
	o->pos = 0;

	return 1;
}

int
add_path_to_vehicle(struct obj_data *obj, int vnum)
{
	PHead *phead;
	Link *i;
	PObject *o;

	if (!obj || !vnum || !IS_VEHICLE(obj))
		return 0;

	/* Find the requested path */
	for (phead = first_path; phead; phead = (PHead *) phead->next)
		if (phead->number == vnum)
			break;

	if (!phead)
		return 0;

	/* See if the car already has a path */
	for (i = path_object_list; i; i = i->next) {
		o = (PObject *) i->object;
		if ((o->type == PVEHICLE) && (o->object == obj))
			return 0;
	}

	CREATE(i, Link, 1);
	CREATE(o, PObject, 1);
	i->object = o;
	if (path_object_list) {
		path_object_list->prev = i;
		i->next = path_object_list;
	}
	path_object_list = i;

	o->type = PVEHICLE;
	o->object = obj;
	o->phead = phead;
	o->wait_time = phead->wait_time;
	o->step = 1;

	return 1;
}

void
free_paths()
{
	PHead *p_head = NULL;
	Link *p_obj = NULL;

	while ((p_obj = path_object_list)) {
		path_object_list = path_object_list->next;
		free(p_obj->object);
		free(p_obj);
	}

	while ((p_head = first_path)) {
		first_path = (PHead *) first_path->next;
		free(p_head->path);
		free(p_head);
	}

	while ((p_obj = path_command_list)) {
		path_command_list = path_command_list->next;
		free(p_obj->object);
		free(p_obj);
	}
}
