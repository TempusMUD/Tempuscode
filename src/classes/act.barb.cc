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
#include "bomb.h"
#include "fight.h"

ACMD(do_charge)
{
    struct affected_type af, af2;
    struct char_data *vict = NULL;
    one_argument(argument, buf);
    // Check for beserk.
    // 

    if (CHECK_SKILL(ch, SKILL_CHARGE) < 50) {
        send_to_char("Do you really think you know what you're doing?",ch);
        return;
    }
    // find out who we're whackin.
    vict = get_char_in_remote_room_vis(ch, buf, ch->in_room);
    if(vict == ch) {
        send_to_char("You charge in and scare yourself silly!\r\n",ch);
        return;
    }
    if(!vict) {
        send_to_char("Charge who?\r\n",ch);
        return;
    }
	af.level = af2.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
	af2.type = af.type = SKILL_CHARGE;
    af2.is_instant = af.is_instant = 1;
	af2.duration = af.duration = number(1,2);
	af2.location = af.location = 0;
	af2.modifier = af.modifier = 0;
    af.aff_index = 3;
	af2.aff_index = 3;
	af.bitvector = AFF3_INST_AFF;
    af2.bitvector = AFF3_DOUBLE_DAMAGE;
	affect_to_char(ch, &af);
	affect_to_char(ch, &af2);
    // Whap the bastard
	hit(ch, vict, TYPE_UNDEFINED);
}

//
// perform_barb_beserk randomly selects and attacks somebody in the room
// who_was attacked is used to return a pointer to the person attacked
// precious_ch is never attacked
// sets return_flags value like damage() retval
// return value is 0 for failure, 1 for success
//

int perform_barb_beserk(struct char_data *ch, 
                        struct char_data **who_was_attacked,
                        struct char_data *precious_ch,
                        int * return_flags )
{
    static struct char_data *vict = NULL;

    for ( vict = ch->in_room->people; vict; vict = vict->next_in_room ) {

        if ( vict == precious_ch )
            continue;

	if ( vict == ch || FIGHTING(ch) || 
	     PRF_FLAGGED(vict, PRF_NOHASSLE) ||
	     (IS_NPC(ch) && IS_NPC(vict) && !MOB2_FLAGGED(ch, MOB2_ATK_MOBS)) ||
	     !CAN_SEE(ch, vict) || 
	     !number(0, 1 + (GET_LEVEL(ch) >> 4)))
	    continue;
	
	if (!IS_NPC(ch) && !IS_NPC(vict)) {
	    if (!PLR_FLAGGED(ch, PLR_TOUGHGUY) || 
		!PLR_FLAGGED(vict, PLR_TOUGHGUY) ||
		(IS_REMORT(ch) && !IS_REMORT(vict) && 
		 !PLR_FLAGGED(vict, PLR_REMORT_TOUGHGUY))) {
		act("You feel a strong urge to attack $N.",
		    FALSE,ch,0,vict,TO_CHAR);
		act("$n looks like $e wants to kill you!",
		    TRUE,ch,0,vict,TO_VICT);
		break;
	    }
	}
	act("You go beserk and attack $N!", FALSE, ch, 0, vict, TO_CHAR);
	act("$n attacks $N in a BESERK rage!!", FALSE, ch,0,vict,TO_NOTVICT);
	act("$n attacks you in a BESERK rage!!", FALSE, ch,0,vict,TO_VICT);
	*return_flags = hit(ch, vict, TYPE_UNDEFINED);

        if ( !IS_SET( *return_flags, DAM_VICT_KILLED ) && who_was_attacked )
            *who_was_attacked = vict;
        
	return 1;
    }

    return 0;
}

ACMD(do_corner)
{
	send_to_char("You back into the corner.\r\n",ch);
	return;
}

