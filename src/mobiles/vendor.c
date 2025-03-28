#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
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
#include "players.h"
#include "tmpstr.h"
#include "spells.h"
#include "obj_data.h"
#include "specs.h"
#include "actions.h"
#include "weather.h"
#include "vendor.h"
#include "strutil.h"
#include "mail.h"
#include "house.h"
#include "bomb.h"

#define MAX_ITEMS   10
#define MIN_COST    12

// From act.comm.cc
void perform_analyze(struct creature *ch, struct obj_data *obj, bool checklev);
void perform_appraise(struct creature *ch, struct obj_data *obj,
                      int skill_lvl);

// From cityguard.cc
void call_for_help(struct creature *ch, struct creature *attacker);

static void
vendor_log(const char *fmt, ...)
{
    static FILE *vendor_log_file = NULL;
    va_list args;

    if (!vendor_log_file) {
        vendor_log_file = fopen("log/vendor.log", "a");
    }
    if (vendor_log_file) {
        va_start(args, fmt);
        fprintf(vendor_log_file, "%s :: %s\n",
                tmp_ctime(time(NULL)),
                tmp_vsprintf(fmt, args));
        va_end(args);
    }
}

static bool
vendor_is_produced(struct obj_data *obj, struct shop_data *shop)
{
    for (GList *it = shop->item_list; it; it = it->next) {
        if (GET_OBJ_VNUM(obj) == GPOINTER_TO_INT(it->data)) {
            return same_obj(real_object_proto(GPOINTER_TO_INT(it->data)), obj);
        }
    }

    return false;
}

int
vendor_inventory(struct obj_data *obj, struct obj_data *obj_list)
{
    struct obj_data *cur_obj;
    int cnt = 0;

    cur_obj = obj_list;
    while (cur_obj && GET_OBJ_VNUM(cur_obj) != GET_OBJ_VNUM(obj) &&
           !same_obj(cur_obj, obj)) {
        cur_obj = cur_obj->next_content;
    }

    if (!cur_obj) {
        return 0;
    }

    while (same_obj(cur_obj, obj)) {
        cur_obj = cur_obj->next_content;
        cnt++;
    }

    return cnt;
}

static struct obj_data *
vendor_items_forsale(struct shop_data *shop, struct creature *self)
{
    if (shop->storeroom > 0) {
        struct room_data *room = real_room(shop->storeroom);
        if (!room) {
            errlog("Can't find room %d", shop->storeroom);
            return NULL;
        }
        return room->contents;
    }
    return self->carrying;
}

static bool
vendor_invalid_buy(struct creature *self, struct creature *ch,
                   struct shop_data *shop, struct obj_data *obj)
{
    if (IS_OBJ_STAT(obj, ITEM_NOSELL) ||
        !OBJ_APPROVED(obj) || obj->shared->owner_id != 0) {
        perform_say_to(self, ch, shop->msg_badobj);
        return true;
    }

    if (GET_OBJ_COST(obj) < 1) {
        perform_say_to(self, ch, "Why would I want that?  It has no value.");
        return true;
    }

    if (shop->item_types) {
        bool accepted = false;
        for (GList *it = shop->item_types; it; it = it->next) {
            if ((GPOINTER_TO_INT(it->data) & 0xFF) == GET_OBJ_TYPE(obj)
                || (GPOINTER_TO_INT(it->data) & 0xFF) == 0) {
                accepted = GPOINTER_TO_INT(it->data) >> 8;
                break;
            }
        }
        if (!accepted) {
            perform_say_to(self, ch, shop->msg_badobj);
            return true;
        }
    } else {
        perform_say_to(self, ch, shop->msg_badobj);
        return true;
    }

    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
        perform_say_to(self, ch,
                       "I'm not buying something that's already broken.");
        return true;
    }

    if (GET_OBJ_EXTRA3(obj) & ITEM3_HUNTED) {
        perform_say_to(self, ch,
                       "This is hunted by the forces of Hell!  I'm not taking this!");
        return true;
    }

    if (GET_OBJ_SIGIL_IDNUM(obj)) {
        perform_say_to(self, ch,
                       "You'll have to remove that warding sigil before I'll bother.");
        return true;
    }

    if (vendor_inventory(obj, vendor_items_forsale(shop, self)) >= MAX_ITEMS) {
        perform_say_to(self, ch,
                       "No thanks.  I've got too many of those in stock already.");
        return true;
    }
    // Adjust cost for missing charges
    if (IS_OBJ_TYPE(obj, ITEM_WAND) || IS_OBJ_TYPE(obj, ITEM_STAFF)) {
        if (GET_OBJ_VAL(obj, 2) == 0) {
            perform_say_to(self, ch, "I don't buy used up wands or staves!");
            return true;
        }
    }

    if (IS_BOMB(obj)
        && obj->contains
        && IS_FUSE(obj->contains)
        && FUSE_STATE(obj->contains)) {
        perform_say_to(self, ch,
                       "That looks a little too hot to handle for me.");
        return true;
    }

    return false;
}

static bool
vendor_invalid_consignment(struct creature *self, struct creature *ch,
                           struct shop_data *shop, struct obj_data *obj)
{
    if (!OBJ_APPROVED(obj) || obj->shared->owner_id != 0) {
        perform_say_to(self, ch, shop->msg_badobj);
        return true;
    }

    if (shop->item_types) {
        bool accepted = false;
        for (GList *it = shop->item_types; it; it = it->next) {
            if ((GPOINTER_TO_INT(it->data) & 0xFF) == GET_OBJ_TYPE(obj)
                || (GPOINTER_TO_INT(it->data) & 0xFF) == 0) {
                accepted = GPOINTER_TO_INT(it->data) >> 8;
                break;
            }
        }
        if (!accepted) {
            perform_say_to(self, ch, shop->msg_badobj);
            return true;
        }
    } else {
        perform_say_to(self, ch, shop->msg_badobj);
        return true;
    }

