//
// File: newbie_fly.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_fly)
{
  struct char_data *vict;

  if (cmd || FIGHTING(ch))
    return 0;

  for (vict = ch->in_room->people;vict;vict = vict->next_in_room) {
    if (IS_AFFECTED(vict, AFF_INFLIGHT) || !CAN_SEE(ch, vict))
      continue;
    cast_spell(ch, vict, 0, SPELL_FLY);
    return 1;
  }
  return 0;
}
