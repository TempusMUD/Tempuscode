#include <time.h>
#include <math.h>
#include "obj_data.h"
#include "room_data.h"
#include "creature.h"

bool
BAD_AUCTION(struct obj_data *obj)
{
    return (obj->obj_flags.type_flag == ITEM_FOOD ||
            obj->obj_flags.type_flag == ITEM_TRASH ||
            obj->obj_flags.type_flag == ITEM_NOTE ||
            obj->obj_flags.type_flag == ITEM_KEY ||
            obj->obj_flags.type_flag == ITEM_PORTAL ||
            obj->obj_flags.type_flag == ITEM_SCRIPT ||
            obj->obj_flags.type_flag == ITEM_CONTAINER ||
            IS_NODROP(obj) ||
            (obj->obj_flags.type_flag == ITEM_BOMB &&
             obj->contains && FUSE_STATE(obj->contains)) ||
            obj->shared->vnum == -1);
}

#define AUC_FILE_NAME "etc/auctioneer_data_%d.xml"

// The vnum of the creature that delivers the goods and takes the cash
const int IMP_VNUM = 3223;
const int TOP_MOOD = 7;
int GOING_ONCE = 45;
int GOING_TWICE = 90;
int SOLD_TIME  = 135;
int NO_BID_THRESH  = 450;
int AUCTION_THRESH = 900;
int MAX_AUC_VALUE = 50000000;
int MAX_AUC_ITEMS = 5;
int MAX_TOTAL_AUC = 100;
float BID_INCREMENT = 0.05; //percent of starting bid

enum {
    IMP_RETURN_ITEM  = (1 << 0),
    IMP_NO_BUYER     = (1 << 1),
    IMP_BUYER_BROKE  = (1 << 2),
    IMP_DELIVER_ITEM = (1 << 3),
    IMP_DELIVER_CASH = (1 << 4),
};

ACMD(do_stun);

struct imp_data {
    struct creature *imp;
    long owner_id;
    long buyer_id;
    struct obj_data *item;
    long owed;
    int mode;
    short fail_count;
};

struct auctioneer_data {
    bool _loaded;
};

struct auction_data {
    long auctioneer_id;
    long owner_id;
    long buyer_id;
    struct obj_data *item;
    long start_bid;
    long start_time;
    long last_bid_time;
    long current_bid;
    bool new_bid;
    bool new_item;
    short announce_count;
};

GList *items;

const char *moods[] = {
    " fiercely",
    " vehemently",
    " wildly",
    " outrageously",
    " quickly",
    " crazily",
    " impatiently",
    " loudly",
    "\n"
};

struct creature *
create_imp(struct room_data *inroom, struct auction_data *info)
{
    struct imp_data *data;
    struct creature *mob;

    mob = read_mobile(IMP_VNUM);
    CREATE(data, struct imp_data, 1);
    mob->mob_specials.func_data = data;
    data->imp = mob;
    data->buyer_id = info->buyer_id;
    data->owner_id = info->owner_id;
    data->item = info->item;
    data->owed = info->current_bid;
    data->fail_count = 0;
    data->mode = 0;
    char_to_room(mob, inroom, false);

    return mob;
}

struct auction_data *
auction_data_from_item_no(int item_no)
{
    return g_list_nth_data(items, item_no - 1);
}


