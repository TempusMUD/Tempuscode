/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: mobact.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "char_class.h"
#include "vehicle.h"
#include "flow_room.h"
#include "char_class.h"
#include "guns.h"
#include "bomb.h"
#include "mobact.h"

/* external structs */
extern struct char_data *character_list;
void npc_steal(struct char_data * ch, struct char_data * victim);
void hunt_victim(struct char_data * ch);
void gain_skill_prof(struct char_data *ch, int skl);
void engage_self_destruct(struct char_data *ch);
void perform_tell(struct char_data *ch, struct char_data *vict, char *messg);
int mag_manacost(struct char_data *ch, int spellnum);
int CLIP_COUNT(struct obj_data *clip);
int find_first_step(struct room_data *src, struct room_data *target,byte mode);
int tarrasque_fight(struct char_data *tarr);
int general_search(struct char_data *ch, struct special_search_data *srch,int mode);
void add_blood_to_room(struct room_data *rm, int amount);
int smart_mobile_move(struct char_data *ch, int dir);
int mob_fight_devil( CHAR * ch );

ACMD(do_flee);
ACMD(do_sleeper);
ACMD(do_stun);
ACMD(do_circle);
ACMD(do_backstab);
ACMD(do_say);
ACMD(do_feign);
ACMD(do_hide);
ACMD(do_offensive_skill);
ACMD(do_gen_comm);
ACMD(do_shoot);
ACMD(do_remove);
ACMD(do_drop);
ACMD(do_load);
ACMD(do_get);
ACMD(do_battlecry);
ACMD(do_psidrain);
ACMD(do_fly);
ACMD(do_extinguish);

// for mobile_activity
ACMD(do_get);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_shoot);
ACMD(do_holytouch);
ACMD(do_medic);
ACMD(do_firstaid);
ACMD(do_bandage);


SPECIAL(thief);
SPECIAL(cityguard);
SPECIAL(hell_hunter);

int
mob_read_script(struct char_data *ch);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)
#define IS_RACIALLY_AGGRO(ch) \
(GET_RACE(ch) == RACE_GOBLIN || \
 GET_RACE(ch) == RACE_ALIEN_1 || \
 GET_RACE(ch) == RACE_ORC)
#define RACIAL_ATTACK(ch, vict) \
    ((GET_RACE(ch) == RACE_GOBLIN && GET_RACE(vict) == RACE_DWARF) || \
     (GET_RACE(ch) == RACE_ALIEN_1 && GET_RACE(vict) == RACE_HUMAN) || \
     (GET_RACE(ch) == RACE_ORC && GET_RACE(vict) == RACE_DWARF))

    void 
burn_update(void)
{
    register struct char_data *ch, *next_ch;
    struct obj_data *obj = NULL;
    struct room_data *fall_to = NULL;
    struct special_search_data *srch = NULL;
    int dam = 0, found = 0;
    struct affected_type *af;

    for (ch = character_list; ch; ch = next_ch) {
	next_ch = ch->next;
	if (!ch->in_room)
	    continue;

	if (!FIGHTING(ch) && GET_MOB_WAIT(ch))
	    GET_MOB_WAIT(ch) = MAX(0, GET_MOB_WAIT(ch) - FIRE_TICK);

	if ((IS_NPC(ch) && ZONE_FLAGGED(ch->in_room->zone, ZONE_FROZEN)) ||
	    IS_AFFECTED_2(ch, AFF2_PETRIFIED))
	    continue;

	// char is flying but unable to continue
	if (GET_POS(ch) == POS_FLYING && !AFF_FLAGGED(ch, AFF_INFLIGHT) &&
	    GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    send_to_char("You can no longer fly!\r\n", ch);
	    GET_POS(ch) = POS_STANDING;
	}

	// character is in open air
	if (ch->in_room->isOpenAir() &&
	    !NOGRAV_ZONE(ch->in_room->zone) &&
	    GET_POS(ch) < POS_FLYING &&
	    (!FIGHTING(ch) || !AFF_FLAGGED(ch, AFF_INFLIGHT)) &&
	    ch->in_room->dir_option[DOWN] &&
	    (fall_to = ch->in_room->dir_option[DOWN]->to_room) &&
	    fall_to != ch->in_room &&
	    !IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED)) {
	    if (AFF_FLAGGED(ch, AFF_INFLIGHT) && AWAKE(ch)) {
		send_to_char("You realize you are about to fall and resume your flight!\r\n", ch);
		GET_POS(ch) = POS_FLYING;
	    } else {

		act("$n falls downward through the air!", TRUE, ch, 0, 0, TO_ROOM);
		act("You fall downward through the air!", TRUE, ch, 0, 0, TO_CHAR);
	
		char_from_room(ch);
		char_to_room(ch, fall_to);
		look_at_room(ch, ch->in_room, 0);
		act("$n falls in from above!", TRUE, ch, 0, 0, TO_ROOM);
		GET_FALL_COUNT(ch)++;

		if ( !ch->in_room->isOpenAir() ||
		    !ch->in_room->dir_option[DOWN] ||
		    !(fall_to = ch->in_room->dir_option[DOWN]->to_room) ||
		    fall_to == ch->in_room ||
		    IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED)) {
		    /* hit the ground */
	  
		    dam = dice(GET_FALL_COUNT(ch) * (GET_FALL_COUNT(ch) + 2), 12);
		    if ((SECT_TYPE(ch->in_room) >= SECT_WATER_SWIM &&
			 SECT_TYPE(ch->in_room) <= SECT_UNDERWATER) ||
			SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER ||
			SECT_TYPE(ch->in_room) == SECT_PITCH_PIT ||
			SECT_TYPE(ch->in_room) == SECT_PITCH_SUB) {
			dam >>= 1;
	    
			act("$n lands hard!", TRUE, ch, 0, 0, TO_ROOM);
			act("You land hard!", TRUE, ch, 0, 0, TO_CHAR);
		    } 
		    else {
			act("$n hits the ground hard!", TRUE, ch, 0, 0, TO_ROOM);
			act("You hit the ground hard!", TRUE, ch, 0, 0, TO_CHAR);
		    }
	  
		    for (srch=ch->in_room->search,found = 0; srch; srch = srch->next) {
			if (SRCH_FLAGGED(srch, SRCH_TRIG_FALL) && SRCH_OK(ch, srch)) {
			    found = general_search(ch, srch, found);
			}
		    }
		    if (found == 2)
			continue;
	  
		    if (IS_MONK(ch))
			dam -= (GET_LEVEL(ch) * dam) / 100;

		    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
			dam = 0;
	  
		    GET_POS(ch) = POS_RESTING;
		    if (dam && damage(NULL, ch, dam, TYPE_FALLING, WEAR_RANDOM))
			continue;
		    GET_FALL_COUNT(ch) = 0;
		}
	    }
	}

	// regen
	if (AFF_FLAGGED(ch, AFF_REGEN) || IS_TROLL(ch) || IS_VAMPIRE(ch))
	    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 1 + number(0, (GET_CON(ch) / 5)));
    
	// mana tap
	if (AFF3_FLAGGED(ch, AFF3_MANA_TAP))
	    GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 1 + number(0, (GET_WIS(ch) >> 2)));
    
	// energy tap
	if (AFF3_FLAGGED(ch, AFF3_ENERGY_TAP))
	    GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 1 + number(0, (GET_CON(ch) >> 2)));

	// affected by sleep spell
	if (GET_POS(ch) > POS_SLEEPING && AFF_FLAGGED(ch, AFF_SLEEP) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    send_to_char("You suddenly fall into a deep sleep.\r\n", ch);
	    act("$n suddenly falls asleep where $e stands.", TRUE, ch, 0, 0, TO_ROOM);
	    GET_POS(ch) = POS_SLEEPING;
	}

	// self destruct
	if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
	    if (MEDITATE_TIMER(ch)) {
		sprintf(buf, "Self-destruct T-minus %d and counting.\r\n", MEDITATE_TIMER(ch));
		send_to_char(buf, ch);
		MEDITATE_TIMER(ch)--;
	    } else {
		if (!IS_CYBORG(ch)) {
		    sprintf(buf, "SYSERR: %s tried to self destruct at [%d].", GET_NAME(ch), ch->in_room->number);
		    REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
		    mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
		} else {
		    engage_self_destruct(ch);
		    continue;
		}
	    }
	} 
	// character is poisoned (3)
	if (HAS_POISON_3(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    if (damage(ch, ch, dice(4, 3) + (affected_by_spell(ch, SPELL_METABOLISM) ? dice (4, 11) : 0), SPELL_POISON, -1))
		continue;
	}
	// psychic crush
	if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH)) {
	    if (damage(ch, ch, mag_savingthrow(ch, 50, SAVING_PSI) ? 0 : dice(4, 20), SPELL_PSYCHIC_CRUSH, WEAR_HEAD))
		continue;
	}
	// character has a stigmata
	if ((af = affected_by_spell(ch, SPELL_STIGMATA))) {
	    if (damage(ch, ch, mag_savingthrow(ch, af->level, SAVING_SPELL) ? 0 : dice(3, af->level), SPELL_STIGMATA, WEAR_FACE))
		continue;
	}
	// character has acidity
	if (AFF3_FLAGGED(ch, AFF3_ACIDITY)) {
	    if (damage(ch, ch, mag_savingthrow(ch, 50, SAVING_PHY) ? 0 : dice(2, 10), TYPE_ACID_BURN, -1))
		continue;
	}

	// motor spasm
	if ((af = affected_by_spell(ch, SPELL_MOTOR_SPASM)) && !MOB_FLAGGED(ch, MOB_NOBASH) &&
	    GET_POS(ch) < POS_FLYING && GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    if (number(3, 6 + (af->level >> 2)) > GET_DEX(ch) && 
		(obj = ch->carrying)) {
		while (obj) {
		    if (CAN_SEE_OBJ(ch, obj) && !IS_OBJ_STAT(obj, ITEM_NODROP))
			break;
		    obj = obj->next_content;
		}
		if (obj) {
		    send_to_char("Your muscles are seized in an uncontrollable spasm!\r\n", ch);
		    act("$n begins spasming uncontrollably.",
			TRUE, ch, 0, 0, TO_ROOM);
		    do_drop(ch, fname(obj->name), 0, SCMD_DROP);
		}
	    }
	    if (!obj && number(0, 12 + (af->level >> 2)) > GET_DEX(ch) && 
		GET_POS(ch) > POS_SITTING) {
		send_to_char("Your muscles are seized in an uncontrollable spasm!\r\n"
			     "You fall to the ground in agony!\r\n", ch);
		act("$n begins spasming uncontrollably and falls to the ground.",
		    TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_RESTING;
	    }
	    WAIT_STATE(ch, 4);
	}

	// hyperscanning increment
	if ((af = affected_by_spell(ch, SKILL_HYPERSCAN))) {
	    if (GET_MOVE(ch) < 10) {
		send_to_char("Hyperscanning device shutting down.\r\n", ch);
		affect_remove(ch, af);
	    } else
		GET_MOVE(ch) -= 1;
	}

	// burning character
	if (IS_AFFECTED_2(ch, AFF2_ABLAZE)) {
	    if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
		SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
		SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM) {
		send_to_char("The flames on your body sizzle out and die, leaving you in a cloud of steam.\r\n", ch);
		act("The flames on $n sizzle and die, leaving a cloud of steam.", 
		    FALSE, ch, 0, 0, TO_ROOM);
		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
	    } else if (number(0, 2)) {
		if (damage(ch, ch, 
			   CHAR_WITHSTANDS_FIRE(ch) ? 0 :
			   ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) ? dice (8, 7) : 
			   dice(5, 5), TYPE_ABLAZE, -1))
		    continue;
		if (IS_MOB(ch) && GET_POS(ch) >= POS_RESTING &&
		    !GET_MOB_WAIT(ch) && !CHAR_WITHSTANDS_FIRE(ch)) 
		    do_extinguish(ch, "", 0, 0);
	    }      
	}
	// burning rooms
	else if ((ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) &&
		  (!CHAR_WITHSTANDS_FIRE(ch) || 
		   ROOM_FLAGGED(ch->in_room, ROOM_GODROOM))) ||
		 (IS_VAMPIRE(ch) && OUTSIDE(ch) && 
		  ch->in_room->zone->weather->sunlight == SUN_LIGHT && 
		  GET_PLANE(ch->in_room) < PLANE_ASTRAL)) {
	    send_to_char("Your body suddenly bursts into flames!\r\n", ch);
	    act("$n suddenly bursts into flames!", FALSE, ch, 0, 0, TO_ROOM);
	    GET_MANA(ch) = 0;
	    SET_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
	    if (damage(ch, ch, dice(4, 5), TYPE_ABLAZE, -1))
		continue;
	} 
	// freezing room
	else if (ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD) && !CHAR_WITHSTANDS_COLD(ch)) {
	    if (damage(ch, ch, dice(4, 5), TYPE_FREEZING, -1))
		continue;
	    if (IS_MOB(ch) && (IS_CLERIC(ch) || IS_RANGER(ch)) && GET_LEVEL(ch) > 15)
		cast_spell(ch, ch, 0, SPELL_ENDURE_COLD);
      
	} 
	// holywater ocean
	else if (ROOM_FLAGGED(ch->in_room, ROOM_HOLYOCEAN) && IS_EVIL(ch) &&  GET_POS(ch) < POS_FLYING) {
	    if (damage(ch, ch, dice(4, 5), TYPE_HOLYOCEAN, WEAR_RANDOM))
		continue;
	    if (IS_MOB(ch) && !FIGHTING(ch)) {
		do_flee(ch, "", 0, 0);
	    }
	} 
	// boiling pitch
	else if ((ch->in_room->sector_type == SECT_PITCH_PIT || ch->in_room->sector_type == SECT_PITCH_SUB) &&
		 !CHAR_WITHSTANDS_HEAT(ch)) {
	    if (damage(ch, ch, dice(4, 3), TYPE_BOILING_PITCH, WEAR_RANDOM))
		continue;
	}

	// radioactive room
	if (ROOM_FLAGGED(ch->in_room, ROOM_RADIOACTIVE) && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) &&
	    !CHAR_WITHSTANDS_RAD(ch)) {

	    if ( affected_by_spell(ch, SKILL_RADIONEGATION) ) {
	    
		GET_HIT(ch) = MAX(0, GET_HIT(ch) - dice(1, 7));

		if ( GET_MOVE(ch) > 10 ) {
		    GET_MOVE(ch) = MAX(10, GET_MOVE(ch) - dice(2, 7));
		}

		else {
		    add_rad_sickness(ch, 10);
		}
	    }

	    else {

		GET_HIT(ch)  = MAX(0, GET_HIT(ch) - dice(2, 7));
	    
		if (GET_MOVE(ch) > 5)
		    GET_MOVE(ch) = MAX(5, GET_MOVE(ch) - dice(2, 7));

		add_rad_sickness(ch, 20);

	    }
	}

	// underwater (underliquid)
	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
	     SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
	     SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM) &&
	    !can_travel_sector(ch, SECT_TYPE(ch->in_room), 1) &&
	    !ROOM_FLAGGED(ch->in_room, ROOM_DOCK) && 
	    GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    
	    int drown_factor = ch->getBreathCount() - ch->getBreathThreshold();
	    
	    drown_factor = MAX( 0, drown_factor );
	    
	    if (damage(ch, ch, dice(4, 5), TYPE_DROWNING, -1))
		continue;
	    
	    if (AFF_FLAGGED(ch, AFF_INFLIGHT) && 
		GET_POS(ch) < POS_FLYING &&
		SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)
		do_fly(ch, "", 0, 0);
	}

	// sleeping gas
	if (ROOM_FLAGGED(ch->in_room, ROOM_SLEEP_GAS) && 
	    GET_POS(ch) > POS_SLEEPING && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
	    send_to_char("You feel very sleepy...\r\n", ch);
	    if (!AFF_FLAGGED(ch, AFF_ADRENALINE)) {
		if (!mag_savingthrow(ch, 50, SAVING_CHEM)) {
		    send_to_char("You suddenly feel very sleepy and collapse where you stood.\r\n", ch);
		    act("$n suddenly falls asleep and collapses!",
			TRUE, ch, 0, 0, TO_ROOM);
		    GET_POS(ch) = POS_SLEEPING;
		    WAIT_STATE(ch, 4 RL_SEC);
		    continue;
		} else
		    send_to_char("You fight off the effects.\r\n", ch);
	    }
	}
    
	// poison gas
	if (ROOM_FLAGGED(ch->in_room, ROOM_POISON_GAS) && 
	    !PRF_FLAGGED(ch, PRF_NOHASSLE) && !IS_UNDEAD(ch) &&
	    !AFF3_FLAGGED(ch, AFF3_NOBREATHE)) {

	    if (!HAS_POISON_3(ch)) {
		send_to_char("Your lungs burn as you inhale a poisonous gas!\r\n", ch);
		act("$n begins choking and sputtering!",FALSE,ch,0,0,TO_ROOM);
	    }

	    found = 1;
	    if (HAS_POISON_2(ch))
		call_magic(ch, ch, NULL, SPELL_POISON, 60, CAST_CHEM);
	    else if (HAS_POISON_1(ch))
		call_magic(ch, ch, NULL, SPELL_POISON, 39, CAST_CHEM);
	    else if (!HAS_POISON_3(ch))
		call_magic(ch, ch, NULL, SPELL_POISON, 10, CAST_CHEM);
	    else
		found = 0;

	    if (found)
		continue;
	}

	//
	// animated corpses decaying
	//

	if ( IS_NPC(ch) && GET_MOB_VNUM(ch) == ZOMBIE_VNUM && 
	     OUTSIDE(ch) && IS_LIGHT(ch->in_room) && PRIME_MATERIAL_ROOM(ch->in_room))
	    if ( damage(ch, ch, dice(4, 5), TOP_SPELL_DEFINE, -1) )
		continue;
	
	/* Hunter Mobs */
	if (HUNTING(ch) && !AFF_FLAGGED(ch, AFF_BLIND) &&
	    GET_POS(ch) > POS_SITTING && !GET_MOB_WAIT(ch))
	    hunt_victim(ch);
    
    }
}

