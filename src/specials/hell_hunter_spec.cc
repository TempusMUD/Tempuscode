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
#include "vehicle.h"
#include "materials.h"
#include "paths.h"
#include "specs.h"
#include "login.h"
#include "matrix.h"
#include "house.h"
#include "fight.h"

#include "xml_utils.h"
#include "hell_hunter_spec.h"

/*   external vars  */
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];

/* extern functions */
void add_follower(struct Creature *ch, struct Creature *leader);
void do_auto_exits(struct Creature *ch, room_num room);
void perform_tell(struct Creature *ch, struct Creature *vict, char *buf);
int get_check_money(struct Creature *ch, struct obj_data **obj, int display);


struct social_type {
	char *cmd;
	int next_line;
};

ACMD(do_echo);
ACMD(do_say);
ACMD(do_gen_comm);
ACMD(do_rescue);
ACMD(do_steal);
ACCMD(do_get);

vector <Devil> devils;
vector <Target> targets;
vector <HuntGroup> hunters;
vector <int>blindSpots;

bool
load_hunter_data()
{
	//cerr << "Loading Hell Hunter Data" << endl;

	devils.erase(devils.begin(), devils.end());
	targets.erase(targets.begin(), targets.end());
	hunters.erase(hunters.begin(), hunters.end());

	xmlDocPtr doc = xmlParseFile("etc/hell_hunter_data.xml");
	if (doc == NULL) {
		errlog("Hell Hunter Brain failed to load hunter data.");
		return false;
	}
	// discard root node
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		xmlFreeDoc(doc);
		return false;
	}
	freq = xmlGetIntProp(cur, "Frequency");
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((xmlMatches(cur->name, "TARGET"))) {
			targets.push_back(Target(cur));
		}
		if ((xmlMatches(cur->name, "DEVIL"))) {
			devils.push_back(Devil(cur));
		}
		if ((xmlMatches(cur->name, "HUNTGROUP"))) {
			hunters.push_back(HuntGroup(cur, devils));
		}
		if ((xmlMatches(cur->name, "BLINDSPOT"))) {
			blindSpots.push_back(xmlGetIntProp(cur, "ID"));
		}
		cur = cur->next;
	}
	sort(targets.begin(), targets.end());
	sort(hunters.begin(), hunters.end());
	sort(blindSpots.begin(), blindSpots.end());
	//cerr << "Targets: " << endl << targets << endl;
	//cerr << "Hunters: " << hunters<<endl;
	//cerr << "Devils:" <<endl<< devils<<endl;
	//cerr << "Hell Hunter Data Load Complete:"<<endl;
	xmlFreeDoc(doc);
	return true;
}

bool
isBlindSpot(zone_data * zone)
{
	return find(blindSpots.begin(),
		blindSpots.end(), zone->number) != blindSpots.end();
}

