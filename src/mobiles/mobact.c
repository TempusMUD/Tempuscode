
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

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
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
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "flow_room.h"
#include "fight.h"
#include "obj_data.h"
#include "actions.h"
#include "guns.h"
#include "mobact.h"
#include "weather.h"
#include "search.h"
#include "voice.h"

/* external structs */
void npc_steal(struct creature *ch, struct creature *victim);
void hunt_victim(struct creature *ch);
int CLIP_COUNT(struct obj_data *clip);
int tarrasque_fight(struct creature *tarr);
int general_search(struct creature *ch, struct special_search_data *srch,
    int mode);
int smart_mobile_move(struct creature *ch, int dir);
bool can_psidrain(struct creature *ch, struct creature *vict, int *out_dist, bool msg);
void perform_psidrain(struct creature *ch, struct creature *vict);

extern int max_npc_corpse_time, max_pc_corpse_time;
extern bool production_mode;

ACMD(do_flee);
ACMD(do_sleeper);
ACMD(do_stun);
ACMD(do_circle);
ACMD(do_backstab);
ACMD(do_feign);
ACMD(do_hide);
ACMD(do_gen_comm);
ACMD(do_remove);
ACMD(do_drop);
ACMD(do_load);
ACMD(do_battlecry);
ACMD(do_fly);
ACMD(do_extinguish);
ACMD(do_pinch);
ACMD(do_combo);
ACMD(do_berserk);

// for mobile_activity
ACMD(do_repair);
ACMD(do_get);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_shoot);
ACMD(do_holytouch);
ACMD(do_medic);
ACMD(do_firstaid);
ACMD(do_bandage);
ACMD(do_activate);
ACMD(do_discharge);
ACMD(do_de_energize);

SPECIAL(thief);
SPECIAL(cityguard);
SPECIAL(hell_hunter);

#define NPC_AGGR_TO_ALIGN (NPC_AGGR_EVIL | NPC_AGGR_NEUTRAL | NPC_AGGR_GOOD)
#define IS_RACIALLY_AGGRO(ch)                   \
    (GET_RACE(ch) == RACE_GOBLIN ||             \
     GET_RACE(ch) == RACE_ALIEN_1 ||            \
     GET_RACE(ch) == RACE_ORC)
#define RACIAL_ATTACK(ch, vict)                                         \
    ((GET_RACE(ch) == RACE_GOBLIN && GET_RACE(vict) == RACE_DWARF) ||   \
     (GET_RACE(ch) == RACE_ALIEN_1 && GET_RACE(vict) == RACE_HUMAN) ||  \
     (GET_RACE(ch) == RACE_ORC && GET_RACE(vict) == RACE_DWARF))

int update_iaffects(struct creature *ch);

