//
// File: shop.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef _SHOP_H_
#define _SHOP_H_

#include <vector>
#include "xml_utils.h"

struct shop_buy_data {
	int type;
	char *keywords;
};

#define BUY_TYPE(i)		((i).type)
#define BUY_WORD(i)		((i).keywords)

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



struct shop_data {
	int vnum;					/* Virtual number of this shop      */
	int *producing;				/* Which item to produce (vnum) */
	byte currency;				/* What is legal tender ?               */
	float profit_buy;			/* Factor to multiply cost with     */
	float profit_sell;			/* Factor to multiply cost with     */
	int revenue;				// How much of thier original cash do they get back
	// Each zone reset?
	struct shop_buy_data *type;	/* Which items to trade         */
	char *no_such_item1;		/* Message if keeper hasn't got an item */
	char *no_such_item2;		/* Message if player hasn't got an item */
	char *missing_cash1;		/* Message if keeper hasn't got cash    */
	char *missing_cash2;		/* Message if player hasn't got cash    */
	char *do_not_buy;			/* If keeper dosn't buy such things */
	char *message_buy;			/* Message when player buys item    */
	char *message_sell;			/* Message when player sells item   */
	int temper1;				/* How does keeper react if no money    */
	int bitvector;				/* Can attack? Use bank? Cast here? */
	int keeper;					/* The mobil who owns the shop (vnum) */
	int with_who;				/* Who does the shop trade with?    */
	int *in_room;				/* Where is the shop?           */
	int open1, open2;			/* When does the shop open?     */
	int close1, close2;			/* When does the shop close?        */
	int bankAccount;			/* Store all gold over 15000 (disabled) */
	int lastsort;				/* How many items are sorted in inven?  */
	 SPECIAL(*func);			/* Secondary spec_proc for shopkeeper   */
	struct shop_data *next;		/* Next shop in linked list             */
};


#define MAX_TRADE	5			/* List maximums for compatibility  */
#define MAX_PROD	5			/*  with shops before v3.0      */
#define VERSION3_TAG	"v3.0"	/* The file has v3.0 shops in it!   */
#define SHP_MOD_LEV		"pl"
#define MAX_SHOP_OBJ	100		/* "Soft" maximum for list maximums */


/* Pretty general macros that could be used elsewhere */
#define IS_GOD(ch)		(!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMMORT))
#define GET_OBJ_NUM(obj)	(obj->item_number)
#define END_OF(buffer)		((buffer) + strlen((buffer)))


/* Possible states for objects trying to be sold */
#define OBJECT_DEAD		0
#define OBJECT_NOTOK		1
#define OBJECT_OK		2
#define OBJECT_NOSELL           3
#define OBJECT_BROKEN           4


/* Types of lists to read */
#define LIST_PRODUCE		0
#define LIST_TRADE		1
#define LIST_ROOM		2


/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_NOGOOD		(1 << 0)
#define TRADE_NOEVIL		(1 << 1)
#define TRADE_NONEUTRAL		(1 << 2)
#define TRADE_NOMAGIC_USER	(1 << 3)
#define TRADE_NOCLERIC		(1 << 4)
#define TRADE_NOTHIEF		(1 << 5)
#define TRADE_NOWARRIOR		(1 << 6)
#define TRADE_NOBARB		(1 << 7)
#define TRADE_NOKNIGHT		(1 << 8)
#define TRADE_NORANGER		(1 << 9)
#define TRADE_NOMONK            (1 << 10)
#define TRADE_NOMERC            (1 << 11)
#define TRADE_NOHOOD            (1 << 12)
#define TRADE_NOPHYSIC          (1 << 13)
#define TRADE_NOPSIONIC         (1 << 14)
#define TRADE_NOCYBORG          (1 << 15)
#define TRADE_NOVAMPIRE         (1 << 16)
#define TRADE_NOSPARE1          (1 << 17)
#define TRADE_NOSPARE2          (1 << 18)
#define TRADE_NOSPARE3          (1 << 19)
#define TRADE_NOHUMAN           (1 << 20)
#define TRADE_NOELF             (1 << 21)
#define TRADE_NODWARF           (1 << 22)
#define TRADE_NOHALF_ORC        (1 << 23)
#define TRADE_NOTABAXI          (1 << 24)
#define TRADE_NOMINOTAUR        (1 << 25)
#define TRADE_NOORC             (1 << 26)

