//
// File: shimmering_portal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(shimmering_portal)
{
  if (!CMD_IS("enter"))
    return FALSE;
  
  skip_spaces(&argument);
  
  if (!*argument) {
    send_to_char("Enter what?\r\n", ch);
    return TRUE;
  }
  
  if (!(strncasecmp(argument, "portal", 6))) {
    send_to_char("You step into the portal...\r\n\r\n", ch);
    act("$n steps into the shimmering portal and vanishes!", TRUE, ch, 0, 0, TO_ROOM);
    if (ch->in_room == real_room(3012)) {
      char_from_room(ch);
      char_to_room(ch, real_room(30012));
    } else if (ch->in_room == real_room(30012)) {
      char_from_room(ch);
      char_to_room(ch, real_room(3012));
    } else {
      send_to_char("You are ejected from the portal with a violent jerk!\r\n", ch);
      act("$n is ejected from the portal with a violent jerk!", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    act("$n steps out of the shimmering portal.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("For a few brief moments you feel completely disoriented.  You float\r\n"
                 "in a timeless void, without obvious form or direction.  You see a\r\n"
                 "glowing disk of light, and feel yourself drawn toward it!  You approach\r\n"
                 "it and step through a shimmering curtain of light...\r\n\r\n", ch);
    look_at_room(ch, ch->in_room, 0);
    ch->in_room->zone->enter_count++;
    return TRUE;
  } else
  send_to_char("Enter what?\r\n", ch);
  return TRUE;
}
  
    

