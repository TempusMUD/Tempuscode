//
// File: act.cyborg.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "char_class.h"
#include "guns.h"
#include "bomb.h"
#include "fight.h"
#include "house.h"
#include "utils.h"
#include "accstr.h"

/* extern variables */
extern struct room_data *world;
extern struct spell_info_type spell_info[];
extern struct obj_data *object_list;

int check_mob_reaction(struct Creature *ch, struct Creature *vict);
void look_at_target(struct Creature *ch, char *arg);
const char *obj_cond(struct obj_data *obj);
const char *obj_cond_color(struct obj_data *obj, struct Creature *ch);

ACMD(do_not_here);
ACMD(do_examine);

/********************************************************************/

const char *microchip_types[] = {
	"none",
	"skill",
	"affects",
	"\n"
};
const char *interface_types[] = {
	"none",
	"power",
	"chips",
	"\n"
};

const char *component_names[][3] = {

	{"NONE", "NONE", "NONE"},
	{"primary transformer", "analog converter", "neural net"},
	{"spinal cable", "internal gyroscope", "cerebral servo"},
	{"hydraulic line", "high speed controller", "interface chip"},
	{"receptor enhancer", "receptor enhancer", "receptor enhancer"},
	{"primary capacitor", "primary capacitor", "math coprocessor"},
	{"cooling pump", "ventilator fan", "ventilation unit"},
	{"system coordinator", "parallel processor", "parallel processor"},
	{"kinetic drive unit", "kinetic drive unit", "power converter"},
	{"hard disk", "hard disk", "hard disk"},

	/******* When adding new components, be sure to increase the value
    ******** of NUM_COMPS in vehicle.h ******************************/

	{"\n", "\n", "\n"}

};

const char *borg_subchar_class_names[] = {
	"power", "speed", "mentant"
};


const char *engine_flags[] = {
	"operational",
	"parked",
	"running",
	"lights",
	"petrol",
	"electric",
	"magic",
	"\n"
};

const char *vehicle_types[] = {
	"atv",
	"roadcar",
	"skycar",
	"boat",
	"spaceship",
	"\n"
};

const char *
get_component_name(int comp, int sub_class)
{
	int idx = 0;

	if (sub_class < 0 || sub_class > 2)
		sub_class = 0;
	while (idx < comp && component_names[idx][0][0] != '\n')
		idx++;
	
	return component_names[idx][sub_class];
}

int
max_component_dam(struct Creature *ch)
{

	int max;
	max = GET_LEVEL(ch) * 80;
	max *= GET_CON(ch);
	max *= (1 + GET_REMORT_GEN(ch));

	max = MAX(1, max);

	return (max);
}

void
perform_recharge(struct Creature *ch, struct obj_data *battery,
	struct Creature *vict, struct obj_data *engine, int amount)
{
	int wait = 0;
	if (battery) {
		if (!amount)
			amount = RECH_RATE(battery);

		if ((CUR_ENERGY(battery) - amount) < 0)
			amount = CUR_ENERGY(battery);

		if (GET_OBJ_TYPE(battery) == ITEM_BATTERY
			&& COST_UNIT(battery) * amount > GET_CASH(ch)) {
			amount = GET_CASH(ch) / COST_UNIT(battery);

			if (!amount) {
				send_to_char(ch, 
					"You do not have the cash to pay for the energy.\r\n");
				return;
			}
		}
	} else {
		if (!amount)
			amount =
				MIN(GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3), GET_MOVE(ch));
		else
			amount = MIN(amount, GET_MOVE(ch));
	}

	if (!engine && !vict) {
		errlog(" NULL engine and vict in perform_recharge");
		return;
	}

	if (vict) {
		if (GET_MOVE(vict) + amount > GET_MAX_MOVE(vict))
			amount = GET_MAX_MOVE(vict) - GET_MOVE(vict);

		GET_MOVE(vict) = MIN(GET_MAX_MOVE(vict), GET_MOVE(vict) + amount);

		if (battery)
			CUR_ENERGY(battery) = MAX(0, CUR_ENERGY(battery) - amount);
		else
			GET_MOVE(ch) -= amount;

	} else {  /** recharging an engine **/

		if ((CUR_ENERGY(engine) + amount) > MAX_ENERGY(engine))
			amount = MAX_ENERGY(engine) - CUR_ENERGY(engine);
		CUR_ENERGY(engine) =
			MIN(MAX_ENERGY(engine), CUR_ENERGY(engine) + amount);

		if (battery)
			CUR_ENERGY(battery) = MAX(0, CUR_ENERGY(battery) - amount);
		else
			GET_MOVE(ch) -= amount;
	}

	send_to_char(ch,
		"%s>>>        ENERGY TRANSFER         <<<%s\r\n"
		"From:  %s%20s%s\r\n"
		"To:    %s%20s%s\r\n\r\n"
		"Transfer Amount:       %s%5d Units%s\r\n"
		"Energy Level(TARGET):  %s%5d Units%s\r\n"
		"Energy Level(SOURCE):  %s%5d Units%s\r\n",
		QGRN, QNRM,
		QCYN, battery ? battery->name : "self", QNRM,
		QCYN, vict ? GET_NAME(vict) : engine->name, QNRM,
		QCYN, amount, QNRM,
		QCYN, vict ? GET_MOVE(vict) : CUR_ENERGY(engine), QNRM,
		QCYN, battery ? CUR_ENERGY(battery) : GET_MOVE(ch), QNRM);
	if (battery) {
		if (GET_OBJ_TYPE(battery) == ITEM_BATTERY && COST_UNIT(battery)) {
			send_to_char(ch, "Your cost: %d credits.\r\n",
				amount * COST_UNIT(battery));
			GET_CASH(ch) -= amount * COST_UNIT(battery);
		}
	}
	if ((battery && CUR_ENERGY(battery) <= 0) ||
		(!battery && GET_MOVE(ch) <= 0))
		send_to_char(ch, "STATUS: %sSOURCE DEPLETED%s\r\n", QRED, QNRM);

	if ((vict && GET_MOVE(vict) == GET_MAX_MOVE(vict)) ||
		(engine && CUR_ENERGY(engine) == MAX_ENERGY(engine)))
		send_to_char(ch, "STATUS: %sTARGET NOW FULLY ENERGIZED%s\r\n",
			QRED, QNRM);

	wait = 10 + MIN((amount / 5), 90);

	if (CHECK_SKILL(ch, SKILL_OVERDRAIN) > 50) {
		wait -= CHECK_SKILL(ch, SKILL_OVERDRAIN) / 200 * wait;
	}
	WAIT_STATE(ch, wait);

	if (vict && vict != ch) {
		if (battery) {
			act("$n recharges you with $p.",
				FALSE, ch, battery, vict, TO_VICT | TO_SLEEP);
			act("$n recharges $N with $p.",
				FALSE, ch, battery, vict, TO_NOTVICT);
		} else {
			act("$n recharges you from $s internal power.",
				FALSE, ch, battery, vict, TO_VICT | TO_SLEEP);
			act("$n recharges $N from $s internal power.",
				FALSE, ch, battery, vict, TO_NOTVICT);
		}
	} else if (ch == vict) {
		if (battery)
			act("$n recharges $mself from $p.",
				FALSE, ch, battery, vict, TO_ROOM);
		else
			send_to_room("BING!  Something weird just happened.\r\n",
				ch->in_room);
	} else if (engine) {
		if (battery)
			act("$n recharges $p from $P.", TRUE, ch, engine, battery,
				TO_ROOM);
		else
			act("$n recharges $p from $s internal supply.",
				TRUE, ch, engine, vict, TO_ROOM);
	}
	if (battery && GET_OBJ_TYPE(battery) == ITEM_BATTERY)
		amount -= RECH_RATE(battery);
	if (amount < 0) {
		amount = 0;
	}
	amount -= amount * CHECK_SKILL(ch, SKILL_OVERDRAIN) / 125;
	if (amount < 0) {
		amount = 0;
	}

	if ((battery && amount) &&
		(GET_OBJ_TYPE(battery) == ITEM_BATTERY
			|| GET_OBJ_TYPE(battery) == ITEM_DEVICE)) {
		send_to_char(ch, "%sERROR: %s damaged during transfer!\r\n", QRED,
			battery->name);
		damage_eq(ch, battery, amount);
	}
}


ACMD(do_hotwire)
{
	send_to_char(ch, "Hotwire what?\r\n");
	return;
}


