//
// File: wagon_driver.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(wagon_driver)
{
    struct obj_data *i;
    struct creature *driver = (struct creature *)me;
    struct room_data *destination = NULL;
    int wagon_obj_rnum = 10, dir;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || ch->in_room->number != 10)
        return 0;

    if (!number(0, 7))          /* Only try to some of the time */
        return 0;

    for (i = object_list; i; i = i->next)
        if ((wagon_obj_rnum == GET_OBJ_VNUM(i)) && (i->in_room != NULL))
            break;

    if (!i) {
        mudlog(LVL_DEMI, BRF, true,
            "WARNING:  Wagon driver cannot find his wagon (object 10)!");
        driver->mob_specials.shared->func = NULL;
        REMOVE_BIT(NPC_FLAGS(driver), NPC_SPEC);
        return 0;
    }

    for (dir = 0; dir <= 3; dir++) {
        if (OEXIT(i, dir) && OEXIT(i, dir)->to_room != NULL &&
            (i->in_room->sector_type ==
                OEXIT(i, dir)->to_room->sector_type) && OCAN_GO(i, dir)
            && !number(0, 4)
            && !ROOM_FLAGGED(OEXIT(i, dir)->to_room, ROOM_INDOORS))
            break;
    }
    if ((dir == 4) || !OCAN_GO(i, dir))
        return 0;
    else {
        destination = OEXIT(i, dir)->to_room;

        act("$n twitches the reins.", true, driver, NULL, NULL, TO_ROOM);
        act("The wagon driver twitches the reins.", true, NULL, i, NULL, TO_ROOM);
        sprintf(buf, "$p rolls off to the %s.", to_dirs[dir]);
        act(buf, true, NULL, i, NULL, TO_ROOM);

        obj_from_room(i);
        obj_to_room(i, destination);

        sprintf(buf, "$p drives in from %s.", from_dirs[dir]);
        act(buf, false, NULL, i, NULL, TO_ROOM);

        return 1;
    }
    return 0;
}