void
burn_update_creature(struct creature *ch)
{
    struct creature *damager = NULL;
    struct obj_data *obj = NULL;
    struct room_data *fall_to = NULL;
    struct special_search_data *srch = NULL;
    int dam = 0;
    int found = 0;
    int idx = 0;
    struct affected_type *af;

    if (is_dead(ch))
        return;

    if (ch->in_room == NULL) {
        errlog("Updating a corpse in burn_update.(%s)",
            GET_NAME(ch) == NULL ? "NULL" : GET_NAME(ch));
        return;
    }

    if (AFF3_FLAGGED(ch, AFF3_INST_AFF)) {
        if (update_iaffects(ch))
            return;
    }

    if (!is_fighting(ch) && GET_NPC_WAIT(ch))
        GET_NPC_WAIT(ch) = MAX(0, GET_NPC_WAIT(ch) - FIRE_TICK);

    if ((IS_NPC(ch) && ZONE_FLAGGED(ch->in_room->zone, ZONE_FROZEN)))
        return;

    if (AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
        && GET_POSITION(ch) == POS_FLYING && (!ch->in_room->dir_option[DOWN]
            || !room_is_open_air(ch->in_room)
            || IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED))
        && !NOGRAV_ZONE(ch->in_room->zone)) {
        send_to_char(ch,
            "You are slammed to the ground by the inexorable force of gravity!\r\n");
        act("$n is slammed to the ground by the inexorable force of gravity!\r\n", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_RESTING;
        WAIT_STATE(ch, 1);
        damager = NULL;
        if ((af = affected_by_spell(ch, SPELL_GRAVITY_WELL)))
            damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        damage(damager, ch, NULL, dice(6, 5), TYPE_FALLING, WEAR_RANDOM);
        if (is_dead(ch)) {
            return;
        }
    }
    // char is flying but unable to return
    if (GET_POSITION(ch) == POS_FLYING && !AFF_FLAGGED(ch, AFF_INFLIGHT)
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "You can no longer fly!\r\n");
        GET_POSITION(ch) = POS_STANDING;
    }
    // character is in open air
    if (ch->in_room->dir_option[DOWN] &&
        GET_POSITION(ch) < POS_FLYING &&
        !IS_SET(ch->in_room->dir_option[DOWN]->exit_info, EX_CLOSED) &&
        (!is_fighting(ch) || !AFF_FLAGGED(ch, AFF_INFLIGHT)) &&
        room_is_open_air(ch->in_room) &&
        !NOGRAV_ZONE(ch->in_room->zone) &&
        (!MOUNTED_BY(ch) || !AFF_FLAGGED(MOUNTED_BY(ch), AFF_INFLIGHT)) &&
        (fall_to = ch->in_room->dir_option[DOWN]->to_room) &&
        fall_to != ch->in_room) {

        if (AFF_FLAGGED(ch, AFF_INFLIGHT) && AWAKE(ch)
            && !AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)) {
            send_to_char(ch,
                "You realize you are about to fall and resume your flight!\r\n");
            GET_POSITION(ch) = POS_FLYING;
        } else {

            act("$n falls downward through the air!", true, ch, NULL, NULL, TO_ROOM);
            act("You fall downward through the air!", true, ch, NULL, NULL, TO_CHAR);

            char_from_room(ch, true);
            char_to_room(ch, fall_to, true);
            look_at_room(ch, ch->in_room, 0);
            act("$n falls in from above!", true, ch, NULL, NULL, TO_ROOM);
            GET_FALL_COUNT(ch)++;

            if (!room_is_open_air(ch->in_room) ||
                !ch->in_room->dir_option[DOWN] ||
                !(fall_to = ch->in_room->dir_option[DOWN]->to_room) ||
                fall_to == ch->in_room ||
                IS_SET(EXIT(ch, DOWN)->exit_info, EX_CLOSED) ||
                IS_SET(EXIT(ch, DOWN)->exit_info, EX_NOPASS)) {
                /* hit the ground */

                dam = dice(GET_FALL_COUNT(ch) * (GET_FALL_COUNT(ch) + 2), 12);
                if (room_is_watery(ch->in_room)
                    || SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER
                    || SECT_TYPE(ch->in_room) == SECT_PITCH_PIT
                    || SECT_TYPE(ch->in_room) == SECT_PITCH_SUB) {
                    dam /= 2;
                    if (AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL))
                        dam *= 4;

                    act("$n makes an enormous splash!", true, ch, NULL, NULL,
                        TO_ROOM);
                    act("You make an enormous splash!", true, ch, NULL, NULL,
                        TO_CHAR);
                } else {
                    act("$n hits the ground hard!", true, ch, NULL, NULL, TO_ROOM);
                    act("You hit the ground hard!", true, ch, NULL, NULL, TO_CHAR);
                }

                for (srch = ch->in_room->search, found = 0; srch;
                    srch = srch->next) {
                    if (SRCH_FLAGGED(srch, SRCH_TRIG_FALL)
                        && SRCH_OK(ch, srch)) {
                        found = general_search(ch, srch, found);
                    }
                }

                if (found == 2)
                    return;

                if (IS_MONK(ch))
                    dam -= (GET_LEVEL(ch) * dam) / 100;

                if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
                    dam = 0;

                GET_POSITION(ch) = POS_RESTING;

                if (dam && damage(NULL, ch, NULL, dam, TYPE_FALLING, WEAR_RANDOM))
                    return;

                GET_FALL_COUNT(ch) = 0;
            }
        }
    }
    // comfortable rooms
    if (ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_COMFORT)) {
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 1);
        GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 1);
        GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 1);
        check_position(ch);
    }
    // regen
    if (AFF_FLAGGED(ch, AFF_REGEN) || IS_TROLL(ch) || IS_VAMPIRE(ch)) {
        GET_HIT(ch) =
            MIN(GET_MAX_HIT(ch),
            GET_HIT(ch) + 1 +
            (random_percentage_zero_low() * GET_CON(ch) / 125));
        check_position(ch);
    }
    // mana tap
    if (AFF3_FLAGGED(ch, AFF3_MANA_TAP))
        GET_MANA(ch) =
            MIN(GET_MAX_MANA(ch),
            GET_MANA(ch) + 1 + random_number_zero_low(GET_WIS(ch) / 4));

    // mana leak
    if (AFF3_FLAGGED(ch, AFF3_MANA_LEAK))
        GET_MANA(ch) = MAX(0, GET_MANA(ch) -
            (1 + random_number_zero_low(GET_WIS(ch) / 4)));

    // energy tap
    if (AFF3_FLAGGED(ch, AFF3_ENERGY_TAP))
        GET_MOVE(ch) =
            MIN(GET_MAX_MOVE(ch),
            GET_MOVE(ch) + 1 + random_number_zero_low(GET_CON(ch) / 4));

    // energy leak
    if (AFF3_FLAGGED(ch, AFF3_ENERGY_LEAK))
        GET_MOVE(ch) = MAX(0, GET_MOVE(ch) -
            (1 + random_number_zero_low(GET_CON(ch) / 4)));

    // nanite reconstruction
    if (affected_by_spell(ch, SKILL_NANITE_RECONSTRUCTION)) {
        bool obj_found = false;
        bool repaired = false;

        for (idx = 0; idx < NUM_WEARS; idx++) {
            obj = GET_IMPLANT(ch, idx);
            if (obj) {
                obj_found = true;
                if (GET_OBJ_MAX_DAM(obj) != -1
                    && GET_OBJ_DAM(obj) != -1
                    && !IS_OBJ_STAT2(obj, ITEM2_BROKEN)
                    && GET_OBJ_DAM(obj) < GET_OBJ_MAX_DAM(obj)) {
                    repaired = true;
                    float amount =
                        (skill_bonus(ch, SKILL_NANITE_RECONSTRUCTION) /
                        number(33, 50));

                    GET_OBJ_DAM(obj) += (int)(ceilf(amount));
                }
            }
        }

        if (!obj_found || !repaired) {
            if (!obj_found)
                send_to_char(ch,
                    "NOTICE: Implants not found.  Nanite reconstruction halted.\r\n");
            else
                send_to_char(ch,
                    "NOTICE: Implant reconstruction complete.  Nanite reconstruction halted.\r\n");
            affect_from_char(ch, SKILL_NANITE_RECONSTRUCTION);
        }
    }
    // Irresistable Dance
    if (affected_by_spell(ch, SONG_IRRESISTABLE_DANCE)) {
        if (random_fractional_5()) {
            int message = number(0, 3);

            switch (message) {
            case 0:
                send_to_char(ch, "You dance around to the rhythm "
                    "pounding in your head!\r\n");
                act("$n dances uncontrollably to the rhythm pounding in $s head!", true, ch, NULL, NULL, TO_ROOM);
                break;
            case 1:
                send_to_char(ch, "You dance around uncontrollably!\r\n");
                act("$n dances around as if on strings!",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            case 2:
                send_to_char(ch, "You almost break your leg with your "
                    "violent dance!\r\n");
                act("$n almost breaks $s leg with $s violent dance!",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            case 3:
                send_to_char(ch, "Dance you fool!\r\n");
                act("$n skanks about like a fool!",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            default:
                send_to_char(ch, "You dance around to the rhythm "
                    "pounding in your head\r\n");
                act("$n dances uncontrollably to the rhythm pounding in $s head!", true, ch, NULL, NULL, TO_ROOM);
                break;
            }
        }
    }
    // Signed the Unholy Compact - Soulless
    if (PLR2_FLAGGED(ch, PLR2_SOULLESS) &&
        GET_POSITION(ch) == POS_SLEEPING && !random_fractional_5()) {
        send_to_char(ch,
            "The tortured cries of hell wake you from your dreams.\r\n");
        act("$n bolts upright, screaming in agony!", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
    }
    // affected by sleep spell
    if (AFF_FLAGGED(ch, AFF_SLEEP) && GET_POSITION(ch) > POS_SLEEPING
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "You suddenly fall into a deep sleep.\r\n");
        act("$n suddenly falls asleep where $e stands.", true, ch, NULL, NULL,
            TO_ROOM);
        GET_POSITION(ch) = POS_SLEEPING;
    }
    // self destruct
    if (AFF3_FLAGGED(ch, AFF3_SELF_DESTRUCT)) {
        if (MEDITATE_TIMER(ch)) {
            send_to_char(ch, "Self-destruct T-minus %d and counting.\r\n",
                MEDITATE_TIMER(ch));
            MEDITATE_TIMER(ch)--;
        } else {
            if (!IS_CYBORG(ch)) {
                errlog("%s tried to self destruct at [%d].",
                    GET_NAME(ch), ch->in_room->number);
                REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
            } else {
                engage_self_destruct(ch);
                return;
            }
        }
    }
    // character is poisoned (3)
    if (HAS_POISON_3(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        damager = NULL;
        if ((af = affected_by_spell(ch, SPELL_POISON)))
            damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        damage(damager, ch, NULL, dice(4, 3)
               + (affected_by_spell(ch, SPELL_METABOLISM) ? dice(4, 11) : 0),
               SPELL_POISON, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // Gravity Well
    if (AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
        && !random_fractional_10()
        && !NOGRAV_ZONE(ch->in_room->zone)) {
        damager = NULL;
        if ((af = affected_by_spell(ch, SPELL_GRAVITY_WELL)))
            damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        if (!mag_savingthrow(ch, af ? af->level : GET_LEVEL(ch), SAVING_PHY)) {
            damage(damager, ch, NULL,
                   number(5, (af ? af->level : GET_LEVEL(ch)) / 5),
                   TYPE_PRESSURE, -1);
        }
        if (is_dead(ch)) {
            return;
        }
    }
    // psychic crush
    if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH)) {
        damager = NULL;
        if ((af = affected_by_spell(ch, SPELL_PSYCHIC_CRUSH)))
            damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        damage(damager, ch, NULL,
               mag_savingthrow(ch, 50, SAVING_PSI) ? 0 : dice(4, 20),
               SPELL_PSYCHIC_CRUSH, WEAR_HEAD);
        if (is_dead(ch)) {
            return;
        }
    }
    // character has a stigmata
    if ((af = affected_by_spell(ch, SPELL_STIGMATA))) {
        damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        damage(damager, ch, NULL,
               mag_savingthrow(ch, af->level, SAVING_SPELL) ? 0 : dice(3, af->level),
               SPELL_STIGMATA, WEAR_FACE);
        if (is_dead(ch)) {
            return;
        }
    }
    // character has entropy field
    if ((af = affected_by_spell(ch, SPELL_ENTROPY_FIELD))
        && !random_fractional_10()
        && !mag_savingthrow(ch, (af ? af->level : GET_LEVEL(ch)), SAVING_PHY)) {
        damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        GET_MANA(ch) = MAX(0, GET_MANA(ch) -
            (13 - random_number_zero_low(GET_WIS(ch) / 4)));
        GET_MOVE(ch) = MAX(0, GET_MOVE(ch) -
            (13 - random_number_zero_low(GET_STR(ch) / 4)));
        damage(damager, ch, NULL,
               (13 - random_number_zero_low(GET_CON(ch) / 4)),
               SPELL_ENTROPY_FIELD, -1);
        if (is_dead(ch)) {
            return;
        }

    }
    // character has acidity
    if (AFF3_FLAGGED(ch, AFF3_ACIDITY)) {
        damager = NULL;
        if ((af = affected_by_spell(ch, SPELL_ACIDITY)))
            damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        damage(damager, ch, NULL,
               mag_savingthrow(ch, 50, SAVING_PHY) ? 0 : dice(2, 10),
               TYPE_ACID_BURN, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // motor spasm
    if ((af = affected_by_spell(ch, SPELL_MOTOR_SPASM))
        && !NPC_FLAGGED(ch, NPC_NOBASH) && GET_POSITION(ch) < POS_FLYING
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        obj = NULL;
        if ((random_number_zero_low(3 + (af->level / 4)) + 3) > GET_DEX(ch)
            && ch->carrying
            && !is_arena_combat(damager, ch)) {
            for (struct obj_data *o = ch->carrying;o;o = o->next_content) {
                if (can_see_object(ch, o) && !IS_OBJ_STAT(o, ITEM_NODROP)) {
                    obj = o;
                    break;
                }
            }
            if (obj) {
                send_to_char(ch, "Your muscles are seized in an uncontrollable "
                             "spasm!\r\n");
                act("$n begins convulsing uncontrollably.", true,
                    ch, NULL, NULL, TO_ROOM);
                do_drop(ch, fname(obj->aliases), 0, SCMD_DROP);
            }
        }
        if (!obj && random_number_zero_low(12 + (af->level / 4)) > GET_DEX(ch)
            && GET_POSITION(ch) > POS_SITTING) {
            send_to_char(ch,
                "Your muscles are seized in an uncontrollable spasm!\r\n"
                "You fall to the ground in agony!\r\n");
            act("$n begins convulsing uncontrollably and falls to the ground.",
                true, ch, NULL, NULL, TO_ROOM);
            GET_POSITION(ch) = POS_RESTING;
        }
        WAIT_STATE(ch, 4);
    }
    // hyperscanning increment
    if ((af = affected_by_spell(ch, SKILL_HYPERSCAN))) {
        if (GET_MOVE(ch) < 10) {
            send_to_char(ch, "Hyperscanning device shutting down.\r\n");
            affect_remove(ch, af);
        } else
            GET_MOVE(ch) -= 1;
    }
    // Soulless Goodie Two Shoes - Unholy Compact sellouts.
    if (PLR2_FLAGGED(ch, PLR2_SOULLESS) && IS_GOOD(ch)
        && random_fractional_10()) {
        int thedam;
        thedam = dice(2, 3);
        if (GET_HIT(ch) + 1 > thedam) {
            damage(ch, ch, NULL, thedam, TYPE_ANGUISH, -1);
            if (is_dead(ch)) {
                return;
            }
        }
    }
    //Lich's Lyrics rotting flesh
    if ((af = affected_by_spell(ch, SONG_LICHS_LYRICS))
        && !random_fractional_10()) {
        int dam = 0;
        if ((GET_CON(ch) / 2) + 85 > random_number_zero_low(100)) {
            dam = (af->level / 8) + dice(2, 5);
            send_to_char(ch, "You feel your life force being drawn away!\r\n");
            act("$n begins to pale as $s life force fades.", false, ch, NULL, NULL,
                TO_ROOM);
        } else {
            dam = (af->level) + dice(4, 5);
            send_to_char(ch,
                "A large chunk of your decaying flesh rots off and falls to the ground!\r\n");
            act("A large chunk of $n's decaying flesh rots off and falls to the ground!", false, ch, NULL, NULL, TO_ROOM);
            //lets make rotted flesh!!!
            struct obj_data *flesh = make_object();
            char desc[MAX_INPUT_LENGTH];

            flesh->shared = null_obj_shared;
            flesh->in_room = NULL;
            flesh->aliases = strdup("decaying flesh hunk");
            sprintf(desc, "A decaying hunk of %s's flesh is lying here.",
                GET_NAME(ch));
            flesh->line_desc = strdup(desc);
            sprintf(desc, "a decaying hunk of %s's flesh", GET_NAME(ch));
            flesh->name = strdup(desc);
            GET_OBJ_TYPE(flesh) = ITEM_FOOD;
            GET_OBJ_WEAR(flesh) = ITEM_WEAR_TAKE;
            GET_OBJ_EXTRA(flesh) = ITEM_NODONATE + ITEM_NOSELL;
            GET_OBJ_EXTRA2(flesh) = ITEM2_BODY_PART;
            GET_OBJ_VAL(flesh, 0) = 3;
            GET_OBJ_VAL(flesh, 3) = 3;
            set_obj_weight(flesh, 2);
            flesh->worn_on = -1;
            if (IS_NPC(ch)) {
                GET_OBJ_TIMER(flesh) = max_npc_corpse_time;
            } else {
                GET_OBJ_TIMER(flesh) = max_pc_corpse_time;
            }
            obj_to_room(flesh, ch->in_room);
        }
        damager = get_char_in_world_by_idnum(af->owner);
        if (damager && damager->in_room == ch->in_room) {
            GET_HIT(damager) += (dam / 4);
            GET_HIT(damager) = MIN(GET_HIT(damager), GET_MAX_HIT(damager));
            act("You absorb some of $n's life force!", false, ch, NULL, damager,
                TO_VICT);
        }
        damage(damager, ch, NULL, dam, SONG_LICHS_LYRICS, WEAR_RANDOM);
        if (is_dead(ch)) {
            return;
        }
    }
    // burning character
    if (AFF2_FLAGGED(ch, AFF2_ABLAZE)) {
        if (room_is_watery(ch->in_room)) {
            send_to_char(ch,
                "The flames on your body sizzle out and die, leaving you in a cloud of steam.\r\n");
            act("The flames on $n sizzle and die, leaving a cloud of steam.",
                false, ch, NULL, NULL, TO_ROOM);
            extinguish_creature(ch);
        }
        //
        // Sect types that don't have oxygen
        //

        else if (SECT_TYPE(ch->in_room) == SECT_FREESPACE) {
            send_to_char(ch,
                "The flames on your body die in the absence of oxygen.\r\n");
            act("The flames on $n die in the absence of oxygen.", false,
                ch, NULL, NULL, TO_ROOM);
            extinguish_creature(ch);
        }

        else if (!random_fractional_3() && !CHAR_WITHSTANDS_FIRE(ch)) {
            damager = NULL;
            if ((af = affected_by_spell(ch, SPELL_ABLAZE)))
                damager = get_char_in_world_by_idnum(af->owner);
            if (!damager)
                damager = ch;
            int dam = 0;
            if (!CHAR_WITHSTANDS_FIRE(ch)) {
                if (ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)) {
                    dam =  dice(8, 7);
                } else {
                    dam = dice(5, 5);
                }
            }
            damage(damager, ch, NULL, dam, TYPE_ABLAZE, -1);
            if (is_dead(ch)) {
                return;
            }
        }
    }
    // burning rooms
    else if ((ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) &&
            (!CHAR_WITHSTANDS_FIRE(ch) ||
                ROOM_FLAGGED(ch->in_room, ROOM_GODROOM))) ||
        (IS_VAMPIRE(ch) && room_is_sunny(ch->in_room))) {
        send_to_char(ch, "Your body suddenly bursts into flames!\r\n");
        act("$n suddenly bursts into flames!", false, ch, NULL, NULL, TO_ROOM);
        GET_MANA(ch) = 0;
        ignite_creature(ch, NULL);
        damage(ch, ch, NULL, dice(4, 5), TYPE_ABLAZE, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // freezing room
    else if (ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD)
        && !CHAR_WITHSTANDS_COLD(ch)) {
        damage(NULL, ch, NULL, dice(4, 5), TYPE_FREEZING, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // holywater ocean
    else if (ROOM_FLAGGED(ch->in_room, ROOM_HOLYOCEAN) && IS_EVIL(ch)
        && GET_POSITION(ch) < POS_FLYING) {
        damage(ch, ch, NULL, dice(4, 5), TYPE_HOLYOCEAN, WEAR_RANDOM);
        if (is_dead(ch)) {
            return;
        }
    }
    // boiling pitch
    else if ((ch->in_room->sector_type == SECT_PITCH_PIT
            || ch->in_room->sector_type == SECT_PITCH_SUB)
        && !CHAR_WITHSTANDS_HEAT(ch)) {
        damage(ch, ch, NULL, dice(4, 3), TYPE_BOILING_PITCH, WEAR_RANDOM);
        if (is_dead(ch)) {
            return;
        }
    }
    // radioactive room
    if (ROOM_FLAGGED(ch->in_room, ROOM_RADIOACTIVE)
        && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)
        && !CHAR_WITHSTANDS_RAD(ch)) {

        if (affected_by_spell(ch, SKILL_RADIONEGATION)) {

            GET_HIT(ch) = MAX(1, GET_HIT(ch) - dice(1, 7));

            if (GET_MOVE(ch) > 10) {
                GET_MOVE(ch) = MAX(10, GET_MOVE(ch) - dice(2, 7));
            }

            else {
                add_rad_sickness(ch, 10);
            }
        }

        else {

            GET_HIT(ch) = MAX(1, GET_HIT(ch) - dice(2, 7));

            if (GET_MOVE(ch) > 5)
                GET_MOVE(ch) = MAX(5, GET_MOVE(ch) - dice(2, 7));

            add_rad_sickness(ch, 20);

        }
    }

    if (SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN) {
        damage(ch, ch, NULL, dice(8, 10), TYPE_CRUSHING_DEPTH, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // no air
    if (!room_has_air(ch->in_room) &&
        !can_travel_sector(ch, SECT_TYPE(ch->in_room), 1) &&
        !ROOM_FLAGGED(ch->in_room, ROOM_DOCK) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {

        int type = 0;

        if (SECT_TYPE(ch->in_room) == SECT_FREESPACE ||
            SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_EARTH) {
            type = TYPE_SUFFOCATING;
        } else {
            type = TYPE_DROWNING;
        }
        damage(ch, ch, NULL, dice(4, 5), type, -1);
        if (is_dead(ch)) {
            return;
        }

        if (AFF_FLAGGED(ch, AFF_INFLIGHT) &&
            GET_POSITION(ch) < POS_FLYING &&
            SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM)
            do_fly(ch, tmp_strdup(""), 0, 0);
    }
    // sleeping gas
    if (ROOM_FLAGGED(ch->in_room, ROOM_SLEEP_GAS) &&
        GET_POSITION(ch) > POS_SLEEPING && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        send_to_char(ch, "You feel very sleepy...\r\n");
        if (!AFF_FLAGGED(ch, AFF_ADRENALINE)) {
            if (!mag_savingthrow(ch, 50, SAVING_CHEM)) {
                send_to_char(ch,
                    "You suddenly feel very sleepy and collapse where you stood.\r\n");
                act("$n suddenly falls asleep and collapses!", true, ch, NULL, NULL,
                    TO_ROOM);
                GET_POSITION(ch) = POS_SLEEPING;
                WAIT_STATE(ch, 4 RL_SEC);
                return;
            } else
                send_to_char(ch, "You fight off the effects.\r\n");
        }
    }
    // poison gas
    if (ROOM_FLAGGED(ch->in_room, ROOM_POISON_GAS) &&
        !PRF_FLAGGED(ch, PRF_NOHASSLE) && !IS_UNDEAD(ch) &&
        !AFF3_FLAGGED(ch, AFF3_NOBREATHE)) {

        if (!HAS_POISON_3(ch)) {
            send_to_char(ch,
                "Your lungs burn as you inhale a poisonous gas!\r\n");
            act("$n begins choking and sputtering!", false, ch, NULL, NULL, TO_ROOM);
        }

        found = 1;
        if (HAS_POISON_2(ch))
            call_magic(ch, ch, NULL, NULL, SPELL_POISON, 60, CAST_CHEM);
        else if (HAS_POISON_1(ch))
            call_magic(ch, ch, NULL, NULL, SPELL_POISON, 39, CAST_CHEM);
        else if (!HAS_POISON_3(ch))
            call_magic(ch, ch, NULL, NULL, SPELL_POISON, 10, CAST_CHEM);
        else
            found = 0;

        if (found)
            return;
    }
    //
    // animated corpses decaying
    //

    if (IS_NPC(ch) &&
        GET_NPC_VNUM(ch) == ZOMBIE_VNUM && room_is_sunny(ch->in_room)) {
        damage(ch, ch, NULL, dice(4, 5), TOP_SPELL_DEFINE, -1);
        if (is_dead(ch)) {
            return;
        }
    }

    /* Hunter Mobs */
    if (NPC_HUNTING(ch) && !AFF_FLAGGED(ch, AFF_BLIND) &&
        GET_POSITION(ch) > POS_SITTING && !GET_NPC_WAIT(ch)) {
        if (NPC_FLAGGED(ch, NPC_WIMPY)) {
            if ((GET_HIT(ch) < MIN(500, GET_MAX_HIT(ch)) * 0.80)
                || (100 - ((GET_HIT(ch) * 100) / GET_MAX_HIT(ch))) >
                GET_MORALE(ch) + number(-5, 10 + (GET_INT(ch) / 4))) {
                if (ch->in_room == ch->char_specials.hunting->in_room)
                    do_flee(ch, tmp_strdup(""), 0, 0);
            }
        } else
            hunt_victim(ch);
        return;
    }
}

void
burn_update(void)
{
    g_list_foreach(creatures, (GFunc) burn_update_creature, NULL);
}

//
// return a measure of how willing ch is to help vict in a fight
// <= 0 return value means ch will not help vict
//

int
helper_help_probability(struct creature *ch, struct creature *vict)
{

    int prob = GET_MORALE(ch);

    if (!IS_NPC(ch) || !vict)
        return 0;

    //
    // newbie bonus!
    //

    if (!IS_NPC(vict) && GET_LEVEL(vict) < 6) {
        prob += GET_LEVEL(ch);
    }
    //
    // don't help ppl on our shitlist
    //

    if (char_in_memory(vict, ch))
        return 0;

    //
    // bonus if vict is leader
    //

    if (IS_NPC(vict) && GET_NPC_LEADER(ch) == GET_NPC_VNUM(vict)) {
        prob += GET_LEVEL(vict) * 2;
    }
    //
    // bonus if vict is follower
    //

    if (IS_NPC(vict) && GET_NPC_LEADER(vict) == GET_NPC_VNUM(ch)) {
        prob += GET_LEVEL(vict);
    }
    //
    // bonus for same race
    //

    if (GET_RACE(ch) == GET_RACE(vict)) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // bonus for same class
    //

    if (GET_CLASS(ch) == GET_CLASS(vict)) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // bonus for same alignment for good and neutral chars
    //

    if ((IS_GOOD(ch) && IS_GOOD(vict)) || (IS_NEUTRAL(ch) && IS_NEUTRAL(vict))) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // penalty for good chars helping evil, or evil helping good
    //

    if ((IS_GOOD(ch) && IS_EVIL(vict)) || (IS_EVIL(ch) && IS_GOOD(vict))) {
        prob -= GET_LEVEL(vict);
    }
    //
    // aggro align penalty
    //

    if ((NPC_FLAGGED(ch, NPC_AGGR_EVIL) && IS_EVIL(vict)) ||
        (NPC_FLAGGED(ch, NPC_AGGR_GOOD) && IS_GOOD(vict)) ||
        (NPC_FLAGGED(ch, NPC_AGGR_NEUTRAL) && IS_NEUTRAL(vict))) {
        prob -= GET_LEVEL(vict);
    }
    //
    // racial hatred penalty
    //

    if (RACIAL_ATTACK(ch, vict)) {
        prob -= GET_LEVEL(vict);
    }

    if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
        prob += GET_LEVEL(vict);

    return prob;
}

//
// return a measure of how willing ch is to attack vict
// <= 0 return value means ch will not attack vict
//

int
helper_attack_probability(struct creature *ch, struct creature *vict)
{

    int prob = GET_MORALE(ch);

    if (!IS_NPC(ch))
        return 0;

    //
    // don't attack newbies
    //

    if (!IS_NPC(vict) && GET_LEVEL(vict) < 6)
        return 0;

    //
    // don't attack other mobs unless flagged
    //

    if (!NPC2_FLAGGED(ch, NPC2_ATK_MOBS) && IS_NPC(vict))
        return 0;

    //
    // don't attack same race if prohibited
    //

    if (NPC2_FLAGGED(ch, NPC2_NOAGGRO_RACE) && GET_RACE(ch) == GET_RACE(vict))
        return 0;

    //
    // don't attack your leader
    //

    if (IS_NPC(vict) && GET_NPC_LEADER(ch) == GET_NPC_VNUM(vict))
        return 0;

    //
    // don't attack your followers
    //

    if (IS_NPC(vict) && GET_NPC_LEADER(vict) == GET_NPC_VNUM(ch))
        return 0;

    //
    // memory bonus
    //

    if (char_in_memory(vict, ch)) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // racial hatred bonus
    //

    if (RACIAL_ATTACK(ch, vict)) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // aggro bonus
    //

    if (NPC_FLAGGED(ch, NPC_AGGRESSIVE))
        prob += GET_MORALE(ch) / 2;

    //
    // aggro align bonus
    //

    if ((NPC_FLAGGED(ch, NPC_AGGR_EVIL) && IS_EVIL(vict)) ||
        (NPC_FLAGGED(ch, NPC_AGGR_GOOD) && IS_GOOD(vict)) ||
        (NPC_FLAGGED(ch, NPC_AGGR_NEUTRAL) && IS_NEUTRAL(vict))) {
        prob += GET_MORALE(ch) / 2;
    }
    //
    // level penalty
    //
    // range from -100 -> 0 with level difference
    //

    prob += (GET_LEVEL(ch) - GET_LEVEL(vict)) * 2;

    //
    // penalty for same race
    //

    if (GET_RACE(vict) == GET_RACE(ch))
        prob -= GET_LEVEL(vict);

    //
    // penalty for same class
    //

    if (GET_CLASS(vict) == GET_CLASS(ch))
        prob -= GET_LEVEL(vict);

    //
    // penalty for same align if positive alignment
    //

    if (GET_ALIGNMENT(ch) > 0) {
        prob -= GET_ALIGNMENT(vict) / 10;
    }
    //
    // bonus for opposite align if negative alignment
    //

    else if (GET_ALIGNMENT(vict) > 0) {
        // bonus ranges from 0 -> 50 with alignment 0 -> 1000
        prob += (GET_ALIGNMENT(vict) / 20);
    }

    if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
        prob -= GET_LEVEL(vict);

    return prob;
}

//
// make ch help fvict by attacking vict
//
// return values are the same as damage()
//

int
helper_assist(struct creature *ch, struct creature *vict,
    struct creature *fvict)
{
    int prob = 0;
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

    if (!ch || !vict || !fvict) {
        errlog("Illegal value(s) passed to helper_assist().");
        return 0;
    }

    if (GET_RACE(ch) == GET_RACE(fvict))
        prob += 10 + (GET_LEVEL(ch) / 2);
    else if (GET_RACE(ch) != GET_RACE(vict))
        prob -= 10;
    else if (IS_DEVIL(ch) || IS_KNIGHT(ch) || IS_WARRIOR(ch) || IS_RANGER(ch))
        prob += GET_LEVEL(ch);

    if (IS_ANIMAL(ch) && affected_by_spell(fvict, SPELL_ANIMAL_KIN))
        prob += GET_LEVEL(vict);

    if (weap) {
        if (IS_THIEF(ch) && GET_LEVEL(ch) > 43 &&
            (GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT ||
                GET_OBJ_VAL(weap, 3) == TYPE_STAB - TYPE_HIT)) {
            do_circle(ch, GET_NAME(vict), 0, 0);
            return 0;
        }

        if (IS_ENERGY_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
            EGUN_CUR_ENERGY(weap) > 10) {
            do_shoot(ch, tmp_sprintf("%s %s", fname(weap->aliases),
                    fname(vict->player.name)), 0, 0);
            return 0;
        }

        if (IS_GUN(weap) && CHECK_SKILL(ch, SKILL_SHOOT) > 40 &&
            GUN_LOADED(weap)) {
            CUR_R_O_F(weap) = MAX_R_O_F(weap);
            sprintf(buf, "%s ", fname(weap->aliases));
            strcat(buf, fname(vict->player.name));
            do_shoot(ch, buf, 0, 0);
            return 0;
        }
    }


    if (prob > random_percentage()) {
        act("$n comes to the rescue of $N!", true, ch, NULL, fvict, TO_NOTVICT);
        act("$n comes to your rescue!", true, ch, NULL, fvict, TO_VICT);
        remove_combat(vict, fvict);
    } else {
        act("$n jumps to the aid of $N!", true, ch, NULL, fvict, TO_NOTVICT);
        act("$n jumps to your aid!", true, ch, NULL, fvict, TO_VICT);
    }

    return hit(ch, vict, TYPE_UNDEFINED);
}

void
mob_load_unit_gun(struct creature *ch, struct obj_data *clip,
    struct obj_data *gun, bool internal)
{
    char loadbuf[1024];
    sprintf(loadbuf, "%s %s", fname(clip->aliases),
        internal ? "internal " : "");
    strcat(loadbuf, fname(gun->aliases));
    do_load(ch, loadbuf, 0, SCMD_LOAD);
    if (GET_NPC_VNUM(ch) == 1516 && IS_CLIP(clip) && random_binary())
        perform_say(ch, "say", "Let's Rock.");
    return;
}

struct obj_data *
find_bullet(struct creature *ch, int gun_type, struct obj_data *list)
{
    struct obj_data *bul = NULL;

    for (bul = list; bul; bul = bul->next_content) {
        if (can_see_object(ch, bul) &&
            IS_BULLET(bul) && GUN_TYPE(bul) == gun_type)
            return (bul);
    }
    return NULL;
}

void
mob_reload_gun(struct creature *ch, struct obj_data *gun)
{
    bool internal = false;
    int count = 0, i;
    struct obj_data *clip = NULL, *bul = NULL, *cont = NULL;

    if (GET_INT(ch) < (random_number_zero_low(4) + 3))
        return;

    if (gun->worn_by && gun != GET_EQ(ch, gun->worn_on))
        internal = true;

    if (IS_GUN(gun)) {
        if (!MAX_LOAD(gun)) {   /** a gun that uses a clip **/
            if (gun->contains) {
                sprintf(buf, "%s%s", internal ? "internal " : "",
                    fname(gun->aliases));
                do_load(ch, buf, 0, SCMD_UNLOAD);
            }

            /** look for a clip that matches the gun **/
            for (clip = ch->carrying; clip; clip = clip->next_content)
                if (IS_CLIP(clip) && GUN_TYPE(clip) == GUN_TYPE(gun))
                    break;

            if (!clip)
                return;

            if ((count = count_contained_objs(clip)) >= MAX_LOAD(clip)) {    /** a full clip **/
                mob_load_unit_gun(ch, clip, gun, internal);
                return;
            }

            /** otherwise look for a bullet to load into the clip **/
            if ((bul = find_bullet(ch, GUN_TYPE(gun), ch->carrying))) {
                mob_load_unit_gun(ch, bul, clip, false);
                count++;
                if (count >= MAX_LOAD(clip))          /** load full clip in gun **/
                    mob_load_unit_gun(ch, clip, gun, internal);
                return;
            }

            /* no bullet found, look for one in bags * */
            for (cont = ch->carrying; cont; cont = cont->next_content) {
                if (!can_see_object(ch, cont)
                    || !IS_OBJ_TYPE(cont, ITEM_CONTAINER) || CAR_CLOSED(cont))
                    continue;
                if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
                    sprintf(buf, "%s ", fname(bul->aliases));
                    strcat(buf, fname(cont->aliases));
                    do_get(ch, buf, 0, 0);
                    mob_load_unit_gun(ch, bul, clip, false);
                    count++;
                    if (count >= MAX_LOAD(clip))          /** load full clip in gun **/
                        mob_load_unit_gun(ch, clip, gun, internal);
                    return;
                }
            }
            for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
                if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER)
                    && !CAR_CLOSED(cont)
                    && (bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
                    sprintf(buf, "%s ", fname(bul->aliases));
                    strcat(buf, fname(cont->aliases));
                    do_get(ch, buf, 0, 0);
                    mob_load_unit_gun(ch, bul, clip, false);
                    count++;
                    if (count >= MAX_LOAD(clip))          /** load full clip in gun **/
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

        /* no bullet found, look for one in bags * */
        for (cont = ch->carrying; cont; cont = cont->next_content) {
            if (!can_see_object(ch, cont) || !IS_OBJ_TYPE(cont, ITEM_CONTAINER)
                || CAR_CLOSED(cont))
                continue;
            if ((bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
                sprintf(buf, "%s ", fname(bul->aliases));
                strcat(buf, fname(cont->aliases));
                do_get(ch, buf, 0, 0);
                mob_load_unit_gun(ch, bul, gun, internal);
            }
        }
        for (i = 0; i < NUM_WEARS; i++) {  /** worn bags **/
            if ((cont = GET_EQ(ch, i)) && IS_OBJ_TYPE(cont, ITEM_CONTAINER) &&
                !CAR_CLOSED(cont) &&
                (bul = find_bullet(ch, GUN_TYPE(gun), cont->contains))) {
                sprintf(buf, "%s ", fname(bul->aliases));
                strcat(buf, fname(cont->aliases));
                do_get(ch, buf, 0, 0);
                mob_load_unit_gun(ch, bul, gun, internal);
                return;
            }
        }
        return;
    }
    /** energy guns **/
}

//
// return values are the same as damage()
//
//

bool
check_infiltrate(struct creature * ch, struct creature * vict)
{
    if (!ch || !vict) {
        errlog("NULL char in check_infiltrate()!");
        return false;
    }

    int prob = skill_bonus(ch, SKILL_INFILTRATE);
    int percent = number(1, 115);

    if (IS_NPC(vict) && NPC_FLAGGED(vict, NPC_SPIRIT_TRACKER) &&
        char_in_memory(ch, vict))
        return false;

    if (!AFF3_FLAGGED(ch, AFF3_INFILTRATE) && !AFF_FLAGGED(ch, AFF_HIDE))
        return false;

    if (affected_by_spell(vict, ZEN_AWARENESS) ||
        AFF2_FLAGGED(vict, AFF2_TRUE_SEEING))
        percent += 17;

    if (AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        prob += 17;

    if (GET_POSITION(vict) <= POS_FIGHTING)
        prob += 10;

    if (AFF2_FLAGGED(ch, AFF2_HASTE))
        prob += 15;

    if (AFF2_FLAGGED(vict, AFF2_HASTE))
        percent += 15;

    percent += (skill_bonus(vict, SKILL_INFILTRATE) / 2);

    if (ch && PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch, "%s[INFILTRATE] chance:%d   roll:%d%s\r\n",
            CCCYN(ch, C_NRM), prob, percent, CCNRM(ch, C_NRM));

    return prob > percent;
}

void
best_initial_attack(struct creature *ch, struct creature *vict)
{

    struct obj_data *gun;
    int cur_class = 0;

    // Standing should take just as long for mobs
    if (GET_POSITION(ch) < POS_STANDING) {
        if (!AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
            || number(1, 50) < GET_STR(ch)) {
            act("$n jumps to $s feet!", true, ch, NULL, NULL, TO_ROOM);
            GET_POSITION(ch) = POS_STANDING;
        }
        GET_NPC_WAIT(ch) += PULSE_VIOLENCE;
        return;
    }
    // Act like the remort class 1/3rd of the time
    if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_fractional_3())
        cur_class = GET_REMORT_CLASS(ch);
    else
        cur_class = GET_CLASS(ch);

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

        if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {
            do_shoot(ch, tmp_sprintf("%s %s", fname(gun->aliases),
                                     fname(vict->player.name)),
                     0, 0);
            return;
        }
    }
    //
    // thief mobs
    //
    if (cur_class == CLASS_THIEF) {
        if (GET_LEVEL(ch) >= 35
            && GET_POSITION(vict) > POS_STUNNED
            && !is_fighting(vict))
            perform_stun(ch, vict);

        else if (((gun = GET_EQ(ch, WEAR_WIELD)) && STAB_WEAPON(gun)) ||
                 ((gun = GET_EQ(ch, WEAR_WIELD_2)) && STAB_WEAPON(gun)) ||
                 ((gun = GET_EQ(ch, WEAR_HANDS)) && STAB_WEAPON(gun))) {

            if (!is_fighting(vict))
                do_backstab(ch, fname(vict->player.name), 0, 0);
            else if (GET_LEVEL(ch) > 43)
                do_circle(ch, fname(vict->player.name), 0, 0);
            else if (GET_LEVEL(ch) >= 30)
                perform_offensive_skill(ch, vict, SKILL_GOUGE);
            else {
                hit(ch, vict, TYPE_UNDEFINED);
            }
        } else {
            if (GET_LEVEL(ch) >= 30) {
                perform_offensive_skill(ch, vict, SKILL_GOUGE);
            } else {
                hit(ch, vict, TYPE_UNDEFINED);
            }
        }
        return;
    }
    //
    // psionic mobs
    //
    if (cur_class == CLASS_PSIONIC) {
        psionic_best_attack(ch, vict);
        return;
    }
    //
    // mage mobs
    //

    if (cur_class == CLASS_MAGE && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        mage_best_attack(ch, vict);
        return;
    }
    //
    // barb and warrior mobs
    //

    if (cur_class == CLASS_BARB || cur_class == CLASS_WARRIOR) {
        if (GET_LEVEL(ch) >= 25 && GET_POSITION(vict) > POS_SITTING)
            perform_offensive_skill(ch, vict, SKILL_BASH);
        else
            hit(ch, vict, TYPE_UNDEFINED);
        return;
    }
    //
    // monk mobs
    //

    if (cur_class == CLASS_MONK) {
        if (GET_LEVEL(ch) >= 39 && GET_POSITION(vict) > POS_STUNNED) {
            do_pinch(ch, tmp_sprintf("omega %s", fname(vict->player.name)),
                0, 0);
            return;
        }
        // Screw them up.
        if (GET_POSITION(vict) <= POS_STUNNED) {
            if (!affected_by_spell(vict, SKILL_PINCH_GAMMA)
                && (GET_LEVEL(ch) > 35)) {
                do_pinch(ch, tmp_sprintf("gamma %s", fname(vict->player.name)),
                    0, 0);
            } else if (!affected_by_spell(vict, SKILL_PINCH_EPSILON)
                && (GET_LEVEL(ch) > 26)) {
                do_pinch(ch, tmp_sprintf("epsilon %s",
                        fname(vict->player.name)), 0, 0);
            } else if (!affected_by_spell(vict, SKILL_PINCH_DELTA)
                && (GET_LEVEL(ch) > 19)) {
                do_pinch(ch, tmp_sprintf("delta %s", fname(vict->player.name)),
                    0, 0);
            } else if (!affected_by_spell(vict, SKILL_PINCH_BETA)
                && (GET_LEVEL(ch) > 12)) {
                do_pinch(ch, tmp_sprintf(" beta %s", fname(vict->player.name)),
                    0, 0);
            } else if (!affected_by_spell(vict, SKILL_PINCH_ALPHA)
                && (GET_LEVEL(ch) > 6)) {
                do_pinch(ch, tmp_sprintf("alpha %s", fname(vict->player.name)),
                    0, 0);
            } else if (GET_LEVEL(ch) >= 33) {
                do_combo(ch, fname(vict->player.name), 0, 0);
            } else if (GET_LEVEL(ch) >= 30) {
                perform_offensive_skill(ch, vict, SKILL_RIDGEHAND);
            } else {
                hit(ch, vict, TYPE_UNDEFINED);
            }
        }
        return;
    }
    //
    // dragon mobs
    //

    if (IS_DRAGON(ch) && random_fractional_4()) {
        switch (GET_CLASS(ch)) {
        case CLASS_GREEN:
            if (random_fractional_3()) {
                call_magic(ch, vict, NULL, NULL, SPELL_GAS_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            }
            break;
        case CLASS_BLACK:
            if (random_fractional_3()) {
                call_magic(ch, vict, NULL, NULL, SPELL_ACID_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            }
            break;
        case CLASS_BLUE:
            if (random_fractional_3()) {
                call_magic(ch, vict, NULL, NULL,
                    SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            }
            break;
        case CLASS_WHITE:
        case CLASS_SILVER:
            if (random_fractional_3()) {
                call_magic(ch, vict, NULL, NULL, SPELL_FROST_BREATH,
                    GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            }
            break;
        case CLASS_RED:
            if (random_fractional_3()) {
                call_magic(ch, vict, NULL, NULL, SPELL_FIRE_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            }
            break;
        }
    }

    hit(ch, vict, TYPE_UNDEFINED);
}

bool
CHAR_LIKES_ROOM(struct creature *ch, struct room_data *room)
{
    if (IS_ELEMENTAL(ch)) {
        // Keep water elementals in the water
        if (GET_CLASS(ch) == CLASS_WATER && !room_is_watery(room))
            return false;

        // Keep fire elementals out of the water
        if (GET_CLASS(ch) == CLASS_FIRE && room_is_watery(room))
            return false;
    }

    if (ROOM_FLAGGED(room, ROOM_FLAME_FILLED) && !CHAR_WITHSTANDS_FIRE(ch))
        return false;

    if (ROOM_FLAGGED(room, ROOM_ICE_COLD) && !CHAR_WITHSTANDS_COLD(ch))
        return false;

    if (ROOM_FLAGGED(room, ROOM_HOLYOCEAN) && IS_EVIL(ch))
        return false;

    if (!can_travel_sector(ch, room->sector_type, 0))
        return false;

    if (NPC2_FLAGGED(ch, NPC2_STAY_SECT) &&
        ch->in_room->sector_type != room->sector_type)
        return false;

    if (!can_see_room(ch, ch->in_room) &&
        (!GET_EQ(ch, WEAR_LIGHT) || GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0)))
        return false;

    return true;
}

void
mobile_spec(void)
{
    extern int no_specials;

    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *ch = cit->data;

        if (!IS_NPC(ch))
            continue;

        if (!ch) {
            errlog("Skipping null mobile in mobile_activity");
            continue;
        }

        if (!ch->in_room) {
            errlog("Skipping mobile in null room");
            continue;
        }

        if (!ch->player.name
            && !ch->player.short_descr
            && !ch->player.description) {
            errlog("Skipping null mobile in mobile_activity");
            continue;
        }

        if (is_dead(ch))
            continue;

        //
        // Check for mob spec
        //
        if (!no_specials
            && NPC_FLAGGED(ch, NPC_SPEC)
            && GET_NPC_WAIT(ch) <= 0
            && !ch->desc) {
            if (ch->mob_specials.shared->func == NULL) {
                zerrlog(ch->in_room->zone,
                    "SPEC bit set with no special: %s (#%d)",
                    GET_NAME(ch), GET_NPC_VNUM(ch));
                REMOVE_BIT(NPC_FLAGS(ch), NPC_SPEC);
            } else {
                if ((ch->mob_specials.shared->func) (ch, ch, 0, tmp_strdup(""), SPECIAL_TICK)) {
                    continue;   /* go to next char */
                }
            }
        }
    }
}

void
single_mobile_activity(struct creature *ch)
{
    struct creature *vict = NULL, *damager = NULL;
    struct obj_data *obj, *best_obj, *i;
    struct affected_type *af_ptr = NULL;
    int dir, max, k;
    struct room_data *room = NULL;
    int cur_class = 0;

    if (!ch) {
        errlog("Skipping null mobile in mobile_activity");
        return;
    }

    if (!ch->in_room) {
        errlog("Skipping mobile in null room");
        return;
    }

    if (!ch->in_room && !ch->player.name && !ch->player.short_descr
        && !ch->player.description) {
        errlog("Skipping null mobile in mobile_activity");
        return;
    }

    if (is_dead(ch))
        return;

    // Non-special mobs in idle zones don't do anything
    if (!production_mode && IS_NPC(ch)
        && ch->in_room->zone->idle_time >= ZONE_IDLE_TIME)
        return;

    // Utility mobs don't do anything
    if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_UTILITY))
        return;

    // poison 2 tick
    if (HAS_POISON_2(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        struct affected_type *af;
        if ((af = affected_by_spell(ch, SPELL_POISON)))
            damager = get_char_in_world_by_idnum(af->owner);
        damage(damager, ch, NULL,
               dice(4, 3) + (affected_by_spell(ch, SPELL_METABOLISM) ? dice(4, 11) : 0),
               SPELL_POISON, -1);
        if (is_dead(ch)) {
            return;
        }
    }
    // Remorts will act like their secondary class 1/3rd of the time
    if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_fractional_3())
        cur_class = GET_REMORT_CLASS(ch);
    else
        cur_class = GET_CLASS(ch);

    // bleed
    if (GET_HIT(ch) &&
        CHAR_HAS_BLOOD(ch) &&
        GET_HIT(ch) < ((GET_MAX_HIT(ch) / 8) +
            random_number_zero_low(MAX(1, GET_MAX_HIT(ch) / 16))))
        add_blood_to_room(ch->in_room, 1);

    // Zen of Motion effect
    if (IS_NEUTRAL(ch) && affected_by_spell(ch, ZEN_MOTION))
        GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch),
            GET_MOVE(ch) +
            random_number_zero_low(MAX(1, CHECK_SKILL(ch, ZEN_MOTION) / 8)));

    // Deplete scuba tanks
    if ((obj = ch->equipment[WEAR_FACE]) &&
        IS_OBJ_TYPE(obj, ITEM_SCUBA_MASK) &&
        !CAR_CLOSED(obj) &&
        obj->aux_obj && IS_OBJ_TYPE(obj->aux_obj, ITEM_SCUBA_TANK) &&
        (GET_OBJ_VAL(obj->aux_obj, 1) > 0 ||
            GET_OBJ_VAL(obj->aux_obj, 0) < 0)) {
        if (GET_OBJ_VAL(obj->aux_obj, 0) > 0) {
            GET_OBJ_VAL(obj->aux_obj, 1)--;
            if (!GET_OBJ_VAL(obj->aux_obj, 1))
                act("A warning indicator reads: $p fully depleted.",
                    false, ch, obj->aux_obj, NULL, TO_CHAR);
            else if (GET_OBJ_VAL(obj->aux_obj, 1) == 5)
                act("A warning indicator reads: $p air level low.",
                    false, ch, obj->aux_obj, NULL, TO_CHAR);
        }
    }

    if (affected_by_spell(ch, SPELL_PETRIFY)) {
        switch (number(0, 3)) {
        case 0:
            send_to_char(ch, "You feel yourself slowing down.\r\n");
            break;
        case 1:
            send_to_char(ch, "Your entire body is beginning to stiffen.\r\n");
            break;
        case 2:
            send_to_char(ch, "Your skin is turning gray.\r\n");
            break;
        case 3:
            send_to_char(ch, "You are slowly turning to stone!\r\n");
            break;
        }
    }

    //
    // nothing below this conditional affects FIGHTING characters
    //
    if (GET_POSITION(ch) == POS_FIGHTING || is_fighting(ch)) {
        return;
    }
    //
    // meditate
    //

    if (IS_NEUTRAL(ch)
        && GET_POSITION(ch) == POS_SITTING && AFF2_FLAGGED(ch, AFF2_MEDITATE))
        perform_monk_meditate(ch);

    //
    // Check if we've gotten knocked down.
    //

    if (IS_NPC(ch) && !NPC2_FLAGGED(ch, NPC2_MOUNT) &&
        !AFF_FLAGGED(ch, AFF_SLEEP) &&
        GET_NPC_WAIT(ch) < 30 &&
        !AFF_FLAGGED(ch, AFF_SLEEP) &&
        GET_POSITION(ch) >= POS_SLEEPING &&
        (GET_DEFAULT_POS(ch) <= POS_STANDING ||
            GET_POSITION(ch) < POS_STANDING) &&
        (GET_POSITION(ch) < GET_DEFAULT_POS(ch))
        && random_fractional_3()) {
        if (GET_POSITION(ch) == POS_SLEEPING)
            act("$n wakes up.", true, ch, NULL, NULL, TO_ROOM);

        switch (GET_DEFAULT_POS(ch)) {
        case POS_SITTING:
            act("$n sits up.", true, ch, NULL, NULL, TO_ROOM);
            GET_POSITION(ch) = POS_SITTING;
            break;
        default:
            if (!AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
                || number(1, 50) < GET_STR(ch)) {
                act("$n stands up.", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_STANDING;
            }
            break;
        }
        return;
    }
    //
    // nothing below this conditional affects characters who are asleep or in a wait state
    //

    if (!AWAKE(ch) || GET_NPC_WAIT(ch) > 0 || CHECK_WAIT(ch))
        return;

    //
    // barbs go BERSERK (berserk)
    //

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && AFF2_FLAGGED(ch, AFF2_BERSERK)
        && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {

        if (perform_barb_berserk(ch, NULL))
            return;
    }
    //
    // drunk effects
    //

    if (GET_COND(ch, DRUNK) > GET_CON(ch) && random_fractional_10()) {
        act("$n burps loudly.", false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You burp loudly.\r\n");
        return;
    } else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 2 && random_fractional_10()) {
        act("$n hiccups.", false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You hiccup.\r\n");
        return;
    }
    //
    // nothing below this conditional affects PCs
    //

    if (!IS_NPC(ch) || ch->desc)
        return;

    /** implicit awake && !fighting **/

    // Attempt to flee from oceans of holy water
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOLYOCEAN)
        && IS_EVIL(ch)
        && GET_POSITION(ch) < POS_FLYING) {
        do_flee(ch, tmp_strdup(""), 0, 0);
    }
    // Attempt to keep from freezing
    if (ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD)
        && !CHAR_WITHSTANDS_COLD(ch)
        && can_cast_spell(ch, SPELL_ENDURE_COLD)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_ENDURE_COLD);
        return;
    }
    // If on fire, try to extinguish the flames
    if (GET_POSITION(ch) >= POS_RESTING
        && AFF2_FLAGGED(ch, AFF2_ABLAZE)
        && !CHAR_WITHSTANDS_FIRE(ch)) {
        do_extinguish(ch, tmp_strdup(""), 0, 0);
        return;
    }

    /** mobiles re-hiding **/
    if (!AFF_FLAGGED(ch, AFF_HIDE)
        && AFF_FLAGGED(NPC_SHARED(ch)->proto, AFF_HIDE))
        SET_BIT(AFF_FLAGS(ch), AFF_HIDE);

    /** mobiles reloading guns **/
    if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
                (IS_GUN(obj) && (count_contained_objs(obj) < MAX_LOAD(obj))) ||
                (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))) {
                mob_reload_gun(ch, obj);
                break;
            }
        }

        if ((obj = GET_EQ(ch, WEAR_WIELD)) &&
            ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
                (IS_GUN(obj) && (count_contained_objs(obj) < MAX_LOAD(obj))) ||
                (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
            mob_reload_gun(ch, obj);
        if ((obj = GET_EQ(ch, WEAR_WIELD_2)) &&
            ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
                (IS_GUN(obj) && (count_contained_objs(obj) < MAX_LOAD(obj))) ||
                (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
            mob_reload_gun(ch, obj);
        if ((obj = GET_IMPLANT(ch, WEAR_WIELD)) &&
            ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
                (IS_GUN(obj) && (count_contained_objs(obj) < MAX_LOAD(obj))) ||
                (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
            mob_reload_gun(ch, obj);
        if ((obj = GET_IMPLANT(ch, WEAR_WIELD_2)) &&
            ((IS_GUN(obj) && !GUN_LOADED(obj)) ||
                (IS_GUN(obj) && (count_contained_objs(obj) < MAX_LOAD(obj))) ||
                (IS_ENERGY_GUN(obj) && !EGUN_CUR_ENERGY(obj))))
            mob_reload_gun(ch, obj);
    }

    /* Mobiles looking at chars */
    if (random_fractional_20()) {
        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            vict = it->data;
            if (vict == ch)
                continue;
            if (can_see_creature(ch, vict) && random_fractional_50()) {
                if ((IS_EVIL(ch) && IS_GOOD(vict))
                    || (IS_GOOD(ch) && IS_EVIL(ch))) {
                    if (GET_LEVEL(ch) < (GET_LEVEL(vict) - 10)) {
                        act("$n looks warily at $N.", false, ch, NULL, vict,
                            TO_NOTVICT);
                        act("$n looks warily at you.", false, ch, NULL, vict,
                            TO_VICT);
                    } else {
                        act("$n growls at $N.", false, ch, NULL, vict,
                            TO_NOTVICT);
                        act("$n growls at you.", false, ch, NULL, vict, TO_VICT);
                    }
                } else if (cur_class == CLASS_PREDATOR) {
                    act("$n growls at $N.", false, ch, NULL, vict, TO_NOTVICT);
                    act("$n growls at you.", false, ch, NULL, vict, TO_VICT);
                } else if (cur_class == CLASS_THIEF) {
                    act("$n glances sidelong at $N.", false, ch, NULL, vict,
                        TO_NOTVICT);
                    act("$n glances sidelong at you.", false, ch, NULL, vict,
                        TO_VICT);
                } else if (((IS_MALE(ch) && IS_FEMALE(vict))
                        || (IS_FEMALE(ch) && IS_MALE(vict)))
                    && random_fractional_4()) {
                    act("$n stares dreamily at $N.", false, ch, NULL, vict,
                        TO_NOTVICT);
                    act("$n stares dreamily at you.", false, ch, NULL, vict,
                        TO_VICT);
                } else if (random_binary()) {
                    act("$n looks at $N.", false, ch, NULL, vict, TO_NOTVICT);
                    act("$n looks at you.", false, ch, NULL, vict, TO_VICT);
                }
                break;
            }
        }
    }

    /* Scavenger (picking up objects) */

    if (NPC_FLAGGED(ch, NPC_SCAVENGER)) {
        if (ch->in_room->contents && random_fractional_10()) {
            max = 1;
            best_obj = NULL;
            for (obj = ch->in_room->contents; obj; obj = obj->next_content)
                if (can_see_object(ch, obj) &&
                    // don't pick up sigil-ized objs if we know better
                    (!GET_OBJ_SIGIL_IDNUM(obj) ||
                        (!AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
                            && !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING)))
                    && CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
                    best_obj = obj;
                    max = GET_OBJ_COST(obj);
                }
            if (best_obj != NULL) {
                strcpy(buf, fname(best_obj->aliases));
                do_get(ch, buf, 0, 0);
            }
        }
    }

    /* Drink from fountains */
    if (!IS_UNDEAD(ch) && GET_RACE(ch) != RACE_DRAGON &&
        GET_RACE(ch) != RACE_GOLEM && GET_RACE(ch) != RACE_ELEMENTAL) {
        if (ch->in_room->contents && random_fractional_100()) {
            for (obj = ch->in_room->contents; obj; obj = obj->next_content)
                if (IS_OBJ_TYPE(obj, ITEM_FOUNTAIN) &&
                    GET_OBJ_VAL(obj, 1) > 0)
                    break;
            if (obj && IS_OBJ_TYPE(obj, ITEM_FOUNTAIN) &&
                GET_OBJ_VAL(obj, 1) > 0) {
                act("$n drinks from $p.", true, ch, obj, NULL, TO_ROOM);
                act("You drink from $p.", false, ch, obj, NULL, TO_CHAR);
            }
            return;
        }
    }

    /* Animals devouring corpses and food */
    if (cur_class == CLASS_PREDATOR) {
        if (ch->in_room->contents && (random_fractional_4()
                || IS_TARRASQUE(ch))) {
            for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
                if (IS_OBJ_TYPE(obj, ITEM_FOOD) && !GET_OBJ_VAL(obj, 3)) {
                    act("$n devours $p, growling and drooling all over.",
                        false, ch, obj, NULL, TO_ROOM);
                    extract_obj(obj);
                    return;
                } else if (IS_OBJ_TYPE(obj, ITEM_CONTAINER) &&
                           GET_OBJ_VAL(obj, 3)) {
                    struct room_data *stuff_rm;

                    if (CORPSE_IDNUM(obj) < 0
                        || ch->in_room->zone->pk_style == ZONE_CHAOTIC_PK) {
                        act("$n devours $p, growling and drooling all over.",
                            false, ch, obj, NULL, TO_ROOM);
                        if (IS_TARRASQUE(ch))
                            stuff_rm = real_room(24919);
                        else
                            stuff_rm = NULL;

                        if (!stuff_rm)
                            stuff_rm = ch->in_room;
                        for (i = obj->contains; i; i = best_obj) {
                            best_obj = i->next_content;
                            if (IS_IMPLANT(i)) {
                                SET_BIT(GET_OBJ_WEAR(i), ITEM_WEAR_TAKE);
                                if (GET_OBJ_DAM(i) > 0)
                                    GET_OBJ_DAM(i) /= 2;
                            }
                            obj_from_obj(i);
                            obj_to_room(i, stuff_rm);

                        }
                        extract_obj(obj);
                        return;
                    } else if (AFF_FLAGGED(ch, AFF_CHARM)
                               && ch->master
                               && ch->master->in_room == ch->in_room) {
                    vict = ch->master;
                    stop_follower(ch);
                    hit(ch, vict, TYPE_UNDEFINED);
                    return;
                    }
                }
            }
        }
    }

    /* Wearing Objects */
    if (!NPC2_FLAGGED(ch, NPC2_WONT_WEAR) && !IS_ANIMAL(ch) &&
        !IS_DRAGON(ch) && !IS_ELEMENTAL(ch)) {
        if (ch->carrying && random_fractional_4()) {
            for (obj = ch->carrying; obj; obj = best_obj) {
                best_obj = obj->next_content;
                if (ANTI_ALIGN_OBJ(ch, obj)
                    && (random_number_zero_low(15) + 6) < GET_INT(ch)
                    && !IS_NODROP(obj)) {
                    // Check to see if the obj is renamed Or Imm Enchanted
                    // Log it if it is.
                    struct obj_data *original =
                        real_object_proto(GET_OBJ_VNUM(obj));
                    int renamed = 0;
                    if (original)
                        renamed = strcmp(obj->name, original->name);
                    if (renamed || isname_exact("imm", obj->aliases)) {
                        mudlog(LVL_IMMORT, CMP, true,
                            "%s [%d] junked by %s at %s [%d]. (%s %s)",
                            obj->name, GET_OBJ_VNUM(obj),
                            GET_NAME(ch), ch->in_room->name,
                            ch->in_room->number, renamed ? "R3nAm3" : "",
                            isname_exact("imm",
                                obj->aliases) ? "|mM3nChAnT" : "");
                    }
                    act("$n junks $p.", true, ch, obj, NULL, TO_ROOM);
                    extract_obj(obj);
                    break;
                }
                if (can_see_object(ch, obj) &&
                    !IS_IMPLANT(obj) &&
                    (IS_OBJ_TYPE(obj, ITEM_WEAPON) ||
                        IS_ENERGY_GUN(obj) || IS_GUN(obj)) &&
                    !invalid_char_class(ch, obj) &&
                    (!GET_EQ(ch, WEAR_WIELD) ||
                        !IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD),
                            ITEM2_NOREMOVE))) {
                    if (GET_EQ(ch, WEAR_WIELD)
                        && (GET_OBJ_WEIGHT(obj) <=
                            strength_wield_weight(GET_STR(ch)))
                        && GET_OBJ_COST(obj) > GET_OBJ_COST(GET_EQ(ch,
                                WEAR_WIELD))) {
                        strcpy(buf, fname(obj->aliases));
                        do_remove(ch, buf, 0, 0);
                    }
                    if (!GET_EQ(ch, WEAR_WIELD)) {
                        do_wield(ch, fname(obj->aliases), 0, 0);
                        if (IS_GUN(obj) && GET_NPC_VNUM(ch) == 1516)
                            perform_say(ch, "say", "Let's Rock.");
                    }
                }
            }

            //
            // "wear all" can cause the death of ch, but we don't care since we will continue
            // immediately afterwards
            //

            do_wear(ch, tmp_strdup("all"), 0, 0);
            return;
        }
    }

    /* Looter */

    if (NPC2_FLAGGED(ch, NPC2_LOOTER)) {
        struct obj_data *o = NULL;
        if (ch->in_room->contents && random_fractional_3()) {
            for (i = ch->in_room->contents; i; i = i->next_content) {
                if ((IS_OBJ_TYPE(i, ITEM_CONTAINER) && GET_OBJ_VAL(i, 3))) {
                    if (!i->contains)
                        continue;
                    for (obj = i->contains; obj; obj = obj->next_content) {
                        if (o) {
                            extract_obj(o);
                            o = NULL;
                        }
                        if (!(obj->in_obj))
                            continue;

                        // skip sigil-ized items, simplest way to deal with it
                        if (GET_OBJ_SIGIL_IDNUM(obj))
                            continue;

                        if (CAN_GET_OBJ(ch, obj)) {
                            obj_from_obj(obj);
                            obj_to_char(obj, ch);
                            act("$n gets $p from $P.", false, ch, obj, i,
                                TO_ROOM);
                            if (IS_OBJ_TYPE(obj, ITEM_MONEY)) {
                                if (GET_OBJ_VAL(obj, 1))    // credits
                                    GET_CASH(ch) += GET_OBJ_VAL(obj, 0);
                                else
                                    GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
                                o = obj;
                            }
                        }
                    }
                    if (o) {
                        extract_obj(o);
                        o = NULL;
                    }
                }
            }
        }
    }

    /* Helper Mobs */
    if (NPC_FLAGGED(ch, NPC_HELPER) && random_binary()) {
        vict = NULL;

        if (AFF_FLAGGED(ch, AFF_CHARM))
            return;

        if (AFF_FLAGGED(ch, AFF_BLIND))
            return;

        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            vict = it->data;
            if (ch != vict
                && is_fighting(vict)
                && !g_list_find(vict->fighting, ch)
                && can_see_creature(ch, vict)) {

                int fvict_help_prob =
                    helper_help_probability(ch, random_opponent(vict));
                int fvict_attack_prob =
                    helper_attack_probability(ch, random_opponent(vict));

                int vict_help_prob = helper_help_probability(ch, vict);
                int vict_attack_prob = helper_attack_probability(ch, vict);

                //
                // if we're not willing to help anybody here, then just continue
                //

                if (fvict_help_prob <= 0 && fvict_attack_prob <= 0)
                    return;
                if (vict_help_prob <= 0 && vict_attack_prob <= 0)
                    return;

                int fvict_composite_prob = fvict_help_prob + vict_attack_prob;
                int vict_composite_prob = vict_help_prob + fvict_attack_prob;

                //
                // attack vict, helping fvict
                //

                if (fvict_composite_prob > vict_composite_prob &&
                    fvict_help_prob > 0 && vict_attack_prob > 0) {

                    //
                    // store a pointer past next_ch if next_ch _happens_ to be vict
                    //

                    (void)helper_assist(ch, vict, random_opponent(vict));
                    return;
                }
                //
                // attack fvict, helping fict
                //

                else if (vict_composite_prob > fvict_composite_prob &&
                    vict_help_prob > 0 && fvict_attack_prob > 0) {
                    //
                    // store a pointer past next_ch if next_ch _happens_ to be fvict
                    //

                    (void)helper_assist(ch, random_opponent(vict), vict);
                    return;
                }

            }
            // continue
        }
    }

    /*Racially aggressive Mobs */

    if (IS_RACIALLY_AGGRO(ch) &&
        !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && random_fractional_4()) {
        vict = NULL;
        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            vict = it->data;
            if ((IS_NPC(vict) && !NPC2_FLAGGED(ch, NPC2_ATK_MOBS))
                || !can_see_creature(ch, vict)
                || PRF_FLAGGED(vict, PRF_NOHASSLE))
                continue;

            if (check_infiltrate(vict, ch))
                continue;

            if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
                continue;

            if (GET_MORALE(ch) + GET_LEVEL(ch) <
                number(GET_LEVEL(vict), (GET_LEVEL(vict) * 4) +
                    (MAX(1, (GET_HIT(vict)) * GET_LEVEL(vict))
                        / MAX(1, GET_MAX_HIT(vict)))) && AWAKE(vict))
                continue;
            else if (RACIAL_ATTACK(ch, vict) &&
                (!IS_NPC(vict) || NPC2_FLAGGED(ch, NPC2_ATK_MOBS))) {
                best_initial_attack(ch, vict);
                return;
            }
        }
    }

    /* Aggressive Mobs */

    if ((NPC_FLAGGED(ch, NPC_AGGRESSIVE)
            || NPC_FLAGGED(ch, NPC_AGGR_TO_ALIGN)) &&
        !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {

        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            vict = it->data;
            if (vict == ch || (!next_living(it) && random_fractional_4())
                || (!IS_NPC(vict) && !vict->desc)) {
                continue;
            }
            if (check_infiltrate(vict, ch))
                continue;

            if ((IS_NPC(vict) && !NPC2_FLAGGED(ch, NPC2_ATK_MOBS))
                || !can_see_creature(ch, vict)
                || PRF_FLAGGED(vict, PRF_NOHASSLE)) {
                continue;
            }
            if (AWAKE(vict) &&
                GET_MORALE(ch) <
                (random_number_zero_low(GET_LEVEL(vict) * 2) + 35 +
                    (GET_LEVEL(vict) / 2)))
                continue;

            if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master
                && ch->master == vict
                && (GET_CHA(vict) + GET_INT(vict)) >
                GET_INT(ch) + !random_fractional_20())
                continue;

            if (NPC2_FLAGGED(ch, NPC2_NOAGGRO_RACE) &&
                GET_RACE(ch) == GET_RACE(vict))
                continue;

            if (!NPC_FLAGGED(ch, NPC_AGGRESSIVE))
                if ((IS_EVIL(vict) && !NPC_FLAGGED(ch, NPC_AGGR_EVIL)) ||
                    (IS_GOOD(vict) && !NPC_FLAGGED(ch, NPC_AGGR_GOOD)) ||
                    (IS_NEUTRAL(vict)
                        && !NPC_FLAGGED(ch, NPC_AGGR_NEUTRAL)))
                    continue;

            if (IS_ANIMAL(ch) && affected_by_spell(vict, SPELL_ANIMAL_KIN))
                continue;

            best_initial_attack(ch, vict);
            return;
        }

        // end for vict

        /** scan surrounding rooms **/
        if (!NPC_HUNTING(ch) && GET_POSITION(ch) > POS_FIGHTING &&
            !NPC_FLAGGED(ch, NPC_SENTINEL) &&
            (GET_LEVEL(ch) + GET_MORALE(ch) >
                (random_number_zero_low(120) + 50)
                || IS_TARRASQUE(ch))) {
            for (dir = 0; dir < NUM_DIRS; dir++) {
                if (!CAN_GO(ch, dir))
                    continue;
                struct room_data *tmp_room = EXIT(ch, dir)->to_room;
                if (!ROOM_FLAGGED(tmp_room,
                        ROOM_DEATH | ROOM_NOMOB | ROOM_PEACEFUL)
                    && tmp_room != ch->in_room && CHAR_LIKES_ROOM(ch, tmp_room)
                    && tmp_room->people
                    && can_see_creature(ch, tmp_room->people->data)
                    && will_fit_in_room(ch, tmp_room)) {
                    break;
                }
            }

            if (dir < NUM_DIRS) {
                vict = NULL;
                struct room_data *tmp_room = EXIT(ch, dir)->to_room;
                bool found = false;
                for (GList * it = first_living(tmp_room->people); it && !found;
                    it = next_living(it)) {
                    vict = it->data;
                    if (can_see_creature(ch, vict)
                        && !PRF_FLAGGED(vict, PRF_NOHASSLE)
                        && (!AFF_FLAGGED(vict, AFF_SNEAK)
                            || AFF_FLAGGED(vict, AFF_GLOWLIGHT)
                            || AFF_FLAGGED(vict,
                                AFF2_FLUORESCENT | AFF2_DIVINE_ILLUMINATION))
                        && (NPC_FLAGGED(ch, NPC_AGGRESSIVE)
                            || (NPC_FLAGGED(ch, NPC_AGGR_EVIL)
                                && IS_EVIL(vict))
                            || (NPC_FLAGGED(ch, NPC_AGGR_GOOD)
                                && IS_GOOD(vict))
                            || (NPC_FLAGGED(ch, NPC_AGGR_NEUTRAL)
                                && IS_NEUTRAL(vict)))
                        && (!NPC2_FLAGGED(ch, NPC2_NOAGGRO_RACE)
                            || GET_RACE(ch) != GET_RACE(vict))
                        && (!IS_NPC(vict)
                            || NPC2_FLAGGED(ch, NPC2_ATK_MOBS))) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (can_cast_spell(ch, SKILL_PSIDRAIN) && can_psidrain(ch, vict, NULL, false)) {
                        perform_psidrain(ch, vict);
                    } else {
                        perform_move(ch, dir, MOVE_NORM, 1);
                    }
                    return;
                }
            }
        }
    }

    /* Mob Memory */

    if (NPC_FLAGGED(ch, NPC_MEMORY) && MEMORY(ch) &&
        !AFF_FLAGGED(ch, AFF_CHARM)) {
        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            vict = it->data;

            if (check_infiltrate(vict, ch))
                continue;

            if ((IS_NPC(vict) && !NPC2_FLAGGED(ch, NPC2_ATK_MOBS)) ||
                !can_see_creature(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE)
                || ((af_ptr = affected_by_spell(vict, SKILL_DISGUISE))
                    && !CAN_DETECT_DISGUISE(ch, vict, af_ptr->duration)))
                continue;
            if (char_in_memory(vict, ch)) {
                if (!random_number_zero_low(4)) {
                    emit_voice(ch, vict, VOICE_TAUNTING);
                } else if (AWAKE(vict)
                           && !IS_UNDEAD(ch)
                           && !IS_DRAGON(ch)
                           && !IS_DEVIL(ch)
                           && GET_HIT(ch) > GET_HIT(vict)
                           && ((GET_LEVEL(vict)
                                + ((50 * GET_HIT(vict))
                                   / MAX(1, GET_MAX_HIT(vict)))) >
                               GET_LEVEL(ch) + (GET_MORALE(ch) / 2)
                               + random_percentage())) {
                    emit_voice(ch, vict, VOICE_PANICKING);
                    do_flee(ch, tmp_strdup(""), 0, 0);
                    return;
                } else {
                    emit_voice(ch, vict, VOICE_ATTACKING);
                    best_initial_attack(ch, vict);
                    return;
                }
            }
        }
    }

    /* Mob Movement -- Lair */
    if (GET_NPC_LAIR(ch) > 0 && ch->in_room->number != GET_NPC_LAIR(ch) &&
        !NPC_HUNTING(ch) &&
        (room = real_room(GET_NPC_LAIR(ch))) &&
        ((dir = find_first_step(ch->in_room, room, STD_TRACK)) >= 0) &&
        NPC_CAN_GO(ch, dir) &&
        !ROOM_FLAGGED(ch->in_room->dir_option[dir]->to_room,
            ROOM_NOMOB | ROOM_DEATH)) {
        smart_mobile_move(ch, dir);
        return;
    }
    // mob movement -- follow the leader

    if (GET_NPC_LEADER(ch) > 0 && ch->master &&
        ch->in_room != ch->master->in_room) {
        if (smart_mobile_move(ch, find_first_step(ch->in_room,
                    ch->master->in_room, STD_TRACK)))
            return;
    }

    /* Mob Movement */
    if (!NPC_FLAGGED(ch, NPC_SENTINEL)
        && !((NPC_FLAGGED(ch, NPC_PET) || NPC2_FLAGGED(ch, NPC2_FAMILIAR))
            && ch->master)
        && GET_POSITION(ch) >= POS_STANDING && !AFF2_FLAGGED(ch, AFF2_MOUNTED)) {

        int door;

        if (IS_TARRASQUE(ch) || g_list_length(ch->in_room->people) > 10)
            door = random_number_zero_low(NUM_OF_DIRS - 1);
        else
            door = random_number_zero_low(20);

        if ((door < NUM_OF_DIRS) &&
            (NPC_CAN_GO(ch, door)) &&
            (rev_dir[door] != ch->mob_specials.last_direction ||
                count_room_exits(ch->in_room) < 2 || random_binary())
            && (EXIT(ch, door)->to_room != ch->in_room)
            && (!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH)
                && !IS_SET(EXIT(ch, door)->exit_info, EX_NOMOB))
            && (CHAR_LIKES_ROOM(ch, EXIT(ch, door)->to_room))
            && (!NPC2_FLAGGED(ch, NPC2_STAY_SECT)
                || (EXIT(ch, door)->to_room->sector_type ==
                    ch->in_room->sector_type))
            && (!NPC_FLAGGED(ch, NPC_STAY_ZONE)
                || (EXIT(ch, door)->to_room->zone == ch->in_room->zone))
            && g_list_length(EXIT(ch, door)->to_room->people) < 10) {
            if (perform_move(ch, door, MOVE_NORM, 1))
                return;
        }
    }
    //
    // thief tries to steal from others
    //

    if (cur_class == CLASS_THIEF && random_binary()) {
        if (thief(ch, ch, 0, tmp_strdup(""), SPECIAL_TICK))
            return;
    }
    //
    // clerics spell up
    //

    else if (cur_class == CLASS_CLERIC && random_binary()) {
        if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
            if (can_cast_spell(ch, SPELL_HEAL))
                cast_spell(ch, ch, NULL, NULL, SPELL_HEAL);
            else if (can_cast_spell(ch, SPELL_CURE_CRITIC))
                cast_spell(ch, ch, NULL, NULL, SPELL_CURE_CRITIC);
            else if (can_cast_spell(ch, SPELL_CURE_LIGHT))
                cast_spell(ch, ch, NULL, NULL, SPELL_CURE_LIGHT);
        } else if (room_is_dark(ch->in_room) &&
                   !has_dark_sight(ch) && can_cast_spell(ch, SPELL_DIVINE_ILLUMINATION)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_DIVINE_ILLUMINATION);
        } else if ((affected_by_spell(ch, SPELL_BLINDNESS) ||
                    affected_by_spell(ch, SKILL_GOUGE))
                   && can_cast_spell(ch, SPELL_CURE_BLIND)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_BLIND);
        } else if (AFF_FLAGGED(ch, AFF_POISON) && can_cast_spell(ch, SPELL_REMOVE_POISON)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_REMOVE_POISON);
        } else if (AFF_FLAGGED(ch, AFF_CURSE) && can_cast_spell(ch, SPELL_REMOVE_CURSE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_REMOVE_CURSE);
        } else if (GET_MANA(ch) > (GET_MAX_MANA(ch) * 0.75)
                   && can_cast_spell(ch, SPELL_SANCTUARY)
                   && !AFF_FLAGGED(ch, AFF_SANCTUARY)
                   && !AFF_FLAGGED(ch, AFF_NOPAIN)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_SANCTUARY);
        } else if (GET_EQ(ch, WEAR_WIELD)
                   && IS_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD), ITEM_WEAPON)
                   && IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_ABLAZE)
                   && can_cast_spell(ch, SPELL_FLAME_OF_FAITH)) {
            cast_spell(ch, NULL, GET_EQ(ch, WEAR_WIELD), NULL, SPELL_FLAME_OF_FAITH);
        }
    }
    //
    // knights spell up
    //

    else if (cur_class == CLASS_KNIGHT && random_binary()) {
        knight_activity(ch);
    }
    //
    // ranger spell up
    //

    else if (cur_class == CLASS_RANGER && random_binary()) {
        ranger_activity(ch);
    }
    //
    // cyborg spell up
    //

    else if (cur_class == CLASS_CYBORG && random_binary()) {
        if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80 && GET_MOVE(ch) > 100) {
            do_repair(ch, tmp_strdup(""), 0, 0);
        }
        if (!random_percentage_zero_low()) {
            if (GET_LEVEL(ch) > 11) {
                if (!affected_by_spell(ch, SKILL_DAMAGE_CONTROL)) {
                    if (GET_LEVEL(ch) >= 12)
                        do_activate(ch, tmp_strdup("damage control"), 0, 1); // 12
                    if (GET_LEVEL(ch) >= 17
                        && GET_REMORT_CLASS(ch) != CLASS_UNDEFINED)
                        do_activate(ch, tmp_strdup("radionegation"), 0, 1);  // 17
                    if (GET_LEVEL(ch) >= 18)
                        do_activate(ch, tmp_strdup("power boost"), 0, 1);    // 18
                    do_activate(ch, tmp_strdup("reflex boost"), 0, 1);   // 18
                    if (GET_LEVEL(ch) >= 24)
                        do_activate(ch, tmp_strdup("melee weapons tactics"), 0, 1);  // 24
                    if (GET_LEVEL(ch) >= 32)
                        do_activate(ch, tmp_strdup("energy field"), 0, 1);   // 32
                    if (GET_LEVEL(ch) >= 37)
                        do_activate(ch, tmp_strdup("shukutei adrenal maximizations"), 1, 0); // 37
                }
            }
        }
    }
    //
    // mage spell up
    //

    else if (cur_class == CLASS_MAGE
        && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
        && random_binary()) {
        mage_activity(ch);
    }
    //
    // psionic spell up
    //

    else if (cur_class == CLASS_PSIONIC
        && !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)
        && random_binary()) {
        psionic_activity(ch);
    }
    //
    // barbs acting barbaric
    //

    else if (cur_class == CLASS_BARB || cur_class == CLASS_HILL
        || GET_RACE(ch) == RACE_DEMON) {
        barbarian_activity(ch);
    }

    else if (GET_RACE(ch) == RACE_CELESTIAL || GET_RACE(ch) == RACE_ARCHON
        || GET_RACE(ch) == RACE_DEVIL) {
        knight_activity(ch);
    }

    else if (GET_RACE(ch) == RACE_GUARDINAL) {
        ranger_activity(ch);
    }
    //
    // elementals heading home
    //

    if (GET_RACE(ch) == RACE_ELEMENTAL) {
        if (((GET_NPC_VNUM(ch) >= 1280 && GET_NPC_VNUM(ch) <= 1283) ||
                GET_NPC_VNUM(ch) == 5318) &&
            (!ch->master || ch->master->in_room->zone != ch->in_room->zone))
            k = 1;
        else
            k = 0;

        switch (GET_CLASS(ch)) {
        case CLASS_EARTH:
            if (k || room_is_watery(ch->in_room)
                || room_is_open_air(ch->in_room)) {
                act("$n dissolves, and returns to $s home plane!", true, ch, NULL,
                    NULL, TO_ROOM);
                creature_purge(ch, true);
                return;
            }
            break;
        case CLASS_FIRE:
            if ((k || room_is_watery(ch->in_room)
                    || room_is_open_air(ch->in_room)
                    || (OUTSIDE(ch)
                        && ch->in_room->zone->weather->sky == SKY_RAINING))
                && !ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)) {
                act("$n dissipates, and returns to $s home plane!",
                    true, ch, NULL, NULL, TO_ROOM);
                creature_purge(ch, true);
                return;
            }
            break;
        case CLASS_WATER:
            if (k || !room_is_watery(ch->in_room)) {
                act("$n dissipates, and returns to $s home plane!",
                    true, ch, NULL, NULL, TO_ROOM);
                creature_purge(ch, true);
                return;
            }
            break;
        case CLASS_AIR:
            if (k && !room_has_air(ch->in_room)) {
                act("$n dissipates, and returns to $s home plane!",
                    true, ch, NULL, NULL, TO_ROOM);
                creature_purge(ch, true);
                return;
            }
            break;
        default:
            if (k) {
                act("$n disappears.", true, ch, NULL, NULL, TO_ROOM);
                creature_purge(ch, true);
                return;
            }
        }
    }
    //
    // unholy stalker's job finished
    //

    if (GET_NPC_VNUM(ch) == UNHOLY_STALKER_VNUM) {
        if (!NPC_HUNTING(ch)) {
            act("$n dematerializes, removing the chill from the air.",
                true, ch, NULL, NULL, TO_ROOM);
            creature_purge(ch, true);
            return;
        }
    }
    //
    // birds fluttering around
    //

    if (GET_RACE(ch) == RACE_ANIMAL &&
        GET_CLASS(ch) == CLASS_BIRD &&
        AFF_FLAGGED(ch, AFF_INFLIGHT) &&
        !room_is_open_air(ch->in_room) && random_fractional_3()) {
        if (GET_CLASS(ch) == CLASS_BIRD && AFF_FLAGGED(ch, AFF_INFLIGHT) &&
            !room_is_open_air(ch->in_room)) {
            if (GET_POSITION(ch) == POS_FLYING && random_fractional_5()) {
                act("$n flutters to the ground.", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_STANDING;
            } else if (GET_POSITION(ch) == POS_STANDING
                && !AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)) {
                act("$n flaps $s wings and takes flight.", true, ch, NULL, NULL,
                    TO_ROOM);
                GET_POSITION(ch) = POS_FLYING;
            }
            return;
        }
    }
    //
    // non-birds flying around
    //

    if (AFF_FLAGGED(ch, AFF_INFLIGHT) && random_fractional_10()) {
        if (!room_is_open_air(ch->in_room)) {
            if (GET_POSITION(ch) == POS_FLYING && random_fractional_10()) {
                GET_POSITION(ch) = POS_STANDING;
                if (!can_travel_sector(ch, ch->in_room->sector_type, 1)) {
                    GET_POSITION(ch) = POS_FLYING;
                    return;
                } else if (FLOW_TYPE(ch->in_room) != F_TYPE_SINKING_SWAMP)
                    act("$n settles to the ground.", true, ch, NULL, NULL, TO_ROOM);

            } else if (GET_POSITION(ch) == POS_STANDING
                && random_fractional_4()) {

                act("$n begins to hover in midair.", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_FLYING;
            }
            return;
        }
    }
    /* Add new mobile actions here */
    /* end for() */
}

