//
// File: vein.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(vein)
{
	struct obj_data *self = (struct obj_data *)me;
	struct obj_data *new_obj = NULL;
	struct obj_data *pick;

	if (spec_mode != SPECIAL_CMD || !CMD_IS("hit"))
		return false;

	pick = GET_EQ(ch, WEAR_HOLD);
	if (pick && !isname("pick", pick->aliases))
		pick = NULL;

	if (!pick) {
		pick = GET_EQ(ch, WEAR_WIELD);
		if (pick && !isname("pick", pick->aliases))
			pick = NULL;
	}

	if (!pick) {
		act("You don't have a pick! how do you think to mine with out one?",
			TRUE, ch, 0, 0, TO_CHAR);
		return true;
	}

	if (number(0, GET_OBJ_VAL(self, 1))) {
		act("$n hits the vein with $p.", TRUE, ch, pick, 0, TO_ROOM);
		act("You hit the vein knocking some rock lose.", TRUE, ch, 0, 0,
			TO_CHAR);
		return (1);
	}
	new_obj = read_object(GET_OBJ_VAL(self, 0));
	if (!new_obj) {
		act("Your pick goes awry and you nearly hit yourself!", true,
			ch, pick, 0, TO_CHAR);
		act("$n's pick goes awry and $e nearly hit $mself!", true,
			ch, pick, 0, TO_ROOM);
		errlog("vein #%d has invalid ovnum #%d",
			GET_OBJ_VNUM(self), GET_OBJ_VAL(self, 0));
		return true;
	}

	act("$n hits the vein with $p, and breaks off $P.",
		TRUE, ch, pick, new_obj, TO_ROOM);
	act("You knock $P off $p.", TRUE, ch, self, new_obj, TO_CHAR);

	if (self->in_room)
		obj_to_room(new_obj, self->in_room);
	else if (self->carried_by)
		obj_to_room(new_obj, self->carried_by->in_room);
	else
		extract_obj(new_obj);

	return (1);
}
