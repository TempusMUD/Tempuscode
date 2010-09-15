//
// File: flow_room.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/* ************************************************************************
*         flow_room.c                                   Part of TempusMUD *
*                                                                         *
*   Hacked by Fireball                                                    *
**************************************************************************/

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#define __flow_room_c__

#include <stdio.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "flow_room.h"
#include "vehicle.h"
#include "smokes.h"
#include "spells.h"
#include "char_class.h"
#include "bomb.h"
#include "house.h"
#include "fight.h"
#include "prog.h"
#include "weather.h"

/* external structs */
extern struct obj_data *object_list;
extern struct zone_data *zone_table;
void npc_steal(struct creature *ch, struct creature *victim);
void hunt_victim(struct creature *ch);
int general_search(struct creature *ch, struct special_search_data *srch,
    int mode);

const char *flow_types[] = {
    "None",
    "Wind",
    "Falling",
    "River",
    "Water Vortex",
    "Underwater",
    "Conduit",
    "Conveyor",
    "Lava Flow",
    "River of Fire",
    "Volcanic Updraft",
    "Rotating Disc",
    "Escalator",
    "Sinking_Swamp",
    "Unseen_Force",
    "Elemental_Wind",
    "Quicksand",
    "Crowds",
    "\n"
};

const char *char_flow_msg[NUM_FLOW_TYPES + 1][3] = {
    {"$n flows %s.",            // Default
            "$n flows in from %s.",
        "You flow %s."},
    {"$n is blown %s by the wind.", // Winds
            "$n blows in from %s.",
        "The wind blows you %s."},
    {"$n falls %s.",            // Falling
            "$n falls in from %s.",
        "You fall %s."},
    {"The current pulls $n %s.",    // River Surface
            "$n flows in from %s on the current.",
        "The current pulls you %s."},
    {"$n is sucked %s by the current!", // Water Vortex
            "$n is sucked in from %s by the current.",
        "You are sucked %s by the current!"},
    {"The current pulls $n %s.",    // Underwater
            "$n flows in from %s on the current.",
        "The current pulls you %s."},
    {"$n spirals off %s through the conduit.",  // Astral Conduit
            "$n spirals in from %s through the conduit.",
        "You spiral %s through the conduit."},
    {"$n moves off %s on the conveyor.",    // Conveyor Belt
            "$n moves in from %s on the conveyor.",
        "The conveyor pulls you %s."},
    {"$n is dragged %s by the lava flow.",  // Lava Flow
            "$n moves in from %s with the lava flow.",
        "The lava flow drags you relentlessly %s."},
    {"The fiery current pulls $n %s.",  // River Surface
            "$n flows in from %s on the fiery current.",
        "The fiery current pulls you %s."},
    {"$n is blown %s by the hot updraft.",  // Volcanic Winds
            "$n is blown in from %s by the hot updraft.",
        "The hot updraft blows you %swards."},
    {"The rotating disc takes $n %sward.",  // Rotating Disc
            "The rotating disc brings $n in from %s.",
        "The rotating disc takes you %sward."},
    {"$n moves %s along the escalator.",    // Escalator
            "$n comes into view from %s riding the escalator.",
        "You ride the escalator %s."},
    {"$n is dragged %s into the swamp.",    // Sinking Swamp
            "$n is dragged in from %s.",
        "You sink suddenly into the swamp!"},
    {"$n is dragged %s by an unseen force.",    // Unseen force
            "$n is dragged in from %s.",
        "You are dragged %s by an unseen force!!"},
    {"$n is forced %sward by the strong elemental winds.",  // Elemental Wind
            "$n is forced in from %s by the strong elemental winds.",
        "The strong elemental winds force you %s."},
    {"$n struggles frantically as they $e is pulled %s by the quicksand!",  // Quicksand
            "The quicksand pulls $n in from the %s as $e struggles to break free!",
        "You struggle frantically as the quicksand pulls you %s!",},

    {"$n is beaten %sward by the bloodthirsty mob!",    // Crowds
            "A wild mob of spectators shoves $n in from %s",
        "Your reluctance is overcome as the bloodthirsty mob shoves you %s!",},

    {"\n", "\n", "\n"}        /******* LEAVE THIS LINE LAST *********/
};

