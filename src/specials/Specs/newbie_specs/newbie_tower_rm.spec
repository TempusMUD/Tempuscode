//
// File: newbie_tower_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_tower_rm)
{
  ACMD(do_help);
  one_argument(argument, arg);

  if (!CMD_IS("look") && !CMD_IS("examine"))
    return 0;

  if (CMD_IS("look")) {
    if (!strncasecmp(arg, "plate", 5) ||
        !strncasecmp(arg, "at plate", 8) ||
        !strncasecmp(arg, "map", 3) ||
        !strncasecmp(arg, "at map", 6)) {
      do_help(ch, "modrian", 0, 0);
      /*
      send_to_char("This map may be viewed at any time by typing 'help modrian'.\r\n", ch);
      send_to_char("You may also look out the windows of the tower by using the\r\n"
                   "look command to look in the cardinal directions.  For example,\r\n"
                   "'look north'.\r\n", ch);
      */
      return 1;
    }
  }
  if (CMD_IS("examine")) {
    if (!strncasecmp(arg, "plate", 5)) {
      do_help(ch, "modrian", 0, 0);
      return 1;
    }
  }
  return 0;
}
