//
// File: act.barb.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.barb.c      --  Barbarian specific functions,      Fireball, July 1998
//

#define __act_barb_c__


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "house.h"
#include "char_class.h"
#include "bomb.h"
#include "fight.h"

ACMD(do_charge)
{
	struct affected_type af;
	struct Creature *vict = NULL;
	one_argument(argument, buf);
	// Check for berserk.
	// 

	if (CHECK_SKILL(ch, SKILL_CHARGE) < 50) {
		send_to_char(ch, "Do you really think you know what you're doing?\r\n");
		return;
	}
	// find out who we're whackin.
	vict = get_char_in_remote_room_vis(ch, buf, ch->in_room);
	if (vict == ch) {
		send_to_char(ch, "You charge in and scare yourself silly!\r\n");
		return;
	}
	if (!vict) {
		send_to_char(ch, "Charge who?\r\n");
		return;
	}
	// Instant affect flag.  AFF3_INST_AFF is checked for
	// every 4 seconds or so in burn_update and decremented.
	af.is_instant = 1;

	af.level = GET_LEVEL(ch);
	af.type = SKILL_CHARGE;
	af.duration = number(0, 1);
	af.location = 0;
	af.modifier = GET_REMORT_GEN(ch);
	af.aff_index = 3;
	af.bitvector = AFF3_INST_AFF;
	affect_to_char(ch, &af);
	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	// Whap the bastard
	hit(ch, vict, TYPE_UNDEFINED);
}

//
// perform_barb_berserk randomly selects and attacks somebody in the room
// who_was attacked is used to return a pointer to the person attacked
// precious_ch is never attacked
// sets return_flags value like damage() retval
// return value is 0 for failure, 1 for success
//

int
perform_barb_berserk(struct Creature *ch, struct Creature **who_was_attacked,
	//struct Creature *precious_ch,
	int *return_flags)
{
	static struct Creature *vict = NULL;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		vict = *it;
		if (vict == ch || ch->isFighting() ||
			PRF_FLAGGED(vict, PRF_NOHASSLE) ||
			(IS_NPC(ch) && IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS)) ||
			!can_see_creature(ch, vict) || !number(0, 1 + (GET_LEVEL(ch) >> 4)))
			continue;

		if (!IS_NPC(ch) && !IS_NPC(vict)) {
			if (!ok_to_damage(ch, vict)) {
				act("You feel a strong urge to attack $N.",
					FALSE, ch, 0, vict, TO_CHAR);
				act("$n looks like $e wants to kill you!",
					TRUE, ch, 0, vict, TO_VICT);
				break;
			}
		}
		act("You go berserk and attack $N!", FALSE, ch, 0, vict, TO_CHAR);
		act("$n attacks $N in a BERSERK rage!!", FALSE, ch, 0, vict,
			TO_NOTVICT);
		act("$n attacks you in a BERSERK rage!!", FALSE, ch, 0, vict, TO_VICT);
		*return_flags = hit(ch, vict, TYPE_UNDEFINED);

		if (!IS_SET(*return_flags, DAM_VICT_KILLED) && who_was_attacked)
			*who_was_attacked = vict;

		return 1;
	}

	return 0;
}

ACMD(do_corner)
{
	send_to_char(ch, "You back into the corner.\r\n");
	return;
}