ACMD(do_recharge)
{
	char *arg1, *arg2, *arg3, *arg4;
	struct obj_data *target = NULL, *battery = NULL;
	struct Creature *vict = NULL;
	int i;

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);
	arg3 = tmp_getword(&argument);
	arg4 = tmp_getword(&argument);

	if ((!*arg1 || ch == get_char_room_vis(ch, arg1)) && IS_CYBORG(ch)) {
		// Find the battery
		if (!(strcmp("internal", arg2))) {
			battery = get_object_in_equip_vis(ch, arg3, ch->implants, &i);
		}
		if (battery == NULL
			&& !(battery = get_obj_in_list_vis(ch, arg2, ch->carrying))
			&& !(battery =
				get_object_in_equip_vis(ch, arg2, ch->equipment, &i))
			&& !(battery =
				get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
			send_to_char(ch, "You need a battery to do that.\r\n");
			return;
		}
		if (!IS_BATTERY(battery)) {
			act("$p is not a battery.", FALSE, ch, battery, 0, TO_CHAR);
			return;
		}
		if (CUR_ENERGY(battery) <= 0) {
			act("$p is depleted of energy.", FALSE, ch, battery, 0, TO_CHAR);
			return;
		}
		if (IS_IMPLANT(battery) && *arg4 &&
			battery != get_object_in_equip_vis(ch, arg4, ch->implants, &i)) {
			act("ERROR: $p not installed properly.", FALSE, ch, battery, 0,
				TO_CHAR);
			return;
		}
		if (GET_MOVE(ch) == GET_MAX_MOVE(ch)) {
			send_to_char(ch, "You are already operating at maximum energy.\r\n");
			return;
		}
		perform_recharge(ch, battery, ch, NULL, 0);
		return;
	}

	if (*arg1 && !str_cmp(arg1, "internal")) {
		if (!*arg2 || !*arg3) {
			send_to_char(ch, 
				"USAGE:  Recharge internal <component> energy_source\r\n");
			return;
		}

		if (!(target = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg2),
				arg2);
			return;
		}
		if (*arg3 && !strcmp(arg3, "internal")) {
			if (!*arg4) {
				send_to_char(ch, "Recharge from an internal what?\r\n");
				return;
			} else if (!(battery =
					get_object_in_equip_vis(ch, arg4, ch->implants, &i))) {
				send_to_char(ch, "You are not implanted with %s '%s'.\r\n",
					AN(arg4), arg4);
				return;
			}
		} else if (strncmp(arg3, "self", 4) &&
			!(battery = get_obj_in_list_vis(ch, arg3, ch->carrying)) &&
			!(battery = get_object_in_equip_vis(ch, arg3, ch->equipment, &i))
			&& !(battery =
				get_obj_in_list_vis(ch, arg3, ch->in_room->contents))) {
			send_to_char(ch, "You can't find any '%s' to recharge from.\r\n",
				arg3);
			return;
		}

		if (battery) {
			if (!IS_BATTERY(battery)) {
				act("$p is not a battery.", FALSE, ch, battery, 0, TO_CHAR);
				return;
			}

			if (CUR_ENERGY(battery) <= 0) {
				act("$p is depleted of energy.", FALSE, ch, battery, 0,
					TO_CHAR);
				return;
			}

			if (IS_IMPLANT(battery) && *arg4 &&
				battery != get_object_in_equip_vis(ch, arg4, ch->implants,
					&i)) {
				act("ERROR: $p not installed properly.", FALSE, ch, battery, 0,
					TO_CHAR);
				return;
			}
		} else {
			if (!IS_CYBORG(ch)) {
				send_to_char(ch, "You cannot recharge things from yourself.\r\n");
				return;
			}
			if (GET_MOVE(ch) <= 0) {
				send_to_char(ch, "You do not have any energy to transfer.\r\n");
				return;
			}
		}

		if (!IS_CYBORG(ch)) {
			for (i = 0; i < NUM_WEARS; i++)
				if ((GET_IMPLANT(ch, i) && IS_INTERFACE(GET_IMPLANT(ch, i)) &&
						INTERFACE_TYPE(GET_IMPLANT(ch, i)) == INTERFACE_POWER)
					|| (GET_EQ(ch, i) && IS_INTERFACE(GET_EQ(ch, i))
						&& INTERFACE_TYPE(GET_EQ(ch, i)) == INTERFACE_POWER))
					break;

			if (i >= NUM_WEARS) {
				send_to_char(ch, 
					"You are not using an appropriate power interface.\r\n");
				return;
			}
		}

		if (IS_ENERGY_GUN(target)) {
			send_to_char(ch, "You can only restore the energy gun cells.\r\n");
			return;
		}
		if (!RECHARGABLE(target)) {
			send_to_char(ch, "You can't recharge that!\r\n");
			return;
		}
		if (CUR_ENERGY(target) >= MAX_ENERGY(target)) {
			act("$p is already fully energized.", FALSE, ch, target, 0,
				TO_CHAR);
			return;
		}


		perform_recharge(ch, battery, NULL, target, 0);
		return;
	}

	if (!*arg1 || !*arg2) {
		send_to_char(ch, "USAGE:  Recharge <component> energy_source\r\n");
		return;
	}

	if (!(target = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		!(target = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
		!(target = get_obj_in_list_vis(ch, arg1, ch->in_room->contents)) &&
		!(vict = get_char_room_vis(ch, arg1))) {
		send_to_char(ch, "You can't find %s '%s' here to recharge.\r\n", AN(arg1),
			arg1);
		return;
	}

	if (*arg2 && !strcmp(arg2, "internal")) {
		if (!*arg3) {
			send_to_char(ch, "Recharge from an internal what?\r\n");
			return;
		} else if (!(battery =
				get_object_in_equip_vis(ch, arg3, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg3),
				arg3);
			return;
		}
	} else if (strncmp(arg2, "self", 4) &&
		!(battery = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
		!(battery = get_object_in_equip_vis(ch, arg2, ch->equipment, &i)) &&
		!(battery = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
		send_to_char(ch, "You can't find %s '%s' here to recharge from.\r\n",
			AN(arg2), arg2);
		return;
	}

	if (battery) {
		if (!IS_BATTERY(battery)) {
			act("$p is not a battery.", FALSE, ch, battery, 0, TO_CHAR);
			return;
		}

		if (CUR_ENERGY(battery) <= 0) {
			act("$p is depleted of energy.", FALSE, ch, battery, 0, TO_CHAR);
			return;
		}
	} else {
		if (!IS_CYBORG(ch)) {
			send_to_char(ch, "You cannot recharge things from yourself.\r\n");
			return;
		}
		if (GET_MOVE(ch) <= 0) {
			send_to_char(ch, "You do not have any energy to transfer.\r\n");
			return;
		}
	}

	if (vict) {
		if (!IS_CYBORG(vict)) {
			act("You cannot recharge $M.", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		if (GET_MOVE(vict) >= GET_MAX_MOVE(vict)) {
			act("$N is fully charged.", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		perform_recharge(ch, battery, vict, NULL, 0);
		return;
	}

	if (IS_VEHICLE(target)) {
		if (!target->contains || !IS_ENGINE(target->contains))
			act("$p is not operational.", FALSE, ch, target, 0, TO_CHAR);
		else if (CUR_ENERGY(target->contains) == MAX_ENERGY(target->contains))
			act("$p is fully energized.", FALSE, ch, target->contains, 0,
				TO_CHAR);
		else {
			perform_recharge(ch, battery, NULL, target->contains, 0);
		}
		return;
	}

	if (IS_ENERGY_GUN(target)) {
		send_to_char(ch, "You can only restore the energy gun cells.\r\n");
		return;
	}
	if (!RECHARGABLE(target)) {
		send_to_char(ch, "You can't recharge that!\r\n");
		return;
	}

	if (CUR_ENERGY(target) == MAX_ENERGY(target))
		act("$p is fully energized.", FALSE, ch, target, 0, TO_CHAR);
	else {
		perform_recharge(ch, battery, NULL, target, 0);
	}
	return;
}


//
// perform_cyborg_activate is the routine to handle cyborg skill activation
// it is normally called only from do_activate
//       

void
perform_cyborg_activate(Creature *ch, int mode, int subcmd)
{
	struct affected_type af[3];
	char *to_room[2], *to_char[2];
	int opposite_mode = 0;

	if (!CHECK_SKILL(ch, mode))
		send_to_char(ch, "You do not have this program in memory.\r\n");
	else if (CHECK_SKILL(ch, mode) < 40)
		send_to_char(ch, "Partial installation insufficient for operation.\r\n");
	else {

		to_room[0] = NULL;
		to_room[1] = NULL;
		to_char[0] = NULL;
		to_char[1] = NULL;

		af[0].type = mode;
		af[0].is_instant = 0;
		af[0].bitvector = 0;
		af[0].duration = -1;
		af[0].location = APPLY_NONE;
		af[0].modifier = 0;
		af[0].aff_index = 1;
		af[0].level = GET_LEVEL(ch);
        af[0].owner = ch->getIdNum();

		af[1].type = 0;
		af[1].is_instant = 0;
		af[1].bitvector = 0;
		af[1].duration = -1;
		af[1].location = APPLY_NONE;
		af[1].modifier = 0;
		af[1].aff_index = 1;
		af[1].level = GET_LEVEL(ch);
        af[1].owner = ch->getIdNum();

		af[2].type = 0;
		af[2].is_instant = 0;
		af[2].bitvector = 0;
		af[2].duration = -1;
		af[2].location = APPLY_NONE;
		af[2].modifier = 0;
		af[2].aff_index = 1;
		af[2].level = GET_LEVEL(ch);
        af[2].owner = ch->getIdNum();

		switch (mode) {
		case SKILL_ADRENAL_MAXIMIZER:
			af[0].bitvector = 0;
			af[0].aff_index = 1;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -30;

			af[1].type = mode;
			af[1].bitvector = AFF_ADRENALINE;
			af[1].aff_index = 1;
			af[1].location = APPLY_SPEED;
			af[1].modifier =
				1 + (GET_LEVEL(ch) >> 3) + (GET_REMORT_GEN(ch) >> 1);

			to_char[1] = "Shukutei adrenal maximizations enabled.\r\n";
			to_char[0] = "Shukutei adrenal maximizations disabled.\r\n";
			break;

			//    case SKILL_OPTIMMUNAL_RESP:
			//
			//        break;

		case SKILL_ENERGY_FIELD:
			af[0].bitvector = AFF2_ENERGY_FIELD;
			af[0].aff_index = 2;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -40;

			to_char[1] = "Energy fields activated.\r\n";
			to_room[1] = "A field of energy hums to life about $n's body.";

			to_char[0] = "Energy field deactivated.\r\n";
			to_room[0] = "$n's energy field dissipates.";

			break;
		case SKILL_REFLEX_BOOST:
			af[0].bitvector = AFF2_HASTE;
			af[0].aff_index = 2;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -40;

			to_char[1] = "Activating reflex boosters.\r\n";
			to_char[0] = "Deactivating reflex boosters.\r\n";
			break;
		case SKILL_MELEE_COMBAT_TAC:
			af[0].bitvector = 0;
			af[0].aff_index = 0;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -30;

			to_char[1] = "Activating melee combat tactics.\r\n";
			to_char[0] = "Deactivating melee combat tactics.\r\n";
			break;
		case SKILL_POWER_BOOST:
			af[0].bitvector = 0;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -40;

			af[1].type = mode;
			af[1].location = APPLY_STR;
			af[1].modifier = 1 + (GET_LEVEL(ch) >> 4);

			to_char[1] = "Power boost enabled.\r\n";
			to_room[1] = "$n boosts $s power levels!  Look out!";

			to_char[0] = "Unboosting power.\r\n";
			to_room[0] = "$n reduces power.";

			break;
		case SKILL_MOTION_SENSOR:
			af[0].bitvector = 0;
			af[0].aff_index = 1;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -30;

			to_char[1] = "Activating motion sensors.\r\n";
			to_char[0] = "Shutting down motion sensor.\r\n";

			break;
		case SKILL_DAMAGE_CONTROL:
			af[0].bitvector = AFF3_DAMAGE_CONTROL;
			af[0].aff_index = 3;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -30;

			to_char[1] = "Activating damage control systems.\r\n";
			to_char[0] = "Terminating damage control process.\r\n";

			break;
		case SKILL_HYPERSCAN:
			af[0].bitvector = 0;
			af[0].aff_index = 1;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -10;

			to_char[1] = "Activating hyperscanning device.\r\n";
			to_char[0] = "Shutting down hyperscanning device.\r\n";

			break;
		case SKILL_STASIS:
			if (subcmd) {			/************ activate ****************/
				if (ch->getPosition() >= POS_FLYING)
					send_to_char(ch, "Go into stasis while flying?!?!?\r\n");
				else {
					TOGGLE_BIT(AFF3_FLAGS(ch), AFF3_STASIS);
					ch->setPosition(POS_SLEEPING);
					WAIT_STATE(ch, PULSE_VIOLENCE * 5);
					send_to_char(ch, 
						"Entering static state.  Halting system processes.\r\n");
					act("$n lies down and enters a static state.", FALSE, ch,
						0, 0, TO_ROOM);
				}
			} else {		   /************ deactivate ****************/
				send_to_char(ch, "An error has occurred.\r\n");
			}
			return;
			break;

		case SKILL_RADIONEGATION:
			af[0].location = APPLY_MOVE;
			af[0].modifier = -15;

			to_char[1] = "Radionegation device activated.\r\n";
			to_char[0] = "Radionegation device shutting down.\r\n";
			to_room[1] = "$n starts humming loudly.\r\n";
			to_room[0] = "$n stops humming.\r\n";

			break;

		case SKILL_NEURAL_BRIDGING:	// Cogenic Neural Bridging
			af[0].bitvector = 0;
			af[0].aff_index = 0;
			af[0].location = APPLY_MOVE;
			af[0].modifier = -37;

			to_char[1] = "Activating cogenic neural bridge.\r\n";
			to_char[0] = "Deactivating cogenic neural bridge.\r\n";
			break;
		case SKILL_OFFENSIVE_POS:	// Offensive Posturing
			af[0].bitvector = 0;
			af[0].aff_index = 0;
			af[0].location = APPLY_AC;
			af[0].modifier = (GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch)));

			af[1].bitvector = 0;
			af[1].aff_index = 0;
			af[1].location = APPLY_DAMROLL;
			af[1].modifier = (GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch))) / 10;
			af[1].type = SKILL_OFFENSIVE_POS;

			af[2].bitvector = 0;
			af[2].aff_index = 0;
			af[2].location = APPLY_HITROLL;
			af[2].modifier = (GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch))) / 7;
			af[2].type = SKILL_OFFENSIVE_POS;

			to_char[1] = "Offensive posturing enabled.\r\n";
			to_char[0] = "Offensive posturing disabled.\r\n";
			opposite_mode = SKILL_DEFENSIVE_POS;
			break;
		case SKILL_DEFENSIVE_POS:	// Defensive Posturing
			af[0].bitvector = 0;
			af[0].aff_index = 0;
			af[0].location = APPLY_AC;
			af[0].modifier = -(GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch)));

			af[1].bitvector = 0;
			af[1].aff_index = 0;
			af[1].location = APPLY_DAMROLL;
			af[1].modifier = -(GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch))) / 10;
			af[1].type = SKILL_DEFENSIVE_POS;

			af[2].bitvector = 0;
			af[2].aff_index = 0;
			af[2].location = APPLY_HITROLL;
			af[2].modifier = -(GET_LEVEL(ch) + (2 * GET_REMORT_GEN(ch))) / 7;
			af[2].type = SKILL_DEFENSIVE_POS;

			to_char[1] = "Defensive posturing enabled.\r\n";
			to_char[0] = "Defensive posturing disabled.\r\n";
			opposite_mode = SKILL_OFFENSIVE_POS;
			break;
		case SKILL_NANITE_RECONSTRUCTION:
			af[0].location = APPLY_MOVE;
			af[0].modifier = -300;
			to_char[1] = "Nanite reconstruction in progress.\r\n";
			to_char[0] = "Nanite reconstruction halted.\r\n";
			break;
		default:
			send_to_char(ch, "ERROR: Unknown mode occurred in switch.\r\n");
			return;
			break;
		}

		if ((subcmd && affected_by_spell(ch, mode)) ||
			(subcmd && affected_by_spell(ch, opposite_mode)) ||
			(!subcmd && !affected_by_spell(ch, mode))) {
			if (affected_by_spell(ch, opposite_mode))
				mode = opposite_mode;
			send_to_char(ch, "%sERROR:%s %s %s %s activated.\r\n", CCCYN(ch,
					C_NRM), CCNRM(ch, C_NRM), spell_to_str(mode),
				ISARE(spell_to_str(mode)), subcmd ? "already" : "not currently");
			return;
		}

		if (subcmd) {

			if ((af[0].location == APPLY_MOVE
					&& (GET_MOVE(ch) + af[0].modifier) < 0)
				|| (af[1].location == APPLY_MOVE
					&& (GET_MOVE(ch) + af[1].modifier) < 0)) {
				send_to_char(ch,
					"%sERROR:%s Energy levels too low to activate %s.\r\n",
					CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), spell_to_str(mode));
				return;
			}

			affect_join(ch, &af[0], 0, FALSE, 1, FALSE);
			if (af[1].type)
				affect_join(ch, &af[1], 0, FALSE, 1, FALSE);

			if (af[2].type)
				affect_join(ch, &af[2], 0, FALSE, 1, FALSE);

			if (to_char[1])
				send_to_char(ch, to_char[1]);
			if (to_room[1])
				act(to_room[1], FALSE, ch, 0, 0, TO_ROOM);

			if (mode == SKILL_ENERGY_FIELD &&
				(SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
					SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN ||
					SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
					SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)) {
				CreatureList::iterator it = ch->in_room->people.begin();
				for (; it != ch->in_room->people.end(); ++it) {
					if (*it != ch) {
						damage(ch, *it, dice(4, GET_LEVEL(ch)),
							SKILL_ENERGY_FIELD, -1);
                        WAIT_STATE(ch, 1 RL_SEC);
                    }
				}
				send_to_char(ch, 
					"DANGER: Hazardous short detected!!  Energy fields shutting down.\r\n");
				affect_from_char(ch, mode);

			}

		} else {
			affect_from_char(ch, mode);
			if (to_char[0])
				send_to_char(ch, to_char[0]);
			if (to_room[0])
				act(to_room[0], FALSE, ch, 0, 0, TO_ROOM);

		}
	}

}

//
// do_activate is the frontend for handling activation of both objects
// and cyborg skill/states
//

