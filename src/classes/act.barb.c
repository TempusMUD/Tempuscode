
// File: act.barb.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.barb.c      --  Barbarian specific functions,      Fireball, July 1998
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
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
#include "race.h"
#include "creature.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "bomb.h"
#include "fight.h"
#include "obj_data.h"

/**
 * do_charge:
 *
 * Command - Starts combat with another creature with bonus damage on
 * the first round.
 *
 */

ACMD(do_charge)
{
    struct affected_type af;
    struct creature *vict = NULL;
    char *arg;

    init_affect(&af);

    arg = tmp_getword(&argument);
    // Check for berserk.

    if (CHECK_SKILL(ch, SKILL_CHARGE) < 50) {
        send_to_char(ch,
            "Do you really think you know what you're doing?\r\n");
        return;
    }
    // find out who we're whackin.
    vict = get_char_in_remote_room_vis(ch, arg, ch->in_room);
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
    af.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af);
    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    // Whap the bastard
    hit(ch, vict, TYPE_UNDEFINED);
}


/**
 * do_corner:
 *
 * Command - intended to trap another person in a room, just a stub now.
 * TODO: Make this do something
 */

ACMD(do_corner)
{
    send_to_char(ch, "You back into the corner.\r\n");
    return;
}

/**
 * select_berserk_victim:
 * @tch: the creature considered for selection
 * @ch: the creature going berserk
 *
 * Decides whether or not @tch will be the creature that @ch will
 * attack due to berserk.  For use as a g_list_find_custom() callback.
 *
 * Returns: 0 if tch is selected, otherwise -1
 */
gint
select_berserk_victim(struct creature *tch, struct creature *ch)
{
    if (tch == ch
        || is_dead(tch)
        || g_list_find(tch->fighting, ch)
        || PRF_FLAGGED(tch, PRF_NOHASSLE)
        || (IS_NPC(ch)
            && IS_NPC(tch)
            && !NPC2_FLAGGED(ch, NPC2_ATK_MOBS))
        || !can_see_creature(ch, tch)
        || !number(0, 1 + (GET_LEVEL(ch) / 16)))
        return -1;

    if (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(tch, AFF_GROUP)
        && (tch->master == ch || tch->master == ch->master || ch->master == tch))
        return -1;

    return 0;
}

/**
 * perform_barb_berserk:
 * @ch: the creature going berserk
 * @who_was_attacked: a return value indicating the creature attacked
 *
 * Selects a random valid creature for @ch to attack and attacks it.
 *
 * Returns: 1 if someone was attacked, otherwise 0
 */

int
perform_barb_berserk(struct creature *ch,
                     struct creature **who_was_attacked)
{
    GList *cit = g_list_find_custom(ch->in_room->people,
        ch,
        (GCompareFunc) select_berserk_victim);
    if (!cit)
        return 0;

    struct creature *vict = cit->data;

    act("You go berserk and attack $N!", false, ch, NULL, vict, TO_CHAR);
    act("$n attacks you in a BERSERK rage!!", false, ch, NULL, vict, TO_VICT);
    act("$n attacks $N in a BERSERK rage!!", false, ch, NULL, vict, TO_NOTVICT);
    hit(ch, vict, TYPE_UNDEFINED);
    if (!is_dead(vict) && who_was_attacked)
        *who_was_attacked = vict;

    return 1;
}

/**
 * do_berserk:
 *
 * Command - toggles the berserk state.  A skill roll must be made to
 * use this command.  If going berserk with another creature in the
 * room, starts attacking the creature.
 *
 */

