//
// File: portal_home.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(portal_home)
{
	struct obj_data *portal = (struct obj_data *)me;
	struct room_data *room = NULL;
	int flags;

	if (spec_mode != SPECIAL_CMD || !CMD_IS("enter") || portal->worn_by ||
		portal->in_obj || (portal->carried_by && portal->carried_by != ch))
		return 0;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "Enter what?\r\n");
		return 1;
	}
	if (!(room = portal->in_room))
		room = portal->carried_by->in_room;

	if (isname(argument, portal->name)) {
		flags = room->room_flags;
		REMOVE_BIT(room->room_flags, ROOM_NOMAGIC);
		act("$n steps into $p.", FALSE, ch, portal, 0, TO_ROOM);
		act("You step into $p.", FALSE, ch, portal, 0, TO_CHAR);
		call_magic(ch, ch, 0, SPELL_WORD_OF_RECALL, GET_LEVEL(ch), CAST_SPELL);
		room->room_flags = flags;
		WAIT_STATE(ch, 2 RL_SEC);
		return 1;
	}
	return 0;
}
