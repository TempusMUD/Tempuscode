/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

//
// File: shop.c                    -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "utils.h"
#include "shop.h"
#include "screen.h"
#include "fight.h"

/* External variables */
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct time_info_data time_info;

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(struct char_data *keeper, struct shop_data *shop);
void perform_tell(struct char_data *ch, struct char_data *vict, char *buf);
void set_local_time(struct zone_data *zone, struct time_info_data *local_time);

/* Local variables */
struct shop_data *shop_index;
int top_shop = 0;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke, cmd_fart, cmd_spit;

//
// returns true if the message format is ok
// to be correct, the message format must contain one
// and only one printf format code, and it must be %d
//

bool
shop_check_message_format(char *format_buf)
{

	bool format_code_found = false;

	for (char *ptr = format_buf; *ptr; ++ptr) {
		if (*ptr == '%') {

			// if we've already found a %d, then another % in the string is an error
			if (format_code_found) {
				return false;
			}
			// we've found the first %d in the string
			if (*(++ptr) == 'd') {
				format_code_found = true;
			}
			// we got a %<something else>, an error
			else {
				return false;
			}
		}
	}

	return format_code_found;
}

int
is_ok_char(struct char_data *keeper, struct char_data *ch,
	struct shop_data *shop)
{
	char buf[200];

	if (IS_GOD(ch))
		return (TRUE);

	if (!CAN_SEE(keeper, ch)) {
		do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
		return (FALSE);
	}

	if ((IS_GOOD(ch) && NOTRADE_GOOD(shop)) ||
		(IS_EVIL(ch) && NOTRADE_EVIL(shop)) ||
		(IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop)) ||
		PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
		strcpy(buf, MSG_NO_SELL_ALIGN);
		perform_tell(keeper, ch, buf);
		return (FALSE);
	}
	if (IS_NPC(ch))
		return (TRUE);

	if ((IS_HUMAN(ch) && NOTRADE(shop, TRADE_NOHUMAN)) ||
		(IS_ELF(ch) && NOTRADE(shop, TRADE_NOELF)) ||
		(IS_DWARF(ch) && NOTRADE(shop, TRADE_NODWARF)) ||
		(IS_HALF_ORC(ch) && NOTRADE(shop, TRADE_NOHALF_ORC)) ||
		(IS_TABAXI(ch) && NOTRADE(shop, TRADE_NOTABAXI)) ||
		(IS_MINOTAUR(ch) && NOTRADE(shop, TRADE_NOMINOTAUR)) ||
		(IS_ORC(ch) && NOTRADE(shop, TRADE_NOORC))) {
		strcpy(buf, MSG_NO_SELL_RACE);
		perform_tell(keeper, ch, buf);
		return (FALSE);
	}

	if ((GET_CLASS(ch) == CLASS_MAGIC_USER && NOTRADE_MAGIC_USER(shop)) ||
		(GET_CLASS(ch) == CLASS_CLERIC && NOTRADE_CLERIC(shop)) ||
		(GET_CLASS(ch) == CLASS_THIEF && NOTRADE_THIEF(shop)) ||
		(GET_CLASS(ch) == CLASS_RANGER && NOTRADE(shop, TRADE_NORANGER)) ||
		(GET_CLASS(ch) == CLASS_MONK && NOTRADE(shop, TRADE_NOMONK)) ||
		(GET_CLASS(ch) == CLASS_MERCENARY && NOTRADE(shop, TRADE_NOMERC)) ||
		(GET_CLASS(ch) == CLASS_BARB && NOTRADE(shop, TRADE_NOBARB)) ||
		(GET_CLASS(ch) == CLASS_KNIGHT && NOTRADE(shop, TRADE_NOKNIGHT)) ||
		(GET_CLASS(ch) == CLASS_HOOD && NOTRADE(shop, TRADE_NOHOOD)) ||
		(GET_CLASS(ch) == CLASS_PHYSIC && NOTRADE(shop, TRADE_NOPHYSIC)) ||
		(GET_CLASS(ch) == CLASS_PSIONIC && NOTRADE(shop, TRADE_NOPSIONIC)) ||
		(GET_CLASS(ch) == CLASS_CYBORG && NOTRADE(shop, TRADE_NOCYBORG)) ||
		(GET_CLASS(ch) == CLASS_VAMPIRE && NOTRADE(shop, TRADE_NOVAMPIRE)) ||
		(GET_CLASS(ch) == CLASS_SPARE1 && NOTRADE(shop, TRADE_NOSPARE1)) ||
		(GET_CLASS(ch) == CLASS_SPARE2 && NOTRADE(shop, TRADE_NOSPARE2)) ||
		(GET_CLASS(ch) == CLASS_SPARE3 && NOTRADE(shop, TRADE_NOSPARE3)) ||
		(GET_CLASS(ch) == CLASS_WARRIOR && NOTRADE_WARRIOR(shop))) {
		strcpy(buf, MSG_NO_SELL_CLASS);
		perform_tell(keeper, ch, buf);
		return (FALSE);
	}
	return (TRUE);
}


int
is_open(struct char_data *keeper, struct shop_data *shop, int msg)
{
	char buf[200];
	struct time_info_data local_time;

	set_local_time(keeper->in_room->zone, &local_time);

	*buf = 0;
	if (SHOP_OPEN1(shop) > local_time.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (SHOP_CLOSE1(shop) < local_time.hours)
		if (SHOP_OPEN2(shop) > local_time.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (SHOP_CLOSE2(shop) < local_time.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);

	if (!(*buf))
		return (TRUE);
	if (msg)
		do_say(keeper, buf, cmd_tell, 0);
	return (FALSE);
}


int
is_ok(struct char_data *keeper, struct char_data *ch, struct shop_data *shop)
{
	if (is_open(keeper, shop, TRUE))
		return (is_ok_char(keeper, ch, shop));
	else
		return (FALSE);
}


void
push(struct stack_data *stack, int pushval)
{
	S_DATA(stack, S_LEN(stack)++) = pushval;
}


int
top(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, S_LEN(stack) - 1));
	else
		return (NOTHING);
}


int
pop(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, --S_LEN(stack)));
	else {
		slog("Illegal expression in shop keyword list");
		return (0);
	}
}


void
evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
	int oper;

	if ((oper = pop(ops)) == OPER_NOT)
		push(vals, !pop(vals));
	else if (oper == OPER_AND)
		push(vals, pop(vals) && pop(vals));
	else if (oper == OPER_OR)
		push(vals, pop(vals) || pop(vals));
}


int
find_oper_num(char token)
{
	int index;

	for (index = 0; index <= MAX_OPER; index++)
		if (strchr(operator_str[index], token))
			return (index);
	return (NOTHING);
}


