//
// File: dyntext.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// ping
// ping 2

#define __DYNTEXT_C__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "editor.h"
#include "screen.h"
#include "interpreter.h"
#include "player_table.h"
#include "accstr.h"

// external variables

// external functions
int check_editors(Creature *ch, char **buffer);	// quest.c

// local variables
dynamic_text_file *dyntext_list = NULL;

char *dynedit_update_string(dynamic_text_file *);

/***
 *
 * Dynamic text file functions
 *
 ***/

void
boot_dynamic_text(void)
{
	DIR *dir;
	struct dirent *dirp;
	dynamic_text_file *newdyn = NULL;
	FILE *fl;
	char filename[1024];
	char c;
	unsigned int i, j;

	if (!(dir = opendir(DYN_TEXT_CONTROL_DIR))) {
		errlog("Cannot open dynamic text control dir.");
		safe_exit(1);
	}

	while ((dirp = readdir(dir))) {

		if ((strlen(dirp->d_name) > 4) &&
			!strcmp(dirp->d_name + strlen(dirp->d_name) - 4, ".dyn")) {

			sprintf(filename, "%s/%s", DYN_TEXT_CONTROL_DIR, dirp->d_name);

			if (!(fl = fopen(filename, "r"))) {
				errlog("error opening dynamic control file '%s'.",
					filename);
				perror("fopen:");
				continue;
			}

			if (!(newdyn =
					(dynamic_text_file *) malloc(sizeof(dynamic_text_file)))) {
				errlog("error allocating dynamic control block, aborting.");
				closedir(dir);
				fclose(fl);
				return;
			}

			if (!(fread(newdyn, sizeof(dynamic_text_file), 1, fl))) {
				errlog("error reading information from '%s'.",
					filename);
				free(newdyn);
				fclose(fl);
				continue;
			}

			fclose(fl);

			slog("dyntext BOOTED %s.", dirp->d_name);

			newdyn->next = NULL;
			newdyn->buffer = NULL;
			newdyn->tmp_buffer = NULL;

			sprintf(buf2, "text/%s", newdyn->filename);

			if (!(fl = fopen(buf2, "r"))) {
				errlog("unable to open dynamic text file '%s'.",
					buf2);
				perror("dyntext fopen:");
			}

			if (fl) {

				*buf = '\0';

				// insert \r\n in the place of \n
				for (i = 0, j = 0; j < MAX_STRING_LENGTH - 1 && !feof(fl); i++) {
					c = fgetc(fl);
					if (c == '\n')
						buf[j++] = '\r';
					buf[j++] = c;
				}

				buf[j] = '\0';

				strcat(buf, "\r\n");

				newdyn->buffer = strdup(buf);

				fclose(fl);
			}

			newdyn->next = dyntext_list;
			dyntext_list = newdyn;

		}
	}

	closedir(dir);

}

int
create_dyntext_backup(dynamic_text_file * dyntext)
{

	DIR *dir;
	struct dirent *dirp;
	int len = strlen(dyntext->filename);
	int num = 0, maxnum = 0;
	char filename[1024];
	FILE *fl = NULL;

	// open backup dir
	if (!(dir = opendir(DYN_TEXT_BACKUP_DIR))) {
		errlog("Cannot open dynamic text backup dir.");
		return 1;
	}
	// scan files in backup dir
	while ((dirp = readdir(dir))) {

		// looks like the right filename
		if (!strncasecmp(dirp->d_name, dyntext->filename, len) &&
			*(dirp->d_name + len) && *(dirp->d_name + len) == '.' &&
			*(dirp->d_name + len + 1)) {

			num = atoi(dirp->d_name + len + 1);

			if (num > maxnum)
				maxnum = num;

		}
	}

	closedir(dir);

	sprintf(filename, "%s/%s.%02d", DYN_TEXT_BACKUP_DIR, dyntext->filename,
		maxnum + 1);

	if (!(fl = fopen(filename, "w"))) {
		errlog("Dyntext backup unable to open '%s'.", filename);
		return 1;
	}

	if (dyntext->buffer) {
		for (char *ptr = dyntext->buffer; *ptr; ptr++) {
			if (*ptr == '\r')
				continue;
			fputc(*ptr, fl);
		}
	}

	fclose(fl);
	return 0;
}

