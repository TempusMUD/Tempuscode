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

static int cmd_slap, cmd_smirk, cmd_cry;
vector<Craftshop *> shop_list;


struct CraftComponent {
	int item;
	int material;
	int weight;
	int value;
	int amount;
};

class CraftItem {
	public:
    CraftItem() : required() {
        vnum = 0;
        cost = 999999999;
        fail_pct = 100;
    }
    ~CraftItem(void) {
        vector<CraftComponent*>::iterator cmp;
        for (cmp = required.begin();cmp != required.end();cmp++) {
            delete (*cmp);
        }
        required.clear();
    }
    char *next_requirement(Creature *keeper);
    obj_data *create(Creature *keeper, Creature *recipient);

    int vnum;	// object to be offered when all components are held
    long cost; // -1 means to use default cost
    int fail_pct;
    vector<CraftComponent *> required;
};


/**
 * Loads the Craftshop described by the given xml node.
 * If the sho has already been created, it is reinitialized.
**/
void
load_craft_shop(xmlNodePtr node)
{
    Craftshop *shop = NULL;
    int id = xmlGetIntProp(node,"id");

	vector<Craftshop *>::iterator it;

	for (it = shop_list.begin(); it != shop_list.end(); ++it) {
        if( (*it)->getID() == id ) {
            shop = (*it);
            break;
        }
    }
    if( shop != NULL ) {
        shop->load(node);
    } else {
        shop = new Craftshop(node);
    }
	shop_list.push_back(shop);
}

/**
 * loads the CraftItem described by the given xml node.
 * Does not reinit the item. Always creates a new item.
**/
void
Craftshop::parse_item(xmlNodePtr node)
{
	xmlNodePtr sub_node;
	CraftItem *new_item;
	CraftComponent *compon;

	new_item = new CraftItem();
	new_item->vnum = xmlGetIntProp(node, "vnum");
	new_item->cost = xmlGetIntProp(node, "cost");
    new_item->fail_pct = xmlGetIntProp(node, "failure", 0);
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		if (xmlMatches(sub_node->name, "requires")) {
			compon = new CraftComponent;
			compon->item = xmlGetIntProp(sub_node, "item");
			compon->material = xmlGetIntProp(sub_node, "material");
			compon->weight = xmlGetIntProp(sub_node, "weight");
			compon->value = xmlGetIntProp(sub_node, "value");
			compon->amount = xmlGetIntProp(sub_node, "amount");
			new_item->required.insert(new_item->required.end(), compon);
		} else if (!xmlMatches(sub_node->name, "text")) {
			errlog("Invalid XML tag '%s' while parsing craftshop item",
				(const char *)sub_node->name);
		}
	}
	items.push_back(new_item);
	return;
}

/**
 * creates and initializes a new Craftshop. Wee.
**/
Craftshop::Craftshop(xmlNodePtr node)
: items()
{
    id = xmlGetIntProp(node, "id");
    load( node );
}

/**
 * destructor for the craftshop
**/
Craftshop::~Craftshop(void)
{
    vector<CraftItem*>::iterator item;
    for (item = items.begin();item != items.end();item++) {
        delete (*item);
	}
    items.clear();
}

/**
 * Loads this Craftshop's data from the given xml node.
**/
void
Craftshop::load( xmlNodePtr node )
{
    xmlNodePtr sub_node;
	xmlChar *prop;
    room = xmlGetIntProp(node, "room");
    keeper_vnum = xmlGetIntProp(node, "keeper");


    // Remove all the currently stored items.
    vector<CraftItem*>::iterator item;
    for (item = items.begin();item != items.end();item++) {
        delete (*item);
	}
    items.clear();
    // Load the described items and thier info.
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		prop = NULL;
		if (xmlMatches(sub_node->name, "item")) {
			parse_item(sub_node);
		} else if (!xmlMatches(sub_node->name, "text")) {
			errlog("Invalid XML tag '%s' while parsing craftshop",
				(const char *)sub_node->name);
		}
	}
}


Craftshop *
Craftshop::find(Creature *keeper)
{
	vector<Craftshop *>::iterator shop;

	for (shop = shop_list.begin(); shop != shop_list.end(); shop++)
		if (keeper->mob_specials.shared->vnum == (*shop)->keeper_vnum &&
				keeper->in_room->number == (*shop)->room)
			return *shop;

	return NULL;
}

char *
CraftItem::next_requirement(Creature *keeper)
{
	obj_data *obj;
	vector<CraftComponent *>::iterator compon;

	for (compon = required.begin();compon != required.end();compon++ ) {
		// Item components are all we support right now
		if ((*compon)->item) {
			obj = get_obj_in_list_num(compon[0]->item, keeper->carrying);
			if (!obj) {
				obj = real_object_proto(compon[0]->item);
				return tmp_strdup(obj->name);
			}
		} else {
			errlog("Unimplemented requirement in artisan");
		}
	}

	return NULL;
}

