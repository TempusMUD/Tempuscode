//
// File: gen_shower_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(gen_shower_rm)
{
	bool acid_cleaned = false;
	obj_data *cur_obj;
	int idx;

	if (spec_mode != SPECIAL_CMD || !CMD_IS("shower"))
		return 0;

	send_to_char(ch,
		"You turn on the shower and step under the pouring water.\r\n");
	act("$n turns on the shower and steps under the pouring water.",
		FALSE, ch, 0, 0, TO_ROOM);

	if (affected_by_spell(ch, SPELL_ACIDITY)) {
		affect_from_char(ch, SPELL_ACIDITY);
		acid_cleaned = true;
	}
	if (affected_by_spell(ch, SPELL_ACID_BREATH)) {
		affect_from_char(ch, SPELL_ACID_BREATH);
		acid_cleaned = true;
	}

	if (acid_cleaned)
		send_to_char(ch, "All the acid on your body is washed off!  Whew!\r\n");

	WAIT_STATE(ch, 3 RL_SEC);

	if (IS_WEARING_W(ch) || IS_CARRYING_W(ch)) {
		send_to_char(ch, "All your things get a good cleaning!\r\n");
		for (idx = 0;idx < NUM_WEARS;idx++) {
			CHAR_SOILAGE(ch, idx) = 0;
			if (GET_EQ(ch, idx))
				OBJ_SOILAGE(GET_EQ(ch, idx)) = 0;
		}
		for (cur_obj = ch->carrying;cur_obj;cur_obj = cur_obj->next_content)
			OBJ_SOILAGE(cur_obj) = 0;
		return 1;
	}
	send_to_char(ch,
		"The hot water runs over your naked body, refreshing you.\r\n");
	act("The hot water runs over $n's naked body.", FALSE, ch, 0, 0, TO_ROOM);
	GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20 * GET_LEVEL(ch));
	GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20 * GET_LEVEL(ch));
	WAIT_STATE(ch, 3 RL_SEC);
	return 1;
}