int
evaluate_expression(struct obj_data *obj, char *expr)
{
	struct stack_data ops, vals;
	char *ptr, *end, name[200];
	int temp, index;

	if (!expr)
		return (TRUE);

	ops.len = vals.len = 0;
	ptr = expr;
	while (*ptr) {
		if (isspace(*ptr))
			ptr++;
		else {
			if ((temp = find_oper_num(*ptr)) == NOTHING) {
				end = ptr;
				while (*ptr && !isspace(*ptr)
					&& (find_oper_num(*ptr) == NOTHING))
					ptr++;
				strncpy(name, end, ptr - end);
				name[ptr - end] = 0;
				for (index = 0; *extra_bits[index] != '\n'; index++)
					if (!strcmp(name, extra_bits[index])) {
						push(&vals, IS_SET(GET_OBJ_EXTRA(obj), 1 << index));
						break;
					}
				if (*extra_bits[index] == '\n')
					push(&vals, isname(name, obj->name));
			} else {
				if (temp != OPER_OPEN_PAREN)
					while (top(&ops) > temp)
						evaluate_operation(&ops, &vals);

				if (temp == OPER_CLOSE_PAREN) {
					if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
						slog("Illegal parenthesis in shop keyword expression");
						return (FALSE);
					}
				} else
					push(&ops, temp);
				ptr++;
			}
		}
	}
	while (top(&ops) != NOTHING)
		evaluate_operation(&ops, &vals);
	temp = pop(&vals);
	if (top(&vals) != NOTHING) {
		slog("Extra operands left on shop keyword expression stack");
		return (FALSE);
	}
	return (temp);
}


int
trade_with(struct obj_data *item, struct shop_data *shop)
{
	int counter;

	if (GET_OBJ_COST(item) < 1)
		return (OBJECT_NOTOK);

	if (IS_OBJ_STAT(item, ITEM_NOSELL) || !OBJ_APPROVED(item))
		return (OBJECT_NOSELL);

	if (IS_OBJ_STAT2(item, ITEM2_BROKEN))
		return (OBJECT_BROKEN);

	for (counter = 0; SHOP_BUYTYPE(shop, counter) != NOTHING; counter++)
		if (SHOP_BUYTYPE(shop, counter) == GET_OBJ_TYPE(item))
			if ((GET_OBJ_VAL(item, 2) == 0) &&
				((GET_OBJ_TYPE(item) == ITEM_WAND) ||
					(GET_OBJ_TYPE(item) == ITEM_STAFF)))
				return (OBJECT_DEAD);
			else if (evaluate_expression(item, SHOP_BUYWORD(shop, counter)))
				return (OBJECT_OK);

	return (OBJECT_NOTOK);
}


int
same_obj(struct obj_data *obj1, struct obj_data *obj2)
{
	int index;

	if (!obj1 || !obj2)
		return (obj1 == obj2);

	if (GET_OBJ_VNUM(obj1) != GET_OBJ_VNUM(obj2))
		return (FALSE);

	if (GET_OBJ_SIGIL_IDNUM(obj1) != GET_OBJ_SIGIL_IDNUM(obj2) ||
		GET_OBJ_SIGIL_LEVEL(obj1) != GET_OBJ_SIGIL_LEVEL(obj2))
		return FALSE;

	if ((obj1->shared->proto &&
			(obj1->short_description != obj1->shared->proto->short_description
				|| obj1->description != obj1->shared->proto->description))
		|| (obj2->shared->proto
			&& (obj2->short_description !=
				obj2->shared->proto->short_description
				|| obj2->description != obj2->shared->proto->description)))
		return FALSE;

	if ((obj1->short_description != obj2->short_description ||
			obj1->description != obj2->description) &&
		(str_cmp(obj1->short_description, obj2->short_description) ||
			!obj1->description || !obj2->description ||
			str_cmp(obj1->description, obj2->description)))
		return (FALSE);

	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2) ||
		GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2) ||
		GET_OBJ_EXTRA2(obj1) != GET_OBJ_EXTRA2(obj2))
		return (FALSE);

	for (index = 0; index < MAX_OBJ_AFFECT; index++)
		if ((obj1->affected[index].location != obj2->affected[index].location)
			|| (obj1->affected[index].modifier !=
				obj2->affected[index].modifier))
			return (FALSE);

	if (obj1->getWeight() != obj2->getWeight())
		return FALSE;

	return (TRUE);
}


int
shop_producing(struct obj_data *item, struct shop_data *shop)
{
	int counter;
	struct obj_data *obj;

	if (GET_OBJ_VNUM(item) < 0)
		return (FALSE);

	for (counter = 0; SHOP_PRODUCT(shop, counter) != NOTHING; counter++) {
		obj = real_object_proto(SHOP_PRODUCT(shop, counter));
		if (same_obj(item, obj))
			return (TRUE);
	}
	return (FALSE);
}


int
transaction_amt(char *arg)
{
	int num;

	one_argument(arg, buf);
	if (*buf)
		if ((is_number(buf))) {
			num = atoi(buf);
			strcpy(arg, arg + strlen(buf) + 1);
			return (num);
		}
	return (1);
}


char *
times_message(struct obj_data *obj, char *name, int num)
{
	static char buf[256];
	char *ptr;

	if (obj)
		strcpy(buf, obj->short_description);
	else {
		if ((ptr = strchr(name, '.')) == NULL)
			ptr = name;
		else
			ptr++;
		sprintf(buf, "%s %s", AN(ptr), ptr);
	}

	if (num > 1)
		sprintf(END_OF(buf), " (x %d)", num);
	return (buf);
}


struct obj_data *
get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list)
{
	struct obj_data *i, *last_match = 0;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if (!(number = get_number(&tmp)))
		return (0);

	for (i = list, j = 1; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->name))
			if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
				if (j == number)
					return (i);
				last_match = i;
				j++;
			}
	return (0);
}


struct obj_data *
get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list)
{
	struct obj_data *loop, *last_obj = 0;
	int index;

	if ((is_number(name + 1)))
		index = atoi(name + 1);
	else
		return (0);

	for (loop = list; loop; loop = loop->next_content)
		if (CAN_SEE_OBJ(ch, loop) && (loop->shared->cost > 0))
			if (!same_obj(last_obj, loop)) {
				if (--index == 0)
					return (loop);
				last_obj = loop;
			}
	return (0);
}


struct obj_data *
get_purchase_obj(struct char_data *ch, char *arg,
	struct char_data *keeper, struct shop_data *shop, int msg)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	struct obj_data *obj;

	one_argument(arg, name);
	do {
		if (*name == '#')
			obj = get_hash_obj_vis(ch, name, keeper->carrying);
		else
			obj = get_slide_obj_vis(ch, name, keeper->carrying);
		if (!obj) {
			if (msg) {
				strcpy(buf, shop->no_such_item1);
				perform_tell(keeper, ch, buf);
			}
			return (0);
		}
		if (GET_OBJ_COST(obj) <= 0) {
			extract_obj(obj);
			obj = 0;
		}
	} while (!obj);
	return (obj);
}


int
buy_price(struct obj_data *obj, struct shop_data *shop)
{
	int price;
	price = ((int)(GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop)));
	if (OBJ_REINFORCED(obj))
		price += (price >> 3);
	if (OBJ_ENHANCED(obj))
		price += (price >> 3);
	return (price);
}


void
shopping_buy(char *arg, struct char_data *ch,
	struct char_data *keeper, struct shop_data *shop)
{
	char tempstr[200], buf[MAX_STRING_LENGTH];
	struct obj_data *obj, *last_obj = NULL;
	int goldamt = 0, buynum, bought = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;

