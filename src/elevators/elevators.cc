//
// File: paths.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

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
#include "elevators.h"

int move_car(struct Creature *ch, struct obj_data *car, int dir);

struct elevator_data *elevators = NULL;

int
load_elevators(void)
{
	struct elevator_elem *elem = NULL;
	struct elevator_data *head = NULL;
	FILE *file;
	char line[256], name[512];
	int last = 99999, nr, vnum, key;

	if (!(file = fopen(ELEVATOR_FILE, "r"))) {
		slog("SYSERR: no elevator file.");
		return 0;
	}

	while ((head = elevators)) {
		elevators = head->next;

		while ((elem = head->list)) {
			head->list = elem->next;
			if (elem->name)
				free(elem->name);
			free(elem);
		}

		free(head);
	}

	elevators = NULL;

	while (1) {

		if (!get_line(file, line)) {
			slog("SYSERR: Format error in elevator file, after %d",
				last);
			fclose(file);
			return 0;
		}
		if (*line == '$') {
			fclose(file);
			return 1;
		}
		if (*line != '#') {
			slog("SYSERR: Format error in elevator file, after %d",
				last);
			fclose(file);
			return 0;
		}
		if (sscanf(line, "#%d", &nr) != 1) {
			slog("SYSERR: Format error in elevator file, after %d",
				last);
			fclose(file);
			return 0;
		}

		if (nr > last) {
			slog("SYSERR: elevator %d is after %d.. must decrease.",
				nr, last);
			fclose(file);
			return 0;
		}

		last = nr;

		CREATE(head, struct elevator_data, 1);
		head->vnum = nr;
		head->list = NULL;
		head->pQ = NULL;

		while (1) {
			if (!get_line(file, line)) {
				slog("SYSERR: Format error in elevator file, in %d",
					nr);
				while ((elem = head->list)) {
					head->list = elem->next;
					if (elem->name)
						free(elem->name);
					free(elem);
				}
				free(head);
				fclose(file);
				return 0;
			}

			if (*line == 'S')
				break;

			if (sscanf(line, " %d %d %s", &vnum, &key, name) != 3) {
				slog("SYSERR: Vnum error in elevator file, in %d", nr);
				while ((elem = head->list)) {
					head->list = elem->next;
					if (elem->name)
						free(elem->name);
					free(elem);
				}
				free(head);
				fclose(file);
				return 0;
			}

			CREATE(elem, struct elevator_elem, 1);
			elem->name = strdup(name);
			elem->rm_vnum = vnum;
			elem->key = key;

			elem->next = head->list;
			head->list = elem;
		}

		head->next = elevators;
		elevators = head;
	}
	fclose(file);
	return 1;
}
struct elevator_data *
real_elevator(int vnum)
{
	struct elevator_data *elev;

	for (elev = elevators; elev; elev = elev->next)
		if (elev->vnum == vnum)
			return (elev);

	return NULL;
}

int
path_to_elevator_queue(int vnum, struct elevator_data *elev)
{
	path_Q_link *lnk, *newlnk;
	PHead *phead;
	char pathname[512];

	sprintf(pathname, "elev%d", vnum);

	if (!(phead = real_path(pathname))) {
		slog("SYSERR: unable to find real path in path_to_elevator_queue.");
		return 0;
	}

	newlnk = (path_Q_link *) malloc(sizeof(path_Q_link));
	newlnk->next = NULL;
	newlnk->phead = phead;

	if (!elev->pQ) {
		elev->pQ = newlnk;
		return 1;
	}
	for (lnk = elev->pQ; lnk; lnk = lnk->next)
		if (!lnk->next) {
			lnk->next = newlnk;
			return 1;
		}

	return 0;
}

struct elevator_elem *
elevator_elem_by_name(char *str, struct elevator_data *elev)
{
	struct elevator_elem *elem;

	for (elem = elev->list; elem; elem = elem->next) {
		if (elem->name && !strncmp(str, elem->name, strlen(elem->name)))
			return (elem);
	}
	return NULL;
}

int
target_room_on_queue(int vnum, struct elevator_data *elev)
{
	path_Q_link *lnk;

	for (lnk = elev->pQ; lnk; lnk = lnk->next)
		if (lnk->phead && lnk->phead->path && lnk->phead->path[0].data == vnum)
			return 1;
	return 0;
}

int
handle_elevator_position(struct obj_data *car, struct room_data *targ_rm)
{
	struct obj_data *panel;
	struct room_data *in_rm;
	struct elevator_data *elev;
	path_Q_link *lnk;

	if (!(in_rm = real_room(ROOM_NUMBER(car)))) {
		slog("SYSERR: elevator car with bogus ROOM_NUMBER.");
		return 0;
	}

	for (panel = in_rm->contents; panel; panel = panel->next_content)
		if (IS_OBJ_TYPE(panel, ITEM_ELEVATOR_PANEL))
			break;

	if (!panel) {
		slog("SYSERR: unable to find panel in handle_elevator_position.");
		return 0;
	}

	if (!(elev = real_elevator(GET_OBJ_VAL(panel, 0)))) {
		slog("SYSERR: real_elevator failed in handle_elevator_position.");
		return 0;
	}

	if (car->in_room == targ_rm) {
		send_to_room("The elevator comes to a halt.\r\n", targ_rm);
		send_to_room("The elevator comes to a halt.\r\n", in_rm);
		if (elev->pQ && elev->pQ->phead)
			delete_path(elev->pQ->phead);
		lnk = elev->pQ;
		elev->pQ = lnk->next;
		free(lnk);
		if (elev->pQ && elev->pQ->phead)
			add_path_to_vehicle(car, elev->pQ->phead->name);
		return 1;
	}

	return 0;
}
