//
// File: moloch.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(moloch)
{

    struct creature *moloch = (struct creature *)me;
    struct creature *vict = NULL;
    struct room_data *targ_room = NULL;
    int throne_rooms[4] = { 16645, 16692, 16692, 16623 }, index;

    if (cmd)
        return 0;
    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (is_fighting(moloch) && GET_NPC_WAIT(moloch) <= 0) {
        vict = random_opponent(moloch);
        if (!number(0, 10)) {
            call_magic(moloch, vict, NULL, NULL, SPELL_FLAME_STRIKE, 50,
                CAST_BREATH);
            return 1;
        } else if (!number(0, 10)) {
            call_magic(moloch, vict, NULL, NULL, SPELL_BURNING_HANDS, 50,
                CAST_SPELL);
            return 1;
        } else if (!number(0, 8) && GET_DEX(vict) < number(10, 25)) {
            act("$n picks you up in $s jaws and flails you around!!",
                false, moloch, NULL, vict, TO_VICT);
            act("$n picks $N up in $s jaws and flails $M around!!",
                false, moloch, NULL, vict, TO_NOTVICT);
            damage(moloch, vict, NULL, dice(30, 29), TYPE_RIP, WEAR_BODY);
            WAIT_STATE(moloch, 7 RL_SEC);
            return 1;
        }
        return 0;
    }

    if (is_fighting(moloch) || number(0, 20))
        return 0;

    if (ch->in_room->number == throne_rooms[(index = number(0, 3))])
        return 0;

    if (!(targ_room = real_room(throne_rooms[index])))
        return 0;

    act("A devilish voice rumbles from all around, in an unknown language.",
        false, moloch, NULL, NULL, TO_ROOM);
    act("$n slowly fades from existence.", true, moloch, NULL, NULL, TO_ROOM);
    char_from_room(moloch, false);
    char_to_room(moloch, targ_room, false);
    act("$n slowly appears from another place.", true, moloch, NULL, NULL, TO_ROOM);
    index = 0;
    for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (tch != moloch) {
            if (!IS_DEVIL(tch)) {
                index = 1;
            } else {
                index = 0;
                break;
            }
        }
    }

    if (index)
        cast_spell(moloch, NULL, NULL, NULL, SPELL_METEOR_STORM);

    return 1;

}
