/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spell_parser.c                      -- Part of TempusMUD
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
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "materials.h"
#include "fight.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "actions.h"
#include "language.h"
#include "prog.h"
#include "strutil.h"

char **spells = NULL;
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct bard_song songs[TOP_SPELL_DEFINE + 1];
struct room_direction_data *knock_door = NULL;
char locate_buf[256];

#define SINFO spell_info[spellnum]

extern int mini_mud;

extern struct room_data *world;
int find_door(struct creature *ch, char *type, char *dir, const char *cmdname);
void name_from_drinkcon(struct obj_data *obj);
struct obj_data *find_item_kit(struct creature *ch);
int perform_taint_burn(struct creature *ch, int spellnum);
int max_spell_num = 0;

int
mag_manacost(struct creature *ch, int spellnum)
{
    int mana, mana2;

    mana = MAX(SINFO.mana_max -
        (SINFO.mana_change *
            (GET_LEVEL(ch) -
                SINFO.min_level[(int)MIN(NUM_CLASSES - 1, GET_CLASS(ch))])),
        SINFO.mana_min);

    if (GET_REMORT_CLASS(ch) >= 0) {
        mana2 = MAX(SINFO.mana_max -
            (SINFO.mana_change *
                (GET_LEVEL(ch) -
                    SINFO.min_level[(int)MIN(NUM_CLASSES - 1,
                            GET_REMORT_CLASS(ch))])), SINFO.mana_min);
        mana = MIN(mana2, mana);
    }

    return mana;
}

bool
is_able_to_learn(struct creature * ch, int spl)
{
    // Return true if the creature ch is able to learn the spell spl
    return ((GET_REMORT_GEN(ch) >= SPELL_GEN(spl, GET_CLASS(ch))
            && GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_CLASS(ch)))
        || (IS_REMORT(ch)
            && GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_REMORT_CLASS(ch))
            && !SPELL_GEN(spl, GET_REMORT_CLASS(ch))));
}