int
save_dyntext_buffer(dynamic_text_file * dyntext)
{
	FILE *fl = NULL;
	char filename[1024];
	char *ptr = NULL;

	sprintf(filename, "text/%s", dyntext->filename);

	if (!(fl = fopen(filename, "w"))) {
		errlog("Unable to open '%s' for write.", filename);
		return 1;
	}

	if (dyntext->buffer) {
		for (ptr = dyntext->buffer; *ptr; ptr++) {
			if (*ptr == '\r')
				continue;
			fputc(*ptr, fl);
		}
	}

	fclose(fl);

	return 0;

}

int
reload_dyntext_buffer(dynamic_text_file * dyntext)
{
	FILE *fl;
	char filename[1024];
	char c;
	unsigned int i, j;

	sprintf(filename, "text/%s", dyntext->filename);

	if (!(fl = fopen(filename, "r"))) {
		errlog("unable to open dynamic text file '%s'.",
			filename);
		perror("dyntext fopen:");
	}

	if (fl) {
		*buf = '\0';
		// insert \r\n in the place of \n
		for (i = 0, j = 0; j < MAX_STRING_LENGTH - 1 && !feof(fl); i++) {
			c = fgetc(fl);
			if (c == '\n')
				buf[j++] = '\r';
			buf[j++] = c;
		}

		buf[j] = '\0';
		strcat(buf, "\r\n");
		if (dyntext->buffer != NULL) {
			free(dyntext->buffer);
		}
		dyntext->buffer = strdup(buf);
		fclose(fl);
		return 0;
	}
	return -1;
}

int
save_dyntext_control(dynamic_text_file * dyntext)
{
	FILE *fl = NULL;
	char filename[1024];

	sprintf(filename, "%s/%s.dyn", DYN_TEXT_CONTROL_DIR, dyntext->filename);

	if (!(fl = fopen(filename, "w"))) {
		errlog("Unable to open '%s' for write.", filename);
		return 1;
	}

	if (!(fwrite(dyntext, sizeof(dynamic_text_file), 1, fl))) {
		errlog("Unable to write data to '%s'.", filename);
		fclose(fl);
		return 1;
	}

	fclose(fl);
	return 0;
}

int
push_update_to_history(Creature *ch, dynamic_text_file * dyntext)
{
	int i;

	for (i = DYN_TEXT_HIST_SIZE - 1; i > 0; i--)
		dyntext->last_edit[i] = dyntext->last_edit[i - 1];

	dyntext->last_edit[i].idnum = GET_IDNUM(ch);
	dyntext->last_edit[i].tEdit = time(0);

	return 0;

}

char *dynedit_options[][2] = {
	{"show", "[ <filename> [old|new|perms|last] ]"},
	{"set", "<filename> <level> <arg>"},
	{"add", "<filename> <idnum>"},
	{"remove", "<filename> <idnum>"},
	{"edit", "<filename>"},
	{"abort", "<filename>"},
	{"update", "<filename>"},
	{"prepend", "<filename> (prepends the old text to the new)"},
	{"append", "<filename> (appends the old text to the new)"},
	{"reload", "<filename>"},
	{NULL, NULL}
};

void
show_dynedit_options(Creature *ch)
{
	int i = 0;

    send_to_char(ch, "Dynedit usage:\r\n");

	for (i = 0; dynedit_options[i][0] != NULL; i++)
		send_to_char(ch, "%10s   %s\r\n", dynedit_options[i][0],
			dynedit_options[i][1]);

}

void
set_dyntext(Creature *ch, dynamic_text_file * dyntext, char *argument)
{

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int lev;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, "Dynedit set reuires more arguments.\r\n");
		show_dynedit_options(ch);
		return;
	}

	if (is_abbrev(arg1, "level")) {

		lev = atoi(arg2);

		if (lev > GET_LEVEL(ch)) {
			send_to_char(ch, 
				"Let's not set it above your own level, shall we?\r\n");
			return;
		}

		dyntext->level = lev;
		send_to_char(ch, "Level set.\r\n");
	} else {
		send_to_char(ch, "Dynedit set valid arguments: level.\r\n");
		return;
	}

	if (save_dyntext_control(dyntext))
		send_to_char(ch, "An error occurred while saving the control file.\r\n");
	else
		send_to_char(ch, "Control file saved.\r\n");
}

