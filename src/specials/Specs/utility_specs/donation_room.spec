//
// File: donation_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(donation_room)
{
  if (!CMD_IS("get") && !CMD_IS("take"))
    return 0;

  if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
    send_to_char("Very funny, asshole.\r\n", ch);
    return 1;
  }

  skip_spaces(&argument);
  if (argument && !strn_cmp(argument, "all", 3)) {
    send_to_char("One at a time, please.\r\n", ch);
    return 1;
  }
  return 0;
}
