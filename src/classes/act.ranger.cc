//
// File: act.ranger.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.ranger.c
//

#define __act_ranger_c__

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
#include "flow_room.h"
#include "house.h"
#include "char_class.h"

ACMD(do_bandage)
{
    struct char_data *vict;
    int mod, cost;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);


    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
	if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
	    send_to_char("What makes you think you're bleeding?\r\n", ch);
	    return;
	}
	mod = 
	    number(GET_WIS(ch), 2*GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_BANDAGE)) >> 4;
	if (GET_CLASS(ch) != CLASS_RANGER && GET_REMORT_CLASS(ch) != CLASS_RANGER)
	    cost = mod*3;
	else cost = mod; 
	if (GET_MOVE(ch) > cost) {
	    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost,  0);
	    send_to_char("You carefully bandage your wounds.\r\n", ch);
	    act("$n carefully bandages $s wounds.", TRUE, ch, 0, 0, TO_ROOM);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You cannot do this now.\r\n", ch);
	return;
    } else if (!(vict = get_char_room_vis(ch, vict_name))) {
	send_to_char("Bandage who?\r\n", ch);
	return;
    } else { 
	if (GET_HIT(vict) == GET_MAX_HIT(vict)) {
	    send_to_char("What makes you think they're bleeding?\r\n", ch);
	    return;
	} else if (FIGHTING(vict) || vict->getPosition() == POS_FIGHTING) {
	    send_to_char("Bandage someone who is in battle?  How's that?\r\n", ch);
	    return;
	}
	mod = number((GET_WIS(ch) >> 1), 
		     11+GET_LEVEL(ch)+CHECK_SKILL(ch, SKILL_BANDAGE))/10;
	if (GET_CLASS(ch) != CLASS_RANGER && GET_REMORT_CLASS(ch) != CLASS_RANGER)
	    cost = mod*3;
	else cost = mod;
	if (GET_MOVE(ch) > cost) {
	    GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
	    act("$N bandages your wounds.", TRUE, vict, 0, ch, TO_CHAR);
	    act("$n bandages $N's wounds.", FALSE, ch, 0, vict, TO_NOTVICT);
	    send_to_char("You do it.\r\n", ch);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You cannot do this now.\r\n", ch);
    }
}

ACMD(do_firstaid)
{
    struct char_data *vict;
    int mod, cost;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);


    if (CHECK_SKILL(ch, SKILL_FIRSTAID) < (IS_NPC(ch) ? 50 : 30)) {
	send_to_char("You have no clue about first aid.\r\n", ch);
	return;
    }
    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
	if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
	    send_to_char("What makes you think you're bleeding?\r\n", ch);
	    return;
	}
	mod = (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_FIRSTAID)) >> 2;
	cost = MAX(0, mod - (GET_LEVEL(ch)/100)*mod);
	if (GET_MOVE(ch) > cost) {
	    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
	    send_to_char("You carefully tend to your wounds.\r\n", ch);
	    act("$n carefully tends to $s wounds.", TRUE, ch, 0, 0, TO_ROOM);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You are too tired to do this now.\r\n", ch);
    } else if (!(vict = get_char_room_vis(ch, vict_name))) {
	send_to_char("Provide first aid to who?\r\n", ch);
	return;
    } else { 
	if (GET_HIT(vict) == GET_MAX_HIT(vict)) {
	    send_to_char("What makes you think they're bleeding?\r\n", ch);
	    return;
	} else if (FIGHTING(vict) || vict->getPosition() == POS_FIGHTING) {
	    send_to_char("They are fighting right now.\r\n", ch);
	    return;
	}    

	mod = (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_FIRSTAID)) >> 2;
	cost = MAX(0, mod - (GET_LEVEL(ch)/100)*mod);
	if (GET_MOVE(ch) > cost) {
	    GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
	    act("$N performs first aid on your wounds.", TRUE, vict, 0, ch, TO_CHAR);
	    act("$n performs first aid on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
	    send_to_char("You do it.\r\n", ch);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You must rest awhile before doing this again.\r\n", ch);
    }
}  
ACMD(do_medic)
{
    struct char_data *vict;
    int mod;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);


    if (CHECK_SKILL(ch, SKILL_MEDIC) < (IS_NPC(ch) ? 50 : 30)) {
	send_to_char("You have no clue about first aid.\r\n", ch);
	return;
    }
    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
	if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
	    send_to_char("What makes you think you're bleeding?\r\n", ch);
	    return;
	}
	mod = (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_MEDIC) + GET_REMORT_GEN(ch));
	if (GET_MOVE(ch) > mod) {
	    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
	    send_to_char("You apply some TLC to your wounds.\r\n", ch);
	    act("$n fixes up $s wounds.", TRUE, ch, 0, 0, TO_ROOM);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You are too tired to do this now.\r\n", ch);
    }
    else if (!(vict = get_char_room_vis(ch, vict_name))) {
	send_to_char("Who do you wish to help out?\r\n", ch);
	return;
    } else if (FIGHTING(vict) || vict->getPosition() == POS_FIGHTING) {
	send_to_char("They are fighting right now.\r\n", ch);
	return;
    } else {
	mod = (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_MEDIC));
	if (GET_MOVE(ch) > mod) {
	    GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
	    act("$N gives you some TLC.  You feel better!", TRUE, vict, 0, ch, TO_CHAR);
	    act("$n gives some TLC to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
	    send_to_char("You do it.\r\n", ch);
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	} else
	    send_to_char("You must rest awhile before doing this again.\r\n", ch);
    } 
}

ACMD(do_autopsy)
{
    struct char_data *vict = NULL;
    struct obj_data *corpse = NULL;
    char *name = NULL;

    skip_spaces(&argument);

    if (!(corpse = get_obj_in_list_vis(ch, argument, ch->in_room->contents))
	&& !(corpse = get_obj_in_list_vis(ch, argument, ch->carrying))) {
	send_to_char("Perform autopsy on what corpse?\r\n", ch);
	return;
    }

    if (GET_OBJ_TYPE(corpse) != ITEM_CONTAINER || !GET_OBJ_VAL(corpse, 3)) {
	send_to_char("You can only autopsy a corpse.\r\n", ch);
	return;
    }

    if (CHECK_SKILL(ch, SKILL_AUTOPSY) < 30) {
	send_to_char("Your forensic skills are not good enough.\r\n", ch);
	return;
    }
    if (CORPSE_KILLER(corpse) > 0) {
	if (!(name = get_name_by_id(CORPSE_KILLER(corpse)))) {
	    send_to_char("The result is indeterminate.\r\n", ch);
	    return;
	}
    } else if (CORPSE_KILLER(corpse) < 0) {
	if (!(vict = real_mobile_proto(-CORPSE_KILLER(corpse)))) {
	    send_to_char("The result is indeterminate.\r\n", ch);
	    return;
	}
    } else {
	act("$p is too far messed up to discern.",FALSE,ch,corpse,0,TO_CHAR);
	return;
    }

    act("$n examines $p.",TRUE,ch,corpse,0,TO_ROOM);

    if (number(30, 151) > GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_AUTOPSY))
	send_to_char("You cannot determine the identity of the killer.\r\n", ch);
    else {
	if (vict)
	    act("The killer seems to have been $N.",FALSE,ch,corpse,vict,TO_CHAR);
	else {
	    sprintf(buf, "The killer seems to have been %s.\r\n", CAP(name));
	    send_to_char(buf, ch);
	}
	gain_skill_prof(ch, SKILL_AUTOPSY);
    }
}

#undef __act_ranger_c__
