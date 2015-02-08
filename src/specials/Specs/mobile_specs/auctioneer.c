#include <time.h>
#include <math.h>
#include "obj_data.h"
#include "room_data.h"
#include "creature.h"

const char *AUC_FILE_PATH = "etc/auctions.xml";

const int TOP_MOOD = 7;
int GOING_ONCE = 45;
int GOING_TWICE = 90;
int SOLD_TIME = 135;
int NO_BID_THRESH = 450;
int AUCTION_THRESH = 900;
int MAX_AUC_VALUE = 500000000;
int MAX_AUCTIONS = 5;
int MAX_TOTAL_AUC = 100;
float BID_INCREMENT = 0.05;     // percent of starting bid

enum auction_state {
    AUCTION_STARTED = 0, /* initial state: auction needs to be announced */
    AUCTION_ANNOUNCED, /* auction has been announced and is waiting for bids */
    AUCTION_NO_BIDS, /* it's been announced that no bids have been made */
    AUCTION_NEW_BID,   /* a bid has just been made */
    AUCTION_BID_ANNOUNCED,   /* the new bid has been announced */
    AUCTION_GOING_ONCE,      /* the bid has gone once */
    AUCTION_GOING_TWICE,     /* the bid has gone twice */
};

const char *auction_state_strs[] = {
    "started", "announced", "no bids", "new bid", "bid announced", "going once",
    "going twice"
};

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

ACMD(do_stun);

void save_auctions(void);

/**
 * bid_data:
 * @bidder_id: The idnum of the bidder
 * @past_amount: The amount of gold used in the bid
 * @future_amount: The amount of cash used in the bid
 **/
struct bid_data {
    long bidder_id;
    money_t past_amount;
    money_t future_amount;
};

/**
 * auction_data:
 * @idnum: a unique id number for the auction
 * @room_id: The room of the auctioneer holding this auction
 * @owner_id: The player ID of the owner of the item
 * @item: The object being auctioned
 * @start_bid: The minimum bid of the auction
 * @start_time: The time the auction started
 * @last_bid_time: The last time the bid was made
 * @new_bid: Whether the new bid announcement has been made
 * @new_item: Whether this auction has started
 * @announce_count: The state of the auction
 * @bids: A #GList of bids, sorted from greatest to least amount
 *
 **/
struct auction_data {
    uint32_t idnum;
    long room_id;
    long owner_id;
    struct obj_data *item;
    money_t start_bid;
    time_t start_time;
    time_t last_bid_time;
    enum auction_state state;
    GList *bids;
};

GList *auctions;

#define PLURAL(num) (num == 1 ? "" : "s")

money_t
bid_total(struct bid_data *bid)
{
    return bid->past_amount + bid->future_amount;
}

char *
describe_bid_amount(struct bid_data *bid)
{
    if (bid->past_amount > 0 && bid->future_amount > 0) {
        return tmp_sprintf("%'" PRId64 " gold coin%s and %'" PRId64 " credit%s",
                           bid->past_amount,
                           PLURAL(bid->past_amount),
                           bid->future_amount,
                           PLURAL(bid->future_amount));
    } else if (bid->past_amount > 0) {
        return tmp_sprintf("%'" PRId64 " gold coin%s",
                           bid->past_amount,
                           PLURAL(bid->past_amount));
    } else if (bid->future_amount > 0) {
        return tmp_sprintf("%'" PRId64 " credit%s",
                           bid->future_amount,
                           PLURAL(bid->future_amount));
    } else {
        return tmp_strdup("nothing");
    }
}

struct auction_data *
auction_data_from_idnum(int idnum)
{
    for (GList *it = auctions; it; it = it->next) {
        struct auction_data *auc = it->data;
        if (auc->idnum == idnum) {
            return auc;
        }
    }
    return NULL;
}

uint32_t
unused_auction_idnum(void)
{
    uint32_t result = 1;
    for (GList *it = auctions; it; it = it->next) {
        struct auction_data *auc = it->data;
        if (auc->idnum >= result) {
            result = auc->idnum + 1;
        }
    }
    return result;
}

/**
 * new_auction:
 *
 * Allocate a blank initialized auction.
 *
 * Returns: An initialized #auction_data
 **/
struct auction_data *
new_auction(void)
{
    struct auction_data *new_ai;

    CREATE(new_ai, struct auction_data, 1);
    new_ai->state = AUCTION_STARTED;

    return new_ai;
}

/**
 * add_new_auction
 * @room_id: Number of the auctioneer's room
 * @seller_id: Number of the selling player
 * @start_bid: Minimum bid of the auction
 * @obj: Object to be sold
 *
 * Creates a new auction and adds it to the collection.
 *
 * Returns: The new auction.
 *
 **/
