//
// File: bearded_devil.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(bearded_devil)
{
  if( spec_mode == SPECIAL_DEATH ) return 0;
  if (cmd || !FIGHTING(ch) || !AWAKE(ch) || GET_MOB_WAIT(ch) > 0)
    return 0;

  if (!number(0, 3)) {
    act("$n thrusts $s wirelike beard at you!",
	FALSE, ch, 0, FIGHTING(ch), TO_VICT);
    act("$n thrusts $s wirelike beard at $N!",
	FALSE, ch, 0, FIGHTING(ch), TO_NOTVICT);
    if (GET_DEX(FIGHTING(ch)) > number(0, 25))
      damage(ch, FIGHTING(ch), 0, TYPE_RIP, WEAR_FACE);
    else
      damage(ch, FIGHTING(ch), dice(8, 8), TYPE_RIP, WEAR_FACE);
    WAIT_STATE(ch, 2 RL_SEC);
    return 1;
  }
  return 0;
}
