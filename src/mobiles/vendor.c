#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "actions.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "weather.h"
#include "utils.h"
#include "vendor.h"
#include "specs.h"

const int MAX_ITEMS = 10;
const unsigned int MIN_COST = 12;

// From act.comm.cc
void perform_analyze( struct creature *ch, struct obj_data *obj, bool checklev );
void perform_appraise( struct creature *ch, struct obj_data *obj, int skill_lvl);

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
        fprintf(vendor_log_file, "%s _ %s\n",
                tmp_ctime(time(NULL)),
                tmp_vsprintf(fmt, args));
        va_end(args);
    }
}

static bool
vendor_is_produced(struct obj_data *obj, ShopData *shop)
{
	vector<int>_iterator it;

	for (it = shop->item_list.begin();it != shop->item_list.end();it++)
		if (GET_OBJ_VNUM(obj) == *it)
			return same_obj(real_object_proto(*it), obj);

	return false;
}

int
vendor_inventory(struct obj_data *obj, struct obj_data *obj_list)
{
	struct obj_data *cur_obj;
	int cnt = 0;

	cur_obj = obj_list;
	while (cur_obj && GET_OBJ_VNUM(cur_obj) != GET_OBJ_VNUM(obj) &&
			!same_obj(cur_obj, obj))
		cur_obj = cur_obj->next_content;

	if (!cur_obj)
		return 0;

	while (same_obj(cur_obj, obj)) {
		cur_obj = cur_obj->next_content;
		cnt++;
	}

	return cnt;
}

static bool
vendor_invalid_buy(struct creature *self, struct creature *ch, ShopData *shop, struct obj_data *obj)
{
	if (IS_OBJ_STAT(obj, ITEM_NOSELL) ||
			!OBJ_APPROVED(obj)|| obj->shared->owner_id != 0 ) {
		perform_say_to(self, ch, shop->msg_badobj);
		return true;
	}

    if (GET_OBJ_COST(obj) < 1) {
		perform_say_to(self, ch, "Why would I want that?  It has no value.");
		return true;
    }

	if (shop->item_types.size() > 0) {
		vector<int>_iterator it;
		bool accepted = false;
		for (it = shop->item_types.begin();it != shop->item_types.end();it++) {
			if ((*it & 0xFF) == GET_OBJ_TYPE(obj) || (*it & 0xFF) == 0) {
				accepted = *it >> 8;
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
		perform_say_to(self, ch, "I'm not buying something that's already broken.");
		return true;
	}

	if (GET_OBJ_EXTRA3(obj) & ITEM3_HUNTED) {
		perform_say_to(self, ch, "This is hunted by the forces of Hell!  I'm not taking this!");
		return true;
	}

	if (GET_OBJ_SIGIL_IDNUM(obj)) {
		perform_say_to(self, ch, "You'll have to remove that warding sigil before I'll bother.");
		return true;
	}

	if (vendor_inventory(obj, self->carrying) >= MAX_ITEMS) {
		perform_say_to(self, ch, "No thanks.  I've got too many of those in stock already.");
		return true;
	}

	// Adjust cost for missing charges
	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) {
		if (GET_OBJ_VAL(obj, 2) == 0) {
			perform_say_to(self, ch, "I don't buy used up wands or staves!");
			return true;
		}
	}
	return false;
}

// Gets the value of an object, checking for buyability.
// costModifier of 0 does nothing.
static unsigned long
vendor_get_value(struct obj_data *obj, int percent, int costModifier, int currency)
{
	unsigned long cost;

	// Adjust cost for wear and tear on a direct percentage basis
	if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1 &&
			GET_OBJ_MAX_DAM(obj) != 0)
		percent = percent *  GET_OBJ_DAM(obj) / GET_OBJ_MAX_DAM(obj);

	// Adjust cost for missing charges
	if ((GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) &&
			GET_OBJ_VAL(obj, 1) != 0)
		percent = percent * GET_OBJ_VAL(obj, 2) / GET_OBJ_VAL(obj, 1);

	cost = GET_OBJ_COST(obj) * percent / 100;
	if (OBJ_REINFORCED(obj))
		cost += cost >> 2;
	if (OBJ_ENHANCED(obj))
		cost += cost >> 2;

    cost += (costModifier*(int)cost)/100;

    if (currency == 2)
        return MAX(1, cost);
    else
        return MAX(MIN_COST, cost);
}

