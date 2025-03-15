//
// File: dyntext.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "libpq-fe.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "players.h"
#include "tmpstr.h"
#include "str_builder.h"
#include "strutil.h"
#include "prog.h"
#include "editor.h"
#include "security.h"

// external variables

// local variables
dynamic_text_file *dyntext_list = NULL;

/***
 *
 * Dynamic text file functions
 *
 ***/

static int
load_dyntext_buffer(dynamic_text_file *dyntext)
{
    char *path = tmp_sprintf("text/%s", dyntext->filename);
    FILE *fl = fopen(path, "r");
    if (fl == NULL) {
        errlog("unable to open dynamic text file '%s'.", path);
        perror("dyntext fopen:");
        return -1;
    }

    char line[1024];

    struct str_builder sb = str_builder_default;
    while (fgets(line, 1024, fl)) {
        sb_strcat(&sb, line, NULL);
    }
    sb_strcat(&sb, "\n", NULL);
    free(dyntext->buffer);
    dyntext->buffer = strdup(tmp_gsub(sb.str, "\n", "\r\n"));
    fclose(fl);
    return 0;
}

void
boot_dynamic_text(void)
{
    DIR *dir;
    struct dirent *dirp;
    dynamic_text_file *newdyn = NULL;
    dynamic_text_file_save savedyn;
    unsigned int i;

    if (!(dir = opendir(DYN_TEXT_CONTROL_DIR))) {
        errlog("Cannot open dynamic text control dir.");
        safe_exit(1);
    }

    while ((dirp = readdir(dir))) {
        size_t br;

        if ((strlen(dirp->d_name) < 4)
            || strcmp(dirp->d_name + strlen(dirp->d_name) - 4, ".dyn") != 0) {
            continue;
        }

        char *path = tmp_sprintf("%s/%s", DYN_TEXT_CONTROL_DIR, dirp->d_name);
        FILE *fl = fopen(path, "r");
        if (fl == NULL) {
            errlog("error opening dynamic control file '%s'.", path);
            perror("fopen:");
            continue;
        }

        CREATE(newdyn, dynamic_text_file, 1);
        if (newdyn == NULL) {
            errlog("error allocating dynamic control block, aborting.");
            closedir(dir);
            fclose(fl);
            return;
        }

        br = fread(&savedyn, sizeof(dynamic_text_file_save), 1, fl);
        if (br == 0) {
            errlog("error reading information from '%s'.", path);
            free(newdyn);
            fclose(fl);
            continue;
        }
        fclose(fl);

        strcpy_s(newdyn->filename, sizeof(newdyn->filename), savedyn.filename);
        for (i = 0; i < DYN_TEXT_HIST_SIZE; i++) {
            newdyn->last_edit[i].idnum = savedyn.last_edit[i].idnum;
            newdyn->last_edit[i].tEdit = savedyn.last_edit[i].tEdit;
        }
        for (i = 0; i < DYN_TEXT_PERM_SIZE; i++) {
            newdyn->perms[i] = savedyn.perms[i];
        }

        newdyn->level = savedyn.level;
        newdyn->lock = 0;
        newdyn->next = NULL;
        newdyn->buffer = NULL;
        newdyn->tmp_buffer = NULL;

        if (load_dyntext_buffer(newdyn) == 0) {
            slog("dyntext BOOTED %s.", dirp->d_name);
        } else {
            errlog("dyntext FAILED TO BOOT %s.", dirp->d_name);
        }

        newdyn->next = dyntext_list;
        dyntext_list = newdyn;
    }

    closedir(dir);

}

static int
create_dyntext_backup(dynamic_text_file *dyntext)
{

    DIR *dir;
    struct dirent *dirp;
    int len = strlen(dyntext->filename);
    int num = 0, maxnum = 0;
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

            if (num > maxnum) {
                maxnum = num;
            }

        }
    }

    closedir(dir);

    char *filename = tmp_sprintf("%s/%s.%02d", DYN_TEXT_BACKUP_DIR, dyntext->filename,
             maxnum + 1);

    if (!(fl = fopen(filename, "w"))) {
        errlog("Dyntext backup unable to open '%s'.", filename);
        return 1;
    }

    if (dyntext->buffer) {
        char *ptr;
        for (ptr = dyntext->buffer; *ptr; ptr++) {
            if (*ptr == '\r') {
                continue;
            }
            fputc(*ptr, fl);
        }
    }

    fclose(fl);
    return 0;
}

