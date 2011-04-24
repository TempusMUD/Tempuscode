//
// File: remorter.cc                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson and John Rothe, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
// Tempus Includes
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "char_class.h"
#include "screen.h"
#include "clan.h"
#include "vehicle.h"
#include "materials.h"
#include "specs.h"
#include "login.h"
#include "house.h"
#include "fight.h"

#include "xml_utils.h"

int
do_pass_remort_test(struct creature *ch)
{
    int i;

    // Wipe thier skills
    for (i = 1; i <= MAX_SKILLS; i++)
        SET_SKILL(ch, i, 0);

    do_start(ch, false);

    REMOVE_BIT(PRF_FLAGS(ch),
        PRF_NOPROJECT | PRF_ROOMFLAGS | PRF_HOLYLIGHT | PRF_NOHASSLE |
        PRF_LOG1 | PRF_LOG2 | PRF_NOWIZ);

    REMOVE_BIT(PLR_FLAGS(ch), PLR_HALT | PLR_INVSTART | PLR_MORTALIZED |
        PLR_OLCGOD);

    GET_INVIS_LVL(ch) = 0;
    GET_COND(ch, DRUNK) = 0;
    GET_COND(ch, FULL) = 0;
    GET_COND(ch, THIRST) = 0;

    // Give em another gen
    if (GET_REMORT_GEN(ch) == 10)
        account_set_quest_points(ch->account, ch->account->quest_points + 1);
    else
        GET_REMORT_GEN(ch)++;

    // At gen 1 they enter the world of pk, like it or not
    if (GET_REMORT_GEN(ch) >= 1 && GET_REPUTATION(ch) <= 0)
        gain_reputation(ch, 5);
    // Whack thier remort invis
    GET_WIMP_LEV(ch) = 0;       // wimpy
    GET_TOT_DAM(ch) = 0;        // cyborg damage

    // Tell everyone that they remorted
    char *msg = tmp_sprintf("%s completed gen %d remort test",
        GET_NAME(ch), GET_REMORT_GEN(ch));
    mudlog(LVL_IMMORT, BRF, false, "%s", msg);

    REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);

    // Save the char and its implants but not its eq
    creature_remort(ch);

    return 1;
}

SPECIAL(remorter)
{
    int value, level;

    if (spec_mode != SPECIAL_CMD)
        return false;
    if (!cmd)
        return 0;

    if (CMD_IS("help") && GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char(ch, "Valid Commands:\r\n");
        send_to_char(ch, "Status - Shows current test state.\r\n");
        send_to_char(ch, "Reload - Resets test to waiting state.\r\n");
        return 1;
    }

    if (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
        return 0;

    if (GET_EXP(ch) < exp_scale[LVL_AMBASSADOR] ||
        GET_LEVEL(ch) < (LVL_AMBASSADOR - 1)) {
        send_to_char(ch, "Piss off.  Come back when you are bigger.\r\n");
        struct room_data *room = player_loadroom(ch);
        if (room == NULL)
            room = real_room(3061); // modrian dump
        act("$n disappears in a mushroom cloud.", false, ch, 0, 0, TO_ROOM);
        char_from_room(ch, false);
        char_to_room(ch, room, false);
        act("$n arrives from a puff of smoke.", false, ch, 0, 0, TO_ROOM);
        act("$n has transferred you!", false, (struct creature *)me, 0, ch,
            TO_VICT);
        look_at_room(ch, room, 0);
        return 1;
    }

    skip_spaces(&argument);
    char *arg1 = tmp_getword(&argument);

    if (!*arg1) {
        send_to_char(ch,
            "You must say 'remort' to begin or 'goodbye' to leave.\r\n");
        return 1;
    }
    if (isname_exact(arg1, "goodbye")) {
        struct room_data *room = player_loadroom(ch);
        if (room == NULL)
            room = real_room(3061); // modrian dump

        if (room == NULL) {
            send_to_char(ch, "There is nowhere to send you.\r\n");
            return 1;
        } else {
            send_to_char(ch, "Very well, coward.\r\n");
            act("$n disappears in a mushroom cloud.", false, ch, 0, 0,
                TO_ROOM);
            char_from_room(ch, false);
            char_to_room(ch, room, false);
            act("$n arrives from a puff of smoke.", false, ch, 0, 0, TO_ROOM);
            act("$n has transferred you!", false, (struct creature *)me, 0, ch,
                TO_VICT);
            look_at_room(ch, room, 0);
            return 1;
        }
    } else if (!isname_exact(arg1, "remort")) {
        send_to_char(ch,
            "You must say 'remort' to begin or 'goodbye' to leave.\r\n");
        return 1;
    }

    value = GET_GOLD(ch);

    level = MIN(10, 3 + GET_REMORT_GEN(ch));

    if (value < level * 5000000) {
        send_to_char(ch,
            "You do not have sufficient sacrifice to do this.\r\n");
        send_to_char(ch,
            "The required sacrifice must be worth %d coins.\r\n"
            "You have only brought a %d coin value.\r\n", level * 5000000,
            value);
        return 1;
    }

    value = MIN(level * 5000000, GET_GOLD(ch));
    GET_GOLD(ch) -= value;

    send_to_char(ch, "Your sacrifice has been accepted.\r\n");
    return do_pass_remort_test(ch);
}