void
mobile_activity(void)
{
    g_list_foreach(creatures, (GFunc) single_mobile_activity, NULL);
}

bool
same_align(struct creature *a, struct creature *b)
{
    return ((IS_GOOD(a) && IS_GOOD(b)) || (IS_EVIL(a) && IS_EVIL(b)));
}

struct creature *
choose_opponent(struct creature *ch, struct creature *ignore_vict)
{

    struct creature *vict = NULL;
    struct creature *best_vict = NULL;

    // first look for someone who is fighting us
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        vict = it->data;

        // ignore bystanders
        if (!g_list_find(vict->fighting, ch))
            continue;

        //
        // ignore the specified ignore_vict
        //

        if (vict == ignore_vict)
            continue;

        // look for opposite/neutral aligns only
        if (same_align(ch, vict))
            continue;

        // pick the weakest victim in hopes of killing one
        if (!best_vict || GET_HIT(vict) < GET_HIT(best_vict)) {
            best_vict = vict;
            continue;
        }
        // if victim's AC is worse, prefer to attack him
        if (!best_vict || GET_AC(vict) > GET_AC(best_vict)) {
            best_vict = vict;
            continue;
        }
    }

    // default to fighting opponent
    if (!best_vict)
        best_vict = random_opponent(ch);

    return (best_vict);
}

