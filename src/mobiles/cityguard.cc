#include <vector>

#include "house.h"
#include "actions.h"
#include "db.h"
#include "comm.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "utils.h"

struct cityguard_data {
	int targ_room;
};

SPECIAL(cityguard);
SPECIAL(guard);

// Cityguards have the following functions:
// - stop all fights not involving a cityguard
// - assist in all fights involving a cityguard
// - they respond to calls for help by going to the room the call originated
// - when fighting, they drag the person away to the jail
// - when in the jail, they throw the person in jail
// - when idle, they do various amusing things
// - when the guard dies, two more spawn, hunting the attacker

void
summon_cityguards(room_data *room)
{
	CreatureList::iterator it;
	cityguard_data *data;

	// Now get about half the cityguards in the zone to respond
	for (it = characterList.begin(); it != characterList.end();it++) {
		if (GET_MOB_SPEC((*it)) != cityguard)
			continue;
		if ((*it)->in_room
				&& (*it)->in_room->zone == room->zone
				&& !number(0, 4)) {
			data = (cityguard_data *)((*it)->mob_specials.func_data);
			if (!data->targ_room)
				data->targ_room = room->number;
		}
	}
}

void
call_for_help(Creature *ch, Creature *attacker)
{
	const char *msg;

	// Emit a shout for help
	if (GET_MOB_SPEC(ch) == cityguard || GET_MOB_SPEC(ch) == guard) {
		switch (number(0, 10)) {
		case 0:
			msg = "To arms!  To arms!  We are under attack!"; break;
		case 1:
			msg = "Protect the city!"; break;
		case 2:
			if (IS_GOOD(ch))
				msg = "By Alron's will, we shall defend our fair city!";
			else if (IS_EVIL(ch))
				msg = "By Veloth, this city is ours by right!";
			else
				msg = "You shall not prevail!";
			break;
		case 3:
			msg = "Death to %s!"; break;
		default:
			msg = "Aid me my comrades!"; break;
		}
	} else {
		switch (number(0, 10)) {
		case 0:
			msg = "Help!  Help!  %s is trying to kill me!"; break;
		case 1:
			msg = "I'm being attacked!  Save me!"; break;
		case 2:
			msg = "Help me!  I'm being attacked!"; break;
		default:
			msg = "Will no one help me?!"; break;
		}
	}
	do_gen_comm(ch, tmp_sprintf(msg, GET_DISGUISED_NAME(ch, attacker)),
		0, SCMD_SHOUT, 0);
	summon_cityguards(ch->in_room);
}

