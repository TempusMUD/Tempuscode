//
// File: red_highlord.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(red_highlord)
{
  ACMD(do_gen_comm);
  ACCMD(do_wield);
  ACCMD(do_get);
  ACMD(do_remove);
  int sword_vnum = 9204;
  room_num v_home_room = 9210;
  struct room_data *r_home_room = NULL; 
  struct room_data *target_room = NULL;
  struct room_data *was_in = NULL;
  struct obj_data *blade = NULL, *container = NULL;
  struct Creature *vict = NULL, *tmp_vict = NULL;

  if (cmd || FIGHTING(ch) || HUNTING(ch) || ch->getPosition() <= POS_SLEEPING)
    return 0; 
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;

  if ((r_home_room = real_room(v_home_room)) != NULL && ch->in_room != 
      r_home_room && !ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)) {
    act("$n kneels and utters an obscene prayer to Takhisis.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n disappears in a flash of light!", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch,false);
    char_to_room(ch, r_home_room,false);
    act("$n appears in a flash of light!", FALSE, ch, 0, 0, TO_ROOM);
    return 1;
  }

  if (GET_EQ(ch, WEAR_WIELD) && 
      GET_OBJ_VNUM(GET_EQ(ch, WEAR_WIELD)) == sword_vnum)
    return 0;
  
  if ((blade = get_obj_in_list_all(ch, "desolation", ch->carrying))) {
    if (GET_EQ(ch, WEAR_WIELD))
      do_remove(ch, fname(GET_EQ(ch, WEAR_WIELD)->name), 0, 0);
    do_wield(ch, "desolation", 0, 0);  
    return 1;
  } if ((blade=get_obj_in_list_all(ch,"desolation", ch->in_room->contents))) {
    do_get(ch, "desolation", 0, 0);  
    return 1;
  }

  for (blade = object_list; blade; blade = blade->next) {
    if (GET_OBJ_VNUM(blade) != sword_vnum || 
	blade->shared->proto->short_description != blade->short_description ||
	IS_OBJ_STAT2(blade, ITEM2_RENAMED) || 
	IS_OBJ_STAT2(blade, ITEM2_PROTECTED_HUNT))
      continue;
    else if (blade->in_room != NULL &&
	     !ROOM_FLAGGED(blade->in_room, ROOM_NORECALL | 
			   ROOM_GODROOM |  ROOM_HOUSE)) {
      target_room = blade->in_room;
      break;
    } else if (((tmp_vict = blade->carried_by) || 
                (tmp_vict = blade->worn_by)) &&
	       tmp_vict != ch) {
      if (GET_LEVEL(tmp_vict) >= LVL_IMMORT || IS_MOB(tmp_vict) ||
          GET_LEVEL(tmp_vict) < 3) {
        continue;
      } else if (find_first_step(ch->in_room, tmp_vict->in_room, STD_TRACK) >= 0) {
        if (!vict || GET_LEVEL(vict) < GET_LEVEL(tmp_vict))
          vict = tmp_vict;
        continue;
      }
    } else if ((container = blade->in_obj)) {
      while (container->in_obj) {
        container = container->in_obj;
      }
      if (container->in_room != NULL && 
	  !ROOM_FLAGGED(container->in_room, ROOM_GODROOM | ROOM_HOUSE)) {
        target_room = container->in_room;
        break;
      } else if (((tmp_vict = container->carried_by) || 
                  (tmp_vict = container->worn_by)) &&
		 tmp_vict != ch) {
        if (GET_LEVEL(tmp_vict) >= LVL_IMMORT ||
            GET_LEVEL(tmp_vict) < 3) {
          continue;
        } else if (find_first_step(ch->in_room, tmp_vict->in_room, STD_TRACK) >= 0) {
          if (!vict || GET_LEVEL(vict) < GET_LEVEL(tmp_vict))
            vict = tmp_vict;
          continue;
        }
      }
    }
  }


  if (target_room != NULL &&
      (target_room->zone == ch->in_room->zone ||
       (!ZONE_FLAGGED(target_room->zone, ZONE_ISOLATED) &&
	(!ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)))) &&
      !ROOM_FLAGGED(target_room, ROOM_NORECALL | ROOM_ARENA |
		    ROOM_GODROOM | ROOM_HOUSE)) {
    if (target_room == blade->in_room || 
	(container && target_room == container->in_room)) {
      act("$n utters some arcane words and disappears in a flash of light.", 
	  FALSE, ch, 0, 0, TO_ROOM);
      was_in = ch->in_room;
      char_from_room(ch,false);
      char_to_room(ch, target_room,false);
      act("$n appears with a flash of light at the center of the room.", 
	  FALSE, ch, 0, 0, TO_ROOM);

      if (blade->in_room == ch->in_room)
        obj_from_room(blade);
      else if (blade->in_obj &&container &&container->in_room==ch->in_room) {
        act("$n grabs $p.", TRUE, ch, container, 0, TO_ROOM);
        act("$n rummages around, and extracts $p.", TRUE,ch,blade,0,TO_ROOM);
        act("$n throws $p to the ground with disgust.",
	    FALSE,ch,container,0,TO_ROOM);
        obj_from_obj(blade);
      }
      obj_to_char(blade, ch);
      act("$n grabs $p and disappears as suddenly as $e came!", 
	  FALSE, ch, blade, 0, TO_ROOM);
      char_from_room(ch,false);
      char_to_room(ch, was_in,false);
      act("$n reappears suddenly, with a big grin on $s face.",
	  FALSE, ch, 0, 0, TO_ROOM);
      return 1;
    }
  } else if (vict && !number(0, 38)) {
    HUNTING(ch) = vict;
    switch (number(0, 2)) {
    case 0:
      do_gen_comm(ch, "Now where might that Desolation Blade be?", 0,
		  SCMD_GOSSIP);
      break;
    case 1:
      perform_tell(ch, vict, "The blade...");
      break;
    default:
      perform_tell(ch, vict, "I seem to be missing something.");
      break;
    }
    return 1;
  }
  return 0;
}



