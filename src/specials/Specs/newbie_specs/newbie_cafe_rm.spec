//
// File: newbie_cafe_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_cafe_rm)
{
  if( spec_mode != SPECIAL_CMD )
    return 0;

  if (IS_MOB(ch) && GET_MOB_VNUM(ch) == 2391)  /* janitor */
    return 0;

  if (CMD_IS("south")) {
    if ((GET_LEVEL(ch) > 5 || IS_REMORT(ch)) && GET_LEVEL(ch) < LVL_IMMORT) {
      send_to_char(ch, "You may no longer pass into this area.\r\n");
      send_to_char(ch, "If you you cannot find a way out of the cafe, type RETURN.\r\n");
      return 1;
    }
  } else if (CMD_IS("return")) {
    act("$n vanishes in a flash of light!", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You are teleported in a flash of light!\r\n\r\n");
    char_from_room(ch,false);
    char_to_room(ch, real_room(3013),false);
    act("$n appears at the center of the square with a flash!", FALSE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, ch->in_room, 0);
    return 1;
  }
  return 0;
}

