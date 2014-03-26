/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: graph.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
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
#include "char_class.h"
#include "tmpstr.h"
#include "accstr.h"
#include "spells.h"
#include "fight.h"
#include "strutil.h"
#include "voice.h"

/* Externals */
extern struct room_data *world;

int has_key(struct creature *ch, obj_num key);

ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_fly);
ACMD(do_bash);
ACMD(do_cast);

struct bfs_queue_struct {
    struct room_data *room;
    char dir;
    struct bfs_queue_struct *next;
};

struct bfs_queue_struct *queue_head = NULL, *queue_tail = NULL;
// Can't be static since it's used in map.
unsigned char find_first_step_index = 0;

/* Utility macros */
#define MARK(room) ( room->find_first_step_index = find_first_step_index )
#define UNMARK(room) ( room->find_first_step_index = 0 )
#define IS_MARKED(room) ( room->find_first_step_index == find_first_step_index )

struct room_data *
to_room(struct room_data *room, int dir)
{
    if (room->dir_option[dir])
        return room->dir_option[dir]->to_room;
    return NULL;
}

bool
valid_edge(struct room_data * room, int dir, enum track_mode mode)
{
    struct room_data *dest = to_room(room, dir);

    if (!dest)
        return false;
    if (IS_MARKED(dest))
        return false;
#ifdef TRACK_THROUGH_DOORS
    if (IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED))
        return false;
#endif
    if (mode == PSI_TRACK && ROOM_FLAGGED(dest, ROOM_NOPSIONICS))
        return false;
    if (mode == GOD_TRACK)
        return true;
    if (ROOM_FLAGGED(dest, ROOM_NOTRACK | ROOM_DEATH))
        return false;
    return true;
}

