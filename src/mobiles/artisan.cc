#include <vector>
#include "db.h"
#include "structs.h"
#include "xml_utils.h"
#include "utils.h"
#include "interpreter.h"
#include "actions.h"
#include "char_class.h"
#include "comm.h"

SPECIAL(artisan);

const int CRAFT_RACE = 0;
const int CRAFT_CLASS = 1;
const int CRAFT_ALIGN = 2;
const int CRAFT_ALL = 4;

const int CRAFT_GOOD = 0;
const int CRAFT_EVIL = 1;
const int CRAFT_NEUTRAL = 2;

static int cmd_slap, cmd_smirk, cmd_sayto, cmd_cry;

struct craft_restr {
	bool match(Creature *ch);
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
		bool can_make(Creature *keeper);

		short vnum;	// object to be offered when all components are held
		long cost; // -1 means to use default cost
		vector<craft_component *> required;
};

struct craftshop {
		bool accepts(Creature *ch);
		bool refuses(Creature *ch);

		short room;
		short keeper_vnum;
		short clan_id;
		short clan_tax;
		vector<craft_item *> items;
		vector<craft_restr *> accept_list;
		vector<craft_restr *> refuse_list;
};

vector<craftshop *> shop_list;

bool
craft_restr::match(Creature *ch)
{
	switch (this->kind) {
		case CRAFT_RACE:
			if (GET_RACE(ch) == this->val)
				return true;
			break;
		case CRAFT_CLASS:
			if (GET_CLASS(ch) == this->val)
				return true;
			break;
		case CRAFT_ALIGN:
			if (IS_GOOD(ch) && this->val == CRAFT_GOOD)
				return true;
			if (IS_EVIL(ch) && this->val == CRAFT_EVIL)
				return true;
			if (IS_NEUTRAL(ch) && this->val == CRAFT_NEUTRAL)
				return true;
			break;
		case CRAFT_ALL:
			return true;
	}

	return false;
}

bool
craftshop::accepts(Creature *ch)
{
	vector<craft_restr *>::iterator restr_iter;

	restr_iter = accept_list.begin();
	for (;restr_iter != accept_list.end();restr_iter++)
		if (restr_iter[0]->match(ch))
			return true;
	return false;
}

bool
craftshop::refuses(Creature *ch)
{
	vector<craft_restr *>::iterator restr_iter;

	restr_iter = refuse_list.begin();
	for (;restr_iter != refuse_list.end();restr_iter++)
		if (restr_iter[0]->match(ch))
			return true;
	return false;
}

void
craft_parse_item(craftshop *shop, xmlNodePtr node)
{
	craft_item *new_item;
	
	new_item = new craft_item;
	new_item->vnum = xmlGetIntProp(node, "vnum");
	new_item->cost = xmlGetIntProp(node, "cost");
	return;
}

void
craftshop_load(xmlNodePtr node)
{
	xmlNodePtr sub_node;
	craftshop *new_shop;
	craft_restr *new_restr;
	const xmlChar *prop;

	new_shop = new craftshop;
	new_shop->room = xmlGetIntProp(node, "room");
	new_shop->keeper_vnum = xmlGetIntProp(node, "keeper");
	for (sub_node = node->xmlChildrenNode; sub_node; sub_node = sub_node->next) {
		if (xmlMatches(sub_node->name, "refuse")) {
			new_restr = new craft_restr;
			
			if ((prop = xmlGetProp(sub_node, (const xmlChar *)"class")) != NULL) {
				new_restr->kind = CRAFT_CLASS;
				new_restr->val = search_block((const char *)prop, pc_char_class_types, true);
				if (new_restr->val < 0)
					slog("WARNING: refuse set to invalid class");
			} else if ((prop = xmlGetProp(sub_node, (const xmlChar *)"race")) != NULL) {
				new_restr->kind = CRAFT_RACE;
				new_restr->val = search_block((const char *)prop, player_race, true);
				if (new_restr->val < 0)
					slog("WARNING: refuse set to invalid race");
			} else {
				slog("SYSERR: RACE and CLASS unspecified in craftshop refusal");
			}

			new_shop->refuse_list.insert(new_shop->refuse_list.end(), new_restr);
		} else if (xmlMatches(sub_node->name, "accepts")) {
			new_restr = new craft_restr;
			
			if ((prop = xmlGetProp(sub_node, (const xmlChar *)"class")) != NULL) {
				new_restr->kind = CRAFT_CLASS;
				new_restr->val = search_block((const char *)prop, pc_char_class_types, true);
				if (new_restr->val < 0)
					slog("WARNING: accept set to invalid class");
			} else if ((prop = xmlGetProp(sub_node, (const xmlChar *)"race")) != NULL) {
				new_restr->kind = CRAFT_RACE;
				new_restr->val = search_block((const char *)prop, player_race, true);
				if (new_restr->val < 0)
					slog("WARNING: accept set to invalid race");
			} else {
				slog("SYSERR: RACE and CLASS unspecified in craftshop refusal");
			}

			new_shop->accept_list.insert(new_shop->accept_list.end(), new_restr);
		} else if (xmlMatches(sub_node->name, "item")) {
			craft_parse_item(new_shop, sub_node);
		}
	}

	shop_list.insert(shop_list.end(), new_shop);
}

craftshop *
craftshop_find(Creature *keeper)
{
	vector<craftshop *>::iterator shop;

	for (shop = shop_list.begin(); shop != shop_list.end(); shop++)
		if (real_mobile_proto(shop[0]->keeper_vnum) == keeper)
			return *shop;

	return NULL;
}

void
craftshop_list(Creature *keeper, Creature *ch)
{
	obj_data *obj;

	if (keeper->carrying) {
		buf[0] = '\0';
		for (obj = keeper->carrying;obj;obj = obj->next_content) {
			if (CAN_SEE_OBJ(ch, obj)) {
				strcat(buf, obj->short_description);
				strcat(buf, "\r\n");
			}
		}

		page_string(ch->desc, buf);
	} else {
		sprintf(buf, "%s I'm not carrying anything!", GET_NAME(ch));
		do_gen_comm(keeper, buf, 0, SCMD_SAY_TO, NULL);
	}
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
	cmd_sayto = find_command(">");
	cmd_cry = find_command("cry");
}

SPECIAL(artisan)
{
	struct Creature *keeper = (Creature *)me;
	craftshop *shop;

	if ( spec_mode != SPECIAL_CMD )
		return false;

	if (!ch || !keeper || !AWAKE(keeper))
		return false;

	if (CMD_IS("steal") && GET_LEVEL(ch) < LVL_IMMORT) {
		do_action(keeper, GET_NAME(ch), cmd_slap, 0);
		sprintf(buf, "%s is a bloody thief!", GET_NAME(ch));
		do_gen_comm(keeper, buf, 0, SCMD_SHOUT, NULL);
		return true;
	}

	if (!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("sell"))
		return false;

	shop = craftshop_find(keeper);

	if (!shop || shop->room != keeper->in_room->number) {
		sprintf(buf, "%s Sorry!  I don't have my tools!", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SAY_TO, NULL);
		do_action(keeper, "", cmd_cry, 0);
		return true;
	}

	if (!shop->accepts(ch) && shop->refuses(ch)) {
		sprintf(buf, "%s I don't deal with your type.", GET_NAME(ch));
		do_say(keeper, buf, 0, SCMD_SAY_TO, NULL);
		do_action(keeper, GET_NAME(ch), cmd_smirk, 0);
		return true;
	}

	if (CMD_IS("list"))
		craftshop_list(keeper, ch);
	else
		return false;
	
	return true;
}
