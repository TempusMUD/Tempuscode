//
// File: aziz.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_bash);

SPECIAL(Aziz)
{
  struct char_data *vict = NULL;

  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
  if (cmd || ch->getPosition() != POS_FIGHTING)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  /* pseudo-randomly choose a mage in the room who is fighting me */
CharacterList::iterator it = ch->in_room->people.begin();
for( ; it != ch->in_room->people.end(); ++it ) {
    if (FIGHTING((*it)) == ch && !number(0, 2) && IS_MAGE((*it))){
        vict = *it;
      break;
    }
}

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  do_bash(ch, "", 0, 0);

  return 0;
}