const char *obj_flow_msg[NUM_FLOW_TYPES + 1][2] = {
    {"$p flows %s.",            /* Default        */
        "$p flows in from %s."},
    {"$p is blown %s by the wind.", /* Winds          */
        "$p blows in from %s."},
    {"$p falls %s.",            /* Falling        */
        "$p falls in from %s."},
    {"The current pulls $p %s.",    /* River Surface  */
        "$p flows in from %s on the current."},
    {"$p is sucked %s by the current!", /* Water Vortex   */
        "$p is sucked in from %s by the current."},
    {"The current pulls $p %s.",    /* Underwater     */
        "$p flows in from %s on the current."},
    {"$p spirals off %s through the conduit.",  /* Astral Conduit */
        "$p spirals in from %s through the conduit."},
    {"$p moves off %s on the conveyor.",    /* Conveyor Belt  */
        "$p moves in from %s on the conveyor."},
    {"$p flows %s with the lava.",  /* Lava Flow      */
        "$p moves in from %s with the lava flow."},
    {"The fiery current pulls $p %s.",  /* Fiery River    */
        "$p flows in from %s on the fiery current."},
    {"$p is blown %s by the hot updraft.",  /* Volc. Updraft  */
        "$p is blown in from %s by the hot updraft."},
    {"The rotating disc takes $p %sward.",  /* Rot. Disc      */
        "The rotating disc brings $p in from %s."},
    {"$p moves %s with the motion of the escalator.",   /* Escalator      */
        "$p is brought in from %s on the escalator."},
    {"$p is dragged %s into the swamp.",    /* Sinking Swamp  */
        "$p is dragged in from %s."},
    {"$p is dragged %s by an unseen force.",    // Unseen force
        "$p is dragged in from %s by an unseen force."},
    {"$p is pushed %sward by the strong elemental winds.",  // Elemental Wind
        "$p is pushed in from %s by the strong elemental winds."},
    {"$p is pulled %s by the quicksand.",   // Quicksand
        "$p is pulled in from %s by the quicksand."},
    {"Someone in the crowd picks up $p and pitches it %s.", // Crowd
        "$p is thrown in from %s by the angry mob."},
    {"\n", "\n"}
};

void
flow_one_creature(struct creature *ch, struct room_data *rnum, int pulse,
    int dir)
{
    struct room_data *was_in;
    struct obj_data *obj, *next_obj;
    struct special_search_data *srch;

    if (!ch)
        return;

    if (CHAR_CUR_PULSE(ch) == pulse ||
        (IS_MOB(ch) &&
            (MOB2_FLAGGED(ch, MOB2_NO_FLOW) ||
                IS_SET(ABS_EXIT(rnum, dir)->exit_info, EX_NOMOB)
                || IS_SET(ROOM_FLAGS(ABS_EXIT(rnum,
                            dir)->to_room), ROOM_NOMOB)))
        || PLR_FLAGGED(ch, PLR_HALT) || (!IS_NPC(ch)
            && !ch->desc)
        || (ROOM_FLAGGED(ABS_EXIT(rnum, dir)->to_room,
                ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GRGOD)
        || (GET_POSITION(ch) == POS_FLYING
            && (FLOW_TYPE(rnum) == F_TYPE_RIVER_SURFACE
                || FLOW_TYPE(rnum) == F_TYPE_LAVA_FLOW
                || FLOW_TYPE(rnum) == F_TYPE_RIVER_FIRE
                || FLOW_TYPE(rnum) == F_TYPE_FALLING
                || FLOW_TYPE(rnum) == F_TYPE_SINKING_SWAMP || IS_DRAGON(ch)))
        || (AFF_FLAGGED(ch, AFF_WATERWALK)
            && FLOW_TYPE(rnum) == F_TYPE_SINKING_SWAMP))
        return;
    if (!can_enter_house(ch, rnum->number))
        return;

    CHAR_CUR_PULSE(ch) = pulse;

    act(tmp_sprintf(char_flow_msg[(int)FLOW_TYPE(rnum)][MSG_TORM_1],
            to_dirs[dir]), true, ch, 0, 0, TO_ROOM);

    act(tmp_sprintf(char_flow_msg[(int)FLOW_TYPE(rnum)][MSG_TOCHAR],
            to_dirs[dir]), false, ch, 0, 0, TO_CHAR);

    char_from_room(ch, true);
    char_to_room(ch, ABS_EXIT(rnum, dir)->to_room, true);
    look_at_room(ch, ch->in_room, 0);
    act(tmp_sprintf(char_flow_msg[(int)FLOW_TYPE(rnum)][MSG_TORM_2],
            from_dirs[dir]), true, ch, 0, 0, TO_ROOM);

    if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH)
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {

        was_in = ch->in_room;
        log_death_trap(ch);
        death_cry(ch);
        die(ch, NULL, -1);

        if (was_in->number == 34004) {
            for (obj = was_in->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                damage_eq(NULL, obj, dice(10, 100), -1);
            }
        }
    } else {
        for (srch = ch->in_room->search; srch; srch = srch->next) {
            if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER)
                && SRCH_OK(ch, srch))
                if (general_search(ch, srch, 0) == 2)
                    break;
        }
    }
}