struct obj_data *
vendor_resolve_hash(struct creature *self, char *obj_str)
{
	struct obj_data *last_obj = NULL, *cur_obj;
	int num;

	if (*obj_str != '#') {
		errlog("Can't happen in vendor_resolve_hash()");
		return NULL;
	}

	num = atoi(obj_str + 1);
	if (num <= 0)
		return NULL;

	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content) {
		if (!last_obj || !same_obj(last_obj, cur_obj))
			if (--num == 0)
				return cur_obj;
		last_obj = cur_obj;
	}

	return cur_obj;
}

struct obj_data *
vendor_resolve_name(struct creature *self, char *obj_str)
{
	struct obj_data *cur_obj;

	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content)
		if (namelist_match(obj_str, cur_obj->aliases))
			return cur_obj;

	return NULL;
}

void
vendor_appraise(struct creature *ch, struct obj_data *obj, struct creature *self, ShopData *shop)
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
		amt_carried = GET_GOLD(ch); break;
	case 1:
		amt_carried = GET_CASH(ch); break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		amt_carried = 0;
		break;
	}
	if (cost > amt_carried) {
		perform_say_to(self, ch, shop->msg_buyerbroke);
		if (shop->cmd_temper)
			command_interpreter(self, tmp_gsub(shop->cmd_temper, "$N", GET_NAME(ch)));
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
			ch->account->set_quest_points(ch->account->get_quest_points() - cost);
			currency_str = "quest points";
			break;
		default:
			errlog("Can't happen at %s:%d", __FILE__, __LINE__);
			currency_str = "-BUGS-";
	}

	perform_say_to(self, ch,
		tmp_sprintf("That will cost you %lu %s.", cost, currency_str));

	if( IS_MAGE(self) ) {
		spell_identify(50, ch, NULL, obj, NULL );
	} else if( IS_PHYSIC(self) || IS_CYBORG(self) ) {
		perform_analyze(ch,obj,false);
	} else {
		perform_appraise(ch,obj, 100);
	}
}

