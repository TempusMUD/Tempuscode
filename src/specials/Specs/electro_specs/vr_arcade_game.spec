//
// File: vr_arcade_game.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/******

  How to build a VR arcade game:
  For the object, set:
  Value[0] : The vnum of the startroom in your game world.
  Value[1] : The number of coins you must pay to enter.
  Value[2] : The maximum level of player allowed in game.
  Value[3] : The Hometown Number associated with your game.

  ********/

SPECIAL(vr_arcade_game)
{
	struct obj_data *game = (struct obj_data *)me;
	struct obj_data *lckr = NULL, *obj = NULL, *nextobj = NULL;
	struct room_data *r_startroom = NULL, *r_lckr_rm = real_room(30197);
	int i;

	if (spec_mode != SPECIAL_CMD) {
		return 0;
	}

	if (!CMD_IS("play") && !CMD_IS("enter"))
		return 0;

	if (GET_LEVEL(ch) < LVL_IMMORT)
		return 0;

	skip_spaces(&argument);

	if (!isname(argument, game->aliases))
		return 0;

	if (r_lckr_rm == NULL) {
		send_to_char(ch, "Sorry, all games are out of order right now.\r\n");
		return 1;
	}

	for (lckr = r_lckr_rm->contents; lckr; lckr = lckr->next_content) {
		if (GET_OBJ_VNUM(lckr) == 30189 && !lckr->contains)
			break;
	}

	if (!lckr) {
		send_to_char(ch, "Sorry, all the lockers are filled right now."
			"  Try again later.\r\n");
		return 1;
	}

	if ((r_startroom = real_room(GET_OBJ_VAL(game, 0))) == NULL) {
		send_to_char(ch, "This game is currently out of order.\r\n");
		return 1;
	}

	if (GET_GOLD(ch) < GET_OBJ_VAL(game, 1)) {
		send_to_char(ch, "You don't have the %d coins required to play.\r\n",
			GET_OBJ_VAL(game, 1));
		return 1;
	}

	if (GET_LEVEL(ch) > GET_OBJ_VAL(game, 2)) {
		send_to_char(ch, "Go play somewhere else.  Trix are for kids.\r\n");
		return 1;
	}

	for (obj = ch->carrying; obj; obj = nextobj) {
		nextobj = obj->next_content;
		obj_from_char(obj);
		obj_to_obj(obj, lckr);
	}

	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i))
			obj_to_obj(unequip_char(ch, i, MODE_EQ), lckr);
	}

	GET_OBJ_VAL(lckr, 0) = GET_IDNUM(ch);
	
	House* house = Housing.findHouseByRoom( lckr->in_room->number );
	if( house != NULL )
		house->save();
	ch->saveToXML();

	send_to_char(ch, "You insert %d coins in %s.\r\n", GET_OBJ_VAL(game, 1),
		game->name);

	send_to_char(ch,
		"You step into the interface... You are blinded by a bright"
		" light!!\r\n");
	act("$n steps into $p's interface and disappears in a flash!", FALSE, ch,
		game, 0, TO_ROOM);

	GET_HOMEROOM(ch) = GET_HOME(ch);
	GET_HOME(ch) = GET_OBJ_VAL(game, 3);

	char_from_room(ch, false);
	char_to_room(ch, r_startroom, false);

	act("$n appears at the center of the room with a flash!",
		FALSE, ch, 0, 0, TO_ROOM);

	look_at_room(ch, ch->in_room, 0);

	return 1;
}
