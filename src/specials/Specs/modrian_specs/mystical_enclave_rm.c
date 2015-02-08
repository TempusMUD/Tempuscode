//
// File: mystical_enclave_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(mystical_enclave)
{
    struct room_data *stair_rnum = NULL;

    if (!CMD_IS("up") || !IS_PSYCHIC(ch) || GET_POSITION(ch) < POS_STANDING ||
        MOUNTED_BY(ch)) {
        return 0;
    }

    if ((stair_rnum = real_room(30126)) == NULL) {
        return 0;
    }

    act("$n vanishes into the ceiling...", true, ch, NULL, NULL, TO_ROOM);
    char_from_room(ch, false);
    char_to_room(ch, stair_rnum, false);
    send_to_char(ch, "You leave upwards, into the future...\r\n");
    look_at_room(ch, ch->in_room, 0);
    act("$n arrives, climbing up from the past...\r\n", true, ch, NULL, NULL,
        TO_ROOM);
    return 1;
}
