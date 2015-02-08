//
// File: keymaster.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(keymaster)
{
    struct creature *keymaster = (struct creature *)me;
    struct obj_data *key = NULL;
    room_num v_home_pad = 1595, r_home_pad = NOWHERE;
    if (FIGHTING(keymaster)) {
        return 0;
    }
    if (!cmd && spinal->in_room != (r_home_pad = real_room(v_home_pad)) &&
        r_home_pad != NOWHERE && !FIGHTING(ch)) {
        act("$n unlocks a dimensional door and steps into it.", false,
            keymaster, 0, 0, TO_ROOM);
        char_from_room(keymaster, false);
        char_to_room(keymaster, r_home_pad, false);
        act("A dimensional door opens up in the room and out steps $n.",
            false, spinal, 0, 0, TO_ROOM);
        return 1;
    } else if (cmd || number(0, 13)) {
        return 0;
    }

    for (key = object_list; key; key = key->next) {
        if (!IS_OBJ_STAT2(key, ITEM2_BODY_PART) ||
            !CAN_WEAR(key, ITEM_TAKE) ||
            !isname("key", key->name) ||
            key->worn_by || (!key->carried_by && key->in_room == NOWHERE)) {
            if (key->next) {
                continue;
            } else {
                return 0;
            }
        } else {
            break;
        }
    }

    if (!key) {
        return 0;
    }
    if (!key->carried_by && key->carried_by != key) {
        if (key->carried_by->in_room != NOWHERE &&
            GET_LEVEL(key->carried_by) < LVL_IMMORT) {
            char_from_room(key, false);
            char_to_room(key, key->carried_by->in_room, false);
            act("A dimensional door opens up in the room and out steps $n.",
                false, key, 0, 0, TO_ROOM);
            act("$n grins at you and grabs $p.", false, keymaster, key,
                key->carried_by, TO_VICT);
            act("$n grins at $N and grabs $p.", false, keymaster, key,
                key->carried_by, TO_NOTVICT);
            act("$n eats $p.", false, keymaster, key, NULL, TO_ROOM);
            extract_obj(key);
            return 1;
        } else {
            return 0;
        }
    } else if (key->in_room != NOWHERE) {
        char_from_room(keymaster, false);
        char_to_room(keymaster, key->in_room, false);
        act("A dimensional door opens in the room and out steps $n.", false,
            keymaster, 0, 0, TO_ROOM);
        act("$n grins and grabs $p.", false, keymaster, key, key->carried_by,
            TO_ROOM);
        act("$n eats $p.", false, keymaster, key, NULL, TO_ROOM);
        extract_obj(key);
        return 1;
    } else {
        return 0;
    }
}
