#ifndef _VENDOR_H_
#define _VENDOR_H_

class CraftItem;

class Craftshop {
	public:
		Craftshop(xmlNodePtr node);
        ~Craftshop();
		static Craftshop *find(Creature *keeper);
        //Loads the Craftshop described by the given xml node.
		void load(xmlNodePtr node);
        // sends a simple status message to the given Creature.
        void sendStatus( Creature *ch );
        //Loads the Craftitem described by the given xml node.
		void parse_item(xmlNodePtr node);
        // Lists the items for sale.
		void list(Creature *keeper, Creature *ch);
        // Attempts to purchase an item from keeper for ch.
		void buy(Creature *keeper, Creature *ch, char *args);
        int getID() { return id; }
    public:
        
		int room;
		int keeper_vnum;
		vector<CraftItem *> items;
    private:
        int id;
};

/** Loads and/or creates the Craftshop described by the given node. **/
void load_craft_shop(xmlNodePtr node);

struct ShopTime {
	int start, end;
};

struct ShopData {
	ShopData(void) : item_list(), item_types() {};

	long room;				// Room of self
	vector<int> item_list;	// list of produced items
	vector<int> item_types;	// list of types of items self deals in
	vector<ShopTime> closed_hours;
	char *msg_denied;		// Message sent to those of wrong race, creed, etc
	char *msg_badobj;		// Attempt to sell invalid obj to self
	char *msg_sell_noobj;	// Attempt to sell missing obj to player
	char *msg_buy_noobj;	// Attempt to buy missing obj from player
	char *msg_selfbroke;	// Shop ran out of money
	char *msg_buyerbroke;	// Buyer doesn't have any money
	char *msg_buy;			// Keeper successfully bought something
	char *msg_sell;			// Keeper successfully sold something
	char *cmd_temper;		// Command to run after buyerbroke
	char *msg_closed;		// Shop is closed at the time
	int markup;				// Price increase when player buying
	int markdown;			// Price decrease when player is selling
	int currency;			// 0 == gold, 1 == cash, 2 == quest points
	long revenue;			// Amount added to money every reset
	bool steal_ok;
	bool attack_ok;
	SPECIAL(*func);
	Reaction reaction;
};

SPECIAL(vendor);
char *vendor_parse_param(Creature *self, char *param, ShopData *shop, int *err_line);
bool ok_damage_vendor(Creature *ch, Creature *victim);
bool same_obj(obj_data *obj1, obj_data *obj2);

#endif
