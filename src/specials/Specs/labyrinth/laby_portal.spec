// Don't allow player into labyrinth with equipment or items
SPECIAL(labyrinth_portal)
{
	obj_data *portal = (obj_data *)me;
	room_data *room;
	int idx;

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (!CMD_IS("enter"))
		return 0;

	skip_spaces(&argument);
	if (!isname(argument, portal->aliases))
		return 0;

	room = real_room(GET_OBJ_VAL(portal, 0));
	if (!room) {
		send_to_char(ch, "The labyrinth is currently closed for remodeling.\r\n");
		return 1;
	}

	// Must be ready to remort
	if (GET_LEVEL(ch) < 49 || GET_EXP(ch) < 1000000000) {
		send_to_char(ch, "The portal of the labyrinth repulses you.\r\n");
		return 1;
	}

	// One at a time, folks
	if (room->zone->num_players) {
		send_to_char(ch, "The portal of the labyrinth repulses you.\r\n");
		return 1;
	}

	// No equip
	for (idx = 0; idx < NUM_WEARS;idx++) 
		if (GET_EQ(ch, idx)) {
			send_to_char(ch, "The portal of the labyrinth repulses you.\r\n");
			return 1;
		}

	// No inventory
	if (ch->carrying) {
		send_to_char(ch, "The portal of the labyrinth repulses you.\r\n");
		return 1;
	}

	send_to_char(ch, "You enter the realm of the labyrinth.\r\n");
	act("$n steps into $p", true, ch, portal, 0, TO_ROOM);
	if (!IS_NPC(ch) && ch->in_room->zone != room->zone)
		room->zone->enter_count++;
	
	char_from_room(ch);
	char_to_room(ch, room);
	act("$n steps out of the portal.", true, ch, 0, 0, TO_ROOM);
	return 1;
}

