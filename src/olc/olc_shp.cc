//
// File: olc_shp.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "security.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "specs.h"
#include "shop.h"

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct shop_data *shop_index;
extern int *shp_index;
extern int top_of_zone_table;
extern int olc_lock;
extern int top_shop;

long asciiflag_conv(char *buf);
extern char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
	arg3[MAX_INPUT_LENGTH];

char *one_argument_no_lower(char *argument, char *first_arg);
int search_block_no_lower(char *arg, char **list, bool exact);
int fill_word_no_lower(char *argument);
void num2str(char *str, int num);
void list_detailed_shop(struct char_data *ch, struct shop_data *shop);

const char *olc_sset_keys[] = {
	"addproduct",
	"delproduct",
	"buyprofit",
	"sellprofit",
	"addtype",
	"deltype",					/* 5 */
	"shop_dont_have",
	"char_dont_have",
	"shop_dont_buy",
	"shop_no_cash",
	"char_no_cash",				/* 10 */
	"message_buy",
	"message_sell",
	"temper",
	"flags",
	"keeper",					/* 15 */
	"tradewith",
	"addroom",
	"delroom",
	"open1",
	"close1",					/* 20 */
	"open2",
	"close2",
	"currency",
	"revenue",
	"\n"
};

#define NUM_SSET_COMMANDS 24

SPECIAL(shop_keeper);

struct shop_data *
do_create_shop(struct char_data *ch, int vnum)
{
	struct shop_data *new_shop = NULL, *tmp_shop = NULL;
	struct zone_data *zone = NULL;
	int found = 0;

	for (tmp_shop = shop_index; tmp_shop; tmp_shop = tmp_shop->next)
		if (SHOP_NUM(tmp_shop) == vnum) {
			found = 1;
			break;
		}

	if (found == 1) {
		send_to_char("ERROR: Shop already exists.\r\n", ch);
		return (NULL);
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum >= zone->number * 100 && vnum <= zone->top)
			break;

	if (!zone) {
		send_to_char("ERROR: A zone must be defined for the shop first.\r\n",
			ch);
		return NULL;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char("Try creating shops in your own zone, luser.\r\n", ch);
		sprintf(buf, "OLC: %s failed attempt to CREATE shop %d.",
			GET_NAME(ch), vnum);
		mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
		return NULL;
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_SHOPS_APPROVED)) {
		send_to_char("Shop OLC is not approved for this zone.\r\n", ch);
		return NULL;
	}

	CREATE(new_shop, struct shop_data, 1);

	SHOP_NUM(new_shop) = vnum;

	CREATE(new_shop->producing, int, 1);
	SHOP_PRODUCT(new_shop, 0) = -1;

	SHOP_BUYPROFIT(new_shop) = (float)2.0;
	SHOP_SELLPROFIT(new_shop) = (float)1.0;
	SHOP_REVENUE(new_shop) = 0;

	CREATE(new_shop->type, struct shop_buy_data, 1);
	SHOP_BUYTYPE(new_shop, 0) = (byte) - 1;

	new_shop->no_such_item1 =
		strdup("I don't have that...type LIST to see that I do have.");
	new_shop->no_such_item2 = strdup("You don't have that in your inventory.");
	new_shop->do_not_buy = strdup("I don't think I want that.");
	new_shop->missing_cash1 = strdup("I can't afford that.");
	new_shop->missing_cash2 =
		strdup("You don't have enough money to buy this, sorry.");
	new_shop->message_buy = strdup("Here you go.  That will be %d coins.");
	new_shop->message_sell = strdup("I'll give you %d coins for that.");

	SHOP_BROKE_TEMPER(new_shop) = 5;
	SHOP_BITVECTOR(new_shop) = 0;
	SHOP_KEEPER(new_shop) = -1;
	SHOP_TRADE_WITH(new_shop) = 0;

	CREATE(new_shop->in_room, int, 1);
	SHOP_ROOM(new_shop, 0) = -1;

	SHOP_OPEN1(new_shop) = 0;
	SHOP_CLOSE1(new_shop) = 28;
	SHOP_OPEN2(new_shop) = 0;
	SHOP_CLOSE2(new_shop) = 28;

	SHOP_BANK(new_shop) = 0;
	SHOP_SORT(new_shop) = 0;
	SHOP_FUNC(new_shop) = 0;
	top_shop++;

	new_shop->next = NULL;

	if (shop_index) {
		for (tmp_shop = shop_index; tmp_shop; tmp_shop = tmp_shop->next)
			if (!tmp_shop->next || (SHOP_NUM(tmp_shop) < vnum &&
					SHOP_NUM(tmp_shop->next) > vnum)) {
				new_shop->next = tmp_shop->next;
				tmp_shop->next = new_shop;
				break;
			}
	} else
		shop_index = new_shop;

	return (new_shop);
}


