//
// File: safiir.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(safiir)
{
  ACMD(do_say);
  struct char_data *safiir = (struct char_data *) me;
  struct obj_data *wand;
  char buf3[MAX_STRING_LENGTH];
  int cost;
  if( spec_mode != SPECIAL_CMD ) return 0;
  if (!CMD_IS("recharge") && !CMD_IS("offer"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "What wand are you talking about?\r\n");
    return 1;
  } else if (!(wand = get_obj_in_list_vis(ch, argument, ch->carrying))) {
    sprintf(buf3, "You are not carrying any %s.", argument);
    do_say(safiir, buf3, 0, 0);
    return 1;
  } else if ((GET_OBJ_TYPE(wand) != ITEM_WAND) &&    
             (GET_OBJ_TYPE(wand) != ITEM_STAFF)) {
    do_say(safiir, "I do not deal with such things.", 0, 0);
    do_say(safiir, "Only wands and staves.", 0, 0);
    return 1;
  } else if (GET_OBJ_VAL(wand, 2) >= GET_OBJ_VAL(wand, 1)) {
    do_say(safiir, "That is already fully energized.", 0, 0);
    return 1;
  } else {
    cost = (GET_OBJ_VAL(wand, 1) - GET_OBJ_VAL(wand, 2));
    cost *= GET_OBJ_COST(wand)/GET_OBJ_VAL(wand, 1); 
    cost *= GET_OBJ_VAL(wand, 0);
    if (IS_MAGE(ch))
      cost >>= 3;
    else
      cost >>= 2;
  }

  if (CMD_IS("recharge")) { 
    sprintf(buf3, "Please recharge this %s.", argument);
    do_say(ch, buf3, 0, 0);
    if (GET_GOLD(ch) < cost) {
      sprintf(buf3, "Hah!  You cannot afford the %d coins i require!", cost);
      do_say(safiir, buf3, 0, 0);
      return 1;
    } else if (IS_MAGE(ch)) {
      sprintf(buf3, "I am happy to do business with you, %s.", PERS(ch, safiir));
      do_say(safiir, buf3, 0, 0); 
    } else {
      do_say(safiir, "Okay, but I usually only do business with mages...", 0, 0);
    }
    sprintf(buf3, "Here, I take %d of your gold coins.", cost);
    do_say(safiir, buf3, 0, 0);
    GET_GOLD(ch) -= cost;
    GET_OBJ_VAL(wand, 2) = GET_OBJ_VAL(wand, 1);
    GET_OBJ_VAL(wand, 1)--;
    act("$n passes $s hand over $p...", FALSE, safiir, wand, 0, TO_ROOM); 
    send_to_room("A bright light fills the room!\r\n", ch->in_room);
    return 1;
  } else if (CMD_IS("offer")) {
    sprintf(buf3, "How much to recharge this %s?", argument);
    do_say(ch, buf3, 0, 0);
    sprintf(buf3, "Hmmm... I think I could do it for %d coins.", cost);
    do_say(safiir, buf3, 0, 0);
    return 1;
  }
  return 0;
}