int 
helper_assist(struct char_data *ch, struct char_data *vict, 
	      struct char_data *fvict)
{
    int prob = 0;
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

    if (!ch || !vict || !fvict) {
	slog("Illegal value(s) passed to helper_assist().");
	return 0;
    } else {

	if (GET_RACE(ch) == GET_RACE(fvict))
	    prob += 10 + (GET_LEVEL(ch) >> 1);
	else if  (GET_RACE(ch) != GET_RACE(vict))
	    prob -= 10;
	else if (IS_DEVIL(ch) || IS_KNIGHT(ch) || IS_WARRIOR(ch) ||
		 IS_RANGER(ch))
	    prob += GET_LEVEL(ch);


	if (weap) {
	    if (IS_THIEF(ch) && GET_LEVEL(ch) > 43 &&
		(GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT ||
		 GET_OBJ_VAL(weap, 3) == TYPE_STAB   - TYPE_HIT)) {
		do_circle(ch, GET_NAME(vict), 0, 0);
		return 1;
	    }

	    if (IS_ENERGY_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
		EGUN_CUR_ENERGY(weap) > 10) {
		CUR_R_O_F(weap) = MAX_R_O_F(weap);
		sprintf(buf, "%s ", fname(weap->name));
		strcat(buf, fname(vict->player.name));
		do_shoot(ch, buf, 0, 0);
		return 1;
	    }

	    if (IS_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
		GUN_LOADED(weap)) {
		CUR_R_O_F(weap) = MAX_R_O_F(weap);
		sprintf(buf, "%s ", fname(weap->name));
		strcat(buf, fname(vict->player.name));
		do_shoot(ch, buf, 0, 0);
		return 1;
	    }
	}

	act("$n jumps to the aid of $N!", TRUE, ch, 0, fvict, TO_NOTVICT);
	act("$n jumps to your aid!", TRUE, ch, 0, fvict, TO_VICT);

	if (prob > number(0, 101)) {
	    stop_fighting(vict);
	    stop_fighting(fvict);
	}

	hit(ch, vict, TYPE_UNDEFINED);
	return 1;
    }
    return 0;
}

void 
mob_load_unit_gun(struct char_data *ch, struct obj_data *clip, 
		  struct obj_data *gun, bool internal)
{
    char loadbuf[1024];
    sprintf(loadbuf, "%s %s", fname(clip->name), internal ? "internal " : "");
    strcat(loadbuf, fname(gun->name));
    do_load(ch, loadbuf, 0, SCMD_LOAD);
    if (GET_MOB_VNUM(ch) == 1516 && IS_CLIP(clip) && !number(0, 1))
	do_say(ch, "Let's Rock.", 0, 0);
    return;
}

struct obj_data *
find_bullet(struct char_data *ch, int gun_type,
	    struct obj_data *list)
{
    struct obj_data *bul = NULL;

    for (bul = list; bul; bul = bul->next_content) {
	if (CAN_SEE_OBJ(ch, bul) &&
	    IS_BULLET(bul) && GUN_TYPE(bul) == gun_type)
	    return (bul);
    }
    return NULL;
}
    
void 
mob_reload_gun(struct char_data *ch, struct obj_data *gun)
{
    bool internal = false;
    int count = 0, i;
    struct obj_data *clip = NULL, *bul = NULL, *cont = NULL;
  
    if (GET_INT(ch) < number(3, 7))
	return;

    if (gun->worn_by && gun != GET_EQ(ch, gun->worn_on))
	internal = true;

    if (IS_GUN(gun)) {
	if (!MAX_LOAD(gun)) {   /** a gun that uses a clip **/
	    if (gun->contains) {
		sprintf(buf, "%s%s", internal ? "internal " : "", fname(gun->name));
		do_load(ch, buf, 0, SCMD_UNLOAD);
	    }
      
	    /** look for a clip that matches the gun **/
	    for (clip = ch->carrying; clip; clip = clip->next_content)
		if (IS_CLIP(clip) && GUN_TYPE(clip) == GUN_TYPE(gun))
		    break;

	    if (!clip)
		return;

	    if ((count = CLIP_COUNT(clip)) >= MAX_LOAD(clip)) {  /** a full clip **/
		mob_load_unit_gun(ch, clip, gun, internal);
		return;
	    }

	    /** otherwise look for a bullet to load into the clip **/
	    if ((bul = find_bullet(ch, GUN_TYPE(gun), ch->carrying))) {
		mob_load_unit_gun(ch, bul, clip, FALSE);
		count++;
		if (count >= MAX_LOAD(clip))  /** load full clip in gun **/
		    mob_load_unit_gun(ch, clip, gun, internal);
		return;
	    }

	    /* no bullet found, look for one in bags **/
	    for (cont = ch->carrying; cont; cont = cont->next_content) {
		if (!CAN_SEE_OBJ(ch, cont) || !IS_OBJ_TYPE(cont, ITEM_CONTAINER) || 
		    CAR_CLOSED(cont))
		    continue;
		if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
		    sprintf(buf, "%s ", fname(bul->name));
		    strcat(buf, fname(cont->name));
		    do_get(ch, buf, 0, 0);
		    mob_load_unit_gun(ch, bul, clip, FALSE);
		    count++;
		    if (count >= MAX_LOAD(clip))  /** load full clip in gun **/
			mob_load_unit_gun(ch, clip, gun, internal);
		    return;
		}
	    }
	    for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
		if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER) &&
		    !CAR_CLOSED(cont) &&
		    (bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
		    sprintf(buf, "%s ", fname(bul->name));
		    strcat(buf, fname(cont->name));
		    do_get(ch, buf, 0, 0);
		    mob_load_unit_gun(ch, bul, clip, FALSE);
		    count++;
		    if (count >= MAX_LOAD(clip))  /** load full clip in gun **/
			mob_load_unit_gun(ch, clip, gun, internal);
		    return;
		}
	    }
      
	    /** might as well load it now **/
	    if (count > 0)
		mob_load_unit_gun(ch, clip, gun, internal);
      
	    return;
	}
	/************************************/
	/** a gun that does not use a clip **/
	if ((bul = find_bullet(ch, GUN_TYPE(gun), ch->carrying))) {
	    mob_load_unit_gun(ch, bul, gun, internal);
	    return;
	}

	/* no bullet found, look for one in bags **/
	for (cont = ch->carrying; cont; cont = cont->next_content) {
	    if (!CAN_SEE_OBJ(ch, cont) || !IS_OBJ_TYPE(cont, ITEM_CONTAINER) || 
		CAR_CLOSED(cont))
		continue;
	    if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
		sprintf(buf, "%s ", fname(bul->name));
		strcat(buf, fname(cont->name));
		do_get(ch, buf, 0, 0);
		mob_load_unit_gun(ch, bul, gun, internal);
	    }
	}
	for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
	    if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER) &&
		!CAR_CLOSED(cont) &&
		(bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
		sprintf(buf, "%s ", fname(bul->name));
		strcat(buf, fname(cont->name));
		do_get(ch, buf, 0, 0);
		mob_load_unit_gun(ch, bul, gun, internal);
		return;
	    }
	}
	return;
    }
    /** energy guns **/
}
  
