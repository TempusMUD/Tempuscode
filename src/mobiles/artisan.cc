#include <vector>
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

SPECIAL(artisan);

struct craft_component {
	short item;
	short material;
	short weight;
	short value;
	short amount;
};

class craft_item {
	public:
		char *next_requirement(Creature *keeper);
		obj_data *create(Creature *keeper, Creature *recipient);

		short vnum;	// object to be offered when all components are held
		long cost; // -1 means to use default cost
		int fail_pct;
		vector<craft_component *> required;
};

struct craftshop {
		short room;
		short keeper_vnum;
		vector<craft_item *> items;
};

static int cmd_slap, cmd_smirk, cmd_cry;
vector<craftshop *> shop_list;

void
craft_parse_item(craftshop *shop, xmlNodePtr node)
{
	xmlNodePtr sub_node;
	craft_item *new_item;
	craft_component *compon;
	
	new_item = new craft_item;
	new_item->vnum = xmlGetIntProp(node, "vnum");
	new_item->cost = xmlGetIntProp(node, "cost");
	new_item->fail_pct = xmlGetIntProp(node, "failure", 0);
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		if (xmlMatches(sub_node->name, "requires")) {
			compon = new craft_component;
			compon->item = xmlGetIntProp(sub_node, "item");
			compon->material = xmlGetIntProp(sub_node, "material");
			compon->weight = xmlGetIntProp(sub_node, "weight");
			compon->value = xmlGetIntProp(sub_node, "value");
			compon->amount = xmlGetIntProp(sub_node, "amount");
			new_item->required.insert(new_item->required.end(), compon);
		} else {
			slog("Invalid XML tag while parsing artisan");
		}
	}
	shop->items.insert(shop->items.end(), new_item);
	return;
}

void
craftshop_load(xmlNodePtr node)
{
	xmlNodePtr sub_node;
	craftshop *new_shop;
	xmlChar *prop;

	new_shop = new craftshop;
	new_shop->room = xmlGetIntProp(node, "room");
	new_shop->keeper_vnum = xmlGetIntProp(node, "keeper");
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		prop = NULL;
		if (xmlMatches(sub_node->name, "item")) {
			craft_parse_item(new_shop, sub_node);
		} else {
			slog("Invalid XML tag while parsing artisan");
		}
	}

	shop_list.insert(shop_list.end(), new_shop);
}

craftshop *
craftshop_find(Creature *keeper)
{
	vector<craftshop *>::iterator shop;

	for (shop = shop_list.begin(); shop != shop_list.end(); shop++)
		if (keeper->mob_specials.shared->vnum == shop[0]->keeper_vnum &&
				keeper->in_room->number == shop[0]->room)
			return *shop;

	return NULL;
}

char *
craft_item::next_requirement(Creature *keeper)
{
	obj_data *obj;
	vector<craft_component *>::iterator compon;

	for (compon = required.begin();compon != required.end();compon++ ) {
		// Item components are all we support right now
		if (compon[0]->item) {
			obj = get_obj_in_list_num(compon[0]->item, keeper->carrying);
			if (!obj) {
				obj = real_object_proto(compon[0]->item);
				return tmp_strdup(obj->short_description);
			}
		} else {
			slog("SYSERR: Unimplemented requirement in artisan");
		}
	}

	return NULL;
}

obj_data *
craft_item::create(Creature *keeper, Creature *recipient)
{
	obj_data *obj;
	vector<craft_component *>::iterator compon;

	for (compon = required.begin();compon != required.end();compon++ ) {
		// Item components are all we support right now
		if (compon[0]->item) {
			obj = get_obj_in_list_num(compon[0]->item, keeper->carrying);
			if (!obj) {
				slog("SYSERR: craft_remove_prereqs called without all prereqs");
				return NULL;
			}
			obj_from_char(obj);
			extract_obj(obj);
		} else {
			slog("SYSERR: Unimplemented requirement in artisan");
			return NULL;
		}
	}

	if (fail_pct && number(0, 100) < fail_pct)
		return NULL;

	obj = read_object(vnum);
	obj_to_char(obj, recipient);
	return obj;
}

