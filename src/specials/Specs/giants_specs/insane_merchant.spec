//
// File: insane_merchant.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(insane_merchant)
{
  if (cmd)
    return 0;

  if (ch->in_room->people == ch && !ch->next_in_room)
    return 0;

  switch (number(0, 50)) {
  case 0:
    act("$n laughs maniacally, chilling you to the bone.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 1:
    act("$n begins to giggle uncontrollably.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 2:
    act("$n begins to sob loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 3:
    act("$n laments the destruction of the fine city Istan.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  default:
    return 0;
  }
  return 1;
}