void
do_shop_sedit(struct char_data *ch, char *argument)
{
	struct shop_data *shop = NULL, *tmp_shop = NULL;
	struct zone_data *zone = NULL;
	struct descriptor_data *d = NULL;
	int j, found = 0;

	shop = GET_OLC_SHOP(ch);

	if (!*argument) {
		if (!shop)
			send_to_char("You are not currently editing a shop.\r\n", ch);
		else {
			sprintf(buf, "Current olc shop: [%5d]\r\n", SHOP_NUM(shop));
			send_to_char(buf, ch);
		}
		return;
	}
	if (!is_number(argument)) {
		if (is_abbrev(argument, "exit")) {
			send_to_char("Exiting shop editor.\r\n", ch);
			GET_OLC_SHOP(ch) = NULL;
			return;
		}
		send_to_char("The argument must be a number.\r\n", ch);
		return;
	} else {
		j = atoi(argument);

		tmp_shop = shop_index;

		do {
			if (SHOP_NUM(tmp_shop) == j) {
				found = 1;
				break;
			} else
				tmp_shop = tmp_shop->next;
		}
		while (tmp_shop && found != 1);

		if (found == 0)
			send_to_char("There is no such shop.\r\n", ch);
		else {
			for (zone = zone_table; zone; zone = zone->next)
				if (j < zone->top)
					break;
			if (!zone) {
				send_to_char("That shop does not belong to any zone!!\r\n",
					ch);
				slog("SYSERR: shop not in any zone.");
				return;
			}

			if (!CAN_EDIT_ZONE(ch, zone)) {
				send_to_char
					("You do not have permission to edit this shop.\r\n", ch);
				return;
			}
			if (!OLC_EDIT_OK(ch, zone, ZONE_SHOPS_APPROVED)) {
				send_to_char("Shop OLC is not approved for this zone.\r\n",
					ch);
				return;
			}

			for (d = descriptor_list; d; d = d->next) {
				if (d->character && GET_OLC_SHOP(d->character) == tmp_shop) {
					act("$N is already editing that shop.", FALSE, ch, 0,
						d->character, TO_CHAR);
					return;
				}
			}

			GET_OLC_SHOP(ch) = tmp_shop;
			sprintf(buf, "Now editing shop [%d]\r\n", SHOP_NUM(tmp_shop));
			send_to_char(buf, ch);
		}
	}
}

void
do_shop_sstat(struct char_data *ch)
{
	struct shop_data *shop = NULL;

	shop = GET_OLC_SHOP(ch);

	if (!shop)
		send_to_char("You are not currently editing a shop.\r\n", ch);
	else
		list_detailed_shop(ch, shop);
}

#define shop_p GET_OLC_SHOP(ch)