    if (GET_OBJ_SIGIL_IDNUM(obj)) {
        perform_say_to(self, ch,
                       "You'll have to remove that warding sigil before I'll bother.");
        return true;
    }

    if (vendor_inventory(obj, vendor_items_forsale(shop, self)) >= MAX_ITEMS) {
        perform_say_to(self, ch,
                       "No thanks.  I've got too many of those in stock already.");
        return true;
    }

    if (IS_BOMB(obj)
        && obj->contains
        && IS_FUSE(obj->contains)
        && FUSE_STATE(obj->contains)) {
        perform_say_to(self, ch,
                       "That looks a little too hot to handle for me.");
        return true;
    }

    return false;
}

// Gets the selling price of an object
static unsigned long
vendor_get_value(struct creature *buyer, struct creature *seller, struct obj_data *obj, int percent, int currency)
{
    unsigned long cost;

    // Consigned object sell for the consignment price, unaffected by
    // charisma or condition
    if (obj->consignor) {
        return obj->consign_price;
    }

    // Adjust cost for wear and tear on a direct percentage basis
    if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1 &&
        GET_OBJ_MAX_DAM(obj) != 0) {
        percent = percent * GET_OBJ_DAM(obj) / GET_OBJ_MAX_DAM(obj);
    }

    // Adjust cost for missing charges
    if ((IS_OBJ_TYPE(obj, ITEM_WAND) || IS_OBJ_TYPE(obj, ITEM_STAFF)) &&
        GET_OBJ_VAL(obj, 1) != 0) {
        percent = percent * GET_OBJ_VAL(obj, 2) / GET_OBJ_VAL(obj, 1);
    }

    cost = GET_OBJ_COST(obj) * percent / 100;
    if (OBJ_REINFORCED(obj)) {
        cost += cost / 4;
    }
    if (OBJ_ENHANCED(obj)) {
        cost += cost / 4;
    }

    if (currency == 2) {
        cost = MAX(1, cost);
    } else {
        cost = MAX(MIN_COST, cost);
    }

    return adjusted_price(buyer, seller, cost);
}

struct obj_data *
vendor_resolve_hash(struct shop_data *shop, struct creature *self, char *obj_str)
{
    struct obj_data *last_obj = NULL, *cur_obj;
    int num;

    if (*obj_str != '#') {
        errlog("Can't happen in vendor_resolve_hash()");
        return NULL;
    }

    num = atoi(obj_str + 1);
    if (num <= 0) {
        return NULL;
    }

    for (cur_obj = vendor_items_forsale(shop, self); cur_obj; cur_obj = cur_obj->next_content) {
        if (!last_obj || !same_obj(last_obj, cur_obj)) {
            if (--num == 0) {
                return cur_obj;
            }
        }
        last_obj = cur_obj;
    }

    return cur_obj;
}

struct obj_data *
vendor_resolve_name(struct shop_data *shop, struct creature *self, char *obj_str)
{
    struct obj_data *cur_obj;

    for (cur_obj = vendor_items_forsale(shop, self); cur_obj; cur_obj = cur_obj->next_content) {
        if (namelist_match(obj_str, cur_obj->aliases)) {
            return cur_obj;
        }
    }

    return NULL;
}

void
vendor_appraise(struct creature *ch, struct obj_data *obj,
                struct creature *self, struct shop_data *shop)
{
    const char *currency_str;
    const unsigned long cost = 2000;
    unsigned long amt_carried;

    if (shop->currency == 2) {
        perform_say_to(self, ch, "I don't do appraisals.");
        return;
    }
    switch (shop->currency) {
    case 0:
        amt_carried = GET_GOLD(ch);
        break;
    case 1:
        amt_carried = GET_CASH(ch);
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        amt_carried = 0;
        break;
    }
    if (cost > amt_carried) {
        perform_say_to(self, ch, shop->msg_buyerbroke);
        if (shop->cmd_temper) {
            command_interpreter(self, tmp_gsub(shop->cmd_temper, "$N",
                                               GET_NAME(ch)));
        }
        return;
    }

    switch (shop->currency) {
    case 0:
        GET_GOLD(ch) -= cost;
        GET_GOLD(self) += cost;
        currency_str = "gold";
        break;
    case 1:
        GET_CASH(ch) -= cost;
        GET_CASH(self) += cost;
        currency_str = "creds";
        break;
    case 2:
        account_set_quest_points(ch->account, ch->account->quest_points -=
                                     cost);
        currency_str = "quest points";
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        currency_str = "-BUGS-";
    }

    perform_say_to(self, ch,
                   tmp_sprintf("That will cost you %'lu %s.", cost, currency_str));

    if (shop->currency == 1) {
        perform_analyze(ch, obj, false);
    } else {
        spell_identify(50, ch, NULL, obj, NULL);
    }
}

static void
vendor_sell(struct creature *ch, char *arg, struct creature *self,
            struct shop_data *shop)
{
    struct obj_data *obj, *next_obj;
    char *obj_str, *msg;
    const char *currency_str;
    int num;
    unsigned long cost, amt_carried;
    bool appraisal_only = false;

    if (!*arg) {
        send_to_char(ch, "What do you wish to buy?\r\n");
        return;
    }

    obj_str = tmp_getword(&arg);
    if (is_abbrev(obj_str, "appraise") || is_abbrev(obj_str, "appraisal")) {
        obj_str = tmp_getword(&arg);
        appraisal_only = true;
    }

    if (is_number(obj_str)) {
        num = atoi(obj_str);
        if (num < 0) {
            perform_say_to(self, ch,
                           "You want to buy a negative amount? Try selling.");
            return;
        } else if (num == 0) {
            perform_say_to(self, ch, "You wanted to buy something?");
            return;
        } else if (num > 24) {
            perform_say_to(self, ch, "I can't sell that many at once.");
            return;
        }

        obj_str = tmp_getword(&arg);
    } else if (streq(obj_str, "all")) {
        num = -1;
        obj_str = tmp_getword(&arg);
    } else {
        num = 1;
    }

