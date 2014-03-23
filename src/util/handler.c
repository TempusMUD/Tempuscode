/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: handler.c                      -- Part of TempusMUD
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
#include "clan.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "fight.h"
#include "obj_data.h"
#include "actions.h"
#include "weather.h"
#include "prog.h"
#include "smokes.h"
#include "strutil.h"

/* external vars */
extern struct descriptor_data *descriptor_list;

/* external functions */
long special(struct creature *ch, int cmd, int subcmd, char *arg,
    enum special_mode spec_mode);
void path_remove_object(void *object);
void free_paths();
void free_socials();
void print_attributes_to_buf(struct creature *ch, char *buff);
extern struct clan_data *clan_list;

void
init_affect(struct affected_type *af)
{
    af->type = 0;
    af->duration = 0;
    af->modifier = 0;
    af->location = APPLY_NONE;
    af->level = 0;
    af->is_instant = 0;
    af->bitvector = 0;
    af->aff_index = 0;
    af->owner = 0;
    af->next = NULL;
}

void
apply_object_affects(struct creature *ch, struct obj_data *obj, bool add)
{
    if (!obj)
        return;

    if (obj->worn_by != ch && (!obj->in_obj || !(IS_INTERFACE(obj->in_obj)
                && INTERFACE_TYPE(obj->in_obj) == INTERFACE_CHIPS
                && IS_CHIP(obj))))
        return;

    if (invalid_char_class(ch, obj))
        return;

    if (obj == ch->equipment[obj->worn_on]) {
        if (obj->worn_on == WEAR_BELT
            && (IS_OBJ_TYPE(obj, ITEM_WEAPON) ||
                IS_OBJ_TYPE(obj, ITEM_PIPE)))
            return;
    } else {
        // We only check for this when implanted or tattooed because
        // check_eq_align() will pop the object off if the align is
        // bad.  We're doing this to avoid apply asymmetry in the
        // event where someone changes to a bad align from a good one.
        if ((IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
            || (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
            || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
            return;

    }

    if (IS_DEVICE(obj) && !ENGINE_STATE(obj))
        return;

    // Apply object affects
    for (int j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify(ch, obj->affected[j].location,
            obj->affected[j].modifier, 0, 0, add);
    affect_modify(ch, 0, 0, obj->obj_flags.bitvector[0], 1, add);
    affect_modify(ch, 0, 0, obj->obj_flags.bitvector[1], 2, add);
    affect_modify(ch, 0, 0, obj->obj_flags.bitvector[2], 3, add);

    // Special skill chip affects
    if (IS_CHIP(obj)
        && SKILLCHIP(obj)
        && CHIP_DATA(obj) > 0 && CHIP_DATA(obj) < MAX_SKILLS)
        affect_modify(ch, -CHIP_DATA(obj), CHIP_MAX(obj), 0, 0, add);

    // Apply affects of contained chips, if this is a chip interface
    if (IS_INTERFACE(obj)
        && INTERFACE_TYPE(obj) == INTERFACE_CHIPS && obj->contains) {
        for (struct obj_data * chip = obj->contains; chip;
            chip = chip->next_content)
            apply_object_affects(ch, chip, add);
    }
}

#define APPLY_SKILL(ch, skill, mod) \
GET_SKILL(ch, skill) = \
MIN(GET_SKILL(ch, skill) + mod, 125)

void
affect_modify(struct creature *ch, int16_t loc, int16_t mod, long bitv,
    int index, bool add)
{
    if (bitv) {
        if (add) {
            if (index == 2) {
				if (ch->in_room
                    && ((bitv == AFF2_FLUORESCENT) ||
						(bitv == AFF2_DIVINE_ILLUMINATION))
                    && !AFF_FLAGGED(ch, AFF_GLOWLIGHT)
                    && !AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
                    && !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)
                    && !affected_by_spell(ch, SPELL_QUAD_DAMAGE))
                    ch->in_room->light++;
                SET_BIT(AFF2_FLAGS(ch), bitv);
            } else if (index == 3) {
                SET_BIT(AFF3_FLAGS(ch), bitv);
            } else {
				if (bitv == AFF_GLOWLIGHT
                    && ch->in_room
                    && !AFF_FLAGGED(ch, AFF_GLOWLIGHT)
                    && !AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
                    && !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)
                    && !affected_by_spell(ch, SPELL_QUAD_DAMAGE))
                    ch->in_room->light++;
                SET_BIT(AFF_FLAGS(ch), bitv);
            }

        } else {                /* remove aff (!add) */
            if (index == 2) {
                REMOVE_BIT(AFF2_FLAGS(ch), bitv);
				if (ch->in_room
                    && ((bitv == AFF2_FLUORESCENT)
                        || (bitv == AFF2_DIVINE_ILLUMINATION))
                    && !AFF_FLAGGED(ch, AFF_GLOWLIGHT)
                    && !AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
                    && !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)
                    && !affected_by_spell(ch, SPELL_QUAD_DAMAGE))
                    ch->in_room->light--;
            } else if (index == 3)
                REMOVE_BIT(AFF3_FLAGS(ch), bitv);
            else {
                REMOVE_BIT(AFF_FLAGS(ch), bitv);
				if (bitv == AFF_GLOWLIGHT
                    && ch->in_room
                    && !AFF_FLAGGED(ch, AFF_GLOWLIGHT)
                    && !AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
                    && !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)
                    && !affected_by_spell(ch, SPELL_QUAD_DAMAGE))
                    ch->in_room->light--;

            }
            mod = -mod;
        }
    } else if (!add)
        mod = -mod;

    if (loc < 0) {
        loc = -loc;
        if (loc < MAX_SKILLS && !IS_NPC(ch))
            APPLY_SKILL(ch, loc, mod);
        return;
    }

    switch (loc) {
    case APPLY_NONE:
        break;
    case APPLY_STR:
        GET_STR(ch) += mod;
        break;
    case APPLY_DEX:
        GET_DEX(ch) += mod;
        break;
    case APPLY_INT:
        GET_INT(ch) += mod;
        break;
    case APPLY_WIS:
        GET_WIS(ch) += mod;
        break;
    case APPLY_CON:
        GET_CON(ch) += mod;
        break;
    case APPLY_CHA:
        GET_CHA(ch) += mod;
        break;
    case APPLY_CLASS:
        break;
    case APPLY_LEVEL:
        break;
    case APPLY_AGE:
        ch->player.age_adjust += mod;
        break;
    case APPLY_CHAR_WEIGHT:
        GET_WEIGHT(ch) += mod;
        break;
    case APPLY_CHAR_HEIGHT:
        GET_HEIGHT(ch) += mod;
        break;
    case APPLY_MANA:
        GET_MAX_MANA(ch) += mod;
        break;
    case APPLY_HIT:
        GET_MAX_HIT(ch) += mod;
        break;
    case APPLY_MOVE:
        GET_MAX_MOVE(ch) += mod;
        break;
    case APPLY_GOLD:
        break;
    case APPLY_EXP:
        break;
    case APPLY_AC:
        GET_AC(ch) += mod;
        break;
    case APPLY_HITROLL:
        if (GET_HITROLL(ch) + mod > 125)
            GET_HITROLL(ch) = MIN(125, GET_HITROLL(ch) + mod);
        else if (GET_HITROLL(ch) + mod < -125)
            GET_HITROLL(ch) = MAX(-125, GET_HITROLL(ch) + mod);
        else
            GET_HITROLL(ch) += mod;
        break;
    case APPLY_DAMROLL:
        if (GET_DAMROLL(ch) + mod > 125)
            GET_DAMROLL(ch) = MIN(125, GET_DAMROLL(ch) + mod);
        else if (GET_DAMROLL(ch) + mod < -125)
            GET_DAMROLL(ch) = MAX(-125, GET_DAMROLL(ch) + mod);
        else
            GET_DAMROLL(ch) += mod;
        break;
    case APPLY_SAVING_PARA:
        GET_SAVE(ch, SAVING_PARA) += mod;
        break;
    case APPLY_SAVING_ROD:
        GET_SAVE(ch, SAVING_ROD) += mod;
        break;
    case APPLY_SAVING_PETRI:
        GET_SAVE(ch, SAVING_PETRI) += mod;
        break;
    case APPLY_SAVING_BREATH:
        GET_SAVE(ch, SAVING_BREATH) += mod;
        break;
    case APPLY_SAVING_SPELL:
        GET_SAVE(ch, SAVING_SPELL) += mod;
        break;
    case APPLY_SAVING_CHEM:
        GET_SAVE(ch, SAVING_CHEM) += mod;
        break;
    case APPLY_SAVING_PSI:
        GET_SAVE(ch, SAVING_PSI) += mod;
        break;
    case APPLY_SNEAK:
        APPLY_SKILL(ch, SKILL_SNEAK, mod);
        break;
    case APPLY_HIDE:
        APPLY_SKILL(ch, SKILL_HIDE, mod);
        break;
    case APPLY_RACE:
        if (mod >= 0 && mod < NUM_RACES)
            GET_RACE(ch) = mod;
        break;
    case APPLY_SEX:
        if (mod == 0 || mod == 1 || mod == 2)
            GET_SEX(ch) = mod;
        break;
    case APPLY_BACKSTAB:
        APPLY_SKILL(ch, SKILL_BACKSTAB, mod);
        break;
    case APPLY_PICK_LOCK:
        APPLY_SKILL(ch, SKILL_PICK_LOCK, mod);
        break;
    case APPLY_PUNCH:
        APPLY_SKILL(ch, SKILL_PUNCH, mod);
        break;
    case APPLY_SHOOT:
        APPLY_SKILL(ch, SKILL_SHOOT, mod);
        break;
    case APPLY_KICK:
        APPLY_SKILL(ch, SKILL_KICK, mod);
        break;
    case APPLY_TRACK:
        APPLY_SKILL(ch, SKILL_TRACK, mod);
        break;
    case APPLY_IMPALE:
        APPLY_SKILL(ch, SKILL_IMPALE, mod);
        break;
    case APPLY_BEHEAD:
        APPLY_SKILL(ch, SKILL_BEHEAD, mod);
        break;
    case APPLY_THROWING:
        APPLY_SKILL(ch, SKILL_THROWING, mod);
        break;
    case APPLY_RIDING:
        APPLY_SKILL(ch, SKILL_RIDING, mod);
        break;
    case APPLY_TURN:
        APPLY_SKILL(ch, SKILL_TURN, mod);
        break;
    case APPLY_ALIGN:
        break;
    case APPLY_SAVING_PHY:
        GET_SAVE(ch, SAVING_PHY) += mod;
        break;
    case APPLY_CASTER:         // special usage
    case APPLY_WEAPONSPEED:    // special usage
    case APPLY_DISGUISE:       // special usage
        break;

        // never set items with negative nothirst, nohunger, or nodrunk
    case APPLY_NOTHIRST:
        if (IS_NPC(ch))
            break;
        if (GET_COND(ch, THIRST) != -1) {
            if (mod > 0)
                GET_COND(ch, THIRST) = -2;
            else
                GET_COND(ch, THIRST) = 0;
        }
        break;
    case APPLY_NOHUNGER:
        if (IS_NPC(ch))
            break;
        if (GET_COND(ch, FULL) != -1) {
            if (mod > 0)
                GET_COND(ch, FULL) = -2;
            else
                GET_COND(ch, FULL) = 0;
            break;
        }
    case APPLY_NODRUNK:
        if (IS_NPC(ch))
            break;
        if (mod > 0)
            GET_COND(ch, DRUNK) = -1;
        else
            GET_COND(ch, DRUNK) = 0;
        break;
    case APPLY_SPEED:
        SPEED_OF(ch) += mod;
        break;

    default:
        errlog
            ("Unknown apply adjust attempt on %20s %3d + %3d in affect_modify. add=%s",
            GET_NAME(ch), loc, mod, add ? "true" : "false");
        break;

    }                           /* switch */
}

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void
affect_total(struct creature *ch)
{
    struct affected_type *af;
    int i;

    // remove all item-based affects
    for (i = 0; i < NUM_WEARS; i++) {
        apply_object_affects(ch, ch->equipment[i], false);
        apply_object_affects(ch, ch->implants[i], false);
        apply_object_affects(ch, ch->tattoos[i], false);
    }

    // remove all spell affects
    for (af = ch->affected; af; af = af->next)
        affect_modify(ch, af->location, af->modifier, af->bitvector,
            af->aff_index, false);

    /************************************************************************
     * Set stats to real stats                                              *
     ************************************************************************/

    ch->aff_abils = ch->real_abils;

    for (i = 0; i < 10; i++)
        GET_SAVE(ch, i) = 0;

    struct race *race = race_by_idnum(GET_RACE(ch));
    if (race) {
        AFF_FLAGS(ch) |= race->aff1;
        AFF2_FLAGS(ch) |= race->aff2;
        AFF3_FLAGS(ch) |= race->aff3;
    }

    if (IS_NPC(ch) && ch->mob_specials.shared->proto) {
        GET_HITROLL(ch) = ch->mob_specials.shared->proto->points.hitroll;
        GET_DAMROLL(ch) = ch->mob_specials.shared->proto->points.damroll;
    } else {
        GET_HITROLL(ch) = 0;
        GET_DAMROLL(ch) = 0;
    }

    SPEED_OF(ch) = 0;



    /************************************************************************
     * Reset affected stats                                                 *
     ************************************************************************/

    // re-apply all item-based affects
    for (i = 0; i < NUM_WEARS; i++) {
        apply_object_affects(ch, ch->equipment[i], true);
        apply_object_affects(ch, ch->implants[i], true);
        apply_object_affects(ch, ch->tattoos[i], true);
    }
    for (af = ch->affected; af; af = af->next)
        affect_modify(ch, af->location, af->modifier, af->bitvector,
            af->aff_index, true);

    /* Make certain values are between 0..50, not < 0 and not > 50! */
    GET_STR(ch) = MAX(0, MIN(GET_STR(ch), 50)); /* str is a special case atm */
    GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), 50));
    GET_INT(ch) = MAX(0, MIN(GET_INT(ch), 50));
    GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), 50));
    GET_CON(ch) = MAX(0, MIN(GET_CON(ch), 50));
    GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), 50));

    /* Make sure that HIT < MAX_HIT, etc...               */

    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch));
    GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch));
    GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch));
}

