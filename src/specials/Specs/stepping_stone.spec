//
// File: stepping_stone.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stepping_stone)
{
	struct obj_data *ruby = (struct obj_data *)me;
	extern room_num arena_start_room;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (CMD_IS("south")) {
		if (ch->getPosition() >= POS_STANDING) {
			if (GET_LOADROOM(ch) != arena_start_room) {
				act("$p flares up suddenly with a bright light!",
					false, ch, ruby, 0, TO_ROOM);
				send_to_char(ch, "You feel a strange sensation...\r\n");
				sprintf(buf,
					"A voice BOOMS out, 'Welcome to the Arena, %s!'\r\n",
					GET_NAME(ch));
				send_to_zone(buf, ch->in_room->zone, 0);
				GET_LOADROOM(ch) = arena_start_room;
			}
		}
	}
	return false;
}

SPECIAL(portal_out)
{
	struct obj_data *portal = (struct obj_data *)me;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (!CMD_IS("enter"))
		return false;
	if (!*argument) {
		send_to_char(ch,
			"Enter what?  Enter the portal to leave the arena.\r\n");
		return true;
	}

	skip_spaces(&argument);
	if (isname(argument, portal->aliases)) {
		send_to_room("A loud buzzing sound fills the room.\r\n", ch->in_room);
		GET_LOADROOM(ch) = 0;
		sprintf(buf, "A voice BOOMS out, '%s has left the arena.'\r\n",
			GET_NAME(ch));
		send_to_zone(buf, ch->in_room->zone, 0);
		call_magic(ch, ch, 0, NULL, SPELL_WORD_OF_RECALL, LVL_GRIMP, CAST_SPELL);
		return true;
	}
	return false;
}


SPECIAL(arena_locker)
{
	struct Creature *atten = (struct Creature *)me;
	struct obj_data *locker, *item, *tmp_item;
	struct room_data *r_locker_room;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (!(r_locker_room = real_room(40099)))
		return false;

	if (CMD_IS("store")) {
		perform_say(ch, "say", "I'd like to store my stuff, please.");
		if (IS_NPC(ch)) {
			perform_say(atten, "say", "Sorry, I cannot store things for mobiles.");
			return true;
		}
		if (!(IS_CARRYING_W(ch) + IS_WEARING_W(ch))) {
			perform_say(atten, "smirk", "Looks to me like you're already stark naked.");
			return true;
		}
		if (IS_WEARING_W(ch)) {
			perform_say(atten, "say", "You need to remove all your gear first.");
			return true;
		}
		for (locker = r_locker_room->contents; locker;
			locker = locker->next_content) {
			if (GET_OBJ_VNUM(locker) != 40099 || GET_OBJ_VAL(locker, 0)
				|| locker->contains)
				continue;
			for (item = ch->carrying; item; item = tmp_item) {
				tmp_item = item->next_content;
				obj_from_char(item);
				obj_to_obj(item, locker);
			}
			GET_OBJ_VAL(locker, 0) = GET_IDNUM(ch);
			act("$n takes all your things and locks them in a locker.",
				false, atten, 0, ch, TO_VICT);
			act("You are now stark naked!", false, atten, 0, ch, TO_VICT);
			act("$n takes all $N's things and locks them in a locker.",
				false, atten, 0, ch, TO_NOTVICT);
			act("$N is now stark naked!", false, atten, 0, ch, TO_NOTVICT);
			return true;
		}
		perform_say(atten, "apologize", "Sorry, all the lockers are occupied at the moment.");
		return true;
	}
	if (CMD_IS("receive")) {
		perform_say(ch, "say", "I'd like to get my stuff back , please.");
		if (IS_NPC(ch)) {
			perform_say(atten, "say", "Sorry, I don't deal with mobiles.");
			return true;
		}

		for (locker = r_locker_room->contents; locker;
			locker = locker->next_content) {
			if (GET_OBJ_VNUM(locker) != 40099
				|| GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
				|| (!locker->contains))
				continue;
			if (GET_OBJ_VNUM(locker) != 40099 ||
				GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
				|| (!locker->contains)) {
				perform_say(atten, "apologize", "Sorry, you don't seem to have a locker here.");
				return true;
			}
			for (item = locker->contains; item; item = tmp_item) {
				tmp_item = item->next_content;
				obj_from_obj(item);
				obj_to_char(item, ch);
			}
			GET_OBJ_VAL(locker, 0) = 0;
			act("$n opens a locker and gives you all your things.",
				false, atten, 0, ch, TO_VICT);
			act("$n opens a locker and gives $N all $S things.",
				false, atten, 0, ch, TO_NOTVICT);

			House *house = Housing.findHouseByRoom( r_locker_room->number );
			if( house != NULL )
				house->save();
			ch->saveToXML();

			return true;
		}
		perform_say(atten, "say", "Sorry, you don't seem to have a locker here.");
		return true;
	}
	return false;
}
