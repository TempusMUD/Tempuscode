//
// File: newbie_fly.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_fly)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || ch->fighting)
        return 0;
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (AFF_FLAGGED(tch, AFF_INFLIGHT) || !can_see_creature(ch, tch))
            continue;
        cast_spell(ch, tch, 0, NULL, SPELL_FLY);
        return 1;
    }
    return 0;
}