struct auction_data *
add_new_auction(long room_id, long seller_id, money_t start_bid, struct obj_data *obj)
{
    struct auction_data *new_ai = new_auction();
    new_ai->idnum = unused_auction_idnum();
    new_ai->room_id = room_id;
    new_ai->owner_id = seller_id;
    new_ai->item = obj;
    new_ai->start_time = time(NULL);
    new_ai->start_bid = start_bid;

    auctions = g_list_append(auctions, new_ai);
    save_auctions();

    return new_ai;
}

/**
 * free_auction:
 * @auc The #auction_data to free
 *
 * Deallocates an auction
 */
void
free_auction(struct auction_data *auc)
{
    g_list_foreach(auc->bids, (GFunc)free, NULL);
    if (auc->item) {
        extract_obj(auc->item);
    }
    free(auc);
}

/**
 * load_auction_from_xml
 * @path: The path of the XML file, used for error reporting
 * @node: The node of the auction entry
 *
 * Loads an auction from an XML document.
 *
 * Returns: The loaded #auction_data
 *
 **/
struct auction_data *
load_auction_from_xml(const char *path, xmlNodePtr node)
{
    if (!xmlMatches(node->name, "itemdata")) {
        return NULL;
    }
    struct auction_data *new_ai = new_auction();
    new_ai->room_id = xmlGetIntProp(node, "room_id", 0);
    new_ai->owner_id = xmlGetIntProp(node, "owner_id", 0);
    new_ai->start_bid = xmlGetIntProp(node, "start_bid", 0);
    new_ai->idnum = xmlGetIntProp(node, "id", 0);
    new_ai->start_time = time(NULL);

    for (xmlNodePtr cnode = node->xmlChildrenNode;
         cnode; cnode = cnode->next) {
        if (xmlMatches(cnode->name, "object")) {
            new_ai->item = load_object_from_xml(NULL, NULL, NULL, cnode);
            if (!new_ai->item) {
                errlog("Auctioneer failed to load item from %s", path);
                continue;
            }
        } else if (xmlMatches(cnode->name, "bid")) {
            struct bid_data *bid;
            CREATE(bid, struct bid_data, 1);
            bid->bidder_id = xmlGetIntProp(cnode, "bidder", -1);
            bid->past_amount = xmlGetIntProp(cnode, "past_amount", 0);
            bid->future_amount = xmlGetIntProp(cnode, "future_amount", 0);
            new_ai->bids = g_list_prepend(new_ai->bids, bid);
        }
    }
    return new_ai;
}

/**
 * load_auctions_from_doc:
 *
 * Loads auctions from an XML document
 *
 * Returns: %true if loading was successful
 **/
bool
load_auctions_from_doc(xmlDocPtr doc)
{
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", AUC_FILE_PATH);
        return false;
    }

    GOING_ONCE = xmlGetIntProp(root, "going_once", 0);
    GOING_TWICE = xmlGetIntProp(root, "going_twice", 0);
    SOLD_TIME = xmlGetIntProp(root, "sold_time", 0);
    NO_BID_THRESH = xmlGetIntProp(root, "nobid_thresh", 0);
    AUCTION_THRESH = xmlGetIntProp(root, "auction_thresh", 0);
    MAX_AUC_VALUE = xmlGetIntProp(root, "max_auc_value", 0);
    MAX_AUCTIONS = xmlGetIntProp(root, "max_auctions", 0);
    MAX_TOTAL_AUC = xmlGetIntProp(root, "max_total_auc", 0);
    BID_INCREMENT =
        (float)atof((char *)xmlGetProp(root, (xmlChar *) "bid_increment"));

    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        struct auction_data *new_ai = load_auction_from_xml(AUC_FILE_PATH, node);
        if (new_ai) {
            auctions = g_list_append(auctions, new_ai);
        }
    }

    for (GList *it = auctions; it; it = it->next) {
        struct auction_data *auc = it->data;
        if (auc->idnum == 0) {
            auc->idnum = unused_auction_idnum();
        }
    }

    return true;
}

/**
 * load_auctions:
 *
 * Loads auctions from the XML file at #AUC_FILE_PATH.
 *
 * Returns: %true if loading was successful
 **/
bool
load_auctions()
{
    xmlDocPtr doc = xmlParseFileWithLog(AUC_FILE_PATH);
    if (!doc) {
        return false;
    }

    return load_auctions_from_doc(doc);
}

/**
 * save_auctions
 *
 * Saves all auctions to an XML file at #AUC_FILE_PATH.
 *
 **/