	sort_keeper_objs(keeper, shop);

	if ((buynum = transaction_amt(arg)) < 0) {
		perform_tell(keeper, ch,
			"A negative amount?  Try selling me something.");
		return;
	}
	if (!(*arg) || !(buynum)) {
		perform_tell(keeper, ch, "What do you want to buy??");
		return;
	}
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop, TRUE)))
		return;

	if ((buy_price(obj, shop) > GET_MONEY(ch, shop)) && !IS_GOD(ch)) {
		strcpy(buf, shop->missing_cash2);
		perform_tell(keeper, ch, buf);

		switch (SHOP_BROKE_TEMPER(shop)) {
		case 0:
			do_action(keeper, GET_NAME(ch), cmd_puke, 0);
			return;
		case 1:
			do_echo(keeper, "smokes on a fat joint.", cmd_emote, SCMD_EMOTE);
			return;
		case 2:
			do_action(keeper, "", cmd_spit, 0);
			return;
		case 3:
			do_action(keeper, GET_NAME(ch), cmd_fart, 0);
			return;
		case 4:
			act("$n tokes on $s golden hookah.", FALSE, ch, 0, 0, TO_ROOM);
			return;
		case 5:
			act("$n inserts a microsoft behind $s ear and zones out.",
				FALSE, ch, 0, 0, TO_ROOM);
			return;
		default:
			return;
		}
	}
	if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
		sprintf(buf, "%s: You can't carry any more items.\r\n",
			fname(obj->name));
		send_to_char(buf, ch);
		return;
	}
	if ((IS_CARRYING_W(ch) + obj->getWeight()) > CAN_CARRY_W(ch)) {
		if (!number(0, 2))
			sprintf(buf, "%s: You can't carry that much weight.\r\n",
				fname(obj->name));
		else if (!number(0, 1))
			sprintf(buf, "%s: You can't carry any more weight.\r\n",
				fname(obj->name));
		else
			sprintf(buf, "%s: You can carry no more weight.\r\n",
				fname(obj->name));
		send_to_char(buf, ch);
		return;
	}
	while ((obj) && ((GET_MONEY(ch, shop) >= buy_price(obj, shop)) ||
			IS_GOD(ch))
		&& (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) && (bought < buynum)
		&& (IS_CARRYING_W(ch) + obj->getWeight() <= CAN_CARRY_W(ch))) {
		bought++;
		/* Test if producing shop ! */
		if (shop_producing(obj, shop))
			obj = read_object(GET_OBJ_VNUM(obj));
		else {
			obj_from_char(obj);
			SHOP_SORT(shop)--;
		}
		obj_to_char(obj, ch);

		goldamt += buy_price(obj, shop);
		if (!IS_GOD(ch)) {
			if (SHOP_CURRENCY(shop) == 1)
				GET_CASH(ch) -= buy_price(obj, shop);
			else
				GET_GOLD(ch) -= buy_price(obj, shop);
		}

		last_obj = obj;
		obj = get_purchase_obj(ch, arg, keeper, shop, FALSE);
		if (!same_obj(obj, last_obj))
			break;
	}

	if (bought < buynum) {
		if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "I only have %d to sell you.", bought);
		else if (GET_MONEY(ch, shop) < buy_price(obj, shop))
			sprintf(buf, "You can only afford %d.", bought);
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			sprintf(buf, "You can only hold %d.", bought);
		else if (IS_CARRYING_W(ch) + obj->getWeight() > CAN_CARRY_W(ch))
			sprintf(buf, "You can only carry %d.", bought);
		else
			sprintf(buf, "Something screwy only gave you %d.", bought);
		perform_tell(keeper, ch, buf);
	}
	if (!IS_GOD(ch)) {
		if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
			GET_CASH(keeper) += goldamt;
		else
			GET_GOLD(keeper) += goldamt;
	}

	sprintf(tempstr, times_message(last_obj, 0, bought));
	sprintf(buf, "$n buys %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop->message_buy, goldamt);
	perform_tell(keeper, ch, buf);
	sprintf(buf, "You now have %s.\r\n", tempstr);
	send_to_char(buf, ch);

	if (SHOP_USES_BANK(shop))
		if (GET_MONEY(keeper, shop) > MAX_OUTSIDE_BANK) {
			SHOP_BANK(shop) += (GET_MONEY(keeper, shop) - MAX_OUTSIDE_BANK);
			if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
				GET_CASH(keeper) = MAX_OUTSIDE_BANK;
			else
				GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
		}
}


struct obj_data *
get_selling_obj(struct char_data *ch, char *name,
	struct char_data *keeper, struct shop_data *shop, int msg)
{
	char buf[MAX_STRING_LENGTH];
	struct obj_data *obj;
	int result;

	if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
		if (msg) {
			strcpy(buf, shop->no_such_item2);
			perform_tell(keeper, ch, buf);
		}
		return (0);
	}
	if ((result = trade_with(obj, shop)) == OBJECT_OK)
		return (obj);

	switch (result) {
	case OBJECT_NOTOK:
		strcpy(buf, shop->do_not_buy);
		break;
	case OBJECT_DEAD:
		strcpy(buf, MSG_NO_USED_WANDSTAFF);
		break;
	case OBJECT_NOSELL:
		strcpy(buf, MSG_NO_SELL_OBJ);
		break;
	case OBJECT_BROKEN:
		strcpy(buf, MSG_BROKEN_OBJ);
		break;
	default:
		sprintf(buf, "Illegal return value of %d from trade_with() (shop.c)",
			result);
		slog(buf);
		strcpy(buf, "An error has occurred.");
		break;
	}
	if (msg)
		perform_tell(keeper, ch, buf);
	return (0);
}


int
sell_price(struct char_data *ch, struct obj_data *obj, struct shop_data *shop)
{
	int price;

	price = ((int)(GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop)));
	if (OBJ_REINFORCED(obj))
		price += (price >> 2);
	if (OBJ_ENHANCED(obj))
		price += (price >> 2);
	return (price);

}

int
shop_inventory(struct char_data *ch, struct obj_data *obj)
{
	int count = 0;
	struct obj_data *o;

	for (o = ch->carrying; o; o = o->next_content) {
		if (same_obj(obj, o)) {
			count++;
		}
	}

	return count;
}

//
// slide_obj can extract the obj !!  do not use it after callig slide_obj!!
//

struct obj_data *
slide_obj(struct obj_data *obj, struct char_data *keeper,
	struct shop_data *shop)
	/*
	   This function is a slight hack!  To make sure that duplicate items are
	   only listed once on the "list", this function groups "identical"
	   objects together on the shopkeeper's inventory list.  The hack involves
	   knowing how the list is put together, and manipulating the order of
	   the objects on the list.  (But since most of DIKU is not encapsulated,
	   and information hiding is almost never used, it isn't that big a deal) -JF
	 */
{
	int temp;

	if (SHOP_SORT(shop) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop);

	/* Extract the object if it is identical to one produced */
	if (shop_producing(obj, shop) || shop_inventory(keeper, obj) >= 5) {
		/*
		   if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
		   GET_CASH(keeper) += GET_OBJ_COST(obj);
		   else
		   GET_GOLD(keeper) += GET_OBJ_COST(obj);
		 */
		temp = GET_OBJ_VNUM(obj);
		extract_obj(obj);
		return (real_object_proto(temp));
	}

	obj_to_char(obj, keeper);
	SHOP_SORT(shop)++;
	return (obj);
}


