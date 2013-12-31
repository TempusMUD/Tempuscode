#ifndef _VENDOR_H_
#define _VENDOR_H_

struct craft_item;
struct reaction;

struct craft_shop {
        int id;
		int room;
		int keeper_vnum;
		GList *items;
};

/** Loads and/or creates the Craftshop described by the given node. **/
void load_craft_shop(xmlNodePtr node);

struct shop_time {
	int start, end;
};

struct shop_data {
	long room;				// Room of self
	GList *item_list;	// list of produced items
	GList *item_types;	// list of types of items self deals in
	GList *closed_hours;
	const char *msg_denied;		// Message sent to those of wrong race, creed, etc
	const char *msg_badobj;		// Attempt to sell invalid obj to self
	const char *msg_sell_noobj;	// Attempt to sell missing obj to player
	const char *msg_buy_noobj;	// Attempt to buy missing obj from player
	const char *msg_selfbroke;	// Shop ran out of money
	const char *msg_buyerbroke;	// Buyer doesn't have any money
	const char *msg_buy;			// Keeper successfully bought something
	const char *msg_sell;			// Keeper successfully sold something
	const char *cmd_temper;		// Command to run after buyerbroke
	const char *msg_closed;		// Shop is closed at the time
	int markup;				// Price increase when player buying
	int markdown;			// Price decrease when player is selling
	int currency;			// 0 == gold, 1 == cash, 2 == quest points
	long revenue;			// Amount added to money every reset
	int storeroom;          // Room to store inventory in
	bool steal_ok;
	bool attack_ok;
	bool call_for_help;
	SPECIAL((*func));
	struct reaction *reaction;
};

SPECIAL(vendor);
const char *vendor_parse_param(char *param, struct shop_data *shop, int *err_line);

#endif
