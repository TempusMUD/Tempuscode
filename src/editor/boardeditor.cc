//
// File: boardeditor.cc                        -- part of TempusMUD
// 

#include <string>
#include <list>
using namespace std;
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
#include "player_table.h"

void
start_editing_board(descriptor_data *d,
                    const char *b_name,
                    const char *subject)
{
	if (d->text_editor) {
		errlog("Text editor object not null in start_editing_mail");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}

    SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING);

	d->text_editor = new CBoardEditor(d, b_name, subject);
}



CBoardEditor::CBoardEditor(descriptor_data *desc,
                           const char *b_name,
                           const char *s)
    : CEditor(desc, MAX_STRING_LENGTH)
{
    board_name = strdup(b_name);
    subject = strdup(s);

    SendStartupMessage();
    DisplayBuffer();
}

CBoardEditor::~CBoardEditor(void)
{
    free(board_name);
    free(subject);
}

bool
CBoardEditor::PerformCommand(char cmd, char *args)
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
CBoardEditor::Finalize(const char *text)
{
    gen_board_save(desc->creature, board_name, subject, text);

    if (IS_PLAYING(desc))
        act("$n nods with satisfaction as $e saves $s work.", false, desc->creature, 0, 0, TO_NOTVICT);
}