bool
detect_opponent_master(struct creature * ch, struct creature * opp)
{
    if (!g_list_find(opp->fighting, ch))
        return false;
    if (IS_PC(ch) || IS_PC(opp))
        return false;
    if (!opp->master)
        return false;
    if (ch->master == opp->master)
        return false;
    if (GET_NPC_WAIT(ch) >= 10)
        return false;
    if (!can_see_creature(ch, opp))
        return false;
    if (opp->master->in_room != ch->in_room)
        return false;
    if (number(0, 50 - GET_INT(ch) - GET_WIS(ch)))
        return false;

    remove_combat(ch, opp);
    add_combat(ch, opp->master, false);
    add_combat(opp->master, ch, false);
    return true;
}

//
// return value like damage()
// if DAM_VICT_KILLED is set, then the victim was FIGHTING(ch)
//
// IMPORTANT: mobile_battle_activity can kill other chars than just FIGHTING(ch)!!
// use precious_vict if you must protect one in a loop or something
//
// TODO: finish implementing return values
//

int
mobile_battle_activity(struct creature *ch, struct creature *precious_vict)
{

    struct creature *vict = random_opponent(ch);
    int prob = 0, dam = 0;
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD), *gun = NULL;

    ACMD(do_disarm);
    ACMD(do_feign);

    if (!is_fighting(ch)) {
        errlog("mobile_battle_activity called on non-fighting ch.");
        raise(SIGSEGV);
    }

    if (g_list_find(ch->fighting, precious_vict)) {
        errlog("FIGHTING(ch) == precious_vict in mobile_battle_activity()");
        return 0;
    }

    if (IS_TARRASQUE(ch)) {     /* tarrasque */
        tarrasque_fight(ch);
        return 0;
    }

    if (!IS_ANIMAL(ch)) {
        // Speaking mobiles
        if (GET_HIT(ch) < GET_MAX_HIT(ch) / 128 && random_fractional_20()) {
            emit_voice(ch, vict, VOICE_FIGHT_LOSING);
        } else if (vict
            && GET_HIT(vict) < GET_MAX_HIT(vict) / 128
            && GET_HIT(ch) > GET_MAX_HIT(ch) / 16 && random_fractional_20()) {
            emit_voice(ch, vict, VOICE_FIGHT_WINNING);
        }
    }

    for (GList * li = first_living(ch->fighting); li; li = next_living(li))
        if (detect_opponent_master(ch, li->data))
            return 0;

    /* Here go the fighting Routines */

    if (CHECK_SKILL(ch, SKILL_SHOOT) + random_number_zero_low(10) > 40) {

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

            if (is_fighting(ch))
                do_shoot(ch, tmp_sprintf("%s %s",
                        fname(gun->aliases), random_opponent(ch)->player.name),
                    0, 0);
            return 0;
        }
    }

    if (GET_CLASS(ch) == CLASS_SNAKE) {
        if (vict && vict->in_room == ch->in_room &&
            (random_number_zero_low(42 - GET_LEVEL(ch)) == 0)) {
            act("$n bites $N!", 1, ch, NULL, vict, TO_NOTVICT);
            act("$n bites you!", 1, ch, NULL, vict, TO_VICT);
            if (random_fractional_10()) {
                call_magic(ch, vict, NULL, NULL, SPELL_POISON, GET_LEVEL(ch),
                    CAST_CHEM);
            }
            return 0;
        }
    }

    /** RACIAL ATTACKS FIRST **/

    // wemics 'leap' out of battle, only to return via the hunt for the kill
    if (IS_RACE(ch, RACE_WEMIC) && GET_MOVE(ch) > (GET_MAX_MOVE(ch) / 2)) {

        // leap
        if (random_fractional_3()) {
            if (GET_HIT(ch) > (GET_MAX_HIT(ch) / 2) &&
                !ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK)) {

                for (int dir = 0; dir < NUM_DIRS; ++dir) {
                    if (CAN_GO(ch, dir) &&
                        !ROOM_FLAGGED(EXIT(ch, dir)->to_room,
                            ROOM_DEATH | ROOM_NOMOB |
                            ROOM_NOTRACK | ROOM_PEACEFUL) &&
                        EXIT(ch, dir)->to_room != ch->in_room &&
                        CHAR_LIKES_ROOM(ch, EXIT(ch, dir)->to_room)
                        && random_binary()) {

                        struct creature *was_fighting = random_opponent(ch);

                        remove_all_combat(ch);

                        for (GList * ci = ch->in_room->people; ci;
                            ci = ci->next) {
                            remove_combat(ci->data, ch);
                        }

                        //
                        // leap in this direction
                        //
                        act(tmp_sprintf("$n leaps over you to the %s!",
                                dirs[dir]), false, ch, NULL, NULL, TO_ROOM);

                        struct room_data *to_room = EXIT(ch, dir)->to_room;
                        char_from_room(ch, false);
                        char_to_room(ch, to_room, false);

                        act(tmp_sprintf("$n leaps in from %s!",
                                from_dirs[dir]), false, ch, NULL, NULL, TO_ROOM);

                        if (was_fighting)
                            start_hunting(ch, was_fighting);
                        return 0;
                    }
                }
            }
        }
        // paralyzing scream

        else if (random_binary()) {
            act("$n releases a deafening scream!!", false, ch, NULL, NULL, TO_ROOM);
            GList *tmp_list = g_list_copy(ch->fighting);
            for (GList * li = first_living(tmp_list); li; li = next_living(li->next)) {
                call_magic(ch, li->data, NULL, NULL, SPELL_FEAR, GET_LEVEL(ch),
                    CAST_BREATH);
            }
            g_list_free(tmp_list);
            return 0;
        }
    }

    if (IS_RACE(ch, RACE_UMBER_HULK)) {
        if (vict && random_fractional_3() && !AFF_FLAGGED(vict, AFF_CONFUSION)) {
            call_magic(ch, vict, NULL, NULL, SPELL_CONFUSION, GET_LEVEL(ch),
                CAST_ROD);
            return 0;
        }
    }

    if (IS_RACE(ch, RACE_DAEMON)) {
        if (GET_CLASS(ch) == CLASS_DAEMON_PYRO) {
            if (vict && random_fractional_3()) {
                WAIT_STATE(ch, 3 RL_SEC);
                call_magic(ch, random_opponent(ch), NULL, NULL, SPELL_FIRE_BREATH,
                    GET_LEVEL(ch), CAST_BREATH);
                return 0;
            }
        }
    }

    if (IS_RACE(ch, RACE_MEPHIT)) {
        if (GET_CLASS(ch) == CLASS_MEPHIT_LAVA) {
            if (vict && random_binary()) {
                return damage(ch, random_opponent(ch), NULL,
                    dice(20, 20), TYPE_LAVA_BREATH, WEAR_RANDOM);
            }
        }
    }

    if (IS_MANTICORE(ch)) {
        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        dam = dice(GET_LEVEL(ch), 5);
        if (GET_LEVEL(ch) + random_number_zero_low(50) >
            GET_LEVEL(vict) - GET_AC(ch) / 2)
            dam = 0;

        WAIT_STATE(ch, 2 RL_SEC);
        return damage(ch, vict, NULL, dam, SPELL_MANTICORE_SPIKES, WEAR_RANDOM);
    }

    /* Devils: Denezens of the 9 hells */
    /* Devils act like knights when not acting like devils */
    if (IS_DEVIL(ch)) {
        if (random_number_zero_low(8) < 1) {
            if (knight_battle_activity(ch, precious_vict)) {
                return 0;
            }
        } else if (mob_fight_devil(ch, precious_vict)) {
            return 0;
        }
    }

    /* Celestials and archons:  Denezens of the  7 heavens */
    /* Archons act like knights unless otherwize told */
    if (GET_RACE(ch) == RACE_CELESTIAL || GET_RACE(ch) == RACE_ARCHON) {
        if (random_number_zero_low(8) < 1) {
            if (knight_battle_activity(ch, precious_vict)) {
                return 0;
            }
        } else if (mob_fight_celestial(ch, precious_vict)) {
            return 0;
        }
    }

    /* Guardinal : Denezins of the Elysian Planes */
    /* Guardinal's act like rangers unless otherwise told */
    if (GET_RACE(ch) == RACE_GUARDINAL) {
        if (random_number_zero_low(8) < 1) {
            if (ranger_battle_activity(ch, precious_vict)) {
                return 0;
            }
        } else if (mob_fight_guardinal(ch, precious_vict)) {
            return 0;
        }
    }

    if (GET_RACE(ch) == RACE_DEMON) {
        if (random_number_zero_low(8) < 1) {
            if (barbarian_battle_activity(ch, precious_vict)) {
                return 0;
            }
        } else if (mob_fight_demon(ch, precious_vict)) {
            return 0;
        }
    }

    /* Slaad */
    if (IS_SLAAD(ch)) {
        return mob_fight_slaad(ch, precious_vict);
    }

    if (IS_TROG(ch)) {
        if (random_fractional_5()) {
            act("$n begins to secrete a disgustingly malodorous oil!",
                false, ch, NULL, NULL, TO_ROOM);

            for (GList * it = ch->in_room->people; it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (!IS_TROG(tch)
                    && !IS_UNDEAD(tch)
                    && !IS_ELEMENTAL(tch)
                    && !IS_DRAGON(tch)) {
                    call_magic(ch, tch, NULL, NULL, SPELL_TROG_STENCH,
                        GET_LEVEL(ch), CAST_POTION);
                }
            }
            return 0;
        }
    }

    if (GET_CLASS(ch) == CLASS_GHOUL) {
        if (random_fractional_10())
            act(" $n emits a terrible shriek!!", false, ch, NULL, NULL, TO_ROOM);
        else if (random_fractional_5()) {
            call_magic(ch, vict, NULL, NULL, SPELL_CHILL_TOUCH, GET_LEVEL(ch),
                CAST_SPELL);
            return 0;
        }
    }

    if (IS_DRAGON(ch)) {
        if (random_number_zero_low(GET_LEVEL(ch)) > 10) {
            act("You feel a wave of sheer terror wash over you as $n approaches!", false, ch, NULL, NULL, TO_ROOM);

            GList *tmp_list = g_list_copy(ch->fighting);
            for (GList *it = first_living(tmp_list); it; it = next_living(it)) {
                vict = it->data;
                if (!mag_savingthrow(vict, GET_LEVEL(ch), SAVING_SPELL) &&
                    !AFF_FLAGGED(vict, AFF_CONFIDENCE)) {
                    do_flee(vict, tmp_strdup(""), 0, 0);
                }
            }
            g_list_free(tmp_list);
            return 0;
        }

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        if (random_fractional_4()) {
            damage(ch, vict, NULL, random_number_zero_low(GET_LEVEL(ch)), TYPE_CLAW,
                WEAR_RANDOM);
            return 0;
        } else if (random_fractional_4() && g_list_find(ch->fighting, vict)) {
            damage(ch, vict, NULL, random_number_zero_low(GET_LEVEL(ch)), TYPE_CLAW,
                WEAR_RANDOM);
            return 0;
        } else if (random_fractional_3()) {
            damage(ch, vict, NULL, random_number_zero_low(GET_LEVEL(ch)), TYPE_BITE,
                WEAR_RANDOM);
            WAIT_STATE(ch, PULSE_VIOLENCE);
            return 0;
        } else if (random_fractional_3()) {
            switch (GET_CLASS(ch)) {
            case CLASS_GREEN:
                call_magic(ch, vict, NULL, NULL, SPELL_GAS_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;

            case CLASS_BLACK:
                call_magic(ch, vict, NULL, NULL, SPELL_ACID_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            case CLASS_BLUE:
                call_magic(ch, vict, NULL, NULL,
                    SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            case CLASS_WHITE:
            case CLASS_SILVER:
                call_magic(ch, vict, NULL, NULL, SPELL_FROST_BREATH,
                    GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            case CLASS_RED:
                call_magic(ch, vict, NULL, NULL, SPELL_FIRE_BREATH, GET_LEVEL(ch),
                    CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            case CLASS_SHADOW_D:
                call_magic(ch, vict, NULL, NULL, SPELL_SHADOW_BREATH,
                    GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            case CLASS_TURTLE:
                call_magic(ch, vict, NULL, NULL, SPELL_STEAM_BREATH,
                    GET_LEVEL(ch), CAST_BREATH);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                break;
            }
            return 0;
        }
    }
    if (GET_RACE(ch) == RACE_ELEMENTAL && random_fractional_4()) {

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        prob =
            GET_LEVEL(ch) - GET_LEVEL(vict) + GET_AC(vict) + GET_HITROLL(ch);
        if (prob > (random_percentage() - 50))
            dam = random_number_zero_low(GET_LEVEL(ch)) + (GET_LEVEL(ch) / 2);
        else
            dam = 0;
        switch (GET_CLASS(ch)) {
        case CLASS_EARTH:
            if (random_fractional_3()) {
                damage(ch, vict, NULL, dam, SPELL_EARTH_ELEMENTAL, WEAR_RANDOM);
                WAIT_STATE(ch, PULSE_VIOLENCE);
            } else if (random_fractional_5()) {
                damage(ch, vict, NULL, dam, SKILL_BODYSLAM, WEAR_BODY);
                WAIT_STATE(ch, PULSE_VIOLENCE);
            }
            break;
        case CLASS_FIRE:
            if (random_fractional_3()) {
                damage(ch, vict, NULL, dam, SPELL_FIRE_ELEMENTAL, WEAR_RANDOM);
                WAIT_STATE(ch, PULSE_VIOLENCE);
            }
            break;
        case CLASS_WATER:
            if (random_fractional_3()) {
                damage(ch, vict, NULL, dam, SPELL_WATER_ELEMENTAL, WEAR_RANDOM);
                WAIT_STATE(ch, PULSE_VIOLENCE);
            }
            break;
        case CLASS_AIR:
            if (random_fractional_3()) {
                damage(ch, vict, NULL, dam, SPELL_AIR_ELEMENTAL, WEAR_RANDOM);
                WAIT_STATE(ch, PULSE_VIOLENCE);
            }
            break;
        }
        return 0;
    }

    /** CLASS ATTACKS HERE **/
    int cur_class = 0;
    if (GET_REMORT_CLASS(ch) != CLASS_UNDEFINED && !random_binary())
        cur_class = GET_REMORT_CLASS(ch);
    else
        cur_class = GET_CLASS(ch);
    if (cur_class == CLASS_PSIONIC
        && !ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)) {
        if (psionic_mob_fight(ch, precious_vict))
            return 0;
    }

    if (cur_class == CLASS_CYBORG) {

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        if (random_fractional_3() == 1) {
            if (GET_MOVE(ch) < 200 && random_binary()) {
                do_de_energize(ch, GET_NAME(vict), 0, 0);
                return 0;
            }
            if (GET_LEVEL(ch) >= 14 && random_binary()) {
                if (GET_MOVE(ch) > 50 && random_binary()) {
                    do_discharge(ch, tmp_sprintf("%d %s", GET_LEVEL(ch) / 3,
                            fname(vict->player.name)), 0, 0);
                    return 0;
                }
            } else if (GET_LEVEL(ch) >= 9) {
                perform_offensive_skill(ch, vict, SKILL_KICK);
                return 0;
            }
        }

    }
    // Check to make sure they didn't die or something
    if (!ch->in_room)
        return 0;

    if ((cur_class == CLASS_MAGE || IS_LICH(ch))
        && !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        if (mage_mob_fight(ch, precious_vict))
            return 0;
    }
    if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {

        if (cur_class == CLASS_CLERIC &&
            !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {

            if (!(vict = choose_opponent(ch, precious_vict)))
                return 0;

            if (random_fractional_10()
                && can_cast_spell(ch, SPELL_ARMOR)
                && !affected_by_spell(ch, SPELL_ARMOR)) {
                cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR);
                return 0;
            } else if ((GET_HIT(ch) / MAX(1,
                        GET_MAX_HIT(ch))) < (GET_MAX_HIT(ch) / 4)) {
                if (can_cast_spell(ch, SPELL_CURE_LIGHT)
                    && random_fractional_10()) {
                    cast_spell(ch, ch, NULL, NULL, SPELL_CURE_LIGHT);
                    return 0;
                } else if (can_cast_spell(ch, SPELL_CURE_CRITIC)
                    && random_fractional_10()) {
                    cast_spell(ch, ch, NULL, NULL, SPELL_CURE_CRITIC);
                    return 0;
                } else if (can_cast_spell(ch, SPELL_HEAL)
                    && random_fractional_10()) {
                    cast_spell(ch, ch, NULL, NULL, SPELL_HEAL);
                    return 0;
                } else if (can_cast_spell(ch, SPELL_GREATER_HEAL)
                    && random_fractional_10()) {
                    cast_spell(ch, ch, NULL, NULL, SPELL_GREATER_HEAL);
                    return 0;
                }
            }
            if (random_fractional_10()) {
                if (can_cast_spell(ch, SPELL_DISPEL_GOOD) && IS_GOOD(vict))
                    cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_GOOD);
                else if (can_cast_spell(ch, SPELL_DISPEL_EVIL)
                    && IS_EVIL(vict))
                    cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_EVIL);
                return 0;
            }

            if (random_fractional_5()) {
                if (can_cast_spell(ch, SPELL_SPIRIT_HAMMER)) {
                    cast_spell(ch, vict, NULL, NULL, SPELL_SPIRIT_HAMMER);
                } else if (can_cast_spell(ch, SPELL_CALL_LIGHTNING)) {
                    cast_spell(ch, vict, NULL, NULL, SPELL_CALL_LIGHTNING);
                } else if (can_cast_spell(ch, SPELL_HARM)) {
                    cast_spell(ch, vict, NULL, NULL, SPELL_HARM);
                } else if (can_cast_spell(ch, SPELL_FLAME_STRIKE)) {
                    cast_spell(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE);
                }
                return 0;
            }
        }
    }

    if (cur_class == CLASS_BARB || IS_GIANT(ch)) {
        if (barbarian_battle_activity(ch, precious_vict)) {
            return 0;
        }
    }

    if (cur_class == CLASS_WARRIOR) {

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        if (can_see_creature(ch, vict) && (IS_MAGE(vict) || IS_CLERIC(vict))
            && GET_POSITION(vict) > POS_SITTING) {
            perform_offensive_skill(ch, vict, SKILL_BASH);
            return 0;
        }

        if ((GET_LEVEL(ch) > 37) && GET_POSITION(vict) > POS_SITTING &&
            random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_BASH);
            return 0;
        } else if ((GET_LEVEL(ch) >= 20) &&
            GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
            random_fractional_5()) {
            do_disarm(ch, tmp_strdup(PERS(vict, ch)), 0, 0);
            return 0;
        } else if ((GET_LEVEL(ch) > 9) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_ELBOW);
            return 0;
        } else if ((GET_LEVEL(ch) > 5) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_STOMP);
            return 0;
        } else if ((GET_LEVEL(ch) > 2) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_PUNCH);
            return 0;
        }

        if (random_fractional_5()) {

            if (GET_LEVEL(ch) < 3) {
                perform_offensive_skill(ch, vict, SKILL_PUNCH);
            } else if (GET_LEVEL(ch) < 5) {
                perform_offensive_skill(ch, vict, SKILL_KICK);
            } else if (GET_LEVEL(ch) < 7) {
                perform_offensive_skill(ch, vict, SKILL_STOMP);
            } else if (GET_LEVEL(ch) < 9) {
                perform_offensive_skill(ch, vict, SKILL_KNEE);
            } else if (GET_LEVEL(ch) < 14) {
                perform_offensive_skill(ch, vict, SKILL_UPPERCUT);
            } else if (GET_LEVEL(ch) < 16) {
                perform_offensive_skill(ch, vict, SKILL_ELBOW);
            } else if (GET_LEVEL(ch) < 24) {
                perform_offensive_skill(ch, vict, SKILL_HOOK);
            } else if (GET_LEVEL(ch) < 36) {
                perform_offensive_skill(ch, vict, SKILL_SIDEKICK);
            }
            return 0;
        }
    }

    if (cur_class == CLASS_THIEF) {

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        if (GET_LEVEL(ch) > 18 && GET_POSITION(vict) >= POS_FIGHTING
            && random_fractional_4()) {
            perform_offensive_skill(ch, vict, SKILL_TRIP);
            return 0;
        } else if ((GET_LEVEL(ch) >= 20) &&
            GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
            random_fractional_5()) {
            do_disarm(ch, tmp_strdup(PERS(vict, ch)), 0, 0);
            return 0;
        } else if (GET_LEVEL(ch) > 29 && random_fractional_4()) {
            perform_offensive_skill(ch, vict, SKILL_GOUGE);
            return 0;
        } else if (GET_LEVEL(ch) > 24 && random_fractional_4()) {
            do_feign(ch, tmp_strdup(""), 0, 0);
            do_hide(ch, tmp_strdup(""), 0, 0);
            SET_BIT(NPC_FLAGS(ch), NPC_MEMORY);
            remember(ch, vict);
            return 0;
        } else if (weap &&
            ((GET_OBJ_VAL(weap, 3) == TYPE_PIERCE - TYPE_HIT) ||
                (GET_OBJ_VAL(weap, 3) == TYPE_STAB - TYPE_HIT)) &&
            CHECK_SKILL(ch, SKILL_CIRCLE) > 40) {
            do_circle(ch, fname(vict->player.name), 0, 0);
            return 0;
        } else if ((GET_LEVEL(ch) > 9) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_ELBOW);
            return 0;
        } else if ((GET_LEVEL(ch) > 5) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_STOMP);
            return 0;
        } else if ((GET_LEVEL(ch) > 2) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_PUNCH);
            return 0;
        }

        if (random_fractional_5()) {

            if (GET_LEVEL(ch) < 3) {
                perform_offensive_skill(ch, vict, SKILL_PUNCH);
            } else if (GET_LEVEL(ch) < 7) {
                perform_offensive_skill(ch, vict, SKILL_STOMP);
            } else if (GET_LEVEL(ch) < 9) {
                perform_offensive_skill(ch, vict, SKILL_KNEE);
            } else if (GET_LEVEL(ch) < 16) {
                perform_offensive_skill(ch, vict, SKILL_ELBOW);
            } else if (GET_LEVEL(ch) < 24) {
                perform_offensive_skill(ch, vict, SKILL_HOOK);
            } else if (GET_LEVEL(ch) < 36) {
                perform_offensive_skill(ch, vict, SKILL_SIDEKICK);
            }
            return 0;
        }
    }
    if (cur_class == CLASS_RANGER) {
        if (ranger_battle_activity(ch, precious_vict)) {
            return 0;
        }
    }
    ///Knights act offensive, through a function! (bwahahaha)
    if (cur_class == CLASS_KNIGHT) {
        if (knight_battle_activity(ch, precious_vict)) {
            return 0;
        }
    }

    if (cur_class == CLASS_MONK) {
        if (GET_LEVEL(ch) >= 23 &&
            GET_MOVE(ch) < (GET_MAX_MOVE(ch) / 2) && GET_MANA(ch) > 100
            && random_fractional_5()) {
            do_battlecry(ch, tmp_strdup(""), 1, SCMD_KIA);
            return 0;
        }
        // Monks are way too nasty. Make them twiddle thier thumbs a tad bit.
        if (random_fractional_5())
            return 0;

        if (!(vict = choose_opponent(ch, precious_vict)))
            return 0;

        if (random_fractional_3() && (GET_CLASS(vict) == CLASS_MAGIC_USER ||
                GET_CLASS(vict) == CLASS_CLERIC)) {
            if (GET_LEVEL(ch) >= 27)
                perform_offensive_skill(ch, vict, SKILL_HIP_TOSS);
            else if (GET_LEVEL(ch) >= 25)
                perform_offensive_skill(ch, vict, SKILL_SWEEPKICK);
            return 0;
        }
        if ((GET_LEVEL(ch) >= 49) && (random_fractional_5() ||
                GET_POSITION(vict) < POS_FIGHTING)) {
            perform_offensive_skill(ch, vict, SKILL_DEATH_TOUCH);
            return 0;
        } else if ((GET_LEVEL(ch) > 33) && random_fractional_5()) {
            do_combo(ch, GET_NAME(vict), 0, 0);
            return 0;
        } else if ((GET_LEVEL(ch) > 27) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_HIP_TOSS);
            return 0;
        } else if (random_fractional_5()
            && !affected_by_spell(vict, SKILL_PINCH_EPSILON)
            && (GET_LEVEL(ch) > 26)) {
            do_pinch(ch, tmp_sprintf("%s epsilon", fname(vict->player.name)),
                0, 0);
            return 0;
        } else if (random_fractional_5()
            && !affected_by_spell(vict, SKILL_PINCH_DELTA)
            && (GET_LEVEL(ch) > 19)) {
            do_pinch(ch, tmp_sprintf("%s delta", fname(vict->player.name)), 0,
                0);
            return 0;
        } else if (random_fractional_5()
            && !affected_by_spell(vict, SKILL_PINCH_BETA)
            && (GET_LEVEL(ch) > 12)) {
            do_pinch(ch, tmp_sprintf("%s beta", fname(vict->player.name)), 0,
                0);
            return 0;
        } else if (random_fractional_5()
            && !affected_by_spell(vict, SKILL_PINCH_ALPHA)
            && (GET_LEVEL(ch) > 6)) {
            do_pinch(ch, tmp_sprintf("%s alpha", fname(vict->player.name)), 0,
                0);
            return 0;
        } else if ((GET_LEVEL(ch) > 30) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_RIDGEHAND);
            return 0;
        } else if ((GET_LEVEL(ch) > 25) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_SWEEPKICK);
            return 0;
        } else if ((GET_LEVEL(ch) > 36) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_GOUGE);
            return 0;
        } else if ((GET_LEVEL(ch) > 10) && random_fractional_5()) {
            perform_offensive_skill(ch, vict, SKILL_THROAT_STRIKE);
            return 0;
        }
    }
    return 0;
}