/* Insert an affect_type in a struct creature structure
   Automatically sets apropriate bits and apply's */
void
affect_to_char(struct creature *ch, struct affected_type *af)
{
    struct affected_type *affected_alloc;
    struct affected_type *prev_quad = affected_by_spell(ch, SPELL_QUAD_DAMAGE);

    CREATE(affected_alloc, struct affected_type, 1);

    memcpy(affected_alloc, af, sizeof(struct affected_type));
    affected_alloc->next = ch->affected;
    ch->affected = affected_alloc;

    affect_modify(ch, af->location, af->modifier,
        af->bitvector, af->aff_index, true);
    affect_total(ch);
	if (af->type == SPELL_QUAD_DAMAGE
        && ch->in_room
        && !AFF_FLAGGED(ch, AFF_GLOWLIGHT)
        && !AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
        && !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)
        && !prev_quad)
        ch->in_room->light++;

    if (IS_SET(af->bitvector, AFF3_SELF_DESTRUCT) && af->aff_index == 3) {
        raise(SIGSEGV);
    }
}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
bool holytouch_after_effect(long owner, struct creature *vict, int level);
int apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
    int pos);

bool
affect_remove(struct creature *ch, struct affected_type *af)
{
    struct affected_type *temp;
    int type = -1;
    int level = 0;
    int duration = 0;
    long owner = 0;
    short is_instant = 0;

    if ((is_instant = af->is_instant)) {
        type = af->type;
        level = af->level;
        duration = af->duration;
    }

    owner = af->owner;

    if (!ch->affected) {
        errlog("!ch->affected in affect_remove()");
        return false;
    }
    if (af->type == SPELL_TAINT) {
        apply_soil_to_char(ch, GET_EQ(ch, WEAR_HEAD), SOIL_BLOOD, WEAR_HEAD);
        apply_soil_to_char(ch, GET_EQ(ch, WEAR_FACE), SOIL_BLOOD, WEAR_FACE);
        apply_soil_to_char(ch, GET_EQ(ch, WEAR_EYES), SOIL_BLOOD, WEAR_EYES);
    } else if (af->type == SPELL_QUAD_DAMAGE && ch->in_room &&
        !AFF_FLAGGED(ch, AFF_GLOWLIGHT) &&
        !AFF2_FLAGGED(ch, AFF2_FLUORESCENT) &&
        !AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION) &&
        !affected_by_spell(ch, SPELL_QUAD_DAMAGE))
        ch->in_room->light--;

    affect_modify(ch, af->location, af->modifier, af->bitvector, af->aff_index,
        false);
    REMOVE_FROM_LIST(af, ch->affected, next);
    free(af);
    affect_total(ch);

    if (is_instant && duration == 0 && ch->in_room) {
        switch (type) {
        case SKILL_HOLY_TOUCH:
            return holytouch_after_effect(owner, ch, level);
            break;
        }
    }
    return true;
}

/* Call affect_remove with every spell of spelltype "skill" */
bool
affect_from_char(struct creature *ch, int16_t type)
{
    struct affected_type *hjp = NULL, *next_hjp = NULL;
    bool found = false;

    for (hjp = ch->affected; hjp; hjp = next_hjp) {
        next_hjp = hjp->next;
        if (hjp->type == type) {
            affect_remove(ch, hjp);
            found = true;
        }
    }
    return found;
}

