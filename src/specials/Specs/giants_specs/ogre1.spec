//
// File: ogre1.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(ogre1)
{
  if (cmd || FIGHTING(ch))
    return 0;
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
    CreatureList::iterator it = ch->in_room->people.begin();
    for( ; it != ch->in_room->people.end(); ++it ) {
        if (IS_ORC((*it)) && CAN_SEE(ch, (*it))) {
            act("$n roars, 'Now I've got $N, you!", FALSE, ch, 0, (*it), TO_ROOM);
            hit(ch, (*it), TYPE_UNDEFINED);
            return 1;
        }
    }
  return 0;
}