bool
aucLoadFromXML(struct creature *auc) {
    char *fname = tmp_sprintf(AUC_FILE_NAME, auc->in_room->number);

    if (access(fname, W_OK)) {
        errlog("Unable to open XML auctioneer file for loading. "
               "[%s] (%s)\n", fname, strerror(errno));
        return false;
    }

    xmlDocPtr doc = xmlParseFile(fname);
    if (!doc) {
        errlog("XML parse error while loading %s", fname);
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", fname);
        return false;
    }

    GOING_ONCE = xmlGetIntProp(root, "going_once", 0);
    GOING_TWICE = xmlGetIntProp(root, "going_twice", 0);
    SOLD_TIME = xmlGetIntProp(root, "sold_time", 0);
    NO_BID_THRESH = xmlGetIntProp(root, "nobid_thresh", 0);
    AUCTION_THRESH = xmlGetIntProp(root, "auction_thresh", 0);
    MAX_AUC_VALUE = xmlGetIntProp(root, "max_auc_value", 0);
    MAX_AUC_ITEMS = xmlGetIntProp(root, "max_auc_items", 0);
    MAX_TOTAL_AUC = xmlGetIntProp(root, "max_total_auc", 0);
    BID_INCREMENT = (float)atof(xmlGetProp(root, "bid_increment"));

    struct auction_data *new_ai;
    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlMatches(node->name, "itemdata")) {
            CREATE(new_ai, struct auction_data, 1);
            new_ai->auctioneer_id = GET_IDNUM(auc);
            new_ai->buyer_id = 0;
            new_ai->start_time = time(NULL);
            new_ai->last_bid_time = 0;
            new_ai->current_bid = 0;
            new_ai->new_bid = false;
            new_ai->new_item = false;
            new_ai->announce_count = 1;
            new_ai->owner_id = xmlGetIntProp(node, "owner_id", 0);
            new_ai->start_bid = xmlGetIntProp(node, "start_bid", 0);

            for (xmlNodePtr cnode = node->xmlChildrenNode;
                 cnode; cnode = cnode->next) {
                if (xmlMatches(cnode->name, "object")) {
                    struct obj_data *obj;
                    obj = load_object_from_xml(NULL, auc, NULL, cnode);
                    if (!obj) {
                        errlog("Auctioneer failed to load item from %s", fname);
                        extract_obj(obj);
                        continue;
                    }
                    new_ai->item = obj;
                    items = g_list_prepend(items, new_ai);
                }
            }
        }
    }

    return true;
}

int
get_max_auction_item(void)
{
    int max_item_no = g_list_length(items) + 1;

    if (max_item_no >= MAX_TOTAL_AUC)
        return -1;
    return max_item_no;
}

ACMD(do_bidstat) {
    int item_no = atoi(tmp_getword(&argument));

    if (!item_no || item_no > MAX_TOTAL_AUC) {
        send_to_char(ch, "Which item do you want stats on?\r\n");
        return;
    }
    struct auction_data *item = auction_data_from_item_no(item_no);

    if (!item) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (GET_IDNUM(ch) == item->owner_id) {
        send_to_char(ch, "You can't get stats on your own item!\r\n");
        return;
    }

    spell_identify(49, ch, NULL, item->item, NULL);
}bool
bidder_can_afford(struct creature *bidder, money_t amount)
{
    money_t tamount = amount;

    for (GList *ai = items;ai;ai = ai->next) {
        struct auction_data *item = ai->data;
        if (item->buyer_id == GET_IDNUM(bidder))
            amount += item->current_bid;
    }

    tamount = GET_GOLD(bidder) + GET_CASH(bidder) +
              GET_PAST_BANK(bidder) + GET_FUTURE_BANK(bidder);

    return tamount > amount;
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
        send_to_char(ch, "You can't afford that much money!\r\n");
        return;
    }

    struct auction_data *item = auction_data_from_item_no(item_no);

    if (!item) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (GET_IDNUM(ch) == item->owner_id) {
        send_to_char(ch, "You can't bid on your own item!\r\n");
        return;
    }

    if (GET_IDNUM(ch) == item->buyer_id) {
        send_to_char(ch, "You are already the high bidder on that item!\r\n");
        return;
    }

    if (item->current_bid == 0) {
        if (amount < item->start_bid) {
            send_to_char(ch, "Your bid amount is invalid, the starting bid is "
                         "%ld coins.\r\n", item->start_bid);
            return;
        }
    }
    else {
        if (amount <= item->current_bid) {
            send_to_char(ch, "Your bid amount is invalid, the current bid is "
                         "%ld coins.\r\n", item->current_bid);
            return;
        }
    }

    if (amount < (item->current_bid + (item->start_bid * BID_INCREMENT))) {
        send_to_char(ch, "Bids must be increased in %d coin increments.\r\n",
                     (int)(floor(item->start_bid * BID_INCREMENT)));
        return;
    }

    item->last_bid_time = time(NULL);
    item->buyer_id = GET_IDNUM(ch);
    item->current_bid = amount;
    item->new_bid = true;

    send_to_char(ch, "Your bid has been entered.\r\n");
}

