//
// File: stepping_stone.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stepping_stone)
{
  struct obj_data *ruby = (struct obj_data *) me;

  if (CMD_IS("south")) {
    if (GET_POS(ch) >= POS_STANDING) {
      if (GET_HOME(ch) != HOME_ARENA) {
        act("$p flares up suddenly with a bright light!", 
	    FALSE, ch, ruby, 0, TO_ROOM);
        send_to_char("You feel a strange sensation...\r\n", ch);
        sprintf(buf, "A voice BOOMS out, 'Welcome to the Arena, %s!'\r\n",
		GET_NAME(ch));
        send_to_zone(buf, ch->in_room->zone, 0);
        GET_HOLD_HOME(ch) = GET_HOME(ch);
	if (PLR_FLAGGED(ch, PLR_LOADROOM))
	  GET_HOLD_LOADROOM(ch) = GET_LOADROOM(ch);
	else
	  GET_HOLD_LOADROOM(ch) = -1;
        GET_HOME(ch) = HOME_ARENA;
	GET_LOADROOM(ch) = -1;
      }
    }
  }
  return 0;
}

SPECIAL(portal_out)
{
  struct obj_data *portal = (struct obj_data *) me;
  skip_spaces(&argument); 

  if (!CMD_IS("enter"))
    return 0;
  if (!*argument)  {
    send_to_char("Enter what?  Enter the portal to leave the arena.\r\n", ch);
    return 1;
  }
  if (isname(argument, portal->name)) {
    send_to_room("A loud buzzing sound fills the room.\r\n", ch->in_room);
    if (GET_HOME(ch) == HOME_ARENA)
      GET_HOME(ch) = GET_HOLD_HOME(ch);
    if (real_room(GET_LOADROOM(ch) = GET_HOLD_LOADROOM(ch)) &&
	GET_HOLD_LOADROOM(ch))
      SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
    GET_HOLD_LOADROOM(ch) = -1;
    sprintf(buf, "A voice BOOMS out, '%s has left the arena.'\r\n", 
	    GET_NAME(ch));
    send_to_zone(buf, ch->in_room->zone, 0);
    call_magic(ch, ch, 0, SPELL_WORD_OF_RECALL, LVL_GRIMP, CAST_SPELL); 
    return 1;
  }
  return 0;
}
  

SPECIAL(arena_locker)
{
  struct char_data *atten = (struct char_data *) me;
  struct obj_data *locker, *item, *tmp_item;
  struct room_data *r_locker_room;

  ACMD(do_say);

  if (!(r_locker_room = real_room(40099)))
    return 0;

  if (CMD_IS("store")) {
    do_say(ch, "I'd like to store my stuff, please.", 0, 0);
    if (IS_NPC(ch)) {
      do_say(atten, "Sorry, I cannot store things for mobiles.", 0, 0);
      return 1;
    }
    if (!(IS_CARRYING_W(ch) + IS_WEARING_W(ch))) {
      do_say(atten, "Looks to me like you're already stark naked.", 0, 0);
      return 1;
    }
    if (IS_WEARING_W(ch)) {
      do_say(atten, "You need to remove all your gear first.", 0, 0);
      return 1;
    }
    for (locker=r_locker_room->contents;locker;locker=locker->next_content) {
      if (GET_OBJ_VNUM(locker) != 40099 || GET_OBJ_VAL(locker, 0) ||
          locker->contains)
        continue;
      for (item = ch->carrying;item;item=tmp_item) {
        tmp_item = item->next_content;
        obj_from_char(item);
        obj_to_obj(item, locker);
      }
      GET_OBJ_VAL(locker, 0) = GET_IDNUM(ch);
      act("$n takes all your things and locks them in a locker.", 
	  FALSE, atten, 0, ch, TO_VICT);
      act("You are now stark naked!", FALSE, atten, 0, ch, TO_VICT);
      act("$n takes all $N's things and locks them in a locker.", 
	  FALSE, atten, 0, ch, TO_NOTVICT);
      act("$N is now stark naked!", FALSE, atten, 0, ch, TO_NOTVICT);
      return 1;
    }
    do_say(atten, "Sorry, all the lockers are occupied at the moment.", 0, 0);
    return 1;
  }
  if (CMD_IS("receive")) {
    do_say(ch, "I'd like to get my stuff back , please.", 0, 0);
    if (IS_NPC(ch)) {
      do_say(atten, "Sorry, I don't deal with mobiles.", 0, 0);
      return 1;
    }

    for (locker=r_locker_room->contents;locker;locker=locker->next_content) {
      if (GET_OBJ_VNUM(locker) != 40099 || 
	  GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
                      || (!locker->contains)) 
        continue;
      if (GET_OBJ_VNUM(locker) != 40099 || 
	  GET_OBJ_VAL(locker, 0) != GET_IDNUM(ch)
                      || (!locker->contains)) {
        do_say(atten, "Sorry, you don't seem to have a locker here.", 0, 0);
        return 1;
      }
      for (item=locker->contains;item;item=tmp_item) {
        tmp_item=item->next_content;
        obj_from_obj(item);
        obj_to_char(item, ch);
      }
      GET_OBJ_VAL(locker, 0) = 0;
      act("$n opens a locker and gives you all your things.", 
	  FALSE, atten, 0, ch, TO_VICT);
      act("$n opens a locker and gives $N all $S things.", 
	  FALSE, atten, 0, ch, TO_NOTVICT);

      House_crashsave(r_locker_room->number);
      save_char(ch, NULL);

      return 1;
    }
    do_say(atten, "Sorry, you don't seem to have a locker here.", 0, 0);
    return 1;
  }
  return 0;
}
