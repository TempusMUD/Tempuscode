//
// File: gen_locker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//



#define CONVERT_TIME_TO_CONT(time) \
     (time.year * 15960 + time.month * 840 + time.day * 24 + time.hours)


SPECIAL(gen_locker)
{
	struct Creature *atten = (struct Creature *)me;
	struct obj_data *locker, *item, *tmp_item;
	struct room_data *locker_room = NULL;
	int locker_vnum = -1;
	int cost_factor = 0;		/* scale 0-10 = 1-100% (-1 means cannot store here) */
	int rent = 0, hours = 0;
	int found = 0;

	ACMD(do_say);

	if (!cmd)
		return 0;

	if (ch->in_room->number == 2346) {	/* Newbie tower     */
		locker_room = real_room(2347);
		locker_vnum = 2311;
		/*    cost_factor = 50; */
	} else if (ch->in_room->number == 30113) {	/* Camshaft Fitness */
		locker_room = real_room(30198);
		locker_vnum = 30076;
		cost_factor = 10;
	} else if (ch->in_room->number == 30134) {	/* VR */
		locker_room = real_room(30197);
		locker_vnum = 30189;
		cost_factor = -1;
	} else {
		locker_room = ch->in_room->next ? ch->in_room->next : NULL;
		locker_vnum = locker_room->number;
		cost_factor = 75;
	}
    
    cost_factor += (cost_factor*ch->getCostModifier(atten))/100;
    
	skip_spaces(&argument);

	if (CMD_IS("offer")) {

		if (cost_factor < 0)
			return 0;

		if (IS_NPC(ch)) {
			do_say(atten, "Sorry, I cannot store things for mobiles.", 0, 0, 0);
			return 1;
		}

		if (!ch->carrying) {
			perform_tell(atten, ch, "You aren't carrying anything.");
			return 1;
		}

		if (!locker_room || !real_object_proto(locker_vnum)) {
			send_to_char(ch,
				"Sorry, the locker room is out of service right now.\r\n");
			errlog("locker room with no storage room or locker.");
			return 1;
		}

		for (locker = locker_room->contents; locker;
			locker = locker->next_content) {
			if (GET_OBJ_VNUM(locker) != locker_vnum ||
				GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch))
				continue;

			break;
		}
		if (!locker) {			/* can't find a current locker, look for empty one */

			for (locker = locker_room->contents; locker;
				locker = locker->next_content) {

				if (GET_OBJ_VNUM(locker) != locker_vnum ||
					GET_OBJ_VAL(locker, 0) || locker->contains)
					continue;

				break;
			}

			if (!locker) {
				perform_tell(atten, ch,
					"Sorry, all the lockers are occupied at the moment.");
				return 1;
			}
		}

		for (item = ch->carrying; item; item = tmp_item) {
			tmp_item = item->next_content;
			if (IS_OBJ_STAT(item, ITEM_NORENT | ITEM_NODROP))
				continue;
			found = 1;
			rent += GET_OBJ_RENT(item);
		}

		if (!found) {
			perform_tell(atten, ch,
				"You're not carrying anything I'm willing to store.");
			return 1;
		}

		rent = (rent * cost_factor) / 100;

		send_to_char(ch,
			"The down payment will be %d %s.\r\n"
			"I will store your stuff for %d %s per day.\r\n",
			cost_factor * 10,
			ch->in_room->zone->time_frame ==
			TIME_ELECTRO ? "credits" : "coins", rent,
			ch->in_room->zone->time_frame ==
			TIME_ELECTRO ? "credits" : "coins");
		return 1;
	}

	if (CMD_IS("store")) {

		if (cost_factor < 0)
			return 0;

		if (IS_NPC(ch)) {
			do_say(atten, "Sorry, I cannot store things for mobiles.", 0, 0, 0);
			return 1;
		}

		if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
			sprintf(buf, "Why don't you kiss my ass, %s.", GET_NAME(ch));
			perform_tell(atten, ch, buf);
			return 1;
		}

		if (!ch->carrying) {
			perform_tell(atten, ch, "You aren't carrying anything.");
			return 1;
		}

		if (!locker_room || !real_object_proto(locker_vnum)) {
			send_to_char(ch,
				"Sorry, the locker room is out of service right now.\r\n");
			errlog("locker room with no storage room or locker.");
			return 1;
		}

		if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
			if (GET_CASH(ch) < cost_factor * 10) {
				send_to_char(ch,
					"You don't have the %d credit down payment required.\r\n",
					cost_factor * 10);
				return 1;
			}
		} else if (GET_GOLD(ch) < cost_factor * 10) {
			send_to_char(ch,
				"You don't have the %d coin down payment required.\r\n",
				cost_factor * 10);
			return 1;
		}

		for (locker = locker_room->contents; locker;
			locker = locker->next_content) {
			if (GET_OBJ_VNUM(locker) != locker_vnum ||
				GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch))
				continue;

			break;
		}
		if (!locker) {			/* can't find a current locker, look for empty one */

			for (locker = locker_room->contents; locker;
				locker = locker->next_content) {

				if (GET_OBJ_VNUM(locker) != locker_vnum ||
					GET_OBJ_VAL(locker, 0) || locker->contains)
					continue;

				break;
			}

			if (!locker) {
				perform_tell(atten, ch,
					"Sorry, all the lockers are occupied at the moment.");
				return 1;
			}
		}

		for (item = ch->carrying; item; item = tmp_item) {
			tmp_item = item->next_content;
			if (IS_OBJ_STAT(item, ITEM_NORENT | ITEM_NODROP))
				continue;
			found = 1;
			obj_from_char(item);
			obj_to_obj(item, locker);
			rent += GET_OBJ_RENT(item);
		}

		if (!found) {
			perform_tell(atten, ch,
				"You're not carrying anything I'm willing to store.");
			return 1;
		}

		GET_OBJ_VAL(locker, 0) = GET_IDNUM(ch);
		rent *= cost_factor;
		rent /= 100;
		GET_OBJ_VAL(locker, 1) += rent;
		GET_OBJ_VAL(locker, 2) = CONVERT_TIME_TO_CONT(time_info);

		if (ch->in_room->zone->time_frame == TIME_ELECTRO)
			GET_CASH(ch) -= cost_factor * 10;
		else
			GET_GOLD(ch) -= cost_factor * 10;

		House* house = Housing.findHouseByRoom( locker->in_room->number );
		if( house != NULL )
			house->save();
		ch->saveToXML();

		act("$n takes all your things and locks them in a locker.", FALSE,
			atten, 0, ch, TO_VICT);
		act("$n takes all $N's things and locks them in a locker.", FALSE,
			atten, 0, ch, TO_NOTVICT);

		if (GET_OBJ_VAL(locker, 1)) {
			send_to_char(ch,
				"It will cost %d %s per day to keep the locker.\r\n",
				GET_OBJ_VAL(locker, 1),
				ch->in_room->zone->time_frame ==
				TIME_ELECTRO ? "credits" : "coins");
			return 1;
		}
		return 1;

	}

	if (CMD_IS("receive")) {

		if (IS_NPC(ch)) {
			do_say(atten, "Sorry, I don't deal with mobiles.", 0, 0, 0);
			return 1;
		}

		if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
			sprintf(buf, "Why don't you kiss my ass, %s.", GET_NAME(ch));
			perform_tell(atten, ch, buf);
			return 1;
		}

		if (!locker_room || !real_object_proto(locker_vnum)) {
			send_to_char(ch,
				"Sorry, the locker room is out of service right now.\r\n");
			errlog("locker room with no storage room or locker.");
			return 1;
		}

		for (locker = locker_room->contents; locker;
			locker = locker->next_content) {
			if (GET_OBJ_VNUM(locker) != locker_vnum
				|| GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
				|| (!locker->contains))
				continue;
			break;
		}
		if (!locker || GET_OBJ_VNUM(locker) != locker_vnum ||
			GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
			|| (!locker->contains)) {
			do_say(atten, "Sorry, you don't seem to have a locker here.", 0,
				0, 0);
			return 1;
		}

		if (GET_OBJ_VAL(locker, 1)) {

			hours =
				CONVERT_TIME_TO_CONT(time_info) - GET_OBJ_VAL(locker, 2) + 1;
			rent = (hours * GET_OBJ_VAL(locker, 1)) / 24;

			if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
				if (GET_CASH(ch) < rent) {
					send_to_char(ch,
						"You don't have the %d credits required.\r\n", rent);
					return 1;
				} else
					GET_CASH(ch) -= rent;
			} else if (GET_GOLD(ch) < rent) {
				send_to_char(ch, "You don't have the %d coins required.\r\n",
					rent);
				return 1;
			} else
				GET_GOLD(ch) -= rent;

			send_to_char(ch, "The cost will be %d %s.\r\n", rent,
				ch->in_room->zone->time_frame == TIME_ELECTRO ?
				"credits" : "coins");

		}

		for (item = locker->contains; item; item = tmp_item) {
			tmp_item = item->next_content;
			obj_from_obj(item);
			obj_to_char(item, ch);
		}

		GET_OBJ_VAL(locker, 0) = 0;
		GET_OBJ_VAL(locker, 1) = 0;
		GET_OBJ_VAL(locker, 2) = 0;

		House* house = Housing.findHouseByRoom( locker->in_room->number );
		if( house != NULL )
			house->save();
		ch->saveToXML();

		act("$n opens a locker and gives you all your things.",
			FALSE, atten, 0, ch, TO_VICT);
		act("$n opens a locker and gives $N all $S things.",
			FALSE, atten, 0, ch, TO_NOTVICT);
		return 1;
	}
	return 0;
}
