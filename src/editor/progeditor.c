//
// File: progeditor.cc                        -- part of TempusMUD
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

struct progeditor_data {
    void *owner;
    enum prog_evt_type owner_type;
};

void
progeditor_finalize(struct editor *editor, const char *text)
{
    struct progeditor_data *prog_data =
        (struct progeditor_data *)editor->mode_data;
    char *new_prog = (*text) ? strdup(text) : NULL;

    switch (prog_data->owner_type) {
    case PROG_TYPE_MOBILE:
        if (GET_NPC_PROG((struct creature *)prog_data->owner))
            free(GET_NPC_PROG((struct creature *)prog_data->owner));
        ((struct creature *)prog_data->owner)->mob_specials.shared->prog =
            new_prog;
        break;
    case PROG_TYPE_ROOM:
        if (GET_ROOM_PROG((struct room_data *)prog_data->owner))
            free(((struct room_data *)prog_data->owner)->prog);
        ((struct room_data *)prog_data->owner)->prog = new_prog;
        break;
    default:
        errlog("Can't happen");
    }

    prog_compile(editor->desc->creature, prog_data->owner,
        prog_data->owner_type);

    if (IS_PLAYING(editor->desc))
        act("$n nods with satisfaction as $e saves $s work.",
            true, editor->desc->creature, NULL, NULL, TO_NOTVICT);
    free(prog_data);
}

void
progeditor_cancel(struct editor *editor)
{
    free(editor->mode_data);
}

void
start_editing_prog(struct descriptor_data *d,
    void *owner, enum prog_evt_type owner_type)
{
    struct progeditor_data *prog_data;
    if (d->text_editor) {
        errlog("Text editor object not null in start_editing_prog");
        REMOVE_BIT(PLR_FLAGS(d->creature),
                   PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }

    SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING | PLR_OLC);

    d->text_editor = make_editor(d, MAX_STRING_LENGTH);
    CREATE(prog_data, struct progeditor_data, 1);
    d->text_editor->finalize = progeditor_finalize;
    d->text_editor->cancel = progeditor_cancel;
    d->text_editor->mode_data = prog_data;

    prog_data->owner = owner;
    prog_data->owner_type = owner_type;

    char *text = prog_get_text(owner, owner_type);
    if (text)
        editor_import(d->text_editor, text);

    emit_editor_startup(d->text_editor);
}
