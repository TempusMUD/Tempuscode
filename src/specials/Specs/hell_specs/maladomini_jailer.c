//
// File: maladomini_jailer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(maladomini_jailer)
{
    struct creature *vict = NULL;
    static struct room_data *to_room = NULL;
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }

    if (cmd || !is_fighting(ch)) {
        return 0;
    }

    vict = random_opponent(ch);
    if (!to_room) {
        if (!(to_room = real_room(16989))) {
            errlog("error loading to_room for maladomini_jailer.");
            ch->mob_specials.shared->func = NULL;
            REMOVE_BIT(NPC_FLAGS(ch), NPC_SPEC);
            return 0;
        }
    }

    if (ch->in_room->number != 16987) {
        return 0;

    }

    if (!number(0, 3)) {

        if (strength_carry_weight(GET_STR(ch)) + number(100, 500) <
            (GET_WEIGHT(vict) + IS_CARRYING_W(vict) + IS_WEARING_W(vict))) {
            act("$n attempts to lift you and hurl you into the chasm to the west!", false, ch, NULL, vict, TO_VICT);
            act("$n attempts to lift $N and hurl $M into the chasm to the west!", false, ch, NULL, vict, TO_NOTVICT);
            WAIT_STATE(ch, 2 RL_SEC);
            return 1;
        } else {
            act("** $n lifts you and hurls you into the chasm to the west! **",
                false, ch, NULL, vict, TO_VICT);
            act("$n lifts $N and hurls $M into the chasm to the west!", false,
                ch, NULL, vict, TO_NOTVICT);
            WAIT_STATE(ch, 2 RL_SEC);
            remove_all_combat(ch);
            char_from_room(vict, false);
            char_to_room(vict, to_room, false);
            act("$n is hurled in from the east!", false, vict, NULL, NULL, TO_ROOM);
            look_at_room(vict, to_room, 0);
            WAIT_STATE(vict, 3 RL_SEC);
            return 1;
        }

    }

    return 0;
}
