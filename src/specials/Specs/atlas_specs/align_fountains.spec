//
// File: align_fountains.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fountain_good)
{
  struct obj_data *obj = (struct obj_data *) me;
  // the variable (void * me) is passed to all SPECIAL funcs.  it points
  // to an obj, room, or char, depending on what called the func

  skip_spaces(&argument); // make sure they want to drink from 'me'.

  if (!CMD_IS("drink") || *argument || !isname(argument, obj->name))
    return 0;
  
  act("$n drinks from $p.", TRUE, ch, obj, 0, TO_ROOM);
  WAIT_STATE(ch, 2 RL_SEC);  // dont let them spam drink
  call_magic(ch, ch, 0, SPELL_ESSENCE_OF_GOOD, 25, CAST_SPELL); 
  // everything you need is handled in call_magic(), damage, etc...
  return 1;
}
SPECIAL(fountain_evil)
{

  struct obj_data *obj = (struct obj_data *) me;
  
  skip_spaces(&argument);
  if (!CMD_IS("drink") || *argument || !isname(argument, obj->name))
    return 0;
  
  if (GET_ALIGNMENT(ch) <= -1000) {
    send_to_char("You are already burning with evil.\r\n", ch);
    return 0;
  }
  
  act("$n drinks from $p.", TRUE, ch, obj, 0, TO_ROOM);
  WAIT_STATE(ch, 2 RL_SEC);
  call_magic(ch, ch, 0, SPELL_ESSENCE_OF_EVIL, 25, CAST_SPELL);

  return 1;
}                                                       







