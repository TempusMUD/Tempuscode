//
// File: free_bricks.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(free_bricks)
{

	struct obj_data *obj = NULL;

	if ((!CMD_IS("get") && !CMD_IS("take")) || !AWAKE(ch))
		return 0;

	skip_spaces(&argument);

	if (str_cmp(argument, "brick"))
		return 0;

	for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		if (isname(argument, obj->name))
			return 0;

	if (CAN_CARRY_N(ch) <= IS_CARRYING_N(ch)) {
		send_to_char(ch, "You cannot carry any more items.\r\n");
		return 1;
	}

	if (CAN_CARRY_W(ch) <= IS_WEARING_W(ch) + IS_CARRYING_W(ch) + 5) {
		send_to_char(ch, "You cannot carry any more weight.\r\n");
		return 1;
	}

	if (!(obj = read_object(50002)))
		return 0;

	send_to_char(ch, "You get a broken brick from the pile of rubble.\r\n");
	act("$n gets $p from the pile of rubble.", TRUE, ch, obj, 0, TO_ROOM);

	obj_to_char(obj, ch);

	return 1;
}