// Returns true if there's a reasonable chance that casting the spell
// will work.  Used for NPC AI
bool
can_cast_spell(struct creature * ch, int spellnum)
{
    struct room_data *room = ch->in_room;

    return (is_able_to_learn(ch, spellnum)
        && (GET_POSITION(ch) >= SINFO.min_position)
        && GET_MANA(ch) >= mag_manacost(ch, spellnum)
        && !(SPELL_IS_EVIL(spellnum) && !IS_EVIL(ch))
        && !(SPELL_IS_GOOD(spellnum) && !IS_GOOD(ch))

        && !(SPELL_FLAGGED(spellnum, MAG_NOSUN) && !room_is_sunny(ch->in_room))
        && !(SPELL_FLAGGED(spellnum, MAG_NOWATER) && room_is_underwater(room))
        && !(SPELL_FLAGGED(spellnum, MAG_OUTDOORS)
            && ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
        && !(SPELL_IS_PSIONIC(spellnum) && ROOM_FLAGGED(room, ROOM_NOPSIONICS))
        && !(SPELL_IS_MAGIC(spellnum) && ROOM_FLAGGED(room, ROOM_NOMAGIC))
        && !(SPELL_IS_PHYSICS(spellnum) && ROOM_FLAGGED(room, ROOM_NOSCIENCE))
        && !(SPELL_USES_GRAVITY(spellnum) && NOGRAV_ZONE(room->zone)));
}

void
say_spell(struct creature *ch,
    int spellnum, struct creature *tch, struct obj_data *tobj)
{
    const char *to_char = NULL, *to_vict = NULL, *to_room = NULL;

    if (SPELL_IS_PSIONIC(spellnum)) {
        if (tobj != NULL && tobj->in_room == ch->in_room) {
            to_char = "You close your eyes and touch $p with a mental finger.";
            to_room = "$n closes $s eyes and touches $p with a mental finger.";
        } else if (tch == NULL || tch->in_room != ch->in_room) {
            to_char = "You close your eyes and slip into a psychic world.";
            to_room = "$n closes $s eyes and slips into the psychic world.";
        } else if (tch == ch) {
            to_char = "You close your eyes and concentrate.";
            to_room = "$n momentarily closes $s eyes and concentrates.";
        } else {
            to_char = "You close your eyes and touch $N with a mental finger.";
            to_vict = "$n closes $s eyes and connects with your mind.";
            to_room = "$n closes $s eyes and touches $N with a mental finger.";
        }
    } else if (SPELL_IS_PHYSICS(spellnum)) {
        if (tobj != NULL && tobj->in_room == ch->in_room) {
            to_char = "You look directly at $p and make a calculation.";
            to_room = "$n looks directly at $p and makes a calculation.";
        } else if (tch == NULL || tch->in_room != ch->in_room) {
            to_char = "You close your eyes and slip into a deep calculation.";
            to_room = "$n closes $s eyes and makes a deep calculation.";
        } else if (tch == ch) {
            to_char = "You close your eyes and make a calculation.";
            to_room = "$n momentarily closes $s eyes and concentrates.";
        } else {
            to_char = "You look at $N and make a calculation.";
            to_vict = "$n closes $s eyes and alters the reality around you.";
            to_room = "$n looks at $N and makes a calculation.";
        }
    } else {
        char *spellname =
            translate_with_tongue(find_tongue_by_idnum(TONGUE_ARCANUM),
            spell_to_str(spellnum),
            0);

        if (tobj != NULL && tobj->in_room == ch->in_room) {
            to_char = "You stare at $p and utter the words, '%s'.";
            to_room = "$n stares at $p and utters the words, '%s'.";
        } else if (tch == NULL || tch->in_room != ch->in_room) {
            to_char = "You utter the words, '%s'.";
            to_room = "$n utters the words, '%s'.";
        } else if (tch == ch) {
            to_char = "You close your eyes and utter the words, '%s'.";
            to_room = "$n closes $s eyes and utters the words, '%s'.";
        } else {
            to_char = "You stare at $N and utter, '%s'.";
            to_vict = "$n stares at you and utters, '%s'.";
            to_room = "$n stares at $N and utters, '%s'.";
        }
        if (to_char)
            to_char = tmp_sprintf(to_char, spellname);
        if (to_vict)
            to_vict = tmp_sprintf(to_vict, spellname);
        if (to_room)
            to_room = tmp_sprintf(to_room, spellname);
    }

    if (to_char)
        act(tmp_sprintf(to_char, spell_to_str(spellnum)), false,
            ch, tobj, tch, TO_CHAR);
    if (to_vict)
        act(tmp_sprintf(to_vict, spell_to_str(spellnum)), false,
            ch, tobj, tch, TO_VICT);
    if (to_room)
        act(tmp_sprintf(to_room, spell_to_str(spellnum)), false,
            ch, tobj, tch, TO_NOTVICT);
}

int
find_skill_num(char *name)
{
    int index = 0, ok;
    char *temp, *temp2;
    char spellname[256];
    char first[256], first2[256];

    while (*spell_to_str(++index) != '\n') {
        if (is_abbrev(name, spell_to_str(index)))
            return index;

        ok = 1;
        strncpy(spellname, spell_to_str(index), 255);
        temp = any_one_arg(spellname, first);
        temp2 = any_one_arg(name, first2);
        while (*first && *first2 && ok) {
            if (!is_abbrev(first2, first))
                ok = 0;
            temp = any_one_arg(temp, first);
            temp2 = any_one_arg(temp2, first2);
        }

        if (ok && !*first2)
            return index;
    }

    return -1;
}

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 *
 * returns 1 on success of casting, 0 on failure
 *
 */
int
call_magic(struct creature *caster, struct creature *cvict,
    struct obj_data *ovict, int *dvict, int spellnum, int level, int casttype)
{

    int savetype, mana = -1;
    struct affected_type *af_ptr = NULL;

    if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
        return 0;

    if (GET_ROOM_PROGOBJ(caster->in_room) != NULL) {
        if (trigger_prog_spell(caster->in_room, PROG_TYPE_ROOM, caster,
                spellnum)) {
            return 0;           //handled
        }
    }

    for (GList * it = first_living(caster->in_room->people); it; it = next_living(it)) {
        struct creature *tch = (struct creature *)it->data;
        if (GET_NPC_PROGOBJ(tch) != NULL) {
            if (trigger_prog_spell(tch, PROG_TYPE_MOBILE, caster, spellnum)) {
                return 0;
            }
        }
    }

    if (ovict) {
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOMAG)) {
            if ((SPELL_IS_MAGIC(spellnum) || SPELL_IS_DIVINE(spellnum)) &&
                spellnum != SPELL_IDENTIFY) {
                act("$p hums and shakes for a moment "
                    "then rejects your spell!", true, caster, ovict, NULL,
                    TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's spell!", true, caster, ovict, NULL,
                    TO_ROOM);
                return 0;
            }
        }
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOSCI)) {
            if (SPELL_IS_PHYSICS(spellnum)) {
                act("$p hums and shakes for a moment "
                    "then rejects your alteration!", true, caster, ovict, NULL,
                    TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's alteration!", true, caster, ovict, NULL,
                    TO_ROOM);
                return 0;
            }
        }
    }

    /* stuff to check caster vs. cvict */
    if (cvict && caster != cvict) {
        if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
            !ok_to_attack(caster, cvict, false)) {
            if (SPELL_IS_PSIONIC(spellnum)) {
                send_to_char(caster,
                    "The Universal Psyche descends on your mind and "
                    "renders you powerless!\r\n");
                act("$n concentrates for an instant, and is suddenly thrown "
                    "into mental shock!\r\n", false, caster, NULL, NULL, TO_ROOM);
                return 0;
            }
            if (SPELL_IS_PHYSICS(spellnum)) {
                send_to_char(caster,
                    "The Supernatural Reality prevents you from twisting "
                    "nature in that way!\r\n");
                act("$n attempts to violently alter reality, but is restrained " "by the whole of the universe.", false, caster, NULL, NULL, TO_ROOM);
                return 0;
            }
            if (SPELL_IS_BARD(spellnum)) {
                send_to_char(caster, "Your voice is stifled!\r\n");
                act("$n attempts to sing a violent song, but is restrained "
                    "by the whole of the universe.", false, caster, NULL, NULL,
                    TO_ROOM);
                return 0;
            } else {
                send_to_char(caster,
                    "A flash of white light fills the room, dispelling your "
                    "violent magic!\r\n");
                act("White light from no particular source suddenly fills the room, " "then vanishes.", false, caster, NULL, NULL, TO_ROOM);
                return 0;
            }
        }

        if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
            !ok_damage_vendor(caster, cvict))
            return 0;

        if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
            check_attack(caster, cvict);
            //Try to make this a little more sane...
            if (creature_distrusts(cvict, caster) &&
                AFF3_FLAGGED(cvict, AFF3_PSISHIELD) &&
                (SPELL_IS_PSIONIC(spellnum) || casttype == CAST_PSIONIC)) {
                bool failed = false;
                int prob, percent;

                if (spellnum == SPELL_PSIONIC_SHATTER &&
                    !mag_savingthrow(cvict, level, SAVING_PSI))
                    failed = true;

                prob = CHECK_SKILL(caster, spellnum) + GET_INT(caster);
                prob += skill_bonus(caster, spellnum);

                percent = skill_bonus(cvict, SPELL_PSISHIELD);
                percent += number(1, 120);

                if (mag_savingthrow(cvict, GET_LEVEL(caster), SAVING_PSI))
                    percent <<= 1;

                if (GET_INT(cvict) < GET_INT(caster))
                    percent += (GET_INT(cvict) - GET_INT(caster)) << 3;

                if (percent >= prob)
                    failed = true;

                if (failed) {
                    act("Your psychic attack is deflected by $N's psishield!",
                        false, caster, NULL, cvict, TO_CHAR);
                    act("$n's psychic attack is deflected by $N's psishield!",
                        false, caster, NULL, cvict, TO_NOTVICT);
                    act("$n's psychic attack is deflected by your psishield!",
                        false, caster, NULL, cvict, TO_VICT);
                    return 0;
                }
            }
        }
        if (creature_distrusts(cvict, caster)) {
            af_ptr = NULL;
            if (SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum))
                af_ptr = affected_by_spell(cvict, SPELL_ANTI_MAGIC_SHELL);
            if (!af_ptr && IS_EVIL(caster) && SPELL_IS_DIVINE(spellnum))
                af_ptr = affected_by_spell(cvict, SPELL_DIVINE_INTERVENTION);
            if (!af_ptr && IS_GOOD(caster) && SPELL_IS_DIVINE(spellnum))
                af_ptr = affected_by_spell(cvict, SPELL_SPHERE_OF_DESECRATION);

            if (af_ptr && number(0, af_ptr->level) > number(0, level)) {
                act(tmp_sprintf("$N's %s absorbs $n's %s!",
                        spell_to_str(af_ptr->type),
                        spell_to_str(spellnum)),
                    false, caster, NULL, cvict, TO_NOTVICT);
                act(tmp_sprintf("Your %s absorbs $n's %s!",
                        spell_to_str(af_ptr->type),
                        spell_to_str(spellnum)),
                    false, caster, NULL, cvict, TO_VICT);
                act(tmp_sprintf("$N's %s absorbs your %s!",
                        spell_to_str(af_ptr->type),
                        spell_to_str(spellnum)),
                    false, caster, NULL, cvict, TO_CHAR);
                GET_MANA(cvict) = MIN(GET_MAX_MANA(cvict),
                    GET_MANA(cvict) + (level >> 1));
                if (casttype == CAST_SPELL) {
                    mana = mag_manacost(caster, spellnum);
                    if (mana > 0)
                        GET_MANA(caster) =
                            MAX(0, MIN(GET_MAX_MANA(caster),
                                GET_MANA(caster) - mana));
                }
                if ((af_ptr->duration -= (level >> 2)) <= 0) {
                    send_to_char(cvict, "Your %s dissolves.\r\n",
                        spell_to_str(af_ptr->type));
                    affect_remove(cvict, af_ptr);
                }
                return 0;

            }
        }
    }
    /* determine the type of saving throw */
    switch (casttype) {
    case CAST_STAFF:
    case CAST_SCROLL:
    case CAST_POTION:
    case CAST_WAND:
        savetype = SAVING_ROD;
        break;
    case CAST_SPELL:
        savetype = SAVING_SPELL;
        break;
    case CAST_PSIONIC:
        savetype = SAVING_PSI;
        break;
    case CAST_PHYSIC:
        savetype = SAVING_PHY;
        break;
    case CAST_CHEM:
        savetype = SAVING_CHEM;
        break;
    case CAST_PARA:
        savetype = SAVING_PARA;
        break;
    case CAST_PETRI:
        savetype = SAVING_PETRI;
        break;
    case CAST_BREATH:
    case CAST_BARD:
        savetype = SAVING_BREATH;
        break;
    case CAST_INTERNAL:
        savetype = SAVING_NONE;
        break;
    default:
        savetype = SAVING_BREATH;
        break;
    }
    if (IS_SET(SINFO.routines, MAG_DAMAGE)) {
        mag_damage(level, caster, cvict, spellnum, savetype);

        //
        // somebody died (either caster or cvict)
        //
        if (is_dead(caster) || (cvict && is_dead(cvict))) {
            return 1;
        }
    }

    if (IS_SET(SINFO.routines, MAG_EXITS) && knock_door)
        mag_exits(level, caster, caster->in_room, spellnum);

    if (IS_SET(SINFO.routines, MAG_AFFECTS))
        mag_affects(level, caster, cvict, dvict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
        mag_unaffects(level, caster, cvict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_POINTS))
        mag_points(level, caster, cvict, dvict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
        mag_alter_objs(level, caster, ovict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_GROUPS))
        mag_groups(level, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_MASSES))
        mag_masses(level, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_AREAS)) {
        mag_areas(level, caster, spellnum, savetype);
    }
    if (IS_SET(SINFO.routines, MAG_SUMMONS))
        mag_summons(level, caster, ovict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_CREATIONS))
        mag_creations(level, caster, spellnum);

    if (IS_SET(SINFO.routines, MAG_OBJECTS) && ovict)
        mag_objects(level, caster, ovict, spellnum);

    if (IS_SET(SINFO.routines, MAG_MANUAL))
        switch (spellnum) {
        case SPELL_ASTRAL_SPELL:
            MANUAL_SPELL(spell_astral_spell);
            break;
        case SPELL_ENCHANT_WEAPON:
            MANUAL_SPELL(spell_enchant_weapon);
            break;
        case SPELL_CHARM:
            MANUAL_SPELL(spell_charm);
            break;
        case SPELL_CHARM_ANIMAL:
            MANUAL_SPELL(spell_charm_animal);
            break;
        case SPELL_WORD_OF_RECALL:
            MANUAL_SPELL(spell_recall);
            break;
        case SPELL_IDENTIFY:
            MANUAL_SPELL(spell_identify);
            break;
        case SPELL_SUMMON:
            MANUAL_SPELL(spell_summon);
            break;
        case SPELL_LOCATE_OBJECT:
            MANUAL_SPELL(spell_locate_object);
            break;
        case SPELL_ENCHANT_ARMOR:
            MANUAL_SPELL(spell_enchant_armor);
            break;
        case SPELL_GREATER_ENCHANT:
            MANUAL_SPELL(spell_greater_enchant);
            break;
        case SPELL_MINOR_IDENTIFY:
            MANUAL_SPELL(spell_minor_identify);
            break;
        case SPELL_CLAIRVOYANCE:
            MANUAL_SPELL(spell_clairvoyance);
            break;
        case SPELL_MAGICAL_VESTMENT:
            MANUAL_SPELL(spell_magical_vestment);
            break;
        case SPELL_TELEPORT:
        case SPELL_RANDOM_COORDINATES:
            MANUAL_SPELL(spell_teleport);
            break;
        case SPELL_CONJURE_ELEMENTAL:
            MANUAL_SPELL(spell_conjure_elemental);
            break;
        case SPELL_KNOCK:
            MANUAL_SPELL(spell_knock);
            break;
        case SPELL_SWORD:
            MANUAL_SPELL(spell_sword);
            break;
        case SPELL_ANIMATE_DEAD:
            MANUAL_SPELL(spell_animate_dead);
            break;
        case SPELL_CONTROL_WEATHER:
            MANUAL_SPELL(spell_control_weather);
            break;
        case SPELL_GUST_OF_WIND:
            MANUAL_SPELL(spell_gust_of_wind);
            break;
        case SPELL_RETRIEVE_CORPSE:
            MANUAL_SPELL(spell_retrieve_corpse);
            break;
        case SPELL_LOCAL_TELEPORT:
            MANUAL_SPELL(spell_local_teleport);
            break;
        case SPELL_PEER:
            MANUAL_SPELL(spell_peer);
            break;
        case SPELL_VESTIGIAL_RUNE:
            MANUAL_SPELL(spell_vestigial_rune);
            break;
        case SPELL_ID_INSINUATION:
            MANUAL_SPELL(spell_id_insinuation);
            break;
        case SPELL_SHADOW_BREATH:
            MANUAL_SPELL(spell_shadow_breath);
            break;
        case SPELL_SUMMON_LEGION:
            MANUAL_SPELL(spell_summon_legion);
            break;
        case SPELL_NUCLEAR_WASTELAND:
            MANUAL_SPELL(spell_nuclear_wasteland);
            break;
        case SPELL_SPACETIME_IMPRINT:
            MANUAL_SPELL(spell_spacetime_imprint);
            break;
        case SPELL_SPACETIME_RECALL:
            MANUAL_SPELL(spell_spacetime_recall);
            break;
        case SPELL_TIME_WARP:
            MANUAL_SPELL(spell_time_warp);
            break;
        case SPELL_UNHOLY_STALKER:
            MANUAL_SPELL(spell_unholy_stalker);
            break;
        case SPELL_CONTROL_UNDEAD:
            MANUAL_SPELL(spell_control_undead);
            break;
        case SPELL_INFERNO:
            MANUAL_SPELL(spell_inferno);
            break;
        case SPELL_BANISHMENT:
            MANUAL_SPELL(spell_banishment);
            break;
        case SPELL_AREA_STASIS:
            MANUAL_SPELL(spell_area_stasis);
            break;
        case SPELL_SUN_RAY:
            MANUAL_SPELL(spell_sun_ray);
            break;
        case SPELL_EMP_PULSE:
            MANUAL_SPELL(spell_emp_pulse);
            break;
        case SPELL_QUANTUM_RIFT:
            MANUAL_SPELL(spell_quantum_rift);
            break;
        case SPELL_DEATH_KNELL:
            MANUAL_SPELL(spell_death_knell);
            break;
        case SPELL_DISPEL_MAGIC:
            MANUAL_SPELL(spell_dispel_magic);
            break;
        case SPELL_DISTRACTION:
            MANUAL_SPELL(spell_distraction);
            break;
        case SPELL_BLESS:
            MANUAL_SPELL(spell_bless);
            break;
        case SPELL_DAMN:
            MANUAL_SPELL(spell_damn);
            break;
        case SPELL_CALM:
            MANUAL_SPELL(spell_calm);
            break;
        case SPELL_CALL_RODENT:
            MANUAL_SPELL(spell_call_rodent);
            break;
        case SPELL_CALL_BIRD:
            MANUAL_SPELL(spell_call_bird);
            break;
        case SPELL_CALL_REPTILE:
            MANUAL_SPELL(spell_call_reptile);
            break;
        case SPELL_CALL_BEAST:
            MANUAL_SPELL(spell_call_beast);
            break;
        case SPELL_CALL_PREDATOR:
            MANUAL_SPELL(spell_call_predator);
            break;
        case SPELL_DISPEL_EVIL:
            MANUAL_SPELL(spell_dispel_evil);
            break;
        case SPELL_DISPEL_GOOD:
            MANUAL_SPELL(spell_dispel_good);
            break;
        case SONG_EXPOSURE_OVERTURE:
            MANUAL_SPELL(song_exposure_overture);
            break;
        case SONG_HOME_SWEET_HOME:
            MANUAL_SPELL(spell_recall);
            break;
        case SONG_CLARIFYING_HARMONIES:
            MANUAL_SPELL(spell_identify);
            break;
        case SONG_LAMENT_OF_LONGING:
            MANUAL_SPELL(song_lament_of_longing);
            break;
        case SONG_UNRAVELLING_DIAPASON:
            MANUAL_SPELL(spell_dispel_magic);
            break;
        case SONG_INSTANT_AUDIENCE:
            MANUAL_SPELL(song_instant_audience);
            break;
        case SONG_RHYTHM_OF_ALARM:
            MANUAL_SPELL(song_rhythm_of_alarm);
            break;
        case SONG_WALL_OF_SOUND:
            MANUAL_SPELL(song_wall_of_sound);
            break;
        case SONG_HYMN_OF_PEACE:
            MANUAL_SPELL(song_hymn_of_peace);
            break;

        }

    knock_door = NULL;
    return 1;
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.
 *
 * staff  - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * wand   - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * scroll - [0]        level        [1] spell num        [2] spell num        [3] spell num
 * book - [0]        level        [1] spell num        [2] spell num        [3] spell num
 * potion - [0] level        [1] spell num        [2] spell num        [3] spell num
 * syringe- [0] level        [1] spell num        [2] spell num        [3] spell num
 * pill   - [0] level        [1] spell num        [2] spell num        [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 *
 * Returns 0 when victim dies, or otherwise an abort is needed.
 */

int
mag_objectmagic(struct creature *ch, struct obj_data *obj,
    char *argument)
{
    int i, k, level;
    struct creature *tch = NULL;
    struct obj_data *tobj = NULL;
    struct room_data *was_in = NULL;

    if (ch == NULL) {
        errlog("NULL ch in mag_objectmagic()");
        return 0;
    }

    char *arg = tmp_getword(&argument);

    k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
        FIND_OBJ_EQUIP, ch, &tch, &tobj);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_STAFF:

        if (obj->action_desc) {
            act(obj->action_desc, false, ch, obj, NULL, TO_ROOM);
        } else if (room_is_watery(ch->in_room)) {
            act("The water bubbles and swirls as you extend $p.", false, ch, obj, NULL,
            TO_CHAR);
            act("The water bubbles and swirls as $n extends $p.", false, ch, obj, NULL,
            TO_ROOM);
        } else if (room_is_open_air(ch->in_room)) {
            act("You swing $p in three broad arcs through the open air.", false, ch, obj, NULL,
            TO_CHAR);
            act("$n swings $p in three broad arcs through the open air.", false, ch, obj, NULL,
            TO_ROOM);
        } else if (SECT_TYPE(ch->in_room) == SECT_FIRE_RIVER) {
            act("Fire licks $p as you hold it above the surface.", false, ch, obj, NULL,
            TO_CHAR);
            act("Fire licks $p as $n holds it above the surface.", false, ch, obj, NULL,
            TO_ROOM);
        } else if (SECT_TYPE(ch->in_room) == SECT_PITCH_SUB || SECT_TYPE(ch->in_room) == SECT_PITCH_PIT) {
            act("You press $p into the viscous pitch.", false, ch, obj, NULL,
            TO_CHAR);
            act("$n presses $p into the viscous pitch.", false, ch, obj, NULL,
            TO_ROOM);
        } else {
            act("You tap $p three times on the ground.", false, ch, obj, NULL,
            TO_CHAR);
            act("$n taps $p three times on the ground.", false, ch, obj, NULL,
            TO_ROOM);            
        }

        if (GET_OBJ_VAL(obj, 2) <= 0) {
            act("It seems powerless.", false, ch, obj, NULL, TO_CHAR);
            act(NOEFFECT, false, ch, obj, NULL, TO_ROOM);
        } else {

            if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
                || invalid_char_class(ch, obj)
                || (GET_INT(ch) + CHECK_SKILL(ch,
                        SKILL_USE_WANDS)) < number(20, 100)) {
                act("$p flashes and shakes for a moment, but nothing else happens.", false, ch, obj, NULL, TO_CHAR);
                act("$p flashes and shakes for a moment, but nothing else happens.", false, ch, obj, NULL, TO_ROOM);
                GET_OBJ_VAL(obj, 2)--;
                return 1;
            }
            gain_skill_prof(ch, SKILL_USE_WANDS);
            GET_OBJ_VAL(obj, 2)--;
            WAIT_STATE(ch, PULSE_VIOLENCE);

            level =
                (GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_USE_WANDS)) / 100;
            level = MIN(level, LVL_AMBASSADOR);

            for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
                struct creature *tch = (struct creature *)it->data;
                if (ch == tch && spell_info[GET_OBJ_VAL(obj, 3)].violent)
                    continue;
                if (level)
                    call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, 3),
                        level, CAST_STAFF);
                else
                    call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, 3),
                        DEFAULT_STAFF_LVL, CAST_STAFF);
            }
        }
        break;
    case ITEM_WAND:
        if (k == FIND_CHAR_ROOM) {
            if (tch == ch) {
                act("You point $p at yourself.", false, ch, obj, NULL, TO_CHAR);
                act("$n points $p at $mself.", false, ch, obj, NULL, TO_ROOM);
            } else {
                act("You point $p at $N.", false, ch, obj, tch, TO_CHAR);
                if (obj->action_desc != NULL)
                    act(obj->action_desc, false, ch, obj, tch, TO_ROOM);
                else {
                    act("$n points $p at $N.", true, ch, obj, tch, TO_NOTVICT);
                    act("$n points $p at you.", true, ch, obj, tch, TO_VICT);
                }
            }
        } else if (tobj != NULL) {
            act("You point $p at $P.", false, ch, obj, tobj, TO_CHAR);
            if (obj->action_desc != NULL)
                act(obj->action_desc, false, ch, obj, tobj, TO_ROOM);
            else
                act("$n points $p at $P.", true, ch, obj, tobj, TO_ROOM);
        } else {
            act("At what should $p be pointed?", false, ch, obj, NULL,
                TO_CHAR);
            return 1;
        }

        if (GET_OBJ_VAL(obj, 2) <= 0) {
            act("It seems powerless.", false, ch, obj, NULL, TO_CHAR);
            act(NOEFFECT, false, ch, obj, NULL, TO_ROOM);
            return 1;
        }

        if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
            || invalid_char_class(ch, obj)
            || (GET_INT(ch) + CHECK_SKILL(ch, SKILL_USE_WANDS)) < number(20,
                100)) {
            act("$p flashes and shakes for a moment, but nothing else happens.", false, ch, obj, NULL, TO_CHAR);
            act("$p flashes and shakes for a moment, but nothing else happens.", false, ch, obj, NULL, TO_ROOM);
            GET_OBJ_VAL(obj, 2)--;
            return 1;
        }

        level = (GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_USE_WANDS)) / 100;
        level = MIN(level, LVL_AMBASSADOR);

        gain_skill_prof(ch, SKILL_USE_WANDS);
        GET_OBJ_VAL(obj, 2)--;
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return (call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, 3),
                level ? level : DEFAULT_WAND_LVL, CAST_WAND));
        break;
    case ITEM_SCROLL:
        if (*arg) {
            if (!k) {
                act("There is nothing to here to affect with $p.", false,
                    ch, obj, NULL, TO_CHAR);
                return 1;
            }
        } else
            tch = ch;

        act("You recite $p which dissolves.", true, ch, obj, NULL, TO_CHAR);
        if (obj->action_desc)
            act(obj->action_desc, false, ch, obj, NULL, TO_ROOM);
        else if (tch && tch != ch) {
            act("$n recites $p in your direction.", false, ch, obj, tch,
                TO_VICT);
            act("$n recites $p in $N's direction.", false, ch, obj, tch,
                TO_NOTVICT);
        } else if (tobj) {
            act("$n looks at $P and recites $p.", false, ch, obj, tobj,
                TO_ROOM);
        } else
            act("$n recites $p.", false, ch, obj, NULL, TO_ROOM);

        if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
            || invalid_char_class(ch, obj)
            || (GET_INT(ch) + CHECK_SKILL(ch, SKILL_READ_SCROLLS)) < number(20,
                100)) {
            act("$p flashes and smokes for a moment, then is gone.", false, ch,
                obj, NULL, TO_CHAR);
            act("$p flashes and smokes for a moment before dissolving.", false,
                ch, obj, NULL, TO_ROOM);
            if (obj)
                extract_obj(obj);
            return 1;
        }

        level =
            (GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_READ_SCROLLS)) / 100;
        level = MIN(level, LVL_AMBASSADOR);

        WAIT_STATE(ch, PULSE_VIOLENCE);
        gain_skill_prof(ch, SKILL_READ_SCROLLS);

        // Remove object in case it's destroyed by its own spells
        if (obj->worn_by)
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
        else if (obj->carried_by)
            obj_from_char(obj);

        for (i = 1; i < 4; i++) {
            call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, i),
                level, CAST_SCROLL);
            if (is_dead(ch) || (tch && is_dead(tch)))
                break;
        }
        extract_obj(obj);
        break;
    case ITEM_BOOK:
        if (GET_OBJ_VAL(obj, 0) == 0) {
            send_to_char(ch, "The pages are blank.\r\n");
            break;
        }

        if (obj->action_desc)
            act(obj->action_desc, false, ch, obj, NULL, TO_ROOM);
        else
            act("$n opens $p and studies it carefully.", true, ch, obj, NULL,
                TO_ROOM);

        if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
            || invalid_char_class(ch, obj)) {
            act("You try to read $p but the writing doesn't make any sense to you.", false, ch, obj, NULL, TO_CHAR);
            return 1;
        }

        act("You study the writing in $p carefully.", true, ch, obj, NULL,
            TO_CHAR);
        level = GET_OBJ_VAL(obj, 0);
        level = MIN(level, LVL_AMBASSADOR);

        WAIT_STATE(ch, 2 RL_SEC);

        // Remove object in case it's destroyed by its own spells
        if (obj->worn_by)
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
        else if (obj->carried_by)
            obj_from_char(obj);
        // Save the room too
        was_in = ch->in_room;

        for (i = 1; i < 4; i++) {
            call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
                level, CAST_SCROLL);
            if (is_dead(ch))
                break;
        }

        // Stick it in the room
        if (ch->in_room)
            obj_to_room(obj, ch->in_room);
        else
            obj_to_room(obj, was_in);

        act("$p bursts into flame and disappears!", true, ch, obj, NULL,
            TO_ROOM);
        extract_obj(obj);
        break;
    case ITEM_FOOD:
        if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
            act("You feel a strange sensation as you eat $p...", false,
                ch, obj, NULL, TO_CHAR);
            return (call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, 2),
                    GET_OBJ_VAL(obj, 1), CAST_POTION));
        }
        break;
    case ITEM_POTION:
        tch = ch;
        act("You quaff $p.", false, ch, obj, NULL, TO_CHAR);
        if (obj->action_desc)
            act(obj->action_desc, false, ch, obj, NULL, TO_ROOM);
        else
            act("$n quaffs $p.", true, ch, obj, NULL, TO_ROOM);

        WAIT_STATE(ch, PULSE_VIOLENCE);

        // Remove object in case it's destroyed by its own spells
        if (obj->worn_by)
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
        else if (obj->carried_by)
            obj_from_char(obj);

        if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
            for (i = 1; i < 4; i++) {
                if (!ch->in_room)
                    break;
                call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
                           GET_OBJ_VAL(obj, 0), CAST_POTION);
                if (is_dead(ch))
                    break;
            }
        }
        extract_obj(obj);
        break;
    case ITEM_SYRINGE:
        if (k != FIND_CHAR_ROOM) {
            act("Who do you want to inject with $p?", false, ch, obj, NULL,
                TO_CHAR);
            return 1;
        }

        if ((affected_by_spell(tch, SPELL_STONESKIN)
             || (affected_by_spell(tch, SPELL_DERMAL_HARDENING) && random_binary())
             || (affected_by_spell(tch, SPELL_BARKSKIN) && random_binary()))) {
            act("$p breaks.", true, ch, obj, tch, TO_CHAR);
            act("$n breaks $p on your arm!", true, ch, obj, tch,
                TO_VICT);
            act("$n breaks $p on $N's arm!", true, ch, obj, tch,
                TO_NOTVICT);
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
            obj->obj_flags.damage = 0;
            extract_obj(obj);
            return 1;
        }

        if (tch == ch) {
            act("You jab $p into your arm and press the plunger.",
                false, ch, obj, NULL, TO_CHAR);
            act("$n jabs $p into $s arm and depresses the plunger.",
                false, ch, obj, NULL, TO_ROOM);
        } else {
            act("You jab $p into $N's arm and press the plunger.",
                false, ch, obj, tch, TO_CHAR);
            act("$n jabs $p into $N's arm and presses the plunger.",
                true, ch, obj, tch, TO_NOTVICT);
            act("$n jabs $p into your arm and presses the plunger.",
                true, ch, obj, tch, TO_VICT);
        }

        if (obj->action_desc != NULL)
            act(obj->action_desc, false, ch, obj, tch, TO_VICT);

        WAIT_STATE(ch, PULSE_VIOLENCE);

        // Remove object in case it's destroyed by its own spells
        if (obj->worn_by)
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
        else if (obj->carried_by)
            obj_from_char(obj);

        for (i = 1; i < 4; i++) {
            call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, i),
                GET_OBJ_VAL(obj, 0),
                IS_OBJ_STAT(obj, ITEM_MAGIC) ? CAST_SPELL :
                CAST_INTERNAL);
            if (is_dead(ch) || (tch && is_dead(tch)))
                break;
        }
        extract_obj(obj);
        break;

    case ITEM_PILL:
        tch = ch;
        act("You swallow $p.", false, ch, obj, NULL, TO_CHAR);
        if (obj->action_desc)
            act(obj->action_desc, false, ch, obj, NULL, TO_ROOM);
        else
            act("$n swallows $p.", true, ch, obj, NULL, TO_ROOM);

        WAIT_STATE(ch, PULSE_VIOLENCE);

        // Remove object in case it's destroyed by its own spells
        if (obj->worn_by)
            unequip_char(ch, obj->worn_on, EQUIP_WORN);
        else if (obj->carried_by)
            obj_from_char(obj);

        for (i = 1; i < 4; i++) {
            call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
                GET_OBJ_VAL(obj, 0), CAST_INTERNAL);
            if (is_dead(ch))
                break;
        }
        extract_obj(obj);
        break;

    default:
        errlog("Unknown object_type in mag_objectmagic");
        break;
    }
    return 1;
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

