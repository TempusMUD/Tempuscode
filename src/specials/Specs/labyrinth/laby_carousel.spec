// This is a fake special, used as a flag to the pendulum_timer mob
// the exits will "spin" clockwise every pendulum tick
SPECIAL(labyrinth_carousel)
{
	return 0;
}

void
labyrinth_spin_carousels(zone_data *zone)
{
	room_data *cur_room;
	CreatureList::iterator it;
	room_direction_data *dir_save[NUM_DIRS];

	for (cur_room = zone->world;cur_room && cur_room->number < zone->top;cur_room = cur_room->next) {
		if (cur_room->func != labyrinth_carousel)
			continue;

		// Spin the room!  Wheee!
		memcpy(dir_save, cur_room->dir_option, sizeof(room_direction_data) * NUM_DIRS);
		cur_room->dir_option[NORTH] = dir_save[WEST];
		cur_room->dir_option[EAST] = dir_save[NORTH];
		cur_room->dir_option[SOUTH] = dir_save[EAST];
		cur_room->dir_option[WEST] = dir_save[SOUTH];

		if (cur_room->people.empty())
			continue;

		// Knock people down and describe if there are observers
		act("The floor lurches beneath your feet!", true, 0, 0, 0, TO_ROOM);
		it = cur_room->people.begin();
		for (;it != cur_room->people.end();++it)
			if (number(1, 26) > GET_DEX(*it)) {
				send_to_char(*it, "You fall to the floor!  Oof!\r\n");
				(*it)->setPosition(POS_SITTING);
			}
	}
}