/* Mob Memory Routines */

/* make ch remember victim */
void
remember(struct creature *ch, struct creature *victim)
{
    struct memory_rec *tmp;
    int present = false;

    if (!IS_NPC(ch) || (!NPC2_FLAGGED(ch, NPC2_ATK_MOBS) && IS_NPC(victim)))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = true;

    if (!present) {
        CREATE(tmp, struct memory_rec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}

/* make ch forget victim */
void
forget(struct creature *ch, struct creature *victim)
{
    struct memory_rec *curr, *prev = NULL;

    if (!(curr = MEMORY(ch)))
        return;

    while (curr && curr->id != GET_IDNUM(victim)) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return;                 /* person wasn't there at all. */

    if (curr == MEMORY(ch))
        MEMORY(ch) = curr->next;
    else
        prev->next = curr->next;
    free(curr);
}

int
char_in_memory(struct creature *victim, struct creature *rememberer)
{
    struct memory_rec *names;

    for (names = MEMORY(rememberer); names; names = names->next)
        if (names->id == GET_IDNUM(victim))
            return true;

    return false;
}

int
mob_fight_slaad(struct creature *ch, struct creature *precious_vict)
{
    struct creature *new_mob = NULL;
    struct creature *vict = NULL;
    int num = 0;

    num = 12;
    new_mob = NULL;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return 0;

    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it))
        if (IS_SLAAD((struct creature *)(it->data)))
            num++;

    switch (GET_CLASS(ch)) {
    case CLASS_SLAAD_BLUE:
        if (!IS_PET(ch)) {
            if (!random_number_zero_low(3 * num)
                && ch->mob_specials.shared->number > 1)
                new_mob = read_mobile(GET_NPC_VNUM(ch));
            else if (!random_number_zero_low(3 * num))  /* red saad */
                new_mob = read_mobile(42000);
        }
        break;
    case CLASS_SLAAD_GREEN:
        if (!IS_PET(ch)) {
            if (!random_number_zero_low(4 * num)
                && ch->mob_specials.shared->number > 1)
                new_mob = read_mobile(GET_NPC_VNUM(ch));
            else if (!random_number_zero_low(3 * num))  /* blue slaad */
                new_mob = read_mobile(42001);
            else if (!random_number_zero_low(2 * num))  /* red slaad */
                new_mob = read_mobile(42000);
        }
        break;
    case CLASS_SLAAD_GREY:
        if (GET_EQ(ch, WEAR_WIELD) &&
            (IS_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD), ITEM_WEAPON) &&
                (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) ==
                    TYPE_SLASH - TYPE_HIT) && random_fractional_4()) &&
            GET_MOVE(ch) > 20) {
            perform_offensive_skill(ch, vict, SKILL_BEHEAD);
            return 0;
        }
        if (!IS_PET(ch)) {
            if (!random_number_zero_low(4 * num)
                && ch->mob_specials.shared->number > 1)
                new_mob = read_mobile(GET_NPC_VNUM(ch));
            else if (!random_number_zero_low(4 * num))  /* green slaad */
                new_mob = read_mobile(42002);
            else if (!random_number_zero_low(3 * num))  /* blue slaad */
                new_mob = read_mobile(42001);
            else if (!random_number_zero_low(2 * num))  /* red slaad */
                new_mob = read_mobile(42000);
        }
        break;
    case CLASS_SLAAD_DEATH:
    case CLASS_SLAAD_LORD:
        if (!IS_PET(ch)) {
            if (!random_number_zero_low(4 * num))   /* grey slaad */
                new_mob = read_mobile(42003);
            else if (!random_number_zero_low(3 * num))  /* green slaad */
                new_mob = read_mobile(42002);
            else if (!random_number_zero_low(3 * num))  /* blue slaad */
                new_mob = read_mobile(42001);
            else if (!random_number_zero_low(4 * num))  /* red slaad */
                new_mob = read_mobile(42000);
        }
        break;
    }
    if (new_mob) {
        WAIT_STATE(ch, 5 RL_SEC);
        GET_MOVE(ch) -= 100;
        char_to_room(new_mob, ch->in_room, false);
        act("$n gestures, a glowing portal appears with a whine!",
            false, ch, NULL, NULL, TO_ROOM);
        act("$n steps out of the portal with a crack of lightning!",
            false, new_mob, NULL, NULL, TO_ROOM);
        struct creature *target = random_opponent(ch);
        if (target && IS_NPC(target))
            hit(new_mob, target, TYPE_UNDEFINED);
        return 0;
    }

    return -1;
}

