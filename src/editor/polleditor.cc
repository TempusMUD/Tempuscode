//
// File: polleditor.cc                        -- part of TempusMUD
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

void voting_add_poll(const char *header, const char *text);

void
start_editing_poll(descriptor_data *d, const char *header)
{
	if (d->text_editor) {
		errlog("Text editor object not null in start_editing_mail");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}

    SET_BIT(PLR_FLAGS(d->creature), PLR_WRITING);

	d->text_editor = new CPollEditor(d, header);
}

CPollEditor::CPollEditor(descriptor_data *desc, const char *h)
    : CEditor(desc, MAX_STRING_LENGTH)
{
    header = strdup(h);

    SendStartupMessage();
    DisplayBuffer();
}

CPollEditor::~CPollEditor(void)
{
    free(header);
}

void
CPollEditor::Finalize(const char *text)
{
    voting_add_poll(header, text);

    if (IS_PLAYING(desc))
        act("$n nods with satisfaction as $e saves $s work.", false, desc->creature, 0, 0, TO_NOTVICT);
}
