//
// File: fire_breather.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fire_breather)
{

  if (cmd)
    return FALSE;
  if (spec_mode != SPECIAL_COMBAT ) return FALSE;
  if (ch->getPosition() != POS_FIGHTING || !FIGHTING(ch))
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
    !number(0, 4)) {
    if (mag_savingthrow(FIGHTING(ch), GET_LEVEL(ch), SAVING_BREATH))
      damage(ch, FIGHTING(ch), 0, SPELL_FIRE_BREATH, -1);
    else
      damage(ch, FIGHTING(ch), GET_LEVEL(ch) +number(8, 30), 
	     SPELL_FIRE_BREATH, -1);
    return TRUE;
  }
  return FALSE;
}