ACMD(do_berserk)
{
    struct affected_type af, af2, af3;
    int percent;

    init_affect(&af);
    init_affect(&af2);
    init_affect(&af3);

    percent = (number(1, 101) - GET_LEVEL(ch));

    if (AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        if (percent > CHECK_SKILL(ch, SKILL_BERSERK)) {
            send_to_char(ch, "You cannot calm down!!\r\n");
            return;
        } else {
            affect_from_char(ch, SKILL_BERSERK);
            send_to_char(ch, "You are no longer berserk.\r\n");
            act("$n calms down by taking deep breaths.", true, ch, NULL, NULL,
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
        af.modifier = -(5 + (GET_LEVEL(ch) / 32));
        af2.modifier = -(5 + (GET_LEVEL(ch) / 32));
        af3.modifier = (2 + GET_REMORT_GEN(ch) + (GET_LEVEL(ch) / 16));
        af.aff_index = 2;
        af.bitvector = AFF2_BERSERK;
        af2.bitvector = 0;
        af3.bitvector = 0;
        af.owner = GET_IDNUM(ch);
        af2.owner = GET_IDNUM(ch);
        af3.owner = GET_IDNUM(ch);
        affect_to_char(ch, &af);
        affect_to_char(ch, &af2);
        affect_to_char(ch, &af3);

        send_to_char(ch, "You go BERSERK!\r\n");
        act("$n goes BERSERK! Run for cover!", true, ch, NULL, ch, TO_ROOM);

        perform_barb_berserk(ch, NULL);
    } else
        send_to_char(ch, "You cannot work up the gumption to do so.\r\n");
}

/**
 * do_battlecry:
 *
 * Command - emit a fierce battlecry, giving recovery to hitpoints or
 * move.  Handles the kia, battle cry, and cry from beyond commands.
 */

ACMD(do_battlecry)
{
    int skillnum = (subcmd == SCMD_KIA ? SKILL_KIA :
        (subcmd == SCMD_BATTLE_CRY ? SKILL_BATTLE_CRY :
            SKILL_CRY_FROM_BEYOND));
    int did = 0;

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        send_to_char(ch,
            "You just feel too damn peaceful here to do that.\r\n");
    } else if (CHECK_SKILL(ch, skillnum) < number(50, 110)) {
        send_to_char(ch, "You emit a feeble warbling sound.\r\n");
        act("$n makes a feeble warbling sound.", false, ch, NULL, NULL, TO_ROOM);
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
        did = true;

    } else {

        int trans = CHECK_SKILL(ch, skillnum);
        trans += GET_LEVEL(ch) << GET_REMORT_GEN(ch);
        trans += GET_CON(ch) * 8;
        trans -= (trans * GET_HIT(ch)) / (GET_MAX_HIT(ch) * 2);
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
        send_to_char(ch,
            "Your fearsome battle cry rings out across the land!\r\n");
        act("$n releases a battle cry that makes your blood run cold!", false,
            ch, NULL, NULL, TO_ROOM);
    } else if (subcmd == SCMD_CRY_FROM_BEYOND) {
        send_to_char(ch, "Your cry from beyond shatters the air!!\r\n");
        act("$n unleashes a cry from beyond that makes your blood run cold!",
            false, ch, NULL, NULL, TO_ROOM);
    } else {
        send_to_char(ch, "You release an earsplitting 'KIA!'\r\n");
        act("$n releases an earsplitting 'KIA!'", false, ch, NULL, NULL, TO_ROOM);
    }

    sound_gunshots(ch->in_room, skillnum, 1, 1);

    if (did != 2)
        gain_skill_prof(ch, skillnum);
}
/**
 * select_cleave_victim:
 * @tch: the creature considered for selection
 * @ch: the creature that is cleaving
 *
 * Decides whether or not @tch will be the creature that @ch will
 * attack due to the cleave skill.  For use as a g_list_find_custom()
 * callback.
 *
 * Returns: 0 if tch is selected, otherwise -1
 */
gint
select_cleave_victim(struct creature *tch, struct creature *ch)
{
    if (tch == ch
        || is_dead(tch)
        || !g_list_find(tch->fighting, ch)
        || PRF_FLAGGED(tch, PRF_NOHASSLE)
        || (IS_NPC(ch)
            && IS_NPC(tch)
            && !NPC2_FLAGGED(ch, NPC2_ATK_MOBS))
        || !can_see_creature(ch, tch))
        return -1;

    return 0;
}

/**
 * perform_cleave:
 * @ch: The creature doing the cleaving
 * @vict: The creature being cleaved
 *
 * Hits @vict with a two-handed weapon, doing bonus damage.  If @vict
 * dies, more enemies in the room may be hit.
 *
 */
void
perform_cleave(struct creature *ch, struct creature *vict)
{
    int maxWhack;
    int percent = 0;
    int skill =
        MAX(GET_SKILL(ch, SKILL_CLEAVE), GET_SKILL(ch, SKILL_GREAT_CLEAVE));
    bool great = (GET_SKILL(ch, SKILL_GREAT_CLEAVE) > 50);
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

    if (weap == NULL || !IS_TWO_HAND(weap) || !IS_OBJ_TYPE(weap, ITEM_WEAPON)) {
        send_to_char(ch,
            "You need to be wielding a two handed weapon to cleave!\r\n");
        return;
    }

    if (great) {
        maxWhack = MAX(3, GET_REMORT_GEN(ch) - 3);
    } else {
        maxWhack = 2;
    }

    for (int i = 0; i < maxWhack && vict != NULL; i++) {
        percent = number(1, 101) + GET_DEX(vict);
        if (AWAKE(vict) && percent > skill) {
            WAIT_STATE(ch, 2 RL_SEC);
            damage(ch, vict, weap, 0, SKILL_CLEAVE, WEAR_RANDOM);
            return;
        } else {
            WAIT_STATE(vict, 1 RL_SEC);
            WAIT_STATE(ch, 3 RL_SEC);
            if (great)
                gain_skill_prof(ch, SKILL_GREAT_CLEAVE);
            gain_skill_prof(ch, SKILL_CLEAVE);
            hit(ch, vict, SKILL_CLEAVE);
            if (is_dead(ch)) {
                return;
            } else if (!is_dead(vict)) {
                return;
            } else if (GET_EQ(ch, WEAR_WIELD) != weap) {
                // Sometimes the weapon breaks and falls out of the wield
                // position.
                return;
            }
            vict = NULL;
            // find a new victim
            GList *it = g_list_find_custom(ch->in_room->people,
                                           ch,
                                           (GCompareFunc) select_cleave_victim);
            vict = (it) ? it->data:NULL;
        }
    }
}

/**
 * do_cleave:
 *
 * Command - Finds the victim and performs the cleave skill.
 *
 */
ACMD(do_cleave)
{
    struct creature *vict = NULL;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg) {
        vict = random_opponent(ch);
    } else {
        vict = get_char_room_vis(ch, arg);
    }

    if (vict == NULL) {
        send_to_char(ch, "Cleave who?\r\n");
        WAIT_STATE(ch, 2);
        return;
    }

    if (vict == ch) {
        send_to_char(ch, "You cannot cleave yourself.\r\n");
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;

    perform_cleave(ch, vict);
}

#undef __act_barb_c__
