//
// File: geryon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(geryon)
{
  struct char_data *vict = NULL;
  struct obj_data *horn = NULL;
  ACMD(do_order);

  if (cmd || !FIGHTING(ch))
    return 0;

  if ((!ch->followers || !ch->followers->next) && (horn = GET_EQ(ch, WEAR_HOLD)) &&
              GET_OBJ_VNUM(horn) == 16144 && GET_OBJ_VAL(horn, 0)) {
    command_interpreter(ch, "wind horn");
    do_order(ch, "minotaur assist geryon", 0, 0);
    return 1;
  } else if (number(0, 2))
    return 0;

  for (vict=ch->in_room->people;vict && vict!=ch;vict=vict->next_in_room)
    if (ch == FIGHTING(vict) && !number(0, 4) && 
                                     !affected_by_spell(vict, SPELL_POISON))
      break;

  if (!vict || !number(0, 3) || vict == ch)
    vict = FIGHTING(ch);

  act("$n stings you with a mighty lash of $s deadly tail!", FALSE, ch, 0, vict, TO_VICT);
  act("$n stings $N with a mighty lash of $s deadly tail!", FALSE, ch, 0, vict, TO_NOTVICT);
  GET_HIT(vict) -= dice(2, 6);
  call_magic(ch, vict, 0, SPELL_POISON, GET_LEVEL(ch), CAST_POTION);
  return 1;
}
