SPECIAL(pit_keeper)
{
    struct creature *vict = 0;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd)
        return 0;

    if (!(ch->fighting))
        return 0;

    vict = random_opponent(ch);
    if (ch->in_room->number != 17027 &&
        ch->in_room->number != 17021 &&
        ch->in_room->number != 17022 &&
        ch->in_room->number != 17026 && ch->in_room->number != 17020)
        return 0;

    if (!ch->in_room->dir_option[DOWN]
        || !ch->in_room->dir_option[DOWN]->to_room)
        return 0;

    if (GET_STR(ch) + number(0, 10) > GET_STR(vict)) {
        act("$n lifts you over his head and hurls you into the pit below!",
            false, ch, 0, vict, TO_VICT);
        act("$n lifts $N and hurls $M into the pit below!", false, ch, 0, vict,
            TO_NOTVICT);
        WAIT_STATE(ch, 3 RL_SEC);
        remove_combat(ch, vict);
        remove_combat(vict, ch);
        char_from_room(vict, false);
        char_to_room(vict, ch->in_room->dir_option[DOWN]->to_room, false);
        act("$n is hurled in from above!", false, vict, 0, 0, TO_ROOM);
        look_at_room(vict, vict->in_room, 0);
        WAIT_STATE(vict, 1 RL_SEC);
        return 1;
    } else {
        act("$n attempts to grapple with you and fails!", false, ch, 0, vict,
            TO_VICT);
        WAIT_STATE(ch, 1 RL_SEC);
        return 1;
    }

    return 0;
}
