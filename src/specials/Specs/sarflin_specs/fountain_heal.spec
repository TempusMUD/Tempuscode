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

  if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
    act("You drink from $p, it tastes oddly refreshing!", 
        TRUE, ch, fountain, 0, TO_CHAR);
      num= number(4,12);
      WAIT_STATE(ch,1/2 RL_SEC);
      GET_HIT(ch) = MIN(GET_HIT(ch)+num,GET_MAX_HIT(ch));
  } else {
    act("You drink from $p.", 
        TRUE, ch, fountain, 0, TO_CHAR);
  }
  act("$n drinks from $p.", TRUE, ch, fountain, 0, TO_ROOM);

  return 1;
}

