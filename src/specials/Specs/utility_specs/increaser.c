//
// File: increaser.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_HITP 1
#define MODE_MANA 2
#define MODE_MOVE 4

SPECIAL(increaser)
{

    struct creature *increaser = (struct creature *)me;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char status_desc[64];
    int64_t gold;
    int life_cost, incr;
    int amount = 0;
    int8_t mode, status = 0;

    if ((!CMD_IS("increase")) || IS_NPC(ch))
        return false;

    two_arguments(argument, arg1, arg2);

    if (GET_LEVEL(ch) < 20) {
        send_to_char(ch, "You are not yet ready to increase.\r\n");
        send_to_char(ch, "Come back when you are level 20 or above.\r\n");
        return true;
    }

    if (GET_NPC_VNUM(increaser) == 30100)
        status = MODE_HITP;
    if (GET_NPC_VNUM(increaser) == 37022 || GET_NPC_VNUM(increaser) == 31712)
        status = MODE_MANA;
    if (GET_NPC_VNUM(increaser) == 5430)
        status = MODE_MOVE;

    switch (status) {
    case MODE_MOVE:
        strcpy(status_desc, "move");
        amount = 4;
        break;
    case MODE_HITP:
        strcpy(status_desc, "hit");
        amount = 2;
        break;
    case MODE_MANA:
        strcpy(status_desc, "mana");
        amount = 2;
        break;
    }

    if (!*arg1) {
        send_to_char(ch, "Increase what?\r\nType 'increase %s <amount>.\r\n",
            status_desc);
        send_to_char(ch, "The cost is 1 lp / %d points of %s.\r\n", amount,
            status_desc);
        return 1;
    }
    if (!strcasecmp(arg1, "hit")) {
        mode = MODE_HITP;
    } else if (!strcasecmp(arg1, "mana")) {
        mode = MODE_MANA;
    } else if (!strcasecmp(arg1, "move")) {
        mode = MODE_MOVE;
    } else {
        send_to_char(ch, "Increase what?\r\nType 'increase %s <amount>.\r\n",
            status_desc);
        send_to_char(ch, "The cost is 1 lp / %d points of %s.\r\n", amount,
            status_desc);
        return 1;
    }

    if (!IS_SET(status, mode)) {
        send_to_char(ch, "You cannot increase that here.\r\n");
        return 1;
    }

    if (!*arg2) {
        send_to_char(ch, "Increase your %s by how much?\r\n", arg1);
        return 1;
    }
    if (!is_number(arg2) || (incr = atoi(arg2)) < 0) {
        send_to_char(ch, "The second argument must be a positive number.\r\n");
        return 1;
    }

    if (incr > 10000) {
        send_to_char(ch, "Let's try just doing 10,000 for now...\r\n");
        incr = 10000;
    }

    if (mode == MODE_MOVE)
        life_cost = ((incr + 3) >> 2);
                                 /** 4 pts/ life point */
    else
        life_cost = ((incr + 1) >> 1);  /* 2 pts/ life point */
    gold = 10000 * life_cost;
    gold += (gold * cost_modifier(ch, increaser)) / 100;

    send_to_char(ch,
        "It will cost you %'" PRId64 " %s and %d life points to increase your %s by %d.\r\n",
        gold, CURRENCY(ch), life_cost, arg1, incr);

    sprintf(buf, "$n considers the implications of increasing $s %s.", arg1);

    if (CASH_MONEY(ch) < gold) {
        send_to_char(ch,
            "But you do not have enough money on you for that.\r\n");
        act(buf, true, ch, NULL, NULL, TO_ROOM);
        return 1;
    }
    if (GET_LIFE_POINTS(ch) < life_cost) {
        send_to_char(ch,
            "But you do not have enough life points for that.\r\n");
        act(buf, true, ch, NULL, NULL, TO_ROOM);
        return true;
    }

    if (ch->in_room->zone->time_frame == TIME_ELECTRO)
        GET_CASH(ch) -= gold;
    else
        GET_GOLD(ch) -= gold;
    GET_LIFE_POINTS(ch) -= life_cost;

    switch (mode) {
    case MODE_MANA:
        GET_MAX_MANA(ch) += incr;
        break;
    case MODE_MOVE:
        GET_MAX_MOVE(ch) += incr;
        break;
    case MODE_HITP:
        GET_MAX_HIT(ch) += incr;
        break;
    }

    slog("%s increased %s %'d points at %d.",
        GET_NAME(ch), arg1, incr, ch->in_room->number);

    send_to_char(ch, "You begin your improvement.\r\n");
    act("$n begins to improve.", false, ch, NULL, NULL, TO_ROOM);
    if (GET_COND(ch, FULL) != -1)
        GET_COND(ch, FULL) = 1;
    if (GET_COND(ch, THIRST) != -1)
        GET_COND(ch, THIRST) = 1;
    WAIT_STATE(ch, PULSE_VIOLENCE * 5);
    crashsave(ch);
    return true;
}
