//
// File: entrance_to_brawling.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(entrance_to_brawling)
{
  if (!CMD_IS("north"))
    return 0;
  if (IS_NPC(ch))
    return 0;
  if (GET_LEVEL(ch) < 5) {
    switch (number(0, 7)) {
    case 0:
      send_to_char("A drunk stumbles into you and drools in your hair.\r\n", ch);
      break;
    case 1:
      send_to_char("An old timer says, 'You better stay out of there.'\r\n", ch);
      act("An old timer tells $n something.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      send_to_char("As you try to enter, a huge man is thrown through, knocking you down.\r\n", ch);
      act("$n tries to go north, but is knocked down.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
    case 3:
      send_to_char("A rabid whore tells you, 'You better go pump some iron first!!'\r\n", ch);
      break;
    case 4:
      send_to_char("You notice some people laughing at you, and change your mind.\r\n", ch);
      act("Some people in the corner start laughing at $n.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 5:
      send_to_char("You slip on a greasy spot on the floor!\r\n", ch);
      act("$n slips down on a greasy spot on the floor.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
    case 6:
      send_to_char("Some fool by the door grabs a fire extinguisher.\r\n", ch);
      send_to_char("Someone sets off the fire extinguisher, filling the room with a fine dust!\r\n", ch);
      break;
    case 7:
      send_to_char("You trip over your shoelaces before you get to the door.\r\n", ch);
      act("$n trips over $s shoelaces and falls down.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
    }
    return 1;
  }
  return 0;
}
