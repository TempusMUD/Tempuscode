//
// File: ogre1.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(ogre1)
{
    if (cmd || is_fighting(ch))
        return 0;
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (IS_ORC(tch) && can_see_creature(ch, tch)) {
            act("$n roars, 'Now I've got $N, you!", false, ch, NULL, tch,
                TO_ROOM);
            hit(ch, tch, TYPE_UNDEFINED);
            return 1;
        }
    }
    return 0;
}
