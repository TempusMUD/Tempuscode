//
// File: enhancer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "guns.h"

#define ENHANCE_BUY 1
#define ENHANCE_OFF 2

SPECIAL(enhancer)
{

	struct Creature *keeper = (struct Creature *)me;
	struct obj_data *obj = NULL;
	char *args = NULL, arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int cmd_type = 0, cost = 0;

	if (!cmd || (!CMD_IS("buy") && !CMD_IS("offer")))
		return 0;

	if (CMD_IS("buy"))
		cmd_type = ENHANCE_BUY;
	else
		cmd_type = ENHANCE_OFF;

	args = two_arguments(argument, arg1, arg2);

	if (!*arg1 || strncmp(arg1, "enhancement", 13)) {
		perform_tell(keeper, ch, "Usage: buy/offer enhancement <item>");
		return 1;
	}

	if (!*arg2) {
		sprintf(buf2, "%s what item?", cmd_type == ENHANCE_BUY ?
			"Perform structural enhancement on" :
			"Get an offer on structural enhancement for");
		perform_tell(keeper, ch, buf2);
		return 1;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		sprintf(buf2, "You don't have %s '%s'.", AN(arg2), arg2);
		perform_tell(keeper, ch, buf2);
		return 1;
	}

	if (OBJ_ENHANCED(obj)) {
		perform_tell(keeper, ch,
			"This item has already been structurally enhanced.");
		perform_tell(keeper, ch, "I cannot enhance it further.");
		return 1;
	}

	if (!(IS_OBJ_TYPE(obj, ITEM_WEAPON) || IS_OBJ_TYPE(obj, ITEM_ENERGY_GUN))) {
		perform_tell(keeper, ch, "I can only enhance weapons and energy guns.");
		return 1;
	}

	cost = GET_OBJ_COST(obj);
    cost += (cost*ch->getCostModifier(keeper))/100;
    
	sprintf(buf2, "It will cost you %d %s to have %s enhanced.",
		cost, ch->in_room->zone->time_frame == TIME_ELECTRO ? "credits" :
		"coins", obj->name);
	perform_tell(keeper, ch, buf2);

	if (cmd_type == ENHANCE_OFF) {
		act("$n gets an offer on enhancement for $p.", FALSE, ch, obj, 0,
			TO_ROOM);
		return 1;
	}

	if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
		if (GET_CASH(ch) < cost) {
			perform_tell(keeper, ch, "You don't have enough credits!");
			return 1;
		} else
			GET_CASH(ch) -= cost;
	} else {
		if (GET_GOLD(ch) < cost) {
			perform_tell(keeper, ch, "You don't have enough coins!");
			return 1;
		} else
			GET_GOLD(ch) -= cost;
	}

	act("$n takes $p and disappears into the back room for a while.\r\n",
		FALSE, keeper, obj, ch, TO_VICT);
	act("$e returns shortly and presents you with the finished product",
		FALSE, keeper, obj, ch, TO_VICT);
	act("$n takes $p from $N and disappears into the back room for a while.",
		FALSE, keeper, obj, ch, TO_NOTVICT);

	SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_ENHANCED);
	if (((GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) >> 1) < 21) {
		if (((GET_OBJ_VAL(obj, 2) + 1) >> 1) >= GET_OBJ_VAL(obj, 1))
			GET_OBJ_VAL(obj, 1) += 1;
		else
			GET_OBJ_VAL(obj, 2) += 2;
	} else {
		if (((GET_OBJ_VAL(obj, 2) + 1) >> 1) >= GET_OBJ_VAL(obj, 1))
			GET_OBJ_VAL(obj, 2) += 2;
		else
			GET_OBJ_VAL(obj, 1) += 1;
	}
    if (IS_ENERGY_GUN(obj)) { //energy guns have their usage cost increased randomly
        GET_OBJ_VAL(obj, 0) += number(0,GET_OBJ_VAL(obj, 0)); 
    }
	WAIT_STATE(ch, 5 RL_SEC);
	ch->saveToXML();

	return 1;
}