int
cast_spell(struct creature *ch, struct creature *tch,
    struct obj_data *tobj, int *tdir, int spellnum)
{

    void sing_song(struct creature *ch, struct creature *vict,
        struct obj_data *ovict, int spellnum);

    //
    // subtract npc mana first, since this is the entry point for npc spells
    // do_cast, do_trigger, do_alter are the entrypoints for PC spells
    //

    if (IS_NPC(ch)) {
        if (GET_MANA(ch) < mag_manacost(ch, spellnum))
            return 0;
        else {
            GET_MANA(ch) -= mag_manacost(ch, spellnum);
            if (!ch->desc) {
                WAIT_STATE(ch, (3 RL_SEC));
            }
        }
    }
    //
    // verify correct position
    //

    if (GET_POSITION(ch) < SINFO.min_position) {
        switch (GET_POSITION(ch)) {
        case POS_SLEEPING:
            if (SPELL_IS_PHYSICS(spellnum))
                send_to_char(ch, "You dream about great physical powers.\r\n");
            if (SPELL_IS_PSIONIC(spellnum))
                send_to_char(ch, "You dream about great psionic powers.\r\n");
            if (SPELL_IS_BARD(spellnum))
                send_to_char(ch, "You dream about great musical talent.\r\n");
            else
                send_to_char(ch, "You dream about great magical powers.\r\n");
            break;
        case POS_RESTING:
            send_to_char(ch, "You cannot concentrate while resting.\r\n");
            break;
        case POS_SITTING:
            send_to_char(ch, "You can't do this sitting!\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
            if (IS_NPC(ch)) {
                errlog("%s tried to cast spell %d in battle.",
                    GET_NAME(ch), spellnum);
            }
            break;
        default:
            send_to_char(ch, "You can't do much of anything like this!\r\n");
            break;
        }
        return 0;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
        send_to_char(ch, "You are afraid you might hurt your master!\r\n");
        return 0;
    }
    if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY) &&
        GET_LEVEL(ch) < LVL_CREATOR) {
        send_to_char(ch, "You can only %s yourself!\r\n",
            SPELL_IS_PHYSICS(spellnum) ? "alter this reality on" :
            SPELL_IS_PSIONIC(spellnum) ? "trigger this psi on" :
            SPELL_IS_MERCENARY(spellnum) ? "apply this device to" :
            SPELL_IS_BARD(spellnum) ? "evoke this song on" :
            "cast this spell upon");
        return 0;
    }
    if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
        send_to_char(ch, "You cannot %s yourself!\r\n",
            SPELL_IS_PHYSICS(spellnum) ? "alter this reality on" :
            SPELL_IS_PSIONIC(spellnum) ? "trigger this psi on" :
            SPELL_IS_MERCENARY(spellnum) ? "apply this device to" :
            SPELL_IS_BARD(spellnum) ? "evoke this song on" :
            "cast this spell upon");
        return 0;
    }
    if (IS_CONFUSED(ch) &&
        CHECK_SKILL(ch, spellnum) + GET_INT(ch) < number(90, 180)) {
        send_to_char(ch, "You are too confused to %s\r\n",
            SPELL_IS_PHYSICS(spellnum) ? "alter any reality!" :
            SPELL_IS_PSIONIC(spellnum) ? "trigger any psi!" :
            SPELL_IS_MERCENARY(spellnum) ? "construct any devices!" :
            SPELL_IS_BARD(spellnum) ? "sing any songs!" : "cast any spells!");
        return 0;
    }
    if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP) &&
        !IS_SET(SINFO.routines, MAG_BARD)) {
        send_to_char(ch, "You can't do this if you're not in a group!\r\n");
        return 0;
    }
    if (room_is_underwater(ch->in_room)
        && SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
        send_to_char(ch, "This spell does not function underwater.\r\n");
        return 0;
    }
    if (!OUTSIDE(ch) && SPELL_FLAGGED(spellnum, MAG_OUTDOORS)) {
        send_to_char(ch, "This spell can only be cast outdoors.\r\n");
        return 0;
    }
    if (SPELL_FLAGGED(spellnum, MAG_NOSUN) && OUTSIDE(ch)
        && room_is_sunny(ch->in_room)) {
        send_to_char(ch, "This spell cannot be cast in sunlight.\r\n");
        return 0;
    }
    // Evil Knight remort skill. 'Taint'
    if (ch != NULL && GET_LEVEL(ch) < LVL_AMBASSADOR
        && AFF3_FLAGGED(ch, AFF3_TAINTED)) {
        int rflags;
        if (!(rflags = perform_taint_burn(ch, spellnum)))
            return rflags;
    }

    if (SPELL_IS_BARD(spellnum))
        sing_song(ch, tch, tobj, spellnum);
    else
        say_spell(ch, spellnum, tch, tobj);

    if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_AMBASSADOR &&
        GET_LEVEL(ch) < LVL_GOD && !mini_mud &&
        (!tch || GET_LEVEL(tch) < LVL_AMBASSADOR) && (ch != tch)) {
        slog("ImmCast: %s called %s on %s.", GET_NAME(ch),
            spell_to_str(spellnum), tch ? GET_NAME(tch) : tobj ?
            tobj->name : knock_door ?
            (knock_door->keyword ? fname(knock_door->keyword) :
                "door") : "NULL");
    }

    int retval = call_magic(ch, tch, tobj, tdir, spellnum,
                            GET_LEVEL(ch) + (!IS_NPC(ch) ? (GET_REMORT_GEN(ch) << 1) : 0),
                            (SPELL_IS_PSIONIC(spellnum) ? CAST_PSIONIC :
                             (SPELL_IS_PHYSICS(spellnum) ? CAST_PHYSIC :
                              (SPELL_IS_BARD(spellnum) ? CAST_BARD : CAST_SPELL))));

    if (is_dead(ch))
        return 0;
    if (tch && is_dead(tch))
        return 1;

    if (tch
        && ch != tch
        && IS_NPC(tch)
        && ch->in_room == tch->in_room
        && SINFO.violent
        && !is_fighting(tch)
        && GET_POSITION(tch) > POS_SLEEPING
        && (!AFF_FLAGGED(tch, AFF_CHARM) || ch != tch->master)) {
        hit(tch, ch, TYPE_UNDEFINED);
        if (is_dead(ch))
            return 0;
    }

    return (retval);
}