void
flow_room(int pulse)
{

    struct obj_data *obj = NULL, *next_obj = NULL;
    register struct zone_data *zone = NULL;
    register struct room_data *rnum = NULL;
    int dir;
    struct room_affect_data *aff, *next_aff;
    struct room_trail_data *trail = NULL;

    for (zone = zone_table; zone; zone = zone->next) {

        if (ZONE_FLAGGED(zone, ZONE_FROZEN) ||
            zone->idle_time >= ZONE_IDLE_TIME)
            continue;

        for (rnum = zone->world; rnum; rnum = rnum->next) {

            // Update room affects
            if (!(pulse % (5 RL_SEC)))
                for (aff = rnum->affects; aff; aff = next_aff) {
                    next_aff = aff->next;
                    if (aff->duration > 0)
                        aff->duration--;
                    if (aff->duration <= 0) {
                        if (aff->duration < 0) {
                            errlog(" Room aff type %d has %d duration at %d.",
                                aff->type, aff->duration, rnum->number);
                        }
                        affect_from_room(rnum, aff);
                    }
                }
            // Alignment ambience
            if (!(pulse % (4 RL_SEC))) {
                if (zone->flags & ZONE_EVIL_AMBIENCE) {
                    void apply_evil(struct creature *tch, gpointer ignore) {
                        if (!IS_NPC(tch) && GET_ALIGNMENT(tch) > -1000 &&
                            !affected_by_spell(tch,
                                SPELL_SHIELD_OF_RIGHTEOUSNESS)) {
                            GET_ALIGNMENT(tch) -= 1;
                            check_eq_align(tch);
                        }
                    }
                    g_list_foreach(rnum->people, (GFunc) apply_evil, 0);
                }
                if (zone->flags & ZONE_GOOD_AMBIENCE) {
                    void apply_good(struct creature *tch, gpointer ignore) {
                        if (!IS_NPC(tch) && GET_ALIGNMENT(tch) < 1000 &&
                            !affected_by_spell(tch,
                                SPELL_SPHERE_OF_DESECRATION)) {
                            GET_ALIGNMENT(tch) += 1;
                            check_eq_align(tch);
                        }
                    }
                    g_list_foreach(rnum->people, (GFunc) apply_good, 0);
                }
            }
            // Active flows only
            if (!FLOW_SPEED(rnum) || pulse % (PULSE_FLOWS * FLOW_SPEED(rnum))
                || (!ABS_EXIT(rnum, (dir = (int)FLOW_DIR(rnum)))
                    || ABS_EXIT(rnum, dir)->to_room == NULL
                    || (IS_SET(ABS_EXIT(rnum, dir)->exit_info, EX_CLOSED))
                    || (IS_SET(ABS_EXIT(rnum, dir)->exit_info, EX_NOPASS))))
                continue;

            if (FLOW_TYPE(rnum) < 0 || FLOW_TYPE(rnum) >= NUM_FLOW_TYPES)
                FLOW_TYPE(rnum) = F_TYPE_NONE;

            /* nix flows */
            if (FLOW_TYPE(rnum) != F_TYPE_CONVEYOR &&
                FLOW_TYPE(rnum) != F_TYPE_ROTATING_DISC &&
                FLOW_TYPE(rnum) != F_TYPE_ESCALATOR) {
                while ((trail = rnum->trail)) {
                    rnum->trail = trail->next;
                    if (trail->name)
                        free(trail->name);
                    if (trail->aliases)
                        free(trail->aliases);
                    free(trail);
                }
            }

            void flow_one(gpointer it, gpointer ignore) {
                flow_one_creature((struct creature *)it, rnum, pulse, dir);
            }

            g_list_foreach(rnum->people, flow_one, 0);

            if ((obj = rnum->contents)) {
                for (; obj; obj = next_obj) {
                    next_obj = obj->next_content;

                    if (OBJ_CUR_PULSE(obj) == pulse ||
                        (!CAN_WEAR(obj, ITEM_WEAR_TAKE) &&
                            GET_OBJ_VNUM(obj) != BLOOD_VNUM &&
                            GET_OBJ_VNUM(obj) != ICE_VNUM) ||
                        (GET_OBJ_WEIGHT(obj) > number(5, FLOW_SPEED(rnum) * 10)
                            && !number(0, FLOW_SPEED(rnum))))
                        continue;

                    OBJ_CUR_PULSE(obj) = pulse;

                    act(tmp_sprintf(obj_flow_msg[(int)
                                FLOW_TYPE(rnum)][MSG_TORM_1], to_dirs[dir]),
                        true, 0, obj, 0, TO_ROOM);
                    obj_from_room(obj);
                    obj_to_room(obj, ABS_EXIT(rnum, dir)->to_room);
                    act(tmp_sprintf(obj_flow_msg[(int)
                                FLOW_TYPE(rnum)][MSG_TORM_2], from_dirs[dir]),
                        true, 0, obj, 0, TO_ROOM);
                }
            }
        }
    }
}