void
show_dyntext(Creature *ch, dynamic_text_file * dyntext, char *argument)
{
	int i;

	if (dyntext) {

		if (!*argument) {
			send_to_char(ch,
				"DYNTEXT: filename: '%s'\r\n"
				"             last: %s (%ld) @ %s\r"
				"            level: %d\r\n"
				"             lock: %s (%ld)\r\n"
				"              old: %-3s (Len: %d)\r\n"
				"              new: %-3s (Len: %d)\r\n",
				dyntext->filename,
				playerIndex.getName(dyntext->last_edit[0].idnum),
				dyntext->last_edit[0].idnum,
				ctime(&(dyntext->last_edit[0].tEdit)),
				dyntext->level,
				playerIndex.getName(dyntext->lock),
				dyntext->lock,
				YESNO(dyntext->buffer),
				dyntext->buffer ? strlen(dyntext->buffer) : 0,
				YESNO(dyntext->tmp_buffer),
				dyntext->tmp_buffer ? strlen(dyntext->tmp_buffer) : 0);
			return;
		}
		// there was an argument, parse it
		if (is_abbrev(argument, "old")) {
			if (!dyntext->buffer) {
				send_to_char(ch, "There is no old text buffer.\r\n");
			} else {
				page_string(ch->desc, dyntext->buffer);
			}
		} else if (is_abbrev(argument, "new")) {
			if (!dyntext->tmp_buffer) {
				send_to_char(ch, "There is no new text buffer.\r\n");
			} else {
				page_string(ch->desc, dyntext->tmp_buffer);
			}
		} else if (is_abbrev(argument, "perms")) {
			send_to_char(ch, "Permissions defined:\r\n");
			for (i = 0; i < DYN_TEXT_PERM_SIZE; i++) {
				send_to_char(ch, "%3d.] (%5ld) %s\r\n",
					i, dyntext->perms[i], playerIndex.getName(dyntext->perms[i]));
			}
		} else if (is_abbrev(argument, "last")) {
			send_to_char(ch, "Last edits:\r\n");
			for (i = 0; i < DYN_TEXT_HIST_SIZE; i++) {
				send_to_char(ch, "%3d.] (%5ld) %30s @ %s\r",
					i, dyntext->last_edit[i].idnum,
					playerIndex.getName(dyntext->last_edit[i].idnum),
					ctime(&(dyntext->last_edit[i].tEdit)));
			}

		} else {
			send_to_char(ch, "Unknown argument for show.\r\n");
			show_dynedit_options(ch);
		}
		return;
	}
	strcpy(buf, "DYNTEXT LIST:\r\n");
	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next)
		send_to_char(ch, "%s%s\r\n", buf, dyntext->filename);



}

int
dyntext_edit_ok(Creature *ch, dynamic_text_file * dyntext)
{
	int i;

	if (dyntext->level <= GET_LEVEL(ch) || GET_IDNUM(ch) != 1)
		return 1;

	for (i = 0; i < DYN_TEXT_PERM_SIZE; i++) {
		if (dyntext->perms[i] == GET_IDNUM(ch))
			return 1;
	}

	return (!check_editors(ch, &(dyntext->tmp_buffer)));
}

int
dynedit_check_dyntext(Creature *ch, dynamic_text_file * dyntext, char *arg)
{
	if (!dyntext) {
		if (*arg) {
			send_to_char(ch, "No such filename, '%s'\r\n", arg);
		} else {
			send_to_char(ch, "Which dynamic text file?\r\n");
		}
		return 1;
	}
	return 0;
}

