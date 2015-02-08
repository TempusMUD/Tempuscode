//
// File: stygian_lightning_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stygian_lightning_rm)
{
    struct creature *new_vict = NULL;

    if (spec_mode != SPECIAL_ENTER && spec_mode != SPECIAL_TICK) {
        return 0;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT || IS_DEVIL(ch) || IS_NPC(ch)) {
        return 0;
    }

    if (number(0, 100) < GET_LEVEL(ch) + GET_DEX(ch)) {
        return 0;
    }

    new_vict = ch;
    for (GList *cit = ch->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (!IS_NPC(tch) && GET_POSITION(tch) > POS_SITTING
            && !IS_DEVIL(tch) && GET_LEVEL(tch) > GET_LEVEL(new_vict)
            && GET_LEVEL(tch) < LVL_IMMORT && !number(0, 3)) {
            new_vict = tch;
        }
    }
    if (!new_vict) {
        new_vict = ch;
    }

    if (mag_savingthrow(new_vict, 50, SAVING_ROD)) {
        if (number(0, 1)) {
            act("A bolt of lightning streaks from the sky and blasts into"
                " the ground!", false, ch, NULL, NULL, TO_ROOM);
            act("A bolt of lightning streaks from the sky and blasts into"
                " the ground!", false, ch, NULL, NULL, TO_CHAR);
        } else {
            act("A bolt of lightning falls from the sky, narrowly missing you!", false, ch, NULL, NULL, TO_CHAR);
            act("A bolt of lightning falls from the sky, narrowly missing $n!",
                false, ch, NULL, NULL, TO_ROOM);
        }
    } else {
        return damage(new_vict, new_vict, NULL, dice(12, 10), TYPE_STYGIAN_LIGHTNING,
                      WEAR_BODY);
    }
    return 0;
}
