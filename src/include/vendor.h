#ifndef _VENDOR_H_
#define _VENDOR_H_

struct craft_item;
struct reaction;

struct craft_shop {
    long room;
    struct reaction *reaction;
    int currency;
    bool call_for_help;
    GList *items;
};

// struct shop_time represents a range of hours for use in marking the
// times in which a shop is closed
struct shop_time {
    int start, end;
};

struct shop_data {
    long room;              // Room of self
    GList *item_list;   // list of produced items
    GList *item_types;  // list of types of items self deals in
    GList *closed_hours;
    char  *msg_denied;     // Message sent to those of wrong race, creed, etc
    char  *msg_badobj;     // Attempt to sell invalid obj to self
    char  *msg_sell_noobj; // Attempt to sell missing obj to player
    char  *msg_buy_noobj;  // Attempt to buy missing obj from player
    char  *msg_selfbroke;  // Shop ran out of money
    char  *msg_buyerbroke; // Buyer doesn't have any money
    char  *msg_buy;            // Keeper successfully bought something
    char  *msg_sell;           // Keeper successfully sold something
    char  *cmd_temper;     // Command to run after buyerbroke
    char  *msg_closed;     // Shop is closed at the time
    int markup;             // Price increase when player buying
    int markdown;           // Price decrease when player is selling
    int currency;           // 0 == gold, 1 == cash, 2 == quest points
    long revenue;           // Amount added to money every reset
    int storeroom;          // Room to store inventory in
    bool steal_ok;
    bool attack_ok;
    bool consignment;
    bool call_for_help;
    SPECIAL((*func));
    struct reaction *reaction;
};

SPECIAL(vendor);
struct shop_data *make_shop(void);
void free_shop(struct shop_data *shop);
const char *vendor_parse_param(char *param, struct shop_data *shop, int *err_line);
struct obj_data *vendor_resolve_hash(struct shop_data *shop, struct creature *self, char *obj_str);
struct obj_data *vendor_resolve_name(struct shop_data *shop, struct creature *self, char *obj_str);

#endif
