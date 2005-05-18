#include <list>
#include <functional>
#include <time.h>
#include "obj_data.h"
#include "room_data.h"
#include "creature.h"

#define BAD_AUCTION(obj) (obj->obj_flags.type_flag == ITEM_FOOD || \
                          obj->obj_flags.type_flag == ITEM_TRASH || \
                          obj->obj_flags.type_flag == ITEM_NOTE || \
                          obj->obj_flags.type_flag == ITEM_KEY || \
                          obj->obj_flags.type_flag == ITEM_PORTAL || \
                          obj->obj_flags.type_flag == ITEM_SCRIPT || \
                          obj->shared->vnum == -1)
// The vnum of the creature that delivers the goods and takes the cash
const int IMP_VNUM = 3223;
const int TOP_MOOD = 7;
const int GOING_ONCE = 45;
const int GOING_TWICE = 90;
const int SOLD_TIME  = 135;
const int NO_BID_THRESH  = 450;
const int AUCTION_THRESH = 900;
const int MAX_AUC_VALUE = 50000000;
const int MAX_AUC_ITEMS = 5;
const int MAX_TOTAL_AUC = 100;
const int BID_INCREMENT = 5; //percent of starting bid

extern const int IMP_DELIVER_ITEM;
extern const int IMP_RETURN_ITEM;

ACMD(do_bid);
ACMD(do_stun);

struct imp_data {
    Creature *imp;
    long owner_id;
    long buyer_id;
    obj_data *item;
    long owed;
    int mode;
    short fail_count;
};

struct auction_data {
    bool operator < (const auction_data &b) const {
        return item_no < b.item_no;
    }
    long owner_id;
    long buyer_id;
    int item_no;
    obj_data *item;
    long start_bid;
    long start_time;
    long last_bid_time;
    long current_bid;
    bool new_bid;
    bool new_item;
    short announce_count;
};

list<auction_data> items;

struct _mood {
    char *mood_name;
};

struct _mood moods[] = {
    { " fiercely" },
    { " vehemently" },
    { " wildly" },
    { " outrageously" },
    { " quickly" },
    { " crazily" },
    { " impatiently" },
    { " loudly" }
};

int number(int from, int to);
int get_max_auction_item();
Creature *create_imp(room_data *inroom, auction_data &);
bool bidder_can_afford(Creature *bidder, long amount);

using namespace std;

