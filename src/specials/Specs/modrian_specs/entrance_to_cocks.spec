//
// File: entrance_to_cocks.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(entrance_to_cocks)
{
	if (!CMD_IS("north"))
		return 0;
	if (GET_LEVEL(ch) > 5 && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch,
			"You are not needed in there!  Go play with something bigger.\r\n");
		return 1;
	}
	return 0;
}
