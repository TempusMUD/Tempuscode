//
// File: act.cyborg.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
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

/* extern variables */
extern struct room_data *world;
extern struct spell_info_type spell_info[];
extern struct obj_data *object_list;

int check_mob_reaction(struct char_data *ch, struct char_data *vict);
void gain_condition(struct char_data *ch, int condition, int value);
void gain_skill_prof(struct char_data *ch, int skillnum);
void appear(struct char_data *ch, struct char_data *vict);
void look_at_target(struct char_data *ch, char *arg);
char *obj_cond(struct obj_data *obj);  /** writes to buf2 **/

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

    {"NONE",                "NONE",                  "NONE"},
    {"primary transformer", "analog converter",      "neural net"},
    {"spinal cable",        "internal gyroscope",    "cerebral servo"},
    {"hydraulic line",      "high speed controller", "interface chip"},
    {"receptor enhancer",   "receptor enhancer",     "receptor enhancer"},
    {"primary capacitor",   "primary capacitor",     "math coprocessor"},
    {"cooling pump",        "ventilator fan",        "ventilation unit"},
    {"system coordinator",  "parallel processor",    "parallel processor"},
    {"kinetic drive unit",  "kinetic drive unit",    "power converter"},
    {"hard disk",           "hard disk",             "hard disk"},

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
    "elevator",
    "\n"
};

int 
max_component_dam(struct char_data *ch)
{

    int max;
    max = GET_LEVEL(ch) * 80;
    max *= GET_CON(ch);
    max *= (1 + GET_REMORT_GEN(ch));
  
    max = MAX(1, max);

    return (max);
}

void 
perform_recharge(struct char_data *ch, struct obj_data *battery, 
		 struct char_data *vict, struct obj_data *engine,
		 int amount)
{
	int wait = 0;
    if (battery) {
	if (!amount)
	    amount = RECH_RATE(battery);
    
	if ((CUR_ENERGY(battery) - amount) < 0)
	    amount = CUR_ENERGY(battery);

	if (GET_OBJ_TYPE(battery) == ITEM_BATTERY && COST_UNIT(battery) * amount > GET_CASH(ch)) {
	    amount = GET_CASH(ch) / COST_UNIT(battery);
      
	    if (!amount) {
			send_to_char("You do not have the cash to pay for the energy.\r\n",
					 ch);
			return;
	    }
	}
    } else {
	if (!amount)
	    amount = MIN(GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3), GET_MOVE(ch));
	else
	    amount = MIN(amount, GET_MOVE(ch));
    }

    if (!engine && !vict) {
	slog("SYSERR:  NULL engine and vict in perform_recharge");
	return;
    }

    if (vict) {
	if (GET_MOVE(vict) + amount > GET_MAX_MOVE(vict))
	    amount = GET_MAX_MOVE(vict) - GET_MOVE(vict);
    
	GET_MOVE(vict) = MIN(GET_MAX_MOVE(vict), GET_MOVE(vict) + amount);

	if (battery)
	    CUR_ENERGY(battery) = MAX(0, CUR_ENERGY(battery) -amount);
	else
	    GET_MOVE(ch) -= amount;

    } else {  /** recharging an engine **/

	if ((CUR_ENERGY(engine) + amount) > MAX_ENERGY(engine))
	    amount = MAX_ENERGY(engine) - CUR_ENERGY(engine);
	CUR_ENERGY(engine) = MIN(MAX_ENERGY(engine), CUR_ENERGY(engine) + amount);

	if (battery)
	    CUR_ENERGY(battery) = MAX(0, CUR_ENERGY(battery) -amount);
	else
	    GET_MOVE(ch) -= amount;
    }
    
    sprintf(buf, 
	    "%s>>>        ENERGY TRANSFER         <<<%s\r\n" 
	    "From:  %s%20s%s\r\n"
	    "To:    %s%20s%s\r\n\r\n" 
	    "Transfer Amount:       %s%5d Units%s\r\n"
	    "Energy Level(TARGET):  %s%5d Units%s\r\n"
	    "Energy Level(SOURCE):  %s%5d Units%s\r\n",
	    QGRN, QNRM,
	    QCYN, battery ? battery->short_description : "self", QNRM,
	    QCYN, vict ? vict->player.name : engine->short_description, QNRM,
	    QCYN, amount, QNRM,
	    QCYN, vict ? GET_MOVE(vict) : CUR_ENERGY(engine), QNRM,
	    QCYN, battery ? CUR_ENERGY(battery) : GET_MOVE(ch), QNRM);
    if (battery) {
	if (GET_OBJ_TYPE(battery) == ITEM_BATTERY && COST_UNIT(battery)) {
	    sprintf(buf, "%sYour cost: %d credits.\r\n", 
		    buf, amount * COST_UNIT(battery));
	    GET_CASH(ch) -= amount * COST_UNIT(battery);
	}
    }
    if ((battery && CUR_ENERGY(battery) <= 0) ||
	(!battery && GET_MOVE(ch) <= 0))
	sprintf(buf, "%sSTATUS: %sSOURCE DEPLETED%s\r\n", buf, QRED, QNRM);
  
    if ((vict && GET_MOVE(vict) == GET_MAX_MOVE(vict)) ||
	(engine && CUR_ENERGY(engine) == MAX_ENERGY(engine)))
	sprintf(buf, "%sSTATUS: %sTARGET NOW FULLY ENERGIZED%s\r\n", 
		buf, QRED, QNRM);
  
    send_to_char(buf, ch);
	wait = 10 + MIN((amount / 5), 90);
	
    if (CHECK_SKILL(ch, SKILL_OVERDRAIN) > 50) {
		wait -= CHECK_SKILL(ch, SKILL_OVERDRAIN)/200 * wait;
	}
    WAIT_STATE(ch,wait);

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
	    send_to_room("BING!  Something wierd just happened.\r\n", ch->in_room);
    } else if (engine) {
	if (battery)
	    act("$n recharges $p from $P.", TRUE, ch, engine, battery, TO_ROOM);
	else
	    act("$n recharges $p from $s internal supply", 
		TRUE, ch, engine, vict, TO_ROOM);
    }
	if (GET_OBJ_TYPE(battery) == ITEM_BATTERY)
		amount -= RECH_RATE(battery);
	if (amount < 0) {amount = 0;}
	amount -= amount * CHECK_SKILL(ch,SKILL_OVERDRAIN) / 125;
	if (amount < 0) {amount = 0;}
    
	if ((battery && amount) && 
		( GET_OBJ_TYPE(battery) == ITEM_BATTERY 
		 || GET_OBJ_TYPE(battery) == ITEM_DEVICE) ) {
	sprintf(buf,"%sERROR: %s damaged during transfer!\r\n",QRED,battery->short_description);
    send_to_char(buf, ch);
	damage_eq(ch, battery, amount);
    }
}
 

ACMD(do_hotwire)
{
    send_to_char("Hotwire what?\r\n", ch);
    return;
}

