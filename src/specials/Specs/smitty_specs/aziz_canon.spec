//
// File: aziz_canon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(aziz_canon)
{
  struct char_data *vict;

  if (cmd)
    return 0;
  if (!FIGHTING(ch) && GET_POS(ch) != POS_FIGHTING) {
    for (vict = ch->in_room->people;vict;vict = vict->next_in_room) {
      if (!number(0, 2))
        break;
    }
    if (!vict)
      return 0;

    switch (number(0, 30)) {
    case 1:
      act("$n looks at you and winks.'.",FALSE,ch,0,vict,TO_VICT);
      act("$n looks at $N and winks.'.",FALSE,ch,0,vict,TO_NOTVICT);	
      break;
    case 2:
      act("$n struts around you like a horny rooster.",TRUE,ch,0,vict,TO_VICT);
      act("$n struts around $N like a horny rooster.",TRUE,ch,0,vict,TO_NOTVICT);
      break;
    case 3:
      act("$n farts loudly and smiles sheepishly.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 4:
      act("$n pounds his shoulder against the wall.",TRUE,ch,0,0,TO_ROOM);
      break;
    case 5:
      act("$n eats a small piece of paper.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 6:
      act("$n rolls a joint and licks it real good.",TRUE,ch,0,0,TO_ROOM);
      break;
    case 7:
      act("$n guzzles a beer and smashes the can against $s forehead.",TRUE,ch,0,0,TO_ROOM);
      break;
    }
    return 1;
  }
  if (FIGHTING(ch)) {
    if (!number(0, 3)) {
      act("$n gets in a three point stance and plows his shoulder into you!",FALSE,ch,0,FIGHTING(ch),TO_VICT);
      act("$n gets in a three point stance and plows his shoulder into $N!",FALSE,ch,0,FIGHTING(ch),TO_NOTVICT);
      if (GET_LEVEL(FIGHTING(ch)) < LVL_IMMORT) {
        GET_HIT(FIGHTING(ch))-=dice (1,20);
        GET_POS(FIGHTING(ch)) = POS_SITTING;
        WAIT_STATE(ch, PULSE_VIOLENCE*4);
        WAIT_STATE(FIGHTING(ch), PULSE_VIOLENCE*2);
      }
      return 1;
    }
  }
  return 0;
}
