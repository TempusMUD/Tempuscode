//
// File: horn_of_geryon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(horn_of_geryon)
{
	struct obj_data *horn = (struct obj_data *)me;
	struct Creature *minotaur = NULL;

	if (spec_mode != SPECIAL_CMD)
		return 0;
	if ((!CMD_IS("wind") && !CMD_IS("blow"))
		|| GET_OBJ_TYPE(horn) == ITEM_PIPE)
		return 0;

	act("$n sounds a clear, ultra low note on $p.", FALSE, ch, horn, 0,
		TO_ROOM);
	act("You sound a clear, ultra low note on $p.", FALSE, ch, horn, 0,
		TO_CHAR);

	if (GET_OBJ_VAL(horn, 0)) {
		if ((minotaur = read_mobile(16144))) {
			char_to_room(minotaur, ch->in_room, false);
			act("$n appears from a cold, swirling mist.", FALSE, minotaur, 0,
				0, TO_ROOM);
			add_follower(minotaur, ch);
			SET_BIT(AFF_FLAGS(minotaur), AFF_CHARM);
			SET_BIT(MOB_FLAGS(minotaur), MOB_PET);
			GET_OBJ_VAL(horn, 0)--;
			if (GET_OBJ_VAL(horn, 0) <= 0)
				extract_obj(horn);
			WAIT_STATE(ch, PULSE_VIOLENCE * 3);
			return 1;
		}
	}
	return 0;
}
