//
// File: mob_helper.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(mob_helper)
{
    struct creature *helpee = NULL;
    struct creature *vict = NULL;

    if (spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || is_fighting(ch))
        return 0;
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        helpee = it->data;
        // Being drawn into combat via a death cry will cause this
        // mob to attack a dead creature
        if (!is_fighting(helpee))
            continue;

        vict = random_opponent(helpee);
        if (GET_POSITION(vict) > POS_DEAD && IS_NPC(helpee)
            && IS_NPC(vict)
            && ((IS_GOOD(ch) && IS_GOOD(helpee)) ||
                (IS_EVIL(ch) && IS_EVIL(helpee)))
            && !number(0, 2)) {
            act("$n jumps to the aid of $N!", false, ch, NULL, helpee,
                TO_NOTVICT);
            hit(ch, vict, TYPE_UNDEFINED);
            return 1;
        }
    }
    return 0;
}
