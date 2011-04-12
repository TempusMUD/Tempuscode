#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "db.h"
#include "structs.h"
#include "xml_utils.h"
#include "utils.h"
#include "interpreter.h"
#include "actions.h"
#include "char_class.h"
#include "comm.h"
#include "tmpstr.h"
#include "screen.h"
#include "handler.h"
#include "security.h"
#include "vendor.h"

SPECIAL(artisan);

int cmd_slap, cmd_smirk, cmd_cry;
GList *shop_list;

struct craft_component {
    int item;
    int material;
    int weight;
    int value;
    int amount;
};

struct craft_item {
    int vnum;                   // object to be offered when all components are held
    long cost;                  // -1 means to use default cost
    int fail_pct;
    GList *required;
};

struct obj_data *create(struct craft_item *item,
    struct creature *keeper, struct creature *recipient);
char *next_crafting_requirement(struct craft_item *item,
    struct creature *keeper);

gint
shop_id_matches(gpointer vnum, struct craft_shop *shop,
                gpointer ignore)
{
    return (shop->id == GPOINTER_TO_INT(vnum)) ? 0 : -1;
}

struct craft_shop *
craft_shop_by_id(int idnum)
{
    GList *it;
    it = g_list_find_custom(shop_list, 0, (GCompareFunc) shop_id_matches);
    return (it) ? it->data : NULL;
}

gint
shop_keeper_matches(gpointer vnum, struct craft_shop *shop,
                    struct creature *keeper)
{
    return (shop->keeper_vnum == GET_NPC_VNUM(keeper)
            && shop->room == keeper->in_room->number) ? 0 : -1;
}


struct craft_shop *
craft_shop_by_keeper(struct creature *keeper)
{
    GList *it;
    it = g_list_find_custom(shop_list, keeper, (GCompareFunc) shop_keeper_matches);
    return (it) ? it->data : NULL;
}


/**
 * loads the craft_item described by the given xml node.
 * Does not reinit the item. Always creates a new item.
**/
void
craft_shop_parse_item(struct craft_shop *shop, xmlNodePtr node)
{
    xmlNodePtr sub_node;
    struct craft_item *new_item;
    struct craft_component *compon;

    CREATE(new_item, struct craft_item, 1);
    new_item->vnum = xmlGetIntProp(node, "vnum", 0);
    new_item->cost = xmlGetIntProp(node, "cost", 0);
    new_item->fail_pct = xmlGetIntProp(node, "failure", 0);
    for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
        if (xmlMatches(sub_node->name, "requires")) {
            CREATE(compon, struct craft_component, 1);
            compon->item = xmlGetIntProp(sub_node, "item", 0);
            compon->material = xmlGetIntProp(sub_node, "material", 0);
            compon->weight = xmlGetIntProp(sub_node, "weight", 0);
            compon->value = xmlGetIntProp(sub_node, "value", 0);
            compon->amount = xmlGetIntProp(sub_node, "amount", 0);
            new_item->required = g_list_prepend(new_item->required, compon);
        } else if (!xmlMatches(sub_node->name, "text")) {
            errlog("Invalid XML tag '%s' while parsing craftshop item",
                (const char *)sub_node->name);
        }
    }
    new_item->required = g_list_reverse(new_item->required);
    shop->items = g_list_prepend(shop->items, new_item);
    return;
}

/**
 * Loads this craft_shop's data from the given xml node.
**/
void
craft_shop_load(struct craft_shop *shop, xmlNodePtr node)
{
    xmlNodePtr sub_node;
    xmlChar *prop;
    shop->room = xmlGetIntProp(node, "room", 0);
    shop->keeper_vnum = xmlGetIntProp(node, "keeper", 0);

    // Remove all the currently stored items.
    g_list_foreach(shop->items, (GFunc) free, 0);
    g_list_free(shop->items);
    shop->items = NULL;

    // Load the described items and thier info.
    for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
        prop = NULL;
        if (xmlMatches(sub_node->name, "item")) {
            craft_shop_parse_item(shop, sub_node);
        } else if (!xmlMatches(sub_node->name, "text")) {
            errlog("Invalid XML tag '%s' while parsing craftshop",
                (const char *)sub_node->name);
        }
    }
}

/**
 * Loads the craft_shop described by the given xml node.
 * If the shop has already been created, it is reinitialized.
**/
void
load_craft_shop(xmlNodePtr node)
{
    struct craft_shop *shop = NULL;
    int id = xmlGetIntProp(node, "id", -1);

    shop = craft_shop_by_id(id);
    if (!shop) {
        CREATE(shop, struct craft_shop, 1);
        craft_shop_load(shop, node);
        shop_list = g_list_prepend(shop_list, shop);
    } else {
        craft_shop_load(shop, node);
    }
}

