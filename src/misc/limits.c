/**************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: limits.c                      -- Part of TempusMUD
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
#include "screen.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "obj_data.h"
#include "specs.h"
#include "language.h"
#include "weather.h"
#include "prog.h"

extern struct obj_data *object_list;
extern struct room_data *world;
extern struct zone_data *zone_table;
extern int max_exp_gain;
extern int max_exp_loss;

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int
graf(int age, int race_id, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{
    if (race_id > RACE_TROLL)
        race_id = RACE_HUMAN;

    struct race *race = race_by_idnum(race_id);
    int lifespan = race->lifespan;

    if (age < (lifespan * 0.2))
        return (p0);
    else if (age <= (lifespan * 0.30))
        return (int)(p1 + (((age - (lifespan * 0.2)) * (p2 - p1)) / (lifespan * 0.2)));
    else if (age <= (lifespan * 0.55))
        return (int)(p2 + (((age - (lifespan * 0.35)) * (p3 -
                        p2)) / (lifespan * 0.2)));
    else if (age <= (lifespan * 0.75))
        return (int)(p3 + (((age - (lifespan * 0.55)) * (p4 -
                        p3)) / (lifespan * 0.2)));
    else if (age <= lifespan)
        return (int)(p4 + (((age - (lifespan * 0.75)) * (p5 -
                        p4)) / (lifespan * 0.2)));
    else
        return (p6);
}

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int
mana_gain(struct creature *ch)
{
    int gain;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_LEVEL(ch);
    } else {
        gain = graf(GET_AGE(ch), GET_RACE(ch), 4, 8, 12, 16, 12, 10, 8);

        /* Class calculations */
        gain += GET_LEVEL(ch) / 8;
    }

    /* Skill/Spell calculations */
    gain += GET_WIS(ch) / 4;

    // evil clerics get a gain bonus, up to 10%
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_EVIL(ch))
        gain -= (gain * GET_ALIGNMENT(ch)) / 10000;

    /* Position calculations    */
    switch (GET_POSITION(ch)) {
    case POS_SLEEPING:
        gain *= 2;
        break;
    case POS_RESTING:
        gain += gain / 2;    /* Divide by 2 */
        break;
    case POS_SITTING:
        gain += gain / 4;    /* Divide by 4 */
        break;
    case POS_FLYING:
        gain += gain / 2;
        break;
    }

    if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_PSYCHIC(ch) || IS_PHYSIC(ch) ||
        IS_BARD(ch))
        gain *= 2;

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
        gain /= 4;

    if (ch->in_room && ch->in_room->sector_type == SECT_DESERT &&
        !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) &&
        ch->in_room->zone->weather->sunlight == SUN_LIGHT)
        gain /= 2;

    if (AFF2_FLAGGED(ch, AFF2_MEDITATE)) {
        if (CHECK_SKILL(ch, ZEN_HEALING) > number(40, 100) ||
            (IS_NPC(ch) && IS_MONK(ch) && GET_LEVEL(ch) > 40))
            gain *= ((16 + GET_LEVEL(ch)) / 22);
        else
            gain *= 2;
    }

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
        gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
        gain /= 4;

    return (gain);
}