static int
save_dyntext_buffer(dynamic_text_file *dyntext)
{
    FILE *fl = NULL;
    char *ptr = NULL;

    const char *filename = tmp_sprintf("text/%s", dyntext->filename);

    if (!(fl = fopen(filename, "w"))) {
        errlog("Unable to open '%s' for write.", filename);
        return 1;
    }

    if (dyntext->buffer) {
        for (ptr = dyntext->buffer; *ptr; ptr++) {
            if (*ptr == '\r') {
                continue;
            }
            fputc(*ptr, fl);
        }
    }

    fclose(fl);

    return 0;

}

static int
save_dyntext_control(dynamic_text_file *dyntext)
{
    FILE *fl = NULL;
    dynamic_text_file_save savedyn;
    int i;

    const char *filename = tmp_sprintf("%s/%s.dyn", DYN_TEXT_CONTROL_DIR, dyntext->filename);

    if (!(fl = fopen(filename, "w"))) {
        errlog("Unable to open '%s' for write.", filename);
        return 1;
    }

    strcpy_s(savedyn.filename, sizeof(savedyn.filename), dyntext->filename);
    for (i = 0; i < DYN_TEXT_HIST_SIZE; i++) {
        savedyn.last_edit[i].idnum = dyntext->last_edit[i].idnum;
        savedyn.last_edit[i].tEdit = dyntext->last_edit[i].tEdit;
    }
    for (i = 0; i < DYN_TEXT_PERM_SIZE; i++) {
        savedyn.perms[i] = dyntext->perms[i];
    }
    savedyn.level = dyntext->level;
    savedyn.unused_ptr1 = 0;
    savedyn.unused_ptr2 = 0;
    savedyn.unused_ptr3 = 0;

    if (!(fwrite(&savedyn, sizeof(dynamic_text_file_save), 1, fl))) {
        errlog("Unable to write data to '%s'.", filename);
        fclose(fl);
        return 1;
    }

    fclose(fl);
    return 0;
}

static int
push_update_to_history(struct creature *ch, dynamic_text_file *dyntext)
{
    for (int i = DYN_TEXT_HIST_SIZE - 1; i > 0; i--) {
        dyntext->last_edit[i] = dyntext->last_edit[i - 1];
    }

    dyntext->last_edit[0].idnum = GET_IDNUM(ch);
    dyntext->last_edit[0].tEdit = time(NULL);

    return 0;

}

static const char *dynedit_options[][2] = {
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

static void
show_dynedit_options(struct creature *ch)
{
    int i = 0;

    send_to_char(ch, "Dynedit usage:\r\n");

    for (i = 0; dynedit_options[i][0] != NULL; i++) {
        send_to_char(ch, "%10s   %s\r\n", dynedit_options[i][0],
                     dynedit_options[i][1]);
    }

}

static void
set_dyntext(struct creature *ch, dynamic_text_file *dyntext, char *argument)
{

    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int lev;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        send_to_char(ch, "Dynedit set requires more arguments.\r\n");
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

    if (save_dyntext_control(dyntext)) {
        send_to_char(ch,
                     "An error occurred while saving the control file.\r\n");
    } else {
        send_to_char(ch, "Control file saved.\r\n");
    }
}

static void
show_dyntext(struct creature *ch, dynamic_text_file *dyntext, char *argument)
{
    int i;

    if (dyntext) {

        if (!*argument) {
            send_to_char(ch,
                         "DYNTEXT: filename: '%s'\r\n"
                         "             last: %s (%d) @ %s\r"
                         "            level: %d\r\n"
                         "             lock: %s (%d)\r\n"
                         "              old: %-3s (Len: %zd)\r\n"
                         "              new: %-3s (Len: %zd)\r\n",
                         dyntext->filename,
                         player_name_by_idnum(dyntext->last_edit[0].idnum),
                         dyntext->last_edit[0].idnum,
                         ctime(&(dyntext->last_edit[0].tEdit)),
                         dyntext->level,
                         player_name_by_idnum(dyntext->lock),
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
                send_to_char(ch, "%3d.] (%5d) %s\r\n",
                             i, dyntext->perms[i],
                             player_name_by_idnum(dyntext->perms[i]));
            }
        } else if (is_abbrev(argument, "last")) {
            send_to_char(ch, "Last edits:\r\n");
            for (i = 0; i < DYN_TEXT_HIST_SIZE; i++) {
                send_to_char(ch, "%3d.] (%5d) %30s @ %s\r",
                             i, dyntext->last_edit[i].idnum,
                             player_name_by_idnum(dyntext->last_edit[i].idnum),
                             ctime(&(dyntext->last_edit[i].tEdit)));
            }

        } else {
            send_to_char(ch, "Unknown argument for show.\r\n");
            show_dynedit_options(ch);
        }
        return;
    }
    send_to_char(ch, "DYNTEXT LIST:\r\n");
    for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
        send_to_char(ch, "%s\r\n", dyntext->filename);
    }

}

