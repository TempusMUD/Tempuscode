//
// File: dt_cleaner.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(dt_cleaner)
{
	extern struct time_info_data time_info;
	struct zone_data *zone;
	struct room_data *room;
	struct obj_data *obj, *tmp_obj;
	int found = 0;

	if (!CMD_IS("clear") || (cmd && GET_LEVEL(ch) < LVL_ELEMENT) ||
		(!cmd && !(time_info.hours % 3)))
		return 0;

	for (zone = zone_table; zone; zone = zone->next)
		for (room = zone->world; room; room = room->next) {
			if (IS_SET(ROOM_FLAGS(room), ROOM_DEATH) && !room->people) {
				if (room->contents) {
					found = 1;
					sprintf(buf, "Room %d : %s cleared.\r\n", room->number,
						room->name);
					send_to_room(buf, ch->in_room);
				}
				for (obj = room->contents; obj; obj = obj) {
					tmp_obj = obj;
					obj = obj->next_content;
					extract_obj(tmp_obj);
				}
			}
		}
	if (!found)
		send_to_room("All DT's are clear.\r\n", ch->in_room);
	return 1;
}
