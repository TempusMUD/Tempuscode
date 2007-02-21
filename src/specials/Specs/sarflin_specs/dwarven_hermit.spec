//
// File: dwarven_hermit.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(dwarven_hermit)
{
	struct obj_data *od = NULL;
	struct Creature *me2 = (struct Creature *)me;
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (CMD_IS("ask"))
		act("$N ignores you!", TRUE, me2, 0, 0, TO_CHAR);

	if (cmd)
		return (0);

	if (!(od = GET_EQ(me2, WEAR_HOLD)))
		return (0);


	if (me2->in_room->people.size() == 1) {
		GET_OBJ_VAL(od, 0) = 0;
		return (0);
	}
	switch (GET_OBJ_VAL(od, 0)) {
	case -1:
		GET_OBJ_VAL(od, 1)++;
		if (GET_OBJ_VAL(od, 1) > 12) {
			GET_OBJ_VAL(od, 0) = 0;
			GET_OBJ_VAL(od, 1) = 0;
		}
		break;
	case 0:
		perform_say(me2, "say", "Back in the old days when clan Brightaxe was");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 1:
		perform_say(me2, "say", "great, our clan lived in the 'Smoking hills'");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 2:
		perform_say(me2, "say", "strong hold. We lived in spelender that could");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 3:
		perform_say(me2, "say", "not be believed and believing that no enemy could");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 4:
		perform_say(me2, "say", "pierce our mighty gates. But we fell to the");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 5:
		perform_say(me2, "say", "Duergar, The vile beasts!, and we fled our ");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 6:
		perform_say(me2, "say", "glorious halls. For you see our gates where not ");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 7:
		perform_say(me2, "say", "pierced, they came up from below.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 8:
		GET_OBJ_VAL(od, 0)++;
		break;
	case 9:
		perform_say(me2, "say", "For many years I have waited and looked for some");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 10:
		perform_say(me2, "say", "way to retake our halls. And against that day I");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 11:
		perform_say(me2, "say", "hid a key to the gates. I hid it in a old stump.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 12:
		perform_say(me2, "say", "But I don't rember where it is anymore!");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 13:
		GET_OBJ_VAL(od, 0)++;
		break;
	case 14:
		perform_say(me2, "say", "Back in the old days when clan Brightaxe was");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 15:
		perform_say(me2, "say", "great our clan lived in the 'Smoking hills'");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 16:
        perform_say(me2, "say", "strong hold. We had treasure beyond all imagination.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 17:
		perform_say(me2, "say", "The magical guardian was build into the door of");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 18:
		perform_say(me2, "say", "the treasure room if you did not speak the right");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 19:
		perform_say(me2, "say", "words it would send you to certen death.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 20:
		GET_OBJ_VAL(od, 0)++;
		break;
	case 21:
		perform_say(me2, "say", "First speak the name of our Homeland....");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 22:
		perform_say(me2, "say", "Then speak the name of our greatest Battle....");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 23:
		perform_say(me2, "say", "Next speak the name of our greatest Hero....");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 24:
		perform_say(me2, "say", "Finaly speak the name of our greatest King....");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 25:
		perform_say(me2, "say", "And so the entrance to the chamber will be opened.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 26:
		perform_say(me2, "say", "And then state your name for the record.");
		GET_OBJ_VAL(od, 0)++;
		break;
	case 27:
		GET_OBJ_VAL(od, 0) = -1;
		break;
	}
	return (0);
}