/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
struct affected_type *
affected_by_spell(struct creature *ch, int16_t type)
{
    struct affected_type *hjp = NULL;

    for (hjp = ch->affected; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return hjp;

    return NULL;
}

int
count_affect(struct creature *ch, int16_t type)
{
    struct affected_type *curr = NULL;
    int count = 0;
    for (curr = ch->affected; curr; curr = curr->next) {
        if (curr->type == type) {
            count++;
        }
    }
    return count;
}

//
// add a new affect to a character, joining it with an existing one if possible
//

void
affect_join(struct creature *ch, struct affected_type *af,
    bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
    struct affected_type *hjp;

    for (hjp = ch->affected; hjp; hjp = hjp->next) {

        if ((hjp->type == af->type) && (hjp->location == af->location) &&
            (hjp->aff_index == af->aff_index)) {
            if (add_dur)
                af->duration = MIN(666, af->duration + hjp->duration);
            if (avg_dur)
                af->duration /= 2;

            if (add_mod) {
                af->modifier =
                    MIN(MAX(af->modifier + hjp->modifier, -666), 666);
            }
            if (avg_mod)
                af->modifier /= 2;
            affect_remove(ch, hjp);
            affect_to_char(ch, af);
            return;
        }
    }

    //
    // if we get here, that means a pre-existing affect wasn't joined to,
    // so just add it fresh
    //

    affect_to_char(ch, af);

}

void
retire_trails(void)
{
    struct zone_data *zn = NULL;
    struct room_data *rm = NULL;
    struct room_trail_data *trail = NULL, *next_trail = NULL, *last_trail =
        NULL;
    time_t cur_time = time(NULL), del_time;

    del_time = cur_time - 360;  // 6 minute lifetime 1440; // 24 minutes lifetime

    for (zn = zone_table; zn; zn = zn->next) {
        for (rm = zn->world; rm; rm = rm->next) {
            for (trail = rm->trail, last_trail = rm->trail; trail;
                trail = next_trail) {
                next_trail = trail->next;

                if (!ZONE_FLAGGED(zn, ZONE_NOWEATHER) &&
                    zn->weather->sky >= SKY_RAINING) {
                    trail->track--;
                }
                if (trail->track <= 0 || trail->time <= del_time) {
                    // delete trail
                    if (trail->name) {
                        free(trail->name);
                        trail->name = NULL;
                    }
                    if (trail->aliases) {
                        free(trail->aliases);
                        trail->aliases = NULL;
                    }
                    if (trail == rm->trail) {
                        rm->trail = next_trail;
                        last_trail = rm->trail;
                    } else {
                        last_trail->next = next_trail;
                    }
                    free(trail);
                } else
                    last_trail = trail;
            }
        }
    }
}

void
update_trail(struct creature *ch, struct room_data *room, int dir, int mode)
{

    struct room_trail_data *trail, *next_trail;
    int max_trails;

    // Immortals never leave tracks
    if (IS_IMMORT(ch))
        return;

    for (trail = room->trail; trail; trail = trail->next) {
        if (IS_NPC(ch) && (trail->idnum == (int)-NPC_IDNUM(ch)))
            break;
        if (!IS_NPC(ch) && (trail->idnum == GET_IDNUM(ch)))
            break;
    }

    // Always create a new trail if the creature is entering the room
    // Create a new trail if they're leaving the room only if there isn't
    // a trail yet or we don't have a corresponding entrance
    if (mode == TRAIL_ENTER || !trail || trail->to_dir != -1) {
        CREATE(trail, struct room_trail_data, 1);
        trail->next = room->trail;
        room->trail = trail;

        trail->name = strdup(tmp_capitalize(GET_NAME(ch)));
        if (IS_NPC(ch)) {
            trail->idnum = -NPC_IDNUM(ch);
            trail->aliases = strdup(ch->player.name);
        } else {
            trail->idnum = GET_IDNUM(ch);
            trail->aliases = strdup(tmp_sprintf("%s .%s", ch->player.name,
                    ch->player.name));
        }
        trail->from_dir = -1;
        trail->to_dir = -1;
    }

    if (mode == TRAIL_ENTER && (dir >= 0 || trail->from_dir < 0)) {
        trail->from_dir = dir;
    } else if (dir >= 0 || trail->to_dir < 0)
        trail->to_dir = dir;

    if (GET_POSITION(ch) == POS_FLYING)
        trail->track = 0;
    else if (AFF_FLAGGED(ch, AFF_NOTRACK)
        || affected_by_spell(ch, SKILL_ELUSION))
        trail->track = 3;
    else
        trail->track = 10;

    if (!IS_UNDEAD(ch) && GET_HIT(ch) < (GET_MAX_HIT(ch) / 4)) {
        SET_BIT(trail->flags, TRAIL_FLAG_BLOOD_DROPS);
        trail->track = 10;
    }

    if (GET_EQ(ch, WEAR_FEET)) {
        if (OBJ_SOILED(GET_EQ(ch, WEAR_FEET), SOIL_BLOOD))
            SET_BIT(trail->flags, TRAIL_FLAG_BLOODPRINTS);
    } else {
        if (CHAR_SOILED(ch, WEAR_FEET, SOIL_BLOOD))
            SET_BIT(trail->flags, TRAIL_FLAG_BLOODPRINTS);
    }

    trail->time = time(NULL);

    // Remove trails over the maximum allowed for that sector type
    max_trails = 10;
    trail = room->trail;
    while (max_trails && trail) {
        trail = trail->next;
        max_trails--;
    }

    if (trail) {
        next_trail = trail->next;
        trail->next = NULL;
        trail = next_trail;
        while (trail) {
            next_trail = trail->next;
            if (trail->name)
                free(trail->name);
            if (trail->aliases)
                free(trail->aliases);
            free(trail);
            trail = next_trail;
        }
    }

}

/*
 * move a creature out of a room;
 *
 *
 * @param ch the struct creature to remove from the room
 * @param check_specials if true, special procedures will be
 * 		searched for and run.
 *
 * @return true on success, false if the struct creature may have died.
 */
bool
char_from_room(struct creature *ch, bool check_specials)
{

    if (ch == NULL || ch->in_room == NULL) {
        errlog("NULL or NOWHERE in handler.c, char_from_room");
        if (ch) {
            sprintf(buf, "Char is %s\r\n", GET_NAME(ch));
            if (ch->in_room != NULL)
                sprintf(buf, "Char is in_room %d\r\n", ch->in_room->number);
        }
        exit(1);
    }

    remove_all_combat(ch);

    if (GET_RACE(ch) == RACE_ELEMENTAL && IS_CLASS(ch, CLASS_FIRE))
        ch->in_room->light--;
    if (ch->equipment[WEAR_LIGHT] != NULL)
        if (IS_OBJ_TYPE(ch->equipment[WEAR_LIGHT], ITEM_LIGHT))
            if (GET_OBJ_VAL(ch->equipment[WEAR_LIGHT], 2))  /* Light is ON */
                ch->in_room->light--;
    if (AFF_FLAGGED(ch, AFF_GLOWLIGHT) || AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
        || AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION) ||
        affected_by_spell(ch, SPELL_QUAD_DAMAGE))
        ch->in_room->light--;

    if (!IS_NPC(ch))
        ch->in_room->zone->num_players--;

    affect_from_char(ch, SPELL_ENTANGLE);   // remove entanglement (summon etc)

    if (GET_OLC_SRCH(ch))
        GET_OLC_SRCH(ch) = NULL;

    // Some specials improperly deal with SPECIAL_LEAVE mode
    // by returning a true value.  This should take care of that.
    struct room_data *tmp_room = ch->in_room;
    long spec_rc = 0;
    if (check_specials) {
        spec_rc = special(ch, 0, 0, tmp_strdup(""), SPECIAL_LEAVE);
    }

    if (spec_rc != 0) {
        if (!g_list_find(tmp_room->people, ch)) {
            if (spec_rc == 1) {
                slog("Creature died leaving search room(0x%lx)[%d]",
                    (long)tmp_room, tmp_room->number);
            } else {
                slog("Creature died leaving spec(0x%lx) room(0x%lx)[%d]", spec_rc, (long)tmp_room, tmp_room->number);
            }
            return false;
        }
    }

    tmp_room->people = g_list_remove(tmp_room->people, ch);
    ch->in_room = NULL;
    return true;
}

/*
 * place a character in a room
 *
 * @param ch the struct creature to move to the room
 * @param room the room to move the struct creature into
 * @param check_specials if true, special procedures will be
 * 		searched for and run.
 *
 * @return true on success, false if the struct creature may have died.
 */
bool
char_to_room(struct creature * ch, struct room_data * room,
    bool check_specials)
{
    struct affected_type *aff = NULL, *next_aff = NULL;

    if (ch->in_room) {
        errlog("creature already in a room in char_to_room()!");
        raise(SIGSEGV);
        return false;
    }

    if (!ch || room == NULL) {
        errlog("Illegal value(s) passed to char_to_room");
        raise(SIGSEGV);
        return false;
    }

    room->people = g_list_prepend(room->people, ch);
    ch->in_room = room;

    if (GET_RACE(ch) == RACE_ELEMENTAL && IS_CLASS(ch, CLASS_FIRE))
        room->light++;

    if (GET_EQ(ch, WEAR_LIGHT))
        if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2)) /* Light ON */
                room->light++;
    if (AFF_FLAGGED(ch, AFF_GLOWLIGHT) || AFF2_FLAGGED(ch, AFF2_FLUORESCENT)
        || AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION) ||
        affected_by_spell(ch, SPELL_QUAD_DAMAGE))
        room->light++;

    if (!IS_NPC(ch)) {
        room->zone->num_players++;
        room->zone->idle_time = 0;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_NULL_MAGIC) &&
        !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        if (ch->affected) {
            send_to_char(ch,
                "You are dazed by a blinding flash inside your brain!\r\n"
                "You feel different...\r\n");
            act("Light flashes from behind $n's eyes.", false, ch, NULL, NULL,
                TO_ROOM);
            for (aff = ch->affected; aff; aff = next_aff) {
                next_aff = aff->next;
                if (SPELL_IS_MAGIC(aff->type) || SPELL_IS_DIVINE(aff->type))
                    affect_remove(ch, aff);
            }

            WAIT_STATE(ch, PULSE_VIOLENCE);
        }
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) &&
        !AFF2_FLAGGED(ch, AFF2_ABLAZE) && !CHAR_WITHSTANDS_FIRE(ch)) {
        act("$n suddenly bursts into flames!", false, ch, NULL, NULL, TO_ROOM);
        act("You suddenly burst into flames!", false, ch, NULL, NULL, TO_CHAR);
        ignite_creature(ch, NULL);
    }

    struct room_affect_data *raff;
    struct creature *raff_owner;

    raff = room_affected_by(ch->in_room, SONG_RHYTHM_OF_ALARM);
    if (raff && GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_NPC(ch)) {
        raff_owner = get_char_in_world_by_idnum(raff->owner);

        if (raff_owner &&
            raff_owner->in_room != ch->in_room &&
            (GET_LEVEL(ch) + number(1, 70)) <
            skill_bonus(raff_owner, SONG_RHYTHM_OF_ALARM)) {

            raff->duration--;
            send_to_char(raff_owner, "%s has just entered %s.\r\n",
                GET_NAME(ch), ch->in_room->name);
        }
    }

    long spec_rc = 0;
    if (check_specials)
        spec_rc = special(ch, 0, 0, tmp_strdup(""), SPECIAL_ENTER);

    if (spec_rc != 0) {
        if (!g_list_find(room->people, ch)) {
            if (spec_rc == 1) {
                slog("Creature died entering search room (0x%lx)[%d]",
                    (long)room, room->number);
            } else {
                slog("Creature died entering spec(0x%lx) room(0x%lx)[%d]", spec_rc, (long)room, room->number);
            }

            return false;
        }
    }
    return true;
}