void
sort_keeper_objs(struct char_data *keeper, struct shop_data *shop)
{
	struct obj_data *list = NULL, *temp = NULL;

	SHOP_SORT(shop) = 0;
	while (SHOP_SORT(shop) < IS_CARRYING_N(keeper)) {
		temp = keeper->carrying;
		obj_from_char(temp);
		temp->next_content = list;
		list = temp;
	}

	while (list) {
		temp = list;
		list = list->next_content;
		if ((shop_producing(temp, shop)) &&
			!(get_obj_in_list_num(GET_OBJ_VNUM(temp), keeper->carrying))) {
			obj_to_char(temp, keeper);
			SHOP_SORT(shop)++;
		} else
			(void)slide_obj(temp, keeper, shop);
	}
}


void
shopping_sell(char *arg, struct char_data *ch,
	struct char_data *keeper, struct shop_data *shop)
{
	char tempstr[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], name[200];
	struct obj_data *obj, *tag = NULL;
	int sellnum, sold = 0, goldamt = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;

	if ((sellnum = transaction_amt(arg)) < 0) {
		strcpy(buf, "A negative amount?  Try buying something.");
		perform_tell(keeper, ch, buf);
		return;
	}
	if (!(*arg) || !(sellnum)) {
		strcpy(buf, "What do you want to sell??");
		perform_tell(keeper, ch, buf);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop, TRUE)))
		return;

	if (GET_MONEY(keeper, shop) + SHOP_BANK(shop) < sell_price(ch, obj, shop)) {
		strcpy(buf, shop->missing_cash1);
		perform_tell(keeper, ch, buf);
		return;
	}
	if (IS_CORPSE(obj)) {
		strcpy(buf, "Take your corpse and get the hell out of my store!");
		perform_tell(keeper, ch, buf);
		return;
	}

	int obj_selling_price = 0;

	while ((obj) && (GET_MONEY(keeper, shop) + SHOP_BANK(shop) >=
			(obj_selling_price = sell_price(ch, obj, shop)))
		&& (sold < sellnum)) {
		sold++;

		obj_from_char(obj);
		tag = slide_obj(obj, keeper, shop);

		goldamt += obj_selling_price;

		if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
			GET_CASH(keeper) -= obj_selling_price;
		else
			GET_GOLD(keeper) -= obj_selling_price;

		obj = get_selling_obj(ch, name, keeper, shop, FALSE);
	}

	if (sold < sellnum) {
		if (!obj)
			sprintf(buf, "You only have %d of those.", sold);
		else if (GET_MONEY(keeper, shop) + SHOP_BANK(shop) <
			sell_price(ch, obj, shop))
			sprintf(buf, "I can only afford to buy %d of those.", sold);
		else
			sprintf(buf, "Something really screwy made me buy %d.", sold);

		perform_tell(keeper, ch, buf);
	}

	if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
		GET_CASH(ch) += goldamt;
	else
		GET_GOLD(ch) += goldamt;
	strcpy(tempstr, times_message(0, name, sold));
	sprintf(buf, "$n sells %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop->message_sell, goldamt);
	perform_tell(keeper, ch, buf);
	sprintf(buf, "$N now has %s.", tempstr);
	act(buf, FALSE, ch, 0, keeper, TO_CHAR | TO_SLEEP);

	if (GET_MONEY(keeper, shop) < MIN_OUTSIDE_BANK) {
		goldamt = MIN(MAX_OUTSIDE_BANK - GET_MONEY(keeper, shop),
			SHOP_BANK(shop));
		SHOP_BANK(shop) -= goldamt;
		if (SHOP_CURRENCY(shop) == CURRENCY_CREDITS)
			GET_CASH(keeper) += goldamt;
		else
			GET_GOLD(keeper) += goldamt;
	}
}


void
shopping_value(char *arg, struct char_data *ch,
	struct char_data *keeper, struct shop_data *shop)
{
	char buf[MAX_STRING_LENGTH];
	struct obj_data *obj;
	char name[MAX_INPUT_LENGTH];

	if (!(is_ok(keeper, ch, shop)))
		return;

	if (!(*arg)) {
		strcpy(buf, "What do you want me to valuate??");
		perform_tell(keeper, ch, buf);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop, TRUE)))
		return;

	sprintf(buf, "I'll give you %d %s for that!", sell_price(ch, obj, shop),
		(SHOP_CURRENCY(shop) == CURRENCY_CREDITS) ? "credits" : "gold coins");
	perform_tell(keeper, ch, buf);

	return;
}


char *
list_object(struct obj_data *obj, struct char_data *ch, int cnt,
	int index, struct shop_data *shop)
{
	static char buf[256];
	char buf2[300], buf3[200];

	if (shop_producing(obj, shop))
		sprintf(buf2, "%sUnlimited%s   ", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	else
		sprintf(buf2, "%s%5d%s       ", CCYEL(ch, C_NRM), cnt, CCNRM(ch,
				C_NRM));
	sprintf(buf, " %2d%s)%s  %s", index, CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
		buf2);

	/* Compile object name and information */
	strcpy(buf3, obj->short_description);
	if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, 1)))
		sprintf(END_OF(buf3), " of %s", drinks[GET_OBJ_VAL(obj, 2)]);

	/* FUTURE: */
	/* Add glow/hum/etc */

	if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF)) {
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			strcat(buf3, " (partially used)");
	} else {
		if (OBJ_REINFORCED(obj))
			strcat(buf3, " [reinforced]");
		if (OBJ_ENHANCED(obj))
			strcat(buf3, " |augmented|");
		if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
			strcat(buf3, " <broken>");
		if (IS_OBJ_STAT(obj, ITEM_HUM)) {
			strcat(buf3, " (humming)");
		} else if (IS_OBJ_STAT(obj, ITEM_GLOW)) {
			strcat(buf3, " (glowing)");
		} else if (IS_OBJ_STAT(obj, ITEM_INVISIBLE)) {
			strcat(buf3, " (invisible)");
		} else if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT)) {
			strcat(buf3, " (transparent)");
		} else if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) ||
			IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) {
			if (IS_OBJ_STAT(obj, ITEM_BLESS)) {
				strcat(buf3, " (holy aura)");
			} else if (IS_OBJ_STAT(obj, ITEM_BLESS)) {
				strcat(buf3, " (unholy aura)");
			}
		}
	}


	sprintf(buf2, "%-48s %6d\r\n", buf3, buy_price(obj, shop));
	strcat(buf, CAP(buf2));
	return (buf);
}