int
perform_taint_burn(struct creature *ch, int spellnum)
{
    struct affected_type *af;
    int mana = mag_manacost(ch, spellnum);
    int dam = dice(mana, 14);
    bool weenie = false;

    // Grab the affect for affect level
    af = affected_by_spell(ch, SPELL_TAINT);

    // Max dam based on level and gen of caster. (level_bonus(taint))
    if (af != NULL)
        dam = dam * af->level / 100;

    if (damage(ch, ch, NULL, dam, TYPE_TAINT_BURN, WEAR_HEAD)) {
        return 0;
    }
    // fucking weaklings can't cast while tainted.
    int attribute = MIN(GET_CON(ch), GET_WIS(ch));
    if (number(1, attribute) < number(mana / 2, mana)) {
        weenie = true;
    }
    if (ch && PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch,
            "%s[TAINT] %s attribute:%d   weenie:%s   mana:%d   damage:%d%s\r\n",
            CCCYN(ch, C_NRM), GET_NAME(ch), attribute,
            weenie ? "t" : "f", mana, dam, CCNRM(ch, C_NRM));

    if (af != NULL) {
        af->duration -= mana;
        af->duration = MAX(af->duration, 0);
        if (af->duration == 0) {
            affect_remove(ch, af);
            act("Blood pours out of $n's forehead as the rune of taint dissolves.", true, ch, NULL, NULL, TO_ROOM);
        }
    }

    if (weenie == true) {
        act("$n screams and clutches at the rune in $s forehead.", true,
            ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "Your concentration fails.\r\n");
        return 0;
    }

    return 1;
}
/**
 * find_spell_targets:
 * @ch the casting creature
 * @cmd the command being used to cast
 * @argument the full argument to the command
 * @spellnm id number of spell
 * @target true if the spell target was found, or none was required
 * @tch the target creature, NULL if none
 * @tobj the target object, NULL if none
 * @tdir the target direction.  -1 if none
 *
 * Parses the arguments to casting commands.  The return value is true
 * when the command should still be processed, and false when the
 * command execution is complete.
 *
 * Returns:
 *   true when the command should continue processing, false when the
 *   command is handled
 *
 **/
