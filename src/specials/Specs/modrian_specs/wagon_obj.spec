//
// File: wagon_obj.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(wagon_obj)
{
	obj_data *self = (obj_data *)me;
	struct room_data *wagon_room;

	if (spec_mode == SPECIAL_TICK) {
		if (!self->in_room || number(0, 6))
			return false;

		switch (number(0,3)) {
		case 0:
			send_to_room("One of the wagon horses whinnies and stamps its foot.\r\n", self->in_room); break;
		case 1:
			send_to_room("A wagon horse nickers softly.\r\n", self->in_room); break;
		case 2:
			send_to_room("The wagon driver cracks his whip.\r\n", self->in_room); break;
		case 3:
			send_to_room("The wagon driver spits off the side of the wagon.\r\n", self->in_room); break;
		default:
			mudlog(LVL_IMMORT, CMP, true, "Can't happen at %s:%d", __FILE__, __LINE__);
		}
	} else if (spec_mode == SPECIAL_CMD) {

		if (!CMD_IS("board") && !CMD_IS("enter"))
			return false;

		skip_spaces(&argument);

		if (!*argument)
			return 0;

		if (strncasecmp(argument, "wagon", 5))
			return 0;

		wagon_room = real_room(10);
		if (!wagon_room) {
			act("You try to leap aboard the wagon, but fall flat on your ass!",
				true, ch, 0, 0, TO_CHAR);
			act("$n tries to leap aboard the wagon and falls flat on $s ass!",
				true, ch, 0, 0, TO_ROOM);
			mudlog(LVL_IMMORT, CMP, true, "Wagon can't find its inside");
			return true;
		}

		send_to_char(ch, "You deftly leap aboard the wagon.\r\n\r\n");
		act("$n deftly leaps aboard the wagon.", TRUE, ch, 0, 0, TO_ROOM);

		char_from_room(ch, false);
		char_to_room(ch, wagon_room, false);
		look_at_room(ch, ch->in_room, 1);

		act("$n climbs onto the wagon.", TRUE, ch, 0, 0, TO_ROOM);
		return true;
	}

	return false;
}
