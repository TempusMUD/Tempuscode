//
// File: texteditor.cc                        -- part of TempusMUD
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

void voting_add_poll(void);

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
	struct mail_recipient_data *next_mail;

	// If we were editing rather than creating, wax what was there.
	if (target) {
		if (*target) {
			free(*target);
			*target = NULL;
		}
	}
  
    *target = strdup(text);

	// Save the board if we were writing to a board
	if (desc->mail_to && desc->mail_to->recpt_idnum >= BOARD_MAGIC) {
		gen_board_save(desc->mail_to->recpt_idnum - BOARD_MAGIC, *target);
		free(*target);
		free(target);
		next_mail = desc->mail_to->next;
		free(desc->mail_to);
		desc->mail_to = next_mail;
	}
	// Add the poll if we were adding to a poll
	if (desc->mail_to && desc->mail_to->recpt_idnum == VOTING_MAGIC) {
		voting_add_poll();
		next_mail = desc->mail_to->next;
		free(desc->mail_to);
		desc->mail_to = next_mail;
	}
	// If editing thier description.
	if (STATE(desc) == CXN_EDIT_DESC) {
		send_to_desc(desc, "\033[H\033[J");
		set_desc_state(CXN_MENU, desc);
		return;
	}

    if (IS_PLAYING(desc)) {
        if (PLR_FLAGGED(desc->creature, PLR_OLC))
            act("$n nods with satisfaction as $e saves $s work.", false, desc->creature, 0, 0, TO_NOTVICT);
        else
            act("$n finishes writing.", false, desc->creature, 0, 0, TO_NOTVICT);
    }
}