ACMD(do_recharge)
{

    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH];
    struct obj_data *target = NULL, *battery = NULL;
    struct char_data *vict = NULL;
    int i;

    argument = two_arguments(one_argument(argument, arg1), arg2, arg3);

    if ((!*arg1 || ch == get_char_room_vis(ch, arg1)) && IS_CYBORG(ch)) {
	if (!(battery = GET_EQ(ch, WEAR_HOLD)) || !IS_BATTERY(battery)) {
	    send_to_char("You need to be holding a battery to do that.\r\n", ch);
	    return;
	}

	if (CUR_ENERGY(battery) <= 0) {
	    act("$p is depleted of energy.", FALSE, ch, battery, 0, TO_CHAR);
	    return;
	}

	if (GET_MOVE(ch) == GET_MAX_MOVE(ch)) {
	    send_to_char("You are already operating at maximum energy.\r\n", ch);
	    return;
	}

	perform_recharge(ch, battery, ch, NULL, 0);
	return;
    }

    if (*arg1 && !str_cmp(arg1, "internal")) {
	if (!*arg2 || !*arg3) {
	    send_to_char("USAGE:  Recharge internal <component> energy_source\r\n",
			 ch);
	    return;
	}

	if (!(target = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
	    sprintf(buf, "You are not implanted with %s '%s'.\r\n",
		    AN(arg2), arg2);
	    send_to_char(buf, ch);
	    return;
	}

	if (strncmp(arg3, "self", 4) &&
	    strncmp(arg3, "me", 2) &&
	    !(battery = get_obj_in_list_vis(ch, arg3, ch->carrying)) &&
	    !(battery = get_object_in_equip_vis(ch, arg3, ch->equipment, &i)) &&
	    !(battery = get_obj_in_list_vis(ch, arg3, ch->in_room->contents))) {
	    sprintf(buf, "You can't find any '%s' to recharge from.\r\n", arg3);
	    send_to_char(buf, ch);
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

		if (IS_IMPLANT(battery) 
			&& battery != get_object_in_equip_vis(ch, arg2, ch->implants, &i)) {
		act ("ERROR: $p not installed properly.", FALSE,ch,battery,0,TO_CHAR);
		return;
		}
	    }
	} else {
	    if (!IS_CYBORG(ch)) {
		send_to_char("You cannot recharge things from yourself.\r\n", ch);
		return;
	    }
	    if (GET_MOVE(ch) <= 0) {
		send_to_char("You do not have any energy to transfer.\r\n", ch);
		return;
	    }
	}

	if (!IS_CYBORG(ch)) {
	    for (i = 0; i < NUM_WEARS; i ++)
		if ((GET_IMPLANT(ch, i) && IS_INTERFACE(GET_IMPLANT(ch, i)) &&
		     INTERFACE_TYPE(GET_IMPLANT(ch, i)) == INTERFACE_POWER) ||
		    (GET_EQ(ch, i) && IS_INTERFACE(GET_EQ(ch, i)) &&
		     INTERFACE_TYPE(GET_EQ(ch, i)) == INTERFACE_POWER))
		    break;

	    if (i >= NUM_WEARS) {
		send_to_char("You are not using an appropriate power interface.\r\n", 
			     ch);
		return;
	    }
	}

	if (IS_ENERGY_GUN(target)) {
	    send_to_char("You can only restore the energy gun cells.\r\n", ch);
	    return;
	}
	if (!RECHARGABLE(target)) {
	    send_to_char("You can't recharge that!\r\n", ch);
	    return;
	}
	if (CUR_ENERGY(target) >= MAX_ENERGY(target)) {
	    act("$p is already fully energized.", FALSE, ch, target, 0, TO_CHAR);
	    return;
	}


	perform_recharge(ch, battery, NULL, target, 0);
	return;
    }  
    
    if (!*arg1 || !*arg2) {
	send_to_char("USAGE:  Recharge <component> energy_source\r\n",
		     ch);
	return;
    }

    if (!(target = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
	!(target = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
	!(target = get_obj_in_list_vis(ch, arg1, ch->in_room->contents)) &&
	!(vict = get_char_room_vis(ch, arg1))) {
	sprintf(buf, "You can't find %s '%s' here to recharge.\r\n", 
		AN(arg1), arg1);
	send_to_char(buf, ch);
	return;
    }

    if (strncmp(arg2, "self", 4) &&
	strncmp(arg2, "me", 2) &&
	!(battery = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
	!(battery = get_object_in_equip_vis(ch, arg2, ch->equipment, &i)) &&
	!(battery = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
	sprintf(buf, "You can't find %s '%s' here to recharge from.\r\n", 
		AN(arg2), arg2);
	send_to_char(buf, ch);
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
	    send_to_char("You cannot recharge things from yourself.\r\n", ch);
	    return;
	}
	if (GET_MOVE(ch) <= 0) {
	    send_to_char("You do not have any energy to transfer.\r\n", ch);
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
	    act("$p is fully energized.", FALSE, ch, target->contains, 0, TO_CHAR);
	else {
	    perform_recharge(ch, battery, NULL, target->contains, 0);
	}
	return;
    }

    if (IS_ENERGY_GUN(target)) {
	send_to_char("You can only restore the energy gun cells.\r\n", ch);
	return;
    }
    if (!RECHARGABLE(target)) {
	send_to_char("You can't recharge that!\r\n", ch);
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

void perform_cyborg_activate(CHAR *ch, int mode, int subcmd)
{
    struct affected_type af[2];
    CHAR *vict = NULL, *nvict = NULL;
    char *to_room[2], *to_char[2];

    if (!CHECK_SKILL(ch, mode))
	send_to_char("You do not have this program in memory.\r\n", ch);
    else if (CHECK_SKILL(ch, mode) < 40)
	send_to_char("Partial installation insufficient for operation.\r\n", ch);
    else {
      
	to_room[0] = NULL;
	to_room[1] = NULL;
	to_char[0] = NULL;
	to_char[1] = NULL;

	af[0].type = mode;
	af[0].bitvector = 0;
	af[0].duration = -1;
	af[0].location = APPLY_NONE;
	af[0].modifier = 0;
	af[0].aff_index = 1;
	af[0].level = GET_LEVEL(ch);

	af[1].type = 0;
	af[1].bitvector = 0;
	af[1].duration = -1;
	af[1].location = APPLY_NONE;
	af[1].modifier = 0;
	af[1].aff_index = 1;
	af[1].level = GET_LEVEL(ch);

	switch (mode) {
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
	case SKILL_OVERDRIVE:
	    af[0].bitvector = AFF2_HASTE;
	    af[0].aff_index = 2;
	    af[0].location = APPLY_MOVE;
	    af[0].modifier = -40;
      
	    to_char[1] = "Switching into overdrive mode.\r\n";
	    to_room[1] = "$n shifts into overdrive with a piercing whine.";
      
	    to_char[0] = "Switching into normal operation mode.\r\n";
	    to_room[0] = "$n switches out of overdrive.";

	    break;
	case SKILL_POWER_BOOST:
	    af[0].bitvector = 0;
	    af[0].location = APPLY_MOVE;
	    af[0].modifier = -40;
      
	    af[1].type = mode;
	    af[1].location = APPLY_STR;
	    af[1].modifier = 1 + (GET_LEVEL(ch) >> 4);

	    to_char[1] = "Power boost enabled.\r\n";
	    to_room[1] = "$n boosts power levels -- ohhh shit!";

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
	    af[0].modifier = -(30 + GET_LEVEL(ch));
      
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
	    if (subcmd) {           /************ activate ****************/
		if (GET_POS(ch) >= POS_FLYING)
		    send_to_char("I don't think so.\r\n", ch);
		else {
		    TOGGLE_BIT(AFF3_FLAGS(ch), AFF3_STASIS);
		    GET_POS(ch) = POS_SLEEPING;
		    WAIT_STATE(ch, PULSE_VIOLENCE*5);
		    send_to_char("Entering static state.  Halting system processes.\r\n", ch);
		    act("$n lies down and enters a static state.",FALSE,ch,0,0,TO_ROOM);
		}
	    } else {           /************ deactivate ****************/
		send_to_char("An error has occured.\r\n", ch);
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

	default:
	    send_to_char("ERROR: Unknown mode occured in switch.\r\n", ch);
	    return;
	    break;
	}

	if ((subcmd && affected_by_spell(ch, mode)) ||
	    (!subcmd && !affected_by_spell(ch, mode))) {
	    sprintf(buf, "%sERROR:%s %s %s %s activated.\r\n", CCCYN(ch, C_NRM),
		    CCNRM(ch, C_NRM), spells[mode], ISARE(spells[mode]),
		    subcmd ? "already" : "not currently");
	    send_to_char(buf, ch);
	    return;
	}

	if (subcmd) {

	    if ((af[0].location==APPLY_MOVE && (GET_MOVE(ch) + af[0].modifier) < 0) ||
		(af[1].location==APPLY_MOVE && (GET_MOVE(ch) + af[1].modifier) < 0)) {
		sprintf(buf, "%sERROR:%s Energy levels too low to activate %s.\r\n",
			CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), spells[mode]);
		send_to_char(buf, ch);
		return;
	    }
      
	    affect_join(ch, &af[0], 0, FALSE, 1, FALSE);
	    if (af[1].type)
		affect_join(ch, &af[1], 0, FALSE, 1, FALSE);

	    if (to_char[1])
		send_to_char(to_char[1], ch);
	    if (to_room[1])
		act(to_room[1], FALSE, ch, 0, 0, TO_ROOM);
      
	    if (mode == SKILL_ENERGY_FIELD && 
		(SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
		 SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
		 SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)) {
		for (vict=ch->in_room->people;vict;vict=nvict) {
		    nvict = vict->next_in_room;
		    if (vict == ch)
			continue;
		    damage(ch, vict, dice(4, GET_LEVEL(ch)), SKILL_ENERGY_FIELD,-1);
		}
		send_to_char("ERROR: Dangerous short!!  Energy fields shutting down.\r\n", ch);
		affect_from_char(ch, mode);

	    }

	} else {
	    affect_from_char(ch, mode);
	    if (to_char[0])
		send_to_char(to_char[0], ch);
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
    struct room_data * targ_room = NULL;
    bool internal = false;

    skip_spaces(&argument);  

    if (!*argument) {
	sprintf(buf, "%s what?\r\n", CMD_NAME);
	send_to_char(CAP(buf), ch);
	return;
    }
    if (IS_CYBORG(ch)) {
	for (i = MAX_SPELLS; i < MAX_SKILLS; i++)
	    if (!strncasecmp(argument, spells[i], strlen(argument) - 1) &&
		IS_SET(spell_info[i].routines, CYB_ACTIVATE)) {
		mode = i;
		break;
	    }
    }

    if (!mode) {          /*** not a skill activation ***/
	argument = two_arguments(argument, buf, buf2);
	if (!*buf) {
	    send_to_char("Activate what?\r\n", ch);
	    return;
	}
	if (!strncmp(buf, "internal", 7)) {
	    internal = true;
	    if (!*buf2) {
		send_to_char("Which implant?\r\n", ch);
		return;
	    }
	    if (!(obj = get_object_in_equip_all(ch, buf2, ch->implants, &i))) {
		sprintf(buf, "You are not implanted with %s '%s'.\r\n",
			AN(buf2), buf2);
		send_to_char(buf, ch);
		return;
	    }
	} else if (!(obj = get_object_in_equip_vis(ch, buf, ch->equipment, &i)) &&
		   !(obj = get_obj_in_list_vis(ch, buf, ch->carrying)) &&
		   !(obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
	    sprintf(buf2, "You don't seem to have %s '%s'.\r\n", AN(buf), 
		    buf);
	    send_to_char(buf2, ch);
	    return;
	}

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_DEVICE:
	    if (!subcmd) {
		if (!ENGINE_STATE(obj))
		    act("$p is already switched off.", FALSE, ch, obj, 0, TO_CHAR);
		else {
		    sprintf(buf, "You deactivate $p%s.", internal ? " (internal)" : "");
		    act(buf, FALSE, ch, obj, 0, TO_CHAR);
		    sprintf(buf, "$n deactivates $p%s.", internal ? " (internal)" : "");
		    act(buf, TRUE, ch, obj, 0, TO_ROOM);
		    ENGINE_STATE(obj) = 0;
		    if (obj->worn_by && 
			(!IS_IMPLANT(obj) || internal)) {
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			    affect_modify(obj->worn_by, obj->affected[i].location,
					  obj->affected[i].modifier, 0, 0, FALSE);
			affect_modify(obj->worn_by, 0, 0, 
				      obj->obj_flags.bitvector[0], 1, FALSE);
			affect_modify(obj->worn_by, 0, 0, 
				      obj->obj_flags.bitvector[1], 2, FALSE);
			affect_modify(obj->worn_by, 0, 0, 
				      obj->obj_flags.bitvector[2], 3, FALSE);
			affect_total(obj->worn_by);
		    }
		}
	    } else {
		if (ENGINE_STATE(obj))
		    act("$p is already switched on.", FALSE, ch, obj, 0, TO_CHAR);
		else if (CUR_ENERGY(obj) < USE_RATE(obj))
		    act("The energy levels of $p are too low.",FALSE,ch,obj,0,TO_CHAR);
		else {
		    sprintf(buf, "You activate $p%s.", internal ? " (internal)" : "");
		    act(buf, FALSE, ch, obj, 0, TO_CHAR);
		    sprintf(buf, "$n activates $p%s.", internal ? " (internal)" : "");
		    act(buf, TRUE, ch, obj, 0, TO_ROOM);
		    if (obj->action_description && ch == obj->worn_by)
			act(obj->action_description, FALSE, ch, obj, 0, TO_CHAR);
		    CUR_ENERGY(obj) -= USE_RATE(obj);
		    ENGINE_STATE(obj) = 1;
		    if (obj->worn_by &&
			(!IS_IMPLANT(obj) || internal)) {
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			    affect_modify(obj->worn_by, obj->affected[i].location,
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
		send_to_char("You cannot figure out how.\r\n", ch);
		return;
	    }
	    if (!subcmd) {
		send_to_char("You cannot deactivate it.\r\n", ch);
		return;
	    }
	
	    /* charmed? */
	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master && 
		ch->in_room == ch->master->in_room) {
		send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
		if (IS_UNDEAD(ch))
		    act("$n makes a hollow moaning sound.", FALSE, ch, 0, 0, TO_ROOM);
		else
		    act("$n looks like $e wants to use $p.", FALSE, ch, obj, 0, TO_ROOM);
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
		send_to_char("A warning indicator blinks on:\r\n"
			     "Transporter is tuned to a deadly area.\r\n", ch);
	    else if ((ROOM_FLAGGED(targ_room, ROOM_GODROOM | ROOM_NORECALL |
				   ROOM_NOTEL |
				   ROOM_NOMAGIC) && GET_LEVEL(ch) < LVL_GRGOD))
		send_to_char("Transporter ERROR: Unable to transport.\r\n", ch);
	    else if (targ_room->zone->plane != ch->in_room->zone->plane)
		send_to_char("An indicator flashes on:\r\n"
			     "Transporter destination plane does not intersect current spacial field.\r\n", ch);
	    else if ((ROOM_FLAGGED(ch->in_room, ROOM_NORECALL) ||
		      (targ_room->zone != ch->in_room->zone &&
		       (ZONE_FLAGGED(targ_room->zone, ZONE_ISOLATED) ||
			ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)))) &&
		     GET_LEVEL(ch) < LVL_AMBASSADOR) {
		act("You flip a switch on $p...\r\n"
		    "Only to be slammed down by an invisible force!!!",
		    FALSE, ch, obj, 0, TO_CHAR);
		act("$n flips a switch on $p...\r\n"
		    "And is slammed down by an invisible force!!!",
		    FALSE, ch, obj, 0, TO_ROOM);
	    } else {
		CUR_ENERGY(obj) -= mass;

		send_to_char("You flip a switch and disappear.\r\n", ch);
		act("$n flips a switch on $p and fades out of existance.",
		    TRUE, ch, obj, 0, TO_ROOM);

		char_from_room(ch);
		char_to_room(ch, targ_room);

		send_to_char("A buzzing fills your ears as you materialize...\r\n",ch); 
		act("You hear a buzzing sound as $n fades into existance.",
		    FALSE,ch, 0, 0, TO_ROOM);
		look_at_room(ch, ch->in_room, 0);

		if (ROOM_FLAGGED(ch->in_room, ROOM_NULL_MAGIC) &&
		    (GET_LEVEL(ch) < LVL_ETERNAL || 
		     (IS_CYBORG(ch) && !IS_MAGE(ch) && 
		      !IS_CLERIC(ch) && !IS_KNIGHT(ch) && !IS_RANGER(ch)))) {
		    if (ch->affected) {
			send_to_char("You are dazed by a blinding flash inside your"
				     " brain!\r\nYou feel different...\r\n", ch);
			act("Light flashes from behind $n's eyes.",FALSE,ch,0,0,TO_ROOM);
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
		    act("$p is already switched off.", FALSE, ch, obj, 0, TO_CHAR);
		else {
		    act("You close the air valve on $p.",FALSE,ch,obj,0,TO_CHAR);
		    act("$n adjusts an air valve on $p.",TRUE,ch,obj,0,TO_ROOM);
		    GET_OBJ_VAL(obj, 0) = 0;
		}
	    } else {
		if (GET_OBJ_VAL(obj, 0))
		    act("$p is already switched on.", FALSE, ch, obj, 0, TO_CHAR);
		else {
		    act("You open the air valve on $p.",FALSE,ch,obj,0,TO_CHAR);
		    act("$n adjusts an air valve on $p.",TRUE,ch,obj,0,TO_ROOM);
		    GET_OBJ_VAL(obj, 0) = 1;
		}
	    }
	    break;

	case ITEM_BOMB:
	    if (!obj->contains || !IS_FUSE(obj->contains)) {
		act("$p is not fitted with a fuse.", FALSE, ch, obj, 0, TO_CHAR);
	    } else if (!subcmd) {
		if (!FUSE_STATE(obj->contains))
		    act("$p has not yet been activated.", 
			FALSE, ch, obj->contains, 0, TO_CHAR);
		else {
		    act("You deactivate $p.",
			FALSE, ch, obj, 0, TO_CHAR);
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
		act("$p is not tuned to any explosives.",FALSE,ch,obj,0,TO_CHAR);
	    else if (!IS_BOMB(obj->aux_obj))
		send_to_char("Activate detonator error.  Please report.\r\n", ch);
	    else if (obj != obj->aux_obj->aux_obj)
		send_to_char("Bomb error.  Please report\r\n", ch);
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
		send_to_char("ok.\r\n", ch);
		obj->aux_obj->aux_obj = NULL;
		obj->aux_obj = NULL;
	    }
	    break;

	case ITEM_COMMUNICATOR:
	    if (subcmd) {
		if (ENGINE_STATE(obj)) {
		    act("$p is already switched on.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (CUR_ENERGY(obj) == 0) {
		    act("$p is deleted of energy.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
		    act("$n switches $p on.", TRUE, ch, obj, 0, TO_ROOM);
		    act("You switch $p on.", TRUE, ch, obj, 0, TO_CHAR);
		    ENGINE_STATE(obj) = 1;
		    sprintf(buf,"$n has joined channel [%d].",COMM_CHANNEL(obj));
		    send_to_comm_channel(ch, buf, COMM_CHANNEL(obj), TRUE, TRUE);
		}
	    } else {
		if (!ENGINE_STATE(obj)) {
		    act("$p is already switched off.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
		    act("$n switches $p off.", TRUE, ch, obj, 0, TO_ROOM);
		    act("You switch $p off.", TRUE, ch, obj, 0, TO_CHAR);
		    ENGINE_STATE(obj) = 0;
		    sprintf(buf,"$n has left channel [%d].",COMM_CHANNEL(obj));
		    send_to_comm_channel(ch, buf, COMM_CHANNEL(obj), TRUE, TRUE);
		}
	    }
	    break;
	default:
	    send_to_char("You cannot figure out how.\r\n", ch);
	    break;
	}
	return;
    }

    perform_cyborg_activate(ch, mode, subcmd);

 
}   

ACMD(do_cyborg_reboot)
{

    if (!IS_CYBORG(ch)) {
	send_to_char("Uh, sure...\r\n", ch);
	return;
    }
    send_to_char("Systems shutting down...\r\n"
		 "Sending all processes the TERM signal.\r\n"
		 "Unmounting filesystems.... done.\r\n"
		 "Reboot succesful.\r\n", ch);
    if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 10) {
	sprintf(buf, "Fastboot script enabled... %d %% efficiency.\r\n", 
		CHECK_SKILL(ch, SKILL_FASTBOOT));
	send_to_char(buf, ch);
    }

    act("$n begins a reboot sequence and shuts down.", FALSE, ch, 0, 0, TO_ROOM);

    if (FIGHTING(ch))
	stop_fighting(ch);

    GET_POS(ch) = POS_SLEEPING;

    if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 90) {
	WAIT_STATE(ch, PULSE_VIOLENCE*4);
    } else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 80) {
	WAIT_STATE(ch, PULSE_VIOLENCE*5);
    } else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 70) {
	WAIT_STATE(ch, PULSE_VIOLENCE*6);
    } else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 60) {
	WAIT_STATE(ch, PULSE_VIOLENCE*7);
    } else if (CHECK_SKILL(ch, SKILL_FASTBOOT) > 50) {
	WAIT_STATE(ch, PULSE_VIOLENCE*8);
    } else 
	WAIT_STATE(ch, PULSE_VIOLENCE*10);

    if (ch->affected)
	send_to_char("Purging system....\r\n", ch);

    while (ch->affected && GET_MANA(ch)) {
	affect_remove(ch, ch->affected);
	GET_MANA(ch) = MAX(GET_MANA(ch) - 10, 0);
    }

    if (GET_COND(ch, DRUNK) > 0)
	GET_COND(ch, DRUNK) = 0;

    gain_skill_prof(ch, SKILL_FASTBOOT);
}


void 
OLD_engage_self_destruct(struct char_data *ch)
{
  
    int level, rhit, dir, i;
    struct char_data *vict = NULL, *n_vict = NULL;
    struct obj_data *obj = NULL, *n_obj = NULL;
    struct room_data * toroom = NULL;

    send_to_char("Self-destruct point reached.  Stand by for termination...\r\n", ch);
    act("$n suddenly explodes into a thousand fragments!", FALSE,ch,0,0,TO_ROOM);  
    level = GET_LEVEL(ch);
    rhit = (GET_HIT(ch) >> 1);

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
	act("You are showered with a rain of fiery debris!", FALSE,ch,0,0,TO_ROOM);
    } else {
	for (vict = ch->in_room->people; vict;  vict = n_vict) {
	    n_vict = vict->next_in_room;

	    if (ch == vict)
		continue;

	    damage(ch, vict, dice(level, 24) + rhit, SKILL_SELF_DESTRUCT,-1);
	    GET_POS(vict) = POS_SITTING;
	}
    }
    for (dir = 0; dir < 8; dir++) {
	if (EXIT(ch, dir) && ((toroom = EXIT(ch, dir)->to_room) != NULL)) {
	    if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR)) {
		if (IS_SET(EXIT(ch, dir)->exit_info, EX_PICKPROOF) ||
		    (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) &&
		     number(0, level) < 10) ||
		    (IS_SET(EXIT(ch, dir)->exit_info, EX_LOCKED) &&
		     number(0, level) < 10) ||
		    (IS_SET(EXIT(ch, dir)->exit_info, EX_HEAVY_DOOR) &&
		     number(0, level) < 30))
		    continue;
	
		REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_CLOSED);
		REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_LOCKED);
	
		if (IS_SET(ABS_EXIT(toroom, rev_dir[dir])->exit_info, EX_ISDOOR)) {
		    if (IS_SET(ABS_EXIT(toroom, rev_dir[dir])->exit_info, EX_CLOSED)) {
			sprintf(buf, "The %s is blown open by an explosion from %s!\r\n",
				ABS_EXIT(toroom, rev_dir[dir])->keyword ?
				fname(ABS_EXIT(toroom, rev_dir[dir])->keyword) : "door",
				from_dirs[dir]);
			send_to_room(buf, toroom);
			REMOVE_BIT(ABS_EXIT(toroom, rev_dir[dir])->exit_info, EX_CLOSED);
			REMOVE_BIT(ABS_EXIT(toroom, rev_dir[dir])->exit_info, EX_LOCKED);
		    } else {
			sprintf(buf, "You are deafened by an explosion from %s!\r\n",
				from_dirs[dir]);
			send_to_room(buf, toroom);
		    }
		    continue;
		}
	    } else {
		sprintf(buf, "There is a deafening explosion from %s!\r\n",
			from_dirs[dir]);
		send_to_room(buf, toroom);
		for (vict = toroom->people; vict; vict = n_vict) {
		    n_vict = vict->next_in_room;
		    if (GET_STR(vict) < number(3, level)) {
			send_to_char("You are blown to your knees by the explosive shock!\r\n", vict);
			if (FIGHTING(vict))
			    stop_fighting(vict);
			GET_POS(vict) = POS_SITTING;
		    }
		}
	    }
	}
    }
    for (i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(ch, i))
	    obj_to_room(unequip_char(ch, i, MODE_EQ), ch->in_room);

    for (obj = ch->carrying; obj; obj = n_obj) {
	n_obj = obj->next_content;

	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);
    }
    if (!mag_savingthrow(ch, LVL_GRIMP-GET_LEVEL(ch), SAVING_PARA))
	ch->real_abils.con--;

    GET_HIT(ch) = 0;
    GET_MOVE(ch) = MAX(100, GET_MAX_MOVE(ch));

    gain_skill_prof(ch, SKILL_SELF_DESTRUCT);
    sprintf(buf, "%s self-destructed at room #%d.", GET_NAME(ch), 
	    ch->in_room->number);
    mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
    die(ch, ch, SKILL_SELF_DESTRUCT, FALSE);
}

ACMD(do_self_destruct)
{

    int countdown = 0;
    skip_spaces(&argument);

    if ( !IS_CYBORG(ch) || GET_SKILL(ch, SKILL_SELF_DESTRUCT) < 50 ) {
	send_to_char("You explode.\r\n", ch);
	act("$n self destructs, spraying you with an awful mess!",
	    FALSE, ch, 0, 0, TO_ROOM);
    } else if (!*argument) {
	send_to_char("You must provide a countdown time, in 3-second pulses.\r\n", ch);
    } else if (!is_number(argument)) {
	if (!str_cmp(argument, "abort")) {
	    if (!AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
		send_to_char("Self-destruct sequence not currently initiated.\r\n", ch);
	    } else {
		REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
		sprintf(buf, "Sending process the KILL Signal.\r\n"
			"Self-destruct sequence terminated with %d pulses remaining.\r\n",
			MEDITATE_TIMER(ch));
		send_to_char(buf, ch);
		MEDITATE_TIMER(ch) = 0;
	    }
	} else   
	    send_to_char("Self destruct what??\r\n", ch);
    } else if ((countdown = atoi(argument)) < 0 || countdown > 100) {
	send_to_char("Selfdestruct countdown argument out of range.\r\n", ch);
    } else {
	if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
	    send_to_char("Self-destruct sequence currently underway.\r\n"
			 "Use 'selfdestruct abort' to abort process!\r\n", ch);
	} else if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
	    act("You fear that your death will grieve $N.",
		FALSE, ch, 0, ch->master, TO_CHAR);
	    return;
	} else {
	    sprintf(buf, "Self-destruct sequence initiated at countdown level %d.\r\n",
		    countdown);
	    send_to_char(buf, ch);
	    SET_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
	    MEDITATE_TIMER(ch) = countdown;
	    if (!countdown)
		engage_self_destruct(ch);
	}
    }
}

ACMD(do_bioscan)
{
  
    struct char_data *vict = NULL;
    int count = 0;

    if (!IS_CYBORG(ch)) {
	send_to_char("You are not equipped with bio-scanners.\r\n", ch);
	return;
    }

    for (vict = ch->in_room->people; vict; vict = vict->next_in_room) {
	if ((((CAN_SEE(ch, vict) || GET_INVIS_LEV(vict) < GET_LEVEL(ch)) &&
	      (CHECK_SKILL(ch, SKILL_BIOSCAN) > number(30, 100) ||
	       affected_by_spell(ch, SKILL_HYPERSCAN))) || ch == vict) &&
	    LIFE_FORM(vict))
	    count++;
    }
  
    sprintf(buf, "%sBIOSCAN:%s %d life form%s detected in room.\r\n", 
	    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), count, count > 1 ? "s" : "");
    send_to_char(buf, ch);
    act("$n scans the room for lifeforms.", FALSE,ch,0,0,TO_ROOM);

    if (count > number(3, 5))
	gain_skill_prof(ch, SKILL_BIOSCAN);
  
}
ACMD(do_discharge)
{

    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent, prob, amount, dam;
	int feedback=0;
	int tolerance=0;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    half_chop(argument, arg1, arg2);
  
    if (!IS_CYBORG(ch)) {
	send_to_char("You discharge some smelly gas.\r\n", ch);
	act("$n discharges some smelly gas.",FALSE,ch,0,0,TO_ROOM);
	return;
    }
  
    if (CHECK_SKILL(ch, SKILL_DISCHARGE) < 20) {
	send_to_char("You are unable to discharge.\r\n", ch);
	return;
    }

    if (!*arg1) {
	send_to_char("Usage: discharge <energy discharge amount> <victim>\r\n",ch);
	return;
    }

    if (!(vict = get_char_room_vis(ch, arg2)) && 
	!(ovict = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
	if (FIGHTING(ch)) {
	    vict = FIGHTING(ch);
	} else {
	    send_to_char("Discharge into who?\r\n", ch);
	    return;
	}
    }

    if (!is_number(arg1)) {
	send_to_char("The discharge amount must be a number.\r\n", ch);
	return;
    }
    amount = atoi(arg1);

    if (amount > GET_MOVE(ch)) {
	send_to_char("ERROR: Energy levels too low for requested discharge.\r\n", ch);
	return;
    }

    if (amount < 0) {
	send_to_char("Discharge into who?\r\n", ch);
	sprintf(buf, "%s neg-discharge %d %s at %d", GET_NAME(ch), 
		amount, vict ? GET_NAME(vict) : ovict->short_description, 
		ch->in_room->number);
	mudlog(buf, NRM, LVL_GRGOD, TRUE);
	return;
    }

    GET_MOVE(ch) -= amount;

	// Tolerance is the amount they can safely discharge.
	tolerance = GET_LEVEL(ch) / 6 + dice( GET_LEVEL(ch) / 8, 4);

	
	if(amount > tolerance) {
		if (amount > (tolerance * 2)) {
			send_to_char("ERROR: Discharge amount far exceeds acceptable parameters. Aborted.\r\n.",ch);
			return;
		}
		// Give them some idea of how much they went overboard by.
		sprintf (buf,"WARNING: Voltage tolerance exceeded by %d%%.\r\n",
			((amount * 100) - (tolerance * 100)) / tolerance );
		send_to_char(buf,ch);
		// Amount of component damage delt to cyborg.
		feedback = dice(amount - tolerance,4);
		feedback *= max_component_dam(ch) / 100;
	
		// Random debug messages.
		if ( PRF2_FLAGGED( ch, PRF2_FIGHT_DEBUG ) ) {
			sprintf(buf,"Tolerance: %d, Amount: %d, Feedback: %d\r\n",
				tolerance, amount, feedback);
			send_to_char(buf,ch);
		}
		if (GET_TOT_DAM(ch) + feedback >= max_component_dam(ch))
			GET_TOT_DAM(ch) = max_component_dam(ch);
		else
			GET_TOT_DAM(ch) += feedback;
		damage(ch, ch, dice(amount - tolerance, 10), TYPE_OVERLOAD,-1);

		if(GET_TOT_DAM(ch) == 0 || GET_TOT_DAM(ch) == max_component_dam(ch)) {
			send_to_char("ERROR: Component failure. Discharge failed.\r\n",ch);
			return;
		}
		send_to_char("WARNING: System components damaged by discharge!\r\n",ch);

	}

    if (ovict) {
	act("$n blasts $p with a stream of pure energy!!",
	    FALSE, ch,ovict,0,TO_ROOM);
	act("You blast $p with a stream of pure energy!!",
	    FALSE, ch,ovict,0,TO_CHAR);
    
	dam = amount * GET_LEVEL(ch) + dice(GET_INT(ch), 4);
	damage_eq(ch, ovict, dam);
	return;
    }

    if (vict == ch) {
	send_to_char("Let's not try that shall we...\r\n", ch);
	return;
    }
    if (!peaceful_room_ok(ch, vict, true))
	return;

    percent = 
	((10 - (GET_AC(vict) / 10)) >> 1) + number(1, 91);
							
    prob = CHECK_SKILL(ch, SKILL_DISCHARGE);


	dam = dice(amount * 3, 20 + GET_REMORT_GEN(ch));

    if (percent > prob) {
		damage(ch, vict, 0, SKILL_DISCHARGE,-1);
    } else {
		damage(ch, vict, dam, SKILL_DISCHARGE,-1);
		gain_skill_prof(ch, SKILL_DISCHARGE);
    }
    WAIT_STATE(ch, (20 + (amount / 100)));
}


ACMD(do_tune)
{
    struct obj_data *obj = NULL, *obj2 = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct room_data * to_room = NULL;
    sh_int count = 0;
    int i;
    bool internal = false;

    skip_spaces(&argument);

    if (!*argument) {
	send_to_char("Tune what?\r\n", ch);
	return;
    }

    argument = two_arguments(argument, arg1, arg2);

    if (!strncmp(arg1, "internal", 8)) {
	internal = true;
	strcpy(arg1, arg2);
	if (!(obj = get_object_in_equip_vis(ch, arg1, ch->implants, &i))) {
	    sprintf(buf, "You are not implanted with '%s'.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}
	one_argument(argument, arg2);

    } else {

	if (!(obj = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
	    !(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
	    !(obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
	    sprintf(buf, "You cannot find %s '%s' to tune.\r\n", AN(arg1), arg1);
	    send_to_char(buf, ch);
	    return;
	} 
    }

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_TRANSPORTER:
	if (CHECK_SKILL(ch, SKILL_ELECTRONICS) < number(10, 50))
	    send_to_char("You can't figure it out.\r\n", ch);
	else if (!GET_OBJ_VAL(obj, 3))
	    act("$p is not tunable.", FALSE, ch, obj, 0, TO_CHAR);
	else if (!CUR_ENERGY(obj))
	    act("$p is depleted of energy.", FALSE, ch, obj, 0, TO_CHAR);
	else {
	    CUR_ENERGY(obj) = MAX(0, CUR_ENERGY(obj) - 10);
	    if (!CUR_ENERGY(obj))
		act("$p buzzes briefly and falls silent.", FALSE,ch,obj,0,TO_CHAR);
	    else {
		if ((CHECK_SKILL(ch, SKILL_ELECTRONICS) + GET_INT(ch)) <
		    number(50, 100)) {
		    do {
			to_room = real_room(number(ch->in_room->zone->number*100, 
						   ch->in_room->zone->top));
			count++;
		    } while (count < 1000 && 
			     (!to_room || to_room->zone != ch->in_room->zone ||
			      ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NOMAGIC | 
					   ROOM_NOTEL|ROOM_NORECALL|ROOM_DEATH) ||
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
	    send_to_char("You can't figure it out.\r\n", ch);
	else if (!*arg2)
	    act("Tune $p to what bomb?", FALSE, ch, obj, 0, TO_CHAR);
	else {
	    if (!(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
		!(obj2 = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
		sprintf(buf, "You don't see %s '%s' here.\r\n", AN(arg2), arg2);
		send_to_char(buf, ch);
		return;
	    }
	    if (!IS_BOMB(obj2))
		send_to_char("You can only tune detonators to explosives.\r\n", ch);
	    if (!obj2->contains || !IS_FUSE(obj2->contains) || 
		!FUSE_IS_REMOTE(obj2->contains))
		act("$P is not fused with a remote activator.",
		    FALSE, ch, obj, obj2, TO_CHAR);
	    else if (obj->aux_obj) {
		if (obj->aux_obj == obj2->aux_obj)
		    act("$p is already tuned to $P.", FALSE, ch, obj, obj2, TO_CHAR);
		else
		    act("$p is already tuned to something.", FALSE,ch,obj,obj2,TO_CHAR);
	    } else if (obj2->aux_obj)
		act("Another detonator is already tuned to $P.", 
		    FALSE,ch,obj,obj2,TO_CHAR);
	    else {
		obj->aux_obj = obj2;
		obj2->aux_obj = obj;
		act("You tune $p to $P.", FALSE,ch,obj,obj2,TO_CHAR);
	    }
	}
	break;
    case ITEM_COMMUNICATOR:
	if (!*arg2)
	    send_to_char("Tune to what channel?\r\n", ch);
	else {
	    i = atoi(arg2);
	    if (i == COMM_CHANNEL(obj)) {
		act("$p is already tuned to that channel.", FALSE,ch,obj,0,TO_CHAR);
		break;
	    }
	    sprintf(buf,"$n has left channel [%d].",COMM_CHANNEL(obj));
	    send_to_comm_channel(ch, buf, COMM_CHANNEL(obj), TRUE, TRUE);

	    COMM_CHANNEL(obj) = i;
	    sprintf(buf, "%s comm-channel set to [%d].\n", obj->short_description,
		    COMM_CHANNEL(obj));
	    send_to_char(buf, ch);
	    sprintf(buf,"$n has joined channel [%d].",COMM_CHANNEL(obj));
	    send_to_comm_channel(ch, buf, COMM_CHANNEL(obj), TRUE, TRUE);
	}
	break;
    default:
	send_to_char("You cannot tune that!\r\n", ch);
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
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct affected_type *aff = NULL;

    skip_spaces(&argument);
    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
	if (!IS_CYBORG(ch))
	    send_to_char("Status of what?\r\n", ch);
	else {
	    sprintf(buf, "%s+++++>>>---    SYSTEM STATUS REPORT   ---<<<+++++%s\r\n",
		    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    sprintf(buf, "%s%sHITP%s:                 %3d percent   (%4d/%4d)\r\n",buf,
		    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
		    (GET_HIT(ch) * 100) / GET_MAX_HIT(ch),
		    GET_HIT(ch), GET_MAX_HIT(ch));
	    sprintf(buf, "%s%sMANA%s:                 %3d percent   (%4d/%4d)\r\n",buf,
		    CCMAG(ch, C_NRM), CCNRM(ch, C_NRM),
		    (GET_MANA(ch) * 100) / GET_MAX_MANA(ch),
		    GET_MANA(ch), GET_MAX_MANA(ch));
	    sprintf(buf, "%s%sMOVE%s:                 %3d percent   (%4d/%4d)\r\n", buf,
		    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
		    (GET_MOVE(ch) * 100) / GET_MAX_MOVE(ch),
		    GET_MOVE(ch), GET_MAX_MOVE(ch));
	    sprintf(buf, "%s%s+++++>>>---   ---   ---   ---   ---   ---<<<+++++%s\r\n", buf,
		    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));

	    sprintf(buf,"%sSystems have sustained %d percent of maximum damage.\r\n",
		    buf, (GET_TOT_DAM(ch) * 100) / max_component_dam(ch));
	    if (GET_BROKE(ch))
		sprintf(buf, "%s%sYour %s has been severely damaged.%s\r\n", buf, 
			CCRED(ch, C_NRM), component_names[GET_BROKE(ch)][GET_OLD_CLASS(ch)], CCNRM(ch, C_NRM));
	    if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT))
		sprintf(buf, "%s%sSELF-DESTRUCT sequence initiated.%s\r\n", buf,
			CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
	    if (AFF3_FLAGGED(ch, AFF3_STASIS))
		strcat(buf, "Systems are in static state.\r\n");
	    if (affected_by_spell(ch, SKILL_MOTION_SENSOR))
		strcat(buf, "Your motion sensors are active.\r\n");
	    if (affected_by_spell(ch, SKILL_ENERGY_FIELD))
		strcat(buf, "Energy fields are activated.\r\n");
	    if (affected_by_spell(ch, SKILL_OVERDRIVE))
		strcat(buf, "You are operating in Overdrive.\r\n");
	    if (affected_by_spell(ch, SKILL_POWER_BOOST))
		strcat(buf, "Power levels are boosted.\r\n");
	    if (AFF_FLAGGED(ch, AFF_INFRAVISION))
		strcat(buf, "Infrared detection system active.\r\n");
	    if (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
		strcat(buf, "Your sonic imagery device is active.\r\n");
	    if (affected_by_spell(ch, SKILL_DAMAGE_CONTROL))
		strcat(buf, "Damage Control systems are operating.\r\n");
	    if ( affected_by_spell(ch, SKILL_HYPERSCAN) )
		strcat(buf, "Hyperscanning device is active.\r\n");
	    if ( affected_by_spell(ch, SKILL_RADIONEGATION) )
		strcat(buf, "Radionegation device is operating.\r\n");
	    if (affected_by_spell(ch, SKILL_ASSIMILATE)) {
		sprintf(buf, "%s%sCurrently active assimilation affects:%s\r\n", buf, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

		for (i = 0, aff = ch->affected; aff; aff = aff->next) {
		    if (aff->type == SKILL_ASSIMILATE) {
		  
			sprintf(buf, "%s  %s%15s%s  %s %3d\r\n", buf, 
				CCGRN(ch, C_NRM), apply_types[aff->location], CCNRM(ch, C_NRM),
				aff->modifier >= 0 ? "+" : "-", abs(aff->modifier));

		    }
		}
	    }
		 
      
	    page_string(ch->desc, buf, 1);
	}
	return;
  
    } 

    if (!strncmp(arg1, "internal", 8)) {
    
	if (!*arg2) {
	    send_to_char("Status of which implant?\r\n", ch);
	    return;
	}
    
	if (!(obj = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
	    sprintf(buf, "You are not implanted with %s '%s'.\r\n",AN(arg2),arg2);
	    send_to_char(buf, ch);
	    return;
	}
    
    } else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))     &&
	       !(obj=get_object_in_equip_all(ch,arg1,ch->equipment,&i))  &&
	       !(obj=get_obj_in_list_vis(ch, arg1,ch->in_room->contents))) {
	sprintf(buf, "You can't seem to find %s '%s'.\r\n",AN(arg1),arg1);
	send_to_char(buf, ch);
	return;
    } 
  
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_TRANSPORTER:
	sprintf(buf,"Energy Levels: %d / %d.\r\n",CUR_ENERGY(obj),MAX_ENERGY(obj));
	send_to_char(buf, ch);
	if (real_room(TRANS_TO_ROOM(obj)) == NULL)
	    act("$p is currently untuned.", FALSE, ch, obj, 0, TO_CHAR);
	else {
	    sprintf(buf, "Tuned location: %s%s%s\r\n", 
		    CCCYN(ch, C_NRM), (CHECK_SKILL(ch, SKILL_ELECTRONICS) +
				       GET_INT(ch)) > number(20, 90) ? 
		    (real_room(TRANS_TO_ROOM(obj)))->name : "UNKNOWN",
		    CCNRM(ch, C_NRM));
	    send_to_char(buf, ch);
	}
	break;

    case ITEM_SCUBA_TANK:
	sprintf(buf, "$p %s currently holding %d percent of capacity.",
		ISARE(obj->short_description), GET_OBJ_VAL(obj, 0) < 0 ?
		100 : GET_OBJ_VAL(obj, 0) == 0 ? 0 : (GET_OBJ_VAL(obj, 1)
						      /GET_OBJ_VAL(obj, 0)
						      * 100));
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	break;

    case ITEM_SCUBA_MASK:
	sprintf(buf, "The valve on $p is currently %s.",
		GET_OBJ_VAL(obj, 0) ? "open" : "closed");
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	break;

    case ITEM_DEVICE:
	sprintf(buf, "$p (%s) %s currently %sactivated.\r\n"
		"The energy level is %d/%d units.", 
		obj->carried_by ? "carried" :
		obj->worn_by ? 
		(obj == GET_EQ(ch, obj->worn_on) ? "worn" : "implanted") : "here",
		ISARE(OBJS(obj, ch)), 
		ENGINE_STATE(obj) ? "" : "de",
		CUR_ENERGY(obj), MAX_ENERGY(obj));
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	break;

    case ITEM_DETONATOR:
	if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) < 50)
	    send_to_char("You have no idea.\r\n", ch);
	else if (obj->aux_obj)
	    act("$p is currently tuned to $P.",FALSE,ch,obj,obj->aux_obj,TO_CHAR);
	else
	    act("$p is currently untuned.", FALSE,ch,obj,obj->aux_obj,TO_CHAR);
	break;

    case ITEM_BOMB:
	if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) < 50)
	    send_to_char("You have no idea.\r\n", ch);
	else {
	    sprintf(buf, "$p is %sfused and %sactive.",
		    (obj->contains && IS_FUSE(obj->contains)) ? "" : "un",
		    (obj->contains && FUSE_STATE(obj->contains)) ? "" : "in");
	    act(buf, FALSE, ch, obj, 0, TO_CHAR);
	}
	break;
    case ITEM_ENERGY_CELL:
    case ITEM_BATTERY:
	sprintf(buf,"Energy Levels: %d / %d.\r\n",CUR_ENERGY(obj),MAX_ENERGY(obj));
	send_to_char(buf, ch);
	break;

    case ITEM_GUN:
    case ITEM_ENERGY_GUN:
	show_gun_status(ch, obj);
	break;

    case ITEM_CLIP:

	for (bul = obj->contains, i = 0; bul; i++, bul = bul->next_content);

	sprintf(buf, "%s contains %d/%d cartridge%s.\r\n", obj->short_description,
		i, MAX_LOAD(obj), i == 1 ? "" : "s");
	send_to_char(CAP(buf), ch);
	break;

    case ITEM_INTERFACE:
    
	if (INTERFACE_TYPE(obj) == INTERFACE_CHIPS) {
	    sprintf(buf, "%s [%d slots] is loaded with:\r\n",
		    obj->short_description, INTERFACE_MAX(obj));
	    for (bul = obj->contains, i = 0; bul; i++, bul = bul->next_content)
		sprintf(buf, "%s%2d. %s\r\n", buf, i, bul->short_description);
	    send_to_char(buf, ch);
	    break;
	}
	break;
   
    case ITEM_COMMUNICATOR:

	if (!ENGINE_STATE(obj)) {
	    act("$p is switched off.", FALSE, ch, obj, 0, TO_CHAR);
	    return;
	}

	sprintf(buf, 
		"%s is active and tuned to channel [%d].\r\n"
		"Energy level is : [%3d/%3d].\r\n"
		"Visible entities monitoring this channel:\r\n",
		obj->short_description, COMM_CHANNEL(obj),
		CUR_ENERGY(obj), MAX_ENERGY(obj));
	for (bul = object_list; bul; bul = bul->next) {
	    if (IS_COMMUNICATOR(bul) && ENGINE_STATE(bul) &&
		COMM_CHANNEL(bul) == COMM_CHANNEL(obj)) {
		if (bul->carried_by && CAN_SEE(ch, bul->carried_by) && 
		    COMM_UNIT_SEND_OK(ch, bul->carried_by))
		    sprintf(buf, "%s  %s%s\r\n", buf, GET_NAME(bul->carried_by),
			    COMM_UNIT_SEND_OK(bul->carried_by, ch) ? "" : "  (mute)");
		else if (bul->worn_by && CAN_SEE(ch, bul->worn_by) &&
			 COMM_UNIT_SEND_OK(ch, bul->worn_by))
		    sprintf(buf, "%s  %s%s\r\n", buf, GET_NAME(bul->worn_by),
			    COMM_UNIT_SEND_OK(bul->worn_by, ch) ? "" : "  (mute)");
	    }
	}
	page_string(ch->desc, buf, 1);
	return;

    default:
	break;
    }
    sprintf(buf, "%s is in %s condition.\r\n",
	    obj->short_description, obj_cond(obj));
    send_to_char(CAP(buf), ch);
  
}


ACMD(do_repair)
{
    struct char_data *vict = NULL;
    struct obj_data *obj = NULL, *tool = NULL;
    int dam, cost, skill = 0;
    int damage_threshold = 0;
    
    skip_spaces(&argument);

    if (!*argument)
	vict = ch;
    else if (!(vict = get_char_room_vis(ch, argument)) &&
	     !(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
	     !(obj =get_obj_in_list_vis(ch,argument,ch->in_room->contents))){
	send_to_char("Repair who or what?\r\n", ch);
	return;
    }

    if (vict) {
	if (!IS_NPC(ch) && (
		(!(tool = GET_EQ(ch, WEAR_HOLD)) &&
	     !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
	    !IS_TOOL(tool) || 
	    TOOL_SKILL(tool) != SKILL_CYBOREPAIR)) {
	    send_to_char("You must be holding a cyber repair tool to do this.\r\n",ch);
	    return;
	}

	if (vict == ch) {
	    if (CHECK_SKILL(ch, SKILL_SELFREPAIR) < 10 || !IS_CYBORG(ch))
		send_to_char("You have no idea how to repair yourself.\r\n", ch);
	    else if (GET_HIT(ch) == GET_MAX_HIT(ch))
		send_to_char("You are not damaged, no repair needed.\r\n", ch);
	    else if ( !IS_NPC(ch) && number(12, 150) > CHECK_SKILL(ch, SKILL_SELFREPAIR) +
		     TOOL_MOD(tool) +
		     (CHECK_SKILL(ch, SKILL_ELECTRONICS) >> 1) + GET_INT(ch))
		send_to_char("You fail to repair yourself.\r\n", ch);
	    else {
		if (IS_NPC(ch)) {
			dam = 2 * GET_LEVEL(ch);
		} else {
			dam = (GET_LEVEL(ch) >> 1) + 
				((CHECK_SKILL(ch, SKILL_SELFREPAIR) + TOOL_MOD(tool)) >> 2) +
				number(0, GET_LEVEL(ch)) +
				dice(GET_REMORT_GEN(ch),10);
		}
		dam = MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dam);
		cost = dam >> 1;
		if ((GET_MANA(ch) + GET_MOVE(ch)) < cost)
		    send_to_char("You lack the energy required to perform this operation.\r\n", ch);
		else {
		    GET_HIT(ch) += dam;
		    GET_MOVE(ch) -= cost;
		    if (GET_MOVE(ch) < 0) {
			send_to_char("WARNING: Auto-accessing reserve energy.\r\n", ch);
			GET_MANA(ch) += GET_MOVE(ch);
			GET_MOVE(ch) = 0;
		    }
		    send_to_char("You skillfully repair yourself.\r\n", ch);
		    act("$n repairs $mself.", TRUE, ch, 0, 0, TO_ROOM);
		    gain_skill_prof(ch, SKILL_SELFREPAIR);
		    WAIT_STATE(ch, PULSE_VIOLENCE * (1 + (dam >> 6)));
		}
	    }
	} else {
	    if (CHECK_SKILL(ch, SKILL_CYBOREPAIR) < 10)
		send_to_char("You have no repair skills for repairing others.\r\n",ch);
	    else if (!IS_CYBORG(vict))
		act("You cannot repair $M.  You can only repair other borgs.",
		    FALSE, ch, 0, vict, TO_CHAR);
	    else if (FIGHTING(ch))
		send_to_char("You cannot perform repairs on fighting patients.\r\n",
			     ch);
	    else if (GET_HIT(vict) == GET_MAX_HIT(vict))
		act("$N's systems are not currently damaged.",FALSE,ch,0,vict,TO_CHAR);
	    else {
		if (number(12, 150) > CHECK_SKILL(ch, SKILL_CYBOREPAIR) +
		    TOOL_MOD(tool) + 
		    (CHECK_SKILL(ch, SKILL_ELECTRONICS) >> 1) + GET_INT(ch)) {
		    act("$n attemps to repair you, but fails.",FALSE,ch,0,vict,TO_VICT);
		    act("You fail to repair $M.\r\n", FALSE, ch, 0, vict, TO_CHAR);
		} else {
	  
		    dam = (GET_LEVEL(ch)>>1) + 
			((CHECK_SKILL(ch,SKILL_CYBOREPAIR) + TOOL_MOD(tool)) >> 2) +
			number(0, GET_LEVEL(ch));
		    dam = MIN(GET_MAX_HIT(vict) - GET_HIT(vict), dam);
		    cost = dam >> 1;
		    if ((GET_MANA(ch) + GET_MOVE(ch)) < cost)
			send_to_char("You lack the energy required to perform this operation.\r\n", ch);
		    else {
			GET_HIT(vict) += dam;
			GET_MOVE(ch) -= cost;
			if (GET_MOVE(ch) < 0) {
			    send_to_char("WARNING: Auto-accessing reserve energy.\r\n", ch);
			    GET_MANA(ch) += GET_MOVE(ch);
			    GET_MOVE(ch) = 0;
			}
			act("You skillfully repair $N.", FALSE,ch,0,vict,TO_CHAR);
			act("$n skillfully makes repairs on $N.", 
			    TRUE, ch, 0, vict, TO_NOTVICT);
			act("$n skillfully makes repairs on you.", 
			    TRUE, ch, 0, vict, TO_VICT);
			gain_skill_prof(ch, SKILL_CYBOREPAIR);
			WAIT_STATE(ch, PULSE_VIOLENCE * (1 + (dam >> 6)));
			update_pos(vict);
		    }
		}
	    }
	}
	return;      /*** end of character repair ***/
    }

    if (!obj)
	return;

    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
	act("$p is too severly damaged for you to repair.",
	    FALSE,ch,obj,0,TO_CHAR);
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
	send_to_char("You can't repair that.\r\n", ch);
	return;
    }
  
    if (CHECK_SKILL(ch, skill) < 20) {
	sprintf(buf, "You are not skilled in the art of %s.\r\n",
		spells[skill]);
	send_to_char(buf, ch); 
	return;
    }

    damage_threshold = GET_OBJ_MAX_DAM( obj ) >> 2;
    damage_threshold -= ( ( GET_OBJ_MAX_DAM( obj) >> 1 ) * CHECK_SKILL( ch, skill ) ) / 150;

    if ( GET_OBJ_DAM(obj) > damage_threshold ) {
	act("$p is not broken.",FALSE,ch,obj,0,TO_CHAR);
	return;
    }
  
    if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
	 !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
	!IS_TOOL(tool) || TOOL_SKILL(tool) != skill) {
	sprintf(buf, "You must be holding a %s tool to do this.\r\n",
		spells[skill]);
	send_to_char(buf, ch);
	return;
    }
  
    dam = ((CHECK_SKILL(ch, skill) + TOOL_MOD(tool)) << 3);
    if (IS_DWARF(ch) && skill == SKILL_METALWORKING)
	dam += (GET_LEVEL(ch) << 2);
  
    GET_OBJ_DAM(obj) = MIN(GET_OBJ_DAM(obj) + dam,
			   (GET_OBJ_MAX_DAM(obj) >> 2) * 3);
    act("You skillfully perform repairs on $p.",
	FALSE,ch,obj,0,TO_CHAR);
    act("$n skillfully performs repairs on $p.",
	FALSE,ch,obj,0,TO_ROOM);
    gain_skill_prof(ch, skill);
    WAIT_STATE(ch, 7 RL_SEC);
    return;
  
}

ACMD(do_overhaul)
{
    struct char_data *vict = NULL;
    int repair, i;
    struct obj_data *tool = NULL;
    skip_spaces(&argument);

    if (!(vict = get_char_room_vis(ch, argument)))
	send_to_char("Overhaul who?\r\n", ch);
    else if (!IS_CYBORG(vict))
	send_to_char("You can only repair other borgs.\r\n", ch);
    else if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
	      !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
	     !IS_TOOL(tool) ||
	     TOOL_SKILL(tool) != SKILL_CYBOREPAIR) {
	send_to_char("You must be holding a cyber repair tool to do this.\r\n",ch);
    }
    else if (GET_TOT_DAM(vict) < (max_component_dam(vict) / 3))
	act("You cannot make any significant improvements to $S systems.",
	    FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) > POS_SITTING)
	send_to_char("Your subject must be sitting, at least.\r\n", ch);
    else if (CHECK_SKILL(ch, SKILL_OVERHAUL) < 10)
	send_to_char("You have no idea how to perform an overhaul.\r\n", ch);
    else if (!vict->master || vict->master != ch)
	send_to_char("Subjects must be following and grouped for you to perform an overhaul.\r\n", ch);
    else {
	repair = GET_TOT_DAM(vict) - (max_component_dam(vict) / 3);
	repair = MIN(repair, GET_LEVEL(ch) * 800);
	repair = MIN(repair, (CHECK_SKILL(ch, SKILL_OVERHAUL) + TOOL_MOD(tool)) * 400);
	repair = MIN(repair, (GET_MOVE(ch) + GET_MANA(ch)) * 100);
    
	act("$n begins to perform a system overhaul on $N.",
	    FALSE, ch, 0, vict, TO_NOTVICT);
	act("$n begins to perform a system overhaul on you.",
	    FALSE, ch, 0, vict, TO_VICT);

	if (!AFF3_FLAGGED(vict, AFF3_STASIS)) {
	    act("Sparks fly from $N's body!!", FALSE, ch, 0, vict, TO_NOTVICT);
	    act("Sparks fly from $N's body!!", FALSE, ch, 0, vict, TO_CHAR);
	    act("Sparks fly from your body!!", FALSE, ch, 0, vict, TO_VICT);
	    if (!IS_NPC(vict)) {
		for (i = 0; i < MAX_SKILLS; i++)
		    if (GET_SKILL(vict, i) && !number(0, GET_LEVEL(vict) >> 3))
			GET_SKILL(vict, i) = MAX(0, GET_SKILL(vict, i) - number(1, 90));
	    }
	    GET_EXP(vict) = MAX(0, GET_EXP(vict) - 
				(GET_LEVEL(vict) * GET_LEVEL(vict) * 100));
	    sprintf(buf, "%sWarning%s:  Some data may be corrupted!!\r\n",
		    CCRED(vict, C_NRM), CCNRM(vict, C_NRM));
	    send_to_char(buf, vict);
	    sprintf(buf,"%sWarning%s:  Subject's data storage may be corrupted!!\r\n",
		    CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    send_to_char(buf, ch);
	}
        
	GET_TOT_DAM(vict) -= repair;
	GET_MOVE(ch) -= (repair / 100);
	if (GET_MOVE(ch) < 0) {
	    GET_MANA(ch) += GET_MOVE(ch);
	    GET_MOVE(ch) = 0;
	}
	WAIT_STATE(vict, PULSE_VIOLENCE * 5);
	WAIT_STATE(ch, PULSE_VIOLENCE * 6);
	gain_skill_prof(ch, SKILL_OVERHAUL);

	send_to_char("Overhaul complete:  System improved.\r\n", ch);
	if (GET_BROKE(vict)) {
	    sprintf(buf, "%sWarning%s:  subject in need of replacement %s.\r\n", 
		    CCRED(ch, C_NRM), CCNRM(ch, C_NRM), 
		    component_names[(int)GET_BROKE(vict)][GET_OLD_CLASS(vict)]); 
	    send_to_char(buf, ch);
	}
    }
}

char *
obj_cond(struct obj_data *obj)
{

    int num;
  
    if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_MAX_DAM(obj) == -1)
	num = 0;
    else if (GET_OBJ_MAX_DAM(obj) == 0) {
	strcpy(buf2, "frail");
	return (buf2);
    } else
	num = ((GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj)) * 100 / 
	       GET_OBJ_MAX_DAM(obj));

    if (num == 0)
	strcpy(buf2, "perfect");
    else if (num < 10)
	strcpy(buf2, "excellent");
    else if (num < 30)
	strcpy(buf2, "good");
    else if (num < 50)
	strcpy(buf2, "fair");
    else if (num < 60)
	strcpy(buf2, "worn");
    else if (num < 70)
	strcpy(buf2, "shabby");
    else if (num < 90)
	strcpy(buf2, "bad");
    else 
	strcpy(buf2, "terrible");
	 
    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN))
	strcat(buf2, " <broken>");

    return(buf2);
}

