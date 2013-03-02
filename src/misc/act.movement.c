/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.movement.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
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
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "obj_data.h"
#include "strutil.h"
#include "actions.h"
#include "weather.h"
#include "search.h"

/* external vars  */
extern struct descriptor_data *descriptor_list;
extern const struct dex_skill_type dex_app_skill[];
extern const int rev_dir[];
extern const char *dirs[];
extern const char *from_dirs[];
extern const char *to_dirs[];
extern const int8_t movement_loss[];
extern struct obj_data *object_list;
extern struct creatureList mountedList;

/* external functs */
ACMD(do_gen_points);
long special(struct creature *ch, int cmd, int subcmd, char *arg,
    enum special_mode spec_mode);
int find_eq_pos(struct creature *ch, struct obj_data *obj, char *arg);
int general_search(struct creature *ch, struct special_search_data *srch,
    int mode);
void update_trail(struct creature *ch, struct room_data *rm, int dir, int j);
int apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
    int pos);

static inline bool
door_is_openable(struct creature *ch, struct obj_data *obj, int door)
{
    if (obj) {
        if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
            return false;

        if (IS_OBJ_TYPE(obj, ITEM_VEHICLE) ||
            IS_OBJ_TYPE(obj, ITEM_PORTAL) ||
            IS_OBJ_TYPE(obj, ITEM_SCUBA_MASK) ||
            IS_OBJ_TYPE(obj, ITEM_WINDOW))
            return true;

        if (IS_OBJ_TYPE(obj, ITEM_CONTAINER) && !GET_OBJ_VAL(obj, 3))
            return true;

        if (IS_OBJ_TYPE(obj, ITEM_V_WINDOW) && CAR_OPENABLE(obj))
            return true;

        return false;
    } else {
        return IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR);
    }
}

static inline bool
door_is_flagged(struct creature *ch, int door, int flag)
{
    return IS_SET(EXIT(ch, door)->exit_info, flag);
}

static inline bool
door_is_open(struct creature *ch, struct obj_data *obj, int door)
{
    if (obj)
        return !IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
    else
        return !door_is_flagged(ch, door, EX_CLOSED);
}

static inline bool
door_is_locked(struct creature *ch, struct obj_data *obj, int door)
{
    if (obj)
        return IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
    else
        return door_is_flagged(ch, door, EX_LOCKED);
}

static inline bool
door_is_unlocked(struct creature *ch, struct obj_data *obj, int door)
{
    return !door_is_locked(ch, obj, door);
}

static inline bool
door_is_pickproof(struct creature *ch, struct obj_data *obj, int door)
{
    if (obj)
        return IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF);
    else
        return door_is_flagged(ch, door, EX_PICKPROOF);
}

static inline int
door_key(struct creature *ch, struct obj_data *obj, int door)
{
    if (obj)
        return GET_OBJ_VAL(obj, 2);
    else
        return EXIT(ch, door)->key;
}

static inline bool
door_is_technical(struct creature *ch, struct obj_data *obj, int door)
{
    return (obj) ? false:door_is_flagged(ch, door, EX_TECH);
}


static inline bool
door_is_heavy(struct creature *ch, struct obj_data *obj, int door)
{
    return (obj) ? false:door_is_flagged(ch, door, EX_HEAVY_DOOR);
}

static inline bool
door_is_special(struct creature *ch, struct obj_data *obj, int door)
{
    return (obj) ? false:door_is_flagged(ch, door, EX_SPECIAL);
}

bool
can_travel_sector(struct creature *ch, int sector_type, bool active)
{
    struct obj_data *obj;
    int i;

    if (GET_LEVEL(ch) > LVL_ELEMENT)
        return true;

    if (sector_type == SECT_UNDERWATER ||
        sector_type == SECT_DEEP_OCEAN ||
        sector_type == SECT_PITCH_SUB ||
        sector_type == SECT_WATER_NOSWIM ||
        sector_type == SECT_ELEMENTAL_OOZE ||
        sector_type == SECT_ELEMENTAL_WATER || sector_type == SECT_FREESPACE) {

        if (IS_RACE(ch, RACE_FISH))
            return true;

        if (sector_type == SECT_WATER_NOSWIM) {
            if (AFF_FLAGGED(ch, AFF_WATERWALK)
                || GET_POSITION(ch) >= POS_FLYING || (IS_ELEMENTAL(ch)
                    && GET_CLASS(ch) == CLASS_WATER))
                return true;
            for (obj = ch->carrying; obj; obj = obj->next_content)
                if (IS_OBJ_TYPE(obj, ITEM_BOAT))
                    return true;
            for (i = 0; i < NUM_WEARS; i++) {
                if (ch->equipment[i] &&
                    IS_OBJ_TYPE(ch->equipment[i], ITEM_BOAT))
                    return true;
            }
        }

        if (!NEEDS_TO_BREATHE(ch) ||
            ((sector_type == SECT_UNDERWATER ||
                    sector_type == SECT_WATER_NOSWIM) &&
                (AFF_FLAGGED(ch, AFF_WATERBREATH) ||
                    (IS_ELEMENTAL(ch) && GET_CLASS(ch) == CLASS_WATER))))
            return true;

        if ((obj = ch->equipment[WEAR_FACE]) &&
            IS_OBJ_TYPE(obj, ITEM_SCUBA_MASK) &&
            !CAR_CLOSED(obj) &&
            obj->aux_obj && IS_OBJ_TYPE(obj->aux_obj, ITEM_SCUBA_TANK) &&
            (GET_OBJ_VAL(obj->aux_obj, 1) > 0 ||
                GET_OBJ_VAL(obj->aux_obj, 0) < 0)) {
            if (active && GET_OBJ_VAL(obj->aux_obj, 0) > 0) {
                GET_OBJ_VAL(obj->aux_obj, 1)--;
                if (!GET_OBJ_VAL(obj->aux_obj, 1)) {
                    act("A warning indicator reads: $p fully depleted.",
                        false, ch, obj->aux_obj, NULL, TO_CHAR);
                    BREATH_COUNT_OF(ch) = 0;
                } else if (GET_OBJ_VAL(obj->aux_obj, 1) == 5)
                    act("A warning indicator reads: $p air level low.",
                        false, ch, obj->aux_obj, NULL, TO_CHAR);
            }
            return true;
        }
        BREATH_COUNT_OF(ch) += 1;

        if (BREATH_COUNT_OF(ch) < creature_breath_threshold(ch) &&
            BREATH_COUNT_OF(ch) > (creature_breath_threshold(ch) - 2)) {
            send_to_char(ch, "You are running out of breath.\r\n");
            return true;
        }
        return false;
    }

    if (sector_type == SECT_FLYING ||
        sector_type == SECT_ELEMENTAL_AIR ||
        sector_type == SECT_ELEMENTAL_RADIANCE ||
        sector_type == SECT_ELEMENTAL_LIGHTNING ||
        sector_type == SECT_ELEMENTAL_VACUUM) {

        if (AFF_FLAGGED(ch, AFF_INFLIGHT) ||
            (IS_ELEMENTAL(ch) && GET_CLASS(ch) == CLASS_AIR))
            return true;
        for (i = 0; i < NUM_WEARS; i++) {
            if (ch->equipment[i]) {
                if (IS_OBJ_TYPE(ch->equipment[i], ITEM_WINGS))
                    return true;
            }
        }
        return false;
    }

    return true;
}

void
get_giveaway(struct creature *ch, struct creature *vict)
{

    if (IS_NPC(ch) && GET_NPC_VNUM(ch) == UNHOLY_STALKER_VNUM) {
        send_to_char(vict,
            "You feel a cold chill as something evil passes nearby.\r\n");
        return;
    }

    if (!AWAKE(vict) && !AFF_FLAGGED(vict, AFF_SLEEP)) {
        if (!number(0, 2))
            send_to_char(vict, "A noise wakes you up.\r\n");
        else if (!number(0, 1))
            send_to_char(vict, "You are disturbed by an odd noise.\r\n");
        else
            send_to_char(vict, "You are awakened by a noise.\r\n");

        GET_POSITION(vict) = POS_RESTING;
        return;
    }

    switch (ch->in_room->sector_type) {
    case SECT_INSIDE:
    case SECT_CITY:
        if (!number(0, 2))
            send_to_char(vict, "You hear faint footsteps.\r\n");
        else if (!number(0, 1))
            send_to_char(vict, "You think you heard something.\r\n");
        else
            send_to_char(vict, "You hear the sound of movement.\r\n");
        break;
    case SECT_FOREST:
        if (!number(0, 1))
            send_to_char(vict, "You hear a twig snap.\r\n");
        else
            send_to_char(vict, "You hear leaves rustling.\r\n");
        break;
    case SECT_DESERT:
        if (can_see_room(vict, vict->in_room) && !number(0, 1))
            send_to_char(vict, "You see footprints appear in the sand.\r\n");
        else
            send_to_char(vict, "You hear something moving.\r\n");
        break;
    case SECT_SWAMP:
    case SECT_MUDDY:
        send_to_char(vict, "You hear a sloshing sound.\r\n");
        break;
    case SECT_BLOOD:
        send_to_char(vict, "You hear a slight squishing sound.\r\n");
        break;
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
        if (can_see_room(vict, vict->in_room) && !number(0, 1))
            send_to_char(vict,
                "You notice ripples forming on the surface.\r\n");
        else
            send_to_char(vict, "You hear a faint splashing.\r\n");
        break;
    case SECT_FIRE_RIVER:
        send_to_char(vict, "You hear a sizzling sound.\r\n");
        break;
    case SECT_FARMLAND:
        send_to_char(vict, "There is a rustling among the crops.\r\n");
        break;
    default:
        if (affected_by_spell(ch, SPELL_SKUNK_STENCH) ||
            affected_by_spell(ch, SPELL_TROG_STENCH))
            send_to_char(vict, "You notice a terrible odor.\r\n");
        else
            send_to_char(vict, "You think you hear something.\r\n");
        break;
    }
}

int
check_sneak(struct creature *ch, struct creature *vict, bool departing,
    bool msgs)
{
    int sneak_prob, sneak_roll;
    int idx;

    // No one sees invisible immortal or tester movements
    if (!can_see_creature(vict, ch) && (IS_IMMORT(ch) || is_tester(ch)))
        return SNEAK_OK;

    // No one can fool an immortal (except an invisible immortal)
    if (IS_IMMORT(vict))
        return SNEAK_FAILED;

    // Sonic imagery sees all other movements, though
    if (AFF3_FLAGGED(vict, AFF3_SONIC_IMAGERY))
        return SNEAK_FAILED;

    // People on fire are quite noticible
    if (AFF2_FLAGGED(ch, AFF2_ABLAZE))
        return SNEAK_FAILED;

    // If they're not sneaking, they are always seen.  If they're invisible,
    // they're always heard.
    if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
        if (!AWAKE(vict))
            return SNEAK_OK;

        if (!can_see_creature(vict, ch) && msgs) {
            get_giveaway(ch, vict);
            return SNEAK_HEARD;
        }

        return SNEAK_FAILED;
    }

    sneak_prob = skill_bonus(ch, SKILL_SNEAK);

    if (GET_CLASS(ch) == CLASS_RANGER &&
        ((ch->in_room->sector_type == SECT_FOREST) ||
            (ch->in_room->sector_type == SECT_HILLS) ||
            (ch->in_room->sector_type == SECT_MOUNTAIN) ||
            (ch->in_room->sector_type == SECT_ROCK) ||
            (ch->in_room->sector_type == SECT_TUNDRA) ||
            (ch->in_room->sector_type == SECT_TRAIL)))
        sneak_prob += GET_LEVEL(ch) / 2;
    if (IS_ELF(ch))
        sneak_prob += 10;
    sneak_prob -=
        ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 16) / CAN_CARRY_W(ch);

    for (idx = 0; idx < NUM_WEARS; idx++)
        if (ch->equipment[idx] && IS_METAL_TYPE(ch->equipment[idx]))
            sneak_prob -= GET_OBJ_WEIGHT(ch->equipment[idx]);

    sneak_roll = number(0, level_bonus(vict, true));
    if (affected_by_spell(vict, ZEN_AWARENESS))
        sneak_roll += level_bonus(vict, ZEN_AWARENESS) / 4;

    if (PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch, "%s[SNEAK] vict:%s   prob:%d   roll:%d%s\r\n",
            CCCYN(ch, C_NRM), GET_NAME(vict), sneak_prob, sneak_roll,
            CCNRM(ch, C_NRM));

    // Vampires have no chance of accidentally failing to sneak
    if (!IS_VAMPIRE(ch) || IS_VAMPIRE(vict)) {
        if (sneak_prob < sneak_roll) {
            gain_skill_prof(vict, SKILL_SNEAK);
            if (!AWAKE(vict) || !can_see_creature(vict, ch)) {
                get_giveaway(ch, vict);
                return SNEAK_HEARD;
            }

            return SNEAK_FAILED;
        }

        sneak_roll += CHECK_SKILL(vict, SKILL_HEARING) / 4;
        if (sneak_prob < sneak_roll) {
            gain_skill_prof(vict, SKILL_SNEAK);
            get_giveaway(ch, vict);
            return SNEAK_HEARD;
        }

    }

    if (AFF_FLAGGED(vict, AFF_SENSE_LIFE) && !IS_UNDEAD(ch) && msgs) {
        if (departing)
            send_to_char(vict, "You sense a departing lifeform.\r\n");
        else
            send_to_char(vict, "You sense that a lifeform has arrived.\r\n");
        return SNEAK_SENSED;
    }

    if (affected_by_spell(vict, SKILL_MOTION_SENSOR) &&
        CHECK_SKILL(vict, SKILL_MOTION_SENSOR) > number(20, 100) && msgs) {
        send_to_char(vict, "-NOTICE- Covert motion detected in vicinity.\r\n");
        gain_skill_prof(vict, SKILL_MOTION_SENSOR);
        return SNEAK_SENSED;
    }

    return SNEAK_OK;
}