void
best_attack(struct char_data *ch, struct char_data *vict)
{
    struct obj_data *gun = GET_EQ(ch, WEAR_WIELD);
    int found = 0;

    if (GET_POS(ch) < POS_STANDING) {
	act("$n jumps to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
	GET_POS(ch) = POS_STANDING;
    }
  
    if (CHECK_SKILL(ch, SKILL_SHOOT) > number(30, 40)) {

	if (((gun = GET_EQ(ch, WEAR_WIELD)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_EQ(ch, WEAR_WIELD_2)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_IMPLANT(ch, WEAR_WIELD)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_IMPLANT(ch, WEAR_WIELD_2)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun))))) {

	    CUR_R_O_F(gun) = MAX_R_O_F(gun);      

	    sprintf(buf, "%s ", fname(gun->name));
	    strcat(buf, fname(vict->player.name));
	    do_shoot(ch, buf, 0, 0);
	    return;
	}
    }
	
    if (IS_THIEF(ch)) {

	if (GET_LEVEL(ch) >= 35 && GET_POS(vict) > POS_STUNNED) 
	    do_stun(ch, fname(vict->player.name), 0, 0);
	  
	else if (((gun = GET_EQ(ch, WEAR_WIELD)) && STAB_WEAPON(gun)) ||
		 ((gun = GET_EQ(ch, WEAR_WIELD_2)) && STAB_WEAPON(gun)) ||
		 ((gun = GET_EQ(ch, WEAR_HANDS)) && STAB_WEAPON(gun))) {
	    
	    if (!FIGHTING(vict))
		do_backstab(ch, fname(vict->player.name), 0, 0);
	    else if (GET_LEVEL(ch) > 43)
		do_circle(ch, fname(vict->player.name), 0, 0);
	    else if (GET_LEVEL(ch) >= 30)
		do_offensive_skill(ch, fname(vict->player.name), 0, SKILL_GOUGE);
	    else {
		hit(ch, vict, TYPE_UNDEFINED);
	    }
	} else {
	    if (GET_LEVEL(ch) >= 30)
		do_offensive_skill(ch, fname(vict->player.name), 0, SKILL_GOUGE);
	    else {
		hit(ch, vict, TYPE_UNDEFINED);
	    }
	} 
	return;
    }

    if (IS_PSIONIC(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
	if (GET_LEVEL(ch) >= 33 &&
	    AFF3_FLAGGED(vict, AFF3_PSISHIELD) &&
	    GET_MANA(ch) > mag_manacost(ch, SPELL_PSIONIC_SHATTER)) {
	    cast_spell(ch, vict, NULL, SPELL_PSIONIC_SHATTER);
	    found = TRUE;
	} else if (GET_POS(vict) > POS_SLEEPING) {
	    found = TRUE;
	    if (GET_LEVEL(ch) >= 40 &&
		GET_MANA(ch) > mag_manacost(ch, SPELL_PSYCHIC_SURGE))
		cast_spell(ch, vict, NULL, SPELL_PSYCHIC_SURGE);
	    else if (GET_LEVEL(ch) >= 18 &&
		     GET_MANA(ch) > mag_manacost(ch, SPELL_MELATONIC_FLOOD))
		cast_spell(ch, vict, NULL, SPELL_MELATONIC_FLOOD);
	    else
		found = FALSE;
	}
	if (!found) {
	    found = TRUE;
	    if (GET_LEVEL(ch) >= 36 &&
		(IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
		 IS_KNIGHT(vict) || IS_PHYSIC(vict)) &&
		!AFF_FLAGGED(vict, AFF_CONFUSION) &&
		GET_MANA(ch) > mag_manacost(ch, SPELL_CONFUSION))
		cast_spell(ch, vict, NULL, SPELL_CONFUSION);
	    else if (GET_LEVEL(ch) >= 31 &&
		     !AFF2_FLAGGED(ch, AFF2_VERTIGO) &&
		     GET_MANA(ch) > mag_manacost(ch, SPELL_VERTIGO))
		cast_spell(ch, vict, NULL, SPELL_VERTIGO);
	    else found = FALSE;
	}
	if (found)
	    return;
    }

    if (IS_MAGE(ch) && GET_MANA(ch) > 100 && 
	!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
	if (GET_LEVEL(ch) >= 37 && GET_POS(vict) > POS_SLEEPING)
	    cast_spell(ch, vict, NULL, SPELL_WORD_STUN);
	else if (GET_LEVEL(ch) < 5)
	    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
	else if (GET_LEVEL(ch) < 9 && !CHAR_WITHSTANDS_COLD(vict))
	    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
	else if (GET_LEVEL(ch) < 11)
	    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
	else if (GET_LEVEL(ch) < 15 && !CHAR_WITHSTANDS_FIRE(vict))
	    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
	else if (GET_LEVEL(ch) < 18 || IS_CYBORG(vict))
	    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
	else if (GET_LEVEL(ch) < 31)
	    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
	else if (GET_LEVEL(ch) < 36)
	    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
	else if (GET_LEVEL(ch) < 42)
	    cast_spell(ch, vict, NULL, SPELL_PRISMATIC_SPRAY);
	else {
	    hit(ch, vict, TYPE_UNDEFINED);
	}
	return;
    }
   
    if (IS_BARB(ch) || IS_WARRIOR(ch)) {
	    
	if (GET_LEVEL(ch) >= 25 && GET_POS(vict) > POS_SLEEPING) 
	    do_sleeper(ch, fname(vict->player.name), 0, 0);
	else if (GET_LEVEL(ch) >= 35 && 
		 ((GET_WEIGHT(vict) +
		   ((IS_CARRYING_W(vict)+IS_WEARING_W(vict)) >> 1)) < 
		  CAN_CARRY_W(ch)*1.4) &&
		 (GET_POS(vict) < POS_FIGHTING || !number(0, 3)))
	    do_offensive_skill(ch, fname(vict->player.name), 0, SKILL_PILEDRIVE);
	else {
	    hit(ch, vict, TYPE_UNDEFINED);
	}	    
	return;
    } 

    if (IS_DRAGON(ch) && !number(0, 3)) {
	switch (GET_CLASS(ch)) {
	case CLASS_GREEN:
	    if (!number(0, 2)) {
		call_magic(ch, vict, 0, SPELL_GAS_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
	    }
	    break;
	case CLASS_BLACK:
	    if (!number(0, 2)) {
		call_magic(ch, vict, 0,SPELL_ACID_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
	    }
	    break;
	case CLASS_BLUE:
	    if (!number(0, 2)) {
		call_magic(ch, vict, 0, 
			   SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
	    }
	    break;
	case CLASS_WHITE:
	case CLASS_SILVER:
	    if (!number(0, 2)) {
		call_magic(ch, vict, 0,SPELL_FROST_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
	    }
	    break;
	case CLASS_RED:
	    if (!number(0, 2)) {
		call_magic(ch, vict, 0,SPELL_FIRE_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
	    }
	    break;
	} 
	return;
    }

    hit(ch, vict, TYPE_UNDEFINED);

}
#define ELEMENTAL_LIKES_ROOM(ch, room)  \
(!IS_ELEMENTAL(ch) ||				\
 (GET_CLASS(ch) == CLASS_WATER &&			\
  (SECT_TYPE(room) == SECT_WATER_NOSWIM  ||		\
   SECT_TYPE(room) == SECT_WATER_SWIM ||		\
   SECT_TYPE(room) == SECT_UNDERWATER)) ||		\
 GET_CLASS(ch) != CLASS_WATER)

inline bool CHAR_LIKES_ROOM( struct char_data *ch, struct room_data *room ) {

    if ( ELEMENTAL_LIKES_ROOM(ch, room) &&

	 ( CHAR_WITHSTANDS_FIRE(ch) ||                    
	   !ROOM_FLAGGED(room, ROOM_FLAME_FILLED) ) &&
	 
	 ( CHAR_WITHSTANDS_COLD(ch) ||                     
	   !ROOM_FLAGGED(room, ROOM_ICE_COLD) ) &&
	 
	 (!IS_EVIL(ch) || !ROOM_FLAGGED(room, ROOM_HOLYOCEAN) ) &&
	 
	 ( can_travel_sector(ch, room->sector_type, 0) ) &&
	 
	 ( (!MOB2_FLAGGED(ch, MOB2_STAY_SECT) ||
	    ch->in_room->sector_type == room->sector_type ) &&
	   (!LIGHT_OK(ch) || IS_LIGHT(room) || CAN_SEE_IN_DARK(ch) || 
	    (GET_EQ(ch, WEAR_LIGHT) &&                          
	     GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0) ) ) ) ) {
	return true;
    }
   return false;
}
			    
/*
#define CHAR_LIKES_ROOM(ch, room) \
    (ELEMENTAL_LIKES_ROOM(ch, room) &&                \
     (CHAR_WITHSTANDS_FIRE(ch) ||                     \
      !ROOM_FLAGGED(room, ROOM_FLAME_FILLED)) &&      \
     (CHAR_WITHSTANDS_COLD(ch) ||                     \
      !ROOM_FLAGGED(room, ROOM_ICE_COLD)) &&          \
     (!IS_EVIL(ch) ||                                 \
      !ROOM_FLAGGED(room, ROOM_HOLYOCEAN)) &&         \
     (can_travel_sector(ch, room->sector_type, 0)) && \
     (!MOB2_FLAGGED(ch, MOB2_STAY_SECT) ||           \
      ch->in_room->sector_type == room->sector_type) && \
     (!LIGHT_OK(ch) || IS_LIGHT(room) || CAN_SEE_IN_DARK(ch) || \
      (GET_EQ(ch, WEAR_LIGHT) &&                          \
       GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0))))
*/		

#define CHECK_STATUS \
    if (found) {              \
        if (ch->in_room)        \
            next_ch = ch->next;   \
	continue;               \
     }

const int mug_eq[] = 
{
    WEAR_WIELD, WEAR_WIELD_2, WEAR_NECK_1, WEAR_NECK_2,  WEAR_BODY,
    WEAR_HEAD,  WEAR_ARMS,    WEAR_HANDS,  WEAR_WRIST_R, WEAR_WRIST_L,
    WEAR_HOLD,  WEAR_EYES,    WEAR_BACK,   WEAR_BELT,    WEAR_FACE,
    WEAR_SHIELD, WEAR_ABOUT
};

#define MUG_MAX 17
void 
mobile_activity(void)
{
    register struct char_data *ch, *next_ch, *vict;
    struct obj_data *obj, *best_obj, *i;
    struct affected_type *af_ptr = NULL;
    struct char_data *tmp_vict = NULL;
    int dir, found, max, k;
    byte ok_vict=0, ok_fvict=0;
    static unsigned int count = 0;
    struct mob_mugger_data *new_mug = NULL;
    struct room_data *room = NULL;

    extern int no_specials;

    for (ch = character_list, count++; ch; ch = next_ch) {
	next_ch = ch->next;
	found = FALSE;

	if (!ch->in_room ||
	    (IS_NPC(ch) && 
	     (ch->in_room->zone->idle_time >= ZONE_IDLE_TIME ||
	      ZONE_FLAGGED(ch->in_room->zone, ZONE_FROZEN))) ||
	    IS_AFFECTED_2(ch, AFF2_PETRIFIED))
	    continue;

	//
	// poison 2 tick
	//

	if (HAS_POISON_2(ch) &&
	    GET_LEVEL(ch) < LVL_AMBASSADOR && !(count % 2)) {
	    if (damage(ch, ch, dice(4, 3) + 
		       (affected_by_spell(ch, SPELL_METABOLISM) ? dice (4, 11) : 0), 
		       SPELL_POISON, -1))
		continue;
	}

	//
	// bleed
	//

	if (GET_HIT(ch) && 
	    CHAR_HAS_BLOOD(ch) &&
	    GET_HIT(ch) < ((GET_MAX_HIT(ch) >> 3) + 
			   number(0, GET_MAX_HIT(ch) >> 4)))
	    add_blood_to_room(ch->in_room, 1); 

	//
	// Zen of Motion effect
	//

	if (IS_NEUTRAL(ch) && affected_by_spell(ch, ZEN_MOTION))
	    GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), 
			       GET_MOVE(ch) + 
			       number(0, CHECK_SKILL(ch, ZEN_MOTION) >> 3));
    
	//
	// Deplete scuba tanks
	//
	
	if ((obj = ch->equipment[WEAR_FACE]) && 
	    GET_OBJ_TYPE(obj) == ITEM_SCUBA_MASK &&
	    !CAR_CLOSED(obj) &&
	    obj->aux_obj && GET_OBJ_TYPE(obj->aux_obj) == ITEM_SCUBA_TANK &&
	    (GET_OBJ_VAL(obj->aux_obj,1) > 0 || 
	     GET_OBJ_VAL(obj->aux_obj,0) < 0)) {
	    if (GET_OBJ_VAL(obj->aux_obj, 0) > 0) {
		GET_OBJ_VAL(obj->aux_obj, 1)--;
		if (!GET_OBJ_VAL(obj->aux_obj, 1))
		    act("A warning indicator reads: $p fully depleted.",
			FALSE, ch, obj->aux_obj, 0, TO_CHAR);
		else if (GET_OBJ_VAL(obj->aux_obj, 1) == 5)
		    act("A warning indicator reads: $p air level low.",
			FALSE, ch, obj->aux_obj, 0, TO_CHAR);
	    }
	}

	//
	// Check for mob spec
	//

	if (!no_specials && MOB_FLAGGED(ch, MOB_SPEC) && 
	    GET_MOB_WAIT(ch) <= 0 && !ch->desc && (count % 2)) {
	    if (ch->mob_specials.shared->func == NULL) {
		sprintf(buf, "%s (#%d): Attempting to call non-existing mob func",
			GET_NAME(ch), GET_MOB_VNUM(ch));
		slog(buf);
		REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
	    } else {
		if ((ch->mob_specials.shared->func) (ch, ch, 0, "", 0)) {
		    continue;		/* go to next char */
		}
	    }
	}

	//
	// nothing below this conditional affects FIGHTING characters
	//
	
	if (FIGHTING(ch) || GET_POS(ch) == POS_FIGHTING)
	    continue;

	//
	// meditate
	// 

	if (IS_NEUTRAL(ch) && GET_POS(ch) == POS_SITTING && IS_AFFECTED_2(ch, AFF2_MEDITATE)) {

	    perform_monk_meditate(ch);
	}

	//
	// Check if we've gotten knocked down.
	//

	if (IS_NPC(ch) && !MOB2_FLAGGED(ch, MOB2_MOUNT) &&
	    !AFF_FLAGGED(ch, AFF_SLEEP) &&
	    GET_MOB_WAIT(ch) < 30 &&
	    !AFF_FLAGGED(ch, AFF_SLEEP) &&
	    GET_POS(ch) >= POS_SLEEPING &&
	    (GET_DEFAULT_POS(ch) <= POS_STANDING ||
	     GET_POS(ch) < POS_STANDING) &&
	    (GET_POS(ch) < GET_DEFAULT_POS(ch)) && !number(0, 2)) {
	    if (GET_POS(ch) == POS_SLEEPING)
		act("$n wakes up.", TRUE, ch, 0, 0, TO_ROOM);
      
	    switch (GET_DEFAULT_POS(ch)) {
	    case POS_SITTING:
		act("$n sits up.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	    default:
		act("$n stands up.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	    }
	    continue;
	}

	//
	// nothing below this conditional affects characters who are asleep or in a wait state
	//
	
	if (!AWAKE(ch) || GET_MOB_WAIT(ch) > 0 || CHECK_WAIT(ch))
	    continue;

	//
	// barbs go BESERK (berserk)
	//

	if (GET_LEVEL(ch) < LVL_AMBASSADOR && AFF2_FLAGGED(ch, AFF2_BESERK) &&
	    !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
	
	    found = perform_barb_beserk(ch);
	}
	
	CHECK_STATUS;
      
	//
	// drunk effects
	//
	
	if (GET_COND(ch, DRUNK) > 8 && !number(0, 10)) {
	    found = FALSE;
	    act("$n burps loudly.", FALSE, ch, 0, 0, TO_ROOM);
	    send_to_char("You burp loudly.\r\n", ch);
	    found = TRUE;
	} else if (GET_COND(ch, DRUNK) > 4 && !number(0, 8)) {
	    found = FALSE;
	    act("$n hiccups.", FALSE, ch, 0, 0, TO_ROOM);
	    send_to_char("You hiccup.\r\n", ch);
	    found = TRUE;
	}
	
	//
	// nothing below this conditional affects PCs
	//

	if (!IS_MOB(ch) || ch->desc) 
	    continue;
	
	/** implicit awake && !fighting **/
	if (MOB2_FLAGGED(ch, MOB2_SCRIPT) && GET_IMPLANT(ch, WEAR_HOLD) && 
	    OBJ_TYPE(GET_IMPLANT(ch, WEAR_HOLD), ITEM_SCRIPT)) {
	    if (mob_read_script(ch))
		continue;
	}

	/** mobiles re-hiding **/
	if (!AFF_FLAGGED(ch, AFF_HIDE) && 
	    AFF_FLAGGED(MOB_SHARED(ch)->proto, AFF_HIDE))
	    SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
    
	/** mobiles reloading guns **/
	if (CHECK_SKILL(ch, SKILL_SHOOT) > number(30, 40)) {
	    for (obj = ch->carrying; obj; obj = obj->next_content) {
		if ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
		    (IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
		    (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))) {
		    mob_reload_gun(ch, obj);
		    break;
		}
	    }
      
	    if ((obj = GET_EQ(ch, WEAR_WIELD)) && 
		((IS_GUN(obj) && !GUN_LOADED(obj)) || 
		 (IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
		 (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
		mob_reload_gun(ch, obj);
	    if ((obj = GET_EQ(ch, WEAR_WIELD_2)) && 
		((IS_GUN(obj) && !GUN_LOADED(obj)) || 
		 (IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
		 (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
		mob_reload_gun(ch, obj);
	    if ((obj = GET_IMPLANT(ch, WEAR_WIELD)) && 
		((IS_GUN(obj) && !GUN_LOADED(obj)) || 
		 (IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
		 (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
		mob_reload_gun(ch, obj);
	    if ((obj = GET_IMPLANT(ch, WEAR_WIELD_2)) && 
		((IS_GUN(obj) && !GUN_LOADED(obj)) || 
		 (IS_GUN(obj) && (CLIP_COUNT(obj) < MAX_LOAD(obj))) ||
		 (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
		mob_reload_gun(ch, obj);
	}
      
	/* Mobiles looking at chars */
	if (!number(0, 20)) {
	    for (vict = ch->in_room->people;vict;vict = vict->next_in_room) {
		if (vict == ch)
		    continue;
		if (CAN_SEE(ch, vict) && !number(0, 40)) {
		    if ((IS_EVIL(ch) && IS_GOOD(vict)) || (IS_GOOD(ch) && IS_EVIL(ch))) {
			if (GET_LEVEL(ch) < (GET_LEVEL(vict) - 10)) {
			    act("$n looks warily at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			    act("$n looks warily at you.", FALSE, ch, 0, vict, TO_VICT);
			} else {
			    act("$n growls at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			    act("$n growls at you.", FALSE, ch, 0, vict, TO_VICT);
			}
		    } else if (GET_CLASS(ch) == CLASS_PREDATOR) {
			act("$n growls at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n growls at you.", FALSE, ch, 0, vict, TO_VICT);
		    } else if (IS_THIEF(ch)) {
			act("$n glances sidelong at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n glances sidelong at you.", FALSE, ch, 0, vict, TO_VICT);
		    } else if (((IS_MALE(ch) && IS_FEMALE(vict)) || 
				(IS_FEMALE(ch) && IS_MALE(vict))) && !number(0, 3)) {
			act("$n stares dreamily at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n stares dreamily at you.", FALSE, ch, 0, vict, TO_VICT);
		    } else if (!number(0, 1)) {
			act("$n looks at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n looks at you.", FALSE, ch, 0, vict, TO_VICT);
		    }
		    break;
		}
	    }
	}

	/* muggers */
	if (MOB2_FLAGGED(ch, MOB2_MUGGER)) {

	    if (!ch->mob_specials.mug) {

		if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) &&
		    !number(0, 3) &&
		    room_count(ch, ch->in_room) < number(GET_LEVEL(ch) >> 4,
							 GET_LEVEL(ch) >> 2)) {

		    for (vict = ch->in_room->people, tmp_vict = NULL, max = 0; vict; 
			 vict = vict->next_in_room) {
			if (IS_NPC(vict) && CAN_SEE(ch, vict) &&
			    (cityguard == vict->mob_specials.shared->func)) {
			    act("$n glances at $N.", TRUE, ch, 0, vict, TO_ROOM);
			    found = TRUE;
			    break;
			}
			if (vict == ch || !CAN_SEE(ch, vict) || !CAN_SEE(vict, ch) ||
			    FIGHTING(vict) || IS_NPC(vict) || !AWAKE(vict) ||
			    GET_LEVEL(vict) < 7 || GET_LEVEL(vict) > (GET_LEVEL(ch) + 5) ||
			    GET_LEVEL(vict) + 15 < GET_LEVEL(ch))
			    continue;
			if (!tmp_vict ||
			    GET_LEVEL(vict) < GET_LEVEL(tmp_vict) ||
			    GET_HIT(vict) < GET_HIT(tmp_vict) || number(0, 2))
			    tmp_vict = vict;
		    }
		    if (found || !tmp_vict)
			continue;

		    vict = tmp_vict;
		    act("$n examines $N.", TRUE, ch, 0, vict, TO_NOTVICT);
		    act("$n examines you.", TRUE, ch, 0, vict, TO_VICT);
	
		    for (k = 0; k < MUG_MAX; k++) {

			if ((obj = GET_EQ(vict, mug_eq[k])) &&
			    !IS_OBJ_STAT(obj, ITEM_NODROP) &&
			    !IS_OBJ_STAT2(obj, ITEM2_NOREMOVE) &&
			    CAN_SEE_OBJ(ch, obj) &&
			    !invalid_char_class(ch, obj) &&

			    obj->shared->proto &&
			    obj->short_description == 
			    obj->shared->proto->short_description) {
			    /* &&
			       (!(i = GET_EQ(vict, mug_eq[k])) ||
			       GET_OBJ_COST(obj) > (GET_OBJ_COST(i) << 1))) { */
			    sprintf(buf, "%s I see you are using %s.  I believe I could appreciate it much more than you.  Give it to me now.", GET_NAME(vict), 
				    obj->short_description);
			    do_say(ch, buf, 0, SCMD_SAY_TO);
			    CREATE(new_mug, struct mob_mugger_data, 1);
			    new_mug->idnum = GET_IDNUM(vict);
			    new_mug->vnum  = GET_OBJ_VNUM(obj);
			    new_mug->timer = 1;
			    ch->mob_specials.mug = new_mug;
			    found = TRUE;
			    break;
			}               /* found the good obj */
		    }                 /* loop thru eq */
		    if (found)
			continue;
		} /* (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) { */
	    }                     /*       if (!ch->mob_specials.mug) */
	    else if (!FIGHTING(ch) && !HUNTING(ch)) {

		for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
		    if (!IS_NPC(vict) && CAN_SEE(ch, vict) && 
			GET_IDNUM(vict) == ch->mob_specials.mug->idnum)
			break;

		if (!vict) {
		    for (vict = character_list; vict; vict = vict->next)
			if (!IS_NPC(vict) && CAN_SEE(ch, vict) && 
			    GET_IDNUM(vict) == ch->mob_specials.mug->idnum)
			    break;

		    if (!vict || 
			find_first_step(ch->in_room, vict->in_room, FALSE) < 0) {
			do_say(ch, "Curses, foiled again!", 0, 0);
			free(ch->mob_specials.mug);
			ch->mob_specials.mug = NULL;
			continue;
		    }
	  
		    sprintf(buf, "You're asking for it, %s!", GET_NAME(vict));
		    do_gen_comm(ch, buf, 0, SCMD_SHOUT);
		    HUNTING(ch) = vict;
		    continue;
		}

		/* vict is still in room */
	
		if (!(obj = real_object_proto(ch->mob_specials.mug->vnum))) {
		    slog("SYSERR: fatal error 1 in mugger code!");
		    continue;
		}
	
		for (i = ch->in_room->contents; i; i = i->next_content) {
		    if (GET_OBJ_VNUM(i) == GET_OBJ_VNUM(obj) && CAN_SEE_OBJ(ch, i) &&
			i->shared->proto && i->shared->proto->short_description ==
			obj->shared->proto->short_description) {
			act("$n snickers and picks up $p.", FALSE, ch, i, 0, TO_ROOM);
			obj_from_room(i);
			obj_to_char(i, ch);
			free(ch->mob_specials.mug);
			ch->mob_specials.mug = NULL;
			found = TRUE;
			break;
		    }
		}
		if (found)
		    continue;

		if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
		    if (ch->mob_specials.mug->timer <= 3) {
			if (ch->mob_specials.mug->timer == 1) {
			    sprintf(buf, "%s I'm warning you.  Give %s to me now, or face the consequences!", GET_NAME(vict), obj->short_description);
			    do_say(ch, buf, 0, SCMD_SAY_TO);
			}
			sprintf(buf, "You have %d seconds to comply.",
				ch->mob_specials.mug->timer == 1 ? 16 :
				ch->mob_specials.mug->timer == 2 ? 12 : 8);
			do_say(ch, buf, 0, 0);
			ch->mob_specials.mug->timer++;
			continue;
		    }
		    else if (ch->mob_specials.mug->timer == 4) {
			sprintf(buf, "%s You have 4 seconds to give %s to me, OR ELSE!", 
				GET_NAME(vict), obj->short_description);
			do_say(ch, buf, 0, SCMD_SAY_TO);
			ch->mob_specials.mug->timer = 3;
			continue;
		    }
		    else if (ch->mob_specials.mug->timer == 5) {

			sprintf(buf, "%s Okay, you asked for it.  Now I'm taking %s!!", GET_NAME(vict), obj->short_description);
			do_say(ch, buf, 0, SCMD_SAY_TO);
			ch->mob_specials.mug->timer = 4;
			best_attack(ch, vict);
			continue;
		    }
		} /*	if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {*/
	    }  /*      else if (!FIGHTING(ch) && !HUNTING(ch)) { */
	}
    
	/* Scavenger (picking up objects) */

	if (MOB_FLAGGED(ch, MOB_SCAVENGER)) {
	    if (ch->in_room->contents && !number(0, 11)) {
		max = 1;
		best_obj = NULL;
		for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		    if (CAN_SEE_OBJ(ch, obj) &&
			// don't pick up sigil-ized objs if we know better
			(!GET_OBJ_SIGIL_IDNUM(obj) ||
			 (!AFF_FLAGGED(ch, AFF_DETECT_MAGIC) && !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))) &&
			CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
			best_obj = obj;
			max = GET_OBJ_COST(obj);
		    }
		if (best_obj != NULL) {
		    strcpy(buf, fname(best_obj->name));
		    do_get(ch, buf, 0, 0);
		}
	    }
	}
	/* Drink from fountains */
	if (GET_RACE(ch) != RACE_UNDEAD && GET_RACE(ch) != RACE_DRAGON &&
	    GET_RACE(ch) != RACE_GOLEM && GET_RACE(ch) != RACE_ELEMENTAL) {
	    if (ch->in_room->contents && !number(0, 80)) {
		for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		    if (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN &&
			GET_OBJ_VAL(obj, 1) > 0)
			break;
		if (obj && GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN && 
		    GET_OBJ_VAL(obj, 1) > 0) {
		    act("$n drinks from $p.",TRUE, ch, obj, 0, TO_ROOM);
		    act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
		    continue;
		}
		continue;
	    }
	}
	/* Animals devouring corpses and food */
	if (IS_CLASS(ch, CLASS_PREDATOR)) {
	    if (ch->in_room->contents && 
		(!number(0, 3) || GET_MOB_VNUM(ch) == 24800)) {
		found = FALSE;
		for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
		    if (GET_OBJ_TYPE(obj) == ITEM_FOOD && !GET_OBJ_VAL(obj, 3)) {
			found = TRUE;
			act("$n devours $p, growling and drooling all over.", 
			    FALSE, ch, obj, 0, TO_ROOM);
			extract_obj(obj);
			break;
		    } else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && 
			       GET_OBJ_VAL(obj, 3)) {
			found = TRUE;
			act("$n devours $p, growling and drooling all over.", 
			    FALSE, ch, obj, 0, TO_ROOM);
			for (i = obj->contains;i;i=best_obj) {
			    best_obj = i->next_content;
			    if (IS_IMPLANT(i)) {
				SET_BIT(GET_OBJ_WEAR(i), ITEM_WEAR_TAKE);
				if (GET_OBJ_DAM(i) > 0)
				    GET_OBJ_DAM(i) >>= 1;
			    }
			    obj_from_obj(i);
			    obj_to_room(i, ch->in_room);

			}
			extract_obj(obj);
			break;
		    }
		}
		if (found)
		    continue;
	    }
	}
	/* Wearing Objects */
	if (!MOB2_FLAGGED(ch, MOB2_WONT_WEAR) && !IS_ANIMAL(ch) && 
	    !FIGHTING(ch) &&
	    !IS_DRAGON(ch) && !IS_ELEMENTAL(ch)) {
	    if (ch->carrying && !number(0, 3)) {
		for (obj = ch->carrying; obj; obj = best_obj) {
		    best_obj = obj->next_content;
		    if (ANTI_ALIGN_OBJ(ch, obj) && number(6, 21) < GET_INT(ch)) {
			act("$n junks $p.",TRUE,ch,obj,0,TO_ROOM);
			extract_obj(obj);
			break;
		    }
		    if (CAN_SEE_OBJ(ch, obj) && 
			!IS_IMPLANT(obj) &&
			(GET_OBJ_TYPE(obj) == ITEM_WEAPON ||
			 IS_ENERGY_GUN(obj) || IS_GUN(obj)) &&
			!invalid_char_class(ch, obj) &&
			(!GET_EQ(ch, WEAR_WIELD) ||
			 !IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_NOREMOVE))) {
			if (GET_EQ(ch, WEAR_WIELD) &&
			    (obj->getWeight() <=
			     str_app[STRENGTH_APPLY_INDEX(ch)].wield_w) &&
			    GET_OBJ_COST(obj) > GET_OBJ_COST(GET_EQ(ch, WEAR_WIELD))) {
			    strcpy(buf, fname(obj->name));
			    do_remove(ch, buf, 0, 0);
			}
			if (!GET_EQ(ch, WEAR_WIELD)) {
			    do_wield(ch, OBJN(obj, ch), 0, 0);
			    if (IS_GUN(obj) && GET_MOB_VNUM(ch) == 1516)
				do_say(ch, "Let's Rock.", 0, 0);
			}
		    }
		}
		do_wear(ch, "all", 0, 0);
		continue;
	    }      
	}
	/* Looter */

	if (MOB2_FLAGGED(ch, MOB2_LOOTER)) {
	    if (ch->in_room->contents && !number(0, 1)) {
		for (i = ch->in_room->contents; i; i = i->next_content) {
		    if ((GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3))) {
			if (!i->contains) continue;
			for (obj = i->contains; obj; obj = obj->next_content) {
			    if (!(obj->in_obj))
				continue;
	
			    // skip sigil-ized items, simplest way to deal with it
			    if (GET_OBJ_SIGIL_IDNUM(obj))
				continue;
 
			    if (CAN_GET_OBJ(ch, obj)) {
				obj_from_obj(obj);
				obj_to_char(obj, ch); 
				act("$n gets $p from $P.", FALSE, ch, obj, i, TO_ROOM);
				if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
				    if (GET_OBJ_VAL(obj, 1)) // credits
					GET_CASH(ch) += GET_OBJ_VAL(obj, 0);
				    else
					GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
				    extract_obj(obj);
				}
			    }
			}
		    } 
		}
	    }
	}
	/* Helper Mobs */

	if (MOB_FLAGGED(ch, MOB_HELPER) && !number(0, 1)) {
	    found = FALSE;
	    if (IS_AFFECTED(ch, AFF_CHARM))
		continue;
	    for (vict=ch->in_room->people;vict&&!found; vict = vict->next_in_room) {
		if (ch != vict && FIGHTING(vict) && ch != FIGHTING(vict) &&
		    CAN_SEE(ch, vict)) {
		    if (!IS_MOB(vict) || (MOB2_FLAGGED(ch, MOB2_ATK_MOBS) &&
					  GET_MOB_LEADER(ch) != GET_MOB_VNUM(vict) &&
					  GET_MOB_LEADER(vict) != GET_MOB_VNUM(ch) &&
					  (!MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) ||
					   GET_RACE(ch) != GET_RACE(vict))))
			ok_vict = 1;
		    else 
			ok_vict = 0;
		    if (!IS_MOB(FIGHTING(vict)) || 
			(MOB2_FLAGGED(ch, MOB2_ATK_MOBS) &&
			 GET_MOB_LEADER(ch) != GET_MOB_VNUM(FIGHTING(vict)) &&
			 GET_MOB_LEADER(FIGHTING(vict)) != GET_MOB_VNUM(ch) &&
			 (!MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) ||
			  GET_RACE(ch) != GET_RACE(FIGHTING(vict)))))
			ok_fvict = 1;
		    else 
			ok_fvict = 0;

		    if (!IS_NPC(vict) && GET_LEVEL(vict) < 5 && 
			IS_NPC(FIGHTING(vict)) && ok_fvict)
			found = helper_assist(ch, FIGHTING(vict), vict);
		    else if (!IS_NPC(FIGHTING(vict)) && GET_LEVEL(FIGHTING(vict)) < 5 && 
			     IS_NPC(vict) && ok_vict)
			found = helper_assist(ch, vict, FIGHTING(vict));
	  
		    else if (GET_LEVEL(vict) > (GET_LEVEL(FIGHTING(vict))*1.5)) { 
			if (ok_vict) {
			    if (helper_assist(ch, vict, FIGHTING(vict)))
				found = TRUE;
			} else if (ok_fvict) {
			    if (helper_assist(ch, FIGHTING(vict), vict))
				found = TRUE;
			} 
		    } else if (IS_GOOD(ch)) {
			if (IS_EVIL(vict)) {
			    if (GET_ALIGNMENT(vict) > GET_ALIGNMENT(FIGHTING(vict)) &&
				ok_fvict) {
				if (helper_assist(ch, FIGHTING(vict), vict))
				    found = TRUE;
			    } else if (ok_vict) {
				if (helper_assist(ch, vict, FIGHTING(vict)))
				    found = TRUE;
			    }
			} else if (IS_EVIL(FIGHTING(vict))) {
			    if (GET_ALIGNMENT(vict) < GET_ALIGNMENT(FIGHTING(vict)) && 
				ok_vict) {
				if (helper_assist(ch, vict, FIGHTING(vict)))
				    found = TRUE;
			    } else if (ok_fvict) {
				if (helper_assist(ch, FIGHTING(vict), vict))
				    found = TRUE;
			    }
			} else if (GET_LEVEL(vict) > GET_LEVEL(FIGHTING(vict)) && 
				   ok_vict) {
			    if (helper_assist(ch, vict, FIGHTING(vict)))
				found = TRUE;
			} else if (ok_fvict) {
			    if (helper_assist(ch, FIGHTING(vict), vict))
				found = TRUE;
			}
		    } else {
			if (GET_RACE(ch) == GET_RACE(vict) && ok_fvict) {
			    if (helper_assist(ch, FIGHTING(vict), vict))
				found = TRUE;
			} else if (GET_RACE(ch) == GET_RACE(FIGHTING(vict)) && ok_vict) {
			    if (helper_assist(ch, vict, FIGHTING(vict)))
				found = TRUE;
			} else if (GET_LEVEL(vict)>GET_LEVEL(FIGHTING(vict)) && ok_vict) {
			    if (helper_assist(ch, vict, FIGHTING(vict)))
				found = TRUE;
			} else if (ok_fvict) {
			    if (helper_assist(ch, FIGHTING(vict), vict))
				found = TRUE;
			}
		    }
		}
	    }
	}

	CHECK_STATUS;

	/*Racially aggressive Mobs */

	if (IS_RACIALLY_AGGRO(ch) && 
	    !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !number(0, 3)) {
	    found = FALSE;
	    for (vict=ch->in_room->people;vict&&!found; vict = vict->next_in_room) {
		if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS))
		    || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
		    IS_AFFECTED_2(vict, AFF2_PETRIFIED))
		    continue;
		if (GET_MORALE(ch) + GET_LEVEL(ch) < 
		    number(GET_LEVEL(vict),  (GET_LEVEL(vict) << 2) + 
			   ((GET_HIT(vict) * GET_LEVEL(vict)) / GET_MAX_HIT(vict))) &&
		    AWAKE(vict))
		    continue;
		else if (RACIAL_ATTACK(ch, vict) &&
			 (!IS_NPC(vict) || MOB2_FLAGGED(ch, MOB2_ATK_MOBS))) {
		    best_attack(ch, vict);
		    found = TRUE;
		}
	    }
	}

	CHECK_STATUS;

	/* Aggressive Mobs */

	if ((MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
	     MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN)) &&
	    !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
	    found = FALSE;
	    for (vict = ch->in_room->people; vict && !found; 
		 vict = vict->next_in_room) {
		if (vict == ch || (vict->next_in_room && !number(0, 3)) ||
		    (!IS_NPC(vict) && !vict->desc)) {
		    continue;
		}
		if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS))
		    || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
		    IS_AFFECTED_2(vict, AFF2_PETRIFIED)) {
		    continue;
		}
		if (AWAKE(vict) &&
		    GET_MORALE(ch) < number(35 + (GET_LEVEL(vict) >> 1), 
					    (GET_LEVEL(vict) << 1) + 35))
		    continue;

		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && ch->master == vict &&
		    (GET_CHA(vict) + GET_INT(vict)) > GET_INT(ch) + number(0, 20))
		    continue;

		if (MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) && 
		    GET_RACE(ch) == GET_RACE(vict))
		    continue;
	
		if (!MOB_FLAGGED(ch, MOB_AGGRESSIVE))
		    if ((IS_EVIL(vict) && !MOB_FLAGGED(ch, MOB_AGGR_EVIL)) ||
			(IS_GOOD(vict) && !MOB_FLAGGED(ch, MOB_AGGR_GOOD)) ||
			(IS_NEUTRAL(vict) && !MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL)))
			continue;

		best_attack(ch, vict);
		found = 1;
		break;
	    }

	    CHECK_STATUS;

	    /** scan surrounding rooms **/
	    if (!found && !HUNTING(ch) && GET_POS(ch) > POS_FIGHTING &&
		!MOB_FLAGGED(ch, MOB_SENTINEL) && 
		(GET_LEVEL(ch) + GET_MORALE(ch) > number(50, 170) ||
		 GET_MOB_VNUM(ch) == 24800)) { /* tarrasque 24800 */
		found = 0;

		for (dir = 0; dir < NUM_DIRS && !found; dir++) {
		    if (CAN_GO(ch, dir) && !ROOM_FLAGGED(EXIT(ch, dir)->to_room,
							 ROOM_DEATH | ROOM_NOMOB |
							 ROOM_PEACEFUL) &&
			EXIT(ch, dir)->to_room != ch->in_room &&
			CHAR_LIKES_ROOM(ch, EXIT(ch, dir)->to_room) &&
			EXIT(ch, dir)->to_room->people &&
			CAN_SEE(ch, EXIT(ch, dir)->to_room->people) &&
			room_count(ch, EXIT(ch, dir)->to_room) <
			EXIT(ch, dir)->to_room->max_occupancy)
			break;
		}
	
		if (dir < NUM_DIRS) {
		    for (vict = EXIT(ch, dir)->to_room->people; vict; 
			 vict = vict->next_in_room) {
			if (CAN_SEE(ch, vict) && !PRF_FLAGGED(vict, PRF_NOHASSLE) &&
			    (!AFF_FLAGGED(vict, AFF_SNEAK) || 
			     AFF_FLAGGED(vict, AFF_GLOWLIGHT) ||
			     AFF_FLAGGED(vict, AFF2_FLUORESCENT | 
					 AFF2_DIVINE_ILLUMINATION)) &&
			    (MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
			     (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
			     (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)) ||
			     (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict))) &&
			    (!MOB2_FLAGGED(ch, MOB2_NOAGGRO_RACE) ||
			     GET_RACE(ch) != GET_RACE(vict)) &&
			    (!IS_NPC(vict) || MOB2_FLAGGED(ch, MOB2_ATK_MOBS))) {
			    found = 1;
			    break;
			}
		    }
		    if (found) {
			if (IS_PSIONIC(ch) && GET_LEVEL(ch) > 23 &&
			    GET_MOVE(ch) > 100 && GET_MANA(vict) > 100)
			    do_psidrain(ch, fname(vict->player.name), 0, 0);
			else
			    perform_move(ch, dir, MOVE_NORM, 1);
		    }
		}
	    }
	}

	CHECK_STATUS;

	/* Mob Memory */

	if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch) && 
	    !AFF_FLAGGED(ch, AFF_CHARM)) {
	    found = FALSE;
	    for (vict=ch->in_room->people;vict&&!found;vict = vict->next_in_room) {
		if ((IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS)) || 
		    !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
		    ((af_ptr = affected_by_spell(vict, SKILL_DISGUISE)) &&
		     !CAN_DETECT_DISGUISE(ch, vict, af_ptr->duration)))
		    continue;
		if (char_in_memory(vict, ch)) {
	  
		    found = TRUE;
	  
		    if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
			if (AWAKE(vict) && !IS_UNDEAD(ch) && !IS_DRAGON(ch) &&
			    !IS_DEVIL(ch) && GET_HIT(ch) > GET_HIT(vict) &&
			    ((GET_LEVEL(vict) + ((50 * GET_HIT(vict)) / 
						 GET_MAX_HIT(vict))) >
			     GET_LEVEL(ch) + (GET_MORALE(ch) >> 1) + number(1, 100))) {
			    if (!IS_ANIMAL(ch) && !IS_SLIME(ch) && !IS_PUDDING(ch)) {
				if (!number(0, 3))
				    do_say(ch, "Oh, shit!", 0, 0);
				else if (!number(0, 2))
				    act("$n screams in terror!", FALSE, ch, 0, 0, TO_ROOM);
				else if (!number(0, 1))
				    do_gen_comm(ch, "Run away!  Run away!", 0, SCMD_SHOUT);
			    }
			    do_flee(ch, "", 0, 0);
			    break;
			}
			if (IS_ANIMAL(ch)) {
			    act("$n snarls and attacks $N!!",FALSE,ch,0,vict,TO_NOTVICT);
			    act("$n snarls and attacks you!!",FALSE,ch,0,vict,TO_VICT);
			} else {
			    if (!number(0, 1)) 
				act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
			    else
				act("'Hey!  You're the punk I've been looking for!!!', exclaims $n.",	FALSE, ch, 0, 0, TO_ROOM);
			}
			best_attack(ch, vict);
		    } else if (GET_POS(ch) != POS_FIGHTING) {
			switch (number(0, 20)) {
			case 0:
			    if ((!IS_ANIMAL(ch) && !IS_DEVIL(ch) && !IS_DEMON(ch) && 
				 !IS_UNDEAD(ch) && !IS_DRAGON(ch)) || !number(0, 3))
				act("'You wimp $N!  Your ass is grass.', exclaims $n.",
				    FALSE, ch, 0, vict, TO_ROOM);
			    else {
				act("$n growls at you menacingly.",
				    TRUE, ch, 0, vict, TO_VICT);
				act("$n growls menacingly at $N.", 
				    TRUE, ch, 0, vict, TO_NOTVICT);
			    }
			    break;
			case 1:
			    act("$n sighs loudly.", FALSE, ch, 0, 0, TO_ROOM);
			    break;
			case 2:
			    act("$n gazes at you coldly.", TRUE, ch, 0, vict, TO_VICT);
			    act("$n gazes coldly at $N.", TRUE, ch, 0, vict, TO_NOTVICT);
			    break;
			case 3:
			    act("$n prepares for battle.", TRUE, ch, 0, 0, TO_ROOM);
			    break;
			case 4:
			    perform_tell(ch, vict, "Let's rumble.");
			    break;
			case 5: // look for help
			    break;
			}
		    }
		}
	    }
	}

	CHECK_STATUS;

	/* Mob Movement -- Lair */
	if (GET_MOB_LAIR(ch) > 0 && ch->in_room->number != GET_MOB_LAIR(ch) &&
	    !HUNTING(ch) &&
	    (room = real_room(GET_MOB_LAIR(ch))) && 
	    ((dir = find_first_step(ch->in_room, room, FALSE)) >= 0) &&
	    MOB_CAN_GO(ch, dir) && 
	    !ROOM_FLAGGED(ch->in_room->dir_option[dir]->to_room, 
			  ROOM_NOMOB | ROOM_DEATH)) {
	    smart_mobile_move(ch,dir);
	    continue;
	}

	// mob movement -- follow the leader

	if (GET_MOB_LEADER(ch) > 0 && ch->master && 
	    ch->in_room != ch->master->in_room) {
	    if (smart_mobile_move(ch, find_first_step(ch->in_room, ch->master->in_room, FALSE)))
		continue;
	}

	/* Mob Movement */
	if ( !MOB_FLAGGED(ch, MOB_SENTINEL) && GET_POS(ch) >= POS_STANDING ) {
	    
	    int door = number( 0, 
			       // tarrasque moves more.  maybe a flag is order?
			       ( GET_MOB_VNUM(ch) == 24800 ? NUM_OF_DIRS-1 :20 ) );

	    if ( ( door < NUM_OF_DIRS ) &&
		 
		 ( MOB_CAN_GO(ch, door) ) &&
		 
		 ( rev_dir[door] !=  ch->mob_specials.last_direction || !number(0, 1) ) &&
		 
		 ( EXIT(ch, door)->to_room != ch->in_room ) &&
		 ( !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
		   !IS_SET(EXIT(ch, door)->exit_info, EX_NOMOB) ) &&
		 ( CHAR_LIKES_ROOM(ch, EXIT(ch, door)->to_room) ) &&
		 ( !MOB2_FLAGGED(ch, MOB2_STAY_SECT) ||
		   ( EXIT(ch, door)->to_room->sector_type == ch->in_room->sector_type ) ) &&
		 (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
		  (EXIT(ch, door)->to_room->zone == ch->in_room->zone) ) ) {
		if (perform_move(ch, door, MOVE_NORM, 1))
		    continue;
	    }
	}

	if (GET_CLASS(ch) == CLASS_THIEF && !number(0, 1)) {

	    if (thief(ch, ch, 0, "", 0))
		continue;

	} else if (IS_CLERIC(ch)) {
	    if (GET_HIT(ch) < GET_MAX_HIT(ch)*0.80) {
		if (GET_LEVEL(ch) > 23) 
		    cast_spell(ch, ch, 0, SPELL_HEAL);
		else if (GET_LEVEL(ch) > 11)
		    cast_spell(ch, ch, 0, SPELL_CURE_CRITIC);
		else 
		    cast_spell(ch, ch, 0, SPELL_CURE_LIGHT);
	    } else if (IS_DARK(ch->in_room) && 
		       !CAN_SEE_IN_DARK(ch) && GET_LEVEL(ch) > 8) {
		cast_spell(ch, ch, 0, SPELL_DIVINE_ILLUMINATION);
	    } else if ((affected_by_spell(ch, SPELL_BLINDNESS) ||
			affected_by_spell(ch, SKILL_GOUGE)) &&
		       GET_LEVEL(ch) > 6) {
		cast_spell(ch, ch, 0, SPELL_CURE_BLIND);
	    } else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 7) {
		cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
	    } else if (IS_AFFECTED(ch, AFF_CURSE) && GET_LEVEL(ch) > 26) {
		cast_spell(ch, ch, 0, SPELL_REMOVE_CURSE);
	    } else if (GET_MANA(ch) > (GET_MAX_MANA(ch) * 0.75) && 
		       GET_LEVEL(ch) >= 28 && !AFF_FLAGGED(ch, AFF_SANCTUARY))
		cast_spell(ch, ch, 0, SPELL_SANCTUARY);
	} else if (IS_KNIGHT(ch)) {
	    if (GET_HIT(ch) < GET_MAX_HIT(ch)*0.80) {
		if (GET_LEVEL(ch) > 27 && number(0, 1)) 
		    cast_spell(ch, ch, 0, SPELL_HEAL);
		else if (GET_LEVEL(ch) > 13 && number(0, 1))
		    cast_spell(ch, ch, 0, SPELL_CURE_CRITIC);
		else if (number(0, 1))
		    cast_spell(ch, ch, 0, SPELL_CURE_LIGHT);
		else 
		    do_holytouch(ch, "self", 0, 0);
	    } else if (IS_DARK(ch->in_room) && 
		       !CAN_SEE_IN_DARK(ch) && GET_LEVEL(ch) > 6) {
		cast_spell(ch, ch, 0, SPELL_DIVINE_ILLUMINATION);
	    } else if ((affected_by_spell(ch, SPELL_BLINDNESS) ||
			affected_by_spell(ch, SKILL_GOUGE)) &&
		       GET_LEVEL(ch) > 20) {
		cast_spell(ch, ch, 0, SPELL_CURE_BLIND);
	    } else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 18) {
		cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
	    } else if (IS_AFFECTED(ch, AFF_CURSE) && GET_LEVEL(ch) > 30) {
		cast_spell(ch, ch, 0, SPELL_REMOVE_CURSE);
	    }
	} else if (IS_RANGER(ch)) {
	    if (GET_HIT(ch) < GET_MAX_HIT(ch)*0.80) {
		if (GET_LEVEL(ch) > 39)
		    do_medic(ch, "self", 0, 0);
		else if (GET_LEVEL(ch) > 18)
		    do_firstaid(ch, "self", 0, 0);
		else if (GET_LEVEL(ch) > 9)
		    do_bandage(ch, "self", 0, 0);
	    } else if (IS_AFFECTED(ch, AFF_POISON) && GET_LEVEL(ch) > 11) {
		cast_spell(ch, ch, 0, SPELL_REMOVE_POISON);
	    }
	} 

	if (IS_MAGE(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
	    if (IS_DARK(ch->in_room) && 
		!CAN_SEE_IN_DARK(ch) && GET_LEVEL(ch) > 6)
		cast_spell(ch, ch, 0, SPELL_INFRAVISION);
	}

	if (IS_PSIONIC(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)) {
	    if (IS_DARK(ch->in_room) && 
		!CAN_SEE_IN_DARK(ch) && GET_LEVEL(ch) >= 6)
		cast_spell(ch, ch, 0, SPELL_INFRAVISION);
	    else if (GET_HIT(ch) < GET_MAX_HIT(ch)*0.80 && GET_LEVEL(ch) >= 10) {
		if (GET_LEVEL(ch) >= 34) 
		    cast_spell(ch, ch, 0, SPELL_CELL_REGEN);
		else 
		    cast_spell(ch, ch, 0, SPELL_WOUND_CLOSURE);
	    }
	    else if (GET_LEVEL(ch) >= 42 && !AFF_FLAGGED(ch, AFF_NOPAIN))
		cast_spell(ch, ch, 0, SPELL_NOPAIN);
	    else if (GET_LEVEL(ch) >= 21 &&
		     (ch->in_room->sector_type == SECT_UNDERWATER   ||
		      ch->in_room->sector_type == SECT_WATER_NOSWIM ||
		      ch->in_room->sector_type == SECT_PITCH_SUB) &&
		     !can_travel_sector(ch, ch->in_room->sector_type, 0) &&
		     !AFF3_FLAGGED(ch, AFF3_NOBREATHE))
		cast_spell(ch, ch, 0, SPELL_BREATHING_STASIS);
	    else if (GET_LEVEL(ch) >= 42 && 
		     !affected_by_spell(ch, SPELL_DERMAL_HARDENING))
		cast_spell(ch, ch, 0, SPELL_DERMAL_HARDENING);
	    else if (GET_LEVEL(ch) >= 30 &&
		     !AFF3_FLAGGED(ch, AFF3_PSISHIELD) &&
		     GET_MANA(ch) > mag_manacost(ch, SPELL_PSISHIELD))
		cast_spell(ch, ch, 0, SPELL_PSISHIELD);
	    else if (GET_LEVEL(ch) >= 20 &&
		     GET_MANA(ch) > mag_manacost(ch, SPELL_PSYCHIC_RESISTANCE) &&
		     !affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
		cast_spell(ch, ch, 0, SPELL_PSYCHIC_RESISTANCE);
	    else if (GET_LEVEL(ch) >= 15 &&	
		     GET_MANA(ch) > mag_manacost(ch, SPELL_POWER) &&
		     !affected_by_spell(ch, SPELL_POWER))
		cast_spell(ch, ch, 0, SPELL_POWER);
	    else if (GET_LEVEL(ch) >= 15 &&	
		     GET_MANA(ch) > mag_manacost(ch, SPELL_CONFIDENCE) &&
		     !AFF_FLAGGED(ch, AFF_CONFIDENCE))
		cast_spell(ch, ch, 0, SPELL_CONFIDENCE);	
	}

	if (GET_CLASS(ch) == CLASS_BARB || GET_CLASS(ch) == CLASS_HILL) {
	    if (GET_POS(ch) != POS_FIGHTING) {
		if (!number(0, 60))
		    act("$n grunts and scratches $s ear.", FALSE, ch, 0, 0, TO_ROOM);
		else if (!number(0, 80))
		    act("$n drools all over $mself.", FALSE, ch, 0, 0, TO_ROOM);
		else if (!number(0, 60))
		    act("$n belches loudly.", FALSE, ch, 0, 0, TO_ROOM);
		else if (!number(0, 60))
		    act("$n swats at an annoying gnat.", FALSE, ch, 0, 0, TO_ROOM);
		else if (!number(0, 90)) {
		    if (GET_SEX(ch) == SEX_MALE)
			act("$n scratches $s nuts and grins.", FALSE, ch, 0, 0, TO_ROOM);
		    else if (GET_SEX(ch) == SEX_FEMALE)
			act("$n belches loudly and grins.", FALSE, ch, 0, 0, TO_ROOM);
		}
		continue;
	    }
	}

	if (GET_RACE(ch) == RACE_ELEMENTAL) {
	    int sect;
	    sect = ch->in_room->sector_type;
      
	    if (((GET_MOB_VNUM(ch) >= 1280 &&
		  GET_MOB_VNUM(ch) <= 1283) ||
		 GET_MOB_VNUM(ch) == 5318) && !ch->master)
		k = 1;
	    else
		k = 0;
	    found = 0;

	    switch (GET_CLASS(ch)) {
	    case CLASS_EARTH:
		if (k || sect == SECT_WATER_SWIM ||
		    sect == SECT_WATER_NOSWIM ||
		    ch->in_room->isOpenAir() ||
		    sect == SECT_UNDERWATER) {
		    found = 1;
		    act("$n dissolves, and returns to $s home plane!", 
			TRUE, ch, 0, 0, TO_ROOM);
		    extract_char(ch, FALSE);
		}
		break;
	    case CLASS_FIRE:
		if ((k || sect == SECT_WATER_SWIM ||
		     sect == SECT_WATER_NOSWIM ||
		     ch->in_room->isOpenAir() ||
		     sect == SECT_UNDERWATER ||
		     (OUTSIDE(ch) && 
		      ch->in_room->zone->weather->sky == SKY_RAINING)) &&
		    !ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)) {
		    found = 1;
		    act("$n dissipates, and returns to $s home plane!", 
			TRUE, ch, 0, 0, TO_ROOM);
		    extract_char(ch, FALSE);
		}
		break;
	    case CLASS_WATER:
		if (k || 
		    (sect != SECT_WATER_SWIM &&
		     sect != SECT_WATER_NOSWIM &&
		     sect != SECT_UNDERWATER &&
		     ch->in_room->zone->weather->sky != SKY_RAINING)) {
		    found = 1;
		    act("$n dissipates, and returns to $s home plane!", 
			TRUE, ch, 0, 0, TO_ROOM);
		    extract_char(ch, FALSE);
		}
		break;
	    case CLASS_AIR:
		if (k || sect == SECT_UNDERWATER || sect == SECT_PITCH_SUB) {
		    found = 1;
		    act("$n dissipates, and returns to $s home plane!", 
			TRUE, ch, 0, 0, TO_ROOM);
		    extract_char(ch, FALSE);
		}
		break;
	    default:
		if (k) {
		    found = 1;
		    act("$n disappears.",TRUE,ch,0,0,TO_ROOM);
		    extract_char(ch, FALSE);
		}
	    } 
	    if (found)
		continue;
	} 
	if (GET_RACE(ch) == RACE_ANIMAL && !number(0, 2)) {
	    if (GET_CLASS(ch) == CLASS_BIRD && IS_AFFECTED(ch, AFF_INFLIGHT) &&
		!ch->in_room->isOpenAir() ) {
		if (GET_POS(ch) == POS_FLYING && !number(0,4)) {
		    act("$n flutters to the ground.", TRUE, ch, 0, 0, TO_ROOM);
		    GET_POS(ch) = POS_STANDING;
		} else if (GET_POS(ch) == POS_STANDING) {
		    act("$n flaps $s wings and takes flight.", TRUE, ch, 0, 0, TO_ROOM);
		    GET_POS(ch) = POS_FLYING;
		}
		continue;
	    }
	} 
	if (IS_AFFECTED(ch, AFF_INFLIGHT) && !number(0, 8)) {
	    if ( !ch->in_room->isOpenAir() ) {
		if (GET_POS(ch) == POS_FLYING && !number(0,10)) {
		    GET_POS(ch) = POS_STANDING;
		    if (!can_travel_sector(ch, ch->in_room->sector_type, 1)) {
			GET_POS(ch) = POS_FLYING;
			continue;
		    } else if (FLOW_TYPE(ch->in_room) != F_TYPE_SINKING_SWAMP)
			act("$n settles to the ground.", TRUE, ch, 0, 0, TO_ROOM);

		} else if (GET_POS(ch) == POS_STANDING && !number(0, 3)) {
		    act("$n begins to hover in midair.", TRUE, ch, 0, 0, TO_ROOM);
		    GET_POS(ch) = POS_FLYING;
		}
		continue;
	    }
	}
	/* Add new mobile actions here */
  				/* end for() */


    }
}

