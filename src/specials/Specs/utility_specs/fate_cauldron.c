//
// File: fate_cauldron.spec                     -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//

SPECIAL(fate_cauldron)
{
    struct obj_data *pot = (struct obj_data *)me;
    char arg1[MAX_INPUT_LENGTH];
    register struct creature *fate = NULL;
    int fateid = 0;

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }
    if (!CMD_IS("look") || !can_see_object(ch, pot) || !AWAKE(ch)) {
        return 0;
    }
    one_argument(argument, arg1);
    if (!isname(arg1, pot->aliases)) {
        return 0;
    }

    act("$n gazes deeply into $p.", false, ch, pot, NULL, TO_ROOM);
    // Is he ready to remort? Or level 49 at least?
    if (GET_LEVEL(ch) != 49 && GET_LEVEL(ch) < LVL_SPIRIT) {
        act("You gaze deep into $p but learn nothing.",
            false, ch, pot, NULL, TO_CHAR);
        return 1;
    }
    // Which fate does this guy need.
    if (GET_REMORT_GEN(ch) < 4) {
        fateid = FATE_VNUM_LOW;
    } else if (GET_REMORT_GEN(ch) < 7) {
        fateid = FATE_VNUM_MID;
    } else {
        fateid = FATE_VNUM_HIGH;
    }
    for (GList *it = first_living(creatures); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (GET_NPC_VNUM(tch) == fateid) {
            fate = tch;
            break;
        }
    }
    act("You gaze deep into $p.", false, ch, pot, NULL, TO_CHAR);

    // Couldn't find her
    if (!fate || !fate->in_room) {
        return 1;
    }
    // Yay. Finally found her!
    look_at_room(ch, fate->in_room, 1);
    WAIT_STATE(ch, 3);
    return 1;
}