ACMD(do_dynedit)
{
	int dyn_com = 0;			// which dynedit command
	long idnum = 0;				// for dynedit add
	int found = 0;				// found variable
	dynamic_text_file *dyntext = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];	// argument vars
	int i;						// counter variable
	char *newbuf = NULL;
	char *s = NULL;				// pointer for update string

	argument = two_arguments(argument, arg1, arg2);

	if (argument)
		skip_spaces(&argument);

	if (!*arg1) {
		show_dynedit_options(ch);
		return;
	}

	for (dyn_com = 0; dynedit_options[dyn_com][0] != NULL; dyn_com++)
		if (is_abbrev(arg1, dynedit_options[dyn_com][0]))
			break;

	if (dynedit_options[dyn_com][0] == NULL) {
		send_to_char(ch, "Unknown option.\r\n");
		show_dynedit_options(ch);
		return;
	}

	if (*arg2) {
		for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next)
			if (!strcasecmp(arg2, dyntext->filename))
				break;
	}

	switch (dyn_com) {

	case 0:					// show
		if (*arg2 && !dyntext) {
			send_to_char(ch, "Unknown dynamic text file, '%s'.\r\n", arg2);
			return;
		}
		show_dyntext(ch, dyntext, argument);
		break;
	case 1:					// set
		set_dyntext(ch, dyntext, argument);
		break;
	case 2:					// add
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (!*argument) {
			send_to_char(ch, "Add who to the permission list?\r\n");
			return;
		}
		if ((idnum = playerIndex.getID(argument)) < 0) {
			send_to_char(ch, "There is no such person.\r\n");
			return;
		}

		for (i = 0, found = 0; i < DYN_TEXT_PERM_SIZE; i++) {
			if (!dyntext->perms[i]) {
				dyntext->perms[i] = idnum;
				send_to_char(ch, "User added.\r\n");

				if (save_dyntext_control(dyntext))
					send_to_char(ch, 
						"An error occurred while saving the control file.\r\n");
				else
					send_to_char(ch, "Control file saved.\r\n");

				return;
			}
		}

		send_to_char(ch, "The permission list is full.\r\n");
		break;

	case 3:					// remove
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (!*argument) {
			send_to_char(ch, "Remove who from the permission list?\r\n");
			return;
		}
		if ((idnum = playerIndex.getID(argument)) < 0) {
			send_to_char(ch, "There is no such person.\r\n");
			return;
		}

		for (i = 0, found = 0; i < DYN_TEXT_PERM_SIZE; i++) {
			if (found) {
				dyntext->perms[i - 1] = dyntext->perms[i];
				dyntext->perms[i] = 0;
				continue;
			}
			if (dyntext->perms[i] == idnum) {
				dyntext->perms[i] = 0;
				found = 1;
			}
		}
		if (found) {
			send_to_char(ch, "User removed from the permission list.\r\n");

			if (save_dyntext_control(dyntext))
				send_to_char(ch, 
					"An error occurred while saving the control file.\r\n");
			else
				send_to_char(ch, "Control file saved.\r\n");

		} else {
			send_to_char(ch, "That user is not on the permisison list.\r\n");
		}

		break;

	case 4:					// edit
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
			send_to_char(ch, "That file is already locked by %s.\r\n",
				playerIndex.getName(dyntext->lock));
			return;
		}
		if (!dyntext_edit_ok(ch, dyntext)) {
			send_to_char(ch, "You cannot edit this file.\r\n");
			return;
		}
		dyntext->lock = GET_IDNUM(ch);

		act("$n begins editing a dynamic text file.", TRUE, ch, 0, 0, TO_ROOM);

		// enter the text editor
		start_editing_text(ch->desc, &dyntext->tmp_buffer,
			MAX_STRING_LENGTH);
		SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

		break;
	case 5:					// abort
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)
			&& GET_LEVEL(ch) < LVL_CREATOR) {
			send_to_char(ch, "That file is already locked by %s.\r\n",
				playerIndex.getName(dyntext->lock));
			return;
		}
		if (!dyntext_edit_ok(ch, dyntext)) {
			send_to_char(ch, "You cannot edit this file.\r\n");
			return;
		}
		dyntext->lock = 0;
		if (dyntext->tmp_buffer) {
			free(dyntext->tmp_buffer);
			dyntext->tmp_buffer = NULL;
		}
		send_to_char(ch, "Buffer edit aborted.\r\n");
		break;

	case 6:					// update

		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)
			&& GET_LEVEL(ch) < LVL_CREATOR) {
			send_to_char(ch, "That file is already locked by %s.\r\n",
				playerIndex.getName(dyntext->lock));
			return;
		}

		if (!dyntext_edit_ok(ch, dyntext)) {
			send_to_char(ch, "You cannot update this file.\r\n");
			return;
		}

		if (!dyntext->tmp_buffer) {
			send_to_char(ch, "There is no need to update this file.\r\n");
			return;
		}
		// make the backup
		if (create_dyntext_backup(dyntext)) {
			send_to_char(ch, "An error occurred while backing up.\r\n");
			return;
		}

		dyntext->lock = 0;

		// update the buffer
		if (dyntext->buffer)
			free(dyntext->buffer);

		dyntext->buffer = dyntext->tmp_buffer;
		dyntext->tmp_buffer = NULL;

		// add update string
		s = dynedit_update_string(dyntext);
		i = strlen(dyntext->buffer) + strlen(s) + 1;

		if (!(newbuf = (char *)malloc(i))) {
			send_to_char(ch, "Unable to allocate newbuf.\r\n");
			errlog(" error allocating newbuf in dynedit update.");
			return;
		}

		*newbuf = '\0';

		strcat(newbuf, s);
		strcat(newbuf, dyntext->buffer);
		free(dyntext->buffer);
		dyntext->buffer = newbuf;


		// save the new file
		if (save_dyntext_buffer(dyntext)) {

			send_to_char(ch, "An error occurred while saving.\r\n");

		} else {

			send_to_char(ch, "Updated and saved successfully.\r\n");

		}


		if (push_update_to_history(ch, dyntext))

			send_to_char(ch, "There was an error updating the history.\r\n");


		if (save_dyntext_control(dyntext))
			send_to_char(ch, "An error occurred while saving the control file.\r\n");
		else
			send_to_char(ch, "Control file saved.\r\n");

		break;

	case 7:					// prepend
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
			send_to_char(ch, "That file is already locked by %s.\r\n",
				playerIndex.getName(dyntext->lock));
			return;
		}
		if (!dyntext_edit_ok(ch, dyntext)) {
			send_to_char(ch, "You cannot edit this file.\r\n");
			return;
		}

		if (!dyntext->buffer) {
			send_to_char(ch, "There is nothing in the old buffer to prepend.\r\n");
		} else {

			if (!dyntext->tmp_buffer) {
				dyntext->tmp_buffer = strdup(dyntext->buffer);
			}

			else {
				if (strlen(dyntext->buffer) + strlen(dyntext->tmp_buffer) >=
					MAX_STRING_LENGTH) {
					send_to_char(ch, 
						"Resulting string would exceed maximum string length, aborting.\r\n");
					return;
				}
				if (!(newbuf =
						(char *)malloc(strlen(dyntext->buffer) +
							strlen(dyntext->tmp_buffer) + 1))) {
					errlog("unable to malloc buffer for prepend in do_dynedit.");
					send_to_char(ch, 
						"Unable to allocate memory for the new buffer.\r\n");
					return;
				}
				*newbuf = '\0';
				strcat(newbuf, dyntext->buffer);
				strcat(newbuf, dyntext->tmp_buffer);
				free(dyntext->tmp_buffer);
				dyntext->tmp_buffer = newbuf;
			}
		}
		send_to_char(ch, "Old buffer prepended to new buffer.\r\n");
		break;

	case 8:					// append
		if (dynedit_check_dyntext(ch, dyntext, arg2))
			return;

		if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
			send_to_char(ch, "That file is already locked by %s.\r\n",
				playerIndex.getName(dyntext->lock));
			return;
		}
		if (!dyntext_edit_ok(ch, dyntext)) {
			send_to_char(ch, "You cannot edit this file.\r\n");
			return;
		}

		if (!dyntext->buffer) {
			send_to_char(ch, "There is nothing in the old buffer to append.\r\n");
		} else {

			if (!dyntext->tmp_buffer) {
				dyntext->tmp_buffer = strdup(dyntext->buffer);
			}

			else {
				if (strlen(dyntext->buffer) + strlen(dyntext->tmp_buffer) >=
					MAX_STRING_LENGTH) {
					send_to_char(ch, 
						"Resulting string would exceed maximum string length, aborting.\r\n");
					return;
				}
				if (!(newbuf =
						(char *)malloc(strlen(dyntext->buffer) +
							strlen(dyntext->tmp_buffer) + 1))) {
					errlog("unable to malloc buffer for append in do_dynedit.");
					send_to_char(ch, 
						"Unable to allocate memory for the new buffer.\r\n");
					return;
				}
				*newbuf = '\0';
				strcat(newbuf, dyntext->tmp_buffer);
				strcat(newbuf, dyntext->buffer);
				free(dyntext->tmp_buffer);
				dyntext->tmp_buffer = newbuf;
			}
		}
		send_to_char(ch, "Old buffer appended to new buffer.\r\n");
		break;
	case 9:{					// reload
			if (dynedit_check_dyntext(ch, dyntext, arg2))
				return;

			if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
				send_to_char(ch, "That file is already locked by %s.\r\n",
					playerIndex.getName(dyntext->lock));
				return;
			}
			if (!dyntext_edit_ok(ch, dyntext)) {
				send_to_char(ch, "You cannot edit this file.\r\n");
				return;
			}
			int rc = reload_dyntext_buffer(dyntext);
			if (rc == 0) {
				send_to_char(ch, "Buffer reloaded.\r\n");
			} else {
				send_to_char(ch, "Error reloading buffer.\r\n");
			}
			break;
		}
	default:					// default
		break;
	}
}