    // Check for hash mark
    if (*obj_str == '#') {
        obj = vendor_resolve_hash(shop, self, obj_str);
    } else {
        obj = vendor_resolve_name(shop, self, obj_str);
    }

    if (!obj) {
        perform_say_to(self, ch, shop->msg_sell_noobj);
        return;
    }

    if (appraisal_only) {
        vendor_appraise(ch, obj, self, shop);
        return;
    }

    if (num == -1) {
        if (vendor_is_produced(obj, shop)) {
            perform_say_to(self, ch, "I can make these things all day!");
            return;
        }
        num = vendor_inventory(obj, obj);
    }

    if (num > 1) {
        int obj_cnt = vendor_inventory(obj, vendor_items_forsale(shop, self));
        if (!vendor_is_produced(obj, shop) && num > obj_cnt) {
            perform_say_to(self, ch,
                           tmp_sprintf("I only have %d to sell to you.", obj_cnt));
            num = obj_cnt;
        }
    }

    cost = vendor_get_value(ch, self, obj, shop->markup, shop->currency);

    switch (shop->currency) {
    case 0:
        amt_carried = GET_GOLD(ch);
        break;
    case 1:
        amt_carried = GET_CASH(ch);
        break;
    case 2:
        amt_carried = ch->account->quest_points;
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        amt_carried = 0;
        break;
    }

    if (cost > amt_carried) {
        perform_say_to(self, ch, shop->msg_buyerbroke);
        if (shop->cmd_temper) {
            command_interpreter(self, tmp_strdup(shop->cmd_temper));
        }
        return;
    }

    if (cost * num > amt_carried && cost > 0) {
        num = amt_carried / cost;
        perform_say_to(self, ch,
                       tmp_sprintf("You only have enough to buy %d.", num));
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        perform_say_to(self, ch, "You can't carry any more items.");
        return;
    }

    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
        switch (number(0, 2)) {
        case 0:
            perform_say_to(self, ch, "You can't carry any more weight.");
            break;
        case 1:
            perform_say_to(self, ch, "You can't carry that much weight.");
            break;
        case 2:
            perform_say_to(self, ch, "You can carry no more weight.");
            break;
        }
        return;
    }

    if ((IS_CARRYING_N(ch) + num > CAN_CARRY_N(ch) ||
         IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) * num > CAN_CARRY_W(ch)) &&
        GET_OBJ_WEIGHT(obj) > 0) {
        num = MIN(num, CAN_CARRY_N(ch) - IS_CARRYING_N(ch));
        num = MIN(num, (CAN_CARRY_W(ch) - IS_CARRYING_W(ch))
                  / GET_OBJ_WEIGHT(obj));
        perform_say_to(self, ch, tmp_sprintf("You can only carry %d.", num));
    }

    struct account *acc = NULL;
    GList *receipt_to = NULL;

    if (obj->consignor) {
        long acc_id = player_account_by_idnum(obj->consignor);
        if (acc_id == 0) {
            slog("WARNING: vendor_sell(), consignor %ld was not found",
                 obj->consignor);
            return;
        }
        acc = account_by_idnum(acc_id);
        if (acc == NULL) {
            slog("WARNING: vendor_sell(), consignor %ld's account was not found",
                 obj->consignor);
        }

        receipt_to = g_list_prepend(NULL, GINT_TO_POINTER(obj->consignor));
    }

    const char *CONSIGN_RECEIPT_TEXT = "\n"
                                       "Consignment Sale Receipt\n"
                                       "------------------------\n"
                                       "\n"
                                       "This receipt is to notify you that your item, %s,\n"
                                       "has been sold to %s.\n"
                                       "%'" PRId64 " %s have been placed into your bank account.\n"
                                                   "\n"
                                                   "Have a great day.\n"
                                                   "\n";


    switch (shop->currency) {
    case 0:
        GET_GOLD(ch) -= cost * num;
        GET_GOLD(self) += cost * num;

        if (acc) {
            deposit_past_bank(acc, cost * num);
            send_mail(self, receipt_to,
                      tmp_sprintf(CONSIGN_RECEIPT_TEXT,
                                  obj->name,
                                  GET_NAME(ch),
                                  cost * num,
                                  "gold coins"),
                      NULL, NULL);
        }

        currency_str = "gold";
        break;
    case 1:
        GET_CASH(ch) -= cost * num;
        GET_CASH(self) += cost * num;

        if (acc) {
            deposit_future_bank(acc, cost * num);
            send_mail(self, receipt_to,
                      tmp_sprintf(CONSIGN_RECEIPT_TEXT,
                                  obj->name,
                                  GET_NAME(ch),
                                  cost * num,
                                  "credits"),
                      NULL, NULL);
        }

        currency_str = "creds";
        break;
    case 2:
        account_set_quest_points(ch->account,
                                 ch->account->quest_points - (cost * num));
        currency_str = "quest points";
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        currency_str = "-BUGS-";
    }

    perform_say_to(self, ch, tmp_sprintf(shop->msg_buy, cost * num));
    msg = tmp_sprintf("You sell $p %sto $N for %'lu %s.",
                      ((num == 1) ? "" : tmp_sprintf("(x%d) ", num)),
                      cost * num, currency_str);
    act(msg, false, self, obj, ch, TO_CHAR);
    msg = tmp_sprintf("$n sells $p %sto you for %'lu %s.",
                      ((num == 1) ? "" : tmp_sprintf("(x%d) ", num)),
                      cost * num, currency_str);
    act(msg, false, self, obj, ch, TO_VICT);
    act("$n sells $p to $N.", false, self, obj, ch, TO_NOTVICT);

    vendor_log("%s[%d] sold %s[%d] (x%d) to %s %s[%d] for %lu %s%s",
               GET_NAME(self), GET_NPC_VNUM(self),
               obj->name, GET_OBJ_VNUM(obj),
               num,
               IS_NPC(ch) ? "NPC" : "PC",
               GET_NAME(ch), IS_NPC(ch) ? GET_NPC_VNUM(ch) : GET_IDNUM(ch),
               cost * num, currency_str,
               (obj->consignor) ? tmp_sprintf(" (on consignment for %s)", player_name_by_idnum(obj->consignor)) : "");

    if (vendor_is_produced(obj, shop)) {
        // Load all-new identical items
        while (num > 0) {
            obj = read_object(GET_OBJ_VNUM(obj));
            obj_to_char(obj, ch);
            num--;
        }
    } else {
        // Actually move the items from vendor to player
        struct room_data *room = (shop->storeroom == 0) ? NULL : real_room(shop->storeroom);

        while (num > 0) {
            next_obj = obj->next_content;

            obj->consignor = 0;
            obj->consign_price = 0;

            if (room) {
                obj_from_room(obj);
            } else {
                obj_from_char(obj);
            }

            obj_to_char(obj, ch);
            obj = next_obj;
            num--;
        }

        if (room && ROOM_FLAGGED(room, ROOM_HOUSE)) {
            struct house *h = find_house_by_room(room->number);
            if (h) {
                save_house(h);
            }
        }
    }

    if (GET_IDNUM(ch) > 0) {
        crashsave(ch);
    }
}