static bool
dyntext_edit_ok(struct creature *ch, dynamic_text_file *dyntext)
{
    int i;

    if (dyntext->level <= GET_LEVEL(ch) || GET_IDNUM(ch) != 1) {
        return 1;
    }

    for (i = 0; i < DYN_TEXT_PERM_SIZE; i++) {
        if (dyntext->perms[i] == GET_IDNUM(ch)) {
            return 1;
        }
    }

    return !already_being_edited(ch, dyntext->tmp_buffer);
}

static int
dynedit_check_dyntext(struct creature *ch,
                      dynamic_text_file *dyntext,
                      char *arg)
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

static char *
dynedit_update_string(dynamic_text_file *d)
{

    if (!strncmp(d->filename, "fate", 4)
        || !strncmp(d->filename, "arenalist", 9)) {
        return tmp_strdup("");
    }
    printf("Updating File: %s\r\n", d->filename);
    time_t t = time(NULL);

    struct tm tmTime;
    localtime_r(&t, &tmTime);

    return tmp_sprintf(
        "\r\n-- %s UPDATE (%s) -----------------------------------------\r\n\r\n",
        tmp_toupper(d->filename),
        tmp_strftime("%Y-%m-%d", &tmTime));
}

ACMD(do_dynedit)
{
    int dyn_com = 0;            // which dynedit command
    long idnum = 0;             // for dynedit add
    int found = 0;              // found variable
    dynamic_text_file *dyntext = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];    // argument vars
    int i;                      // counter variable
    char *newbuf = NULL;
    char *s = NULL;             // pointer for update string

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        show_dynedit_options(ch);
        return;
    }

    for (dyn_com = 0; dynedit_options[dyn_com][0] != NULL; dyn_com++) {
        if (is_abbrev(arg1, dynedit_options[dyn_com][0])) {
            break;
        }
    }

    if (dynedit_options[dyn_com][0] == NULL) {
        send_to_char(ch, "Unknown option.\r\n");
        show_dynedit_options(ch);
        return;
    }

    if (*arg2) {
        for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
            if (!strcasecmp(arg2, dyntext->filename)) {
                break;
            }
        }
    }

    switch (dyn_com) {

    case 0:                    // show
        if (*arg2 && !dyntext) {
            send_to_char(ch, "Unknown dynamic text file, '%s'.\r\n", arg2);
            return;
        }
        show_dyntext(ch, dyntext, argument);
        break;
    case 1:                    // set
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }
        set_dyntext(ch, dyntext, argument);
        break;
    case 2:                    // add
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (!*argument) {
            send_to_char(ch, "Add who to the permission list?\r\n");
            return;
        }
        if ((idnum = player_idnum_by_name(argument)) < 0) {
            send_to_char(ch, "There is no such person.\r\n");
            return;
        }

        for (i = 0, found = 0; i < DYN_TEXT_PERM_SIZE; i++) {
            if (!dyntext->perms[i]) {
                dyntext->perms[i] = idnum;
                send_to_char(ch, "User added.\r\n");

                if (save_dyntext_control(dyntext)) {
                    send_to_char(ch,
                                 "An error occurred while saving the control file.\r\n");
                } else {
                    send_to_char(ch, "Control file saved.\r\n");
                }

                return;
            }
        }

        send_to_char(ch, "The permission list is full.\r\n");
        break;

    case 3:                    // remove
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (!*argument) {
            send_to_char(ch, "Remove who from the permission list?\r\n");
            return;
        }
        if ((idnum = player_idnum_by_name(argument)) < 0) {
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

            if (save_dyntext_control(dyntext)) {
                send_to_char(ch,
                             "An error occurred while saving the control file.\r\n");
            } else {
                send_to_char(ch, "Control file saved.\r\n");
            }

        } else {
            send_to_char(ch, "That user is not on the permisison list.\r\n");
        }

        break;

    case 4:                    // edit
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
            return;
        }
        if (!dyntext_edit_ok(ch, dyntext)) {
            send_to_char(ch, "You cannot edit this file.\r\n");
            return;
        }
        dyntext->lock = GET_IDNUM(ch);

        act("$n begins editing a dynamic text file.", true, ch, NULL, NULL, TO_ROOM);

        // enter the text editor
        start_editing_text(ch->desc, &dyntext->tmp_buffer, MAX_STRING_LENGTH);
        SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

        break;
    case 5:                    // abort
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)
            && GET_LEVEL(ch) < LVL_CREATOR) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
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

    case 6:                    // update

        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)
            && GET_LEVEL(ch) < LVL_CREATOR) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
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
        if (dyntext->buffer) {
            free(dyntext->buffer);
        }

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

        strcat_s(newbuf, i, s);
        strcat_s(newbuf, i, dyntext->buffer);
        free(dyntext->buffer);
        dyntext->buffer = newbuf;

        // save the new file
        if (save_dyntext_buffer(dyntext)) {

            send_to_char(ch, "An error occurred while saving.\r\n");

        } else {

            send_to_char(ch, "Updated and saved successfully.\r\n");

        }

        if (push_update_to_history(ch, dyntext)) {

            send_to_char(ch, "There was an error updating the history.\r\n");
        }

        if (save_dyntext_control(dyntext)) {
            send_to_char(ch,
                         "An error occurred while saving the control file.\r\n");
        } else {
            send_to_char(ch, "Control file saved.\r\n");
        }

        break;

    case 7:                    // prepend
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
            return;
        }
        if (!dyntext_edit_ok(ch, dyntext)) {
            send_to_char(ch, "You cannot edit this file.\r\n");
            return;
        }

        if (!dyntext->buffer) {
            send_to_char(ch,
                         "There is nothing in the old buffer to prepend.\r\n");
        } else {

            if (!dyntext->tmp_buffer) {
                dyntext->tmp_buffer = strdup(dyntext->buffer);
            } else {
                if (strlen(dyntext->buffer) + strlen(dyntext->tmp_buffer) >=
                    MAX_STRING_LENGTH) {
                    send_to_char(ch,
                                 "Resulting string would exceed maximum string length, aborting.\r\n");
                    return;
                }
                size_t newbuf_size = strlen(dyntext->buffer) +
                                     strlen(dyntext->tmp_buffer) + 1;
                if (!(newbuf =
                          (char *)malloc(newbuf_size))) {
                    errlog
                        ("unable to malloc buffer for prepend in do_dynedit.");
                    send_to_char(ch,
                                 "Unable to allocate memory for the new buffer.\r\n");
                    return;
                }
                *newbuf = '\0';
                strcat_s(newbuf, newbuf_size, dyntext->buffer);
                strcat_s(newbuf, newbuf_size, dyntext->tmp_buffer);
                free(dyntext->tmp_buffer);
                dyntext->tmp_buffer = newbuf;
            }
        }
        send_to_char(ch, "Old buffer prepended to new buffer.\r\n");
        break;

    case 8:                    // append
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
            return;
        }
        if (!dyntext_edit_ok(ch, dyntext)) {
            send_to_char(ch, "You cannot edit this file.\r\n");
            return;
        }

        if (!dyntext->buffer) {
            send_to_char(ch,
                         "There is nothing in the old buffer to append.\r\n");
        } else {

            if (!dyntext->tmp_buffer) {
                dyntext->tmp_buffer = strdup(dyntext->buffer);
            } else {
                if (strlen(dyntext->buffer) + strlen(dyntext->tmp_buffer) >=
                    MAX_STRING_LENGTH) {
                    send_to_char(ch,
                                 "Resulting string would exceed maximum string length, aborting.\r\n");
                    return;
                }
                size_t newbuf_size = strlen(dyntext->buffer) + strlen(dyntext->tmp_buffer) + 1;
                newbuf = (char *)malloc(newbuf_size);
                if (newbuf == NULL) {
                    errlog("unable to malloc buffer for append in do_dynedit.");
                    send_to_char(ch, "Unable to allocate memory for the new buffer.\r\n");
                    return;
                }
                *newbuf = '\0';
                strcat_s(newbuf, newbuf_size, dyntext->tmp_buffer);
                strcat_s(newbuf, newbuf_size, dyntext->buffer);
                free(dyntext->tmp_buffer);
                dyntext->tmp_buffer = newbuf;
            }
        }
        send_to_char(ch, "Old buffer appended to new buffer.\r\n");
        break;
    case 9: {                   // reload
        if (dynedit_check_dyntext(ch, dyntext, arg2)) {
            return;
        }

        if (dyntext->lock && dyntext->lock != GET_IDNUM(ch)) {
            send_to_char(ch, "That file is already locked by %s.\r\n",
                         player_name_by_idnum(dyntext->lock));
            return;
        }
        if (!dyntext_edit_ok(ch, dyntext)) {
            send_to_char(ch, "You cannot edit this file.\r\n");
            return;
        }
        int rc = load_dyntext_buffer(dyntext);
        if (rc == 0) {
            send_to_char(ch, "Buffer reloaded.\r\n");
        } else {
            send_to_char(ch, "Error reloading buffer.\r\n");
        }
        break;
    }
    default:                   // default
        break;
    }
}

