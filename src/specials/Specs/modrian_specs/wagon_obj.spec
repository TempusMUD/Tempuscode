//
// File: wagon_obj.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(wagon_obj)
{
  struct room_data *wagon_room_rnum;
  wagon_room_rnum = real_room(10);

  if (!cmd)
    if (!number(0, 6))
      act("One of the wagon horses whinnies and stamps its foot.", FALSE, 0, 0, 0, TO_ROOM);

  if (!CMD_IS("board") && !CMD_IS("enter"))
    return 0;
  
  skip_spaces(&argument);

  if (!*argument) 
    return 0;
    
  if (strncasecmp(argument, "wagon", 5))
    return 0;

  send_to_char("You deftly leap aboard the wagon.\r\n\r\n", ch);
  act("$n deftly leaps aboard the wagon.", TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, wagon_room_rnum);
  look_at_room(ch, ch->in_room, 1);

  act("$n has climbed onto the wagon.", TRUE, ch, 0, 0, TO_ROOM);
  
  return 1;
}