static void
vendor_buy(struct creature *ch, char *arg, struct creature *self,
           struct shop_data *shop)
{
    struct obj_data *obj, *next_obj;
    char *obj_str;
    int num = 1;
    unsigned long cost, amt_carried;

    if (shop->currency == 2) {
        perform_say_to(self, ch, "Hey, I only sell stuff.");
        return;
    }

    if (!*arg) {
        send_to_char(ch, "What do you wish to sell?\r\n");
        return;
    }

    obj_str = tmp_getword(&arg);
    if (is_number(obj_str)) {
        num = atoi(obj_str);
        if (num < 0) {
            perform_say_to(self, ch,
                           "You want to sell a negative amount? Try buying.");
            return;
        } else if (num == 0) {
            perform_say_to(self, ch, "You wanted to sell something?");
            return;
        }

        obj_str = tmp_getword(&arg);
    } else if (streq(obj_str, "all")) {
        num = -1;
        obj_str = tmp_getword(&arg);
    } else {
        num = 1;
    }

    obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
    if (!obj) {
        perform_say_to(self, ch, shop->msg_buy_noobj);
        return;
    }

    if (vendor_invalid_buy(self, ch, shop, obj)) {
        return;
    }

    if (num == -1) {
        num = vendor_inventory(obj, obj);
    }

    // We only check inventory after the object selected.  This allows people
    // to sell 5 3.shirt if they have 2 shirts ahead of the one they want to
    // sell
    if (vendor_inventory(obj, obj) < num) {
        send_to_char(ch, "You only have %d of those!\r\n",
                     vendor_inventory(obj, ch->carrying));
        return;
    }

    if (vendor_is_produced(obj, shop)) {
        perform_say_to(self, ch,
                       "I make these.  Why should I buy it back from you?");
        return;
    }
    cost = vendor_get_value(self, ch, obj, shop->markdown, shop->currency);

    amt_carried = (shop->currency) ? GET_CASH(self) : GET_GOLD(self);

    if (amt_carried < cost) {
        perform_say_to(self, ch, shop->msg_selfbroke);
        return;
    }

    if (amt_carried < cost * num && cost > 0) {
        num = amt_carried / cost;
        perform_say_to(self, ch,
                       tmp_sprintf("I can only afford to buy %d.", num));
    }

    if (vendor_inventory(obj, self->carrying) + num > MAX_ITEMS) {
        num = MAX_ITEMS - vendor_inventory(obj, vendor_items_forsale(shop, self));
        perform_say_to(self, ch, tmp_sprintf("I only want to buy %d.", num));
    }

    perform_say_to(self, ch, tmp_sprintf(shop->msg_sell, cost * num));

    transfer_money(self, ch, cost * num, shop->currency, false);

    vendor_log("%s[%d] bought %s[%d] (x%d) from %s %s[%d] for %lu %s",
               GET_NAME(self), GET_NPC_VNUM(self),
               obj->name, GET_OBJ_VNUM(obj),
               num,
               IS_NPC(ch) ? "NPC" : "PC",
               GET_NAME(ch), IS_NPC(ch) ? GET_NPC_VNUM(ch) : GET_IDNUM(ch),
               cost * num, shop->currency ? "creds" : "gold");

    struct room_data *room = (shop->storeroom == 0) ? NULL : real_room(shop->storeroom);

    // We've already verified that they have enough of the item via a
    // call to vendor_inventory(), so we can just blindly transfer objects
    while (num-- && obj) {
        // transfer object
        next_obj = obj->next_content;
        obj_from_char(obj);
        if (room) {
            obj_to_room(obj, room);
        } else {
            obj_to_char(obj, self);
        }

        // repair object
        if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1) {
            GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);
        }

        obj = next_obj;
    }

    if (room && ROOM_FLAGGED(room, ROOM_HOUSE)) {
        struct house *h = find_house_by_room(room->number);
        if (h) {
            save_house(h);
        }
    }

    if (GET_IDNUM(ch) > 0) {
        crashsave(ch);
    }
}