void
shopping_list(char *arg, struct char_data *ch,
	struct char_data *keeper, struct shop_data *shop)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	struct obj_data *obj, *last_obj = 0;
	int cnt = 0, index = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;

	sort_keeper_objs(keeper, shop);

	one_argument(arg, name);
	strcpy(buf, CCCYN(ch, C_NRM));
	strcat(buf,
		" ##   Available   Item                                               Cost\r\n");
	strcat(buf,
		"-------------------------------------------------------------------------\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	if (keeper->carrying)
		for (obj = keeper->carrying; obj; obj = obj->next_content)
			if (CAN_SEE_OBJ(ch, obj) && (obj->shared->cost > 0)) {
				if (!last_obj) {
					last_obj = obj;
					cnt = 1;
				} else if (same_obj(last_obj, obj))
					cnt++;
				else {
					index++;
					if (!(*name) || isname(name, last_obj->name))
						strcat(buf, list_object(last_obj, ch, cnt, index,
								shop));
					cnt = 1;
					last_obj = obj;
				}
			}
	index++;
	if (!last_obj)
		if (*name)
			strcpy(buf, "Presently, none of those are for sale.\r\n");
		else
			strcpy(buf, "Currently, there is nothing for sale.\r\n");
	else if (!(*name) || isname(name, last_obj->name))
		strcat(buf, list_object(last_obj, ch, cnt, index, shop));

	page_string(ch->desc, buf, 1);
}


int
ok_shop_room(struct shop_data *shop, int room)
{
	int index;

	for (index = 0; SHOP_ROOM(shop, index) != NOWHERE; index++)
		if (SHOP_ROOM(shop, index) == room)
			return (TRUE);
	return (FALSE);
}


