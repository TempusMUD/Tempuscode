#ifdef HAS_CONFIG_H
#include "config.h"
#endif


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
void perform_analyze( Creature *ch, obj_data *obj, bool checklev );
void perform_appraise( Creature *ch, obj_data *obj, int skill_lvl);

// From cityguard.cc
void call_for_help(Creature *ch, Creature *attacker);

// From vendor.cc
bool same_obj(obj_data *obj1, obj_data *obj2);
obj_data *vendor_resolve_hash(Creature *self, char *obj_str);
obj_data *vendor_resolve_name(Creature *self, char *obj_str);
void vendor_appraise(Creature *ch, obj_data *obj, Creature *self, ShopData *shop);

static void
tattooist_show_pos(Creature * me, Creature * ch, obj_data * obj)
{
	int pos;
	bool not_first = false;

	strcpy(buf, "You can have tattoos in these positions: ");
	for (pos = 0; wear_tattoopos[pos][0] != '\n'; pos++)
		if (!ILLEGAL_TATTOOPOS(pos) && CAN_WEAR(obj, wear_bitvectors[pos])) {
			if (not_first)
				strcat(buf, ", ");
			else
				not_first = true;

			strcat(buf, wear_tattoopos[pos]);
		}

	perform_tell(me, ch, buf);
}

static unsigned long
tattooist_get_value(obj_data *obj, int percent, int costModifier)
{
	unsigned long cost;
    
	cost = GET_OBJ_COST(obj) * percent / 100;
    cost += (costModifier*(int)cost)/100;
    
	return cost;
}


static void
tattooist_sell(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *obj;
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

	// Check for hash mark
	if (*obj_str == '#')
		obj = vendor_resolve_hash(self, obj_str);
	else
		obj = vendor_resolve_name(self, obj_str);

	if (!obj) {
		perform_say_to(self, ch, shop->msg_sell_noobj);
		return;
	}

    char *pos_str = tmp_getword(&arg);
    if (!pos_str[0]) {
        perform_say_to(self, ch, "Where did you want that tattoo?");
        return;
    }
    int pos = search_block(pos_str, wear_tattoopos, 0);
    if (pos < 0 || ILLEGAL_TATTOOPOS(pos)) {
		msg = tmp_sprintf("'%s' isn't a valid position.", pos_str);
		perform_tell(self, ch, msg);
		tattooist_show_pos(self, ch, obj);
		return;
	}

	if (!CAN_WEAR(obj, wear_bitvectors[pos])) {
		msg = tmp_sprintf("%s cannot be inked there.",
			obj->name);
		perform_tell(self, ch, msg);
		tattooist_show_pos(self, ch, obj);
		return;
	}
    if (GET_TATTOO(ch, pos)) {
        perform_tell(self, ch, "You already have a tattoo inscribed there.");
        return;
    }
        
    cost = GET_OBJ_COST(obj);
    cost += ch->getCostModifier(self) * cost / 100;
	switch (shop->currency) {
	case 0:	
		amt_carried = GET_GOLD(ch); break;
	case 1:	
		amt_carried = GET_CASH(ch); break;
	case 2:	
		amt_carried = ch->account->get_quest_points(); break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		amt_carried = 0;
		break;
	}
	
	if (cost > amt_carried) {
		perform_say_to(self, ch, shop->msg_buyerbroke);
		if (shop->cmd_temper)
			command_interpreter(self, shop->cmd_temper);
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
		ch->account->set_quest_points(ch->account->get_quest_points() - (cost * num));
		currency_str = "quest points";
		break;
	default:
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		currency_str = "-BUGS-";
	}

	perform_say_to(self, ch, tmp_sprintf(shop->msg_buy, cost * num));
	msg = tmp_sprintf("You sell $p %sto $N for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost,
		currency_str);
	act(msg, false, self, obj, ch, TO_CHAR);
	msg = tmp_sprintf("$n sells $p %sto you for %lu %s.",
		((num == 1) ? "":tmp_sprintf("(x%d) ", num)),
		cost,
		currency_str);
	act(msg, false, self, obj, ch, TO_VICT);
	act("$n sells $p to $N.", false, self, obj, ch, TO_NOTVICT);

    // Load all-new item
    obj = read_object(GET_OBJ_VNUM(obj));
    equip_char(ch, obj, pos, EQUIP_TATTOO);
}

