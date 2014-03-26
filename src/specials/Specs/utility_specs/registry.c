//
// File: registry.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(registry)
{
    struct creature *reg = (struct creature *)me;
    struct obj_data *cert = NULL;
    char buf3[MAX_STRING_LENGTH];
    int cost, home, vcert = -1;

    if (!CMD_IS("register"))
        return 0;

    if (!can_see_creature(reg, ch)) {
        perform_say(reg, "say",
            "Who's there?  Come visible to register, infidel!");
        return 1;
    }
    cost = adjusted_price(ch, reg, GET_LEVEL(ch) * 100);

    if (GET_GOLD(ch) < cost) {
        sprintf(buf2,
            "It costs %'d coins to register here, which you do not have.",
            cost);
        perform_tell(reg, ch, buf2);
        return 1;
    }

    switch (ch->in_room->zone->number) {
    case 204:
        home = HOME_ISTAN;
        break;
    case 54:
        home = HOME_NEW_THALOS;
        vcert = 5497;       /** Vnum of certificate **/
        break;
    case 391:
        home = HOME_KROMGUARD;
        vcert = 39199;
        break;
    case 30:
    case 31:
    case 32:
        home = HOME_MODRIAN;
        vcert = 3189;
        break;
    case 300:
    case 301:
        home = HOME_ELECTRO;
        break;
    case 590:
        home = HOME_MAVERNAL;
        break;
    case 210:
        if (!IS_MONK(ch)) {
            send_to_char(ch, "Only monks can register here.\r\n");
            return 1;
        }
        home = HOME_MONK;
        break;
    case 226:
        home = HOME_DROW_ISLE;
        break;
    case 228:
        if (ch->in_room->number == 22803)
            home = HOME_DWARVEN_CAVERNS;
        else if (ch->in_room->number == 22878)
            home = HOME_HUMAN_SQUARE;
        else
            home = HOME_SKULLPORT;
        break;
    case 190:
        if (GET_HOME(ch) == HOME_ELVEN_VILLAGE) {
            perform_tell(reg, ch, "You are already a resident here.");
            if (IS_EVIL(ch)) {
                perform_tell(reg, ch, "But you don't need to be, EVIL scum!");
                send_to_char(ch,
                    "You are no longer a resident of the Elven Village.\r\n");
                act("$n just lost $s residence in the village!", true, ch, NULL,
                    NULL, TO_ROOM);
                population_record[GET_HOME(ch)]--;
                GET_HOME(ch) = HOME_MODRIAN;
                population_record[GET_HOME(ch)]++;
            }
            return 1;
        }
        if (IS_EVIL(ch)) {
            sprintf(buf2, "We don't need your kind here, %s.", GET_NAME(ch));
            perform_tell(reg, ch, buf2);
            return 1;
        } else if (!IS_ELF(ch) && !IS_GOOD(ch)) {
            sprintf(buf2, "Sorry %s, we cannot accept you.", GET_NAME(ch));
            perform_tell(reg, ch, buf2);
            return 1;
        }
        home = HOME_ELVEN_VILLAGE;
        vcert = 19099;
        break;

    case 425:
        home = HOME_ASTRAL_MANSE;
        break;

    default:
        perform_say(reg, "say", "Sorry, this office is temporarily closed.");
        return 1;
        break;
    }

    if (GET_HOME(ch) == home) {
        send_to_char(ch, "You are already a legal resident of %s!\r\n",
            home_towns[home]);
        return 1;
    }

    population_record[GET_HOME(ch)]--;
    population_record[home]++;

    GET_HOME(ch) = home;
    GET_GOLD(ch) -= cost;
    sprintf(buf3, "Welcome to %s, %s.", home_towns[home], GET_NAME(ch));
    perform_say(reg, "say", buf3);
    sprintf(buf2, "That will be %'d coins.", cost);
    perform_tell(reg, ch, buf2);

    if ((cert = read_object(vcert))) {
        obj_to_char(cert, ch);
        act("$n presents $N with $p.", false, reg, cert, ch, TO_NOTVICT);
        act("$n presents you with $p.", false, reg, cert, ch, TO_VICT);
    }

    mudlog(GET_INVIS_LVL(ch), CMP, true,
        "%s has registered at %s.", GET_NAME(ch),
        home_towns[(int)GET_HOME(ch)]);
    return 1;
}
