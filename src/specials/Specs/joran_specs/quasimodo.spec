//
// File: quasimodo.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(quasimodo)
{
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd)
		return 0;

	switch (number(0, 6)) {
	case 0:
		act("$n bows with a flourish.", FALSE, ch, 0, 0, TO_ROOM);
		do_say(ch, "Bienvenue, je m'appelle Quasimodo.", 0, 0);
		break;
	case 1:
		act("$n goes about his bell-ringing duties.", FALSE, ch, 0, 0,
			TO_ROOM);
		do_say(ch, "Cannot be late ringing the bells.", 0, 0);
		break;
	case 2:
		act("$n widens his grin, through rotten teeth.", FALSE, ch, 0, 0,
			TO_ROOM);
		break;
	case 3:
		do_say(ch,
			"Master has been most generous by giving me a place to live.", 0,
			0);
		break;
	case 4:
		act("$n grips the bell rope tightly and says a prayer.", FALSE,
			ch, 0, 0, TO_ROOM);
		do_say(ch, "May the next hour bring us hope.", 0, 0);
		break;
	case 5:
		do_say(ch, "You better leave now, the bells are sacred.", 0, 0);
		act("$n grunts and scratches his hump.", FALSE, ch, 0, 0, TO_ROOM);
		break;
	default:
		return (FALSE);
		break;
	}
	return 1;
}
