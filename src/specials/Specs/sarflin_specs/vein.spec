//
// File: vein.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(vein)
{
	struct obj_data *me2 = (struct obj_data *)me;
	struct obj_data *new_obj = NULL;
	struct obj_data *obj_hand = GET_EQ(ch, WEAR_HOLD);
	int pick = 0;

	if (!CMD_IS("hit"))
		return (0);

	if (obj_hand && isname("pick", obj_hand->name))
		pick = 1;

	if (!pick && (obj_hand = GET_EQ(ch, WEAR_WIELD)) &&
		isname("pick", obj_hand->name))
		pick = 1;

	if (!pick) {
		act("You don't have a pick! how do you think to mine with out one?",
			TRUE, ch, 0, 0, TO_CHAR);
		return (1);
	}

	if (number(0, GET_OBJ_VAL(me2, 1))) {
		act("$n hits the vein with $p.", TRUE, ch, obj_hand, 0, TO_ROOM);
		act("You hit the vein knocking some rock lose.", TRUE, ch, 0, 0,
			TO_CHAR);
		return (1);
	}

	if (!(new_obj = read_object(GET_OBJ_VAL(me2, 0))))
		return 1;

	act("$n hits the vein with $p, and breaks off $P.",
		TRUE, ch, obj_hand, new_obj, TO_ROOM);
	act("You knock $P off $p.", TRUE, ch, me2, new_obj, TO_CHAR);

	if (me2->in_room)
		obj_to_room(new_obj, me2->in_room);
	else if (me2->carried_by)
		obj_to_room(new_obj, me2->carried_by->in_room);
	else
		extract_obj(new_obj);

	return (1);
}
