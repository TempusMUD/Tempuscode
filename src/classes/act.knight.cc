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
#include "char_data.h"
#include "fight.h"

int holytouch_after_effect(char_data *vict );
void healing_holytouch(char_data *ch, char_data *vict);
void malovent_holy_touch(char_data *ch,char_data *vict);
ACMD(do_holytouch)
{
    struct char_data *vict = NULL;
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

    if (vict == NULL) return;

    if(true || !IS_EVIL(ch) || !IS_GOOD(vict)) {
        healing_holytouch(ch,vict);
    } else {
        malovent_holy_touch(ch,vict);
    }

}
/*
    Called when holytouch "wears off"
    vict makes a saving throw modified by thier align (the higher the worse)
    if success, nothing happens.
    if failed, victim tries to gouge thier own eyes out
        - remove face and eyewear
        - falls to knees screaming
        - eyeballs appear on death
*/
int
holytouch_after_effect(char_data *vict ) {
    struct affected_type af;
    int dam = af.level * 2;

    send_to_char("Visions of pure evil sear through your mind!\r\n",vict);
    if(vict->getPosition() > POS_SITTING) {
        act("$n falls to his knees screaming!", TRUE, vict, 0, 0, TO_ROOM);
        vict->setPosition(POS_SITTING);
    } else {
        act("$n begins to scream!", TRUE, vict, 0, 0, TO_ROOM);
    }
    if(damage( NULL, vict, dam, TYPE_MALOVENT_HOLYTOUCH,WEAR_EYES))
        return 1;
    
    //obj_to_char(unequip_char(vict, WEAR_FACE, MODE_EQ), vict);
    unequip_char(vict, WEAR_FACE, MODE_EQ);
    unequip_char(vict, WEAR_EYES, MODE_EQ);
    //obj_to_char(unequip_char(vict, WEAR_EYES, MODE_EQ), vict);

    if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_NOBLIND)) {
        af.type = TYPE_MALOVENT_HOLYTOUCH;
        af.duration = af.level/10;
        af.modifier = -(af.level/10);
        af.location = APPLY_HITROLL;
        af.bitvector = AFF_BLIND;
    }
    affect_to_char(vict, &af);
    
    return 0;
}
void
malovent_holy_touch(char_data *ch,char_data *vict) {
    if(GET_LEVEL(vict) > LVL_AMBASSADOR) {
        send_to_char("Aren't they evil enough already?\r\n",ch);
        return;
    }
    struct affected_type af;
    af.type = TYPE_MALOVENT_HOLYTOUCH;
    af.is_instant = 1;
    af.level = ch->getLevelBonus(SKILL_HOLY_TOUCH);
    if(IS_SOULLESS(ch)) 
        af.level *= 2;
    af.duration = number(1,3);
    af.aff_index = 3;
    af.bitvector = AFF3_INST_AFF;
    affect_to_char(vict, &af);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
        WAIT_STATE(ch, PULSE_VIOLENCE);
            
    if(damage( ch, vict, ch->getLevelBonus(SKILL_HOLY_TOUCH) * 2, TYPE_MALOVENT_HOLYTOUCH,WEAR_EYES))
        return;

}

void 
healing_holytouch(char_data *ch, char_data *vict) {
    int mod;
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
    } else {
        send_to_char("You must rest awhile before doing this again.\r\n", ch);
    }
}

#undef __act_knight_c__
