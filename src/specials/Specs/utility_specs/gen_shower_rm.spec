//
// File: gen_shower_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(gen_shower_rm)
{
	if (!CMD_IS("shower"))
		return 0;

	send_to_char(ch,
		"You turn on the shower and step under the pouring water.\r\n");
	act("$n turns on the shower and steps under the pouring water.", FALSE, ch,
		0, 0, TO_ROOM);

	if (IS_WEARING_W(ch) || IS_CARRYING_W(ch)) {
		send_to_char(ch, "All your things get soaking wet!\r\n");
		return 1;
	}
	send_to_char(ch,
		"The hot water runs over your naked body, refreshing you.\r\n");
	act("The hot water runs over $n's naked body.", FALSE, ch, 0, 0, TO_ROOM);
	GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20 * GET_LEVEL(ch));
	GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20 * GET_LEVEL(ch));
	return 1;
}
