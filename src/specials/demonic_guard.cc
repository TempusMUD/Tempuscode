// File: hell_hunter.spec                     -- Part of TempusMUD
//
// DataFile: lib/etc/hell_hunter_data
//
// Copyright 1998 by John Watson, all rights reserved.
// Hacked to use classes and XML John Rothe 2001
//
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;
#include <libxml/parser.h>
#include <libxml/tree.h>
// Undefine CHAR to avoid collisions
#undef CHAR
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
#include "desc_data.h"
#include "materials.h"
#include "paths.h"
#include "specs.h"
#include "login.h"
#include "house.h"
#include "fight.h"
#include "player_table.h"
#include "security.h"

struct criminal_rec {
	criminal_rec *next;
	int idnum;
	int grace;
};
#define DEMONIC_BASE 90
#define ARCHONIC_BASE 100
#define KILLER_GRACE	5

criminal_rec *criminal_list = NULL;
/*   external vars  */
bool
perform_get_from_room(struct Creature * ch,
    struct obj_data * obj, bool display, int counter);

extern struct descriptor_data *descriptor_list;

bool
summon_criminal_demons(Creature *vict)
{
	Creature *mob;
	int vnum_base = (IS_EVIL(vict)) ? ARCHONIC_BASE:DEMONIC_BASE;;
	int demon_num = GET_REMORT_GEN(vict) / 2 + 1;
	int idx;

	for (idx = 0;idx < demon_num;idx++) {
		mob = read_mobile(vnum_base + MIN(4, (GET_LEVEL(vict) / 9))
			+ number(0,1));
		if (!mob) {
			slog("SYSERR: Unable to load mob in demonic_overmind");
			return false;
		}
		HUNTING(mob) = vict;
		SET_BIT(MOB_FLAGS(mob), MOB_SPIRIT_TRACKER);
		CREATE(mob->mob_specials.func_data, int, 1);
		*((int *)mob->mob_specials.func_data) = GET_IDNUM(vict);

		char_to_room(mob, vict->in_room);
		act("The air suddenly cracks open and $n steps out!", false,
			mob, 0, 0, TO_ROOM);
	}

	if (IS_EVIL(vict))
		mudlog(GET_INVIS_LVL(vict), NRM, true,
			"%d archons dispatched to hunt down %s",
			demon_num, GET_NAME(vict));
	else
		mudlog(GET_INVIS_LVL(vict), NRM, true,
			"%d demons dispatched to hunt down %s",
			demon_num, GET_NAME(vict));

	return true;
}

// Devils can form hunting parties - demons are gonna just go after them
SPECIAL(demonic_overmind)
{
	Creature *vict;
	criminal_rec *cur_rec;
	descriptor_data *cur_desc;
	bool summoned = false;

	if (spec_mode == SPECIAL_TICK) {
		for (cur_desc = descriptor_list;cur_desc;cur_desc = cur_desc->next) {
			if (!cur_desc->creature || !IS_PLAYING(cur_desc))
				continue;
			if (IS_IMMORT(cur_desc->creature))
				continue;
			if (GET_REPUTATION(cur_desc->creature) < 700)
				continue;
			vict = cur_desc->creature;

			for (cur_rec = criminal_list;cur_rec;cur_rec = cur_rec->next)
				if (cur_rec->idnum == GET_IDNUM(vict))
					break;

			// grace periods:
			// reputation 700 : 1.75 - 2.25 hours : 1575 - 2025 updates
			// reputation 1000: 15 - 20 minutes : 225 - 300 updates

			// Reputation 700 -- 
			//    Min: 78750 / (700 - 650) = 1575 = 1.75 hours
			//    Av:  90000 / (700 - 650)  = 1800 = 2.0 hours
			//    Max: 101250 / (700 - 650) = 2025 = 2.25 hours

			// Reputation 1000 -- 
			//    Min: 78750 / (1000 - 650) = 225 = 15 minutes
			//    Av:  90000 / (1000 - 650)  = 257 = 17 minutes
			//    Max: 101250 / (1000 - 650) = 289 = 19 minutes

			if (!cur_rec) {
				cur_rec = new criminal_rec;
				cur_rec->idnum = GET_IDNUM(vict);
				cur_rec->grace = number(78750, 101250);
				cur_rec->next = criminal_list;
				criminal_list = cur_rec;
			}

			if (cur_rec->grace > 0) {
				cur_rec->grace -= GET_REPUTATION(vict) - 650;
				continue;
			}
			
			// Their grace has run out.  Get em.
			cur_rec->grace = number(78750, 101250);
			summoned = summoned || summon_criminal_demons(vict);
		}

		return summoned;
	}

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (CMD_IS("status")) {
		send_to_char(ch, "Demonic overmind status:\r\n");
		send_to_char(ch, " Player                    Grace\r\n"
			"-------------------------  -----\r\n");
		for (cur_rec = criminal_list;cur_rec;cur_rec = cur_rec->next)
			send_to_char(ch, "%-25s  %4d\r\n",
				playerIndex.getName(cur_rec->idnum), cur_rec->grace);

		return true;
	}

	if (CMD_IS("spank")) {
		char *name;

		if (!Security::isMember(ch, "AdminFull"))
			return false;

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
	Creature *self = (Creature *)me;
	int vict_id;

	if (spec_mode != SPECIAL_TICK)
		return false;

	if (!self->mob_specials.func_data)
		return false;
	vict_id = *((int *)self->mob_specials.func_data);

	ch = get_char_in_world_by_idnum(vict_id);
	if (!ch || !HUNTING(self) || GET_REPUTATION(ch) < 700) {
		act("$n vanishes into the mouth of an interplanar conduit.",
			FALSE, self, 0, 0, TO_ROOM);
		self->purge(true);
		return false;
	}

	if (HUNTING(self)->in_room->zone != self->in_room->zone) {
		act("$n vanishes into the mouth of an interplanar conduit.",
			FALSE, self, 0, 0, TO_ROOM);
		char_from_room(self);
		char_to_room(self, HUNTING(self)->in_room);
		act("The air suddenly cracks open and $n steps out!",
			FALSE, self, 0, 0, TO_ROOM);
		return true;
	}

	return false;
}
