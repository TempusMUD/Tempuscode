//
// File: mavernal_citizen.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(maveral_citizen)
{
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
  if (cmd || FIGHTING(ch) || !AWAKE(ch))
    return (0);

  switch (number(0, 40)) {
  case 0:
    do_say(ch, "Welcome to the magical village of Mavernal!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "Hello there adventurer.", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I heard there's a sale on spells at Dooley's today!", 0, 0);
    return (1);
  case 3:
    do_say(ch, "It's a beautiful day out isn't it?", 0, 0);
    return (1);
  default:
    return (0);
  }
  return (0);
}


