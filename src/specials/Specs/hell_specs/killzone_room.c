// room special for gatehouse of Malagard

SPECIAL(killzone_room)
{
    struct room_data *uproom = 0;
    struct creature *devil = 0, *vict = 0;
    GList *cit;
    int retval = 0;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;

    if (IS_NPC(ch) || number(0, 4))
        return 0;

    if (ch->in_room->number == 17001)
        uproom = real_room(17007);
    else
        uproom = real_room(17008);

    if (!uproom)
        return 0;

    // mix up the victims a bit so it doesnt feel like commands are triggering it as much
    cit = first_living(ch->in_room->people);
    if (!cit)
        return 0;
    vict = cit->data;

    for (cit = first_living(uproom->people); cit; cit = next_living(cit)) {
        devil = cit->data;
        if (IS_NPC(devil) && IS_DEVIL(devil)) {

            int is_miss = mag_savingthrow(vict, GET_LEVEL(devil), SAVING_ROD)
                && random_binary();

            switch (number(0, 3)) {
            case 0:            // lightning bolt
                act("$n fires a bolt of lightning down through the murder holes!", false, devil, 0, 0, TO_ROOM);

                if (is_miss) {
                    act("A bolt of lightning blasts down from above, barely missing you!", false, vict, 0, 0, TO_ROOM);
                    act("A bolt of lightning blasts down from above, barely missing you!", false, vict, 0, 0, TO_CHAR);
                } else {
                    act("A bolt of lightning blasts down from above and hits you!", false, vict, 0, 0, TO_CHAR);
                    act("A bolt of lightning blasts down from above and hits $n!", false, vict, 0, 0, TO_ROOM);
                    retval =
                        damage(NULL, vict, NULL, dice(20, 20), SPELL_LIGHTNING_BOLT,
                        WEAR_HEAD);

                    return (ch == vict ? retval : 0);
                }
                break;
            case 1:            // fireball
                act("$n sends a fireball hurtling downward through a murder hole!", false, devil, 0, 0, TO_ROOM);

                if (is_miss) {
                    act("A fireball comes hurtling in from above and explodes on the floor nearby!", false, vict, 0, 0, TO_ROOM);
                    act("A fireball comes hurtling in from above and explodes on the floor nearby!", false, vict, 0, 0, TO_CHAR);
                } else {
                    act("A fireball comes hurtling in from above and slams into $n!", false, vict, 0, 0, TO_ROOM);
                    act("A fireball comes hurtling in from above, explosively slamming into you!", false, vict, 0, 0, TO_CHAR);
                    retval =
                        damage(NULL, vict, NULL, dice(25, 20), SPELL_FIREBALL,
                        WEAR_HEAD);
                    return (ch == vict ? retval : 0);

                }
                break;
            case 2:            // burning javelins
                act("$n sends a rain of burning javelins downward through a murder hole!", false, devil, 0, 0, TO_ROOM);

                if (is_miss) {
                    act("A rain of burning javelins flies in from above, barely missing you!", false, vict, 0, 0, TO_ROOM);
                    act("A rain of burning javelins flies in from above, barely missing you!", false, vict, 0, 0, TO_CHAR);
                } else {
                    act("A rain of burning javelins flies in from above onto $n!", false, vict, 0, 0, TO_ROOM);
                    act("A rain of burning javelins flies in from above, puncturing you mercilessly!", false, vict, 0, 0, TO_CHAR);
                    retval =
                        damage(NULL, vict, NULL, dice(25, 20), TYPE_PIERCE,
                        WEAR_HEAD);
                    return (ch == vict ? retval : 0);

                }
                break;

            default:           // boiling pitch
                act("$n tips a cauldron pours boiling pitch into the Killzone below!", false, devil, 0, 0, TO_ROOM);

                if (is_miss) {
                    act("A torrent of boiling pitch comes pouring in from above as you leap aside!", false, vict, 0, 0, TO_ROOM);
                    act("A torrent of boiling pitch comes pouring in from above as you leap aside!", false, vict, 0, 0, TO_CHAR);
                } else {
                    act("A torrent of boiling pitch pours onto $n from above!",
                        false, vict, 0, 0, TO_ROOM);
                    act("A torrent of boiling pitch pours onto you from above!!", false, vict, 0, 0, TO_CHAR);
                    retval =
                        damage(NULL, vict, NULL, dice(25, 20), TYPE_BOILING_PITCH,
                        WEAR_HEAD);
                    return (ch == vict ? retval : 0);

                }
                break;
            }
            return 0;
        }

    }

    return 0;
}
