//
// File: jail_locker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(jail_locker)
{
	void summon_cityguards(room_data *);

	struct Creature *self = (struct Creature *)me;
	struct obj_data *locker, *item, *tmp_item;
	int cost;
	char *str, *line, *param_key;
	room_data *jail_room;
	int jail_num = 0;

	ACMD(do_say);

	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	str = GET_MOB_PARAM(self);
	if (str) {
		for (line = tmp_getline(&str);line;line = tmp_getline(&str)) {
			param_key = tmp_getword(&line);
			if (!strcmp(param_key, "jailroom"))
				jail_num = atoi(line);
		}
	}

	if (!CMD_IS("receive"))
		return 0;

	if (jail_num == 0) {
		slog("Jailer #%d didn't have his jail set", GET_MOB_VNUM(self));
		do_say(self, "I can't seem to find my files...", 0, 0, 0);
		return 0;
	}
	jail_room = real_room(jail_num);
	if (!jail_room) {
		slog("Jailer #%d couldn't find jail room #%d",
			GET_MOB_VNUM(self), jail_num);
		do_say(self, "I can't seem to find my files...", 0, 0, 0);
		return 0;
	}
	do_say(ch, "I'd like to get my stuff back, please.", 0, 0, 0);
	if (IS_NPC(ch)) {
		do_say(self, "Sorry, I don't deal with mobiles.", 0, 0, 0);
		return 1;
	}
	if (IS_CRIMINAL(ch)) {
		do_say(self, "Your equipment has been confiscated for the good of the city.  Criminal.", 0, 0, 0);
		summon_cityguards(self->in_room);
		return 1;
	}

	if (!IS_IMMORT(ch)) {
		cost = MAX(10000, (GET_REPUTATION(ch) + 1)
			* GET_LEVEL(ch)
			* (GET_REMORT_GEN(ch) + 1)
			* 50);

		if (GET_TIME_FRAME(self->in_room) == TIME_FUTURE) {
			if (GET_CASH(ch) < cost) {
				do_say(self, tmp_sprintf(
					"You don't have the %d creds bail which I require.",
						cost), 0, 0, 0);
				return 1;
			}
		} else {
			if (GET_GOLD(ch) < cost) {
				do_say(self, tmp_sprintf(
					"You don't have the %d gold coins bail which I require.",
						cost), 0, 0, 0);
				return 1;
			}
		}
	}

	for (locker = jail_room->contents; locker; locker = locker->next_content) {
		if (GET_OBJ_VNUM(locker) != JAIL_LOCKER_VNUM
				|| GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
				|| (!locker->contains))
			continue;
		for (item = locker->contains; item; item = tmp_item) {
			tmp_item = item->next_content;
			obj_from_obj(item);
			obj_to_char(item, ch);
		}
		GET_OBJ_VAL(locker, 0) = 0;
		act("$n opens a locker and gives you all your things.", FALSE,
			self, 0, ch, TO_VICT);
		act("$n opens a locker and gives $N all $S things.", FALSE, self,
			0, ch, TO_NOTVICT);
		if (GET_TIME_FRAME(self->in_room) == TIME_FUTURE) {
			do_say(self, tmp_sprintf(
				"That will be %d creds.  Try to stay out of trouble.",
				cost), 0, 0, 0);
			GET_CASH(ch) -= cost;
		} else {
			do_say(self, tmp_sprintf(
				"That will be %d gold coins.  Try to stay out of trouble.",
				cost), 0, 0, 0);
			GET_GOLD(ch) -= cost;
		}

		slog("%s received jail impound at %d.", GET_NAME(ch),
			ch->in_room->number);

		House* house = Housing.findHouseByRoom( jail_room->number );
		if( house != NULL )
			house->save();
		ch->saveToXML();
		return 1;
	}
	do_say(self, "Sorry, you don't seem to have a locker here.", 0, 0, 0);
	return 1;
}
