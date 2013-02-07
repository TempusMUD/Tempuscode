//
// File: act.ranger.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.ranger.c
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
#include "libpq-fe.h"
#include "db.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "spells.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"

ACMD(do_bandage)
{
    struct creature *vict;
    int mod, cost;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);

    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
        if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
            send_to_char(ch, "What makes you think you're bleeding?\r\n");
            return;
        }
        mod =
            number(GET_WIS(ch), 2 * GET_LEVEL(ch) + CHECK_SKILL(ch,
                SKILL_BANDAGE)) / 16;
        if (GET_CLASS(ch) != CLASS_RANGER
            && GET_REMORT_CLASS(ch) != CLASS_RANGER)
            cost = mod * 3;
        else
            cost = mod;
        if (GET_MOVE(ch) > cost) {
            GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
            send_to_char(ch, "You carefully bandage your wounds.\r\n");
            act("$n carefully bandages $s wounds.", true, ch, NULL, NULL, TO_ROOM);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch, "You cannot do this now.\r\n");
        return;
    } else if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char(ch, "Bandage who?\r\n");
        return;
    } else {
        if (GET_HIT(vict) == GET_MAX_HIT(vict)) {
            send_to_char(ch, "What makes you think they're bleeding?\r\n");
            return;
        } else if (vict->fighting || GET_POSITION(vict) == POS_FIGHTING) {
            send_to_char(ch,
                "Bandage someone who is in battle?  How's that?\r\n");
            return;
        }
        mod = number((GET_WIS(ch) / 2),
            11 + GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_BANDAGE)) / 10;
        if (GET_CLASS(ch) != CLASS_RANGER
            && GET_REMORT_CLASS(ch) != CLASS_RANGER)
            cost = mod * 3;
        else
            cost = mod;
        if (GET_MOVE(ch) > cost) {
            GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
            act("$N bandages your wounds.", true, vict, NULL, ch, TO_CHAR);
            act("$n bandages $N's wounds.", false, ch, NULL, vict, TO_NOTVICT);
            send_to_char(ch, "You do it.\r\n");
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch, "You cannot do this now.\r\n");
    }
}

ACMD(do_firstaid)
{
    struct creature *vict;
    int mod, cost;
    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);

    mod =
        (GET_LEVEL(ch) / 2 + CHECK_SKILL(ch,
            SKILL_FIRSTAID) + GET_REMORT_GEN(ch) * 3);
    if (GET_CLASS(ch) == CLASS_MERCENARY || GET_CLASS(ch) == CLASS_RANGER
        || GET_CLASS(ch) == CLASS_MONK) {
        mod /= 2;
    } else {
        mod /= 4;
    }

    if (CHECK_SKILL(ch, SKILL_FIRSTAID) < (IS_NPC(ch) ? 50 : 30)) {
        send_to_char(ch, "You have no clue about first aid.\r\n");
        return;
    }
    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
        if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
            send_to_char(ch, "What makes you think you're bleeding?\r\n");
            return;
        }
        cost = MAX(0, mod - (GET_LEVEL(ch) / 100) * mod);
        if (GET_MOVE(ch) > cost) {
            GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
            send_to_char(ch, "You carefully tend to your wounds.\r\n");
            act("$n carefully tends to $s wounds.", true, ch, NULL, NULL, TO_ROOM);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch, "You are too tired to do this now.\r\n");
    } else if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char(ch, "Provide first aid to who?\r\n");
        return;
    } else {
        if (GET_HIT(vict) == GET_MAX_HIT(vict)) {
            send_to_char(ch, "What makes you think they're bleeding?\r\n");
            return;
        } else if (vict->fighting || GET_POSITION(vict) == POS_FIGHTING) {
            send_to_char(ch, "They are fighting right now.\r\n");
            return;
        }

        cost = MAX(0, mod - (GET_LEVEL(ch) / 100) * mod);
        if (GET_MOVE(ch) > cost) {
            GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - cost, 0);
            act("$N performs first aid on your wounds.", true, vict, NULL, ch,
                TO_CHAR);
            act("$n performs first aid on $N.", false, ch, NULL, vict,
                TO_NOTVICT);
            send_to_char(ch, "You do it.\r\n");
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch,
                "You must rest awhile before doing this again.\r\n");
    }
}