//
// devils
//
// return value like damage() but it is impossible to be sure who the victim was
//

int
mob_fight_devil(struct creature *ch, struct creature *precious_vict)
{

    int prob = 0;
    struct creature *new_mob = NULL;
    struct creature *vict = NULL;
    int num = 0;

    if (IS_PET(ch)) {           // pets should only fight who they're told to
        vict = random_opponent(ch);
    } else {                    // find a suitable victim
        vict = choose_opponent(ch, precious_vict);
    }
    // if you have noone to fight, don't fight
    // what a waste.
    if (vict == NULL)
        return 0;

    // prob determines devil's chance of drawing a bead on his victim for blasting
    prob =
        10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
        GET_HITROLL(ch);

    if (prob > (random_percentage() - 25)) {
        WAIT_STATE(ch, 2 RL_SEC);
        call_magic(ch, vict, NULL, NULL,
            (IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
            SPELL_HELL_FROST : SPELL_HELL_FIRE,
            GET_LEVEL(ch), CAST_BREATH);
        return 0;
    }
    // see if we're fighting more than 1 person, if so, blast the room
    num = g_list_length(ch->fighting);

    if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
        WAIT_STATE(ch, 3 RL_SEC);
        if (call_magic(ch, NULL, NULL, NULL,
                (IN_ICY_HELL(ch) || ICY_DEVIL(ch)) ?
                SPELL_HELL_FROST_STORM : SPELL_HELL_FIRE_STORM,
                GET_LEVEL(ch), CAST_BREATH)) {
            return 0;
        }
    }
    // pets shouldnt port, ever, not even once. not on a train. not on a plane
    // not even for green eggs and spam.
    if (IS_PET(ch)) {
        return 0;
    }
    // 100 move flat rate to gate, removed when the gating actually occurs
    if (GET_MOVE(ch) < 100) {
        return 0;
    }
    // hell hunters only gate IN hell, where they should not be anyway...
    if (hell_hunter == GET_NPC_SPEC(ch) && PRIME_MATERIAL_ROOM(ch->in_room))
        return 0;

    // see how many devils are already in the room
    num = 12;
    for (GList * it = ch->in_room->people; it; it = next_living(it))
        if (IS_DEVIL((struct creature *)it->data))
            num++;

    // less chance of gating for psionic devils with mana
    if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
        num += 3;

    // Devils under the affects of stigmata cannot gate
    // and take damage every time they try
    if (affected_by_spell(ch, SPELL_STIGMATA)) {
        act("$n tries to open a portal, but the stigmata prevents it!",
            false, ch, NULL, NULL, TO_ROOM);
        return damage(ch, ch, NULL, dice(15, 100), SPELL_STIGMATA, WEAR_RANDOM);
    }
    // gating results depend on devil char_class
    switch (GET_CLASS(ch)) {
    case CLASS_LESSER:
        if (random_number_zero_low(1) > num) {
            if (ch->mob_specials.shared->number > 1 && random_binary())
                new_mob = read_mobile(GET_NPC_VNUM(ch));
            else if (random_fractional_3()) // nupperibo-spined
                new_mob = read_mobile(16111);
            else {              // Abishais
                if (HELL_PLANE(ch->in_room->zone, 3))
                    new_mob = read_mobile(16133);
                else if (HELL_PLANE(ch->in_room->zone, 4) ||
                    HELL_PLANE(ch->in_room->zone, 6))
                    new_mob = read_mobile(16135);
                else if (HELL_PLANE(ch->in_room->zone, 5))
                    new_mob = read_mobile(16132);
                else if (HELL_PLANE(ch->in_room->zone, 1) ||
                    HELL_PLANE(ch->in_room->zone, 2))
                    new_mob = read_mobile(number(16131, 16135));
            }
        }
        break;
    case CLASS_GREATER:
        if (random_number_zero_low(2) > num) {
            if (ch->mob_specials.shared->number > 1 && random_binary())
                new_mob = read_mobile(GET_NPC_VNUM(ch));
            else if (GET_NPC_VNUM(ch) == 16118 ||   // Pit Fiends
                GET_NPC_VNUM(ch) == 15252) {
                if (random_binary())
                    new_mob = read_mobile(16112);   // Barbed
                else
                    new_mob = read_mobile(16116);   // horned
            } else if (HELL_PLANE(ch->in_room->zone, 5) || ICY_DEVIL(ch)) { //  Ice Devil
                if (random_fractional_3())
                    new_mob = read_mobile(16115);   // Bone
                else if (random_binary() && GET_LEVEL(ch) > 40)
                    new_mob = read_mobile(16146);
                else
                    new_mob = read_mobile(16132);   // white abishai
            }
        }
        break;
    case CLASS_DUKE:
    case CLASS_ARCH:
        if (random_number_zero_low(GET_LEVEL(ch)) > 30) {
            act("You feel a wave of sheer terror wash over you as $n approaches!", false, ch, NULL, NULL, TO_ROOM);

            for (GList * it = ch->in_room->people;
                it && it->data != ch; it = next_living(it)) {
                vict = it->data;
                if (g_list_find(vict->fighting, ch) &&
                    GET_LEVEL(vict) < LVL_AMBASSADOR &&
                    !mag_savingthrow(vict, GET_LEVEL(ch) * 4, SAVING_SPELL) &&
                    !AFF_FLAGGED(vict, AFF_CONFIDENCE))
                    do_flee(vict, tmp_strdup(""), 0, 0);
            }

        } else if (random_number_zero_low(4) > num) {
            if (GET_NPC_VNUM(ch) == 16142) {    // Sekolah
                if (random_number_zero_low(12) > num) {
                    new_mob = read_mobile(16165);   // DEVIL FISH
                }

            } else if (random_number_zero_low(12) > num) {
                if (random_binary())
                    new_mob = read_mobile(16118);   // Pit Fiend
                else
                    new_mob = read_mobile(number(16114, 16118));    // barbed bone horned ice pit
            }
        }
        break;
    }
    if (new_mob) {
        if (IS_PET(ch))
            SET_BIT(NPC_FLAGS(new_mob), NPC_PET);
        WAIT_STATE(ch, 5 RL_SEC);
        GET_MOVE(ch) -= 100;
        char_to_room(new_mob, ch->in_room, false);
        WAIT_STATE(new_mob, 3 RL_SEC);
        act("$n gestures, a glowing portal appears with a whine!",
            false, ch, NULL, NULL, TO_ROOM);

        if (GET_NPC_VNUM(ch) == 16142) {    // SEKOLAH
            act("$n swims out from the submerged portal in a jet of bubbles.",
                false, new_mob, NULL, NULL, TO_ROOM);
        } else {
            act("$n steps out of the portal with a crack of lightning!",
                false, new_mob, NULL, NULL, TO_ROOM);
        }
        struct creature *target = random_opponent(ch);
        if (target && IS_NPC(target))
            return hit(new_mob, target, TYPE_UNDEFINED);
        return 0;
    }

    return -1;
}

