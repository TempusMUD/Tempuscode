//
// File: black_rose.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(black_rose)
{
	byte num;
	struct obj_data *trash = (struct obj_data *)me;

	if (!CMD_IS("smell"))
		return 0;

	skip_spaces(&argument);

	if (!*argument)
		return 0;

	if (!isname(argument, trash->name))
		return 0;

	send_to_char(ch,
		"You smell the scent of the black rose and feel oddly invigorated!\r\n");
	act("$n smells $p.", TRUE, ch, trash, 0, TO_ROOM);

	num = number(10, 30);
	GET_MANA(ch) = MIN(GET_MANA(ch) + num, GET_MAX_MANA(ch));
	WAIT_STATE(ch, 2 RL_SEC);
	return 1;
}