ACMD(do_medic)
{
    struct creature *vict;
    int mod =
        (GET_LEVEL(ch) / 2 + CHECK_SKILL(ch,
            SKILL_MEDIC) + GET_REMORT_GEN(ch) * 3);

    char vict_name[MAX_INPUT_LENGTH];
    one_argument(argument, vict_name);

    if (CHECK_SKILL(ch, SKILL_MEDIC) < (IS_NPC(ch) ? 50 : 30)) {
        send_to_char(ch, "You have no clue about first aid.\r\n");
        return;
    }
    if ((!*argument) || (ch == get_char_room_vis(ch, vict_name))) {
        if (GET_HIT(ch) == GET_MAX_HIT(ch)) {
            send_to_char(ch, "What makes you think you're bleeding?\r\n");
            return;
        }
        if (GET_MOVE(ch) > mod) {
            GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
            if (GET_CLASS(ch) == CLASS_RANGER) {    //2x multiplier for prime rangers
                GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + mod);
            }
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
            send_to_char(ch, "You apply some TLC to your wounds.\r\n");
            act("$n fixes up $s wounds.", true, ch, NULL, NULL, TO_ROOM);
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch, "You are too tired to do this now.\r\n");
    } else if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char(ch, "Who do you wish to help out?\r\n");
        return;
    } else if (vict->fighting || GET_POSITION(vict) == POS_FIGHTING) {
        send_to_char(ch, "They are fighting right now.\r\n");
        return;
    } else if (GET_HIT(vict) == GET_MAX_HIT(vict)) {
        act("What makes you think $E's bleeding?", true, ch, NULL, vict, TO_CHAR);
    } else {
        if (GET_MOVE(ch) > mod) {
            if (GET_CLASS(ch) == CLASS_RANGER)
                mod *= 2;       //2x multiplier for prime rangers
            GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
            GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
            act("$n gives you some TLC.  You feel better!", true, ch, NULL, vict,
                TO_VICT);
            act("$n gives some TLC to $N.", false, ch, NULL, vict, TO_NOTVICT);
            send_to_char(ch, "You do it.\r\n");
            if (GET_LEVEL(ch) < LVL_AMBASSADOR)
                WAIT_STATE(ch, PULSE_VIOLENCE);
        } else
            send_to_char(ch,
                "You must rest awhile before doing this again.\r\n");
    }
}

ACMD(do_autopsy)
{
    struct creature *vict = NULL;
    struct obj_data *corpse = NULL;
    const char *name = NULL;

    skip_spaces(&argument);

    if (!(corpse = get_obj_in_list_vis(ch, argument, ch->in_room->contents))
        && !(corpse = get_obj_in_list_vis(ch, argument, ch->carrying))) {
        send_to_char(ch, "Perform autopsy on what corpse?\r\n");
        return;
    }

    if (GET_OBJ_TYPE(corpse) != ITEM_CONTAINER || !GET_OBJ_VAL(corpse, 3)) {
        send_to_char(ch, "You can only autopsy a corpse.\r\n");
        return;
    }

    if (CHECK_SKILL(ch, SKILL_AUTOPSY) < 30) {
        send_to_char(ch, "Your forensic skills are not good enough.\r\n");
        return;
    }
    if (CORPSE_KILLER(corpse) > 0) {
        if (!(name = player_name_by_idnum(CORPSE_KILLER(corpse)))) {
            send_to_char(ch, "The result is indeterminate.\r\n");
            return;
        }
    } else if (CORPSE_KILLER(corpse) < 0) {
        if (!(vict = real_mobile_proto(-CORPSE_KILLER(corpse)))) {
            send_to_char(ch, "The result is indeterminate.\r\n");
            return;
        }
    } else {
        act("$p is too far messed up to discern.", false, ch, corpse, NULL,
            TO_CHAR);
        return;
    }

    act("$n examines $p.", true, ch, corpse, NULL, TO_ROOM);

    if (number(30, 151) > GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_AUTOPSY)) {
        send_to_char(ch,
            "You cannot determine the identity of the killer.\r\n");
        return;
    }

    if (vict)
        name = GET_NAME(vict);
    send_to_char(ch, "The killer seems to have been %s.\r\n",
        tmp_capitalize(name));
    gain_skill_prof(ch, SKILL_AUTOPSY);
}

ACMD(do_ambush)
{
    char *vict_name;
    struct creature *vict;

    vict_name = tmp_getword(&argument);
    vict = get_char_room_vis(ch, vict_name);
    if (!vict) {
        send_to_char(ch, "Ambush who?\r\n");
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;

    if (!AFF_FLAGGED(ch, AFF_HIDE)) {
        send_to_char(ch, "You must first be hidden to ambush.\r\n");
        return;
    }

    if (GET_MOVE(ch) < 48) {
        send_to_char(ch, "You're too tired... better wait awhile.\r\n");
        return;
    }

    GET_MOVE(ch) -= 48;

    if (cannot_damage(ch, vict, NULL, SKILL_AMBUSH) ||
        number(30, 120) > CHECK_SKILL(ch, SKILL_AMBUSH)) {
        act("You spring out in front of $N, surprising nobody.",
            true, ch, NULL, vict, TO_CHAR);
        act("$n springs out in front of you, but you were ready for $m",
            true, ch, NULL, vict, TO_VICT);
        act("$n springs out from hiding, but $N is ready to fight!",
            true, ch, NULL, vict, TO_NOTVICT);
        WAIT_STATE(ch, 2 RL_SEC);
        if (IS_NPC(vict))
            hit(vict, ch, TYPE_UNDEFINED);
        return;
    }

    act("You catch $N completely by surprise with your ambush!",
        true, ch, NULL, vict, TO_CHAR);
    act("$n appears out of nowhere, catching you completely by surprise!",
        true, ch, NULL, vict, TO_VICT);
    act("$n catches $N completely by surprise with $s ambush!",
        true, ch, NULL, vict, TO_NOTVICT);
    WAIT_STATE(vict, (CHECK_SKILL(ch, SKILL_AMBUSH) / 30 + 3) RL_SEC);
    gain_skill_prof(ch, SKILL_AMBUSH);
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
    hit(ch, vict, TYPE_UNDEFINED);
}

#undef __act_ranger_c__