int
mob_fight_celestial(struct creature *ch, struct creature *precious_vict)
{

    const int LANTERN_ARCHON = 43000;
    const int HOUND_ARCHON = 43001;
    const int WARDEN_ARCHON = 43002;
    const int SWORD_ARCHON = 43003;
    const int TOME_ARCHON = 43004;

    int prob = 0;
    struct creature *new_mob = NULL;
    struct creature *vict = NULL;
    int num = 0;

    if (IS_PET(ch)) {           // pets should only fight who they're told to
        vict = random_opponent(ch);
    } else {                    // find a suitable victim
        vict = choose_opponent(ch, precious_vict);
    }
    // if you have noone to fight, don't fight
    // what a waste.
    if (vict == NULL)
        return 0;

    // prob determines celestials's chance of drawing a bead on his victim for blasting
    prob =
        10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
        GET_HITROLL(ch);

    if (prob > (random_percentage() - 25)) {
        WAIT_STATE(ch, 2 RL_SEC);
        call_magic(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE,
            GET_LEVEL(ch), CAST_BREATH);
        return 0;
    }
    // see if we're fighting more than 1 person, if so, blast the room
    num = g_list_length(ch->fighting);

    if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
        WAIT_STATE(ch, 3 RL_SEC);
        if (call_magic(ch, NULL, NULL, NULL, SPELL_FLAME_STRIKE,
                GET_LEVEL(ch), CAST_BREATH)) {
            return 0;
        }
    }
    // pets shouldnt port, ever, not even once. not on a train. not on a plane
    // not even for green eggs and spam.
    if (IS_PET(ch)) {
        return 0;
    }
    // 100 move flat rate to gate, removed when the gating actually occurs
    if (GET_MOVE(ch) < 100) {
        return 0;
    }
    // see how many celestials are already in the room
    num = 0;
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (GET_RACE(tch) == RACE_CELESTIAL || GET_RACE(tch) == RACE_ARCHON)
            num++;
    }

    // less chance of gating for psionic celeestials with mana
    if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
        num += 3;

    // gating results depend on celestials char_class
    switch (GET_CLASS(ch)) {
    case CLASS_LESSER:
        if (random_number_zero_low(1) > num) {
            if (random_fractional_4()) {    // Warden Archon
                new_mob = read_mobile(WARDEN_ARCHON);
            } else if (random_fractional_3()) { //Hound Archon
                new_mob = read_mobile(HOUND_ARCHON);
            } else {            // Lantern Archon
                new_mob = read_mobile(LANTERN_ARCHON);
            }
        }
        break;
    case CLASS_GREATER:
        if (random_number_zero_low(2) > num) {
            if (random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(TOME_ARCHON);
                } else {
                    new_mob = read_mobile(SWORD_ARCHON);
                }

            } else if (random_binary()) {
                new_mob = read_mobile(WARDEN_ARCHON);
            } else {
                new_mob = read_mobile(HOUND_ARCHON);
            }
        }
        break;
    case CLASS_GODLING:
        if (random_number_zero_low(3) > num) {
            if (random_binary())
                new_mob = read_mobile(TOME_ARCHON); // Tome Archon
            else
                new_mob = read_mobile(SWORD_ARCHON);    // Sword Archon

        }
        break;
    case CLASS_DIETY:
        if (random_number_zero_low(5) > num) {
            if (random_binary())
                new_mob = read_mobile(TOME_ARCHON); // Tome Archon
            else
                new_mob = read_mobile(SWORD_ARCHON);    // Sword Archon

        }
        break;
    }
    if (new_mob) {
        if (IS_PET(ch))
            SET_BIT(NPC_FLAGS(new_mob), NPC_PET);
        WAIT_STATE(ch, 5 RL_SEC);
        GET_MOVE(ch) -= 100;
        char_to_room(new_mob, ch->in_room, false);
        WAIT_STATE(new_mob, 3 RL_SEC);
        act("$n gestures, a glowing silver portal appears with a hum!",
            false, ch, NULL, NULL, TO_ROOM);
        act("$n steps out of the portal with a flash of white light!",
            false, new_mob, NULL, NULL, TO_ROOM);
        struct creature *target = random_opponent(ch);
        if (target && IS_NPC(target))
            return hit(new_mob, target, TYPE_UNDEFINED);
        return 0;
    }

    return -1;
}

/**mob_fight_Guardinal:  /
 *  This is the port and combat code for the guardinal mobs that enhabit elysium
 */

int
mob_fight_guardinal(struct creature *ch, struct creature *precious_vict)
{

    const int AVORIAL_GUARDINAL = 48160;
    const int HAWCINE_GUARDINAL = 48161;
    const int PANTHRAL_GUARDINAL = 48162;
    const int LEONAL_GUARDINAL = 48163;

    int prob = 0;
    struct creature *new_mob = NULL;
    struct creature *vict = NULL;
    int num = 0;

    if (IS_PET(ch)) {           // pets should only fight who they're told to
        vict = random_opponent(ch);
    } else {                    // find a suitable victim
        vict = choose_opponent(ch, precious_vict);
    }
    // if you have noone to fight, don't fight
    // what a waste.
    if (vict == NULL)
        return 0;

    // prob determines guardinal's chance of drawing a bead on his victim for blasting
    prob =
        10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
        GET_HITROLL(ch);

    if (prob > (random_percentage() - 25)) {
        WAIT_STATE(ch, 2 RL_SEC);
        call_magic(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE,
            GET_LEVEL(ch), CAST_BREATH);
        return 0;
    }
    // see if we're fighting more than 1 person, if so, blast the room
    num = g_list_length(ch->fighting);

    if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
        WAIT_STATE(ch, 3 RL_SEC);
        if (call_magic(ch, NULL, NULL, NULL, SPELL_FLAME_STRIKE,
                GET_LEVEL(ch), CAST_BREATH)) {
            return 0;
        }
    }
    // pets shouldnt port, ever, not even once. not on a train. not on a plane
    // not even for green eggs and spam.
    if (IS_PET(ch)) {
        return 0;
    }
    // 100 move flat rate to gate, removed when the gating actually occurs
    if (GET_MOVE(ch) < 100) {
        return 0;
    }
    // see how many guardinal are already in the room

    num = 0;
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it))
        if (GET_RACE((struct creature *)it->data) == RACE_GUARDINAL)
            num++;

    // less chance of gating for psionic celeestials with mana
    if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
        num += 3;

    // gating results depend on guardinal char_class
    switch (GET_CLASS(ch)) {
    case CLASS_LESSER:
        if (random_number_zero_low(1) > num) {
            if (random_binary()) {
                new_mob = read_mobile(AVORIAL_GUARDINAL);
            } else if (!random_fractional_5()) {
                new_mob = read_mobile(HAWCINE_GUARDINAL);
            } else {
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            }
        }
        break;
    case CLASS_GREATER:
        if (random_number_zero_low(2) > num) {
            if (!random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(AVORIAL_GUARDINAL);
                } else {
                    new_mob = read_mobile(HAWCINE_GUARDINAL);
                }

            } else if (!random_fractional_3()
                || ch->in_room->zone->number == 481) {
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            } else {
                new_mob = read_mobile(LEONAL_GUARDINAL);
            }
        }
        break;
    case CLASS_GODLING:
        if (random_number_zero_low(3) > num) {
            if (random_binary() || ch->in_room->zone->number == 481)
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            else
                new_mob = read_mobile(LEONAL_GUARDINAL);

        }
        break;
    case CLASS_DIETY:
        if (random_number_zero_low(5) > num) {
            if (random_binary() || ch->in_room->zone->number == 481)
                new_mob = read_mobile(PANTHRAL_GUARDINAL);
            else
                new_mob = read_mobile(LEONAL_GUARDINAL);

        }
        break;
    }
    if (new_mob) {
        if (IS_PET(ch))
            SET_BIT(NPC_FLAGS(new_mob), NPC_PET);
        WAIT_STATE(ch, 5 RL_SEC);
        GET_MOVE(ch) -= 100;
        char_to_room(new_mob, ch->in_room, false);
        WAIT_STATE(new_mob, 3 RL_SEC);
        act("$n gestures, a glowing golden portal appears with a hum!",
            false, ch, NULL, NULL, TO_ROOM);
        act("$n steps out of the portal with a flash of blue light!",
            false, new_mob, NULL, NULL, TO_ROOM);
        struct creature *target = random_opponent(ch);
        if (target && IS_NPC(target))
            return hit(new_mob, target, TYPE_UNDEFINED);
        return 0;
    }

    return -1;
}

/**mob_fight_demon:  /
 *  This is the port and combat code for the demonic mobs that enhabit the abyss
 */

int
mob_fight_demon(struct creature *ch, struct creature *precious_vict)
{

    //Uncoment when world updated
    const int DRETCH_DEMON = 28200;
    const int BABAU_DEMON = 28201;
    const int VROCK_DEMON = 28202;
    const int HEZROU_DEMON = 28203;
    const int GLABREZU_DEMON = 28204;
    const int ARMANITE_DEMON = 28205;
    const int KNECHT_DEMON = 28206;
    const int SUCCUBUS_DEMON = 28207;
    const int GORISTRO_DEMON = 28208;
    const int BALOR_DEMON = 28209;
    const int NALFESHNEE_DEMON = 28210;

    int prob = 0;
    //Uncomment when world updated
    struct creature *new_mob = NULL;
    struct creature *vict = NULL;
    int num = 0;

    if (IS_PET(ch)) {           // pets should only fight who they're told to
        vict = random_opponent(ch);
    } else {                    // find a suitable victim
        vict = choose_opponent(ch, precious_vict);
    }
    // if you have noone to fight, don't fight
    // what a waste.
    if (vict == NULL)
        return 0;

    // prob determines demon chance of drawing a bead on his victim for blasting
    prob =
        10 + GET_LEVEL(ch) - GET_LEVEL(vict) + (GET_AC(vict) / 10) +
        GET_HITROLL(ch);

    if (prob > (random_percentage() - 25)) {
        WAIT_STATE(ch, 2 RL_SEC);
        call_magic(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE,
            GET_LEVEL(ch), CAST_BREATH);
        return 0;
    }
    // see if we're fighting more than 1 person, if so, blast the room
    num = g_list_length(ch->fighting);

    if (num > 1 && GET_LEVEL(ch) > (random_number_zero_low(50) + 30)) {
        WAIT_STATE(ch, 3 RL_SEC);
        if (call_magic(ch, NULL, NULL, NULL, SPELL_HELL_FIRE,
                GET_LEVEL(ch), CAST_BREATH)) {
            return 0;
        }
    }
    // pets shouldnt port, ever, not even once. not on a train. not on a plane
    // not even for green eggs and spam.
    if (IS_PET(ch)) {
        return 0;
    }
    // Mobs with specials set shouldn't port either
    if (GET_NPC_SPEC(ch)) {
        return 0;
    }
    // 100 move flat rate to gate, removed when the gating actually occurs
    if (GET_MOVE(ch) < 100) {
        return 0;
    }
    // see how many demon are already in the room

    num = 0;
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it))
        if (GET_RACE((struct creature *)it->data) == RACE_DEMON)
            num++;

    // less chance of gating for psionic celeestials with mana
    if (IS_PSIONIC(ch) && GET_MANA(ch) > 100)
        num += 3;
    // gating results depend on demon char_class

    switch (GET_CLASS(ch)) {
    case CLASS_DEMON_II:
        if (random_number_zero_low(8) > num) {
            if (!random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(DRETCH_DEMON);
                } else {
                    new_mob = read_mobile(BABAU_DEMON);
                }
            } else if (random_binary()) {
                new_mob = read_mobile(VROCK_DEMON);
            } else {
                new_mob = read_mobile(HEZROU_DEMON);
            }
        }
        break;
    case CLASS_DEMON_III:
        if (random_number_zero_low(1) > num) {
            if (random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(DRETCH_DEMON);
                } else {
                    new_mob = read_mobile(BABAU_DEMON);
                }
            } else if (!random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(HEZROU_DEMON);
                } else {
                    new_mob = read_mobile(VROCK_DEMON);
                }
            } else if (random_binary()) {
                new_mob = read_mobile(GLABREZU_DEMON);
            } else {
                new_mob = read_mobile(ARMANITE_DEMON);
            }
        }
        break;
    case CLASS_DEMON_IV:
        if (random_number_zero_low(2) > num) {
            if (random_fractional_3()) {
                if (random_binary()) {
                    new_mob = read_mobile(HEZROU_DEMON);
                } else {
                    new_mob = read_mobile(VROCK_DEMON);
                }
            } else if (random_binary()) {
                if (random_binary()) {
                    new_mob = read_mobile(GLABREZU_DEMON);
                } else {
                    new_mob = read_mobile(ARMANITE_DEMON);
                }
            } else if (random_fractional_3()) {
                new_mob = read_mobile(KNECHT_DEMON);
            } else if (random_binary()) {
                new_mob = read_mobile(SUCCUBUS_DEMON);
            } else {
                new_mob = read_mobile(NALFESHNEE_DEMON);
            }
        }
        break;
    case CLASS_DEMON_V:
        if (random_number_zero_low(3) > num) {
            if (random_binary()) {
                if (random_binary()) {
                    new_mob = read_mobile(GLABREZU_DEMON);
                } else {
                    new_mob = read_mobile(ARMANITE_DEMON);
                }
            } else if (!random_fractional_3()) {
                if (random_fractional_3()) {
                    new_mob = read_mobile(KNECHT_DEMON);
                } else if (random_binary()) {
                    new_mob = read_mobile(SUCCUBUS_DEMON);
                } else {
                    new_mob = read_mobile(NALFESHNEE_DEMON);
                }
            } else if (random_binary()) {
                new_mob = read_mobile(GORISTRO_DEMON);
            } else {
                new_mob = read_mobile(BALOR_DEMON);
            }
        }
        break;
    case CLASS_DEMON_LORD:
    case CLASS_SLAAD_LORD:
        if (GET_NPC_VNUM(ch) == 42819) {    // Pigeon God
            if (random_number_zero_low(4) > num) {
                if (random_binary()) {
                    new_mob = read_mobile(42875);   // grey pigeion
                } else {
                    new_mob = read_mobile(42888);   // roosting pigeon
                }
            }
        } else if (random_number_zero_low(4) > num) {
            if (!random_fractional_3()) {
                if (random_fractional_3()) {
                    new_mob = read_mobile(KNECHT_DEMON);
                } else if (random_binary()) {
                    new_mob = read_mobile(SUCCUBUS_DEMON);
                } else {
                    new_mob = read_mobile(NALFESHNEE_DEMON);
                }
            } else if (random_binary()) {
                new_mob = read_mobile(GORISTRO_DEMON);
            } else {
                new_mob = read_mobile(BALOR_DEMON);
            }
        }
        break;
    case CLASS_DEMON_PRINCE:
        if (random_number_zero_low(6) > num) {
            if (random_binary()) {
                new_mob = read_mobile(GORISTRO_DEMON);
            } else {
                new_mob = read_mobile(BALOR_DEMON);
            }
        }
        break;
    }
    if (new_mob) {
        if (IS_PET(ch))
            SET_BIT(NPC_FLAGS(new_mob), NPC_PET);
        WAIT_STATE(ch, 5 RL_SEC);
        GET_MOVE(ch) -= 100;
        char_to_room(new_mob, ch->in_room, false);
        WAIT_STATE(new_mob, 3 RL_SEC);
        act("$n gestures, a glowing yellow portal appears with a hum!",
            false, ch, NULL, NULL, TO_ROOM);

        if (GET_NPC_VNUM(ch) == 42819) {    // Pigeon god!
            act("$n flies out of the portal with a clap of thunder!",
                false, new_mob, NULL, NULL, TO_ROOM);
        } else {
            act("$n steps out of the portal with a clap of thunder!",
                false, new_mob, NULL, NULL, TO_ROOM);
        }
        struct creature *target = random_opponent(ch);
        if (target && IS_NPC(target))
            return hit(new_mob, target, TYPE_UNDEFINED);
        return 0;
    }
    return -1;
}

