//
// File: circus_clown.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(circus_clown)
{

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_ENTER
		&& spec_mode != SPECIAL_TICK)
		return 0;
	if (!cmd && !ch->numCombatants() && ch->getPosition() != POS_FIGHTING) {
		switch (number(0, 30)) {
		case 0:
			act("$n flips head over heels.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You flip head over heels.\r\n");
			break;
		case 1:
			act("$n grabs your nose and screams 'HONK!'.", TRUE, ch, 0, 0,
				TO_ROOM);
			send_to_char(ch, "You grab everyone's honker!\r\n");
			break;
		case 2:
			act("$n rolls around you like a cartwheel.", TRUE, ch, 0, 0,
				TO_ROOM);
			send_to_char(ch, "You cartwheel around.\r\n");
			break;
		case 3:
			act("$n starts walking around on $s hands.", TRUE, ch, 0, 0,
				TO_ROOM);
			break;
		case 4:
			act("$n trips on $s shoelaces and falls flat on $s face!", TRUE,
				ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You trip and fall.\r\n");
			break;
		case 5:
			act("$n eats a small piece of paper.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You hit some acid.\r\n");
			break;
		case 6:
			act("$n rolls a joint and licks it enthusiastically.", TRUE, ch, 0,
				0, TO_ROOM);
			send_to_char(ch, "You roll a joint.\r\n");
			break;
		case 7:
			act("$n guzzles a beer and smashes the can against $s forehead.",
				TRUE, ch, 0, 0, TO_ROOM);
			send_to_char(ch,
				"You guzzle a beer and smash the can on you forehead.\r\n");
			break;
		}
		return 1;
	}
	return 0;
}
