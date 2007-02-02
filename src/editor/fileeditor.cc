//
// File: texteditor.cc                        -- part of TempusMUD
// 

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <list>
using namespace std;
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

// Tempus Includes
#include "screen.h"
#include "desc_data.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "login.h"
#include "interpreter.h"
#include "boards.h"
#include "mail.h"
#include "editor.h"
#include "tmpstr.h"
#include "accstr.h"
#include "help.h"
#include "comm.h"
#include "player_table.h"

extern int max_filesize;
const int MAX_EDIT_FILESIZE = max_filesize + 128;
void
start_editing_file(struct descriptor_data *d, const char *fname)
{
    struct stat sbuf;

    if (!d) {
        errlog("NULL desriptor passed into start_editing_file()!!");
        return;
    }
	if (!fname) {
		errlog("NULL file name pointer passed into start_editing_file()!!");
		send_to_char(d->creature, "This command seems to be broken. Bug this.\r\n");
		return;
	}
	if (d->text_editor) {
		errlog("Text editor object not null in start_editing_file().");
		return;
	}

    if (access(fname, W_OK | R_OK)) {
        errlog("Unable to edit requested file (%s) in start_editing_file()!",
               fname);
        send_to_char(d->creature, "I'm unable to edit %s.  Bug this.\r\n",
                     fname);
        return;
    }

    stat(fname, &sbuf);
    if (sbuf.st_size > MAX_EDIT_FILESIZE) {
        errlog("Attempt to edit file too large for editor!");
        send_to_char(d->creature, "%s is too large. Bug this.\r\n",
                     fname);
        return;
    }

	d->text_editor = new CFileEditor(d, fname);
    SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING);
}

CFileEditor::CFileEditor(descriptor_data *desc, const char *filename)
    : CEditor(desc, MAX_EDIT_FILESIZE), fname(filename)
{
    FILE *fd = NULL;
    struct stat sbuf;
    char *target;

    loaded = true;
    wrap = false;

    if (stat(filename, &sbuf) < 0) {
		errlog("Couldn't open %s for editing: %s", filename, strerror(errno));
		loaded = false;
		return;
	}
	CREATE(target, char, sbuf.st_size + 1);
	if (!target) {
		errlog("Couldn't allocate memory to edit %s", filename);
        loaded = false;
		return;
	}

	fd = fopen(filename, "r");
    if (fd && fread(target, sizeof(char), sbuf.st_size, fd) == (size_t)sbuf.st_size) {
		if (*target)
			ImportText(target);

		SendStartupMessage();
		DisplayBuffer();
	} else {
        sprintf(target, "An unknown error occured while reading the file. "
                        "Please bug this.  You will not be able to save.");
        wrap = true;
    }
	
	if (target)
		free(target);
	if (fd)
		fclose(fd);
}

CFileEditor::~CFileEditor(void)
{
}

bool
CFileEditor::PerformCommand(char cmd, char *args)
{
    switch (cmd) {
    case 'u':
        UndoChanges();
        break;
    default:
        return CEditor::PerformCommand(cmd, args);
    }

    return true;
}

void
CFileEditor::Finalize(const char *text)
{
    FILE *fd = NULL;

    if (loaded) {
        if ((fd = fopen(fname.c_str(), "w"))) {
            list<string>::iterator li = theText.begin();
            for (; li != theText.end(); ++li) {
                fwrite((*li).c_str(), (*li).length(), 1, fd);
                fwrite("\n", 1, 1, fd);
            }
        }
    }

    REMOVE_BIT(PLR_FLAGS(desc->creature), PLR_WRITING);

    if (IS_PLAYING(desc))
        act("$n finishes editing.", false, desc->creature, 0, 0, TO_NOTVICT);

    if (!fd)
        send_to_char(desc->creature, "Your file was not saved.  Please bug this.\r\n");
    else
        fclose(fd);
}
