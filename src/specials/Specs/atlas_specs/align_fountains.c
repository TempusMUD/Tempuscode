//
// File: align_fountains.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fountain_good)
{
    struct obj_data *obj = me;

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    skip_spaces(&argument);     // make sure they want to drink from 'me'.

    if (!CMD_IS("drink") || *argument || !isname(argument, obj->aliases)) {
        return 0;
    }

    act("$n drinks from $p.", true, ch, obj, NULL, TO_ROOM);
    WAIT_STATE(ch, 2 RL_SEC);   // don't let them spam drink
    call_magic(ch, ch, NULL, NULL, SPELL_ESSENCE_OF_GOOD, 25, CAST_SPELL);
    // everything you need is handled in call_magic(), damage, etc...
    return 1;
}

SPECIAL(fountain_evil)
{

    struct obj_data *obj = (struct obj_data *)me;

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    skip_spaces(&argument);
    if (!CMD_IS("drink") || *argument || !isname(argument, obj->aliases)) {
        return 0;
    }

    if (GET_ALIGNMENT(ch) <= -1000) {
        send_to_char(ch, "You are already burning with evil.\r\n");
        return 0;
    }

    act("$n drinks from $p.", true, ch, obj, NULL, TO_ROOM);
    WAIT_STATE(ch, 2 RL_SEC);
    call_magic(ch, ch, NULL, NULL, SPELL_ESSENCE_OF_EVIL, 25, CAST_SPELL);

    return 1;
}
