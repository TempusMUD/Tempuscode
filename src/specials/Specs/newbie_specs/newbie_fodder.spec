#define NEWBIE_ROOM_MAX		467
#define NEWBIE_ROOM_MIN		436
#define NEWBIE_EQ_MAX			470
#define NEWBIE_EQ_MIN			420

SPECIAL(newbie_fodder)
{
	Creature *self = (Creature *)me;
	Creature *new_mob;
	room_data *new_loc;
	int idx, count;

	if (spec_mode != SPECIAL_DEATH)
		return 0;

	// Get replacement mobile
	new_mob = read_mobile(GET_MOB_VNUM(self));
	if (!new_mob)
		return 0;

	// Mobs have a 75% chance of carrying a random piece of newbie eq
	if (number(0, 3)) {
		obj_data *item;

		item = read_object(number(NEWBIE_EQ_MIN, NEWBIE_EQ_MAX));
		if (item)
			obj_to_char(item, new_mob);
	}

	// Now place a mob in a random room in which there are no players
	count = 0;
	for (idx = NEWBIE_ROOM_MIN;idx < NEWBIE_ROOM_MAX;idx++) {
		room_data *room;
		CreatureList::iterator it;
		
		room = real_room(idx);
		if (!room)
			continue;

		for(it = room->people.begin();it != room->people.end();it++)
			if (IS_PC(*it))
				break;
		if (it != room->people.end())
			continue;

		if (!number(0, count))
			new_loc = room;

		count++;
	}

	// Place the mob in the center of the room if the arena is filled
	// with players
	if (!new_loc)
		new_loc = real_room(440);

	// Move the replacement mob to its new home
	char_to_room(new_mob, new_loc, false);

	return 0;
}