#define SAME_ALIGN(a, b) ( ( IS_GOOD(a) && IS_GOOD(b) ) || ( IS_EVIL(a) && IS_EVIL(b) ) )

struct char_data * choose_opponent(struct char_data *ch)
{
    
    struct char_data *vict = NULL;
    struct char_data *best_vict = NULL;

    // first look for someone who is fighting us
    for ( vict = ch->in_room->people; vict; vict = vict->next_in_room ) {

	// ignore bystanders
	if ( ch != FIGHTING(vict) )
	    continue;

	// look for opposite/neutral aligns only
	if ( SAME_ALIGN(ch, vict) )
	    continue;

	// pick the weakest victim in hopes of killing one
	if ( !best_vict || GET_HIT(vict) < GET_HIT(best_vict) ) {
	    best_vict = vict;
	    continue;
	}

	// if victim's AC is worse, prefer to attack him
	if ( !best_vict || GET_AC(vict) > GET_AC(best_vict) ) {
	    best_vict = vict;
	    continue;
	}
    }
	
    // default to fighting opponent
    if ( !best_vict )
	best_vict = FIGHTING(ch);

    return (best_vict);
}

void 
mobile_battle_activity(struct char_data *ch)
{
    register struct char_data *vict = NULL, *new_mob = NULL,
	*count_char = NULL, *next_vict = NULL;
    int num = 0, prob = 0, dam = 0;
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD), *gun = NULL;

    char buf[MAX_STRING_LENGTH];
    ACMD(do_disarm);
    ACMD(do_feign);

    if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
	return;

    if (GET_MOB_VNUM(ch) == 24800) { /* tarrasque */
	tarrasque_fight(ch);
	return;
    }

    if (GET_HIT(ch) < GET_MAX_HIT(ch) >> 7 && !IS_ANIMAL(ch) &&
	!number(0, 30)) {
	sprintf(buf, "%s you bastard!", PERS(FIGHTING(ch), ch));
	do_say(ch, buf, 0, 0);
	return;
    }
 
    if (!IS_ANIMAL(ch) && 
	GET_HIT(FIGHTING(ch)) < GET_MAX_HIT(FIGHTING(ch)) >> 7 &&
	GET_HIT(ch) > GET_MAX_HIT(ch) >> 4 && !number(0, 30)) {
	if (!number(0, 30)) {
	    do_say(ch, "Let this be a lesson to you!", 0, 0);
	    return;
	} else if (!number(0, 30)) {
	    do_say(ch, "Oh, you thought you were a toughguy, eh?", 0, 0);
	    return;
	} else if (!number(0, 30)) {
	    sprintf(buf, "Kiss my ass, %s!", PERS(ch, FIGHTING(ch)));
	    do_say(ch, buf, 0, 0);
	    return;
	}
    }

    /* Here go the fighting Routines */

    if (CHECK_SKILL(ch, SKILL_SHOOT) > number(30, 40)) {

	if (((gun = GET_EQ(ch, WEAR_WIELD)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_EQ(ch, WEAR_WIELD_2)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_IMPLANT(ch, WEAR_WIELD)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun)))) ||
	    ((gun = GET_IMPLANT(ch, WEAR_WIELD_2)) && 
	     ((IS_GUN(gun) && GUN_LOADED(gun)) || 
	      (IS_ENERGY_GUN(gun) && EGUN_CUR_ENERGY(gun))))) {

	    CUR_R_O_F(gun) = MAX_R_O_F(gun);      

	    sprintf(buf, "%s ", fname(gun->name));
	    strcat(buf, fname(FIGHTING(ch)->player.name));
	    do_shoot(ch, buf, 0, 0);
	    return;
	}
    }

    if (GET_CLASS(ch) == CLASS_SNAKE) {
	if (FIGHTING(ch)->in_room == ch->in_room &&
	    (number(0, 42 - GET_LEVEL(ch)) == 0)) {
	    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
	    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
	    if (number(0, 10) == 0)
		call_magic(ch, FIGHTING(ch),0,SPELL_POISON,GET_LEVEL(ch),CAST_CHEM);
	    return;
	}
	return;
    }  

    /** RACIAL ATTACKS FIRST **/

    // wemics 'leap' out of battle, only to return via the hunt for the kill
    if ( IS_RACE( ch, RACE_WEMIC ) &&
	 GET_MOVE( ch ) > ( GET_MAX_MOVE( ch ) >> 1 ) ) {
	
	// leap
	if ( !number( 0, 2 ) ) {
	    if ( GET_HIT( ch ) > ( GET_MAX_HIT( ch ) >> 1 ) &&
		 !ROOM_FLAGGED( ch->in_room, ROOM_NOTRACK ) ) {
		
		for ( int dir = 0; dir < NUM_DIRS; ++dir ) {
		    if ( CAN_GO( ch, dir ) &&
			 !ROOM_FLAGGED(EXIT(ch, dir)->to_room,
				       ROOM_DEATH | ROOM_NOMOB |
				       ROOM_PEACEFUL | ROOM_NOTRACK ) &&
			 EXIT(ch, dir)->to_room != ch->in_room &&
			 CHAR_LIKES_ROOM(ch, EXIT(ch, dir)->to_room) &&
			 !number( 0 , 1 ) ) {
		
			struct char_data *was_fighting = FIGHTING( ch );

			stop_fighting( FIGHTING( ch ) );
			stop_fighting( ch );
			
			//
			// leap in this direction
			//
			sprintf( buf, "$n leaps over you to the %s!", dirs[ dir ] );
			act( buf, FALSE, ch, 0, 0, TO_ROOM );
		
			struct room_data *to_room = EXIT(ch, dir)->to_room;
			char_from_room( ch );
			char_to_room( ch, to_room );
		
			sprintf( buf, "$n leaps in from %s!", from_dirs[ dir ] );
			act( buf, FALSE, ch, 0, 0, TO_ROOM );
		
			HUNTING( ch ) = was_fighting;
			return;
		    }
		}
	    }
	}

	// paralyzing scream

	else if ( !number( 0, 1 ) ) {
	    act( "$n releases a deafening scream!!", FALSE, ch, 0, 0, TO_ROOM );
	    call_magic( ch, FIGHTING( ch ), 0, SPELL_FEAR, GET_LEVEL( ch ), CAST_BREATH );
	    WAIT_STATE( FIGHTING( ch ), 3 RL_SEC );
	    return;
	}
    }
	    
	    
    if (IS_RACE(ch, RACE_UMBER_HULK)) {
	if (!number(0, 2) &&
	    !AFF_FLAGGED(FIGHTING(ch), AFF_CONFUSION)) {
	    call_magic(ch,FIGHTING(ch),0,SPELL_CONFUSION,GET_LEVEL(ch),CAST_ROD);
	}
    }

    if (IS_RACE(ch, RACE_DAEMON)) {
	if (GET_CLASS(ch) == CLASS_DAEMON_PYRO) {
	    if (!number(0, 2)) {
		call_magic(ch,FIGHTING(ch),0,SPELL_FIRE_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, 3 RL_SEC);
		return;
	    }
	}
    } 
  
    if (IS_RACE(ch, RACE_MEPHIT)) {
	if (GET_CLASS(ch) == CLASS_MEPHIT_LAVA) {
	    if (!number(0, 1)) {
		damage(ch, FIGHTING(ch), dice(20, 20), TYPE_LAVA_BREATH, WEAR_RANDOM);
		return;
	    }
	}
    }

    if (IS_MANTICORE(ch)) {

	for (vict = ch->in_room->people; vict; vict = vict->next_in_room) {
	    if (FIGHTING(vict) && FIGHTING(vict) == ch)
		break;
	}

	if (!vict)
	    vict = FIGHTING(ch);

	dam = dice(GET_LEVEL(ch), 5);
	if (GET_LEVEL(ch) + number(0, 50) > GET_LEVEL(vict) - GET_AC(ch)/2)
	    dam = 0;

	damage(ch, vict, dam, SPELL_MANTICORE_SPIKES, WEAR_RANDOM);
	WAIT_STATE(ch, 2 RL_SEC);
	return;
    }
  
    if ( IS_DEVIL( ch ) ) {

	if ( mob_fight_devil( ch ) )
	    return;
    }

    /* Slaad */
    if (IS_SLAAD(ch)) {
	num = 12;
	vict = NULL;
	new_mob = NULL;
	for (vict = ch->in_room->people;vict;vict=vict->next_in_room)
	    if (FIGHTING(vict) == ch && CAN_SEE(ch, vict) && !number(0, 4))
		break;
	if (!vict)
	    if (!(vict = FIGHTING(ch)))
		return;

	for (count_char = ch->in_room->people;count_char;count_char = count_char->next_in_room)
	    if (IS_SLAAD(count_char))
		num++;

	switch (GET_CLASS(ch)) {
	case CLASS_SLAAD_BLUE:
	    if (!number(0, 3*num) && ch->mob_specials.shared->number > 1)
		new_mob = read_mobile(GET_MOB_VNUM(ch));
	    else if (!number(0, 3*num))     	    /* red saad */
		new_mob = read_mobile (42000);
	    break;
	case CLASS_SLAAD_GREEN:
	    if (!number(0, 4*num) && ch->mob_specials.shared->number > 1)
		new_mob = read_mobile(GET_MOB_VNUM(ch));
	    else if (!number(0, 3*num))     	    /* blue slaad */
		new_mob = read_mobile (42001);
	    else if (!number(0, 2*num))     	    /* red slaad */
		new_mob = read_mobile (42000);
	    break;
	case CLASS_SLAAD_GREY:
	    if (GET_EQ(ch, WEAR_WIELD) && 
		(IS_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD), ITEM_WEAPON) &&
		 (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) ==
		  TYPE_SLASH-TYPE_HIT) && !number(0, 3)) &&
		GET_MOVE(ch) > 20)      {
		do_offensive_skill(ch, fname(vict->player.name), 0, SKILL_BEHEAD);
		return;
	    }
	    if (!number(0, 4*num) && ch->mob_specials.shared->number > 1)
		new_mob = read_mobile(GET_MOB_VNUM(ch));
	    else if (!number(0, 4*num))     	    /* green slaad */
		new_mob = read_mobile (42002);
	    else if (!number(0, 3*num))     	    /* blue slaad */
		new_mob = read_mobile (42001);
	    else if (!number(0, 2*num))     	    /* red slaad */
		new_mob = read_mobile (42000);
	    break;     
	case CLASS_SLAAD_DEATH:
	case CLASS_SLAAD_LORD:
	    if (!number(0, 4*num))     	    /* grey slaad */
		new_mob = read_mobile (42003);
	    else if (!number(0, 3*num))     	    /* green slaad */
		new_mob = read_mobile (42002);
	    else if (!number(0, 3*num))     	    /* blue slaad */
		new_mob = read_mobile (42001);
	    else if (!number(0, 4*num))     	    /* red slaad */
		new_mob = read_mobile (42000);
	    break;     
	}
	if (new_mob) {
	    char_to_room(new_mob, ch->in_room);
	    act("$n gestures, a glowing portal appears with a whine!", 
		FALSE, ch, 0, 0, TO_ROOM);
	    act("$n steps out of the portal with a crack of lightning!", 
		FALSE, new_mob, 0, 0, TO_ROOM);
	    if (FIGHTING(ch) && IS_MOB(FIGHTING(ch)))
		hit(new_mob, FIGHTING(ch), TYPE_UNDEFINED);
	    return;
	}
    }
    if (IS_TROG(ch)) {
	if (!number(0, 6)) {
	    act("$n begins to secrete a disgustingly malodorous oil!", 
		FALSE, ch, 0, 0, TO_ROOM);
	    for (vict = ch->in_room->people; vict;vict=vict->next_in_room)
		if (!IS_TROG(vict) && 
		    !IS_UNDEAD(vict) && !IS_ELEMENTAL(vict) && !IS_DRAGON(vict))
		    call_magic(ch, vict, 0, SPELL_TROG_STENCH,GET_LEVEL(ch),CAST_POTION);
	    return;
	}
    }
    if (IS_UNDEAD(ch)) {
	if (GET_CLASS(ch) == CLASS_GHOUL) {
	    if (!number(0, 10))
		act(" $n emits a terrible shriek!!", FALSE, ch, 0, 0, TO_ROOM);
	    else if (!number(0, 4))
		call_magic(ch, vict, 0, SPELL_CHILL_TOUCH, GET_LEVEL(ch), CAST_SPELL);
	}
    }

    if (IS_DRAGON(ch)) {

	if (number(0, GET_LEVEL(ch)) > 40) {
	    act("You feel a wave of sheer terror wash over you as $n approaches!", FALSE, ch, 0, 0, TO_ROOM);
	    for (vict = ch->in_room->people;vict && vict != ch; 
		 vict = next_vict) {
		next_vict = vict->next_in_room;
		if (FIGHTING(vict) && FIGHTING(vict) == ch && 
		    !mag_savingthrow(vict, GET_LEVEL(ch), SAVING_SPELL) &&
		    !IS_AFFECTED(vict, AFF_CONFIDENCE)) 
		    do_flee(vict, "", 0, 0);
	    }
	    return;
	}
	for (vict = ch->in_room->people;vict;vict=vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 4))
		break;
	if (vict == NULL)
	    vict = FIGHTING(ch);
	if (!number(0, 3)) {
	    damage(ch, vict, number(0, GET_LEVEL(ch)), TYPE_CLAW, WEAR_RANDOM);
	    return;
	} else if (!number(0, 3) && vict == FIGHTING(ch)) {
	    damage(ch, vict, number(0, GET_LEVEL(ch)), TYPE_CLAW, WEAR_RANDOM);
	    return;
	} else if (!number(0, 2)) {
	    damage(ch, vict, number(0, GET_LEVEL(ch)), TYPE_BITE, WEAR_RANDOM);
	    WAIT_STATE(ch, PULSE_VIOLENCE);
	    return;
	} else if (!number(0, 2)) {
	    switch (GET_CLASS(ch)) {
	    case CLASS_GREEN:
		call_magic(ch, vict, 0, SPELL_GAS_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;

	    case CLASS_BLACK:
		call_magic(ch, vict, 0,SPELL_ACID_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    case CLASS_BLUE:
		call_magic(ch, vict, 0, 
			   SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    case CLASS_WHITE:
	    case CLASS_SILVER:
		call_magic(ch, vict, 0,SPELL_FROST_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    case CLASS_RED:
		call_magic(ch, vict, 0,SPELL_FIRE_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    case CLASS_SHADOW_D:
		call_magic(ch,vict,0,SPELL_SHADOW_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    case CLASS_TURTLE:
		call_magic(ch, vict, 0,SPELL_STEAM_BREATH,GET_LEVEL(ch),CAST_BREATH);
		WAIT_STATE(ch, PULSE_VIOLENCE*2);
		break;
	    }
	    return;
	}
    }
    if (GET_RACE(ch) == RACE_ELEMENTAL && !number(0, 3)) {

	for (vict = ch->in_room->people;vict;vict=vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 4))
		break;
	if (vict == NULL)
	    vict = FIGHTING(ch);
	prob = GET_LEVEL(ch) - GET_LEVEL(vict) + GET_AC(vict) + GET_HITROLL(ch);
	if (prob > number(-50, 50))
	    dam = number(0, GET_LEVEL(ch)) + (GET_LEVEL(ch) >> 1);
	else
	    dam = 0;
	switch (GET_CLASS(ch)) {
	case CLASS_EARTH:
	    if (!number(0, 2)) {
		damage(ch, vict, dam, SPELL_EARTH_ELEMENTAL, WEAR_RANDOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    } else if (!number(0, 4)) {
		damage(ch, vict, dam, SKILL_BODYSLAM, WEAR_BODY);
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    }
	    break;
	case CLASS_FIRE:
	    if (!number(0, 2)) {
		damage(ch, vict, dam, SPELL_FIRE_ELEMENTAL, WEAR_RANDOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    }
	    break;
	case CLASS_WATER:
	    if (!number(0, 2)) {
		damage(ch, vict, dam, SPELL_WATER_ELEMENTAL, WEAR_RANDOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    }
	    break;
	case CLASS_AIR:
	    if (!number(0, 2)) {
		damage(ch, vict, dam, SPELL_AIR_ELEMENTAL, WEAR_RANDOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    }
	    break;
	}
	return;
    }


    /** CLASS ATTACKS HERE **/

    if ( IS_PSIONIC(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS) ) {
	if ( mob_fight_psionic( ch ) )
	    return;
    }


    if ((IS_MAGE(ch) || IS_LICH(ch)) && 
	!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 2))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);
	if (vict == FIGHTING(ch) &&
	    GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2) &&
	    GET_HIT(vict) > (GET_MAX_HIT(vict) >> 1)) {
	    if (GET_LEVEL(ch) >= 33 && !number(0, 2) &&
		GET_MANA(ch) > mag_manacost(ch, SPELL_TELEPORT)) {
		cast_spell(ch, vict, NULL, SPELL_TELEPORT);
		return;
	    } else if (GET_LEVEL(ch) >= 18 &&
		       GET_MANA(ch) > mag_manacost(ch, SPELL_LOCAL_TELEPORT)) {
		cast_spell(ch, vict, NULL, SPELL_LOCAL_TELEPORT);
		return;
	    }
	}

	if ((GET_LEVEL(ch) > 18) && !number(0, 4) && 
	    !IS_AFFECTED_2(ch, AFF2_FIRE_SHIELD)) {
	    cast_spell(ch, ch, NULL, SPELL_FIRE_SHIELD);
	    return;
	} else if ((GET_LEVEL(ch) > 14) && !number(0, 4) && 
		   !IS_AFFECTED(ch, AFF_BLUR)) {
	    cast_spell(ch, ch, NULL, SPELL_BLUR);
	    return;
	} else if ((GET_LEVEL(ch) > 5) && !number(0, 4) &&
		   !affected_by_spell(ch, SPELL_ARMOR)) {
	    cast_spell(ch, ch, NULL, SPELL_ARMOR);
	    return;
	} else if ((GET_LEVEL(ch) >= 38) && !AFF2_FLAGGED(vict, AFF2_SLOW) &&
		   !number(0, 1)) {
	    cast_spell(ch, vict, NULL, SPELL_SLOW);
	    return;
	} else if ((GET_LEVEL(ch) > 12) && !number(0, 12)) {
	    if (IS_EVIL(ch)) {
		cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
		return;
	    } else if (IS_GOOD(ch) && IS_EVIL(vict)) {
		cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
		return;
	    }
	} else if (!number(0, 3))
	    command_interpreter(ch, "cackle"); 
    
	if (!number(0, 4)) {
      
	    if (GET_LEVEL(ch) < 6) {
		cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
	    } else if (GET_LEVEL(ch) < 8) {
		cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
	    } else if (GET_LEVEL(ch) < 12) {
		cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
	    } else if (GET_LEVEL(ch) < 15) {
		cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
	    } else if (GET_LEVEL(ch) < 20) {
		cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
	    } else if (GET_LEVEL(ch) < 33) {
		cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
	    } else if (GET_LEVEL(ch) < 43) {
		cast_spell(ch, vict, NULL, SPELL_FIREBALL);
	    } else {
		cast_spell(ch, vict, NULL, SPELL_PRISMATIC_SPRAY);
	    }
	    return;
	}
    }    
  
    if (IS_CLERIC(ch) &&
	!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 3))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);
    
	if ((GET_LEVEL(ch) > 2) && (number(0, 8) == 0) &&
	    !affected_by_spell(ch, SPELL_ARMOR)) {
	    cast_spell(ch, ch, NULL, SPELL_ARMOR);
	    return;
	} else if ((GET_HIT(ch) / GET_MAX_HIT(ch)) < (GET_MAX_HIT(ch) >> 2)) {
	    if ((GET_LEVEL(ch) < 12) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
		return;
	    } else if ((GET_LEVEL(ch) < 24) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
		return;
	    } else if ((GET_LEVEL(ch) <= 34) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_HEAL);
		return;
	    } else if ((GET_LEVEL(ch) > 34) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_GREATER_HEAL);
		return;
	    }
	} 
	if ((GET_LEVEL(ch) > 12) && (number(0, 16) == 0)) {
	    if (IS_EVIL(ch))
		cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD);
	    else if (IS_GOOD(ch))
		cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
	    return;
	}

	if (!number(0, 4)) {

	    if (GET_LEVEL(ch) < 10) 
		return;
	    if (GET_LEVEL(ch) < 26) {
		cast_spell(ch, vict, NULL, SPELL_SPIRIT_HAMMER);
	    } else if (GET_LEVEL(ch) < 31) {
		cast_spell(ch, vict, NULL, SPELL_CALL_LIGHTNING);
	    } else if (GET_LEVEL(ch) < 36) {
		cast_spell(ch, vict, NULL, SPELL_HARM);
	    } else {
		cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
	    }
	    return;
	}
    }
  
    if (IS_BARB(ch) || IS_GIANT(ch)) {
	if (IS_BARB(ch) && GET_LEVEL(ch) >= 42 &&
	    GET_HIT(ch) < (GET_MAX_HIT(ch) >> 1) && GET_MANA(ch) > 30 &&
	    !number(0, 3)) {
	    do_battlecry(ch, "", 0, SCMD_CRY_FROM_BEYOND);
	    return;
	}
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 4))
		break;
	/* Else check for mages to bash */
	if (vict == NULL)
	    for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
		if (FIGHTING(vict) == ch && !number(0, 1) && 
		    (IS_MAGE(vict) || IS_CLERIC(vict)))
		    break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);
	if (!number(0,2) && (GET_CLASS(vict) == CLASS_MAGIC_USER ||
			     GET_CLASS(vict) == CLASS_CLERIC)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BASH);
	    return;
	}
	if ((GET_LEVEL(ch) > 32) && (!number(0, 4) || 
				     GET_POS(vict) < POS_FIGHTING)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PILEDRIVE);
	    return;
	} else if ((GET_LEVEL(ch) > 27) && !number(0, 6)
		   && GET_POS(vict) > POS_SLEEPING) {
	    do_sleeper(ch, GET_NAME(vict), 0, 0);
	    return;
	} else if ((GET_LEVEL(ch) > 22) && !number(0, 6)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BODYSLAM);
	    return;
	} else if ((GET_LEVEL(ch) > 20) && !number(0, 6)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_CLOTHESLINE);
	    return;
	} else if ((GET_LEVEL(ch) > 16) && !number(0, 6)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_CHOKE);
	    return;
	} else if ((GET_LEVEL(ch) > 9) && !number(0, 6)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    return;
	} else if ((GET_LEVEL(ch) > 5) && !number(0, 6)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    return;
	} else if ((GET_LEVEL(ch) > 2) && (number(0, 4) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    return;
	}
  
	if (!number(0, 4)) {
  
	    if (GET_LEVEL(ch) < 5) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    } else if (GET_LEVEL(ch) < 7) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    } else if (GET_LEVEL(ch) < 9) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_KNEE);
	    } else if (GET_LEVEL(ch) < 16) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    } else if (GET_LEVEL(ch) < 19) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_CHOKE);
	    } else if (GET_LEVEL(ch) < 25) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_CLOTHESLINE);
	    } else if (GET_LEVEL(ch) < 30) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BODYSLAM);
	    } else {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PILEDRIVE);
	    }
	    return;
	}
    }    

    if (IS_WARRIOR(ch)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && CAN_SEE(ch, vict) && !number(0, 2))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);

	if (CAN_SEE(ch, vict) && 
	    (IS_MAGE(vict) || IS_CLERIC(vict)) && GET_POS(vict) > POS_SITTING) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BASH);
	    return;
	}

	if ((GET_LEVEL(ch) > 37) && (number(0, 6) == 0) && 
	    GET_POS(vict) > POS_SLEEPING) {
	    do_sleeper(ch, GET_NAME(vict), 0, 0);
	    return;
	} else if ((GET_LEVEL(ch) >= 20) && (number(0, 6) == 0) &&
		   GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD)) {
	    do_disarm(ch, PERS(vict, ch), 0, 0);
	    return;
	} else if ((GET_LEVEL(ch) > 9) && (number(0, 6) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    return;
	} else if ((GET_LEVEL(ch) > 5) && (number(0, 6) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    return;
	} else if ((GET_LEVEL(ch) > 2) && (number(0, 4) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    return;
	}
  
	if (!number(0, 4)) {
  
	    if (GET_LEVEL(ch) < 3) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    } else if (GET_LEVEL(ch) < 5) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_KICK);
	    } else if (GET_LEVEL(ch) < 7) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    } else if (GET_LEVEL(ch) < 9) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_KNEE);
	    } else if (GET_LEVEL(ch) < 14) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_UPPERCUT);
	    } else if (GET_LEVEL(ch) < 16) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    } else if (GET_LEVEL(ch) < 24) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_HOOK);
	    } else if (GET_LEVEL(ch) < 36) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_SIDEKICK);
	    }
	    return;
	}
    }    

    if (IS_THIEF(ch)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 2))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);
	if (GET_LEVEL(ch) > 18 && GET_POS(vict) >= POS_FIGHTING && 
	    number(0, 3) == 0) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_TRIP);
	    return;
	} else if ((GET_LEVEL(ch) >= 20) && !number(0, 4) && 
		   GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD)) {
	    do_disarm(ch, PERS(vict, ch), 0, 0);
	    return;
	} else if (GET_LEVEL(ch) > 29 && !number(0, 3)) {
	    do_offensive_skill(ch, PERS(vict, ch), 0, SKILL_GOUGE);
	    return;
	} else if (GET_LEVEL(ch) > 24 && !number(0, 3)) {
	    do_feign(ch, "", 0, 0);
	    do_hide(ch, "", 0, 0);
	    SET_BIT(MOB_FLAGS(ch), MOB_MEMORY);
	    remember(ch, vict);
	    return;
	} else if (weap &&
		   ((GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT) ||
		    (GET_OBJ_VAL(weap, 3) == TYPE_STAB - TYPE_HIT)) &&
		   CHECK_SKILL(ch, SKILL_CIRCLE) > 40) {
	    do_circle(ch, fname(vict->player.name), 0, 0);
	    return;
	} else if ((GET_LEVEL(ch) > 9) && (number(0, 6) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    return;
	} else if ((GET_LEVEL(ch) > 5) && (number(0, 6) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    return;
	} else if ((GET_LEVEL(ch) > 2) && (number(0, 4) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    return;
	}
  
	if (!number(0, 4)) {
  
	    if (GET_LEVEL(ch) < 3) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_PUNCH);
	    } else if (GET_LEVEL(ch) < 7) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_STOMP);
	    } else if (GET_LEVEL(ch) < 9) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_KNEE);
	    } else if (GET_LEVEL(ch) < 16) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ELBOW);
	    } else if (GET_LEVEL(ch) < 24) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_HOOK);
	    } else if (GET_LEVEL(ch) < 36) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_SIDEKICK);
	    }
	    return;
	}
    }    
    if (IS_RANGER(ch)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 4))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);
    
	if ((GET_LEVEL(ch) > 4) && (number(0, 8) == 0) &&
	    !affected_by_spell(ch, SPELL_BARKSKIN)) {
	    cast_spell(ch, ch, NULL, SPELL_BARKSKIN);
	    WAIT_STATE(ch, PULSE_VIOLENCE);
	    return;
	} else if ((GET_LEVEL(ch) > 21) && (number(0, 8) == 0) &&
		   GET_RACE(vict) == RACE_UNDEAD && 
		   !affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
	    cast_spell(ch, ch, NULL, SPELL_INVIS_TO_UNDEAD);
	    WAIT_STATE(ch, PULSE_VIOLENCE);
	    return;
	}

	if (!number(0,1) && GET_LEVEL(ch) >= 27 && 
	    (GET_CLASS(vict) == CLASS_MAGIC_USER || 
	     GET_CLASS(vict) == CLASS_CLERIC)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_SWEEPKICK);
	    return;
	}
	if ((GET_LEVEL(ch) > 42) && (number(0, 4) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_LUNGE_PUNCH);
	    return;
	} else if ((GET_LEVEL(ch) > 25) && (number(0, 6) == 0)) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BEARHUG);
	    return;
	} else if ((GET_LEVEL(ch) >= 20) && (number(0, 6) == 0) &&
		   GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD)) {
	    do_disarm(ch, PERS(vict, ch), 0, 0);
	    return;
	}

	if (!number(0, 3)) {

	    if (GET_LEVEL(ch) < 3) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_KICK);
	    } else if (GET_LEVEL(ch) < 6) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BITE);
	    } else if (GET_LEVEL(ch) < 14) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_UPPERCUT);
	    } else if (GET_LEVEL(ch) < 17) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_HEADBUTT);
	    } else if (GET_LEVEL(ch) < 22) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_ROUNDHOUSE);
	    } else if (GET_LEVEL(ch) < 24) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_HOOK);
	    } else if (GET_LEVEL(ch) < 36) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_SIDEKICK);
	    } else {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_LUNGE_PUNCH);
	    }
	    return;
	}
    }    

    if (IS_KNIGHT(ch)) {
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	    if (FIGHTING(vict) == ch && !number(0, 4))
		break;
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
	    vict = FIGHTING(ch);

	if (CAN_SEE(ch, vict) && 
	    (IS_MAGE(vict) || IS_CLERIC(vict)) && GET_POS(vict) > POS_SITTING) {
	    do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BASH);
	    return;
	}
     
	if (GET_LEVEL(ch) > 4 && (number(0, 8) == 0) &&
	    !affected_by_spell(ch, SPELL_ARMOR)) {
	    cast_spell(ch, ch, NULL, SPELL_ARMOR);
	    return;
	} else if ((GET_HIT(ch) / GET_MAX_HIT(ch)) < (GET_MAX_HIT(ch) >> 2)) {
	    if ((GET_LEVEL(ch) < 14) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
		return;
	    } else if ((GET_LEVEL(ch) < 28) && (number(0, 10) == 0)) {
		cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
		return;
	    } else if (!number(0, 8)){
		cast_spell(ch, ch, NULL, SPELL_HEAL);
		return;
	    } 
	} else if (IS_GOOD(ch) && IS_EVIL(FIGHTING(ch)) && 
		   !number(0, 2) &&!affected_by_spell(ch, SPELL_PROT_FROM_EVIL)) {
	    cast_spell(ch, ch, NULL, SPELL_PROT_FROM_EVIL);
	    return;
	} else if (IS_EVIL(ch) && IS_GOOD(FIGHTING(ch)) && 
		   !number(0, 2) && !affected_by_spell(ch,SPELL_PROT_FROM_GOOD)) {
	    cast_spell(ch, ch, NULL, SPELL_PROT_FROM_GOOD);
	    return;
	} else if ((GET_LEVEL(ch) > 21) && (number(0, 8) == 0) &&
		   GET_RACE(vict) == RACE_UNDEAD && 
		   !affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
	    cast_spell(ch, ch, NULL, SPELL_INVIS_TO_UNDEAD);
	    return;
	} else if ((GET_LEVEL(ch) > 15) && (number(0, 5) == 0)) {
	    cast_spell(ch, vict, NULL, SPELL_SPIRIT_HAMMER);
	    return;
	} else if ((GET_LEVEL(ch) > 35) && (number(0, 5) == 0)) {
	    cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
	    return;
	} else if ((GET_LEVEL(ch) >= 20) && (number(0, 6) == 0) &&
		   GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD)) {
	    do_disarm(ch, PERS(vict, ch), 0, 0);
	    return;
	}

	if (!number(0, 3)) {
  
	    if (GET_LEVEL(ch) < 7) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_SPINFIST);
	    } else if (GET_LEVEL(ch) < 16) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_UPPERCUT);
	    } else if (GET_LEVEL(ch) < 23) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_HOOK);
	    } else if (GET_LEVEL(ch) < 35) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_LUNGE_PUNCH);
	    } else if (GET_EQ(ch, WEAR_WIELD) && 
		       GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) == 3) {
		do_offensive_skill(ch, GET_NAME(vict), 0, SKILL_BEHEAD);
	    }
	    return;
	}
    }    


    /* add new mobile fight routines here. */
}

