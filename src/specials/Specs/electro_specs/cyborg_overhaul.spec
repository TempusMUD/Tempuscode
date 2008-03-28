//
// File: cyborg_overhaul.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cyborg_overhaul)
{

	struct Creature *borg = (struct Creature *)me;
	int cost;

	if (!cmd || (!CMD_IS("buy") && !CMD_IS("value")))
		return 0;

	if (!IS_CYBORG(ch)) {
		perform_tell(borg, ch,
			"I am not programmed to deal with non-cyborgs.");
		return 1;
	}

	skip_spaces(&argument);

	if (!strcasecmp(argument, "overhaul")) {
		cost = GET_TOT_DAM(ch);
		if (GET_BROKE(ch))
			cost += GET_LEVEL(ch) * 100;
        
        cost += (cost*ch->getCostModifier(borg))/100;
        
        
		if (!cost) {
			perform_tell(borg, ch, "You are not in need of overhaul.");
			return 1;
		}

		if (CMD_IS("value")) {
			sprintf(buf2, "Cost of operation: %d credits.", cost);
			perform_tell(borg, ch, buf2);
			act("$N interfaces momentarily with $n.", TRUE, ch, 0, borg,
				TO_ROOM);
			return 1;
		}

		if (GET_CASH(ch) < cost) {
			sprintf(buf2, "You do not have the %d required credits.", cost);
			perform_tell(borg, ch, buf2);
			return 1;
		}

		sprintf(buf2, "Your account has been debited %d credits.", cost);
		perform_tell(borg, ch, buf2);

		act("$n lies down on the table and enters a static state.\r\n"
			"$N begins to overhaul $n.", FALSE, ch, 0, borg, TO_ROOM);
		act("You lie down on the table and enter a static state.\r\n"
			"$N begins to overhaul you.", FALSE, ch, 0, borg, TO_CHAR);

		ch->setPosition(POS_SLEEPING);
		SET_BIT(AFF3_FLAGS(ch), AFF3_STASIS);

		GET_TOT_DAM(ch) = 0;
		GET_BROKE(ch) = 0;
		GET_CASH(ch) -= cost;
		WAIT_STATE(ch, 10 RL_SEC);

		return 1;

	}

	if (!strcasecmp(argument, "repairs")) {
		cost = (GET_MAX_HIT(ch) - GET_HIT(ch)) * 100;
        cost += (cost*ch->getCostModifier(borg))/100;
        
		if (!cost) {
			perform_tell(borg, ch, "You are not in need of repairs.");
			return 1;
		}

		if (CMD_IS("value")) {
			sprintf(buf2, "Cost of operation: %d credits.", cost);
			perform_tell(borg, ch, buf2);
			act("$N interfaces momentarily with $n.", TRUE, ch, 0, borg,
				TO_ROOM);
			return 1;
		}

		if (GET_CASH(ch) < cost) {
			sprintf(buf2, "You do not have the %d required credits.", cost);
			perform_tell(borg, ch, buf2);
			return 1;
		}

		sprintf(buf2, "Your account has been debited %d credits.", cost);
		perform_tell(borg, ch, buf2);

		act("$N begins to perform repairs on you.", FALSE, ch, 0, borg,
			TO_CHAR);
		act("$N begins to perform repairs on $n.", FALSE, ch, 0, borg,
			TO_ROOM);

		GET_HIT(ch) = GET_MAX_HIT(ch);
		GET_CASH(ch) -= cost;
		WAIT_STATE(ch, 6 RL_SEC);
		return 1;

	}

	send_to_char(ch, "You may 'buy overhaul', 'buy repairs',\r\n"
		"'value overhaul', or 'value repairs'.\r\n");
	return 0;
}
