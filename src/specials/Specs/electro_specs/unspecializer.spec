//
// File: unspecializer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

const int U_MODE_BUY = 1;
const int U_MODE_OFFER = 2;

SPECIAL(unspecializer)
{
	struct Creature *self = (struct Creature *)me;
	char *word, *msg;
	int mode = 0;
	struct obj_data *o_proto = NULL;
	int cash_cost = 0;
	int prac_cost = 0;
	int i;

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (CMD_IS("buy"))
		mode = U_MODE_BUY;
	else if (CMD_IS("offer"))
		mode = U_MODE_OFFER;
	else
		return 0;

	word = tmp_getword(&argument);

	if (!*arg || !is_abbrev(word, "unspecialization")) {
		msg =
			tmp_sprintf
			("You must type '%s unspecialization <weapon name>'\r\n",
			U_MODE_BUY ? "buy" : "offer");
		perform_tell(self, ch, msg);
		return 1;
	}

	word = tmp_getword(&argument);

	if (!*word) {
		perform_tell(self, ch,
			"You would like to remove your specialization in what?\r\n");
		return 1;
	}

	for (i = 0; i < MAX_WEAPON_SPEC; i++) {
		if (!GET_WEAP_SPEC(ch, i).vnum ||
			!GET_WEAP_SPEC(ch, i).level ||
			!(o_proto = real_object_proto(GET_WEAP_SPEC(ch, i).vnum)))
			continue;
		if (isname(word, o_proto->name))
			break;
	}

	if (i >= MAX_WEAPON_SPEC) {
		perform_tell(self, ch, "You aren't specialized in that weapon.\r\n");
		return 1;
	}

	prac_cost = GET_WEAP_SPEC(ch, i).level;
	cash_cost = GET_WEAP_SPEC(ch, i).level * 200000;

	msg = tmp_sprintf(
		"The service of complete neural erasure of the weapon specialization of\r\n"
		"'%s' will cost you %d pracs and %d credits....\r\n",
		o_proto->short_description, prac_cost, cash_cost);
	perform_tell(self, ch, msg);

	if (GET_PRACTICES(ch) < prac_cost) {
		msg = tmp_sprintf(
			"You don't have enough spare neurons!  It costs %d pracs, you know\r\n",
			prac_cost);
		perform_tell(self, ch, msg);
		return 1;
	}

	if (GET_CASH(ch) < cash_cost) {
		msg = tmp_sprintf(
			"You don't have enough cash!  I need %d credits for the job.\r\n",
			cash_cost);
		perform_tell(self, ch, msg);
		return 1;
	}

	if (mode == U_MODE_OFFER)
		return 1;

	GET_PRACTICES(ch) -= prac_cost;
	GET_CASH(ch) -= cash_cost;

	GET_WEAP_SPEC(ch, i).level = GET_WEAP_SPEC(ch, i).vnum = 0;

	act("\r\n$n takes your money and straps you into the chair.\r\n"
		"He attaches a cold, vaseline-coated probe to each of your temples and walks\r\n"
		"to one corner of the room.\r\n"
		"You see him pour some ether from a small glass vial onto a small\r\n"
		"white rag, which he then proceeds to press to his nose and inhale deeply.\r\n"
		"He stomps slowly back across the room on rubbery, wavering legs, and clamps\r\n"
		"a heavy metal helmet onto your head.  He then produces a large set of jumper\r\n"
		"cables.  He clips one end to your nipples, and the other end to his.  He then\r\n"
		"opens the door of a large, vented, metal case with a bang.  Inside you see\r\n"
		"a shiny metal rod, coated with conductive grease.  He grasps this rod firmly\r\n"
		"with his left hand.  He lays his right hand on a large double bladed knife\r\n"
		"switch on the side of your chair.\r\n\r\n"
		"He grins at you and says, 'This won't hurt a bit.\r\n\r\n"
		"You black out...\r\n\r\n", FALSE, self, 0, ch, TO_VICT);

	act("$n straps $N into the chair at the center of the room.\r\n"
		"$n proceeds to inhale heavily on an ether-laden rag, and then\r\n"
		"connects a series of cables to $N.\r\n"
		"$n then throws a large knife switch, apparently sending a strong\r\n"
		"current of electricity through $N's body.\r\n"
		"$N twitches and writhes for a while, and finally, seemingly\r\n"
		"prompted by some information on a glowing monitor,\r\n"
		"$n turns the switch off and walks away.\r\n\r\n"
		"$N lies on the chair motionless...\r\n\r\n", FALSE, self, 0, ch,
		TO_NOTVICT);

	WAIT_STATE(ch, 10 RL_SEC);

	return 1;
}

#undef U_MODE_BUY
#undef U_MODE_OFFER