ACMD(do_dyntext_show)
{
	char *dynname;
	char *humanname;
	char color1[16], color2[16], color3[16];
	dynamic_text_file *dyntext = NULL;

	switch (subcmd) {
	case SCMD_DYNTEXT_NEWS:
		dynname = "news";
		humanname = "NEWS";
		break;
	case SCMD_DYNTEXT_INEWS:
		dynname = "inews";
		humanname = "iNEWS";
		break;
	default:
		send_to_char(ch, "Unknown dynamic text request.\r\n");
		return;
	}

	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next)
		if (!strcmp(dyntext->filename, dynname))
			break;

	if (!dyntext) {
		send_to_char(ch, "Sorry, unable to load that dynamic text.\r\n");
		return;
	}

	if (!dyntext->buffer) {
		send_to_char(ch, "That dynamic text buffer is empty.\r\n");
		return;
	}

	if (clr(ch, C_NRM)) {
		sprintf(color1, "\x1B[%dm", number(31, 37));
		sprintf(color2, "\x1B[%dm", number(31, 37));
		sprintf(color3, "\x1B[%dm", number(31, 37));
	} else {
		strcpy(color1, "");
		strcpy(color2, color1);
		strcpy(color3, color1);
	}

    acc_string_clear();
	acc_sprintf(
		"   %s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-%s\r\n"
		"   %s::::::::  :::::::::  ::::::::::::  :::::::::  :::   :::  ::::::::\r\n"
		"     :::    :::        :::  ::  :::  :::    ::  :::   :::  :::\r\n"
		"    :::    :::::::    :::  ::  :::  :::::::::  :::   :::  ::::::::      %s%s%s\r\n"
		"   %s:::    :::        :::  ::  :::  :::        :::   :::       :::\r\n"
		"  :::    :::::::::  :::  ::  :::  :::        :::::::::  ::::::::%s\r\n"
		" %s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-%s\r\n ",
		color1, CCNRM(ch, C_NRM),
		color2, color3, humanname, CCNRM(ch, C_NRM),
		color2, CCNRM(ch, C_NRM), color1, CCNRM(ch, C_NRM));

	acc_strcat(
        tmp_capitalize(
            tmp_sprintf("%s was last updated on %s", dynname,
                        ctime(&(dyntext->last_edit[0].tEdit)))),
        dyntext->buffer,
        NULL);

	page_string(ch->desc, acc_get_string());
}