obj_data *
CraftItem::create(Creature *keeper, Creature *recipient)
{
	obj_data *obj;
	vector<CraftComponent *>::iterator compon;

	for (compon = required.begin();compon != required.end();compon++ ) {
		// Item components are all we support right now
		if ((*compon)->item) {
			obj = get_obj_in_list_num(compon[0]->item, keeper->carrying);
			if (!obj) {
				errlog("craft_remove_prereqs called without all prereqs");
				return NULL;
			}
			obj_from_char(obj);
			extract_obj(obj);
		} else {
			errlog("Unimplemented requirement in artisan");
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
list_commission_item(Creature *ch, Creature *keeper, int idx, CraftItem *item, char *msg)
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
		CAP(tmp_strdup(obj->name)),
		item->cost+(item->cost*ch->getCostModifier(keeper))/100);

}

// sends a simple status message to the given Creature.
void
Craftshop::sendStatus( Creature *ch ) {
    const char *name = "<not loaded>";
    Creature *keeper = real_mobile_proto(keeper_vnum);
    if( keeper != NULL )
        name = GET_NAME(keeper);
    send_to_char(ch, "[%6d] %15s [%6d] ( %zd items )\r\n",
                    id, name, keeper_vnum, items.size() );
}


// Lists the items for sale.
void
Craftshop::list(Creature *keeper, Creature *ch)
{
	vector<CraftItem *>::iterator item;
	int idx = 0;

	if (items.empty()) {
		perform_say_to(keeper, ch, "I'm not in business right now.");
		return;
	}

	char *msg = tmp_strcat(
		CCCYN(ch, C_NRM),
		" ##               Item                                               Cost\r\n-------------------------------------------------------------------------\r\n",
		CCNRM(ch, C_NRM),
		NULL);
	for (item = items.begin();item != items.end();item++) {
		idx++;
		msg = list_commission_item(ch, keeper, idx, *item, msg);
	}

	page_string(ch->desc, msg);
}

// Attempts to purchase an item from keeper for ch.
void
Craftshop::buy(Creature *keeper, Creature *ch, char *arguments)
{
	vector<CraftItem *>::iterator item_itr;
	CraftItem *item;
	obj_data *obj;
	char *arg, *msg, *needed_str;
    arg = tmp_getword(&arguments);

	item = NULL;
	if (*arg == '#') {
		unsigned int num;

		arg++;
		num = (unsigned int)atoi(arg) - 1;
		if (num >= 0 && num < items.size())
			item = items[num];
	} else {
		for (item_itr= items.begin();item_itr!= items.end();item_itr++) {
			if (isname(arg, real_object_proto((*item_itr)->vnum)->aliases)) {
				item = *item_itr;
				break;
			}
		}
	}

	if (!item) {
		perform_say_to(keeper, ch, "I can't make that!  Use the LIST command!");
		return;
	}

    long modCost = item->cost+(item->cost*ch->getCostModifier(keeper))/100;

	needed_str = item->next_requirement(keeper);
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

	if (IS_CARRYING_W(ch) + real_object_proto(item->vnum)->getWeight() > CAN_CARRY_W(ch)) {
		msg = tmp_sprintf("You can't carry any more weight.");
		perform_say_to(keeper, ch, msg);
		return;
	}

	GET_GOLD(ch) -= modCost;
	obj = item->create(keeper, ch);

	if (!obj) {
		msg = tmp_sprintf("I am sorry.  I failed to make it and I used up all my materials.");
		perform_say_to(keeper, ch, msg);
		return;
	}

	send_to_char(ch, "You buy %s for %ld gold.\r\n", obj->name, modCost);
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
		perform_say_to(keeper, ch, msg);
}

void
assign_artisans(void)
{
	vector<Craftshop *>::iterator shop;
	Creature *mob;

	for (shop = shop_list.begin(); shop != shop_list.end(); shop++) {
		mob = real_mobile_proto((*shop)->keeper_vnum);
		if (mob)
			mob->mob_specials.shared->func = artisan;
		else
			errlog("Artisan mob %d not found!", (*shop)->keeper_vnum);
	}

	cmd_slap = find_command("slap");
	cmd_smirk = find_command("smirk");
	cmd_cry = find_command("cry");
}

SPECIAL(artisan)
{
	struct Creature *keeper = (Creature *)me;
	char *msg;
	Craftshop *shop;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (!ch || !keeper || !AWAKE(keeper))
		return false;

	if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
		do_action(keeper, GET_NAME(ch), cmd_slap, 0, 0);
		sprintf(buf, "%s is a bloody thief!", GET_NAME(ch));
        buf[0] = toupper(buf[0]);
		perform_say(keeper, "shout", buf);
		return true;
	}

	if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("sell") && !CMD_IS("status"))
		return false;

	shop = Craftshop::find(keeper);

	if (!shop || shop->room != keeper->in_room->number) {
		msg = tmp_sprintf("Sorry!  I don't have my tools!");
		perform_say_to(keeper, ch, msg);
		do_action(keeper, tmp_strdup(""), cmd_cry, 0, 0);
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
		shop->list(keeper, ch);
	} else if (CMD_IS("buy")) {
		shop->buy(keeper, ch, argument);
	} else if (CMD_IS("sell")) {
		msg = tmp_sprintf("I don't buy things, I make them.");
		perform_say_to(keeper, ch, msg);
    } else if( CMD_IS("status") && Security::isMember(ch,"Coder") ) {
        vector<Craftshop *>::iterator shop;
        for (shop = shop_list.begin(); shop != shop_list.end(); shop++) {
            (*shop)->sendStatus(ch);
        }
	} else {
		return false;
    }

	return true;
}
