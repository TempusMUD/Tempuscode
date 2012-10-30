//
// File: repairer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MS_VNUM 30185
#define IS_MS(mob)  (GET_NPC_VNUM(mob) == MS_VNUM)

SPECIAL(repairer)
{

    struct creature *repairer = (struct creature *)me;
    struct obj_data *obj = NULL, *proto_obj = NULL;
    int cost, obj_damage;
    bool currency;

    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
        tellbuf[MAX_STRING_LENGTH];

    if (!CMD_IS("buy") && !CMD_IS("value"))
        return 0;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "You can BUY or VALUE repairs <item>.\r\n");
        return 1;
    }

    argument = two_arguments(argument, arg1, arg2);

    if (strcasecmp(arg1, "repairs")) {
        perform_tell(repairer, ch,
            "To deal with me: buy/value repairs <item>");
        return 1;
    }

    if (!*arg2) {
        perform_tell(repairer, ch, "What item?");
        return 1;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
        perform_tell(repairer, ch, "You don't have that item.");
        return 1;
    }

    if (!(proto_obj = real_object_proto(obj->shared->vnum))) {
        perform_tell(repairer, ch, "No way I'm going to repair that.");
        return 1;
    }

    if (GET_OBJ_DAM(obj) <= -1 || GET_OBJ_MAX_DAM(obj) <= -1) {
        sprintf(tellbuf, "There is no point... %s is unbreakable.", obj->name);
        perform_tell(repairer, ch, tellbuf);
        return 1;
    }

    if (GET_OBJ_MAX_DAM(obj) == 0 ||
        GET_OBJ_MAX_DAM(obj) <= (GET_OBJ_MAX_DAM(proto_obj) >> 4)) {
        sprintf(tellbuf, "Sorry, %s is damaged beyond repair.", obj->name);
        perform_tell(repairer, ch, tellbuf);
        return 1;
    }

    if (IS_MS(repairer) && !IS_METAL_TYPE(obj)) {
        perform_tell(repairer, ch, "Sorry, I only repair metals.");
        return 1;
    }

    obj_damage = GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj);
    cost = (obj_damage * GET_OBJ_COST(obj) / GET_OBJ_MAX_DAM(obj)) / 8;
    cost += (cost * cost_modifier(ch, repairer)) / 100;

    currency = (ch->in_room->zone->time_frame == TIME_ELECTRO);

    if (CMD_IS("value")) {
        sprintf(tellbuf, "It will cost you %'d %s to repair %s.", cost,
            currency ? "credits" : "coins", obj->name);
        perform_tell(repairer, ch, tellbuf);
        return 1;
    }

    if ((currency && cost > GET_CASH(ch)) ||
        (!currency && cost > GET_GOLD(ch))) {
        sprintf(tellbuf, "You don't have the %'d %s I require.", cost,
            currency ? "credits" : "gold coins");
        perform_tell(repairer, ch, tellbuf);
        return 1;
    }

    act("$n takes $p into $s shop and repairs it.",
        false, repairer, obj, NULL, TO_ROOM);
    WAIT_STATE(ch, 5 RL_SEC);

    if (currency)
        GET_CASH(ch) -= cost;
    else
        GET_GOLD(ch) -= cost;

    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
        GET_OBJ_MAX_DAM(obj) -= ((GET_OBJ_MAX_DAM(obj) * 15) / 100) + 1;
        REMOVE_BIT(GET_OBJ_EXTRA2(obj), ITEM2_BROKEN);
    }
    // Object gets repaired fully, but repairs subtract 2% of the
    // damage off of the max damage
    GET_OBJ_MAX_DAM(obj) -= MAX(1, obj_damage / 50);
    GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);

    return 1;
}
