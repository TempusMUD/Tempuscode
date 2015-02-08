//
// File: djinn_lamp.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(djinn_lamp)
{
    struct obj_data *lamp = (struct obj_data *)me;
    struct creature *djinn = NULL;
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }

    if ((!CMD_IS("rub") && !CMD_IS("clean"))) {
        return 0;
    }

    act("$n rubs $p.", false, ch, lamp, NULL, TO_ROOM);
    act("You rub $p.", false, ch, lamp, NULL, TO_CHAR);

    if ((djinn = read_mobile(5318))) {
        char_to_room(djinn, ch->in_room, false);
        act("$n appears in a cloud of swirling greenish mist.", false, djinn,
            NULL, NULL, TO_ROOM);
        add_follower(djinn, ch);
        SET_BIT(AFF_FLAGS(djinn), AFF_CHARM);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        extract_obj(lamp);
        return 1;
    }
    return 0;
}