int
hit_gain(struct creature *ch)
/* Hitpoint gain pr. game hour */
{
    int gain;
    struct affected_type *af = NULL;

    if (IS_NPC(ch)) {
        gain = GET_LEVEL(ch);
    } else {
        gain = graf(GET_AGE(ch), GET_RACE(ch), 10, 14, 22, 34, 18, 12, 6);

        /* Class/Level calculations */
        gain += (GET_LEVEL(ch) / 8);

    }

    /* Skill/Spell calculations */
    gain += (GET_CON(ch) * 2);
    gain += (CHECK_SKILL(ch, SKILL_SPEED_HEALING) / 8);
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL))
        gain += (GET_LEVEL(ch) + CHECK_SKILL(ch, SKILL_DAMAGE_CONTROL)) / 4;

    // good clerics get a gain bonus, up to 10%
    if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch))
        gain += (gain * GET_ALIGNMENT(ch)) / 10000;

    // radiation sickness prevents healing
    if ((af = affected_by_spell(ch, TYPE_RAD_SICKNESS))) {
        if (af->level <= 50)
            gain -= (gain * af->level) / 50;    // important, divisor must be 50
    }

    /* Position calculations    */
    switch (GET_POSITION(ch)) {
    case POS_SLEEPING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 1);    /*  gain + gain  */
        else
            gain += (gain / 2);    /* gain + gain/2 */
        break;
    case POS_RESTING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 2);    /* divide by 2 */
        else
            gain += (gain / 4);    /* Divide by 4 */
        break;
    case POS_SITTING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 4);    /* divide by 4 */
        else
            gain += (gain / 8);    /* Divide by 8 */
        break;
    }

    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
        gain /= 2;

    if (IS_POISONED(ch))
        gain /= 4;
    if (IS_SICK(ch))
        gain /= 2;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
        gain /= 4;

    else if (AFF2_FLAGGED(ch, AFF2_MEDITATE))
        gain += (gain / 2);

    if (affected_by_spell(ch, SPELL_METABOLISM))
        gain += (gain / 4);

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
        gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
        gain /= 4;

    return (gain);
}

int
move_gain(struct creature *ch)
/* move gain pr. game hour */
{
    int gain;

    if (IS_NPC(ch)) {
        gain = GET_LEVEL(ch);
        if (NPC2_FLAGGED(ch, NPC2_MOUNT))
            gain *= 2;
    } else
        gain = graf(GET_AGE(ch), GET_RACE(ch), 18, 22, 26, 22, 18, 14, 12);

    /* Class/Level calculations */
    if (IS_RANGER(ch))
        gain += (gain / 4);
    gain += (GET_LEVEL(ch) / 8);

    /* Skill/Spell calculations */
    gain += (GET_CON(ch) / 4);

    /* Position calculations    */
    switch (GET_POSITION(ch)) {
    case POS_SLEEPING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 1);    /* divide by 1 */
        else
            gain += (gain / 2);    /* Divide by 2 */
        break;
    case POS_RESTING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 2);    /* divide by 2 */
        else
            gain += (gain / 4);    /* Divide by 4 */
        break;
    case POS_SITTING:
        if (AFF_FLAGGED(ch, AFF_REJUV))
            gain += (gain / 4);    /* divide by 4 */
        else
            gain += (gain / 8);    /* Divide by 8 */
        break;
    }

    if (IS_POISONED(ch))
        gain /= 4;
    if (IS_SICK(ch))
        gain /= 2;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
        gain /= 4;

    if (AFF2_FLAGGED(ch, AFF2_MEDITATE))
        gain += (gain / 2);

    if (IS_REMORT(ch) && GET_REMORT_GEN(ch))
        gain += (GET_REMORT_GEN(ch) * gain) / 3;

    if (IS_NEUTRAL(ch) && (IS_KNIGHT(ch) || IS_CLERIC(ch)))
        gain /= 4;

    return (gain);
}

void
set_title(struct creature *ch, const char *title)
{
    if (title == NULL) {
        title = strdup("");
    }

    if (strlen(title) > MAX_TITLE_LENGTH)
        title = tmp_substr(title, 0, MAX_TITLE_LENGTH);

    free(GET_TITLE(ch));

    while (*title && ' ' == *title)
        title++;
    GET_TITLE(ch) = (char *)malloc(strlen(title) + 2);
    if (*title) {
        if (!strncmp(title, "'s", 2)) {
            strcpy(GET_TITLE(ch), title);
        } else {
            strcpy(GET_TITLE(ch), " ");
            strcat(GET_TITLE(ch), title);
        }
    } else
        *GET_TITLE(ch) = '\0';
}

