//
// File: nohunger_dude.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define LVL_CAN_GAIN_NODRUNK  40   /* <-----Set desired level here */

SPECIAL(nohunger_dude)
{
    struct creature *dude = (struct creature *)me;
    int gold, life_cost;

    if (!CMD_IS("gain"))
        return 0;
    skip_spaces(&argument);

    life_cost = MAX(10, 70 - GET_CON(ch));
    gold = adjusted_price(ch, dude, GET_LEVEL(ch) * 10000);

    if (!*argument) {
        send_to_char(ch, "Gain what?\r\n"
            "It will cost you %d life points and %'d gold coins to gain alcoholic immunity.\r\n",
            life_cost, gold);
    } else if (GET_LEVEL(ch) < LVL_CAN_GAIN_NODRUNK && !IS_REMORT(ch)) {
        send_to_char(ch, "You are not yet ready to gain this.\r\n");
    } else {
        if (!is_abbrev(argument, "nodrunk")) {
            perform_tell(dude, ch,
                "You can only gain 'nodrunk'.");
            return 1;
        }

        if (GET_COND(ch, DRUNK) < 0) {
            perform_tell(dude, ch,
                "You are already unaffected by alcohol.");
            return 1;
        }

        if (GET_GOLD(ch) < gold) {
            snprintf(buf, sizeof(buf),
                "You don't have the %'d gold coins I require for that.", gold);
            perform_tell(dude, ch, buf);
        } else if (GET_LIFE_POINTS(ch) < life_cost) {
            snprintf(buf, sizeof(buf),
                "You haven't gained the %d life points you need to do that.",
                life_cost);
            perform_tell(dude, ch, buf);
        } else {
            act("$n outstretches $s arms in supplication to the powers that be.", true, dude, NULL, NULL, TO_ROOM);
            send_to_char(ch,
                "You feel a strange sensation pass through your liver.\r\n");
            act("A strange expression crosses $N's face...", true, dude, NULL, ch,
                TO_NOTVICT);
            GET_LIFE_POINTS(ch) -= life_cost;
            GET_GOLD(ch) -= gold;
            GET_COND(ch, DRUNK) = -1;
            send_to_char(ch, "You will no longer be affected by alcohol!\r\n");

            slog("%s has gained nodrunk.", GET_NAME(ch));

            return 1;
        }
    }
    return 1;
}
