//
// File: hell_hound.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(hell_hound)
{
	if (cmd)
		return 0;
	if (spec_mode != SPECIAL_COMBAT && spec_mode != SPECIAL_ENTER)
		return 0;
	if (FIGHTING(ch) && !number(0, 6))
		damage(ch, FIGHTING(ch),
			mag_savingthrow(FIGHTING(ch), GET_LEVEL(ch), SAVING_BREATH) ?
			(GET_LEVEL(ch) >> 1) : GET_LEVEL(ch), SPELL_FIRE_BREATH, -1);
	else
		return 0;
	return 1;
}
