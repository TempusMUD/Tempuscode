/*************************************************************************
*   File: combat_utils.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: combat_utils.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#define __combat_code__
#define __combat_utils__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
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
#include "libpq-fe.h"
#include "db.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "materials.h"
#include "bomb.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "specs.h"
#include "strutil.h"
#include "guns.h"
#include "weather.h"
#include "vendor.h"

extern int corpse_state;
/* The Fight related routines */
struct obj_data *get_random_uncovered_implant(struct creature *ch, int type);

static inline int
DAM_OBJECT_IDNUM(struct obj_data *obj)
{
    return IS_BOMB(obj) ? BOMB_IDNUM(obj) : GET_OBJ_SIGIL_IDNUM(obj);
}

//checks for both vendors and utility mobs
bool
ok_damage_vendor(struct creature *ch, struct creature *victim)
{
    if (ch && GET_LEVEL(ch) > LVL_CREATOR)
        return true;
    if (GET_LEVEL(victim) > LVL_IMMORT &&
        (IS_NPC(victim) || !PLR_FLAGGED(victim, PLR_MORTALIZED)))
        return false;

    if (IS_NPC(victim)
        && (NPC2_FLAGGED(victim, NPC2_SELLER)
            || victim->mob_specials.shared->func == vendor)) {
        struct shop_data *shop =
            (struct shop_data *)victim->mob_specials.func_data;

        if (!GET_NPC_PARAM(victim))
            return false;

        if (!shop) {
            CREATE(shop, struct shop_data, 1);
            vendor_parse_param(GET_NPC_PARAM(victim), shop, NULL);
            victim->mob_specials.func_data = shop;
        }

        return shop->attack_ok;
    }

    if (IS_NPC(victim) && NPC_FLAGGED(victim, NPC_UTILITY)) {
        //utility mobs shouldn't be attacked either
        return false;
    }

    return true;
}