static struct obj_data *
insert_obj_into_contents(struct obj_data *contents, struct obj_data *object)
{
    // Find the first "identical" object in the carry list, and put the new
    // object before that one if one exists.  Otherwise, push onto the
    // head of the list.
    if (!contents || same_obj(contents, object)) {
        object->next_content = contents;
        return object;
    }

    for (struct obj_data *obj = contents;obj->next_content;obj = obj->next_content) {
        if (same_obj(obj->next_content, object)) {
            object->next_content = obj->next_content;
            obj->next_content = object;
            return contents;
        }
    }

    object->next_content = contents;
    return object;
}

static struct obj_data *
append_obj_to_contents(struct obj_data *contents, struct obj_data *object)
{
    object->next_content = NULL;

    if (!contents) {
        return object;
    }

    struct obj_data *obj = contents;

    while (obj->next_content)
        obj = obj->next_content;

    obj->next_content = object;

    return contents;
}

typedef struct obj_data *(*insert_func_t)(struct obj_data *, struct obj_data *);

/* give an object to a char   */
static void
general_obj_to_char(struct obj_data *object,
                    struct creature *ch,
                    insert_func_t insert_func)
{
    struct zone_data *zn = NULL;

    if (!object || !ch) {
        errlog("NULL obj or char passed to obj_to_char");
        return;
    }

    ch->carrying = insert_func(ch->carrying, object);
    object->carried_by = ch;
    object->in_room = NULL;

    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    if (IS_OBJ_TYPE(object, ITEM_KEY) && !GET_OBJ_VAL(object, 1) &&
        !IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && !GET_OBJ_TIMER(object)) {

        if ((zn = real_zone(zone_number(GET_OBJ_VNUM(object)))))
            GET_OBJ_TIMER(object) = MAX(2, zn->lifespan / 2);
        else
            GET_OBJ_TIMER(object) = 15;
    }
    /* set flag for crash-save system */
    if (!IS_NPC(ch))
        SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
}

void
obj_to_char(struct obj_data *object, struct creature *ch)
{
    general_obj_to_char(object, ch, insert_obj_into_contents);
}

void
unsorted_obj_to_char(struct obj_data *object, struct creature *ch)
{
    general_obj_to_char(object, ch, append_obj_to_contents);
}

/* take an object from a char */
void
obj_from_char(struct obj_data *object)
{
    struct obj_data *temp;

    if (object == NULL) {
        errlog("NULL object passed to obj_from_char");
        return;
    }

    if (!object->carried_by) {
        errlog("object->carried_by == NULL in obj_from_char");
        raise(SIGSEGV);
        return;
    }
#ifdef TRACK_OBJS
    object->obj_flags.tracker.lost_time = time(0);
    sprintf(buf, "carried by %s", GET_NAME(object->carried_by));
    strncpy(object->obj_flags.tracker.string, buf, TRACKER_STR_LEN - 1);
#endif

    REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

    /* set flag for crash-save system */
    if (!IS_NPC(object->carried_by))
        SET_BIT(PLR_FLAGS(object->carried_by), PLR_CRASH);

    IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(object->carried_by)--;
    if (object->aux_obj) {
        if (IS_OBJ_TYPE(object, ITEM_SCUBA_MASK) ||
            IS_OBJ_TYPE(object, ITEM_SCUBA_TANK)) {
            object->aux_obj->aux_obj = NULL;
            object->aux_obj = NULL;
        }
    }

    object->carried_by = NULL;
    object->next_content = NULL;
}

/* Return the effect of a piece of armor in position eq_pos */
int
apply_ac(struct creature *ch, int eq_pos)
{
    int factor;

    if (!GET_EQ(ch, eq_pos)) {
        errlog("!GET_EQ(ch, eq_pos) in apply_ac");
        return 0;
    }

    if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
        return 0;

    switch (eq_pos) {

    case WEAR_BODY:
        factor = 3;
        break;                  /* 30% */
    case WEAR_HEAD:
    case WEAR_LEGS:
        factor = 2;
        break;                  /* 20% */
    case WEAR_EAR_L:
    case WEAR_EAR_R:
        factor = 1;
        break;                  /* 2% */
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
        factor = 1;
        break;                  /* 2% */
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
        factor = 1;
        break;                  /* 2% */
    default:
        factor = 1;
        break;                  /* all others 10% */
    }

    return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));

}

int
weapon_prof(struct creature *ch, struct obj_data *obj)
{

    int skill = 0;

    if (!obj || !IS_OBJ_TYPE(obj, ITEM_WEAPON))
        return 0;

    if (GET_OBJ_VAL(obj, 3) >= 0 &&
        GET_OBJ_VAL(obj, 3) < TOP_ATTACKTYPE - TYPE_HIT)
        skill = weapon_proficiencies[GET_OBJ_VAL(obj, 3)];
    else
        return 0;

    if (skill)
        return (CHECK_SKILL(ch, skill));
    else
        return 0;
}

/* equip_char returns true if victim is killed by equipment :> */
int
equip_char(struct creature *ch, struct obj_data *obj, int pos, int mode)
{
    int invalid_char_class(struct creature *ch, struct obj_data *obj);

    if (pos < 0 || pos >= NUM_WEARS) {
        errlog("Illegal pos %d in equip_char.", pos);
        obj_to_room(obj, zone_table->world);
        return 0;
    }

    if (obj->carried_by) {
        errlog("EQUIP: Obj is carried_by when equip.");
        obj_to_room(obj, zone_table->world);
        return 0;
    }
    if (obj->in_room != NULL) {
        errlog("EQUIP: Obj is in_room when equip.");
        return 0;
    }

    switch (mode) {
    case EQUIP_WORN:
        if (GET_EQ(ch, pos)) {
            errlog("%s is already equipped: %s", GET_NAME(ch), obj->name);
            obj_to_room(obj, zone_table->world);
            return 0;
        }
        GET_EQ(ch, pos) = obj;
        if (IS_OBJ_TYPE(obj, ITEM_ARMOR))
            GET_AC(ch) -= apply_ac(ch, pos);

        IS_WEARING_W(ch) += GET_OBJ_WEIGHT(obj);

        if (ch->in_room != NULL) {
            if (pos == WEAR_LIGHT && IS_OBJ_TYPE(obj, ITEM_LIGHT))
                if (GET_OBJ_VAL(obj, 2))    /* if light is ON */
                    ch->in_room->light++;
        }
        break;
    case EQUIP_IMPLANT:
        if (GET_IMPLANT(ch, pos)) {
            errlog("%s is already implanted: %s", GET_NAME(ch), obj->name);
            obj_to_room(obj, zone_table->world);
            return 0;
        }
        GET_IMPLANT(ch, pos) = obj;
        if (IS_OBJ_TYPE(obj, ITEM_ARMOR))
            GET_AC(ch) -= GET_OBJ_VAL(obj, 0);

        GET_WEIGHT(ch) += (int)GET_OBJ_WEIGHT(obj);
        break;
    case EQUIP_TATTOO:
        if (GET_TATTOO(ch, pos)) {
            errlog("%s is already tattooed: %s", GET_NAME(ch), obj->name);
            obj_to_room(obj, zone_table->world);
            return 0;
        }
        GET_TATTOO(ch, pos) = obj;
        break;
    }

    obj->worn_by = ch;
    obj->worn_on = pos;

    apply_object_affects(ch, obj, true);
    affect_total(ch);

    return 0;
}

struct obj_data *
unequip_char(struct creature *ch, int pos, int mode)
{
    struct obj_data *obj = raw_unequip_char(ch, pos, mode);

    if (mode == EQUIP_WORN) {
        if (pos == WEAR_WAIST && GET_EQ(ch, WEAR_BELT))
            obj_to_char(unequip_char(ch, WEAR_BELT, EQUIP_WORN), ch);
        if (pos == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2)) {
            equip_char(ch, unequip_char(ch, WEAR_WIELD_2, EQUIP_WORN),
                WEAR_WIELD, EQUIP_WORN);
        }
    }

    return obj;
}

