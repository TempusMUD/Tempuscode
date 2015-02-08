// This is a fake special, used as a flag to the pendulum_timer mob
// the exits will "spin" clockwise every pendulum tick
SPECIAL(labyrinth_carousel)
{
    return 0;
}

void
labyrinth_spin_carousels(struct zone_data *zone)
{
    struct room_data *cur_room;
    struct room_direction_data *dir_save[NUM_DIRS];

    for (cur_room = zone->world;
         cur_room && cur_room->number < zone->top; cur_room = cur_room->next) {
        if (cur_room->func != labyrinth_carousel) {
            continue;
        }

        // Spin the room!  Wheee!
        memcpy(dir_save, cur_room->dir_option, sizeof(dir_save));
        cur_room->dir_option[NORTH] = dir_save[WEST];
        cur_room->dir_option[EAST] = dir_save[NORTH];
        cur_room->dir_option[SOUTH] = dir_save[EAST];
        cur_room->dir_option[WEST] = dir_save[SOUTH];

        if (cur_room->people) {
            continue;
        }

        // Knock people down and describe if there are observers
        act("The floor lurches beneath your feet!", true, NULL, NULL, NULL, TO_ROOM);
        for (GList *it = cur_room->people; it; it = it->next) {
            struct creature *vict = it->data;
            if (number(1, 26) > GET_DEX(vict)) {
                send_to_char(vict, "You fall to the floor!  Oof!\r\n");
                GET_POSITION(vict) = POS_SITTING;
            }
        }
    }
}
