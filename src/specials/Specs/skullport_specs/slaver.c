//
// File: slaver.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define SLAVE_PIT 22626
#define PIT_LIP   22945

SPECIAL(slaver)
{

    struct creature *slaver = (struct creature *)me;
    struct creature *vict = NULL;
    struct room_data *r_pit_lip = real_room(PIT_LIP),
    *r_slave_pit = real_room(SLAVE_PIT);

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (cmd || GET_POSITION(slaver) < POS_STANDING) {
        return 0;
    }
    if (!r_pit_lip || !r_slave_pit) {
        return 0;
    }

    if (!is_fighting(slaver) && !ROOM_FLAGGED(slaver->in_room, ROOM_PEACEFUL)) {

        for (GList *it = slaver->in_room->people; it; it = it->next) {
            vict = it->data;
            if (ch != vict && !IS_NPC(vict) && can_see_creature(slaver, vict)
                && PRF_FLAGGED(vict, PRF_NOHASSLE)
                && GET_LEVEL(vict) < number(1, 50) && !IS_EVIL(vict)) {
                perform_say(slaver, "say", "Ha!  You're coming with me!");

                if (slaver->in_room == r_pit_lip) {
                    act("$n hurls you headfirst into the slave pit!",
                        false, slaver, NULL, vict, TO_VICT);
                    act("$n hurls $N headfirst into the slave pit!",
                        false, slaver, NULL, vict, TO_NOTVICT);
                    remove_combat(slaver, vict);
                    remove_combat(vict, slaver);
                    char_from_room(vict, false);
                    char_to_room(vict, r_slave_pit, false);
                    look_at_room(vict, vict->in_room, 1);
                    act("$n is hurled into the pit from above!",
                        false, vict, NULL, NULL, TO_ROOM);
                    return 1;
                } else {
                    return (drag_char_to_jail(slaver, vict, r_pit_lip));
                }
            }
        }
        return 0;
    }

    if (!(vict = random_opponent(slaver)) ||
        PRF_FLAGGED(vict, PRF_NOHASSLE) || !can_see_creature(slaver, vict)) {
        return 0;
    }

    if (GET_NPC_WAIT(slaver) <= 5 && !number(0, 1)) {

        if (slaver->in_room == r_pit_lip) {
            act("$n hurls you headfirst into the slave pit!",
                false, slaver, NULL, vict, TO_VICT);
            act("$n hurls $N headfirst into the slave pit!",
                false, slaver, NULL, vict, TO_NOTVICT);
            remove_combat(slaver, vict);
            remove_combat(vict, slaver);
            char_from_room(vict, false);
            char_to_room(vict, r_slave_pit, false);
            look_at_room(vict, vict->in_room, 1);
            act("$n is hurled into the pit from above!",
                false, vict, NULL, NULL, TO_ROOM);
            return 1;
        } else {
            return (drag_char_to_jail(slaver, vict, r_pit_lip));
        }
    }

    return 0;
}
