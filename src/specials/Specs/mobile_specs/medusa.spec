//
// File: medusa.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(medusa)
{
  if( spec_mode != SPECIAL_COMBAT ) return 0;
  if (cmd || ch->getPosition() != POS_FIGHTING)
    return FALSE;

  if (isname("medusa", ch->player.name) &&
      FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 57 - GET_LEVEL(ch)) == 0)) {
      act("The snakes on $n bite $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
      act("You are bitten by the snakes on $n!", 1, ch, 0, FIGHTING(ch), 
      TO_VICT);
      call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), 
      CAST_SPELL);

    return TRUE;
  } else if (FIGHTING(ch) && !number(0, 4)) {
      act("$n gazes into your eyes!", FALSE, ch, 0, FIGHTING(ch), TO_VICT);
      act("$n gazes into $N's eyes!", FALSE, ch, 0, FIGHTING(ch), TO_NOTVICT);
      call_magic(ch, FIGHTING(ch), 0, SPELL_PETRIFY,GET_LEVEL(ch),CAST_PETRI);
      if (IS_AFFECTED_2(FIGHTING(ch), AFF2_PETRIFIED))
        stop_fighting(ch);
      return 1;
  }
  return FALSE;
}
