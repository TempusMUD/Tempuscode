//
// File: wand of wonder.spec   -- Part of TempusMUD
//
// Copyright 2007 by Daniel Lowe, all rights reserved.
//
SPECIAL(wand_of_wonder)
{
    if (spec_mode != SPECIAL_CMD || !CMD_IS("use"))
        return 0;
	obj_data *self = (struct obj_data *)me;

    // make sure it's held by the creature
    if (!self->worn_by || GET_EQ(self->worn_by, WEAR_HOLD) != self)
        return 0;

    // make sure it's a player holding it
    if (IS_NPC(self->worn_by))
        return 0;

    // make sure it's for the wand
    char *arg = tmp_getword(&argument);
    if (!isname(arg, self->aliases))
		return 0;
    
    // they're about to use it, randomize the spell and let pass
    // through
    int selected_spell = 0;
    int spell_count = 0;
    for (int spell_idx = 1;spell_idx < TOP_SPELL_DEFINE;spell_idx++)
        if (spell_info[spell_idx].routines && !number(0, spell_count++))
            selected_spell = spell_idx;
    slog("wand of wonder casting spell '%s'",
         strlist_aref(selected_spell, spells));

    GET_OBJ_VAL(self, 3) = selected_spell;

	return 0;
}