void
save_auctions(void)
{
    FILE *ouf = fopen(AUC_FILE_PATH, "w");

    if (!ouf) {
        errlog("Can't write to auction file %s: %s",
               AUC_FILE_PATH, strerror(errno));
        return;
    }

    fprintf(ouf, "<?xml version=\"1.0\"?>\n");
    fprintf(ouf, "<auctioneer going_once=\"%d\" going_twice=\"%d\" "
                 "sold_time=\"%d\" nobid_thresh=\"%d\" auction_thresh=\"%d\" "
                 "max_auc_value=\"%d\" max_auctions=\"%d\" "
                 "max_total_auc=\"%d\" bid_increment=\"%f\">\n", GOING_ONCE,
            GOING_TWICE, SOLD_TIME, NO_BID_THRESH, AUCTION_THRESH,
            MAX_AUC_VALUE, MAX_AUCTIONS, MAX_TOTAL_AUC, BID_INCREMENT);

    for (GList *ai = auctions; ai; ai = ai->next) {
        struct auction_data *auc = ai->data;

        fprintf(ouf, "<itemdata id=\"%" PRIu32 "\" room_id=\"%ld\" owner_id=\"%ld\" start_bid=\"%" PRId64 "\">\n",
                auc->idnum, auc->room_id, auc->owner_id, auc->start_bid);
        save_object_to_xml(auc->item, ouf);
        for (GList *it = auc->bids; it; it = it->next) {
            struct bid_data *bid = it->data;
            fprintf(ouf, "  <bid bidder=\"%ld\"  past_amount=\"%" PRId64 "\" future_amount=\"%" PRId64 "\"/>\n",
                    bid->bidder_id,
                    bid->past_amount,
                    bid->future_amount);
        }
        fprintf(ouf, "</itemdata>\n");
    }
    fprintf(ouf, "</auctioneer>");
    fclose(ouf);
}

/**
 * auction_winning_bid:
 * @auc: The auction being queried
 *
 * Returns: the winning bid of the auction if one exists, otherwise %NULL
 **/
struct bid_data *
auction_winning_bid(struct auction_data *auc)
{
    if (auc->bids == NULL) {
        return NULL;
    }

    return (struct bid_data *)auc->bids->data;
}

/**
 * is_open_auction:
 * @auc The auction being predicated upon
 *
 * Returns: %true if the auction can take bids, otherwise %false
 **/
bool
is_open_auction(struct auction_data *auc)
{
    int open_states[] = { AUCTION_ANNOUNCED,
                          AUCTION_NO_BIDS,
                          AUCTION_NEW_BID,
                          AUCTION_BID_ANNOUNCED,
                          AUCTION_GOING_ONCE,
                          AUCTION_GOING_TWICE,
                          -1 };

    for (int i = 0; open_states[i] != -1; i++) {
        if (auc->state == open_states[i]) {
            return true;
        }
    }
    return false;
}

/**
 * auction_minimum_bid:
 * @auc: Auction to calculate minimum bid for
 *
 * Calculates the minimum bid for the auction.
 *
 * Returns: The minimum bid amount
 **/
money_t
auction_minimum_bid(struct auction_data *auc)
{
    money_t min_bid = auc->start_bid;
    struct bid_data *winning_bid = auction_winning_bid(auc);

    if (winning_bid != NULL) {
        money_t inc = auc->start_bid * BID_INCREMENT;
        min_bid = bid_total(winning_bid) + MAX(1, inc);
    }

    return min_bid;
}

/**
 * bid_by_bidder_idnum:
 * @auc: Auction to search for bid
 * @idnum: Idnum of bidder to match
 *
 * Finds the bid made by the player with @idnum.
 *
 * Returns: The bid if found, otherwise %NULL
 **/
struct bid_data *
bid_by_bidder_idnum(struct auction_data *auc, long idnum)
{
    for (GList *it = auc->bids; it; it = it->next) {
        struct bid_data *bid = it->data;

        if (bid->bidder_id == idnum) {
            return bid;
        }
    }

    return NULL;
}

/**
 * can_make_bid:
 * @bidder: The player making the bid
 * @auc: The auction on which the bid will be made
 * @amount: Amount of the new bid
 *
 * Determines if @bidder can bid the @amount.
 *
 * Returns: %true if @bidder can make the bid.
 **/
bool
can_make_bid(struct creature *bidder, struct auction_data *auc, money_t amount)
{
    struct bid_data *bid = bid_by_bidder_idnum(auc, GET_IDNUM(bidder));
    money_t deduct_amt = amount;

    if (bid) {
        deduct_amt = amount - bid_total(bid);
    }

    return (GET_PAST_BANK(bidder) + GET_FUTURE_BANK(bidder)) > deduct_amt;
}

