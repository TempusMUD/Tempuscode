//
// File: wagon_driver.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(wagon_driver)
{
  struct obj_data *i;
  struct char_data *driver = (struct char_data *) me;
  struct room_data *destination = NULL;
  int wagon_obj_rnum = 10, dir, num;
  
  if (cmd || !AWAKE(ch) || ch->in_room->number != 10)
    return 0;

  if (!number(0,7))   /* Only try to some of the time */
    return 0;

  for (num = 0, i = object_list; i; i = i->next)
    if ((wagon_obj_rnum == GET_OBJ_VNUM(i)) && (i->in_room != NULL)) 
      break;

  if (!i) {
    mudlog(" WARNING:  Wagon driver cannot find his wagon (object 10)!", BRF, LVL_DEMI, TRUE);
    driver->mob_specials.shared->func = NULL;
    REMOVE_BIT(MOB_FLAGS(driver), MOB_SPEC);
    return 0;
  }
 
  for (dir = 0; dir <= 3 ; dir++) {
    if (EXIT(i, dir) && EXIT(i, dir)->to_room != NULL &&
        (i->in_room->sector_type ==
        EXIT(i, dir)->to_room->sector_type) && CAN_GO(i, dir)
        && !number(0, 4) && !ROOM_FLAGGED(EXIT(i, dir)->to_room, ROOM_INDOORS)) 
      break;
  }
  if ((dir == 4) || !CAN_GO(i, dir))
    return 0;
  else {
    destination = EXIT(i, dir)->to_room;

    act("$n twitches the reins.", TRUE, driver, 0, 0, TO_ROOM);
    act("The wagon driver twitches the reins.", TRUE, 0, i, 0, TO_ROOM);
    sprintf(buf, "$p rolls off to the %s.", to_dirs[dir]);
    act(buf, TRUE, 0, i, 0, TO_ROOM);

    obj_from_room(i);
    obj_to_room(i, destination);

    sprintf(buf, "$p drives in from %s.", from_dirs[dir]);
    act(buf, FALSE, 0, i, 0, TO_ROOM);

    return 1;
  } 
  return 0;
}
