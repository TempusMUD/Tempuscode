//
// File: lawyer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define KILLER 0
#define THIEF  1

SPECIAL(lawyer)
{
	struct Creature *lawy = (struct Creature *)me;
	int mode, cost = 0;
	char arg1[MAX_INPUT_LENGTH];

	if (!cmd || FIGHTING(lawy) || IS_NPC(ch))
		return 0;

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (CMD_IS("value")) {
		if (!*arg1)
			send_to_char(ch, "Value what?  A pardon?\r\n");
		else if (!is_abbrev(arg1, "pardon")) {
			send_to_char(ch, "We dont sell any '%s'.\r\n", argument);
		} else {
			send_to_char(ch,
				"For you, I'll make you a deal for %d credits.\r\n"
				"That's better than you'll get from the bounty hunters.\r\n",
				GET_SEVERITY(ch) * 1000000);
		}
		return 1;
	}

	if (CMD_IS("buy")) {
		if (!*arg1) {
			send_to_char(ch, "Buy what?  A pardon?\r\n");
			return 1;
		} else if (!is_abbrev(arg1, "pardon")) {
			send_to_char(ch, "We dont sell any '%s'.\r\n", argument);
			return 1;
		}

		if (!*argument) {
			send_to_char(ch, "Buy a pardon for thief or killer?\r\n");
			return 1;
		}
		if (is_abbrev(argument, "killer")) {
			if (!PLR_FLAGGED(ch, PLR_KILLER)) {
				send_to_char(ch, "You have not been accused of murder...\r\n"
					"Got a guilty conscience?\r\n");
				return 1;
			}
			mode = KILLER;
		} else if (is_abbrev(argument, "thief")) {
			if (!PLR_FLAGGED(ch, PLR_THIEF)) {
				return 1;
			}
			mode = THIEF;
		} else {
			send_to_char(ch, "We don't provide pardons for '%s'.\r\n",
				argument);
			return 1;
		}

		cost = GET_SEVERITY(ch) * 1000000;

		if (GET_LEVEL(ch) >= 51) {
			cost = 1;
		}
		if (GET_CASH(ch) < cost) {
			send_to_char(ch,
				"That will cost you %d credits bucko..."
				" which you don't have.\r\n", cost);

			act("$n snickers.", FALSE, lawy, 0, 0, TO_ROOM);
			return 1;
		}

		GET_CASH(ch) -= cost;
		send_to_char(ch, "You pay %d credits for the pardon.\r\n", cost);

		if (mode == THIEF)
			REMOVE_BIT(PLR_FLAGS(ch), PLR_THIEF);
		else if (mode == KILLER)
			REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER);

		mudlog(LVL_IMMORT, NRM, true,
			"%s paid %d credits to pardon a %s.",
			GET_NAME(ch), cost, mode == KILLER ? "KILLER" : "THIEF");
		return 1;

	}
	return 0;
}
