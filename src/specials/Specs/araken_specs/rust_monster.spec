//
// File: rust_monster.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(rust_monster)
{

	struct obj_data *obj = NULL;
	int i, count = 0;

	if (cmd)
		return 0;
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;

	if (!ch->numCombatants()) {

		for (obj = ch->in_room->contents; obj; obj = obj->next_content) {

			if (IS_MAT(obj, MAT_RUST)) {

				act("$n hungrily devours $p.", TRUE, ch, obj, 0, TO_ROOM);
				extract_obj(obj);
				GET_HIT(ch) = MAX(GET_MAX_HIT(ch), GET_HIT(ch) + 10);
				return 1;
			}

			if (IS_FERROUS(obj)) {

				act("$n flays $p with $s antennae.", TRUE, ch, obj, 0,
					TO_ROOM);
				if ((!IS_OBJ_STAT(obj, ITEM_MAGIC | ITEM_MAGIC_NODISPEL)
						|| mag_savingthrow(ch, 40, SAVING_ROD))) {

					act("$p spontaneously oxidizes and crumbles into a pile of rust!", FALSE, ch, obj, 0, TO_ROOM);

					extract_obj(obj);

					if ((obj = read_object(RUSTPILE)))
						obj_to_room(obj, ch->in_room);

					WAIT_STATE(ch, 2 RL_SEC);

					return 1;
				}
				return 1;
			}
		}
		return 0;
	}


	while (count < 35) {

		i = number(2, NUM_WEARS - 2);
		count++;

        Creature *vict = ch->findRandomCombat();
		if ((!(obj = GET_EQ(vict, i)) &&
				!(obj = GET_EQ(vict, i - 1)) &&
				!(obj = GET_EQ(vict, i + 1)) &&
				!(obj = GET_EQ(vict, i - 2)) &&
				!(obj = GET_EQ(vict, i + 2))) ||
			!IS_FERROUS(obj) || !number(0, 2))
			continue;

		act("$n flays $p with $s antennae.", TRUE, ch, obj, vict,
			TO_VICT);
		act("$n flays $N with $s antennae.", TRUE, ch, obj, vict,
			TO_NOTVICT);

		if ((!IS_OBJ_STAT(obj, ITEM_MAGIC) ||
				mag_savingthrow(ch, 40, SAVING_ROD)) &&
			(!IS_OBJ_STAT(obj, ITEM_MAGIC_NODISPEL) ||
				mag_savingthrow(ch, 40, SAVING_ROD))) {

			act("$p spontaneously oxidizes and crumbles into a pile of rust!",
				FALSE, ch, obj, 0, TO_ROOM);

			extract_obj(obj);

			if ((obj = read_object(RUSTPILE)))
				obj_to_room(obj, ch->in_room);

			WAIT_STATE(ch, 2 RL_SEC);

		}
		return 1;
	}
	return 0;
}