#define NUM_TRADE_WITH 27


struct stack_data {
	int data[100];
	int len;
};

#define S_DATA(stack, index)	((stack)->data[(index)])
#define S_LEN(stack)		((stack)->len)


/* Which expression type we are now parsing */
#define OPER_OPEN_PAREN		0
#define OPER_CLOSE_PAREN	1
#define OPER_OR			2
#define OPER_AND		3
#define OPER_NOT		4
#define MAX_OPER		4


#define SHOP_NUM(i)		((i)->vnum)
#define SHOP_KEEPER(i)		((i)->keeper)
#define SHOP_OPEN1(i)		((i)->open1)
#define SHOP_CLOSE1(i)		((i)->close1)
#define SHOP_OPEN2(i)		((i)->open2)
#define SHOP_CLOSE2(i)		((i)->close2)
#define SHOP_ROOM(i, num)	((i)->in_room[(num)])
#define SHOP_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define SHOP_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))
#define SHOP_PRODUCT(i, num)	((i)->producing[(num)])
#define SHOP_BANK(i)		((i)->bankAccount)
#define SHOP_BROKE_TEMPER(i)	((i)->temper1)
#define SHOP_BITVECTOR(i)	((i)->bitvector)
#define SHOP_TRADE_WITH(i)	((i)->with_who)
#define SHOP_SORT(i)		((i)->lastsort)
#define SHOP_CURRENCY(i)       	((i)->currency)
#define GET_MONEY(ch, i)        ((i)->currency == 1 ? \
				 GET_CASH(ch) : GET_GOLD(ch))

#define CURRENCY_GOLD           0
#define CURRENCY_CREDITS        1

#define SHOP_BUYPROFIT(i)	((i)->profit_buy)
#define SHOP_SELLPROFIT(i)	((i)->profit_sell)
#define SHOP_FUNC(i)		((i)->func)

#define NOTRADE_GOOD(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOGOOD))
#define NOTRADE_EVIL(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOEVIL))
#define NOTRADE_NEUTRAL(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NONEUTRAL))
#define NOTRADE_MAGIC_USER(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMAGIC_USER))
#define NOTRADE_CLERIC(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOCLERIC))
#define NOTRADE_THIEF(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTHIEF))
#define NOTRADE_WARRIOR(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOWARRIOR))

#define NOTRADE(i, flag)        (IS_SET(SHOP_TRADE_WITH((i)), flag))

#define	WILL_START_FIGHT	1
#define WILL_BANK_MONEY		2

#define SHOP_KILL_CHARS(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_START_FIGHT))
#define SHOP_USES_BANK(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_BANK_MONEY))

#define SHOP_REVENUE(i) ((i)->revenue)
#define MIN_OUTSIDE_BANK	10000
#define MAX_OUTSIDE_BANK	35000

#define MSG_NOT_OPEN_YET	"Come back later!"
#define MSG_NOT_REOPEN_YET	"Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY	"Sorry, come back tomorrow."
#define MSG_NO_STEAL_HERE	"$n is a bloody thief!"
#define MSG_NO_SEE_CHAR		"I don't trade with someone I can't see!"
#define MSG_NO_SELL_ALIGN	"We don't serve your alignment here."
#define MSG_NO_SELL_RACE        "We don't serve your kind here!"
#define MSG_NO_SELL_CLASS	"We don't deal with members of your profession here."
#define MSG_NO_USED_WANDSTAFF	"I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER	"Get out of here before I call the guards!"
#define MSG_NO_SELL_OBJ         "I have no interest in that."
#define MSG_BROKEN_OBJ          "I don't buy broken objects."

#define SHOP_DEFAULT_MESSAGE_BUY  "You spend %d."
#define SHOP_DEFAULT_MESSAGE_SELL "You receive %d."

int ok_damage_shopkeeper(struct Creature *ch, struct Creature *victim);
bool shop_check_message_format(char *format_buf);

#endif
