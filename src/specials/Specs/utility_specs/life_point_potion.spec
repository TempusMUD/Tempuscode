//
// File: life_point_potion.spec                     -- Part of TempusMUD
//
// Copyright 1999 by John, all rights reserved.
//

SPECIAL(life_point_potion)
{
	struct obj_data *potion = (struct obj_data *) me;
	if (!CMD_IS("quaff") || potion->carried_by != ch)
	return 0;
	skip_spaces(&argument);
	if (!isname(argument, potion->name)) {
		sprintf(buf,"Argument (%s), Name (%s)\r\n",argument,potion->name);
		return 0;
	}
	//Format : <number of life points> 
	act("You hungrily consume $p, careful not to miss a single drop.",
		      FALSE, ch, potion, 0, TO_CHAR);
	act("$n hungrily consumes $p, careful not to miss a single drop.",
		      FALSE, ch, potion, 0, TO_ROOM);
	if ((IS_OBJ_STAT(potion, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(potion, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
		(IS_OBJ_STAT(potion, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
        || GET_LEVEL(ch) < 10 ) {
		send_to_char(ch, "\r\nYou feel you have just done something very, very wrong.\r\n");
		ch->setPosition( POS_STUNNED );
	} else {
		send_to_char(ch, "\r\nThe essence of the gods courses through your veins.\r\n");
		GET_LIFE_POINTS(ch) += MIN(10, GET_OBJ_VAL(potion, 0));
	}
	obj_from_char(potion);
	extract_obj(potion);
	return 1;
}
  
