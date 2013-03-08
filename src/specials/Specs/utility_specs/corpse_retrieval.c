//
// File: corpse_retrieval.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void call_for_help(struct creature *ch, struct creature *attacker);

SPECIAL(corpse_retrieval)
{
    const char *currency;
    struct obj_data *corpse;
    struct creature *retriever = (struct creature *)me;
    int price;
    int amt_carried;

    if (spec_mode == SPECIAL_TICK) {
        if (is_fighting(((struct creature *)me)) && !number(0, 4)) {
            call_for_help(retriever, random_opponent(retriever));
            return 1;
        }
        return 0;
    }

    if (!CMD_IS("retrieve"))
        return 0;
    if (!can_see_creature(retriever, ch)) {
        perform_say(retriever, "say", "Who's there?  I can't see you.");
        return 1;
    }
    if (IS_NPC(ch)) {
        act("$n snickers at $N.", false, retriever, NULL, ch, TO_NOTVICT);
        act("$n snickers at you.", false, retriever, NULL, ch, TO_VICT);
        return 1;
    }

    corpse = object_list;
    while (corpse) {
        if (IS_OBJ_TYPE(corpse, ITEM_CONTAINER) && GET_OBJ_VAL(corpse, 3)
            && CORPSE_IDNUM(corpse) == GET_IDNUM(ch))
            break;
        corpse = corpse->next;
    }

    if (!corpse) {
        perform_tell(retriever, ch,
            tmp_sprintf("Sorry %s, I cannot locate your corpse.",
                PERS(ch, retriever)));
        return 1;
    }

    if (corpse->contains) {
        price = GET_LEVEL(ch) * 4000;
    } else {
        price = GET_LEVEL(ch) * 100;
    }
    price = adjusted_price(ch, retriever, price);

    if (retriever->in_room->zone->time_frame == TIME_ELECTRO) {
        amt_carried = GET_CASH(ch);
        currency = "credits";
    } else {
        amt_carried = GET_GOLD(ch);
        currency = "coins";
    }

    if (price > amt_carried) {
        perform_tell(retriever, ch,
            tmp_sprintf("You don't have enough money.  It costs %d %s.",
                price, currency));
        return 1;
    }

    if (corpse->carried_by && corpse->carried_by == ch) {
        perform_tell(retriever, ch, "You already have it you dolt!");
        return 1;
    }

    if (corpse->in_room) {
        if (corpse->in_room == retriever->in_room) {
            perform_tell(retriever, ch,
                "You fool.  It's already in this room!");
            return 1;
        }
        if (ROOM_FLAGGED(corpse->in_room, ROOM_NORECALL) ||
            (corpse->in_room->zone != ch->in_room->zone &&
                (ZONE_FLAGGED(corpse->in_room->zone, ZONE_ISOLATED)
                    || ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)))) {
            send_to_char(ch, "Your corpse cannot be located!\r\n");
            return 1;
        }
        act("$p disappears with a flash!", true, NULL, corpse, NULL, TO_ROOM);
        obj_from_room(corpse);
    } else if (corpse->in_obj)
        obj_from_obj(corpse);
    else if (corpse->carried_by) {
        act("$p disappears out of your hands!",
            false, corpse->carried_by, corpse, NULL, TO_CHAR);
        obj_from_char(corpse);
    } else if (corpse->worn_by) {
        act("$p disappears off of your body!",
            false, corpse->worn_by, corpse, NULL, TO_CHAR);
        if (corpse == GET_EQ(corpse->worn_by, corpse->worn_on))
            unequip_char(corpse->worn_by, corpse->worn_on, EQUIP_WORN);
        else if (corpse == GET_IMPLANT(corpse->worn_by, corpse->worn_on))
            unequip_char(corpse->worn_by, corpse->worn_on, EQUIP_IMPLANT);
        else if (corpse == GET_TATTOO(corpse->worn_by, corpse->worn_on))
            unequip_char(corpse->worn_by, corpse->worn_on, EQUIP_TATTOO);
        else
            errlog("Can't happen");
    } else {
        perform_tell(retriever, ch,
            "I'm sorry, your corpse has shifted out of the universe.");
        return 1;
    }

    sprintf(buf2, "Very well.  I will retrieve your corpse for %d %s.",
        price, currency);
    perform_tell(retriever, ch, buf2);
    obj_to_char(corpse, ch);

    switch (retriever->in_room->zone->time_frame) {
    case TIME_MODRIAN:
        GET_GOLD(ch) -= price;
        act("$n makes some strange gestures and howls!",
            false, retriever, NULL, NULL, TO_ROOM);
        break;
    case TIME_ELECTRO:
        GET_CASH(ch) -= price;
        act("$n slips into a deep concentration and there is a momentary flash of light!", false, retriever, NULL, NULL, TO_ROOM);
        break;
    case TIME_TIMELESS:
        GET_GOLD(ch) -= price;
        act("The air shimmers violently as $n lifts $s hands to the heavens!",
            false, retriever, NULL, NULL, TO_ROOM);
        break;
    }
    act("$p appears in your hands!", false, ch, corpse, NULL, TO_CHAR);

    crashsave(ch);

    return 1;
}