ACMD(do_dyntext_show)
{
    const char *dynname;
    const char *humanname;
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

    for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
        if (streq(dyntext->filename, dynname)) {
            break;
        }
    }

    if (!dyntext) {
        send_to_char(ch, "Sorry, unable to load that dynamic text.\r\n");
        return;
    }

    if (!dyntext->buffer) {
        send_to_char(ch, "That dynamic text buffer is empty.\r\n");
        return;
    }

    if (clr(ch, C_NRM)) {
        snprintf(color1, sizeof(color1), "\x1B[%dm", number(31, 37));
        snprintf(color2, sizeof(color2), "\x1B[%dm", number(31, 37));
        snprintf(color3, sizeof(color3), "\x1B[%dm", number(31, 37));
    } else {
        strcpy_s(color1, sizeof(color1), "");
        strcpy_s(color2, sizeof(color2), color1);
        strcpy_s(color3, sizeof(color3), color1);
    }

    struct str_builder sb = str_builder_default;
    sb_sprintf(&sb, "   %s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-%s\r\n"
         "   %s_::::::  :::::::::  ::::::::::::  :::::::::  :::   :::  ::::::::\r\n"
         "     _:    :::        :::  ::  :::  :::    ::  :::   :::  :::\r\n"
         "    _:    :::::::    :::  ::  :::  :::::::::  :::   :::  ::::::::      %s%s%s\r\n"
         "   %s_:    :::        :::  ::  :::  :::        :::   :::       :::\r\n"
         "  _:    :::::::::  :::  ::  :::  :::        :::::::::  ::::::::%s\r\n"
         " %s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-%s\r\n ",
        color1, CCNRM(ch, C_NRM), color2, color3, humanname, CCNRM(ch, C_NRM),
        color2, CCNRM(ch, C_NRM), color1, CCNRM(ch, C_NRM));

    sb_strcat(&sb, tmp_capitalize(tmp_sprintf("%s was last updated on %s\r\n", dynname,
                                          tmp_ctime(dyntext->last_edit[0].tEdit))),
               dyntext->buffer,
               NULL);

    page_string(ch->desc, sb.str);
}

void
check_dyntext_updates(struct creature *ch)
{
    dynamic_text_file *dyntext = NULL;

    for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
        if (dyntext->last_edit[0].tEdit > ch->account->entry_time) {
            if (streq(dyntext->filename, "inews")
                && GET_LEVEL(ch) < LVL_AMBASSADOR) {
                continue;
            }
            if (streq(dyntext->filename, "tnews")
                && GET_LEVEL(ch) < LVL_AMBASSADOR && !is_tester(ch)) {
                continue;
            }
            if (!strncmp(dyntext->filename, "fate", 4)
                || !strncmp(dyntext->filename, "arenalist", 9)) {
                continue;
            }

            send_to_char(ch, "%s [ The %s file has been updated. Use the %s command to view. ]%s\r\n",
                         CCYEL(ch, C_NRM), dyntext->filename, dyntext->filename, CCNRM(ch, C_NRM));

        }
    }
}
