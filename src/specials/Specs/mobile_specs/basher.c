//
// File: basher.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(basher)
{
    struct creature *vict = NULL;
    ACMD(do_bash);
    if (spec_mode != SPECIAL_TICK)
        return false;
    if (GET_POSITION(ch) != POS_FIGHTING || !ch->fighting)
        return 0;

    if (number(0, 81) > GET_LEVEL(ch))
        return 0;

    for (GList * it = ch->fighting; it; it = it->next) {
        struct creature *tch = it->data;
        if (IS_MAGE(tch) && number(0, 1)) {
            vict = tch;
            break;
        }
    }

    if (vict == NULL)
        vict = random_opponent(ch);

    do_bash(ch, tmp_strdup(""), 0, 0, 0);
    return 1;
}