static void
vendor_sell(struct creature *ch, char *arg, struct creature *self, ShopData *shop)
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
			perform_say_to(self, ch, "You want to buy a negative amount? Try selling.");
			return;
		} else if (num == 0) {
			perform_say_to(self, ch, "You wanted to buy something?");
			return;
		} else if (num > 24 ) {
			perform_say_to(self, ch, "I can't sell that many at once.");
			return;
        }

		obj_str = tmp_getword(&arg);
	} else if (!strcmp(obj_str, "all")) {
		num = -1;
		obj_str = tmp_getword(&arg);
	} else {
		num = 1;
	}

	// Check for hash mark
	if (*obj_str == '#')
		obj = vendor_resolve_hash(self, obj_str);
	else
		obj = vendor_resolve_name(self, obj_str);

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
		int obj_cnt = vendor_inventory(obj, self->carrying);
		if (!vendor_is_produced(obj, shop) && num > obj_cnt) {
			perform_say_to(self, ch,
				tmp_sprintf("I only have %d to sell to you.", obj_cnt));
			num = obj_cnt;
		}
	}

	cost = vendor_get_value(obj,
                            shop->markup,
                            ch->getCostModifier(self),
                            shop->currency);
	switch (shop->currency) {
	case 0:
		amt_carried = GET_GOLD(ch); break;
	case 1:
		amt_carried = GET_CASH(ch); break;
	case 2:
		amt_carried = ch->account->get_quest_points(); break;
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

	if (cost * num > amt_carried && cost > 0) {
		num = amt_carried / cost;
		perform_say_to(self, ch,
			tmp_sprintf("You only have enough to buy %d.", num));
	}

	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
		perform_say_to(self, ch, "You can't carry any more items.");
		return;
	}

	if (IS_CARRYING_W(ch) + obj->getWeight() > CAN_CARRY_W(ch)) {
		switch (number(0,2)) {
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
			IS_CARRYING_W(ch) + obj->getWeight() * num > CAN_CARRY_W(ch)) &&
			obj->getWeight() > 0) {
		num = MIN(num, CAN_CARRY_N(ch) - IS_CARRYING_N(ch));
		num = MIN(num, (CAN_CARRY_W(ch) - IS_CARRYING_W(ch))
			/ obj->getWeight());
		perform_say_to(self, ch,
			tmp_sprintf("You can only carry %d.", num));
	}

	switch (shop->currency) {
	case 0:
		GET_GOLD(ch) -= cost * num;
		GET_GOLD(self) += cost * num;
		currency_str = "gold";
		break;
	case 1:
		GET_CASH(ch) -= cost * num;
		GET_CASH(self) += cost * num;
		currency_str = "creds";
		break;
	case 2:
		ch->account->set_quest_points(ch->account->get_quest_points() - (cost * num));
		currency_str = "quest points";
		break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		currency_str = "-BUGS-";
	}

	perform_say_to(self, ch, tmp_sprintf(shop->msg_buy, cost * num));
	msg = tmp_sprintf("You sell $p %sto $N for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost * num,
		currency_str);
	act(msg, false, self, obj, ch, TO_CHAR);
	msg = tmp_sprintf("$n sells $p %sto you for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost * num,
		currency_str);
	act(msg, false, self, obj, ch, TO_VICT);
	act("$n sells $p to $N.", false, self, obj, ch, TO_NOTVICT);

    vendor_log("%s[%d] sold %s[%d] (x%d) to %s %s[%d] for %lu %s",
               GET_NAME(self), GET_MOB_VNUM(self),
               obj->name, GET_OBJ_VNUM(obj),
               num,
               IS_NPC(ch) ? "NPC":"PC",
               GET_NAME(ch), IS_NPC(ch) ? GET_MOB_VNUM(ch):GET_IDNUM(ch),
               cost * num,
               currency_str);

	if (vendor_is_produced(obj, shop)) {
		// Load all-new identical items
		while (num--) {
			obj = read_object(GET_OBJ_VNUM(obj));
			obj_to_char(obj, ch);
		}
	} else {
		// Actually move the items from vendor to player
		while (num--) {
			next_obj = obj->next_content;
			obj_from_char(obj);
			obj_to_char(obj, ch);
			obj = next_obj;
		}
	}

}

