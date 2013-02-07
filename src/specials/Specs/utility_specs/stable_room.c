//
// File: stable_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(stable_room)
{
    char pet_name[256];
    struct room_data *pet_room;
    struct creature *pet = NULL;
    int price;

    if (!(pet_room = ch->in_room->next)) {
        send_to_char(ch, "stable error.\r\n");
        return 0;
    }

    if (CMD_IS("list")) {
        send_to_char(ch, "Available mounts are:\r\n");
        for (GList * it = pet_room->people; it; it = it->next) {
            struct creature *tch = it->data;
            if (!IS_NPC(tch))
                continue;
            send_to_char(ch, "%8d - %s\r\n", 3 * GET_EXP(tch), GET_NAME(tch));
        }
        return (true);
    } else if (CMD_IS("buy")) {

        argument = one_argument(argument, buf);
        argument = one_argument(argument, pet_name);

        if (!(pet = get_char_room(buf, pet_room))) {
            send_to_char(ch, "There is no such mount!\r\n");
            return (true);
        }
        if (!IS_NPC(pet)) {
            send_to_char(ch, "Funny.  Real funny.  Ha ha.\r\n");
            return (true);
        }
        if (GET_GOLD(ch) < (GET_EXP(pet) * 3)) {
            send_to_char(ch, "You don't have enough gold!\r\n");
            return (true);
        }
        GET_GOLD(ch) -= GET_EXP(pet) * 3;

        pet = read_mobile(GET_NPC_VNUM(pet));
        GET_EXP(pet) = 0;
        SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

        if (*pet_name) {
            sprintf(buf, "%s %s", pet->player.name, pet_name);
            pet->player.name = strdup(buf);

            sprintf(buf,
                "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
                pet->player.description, pet_name);
            pet->player.description = strdup(buf);
        }
        char_to_room(pet, ch->in_room, false);
        add_follower(pet, ch);

        /* Be certain that pets can't get/carry/use/wield/wear items */
        IS_CARRYING_W(pet) = 1000;
        IS_CARRYING_N(pet) = 100;

        send_to_char(ch, "May this mount serve you well.\r\n");
        act("$n buys $N as a mount.", false, ch, NULL, pet, TO_ROOM);

        return 1;
    }

    if (CMD_IS("sell") || CMD_IS("value")) {

        skip_spaces(&argument);

        if (!*argument) {
            sprintf(buf, "%s what mount?\r\n",
                CMD_IS("sell") ? "Sell" : "Value");
            return 1;
        }
        if (!(pet = get_char_room_vis(ch, argument))) {
            send_to_char(ch, "I don't see that mount here.\r\n");
            return 1;
        }
        if (!pet->master || pet->master != ch || !AFF_FLAGGED(pet, AFF_CHARM)) {
            act("You don't have the authority to sell $N.", false, ch, NULL, pet,
                TO_CHAR);
            return 1;
        }
        if (!IS_NPC(pet) || !NPC2_FLAGGED(pet, NPC2_MOUNT)) {
            send_to_char(ch, "I only buy mounts.\r\n");
            return 1;
        }
        price = GET_LEVEL(pet) * 10 + GET_MOVE(pet) * 10 + GET_HIT(pet) * 10;

        sprintf(buf, "I will pay you %'d gold coins for $N.", price);
        act(buf, false, ch, NULL, pet, TO_CHAR);
        if (CMD_IS("value"))
            return 1;

        if (MOUNTED_BY(ch) && MOUNTED_BY(ch) == pet) {
            dismount(ch);
            REMOVE_BIT(AFF2_FLAGS(pet), AFF2_MOUNTED);
            GET_POSITION(ch) = POS_STANDING;
        }
        stop_follower(pet);

        act("The stables now owns $N.", false, ch, NULL, pet, TO_CHAR);
        act("$n sells $N to the stables.", false, ch, NULL, pet, TO_ROOM);

        GET_GOLD(ch) += price;
        GET_EXP(pet) = price / 2;

        char_from_room(pet, false);
        char_to_room(pet, pet_room, false);
        for (GList * it = pet_room->people; it; it = it->next) {
            struct creature *tch = it->data;
            if (tch != pet && IS_NPC(tch)
                && GET_NPC_VNUM(tch) == GET_NPC_VNUM(pet)) {
                creature_purge(tch, true);
                return 1;
            }
        }

        return 1;
    }

    /* All commands except list and buy */
    return 0;
}