ACMD(do_berserk)
{
	struct affected_type af, af2, af3;
	byte percent;
	percent = (number(1, 101) - GET_LEVEL(ch));

	if (IS_AFFECTED_2(ch, AFF2_BERSERK)) {
		if (percent > CHECK_SKILL(ch, SKILL_BERSERK)) {
			send_to_char(ch, "You cannot calm down!!\r\n");
			return;
		} else {
			affect_from_char(ch, SKILL_BERSERK);
			send_to_char(ch, "You are no longer berserk.\r\n");
			act("$n calms down by taking deep breaths.", TRUE, ch, 0, 0,
				TO_ROOM);
		}
		return;
	} else if (CHECK_SKILL(ch, SKILL_BERSERK) > number(0, 101)) {
		if (GET_MANA(ch) < 50) {
			send_to_char(ch, "You cannot summon the energy to do so.\r\n");
			return;
		}
		af.level = af2.level = af3.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
		af.is_instant = af2.is_instant = af3.is_instant = 0;
		af.type = SKILL_BERSERK;
		af2.type = SKILL_BERSERK;
		af3.type = SKILL_BERSERK;
		af.duration = MAX(2, 20 - GET_INT(ch));
		af2.duration = af.duration;
		af3.duration = af.duration;
		af.location = APPLY_INT;
		af2.location = APPLY_WIS;
		af3.location = APPLY_DAMROLL;
		af.modifier = -(5 + (GET_LEVEL(ch) >> 5));
		af2.modifier = -(5 + (GET_LEVEL(ch) >> 5));
		af3.modifier = (2 + GET_REMORT_GEN(ch) + (GET_LEVEL(ch) >> 4));
		af.aff_index = 2;
		af.bitvector = AFF2_BERSERK;
		af2.bitvector = 0;
		af3.bitvector = 0;
		affect_to_char(ch, &af);
		affect_to_char(ch, &af2);
		affect_to_char(ch, &af3);

		send_to_char(ch, "You go BERSERK!\r\n");
		act("$n goes BERSERK! Run for cover!", TRUE, ch, 0, ch, TO_ROOM);
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (ch == (*it) || !can_see_creature(ch, (*it)) ||
				!ok_to_damage(ch, (*it)))
				continue;
			if (percent < CHECK_SKILL(ch, SKILL_BERSERK))
				continue;
			else {
				act("You attack $N in your berserk rage!!!",
					FALSE, ch, 0, (*it), TO_CHAR);
				act("$n attacks you in $s berserk rage!!!",
					FALSE, (*it), 0, ch, TO_CHAR);
				act("$n attacks $N in $s berserk rage!!!",
					TRUE, ch, 0, (*it), TO_ROOM);
				hit(ch, (*it), TYPE_UNDEFINED);
				break;
			}
		}
	} else
		send_to_char(ch, "You cannot work up the gumption to do so.\r\n");
}

//
// do_battlecry handles both cry_from_beyond and kia
//

