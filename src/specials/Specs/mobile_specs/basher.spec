//
// File: basher.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(basher)
{
  struct char_data *vict;
  ACMD(do_bash);

  if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
    return 0;

  if (number(0, 81) > GET_LEVEL(ch))
    return 0;

  /* pseudo-randomly choose a mage in the room who is fighting me */
  for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 2) && IS_MAGE(vict))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  do_bash(ch, "", 0, 0);
  return 1;
}

