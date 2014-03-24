//
// File: act.knight.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.knight.c
//

#ifdef HAS_CONFIG_H
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "fight.h"

bool holytouch_after_effect(long owner, struct creature *vict, int level);
void healing_holytouch(struct creature *ch, struct creature *vict);
void malovent_holy_touch(struct creature *ch, struct creature *vict);
ACMD(do_holytouch)
{
    struct creature *vict = NULL;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);

    if (CHECK_SKILL(ch, SKILL_HOLY_TOUCH) < 40 || IS_NEUTRAL(ch)) {
        send_to_char(ch,
            "You are unable to call upon the powers necessary.\r\n");
        return;
    }

    if (!*vict_name)
        vict = ch;
    else if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char(ch, "Holytouch who?\r\n");
        return;
    }

    if (vict == NULL)
        return;

    if ((NON_CORPOREAL_MOB(vict) || NULL_PSI(vict))) {
        send_to_char(ch, "That is unlikely to work.\r\n");
        return;
    }

    if (IS_EVIL(ch) && ch != vict) {
        if (!IS_GOOD(vict))
            healing_holytouch(ch, vict);
        else
            malovent_holy_touch(ch, vict);
    } else {
        healing_holytouch(ch, vict);
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
bool
holytouch_after_effect(long owner, struct creature *vict, int level)
{
    int dam = level * 2;

    send_to_char(vict, "Visions of pure evil sear through your mind!\r\n");
    if (GET_POSITION(vict) > POS_SITTING) {
        act("$n falls to $s knees screaming!", true, vict, NULL, NULL, TO_ROOM);
        GET_POSITION(vict) = POS_SITTING;
    } else {
        act("$n begins to scream!", true, vict, NULL, NULL, TO_ROOM);
    }
    WAIT_STATE(vict, 1 RL_SEC);
    if (GET_EQ(vict, WEAR_FACE))
        obj_to_char(unequip_char(vict, WEAR_FACE, EQUIP_WORN), vict);
    if (GET_EQ(vict, WEAR_EYES))
        obj_to_char(unequip_char(vict, WEAR_EYES, EQUIP_WORN), vict);

    if (damage(vict, vict, NULL, dam, TYPE_MALOVENT_HOLYTOUCH, WEAR_EYES))
        return true;
    if (!IS_NPC(vict) || !NPC_FLAGGED(vict, NPC_NOBLIND)) {
        struct affected_type af;

        init_affect(&af);

        af.next = NULL;
        af.type = TYPE_MALOVENT_HOLYTOUCH;
        af.duration = level / 10;
        af.modifier = -(level / 5);
        af.location = APPLY_HITROLL;
        af.bitvector = AFF_BLIND;
        af.aff_index = 0;
        af.owner = owner;

        affect_to_char(vict, &af);
    }

    return false;
}

/*
    Initial "bad" holytouch effect.  Happens when evil holytouch's good.
 */
void
malovent_holy_touch(struct creature *ch, struct creature *vict)
{
    int chance = 0;
    struct affected_type af;

    init_affect(&af);

    if (GET_LEVEL(vict) > LVL_AMBASSADOR) {
        send_to_char(ch, "Aren't they evil enough already?\r\n");
        return;
    }

    if (affected_by_spell(vict, SKILL_HOLY_TOUCH)
        || affected_by_spell(vict, TYPE_MALOVENT_HOLYTOUCH)) {
        act("There is nothing more you can show $N.", false, ch, NULL, vict,
            TO_CHAR);
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;

    chance = GET_ALIGNMENT(vict) / 10;
    if (GET_WIS(vict) > 20)
        chance -= (GET_WIS(vict) - 20) * 5;
    if (AFF_FLAGGED(vict, AFF_PROTECT_EVIL))
        chance -= 20;
    if (affected_by_spell(vict, SPELL_BLESS))
        chance -= 10;
    if (affected_by_spell(vict, SPELL_PRAY))
        chance -= 10;

    if (IS_SOULLESS(ch))
        chance += 25;

    WAIT_STATE(ch, 2 RL_SEC);
    check_attack(ch, vict);

    int roll = random_percentage_zero_low();

    if (PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch, "%s[HOLYTOUCH] %s   roll:%d   chance:%d%s\r\n",
            CCCYN(ch, C_NRM), GET_NAME(ch), roll, chance, CCNRM(ch, C_NRM));
    if (PRF2_FLAGGED(vict, PRF2_DEBUG))
        send_to_char(vict, "%s[HOLYTOUCH] %s   roll:%d   chance:%d%s\r\n",
            CCCYN(vict, C_NRM), GET_NAME(ch), roll, chance, CCNRM(vict,
                C_NRM));

    if (roll > chance) {
        damage(ch, vict, NULL, 0, SKILL_HOLY_TOUCH, 0);
        return;
    }

    af.type = SKILL_HOLY_TOUCH;
    af.is_instant = 1;
    af.level = skill_bonus(ch, SKILL_HOLY_TOUCH);
    af.duration = number(1, 3);
    af.aff_index = 3;
    af.bitvector = AFF3_INST_AFF;
    af.owner = GET_IDNUM(ch);

    if (IS_SOULLESS(ch))
        af.level += 20;

    affect_to_char(vict, &af);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR)
        WAIT_STATE(ch, PULSE_VIOLENCE);

    gain_skill_prof(ch, SKILL_HOLY_TOUCH);
    if (damage(ch, vict, NULL, skill_bonus(ch, SKILL_HOLY_TOUCH) * 2,
            SKILL_HOLY_TOUCH, WEAR_EYES))
        return;

}

void
healing_holytouch(struct creature *ch, struct creature *vict)
{
    int mod, gen;

    gen = GET_REMORT_GEN(ch);
    mod = (GET_LEVEL(ch) + gen + CHECK_SKILL(ch, SKILL_HOLY_TOUCH) / 16);

    if (GET_MOVE(ch) > mod) {
        if (GET_MANA(ch) < 5) {
            send_to_char(ch, "You are too spiritually exhausted.\r\n");
            return;
        }
        GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
        GET_MANA(ch) = MAX(0, GET_MANA(ch) - (mod / 2));
        GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
        if (ch == vict) {
            send_to_char(ch,
                "You cover your head with your hands and pray.\r\n");
            act("$n covers $s head with $s hands and prays.", true, ch, NULL, NULL,
                TO_ROOM);
        } else {
            act("$N places $S hands on your head and prays.", false, vict, NULL,
                ch, TO_CHAR);
            act("$n places $s hands on the head of $N.", false, ch, NULL, vict,
                TO_NOTVICT);
            send_to_char(ch, "You do it.\r\n");
        }
        if (GET_LEVEL(ch) < LVL_AMBASSADOR)
            WAIT_STATE(ch, PULSE_VIOLENCE);

        if (IS_GOOD(ch)) {
            if (gen > 0 && affected_by_spell(vict, TYPE_MALOVENT_HOLYTOUCH))
                affect_from_char(vict, TYPE_MALOVENT_HOLYTOUCH);
            if (gen > 1) {
                if (affected_by_spell(vict, SPELL_SICKNESS))
                    affect_from_char(vict, SPELL_SICKNESS);
                REMOVE_BIT(AFF2_FLAGS(vict), AFF3_SICKNESS);
            }
            if (gen > 2 && affected_by_spell(vict, SPELL_POISON))
                affect_from_char(vict, SPELL_POISON);
            if (gen > 3 && affected_by_spell(vict, TYPE_RAD_SICKNESS))
                affect_from_char(vict, TYPE_RAD_SICKNESS);
            if (gen > 4 && affected_by_spell(vict, SPELL_BLINDNESS))
                affect_from_char(vict, SPELL_BLINDNESS);
            if (gen > 5 && affected_by_spell(vict, SKILL_GOUGE))
                affect_from_char(vict, SKILL_GOUGE);
            if (gen > 6 && affected_by_spell(vict, SKILL_HAMSTRING))
                affect_from_char(vict, SKILL_HAMSTRING);
            if (gen > 7 && affected_by_spell(vict, SPELL_CURSE))
                affect_from_char(vict, SPELL_CURSE);
            if (gen > 8 && affected_by_spell(vict, SPELL_MOTOR_SPASM))
                affect_from_char(vict, SPELL_MOTOR_SPASM);
            if (gen > 9) {
                if (affected_by_spell(vict, SPELL_PETRIFY))
                    affect_from_char(vict, SPELL_PETRIFY);
            }
        }
    } else {
        send_to_char(ch, "You must rest awhile before doing this again.\r\n");
    }
}

#undef __act_knight_c__
