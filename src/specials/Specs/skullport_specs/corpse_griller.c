//
// File: corpse_griller.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define PROTOSTEAK  23005
#define GRILL_COST 100
SPECIAL(corpse_griller)
{
    struct obj_data *corpse = NULL, *steak = NULL;
    struct creature *griller = (struct creature *)me;
    char arg[MAX_INPUT_LENGTH];
    int cost = GRILL_COST;
    cost += (cost * cost_modifier(ch, griller)) / 100;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (!cmd || !CMD_IS("buy"))
        return 0;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch,
            "Type 'list' to see what I have on the menu.\r\n"
            "Type 'buy grill <corpse> to grill a corpse.\r\n"
            "Better empty it out first if you want the contents.\r\n");
        return 1;
    }

    if (strncmp(arg, "grill", 5))
        return 0;

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Buy grilling of what corpse?\r\n");
        return 1;
    }

    if (!(corpse = get_obj_in_list_vis(ch, arg, ch->carrying))) {
        send_to_char(ch, "You don't have %s '%s'.\r\n", AN(arg), arg);
        return 1;
    }

    if (!IS_FLESH_TYPE(corpse)) {
        send_to_char(ch, "I only grill flesh, homes.\r\n");
        return 1;
    }
    if (IS_MAT(corpse, MAT_MEAT_COOKED)) {
        send_to_char(ch, "That's already cooked, freak.\r\n");
        return 1;
    }

    if (GET_GOLD(ch) < cost) {
        send_to_char(ch, "It costs %'d gold coins to grill, buddy.\r\n", cost);
        return 1;
    }

    GET_GOLD(ch) -= cost;

    if (!(steak = read_object(PROTOSTEAK)))
        return 0;

    sprintf(buf, "a steak made from %s", corpse->name);
    steak->name = strdup(buf);

    sprintf(buf, "steak meat %s", corpse->aliases);
    steak->aliases = strdup(buf);

    sprintf(buf, "%s is here.", steak->name);
    steak->line_desc = strdup(buf);

    obj_to_char(steak, ch);
    extract_obj(corpse);

    act("You now have $p.", false, ch, steak, NULL, TO_CHAR);
    act("$n now has $p.", false, ch, steak, NULL, TO_ROOM);
    return 1;
}
