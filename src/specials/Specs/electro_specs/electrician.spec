//
// File: electrician.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_flee);

SPECIAL(electrician)
{
  if (cmd || FIGHTING(ch) || !AWAKE(ch) || number(0, 150))
    return 0;

  switch (number(0, 5)) {
  case 0:
  case 4:
    act("$n shocks the hell out of $mself!!", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 1:
    act("$n fumbles and drops a screwdriver.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 2:
    act("$n jabs $s finger with a wire.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 3:
    act("$n's hair suddenly stands on end.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 5:
    do_say(ch, "Uh-oh.", 0, 0);
    do_flee(ch, "", 0, 0);
    break;
  }
  return 1;
}

    
