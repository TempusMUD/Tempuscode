#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "screen.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "obj_data.h"
#include "actions.h"
#include "weather.h"
#include "vendor.h"
#include "strutil.h"

const int MAX_ITEMS = 10;

// From act.comm.cc
void perform_analyze(struct creature *ch, struct obj_data *obj, bool checklev);
void perform_appraise(struct creature *ch, struct obj_data *obj,
    int skill_lvl);

// From cityguard.cc
void call_for_help(struct creature *ch, struct creature *attacker);

// From vendor.cc
bool same_obj(struct obj_data *obj1, struct obj_data *obj2);
void vendor_appraise(struct creature *ch, struct obj_data *obj,
    struct creature *self, struct shop_data *shop);

static void
tattooist_show_pos(struct creature *me, struct creature *ch,
    struct obj_data *obj)
{
    int pos;
    bool not_first = false;

    strcpy(buf, "You can have tattoos in these positions: ");
    for (pos = 0; wear_tattoopos[pos][0] != '\n'; pos++)
        if (!ILLEGAL_TATTOOPOS(pos) && CAN_WEAR(obj, wear_bitvectors[pos])) {
            if (not_first)
                strcat(buf, ", ");
            else
                not_first = true;

            strcat(buf, wear_tattoopos[pos]);
        }

    perform_tell(me, ch, buf);
}

static unsigned long
tattooist_get_value(struct obj_data *obj, int percent)
{
    return GET_OBJ_COST(obj) * percent / 100;
}

static void
tattooist_sell(struct creature *ch, char *arg, struct creature *self,
    struct shop_data *shop)
{
    struct obj_data *obj;
    char *obj_str;
    const char *currency_str, *msg;
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
    // Check for hash mark
    if (*obj_str == '#')
        obj = vendor_resolve_hash(shop, self, obj_str);
    else
        obj = vendor_resolve_name(shop, self, obj_str);

    if (!obj) {
        perform_say_to(self, ch, shop->msg_sell_noobj);
        return;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_TATTOO) {
        perform_say_to(self, ch, "That's not a tattoo, now is it?");
        return;
    }

    if (appraisal_only) {
        vendor_appraise(ch, obj, self, shop);
        return;
    }

    char *pos_str = tmp_getword(&arg);
    if (!pos_str[0]) {
        perform_say_to(self, ch, "Where did you want that tattoo?");
        return;
    }
    int pos = search_block(pos_str, wear_tattoopos, 0);
    if (pos < 0 || ILLEGAL_TATTOOPOS(pos)) {
        msg = tmp_sprintf("'%s' isn't a valid position.", pos_str);
        perform_tell(self, ch, msg);
        tattooist_show_pos(self, ch, obj);
        return;
    }

    if (!CAN_WEAR(obj, wear_bitvectors[pos])) {
        msg = tmp_sprintf("%s cannot be inked there.", obj->name);
        perform_tell(self, ch, msg);
        tattooist_show_pos(self, ch, obj);
        return;
    }
    if (GET_TATTOO(ch, pos)) {
        perform_tell(self, ch, "You already have a tattoo inscribed there.");
        return;
    }

    for (int idx = 0; idx < NUM_WEARS; idx++) {
        if (GET_TATTOO(ch, idx) &&
            GET_OBJ_VNUM(GET_TATTOO(ch, idx)) == GET_OBJ_VNUM(obj)) {
            perform_tell(self, ch,
                tmp_sprintf("You already have that tattoo on your %s!",
                    wear_tattoopos[idx]));
            return;
        }
    }

    cost = tattooist_get_value(obj, shop->markup);
    cost = adjusted_price(ch, self, cost);
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
        if (shop->cmd_temper)
            command_interpreter(self, tmp_strdup(shop->cmd_temper));
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
        account_set_quest_points(ch->account,
            ch->account->quest_points - cost);
        currency_str = "quest points";
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        currency_str = "-BUGS-";
    }

    perform_say_to(self, ch, tmp_sprintf(shop->msg_buy, cost));
    msg = tmp_sprintf("You carefully ink $p onto $N's %s for %lu %s.",
        wear_tattoopos[pos], cost, currency_str);
    act(msg, false, self, obj, ch, TO_CHAR);
    msg = tmp_sprintf("$n carefully inks $p onto your %s for %lu %s.",
        wear_tattoopos[pos], cost, currency_str);
    act(msg, false, self, obj, ch, TO_VICT);
    msg = tmp_sprintf("$n carefully inks $p onto $N's %s.",
        wear_tattoopos[pos]);
    act(msg, false, self, obj, ch, TO_NOTVICT);

    // Load all-new item
    obj = read_object(GET_OBJ_VNUM(obj));
    obj->creation_method = 5;
    obj->creator = GET_NPC_VNUM(self);
    equip_char(ch, obj, pos, EQUIP_TATTOO);

    WAIT_STATE(ch, 5 RL_SEC);
}