/* do_simple_move assumes
 *        1. That there is no master and no followers.
 *        2. That the direction exists.
 *
 *   Returns :
 *   0 : If succes.
 *   1 : If fail
 *   2 : If critical failure / possible death
 */

int
do_simple_move(struct creature *ch, int dir, int mode, int need_specials_check)
{

    int need_movement, has_boat = 0, wait_state = 0;
    struct room_data *was_in;
    struct obj_data *obj = NULL, *next_obj = NULL, *car = NULL, *c_obj = NULL;
    struct creature *mount = MOUNTED_BY(ch);
    int found = 0;
    struct special_search_data *srch = NULL;
    struct affected_type *af_ptr = NULL;
    char *blur_msg = NULL;

    if (mount && ch->in_room != mount->in_room) {
        REMOVE_BIT(AFF2_FLAGS(mount), AFF2_MOUNTED);
        dismount(ch);
        mount = NULL;
    }

    /*
     * Check for special routines (North is 1 in command list, but 0 here) Note
     * -- only check if following; this avoids 'double spec-proc' bug
     */
    if (need_specials_check
        && special(ch, dir + 1, dir + 1, tmp_strdup(""), SPECIAL_CMD))
        return 2;

    /* afk?     */
    if (PLR_FLAGGED(ch, PLR_AFK)) {
        send_to_char(ch, "You are no longer afk.\r\n");
        REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
        free(AFK_REASON(ch));
        AFK_REASON(ch) = NULL;
    }

    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        ch->in_room == ch->master->in_room) {
        send_to_char(ch,
            "The thought of leaving your master makes you weep.\r\n");
        if (IS_UNDEAD(ch))
            act("$n makes a hollow moaning sound.", false, ch, NULL, NULL, TO_ROOM);
        else
            act("$n looks like $e wants to go somewhere.", false, ch, NULL, NULL,
                TO_ROOM);
        return 1;
    }

    /* petrified? */
    if (AFF2_FLAGGED(ch, AFF2_PETRIFIED)) {
        send_to_char(ch, "You stand as still as the statue you are.\r\n");
        return 1;
    }

    if ((af_ptr = affected_by_spell(ch, SPELL_ENTANGLE))) {
        if (!GET_MOVE(ch)) {
            send_to_char(ch,
                "You are too exhausted to break free from the entangling vegetation.\r\n");
            return 1;
        }
        if (ch->in_room->sector_type == SECT_CITY
            || ch->in_room->sector_type == SECT_CRACKED_ROAD) {
            send_to_char(ch,
                "You struggle against the entangling vegetation!\r\n");
            act("$n struggles against the entangling vegetation.", true, ch, NULL,
                NULL, TO_ROOM);
        } else {
            send_to_char(ch, "You struggle against the entangling vines!\r\n");
            act("$n struggles against the entangling vines.", true, ch, NULL, NULL,
                TO_ROOM);
        }
        GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 10);
        WAIT_STATE(ch, 2 RL_SEC);

        af_ptr->duration -= strength_damage_bonus(GET_STR(ch));
        if (af_ptr->duration <= 0) {
            send_to_char(ch, "You break free!\r\n");
            act("$n breaks free from the entanglement!", true, ch, NULL, NULL,
                TO_ROOM);
            while ((af_ptr = affected_by_spell(ch, SPELL_ENTANGLE)))
                affect_remove(ch, af_ptr);
        }
        return 1;
    }

    if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_HOUSE) &&
        !can_enter_house(ch, EXIT(ch, dir)->to_room->number)) {
        send_to_char(ch, "That's private property -- no trespassing!\r\n");
        return 1;
    }

    if (!is_authorized(ch, ENTER_ROOM, EXIT(ch, dir)->to_room)) {
        send_to_char(ch, "You cannot set foot in that Ultracosmic place.\r\n");
        return 1;
    }

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(ch) < LVL_SPIRIT &&
        ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DEATH)
        && IS_APPR(ch->in_room->zone)) {
        send_to_char(ch, "You are repelled by an invisible order-shield.\r\n");
        return 1;
    }

    /* if this room or the one we're going to needs a boat, check for one */
    if (EXIT(ch, dir)->to_room->sector_type == SECT_WATER_NOSWIM &&
        ch->in_room->sector_type != SECT_UNDERWATER) {
        if (GET_POSITION(ch) == POS_FLYING) {
            has_boat = true;
            send_to_char(ch, "You fly over the waters.\r\n");
        } else if (IS_FISH(ch) || AFF_FLAGGED(ch, AFF_WATERWALK))
            has_boat = true;
        else {
            for (obj = ch->carrying; obj; obj = obj->next_content)
                if (IS_OBJ_TYPE(obj, ITEM_BOAT)) {
                    has_boat = true;
                    break;
                }
        }
        if (!has_boat) {
            send_to_char(ch, "You need a boat to go there.\r\n");
            return 1;
        }
    }
    /* if this room or the one we're going to needs wings, check for one */
    if (room_is_open_air(ch->in_room)
        && GET_POSITION(ch) != POS_FLYING
        && (!MOUNTED_BY(ch) || GET_POSITION(MOUNTED_BY(ch)) != POS_FLYING)) {
        send_to_char(ch, "You scramble wildly for a grasp of thin air.\r\n");
        return 1;
    }
    if (room_is_open_air(EXIT(ch, dir)->to_room)
        && !NOGRAV_ZONE(ch->in_room->zone)
        && GET_POSITION(ch) != POS_FLYING
        && mode != MOVE_JUMP &&
        (!MOUNTED_BY(ch) || GET_POSITION(MOUNTED_BY(ch)) != POS_FLYING)) {
        send_to_char(ch, "You need to be flying to go there.\r\n");
        if (dir != UP)
            send_to_char(ch,
                "You can 'jump' in that direction however...\r\n");
        return 1;
    }

    if (dir == UP && mode == MOVE_JUMP
        && room_is_open_air(EXIT(ch, dir)->to_room)) {
        send_to_char(ch, "You jump up in the air and land on your feet.\r\n");
        act("$n jumps up into the air and lands on $s feet.\r\n",
            false, ch, NULL, NULL, TO_ROOM);
        return 1;
    }

    if (GET_POSITION(ch) == POS_FLYING && mode == MOVE_CRAWL) {
        send_to_char(ch,
            "Maybe you should return to the ground before crawling.\r\n");
        return 1;
    }

    if (ch->fighting && mode == MOVE_CRAWL) {
        send_to_char(ch, "No way!  You're fighting for your life!\r\n");
        return 1;
    }

    /* check room count */
    if (!will_fit_in_room(ch, EXIT(ch, dir)->to_room)) {
        send_to_char(ch, "It is too crowded there to enter.\r\n");
        return 1;
    }

    /*  if we are mounted and the exit is a doorway, don't let it happen. */
    if (mount && (GET_WEIGHT(mount) > 500 || GET_HEIGHT(mount) > 100) &&
        IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) &&
        IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_INDOORS)) {
        act("You cannot ride $N through there.", false, ch, NULL, mount, TO_CHAR);
        return 1;
    }

    // Attempt to keep unapproved NPCs in unapproved zones
    if (NPC2_FLAGGED(ch, NPC2_UNAPPROVED)
        && !ZONE_FLAGGED(EXIT(ch, dir)->to_room->zone, ZONE_MOBS_APPROVED)) {
        send_to_char(ch, "Sorry, you aren't allowed to play yet.\r\n");
        return 1;
    }

    need_movement = (movement_loss[ch->in_room->sector_type] +
        movement_loss[ch->in_room->dir_option[dir]->
            to_room->sector_type]) / 2;

    need_movement += (((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 2) /
        CAN_CARRY_W(ch));

    if (SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
        SECT_TYPE(EXIT(ch, dir)->to_room) == SECT_WATER_SWIM)
        need_movement +=
            (number(0, MAX(1, 101 - GET_SKILL(ch, SKILL_SWIMMING))) / 16);

    if (mount && mount->in_room == ch->in_room)
        need_movement /= 2;
    else if (IS_DWARF(ch))
        need_movement = (int)(need_movement * 1.5);
    if (mode == MOVE_CRAWL)
        need_movement = 0;
    if (affected_by_spell(ch, ZEN_MOTION))
        need_movement /= 2;
    if (affected_by_spell(ch, SKILL_SNEAK))
        need_movement += 1;
    if (affected_by_spell(ch, SKILL_ELUSION))
        need_movement += 1;
    if (IS_DROW(ch) &&
        OUTSIDE(ch) && PRIME_MATERIAL_ROOM(ch->in_room) &&
        ch->in_room->zone->weather->sunlight == SUN_LIGHT)
        need_movement += 2;

    if (GET_POSITION(ch) == POS_FLYING
        && SECT_TYPE(ch->in_room) != SECT_INSIDE
        && SECT_TYPE(EXIT(ch, dir)->to_room) != SECT_INSIDE
        && !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
        need_movement /= 2;

    if (GET_POSITION(ch) != POS_FLYING &&
        SECT_TYPE(ch->in_room) == SECT_MOUNTAIN && (dir == UP || dir == DOWN))
        need_movement *= 2;

    if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
        if (need_specials_check && ch->master)
            send_to_char(ch, "You are too exhausted to follow.\r\n");
        else
            send_to_char(ch, "You are too exhausted.\r\n");

        return 1;
    }
    if (mount && GET_MOVE(mount) < need_movement) {
        send_to_char(ch, "Your mount is too exhausted to continue.\r\n");
        return 1;
    }
    // At this point we're definately going to move.  Let's iterate though
    // the people in the room and make sure that no one is fighting us.
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (g_list_find(tch->fighting, ch))
            remove_combat(tch, ch);
    }

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_NPC(ch))
        GET_MOVE(ch) -= need_movement;
    if (mount)
        GET_MOVE(mount) -= need_movement;

    if (AFF_FLAGGED(ch, AFF_BLUR) || (mount && AFF_FLAGGED(mount, AFF_BLUR)))
        blur_msg = tmp_sprintf("A blurred, shifted image leaves %s",
            to_dirs[dir]);

    if (mode == MOVE_JUMP) {
        if (!number(0, 1))
            sprintf(buf, "$n jumps %sward.", dirs[dir]);
        else
            sprintf(buf, "$n takes a flying leap %sward.", dirs[dir]);
    } else if (mode == MOVE_FLEE) {
        sprintf(buf, "$n flees %sward.", dirs[dir]);
    } else if (mode == MOVE_RETREAT) {
        sprintf(buf, "$n retreats %sward.", dirs[dir]);
    } else if (mode == MOVE_CRAWL) {
        sprintf(buf, "$n crawls slowly %sward.", dirs[dir]);
    } else if (GET_POSITION(ch) == POS_FLYING || room_is_open_air(ch->in_room)
        || room_is_open_air(EXIT(ch, dir)->to_room)) {
        sprintf(buf, "$n flies %s.", to_dirs[dir]);
    } else if (ch->in_room->sector_type == SECT_ASTRAL) {
        sprintf(buf, "$n travels what seems to be %sward.", to_dirs[dir]);
    } else if (SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
        SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER ||
        SECT_TYPE(ch->in_room) == SECT_PITCH_PIT ||
        SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
        SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN) {
        if (((SECT_TYPE(ch->in_room) != SECT_UNDERWATER &&
                    SECT_TYPE(EXIT(ch, dir)->to_room) == SECT_UNDERWATER)
                || (SECT_TYPE(ch->in_room) != SECT_PITCH_SUB
                    && SECT_TYPE(EXIT(ch, dir)->to_room) == SECT_PITCH_SUB))
            && dir == DOWN)
            strcpy(buf, "$n disappears below the surface.");
        else if (IS_FISH(ch)) {
            sprintf(buf, "$n swims %s.", to_dirs[dir]);
        } else {
            if (AFF_FLAGGED(ch, AFF_WATERWALK) &&
                SECT_TYPE(ch->in_room) != SECT_UNDERWATER &&
                SECT_TYPE(ch->in_room) != SECT_DEEP_OCEAN &&
                SECT_TYPE(ch->in_room) != SECT_PITCH_SUB) {
                sprintf(buf, "$n walks %s across the %s.", to_dirs[dir],
                    SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER ? "flames" :
                    SECT_TYPE(ch->in_room) == SECT_PITCH_PIT ? "pitch" :
                    "water");
            } else if ((IS_DWARF(ch) ||
                    SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
                    SECT_TYPE(ch->in_room) == SECT_PITCH_PIT))
                sprintf(buf, "$n struggles %s.", to_dirs[dir]);
            else
                sprintf(buf, "$n swims %s.", to_dirs[dir]);
        }
    } else if (SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM) {
        if (AFF_FLAGGED(ch, AFF_WATERWALK) && !IS_FISH(ch))
            sprintf(buf, "$n walks %s across the water.", to_dirs[dir]);
        else
            sprintf(buf, "$n swims %s.", to_dirs[dir]);
    } else if (AFF2_FLAGGED(ch, AFF2_ABLAZE)) {
        sprintf(buf, "$n staggers %s, covered in flames.", to_dirs[dir]);
    } else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 2) {
        sprintf(buf, "$n staggers %s.", to_dirs[dir]);
    } else if (AFF_FLAGGED(ch, AFF_SNEAK)) {
        sprintf(buf, "$n sneaks off %sward.", dirs[dir]);
    } else if (IS_NPC(ch) && NPC_SHARED(ch)->move_buf) {
        sprintf(buf, "$n %s off %s.", NPC_SHARED(ch)->move_buf, to_dirs[dir]);
    } else if (IS_DRAGON(ch)) {
        sprintf(buf, "$n lumbers off %s.", to_dirs[dir]);
    } else if (IS_GIANT(ch)) {
        sprintf(buf, "$n stomps off %s.", to_dirs[dir]);
    } else if (IS_TABAXI(ch) && !number(0, 4)) {
        sprintf(buf, "$n slinks off %sward.", dirs[dir]);
    } else if (GET_CLASS(ch) == CLASS_SNAKE ||
        (IS_NPC(ch) && GET_NPC_VNUM(ch) == 3066)) {
        sprintf(buf, "$n slithers %s.", to_dirs[dir]);
    } else if ((IS_NPC(ch) && GET_NPC_VNUM(ch) == 3068) ||
        IS_LEMURE(ch) || IS_PUDDING(ch) || IS_SLIME(ch)) {
        sprintf(buf, "$n oozes %s.", to_dirs[dir]);
    } else if (IS_GHOST(ch)) {
        sprintf(buf, "$n drifts off %s.", to_dirs[dir]);
    } else if (GET_CLASS(ch) == CLASS_TURTLE) {
        sprintf(buf, "$n waddles %s.", to_dirs[dir]);
    } else if (IS_TARRASQUE(ch)) {
        sprintf(buf, "$n rampages off %sward.", to_dirs[dir]);
    } else {
        if (is_fighting(ch))
            sprintf(buf, "$n splits %s!", to_dirs[dir]);
        else if (GET_RACE(ch) != RACE_MOBILE && !IS_SPECTRE(ch) &&
            !IS_WRAITH(ch) && !IS_SHADOW(ch)) {
            if (SECT(ch->in_room) == SECT_ASTRAL ||
                (!number(0, 3) && (SECT(ch->in_room) == SECT_ROAD)))
                sprintf(buf, "$n travels %sward.", to_dirs[dir]);
            else if (!number(0, 3) && (SECT(ch->in_room) == SECT_CITY ||
                    SECT(ch->in_room) == SECT_INSIDE ||
                    SECT(ch->in_room) == SECT_ROAD))
                sprintf(buf, "$n strolls %s.", to_dirs[dir]);
            else if (!number(0, 2))
                sprintf(buf, "$n walks %s.", to_dirs[dir]);
            else if (!number(0, 2))
                sprintf(buf, "$n departs %sward.", dirs[dir]);
            else
                sprintf(buf, "$n leaves %s.", to_dirs[dir]);
        } else
            sprintf(buf, "$n leaves %s.", to_dirs[dir]);
    }

    if (mount)
        sprintf(buf + strlen(buf) - 1, ", carrying $N.");
    if (blur_msg) {
        if (mount && !AFF_FLAGGED(ch, AFF_BLUR))
            blur_msg = tmp_strcat(blur_msg, ", carrying $N.", NULL);
        else
            blur_msg = tmp_strcat(blur_msg, ".", NULL);
    }

    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch == ch || !AWAKE(tch))
            continue;
        if (check_sneak(ch, tch, true, true) == SNEAK_FAILED) {
            if (blur_msg && !PRF_FLAGGED(tch, PRF_HOLYLIGHT) &&
                !AFF2_FLAGGED(tch, AFF2_TRUE_SEEING))
                perform_act(blur_msg, ch, NULL, ch, tch, 0);
            else if (mount)
                perform_act(buf, mount, NULL, ch, tch, 0);
            else
                perform_act(buf, ch, NULL, NULL, tch, 0);
        }
    }

    for (car = ch->in_room->contents; car; car = car->next_content) {
        if (!IS_VEHICLE(car))
            continue;

        for (c_obj = object_list; c_obj; c_obj = c_obj->next) {
            if (((IS_OBJ_TYPE(c_obj, ITEM_V_DOOR) && !CAR_CLOSED(car)) ||
                    (IS_OBJ_TYPE(c_obj, ITEM_V_WINDOW)
                        && !CAR_CLOSED(c_obj)))
                && ROOM_NUMBER(c_obj) == ROOM_NUMBER(car)
                && GET_OBJ_VNUM(car) == V_CAR_VNUM(c_obj)
                && c_obj->in_room) {
                for (GList * it = first_living(c_obj->in_room->people); it; it = next_living(it)) {
                    struct creature *tch = it->data;
                    if (ch == tch)
                        continue;

                    if (check_sneak(ch, tch, true, false) == SNEAK_FAILED) {
                        if (blur_msg && !PRF_FLAGGED(tch, PRF_HOLYLIGHT) &&
                            !AFF2_FLAGGED(tch, AFF2_TRUE_SEEING))
                            perform_act(blur_msg, ch, NULL, ch, tch, 1);
                        else if (mount)
                            perform_act(buf, mount, NULL, ch, tch, 1);
                        else
                            perform_act(buf, ch, NULL, NULL, tch, 1);
                    }
                }
                break;
            }
        }
    }

    was_in = ch->in_room;
    GET_FALL_COUNT(ch) = 0;

    if (IS_SET(was_in->dir_option[dir]->exit_info, EX_ISDOOR) &&
        was_in->dir_option[dir]->keyword) {
        send_to_char(ch, "You %s through the %s %s.\r\n",
            (was_in->sector_type == SECT_UNDERWATER ||
                was_in->sector_type == SECT_DEEP_OCEAN ||
                (was_in->sector_type == SECT_WATER_NOSWIM &&
                    !AFF_FLAGGED(ch, AFF_WATERWALK)) ||
                was_in->sector_type == SECT_PITCH_SUB) ? "swim" :
            (room_is_open_air(was_in) ||
                GET_POSITION(ch) == POS_FLYING) ? "fly" :
            MOUNTED_BY(ch) ? "ride" : "pass",
            IS_SET(was_in->dir_option[dir]->exit_info, EX_CLOSED) ?
            "closed" : "open", fname(was_in->dir_option[dir]->keyword));
    }

    if ((ch->in_room->sector_type == SECT_ELEMENTAL_OOZE &&
            was_in->sector_type != SECT_ELEMENTAL_OOZE) ||
        (ch->in_room->sector_type == SECT_UNDERWATER &&
            was_in->sector_type != SECT_UNDERWATER) ||
        (ch->in_room->sector_type == SECT_PITCH_SUB &&
            was_in->sector_type != SECT_PITCH_SUB)) {
        send_to_char(ch, "You submerge in the %s.\r\n",
            ch->in_room->sector_type == SECT_UNDERWATER ? "water" :
            ch->in_room->sector_type == SECT_ELEMENTAL_OOZE ? "ooze" :
            "boiling pitch");
    }

    if (!char_from_room(ch, true))
        return 2;

    update_trail(ch, was_in, dir, TRAIL_EXIT);

    if (mode == MOVE_RETREAT)
        send_to_char(ch, "You beat a hasty retreat %s.\r\n", to_dirs[dir]);

    if (!char_to_room(ch, was_in->dir_option[dir]->to_room, true))
        return 2;

    update_trail(ch, ch->in_room, rev_dir[dir], TRAIL_ENTER);

    wait_state = 2 + (affected_by_spell(ch, SKILL_SNEAK) ? 1 : 0) +
        (affected_by_spell(ch, SKILL_ELUSION) ? 1 : 0);

    if (mode == MOVE_RETREAT)   // retreating takes a little longer
        wait_state += 10 + ((120 - (CHECK_SKILL(ch, SKILL_RETREAT))) / 5);

    if (mode == MOVE_CRAWL)
        wait_state += 3 RL_SEC;

    WAIT_STATE(ch, wait_state);

    if (!IS_NPC(ch) && ch->in_room->zone != was_in->zone)
        ch->in_room->zone->enter_count++;

    if (was_in->zone->pk_style != ch->in_room->zone->pk_style) {
        if (ch->in_room->zone->pk_style == ZONE_NEUTRAL_PK) {
            send_to_char(ch, "%s%sYou have just entered a "
                "neutral PK zone.%s\r\n", CCBLD(ch, C_CMP),
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        if (ch->in_room->zone->pk_style == ZONE_CHAOTIC_PK) {
            send_to_char(ch, "%s%sYou have just entered a "
                "chaotic PK zone.%s\r\n", CCBLD(ch, C_CMP),
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
    }
    if (mount) {
        char_from_room(mount, false);
        if (!char_to_room(mount, ch->in_room, false))
            return 2;
    }

    if (AFF_FLAGGED(ch, AFF_BLUR) || (mount && AFF_FLAGGED(mount, AFF_BLUR)))
        blur_msg = tmp_sprintf("A blurred, shifted image arrives from %s",
            from_dirs[dir]);

    if (mode == MOVE_JUMP) {
        if (!number(0, 1))
            sprintf(buf, "$n jumps in from %s.", from_dirs[dir]);
        else
            sprintf(buf, "$n leaps in from %s.", from_dirs[dir]);
    } else if (mode == MOVE_FLEE) {
        sprintf(buf, "$n runs in from %s.", from_dirs[dir]);
    } else if (mode == MOVE_DRAG) {
        sprintf(buf, "$n is dragged in from %s.", from_dirs[dir]);
    } else if (mode == MOVE_CRAWL) {
        sprintf(buf, "$n crawls slowly in from %s.", from_dirs[dir]);
    } else if (GET_POSITION(ch) == POS_FLYING || room_is_open_air(ch->in_room)) {
        if (!AFF2_FLAGGED(ch, AFF2_ABLAZE))
            sprintf(buf, "$n flies in from %s.", from_dirs[dir]);
        else
            sprintf(buf, "$n flies in from %s, covered in flames.",
                from_dirs[dir]);
    } else if (SECT_TYPE(ch->in_room) == SECT_ASTRAL) {
        if (!number(0, 1))
            sprintf(buf, "$n moves into view from what might be %s.",
                from_dirs[dir]);
        else
            sprintf(buf, "$n arrives from what appears to be %s.",
                from_dirs[dir]);
    } else if (AFF3_FLAGGED(ch, AFF3_HAMSTRUNG)
        && GET_POSITION(ch) == POS_STANDING) {
        sprintf(buf, "$n limps in from the %s, bleeding profusely.",
            from_dirs[dir]);
        add_blood_to_room(ch->in_room, 5);
    } else if (AFF2_FLAGGED(ch, AFF2_ABLAZE)) {
        sprintf(buf, "$n staggers in from %s, covered in flames.",
            from_dirs[dir]);
    } else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 2) {
        sprintf(buf, "$n staggers in from %s.", from_dirs[dir]);
    } else if (SECT_TYPE(ch->in_room) == SECT_WATER_SWIM
        || SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER
        || SECT_TYPE(ch->in_room) == SECT_PITCH_PIT
        || SECT_TYPE(ch->in_room) == SECT_PITCH_SUB
        || SECT_TYPE(ch->in_room) == SECT_UNDERWATER
        || SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN) {
        if (((SECT_TYPE(ch->in_room) != SECT_UNDERWATER
                    && SECT_TYPE(was_in) == SECT_UNDERWATER)
                || (SECT_TYPE(ch->in_room) != SECT_PITCH_SUB
                    && SECT_TYPE(was_in) == SECT_PITCH_SUB))
            && dir == UP)
            strcpy(buf, "$n appears from below the surface.");
        else if (AFF_FLAGGED(ch, AFF_WATERWALK) && !IS_FISH(ch) &&
            SECT_TYPE(ch->in_room) != SECT_PITCH_SUB &&
            SECT_TYPE(ch->in_room) != SECT_UNDERWATER)
            sprintf(buf, "$n walks across the %s from the %s.",
                SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER ? "flames" :
                SECT_TYPE(ch->in_room) == SECT_PITCH_PIT ? "pitch" :
                "water", to_dirs[dir]);
        else if ((IS_DWARF(ch) ||
                SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
                SECT_TYPE(ch->in_room) == SECT_PITCH_PIT) && !IS_FISH(ch))
            sprintf(buf, "$n struggles in from %s.", from_dirs[dir]);
        else
            sprintf(buf, "$n swims in from %s.", from_dirs[dir]);
    } else if (AFF_FLAGGED(ch, AFF_SNEAK)) {
        if (!number(0, 1))
            sprintf(buf, "$n sneaks in from %s.", from_dirs[dir]);
        else
            sprintf(buf, "$n edges in from %s.", from_dirs[dir]);
        /* most racial defaults now */
    } else if (IS_NPC(ch) && NPC_SHARED(ch)->move_buf) {
        sprintf(buf, "$n %s in from %s.",
            NPC_SHARED(ch)->move_buf, from_dirs[dir]);
    } else if (IS_DRAGON(ch)) {
        sprintf(buf, "$n lumbers in from %s.", from_dirs[dir]);
    } else if (IS_GIANT(ch)) {
        sprintf(buf, "$n stomps in from %s.", from_dirs[dir]);
    } else if (IS_TABAXI(ch) && !number(0, 4)) {
        sprintf(buf, "$n slinks in from %s.", from_dirs[dir]);
    } else if ((GET_CLASS(ch) == CLASS_SNAKE) ||
        (IS_NPC(ch) && GET_NPC_VNUM(ch) == 3066)) {
        sprintf(buf, "$n slithers in from %s.", from_dirs[dir]);
    } else if ((IS_NPC(ch) && GET_NPC_VNUM(ch) == 3068) ||
        IS_LEMURE(ch) || IS_PUDDING(ch) || IS_SLIME(ch)) {
        sprintf(buf, "$n oozes in from %s.", from_dirs[dir]);
    } else if (IS_GHOST(ch)) {
        sprintf(buf, "$n drifts in from %s.", from_dirs[dir]);
    } else if (GET_CLASS(ch) == CLASS_TURTLE) {
        sprintf(buf, "$n waddles in from %s.", to_dirs[dir]);
    } else if (IS_TARRASQUE(ch)) {
        sprintf(buf, "$n rampages in from %s.", from_dirs[dir]);
    } else {
        if (GET_RACE(ch) != RACE_MOBILE && !IS_SPECTRE(ch) &&
            !IS_WRAITH(ch) && !IS_SHADOW(ch)) {
            if (SECT(ch->in_room) == SECT_ASTRAL ||
                (!number(0, 2) && SECT_TYPE(ch->in_room) == SECT_ROAD))
                sprintf(buf, "$n travels in from %s.", from_dirs[dir]);
            else if (!number(0, 3))
                sprintf(buf, "$n walks in from %s.", from_dirs[dir]);
            else if (!number(0, 2) && (SECT(ch->in_room) == SECT_CITY ||
                    SECT(ch->in_room) == SECT_INSIDE ||
                    SECT(ch->in_room) == SECT_ROAD))
                sprintf(buf, "$n strolls in from %s.", from_dirs[dir]);
            else
                sprintf(buf, "$n has arrived from %s.", from_dirs[dir]);
        } else
            sprintf(buf, "$n has arrived from %s.", from_dirs[dir]);
    }

    if (mount)
        sprintf(buf + strlen(buf) - 1, ", carrying $N.");
    if (blur_msg) {
        if (mount && !AFF_FLAGGED(ch, AFF_BLUR))
            blur_msg = tmp_strcat(blur_msg, ", carrying $N.", NULL);
        else
            blur_msg = tmp_strcat(blur_msg, ".", NULL);
    }

    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch == ch)
            continue;

        if (check_sneak(ch, tch, false, true) == SNEAK_FAILED) {
            if (blur_msg && !PRF_FLAGGED(tch, PRF_HOLYLIGHT) &&
                !AFF2_FLAGGED(tch, AFF2_TRUE_SEEING))
                perform_act(blur_msg, ch, NULL, ch, tch, 0);
            else if (mount)
                perform_act(buf, mount, NULL, ch, tch, 0);
            else
                perform_act(buf, ch, NULL, NULL, tch, 0);

            if (affected_by_spell(ch, SPELL_QUAD_DAMAGE))
                act("...$e is glowing with a bright blue light!",
                    true, ch, NULL, tch, TO_VICT);
        }
    }

    for (car = ch->in_room->contents; car; car = car->next_content) {
        if (!IS_VEHICLE(car))
            continue;

        for (c_obj = object_list; c_obj; c_obj = c_obj->next) {
            if (((IS_OBJ_TYPE(c_obj, ITEM_V_DOOR) && !CAR_CLOSED(car))
                    || (IS_OBJ_TYPE(c_obj, ITEM_V_WINDOW)
                        && !CAR_CLOSED(c_obj)))
                && ROOM_NUMBER(c_obj) == ROOM_NUMBER(car)
                && GET_OBJ_VNUM(car) == V_CAR_VNUM(c_obj)
                && c_obj->in_room) {
                for (GList * it = first_living(c_obj->in_room->people); it; it = next_living(it)) {
                    struct creature *tch = it->data;
                    if (ch == tch)
                        continue;

                    if (check_sneak(ch, tch, false, false) == SNEAK_FAILED) {
                        if (blur_msg && !PRF_FLAGGED(tch, PRF_HOLYLIGHT) &&
                            !AFF2_FLAGGED(tch, AFF2_TRUE_SEEING))
                            perform_act(blur_msg, ch, NULL, ch, tch, 1);
                        else if (mount)
                            perform_act(buf, mount, NULL, ch, tch, 1);
                        else
                            perform_act(buf, ch, NULL, NULL, tch, 1);
                    }
                }
                break;
            }
        }
    }

    if (ch->desc)
        look_at_room(ch, ch->in_room, 0);

    struct creature *damager = NULL;
    struct affected_type *af = affected_by_spell(ch, SKILL_HAMSTRING);
    if (af && GET_POSITION(ch) == POS_STANDING) {
        damager = get_char_in_world_by_idnum(af->owner);
        if (!damager)
            damager = ch;
        if (!(mode == MOVE_FLEE) || random_fractional_3())
            if (damage(damager, ch, NULL, dice(5, 9), TYPE_BLEED, 0))
                return 2;
    }

    if (room_is_underwater(ch->in_room)) {
        if (room_is_underwater(was_in)) {
            BREATH_COUNT_OF(ch) += 1;
        } else {
            send_to_char(ch, "You are now submerged in water.\r\n");
            BREATH_COUNT_OF(ch) = 0;
        }
    }
    if (ch->in_room->sector_type == SECT_WATER_NOSWIM) {
        if (was_in->sector_type != SECT_UNDERWATER &&
            was_in->sector_type != SECT_WATER_NOSWIM) {
            BREATH_COUNT_OF(ch) = 0;
        } else {
            BREATH_COUNT_OF(ch) += 2;
        }
    }
    //
    // Blood application
    //

    if (ch->in_room->sector_type == SECT_BLOOD) {
        if (was_in->sector_type != SECT_BLOOD
            && (GET_POSITION(ch)) != (POS_FLYING)) {
            send_to_char(ch,
                "Your feet sink slightly into the blood soaked ground.\r\n");
            apply_soil_to_char(ch, GET_EQ(ch, WEAR_FEET), SOIL_BLOOD,
                WEAR_FEET);
            apply_soil_to_char(ch, GET_EQ(ch, WEAR_LEGS), SOIL_BLOOD,
                WEAR_LEGS);
        }

        if (GET_POSITION(ch) != POS_FLYING) {
            apply_soil_to_char(ch, NULL, SOIL_BLOOD, WEAR_FEET);
            apply_soil_to_char(ch, NULL, SOIL_BLOOD, WEAR_LEGS);
        }
    }
    //
    // Mud application
    //

    if (ch->in_room->sector_type == SECT_SWAMP ||
        ch->in_room->sector_type == SECT_MUDDY) {
        if (was_in->sector_type != SECT_SWAMP
            && was_in->sector_type != SECT_MUDDY
            && (GET_POSITION(ch)) != (POS_FLYING)) {
            send_to_char(ch,
                "Your feet sink slightly into the muddy ground.\r\n");
            apply_soil_to_char(ch, GET_EQ(ch, WEAR_FEET), SOIL_MUD, WEAR_FEET);
            apply_soil_to_char(ch, GET_EQ(ch, WEAR_LEGS), SOIL_MUD, WEAR_LEGS);
        }

        if (GET_POSITION(ch) != POS_FLYING) {
            apply_soil_to_char(ch, NULL, SOIL_MUD, WEAR_FEET);
            apply_soil_to_char(ch, NULL, SOIL_MUD, WEAR_LEGS);
        }
    }

    if ((ch->in_room->sector_type == SECT_FARMLAND
            && ch->in_room->zone->weather->sky == SKY_RAINING)
        || (ch->in_room->sector_type == SECT_JUNGLE
            && ch->in_room->zone->weather->sky == SKY_RAINING)
        || (ch->in_room->sector_type == SECT_FIELD
            && ch->in_room->zone->weather->sky == SKY_RAINING)
        || (ch->in_room->sector_type == SECT_FARMLAND
            && ch->in_room->zone->weather->sky == SKY_LIGHTNING)
        || (ch->in_room->sector_type == SECT_JUNGLE
            && ch->in_room->zone->weather->sky == SKY_LIGHTNING)
        || (ch->in_room->sector_type == SECT_FIELD
            && ch->in_room->zone->weather->sky == SKY_LIGHTNING)) {

        if (GET_POSITION(ch) != POS_FLYING) {
            apply_soil_to_char(ch, NULL, SOIL_MUD, WEAR_FEET);
            apply_soil_to_char(ch, NULL, SOIL_MUD, WEAR_LEGS);
        }

    }
    //
    // Light -> Darkness Emit
    //
    if (!can_see_room(ch, ch->in_room) && can_see_room(ch, was_in)) {
        send_to_char(ch, "You are enveloped by darkness.\r\n");
    }

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
        if (GET_OBJ_VNUM(obj) == BLOOD_VNUM)
            break;

    if (obj) {                  /* blood */

        if (ch->in_room->sector_type == SECT_UNDERWATER ||
            ch->in_room->sector_type == SECT_DEEP_OCEAN ||
            ch->in_room->sector_type == SECT_WATER_NOSWIM ||
            ch->in_room->sector_type == SECT_WATER_SWIM) {

        } else if (room_is_open_air(ch->in_room)) {
            if (GET_DEX(ch) + number(0, 10) < GET_OBJ_TIMER(obj)) {
                if (apply_soil_to_char(ch, NULL, SOIL_BLOOD, WEAR_RANDOM))
                    send_to_char(ch, "You fly through the mist of blood.\r\n");
            }
        } else if (GET_POSITION(ch) < POS_FLYING &&
            GET_DEX(ch) + number(0, 10) < GET_OBJ_TIMER(obj)) {
            if (apply_soil_to_char(ch, GET_EQ(ch, WEAR_FEET), SOIL_BLOOD,
                    WEAR_FEET)) {
                sprintf(buf,
                    "You walk through the blood and get it all over your %s.\r\n",
                    GET_EQ(ch, WEAR_FEET) ? OBJS(GET_EQ(ch, WEAR_FEET),
                        ch) : "feet");
                send_to_char(ch, "%s", buf);
            }
        }
    }
    //
    // death trap
    //

    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {
        was_in = ch->in_room;
        log_death_trap(ch);
        death_cry(ch);
        // extract it, leaving it's eq and such in the dt.
        creature_die(ch);
        if (was_in->number == 34004) {
            for (obj = was_in->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                damage_eq(NULL, obj, dice(10, 100), -1);
            }
        }
        return 2;
    }
    //
    // trig_enter searches
    //

    for (srch = ch->in_room->search, found = 0; srch; srch = srch->next) {
        if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER) && SRCH_OK(ch, srch)) {
            if (ch->desc != NULL && STATE(ch->desc) != CXN_AFTERLIFE)
                found = general_search(ch, srch, found);
            else
                return 22;
        }
    }

    if (found == 2)
        return 2;

    if (IS_NPC(ch))
        ch->mob_specials.last_direction = dir;

    return 0;
}

