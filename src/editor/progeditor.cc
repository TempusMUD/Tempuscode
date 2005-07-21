//
// File: progeditor.cc                        -- part of TempusMUD
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

CProgEditor::CProgEditor(descriptor_data *desc,
                         void *o,
                         prog_evt_type t)
    : CEditor(desc, MAX_STRING_LENGTH)
{
    owner = o;
    owner_type = t;
    ImportText(prog_get_text(owner, owner_type));

    SendStartupMessage();
    DisplayBuffer();
}

CProgEditor::~CProgEditor(void)
{
}

bool
CProgEditor::PerformCommand(char cmd, char *args)
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
CProgEditor::Finalize(const char *text)
{
    switch (owner_type) {
    case PROG_TYPE_MOBILE:
        if (GET_MOB_PROG((Creature *)owner))
            free(((Creature *)owner)->mob_specials.shared->prog);
        ((Creature *)owner)->mob_specials.shared->prog = strdup(text);
        break;
    case PROG_TYPE_ROOM:
        if (GET_ROOM_PROG((room_data *)owner))
            free(((room_data *)owner)->prog);
        ((room_data *)owner)->prog = strdup(text);
        break;
    default:
        errlog("Can't happen");
    }

    prog_compile(desc->creature, owner, owner_type);

    if (IS_PLAYING(desc))
        act("$n nods with satisfaction as $e saves $s work.", false, desc->creature, 0, 0, TO_NOTVICT);
}