char *
tattooist_list_obj(struct creature *ch, struct obj_data *obj, int idx,
    int cost)
{
    char *obj_desc;

    obj_desc = obj->name;
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
        if (IS_OBJ_STAT(obj, ITEM_BLESS))
            obj_desc = tmp_strcat(obj_desc, " (holy aura)", NULL);
        if (IS_OBJ_STAT(obj, ITEM_DAMNED))
            obj_desc = tmp_strcat(obj_desc, " (unholy aura)", NULL);
    }

    obj_desc = tmp_capitalize(obj_desc);
    return tmp_sprintf(" %2d%s)  %s%-48s %'6d\r\n",
        idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM), obj_desc, cost);
}

static void
tattooist_list(struct creature *ch, char *arg, struct creature *self,
    struct shop_data *shop)
{
    struct obj_data *cur_obj;
    int idx;
    const char *msg;
    unsigned long cost;

    if (!self->carrying) {
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
        " ##   Item                                       ", msg,
        "\r\n",
        "-------------------------------------------------------------------------\r\n",
        CCNRM(ch, C_NRM), NULL);

    idx = 1;
    for (cur_obj = self->carrying; cur_obj; cur_obj = cur_obj->next_content) {
        if (!*arg || namelist_match(arg, cur_obj->aliases)) {
            cost = tattooist_get_value(cur_obj, shop->markup);
            cost = adjusted_price(ch, self, cost);
            msg = tmp_strcat(msg, tattooist_list_obj(ch, cur_obj, idx, cost), NULL);
            idx++;
        }
    }

    act("$n peruses the tattooists offerings.", false, ch, NULL, NULL, TO_ROOM);
    page_string(ch->desc, msg);
}

SPECIAL(tattooist)
{
    struct creature *self = (struct creature *)me;
    char *config;
    const char *err = NULL;
    int err_line;
    struct shop_data *shop;

    config = GET_NPC_PARAM(self);
    if (!config)
        return 0;

    shop = (struct shop_data *)self->mob_specials.func_data;
    if (!shop) {
        CREATE(shop, struct shop_data, 1);
        err = vendor_parse_param(config, shop, &err_line);
        self->mob_specials.func_data = shop;
    }

    if (shop->func &&
        shop->func != tattooist &&
        shop->func(ch, me, cmd, argument, spec_mode))
        return 1;

    if (spec_mode == SPECIAL_TICK) {
        struct creature *target = random_opponent(self);
        if (target && shop->call_for_help && !number(0, 4)) {
            call_for_help(self, target);
            return 1;
        }
        return 0;
    }

    if (spec_mode != SPECIAL_CMD)
        return 0;

    if (!(CMD_IS("buy") || CMD_IS("list") || CMD_IS("steal")))
        return 0;

    if (err) {
        // Specparam error
        if (IS_PC(ch)) {
            if (IS_IMMORT(ch))
                perform_tell(self, ch,
                    tmp_sprintf("I have %s in line %d of my specparam", err,
                        err_line));
            else {
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
        if (GET_LEVEL(ch) < LVL_IMMORT)
            return false;
        do_gen_comm(self,
            tmp_capitalize(tmp_sprintf("%s is a bloody thief!",
                    GET_NAME(ch))), 0, SCMD_SHOUT);
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

    if (!shop->closed_hours) {
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

    if (CMD_IS("list"))
        tattooist_list(ch, argument, self, shop);
    else if (CMD_IS("buy"))
        tattooist_sell(ch, argument, self, shop);
    else
        mudlog(LVL_IMPL, CMP, true, "Can't happen at %s:%d", __FILE__,
            __LINE__);

    return true;
}