/*
 *
 *   Returns :
 *   0 : If succes.
 *   1 : If fail
 *   2 : If critical failure / possible death
 * same as do_simple_move
 */
int
perform_move(struct creature *ch, int dir, int mode, int need_specials_check)
{
    struct room_data *was_in;
    struct follow_type *k;

    if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || ch->in_room == NULL)
        return 1;

    if (mode != MOVE_CRAWL &&
        ((AFF2_FLAGGED(ch, AFF2_VERTIGO) &&
                GET_LEVEL(ch) < LVL_AMBASSADOR &&
                number(0, 60) > (GET_LEVEL(ch) + GET_DEX(ch)) &&
                GET_POSITION(ch) >= POS_FIGHTING)
            ||
            (GET_COND(ch, DRUNK) > GET_CON(ch) &&
                (number(0, GET_COND(ch, DRUNK)) > GET_DEX(ch))))) {
        if (GET_POSITION(ch) == POS_MOUNTED && MOUNTED_BY(ch) &&
            CHECK_SKILL(ch, SKILL_RIDING) < number(50, 150)) {
            act("$n sways and falls from the back of $N!",
                true, ch, NULL, MOUNTED_BY(ch), TO_ROOM);
            act("You sway and fall from the back of $N!",
                false, ch, NULL, MOUNTED_BY(ch), TO_CHAR);
            REMOVE_BIT(AFF2_FLAGS(MOUNTED_BY(ch)), AFF2_MOUNTED);
            dismount(ch);
            GET_POSITION(ch) = POS_SITTING;
        } else if (GET_POSITION(ch) == POS_FLYING) {
            act("You lose control and begin to fall!", false, ch, NULL, NULL,
                TO_CHAR);
            act("$n loses control and begins to fall!", true, ch, NULL, NULL,
                TO_ROOM);
            GET_POSITION(ch) = POS_SITTING;
        } else {
            switch (number(0, 4)) {
            case 0:
                act("You stumble and fall down.", true, ch, NULL, NULL, TO_CHAR);
                act("$n stumbles and falls down.", true, ch, NULL, NULL, TO_ROOM);
                break;
            case 1:
                act("You become dizzy and topple over.", true, ch, NULL, NULL,
                    TO_CHAR);
                act("$n suddenly topples over.", true, ch, NULL, NULL, TO_ROOM);
                break;
            case 2:
                act("Your head begins to spin and you fall down.", true, ch, NULL,
                    NULL, TO_CHAR);
                act("$n staggers around and falls over.", true, ch, NULL, NULL,
                    TO_ROOM);
                break;
            case 3:
                act("A sudden rush of vertigo overcomes you, and you fall down.", true, ch, NULL, NULL, TO_CHAR);
                act("$n whirls around and hits the ground!", true, ch, NULL, NULL,
                    TO_ROOM);
                break;
            default:
                act("You lose your balance and fall over.", true, ch, NULL, NULL,
                    TO_CHAR);
                act("$n loses $s balance and falls down.", true, ch, NULL, NULL,
                    TO_ROOM);
            }
            GET_POSITION(ch) = POS_SITTING;
        }
        return 1;
    }

    if ((GET_COND(ch, DRUNK) > GET_CON(ch))
        && (number(0, GET_COND(ch, DRUNK)) > GET_WIS(ch))) {
        act("You become disoriented!", true, ch, NULL, NULL, TO_CHAR);
        dir = number(0, 7);
    }

    if (!EXIT(ch, dir)
        || EXIT(ch, dir)->to_room == NULL
        || ((IS_SET(EXIT(ch, dir)->exit_info, EX_NOPASS)
                || (IS_SET(EXIT(ch, dir)->exit_info, EX_SECRET | EX_HIDDEN)
                    && IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)))
            && GET_LEVEL(ch) < LVL_AMBASSADOR && !NON_CORPOREAL_MOB(ch))) {
        switch (number(0, 5)) {
        case 0:
            send_to_char(ch, "Alas, you cannot go that way...\r\n");
            break;
        case 1:
            send_to_char(ch, "You don't seem to be able to go that way.\r\n");
            break;
        case 2:
            send_to_char(ch, "Sorry, you can't go in that direction!\r\n");
            break;
        case 3:
            send_to_char(ch, "There is no way to move in that direction.\r\n");
            break;
        case 4:
            send_to_char(ch, "You can't go that way, slick...\r\n");
            break;
        default:
            send_to_char(ch, "You'll have to choose another direction.\r\n");
            break;
        }
    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) &&
        !NON_CORPOREAL_MOB(ch) &&
        (GET_LEVEL(ch) < LVL_AMBASSADOR || PLR_FLAGGED(ch, PLR_MORTALIZED))) {
        if (EXIT(ch, dir)->keyword) {
            send_to_char(ch, "The %s seem%s to be closed.\r\n",
                fname(EXIT(ch, dir)->keyword),
                EXIT(ch, dir)->keyword[strlen(fname(EXIT(ch,
                                dir)->keyword)) - 1]
                == 's' ? "" : "s");

        } else
            send_to_char(ch, "It seems to be closed.\r\n");
    } else {
        if (!ch->followers)
            return (do_simple_move(ch, dir, mode, need_specials_check));

        was_in = ch->in_room;

        // This insanity is because we want followers to follow the
        // leader after the leader goes, but we want to know if they
        // saw the leader leave before he actually leaves.  We're just
        // iterating twice over the list, storing the visibility into
        // a FIFO of booleans.  It may seem like overkill, but this
        // caused some player confusion.
        GQueue *visibility = NULL;

        struct leader_visibility {
            struct creature *follower;
            bool visible;
        };
        struct leader_visibility *vis = NULL;

        visibility = g_queue_new();

        for (k = ch->followers; k; k = k->next) {
            vis = malloc(sizeof(struct leader_visibility));
            vis->follower = k->follower;
            vis->visible = can_see_creature(k->follower, ch);

            g_queue_push_tail(visibility, vis);
        }

        int retval = 0;
        if ((retval = do_simple_move(ch, dir, mode, need_specials_check)) != 0) {
            while ((vis = g_queue_pop_head(visibility)) != NULL)
                free(vis);
            g_queue_free(visibility);
            return retval;
        }

        while ((vis = g_queue_pop_head(visibility)) != NULL) {
            if ((was_in == vis->follower->in_room) &&
                !PLR_FLAGGED(vis->follower, PLR_WRITING)
                && (GET_POSITION(vis->follower) >= POS_STANDING)
                && vis->visible) {
                const char *msg = "You follow $N.\r\n";
                // These conditions match those in check_sight_room() in sight.cc
                if (room_is_dark(ch->in_room) && !has_dark_sight(vis->follower))
                    msg = tmp_sprintf("You follow %s into darkness.\r\n",
                        GET_DISGUISED_NAME(vis->follower, ch));
                else if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED)
                    && !AFF3_FLAGGED(vis->follower, AFF3_SONIC_IMAGERY))
                    msg = tmp_sprintf("You follow %s into dense smoke.\r\n",
                        GET_DISGUISED_NAME(vis->follower, ch));

                act(msg, false, vis->follower, NULL, ch, TO_CHAR);
                perform_move(vis->follower, dir, MOVE_NORM, 1);
            }
            free(vis);

        }
        // We should have used up all the visibility items
        assert(g_queue_is_empty(visibility));
        g_queue_free(visibility);
        return 0;
    }

    return 1;
}

