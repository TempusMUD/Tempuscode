//
// File: sunstation.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(sunstation)
{
  struct obj_data *sun = (struct obj_data *) me;
  
  if (!ch->desc || !CMD_IS("login"))
    return 0;
  if (!IS_CYBORG(ch)) {
    send_to_char("You seem to have forgotten your password...\r\n", ch);
    return 1;
  }

  act("You log in on $p.", FALSE, ch, sun, 0, TO_CHAR);
  act("$n logs in on $p.", FALSE, ch, sun, 0, TO_ROOM);
  send_to_char("The screen flashes and changes color.\r\n", ch);
  send_to_char("The escape character is '@'.\r\n", ch);
  show_net_menu1_to_descriptor(ch->desc);
  STATE(ch->desc) = CON_NET_MENU1;
  return 1;
}