char *
vendor_list_obj(struct creature *ch, struct obj_data *obj, int cnt, int idx,
                int cost)
{
    char *obj_desc;

    obj_desc = obj->name;
    if (IS_OBJ_TYPE(obj, ITEM_DRINKCON) && GET_OBJ_VAL(obj, 1)) {
        obj_desc = tmp_strcat(obj_desc, " of ",
                              liquid_to_str(GET_OBJ_VAL(obj, 2)), NULL);
    }
    if ((IS_OBJ_TYPE(obj, ITEM_WAND) || IS_OBJ_TYPE(obj, ITEM_STAFF)) &&
        GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
        obj_desc = tmp_strcat(obj_desc, " (partially used)", NULL);
    }
    if (OBJ_REINFORCED(obj)) {
        obj_desc = tmp_strcat(obj_desc, " [reinforced]", NULL);
    }
    if (OBJ_ENHANCED(obj)) {
        obj_desc = tmp_strcat(obj_desc, " |augmented|", NULL);
    }
    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
        obj_desc = tmp_strcat(obj_desc, " <broken>", NULL);
    }
    if (IS_OBJ_STAT(obj, ITEM_HUM)) {
        obj_desc = tmp_strcat(obj_desc, " (humming)", NULL);
    }
    if (IS_OBJ_STAT(obj, ITEM_GLOW)) {
        obj_desc = tmp_strcat(obj_desc, " (glowing)", NULL);
    }
    if (IS_OBJ_STAT(obj, ITEM_INVISIBLE)) {
        obj_desc = tmp_strcat(obj_desc, " (invisible)", NULL);
    }
    if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT)) {
        obj_desc = tmp_strcat(obj_desc, " (transparent)", NULL);
    }
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
        if (IS_OBJ_STAT(obj, ITEM_BLESS)) {
            obj_desc = tmp_strcat(obj_desc, " (holy aura)", NULL);
        }
        if (IS_OBJ_STAT(obj, ITEM_DAMNED)) {
            obj_desc = tmp_strcat(obj_desc, " (unholy aura)", NULL);
        }
    }
    if (obj->consignor) {
        obj_desc = tmp_strcat(obj_desc, " (from ", player_name_by_idnum(obj->consignor), ")", NULL);
    }

    obj_desc = tmp_capitalize(obj_desc);
    if (cnt < 0) {
        return tmp_sprintf(" %2d%s)  %sUnlimited%s   %-47s %'7d\r\n",
                           idx, CCRED(ch, C_NRM), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                           obj_desc, cost);
    }

    return tmp_sprintf(" %2d%s)  %s%5d%s       %-47s %'7d\r\n",
                       idx, CCRED(ch, C_NRM), CCYEL(ch, C_NRM), cnt, CCNRM(ch, C_NRM),
                       obj_desc, cost);
}

static void
vendor_list(struct creature *ch, char *arg, struct creature *self,
            struct shop_data *shop)
{
    struct obj_data *cur_obj, *last_obj;
    int idx, cnt;
    const char *msg;
    unsigned long cost;

    if (!vendor_items_forsale(shop, self)) {
        perform_say_to(self, ch, "I'm out of stock at the moment.");
        return;
    }

    switch (shop->currency) {
    case 0:
        msg = "        Gold";
        break;
    case 1:
        msg = "       Creds";
        break;
    case 2:
        msg = "Quest Points";
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        msg = "PleaseReport";
    }

    msg = tmp_strcat(CCCYN(ch, C_NRM),
                     " ##   Available   Item                                       ", msg,
                     "\r\n",
                     "-------------------------------------------------------------------------\r\n",
                     CCNRM(ch, C_NRM), NULL);

    last_obj = NULL;
    cnt = idx = 1;
    for (cur_obj = vendor_items_forsale(shop, self); cur_obj; cur_obj = cur_obj->next_content) {
        if (last_obj) {
            if (same_obj(last_obj, cur_obj)) {
                cnt++;
            } else {
                if (vendor_is_produced(last_obj, shop)) {
                    cnt = -1;
                }
                if (!*arg || namelist_match(arg, last_obj->aliases)) {
                    cost = vendor_get_value(ch, self, last_obj, shop->markup, shop->currency);
                    msg = tmp_strcat(msg,
                                     vendor_list_obj(ch, last_obj, cnt, idx, cost), NULL);
                }
                cnt = 1;
                idx++;
            }
        }
        last_obj = cur_obj;
    }
    if (last_obj) {
        if (vendor_is_produced(last_obj, shop)) {
            cnt = -1;
        }
        if (!*arg || namelist_match(arg, last_obj->aliases)) {
            cost = vendor_get_value(ch, self, last_obj, shop->markup, shop->currency);
            msg = tmp_strcat(msg, vendor_list_obj(ch, last_obj, cnt, idx, cost), NULL);
        }
    }

    act("$n peruses the shop's wares.", false, ch, NULL, NULL, TO_ROOM);
    page_string(ch->desc, msg);
}

static void
vendor_value(struct creature *ch, char *arg, struct creature *self,
             struct shop_data *shop)
{
    struct obj_data *obj;
    char *obj_str;
    unsigned long cost;

    if (shop->currency == 2) {
        perform_say_to(self, ch, "I'm not the buying kind.");
        return;
    }

    if (!*arg) {
        send_to_char(ch, "What do you wish to value?\r\n");
        return;
    }

    obj_str = tmp_getword(&arg);
    obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
    if (!obj) {
        send_to_char(ch, "You don't seem to have any '%s'.\r\n", obj_str);
        return;
    }

    if (vendor_invalid_buy(self, ch, shop, obj)) {
        return;
    }

    cost = vendor_get_value(self, ch, obj, shop->markdown, shop->currency);

    perform_say_to(self, ch, tmp_sprintf("I'll give you %'lu %s for it!", cost,
                                         shop->currency ? "creds" : "gold"));
}

static void
vendor_consign(struct creature *ch, char *arg, struct creature *self,
               struct shop_data *shop)
{
    const char *USAGE = "Usage: consign <item> <price>\r\n";
    struct obj_data *obj;
    char *obj_str;
    money_t amt_carried;

    if (!shop->consignment || shop->currency == 2) {
        perform_say_to(self, ch, "Sorry, I don't do consignments.");
        return;
    }

    if (!*arg) {
        send_to_char(ch, "%s", USAGE);
        return;
    }