ACMD(do_bidlist) {
    const char * obj_cond_color(struct obj_data *obj, struct creature *ch);

    if (!items)
        send_to_char(ch, "There are no items for auction.\r\n");

    acc_string_clear();
    int item_no = 1;
    for (GList *ai = items;ai;ai = ai->next, item_no++) {
        struct auction_data *item = ai->data;
        acc_sprintf("%sItem Number:%s   %d\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), item_no);
        acc_sprintf("%sItem:%s          %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    item->item->name);
        acc_sprintf("%sCondition:%s     %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    tmp_capitalize(obj_cond_color(item->item, ch)));
        acc_sprintf("%sStarting Bid:%s  %ld coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    item->start_bid);
        acc_sprintf("%sCurrent Bid:%s   %ld coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    item->current_bid);
        time_t time_left = 0;
        if (item->last_bid_time)
            time_left = (item->last_bid_time + SOLD_TIME) - time(NULL);
        else
            time_left = (item->start_time + AUCTION_THRESH) - time(NULL);
        // Convert to hours, mins, seconds
        int hours = time_left / 3600;
        time_left = time_left % 3600;
        int mins = time_left / 60;
        int secs = time_left % 60;
        acc_sprintf("%sTime Left:%s     %d Hour(s) %d Mins %d Secs\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    hours, mins, secs);
        acc_strcat("---------------------------------------\r\n", NULL);
    }
    page_string(ch->desc, acc_get_string());
}

void
aucSaveToXML(struct creature *auc) {
    FILE *ouf;
    char *fname = tmp_sprintf(AUC_FILE_NAME, auc->in_room->number);

    ouf = fopen(fname, "w");

    if (!ouf) {
        errlog("Unable to open XML auctioneer file for saving. "
               "[%s] (%s)\n", fname, strerror(errno));
        return;
    }

    fprintf(ouf, "<?xml version=\"1.0\"?>");
    fprintf(ouf, "<auctioneer going_once=\"%d\" going_twice=\"%d\" "
                 "sold_time=\"%d\" nobid_thresh=\"%d\" auction_thresh=\"%d\" "
                 "max_auc_value=\"%d\" max_auc_items=\"%d\" "
                 "max_total_auc=\"%d\" bid_increment=\"%f\">\n", GOING_ONCE,
                 GOING_TWICE, SOLD_TIME, NO_BID_THRESH, AUCTION_THRESH,
                 MAX_AUC_VALUE, MAX_AUC_ITEMS, MAX_TOTAL_AUC, BID_INCREMENT);

    for (GList *ai = items;ai;ai = ai->next) {
        struct auction_data *item = ai->data;
        if (item->auctioneer_id != GET_IDNUM(auc))
            continue;

        fprintf(ouf, "<itemdata owner_id=\"%ld\" start_bid=\"%ld\">\n",
                item->owner_id, item->start_bid);
        save_object_to_xml(item->item, ouf);
        fprintf(ouf, "</itemdata>\n");
    }
    fprintf(ouf, "</auctioneer>");
    fclose(ouf);
}

int
auctioneer_tick(struct creature *self)
{
    int mood_index;
    char *auc_str = NULL;
    struct creature *imp;

    int item_no = 1;
    for (GList *ai = items;ai;ai = ai->next, item_no++) {
        struct auction_data *item = ai->data;

        if (item->auctioneer_id != GET_IDNUM(self)) {
            continue;
        }

        mood_index = number(0, TOP_MOOD);
        if (item->new_bid) {
            auc_str = tmp_sprintf("%ld coins heard for item number %d, %s!!",
                                  item->current_bid, item_no,
                                  item->item->name);
            item->new_bid = false;
            item->announce_count = 0;
        }
        else if (item->new_item) {
            auc_str = tmp_sprintf("We now have a new item up for bids!  "
                                  "Item number %d, %s. We'll start the "
                                  "bidding at %ld coins!",
                                  item_no, item->item->name,
                                  item->start_bid);
            item->new_item = false;
        }
        else if (item->last_bid_time &&
                 (time(NULL) - item->last_bid_time) > SOLD_TIME) {
            auc_str = tmp_sprintf("Item number %d, %s, SOLD!",
                                  item_no, item->item->name);
            slog("AUCTION: %s (#%d) has been sold to %s (#%ld)",
                 item->item->name, GET_OBJ_VNUM(item->item),
                 player_name_by_idnum(item->buyer_id),
                 item->buyer_id);
            imp = create_imp(self->in_room, item);
            ((struct imp_data *)imp->mob_specials.func_data)->mode = IMP_DELIVER_ITEM;
            obj_from_char(item->item);
            obj_to_char(item->item, imp);
            items = g_list_remove(items, item);
            aucSaveToXML(self);
        }
        else if (item->last_bid_time &&
                 (time(NULL) - item->last_bid_time) > GOING_TWICE &&
                 item->announce_count != GOING_TWICE) {
            auc_str = tmp_sprintf("Item number %d, %s, Going TWICE!",
                                  item_no, item->item->name);
            item->announce_count = GOING_TWICE;
        }
        else if (item->last_bid_time &&
                 (time(NULL) - item->last_bid_time) > GOING_ONCE &&
                 item->announce_count != GOING_ONCE &&
                 item->announce_count != GOING_TWICE) {
            auc_str = tmp_sprintf("Item number %d, %s, Going once!",
                                  item_no, item->item->name);
            item->announce_count = GOING_ONCE;
        }
        else if ((!item->last_bid_time) &&
                 (time(NULL) - item->start_time) > AUCTION_THRESH) {
            auc_str = tmp_sprintf("Item number %d, %s is no longer "
                                  "avitemlable for bids!",
                                  item_no, item->item->name);
            item->current_bid = 0;
            item->buyer_id = item->owner_id;
            imp = create_imp(self->in_room, item);
            ((struct imp_data *)imp->mob_specials.func_data)->mode = IMP_RETURN_ITEM;
            obj_from_char(item->item);
            obj_to_char(item->item, imp);
            items = g_list_remove(items, item);
            aucSaveToXML(self);
        }
        else if ((!item->last_bid_time) &&
                 (time(NULL) - item->start_time) >
                 (NO_BID_THRESH * item->announce_count)) {
            auc_str = tmp_sprintf("No bids yet for Item number %d, %s! "
                                  "Bidding starts at %ld coins!",
                                  item_no, item->item->name,
                                  item->start_bid);
            item->announce_count++;
        }

        if (auc_str && *auc_str) {
            GET_MOOD(self) = moods[mood_index];
            do_gen_comm(self, auc_str, 0, SCMD_AUCTION, NULL);
            GET_MOOD(self) = NULL;
            auc_str = NULL;
        }
    }

    return 1;
}

SPECIAL(do_auctions)
{
	struct creature *self = (struct creature *)me;

    if (!self->mob_specials.func_data) {
        char *fname = tmp_sprintf(AUC_FILE_NAME, self->in_room->number);
        struct auctioneer_data *data;

        CREATE(data, struct auctioneer_data, 1);
        self->mob_specials.func_data = data;
        if (!access(fname, W_OK)) {
            if (aucLoadFromXML(self))
                data->_loaded = true;
            else
                data->_loaded = false;
        }
        data->_loaded = false;
    }

    struct creature *dick;
    while (self->fighting) {
        dick = random_opponent(self);
        act ("A ball of light streaks from $N's hand and hits you "
             "square in the chest, burning you to a cinder!", false,
             dick, 0, self, TO_CHAR);
        act ("A ball of light streaks from $n's hand and hits $N "
             "square in the chest, burning $M to a cinder!", false,
             self, 0, dick, TO_NOTVICT);
        raw_kill(dick, self, TYPE_SLASH);
    }

	if (spec_mode == SPECIAL_TICK)
        auctioneer_tick(self);

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

            if (!items)
                send_to_char(ch, "There are no items for auction.\r\n");
            acc_string_clear();
            int item_no = 1;
            for (GList *ai = items;ai;ai = ai->next, item_no++) {
                struct auction_data *item = ai->data;
                acc_sprintf("%sAuctioneer Id:%s   %ld\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), item->auctioneer_id);
                acc_sprintf("%sItem Number:%s   %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), item_no);
                acc_sprintf("%sOwner:%s         %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            player_name_by_idnum(item->owner_id));
                acc_sprintf("%sHigh Bidder:%s   %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            (item->buyer_id) ?
                            player_name_by_idnum(item->buyer_id) : "NULL");
                acc_sprintf("%sItem:%s          %s\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            item->item->name);
                acc_sprintf("%sStart Time:%s    %s",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            ctime(&item->start_time));
                acc_sprintf("%sLast Bid:%s      %s",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            (item->last_bid_time ?
                             ctime(&item->last_bid_time) : "NULL\r\n"));
                acc_sprintf("%sCurrent Bid:%s   %ld coins/cash\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            item->current_bid);
                acc_sprintf("%sStarting Bid:%s  %ld coins/cash\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            item->start_bid);
                acc_sprintf("%sAnnounced:%s     %d times\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                            item->announce_count);
                acc_strcat("---------------------------------------\r\n", NULL);
            }
            page_string(ch->desc, acc_get_string());
            return 1;
        }

        if (CMD_IS("aucset") && GET_LEVEL(ch) > LVL_ENERGY) {
            const char *aucset_commands[] = {
                "going_once",
                "going_twice",
                "sold_time",
                "nobid_thresh",
                "auction_thresh",
                "max_auc_value",
                "max_auc_items",
                "max_total_auc",
                "bid_increment",
                "\n"
            };

            char *var = tmp_getword(&argument);
            char *val = tmp_getword(&argument);
            int aucset_command;
            float fval;

            if (!*var && !*val) {
                // Print current settings
                acc_string_clear();
                acc_sprintf("%sGoing Once Time:%s    %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), GOING_ONCE);
                acc_sprintf("%sGoing Twice Time:%s   %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), GOING_TWICE);
                acc_sprintf("%sSold Time:%s          %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), SOLD_TIME);
                acc_sprintf("%sNo Bids Announce:%s   %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), NO_BID_THRESH);
                acc_sprintf("%sMax Auction Time:%s   %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), AUCTION_THRESH);
                acc_sprintf("%sMax Item Value:%s     %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), MAX_AUC_VALUE);
                acc_sprintf("%sMax Items/Pers:%s     %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), MAX_AUC_ITEMS);
                acc_sprintf("%sMax Total Aucs:%s     %d\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), MAX_TOTAL_AUC);
                acc_sprintf("%sBid Increment:%s      %f\r\n",
                            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), BID_INCREMENT);
                page_string(ch->desc, acc_get_string());
                return 1;
            }
            else if (is_abbrev(var, "help") || !*var || !*val) {
                // Print usage
                send_to_char(ch, "Usage: aucset <VAR> <VAL>\r\n\r\n"
                                 "The following variables are accepted:\r\n"
                                 "going_once\r\ngoing_twice\r\nsold_time\r\n"
                                 "nobid_thresh\r\nauction_thresh\r\n"
                                 "max_auc_value\r\nmax_auc_items\r\n"
                                 "max_total_auc\r\nbid_increment\r\n\r\n");
                return 1;
            }
            // They knew what they were doing, set the var
            if ((aucset_command =
                        search_block(var, aucset_commands, false)) < 0) {
                send_to_char(ch, "Invalid aucset command.\r\n");
                return 1;
            }

            if ((fval = (float)(atof(val))) <= 0) {
                send_to_char(ch, "That's not a valid value.\r\n");
                return 1;
            }

            switch(aucset_command) {
                case 0:
                    GOING_ONCE = (int)fval;
                    break;
                case 1:
                    GOING_TWICE = (int)fval;
                    break;
                case 2:
                    SOLD_TIME = (int)fval;
                    break;
                case 3:
                    NO_BID_THRESH = (int)fval;
                    break;
                case 4:
                    AUCTION_THRESH = (int)fval;
                    break;
                case 5:
                    MAX_AUC_VALUE = (int)fval;
                    break;
                case 6:
                    MAX_AUC_ITEMS = (int)fval;
                    break;
                case 7:
                    MAX_TOTAL_AUC = (int)fval;
                    break;
                case 8:
                    BID_INCREMENT = fval;
                    break;
            }

            aucSaveToXML(self);
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
            perform_say_to(self, ch, "I see you want to auction something, but WHAT?");
            return 1;
        }

        if (!amount) {
            perform_say_to(self, ch, "And how much would you like to sell it for?");
            return 1;
        }

        if (amount < 0) {
            perform_say_to(self, ch, "You want to pay someone to take it?!");
            return 1;
        }

        if (amount > MAX_AUC_VALUE) {
            perform_say_to(self, ch, "I'm sorry, we have a maximum value "
                           "that can be placed on the items we sell.");
            return 1;
        }

        struct obj_data *obj = get_obj_in_list_all(ch, item_name, ch->carrying);
        if (!obj) {
            perform_say_to(self, ch, "You don't even have that!  Stop "
                           "wasting my time!");
            return 1;
        }

        short item_count = 0;
        for (GList *ai = items;ai;ai = ai->next) {
            struct auction_data *item = ai->data;
            if (item->owner_id == GET_IDNUM(ch))
                item_count++;

            if ((item_count >= MAX_AUC_ITEMS) && !IS_IMMORT(ch)) {
                perform_say_to(self, ch, "You already have too many items "
                               "up for auction.");
                return 1;
            }
        }

        if (BAD_AUCTION(obj) && !IS_IMMORT(ch)) {
            perform_say_to(self, ch, "I run a respectable establishment "
                           "here!  I don't deal in such trash!");
            return 1;
        }

        int item_no = get_max_auction_item();
        if (item_no == -1) {
            perform_say_to(self, ch, "There are too many items up for "
                           "auction now.  Try again later.");
            return 1;
        }

        struct auction_data *new_ai;
        CREATE(new_ai, struct auction_data, 1);
        new_ai->auctioneer_id = GET_IDNUM(self);
        new_ai->owner_id = GET_IDNUM(ch);
        new_ai->buyer_id = 0;
        new_ai->item = obj;
        new_ai->start_time = time(NULL);
        new_ai->last_bid_time = 0;
        new_ai->current_bid = 0;
        new_ai->new_bid = false;
        new_ai->new_item = true;
        new_ai->start_bid = amount;
        new_ai->announce_count = 1;

        obj_from_char(obj);
        obj_to_char(obj, self);
        save_player_to_xml(ch);
        items = g_list_prepend(items, new_ai);
        aucSaveToXML(self);

        send_to_char(ch, "Your item has been entered for auction.\r\n");
        slog("AUCTION: %s (#%ld) has put up %s (#%d) for auction",
             GET_NAME(ch), GET_IDNUM(ch),
             obj->name, GET_OBJ_VNUM(obj));

        return 1;
    }

    if (CMD_IS("withdraw") && ch != self) {
        int item_no = atoi(tmp_getword(&argument));

        if (GET_IDNUM(ch) < 0)
            return 0;

        if (!item_no || item_no > MAX_TOTAL_AUC) {
            perform_say_to(self, ch, "Which item do you want to withdraw?");
            return 1;
        }

        struct auction_data *item = auction_data_from_item_no(item_no);

        if (!item) {
            perform_say_to(self, ch, "I don't see that item!  "
                           "Maybe you should try another auctioneer.\r\n");
            return 1;
        }

        if (GET_IDNUM(ch) != item->owner_id && !IS_IMMORT(ch)) {
            send_to_char(ch, "You can only withdraw your own item!\r\n");
            return 1;
        }

        if (((item->current_bid != 0) || (item->buyer_id != 0)) && !IS_IMMORT(ch)) {
            send_to_char(ch, "You cannot withdraw an item that has bids\r\n");
            return 1;
        }

        if (item->item == NULL) {
            send_to_char(ch, "Something bad just happened. Please report it!");
            items = g_list_remove(items, item);
            free(item);
            return 1;
        }

        struct obj_data *obj = item->item;
        GET_MOOD(self) = " sadly";
        do_gen_comm(self,tmp_sprintf("Item number %d, %s, withdrawn.",
                    item_no, obj->name), 0, SCMD_AUCTION, NULL);
        GET_MOOD(self) = NULL;

        obj_from_char(obj);
        obj_to_char(obj, ch);
        items = g_list_remove(items, item);
        save_player_to_xml(ch);
        aucSaveToXML(self);

        send_to_char(ch, "Your item has been withdrawn from auction.\r\n");
        slog("AUCTION: %s (#%ld) has withdrawn %s (#%d)",
             GET_NAME(ch), GET_IDNUM(ch),
             obj->name, GET_OBJ_VNUM(obj));

        return 1;
    }

	return 0;
}
