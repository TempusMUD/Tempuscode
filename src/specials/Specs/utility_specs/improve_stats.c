//
// File: improve_stats.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

const char *improve_modes[7] = {
    "strength", "intelligence", "wisdom",
    "dexterity", "constitution", "charisma",
    "\n"
};

int8_t *
real_stat_ptr(struct creature *ch, int mode)
{
    switch (mode) {
    case ATTR_STR:
        return &ch->real_abils.str;
    case ATTR_INT:
        return &ch->real_abils.intel;
    case ATTR_WIS:
        return &ch->real_abils.wis;
    case ATTR_DEX:
        return &ch->real_abils.dex;
    case ATTR_CON:
        return &ch->real_abils.con;
    case ATTR_CHA:
        return &ch->real_abils.cha;
    }
    return NULL;
}

int
do_gen_improve(struct creature *ch, struct creature *trainer, int cmd,
    int mode, char *argument)
{

    int gold, life_cost;
    int8_t *real_stat = real_stat_ptr(ch, mode);
    int8_t old_stat = *real_stat;
    int max_stat;

    if ((!CMD_IS("improve") && !CMD_IS("train")) || IS_NPC(ch))
        return false;

    if (GET_LEVEL(ch) < 10) {
        send_to_char(ch, "You are not yet ready to improve this way.\r\n");
        send_to_char(ch, "Come back when you are level 10 or above.\r\n");
        return true;
    }

    gold = *real_stat * GET_LEVEL(ch) * 50;
    if (mode == ATTR_STR && IS_MAGE(ch))
        gold <<= 1;
    gold += (gold * cost_modifier(ch, trainer)) / 100;

    life_cost = MAX(6, (*real_stat << 1) - (GET_WIS(ch)));

    skip_spaces(&argument);

    max_stat = max_creature_attr(ch, mode);

    if (!*argument) {
        if (*real_stat >= max_stat) {
            send_to_char(ch, "%sYour %s cannot be improved further.%s\r\n",
                CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
            return true;
        }

        send_to_char(ch,
            "It will cost you %d coins and %d life points to improve your %s.\r\n",
            gold, life_cost, improve_modes[mode]);
        sprintf(buf, "$n considers the implications of improving $s %s.",
            improve_modes[mode]);
        act(buf, true, ch, 0, 0, TO_ROOM);
        if (GET_GOLD(ch) < gold)
            send_to_char(ch,
                "But you do not have enough gold on you for that.\r\n");
        else if (GET_LIFE_POINTS(ch) < life_cost)
            send_to_char(ch,
                "But you do not have enough life points for that.\r\n");

        return true;
    }

    if (!is_abbrev(argument, improve_modes[mode])) {
        send_to_char(ch, "The only thing you can improve here is %s.\r\n",
            improve_modes[mode]);
        return true;
    }

    if (*real_stat >= max_stat) {
        send_to_char(ch, "%sYour %s cannot be improved further.%s\r\n",
            CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
        return true;
    }

    if (GET_GOLD(ch) < gold) {
        send_to_char(ch, "You cannot afford it.  The cost is %d coins.\r\n",
            gold);
        return 1;
    }
    if (GET_LIFE_POINTS(ch) < life_cost) {
        sprintf(buf,
            "You have not gained sufficient life points to do this.\r\n"
            "It requires %d.\r\n", life_cost);
        send_to_char(ch, "%s", buf);
        return 1;
    }

    GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - gold);
    GET_LIFE_POINTS(ch) -= life_cost;

    *real_stat += 1;

    // crashsaving does the necessary adjustments to aff_attrs
    crashsave(ch);

    slog("%s improved %s from %d to %d at %d.",
         GET_NAME(ch), improve_modes[mode], old_stat, *real_stat,
         ch->in_room->number);

    send_to_char(ch, "You begin your training.\r\n");
    act("$n begins to train.", false, ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, *real_stat RL_SEC);

    return true;
}

SPECIAL(improve_dex)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_DEX,
            argument));
}

SPECIAL(improve_str)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_STR,
            argument));
}

SPECIAL(improve_int)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_INT,
            argument));
}

SPECIAL(improve_wis)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_WIS,
            argument));
}

SPECIAL(improve_con)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_CON,
            argument));
}

SPECIAL(improve_cha)
{
    return (do_gen_improve(ch, (struct creature *)me, cmd, ATTR_CHA,
            argument));
}
