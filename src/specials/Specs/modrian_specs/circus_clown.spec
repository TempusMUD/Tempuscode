//
// File: circus_clown.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(circus_clown)
{
  if (!cmd && !FIGHTING(ch) && ch->getPosition() != POS_FIGHTING) {
    switch (number(0, 30)) {
    case 0:
      act("$n flips head over heels.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You flip head over heels.\r\n", ch);
      break;
    case 1:
      act("$n grabs your nose and screams 'HONK!'.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You grab everyone's honker!\r\n", ch);
      break;
    case 2:
      act("$n rolls around you like a cartwheel.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You cartwheel around.\r\n", ch);
      break;
    case 3:
      act("$n starts walking around on $s hands.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You walk around on your hands.\r\n",ch);
      break;
    case 4:
      act("$n trips on $s shoelaces and falls flat on $s face!", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You trip and fall.\r\n", ch);
      break;
    case 5:
      act("$n eats a small piece of paper.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You hit some acid.\r\n", ch);
      break;
    case 6:
      act("$n rolls a joint and licks it enthusiastically.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You roll a joint.\r\n", ch);
      break;
    case 7:
      act("$n guzzles a beer and smashes the can against $s forehead.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You guzzle a beer and smash the can on you forehead.\r\n", ch);
      break;
    }
    return 1;
  }
  return 0;
}