SPECIAL(do_auctions)
{
	Creature *self = (Creature *)me;
    Creature *imp;
    list<auction_data>::iterator ai = items.begin();
    short mood_index = 0;

    Creature *dick;
    while (self->numCombatants()) {
        dick = self->findRandomCombat();
        act ("A ball of light streaks from $N's hand and hits you "
             "square in the chest, burning you to a cinder!", false,
             dick, 0, self, TO_CHAR);
        act ("A ball of light streaks from $n's hand and hits $N "
             "square in the chest, burning $S to a cinder!", false,
             self, 0, dick, TO_ROOM);
        raw_kill(dick, self, TYPE_SLASH);
    }


	if (spec_mode == SPECIAL_TICK) { 
        if (items.empty())
            return 0;

        char *auc_str = NULL;
        ai = items.begin();
        while (ai != items.end()) {
            mood_index = number(0, TOP_MOOD);
            if (ai->new_bid) {
                auc_str = tmp_sprintf("%ld coins heard for item number %d, %s!!",
                                      ai->current_bid, ai->item_no, 
                                      ai->item->name);
                ai->new_bid = false;
                ai->announce_count = 0;
            }
            else if (ai->new_item) {
                auc_str = tmp_sprintf("We now have a new item up for bids!  "
                                      "Item number %d, %s. We'll start the " 
                                      "bidding at %ld coins!",
                                      ai->item_no, ai->item->name,
                                      ai->start_bid);
                ai->new_item = false;
            }
            else if (ai->last_bid_time && 
                     (time(NULL) - ai->last_bid_time) > SOLD_TIME) {
                auc_str = tmp_sprintf("Item number %d, %s, SOLD!", 
                                      ai->item_no, ai->item->name);
                imp = create_imp(self->in_room, *ai);
                ((imp_data *)imp->mob_specials.func_data)->mode = IMP_DELIVER_ITEM;
                obj_from_char(ai->item);
                obj_to_char(ai->item, imp);
                list<auction_data>::iterator ti = ai;
                ai++;
                items.erase(ti);
            }
            else if (ai->last_bid_time && 
                     (time(NULL) - ai->last_bid_time) > GOING_TWICE &&
                     ai->announce_count != GOING_TWICE) {
                auc_str = tmp_sprintf("Item number %d, %s, Going TWICE!", 
                                      ai->item_no, ai->item->name);
                ai->announce_count = GOING_TWICE;
            }
            else if (ai->last_bid_time &&
                     (time(NULL) - ai->last_bid_time) > GOING_ONCE &&
                     ai->announce_count != GOING_ONCE && 
                     ai->announce_count != GOING_TWICE) {
                auc_str = tmp_sprintf("Item number %d, %s, Going once!", 
                                      ai->item_no, ai->item->name);
                ai->announce_count = GOING_ONCE;
            }
            else if ((!ai->last_bid_time) &&
                    (time(NULL) - ai->start_time) > AUCTION_THRESH) {
                auc_str = tmp_sprintf("Item number %d, %s is no longer "
                                      "available for bids!",
                                      ai->item_no, ai->item->name);
                ai->current_bid = 0;
                ai->buyer_id = ai->owner_id;
                imp = create_imp(self->in_room, *ai);
                ((imp_data *)imp->mob_specials.func_data)->mode = IMP_RETURN_ITEM;
                obj_from_char(ai->item);
                obj_to_char(ai->item, imp);
                list<auction_data>::iterator ti = ai;
                ai++;
                items.erase(ti);
            }
            else if ((!ai->last_bid_time) && 
                     (time(NULL) - ai->start_time) > 
                     (NO_BID_THRESH * ai->announce_count)) {
                auc_str = tmp_sprintf("No bids yet for Item number %d, %s! "
                                      "Bidding starts at %ld coins!", 
                                      ai->item_no, ai->item->name,
                                      ai->start_bid);
                ai->announce_count++;
            }

            if (auc_str && *auc_str) {
                GET_MOOD(self) = moods[mood_index].mood_name;
                do_gen_comm(self, auc_str, 0, SCMD_AUCTION, NULL);
                GET_MOOD(self) = NULL;
                auc_str = NULL;
            }
            ++ai;
        }

        return 1;
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;
	
    if (IS_NPC(ch))
        return 0;

    // Handle commands in the presence of the auctioneer

    if (IS_IMMORT(ch)) {
        //Imm only commands here
        if (CMD_IS("stat")) {
            char *arg = tmp_getword(&argument);
            if (*arg)
                return 0;

            if (items.empty())
                send_to_char(ch, "There are no items for auction.\r\n");
            acc_string_clear();
            for (ai = items.begin(); ai != items.end(); ++ai) {
                acc_sprintf("%sItem Number:%s   %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), ai->item_no);
                acc_sprintf("%sOwner:%s         %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            playerIndex.getName(ai->owner_id));
                acc_sprintf("%sHigh Bidder:%s   %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            (ai->buyer_id) ? 
                            playerIndex.getName(ai->buyer_id) : "NULL");
                acc_sprintf("%sItem:%s          %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            ai->item->name);
                acc_sprintf("%sStart Time:%s    %s",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            ctime(&ai->start_time));
                acc_sprintf("%sLast Bid:%s      %s",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            (ai->last_bid_time ? 
                             ctime(&ai->last_bid_time) : "NULL\r\n"));
                acc_sprintf("%sCurrent Bid:%s   %ld coins/cash\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            ai->current_bid);
                acc_sprintf("%sStarting Bid:%s  %ld coins/cash\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            ai->start_bid);
                acc_sprintf("%sAnnounced:%s     %d times\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                            ai->announce_count);
                acc_strcat("---------------------------------------\r\n", NULL);
            }
            page_string(ch->desc, acc_get_string());
            return 1;
        }
    }

    if (CMD_IS("stun") || CMD_IS("steal") || 
        CMD_IS("pinch") || CMD_IS("glance")) {
        do_stun(self, tmp_sprintf("%s", GET_NAME(ch)), 0, 0, NULL);
        return 1;
    }

    if (CMD_IS("auction") && ch != self) {
        char *item_name = tmp_getword(&argument);
        long amount = atol(tmp_getword(&argument));

        if (GET_IDNUM(ch) < 0)
            return 0;

        if (!*item_name) {
            do_say(self, tmp_sprintf("%s I see you want to auction something, "
                   "but WHAT?", GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        if (!amount) {
            do_say(self, tmp_sprintf("%s And how much would you like to sell "
                   "it for?", GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        if (amount < 0) {
            do_say(self, tmp_sprintf("%s You want to pay someone to take it?!",
                   GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        if (amount > MAX_AUC_VALUE) {
            do_say(self, tmp_sprintf("%s I'm sorry, we have a maximum value "
                   "that can be placed on the items we sell.",
                   GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        obj_data *obj = get_obj_in_list_all(ch, item_name, ch->carrying);
        if (!obj) {
            do_say(self, tmp_sprintf("%s You don't even have that!  Stop "
                   "wasting my time!", GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        short item_count = 0;
        for (ai = items.begin(); ai != items.end(); ++ai) {
            if (ai->owner_id == ch->getIdNum())
                item_count++;

            if ((item_count >= MAX_AUC_ITEMS) && !IS_IMMORT(ch)) {
                do_say(self, tmp_sprintf("%s You already have too many items "
                       "up for auction", GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
                return 1; 
            }
        }

        if (BAD_AUCTION(obj) && !IS_IMMORT(ch)) {
            do_say(self, tmp_sprintf("%s I run a respectable establishment "
                   "here!  I don't deal in such trash!", GET_NAME(ch)), 
                    0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        int item_no = get_max_auction_item();
        if (item_no == -1) {
            do_say(self, tmp_sprintf("%s I'm sorry but my auction queue is "
                   "full at the moment.  Try again later.", GET_NAME(ch)), 
                    0, SCMD_SAY_TO, NULL);
            return 1; 
        }
        
        struct auction_data new_ai;
        new_ai.owner_id = ch->getIdNum();
        new_ai.buyer_id = 0;
        new_ai.item_no = item_no;
        new_ai.item = obj;
        new_ai.start_time = time(NULL);
        new_ai.last_bid_time = 0;
        new_ai.current_bid = 0;
        new_ai.new_bid = false;
        new_ai.new_item = true;
        new_ai.start_bid = amount;
        new_ai.announce_count = 1;

        obj_from_char(obj);
        obj_to_char(obj, self);
        ch->saveToXML();
        items.push_back(new_ai);
        items.sort();

        send_to_char(ch, "Your item has been entered for auction.\r\n");

        return 1;
    }

    if (CMD_IS("withdraw") && ch != self) {
        int item_no = atoi(tmp_getword(&argument));

        if (GET_IDNUM(ch) < 0)
            return 0;

        if (!item_no || item_no > MAX_TOTAL_AUC) {
            do_say(self, tmp_sprintf("%s Which item do you want to withdraw?",
                   GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
            return 1; 
        }

        list<auction_data>::iterator ai = items.begin();
        for (; ai != items.end(); ai++) {
            if (ai->item_no == item_no)
                break;
        }

        if (ai == items.end()) {
            send_to_char(ch, "That item doesn't exist!\r\n");
            return 1;
        }

        if (ch->getIdNum() != ai->owner_id && !IS_IMMORT(ch)) {
            send_to_char(ch, "You can only withdraw your own item!\r\n");
            return 1;
        }

        if (((ai->current_bid != 0) || (ai->buyer_id != 0)) && !IS_IMMORT(ch)) {
            send_to_char(ch, "You cannot withdraw and item that has bids\r\n");
            return 1;
        }

        if (ai->item == NULL) {
            send_to_char(ch, "Something bad just happened.  Please report it!");
            items.erase(ai);
            return 1;
        }

        obj_data *obj = ai->item;
        GET_MOOD(self) = " sadly";
        do_gen_comm(self,tmp_sprintf("Item number %d, %s, withdrawn.",
                    ai->item_no, obj->name), 0, SCMD_AUCTION, NULL);
        GET_MOOD(self) = NULL;
        
        obj_from_char(obj);
        obj_to_char(obj, ch);
        items.erase(ai);
        ch->saveToXML();

        send_to_char(ch, "Your item has been withdrawn from auction.\r\n");

        return 1;
    }

	return 0;
}

int 
get_max_auction_item() {
    int i;
    list<auction_data>::iterator ai = items.begin();

    items.sort();
    if (items.empty())
        return 1;
    
    for (i = 1; i < MAX_TOTAL_AUC; i++) {
        if (ai->item_no != i)
            return i;
        ++ai;
        if (ai == items.end())
            return ++i;
    }

    return -1;
}

Creature *
create_imp(room_data *inroom, auction_data &info) {
    imp_data *data;
    Creature *mob;
    
    mob = read_mobile(IMP_VNUM);
    CREATE(data, imp_data, 1);
    mob->mob_specials.func_data = data;
    data->imp = mob;
    data->buyer_id = info.buyer_id;
    data->owner_id = info.owner_id;
    data->item = info.item;
    data->owed = info.current_bid;
    data->fail_count = 0;
    data->mode = 0;
    char_to_room(mob, inroom, false);

    return mob;
}

ACMD(do_bidstat) {
    int item_no = atoi(tmp_getword(&argument));

    if (!item_no || item_no > MAX_TOTAL_AUC) {
        send_to_char(ch, "Which item do you want stats on?\r\n");
        return;
    }

    list<auction_data>::iterator ai = items.begin();
    for (; ai != items.end(); ai++) {
        if (ai->item_no == item_no)
            break;
    }

    if (ai == items.end()) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (ch->getIdNum() == ai->owner_id) {
        send_to_char(ch, "You can't get stats on your own item!\r\n");
        return;
    }

    spell_identify(49, ch, NULL, ai->item, NULL);
}

ACMD(do_bid) {
    int item_no = atoi(tmp_getword(&argument));
    long amount = atol(tmp_getword(&argument));

    if (!item_no || item_no > MAX_TOTAL_AUC) {
        send_to_char(ch, "Which item do you want to bid on?\r\n");
        return;
    }
    
    if (!amount) {
        send_to_char(ch, "How much do you want to bid?\r\n");
        return;
    }

    if (!bidder_can_afford(ch, amount)) {
        send_to_char(ch, "You don't have that much money!\r\n");
        return;
    }

    list<auction_data>::iterator ai = items.begin();
    for (; ai != items.end(); ai++) {
        if (ai->item_no == item_no)
            break;
    }

    if (ai == items.end()) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (ch->getIdNum() == ai->owner_id) {
        send_to_char(ch, "You can't bid on your own item!\r\n");
        return;
    }

    if (ch->getIdNum() == ai->buyer_id) {
        send_to_char(ch, "You are already the high bidder on that item!\r\n");
        return;
    }

    if (amount < MAX(ai->start_bid, ai->current_bid)) {
        send_to_char(ch, "Your bid amount is invalid!\r\n");
        return;
    }

    if (amount < (ai->current_bid + (ai->start_bid * BID_INCREMENT))) {
        send_to_char(ch, "Bids must be increased in %d coin increments.\r\n",
                     (int)(ai->start_bid * BID_INCREMENT));
        return;
    }

    ai->last_bid_time = time(NULL); 
    ai->buyer_id = ch->getIdNum();
    ai->current_bid = amount;
    ai->new_bid = true;

    send_to_char(ch, "Your bid has been entered.\r\n");
}

ACMD(do_bidlist) {
    if (items.empty())
        send_to_char(ch, "There are no items for auction.\r\n");

    acc_string_clear();
    list<auction_data>::iterator ai = items.begin();
    for (ai = items.begin(); ai != items.end(); ++ai) {
        acc_sprintf("%sItem Number:%s   %d\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), ai->item_no);
        acc_sprintf("%sItem:%s          %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                    ai->item->name);
        acc_sprintf("%sStarting Bid:%s  %ld coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                    ai->start_bid);
        acc_sprintf("%sCurrent Bid:%s   %ld coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), 
                    ai->current_bid);
        acc_strcat("---------------------------------------\r\n", NULL);
    }
    page_string(ch->desc, acc_get_string());
}

bool bidder_can_afford(Creature *bidder, long amount) {
    long tamount = amount;

    tamount = GET_GOLD(bidder) + GET_CASH(bidder) + 
              GET_PAST_BANK(bidder) + GET_FUTURE_BANK(bidder);

    return tamount > amount;
}













