//
// File: sunstation.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void set_desc_state(int state, struct descriptor_data *d);

SPECIAL(sunstation)
{
	struct obj_data *sun = (struct obj_data *)me;

	if (spec_mode != SPECIAL_CMD || !ch->desc || !CMD_IS("login"))
		return 0;

	act("You log in on $p.", FALSE, ch, sun, 0, TO_CHAR);
	act("$n logs in on $p.", FALSE, ch, sun, 0, TO_ROOM);
	set_desc_state(CXN_NETWORK, ch->desc);
	return 1;
}
