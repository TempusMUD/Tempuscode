//
// File: act.hood.c                     -- Part of TempusMUD
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
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "guns.h"
#include "bomb.h"

 
int apply_soil_to_char(struct char_data *ch,struct obj_data *obj,int type,int pos);
void add_blood_to_room(struct room_data *rm, int amount);


ACMD(do_hamstring)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;
	struct affected_type af;


    one_argument(argument, arg);
	if (CHECK_SKILL(ch,SKILL_HAMSTRING) < 50) {
		send_to_char("Even if you knew what that was, you wouldn't do it.\r\n",ch);
	return;
	}
	// If there's noone in the room that matches your alias
	// Then it must be an object.
    if (!(vict = get_char_room_vis(ch, arg))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else {
			if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
				act("You open a deep gash in $p's hamstring!", FALSE, ch, ovict, 0, TO_CHAR);
				return;
			} else {
				send_to_char("Hamstring who?\r\n", ch);
				return;
			}
		}
	}
	if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && SLASHING(weap)) ||
	  ((weap = GET_EQ(ch, WEAR_WIELD_2)) && SLASHING(weap)) ||
	  ((weap = GET_EQ(ch, WEAR_HANDS)) && SLASHING(weap))	||
	  ((weap = GET_IMPLANT(ch, WEAR_HANDS)) && SLASHING(weap)) ||
	  ((weap = GET_IMPLANT(ch, WEAR_ARMS)) && SLASHING(weap)))
	  ) {
		send_to_char("You need to be using a slashing weapon.\r\n", ch);
		return;
	}
	if (vict == ch) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
			FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
		act("You manage to get your leg half removed before you die.", 
			FALSE, ch, weap, 0, TO_CHAR);
		act("$n starts hacking at $s his leg and bleeds to death!", TRUE, ch, weap, 0, TO_ROOM);
		sprintf(buf, "%s killed self with hamstring at %d.",
			GET_NAME(ch), ch->in_room->number);
		mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
		gain_exp(ch, -(GET_LEVEL(ch) * 1000));
		raw_kill(ch, ch, SKILL_HAMSTRING);
		return;
	}
	if (!peaceful_room_ok(ch, vict, true))
		return;

	prob = CHECK_SKILL(ch,SKILL_HAMSTRING) + GET_REMORT_GEN(ch);
	percent = number (0,101);
	if(affected_by_spell(vict,ZEN_AWARENESS)) {
		percent += 20;
	}
	if ( AFF2_FLAGGED(vict, AFF2_HASTE) && !AFF2_FLAGGED(ch, AFF2_HASTE) ) {
		percent += 20;
	}

	if ( GET_DEX(ch) > GET_DEX(vict) ) {
		prob += 3 * (GET_DEX(ch) - GET_DEX(vict));
	} else {
		percent += 3 * (GET_DEX(vict) - GET_DEX(ch));
	}

	if ( GET_LEVEL(ch) > GET_LEVEL(vict) ) {
		prob += GET_LEVEL(ch) - GET_LEVEL(vict);
	} else {
		percent += GET_LEVEL(vict) - GET_LEVEL(ch);
	}
	
	if (   IS_PUDDING(vict)|| IS_SLIME(vict)
		|| NON_CORPOREAL_UNDEAD(vict) || IS_ELEMENTAL(vict))
		prob = 0;
	if ( CHECK_SKILL(ch, SKILL_HAMSTRING) < 30 ) {
		prob = 0;
	}

	cur_weap = weap;
	if (percent > prob) {
		damage(ch, vict, 0, SKILL_HAMSTRING, WEAR_LEGS);
		WAIT_STATE(ch, 2 RL_SEC);
		return;
	} else {
		int level = 0, gen = 0;
		level = GET_LEVEL(ch);
		gen = GET_REMORT_GEN(ch);
		dam = dice(level, 20 + gen/2);
		add_blood_to_room(vict->in_room,1);
		apply_soil_to_char(ch, GET_EQ(vict,WEAR_LEGS), SOIL_BLOOD, WEAR_LEGS);
		apply_soil_to_char(ch, GET_EQ(vict,WEAR_FEET), SOIL_BLOOD, WEAR_FEET);
		if(!affected_by_spell(vict,SKILL_HAMSTRING)) {
			af.type = SKILL_HAMSTRING;
			af.bitvector = AFF3_HAMSTRUNG;
			af.aff_index = 3;
			af.level = level + gen;
			af.duration = level + gen  / 10;
			af.location = APPLY_DEX;
			af.modifier = -1 * (
							( level/2 + dice(7,7) + dice(gen,5)) 
							/ 10) 
							  * (CHECK_SKILL(ch,SKILL_HAMSTRING)/100);
			sprintf(buf,"Modifier: %d\r\n",af.modifier);
			send_to_char(buf,ch);
			affect_to_char(vict, &af);
			WAIT_STATE(vict, 6 RL_SEC);
			GET_POS(vict) = POS_RESTING;
			damage(ch, vict, dam, SKILL_HAMSTRING, WEAR_LEGS);
		} else {
			WAIT_STATE(vict, 3 RL_SEC);
			GET_POS(vict) = POS_SITTING;
			damage(ch, vict, dam/2, SKILL_HAMSTRING, WEAR_LEGS);
		}
		gain_skill_prof(ch, SKILL_HAMSTRING);
	}
	WAIT_STATE(ch, 2 RL_SEC);
}

