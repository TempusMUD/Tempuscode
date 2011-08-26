//
// File: act.cleric.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
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
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "tmpstr.h"
#include "spells.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"

ASPELL(spell_dispel_evil)
{
    int dam = 0;

    if (IS_EVIL(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch,
            "Your soul is not righteous enough to cast this spell.\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch,
            "That is not the way to the path of righteousness.\n");
        return;
    }

    if (victim) {
        if (!IS_NPC(ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
            if (!IS_NPC(victim) && IS_EVIL(victim)) {
                act("You cannot do this because you have chosen not "
                    "to be a pkiller.\r\nYou can toggle this with the "
                    "command 'pkiller'.", false, ch, 0, victim, TO_CHAR);
                return;
            }
        }
        if (IS_EVIL(victim)) {
            dam = dice(10, 15) + skill_bonus(ch, SPELL_DISPEL_EVIL);
            damage(ch, victim, NULL, dam, SPELL_DISPEL_EVIL, WEAR_RANDOM);
        }

        GET_ALIGNMENT(victim) += MAX(10,
            skill_bonus(ch, SPELL_DISPEL_EVIL) >> 2);

        if (!is_dead(ch))
            WAIT_STATE(ch, 2 RL_SEC);
        if (!is_dead(victim))
            WAIT_STATE(victim, 1 RL_SEC);

        return;
    }

    if (obj) {
        if (!IS_OBJ_STAT(obj, ITEM_DAMNED)) {
            act("This item does not need to be cleansed.", false, ch, 0,
                NULL, TO_CHAR);
            return;
        }

        if (dice(30, (skill_bonus(ch, SPELL_DISPEL_EVIL) / 5)) < 50) {
            act("$p crumbles to dust as you attempt to cleanse it.", false,
                ch, obj, NULL, TO_CHAR);
            act("$p crumbles to dust as $n tries to cleanse it.", false,
                ch, obj, NULL, TO_NOTVICT);
            destroy_object(ch, obj, SPELL_DISPEL_EVIL);

            return;
        }

        REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_DAMNED);
        act("$p has been cleansed of evil!", false, ch, obj, NULL, TO_CHAR);

        return;
    }

    return;
}

ASPELL(spell_dispel_good)
{
    int dam = 0;

    if (IS_GOOD(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch,
            "Your soul is not stained enough to cast this spell.\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch,
            "That is not the way to the path of unrighteousness.\n");
        return;
    }

    if (victim) {
        if (!IS_NPC(ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
            if (!IS_NPC(victim) && IS_GOOD(victim)) {
                act("You cannot do this because you have chosen not "
                    "to be a pkiller.\r\nYou can toggle this with the "
                    "command 'pkiller'.", false, ch, 0, victim, TO_CHAR);
                return;
            }
        }

        if (IS_GOOD(victim)) {
            dam = dice(15, 20) + skill_bonus(ch, SPELL_DISPEL_GOOD);
            damage(ch, victim, NULL, dam, SPELL_DISPEL_GOOD, WEAR_RANDOM);
        }

        GET_ALIGNMENT(victim) -= MAX(5,
            skill_bonus(ch, SPELL_DISPEL_GOOD) / 5);

        if (!is_dead(ch))
            WAIT_STATE(ch, 2 RL_SEC);
        if (!is_dead(victim))
            WAIT_STATE(victim, 1 RL_SEC);

        return;
    }

    if (obj) {
        if (!IS_OBJ_STAT(obj, ITEM_BLESS)) {
            act("This item does not need to be defiled.", false, ch, 0,
                NULL, TO_CHAR);
            return;
        }

        if (dice(30, (skill_bonus(ch, SPELL_DISPEL_GOOD) / 5)) < 50) {
            act("$p crumbles to dust as you attempt to defile it.", false,
                ch, obj, NULL, TO_CHAR);
            act("$p crumbles to dust as $n tries to defile it.", false,
                ch, obj, NULL, TO_NOTVICT);
            destroy_object(ch, obj, SPELL_DISPEL_GOOD);

            return;
        }

        REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
        act("$p has been defiled!", false, ch, obj, NULL, TO_CHAR);

        return;
    }

    return;
}

#undef __act_cleric_c__
