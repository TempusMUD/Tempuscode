//
// File: nohunger_dude.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define LVL_CAN_GAIN_NOHUNGER  40	/* <-----Set desired level here */

SPECIAL(nohunger_dude)
{
	struct Creature *dude = (struct Creature *)me;
	int gold, life_cost, mode;
	char buf[MAX_STRING_LENGTH];

	if (!CMD_IS("gain"))
		return 0;
	skip_spaces(&argument);

	life_cost = MAX(10, 70 - GET_INT(ch) - GET_WIS(ch) - GET_CON(ch));
	gold = 10000 * GET_LEVEL(ch);
    gold += (gold*ch->getCostModifier(dude))/100;

	if (!*argument) {
		send_to_char(ch, "Gain what?\r\n"
			"It will cost you %d life points and %d gold coins to gain\r\n"
			"nohunger, nothirst, or nodrunk.\r\n", life_cost, gold);
	} else if (GET_LEVEL(ch) < LVL_CAN_GAIN_NOHUNGER && !IS_REMORT(ch))
		send_to_char(ch, "You are not yet ready to gain this.\r\n");
	else {
		if (!is_abbrev(argument, "nohunger")) {
			if (!is_abbrev(argument, "nothirst")) {
				if (!is_abbrev(argument, "nodrunk")) {
					perform_tell(dude, ch,
						"You can only gain 'nohunger', 'nothirst', or 'nodrunk'.");
					return 1;
				} else
					mode = DRUNK;
			} else
				mode = THIRST;
		} else
			mode = FULL;

		if (GET_COND(ch, mode) < 0) {
			sprintf(buf, "You are already unaffected by %s.",
				mode == DRUNK ? "alcohol" : mode ==
				THIRST ? "thirst" : "hunger");
			perform_tell(ch, dude, buf);
			return 1;
		}

		if (GET_GOLD(ch) < gold) {
			sprintf(buf,
				"You don't have the %d gold coins I require for that.", gold);
			perform_tell(dude, ch, buf);
		} else if (GET_LIFE_POINTS(ch) < life_cost) {
			sprintf(buf,
				"You haven't gained the %d life points you need to do that.",
				life_cost);
			perform_tell(dude, ch, buf);
		} else {
			act("$n outstretches $s arms in supplication to the powers that be.", TRUE, dude, 0, 0, TO_ROOM);
			send_to_char(ch,
				"You feel a strange sensation pass through your soul.\r\n");
			act("A strange expression crosses $N's face...", TRUE, dude, 0, ch,
				TO_NOTVICT);
			GET_LIFE_POINTS(ch) -= life_cost;
			GET_GOLD(ch) -= gold;
			GET_COND(ch, mode) = -1;
			send_to_char(ch, "You will no longer be affected by %s!\r\n",
				mode == DRUNK ? "alcohol" : mode ==
				THIRST ? "thirst" : "hunger");

			slog("%s has gained %s.", GET_NAME(ch),
				mode == DRUNK ? "nodrunk" : mode == THIRST ? "nothirst" :
				"nohunger");

			return 1;
		}
	}
	return 1;
}
