#include <vector>

#include "actions.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "weather.h"
#include "utils.h"
#include "vendor.h"
#include "specs.h"

const int MAX_ITEMS = 10;

// From act.comm.cc
void perform_tell(struct Creature *ch, struct Creature *vict, char *arg);
void perform_analyze( Creature *ch, obj_data *obj, bool checklev );
void perform_appraise( Creature *ch, obj_data *obj, int skill_lvl);

bool
same_obj(obj_data *obj1, obj_data *obj2)
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
			(obj1->name != obj1->shared->proto->name
				|| obj1->line_desc != obj1->shared->proto->line_desc))
		|| (obj2->shared->proto
			&& (obj2->name !=
				obj2->shared->proto->name
				|| obj2->line_desc != obj2->shared->proto->line_desc)))
		return FALSE;

	if ((obj1->name != obj2->name ||
			obj1->line_desc != obj2->line_desc) &&
		(str_cmp(obj1->name, obj2->name) ||
			!obj1->line_desc || !obj2->line_desc ||
			str_cmp(obj1->line_desc, obj2->line_desc)))
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

bool
ok_damage_vendor(struct Creature *ch, struct Creature *victim)
{
	ShopData shop;

	if (ch && GET_LEVEL(ch) > LVL_CREATOR)
		return true;
	if (victim && GET_LEVEL(victim) > LVL_IMMORT)
		return false;
	
	if (IS_NPC(victim) && victim->mob_specials.shared->func == vendor) {

		if (!GET_MOB_PARAM(victim))
			return false;

		vendor_parse_param(victim, GET_MOB_PARAM(victim), &shop, NULL);

		return shop.attack_ok;
	}

	return true;
}



static bool
vendor_is_produced(obj_data *obj, ShopData *shop)
{
	vector<int>::iterator it;

	for (it = shop->item_list.begin();it != shop->item_list.end();it++)
		if (GET_OBJ_VNUM(obj) == *it)
			return same_obj(real_object_proto(*it), obj);

	return false;
}

static int
vendor_inventory(Creature *self, obj_data *obj, obj_data *obj_list)
{
	obj_data *cur_obj;
	int cnt = 0;

	cur_obj = obj_list;
	while (cur_obj && GET_OBJ_VNUM(cur_obj) != GET_OBJ_VNUM(obj) &&
			!same_obj(cur_obj, obj))
		cur_obj = cur_obj->next_content;

	if (!cur_obj)
		return 0;

	while (same_obj(cur_obj, obj)) {
		cur_obj = cur_obj->next_content;
		cnt++;
	}

	return cnt;
}