struct obj_data *
raw_unequip_char(struct creature *ch, int pos, int mode)
{
    struct obj_data *obj = NULL;
    int invalid_char_class(struct creature *ch, struct obj_data *obj);

    if (pos < 0 || pos >= NUM_WEARS) {
        errlog("Illegal pos in unequip_char.");
        return NULL;
    }

    switch (mode) {
    case EQUIP_WORN:
        if (!GET_EQ(ch, pos)) {
            errlog("eq pointer NULL at pos %d in unequip_char.", pos);
            return NULL;
        }
        obj = GET_EQ(ch, pos);

        if (IS_OBJ_TYPE(obj, ITEM_ARMOR))
            GET_AC(ch) += apply_ac(ch, pos);

        IS_WEARING_W(ch) -= GET_OBJ_WEIGHT(obj);

        if (ch->in_room != NULL) {
            if (pos == WEAR_LIGHT && IS_OBJ_TYPE(obj, ITEM_LIGHT))
                if (GET_OBJ_VAL(obj, 2))    /* if light is ON */
                    ch->in_room->light--;
        }

        break;
    case EQUIP_IMPLANT:
        if (!GET_IMPLANT(ch, pos)) {
            errlog("implant pointer NULL at pos %d in unequip_char.", pos);
            return NULL;
        }
        obj = GET_IMPLANT(ch, pos);
        if (IS_OBJ_TYPE(obj, ITEM_ARMOR))
            GET_AC(ch) += GET_OBJ_VAL(obj, 0);

        GET_WEIGHT(ch) -= (int)GET_OBJ_WEIGHT(obj);
        break;
    case EQUIP_TATTOO:
        if (!GET_TATTOO(ch, pos)) {
            errlog("tattoo pointer NULL at pos %d in unequip_char.", pos);
            return NULL;
        }
        obj = GET_TATTOO(ch, pos);
        break;
    default:
        errlog("invalid mode given to raw_unequip_char()");
        return NULL;
    }

#ifdef TRACK_OBJS
    obj->obj_flags.tracker.lost_time = time(0);
    sprintf(buf, "%s %s @ %d", internal ? "implanted" : "worn",
        GET_NAME(obj->worn_by), pos);
    strncpy(obj->obj_flags.tracker.string, buf, TRACKER_STR_LEN - 1);
#endif

    apply_object_affects(ch, obj, false);

    switch (mode) {
    case EQUIP_WORN:
        GET_EQ(ch, pos) = NULL;
        break;
    case EQUIP_IMPLANT:
        GET_IMPLANT(ch, pos) = NULL;
        break;
    case EQUIP_TATTOO:
        GET_TATTOO(ch, pos) = NULL;
        break;
    }

    obj->worn_by = NULL;
    obj->worn_on = -1;

    affect_total(ch);

    return (obj);
}

int
check_eq_align(struct creature *ch)
{
    struct obj_data *obj, *implant;
    int pos;

    if (!ch->in_room || GET_LEVEL(ch) >= LVL_GOD)
        return 0;

    for (pos = 0; pos < NUM_WEARS; pos++) {
        if ((implant = ch->implants[pos])) {

            if ((IS_GOOD(ch) && IS_OBJ_STAT(ch->implants[pos], ITEM_DAMNED)) ||
                (IS_EVIL(ch) && IS_OBJ_STAT(ch->implants[pos], ITEM_BLESS))) {

                obj_to_char(unequip_char(ch, pos, EQUIP_IMPLANT), ch);

                act("$p burns its way out through your flesh!", false, ch,
                    implant, NULL, TO_CHAR);
                act("$n screams in horror as $p burns its way out through $s flesh!", false, ch, implant, NULL, TO_ROOM);

                damage_eq(NULL, implant, (GET_OBJ_DAM(implant) / 2), -1);

                int extraction_damage =
                    MAX(GET_ALIGNMENT(ch), -GET_ALIGNMENT(ch));
                if (pos == WEAR_BODY)
                    extraction_damage *= 3;
                else if (pos == WEAR_HEAD || pos == WEAR_LEGS)
                    extraction_damage *= 2;
                extraction_damage /= 8;
                return damage(ch, ch, NULL, dice(extraction_damage, 3),
                    TOP_SPELL_DEFINE, pos);
            }
        }

        obj = GET_EQ(ch, pos);
        if (!obj)
            continue;

        if ((IS_OBJ_STAT(obj, ITEM_BLESS) && IS_EVIL(ch)) ||
            (IS_OBJ_STAT(obj, ITEM_DAMNED) && IS_GOOD(ch))) {
            int skill;

            act("You are burned by $p and frantically take it off!", false, ch,
                obj, NULL, TO_CHAR);
            act("$n frantically takes off $p as $e screams in agony!", false,
                ch, obj, NULL, TO_ROOM);
            skill = MAX(GET_ALIGNMENT(ch), -GET_ALIGNMENT(ch));
            skill /= 32;
            skill = MAX(1, skill);
            obj_to_char(unequip_char(ch, pos, false), ch);

            return damage(ch, ch, NULL, dice(skill, 2), TOP_SPELL_DEFINE, pos);
        }

        if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
            (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
            (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
            act("You are zapped by $p and instantly let go of it.",
                false, ch, obj, NULL, TO_CHAR);
            act("$n is zapped by $p and instantly lets go of it.",
                false, ch, obj, NULL, TO_ROOM);
            obj_to_char(unequip_char(ch, pos, false), ch);
            if (IS_NPC(ch)) {
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
            }
        }
    }
    return 0;
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *
get_obj_in_list_num(int num, struct obj_data *list)
{
    struct obj_data *i;

    for (i = list; i; i = i->next_content)
        if (GET_OBJ_VNUM(i) == num)
            return i;

    return NULL;
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *
get_obj_num(int nr)
{
    struct obj_data *i;

    for (i = object_list; i; i = i->next)
        if (GET_OBJ_VNUM(i) == nr)
            return i;

    return NULL;
}

/* search a room for a char, and return a pointer if found..  */
struct creature *
get_char_room(char *name, struct room_data *room)
{
    int j = 0, number;
    char *tmp = tmp_strdup(name);

    if (!(number = get_number(&tmp)))
        return NULL;

    for (GList *it = first_living(room->people); it && (j <= number);
         it = next_living(it)) {
        struct creature *tch = it->data;
        if (isname(tmp, tch->player.name))
            if (++j == number)
                return tch;
    }
    return NULL;
}

struct creature *
get_char_in_world_by_idnum(long nr)
{
    struct creature *result = g_hash_table_lookup(creature_map, GINT_TO_POINTER(nr));
    if (!result || is_dead(result))
        return NULL;
    return result;
}

bool
same_obj(struct obj_data * obj1, struct obj_data * obj2)
{
    int index;

    if (!obj1 || !obj2)
        return (obj1 == obj2);

    if (GET_OBJ_VNUM(obj1) != GET_OBJ_VNUM(obj2))
        return (false);

    if (obj1->consignor != obj2->consignor)
        return false;

    if (GET_OBJ_SIGIL_IDNUM(obj1) != GET_OBJ_SIGIL_IDNUM(obj2) ||
        GET_OBJ_SIGIL_LEVEL(obj1) != GET_OBJ_SIGIL_LEVEL(obj2))
        return false;

    if (obj1->shared->proto
        && (obj1->name != obj1->shared->proto->name
            || obj1->line_desc != obj1->shared->proto->line_desc))
        return false;

            
    if (obj2->shared->proto
        && (obj2->name != obj2->shared->proto->name
            || obj2->line_desc != obj2->shared->proto->line_desc))
        return false;

    if (obj1->name != obj2->name && strcmp(obj1->name, obj2->name))
        return false;

    if (obj1->line_desc != obj2->line_desc
        && (!obj1->line_desc || !obj2->line_desc
            || strcmp(obj1->line_desc, obj2->line_desc)))
        return false;

    if (obj1->engraving != obj2->engraving
        && (!obj1->engraving || !obj2->engraving
            || strcmp(obj1->engraving, obj2->engraving)))
        return false;

    if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2) ||
        GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2) ||
        GET_OBJ_EXTRA2(obj1) != GET_OBJ_EXTRA2(obj2))
        return (false);

    for (index = 0; index < MAX_OBJ_AFFECT; index++)
        if ((obj1->affected[index].location != obj2->affected[index].location)
            || (obj1->affected[index].modifier !=
                obj2->affected[index].modifier))
            return (false);

    if (GET_OBJ_WEIGHT(obj1) != GET_OBJ_WEIGHT(obj2))
        return false;

    return (true);
}

/* put an object in a room */
static void
general_obj_to_room(struct obj_data *object,
                    struct room_data *room,
                    insert_func_t insert_func)
{
    if (!object || !room) {
        errlog("Illegal %s | %s passed to obj_to_room",
            object ? "" : "OBJ", room ? "" : "ROOM");
        raise(SIGSEGV);
        return;
    }

    if (object->carried_by) {
        errlog("Object already carried in obj_to_room()!");
        raise(SIGSEGV);
        return;
    }

    room->contents = insert_func(room->contents, object);
    object->in_room = room;

    if (ROOM_FLAGGED(room, ROOM_HOUSE))
        SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

    if (IS_VEHICLE(object) && object->contains &&
        IS_ENGINE(object->contains) && HEADLIGHTS_ON(object->contains))
        object->in_room->light++;
    REMOVE_BIT(GET_OBJ_EXTRA2(object), ITEM2_HIDDEN);

    if (IS_CIGARETTE(object)
        && SMOKE_LIT(object)
        && (room_is_watery(room) || !room_has_air(room)))
        SMOKE_LIT(object) = 0;
}

void
obj_to_room(struct obj_data *object, struct room_data *room)
{
    general_obj_to_room(object, room, insert_obj_into_contents);
}

void
unsorted_obj_to_room(struct obj_data *object, struct room_data *room)
{
    general_obj_to_room(object, room, append_obj_to_contents);
}

/* Take an object from a room */
void
obj_from_room(struct obj_data *object)
{
    struct obj_data *temp;

    if (!object) {
        errlog("NULL object passed to obj_from_room");
        raise(SIGSEGV);
        return;
    }
    if (!object->in_room) {
        errlog("NULL object->in_room in obj_from_room");
        raise(SIGSEGV);
        return;
    }
#ifdef TRACK_OBJS
    object->obj_flags.tracker.lost_time = time(0);
    sprintf(object->obj_flags.tracker.string, "inroom %d",
        object->in_room->number);
#endif

    if (IS_VEHICLE(object) && object->contains && IS_ENGINE(object->contains)
        && HEADLIGHTS_ON(object->contains))
        object->in_room->light--;

    REMOVE_FROM_LIST(object, object->in_room->contents, next_content);

    if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
        SET_BIT(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);
    object->in_room = NULL;
    object->next_content = NULL;
    REMOVE_BIT(GET_OBJ_EXTRA2(object), ITEM2_HIDDEN);
}