SPECIAL(hell_hunter_brain)
{
	static bool data_loaded = false;
	static int counter = 1;
	struct obj_data *obj = NULL, *weap = NULL;
	struct Creature *mob = NULL, *vict = NULL;
	unsigned int i, j;
	int num_devils = 0, regulator = 0;
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (!data_loaded) {
		if (!load_hunter_data())
			return 0;
		data_loaded = true;
		slog("Hell Hunter Brain data successfully loaded.");
	}
	if (cmd) {
		if (CMD_IS("reload")) {
			if (!load_hunter_data()) {
				data_loaded = false;
				send_to_char(ch,
					"Hell Hunter Brain failed to load hunter data.\r\n");
			} else {
				data_loaded = true;
				send_to_char(ch,
					"Hell Hunter Brain data successfully reloaded.\r\n");
			}
			return 1;
		} else if (CMD_IS("status")) {
			send_to_char(ch, "Counter is at %d, freq %d.\r\n", counter, freq);
			sprintf(buf, "     [vnum] %30s exist/housed\r\n", "Object Name");
			for (i = 0; i < targets.size(); i++) {
				if (!(obj = real_object_proto(targets[i].o_vnum)))
					continue;
				send_to_char(ch, "%3d. [%5d] %30s %3d/%3d\r\n",
					i, targets[i].o_vnum, obj->name,
					obj->shared->number, obj->shared->house_count);
			}
			//TODO: Add blind spots to status
			return 1;
		} else if (CMD_IS("activate")) {
			skip_spaces(&argument);

			if (*argument && isdigit(*argument)) {
				freq = atoi(argument);
				send_to_char(ch, "Frequency set to %d.\n", freq);
				counter = freq;
				return 1;
			} else if (*argument && strcmp(argument, "now") == 1) {
				sprintf(buf, "Counter set to 1.\r\n");
				counter = 1;
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}

	if (--counter > 0) {
		return 0;
	}

	counter = freq;

	for (obj = object_list; obj; vict = NULL, obj = obj->next) {

		if (!obj->in_room && !(vict = obj->carried_by)
			&& !(vict = obj->worn_by))
			continue;
		if (!IS_OBJ_STAT3(obj, ITEM3_HUNTED)) {
			continue;
		}

		if (vict && (IS_NPC(vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
				// some rooms are safe
				ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) ||
				// no players in quest
				GET_QUEST(vict) ||
				// can't go to isolated zones
				ZONE_FLAGGED(vict->in_room->zone, ZONE_ISOLATED) ||
				// ignore shopkeepers
				(IS_NPC(vict) && vendor == GET_MOB_SPEC(vict)) ||
				// don't go to heaven
				isBlindSpot(vict->in_room->zone))) {
			continue;
		}
		if (vict && IS_SOULLESS(vict)) {
			send_to_char(ch,
				"You feel the eyes of hell look down upon you.\r\n");
			continue;
		}

		if (obj->in_room &&
			(ROOM_FLAGGED(obj->in_room, SAFE_ROOM_BITS) ||
				ZONE_FLAGGED(obj->in_room->zone, ZONE_ISOLATED))) {
			continue;
		}

		for (i = 0; (unsigned)i < targets.size(); i++) {
			if (targets[i].o_vnum == GET_OBJ_VNUM(obj)
				&& IS_OBJ_STAT3(obj, ITEM3_HUNTED)) {
				break;
			}
		}
		if ((unsigned)i == targets.size())
			continue;

		// try to skip the first person sometimes
		if (vict && !number(0, 2)) {
			continue;
		}

		for (j = 0; j < 4; j++) {
			if (number(0, 100) <= hunters[targets[i].level][j].prob) {	// probability
				if (!(mob = read_mobile(hunters[targets[i].level][j].m_vnum))) {
					errlog("Unable to load mob in hell_hunter_brain()");
					return 0;
				}
				if (hunters[targets[i].level][j].weapon >= 0 &&
					(weap =
						read_object(hunters[targets[i].level][j].weapon))) {
					if (equip_char(mob, weap, WEAR_WIELD, MODE_EQ)) {	// mob equipped
						errlog("(non-critical) Hell Hunter killed by eq.");
						return 1;	// return if equip killed mob
					}
				}

				num_devils++;

				if (vict) {
					HUNTING(mob) = vict;
					SET_BIT(MOB_FLAGS(mob), MOB_SPIRIT_TRACKER);

					if (!IS_NPC(vict) && GET_REMORT_GEN(vict)) {
						// hps GENx
						/*
						 * GET_MAX_HIT(mob) = GET_HIT(mob) =
						 MIN(10000, (GET_MAX_HIT(mob) + GET_MAX_HIT(vict)));
						 // damroll GENx/3
						 GET_DAMROLL(mob) =
						 MIN(50, (GET_DAMROLL(mob) + GET_REMORT_GEN(vict)));
						 // hitroll GENx/3
						 GET_HITROLL(mob) =
						 MIN(50, (GET_HITROLL(mob) + GET_REMORT_GEN(vict)));
						 */
					}
				}

				char_to_room(mob, vict ? vict->in_room : obj->in_room, false);
				act("$n steps suddenly out of an infernal conduit from the outer planes!", FALSE, mob, 0, 0, TO_ROOM);

			}
		}

		if (vict && !IS_NPC(vict) && GET_REMORT_GEN(vict)
			&& number(0, GET_REMORT_GEN(vict)) > 1) {

			if (!(mob = read_mobile(H_REGULATOR)))
				errlog("Unable to load hell hunter regulator in hell_hunter_brain.");
			else {
				regulator = 1;
				HUNTING(mob) = vict;
				char_to_room(mob, vict->in_room, false);
				act("$n materializes suddenly from a stream of hellish energy!", FALSE, mob, 0, 0, TO_ROOM);
			}
		}

		mudlog(vict ? GET_INVIS_LVL(vict) : 0, CMP, true,
			"HELL: %d Devils%s sent after obj %s (%s@%d)",
			num_devils,
			regulator ? " (+reg)" : "",
			obj->name,
			vict ? GET_NAME(vict) : "Nobody",
			obj->in_room ? obj->in_room->number :
			(vict && vict->in_room) ? vict->in_room->number : -1);
		sprintf(buf, "%d Devils%s sent after obj %s (%s@%d)",
			num_devils,
			regulator ? " (+reg)" : "",
			obj->name, vict ? "$N" : "Nobody",
			obj->in_room ? obj->in_room->number :
			(vict && vict->in_room) ? vict->in_room->number : -1);
		act(buf, FALSE, ch, 0, vict, TO_ROOM);
		act(buf, FALSE, ch, 0, vict, TO_CHAR);
		return 1;
	}

	if (cmd) {
		send_to_char(ch, "Falling through, no match.\r\n");
	} else {

		// we fell through, lets check again sooner than freq
		counter = freq >> 3;
	}

	return 0;
}

SPECIAL(hell_hunter)
{

	if (spec_mode != SPECIAL_TICK)
		return 0;
	struct obj_data *obj = NULL, *t_obj = NULL;
	unsigned int i;
	struct Creature *vict = NULL, *devil = NULL;

	if (cmd)
		return 0;

	for (obj = ch->in_room->contents; obj; obj = obj->next_content) {

		if (IS_CORPSE(obj)) {
			for (t_obj = obj->contains; t_obj; t_obj = t_obj->next_content) {
				for (i = 0; i < targets.size(); i++) {
					if (targets[i].o_vnum == GET_OBJ_VNUM(t_obj)) {
						act("$n takes $p from $P.", FALSE, ch, t_obj, obj,
							TO_ROOM);
						mudlog(0, CMP, true,
							"HELL: %s looted %s at %d.", GET_NAME(ch),
							t_obj->name, ch->in_room->number);
						extract_obj(t_obj);
						return 1;
					}
				}
			}
			continue;
		}
		for (i = 0; i < targets.size(); i++) {
			if (targets[i].o_vnum == GET_OBJ_VNUM(obj)) {
				act("$n takes $p.", FALSE, ch, obj, t_obj, TO_ROOM);
				mudlog(0, CMP, true,
					"HELL: %s retrieved %s at %d.", GET_NAME(ch),
					obj->name, ch->in_room->number);
				extract_obj(obj);
				return 1;
			}
		}
	}

	if (!ch->numCombatants() && !HUNTING(ch) && !AFF_FLAGGED(ch, AFF_CHARM)) {
		act("$n vanishes into the mouth of an interplanar conduit.",
			FALSE, ch, 0, 0, TO_ROOM);
		ch->purge(true);
		return 1;
	}

	if (GET_MOB_VNUM(ch) == H_REGULATOR) {

		if (GET_MANA(ch) < 100) {
			act("$n vanishes into the mouth of an interplanar conduit.",
				FALSE, ch, 0, 0, TO_ROOM);
			ch->purge(true);
			return 1;
		}

		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			vict = *it;
			if (vict == ch)
				continue;

			// REGULATOR doesn't want anyone attacking him
			if (!IS_DEVIL(vict) && vict->findCombat(ch)) {

				if (!(devil = read_mobile(H_SPINED))) {
					errlog("HH REGULATOR failed to load H_SPINED for defense.");
					// set mana to zero so he will go away on the next loop
					GET_MANA(ch) = 0;
					return 1;
				}

				char_to_room(devil, ch->in_room, false);
				act("$n gestures... A glowing conduit flashes into existence!",
					FALSE, ch, 0, vict, TO_ROOM);
				act("...$n leaps out and attacks $N!", FALSE, devil, 0, vict,
					TO_NOTVICT);
				act("...$n leaps out and attacks you!", FALSE, devil, 0, vict,
					TO_VICT);

				ch->removeAllCombat();
				hit(devil, vict, TYPE_UNDEFINED);
				WAIT_STATE(vict, 1 RL_SEC);

				return 1;
			}


			if (IS_DEVIL(vict) && IS_NPC(vict) &&
				GET_HIT(vict) < (GET_MAX_HIT(vict) - 500)) {

				act("$n opens a conduit of streaming energy to $N!\r\n"
					"...$N's wounds appear to regenerate!", FALSE, ch, 0, vict,
					TO_ROOM);

				GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + 500);
				GET_MANA(ch) -= 100;
				return 1;
			}

		}

	}

	return 0;
}