ACMD(do_move)
{
    /*
     * This is basically a mapping of cmd numbers to perform_move indices.
     * It cannot be done in perform_move because perform_move is called
     * by other functions which do not require the remapping.
     */
    int dir = -1;
    char arg1[MAX_INPUT_LENGTH];

    if (subcmd == SCMD_MOVE || subcmd == SCMD_JUMP || subcmd == SCMD_CRAWL) {
        argument = one_argument(argument, arg1);
        if (!*arg1) {
            if (subcmd == SCMD_MOVE)
                do_gen_points(ch, arg1, cmd, subcmd);
            else
                send_to_char(ch, "In which direction?\r\n");
            return;
        }
        if ((dir = search_block(arg1, dirs, false)) < 0) {
            send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
            return;
        }
        if (subcmd == SCMD_MOVE)
            perform_move(ch, dir, MOVE_NORM, 1);
        else if (subcmd == SCMD_JUMP)
            perform_move(ch, dir, MOVE_JUMP, 1);
        else if (subcmd == SCMD_CRAWL)
            perform_move(ch, dir, MOVE_CRAWL, 1);
        else {
            send_to_char(ch, "This motion is not implenented.\r\n");
            return;
        }
    }
    perform_move(ch, cmd - 1, MOVE_NORM, 0);
}

