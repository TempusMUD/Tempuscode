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

struct killer_rec {
	killer_rec *next;
	int idnum;
	int gen;
	int demons;
};
#define DEMONIC_BASE 1530

killer_rec *killer_list = NULL;
/*   external vars  */
bool
perform_get_from_room(struct Creature * ch,
    struct obj_data * obj, bool display, int counter);

extern struct descriptor_data *descriptor_list;

// If devils can form hunting parties, demons are gonna just go after them

// Pick a random killer to go after
Creature *
pick_random_killer(void)
{
	descriptor_data *cur_desc;
	Creature *result = NULL;
	int total_count = 0;

	for (cur_desc = descriptor_list;cur_desc;cur_desc = cur_desc->next) {
		if (!cur_desc->creature || !IS_PLAYING(cur_desc))
			continue;
		if (IS_IMMORT(cur_desc->creature))
			continue;
		if (!PLR_FLAGGED(cur_desc->creature, PLR_KILLER))
			continue;
		total_count++;
		if (number(1,total_count) == 1)
			result = cur_desc->creature;
	}

	return result;
}

void
forget_non_killers(void)
{
	killer_rec *prev_rec, *cur_rec, *next_rec;
	descriptor_data *cur_desc;
	if (!killer_list)
		return;

	prev_rec = NULL;
	for (cur_rec = killer_list;cur_rec;cur_rec = next_rec) {
		next_rec = cur_rec->next;
		for (cur_desc = descriptor_list;cur_desc;cur_desc = cur_desc->next) {
			if (!cur_desc->creature || !IS_PLAYING(cur_desc))
				continue;
			if (GET_IDNUM(cur_desc->creature) != cur_rec->idnum)
				continue;
			// Is player still a killer?
			if (PLR_FLAGGED(cur_desc->creature, PLR_KILLER))
				break;
			// player is no longer considered a killer - remove from list
			// demons will dissipate by themselves
			if (!prev_rec)
				killer_list = cur_rec->next;
			else
				prev_rec->next = cur_rec->next;
			free(cur_rec);
		}
		prev_rec = cur_rec;
	}

}

SPECIAL(demonic_overmind)
{
	Creature *vict, *mob;
	killer_rec *cur_rec;
	obj_data *brain;

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return false;
	
	if (spec_mode == SPECIAL_TICK) {
		// Eliminate chars who are non-killers
		forget_non_killers();
		
		vict = pick_random_killer();
		if (!vict)
			return false;

		for (cur_rec = killer_list;cur_rec;cur_rec = cur_rec->next)
			if (cur_rec->idnum == GET_IDNUM(vict))
				break;
		if (!cur_rec) {
			cur_rec = new killer_rec;
			cur_rec->idnum = GET_IDNUM(vict);
			cur_rec->gen = GET_REMORT_GEN(vict);
			cur_rec->demons = 0;
			cur_rec->next = killer_list;
			killer_list = cur_rec;
		}
		
		if (cur_rec->demons < cur_rec->gen + 2) {
			mob = read_mobile(DEMONIC_BASE + MIN(5, (GET_LEVEL(vict) / 9))
				+ number(0,1));
			if (!mob) {
				slog("SYSERR: Unable to load mob in demonic_overmind");
				return false;
			}
			HUNTING(mob) = vict;
			SET_BIT(MOB_FLAGS(mob), MOB_SPIRIT_TRACKER);
			brain = read_object(DEMONIC_BASE);
			if (!brain) {
				slog("SYSERR: Couldn't get hunter's brain!");
				return false;
			}
			GET_OBJ_VAL(brain, 3) = GET_IDNUM(vict);
			equip_char(mob, brain, WEAR_HEAD, MODE_IMPLANT);
			cur_rec->demons++;

			char_to_room(mob, vict->in_room);
			act("The air suddenly cracks open and $n steps out!", false,
				mob, 0, 0, TO_ROOM);
		}
		return true;
	}

	return false;
}

SPECIAL(demonic_guard)
{
	Creature *self = (Creature *)me;
	obj_data *brain, *obj;
	killer_rec *cur_rec;
	int vict_id;

	if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_DEATH)
		return false;

	brain = GET_IMPLANT(self, WEAR_HEAD);
	if (!brain)
		return false;
	vict_id = GET_OBJ_VAL(brain, 3);

	for (cur_rec = killer_list;cur_rec;cur_rec = cur_rec->next)
		if (cur_rec->idnum == vict_id)
			break;
	if (spec_mode == SPECIAL_DEATH) {
		if (cur_rec)
			cur_rec->demons--;
		brain = unequip_char(self, WEAR_HEAD, MODE_IMPLANT);
		extract_obj(brain);
		return false;
	}


	// Check to see if the victim's corpse is here
	for (obj = self->in_room->contents; obj; obj = obj->next_content) {
		if (IS_CORPSE(obj) && CORPSE_IDNUM(obj) == vict_id) {
			act("You get $p.", true, self, obj, 0, TO_CHAR);
			act("$n gets $p.", true, self, obj, 0, TO_ROOM);
			obj_from_room(obj);
			obj_to_char(obj, self);
			return true;
		}
	}

	if (!HUNTING(self) || !PLR_FLAGGED(HUNTING(self), PLR_KILLER)) {
		act("$n vanishes into the mouth of an interplanar conduit.",
			FALSE, ch, 0, 0, TO_ROOM);
		if (cur_rec)
			cur_rec->demons--;
		self->purge(true);
		return false;
	}

	if (HUNTING(self)->in_room->zone != self->in_room->zone) {
		act("$n vanishes into the mouth of an interplanar conduit.",
			FALSE, ch, 0, 0, TO_ROOM);
		char_from_room(self);
		char_to_room(self, HUNTING(self)->in_room);
		act("The air suddenly cracks open and $n steps out!",
			FALSE, ch, 0, 0, TO_ROOM);
		return true;
	}

	return false;
}
