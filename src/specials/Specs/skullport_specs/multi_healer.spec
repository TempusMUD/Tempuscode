//
// File: multi_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// Lady Sarah - Skullport Pt II
#define SARAH 22803
// Skullport Healer
#define DARK  22807
// Dr Cheetah - Family Clan Healer
#define CHEETA 74003

#define MOBV(mode)  (GET_MOB_VNUM(ch) == mode)

#define MODE_OK(vict) \
                (MOBV(CHEETA) || ((MOBV(SARAH) && IS_GOOD(vict) && IS_HUMAN(vict)) ? 1 : \
		 (MOBV(DARK) && !IS_GOOD(vict)) ? 1 : 0))

SPECIAL(multi_healer)
{

  struct char_data *vict = NULL;
  int found = 0;

  if (cmd)
    return FALSE;

  for (vict = ch->in_room->people; vict && !found; 
       vict = vict->next_in_room) {
    if (ch == vict || IS_NPC(vict) || !number(0, 2) || !CAN_SEE(ch, vict))
      continue;
    if (!MODE_OK(vict))
      continue;

    if (IS_POISONED(vict))
      cast_spell(ch, vict, 0, SPELL_REMOVE_POISON);
    else if (IS_SICK(vict))
      cast_spell(ch, vict, 0, SPELL_REMOVE_SICKNESS);
    else if (GET_HIT(vict) < GET_MAX_HIT(vict)) {
      cast_spell(ch, vict, 0,
		 GET_LEVEL(vict) <= 10 ? SPELL_CURE_LIGHT :
		 GET_LEVEL(vict) <= 20 ? SPELL_CURE_CRITIC :
		 GET_LEVEL(vict) <= 30 ? SPELL_HEAL : SPELL_GREATER_HEAL);
    }
    
    else if (affected_by_spell(vict, SPELL_BLINDNESS) || 
	     affected_by_spell(vict, SKILL_GOUGE))
      cast_spell(ch, vict, 0, SPELL_CURE_BLIND);
    else
      continue;
    
    return 1;
  }
  return 0;
}