void
bfs_enqueue(struct room_data *room, char dir)
{

    struct bfs_queue_struct *curr;

    CREATE(curr, struct bfs_queue_struct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = NULL;

    if (queue_tail) {
        queue_tail->next = curr;
        queue_tail = curr;
    } else
        queue_head = queue_tail = curr;
}

void
bfs_dequeue(void)
{

    struct bfs_queue_struct *curr;

    curr = queue_head;
    queue_head = queue_head->next;
    if (!queue_head)
        queue_tail = NULL;

#ifdef DMALLOC
    dmalloc_verify(0);
#endif
    free(curr);
#ifdef DMALLOC
    dmalloc_verify(0);
#endif
}

void
bfs_clear_queue(void)
{
    while (queue_head)
        bfs_dequeue();
}

/* find_first_step: given a source room and a target room, find the first
   step on the shortest path from the source to the target.

   Intended usage: in mobile_activity, give a mob a dir to go if they're
   tracking another mob or a PC.  Or, a 'track' skill for PCs.

   mode: true -> go thru DT's, doorz, !track roomz
*/

int
find_first_step(struct room_data *src, struct room_data *target,
    enum track_mode mode)
{
    int curr_dir;
    struct room_data *curr_room;
    struct zone_data *zone;

    if (!src || !target) {
        slog("Illegal value passed to find_first_step (graph.c)");
        return BFS_ERROR;
    }
    if (src == target)
        return BFS_ALREADY_THERE;

    // increment the static index
    ++find_first_step_index;

    // check if find_first_step_index has rolled over to zero
    if (!find_first_step_index) {
        // put index at 1;
        ++find_first_step_index;

        // reset all rooms' indices to zero
        for (zone = zone_table; zone; zone = zone->next)
            for (curr_room = zone->world; curr_room;
                curr_room = curr_room->next)
                UNMARK(curr_room);
    }

    MARK(src);

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
        if (valid_edge(src, curr_dir, mode)) {
            MARK(to_room(src, curr_dir));
            bfs_enqueue(to_room(src, curr_dir), curr_dir);
        }
    /* now, do the char_classic BFS. */
    while (queue_head) {
        if (queue_head->room == target) {
            curr_dir = queue_head->dir;
            bfs_clear_queue();
            return curr_dir;
        } else {
            for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
                if (valid_edge(queue_head->room, curr_dir, mode)) {
                    MARK(to_room(queue_head->room, curr_dir));
                    bfs_enqueue(to_room(queue_head->room, curr_dir),
                        queue_head->dir);
                }
            bfs_dequeue();
        }
    }

    return BFS_NO_PATH;
}

int
find_distance(struct room_data *start, struct room_data *dest)
{
    int dir = -1, steps = 0;
    struct room_data *cur_room = start;

    dir = find_first_step(cur_room, dest, GOD_TRACK);
    while (cur_room && dir >= 0 && steps < 600) {
        cur_room = ABS_EXIT(cur_room, dir)->to_room;
        steps++;
        dir = find_first_step(cur_room, dest, GOD_TRACK);
    }

    if (cur_room != dest || steps >= 600)
        return (-1);

    return steps;
}

/************************************************************************
*  Functions and Commands which use the above fns		        *
************************************************************************/
void
show_trails_to_char(struct creature *ch, char *str)
{
    struct room_trail_data *trail;
    bool found = false;
    const char *foot_desc, *drop_desc;
    int prob;

    acc_string_clear();

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && room_is_open_air(ch->in_room)) {
        send_to_char(ch, "Track through the open air?\r\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && room_is_watery(ch->in_room)) {
        send_to_char(ch,
            "You find it difficult to follow the ripples of the water.\r\n");
        return;
    }

    for (trail = ch->in_room->trail; trail; trail = trail->next) {
        if (str && !isname(str, trail->aliases))
            continue;

        if ((!IS_NPC(ch) || !NPC_FLAGGED(ch, NPC_SPIRIT_TRACKER))
            && !IS_IMMORT(ch)) {

            prob = (10 - trail->track) * 10;
            if (str)
                prob += 20;
            if (GET_CLASS(ch) == CLASS_RANGER) {
                if ((ch->in_room->sector_type == SECT_FOREST) ||
                    (ch->in_room->sector_type == SECT_FIELD) ||
                    (ch->in_room->sector_type == SECT_HILLS) ||
                    (ch->in_room->sector_type == SECT_MOUNTAIN) ||
                    (ch->in_room->sector_type == SECT_TRAIL) ||
                    (ch->in_room->sector_type == SECT_ROCK) ||
                    (ch->in_room->sector_type == SECT_MUDDY))
                    prob = 20;
                else
                    prob = 10;
                if (!OUTSIDE(ch))
                    prob -= 40;
            }
            if (IS_TABAXI(ch))
                prob += GET_LEVEL(ch) / 2;
            if (number(0, 101) < MIN(100, prob))
                continue;
        }

        found = true;
        if (IS_SET(trail->flags, TRAIL_FLAG_BLOOD_DROPS) && number(0, 1)) {
            foot_desc = "A trail of blood drops";
            drop_desc = "s";
        } else {
            foot_desc = tmp_sprintf("%s's %s%sfootprints",
                trail->name,
                (trail->track < 4) ? "faint " : "",
                (IS_SET(trail->flags,
                        TRAIL_FLAG_BLOODPRINTS)) ? "bloody " : "");
            drop_desc = "";
        }
        if (trail->from_dir >= 0) {
            if (trail->to_dir >= 0) {
                if (trail->from_dir == trail->to_dir)
                    acc_sprintf("%s double%s back %s.\r\n",
                        foot_desc, drop_desc, to_dirs[(int)trail->from_dir]);
                else if (trail->from_dir == rev_dir[(int)trail->to_dir])
                    acc_sprintf("%s lead%s straight from %s to %s.\r\n",
                        foot_desc, drop_desc, from_dirs[(int)trail->to_dir],
                        dirs[(int)trail->to_dir]);
                else
                    acc_sprintf("%s turn%s from %s to %s.\r\n",
                        foot_desc, drop_desc, dirs[(int)trail->from_dir],
                        dirs[(int)trail->to_dir]);
            } else
                acc_sprintf("%s lead%s from the %s.\r\n",
                    foot_desc, drop_desc, dirs[(int)trail->from_dir]);
        } else if (trail->to_dir >= 0)
            acc_sprintf("%s lead%s %s.\r\n",
                foot_desc, drop_desc, to_dirs[(int)trail->to_dir]);
    }

    if (!found) {
        if (str)
            acc_sprintf("You don't see any matching footprints here.\r\n");
        else
            acc_sprintf("You don't see any prints here.\r\n");
    }
    page_string(ch->desc, acc_get_string());
}

ACMD(do_track)
{
    struct creature *vict;
    int dir;
    bool spirit = affected_by_spell(ch, SPELL_SPIRIT_TRACK);

    if (GET_MOVE(ch) < 15 && !spirit) {
        send_to_char(ch, "You are too exhausted!\r\n");
        return;
    }

    bool good_track = false;
    if (number(30, 200) < skill_bonus(ch, SKILL_TRACK))
        good_track = true;

    if (!GET_SKILL(ch, SKILL_TRACK) && !spirit) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }
    char *arg = tmp_getword(&argument);
    if (!*arg || (!spirit && !good_track)) {
        show_trails_to_char(ch, NULL);
        if (!spirit) {
            GET_MOVE(ch) -= 10;
        }
        WAIT_STATE(ch, 6);
        return;
    }

    vict = get_char_vis(ch, arg);

    if (!vict || !can_see_creature(ch, vict)) {
        send_to_char(ch, "No-one around by that name.\r\n");
        return;
    }

    if (vict->in_room == ch->in_room) {
        send_to_char(ch, "You're already in the same room!\r\n");
        return;
    }

    dir = find_first_step(ch->in_room, vict->in_room,
        GET_LEVEL(ch) > LVL_TIMEGOD ? GOD_TRACK : STD_TRACK);

    //misdirection melisma
    struct affected_type *misdirection =
        affected_by_spell(vict, SONG_MISDIRECTION_MELISMA);
    if (misdirection && misdirection->level + 40 > random_number_zero_low(150)) {
        int total = 0;
        int dirs[NUM_OF_DIRS];

        for (int idx = 0; idx < NUM_OF_DIRS; idx++) {
            if (vict->in_room->dir_option[idx] &&
                vict->in_room->dir_option[idx]->to_room &&
                vict->in_room->dir_option[idx]->to_room != vict->in_room) {
                dirs[total] = idx;
                total++;
            }
        }
        dir = dirs[random_number_zero_low(total - 1)];
    }

    switch (dir) {
    case BFS_ERROR:
        send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
        break;
    case BFS_ALREADY_THERE:
        send_to_char(ch, "You're already in the same room!!\r\n");
        break;
    case BFS_NO_PATH:
        send_to_char(ch, "You can't sense a trail to %s from here.\r\n",
            HMHR(vict));
        break;
    default:
        if (!number(0, 99))
            gain_skill_prof(ch, SKILL_TRACK);
        send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]);
        WAIT_STATE(ch, 6);
        break;
    }
    if (!spirit) {
        GET_MOVE(ch) -= 10;
    }
}

