//
// File: monastery_eating.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(monastery_eating)
{
	if (!CMD_IS("eat"))
		return 0;

	if (GET_COND(ch, FULL) >= 24) {
		send_to_char(ch, "You are already completely filled.\r\n");
		return 1;
	}

	send_to_char(ch, "You enjoy a good free meal.\r\n");
	gain_condition(ch, FULL, 10);

	return 1;
}
