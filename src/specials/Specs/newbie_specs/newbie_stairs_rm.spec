//
// File: newbie_stairs_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_stairs_rm)
{
  if (CMD_IS("up") && !IS_NPC(ch) && (GET_LEVEL(ch) > 5 || IS_REMORT(ch)) &&
      GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You no longer need to enter here.  Go out into the\r\n"
                 "world and find fame and fortune for yourself.\r\n");
    return 1;
  }
  return 0;
}