/* Mob Memory Routines */

/* make ch remember victim */
void 
remember(struct char_data * ch, struct char_data * victim)
{
    memory_rec *tmp;
    int present = FALSE;

    if (!IS_NPC(ch) ||
	(!MOB2_FLAGGED(ch, MOB2_ATK_MOBS) && IS_NPC(victim)))
	return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
	if (tmp->id == GET_IDNUM(victim))
	    present = TRUE;

    if (!present) {
	CREATE(tmp, memory_rec, 1);
	tmp->next = MEMORY(ch);
	tmp->id = GET_IDNUM(victim);
	MEMORY(ch) = tmp;
    }
}


/* make ch forget victim */
void 
forget(struct char_data * ch, struct char_data * victim)
{
    memory_rec *curr, *prev=NULL;

    if (!(curr = MEMORY(ch)))
	return;

    while (curr && curr->id != GET_IDNUM(victim)) {
	prev = curr;
	curr = curr->next;
    }

    if (!curr)
	return;			/* person wasn't there at all. */

    if (curr == MEMORY(ch))
	MEMORY(ch) = curr->next;
    else
	prev->next = curr->next;
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    free(curr);
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}


/* erase ch's memory */
void 
clearMemory(struct char_data * ch)
{
    memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
	next = curr->next;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	free(curr);
	curr = next;
    }

    MEMORY(ch) = NULL;
}