    obj_str = tmp_getword(&arg);
    obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
    if (!obj) {
        send_to_char(ch, "You don't seem to have any '%s'.\r\n", obj_str);
        return;
    }

    if (!*arg) {
        send_to_char(ch, "%s", USAGE);
        return;
    }

    money_t consign_amt = atol(tmp_getword(&arg));
    if (consign_amt < MIN_COST) {
        perform_say_to(self, ch, tmp_sprintf("You must charge a price of at least %d.", MIN_COST));
        return;
    }

    if (vendor_invalid_consignment(self, ch, shop, obj)) {
        return;
    }

    // calculate consignment fee
    money_t consign_fee = (consign_amt + 5) / 10;

    amt_carried = (shop->currency) ? GET_CASH(self) : GET_GOLD(self);

    if (consign_fee > amt_carried) {
        perform_say_to(self, ch, shop->msg_buyerbroke);
        return;
    }

    transfer_money(ch, self, consign_fee, shop->currency, false);

    struct room_data *room = (shop->storeroom == 0) ? NULL : real_room(shop->storeroom);

    obj_from_char(obj);
    if (room) {
        obj_to_room(obj, room);
        if (ROOM_FLAGGED(room, ROOM_HOUSE)) {
            struct house *h = find_house_by_room(room->number);
            if (h) {
                save_house(h);
            }
        }
    } else {
        obj_to_char(obj, self);
    }
    if (GET_IDNUM(ch) > 0) {
        crashsave(ch);
    }
    obj->consignor = GET_IDNUM(ch);
    obj->consign_price = consign_amt;

    vendor_log("%s[%ld] consigns %s to %s for %'" PRId64 " %s.",
               GET_NAME(ch), GET_IDNUM(ch),
               obj->name,
               GET_NAME(self),
               consign_amt,
               (shop->currency) ? "credits" : "gold");

    act("You give $p to $N on consignment.", false, ch, obj, self, TO_CHAR);
    act("$n gives $p to you on consignment.", false, ch, obj, self, TO_VICT);
    act("$n gives $p to $N on consignment.", false, ch, obj, self, TO_NOTVICT);
}

static void
vendor_unconsign(struct creature *ch, char *arg, struct creature *self,
                 struct shop_data *shop)
{
    struct obj_data *obj;
    char *obj_str;

    if (!*arg) {
        send_to_char(ch, "What do you wish to unconsign?\r\n");
        return;
    }

    obj_str = tmp_getword(&arg);

    // Check for hash mark
    if (*obj_str == '#') {
        obj = vendor_resolve_hash(shop, self, obj_str);
    } else {
        obj = vendor_resolve_name(shop, self, obj_str);
    }

    if (!obj) {
        perform_say_to(self, ch, shop->msg_sell_noobj);
        return;
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        perform_say_to(self, ch, "You can't carry any more items.");
        return;
    }

    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
        switch (number(0, 2)) {
        case 0:
            perform_say_to(self, ch, "You can't carry any more weight.");
            break;
        case 1:
            perform_say_to(self, ch, "You can't carry that much weight.");
            break;
        case 2:
            perform_say_to(self, ch, "You can carry no more weight.");
            break;
        }
        return;
    }

    if (obj->consignor != GET_IDNUM(ch)) {
        perform_say_to(self, ch, "That isn't under consignment by you.");
        return;
    }

    vendor_log("%s[%ld] unconsigns %s from %s.",
               GET_NAME(ch), GET_IDNUM(ch),
               obj->name,
               GET_NAME(self));

    perform_say_to(self, ch, "Here you go.");
    act("You gives $p back to $N.", false, self, obj, ch, TO_CHAR);
    act("$n gives $p back to you.", false, self, obj, ch, TO_VICT);
    act("$n gives $p back to $N.", false, self, obj, ch, TO_NOTVICT);

    // Actually move the items from vendor to player
    struct room_data *room = (shop->storeroom == 0) ? NULL : real_room(shop->storeroom);

    obj->consignor = 0;
    obj->consign_price = 0;

    if (room) {
        obj_from_room(obj);
        if (ROOM_FLAGGED(room, ROOM_HOUSE)) {
            struct house *h = find_house_by_room(room->number);
            if (h) {
                save_house(h);
            }
        }
    } else {
        obj_from_char(obj);
    }

    obj_to_char(obj, ch);
    if (GET_IDNUM(ch) > 0) {
        crashsave(ch);
    }
}

static void
vendor_revenue(struct creature *self, struct shop_data *shop)
{
    struct creature *vkeeper;
    long cur_money, max_money;

    vkeeper = real_mobile_proto(GET_NPC_VNUM(self));
    max_money = (shop->currency) ? GET_CASH(vkeeper) : GET_GOLD(vkeeper);
    cur_money = (shop->currency) ? GET_CASH(self) : GET_GOLD(self);
    if (cur_money >= max_money) {
        return;
    }
    if (shop->currency) {
        GET_CASH(self) = MIN(max_money, GET_CASH(self) + shop->revenue);
    } else {
        GET_GOLD(self) = MIN(max_money, GET_GOLD(self) + shop->revenue);
    }
}

struct shop_data *
make_shop(void) {
    struct shop_data *shop;

    CREATE(shop, struct shop_data, 1);
    shop->room = -1;
    shop->msg_denied = strdup("I'm not doing business with YOU!");
    shop->msg_badobj = strdup("I don't buy that sort of thing.");
    shop->msg_selfbroke = strdup("Sorry, but I don't have the cash.");
    shop->msg_buyerbroke = strdup("You don't have enough money to buy this!");
    shop->msg_sell_noobj = strdup("Sorry, but I don't carry that item.");
    shop->msg_buy_noobj = strdup("You don't have that item!");
    shop->msg_buy = strdup("Here you go.");
    shop->msg_sell = strdup("There you go.");
    shop->msg_closed = strdup("Come back later!");
    shop->cmd_temper = NULL;
    shop->markdown = 70;
    shop->markup = 120;
    shop->currency = false;
    shop->revenue = 0;
    shop->steal_ok = false;
    shop->attack_ok = false;
    shop->consignment = false;
    shop->func = NULL;
    shop->reaction = make_reaction();

    return shop;
}

