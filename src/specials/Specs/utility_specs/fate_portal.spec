//
// File: fate_portal.spec                     -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//


SPECIAL(fate_portal)
{
	struct obj_data *portal = (struct obj_data *)me;
	char arg1[MAX_INPUT_LENGTH];

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (!CMD_IS("enter") || !CAN_SEE_OBJ(ch, portal) || !AWAKE(ch))
		return 0;
	one_argument(argument, arg1);
	if (!isname(arg1, portal->name))
		return 0;

	// Is he ready to remort? Or level 49 at least?
	if (!IS_NPC(ch) && GET_LEVEL(ch) >= 49) {
		if (GET_OBJ_VAL(portal, 2) == 1 && GET_REMORT_GEN(ch) <= 3) {
			return 0;
		} else if (GET_OBJ_VAL(portal, 2) == 2
			&& GET_REMORT_GEN(ch) <= 6 && GET_REMORT_GEN(ch) >= 4) {
			return 0;
		} else if (GET_OBJ_VAL(portal, 2) == 3 && GET_REMORT_GEN(ch) >= 7) {
			return 0;
		}
	}
	act("$n is repulsed by $p.", FALSE, ch, portal, 0, TO_ROOM);
	act("You are repulsed by $p.", FALSE, ch, portal, 0, TO_CHAR);
	return 1;
}
