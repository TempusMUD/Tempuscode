//
// File: fountain_heal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fountain_heal)
{
  byte num;
  struct obj_data *fountain = (struct obj_data *) me;

  if (!CMD_IS("drink"))
    return 0;
  
  if ((IS_OBJ_STAT(fountain, ITEM_BLESS | ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
      (IS_OBJ_STAT(fountain, ITEM_EVIL_BLESS | ITEM_ANTI_GOOD) && IS_GOOD(ch)))
    return 0;

  skip_spaces(&argument);

  if (!*argument) 
    return 0;
    
  if (!isname(argument, fountain->name))
    return 0;

  if (GET_HIT(ch) < GET_MAX_HIT(ch))
    send_to_char("You drink from the fountain, it tastes oddly refreshing!\r\n", ch);
  act("$n drinks from $p.", TRUE, ch, fountain, 0, TO_ROOM);

  num= number(1,3);
  GET_HIT(ch) = MIN(GET_HIT(ch)+num,GET_MAX_HIT(ch));
  return 1;
}