void
gain_exp(struct creature *ch, int gain)
{
    int is_altered = false;
    int num_levels = 0;

    if (ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
        return;

    if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_AMBASSADOR)))
        return;

    if (IS_PC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (IS_NPC(ch)) {
        GET_EXP(ch) = MIN(2000000000,
            ((unsigned int)GET_EXP(ch) + (unsigned int)gain));
        return;
    }
    if (gain > 0) {
        gain = MIN(max_exp_gain, gain); /* put a cap on the max gain per kill */
        if ((gain + GET_EXP(ch)) > exp_scale[GET_LEVEL(ch) + 2])
            gain =
                (((exp_scale[GET_LEVEL(ch) + 2] - exp_scale[GET_LEVEL(ch) +
                            1]) / 2) + exp_scale[GET_LEVEL(ch) + 1]) -
                GET_EXP(ch);

        GET_EXP(ch) += gain;
        while (GET_LEVEL(ch) < (LVL_AMBASSADOR - 1) &&
            GET_EXP(ch) >= exp_scale[GET_LEVEL(ch) + 1]) {
            GET_LEVEL(ch) += 1;
            num_levels++;
            advance_level(ch, false);
            is_altered = true;
        }

        if (is_altered) {
            if (num_levels == 1) {
                send_to_char(ch, "You rise a level!\r\n");
                if (GET_LEVEL(ch) == LVL_CAN_RETURN + 1 &&
                    GET_REMORT_GEN(ch) == 0)
                    send_to_char(ch,
                        "%sYou are now too powerful to use the 'return' command to recall.%s\r\n",
                        CCCYN(ch, NRM), CCNRM(ch, NRM));
            } else {
                send_to_char(ch, "You rise %d levels!\r\n", num_levels);
            }
            crashsave(ch);
        }
    } else if (gain < 0) {
        gain = MAX(-max_exp_loss, gain);    /* Cap max exp lost per death */
        GET_EXP(ch) += gain;
        if (GET_EXP(ch) < 0)
            GET_EXP(ch) = 0;
    }
}

void
gain_exp_regardless(struct creature *ch, int gain)
{
    int is_altered = false;
    int num_levels = 0;

    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
        GET_EXP(ch) = 0;

    if (!IS_NPC(ch)) {
        while (GET_LEVEL(ch) < LVL_GRIMP &&
            GET_EXP(ch) >= exp_scale[GET_LEVEL(ch) + 1]) {
            GET_LEVEL(ch) += 1;
            num_levels++;
            advance_level(ch, 1);
            is_altered = true;
        }

        if (is_altered) {
            if (num_levels == 1)
                send_to_char(ch, "You rise a level!\r\n");
            else {
                send_to_char(ch, "You rise %d levels!\r\n", num_levels);
            }
        }
    }
}

void
gain_condition(struct creature *ch, int condition, int value)
{
    int intoxicated = 0;

    if (GET_COND(ch, condition) < 0 || !value)  /* No change */
        return;

    intoxicated = (GET_COND(ch, DRUNK) > 0);

    GET_COND(ch, condition) += value;

    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

    if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
        return;

    if (ch->desc && !STATE(ch->desc)) {
        switch (condition) {
        case FULL:
            if (!number(0, 3))
                send_to_char(ch, "You feel quite hungry.\r\n");
            else if (!number(0, 2))
                send_to_char(ch, "You are famished.\r\n");
            else
                send_to_char(ch, "You are hungry.\r\n");
            return;
        case THIRST:
            if (IS_VAMPIRE(ch))
                send_to_char(ch, "You feel a thirst for blood...\r\n");
            else if (!number(0, 1))
                send_to_char(ch, "Your throat is parched.\r\n");
            else
                send_to_char(ch, "You are thirsty.\r\n");
            return;
        case DRUNK:
            if (intoxicated)
                send_to_char(ch, "You are now sober.\r\n");
            return;
        default:
            break;
        }
    }
}