int
throw_char_in_jail(struct Creature *ch, struct Creature *vict)
{
	room_num jail_cells[6] = { 10908, 10910, 10911, 10921, 10920, 10919 };
	struct room_data *cell_rnum = NULL;
	struct obj_data *locker = NULL, *torch = NULL, *obj = NULL, *next_obj =
		NULL;
	int i, count = 0, found = false;

	while (count < 12) {
		count++;
		cell_rnum = real_room(jail_cells[number(0, 5)]);
		if (cell_rnum == NULL || cell_rnum->people)
			continue;

		if (cell_rnum->people)
			continue;
	}

	if (cell_rnum == NULL)
		return 0;

	for (locker = (real_room(ch->in_room->number + 1))->contents; locker;
		locker = locker->next_content) {
		if (GET_OBJ_VNUM(locker) == 3178 && !locker->contains) {
			for (i = 0; i < NUM_WEARS; i++) {
				if (GET_EQ(vict, i) && can_see_object(ch, GET_EQ(vict, i))) {
					found = 1;
					if (GET_OBJ_TYPE(GET_EQ(vict, i)) == ITEM_KEY &&
						!GET_OBJ_VAL(GET_EQ(vict, i), 1))
						extract_obj(GET_EQ(vict, i));
					else if (IS_NPC(vict))
						extract_obj(GET_EQ(vict, i));
					else if (!IS_OBJ_STAT2(GET_EQ(vict, i), ITEM2_NOREMOVE) &&
						!IS_OBJ_STAT(GET_EQ(vict, i), ITEM_NODROP))
						obj_to_obj(unequip_char(vict, i, MODE_EQ), locker);
				}
			}
			for (obj = vict->carrying; obj; obj = next_obj) {
				next_obj = obj->next_content;
				if (!IS_OBJ_STAT(obj, ITEM_NODROP) && can_see_object(ch, obj)) {
					found = 1;
					if (GET_OBJ_TYPE(obj) == ITEM_KEY && !GET_OBJ_VAL(obj, 1))
						extract_obj(obj);
					else if (IS_NPC(vict))
						extract_obj(obj);
					else if (!IS_OBJ_STAT(obj, ITEM_NODROP)) {
						obj_from_char(obj);
						obj_to_obj(obj, locker);
					}
				}
			}
			if (found) {
				GET_OBJ_VAL(locker, 0) = GET_IDNUM(vict);
				act("$n removes all your gear and stores it in a strongbox.",
					false, ch, 0, vict, TO_VICT);
				House* house = Housing.findHouseByRoom( locker->in_room->number );
				if( house != NULL )
					house->save();
				ch->saveToXML();
			}
			break;
		}
	}

	if (FIGHTING(ch))
		stop_fighting(ch);
	if (FIGHTING(vict))
		stop_fighting(vict);

	act("$n throws $N into a cell and slams the door!", false, ch, 0, vict,
		TO_NOTVICT);
	act("$n throws you into a cell and slams the door behind you!\r\n", false,
		ch, 0, vict, TO_VICT);

	char_from_room(vict,false);
	char_to_room(vict, cell_rnum,false);
	if (IS_NPC(vict))
		cell_rnum->zone->enter_count++;

	look_at_room(vict, vict->in_room, 1);
	act("$n is thrown into the cell, and the door slams shut behind $m!",
		false, vict, 0, 0, TO_ROOM);

	if (HUNTING(ch) && HUNTING(ch) == vict)
		HUNTING(ch) = NULL;

	if ((torch = read_object(3030)))
		obj_to_char(torch, vict);

	mudlog(GET_INVIS_LVL(vict), NRM, true,
		"%s has been thrown into jail by %s at %d.", GET_NAME(vict),
		GET_NAME(ch), ch->in_room->number);
	return 1;
}

int
drag_char_to_jail(Creature *ch, Creature *vict, room_data *jail_room)
{
	CreatureList::iterator it;
	cityguard_data *data;
	int dir;

	if (IS_MOB(vict) && GET_MOB_SPEC(ch) == GET_MOB_SPEC(vict))
		return 0;

	if (ch->in_room == jail_room) {
		if (throw_char_in_jail(ch, vict)) {
			forget(ch, vict);
			return 1;
		} else
			return 0;
	}

	dir = find_first_step(ch->in_room, jail_room, STD_TRACK);
	if (dir < 0)
		return false;
	if (!CAN_GO(ch, dir)
			|| !can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0)
			|| !CAN_GO(vict, dir)
			|| !can_travel_sector(vict, SECT_TYPE(EXIT(ch, dir)->to_room), 0))
		return false;

	act("$n grabs you and says 'You're coming with me!'", false, ch, 0, vict,
		TO_VICT);
	act("$n grabs $N and says 'You're coming with me!'", false, ch, 0, vict,
		TO_NOTVICT);
	act(tmp_sprintf("$n drags you %s!\r\n", to_dirs[dir]),
		false, ch, 0, vict, TO_VICT);
	act(tmp_sprintf("$n drags $N %s!", to_dirs[dir]),
		false, ch, 0, vict, TO_NOTVICT);
	
	// Get other guards to follow
	for (it = ch->in_room->people.begin(); it != ch->in_room->people.end();it++) {
		if (IS_NPC(*it) && GET_MOB_SPEC(*it) == cityguard) {
			data = (cityguard_data *)(*it)->mob_specials.func_data;
			if (data)
				data->targ_room = EXIT(vict, dir)->to_room->number;
		}
	}

	char_from_room(ch,false);
	char_to_room(ch, EXIT(vict, dir)->to_room,false);
	char_from_room(vict,false);
	char_to_room(vict, ch->in_room,false);
	look_at_room(vict, vict->in_room, 0);

	act(tmp_sprintf("$n drags $N in from %s, punching and kicking!",
		from_dirs[dir]), false, ch, 0, vict, TO_NOTVICT);
	hit(ch, vict, TYPE_UNDEFINED);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return TRUE;
}