void
update_pos(struct creature *ch)
{
    // Various stages of unhappiness
    if (GET_HIT(ch) <= -11) {
        GET_POSITION(ch) = POS_DEAD;
    } else if (GET_HIT(ch) <= -6) {
        GET_POSITION(ch) = POS_MORTALLYW;
    } else if (GET_HIT(ch) <= -3) {
        GET_POSITION(ch) = POS_INCAP;
    } else if (GET_HIT(ch) <= 0) {
        GET_POSITION(ch) = POS_STUNNED;
    } else if (GET_POSITION(ch) == POS_STUNNED) {
        // Unstun the poor bastard
        GET_POSITION(ch) = POS_RESTING;
    } else if (GET_POSITION(ch) == POS_SLEEPING) {
        // Wake them up from thier nap.
        act("You wake up.", true, ch, NULL, NULL, TO_CHAR);
        act("$n wakes up.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_RESTING;
    } else if (GET_POSITION(ch) > POS_FIGHTING && is_fighting(ch)) {
        // If everything is normal and they're fighting, set them fighting
        GET_POSITION(ch) = POS_FIGHTING;
    } else if (ch->in_room && room_is_open_air(ch->in_room)
               && !AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
               && AFF_FLAGGED(ch, AFF_INFLIGHT)
               && GET_POSITION(ch) != POS_FLYING) {
        // if they're in an open air room, not affected by gravity
        // well, are able to fly, but not flying, set them flying
        GET_POSITION(ch) = POS_FLYING;
    } else if (IS_PC(ch) || GET_NPC_WAIT(ch) > 0) {
        // Do nothing.  The rest of this function is voluntary mob
        // actions
    } else if (GET_POSITION(ch) > POS_STUNNED
               && GET_POSITION(ch) < POS_FIGHTING
               && is_fighting(ch)) {
        // If they're alive, not stunned, in a fight, and not pos_fighting
        // (Making mobs stand when they get popped)
        if (!AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL) ||
            number(1, 50) < GET_STR(ch)) {

            GET_POSITION(ch) = POS_FIGHTING;
            act("You scramble to your feet!", true, ch, NULL, NULL, TO_CHAR);
            act("$n scrambles to $s feet!", true, ch, NULL, NULL, TO_ROOM);
            WAIT_STATE(ch, PULSE_VIOLENCE);
        }
    } else if (GET_POSITION(ch) < POS_FIGHTING) {
        if (!AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)
            || number(1, 50) < GET_STR(ch)) {
            if (is_fighting(ch)) {
                act("You scramble to your feet!", true, ch, NULL, NULL, TO_CHAR);
                act("$n scrambles to $s feet!", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_FIGHTING;
            } else {
                act("You stand up.", true, ch, NULL, NULL, TO_CHAR);
                act("$n stands up.", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_STANDING;
            }
        }
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

char *
replace_string(const char *str,
    const char *weapon_singular,
    const char *weapon_plural, const char *location, const char *substance)
{
    static char buf[256];
    char *cp;
    char *prefixed_substance = NULL;
    if (substance) {
        prefixed_substance = tmp_strcat(AN(substance), " ", substance, NULL);
    }
    cp = buf;

    for (; *str; str++) {
        if (*str == '#') {
            switch (*(++str)) {
            case 'W':
                while (*weapon_plural)
                    *(cp++) = *(weapon_plural++);
                break;
            case 'w':
                while (*weapon_singular)
                    *(cp++) = *(weapon_singular++);
                break;
            case 'p':
                if (location)
                    while (*location)
                        *(cp++) = *(location++);
                break;
            case 'S':
                if (substance)
                    while (*substance)
                        *(cp++) = *(substance++);
                break;
            case 's':
                if (prefixed_substance)
                    while (*prefixed_substance)
                        *(cp++) = *(prefixed_substance++);
                break;
            default:
                *(cp++) = '#';
                break;
            }
        } else
            *(cp++) = *str;

        *cp = 0;
    }                           /* For */

    return (CAP(buf));
}

/* Calculate the raw armor including magic armor.  Lower AC is better. */
int
calculate_thaco(struct creature *ch, struct creature *victim,
                struct obj_data *weap)
{
    int calc_thaco, wpn_wgt, i;

    calc_thaco = (int)MIN(THACO(GET_CLASS(ch), GET_LEVEL(ch)),
        THACO(GET_REMORT_CLASS(ch), GET_LEVEL(ch)));

    if (weap && IS_ENERGY_GUN(weap))
        calc_thaco -= dexterity_hit_bonus(GET_DEX(ch));
    else
        calc_thaco -= strength_hit_bonus(GET_STR(ch));

    if (GET_HITROLL(ch) <= 5)
        calc_thaco -= GET_HITROLL(ch);
    else if (GET_HITROLL(ch) <= 50)
        calc_thaco -= 5 + (((GET_HITROLL(ch) - 5)) / 3);
    else
        calc_thaco -= 20;

    calc_thaco -= (int)((GET_INT(ch) - 12) / 2);   /* Intelligence helps! */
    calc_thaco -= (int)((GET_WIS(ch) - 10) / 4);   /* So does wisdom */

    if (AWAKE(victim))
        calc_thaco -= dexterity_defense_bonus(GET_DEX(victim));

    if (IS_DROW(ch)) {
        if (OUTSIDE(ch) && PRIME_MATERIAL_ROOM(ch->in_room)) {
            if (ch->in_room->zone->weather->sunlight == SUN_LIGHT)
                calc_thaco += 10;
            else if (ch->in_room->zone->weather->sunlight == SUN_DARK)
                calc_thaco -= 5;
        } else if (room_is_dark(ch->in_room))
            calc_thaco -= 5;
    }

    if (weap) {
        if (ch != weap->worn_by) {
            errlog("inconsistent weap->worn_by ptr in calculate_thaco.");
            slog("weap: ( %s ), ch: ( %s ), weap->worn->by: ( %s )",
                weap->name, GET_NAME(ch), weap->worn_by ?
                GET_NAME(weap->worn_by) : "NULL");
            return 0;
        }

        if (GET_OBJ_VNUM(weap) > 0) {
            for (i = 0; i < MAX_WEAPON_SPEC; i++)
                if (GET_WEAP_SPEC(ch, i).vnum == GET_OBJ_VNUM(weap)) {
                    calc_thaco -= GET_WEAP_SPEC(ch, i).level;
                    break;
                }
        }
        // Bonuses for bless/damn
        if (IS_EVIL(victim) && IS_OBJ_STAT(weap, ITEM_BLESS))
            calc_thaco -= 1;
        if (IS_GOOD(victim) && IS_OBJ_STAT(weap, ITEM_DAMNED))
            calc_thaco -= 1;

        wpn_wgt = GET_OBJ_WEIGHT(weap);
        if (wpn_wgt > strength_wield_weight(GET_STR(ch)))
            calc_thaco += 2;
        if (IS_MAGE(ch) &&
            (wpn_wgt >
             ((GET_LEVEL(ch) * strength_wield_weight(GET_STR(ch)) /
                        100)
                    + (strength_wield_weight(GET_STR(ch)) / 2))))
            calc_thaco += (wpn_wgt / 4);
        else if (IS_THIEF(ch) && (wpn_wgt > 12 + (GET_STR(ch) / 4)))
            calc_thaco += (wpn_wgt / 8);

        if (IS_BARB(ch))
            calc_thaco += (LEARNED(ch) - weapon_prof(ch, weap)) / 8;

        if (IS_ENERGY_GUN(weap))
            calc_thaco +=
                (LEARNED(ch) - GET_SKILL(ch, SKILL_ENERGY_WEAPONS)) / 8;

        if (IS_ENERGY_GUN(weap) && GET_SKILL(ch, SKILL_SHOOT) < 80)
            calc_thaco += (100 - GET_SKILL(ch, SKILL_SHOOT)) / 20;

        if (GET_EQ(ch, WEAR_WIELD_2)) {
            // They don't know how to second wield and
            // they dont have neural bridging
            if (CHECK_SKILL(ch, SKILL_SECOND_WEAPON) < LEARNED(ch)
                && !affected_by_spell(ch, SKILL_NEURAL_BRIDGING)) {
                if (weap == GET_EQ(ch, WEAR_WIELD_2))
                    calc_thaco -=
                        (LEARNED(ch) - CHECK_SKILL(ch,
                            SKILL_SECOND_WEAPON)) / 5;
                else
                    calc_thaco -=
                        (LEARNED(ch) - CHECK_SKILL(ch,
                            SKILL_SECOND_WEAPON)) / 10;
            }
        }
    }
    /* end if ( weap ) */
    if ((IS_EVIL(ch) && AFF_FLAGGED(victim, AFF_PROTECT_EVIL)) ||
        (IS_GOOD(ch) && AFF_FLAGGED(victim, AFF_PROTECT_GOOD)) ||
        (IS_UNDEAD(ch) && AFF2_FLAGGED(victim, AFF2_PROTECT_UNDEAD)))
        calc_thaco += 2;

    if (IS_CARRYING_N(ch) > (CAN_CARRY_N(ch) * 0.80))
        calc_thaco += 1;

    if (CAN_CARRY_W(ch))
        calc_thaco += ((TOTAL_ENCUM(ch) * 2) / CAN_CARRY_W(ch));
    else
        calc_thaco += 10;

    if (AFF2_FLAGGED(ch, AFF2_DISPLACEMENT) &&
        !AFF2_FLAGGED(victim, AFF2_TRUE_SEEING))
        calc_thaco -= 2;
    if (AFF_FLAGGED(ch, AFF_BLUR))
        calc_thaco -= 1;
    if (!can_see_creature(victim, ch))
        calc_thaco -= 3;

    if (!can_see_creature(ch, victim))
        calc_thaco += 2;
    if (GET_COND(ch, DRUNK))
        calc_thaco += 2;
    if (IS_SICK(ch))
        calc_thaco += 2;
    if (AFF2_FLAGGED(victim, AFF2_DISPLACEMENT) &&
        !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        calc_thaco += 2;
    if (AFF2_FLAGGED(victim, AFF2_EVADE))
        calc_thaco += skill_bonus(victim, SKILL_EVASION) / 6;

    if (room_is_watery(ch->in_room) && !IS_NPC(ch))
        calc_thaco += 4;

    calc_thaco -= MIN(5, MAX(0, (POS_FIGHTING - GET_POSITION(victim))));

    calc_thaco -= char_class_race_hit_bonus(ch, victim);

    return calc_thaco;
}

void
add_blood_to_room(struct room_data *rm, int amount)
{
    struct obj_data *blood;
    int new_blood = false;

    if (amount <= 0)
        return;

    for (blood = rm->contents; blood; blood = blood->next_content)
        if (GET_OBJ_VNUM(blood) == BLOOD_VNUM)
            break;

    if (!blood && (new_blood = true) && !(blood = read_object(BLOOD_VNUM))) {
        errlog("Unable to load blood.");
        return;
    }

    if (GET_OBJ_TIMER(blood) > 50)
        return;

    GET_OBJ_TIMER(blood) += amount;

    if (new_blood)
        obj_to_room(blood, rm);

}

int
apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
    int pos)
{

    int cnt, idx;

    if (pos == WEAR_RANDOM) {
        cnt = 0;
        for (idx = 0; idx < NUM_WEARS; idx++) {
            if (ILLEGAL_SOILPOS(idx))
                continue;
            if (!GET_EQ(ch, idx) && CHAR_SOILED(ch, pos, type))
                continue;
            if (GET_EQ(ch, idx) && (OBJ_SOILED(GET_EQ(ch, idx), type) ||
                    IS_OBJ_STAT2(GET_EQ(ch, idx), ITEM2_NOSOIL)))
                continue;
            if (!number(0, cnt))
                pos = idx;
            cnt++;
        }

        // A position will only be unchosen if there are no valid positions
        // with which to soil.
        if (!cnt)
            return 0;
    }

    if (ILLEGAL_SOILPOS(pos))
        return 0;
    if (GET_EQ(ch, pos) && (GET_EQ(ch, pos) == obj || !obj)) {
        if (IS_OBJ_STAT2(GET_EQ(ch, pos), ITEM2_NOSOIL) ||
            OBJ_SOILED(GET_EQ(ch, pos), type))
            return 0;

        SET_BIT(OBJ_SOILAGE(GET_EQ(ch, pos)), type);
    } else if (CHAR_SOILED(ch, pos, type)) {
        return 0;
    } else {
        SET_BIT(CHAR_SOILAGE(ch, pos), type);
    }

    if (type == SOIL_BLOOD && obj && GET_OBJ_VNUM(obj) == BLOOD_VNUM)
        GET_OBJ_TIMER(obj) = MAX(1, GET_OBJ_TIMER(obj) - 5);

    return pos;
}

int limb_probs[] = {
    0,                          //#define WEAR_LIGHT      0
    3,                          //#define WEAR_FINGER_R   1
    3,                          //#define WEAR_FINGER_L   2
    4,                          //#define WEAR_NECK_1     3
    4,                          //#define WEAR_NECK_2     4
    35,                         //#define WEAR_BODY       5
    12,                         //#define WEAR_HEAD       6
    20,                         //#define WEAR_LEGS       7
    5,                          //#define WEAR_FEET       8
    10,                         //#define WEAR_HANDS      9
    25,                         //#define WEAR_ARMS      10
    50,                         //#define WEAR_SHIELD    11
    0,                          //#define WEAR_ABOUT     12
    10,                         //#define WEAR_WAIST     13
    5,                          //#define WEAR_WRIST_R   14
    5,                          //#define WEAR_WRIST_L   15
    0,                          //#define WEAR_WIELD     16
    0,                          //#define WEAR_HOLD      17
    15,                         //#define WEAR_CROTCH    18
    5,                          //#define WEAR_EYES      19
    5,                          //#define WEAR_BACK      20
    0,                          //#define WEAR_BELT      21
    10,                         //#define WEAR_FACE      22
    4,                          //#define WEAR_EAR_L     23
    4,                          //#define WEAR_EAR_R     24
    0,                          //#define WEAR_WIELD_2   25
    0,                          //#define WEAR_ASS       26
};

int
choose_random_limb(struct creature *victim)
{
    int prob;
    int i;
    static int limb_probmax = 0;

    if (!limb_probmax) {
        for (i = 0; i < NUM_WEARS; i++) {
            limb_probmax += limb_probs[i];
            if (i >= 1)
                limb_probs[i] += limb_probs[i - 1];
        }
    }

    prob = number(1, limb_probmax);

    for (i = 1; i < NUM_WEARS; i++) {
        if (prob > limb_probs[i - 1] && prob <= limb_probs[i])
            break;
    }

    if (i >= NUM_WEARS) {
        errlog("exceeded NUM_WEARS-1 in choose_random_limb.");
        return WEAR_BODY;
    }
    // shield will be the only armor check we do here, since it is a special position
    if (i == WEAR_SHIELD) {
        if (!GET_EQ(victim, WEAR_SHIELD)) {
            if (!number(0, 2))
                i = WEAR_ARMS;
            else
                i = WEAR_BODY;
        }
    }

    if (!POS_DAMAGE_OK(i)) {
        errlog("improper pos %d leaving choose_random_limb.", i);
        return WEAR_BODY;
    }

    return i;
}

struct obj_data *
make_corpse(struct creature *ch, struct creature *killer, int attacktype)
{
    struct obj_data *corpse = NULL, *head = NULL, *heart = NULL,
        *spine = NULL, *o = NULL, *next_o = NULL, *leg = NULL;
    struct obj_data *money = NULL;
    int i;
    char typebuf[256] = "";
    char adj[256] = "";
    char namestr[256] = "";
    char isare[16] = "";

    extern int max_npc_corpse_time, max_pc_corpse_time;

    // The Fate's corpses are portals to the remorter.
    if (GET_NPC_SPEC(ch) == fate) { // GetMobSpec checks IS_NPC
        struct obj_data *portal = NULL;
        extern int fate_timers[];
        if ((portal = read_object(FATE_PORTAL_VNUM))) {
            GET_OBJ_TIMER(portal) = max_npc_corpse_time;
            if (GET_NPC_VNUM(ch) == FATE_VNUM_LOW) {
                GET_OBJ_VAL(portal, 2) = 1;
                fate_timers[0] = 0;
            } else if (GET_NPC_VNUM(ch) == FATE_VNUM_MID) {
                GET_OBJ_VAL(portal, 2) = 2;
                fate_timers[1] = 0;
            } else if (GET_NPC_VNUM(ch) == FATE_VNUM_HIGH) {
                GET_OBJ_VAL(portal, 2) = 3;
                fate_timers[2] = 0;
            } else {
                GET_OBJ_VAL(portal, 2) = 12;
            }
            obj_to_room(portal, ch->in_room);
        }
    }
    // End Fate
    if (corpse_state) {
        attacktype = corpse_state;
        corpse_state = 0;
    }
    corpse = make_object();
    corpse->shared = null_obj_shared;
    corpse->in_room = NULL;
    corpse->worn_on = -1;

    if (attacktype == SPELL_PETRIFY)
        GET_OBJ_MATERIAL(corpse) = MAT_STONE;
    else if (IS_ROBOT(ch))
        GET_OBJ_MATERIAL(corpse) = MAT_FLESH;
    else if (IS_SKELETON(ch))
        GET_OBJ_MATERIAL(corpse) = MAT_BONE;
    else if (IS_PUDDING(ch))
        GET_OBJ_MATERIAL(corpse) = MAT_PUDDING;
    else if (IS_SLIME(ch))
        GET_OBJ_MATERIAL(corpse) = MAT_SLIME;
    else if (IS_PLANT(ch))
        GET_OBJ_MATERIAL(corpse) = MAT_VEGETABLE;
    else
        GET_OBJ_MATERIAL(corpse) = MAT_FLESH;

    strcpy(isare, "is");

    if (GET_RACE(ch) == RACE_ROBOT || GET_RACE(ch) == RACE_PLANT ||
        attacktype == TYPE_FALLING) {
        strcpy(isare, "are");
        strcpy(typebuf, "remains");
        strcpy(namestr, typebuf);
    } else if (attacktype == TYPE_SWALLOW
        || GET_CLASS(ch) == CLASS_SKELETON
        || GET_REMORT_CLASS(ch) == CLASS_SKELETON) {
        strcpy(typebuf, "bones");
        strcpy(namestr, typebuf);
    } else {
        strcpy(typebuf, "corpse");
        strcpy(namestr, typebuf);
    }

    if (attacktype == SPELL_PETRIFY) {
        strcat(namestr, " stone");
        strcpy(adj, "stone ");
        strcat(adj, typebuf);
        strcpy(typebuf, adj);
        adj[0] = '\0';
    }
#ifdef DMALLOC
    malloc_verify(0);
#endif

    if ((attacktype == SKILL_BEHEAD ||
            attacktype == SKILL_PELE_KICK || attacktype == SKILL_CLOTHESLINE)
        && isname("headless", ch->player.name)) {
        attacktype = TYPE_HIT;
    }

    switch (attacktype) {
    case SPELL_PETRIFY:
        corpse->line_desc =
            strdup(tmp_sprintf("A stone statue of %s is standing here.",
                GET_NAME(ch)));
        strcpy(adj, "stone statue");
        break;

    case TYPE_HIT:
    case SKILL_BASH:
    case SKILL_PISTOLWHIP:
        corpse->line_desc =
            strdup(tmp_sprintf("The bruised-up %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "bruised");
        break;

    case TYPE_STING:
        corpse->line_desc =
            strdup(tmp_sprintf("The bloody, swollen %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "stung");
        break;

    case TYPE_WHIP:
        corpse->line_desc =
            strdup(tmp_sprintf("The scarred %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "scarred");
        break;

    case TYPE_SLASH:
    case TYPE_CHOP:
    case SPELL_BLADE_BARRIER:
        corpse->line_desc =
            strdup(tmp_sprintf("The chopped up %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "chopped up");
        break;

    case SONG_WOUNDING_WHISPERS:
        corpse->line_desc =
            strdup(tmp_sprintf("The perforated %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "perforated");
        break;

    case SKILL_HAMSTRING:
        corpse->line_desc =
            strdup(tmp_sprintf("The legless %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "legless");

        if (IS_RACE(ch, RACE_BEHOLDER) || NON_CORPOREAL_MOB(ch))
            break;

        leg = make_object();
        leg->shared = null_obj_shared;
        leg->in_room = NULL;

        leg->line_desc =
            strdup(tmp_sprintf("The severed leg of %s %s lying here.",
                GET_NAME(ch), isare));
        leg->name =
            strdup(tmp_sprintf("the severed leg of %s", GET_NAME(ch)));
        GET_OBJ_TYPE(leg) = ITEM_WEAPON;
        GET_OBJ_WEAR(leg) = ITEM_WEAR_TAKE + ITEM_WEAR_WIELD;
        GET_OBJ_EXTRA(leg) = ITEM_NODONATE + ITEM_NOSELL;
        GET_OBJ_EXTRA2(leg) = ITEM2_BODY_PART;
        GET_OBJ_VAL(leg, 0) = 0;
        GET_OBJ_VAL(leg, 1) = 2;
        GET_OBJ_VAL(leg, 2) = 9;
        GET_OBJ_VAL(leg, 3) = 7;
        set_obj_weight(leg, 7);
        leg->worn_on = -1;
        if (IS_NPC(ch))
            GET_OBJ_TIMER(leg) = max_npc_corpse_time;
        else {
            GET_OBJ_TIMER(leg) = max_pc_corpse_time;
        }
        obj_to_room(leg, ch->in_room);
        if (!is_arena_combat(killer, ch) &&
            !is_npk_combat(killer, ch) && GET_LEVEL(ch) <= LVL_AMBASSADOR) {

            /* transfer character's leg EQ to room, if applicable */
            if (GET_EQ(ch, WEAR_LEGS))
                obj_to_room(unequip_char(ch, WEAR_LEGS, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_FEET))
                obj_to_room(unequip_char(ch, WEAR_FEET, EQUIP_WORN),
                    ch->in_room);

        /** transfer implants to leg or corpse randomly**/
            if (GET_IMPLANT(ch, WEAR_LEGS) && number(0, 1)) {
                obj_to_obj(unequip_char(ch, WEAR_LEGS, EQUIP_IMPLANT), leg);
                REMOVE_BIT(GET_OBJ_WEAR(leg), ITEM_WEAR_TAKE);
            }
            if (GET_IMPLANT(ch, WEAR_FEET) && number(0, 1)) {
                obj_to_obj(unequip_char(ch, WEAR_FACE, EQUIP_IMPLANT), leg);
                REMOVE_BIT(GET_OBJ_WEAR(leg), ITEM_WEAR_TAKE);
            }
        }                       // end if !arena room
        break;

    case SKILL_BITE:
    case TYPE_BITE:
        corpse->line_desc =
            strdup(tmp_sprintf("The chewed-up %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "chewed up");
        break;

    case SKILL_SNIPE:
        corpse->line_desc =
            strdup(tmp_sprintf("The sniped %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "sniped");
        break;

    case TYPE_BLUDGEON:
    case TYPE_POUND:
    case TYPE_PUNCH:
        corpse->line_desc =
            strdup(tmp_sprintf("The battered %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "battered");
        break;

    case TYPE_CRUSH:
    case SPELL_PSYCHIC_CRUSH:
        corpse->line_desc =
            strdup(tmp_sprintf("The crushed %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "crushed");
        break;

    case TYPE_CLAW:
        corpse->line_desc =
            strdup(tmp_sprintf("The shredded %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "shredded");
        break;

    case TYPE_MAUL:
        corpse->line_desc =
            strdup(tmp_sprintf("The mauled %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "mauled");
        break;

    case TYPE_THRASH:
        corpse->line_desc =
            strdup(tmp_sprintf("The %s of %s %s lying here, badly thrashed.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "thrashed");
        break;

    case SKILL_BACKSTAB:
        corpse->line_desc =
            strdup(tmp_sprintf("The backstabbed %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "backstabbed");
        break;

    case TYPE_PIERCE:
    case TYPE_STAB:
    case TYPE_EGUN_PARTICLE:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The bloody %s of %s %s lying here, full of holes.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "stabbed");
        break;

    case TYPE_GORE_HORNS:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The gored %s of %s %s lying here in a pool of blood.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "gored");
        break;

    case TYPE_TRAMPLING:
        corpse->line_desc =
            strdup(tmp_sprintf("The trampled %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "trampled");
        break;

    case TYPE_TAIL_LASH:
        corpse->line_desc =
            strdup(tmp_sprintf("The lashed %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "lashed");
        break;

    case TYPE_SWALLOW:
        corpse->line_desc = strdup("A bloody pile of bones is lying here.");
        strcpy(adj, "bloody pile bones");
        break;

    case TYPE_BLAST:
    case SPELL_MAGIC_MISSILE:
    case SKILL_ENERGY_WEAPONS:
    case SPELL_SYMBOL_OF_PAIN:
    case SPELL_DISRUPTION:
    case SPELL_PRISMATIC_SPRAY:
    case SKILL_DISCHARGE:
    case TYPE_EGUN_LASER:
        corpse->line_desc =
            strdup(tmp_sprintf("The blasted %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "blasted");
        break;

    case SKILL_PROJ_WEAPONS:
        corpse->line_desc =
            strdup(tmp_sprintf("The shot up %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "shot up");
        break;

    case SKILL_ARCHERY:
        corpse->line_desc =
            strdup(tmp_sprintf("The pierced %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "pierced");
        break;

    case SPELL_BURNING_HANDS:
    case SPELL_CALL_LIGHTNING:
    case SPELL_FIREBALL:
    case SPELL_FLAME_STRIKE:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_MICROWAVE:
    case SPELL_FIRE_BREATH:
    case SPELL_LIGHTNING_BREATH:
    case SPELL_FIRE_ELEMENTAL:
    case TYPE_ABLAZE:
    case SPELL_METEOR_STORM:
    case SPELL_FIRE_SHIELD:
    case SPELL_HELL_FIRE:
    case TYPE_FLAMETHROWER:
    case SPELL_ELECTRIC_ARC:
    case TYPE_EGUN_PLASMA:
        corpse->line_desc =
            strdup(tmp_sprintf("The charred %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "charred");
        break;

    case SKILL_ENERGY_FIELD:
    case SKILL_SELF_DESTRUCT:
    case TYPE_EGUN_ION:
        corpse->line_desc =
            strdup(tmp_sprintf("The smoking %s of %s %s lying here,", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "smoking");
        break;

    case TYPE_BOILING_PITCH:
        corpse->line_desc =
            strdup(tmp_sprintf("The scorched %s of %s %s here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "scorched");
        break;

    case SPELL_STEAM_BREATH:
        corpse->line_desc =
            strdup(tmp_sprintf("The scalded %s of %s %s here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "scalded");
        break;

    case JAVELIN_OF_LIGHTNING:
    case TYPE_EGUN_LIGHTNING:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The %s of %s %s lying here, blasted and smoking.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "blasted");
        break;

    case SPELL_CONE_COLD:
    case SPELL_CHILL_TOUCH:
    case TYPE_FREEZING:
    case SPELL_HELL_FROST:
    case SPELL_FROST_BREATH:
        corpse->line_desc =
            strdup(tmp_sprintf("The frozen %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "frozen");
        break;

    case SPELL_SPIRIT_HAMMER:
    case SPELL_EARTH_ELEMENTAL:
        corpse->line_desc =
            strdup(tmp_sprintf("The smashed %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "smashed");
        break;

    case SPELL_AIR_ELEMENTAL:
    case TYPE_RIP:
        corpse->line_desc =
            strdup(tmp_sprintf("The ripped apart %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "ripped apart");
        break;

    case SPELL_WATER_ELEMENTAL:
        corpse->line_desc =
            strdup(tmp_sprintf("The drenched %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "drenched");
        break;

    case SPELL_GAMMA_RAY:
    case SPELL_HALFLIFE:
    case TYPE_EGUN_GAMMA:
        corpse->line_desc =
            strdup(tmp_sprintf("The radioactive %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "radioactive");
        break;

    case SPELL_ACIDITY:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The sizzling %s of %s %s lying here, dripping acid.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "sizzling");
        break;

    case SPELL_GAS_BREATH:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The %s of %s lie%s here, stinking of chlorine gas.", typebuf,
                GET_NAME(ch), ISARE(typebuf)));
        strcpy(adj, "chlorinated");
        break;

    case SKILL_TURN:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The burned up %s of %s %s lying here, finally still.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "burned");
        break;

    case TYPE_FALLING:
        corpse->line_desc =
            strdup(tmp_sprintf("The splattered %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "splattered");
        break;

        // attack that tears the victim's spine out
    case SKILL_PILEDRIVE:
        corpse->line_desc =
            strdup(tmp_sprintf
            ("The shattered, twisted %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "shattered");
        if (GET_NPC_VNUM(ch) == 1511) {
            if ((spine = read_object(1541)))
                obj_to_room(spine, ch->in_room);
        } else {
            spine = make_object();
            spine->shared = null_obj_shared;
            spine->in_room = NULL;
            spine->aliases = strdup("spine spinal column bloody");
            spine->line_desc =
                strdup("A bloody spinal column is lying here.");
            spine->name = strdup("a bloody spinal column");
            GET_OBJ_VAL(spine, 1) = 2;

            GET_OBJ_TYPE(spine) = ITEM_WEAPON;
            GET_OBJ_WEAR(spine) = ITEM_WEAR_TAKE + ITEM_WEAR_WIELD;
            GET_OBJ_EXTRA(spine) = ITEM_NODONATE + ITEM_NOSELL;
            GET_OBJ_EXTRA2(spine) = ITEM2_BODY_PART;
            if (GET_LEVEL(ch) > number(30, 59))
                SET_BIT(GET_OBJ_EXTRA(spine), ITEM_HUM);
            GET_OBJ_VAL(spine, 0) = 0;
            GET_OBJ_VAL(spine, 2) = 6;
            GET_OBJ_VAL(spine, 3) = 5;
            set_obj_weight(spine, 5);
            GET_OBJ_MATERIAL(spine) = MAT_BONE;
            spine->worn_on = -1;
            obj_to_room(spine, ch->in_room);
        }
        break;

    case SPELL_TAINT:
    case TYPE_TAINT_BURN:
        if (IS_RACE(ch, RACE_BEHOLDER) || NON_CORPOREAL_MOB(ch)) {
            // attack that rips the victim's head off
            corpse->line_desc =
                strdup(tmp_sprintf("The smoking %s of %s %s lying here.",
                    typebuf, GET_NAME(ch), isare));
            sprintf(adj, "smoking");
            break;
        } else {
            // attack that rips the victim's head off
            corpse->line_desc =
                strdup(tmp_sprintf
                ("The headless smoking %s of %s %s lying here.", typebuf,
                    GET_NAME(ch), isare));
            sprintf(adj, "headless smoking");
        }

        if (!is_arena_combat(killer, ch) &&
            !is_npk_combat(killer, ch) && GET_LEVEL(ch) <= LVL_AMBASSADOR) {
            struct obj_data *o;
            /* transfer character's head EQ to room, if applicable */
            if (GET_EQ(ch, WEAR_HEAD))
                obj_to_room(unequip_char(ch, WEAR_HEAD, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_FACE))
                obj_to_room(unequip_char(ch, WEAR_FACE, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EAR_L))
                obj_to_room(unequip_char(ch, WEAR_EAR_L, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EAR_R))
                obj_to_room(unequip_char(ch, WEAR_EAR_R, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EYES))
                obj_to_room(unequip_char(ch, WEAR_EYES, EQUIP_WORN),
                    ch->in_room);
            /** transfer implants to ground **/
            if ((o = GET_IMPLANT(ch, WEAR_HEAD))) {
                obj_to_room(unequip_char(ch, WEAR_HEAD, EQUIP_IMPLANT),
                    ch->in_room);
                SET_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_FACE))) {
                obj_to_room(unequip_char(ch, WEAR_FACE, EQUIP_IMPLANT),
                    ch->in_room);
                SET_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EAR_L))) {
                obj_to_room(unequip_char(ch, WEAR_EAR_L, EQUIP_IMPLANT),
                    ch->in_room);
                SET_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EAR_R))) {
                obj_to_room(unequip_char(ch, WEAR_EAR_R, EQUIP_IMPLANT),
                    ch->in_room);
                SET_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EYES))) {
                obj_to_room(unequip_char(ch, WEAR_EYES, EQUIP_IMPLANT),
                    ch->in_room);
                SET_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
        }                       // end if !arena room
        break;
    case SKILL_BEHEAD:
    case SKILL_PELE_KICK:
    case SKILL_CLOTHESLINE:
        corpse->line_desc =
            strdup(tmp_sprintf("The headless %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        sprintf(adj, "headless");

        if (IS_RACE(ch, RACE_BEHOLDER) || NON_CORPOREAL_MOB(ch))
            break;

        head = make_object();
        head->shared = null_obj_shared;
        head->in_room = NULL;
        head->aliases = strdup("blood head skull");

        head->line_desc =
            strdup(tmp_sprintf("The severed head of %s %s lying here.",
                               GET_NAME(ch), isare));
        head->name =
            strdup(tmp_sprintf("the severed head of %s", GET_NAME(ch)));
        GET_OBJ_TYPE(head) = ITEM_DRINKCON;
        GET_OBJ_WEAR(head) = ITEM_WEAR_TAKE;
        GET_OBJ_EXTRA(head) = ITEM_NODONATE | ITEM_NOSELL;
        GET_OBJ_EXTRA2(head) = ITEM2_BODY_PART;
        GET_OBJ_VAL(head, 0) = 5;   /* Head full of blood */
        GET_OBJ_VAL(head, 1) = 5;
        GET_OBJ_VAL(head, 2) = 13;

        head->worn_on = -1;

        GET_OBJ_MATERIAL(head) = GET_OBJ_MATERIAL(corpse);
        set_obj_weight(head, 10);

        if (IS_NPC(ch))
            GET_OBJ_TIMER(head) = max_npc_corpse_time;
        else {
            GET_OBJ_TIMER(head) = max_pc_corpse_time;
        }
        obj_to_room(head, ch->in_room);
        if (!is_arena_combat(killer, ch) &&
            !is_npk_combat(killer, ch) && GET_LEVEL(ch) <= LVL_AMBASSADOR) {
            struct obj_data *o;
            /* transfer character's head EQ to room, if applicable */
            if (GET_EQ(ch, WEAR_HEAD))
                obj_to_room(unequip_char(ch, WEAR_HEAD, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_FACE))
                obj_to_room(unequip_char(ch, WEAR_FACE, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EAR_L))
                obj_to_room(unequip_char(ch, WEAR_EAR_L, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EAR_R))
                obj_to_room(unequip_char(ch, WEAR_EAR_R, EQUIP_WORN),
                    ch->in_room);
            if (GET_EQ(ch, WEAR_EYES))
                obj_to_room(unequip_char(ch, WEAR_EYES, EQUIP_WORN),
                    ch->in_room);
            /** transfer implants to head **/
            if ((o = GET_IMPLANT(ch, WEAR_HEAD))) {
                obj_to_obj(unequip_char(ch, WEAR_HEAD, EQUIP_IMPLANT), head);
                REMOVE_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_FACE))) {
                obj_to_obj(unequip_char(ch, WEAR_FACE, EQUIP_IMPLANT), head);
                REMOVE_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EAR_L))) {
                obj_to_obj(unequip_char(ch, WEAR_EAR_L, EQUIP_IMPLANT), head);
                REMOVE_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EAR_R))) {
                obj_to_obj(unequip_char(ch, WEAR_EAR_R, EQUIP_IMPLANT), head);
                REMOVE_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
            if ((o = GET_IMPLANT(ch, WEAR_EYES))) {
                obj_to_obj(unequip_char(ch, WEAR_EYES, EQUIP_IMPLANT), head);
                REMOVE_BIT(GET_OBJ_WEAR(o), ITEM_WEAR_TAKE);
            }
        }                       // end if !arena room
        break;

        // attack that rips the victim's heart out
    case SKILL_LUNGE_PUNCH:
        corpse->line_desc =
            strdup(tmp_sprintf("The maimed %s of %s %s lying here.", typebuf,
                GET_NAME(ch), isare));
        strcpy(adj, "maimed");

        heart = make_object();
        heart->shared = null_obj_shared;
        heart->in_room = NULL;
        GET_OBJ_TYPE(heart) = ITEM_FOOD;
        heart->aliases = strdup("heart bloody");
        heart->name =
            strdup(tmp_sprintf("the bloody heart of %s", GET_NAME(ch)));

        heart->line_desc =
            strdup(tmp_sprintf("The heart of %s %s lying here.",
                               GET_NAME(ch), isare));

        GET_OBJ_WEAR(heart) = ITEM_WEAR_TAKE + ITEM_WEAR_HOLD;
        GET_OBJ_EXTRA(heart) = ITEM_NODONATE | ITEM_NOSELL;
        GET_OBJ_EXTRA2(heart) = ITEM2_BODY_PART;
        GET_OBJ_VAL(heart, 0) = 10;
        if (IS_DEVIL(ch) || IS_DEMON(ch) || IS_LICH(ch)) {

            if (GET_CLASS(ch) == CLASS_GREATER || GET_CLASS(ch) == CLASS_ARCH
                || GET_CLASS(ch) == CLASS_DUKE
                || GET_CLASS(ch) == CLASS_DEMON_PRINCE
                || GET_CLASS(ch) == CLASS_DEMON_LORD
                || GET_CLASS(ch) == CLASS_LESSER || GET_LEVEL(ch) > 30) {
                GET_OBJ_VAL(heart, 1) = GET_LEVEL(ch);
                GET_OBJ_VAL(heart, 2) = SPELL_ESSENCE_OF_EVIL;
                if (GET_CLASS(ch) == CLASS_LESSER) {
                    GET_OBJ_VAL(heart, 1) /= 2;
                }
            } else {
                GET_OBJ_VAL(heart, 1) = 0;
                GET_OBJ_VAL(heart, 2) = 0;
            }
        } else {
            GET_OBJ_VAL(heart, 1) = 0;
            GET_OBJ_VAL(heart, 2) = 0;
        }

        set_obj_weight(heart, 0.5);
        heart->worn_on = -1;

        if (IS_NPC(ch))
            GET_OBJ_TIMER(heart) = max_npc_corpse_time;
        else {
            GET_OBJ_TIMER(heart) = max_pc_corpse_time;
        }
        obj_to_room(heart, ch->in_room);
        break;

    case SKILL_IMPALE:
        corpse->line_desc =
            strdup(tmp_sprintf("The run through %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "impaled");
        break;

    case TYPE_DROWNING:
        if (room_is_watery(ch->in_room))
            corpse->line_desc =
                strdup(tmp_sprintf("The waterlogged %s of %s %s lying here.",
                    typebuf, GET_NAME(ch), ISARE(typebuf)));
        else
            corpse->line_desc =
                strdup(tmp_sprintf("The %s of %s %s lying here.", typebuf,
                    GET_NAME(ch), ISARE(typebuf)));
        strcpy(adj, "drowned");
        break;

    default:
        corpse->line_desc = strdup(tmp_sprintf("The %s of %s %s lying here.",
                typebuf, GET_NAME(ch), isare));
        strcpy(adj, "");
        break;
    }

    //  make the short name
    switch (attacktype) {
    case TYPE_SWALLOW:
        corpse->name = strdup("a bloody pile of bones");
        break;
    case SPELL_PETRIFY:
        corpse->name = strdup(tmp_sprintf("a stone statue of %s", GET_NAME(ch)));
        break;
    default:
        corpse->name =
            strdup(tmp_sprintf("the %s%s%s of %s", adj, *adj ? " " : "",
                typebuf, GET_NAME(ch)));
        break;
    }

    // make the alias list
    strcat(strcat(strcat(strcat(namestr, " "), adj), " "), ch->player.name);
    if (namestr[strlen(namestr)] == ' ')
        namestr[strlen(namestr)] = '\0';
    corpse->aliases = strdup(namestr);

    // now flesh out the other vairables on the corpse
    GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
    GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
    if (is_tester(ch))
        SET_BIT(GET_OBJ_EXTRA2(corpse), ITEM2_UNAPPROVED);
    GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */
    GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */
    set_obj_weight(corpse, GET_WEIGHT(ch));
    corpse->contains = NULL;

    if (IS_NPC(ch)) {
        GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
        CORPSE_IDNUM(corpse) = -ch->mob_specials.shared->vnum;
        corpse->obj_flags.max_dam = corpse->obj_flags.damage = 100;
    } else {
        GET_OBJ_TIMER(corpse) = max_pc_corpse_time;
        CORPSE_IDNUM(corpse) = GET_IDNUM(ch);
        corpse->obj_flags.max_dam = corpse->obj_flags.damage = -1;  //!break player corpses
    }

    if (killer) {
        if (IS_NPC(killer))
            CORPSE_KILLER(corpse) = -GET_NPC_VNUM(killer);
        else
            CORPSE_KILLER(corpse) = GET_IDNUM(killer);
    } else if (dam_object)
        CORPSE_KILLER(corpse) = DAM_OBJECT_IDNUM(dam_object);

    // if non-arena room, transfer eq to corpse
    bool lose_eq = (!is_arena_combat(killer, ch) || IS_NPC(ch))
        && GET_LEVEL(ch) < LVL_AMBASSADOR;

    bool lose_implants = !(is_npk_combat(killer, ch) ||
        is_arena_combat(killer, ch) ||
        (IS_NPC(ch) && GET_LEVEL(ch) > LVL_AMBASSADOR));

    bool lose_tattoos = lose_eq;

    struct obj_data *next_obj;

    /* transfer character's inventory to the corpse */
    for (o = ch->carrying; o != NULL; o = next_obj) {
        next_obj = o->next_content;
        if (lose_eq || obj_is_unrentable(o)) {
            obj_from_char(o);
            obj_to_obj(o, corpse);
        }
    }

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, i) && (lose_eq || obj_is_unrentable(GET_EQ(ch, i))))
            obj_to_obj(raw_unequip_char(ch, i, EQUIP_WORN), corpse);
        if (GET_IMPLANT(ch, i) && (lose_implants
                || obj_is_unrentable(GET_IMPLANT(ch, i)))) {
            REMOVE_BIT(GET_OBJ_WEAR(GET_IMPLANT(ch, i)), ITEM_WEAR_TAKE);
            obj_to_obj(raw_unequip_char(ch, i, EQUIP_IMPLANT), corpse);
        }
        // Tattoos get discarded
        if (GET_TATTOO(ch, i) && lose_tattoos)
            extract_obj(raw_unequip_char(ch, i, EQUIP_TATTOO));
    }

    /* transfer gold */
    if (GET_GOLD(ch) > 0 && lose_eq) {
        /* following 'if' clause added to fix gold duplication loophole */
        if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
            if ((money = create_money(GET_GOLD(ch), 0)))
                obj_to_obj(money, corpse);
        }
        GET_GOLD(ch) = 0;
    }
    if (GET_CASH(ch) > 0 && lose_eq) {
        /* following 'if' clause added to fix gold duplication loophole */
        if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
            if ((money = create_money(GET_CASH(ch), 1)))
                obj_to_obj(money, corpse);
        }
        GET_CASH(ch) = 0;
    }
    if (lose_eq) {
        ch->carrying = NULL;
        IS_CARRYING_N(ch) = 0;
        IS_CARRYING_W(ch) = 0;
    }
    // Remove all script objects if not an immortal
    if (!IS_IMMORT(ch)) {
        for (o = corpse->contains; o; o = next_o) {
            next_o = o->next_content;
            if (IS_OBJ_TYPE(o, ITEM_SCRIPT))
                extract_obj(o);
        }
    }
    // leave no corpse behind
    if (NON_CORPOREAL_MOB(ch) || GET_NPC_SPEC(ch) == fate) {
        while ((o = corpse->contains)) {
            obj_from_obj(o);
            obj_to_room(o, ch->in_room);
        }
        extract_obj(corpse);
        corpse = NULL;
    } else {
        obj_to_room(corpse, ch->in_room);
        if (CORPSE_IDNUM(corpse) > 0 && !is_arena_combat(killer, ch)) {
            FILE *corpse_file;
            char *fname;

            fname = get_corpse_file_path(CORPSE_IDNUM(corpse));
            if ((corpse_file = fopen(fname, "w+")) != NULL) {
                fprintf(corpse_file, "<corpse>");
                save_object_to_xml(corpse, corpse_file);
                fprintf(corpse_file, "</corpse>");
                fclose(corpse_file);
            } else {
                errlog("Failed to open corpse file [%s] (%s)", fname,
                    strerror(errno));
            }
        }
    }

    return corpse;
}

int
calculate_weapon_probability(struct creature *ch, int prob,
    struct obj_data *weap)
{
    int i, weap_weight;

    if (!ch || !weap)
        return prob;

    // Add in weapon specialization bonus.
    if (GET_OBJ_VNUM(weap) > 0) {
        for (i = 0; i < MAX_WEAPON_SPEC; i++) {
            if (GET_WEAP_SPEC(ch, i).vnum == GET_OBJ_VNUM(weap)) {
                prob += GET_WEAP_SPEC(ch, i).level * 4;
                break;
            }
        }
    }
    // Below this check applies to WIELD and WIELD_2 only!
    if (weap->worn_on != WEAR_WIELD && weap->worn_on != WEAR_WIELD_2) {
        return prob;
    }
    // Weapon speed
    for (i = 0, weap_weight = 0; i < MAX_OBJ_AFFECT; i++) {
        if (weap->affected[i].location == APPLY_WEAPONSPEED) {
            weap_weight -= weap->affected[i].modifier;
            break;
        }
    }
    // 1/4 actual weapon weight or effective weapon weight, whichever is higher.
    weap_weight += GET_OBJ_WEIGHT(weap);
    weap_weight = MAX((GET_OBJ_WEIGHT(weap) / 4), weap_weight);

    if (weap->worn_on == WEAR_WIELD_2) {
        prob -=
            (prob * weap_weight) /
            MAX(1, (strength_wield_weight(GET_STR(ch)) / 2));
        if (affected_by_spell(ch, SKILL_NEURAL_BRIDGING)) {
            prob += CHECK_SKILL(ch, SKILL_NEURAL_BRIDGING) - 60;
        } else {
            if (CHECK_SKILL(ch, SKILL_SECOND_WEAPON) >= LEARNED(ch)) {
                prob += CHECK_SKILL(ch, SKILL_SECOND_WEAPON) - 60;
            }
        }
    } else {
        prob -=
            (prob * weap_weight) /
            (strength_wield_weight(GET_STR(ch)) * 2);
        if (IS_BARB(ch)) {
            prob += (LEARNED(ch) - weapon_prof(ch, weap)) / 8;
        }
    }
    return prob;
}

int
calculate_attack_probability(struct creature *ch)
{
    int prob;
    struct obj_data *weap = NULL;

    if (!is_fighting(ch))
        return 0;

    prob = 1 + (GET_LEVEL(ch) / 7) + (GET_DEX(ch) * 2);

    // Penalize rangers for wearing metal body armor
    if (IS_RANGER(ch)
        && GET_EQ(ch, WEAR_BODY)
        && IS_OBJ_TYPE(GET_EQ(ch, WEAR_BODY), ITEM_ARMOR)
        && IS_METAL_TYPE(GET_EQ(ch, WEAR_BODY)))
        prob -= (GET_LEVEL(ch) / 4);

    if (GET_EQ(ch, WEAR_WIELD_2))
        prob =
            calculate_weapon_probability(ch, prob, GET_EQ(ch, WEAR_WIELD_2));

    if (GET_EQ(ch, WEAR_WIELD))
        prob = calculate_weapon_probability(ch, prob, GET_EQ(ch, WEAR_WIELD));

    if (GET_EQ(ch, WEAR_HANDS))
        prob = calculate_weapon_probability(ch, prob, GET_EQ(ch, WEAR_HANDS));

    prob += ((POS_FIGHTING - (GET_POSITION(random_opponent(ch)))) * 2);

    if (CHECK_SKILL(ch, SKILL_DBL_ATTACK))
        prob += (int)((CHECK_SKILL(ch, SKILL_DBL_ATTACK) * 0.15) +
            (CHECK_SKILL(ch, SKILL_TRIPLE_ATTACK) * 0.17));

    if (CHECK_SKILL(ch, SKILL_MELEE_COMBAT_TAC) &&
        affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC))
        prob += (int)(CHECK_SKILL(ch, SKILL_MELEE_COMBAT_TAC) * 0.10);

    if (affected_by_spell(ch, SKILL_OFFENSIVE_POS))
        prob += (int)(CHECK_SKILL(ch, SKILL_OFFENSIVE_POS) * 0.10);
    else if (affected_by_spell(ch, SKILL_DEFENSIVE_POS))
        prob -= (int)(CHECK_SKILL(ch, SKILL_DEFENSIVE_POS) * 0.05);

    if (IS_MERC(ch) && ((((weap = GET_EQ(ch, WEAR_WIELD)) && IS_GUN(weap)) ||
                ((weap = GET_EQ(ch, WEAR_WIELD_2)) && IS_GUN(weap))) &&
            CHECK_SKILL(ch, SKILL_SHOOT) > 50))
        prob += (int)(CHECK_SKILL(ch, SKILL_SHOOT) * 0.18);

    if (AFF_FLAGGED(ch, AFF_ADRENALINE))
        prob = (int)(prob * 1.10);

    if (AFF2_FLAGGED(ch, AFF2_HASTE))
        prob = (int)(prob * 1.30);

    if (SPEED_OF(ch))
        prob += (prob * SPEED_OF(ch)) / 100;

    if (AFF2_FLAGGED(ch, AFF2_SLOW))
        prob = (int)(prob * 0.70);

    if (SECT(ch->in_room) == SECT_ELEMENTAL_OOZE)
        prob = (int)(prob * 0.70);

    if (AFF2_FLAGGED(ch, AFF2_BERSERK))
        prob += (GET_LEVEL(ch) + (GET_REMORT_GEN(ch) * 4)) / 2;

    if (IS_MONK(ch))
        prob += GET_LEVEL(ch) / 4;

    if (AFF3_FLAGGED(ch, AFF3_DIVINE_POWER))
        prob += (skill_bonus(ch, SPELL_DIVINE_POWER) / 3);

    if (ch->desc)
        prob -= ((MAX(0, ch->desc->wait / 2)) * prob) / 100;
    else
        prob -= ((MAX(0, GET_NPC_WAIT(ch) / 2)) * prob) / 100;

    prob -= ((((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 32) * prob) /
        (CAN_CARRY_W(ch) * 85));

    if (GET_COND(ch, DRUNK) > 5)
        prob -= (int)((prob * 0.15) + (prob * (GET_COND(ch, DRUNK) / 100)));

    return prob;
}

#undef __combat_code__
#undef __combat_utils__