ACMD(do_psilocate)
{

    struct creature *vict = NULL;
    int dist, dir;
    int8_t error;

    skip_spaces(&argument);

    if (CHECK_SKILL(ch, SKILL_PSILOCATE) < 10) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }
    if (!*argument) {
        send_to_char(ch, "Locate who's psyche?\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, argument))) {
        send_to_char(ch, "You cannot locate %s '%s'.\r\n", AN(argument),
            argument);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS) && GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "Psychic powers are useless here!\r\n");
        return;
    }
    if (ROOM_FLAGGED(vict->in_room, ROOM_NOPSIONICS)
        && GET_LEVEL(ch) < LVL_GOD) {
        act("Psychic powers are useless where $E is!", false, ch, NULL, vict,
            TO_CHAR);
        return;
    }
    if (vict && (IS_UNDEAD(vict) || IS_SLIME(vict) || IS_PUDDING(vict) ||
            IS_ROBOT(vict) || IS_PLANT(vict))) {
        act("It is pointless to attempt this on $M.", false, ch, NULL, vict,
            TO_CHAR);
        return;
    }
    if (GET_MANA(ch) < mag_manacost(ch, SKILL_PSILOCATE)) {
        send_to_char(ch, "You are too psychically exhausted.\r\n");
        return;
    }

    GET_MANA(ch) -= mag_manacost(ch, SKILL_PSILOCATE);
    act("$n begins concentrating deeply, on a distant psyche.",
        true, ch, NULL, NULL, TO_ROOM);

    if ((AFF3_FLAGGED(vict, AFF3_SHROUD_OBSCUREMENT) &&
            ((GET_LEVEL(vict) * 3) / 4 > number(10, CHECK_SKILL(ch,
                        SKILL_PSILOCATE))))
        || AFF3_FLAGGED(vict, AFF3_PSISHIELD)) {
        act("You cannot sense $S psi.", false, ch, NULL, vict, TO_CHAR);
        return;
    }

    if ((dist = find_distance(ch->in_room, vict->in_room)) +
        (AFF3_FLAGGED(vict, AFF3_PSISHIELD) ? (GET_LEVEL(vict) / 2) : 0) >
        GET_LEVEL(ch) + (GET_REMORT_GEN(ch) * 16) + GET_INT(ch)) {
        act("$N is out of your psychic range.", false, ch, NULL, vict, TO_CHAR);
        return;
    }

    if ((dir = find_first_step(ch->in_room, vict->in_room, PSI_TRACK)) ==
        BFS_ALREADY_THERE) {
        send_to_char(ch, "You are in the same room!\r\n");
        return;
    }
    if (dir < 0) {
        act("You cannot sense $S psi.", false, ch, NULL, vict, TO_CHAR);
        return;
    }

    if (CHECK_SKILL(vict, SKILL_PSILOCATE) >
        number(10, CHECK_SKILL(ch, SKILL_PSILOCATE) + GET_LEVEL(ch)))
        act("You feel $n's psyche connect with your mind briefly.",
            false, ch, NULL, vict, TO_VICT);
    else
        send_to_char(vict,
            "You feel a strange sensation on the periphery of your psyche.\r\n");

    //misdirection melisma and psilocate failure
    struct affected_type *misdirection =
        affected_by_spell(vict, SONG_MISDIRECTION_MELISMA);
    if (number(0, 121) > CHECK_SKILL(ch, SKILL_PSILOCATE) + GET_INT(ch)
        || (misdirection
            && misdirection->level + 40 > random_number_zero_low(150))) {
        int total = 0;
        int dirs[NUM_OF_DIRS];

        for (int idx = 0; idx < NUM_OF_DIRS; idx++) {
            if (vict->in_room->dir_option[idx] &&
                vict->in_room->dir_option[idx]->to_room &&
                vict->in_room->dir_option[idx]->to_room != vict->in_room) {
                dirs[total] = idx;
                total++;
            }
        }
        dir = dirs[random_number_zero_low(total - 1)];
    }
    error = number(0, 140 - CHECK_SKILL(ch, SKILL_PSILOCATE) - GET_INT(ch));
    error = MAX(0, error) / 8;

    if (error)
        dist += number(-error, error);

    dist = MAX(1, dist);

    act(tmp_sprintf("$N seems to be about %d rooms away %s.", dist,
            to_dirs[dir]), false, ch, NULL, vict, TO_CHAR);
}