ACMD(do_beserk)
{
    struct affected_type af, af2, af3;
    byte percent;
    struct char_data *vict = NULL;
    percent = (number(1, 101) - GET_LEVEL(ch));
  
    if (IS_AFFECTED_2(ch, AFF2_BESERK)) {
	if (percent > CHECK_SKILL(ch, SKILL_BESERK)) {
	    send_to_char("You cannot calm down!!\r\n", ch);
	    return;
	} else {
	    affect_from_char(ch, SKILL_BESERK);
	    send_to_char("You are no longer beserk.\r\n", ch);
	    act("$n calms down by taking deep breaths.", TRUE, ch, 0, 0, TO_ROOM);
	}   
	return;
    } else if (CHECK_SKILL(ch, SKILL_BESERK) > number(0, 101)) {
	if (GET_MANA(ch) < 50) {
	    send_to_char("You cannot summon the energy to do so.\r\n", ch);
	    return;
	}
	af.level = af2.level = af3.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
	af.type = SKILL_BESERK;
	af2.type = SKILL_BESERK;
	af3.type = SKILL_BESERK;
	af.duration = MAX(2, 20 - GET_INT(ch));
	af2.duration = af.duration;
	af3.duration = af.duration;
	af.location = APPLY_INT;
	af2.location = APPLY_WIS;
	af3.location = APPLY_DAMROLL;
	af.modifier = - (5 + (GET_LEVEL(ch) >> 5));
	af2.modifier = - (5 + (GET_LEVEL(ch) >> 5));
	af3.modifier = (2 + GET_REMORT_GEN(ch) + (GET_LEVEL(ch) >> 4));
	af.aff_index    = 2;
	af.bitvector = AFF2_BESERK;
	af2.bitvector = 0;
	af3.bitvector = 0;
	affect_to_char(ch, &af);
	affect_to_char(ch, &af2);
	affect_to_char(ch, &af3);
    
	send_to_char("You go BESERK!\r\n", ch);
	act("$n goes BESERK! Run for cover!", TRUE, ch, 0, ch, TO_ROOM);
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room) {
	    if (ch == vict || !CAN_SEE(ch, vict) || 
		(!IS_NPC(vict) && 
		 (!PLR_FLAGGED(ch, PLR_TOUGHGUY) || 
		  !PLR_FLAGGED(vict, PLR_TOUGHGUY))) ||
		(!IS_NPC(vict) && IS_REMORT(ch) && 
		 !PLR_FLAGGED(vict, PLR_REMORT_TOUGHGUY)))
		continue;
	    if (percent < CHECK_SKILL(ch, SKILL_BESERK))
		continue;
	    else {
		act("You attack $N in your beserk rage!!!", 
		    FALSE, ch, 0, vict, TO_CHAR);
		act("$n attacks you in $s beserk rage!!!", 
		    FALSE, vict, 0, ch, TO_CHAR);
		act("$n attacks $N in $s beserk rage!!!", 
		    TRUE, ch, 0, vict, TO_ROOM);
		hit(ch, vict, TYPE_UNDEFINED);
		break;
	    }
	}
    } else 
	send_to_char("You cannot work up the gumption to do so.\r\n", ch);
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

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
	send_to_char("You just feel too damn peaceful here to do that.\r\n",ch);
    else if (CHECK_SKILL(ch, skillnum) < number(50, 110)) {
	send_to_char("You emit a feeble warbling sound.\r\n", ch);
	act("$n makes a feeble warbling sound.", FALSE,ch,0,0,TO_ROOM);
    } else if (GET_MANA(ch) < 5)
	send_to_char("You cannot work up the energy to do it.\r\n", ch);
    else if (skillnum == SKILL_CRY_FROM_BEYOND &&
	     GET_MAX_HIT(ch) == GET_HIT(ch))
	send_to_char("But you are feeling in perfect health!\r\n", ch);
    else if (skillnum != SKILL_CRY_FROM_BEYOND &&
	     GET_MOVE(ch) == GET_MAX_MOVE(ch))
	send_to_char("There is no need to do this when your movement is at maximum.\r\n", ch);
    else if (subcmd == SCMD_CRY_FROM_BEYOND) {

	GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + GET_MANA(ch));
	GET_MANA(ch) = 0;
	WAIT_STATE(ch, 4 RL_SEC);
	did = TRUE;

    } else {

	trans = CHECK_SKILL(ch, skillnum) + (GET_LEVEL(ch) << GET_REMORT_GEN(ch)) +
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
	send_to_char("Your fearsome battle cry rings out across the land!\r\n",
		     ch);
	act("$n releases a battle cry that makes your blood run cold!",
	    FALSE, ch, 0, 0, TO_ROOM);
    } else if (subcmd == SCMD_CRY_FROM_BEYOND) {
	send_to_char("Your cry from beyond shatters the air!!\r\n", ch);
	act("$n unleashes a cry from beyond that makes your blood run cold!",
	    FALSE, ch, 0, 0, TO_ROOM);
    } else {
	send_to_char("You release an earsplitting 'KIA!'\r\n", ch);
	act("$n releases an earsplitting 'KIA!'",
	    FALSE, ch, 0, 0, TO_ROOM);
    }
  
    sound_gunshots(ch->in_room, skillnum, 1, 1);
  
    if (did != 2)
	gain_skill_prof(ch, skillnum);
}


#undef __act_barb_c__