/* put an object in an object (quaint)  */
static void
general_obj_to_obj(struct obj_data *obj, struct obj_data *obj_to, insert_func_t insert_func)
{
    struct creature *vict = NULL;

    if (!obj || !obj_to || obj == obj_to) {
        errlog("NULL object or same src and targ obj passed to obj_to_obj");
        return;
    }

    obj_to->contains = insert_func(obj_to->contains, obj);
    obj->in_obj = obj_to;

    /* top level object.  Subtract weight from inventory if necessary. */
    modify_object_weight(obj_to, GET_OBJ_WEIGHT(obj));

    if (obj_to->in_room && ROOM_FLAGGED(obj_to->in_room, ROOM_HOUSE))
        SET_BIT(ROOM_FLAGS(obj_to->in_room), ROOM_HOUSE_CRASH);

    if (IS_INTERFACE(obj_to)
        && (vict = obj_to->worn_by)
        && obj_to == GET_IMPLANT(vict, obj_to->worn_on)) {
        apply_object_affects(vict, obj, true);
        affect_total(vict);
    }
}

void
obj_to_obj(struct obj_data *object, struct obj_data *obj_to)
{
    general_obj_to_obj(object, obj_to, insert_obj_into_contents);
}

void
unsorted_obj_to_obj(struct obj_data *object, struct obj_data *obj_to)
{
    general_obj_to_obj(object, obj_to, append_obj_to_contents);
}

/* remove an object from an object */
void
obj_from_obj(struct obj_data *obj)
{
    struct obj_data *obj_from = NULL, *temp = NULL;
    struct creature *vict = NULL;

    if (obj->in_obj == NULL) {
        errlog("(handler.c): trying to illegally extract obj from obj");
        raise(SIGSEGV);
        return;
    }
#ifdef TRACK_OBJS
    obj->obj_flags.tracker.lost_time = time(0);
    sprintf(obj->obj_flags.tracker.string, "in obj %s", obj->in_obj->name);
#endif

    obj_from = obj->in_obj;

    modify_object_weight(obj_from, -GET_OBJ_WEIGHT(obj));

    REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

    if (IS_INTERFACE(obj_from)
        && (vict = obj_from->worn_by)
        && obj_from == GET_IMPLANT(vict, obj_from->worn_on)) {
        apply_object_affects(vict, obj, false);
        affect_total(vict);
    }

    if (obj_from->in_room && ROOM_FLAGGED(obj_from->in_room, ROOM_HOUSE))
        SET_BIT(ROOM_FLAGS(obj_from->in_room), ROOM_HOUSE_CRASH);

    obj->in_obj = NULL;
    obj->next_content = NULL;

    REMOVE_BIT(GET_OBJ_EXTRA2(obj), ITEM2_HIDDEN);
}

/* Extract an object from the world */
void
extract_obj(struct obj_data *obj)
{

    struct obj_data *temp = NULL;

    if (obj->worn_by != NULL)
        if (unequip_char(obj->worn_by, obj->worn_on,
                (obj == GET_EQ(obj->worn_by, obj->worn_on) ?
                    EQUIP_WORN : EQUIP_IMPLANT)) != obj)
            errlog("Inconsistent worn_by and worn_on pointers!!");
    if (obj->in_room != NULL)
        obj_from_room(obj);
    else if (obj->carried_by)
        obj_from_char(obj);
    else if (obj->in_obj)
        obj_from_obj(obj);

    if (obj->aux_obj) {
        if (obj->aux_obj->aux_obj && obj->aux_obj->aux_obj == obj)
            obj->aux_obj->aux_obj = NULL;
        obj->aux_obj = NULL;
    }
    // Make sure no progs are referring to the object
    prog_unreference_object(obj);

    /* Get rid of the contents of the object, as well. */
    while (obj->contains)
        extract_obj(obj->contains);

    if (obj->func_data)
        free(obj->func_data);

    if (obj->shared && obj->shared->vnum >= 0)
        obj->shared->number--;

    REMOVE_FROM_LIST(obj, object_list, next);

    /* remove obj from any paths */
    if (IS_VEHICLE(obj))
        path_remove_object(obj);

    if (IS_CORPSE(obj)) {
        char *fname;

        if (CORPSE_IDNUM(obj)) {
            fname = get_corpse_file_path(CORPSE_IDNUM(obj));
            remove(fname);
        }
    }
    free_object(obj);
}

void
update_object(struct obj_data *obj, int use)
{
    while (obj) {
        if (GET_OBJ_TIMER(obj) > 0)
            GET_OBJ_TIMER(obj) -= use;
        if (obj->contains)
            update_object(obj->contains, use);
        obj = obj->next_content;
    }
}

void
update_char_objects(struct creature *ch)
{
    int i;

    if (!ch->in_room)
        return;

    if (GET_EQ(ch, WEAR_LIGHT) != NULL)
        if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0) {
                i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
                if (i == 1) {
                    act("Your light begins to flicker and fade.", false, ch, NULL,
                        NULL, TO_CHAR);
                    act("$n's light begins to flicker and fade.", false, ch, NULL,
                        NULL, TO_ROOM);
                } else if (i == 0) {
                    act("Your light sputters out and dies.", false, ch, NULL, NULL,
                        TO_CHAR);
                    act("$n's light sputters out and dies.", false, ch, NULL, NULL,
                        TO_ROOM);
                    ch->in_room->light--;
                }
            }

    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
            update_object(GET_EQ(ch, i), 2);

    if (ch->carrying)
        update_object(ch->carrying, 1);
}

/* ***********************************************************************
   Here follows high-level versions of some earlier routines, ie functions
   which incorporate the actual player-data.
   *********************************************************************** */

struct creature *
get_player_vis(struct creature *ch, const char *name, int inroom)
{
    struct creature *i, *match;
    char *tmpname, *write_pt;

    // remove leading spaces
    while (*name && (isspace(*name) || '.' == *name))
        name++;

    write_pt = tmpname = tmp_strdup(name);
    while (*name && !isspace(*name))
        *write_pt++ = *name++;

    *write_pt = '\0';

    match = NULL;
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        i = cit->data;
        if ((!IS_NPC(i) || i->desc) &&
            (!inroom || i->in_room == ch->in_room) &&
            can_see_creature(ch, i)) {
            switch (is_abbrev(tmpname, i->player.name)) {
            case 1:            // abbreviated match
                if (!match)
                    match = i;
                break;
            case 2:            // exact match
                return i;
            default:
                break;
            }
        }
    }

    return match;
}

struct creature *
get_mobile_vis(struct creature *ch, const char *name, int inroom)
{
    struct creature *i, *match;
    char *tmpname, *write_pt;

    // remove leading spaces
    while (*name && (isspace(*name) || '.' == *name))
        name++;

    write_pt = tmpname = tmp_strdup(name);
    while (*name && !isspace(*name))
        *write_pt++ = *name++;

    *write_pt = '\0';

    match = NULL;
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        i = cit->data;
        if (IS_NPC(i)
            && (!inroom || i->in_room == ch->in_room)
            && can_see_creature(ch, i)) {
            switch (is_abbrev(tmpname, i->player.name)) {
            case 1:            // abbreviated match
                if (!match)
                    match = i;
                break;
            case 2:            // exact match
                return i;
            default:
                break;
            }
        }
    }

    return match;
}

struct creature *
get_char_room_vis(struct creature *ch, const char *name)
{
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;
    struct affected_type *af = NULL;

    /* 0.<name> means PC with name */
    strcpy(tmp, name);
    number = get_number(&tmp);

    if (number == 0)
        return get_player_vis(ch, tmp, 1);

    if (strcasecmp(name, "self") == 0)
        return ch;

    for (GList * cit = first_living(ch->in_room->people); cit && j <= number;
        cit = next_living(cit)) {
        struct creature *tch = cit->data;
        struct creature *mob = NULL;
        af = affected_by_spell(tch, SKILL_DISGUISE);
        if (af != NULL) {
            mob = real_mobile_proto(af->modifier);
        }

        if ((mob != NULL && isname(tmp, mob->player.name)) ||
            ((af == NULL || CAN_DETECT_DISGUISE(ch, tch, af->duration))
                && isname(tmp, tch->player.name))) {
            if (can_see_creature(ch, tch)) {
                if (++j == number) {
                    return tch;
                }
            }
        }
    }

    return NULL;
}

struct creature *
get_char_random(struct room_data *room)
{
    struct creature *result = NULL;
    int total = 0;

    for (GList * cit = first_living(room->people); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (!number(0, total))
            result = tch;
        total++;
    }

    return result;
}

struct creature *
get_char_random_vis(struct creature *ch, struct room_data *room)
{
    struct creature *result = NULL;
    int total = 0;

    if (!room->people)
        return NULL;

    for (GList * cit = first_living(room->people); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (tch != ch && can_see_creature(ch, tch)) {
            if (!number(0, total))
                result = tch;
            total++;
        }
    }

    return result;
}

struct creature *
get_player_random(struct room_data *room)
{
    struct creature *result = NULL;
    int total = 0;

    if (!room->people)
        return NULL;

    for (GList * cit = first_living(room->people); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (IS_PC(tch)) {
            if (!number(0, total))
                result = tch;
            total++;
        }
    }

    return result;
}

