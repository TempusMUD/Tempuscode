//
// File: texteditor.cc                        -- part of TempusMUD
//

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <glib.h>
#include <errno.h>

#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "prog.h"
#include "editor.h"

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
        send_to_char(editor->desc->creature,
            "Your file was not saved.  Please bug this.\r\n");
    }

    REMOVE_BIT(PLR_FLAGS(editor->desc->creature), PLR_WRITING);

    if (IS_PLAYING(editor->desc))
        act("$n finishes editing.", true, editor->desc->creature, NULL, NULL,
            TO_NOTVICT);
}

void
start_editing_file(struct descriptor_data *d, const char *fname)
{
    FILE *inf = NULL;
    char *target;

    if (!d) {
        errlog("NULL desriptor passed into start_editing_file()!!");
        return;
    }
    if (!fname) {
        errlog("NULL file name pointer passed into start_editing_file()!!");
        d_printf(d, "This command seems to be broken. Bug this.\r\n");
        return;
    }
    if (d->text_editor) {
        errlog("Text editor object not null in start_editing_file().");
        return;
    }

    inf = fopen(fname, "r");
    if (inf == NULL) {
        errlog("File editor couldn't open file %s: %s", fname, strerror(errno));
        d_printf(d, "Couldn't open file.  Sorry.\r\n");
        return;
    }
    
    int err = fseek(inf, 0, SEEK_END);
    if (err < 0) {
        errlog("Call to fseek() failed: %s", strerror(errno));
        d_printf(d, "Couldn't open file.  Sorry.\r\n");
        fclose(inf);
        return;
    }

    long len = ftell(inf);
    if (len < 0) {
        errlog("Call to ftell() failed: %s", strerror(errno));
        d_printf(d, "Couldn't open file.  Sorry.\r\n");
        fclose(inf);
        return;
    }

    rewind(inf);

    if (len > MAX_EDIT_FILESIZE) {
        errlog("Attempt to edit file too large for editor!");
        d_printf(d, "%s is too large. Bug this.\r\n", fname);
        return;
    }

    
    CREATE(target, char, (size_t)len + 1);
    if (!target) {
        errlog("Couldn't allocate memory to edit %s", fname);
        d_send(d, "Internal error #09384\r\n");
        return;
    }

    if (fread(target, sizeof(char), (size_t)len, inf) != (size_t) len) {
        d_printf(d, "Internal error #42372\r\n");
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
}
