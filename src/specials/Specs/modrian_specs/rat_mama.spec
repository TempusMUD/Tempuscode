//
// File: rat_mama.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(rat_mama)
{
  struct char_data *rat;
  int i;
  int rat_rooms[] = {
         2940,
         2941,
         2942,
         2943,
         -1
  };

  if (cmd || FIGHTING(ch) || GET_POS(ch) == POS_FIGHTING)
    return (0);


  for (i = 0; i != -1; i++) 
  {
    for (rat = (real_room(rat_rooms[i]))->people; rat; rat = rat->next_in_room)
    {
      if (!number(0, 1) == 0 && IS_NPC(rat) && isname("rat", rat->player.name)) {
        act("$n climbs into a hole in the wall.", FALSE, rat, 0, 0, TO_ROOM);
        char_from_room(rat);
        extract_char(rat, FALSE);
        return TRUE;
      } 
    }
    if ((real_room(rat_rooms[i]))->people != NULL && !number(0, 1)) {
      rat = read_mobile(number(4206, 4208));
      char_to_room(rat, real_room(rat_rooms[i]));
      act("$n climbs out of a hole in the wall.", FALSE, rat, 0, 0, TO_ROOM);
      return TRUE;
    }
  }
  return FALSE;
}