bool
find_spell_targets(struct creature *ch, int cmd, char *argument,
                   int *spellnm, bool *target,
                   struct creature **tch,
                   struct obj_data **tobj,
                   int *tdir)
{
    char *s, *targets = "", *target_word, *ptr;
    int i, spellnum;

    *tch = NULL;
    *tobj = NULL;
    *tdir = -1;
    knock_door = NULL;

    // There must be exactly 0 or exactly 2 's in the argument
    if ((s = strstr(argument, "\'"))) {
        if (!(strstr(s + 1, "\'"))) {
            send_to_char(ch, "The skill name must be completely enclosed "
                "in the symbols: '\n");
            return false;
        } else {
            s = tmp_getquoted(&argument);
            spellnum = find_skill_num(s);
            *spellnm = spellnum;
            targets = tmp_strdup(argument);
        }
    } else {
        s = argument;
        spellnum = find_skill_num(s);
        while (spellnum == -1 && *s) {
            ptr = s + strlen(s) - 1;
            while (*ptr != '\0' && *ptr != ' ' && ptr != s)
                ptr--;
            if (ptr == s)
                break;

            targets = tmp_sprintf("%s%s", ptr, targets ? targets : "");
            *ptr = '\0';

            spellnum = find_skill_num(s);
        }
    }

    *spellnm = spellnum;

    if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
        act(tmp_sprintf("%s what?!?", cmd_info[cmd].command),
            false, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
        return false;
    }

    if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)] &&
        (!IS_REMORT(ch) ||
            GET_LEVEL(ch) < SINFO.min_level[(int)GET_REMORT_CLASS(ch)]) &&
        CHECK_SKILL(ch, spellnum) < 30) {
        send_to_char(ch, "You do not know that %s!\r\n",
            SPELL_IS_PSIONIC(spellnum) ? "trigger" :
            SPELL_IS_PHYSICS(spellnum) ? "alteration" :
            SPELL_IS_MERCENARY(spellnum) ? "device" :
            SPELL_IS_BARD(spellnum) ? "song" : "spell");
        return false;
    }

    if (CHECK_SKILL(ch, spellnum) == 0) {
        send_to_char(ch, "You are unfamiliar with that %s.\r\n",
            SPELL_IS_PSIONIC(spellnum) ? "trigger" :
            SPELL_IS_PHYSICS(spellnum) ? "alteration" :
            SPELL_IS_MERCENARY(spellnum) ? "device" :
            SPELL_IS_BARD(spellnum) ? "song" : "spell");
        return false;
    }
    /* Find the target */

    strncpy(locate_buf, targets, 255);
    locate_buf[255] = '\0';
    target_word = tmp_getword(&targets);

    if (IS_SET(SINFO.targets, TAR_IGNORE)) {
        *target = true;
    } else if (*target_word) {
        if (!*target && (IS_SET(SINFO.targets, TAR_DIR))) {
            *tdir = search_block(target_word, dirs, false);
            if (*tdir >= 0)
                *target = true;

        } else if (!*target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
            if ((*tch = get_char_room_vis(ch, target_word)) != NULL)
                *target = true;
        }
        if (!*target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
            if ((*tch = get_char_vis(ch, target_word)))
                *target = true;

        if (!*target && IS_SET(SINFO.targets, TAR_OBJ_INV))
            if ((*tobj = get_obj_in_list_vis(ch, target_word, ch->carrying)))
                *target = true;

        if (!*target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
            for (i = 0; !*target && i < NUM_WEARS; i++)
                if (GET_EQ(ch, i)
                    && !strcasecmp(target_word, GET_EQ(ch, i)->aliases)) {
                    *tobj = GET_EQ(ch, i);
                    *target = true;
                }
        }
        if (!*target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
            if ((*tobj =
                    get_obj_in_list_vis(ch, target_word, ch->in_room->contents)))
                *target = true;

        if (!*target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
            if ((*tobj = get_obj_vis(ch, target_word)))
                *target = true;

        if (!*target && IS_SET(SINFO.targets, TAR_DOOR)) {
            char *t3 = tmp_getword(&targets);
            if ((i = find_door(ch, target_word, t3,
                        spellnum == SPELL_KNOCK ? "knock" : "cast")) >= 0) {
                knock_door = ch->in_room->dir_option[i];
                *target = true;
            } else {
                return false;
            }
        }

    } else {                    /* if target string is empty */
        if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
            if (is_fighting(ch)) {
                *tch = ch;
                *target = true;
            }
        if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
            if (is_fighting(ch)) {
                *tch = random_opponent(ch);
                *target = true;
            }
        /* if no target specified,
           and the spell isn't violent, i
           default to self */
        if (!*target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
            !SINFO.violent && !IS_SET(SINFO.targets, TAR_UNPLEASANT)) {
            *tch = ch;
            *target = true;
        }

        if (!*target) {
            send_to_char(ch, "Upon %s should the spell be cast?\r\n",
                IS_SET(SINFO.targets,
                    TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ? "what" :
                "whom");
            return false;
        }
    }
    return true;
}

ACMD(do_cast)
{
    struct creature *tch = NULL;
    int tdir;
    struct obj_data *tobj = NULL, *holy_symbol = NULL, *metal = NULL;
    bool target = false;
    int mana, spellnum, i, prob = 0, metal_wt = 0, num_eq =
        0, temp = 0;

    if (IS_NPC(ch))
        return;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char(ch, "You were going to cast something?\r\n");
        return;
    }

    if (!IS_MAGE(ch) && !IS_CLERIC(ch) && !IS_KNIGHT(ch) && !IS_RANGER(ch)
        && !IS_VAMPIRE(ch) && GET_CLASS(ch) < NUM_CLASSES
        && (GET_LEVEL(ch) < LVL_GRGOD)) {
        send_to_char(ch, "You are not learned in the ways of magic.\r\n");
        return;
    }
    if (GET_LEVEL(ch) < LVL_AMBASSADOR &&
        IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.90)) {
        send_to_char(ch,
            "Your equipment is too heavy and bulky to cast anything useful!\r\n");
        return;
    }
    if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_WIELD) &&
        IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
        send_to_char(ch,
            "You can't cast spells while wielding a two handed weapon!\r\n");
        return;
    }

    if (!(find_spell_targets(ch, cmd, argument, &spellnum, &target, &tch, &tobj, &tdir)))
        return;

    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) &&
        GET_LEVEL(ch) < LVL_TIMEGOD && (SPELL_IS_MAGIC(spellnum) ||
            SPELL_IS_DIVINE(spellnum))) {
        send_to_char(ch, "Your magic fizzles out and dies.\r\n");
        act("$n's magic fizzles out and dies.", false, ch, NULL, NULL, TO_ROOM);
        return;
    }
    // Drunk bastards don't cast very well, do they... -- Nothing 1/22/2001
    if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
        if (temp < 34) {
            send_to_char(ch,
                "Your mind is too clouded to cast any spells!\r\n");
            return;
        } else {
            send_to_char(ch, "You feel your concentration slipping!\r\n");
            WAIT_STATE(ch, 2 RL_SEC);
            spellnum = number(1, MAX_SPELLS);
            if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
                send_to_char(ch,
                    "Your concentration slips away entirely.\r\n");
                return;
            }
        }
    }

    if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
        act("That is not a spell.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (!target && !IS_SET(SINFO.targets, TAR_DOOR)) {
        send_to_char(ch, "Cannot find the target of your spell!\r\n");
        return;
    }

    mana = mag_manacost(ch, spellnum);

    if ((mana > 0) && (GET_MANA(ch) < mana) && GET_LEVEL(ch) < LVL_AMBASSADOR
        && !PLR_FLAGGED(ch, PLR_MORTALIZED)) {
        send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT
        && ((!IS_EVIL(ch) && SPELL_IS_EVIL(spellnum))
            || (!IS_GOOD(ch) && SPELL_IS_GOOD(spellnum)))) {
        send_to_char(ch, "You cannot cast that spell.\r\n");
        return;
    }

    if (GET_LEVEL(ch) > 3 && GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_NPC(ch) &&
        (IS_CLERIC(ch) || IS_KNIGHT(ch)) && SPELL_IS_DIVINE(spellnum)) {
        bool need_symbol = true;
        int gen = GET_REMORT_GEN(ch);
        if (IS_EVIL(ch) && IS_SOULLESS(ch)) {
            if (GET_CLASS(ch) == CLASS_CLERIC && gen >= 4)
                need_symbol = false;
            else if (GET_CLASS(ch) == CLASS_KNIGHT && gen >= 6)
                need_symbol = false;
        }
        if (need_symbol) {
            for (i = 0; i < NUM_WEARS; i++) {
                if (ch->equipment[i]) {
                    if (IS_OBJ_TYPE(ch->equipment[i], ITEM_HOLY_SYMB)) {
                        holy_symbol = ch->equipment[i];
                        break;
                    }
                }
            }
            if (!holy_symbol) {
                send_to_char(ch,
                    "You do not even wear the symbol of your faith!\r\n");
                return;
            }
            if ((IS_GOOD(ch) && (GET_OBJ_VAL(holy_symbol, 0) == 2)) ||
                (IS_EVIL(ch) && (GET_OBJ_VAL(holy_symbol, 0) == 0))) {
                send_to_char(ch,
                    "You are not aligned with your holy symbol!\r\n");
                return;
            }
            if (GET_CLASS(ch) != GET_OBJ_VAL(holy_symbol, 1) &&
                GET_REMORT_CLASS(ch) != GET_OBJ_VAL(holy_symbol, 1)) {
                send_to_char(ch,
                    "The holy symbol you wear is not of your faith!\r\n");
                return;
            }
            if (GET_LEVEL(ch) < GET_OBJ_VAL(holy_symbol, 2)) {
                send_to_char(ch,
                    "You are not powerful enough to utilize your holy symbol!\r\n");
                return;
            }
            if (GET_LEVEL(ch) > GET_OBJ_VAL(holy_symbol, 3)) {
                act("$p will no longer support your power!",
                    false, ch, holy_symbol, NULL, TO_CHAR);
                return;
            }
        }
    }

    /***** casting probability calculation *****/
    if ((IS_CLERIC(ch) || IS_KNIGHT(ch)) && SPELL_IS_DIVINE(spellnum)) {
        prob -= (GET_WIS(ch) + (abs(GET_ALIGNMENT(ch) / 70)));
        if (IS_NEUTRAL(ch))
            prob += 30;
    }
    if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
        && SPELL_IS_MAGIC(spellnum))
        prob -= (GET_INT(ch) + GET_DEX(ch));

    if (IS_SICK(ch))
        prob += 20;

    if (IS_CONFUSED(ch))
        prob += number(35, 55) - GET_INT(ch);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
        prob += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD));

    prob += ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 8) / CAN_CARRY_W(ch);

    for (i = 0, num_eq = 0, metal_wt = 0; i < NUM_WEARS; i++) {
        if (ch->equipment[i]) {
            num_eq++;
            if (i != WEAR_WIELD && IS_OBJ_TYPE(ch->equipment[i], ITEM_ARMOR)
                && IS_METAL_TYPE(ch->equipment[i])) {
                metal_wt += GET_OBJ_WEIGHT(ch->equipment[i]);
                if (!metal || !number(0, 8) ||
                    (GET_OBJ_WEIGHT(ch->equipment[i]) > GET_OBJ_WEIGHT(metal)
                        && !number(0, 1)))
                    metal = ch->equipment[i];
            }
            if (ch->implants[i]) {
                if (IS_METAL_TYPE(ch->equipment[i])) {
                    metal_wt += GET_OBJ_WEIGHT(ch->equipment[i]);
                    if (!metal || !number(0, 8) ||
                        (GET_OBJ_WEIGHT(ch->implants[i]) >
                            GET_OBJ_WEIGHT(metal) && !number(0, 1)))
                        metal = ch->implants[i];
                }
            }
        }
    }

    if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
        && SPELL_IS_MAGIC(spellnum)
        && GET_LEVEL(ch) < LVL_ETERNAL)
        prob += metal_wt;

    prob -= (NUM_WEARS - num_eq);

    if (tch && GET_POSITION(ch) == POS_FIGHTING)
        prob += (GET_LEVEL(tch) >> 3);

    /**** casting probability ends here *****/

    /* Begin fucking them over for failing and describing said fuckover */

    if ((prob + number(0, 75)) > CHECK_SKILL(ch, spellnum)) {
        /* there is a failure */

        WAIT_STATE(ch, ((3 RL_SEC) - GET_REMORT_GEN(ch)));

        if (tch && tch != ch) {
            /* victim exists */

            if ((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent) &&
                !IS_SET(SINFO.routines, MAG_TOUCH) &&
                (prob + number(0, 111)) > CHECK_SKILL(ch, spellnum)) {
                /* misdirect */

                for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
                    struct creature *new_vict = (struct creature *)it->data;

                    if (new_vict != ch
                        && new_vict != tch
                        && GET_LEVEL(tch) < LVL_AMBASSADOR
                        && (!number(0, 4) || !next_living(it))) {

                        if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
                            && metal && SPELL_IS_MAGIC(spellnum)
                            && metal_wt > number(5, 80))
                            act("$p has misdirected your spell toward $N!!",
                                false, ch, metal, tch, TO_CHAR);
                        else
                            act("Your spell has been misdirected toward $N!!",
                                false, ch, NULL, tch, TO_CHAR);
                        cast_spell(ch, tch, tobj, &tdir, spellnum);
                        if (mana > 0)
                            GET_MANA(ch) =
                                MAX(0, MIN(GET_MAX_MANA(ch),
                                    GET_MANA(ch) - (mana >> 1)));
                        WAIT_STATE(ch, 2 RL_SEC);
                        return;
                    }
                }
            }

            if ((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent) &&
                (prob + number(0, 81)) > CHECK_SKILL(ch, spellnum)) {

                /* backfire */

                if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) && metal
                    && SPELL_IS_MAGIC(spellnum) && metal_wt > number(5, 80))
                    act("$p has caused your spell to backfire!!", false, ch,
                        metal, NULL, TO_CHAR);
                else
                    send_to_char(ch, "Your spell has backfired!!\r\n");
                cast_spell(ch, ch, tobj, &tdir, spellnum);
                if (mana > 0)
                    GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) -
                            (mana >> 1)));
                WAIT_STATE(ch, 2 RL_SEC);
                return;
            }

            /* plain dud */
            if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) && metal &&
                SPELL_IS_MAGIC(spellnum) && metal_wt > number(5, 100))
                act("$p has interfered with your spell!",
                    false, ch, metal, NULL, TO_CHAR);
            else
                send_to_char(ch, "You lost your concentration!\r\n");
            if (!skill_message(0, ch, tch, NULL, spellnum)) {
                send_to_char(ch, "%s", NOEFFECT);
            }

            if (((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent)) &&
                IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_NOHASSLE) &&
                can_see_creature(tch, ch))
                hit(tch, ch, TYPE_UNDEFINED);

        } else if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) &&
            SPELL_IS_MAGIC(spellnum) && metal && metal_wt > number(5, 90))
            act("$p has interfered with your spell!",
                false, ch, metal, NULL, TO_CHAR);
        else
            send_to_char(ch, "You lost your concentration!\r\n");

        if (mana > 0)
            GET_MANA(ch) =
                MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
        /* cast spell returns 1 on success; subtract mana & set waitstate */
        //HERE
    } else {
        if (cast_spell(ch, tch, tobj, &tdir, spellnum)) {
            WAIT_STATE(ch, ((3 RL_SEC) - (GET_CLASS(ch) == CLASS_MAGIC_USER ?
                        GET_REMORT_GEN(ch) : 0)));
            if (mana > 0)
                GET_MANA(ch) =
                    MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
            gain_skill_prof(ch, spellnum);

        }
    }
}

