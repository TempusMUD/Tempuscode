//
// File: ramp_leaver.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(ramp_leaver)
{
	struct obj_data *me2 = (struct obj_data *)me;
	struct room_data *room;

	if (CMD_IS("down") && GET_OBJ_VAL(me2, 0) == 0) {
		if ((room = real_room(19431)) != NULL)
			room->dir_option[2]->to_room = real_room(19426);

		if ((room = real_room(19429)) != NULL)
			room->dir_option[0]->to_room = NULL;

		if ((room = real_room(19425)) != NULL)
			room->dir_option[0]->to_room = real_room(19426);

		GET_OBJ_VAL(me2, 0) = 1;
		send_to_room("You hear a rumbling from the south east.\n",
			ch->in_room);
		return (1);
	}

	if (CMD_IS("up") && GET_OBJ_VAL(me2, 0) == 1) {
		if ((room = real_room(19431)) != NULL)
			room->dir_option[2]->to_room = real_room(19430);

		if ((room = real_room(19429)) != NULL)
			room->dir_option[0]->to_room = real_room(19430);

		if ((room = real_room(19425)) != NULL)
			room->dir_option[0]->to_room = NULL;

		GET_OBJ_VAL(me2, 0) = 0;
		send_to_room("You hear a rumbling from the south east.\n",
			ch->in_room);
		return (1);
	}
	return (0);

}