int
find_door(struct creature *ch, char *type, char *dir, const char *cmdname)
{
    int door;

    if (PLR_FLAGGED(ch, PLR_AFK)) {
        REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
    }
    if (*dir) {                 /* a direction was specified */
        if ((door = search_block(dir, dirs, false)) == -1) {    // Partial Match
            send_to_char(ch, "That's not a direction.\r\n");
            return -1;
        }
        if (EXIT(ch, door))
            if (EXIT(ch, door)->keyword)
                if (isname(type, EXIT(ch, door)->keyword))
                    return door;
                else {
                    send_to_char(ch, "I see no %s there.\r\n", type);
                    return -1;
            } else
                return door;
        else {
            send_to_char(ch, "I really don't see how you can %s anything there.\r\n",
                         cmdname);
            return -1;
        }
    } else {                    /* try to locate the keyword */
        if (!*type) {
            send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
            return -1;
        }
        for (door = 0; door < NUM_OF_DIRS; door++)
            if (EXIT(ch, door))
                if (EXIT(ch, door)->keyword)
                    if (isname(type, EXIT(ch, door)->keyword))
                        return door;

        send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
        return -1;
    }
}

int
has_key(struct creature *ch, int key)
{
    struct obj_data *o;

    for (o = ch->carrying; o; o = o->next_content)
        if (GET_OBJ_VNUM(o) == key && !IS_OBJ_STAT2(o, ITEM2_BROKEN))
            return 1;

    if (GET_EQ(ch, WEAR_HOLD))
        if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key
            && !IS_OBJ_STAT2(GET_EQ(ch, WEAR_HOLD), ITEM2_BROKEN))
            return 1;

    return 0;
}

#define NEED_OPEN        1
#define NEED_CLOSED        2
#define NEED_UNLOCKED        4
#define NEED_LOCKED        8

const char *cmd_door[] = {
    "open",
    "close",
    "unlock",
    "lock",
    "pick",
    "hack",
};

const int flags_door[] = {
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_OPEN,
    NEED_CLOSED | NEED_LOCKED,
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_CLOSED | NEED_LOCKED,
    NEED_CLOSED | NEED_LOCKED
};

#define EXITN(room, door)                (room->dir_option[door])

void
OPEN_DOOR(struct room_data *room, struct obj_data *obj, int direction)
{
    if (obj != NULL) {
        TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
    } else {
        TOGGLE_BIT(EXITN(room, direction)->exit_info, EX_CLOSED);
    }
}

void
LOCK_DOOR(struct room_data *room, struct obj_data *obj, int direction)
{
    if (obj != NULL) {
        TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
    } else {
        TOGGLE_BIT(EXITN(room, direction)->exit_info, EX_LOCKED);
    }
}

void
do_doorcmd(struct creature *ch, struct obj_data *obj, int door, int scmd)
{
    struct room_data *other_room = NULL;
    struct room_direction_data *back = NULL;
    int wait_state = 0;

    sprintf(buf, "$n %ss ", cmd_door[scmd]);
    if (!obj && ((other_room = EXIT(ch, door)->to_room) != NULL))
        if ((back = other_room->dir_option[rev_dir[door]]))
            if (back->to_room != ch->in_room)
                back = NULL;

    switch (scmd) {
    case SCMD_OPEN:
        OPEN_DOOR(ch->in_room, obj, door);
        if (back)
            OPEN_DOOR(other_room, obj, (int)rev_dir[door]);
        if (obj && IS_OBJ_TYPE(obj, ITEM_SCUBA_MASK))
            act("You open the air valve on $p.", false, ch, obj, NULL, TO_CHAR);
        else
            send_to_char(ch, "Okay, opened.\r\n");

        if (obj)
            wait_state = 7;     // 0.7 sec
        else if (door_is_heavy(ch, obj, door))
            wait_state = 30;    // 3 sec
        else
            wait_state = 15;    // 1.5 sec

        break;
    case SCMD_CLOSE:
        OPEN_DOOR(ch->in_room, obj, door);
        if (back)
            OPEN_DOOR(other_room, obj, rev_dir[door]);
        if (obj && IS_OBJ_TYPE(obj, ITEM_SCUBA_MASK))
            act("You close the air valve on $p.", false, ch, obj, NULL, TO_CHAR);
        else
            send_to_char(ch, "Okay, closed.\r\n");

        if (obj)
            wait_state = 7;     // 0.7 sec
        else if (door_is_heavy(ch, obj, door))
            wait_state = 30;    // 3 sec
        else
            wait_state = 15;    // 1.5 sec

        break;
    case SCMD_UNLOCK:
    case SCMD_LOCK:
        LOCK_DOOR(ch->in_room, obj, door);
        if (back)
            LOCK_DOOR(other_room, obj, rev_dir[door]);
        send_to_char(ch, "*Click*\r\n");
        wait_state = 20;        // 2 sec
        break;
    case SCMD_PICK:
        LOCK_DOOR(ch->in_room, obj, door);
        if (back)
            LOCK_DOOR(other_room, obj, rev_dir[door]);
        send_to_char(ch, "The lock quickly yields to your skills.\r\n");
        strcpy(buf, "$n skillfully picks the lock on ");
        wait_state = 30;        // 3 sec
        break;
    case SCMD_HACK:
        LOCK_DOOR(ch->in_room, obj, door);
        if (back)
            LOCK_DOOR(other_room, obj, rev_dir[door]);
        send_to_char(ch, "The system quickly yields to your skills.\r\n");
        strcpy(buf, "$n skillfully hacks access to ");
        wait_state = 30;        // 3 sec
        break;

    }

    if (wait_state)
        WAIT_STATE(ch, wait_state);

    /* If we're opening a car, notify the interior */
    if (obj && IS_VEHICLE(obj)) {
        if ((other_room = real_room(ROOM_NUMBER(obj))) && other_room->people) {
            sprintf(buf2, "The door of the %s is %sed from the outside.\r\n",
                fname(obj->aliases), cmd_door[scmd]);
            send_to_room(buf2, other_room);
        }
        return;
    }

    /* Notify the room */
    if (obj)
        strcpy(buf + strlen(buf), "$p.");
    else if (EXIT(ch, door)->keyword)
        sprintf(buf + strlen(buf), "the %s.", fname(EXIT(ch, door)->keyword));
    else
        strcpy(buf + strlen(buf), "the door.");

    if (!(obj) || (obj->in_room != NULL))
        act(buf, false, ch, obj, NULL, TO_ROOM);

    // set house crash save bit
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
        SET_BIT(ROOM_FLAGS(ch->in_room), ROOM_HOUSE_CRASH);

    /* Notify the other room */
    if (((scmd == SCMD_OPEN) || (scmd == SCMD_CLOSE)) && (back)) {
        sprintf(buf, "The %s is %s%s from the other side.\r\n",
            (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
            (scmd == SCMD_CLOSE) ? "d" : "ed");
        send_to_room(buf, EXIT(ch, door)->to_room);

        // set house crash save bit
        if (ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_HOUSE))
            SET_BIT(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_HOUSE_CRASH);

    }

}

int
ok_pick(struct creature *ch, int keynum, int pickproof, int tech, int scmd)
{
    int percent, mod;
    struct obj_data *tool = NULL;

    percent = number(2, 101);

    if (scmd == SCMD_PICK) {
        if (tech)
            send_to_char(ch,
                "This exit is not pickable by traditional means.\r\n");
        else if (CHECK_SKILL(ch, SKILL_PICK_LOCK) <= 0)
            send_to_char(ch, "You don't know how.\r\n");
        else if (keynum < 0)
            send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
        else if (pickproof)
            send_to_char(ch, "It resists your attempts at picking it.\r\n");
        else {
            if ((!(tool = GET_EQ(ch, WEAR_HOLD)) &&
                    !(tool = GET_IMPLANT(ch, WEAR_HOLD))) ||
                !IS_TOOL(tool) || TOOL_SKILL(tool) != SKILL_PICK_LOCK) {
                if (PRF_FLAGGED(ch, PRF_NOHASSLE) || IS_NPC(ch))
                    mod = 15;
                else {
                    send_to_char(ch,
                        "You need to be holding a lockpicking tool, you deviant!\r\n");
                    return 0;
                }
            } else
                mod = TOOL_MOD(tool);

            if (percent > (CHECK_SKILL(ch, SKILL_PICK_LOCK) + mod)) {
                send_to_char(ch, "You failed to pick the lock.\r\n");
                WAIT_STATE(ch, 4);
            } else
                return 1;
        }
        return (0);

    } else if (scmd == SCMD_HACK) {
        if (!tech)
            send_to_char(ch, "You can't hack that.\r\n");
        else if (pickproof)
            send_to_char(ch, "This system seems to be uncrackable.\r\n");
        else if (percent > CHECK_SKILL(ch, SKILL_HACKING)) {
            send_to_char(ch, "You fail to crack the system.\r\n");
            WAIT_STATE(ch, 4);
        } else
            return (1);
        return (0);
    }
    return (1);
}

