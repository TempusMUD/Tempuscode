//
// File: archon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(archon)
{

  struct room_data *room = real_room(43252);

  if (cmd)
    return 0;
  if( spec_mode == SPECIAL_DEATH ) return 0;
  if (!FIGHTING(ch) && ch->in_room->zone->plane != PLANE_HEAVEN) {
    CharacterList::iterator it = ch->in_room->people.begin();
    for( ; it != ch->in_room->people.end(); ++it ) 
      if ((*it) != ch && IS_ARCHON((*it)) && FIGHTING((*it))) {
        do_rescue(ch, fname((*it)->player.name), 0, 0);
        return 1;
      }

    act("$n disappears in a flash of light.",FALSE,ch,0,0,TO_ROOM);
    if (room) {
      char_from_room(ch);
      char_to_room(ch, room);
      act("$n appears at the center of the room.",FALSE,ch,0,0,TO_ROOM);
    } else {
        ch->extract(true, false, CON_MENU);
    }
    return 1;
  }
  return 0;
}