/**
 * reversed_compare_bids:
 * @a: A bid to compare
 * @b: A bid to compare
 *
 * A #GCompareFunc for sorting bids in order from greatest to least.
 *
 * Returns: a #gint indicating the sort order
 **/
gint
reversed_compare_bids(struct bid_data *a, struct bid_data *b)
{
    money_t a_total = bid_total(a);
    money_t b_total = bid_total(b);

    if (a_total < b_total) {
        return 1;
    }
    if (a_total > b_total) {
        return -1;
    }
    return 0;
}

/**
 * make_bid:
 * @auc: The auction to bid on
 * @ch: The creature making the bid
 * @amount: The amount of money to bid
 *
 * Adds a bid by player @ch to the auction @auc for the given amount.
 * Withdraws the bid amount or the difference from a previous bid from
 * the player's bank account.
 *
 **/
void
make_bid(struct auction_data *auc, struct creature *ch, money_t amount)
{
    struct bid_data *bid = bid_by_bidder_idnum(auc, GET_IDNUM(ch));
    money_t deduct_amt = amount;
    money_t deduct_past = 0;
    money_t deduct_future = 0;

    if (bid) {
        deduct_amt = amount - bid_total(bid);
    }

    if (GET_PAST_BANK(ch) > GET_FUTURE_BANK(ch)) {
        deduct_past = MIN(GET_PAST_BANK(ch), deduct_amt);
        deduct_amt -= deduct_past;
        deduct_future = MIN(GET_FUTURE_BANK(ch), deduct_amt);
        deduct_amt -= deduct_future;
    } else {
        deduct_future = MIN(GET_FUTURE_BANK(ch), deduct_amt);
        deduct_amt -= deduct_future;
        deduct_past = MIN(GET_PAST_BANK(ch), deduct_amt);
        deduct_amt -= deduct_past;
    }

    if (deduct_amt != 0) {
        errlog("Bidder couldn't pay in make_bid()");
        return;
    }

    if (deduct_past > 0) {
        withdraw_past_bank(ch->account, deduct_past);
    }
    if (deduct_future > 0) {
        withdraw_future_bank(ch->account, deduct_future);
    }

    if (bid) {
        bid->past_amount += deduct_past;
        bid->future_amount += deduct_future;
        auc->bids = g_list_sort(auc->bids, (GCompareFunc)reversed_compare_bids);
    } else {
        CREATE(bid, struct bid_data, 1);
        bid->bidder_id = GET_IDNUM(ch);
        bid->past_amount = deduct_past;
        bid->future_amount = deduct_future;
        auc->bids = g_list_prepend(auc->bids, bid);
    }
    save_auctions();
}

/**
 * refund_bid
 * @bid: The bid to be refunded
 *
 * Deposits the bid money back into the bidder's bank accounts.
 **/
void
refund_bid(struct bid_data *bid)
{
    // Load account and add bid amount to banks
    long acc_id = player_account_by_idnum(bid->bidder_id);
    if (acc_id == 0) {
        slog("WARNING: refund_bid(), bidder %ld was not found",
             bid->bidder_id);
        return;
    }
    struct account *acc = account_by_idnum(acc_id);
    if (acc == NULL) {
        errlog("WARNING: refund_bid(), bidder %ld's account was not found",
               bid->bidder_id);
        return;
    }
    if (bid->past_amount != 0) {
        deposit_past_bank(acc, bid->past_amount);
    }
    if (bid->future_amount != 0) {
        deposit_future_bank(acc, bid->future_amount);
    }
}

/**
 * can_auction_obj:
 * @obj The object to evaluate for auctionability
 *
 * Returns: %true if the object can be auctioned, otherwise %false
 **/
bool
can_auction_obj(struct obj_data *obj)
{
    return (obj->obj_flags.type_flag == ITEM_FOOD
            || obj->obj_flags.type_flag == ITEM_TRASH
            || obj->obj_flags.type_flag == ITEM_NOTE
            || obj->obj_flags.type_flag == ITEM_KEY
            || obj->obj_flags.type_flag == ITEM_PORTAL
            || obj->obj_flags.type_flag == ITEM_SCRIPT
            || obj->obj_flags.type_flag == ITEM_CONTAINER
            || IS_NODROP(obj)
            || (obj->obj_flags.type_flag == ITEM_BOMB
                && obj->contains
                && FUSE_STATE(obj->contains))
            || obj->shared->vnum == -1);
}

void
end_auction(struct auction_data *auc)
{
    auctions = g_list_remove(auctions, auc);
    auc->item = NULL;
    free_auction(auc);
    save_auctions();
}

