//
// File: jail_locker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(jail_locker)
{
  struct Creature *atten = (struct Creature *) me;
  struct obj_data *locker, *item, *tmp_item;
  int cost, v_locker;
  ACMD(do_say);

  if (!cmd)
    return 0;

  if (ch->in_room->number == 3100)
    v_locker = 3178;
  else if (ch->in_room->number == 1214)
    v_locker = 1589;
  else {
    send_to_char(ch, "There is an error here.\r\n");
    return 0;
  }

  if (CMD_IS("receive")) {
    do_say(ch, "I'd like to get my stuff back, please.", 0, 0);
    if (IS_NPC(ch)) {
      do_say(atten, "Sorry, I don't deal with mobiles.", 0, 0);
      return 1;
    }
    cost = GET_LEVEL(ch) * 1000;
    if (GET_GOLD(ch) < cost && GET_LEVEL(ch) < LVL_IMMORT) {
      sprintf(buf2, "You don't have the %d gold coins bail money which I require.", cost);
      perform_tell(atten, ch, buf2);
      return 1;
    }

    if (!ch->in_room->next)
      return 0;

    for (locker = ch->in_room->next->contents; locker;
	 locker=locker->next_content) {
      if (GET_OBJ_VNUM(locker) != v_locker || 
	  GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch) || 
	  (!locker->contains)) 
        continue;
      if (GET_OBJ_VNUM(locker) != v_locker || 
	  GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch) || 
	  (!locker->contains)) {
        do_say(atten, "Sorry, you don't seem to have a locker here.", 0, 0);
        return 1;
      }
      for (item=locker->contains;item;item=tmp_item) {
        tmp_item=item->next_content;
        obj_from_obj(item);
        obj_to_char(item, ch);
      }
      GET_OBJ_VAL(locker, 0) = 0;
      act("$n opens a locker and gives you all your things.", FALSE, atten, 0, ch, TO_VICT);
      act("$n opens a locker and gives $N all $S things.", FALSE, atten, 0, ch, TO_NOTVICT);
      sprintf(buf2, "That will be %d gold coins.  Try to stay out of trouble.", cost);
      perform_tell(atten, ch, buf2);

      if (GET_LEVEL(ch) < LVL_IMMORT)
	GET_GOLD(ch) -= cost;

      if (ch->in_room->number == 3100) {
	sprintf(buf, "%s received jail impound at %d.", GET_NAME(ch),
		ch->in_room->number);
	slog(buf);
      }

      House_crashsave(ch->in_room->next->number);
      save_char(ch, NULL);

      return 1;
    }
    do_say(atten, "Sorry, you don't seem to have a locker here.", 0, 0);
    return 1;
  }
  return 0;
}
