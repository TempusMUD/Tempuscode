// room special for the battlement of malagard

SPECIAL(malagard_lightning_room)
{
    struct creature *vict = 0;
    int retval = 0;
    GList *cit = 0;

    if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_ENTER)
        return 0;

    if (IS_NPC(ch) || number(0, 4))
        return 0;

    cit = first_living(ch->in_room->people);
    if (cit)
        vict = cit->data;

    if (vict == NULL || IS_NPC(vict))
        vict = ch;

    if (mag_savingthrow(vict, 50, SAVING_ROD)) {
        act("A bolt of lightning strikes nearby!", false, vict, 0, 0, TO_CHAR);
        act("A bolt of lightning strikes nearby!", false, vict, 0, 0, TO_ROOM);
    } else {
        act("A bolt of lightning blasts down from above and hits you!", false,
            vict, 0, 0, TO_CHAR);
        act("A bolt of lightning blasts down from above and hits $n!", false,
            vict, 0, 0, TO_ROOM);
        retval =
            damage(NULL, vict, NULL, dice(20, 20), SPELL_LIGHTNING_BOLT, WEAR_HEAD);
        return (ch == vict ? retval : 0);
    }

    return 0;
}
