//
// File: spinal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(spinal)
{
    struct creature *spinal = (struct creature *)me;
    struct obj_data *spine = NULL;
    struct room_data *r_home_pad = real_room(1595);

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;

    if (is_fighting(spinal))
        return 0;

    if (!cmd && spinal->in_room != r_home_pad &&
        r_home_pad != NULL && !is_fighting(ch)) {
        act("$n departs suddenly into the deeper ethereal.",
            false, spinal, NULL, NULL, TO_ROOM);
        char_from_room(spinal, false);
        char_to_room(spinal, r_home_pad, false);
        spinal->in_room->zone->idle_time = 0;

        act("$n steps in from another plane of existence.",
            false, spinal, NULL, NULL, TO_ROOM);
        return 1;
    } else if (cmd) {
        return 0;
    }

    for (spine = object_list; spine; spine = spine->next) {
        if (!IS_OBJ_STAT2(spine, ITEM2_BODY_PART) ||
            !CAN_WEAR(spine, ITEM_WEAR_WIELD) ||
            !isname("spine", spine->aliases) ||
            spine->worn_by || (!spine->carried_by && spine->in_room == NULL)) {
            if (spine->next) {
                continue;
            } else {
                return 0;
            }
        } else
            break;
    }

    if (!spine)
        return 0;

    if (spine->carried_by && spine->carried_by != spinal) {
        if (spine->carried_by->in_room != NULL &&
            GET_LEVEL(spine->carried_by) < LVL_IMMORT) {
            char_from_room(spinal, false);
            char_to_room(spinal, spine->carried_by->in_room, false);
            spinal->in_room->zone->idle_time = 0;

            act("$n steps in from the ethereal plane.",
                false, spinal, NULL, NULL, TO_ROOM);
            act("$n grins at you and grabs $p.",
                false, spinal, spine, spine->carried_by, TO_VICT);
            act("$n grins at $N and grabs $p.",
                false, spinal, spine, spine->carried_by, TO_NOTVICT);
            act("$n eats $p.", false, spinal, spine, NULL, TO_ROOM);
            extract_obj(spine);
            return 1;
        } else {
            return 0;
        }
    } else if (spine->in_room != NULL) {
        char_from_room(spinal, false);
        char_to_room(spinal, spine->in_room, false);
        spinal->in_room->zone->idle_time = 0;

        act("$n steps in from the ethereal plane.", false, spinal, NULL, NULL,
            TO_ROOM);
        act("$n grins and grabs $p.", false, spinal, spine, spine->carried_by,
            TO_ROOM);
        act("$n eats $p.", false, spinal, spine, NULL, TO_ROOM);
        extract_obj(spine);
        return 1;
    } else {
        return 0;
    }
}
