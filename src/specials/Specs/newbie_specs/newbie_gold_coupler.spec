//
// File: newbie_gold_coupler.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_gold_coupler)
{
  struct char_data *coup = (struct char_data *) me;
  extern struct time_info_data time_info;
  struct obj_data *o, *tmp_o, *money;
  int i, count = 0;

  if( spec_mode != SPECIAL_CMD ) return 0;
  if ((cmd && !CMD_IS("clear")) || (!cmd && time_info.hours % 3))
    return 0;

  for (i = 2351; i < 2379; i++) {
    if ((real_room(i)) != NULL && (real_room(i))->contents) {
      for (o = (real_room(i))->contents;o;o = o) {
        if (GET_OBJ_TYPE(o) == ITEM_MONEY && GET_OBJ_VAL(o, 0) &&
                                                 GET_OBJ_VAL(o, 0) < 80) {
          count += GET_OBJ_VAL(o, 0);
          tmp_o = o;
          o = o->next_content;
          extract_obj(tmp_o);
        } else
          o = o->next_content;
      }
      if (count > 80) {
        money = create_money(count, 0);
        obj_to_room(money, (real_room(i)));
        if ((real_room(i))->people) {
          sprintf(buf, "%s appears from a cloud of colored smoke!\r\n"
            "%s gathers up many scattered coins of gold.\r\n"
            "%s drops %s and disappears suddenly!\r\n", GET_NAME(coup), 
             GET_NAME(coup), GET_NAME(coup), money->short_description);
          send_to_room(buf, (real_room(i)));
        } 
        if (coup->in_room->people) {
          sprintf(buf, "%s goes to room %d, compiles %d coins.\r\n", GET_NAME(coup), i, count);
          send_to_room(buf, coup->in_room);
        }
        count = 0;
	continue;
      } else if (count) {
        if ((real_room(i))->people) {
          sprintf(buf, "%s appears from a cloud of colored smoke!\r\n"
            "%s gathers up many scattered coins of gold and disappears!.\r\n",
             CAP(GET_NAME(coup)), CAP(GET_NAME(coup)));
          send_to_room(buf, (real_room(i)));
        } 
        if (coup->in_room->people) {
          sprintf(buf, "%s goes to room %d, removes %d coins.\r\n", CAP(GET_NAME(coup)), i, count);
          send_to_room(buf, coup->in_room);
        }
      }
    }
  }
  return 1;
}
