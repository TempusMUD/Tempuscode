//
// File: life_bldg_guard.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(life_bldg_guard)
{
  struct char_data *guard = (struct char_data *) me;
  if( spec_mode == SPECIAL_DEATH ) return 0;
  if (cmd != EAST + 1 || GET_LEVEL(ch) > 25)
    return FALSE;

  if (!CAN_SEE(guard, ch) || !AWAKE(guard))
    return 0;

  act("$N snickers at $n and pushes $m back.", TRUE, ch, 0, guard, TO_ROOM);
  act("$N snickers at you and pushes you back.", TRUE, ch, 0, guard, TO_CHAR);
  return TRUE;
} 

