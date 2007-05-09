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
start_editing_text(struct descriptor_data *d, char **dest, int max)
{
	/*  Editor Command
	   Note: Add info for logall
	 */
	// There MUST be a destination!
	if (!dest) {
		errlog("NULL destination pointer passed into start_text_editor!!");
		send_to_char(d->creature, "This command seems to be broken. Bug this.\r\n");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}
	if (d->text_editor) {
		errlog("Text editor object not null in start_text_editor.");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}
	if (*dest && (strlen(*dest) > (unsigned int)max)) {
		send_to_char(d->creature, "ERROR: Buffer too large for editor.\r\n");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}

	d->text_editor = new CTextEditor(d, dest, max);
}

CTextEditor::CTextEditor(descriptor_data *desc,
                         char **target_ptr,
                         int max)
    : CEditor(desc, max)
{
    target = target_ptr;
    if (*target)
        ImportText(*target);
    SendStartupMessage();
    DisplayBuffer();
}

CTextEditor::~CTextEditor(void)
{
}

bool
CTextEditor::PerformCommand(char cmd, char *args)
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
CTextEditor::Finalize(const char *text)
{
	// If we were editing rather than creating, wax what was there.
	if (target) {
		if (*target) {
			free(*target);
			*target = NULL;
		}
	}
  
    *target = strdup(text);

	// If editing thier description.
	if (STATE(desc) == CXN_EDIT_DESC) {
		send_to_desc(desc, "\033[H\033[J");
		set_desc_state(CXN_MENU, desc);
		return;
	}

    if (IS_PLAYING(desc)) {
        if (PLR_FLAGGED(desc->creature, PLR_OLC))
            act("$n nods with satisfaction as $e saves $s work.", true, desc->creature, 0, 0, TO_NOTVICT);
        else
            act("$n finishes writing.", true, desc->creature, 0, 0, TO_NOTVICT);
    }
}