void
do_shop_sset(struct char_data *ch, char *argument)
{
	struct char_data *mob = NULL, *mob2 = NULL;
	struct obj_data *obj;
	struct zone_data *zone;
	struct room_data *room;
	struct shop_buy_data *new_types;
	int i, j, found = 0, sset_command, tmp_flags, flag, cur_flags, state;
	int *new_products, count = 0, type, *new_rooms;
	float profit;

	if (!shop_p) {
		send_to_char("You are not currently editing a shop.\r\n", ch);
		return;
	}

	if (!*argument) {
		strcpy(buf, "Valid sset commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (i < NUM_SSET_COMMANDS) {
			strcat(buf, olc_sset_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf, 1);
		return;
	}

	half_chop(argument, arg1, arg2);
	skip_spaces(&argument);

	if ((sset_command = search_block(arg1, olc_sset_keys, FALSE)) < 0) {
		sprintf(buf, "Invalid sset command '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}

	if (!*arg2) {
		sprintf(buf, "Set %s to what??\r\n", olc_sset_keys[sset_command]);
		send_to_char(buf, ch);
		return;
	}

	switch (sset_command) {
	case 0:					/* addproduct */
		if (is_number(arg2)) {
			i = atoi(arg2);
			obj = real_object_proto(i);

			if (!obj) {
				send_to_char("That object does not exist, buttmunch.\r\n", ch);
				return;
			}

			for (i = 0;; i++)
				if (SHOP_PRODUCT(shop_p, i) != -1)
					count++;
				else
					break;

			CREATE(new_products, int, count + 2);

			for (i = 0; i <= count; i++)
				if (SHOP_PRODUCT(shop_p, i) != -1)
					new_products[i] = SHOP_PRODUCT(shop_p, i);
				else
					break;

			new_products[count] = GET_OBJ_VNUM(obj);
			new_products[count + 1] = -1;
			free(shop_p->producing);
			shop_p->producing = new_products;

			send_to_char("Product added to shopping list.\r\n", ch);
		} else
			send_to_char("Usage: olc sset addproduct <obj vnum>\r\n", ch);
		break;
	case 1:					/* delproduct */
		if (is_number(arg2)) {
			i = atoi(arg2);

			for (j = 0;; j++) {
				if (SHOP_PRODUCT(shop_p, j) == -1)
					break;
				else if (SHOP_PRODUCT(shop_p, j) == i) {
					found = 1;
					SHOP_PRODUCT(shop_p, j) = -999;
				}
				count++;
			}

			if (found == 1) {
				CREATE(new_products, int, count);
				for (j = 0, count = 0;; j++) {
					if (SHOP_PRODUCT(shop_p, j) != -999) {
						new_products[count] = SHOP_PRODUCT(shop_p, j);
						count++;
					}
					if (SHOP_PRODUCT(shop_p, j) == -1)
						break;
				}

				free(shop_p->producing);
				shop_p->producing = new_products;
				send_to_char("Product deleted from shopping list.\r\n", ch);
			} else
				send_to_char("Could not find product on this shop's list.\r\n",
					ch);
		} else
			send_to_char("Usage: olc sset delproduct <obj vnum>\r\n", ch);
		break;
	case 2:					/* buyprofit */
		profit = atof(arg2);
		if (profit < (float)0.00 || profit > (float)5.00)
			send_to_char("Buying profit must be between 0.00 and 5.00.\r\n",
				ch);
		else {
			SHOP_BUYPROFIT(shop_p) = profit;
			send_to_char("Shop buying profit set.\r\n", ch);
		}
		break;
	case 3:					/* sellprofit */
		profit = atof(arg2);
		if (profit < (float)0.00 || profit > (float)5.00)
			send_to_char("Selling profit must be between 0.00 and 5.00.\r\n",
				ch);
		else {
			SHOP_SELLPROFIT(shop_p) = profit;
			send_to_char("Shop selling profit set.\r\n", ch);
		}
		break;
	case 4:					/* addtype */
		if ((type = search_block(arg2, item_types, FALSE)) < 0) {
			sprintf(buf, "Invalid product type '%s'.\r\n", arg2);
			send_to_char(buf, ch);
			return;
		}

		for (i = 0;; i++)
			if (SHOP_BUYTYPE(shop_p, i) != -1)
				count++;
			else
				break;

		CREATE(new_types, struct shop_buy_data, count + 2);

		for (i = 0; i <= count; i++)
			if (SHOP_BUYTYPE(shop_p, i) != -1)
				new_types[i].type = SHOP_BUYTYPE(shop_p, i);
			else
				break;

		new_types[count].type = type;
		new_types[count + 1].type = -1;

		free(shop_p->type);
		shop_p->type = new_types;

		send_to_char("Product type added to list.\r\n", ch);
		break;
	case 5:					/* deltype */
		if ((type = search_block(arg2, item_types, FALSE)) < 0) {
			sprintf(buf, "Invalid product type '%s'.\r\n", arg2);
			send_to_char(buf, ch);
			return;
		}

		for (j = 0;; j++) {
			if (SHOP_BUYTYPE(shop_p, j) == -1)
				break;
			else if (SHOP_BUYTYPE(shop_p, j) == type) {
				found = 1;
				SHOP_BUYTYPE(shop_p, j) = -999;
			}
			count++;
		}

		if (found == 1) {
			CREATE(new_types, struct shop_buy_data, count);
			for (j = 0, count = 0;; j++) {
				if (SHOP_BUYTYPE(shop_p, j) != -999) {
					new_types[count].type = SHOP_BUYTYPE(shop_p, j);
					count++;
				}
				if (SHOP_BUYTYPE(shop_p, j) == -1)
					break;
			}

			free(shop_p->type);
			shop_p->type = new_types;
			send_to_char("Product type deleted from list.\r\n", ch);
		} else
			send_to_char("Could not find product type on this list.\r\n", ch);
		break;
	case 6:					/* msg_no_item1 */
		if (shop_p->no_such_item1)
			free(shop_p->no_such_item1);
		shop_p->no_such_item1 = strdup(arg2);
		send_to_char("Shop message no such item1 set.\r\n", ch);
		break;
	case 7:					/* msg_no_item2 */
		if (shop_p->no_such_item2)
			free(shop_p->no_such_item2);
		shop_p->no_such_item2 = strdup(arg2);
		send_to_char("Shop message no such item2 set.\r\n", ch);
		break;
	case 8:					/* msg_no_buy */
		if (shop_p->do_not_buy)
			free(shop_p->do_not_buy);
		shop_p->do_not_buy = strdup(arg2);
		send_to_char("Shop message do not buy set.\r\n", ch);
		break;
	case 9:					/* msg_miss_cash1 */
		if (shop_p->missing_cash1)
			free(shop_p->missing_cash1);
		shop_p->missing_cash1 = strdup(arg2);
		send_to_char("Shop message missing cash1 set.\r\n", ch);
		break;
	case 10:					/* msg_miss_cash2 */
		if (shop_p->missing_cash2)
			free(shop_p->missing_cash2);
		shop_p->missing_cash2 = strdup(arg2);
		send_to_char("Shop message missing cash2 set.\r\n", ch);
		break;
	case 11:					/* msg_buy */

		if (!shop_check_message_format(arg2)) {
			send_to_char
				("Shop message format invalid.  Must contain one and only one %d.\r\n",
				ch);
			return;
		}

		if (shop_p->message_buy)
			free(shop_p->message_buy);
		shop_p->message_buy = strdup(arg2);
		send_to_char("Shop message buy set.\r\n", ch);
		break;
	case 12:					/* msg_sell */

		if (!shop_check_message_format(arg2)) {
			send_to_char
				("Shop message format invalid.  Must contain one and only one %d.\r\n",
				ch);
			return;
		}

		if (shop_p->message_sell)
			free(shop_p->message_sell);
		shop_p->message_sell = strdup(arg2);
		send_to_char("Shop message sell set.\r\n", ch);
		break;
	case 13:					/* temper */
		i = atoi(arg2);
		if (i < 0 || i > 5)
			send_to_char("Temper must be between 0 and 5.\r\n", ch);
		else {
			SHOP_BROKE_TEMPER(shop_p) = i;
			send_to_char("Temper set.\r\n", ch);
		}
		break;
	case 14:					/* flags */
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		for (zone = zone_table; zone; zone = zone->next)
			if (SHOP_NUM(shop_p) >= zone->number * 100 &&
				SHOP_NUM(shop_p) <= zone->top)
				break;

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char("Usage: olc sset flags [+/-] [FLAG, FLAG, ...]\r\n",
				ch);
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = SHOP_BITVECTOR(shop_p);

		while (*arg1) {
			if ((flag = search_block(arg1, shop_bits, FALSE)) == -1) {
				sprintf(buf, "Invalid flag %s, skipping...\r\n", arg1);
				send_to_char(buf, ch);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		SHOP_BITVECTOR(shop_p) = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char("Shop flags set\r\n", ch);
		} else if (tmp_flags == 0)
			send_to_char("Shop flags not altered.\r\n", ch);
		else {
			send_to_char("Shop flags set.\r\n", ch);
		}
		break;
	case 15:					/* keeper */
		i = atoi(arg2);
		mob = real_mobile_proto(i);
		if (!mob)
			send_to_char("Could not find a mobile with that number.\r\n", ch);
		else {
			if ((mob2 = real_mobile_proto(SHOP_KEEPER(shop_p)))) {
				mob2->mob_specials.shared->func = SHOP_FUNC(shop_p);
				SHOP_FUNC(shop_p) = NULL;
			}
			SHOP_KEEPER(shop_p) = GET_MOB_VNUM(mob);
			if (mob->mob_specials.shared->func != shop_keeper)
				SHOP_FUNC(shop_p) = mob->mob_specials.shared->func;
			else
				SHOP_FUNC(shop_p) = NULL;
			mob->mob_specials.shared->func = shop_keeper;
			send_to_char("Shop keeper set.\r\n", ch);
		}
		break;
	case 16:					/* tradewith */
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		for (zone = zone_table; zone; zone = zone->next)
			if (SHOP_NUM(shop_p) >= zone->number * 100 &&
				SHOP_NUM(shop_p) <= zone->top)
				break;

		if (*arg1 == '+')
			state = 2;
		else if (*arg1 == '-')
			state = 1;
		else {
			send_to_char("Usage: olc sset tradewith [+/-] [WHO, WHO, ...]\r\n",
				ch);
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = SHOP_TRADE_WITH(shop_p);

		while (*arg1) {
			if ((flag = search_block(arg1, trade_letters, FALSE)) == -1) {
				sprintf(buf, "Invalid type %s, skipping...\r\n", arg1);
				send_to_char(buf, ch);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		SHOP_TRADE_WITH(shop_p) = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char("Shop tradewith set\r\n", ch);
		} else if (tmp_flags == 0)
			send_to_char("Shop tradewith not altered.\r\n", ch);
		else {
			send_to_char("Shop tradewith set.\r\n", ch);
		}
		break;
	case 17:					/* addroom */
		if (is_number(arg2)) {
			i = atoi(arg2);
			room = real_room(i);

			if (!room) {
				send_to_char("That room does not exist, buttmunch.\r\n", ch);
				return;
			}

			for (i = 0;; i++)
				if (SHOP_ROOM(shop_p, i) != -1)
					count++;
				else
					break;

			CREATE(new_rooms, int, count + 2);

			for (i = 0; i <= count; i++)
				if (SHOP_ROOM(shop_p, i) != -1)
					new_rooms[i] = SHOP_ROOM(shop_p, i);
				else
					break;

			new_rooms[count] = room->number;
			new_rooms[count + 1] = -1;

			free(shop_p->in_room);
			shop_p->in_room = new_rooms;

			send_to_char("Room added to room list.\r\n", ch);
		} else
			send_to_char("Usage: olc sset addroom <room vnum>\r\n", ch);
		break;
	case 18:					/* delroom */
		if (is_number(arg2)) {
			i = atoi(arg2);

			for (j = 0;; j++) {
				if (SHOP_ROOM(shop_p, j) == -1)
					break;
				else if (SHOP_ROOM(shop_p, j) == i) {
					found = 1;
					SHOP_ROOM(shop_p, j) = -999;
				}
				count++;
			}

			if (found == 1) {
				CREATE(new_rooms, int, count);
				for (j = 0, count = 0;; j++) {
					if (SHOP_ROOM(shop_p, j) != -999) {
						new_rooms[count] = SHOP_ROOM(shop_p, j);
						count++;
					}
					if (SHOP_ROOM(shop_p, j) == -1)
						break;
				}

				free(shop_p->in_room);
				shop_p->in_room = new_rooms;
				send_to_char("Room deleted from room list.\r\n", ch);
			} else
				send_to_char("Could not find room in this shop's list.\r\n",
					ch);
		} else
			send_to_char("Usage: olc sset delroom <room vnum>\r\n", ch);
		break;
	case 19:					/* open1 */
		i = atoi(arg2);
		if (i < 0 || i > 28)
			send_to_char("Shop open hours must be between 0 and 28.\r\n", ch);
		else {
			SHOP_OPEN1(shop_p) = i;
			send_to_char("Shop open1 hour set.\r\n", ch);
		}
		break;


	case 20:					/* closed1 */
		i = atoi(arg2);
		if (i < 0 || i > 28)
			send_to_char("Shop closed hours must be between 0 and 28.\r\n",
				ch);
		else {
			SHOP_CLOSE1(shop_p) = i;
			send_to_char("Shop closed1 hour set.\r\n", ch);
		}
		break;
	case 21:					/* open2 */
		i = atoi(arg2);
		if (i < 0 || i > 28)
			send_to_char("Shop open hours must be between 0 and 28.\r\n", ch);
		else {
			SHOP_OPEN2(shop_p) = i;
			send_to_char("Shop open2 hour set.\r\n", ch);
		}
		break;

	case 22:					/* closed2 */
		i = atoi(arg2);
		if (i < 0 || i > 28)
			send_to_char("Shop closed hours must be between 0 and 28.\r\n",
				ch);
		else {
			SHOP_CLOSE2(shop_p) = i;
			send_to_char("Shop closed2 hour set.\r\n", ch);
		}
		break;
	case 23:					/* currency */
		if (is_abbrev(arg2, "gold"))
			SHOP_CURRENCY(shop_p) = CURRENCY_GOLD;
		else if (is_abbrev(arg2, "credits"))
			SHOP_CURRENCY(shop_p) = CURRENCY_CREDITS;
		else {
			send_to_char("Argument must be 'gold' or 'credits'.\r\n", ch);
			break;
		}
		send_to_char("Shop currency set.\r\n", ch);
		break;
	case 24:					// revenue
		i = atoi(arg2);
		if (i < 0) {
			send_to_char("This value cannot be a negative number.\r\n", ch);
		} else {
			SHOP_REVENUE(shop_p) = i;
			send_to_char("Shop revenue set.\r\n", ch);
		}
	}
}


int
write_shp_index(struct char_data *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; shp_index[i] != -1; i++) {
		count++;
		if (shp_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (shp_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (shp_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = shp_index[i];
			done = 1;
		} else
			new_index[j] = shp_index[i];
		j++;
	}

	free(shp_index);
	shp_index = new_index;

	sprintf(fname, "world/shp/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char("Could not open index file, shop save aborted.\r\n", ch);
		return (0);
	}

	for (i = 0; shp_index[i] != -1; i++)
		fprintf(index, "%d.shp\n", shp_index[i]);

	fprintf(index, "$\n");

	send_to_char("Shop index file re-written.\r\n", ch);

	fclose(index);

	return (1);
}



int
save_shops(struct char_data *ch)
{
	unsigned int i, tmp;
	int s_vnum;
	room_num low = 0;
	room_num high = 0;
	char fname[64];
	int found = 0;
	struct zone_data *zone;
	struct shop_data *shop;
	FILE *file;
	FILE *realfile;

	if (GET_OLC_SHOP(ch)) {
		shop = GET_OLC_SHOP(ch);
		s_vnum = SHOP_NUM(shop);
		for (zone = zone_table; zone; zone = zone->next)
			if (s_vnum >= zone->number * 100 && s_vnum <= zone->top)
				break;
		if (!zone) {
			slog("OLC: ERROR finding zone for shop %d.", s_vnum);
			send_to_char("Unable to match shop with zone error..\r\n", ch);
			return 1;
		}
	} else
		zone = ch->in_room->zone;

	for (low = zone->number * 100, shop = shop_index;
		shop && shop->vnum <= zone->top; shop = shop->next) {
		if (shop->vnum >= low) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		return 0;
	}
	sprintf(fname, "world/shp/%d.shp", zone->number);
	if ((access(fname, F_OK) >= 0) && (access(fname, W_OK) < 0)) {
		sprintf(buf, "OLC: ERROR - Main shop file for zone %d is read-only.",
			ch->in_room->zone->number);
		mudlog(buf, BRF, 0, TRUE);
	}


	sprintf(fname, "world/shp/olc/%d.shp", zone->number);
	if (!(file = fopen(fname, "w+"))) {
		send_to_char("Unable to open shop file.\r\n", ch);
		return 1;
	}
	if ((write_shp_index(ch, zone)) != 1) {
		send_to_char("write_shop_index() failed.\r\n", ch);
		return (1);
	}

	low = zone->number * 100;
	high = zone->top;

	fprintf(file, "CircleMUD v3.0pl1 Shop File~\n");

	for (shop = shop_index; shop; shop = shop->next) {
		if (SHOP_NUM(shop) < low)
			continue;
		if (SHOP_NUM(shop) > high)
			break;

		fprintf(file, "#%d~\n", SHOP_NUM(shop));

		for (i = 0;; i++) {
			fprintf(file, "%d\n", SHOP_PRODUCT(shop, i));
			if (SHOP_PRODUCT(shop, i) == -1)
				break;
		}

		fprintf(file, "%.2f\n", SHOP_BUYPROFIT(shop));

		fprintf(file, "%.2f\n", SHOP_SELLPROFIT(shop));

		for (i = 0;; i++) {
			fprintf(file, "%d\n", SHOP_BUYTYPE(shop, i));
			if (SHOP_BUYTYPE(shop, i) == -1)
				break;
		}

		fprintf(file, "%s~\n", shop->no_such_item1);
		fprintf(file, "%s~\n", shop->no_such_item2);
		fprintf(file, "%s~\n", shop->do_not_buy);
		fprintf(file, "%s~\n", shop->missing_cash1);
		fprintf(file, "%s~\n", shop->missing_cash2);
		fprintf(file, "%s~\n", shop->message_buy);
		fprintf(file, "%s~\n", shop->message_sell);

		fprintf(file, "%d\n", SHOP_BROKE_TEMPER(shop));
		fprintf(file, "%d\n", SHOP_BITVECTOR(shop));
		fprintf(file, "%d\n", SHOP_KEEPER(shop));

		fprintf(file, "%d\n", SHOP_TRADE_WITH(shop));

		for (i = 0;; i++) {
			fprintf(file, "%d\n", SHOP_ROOM(shop, i));
			if (SHOP_ROOM(shop, i) == -1)
				break;
		}

		fprintf(file, "%d\n", SHOP_OPEN1(shop));
		fprintf(file, "%d\n", SHOP_CLOSE1(shop));
		fprintf(file, "%d\n", SHOP_OPEN2(shop));
		fprintf(file, "%d\n", SHOP_CLOSE2(shop));
		fprintf(file, "%d\n", SHOP_CURRENCY(shop));
		fprintf(file, "%d\n", SHOP_REVENUE(shop));
	}

	fprintf(file, "$~\n");

	sprintf(fname, "world/shp/%d.shp", zone->number);
	realfile = fopen(fname, "w");
	if (realfile) {
		fclose(file);
		sprintf(fname, "world/shp/olc/%d.shp", zone->number);
		if (!(file = fopen(fname, "r"))) {
			slog("SYSERR: Failure to reopen olc shop file.");
			send_to_char
				("OLC Error: Failure to duplicate shop file in main dir."
				"\r\n", ch);
			fclose(realfile);
			return 0;
		}
		do {
			tmp = fread(buf, 1, 512, file);
			if (fwrite(buf, 1, tmp, realfile) != tmp) {
				slog("SYSERR: Failure to duplicate olc shop file in the main wld dir.");
				send_to_char
					("OLC Error: Failure to duplicate shop file in main dir."
					"\r\n", ch);
				fclose(realfile);
				fclose(file);
				return 0;
			}
		} while (tmp == 512);

		fclose(realfile);
		fclose(file);

		return 0;
	}
	return 0;
}


int
do_destroy_shop(struct char_data *ch, int vnum)
{

	struct zone_data *zone = NULL;
	struct shop_data *shop = NULL, *tmp_shop = NULL;
	struct char_data *mob = NULL;
	struct descriptor_data *d = NULL;
	int found = 0;

	shop = shop_index;

	do {
		if (SHOP_NUM(shop) == vnum) {
			found = 1;
			break;
		} else
			shop = shop->next;
	}
	while (shop && found != 1);

	if (found == 0) {
		send_to_char("ERROR: That shop does not exist.\r\n", ch);
		return 1;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum < zone->top)
			break;

	if (!zone) {
		send_to_char("That shop does not belong to any zone!!\r\n", ch);
		slog("SYSERR: shop not in any zone.");
		return 1;
	}

	if (GET_IDNUM(ch) != zone->owner_idnum && GET_LEVEL(ch) < LVL_LUCIFER) {
		send_to_char("Oh, no you dont!!!\r\n", ch);
		sprintf(buf, "OLC: %s failed attempt to DESTROY shop %d.",
			GET_NAME(ch), SHOP_NUM(shop));
		mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
		return 1;
	}

	if ((mob = real_mobile_proto(SHOP_KEEPER(shop))))
		mob->mob_specials.shared->func = SHOP_FUNC(shop);

	for (tmp_shop = shop_index; tmp_shop; tmp_shop = tmp_shop->next) {
		if (tmp_shop == shop) {
			shop_index = shop->next;
			break;
		}
		if (tmp_shop->next == shop) {
			tmp_shop->next = shop->next;
			break;
		}
	}

	for (d = descriptor_list; d; d = d->next)
		if (d->character && GET_OLC_SHOP(d->character) == shop) {
			GET_OLC_SHOP(d->character) = NULL;
			send_to_char("The shop you were editing has been destroyed!\r\n",
				d->character);
			break;
		}


	if (shop->producing)
		free(shop->producing);
	if (shop->no_such_item1)
		free(shop->no_such_item1);
	if (shop->no_such_item2)
		free(shop->no_such_item2);
	if (shop->missing_cash1)
		free(shop->missing_cash1);
	if (shop->missing_cash2)
		free(shop->missing_cash2);
	if (shop->do_not_buy)
		free(shop->do_not_buy);
	if (shop->message_buy)
		free(shop->message_buy);
	if (shop->message_sell)
		free(shop->message_sell);
	if (shop->in_room)
		free(shop->in_room);

	top_shop--;
	free(shop);

	return 0;
}