ACMD(do_trigger)
{
    struct creature *tch = NULL;
    struct obj_data *tobj = NULL;
    int tdir;
    bool target = false;
    int mana, spellnum, prob = 0, temp = 0;

    if (IS_NPC(ch))
        return;

    if (!*argument) {
        send_to_char(ch, "You were going to trigger a psionic potential?\r\n");
        return;
    }

    if (!IS_PSYCHIC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "You are not able to trigger the mind.\r\n");
        return;
    }
    if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
        send_to_char(ch,
            "Your equipment is too heavy and bulky for you to concentrate!\r\n");
        return;
    }

    if (!(find_spell_targets(ch, cmd, argument, &spellnum, &target, &tch, &tobj, &tdir)))
        return;

    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)) &&
        GET_LEVEL(ch) < LVL_TIMEGOD && SPELL_IS_PSIONIC(spellnum)) {
        send_to_char(ch, "You cannot establish a mental link.\r\n");
        act("$n appears to be psionically challenged.",
            false, ch, NULL, NULL, TO_ROOM);
        return;
    }
    // Drunk bastards don't trigger very well, do they... -- Nothing 1/22/2001
    if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
        if (temp < 34) {
            send_to_char(ch,
                "Your mind is too clouded to make any triggers!\r\n");
            return;
        } else {
            send_to_char(ch, "You feel your concentration slipping!\r\n");
            WAIT_STATE(ch, 2 RL_SEC);
            spellnum = number(1, MAX_SPELLS);
            if (!SPELL_IS_PSIONIC(spellnum)) {
                send_to_char(ch,
                    "Your concentration slips away entirely.\r\n");
                return;
            }
        }
    }

    if (!SPELL_IS_PSIONIC(spellnum)) {
        act("That is not a psionic trigger.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if (!target) {
        send_to_char(ch, "Cannot find the target of your trigger!\r\n");
        return;
    }
    mana = mag_manacost(ch, spellnum);
    if ((mana > 0) && (GET_MANA(ch) < mana)
        && (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
        send_to_char(ch, "You haven't the energy to make that trigger!\r\n");
        return;
    }

    if (tch && (IS_UNDEAD(tch) || IS_SLIME(tch) || IS_PUDDING(tch) ||
            IS_ROBOT(tch) || IS_PLANT(tch))) {
        act("You cannot make a mindlink with $N!", false, ch, NULL, tch, TO_CHAR);
        return;
    }

    if (room_is_underwater(ch->in_room)
        && SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
        send_to_char(ch, "This trigger will not function underwater.\r\n");
        return;
    }
    /***** trigger probability calculation *****/
    prob = CHECK_SKILL(ch, spellnum) + (GET_INT(ch) << 1) +
        (GET_REMORT_GEN(ch) << 2);
    if (IS_SICK(ch))
        prob -= 20;
    if (IS_CONFUSED(ch))
        prob -= number(35, 55) + GET_INT(ch);

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
        prob -= GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD));

    prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 8) / CAN_CARRY_W(ch);

    if (tch && GET_POSITION(ch) == POS_FIGHTING)
        prob -= (GET_LEVEL(tch) >> 3);

    /**** casting probability ends here *****/

    /* You throws the dice and you takes your chances.. 101% is total failure */
    if (number(0, 111) > prob) {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (!tch || spellnum == SPELL_ELECTROSTATIC_FIELD
            || !skill_message(0, ch, tch, NULL, spellnum))
            send_to_char(ch, "Your concentration was disturbed!\r\n");
        if (mana > 0)
            GET_MANA(ch) =
                MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
        if (SINFO.violent && tch && IS_NPC(tch))
            hit(tch, ch, TYPE_UNDEFINED);
    } else {
        if (cast_spell(ch, tch, tobj, &tdir, spellnum)) {
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (mana > 0)
                GET_MANA(ch) =
                    MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
            gain_skill_prof(ch, spellnum);
        }
    }
}