static bool
vendor_invalid_buy(Creature *self, Creature *ch, ShopData *shop, obj_data *obj)
{
	if (GET_OBJ_COST(obj) < 1 || IS_OBJ_STAT(obj, ITEM_NOSELL) ||
			!OBJ_APPROVED(obj)|| obj->shared->owner_id != 0 ) {
		do_say(self, tmp_sprintf("%s %s", GET_NAME(ch), shop->msg_badobj),
			0, SCMD_SAY_TO, NULL);
		return true;
	}

	if (shop->item_types.size() > 0) {
		vector<int>::iterator it;
		bool accepted = false;
		for (it = shop->item_types.begin();it != shop->item_types.end();it++) {
			if ((*it & 0xFF) == GET_OBJ_TYPE(obj) || (*it & 0xFF) == 0) {
				accepted = *it >> 8;
				break;
			}
		}
		if (!accepted) {
			do_say(self, tmp_sprintf("%s %s", GET_NAME(ch),
				shop->msg_badobj), 0, SCMD_SAY_TO, NULL);
			return true;
		}
	}

	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
		do_say(self, tmp_sprintf("%s I'm not buying something that's already broken.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return true;
	}

	if (GET_OBJ_EXTRA3(obj) & ITEM3_HUNTED) {
		do_say(self, tmp_sprintf("%s This is hunted by the forces of Hell!  I'm not taking this!", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return true;
	}

	if (GET_OBJ_SIGIL_IDNUM(obj)) {
		do_say(self, tmp_sprintf("%s You'll have to remove that warding sigil before I'll bother.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return true;
	}

	if (vendor_inventory(self, obj, self->carrying) >= MAX_ITEMS) {
		do_say(self, tmp_sprintf("%s No thanks.  I've got too many of those in stock already.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return true;
	}

	// Adjust cost for missing charges
	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) {
		if (GET_OBJ_VAL(obj, 2) == 0) {
			do_say(self, tmp_sprintf("%s I don't buy used up wands or staves!", GET_NAME(ch)),
				0, SCMD_SAY_TO, NULL);
			return true;
		}
	}
	return false;
}

// Gets the value of an object, checking for buyability.
static unsigned long
vendor_get_value(obj_data *obj, int percent)
{
	unsigned long cost;

	// Adjust cost for wear and tear on a direct percentage basis
	if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1 &&
			GET_OBJ_MAX_DAM(obj) != 0)
		percent = percent *  GET_OBJ_DAM(obj) / GET_OBJ_MAX_DAM(obj);

	// Adjust cost for missing charges
	if ((GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) &&
			GET_OBJ_VAL(obj, 1) != 0)
		percent = percent * GET_OBJ_VAL(obj, 2) / GET_OBJ_VAL(obj, 1);

	cost = GET_OBJ_COST(obj) * percent / 100;
	if (OBJ_REINFORCED(obj))
		cost += cost >> 2;
	if (OBJ_ENHANCED(obj))
		cost += cost >> 2;

	return cost;
}

static obj_data *
vendor_resolve_hash(Creature *self, char *obj_str)
{
	obj_data *last_obj = NULL, *cur_obj;
	int num;

	if (*obj_str != '#') {
		slog("Can't happen in vendor_resolve_hash()");
		return NULL;
	}

	num = atoi(obj_str + 1);
	if (num <= 0)
		return NULL;

	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content) {
		if (!last_obj || !same_obj(last_obj, cur_obj))
			if (--num == 0)
				return cur_obj;
		last_obj = cur_obj;
	}

	return cur_obj;
}

static obj_data *
vendor_resolve_name(Creature *self, char *obj_str)
{
	obj_data *cur_obj;

	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content)
		if (namelist_match(obj_str, cur_obj->aliases))
			return cur_obj;

	return NULL;
}

static void
vendor_appraise(Creature *ch, obj_data *obj, Creature *self, ShopData *shop)
{
	char *currency_str, *msg;
	const unsigned long cost = 2000;
	unsigned long amt_carried;
		
	if (shop->currency == 2) {
		do_say(self, tmp_sprintf("%s I don't do appraisals.", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return;
	}
	switch (shop->currency) {
	case 0:	
		amt_carried = GET_GOLD(ch); break;
	case 1:	
		amt_carried = GET_CASH(ch); break;
	default:
		slog("Can't happen at %s:%d", __FILE__, __LINE__);
		amt_carried = 0;
		break;
	}
	if (cost > amt_carried) {
		do_say(self, tmp_sprintf("%s %s",
			GET_NAME(ch), shop->msg_buyerbroke), 0, SCMD_SAY_TO, NULL);
		if (shop->cmd_temper)
			command_interpreter(self, tmp_gsub(shop->cmd_temper, "$N", GET_NAME(ch)));
		return;
	}
	
	switch (shop->currency) {
		case 0:
			GET_GOLD(ch) -= cost;
			GET_GOLD(self) += cost;
			currency_str = "gold";
			break;
		case 1:
			GET_CASH(ch) -= cost;
			GET_CASH(self) += cost;
			currency_str = "creds";
			break;
		case 2:
			GET_QUEST_POINTS(ch) -= cost;
			currency_str = "quest points";
			break;
		default:
			slog("Can't happen at %s:%d", __FILE__, __LINE__);
			currency_str = "-BUGS-";
	}

	msg = tmp_sprintf("%s That will cost you %lu %s.", GET_NAME(ch),
		cost, currency_str);
	do_say(self, msg, 0, SCMD_SAY_TO, NULL);


	if( IS_MAGE(self) ) {
		spell_identify(50, ch, NULL, obj );
	} else if( IS_PHYSIC(self) || IS_CYBORG(self) ) {
		perform_analyze(ch,obj,false);
	} else {
		perform_appraise(ch,obj, 100);
	}
}

static void
vendor_sell(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *obj, *next_obj;
	char *obj_str, *currency_str, *msg;
	int num;
	unsigned long cost, amt_carried;
	bool appraisal_only = false;

	if (!*arg) {
		send_to_char(ch, "What do you wish to buy?\r\n");
		return;
	}

	obj_str = tmp_getword(&arg);
	if (is_abbrev(obj_str, "appraise") || is_abbrev(obj_str, "appraisal")) {
		obj_str = tmp_getword(&arg);
		appraisal_only = true;
	}

	if (is_number(obj_str)) {
		num = atoi(obj_str);
		if (num < 0) {
			do_say(self,
				tmp_sprintf("%s You want to buy a negative amount? Try selling.", GET_NAME(ch)), 
				0, SCMD_SAY_TO, NULL);
			return;
		} else if (num == 0) {
			do_say(self,
				tmp_sprintf("%s You wanted to buy something?", GET_NAME(ch)), 
				0, SCMD_SAY_TO, NULL);
			return;
		}

		obj_str = tmp_getword(&arg);
	} else if (!strcmp(obj_str, "all")) {
		num = -1;
		obj_str = tmp_getword(&arg);
	} else{
		num = 1;
	}

	// Check for hash mark
	if (*obj_str == '#')
		obj = vendor_resolve_hash(self, obj_str);
	else
		obj = vendor_resolve_name(self, obj_str);

	if (!obj) {
		do_say(self,
			tmp_sprintf("%s %s",
			GET_NAME(ch), shop->msg_sell_noobj), 0, SCMD_SAY_TO, NULL);
		return;
	}

	if (appraisal_only) {
		vendor_appraise(ch, obj, self, shop);
		return;
	}

	if (num == -1) {
		if (vendor_is_produced(obj, shop)) {
			do_say(self,
				tmp_sprintf("%s I can make these things all day!",
				GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			return;
		}
		num = vendor_inventory(self, obj, obj);
	}

	if (num > 1) {
		int obj_cnt = vendor_inventory(self, obj, self->carrying);
		if (!vendor_is_produced(obj, shop) && num > obj_cnt) {
			do_say(self,
				tmp_sprintf("%s I only have %d to sell to you.",
				GET_NAME(ch), obj_cnt), 0, SCMD_SAY_TO, NULL);
			num = obj_cnt;
		}
	}

	cost = vendor_get_value(obj, shop->markup);
	switch (shop->currency) {
	case 0:	
		amt_carried = GET_GOLD(ch); break;
	case 1:	
		amt_carried = GET_CASH(ch); break;
	case 2:	
		amt_carried = GET_QUEST_POINTS(ch); break;
	default:
		slog("Can't happen at %s:%d", __FILE__, __LINE__);
		amt_carried = 0;
		break;
	}
	
	if (cost > amt_carried) {
		do_say(self, tmp_sprintf("%s %s",
			GET_NAME(ch), shop->msg_buyerbroke), 0, SCMD_SAY_TO, NULL);
		if (shop->cmd_temper)
			command_interpreter(self, shop->cmd_temper);
		return;
	}
	
	if (cost * num > amt_carried && cost > 0) {
		num = amt_carried / cost;
		do_say(self,
			tmp_sprintf("%s You only have enough to buy %d.",
				GET_NAME(ch), num), 0, SCMD_SAY_TO, NULL);
	}

	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
		do_say(self,
			tmp_sprintf("%s You can't carry any more items.",
				GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
		return;
	}

	if (IS_CARRYING_W(ch) + obj->getWeight() > CAN_CARRY_W(ch)) {
		switch (number(0,2)) {
		case 0:
			do_say(self,
				tmp_sprintf("%s You can't carry any more weight.",
					GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			break;
		case 1:
			do_say(self,
				tmp_sprintf("%s You can't carry that much weight.",
					GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			break;
		case 2:
			do_say(self,
				tmp_sprintf("%s You can carry no more weight.",
					GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			break;
		}
		return;
	}

	if ((IS_CARRYING_N(ch) + num > CAN_CARRY_N(ch) ||
			IS_CARRYING_W(ch) + obj->getWeight() * num > CAN_CARRY_W(ch)) &&
			obj->getWeight() > 0) {
		num = MIN(num, CAN_CARRY_N(ch) - IS_CARRYING_N(ch));
		num = MIN(num, (CAN_CARRY_W(ch) - IS_CARRYING_W(ch))
			/ obj->getWeight());
		do_say(self,
			tmp_sprintf("%s You can only carry %d.",
				GET_NAME(ch), num), 0, SCMD_SAY_TO, NULL);
	}

	switch (shop->currency) {
	case 0:
		GET_GOLD(ch) -= cost * num;
		GET_GOLD(self) += cost * num;
		currency_str = "gold";
		break;
	case 1:
		GET_CASH(ch) -= cost * num;
		GET_CASH(self) += cost * num;
		currency_str = "creds";
		break;
	case 2:
		GET_QUEST_POINTS(ch) -= cost * num;
		currency_str = "quest points";
		break;
	default:
		slog("Can't happen at %s:%d", __FILE__, __LINE__);
		currency_str = "-BUGS-";
	}

	do_say(self,
		tmp_sprintf("%s %s",
			GET_NAME(ch), tmp_sprintf(shop->msg_buy, cost * num)), 0, SCMD_SAY_TO, NULL);
	msg = tmp_sprintf("You sell $p %sto $N for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost * num,
		currency_str);
	act(msg, false, self, obj, ch, TO_CHAR);
	msg = tmp_sprintf("$n sells $p %sto you for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost * num,
		currency_str);
	act(msg, false, self, obj, ch, TO_VICT);
	act("$n sells $p to $N.", false, self, obj, ch, TO_NOTVICT);

	if (vendor_is_produced(obj, shop)) {
		// Load all-new identical items
		while (num--) {
			obj = read_object(GET_OBJ_VNUM(obj));
			obj_to_char(obj, ch);
		}
	} else {
		// Actually move the items from vendor to player
		while (num--) {
			next_obj = obj->next_content;
			obj_from_char(obj);
			obj_to_char(obj, ch);
			obj = next_obj;
		}
	}

}

static void
vendor_buy(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *obj, *next_obj;
	char *obj_str;
	int num = 1;
	unsigned long cost, amt_carried;

	if (shop->currency == 2) {
		do_say(self,
			tmp_sprintf("%s Hey, I only sell stuff.", GET_NAME(ch)), 
			0, SCMD_SAY_TO, NULL);
		return;
	}
		
	if (!*arg) {
		send_to_char(ch, "What do you wish to sell?\r\n");
		return;
	}

	obj_str = tmp_getword(&arg);
	if (is_number(obj_str)) {
		num = atoi(obj_str);
		if (num < 0) {
			do_say(self,
				tmp_sprintf("%s You want to sell a negative amount? Try buying.", GET_NAME(ch)), 
				0, SCMD_SAY_TO, NULL);
			return;
		} else if (num == 0) {
			do_say(self,
				tmp_sprintf("%s You wanted to sell something?", GET_NAME(ch)), 
				0, SCMD_SAY_TO, NULL);
			return;
		}

		obj_str = tmp_getword(&arg);
	} else if (!strcmp(obj_str, "all")) {
		num = -1;
		obj_str = tmp_getword(&arg);
	} else
		num = 1;

	obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
	if (!obj) {
		do_say(self,
			tmp_sprintf("%s %s", GET_NAME(ch), shop->msg_buy_noobj), 
			0, SCMD_SAY_TO, NULL);
		return;
	}

	if (vendor_invalid_buy(self, ch, shop, obj))
		return;

	if (num == -1)
		num = vendor_inventory(ch, obj, obj);

	// We only check inventory after the object selected.  This allows people
	// to sell 5 3.shirt if they have 2 shirts ahead of the one they want to
	// sell
	if (vendor_inventory(ch, obj, obj) < num) {
		send_to_char(ch, "You only have %d of those!\r\n",
			vendor_inventory(ch, obj, ch->carrying));
		return;
	}

	if (vendor_is_produced(obj, shop)) {
		do_say(self, tmp_sprintf("%s I make these.  Why should I buy it back from you?", GET_NAME(ch)),
			0, SCMD_SAY_TO, NULL);
		return;
	}
	cost = vendor_get_value(obj, shop->markdown);
	amt_carried = (shop->currency) ? GET_CASH(self):GET_GOLD(self);

	if (amt_carried < cost) {
		do_say(self, tmp_sprintf("%s %s", GET_NAME(ch), shop->msg_selfbroke),
			0, SCMD_SAY_TO, NULL);
		return;
	}

	if (amt_carried < cost * num && cost > 0) {
		num = amt_carried / cost;
		do_say(self, tmp_sprintf("%s I can only afford to buy %d.",
			GET_NAME(ch), num),
			0, SCMD_SAY_TO, NULL);
	}

	if (vendor_inventory(self, obj, self->carrying) + num > MAX_ITEMS) {
		num = MAX_ITEMS - vendor_inventory(self, obj, self->carrying);
		do_say(self, tmp_sprintf("%s I only want to buy %d.",
			GET_NAME(ch), num),
			0, SCMD_SAY_TO, NULL);
	}

	do_say(self, tmp_sprintf("%s %s", GET_NAME(ch), tmp_sprintf(shop->msg_sell, cost * num)),
		0, SCMD_SAY_TO, NULL);

	transfer_money(self, ch, cost * num, shop->currency, false);

	// We've already verified that they have enough of the item via a
	// call to vendor_inventory(), so we can just blindly transfer objects
	while (num-- && obj) {
		// transfer object
		next_obj = obj->next_content;
		obj_from_char(obj);
		obj_to_char(obj, self);

		// repair object
		if (GET_OBJ_DAM(obj) != -1 && GET_OBJ_MAX_DAM(obj) != -1)
			GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj);

		obj = next_obj;
		}
	ch->saveToXML();
}

char *
vendor_list_obj(Creature *ch, obj_data *obj, int cnt, int idx, int cost)
{
	char *obj_desc;

	obj_desc = obj->name;
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON && GET_OBJ_VAL(obj, 1))
		obj_desc = tmp_strcat(obj_desc, " of ",
			liquid_to_str(GET_OBJ_VAL(obj,2)), NULL);
	if ((GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) &&
			GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
		obj_desc = tmp_strcat(obj_desc, " (partially used)");
	if (OBJ_REINFORCED(obj))
		obj_desc = tmp_strcat(obj_desc, " [reinforced]");
	if (OBJ_ENHANCED(obj))
		obj_desc = tmp_strcat(obj_desc, " |augmented|");
	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
		obj_desc = tmp_strcat(obj_desc, " <broken>");
	if (IS_OBJ_STAT(obj, ITEM_HUM))
		obj_desc = tmp_strcat(obj_desc, " (humming)");
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		obj_desc = tmp_strcat(obj_desc, " (glowing)");
	if (IS_OBJ_STAT(obj, ITEM_INVISIBLE))
		obj_desc = tmp_strcat(obj_desc, " (invisible)");
	if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT))
		obj_desc = tmp_strcat(obj_desc, " (transparent)");
	if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
		if (IS_OBJ_STAT(obj, ITEM_BLESS))	
			obj_desc = tmp_strcat(obj_desc, " (holy aura)");
		if (IS_OBJ_STAT(obj, ITEM_EVIL_BLESS))
			obj_desc = tmp_strcat(obj_desc, " (unholy aura)");
	}

	obj_desc = tmp_capitalize(obj_desc);
	if (cnt < 0)
		return tmp_sprintf(" %2d%s)  %sUnlimited%s   %-48s %6d\r\n",
			idx, CCRED(ch, C_NRM), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			obj_desc, cost);

	return tmp_sprintf(" %2d%s)  %s%5d%s       %-48s %6d\r\n",
		idx, CCRED(ch, C_NRM), CCYEL(ch, C_NRM), cnt, CCNRM(ch, C_NRM),
		obj_desc, cost);
}

static void
vendor_list(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *cur_obj, *last_obj;
	int idx, cnt;
	char *msg;

	if (!self->carrying) {
		do_say(self,
			tmp_sprintf("%s I'm out of stock at the moment", GET_NAME(ch)), 
			0, SCMD_SAY_TO, NULL);
		return;
	}

	switch (shop->currency) {
	case 0:
		msg = "        Gold"; break;
	case 1:
		msg = "       Creds"; break;
	case 2:
		msg = "Quest Points"; break;
	default:
		slog("Can't happen at %s:%d", __FILE__, __LINE__);
		msg = "PleaseReport";
	}

	msg = tmp_strcat(CCCYN(ch, C_NRM),
" ##   Available   Item                                       ", msg,
"\r\n",
"-------------------------------------------------------------------------\r\n"
		, CCNRM(ch, C_NRM), NULL);

	last_obj = NULL;
	cnt = idx = 1;
	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content) {
		if (last_obj) {
			if (same_obj(last_obj, cur_obj)) {
				cnt++;
			} else {
				if (vendor_is_produced(last_obj, shop))
					cnt = -1;
				if (!*arg || namelist_match(arg, last_obj->aliases)) 
					msg = tmp_strcat(msg, vendor_list_obj(ch, last_obj, cnt, idx,
						vendor_get_value(last_obj, shop->markup)));
				cnt = 1;
				idx++;
			}
		}
		last_obj = cur_obj;
	}
	if (last_obj) {
		if (vendor_is_produced(last_obj, shop))
			cnt = -1;
		if (!*arg || namelist_match(arg, last_obj->aliases)) 
			msg = tmp_strcat(msg, vendor_list_obj(ch, last_obj, cnt, idx,
				vendor_get_value(last_obj, shop->markup)));
	}

	act("$n peruses the shop's wares.", false, ch, 0, 0, TO_ROOM);
	page_string(ch->desc, msg);
}


static void
vendor_value(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *obj;
	char *obj_str;
	unsigned long cost;
	char *msg;

	if (shop->currency == 2) {
		do_say(self,
			tmp_sprintf("%s I'm not the buying kind.", GET_NAME(ch)), 
			0, SCMD_SAY_TO, NULL);
		return;
	}
		
	if (!*arg) {
		send_to_char(ch, "What do you wish to value?\r\n");
		return;
	}

	obj_str = tmp_getword(&arg);
	obj = get_obj_in_list_all(ch, obj_str, ch->carrying);
	if (!obj) {
		send_to_char(ch, "You don't seem to have any '%s'.\r\n", obj_str);
		return;
	}

	if (vendor_invalid_buy(self, ch, shop, obj))
		return;

	cost = vendor_get_value(obj, shop->markdown);

	msg = tmp_sprintf("%s I'll give you %lu %s for it!", GET_NAME(ch),
		cost, shop->currency ? "creds":"gold");
	do_say(self, msg, 0, SCMD_SAY_TO, NULL);
}

static void
vendor_revenue(Creature *self, ShopData *shop)
{
	Creature *vkeeper;
	long cur_money, max_money;

	vkeeper = real_mobile_proto(GET_MOB_VNUM(self));
	max_money = (shop->currency) ? GET_CASH(vkeeper):GET_GOLD(vkeeper);
	cur_money = (shop->currency) ? GET_CASH(self):GET_GOLD(self);
	if (cur_money >= max_money)
		return;
	if (shop->currency)
		GET_CASH(self) = MIN(max_money, GET_CASH(self) + shop->revenue);
	else
		GET_GOLD(self) = MIN(max_money, GET_GOLD(self) + shop->revenue);
	return;
}

char *
vendor_parse_param(Creature *self, char *param, ShopData *shop, int *err_line)
{
	char *line, *param_key;
	char *err = NULL;
	int val, lineno = 0;

	// Initialize default values
	shop->room = -1;
	shop->msg_denied = "I'm not doing business with YOU!";
	shop->msg_badobj = "I don't buy that sort of thing.";
	shop->msg_selfbroke = "Sorry, but I don't have the cash.";
	shop->msg_buyerbroke = "You don't have enough money to buy this!";
	shop->msg_sell_noobj= "Sorry, but I don't carry that item.";
	shop->msg_buy_noobj= "You don't have that item!";
	shop->msg_buy = "Here you go.";
	shop->msg_sell = "There you go.";
	shop->msg_closed = "Come back later!";
	shop->cmd_temper = NULL;
	shop->markdown = 70;
	shop->markup = 120;
	shop->currency = false;
	shop->revenue = 0;
	shop->steal_ok = false;
	shop->attack_ok = false;
	shop->func = NULL;

	while ((line = tmp_getline(&param)) != NULL) {
		lineno++;
		if (shop->reaction.add_reaction(line))
			continue;

		param_key = tmp_getword(&line);
		if (!strcmp(param_key, "room")) {
			shop->room = atol(line);
		} else if (!strcmp(param_key, "produce")) {
			val = atoi(line);
			if (val <= 0 || !real_object_proto(val)) {
				err = "non-existant produced item";
				break;
			}
			shop->item_list.push_back(atoi(line));
		} else if (!strcmp(param_key, "accept")) {
			if (strcmp(line, "all")) {
				val = search_block(line, item_types, 0);
				if (val <= 0) {
					err = "an invalid accept line";
					break;
				}
			} else
				val = 0;
			shop->item_types.push_back( 1 << 8 | val);
		} else if (!strcmp(param_key, "refuse")) {
			if (strcmp(line, "all")) {
				val = search_block(line, item_types, 0);
				if (val <= 0) {
					err = "an invalid accept line";
					break;
				}
			} else
				val = 0;
			shop->item_types.push_back( 0 << 8 | val);
		} else if (!strcmp(param_key, "denied-msg")) {
			shop->msg_denied = line;
		} else if (!strcmp(param_key, "keeper-broke-msg")) {
			shop->msg_selfbroke= line;
		} else if (!strcmp(param_key, "buyer-broke-msg")) {
			shop->msg_buyerbroke = line;
		} else if (!strcmp(param_key, "buy-msg")) {
			shop->msg_buy = line;
		} else if (!strcmp(param_key, "sell-msg")) {
			shop->msg_sell = line;
		} else if (!strcmp(param_key, "closed-msg")) {
			shop->msg_closed = line;
		} else if (!strcmp(param_key, "no-buy-msg")) {
			shop->msg_badobj = line;
		} else if (!strcmp(param_key, "sell-noobj-msg")) {
			shop->msg_sell_noobj= line;
		} else if (!strcmp(param_key, "buy-noobj-msg")) {
			shop->msg_buy_noobj= line;
		} else if (!strcmp(param_key, "temper-cmd")) {
			shop->cmd_temper = line;
		} else if (!strcmp(param_key, "closed-hours")) {
			ShopTime time;

			time.start = atoi(tmp_getword(&line));
			time.end = atoi(tmp_getword(&line));
			if (time.start < 0 || time.start > 24)
				err = "an out of bounds closing hour";
			else if (time.end < 0 || time.end > 24)
				err = "an out of bounds opening hour";
			else
				shop->closed_hours.push_back(time);
		} else if (!strcmp(param_key, "markup")) {
			shop->markup= atoi(line);
			if (shop->markup <= 0 ||  shop->markup > 1000) {
				err = "an invalid markup";
				break;
			}
		} else if (!strcmp(param_key, "markdown")) {
			shop->markdown= atoi(line);
			if (shop->markdown <= 0 ||  shop->markdown > 1000) {
				err = "an invalid markdown";
				break;
			}
		} else if (!strcmp(param_key, "revenue")) {
			shop->revenue= atoi(line);
			if (shop->revenue < 0) {
				err = "a negative revenue";
				break;
			}
		} else if (!strcmp(param_key, "currency")) {
			if (is_abbrev(line, "past") || is_abbrev(line, "gold"))
				shop->currency = 0;
			else if (is_abbrev(line, "future") || is_abbrev(line, "cash"))
				shop->currency = 1;
			else if (is_abbrev(line, "qp") || is_abbrev(line, "quest"))
				shop->currency = 2;
			else {
				err = "invalid currency";
				break;
			}
		} else if (!strcmp(param_key, "steal-ok")) {
			shop->steal_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "attack-ok")) {
			shop->attack_ok = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "special")) {
			val = find_spec_index_arg(line);
			if (val == -1)
				err = "invalid special";
			else
				shop->func = spec_list[val].func;
		} else {
			err = "invalid directive";
			break;
		}
	}

	if (err_line)
		*err_line = (err) ? lineno:-1;

	return err;
}

SPECIAL(vendor)
{
	Creature *self = (Creature *)me;
	char *config, *err;
	int err_line;
	ShopData shop;

	config = GET_MOB_PARAM(self);
	if (!config)
		return 0;

	err = vendor_parse_param(self, config, &shop, &err_line);
	if (shop.func && shop.func(ch, me, cmd, argument, spec_mode))
		return 1;

	if (spec_mode == SPECIAL_RESET) {
		vendor_revenue(self, &shop);
		return 0;
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;	

	if (!(CMD_IS("buy")   || CMD_IS("sell")  || CMD_IS("list")  || 
		  CMD_IS("value") || CMD_IS("offer") || CMD_IS("steal"))) {
		return 0;
	}

	if (err) {
		// Specparam error
		if (IS_PC(ch)) {
			if (IS_IMMORT(ch))
				perform_tell(self, ch, tmp_sprintf(
					"I have %s in line %d of my specparam", err, err_line));
			else {
				mudlog(LVL_IMMORT, NRM, true,
					"ERR: Mobile %d has %s in line %d of specparam",
					GET_MOB_VNUM(self), err, err_line);
				do_say(self, tmp_sprintf(
					"%s Sorry.  I'm broken, but a god has already been notified.",
					GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			}
		}
		return true;
	}

	if (CMD_IS("steal")) {
		if (shop.steal_ok && GET_LEVEL(ch) < LVL_IMMORT)
			return false;
		do_gen_comm(self, tmp_sprintf("%s is a bloody thief!", GET_NAME(ch)),
			0, SCMD_SHOUT, 0);
		return true;
	}
	
	if (!can_see_creature(self, ch)) {
		do_say(self, "Show yourself if you want to do business with me!",
			0, 0, 0);
		return true;
	}

	if (shop.room != -1 && shop.room != self->in_room->number) {
		do_say(self, tmp_sprintf("%s Catch me when I'm in my store.",
			GET_NAME(ch)), 0, SCMD_SAY_TO, 0);
		return true;
	}

	if (!shop.closed_hours.empty()) {
		vector<ShopTime>::iterator shop_time;
		struct time_info_data local_time;

		set_local_time(self->in_room->zone, &local_time);
		for (shop_time = shop.closed_hours.begin();shop_time != shop.closed_hours.end();shop_time++)
			if (local_time.hours >= shop_time->start &&
					local_time.hours < shop_time->end ) {
				do_say(self, tmp_sprintf("%s %s", GET_NAME(ch), shop.msg_closed), 0, SCMD_SAY_TO, 0);
				return true;
			}
	}

	if (shop.reaction.react(ch) != ALLOW) {
		do_say(self, tmp_sprintf("%s %s", GET_NAME(ch), shop.msg_denied),
			0, SCMD_SAY_TO, 0);
		return true;
	}

	if (CMD_IS("buy")) {
		vendor_sell(ch, argument, self, &shop);
	} else if (CMD_IS("sell")) {
		vendor_buy(ch, argument, self, &shop);
	} else if (CMD_IS("list")) {
		vendor_list(ch, argument, self, &shop);
	} else if (CMD_IS("value") || CMD_IS("offer")) {
		vendor_value(ch, argument, self, &shop);
	} else {
		mudlog(LVL_IMPL, CMP, true, "Can't happen at %s:%d", __FILE__,
			__LINE__);
	}
	
	return true;
}
