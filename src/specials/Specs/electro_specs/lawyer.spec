//
// File: lawyer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define KILLER 0
#define THIEF  1

#define KILLER_COST   3000000
#define THIEF_COST    1000000

SPECIAL(lawyer)
{
  struct char_data *lawy = (struct char_data *) me;
  int mode, cost = 0;
  char arg1[MAX_INPUT_LENGTH];

  if (!cmd || FIGHTING(lawy) || IS_NPC(ch))
    return 0;

  argument = one_argument(argument, arg1);
  skip_spaces(&argument);

  if (CMD_IS("value")) {
    if (!*arg1)
      send_to_char("Value what?  A pardon?\r\n", ch);
    else if (!is_abbrev(arg1, "pardon")) {
      sprintf(buf, "We dont sell any '%s'.\r\n", argument);
      send_to_char(buf, ch);
    } else {
      sprintf(buf, 
	      "The value of a pardon will be %d credits for thieves,\r\n"
	      "%d credits for killers, plus a %d credit fee.\r\n",
	      THIEF_COST, KILLER_COST, 
	      (GET_LEVEL(ch) * 300000) + GET_REMORT_GEN(ch) * 3000000);
      send_to_char(buf, ch);
    }
    return 1;
  }
   
  if (CMD_IS("buy")) {
    if (!*arg1) {
      send_to_char("Buy what?  A pardon?\r\n", ch);
      return 1;
    }
    else if (!is_abbrev(arg1, "pardon")) {
      sprintf(buf, "We dont sell any '%s'.\r\n", argument);
      send_to_char(buf, ch);
      return 1;
    }

    if (!*argument) {
      send_to_char("Buy a pardon for thief or killer?\r\n", ch);
      return 1;
    }
    if (is_abbrev(argument, "killer")) {
      if (!PLR_FLAGGED(ch, PLR_KILLER)) {
	send_to_char("You have not been accused of murder...\r\n"
		     "Got a guilty conscience?\r\n", ch);
	return 1;
      }
      mode = KILLER;
      cost = KILLER_COST;
    } else if (is_abbrev(argument, "thief")) {
      if (!PLR_FLAGGED(ch, PLR_THIEF)) {
	send_to_char("You have not been accused of larceny...  yet.\r\n",ch);
	return 1;
      }
      mode = THIEF;
      cost = THIEF_COST;
    }
    else {
      sprintf(buf, "We don't provide pardons for '%s'.\r\n", argument);
      send_to_char(buf, ch);
      return 1;
    }
    
    cost += GET_LEVEL(ch) * 300000;
    cost += GET_REMORT_GEN(ch) * 3000000;
	if(GET_LEVEL(ch) >= 51) {
		cost = 1;
	}
    if (GET_CASH(ch) < cost) {
      sprintf(buf, 
	      "That will cost you %d credits bucko..."
	      " which you don't have.\r\n", cost);
      
      send_to_char(buf, ch);
      act("$n snickers.", FALSE, lawy, 0, 0, TO_ROOM);
      return 1;
    }
    
    GET_CASH(ch) -= cost;
    sprintf(buf, "You pay %d credits for the pardon.\r\n", cost);
    send_to_char(buf, ch);

    if (mode == THIEF)
      REMOVE_BIT(PLR_FLAGS(ch), PLR_THIEF);
    else if (mode == KILLER)
      REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER);

    sprintf(buf,"%s paid %d credits to pardon a %s.",GET_NAME(ch),cost,mode == KILLER ? "KILLER" : "THIEF");
    mudlog(buf,NRM,LVL_IMMORT,TRUE);
    return 1;
    
  }
  return 0;
}      