ACMD(do_gen_door)
{
    int door = 0, keynum;
    char *type, *dir;
    char dname[128];
    struct obj_data *obj = NULL;
    struct creature *victim = NULL;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char(ch, "%s what?\r\n", cmd_door[subcmd]);
        return;
    }

    type = tmp_getword(&argument);
    dir = tmp_getword(&argument);

    if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_IGNORE_WIERD,
            ch, &victim, &obj))
        door = find_door(ch, type, dir, cmd_door[subcmd]);

    if ((obj) || (door >= 0)) {
        keynum = door_key(ch, obj, door);
        if (!door_is_openable(ch, obj, door))
            act(tmp_sprintf("You can't %s that!", cmd_door[subcmd]),
                false, ch, NULL, NULL, TO_CHAR);
        else if (!door_is_open(ch, obj, door) &&
                 IS_SET(flags_door[subcmd], NEED_OPEN)) {
            send_to_char(ch, "But it's already closed!\r\n");
        } else if (door_is_open(ch, obj, door) &&
                   IS_SET(flags_door[subcmd], NEED_CLOSED)) {
            send_to_char(ch, "But it's currently open!\r\n");
        } else if (IS_SET(flags_door[subcmd], NEED_CLOSED | NEED_OPEN) &&
                   door_is_special(ch, obj, door)) {
            act(tmp_sprintf("You can't %s that from here.", cmd_door[subcmd]),
                false, ch, NULL, NULL, TO_CHAR);
        } else if (!(door_is_locked(ch, obj, door))
                   && IS_SET(flags_door[subcmd], NEED_LOCKED))
            send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
        else if (!(door_is_unlocked(ch, obj, door)) &&
            IS_SET(flags_door[subcmd], NEED_UNLOCKED))
            send_to_char(ch, "It seems to be locked.\r\n");
        else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
                 ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
            send_to_char(ch, "You don't seem to have the proper key.\r\n");
        else if (door >= 0 && door_is_heavy(ch, obj, door) &&
                 (subcmd == SCMD_OPEN) &&
                 (IS_SET(flags_door[subcmd], NEED_CLOSED)) &&
                 ((dice(2, 7) + strength_damage_bonus(GET_STR(ch))) < 12)) {
            if (EXIT(ch, door)->keyword)
                strcpy(dname, fname(EXIT(ch, door)->keyword));
            else
                strcpy(dname, "door");

            if (GET_MOVE(ch) < 10) {
                send_to_char(ch, "You are too exhausted.\r\n");
                return;
            }
            switch (number(0, 3)) {
            case 0:
                send_to_char(ch, "The %s %s too heavy for you to open!\r\n",
                    dname, ISARE(dname));
                sprintf(buf, "$n attempts to open the %s.", dname);
                break;
            case 1:
                send_to_char(ch,
                    "You push against the %s, but %s barely budge%s.\r\n",
                    dname, IT_THEY(dname), PLUR(dname) ? "" : "s");
                sprintf(buf,
                    "$n pushes against the %s, but %s barely budge%s.", dname,
                    IT_THEY(dname), PLUR(dname) ? "" : "s");
                break;
            case 2:
                send_to_char(ch, "You strain against the %s, to no avail.\r\n",
                    dname);
                sprintf(buf, "$n strains against the %s, to no avail.", dname);
                break;
            default:
                send_to_char(ch,
                    "You throw yourself against the heavy %s in an attempt to open %s.\r\n",
                    dname, IT_THEM(dname));
                sprintf(buf,
                    "$n throws $mself against the heavy %s in an attempt to open %s.\r\n",
                    dname, IT_THEM(dname));
                break;
            }
            WAIT_STATE(ch, PULSE_VIOLENCE);
            GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 10);
            act(buf, false, ch, NULL, NULL, TO_ROOM);
        } else if (ok_pick(ch,
                           keynum,
                           door_is_pickproof(ch, obj, door),
                           door_is_technical(ch, obj, door), subcmd))
            do_doorcmd(ch, obj, door, subcmd);
    }
    return;
}

ACMD(do_enter)
{
    int door;
    struct obj_data *car = NULL;
    struct room_data *room = NULL, *was_in = NULL;
    struct follow_type *k, *next;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg) {
        // Enter without argument
        if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
            send_to_char(ch, "You are already indoors.\r\n");
        else {
            // try to locate an entrance
            for (door = 0; door < NUM_OF_DIRS; door++)
                if (EXIT(ch, door) && EXIT(ch, door)->to_room)
                    if (!door_is_flagged(ch, door, EX_CLOSED) &&
                        IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room),
                            ROOM_INDOORS)) {
                        perform_move(ch, door, MOVE_NORM, 1);
                        return;
                    }
            send_to_char(ch, "You can't seem to find anything to enter.\r\n");
        }
        return;
    }
    // Handle entering doorways
    for (door = 0; door < NUM_OF_DIRS; door++)
        if (EXIT(ch, door) && EXIT(ch, door)->keyword)
            if (!strcasecmp(EXIT(ch, door)->keyword, arg)) {
                perform_move(ch, door, MOVE_NORM, 1);
                return;
            }
    // We must be entering an object
    car = get_obj_in_list_vis(ch, arg, ch->in_room->contents);
    if (car && IS_OBJ_TYPE(car, ITEM_VEHICLE)) {
        if (CAR_CLOSED(car)) {
            act("The door of $p seems to be closed now.", false, ch,
                car, NULL, TO_CHAR);
            return;
        }

        room = real_room(ROOM_NUMBER(car));
        if (!room) {
            act("The damn door is stuck!", false, ch, NULL, NULL, TO_ROOM);
            return;
        }

        if (!will_fit_in_room(ch, room)) {
            act("$p is already full of people.", false, ch, car, NULL, TO_CHAR);
            return;
        }

        act("$n climbs into $p.", true, ch, car, NULL, TO_ROOM);
        act("You climb into $p.", true, ch, car, NULL, TO_CHAR);
        if (!char_from_room(ch, true) || !char_to_room(ch, room, true))
            return;
        look_at_room(ch, ch->in_room, 0);
        act("$n has climbed into $p.", false, ch, car, NULL, TO_ROOM);
        return;
    }

    if (!car) {
        car = get_obj_in_list_all(ch, arg, ch->carrying);
        if (!car) {
            send_to_char(ch, "There is no %s here.\r\n", arg);
            return;
        }
    }

    if (GET_OBJ_TYPE(car) != ITEM_PORTAL) {
        send_to_char(ch, "You can't enter that!\r\n");
        return;
    }

    if (ROOM_NUMBER(car))
        room = real_room(ROOM_NUMBER(car));
    else
        room = player_loadroom(ch);

    if (!room) {
        send_to_char(ch,
            "This portal has become twisted.  Please notify someone.\r\n");
        return;
    }

    if (CAR_CLOSED(car)) {
        act("$p is currently closed.", false, ch, car, NULL, TO_CHAR);
        return;
    }

    if (!is_authorized(ch, ENTER_ROOM, room)
        || (ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)
            && (!car->in_room || CAN_WEAR(car, ITEM_WEAR_TAKE)))
        || (ROOM_FLAGGED(room, ROOM_HOUSE)
            && !can_enter_house(ch, room->number))
        || (ROOM_FLAGGED(room, ROOM_CLAN_HOUSE)
            && !clan_house_can_enter(ch, room))) {
        act("$p repulses you.", false, ch, car, NULL, TO_CHAR);
        act("$n is repulsed by $p as $e tries to enter it.",
            true, ch, car, NULL, TO_ROOM);
        return;
    }

    if (!will_fit_in_room(ch, room)) {
        act("You are unable to enter $p!", false, ch, car, NULL, TO_CHAR);
        return;
    }
    // charmed check
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        ch->master->in_room == ch->in_room) {
        act("You fear that if you enter $p, you will be separated from $N!",
            false, ch, car, ch->master, TO_CHAR);
        return;
    }

    if (car->action_desc) {
        act(car->action_desc, true, ch, car, NULL, TO_ROOM);
    } else {
        act("$n steps into $p.", true, ch, car, NULL, TO_ROOM);
    }
    act("You step into $p.\r\n", true, ch, car, NULL, TO_CHAR);

    if (!IS_NPC(ch) && ch->in_room->zone != room->zone)
        room->zone->enter_count++;

    was_in = ch->in_room;
    if (!char_from_room(ch, true) || !char_to_room(ch, room, true))
        return;
    look_at_room(ch, ch->in_room, 0);
    if (GET_OBJ_VNUM(car) == 42504) // astral mansion
        act("$n has entered the mansion.", false, ch, car, NULL, TO_ROOM);
    else
        act("$n steps out of $p.", false, ch, car, NULL, TO_ROOM);
    if (GET_OBJ_VAL(car, 3) == 1) {
        act("$p disintegrates as you step through.",
            false, ch, car, NULL, TO_CHAR);

        if (car->in_room) {
            act("$p disintegrates as $n steps through.",
                false, ch, car, NULL, TO_ROOM);
            obj_from_room(car);
        } else
            obj_from_char(car);

        extract_obj(car);
    } else if (GET_OBJ_VAL(car, 3) > 0) {
        if (GET_OBJ_VAL(car, 3) < 5) {
            act("$p flickers as you step through.",
                false, ch, car, NULL, TO_CHAR);
        }
        GET_OBJ_VAL(car, 3) -= 1;
    }

    WAIT_STATE(ch, 2 RL_SEC);

    // Imms should be able to follow
    for (k = ch->followers; k; k = next) {
        next = k->next;
        if (was_in == k->follower->in_room &&
            GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
            !PLR_FLAGGED(k->follower, PLR_WRITING) &&
            can_see_creature(k->follower, ch)) {
            act("You follow $N.\r\n", false, k->follower, NULL, ch, TO_CHAR);
            perform_goto(k->follower, room, true);
        }
    }

    return;
}

ACMD(do_leave)
{
    int door;

    if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
        send_to_char(ch, "You are outside.. where do you want to go?\r\n");
    else {
        for (door = 0; door < NUM_OF_DIRS; door++)
            if (EXIT(ch, door))
                if (EXIT(ch, door)->to_room != NULL)
                    if (!door_is_flagged(ch, door, EX_CLOSED) &&
                        !IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room),
                            ROOM_INDOORS)) {
                        perform_move(ch, door, MOVE_NORM, 1);
                        return;
                    }
        send_to_char(ch, "I see no obvious exits to the outside.\r\n");
    }
}

