//
// File: act.merc.c                     -- Part of TempusMUD
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

#define PISTOL(gun)  ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && !IS_TWO_HAND(gun))

ACMD(do_pistolwhip)
{
    struct char_data *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;

    one_argument(argument, arg);

    if (!(vict = get_char_room_vis(ch, arg))) {
	if (FIGHTING(ch)) {
	    vict = FIGHTING(ch);
	} else if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
	    act("You pistolwhip $p!", FALSE, ch, ovict, 0, TO_CHAR);
	    return;
	} else {
	    send_to_char("Pistolwhip who?\r\n", ch);
	    return;
	}
    }
    if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && PISTOL(weap)) ||
	  ((weap = GET_EQ(ch, WEAR_WIELD_2)) && PISTOL(weap)) ||
	  ((weap = GET_EQ(ch, WEAR_HANDS)) && PISTOL(weap)))) {
	send_to_char("You need to be using a pistol.\r\n", ch);
	return;
    }
    if (vict == ch) {
	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
	    act("You fear that your death will grieve $N.",
		FALSE, ch, 0, ch->master, TO_CHAR);
	    return;
	}
	act("You ruthlessly beat yourself to death with $p!", 
	    FALSE, ch, weap, 0, TO_CHAR);
	act("$n beats $mself to death with $p!", TRUE, ch, weap, 0, TO_ROOM);
	sprintf(buf, "%s killed self with an pistolwhip at %d.",
		GET_NAME(ch), ch->in_room->number);
	mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
	gain_exp(ch, -(GET_LEVEL(ch) * 1000));
	raw_kill(ch, ch, SKILL_PISTOLWHIP);
	return;
    }
    if (!peaceful_room_ok(ch, vict, true))
	return;

    percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_PISTOLWHIP);

    if (IS_PUDDING(vict) || IS_SLIME(vict))
	prob = 0;

    cur_weap = weap;
    if (percent > prob) {
	damage(ch, vict, 0, SKILL_IMPALE, WEAR_BODY);
    } else {
	dam = dice(GET_LEVEL(ch), str_app[STRENGTH_APPLY_INDEX(ch)].todam) +
	    dice(4, weap->getWeight() );
	dam /= 5;
	damage(ch, vict, dam, SKILL_PISTOLWHIP, WEAR_HEAD);
	gain_skill_prof(ch, SKILL_PISTOLWHIP);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