// n should run between 1 and 7
#define ALEV(n)    (CHECK_SKILL(ch, SKILL_ANALYZE) > number(50, 50+(n*10)))

ACMD(do_analyze)
{

    struct obj_data *obj   = NULL;
    struct char_data *vict = NULL;
    char buf3[MAX_STRING_LENGTH];
    int i, found = 0;

    skip_spaces(&argument);
  
    if (!*argument ||
	(!(vict = get_char_room_vis(ch, argument)) &&
	 !(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
	 !(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents)))) {
	send_to_char("Analyze what?\r\n", ch);
	return;
    }

    if (CHECK_SKILL(ch, SKILL_ANALYZE) < 10) {
	send_to_char("You are not trained in the methods of analysis.\r\n", ch);
	return;
    }
    if (CHECK_SKILL(ch, SKILL_ANALYZE) < number(0, 100)) {
	send_to_char("You fail miserably.\r\n", ch);
	WAIT_STATE(ch, 2 RL_SEC);
	return;
    }
    if (GET_MOVE(ch) < 10) {
	send_to_char("You lack the energy necessary to perform the analysis.\r\n",
		     ch);
	return;
    }
  
    if (obj) {
    
	sprintf(buf, "       %s***************************************\r\n", CCGRN(ch, C_NRM));
	sprintf(buf, "%s       %s>>>     OBJECT ANALYSIS RESULTS:    <<<\r\n", 
		buf, CCCYN(ch,C_NRM));
	sprintf(buf, "%s       %s***************************************%s\r\n", 
		buf, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf, "%sDescription:          %s%s%s\r\n", buf, CCCYN(ch, C_NRM),
		obj->short_description, CCNRM(ch, C_NRM));
	sprintf(buf, "%sItem Classification:  %s%s%s\r\n", buf, CCCYN(ch, C_NRM),
		item_types[(int)GET_OBJ_TYPE(obj)], CCNRM(ch, C_NRM));
	sprintf(buf, "%sMaterial Composition: %s%s%s\r\n", buf, CCCYN(ch, C_NRM),
		material_names[GET_OBJ_MATERIAL(obj)], CCNRM(ch, C_NRM));
	// give detailed item damage info
	if (ALEV(5))
	    sprintf(buf3, "  [%4d/%4d]", GET_OBJ_DAM(obj), GET_OBJ_MAX_DAM(obj));
	else
	    strcpy(buf3, "");
	sprintf(buf, "%sCondition:            %s%-15s%s%s\r\n", buf, 
		CCCYN(ch, C_NRM), obj_cond(obj), CCNRM(ch, C_NRM), buf3);
	sprintf(buf, "%sCommerce Value:       %s%d coins%s\r\n", buf, 
		CCCYN(ch, C_NRM), GET_OBJ_COST(obj), CCNRM(ch, C_NRM));
	sprintf(buf, "%sWeight:               %s%d pounds%s\r\n", buf, 
		CCCYN(ch, C_NRM), obj->getWeight(), CCNRM(ch, C_NRM));

	sprintf(buf, "%sIntrinsic Properties: %s", buf, CCCYN(ch, C_NRM));
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf2);
	strcat(buf, buf2);
	sprintbit(GET_OBJ_EXTRA2(obj), extra2_bits, buf2);
	strcat(buf, buf2);
	sprintf(buf, "%s%s\r\n", buf, CCNRM(ch, C_NRM));

	// check for affections
	found = 0;
	strcpy(buf2, "Apply:               ");
	strcat(buf2, CCCYN(ch, C_NRM));
	for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	    if (obj->affected[i].modifier) {
		sprinttype(obj->affected[i].location, apply_types, buf3);
		sprintf(buf2, "%s%s %+d to %s", buf2, found++ ? "," : "",
			obj->affected[i].modifier, buf3);
	    }
	}
	if (found)
	    strcat(buf, strcat(strcat(buf2, CCNRM(ch, C_NRM)), "\r\n"));
      
	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_ARMOR:
	    if (ALEV(4))
		sprintf(buf3, "  [%d]", GET_OBJ_VAL(obj, 0));
	    else
		strcpy(buf3, "");
	    sprintf(buf, "%sProtective Quality:   %s%-15s%s%s\r\n", buf,
		    CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 0) < 2 ? "poor" : 
		    GET_OBJ_VAL(obj, 0) < 4 ? "minimal" :
		    GET_OBJ_VAL(obj, 0) < 6 ? "moderate" :
		    GET_OBJ_VAL(obj, 0) < 8 ? "good" :
		    GET_OBJ_VAL(obj, 0) < 10 ? "excellent" :
		    "extraordinary", CCNRM(ch, C_NRM), buf3);
	    break;
	case ITEM_LIGHT:
	    sprintf(buf, "%sDuration remaining:   %s%d hours%s\r\n", buf,
		    CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 2), CCNRM(ch, C_NRM));
	    break;
	case ITEM_WAND:
	case ITEM_SCROLL:
	case ITEM_STAFF:
	    sprintf(buf, "%s%sERROR:%s no further information available.\r\n",
		    buf, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    break;
	case ITEM_WEAPON:
	    sprintf(buf, "%sDamage Dice:          %s%dd%d%s\r\n", buf,
		    CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
		    CCNRM(ch, C_NRM));
	    break;
	case ITEM_CONTAINER:
	    if (GET_OBJ_VAL(obj, 3))
		strcat(buf, "Item is a corpse.\r\n");
	    else
		sprintf(buf, "%sCapacity:            %s%d pounds%s\r\n", buf,
			CCCYN(ch, C_NRM), GET_OBJ_VAL(obj, 0), CCNRM(ch, C_NRM));
	    break;
	case ITEM_VEHICLE:
	    if (obj->contains && GET_OBJ_TYPE(obj->contains) == ITEM_ENGINE)
		sprintf(buf, "%sVehicle is equipped with:     %s%s%s\r\n", buf,
			CCCYN(ch, C_NRM), obj->contains->short_description, 
			CCNRM(ch, C_NRM));
	    else
		strcat(buf, "Vehicle is not equipped with an engine.\r\n");
	    break;
	case ITEM_ENGINE:
	    sprintf(buf, "%sMax Fuel: %s%d%s, Current Fuel: %s%d%s, Type: %s%s%s, "
		    "Eff: %s%d%s\r\n", buf, 
		    CCCYN(ch, C_NRM), MAX_ENERGY(obj), CCNRM(ch, C_NRM), 
		    CCCYN(ch, C_NRM), CUR_ENERGY(obj), CCNRM(ch, C_NRM), 
		    CCCYN(ch, C_NRM),   
		    IS_SET(ENGINE_STATE(obj), ENG_PETROL) ? "Petrol" :
		    IS_SET(ENGINE_STATE(obj), ENG_ELECTRIC) ? "Elect" :
		    IS_SET(ENGINE_STATE(obj), ENG_MAGIC) ? "Magic" : "Fucked",
		    CCNRM(ch, C_NRM), 
		    CCCYN(ch, C_NRM), USE_RATE(obj), CCNRM(ch, C_NRM));
	    break;
	case ITEM_BATTERY:
	    sprintf(buf, "%sMax Energy: %s%d%s, Current Energy: %s%d%s, Recharge Rate: %s%d%s\r\n", buf, CCCYN(ch, C_NRM), MAX_ENERGY(obj), CCNRM(ch, C_NRM),
		    CCCYN(ch, C_NRM), CUR_ENERGY(obj), CCNRM(ch, C_NRM),
		    CCCYN(ch, C_NRM), RECH_RATE(obj), CCNRM(ch, C_NRM));
	    break;

	case ITEM_BOMB:
	    if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) > 50 && obj->contains)
		sprintf(buf, "%sFuse: %s.  Fuse State: %sactive.\r\n", buf,
			obj->contains->short_description,
			FUSE_STATE(obj->contains) ? "" : "in");
	    break;
      
	}


	GET_MOVE(ch) -= 10;
	page_string(ch->desc, buf, 1);
	return;
    }

    if (vict) {

	sprintf(buf,   "       %s***************************************\r\n", CCGRN(ch, C_NRM));
	sprintf(buf, "%s       %s>>>     ENTITY ANALYSIS RESULTS:    <<<\r\n", 
		buf, CCCYN(ch,C_NRM));
	sprintf(buf, "%s       %s***************************************%s\r\n", 
		buf, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf, "%sName:                  %s%s%s\r\n", buf, CCCYN(ch, C_NRM),
		GET_NAME(vict), CCNRM(ch, C_NRM));
	sprintf(buf, "%sRacial Classification: %s%s%s\r\n", buf, CCCYN(ch, C_NRM),
		player_race[(int)GET_RACE(vict)], CCNRM(ch, C_NRM));
	if (GET_CLASS(vict) < NUM_CLASSES) {
	    sprintf(buf, "%sPrimary Occupation:    %s%s%s\r\n", buf,CCCYN(ch, C_NRM),
		    pc_char_class_types[(int)GET_CLASS(vict)], CCNRM(ch, C_NRM));
	} 
	else {
	    sprintf(buf, "%sPrimary Type:          %s%s%s\r\n", buf,CCCYN(ch, C_NRM),
		    pc_char_class_types[(int)GET_CLASS(vict)], CCNRM(ch, C_NRM));
	}
	if (GET_REMORT_CLASS(vict) >= 0)
	    sprintf(buf, "%sSecondary Occupation:  %s%s%s\r\n", buf,
		    CCCYN(ch, C_NRM),
		    pc_char_class_types[(int)GET_REMORT_CLASS(vict)], CCNRM(ch, C_NRM));
    
	GET_MOVE(ch) -= 10;
	page_string(ch->desc, buf, 1);
	return;
    }
  
    send_to_char("Error.\r\n", ch);
    
}
    
