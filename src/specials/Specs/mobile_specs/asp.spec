//
// File: asp.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(asp)
{
  if (cmd)
    return FALSE;

  if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) 
    return FALSE;
  if (ch->getPosition() != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 57 - GET_LEVEL(ch)) == 0)) {
      act("$n bites $N with deadly fangs!", 1, ch, 0, FIGHTING(ch), 
      TO_NOTVICT);

    act("$n bites you with its fangs!", 1, ch, 0, FIGHTING(ch), TO_VICT);
       call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), 
       CAST_SPELL);

    return TRUE;
  }
  return FALSE;
}
