//
// File: cloak_of_deception.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cloak_of_deception)
{
	struct obj_data *cloak = (struct obj_data *)me;

	if (!CMD_IS("hide"))
		return 0;
	if (!cloak->worn_by || cloak->worn_by != ch)
		return 0;

	if (IS_AFFECTED(ch, AFF_HIDE))
		send_to_char(ch, "You are already hidden.\r\n");
	else {
		SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
		act("You wrap $p about your body and disappear.", FALSE, ch, cloak, 0,
			TO_CHAR);
	}
	return 1;
}
