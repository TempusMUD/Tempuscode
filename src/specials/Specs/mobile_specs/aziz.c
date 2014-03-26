//
// File: aziz.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_bash);

SPECIAL(Aziz)
{
    struct creature *vict = NULL;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (!is_fighting(ch))
        return 0;

    /* pseudo-randomly choose a mage in the room who is fighting me */
    for (GList *it = first_living(ch->fighting); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (IS_MAGE(tch) && number(0, 1)) {
            vict = tch;
            break;
        }
    }

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = random_opponent(ch);

    do_bash(ch, tmp_strdup(""), 0, 0);

    return 0;
}