void
check_dyntext_updates(Creature *ch, int mode)
{
	dynamic_text_file *dyntext = NULL;

	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
		if (dyntext->last_edit[0].tEdit > ch->account->get_entry_time()) {
			if (!strcmp(dyntext->filename, "inews")
				&& GET_LEVEL(ch) < LVL_AMBASSADOR)
				continue;
			if (!strncmp(dyntext->filename, "fate", 4)
				|| !strncmp(dyntext->filename, "arenalist", 9))
				continue;

			if (mode == CHECKDYN_RECONNECT)
				send_to_char(ch,
					"%s [ The %s file was updated while you were disconnected. ]%s\r\n",
					CCYEL(ch, C_NRM), dyntext->filename, CCNRM(ch, C_NRM));
			else
				send_to_char(ch,
					"%s [ The %s file has been updated since your last visit. ]%s\r\n",
					CCYEL(ch, C_NRM), dyntext->filename, CCNRM(ch, C_NRM));

		}
	}
}

char *
dynedit_update_string(dynamic_text_file * d)
{

	struct tm tmTime;
	time_t t;
	static char buffer[1024];
	int len;

	if (!strncmp(d->filename, "fate", 4)
		|| !strncmp(d->filename, "arenalist", 9))
		return "";
	printf("Updating File: %s\r\n", d->filename);
	t = time(0);
	tmTime = *(localtime(&t));

	sprintf(buf, "%s", d->filename);
	len = strlen(buf);

	for (--len; len >= 0; len--)
		buf[len] = toupper(buf[len]);

	sprintf(buffer,
		"\r\n-- %s UPDATE (%d/%d) -----------------------------------------\r\n\n",
		buf, tmTime.tm_mon + 1, tmTime.tm_mday);

	return buffer;

}