static void
vendor_buy(struct creature *ch, char *arg, struct creature *self, ShopData *shop)
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
			perform_say_to(self, ch, "You want to sell a negative amount? Try buying.");
			return;
		} else if (num == 0) {
			perform_say_to(self, ch, "You wanted to sell something?");
			return;
		}

		obj_str = tmp_getword(&arg);
	} else if (!strcmp(obj_str, "all")) {
		num = -1;
		obj_str = tmp_getword(&arg);
	} else
		num = 1;

	obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
	if (!obj) {
		perform_say_to(self, ch, shop->msg_buy_noobj);
		return;
	}

	if (vendor_invalid_buy(self, ch, shop, obj))
		return;

	if (num == -1)
		num = vendor_inventory(obj, obj);

	// We only check inventory after the object selected.  This allows people
	// to sell 5 3.shirt if they have 2 shirts ahead of the one they want to
	// sell
	if (vendor_inventory(obj, obj) < num) {
		send_to_char(ch, "You only have %d of those!\r\n",
			vendor_inventory(obj, ch->carrying));
		return;
	}

	if (vendor_is_produced(obj, shop)) {
		perform_say_to(self, ch, "I make these.  Why should I buy it back from you?");
		return;
	}
	cost = vendor_get_value(obj,
                            shop->markdown,
                            self->getCostModifier(ch),
                            shop->currency);
	amt_carried = (shop->currency) ? GET_CASH(self):GET_GOLD(self);

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
		num = MAX_ITEMS - vendor_inventory(obj, self->carrying);
		perform_say_to(self, ch,
				tmp_sprintf("I only want to buy %d.", num));
	}

	perform_say_to(self, ch, tmp_sprintf(shop->msg_sell, cost * num));

	transfer_money(self, ch, cost * num, shop->currency, false);

    vendor_log("%s[%d] bought %s[%d] (x%d) from %s %s[%d] for %lu %s",
               GET_NAME(self), GET_MOB_VNUM(self),
               obj->name, GET_OBJ_VNUM(obj),
               num,
               IS_NPC(ch) ? "NPC":"PC",
               GET_NAME(ch), IS_NPC(ch) ? GET_MOB_VNUM(ch):GET_IDNUM(ch),
               cost * num,
               shop->currency ? "creds":"gold");
	// We've already verified that they have enough of the item via a
	// call to vendor_inventory(), so we can just blindly transfer objects
	while (num-- && obj) {
		// transfer object
		next_obj = obj->next_content;
		obj_from_char(obj);
		obj_to_char(obj, self);

		// repair object
		if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1)
			GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);

		obj = next_obj;
		}
    if (GET_IDNUM(ch) > 0) {
	    save_player_to_xml(ch);
    }
}

char *
vendor_list_obj(struct creature *ch, struct obj_data *obj, int cnt, int idx, int cost)
{
	char *obj_desc;

	obj_desc = obj->name;
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON && GET_OBJ_VAL(obj, 1))
		obj_desc = tmp_strcat(obj_desc, " of ",
			liquid_to_str(GET_OBJ_VAL(obj,2)), NULL);
	if ((GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) &&
			GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
		obj_desc = tmp_strcat(obj_desc, " (partially used)");
	if (OBJ_REINFORCED(obj))
		obj_desc = tmp_strcat(obj_desc, " [reinforced]");
	if (OBJ_ENHANCED(obj))
		obj_desc = tmp_strcat(obj_desc, " |augmented|");
	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
		obj_desc = tmp_strcat(obj_desc, " <broken>");
	if (IS_OBJ_STAT(obj, ITEM_HUM))
		obj_desc = tmp_strcat(obj_desc, " (humming)");
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		obj_desc = tmp_strcat(obj_desc, " (glowing)");
	if (IS_OBJ_STAT(obj, ITEM_INVISIBLE))
		obj_desc = tmp_strcat(obj_desc, " (invisible)");
	if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT))
		obj_desc = tmp_strcat(obj_desc, " (transparent)");
	if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
		if (IS_OBJ_STAT(obj, ITEM_BLESS))
			obj_desc = tmp_strcat(obj_desc, " (holy aura)");
		if (IS_OBJ_STAT(obj, ITEM_DAMNED))
			obj_desc = tmp_strcat(obj_desc, " (unholy aura)");
	}

	obj_desc = tmp_capitalize(obj_desc);
	if (cnt < 0)
		return tmp_sprintf(" %2d%s)  %sUnlimited%s   %-48s %6d\r\n",
			idx, CCRED(ch, C_NRM), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			obj_desc, cost);

	return tmp_sprintf(" %2d%s)  %s%5d%s       %-48s %6d\r\n",
		idx, CCRED(ch, C_NRM), CCYEL(ch, C_NRM), cnt, CCNRM(ch, C_NRM),
		obj_desc, cost);
}