SPECIAL(cityguard)
{
	Creature *self = (struct Creature *)me;
	Creature *tch, *target, *new_guard;
	cityguard_data *data;
	char *str, *line, *param_key;
	int action, dir;
	int jail_num = 0, hq_num = 0;
	bool lawful;
	CreatureList::iterator it;

	if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_DEATH)
		return 0;

	str = GET_MOB_PARAM(self);
	if (str) {
		for (line = tmp_getline(&str);line;line = tmp_getline(&str)) {
			param_key = tmp_getword(&line);
			if (!strcmp(param_key, "jailroom"))
				jail_num = atoi(line);
			else if (!strcmp(param_key, "headquarters"))
				hq_num = atoi(line);
		}
	}

	data = (cityguard_data *)(self->mob_specials.func_data);
	if (!data) {
		CREATE(data, cityguard_data, 1);
		data->targ_room = 0;
		self->mob_specials.func_data = data;
	}

	if (spec_mode == SPECIAL_DEATH) {
		if (!hq_num)
			return 0;

		// make two new guards that will go to the place of death
		new_guard = read_mobile(GET_MOB_VNUM(self));
		CREATE(data, cityguard_data, 1);
		new_guard->mob_specials.func_data = data;
		data->targ_room = self->in_room->number;
		char_to_room(new_guard, real_room(hq_num));

		new_guard = read_mobile(GET_MOB_VNUM(self));
		CREATE(data, cityguard_data, 1);
		new_guard->mob_specials.func_data = data;
		data->targ_room = self->in_room->number;
		char_to_room(new_guard, real_room(hq_num));

		return 0;
	}

	if (!AWAKE(self))
		return false;

	// We're fighting someone - drag them to the jail
	if (FIGHTING(self)) {
		if (jail_num
				&&GET_MOB_WAIT(self) <= 5
				&& self->getPosition() >= POS_FIGHTING
				&& !number(0, 1)) {
			if (drag_char_to_jail(self, FIGHTING(self), real_room(jail_num)))
				return true;
		}

		if (number(0,3)) {
			call_for_help(self, FIGHTING(self));
			return true;
		}

		return false;
	}

	if (data->targ_room) {
		if (self->in_room->number != data->targ_room) {
			// We're marching to where someone shouted for help
			dir = find_first_step(self->in_room, real_room(data->targ_room),
				STD_TRACK);
			if (dir >= 0
					&& MOB_CAN_GO(self, dir)
					&& !ROOM_FLAGGED(self->in_room->dir_option[dir]->to_room, ROOM_DEATH)) {
				smart_mobile_move(self, dir);
				return true;
			}
		} else {
			// They're in the room, so stop trying to go to it
			data->targ_room = 0;
		}
	}

	// The action value is in order of importance
	// action == 0 : do emote
	// action == 1 : emote half-criminal
	// action == 2 : stop fight
	// action == 3 : attack criminal
	// action == 4 : assist guard
	action = 0;
	lawful = !ZONE_FLAGGED(self->in_room->zone, ZONE_NOLAW);

	tch = NULL;
	it = self->in_room->people.begin();
	for (; it != self->in_room->people.end(); ++it) {
		tch = *it;
		if (action < 4
				&& FIGHTING(tch)
				&& IS_NPC(FIGHTING(tch))
				&& GET_MOB_SPEC(FIGHTING(tch)) == cityguard) {
			action = 4;
			target = tch;
		}
		if (action < 3
				&& lawful
				&& can_see_creature(self, tch)
				&& !PRF_FLAGGED(tch, PRF_NOHASSLE)
				&& GET_REPUTATION(tch) >= CRIMINAL_REP) {
			action = 3;
			target = tch;
		}
		if (action < 2
				&& can_see_creature(self, tch)
				&& !PRF_FLAGGED(tch, PRF_NOHASSLE)
				&& FIGHTING(tch)) {
			action = 2;
			target = tch;
		}
		if (action < 1
				&& lawful
				&& can_see_creature(self, tch)
				&& !PRF_FLAGGED(tch, PRF_NOHASSLE)
				&& GET_REPUTATION(tch) >= CRIMINAL_REP / 2) {
			action = 1;
			target = tch;
		}
	}

	switch (action) {
	case 0:
		// do general emote
		if (number(0, 11))
			break;
		target = get_char_random_vis(self, self->in_room);
		if (!target)
			break;
		if (IS_GOOD(self) && IS_THIEF(target)) {
			if (IS_EVIL(target)) {
				act("$n looks at you suspiciously.", false,
					self, 0, tch, TO_VICT);
				act("$n looks at $N suspiciously.", false,
					self, 0, tch, TO_NOTVICT);
			} else {
				act("$n looks at you skeptically.", false,
					self, 0, tch, TO_VICT);
				act("$n looks at $N skeptically.", false,
					self, 0, tch, TO_NOTVICT);
			}
		} else if (cityguard == GET_MOB_SPEC(target)) {
			act("$n nods at $N.", false, self, 0, tch, TO_NOTVICT);
		} else if (((IS_CLERIC(target) || IS_KNIGHT(target))
					&& IS_EVIL(self) == IS_EVIL(target)
					&& !IS_NEUTRAL(target))
				|| GET_LEVEL(target) >= LVL_AMBASSADOR) {
			act("$n bows before you.", false,
				self, 0, tch, TO_VICT);
			act("$n bows before $N.", false,
				self, 0, tch, TO_NOTVICT);
		} else if (IS_EVIL(self) != IS_EVIL(target)) {
			switch (number(0,2)) {
			case 0:
				act("$n watches you carefully.", false, self, 0, tch,
					TO_VICT);
				act("$n watches $N carefully.", false,
					self, 0, tch, TO_NOTVICT);
				break;
			case 1:
				act("$n thoroughly examines you.", false,
					self, 0, tch, TO_VICT);
				act("$n thoroughly examines $N.", false,
					self, 0, tch, TO_NOTVICT);
			case 2:
				act("$n mutters something under $s breath.",
					false, self, 0, tch, TO_ROOM);
				break;
			}
		}
		break;
	case 1:
		// emote half-criminal
		if (!number(0, 3)) {
			act("$n growls at you.", false, self, 0, tch, TO_VICT);
			act("$n growls at $N.", false, self, 0, tch, TO_NOTVICT);
		} else if (!number(0, 2)) {
			act("$n cracks $s knuckles.", false, self, 0, tch, TO_ROOM);
		} else if (!number(0, 1) && GET_EQ(self, WEAR_WIELD) &&
			(GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
				(TYPE_SLASH - TYPE_HIT) ||
				GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
				(TYPE_PIERCE - TYPE_HIT) ||
				GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
				(TYPE_STAB - TYPE_HIT))) {
			act("$n sharpens $p while watching $N.",
				false, self, GET_EQ(self, WEAR_WIELD), tch, TO_NOTVICT);
			act("$n sharpens $p while watching you.",
				false, self, GET_EQ(self, WEAR_WIELD), tch, TO_VICT);
		}
		break;
	case 2:
		// stopping fight
		switch (number(0, 2)) {
		case 0:
			do_say(self, "Knock it off!", 0, SCMD_BELLOW, 0); break;
		case 1:
			do_say(self, "Stop disturbing the peace of this city!", 0, SCMD_BELLOW, 0); break;
		case 2:
			do_say(self, "Here now, here now!  Stop that!", 0, SCMD_BELLOW, 0); break;
		}
		act("You shove $n to the ground.", false, self, 0, target, TO_CHAR);
		act("$n shoves you to the ground.", false, self, 0, target, TO_VICT);
		act("$n shoves $N to the ground.", false, self, 0, target, TO_NOTVICT);
		stop_fighting(FIGHTING(target));
		stop_fighting(target);
		break;
	case 3:
		// attack criminal
		act("$n screams 'HEY!!!  You're one of those CRIMINALS!!!!!!'",
			false, self, 0, 0, TO_ROOM);
		hit(self, target, TYPE_UNDEFINED);
		break;
	case 4:
		// assist other cityguard
		if (!number(0, 10)) {
			if (number(0, 1))
				do_say(self, "To arms!  To arms!!", 0, SCMD_YELL, 0);
			else
				do_say(self, "BAAAANNNZZZZZAAAAAIIIIII!!!", 0, SCMD_YELL, 0);
		}
		act("You join the fight!.", false, self, 0, FIGHTING(target), TO_CHAR);
		act("$n assists you.", false, self, 0, FIGHTING(target), TO_VICT);
		act("$n assists $N.", false, self, 0, FIGHTING(target), TO_NOTVICT);
		hit(self, target, TYPE_UNDEFINED);
		break;
	default:
		slog("SYSERR: Can't happen at %s:%d", __FILE__, __LINE__); break;
	}

	return false;
}
