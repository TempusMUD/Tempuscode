//
// File: modrian_fountain_obj.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(modrian_fountain_obj)
{
	struct obj_data *fount = (struct obj_data *)me;
	struct room_data *fountain_room;
	one_argument(argument, arg);

	if ((fountain_room = real_room(2899)) == NULL)
		return 0;
	if (!CMD_IS("enter"))
		return 0;

	if (isname(arg, fount->name)) {
		act("$n leaps into $p.", FALSE, ch, fount, 0, TO_ROOM);
		act("You leaps into $p.", FALSE, ch, fount, 0, TO_CHAR);
		char_from_room(ch, false);
		char_to_room(ch, fountain_room, false);
		act("$n leaps into $p, splashig water all over you.", FALSE, ch, fount,
			0, TO_ROOM);
		look_at_room(ch, ch->in_room, 0);
		return 1;
	}
	return 0;
}
