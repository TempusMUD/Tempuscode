//
// File: stable_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stable_room)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  struct room_data *pet_room;
  struct char_data *pet = NULL, *pet2 = NULL;
  int price;

  if (!(pet_room = ch->in_room->next)) {
    send_to_char("stable error.\r\n", ch);
    return 0;
  }

  if (CMD_IS("list")) {
    send_to_char("Available mounts are:\r\n", ch);
    for (pet = pet_room->people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", 3 * GET_EXP(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such mount!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < (GET_EXP(pet) * 3)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= GET_EXP(pet) * 3;

    pet = read_mobile(GET_MOB_VNUM(pet));
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May this mount serve you well.\r\n", ch);
    act("$n buys $N as a mount.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }

  if (CMD_IS("sell") || CMD_IS("value")) {
    
    skip_spaces(&argument);

    if (!*argument) {
      sprintf(buf, "%s what mount?\r\n", CMD_IS("sell") ? "Sell" : "Value");
      return 1;
    }
    if (!(pet = get_char_room_vis(ch, argument))) {
      send_to_char("I don't see that mount here.\r\n", ch);
      return 1;
    }
    if (!pet->master || pet->master != ch || !AFF_FLAGGED(pet, AFF_CHARM)) {
      act("You don't have the authority to sell $N.",FALSE,ch,0,pet,TO_CHAR);
      return 1;
    }
    if (!IS_NPC(pet) || !MOB2_FLAGGED(pet, MOB2_MOUNT)) {
      send_to_char("I only buy mounts.\r\n", ch);
      return 1;
    }
    price = GET_LEVEL(pet) * 10 + GET_MOVE(pet) * 10 + GET_HIT(pet) * 10;

    sprintf(buf, "I will pay you %d gold coins for $N.", price);
    act(buf, FALSE,ch,0,pet,TO_CHAR);
    if (CMD_IS("value"))
      return 1;

    if (MOUNTED(ch) && MOUNTED(ch) == pet) {
      MOUNTED(ch) = NULL;
      REMOVE_BIT(AFF2_FLAGS(pet), AFF2_MOUNTED);
      ch->setPosition( POS_STANDING );
    }
    stop_follower(pet);

    act("The stables now owns $N.", FALSE, ch, 0, pet, TO_CHAR);
    act("$n sells $N to the stables.", FALSE, ch, 0, pet, TO_ROOM);

    GET_GOLD(ch) += price;
    GET_EXP(pet) = price >> 1;

    char_from_room(pet);
    char_to_room(pet, pet_room);
    for (pet2 = pet_room->people; pet2; pet2 = pet2->next_in_room) {
      if (pet2 != pet && IS_NPC(pet2) && GET_MOB_VNUM(pet2) ==
	  GET_MOB_VNUM(pet)) {
	extract_char(pet2, 0);
	return 1;
      }
    }
      
    return 1;
  }
    
  /* All commands except list and buy */
  return 0;
}