static void
vendor_list(struct creature *ch, char *arg, struct creature *self, ShopData *shop)
{
	struct obj_data *cur_obj, *last_obj;
	int idx, cnt;
	const char *msg;
    unsigned long cost;

	if (!self->carrying) {
		perform_say_to(self, ch, "I'm out of stock at the moment.");
		return;
	}

	switch (shop->currency) {
	case 0:
		msg = "        Gold"; break;
	case 1:
		msg = "       Creds"; break;
	case 2:
		msg = "Quest Points"; break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		msg = "PleaseReport";
	}

	msg = tmp_strcat(CCCYN(ch, C_NRM),
" ##   Available   Item                                       ", msg,
"\r\n",
"-------------------------------------------------------------------------\r\n"
		, CCNRM(ch, C_NRM), NULL);

	last_obj = NULL;
	cnt = idx = 1;
	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content) {
		if (last_obj) {
			if (same_obj(last_obj, cur_obj)) {
				cnt++;
			} else {
				if (vendor_is_produced(last_obj, shop))
					cnt = -1;
				if (!*arg || namelist_match(arg, last_obj->aliases)) {
					cost = vendor_get_value(last_obj,
                                            shop->markup,
                                            ch->getCostModifier(self),
                                            shop->currency);
                    msg = tmp_strcat(msg, vendor_list_obj(ch, last_obj, cnt, idx, cost));
                }
				cnt = 1;
				idx++;
			}
		}
		last_obj = cur_obj;
	}
	if (last_obj) {
		if (vendor_is_produced(last_obj, shop))
			cnt = -1;
		if (!*arg || namelist_match(arg, last_obj->aliases))
			msg = tmp_strcat(msg, vendor_list_obj(ch, last_obj, cnt, idx,
				vendor_get_value(last_obj,
                                 shop->markup,
                                 ch->getCostModifier(self),
                                 shop->currency)));
	}

	act("$n peruses the shop's wares.", false, ch, 0, 0, TO_ROOM);
	page_string(ch->desc, msg);
}

static void
vendor_value(struct creature *ch, char *arg, struct creature *self, ShopData *shop)
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

	if (vendor_invalid_buy(self, ch, shop, obj))
		return;

	cost = vendor_get_value(obj,
                            shop->markdown,
                            self->getCostModifier(ch),
                            shop->currency);

	perform_say_to(self, ch, tmp_sprintf("I'll give you %lu %s for it!", cost, shop->currency ? "creds":"gold"));
}

static void
vendor_revenue(struct creature *self, ShopData *shop)
{
	struct creature *vkeeper;
	long cur_money, max_money;

	vkeeper = real_mobile_proto(GET_MOB_VNUM(self));
	max_money = (shop->currency) ? GET_CASH(vkeeper):GET_GOLD(vkeeper);
	cur_money = (shop->currency) ? GET_CASH(self):GET_GOLD(self);
	if (cur_money >= max_money)
		return;
	if (shop->currency)
		GET_CASH(self) = MIN(max_money, GET_CASH(self) + shop->revenue);
	else
		GET_GOLD(self) = MIN(max_money, GET_GOLD(self) + shop->revenue);
	return;
}