ACMD(do_activate)
{

	int mode = 0, mass, i;
	struct obj_data *obj = NULL;
	struct room_data *targ_room = NULL;
	bool internal = false;
	char *arg;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "%s what?\r\n", CMD_NAME);
		return;
	}
	if (IS_CYBORG(ch)) {
		for (i = MAX_SPELLS; i < MAX_SKILLS; i++)
			if (!strncasecmp(argument, spell_to_str(i), strlen(argument) - 1) &&
				IS_SET(spell_info[i].routines, CYB_ACTIVATE)) {
				mode = i;
				break;
			}
	}

	if (!mode) {		  /*** not a skill activation ***/
		arg = tmp_getword(&argument);
		if (!*arg) {
			send_to_char(ch, "Activate what?\r\n");
			return;
		}
		if (!strncmp(arg, "internal", 7)) {
			internal = true;
			arg = tmp_getword(&argument);
			if (!*arg) {
				send_to_char(ch, "Which implant?\r\n");
				return;
			}
			if (!(obj = get_object_in_equip_all(ch, arg, ch->implants, &i))) {
				send_to_char(ch, "You are not implanted with %s '%s'.\r\n",
					AN(arg), arg);
				return;
			}
		} else if (!(obj = get_object_in_equip_vis(ch, arg, ch->equipment, &i))
				&& !(obj = get_obj_in_list_vis(ch, arg, ch->carrying))
				&& !(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			send_to_char(ch, "You don't seem to have %s '%s'.\r\n", AN(arg),
				arg);
			return;
		}

		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_DEVICE:
			if (!subcmd) {
				if (!ENGINE_STATE(obj))
					act("$p is already switched off.", FALSE, ch, obj, 0,
						TO_CHAR);
				else {
					act(tmp_sprintf("You deactivate $p%s.",
							internal ? " (internal)" : ""),
						false, ch, obj, 0, TO_CHAR);
					act(tmp_sprintf("$n deactivates $p%s.",
							internal ? " (internal)" : ""),
						true, ch, obj, 0, TO_ROOM);

					if (obj_gives_affects(obj, ch, internal)) {
						for (i = 0; i < MAX_OBJ_AFFECT; i++)
							affect_modify(obj->worn_by,
								obj->affected[i].location,
								obj->affected[i].modifier, 0, 0, FALSE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[0], 1, FALSE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[1], 2, FALSE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[2], 3, FALSE);

						ENGINE_STATE(obj) = 0;
						affect_total(ch);
					}

					ENGINE_STATE(obj) = 0;
				}
			} else {
				if (ENGINE_STATE(obj))
					act("$p is already switched on.", FALSE, ch, obj, 0,
						TO_CHAR);
				else if (CUR_ENERGY(obj) < USE_RATE(obj))
					act("The energy levels of $p are too low.", FALSE, ch, obj,
						0, TO_CHAR);
				else {
					act(tmp_sprintf("You activate $p%s.",
							internal ? " (internal)" : ""),
						false, ch, obj, 0, TO_CHAR);
					act(tmp_sprintf("$n activates $p%s.",
							internal ? " (internal)" : ""),
						true, ch, obj, 0, TO_ROOM);

					if (obj->action_desc && ch == obj->worn_by)
						act(obj->action_desc, FALSE, ch, obj, 0,
							TO_CHAR);
					CUR_ENERGY(obj) -= USE_RATE(obj);
					ENGINE_STATE(obj) = 1;
					if (obj_gives_affects(obj, ch, internal)) {
						for (i = 0; i < MAX_OBJ_AFFECT; i++)
							affect_modify(obj->worn_by,
								obj->affected[i].location,
								obj->affected[i].modifier, 0, 0, TRUE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[0], 1, TRUE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[1], 2, TRUE);
						affect_modify(obj->worn_by, 0, 0,
							obj->obj_flags.bitvector[2], 3, TRUE);
						affect_total(obj->worn_by);

					}
				}
			}
			break;
			return;

		case ITEM_TRANSPORTER:
			if (CHECK_SKILL(ch, SKILL_ELECTRONICS) < number(30, 100)) {
				send_to_char(ch, "You cannot figure out how.\r\n");
				return;
			}
			if (!subcmd) {
				send_to_char(ch, "You cannot deactivate it.\r\n");
				return;
			}

			/* charmed? */
			if (IS_AFFECTED(ch, AFF_CHARM) && ch->master &&
				ch->in_room == ch->master->in_room) {
				send_to_char(ch, 
					"The thought of leaving your master makes you weep.\r\n");
				if (IS_UNDEAD(ch))
					act("$n makes a hollow moaning sound.", FALSE, ch, 0, 0,
						TO_ROOM);
				else
					act("$n looks like $e wants to use $p.", FALSE, ch, obj, 0,
						TO_ROOM);
				return;
			}

			mass = GET_WEIGHT(ch) + IS_CARRYING_W(ch) + IS_WEARING_W(ch);
			mass >>= 4;

			if (CUR_ENERGY(obj) < mass)
				act("$p lacks the energy to transport you and your gear.",
					FALSE, ch, obj, 0, TO_CHAR);
			else if ((targ_room = real_room(TRANS_TO_ROOM(obj))) == NULL)
				act("$p is not tuned to a real spacetime location.",
					FALSE, ch, obj, 0, TO_CHAR);
			else if (ROOM_FLAGGED(targ_room, ROOM_DEATH))
				send_to_char(ch, "A warning indicator blinks on:\r\n"
					"Transporter is tuned to a deadly area.\r\n");
			else if ((ROOM_FLAGGED(targ_room, ROOM_GODROOM | ROOM_NORECALL |
						ROOM_NOTEL |
						ROOM_NOMAGIC) && GET_LEVEL(ch) < LVL_GRGOD))
				send_to_char(ch, "Transporter ERROR: Unable to transport.\r\n");
			else if (targ_room->zone->plane != ch->in_room->zone->plane)
				send_to_char(ch, "An indicator flashes on:\r\n"
					"Transporter destination plane does not intersect current spacial field.\r\n");
			else if ((ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)
					|| (targ_room->zone != ch->in_room->zone
						&& (ZONE_FLAGGED(targ_room->zone, ZONE_ISOLATED)
							|| ZONE_FLAGGED(ch->in_room->zone,
								ZONE_ISOLATED))))
				&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
				act("You flip a switch on $p...\r\n"
					"Only to be slammed down by an invisible force!!!", FALSE,
					ch, obj, 0, TO_CHAR);
				act("$n flips a switch on $p...\r\n"
					"And is slammed down by an invisible force!!!", FALSE, ch,
					obj, 0, TO_ROOM);
			} else {
				CUR_ENERGY(obj) -= mass;

				send_to_char(ch, "You flip a switch and disappear.\r\n");
				act("$n flips a switch on $p and fades out of existence.",
					TRUE, ch, obj, 0, TO_ROOM);

				char_from_room(ch);
				char_to_room(ch, targ_room);

				send_to_char(ch, 
					"A buzzing fills your ears as you materialize...\r\n");
				act("You hear a buzzing sound as $n fades into existence.",
					FALSE, ch, 0, 0, TO_ROOM);
				look_at_room(ch, ch->in_room, 0);

				if (ROOM_FLAGGED(ch->in_room, ROOM_NULL_MAGIC) &&
					(GET_LEVEL(ch) < LVL_ETERNAL ||
						(IS_CYBORG(ch) && !IS_MAGE(ch) &&
							!IS_CLERIC(ch) && !IS_KNIGHT(ch)
							&& !IS_RANGER(ch)))) {
					if (ch->affected) {
						send_to_char(ch, 
							"You are dazed by a blinding flash inside your"
							" brain!\r\nYou feel different...\r\n");
						act("Light flashes from behind $n's eyes.", FALSE, ch,
							0, 0, TO_ROOM);
						while (ch->affected)
							affect_remove(ch, ch->affected);

						WAIT_STATE(ch, PULSE_VIOLENCE);
					}
				}
				gain_skill_prof(ch, SKILL_ELECTRONICS);
				WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			}
			break;

		case ITEM_SCUBA_MASK:
			if (!obj->aux_obj || GET_OBJ_TYPE(obj->aux_obj) != ITEM_SCUBA_TANK) {
				act("$p needs to be attached to an air tank first.",
					FALSE, ch, obj, 0, TO_CHAR);
				return;
			}
			if (!subcmd) {
				if (!GET_OBJ_VAL(obj, 0))
					act("$p is already switched off.", FALSE, ch, obj, 0,
						TO_CHAR);
				else {
					act("You close the air valve on $p.", FALSE, ch, obj, 0,
						TO_CHAR);
					act("$n adjusts an air valve on $p.", TRUE, ch, obj, 0,
						TO_ROOM);
					GET_OBJ_VAL(obj, 0) = 0;
				}
			} else {
				if (GET_OBJ_VAL(obj, 0))
					act("$p is already switched on.", FALSE, ch, obj, 0,
						TO_CHAR);
				else {
					act("You open the air valve on $p.", FALSE, ch, obj, 0,
						TO_CHAR);
					act("$n adjusts an air valve on $p.", TRUE, ch, obj, 0,
						TO_ROOM);
					GET_OBJ_VAL(obj, 0) = 1;
				}
			}
			break;

		case ITEM_BOMB:
			if (!obj->contains || !IS_FUSE(obj->contains)) {
				act("$p is not fitted with a fuse.", FALSE, ch, obj, 0,
					TO_CHAR);
			} else if (!subcmd) {
				if (!FUSE_STATE(obj->contains))
					act("$p has not yet been activated.",
						FALSE, ch, obj->contains, 0, TO_CHAR);
				else {
					act("You deactivate $p.", FALSE, ch, obj, 0, TO_CHAR);
					FUSE_STATE(obj->contains) = 0;
				}
			} else {
				if (FUSE_STATE(obj->contains))
					act("$p is already live!",
						FALSE, ch, obj->contains, 0, TO_CHAR);
				else if (FUSE_IS_BURN(obj->contains))
					act("$p is fused with $P -- a burning fuse.",
						FALSE, ch, obj, obj->contains, TO_CHAR);
				else {
					act("You activate $P... $p is now live.",
						FALSE, ch, obj, obj->contains, TO_CHAR);
					FUSE_STATE(obj->contains) = 1;
					BOMB_IDNUM(obj) = GET_IDNUM(ch);
				}
			}
			break;

		case ITEM_DETONATOR:
			if (!obj->aux_obj)
				act("$p is not tuned to any explosives.", FALSE, ch, obj, 0,
					TO_CHAR);
			else if (!IS_BOMB(obj->aux_obj))
				send_to_char(ch, "Activate detonator error.  Please report.\r\n");
			else if (obj != obj->aux_obj->aux_obj)
				send_to_char(ch, "Bomb error.  Please report\r\n");
			else if (!obj->aux_obj->contains ||
				!IS_FUSE(obj->aux_obj->contains) ||
				!FUSE_IS_REMOTE(obj->aux_obj->contains))
				act("$P is not fused with a remote device.",
					FALSE, ch, obj, obj->aux_obj, TO_CHAR);
			else if (subcmd) {
				if (!FUSE_STATE(obj->aux_obj->contains))
					act("$P's fuse is not active.",
						FALSE, ch, obj, obj->aux_obj, TO_CHAR);
				else {
					act("$n activates $p.", FALSE, ch, obj, 0, TO_CHAR);
					detonate_bomb(obj->aux_obj);
				}
			} else {
				act("$n deactivates $p.", FALSE, ch, obj, 0, TO_CHAR);
				send_to_char(ch, "ok.\r\n");
				obj->aux_obj->aux_obj = NULL;
				obj->aux_obj = NULL;
			}
			break;

		case ITEM_COMMUNICATOR:
			if (subcmd) {
				if (ENGINE_STATE(obj)) {
					act("$p is already switched on.", FALSE, ch, obj, 0,
						TO_CHAR);
				} else if (CUR_ENERGY(obj) == 0) {
					act("$p is depleted of energy.", FALSE, ch, obj, 0,
						TO_CHAR);
				} else {
					act("$n switches $p on.", TRUE, ch, obj, 0, TO_ROOM);
					act("You switch $p on.", TRUE, ch, obj, 0, TO_CHAR);
					ENGINE_STATE(obj) = 1;
				}
			} else {
				if (!ENGINE_STATE(obj)) {
					act("$p is already switched off.", FALSE, ch, obj, 0,
						TO_CHAR);
				} else {
					act("$n switches $p off.", TRUE, ch, obj, 0, TO_ROOM);
					act("You switch $p off.", TRUE, ch, obj, 0, TO_CHAR);
					ENGINE_STATE(obj) = 0;
				}
			}
			break;
		default:
			send_to_char(ch, "You cannot figure out how.\r\n");
			break;
		}
		return;
	}

	perform_cyborg_activate(ch, mode, subcmd);


}

ACMD(do_cyborg_reboot)
{

	if (!IS_CYBORG(ch)) {
		send_to_char(ch, "Uh, sure...\r\n");
		return;
	}
	send_to_char(ch, "Systems shutting down...\r\n"
		"Sending all processes the TERM signal.\r\n"
		"Unmounting filesystems.... done.\r\n" "Reboot successful.\r\n");
	if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 10) {
		send_to_char(ch, "Fastboot script enabled... %d %% efficiency.\r\n",
			CHECK_SKILL(ch, SKILL_FASTBOOT));
	}

	act("$n begins a reboot sequence and shuts down.", FALSE, ch, 0, 0,
		TO_ROOM);

	if (ch->numCombatants())
		ch->removeAllCombat();

	ch->setPosition(POS_SLEEPING);

	if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 110) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 100) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 90) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 4);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 80) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 5);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 70) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 6);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 60) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 7);
	} else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 50) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 8);
	} else
		WAIT_STATE(ch, PULSE_VIOLENCE * 10);

	if (ch->affected)
		send_to_char(ch, "Purging system....\r\n");

	while (ch->affected && GET_MANA(ch)) {
		affect_remove(ch, ch->affected);
		GET_MANA(ch) = MAX(GET_MANA(ch) - 10, 0);
	}

	if (GET_COND(ch, DRUNK) > 0)
		GET_COND(ch, DRUNK) = 0;

	gain_skill_prof(ch, SKILL_FASTBOOT);
}


ACMD(do_self_destruct)
{

	int countdown = 0;
	skip_spaces(&argument);

	if (!IS_CYBORG(ch) || GET_SKILL(ch, SKILL_SELF_DESTRUCT) < 50) {
		send_to_char(ch, "You explode.\r\n");
		act("$n self destructs, spraying you with an awful mess!",
			FALSE, ch, 0, 0, TO_ROOM);
	} else if (!*argument) {
		send_to_char(ch, 
			"You must provide a countdown time, in 3-second pulses.\r\n");
	} else if (!is_number(argument)) {
		if (!str_cmp(argument, "abort")) {
			if (!AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
				send_to_char(ch, 
					"Self-destruct sequence not currently initiated.\r\n");
			} else {
				REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
				send_to_char(ch, "Sending process the KILL Signal.\r\n"
					"Self-destruct sequence terminated with %d pulses remaining.\r\n",
					MEDITATE_TIMER(ch));
				MEDITATE_TIMER(ch) = 0;
			}
		} else
			send_to_char(ch, "Self destruct what??\r\n");
	} else if ((countdown = atoi(argument)) < 0 || countdown > 100) {
		send_to_char(ch, "Selfdestruct countdown argument out of range.\r\n");
	} else {
		if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
			send_to_char(ch, "Self-destruct sequence currently underway.\r\n"
				"Use 'selfdestruct abort' to abort process!\r\n");
		} else if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else {
			send_to_char(ch,
				"Self-destruct sequence initiated at countdown level %d.\r\n",
				countdown);
			SET_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
			MEDITATE_TIMER(ch) = countdown;
			if (!countdown)
				engage_self_destruct(ch);
		}
	}
}

ACMD(do_bioscan)
{

	int count = 0;

	if (!IS_CYBORG(ch)) {
		send_to_char(ch, "You are not equipped with bio-scanners.\r\n");
		return;
	}
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((((can_see_creature(ch, (*it)) || GET_INVIS_LVL((*it)) < GET_LEVEL(ch)) &&
					(CHECK_SKILL(ch, SKILL_BIOSCAN) > number(30, 100) ||
						affected_by_spell(ch, SKILL_HYPERSCAN)))
				|| ch == (*it)) && LIFE_FORM((*it)))
			count++;
	}

	send_to_char(ch, "%sBIOSCAN:%s %d life form%s detected in room.\r\n",
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), count, count > 1 ? "s" : "");
	act("$n scans the room for lifeforms.", FALSE, ch, 0, 0, TO_ROOM);

	if (count > number(3, 5))
		gain_skill_prof(ch, SKILL_BIOSCAN);

}

ACMD(do_discharge)
{

	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	int percent, prob, amount, dam;
	int feedback = 0;
	int tolerance = 0;
	int level = 0;
	int wait = 0;
	char *arg1, *arg2;

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);

	if (!IS_CYBORG(ch) && GET_LEVEL(ch) < LVL_DEMI) {
		send_to_char(ch, "You discharge some smelly gas.\r\n");
		act("$n discharges some smelly gas.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (CHECK_SKILL(ch, SKILL_DISCHARGE) < 20) {
		send_to_char(ch, "You are unable to discharge.\r\n");
		return;
	}

	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
			SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN ||
			SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
			SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM ||
			SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_WATER)) {
		send_to_char(ch, 
			"ERROR: Systems halted a process that would have caused a short circuit.\r\n");
		return;
	}

	if (!*arg1) {
		send_to_char(ch, "Usage: discharge <energy discharge amount> <victim>\r\n");
		return;
	}

	if (!(vict = get_char_room_vis(ch, arg2)) &&
		!(ovict = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
		if (ch->numCombatants()) {
			vict = ch->findRandomCombat();
		} else {
			send_to_char(ch, "Discharge into who?\r\n");
			return;
		}
	}

	if (!is_number(arg1)) {
		send_to_char(ch, "The discharge amount must be a number.\r\n");
		return;
	}
	amount = atoi(arg1);

	if (amount > GET_MOVE(ch)) {
		send_to_char(ch, 
			"ERROR: Energy levels too low for requested discharge.\r\n");
		return;
	}

	if (amount < 0) {
		send_to_char(ch, "Discharge into who?\r\n");
		mudlog(LVL_GRGOD, NRM, true, "%s neg-discharge %d %s at %d",
			GET_NAME(ch), amount,
			vict ? GET_NAME(vict) : ovict->name,
			ch->in_room->number);
		return;
	}
	if (vict == ch) {
		send_to_char(ch, "Let's not try that shall we...\r\n");
		return;
	}

	GET_MOVE(ch) -= amount;

	level = GET_LEVEL(ch);
	level += GET_REMORT_GEN(ch);
	// Tolerance is the amount they can safely discharge.
	tolerance = 5 + (ch->getLevelBonus(SKILL_DISCHARGE) / 4);


	if (amount > tolerance) {
		if (amount > (tolerance * 2)) {
			send_to_char(ch, 
				"ERROR: Discharge amount far exceeds acceptable parameters. Aborted.\r\n.");
			return;
		}
		// Give them some idea of how much they went overboard by.
		send_to_char(ch, "WARNING: Voltage tolerance exceeded by %d%%.\r\n",
			((amount * 100) - (tolerance * 100)) / tolerance);
		// Amount of component damage delt to cyborg.
		feedback = dice(amount - tolerance, 4);
		feedback *= max_component_dam(ch) / 100;

		// Random debug messages.
		if (PRF2_FLAGGED(ch, PRF2_DEBUG))
			send_to_char(ch,
				"%s[DISCHARGE] %s tolerance:%d   amount:%d   feedback: %d%s\r\n",
				CCCYN(ch, C_NRM), GET_NAME(ch), tolerance, amount, feedback,
				CCNRM(ch, C_NRM));
		if (vict && PRF2_FLAGGED(vict, PRF2_DEBUG))
			send_to_char(vict,
				"%s[DISCHARGE] %s tolerance:%d   amount:%d   feedback: %d%s\r\n",
				CCCYN(vict, C_NRM), GET_NAME(ch), tolerance, amount, feedback,
				CCNRM(vict, C_NRM));

		if (GET_TOT_DAM(ch) + feedback >= max_component_dam(ch))
			GET_TOT_DAM(ch) = max_component_dam(ch);
		else
			GET_TOT_DAM(ch) += feedback;
		send_to_char(ch, "WARNING: System components damaged by discharge!\r\n");

		if (damage(ch, ch, dice(amount - tolerance, 10), TYPE_OVERLOAD, -1)) {
			return;
		}

		if (GET_TOT_DAM(ch) == 0 || GET_TOT_DAM(ch) == max_component_dam(ch)) {
			send_to_char(ch, "ERROR: Component failure. Discharge failed.\r\n");
			return;
		}

	}

	if (ovict) {
		act("$n blasts $p with a stream of pure energy!!",
			FALSE, ch, ovict, 0, TO_ROOM);
		act("You blast $p with a stream of pure energy!!",
			FALSE, ch, ovict, 0, TO_CHAR);

		dam = amount * (level + dice(GET_INT(ch), 4));
		damage_eq(ch, ovict, dam);
		return;
	}


	if (!ch->isOkToAttack(vict))
		return;

	percent = ((10 - (GET_AC(vict) / 10)) >> 1) + number(1, 91);

	prob = CHECK_SKILL(ch, SKILL_DISCHARGE);

    dam = dice(amount * 3, 16 + (ch->getLevelBonus(SKILL_DISCHARGE) / 10));

	wait = (1 + amount / 5) RL_SEC;
	WAIT_STATE(ch, wait);

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_DISCHARGE, -1);
	} else {
		gain_skill_prof(ch, SKILL_DISCHARGE);
		damage(ch, vict, dam, SKILL_DISCHARGE, -1);
	}
}


