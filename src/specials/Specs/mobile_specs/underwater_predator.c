//
// File: underwater_predator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(underwater_predator)
{

    struct creature *pred = (struct creature *)me;
    struct creature *vict = NULL;
    struct room_data *troom = NULL;
    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (GET_POSITION(pred) < POS_RESTING || !AWAKE(pred))
        return 0;

    if ((vict = random_opponent(pred))
        && SECT_TYPE(pred->in_room) != SECT_UNDERWATER
        && SECT_TYPE(pred->in_room) != SECT_DEEP_OCEAN && !number(0, 3)) {

        if (EXIT(pred, DOWN) &&
            ((troom = EXIT(pred, DOWN)->to_room) != NULL) &&
            (SECT_TYPE(troom) == SECT_UNDERWATER
                || SECT_TYPE(troom) == SECT_DEEP_OCEAN) &&
            !ROOM_FLAGGED(troom, ROOM_GODROOM | ROOM_DEATH | ROOM_PEACEFUL)) {

            if (GET_STR(pred) + number(1, 6) > GET_STR(vict)) {
                act("$n drags you under!!!", false, pred, NULL, vict, TO_VICT);
                act("$n drags $N under!!!", false, pred, NULL, vict, TO_NOTVICT);
                char_from_room(pred, false);
                char_to_room(pred, troom, false);
                char_from_room(vict, false);
                char_to_room(vict, troom, false);
                look_at_room(vict, vict->in_room, 0);

                act("$N is dragged, struggling,  in from above by $n!!",
                    false, pred, NULL, vict, TO_NOTVICT);

                WAIT_STATE(pred, PULSE_VIOLENCE);
                return 1;
            }
        }
    } else if (!is_fighting(pred) && NPC_FLAGGED(pred, NPC_AGGRESSIVE) &&
        (SECT_TYPE(pred->in_room) == SECT_UNDERWATER
            || SECT_TYPE(pred->in_room) == SECT_DEEP_OCEAN) &&
        EXIT(pred, UP) &&
        ((troom = EXIT(pred, UP)->to_room) != NULL) &&
        !ROOM_FLAGGED(troom, ROOM_NOMOB | ROOM_PEACEFUL |
            ROOM_DEATH | ROOM_GODROOM)) {
        for (GList * it = first_living(troom->people); it; it = next_living(it)) {
            vict = it->data;
            if ((IS_NPC(vict) && !NPC2_FLAGGED(pred, NPC2_ATK_MOBS)) ||
                (!IS_NPC(vict) && !vict->desc) ||
                PRF_FLAGGED(vict, PRF_NOHASSLE)
                || !can_see_creature(pred, vict)
                || PLR_FLAGGED(vict, PLR_WRITING)
                || GET_POSITION(vict) == POS_FLYING || (MOUNTED_BY(vict)
                    && GET_POSITION(MOUNTED_BY(vict)) == POS_FLYING))
                continue;

            act("$n cruises up out of sight with deadly intention.",
                true, pred, NULL, NULL, TO_ROOM);
            char_from_room(pred, false);

            char_to_room(pred, troom, false);
            act("$n emerges from the depths and attacks!!!",
                true, pred, NULL, NULL, TO_ROOM);

            hit(pred, vict, TYPE_UNDEFINED);
            return 1;
        }
    }
    return 0;
}