char *
tattooist_list_obj(Creature *ch, obj_data *obj, int idx, int cost)
{
	char *obj_desc;

	obj_desc = obj->name;
	if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
		if (IS_OBJ_STAT(obj, ITEM_BLESS))	
			obj_desc = tmp_strcat(obj_desc, " (holy aura)");
		if (IS_OBJ_STAT(obj, ITEM_DAMNED))
			obj_desc = tmp_strcat(obj_desc, " (unholy aura)");
	}

	obj_desc = tmp_capitalize(obj_desc);
    return tmp_sprintf(" %2d%s)  %s%-48s %6d\r\n",
                       idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM), obj_desc, cost);
}

static void
tattooist_list(Creature *ch, char *arg, Creature *self, ShopData *shop)
{
	obj_data *cur_obj, *last_obj;
	int idx, cnt;
	char *msg;
    unsigned long cost;
    
	if (!self->carrying) {
		perform_say_to(self, ch, "I'm out of stock at the moment.");
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
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		msg = "PleaseReport";
	}

	msg = tmp_strcat(CCCYN(ch, C_NRM),
" ##   Item                                       ", msg,
"\r\n",
"-------------------------------------------------------------------------\r\n"
		, CCNRM(ch, C_NRM), NULL);

	cnt = idx = 1;
	for (cur_obj = self->carrying;cur_obj;cur_obj = cur_obj->next_content) {
        if (!*arg || namelist_match(arg, last_obj->aliases)) {
            cost = tattooist_get_value(cur_obj, shop->markup, ch->getCostModifier(self));
            msg = tmp_strcat(msg, tattooist_list_obj(ch, cur_obj, idx, cost));
        }
	}

	act("$n peruses the tattooists offerings.", false, ch, 0, 0, TO_ROOM);
	page_string(ch->desc, msg);
}

SPECIAL(tattooist)
{
	Creature *self = (Creature *)me;
	char *config, *err = NULL;
	int err_line;
	ShopData *shop;

	config = GET_MOB_PARAM(self);
	if (!config)
		return 0;

	shop = (ShopData *)self->mob_specials.func_data;
	if (!shop) {
		CREATE(shop, ShopData, 1);
		err = vendor_parse_param(self, config, shop, &err_line);
		self->mob_specials.func_data = shop;
	}

	if (shop->func &&
			shop->func != tattooist &&
			shop->func(ch, me, cmd, argument, spec_mode))
		return 1;

	if (spec_mode == SPECIAL_TICK) {
        Creature *target = self->findRandomCombat();
		if (target && shop->call_for_help && !number(0, 4)) {
			call_for_help(self, target);
			return 1;
		}
		return 0;
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;	

	if (!(CMD_IS("buy") || CMD_IS("list") || CMD_IS("steal")))
		return 0;

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
				perform_say_to(self, ch, "Sorry.  I'm broken, but a god has already been notified.");
			}
		}
		return true;
	}

	if (CMD_IS("steal")) {
		if (GET_LEVEL(ch) < LVL_IMMORT)
			return false;
		do_gen_comm(self,
                    tmp_capitalize(tmp_sprintf("%s is a bloody thief!",
                                               GET_NAME(ch))),
                    0, SCMD_SHOUT, 0);
		return true;
	}
	
	if (!can_see_creature(self, ch)) {
		perform_say(self, "yell", "Show yourself if you want to do business with me!");
		return true;
	}

	if (shop->room != -1 && shop->room != self->in_room->number) {
		perform_say_to(self, ch, "Catch me when I'm in my store.");
		return true;
	}

	if (!shop->closed_hours.empty()) {
		vector<ShopTime>::iterator shop_time;
		struct time_info_data local_time;

		set_local_time(self->in_room->zone, &local_time);
		for (shop_time = shop->closed_hours.begin();shop_time != shop->closed_hours.end();shop_time++)
			if (local_time.hours >= shop_time->start &&
					local_time.hours < shop_time->end ) {
				perform_say_to(self, ch, shop->msg_closed);
				return true;
			}
	}

	if (shop->reaction.react(ch) != ALLOW) {
		perform_say_to(self, ch, shop->msg_denied);
		return true;
	}

	if (!IS_EVIL(self) && IS_CRIMINAL(ch)) {
		perform_say_to(self, ch, "I don't deal with CRIMINALS.");
		return true;
	}

    if (CMD_IS("list"))
		tattooist_list(ch, argument, self, shop);

	if (CMD_IS("buy")) {
		tattooist_sell(ch, argument, self, shop);
	} else {
		mudlog(LVL_IMPL, CMP, true, "Can't happen at %s:%d", __FILE__,
			__LINE__);
	}
	
	return true;
}