ACMD(do_arm)
{
/*	struct creature *tch = NULL;
	struct obj_data *tobj = NULL, *resource = NULL;
	int rpints, spellnum, target = 0, prob = 0;

	if (IS_NPC(ch))
		return;

	if (!IS_MERC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able arm devices!\r\n");
		return;
	}

	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch,
			"Your equipment is too heavy and bulky to arm anything!\r\n");
		return;
	}

    if (!(find_spell_targets(ch, cmd, argument, &spellnum, &targets, &tch, &tobj, &tdir)))
        return;

	if (!SPELL_IS_MERCENARY(spellnum)) {
		act("You don't seem to have that.", false, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target) {
		send_to_char(ch, "I cannot find your target!\r\n");
		return;
	}

	rpoints = mag_manacost(ch, spellnum);
    resource = find_item_kit(ch);

    if (!resource && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "But you don't have a resource kit!\r\n");
        return;
    }

	if ((rpoints > 0) && (GET_RPOINTS(resource) < rpoints)
		&& (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the resources!\r\n");
		return;
	}

	if (room_is_underwater(ch->in_room)
        && SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
		send_to_char(ch, "This device will not function underwater.\r\n");
		return 0;
	}

	prob = CHECK_SKILL(ch, spellnum) + (GET_INT(ch) << 1) +
		(GET_REMORT_GEN(ch) << 2);

	if (IS_SICK(ch))
		prob -= 20;

	if (IS_CONFUSED(ch))
		prob -= number(35, 55) + GET_INT(ch);

	if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
		prob -= GET_EQ(ch, GET_OBJ_WEIGHT(WEAR_SHIELD));

	prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	if (FIGHTING(ch) && (FIGHTING(ch))->getPosition() == POS_FIGHTING)
		prob -= (GET_LEVEL(FIGHTING(ch)) >> 3);

	if (number(0, 111) > prob) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, spellnum)) {
            act("You fumble around in $p and come up with nothing!", false,
                ch, resource, NULL, TO_CHAR);
            return;
        }

		if (rpoints > 0)
			GET_RPOINTS(obj) = MAX(0, GET_RPOINTS(obj) - (rpoints >> 1));

		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} else {
		if (cast_spell(ch, tch, tobj, spellnum)) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (rpoints > 0)
				GET_RPOINTS(obj) = MAX(0, GET_RPOINTS(obj) - mana);
			gain_skill_prof(ch, spellnum);
		}
	}*/
}

struct obj_data *
find_item_kit(struct creature *ch __attribute__ ((unused)))
{
/*    struct obj_data *cur_obj;

    for (int i = 0; i < NUM_WEARS; i++) {
        cur_obj = ch->equipment[i];
        if (cur_obj) {
            if ((GET_OBJ_TYPE(cur_obj == ITEM_KIT)) &&
                (GET_LEVEL(ch) > GET_OBJ_VAL(cur_obj, 0)) &&
                (GET_LEVEL(ch) < GET_OBJ_VAL(cur_obj, 1))) {
                return cur_obj;
            }
        }
    }

    for (struct obj_data = ch->carrying; struct obj_data; struct obj_data = struct obj_data->next) {
        if ((GET_OBJ_TYPE(cur_obj == ITEM_KIT)) &&
            (GET_LEVEL(ch) > GET_OBJ_VAL(cur_obj, 0)) &&
            (GET_LEVEL(ch) < GET_OBJ_VAL(cur_obj, 1))) {
            return cur_obj;
        }
    } */

    return NULL;
}

