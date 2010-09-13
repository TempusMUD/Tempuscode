//
// File: duke_nukem.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(duke_nukem)
{

	struct creature *duke = (struct creature *)me, *vict = NULL;
	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (cmd || !AWAKE(duke))
		return 0;

	if (isFighting(duke)) {
        vict = findRandomCombat(duke);
		if (GET_HIT(duke) > (GET_MAX_HIT(duke) >> 2) &&
			GET_HIT(vict) < (GET_MAX_HIT(vict) >> 1)) {
			if (!number(0, 10))
				perform_say(duke, "say", "You're an inspiration for birth control.");
			else if (!number(0, 10))
				perform_say(duke, "say",
                            "What are you, some bottom feeding, scum sucking algae eater?");
			else if (!number(0, 10))
				perform_say(duke, "say", "It hurts to be you.");
			else if (!number(0, 10))
				perform_say(duke, "say", "Eat shit and die.");
			else if (!number(0, 10))
				perform_say(duke, "say", "Blow it out your ass.");
			else if (IS_MALE(vict) && !number(0, 10))
				perform_say(duke, "say", "Die, you son of a bitch.");
			else if (IS_FEMALE(vict) && !number(0, 10))
				perform_say(duke, "say", "Shake it baby.");
			else if (GET_HIT(vict) < 50)
				perform_say(duke, "say", "Game Over.");
		}
		return 0;
	}

	if (MOB_HUNTING(duke) && can_see_creature(duke, MOB_HUNTING(duke))) {
		if (!number(0, 10))
			perform_tell(duke, MOB_HUNTING(duke),
				"What are you, some bottom feeding, scum sucking algae eater?");
		else if (!number(0, 10))
			perform_tell(duke, MOB_HUNTING(duke), "Come get some.");
		else if (!number(0, 10))
			perform_tell(duke, MOB_HUNTING(duke),
				"I'll rip off your head and shit down your neck.");
		return 0;
	}

	if (!number(0, 5)) {
		    for (GList *it = ch->fighting;it;it = it->next) {
			vict = it->data;
			if (vict == duke || !can_see_creature(duke, vict))
				continue;
			if (GET_LEVEL(vict) > 40 && !isFighting(vict) &&
				!PRF_FLAGGED(vict, PRF_NOHASSLE)) {
				best_initial_attack(duke, vict);
				return 1;
			} else if (!number(0, 10)) {
                perform_say_to(duke, vict, "Damn.  You're ugly.");
				return 1;
			}
		}
	}

	switch (number(0, 1000)) {
	case 0:
		do_gen_comm(duke, tmp_strdup("Damn.  I'm looking good."), 0, SCMD_GOSSIP, 0);
		break;
	case 1:
		do_gen_comm(duke, tmp_strdup("Hail to the king, baby!"), 0, SCMD_HOLLER, 0);
		break;
	case 2:
		do_gen_comm(duke, tmp_strdup("Your face, your ass.  What's the difference?"),
			0, SCMD_SPEW, 0);
		break;
	case 3:
	case 4:
		do_gen_comm(duke, tmp_strdup("Holy Shit!"), 0, SCMD_SPEW, 0);
		break;
	case 5:
		do_gen_comm(duke, tmp_strdup("Hehehe.  What a mess."), 0, SCMD_GOSSIP, 0);
		break;
	case 6:
		do_gen_comm(duke, tmp_strdup("Damn I'm good."), 0, SCMD_GOSSIP, 0);
		break;
	case 7:
		do_gen_comm(duke, tmp_strdup("Hmm.  Don't have time to play with myself."),
			0, SCMD_SPEW, 0);
		break;
	case 8:
		do_gen_comm(duke, tmp_strdup("Suck it down."), 0, SCMD_SPEW, 0);
		break;
	case 9:
		do_gen_comm(duke, tmp_strdup("Gonna rip 'em a new one."), 0, SCMD_SPEW, 0);
		break;
	case 10:
		do_gen_comm(duke, tmp_strdup("It's time to kick ass and chew bubble gum.\r\n"
                                     "And I'm all out of gum."),
                    0, SCMD_SHOUT, 0);
		break;
	case 11:
		do_gen_comm(duke, tmp_strdup("This really pisses me off."), 0, SCMD_SPEW, 0);
		break;
	case 12:
		do_gen_comm(duke, tmp_strdup("Shit happens."), 0, SCMD_SPEW, 0);
		break;
	case 13:
		perform_say(duke, "say", "Ready for action.");
		break;
	case 14:
		do_gen_comm(duke, tmp_strdup("Ready for action."), 0, SCMD_SHOUT, 0);
		break;
	case 15:
		perform_say(duke, "say", "Cool.");
		break;
	case 16:
		do_gen_comm(duke, tmp_strdup("Let God sort 'em out."), 0, SCMD_SPEW, 0);
		break;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
		do_gen_comm(duke, tmp_strdup("Who wants some?"), 0, SCMD_GOSSIP, 0);
		break;
	case 23:
		do_gen_comm(duke,
                    tmp_strdup("Those alien bastards are gonna PAY for shooting up my ride!"),
                    0, SCMD_SHOUT, 0);
		break;
	default:
		break;
	}

	return 0;
}
