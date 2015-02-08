//
// File: boardeditor.cc                        -- part of TempusMUD
//

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

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
#include "boards.h"

struct board_data {
    int idnum;
    char *board;
    char *subject;
};

bool
board_command(struct editor *editor, char cmd, char *args __attribute__((unused)))
{
    if (cmd == 'u') {
        editor_undo(editor);
        return true;
    }
    return editor_do_command(editor, cmd, args);
}

void
board_finalize(struct editor *editor, const char *text)
{
    struct board_data *data = (struct board_data *)editor->mode_data;
    gen_board_save(editor->desc->creature,
                   data->board, data->idnum, data->subject, text);

    if (IS_PLAYING(editor->desc)) {
        act("$n nods with satisfaction as $e saves $s work.", true,
            editor->desc->creature, NULL, NULL, TO_NOTVICT);
    }
    free(((struct board_data *)editor->mode_data)->board);
    free(((struct board_data *)editor->mode_data)->subject);
    free(editor->mode_data);
}

void
board_cancel(struct editor *editor)
{
    if (IS_PLAYING(editor->desc)) {
        act("$n's awareness returns to $s surroundings.", true,
            editor->desc->creature, NULL, NULL, TO_NOTVICT);
    }
    free(((struct board_data *)editor->mode_data)->board);
    free(((struct board_data *)editor->mode_data)->subject);
    free(editor->mode_data);
}

void
start_editing_board(struct descriptor_data *d,
                    const char *b_name, int idnum, const char *subject, const char *body)
{
    if (d->text_editor) {
        errlog("Text editor object not null in start_editing_mail");
        REMOVE_BIT(PLR_FLAGS(d->creature),
                   PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }

    SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING);

    struct board_data *data;

    CREATE(data, struct board_data, 1);
    data->idnum = idnum;
    data->board = strdup(b_name);
    data->subject = strdup(subject);

    d->text_editor = make_editor(d, MAX_STRING_LENGTH);
    d->text_editor->do_command = board_command;
    d->text_editor->finalize = board_finalize;
    d->text_editor->cancel = board_cancel;
    d->text_editor->mode_data = data;

    if (body) {
        editor_import(d->text_editor, body);
    }

    emit_editor_startup(d->text_editor);
}