ACMD(do_battlecry)
{
	int trans = 0, skillnum = (subcmd == SCMD_KIA ? SKILL_KIA :
		(subcmd == SCMD_BATTLE_CRY ? SKILL_BATTLE_CRY :
			SKILL_CRY_FROM_BEYOND));
	int did = 0;

	if (CHECK_SKILL(ch, skillnum) < number(50, 110)) {
		send_to_char(ch, "You emit a feeble warbling sound.\r\n");
		act("$n makes a feeble warbling sound.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (GET_MANA(ch) < 5)
		send_to_char(ch, "You cannot work up the energy to do it.\r\n");
	else if (skillnum == SKILL_CRY_FROM_BEYOND &&
		GET_MAX_HIT(ch) == GET_HIT(ch))
		send_to_char(ch, "But you are feeling in perfect health!\r\n");
	else if (skillnum != SKILL_CRY_FROM_BEYOND &&
		GET_MOVE(ch) == GET_MAX_MOVE(ch))
		send_to_char(ch, 
			"There is no need to do this when your movement is at maximum.\r\n");
	else if (subcmd == SCMD_CRY_FROM_BEYOND) {

		GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + GET_MANA(ch));
		GET_MANA(ch) = 0;
		WAIT_STATE(ch, 4 RL_SEC);
		did = TRUE;

	} else {

		trans =
			CHECK_SKILL(ch,
			skillnum) + (GET_LEVEL(ch) << GET_REMORT_GEN(ch)) +
			(GET_CON(ch) << 3);
		trans -= (trans * GET_HIT(ch)) / (GET_MAX_HIT(ch) << 1);
		trans = MIN(MIN(trans, GET_MANA(ch)), GET_MAX_MOVE(ch) - GET_MOVE(ch));

		if (skillnum != SKILL_KIA || IS_NEUTRAL(ch)) {
			GET_MOVE(ch) += trans;
			GET_MANA(ch) -= trans;
			did = 1;
		} else
			did = 2;

		WAIT_STATE(ch, PULSE_VIOLENCE);

	}

	if (!did)
		return;

	if (subcmd == SCMD_BATTLE_CRY) {
		send_to_char(ch, "Your fearsome battle cry rings out across the land!\r\n");
		act("$n releases a battle cry that makes your blood run cold!",
			FALSE, ch, 0, 0, TO_ROOM);
	} else if (subcmd == SCMD_CRY_FROM_BEYOND) {
		send_to_char(ch, "Your cry from beyond shatters the air!!\r\n");
		act("$n unleashes a cry from beyond that makes your blood run cold!",
			FALSE, ch, 0, 0, TO_ROOM);
	} else {
		send_to_char(ch, "You release an earsplitting 'KIA!'\r\n");
		act("$n releases an earsplitting 'KIA!'", FALSE, ch, 0, 0, TO_ROOM);
	}

	sound_gunshots(ch->in_room, skillnum, 1, 1);

	if (did != 2)
		gain_skill_prof(ch, skillnum);
}

ACMD(do_cleave)
{
    Creature *vict = NULL;
	obj_data *weap = GET_EQ(ch, WEAR_WIELD);
    int percent = 0;
    int skill = MAX( GET_SKILL(ch, SKILL_CLEAVE), GET_SKILL(ch, SKILL_GREAT_CLEAVE) );
    bool great = ( GET_SKILL(ch, SKILL_GREAT_CLEAVE) > 50 );
    

	ACMD_set_return_flags(0);
	one_argument(argument, buf);

	if( weap == NULL || !IS_TWO_HAND(weap) ) {
		send_to_char(ch, "You need to be wielding a two handed weapon to cleave!\r\n");
		return;
	}

    if( !*buf ) {
        vict = FIGHTING(ch);
    } else {
        vict = get_char_room_vis(ch, buf);
    }

	if( vict == NULL ) {
		send_to_char(ch, "Cleave who?\r\n");
		WAIT_STATE(ch, 2);
		return;
	}

	if( vict == ch ) {
		send_to_char(ch, "You cannot cleave yourself.\r\n");
		return;
	}

	if (!peaceful_room_ok(ch, vict, true))
		return;
        
    int maxWhack;
    if( great ) {
        maxWhack = MAX( 3, GET_REMORT_GEN(ch)-3 );
    } else {
        maxWhack = 2;
    }

    for( int i = 0; i < maxWhack && vict != NULL; i++ ) 
    {
        percent = number(1, 101) + GET_DEX(vict);
        cur_weap = weap;
        if( AWAKE(vict) && percent > skill ) {
            WAIT_STATE(ch, 2 RL_SEC);
            int retval = damage(ch, vict, 0, SKILL_CLEAVE, WEAR_RANDOM);
            ACMD_set_return_flags(retval);
            return;
        } else {
            WAIT_STATE(vict, 1 RL_SEC);
            WAIT_STATE(ch, 3 RL_SEC);
            if( great )
                gain_skill_prof(ch, SKILL_GREAT_CLEAVE);
            gain_skill_prof(ch, SKILL_CLEAVE);
            int retval = hit(ch, vict, SKILL_CLEAVE);
            ACMD_set_return_flags(retval);
            if( IS_SET( retval, DAM_ATTACKER_KILLED ) ) {
                return;
            } else if(! IS_SET( retval, DAM_VICT_KILLED ) ) {
                return;
            }
            vict = NULL;
            // find a new victim
            CreatureList::iterator it = ch->in_room->people.begin();
			for( ; it != ch->in_room->people.end(); ++it ) {
				if( (*it) == ch || ch != (*it)->getFighting() || !can_see_creature(ch, (*it)))
                    continue;
                vict = *it;
                break;
            }
            
        }
    }
}

#undef __act_barb_c__
