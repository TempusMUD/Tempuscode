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
			errlog("Unable to load mob in demonic_overmind");
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
			// reputation 700 : 3 - 4 hours : 2025 - 4500 updates
			// reputation 1000: 30 - 60 minutes : 427 - 947 updates

			// Reputation 700 -- 
			//    Min: 162000 / (700 - 620) = 2025 = 2.25 hours
			//    Max: 360000 / (700 - 620) = 4500 = 5 hours

			// Reputation 1000 -- 
			//    Min: 162000 / (1000 - 620) = 427 = 28 minutes
			//    Max: 360000 / (1000 - 620) = 947 = 63 minutes

			// Calculation of the grace is pretty damn hard.  here's how
			// I did it.
			//
			// I solved two simultaneous equations of the form
			//   a - tb = 700t where t=min time for 700 rep
			//   a - tb = 1000t where t=min time for 1000 rep
			// and
			//   a - tb = 700t where t=max time for 700 rep
			//   a - tb = 1000t where t=max time for 700 rep

			// this gave me the grace to start with and the amount to
			// subtract from the decrementing.  However, the latter value
			// rarely comes out equal, so I took the midpoint of that.
			// It's an approximate method with a large error, but it works
			// well enough.

			if (!cur_rec) {
				cur_rec = new criminal_rec;
				cur_rec->idnum = GET_IDNUM(vict);
				cur_rec->grace = number(162000, 360000);
				cur_rec->next = criminal_list;
				criminal_list = cur_rec;
			}

			if (cur_rec->grace > 0) {
				cur_rec->grace -= GET_REPUTATION(vict) - 620;
				continue;
			}
			
			// If they're in an arena or a quest, their grace still
			// decrements.  They just get attacked as soon as they leave
			if (GET_QUEST(cur_desc->creature))
				continue;
			if (ROOM_FLAGGED(cur_desc->creature->in_room, ROOM_ARENA))
				continue;

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
