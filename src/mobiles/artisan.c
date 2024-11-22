#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>
#include <inttypes.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "tmpstr.h"
#include "spells.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "actions.h"
#include "vendor.h"

SPECIAL(artisan);

int cmd_slap, cmd_smirk, cmd_cry;

struct craft_component {
    int item;
    // These are for further expansion.
    int material;
    int weight;
    int value;
    int amount;
};

struct craft_item {
    int vnum;                   // object to be offered when all components are held
    bool custom_cost;           // true if cost field is used, false if item cost is used
    long cost;
    char *action_char_desc;     // Description of item being crafted to character
    char *action_room_desc;     // Description of item being crafted to room
    int fail_pct;
    GList *required;
};

struct obj_data *create(struct craft_item *item,
                        struct creature *keeper, struct creature *recipient);
char *next_crafting_requirement(struct craft_item *item,
                                struct creature *keeper);

static gint
free_craft_item(struct craft_item *item)
{
    g_list_foreach(item->required, (GFunc)free, NULL);
    g_list_free(item->required);
    free(item->action_char_desc);
    free(item->action_room_desc);
    free(item);
    return 0;
}

static void
free_craft_shop(struct craft_shop *shop)
{
    g_list_foreach(shop->items, (GFunc) free_craft_item, NULL);
    g_list_free(shop->items);
    free_reaction(shop->reaction);
    free(shop);
}


const char *
artisan_parse_param(char *param, struct craft_shop *shop, int *err_line)
{
    char *line, *param_key;
    const char *err = NULL;
    int lineno = 0;
    struct craft_item *cur_item = NULL;

    // Remove all the currently stored items.
    g_list_foreach(shop->items, (GFunc) free_craft_item, NULL);
    g_list_free(shop->items);
    shop->items = NULL;

    // Initialize default values
    while ((line = tmp_getline(&param)) != NULL) {
        lineno++;
        if (*line == '\0' || *line == '-') {
            continue;
        }
        if (add_reaction(shop->reaction, line)) {
            continue;
        }

        param_key = tmp_getword(&line);
        if (!strcmp(param_key, "room")) {
            shop->room = atol(line);
        } else if (!strcmp(param_key, "currency")) {
            if (is_abbrev(line, "past") || is_abbrev(line, "gold")) {
                shop->currency = 0;
            } else if (is_abbrev(line, "future") || is_abbrev(line, "cash")) {
                shop->currency = 1;
            } else if (is_abbrev(line, "qp") || is_abbrev(line, "quest")) {
                shop->currency = 2;
            } else {
                err = tmp_sprintf("invalid currency %s", line);
                break;
            }
        } else if (!strcmp(param_key, "craft")) {
            CREATE(cur_item, struct craft_item, 1);
            cur_item->vnum = atoi(line);
            if (cur_item->vnum <= 0) {
                err = tmp_sprintf("invalid item vnum %s", line);
                free(cur_item);
                break;
            }
            shop->items = g_list_append(shop->items, cur_item);
        } else if (!strcmp(param_key, "cost")) {
            if (!cur_item) {
                err = tmp_strdup("cost specified without item declaration");
                break;
            }
            cur_item->custom_cost = true;
            cur_item->cost = atol(line);
            if (cur_item->cost <= 0) {
                err = tmp_sprintf("invalid item vnum %d cost %s", cur_item->vnum, line);
                break;
            }
        } else if (!strcmp(param_key, "component")) {
            if (!cur_item) {
                err = tmp_strdup("component specified without preceding item");
                break;
            }
            struct craft_component *craft_comp;

            CREATE(craft_comp, struct craft_component, 1);
            craft_comp->item = atoi(line);
            if (craft_comp->item <= 0) {
                err = tmp_sprintf("invalid item vnum %d component %s", cur_item->vnum, line);
                free(craft_comp);
                break;
            }
            cur_item->required = g_list_append(cur_item->required, craft_comp);
        } else if (!strcmp(param_key, "failure")) {
            if (!cur_item) {
                err = tmp_strdup("failure specified without preceding item");
                break;
            }
            cur_item->fail_pct = atoi(line);
            if (cur_item->fail_pct < 0 || cur_item->fail_pct >= 100) {
                err = tmp_sprintf("invalid item vnum %d failure %s", cur_item->vnum, line);
                break;
            }
        } else if (!strcmp(param_key, "action-char")) {
            if (!cur_item) {
                err = tmp_strdup("action-char specified without preceding item");
                break;
            }
            if (*line) {
                cur_item->action_char_desc = strdup(line);
            }
        } else if (!strcmp(param_key, "action-room")) {
            if (!cur_item) {
                err = tmp_strdup("action-room specified without preceding item");
                break;
            }
            if (*line) {
                cur_item->action_room_desc = strdup(line);
            }
        } else {
            err = tmp_sprintf("invalid directive %s", param_key);
        }
        if (err) {
            break;
        }
    }

    if (err_line) {
        *err_line = (err) ? lineno : -1;
    }

    return err;
}