//
// smart_mobile_move()
// returns -1 on a critical failure
//

int
smart_mobile_move(struct creature *ch, int dir)
{

    char doorbuf[128];

    if (dir < 0)
        return 0;

    if (EXIT(ch, dir) && EXIT(ch, dir)->to_room != NULL) {
        if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
            if (IS_SET(EXIT(ch, dir)->exit_info, EX_SPECIAL))   // can't open here
                return 0;
            if (EXIT(ch, dir)->keyword)
                strcpy(doorbuf, fname(EXIT(ch, dir)->keyword));
            else
                sprintf(doorbuf, "door %s", dirs[dir]);

            if (IS_SET(EXIT(ch, dir)->exit_info, EX_LOCKED)) {
                if (has_key(ch, EXIT(ch, dir)->key))
                    do_gen_door(ch, doorbuf, 0, SCMD_UNLOCK);
                else if ((IS_THIEF(ch) || IS_BARD(ch)) &&
                    CHECK_SKILL(ch, SKILL_PICK_LOCK) > 30)
                    do_gen_door(ch, doorbuf, 0, SCMD_PICK);
                else if (IS_MAGE(ch) && CHECK_SKILL(ch, SPELL_KNOCK) &&
                    GET_MANA(ch) > (GET_MAX_MANA(ch) / 2)) {
                    sprintf(doorbuf, "'knock' %s", doorbuf);
                    do_cast(ch, doorbuf, 0, 0);
                } else if (CHECK_SKILL(ch, SKILL_BREAK_DOOR) > 30 &&
                    GET_HIT(ch) > (GET_MAX_HIT(ch) / 2))
                    do_bash(ch, doorbuf, 0, 0);
            } else
                do_gen_door(ch, doorbuf, 0, SCMD_OPEN);

        } else if (room_is_open_air(EXIT(ch, dir)->to_room) &&
            GET_POSITION(ch) != POS_FLYING) {
            if (can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0))
                do_fly(ch, tmp_strdup(""), 0, 0);
            else if (IS_MAGE(ch) && GET_LEVEL(ch) >= 33)
                cast_spell(ch, ch, NULL, NULL, SPELL_FLY);
            else if (IS_CLERIC(ch) && GET_LEVEL(ch) >= 32)
                cast_spell(ch, ch, NULL, NULL, SPELL_AIR_WALK);
            else if (IS_PHYSIC(ch))
                cast_spell(ch, ch, NULL, NULL, SPELL_TIDAL_SPACEWARP);
            else if (!number(0, 10)) {
                emit_voice(ch, NULL, VOICE_HUNT_OPENAIR);
                return 0;
            }
        } else if (SECT_TYPE(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM &&
            GET_POSITION(ch) != POS_FLYING &&
            can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0)) {
            if (AFF_FLAGGED(ch, AFF_INFLIGHT))
                do_fly(ch, tmp_strdup(""), 0, 0);
            else if (IS_MAGE(ch) && GET_LEVEL(ch) >= 32)
                cast_spell(ch, ch, NULL, NULL, SPELL_WATERWALK);
            else if (!number(0, 10)) {
                emit_voice(ch, NULL, VOICE_HUNT_WATER);
                return 0;
            }
        } else if (perform_move(ch, dir, MOVE_NORM, 1) == 2) {
            //critical failure - possible death
            return -1;
        }

    }
    return 0;
}

