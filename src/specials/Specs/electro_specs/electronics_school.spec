//
// File: electronics_school.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(electronics_school)
{
  struct char_data *teacher = (struct char_data *) me;
  int  gold_cost = 0, prac_cost = 0;
  
  if (!CMD_IS("learn") && !CMD_IS("train") && !CMD_IS("offer"))
    return 0;


  gold_cost = (GET_LEVEL(ch) << 6) + 2000;
  prac_cost = MAX(3, 21 - GET_INT(ch));

  if (IS_CYBORG(ch)) {
    perform_tell(teacher,ch,"Why don't you just download the program, borg?!");
    return 1;
  }

  if (CMD_IS("offer")) {
    sprintf(buf2, "You can enroll for %d coins.  "
	    "It will require %d practice points as well.", 
	    gold_cost, prac_cost);
    perform_tell(teacher, ch, buf2);
    strcpy(buf2, "You may not learn the skill with a single lesson, however.");
    perform_tell(teacher, ch, buf2);
    return 1;
  }

  if (GET_SKILL(ch, SKILL_ELECTRONICS) >= LEARNED(ch)) {
    perform_tell(teacher,ch,"I cannot teach you any more about electronics.");
    return 1;
  }

  if (GET_CASH(ch) < gold_cost) {
    sprintf(buf2, "You don't have the %d credit tuition I require.", gold_cost);
    perform_tell(teacher, ch, buf2);
    return 1;
  }

  if (GET_PRACTICES(ch) < prac_cost) {
    perform_tell(teacher, ch, "Come back when you have more practices!");
    return 1;
  }


  GET_PRACTICES(ch) -= prac_cost;
  GET_CASH(ch) -= gold_cost;

  GET_SKILL(ch, SKILL_ELECTRONICS) = 
    MIN(LEARNED(ch), GET_SKILL(ch, SKILL_ELECTRONICS) + (30 + GET_INT(ch)));
  
  act("$n begins to study electronics.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You pay %s %d coins and begin to study electronics.\r\n",
	  PERS(teacher, ch), gold_cost);

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);

  return 1;
}
    