ACMD(do_tune)
{
	struct obj_data *obj = NULL, *obj2 = NULL;
	char *arg1, *arg2;
	struct room_data *to_room = NULL;
	sh_int count = 0;
	int i;
	bool internal = false;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "Tune what?\r\n");
		return;
	}

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);

	if (!strncmp(arg1, "internal", 8)) {
		internal = true;
		strcpy(arg1, arg2);
		if (!(obj = get_object_in_equip_vis(ch, arg1, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with '%s'.\r\n", arg1);
			return;
		}
		arg2 = tmp_getword(&argument);

	} else {

		if (!(obj = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
			!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
			!(obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
			send_to_char(ch, "You cannot find %s '%s' to tune.\r\n", AN(arg1),
				arg1);
			return;
		}
	}

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_TRANSPORTER:
		if (CHECK_SKILL(ch, SKILL_ELECTRONICS) < number(10, 50))
			send_to_char(ch, "You can't figure it out.\r\n");
		else if (!GET_OBJ_VAL(obj, 3))
			act("$p is not tunable.", FALSE, ch, obj, 0, TO_CHAR);
		else if (!CUR_ENERGY(obj))
			act("$p is depleted of energy.", FALSE, ch, obj, 0, TO_CHAR);
		else {
			CUR_ENERGY(obj) = MAX(0, CUR_ENERGY(obj) - 10);
			if (!CUR_ENERGY(obj))
				act("$p buzzes briefly and falls silent.", FALSE, ch, obj, 0,
					TO_CHAR);
			else {
				if ((CHECK_SKILL(ch, SKILL_ELECTRONICS) + GET_INT(ch)) <
					number(50, 100)) {
					do {
						to_room =
							real_room(number(ch->in_room->zone->number * 100,
								ch->in_room->zone->top));
						count++;
					} while (count < 1000 &&
						(!to_room || to_room->zone != ch->in_room->zone ||
							ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NOMAGIC |
								ROOM_NOTEL | ROOM_NORECALL | ROOM_DEATH) ||
							(ROOM_FLAGGED(to_room, ROOM_HOUSE) &&
								!House_can_enter(ch, to_room->number))));
				}
				if (to_room == NULL)
					TRANS_TO_ROOM(obj) = ch->in_room->number;

				act("You succesfully tune $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n turns some knobs on $p, which blinks and whines.",
					FALSE, ch, obj, 0, TO_ROOM);
				WAIT_STATE(ch, PULSE_VIOLENCE);
			}
		}
		break;

	case ITEM_DETONATOR:
		if (CHECK_SKILL(ch, SKILL_ELECTRONICS) < number(10, 50) ||
			CHECK_SKILL(ch, SKILL_DEMOLITIONS) < number(10, 50))
			send_to_char(ch, "You can't figure it out.\r\n");
		else if (!*arg2)
			act("Tune $p to what bomb?", FALSE, ch, obj, 0, TO_CHAR);
		else {
			if (!(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
				!(obj2 =
					get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
				send_to_char(ch, "You don't see %s '%s' here.\r\n", AN(arg2),
					arg2);
				return;
			}
			if (!IS_BOMB(obj2))
				send_to_char(ch, "You can only tune detonators to explosives.\r\n");
			if (!obj2->contains || !IS_FUSE(obj2->contains)
				|| !FUSE_IS_REMOTE(obj2->contains))
				act("$P is not fused with a remote activator.", FALSE, ch, obj,
					obj2, TO_CHAR);
			else if (obj->aux_obj) {
				if (obj->aux_obj == obj2->aux_obj)
					act("$p is already tuned to $P.", FALSE, ch, obj, obj2,
						TO_CHAR);
				else
					act("$p is already tuned to something.", FALSE, ch, obj,
						obj2, TO_CHAR);
			} else if (obj2->aux_obj)
				act("Another detonator is already tuned to $P.",
					FALSE, ch, obj, obj2, TO_CHAR);
			else {
				obj->aux_obj = obj2;
				obj2->aux_obj = obj;
				act("You tune $p to $P.", FALSE, ch, obj, obj2, TO_CHAR);
			}
		}
		break;
	case ITEM_COMMUNICATOR:
		if (!*arg2)
			send_to_char(ch, "Tune to what channel?\r\n");
		else {
			i = atoi(arg2);
			if (i == COMM_CHANNEL(obj)) {
				act("$p is already tuned to that channel.", FALSE, ch, obj, 0,
					TO_CHAR);
				break;
			}
			COMM_CHANNEL(obj) = i;
			send_to_char(ch, "%s comm-channel set to [%d].\n",
				obj->name, COMM_CHANNEL(obj));
		}
		break;
	default:
		send_to_char(ch, "You cannot tune that!\r\n");
		break;
	}
}

//
// do_status handles finding out interesting information about objects/devices
// cyborgs can use it with no argument to find out stuff about themselves
//

ACMD(do_status)
{

	struct obj_data *obj = NULL, *bul = NULL;
	int i;
	char *arg1, *arg2;
	struct affected_type *aff = NULL;

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);

	if (!*arg1) {
		if (!IS_CYBORG(ch)) {
			send_to_char(ch, "Status of what?\r\n");
			return;
		}

		acc_string_clear();
		acc_sprintf(
			"%s+++++>>>---    SYSTEM STATUS REPORT   ---<<<+++++%s\r\n",
			CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
		acc_sprintf(
			"%sHITP%s:                 %3d percent   (%4d/%4d)\r\n",
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			(GET_HIT(ch) * 100) / GET_MAX_HIT(ch), GET_HIT(ch),
			GET_MAX_HIT(ch));
		acc_sprintf(
			"%sMANA%s:                 %3d percent   (%4d/%4d)\r\n",
			CCMAG(ch, C_NRM), CCNRM(ch, C_NRM),
			(GET_MANA(ch) * 100) / GET_MAX_MANA(ch), GET_MANA(ch),
			GET_MAX_MANA(ch));
		acc_sprintf(
			"%sMOVE%s:                 %3d percent   (%4d/%4d)\r\n",
			CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
			(GET_MOVE(ch) * 100) / GET_MAX_MOVE(ch), GET_MOVE(ch),
			GET_MAX_MOVE(ch));
		acc_sprintf(
			"%s+++++>>>---   ---   ---   ---   ---   ---<<<+++++%s\r\n",
			CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));

		acc_sprintf(
			"Systems have sustained %d percent of maximum damage.\r\n",
			(GET_TOT_DAM(ch) * 100) / max_component_dam(ch));
		if (GET_BROKE(ch))
			acc_sprintf("%sYour %s has been severely damaged.%s\r\n",
				CCRED(ch, C_NRM),
				component_names[GET_BROKE(ch)][GET_OLD_CLASS(ch)],
				CCNRM(ch, C_NRM));
		if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT))
			acc_sprintf("%sSELF-DESTRUCT sequence initiated.%s\r\n",
				CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
		if (AFF3_FLAGGED(ch, AFF3_STASIS))
			acc_strcat("Systems are in static state.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_MOTION_SENSOR))
			acc_strcat("Your motion sensors are active.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_ENERGY_FIELD))
			acc_strcat("Energy fields are activated.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_REFLEX_BOOST))
			acc_strcat("Your Reflex Boosters are activated.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_POWER_BOOST))
			acc_strcat("Power levels are boosted.\r\n", NULL);
		if (AFF_FLAGGED(ch, AFF_INFRAVISION))
			acc_strcat("Infrared detection system active.\r\n", NULL);
		if (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
			acc_strcat("Your sonic imagery device is active.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_DAMAGE_CONTROL))
			acc_strcat("Damage Control systems are operating.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_HYPERSCAN))
			acc_strcat("Hyperscanning device is active.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_ADRENAL_MAXIMIZER))
			acc_strcat("Shukutei Adrenal Maximizations are active.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC))
			acc_strcat("Melee Combat Tactics are in effect.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_RADIONEGATION))
			acc_strcat("Radionegation device is operating.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_OFFENSIVE_POS))
			acc_strcat("Systems postured for offensive tactics.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_DEFENSIVE_POS))
			acc_strcat("Systems postured for defensive tactics.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_NEURAL_BRIDGING))
			acc_strcat("Cogenic Neural Bridging enabled.\r\n", NULL);
		if (affected_by_spell(ch, SKILL_ASSIMILATE)) {
			acc_sprintf(
				"%sCurrently active assimilation affects:%s\r\n",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));


			for (i = 0, aff = ch->affected; aff; aff = aff->next) {
				if (aff->type == SKILL_ASSIMILATE) {

					acc_sprintf("  %s%15s%s  %s %3d\r\n",
						CCGRN(ch, C_NRM), apply_types[aff->location],
						CCNRM(ch, C_NRM), aff->modifier >= 0 ? "+" : "-",
						abs(aff->modifier));

				}
			}
		}


		page_string(ch->desc, acc_get_string());
		return;
	}

	if (!strncmp(arg1, "internal", 8)) {

		if (!*arg2) {
			send_to_char(ch, "Status of which implant?\r\n");
			return;
		}

		if (!(obj = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg2),
				arg2);
			return;
		}

	} else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		!(obj = get_object_in_equip_all(ch, arg1, ch->equipment, &i)) &&
		!(obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
		send_to_char(ch, "You can't seem to find %s '%s'.\r\n", AN(arg1), arg1);
		return;
	}

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_TRANSPORTER:
		send_to_char(ch, "Energy Levels: %d / %d.\r\n", CUR_ENERGY(obj),
			MAX_ENERGY(obj));
		if (real_room(TRANS_TO_ROOM(obj)) == NULL)
			act("$p is currently untuned.", FALSE, ch, obj, 0, TO_CHAR);
		else {
			send_to_char(ch, "Tuned location: %s%s%s\r\n",
				CCCYN(ch, C_NRM), (CHECK_SKILL(ch, SKILL_ELECTRONICS) +
					GET_INT(ch)) > number(20, 90) ?
				(real_room(TRANS_TO_ROOM(obj)))->name : "UNKNOWN",
				CCNRM(ch, C_NRM));
		}
		break;

	case ITEM_SCUBA_TANK:
		act(tmp_sprintf("$p %s currently holding %d percent of capacity.",
				ISARE(obj->name), GET_OBJ_VAL(obj, 0) < 0 ?
				100 : GET_OBJ_VAL(obj, 0) == 0 ? 0 : (GET_OBJ_VAL(obj, 1)
					/ GET_OBJ_VAL(obj, 0)
					* 100)),
			false, ch, obj, 0, TO_CHAR);
		break;

	case ITEM_SCUBA_MASK:
		act(tmp_sprintf("The valve on $p is currently %s.",
				GET_OBJ_VAL(obj, 0) ? "open" : "closed"),
			false, ch, obj, 0, TO_CHAR);
		break;

	case ITEM_DEVICE:
		act(tmp_sprintf("$p (%s) %s currently %sactivated.\r\n"
				"The energy level is %d/%d units.",
				obj->carried_by ? "carried" :
				obj->worn_by ?
				(obj == GET_EQ(ch, obj->worn_on) ? "worn" : "implanted") : "here",
				ISARE(OBJS(obj, ch)),
				ENGINE_STATE(obj) ? "" : "de", CUR_ENERGY(obj), MAX_ENERGY(obj)),
			false, ch, obj, 0, TO_CHAR);
		break;

	case ITEM_DETONATOR:
		if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) < 50)
			send_to_char(ch, "You have no idea how.\r\n");
		else if (obj->aux_obj)
			act("$p is currently tuned to $P.", FALSE, ch, obj, obj->aux_obj,
				TO_CHAR);
		else
			act("$p is currently untuned.", FALSE, ch, obj, obj->aux_obj,
				TO_CHAR);
		break;

	case ITEM_BOMB:
		if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) < 50)
			send_to_char(ch, "You have no idea how.\r\n");
		else
			act(tmp_sprintf("$p is %sfused and %sactive.",
					(obj->contains && IS_FUSE(obj->contains)) ? "" : "un",
					(obj->contains && FUSE_STATE(obj->contains)) ? "" : "in"),
				false, ch, obj, 0, TO_CHAR);
		break;
	case ITEM_ENERGY_CELL:
	case ITEM_BATTERY:
		send_to_char(ch, "Energy Levels: %d / %d.\r\n", CUR_ENERGY(obj),
			MAX_ENERGY(obj));
		break;

	case ITEM_GUN:
        show_gun_status(ch, obj);
		break;
    case ITEM_ENERGY_GUN:
		show_gun_status(ch, obj);
		break;

	case ITEM_CLIP:

		for (bul = obj->contains, i = 0; bul; i++, bul = bul->next_content);

		send_to_char(ch, "%s contains %d/%d cartridge%s.\r\n",
			obj->name, i, MAX_LOAD(obj), i == 1 ? "" : "s");
		break;

	case ITEM_INTERFACE:

		if (INTERFACE_TYPE(obj) == INTERFACE_CHIPS) {
			send_to_char(ch, "%s [%d slots] is loaded with:\r\n",
				obj->name, INTERFACE_MAX(obj));
			for (bul = obj->contains, i = 0; bul; i++, bul = bul->next_content)
				send_to_char(ch, "%2d. %s\r\n", i, bul->name);
			break;
		}
		break;

	case ITEM_COMMUNICATOR:

		if (!ENGINE_STATE(obj)) {
			act("$p is switched off.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}

		acc_string_clear();
		acc_sprintf(
			"%s is active and tuned to channel [%d].\r\n"
			"Energy level is : [%3d/%3d].\r\n"
			"Visible entities monitoring this channel:\r\n",
			obj->name, COMM_CHANNEL(obj),
			CUR_ENERGY(obj), MAX_ENERGY(obj));
		for (bul = object_list; bul; bul = bul->next) {
			if (IS_COMMUNICATOR(bul) && ENGINE_STATE(bul) &&
				COMM_CHANNEL(bul) == COMM_CHANNEL(obj)) {
				if (bul->carried_by && can_see_creature(ch, bul->carried_by) &&
					COMM_UNIT_SEND_OK(ch, bul->carried_by))
					acc_sprintf("  %s%s\r\n",
						GET_NAME(bul->carried_by),
						COMM_UNIT_SEND_OK(bul->carried_by,
							ch) ? "" : "  (mute)");
				else if (bul->worn_by && can_see_creature(ch, bul->worn_by)
					&& COMM_UNIT_SEND_OK(ch, bul->worn_by))
					acc_sprintf("  %s%s\r\n", GET_NAME(bul->worn_by),
						COMM_UNIT_SEND_OK(bul->worn_by, ch) ? "" : "  (mute)");
			}
		}
		page_string(ch->desc, acc_get_string());
		return;

	default:
		break;
	}

	send_to_char(ch, "%s is in %s condition.\r\n", obj->name,
		obj_cond_color(obj, ch));

}


ACMD(do_repair)
{
	struct Creature *vict = NULL;
	struct obj_data *obj = NULL, *tool = NULL;
	int dam, cost, skill = 0;

	skip_spaces(&argument);

	if (!*argument)
		vict = ch;
	else if (!(vict = get_char_room_vis(ch, argument)) &&
		!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
		!(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
		send_to_char(ch, "Repair who or what?\r\n");
		return;
	}

	if (vict) {
		if (!IS_NPC(ch) && (
				(!(tool = GET_IMPLANT(ch, WEAR_HOLD)) &&
					!(tool = GET_EQ(ch, WEAR_HOLD))) ||
				!IS_TOOL(tool) || TOOL_SKILL(tool) != SKILL_CYBOREPAIR)) {
			send_to_char(ch, 
				"You must be holding a cyber repair tool to do this.\r\n");
			return;
		}

		if (vict == ch) {
			if (CHECK_SKILL(ch, SKILL_SELFREPAIR) < 10 || !IS_CYBORG(ch))
				send_to_char(ch, "You have no idea how to repair yourself.\r\n");
			else if (GET_HIT(ch) == GET_MAX_HIT(ch))
				send_to_char(ch, "You are not damaged, no repair needed.\r\n");
			else if (!IS_NPC(ch)
				&& number(12, 150) > CHECK_SKILL(ch,
					SKILL_SELFREPAIR) + TOOL_MOD(tool) + (CHECK_SKILL(ch,
						SKILL_ELECTRONICS) >> 1) + GET_INT(ch))
				send_to_char(ch, "You fail to repair yourself.\r\n");
			else {
				if (IS_NPC(ch)) {
					dam = 2 * GET_LEVEL(ch);
				} else {
					dam = (GET_LEVEL(ch)) +
						((CHECK_SKILL(ch,
								SKILL_SELFREPAIR) + TOOL_MOD(tool)) >> 2) +
						number(0, GET_LEVEL(ch));
					if (GET_CLASS(ch) == CLASS_CYBORG)
						dam += dice(GET_REMORT_GEN(ch), 10);
					else
						dam += dice(GET_REMORT_GEN(ch), 5);
				}
				dam = MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dam);
				cost = dam >> 1;
				if ((GET_MANA(ch) + GET_MOVE(ch)) < cost)
					send_to_char(ch, 
						"You lack the energy required to perform this operation.\r\n");
				else {
					GET_HIT(ch) += dam;
					GET_MOVE(ch) -= cost;
					if (GET_MOVE(ch) < 0) {
						send_to_char(ch, 
							"WARNING: Auto-accessing reserve energy.\r\n");
						GET_MANA(ch) += GET_MOVE(ch);
						GET_MOVE(ch) = 0;
					}
					send_to_char(ch, "You skillfully repair yourself.\r\n");
					act("$n repairs $mself.", TRUE, ch, 0, 0, TO_ROOM);
					gain_skill_prof(ch, SKILL_SELFREPAIR);
					WAIT_STATE(ch, PULSE_VIOLENCE * (1 + (dam >> 7)));
				}
			}
		} else {
			if (CHECK_SKILL(ch, SKILL_CYBOREPAIR) < 10)
				send_to_char(ch, 
					"You have no repair skills for repairing others.\r\n");
			else if (!IS_CYBORG(vict))
				act("You cannot repair $M.  You can only repair other borgs.",
					FALSE, ch, 0, vict, TO_CHAR);
			else if (ch->numCombatants())
				send_to_char(ch, 
					"You cannot perform repairs on fighting patients.\r\n");
			else if (GET_HIT(vict) == GET_MAX_HIT(vict))
				act("$N's systems are not currently damaged.", FALSE, ch, 0,
					vict, TO_CHAR);
			else {
				if (number(12, 150) > CHECK_SKILL(ch, SKILL_CYBOREPAIR) +
					TOOL_MOD(tool) +
					(CHECK_SKILL(ch, SKILL_ELECTRONICS) >> 1) + GET_INT(ch)) {
					act("$n attempts to repair you, but fails.", FALSE, ch, 0,
						vict, TO_VICT);
					act("You fail to repair $M.\r\n", FALSE, ch, 0, vict,
						TO_CHAR);
				} else {

					dam = (GET_LEVEL(ch) >> 1) +
						((CHECK_SKILL(ch,
								SKILL_CYBOREPAIR) + TOOL_MOD(tool)) >> 2) +
						number(0, GET_LEVEL(ch));
					dam = MIN(GET_MAX_HIT(vict) - GET_HIT(vict), dam);
					cost = dam >> 1;
					if ((GET_MANA(ch) + GET_MOVE(ch)) < cost)
						send_to_char(ch, 
							"You lack the energy required to perform this operation.\r\n");
					else {
						GET_HIT(vict) += dam;
						GET_MOVE(ch) -= cost;
						if (GET_MOVE(ch) < 0) {
							send_to_char(ch, 
								"WARNING: Auto-accessing reserve energy.\r\n");
							GET_MANA(ch) += GET_MOVE(ch);
							GET_MOVE(ch) = 0;
						}
						act("You skillfully repair $N.", FALSE, ch, 0, vict,
							TO_CHAR);
						act("$n skillfully makes repairs on $N.", TRUE, ch, 0,
							vict, TO_NOTVICT);
						act("$n skillfully makes repairs on you.", TRUE, ch, 0,
							vict, TO_VICT);
						gain_skill_prof(ch, SKILL_CYBOREPAIR);
						WAIT_STATE(ch, PULSE_VIOLENCE * (1 + (dam >> 6)));
						update_pos(vict);
					}
				}
			}
		}
		return;		 /*** end of character repair ***/
	}

	if (!obj)
		return;

	if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
		act("$p is too severly damaged for you to repair.",
			FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
		if (IS_METAL_TYPE(obj))
			skill = SKILL_METALWORKING;
		else if (IS_LEATHER_TYPE(obj))
			skill = SKILL_LEATHERWORKING;
	} else if (IS_GUN(obj) || IS_ENERGY_GUN(obj)) {
		if (IS_ARROW(obj))
			skill = SKILL_BOW_FLETCH;
		else
			skill = SKILL_GUNSMITHING;
	} else if (IS_BULLET(obj) && IS_ARROW(obj))
		skill = SKILL_BOW_FLETCH;

	if (!skill) {
		send_to_char(ch, "You can't repair that.\r\n");
		return;
	}

	if (CHECK_SKILL(ch, skill) < 20) {
		send_to_char(ch, "You are not skilled in the art of %s.\r\n",
			spell_to_str(skill));
		return;
	}

	if (GET_OBJ_DAM(obj) > GET_OBJ_MAX_DAM(obj) * 3 / 4) {
		act("$p is not damaged enough to need repair.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
			!(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
		!IS_TOOL(tool) || TOOL_SKILL(tool) != skill) {
		send_to_char(ch, "You must be holding a %s tool to do this.\r\n",
			spell_to_str(skill));
		return;
	}

	dam = ((CHECK_SKILL(ch, skill) + TOOL_MOD(tool)) << 3);
	if (IS_DWARF(ch) && skill == SKILL_METALWORKING)
		dam += (GET_LEVEL(ch) << 2);

	GET_OBJ_DAM(obj) = MIN(GET_OBJ_DAM(obj) + dam,
		(GET_OBJ_MAX_DAM(obj) >> 2) * 3);
	act("You skillfully perform repairs on $p.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n skillfully performs repairs on $p.", FALSE, ch, obj, 0, TO_ROOM);
	gain_skill_prof(ch, skill);
	WAIT_STATE(ch, 7 RL_SEC);
	return;

}

ACMD(do_overhaul)
{
	struct Creature *vict = NULL;
	struct obj_data *tool = NULL;
	skip_spaces(&argument);

	if (!(vict = get_char_room_vis(ch, argument))) {
		send_to_char(ch, "Overhaul who?\r\n");
	} else if (!IS_CYBORG(vict)) {
		send_to_char(ch, "You can only repair other borgs.\r\n");
	} else if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
			!(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
		!IS_TOOL(tool) || TOOL_SKILL(tool) != SKILL_CYBOREPAIR) {
		send_to_char(ch, "You must be holding a cyber repair tool to do this.\r\n");
	} else if (GET_TOT_DAM(vict) < (max_component_dam(vict) / 3)) {
		act("You cannot make any significant improvements to $S systems.",
			FALSE, ch, 0, vict, TO_CHAR);
	} else if (vict->getPosition() > POS_SITTING) {
		send_to_char(ch, "Your subject must be sitting, at least.\r\n");
	} else if (CHECK_SKILL(ch, SKILL_OVERHAUL) < 10) {
		send_to_char(ch, "You have no idea how to perform an overhaul.\r\n");
	} else if (!vict->master || vict->master != ch) {
		send_to_char(ch, 
			"Subjects must be following and grouped for you to perform an overhaul.\r\n");
	} else if (!AFF3_FLAGGED(vict, AFF3_STASIS)) {
		send_to_char(ch, 
			"Error: Overhauling an active subject could lead to severe data loss.\r\n");
	} else {
		int repair = GET_TOT_DAM(vict) - (max_component_dam(vict) / 3);
		repair = MIN(repair, GET_LEVEL(ch) * 800);
		repair =
			MIN(repair, (CHECK_SKILL(ch,
					SKILL_OVERHAUL) + TOOL_MOD(tool)) * 400);
		repair = MIN(repair, (GET_MOVE(ch) + GET_MANA(ch)) * 100);

		act("$n begins to perform a system overhaul on $N.",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n begins to perform a system overhaul on you.",
			FALSE, ch, 0, vict, TO_VICT);

		GET_TOT_DAM(vict) -= repair;
		GET_MOVE(ch) -= (repair / 100);
		if (GET_MOVE(ch) < 0) {
			GET_MANA(ch) += GET_MOVE(ch);
			GET_MOVE(ch) = 0;
		}
		WAIT_STATE(vict, PULSE_VIOLENCE * 5);
		WAIT_STATE(ch, PULSE_VIOLENCE * 6);
		gain_skill_prof(ch, SKILL_OVERHAUL);

		send_to_char(ch, "Overhaul complete:  System improved.\r\n");
		if (GET_BROKE(vict)) {
			send_to_char(ch,
				"%sWarning%s:  subject in need of replacement %s.\r\n",
				CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
				component_names[(int)GET_BROKE(vict)][GET_OLD_CLASS(vict)]);
		}
	}
}

const char *
obj_cond(struct obj_data *obj)
{

	int num;

	if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_MAX_DAM(obj) == -1)
		num = 0;
	else if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
		return "<broken>";
	else if (GET_OBJ_MAX_DAM(obj) == 0)
		return "frail";
	else
		num = ((GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj)) * 100 /
			GET_OBJ_MAX_DAM(obj));

	if (num == 0)
		return "perfect";
	else if (num < 10)
		return "excellent";
	else if (num < 30)
		return "good";
	else if (num < 50)
		return "fair";
	else if (num < 60)
		return "worn";
	else if (num < 70)
		return "shabby";
	else if (num < 90)
		return "bad";

	return "terrible";
}

const char *
obj_cond_color(struct obj_data *obj, struct Creature *ch)
{
	int num;

	if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_MAX_DAM(obj) == -1)
		num = 0;
	else if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
		return "<broken>";
	else if (GET_OBJ_MAX_DAM(obj) == 0)
		return tmp_sprintf("%sfrail%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else
		num = ((GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj)) * 100 /
			GET_OBJ_MAX_DAM(obj));

	if (num == 0)
		return "perfect";
	else if (num < 10)
		return "excellent";
	else if (num < 30)
		return tmp_sprintf("%sgood%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (num < 50)
		return tmp_sprintf("%sfair%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (num < 60)
		return tmp_sprintf("%sworn%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (num < 70)
		return tmp_sprintf("%sshabby%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (num < 90)
		return tmp_sprintf("%sbad%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));

	return tmp_sprintf("%sterrible%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
}

// n should run between 1 and 7
#define ALEV(n)    (CHECK_SKILL(ch, SKILL_ANALYZE) > number(50, 50+(n*10)))


void
perform_analyze( Creature *ch, obj_data *obj, bool checklev=true )
{
	extern const char *egun_types[];
    acc_string_clear();

	acc_sprintf("       %s***************************************\r\n",
		CCGRN(ch, C_NRM));
	acc_sprintf("       %s>>>     OBJECT ANALYSIS RESULTS:    <<<\r\n",
		CCCYN(ch, C_NRM));
	acc_sprintf(
		"       %s***************************************%s\r\n",
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	acc_sprintf("Description:          %s%s%s\r\n", CCCYN(ch,
			C_NRM), obj->name, CCNRM(ch, C_NRM));
	acc_sprintf("Item Classification:  %s%s%s\r\n", CCCYN(ch,
			C_NRM), item_types[(int)GET_OBJ_TYPE(obj)], CCNRM(ch, C_NRM));
	acc_sprintf("Material Composition: %s%s%s\r\n", CCCYN(ch,
			C_NRM), material_names[GET_OBJ_MATERIAL(obj)], CCNRM(ch,
			C_NRM));
	// give detailed item damage info
	acc_sprintf("Structural Integrity: %s%-15s%s%s\r\n",
		CCCYN(ch, C_NRM), obj_cond(obj), CCNRM(ch, C_NRM),
			((ALEV(5) || !checklev) && GET_OBJ_MAX_DAM(obj) > 0) ?
				tmp_sprintf("  [%3d%%]", GET_OBJ_DAM(obj) * 100 / GET_OBJ_MAX_DAM(obj)) :"");
	acc_sprintf("Commerce Value:       %s%d coins%s\r\n",
		CCCYN(ch, C_NRM), GET_OBJ_COST(obj), CCNRM(ch, C_NRM));
	acc_sprintf("Total Mass:           %s%d pounds%s\r\n",
		CCCYN(ch, C_NRM), obj->getWeight(), CCNRM(ch, C_NRM));

	acc_sprintf("Intrinsic Properties: %s", CCCYN(ch, C_NRM));
    if( GET_OBJ_EXTRA(obj) == 0 && GET_OBJ_EXTRA2(obj) == 0 ) {
        acc_strcat( "None", CCNRM(ch, C_NRM), "\r\n", NULL );
    } else {
        acc_strcat(tmp_printbits(GET_OBJ_EXTRA(obj), extra_bits),
			" ",
			tmp_printbits(GET_OBJ_EXTRA2(obj), extra2_bits),
			CCNRM(ch, C_NRM),
			"\r\n",
			NULL);
    }

	acc_sprintf("Inherent Properties:  %s", CCCYN(ch, C_NRM));
    if( GET_OBJ_EXTRA3(obj) == 0 ) {
        acc_strcat( "None", CCNRM(ch, C_NRM), "\r\n", NULL );
    } else {
        acc_strcat(tmp_printbits(GET_OBJ_EXTRA3(obj), extra3_bits),
			CCNRM(ch, C_NRM), "\r\n", NULL);
    }

	// check for affections
	bool found = false;
	for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
		if (obj->affected[i].modifier) {
			if (found)
				acc_strcat("\r\n                      ", NULL);
            else
				acc_strcat("User modifications:   ", CCCYN(ch, C_NRM), NULL);

            found = true;
			acc_sprintf("%+d to %s", obj->affected[i].modifier,
				strlist_aref(obj->affected[i].location, apply_types));
		}
	}
	if (found)
		acc_strcat(CCNRM(ch, C_NRM), "\r\n", NULL);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_ARMOR:
		acc_sprintf("Protective Quality:   %s%-15s%s%s\r\n",
			CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 0) < 2 ? "poor" :
			GET_OBJ_VAL(obj, 0) < 4 ? "minimal" :
			GET_OBJ_VAL(obj, 0) < 6 ? "moderate" :
			GET_OBJ_VAL(obj, 0) < 8 ? "good" :
			GET_OBJ_VAL(obj, 0) < 10 ? "excellent" :
			"extraordinary", CCNRM(ch, C_NRM),
			(ALEV(4) || !checklev) ?
				tmp_sprintf("  [%d]", GET_OBJ_VAL(obj, 0)):"");
		break;
	case ITEM_LIGHT:
		acc_sprintf("Duration remaining:   %s%d hours%s\r\n",
			CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 2), CCNRM(ch, C_NRM));
		break;
	case ITEM_WAND:
	case ITEM_SCROLL:
	case ITEM_STAFF:
		acc_sprintf("%sERROR:%s no further information available.\r\n",
			CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
		break;
	case ITEM_WEAPON:
		acc_sprintf("Damage Dice:          %s%dd%d%s\r\n",
			CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
			CCNRM(ch, C_NRM));
		break;
	case ITEM_CONTAINER:
		if (GET_OBJ_VAL(obj, 3))
			acc_strcat("Item is a corpse.\r\n", NULL);
		else
			acc_sprintf("Capacity:            %s%d pounds%s\r\n",
				CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 0), CCNRM(ch, C_NRM));
		break;
	case ITEM_VEHICLE:
		if (obj->contains && GET_OBJ_TYPE(obj->contains) == ITEM_ENGINE)
			acc_sprintf("Vehicle is equipped with:     %s%s%s\r\n",
				CCCYN(ch, C_NRM), obj->contains->name,
				CCNRM(ch, C_NRM));
		else
			acc_strcat("Vehicle is not equipped with an engine.\r\n", NULL);
		break;
	case ITEM_ENGINE:
		acc_sprintf(
			"Max Fuel: %s%d%s, Current Fuel: %s%d%s, Type: %s%s%s, "
			"Eff: %s%d%s\r\n", CCCYN(ch, C_NRM), MAX_ENERGY(obj),
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), CUR_ENERGY(obj), CCNRM(ch,
				C_NRM), CCCYN(ch, C_NRM), IS_SET(ENGINE_STATE(obj),
				ENG_PETROL) ? "Petrol" : IS_SET(ENGINE_STATE(obj),
				ENG_ELECTRIC) ? "Elect" : IS_SET(ENGINE_STATE(obj),
				ENG_MAGIC) ? "Magic" : "Fucked", CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), USE_RATE(obj), CCNRM(ch, C_NRM));
		break;
	case ITEM_ENERGY_GUN:
       	acc_sprintf("Damage Dice:          %s%dd%d%s\r\n"
                    "Drain Rate:           %s%d units/shot%s\r\n"
                    "Weapon Type:          %s%s%s\r\n",
			CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 0),
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), 
            GET_OBJ_VAL(obj, 3) >= EGUN_TOP ? "unknown" : 
            egun_types[GET_OBJ_VAL(obj, 3)], CCNRM(ch, C_NRM));
		break;
	case ITEM_BATTERY:
		acc_sprintf(
			"Max Energy: %s%d%s, Current Energy: %s%d%s, Recharge Rate: %s%d%s\r\n",
			CCCYN(ch, C_NRM), MAX_ENERGY(obj), CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), CUR_ENERGY(obj), CCNRM(ch, C_NRM), CCCYN(ch,
				C_NRM), RECH_RATE(obj), CCNRM(ch, C_NRM));
		break;

	case ITEM_BOMB:
		if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) > 50 && obj->contains)
			acc_sprintf("Fuse: %s.  Fuse State: %sactive.\r\n",
				obj->contains->name,
				FUSE_STATE(obj->contains) ? "" : "in");
		break;
	case ITEM_MICROCHIP:
		if (SKILLCHIP(obj)) {
			if (CHIP_DATA(obj) > 0 && CHIP_DATA(obj) < MAX_SKILLS) {
				acc_sprintf("%sData Contained: %s\'%s\'%s\r\n",
					CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
					spell_to_str(CHIP_DATA(obj)), CCNRM(ch, C_NRM));
			}
		}
		break;

	}
	page_string(ch->desc, acc_get_string());
}


ACMD(do_analyze)
{

	struct obj_data *obj = NULL;
	struct Creature *vict = NULL;
	skip_spaces(&argument);

	if (!*argument ||
		(!(vict = get_char_room_vis(ch, argument)) &&
			!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
			!(obj =
				get_obj_in_list_vis(ch, argument, ch->in_room->contents)))) {
		send_to_char(ch, "Analyze what?\r\n");
		return;
	}

	if (CHECK_SKILL(ch, SKILL_ANALYZE) < 10) {
		send_to_char(ch, "You are not trained in the methods of analysis.\r\n");
		return;
	}
	if (CHECK_SKILL(ch, SKILL_ANALYZE) < number(0, 100)) {
		send_to_char(ch, "You fail miserably.\r\n");
		WAIT_STATE(ch, 2 RL_SEC);
		return;
	}
	if (GET_MOVE(ch) < 10) {
		send_to_char(ch, 
			"You lack the energy necessary to perform the analysis.\r\n");
		return;
	}

	if (obj) {
		perform_analyze(ch, obj);
		GET_MOVE(ch) -= 10;
		return;
	}

	if (vict) {
		acc_string_clear();

		acc_sprintf("       %s***************************************\r\n",
			CCGRN(ch, C_NRM));
		acc_sprintf("       %s>>>     ENTITY ANALYSIS RESULTS:    <<<\r\n",
			CCCYN(ch, C_NRM));
		acc_sprintf("       %s***************************************%s\r\n",
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		acc_sprintf("Name:                  %s%s%s\r\n", CCCYN(ch,
				C_NRM), GET_NAME(vict), CCNRM(ch, C_NRM));
		acc_sprintf("Racial Classification: %s%s%s\r\n", CCCYN(ch,
				C_NRM), player_race[(int)GET_RACE(vict)], CCNRM(ch, C_NRM));
		if (GET_CLASS(vict) < NUM_CLASSES) {
			acc_sprintf("Primary Occupation:    %s%s%s\r\n", CCCYN(ch,
					C_NRM), pc_char_class_types[(int)GET_CLASS(vict)],
				CCNRM(ch, C_NRM));
		} else {
			acc_sprintf("Primary Type:          %s%s%s\r\n", CCCYN(ch,
					C_NRM), pc_char_class_types[(int)GET_CLASS(vict)],
				CCNRM(ch, C_NRM));
		}
		if (GET_REMORT_CLASS(vict) != CLASS_UNDEFINED)
			acc_sprintf("Secondary Occupation:  %s%s%s\r\n",
				CCCYN(ch, C_NRM),
				pc_char_class_types[(int)GET_REMORT_CLASS(vict)], CCNRM(ch,
					C_NRM));

		GET_MOVE(ch) -= 10;
		page_string(ch->desc, acc_get_string());
		return;
	}

	send_to_char(ch, "Error.\r\n");

}

ACMD(do_insert)
{
	struct obj_data *obj = NULL, *tool = NULL;
	struct Creature *vict = NULL;
	char *obj_str, *vict_str, *pos_str;
	int pos, i;

	obj_str = tmp_getword(&argument);
	vict_str = tmp_getword(&argument);
	pos_str = tmp_getword(&argument);

	if (!*obj_str || !*vict_str || !*pos_str) {
		send_to_char(ch, "Insert <object> <victim> <position>\r\n");
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, obj_str, ch->carrying))) {
		send_to_char(ch, "You do not carry that implant.\r\n");
		return;
	}
	if (!(vict = get_char_room_vis(ch, vict_str))) {
		act("Insert $p in who?", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (GET_LEVEL(ch) < LVL_IMMORT) {
		if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) < 30) {
			send_to_char(ch, "You are unskilled in the art of cybosurgery.\r\n");
			return;
		}

        if (ch->isOkToAttack(vict, false)) {
            send_to_char(ch, "You can only perform surgery in a safe room.\r\n");
            return;
        }
		if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
				!(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
				!IS_TOOL(tool) || TOOL_SKILL(tool) != SKILL_CYBO_SURGERY) {
			send_to_char(ch, 
				"You must be holding a cyber surgery tool to do this.\r\n");
			return;
		}

		if (!IS_CYBORG(vict)) {
			send_to_char(ch,
				"Your subject is not prepared for such enhancement.\r\n");
			return;
		}
	}

	pos = search_block(pos_str, wear_implantpos, 0);
	if (pos < 0 || ((GET_LEVEL(ch) < LVL_IMMORT &&
		ILLEGAL_IMPLANTPOS(pos) && !IS_OBJ_TYPE(obj, ITEM_TOOL)))) {
		send_to_char(ch, "Invalid implant position.\r\n");
		return;
	}

	if (!IS_IMPLANT(obj)) {
		act("ERROR: $p is not an implantable object.",
			FALSE, ch, obj, vict, TO_CHAR);
		return;
	}
	if (ANTI_ALIGN_OBJ(vict, obj)) {
		act("$N is unable to safely utilize $p.", FALSE, ch, obj, vict,
			TO_CHAR);
		return;
	}
	if (GET_IMPLANT(vict, pos)) {
		act("ERROR: $E is already implanted with $p there.",
			FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_CHAR);
		return;
	}

	if (!CAN_WEAR(obj, wear_bitvectors[pos])) {
		act("$p cannot be implanted there.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (obj->getWeight() > GET_STR(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
		if (ch != vict)
			act("ERROR: $p is to heavy to implant in $N.",
				FALSE, ch, obj, vict, TO_CHAR);
		else
			act("ERROR: $p is to heavy to implant.",
				FALSE, ch, obj, vict, TO_CHAR);
		return;
	}

	if (IS_INTERFACE(obj) && INTERFACE_TYPE(obj) == INTERFACE_CHIPS) {
		for (i = 0; i < NUM_WEARS; i++) {
			if ((GET_EQ(vict, i) && IS_INTERFACE(GET_EQ(vict, i)) &&
					INTERFACE_TYPE(GET_EQ(vict, i)) == INTERFACE_CHIPS) ||
				(GET_IMPLANT(vict, i) && IS_INTERFACE(GET_IMPLANT(vict, i)) &&
					INTERFACE_TYPE(GET_IMPLANT(vict, i)) == INTERFACE_CHIPS)) {
				act("$N is already using an interface.\r\n",
					FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
		}
	}

	if (IS_OBJ_STAT2(obj, ITEM2_SINGULAR)) {
		for (i = 0; i < NUM_WEARS; i++) {
			if( GET_IMPLANT(vict, i) != NULL &&
			    GET_OBJ_VNUM(GET_IMPLANT(vict, i)) == GET_OBJ_VNUM(obj) )
			{
				act("$p conflicts with an existing device.",
					FALSE, ch, obj, vict, TO_CHAR);
				return;
			}
		}
	}

	if (!IS_WEAR_EXTREMITY(pos)) {
		if (GET_LEVEL(ch) < LVL_IMMORT && vict == ch) {
			send_to_char(ch, 
				"You can only perform surgery on your extremities!\r\n");
			return;
		}
		if (GET_LEVEL(ch) < LVL_IMMORT && AWAKE(vict) && ch != vict) {
			send_to_char(ch, "Your subject is not properly sedated.\r\n");
			return;
		}
	}

	if (!(vict == ch && IS_WEAR_EXTREMITY(pos)) &&
		!IS_IMMORT(ch) && !AFF_FLAGGED(vict, AFF_SLEEP) &&
		!AFF3_FLAGGED(vict, AFF3_STASIS)) {
		act("$N wakes up with a scream as you cut into $M!",
			FALSE, ch, 0, vict, TO_CHAR);
		act("$N wakes up with a scream as $n cuts into $M!",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("$N wakes up with a scream as $n cuts into you!",
			FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
		vict->setPosition(POS_RESTING);
		//    damage(ch, vict, dice(5, 10), TYPE_SURGERY, pos);
		return;
	}

	if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) +
		(GET_DEX(ch) << 2) < number(50, 100)) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	obj_from_char(obj);
	if (equip_char(vict, obj, pos, MODE_IMPLANT))
		return;

	if (ch == vict) {
		;
		act(tmp_sprintf("$p inserted into your %s.", wear_implantpos[pos]),
			false, ch, GET_IMPLANT(vict, pos), vict, TO_CHAR);
		;
		act(tmp_sprintf("$n inserts $p into $s %s.", wear_implantpos[pos]),
			false, ch, GET_IMPLANT(vict, pos), vict, TO_NOTVICT);

		// Immortals shouldn't have to deal with messiness
		if (!IS_IMMORT(ch)) {
			if (vict->getPosition() > POS_SITTING)
				ch->setPosition(POS_SITTING);
			GET_HIT(vict) = GET_HIT(vict) / 4;
			GET_MOVE(vict) = GET_MOVE(vict) / 4;
			if (GET_HIT(vict) < 1)
				GET_HIT(vict) = 1;
			if (GET_MOVE(vict) < 1)
				GET_MOVE(vict) = 1;
			WAIT_STATE(vict, 6 RL_SEC);
		}
		slog("IMPLANT: %s inserts %s into self at %d.",
			GET_NAME(ch), obj->name, ch->in_room->number);
	} else {
		;
		act(tmp_sprintf("$p inserted into $N's %s.", wear_implantpos[pos]),
			false, ch, GET_IMPLANT(vict, pos), vict, TO_CHAR);
		act(tmp_sprintf("$n inserts $p into your %s.", wear_implantpos[pos]),
			false, ch, GET_IMPLANT(vict, pos), vict, TO_VICT);
		act(tmp_sprintf("$n inserts $p into $N's %s.", wear_implantpos[pos]),
			false, ch, GET_IMPLANT(vict, pos), vict, TO_NOTVICT);

		// Immortals shouldn't have to deal with messiness
		if (!IS_IMMORT(ch)) {
			if (vict->getPosition() > POS_RESTING)
				vict->setPosition(POS_RESTING);
			GET_HIT(vict) = 1;
			GET_MOVE(vict) = 1;
			WAIT_STATE(vict, 10 RL_SEC);
		}

		slog("IMPLANT: %s inserts %s into %s at %d.",
			GET_NAME(ch), obj->name,
			GET_NAME(vict), ch->in_room->number);
	}
}


ACMD(do_extract)
{

	struct obj_data *obj = NULL, *corpse = NULL, *tool = NULL;
	struct Creature *vict = NULL;
	char *obj_str, *vict_str, *pos_str;
	int pos;

	obj_str = tmp_getword(&argument);
	vict_str = tmp_getword(&argument);
	pos_str = tmp_getword(&argument);

	if (!*obj_str || !*vict_str) {
		send_to_char(ch,
			"Extract <object> <victim> <position>        ...or...\r\n"
			"Extract <object> <corpse>\r\n");
		return;
	}

	if (!IS_IMMORT(ch)) {
		if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) < 30) {
			send_to_char(ch,
				"You are unskilled in the art of cybosurgery.\r\n");
			return;
		}

		if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
				!(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
			!IS_TOOL(tool) || TOOL_SKILL(tool) != SKILL_CYBO_SURGERY) {
			send_to_char(ch, 
				"You must be holding a cyber surgery tool to do this.\r\n");
			return;
		}
	}

	vict = get_char_room_vis(ch, vict_str);
	if (!vict) {
		corpse = get_obj_in_list_vis(ch, vict_str, ch->in_room->contents);
		if (!corpse) {
			send_to_char(ch, "Extract from who?\r\n");
			return;
		}

		if (!(obj = get_obj_in_list_vis(ch, obj_str, corpse->contains))) {
			act("There is no such thing in $p.", FALSE, ch, corpse, 0,
				TO_CHAR);
			return;
		}

		if (IS_CORPSE(corpse)) {
			if (!IS_IMPLANT(obj) || CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
				act("$p is not implanted.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			}
		} else if (!IS_BODY_PART(corpse) || !isname("head", corpse->aliases) ||
			!OBJ_TYPE(corpse, ITEM_DRINKCON)) {
			send_to_char(ch, "You cannot extract from that.\r\n");
			return;
		}

		if (((CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) +
					(GET_DEX(ch) << 2)) < number(50, 100)) ||
			(IS_OBJ_TYPE(obj, ITEM_SCRIPT) && GET_LEVEL(ch) < 50)) {
			send_to_char(ch, "You fail.\r\n");
			return;
		}

		obj_from_obj(obj);
		obj_to_char(obj, ch);
		SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

        // Remove the implant from the corpse file
        if (IS_CORPSE(corpse) && CORPSE_IDNUM(corpse) > 0) {
            char *fname;
            FILE *corpse_file;

            fname = get_corpse_file_path(CORPSE_IDNUM(corpse));
            if ((corpse_file = fopen(fname, "w+")) != NULL) {
                fprintf(corpse_file, "<corpse>");
                corpse->saveToXML(corpse_file);
                fprintf(corpse_file, "</corpse>");
                fclose(corpse_file);
            }
            else {
                errlog("Failed to open corpse file [%s] (%s)", fname,
                strerror(errno));
            }
        }

		act("You carefully extract $p from $P.", FALSE, ch, obj, corpse,
			TO_CHAR);
		if (CHECK_SKILL(ch,
				SKILL_CYBO_SURGERY) + TOOL_MOD(tool) + (GET_DEX(ch) << 2) <
			number(50, 100)) {
			act("You damage $p during the extraction!", FALSE, ch, obj, 0,
				TO_CHAR);
			damage_eq(ch, obj, MAX((GET_OBJ_DAM(obj) >> 2), (dice(10,
							10) - CHECK_SKILL(ch,
							SKILL_CYBO_SURGERY) - (GET_DEX(ch) << 2))));
		} else
			gain_skill_prof(ch, SKILL_CYBO_SURGERY);

		return;
	}

    if (ch->isOkToAttack(vict, false) && !IS_IMMORT(ch)) {
        send_to_char(ch, "You can only perform surgery in a safe room.\r\n");
        return;
    }

	if (!(obj = get_object_in_equip_vis(ch, obj_str, vict->implants, &pos))) {
		send_to_char(ch, "Invalid object.  Type 'extract' for usage.\r\n");
		return;
	}

	if (!*pos_str) {
		send_to_char(ch, "Extract the implant from what position.\r\n");
		return;
	}

	if ((pos = search_block(pos_str, wear_implantpos, 0)) < 0) {
		send_to_char(ch, "Invalid implant position.\r\n");
		return;
	}
	if (!IS_WEAR_EXTREMITY(pos)) {
		if (GET_LEVEL(ch) < LVL_IMMORT && vict == ch) {
			send_to_char(ch, 
				"You can only perform surgery on your extrimities!\r\n");
			return;
		}

		if (GET_LEVEL(ch) < LVL_IMMORT && AWAKE(vict) && ch != vict) {
			send_to_char(ch, "Your subject is not properly sedated.\r\n");
			return;
		}
	}

	if (!(vict == ch && IS_WEAR_EXTREMITY(pos)) &&
		GET_LEVEL(ch) < LVL_AMBASSADOR && !AFF_FLAGGED(vict, AFF_SLEEP) &&
		!AFF3_FLAGGED(vict, AFF3_STASIS)) {
		act("$N wakes up with a scream as you cut into $M!",
			FALSE, ch, 0, vict, TO_CHAR);
		act("$N wakes up with a scream as $n cuts into $M!",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("$N wakes up with a scream as $n cuts into you!",
			FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
		vict->setPosition(POS_RESTING);
		return;
	}

	if (!GET_IMPLANT(vict, pos)) {
		act("ERROR: $E is not implanted there.", FALSE, ch, 0, vict, TO_CHAR);
		return;					// unequip_char(vict, pos, MODE_IMPLANT);
	}

	if ((CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) +
			(GET_DEX(ch) << 2) < number(50, 100)) ||
		(IS_OBJ_TYPE(obj, ITEM_SCRIPT) && GET_LEVEL(ch) < 50)) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	obj_to_char((obj = unequip_char(vict, pos, MODE_IMPLANT)), ch);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	if (ch == vict) {
		act(tmp_sprintf("$p extracted from your %s.", wear_implantpos[pos]),
			false, ch, obj, vict, TO_CHAR);
		act(tmp_sprintf("$n extracts $p from $s %s.", wear_implantpos[pos]),
			false, ch, obj, vict, TO_NOTVICT);

		if (!IS_IMMORT(ch)) {
			if (vict->getPosition() > POS_SITTING)
				vict->setPosition(POS_SITTING);
			GET_HIT(vict) = GET_HIT(vict) / 4;
			GET_MOVE(vict) = GET_MOVE(vict) / 4;
			if (GET_HIT(vict) < 1)
				GET_HIT(vict) = 1;
			if (GET_MOVE(vict) < 1)
				GET_MOVE(vict) = 1;
			WAIT_STATE(vict, 6 RL_SEC);
		}
	} else {
		;
		act(tmp_sprintf("$p extracted from $N's %s.", wear_implantpos[pos]),
			false, ch, obj, vict, TO_CHAR);
		act(tmp_sprintf("$n extracts $p from your %s.", wear_implantpos[pos]),
			false, ch, obj, vict, TO_VICT);
		act(tmp_sprintf("$n extracts $p from $N's %s.", wear_implantpos[pos]),
			false, ch, obj, vict, TO_NOTVICT);

		if (!IS_IMMORT(ch)) {
			if (vict->getPosition() > POS_RESTING)
				vict->setPosition(POS_RESTING);
			GET_HIT(vict) = 1;
			GET_MOVE(vict) = 1;
			WAIT_STATE(vict, 10 RL_SEC);
		}
	}

	if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) +
		(GET_DEX(ch) << 2) < number(50, 100)) {
		act("You damage $p during the extraction!", FALSE, ch, obj, 0,
			TO_CHAR);
		damage_eq(ch, obj, MAX((GET_OBJ_DAM(obj) >> 2), (dice(10,
						10) - CHECK_SKILL(ch,
						SKILL_CYBO_SURGERY) - (GET_DEX(ch) << 2))));
	} else
		gain_skill_prof(ch, SKILL_CYBO_SURGERY);
}

ACMD(do_cyberscan)
{
	struct obj_data *obj = NULL, *impl = NULL;
	struct Creature *vict = NULL;
	int i;
	int found = 0;

	skip_spaces(&argument);

	if (!(vict = get_char_room_vis(ch, argument))) {
		if ((obj = get_obj_in_list_vis(ch, argument, ch->carrying)) ||
			(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
			for (impl = obj->contains, found = 0; impl;
				impl = impl->next_content)
				if (IS_IMPLANT(impl) && can_see_object(ch, impl)
					&& !IS_OBJ_TYPE(obj, ITEM_SCRIPT)
					&& ((CHECK_SKILL(ch, SKILL_CYBERSCAN) + (AFF3_FLAGGED(ch,
									AFF3_SONIC_IMAGERY) ? 50 : 0) > number(70,
								150)) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
					act("$p detected.", FALSE, ch, impl, 0, TO_CHAR);
					++found;
				}
			if (!found)
				act("No implants detected in $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n scans $p.", TRUE, ch, obj, 0, TO_ROOM);
		} else
			send_to_char(ch, "Cyberscan who?\r\n");
		return;
	}

	if (ch != vict && can_see_creature(vict, ch)) {
		act("$n scans $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n scans you.", FALSE, ch, 0, vict, TO_VICT);
	}

	send_to_char(ch, "CYBERSCAN RESULTS:\r\n");
	for (i = 0; i < NUM_WEARS; i++) {
		if ((obj = GET_IMPLANT(vict, i)) && can_see_object(ch, obj) &&
				!IS_OBJ_TYPE(obj, ITEM_SCRIPT) &&
				((CHECK_SKILL(ch, SKILL_CYBERSCAN) +
						(AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 50 : 0) >
						number(50, 120)) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {

			send_to_char(ch, "[%12s] - %s -\r\n", wear_implantpos[i],
				obj->name);
			++found;
		}
	}

	if (!found)
		send_to_char(ch, "Negative.\r\n");
	else if (ch != vict)
		gain_skill_prof(ch, SKILL_CYBERSCAN);
}

int
CLIP_COUNT(struct obj_data *clip)
{
	struct obj_data *bul = NULL;
	int count = 0;

	for (bul = clip->contains, count = 0; bul;
		count++, bul = bul->next_content);

	return count;
}

ACMD(do_load)
{
	struct obj_data *obj1 = NULL, *obj2 = NULL, *bullet = NULL;
	char *arg1, *arg2;
	int i = 0;
	int count = 0;
	bool internal = false;

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);

	if (!*arg1) {
		send_to_char(ch, "%s what?\r\n", CMD_NAME);
		return;
	}

	if (subcmd == SCMD_LOAD) {

		if (!(obj1 = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			send_to_char(ch, "You can't find any '%s' to load.\r\n", arg1);
			return;
		}
		if (!*arg2) {
			act("Load $p into what?", FALSE, ch, obj1, 0, TO_CHAR);
			return;
		}
		if (!strncmp(arg2, "internal", 8)) {

			internal = true;
			arg2 = tmp_getword(&argument);

			if (!*arg2) {
				send_to_char(ch, "Load which implant?\r\n");
				return;
			}

			if (!(obj2 = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
				send_to_char(ch, "You are not implanted with %s '%s'.\r\n",
					AN(arg2), arg2);
				return;
			}

		} else if (!(obj2 =
				get_object_in_equip_vis(ch, arg2, ch->equipment, &i))
			&& !(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying))
			&& !(obj2 =
				get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
			act(tmp_sprintf("You don't have any '%s' to load $p into.", arg2),
				false, ch, obj1, 0, TO_CHAR);
			return;
		}
		if (obj2 == obj1) {
			send_to_char(ch, "Real funny.\r\n");
			return;
		}
		if (IS_OBJ_STAT(obj1, ITEM2_BROKEN)) {
			act("$p is broken, and cannot be loaded.",
				FALSE, ch, obj1, obj2, TO_CHAR);
			return;
		}
		if (IS_OBJ_STAT(obj2, ITEM2_BROKEN)) {
			act("$P is broken, and cannot be loaded.",
				FALSE, ch, obj1, obj2, TO_CHAR);
			return;
		}

		if (IS_ENERGY_GUN(obj2)) {
			if (!IS_ENERGY_CELL(obj1)) {
				act("You can't load $p into $P.", FALSE, ch, obj1, obj2,
					TO_CHAR);
				return;
			}
			if (obj2->contains) {
				act("$P is already loaded with $p.",
					FALSE, ch, obj2->contains, obj2, TO_CHAR);
				return;
			}
		} else if (IS_CLIP(obj2)) {

			if (!IS_BULLET(obj1)) {
				act("You can't load $p into $P.", FALSE, ch, obj1, obj2,
					TO_CHAR);
				return;
			}
			if (GUN_TYPE(obj1) != GUN_TYPE(obj2)) {
				act("$p does not fit in $P", FALSE, ch, obj1, obj2, TO_CHAR);
				return;
			}
			for (bullet = obj2->contains, count = 0; bullet;
				count++, bullet = bullet->next_content);
			if (count >= MAX_LOAD(obj2)) {
				act("$P is already loaded to capacity.", FALSE, ch, obj1, obj2,
					TO_CHAR);
				return;
			}
		} else if (IS_GUN(obj2)) {

			if (!MAX_LOAD(obj2)) {	/** clip gun **/
				if (!IS_CLIP(obj1)) {
					act("You have to use a clip with $P.",
						FALSE, ch, obj1, obj2, TO_CHAR);
					return;
				}
				if (obj2->contains) {
					act("$P is already loaded with $p.",
						FALSE, ch, obj2->contains, obj2, TO_CHAR);
					return;
				}
			} else {			   /** revolver type **/
				if (!IS_BULLET(obj1)) {
					act("You can only load $P with bullets",
						FALSE, ch, obj1, obj2, TO_CHAR);
					return;
				}
				for (bullet = obj2->contains, count = 0; bullet;
					count++, bullet = bullet->next_content);
				if (count >= MAX_LOAD(obj2)) {
					act("$P is already fully loaded.", FALSE, ch, obj1, obj2,
						TO_CHAR);
					return;
				}
			}

			if (GUN_TYPE(obj1) != GUN_TYPE(obj2)) {
				act("$p does not fit in $P", FALSE, ch, obj1, obj2, TO_CHAR);
				return;
			}
		} else if (IS_CHIP(obj1) && IS_INTERFACE(obj2) &&
				INTERFACE_TYPE(obj2) == INTERFACE_CHIPS) {
			if (obj2->getNumContained() >= INTERFACE_MAX(obj2)) {
				act("$P is already loaded with microchips.",
					FALSE, ch, obj1, obj2, TO_CHAR);
				return;
			}
			if (redundant_skillchip(obj1, obj2)) {
				send_to_char(ch, "Attachment of that chip is redundant.\r\n");
				return;
			}
			if (obj1->getWeight() > 5) {
				act("You can't fit something that big into $P!",
					FALSE, ch, obj1, obj2, TO_CHAR);
				return;
			}
		} else {
			act("You can't load $p into $P.", FALSE, ch, obj1, obj2, TO_CHAR);
			return;
		}

		if (internal) {
			act("You load $p into $P (internal).", TRUE, ch, obj1, obj2,
				TO_CHAR);
			act("$n loads $p into $P (internal).", TRUE, ch, obj1, obj2,
				TO_ROOM);
		} else {
			act("You load $p into $P.", TRUE, ch, obj1, obj2, TO_CHAR);
			act("$n loads $p into $P.", TRUE, ch, obj1, obj2, TO_ROOM);
		}
		obj_from_char(obj1);
		obj_to_obj(obj1, obj2);
		if (IS_CHIP(obj1) && ch == obj2->worn_by && obj1->action_desc)
			act(obj1->action_desc, FALSE, ch, 0, 0, TO_CHAR);
		i = 10 - MIN(8,
			((CHECK_SKILL(ch, SKILL_SPEED_LOADING) + GET_DEX(ch)) >> 4));
		WAIT_STATE(ch, i);
		return;
	}

	if (subcmd == SCMD_UNLOAD) {

		if (!strncmp(arg1, "internal", 8)) {

			internal = true;
			if (!*arg2) {
				send_to_char(ch, "Unload which implant?\r\n");
				return;
			}

			if (!(obj2 = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
				send_to_char(ch, "You are not implanted with %s '%s'.\r\n",
					AN(arg2), arg2);
				return;
			}

		} else if (!(obj2 =
				get_object_in_equip_vis(ch, arg1, ch->equipment, &i))
			&& !(obj2 = get_obj_in_list_vis(ch, arg1, ch->carrying))
			&& !(obj2 =
				get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
			act(tmp_sprintf("You can't find %s '%s' to unload.", AN(arg1), arg1),
				false, ch, obj1, 0, TO_CHAR);
			return;
		}

		if (IS_ENERGY_GUN(obj2)) {
			if (!(obj1 = obj2->contains)) {
				act("$p is not loaded with an energy cell.", FALSE, ch, obj2,
					0, TO_CHAR);
				return;
			}
		} else if (IS_GUN(obj2) || IS_CLIP(obj2) || IS_INTERFACE(obj2)) {
			if (!(obj1 = obj2->contains)) {
				act("$p is not loaded.", FALSE, ch, obj2, 0, TO_CHAR);
				return;
			}
		} else {
			send_to_char(ch, "You cannot unload anything from that.\r\n");
			return;
		}

		if (internal) {
			act("You unload $p from $P (internal).", TRUE, ch, obj1, obj2,
				TO_CHAR);
			act("$n unloads $p from $P (internal).", TRUE, ch, obj1, obj2,
				TO_ROOM);
		} else {
			act("You unload $p from $P.", TRUE, ch, obj1, obj2, TO_CHAR);
			act("$n unloads $p from $P.", TRUE, ch, obj1, obj2, TO_ROOM);
		}
		obj_from_obj(obj1);
		obj_to_char(obj1, ch);
		i = 10 - MIN(8,
			((CHECK_SKILL(ch, SKILL_SPEED_LOADING) + GET_DEX(ch)) >> 4));
		WAIT_STATE(ch, i);

		return;
	}
}

int
redundant_skillchip(struct obj_data *chip, struct obj_data *slot)
{

	struct obj_data *tchp = NULL;

	if (CHIP_TYPE(chip) != CHIP_SKILL)
		return 0;

	for (tchp = slot->contains; tchp; tchp = tchp->next_content)
		if (GET_OBJ_VNUM(tchp) == GET_OBJ_VNUM(chip))
			return 1;

	return 0;
}

ACMD(do_refill)
{
	struct obj_data *syr = NULL, *vial = NULL;
	char *arg1, *arg2;
	int i;

	arg1 = tmp_getword(&argument);
	arg2 = tmp_getword(&argument);

	if (!*arg1)
		send_to_char(ch, "Refill what from what?\r\n");
	else if (!*arg2)
		send_to_char(ch, "Usage: refill <to obj> <from obj>\r\n");
	else if (!(syr = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		!(syr = get_object_in_equip_vis(ch, arg1, ch->equipment, &i))) {
		send_to_char(ch, "You don't seem to have %s '%s' to refill.\r\n",
			AN(arg1), arg1);
	} else if (!IS_SYRINGE(syr))
		send_to_char(ch, "You can only refill syringes.\r\n");
	else if (GET_OBJ_VAL(syr, 0))
		act("$p is already full.", FALSE, ch, syr, 0, TO_CHAR);
	else if (!(vial = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		send_to_char(ch, "You can't find %s '%s' to refill from.\r\n", AN(arg2),
			arg2);
	} else if (!GET_OBJ_VAL(vial, 0))
		act("$p is empty.", FALSE, ch, vial, 0, TO_CHAR);
	else {

		act("You refill $p from $P.", FALSE, ch, syr, vial, TO_CHAR);

		GET_OBJ_VAL(syr, 0) = GET_OBJ_VAL(vial, 0);
		GET_OBJ_VAL(syr, 1) = GET_OBJ_VAL(vial, 1);
		GET_OBJ_VAL(syr, 2) = GET_OBJ_VAL(vial, 2);
		GET_OBJ_VAL(syr, 3) = GET_OBJ_VAL(vial, 3);

		if (IS_POTION(vial) || IS_OBJ_STAT(vial, ITEM_MAGIC))
			SET_BIT(GET_OBJ_EXTRA(syr), ITEM_MAGIC);

		if (!syr->shared->proto || syr->shared->proto->aliases != syr->aliases) {
			free(syr->aliases);
		}
		syr->aliases = str_dup(tmp_sprintf("%s %s", fname(vial->aliases), syr->aliases));

		if (IS_POTION(vial)) {
			act("$P dissolves before your eyes.", FALSE, ch, syr, vial,
				TO_CHAR);
			extract_obj(vial);
		} else {
			vial->modifyWeight(-1);

			if (vial->getWeight() <= 0) {
				act("$P has been exhausted.", FALSE, ch, syr, vial, TO_CHAR);
				extract_obj(vial);
			}
		}
	}
}

ACMD(do_transmit)
{

	struct obj_data *obj = NULL;
	int i;
	char *arg;

	arg = tmp_getword(&argument);

	if (!*arg) {
		send_to_char(ch, "Usage:  transmit [internal] <device> <message>\r\n");
		return;
	}

	if (!strncmp(arg, "internal", 8)) {

		arg = tmp_getword(&argument);

		if (!*arg) {
			send_to_char(ch, "Transmit with which implant?\r\n");
			return;
		}

		if (!(obj = get_object_in_equip_vis(ch, arg, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg),
				arg);
			return;
		}

	} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)) &&
		!(obj = get_object_in_equip_all(ch, arg, ch->equipment, &i)) &&
		!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
		send_to_char(ch, "You can't seem to find %s '%s'.\r\n", AN(arg), arg);
		return;
	}

	if (!IS_COMMUNICATOR(obj)) {
		act("$p is not a communication device.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (!ENGINE_STATE(obj)) {
		act("$p is not switched on.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (!*argument) {
		send_to_char(ch, "Usage:  transmit [internal] <device> <message>\r\n");
		return;
	}

	if (!obj->worn_by || (obj != GET_IMPLANT(ch, obj->worn_on)))
		act(tmp_sprintf("$n speaks into $p%s.", obj->worn_by ? " (worn)" : ""),
			false, ch, obj, 0, TO_ROOM);

	send_to_comm_channel(ch, tmp_sprintf("$n >%s", argument),
		COMM_CHANNEL(obj), FALSE, FALSE);
}


ACMD(do_overdrain)
{
	struct obj_data *source = NULL;
	int i;
	int amount = 0;
	char *arg1, *arg2;

	//Make sure they know how to overdrain
	if (CHECK_SKILL(ch, SKILL_OVERDRAIN) <= 0) {
		send_to_char(ch, "You don't know how.\r\n");
		return;
	}

	arg1 = tmp_getword(&argument);
	if (!*arg1) {
		send_to_char(ch, "Usage:  overdrain [internal] <battery/device>\r\n");
		return;
	}

	arg2 = tmp_getword(&argument);

	// Find the object to drain from
	if (!strncmp(arg1, "internal", 8)) {
		if (!*arg2) {
			send_to_char(ch, "Drain energy from which implant?\r\n");
			return;
		}

		source = get_object_in_equip_vis(ch, arg2, ch->implants, &i);
		if (!source) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg2),
				arg2);
			return;
		}

	} else if (!(source = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
			!(source = get_object_in_equip_all(ch, arg1, ch->equipment, &i)) &&
			!(source = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
		send_to_char(ch, "You can't seem to find %s '%s'.\r\n", AN(arg1), arg1);
		return;
	}
	if (IS_IMPLANT(source)
		&& source != get_object_in_equip_vis(ch, arg2, ch->implants, &i)) {
		act("ERROR: $p not installed properly.", FALSE, ch, source, 0,
			TO_CHAR);
		return;
	}

	if (!IS_BATTERY(source) && !IS_DEVICE(source)) {
		act("You cannot draw energy from $p.", FALSE, ch, source, 0, TO_CHAR);
		return;
	}

	if (MAX_ENERGY(source) <= 0) {
		act("$p has no energy to drain.", FALSE, ch, source, 0, TO_CHAR);
		return;
	}

	if (CUR_ENERGY(source) <= 0) {
		act("$p is depleted of energy.", FALSE, ch, source, 0, TO_CHAR);
		return;
	}
	if (IS_BATTERY(source) && COST_UNIT(source)) {
		act("$p is equipped with a power regulator.", FALSE, ch, source, 0,
			TO_CHAR);
		return;
	}
	gain_skill_prof(ch, SKILL_OVERDRAIN);
	amount = number(0, ch->getLevelBonus(SKILL_OVERDRAIN));
	perform_recharge(ch, source, ch, 0, amount);
	return;
}


ACMD(do_de_energize)
{
	Creature *vict;
	int move = 0;

	skip_spaces(&argument);

	if (!(vict = get_char_room_vis(ch, argument))) {
		if (ch->numCombatants()) {
			vict = ch->findRandomCombat();
		} else {
			send_to_char(ch, "De-energize who??\r\n");
			return;
		}
	}

	if (vict == ch) {
		send_to_char(ch, "Let's not try that shall we...\r\n");
		return;
	}

	if (!ch->isOkToAttack(vict))
		return;

	appear(ch, vict);
	check_killer(ch, vict);

	// missed attempt
	if (AWAKE(vict) &&
		CHECK_SKILL(ch, SKILL_DE_ENERGIZE) < (number(0, 80) + GET_DEX(vict))) {
		act("You avoid $n's attempt to de-energize you!",
			FALSE, ch, 0, vict, TO_VICT);
		act("$N avoid $n's attempt to de-energize $M!",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("$N avoids your attempt to de-energize $M!",
			FALSE, ch, 0, vict, TO_CHAR);
		WAIT_STATE(ch, 2 RL_SEC);
		return;
	}
	// victim resists
	if (mag_savingthrow(vict,
			GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3), SAVING_ROD)) {
		act("$n attempts to de-energize you, but fails!",
			FALSE, ch, 0, vict, TO_VICT);
		act("$n attempts to de-energize $N, but fails!",
			FALSE, ch, 0, vict, TO_NOTVICT);
		act("You attempt to de-energize $N, but fail!",
			FALSE, ch, 0, vict, TO_CHAR);
		WAIT_STATE(ch, 4 RL_SEC);
		hit(vict, ch, TYPE_UNDEFINED);
		return;
	}

	act("$n grabs you in a cold grip, draining you of energy!",
		FALSE, ch, 0, vict, TO_VICT);
	act("You grab $N in a cold grip, draining $M of energy!",
		FALSE, ch, 0, vict, TO_CHAR);
	act("$n grabs $N in a cold grip, draining $M of energy!",
		FALSE, ch, 0, vict, TO_NOTVICT);

	move = MIN(GET_MOVE(vict), dice(GET_REMORT_GEN(ch) + 5, 20));

	GET_MOVE(vict) -= move;

	GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + move);

	if (!move)					// vict all drained up
		act("$N is already completely de-energized!", FALSE, ch, 0, vict,
			TO_CHAR);
	else {
		if (!GET_MOVE(vict))
			act("$N is now completely de-energized!", FALSE, ch, 0, vict,
				TO_CHAR);
		gain_skill_prof(ch, SKILL_DE_ENERGIZE);
	}

	WAIT_STATE(ch, 5 RL_SEC);
	hit(vict, ch, TYPE_UNDEFINED);
}

/**
 * Function to allow cyborgs to assimilate objects
 *
 **/
ACMD(do_assimilate)
{
	struct obj_data *obj = NULL;
	struct affected_type *aff = NULL, *oldaffs = NULL, *newaffs = NULL;
	int manacost;
	int num_affs = 0;
	int i, j;
	int found = 0;
	int num_newaffs = 0;
	int damd = 0;

	if (!IS_CYBORG(ch)) {
		send_to_char(ch, "Only the Borg may assimilate.\r\n");
		return;
	}

	if (CHECK_SKILL(ch, SKILL_ASSIMILATE) < 60) {
		send_to_char(ch, 
			"You do not have sufficient programming to assimilate.\r\n");
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "Assimilate what?\r\n");
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying))) {
		send_to_char(ch, "You can't find any '%s'.\r\n", argument);
		return;
	}

	manacost = MIN(10, obj->getWeight());

	if (GET_MANA(ch) < manacost) {
		send_to_char(ch, "You lack the mana required to assimilate this.\r\n");
		return;
	}

	GET_MANA(ch) -= manacost;

	// see if it can be assimilated
	if ((IS_OBJ_STAT(obj, ITEM_BLESS) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_DAMNED) && IS_GOOD(ch))) {
		act("You are burned by $p as you try to assimilate it!", FALSE, ch,
			obj, 0, TO_CHAR);
		act("$n screams in agony as $e tries to assimilate $p!", FALSE, ch,
			obj, 0, TO_ROOM);
		damd = abs(GET_ALIGNMENT(ch));
		damd >>= 5;
		damd = MAX(5, damd);
		damage(ch, ch, dice(damd, 6), TOP_SPELL_DEFINE, -1);
		return;
	}

	if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
		act("You are zapped by $p and instantly let go of it.",
			FALSE, ch, obj, 0, TO_CHAR);
		act("$n is zapped by $p and instantly lets go of it.",
			FALSE, ch, obj, 0, TO_ROOM);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return;
	}
	// do the affect calculations only if the char_class flags are valid for the char
	if (!invalid_char_class(ch, obj)) {

		// count the existing affs on the char
		for (aff = ch->affected; aff; aff = aff->next)
			if (aff->type == SKILL_ASSIMILATE)
				num_affs++;

		// allocate oldaffs if needed
		if (num_affs
			&& !(oldaffs =
				(struct affected_type *)malloc(sizeof(struct affected_type) *
					num_affs))) {
			errlog("unable to allocate oldaffs in do_assimilate.");
			return;
		}
		// make a copy of the old assimilate affs
		for (i = 0, aff = ch->affected; aff; aff = aff->next)
			if (aff->type == SKILL_ASSIMILATE)
				oldaffs[i++] = *aff;


		// remove all the old affs
		affect_from_char(ch, SKILL_ASSIMILATE);

		// scan the list of affs on the object
		for (i = 0; i < MAX_OBJ_AFFECT; i++) {

			if (obj->affected[i].location == APPLY_NONE)
				continue;

			found = 0;

			// scan the list of old affs
			for (j = 0; j < num_affs && !found; j++) {

				// see if the affs match
				if (oldaffs[j].location == obj->affected[i].location) {

					// opposite signs, combine
					if ((oldaffs[j].modifier > 0
							&& obj->affected[i].modifier < 0)
						|| (oldaffs[j].modifier < 0
							&& obj->affected[i].modifier > 0)) {

						oldaffs[j].modifier += obj->affected[i].modifier;

					}
					// same signs, choose extreme value
					else if (oldaffs[j].modifier > 0)
						oldaffs[j].modifier =
							MAX(oldaffs[j].modifier,
							obj->affected[i].modifier);
					else
						oldaffs[j].modifier =
							MIN(oldaffs[j].modifier,
							obj->affected[i].modifier);

					// reset level and duration
					oldaffs[j].level =
						GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 2);

					oldaffs[j].duration =
						((CHECK_SKILL(ch,
								SKILL_ASSIMILATE) / 10) +
						(oldaffs[j].level >> 4));

					found = 1;
					break;

				}

			}
			if (found)
				continue;

			num_newaffs++;

			// create a new aff
			if (!(newaffs =
					(struct affected_type *)realloc(oldaffs,
						sizeof(struct affected_type) * (++num_affs)))) {
				errlog("error reallocating newaffs in do_assimilate.");
				return;
			}

			newaffs[num_affs - 1].type = SKILL_ASSIMILATE;
			newaffs[num_affs - 1].level =
				GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 2);
			newaffs[num_affs - 1].duration =
				(CHECK_SKILL(ch,
					SKILL_ASSIMILATE) / 10) + (newaffs[num_affs -
					1].level >> 4);
			newaffs[num_affs - 1].modifier = obj->affected[i].modifier;
			newaffs[num_affs - 1].location = obj->affected[i].location;
			newaffs[num_affs - 1].bitvector = 0;
			newaffs[num_affs - 1].aff_index = 0;

			oldaffs = newaffs;
		}

	}

	act("You assimilate $p.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n assimilates $p...", TRUE, ch, obj, 0, TO_ROOM);

	// stick the affs on the char
	for (i = 0; i < num_affs; i++) {
		if (oldaffs[i].modifier)
			affect_to_char(ch, oldaffs + i);
	}

	if (num_newaffs) {
		send_to_char(ch, "You feel... different.\r\n");
		gain_skill_prof(ch, SKILL_ASSIMILATE);
		free(oldaffs);
	}

	extract_obj(obj);
}

ACMD(do_deassimilate)
{

	if (!IS_CYBORG(ch)) {
		send_to_char(ch, "Resistance is Futile.\r\n");
		return;
	}

	if (CHECK_SKILL(ch, SKILL_ASSIMILATE) < 60) {
		send_to_char(ch, "Your programming is insufficient to deassimilate.\r\n");
		return;
	}

	if (!affected_by_spell(ch, SKILL_ASSIMILATE)) {
		send_to_char(ch, "You currently have no active assimilations.\r\n");
		return;
	}

	if (GET_MANA(ch) < 10) {
		send_to_char(ch, "Your bioenergy level is too low to deassimilate.\r\n");
		return;
	}

	affect_from_char(ch, SKILL_ASSIMILATE);
	GET_MANA(ch) -= 10;
	send_to_char(ch, "Deassimilation complete.\r\n");
}
