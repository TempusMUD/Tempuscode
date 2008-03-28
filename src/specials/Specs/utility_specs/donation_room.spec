//
// File: donation_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(donation_room)
{
	if (!CMD_IS("get") && !CMD_IS("take"))
		return 0;

	if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
		send_to_char(ch, "Very funny, asshole.\r\n");
		return 1;
	}

	skip_spaces(&argument);
	if (argument && !strncasecmp(argument, "all", 3) && GET_LEVEL(ch) < 51 ) {
		send_to_char(ch, "One at a time, please.\r\n");
		return 1;
	}
	return 0;
}
