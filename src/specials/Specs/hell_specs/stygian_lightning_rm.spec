//
// File: stygian_lightning_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stygian_lightning_rm)
{
    struct char_data *new_vict = NULL;

    if (GET_LEVEL(ch) >= LVL_IMMORT || IS_DEVIL(ch) || IS_NPC(ch))
        return 0;

    if (number(0, 100) < GET_LEVEL(ch) + GET_DEX(ch))
        return 0;

    new_vict = ch;
    CharacterList::iterator it = ch->in_room->people.begin();
    for( ; it != ch->in_room->people.end(); ++it ) {
        if (!IS_NPC((*it)) && (*it)->getPosition() > POS_SITTING && !IS_DEVIL((*it)) &&
            GET_LEVEL((*it)) > GET_LEVEL(new_vict) && GET_LEVEL((*it)) < LVL_IMMORT && !number(0, 3))
            new_vict = *it;
    }
    if (!new_vict)
        new_vict = ch;
  
    if (mag_savingthrow(new_vict, 50, SAVING_ROD)) {
        if (number(0, 1)) {
            act("A bolt of lightning streaks from the sky and blasts into"
                " the ground!",  FALSE, ch, 0, 0, TO_ROOM);
            act("A bolt of lightning streaks from the sky and blasts into"
                " the ground!", FALSE, ch, 0, 0, TO_CHAR);
        } else {
            act("A bolt of lightning falls from the sky, narrowly missing you!",
                FALSE, ch, 0, 0, TO_CHAR);
            act("A bolt of lightning falls from the sky, narrowly missing $n!",
                FALSE, ch, 0, 0, TO_ROOM);
        }
    } else {
        return damage(new_vict, new_vict, dice(12, 10), TYPE_STYGIAN_LIGHTNING, WEAR_BODY);
    }
    return 0;
}


