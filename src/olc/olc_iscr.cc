//
// File: olc_iscr.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/*******************************************************************
*  File: olc_iscr.c                              Part of TempusMUD *
* Usage: Interactive Event Based Mobile Scripts                    *
*                                                                  *
* All rights reserved.                                             *
*                                                                  *
*                                              (Nothing@TempusMUD) *
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "security.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "specs.h"
#include "iscript.h"
#include "events.h"
#include <fstream>

extern list <CIScript *>scriptList;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern int olc_lock;
extern int *iscr_index;

extern char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
	arg3[MAX_INPUT_LENGTH];

const char *olc_iset_keys[] = {
	"name",
//    "flags",
	"\n"
};

const char *olc_ihandler_keys[] = {
	"create",
	"delete",
	"edit",
	"\n"
};

const char *event_types[] = {
	"EVT_EXAMINE",
	"EVT_PHYSICAL_ATTACK",
	"EVT_STEAL",
	"\n"
};

#define NUM_ISET_COMMANDS 1
#define NUM_IHANDLER_COMMANDS 3

int
write_iscr_index(struct char_data *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; iscr_index[i] != -1; i++) {
		count++;
		if (iscr_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (iscr_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (iscr_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = iscr_index[i];
			done = 1;
		} else
			new_index[j] = iscr_index[i];
		j++;
	}

	free(iscr_index);

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	iscr_index = new_index;

	sprintf(fname, "world/iscr/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char(ch, "Could not open index file, iscript save aborted.\r\n");
		return (0);
	}
	for (i = 0; iscr_index[i] != -1; i++)
		fprintf(index, "%d.iscr\n", iscr_index[i]);

	fprintf(index, "$\n");

	send_to_char(ch, "IScript index file re-written.\r\n");

	fclose(index);

	return (1);
}

int
do_olc_isave(struct char_data *ch)
{
	struct zone_data *zone;
	int svnum;
	room_num low = 0, high = 0;
	char fname[64];
	string line;

	if (GET_OLC_ISCR(ch)) {
		svnum = GET_OLC_ISCR(ch)->getVnum();
		for (zone = zone_table; zone; zone = zone->next) {
			if (svnum >= zone->number * 100 && svnum <= zone->top) {
				break;
			}
		}

		if (!zone) {
			slog("OLC:  ERROR finding zone for iscript %d.", svnum);
			send_to_char(ch, "Unable to match iscript with zone error.\r\n");
			return 1;
		}
	} else
		zone = ch->in_room->zone;

	sprintf(fname, "world/iscr/%d.iscr", zone->number);
	if ((access(fname, F_OK) >= 0) && (access(fname, W_OK) < 0)) {
		mudlog(0, BRF, true,
			"OLC: ERROR - Main iscript file for zone %d is read-only.",
			zone->number);
	}
	sprintf(fname, "world/iscr/olc/%d.iscr", zone->number);
	ofstream ofile(fname);
	if (!ofile)
		return 1;
	if ((write_iscr_index(ch, zone)) != 1) {
		return 1;
	}

	low = zone->number * 100;
	high = zone->top;

	list <CIScript *>::iterator si;
	list <CHandler *>::iterator hi;
	list <string>::iterator li;

	for (si = scriptList.begin(); si != scriptList.end(); si++) {
		if ((*si)->getVnum() < low)
			continue;
		if ((*si)->getVnum() > high)
			break;

		ofile << "#" << (*si)->getVnum() << endl;
		ofile << "Name: " << (*si)->getName() << endl;
		for (hi = (*si)->theScript.begin(); hi != (*si)->theScript.end(); hi++) {
			ofile << (*hi)->getEventType() << endl;
			for (li = (*hi)->getTheLines().begin();
				li != (*hi)->getTheLines().end(); li++) {
				ofile << *li << endl;
			}
			ofile << "__end event__" << endl;
		}
		ofile << "__end script__" << endl;
	}

	ofile << "$" << endl;
	ofile.close();

	slog("OLC:  %s isaved %d.", GET_NAME(ch), zone->number);

	sprintf(fname, "world/iscr/%d.iscr", zone->number);
	ofstream realfile(fname);
	if (realfile) {
		sprintf(fname, "world/iscr/olc/%d.iscr", zone->number);
		ifstream ifile(fname);
		if (!ifile) {
			slog("SYSERR: Failure to reopen olc iscript file.");
			send_to_char(ch, 
				"OLC Error:  Failure to duplicate mob file in main dir."
				"\r\n");
			realfile.close();
			return 1;
		}

		while (getline(ifile, line))
			realfile << line << endl;

		realfile.close();
		ifile.close();
	} else {
		slog("SYSERR: Failure to open main iscript file: %s", fname);
		send_to_char(ch, "OLC Error:  Failure to open main iscript file.\r\n");
	}
	return 0;
}

void
do_olc_ihandler(struct char_data *ch, char *arg)
{
	class CIScript *script;
	int i, ihandler_command;
	bool found = false;

	if (!(script = GET_OLC_ISCR(ch))) {
		send_to_char(ch, "You are not currently editing an iscript.\r\n");
		return;
	}

	if (!*arg) {
		strcpy(buf, "Valid ihandler commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (i < NUM_IHANDLER_COMMANDS) {
			strcat(buf, olc_ihandler_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf);
		return;
	}

	half_chop(arg, arg1, arg2);
	skip_spaces(&arg);

	if ((ihandler_command = search_block(arg1, olc_ihandler_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid ihandler command '%s'.\r\n", arg1);
		return;
	}

	if (!*arg2) {
		send_to_char(ch, "%s what??\r\n", olc_ihandler_keys[ihandler_command]);
		return;
	}

	if (is_number(arg2)) {
		if (*arg2 == '0') {
			i = 0;
			strcpy(arg2, event_types[i]);
		} else if ((i = atoi(arg2))) {
			if (i < NUM_EVENTS)
				strcpy(arg2, event_types[i]);
			else {
				send_to_char(ch, 
					"Please check olc help ihandler to get a valid event"
					" number\r\n");
				return;
			}
		} else {
			mudlog(0, BRF, true,
				"OLC: Error converting arg2 to an integer in do_olc_ihandler.\r\n");
		}
	}

	for (i = 0; i < NUM_EVENTS; i++) {
		if (strcmp(arg2, event_types[i]) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		strcpy(buf, "EVENT TYPES:\r\n");
		for (i = 0; i < NUM_EVENTS; i++) {
			sprintf(buf2, "%s%2d)  %s%s%s\r\n",
				CCYEL(ch, C_NRM), i, CCCYN(ch, C_NRM), event_types[i],
				CCNRM(ch, C_NRM));
			strcat(buf, buf2);
		}
		page_string(ch->desc, buf);
		return;
	}

	CHandler *h = new CHandler(strdup(arg2));

	switch (ihandler_command) {
	case 0:					// Create
		if ((*script).handler_exists(arg2) != NULL) {
			send_to_char(ch, "That event already exists.\r\n");
			return;
		}
		if (h != NULL) {
			SET_BIT(PLR_FLAGS(ch), PLR_OLC);
			GET_OLC_HANDLER(ch) = h;
			(*script).theScript.push_back(h);
			start_script_editor(ch->desc, h->getTheLines(), true);
		} else {
			slog("ERROR: do_olc_ihandler unable to create handler!");
			send_to_char(ch, 
				"ERROR:  do_olc_ihandler unable to create handler!\r\n");
		}
		break;

	case 1:					// Delete
		if ((*script).deleteHandler(arg2))
			send_to_char(ch, "Handler successfully deleted.\r\n");
		else
			send_to_char(ch, "ERROR:  Could not delete handler.\r\n");
		break;

	case 2:					// Edit 
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		CHandler *hr = (*script).handler_exists(arg2);
		if (hr == NULL) {
			send_to_char(ch, "No handler: %s  exists in script: %s.\r\n", arg2,
				(*script).getName().c_str());
			return;
		}
		GET_OLC_HANDLER(ch) = hr;
		start_script_editor(ch->desc, hr->getTheLines(), true);
		break;
	}
}

void
do_olc_iset(struct char_data *ch, char *arg)
{
	class CIScript *script;
	int i, iset_command;

	if (!(script = GET_OLC_ISCR(ch))) {
		send_to_char(ch, "You are not currently editing an iscript.\r\n");
		return;
	}
	if (!*arg) {
		strcpy(buf, "Valid iset commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (i < NUM_ISET_COMMANDS) {
			strcat(buf, olc_iset_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf);
		return;
	}

	half_chop(arg, arg1, arg2);
	skip_spaces(&arg);

	if ((iset_command = search_block(arg1, olc_iset_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid iset command '%s'.\r\n", arg1);
		return;
	}

	if (!*arg2) {
		send_to_char(ch, "Set %s to what??\r\n", olc_iset_keys[iset_command]);
		return;
	}

	switch (iset_command) {
	case 0:					// Name
		(*script).setName(arg2);
		send_to_char(ch, "IScript name set.\r\n");
		break;

	default:
		break;
	}
}

void
do_olc_idelete(struct char_data *ch, char *argument)
{
	struct zone_data *zone = NULL;
	struct descriptor_data *d = NULL;
	class CIScript *script = GET_OLC_ISCR(ch);
	bool found = false;
	int i;

	if (!argument) {
		if (!script)
			send_to_char(ch, "You are not currently editing an iscript.\r\n");
	}

	if (!is_number(argument) && !script) {
		send_to_char(ch, "The argument must be a number.\r\n");
		return;
	}

	i = atoi(argument);
	if ((script = real_iscript(i)) == NULL)
		send_to_char(ch, "There is no such iscript.\r\n");
	else
		for (zone = zone_table; zone; zone = zone->next) {
			if (i <= zone->top)
				break;
		}
	if (!zone) {
		send_to_char(ch, "That iscript doesn't belong to any zone!!\r\n");
		slog("SYSERR:  iscript not in any zone.");
		return;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "You do not have permission to delete that iscript.\r\n");
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->character && GET_OLC_ISCR(d->character) == script) {
			act("$N is currently editing that iscript.", FALSE, ch, 0,
				d->character, TO_CHAR);
			return;
		}
	}

	list <CIScript *>::iterator si;

	for (si = scriptList.begin(); si != scriptList.end();) {
		if ((*si)->getVnum() == (*script).getVnum()) {
			found = true;
			si = scriptList.erase(si);
		} else
			si++;
	}

	send_to_char(ch, found ? "IScript successfully deleted.\r\n" :
		"Could not find that IScript.  Bug this.\r\n");
	if (!found) {
		mudlog(0, BRF, true,
			"OLC: Error in do_olc_idelete.  prototype exists for script "
			"[%5d] [%s] but it's not in the list!\r\n", (*script).getVnum(),
			(*script).getName().c_str());
	}
}

void
do_olc_iedit(struct char_data *ch, char *argument)
{
	struct zone_data *zone = NULL;
	struct descriptor_data *d = NULL;
	class CIScript *script = GET_OLC_ISCR(ch);
	int i;

	if (!*argument) {
		if (!script)
			send_to_char(ch, "You are not currently editing an iscript.\r\n");
		else {
			send_to_char(ch, "Current olc iscript: [%5d] %s\r\n",
				(*script).getVnum(), (*script).getName().c_str());
		}
		return;
	}
	if (!is_number(argument)) {
		if (is_abbrev(argument, "exit")) {
			send_to_char(ch, "Exiting iscript editor.\r\n");
			GET_OLC_ISCR(ch) = NULL;
			return;
		}
		send_to_char(ch, "The argument must be a number.\r\n");
		return;
	} else {
		i = atoi(argument);
		if ((script = real_iscript(i)) == NULL)
			send_to_char(ch, "There is no such iscript.\r\n");
		else {
			for (zone = zone_table; zone; zone = zone->next) {
				if (i <= zone->top)
					break;
			}
			if (!zone) {
				send_to_char(ch, "That iscript doesn't belong to any zone!!\r\n");
				slog("SYSERR:  iscript not in any zone.");
				return;
			}

			if (!CAN_EDIT_ZONE(ch, zone)) {
				send_to_char(ch, 
					"You do not have permission to edit that iscript.\r\n");
				return;
			}

			for (d = descriptor_list; d; d = d->next) {
				if (d->character && GET_OLC_ISCR(d->character) == script) {
					act("$N is already editing that iscript.", FALSE, ch, 0,
						d->character, TO_CHAR);
					return;
				}
			}

			GET_OLC_ISCR(ch) = script;
			send_to_char(ch, "Now editing iscript [%5d] %s%s%s\r\n",
				(*script).getVnum(), CCGRN(ch, C_NRM),
				(*script).getName().c_str(), CCNRM(ch, C_NRM));
		}
	}
}

void
do_olc_ilist(struct char_data *ch, char *argument)
{
	int counter = 0, znum = -1;
	list <CIScript *>::iterator si;
	struct zone_data *zone;
	string buf1, head_buf, body_buf;

	buf1 = "";

	// Ok, this is screwed up and I know it...but it saves code duplication
	for (is_abbrev(argument, "all") ? (zone = zone_table) : (zone =
			ch->in_room->zone); zone;
		is_abbrev(argument, "all") ? (zone = zone->next) : zone = NULL) {
		znum = zone->number;
		counter = 0;
		head_buf = "";
		body_buf = "";
		sprintf(buf, "\r\n"
			"%sIScripts for zone: [%s%5d%s]\r\n"
			"---------------------------------------------------%s\r\n",
			CCGRN(ch, C_NRM),
			CCRED(ch, C_NRM), znum, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

		head_buf = buf;
		for (si = scriptList.begin(); si != scriptList.end(); si++) {
			if (((*si)->getVnum() >= znum * 100)
				&& ((*si)->getVnum() <= zone->top)) {
				counter++;
				sprintf(buf, "%s%3d%s)  [%s%5d%s]%s %s%s\r\n",
					CCYEL(ch, C_NRM),
					counter,
					CCGRN(ch, C_NRM),
					CCRED(ch, C_NRM),
					(*si)->getVnum(),
					CCGRN(ch, C_NRM),
					CCCYN(ch, C_NRM),
					(*si)->getName().c_str(), CCNRM(ch, C_NRM));
				body_buf += buf;
			}
		}
		if (is_abbrev(argument, "all")) {
			if (counter > 0) {
				buf1 += head_buf;
				buf1 += body_buf;
			}
		} else if (counter == 0) {
			buf1 += head_buf;
			buf1 += "There are no iscripts defined for this zone.\r\n";
		} else {
			buf1 += head_buf;
			buf1 += body_buf;
		}
	}
	char *tmp_buf = new char[buf1.length() + 1];
	strcpy(tmp_buf, buf1.c_str());
	page_string(ch->desc, tmp_buf);
}

class CIScript *
do_create_iscr(struct char_data *ch, int vnum)
{
	struct zone_data *zone = NULL;

	if (real_iscript(vnum)) {
		send_to_char(ch, "ERROR:  ISCRIPT already exists\r\n");
		return NULL;
	}

	for (zone = zone_table; zone; zone = zone->next) {
		if (vnum >= zone->number * 100 && vnum <= zone->top) {
			break;
		}
	}

	if (!zone) {
		send_to_char(ch, 
			"ERROR: A zone must be defined for the ISCRIPT first.\r\n");
		return NULL;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Try again in your own zone, pissant.\r\n");
		mudlog(GET_INVIS_LEV(ch), BRF, true,
			"OLC: %s failed attempt to CREATE iscript %d.",
			GET_NAME(ch), vnum);
		return NULL;
	}

	CIScript *script = new CIScript(vnum);
	list <CIScript *>::iterator si;
	list <CIScript *>::iterator tmp_si;
	if (scriptList.empty()) {
		scriptList.push_front(script);
		return script;
	}
	for (si = scriptList.begin(); si != scriptList.end(); si++) {
		tmp_si = si;
		tmp_si--;
		if (si == scriptList.begin() && (*si)->getVnum() > vnum) {
			scriptList.push_front(script);
			break;
		} else if ((*si)->getVnum() > vnum && (*tmp_si)->getVnum() < vnum) {
			scriptList.insert(si, script);
			break;
		} else {
			tmp_si = si;
			tmp_si++;
			if (tmp_si == scriptList.end()) {
				scriptList.push_back(script);
				break;
			}
		}
	}

	return script;
}

void
do_olc_istat(struct char_data *ch, char *arg)
{
	list <CIScript *>::iterator si;
	list <CHandler *>::iterator hi;
	int svnum;
	int counter = -1;
	char buf1[1024];

	skip_spaces(&arg);

	if (!(*arg) && !GET_OLC_ISCR(ch)) {
		send_to_char(ch, "Which iscript?\r\n");
		return;
	}

	if (!is_number(arg)) {
		send_to_char(ch, "The argument must be a number.\r\n");
		return;
	}

	if (!(svnum = atoi(arg))) {
		if (!(svnum = GET_OLC_ISCR(ch)->getVnum())) {
			send_to_char(ch, "Usage:  olc istat [vnum]\r\n");
		}
	}

	for (si = scriptList.begin();
		si != scriptList.end() && (svnum != (*si)->getVnum()); si++);

	if (si == scriptList.end()) {
		send_to_char(ch, "There is no such iscript.\r\n");
		return;
	}

	bzero(buf1, 1024);
	send_to_char(ch, "\r\n"
		"%sStatus for IScript: [%s%5d%s]\r\n"
		"              Name: [%s%s%s]\r\n"
		"---------------------------------------------------%s\r\n\r\n",
		CCGRN(ch, C_NRM),
		CCRED(ch, C_NRM),
		(*si)->getVnum(),
		CCGRN(ch, C_NRM),
		CCCYN(ch, C_NRM),
		(*si)->getName().c_str(), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

	send_to_char(ch, "  %sEvents:\r\n%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	for (hi = (*si)->theScript.begin(); hi != (*si)->theScript.end(); hi++) {
		counter++;
		sprintf(buf, "    %s%s%s\r\n",
			CCCYN(ch, C_NRM), (*hi)->getEventType().c_str(), CCNRM(ch, C_NRM));
		strcat(buf1, buf);
	}
	if (counter == -1)
		send_to_char(ch, 
			"    There are no event handlers defined for this iscript.\r\n");
	else
		page_string(ch->desc, buf1);
}