SPECIAL(shop_keeper)
{
	char argm[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	struct char_data *keeper = (struct char_data *)me;
	struct char_data *sucker;
	struct shop_data *shop = NULL;
	ACMD(do_order);
	ACMD(do_gen_comm);
	void perform_tell(struct char_data *ch, struct char_data *vict,
		char *messg);


	if (spec_mode == SPECIAL_DEATH)
		return 0;

	if (!MOB2_FLAGGED(keeper, MOB2_WONT_WEAR)
		&& !MOB2_FLAGGED(keeper, MOB2_SELLER))
		SET_BIT(MOB2_FLAGS(keeper), MOB2_WONT_WEAR);

	if (!CMD_IS("moan") && !CMD_IS("gossip") && !CMD_IS("holler") &&
		!CMD_IS("list") && !CMD_IS("buy") && !CMD_IS("value") &&
		!CMD_IS("sell") && !CMD_IS("help") && !CMD_IS("cringe") &&
		!CMD_IS("lick") && !CMD_IS("fart") && !CMD_IS("hump") &&
		!CMD_IS("howl") && !CMD_IS("cringe") && !CMD_IS("rofl") &&
		!CMD_IS("cry") && !CMD_IS("worship") && !CMD_IS("remove") &&
		!CMD_IS("bark") && !CMD_IS("joint") && !CMD_IS("sleep") &&
		!CMD_IS("wake") && !CMD_IS("drop")) {
		if (keeper->followers) {
			sucker = ch;
			if (IS_AFFECTED(sucker, AFF_CHARM) && sucker->master &&
				sucker->master == keeper
				&& sucker->in_room == keeper->in_room) {
				switch (number(0, 60)) {
				case 0:
					act("$n falls down laughing at you!",
						FALSE, keeper, 0, sucker, TO_VICT);
					act("$n falls down laughing at $N!",
						FALSE, keeper, 0, sucker, TO_NOTVICT);
					break;
				case 1:
					act("$n slaps your face and laughs!",
						FALSE, keeper, 0, sucker, TO_VICT);
					act("$n slaps $N's face and laughs!",
						FALSE, keeper, 0, sucker, TO_NOTVICT);
					break;
				case 2:
					if (GET_GOLD(sucker)) {
						act("$n convinces you to give $m a loan.",
							FALSE, keeper, 0, sucker, TO_VICT);
						act("$n convinces $N to give $m a loan.",
							FALSE, keeper, 0, sucker, TO_NOTVICT);
						GET_GOLD(sucker) =
							MAX(0, GET_GOLD(sucker) - GET_LEVEL(keeper));
					}
					break;
				case 3:
					sprintf(buf2,
						"%s gossip I am a fool.  I tried to charm %s.",
						GET_NAME(sucker), GET_NAME(keeper));
					do_order(keeper, buf2, 0, 0);
					break;
				case 4:
					sprintf(buf2,
						"%s gossip Anybody want anything from the store?",
						GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 5:
					sprintf(buf, CAP(GET_NAME(keeper)));
					sprintf(buf2, "%s holler WOW!! %s is so great!",
						GET_NAME(sucker), buf);
					do_order(keeper, buf2, 0, 0);
					break;
				case 6:
					sprintf(buf2, "Everyone come to my store!  %s is buying.",
						GET_NAME(sucker));
					do_gen_comm(keeper, buf2, 0, SCMD_GOSSIP);
					break;
				case 7:
					sprintf(buf2, "%s fart", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 8:
					sprintf(buf2, "%s lick", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 9:
					sprintf(buf2, "%s hump", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 10:
					sprintf(buf2, "%s howl", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 11:
					sprintf(buf2, "%s cringe", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 12:
					sprintf(buf2, "%s rofl", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 13:
					sprintf(buf2, "%s cry", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 14:
					sprintf(buf2, "%s worship", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 15:
					sprintf(buf2, "%s remove all", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					sprintf(buf2, "%s drop all", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 16:
					sprintf(buf2, "%s moan", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 17:
					sprintf(buf2, "%s bark", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 18:
					sprintf(buf2, "%s joint", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				case 19:
					if (sucker->getPosition() > POS_SLEEPING)
						sprintf(buf2, "%s sleep", GET_NAME(sucker));
					if (sucker->getPosition() == POS_SLEEPING)
						sprintf(buf2, "%s wake", GET_NAME(sucker));
					do_order(keeper, buf2, 0, 0);
					break;
				default:
					return 0;
				}
				return 1;
			}
		}
	}

	for (shop = shop_index; shop; shop = shop->next)
		if (SHOP_KEEPER(shop) == keeper->mob_specials.shared->vnum)
			break;

	if (!shop) {
		do_say(keeper, "Dammit!!  Who's the moron?", 0, 0);
		keeper->mob_specials.shared->func = NULL;
		return (FALSE);
	}

	if (SHOP_FUNC(shop))		/* Check secondary function */
		if ((SHOP_FUNC(shop)) (ch, me, cmd, argument, 0))
			return (TRUE);

	if (keeper == ch) {
		if (cmd)
			SHOP_SORT(shop) = 0;	/* Safety in case "drop all" */
		return (FALSE);
	}
	if (!MOB2_FLAGGED(keeper, MOB2_SELLER) &&
		!ok_shop_room(shop, ch->in_room->number))
		return (0);

	if (!AWAKE(keeper))
		return (FALSE);

	if (CMD_IS("steal") && (GET_CLASS(keeper) != CLASS_THIEF)
		&& (GET_CLASS(keeper) != CLASS_HOOD)
		&& GET_LEVEL(ch) < LVL_IMMORT) {
		sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
		do_action(keeper, GET_NAME(ch), cmd_slap, 0);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return (TRUE);
	}
	if (CMD_IS("buy")) {
		shopping_buy(argument, ch, keeper, shop);
		return (TRUE);
	} else if (CMD_IS("sell")) {
		shopping_sell(argument, ch, keeper, shop);
		return (TRUE);
	} else if (CMD_IS("value")) {
		shopping_value(argument, ch, keeper, shop);
		return (TRUE);
	} else if (CMD_IS("list")) {
		shopping_list(argument, ch, keeper, shop);
		return (TRUE);
	} else if (CMD_IS("ask") || CMD_IS("tell")) {
		skip_spaces(&argument);
		if (!*argument)
			return 0;
		half_chop(argument, buf, buf2);

		if (!isname(buf, keeper->player.name))
			return 0;
		else if (strstr(buf2, "list") != NULL) {
			perform_tell(keeper, ch, "I am happy to list all my goods.");
			perform_tell(keeper, ch,
				"Just type LIST. Then you buy them.. NO?");
		} else if (strstr(buf2, "buy") != NULL) {
			perform_tell(keeper, ch,
				"Type BUY <item name>, <item number>, or BUY # <item>.");
		} else if (strstr(buf2, "sell") != NULL) {
			perform_tell(keeper, ch,
				"I will gladly consider anything you wish to sell.");
			perform_tell(keeper, ch, "As long as you will buy my goods.");
		} else if (strstr(buf2, "value") != NULL) {
			perform_tell(keeper, ch,
				"I will gladly give you the value of any of your goods."
				"That way if I give you a fair price you will give me a fair price.");
		} else if (strstr(buf2, "help") != NULL) {
			perform_tell(keeper, ch, "I can help you by LIST, BUY, or SELL");
		} else
			return 0;
		return 1;
	} else
		return (FALSE);
}


int
ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{
	char buf[200];
	struct shop_data *shop = NULL;

	if (ch && GET_LEVEL(ch) > LVL_CREATOR)
		return TRUE;

	if (IS_NPC(victim) && victim->mob_specials.shared->func == shop_keeper)
		for (shop = shop_index; shop; shop = shop->next)
			if ((GET_MOB_VNUM(victim) == SHOP_KEEPER(shop))
				&& !SHOP_KILL_CHARS(shop)
				&& !IS_AFFECTED(victim, AFF_CHARM)) {
				if (ch) {
					do_action(victim, GET_NAME(ch), cmd_slap, 0);
					strcpy(buf, MSG_CANT_KILL_KEEPER);
					perform_tell(victim, ch, buf);
				}
				if (FIGHTING(victim)) {
					stop_fighting(FIGHTING(victim));
				}
				return (FALSE);
			}
	return (TRUE);
}


int
add_to_list(struct shop_buy_data *list, int type, int *len, int *val)
{
	if (*val >= 0)
		if (*len < MAX_SHOP_OBJ) {
			if (type == LIST_PRODUCE)
				if (!real_object_proto(*val))
					*val = -1;
			if (*val >= 0) {
				BUY_TYPE(list[*len]) = *val;
				BUY_WORD(list[(*len)++]) = 0;
			} else
				*val = 0;
			return (FALSE);
		} else
			return (TRUE);
	return (FALSE);
}


int
end_read_list(struct shop_buy_data *list, int len, int error)
{
	if (error) {
		sprintf(buf, "Raise MAX_SHOP_OBJ constant in shop.h to %d",
			len + error);
		slog(buf);
	}
	BUY_WORD(list[len]) = 0;
	BUY_TYPE(list[len++]) = NOTHING;
	return (len);
}


void
read_line(FILE * shop_f, byte * data, struct shop_data *shop)
{
	int i;

	if (!get_line(shop_f, buf) || !sscanf(buf, "%d", &i)) {
		fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(shop));
		safe_exit(1);
	}
	*data = i;
}

void
read_line(FILE * shop_f, int *data, struct shop_data *shop)
{
	if (!get_line(shop_f, buf) || !sscanf(buf, "%d", data)) {
		fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(shop));
		safe_exit(1);
	}
}

void
read_line(FILE * shop_f, float *data, struct shop_data *shop)
{
	if (!get_line(shop_f, buf) || !sscanf(buf, "%f", data)) {
		fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(shop));
		safe_exit(1);
	}
}


int
read_list(FILE * shop_f, struct shop_buy_data *list, int new_format,
	int max, int type, struct shop_data *shop)
{
	int count, temp, len = 0, error = 0;

	if (new_format) {
		do {
			read_line(shop_f, &temp, shop);
			error += add_to_list(list, type, &len, &temp);
		} while (temp >= 0);
	} else
		for (count = 0; count < max; count++) {
			read_line(shop_f, &temp, shop);
			error += add_to_list(list, type, &len, &temp);
		}
	return (end_read_list(list, len, error));
}


int
read_type_list(FILE * shop_f, struct shop_buy_data *list,
	int new_format, int max, struct shop_data *shop)
{
	int index, num, len = 0, error = 0;
	char *ptr;

	if (!new_format)
		return (read_list(shop_f, list, 0, max, LIST_TRADE, shop));
	do {
		fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
		if ((ptr = strchr(buf, ';')) != NULL)
			*ptr = 0;
		else
			*(END_OF(buf) - 1) = 0;
		for (index = 0, num = NOTHING; *item_types[index] != '\n'; index++)
			if (!strncmp(item_types[index], buf, strlen(item_types[index]))) {
				num = index;
				strcpy(buf, buf + strlen(item_types[index]));
				break;
			}
		ptr = buf;
		if (num == NOTHING) {
			sscanf(buf, "%d", &num);
			while (!isdigit(*ptr))
				ptr++;
			while (isdigit(*ptr))
				ptr++;
		}
		while (isspace(*ptr))
			ptr++;
		while (isspace(*(END_OF(ptr) - 1)))
			*(END_OF(ptr) - 1) = 0;
		error += add_to_list(list, LIST_TRADE, &len, &num);
		if (*ptr)
			BUY_WORD(list[len - 1]) = str_dup(ptr);
	} while (num >= 0);
	return (end_read_list(list, len, error));
}


void
boot_the_shops(FILE * shop_f, char *filename, int rec_count)
{
	char *buf, buf2[150], output_buf[MAX_STRING_LENGTH];
	int temp, count, new_format = 0;
	struct char_data *shop_dude = NULL;
	struct shop_data *new_shop = NULL, *tmp_shop = NULL;
	struct shop_buy_data list[MAX_SHOP_OBJ + 1];
	int done = 0;

	sprintf(buf2, "beginning of shop file %s", filename);

	while (!done) {
		buf = fread_string(shop_f, buf2);
		if (*buf == '#') {		/* New shop */
			sscanf(buf, "#%d\n", &temp);
			sprintf(buf2, "shop #%d in shop file %s", temp, filename);
			free(buf);			/* Plug memory leak! */
			CREATE(new_shop, struct shop_data, 1);

			SHOP_NUM(new_shop) = temp;
			temp =
				read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE,
				new_shop);
			CREATE(new_shop->producing, int, temp);
			for (count = 0; count < temp; count++)
				SHOP_PRODUCT(new_shop, count) = BUY_TYPE(list[count]);

			read_line(shop_f, &SHOP_BUYPROFIT(new_shop), new_shop);
			read_line(shop_f, &SHOP_SELLPROFIT(new_shop), new_shop);

			temp =
				read_type_list(shop_f, list, new_format, MAX_TRADE, new_shop);
			CREATE(new_shop->type, struct shop_buy_data, temp);
			for (count = 0; count < temp; count++) {
				SHOP_BUYTYPE(new_shop, count) = (byte) BUY_TYPE(list[count]);
				SHOP_BUYWORD(new_shop, count) = BUY_WORD(list[count]);
			}

			new_shop->no_such_item1 = fread_string(shop_f, buf2);
			new_shop->no_such_item2 = fread_string(shop_f, buf2);
			new_shop->do_not_buy = fread_string(shop_f, buf2);
			new_shop->missing_cash1 = fread_string(shop_f, buf2);
			new_shop->missing_cash2 = fread_string(shop_f, buf2);
			new_shop->message_buy = fread_string(shop_f, buf2);
			new_shop->message_sell = fread_string(shop_f, buf2);

			//
			// check the printf-formatted buy/sell messages
			//
			if (!shop_check_message_format(new_shop->message_buy)) {
				sprintf(output_buf,
					"SYSERR: error in shop %d buy message: '%s'.",
					SHOP_NUM(new_shop), new_shop->message_buy);
				slog(output_buf);
				free(new_shop->message_buy);
				new_shop->message_buy = strdup(SHOP_DEFAULT_MESSAGE_BUY);
			}

			if (!shop_check_message_format(new_shop->message_sell)) {
				sprintf(output_buf,
					"SYSERR: error in shop %d sell message: '%s'.",
					SHOP_NUM(new_shop), new_shop->message_sell);
				slog(output_buf);
				free(new_shop->message_sell);
				new_shop->message_sell = strdup(SHOP_DEFAULT_MESSAGE_SELL);
			}

			read_line(shop_f, &SHOP_BROKE_TEMPER(new_shop), new_shop);
			read_line(shop_f, &SHOP_BITVECTOR(new_shop), new_shop);
			read_line(shop_f, &SHOP_KEEPER(new_shop), new_shop);

			if (!real_mobile_proto(SHOP_KEEPER(new_shop)))
				SHOP_KEEPER(new_shop) = -1;
			read_line(shop_f, &SHOP_TRADE_WITH(new_shop), new_shop);

			temp = read_list(shop_f, list, new_format, 1, LIST_ROOM, new_shop);
			CREATE(new_shop->in_room, int, temp);
			for (count = 0; count < temp; count++)
				SHOP_ROOM(new_shop, count) = BUY_TYPE(list[count]);

			read_line(shop_f, &SHOP_OPEN1(new_shop), new_shop);
			read_line(shop_f, &SHOP_CLOSE1(new_shop), new_shop);
			read_line(shop_f, &SHOP_OPEN2(new_shop), new_shop);
			read_line(shop_f, &SHOP_CLOSE2(new_shop), new_shop);
			read_line(shop_f, &SHOP_CURRENCY(new_shop), new_shop);
			if (new_format > 1)
				read_line(shop_f, &SHOP_REVENUE(new_shop), new_shop);

			/*      if (new_shop->vnum >= 30000 && new_shop->vnum < 40000)
			   SHOP_CURRENCY(new_shop)  = CURRENCY_CREDITS; */
			SHOP_BANK(new_shop) = 0;
			SHOP_SORT(new_shop) = 0;
			SHOP_FUNC(new_shop) = 0;
			top_shop++;

			shop_dude = real_mobile_proto(SHOP_KEEPER(new_shop));

			if (SHOP_KILL_CHARS(new_shop) && (shop_dude != NULL) &&
				MOB_FLAGGED(shop_dude, MOB_HELPER))
				REMOVE_BIT(MOB_FLAGS(shop_dude), MOB_HELPER);

			new_shop->next = NULL;

			if (shop_index) {
				for (tmp_shop = shop_index; tmp_shop;
					tmp_shop = tmp_shop->next)
					if (!tmp_shop->next) {
						tmp_shop->next = new_shop;
						break;
					}
			} else
				shop_index = new_shop;


		} else {
			if (*buf == '$')	/* EOF */
				done = TRUE;
			else if (strstr(buf, VERSION3_TAG)) {	/* New format marker */
				new_format = 1;
				char *r = NULL;
				int patch_level = 0;
				char temp[100];
				sprintf(temp, "%s%s", VERSION3_TAG, SHP_MOD_LEV);
				r = strstr(buf, temp);
				if (r) {
					r += strlen(temp);
					sscanf(r, "%d", &patch_level);
					new_format += patch_level;
				}
			}
			free(buf);			/* Plug memory leak! */
		}
	}
}


void
assign_the_shopkeepers(void)
{
	struct char_data *mob;
	struct shop_data *shop;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");
	cmd_emote = find_command("emote");
	cmd_slap = find_command("slap");
	cmd_puke = find_command("puke");
	cmd_fart = find_command("fart");
	cmd_spit = find_command("spit");

	for (shop = shop_index; shop; shop = shop->next) {
		if (SHOP_KEEPER(shop) < 0)
			continue;
		mob = real_mobile_proto(SHOP_KEEPER(shop));

		if (mob->mob_specials.shared->func != NULL
			&& mob->mob_specials.shared->func != shop_keeper) {
			SHOP_FUNC(shop) = mob->mob_specials.shared->func;
		}
		mob->mob_specials.shared->func = shop_keeper;
	}
}


char *
customer_string(struct shop_data *shop, int detailed)
{
	int index, cnt = 1;
	static char buf[256];

	*buf = 0;
	for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
		if (!(SHOP_TRADE_WITH(shop) & cnt))
			if (detailed) {
				if (*buf)
					strcat(buf, ", ");
				strcat(buf, trade_letters[index]);
			} else
				sprintf(END_OF(buf), "%c", *trade_letters[index]);
		else if (!detailed)
			strcat(buf, "_");

	return (buf);
}


void
list_all_shops(struct char_data *ch, struct zone_data *zone)
{
	struct shop_data *shop;
	int shop_nr;
	struct char_data *keeper = NULL;

	strcpy(buf, "\r\n");
	for (shop_nr = 0, shop = shop_index; shop; shop_nr++, shop = shop->next) {
		if (zone != NULL && (SHOP_NUM(shop) < (zone->number * 100) ||
				SHOP_NUM(shop) > zone->top))
			continue;
		if (!(shop_nr % (GET_PAGE_LENGTH(ch)))) {
			strcat(buf,
				"Virtual   Where    Keeper    Buy   Sell   Customers\r\n");
			strcat(buf,
				"---------------------------------------------------\r\n");
		}
		sprintf(buf2, "%s%6d%s   %s%6d%s    ",
			CCGRN(ch, C_NRM), SHOP_NUM(shop), CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), SHOP_ROOM(shop, 0), CCNRM(ch, C_NRM));
		if (!(keeper = real_mobile_proto(SHOP_KEEPER(shop))))
			strcpy(buf1, "<NONE>");
		else
			sprintf(buf1, "%s%6d%s", CCYEL(ch, C_NRM),
				keeper->mob_specials.shared->vnum, CCNRM(ch, C_NRM));
		sprintf(END_OF(buf2), "%s   %3.2f   %3.2f    ", buf1,
			SHOP_SELLPROFIT(shop), SHOP_BUYPROFIT(shop));
		strcat(buf2, customer_string(shop, FALSE));
		sprintf(END_OF(buf), "%s\r\n", buf2);
	}

	page_string(ch->desc, buf, 1);
}


void
handle_detailed_list(char *buf, char *buf1, struct char_data *ch)
{
	if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
		strcat(buf, buf1);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		sprintf(buf, "            %s", buf1);
	}
}


void
list_detailed_shop(struct char_data *ch, struct shop_data *shop)
{
	struct obj_data *obj;
	struct char_data *keeper = NULL;
	struct room_data *temp;
	int index;

	sprintf(buf, "Vnum:       [%s%5d%s]\r\n", CCGRN(ch, C_NRM),
		SHOP_NUM(shop), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	strcpy(buf, "Rooms:      ");
	for (index = 0; SHOP_ROOM(shop, index) != NOWHERE; index++) {
		if (index)
			strcat(buf, ", ");
		if ((temp = real_room(SHOP_ROOM(shop, index))) != NULL)
			sprintf(buf1, "%s%s%s (#%d)", CCCYN(ch, C_NRM), temp->name,
				CCNRM(ch, C_NRM), temp->number);
		else
			sprintf(buf1, "<UNKNOWN> (#%d)", SHOP_ROOM(shop, index));
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Rooms:      None!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Shopkeeper: ");
	if ((keeper = real_mobile_proto(SHOP_KEEPER(shop)))) {
		sprintf(END_OF(buf), "%s%s%s (#%d), Special Function: %s\r\n",
			CCYEL(ch, C_NRM), GET_NAME(keeper), CCNRM(ch, C_NRM),
			SHOP_KEEPER(shop), YESNO(SHOP_FUNC(shop)));
		send_to_char(buf, ch);
		if (shop->currency == 1) {
			sprintf(buf, "Credits     [%9d]\r\n", GET_CASH(keeper));
		} else {
			sprintf(buf,
				"Coins:      [%9d], Credits: [%9d], Bank: [%9d] (Total: %d)\r\n",
				GET_GOLD(keeper), GET_CASH(keeper),
				SHOP_BANK(shop),
				(GET_GOLD(keeper) + GET_CASH(keeper) + SHOP_BANK(shop)));
		}
	} else
		strcat(buf, "<NONE>\r\n");
	send_to_char(buf, ch);

	strcpy(buf1, customer_string(shop, TRUE));
	sprintf(buf, "Customers:  %s%s%s\r\n",
		CCMAG(ch, C_NRM), (*buf1) ? buf1 : "None", CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	strcpy(buf, "Produces:   ");
	for (index = 0;
		(obj = real_object_proto(SHOP_PRODUCT(shop, index))) != NULL;
		index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s%s%s (#%d)", CCGRN(ch, C_NRM),
			obj->short_description, CCNRM(ch, C_NRM), obj->shared->vnum);
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Produces:   Nothing!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Buys:       ");
	for (index = 0; SHOP_BUYTYPE(shop, index) != NOTHING; index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop, index)],
			SHOP_BUYTYPE(shop, index));
		if (SHOP_BUYWORD(shop, index))
			sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop, index));
		else
			strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Buys:       Nothing!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	sprintf(buf,
		"Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d], Currency: %s%s%s\r\n",
		SHOP_BUYPROFIT(shop), SHOP_SELLPROFIT(shop), SHOP_OPEN1(shop),
		SHOP_CLOSE1(shop), SHOP_OPEN2(shop), SHOP_CLOSE2(shop), CCCYN(ch,
			C_NRM),
		SHOP_CURRENCY(shop) == CURRENCY_CREDITS ? "CREDITS" : "GOLD", CCNRM(ch,
			C_NRM));

	send_to_char(buf, ch);

	sprintbit((long)SHOP_BITVECTOR(shop), shop_bits, buf1);
	sprintf(buf,
		"Bits:       %s\r\n"
		"Temper:     %d\r\n"
		"Revenue:    %d\r\n",
		buf1, SHOP_BROKE_TEMPER(shop), SHOP_REVENUE(shop));
	send_to_char(buf, ch);

	sprintf(buf,
		"Messages:\r\n"
		"%sSHOP_DONT_HAVE%s: %s\r\n"
		"%sCHAR_DONT_HAVE%s: %s\r\n"
		"%sSHOP_DONT_BUY%s:  %s\r\n"
		"%sSHOP_NO_CASH%s:   %s\r\n"
		"%sCHAR_NO_CASH%s:   %s\r\n"
		"%sMESSAGE_BUY%s:    %s\r\n"
		"%sMESSAGE_SELL%s:   %s\r\n\r\n",
		QRED, QNRM, shop->no_such_item1,
		QRED, QNRM, shop->no_such_item2,
		QRED, QNRM, shop->do_not_buy,
		QRED, QNRM, shop->missing_cash1,
		QRED, QNRM, shop->missing_cash2,
		QRED, QNRM, shop->message_buy, QRED, QNRM, shop->message_sell);
	send_to_char(buf, ch);
}


void
show_shops(struct char_data *ch, char *arg)
{
	struct shop_data *shop = NULL;
	struct zone_data *zone = NULL;
	int shop_nr = 0, found = 0;
	char part1[MAX_INPUT_LENGTH], part2[MAX_INPUT_LENGTH];
	char *argument;

	argument = one_argument(arg, part1);
	argument = two_arguments(argument, part1, part2);

	if (!*part1)
		list_all_shops(ch, NULL);
	else {
		if (!str_cmp(part1, ".")) {
			for (shop = shop_index; shop; shop = shop->next)
				if (ok_shop_room(shop, ch->in_room->number)) {
					list_detailed_shop(ch, shop);
					found = 1;
					break;
				}

			if (found != 1)
				send_to_char("This is not a shop!\r\n", ch);
			return;
		} else if (is_abbrev(part1, "zone")) {
			if (*part2 && is_number(part2)) {
				shop_nr = atoi(part2);
				for (zone = zone_table; zone; zone = zone->next)
					if (zone->number == shop_nr) {
						found = 1;
						list_all_shops(ch, zone);
						break;
					}
				if (found == 0)
					send_to_char("That zone doesn't exist, luser.\r\n", ch);
				else
					return;
			} else
				send_to_char
					("Usage: show shop [.|zone|vnum] [zone number].\r\n", ch);
		} else if (is_number(part1))
			shop_nr = atoi(part1);
		else {
			send_to_char("Usage: show shop [.|zone|vnum] [zone number].\r\n",
				ch);
			return;
		}

		for (shop = shop_index; shop; shop = shop->next) {
			if (SHOP_NUM(shop) == shop_nr) {
				list_detailed_shop(ch, shop);
				found = 1;
				break;
			}
		}

		if (found != 1)
			send_to_char("No such shop!\r\n", ch);
	}
}
