//
// File: act.physic.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/* ************************************************************************
*   File: act.physic.c                Created for TempusMUD by Fireball   *
************************************************************************ */

/*
  //
  ************************************************************************

  State of the physics:

  #define SPELL_ACIDITY              301
  complete

  #define SPELL_ATTRACTION_FIELD     302
  complete

  #define SPELL_NUCLEAR_WASTELAND    303
  complete

  #define SPELL_FLUORESCE                   304
  complete

  #define SPELL_GAMMA_RAY            305
  incomplete ( needs damage adjustment )

  #define SPELL_HALFLIFE             306
  incomplete ( needs damage adjustment, possible affects adjustment )

  #define SPELL_MICROWAVE            307
  incomplete ( needs damage adjustment )

  #define SPELL_OXIDIZE              308
  complete

  #define SPELL_RANDOM_COORDINATES   309  // random teleport
  complete

  #define SPELL_REPULSION_FIELD      310
  complete

  #define SPELL_TRANSMITTANCE        311  // transparency
  complete

  #define SPELL_SPACETIME_IMPRINT    312  // sets room as teleport spot
  complete

  #define SPELL_SPACETIME_RECALL     313  // teleports to imprint telep spot
  complete

  #define SPELL_TIME_WARP            314  // random teleport into other time
  complete

  #define SPELL_TIDAL_SPACEWARP      315  // fly
  complete

  #define SPELL_FISSION_BLAST        316  // full-room damage
  complete

  #define SPELL_REFRACTION           317  // like displacement
  complete

  #define SPELL_ELECTROSHIELD        318  // prot_lightning
  complete

  #define SPELL_VACUUM_SHROUD        319  // eliminates breathing and fire
  complete

  #define SPELL_DENSIFY              320  // increase weight of obj & char
  complete

  #define SPELL_UNUSED_321

  #define SPELL_ENTROPY_FIELD        322  // drains move on victim (time effect)
  complete

  #define SPELL_GRAVITY_WELL         323  // time effect crushing damage
  complete

  #define SPELL_CAPACITANCE_BOOST    324  // increase maxmv
  complete

  #define SPELL_ELECTRIC_ARC         325  // lightning bolt
  complete

  #define SPELL_SONIC_BOOM           326  // area damage + wait state
  incomplete

  #define SPELL_LATTICE_HARDENING     327  // dermal hard or increase obj maxdam
  complete

  #define SPELL_NULLIFY              328  // like dispel magic

  #define SPELL_FORCE_WALL           329  // sets up an exit blocker
  incomplete

  #define SPELL_UNUSED_330           330  //

  #define SPELL_PHASING              331  // invuln.
  incomplete

  #define SPELL_ABSORPTION_SHIELD    332  // works like mana shield
  incomplete

  #define SPELL_TEMPORAL_COMPRESSION 333  // works like haste
  complete

  #define SPELL_TEMPORAL_DILATION    334  // works like slow
  complete

  #define SPELL_GAUSS_SHIELD         335  // half damage from metal
  complete

  #define SPELL_ALBEDO_SHIELD        336  // reflects e/m attacks
  complete

  #define SPELL_THERMOSTATIC_FIELD   337  // sets prot_heat + end_cold

  #define SPELL_RADIOIMMUNITY        338  // sets prot_rad
  complete

  #define SPELL_TRANSDIMENSIONALITY  339  // randomly teleport to another plane
  incomplete

  #define SPELL_AREA_STASIS          340  // sets !phy room flag
  complete

  #define SPELL_ELECTROSTATIC_FIELD  341  // protective static field does damage to attackers
  complete

  #define SKILL_WORMHOLE              599 // physic's wormhole
  complete

  #define SKILL_LECTURE               600 // physic's boring-ass lecture
  complete

  #define SKILL_ENERGY_CONVERSION     679 // physic's energy conversion
  complete
  #define SPELL_EMP_PULSE            342  // Shuts off devices, communicators
                                          // deactivats all cyborg programs
                                          // blocked by emp shield

  ************************************************************************
  //
*/

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <stdlib.h>
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
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "libpq-fe.h"
#include "db.h"
#include "screen.h"
#include "house.h"
#include "char_class.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "flow_room.h"
#include "bomb.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "specs.h"
#include "strutil.h"
#include "actions.h"