struct creature *
get_player_random_vis(struct creature *ch, struct room_data *room)
{
    struct creature *result = NULL;
    int total = 0;

    if (!room->people)
        return NULL;

    for (GList * cit = first_living(room->people); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (tch != ch && IS_PC(tch) && can_see_creature(ch, tch)) {
            if (!number(0, total))
                result = tch;
            total++;
        }
    }

    return result;
}

struct creature *
get_char_in_remote_room_vis(struct creature *ch, const char *name,
    struct room_data *inroom)
{
    struct room_data *was_in = ch->in_room;
    struct creature *i = NULL;

    ch->in_room = inroom;
    i = get_char_room_vis(ch, name);
    ch->in_room = was_in;
    return (i);
}

struct creature *
get_char_vis(struct creature *ch, const char *name)
{
    struct creature *i;
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    /* check the room first */
    if ((i = get_char_room_vis(ch, name)) != NULL)
        return i;

    // 0.name means player only
    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
        return get_player_vis(ch, tmp, 0);

    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        i = cit->data;
        if (isname(tmp, i->player.name) && can_see_creature(ch, i))
            if (++j == number)
                return i;
    }
    return NULL;
}

struct obj_data *
get_obj_in_list_vis(struct creature *ch, const char *name,
    struct obj_data *list)
{
    struct obj_data *i;
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
        return NULL;

    for (i = list; i && (j <= number); i = i->next_content)
        if (isname(tmp, i->aliases))
            if (can_see_object(ch, i))
                if (++j == number)
                    return i;

    return NULL;
}

struct obj_data *
get_obj_in_list_all(struct creature *ch, const char *name,
    struct obj_data *list)
{
    struct obj_data *i;
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
        return NULL;

    for (i = list; i && (j <= number); i = i->next_content)
        if (isname(tmp, i->aliases))
            if (can_see_object(ch, i))
                if (++j == number)
                    return i;

    return NULL;
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *
get_obj_vis(struct creature *ch, const char *name)
{
    struct obj_data *i;
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    if (is_number(name) && is_authorized(ch, DEBUGGING, NULL)) {
        // Scan the object list for the unique ID given by the number
        number = atoi(name);
        for (i = object_list; i; i = i->next)
            if (i->unique_id == number)
                return i;
    } else {
        /* scan items carried */
        if ((i = get_obj_in_list_vis(ch, name, ch->carrying)))
            return i;

        /* scan room */
        if ((i = get_obj_in_list_vis(ch, name, ch->in_room->contents)))
            return i;

        strcpy(tmp, name);
        if (!(number = get_number(&tmp)))
            return NULL;

        /* ok.. no luck yet. scan the entire obj list   */
        for (i = object_list; i && (j <= number); i = i->next)
            if (isname(tmp, i->aliases) && can_see_object(ch, i)) {
                j++;
                if (j == number)
                    return i;
            }
    }

    return NULL;
}

struct obj_data *
get_object_in_equip_pos(struct creature *ch, const char *arg, int pos)
{
    if (GET_EQ(ch, pos) && isname(arg, GET_EQ(ch, pos)->aliases) &&
        can_see_object(ch, GET_EQ(ch, pos)))
        return (GET_EQ(ch, pos));
    else
        return (NULL);
}

struct obj_data *
get_object_in_equip_vis(struct creature *ch,
    const char *arg, struct obj_data *equipment[], int *j)
{
    int x = 0;
    int number = 0;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    strcpy(tmp, arg);
    if (!(number = get_number(&tmp)))
        return NULL;

    for ((*j) = 0; (*j) < NUM_WEARS && x <= number; (*j)++)
        if (equipment[(*j)])
            if (can_see_object(ch, equipment[(*j)]))
                if (isname(tmp, equipment[(*j)]->aliases))
                    if (++x == number)
                        return (equipment[(*j)]);

    return NULL;
}

struct obj_data *
get_object_in_equip_all(struct creature *ch,
    const char *arg, struct obj_data *equipment[], int *j)
{
    int x = 0;
    int number = 0;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    strcpy(tmp, arg);
    if (!(number = get_number(&tmp)))
        return NULL;

    for ((*j) = 0; (*j) < NUM_WEARS && x <= number; (*j)++)
        if (equipment[(*j)])
            if (can_see_object(ch, equipment[(*j)]))
                if (isname(tmp, equipment[(*j)]->aliases))
                    if (++x == number)
                        return (equipment[(*j)]);
    return NULL;
}

char *
money_desc(int amount, int mode)
{
    char *result;

    if (amount <= 0) {
        errlog("Try to create negative or 0 money.");
        return NULL;
    }
    if (mode == 0) {
        if (amount == 1)
            result = tmp_strdup("a gold coin");
        else if (amount <= 10)
            result = tmp_strdup("a tiny pile of gold coins");
        else if (amount <= 20)
            result = tmp_strdup("a handful of gold coins");
        else if (amount <= 75)
            result = tmp_strdup("a little pile of gold coins");
        else if (amount <= 200)
            result = tmp_strdup("a small pile of gold coins");
        else if (amount <= 1000)
            result = tmp_strdup("a pile of gold coins");
        else if (amount <= 5000)
            result = tmp_strdup("a big pile of gold coins");
        else if (amount <= 10000)
            result = tmp_strdup("a large heap of gold coins");
        else if (amount <= 20000)
            result = tmp_strdup("a huge mound of gold coins");
        else if (amount <= 75000)
            result = tmp_strdup("an enormous mound of gold coins");
        else if (amount <= 150000)
            result = tmp_strdup("a small mountain of gold coins");
        else if (amount <= 250000)
            result = tmp_strdup("a mountain of gold coins");
        else if (amount <= 500000)
            result = tmp_strdup("a huge mountain of gold coins");
        else if (amount <= 1000000)
            result = tmp_strdup("an enormous mountain of gold coins");
        else
            result =
                tmp_strdup("an absolutely colossal mountain of gold coins");
    } else {                    // credits
        if (amount == 1)
            result = tmp_strdup("a one-credit note");
        else if (amount <= 10)
            result = tmp_strdup("a small wad of cash");
        else if (amount <= 20)
            result = tmp_strdup("a handful of cash");
        else if (amount <= 75)
            result = tmp_strdup("a large wad of cash");
        else if (amount <= 200)
            result = tmp_strdup("a huge wad of cash");
        else if (amount <= 1000)
            result = tmp_strdup("a small pile of cash");
        else if (amount <= 5000)
            result = tmp_strdup("a big pile of cash");
        else if (amount <= 10000)
            result = tmp_strdup("a large heap of cash");
        else if (amount <= 20000)
            result = tmp_strdup("a huge mound of cash");
        else if (amount <= 75000)
            result = tmp_strdup("an enormous mound of cash");
        else if (amount <= 150000)
            result = tmp_strdup("a small mountain of cash money");
        else if (amount <= 250000)
            result = tmp_strdup("a mountain of cash money");
        else if (amount <= 500000)
            result = tmp_strdup("a huge mountain of cash");
        else if (amount <= 1000000)
            result = tmp_strdup("an enormous mountain of cash");
        else
            result = tmp_strdup("an absolutely colossal mountain of cash");
    }
    return result;
}

struct obj_data *
create_money(int amount, int mode)
{
    struct obj_data *obj;
    struct extra_descr_data *new_descr;

    if (amount <= 0) {
        errlog("Try to create negative or 0 money.");
        return NULL;
    }
    obj = make_object();
    CREATE(new_descr, struct extra_descr_data, 1);

    if (mode == 0) {
        if (amount == 1) {
            obj->aliases = strdup("coin gold");
            new_descr->keyword = strdup("coin gold");
            obj->name = strdup("a gold coin");
            obj->line_desc = strdup("One miserable gold coin is lying here.");
        } else {
            obj->aliases = strdup("coins gold");
            new_descr->keyword = strdup("coins gold");
            obj->name = strdup(money_desc(amount, mode));
            obj->line_desc =
                strdup(tmp_capitalize(tmp_sprintf("%s is lying here.",
                        obj->name)));
        }
        GET_OBJ_MATERIAL(obj) = MAT_GOLD;
    } else {                    // credits
        if (amount == 1) {
            obj->aliases = strdup("credit money note one-credit");
            obj->name = strdup("a one-credit note");
            obj->line_desc =
                strdup("A single one-credit note has been dropped here.");
            new_descr->keyword = strdup("credit money note one-credit");
        } else {
            obj->aliases = strdup("credits money cash");
            new_descr->keyword = strdup("credits money cash");
            obj->name = strdup(money_desc(amount, mode));
            obj->line_desc =
                strdup(tmp_capitalize(tmp_sprintf("%s is lying here.",
                        obj->name)));
        }
        GET_OBJ_MATERIAL(obj) = MAT_PAPER;
    }

    if (amount == 1) {
        if (mode)
            new_descr->description = strdup("It's one almighty credit!");
        else
            new_descr->description =
                strdup("It's just one miserable little gold coin.");
    } else if (amount < 10) {
        new_descr->description =
            strdup(tmp_sprintf("There are %d %s.", amount,
                mode ? "credits" : "coins"));
    } else if (amount < 100) {
        new_descr->description =
            strdup(tmp_sprintf("There are about %d %s.", 10 * (amount / 10),
                mode ? "credits" : "coins"));
    } else if (amount < 1000) {
        new_descr->description =
            strdup(tmp_sprintf("It looks to be about %d %s.",
                100 * (amount / 100), mode ? "credits" : "coins"));
    } else if (amount < 100000) {
        new_descr->description =
            strdup(tmp_sprintf("You guess there are, maybe, %d %s.",
                1000 * ((amount / 1000) + number(0, (amount / 1000))),
                mode ? "credits" : "coins"));
    } else {
        new_descr->description =
            strdup(tmp_sprintf("There are a LOT of %s.",
                mode ? "credits" : "coins"));
    }
    new_descr->next = NULL;
    obj->ex_description = new_descr;

    GET_OBJ_VAL(obj, 1) = mode;
    GET_OBJ_TYPE(obj) = ITEM_MONEY;
    GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
    GET_OBJ_VAL(obj, 0) = amount;
    obj->shared = null_obj_shared;
    GET_OBJ_COST(obj) = amount;

    return obj;
}

/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

// is_weird helps to ignore 'special' items that shouldnt be there
bool
is_weird(struct creature *ch, struct obj_data *obj, struct creature *vict)
{
    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        return false;

    if (obj) {
        if (GET_OBJ_VNUM(obj) == BLOOD_VNUM)
            return true;
        if (!OBJ_APPROVED(obj)
            && !is_authorized(ch, TESTER, NULL)
            && !NPC2_FLAGGED(ch, NPC2_UNAPPROVED))
            return true;
    }

    if (vict && IS_NPC(vict)) {
        if (NPC2_FLAGGED(vict, NPC2_UNAPPROVED)
            && !is_authorized(ch, TESTER, NULL)
            && !NPC2_FLAGGED(ch, NPC2_UNAPPROVED))
            return true;
    }

    return false;
}

int
generic_find(char *arg, int bitvector, struct creature *ch,
    struct creature **tar_ch, struct obj_data **tar_obj)
{
    int i, found;
    char *name;

