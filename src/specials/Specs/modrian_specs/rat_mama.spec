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
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
  if (cmd || FIGHTING(ch) || ch->getPosition() == POS_FIGHTING)
    return (0);

  CharacterList::iterator it;
  for (i = 0; i != -1; i++) 
  {
    it = (real_room(rat_rooms[i]))->people.begin();
    for( ; it != (real_room(rat_rooms[i]))->people.end(); ++it ) {
      if (!number(0, 1) == 0 && IS_NPC((*it)) && isname("rat", (*it)->player.name)) {
        act("$n climbs into a hole in the wall.", FALSE, (*it), 0, 0, TO_ROOM);
        char_from_room((*it));
        (*it)->extract(true, false, CON_MENU);
        return TRUE;
      } 
    }
    if (*((real_room(rat_rooms[i]))->people.begin()) != NULL && !number(0, 1)) {
      rat = read_mobile(number(4206, 4208));
      char_to_room(rat, real_room(rat_rooms[i]));
      act("$n climbs out of a hole in the wall.", FALSE, rat, 0, 0, TO_ROOM);
      return TRUE;
    }
  }
  return FALSE;
}