#define BLADE_VNUM 16203
#define ARIOCH_LAIR 16284
#define PENTAGRAM_ROOM 15437
#define ARIOCH_LEAVE_MSG "A glowing portal spins into existence behind $n, who is then drawn backwards into the mouth of the conduit, and out of this plane.  The portal then spins down to a singular point and disappears."
#define ARIOCH_ARRIVE_MSG "A glowing portal spins into existence before you, and you see a dark figure approaching from the depths.  $n steps suddenly from the mouth of the conduit, which snaps shut behind $m."

SPECIAL(arioch)
{
	struct obj_data *blade = NULL, *obj = NULL;
	struct room_data *rm = NULL;
	struct Creature *vict = NULL;
	unsigned int i;

	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (ch->in_room->zone->number != 162) {

		if (!HUNTING(ch) && !ch->numCombatants()) {

			for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
				if (IS_CORPSE(obj)) {
					for (blade = obj->contains; blade;
						blade = blade->next_content) {
						for (i = 0; i < targets.size(); i++) {
							if (BLADE_VNUM == GET_OBJ_VNUM(blade)) {
								act("$n takes $p from $P.", FALSE, ch, blade,
									obj, TO_ROOM);
								mudlog(0, CMP, true,
									"HELL: %s looted %s at %d.",
									GET_NAME(ch), blade->name,
									ch->in_room->number);
								if (!GET_EQ(ch, WEAR_WIELD)) {
									obj_from_obj(blade);
									obj_to_char(blade, ch);
								} else
									extract_obj(blade);
								return 1;
							}
						}
					}
				}
				if (BLADE_VNUM == GET_OBJ_VNUM(obj)) {
					act("$n takes $p.", FALSE, ch, obj, obj, TO_ROOM);
					mudlog(0, CMP, true,
						"HELL: %s retrieved %s at %d.", GET_NAME(ch),
						obj->name, ch->in_room->number);
					if (!GET_EQ(ch, WEAR_WIELD)) {
						obj_from_room(obj);
						obj_to_char(obj, ch);
					} else
						extract_obj(obj);
					return 1;
				}
			}
			act(ARIOCH_LEAVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			char_from_room(ch, false);
			char_to_room(ch, real_room(ARIOCH_LAIR), false);
			act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			return 1;
		}
		if (GET_HIT(ch) < 800) {
			act(ARIOCH_LEAVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			char_from_room(ch, false);
			char_to_room(ch, real_room(ARIOCH_LAIR), false);
			act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			return 1;
		}
		return 0;
	}

	if (GET_EQ(ch, WEAR_WIELD))
		return 0;

	for (blade = object_list; blade; blade = blade->next) {
		if (GET_OBJ_VNUM(blade) == BLADE_VNUM &&
			(((rm = blade->in_room) && !ROOM_FLAGGED(rm, SAFE_ROOM_BITS)) ||
				(((vict = blade->carried_by) || (vict = blade->worn_by)) &&
					!ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) &&
					(!IS_NPC(vict) || vendor != GET_MOB_SPEC(vict)) &&
					!PRF_FLAGGED(vict, PRF_NOHASSLE)
					&& can_see_creature(ch, vict)))) {
			if (vict) {
				HUNTING(ch) = vict;
				rm = vict->in_room;
			}
			act(ARIOCH_LEAVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			char_from_room(ch, false);
			char_to_room(ch, rm, false);
			act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
			mudlog(0, CMP, true,
				"HELL: Arioch ported to %s@%d",
				vict ? GET_NAME(vict) : "Nobody", rm->number);
			return 1;
		}
	}
	return 0;
}

#undef H_SPINED
#undef H_REGULATOR
