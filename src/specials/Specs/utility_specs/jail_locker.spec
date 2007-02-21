//
// File: jail_locker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(jail_locker)
{
	void summon_cityguards(room_data *);

	struct Creature *self = (struct Creature *)me;
	char *str, *line, *param_key;
	struct obj_data *locker, *item, *tmp_item;
	room_data *jail_room;
	int cost, jail_num = 0;

	if (spec_mode != SPECIAL_CMD)
		return 0;
	if (!CMD_IS("receive") && !CMD_IS("offer"))
		return 0;
	
	str = GET_MOB_PARAM(self);
	if (str) {
		for (line = tmp_getline(&str);line;line = tmp_getline(&str)) {
			param_key = tmp_getword(&line);
			if (!strcmp(param_key, "jailroom"))
				jail_num = atoi(line);
		}
	}

	if (jail_num == 0) {
		slog("Jailer #%d didn't have his jail set", GET_MOB_VNUM(self));
		perform_say(self, "say", "I can't seem to find my files...");
		return 0;
	}
	jail_room = real_room(jail_num);
	if (!jail_room) {
		slog("Jailer #%d couldn't find jail room #%d",
			GET_MOB_VNUM(self), jail_num);
		perform_say(self, "say", "I can't seem to find my files...");
		return 0;
	}

	if (IS_NPC(ch)) {
		perform_say(self, "say", "Sorry, I don't deal with mobiles.");
		return 1;
	}
	for (locker = jail_room->contents; locker; locker = locker->next_content)
		if (GET_OBJ_VNUM(locker) == JAIL_LOCKER_VNUM
				&& GET_OBJ_VAL(locker, 0) == GET_IDNUM(ch)
				&& locker->contains)
			break;

	if (IS_IMMORT(ch))
		cost = 0;
	else
		cost = MAX(10000, (GET_REPUTATION(ch) + 1)
			* GET_LEVEL(ch)
			* (GET_REMORT_GEN(ch) + 1)
			* 50);
    cost += (cost*ch->getCostModifier(self))/100;
	
	if (CMD_IS("offer")) {
		perform_say(ch, "say", "How much will it cost to get my stuff back, sir?");
		if (!locker) {
			perform_say(self, "say", "Sorry, you don't seem to have a locker here.");
		} else if (IS_CRIMINAL(ch)) {
			perform_say(self, "say", "Your belongings have been confiscated, criminal.");
			summon_cityguards(self->in_room);
		} else if (GET_TIME_FRAME(self->in_room) == TIME_FUTURE) {
			perform_say(self, "say", tmp_sprintf( "It will cost you %d credits, payable in cash.", cost));
		} else {
			perform_say(self, "say", tmp_sprintf( "It will cost you %d gold coins.", cost));
		}
		return 1;
	}

	perform_say(ch, "say", "I'd like to get my stuff back, please.");

	if (!locker) {
		perform_say(self, "say", "Sorry, you don't seem to have a locker here.");
		return 1;
	}

	if (IS_CRIMINAL(ch)) {
		perform_say(self, "say", "Your equipment has been confiscated for the good of the city.  Criminal.");
		summon_cityguards(self->in_room);
		return 1;
	}

	if (cost > 0) {
		if (GET_TIME_FRAME(self->in_room) == TIME_FUTURE) {
			if (GET_CASH(ch) >= cost) {
				perform_say(self, "say", tmp_sprintf(
					"That will be %d creds.  Try to stay out of trouble.",
					cost));
				GET_CASH(ch) -= cost;
			} else {
				perform_say(self, "say", tmp_sprintf("You don't have the %d creds bail which I require.", cost));
				return 1;
			}
		} else {
			if (GET_GOLD(ch) >= cost) {
				perform_say(self, "say", tmp_sprintf(
					"That will be %d gold coins.  Try to stay out of trouble.",
					cost));
				GET_GOLD(ch) -= cost;
			} else {
				perform_say(self, "smirk", tmp_sprintf(
					"You don't have the %d gold coins bail which I require.",
                    cost));
				return 1;
			}
		}
	}

	for (item = locker->contains; item; item = tmp_item) {
		tmp_item = item->next_content;
		obj_from_obj(item);
		obj_to_char(item, ch);
	}

	extract_obj(locker);

	act("$n opens a locker and gives you all your things.", FALSE,
		self, 0, ch, TO_VICT);
	act("$n opens a locker and gives $N all $S things.", FALSE, self,
		0, ch, TO_NOTVICT);

	slog("%s received jail impound at %d.", GET_NAME(ch),
		ch->in_room->number);

	House* house = Housing.findHouseByRoom( jail_room->number );
	if( house != NULL )
		house->save();
	ch->saveToXML();
	return 1;
}
