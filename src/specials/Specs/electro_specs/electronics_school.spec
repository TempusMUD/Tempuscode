//
// File: electronics_school.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// Random Act Of Electronics
static void
electronics_raoe(Creature *self)
{
	switch (number(0, 7)) {
	case 0:
		act("$n begins poring through a parts manual.",
			false, self, 0, 0, TO_ROOM);
		break;
	case 1:
		act("$n dabs a few drops of solder on a circuit board.",
			false, self, 0, 0, TO_ROOM);
		break;
	case 2:
		act("$n gets a part out of a box.", false, self, 0, 0, TO_ROOM);
		break;
	case 3:
		act("$n frowns as $e breaks a wire.", false, self, 0, 0, TO_ROOM);
		break;
	case 4:
		act("Sparks fly as $n toggles a switch.", false, self, 0, 0, TO_ROOM);
		break;
	case 5:
		act("$n gazes serenely at a frenetically gyrating oscilloscope",
			false, self, 0, 0, TO_ROOM);
		break;
	case 6:
		act("$n quietly curses as $e burns $mself on $s soldering iron.",
			false, self, 0, 0, TO_ROOM);
		break;
	case 7:
		act("$n pushes a button, which blinks a few times and stops.",
			false, self, 0, 0, TO_ROOM);
		break;
	default:
		break;
	}
}

SPECIAL(electronics_school)
{
	Creature *self = (Creature *)me;
	int cred_cost = 0;

	if (spec_mode == SPECIAL_TICK) {
		if (!number(0, 20))
			electronics_raoe(self);
		return 0;
	}
		
	if (!CMD_IS("learn") && !CMD_IS("train") && !CMD_IS("offer"))
		return 0;

	cred_cost = (GET_LEVEL(ch) << 6) + 2000;
	cred_cost += (cred_cost*ch->getCostModifier(self))/100;
    
	if (!can_see_creature(self, ch)) {
		do_say(self, "I can't train ya if I can't see ya, see?", 0, 0, 0);
		return 1;
	}

	if (IS_CYBORG(ch)) {
		do_say(self, tmp_sprintf(
			"%s You're a borg, why don't you just download the program?",
			GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
		return 1;
	}

	if (CMD_IS("offer")) {
		do_say(self, tmp_sprintf(
			"%s Yeah, I'll give you a lesson for %d creds.",
			GET_NAME(ch), cred_cost), 0, SCMD_SAY_TO, NULL);
		return 1;
	}

	if (GET_SKILL(ch, SKILL_ELECTRONICS) >= LEARNED(ch)) {
		do_say(self, tmp_sprintf(
			"%s I cannot teach you any more about electronics.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return 1;
	}

	if (GET_CASH(ch) < cred_cost) {
		do_say(self, tmp_sprintf(
			"%s You don't have the %d cred tuition I require.", GET_NAME(ch),
			cred_cost), 0, SCMD_SAY_TO, NULL);
		return 1;
	}


	GET_CASH(ch) -= cred_cost;

	GET_SKILL(ch, SKILL_ELECTRONICS) =
		MIN(LEARNED(ch), GET_SKILL(ch,
			SKILL_ELECTRONICS) + (30 + GET_INT(ch)));

	send_to_char(ch, "You pay %d creds to %s and are given a short electronics lesson.\r\n",
		cred_cost, PERS(self, ch));
	act("$n pays $N, who gives an impromptu electronics lesson.",
		true, ch, 0, self, TO_ROOM);

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

	return 1;
}