static timewarp_data *timewarp_list = NULL;
static int num_timewarp_data = 0;

struct creature *
check_char_room_vis(struct creature *ch, char *argument)
{
    struct creature *vict = NULL;

    if (!(vict = get_char_room_vis(ch, argument))) {
        send_to_char(ch, "There's no-one named '%s' here.\r\n", argument);
    }
    return (vict);
}

// physic's boring-ass lecture
#define NUM_TOPICS 19

const char *lecture_topics[NUM_TOPICS] = {
    "on the subject of particle physics.",
    "on the benefits of Hilbert Spaces.",
    "about the lesser-known properties of the cross product.",
    "on LaGrangian mechanics.",
    "on the nature of wave-particle duality.",
    "about the equations for Fresnel diffraction.",
    "on the analysis of fluid flow.",
    "on the analysis of rigid body motion.",
    "on the nature of conservative fields.",
    "on the properties of lamellar fields.",
    "on the use of integrating circuits.",
    "on the subject of microwave interference.",
    "on the construction of solid-state lasers.",
    "on the use of Hermitian operators.",
    "on the applications of the Gradient Theorem.",
    "on forced underdamped harmonic oscillators.",
    "on the effects of magnetic induction.",
    "on the theory of General Relativity.",
    "on Special Relativistic mechanics.",
};

