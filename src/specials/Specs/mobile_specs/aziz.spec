//
// File: aziz.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_bash);

SPECIAL(Aziz)
{
  struct char_data *vict;

  if (cmd || ch->getPosition() != POS_FIGHTING)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  /* pseudo-randomly choose a mage in the room who is fighting me */
  for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 2) && IS_MAGE(vict))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  do_bash(ch, "", 0, 0);

  return 0;
}