int
check_idling(struct creature *ch)
{
    void set_desc_state(enum cxn_state state, struct descriptor_data *d);

    ch->char_specials.timer++;

    if (ch->desc)
        ch->desc->repeat_cmd_count = 0;

    if (!PLR_FLAGGED(ch, PLR_OLC)) {
        if (ch->char_specials.timer > 10
            && ch->desc
            && STATE(ch->desc) == CXN_NETWORK) {
            send_to_char(ch, "Idle limit reached.  Connection reset by peer.\r\n");
            set_desc_state(CXN_PLAYING, ch->desc);
        } else if (ch->char_specials.timer > 60) {
            if (GET_LEVEL(ch) > 49) {
                // Immortals have their afk flag set when idle
                if (!PLR_FLAGGED(ch, PLR_AFK)) {
                    send_to_char(ch,
                        "You have been idle, and are now marked afk.\r\n");
                    SET_BIT(PLR_FLAGS(ch), PLR_AFK);
                    AFK_REASON(ch) = strdup("idle");
                }
            } else {
                if (ch->in_room != NULL)
                    char_from_room(ch, false);
                char_to_room(ch, real_room(3), true);
                if (ch->desc) {
                    close_socket(ch->desc);
                    ch->desc = NULL;
                }
                creature_idle(ch);
                return true;
            }
        }
    }
    return false;
}

void
save_all_players(void)
{
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (IS_PC(tch))
            crashsave(tch);
    }
}

