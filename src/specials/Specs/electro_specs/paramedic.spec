//
// File: paramedic.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(paramedic)
{

	Creature *pm = (Creature *) me;

	if (!cmd)
		return 0;

	// price lists
	if (CMD_IS("list")) {

		if (IS_CYBORG(ch))
			send_to_char(ch, "Sorry, we cannot service cyborgs here!\r\n");
		else if (!CAN_SEE(pm, ch))
			send_to_char(ch, "The paramedic cannot see you.\r\n");
		else
			send_to_char(ch, "You can buy the following services:\r\n"
				"Healing           -- 100 credits / 100 HPS\r\n"
				"Stim              -- 100 credits / 100 MOVE\r\n"
				"Detox             -- 100 credits\r\n");

		return 1;
	}
	// buy the service
	if (CMD_IS("buy")) {

		if (IS_CYBORG(ch)) {
			send_to_char(ch, "Sorry, we cannot service cyborgs here!\r\n");
			return 1;
		}

		if (!CAN_SEE(pm, ch)) {
			send_to_char(ch, "The paramedic cannot see you.\r\n");
			return 1;
		}
		if (!*argument) {
			send_to_char(ch, "Buy what service?\r\n");
			return 1;
		}

		skip_spaces(&argument);


		// healing
		if (is_abbrev(argument, "healing")) {

			if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
				send_to_char(ch, "You are already fully healed.\r\n");
				return 1;
			}

			if (GET_CASH(ch) < 100) {
				send_to_char(ch,
					"That costs 100 credits, which you cannot afford.\r\n");
				return 1;
			}

			GET_HIT(ch) = (MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 100));
			GET_CASH(ch) -= 100;

			act("$n performs first aid on $N.", TRUE, pm, 0, ch, TO_NOTVICT);
			act("$n performs first aid on you.", TRUE, pm, 0, ch, TO_VICT);

			send_to_char(ch, "You feel better.\r\n");

			WAIT_STATE(ch, 2 RL_SEC);
			return 1;
		}
		// stim
		if (is_abbrev(argument, "stim")) {

			if (GET_MOVE(ch) >= GET_MAX_MOVE(ch)) {
				send_to_char(ch, "You are already fully stimulated.\r\n");
				return 1;
			}

			if (GET_CASH(ch) < 100) {
				send_to_char(ch,
					"That costs 100 credits, which you cannot afford.\r\n");
				return 1;
			}

			GET_MOVE(ch) = (MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 100));
			GET_CASH(ch) -= 100;

			act("$n administers a stim-pack to $N.", TRUE, pm, 0, ch,
				TO_NOTVICT);
			act("$n administers a stim-pack to you.", TRUE, pm, 0, ch,
				TO_VICT);

			send_to_char(ch, "You feel stimulated.\r\n");

			WAIT_STATE(ch, 2 RL_SEC);
			return 1;
		}
		// detox
		if (is_abbrev(argument, "detox")) {

			if (!affected_by_spell(ch, SPELL_POISON) &&
				GET_COND(ch, DRUNK) <= 0) {
				send_to_char(ch,
					"Sorry, there is no need to detoxify you.\r\n");
				return 1;
			}

			if (GET_CASH(ch) < 100) {
				send_to_char(ch,
					"That costs 100 credits, which you cannot afford.\r\n");
				return 1;
			}

			affect_from_char(ch, SPELL_POISON);

			if (GET_COND(ch, DRUNK) >= 0)
				GET_COND(ch, DRUNK) = 0;

			GET_CASH(ch) -= 100;

			act("$n performs a detoxification on $N.", TRUE, pm, 0, ch,
				TO_NOTVICT);
			act("$n performs a detoxification on you.", TRUE, pm, 0, ch,
				TO_VICT);

			send_to_char(ch, "You feel less toxic.\r\n");

			WAIT_STATE(ch, 5 RL_SEC);

			return 1;
		}

		send_to_char(ch, "You can't buy such a service.  Type LIST.\r\n");
		return 1;
	}

	return 0;
}
