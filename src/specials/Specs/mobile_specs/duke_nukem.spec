//
// File: duke_nukem.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void best_attack(struct char_data *ch, struct char_data *vict);

SPECIAL(duke_nukem)
{

  struct char_data *duke = (struct char_data *) me, *vict = NULL;

  if (cmd || !AWAKE(duke))
    return 0;


  if (FIGHTING(duke)) {
    if (GET_HIT(duke) > (GET_MAX_HIT(duke) >> 2) &&
	GET_HIT(FIGHTING(duke)) < (GET_MAX_HIT(FIGHTING(duke)) >> 1)) {
      if (!number(0, 10))
	do_say(duke, "You're an inspiration for birth control.", 0, 0);
      else if (!number(0, 10))
	do_say(duke, "What are you, some bottom feeding, scum sucking algae eater?", 0, 0);
      else if (!number(0, 10))
	do_say(duke, "It hurts to be you.", 0, 0);
      else if (!number(0, 10))
	do_say(duke, "Eat shit and die.", 0, 0);
      else if (!number(0, 10))
	do_say(duke, "Blow it out your ass.", 0, 0);
      else if (IS_MALE(FIGHTING(duke)) && !number(0, 10))
	do_say(duke, "Die, you son of a bitch.", 0, 0);
      else if (IS_FEMALE(FIGHTING(duke)) && !number(0, 10))
	do_say(duke, "Shake it baby.", 0, 0);
      else if (GET_HIT(FIGHTING(duke)) < 50)
	do_say(duke, "Game Over.", 0, 0);
    }
    return 0;
  }

  if (HUNTING(duke) && CAN_SEE(duke, HUNTING(duke))) {
    if (!number(0, 10))
      perform_tell(duke, HUNTING(duke), "What are you, some bottom feeding, scum sucking algae eater?");
    else if (!number(0, 10))
      perform_tell(duke, HUNTING(duke), "Come get some.");
    else if (!number(0, 10))
      perform_tell(duke, HUNTING(duke), "I'll rip off your head and shit down your neck.");
    return 0;
  }
  
  if (!number(0, 5)) {
    for (vict = duke->in_room->people; vict; vict = vict->next_in_room) {
      if (vict == duke || !CAN_SEE(duke, vict))
	continue;
      if (GET_LEVEL(vict) > 40 && !FIGHTING(vict) && 
	  !PRF_FLAGGED(vict, PRF_NOHASSLE)) {
	best_attack(duke, vict);
	return 1;
      } else if (!number(0, 10)) {
	sprintf(buf2, ">%s Damn.  You're ugly.", fname(vict->player.name));
	do_say(duke, buf2, 0, SCMD_SAY_TO);
	return 1;
      }
    }
  }

  switch (number(0, 1000)) {
  case 0:
    do_gen_comm(duke, "Damn.  I'm looking good.", 0, SCMD_GOSSIP);
    break;
  case 1:
    do_gen_comm(duke, "Hail to the king, baby!", 0, SCMD_HOLLER);
    break;
  case 2:
    do_gen_comm(duke, "Your face, your ass.  What's the difference?", 
		0, SCMD_SPEW);
    break;
  case 3:
  case 4:
    do_gen_comm(duke, "Holy Shit!", 0, SCMD_SPEW);
    break;
  case 5:
    do_gen_comm(duke, "Hehehe.  What a mess.", 0, SCMD_GOSSIP);
    break;
  case 6:
    do_gen_comm(duke, "Damn I'm good.", 0, SCMD_GOSSIP);
    break;
  case 7:
    do_gen_comm(duke, "Hmm.  Don't have time to play with myself.",
		0, SCMD_SPEW);
    break;
  case 8:
    do_gen_comm(duke, "Suck it down.", 0, SCMD_SPEW);
    break;
  case 9:
    do_gen_comm(duke, "Gonna rip 'em a new one.", 0, SCMD_SPEW);
    break;
  case 10:
    do_gen_comm(duke, "It's time to kick ass and chew bubble gum.\r\n"
		"And I'm all out of gum.", 0, SCMD_SHOUT);
    break;
  case 11:
    do_gen_comm(duke, "This really pisses me off.", 0, SCMD_SPEW);
    break;
  case 12:
    do_gen_comm(duke, "Shit happens.", 0, SCMD_SPEW);
    break;
  case 13:
    do_say(duke, "Ready for action.", 0, 0);
    break;
  case 14:
    do_gen_comm(duke, "Ready for action.", 0, SCMD_SHOUT);
    break;
  case 15:
    do_say(duke, "Cool.", 0, 0);
    break;
  case 16:
    do_gen_comm(duke, "Let God sort 'em out.", 0, SCMD_SPEW);
    break;
  case 17:
  case 18:
  case 19:
  case 20:
  case 21:
  case 22:
    do_gen_comm(duke, "Who wants some?", 0, SCMD_GOSSIP);
    break;
  case 23:
    do_gen_comm(duke, "Those alien bastards are gonna PAY for shooting up my ride!", 0, SCMD_SHOUT);
    break;
  default:
    break;
  }
    
  return 0;
}