const char *
vendor_parse_param(char *param, ShopData *shop, int *err_line)
{
	char *line, *param_key;
	const char *err = NULL;
	int val, lineno = 0;

	// Initialize default values
	shop->room = -1;
	shop->msg_denied = "I'm not doing business with YOU!";
	shop->msg_badobj = "I don't buy that sort of thing.";
	shop->msg_selfbroke = "Sorry, but I don't have the cash.";
	shop->msg_buyerbroke = "You don't have enough money to buy this!";
	shop->msg_sell_noobj= "Sorry, but I don't carry that item.";
	shop->msg_buy_noobj= "You don't have that item!";
	shop->msg_buy = "Here you go.";
	shop->msg_sell = "There you go.";
	shop->msg_closed = "Come back later!";
	shop->cmd_temper = NULL;
	shop->markdown = 70;
	shop->markup = 120;
	shop->currency = false;
	shop->revenue = 0;
	shop->steal_ok = false;
	shop->attack_ok = false;
	shop->func = NULL;

	while ((line = tmp_getline(&param)) != NULL) {
		lineno++;
		if (shop->reaction.add_reaction(line))
			continue;

		param_key = tmp_getword(&line);
		if (!strcmp(param_key, "room")) {
			shop->room = atol(line);
		} else if (!strcmp(param_key, "produce")) {
			val = atoi(line);
			if (val <= 0 || !real_object_proto(val)) {
				err = "nonexistent produced item";
				break;
			}
			shop->item_list.push_back(atoi(line));
		} else if (!strcmp(param_key, "accept")) {
			if (strcmp(line, "all")) {
				val = search_block(line, item_types, 0);
				if (val <= 0) {
					err = "an invalid accept line";
					break;
				}
			} else
				val = 0;
			shop->item_types.push_back( 1 << 8 | val);
		} else if (!strcmp(param_key, "refuse")) {
			if (strcmp(line, "all")) {
				val = search_block(line, item_types, 0);
				if (val <= 0) {
					err = "an invalid accept line";
					break;
				}
			} else
				val = 0;
			shop->item_types.push_back( 0 << 8 | val);
		} else if (!strcmp(param_key, "denied-msg")) {
			shop->msg_denied = strdup(line);
		} else if (!strcmp(param_key, "keeper-broke-msg")) {
			shop->msg_selfbroke= strdup(line);
		} else if (!strcmp(param_key, "buyer-broke-msg")) {
			shop->msg_buyerbroke = strdup(line);
		} else if (!strcmp(param_key, "buy-msg")) {
			shop->msg_buy = strdup(line);
		} else if (!strcmp(param_key, "sell-msg")) {
			shop->msg_sell = strdup(line);
		} else if (!strcmp(param_key, "closed-msg")) {
			shop->msg_closed = strdup(line);
		} else if (!strcmp(param_key, "no-buy-msg")) {
			shop->msg_badobj = strdup(line);
		} else if (!strcmp(param_key, "sell-noobj-msg")) {
			shop->msg_sell_noobj = strdup(line);
		} else if (!strcmp(param_key, "buy-noobj-msg")) {
			shop->msg_buy_noobj = strdup(line);
		} else if (!strcmp(param_key, "temper-cmd")) {
			shop->cmd_temper = strdup(line);
		} else if (!strcmp(param_key, "closed-hours")) {
			ShopTime time;

			time.start = atoi(tmp_getword(&line));
			time.end = atoi(tmp_getword(&line));
			if (time.start < 0 || time.start > 24)
				err = "an out of bounds closing hour";
			else if (time.end < 0 || time.end > 24)
				err = "an out of bounds opening hour";
			else
				shop->closed_hours.push_back(time);
		} else if (!strcmp(param_key, "markup")) {
			shop->markup= atoi(line);
			if (shop->markup <= 0 ||  shop->markup > 1000) {
				err = "an invalid markup";
				break;
			}
		} else if (!strcmp(param_key, "markdown")) {
			shop->markdown= atoi(line);
			if (shop->markdown <= 0 ||  shop->markdown > 1000) {
				err = "an invalid markdown";
				break;
			}
		} else if (!strcmp(param_key, "revenue")) {
			shop->revenue= atoi(line);
			if (shop->revenue < 0) {
				err = "a negative revenue";
				break;
			}
		} else if (!strcmp(param_key, "currency")) {
			if (is_abbrev(line, "past") || is_abbrev(line, "gold"))
				shop->currency = 0;
			else if (is_abbrev(line, "future") || is_abbrev(line, "cash"))
				shop->currency = 1;
			else if (is_abbrev(line, "qp") || is_abbrev(line, "quest"))
				shop->currency = 2;
			else {
				err = "invalid currency";
				break;
			}
		} else if (!strcmp(param_key, "steal-ok")) {
			shop->steal_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "attack-ok")) {
			shop->attack_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "call-for-help")) {
			shop->call_for_help = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "special")) {
			val = find_spec_index_arg(line);
			if (val == -1)
				err = "invalid special";
			else
				shop->func = spec_list[val].func;
		} else {
			err = "invalid directive";
			break;
		}
	}

	if (err_line)
		*err_line = (err) ? lineno:-1;

	return err;
}

