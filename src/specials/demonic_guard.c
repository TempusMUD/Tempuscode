// File: hell_hunter.spec                     -- Part of TempusMUD
//
// DataFile: lib/etc/hell_hunter_data
//
// Copyright 1998 by John Watson, all rights reserved.
// Hacked to use classes and XML John Rothe 2001
//
#ifdef HAS_CONFIG_H
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "players.h"
#include "tmpstr.h"

struct criminal_rec {
    struct criminal_rec *next;
    int idnum;
    int grace;
};
#define DEMONIC_BASE 90
#define ARCHONIC_BASE 100

struct criminal_rec *criminal_list = NULL;
/*   external vars  */
bool
perform_get_from_room(struct creature *ch,
                      struct obj_data *obj, bool display, int counter);

extern struct descriptor_data *descriptor_list;

bool
summon_criminal_demons(struct creature *vict)
{
    struct creature *mob;
    int vnum_base = (IS_EVIL(vict)) ? ARCHONIC_BASE : DEMONIC_BASE;
    int demon_num = GET_REMORT_GEN(vict) / 2 + 1;
    int idx;

    for (idx = 0; idx < demon_num; idx++) {
        mob = read_mobile(vnum_base + MIN(4, (GET_LEVEL(vict) / 9))
                          + number(0, 1));
        if (!mob) {
            errlog("Unable to load mob in demonic_overmind");
            return false;
        }
        start_hunting(mob, vict);
        SET_BIT(NPC_FLAGS(mob), NPC_SPIRIT_TRACKER);
        CREATE(mob->mob_specials.func_data, int, 1);
        *((int *)mob->mob_specials.func_data) = GET_IDNUM(vict);

        char_to_room(mob, vict->in_room, true);
        act("The air suddenly cracks open and $n steps out!", false,
            mob, NULL, NULL, TO_ROOM);
    }

    if (IS_EVIL(vict)) {
        mudlog(GET_INVIS_LVL(vict), NRM, true,
               "%d archons dispatched to hunt down %s",
               demon_num, GET_NAME(vict));
    } else {
        mudlog(GET_INVIS_LVL(vict), NRM, true,
               "%d demons dispatched to hunt down %s", demon_num, GET_NAME(vict));
    }

    return true;
}

// Devils can form hunting parties - demons are gonna just go after them
SPECIAL(demonic_overmind)
{
    struct creature *vict;
    struct criminal_rec *cur_rec;
    struct descriptor_data *cur_desc;
    bool summoned = false;

    if (spec_mode == SPECIAL_TICK) {
        for (cur_desc = descriptor_list; cur_desc; cur_desc = cur_desc->next) {
            if (!cur_desc->creature || !IS_PLAYING(cur_desc)) {
                continue;
            }
            if (IS_IMMORT(cur_desc->creature)) {
                continue;
            }
            if (reputation_of(cur_desc->creature) < 700) {
                continue;
            }

            vict = cur_desc->creature;

            for (cur_rec = criminal_list; cur_rec; cur_rec = cur_rec->next) {
                if (cur_rec->idnum == GET_IDNUM(vict)) {
                    break;
                }
            }

            // grace periods:
            // reputation 700 : 3 - 4 hours : 2025 - 4500 updates
            // reputation 1000: 30 - 60 minutes : 427 - 947 updates

            // Reputation 700 --
            // Min: 162000 / (700 - 620) = 2025 = 2.25 hours
            // Max: 360000 / (700 - 620) = 4500 = 5 hours

            // Reputation 1000 --
            // Min: 162000 / (1000 - 620) = 427 = 28 minutes
            // Max: 360000 / (1000 - 620) = 947 = 63 minutes

            // Calculation of the grace is pretty damn hard.  here's how
            // I did it.
            //
            // I solved two simultaneous equations of the form
            // a - tb = 700t where t=min time for 700 rep
            // a - tb = 1000t where t=min time for 1000 rep
            // and
            // a - tb = 700t where t=max time for 700 rep
            // a - tb = 1000t where t=max time for 700 rep

            // this gave me the grace to start with and the amount to
            // subtract from the decrementing.  However, the latter value
            // rarely comes out equal, so I took the midpoint of that.
            // It's an approximate method with a large error, but it works
            // well enough.

            if (!cur_rec) {
                CREATE(cur_rec, struct criminal_rec, 1);
                cur_rec->idnum = GET_IDNUM(vict);
                cur_rec->grace = number(162000, 360000);
                cur_rec->next = criminal_list;
                criminal_list = cur_rec;
            }

            if (cur_rec->grace > 0) {
                cur_rec->grace -= reputation_of(vict) - 620;
                continue;
            }
            // If they're in an arena or a quest, their grace still
            // decrements.  They just get attacked as soon as they leave
            if (GET_QUEST(cur_desc->creature)) {
                continue;
            }
            if (ROOM_FLAGGED(cur_desc->creature->in_room, ROOM_ARENA)) {
                continue;
            }

            // Their grace has run out.  Get em.
            cur_rec->grace = number(78750, 101250);
            summoned = summoned || summon_criminal_demons(vict);
        }

        return summoned;
    }

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    if (CMD_IS("status")) {
        send_to_char(ch, "Demonic overmind status:\r\n");
        send_to_char(ch, " Player                    Grace\r\n"
                         "-------------------------  -----\r\n");
        for (cur_rec = criminal_list; cur_rec; cur_rec = cur_rec->next) {
            send_to_char(ch, "%-25s  %4d\r\n",
                         player_name_by_idnum(cur_rec->idnum), cur_rec->grace);
        }

        return true;
    }

    if (CMD_IS("spank")) {
        char *name;

        name = tmp_getword(&argument);
        vict = get_char_vis(ch, name);
        if (!vict) {
            send_to_char(ch, "I can't find that person.\r\n");
            return true;
        }
        send_to_char(ch, "Sending demons after %s.\r\n", GET_NAME(vict));
        summon_criminal_demons(vict);
        return true;
    }

    return false;
}

SPECIAL(demonic_guard)
{
    struct creature *self = (struct creature *)me;
    int vict_id;

    if (spec_mode != SPECIAL_TICK) {
        return false;
    }

    if (!self->mob_specials.func_data) {
        return false;
    }
    vict_id = *((int *)self->mob_specials.func_data);

    ch = get_char_in_world_by_idnum(vict_id);
    if (!ch || !NPC_HUNTING(self) || reputation_of(ch) < 700) {
        act("$n vanishes into the mouth of an interplanar conduit.",
            false, self, NULL, NULL, TO_ROOM);
        creature_purge(self, true);
        return true;
    }

    if (NPC_HUNTING(self)->in_room->zone != self->in_room->zone) {
        act("$n vanishes into the mouth of an interplanar conduit.",
            false, self, NULL, NULL, TO_ROOM);
        char_from_room(self, true);
        char_to_room(self, NPC_HUNTING(self)->in_room, true);
        act("The air suddenly cracks open and $n steps out!",
            false, self, NULL, NULL, TO_ROOM);
        return true;
    }

    return false;
}