int
char_in_memory(struct char_data *victim, struct char_data *rememberer)
{
    memory_rec *names;

    for (names = MEMORY(rememberer); names; names = names->next)
	if (names->id == GET_IDNUM(victim))
	    return TRUE;
  
    return FALSE;
}   

// devils
int mob_fight_devil( CHAR * ch )
{

    int prob = 0;
    CHAR *new_mob = NULL;
    CHAR *vict = NULL;
    int num = 0;
    CHAR *count_char = NULL;
    CHAR *next_vict = NULL;
    
    // find a suitable victim
    if ( ! ( vict = choose_opponent( ch ) ) ) {
	return 0;
    }

    // prob determines devil's chance of drawing a bead on his victim for blasting
    prob = 10 + GET_LEVEL(ch) - GET_LEVEL(vict) +  (GET_AC(vict)/10) + GET_HITROLL(ch);
    
    if (prob > number(-50, 50) && !number(0, 1)) {
	call_magic(ch, vict, NULL, 
		   (IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
		   SPELL_HELL_FROST : SPELL_HELL_FIRE,
		   GET_LEVEL(ch), CAST_BREATH);
	WAIT_STATE(ch, 2 RL_SEC);
	return 1;
    }
	
    // see if we're fighting more than 1 person, if so, blast the room
    for (vict = ch->in_room->people, num = 0;vict;vict=vict->next_in_room)
	if (ch == FIGHTING(vict))
	    num++;
    
    if (num > 1 && GET_LEVEL(ch) > number(30, 80)) {
	if (call_magic(ch, NULL, NULL, 
		       (IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
		       SPELL_HELL_FROST_STORM : SPELL_HELL_FIRE_STORM,
		       GET_LEVEL(ch), CAST_BREATH)) {
	    WAIT_STATE(ch, 3 RL_SEC);
	    return 1;
	}
    }

    // 100 move flat rate to gate, removed when the gating actually occurs
    if (GET_MOVE(ch) < 100) {
	return 0;
    }

    // hell hunters only gate IN hell, where they should not be anyway...
    if ( hell_hunter == GET_MOB_SPEC(ch) && PRIME_MATERIAL_ROOM(ch->in_room) )
	return 0;


    // see how many devils are already in the room
    for (count_char = ch->in_room->people, num = 0;count_char;
	 count_char = count_char->next_in_room)
	if (IS_DEVIL(count_char))
	    num++;

    // less chance of gating for psionic devils with mana
    if ( IS_PSIONIC( ch ) && GET_MANA( ch ) > 100 )
	num += 3;

    // gating results depend on devil char_class
    switch (GET_CLASS(ch)) {
    case CLASS_LESSER:
	if (number(0, 8) > num) {
	    if (ch->mob_specials.shared->number > 1 && !number(0, 1))
		new_mob = read_mobile(GET_MOB_VNUM(ch));
	    else if (!number(0, 2))     	    // nupperibo-spined
		new_mob = read_mobile (16111);
	    else {           	        // Abishais
		if (HELL_PLANE(ch->in_room->zone, 3))	  
		    new_mob = read_mobile(16133);
		else if (HELL_PLANE(ch->in_room->zone, 4) ||
			 HELL_PLANE(ch->in_room->zone, 6))	  
		    new_mob = read_mobile(16135);
		else if (HELL_PLANE(ch->in_room->zone, 5))	  
		    new_mob = read_mobile(16132);
		else if (HELL_PLANE(ch->in_room->zone, 1) ||
			 HELL_PLANE(ch->in_room->zone, 2))
		    new_mob = read_mobile (number(16131, 16135));
	    }
	} 
	break;
    case CLASS_GREATER  :
	if (number(0, 10) > num) {
	    if (ch->mob_specials.shared->number > 1 && !number(0, 1))
		new_mob = read_mobile(GET_MOB_VNUM(ch));
	    else if (GET_MOB_VNUM(ch) == 16118 ||    	   // Pit Fiends
		     GET_MOB_VNUM(ch) == 15252) {
		if (!number(0, 1))
		    new_mob = read_mobile(16112);	  // Barbed
		else
		    new_mob = read_mobile(16116);         // horned
	    }
	    else if (HELL_PLANE(ch->in_room->zone, 5) || 
		     ICY_DEVIL(ch)) {                    //  Ice Devil
		if (!number(0, 2))
		    new_mob = read_mobile(16115);        // Bone
		else if (!number(0, 1) && GET_LEVEL(ch) > 40)
		    new_mob = read_mobile(16146);
		else
		    new_mob = read_mobile(16132);        // white abishai
	    }
	}
	break;
    case CLASS_DUKE    :
    case CLASS_ARCH    :
	if (number(0, GET_LEVEL(ch)) > 30) {
	    act("You feel a wave of sheer terror wash over you as $n approaches!",
		FALSE, ch, 0, 0, TO_ROOM);
	    for (vict = ch->in_room->people;vict && vict != ch; 
		 vict = next_vict) {
		next_vict = vict->next_in_room;
		if (FIGHTING(vict) && FIGHTING(vict) == ch && 
		    GET_LEVEL(vict) < LVL_AMBASSADOR &&
		    !mag_savingthrow(vict, GET_LEVEL(ch) << 2, SAVING_SPELL) &&
		    !IS_AFFECTED(vict, AFF_CONFIDENCE)) 
		    do_flee(vict, "", 0, 0);
	    }
	} else if (number(0, 12) > num) {
	    if (!number(0, 1))
		new_mob = read_mobile(16118); // Pit Fiend 
	    else 
		new_mob = read_mobile(number(16114, 16118)); // barbed bone horned ice pit 
		
	}
	break;
    }
    if ( new_mob ) {
	WAIT_STATE(ch, 5 RL_SEC);
	GET_MOVE(ch) -= 100;
	char_to_room(new_mob, ch->in_room);
	WAIT_STATE(new_mob, 3 RL_SEC);
	act("$n gestures, a glowing portal appears with a whine!", 
	    FALSE, ch, 0, 0, TO_ROOM);
	act("$n steps out of the portal with a crack of lightning!", 
	    FALSE, new_mob, 0, 0, TO_ROOM);
	if (FIGHTING(ch) && IS_MOB(FIGHTING(ch)))
	    hit(new_mob, FIGHTING(ch), TYPE_UNDEFINED);
	return 1;
    }
    
    return 0;
}
