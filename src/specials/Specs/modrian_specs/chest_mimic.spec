//
// File: chest_mimic.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(chest_mimic)
{
  struct char_data *mimic = (struct char_data *) me;
  skip_spaces(&argument);
  
  if( spec_mode == SPECIAL_DEATH ) return 0;
  if (!(CMD_IS("open") || CMD_IS("pick") || CMD_IS("unlock") ||
        CMD_IS("look") || CMD_IS("examine"))) 
    return 0;

  if (!*argument)  
    return 0;

  if (!strncasecmp(argument, "in chest", 8)) {
    send_to_char("It seems to be closed.", ch);
    act("$n looks at the bulging chest by the wall.", TRUE, ch, 0, 0, TO_ROOM);
    return 1;
  }

  if (strncasecmp(argument, "chest", 5) && 
      strncasecmp(argument, "at bulging", 10) &&
      strncasecmp(argument, "at chest", 8) &&
      strncasecmp(argument, "at mimic", 8) &&
      strncasecmp(argument, "bulging", 7) &&
      strncasecmp(argument, "mimic", 5)) {
    return 0;
  }

  if (ch->getPosition() == POS_FIGHTING)  
    return 0;
  if (CMD_IS("open") || CMD_IS("pick") || CMD_IS("unlock")) {
    if (strncasecmp(argument, "chest", 5) && 
        strncasecmp(argument, "bulging", 7) &&
        strncasecmp(argument, "mimic", 5)) {
      return 0;
    }
    send_to_char("The chest lurches forward suddenly!  It has sharp teeth!!\r\n", ch);
    act("$n attempts to open the bulging chest.", FALSE, ch, 0, 0, TO_ROOM);
    act("The chest lurches forward suddenly and snaps at $n!", FALSE, ch, 0, 0, TO_ROOM);
    hit(mimic, ch, TYPE_BITE);
    return TRUE;

  } else if (CMD_IS("look") || CMD_IS("examine")) {
    send_to_char("The chest appears to be full of jewels and precious metals.\r\n", ch);
    act("$n looks at the bulging chest by the wall.", TRUE, ch, 0, 0, TO_ROOM);
    return TRUE;
  } else return FALSE;
}