char *
list_commission_item(Creature *ch, Creature *keeper, int idx, craft_item *item, char *msg)
{
	obj_data *obj;
	char *needed;
	char *item_prefix;

	obj = real_object_proto(item->vnum);
	item_prefix = tmp_sprintf("%3d%s)%s",
		idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	// We have to find out what we need.
	if (item->next_requirement(keeper))
		needed = tmp_strcat(CCRED(ch, C_NRM), "Unavailable", CCNRM(ch, C_NRM), NULL);
	else
		needed = tmp_strcat(CCGRN(ch, C_NRM), " Available ", CCNRM(ch, C_NRM), NULL);
	
	return tmp_sprintf("%s%s  %s %-43s %11ld\r\n", msg,
		item_prefix,
		needed,
		CAP(tmp_strdup(obj->short_description)),
		item->cost);
		
}

void
craftshop_list(Creature *keeper, Creature *ch)
{
	craftshop *shop;
	vector<craft_item *>::iterator item;
	char *msg = "";
	int idx = 0;

	shop = craftshop_find(keeper);

	if (shop->items.empty()) {
		msg = tmp_sprintf("%s I'm not in business right now.", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	msg = tmp_strcat(msg,
		CCCYN(ch, C_NRM),
		" ##               Item                                               Cost\r\n-------------------------------------------------------------------------\r\n",
		CCNRM(ch, C_NRM),
		NULL);
	for (item = shop->items.begin();item != shop->items.end();item++) {
		idx++;
		msg = list_commission_item(ch, keeper, idx, *item, msg);
	}

	page_string(ch->desc, msg);
}

void
craftshop_buy(craftshop *shop, Creature *keeper, Creature *ch, char *arguments)
{
	vector<craft_item *>::iterator item_itr;
	craft_item *item;
	obj_data *obj;
	char *arg, *msg, *needed_str;

	arg = tmp_getword(&arguments);

	item = NULL;
	if (*arg == '#') {
		unsigned int num;

		arg++;
		num = (unsigned int)atoi(arg) - 1;
		if (num >= 0 && num < shop->items.size())
			item = shop->items[atoi(arg)];
	} else {
		for (item_itr= shop->items.begin();item_itr!= shop->items.end();item_itr++) {
			if (isname(arg, real_object_proto(item_itr[0]->vnum)->name)) {
				item = *item_itr;
				break;
			}
		}
	}

	if (!item) {
		msg = tmp_sprintf("%s I can't make that!  Use the LIST command!", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	needed_str = item->next_requirement(keeper);
	if (needed_str) {
		msg = tmp_sprintf("%s I don't have the necessary materials.", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		msg = tmp_sprintf("%s Give me %s.", GET_NAME(ch), needed_str);
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	if (item->cost > GET_GOLD(ch)) {
		msg = tmp_sprintf("%s You don't have enough money for me to make that.  It costs %ld.", GET_NAME(ch), item->cost);
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
		msg = tmp_sprintf("%s You can't carry any more items.", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	if (IS_CARRYING_W(ch) + real_object_proto(item->vnum)->getWeight() > CAN_CARRY_W(ch)) {
		msg = tmp_sprintf("%s You can't carry any more weight.", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	GET_GOLD(ch) -= item->cost;
	obj = item->create(keeper, ch);

	if (!obj) {
		msg = tmp_sprintf("%s I am sorry.  I failed to make it and I used up all my materials.", GET_NAME(ch));
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
		return;
	}

	send_to_char(ch, "You buy %s for %ld gold.\r\n", obj->short_description, item->cost);
	switch (number(0, 20)) {
		case 0:
			msg = tmp_strcat(GET_NAME(ch), " Glad to do business with you");
			break;
		case 1:
			msg = tmp_strcat(GET_NAME(ch), " Come back soon!");
			break;
		case 2:
			msg = tmp_strcat(GET_NAME(ch), " Have a nice day.");
			break;
		case 3:
			msg = tmp_strcat(GET_NAME(ch), " Thanks, and come again!");
			break;
		case 4:
			msg = tmp_strcat(GET_NAME(ch), " Nice doing business with you");
			break;
		default:
			msg = NULL;
			break;
	}
	if (msg)
		do_say(keeper, msg, 0, SCMD_SAY_TO, NULL);
}

void
assign_artisans(void)
{
	vector<craftshop *>::iterator shop;
	Creature *mob;

	for (shop = shop_list.begin(); shop != shop_list.end(); shop++) {
		mob = real_mobile_proto(shop[0]->keeper_vnum);
		if (mob)
			mob->mob_specials.shared->func = artisan;
		else
			slog("SYSERR: Artisan mob %d not found!", shop[0]->keeper_vnum);
	}

	cmd_slap = find_command("slap");
	cmd_smirk = find_command("smirk");
	cmd_cry = find_command("cry");
}

SPECIAL(artisan)
{
	struct Creature *keeper = (Creature *)me;
	char *msg;
	craftshop *shop;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (!ch || !keeper || !AWAKE(keeper))
		return false;

	if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
		do_action(keeper, GET_NAME(ch), cmd_slap, 0);
		sprintf(buf, "%s is a bloody thief!", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SHOUT, NULL);
		return true;
	}

	if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("sell"))
		return false;

	shop = craftshop_find(keeper);

	if (!shop || shop->room != keeper->in_room->number) {
		msg = tmp_sprintf("%s Sorry!  I don't have my tools!", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SAY_TO, NULL);
		do_action(keeper, "", cmd_cry, 0);
		return true;
	}

/*
	if (!shop->accepts(ch) && shop->refuses(ch)) {
		sprintf(buf, "%s I don't deal with your type.", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SAY_TO, NULL);
		do_action(keeper, GET_NAME(ch), cmd_smirk, 0);
		return true;
	}
*/

	if (CMD_IS("list"))
		craftshop_list(keeper, ch);
	else if (CMD_IS("buy"))
		craftshop_buy(shop, keeper, ch, argument);
	else if (CMD_IS("sell")) {
		msg = tmp_sprintf("%s I don't sell things, I make them.", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SAY_TO, NULL);
	} else
		return false;
	
	return true;
}
