//
// File: fountain_youth.spec
//
// Copyright 2001 by Daniel Carruth
//
// Part of TempusMUD:
// Copyright 1998 by John Watson, all rights reserved
//
/***************************************************/

SPECIAL(fountain_youth)
{
	/* applies the affects of youth to anyone
	   drinking from the fountain */
	/* Obj Vnum is 23315 */

	struct affected_type af, af1, af2, af3;
	struct obj_data *fountain = (struct obj_data *)me;

	if (!CMD_IS("drink"))
		return 0;

	if (ch->getPosition() == POS_SLEEPING) {
		send_to_char(ch, "You'll have to wake up first!\r\n");
		return 1;
	}

	/* check here for argument */
	skip_spaces(&argument);
	if (!isname(argument, fountain->name))
		return 0;

	if (affected_by_spell(ch, SPELL_YOUTH)) {
		act("$n drinks from $p.", TRUE, ch, fountain, 0, TO_ROOM);
		return 1;
	}

	af.type = SPELL_YOUTH;
	af.duration = 60;
	af.location = APPLY_AGE;
	af.modifier = -(GET_AGE(ch) / 4);
	af.aff_index = 0;
	af.bitvector = 0;
	af.level = 1;
	af.is_instant = FALSE;
	affect_to_char(ch, &af);

	af1.type = SPELL_YOUTH;
	af1.duration = 60;
	af1.location = APPLY_CON;
	af1.modifier = 3;
	af1.aff_index = 0;
	af1.bitvector = 0;
	af.level = 1;
	af.is_instant = FALSE;
	affect_to_char(ch, &af1);

	af2.type = SPELL_YOUTH;
	af2.duration = 60;
	af2.location = APPLY_STR;
	af2.modifier = 2;
	af2.aff_index = 0;
	af2.bitvector = 0;
	af.level = 1;
	af.is_instant = FALSE;
	affect_to_char(ch, &af2);

	af3.type = SPELL_YOUTH;
	af3.duration = 60;
	af3.location = APPLY_WIS;
	af3.modifier = -4;
	af3.aff_index = 0;
	af3.bitvector = 0;
	af.level = 1;
	af.is_instant = FALSE;
	affect_to_char(ch, &af3);

	act("As $n drinks from the pool, $e becomes visibly younger!", TRUE, ch, 0,
		ch, TO_ROOM);
	return 1;
}
