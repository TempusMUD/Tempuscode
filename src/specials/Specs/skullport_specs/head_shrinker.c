//
// File: head_shrinker.spec                                      -- Part of TempusMUD
//
// Editted version of corpse_griller.spec
// Editted by Daniel Carruth
// Copyright 1998 by John Watson, all rights reserved.
//

#define PROTOHEAD 23200
#define SHRINKER_COST 150
SPECIAL(head_shrinker)
{
    struct obj_data *corpse = NULL, *head = NULL;
    struct creature *shrinker = (struct creature *)me;
    char *s = NULL;
    char arg[MAX_INPUT_LENGTH];
    int cost = SHRINKER_COST;
    cost = adjusted_price(ch, shrinker, cost);

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (CMD_IS("list")) {
        send_to_char(ch,
            "Better empty the corpse out first if you want the contents.\r\n"
            "Type 'buy talisman <corpse>' to have a talisman made from it's head.\r\n");
        return 1;
    }

    if (!cmd || !CMD_IS("buy"))
        return 0;

    argument = one_argument(argument, arg);

    if (!is_abbrev(arg, "talisman"))
        return 0;

    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Buy head shrinking of what corpse?\r\n");
        return 1;
    }

    if (!(corpse = get_obj_in_list_vis(ch, arg, ch->carrying))) {
        send_to_char(ch, "You don't have %s '%s'.\r\n", AN(arg), arg);
        return 1;
    }

    if (!IS_FLESH_TYPE(corpse)) {
        send_to_char(ch,
            "I only shrink flesh heads. What's wrong with you?\r\n");
        return 1;
    }
    if (IS_MAT(corpse, MAT_MEAT_COOKED)) {
        send_to_char(ch, "That's a cooked corpse.  Are you crazy?\r\n");
        return 1;
    }
    if (!strncmp(corpse->name, "the headless", 12)) {
        return 1;
    }
    if (GET_GOLD(ch) < cost) {
        send_to_char(ch,
            "It costs %'d gold coins to shrink a head, buddy.\r\n", cost);
        return 1;
    }

    if (!(head = read_object(PROTOHEAD))) {
        mudlog(LVL_POWER, BRF, true,
            "Error: Head Shrinker cannot find proto head vnum %d.\r\n",
            PROTOHEAD);
        return 1;
    }

    GET_GOLD(ch) -= cost;

    if (!strncmp(corpse->name, "the severed head of", 19)) {
        sprintf(buf, "the shrunken head of%s", corpse->name + 19);
    } else if ((s = strstr(corpse->name, "corpse of"))) {
        s += 9;
        if (!*s) {
            return 1;
        }
        skip_spaces(&s);
        sprintf(buf, "the shrunken head of %s", s);
    } else {
        sprintf(buf, "the shrunken head of %s", corpse->name);
    }

    head->name = strdup(buf);

    sprintf(buf, "talisman head shrunken %s", corpse->aliases);
    head->aliases = strdup(buf);

    sprintf(buf, "%s is here.", head->name);
    head->line_desc = strdup(buf);

    obj_to_char(head, ch);
    extract_obj(corpse);

    act("You now have $p.", false, ch, head, NULL, TO_CHAR);
    act("$n now has $p.", false, ch, head, NULL, TO_ROOM);
    return 1;
}