char *
craft_item_next_requirement(struct craft_item *item, struct creature *keeper)
{
    struct craft_component *compon;
    struct obj_data *obj;
    GList *it;

    for (it = item->required; it; it = it->next) {
        compon = it->data;
        // Item components are all we support right now
        if (compon->item) {
            obj = get_obj_in_list_num(compon->item, keeper->carrying);
            if (!obj) {
                obj = real_object_proto(compon->item);
                return tmp_strdup(obj->name);
            }
        } else {
            errlog("Unimplemented requirement in artisan");
        }
    }

    return NULL;
}

struct obj_data *
craft_item_create(struct craft_item *item,
    struct creature *keeper, struct creature *recipient)
{
    struct craft_component *compon;
    struct obj_data *obj;
    GList *it;

    for (it = item->required; it; it = it->next) {
        compon = it->data;
        // Item components are all we support right now
        if (compon->item) {
            obj = get_obj_in_list_num(compon->item, keeper->carrying);
            if (!obj) {
                errlog("craft_remove_prereqs called without all prereqs");
                return NULL;
            }
            obj_from_char(obj);
            extract_obj(obj);
        } else {
            errlog("Unimplemented requirement in artisan");
        }
    }

    if (item->fail_pct && number(0, 100) < item->fail_pct)
        return NULL;

    obj = read_object(item->vnum);
    obj_to_char(obj, recipient);
    return obj;
}

char *
list_commission_item(struct creature *ch,
    struct creature *keeper, int idx, struct craft_item *item, char *msg)
{
    struct obj_data *obj;
    char *needed;
    char *item_prefix;

    obj = real_object_proto(item->vnum);
    item_prefix = tmp_sprintf("%3d%s)%s",
        idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    // We have to find out what we need.
    if (craft_item_next_requirement(item, keeper))
        needed =
            tmp_strcat(CCRED(ch, C_NRM), "Unavailable", CCNRM(ch, C_NRM),
            NULL);
    else
        needed =
            tmp_strcat(CCGRN(ch, C_NRM), " Available ", CCNRM(ch, C_NRM),
            NULL);

    return tmp_sprintf("%s%s  %s %-43s %11ld\r\n", msg,
        item_prefix,
        needed,
        CAP(tmp_strdup(obj->name)),
        item->cost + (item->cost * cost_modifier(ch, keeper)) / 100);

}

// sends a simple status message to the given struct creature.
void
send_craft_shop_status(struct craft_shop *shop, struct creature *ch)
{
    const char *name = "<not loaded>";
    struct creature *keeper = real_mobile_proto(shop->keeper_vnum);
    if (keeper != NULL)
        name = GET_NAME(keeper);
    send_to_char(ch, "[%6d] %15s [%6d] ( %d items )\r\n",
        shop->id, name, shop->keeper_vnum, g_list_length(shop->items));
}

// Lists the items for sale.
void
craft_shop_list(struct craft_shop *shop, struct creature *keeper,
    struct creature *ch)
{
    GList *it;
    struct craft_item *item;
    int idx = 0;

    if (!shop->items) {
        perform_say_to(keeper, ch, "I'm not in business right now.");
        return;
    }

    char *msg = tmp_strcat(CCCYN(ch, C_NRM),
        " ##               Item                                               Cost\r\n-------------------------------------------------------------------------\r\n",
        CCNRM(ch, C_NRM),
        NULL);
    for (it = shop->items; it; it = it->next) {
        item = it->data;
        idx++;
        msg = list_commission_item(ch, keeper, idx, item, msg);
    }

    page_string(ch->desc, msg);
}

// Attempts to purchase an item from keeper for ch.
void
craft_shop_buy(struct craft_shop *shop,
    struct creature *keeper, struct creature *ch, char *arguments)
{
    GList *item_itr;
    struct craft_item *item;
    struct obj_data *obj;
    char *arg, *msg, *needed_str;

    arg = tmp_getword(&arguments);

    item = NULL;
    if (*arg == '#') {
        int num;

        arg++;
        num = atoi(arg) - 1;
        if (num <= 0) {
            perform_say_to(keeper, ch, "That's not a proper item!");
            return;
        }
        if ((unsigned int)num < g_list_length(shop->items))
            item = g_list_nth_data(shop->items, num);
    } else {
        for (item_itr = shop->items; item_itr; item_itr = item_itr->next) {
            item = item_itr->data;
            if (isname(arg, real_object_proto(item->vnum)->aliases))
                break;
        }
    }

    if (!item) {
        perform_say_to(keeper, ch,
            "I can't make that!  Use the LIST command!");
        return;
    }

