//
// File: elevator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(elevator_panel)
{
  struct obj_data *panel = (struct obj_data *) me, *door = NULL, *car = NULL;
  struct elevator_data *elev = NULL;
  struct elevator_elem *elem = NULL;
  static unsigned int elev_index = 700;
  int start = FALSE;

  if (!(elev = real_elevator(GET_OBJ_VAL(panel, 0)))) {
    slog("SYSERR:  Invalid elevator vnum on elevator_panel.");
    return 0;
  }

  for (door = ch->in_room->contents; door; door = door->next_content)
    if (IS_OBJ_TYPE(door, ITEM_V_DOOR) &&
	ROOM_NUMBER(door) == ch->in_room->number)
      break;

  if (!door)
    return 1;
  
  if (!(car = find_vehicle(door)))
    return 1;

  if (CMD_IS("status")) {
    sprintf(buf, "Elevator [%d]\r\n"
	    "Currently at room [%5d] %s\r\n", elev->vnum,
	    car->in_room->number, car->in_room->name);
    
    for (elem = elev->list; elem; elem = elem->next) {
	send_to_char(ch, "%s [%5d ] %s [%8s]\r\n", buf,
		elem->rm_vnum, 
		(elem->rm_vnum == car->in_room->number) ? "*" : " ",
		elem->name);
    }
    return 1;
  }
  if (CMD_IS("press") || CMD_IS("push")) {
    one_argument(argument, buf2);
    
    if (!*buf2)
      return 0;

    if (!(elem = elevator_elem_by_name(buf2, elev))) {
      send_to_char(ch, "No such operation.\r\n");
      return 1;
    }

    sprintf(buf, "$n pushes the button labeled [%s] on $p.", elem->name);
    act(buf, FALSE, ch, panel, 0, TO_ROOM);

    if (!real_room(elem->rm_vnum)) {
      send_to_char(ch, "That function is out of order.\r\n");
      return 1;
    }

    if (elem->rm_vnum == car->in_room->number) {
      send_to_char(ch, "Nothing seems to happen.\r\n");
      return 1;
    }

    if (target_room_on_queue(elem->rm_vnum, elev)) {
      act("The panel beeps with annoyance.", FALSE,ch,0,0,TO_ROOM);
      return 1;
    }
    elev_index++;

    sprintf(buf, "%d elev%d 1 1 1 %d", elev_index, elev_index, elem->rm_vnum);

    if (add_path(buf, FALSE)) {
      send_to_char(ch, "Major Error has arrived from the west.\r\n");
      return 1;
    }
    
    if (!elev->pQ)
      start = TRUE;

    if (!(path_to_elevator_queue(elev_index, elev))) {
      send_to_char(ch, "Shitfire.\r\n");
      return 1;
    }

    if (start)
      add_path_to_vehicle(car, elev->pQ->phead->name);

    send_to_char(ch, "You push it.\r\n");
    return 1;
  }
  return 0;
}

