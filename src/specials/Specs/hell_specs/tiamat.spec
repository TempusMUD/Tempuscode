//
// File: tiamat.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(tiamat)
{

  int type = 0;
  struct room_data *lair = real_room(16182);

  if (cmd || !FIGHTING(ch) || GET_MOB_WAIT(ch) || number(0, 4))
    return 0;
  
  if (GET_HIT(ch) < 200 && ch->in_room != lair && lair != NULL) {
    act("$n vanishes in a prismatic blast of light!",
	FALSE, ch, 0, 0, TO_ROOM);
    stop_fighting(FIGHTING(ch));
    stop_fighting(ch);
    char_from_room(ch);
    char_to_room(ch, lair);
    act("$n appears in a prismatic blast of light!",
	FALSE, ch, 0, 0, TO_ROOM);
    return 1;
  }
  switch (number(0, 4)) {
  case 0:
    type = SPELL_GAS_BREATH;
    break;
  case 1:
    type = SPELL_ACID_BREATH;
    break;
  case 2:
    type = SPELL_LIGHTNING_BREATH;
    break;
  case 3:
    type = SPELL_FROST_BREATH;
    break;
  case 4:
    type = SPELL_FIRE_BREATH;
    break;
  }
  
  call_magic(ch, FIGHTING(ch), 0, type, GET_LEVEL(ch), SAVING_BREATH);
  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  return 1;
}



