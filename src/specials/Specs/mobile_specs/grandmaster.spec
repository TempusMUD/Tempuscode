//
// File: grandmaster.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(grandmaster)
{
  if (cmd || !AWAKE(ch)) {
    return 0;
  }

  if (!FIGHTING(ch)) {
    switch (number(0, 25)) {
    case 0:
      act("$n performs a spinning jump kick.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You jump kick.\r\n", ch);
      break;
    case 1:
      act("$n bows to you.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You bow.\r\n", ch);
      break;
    case 2:
      act("$n shatters a thick board with $s fist.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You break  a board.\r\n", ch);
      break;
    case 3:
      act("$n performs a back flip and kicks a punching bag.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Back flip, punch.\r\n", ch);
      break;
    case 4:
      act("$n dusts off an old trophy.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You dust trophy.\r\n", ch);    
      break;
    default:
      return 0;
    }
    return 1;
  } 

  switch(number(0, 12)) {
  case 0:
    act("$N whacks you with a spinning backfist!", FALSE, FIGHTING(ch), 0, ch, TO_CHAR); 
    act("$N whacks $n with a spinning backfist!", FALSE, FIGHTING(ch), 0, ch, TO_ROOM); 
    break;
  case 1:
    act("$N leaps through the air, kicking you in the head!", FALSE, 
                                        FIGHTING(ch), 0, ch, TO_CHAR);
    act("$N leaps through the air, kicking $n in the head!", FALSE, 
                                       FIGHTING(ch), 0, ch, TO_ROOM);
    break;
  case 2:
    if (GET_POS(FIGHTING(ch)) == POS_FIGHTING) {
      act("$N trips you, sending you staggering!", FALSE, FIGHTING(ch),
                                        0, ch, TO_CHAR);
      act("$N trips $n, who staggers across the room!", TRUE, FIGHTING(ch),
                                        0, ch, TO_ROOM);
      GET_POS(FIGHTING(ch)) = POS_SITTING;
    }
    break;
  case 3:
    act("$N drops into a split, dodging your attack.", TRUE, FIGHTING(ch),
			  0, ch, TO_CHAR);
    act("$N drops into a split, dodging $n's attack.", TRUE, FIGHTING(ch),
			  0, ch, TO_ROOM);
    break;
  case 4:
    act("$N punches you in the throat!", FALSE, FIGHTING(ch), 0, ch, TO_CHAR);
    act("$N punches $n in the throat!", FALSE, FIGHTING(ch), 0, ch, TO_ROOM);
    break;
  default:
    return 0;
  }
  return 1;
}

