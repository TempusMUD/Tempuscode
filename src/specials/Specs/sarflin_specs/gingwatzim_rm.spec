//
// File: gingwatzim_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(gingwatzim)
{
  if (cmd != 5) return 0;
  
  char_from_room(ch,false);
  char_to_room(ch,real_room(8343),false);
  look_at_room(ch,real_room(8343),0);
  return 1;
}   

