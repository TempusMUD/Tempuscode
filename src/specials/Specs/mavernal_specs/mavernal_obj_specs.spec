//
// File: mavernal_obj_specs.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(anti_spank_device)
{
	if (CMD_IS("spank")) {
		send_to_char(ch,
			"As you reach out your hand to give a sound spanking, you are\r\n"
			"repelled by an invisible force.  Your heart is filled with despair\r\n"
			"as your spanking goes undone.\r\n");
		return (TRUE);
	} else
		return (0);
}


SPECIAL(chastity_belt)
{

	struct obj_data *belt = (struct obj_data *)me;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (ch != belt->carried_by || !*arg || !isname(arg, belt->name))
		return 0;

	if (CMD_IS("wear")) {
		send_to_char(ch,
			"As you put the chastity belt on, you feel more pure.  You feel\r\n"
			"like a born-again virgin!\r\n");
		act("$N seems more chaste for some reason.", TRUE, ch, 0, 0, TO_ROOM);
	}
	return 0;
}
