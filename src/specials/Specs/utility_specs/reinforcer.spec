//
// File: reinforcer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define REIN_BUY 1
#define REIN_OFF 2

SPECIAL(reinforcer)
{

  struct char_data *keeper = (struct char_data *) me;
  struct obj_data *obj = NULL;
  char *args = NULL, arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int cmd_type = 0, cost = 0;

  if (!cmd || (!CMD_IS("buy") && !CMD_IS("offer")))
    return 0;
  
  if (CMD_IS("buy"))
    cmd_type = REIN_BUY;
  else
    cmd_type = REIN_OFF;

  args = two_arguments(argument, arg1, arg2);

  if (!*arg1 || strncmp(arg1, "reinforcement", 13)) {
    perform_tell(keeper, ch, "Usage: buy/offer reinforcement <item>");
    return 1;
  }

  if (!*arg2) {
    sprintf(buf2, "%s what item?", cmd_type == REIN_BUY ?
	    "Perform structural reinforcement on" : 
	    "Get an offer on structural reinforcement for");
    perform_tell(keeper, ch, buf2);
    return 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
    sprintf(buf2, "You don't have %s '%s'.", AN(arg2), arg2);
    perform_tell(keeper, ch, buf2);
    return 1;
  }

  if (OBJ_REINFORCED(obj)) {
    perform_tell(keeper, ch, "This item has already been structurally reinforced.");
    perform_tell(keeper, ch, "I cannot reinforce it further.");
    return 1;
  }

  cost = GET_OBJ_COST(obj) >> 1;

  sprintf(buf2, "It will cost you %d %s to have %s reinforced.",
	  cost, ch->in_room->zone->time_frame == TIME_ELECTRO ? "credits" :
	  "coins", obj->short_description);
  perform_tell(keeper, ch, buf2);

  if (cmd_type == REIN_OFF) {
    act("$n gets an offer on reinforcement for $p.",FALSE,ch,obj,0,TO_ROOM);
    return 1;
  }

  if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
    if (GET_CASH(ch) < cost) {
      perform_tell(keeper, ch, "You don't have enough credits!");
      return 1;
    } else
      GET_CASH(ch) -= cost;
  } else {
    if (GET_GOLD(ch) < cost) {
      perform_tell(keeper, ch, "You don't have enough coins!");
      return 1;
    } else
      GET_GOLD(ch) -= cost;
  }

  act("$n takes $p and disappears into the back room for a while.\r\n",
      FALSE, keeper, obj, ch, TO_VICT);
  act("$e returns shortly and presents you with the finished product",
      FALSE, keeper, obj, ch, TO_VICT);
  act("$n takes $p from $N and disappears into the back room for a while.",
      FALSE, keeper, obj, ch, TO_NOTVICT);

  SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_REINFORCED);
  GET_OBJ_MAX_DAM(obj) += (GET_OBJ_MAX_DAM(obj) >> 2);
  GET_OBJ_DAM(obj) += (GET_OBJ_DAM(obj) >> 2);
  GET_OBJ_WEIGHT(obj)++;
  WAIT_STATE(ch, 5 RL_SEC);
  save_char(ch, NULL);

  return 1;
}
