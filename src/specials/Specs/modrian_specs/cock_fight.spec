//
// File: cock_fight.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cock_fight)
{
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
  if (cmd)
    return 0;

  if (!FIGHTING(ch)) {
    switch (number(0, 40)) {
    case 0:
      act("$n scratches the ground with $s claw.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You scratch.\r\n", ch);
      return 1;
    case 1:
      act("$n squawks loudly!", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You squawk!\r\n", ch);
      return 1;
    case 2:
      act("$n struts around proudly.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You strut.\r\n", ch);
      return 1;
    default:
      return 0;
    }
  }
  switch (number(0, 16)) {
  case 0:
    act("$n leaps into the air, feathers flying!", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You leap.\r\n", ch);
    return 1;
  case 1:
    send_to_room("Feathers fly everywhere!\r\n", ch->in_room);
    return 1;
  case 2:
    act("$N screams as $E attacks you!", FALSE, FIGHTING(ch), 0, ch, TO_CHAR);
    act("$N screams as $E attacks $n!", FALSE, FIGHTING(ch), 0, ch, TO_ROOM);
    return 1;
  default:
    return 0;
  } 
  return 0;
}