char *
craft_item_next_requirement(struct craft_item *item, struct creature *keeper)
{
    for (GList *it = item->required; it; it = it->next) {
        struct craft_component *compon = it->data;

        // Item components are all we support right now
        if (compon->item) {
            struct obj_data *obj = get_obj_in_list_num(compon->item, keeper->carrying);
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

    if (item->fail_pct && number(0, 100) < item->fail_pct) {
        return NULL;
    }

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
    if (craft_item_next_requirement(item, keeper)) {
        needed =
            tmp_strcat(CCRED(ch, C_NRM), "Unavailable", CCNRM(ch, C_NRM),
                       NULL);
    } else {
        needed =
            tmp_strcat(CCGRN(ch, C_NRM), " Available ", CCNRM(ch, C_NRM),
                       NULL);
    }

    return tmp_sprintf("%s%s  %s %-43s %" PRId64 "\r\n", msg,
                       item_prefix,
                       needed,
                       CAP(tmp_strdup(obj->name)),
                       adjusted_price(ch, keeper, item->cost));

}

// Lists the items for sale.
void
craft_shop_list(struct craft_shop *shop, struct creature *keeper,
                struct creature *ch)
{
    int idx = 0;

    if (!shop->items) {
        perform_say_to(keeper, ch, "I'm not in business right now.");
        return;
    }

    char *msg = tmp_strcat(CCCYN(ch, C_NRM),
                           " ##               Item                                               Cost\r\n-------------------------------------------------------------------------\r\n",
                           CCNRM(ch, C_NRM),
                           NULL);
    for (GList *it = shop->items; it; it = it->next) {
        struct craft_item *item = it->data;
        idx++;
        msg = list_commission_item(ch, keeper, idx, item, msg);
    }

    page_string(ch->desc, msg);
}


static int
is_shop_item_obj_name(gconstpointer a_ptr, gconstpointer b_ptr)
{
    const struct craft_item *item = a_ptr;
    const char *arg = b_ptr;
    return namelist_match(arg, real_object_proto(item->vnum)->aliases) ? 0:1;
}


// Attempts to purchase an item from keeper for ch.
void
craft_shop_buy(struct craft_shop *shop,
               struct creature *keeper, struct creature *ch, char *arguments)
{
    GList *item_itr;
    struct craft_item *item = NULL;
    struct obj_data *obj;
    char *arg, *msg, *needed_str;

    if (!*arguments) {
        send_to_char(ch, "What do you wish to buy?\r\n");
        return;
    }

    arg = tmp_getword(&arguments);

    item = NULL;
    if (*arg == '#') {
        int num;

        arg++;
        num = atoi(arg) - 1;
        if (num < 0) {
            perform_say_to(keeper, ch, "That's not a proper item!");
            return;
        }
        item = g_list_nth_data(shop->items, (unsigned int)num);
    } else {
        item_itr = g_list_find_custom(shop->items, arg, is_shop_item_obj_name);
        if (item_itr) {
            item = item_itr->data;
        }
    }

    if (!item) {
        perform_say_to(keeper, ch,
                       "I can't make that!  Use the LIST command!");
        return;
    }

    long modCost = adjusted_price(ch, keeper, item->cost);

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

    if (item->action_char_desc) {
        act(item->action_char_desc, false, keeper, obj, ch, TO_VICT);
    }
    if (item->action_room_desc) {
        act(item->action_room_desc, false, keeper, obj, ch, TO_NOTVICT);
    }
    send_to_char(ch, "You buy %s for %ld gold.\r\n", obj->name, modCost);
    switch (number(0, 20)) {
    case 0:
        msg = "Glad to do business with you";
        break;
    case 1:
        msg = "Come back soon!";
        break;
    case 2:
        msg = "Have a nice day.";
        break;
    case 3:
        msg = "Thanks, and come again!";
        break;
    case 4:
        msg = "Nice doing business with you";
        break;
    default:
        msg = NULL;
        break;
    }
    if (msg) {
        perform_say_to(keeper, ch, msg);
    }
}

void
assign_artisans(void)
{
    cmd_slap = find_command("slap");
    cmd_smirk = find_command("smirk");
    cmd_cry = find_command("cry");
}

SPECIAL(artisan)
{
    struct creature *keeper = (struct creature *)me;
    char *msg;
    struct craft_shop *shop;

    shop = (struct craft_shop *)keeper->mob_specials.func_data;
    if (spec_mode == SPECIAL_FREE && shop) {
        free_craft_shop(shop);
        keeper->mob_specials.func_data = NULL;
        return 1;
    }

    if (spec_mode != SPECIAL_CMD) {
        return false;
    }

    if (!ch || !keeper || !AWAKE(keeper)) {
        return false;
    }

    if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
        do_action(keeper, GET_NAME(ch), cmd_slap, 0);
        perform_say(keeper, "shout",
                    tmp_sprintf("%s is a bloody thief!", tmp_capitalize(GET_NAME(ch))));
        return true;
    }

    if (!CMD_IS("list") && !CMD_IS("buy")) {
        return false;
    }

    char *config = GET_NPC_PARAM(keeper);
    if (!config) {
        return 0;
    }

    shop = (struct craft_shop *)keeper->mob_specials.func_data;
    if (!shop) {
        CREATE(shop, struct craft_shop, 1);
        shop->reaction = make_reaction();
        int err_line;
        const char *err = artisan_parse_param(config, shop, &err_line);
        if (err != NULL) {
            errlog("vendor %d spec error on line %d: %s",
                   GET_NPC_VNUM(keeper), err_line, err);
            keeper->mob_specials.shared->func = NULL;
            free_craft_shop(shop);
            return 1;
        }
        keeper->mob_specials.func_data = shop;
    }

    if (react(shop->reaction, ch) != ALLOW) {
        perform_say_to(keeper, ch, "Not doing business with YOU.");
        return true;
    }

    if (!shop || shop->room != keeper->in_room->number) {
        msg = tmp_sprintf("Sorry!  I don't have my tools!");
        perform_say_to(keeper, ch, msg);
        do_action(keeper, tmp_strdup(""), cmd_cry, 0);
        return true;
    }

    if (CMD_IS("list")) {
        craft_shop_list(shop, keeper, ch);
    } else if (CMD_IS("buy")) {
        craft_shop_buy(shop, keeper, ch, argument);
    } else if (CMD_IS("sell")) {
        msg = tmp_sprintf("I don't buy things, I make them.");
        perform_say_to(keeper, ch, msg);
    } else {
        return false;
    }

    return true;
}
