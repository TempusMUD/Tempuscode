//
// File: kata.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(kata)
{
  struct Creature *kata = (struct Creature *) me;
  ACMD(do_say);
  char buf[MAX_STRING_LENGTH];
  
  if (kata->master || IS_AFFECTED(kata, AFF_CHARM))
    return 0;

  if (!CMD_IS("rescue"))
    return 0;

  skip_spaces(&argument);

  if (!isname(argument, kata->player.name))
    return 0;

  if (IS_EVIL(ch) && IS_GOOD(kata)) {
    do_say(kata, "No! I will not follow you.", 0, 0);
    return 1;
  }

  sprintf(buf, "Thank you for rescuing me, %s!  I will be a loyal companion.", GET_NAME(ch));
  do_say(kata, buf, 0, 0);
  add_follower(kata, ch);
  SET_BIT(AFF_FLAGS(kata), AFF_CHARM);
  return 1;
}