ACMD(do_alter)
{
    struct creature *tch = NULL;
    struct obj_data *tobj = NULL;
    bool target = false;
    int tdir;
    int mana, spellnum, temp = 0;

    if (IS_NPC(ch))
        return;

    if (!*argument) {
        send_to_char(ch, "You were going to alter a physical law?\r\n");
        return;
    }

    if (GET_CLASS(ch) != CLASS_PHYSIC &&
        GET_REMORT_CLASS(ch) != CLASS_PHYSIC
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch,
            "You are not able to alter the fabric of reality.\r\n");
        return;
    }
    if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
        send_to_char(ch,
            "Your equipment is too heavy and bulky to alter reality!\r\n");
        return;
    }
    if (GET_EQ(ch, WEAR_WIELD) &&
        IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
        send_to_char(ch,
            "You can't alter while wielding a two handed weapon!\r\n");
        return;
    }

    if (!(find_spell_targets(ch, cmd, argument, &spellnum, &target, &tch, &tobj, &tdir)))
        return;

    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOSCIENCE)) &&
        SPELL_IS_PHYSICS(spellnum)) {
        send_to_char(ch,
            "You are unable to alter physical reality in this space.\r\n");
        act("$n tries to solve an elaborate equation, but fails.", false,
            ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (SPELL_USES_GRAVITY(spellnum) && NOGRAV_ZONE(ch->in_room->zone) &&
        SPELL_IS_PHYSICS(spellnum)) {
        send_to_char(ch, "There is no gravity here to alter.\r\n");
        act("$n tries to solve an elaborate equation, but fails.",
            false, ch, NULL, NULL, TO_ROOM);
        return;
    }
    // Drunk bastards don't cast very well, do they... -- Nothing 1/22/2001
    if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
        if (temp < 34) {
            send_to_char(ch,
                "Your mind is too clouded to alter the fabric of reality!\r\n");
            return;
        } else {
            send_to_char(ch, "You feel your concentration slipping!\r\n");
            WAIT_STATE(ch, 2 RL_SEC);
            spellnum = number(1, MAX_SPELLS);
            if (!SPELL_IS_PHYSICS(spellnum)) {
                send_to_char(ch,
                    "Your concentration slips away entirely.\r\n");
                return;
            }
        }
    }

    if (!SPELL_IS_PHYSICS(spellnum)) {
        act("That is not a physical alteration.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (!target) {
        send_to_char(ch, "Cannot find the target of your alteration!\r\n");
        return;
    }
    mana = mag_manacost(ch, spellnum);
    if ((mana > 0) && (GET_MANA(ch) < mana)
        && (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
        send_to_char(ch,
            "You haven't the energy to make that alteration!\r\n");
        return;
    }

    if (room_is_underwater(ch->in_room)
        && SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
        send_to_char(ch, "This alteration will not function underwater.\r\n");
        return;
    }
    if (!OUTSIDE(ch) && SPELL_FLAGGED(spellnum, MAG_OUTDOORS)) {
        send_to_char(ch, "This alteration can only be made outdoors.\r\n");
        return;
    }

    /* You throws the dice and you takes your chances.. 101% is total failure */
    if (number(0, 101) > GET_SKILL(ch, spellnum)) {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (!tch || !skill_message(0, ch, tch, NULL, spellnum))
            send_to_char(ch, "Your concentration was disturbed!\r\n");
        if (mana > 0)
            GET_MANA(ch) =
                MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
        if (SINFO.violent && tch && IS_NPC(tch))
            hit(tch, ch, TYPE_UNDEFINED);
    } else {
        if (cast_spell(ch, tch, tobj, &tdir, spellnum)) {
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (mana > 0)
                GET_MANA(ch) =
                    MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
            gain_skill_prof(ch, spellnum);
        }
    }
}

ACMD(do_perform)
{
    struct creature *tch = NULL;
    struct obj_data *tobj = NULL;
    int tdir;
    bool target = false;
    int mana, spellnum, temp = 0;

    extern bool check_instrument(struct creature *ch, int songnum);
    extern char *get_instrument_type(int songnum);

    if (IS_NPC(ch))
        return;

    if (!*argument) {
        send_to_char(ch, "You were going to perform something?\r\n");
        return;
    }

    if (GET_CLASS(ch) != CLASS_BARD &&
        GET_REMORT_CLASS(ch) != CLASS_BARD && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "You raise your pitiful voice to the heavens.\r\n");
        return;
    }
    if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
        send_to_char(ch,
            "Your heavy equipment prevents you from breathing properly!\r\n");
        return;
    }

    if (!(find_spell_targets(ch, cmd, argument, &spellnum, &target, &tch, &tobj, &tdir)))
        return;

    // Drunk bastards don't sing very well, do they... -- Nothing 8/28/2004
    if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
        if (temp < 34) {
            send_to_char(ch, "You try but you can't remember the tune!\r\n");
            return;
        } else {
            send_to_char(ch,
                "You begin to play but you can't hold the tune!\r\n");
            WAIT_STATE(ch, 2 RL_SEC);
            spellnum = number(1, MAX_SPELLS);
            if (!SPELL_IS_BARD(spellnum)) {
                send_to_char(ch,
                    "Your concentration slips away entirely.\r\n");
                return;
            }
        }
    }

    if (tch && IS_UNDEAD(tch)) {
        send_to_char(ch, "The undead have no appreciation for music!\r\n");
        return;
    }

    if (!SPELL_IS_BARD(spellnum)) {
        act("You do not know that song!", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if ((GET_LEVEL(ch) < LVL_AMBASSADOR && (GET_LEVEL(ch) > 10)) &&
        !check_instrument(ch, spellnum)) {
        send_to_char(ch, "But you're not using a %s instrument!\r\n",
            get_instrument_type(spellnum));
        return;
    }
    if (!target) {
        send_to_char(ch, "Cannot find the target of your serenade!\r\n");
        return;
    }
    mana = mag_manacost(ch, spellnum);
    if ((mana > 0) && (GET_MANA(ch) < mana)
        && (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
        send_to_char(ch, "You haven't the breath to perform that song!\r\n");
        return;
    }

    if (room_is_underwater(ch->in_room)) {
        send_to_char(ch, "You can't sing or play underwater!\r\n");
        return;
    }

    /* You throws the dice and you takes your chances.. 101% is total failure */
    if (number(0, 101) > GET_SKILL(ch, spellnum)) {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (!tch || !skill_message(0, ch, tch, NULL, spellnum))
            send_to_char(ch, "Your song was disturbed!\r\n");
        if (mana > 0)
            GET_MANA(ch) =
                MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
        if (SINFO.violent && tch && IS_NPC(tch))
            hit(tch, ch, TYPE_UNDEFINED);
    } else {
        if (cast_spell(ch, tch, tobj, &tdir, spellnum)) {
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (mana > 0)
                GET_MANA(ch) =
                    MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
            gain_skill_prof(ch, spellnum);
        }
    }
}

/* Assign the spells on boot up */

bool
load_spell(xmlNodePtr node)
{
    int parse_player_class(char *);
    int idnum = xmlGetIntProp(node, "id", -1);
    xmlNodePtr child;

    if (idnum < 1) {
        errlog("Spell idnum was not specified!");
        return false;
    }
    if (idnum > TOP_SPELL_DEFINE) {
        errlog("Spell idnum %d > TOP_SPELL!", idnum);
        return false;
    }
    // for defined classes, initialize minimum level to ambassador
    for (int class_idx = 0; class_idx < NUM_CLASSES; class_idx++)
        spell_info[idnum].min_level[class_idx] = LVL_AMBASSADOR;

    spells[idnum] = (char *)xmlGetProp(node, (xmlChar *) "name");
    for (child = node->children; child; child = child->next) {
        if (xmlMatches(child->name, "granted")) {
            char *class_str = (char *)xmlGetProp(child, (xmlChar *) "class");
            int level = xmlGetIntProp(child, "level", 0);
            int gen = xmlGetIntProp(child, "gen", 0);
            int class_idx = parse_player_class(class_str);

            if (class_idx < 0) {
                errlog("Granted class '%s' is not a valid class in spell!",
                    class_str);
                free(class_str);
                return false;
            }
            free(class_str);

            if (level < 1 || level > LVL_AMBASSADOR) {
                errlog("Granted level %d is not a valid level", level);
                return false;
            }
            if (gen > 10) {
                errlog("Granted gen %d is not a valid gen", gen);
                return false;
            }
            spell_info[idnum].min_level[class_idx] = level;
            spell_info[idnum].gen[class_idx] = gen;
        } else if (xmlMatches(child->name, "manacost")) {
            spell_info[idnum].mana_max = xmlGetIntProp(child, "initial", 0);
            spell_info[idnum].mana_change =
                xmlGetIntProp(child, "level_dec", 0);
            spell_info[idnum].mana_min = xmlGetIntProp(child, "minimum", 0);
        } else if (xmlMatches(child->name, "position")) {
            char *pos_str = (char *)xmlGetProp(child, (xmlChar *) "minimum");

            if (!pos_str) {
                errlog
                    ("Required property minimum missing from position element");
                return false;
            }

            int pos = search_block(pos_str, position_types, false);
            if (pos < 0) {
                errlog("Invalid minimum position '%s' for spell", pos_str);
                free(pos_str);
                return false;
            }
            free(pos_str);
            spell_info[idnum].min_position = pos;
        } else if (xmlMatches(child->name, "target")) {
            char *type_str = (char *)xmlGetProp(child, (xmlChar *) "type");
            char *scope_str = (char *)xmlGetProp(child, (xmlChar *) "scope");

            if (!strcmp(type_str, "door")) {
                spell_info[idnum].targets |= TAR_DOOR;
            } else if (!strcmp(type_str, "direction")) {
                spell_info[idnum].targets |= TAR_DIR;
            } else if (!strcmp(type_str, "self")) {
                if (!strcmp(scope_str, "fighting"))
                    spell_info[idnum].targets |= TAR_FIGHT_SELF;
                else if (!strcmp(scope_str, "only"))
                    spell_info[idnum].targets |= TAR_SELF_ONLY;
                else if (!strcmp(scope_str, "never"))
                    spell_info[idnum].targets |= TAR_NOT_SELF;
            } else if (!strcmp(type_str, "creature")) {
                if (!strcmp(scope_str, "room")) {
                    spell_info[idnum].targets |= TAR_CHAR_ROOM;
                } else if (!strcmp(scope_str, "world")) {
                    spell_info[idnum].targets |= TAR_CHAR_WORLD;
                } else if (!strcmp(scope_str, "fighting")) {
                    spell_info[idnum].targets |= TAR_FIGHT_VICT;
                }
            } else if (!strcmp(type_str, "object")) {
                if (!strcmp(scope_str, "room")) {
                    spell_info[idnum].targets |= TAR_OBJ_ROOM;
                } else if (!strcmp(scope_str, "world")) {
                    spell_info[idnum].targets |= TAR_OBJ_WORLD;
                } else if (!strcmp(scope_str, "inventory")) {
                    spell_info[idnum].targets |= TAR_OBJ_INV;
                } else if (!strcmp(scope_str, "equip")) {
                    spell_info[idnum].targets |= TAR_OBJ_EQUIP;
                }
            }
            free(type_str);
            free(scope_str);
        } else if (xmlMatches(child->name, "flag")) {
            char *value_str = (char *)xmlGetProp(child, (xmlChar *) "value");

            if (!strcmp(value_str, "violent")) {
                spell_info[idnum].violent = true;
            } else if (!strcmp(value_str, "unpleasant")) {
                spell_info[idnum].targets |= TAR_UNPLEASANT;
            } else {
                int flag = search_block(value_str, spell_bit_keywords, true);
                if (flag < 0) {
                    errlog("Invalid flag '%s' in spell", value_str);
                    free(value_str);
                    return false;
                }
                spell_info[idnum].routines |= (1 << flag);
            }
            free(value_str);
        } else if (xmlMatches(child->name, "instrument")) {
            char *type_str = (char *)xmlGetProp(child, (xmlChar *) "type");

            if (!strcmp(type_str, "wind")) {
                songs[idnum].type = ITEM_WIND;
            } else if (!strcmp(type_str, "percussion")) {
                songs[idnum].type = ITEM_PERCUSSION;
            } else if (!strcmp(type_str, "string")) {
                songs[idnum].type = ITEM_STRING;
            } else {
                errlog("Invalid instrument type '%s' in spell", type_str);
                free(type_str);
                return false;
            }
            free(type_str);
        } else if (xmlMatches(child->name, "description")) {
            char *text = (char *)xmlNodeGetContent(child);

            songs[idnum].lyrics = strdup(text);
            songs[idnum].instrumental = true;
            free(text);
        } else if (xmlMatches(child->name, "lyrics")) {
            char *text = (char *)xmlNodeGetContent(child);

            songs[idnum].lyrics = strdup(text);
            songs[idnum].instrumental = false;
            free(text);
        }
    }

    if (!spell_info[idnum].targets)
        spell_info[idnum].targets = TAR_IGNORE;

    return true;
}

char *UNUSED_SPELL_NAME = NULL;

void
clear_spells(void)
{
    if (!UNUSED_SPELL_NAME)
        UNUSED_SPELL_NAME = strdup("!UNUSED!");

    for (int spl = 1; spl < TOP_SPELL_DEFINE; spl++) {
        if (spells[spl] != UNUSED_SPELL_NAME)
            free(spells[spl]);
        spells[spl] = UNUSED_SPELL_NAME;
        for (int class_idx = 0; class_idx < NUM_CLASSES; class_idx++) {
            spell_info[spl].min_level[class_idx] = LVL_GRIMP + 1;
            spell_info[spl].gen[class_idx] = 0;
        }
        spell_info[spl].mana_max = 0;
        spell_info[spl].mana_min = 0;
        spell_info[spl].mana_change = 0;
        spell_info[spl].min_position = 0;
        spell_info[spl].targets = 0;
        spell_info[spl].violent = 0;
        spell_info[spl].routines = 0;
        if (songs[spl].lyrics)
            free(songs[spl].lyrics);
        songs[spl].lyrics = NULL;
        songs[spl].instrumental = false;
        songs[spl].type = 0;
    }

    // Initialize string list terminator
    spells[0] = "!RESERVED!";
    spells[TOP_SPELL_DEFINE] = "\n";
}

void
boot_spells(const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;
    int num_spells = 0;

    free(spells);
    spells = calloc(TOP_SPELL_DEFINE + 1, sizeof(const char *));
    memset(spells, 0, sizeof(char *) * (TOP_SPELL_DEFINE + 1));
    memset(spell_info, 0, sizeof(spell_info));
    memset(songs, 0, sizeof(songs));
    clear_spells();

    doc = xmlParseFile(path);
    if (!doc) {
        errlog("Couldn't load %s", path);
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("%s is empty", path);
        return;
    }

    for (node = node->children; node; node = node->next) {
        // Parse different nodes here.
        if (!xmlMatches(node->name, "spell") &&
            !xmlMatches(node->name, "skill"))
            continue;

        if (load_spell(node))
            num_spells++;
    }

    xmlFreeDoc(doc);
}

#undef __spell_parser_c__
