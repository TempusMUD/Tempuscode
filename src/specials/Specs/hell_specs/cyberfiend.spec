//
// File: cyberfiend.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cyberfiend)
{

  struct char_data *fiend = (struct char_data *) me;

  if (!cmd || !AWAKE(fiend))
    return 0;

  if (CMD_IS("enter") || CMD_IS("hop")) {

    act("$n levels you with a terrible blow to the head!!",
	FALSE, fiend, 0, ch, TO_VICT);
    act("$n levels $N with a terrible blow to the head!!",
	FALSE, fiend, 0, ch, TO_NOTVICT);

    GET_POS(ch) = POS_SITTING;
    WAIT_STATE(ch, 4 RL_SEC);
    return 1;
  }
  return 0;
}
