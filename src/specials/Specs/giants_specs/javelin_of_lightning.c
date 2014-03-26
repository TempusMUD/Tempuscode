//
// File: javelin_of_lightning.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(javelin_of_lightning)
{
    struct obj_data *jav = (struct obj_data *)me;
    struct creature *vict = NULL;
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
    int dam;
    void update_pos(struct creature *victim);

    if (spec_mode != SPECIAL_CMD)
        return 0;

    two_arguments(argument, arg1, arg2);

    if (!CMD_IS("throw") && !CMD_IS("hurl") && !CMD_IS("sling"))
        return 0;

    if (!*arg1)
        send_to_char(ch, "Throw what where?\r\n");
    else if (!isname(arg1, jav->aliases))
        return 0;
    else if (jav != GET_EQ(ch, WEAR_WIELD))
        send_to_char(ch, "You need to wield it first.\r\n");
    else if (!(*arg2) && !(vict = random_opponent(ch)))
        send_to_char(ch, "Who would you like to throw it at?\r\n");
    else if (!vict && !(vict = get_char_room_vis(ch, arg2)))
        send_to_char(ch, "Throw it at who?\r\n");
    else if (GET_LEVEL(vict) > LVL_IMMORT)
        send_to_char(ch, "Ummm, thats probably not such a hot idea.\r\n");
    else {
        act("You hurl $p at $N!!", false, ch, jav, vict, TO_CHAR);
        act("$n hurls $p at you!!", false, ch, jav, vict, TO_VICT);
        act("$n hurls $p at $N!!", false, ch, jav, vict, TO_NOTVICT);

        if (!ok_to_attack(ch, vict, false))
            return 1;

        if ((GET_SKILL(ch, SKILL_SHOOT) / 16) +
            GET_INT(ch) + GET_DEX(ch) + number(0, GET_LEVEL(ch) + 5) >
            (-(GET_AC(vict) / 8) + GET_DEX(vict) + number(10,
                    GET_LEVEL(vict) + 5))) {
            dam = dice(20, 24) + GET_STR(ch);
            damage(ch, vict, jav, dam, JAVELIN_OF_LIGHTNING, -1);
        } else
            damage(ch, vict, jav, 0, JAVELIN_OF_LIGHTNING, -1);
        obj_to_char(unequip_char(ch, WEAR_WIELD, EQUIP_WORN), ch);
        obj_from_char(jav);
        extract_obj(jav);
        return 1;
    }
    return 1;
}
