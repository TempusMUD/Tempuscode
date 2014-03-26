//
// File: junker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_drop);

SPECIAL(junker)
{

    struct obj_data *obj = NULL, *next_obj = NULL;
    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (cmd || !AWAKE(ch) || is_fighting(ch))
        return 0;

    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (can_see_object(ch, obj) && !IS_OBJ_STAT(obj, ITEM_NODROP) &&
            !number(0, 2)) {
            do_drop(ch, fname(obj->aliases), 0, SCMD_JUNK);
            return 1;
        }
    }
    return 0;
}