void
free_shop(struct shop_data *shop)
{
    g_list_free(shop->item_list);
    g_list_free(shop->item_types);
    g_list_free_full(shop->closed_hours, (GDestroyNotify)free);
    free(shop->msg_denied);
    free(shop->msg_badobj);
    free(shop->msg_sell_noobj);
    free(shop->msg_buy_noobj);
    free(shop->msg_selfbroke);
    free(shop->msg_buyerbroke);
    free(shop->msg_buy);
    free(shop->msg_sell);
    free(shop->cmd_temper);
    free(shop->msg_closed);
    free_reaction(shop->reaction);
    free(shop);
}

const char *
vendor_parse_param(char *param, struct shop_data *shop, int *err_line)
{
    char *line, *param_key;
    const char *err = NULL;
    int val, lineno = 0;

    // Initialize default values
    while ((line = tmp_getline(&param)) != NULL) {
        lineno++;
        if (add_reaction(shop->reaction, line)) {
            continue;
        }

        param_key = tmp_getword(&line);
        if (streq(param_key, "room")) {
            shop->room = atol(line);
        } else if (streq(param_key, "produce")) {
            val = atoi(line);
            if (val <= 0 || !real_object_proto(val)) {
                err = "nonexistent produced item";
                break;
            }
            shop->item_list = g_list_prepend(shop->item_list,
                                             GINT_TO_POINTER(atoi(line)));
        } else if (streq(param_key, "accept")) {
            if (!streq(line, "all")) {
                val = search_block(line, item_types, 0);
                if (val <= 0) {
                    err = "an invalid accept line";
                    break;
                }
            } else {
                val = 0;
            }
            shop->item_types = g_list_prepend(shop->item_types,
                                              GINT_TO_POINTER(1U << 8 | val));
        } else if (streq(param_key, "refuse")) {
            if (!streq(line, "all")) {
                val = search_block(line, item_types, 0);
                if (val <= 0) {
                    err = "an invalid accept line";
                    break;
                }
            } else {
                val = 0;
            }
            shop->item_types = g_list_prepend(shop->item_types,
                                              GINT_TO_POINTER(0 << 8 | val));
        } else if (streq(param_key, "denied-msg")) {
            free(shop->msg_denied);
            shop->msg_denied = strdup(line);
        } else if (streq(param_key, "keeper-broke-msg")) {
            free(shop->msg_selfbroke);
            shop->msg_selfbroke = strdup(line);
        } else if (streq(param_key, "buyer-broke-msg")) {
            free(shop->msg_buyerbroke);
            shop->msg_buyerbroke = strdup(line);
        } else if (streq(param_key, "buy-msg")) {
            free(shop->msg_buy);
            shop->msg_buy = strdup(line);
        } else if (streq(param_key, "sell-msg")) {
            free(shop->msg_sell);
            shop->msg_sell = strdup(line);
        } else if (streq(param_key, "closed-msg")) {
            free(shop->msg_closed);
            shop->msg_closed = strdup(line);
        } else if (streq(param_key, "no-buy-msg")) {
            free(shop->msg_badobj);
            shop->msg_badobj = strdup(line);
        } else if (streq(param_key, "sell-noobj-msg")) {
            free(shop->msg_sell_noobj);
            shop->msg_sell_noobj = strdup(line);
        } else if (streq(param_key, "buy-noobj-msg")) {
            free(shop->msg_buy_noobj);
            shop->msg_buy_noobj = strdup(line);
        } else if (streq(param_key, "temper-cmd")) {
            free(shop->cmd_temper);
            shop->cmd_temper = strdup(line);
        } else if (streq(param_key, "closed-hours")) {
            struct shop_time time;

            time.start = atoi(tmp_getword(&line));
            time.end = atoi(tmp_getword(&line));
            if (time.start < 0 || time.start > 24) {
                err = "an out of bounds closing hour";
            } else if (time.end < 0 || time.end > 24) {
                err = "an out of bounds opening hour";
            } else {
                struct shop_time *new_time;
                CREATE(new_time, struct shop_time, 1);
                *new_time = time;
                shop->closed_hours = g_list_prepend(shop->closed_hours,
                                                    new_time);
            }
        } else if (streq(param_key, "markup")) {
            shop->markup = atoi(line);
            if (shop->markup <= 0 || shop->markup > 1000) {
                err = "an invalid markup";
                break;
            }
        } else if (streq(param_key, "markdown")) {
            shop->markdown = atoi(line);
            if (shop->markdown <= 0 || shop->markdown > 1000) {
                err = "an invalid markdown";
                break;
            }
        } else if (streq(param_key, "revenue")) {
            shop->revenue = atoi(line);
            if (shop->revenue < 0) {
                err = "a negative revenue";
                break;
            }
        } else if (streq(param_key, "storeroom")) {
            shop->storeroom = atoi(line);
            if (!real_room(shop->storeroom)) {
                err = "a non-existent storeroom";
                break;
            }
        } else if (streq(param_key, "currency")) {
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
        } else if (streq(param_key, "steal-ok")) {
            shop->steal_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on")
                              || is_abbrev(line, "1") || is_abbrev(line, "true"));
        } else if (streq(param_key, "attack-ok")) {
            shop->attack_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on")
                               || is_abbrev(line, "1") || is_abbrev(line, "true"));
        } else if (streq(param_key, "consignment")) {
            shop->consignment = (is_abbrev(line, "yes") || is_abbrev(line, "on")
                                 || is_abbrev(line, "1") || is_abbrev(line, "true"));
        } else if (streq(param_key, "call-for-help")) {
            shop->call_for_help = (is_abbrev(line, "yes")
                                   || is_abbrev(line, "on") || is_abbrev(line, "1")
                                   || is_abbrev(line, "true"));
        } else if (streq(param_key, "special")) {
            val = find_spec_index_arg(line);
            if (val == -1) {
                err = tmp_sprintf("invalid special %s", line);
            } else {
                shop->func = spec_list[val].func;
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

SPECIAL(vendor)
{
    struct creature *self = (struct creature *)me;
    char *config;
    const char *err = NULL;
    int err_line;
    struct shop_data *shop;

    shop = (struct shop_data *)self->mob_specials.func_data;
    if (spec_mode == SPECIAL_FREE && shop) {
        free_shop(shop);
        self->mob_specials.func_data = NULL;
        return 1;
    }

    config = GET_NPC_PARAM(self);
    if (!config) {
        return 0;
    }

    shop = (struct shop_data *)self->mob_specials.func_data;
    if (!shop) {
        shop = make_shop();
        err = vendor_parse_param(config, shop, &err_line);
        if (err != NULL) {
            errlog("vendor %d spec error on line %d: %s",
                   GET_NPC_VNUM(self), err_line, err);
            self->mob_specials.shared->func = NULL;
            free_shop(shop);
            return 1;
        }
        self->mob_specials.func_data = shop;
    }
    // If there's a subspecial, try that first
    if (shop->func &&
        shop->func != vendor && shop->func(ch, me, cmd, argument, spec_mode)) {
        return 1;
    }

    if (spec_mode == SPECIAL_CMD &&
        !(CMD_IS("buy") || CMD_IS("sell") || CMD_IS("list") ||
          CMD_IS("value") || CMD_IS("offer") || CMD_IS("steal") ||
          CMD_IS("consign") || CMD_IS("unconsign"))) {
        return 0;
    }

    if (spec_mode == SPECIAL_RESET) {
        if (shop->func && shop->func != vendor) {
            shop->func(ch, me, cmd, argument, spec_mode);
        }
        vendor_revenue(self, shop);
        return 0;
    }

    if (spec_mode == SPECIAL_TICK) {
        struct creature *target = random_opponent(self);
        if (target && shop->call_for_help && !number(0, 4)) {
            call_for_help(self, target);
            return 1;
        }
        if (shop->func && shop->func != vendor) {
            return shop->func(ch, me, cmd, argument, spec_mode);
        }
        return 0;
    }

    if (spec_mode != SPECIAL_CMD) {
        if (shop->func && shop->func != vendor) {
            shop->func(ch, me, cmd, argument, spec_mode);
        }
        return 0;
    }

    if (err) {
        // Specparam error
        if (IS_PC(ch)) {
            if (IS_IMMORT(ch)) {
                perform_tell(self, ch,
                             tmp_sprintf("I have %s in line %d of my specparam", err,
                                         err_line));
            } else {
                mudlog(LVL_IMMORT, NRM, true,
                       "ERR: Mobile %d has %s in line %d of specparam",
                       GET_NPC_VNUM(self), err, err_line);
                perform_say_to(self, ch,
                               "Sorry.  I'm broken, but a god has already been notified.");
            }
        }
        return true;
    }

    if (CMD_IS("steal")) {
        if (shop->steal_ok && GET_LEVEL(ch) < LVL_IMMORT) {
            return false;
        }
        if (AWAKE(self)) {
            do_gen_comm(self,
                        tmp_capitalize(tmp_sprintf("%s is a bloody thief!",
                                                   GET_NAME(ch))), 0, SCMD_SHOUT);
        } else {
            act("You mumble about bloody thieves and slaps $n's hand away.", false,
                self, NULL, ch, TO_CHAR);
            act("$n mumbles about bloody thieves and slaps your hand away.", false,
                self, NULL, ch, TO_VICT);
            act("$n mumbles about bloody thieves and slaps $n's hand away.", false,
                self, NULL, ch, TO_NOTVICT);
        }
        return true;
    }

    if (!can_see_creature(self, ch)) {
        perform_say(self, "yell",
                    "Show yourself if you want to do business with me!");
        return true;
    }

    if (shop->room != -1 && shop->room != self->in_room->number) {
        perform_say_to(self, ch, "Catch me when I'm in my store.");
        return true;
    }

    if (shop->closed_hours) {
        GList *it;
        struct time_info_data local_time;

        set_local_time(self->in_room->zone, &local_time);
        for (it = shop->closed_hours; it; it = it->next) {
            struct shop_time *shop_time = it->data;
            if (local_time.hours >= shop_time->start &&
                local_time.hours < shop_time->end) {
                perform_say_to(self, ch, shop->msg_closed);
                return true;
            }
        }
    }

    if (react(shop->reaction, ch) != ALLOW) {
        perform_say_to(self, ch, shop->msg_denied);
        return true;
    }

    if (!IS_EVIL(self) && IS_CRIMINAL(ch)) {
        perform_say_to(self, ch, "I don't deal with CRIMINALS.");
        return true;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM)) {
        perform_say_to(self, ch, "You don't look in your right mind, there.");
        return true;
    }

    if (CMD_IS("buy")) {
        vendor_sell(ch, argument, self, shop);
    } else if (CMD_IS("sell")) {
        vendor_buy(ch, argument, self, shop);
    } else if (CMD_IS("list")) {
        vendor_list(ch, argument, self, shop);
    } else if (CMD_IS("value") || CMD_IS("offer")) {
        vendor_value(ch, argument, self, shop);
    } else if (CMD_IS("consign")) {
        vendor_consign(ch, argument, self, shop);
    } else if (CMD_IS("unconsign")) {
        vendor_unconsign(ch, argument, self, shop);
    } else {
        mudlog(LVL_IMPL, CMP, true, "Can't happen at %s:%d", __FILE__,
               __LINE__);
    }

    return true;
}
