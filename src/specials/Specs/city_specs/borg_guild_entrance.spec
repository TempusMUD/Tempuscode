//
// File: borg_guild_entrance.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(borg_guild_entrance)
{

  if (!CMD_IS("south"))
    return 0;

  if (!IS_CYBORG(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("The panel lights up: WARNING.  Retinal scan negative.  Access denied.", ch);
    act("The door panel flashes a warning as $n tries to enter.",
	FALSE, ch, 0, 0, TO_ROOM);
    return 1;
  }
  
  return 0;
}
  