/* Update PCs, NPCs, objects, and shadow zones */
void
point_update(void)
{
    void update_char_objects(struct creature *ch);  /* handler.c */
    void extract_obj(struct obj_data *obj); /* handler.c */
    register struct obj_data *j, *next_thing, *jj, *next_thing2;
    struct room_data *rm;
    struct zone_data *zone;
    int full = 0, thirst = 0, drunk = 0, z, out_of_zone = 0;

    /* shadow zones */
    for (zone = zone_table; zone; zone = zone->next) {
        if (SHADOW_ZONE(zone)) {
            if (ZONE_IS_SHADE(zone)) {
                shade_zone(NULL, zone, 0, NULL, SPECIAL_TICK);
            }
            // Add future shadow zone specs here shadow zone spec should work for all
            // shadow zones so long as you create an integer array with the number for
            // all the rooms which link the zone to the rest of the world.
        }
        /* rooms */
        for (struct room_data * room = zone->world; room; room = room->next)
            if (GET_ROOM_PROGOBJ(room))
                trigger_prog_tick(room, PROG_TYPE_ROOM);
    }

    /* characters */
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;

        full = 1;
        thirst = 1;
        drunk = 1;
        //reputation should slowly decrease when not in a house, clan, arena, or afk
        if (IS_PC(tch) &&
            !ROOM_FLAGGED(tch->in_room, ROOM_ARENA) &&
            !ROOM_FLAGGED(tch->in_room, ROOM_CLAN_HOUSE) &&
            !ROOM_FLAGGED(tch->in_room, ROOM_HOUSE) &&
            !ROOM_FLAGGED(tch->in_room, ROOM_PEACEFUL) &&
            !PLR_FLAGGED(tch, PLR_AFK) &&
            RAW_REPUTATION_OF(tch) > 1 && number(0, 153) == 0)
            gain_reputation(tch, -1);

        if (GET_POSITION(tch) >= POS_STUNNED) {
            GET_HIT(tch) = MIN(GET_HIT(tch) + hit_gain(tch), GET_MAX_HIT(tch));
            GET_MANA(tch) =
                MIN(GET_MANA(tch) + mana_gain(tch), GET_MAX_MANA(tch));
            GET_MOVE(tch) =
                MIN(GET_MOVE(tch) + move_gain(tch), GET_MAX_MOVE(tch));

            if (AFF_FLAGGED(tch, AFF_POISON) && damage(tch, tch, NULL, dice(4, 11)
                    + (affected_by_spell(tch, SPELL_METABOLISM) ? dice(4, 3) : 0),
                                                       SPELL_POISON, -1))
                continue;

            if (GET_POSITION(tch) <= POS_STUNNED)
                update_pos(tch);

            if (IS_SICK(tch)
                && affected_by_spell(tch, SPELL_SICKNESS)
                && !number(0, 4)) {
                // You lose a lot when you puke
                full = 10;
                thirst = 10;
                switch (number(0, 6)) {
                case 0:
                    act("You puke all over the place.",
                        true, tch, NULL, NULL, TO_CHAR);
                    act("$n pukes all over the place.", false, tch, NULL, NULL,
                        TO_ROOM);
                    break;
                case 1:
                    act("You vomit uncontrollably.", true, tch, NULL, NULL, TO_CHAR);
                    act("$n vomits uncontrollably.", false, tch, NULL, NULL,
                        TO_ROOM);
                    break;

                case 2:
                    act("You begin to regurgitate steaming bile.",
                        true, tch, NULL, NULL, TO_CHAR);
                    act("$n begins to regurgitate steaming bile.", false, tch,
                        NULL, NULL, TO_ROOM);
                    break;

                case 3:
                    act("You are violently overcome with a fit of dry heaving.", true, tch, NULL, NULL, TO_CHAR);
                    act("$n is violently overcome with a fit of dry heaving.",
                        false, tch, NULL, NULL, TO_ROOM);
                    break;

                case 4:
                    act("You begin to retch.", true, tch, NULL, NULL, TO_CHAR);
                    act("$n begins to retch.", false, tch, NULL, NULL, TO_ROOM);
                    break;

                case 5:
                    act("You violently eject your lunch!",
                        true, tch, NULL, NULL, TO_CHAR);
                    act("$n violently ejects $s lunch!", false, tch, NULL, NULL,
                        TO_ROOM);
                    break;

                default:
                    act("You begin an extended session of tossing your cookies.", true, tch, NULL, NULL, TO_CHAR);
                    act("$n begins an extended session of tossing $s cookies.",
                        false, tch, NULL, NULL, TO_ROOM);
                    break;
                }
            }
        } else if ((GET_POSITION(tch) == POS_INCAP
                || GET_POSITION(tch) == POS_MORTALLYW)) {
            // If they've been healed since they were incapacitated,
            //  Update their position appropriately.
            if (GET_HIT(tch) > -11) {
                if (GET_HIT(tch) > 0) {
                    GET_POSITION(tch) = POS_RESTING;
                } else if (GET_HIT(tch) > -3) {
                    GET_POSITION(tch) = POS_STUNNED;
                } else if (GET_HIT(tch) > -6) {
                    GET_POSITION(tch) = POS_INCAP;
                } else {
                    GET_POSITION(tch) = POS_MORTALLYW;
                }
            }
            // No longer shall struct creatures die of blood loss.
            // We will now heal them slowly instead.
            if (GET_POSITION(tch) == POS_INCAP)
                GET_HIT(tch) += 2;
            else if (GET_POSITION(tch) == POS_MORTALLYW)
                GET_HIT(tch) += 1;

            continue;
        }
        // progs
        if (IS_NPC(tch) && GET_NPC_PROGOBJ(tch))
            trigger_prog_tick(tch, PROG_TYPE_MOBILE);

        if (!IS_NPC(tch)) {
            update_char_objects(tch);
            if (check_idling(tch))
                continue;
        } else if (tch->char_specials.timer)
            tch->char_specials.timer -= 1;

        while (GET_LANG_HEARD(tch)) {
            int lang = GPOINTER_TO_INT(GET_LANG_HEARD(tch)->data);
            if (number(0, 600) < GET_INT(tch) + GET_WIS(tch) + GET_CHA(tch))
                SET_TONGUE(tch, lang, CHECK_TONGUE(tch, lang) + 1);
            GET_LANG_HEARD(tch) = g_list_delete_link(GET_LANG_HEARD(tch),
                GET_LANG_HEARD(tch));
        }

        if (affected_by_spell(tch, SPELL_METABOLISM))
            full += 1;

        if (IS_CYBORG(tch)) {
            if (AFF3_FLAGGED(tch, AFF3_STASIS))
                full /= 4;
            else if (GET_LEVEL(tch) > number(10, 60))
                full /= 2;
        }

        gain_condition(tch, FULL, -full);

        if (SECT_TYPE(tch->in_room) == SECT_DESERT)
            thirst += 2;
        if (ROOM_FLAGGED(tch->in_room, ROOM_FLAME_FILLED))
            thirst += 2;
        if (affected_by_spell(tch, SPELL_METABOLISM))
            thirst += 1;
        if (IS_CYBORG(tch)) {
            if (AFF3_FLAGGED(tch, AFF3_STASIS))
                thirst /= 4;
            else if (GET_LEVEL(tch) > number(10, 60))
                thirst /= 2;
        }

        gain_condition(tch, THIRST, -thirst);

        if (IS_MONK(tch))
            drunk += 1;
        if (IS_CYBORG(tch))
            drunk += 2;
        if (affected_by_spell(tch, SPELL_METABOLISM))
            drunk += 1;

        gain_condition(tch, DRUNK, -drunk);
        /* player frozen */
        if (PLR_FLAGGED(tch, PLR_FROZEN)
            && tch->player_specials->thaw_time > -1) {
            time_t now;

            now = time(NULL);

            if (now > tch->player_specials->thaw_time) {
                REMOVE_BIT(PLR_FLAGS(tch), PLR_FROZEN);
                tch->player_specials->thaw_time = 0;
                crashsave(tch);
                mudlog(MAX(LVL_POWER, GET_INVIS_LVL(tch)), BRF, true,
                    "(GC) %s un-frozen by timeout.", GET_NAME(tch));
                send_to_char(tch, "You thaw out and can move again.\r\n");
                act("$n has thawed out and can move again.",
                    false, tch, NULL, NULL, TO_ROOM);
            }
        }
    }

    /* objects */
    for (j = object_list; j; j = next_thing) {
        next_thing = j->next;   /* Next in object list */

        // First check for a tick special
        if (GET_OBJ_SPEC(j)
            && GET_OBJ_SPEC(j) (NULL, j, 0, NULL, SPECIAL_TICK))
            continue;

        /* If this is a corpse */
        if (IS_CORPSE(j)) {
            /* timer count down */
            if (GET_OBJ_TIMER(j) > 0)
                GET_OBJ_TIMER(j)--;

            if (GET_OBJ_TIMER(j) <= 0) {
                if (j->carried_by)
                    act("$p decays in your hands.", false, j->carried_by, j, NULL,
                        TO_CHAR);
                if (j->worn_by)
                    act("$p disintegrates as you are wearing it.", false,
                        j->worn_by, j, NULL, TO_CHAR);
                else if ((j->in_room != NULL) && (j->in_room->people)) {
                    const char *msg =
                        "$p decays into nothing right before your eyes.";

                    if (ROOM_FLAGGED(j->in_room, ROOM_FLAME_FILLED) ||
                        SECT_TYPE(j->in_room) == SECT_FIRE_RIVER ||
                        GET_PLANE(j->in_room) == PLANE_HELL_4 ||
                        GET_PLANE(j->in_room) == PLANE_HELL_1
                        || !number(0, 50)) {
                        msg =
                            "$p spontaneously combusts and is devoured by flames.";
                    } else if (ROOM_FLAGGED(j->in_room, ROOM_ICE_COLD)
                        || GET_PLANE(j->in_room) == PLANE_HELL_5
                        || !number(0, 250)) {
                        msg = "$p freezes and shatters into dust.";
                    } else if (GET_PLANE(j->in_room) == PLANE_ASTRAL
                        || !number(0, 250)) {
                        msg = "A sudden psychic wind rips through $p.";
                    } else if (GET_TIME_FRAME(j->in_room) == TIME_TIMELESS
                        || !number(0, 250)) {
                        msg =
                            "$p is pulled into a timeless void and nullified.";
                    } else if (SECT_TYPE(j->in_room) == SECT_WATER_SWIM
                        || SECT_TYPE(j->in_room) == SECT_WATER_NOSWIM) {
                        msg = "$p sinks beneath the surface and is gone.";
                    } else if (IS_METAL_TYPE(j)
                        || IS_GLASS_TYPE(j)
                        || IS_WOOD_TYPE(j)
                        || IS_PLASTIC_TYPE(j)) {
                        msg = "$p disintegrates before your eyes.";
                    } else if (IS_STONE_TYPE(j)
                        || GET_OBJ_MATERIAL(j) == MAT_BONE) {
                        msg = "$p disintegrates into dust and is blown away.";
                    } else if (room_is_underwater(j->in_room)) {
                        msg = "A school of small fish appears and devours $p.";
                    } else if (!ROOM_FLAGGED(j->in_room, ROOM_INDOORS)
                        && !number(0, 2)) {
                        msg = "A flock of carrion birds hungrily devours $p.";
                    } else if (number(0, 3)) {
                        msg = "A quivering horde of maggots consumes $p.";
                    } else {
                        msg = "$p decays into nothing before your eyes.";
                    }

                    act(msg, true, NULL, j, NULL, TO_ROOM);
                    act(msg, true, NULL, j, NULL, TO_CHAR);
                }

                for (jj = j->contains; jj; jj = next_thing2) {
                    next_thing2 = jj->next_content; /* Next in inventory */
                    obj_from_obj(jj);

                    if (j->in_obj)
                        obj_to_obj(jj, j->in_obj);
                    else if (j->carried_by)
                        obj_to_char(jj, j->carried_by);
                    else if (j->worn_by)
                        obj_to_char(jj, j->worn_by);
                    else if (j->in_room != NULL)
                        obj_to_room(jj, j->in_room);
                    else
                        raise(SIGSEGV);

                    if (IS_IMPLANT(jj) && !CAN_WEAR(jj, ITEM_WEAR_TAKE)) {

                        SET_BIT(jj->obj_flags.wear_flags, ITEM_WEAR_TAKE);

                        if (!IS_OBJ_TYPE(jj, ITEM_ARMOR))
                            damage_eq(NULL, jj, (GET_OBJ_DAM(jj) / 4), -1);
                        else
                            damage_eq(NULL, jj, (GET_OBJ_DAM(jj) / 2), -1);
                    }
                }
                extract_obj(j);
            }
        } else if (GET_OBJ_VNUM(j) < 0 &&
            ((IS_OBJ_TYPE(j, ITEM_DRINKCON) && isname("head", j->aliases)) ||
                ((IS_OBJ_TYPE(j, ITEM_WEAPON) && isname("leg", j->aliases)) &&
                    (j->worn_on != WEAR_WIELD && j->worn_on != WEAR_WIELD_2))
                || (IS_OBJ_TYPE(j, ITEM_FOOD) && isname("heart", j->aliases)))) {
            // body parts
            if (GET_OBJ_TIMER(j) > 0)
                GET_OBJ_TIMER(j)--;
            if (GET_OBJ_TIMER(j) == 0) {
                if (j->carried_by)
                    act("$p collapses into mush in your hands.",
                        false, j->carried_by, j, NULL, TO_CHAR);
                else if ((j->in_room != NULL) && (j->in_room->people)) {
                    act("$p collapses into nothing.",
                        true, NULL, j, NULL, TO_ROOM);
                    act("$p collapses into nothing.",
                        true, NULL, j, NULL, TO_CHAR);
                }
                // drop out the (damaged) implants
                for (jj = j->contains; jj; jj = next_thing2) {
                    next_thing2 = jj->next_content; /* Next in inventory */
                    obj_from_obj(jj);

                    if (j->carried_by)
                        obj_to_char(jj, j->carried_by);
                    else if (j->worn_by)
                        obj_to_char(jj, j->worn_by);
                    else if (j->in_obj)
                        obj_to_obj(jj, j->in_obj);
                    else if (j->in_room)
                        obj_to_room(jj, j->in_room);
                    else
                        raise(SIGSEGV);

                    // fix up the implants, and damage them
                    if (IS_IMPLANT(jj) && !CAN_WEAR(jj, ITEM_WEAR_TAKE)) {
                        SET_BIT(jj->obj_flags.wear_flags, ITEM_WEAR_TAKE);
                        if (!IS_OBJ_TYPE(jj, ITEM_ARMOR))
                            damage_eq(NULL, jj, (GET_OBJ_DAM(jj) / 2), -1);
                        else
                            damage_eq(NULL, jj, (GET_OBJ_DAM(jj) / 4), -1);
                    }
                }
                extract_obj(j);
            }
        } else if (GET_OBJ_VNUM(j) == BLOOD_VNUM) { /* blood pools */
            GET_OBJ_TIMER(j)--;
            if (GET_OBJ_TIMER(j) <= 0) {
                extract_obj(j);
            }
        } else if (GET_OBJ_VNUM(j) == QUANTUM_RIFT_VNUM) {
            GET_OBJ_TIMER(j)--;
            if (GET_OBJ_TIMER(j) <= 0) {
                if (j->action_desc && j->in_room) {
                    act("$p collapses in on itself.",
                        true, NULL, j, NULL, TO_CHAR);
                    act("$p collapses in on itself.",
                        true, NULL, j, NULL, TO_ROOM);
                }
                extract_obj(j);
            }
        } else if (GET_OBJ_VNUM(j) == ICE_VNUM) {
            if (j->in_room) {
                if (SECT_TYPE(j->in_room) == SECT_DESERT ||
                    SECT_TYPE(j->in_room) == SECT_FIRE_RIVER ||
                    SECT_TYPE(j->in_room) == SECT_PITCH_PIT ||
                    SECT_TYPE(j->in_room) == SECT_PITCH_SUB ||
                    SECT_TYPE(j->in_room) == SECT_ELEMENTAL_FIRE) {
                    GET_OBJ_TIMER(j) = 0;
                }

                if (!ROOM_FLAGGED(j->in_room, ROOM_ICE_COLD)) {
                    GET_OBJ_TIMER(j)--;
                }

                if (GET_OBJ_TIMER(j) <= 0) {
                    act("$p melts and is gone.", true, NULL, j, NULL, TO_ROOM);
                    extract_obj(j);
                }
            }
        } else if (IS_OBJ_STAT2(j, ITEM2_UNAPPROVED) ||
            (IS_OBJ_TYPE(j, ITEM_KEY) && GET_OBJ_TIMER(j)) ||
            (GET_OBJ_SPEC(j) == fate_portal) ||
            (IS_OBJ_STAT3(j, ITEM3_STAY_ZONE))) {

            // keys, unapp, zone only objects && fate portals
            if (IS_OBJ_TYPE(j, ITEM_KEY)) { // skip keys still in zone
                z = zone_number(GET_OBJ_VNUM(j));
                if ((rm = where_obj(j)) && rm->zone->number == z) {
                    continue;
                }
            }

            if (IS_OBJ_STAT3(j, ITEM3_STAY_ZONE)) {
                z = zone_number(GET_OBJ_VNUM(j));
                if ((rm = where_obj(j)) && rm->zone->number != z) {
                    out_of_zone = 1;
                }
            }

            /* timer count down */
            if (GET_OBJ_TIMER(j) > 0)
                GET_OBJ_TIMER(j)--;

            if (!GET_OBJ_TIMER(j) || out_of_zone) {

                if (j->carried_by)
                    act("$p slowly fades out of existence.",
                        false, j->carried_by, j, NULL, TO_CHAR);
                if (j->worn_by)
                    act("$p disintegrates as you are wearing it.",
                        false, j->worn_by, j, NULL, TO_CHAR);
                else if ((j->in_room != NULL) && (j->in_room->people)) {
                    act("$p slowly fades out of existence.",
                        true, NULL, j, NULL, TO_ROOM);
                    act("$p slowly fades out of existence.",
                        true, NULL, j, NULL, TO_CHAR);
                }
                for (jj = j->contains; jj; jj = next_thing2) {
                    next_thing2 = jj->next_content; /* Next in inventory */
                    obj_from_obj(jj);

                    if (j->in_obj)
                        obj_to_obj(jj, j->in_obj);
                    else if (j->carried_by)
                        obj_to_room(jj, j->carried_by->in_room);
                    else if (j->worn_by)
                        obj_to_room(jj, j->worn_by->in_room);
                    else if (j->in_room != NULL)
                        obj_to_room(jj, j->in_room);
                    else
                        raise(SIGSEGV);
                }
                extract_obj(j);
            }
        }
    }
}