ACMD(do_bidstat)
{
    int idnum = atoi(tmp_getword(&argument));

    if (!idnum || idnum > MAX_TOTAL_AUC) {
        send_to_char(ch, "Which item do you want stats on?\r\n");
        return;
    }
    struct auction_data *auc = auction_data_from_idnum(idnum);

    if (!auc) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (GET_IDNUM(ch) == auc->owner_id) {
        send_to_char(ch, "You can't get stats on your own item!\r\n");
        return;
    }

    spell_identify(49, ch, NULL, auc->item, NULL);
}

ACMD(do_bid)
{
    int auc_no = atoi(tmp_getword(&argument));
    long amount = atol(tmp_getword(&argument));

    if (!auc_no || auc_no > MAX_TOTAL_AUC) {
        send_to_char(ch, "Which item do you want to bid on?\r\n");
        return;
    }

    if (!amount) {
        send_to_char(ch, "How much do you want to bid?\r\n");
        return;
    }

    struct auction_data *auc = auction_data_from_idnum(auc_no);

    if (!auc) {
        send_to_char(ch, "That item doesn't exist!\r\n");
        return;
    }

    if (GET_IDNUM(ch) == auc->owner_id) {
        send_to_char(ch, "You can't bid on your own item!\r\n");
        return;
    }

    if (!can_make_bid(ch, auc, amount)) {
        send_to_char(ch, "You can't afford that much money!\r\n");
        return;
    }

    money_t min_bid = auction_minimum_bid(auc);
    if (amount < min_bid) {
        send_to_char(ch, "You must bid at least %'" PRId64 ".\r\n", min_bid);
        return;
    }

    make_bid(auc, ch, amount);
    auc->last_bid_time = time(NULL);
    auc->state = AUCTION_NEW_BID;

    send_to_char(ch, "Your bid has been entered.\r\n");
}

