//
// File: physic_guildguard.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(physic_guildguard)
{
  if (!CMD_IS("south") || IS_PHYSIC(ch) || GET_LEVEL(ch) > LVL_GRGOD)
    return FALSE;

  send_to_char(ch, "You are repulsed by a strong, invisible force field!\r\n");
  act("$n tries to enter the Physics guild, but is shoved back!", TRUE, ch, 0, 0, TO_ROOM);
  return TRUE;
}
