//
// File: act.knight.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.knight.c
//

#define __act_knight_c__

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


ACMD(do_holytouch)
{
    struct char_data *vict = NULL;
    int mod;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);

    if (CHECK_SKILL(ch, SKILL_HOLY_TOUCH) < 40 || IS_NEUTRAL(ch)) {
	send_to_char("You are unable to call upon the powers necessary.\r\n", ch);
	return;
    }

    if (!*vict_name)
	vict = ch;
    else if (!(vict = get_char_room_vis(ch, vict_name))) {
	send_to_char("Heal who?\r\n", ch);
	return;
    }

    if (vict) {
	mod = (GET_LEVEL(ch) + (CHECK_SKILL(ch, SKILL_HOLY_TOUCH) >> 4));
	if (GET_MOVE(ch) > mod) {
	    if (GET_MANA(ch) < 5) {
		send_to_char("You are too spiritually exhausted.\r\n", ch);
		return;
	    }
	    GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
	    GET_MANA(ch) = MAX(0, GET_MANA(ch) - 5);
	    GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
	    if (ch == vict) {
		send_to_char("You cover your head with your hands and pray.\r\n", ch);
		act("$n covers $s head with $s hands and prays.", TRUE, ch, 0, 0, TO_ROOM);
	    } else {
		act("$N places $S hands on your head and prays.", FALSE, vict, 0, ch, TO_CHAR);
		act("$n places $s hands on the head of $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		send_to_char("You do it.\r\n", ch);
	    }
	    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);
	    if (IS_SICK(vict)&&CHECK_SKILL(ch,SKILL_CURE_DISEASE)>number(30, 100)){ 
		if (affected_by_spell(vict, SPELL_SICKNESS))
		    affect_from_char(vict, SPELL_SICKNESS);
		else
		    REMOVE_BIT(AFF3_FLAGS(vict), AFF3_SICKNESS);
	    }    
	} else
	    send_to_char("You must rest awhile before doing this again.\r\n", ch);
    } 
}

#undef __act_knight_c__