ACMD(do_stand)
{

/* To determine if char can_land on water, if flying over */

    struct obj_data *obj;
    int can_land = 0;
    const char *to_char, *to_room;

    if (AFF_FLAGGED(ch, AFF_WATERWALK))
        can_land = true;
    for (obj = ch->carrying; obj && !can_land; obj = obj->next_content) {
        if (IS_OBJ_TYPE(obj, ITEM_BOAT))
            can_land = true;
    }

    switch (GET_POSITION(ch)) {
    case POS_SLEEPING:
    case POS_RESTING:
    case POS_SITTING:
        if (AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL)) {
            if (number(1, 50) < GET_STR(ch)) {
                act("You defy the probability waves of the gravity well and struggle to your feet.", false, ch, NULL, NULL, TO_CHAR);
                act("$n defies the gravity well and struggles to $s feet.",
                    true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_STANDING;
            } else {
                act("The gravity well drives you into the ground as you try to stand.", false, ch, NULL, NULL, TO_CHAR);
                act("The gravity well drives $n into the ground as $e attempts to stand.", true, ch, NULL, NULL, TO_ROOM);
                GET_POSITION(ch) = POS_RESTING;
            }
        } else {
            // No gravity well
            switch (GET_POSITION(ch)) {
            case POS_SLEEPING:
                to_char = "You wake up, and stagger to your feet.";
                to_room = "$n wakes up, and staggers to $s feet.";
                break;
            case POS_RESTING:
                to_char = "You stop resting, and stand up.";
                to_room = "$n stops resting, and clambers onto $s feet.";
                break;
            case POS_SITTING:
                to_char = "You clamber to your feet.";
                to_room = "$n clambers to $s feet.";
                break;
            default:
                to_char = "You stand on your feet.";
                to_room = "$n stands on $s feet.";
                slog("Can't happen 1 in do_stand()");
            }
            GET_POSITION(ch) = POS_STANDING;
            act(to_char, false, ch, NULL, NULL, TO_CHAR);
            act(to_room, false, ch, NULL, NULL, TO_ROOM);
        }
        break;
    case POS_STANDING:
    case POS_FIGHTING:
        act("You are already standing.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_MOUNTED:
        act("You should dismount first.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FLYING:
        if (IS_RACE(ch, RACE_GASEOUS)) {
            send_to_char(ch, "You don't have legs.\r\n");
            break;
        } else if (room_is_open_air(ch->in_room)) {
            act("You can't stand on air, silly!", false, ch, NULL, NULL, TO_CHAR);
            break;
        } else if ((ch->in_room->sector_type == SECT_WATER_NOSWIM)
            && (!can_land)) {
            act("You can't land here, on the water!", false, ch, NULL, NULL,
                TO_CHAR);
            break;
        } else {
            switch (SECT_TYPE(ch->in_room)) {
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
                act("You settle lightly to the surface.", false, ch, NULL, NULL,
                    TO_CHAR);
                act("$n drifts downward and stands upon the waters.", true, ch,
                    NULL, NULL, TO_ROOM);
                break;
            case SECT_SWAMP:
                act("You settle lightly onto the swampy ground.",
                    false, ch, NULL, NULL, TO_CHAR);
                act("$n drifts downward and stands on the swampy ground.",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            case SECT_BLOOD:
                act("You settle lightly onto the blood soaked ground.",
                    false, ch, NULL, NULL, TO_CHAR);
                act("$n drifts downward and stand on the blood soaked ground.",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            case SECT_MUDDY:
                act("You settle lightly into the mud.",
                    false, ch, NULL, NULL, TO_CHAR);
                act("$n drifts downward and stands on the mud.",
                    true, ch, NULL, NULL, TO_ROOM);
                break;
            case SECT_DESERT:
                act("You settle lightly to the sands.", false, ch, NULL, NULL,
                    TO_CHAR);
                act("$n drifts downward and stands on the sand.", true, ch, NULL,
                    NULL, TO_ROOM);
                break;
            case SECT_FIRE_RIVER:
                act("You settle lightly into the fires.", false, ch, NULL, NULL,
                    TO_CHAR);
                act("$n drifts downward and stands within the fire.", true, ch,
                    NULL, NULL, TO_ROOM);
                break;
            default:
                act("You settle lightly to the ground.", false, ch, NULL, NULL,
                    TO_CHAR);
                act("$n drifts downward and stands on the ground.", true, ch,
                    NULL, NULL, TO_ROOM);
            }
            GET_POSITION(ch) = POS_STANDING;
        }
        break;
    default:
        act("You stop floating around, and put your feet on the ground.",
            false, ch, NULL, NULL, TO_CHAR);
        act("$n stops floating around, and puts $s feet on the ground.",
            true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_STANDING;
        break;
    }
}

bool
creature_can_fly(struct creature *ch)
{
    int i;

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
        return true;

    for (i = 0; i < NUM_WEARS; i++) {
        if (ch->equipment[i]) {
            if (IS_OBJ_TYPE(ch->equipment[i], ITEM_WINGS))
                return true;
        }
    }
    if (AFF_FLAGGED(ch, AFF_INFLIGHT))
        return true;
    return false;
}

ACMD(do_fly)
{
    if (!creature_can_fly(ch)) {
        send_to_char(ch, "You are not currently able to fly.\r\n");
        return;
    }

    switch (GET_POSITION(ch)) {
    case POS_STANDING:
    case POS_SITTING:
    case POS_RESTING:
        act("Your feet lift from the ground.", false, ch, NULL, NULL, TO_CHAR);
        act("$n begins to float above the ground.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_FLYING;
        break;
    case POS_SLEEPING:
        act("You have to wake up first!", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FIGHTING:
        act("You can't fly until you beat this fool off of you!", false, ch, NULL,
            NULL, TO_CHAR);
        break;
    case POS_FLYING:
        act("You are already in flight.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_MOUNTED:
        if (MOUNTED_BY(ch)) {
            act("You rise off of $N.", false, ch, NULL, MOUNTED_BY(ch), TO_CHAR);
            act("$n rises off of you.", false, ch, NULL, MOUNTED_BY(ch), TO_VICT);
            act("$n rises off of $N.", false, ch, NULL, MOUNTED_BY(ch),
                TO_NOTVICT);
            REMOVE_BIT(AFF2_FLAGS(MOUNTED_BY(ch)), AFF2_MOUNTED);
        }
        dismount(ch);
        GET_POSITION(ch) = POS_FLYING;
        break;
    default:
        act("You stop floating around, and put your feet on the ground.",
            false, ch, NULL, NULL, TO_CHAR);
        act("$n stops floating around, and puts $s feet on the ground.",
            true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_STANDING;
        break;
    }
}

ACMD(do_sit)
{
    switch (GET_POSITION(ch)) {
    case POS_STANDING:
        act("You sit down.", false, ch, NULL, NULL, TO_CHAR);
        act("$n sits down.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
        break;
    case POS_SITTING:
        send_to_char(ch, "You're sitting already.\r\n");
        break;
    case POS_RESTING:
        act("You stop resting, and sit up.", false, ch, NULL, NULL, TO_CHAR);
        act("$n stops resting.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
        break;
    case POS_SLEEPING:
        act("You have to wake up first.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FIGHTING:
        act("Sit down while fighting? are you MAD?", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FLYING:
        act("That's probably not a good idea while flying.", false, ch, NULL, NULL,
            TO_CHAR);
        break;
    case POS_MOUNTED:
        act("You are already seated on $N.", false, ch, NULL, MOUNTED_BY(ch),
            TO_CHAR);
        break;
    default:
        act("You stop floating around, and sit down.", false, ch, NULL, NULL,
            TO_CHAR);
        act("$n stops floating around, and sits down.", true, ch, NULL, NULL,
            TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
        break;
    }
}

ACMD(do_rest)
{
    switch (GET_POSITION(ch)) {
    case POS_STANDING:
        act("You sit down and lay back into a relaxed position.", false, ch, NULL,
            NULL, TO_CHAR);
        act("$n sits down and rests.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_RESTING;
        break;
    case POS_SITTING:
        act("You lay back and rest your tired bones.", false, ch, NULL, NULL,
            TO_CHAR);
        act("$n rests.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_RESTING;
        break;
    case POS_RESTING:
        act("You are already resting.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_SLEEPING:
        act("You have to wake up first.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FIGHTING:
        act("Rest while fighting?  Are you MAD?", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_FLYING:
        act("You better not try that while flying.", false, ch, NULL, NULL, TO_CHAR);
        break;
    case POS_MOUNTED:
        act("You had better get off of $N first.", false, ch, NULL,
            MOUNTED_BY(ch), TO_CHAR);
        break;
    default:
        act("You stop floating around, and stop to rest your tired bones.",
            false, ch, NULL, NULL, TO_CHAR);
        act("$n stops floating around, and rests.", false, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
        break;
    }
}

ACMD(do_sleep)
{
    if (AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        send_to_char(ch, "What, sleep while in a berserk rage??\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_ADRENALINE)) {
        send_to_char(ch, "You can't seem to relax.\r\n");
        return;
    }
    switch (GET_POSITION(ch)) {
    case POS_STANDING:
    case POS_SITTING:
    case POS_RESTING:
        send_to_char(ch, "You go to sleep.\r\n");
        act("$n lies down and falls asleep.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SLEEPING;
        break;
    case POS_SLEEPING:
        send_to_char(ch, "You are already sound asleep.\r\n");
        break;
    case POS_FIGHTING:
        send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
        break;
    case POS_FLYING:
        send_to_char(ch, "That's a really bad idea while flying!\r\n");
        break;
    case POS_MOUNTED:
        if (MOUNTED_BY(ch))
            act("Better not sleep while mounted on $N.", false, ch, NULL,
                MOUNTED_BY(ch), TO_CHAR);
        else {
            send_to_char(ch,
                "You totter around bowlegged... better try that again!\r\n");
            GET_POSITION(ch) = POS_STANDING;
        }
        break;
    default:
        act("You stop floating around, and lie down to sleep.",
            false, ch, NULL, NULL, TO_CHAR);
        act("$n stops floating around, and lie down to sleep.",
            true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SLEEPING;
        break;
    }
}

ACMD(do_wake)
{
    struct creature *vict;
    int self = 0;
    char *arg = tmp_getword(&argument);

    if (*arg) {
        if (GET_POSITION(ch) == POS_SLEEPING)
            send_to_char(ch,
                "You can't wake people up if you're asleep yourself!\r\n");
        else if ((vict = get_char_room_vis(ch, arg)) == NULL) {
            send_to_char(ch, "%s", NOPERSON);
        } else if (vict == ch)
            self = 1;
        else if (GET_POSITION(vict) > POS_SLEEPING) {
            if (AFF2_FLAGGED(vict, AFF2_MEDITATE)) {
                REMOVE_BIT(AFF2_FLAGS(vict), AFF2_MEDITATE);
                act("You break $M out of $S trance.", false, ch, NULL, vict,
                    TO_CHAR);
                act("$n breaks $N out of a trance.", false, ch, NULL, vict,
                    TO_NOTVICT);
                act("$n interrupts your state of meditation.", false, ch, NULL,
                    vict, TO_VICT);
            } else if (AFF_FLAGGED(vict, AFF_CONFUSION)) {
                act("You shake $M around, snapping $s out of a daze.",
                    true, ch, NULL, vict, TO_CHAR);
                act("$n snaps you out of your daze.", true, ch, NULL, vict,
                    TO_VICT);
                REMOVE_BIT(AFF_FLAGS(ch), AFF_CONFUSION);
            } else
                act("$E is already awake.", false, ch, NULL, vict, TO_CHAR);
        } else if (AFF_FLAGGED(vict, AFF_SLEEP))
            act("You can't wake $M up!", false, ch, NULL, vict, TO_CHAR);
        else if (GET_POSITION(vict) == POS_STUNNED)
            act("$E is stunned, and you fail to rouse $M.",
                false, ch, NULL, vict, TO_CHAR);
        else if (GET_POSITION(vict) < POS_STUNNED)
            act("$N is dying, and you can't wake $M up!",
                false, ch, NULL, vict, TO_CHAR);
        else if (AFF3_FLAGGED(vict, AFF3_STASIS))
            act("$N's processes are all inactive, you cannot wake $M.",
                false, ch, NULL, vict, TO_CHAR);
        else {
            act("You wake $M up.", false, ch, NULL, vict, TO_CHAR);
            if (IS_VAMPIRE(vict))
                act("$n has roused you from your unholy slumber!!",
                    false, ch, NULL, vict, TO_VICT | TO_SLEEP);
            else
                act("You are awakened by $n.", false, ch, NULL, vict,
                    TO_VICT | TO_SLEEP);
            GET_POSITION(vict) = POS_SITTING;
        }
        if (!self)
            return;
    }
    if (AFF_FLAGGED(ch, AFF_SLEEP))
        send_to_char(ch, "You can't wake up!\r\n");
    else if (IS_VAMPIRE(ch) && ch->in_room->zone->weather->sunlight ==
        SUN_LIGHT && PRIME_MATERIAL_ROOM(ch->in_room))
        send_to_char(ch, "You are unable to rise from your sleep.\r\n");
    else if (GET_POSITION(ch) > POS_SLEEPING)
        send_to_char(ch, "You are already awake...\r\n");
    else {
        if (AFF3_FLAGGED(ch, AFF3_STASIS)) {
            send_to_char(ch, "Reactivating processes....\r\n");
            REMOVE_BIT(AFF3_FLAGS(ch), AFF3_STASIS);
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        } else
            send_to_char(ch, "You awaken, and sit up.\r\n");
        act("$n awakens.", true, ch, NULL, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_SITTING;
    }
}

ACMD(do_makemount)
{
    struct creature *vict;
    one_argument(argument, buf);

    if (*buf) {
        if (!(vict = get_char_room_vis(ch, buf))) {
            send_to_char(ch, "%s", NOPERSON);
            return;
        }
    } else {
        send_to_char(ch, "What do you wish to make a mount?\r\n");
        return;
    }
    SET_BIT(NPC2_FLAGS(vict), NPC2_MOUNT);
    send_to_char(ch, "%s is now a mount.\r\n", GET_NAME(vict));
    send_to_char(vict, "A saddle suddenly grows on your back.\r\n");
    return;
}

ACMD(do_mount)
{
    struct creature *vict;
    one_argument(argument, buf);

    if (*buf) {
        if (!(vict = get_char_room_vis(ch, buf))) {
            send_to_char(ch, "%s", NOPERSON);
            return;
        }
    } else {
        send_to_char(ch, "What do you wish to mount?\r\n");
        return;
    }
    if (MOUNTED_BY(ch) == vict) {
        act("You are already mounted on $M.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (ch == vict) {
        send_to_char(ch, "Are you some kind of fool, or what!?\r\n");
        return;
    }
    if (MOUNTED_BY(ch)) {
        send_to_char(ch, "You'll have to dismount first.\r\n");
        return;
    }

    if (!(NPC2_FLAGGED(vict, NPC2_MOUNT))) {
        act("You cannot mount $M.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (AFF2_FLAGGED(vict, AFF2_MOUNTED)) {
        for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
            struct creature *tch = it->data;
            if (MOUNTED_BY(tch) == vict) {
                send_to_char(ch, "But %s is already mounted on %s!\r\n",
                    PERS(tch, ch), PERS(vict, ch));
                return;
            }
        }
        REMOVE_BIT(AFF2_FLAGS(vict), AFF2_MOUNTED);
    }
    if (GET_POSITION(ch) < POS_STANDING) {
        act("You need to stand up first!", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if ((!AFF_FLAGGED(vict, AFF_CHARM) || !vict->master || vict->master != ch)
        && (CHECK_SKILL(ch, SKILL_RIDING) < number(50, 101) + GET_LEVEL(vict))
        && GET_LEVEL(ch) < LVL_ETERNAL) {
        act("$N refuses to be mounted by $n.", false, ch, NULL, vict, TO_NOTVICT);
        act("$N refuses to be mounted by you.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
        send_to_char(ch, "You cannot mount up indoors.\r\n");
        return;
    }
    if (GET_MOVE(ch) < 5) {
        send_to_char(ch, "You are too exhausted.\r\n");
        return;
    }
    act("You skillfully mount $N.", false, ch, NULL, vict, TO_CHAR);
    act("$n skillfully mounts $N.", false, ch, NULL, vict, TO_ROOM);

    if (CHECK_SKILL(ch, SKILL_RIDING) + GET_CHA(ch) <
        number(0, 61) + GET_LEVEL(vict)) {
        act("$N throws you off!", false, ch, NULL, vict, TO_CHAR);
        act("$N throws $n off!", false, ch, NULL, vict, TO_ROOM);
        GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 5);
    } else {
        GET_POSITION(ch) = POS_MOUNTED;
        mount(ch, vict);
        SET_BIT(AFF2_FLAGS(vict), AFF2_MOUNTED);
        gain_skill_prof(ch, SKILL_RIDING);
    }
}

ACMD(do_dismount)
{
    if (!MOUNTED_BY(ch)) {
        send_to_char(ch, "You're not even mounted!\r\n");
        return;
    } else {
        act("You skillfully dismount $N.", false, ch, NULL, MOUNTED_BY(ch),
            TO_CHAR);
        act("$n skillfully dismounts $N.", false, ch, NULL, MOUNTED_BY(ch),
            TO_ROOM);
        GET_POSITION(ch) = POS_STANDING;
        REMOVE_BIT(AFF2_FLAGS(MOUNTED_BY(ch)), AFF2_MOUNTED);
        dismount(ch);
        return;
    }
}

ACMD(do_stalk)
{
    void add_stalker(struct creature *ch, struct creature *vict);
    struct creature *vict;

    one_argument(argument, buf);

    if (*buf == '\0') {
        send_to_char(ch, "Whom do you wish to stalk?\r\n");
        return;
    }
    
    vict = get_char_room_vis(ch, buf);
    if (vict == NULL) {
        send_to_char(ch, "%s", NOPERSON);
        return;
    }

    if (ch->master == vict) {
        act("You are already following $M.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
        act("But you only feel like following $N!", false, ch, NULL, ch->master,
            TO_CHAR);
    } else {                    /* Not Charmed follow person */
        if (vict == ch) {
            if (!ch->master) {
                send_to_char(ch, "You must be insane...\r\n");
                return;
            }
        } else {
            if (circle_follow(ch, vict)) {
                act("Sorry, but following in loops is not allowed.", false, ch,
                    NULL, NULL, TO_CHAR);
                return;
            }
            if (ch->master)
                stop_follower(ch);
            REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
            add_stalker(ch, vict);
        }
    }
}

ACMD(do_follow)
{
    void stop_follower(struct creature *ch);
    void add_follower(struct creature *ch, struct creature *leader);

    struct creature *leader;

    one_argument(argument, buf);

    if (*buf == '\0') {
        send_to_char(ch, "Whom do you wish to follow?\r\n");
        return;
    }

    leader = get_char_room_vis(ch, buf);
    if (leader == NULL) {
        send_to_char(ch, "%s", NOPERSON);
        return;
    }

    if (ch->master == leader) {
        act("You are already following $M.", false, ch, NULL, leader, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
        act("But you only feel like following $N!", false, ch, NULL, ch->master,
            TO_CHAR);
    } else {                    /* Not Charmed follow person */
        if (leader == ch) {
            if (!ch->master) {
                send_to_char(ch, "You are already following yourself.\r\n");
                return;
            }
            stop_follower(ch);
        } else {
            if (circle_follow(ch, leader)) {
                act("Sorry, but following in loops is not allowed.", false, ch,
                    NULL, NULL, TO_CHAR);
                return;
            }
            if (ch->master)
                stop_follower(ch);
            REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
            add_follower(ch, leader);
        }
    }
}

ACMD(do_defend)
{
    struct creature *targ;
    char *arg;

    arg = tmp_getword(&argument);

    if (*buf) {
        if (!(targ = get_char_room_vis(ch, arg))) {
            send_to_char(ch, "%s", NOPERSON);
            return;
        }
    } else {
        send_to_char(ch, "Whom do you wish to defend?\r\n");
        return;
    }

    if (targ == ch) {
        if (DEFENDING(ch))
            stop_defending(ch);
        else
            send_to_char(ch,
                "You aren't defending anyone except yourself.\r\n");
        return;
    }

    if (DEFENDING(ch) == targ) {
        act("You are already defending $M.", false, ch, NULL, targ, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && targ != ch->master) {
        act("But you only feel like defending $N!", false, ch, NULL, ch->master,
            TO_CHAR);
    } else {
        if (targ == ch) {
            stop_defending(ch);
        } else {
            start_defending(ch, targ);
        }
    }
}

#define TL_USAGE (subcmd == SKILL_WORMHOLE ?                        \
                  "Usage: wormhole <direction> <distance>\r\n" :    \
                  "Usage: translocate <direction> <distance>\r\n")

#define TL_VANISH (subcmd == SKILL_WORMHOLE ?                       \
                   "$n disappears into the mouth of a transient spacetime wormhole!" : \
                   "$n fades from view into non-corporeality.")

#define TL_APPEAR (subcmd == SKILL_WORMHOLE ?                       \
                   "$n is hurled from the mouth of a transient spacetime wormhole!" : \
                   "$n slowly fades into view from non-corporeality.")

ACMD(do_translocate)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int dir, num = 0, i;
    struct room_data *rm = NULL;
    struct obj_data *obj = NULL, *next_obj = NULL;
    struct affected_type *af_ptr = NULL;

    if (CHECK_SKILL(ch, subcmd) < 20) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }
    two_arguments(argument, arg1, arg2);
    if (!*arg1 || !*arg2) {
        send_to_char(ch, TL_USAGE);
        return;
    }
    if (subcmd == ZEN_TRANSLOCATION) {
        if (!(af_ptr = affected_by_spell(ch, ZEN_TRANSLOCATION))) {
            send_to_char(ch,
                "You have not achieved the zen of translocation.\r\n");
            return;
        }
    }
    if ((dir = search_block(arg1, dirs, 0)) < 0) {
        send_to_char(ch, "There is no such direction.\r\n");
        return;
    }
    if (!ch->in_room->dir_option[dir] ||
        !ch->in_room->dir_option[dir]->to_room ||
        IS_SET(ch->in_room->dir_option[dir]->exit_info, EX_NOPASS)) {
        send_to_char(ch, "There doesn't seem to be a way there.\r\n");
        return;
    }
    if ((num = atoi(arg2)) > (CHECK_SKILL(ch, subcmd) / 4)) {
        send_to_char(ch, "You can't go that far.\r\n");
        return;
    }
    if (num < 0) {
        send_to_char(ch, "Turn around and go the other way, wiseguy.\r\n");
        return;
    }

    if (GET_MANA(ch) < (mag_manacost(ch, subcmd) * num)) {
        send_to_char(ch, "You don't have enough mana to go that far.\r\n");
        return;
    }
    // iterate to find room
    for (i = 0, rm = ch->in_room; i < num; i++) {
        if (ROOM_FLAGGED(rm, ROOM_DEATH) ||
            (rm->dir_option[dir] &&
                IS_SET(rm->dir_option[dir]->exit_info, EX_NOPASS)))
            break;

        if (rm->dir_option[dir] && rm->dir_option[dir]->to_room &&
            (!can_enter_house(ch, rm->dir_option[dir]->to_room->number) ||
                (ROOM_FLAGGED(rm->dir_option[dir]->to_room, ROOM_GODROOM) &&
                    GET_LEVEL(ch) < LVL_CREATOR) ||
                ROOM_FLAGGED(rm->dir_option[dir]->to_room,
                    ROOM_NOTEL | ROOM_NOMAGIC | ROOM_NORECALL)))
            break;

        if (!rm->dir_option[dir] || !(rm = rm->dir_option[dir]->to_room)) {
            rm = NULL;          // bad news, slick
            break;
        }

    }

    GET_MANA(ch) -= (mag_manacost(ch, subcmd) * num);

    if (ch->in_room == rm) {
        send_to_char(ch, "You fail.\r\n");
        return;
    }

    if (subcmd == SKILL_WORMHOLE)
        send_to_char(ch, "You step into the mouth of the wormhole:\r\n");
    else
        send_to_char(ch, "You fade swiftly into another place:\r\n");

    act(TL_VANISH, true, ch, NULL, NULL, TO_ROOM);

    if (!rm) {

        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i))
                extract_obj(GET_EQ(ch, i));
            if (GET_IMPLANT(ch, i))
                extract_obj(GET_IMPLANT(ch, i));
        }
        while ((obj = ch->carrying)) {
            ch->carrying = obj->next_content;
            extract_obj(obj);
        }
        mudlog(GET_INVIS_LVL(ch), NRM, true,
            "%s died %s into solid matter from %d.", GET_NAME(ch),
            subcmd == SKILL_WORMHOLE ? "wormholed" : "translocated",
            ch->in_room->number);

        send_to_char(ch,
            "You go too far, rematerializing inside solid matter!!\r\n"
            "Better luck next time...\r\n");
        creature_die(ch);
        return;
    } else {
        if (!char_from_room(ch, true) || !char_to_room(ch, rm, true))
            return;
        act(TL_APPEAR, true, ch, NULL, NULL, TO_ROOM);
        gain_skill_prof(ch, subcmd);
        look_at_room(ch, ch->in_room, false);

        // translocation has expired
        if (af_ptr && (af_ptr->duration -= ((num * 4) + 10)) <= 0)
            affect_remove(ch, af_ptr);

        // we hit a DT
        if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) &&
            GET_LEVEL(ch) < LVL_AMBASSADOR) {
            log_death_trap(ch);
            death_cry(ch);
            creature_die(ch);
            if (rm->number == 34004) {
                for (obj = rm->contents; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    damage_eq(NULL, obj, dice(10, 100), -1);
                }
            }
        }
    }
}

#undef TL_USAGE
#undef TL_VANISH
#undef TL_APPEAR

int
drag_object(struct creature *ch, struct obj_data *obj, char *argument)
{

    int max_drag = 0;
    int dir = -1;
    int drag_wait = 0;
    int mvm_cost = 0;
    char *arg1;
    struct room_data *theroom = NULL;

    arg1 = tmp_getword(&argument);

    // a character can drag an object twice the weight of his maximum
    // encumberance + a little luck
    drag_wait = MAX(1, (GET_OBJ_WEIGHT(obj) / 100));
    drag_wait = MIN(drag_wait, 3);

    mvm_cost = MAX(15, (GET_OBJ_WEIGHT(obj) / 30));
    mvm_cost = MIN(mvm_cost, 30);

    max_drag =
        (2 * strength_carry_weight(GET_STR(ch))) + (dice(1, (2 * GET_LEVEL(ch))));

    if (CHECK_SKILL(ch, SKILL_DRAG) > 30) {
        max_drag += (3 * CHECK_SKILL(ch, SKILL_DRAG));
    }

    if ((GET_OBJ_WEIGHT(obj)) > max_drag || (GET_MOVE(ch) < 50)) {
        send_to_char(ch, "You don't have the strength to drag %s.\r\n",
            obj->name);
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }
    // Find out which direction the player wants to drag in
    if (is_abbrev(arg1, "north")) {
        dir = 0;
    }

    else if (is_abbrev(arg1, "east")) {
        dir = 1;
    }

    else if (is_abbrev(arg1, "south")) {
        dir = 2;
    }

    else if (is_abbrev(arg1, "west")) {
        dir = 3;
    }

    else if (is_abbrev(arg1, "up")) {
        dir = 4;
    }

    else if (is_abbrev(arg1, "down")) {
        dir = 5;
    }

    else if (is_abbrev(arg1, "future")) {
        dir = 6;
    }

    else if (is_abbrev(arg1, "past")) {
        dir = 7;
    }

    else {
        send_to_char(ch, "Sorry, that's not a valid direction.\r\n");
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }

    if (EXIT(ch, dir) && EXIT(ch, dir)->to_room) {
        theroom = EXIT(ch, dir)->to_room;
    } else {
        send_to_char(ch, "You can't go in that direction.\r\n");
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }

    if ((GET_OBJ_WEIGHT(obj)) > max_drag) {
        send_to_char(ch, "You don't have the strength to drag %s.", obj->name);
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }
    // check for sectors
    if (SECT_TYPE(ch->in_room) == SECT_FLYING ||
        SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM ||
        SECT_TYPE(ch->in_room) == SECT_CLIMBING ||
        SECT_TYPE(ch->in_room) == SECT_FREESPACE ||
        SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER ||
        SECT_TYPE(ch->in_room) == SECT_PITCH_PIT ||
        SECT_TYPE(ch->in_room) == SECT_PITCH_SUB ||
        SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_FIRE) {

        send_to_char(ch, "You are unable to drag objects here.\r\n");
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }
    //now check to make sure the character can go in the specified direction
    if (!CAN_GO(ch, dir) || !can_travel_sector(ch, SECT_TYPE(theroom), 0)) {
        send_to_char(ch, "Sorry you can't go in that direction.\r\n");
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }

    if (room_is_underwater(ch->in_room) ||
        SECT_TYPE(ch->in_room) == SECT_ELEMENTAL_OOZE) {
        mvm_cost = mvm_cost * 2;
        drag_wait = drag_wait * 2;
    }

    if ((ROOM_FLAGGED(theroom, ROOM_HOUSE)
            && !can_enter_house(ch, theroom->number))
        || (ROOM_FLAGGED(theroom, ROOM_CLAN_HOUSE)
            && !clan_house_can_enter(ch, theroom))) {

        act("You are unable to drag $p there.\r\n", false, ch, obj, NULL,
            TO_CHAR);
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }

    if (!CAN_WEAR(obj, ITEM_WEAR_TAKE) || IS_OBJ_TYPE(obj, ITEM_MONEY) ||
        IS_OBJ_TYPE(obj, ITEM_MONEY) || IS_OBJ_TYPE(obj, ITEM_VEHICLE) ||
        IS_OBJ_TYPE(obj, ITEM_PORTAL) || IS_OBJ_TYPE(obj, ITEM_WINDOW) ||
        IS_OBJ_TYPE(obj, ITEM_V_WINDOW) || IS_OBJ_TYPE(obj, ITEM_V_CONSOLE) ||
        IS_OBJ_TYPE(obj, ITEM_V_DOOR) || IS_OBJ_TYPE(obj, ITEM_SCRIPT) ||
        !OBJ_APPROVED(obj)) {

        act("You can't drag $p you buffoon!", false, ch, obj, NULL, TO_CHAR);
        WAIT_STATE(ch, 1 RL_SEC);
        return 0;
    }
    // Now we're going to move the character and the object from the
    // original room to the new one

    act(tmp_sprintf("You drag $p %s.", to_dirs[dir]),
        false, ch, obj, NULL, TO_CHAR);
    act(tmp_sprintf("$n drags $p %s.", to_dirs[dir]),
        false, ch, obj, NULL, TO_ROOM);

    // We want the player to see the object in the room, so we move it
    // before the player.  If the move fails, we move it back to its
    // previous position.
    struct room_data *orig_room = obj->in_room;
    obj_from_room(obj);
    obj_to_room(obj, theroom);
    switch (perform_move(ch, dir, MOVE_NORM, 1)) {
    case 0:                    // Success
        act(tmp_sprintf("$n drags $p in from %s.", from_dirs[dir]),
            false, ch, obj, NULL, TO_ROOM);
        GET_MOVE(ch) = MAX(0, (GET_MOVE(ch) - mvm_cost));
        WAIT_STATE(ch, (drag_wait RL_SEC));
        break;
    case 1:                    // Simple failure
        obj_from_room(obj);
        obj_to_room(obj, orig_room);
        break;
    case 2:                    // Death failure
        // Leave object where it is
        break;
    }

    return 1;
}
