//
// File: corpse_retrieval.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(corpse_retrieval)
{
  struct obj_data *corpse;
  struct char_data *retriever = (struct char_data *) me;
  char buf2[MAX_STRING_LENGTH];
  ACMD(do_say);
  int price;

  if (!CMD_IS("retrieve"))
    return 0;
  if (!CAN_SEE(retriever, ch)) {
    do_say(retriever, "Who's there?  I can't see you.", 0, 0);
    return 1;
  }
  if (IS_MOB(ch)) {
    act("$n snickers at $N.", FALSE, retriever, 0, ch, TO_NOTVICT);
    act("$n snickers at you.", FALSE, retriever, 0, ch, TO_VICT);
    return 1;
  }

   
  for (corpse = object_list;corpse;corpse=corpse->next)
    if (GET_OBJ_TYPE(corpse) == ITEM_CONTAINER && GET_OBJ_VAL(corpse, 3) && 
                               CORPSE_IDNUM(corpse) == GET_IDNUM(ch)) {
      if (corpse->contains)
        price = GET_LEVEL(ch) * 4000;     
      else
        price = GET_LEVEL(ch) * 100;

      if (price > GET_GOLD(ch)) { 
        sprintf(buf2, "You don't have enough gold.  It costs %d coins.", price);
        perform_tell(retriever, ch, buf2);
      } else {
        if (corpse->carried_by && corpse->carried_by == ch) {
            perform_tell(retriever, ch, 
			 "You already have it you dolt!");
            return 1;
        }
        if (corpse->in_room) {
          if (corpse->in_room == retriever->in_room) {
            perform_tell(retriever, ch, 
			 "You fool.  It's already in this room!");
            return 1;
          }
	  if (ROOM_FLAGGED(corpse->in_room, ROOM_NORECALL) ||
	      (corpse->in_room->zone != ch->in_room->zone &&
	       (ZONE_FLAGGED(corpse->in_room->zone, ZONE_ISOLATED) ||
		ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)))) {
	    send_to_char(ch, "Your corpse cannot be located!\r\n");
	    return 1;
	  }
          act("$p disappears with a flash!", TRUE, 0, corpse, 0, TO_ROOM);
          obj_from_room(corpse);
        } else if (corpse->in_obj)
          obj_from_obj(corpse);
        else if (corpse->carried_by) {
          act("$p disappears out of your hands!", 
	      FALSE, corpse->carried_by, corpse, 0, TO_CHAR);
          obj_from_char(corpse);
        } else if (corpse->worn_by) {
          act("$p disappears off of your body!", 
	      FALSE, corpse->worn_by, corpse, 0, TO_CHAR);
	  if (corpse == GET_EQ(corpse->worn_by, corpse->worn_on))
	    unequip_char(corpse->worn_by, corpse->worn_on, MODE_EQ);
	  else
	    unequip_char(corpse->worn_by, corpse->worn_on, MODE_IMPLANT);
        } else {
          perform_tell(retriever, ch, 
		       "I'm sorry, your corpse has shifted out of the universe.");
          return 1;
        }
        sprintf(buf2, 
		"Very well.  I will retrieve your corpse for %d coins.", 
		price);
        perform_tell(retriever, ch, buf2);
        GET_GOLD(ch) -= price;
        act("$n makes some strange gestures and howls at $s head!", 
            FALSE, retriever, 0, 0, TO_ROOM);
        act("$p appears in your hands!", FALSE, ch, corpse, 0, TO_CHAR);
        obj_to_char(corpse, ch);
      } 
      return 1;
    }
  sprintf(buf2, "Sorry %s, I cannot locate your corpse.", PERS(ch, retriever));
  perform_tell(retriever, ch, buf2);
  return 1;
}
        

