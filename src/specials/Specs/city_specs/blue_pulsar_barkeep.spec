//
// File: blue_pulsar_barkeep.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(blue_pulsar)
{

  struct char_data *bartender  = (struct char_data *) me;
  struct obj_data *beer = NULL;
  int beer_vnum = (number(0, 1) ? 50300 : 50301);

  if (!CMD_IS("yell"))
    return 0;
  if( spec_mode == SPECIAL_DEATH ) return 0;
  skip_spaces(&argument);

  if (!*argument || str_cmp(argument, "beer"))
    return 0;

  if (!(beer = read_object(beer_vnum)))
    slog("SYSERR: Blue Pulsar beer not in database.");
  else {
    act("$N yells, 'BEER!!!!'", FALSE, ch, 0, 0, TO_NOTVICT);
    act("$n throws $p across the room to $N!", FALSE, bartender, beer, ch, TO_NOTVICT);
    act("$n throws $p to you.", FALSE, bartender, beer, ch, TO_VICT);
    return 1;
  }
  return 0;
}
  
  
  
  
