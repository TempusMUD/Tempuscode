//
// File: newbie_improve.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_improve)
{

#define STR  0
#define INT  1
#define WIS  2
#define DEX  3
#define CON  4
#define CHA  5

  ACMD(do_say);
  void perform_tell(struct char_data *ch, struct char_data *vict, char *mssg);
  struct char_data *impro = (struct char_data *) me;
  byte index = -1;
  char *mssg = NULL;
  char buf3[MAX_STRING_LENGTH];

  if (!CMD_IS("improve") && !CMD_IS("train"))
    return 0;
  
  if (GET_LEVEL(ch) > 7) {
    sprintf(buf3, "Get out of here, %s.  I cannot help you.", GET_NAME(ch));
    do_say(impro, buf3, 0, 0);
    return 1;
  } else if (GET_LEVEL(ch) > 5) {
    sprintf(buf3, "I am no longer able to train you, %s.", GET_NAME(ch));
    do_say(impro, buf3, 0, 0);
    return 1;
  }

  skip_spaces(&argument);
  
  if (strlen(argument) == 0) {
    send_to_char("Your statistics are what define your character physically and mentally.\r\n"
                 "These statistics are: Strength, Intellegence, Wisdom, Dexterity,\r\n"
                 "Constitution, and Charisma.  To learn more about any one of these,\r\n"
                 "activate the help screen by typing 'help strength', for example.\r\n"
                 "To have a look at how your stats stack up, type 'attributes'.\r\n\r\n"
                 "Each of your statistics may be raised to a certain level.  You may\r\n"
                 "increase your each of your statistics to its maximum, by spending\r\n"
                 "your life points.  To improve, type 'improve' followed by one\r\n"
                 "of the following: str, int, wis, dex, con, cha.\r\n\r\n", ch);
    sprintf(buf, "You currently have %d life points remaining.\r\n", 
                  GET_LIFE_POINTS(ch));
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_LIFE_POINTS(ch) <= 0) {
    send_to_char("You have no more life points left.  From now on, you will gain life\r\n"
                 "points slowly as you increase in experience.\r\n", ch);
    return 1;
  }

  if (!strn_cmp(argument, "s", 1))
    index = STR;
  else if (!strn_cmp(argument, "i", 1))
    index = INT;
  else if (!strn_cmp(argument, "w", 1))
    index = WIS;
  else if (!strn_cmp(argument, "d", 1))
    index = DEX;
  else if (!strn_cmp(argument, "co", 2))
    index = CON;
  else if (!strn_cmp(argument, "ch", 2))
    index = CHA;
  else {
    send_to_char("You must specify one of:\r\n"
                 "strength, intelligence, wisdom,\r\n"
                 "dexterity, constitution, charisma.\r\n", ch);
    return 1;
  }
  switch (index) {
  case STR:
    if ((ch->real_abils.str == 18 && ch->real_abils.str_add >= 100) || 
	ch->real_abils.str > 18) {
      send_to_char("Your strength is already at the peak of mortal ablility.\r\n", ch);
      return 1;
    }
    if (ch->real_abils.str < 18)
      ch->real_abils.str++;
    else if (ch->real_abils.str == 18) 
      ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
    mssg = "Your strength improves!\r\n";
    break;
  case INT:
    if (ch->real_abils.intel >= 18) {
      send_to_char("You have already reached the peak of mortal intellegence.\r\n", ch);
      return 1;
    }
    ch->real_abils.intel++;
    mssg = "Your intellegence improves!\r\n";
    break;
  case WIS:
    if (ch->real_abils.wis >= 18) {
      send_to_char("Your wisdom is already legendary.\r\n", ch);
      return 1;
    }
    ch->real_abils.wis++;
    mssg = "Your become wiser!\r\n";
    break;
  case DEX:
    if (ch->real_abils.dex >= 18) {
      send_to_char("You have already reached the peak of mortal agility.\r\n", ch);
      return 1;
    }
    ch->real_abils.dex++;
    mssg = "Your dexterity improves!\r\n";
    break;
  case CON:
    if (ch->real_abils.con >= 18) {
      send_to_char("You have already reached the peak of mortal health.\r\n", ch);
      return 1;
    }
    ch->real_abils.con++;
    mssg = "Your health improves!\r\n";
    break;
  case CHA:
    if (ch->real_abils.cha >= 18) {
      send_to_char("You have already reached the peak of mortal charisma.\r\n", ch);
      return 1;
    }
    ch->real_abils.cha++;
    mssg = "Your charisma improves!\r\n";
    break;
  default:
    send_to_char("Suddenly, a rift appears in the fabric of spacetime, then\r\n"
                 "snaps shut!  Report this incident using the bug command.\r\n", ch);
    return 1;
  }
  GET_LIFE_POINTS(ch) -= 1;
  send_to_char(mssg, ch);
  save_char(ch, NULL);
  if (GET_LIFE_POINTS(ch)) {
    sprintf(buf3, "You have %d life points left, %s.", GET_LIFE_POINTS(ch), GET_NAME(ch));
    perform_tell(impro, ch, buf3);
  }
  if (GET_LIFE_POINTS(ch) <= 0) {
    send_to_char("You have no more life points left.  From now on, you will gain life\r\n"
                 "points slowly as you increase in experience.\r\n", ch);
    return 1;
  } else
  return 1;
}

  
