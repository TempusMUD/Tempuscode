#include <vector>

#include "actions.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "utils.h"

struct ShopData {
	ShopData(void) : item_list() {};

	long room;				// Room of keeper
	vector<int> item_list;	// list of produced items
	vector<int> item_types;	// list of types of items keeper deals in
	char *msg_denied;		// Message sent to those of wrong race, creed, etc
	char *msg_badobj;		// Attempt to sell invalid obj to keeper
	char *msg_keeperbroke;	// Shop ran out of money
	char *msg_buyerbroke;	// Buyer doesn't have any money
	char *msg_buy;			// Keeper successfully bought something
	char *msg_sell;			// Keeper successfully sold something
	char *cmd_temper;		// Command to run after buyerbroke
	int buy_percent;		// Percent keeper marks down when buying
	int sell_percent;		// Percent keeper marks down when selling
	bool currency;			// True == cash, False == gold
	bool steal_ok;
	bool attack_ok;
	Reaction reaction;
};

static long
shop_get_value(Creature *keeper, Creature *ch, obj_data *obj, int percent)
{
	long cost;

	if (GET_OBJ_COST(obj) < 1 || IS_OBJ_STAT(obj, ITEM_NOSELL) ||
			!OBJ_APPROVED(obj)) {
		do_say(keeper, tmp_sprintf("%s I don't buy that sort of thing.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return -1;
	}

	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
		do_say(keeper, tmp_sprintf("%s I'm not buying something that's already broken.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return -1;
	}

	if (GET_OBJ_EXTRA3(obj) & ITEM3_HUNTED) {
		do_say(keeper, tmp_sprintf("%s This is hunted by the forces of Hell!  I'm not taking this!", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return -1;
	}

	// Adjust cost for wear and tear on a direct percentage basis
	if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1)
		percent = percent *  GET_OBJ_DAM(obj) / GET_OBJ_MAX_DAM(obj);

	// Adjust cost for missing charges
	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) {
		if (GET_OBJ_VAL(obj, 2) == 0) {
			do_say(keeper, tmp_sprintf("%s I don't buy used up wands or staves!", GET_NAME(ch)),
				0, SCMD_SAY_TO, NULL);
			return -1;
		}
		percent = percent * GET_OBJ_VAL(obj, 2) / GET_OBJ_VAL(obj, 1);
	}

	cost = GET_OBJ_COST(obj) * percent / 100;
	if (OBJ_REINFORCED(obj))
		cost += cost >> 2;
	if (OBJ_ENHANCED(obj))
		cost += cost >> 2;

	return cost;
}


static void
shopkeeper_buy(Creature *ch, char *arg, Creature *keeper, ShopData *shop)
{
	do_say(keeper,
		tmp_sprintf("%s Sorry, but you can't buy from me yet.", GET_NAME(ch)), 
		0, SCMD_SAY_TO, NULL);
}

static void
shopkeeper_sell(Creature *ch, char *arg, Creature *keeper, ShopData *shop)
{
	obj_data *obj;
	char *obj_str;
	long cost;
	int num = 1;

	if (!*arg) {
		send_to_char(ch, "What do you wish to sell?\r\n");
		return;
	}

	obj_str = tmp_getword(&arg);
	num = atoi(obj_str);
	if (num < 0) {
		do_say(keeper,
			tmp_sprintf("%s You want to sell a negative amount? Try buying.", GET_NAME(ch)), 
			0, SCMD_SAY_TO, NULL);
		return;
	}

	if (num)
		obj_str = tmp_getword(&arg);

	obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
	if (!obj) {
		send_to_char(ch, "You don't seem to have any %s.\r\n", obj_str);
		return;
	}

	cost = shop_get_value(keeper, ch, obj, shop->buy_percent);
	if (cost < 0)
		return;

	if ((shop->currency) ? GET_CASH(keeper):GET_GOLD(keeper) < cost) {
		do_say(keeper, tmp_sprintf("%s %s", GET_NAME(ch), shop->msg_keeperbroke),
			0, SCMD_SAY_TO, NULL);
		return;
	}

	do_say(keeper, tmp_sprintf("%s %s", GET_NAME(ch), shop->msg_sell),
		0, SCMD_SAY_TO, NULL);

	if (shop->currency)
		perform_give_credits(keeper, ch, cost);
	else
		perform_give_gold(keeper, ch, cost);

	obj_from_char(obj);
	obj_to_char(obj, keeper);

	// repair object
	if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1)
		GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);
}

static void
shopkeeper_list(Creature *ch, char *arg, Creature *keeper, ShopData *shop)
{
	do_say(keeper,
		tmp_sprintf("%s Sorry, but you can't list yet.", GET_NAME(ch)), 
		0, SCMD_SAY_TO, NULL);
}

static void
shopkeeper_value(Creature *ch, char *arg, Creature *keeper, ShopData *shop)
{
	obj_data *obj;
	char *obj_str;
	int cost;
	char *msg;

	if (!*arg) {
		send_to_char(ch, "What do you wish to value?\r\n");
		return;
	}

	obj_str = tmp_getword(&arg);
	obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
	if (!obj) {
		send_to_char(ch, "You don't seem to have any %s.\r\n", obj_str);
		return;
	}

	cost = shop_get_value(keeper, ch, obj, shop->buy_percent);
	if (cost < 0)
		return;

	msg = tmp_sprintf("%s I'll give you %d %s for it!", GET_NAME(ch),
		cost, shop->currency ? "cash":"gold");
	do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
}

SPECIAL(vendor)
{
	Creature *keeper;
	char *config, *line, *param_key;
	ShopData shop;

	if (spec_mode != SPECIAL_CMD)
		return 0;	

	keeper = (Creature *)me;
	config = GET_MOB_PARAM(keeper);
	if (!config)
		return 0;

	if (!(CMD_IS("buy") || CMD_IS("sell") || CMD_IS("list") || CMD_IS("value") || CMD_IS("steal")))
		return 0;

	// Initialize default values
	shop.msg_denied = "I don't buy that sort of thing.";
	shop.msg_badobj = "I can't buy that!";
	shop.msg_keeperbroke = "Sorry, but I don't have the cash.";
	shop.msg_buyerbroke = "You don't have enough money to buy this!";
	shop.msg_buy = "Here you go.";
	shop.msg_sell = "There you go.";
	shop.cmd_temper = NULL;
	shop.buy_percent = 70;
	shop.sell_percent = 103;
	shop.currency = false;
	shop.steal_ok = false;
	shop.attack_ok = false;

	while ((line = tmp_getline(&config)) != NULL) {
		if (shop.reaction.add_reaction(line))
			continue;

		param_key = tmp_getword(&line);
		if (!strcmp(param_key, "room")) {
			shop.room = atol(line);
		} else if (!strcmp(param_key, "produce")) {
			shop.item_list.push_back(atoi(line));
		} else if (!strcmp(param_key, "buy")) {
			
		} else if (!strcmp(param_key, "denied-msg")) {
			shop.msg_denied = line;
		} else if (!strcmp(param_key, "keeper-broke-msg")) {
			shop.msg_keeperbroke= line;
		} else if (!strcmp(param_key, "buyer-broke-msg")) {
			shop.msg_buyerbroke = line;
		} else if (!strcmp(param_key, "buy-msg")) {
			shop.msg_buy = line;
		} else if (!strcmp(param_key, "sell-msg")) {
			shop.msg_sell = line;
		} else if (!strcmp(param_key, "temper-cmd")) {
			shop.cmd_temper = line;
		} else if (!strcmp(param_key, "buy-percent")) {
			shop.buy_percent = atoi(line);
		} else if (!strcmp(param_key, "sell-percent")) {
			shop.sell_percent = atoi(line);
		} else if (!strcmp(param_key, "currency")) {
			if (is_abbrev(line, "cash"))
				shop.currency = true;
			else
				shop.currency = false;
		} else if (!strcmp(param_key, "steal-ok")) {
			shop.steal_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "attack-ok")) {
			shop.attack_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else {
			mudlog(LVL_IMMORT, CMP, true, "ERR: shopkeeper specparam");
		}
	}

	if (CMD_IS("steal") && !shop.steal_ok && GET_LEVEL(ch) < LVL_IMMORT) {
		do_gen_comm(keeper, tmp_sprintf("%s is a bloody thief!", GET_NAME(ch)),
			0, SCMD_SHOUT, 0);
		return 1;
	}
	
	if (shop.reaction.react(ch) != ALLOW) {
		do_say(keeper, tmp_sprintf("%s %s", GET_NAME(ch), shop.msg_denied),
			0, SCMD_SAY_TO, 0);
		return 1;
	}

	if (CMD_IS("buy"))
		shopkeeper_buy(ch, argument, keeper, &shop);
	else if (CMD_IS("sell"))
		shopkeeper_sell(ch, argument, keeper, &shop);
	else if (CMD_IS("list"))
		shopkeeper_list(ch, argument, keeper, &shop);
	else if (CMD_IS("value"))
		shopkeeper_value(ch, argument, keeper, &shop);
	
	return 1;
}
