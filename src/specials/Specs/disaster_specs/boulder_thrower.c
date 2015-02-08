//
// File: boulder_thrower.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

bool
THROW_OK(struct obj_data *obj)
{
    if (!CAN_WEAR(obj, ITEM_WEAR_TAKE) ||
        !IS_OBJ_TYPE(obj, ITEM_WEAPON) ||
        !IS_THROWN(obj) ||
        IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) {
        return 0;
    }

    return 1;
}

ACMD(do_throw);

SPECIAL(boulder_thrower)
{

    struct obj_data *obj = NULL, *tmp_obj = NULL;

    if (spec_mode != SPECIAL_TICK) {
        return 0;
    }

    if (cmd || !AWAKE(ch)) {
        return 0;
    }

    struct creature *vict = random_opponent(ch);
    if (vict && can_see_creature(ch, vict)) {
        // are we wielding a throwable weapon?
        if ((obj = GET_EQ(ch, WEAR_WIELD)) && THROW_OK(obj)) {
            tmp_obj = obj;
        }
        // are we holding a throwable weapon?
        else if ((obj = GET_EQ(ch, WEAR_HOLD)) && THROW_OK(obj)) {
            tmp_obj = obj;
        }
        // do we have on in the inventory?
        else {
            obj = NULL;
            for (tmp_obj = obj = ch->carrying; obj; obj = obj->next_content) {
                if (THROW_OK(obj) &&
                    (GET_OBJ_WEIGHT(obj) > GET_OBJ_WEIGHT(tmp_obj) ||
                     !THROW_OK(tmp_obj))) {
                    tmp_obj = obj;
                }
            }
        }
        obj = tmp_obj;

        // object found, throw it
        if (obj) {

            char *str = tmp_sprintf("%s %s",
                                    fname(obj->aliases),
                                    fname(vict->player.name));
            do_throw(ch, str, 0, 0);
            return 1;
        }
    }
    // look for a boulder to pick up

    // arms already full
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
        return 0;
    }

    // already over-encumbered
    if (IS_CARRYING_W(ch) + IS_WEARING_W(ch) > (CAN_CARRY_W(ch) / 2)) {
        return 0;
    }

    // look for something in the room to pick up
    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if (THROW_OK(obj) &&
            GET_OBJ_WEIGHT(obj) < (CAN_CARRY_W(ch) -
                                   IS_CARRYING_W(ch) - IS_WEARING_W(ch))) {
            act("$n picks up $p.", true, ch, obj, NULL, TO_ROOM);
            obj_from_room(obj);
            obj_to_char(obj, ch);
            return 1;
        }
    }

    return 0;
}