ACMD(do_lecture)
{
    struct creature *vict = NULL;
    int prob, index, wait, percent;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Lecture who?\r\n");
        return;
    }

    if (!(vict = check_char_room_vis(ch, argument)))
        return;

    if (vict == ch) {
        send_to_char(ch, "You can't lecture yourself.\r\n");
        return;
    }

    if (!AWAKE(vict)) {
        act("$E is not in a state which is receptive to the finer points of lecturing.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (CHECK_SKILL(ch, SKILL_LECTURE) < 30) {
        act("$n explains the finer points of gravity, by tossing $mself to the ground!", false, ch, NULL, NULL, TO_ROOM);
        act("Your flying leap seems to have convinced $N.", false, ch, NULL, vict,
            TO_CHAR);
        GET_POSITION(ch) = POS_RESTING;
        return;
    }

    if (is_fighting(vict)) {
        act("$E is busy fighting right now!", false, ch, NULL, vict, TO_CHAR);
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;
    if (!ok_damage_vendor(ch, vict)) {
        act("$E is interested in your wallet, not your words.", false, ch, NULL,
            vict, TO_CHAR);
        return;
    }

    appear(ch, vict);
    check_attack(ch, vict);

    prob = skill_bonus(ch, SKILL_LECTURE) + (GET_INT(ch) * 2);
    if (AFF_FLAGGED(ch, AFF_CONFUSION))
        prob -= 60;

    index = number(0, NUM_TOPICS - 1);

    act(tmp_sprintf("You begin lecturing $N %s", lecture_topics[index]),
        false, ch, NULL, vict, TO_CHAR);
    act(tmp_sprintf("$n begins lecturing $N %s", lecture_topics[index]),
        false, ch, NULL, vict, TO_NOTVICT);
    act(tmp_sprintf("$n begins lecturing you %s", lecture_topics[index]),
        false, ch, NULL, vict, TO_VICT);

    percent = (GET_LEVEL(vict) / 2) + GET_REMORT_GEN(vict) + GET_INT(vict);
    percent += number(0, 60);

    if (affected_by_spell(vict, SPELL_ENDURANCE))
        percent += 10;
    if (AFF_FLAGGED(vict, AFF_ADRENALINE))
        percent += 10;
    if (AFF2_FLAGGED(vict, AFF2_BERSERK))
        percent += 50;

    if (NPC_FLAGGED(vict, NPC_NOSLEEP) || IS_UNDEAD(vict) ||
        CHECK_SKILL(ch, SKILL_LECTURE) < 30 || IS_PHYSIC(vict))
        prob = 0;

    // he likes it!
    if (IS_PHYSIC(vict)) {
        act("$n appears to be fascinated, and nods in agreement.", true, vict,
            NULL, NULL, TO_ROOM);
        send_to_char(vict, "Fascinating!\r\n");
    }
    // victim falls asleep
    else if (percent < prob) {
        act("$n immediately dozes off to sleep.", true, vict, NULL, NULL, TO_ROOM);
        send_to_char(vict, "You start to feel very sleepy...\r\n");
        GET_POSITION(vict) = POS_SLEEPING;
        wait = 2 RL_SEC + ((prob - percent) / 2);
        WAIT_STATE(vict, wait);
        gain_skill_prof(ch, SKILL_LECTURE);
    }
    // resist
    else {
        act("$n starts to doze off, but resists.", true, vict, NULL, NULL, TO_ROOM);
        send_to_char(vict, "You start to doze off, but resist.\r\n");
    }

    WAIT_STATE(ch, 2 RL_SEC);
}

#undef NUM_TOPICS

ACMD(do_evaluate)
{
    struct creature *vict = NULL;
    int delta, cost;

    if (CHECK_SKILL(ch, SKILL_EVALUATE) < 30) {
        send_to_char(ch, "Your evaluation abilities are too weak.\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Evaluate who?\r\n");
        return;
    }

    if (!(vict = check_char_room_vis(ch, argument)))
        return;

    cost = (GET_LEVEL(vict) / 4) + GET_REMORT_GEN(vict);

    if (GET_MOVE(ch) < cost) {
        send_to_char(ch, "You don't have the %d move points needed.\r\n",
            cost);
        return;
    }

    GET_MOVE(ch) -= cost;

    delta = (cost + 126 - CHECK_SKILL(ch, SKILL_EVALUATE) - GET_INT(ch));
    delta = MAX(5, delta);

    send_to_char(ch, "%s appears to have about %d hitpoints.\r\n",
        GET_NAME(vict), GET_HIT(vict) + number(-delta, delta));

    if (delta < 100)
        gain_skill_prof(ch, SKILL_EVALUATE);

}

void
add_rad_sickness(struct creature *ch, int level)
{

    struct affected_type *af = NULL, newaff;

    // dangerous code, here, but hey
    if ((af = affected_by_spell(ch, TYPE_RAD_SICKNESS))) {
        af->duration = MIN(af->duration + 1, 100);
        af->level = MIN(af->level + 1, 50); // important -- level max must be 50
        return;
    }
    // add a new effect
    newaff.type = TYPE_RAD_SICKNESS;
    newaff.is_instant = 0;
    newaff.duration = MIN(level, 100);
    newaff.modifier = -(level / 16);
    newaff.location = APPLY_CON;
    newaff.level = level;
    newaff.bitvector = 0;
    newaff.aff_index = 0;
    newaff.owner = 0;
    newaff.next = NULL;

    affect_to_char(ch, &newaff);

}

ASPELL(spell_nuclear_wasteland)
{
    struct room_affect_data rm_aff;

    if (ROOM_FLAGGED(ch->in_room, ROOM_RADIOACTIVE)) {
        send_to_char(ch, "This room is already radioactive.\r\n");
        return;
    }

    send_to_room
        ("A blinding flash heralds the beginning of a local nuclear winter!\r\n",
        ch->in_room);

    rm_aff.description =
        strdup
        ("   This area bears a strong resemblance to a radioactive wasteland.\r\n");
    rm_aff.level = level;
    rm_aff.type = RM_AFF_FLAGS;
    rm_aff.flags = ROOM_RADIOACTIVE;
    rm_aff.duration = level;
    affect_to_room(ch->in_room, &rm_aff);
}

void
push_imprint(struct creature *ch, int max)
{

    int tmp[MAX_IMPRINT_ROOMS];
    int i;

    memcpy(tmp, ch->player_specials->imprint_rooms,
        sizeof(int) * MAX_IMPRINT_ROOMS);

    GET_IMPRINT_ROOM(ch, 0) = ch->in_room->number;

    memcpy(ch->player_specials->imprint_rooms + 1, tmp,
        sizeof(int) * (MAX_IMPRINT_ROOMS - 1));

    for (i = max; i < MAX_IMPRINT_ROOMS; i++)
        GET_IMPRINT_ROOM(ch, i) = -1;
}

int
pop_imprint(struct creature *ch)
{
    int i = 0;
    int ret = -1;

    for (i = MAX_IMPRINT_ROOMS - 1; i >= 0; i--) {
        // valid room number
        if (GET_IMPRINT_ROOM(ch, i) >= 0) {
            ret = GET_IMPRINT_ROOM(ch, i);
            // invalidate it
            GET_IMPRINT_ROOM(ch, i) = -1;
            break;
        }
    }

    return (ret);
}

ASPELL(spell_spacetime_imprint)
{

    int max = MIN(MAX_IMPRINT_ROOMS, (GET_LEVEL(ch) / 10));

    if (ROOM_FLAGGED(ch->in_room, ROOM_NORECALL | ROOM_NOPHYSIC | ROOM_NOTEL)) {
        send_to_char(ch,
            "You cannot make a spacetime imprint in this place.\r\n");
        return;
    }

    push_imprint(ch, max);
    act("You feel a strange sensation, which quickly passes.", false, ch, NULL, NULL,
        TO_ROOM);
    send_to_char(ch, "A spacetime imprint has been made of this place.\r\n");
}

ASPELL(spell_quantum_rift)
{
    int rnum;
    struct room_data *room = NULL;
    struct obj_data *rift = NULL;
    struct obj_data *o = NULL;
    rnum = pop_imprint(ch);

    if (rnum < 0) {
        // Change this to open a REALLY random portal.
        // Include DT's in this room list.
        send_to_char(ch,
            "You do not have any outstanding spacetime imprints in effect.\r\n");
        return;
    }

    if (!(room = real_room(rnum))) {
        send_to_char(ch,
            "The imprinted location you have requested no longer exists!\r\n");
        return;
    }

    if (ROOM_FLAGGED(room, ROOM_NORECALL | ROOM_NOPHYSIC | ROOM_NOTEL)) {
        send_to_char(ch,
            "You are unable to open the rift into that place.\r\n");
        return;
    }
    struct obj_data *next_o;
    for (o = object_list; o; o = next_o) {
        next_o = o->next;
        if (GET_OBJ_VNUM(o) == QUANTUM_RIFT_VNUM
            && GET_OBJ_VAL(o, 2) == GET_IDNUM(ch)
            && !o->in_room->people) {
            act("$p collapses in on itself.", true, NULL, o, NULL, TO_CHAR);
            act("$p collapses in on itself.", true, NULL, o, NULL, TO_ROOM);
            extract_obj(o);
        }
    }
    // Quantum Rift
    if ((rift = read_object(QUANTUM_RIFT_VNUM))) {
        GET_OBJ_TIMER(rift) = (int)(GET_REMORT_GEN(ch) / 2);
        if (!IS_NPC(ch)) {
            GET_OBJ_VAL(rift, 2) = GET_IDNUM(ch);
        }
        obj_to_room(rift, ch->in_room);
        // Set the target room number.
        // Note: Add in some random change of going to the wrong place.
        GET_OBJ_VAL(rift, 0) = rnum;

        act("$n shreds the fabric of space and time creating $p!",
            true, ch, rift, NULL, TO_ROOM);
        act("You shred the fabric of space and time creating $p!",
            true, ch, rift, NULL, TO_CHAR);
    } else {
        send_to_char(ch,
            "The rift has failed to form.  Something is terribly wrong.\r\n");
        return;
    }
    WAIT_STATE(ch, 2 RL_SEC);
}

ASPELL(spell_spacetime_recall)
{
    int rnum;
    struct room_data *room = NULL;

    rnum = pop_imprint(ch);

    if (rnum < 0) {
        send_to_char(ch,
            "You do not have any outstanding spacetime imprints in effect.\r\n");
        return;
    }

    if (!(room = real_room(rnum))) {
        send_to_char(ch,
            "The imprinted location you have requested no longer exists!\r\n");
        return;
    }

    if (ROOM_FLAGGED(room, ROOM_NORECALL | ROOM_NOPHYSIC | ROOM_NOTEL)) {
        send_to_char(ch,
            "You are unable to make the transition into that place.\r\n");
        return;
    }

    if (ROOM_FLAGGED(room, ROOM_HOUSE) && !can_enter_house(ch, room->number)) {
        send_to_char(ch, "You are unable to enter that place.\r\n");
        return;
    }

    act("$n fades from view and disappears.", true, ch, NULL, NULL, TO_ROOM);
    char_from_room(ch, true);
    send_to_char(ch, "You shift through space and time:\r\n");
    char_to_room(ch, room, true);
    look_at_room(ch, room, 0);
    act("$n fades into view from some other place and time.", true, ch, NULL, NULL,
        TO_ROOM);

}

int
boot_timewarp_data(void)
{
    FILE *fl;
    timewarp_data elem, *newlist = NULL;

    if (!(fl = fopen(TIMEWARP_FILE, "r"))) {
        errlog("unable to open timewarp file.");
        return 1;
    }

    if (timewarp_list) {
        free(timewarp_list);
        timewarp_list = NULL;
    }

    num_timewarp_data = 0;

    while (fscanf(fl, "%d %d", &elem.from, &elem.to) == 2) {
        num_timewarp_data++;

        newlist =
            (timewarp_data *) realloc(timewarp_list,
            num_timewarp_data * sizeof(timewarp_data));

        newlist[num_timewarp_data - 1].from = elem.from;
        newlist[num_timewarp_data - 1].to = elem.to;

        timewarp_list = newlist;
    }

    slog("Timewarp data booted, %d elements read.", num_timewarp_data);
    fclose(fl);

    return 0;
}

void
show_timewarps(struct creature *ch)
{
    acc_string_clear();
    acc_strcat("Timewarp data:\r\n", NULL);
    int i;
    for (i = 0; i < num_timewarp_data; i++) {
        struct zone_data *to_zn = real_zone(timewarp_list[i].from);
        struct zone_data *from_zn = real_zone(timewarp_list[i].to);

        acc_sprintf("  %3d.]  %s%25s%s [%s%3d%s] --> [%s%3d%s] %s%s%s\r\n",
            i,
            CCCYN(ch, C_NRM), (from_zn) ? from_zn->name : "FROM-ERROR",
            CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            timewarp_list[i].from,
            CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            timewarp_list[i].to, CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
            (to_zn) ? to_zn->name : "TO-ERROR", CCNRM(ch, C_NRM));
    }

    acc_sprintf("  %d total.\r\n", num_timewarp_data);
    page_string(ch->desc, acc_get_string());
}

// function to find the matchint zone in the timewarp info
struct zone_data *
timewarp_target(struct zone_data *zSrc)
{
    int i;

    for (i = 0; i < num_timewarp_data; i++)
        if (zSrc->number == timewarp_list[i].from)
            return (real_zone(timewarp_list[i].to));

    for (i = 0; i < num_timewarp_data; i++)
        if (zSrc->number == timewarp_list[i].to)
            return (real_zone(timewarp_list[i].from));

    return NULL;
}

bool
room_tele_ok(struct creature *ch, struct room_data *room)
{

    if (ROOM_FLAGGED(room, ROOM_NORECALL | ROOM_NOPHYSIC | ROOM_NOTEL))
        return 0;

    if (ROOM_FLAGGED(room, ROOM_HOUSE) && !can_enter_house(ch, room->number))
        return 0;

    if (ROOM_FLAGGED(room, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GRGOD)
        return 0;

    if (ROOM_FLAGGED(room, ROOM_DEATH))
        return 0;

    if (!IS_APPR(room->zone))
        return 0;

    return 1;
}

struct room_data *
random_room(struct creature *ch, struct zone_data *zone)
{

    struct room_data *room;
    int num_rooms = 0;

    if (!zone)
        return (NULL);

    for (room = zone->world; room; room = room->next)
        if (room_tele_ok(ch, room))
            num_rooms++;

    for (room = zone->world; room; room = room->next)
        if (room_tele_ok(ch, room))
            if (!number(0, --num_rooms))
                return (room);

    return (NULL);
}

// tmode == true, choose other time frame
bool
zone_tele_ok(struct creature *ch, struct zone_data *zone, int tmode)
{

    if (!IS_APPR(ch->in_room->zone) || !IS_APPR(zone))
        return 0;

    if ((ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED) ||
            ZONE_FLAGGED(zone, ZONE_ISOLATED))
        && zone != ch->in_room->zone)
        return 0;

    if (zone->plane != ch->in_room->zone->plane)
        return 0;

    if (zone->time_frame == ch->in_room->zone->time_frame) {
        if (tmode)
            return 0;
    } else if (!tmode)
        return 0;

    return 1;
}

// choose a random teleportable zone.  mode == true means only choose other times (timewarp)
struct zone_data *
random_zone(struct creature *ch, int mode)
{
    struct zone_data *zone = NULL;
    int num_zones = 0;

    for (zone = zone_table; zone; zone = zone->next)
        if (zone_tele_ok(ch, zone, mode))
            num_zones++;

    for (zone = zone_table; zone; zone = zone->next)
        if (zone_tele_ok(ch, zone, mode))
            if (!number(0, --num_zones))
                return (zone);

    return (NULL);
}

ASPELL(spell_time_warp)
{

    struct zone_data *zone = NULL, *oldzone = ch->in_room->zone;
    struct room_data *to_room = NULL;
    int i;

    // determine the target zone
    if ((zone = timewarp_target(ch->in_room->zone)))
        to_room = random_room(ch, zone);

    // if no room we need a random zone to choose from
    if (!to_room) {

        // try up to 3 random zones
        for (i = 0; i < 3 && !to_room; i++) {

            zone = random_zone(ch, 1);

            // find the target room in the random zone
            if ((to_room = random_room(ch, zone)))
                break;

        }

        // we still couldnt find a suitable room, abort
        if (!to_room) {
            send_to_char(ch,
                "You were unable to link to a cross-time room.\r\n");
            return;
        }

    }
    // do the player transferral
    act(tmp_sprintf("$n fades silently into the %s.",
            zone->time_frame == TIME_ELECTRO ? "future" :
            zone->time_frame == TIME_MODRIAN ? "past" : "unknown"),
        true, ch, NULL, NULL, TO_ROOM);

    send_to_char(ch, "You fade silently into the %s:\r\n",
        zone->time_frame == TIME_ELECTRO ? "future" :
        zone->time_frame == TIME_MODRIAN ? "past" : "unknown");

    char_from_room(ch, true);
    char_to_room(ch, to_room, true);
    look_at_room(ch, to_room, 0);

    act(tmp_sprintf("$n fades silently in from the %s.",
            oldzone->time_frame == TIME_ELECTRO ? "future" :
            oldzone->time_frame == TIME_MODRIAN ? "past" : "unknown"),
        true, ch, NULL, NULL, TO_ROOM);
}

//
// econvert is essentially a raw matter-to-energy conversion, although
// energy can be extracted via chemical reactions as well, thus a bomb
// or battery yields more energy than an equivalently massive stone
//

int
recurs_econvert_points(struct obj_data *obj, bool top)
{

    if (!obj)
        return 0;

    int num_points = GET_OBJ_WEIGHT(obj);

    switch (GET_OBJ_TYPE(obj)) {
        // double points for money
    case ITEM_MONEY:
        num_points *= 2;
        break;
        // batteries and devices get stored energy tacked on
    case ITEM_DEVICE:
    case ITEM_BATTERY:
    case ITEM_ENERGY_CELL:
    case ITEM_ENGINE:
        num_points += CUR_ENERGY(obj);
        break;
        // bombs add 8x bomb power
    case ITEM_BOMB:
        num_points += BOMB_POWER(obj) * 8;
        break;
    }

    return (num_points +
        recurs_econvert_points(obj->contains, false) +
        (top ? 0 : recurs_econvert_points(obj->next_content, false)));
}

ACMD(do_econvert)
{

    struct obj_data *obj = NULL;
    struct obj_data *battery = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
        arg3[MAX_INPUT_LENGTH];

    int num_points = 0;
    int i;
    int wait;

    if (CHECK_SKILL(ch, SKILL_ENERGY_CONVERSION) < 40) {
        send_to_char(ch,
            "You have not been trained in the science of matter conversion.\r\n");
        return;
    }

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Usage: econvert <object> [battery]\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s '%s' to convert.\r\n",
            AN(arg1), arg1);
        return;
    }

    if (IS_CORPSE(obj) && CORPSE_IDNUM(obj) > 0 && obj->contains &&
        !is_authorized(ch, DESTROY_CORPSES, NULL)) {
        send_to_char(ch,
            "You can't econvert a player's corpse while it still has objects in it.");
        return;
    }
    // check for a battery to store in
    if (*arg2) {
        argument = one_argument(argument, arg3);
        // Is it an internal energy destination?
        if (!strcmp(arg2, "internal") && *arg3 &&
            (battery = get_object_in_equip_vis(ch, arg3, ch->implants, &i))) {

        } else if ((battery =
                get_object_in_equip_vis(ch, arg2, ch->equipment, &i))
            || (battery = get_obj_in_list_vis(ch, arg2, ch->carrying))) {

        } else {
            send_to_char(ch,
                "You don't seem to have %s '%s' to store the energy in.\r\n",
                AN(arg2), arg2);
            return;
        }

        if (!IS_BATTERY(battery)) {
            act("Sorry, $p is not a battery.", false, ch, battery, NULL, TO_CHAR);
            return;
        }
    }
    // how many points is this worth?
    num_points = recurs_econvert_points(obj, true);

    // adjust it for skill level (  x ( skill lev / 100 ) )
    num_points = (num_points * CHECK_SKILL(ch, SKILL_ENERGY_CONVERSION)) / 100;
    // adjust it for exceptional intelligence ( int x 2 )
    num_points += (GET_INT(ch) - 18) * 2;
    // adjust it for secondary phys
    if (GET_CLASS(ch) != CLASS_PHYSIC)
        num_points /= 2;

    // adjust it for remort gen ( gen x 2 )
    // num_points += GET_REMORT_GEN( ch ) * 2;

    act("E=mc^2.... Voila!  You convert $p into raw energy.", false, ch, obj,
        NULL, TO_CHAR);
    act("With a flash, $n converts $p into raw energy.", false, ch, obj, NULL,
        TO_ROOM);

    extract_obj(obj);

    if (battery) {
        if (CUR_ENERGY(battery) >= MAX_ENERGY(battery)) {
            act("The newly converted energy dissipates into the void, because\r\n" "$p is already maxed out.", false, ch, battery, NULL, TO_CHAR);
            return;
        }

        num_points = MIN(num_points,
            MAX_ENERGY(battery) - CUR_ENERGY(battery));
        CUR_ENERGY(battery) += num_points;
        act(tmp_sprintf
            ("You have increased $p's energy level by %d to %d units.",
                num_points, CUR_ENERGY(battery)), false, ch, battery, NULL,
            TO_CHAR);

        if (num_points > number(50, 300))
            gain_skill_prof(ch, SKILL_ENERGY_CONVERSION);
        return;
    }

    if (GET_MANA(ch) >= GET_MAX_MANA(ch)) {
        act("The newly converted energy dissipates into the void, because\r\n"
            "your mana is already maxed out.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }

    num_points = MIN(num_points, GET_MAX_MANA(ch) - GET_MANA(ch));
    num_points = MAX(num_points, 0);
    GET_MANA(ch) += num_points;

    send_to_char(ch, "You have increased your mana level by %d to %d.\r\n",
        num_points, GET_MANA(ch));

    // Wait state.  1 + number of points / skill
    wait =
        (1 + (num_points / CHECK_SKILL(ch, SKILL_ENERGY_CONVERSION))) RL_SEC;
    WAIT_STATE(ch, wait);

    if (num_points > number(50, 300))
        gain_skill_prof(ch, SKILL_ENERGY_CONVERSION);

    return;
}

void
do_deactivate_device(struct obj_data *obj)
{
    if (obj->worn_by) {
        // unapply object affects if the object is being worn
        apply_object_affects(obj->worn_by, obj, false);
        ENGINE_STATE(obj) = 0;
        affect_total(obj->worn_by);
    } else {
        // no object affects, so just deactivate
        ENGINE_STATE(obj) = 0;
    }
}

void
do_emp_pulse_olist(struct obj_data *list, struct creature *ch,
    struct creature *vict)
{
    struct obj_data *o;
    for (o = list; o; o = o->next_content) {
        if ((IS_DEVICE(o) || IS_COMMUNICATOR(o))
            && !random_fractional_3()) {
            if (GET_OBJ_VAL(o, 2) == 1) {
                do_deactivate_device(o);
                if (vict)
                    act("$p shuts off. (carried)", false, ch, o, vict,
                        TO_VICT);
                else
                    act("$p shuts off.", false, ch, o, vict, TO_ROOM);
            }
        }
    }
}

void
do_emp_pulse_eq(struct obj_data *list[], struct creature *ch,
    struct creature *vict, int internal)
{
    int i;
    for (i = 0; i < NUM_WEAR_FLAGS; i++) {
        if (list[i] && (IS_DEVICE(list[i]) || IS_COMMUNICATOR(list[i]))
            && !random_fractional_3()) {
            if (GET_OBJ_VAL(list[i], 2) == 1) {
                do_deactivate_device(list[i]);
                act(tmp_sprintf("$p shuts off. %s",
                        internal ? "(internal)" : "(worn)"),
                    false, ch, list[i], vict, TO_VICT);
            }
        }
    }
}

void
do_emp_pulse_char(struct creature *ch, struct creature *vict)
{
    struct affected_type *af = NULL;
    int removed = 0;

    if (AFF3_FLAGGED(vict, AFF3_EMP_SHIELD) && !random_fractional_5()) {
        send_to_char(vict, "Your emp shielding stops the pulse!\r\n");
        return;
    }
    // Put a saving throw in here!!!
    if (IS_CYBORG(vict)) {
        for (af = vict->affected; af; af = af ? af->next : NULL) {
            if (SPELL_IS_PROGRAM(af->type)
                && !mag_savingthrow(vict, GET_LEVEL(ch), SAVING_CHEM)) {
                affect_remove(vict, af);
                af = vict->affected;
                removed++;
            }
        }
        if (removed > 0) {
            send_to_char(vict,
                "ERROR: Excessive electromagnetic interference! Some systems failing!\r\n");
            act("$N twitches and begins to smoke.", false, ch, NULL, vict,
                TO_NOTVICT);
        }
    }
    do_emp_pulse_eq(vict->equipment, ch, vict, 0);
    if (!IS_CYBORG(vict) || !mag_savingthrow(vict, GET_LEVEL(ch), SAVING_CHEM))
        do_emp_pulse_eq(vict->implants, ch, vict, 1);
    do_emp_pulse_olist(vict->carrying, ch, vict);
}

gint
invalid_emp_target(struct creature *tch, gpointer ch)
{
    if (tch != ch && !ok_to_attack(ch, tch, true))
        return 0;
    return -1;
}

    // Shuts off devices, communicators
// deactivats all cyborg programs
// blocked by emp shield
ASPELL(spell_emp_pulse)
{

    if (ch->in_room == NULL)
        return;
    if (ROOM_FLAGGED(ch->in_room, ROOM_NOPHYSIC)) {
        send_to_char(ch,
            "You are unable to alter physical reality in this space.\r\n");
        return;
    }
    // Make sure non-pkillers don't get killer flags.
    if (g_list_find_custom(ch->in_room->people, ch,
            (GCompareFunc) invalid_emp_target))
        return;

    send_to_room("An electromagnetic pulse jolts the room!\r\n", ch->in_room);

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (tch != ch && GET_LEVEL(tch) < LVL_IMMORT)
            do_emp_pulse_char(ch, tch);
    }

    if (ch->in_room->contents)
        do_emp_pulse_olist(ch->in_room->contents, ch, NULL);
    return;
}

ASPELL(spell_area_stasis)
{

    struct room_affect_data rm_aff;
    struct obj_data *o;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOPHYSIC)) {
        send_to_char(ch,
            "The surrounding construct of spacetime is already stable.\r\n");
        return;
    }

    send_to_room("A shockwave rips through the fabric of time and space!\r\n",
        ch->in_room);

    // Destroy all quantum rifts.
    for (o = ch->in_room->contents; o; o = o->next) {
        if (GET_OBJ_VNUM(o) == QUANTUM_RIFT_VNUM) {
            if (o->in_room->people) {
                act("$p collapses in on itself.",
                    true, g_list_nth_data(o->in_room->people, 0),
                    o, NULL, TO_NOTVICT);
            }
            extract_obj(o);
        }
    }

    rm_aff.description =
        strdup("    The room seems to be physically stable.\r\n");
    rm_aff.level = skill_bonus(ch, SPELL_AREA_STASIS);
    rm_aff.type = RM_AFF_FLAGS;
    rm_aff.flags = ROOM_NOPHYSIC;
    rm_aff.duration = skill_bonus(ch, SPELL_AREA_STASIS);
    affect_to_room(ch->in_room, &rm_aff);
}