void
dynamic_object_pulse()
{

    register struct obj_data *obj = NULL, *next_obj = NULL;
    struct creature *vict = NULL;
    struct room_data *fall_to = NULL;
    static int counter = 0;
    int tenpulse = 0, fallpulse = 0;

    counter += PULSE_FLOWS;

    fallpulse = !(counter % (3 RL_SEC));
    tenpulse = !(counter % (10 RL_SEC));

    for (obj = object_list; obj; obj = next_obj) {
        next_obj = obj->next;

        // Nothing happens to objects in the Void
        if (obj->in_room == zone_table->world)
            continue;

        if (fallpulse &&
            obj->in_room && room_is_open_air(obj->in_room) &&
            (CAN_WEAR(obj, ITEM_WEAR_TAKE) || OBJ_IS_SOILAGE(obj)) &&
            obj->in_room->dir_option[DOWN] &&
            (fall_to = obj->in_room->dir_option[DOWN]->to_room) &&
            fall_to != obj->in_room &&
            !IS_SET(obj->in_room->dir_option[DOWN]->exit_info, EX_CLOSED)) {
            if (obj->in_room->people)
                act("$p falls downward through the air!", true, 0, obj, 0,
                    TO_ROOM);
            obj_from_room(obj);
            obj_to_room(obj, fall_to);
            if (obj->in_room->people)
                act("$p falls in from above.", false, 0, obj, 0, TO_ROOM);
            continue;
        }

        if (IS_FUSE(obj) && obj->in_obj && IS_BOMB(obj->in_obj) &&
            FUSE_STATE(obj) &&
            (FUSE_IS_BURN(obj) || FUSE_IS_ELECTRONIC(obj))) {
            FUSE_TIMER(obj)--;
            if (!FUSE_TIMER(obj)) {
                next_obj = (detonate_bomb(obj->in_obj));
            }
        } else if (tenpulse) {
            if (IS_CIGARETTE(obj) && SMOKE_LIT(obj)) {
                CUR_DRAGS(obj)--;
                if (CUR_DRAGS(obj) <= 0) {
                    if (obj->worn_by || obj->carried_by)
                        act("$p burns itself out.",
                            true,
                            obj->worn_by ? obj->worn_by : obj->carried_by, obj,
                            0, TO_CHAR);
                    else if (obj->in_room && obj->in_room->people)
                        act("$p burns itself out.", true, 0, obj, 0, TO_ROOM);
                    extract_obj(obj);
                    continue;
                }
                if (obj->in_room && OUTSIDE(obj)
                    && obj->in_room->zone->weather->sky >= SKY_RAINING)
                    SMOKE_LIT(obj) = 0;
            }
            if (IS_COMMUNICATOR(obj) && ENGINE_STATE(obj) &&
                (CUR_ENERGY(obj) >= 0)) {
                CUR_ENERGY(obj)--;

                if (CUR_ENERGY(obj) <= 0) {
                    CUR_ENERGY(obj) = 0;
                    ENGINE_STATE(obj) = 0;

                    if ((vict = obj->carried_by) || (vict = obj->worn_by)) {
                        act("$p auto switching off: depleted of energy.",
                            false, vict, obj, 0, TO_CHAR | TO_SLEEP);
                    }
                }
                continue;
            }

            if (IS_DEVICE(obj) && ENGINE_STATE(obj) && USE_RATE(obj) > 0) {
                CUR_ENERGY(obj) -= USE_RATE(obj);
                if (CUR_ENERGY(obj) <= 0) {
                    CUR_ENERGY(obj) = 0;
                    if ((vict = obj->carried_by) || (vict = obj->worn_by)) {
                        act("$p auto switching off: depleted of energy.",
                            false, vict, obj, 0, TO_CHAR | TO_SLEEP);
                        if (obj->worn_by) {
                            apply_object_affects(obj->worn_by, obj, false);
                            ENGINE_STATE(obj) = 0;
                            affect_total(obj->worn_by);
                        }
                    }
                    ENGINE_STATE(obj) = 0;
                }
            }
            // radioactive objects worn/carried                           (RADIOACTIVE)
            if (OBJ_IS_RAD(obj) && ((vict = obj->carried_by)
                    || (vict = obj->worn_by))
                && !ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)
                && !CHAR_WITHSTANDS_RAD(vict)) {

                if (affected_by_spell(vict, SKILL_RADIONEGATION)) {

                    GET_HIT(vict) = MAX(0, GET_HIT(vict) - dice(1, 7));

                    if (GET_MOVE(vict) > 10) {
                        GET_MOVE(vict) = MAX(10, GET_MOVE(vict) - dice(2, 7));
                    }

                    else {
                        add_rad_sickness(vict, 10);
                    }
                }

                else {

                    GET_HIT(vict) = MAX(0, GET_HIT(vict) - dice(2, 7));

                    if (GET_MOVE(vict) > 5)
                        GET_MOVE(vict) = MAX(5, GET_MOVE(vict) - dice(2, 7));

                    add_rad_sickness(vict, 20);
                }
            }
        }
    }
}