    long modCost = item->cost + (item->cost * cost_modifier(ch, keeper)) / 100;

    needed_str = craft_item_next_requirement(item, keeper);
    if (needed_str) {
        msg = tmp_sprintf("I don't have the necessary materials.");
        perform_say_to(keeper, ch, msg);
        msg = tmp_sprintf("Give me %s.", needed_str);
        perform_say_to(keeper, ch, msg);
        return;
    }

    if (modCost > GET_GOLD(ch)) {
        msg = tmp_sprintf("You don't have enough money");
        perform_say_to(keeper, ch, msg);
        msg = tmp_sprintf("It costs %ld.", modCost);
        perform_say_to(keeper, ch, msg);
        return;
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        msg = tmp_sprintf("You can't carry any more items.");
        perform_say_to(keeper, ch, msg);
        return;
    }

    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(real_object_proto(item->vnum)) >
        CAN_CARRY_W(ch)) {
        msg = tmp_sprintf("You can't carry any more weight.");
        perform_say_to(keeper, ch, msg);
        return;
    }

    GET_GOLD(ch) -= modCost;
    obj = craft_item_create(item, keeper, ch);

    if (!obj) {
        msg =
            tmp_sprintf
            ("I am sorry.  I failed to make it and I used up all my materials.");
        perform_say_to(keeper, ch, msg);
        return;
    }

    send_to_char(ch, "You buy %s for %ld gold.\r\n", obj->name, modCost);
    switch (number(0, 20)) {
    case 0:
        msg = tmp_strcat(GET_NAME(ch), " Glad to do business with you", NULL);
        break;
    case 1:
        msg = tmp_strcat(GET_NAME(ch), " Come back soon!", NULL);
        break;
    case 2:
        msg = tmp_strcat(GET_NAME(ch), " Have a nice day.", NULL);
        break;
    case 3:
        msg = tmp_strcat(GET_NAME(ch), " Thanks, and come again!", NULL);
        break;
    case 4:
        msg = tmp_strcat(GET_NAME(ch), " Nice doing business with you", NULL);
        break;
    default:
        msg = NULL;
        break;
    }
    if (msg)
        perform_say_to(keeper, ch, msg);
}

void
assign_artisans(void)
{
    GList *it;
    struct craft_shop *shop;
    struct creature *mob;

    for (it = shop_list; it; it = it->next) {
        shop = it->data;
        mob = real_mobile_proto(shop->keeper_vnum);
        if (mob)
            mob->mob_specials.shared->func = artisan;
        else
            errlog("Artisan mob %d not found!", shop->keeper_vnum);
    }

    cmd_slap = find_command("slap");
    cmd_smirk = find_command("smirk");
    cmd_cry = find_command("cry");
}

SPECIAL(artisan)
{
    struct creature *keeper = (struct creature *)me;
    char *msg;
    struct craft_shop *shop;

    if (spec_mode != SPECIAL_CMD)
        return false;

    if (!ch || !keeper || !AWAKE(keeper))
        return false;

    if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
        do_action(keeper, GET_NAME(ch), cmd_slap, 0);
        sprintf(buf, "%s is a bloody thief!", GET_NAME(ch));
        buf[0] = toupper(buf[0]);
        perform_say(keeper, "shout", buf);
        return true;
    }

    if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("sell")
        && !CMD_IS("status"))
        return false;

    shop = craft_shop_by_keeper(keeper);

    if (!shop || shop->room != keeper->in_room->number) {
        msg = tmp_sprintf("Sorry!  I don't have my tools!");
        perform_say_to(keeper, ch, msg);
        do_action(keeper, tmp_strdup(""), cmd_cry, 0);
        return true;
    }

/*
	if (!shop->accepts(ch) && shop->refuses(ch)) {
		sprintf(buf, "I don't deal with your type.");
		perform_say_to(keeper, ch, buf);
		do_action(keeper, GET_NAME(ch), cmd_smirk, 0);
		return true;
	}
*/

    if (CMD_IS("list")) {
        craft_shop_list(shop, keeper, ch);
    } else if (CMD_IS("buy")) {
        craft_shop_buy(shop, keeper, ch, argument);
    } else if (CMD_IS("sell")) {
        msg = tmp_sprintf("I don't buy things, I make them.");
        perform_say_to(keeper, ch, msg);
    } else if (CMD_IS("status") && is_authorized(ch, DEBUGGING, NULL)) {
        GList *it;
        for (it = shop_list; it; it = it->next) {
            send_craft_shop_status(((struct craft_shop *)it->data), ch);
        }
    } else {
        return false;
    }

    return true;
}