ACMD(do_bidlist)
{
    if (!auctions) {
        send_to_char(ch, "There are no items for auction.\r\n");
    }

    acc_string_clear();
    for (GList *ai = auctions; ai; ai = ai->next) {
        struct auction_data *auc = ai->data;

        if (!is_open_auction(auc)) {
            continue;
        }

        acc_sprintf("%sItem Number:%s   %d\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->idnum);
        acc_sprintf("%sItem:%s          %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->item->name);
        acc_sprintf("%sCondition:%s     %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    tmp_capitalize(obj_cond_color(auc->item, COLOR_LEV(ch))));
        acc_sprintf("%sStarting Bid:%s  %'" PRId64 " coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->start_bid);
        if (auc->bids == NULL) {
            acc_sprintf("%sCurrent Bid:%s   None\r\n",
                        CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
        } else {
            acc_sprintf("%sCurrent Bid:%s   %'" PRId64 " coins/cash\r\n",
                        CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                        bid_total(auction_winning_bid(auc)));
        }
        acc_sprintf("%sMinimum Bid:%s  %'" PRId64 " coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auction_minimum_bid(auc));
        time_t time_left = 0;
        if (auc->bids) {
            time_left = (auc->last_bid_time + SOLD_TIME) - time(NULL);
        } else {
            time_left = (auc->start_time + AUCTION_THRESH) - time(NULL);
        }

        // Convert to hours, mins, seconds
        int hours = time_left / 3600;
        time_left = time_left % 3600;
        int mins = time_left / 60;
        int secs = time_left % 60;
        acc_sprintf("%sTime Left:%s     %d Hour(s) %d Mins %d Secs\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), hours, mins, secs);

        if (ai->next) {
            acc_strcat("---------------------------------------\r\n", NULL);
        }
    }
    page_string(ch->desc, acc_get_string());
}

static void
auction_mail(struct creature *self,
             long to_idnum,
             struct obj_data *item,
             const char *txt,
             ...)
{
    va_list args;
    char *error = NULL;

    va_start(args, txt);
    char *msg = tmp_vsprintf(txt, args);
    va_end(args);

    GList *recipient = g_list_append(NULL, GINT_TO_POINTER(to_idnum));
    bool sent = send_mail(self, recipient, msg, item, &error);
    if (!sent) {
        errlog("Error while sending mail from auction: %s", error);
    }

    g_list_free(recipient);
}

int
auctioneer_tick(struct creature *self)
{
    int mood_index;
    char *auc_str = NULL;
    GList *next_ai;

    for (GList *ai = auctions; ai; ai = next_ai) {
        next_ai = ai->next;
        struct auction_data *auc = ai->data;
        struct bid_data *winning_bid = auction_winning_bid(auc);

        if (auc->room_id != self->in_room->number) {
            continue;
        }

        if ((!auc->last_bid_time) &&
            (time(NULL) - auc->start_time) > AUCTION_THRESH) {
            auc_str = tmp_sprintf("Item number %d, %s is no longer "
                                  "available for bids!", auc->idnum, auc->item->name);
            auction_mail(self, auc->owner_id, auc->item,
                         "Sorry, your %s did not get any bids.\r\n"
                         "Please try again at %s!\r\n"
                         "- %s",
                         auc->item->name,
                         self->in_room->name,
                         GET_NAME(self));
            end_auction(auc);
            save_auctions();
            continue;
        }

        mood_index = number(0, TOP_MOOD);
        switch (auc->state) {
        case AUCTION_STARTED:
            auc_str = tmp_sprintf("We now have a new item up for bids!  "
                                  "Item number %d, %s. We'll start the "
                                  "bidding at %'" PRId64 " coins!",
                                  auc->idnum, auc->item->name, auc->start_bid);
            auc->state = AUCTION_ANNOUNCED;
            break;
        case AUCTION_ANNOUNCED:
            if (auc->bids == NULL && (time(NULL) - auc->start_time) > NO_BID_THRESH) {
                auc_str = tmp_sprintf("No bids yet for Item number %d, %s! "
                                      "Do I hear %'" PRId64 " coins?",
                                      auc->idnum, auc->item->name, auc->start_bid);
                auc->state = AUCTION_NO_BIDS;
            }
            break;
        case AUCTION_NEW_BID:
            auc_str = tmp_sprintf("%'" PRId64 " coins heard for item number %d, %s!!  Do I hear %'" PRId64 "?",
                                  bid_total(winning_bid), auc->idnum, auc->item->name,
                                  auction_minimum_bid(auc));
            auc->state = AUCTION_BID_ANNOUNCED;
            break;
        case AUCTION_BID_ANNOUNCED:
            if (auc->last_bid_time &&
                (time(NULL) - auc->last_bid_time) > GOING_ONCE) {
                auc_str = tmp_sprintf("Item number %d, %s, Going once!",
                                      auc->idnum, auc->item->name);
                auc->state = AUCTION_GOING_ONCE;
            }
            break;
        case AUCTION_GOING_ONCE:
            if (auc->last_bid_time &&
                (time(NULL) - auc->last_bid_time) > GOING_TWICE) {
                auc_str = tmp_sprintf("Item number %d, %s, Going TWICE!",
                                      auc->idnum, auc->item->name);
                auc->state = AUCTION_GOING_TWICE;
            }
            break;
        case AUCTION_GOING_TWICE:
            if (auc->last_bid_time
                && (time(NULL) - auc->last_bid_time) > SOLD_TIME) {
                auc_str = tmp_sprintf("Item number %d, %s, SOLD!",
                                      auc->idnum, auc->item->name);
                slog("AUCTION: %s (#%d) has been sold to %s (#%ld)",
                     auc->item->name, GET_OBJ_VNUM(auc->item),
                     player_name_by_idnum(winning_bid->bidder_id), winning_bid->bidder_id);

                /* Deposit money to seller's bank account */
                struct account *seller_acc = account_by_idnum(player_account_by_idnum(auc->owner_id));
                if (winning_bid->past_amount > 0) {
                    deposit_past_bank(seller_acc, winning_bid->past_amount);
                }
                if (winning_bid->future_amount > 0) {
                    deposit_future_bank(seller_acc, winning_bid->future_amount);
                }

                auction_mail(self, auc->owner_id, NULL,
                             "Congratulations!  %s sold for %s!  The money has been\r\n"
                             "deposited to your bank account.\r\n"
                             "Please frequent us again at %s!\r\n\r\n"
                             "- %s\r\n",
                             auc->item->name,
                             describe_bid_amount(winning_bid),
                             self->in_room->name,
                             GET_NAME(self));

                // Refund losing bids
                for (GList *it = auc->bids; it; it = it->next) {
                    struct bid_data *bid = it->data;
                    if (bid != winning_bid) {
                        refund_bid(bid);
                        auction_mail(self, bid->bidder_id, NULL,
                                     "Sorry, you lost the auction for %s.  Your money has been\r\n"
                                     "refunded to your bank account.\r\n"
                                     "Please patronize us again at %s!\r\n\r\n"
                                     "- %s\r\n",
                                     auc->item->name,
                                     self->in_room->name,
                                     GET_NAME(self));
                    }
                }

                auction_mail(self, winning_bid->bidder_id, auc->item,
                             "You have won %s at auction for %s!\r\n"
                             "Please visit us again at %s!\r\n\r\n"
                             "- %s\r\n",
                             auc->item->name,
                             describe_bid_amount(winning_bid),
                             self->in_room->name,
                             GET_NAME(self));

                auc->item = NULL;
                auctions = g_list_delete_link(auctions, ai);
                free_auction(auc);
                save_auctions();
            }
            break;
        default:
            /* Do nothing */
            break;
        }

        if (auc_str && *auc_str) {
            GET_MOOD(self) = moods[mood_index];
            do_gen_comm(self, auc_str, 0, SCMD_AUCTION);
            GET_MOOD(self) = NULL;
            auc_str = NULL;
        }
    }

    return 1;
}

int
auctioneer_stat(struct creature *self, struct creature *ch, char *argument)
{
    char *arg = tmp_getword(&argument);
    if (*arg) {
        return 0;
    }

    if (!auctions) {
        send_to_char(ch, "There are no items for auction.\r\n");
    }
    acc_string_clear();
    for (GList *ai = auctions; ai; ai = ai->next) {
        struct auction_data *auc = ai->data;
        acc_sprintf("%sItem Number:%s   %d\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->idnum);
        acc_sprintf("%sAuctioneer Room Id:%s   %ld\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->room_id);
        acc_sprintf("%sOwner:%s         %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    player_name_by_idnum(auc->owner_id));
        acc_sprintf("%sHigh Bidder:%s   %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    (auc->bids) ?
                    player_name_by_idnum(((struct bid_data *)auc->bids->data)->bidder_id) : "NULL");
        acc_sprintf("%sItem:%s          %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->item->name);
        acc_sprintf("%sStart Time:%s    %s",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    ctime(&auc->start_time));
        acc_sprintf("%sLast Bid:%s      %s",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    (auc->last_bid_time ?
                     ctime(&auc->last_bid_time) : "NULL\r\n"));
        acc_sprintf("%sStarting Bid:%s  %'" PRId64 " coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auc->start_bid);
        acc_sprintf("%sMinimum Bid:%s  %'" PRId64 " coins/cash\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), auction_minimum_bid(auc));
        acc_sprintf("Auction state:      %s\r\n", auction_state_strs[auc->state]);
        for (GList *it = auc->bids; it != NULL; it = it->next) {
            struct bid_data *bid = it->data;
            acc_sprintf("%s bid a total of %'" PRId64 "\r\n",
                        player_name_by_idnum(bid->bidder_id),
                        bid->past_amount + bid->future_amount);
        }

        if (ai->next) {
            acc_strcat("---------------------------------------\r\n", NULL);
        }
    }
    page_string(ch->desc, acc_get_string());
    return 1;
}

int
auctioneer_aucset(struct creature *self, struct creature *ch, char *argument)
{
    const char *aucset_commands[] = {
        "going_once",
        "going_twice",
        "sold_time",
        "nobid_thresh",
        "auction_thresh",
        "max_auc_value",
        "max_auctions",
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
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), MAX_AUCTIONS);
        acc_sprintf("%sMax Total Aucs:%s     %d\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), MAX_TOTAL_AUC);
        acc_sprintf("%sBid Increment:%s      %f\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), BID_INCREMENT);
        page_string(ch->desc, acc_get_string());
        return 1;
    } else if (is_abbrev(var, "help") || !*var || !*val) {
        // Print usage
        send_to_char(ch, "Usage: aucset <VAR> <VAL>\r\n\r\n"
                         "The following variables are accepted:\r\n"
                         "going_once\r\ngoing_twice\r\nsold_time\r\n"
                         "nobid_thresh\r\nauction_thresh\r\n"
                         "max_auc_value\r\nmax_auctions\r\n"
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

    switch (aucset_command) {
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
        MAX_AUCTIONS = (int)fval;
        break;
    case 7:
        MAX_TOTAL_AUC = (int)fval;
        break;
    case 8:
        BID_INCREMENT = fval;
        break;
    }

    save_auctions();
    return 1;
}

int
auctioneer_auction(struct creature *self, struct creature *ch, char *argument)
{
    char *auc_name = tmp_getword(&argument);
    long amount = atol(tmp_getword(&argument));

    if (GET_IDNUM(ch) < 0) {
        return 0;
    }

    if (!*auc_name) {
        perform_say_to(self, ch,
                       "I see you want to auction something, but WHAT?");
        return 1;
    }

    if (!amount) {
        perform_say_to(self, ch,
                       "And how much would you like to sell it for?");
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

    struct obj_data *obj =
        get_obj_in_list_all(ch, auc_name, ch->carrying);
    if (!obj) {
        perform_say_to(self, ch, "You don't even have that!  Stop "
                                 "wasting my time!");
        return 1;
    }

    short auc_count = 0;
    for (GList *ai = auctions; ai; ai = ai->next) {
        struct auction_data *auc = ai->data;
        if (auc->owner_id == GET_IDNUM(ch)) {
            auc_count++;
        }

        if ((auc_count >= MAX_AUCTIONS) && !IS_IMMORT(ch)) {
            perform_say_to(self, ch, "You already have too many items "
                                     "up for auction.");
            return 1;
        }
    }

    if (can_auction_obj(obj) && !IS_IMMORT(ch)) {
        perform_say_to(self, ch, "I run a respectable establishment "
                                 "here!  I don't deal in such trash!");
        return 1;
    }

    if (obj->shared->owner_id) {

        perform_say_to(self, ch,
                       tmp_sprintf("That item is bonded to %s.  I can't auction that!",
                                   (obj->shared->owner_id == GET_IDNUM(ch)) ?
                                   "you" : "someone"));
        return 1;
    }

    if (g_list_length(auctions) > MAX_TOTAL_AUC) {
        perform_say_to(self, ch, "There are too many items up for "
                                 "auction now.  Try again later.");
        return 1;
    }

    obj_from_char(obj);
    crashsave(ch);
    add_new_auction(self->in_room->number,
                    GET_IDNUM(ch),
                    amount,
                    obj);

    send_to_char(ch, "Your item has been entered for auction.\r\n");
    slog("AUCTION: %s (#%ld) has put up %s (#%d) for auction",
         GET_NAME(ch), GET_IDNUM(ch), obj->name, GET_OBJ_VNUM(obj));

    return 1;
}

int
auctioneer_withdraw(struct creature *self, struct creature *ch, char *argument)
{
    int idnum = atoi(tmp_getword(&argument));

    if (GET_IDNUM(ch) < 0) {
        return 0;
    }

    if (!idnum || idnum > MAX_TOTAL_AUC) {
        perform_say_to(self, ch, "Which item do you want to withdraw?");
        return 1;
    }

    struct auction_data *auc = auction_data_from_idnum(idnum);

    if (!auc) {
        perform_say_to(self, ch, "I don't see that item!  "
                                 "Maybe you should try another auctioneer.\r\n");
        return 1;
    }

    if (GET_IDNUM(ch) != auc->owner_id && !IS_IMMORT(ch)) {
        send_to_char(ch, "You can only withdraw your own item!\r\n");
        return 1;
    }

    if (auc->bids != NULL && !IS_IMMORT(ch)) {
        send_to_char(ch, "You cannot withdraw an item that has bids\r\n");
        return 1;
    }

    if (auc->item == NULL) {
        send_to_char(ch, "Something bad just happened. Please report it!");
        auctions = g_list_remove(auctions, auc);
        free(auc);
        return 1;
    }

    struct obj_data *obj = auc->item;
    GET_MOOD(self) = " sadly";
    do_gen_comm(self, tmp_sprintf("Item number %d, %s, withdrawn.",
                                  idnum, obj->name), 0, SCMD_AUCTION);
    GET_MOOD(self) = NULL;

    obj_to_char(obj, ch);
    auctions = g_list_remove(auctions, auc);
    crashsave(ch);
    save_auctions();

    send_to_char(ch, "Your item has been withdrawn from auction.\r\n");
    slog("AUCTION: %s (#%ld) has withdrawn %s (#%d)",
         GET_NAME(ch), GET_IDNUM(ch), obj->name, GET_OBJ_VNUM(obj));

    return 1;
}

SPECIAL(do_auctions)
{
    struct creature *self = (struct creature *)me;

    while (is_fighting(self)) {
        struct creature *dick = random_opponent(self);

        act("A ball of light streaks from $N's hand and hits you "
            "square in the chest, burning you to a cinder!", false,
            dick, NULL, self, TO_CHAR);
        act("A ball of light streaks from $n's hand and hits $N "
            "square in the chest, burning $M to a cinder!", false,
            self, NULL, dick, TO_NOTVICT);
        raw_kill(dick, self, TYPE_SLASH);
    }

    if (spec_mode == SPECIAL_TICK) {
        auctioneer_tick(self);
    }

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    if (CMD_IS("stun") || CMD_IS("steal") ||
        CMD_IS("pinch") || CMD_IS("glance")) {
        perform_stun(self, ch);
        return 1;
    }

    if (IS_NPC(ch)) {
        return 0;
    }

    // Handle commands in the presence of the auctioneer
    if (CMD_IS("stat") && is_named_role_member(ch, "WizardFull")) {
        auctioneer_stat(self, ch, argument);
    } else if (CMD_IS("aucset") && GET_LEVEL(ch) > LVL_ENERGY) {
        auctioneer_aucset(self, ch, argument);
    } else if (CMD_IS("auction")) {
        auctioneer_auction(self, ch, argument);
    } else if (CMD_IS("withdraw")) {
        auctioneer_withdraw(self, ch, argument);
    } else {
        // Command was not handled by auctioneer
        return 0;
    }

    // Command was handled
    return 1;
}