ACMD(do_insert)
{

    struct obj_data *obj = NULL, *tool = NULL;
    struct char_data *vict = NULL;
    int pos, i;

    skip_spaces(&argument);
  
    if (!*argument || !*(argument = two_arguments(argument, buf, buf2))) {
	send_to_char("Insert <object> <victim> <position>\r\n", ch);
	return;
    }

    if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
	send_to_char("You do not carry that implant.\r\n", ch);
	return;
    }
    if (!(vict = get_char_room_vis(ch, buf2))) {
	act("Insert $p in who?", FALSE, ch, obj, 0, TO_CHAR);
	return;
    }
  
    if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) < 30) {
	send_to_char("You are unskilled in the art of cybosurgery.\r\n", ch);
	return;
    }

    if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && GET_LEVEL(ch) < LVL_IMMORT) {
	send_to_char("You can only perform surgery in a safe room.\r\n", ch);
	return;
    }

    if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
		 !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
		!IS_TOOL(tool) ||
		TOOL_SKILL(tool) != SKILL_CYBO_SURGERY) {
	send_to_char("You must be holding a cyber surgery tool to do this.\r\n", ch);
	return;
    }
	if (!IS_CYBORG(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char("Your subject is not prepared for such enhancement.\r\n",ch);
		return;
	}

    one_argument(argument, buf);

    if ((pos = search_block(buf, wear_implantpos, 0)) < 0 ||
	(ILLEGAL_IMPLANTPOS(pos) && !IS_OBJ_TYPE(obj, ITEM_TOOL))) {
		send_to_char("Invalid implant position.\r\n", ch);
		return;
    }

    if (!IS_IMPLANT(obj)) {
		act("ERROR: $p is not an implantable object.",
			FALSE, ch, obj, vict, TO_CHAR);
		return;
    }
    if (ANTI_ALIGN_OBJ(vict, obj)) {
		act("$N is unable to safely utilize $p.",FALSE,ch,obj,vict,TO_CHAR);
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
	if (!IS_WEAR_EXTREMITY(pos)) {    
		if (GET_LEVEL(ch) < LVL_IMMORT && vict == ch) {
			send_to_char("You can only perform surgery on your extremities!\r\n", ch);
			return;
		}
		if (GET_LEVEL(ch) < LVL_IMMORT && AWAKE(vict) && ch != vict) {
			send_to_char("Your subject is not properly sedated.\r\n", ch);
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
		GET_POS(vict) = POS_RESTING;
		//    damage(ch, vict, dice(5, 10), TYPE_SURGERY, pos);
		return;
    }

    if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) + 
	(GET_DEX(ch) << 2) <
	number(50, 100)) {
		send_to_char("You fail.\r\n", ch);
		return;
    }

    obj_from_char(obj);
    if (equip_char(vict, obj, pos, MODE_IMPLANT))
		return;

	if(ch == vict) {
		sprintf(buf,"$p inserted into your %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_CHAR);
		sprintf(buf,"$n inserts $p into $s %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_NOTVICT);
		if(GET_POS(vict) > POS_SITTING)
			GET_POS(ch) = POS_SITTING;
		GET_HIT(vict) = GET_HIT(vict) / 4;
		GET_MOVE(vict) = GET_MOVE(vict) / 4;
		if(GET_HIT(vict) < 1)
			GET_HIT(vict) = 1;
		if(GET_MOVE(vict) < 1)
			GET_MOVE(vict) = 1;
		WAIT_STATE(vict, 6 RL_SEC);
	} else {
		sprintf(buf,"$p inserted into $N's %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_CHAR);

		sprintf(buf,"$n inserts $p into your %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_VICT);

		sprintf(buf,"$n inserts $p into $n's %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, GET_IMPLANT(vict, pos), vict, TO_NOTVICT);
		if(GET_POS(vict) > POS_RESTING)
			GET_POS(vict) = POS_RESTING;
		GET_HIT(vict) = 1;
		GET_MOVE(vict) = 1;
		WAIT_STATE(vict, 10 RL_SEC);
	}

}


ACMD(do_extract)
{

    struct obj_data *obj = NULL, *corpse = NULL, *tool = NULL;
    struct char_data *vict = NULL;
    int pos;

    skip_spaces(&argument);
  
    if (!*argument) {
		send_to_char("Extract <object> <victim> <position>        ...or...\r\n"
				 "Extract <object> <corpse>\r\n", ch);
		return;
    }
  
    argument = two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2) {
		send_to_char("Extract <object> <victim> <position>        ...or...\r\n"
				 "Extract <object> <corpse>\r\n", ch);
		return;
    }

    if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) < 30) {
		send_to_char("You are unskilled in the art of cybosurgery.\r\n", ch);
		return;
    }

    if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
	 !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
	!IS_TOOL(tool) ||
	TOOL_SKILL(tool) != SKILL_CYBO_SURGERY) {
		send_to_char("You must be holding a cyber surgery tool to do this.\r\n", ch);
		return;
    }

    if (!(vict = get_char_room_vis(ch, buf2))) {
	if (!(corpse = get_obj_in_list_vis(ch, buf2, ch->in_room->contents))) {
	    send_to_char("Extract from who?\r\n", ch);
	    return;
	}

	if (!(obj = get_obj_in_list_vis(ch, buf, corpse->contains))) {
	    act("There is no such thing in $p.", FALSE, ch, corpse, 0, TO_CHAR);
	    return;
	}

	if (IS_CORPSE(corpse)) {
	    if (!IS_IMPLANT(obj) || CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
		act("$p is not implanted.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	    }
	} else if (!IS_BODY_PART(corpse) || !isname("head", corpse->name) ||
		   !OBJ_TYPE(corpse, ITEM_DRINKCON)) {
	    send_to_char("You cannot extract from that.\r\n", ch);
	    return;
	}
    
	if (((CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) + 
	      (GET_DEX(ch) << 2)) < number(50, 100)) ||
	    (IS_OBJ_TYPE(obj, ITEM_SCRIPT) && GET_LEVEL(ch) < 50)) {
	    send_to_char("You fail.\r\n", ch);
	    return;
	}
      
	obj_from_obj(obj);
	obj_to_char(obj, ch);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	act("You carefully extract $p from $P.",FALSE, ch, obj, corpse, TO_CHAR);
	if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) + 
	    (GET_DEX(ch) << 2) <
	    number(50, 100)) {
	    act("You damage $p during the extraction!",FALSE,ch,obj,0,TO_CHAR);
	    damage_eq(ch, obj, MAX((GET_OBJ_DAM(obj) >> 2),
				   (dice(10, 10) - 
				    CHECK_SKILL(ch, SKILL_CYBO_SURGERY) -
				    (GET_DEX(ch) << 2))));
	} else
	    gain_skill_prof(ch, SKILL_CYBO_SURGERY);

	return;
    }
    
	if (GET_LEVEL(ch) < LVL_IMMORT && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
		send_to_char("You can only perform surgery in a safe room.\r\n", ch);
		return;
    }

    if (!(obj = get_object_in_equip_vis(ch, buf, vict->implants, &pos))) {
		send_to_char("Invalid object.  Type 'extract' for usage.\r\n",ch);
		return;
    }

    if (!*argument) {
		send_to_char("Extract the implant from what position.\r\n", ch);
		return;
    }

    one_argument(argument, buf);
  
    if ((pos = search_block(buf, wear_implantpos, 0)) < 0) {
		send_to_char("Invalid implant position.\r\n", ch);
		return;
    }
	if (!IS_WEAR_EXTREMITY(pos)) {    
		if (GET_LEVEL(ch) < LVL_IMMORT && vict == ch) {
			send_to_char("You can only perform surgery on your extrimities!\r\n", ch);
			return;
		}

		if (GET_LEVEL(ch) < LVL_IMMORT && AWAKE(vict) && ch != vict) {
			send_to_char("Your subject is not properly sedated.\r\n", ch);
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
		GET_POS(vict) = POS_RESTING;
		//    damage(ch, vict, dice(5, 10), TYPE_SURGERY, pos);
		return;
    }

    if (!GET_IMPLANT(vict, pos)) {
		act("ERROR: $E is not implanted there.", FALSE, ch, 0, vict, TO_CHAR);
		return; // unequip_char(vict, pos, MODE_IMPLANT);
    }

    if ((CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) + 
	 (GET_DEX(ch) << 2) <  number(50, 100)) ||
	(IS_OBJ_TYPE(obj, ITEM_SCRIPT) && GET_LEVEL(ch) < 50)) {
		send_to_char("You fail.\r\n", ch);
		return;
    }

    obj_to_char((obj = unequip_char(vict, pos, MODE_IMPLANT)), ch);
    SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	if(ch == vict) {
		sprintf(buf,"$p extracted from your %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, obj, vict, TO_CHAR);
		sprintf(buf,"$n extracts $p from $s %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, obj, vict, TO_NOTVICT);
		if(GET_POS(vict) > POS_SITTING)
			GET_POS(vict) = POS_SITTING;
		GET_HIT(vict) = GET_HIT(vict) / 4;
		GET_MOVE(vict) = GET_MOVE(vict) / 4;
		if(GET_HIT(vict) < 1)
			GET_HIT(vict) = 1;
		if(GET_MOVE(vict) < 1)
			GET_MOVE(vict) = 1;
		WAIT_STATE(vict, 6 RL_SEC);
	} else {
		sprintf(buf,"$p extracted from $N's %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, obj, vict, TO_CHAR);

		sprintf(buf,"$n extracts $p from your %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, obj, vict, TO_VICT);

		sprintf(buf,"$n extracts $p from $n's %s.",wear_implantpos[pos]);
		act(buf, FALSE, ch, obj, vict, TO_NOTVICT);
		if(GET_POS(vict) > POS_RESTING)
			GET_POS(vict) = POS_RESTING;
		GET_HIT(vict) = 1;
		GET_MOVE(vict) = 1;
		WAIT_STATE(vict, 10 RL_SEC);
	}

    if (CHECK_SKILL(ch, SKILL_CYBO_SURGERY) + TOOL_MOD(tool) +
	(GET_DEX(ch) << 2) <
	number(50, 100)) {
	act("You damage $p during the extraction!",FALSE,ch,obj,0,TO_CHAR);
	damage_eq(ch, obj, MAX((GET_OBJ_DAM(obj) >> 2),
			       (dice(10, 10) - 
				CHECK_SKILL(ch, SKILL_CYBO_SURGERY) -
				(GET_DEX(ch) << 2))));
    } else
	gain_skill_prof(ch, SKILL_CYBO_SURGERY);
}

ACMD(do_cyberscan)
{
    struct obj_data *obj = NULL, *impl = NULL;
    struct char_data *vict = NULL;
    int i;
    int found = 0;

    skip_spaces(&argument);

    if (!(vict = get_char_room_vis(ch, argument))) {
	if ((obj = get_obj_in_list_vis(ch, argument, ch->carrying)) ||
	    (obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
	    for (impl = obj->contains, found = 0; impl; impl = impl->next_content)
		if (IS_IMPLANT(impl) && CAN_SEE_OBJ(ch, impl) &&
			!IS_OBJ_TYPE(obj, ITEM_SCRIPT) &&
		    ((CHECK_SKILL(ch, SKILL_CYBERSCAN) +
		      (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 50 : 0) > 
		      number(70, 150)) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		    act("$p detected.", FALSE, ch, impl, 0, TO_CHAR);
		    ++found;
		}
	    if (!found)
		act("No implants detected in $p.",FALSE,ch,obj,0,TO_CHAR);
	    act("$n scans $p.", TRUE, ch, obj, 0, TO_ROOM);
	} else
	    send_to_char("Cyberscan who?\r\n", ch);
	return;
    }

    if (ch != vict) {
	act("$n scans $N.", TRUE, ch, 0, vict, TO_NOTVICT);
	act("$n scans you.", TRUE, ch, 0, vict, TO_VICT);
    }

    send_to_char("CYBERSCAN RESULTS:\r\n", ch);
    for (i = 0; i < NUM_WEARS; i++) {
	if ((obj = GET_IMPLANT(vict, i)) && CAN_SEE_OBJ(ch, obj) &&
	    ((CHECK_SKILL(ch, SKILL_CYBERSCAN) +
	      (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 50 : 0) > 
	      number(50, 120)) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
      
	    sprintf(buf, "[%12s] - %s -\r\n", wear_implantpos[i],
		    obj->short_description);
	    send_to_char(buf, ch);
	    ++found;
	}
    }

    if (!found)
	send_to_char("Negative.\r\n", ch);
    else if (ch != vict)
	gain_skill_prof(ch, SKILL_CYBERSCAN);
}

int 
CLIP_COUNT(struct obj_data *clip)
{
    struct obj_data *bul = NULL;
    int count = 0;

    for (bul = clip->contains, count = 0; bul; count++, bul = bul->next_content);
  
    return count;
}

ACMD(do_load)
{
    struct obj_data *obj1 = NULL, *obj2 = NULL, *bullet = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i = 0;
    int count = 0;
    bool internal = false;
  
    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
	sprintf(buf, "%s what?\r\n", CMD_NAME);
	send_to_char(CAP(buf), ch);
	return;
    }

    if (subcmd == SCMD_LOAD) {

	if (!(obj1 = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
	    sprintf(buf, "You can't find any '%s' to load.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}
	if (!*arg2) {
	    act("Load $p into what?", FALSE, ch, obj1, 0, TO_CHAR);
	    return;
	}
	if (!strncmp(arg2, "internal", 8)) {
      
	    internal = true;
	    argument = one_argument(argument, arg1);
      
	    if (!*arg1) {
		send_to_char("Load which implant?\r\n", ch);
		return;
	    }
      
	    if (!(obj2 = get_object_in_equip_vis(ch, arg1, ch->implants, &i))) {
		sprintf(buf, "You are not implanted with %s '%s'.\r\n",AN(arg1),arg1);
		send_to_char(buf, ch);
		return;
	    }
      
	} else if (!(obj2=get_object_in_equip_vis(ch, arg2, ch->equipment, &i)) &&
		   !(obj2=get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
		   !(obj2=get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
	    sprintf(buf, "You don't have any '%s' to load $p into.", arg2);
	    act(buf, FALSE, ch, obj1, 0, TO_CHAR);
	    return;
	}
	if (obj2 == obj1) {
	    send_to_char("Real funny.\r\n", ch);
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
		act("You can't load $p into $P.", FALSE, ch, obj1, obj2, TO_CHAR);
		return;
	    }
	    if (obj2->contains) {
		act("$P is already loaded with $p.", 
		    FALSE, ch, obj2->contains, obj2, TO_CHAR);
		return;
	    }
	} else if (IS_CLIP(obj2)) {

	    if (!IS_BULLET(obj1)) {
		act("You can't load $p into $P.", FALSE, ch, obj1, obj2, TO_CHAR);
		return;
	    }
	    if (GUN_TYPE(obj1) != GUN_TYPE(obj2)) {
		act("$p does not fit in $P", FALSE, ch, obj1, obj2, TO_CHAR);
		return;
	    }
	    for (bullet = obj2->contains, count = 0; bullet; 
		 count++, bullet = bullet->next_content);
	    if (count >= MAX_LOAD(obj2)) {
		act("$P is already loaded to capacity.",FALSE,ch,obj1,obj2,TO_CHAR);
		return;
	    }
	} else if (IS_GUN(obj2)) {

	    if (!MAX_LOAD(obj2)) {  /** clip gun **/
		if (!IS_CLIP(obj1)) {
		    act("You have to use a clip with $P.",
			FALSE,ch,obj1,obj2,TO_CHAR);
		    return;
		} 
		if (obj2->contains) {
		    act("$P is already loaded with $p.",
			FALSE, ch, obj2->contains, obj2, TO_CHAR);
		    return;
		}
	    } 
	    else {                 /** revolver type **/
		if (!IS_BULLET(obj1)) {
		    act("You can only load $P with bullets",
			FALSE,ch,obj1,obj2,TO_CHAR);
		    return;
		}
		for (bullet = obj2->contains, count = 0; bullet;
		     count++, bullet = bullet->next_content);
		if (count >= MAX_LOAD(obj2)) {
		    act("$P is already fully loaded.",FALSE,ch,obj1,obj2,TO_CHAR);
		    return;
		}
	    }

	    if (GUN_TYPE(obj1) != GUN_TYPE(obj2)) {
		act("$p does not fit in $P", FALSE, ch, obj1, obj2, TO_CHAR);
		return;
	    }
	}
	else if (IS_CHIP(obj1) && IS_INTERFACE(obj2) &&
		 INTERFACE_TYPE(obj2) == INTERFACE_CHIPS) {
	    if (INTERFACE_CUR(obj2) >= INTERFACE_MAX(obj2)) {
		act("$P is already loaded with microchips.",
		    FALSE, ch, obj1, obj2, TO_CHAR);
		return;
	    }
	    if (redundant_skillchip(obj1, obj2)) {
		send_to_char("Attachment of that chip is redundant.\r\n", ch);
		return;
	    }
	}
	else {
	    act("You can't load $p into $P.", FALSE, ch, obj1, obj2, TO_CHAR);
	    return;
	}

	if (internal) {
	    act("You load $p into $P (internal).", TRUE, ch, obj1, obj2, TO_CHAR);
	    act("$n loads $p into $P (internal).", TRUE, ch, obj1, obj2, TO_ROOM);
	} else {
	    act("You load $p into $P.", TRUE, ch, obj1, obj2, TO_CHAR);
	    act("$n loads $p into $P.", TRUE, ch, obj1, obj2, TO_ROOM);
	}
	obj_from_char(obj1);
	obj_to_obj(obj1, obj2);
	if (IS_CHIP(obj1) && ch == obj2->worn_by &&
	    obj1->action_description)
	    act(obj1->action_description, FALSE, ch, 0, 0, TO_CHAR);
	i = 10 - MIN(8, 
		     ((CHECK_SKILL(ch, SKILL_SPEED_LOADING) + GET_DEX(ch)) >> 4));
	WAIT_STATE(ch, i);
	return;
    }

    if (subcmd == SCMD_UNLOAD) {

	if (!strncmp(arg1, "internal", 8)) {

	    internal = true;
	    if (!*arg2) {
		send_to_char("Unload which implant?\r\n", ch);
		return;
	    }
      
	    if (!(obj2 = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
		sprintf(buf, "You are not implanted with %s '%s'.\r\n",AN(arg2),arg2);
		send_to_char(buf, ch);
		return;
	    }
      
	} else if (!(obj2=get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
		   !(obj2=get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		   !(obj2=get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
	    sprintf(buf, "You can't find %s '%s' to unload.", AN(arg1), arg1);
	    act(buf, FALSE, ch, obj1, 0, TO_CHAR);
	    return;
	}

	if (IS_ENERGY_GUN(obj2)) {
	    if (!(obj1 = obj2->contains)) {
		act("$p is not loaded with an energy cell.",FALSE,ch,obj2,0,TO_CHAR);
		return;
	    }
	} else if (IS_GUN(obj2) || IS_CLIP(obj2) || IS_INTERFACE(obj2)) {
	    if (!(obj1 = obj2->contains)) {
		act("$p is not loaded.",FALSE,ch,obj2,0,TO_CHAR);
		return;
	    }
	} else {
	    send_to_char("You cannot unload anything from that.\r\n", ch);
	    return;
	}

	if (internal) {
	    act("You unload $p from $P (internal).", TRUE, ch, obj1, obj2, TO_CHAR);
	    act("$n unloads $p from $P (internal).", TRUE, ch, obj1, obj2, TO_ROOM);
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
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1)
	send_to_char("Refill what from what?\r\n", ch);
    else if (!*arg2)
	send_to_char("Usage: refill <to obj> <from obj>\r\n", ch);
    else if (!(syr = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
	     !(syr = get_object_in_equip_vis(ch, arg1, ch->equipment, &i))) {
	sprintf(buf, "You don't seem to have %s '%s' to refill.\r\n", 
		AN(arg1), arg1);
	send_to_char(buf, ch);
    } else if (!IS_SYRINGE(syr))
	send_to_char("You can only refill syringes.\r\n", ch);
    else if (GET_OBJ_VAL(syr, 0))
	act("$p is already full.", FALSE, ch, syr, 0, TO_CHAR);
    else if (!(vial = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
	sprintf(buf, "You can't find %s '%s' to refill from.\r\n", AN(arg2), arg2);
	send_to_char(buf, ch);
    } else if (!GET_OBJ_VAL(vial, 0))
	act("$p is empty.", FALSE, ch, vial, 0, TO_CHAR);
    else {

	act("You refill $p from $P.", FALSE, ch, syr, vial, TO_CHAR);

	GET_OBJ_VAL(syr, 0)  =  GET_OBJ_VAL(vial, 0);
	GET_OBJ_VAL(syr, 1)  =  GET_OBJ_VAL(vial, 1);
	GET_OBJ_VAL(syr, 2)  =  GET_OBJ_VAL(vial, 2);
	GET_OBJ_VAL(syr, 3)  =  GET_OBJ_VAL(vial, 3);

	if (IS_POTION(vial) || IS_OBJ_STAT(vial, ITEM_MAGIC))
	    SET_BIT(GET_OBJ_EXTRA(syr), ITEM_MAGIC);

	sprintf(buf, "%s %s", fname(vial->name), syr->name);
	if (!syr->shared->proto || syr->shared->proto->name != syr->name) {
#ifdef DMALLOC 
	    dmalloc_verify(0); 
#endif
	    free(syr->name);
#ifdef DMALLOC 
	    dmalloc_verify(0); 
#endif    
	}
#ifdef DMALLOC 
	dmalloc_verify(0); 
#endif    
	syr->name = str_dup(buf);
#ifdef DMALLOC 
	dmalloc_verify(0); 
#endif
	if (IS_POTION(vial)) {
	    act("$P dissolves before your eyes.", FALSE, ch, syr, vial, TO_CHAR);
	    extract_obj(vial);
	} else {
	    vial->modifyWeight( -1 );

	    if ( vial->getWeight() <= 0) {
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
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    skip_spaces(&argument);
    argument = one_argument(argument, arg1);

    if (!*arg1) {
	send_to_char("Usage:  transmit [internal] <device> <message>\r\n", ch);
	return;
    }

    if (!strncmp(arg1, "internal", 8)) {
    
	argument = one_argument(argument, arg2);

	if (!*arg2) {
	    send_to_char("Transmit with which implant?\r\n", ch);
	    return;
	}
    
	if (!(obj = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
	    sprintf(buf, "You are not implanted with %s '%s'.\r\n",AN(arg2),arg2);
	    send_to_char(buf, ch);
	    return;
	}
    
    } else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))     &&
	       !(obj=get_object_in_equip_all(ch,arg1,ch->equipment,&i))  &&
	       !(obj=get_obj_in_list_vis(ch, arg1,ch->in_room->contents))) {
	sprintf(buf, "You can't seem to find %s '%s'.\r\n",AN(arg1),arg1);
	send_to_char(buf, ch);
	return;
    } 

    if (!IS_COMMUNICATOR(obj)) {
	act("$p is not a communication device.", FALSE, ch, obj, 0, TO_CHAR);
	return;
    }
    if (!ENGINE_STATE(obj)) {
	act("$p is not switched on.",FALSE,ch,obj,0,TO_CHAR);
	return;
    }

    if (!*argument) {
	send_to_char("Usage:  transmit [internal] <device> <message>\r\n", ch);
	return;
    }

    if (!obj->worn_by || (obj != GET_IMPLANT(ch, obj->worn_on))) {
	sprintf(buf, "$n speaks into $p%s.",
		obj->worn_by ? " (worn)" : "");
	act(buf, FALSE, ch, obj, 0, TO_ROOM);
    }

    sprintf(buf,"$n >%s",argument);
    send_to_comm_channel(ch, buf, COMM_CHANNEL(obj), 
			 PRF_FLAGGED(ch, PRF_NOREPEAT) ? TRUE :FALSE, FALSE);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char("Okay, message transmitted.\r\n", ch);
  
}


ACMD(do_overdrain)
{

    struct obj_data *source= NULL;

    int i;
	int amount = 0;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	//Make sure they know how to overdrain
    if (CHECK_SKILL(ch, SKILL_OVERDRAIN) < 50) {
		send_to_char("You don't know how.\r\n", ch);
		return;
    }

    skip_spaces(&argument);
    argument = one_argument(argument, arg1);

    if (!*arg1) {
		send_to_char("Usage:  overdrain [internal] <battery/device>\r\n", ch);
		return;
    }

	// Find the object to drain from
    if (!strncmp(arg1, "internal", 8)) {
   
    argument = one_argument(argument, arg2);

    if (!*arg2) {
        send_to_char("Drain energy from which implant?\r\n", ch);
        return;
    }
   
    if (!(source = get_object_in_equip_vis(ch, arg2, ch->implants, &i))) {
        sprintf(buf, "You are not implanted with %s '%s'.\r\n",AN(arg2),arg2);
        send_to_char(buf, ch);
        return;
    }
   
    } else if (!(source = get_obj_in_list_vis(ch, arg1, ch->carrying))     &&
           !(source=get_object_in_equip_all(ch,arg1,ch->equipment,&i))  &&
           !(source=get_obj_in_list_vis(ch, arg1,ch->in_room->contents))) {
    sprintf(buf, "You can't seem to find %s '%s'.\r\n",AN(arg1),arg1);
    send_to_char(buf, ch);
    return;
    }
	if (IS_IMPLANT(source) 
		&& source != get_object_in_equip_vis(ch, arg2, ch->implants, &i)) {
		act ("ERROR: $p not installed properly.", FALSE,ch,source,0,TO_CHAR);
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
		act("$p is equipped with a power regulator.",FALSE,ch,source,0,TO_CHAR);
		return;
    }
    gain_skill_prof(ch, SKILL_OVERDRAIN);
	amount = number(0 , GET_LEVEL(ch)) 
		+ GET_LEVEL(ch) 
		+ (GET_REMORT_GEN(ch) << 2);
    perform_recharge(ch, source, ch, 0, amount);
    return;
}


ACMD(do_de_energize)
{
    CHAR *vict;
    int move = 0;

    skip_spaces(&argument);
  
    if (!*argument) {
	send_to_char("De-energize who?\r\n", ch);
	return;
    }

    if (!(vict = get_char_room_vis(ch, argument))) {
	send_to_char("No-one by that name here.\r\n", ch);
	return;
    }

    if (vict == ch) {
	send_to_char("Let's not try that shall we...\r\n", ch);
	return;
    }

    if (!peaceful_room_ok(ch, vict, true))
	return;

    appear(ch, vict);
    check_toughguy(ch, vict, 1);
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
			GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3),
			SAVING_ROD)) {
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

    if (!move) // vict all drained up
	act("$N is already completely de-energized!", FALSE, ch, 0, vict, TO_CHAR);
    else {
	if (!GET_MOVE(vict))
	    act("$N is now completely de-energized!", FALSE, ch, 0, vict, TO_CHAR);
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
	send_to_char("Only the Borg may assimilate.\r\n", ch);
	return;
    }

    if (CHECK_SKILL(ch, SKILL_ASSIMILATE) < 60) {
	send_to_char("You do not have sufficient programming to assimilate.\r\n", ch);
	return;
    }

    skip_spaces(&argument);

    if (!*argument) {
	send_to_char("Assimilate what?\r\n", ch);
	return;
    }

    if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying))) {
	sprintf(buf, "You can't find any '%s'.\r\n", argument);
	send_to_char(buf, ch);
	return;
    }

    manacost = MIN(10, obj->getWeight() );

    if (GET_MANA(ch) < manacost) {
	send_to_char("You lack the mana required to assimilate this.\r\n", ch);
	return;
    }

    GET_MANA(ch) -= manacost;

    // see if it can be assimilated
    if ((IS_OBJ_STAT(obj, ITEM_BLESS) && IS_EVIL(ch)) ||
	(IS_OBJ_STAT(obj, ITEM_EVIL_BLESS) && IS_GOOD(ch))) {
	act("You are burned by $p as you try to assimilate it!",FALSE,ch,obj,0,TO_CHAR);
	act("$n screams in agony as $e tries to assimilate $p!",FALSE,ch,obj,0,TO_ROOM);
	damd = abs(GET_ALIGNMENT(ch));
	damd >>= 5;
	damd = MAX(5, damd);
	damage(ch, ch, dice(damd, 6), TOP_SPELL_DEFINE, -1);
	return;
    }
    
    if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
	(IS_OBJ_STAT(obj,ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
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
	if (num_affs && !(oldaffs = (struct affected_type *) malloc(sizeof(struct affected_type) * num_affs))) {
	    slog("SYSERR: unable to allocate oldaffs in do_assimilate.");
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
		    if ((oldaffs[j].modifier > 0 && obj->affected[i].modifier < 0) ||
			(oldaffs[j].modifier < 0 && obj->affected[i].modifier > 0)) {
		
			oldaffs[j].modifier += obj->affected[i].modifier;

		    }
		    // same signs, choose extreme value
		    else if (oldaffs[j].modifier > 0)
			oldaffs[j].modifier = MAX(oldaffs[j].modifier, obj->affected[i].modifier);
		    else
			oldaffs[j].modifier = MIN(oldaffs[j].modifier, obj->affected[i].modifier);

		    // reset level and duration
		    oldaffs[j].level = GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 2);
		    
		    oldaffs[j].duration = ((CHECK_SKILL(ch, SKILL_ASSIMILATE) / 10) + (oldaffs[j].level >> 4));
		    
		    found = 1;
		    break;
		    
		}
		
	    }
	    if (found)
		continue;
	    
	    num_newaffs++;
	    
	    // create a new aff
	    if (!(newaffs = (struct affected_type *) realloc(oldaffs, sizeof(struct affected_type) * (++num_affs)))) {
		slog("SYSERR: error reallocating newaffs in do_assimilate.");
		return;
	    }
	    
	    newaffs[num_affs-1].type = SKILL_ASSIMILATE;
	    newaffs[num_affs-1].level = GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 2);
	    newaffs[num_affs-1].duration = (CHECK_SKILL(ch, SKILL_ASSIMILATE) / 10) + (newaffs[num_affs-1].level >> 4);
	    newaffs[num_affs-1].modifier = obj->affected[i].modifier;
	    newaffs[num_affs-1].location = obj->affected[i].location;
	    newaffs[num_affs-1].bitvector = 0;
	    newaffs[num_affs-1].aff_index = 0;
	    
	    oldaffs = newaffs;
	}
	
    }
	
    act("You assimilate $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n assimilates $p...", TRUE, ch, obj, 0, TO_ROOM);

    // stick the affs on the char
    for (i = 0; i < num_affs; i++) {
	if (oldaffs[i].modifier)
	    affect_to_char(ch, oldaffs+i);
    }
    
    if (num_newaffs) {
	send_to_char("You feel... different.\r\n", ch);
	gain_skill_prof(ch, SKILL_ASSIMILATE);
	free(oldaffs);
    }
    
    extract_obj(obj);
}

ACMD(do_deassimilate)
{

    if (!IS_CYBORG(ch)) {
	send_to_char("Resistance is Futile.\r\n", ch);
	return;
    }

    if (CHECK_SKILL(ch, SKILL_ASSIMILATE) < 60) {
	send_to_char("Your programming is insufficient to deassimilate.\r\n", ch);
	return;
    }
    
    if (!affected_by_spell(ch, SKILL_ASSIMILATE)) {
	send_to_char("You currently have no active assimilations.\r\n", ch);
	return;
    }

    if (GET_MANA(ch) < 10) {
	send_to_char("Your BioEnergy level is too low to deassimilate.\r\n", ch);
	return;
    }

    affect_from_char(ch, SKILL_ASSIMILATE);

    GET_MANA(ch) -= 10;

    send_to_char("Deassimilation complete.\r\n", ch);

}