//
// hunt_victim()
//
// has a return value like damage()
//

void
hunt_victim(struct creature *ch)
{
    struct affected_type *af_ptr = NULL;
    int dir;

    if (!ch || !NPC_HUNTING(ch))
        return;

    if (is_dead(NPC_HUNTING(ch))) {
        stop_hunting(ch);
        return;
    }

    if (!NPC_HUNTING(ch)->in_room) {
        errlog(" hunting ! NPC_HUNTING(ch)->in_room !!");
        return;
    }

    /* make sure the char still exists */
    if (!g_list_find(creatures, NPC_HUNTING(ch))) {
        if (!is_fighting(ch)) {
            emit_voice(ch, NULL, VOICE_HUNT_GONE);
            stop_hunting(ch);
        }
        return;
    }
    if (GET_LEVEL(NPC_HUNTING(ch)) >= LVL_AMBASSADOR) {
        stop_hunting(ch);
        return;
    }
    if (g_list_find(ch->fighting, NPC_HUNTING(ch)))
        return;

    if (ch->in_room == NPC_HUNTING(ch)->in_room &&
        !is_fighting(ch) && can_see_creature(ch, NPC_HUNTING(ch)) &&
        !PLR_FLAGGED(NPC_HUNTING(ch), PLR_WRITING) &&
        (!(af_ptr = affected_by_spell(NPC_HUNTING(ch), SKILL_DISGUISE)) ||
            CAN_DETECT_DISGUISE(ch, NPC_HUNTING(ch), af_ptr->duration))) {
        if (ok_to_attack(ch, NPC_HUNTING(ch), false)
            && !check_infiltrate(NPC_HUNTING(ch), ch)) {
            best_initial_attack(ch, NPC_HUNTING(ch));
        }
        return;
    }
    if (IS_CLERIC(ch) || IS_MAGE(ch)) {
        if (NPC_HUNTING(ch)->in_room && can_see_creature(ch, NPC_HUNTING(ch))
            && ok_to_attack(ch, NPC_HUNTING(ch), false)) {
            if ((IS_CLERIC(ch) && GET_LEVEL(ch) > 16) || (IS_MAGE(ch)
                    && GET_LEVEL(ch) > 27)) {
                if (GET_MANA(ch) < mag_manacost(ch, SPELL_SUMMON)) {
                    cast_spell(ch, NPC_HUNTING(ch), NULL, NULL, SPELL_SUMMON);
                    return;
                }
            }
        }
    }

    if (!AFF_FLAGGED(NPC_HUNTING(ch), AFF_NOTRACK) ||
        (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_SPIRIT_TRACKER))) {
        dir =
            find_first_step(ch->in_room, NPC_HUNTING(ch)->in_room, STD_TRACK);

        //misdirection melisma
        struct affected_type *misdirection =
            affected_by_spell(NPC_HUNTING(ch), SONG_MISDIRECTION_MELISMA);
        if (misdirection
            && misdirection->level + 40 > random_number_zero_low(150)) {
            int total = 0;
            int dirs[NUM_OF_DIRS];

            memset(dirs, -1, sizeof(dirs));

            for (int idx = 0; idx < NUM_OF_DIRS; idx++) {
                if (NPC_HUNTING(ch)->in_room->dir_option[idx] &&
                    NPC_HUNTING(ch)->in_room->dir_option[idx]->to_room &&
                    NPC_HUNTING(ch)->in_room->dir_option[idx]->to_room !=
                    NPC_HUNTING(ch)->in_room) {
                    dirs[total] = idx;
                    total++;
                }
            }
            dir = dirs[random_number_zero_low(total - 1)];
        }
    } else {
        dir = -1;
    }
    if (dir < 0
        || find_distance(ch->in_room,
            NPC_HUNTING(ch)->in_room) > GET_INT(ch)) {
        emit_voice(ch, NPC_HUNTING(ch), VOICE_HUNT_LOST);
        stop_hunting(ch);
        return;
    }
    if (smart_mobile_move(ch, dir) < 0) {
        stop_hunting(ch);
        return;
    }

    if ((ch->in_room == NPC_HUNTING(ch)->in_room)
        && can_see_creature(ch, NPC_HUNTING(ch))
        && (!(af_ptr = affected_by_spell(NPC_HUNTING(ch), SKILL_DISGUISE))
            || CAN_DETECT_DISGUISE(ch, NPC_HUNTING(ch), af_ptr->duration))
        && !check_infiltrate(NPC_HUNTING(ch), ch)) {
        if (ok_to_attack(ch, NPC_HUNTING(ch), false)
            && !PLR_FLAGGED(NPC_HUNTING(ch), PLR_WRITING)) {
            if (GET_POSITION(ch) >= POS_STANDING && !is_fighting(ch)) {
                emit_voice(ch, NPC_HUNTING(ch), VOICE_HUNT_FOUND);
                best_initial_attack(ch, NPC_HUNTING(ch));
                return;
            }
        }
    } else if (ch->in_room == NPC_HUNTING(ch)->in_room) {
        if (!number(0, 10))
            emit_voice(ch, NPC_HUNTING(ch), VOICE_HUNT_UNSEEN);
        return;
    } else {
        if (!NPC2_FLAGGED(ch, NPC2_SILENT_HUNTER) &&
            (GET_LEVEL(ch) + number(1, 12)) > GET_LEVEL(NPC_HUNTING(ch))) {
            emit_voice(ch, NPC_HUNTING(ch), VOICE_HUNT_TAUNT);
        }
    }
}