ACMD(do_breathe)
{                               // Breath Weapon Attack
    struct affected_type *fire = affected_by_spell(ch, SPELL_FIRE_BREATHING);
    struct affected_type *frost = affected_by_spell(ch, SPELL_FROST_BREATHING);

    if (IS_PC(ch) && fire == NULL && frost == NULL) {
        act("You breathe heavily.", false, ch, NULL, NULL, TO_CHAR);
        act("$n seems to be out of breath.", false, ch, NULL, NULL, TO_ROOM);
        return;
    }
    // Find the victim
    skip_spaces(&argument);
    struct creature *vict = get_char_room_vis(ch, argument);
    if (vict == NULL)
        vict = random_opponent(ch);
    if (vict == NULL) {
        act("Breathe on whom?", false, ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (IS_PC(ch)) {
        if (fire != NULL) {
            call_magic(ch, vict, NULL, NULL, SPELL_FIRE_BREATH, GET_LEVEL(ch),
                CAST_BREATH);
            fire = affected_by_spell(ch, SPELL_FIRE_BREATHING);
            if (fire) {
                fire->duration -= 5;
                if (fire->duration <= 0)
                    affect_remove(ch, fire);
            }
        } else if (frost != NULL) {
            call_magic(ch, vict, NULL, NULL, SPELL_FROST_BREATH, GET_LEVEL(ch),
                CAST_BREATH);
            frost = affected_by_spell(ch, SPELL_FROST_BREATH);
            if (frost) {
                frost->duration -= 5;
                if (frost->duration <= 0)
                    affect_remove(ch, frost);
            }
        } else {
            send_to_char(ch, "ERROR: No breath type found.\r\n");
        }
        return;
    }

    switch (GET_CLASS(ch)) {
    case CLASS_GREEN:
        call_magic(ch, vict, NULL, NULL, SPELL_GAS_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;

    case CLASS_BLACK:
        call_magic(ch, vict, NULL, NULL, SPELL_ACID_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    case CLASS_BLUE:
        call_magic(ch, vict, NULL, NULL,
            SPELL_LIGHTNING_BREATH, GET_LEVEL(ch), CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    case CLASS_WHITE:
    case CLASS_SILVER:
        call_magic(ch, vict, NULL, NULL, SPELL_FROST_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    case CLASS_RED:
        call_magic(ch, vict, NULL, NULL, SPELL_FIRE_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    case CLASS_SHADOW_D:
        call_magic(ch, vict, NULL, NULL, SPELL_SHADOW_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    case CLASS_TURTLE:
        call_magic(ch, vict, NULL, NULL, SPELL_STEAM_BREATH, GET_LEVEL(ch),
            CAST_BREATH);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        break;
    default:
        act("You don't seem to have a breath weapon.", false, ch, NULL, NULL,
            TO_CHAR);
        break;
    }

}

/***********************************************************************************
 *
 *                        Knight Activity
 *   First in a series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/
// cure disease, damn, divine intervention, endure cold, 
void
knight_activity(struct creature *ch)
{
    // Don't cast any spells if tainted
    if (AFF3_FLAGGED(ch, AFF3_TAINTED))
        return;
    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (can_cast_spell(ch, SPELL_HEAL) && random_binary())
            cast_spell(ch, ch, NULL, NULL, SPELL_HEAL);
        else if (can_cast_spell(ch, SPELL_CURE_CRITIC) && random_binary())
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_CRITIC);
        else if (can_cast_spell(ch, SPELL_CURE_LIGHT) && random_binary())
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_LIGHT);
        else if (IS_GOOD(ch)) {
            do_holytouch(ch, tmp_strdup("self"), 0, 0);
        }
    } else if (room_is_dark(ch->in_room)
        && !has_dark_sight(ch)
        && can_cast_spell(ch, SPELL_DIVINE_ILLUMINATION)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_DIVINE_ILLUMINATION);
    } else if (can_cast_spell(ch, SPELL_CURE_BLIND)
        && (affected_by_spell(ch, SPELL_BLINDNESS) ||
            affected_by_spell(ch, SKILL_GOUGE))) {
        cast_spell(ch, ch, NULL, NULL, SPELL_CURE_BLIND);
    } else if (AFF_FLAGGED(ch, AFF_POISON)
        && can_cast_spell(ch, SPELL_REMOVE_POISON)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_REMOVE_POISON);
    } else if (AFF_FLAGGED(ch, AFF_CURSE)
        && can_cast_spell(ch, SPELL_REMOVE_CURSE)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_REMOVE_CURSE);
    } else if (can_cast_spell(ch, SPELL_SANCTIFICATION)
        && !affected_by_spell(ch, SPELL_SANCTIFICATION)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_SANCTIFICATION);
    } else if (can_cast_spell(ch, SPELL_SPHERE_OF_DESECRATION)
        && !affected_by_spell(ch, SPELL_SPHERE_OF_DESECRATION)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_SPHERE_OF_DESECRATION);
    } else if (can_cast_spell(ch, SPELL_ARMOR)
        && !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR);
    } else if (can_cast_spell(ch, SPELL_BLESS)
        && !affected_by_spell(ch, SPELL_BLESS)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_BLESS);
    } else if (can_cast_spell(ch, SPELL_PROT_FROM_EVIL)
               && !affected_by_spell(ch, SPELL_PROT_FROM_EVIL)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_PROT_FROM_EVIL);
    } else if (can_cast_spell(ch, SPELL_PROT_FROM_GOOD)
               && !affected_by_spell(ch, SPELL_PROT_FROM_GOOD)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_PROT_FROM_GOOD);
    } else if (can_cast_spell(ch, SPELL_PRAY)
        && !affected_by_spell(ch, SPELL_PRAY)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_PRAY);
    } else if (can_cast_spell(ch, SPELL_MAGICAL_VESTMENT)
        && !affected_by_spell(ch, SPELL_MAGICAL_VESTMENT)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_MAGICAL_VESTMENT);
    }
}

/***********************************************************************************
 *
 *                        Knight Battle Activity
 *   member of  a series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/

bool
knight_battle_activity(struct creature *ch, struct creature *precious_vict)
{
    ACMD(do_disarm);
    struct creature *vict;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return true;

    if (can_see_creature(ch, vict) && (IS_MAGE(vict) || IS_CLERIC(vict))
        && GET_POSITION(vict) > POS_SITTING) {
        perform_offensive_skill(ch, vict, SKILL_BASH);
        return true;
    }

    if (can_cast_spell(ch, SPELL_ARMOR)
        && random_fractional_5()
        && !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR);
        return true;
    } else if ((GET_HIT(ch) / MAX(1,
                GET_MAX_HIT(ch))) < (GET_MAX_HIT(ch) / 4)) {
        if (can_cast_spell(ch, SPELL_CURE_LIGHT)
            && random_fractional_10()) {
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_LIGHT);
            return true;
        } else if (can_cast_spell(ch, SPELL_CURE_CRITIC)
            && random_fractional_10()) {
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_CRITIC);
            return true;
        } else if (can_cast_spell(ch, SPELL_HEAL)
            && random_fractional_5()) {
            cast_spell(ch, ch, NULL, NULL, SPELL_HEAL);
            return true;
        }
    } else if (random_fractional_10()
               && can_cast_spell(ch, SPELL_INVIS_TO_UNDEAD)
               && !affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_INVIS_TO_UNDEAD);
        return true;
    } else if (random_fractional_5() && can_cast_spell(ch, SPELL_SPIRIT_HAMMER)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_SPIRIT_HAMMER);
        return true;
    } else if (random_fractional_4() && can_cast_spell(ch, SPELL_FLAME_STRIKE)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE);
        return true;
    } else if (!IS_EVIL(vict)
               && can_cast_spell(ch, SPELL_DAMN)
               && !affected_by_spell(vict, SPELL_DAMN)
               && random_fractional_4()) {
        cast_spell(ch, vict, NULL, NULL, SPELL_DAMN);
    } else if (!IS_EVIL(vict)
               && can_cast_spell(ch, SPELL_TAINT)
               && !affected_by_spell(vict, SPELL_TAINT)
               && random_fractional_4()) {
        cast_spell(ch, vict, NULL, NULL, SPELL_TAINT);
    } else if ((GET_LEVEL(ch) >= 20) &&
        GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
        random_fractional_3()) {
        do_disarm(ch, tmp_strdup(PERS(vict, ch)), 0, 0);
        return true;
    }

    if (random_fractional_4()) {

        if (GET_LEVEL(ch) < 7) {
            perform_offensive_skill(ch, vict, SKILL_SPINFIST);
        } else if (GET_LEVEL(ch) < 16) {
            perform_offensive_skill(ch, vict, SKILL_UPPERCUT);
        } else if (GET_LEVEL(ch) < 23) {
            perform_offensive_skill(ch, vict, SKILL_HOOK);
        } else if (GET_LEVEL(ch) < 35) {
            perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH);
        } else if (GET_EQ(ch, WEAR_WIELD)
            && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) == 3
            && random_binary()) {
            perform_offensive_skill(ch, vict, SKILL_BEHEAD);
        } else if (IS_EVIL(ch) && IS_GOOD(vict)) {
            do_holytouch(ch, tmp_strdup(PERS(vict, ch)), 0, 0);
        }
        return true;
    }
    return false;
}

/***********************************************************************************
 *
 *                        Ranger Activity
 *   Continuing series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/

void
ranger_activity(struct creature *ch)
{

    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (GET_LEVEL(ch) > 39)
            do_medic(ch, tmp_strdup("self"), 0, 0);
        else if (GET_LEVEL(ch) > 18)
            do_firstaid(ch, tmp_strdup("self"), 0, 0);
        else if (GET_LEVEL(ch) > 9)
            do_bandage(ch, tmp_strdup("self"), 0, 0);
    } else if (AFF_FLAGGED(ch, AFF_POISON) && can_cast_spell(ch, SPELL_REMOVE_POISON)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_REMOVE_POISON);
    } else if (!affected_by_spell(ch, SPELL_STONESKIN) && can_cast_spell(ch, SPELL_STONESKIN)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_STONESKIN);
    } else if (!affected_by_spell(ch, SPELL_BARKSKIN)
               && !affected_by_spell(ch, SPELL_STONESKIN)
               && can_cast_spell(ch, SPELL_BARKSKIN)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_BARKSKIN);
    }
}

/***********************************************************************************
 *
 *                        Ranger battle  Activity
 *   Continuing series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/

bool
ranger_battle_activity(struct creature *ch, struct creature *precious_vict)
{
    ACMD(do_disarm);

    struct creature *vict;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return true;

    if (random_fractional_5()
        && !affected_by_spell(ch, SPELL_BARKSKIN)
        && !affected_by_spell(ch, SPELL_STONESKIN)
        && can_cast_spell(ch, SPELL_BARKSKIN)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_BARKSKIN);
        return true;
    }
    if (GET_LEVEL(ch) >= 27 &&
        (GET_CLASS(vict) == CLASS_MAGIC_USER ||
            GET_CLASS(vict) == CLASS_CLERIC) && random_fractional_3()) {
        perform_offensive_skill(ch, vict, SKILL_SWEEPKICK);
        return true;
    }
    if ((GET_LEVEL(ch) > 42) && random_fractional_5()) {
        perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH);
        return true;
    } else if ((GET_LEVEL(ch) > 25) && random_fractional_5()) {
        perform_offensive_skill(ch, vict, SKILL_BEARHUG);
        return true;
    } else if ((GET_LEVEL(ch) >= 20) &&
        GET_EQ(ch, WEAR_WIELD) && GET_EQ(vict, WEAR_WIELD) &&
        random_fractional_5()) {
        do_disarm(ch, tmp_strdup(PERS(vict, ch)), 0, 0);
        return true;
    }

    if (random_fractional_4()) {

        if (GET_LEVEL(ch) < 3) {
            perform_offensive_skill(ch, vict, SKILL_KICK);
        } else if (GET_LEVEL(ch) < 6) {
            perform_offensive_skill(ch, vict, SKILL_BITE);
        } else if (GET_LEVEL(ch) < 14) {
            perform_offensive_skill(ch, vict, SKILL_UPPERCUT);
        } else if (GET_LEVEL(ch) < 17) {
            perform_offensive_skill(ch, vict, SKILL_HEADBUTT);
        } else if (GET_LEVEL(ch) < 22) {
            perform_offensive_skill(ch, vict, SKILL_ROUNDHOUSE);
        } else if (GET_LEVEL(ch) < 24) {
            perform_offensive_skill(ch, vict, SKILL_HOOK);
        } else if (GET_LEVEL(ch) < 36) {
            perform_offensive_skill(ch, vict, SKILL_SIDEKICK);
        } else {
            perform_offensive_skill(ch, vict, SKILL_LUNGE_PUNCH);
        }
        return true;
    }
    return false;
}

/***********************************************************************************
 *
 *                        Barbarian Activity
 *   Continuing series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/

void
barbarian_activity(struct creature *ch)
{
    if (AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        do_berserk(ch, tmp_strdup(""), 0, 0);
        return;
    }
    if (GET_POSITION(ch) != POS_FIGHTING && random_fractional_20()) {
        if (random_fractional_50())
            act("$n grunts and scratches $s ear.", false, ch, NULL, NULL, TO_ROOM);
        else if (random_fractional_50())
            act("$n drools all over $mself.", false, ch, NULL, NULL, TO_ROOM);
        else if (random_fractional_50())
            act("$n belches loudly.", false, ch, NULL, NULL, TO_ROOM);
        else if (random_fractional_50())
            act("$n swats at an annoying gnat.", false, ch, NULL, NULL, TO_ROOM);
        else if (random_fractional_100()) {
            if (GET_SEX(ch) == SEX_MALE)
                act("$n scratches $s nuts and grins.", false, ch, NULL, NULL,
                    TO_ROOM);
            else if (GET_SEX(ch) == SEX_FEMALE)
                act("$n belches loudly and grins.", false, ch, NULL, NULL, TO_ROOM);
        }
    } else if (IS_BARB(ch) && GET_LEVEL(ch) >= 42 &&
        GET_HIT(ch) < (GET_MAX_HIT(ch) / 2) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, tmp_strdup(""), 0, SCMD_CRY_FROM_BEYOND);
        return;
    } else if (IS_BARB(ch) && GET_LEVEL(ch) >= 30 &&
        GET_MOVE(ch) < (GET_MAX_MOVE(ch) / 4) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, tmp_strdup(""), 0, SCMD_BATTLE_CRY);
        return;
    }
}

/***********************************************************************************
 *
 *                        Barbarian battle  Activity
 *   Continuing series of functions designed to extract activity and fight code
 * into many small functions as opposed to one large one.  Also allows for easeir
 * modifications to mob ai.
 *******************************************************************************/

bool
barbarian_battle_activity(struct creature * ch,
    struct creature * precious_vict)
{
    ACMD(do_disarm);

    struct creature *vict;

    if (!(vict = choose_opponent(ch, precious_vict)))
        return true;

    if (IS_BARB(ch) && GET_LEVEL(ch) >= 42 &&
        GET_HIT(ch) < (GET_MAX_HIT(ch) / 2) && GET_MANA(ch) > 30
        && random_fractional_4()) {
        do_battlecry(ch, tmp_strdup(""), 0, SCMD_CRY_FROM_BEYOND);
        return true;
    }

    if (random_fractional_3() && (GET_CLASS(vict) == CLASS_MAGIC_USER ||
            GET_CLASS(vict) == CLASS_CLERIC)) {
        perform_offensive_skill(ch, vict, SKILL_BASH);
        return true;
    }
    if ((GET_LEVEL(ch) > 27) && random_fractional_5()
        && GET_POSITION(vict) > POS_RESTING) {
        perform_offensive_skill(ch, vict, SKILL_BASH);
        return true;
    }
    //
    // barbs go BERSERK (berserk)
    //
    if (GET_LEVEL(ch) < LVL_AMBASSADOR
        && !AFF2_FLAGGED(ch, AFF2_BERSERK)
        && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        do_berserk(ch, tmp_strdup(""), 0, 0);
        return true;
	}
    if (random_fractional_4()) {
        if (GET_LEVEL(ch) >= 30 && GET_EQ(ch, WEAR_WIELD)
            && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
            perform_cleave(ch, vict);
        } else if (GET_LEVEL(ch) >= 17) {
            perform_offensive_skill(ch, vict, SKILL_STRIKE);
        } else if (GET_LEVEL(ch) >= 13) {
            perform_offensive_skill(ch, vict, SKILL_HEADBUTT);
        } else if (GET_LEVEL(ch) >= 4) {
            perform_offensive_skill(ch, vict, SKILL_STOMP);
        } else {
            perform_offensive_skill(ch, vict, SKILL_KICK);
        }
        return true;
    }
    return false;
}
