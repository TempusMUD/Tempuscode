//
// File: ogre1.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(ogre1)
{
  struct char_data *vict;
  if (cmd || FIGHTING(ch))
    return 0;
  for (vict = ch->in_room->people;vict;vict = vict->next_in_room)
    if (IS_ORC(vict) && CAN_SEE(ch, vict)) {
      act("$n roars, 'Now I've got $N, you!", FALSE, ch, 0, vict, TO_ROOM);
      hit(ch, vict, TYPE_UNDEFINED);
      return 1;
    }
  return 0;
}
