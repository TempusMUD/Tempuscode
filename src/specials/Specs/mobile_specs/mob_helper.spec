//
// File: mob_helper.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(mob_helper)
{
  struct char_data *vict = NULL;

  if (cmd || FIGHTING(ch))
    return 0;
  
  for (vict=ch->in_room->people;vict != NULL;vict=vict->next_in_room) {
    if (FIGHTING(vict) && IS_MOB(vict) && IS_MOB(FIGHTING(vict)))
      if (((IS_GOOD(ch) && IS_GOOD(vict)) || (IS_EVIL(ch) && IS_EVIL(vict)))
                   && !number(0, 2)) {
	act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_NOTVICT);
 	hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
	return 1;
      }
  }
  return 0;
}

    