    if (IS_SET(bitvector, FIND_CHAR_ROOM | FIND_CHAR_WORLD)) {
        if (tar_ch) {
            *tar_ch = NULL;
        } else {
            errlog("NULL tar_ch passed to generic_find()");
            return false;
        }
    }

    if (IS_SET(bitvector,
               FIND_OBJ_EQUIP | FIND_OBJ_EQUIP_CONT
               | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD)) {
        if (tar_obj) {
            *tar_obj = NULL;
        } else {
            errlog("NULL tar_obj passed to generic_find()");
            return false;
        }
    }

    name = tmp_getword(&arg);

    if (!*name)
        return (0);

    if (IS_SET(bitvector, FIND_CHAR_ROOM)) {    /* Find person in room */
        if ((*tar_ch = get_char_room_vis(ch, name))) {
            return (FIND_CHAR_ROOM);
        }
    }
    if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
        if ((*tar_ch = get_char_vis(ch, name))) {
            return (FIND_CHAR_WORLD);
        }
    }
    if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
        for (found = false, i = 0; i < NUM_WEARS && !found; i++)
            if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->aliases)) {
                *tar_obj = GET_EQ(ch, i);
                found = true;
            }
        if (found) {
            return (FIND_OBJ_EQUIP);
        }
    }
    if (IS_SET(bitvector, FIND_OBJ_EQUIP_CONT)) {
        for (found = false, i = 0; i < NUM_WEARS && !found; i++)
            if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->aliases) &&
                (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_CONTAINER) &&
                (i != WEAR_BACK)) {
                *tar_obj = GET_EQ(ch, i);
                found = true;
            }
        if (found) {
            return (FIND_OBJ_EQUIP_CONT);
        }
    }
    if (IS_SET(bitvector, FIND_OBJ_INV)) {
        if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
            return (FIND_OBJ_INV);
        }
    }
    if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
        *tar_obj = get_obj_in_list_vis(ch, name, ch->in_room->contents);
        while (*tar_obj) {
            if (IS_SET(bitvector, FIND_IGNORE_WIERD)
                && is_weird(ch, *tar_obj, NULL)) {
                *tar_obj =
                    get_obj_in_list_vis(ch, name, (*tar_obj)->next_content);
                continue;
            }
            break;
        }
        if (*tar_obj)
            return (FIND_OBJ_ROOM);
    }

    if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
        if ((*tar_obj = get_obj_vis(ch, name))) {
            return (FIND_OBJ_WORLD);
        }
    }
    return (0);
}

// The reaction class compiles a list of allow/deny rules into a string
// of int8_t codes, and executes them on a match.  This way, we can compile
// an arbitrary number of rules into a convenient, fast format.

// The int8_t code is:
//    bit 4-7
//       0 == deny
//       1 == allow
//       2 == deny (not)
//       3 == allow (not)
//    bit 0 - 3
//       0 == terminate search
//       1 == all
//       2 == good align
//       3 == evil align
//       4 == neutral align
//       5 == class (next int8_t is class + 1)
//       6 == race (next int8_t is race + 1)
//       7 == clan (next int8_t is clan + 1)
//       8 == killer
//       9 == thief
//       a == player
//       b == level less than (next int8_t is number compared to)
//       c == level greater than (next int8_t is number compared to)
// so 0x12 means 'accept good'
// and 0x01 means 'deny all'

int parse_char_class(char *);
int parse_race(char *);

struct reaction *
make_reaction(void)
{
    struct reaction *reaction;

    CREATE(reaction, struct reaction, 1);
    return reaction;
}

void
free_reaction(struct reaction *reaction)
{
    free(reaction->reaction);
    free(reaction);
}

bool
add_reaction(struct reaction *reaction, char *arg)
{
    char *tmp;
    struct clan_data *clan;
    char *condition;
    char new_reaction[3];
    char *action_str;
    enum decision_t action;

    action_str = tmp_getword(&arg);
    if (!strcmp(action_str, "allow"))
        action = ALLOW;
    else if (!strcmp(action_str, "deny"))
        action = DENY;
    else
        return false;

    new_reaction[0] = (action == ALLOW) ? 0x10 : 0x00;

    condition = tmp_getword(&arg);
    if (!strcmp(condition, "not")) {
        condition = tmp_getword(&arg);
        new_reaction[0] |= 0x20;
    }
    new_reaction[1] = new_reaction[2] = '\0';
    if (is_abbrev(condition, "all"))
        new_reaction[0] |= 0x01;
    else if (is_abbrev(condition, "good"))
        new_reaction[0] |= 0x02;
    else if (is_abbrev(condition, "evil"))
        new_reaction[0] |= 0x03;
    else if (is_abbrev(condition, "neutral"))
        new_reaction[0] |= 0x04;
    else if ((new_reaction[1] = parse_char_class(condition) + 1) != 0)
        new_reaction[0] |= 0x05;
    else if ((new_reaction[1] = parse_race(condition) + 1) != 0)
        new_reaction[0] |= 0x06;
    else if ((clan = clan_by_name(condition)) != NULL) {
        new_reaction[0] |= 0x07;
        new_reaction[1] = clan->number + 1;
    } else if (is_abbrev(condition, "criminal"))
        new_reaction[0] |= 0x08;
    else if (is_abbrev(condition, "player"))
        new_reaction[0] |= 0x09;
    else if (is_abbrev(condition, "lvl<")) {
        new_reaction[0] |= 0x0a;
        new_reaction[1] = atoi(arg);
        if (new_reaction[1] < 1 || new_reaction[1] > 70)
            return false;
    } else if (is_abbrev(condition, "lvl>")) {
        new_reaction[0] |= 0x0b;
        new_reaction[1] = atoi(arg);
        if (new_reaction[1] < 1 || new_reaction[1] > 70)
            return false;
    } else if (is_abbrev(condition, "clanleader")) {
        new_reaction[0] |= 0x0c;
    } else
        return false;

    if (reaction->reaction) {
        tmp = tmp_strcat(reaction->reaction, new_reaction, NULL);
        free(reaction->reaction);
        reaction->reaction = strdup(tmp);
    } else {
        reaction->reaction = strdup(new_reaction);
    }

    return true;
}

enum decision_t
react(struct reaction *reaction, struct creature *ch)
{
    char *read_pt;
    bool match, wantmatch;
    enum decision_t action;

    if (!reaction->reaction)
        return UNDECIDED;

    for (read_pt = reaction->reaction; *read_pt; read_pt++) {
        match = false;
        action = ((*read_pt) & 0x10) ? ALLOW : DENY;
        wantmatch = ((*read_pt) & 0x20) ? false : true;
        switch (*read_pt & 0xF) {
        case 0:                // terminate
            return UNDECIDED;
        case 1:                // all
            return action;
        case 2:                // good
            if (IS_GOOD(ch))
                match = true;
            break;
        case 3:                // evil
            if (IS_EVIL(ch))
                match = true;
            break;
        case 4:                // neutral
            if (IS_NEUTRAL(ch))
                match = true;
            break;
        case 5:
            read_pt++;
            if (GET_CLASS(ch) + 1 == *read_pt)
                match = true;
            else if (GET_REMORT_CLASS(ch) + 1 == *read_pt)
                match = true;
            break;
        case 6:
            if (GET_RACE(ch) + 1 == *(++read_pt))
                match = true;
            break;
        case 7:
            if (GET_CLAN(ch) + 1 == *(++read_pt))
                match = true;
            break;
        case 8:
            if (IS_CRIMINAL(ch))
                match = true;
            break;
        case 9:
            if (IS_PC(ch))
                match = true;
            break;
        case 10:
            if (GET_LEVEL(ch) < *(++read_pt))
                match = true;
            break;
        case 11:
            if (GET_LEVEL(ch) > *(++read_pt))
                match = true;
            break;
        case 12:
            if (IS_PC(ch) && PLR_FLAGGED(ch, PLR_CLAN_LEADER))
                match = true;
            break;
        default:
            errlog("Invalid reaction code %x", *read_pt);
            return UNDECIDED;
        }

        if (match == wantmatch)
            return action;
    }
    return UNDECIDED;
}