void
affect_to_room(struct room_data *room, struct room_affect_data *aff)
{

    struct room_affect_data *tmp_aff;
    int i;

    if (aff->type < NUM_DIRS && aff->type > -1) {
        if (room->dir_option[(int)aff->type]) {
            for (i = 0; i < 32; i++)
                if (IS_SET(aff->flags, (1 << i)) &&
                    IS_SET(room->dir_option[(int)aff->type]->exit_info,
                        (1 << i)))
                    REMOVE_BIT(aff->flags, (1 << i));
        } else
            return;

        if (aff->flags)
            SET_BIT(room->dir_option[(int)aff->type]->exit_info, aff->flags);
        else
            return;
    } else if (aff->type == RM_AFF_FLAGS) {
        for (i = 0; i < 32; i++)
            if (IS_SET(aff->flags, (1 << i))
                && IS_SET(room->room_flags, (1 << i)))
                REMOVE_BIT(aff->flags, (1 << i));

        if (aff->flags)
            SET_BIT(room->room_flags, aff->flags);
        else
            return;
    } else if (aff->spell_type) {
    } else if (!(aff->type == -1) && aff->spell_type) {
        errlog("Invalid aff->type passed to affect_to_room.");
        return;
    }

    CREATE(tmp_aff, struct room_affect_data, 1);

    tmp_aff->description = aff->description;
    tmp_aff->next = room->affects;
    tmp_aff->duration = aff->duration;
    tmp_aff->flags = aff->flags;
    tmp_aff->level = aff->level;
    tmp_aff->type = aff->type;
    tmp_aff->owner = aff->owner;
    tmp_aff->spell_type = aff->spell_type;
    for (i = 0; i < 4; i++)
        tmp_aff->val[i] = aff->val[i];
    room->affects = tmp_aff;

}

void
affect_from_room(struct room_data *room, struct room_affect_data *aff)
{

    struct room_affect_data *tmp_aff;

    for (tmp_aff = room->affects; tmp_aff; tmp_aff = tmp_aff->next) {
        if (tmp_aff == aff)
            room->affects = tmp_aff->next;
        else if (tmp_aff->next == aff)
            tmp_aff->next = aff->next;
        else
            continue;

        break;
    }

    if (!tmp_aff)
        errlog("affect_from_room(): aff not found in room->affects");

    if (aff->type == RM_AFF_FLAGS)
        REMOVE_BIT(room->room_flags, aff->flags);
    else if (aff->type < NUM_DIRS && room->dir_option[(int)aff->type])
        REMOVE_BIT(room->dir_option[(int)aff->type]->exit_info, aff->flags);
    else if (aff->type != RM_AFF_OTHER)
        errlog("affect_from_room(): Invalid aff->type %d", aff->type);

    if (aff->description)
        free(aff->description);

    free(aff);
}

struct room_affect_data *
room_affected_by(struct room_data *room, int type)
{
    struct room_affect_data *aff;

    for (aff = room->affects; aff; aff = aff->next) {
        if (aff->spell_type == type)
            return aff;
    }

    return false;
}

#undef __flow_room_c__
