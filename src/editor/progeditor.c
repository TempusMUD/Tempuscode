//
// File: progeditor.cc                        -- part of TempusMUD
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <fcntl.h>
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

struct progeditor_data {
    void *owner;
    enum prog_evt_type owner_type;
};

void
progeditor_finalize(struct editor *editor, const char *text)
{
    struct progeditor_data *prog_data = (struct progeditor_data *)editor->mode_data;
    char *new_prog = (*text) ? strdup(text):NULL;

    switch (prog_data->owner_type) {
    case PROG_TYPE_MOBILE:
        if (GET_MOB_PROG((struct creature *)prog_data->owner))
            free(GET_MOB_PROG((struct creature *)prog_data->owner));
        ((struct creature *)prog_data->owner)->mob_specials.shared->prog = new_prog;
        break;
    case PROG_TYPE_ROOM:
        if (GET_ROOM_PROG((struct room_data *)prog_data->owner))
            free(((struct room_data *)prog_data->owner)->prog);
        ((struct room_data *)prog_data->owner)->prog = new_prog;
        break;
    default:
        errlog("Can't happen");
    }

    prog_compile(editor->desc->creature, prog_data->owner, prog_data->owner_type);

    if (IS_PLAYING(editor->desc))
        act("$n nods with satisfaction as $e saves $s work.",
            true, editor->desc->creature, 0, 0, TO_NOTVICT);
    free(prog_data);
}

void
progeditor_cancel(struct editor *editor)
{
    free(editor->mode_data);
}

void
start_editing_prog(struct descriptor_data *d,
                   void *owner,
                   enum prog_evt_type owner_type)
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

    emit_editor_startup(d->text_editor);
    editor_display(d->text_editor, 0, 0);
}

