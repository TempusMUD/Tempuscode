//
// File: spinal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(spinal)
{
  struct char_data *spinal = (struct char_data *) me;
  struct obj_data *spine = NULL;
  struct room_data *r_home_pad = real_room(1595);
  
  if (FIGHTING(spinal))
    return 0;

  if (!cmd && spinal->in_room != r_home_pad &&
      r_home_pad != NULL && !FIGHTING(ch)) {
    act("$n departs suddenly into the deeper ethereal.", 
	FALSE, spinal, 0, 0, TO_ROOM);
    char_from_room(spinal);
    char_to_room(spinal, r_home_pad);
    spinal->in_room->zone->idle_time = 0;

    act("$n steps in from another plane of existence.", 
	FALSE, spinal, 0, 0, TO_ROOM);
    return 1;
  } else if (cmd) {
    return 0;
  }

  for (spine = object_list; spine; spine = spine->next) {
    if (!IS_OBJ_STAT2(spine, ITEM2_BODY_PART) ||
        !CAN_WEAR(spine, ITEM_WEAR_WIELD) ||
        !isname("spine", spine->name) ||
        spine->worn_by || 
        (!spine->carried_by && spine->in_room == NULL)) {
      if (spine->next) {
        continue;
      } else {
        return 0;
      }
    } else
      break;
  } 

  if (!spine) 
    return 0;

  if (spine->carried_by && spine->carried_by != spinal) {
    if (spine->carried_by->in_room != NULL &&
        GET_LEVEL(spine->carried_by) < LVL_IMMORT) {
      char_from_room(spinal);
      char_to_room(spinal, spine->carried_by->in_room);
      spinal->in_room->zone->idle_time = 0;

      act("$n steps in from the ethereal plane.", 
	  FALSE, spinal, 0, 0, TO_ROOM);
      act("$n grins at you and grabs $p.", 
	  FALSE, spinal, spine, spine->carried_by, TO_VICT);
      act("$n grins at $N and grabs $p.", 
	  FALSE, spinal, spine, spine->carried_by, TO_NOTVICT);
      act("$n eats $p.", FALSE, spinal, spine, 0, TO_ROOM);
      extract_obj(spine);
      return 1;
    } else {
      return 0;
    }
  } else if (spine->in_room != NULL) {
    char_from_room(spinal);
    char_to_room(spinal, spine->in_room);
    spinal->in_room->zone->idle_time = 0;

    act("$n steps in from the ethereal plane.", FALSE, spinal, 0, 0, TO_ROOM);
    act("$n grins and grabs $p.", 
	FALSE, spinal, spine, spine->carried_by, TO_ROOM);
    act("$n eats $p.", FALSE, spinal, spine, 0, TO_ROOM);
    extract_obj(spine);
    return 1;
  } else {
    return 0;
  }
}