SPECIAL(vendor)
{
	struct creature *self = (struct creature *)me;
	char *config;
    const char *err = NULL;
	int err_line;
	ShopData *shop;

	config = GET_MOB_PARAM(self);
	if (!config)
		return 0;

	shop = (ShopData *)self->mob_specials.func_data;
	if (!shop) {
		CREATE(shop, ShopData, 1);
		err = vendor_parse_param(config, shop, &err_line);
		self->mob_specials.func_data = shop;
	}

    // If there's a subspecial, try that first
    if (shop->func &&
        shop->func != vendor &&
        shop->func(ch, me, cmd, argument, spec_mode))
        return 1;

	if (spec_mode == SPECIAL_CMD &&
        !(CMD_IS("buy")   || CMD_IS("sell")  || CMD_IS("list")  ||
          CMD_IS("value") || CMD_IS("offer") || CMD_IS("steal"))) {
        return 0;
    }

	if (spec_mode == SPECIAL_RESET) {
        if (shop->func && shop->func != vendor)
            shop->func(ch, me, cmd, argument, spec_mode);
		vendor_revenue(self, shop);
		return 0;
	}

	if (spec_mode == SPECIAL_TICK) {
        struct creature *target = self->findRandomCombat();
		if (target && shop->call_for_help && !number(0, 4)) {
			call_for_help(self, target);
			return 1;
		}
        if (shop->func && shop->func != vendor)
            return shop->func(ch, me, cmd, argument, spec_mode);
		return 0;
	}

    if (spec_mode != SPECIAL_CMD) {
        if (shop->func && shop->func != vendor)
            shop->func(ch, me, cmd, argument, spec_mode);
        return 0;
    }

	if (err) {
		// Specparam error
		if (IS_PC(ch)) {
			if (IS_IMMORT(ch))
				perform_tell(self, ch, tmp_sprintf(
					"I have %s in line %d of my specparam", err, err_line));
			else {
				mudlog(LVL_IMMORT, NRM, true,
					"ERR: Mobile %d has %s in line %d of specparam",
					GET_MOB_VNUM(self), err, err_line);
				perform_say_to(self, ch, "Sorry.  I'm broken, but a god has already been notified.");
			}
		}
		return true;
	}

	if (CMD_IS("steal")) {
		if (shop->steal_ok && GET_LEVEL(ch) < LVL_IMMORT)
			return false;
		do_gen_comm(self,
                    tmp_capitalize(tmp_sprintf("%s is a bloody thief!",
                                               GET_NAME(ch))),
                    0, SCMD_SHOUT, 0);
		return true;
	}

	if (!can_see_creature(self, ch)) {
		perform_say(self, "yell", "Show yourself if you want to do business with me!");
		return true;
	}

	if (shop->room != -1 && shop->room != self->in_room->number) {
		perform_say_to(self, ch, "Catch me when I'm in my store.");
		return true;
	}

	if (!shop->closed_hours.empty()) {
		vector<ShopTime>_iterator shop_time;
		struct time_info_data local_time;

		set_local_time(self->in_room->zone, &local_time);
		for (shop_time = shop->closed_hours.begin();shop_time != shop->closed_hours.end();shop_time++)
			if (local_time.hours >= shop_time->start &&
					local_time.hours < shop_time->end ) {
				perform_say_to(self, ch, shop->msg_closed);
				return true;
			}
	}

	if (shop->reaction.react(ch) != ALLOW) {
		perform_say_to(self, ch, shop->msg_denied);
		return true;
	}

	if (!IS_EVIL(self) && IS_CRIMINAL(ch)) {
		perform_say_to(self, ch, "I don't deal with CRIMINALS.");
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
	} else {
		mudlog(LVL_IMPL, CMP, true, "Can't happen at %s:%d", __FILE__,
			__LINE__);
	}

	return true;
}