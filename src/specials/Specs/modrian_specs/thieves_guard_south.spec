//
// File: thieves_guard_south.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(thieves_guard_south)
{
  struct char_data *guard = (struct char_data *) me;
  if( spec_mode == SPECIAL_DEATH ) return 0;
  if (cmd != SOUTH + 1 && !CMD_IS("unlock") && !CMD_IS("pick")) 
    return FALSE;

  if (!CAN_SEE(guard, ch) || !AWAKE(guard) || GET_LEVEL(ch) >= LVL_IMMORT)
    return 0;

  skip_spaces(&argument);

  if (cmd == SOUTH + 1) { 
    act("$N snickers at $n and pushes $m back.", TRUE, ch, 0, guard, TO_ROOM);
    act("$N snickers at you and pushes you back.", TRUE, ch, 0, guard, TO_CHAR);
    return TRUE;
  }
  if (CMD_IS("unlock")) {
    if (strncasecmp(argument, "door", 4))
      return 0;
    act("$N says, 'Scram, $n!'", TRUE, ch, 0, guard, TO_ROOM);
    act("$N says, 'Scram, $n!'", TRUE, ch, 0, guard, TO_CHAR);
    return 1;
  }
  if (CMD_IS("pick")) {
    if (strncasecmp(argument, "door", 4))
      return 0;
    act("$N screams, 'So you think yer a wiseguy, $n!?'", TRUE, ch, 0, guard, TO_ROOM);
    act("$N screams, 'So you think yer a wiseguy, $n!?'", TRUE, ch, 0, guard, TO_CHAR);
    damage(guard, ch, 4, SKILL_PUNCH, -1);
    return 1;
  }
  send_to_char("You get a prickly feeling on the back of your neck.\r\n", ch);
  return FALSE;
}

