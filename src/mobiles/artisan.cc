#include <vector>
#include "db.h"
#include "structs.h"
#include "xml_utils.h"
#include "utils.h"
#include "interpreter.h"
#include "actions.h"

/*
<CRAFTSHOP ROOM="1234" KEEPER="12343">
	<REFUSE RACE="Orc"/>
	<REFUSE CLASS="Thief"/>
	<ACCEPT CLASS="Mage"/>
	<ITEM ID="54321" COST="1000000">
		<REQUIRED MATERIAL="ivory" WEIGHT="10000"/>
		<REQUIRED MATERIAL="gold" WEIGHT="10"/>
		<REQUIRED MATERIAL="emerald" VALUE="10000"/>
		<REQUIRED ITEM="12341"/>
	</ITEM>
</CRAFTSHOP>
*/
const int CRAFT_RACE = 0;
const int CRAFT_CLASS = 1;
const int CRAFT_ALL = 2;

static int cmd_slap, cmd_smirk, cmd_sayto, cmd_cry;

struct craft_restr {
	int kind;
	int val;
};

struct craft_component {
	short item;
	short material;
	short weight;
	short value;
	short amount;
};

class craft_item {
	public:
		craft_item(void);
		~craft_item(void);

		bool can_make(char_data *keeper);

		short object_id;	// object to be offered when all components are held
		long cost;
		vector<craft_component> required;
};

class craftshop {
	public:
		craftshop(void);
		~craftshop(void);

		bool accepts(char_data *ch);
		bool refuses(char_data *ch);

		short room;
		short keeper;
		vector<craft_item> items;
		vector<craft_restr> accept_list;
		vector<craft_restr> refuse_list;
};

bool
craftshop::accepts(char_data *ch)
{
	return true;
}

bool
craftshop::refuses(char_data *ch)
{
	return true;
}

vector<craftshop *> shop_list;

void
load_xml_craftshop(xmlNodePtr node)
{
	xmlNodePtr sub_node;
	craftshop *new_shop;

	cmd_slap = find_command("slap");
	cmd_smirk = find_command("smirk");
	cmd_sayto = find_command(">");
	cmd_cry = find_command("cry");

	CREATE(new_shop, craftshop, 1);
	new_shop->room = xmlGetIntProp(node, "room");
	new_shop->keeper = xmlGetIntProp(node, "keeper");
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		if (!xmlStrcmp(sub_node->name, (const xmlChar *)"refuses")) {
		} else if (!xmlStrcmp(sub_node->name, (const xmlChar *)"accepts")) {
		} else if (!xmlStrcmp(sub_node->name, (const xmlChar *)"item")) {
		} else {
			slog("SYSERR: Invalid xml node in <craftshop>");
		}
	}

	shop_list.insert(shop_list.end(), new_shop);
}

craftshop *
craftshop_find(char_data *keeper)
{
	return NULL;
}

void
craftshop_list(char_data *keeper, char_data *ch)
{
	sprintf(buf, "%s I can't do that yet!", GET_NAME(ch));
	do_gen_comm(keeper, buf, 0, SCMD_SAY, NULL);
}

SPECIAL(artisan)
{
	struct char_data *keeper = (char_data *)me;
	craftshop *shop;

	if (spec_mode == SPECIAL_DEATH)
		return false;

	if (!AWAKE(keeper))
		return false;

	if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
		do_action(keeper, GET_NAME(ch), cmd_slap, 0);
		sprintf(buf, "%s is a bloody thief!", GET_NAME(ch));
		do_gen_comm(keeper, buf, 0, SCMD_SHOUT, NULL);
		return true;
	}

	shop = craftshop_find(keeper);

	if (!shop->accepts(ch) && shop->refuses(ch)) {
		sprintf(buf, "%s I don't deal with your type.", GET_NAME(ch));
		do_gen_comm(keeper, buf, 0, SCMD_SAY, NULL);
		do_action(keeper, GET_NAME(ch), cmd_smirk, 0);
		return true;
	}

	if (shop->room != keeper->in_room->number) {
		sprintf(buf, "%s Sorry!  I don't have my tools!", GET_NAME(ch));
		do_gen_comm(keeper, buf, 0, SCMD_SAY, NULL);
		do_action(keeper, "", cmd_cry, 0);
		return true;
	}

	if (CMD_IS("list"))
		craftshop_list(keeper, ch);
	else
		return false;
	
	return true;
}
