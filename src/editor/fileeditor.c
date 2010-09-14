//
// File: texteditor.cc                        -- part of TempusMUD
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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

extern const int max_filesize;
#define MAX_EDIT_FILESIZE (max_filesize + 128)

void
fileeditor_finalize(struct editor *editor, const char *text)
{
    FILE *fd = NULL;

    if ((fd = fopen((char *)editor->mode_data, "w"))) {
        fwrite(text, 1, strlen(text), fd);
        fclose(fd);
    } else {
        send_to_char(editor->desc->creature, "Your file was not saved.  Please bug this.\r\n");
    }

    REMOVE_BIT(PLR_FLAGS(editor->desc->creature), PLR_WRITING);

    if (IS_PLAYING(editor->desc))
        act("$n finishes editing.", true, editor->desc->creature, 0, 0, TO_NOTVICT);
}

void
start_editing_file(struct descriptor_data *d, const char *fname)
{
    FILE *inf = NULL;
    char *target;
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

	inf = fopen(fname, "r");
    if (inf) {
        CREATE(target, char, sbuf.st_size + 1);
        if (!target) {
            errlog("Couldn't allocate memory to edit %s", fname);
            send_to_desc(d, "Internal error #09384\r\n");
            return;
        }

        if (fread(target, sizeof(char), sbuf.st_size, inf) != (size_t)sbuf.st_size) {
            send_to_desc(d, "Internal error #42372\r\n");
            free(target);
            fclose(inf);
            return;
        }

        d->text_editor = make_editor(d, MAX_EDIT_FILESIZE);
        editor_import(d->text_editor, target);

        SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING);

        d->text_editor->finalize = fileeditor_finalize;
        d->text_editor->mode_data = strdup(fname);
        free(target);
        fclose(inf);

        emit_editor_startup(d->text_editor);
        editor_display(d->text_editor, 0, 0);
    }
}
