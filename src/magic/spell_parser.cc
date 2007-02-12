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
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "materials.h"
#include "char_class.h"
#include "fight.h"
#include "screen.h"
#include "tmpstr.h"
#include "vendor.h"
#include "prog.h"

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct room_direction_data *knock_door = NULL;
char locate_buf[256];

#define SINFO spell_info[spellnum]

extern int mini_mud;

extern struct room_data *world;
int find_door(struct Creature *ch, char *type, char *dir,
	const char *cmdname);
void name_from_drinkcon(struct obj_data *obj);
struct obj_data *find_item_kit(Creature *ch);
int perform_taint_burn(Creature *ch, int spellnum);
/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, this should provide
 * ample slots for skills.
 */

int max_spell_num = 0;

const char **spells = new const char *[TOP_SPELL_DEFINE + 1];

struct syllable {
	char *org;
	char *new_syl;
};


struct syllable syls[] = {
	{" ", " "},
	{"ar", "abra"},
	{"ate", "i"},
	{"cau", "kada"},
	{"blind", "nose"},
	{"bur", "mosa"},
	{"cu", "judi"},
	{"de", "oculo"},
	{"dis", "mar"},
	{"ect", "kamina"},
	{"en", "uns"},
	{"gro", "cra"},
	{"light", "dies"},
	{"lo", "hi"},
	{"magi", "kari"},
	{"mon", "bar"},
	{"mor", "zak"},
	{"move", "sido"},
	{"ness", "lacri"},
	{"ning", "illa"},
	{"per", "duda"},
	{"ra", "gru"},
	{"re", "candus"},
	{"son", "sabru"},
	{"tect", "infra"},
	{"tri", "cula"},
	{"ven", "nofo"},
	{"word of", "inset"},
	{"clair", "alto'k"},
	{"ning", "gzapta"},
	{"vis", "tappa"},
	{"light", "lumo"},
	{"fly", "a lifto"},
	{"align", "iptor "},
	{"prot", "provec "},
	{"fire", "infverrn'o"},
	{"heat", "skaaldr"},
	{"divine", "hohhlguihar"},
	{"sphere", "en'kompaz"},
	{"animate", "ak'necro pa"},
	{"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"},
		{"g", "t"},
	{"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"},
		{"n", "b"},
	{"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
		{"u", "e"},
	{"v", "z"},
	{"w", "x"},
	{"x", "n"},
	{"y", "l"},
	{"z", "k"},
	{"-", "rr't"},
	{"", ""}
};

int
mag_manacost(struct Creature *ch, int spellnum)
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


/* say_spell erodes buf, buf1, buf2 */
void
say_spell(struct Creature *ch, int spellnum, struct Creature *tch,
	struct obj_data *tobj)
{
	char lbuf[256], xbuf[256];
	int j, ofs = 0;

	*buf = '\0';
	strcpy(lbuf, spell_to_str(spellnum));

	while (*(lbuf + ofs)) {
		for (j = 0; *(syls[j].org); j++) {
			if (!strncasecmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
				strcat(buf, syls[j].new_syl);
				ofs += strlen(syls[j].org);
			}
		}
	}

	/* Magic Casting messages... */

	if (SPELL_IS_PSIONIC(spellnum)) {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(buf2,
					"$n momentarily closes $s eyes and concentrates.");
				send_to_char(ch, "You close your eyes and concentrate.\r\n");
			} else {
				sprintf(buf2,
					"$n closes $s eyes and touches $N with a mental finger.");
				act("You close your eyes and touch $N with a mental finger.",
					FALSE, ch, 0, tch, TO_CHAR);
			}
		} else if (tobj != NULL && tobj->in_room == ch->in_room) {
			sprintf(buf2,
				"$n closes $s eyes and touches $p with a mental finger.");
			act("You close your eyes and touch $p with a mental finger.",
				FALSE, ch, tobj, 0, TO_CHAR);
		} else {
			sprintf(buf2,
				"$n closes $s eyes and slips into the psychic world.");
			send_to_char(ch, 
				"You close your eyes and slip into a psychic world.\r\n");
		}
	} else if (SPELL_IS_PHYSICS(spellnum)) {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(buf2,
					"$n momentarily closes $s eyes and concentrates.");
				send_to_char(ch, "You close your eyes and make a calculation.\r\n");
			} else {
				sprintf(buf2, "$n looks at $N and makes a calculation.");
				act("You look at $N and make a calculation.", FALSE, ch, 0,
					tch, TO_CHAR);
			}
		} else if (tobj != NULL && tobj->in_room == ch->in_room) {
			sprintf(buf2, "$n looks directly at $p and makes a calculation.");
			act("You look directly at $p and make a calculation.", FALSE, ch,
				tobj, 0, TO_CHAR);
		} else {
			sprintf(buf2,
				"$n closes $s eyes and slips into a deep calculation.");
			send_to_char(ch, 
				"You close your eyes and make a deep calculation.\r\n");
		}
	} else {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(lbuf,
					"$n closes $s eyes and utters the words, '%%s'.");
				sprintf(xbuf, "You close your eyes and utter the words, '%s'.",
					buf);
			} else {
				sprintf(lbuf, "$n stares at $N and utters, '%%s'.");
				sprintf(xbuf, "You stare at $N and utter, '%s'.", buf);
			}
		} else if (tobj != NULL &&
			((tobj->in_room == ch->in_room) || (tobj->carried_by == ch))) {
			sprintf(lbuf, "$n stares at $p and utters the words, '%%s'.");
			sprintf(xbuf, "You stare at $p and utter the words, '%s'.", buf);
		} else {
			sprintf(lbuf, "$n utters the words, '%%s'.");
			sprintf(xbuf, "You utter the words, '%s'.", buf);
		}

		act(xbuf, FALSE, ch, tobj, tch, TO_CHAR);
		sprintf(buf1, lbuf, spell_to_str(spellnum));
		sprintf(buf2, lbuf, buf);
	}

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (*it == ch || *it == tch || !(*it)->desc || !AWAKE((*it)) ||
			PLR_FLAGGED((*it), PLR_WRITING | PLR_OLC))
			continue;
		perform_act(buf2, ch, tobj, tch, (*it), 0);
	}

	if (tch != NULL && tch != ch && tch->in_room == ch->in_room) {
		if (SPELL_IS_PSIONIC(spellnum)) {
			sprintf(buf1, "$n closes $s eyes and connects with your mind.");
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		} else if (SPELL_IS_PHYSICS(spellnum)) {
			sprintf(buf1,
				"$n closes $s eyes and alters the reality around you.");
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		} else {
			sprintf(buf1, "$n stares at you and utters the words, '%s'.", buf);
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		}
	}
}

// 06/18/99
// reversed order of args to is_abbrev
// inside while. 
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
 * return_flags has CALL_MAGIC_VICT_KILLED bit set if cvict dies
 *
 */
int
call_magic(struct Creature *caster, struct Creature *cvict,
	struct obj_data *ovict, int *dvict, int spellnum, int level, int casttype,
	int *return_flags)
{

	int savetype, mana = -1;
	bool same_vict = false;
	struct affected_type *af_ptr = NULL;

	if (return_flags)
		*return_flags = 0;

	if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
		return 0;

	if (GET_ROOM_PROGOBJ(caster->in_room) != NULL) {
	    if (trigger_prog_spell(caster->in_room, PROG_TYPE_ROOM, caster, spellnum)) {
            return 0; //handled
        }
    }
    
    CreatureList::iterator it = caster->in_room->people.begin();
	for (; it != caster->in_room->people.end(); ++it) {
		if (GET_MOB_PROGOBJ((*it)) != NULL) {
			if (trigger_prog_spell(*it, PROG_TYPE_MOBILE, caster, spellnum)) {
                return 0;
            }
        }
	}
    
    if (ovict) {
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOMAG)) {
            if  ((SPELL_IS_MAGIC(spellnum) || SPELL_IS_DIVINE(spellnum)) && 
                 spellnum != SPELL_IDENTIFY) {
                act("$p hums and shakes for a moment "
                    "then rejects your spell!", TRUE, caster, ovict, 0, TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's spell!", TRUE, caster, ovict, 0, TO_ROOM); 
                return 0;
            }
        }
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOSCI)) {
            if (SPELL_IS_PHYSICS(spellnum)) {
                act("$p hums and shakes for a moment "
                    "then rejects your alteration!", TRUE, caster, ovict, 0, TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's alteration!", TRUE, caster, ovict, 0, TO_ROOM); 
                return 0;
            }
        }
    }

	/* stuff to check caster vs. cvict */
	if (cvict && caster != cvict) {
		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
			!caster->isOkToAttack(cvict, false)) {
			if (SPELL_IS_PSIONIC(spellnum)) {
				send_to_char(caster, "The Universal Psyche descends on your mind and "
					"renders you powerless!\r\n");
				act("$n concentrates for an instant, and is suddenly thrown "
					"into mental shock!\r\n", FALSE, caster, 0, 0, TO_ROOM);
				return 0;
			}
			if (SPELL_IS_PHYSICS(spellnum)) {
				send_to_char(caster, 
					"The Supernatural Reality prevents you from twisting "
					"nature in that way!\r\n");
				act("$n attempts to violently alter reality, but is restrained "
					"by the whole of the universe.", FALSE, caster, 0, 0,
					TO_ROOM);
				return 0;
			} 
            if (SPELL_IS_BARD(spellnum)) {
				send_to_char(caster, 
					"Your voice is stifled!\r\n");
				act("$n attempts to sing a violent song, but is restrained "
					"by the whole of the universe.", FALSE, caster, 0, 0,
					TO_ROOM);
				return 0;
            } else {
				send_to_char(caster, 
					"A flash of white light fills the room, dispelling your "
					"violent magic!\r\n");
				act("White light from no particular source suddenly fills the room, " "then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
				return 0;
			}
		}

		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
			!ok_damage_vendor(caster, cvict))
			return 0;

        if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
            caster->checkReputations(cvict))
            return 0;

		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
			check_killer(caster, cvict);
            //Try to make this a little more sane...
            if (cvict->distrusts(caster) &&
				AFF3_FLAGGED(cvict, AFF3_PSISHIELD) && 
                (SPELL_IS_PSIONIC(spellnum) || casttype == CAST_PSIONIC)) {
                bool failed = false;
                int prob, percent;
                
                if (spellnum == SPELL_PSIONIC_SHATTER && 
                    !mag_savingthrow(cvict, level, SAVING_PSI))
                    failed = true;
                
                prob = CHECK_SKILL(caster, spellnum) + GET_INT(caster);
                prob += caster->getLevelBonus(spellnum);

                percent = cvict->getLevelBonus(SPELL_PSISHIELD);
                percent += number(1, 120);

                if (mag_savingthrow(cvict, GET_LEVEL(caster), SAVING_PSI))
                    percent <<= 1;

                if (GET_INT(cvict) < GET_INT(caster))
                    percent += (GET_INT(cvict) - GET_INT(caster)) << 3;

                if (percent >= prob)
                    failed = true;

                if (failed) {
                    act("Your psychic attack is deflected by $N's psishield!",
                        FALSE, caster, 0, cvict, TO_CHAR);
                    act("$n's psychic attack is deflected by $N's psishield!",
                        FALSE, caster, 0, cvict, TO_NOTVICT);
                    act("$n's psychic attack is deflected by your psishield!",
                        FALSE, caster, 0, cvict, TO_VICT);
                    return 0; 
                }
            }
		}
		if (cvict->distrusts(caster)) {
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
					false, caster, 0, cvict, TO_NOTVICT);
				act(tmp_sprintf("Your %s absorbs $n's %s!",
						spell_to_str(af_ptr->type),
						spell_to_str(spellnum)),
					false, caster, 0, cvict, TO_VICT);
				act(tmp_sprintf("$N's %s absorbs your %s!",
						spell_to_str(af_ptr->type),
						spell_to_str(spellnum)),
					false, caster, 0, cvict, TO_CHAR);
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
					send_to_char(cvict, "Your %s dissolves.\r\n", spell_to_str(af_ptr->type));
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
		if (caster == cvict)
			same_vict = true;

		int retval = mag_damage(level, caster, cvict, spellnum, savetype);

		//
		// somebody died (either caster or cvict)
		//

		if (retval) {

			if (IS_SET(retval, DAM_ATTACKER_KILLED) ||
				(IS_SET(retval, DAM_VICT_KILLED) && same_vict)) {
				if (return_flags) {
					*return_flags = retval | DAM_ATTACKER_KILLED;
				}

				return 1;
			}

			if (return_flags) {
				*return_flags = retval;
			}
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
		int retval = mag_areas(level, caster, spellnum, savetype);
		if (retval) {
			if (IS_SET(retval, DAM_ATTACKER_KILLED)
				|| (IS_SET(retval, DAM_VICT_KILLED) && same_vict)) {
				if (return_flags) {
					*return_flags = retval | DAM_ATTACKER_KILLED;
				}
				return 1;
			}
			if (return_flags) {
				*return_flags = retval;
			}
			return 1;
		}
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
			MANUAL_SPELL(spell_astral_spell); break;
		case SPELL_ENCHANT_WEAPON:
			MANUAL_SPELL(spell_enchant_weapon); break;
		case SPELL_CHARM:
			MANUAL_SPELL(spell_charm); break;
		case SPELL_CHARM_ANIMAL:
			MANUAL_SPELL(spell_charm_animal); break;
		case SPELL_WORD_OF_RECALL:
			MANUAL_SPELL(spell_recall); break;
		case SPELL_IDENTIFY:
			MANUAL_SPELL(spell_identify); break;
		case SPELL_SUMMON:
			MANUAL_SPELL(spell_summon); break;
		case SPELL_LOCATE_OBJECT:
			MANUAL_SPELL(spell_locate_object); break;
		case SPELL_ENCHANT_ARMOR:
			MANUAL_SPELL(spell_enchant_armor); break;
		case SPELL_GREATER_ENCHANT:
			MANUAL_SPELL(spell_greater_enchant); break;
		case SPELL_MINOR_IDENTIFY:
			MANUAL_SPELL(spell_minor_identify); break;
		case SPELL_CLAIRVOYANCE:
			MANUAL_SPELL(spell_clairvoyance); break;
		case SPELL_MAGICAL_VESTMENT:
			MANUAL_SPELL(spell_magical_vestment); break;
		case SPELL_TELEPORT:
		case SPELL_RANDOM_COORDINATES:
			MANUAL_SPELL(spell_teleport); break;
		case SPELL_CONJURE_ELEMENTAL:
			MANUAL_SPELL(spell_conjure_elemental); break;
		case SPELL_KNOCK:
			MANUAL_SPELL(spell_knock); break;
		case SPELL_SWORD:
			MANUAL_SPELL(spell_sword); break;
		case SPELL_ANIMATE_DEAD:
			MANUAL_SPELL(spell_animate_dead); break;
		case SPELL_CONTROL_WEATHER:
			MANUAL_SPELL(spell_control_weather); break;
		case SPELL_GUST_OF_WIND:
			MANUAL_SPELL(spell_gust_of_wind); break;
		case SPELL_RETRIEVE_CORPSE:
			MANUAL_SPELL(spell_retrieve_corpse); break;
		case SPELL_LOCAL_TELEPORT:
			MANUAL_SPELL(spell_local_teleport); break;
		case SPELL_PEER:
			MANUAL_SPELL(spell_peer); break;
		case SPELL_VESTIGIAL_RUNE:
			MANUAL_SPELL(spell_vestigial_rune); break;
		case SPELL_ID_INSINUATION:
			MANUAL_SPELL(spell_id_insinuation); break;
		case SPELL_SHADOW_BREATH:
			MANUAL_SPELL(spell_shadow_breath); break;
		case SPELL_SUMMON_LEGION:
			MANUAL_SPELL(spell_summon_legion); break;
		case SPELL_NUCLEAR_WASTELAND:
			MANUAL_SPELL(spell_nuclear_wasteland); break;
		case SPELL_SPACETIME_IMPRINT:
			MANUAL_SPELL(spell_spacetime_imprint); break;
		case SPELL_SPACETIME_RECALL:
			MANUAL_SPELL(spell_spacetime_recall); break;
		case SPELL_TIME_WARP:
			MANUAL_SPELL(spell_time_warp); break;
		case SPELL_UNHOLY_STALKER:
			MANUAL_SPELL(spell_unholy_stalker); break;
		case SPELL_CONTROL_UNDEAD:
			MANUAL_SPELL(spell_control_undead); break;
		case SPELL_INFERNO:
			MANUAL_SPELL(spell_inferno); break;
		case SPELL_BANISHMENT:
			MANUAL_SPELL(spell_banishment); break;
		case SPELL_AREA_STASIS:
			MANUAL_SPELL(spell_area_stasis); break;
		case SPELL_SUN_RAY:
			MANUAL_SPELL(spell_sun_ray); break;
		case SPELL_EMP_PULSE:
			MANUAL_SPELL(spell_emp_pulse); break;
		case SPELL_QUANTUM_RIFT:
			MANUAL_SPELL(spell_quantum_rift); break;
		case SPELL_DEATH_KNELL:
			MANUAL_SPELL(spell_death_knell); break;
		case SPELL_DISPEL_MAGIC:
			MANUAL_SPELL(spell_dispel_magic); break;
		case SPELL_DISTRACTION:
			MANUAL_SPELL(spell_distraction); break;
		case SPELL_BLESS:
			MANUAL_SPELL(spell_bless); break;
		case SPELL_DAMN:
			MANUAL_SPELL(spell_damn); break;
        case SPELL_CALM:
            MANUAL_SPELL(spell_calm); break;
		case SPELL_CALL_RODENT:
			MANUAL_SPELL(spell_call_rodent); break;
		case SPELL_CALL_BIRD:
			MANUAL_SPELL(spell_call_bird); break;
		case SPELL_CALL_REPTILE:
			MANUAL_SPELL(spell_call_reptile); break;
		case SPELL_CALL_BEAST:
			MANUAL_SPELL(spell_call_beast); break;
		case SPELL_CALL_PREDATOR:
			MANUAL_SPELL(spell_call_predator); break;
        case SPELL_DISPEL_EVIL:
            MANUAL_SPELL(spell_dispel_evil); break;
        case SPELL_DISPEL_GOOD:
            MANUAL_SPELL(spell_dispel_good); break;
        case SONG_EXPOSURE_OVERTURE:
            MANUAL_SPELL(song_exposure_overture); break;
		case SONG_HOME_SWEET_HOME:
			MANUAL_SPELL(spell_recall); break;
		case SONG_CLARIFYING_HARMONIES:
			MANUAL_SPELL(spell_identify); break;
        case SONG_LAMENT_OF_LONGING:
            MANUAL_SPELL(song_lament_of_longing); break;
        case SONG_UNRAVELLING_DIAPASON:
            MANUAL_SPELL(spell_dispel_magic); break;
        case SONG_INSTANT_AUDIENCE:
            MANUAL_SPELL(song_instant_audience); break;
        case SONG_RHYTHM_OF_ALARM:
            MANUAL_SPELL(song_rhythm_of_alarm); break;
        case SONG_WALL_OF_SOUND:
            MANUAL_SPELL(song_wall_of_sound); break;
        case SONG_HYMN_OF_PEACE:
            MANUAL_SPELL(song_hymn_of_peace); break;

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
 * potion - [0] level        [1] spell num        [2] spell num        [3] spell num
 * syringe- [0] level        [1] spell num        [2] spell num        [3] spell num
 * pill   - [0] level        [1] spell num        [2] spell num        [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 *
 * Returns 0 when victim dies, or otherwise an abort is needed.
 */

int
mag_objectmagic(struct Creature *ch, struct obj_data *obj,
	char *argument, int *return_flags)
{
	int i, k, level;
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
	int my_return_flags = 0;
	if (return_flags == NULL)
		return_flags = &my_return_flags;

	one_argument(argument, arg);

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		FIND_OBJ_EQUIP, ch, &tch, &tobj);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_STAFF:
		act("You tap $p three times on the ground.", FALSE, ch, obj, 0,
			TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, 0, TO_ROOM);
		else
			act("$n taps $p three times on the ground.", FALSE, ch, obj, 0,
				TO_ROOM);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act(NOEFFECT, FALSE, ch, obj, 0, TO_ROOM);
		} else {

			if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
				|| invalid_char_class(ch, obj)
				|| (GET_INT(ch) + CHECK_SKILL(ch,
						SKILL_USE_WANDS)) < number(20, 100)) {
				act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_CHAR);
				act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_ROOM);
				GET_OBJ_VAL(obj, 2)--;
				return 1;
			}
			gain_skill_prof(ch, SKILL_USE_WANDS);
			GET_OBJ_VAL(obj, 2)--;
			WAIT_STATE(ch, PULSE_VIOLENCE);

			level =
				(GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_USE_WANDS)) / 100;
			level = MIN(level, LVL_AMBASSADOR);


			room_data *room = ch->in_room;
			CreatureList::iterator it = room->people.begin();
			for (; it != room->people.end(); ++it) {
				if (ch == *it && spell_info[GET_OBJ_VAL(obj, 3)].violent)
					continue;
				if (level)
					call_magic(ch, (*it), NULL, NULL, GET_OBJ_VAL(obj, 3),
						level, CAST_STAFF);
				else
					call_magic(ch, (*it), NULL, NULL, GET_OBJ_VAL(obj, 3),
						DEFAULT_STAFF_LVL, CAST_STAFF);
			}
		}
		break;
	case ITEM_WAND:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
				if (obj->action_desc != NULL)
					act(obj->action_desc, FALSE, ch, obj, tch, TO_ROOM);
				else {
					act("$n points $p at $N.", TRUE, ch, obj, tch, TO_NOTVICT);
					act("$n points $p at you.", TRUE, ch, obj, tch, TO_VICT);
				}
			}
		} else if (tobj != NULL) {
			act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
			if (obj->action_desc != NULL)
				act(obj->action_desc, FALSE, ch, obj, tobj, TO_ROOM);
			else
				act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
		} else {
			act("At what should $p be pointed?", FALSE, ch, obj, NULL,
				TO_CHAR);
			return 1;
		}

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act(NOEFFECT, FALSE, ch, obj, 0, TO_ROOM);
			return 1;
		}

		if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			|| invalid_char_class(ch, obj)
			|| (GET_INT(ch) + CHECK_SKILL(ch, SKILL_USE_WANDS)) < number(20,
				100)) {
			act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_CHAR);
			act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_ROOM);
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
				act("There is nothing to here to affect with $p.", FALSE,
					ch, obj, NULL, TO_CHAR);
				return 1;
			}
		} else
			tch = ch;

		act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else if (tch && tch != ch) {
			act("$n recites $p in your direction.", FALSE, ch, obj, tch,
				TO_VICT);
			act("$n recites $p in $N's direction.", FALSE, ch, obj, tch,
				TO_NOTVICT);
		} else if (tobj) {
			act("$n looks at $P and recites $p.", FALSE, ch, obj, tobj,
				TO_ROOM);
		} else
			act("$n recites $p.", FALSE, ch, obj, 0, TO_ROOM);

		if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			|| invalid_char_class(ch, obj)
			|| (GET_INT(ch) + CHECK_SKILL(ch, SKILL_READ_SCROLLS)) < number(20,
				100)) {
			act("$p flashes and smokes for a moment, then is gone.", FALSE, ch,
				obj, 0, TO_CHAR);
			act("$p flashes and smokes for a moment before dissolving.", FALSE,
				ch, obj, 0, TO_ROOM);
			if (obj)
				extract_obj(obj);
			return 1;
		}

		level =
			(GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_READ_SCROLLS)) / 100;
		level = MIN(level, LVL_AMBASSADOR);

		WAIT_STATE(ch, PULSE_VIOLENCE);
		gain_skill_prof(ch, SKILL_READ_SCROLLS);
		for (i = 1; i < 4; i++) {
			call_magic(ch, tch, tobj, NULL, GET_OBJ_VAL(obj, i),
				level, CAST_SCROLL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
		}
		extract_obj(obj);
		break;
	case ITEM_FOOD:
		if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
			act("You feel a strange sensation as you eat $p...", FALSE,
				ch, obj, NULL, TO_CHAR);
			return (call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, 2),
					GET_OBJ_VAL(obj, 1), CAST_POTION));
		}
		break;
	case ITEM_POTION:
		tch = ch;
		act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
			for (i = 1; i < 4; i++) {
				if (!ch->in_room)
					break;
				call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
					GET_OBJ_VAL(obj, 0), CAST_POTION, &my_return_flags);
				if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
					IS_SET(my_return_flags, DAM_VICT_KILLED)) {
					*return_flags = my_return_flags;
					break;
				}
			}
		}
		extract_obj(obj);
		break;
	case ITEM_SYRINGE:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				act("You jab $p into your arm and press the plunger.",
					FALSE, ch, obj, 0, TO_CHAR);
				act("$n jabs $p into $s arm and depresses the plunger.",
					FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You jab $p into $N's arm and press the plunger.",
					FALSE, ch, obj, tch, TO_CHAR);
				act("$n jabs $p into $N's arm and presses the plunger.",
					TRUE, ch, obj, tch, TO_NOTVICT);
				act("$n jabs $p into your arm and presses the plunger.",
					TRUE, ch, obj, tch, TO_VICT);
			}
		} else {
			act("Who do you want to inject with $p?", FALSE, ch, obj, NULL,
				TO_CHAR);
			return 1;
		}

		if (obj->action_desc != NULL)
			act(obj->action_desc, FALSE, ch, obj, tch, TO_VICT);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		for (i = 1; i < 4; i++) {
			call_magic(ch, tch, NULL, NULL, GET_OBJ_VAL(obj, i),
				GET_OBJ_VAL(obj, 0),
				IS_OBJ_STAT(obj, ITEM_MAGIC) ? CAST_SPELL :
				CAST_INTERNAL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
		}
		extract_obj(obj);
		break;

	case ITEM_PILL:
		tch = ch;
		act("You swallow $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n swallows $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		for (i = 1; i < 4; i++) {
			call_magic(ch, ch, NULL, NULL, GET_OBJ_VAL(obj, i),
				GET_OBJ_VAL(obj, 0), CAST_INTERNAL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
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
cast_spell(struct Creature *ch, struct Creature *tch,
	struct obj_data *tobj, int *tdir, int spellnum, int *return_flags)
{

    void sing_song(struct Creature *ch, Creature *vict, 
                   struct obj_data *ovict, int spellnum);

	if (return_flags)
		*return_flags = 0;

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
				WAIT_STATE( ch, (3 RL_SEC) ); //PULSE_VIOLENCE);
			}
		}
	}
	//
	// verify correct position
	//

	if (ch->getPosition() < SINFO.min_position) {
		switch (ch->getPosition()) {
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
			if (IS_MOB(ch)) {
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
	if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tch)) {
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
            SPELL_IS_BARD(spellnum) ? "sing any songs!" :
			"cast any spells!");
		return 0;
	}
	if (IS_SET(SINFO.routines, MAG_GROUPS) && !IS_AFFECTED(ch, AFF_GROUP) && 
        !IS_SET(SINFO.routines, MAG_BARD)) {
		send_to_char(ch, "You can't do this if you're not in a group!\r\n");
		return 0;
	}
	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN)
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

	if (!IS_MOB(ch) && GET_LEVEL(ch) >= LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD && !mini_mud &&
		(!tch || GET_LEVEL(tch) < LVL_AMBASSADOR) && (ch != tch)) {
		slog("ImmCast: %s called %s on %s.", GET_NAME(ch),
			spell_to_str(spellnum), tch ? GET_NAME(tch) : tobj ?
			tobj->name : knock_door ?
			(knock_door->keyword ? fname(knock_door->keyword) :
				"door") : "NULL");
	}

	int my_return_flags = 0;

	int retval = call_magic(ch, tch, tobj, tdir, spellnum,
		GET_LEVEL(ch) + (!IS_NPC(ch) ? (GET_REMORT_GEN(ch) << 1) : 0),
		(SPELL_IS_PSIONIC(spellnum) ? CAST_PSIONIC :
		(SPELL_IS_PHYSICS(spellnum) ? CAST_PHYSIC : 
        (SPELL_IS_BARD(spellnum) ? CAST_BARD :
         CAST_SPELL))),
		&my_return_flags);

	if (return_flags) {
		*return_flags = my_return_flags;
	}

	if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
		return 0;
	}

	if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
		return retval;
	}

	if (tch && ch != tch && IS_NPC(tch) &&
		ch->in_room == tch->in_room &&
		SINFO.violent && !tch->numCombatants() && tch->getPosition() > POS_SLEEPING &&
		(!AFF_FLAGGED(tch, AFF_CHARM) || ch != tch->master)) {
		int my_return_flags = hit(tch, ch, TYPE_UNDEFINED);

		my_return_flags = SWAP_DAM_RETVAL(my_return_flags);

		if (return_flags) {
			*return_flags = my_return_flags;
		}

		if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
			return 0;
		}
	}

	return (retval);
}

int perform_taint_burn(Creature *ch, int spellnum)
{
    struct affected_type *af;
    int mana = mag_manacost(ch, spellnum);
    int dam = dice(mana, 14);
    bool weenie = false;

    // Grab the affect for affect level
    af = affected_by_spell(ch, SPELL_TAINT);

    // Max dam based on level and gen of caster. (getLevelBonus(taint))
    if (af != NULL)
        dam = dam * af->level / 100;

    if (damage(ch, ch, dam, TYPE_TAINT_BURN, WEAR_HEAD)) {
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
            act("Blood pours out of $n's forehead as the rune of taint dissolves.", TRUE, ch, 0, 0, TO_ROOM);
        }
    }

    if (weenie == true) {
        act("$n screams and clutches at the rune in $s forehead.", TRUE,
            ch, 0, 0, TO_ROOM);
        send_to_char(ch, "Your concentration fails.\r\n");
        return 0;
    }
    
    return 1;
}

int
find_spell_targets(struct Creature *ch, char *argument,
	struct Creature **tch, struct obj_data **tobj, int *tdir, int *target, int *spellnm,
	int cmd)
{
	char *s, *targets = NULL, *ptr;
	char t3[MAX_INPUT_LENGTH], t2[MAX_INPUT_LENGTH];
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
            return 0;
        } 
        else {
            s = tmp_getquoted(&argument);
            spellnum = find_skill_num(s);
            *spellnm = spellnum;
            targets = tmp_strdup(argument);
        }
    }
    else {
        s = argument;
        spellnum = find_skill_num(s);
        while (spellnum == -1 && *s) {
            ptr = s + strlen(s) - 1;
            while (*ptr != '\0' && *ptr != ' ' && ptr != s)
                ptr--;
            if (ptr == s)
                break;

            targets = tmp_sprintf("%s%s", targets ? targets : "", ptr);
            *ptr = '\0';

            spellnum = find_skill_num(s);
        }       
    }

	*spellnm = spellnum;

	if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
		sprintf(buf, "%s what?!?", cmd_info[cmd].command);
		act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		return 0;
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
		return 0;
	}

	if (CHECK_SKILL(ch, spellnum) == 0) {
		send_to_char(ch, "You are unfamiliar with that %s.\r\n",
			SPELL_IS_PSIONIC(spellnum) ? "trigger" :
			SPELL_IS_PHYSICS(spellnum) ? "alteration" :
			SPELL_IS_MERCENARY(spellnum) ? "device" : 
            SPELL_IS_BARD(spellnum) ? "song" : "spell");
		return 0;
	}
	/* Find the target */

	if (targets != NULL) {
		// DL - moved this here so we can handle multiple locate arguments
		strncpy(locate_buf, targets, 255);
		locate_buf[255] = '\0';
		one_argument(strcpy(arg, targets), targets);
		skip_spaces(&targets);
	}
	if (IS_SET(SINFO.targets, TAR_IGNORE)) {
		*target = TRUE;
	} else if (targets != NULL && *targets) {
        if (!*target && (IS_SET(SINFO.targets, TAR_DIR))) {
            *tdir = search_block(targets, dirs, false); 
            if (*tdir >= 0)
                *target = true;
            
        }
        else if (!*target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
			if ((*tch = get_char_room_vis(ch, targets)) != NULL)
				*target = TRUE;
		}
		if (!*target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
			if ((*tch = get_char_vis(ch, targets)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, targets, ch->carrying)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
			for (i = 0; !*target && i < NUM_WEARS; i++)
				if (GET_EQ(ch, i) && !str_cmp(targets, GET_EQ(ch, i)->aliases)) {
					*tobj = GET_EQ(ch, i);
					*target = TRUE;
				}
		}
		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, targets, ch->in_room->contents)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
			if ((*tobj = get_obj_vis(ch, targets)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_DOOR)) {
			half_chop(arg, t2, t3);
			if ((i = find_door(ch, t2, t3,
						spellnum == SPELL_KNOCK ? "knock" : "cast")) >= 0) {
				knock_door = ch->in_room->dir_option[i];
				*target = TRUE;
			} else {
				return 0;
			}
		}

	} else {					/* if target string is empty */
		if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
			if (ch->numCombatants()) {
				*tch = ch;
				*target = TRUE;
			}
		if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
			if (ch->numCombatants()) {
				*tch = ch->findRandomCombat();
				*target = TRUE;
			}
		/* if no target specified, 
		   and the spell isn't violent, i
		   default to self */
		if (!*target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
			!SINFO.violent && !IS_SET(SINFO.targets, TAR_UNPLEASANT)) {
			*tch = ch;
			*target = TRUE;
		}

		if (!*target) {
			send_to_char(ch, "Upon %s should the spell be cast?\r\n",
				IS_SET(SINFO.targets,
					TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ? "what" :
				"who");
			return 0;
		}
	}
	return 1;
}


ACMD(do_cast)
{
	Creature *tch = NULL;
    int tdir;
	obj_data *tobj = NULL, *holy_symbol = NULL, *metal = NULL;

	int mana, spellnum, i, target = 0, prob = 0, metal_wt = 0, num_eq =
		0, temp = 0;

	if (IS_NPC(ch))
		return;

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

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &tdir, &target, &spellnum,
				cmd)))
		return;

    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) &&
		GET_LEVEL(ch) < LVL_TIMEGOD && (SPELL_IS_MAGIC(spellnum) ||
			SPELL_IS_DIVINE(spellnum))) {
		send_to_char(ch, "Your magic fizzles out and dies.\r\n");
		act("$n's magic fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	// Drunk bastards don't cast very well, do they... -- Nothing 1/22/2001 
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, "Your mind is too clouded to cast any spells!\r\n");
			return;
		} else {
			send_to_char(ch, "You feel your concentration slipping!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
		act("That is not a spell.", FALSE, ch, 0, 0, TO_CHAR);
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

	if (GET_LEVEL(ch) < LVL_IMMORT && (!IS_EVIL(ch) && SPELL_IS_EVIL(spellnum))
		|| (!IS_GOOD(ch) && SPELL_IS_GOOD(spellnum))) {
		send_to_char(ch, "You cannot cast that spell.\r\n");
		return;
	}

	if (GET_LEVEL(ch) > 3 && GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_NPC(ch) &&
		(IS_CLERIC(ch) || IS_KNIGHT(ch)) && SPELL_IS_DIVINE(spellnum)) {
		bool need_symbol = true;
		int gen = GET_REMORT_GEN(ch);
		if (IS_EVIL(ch) && IS_SOULLESS(ch)) {
			if (GET_CLASS(ch) == CLASS_CLERIC && gen > 4)
				need_symbol = false;
			else if (GET_CLASS(ch) == CLASS_KNIGHT && gen > 6)
				need_symbol = false;
		}
		if (need_symbol) {
			for (i = 0; i < NUM_WEARS; i++) {
				if (ch->equipment[i]) {
					if (GET_OBJ_TYPE(ch->equipment[i]) == ITEM_HOLY_SYMB) {
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
				send_to_char(ch, "You are not aligned with your holy symbol!\r\n");
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
					FALSE, ch, holy_symbol, 0, TO_CHAR);
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
		prob += GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob += ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	for (i = 0, num_eq = 0, metal_wt = 0; i < NUM_WEARS; i++) {
		if (ch->equipment[i]) {
			num_eq++;
			if (i != WEAR_WIELD && GET_OBJ_TYPE(ch->equipment[i]) == ITEM_ARMOR
				&& IS_METAL_TYPE(ch->equipment[i])) {
				metal_wt += ch->equipment[i]->getWeight();
				if (!metal || !number(0, 8) ||
					(ch->equipment[i]->getWeight() > metal->getWeight() &&
						!number(0, 1)))
					metal = ch->equipment[i];
			}
			if (ch->implants[i]) {
				if (IS_METAL_TYPE(ch->equipment[i])) {
					metal_wt += ch->equipment[i]->getWeight();
					if (!metal || !number(0, 8) ||
						(ch->implants[i]->getWeight() > metal->getWeight() &&
							!number(0, 1)))
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

	if (tch && tch->getPosition() == POS_FIGHTING)
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
				ok_to_damage(ch, tch) &&
				(prob + number(0, 111)) > CHECK_SKILL(ch, spellnum)) {
				/* misdirect */


				CreatureList::iterator it = ch->in_room->people.begin();
				CreatureList::iterator nit = ch->in_room->people.begin();
				for (; it != ch->in_room->people.end(); ++it) {
					++nit;
					if ((*it) != ch && (*it) != tch &&
						GET_LEVEL((*it)) < LVL_AMBASSADOR &&
						ok_to_damage(ch, (*it)) && (!number(0, 4)
							|| nit != ch->in_room->people.end())) {
						if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
							&& metal && SPELL_IS_MAGIC(spellnum)
							&& metal_wt > number(5, 80))
							act("$p has misdirected your spell toward $N!!",
								FALSE, ch, metal, (*it), TO_CHAR);
						else
							act("Your spell has been misdirected toward $N!!",
								FALSE, ch, 0, (*it), TO_CHAR);
						cast_spell(ch, (*it), tobj, &tdir, spellnum);
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
					act("$p has caused your spell to backfire!!", FALSE, ch,
						metal, 0, TO_CHAR);
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
					FALSE, ch, metal, 0, TO_CHAR);
			else
				send_to_char(ch, "You lost your concentration!\r\n");
			if (!skill_message(0, ch, tch, spellnum)) {
				send_to_char(ch, NOEFFECT);
			}

			if (((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent)) &&
				IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_NOHASSLE) &&
				can_see_creature(tch, ch))
				hit(tch, ch, TYPE_UNDEFINED);

		} else if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) &&
			SPELL_IS_MAGIC(spellnum) && metal && metal_wt > number(5, 90))
			act("$p has interfered with your spell!",
				FALSE, ch, metal, 0, TO_CHAR);
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
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
    int tdir;
	int mana, spellnum, target = 0, prob = 0, temp = 0;

	if (IS_NPC(ch))
		return;

	if (!IS_PSYCHIC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able to trigger the mind.\r\n");
		return;
	}
	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky for you to concentrate!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &tdir, &target, &spellnum,
				cmd)))
		return;

	if ((ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS)) &&
		GET_LEVEL(ch) < LVL_TIMEGOD && SPELL_IS_PSIONIC(spellnum)) {
		send_to_char(ch, "You cannot establish a mental link.\r\n");
		act("$n appears to be psionically challenged.",
			FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	// Drunk bastards don't trigger very well, do they... -- Nothing 1/22/2001
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, "Your mind is too clouded to make any triggers!\r\n");
			return;
		} else {
			send_to_char(ch, "You feel your concentration slipping!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_PSIONIC(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_PSIONIC(spellnum)) {
		act("That is not a psionic trigger.", FALSE, ch, 0, 0, TO_CHAR);
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
		act("You cannot make a mindlink with $N!", FALSE, ch, 0, tch, TO_CHAR);
		return;
	}

	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN)
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
		prob -= GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	if (tch && tch->getPosition() == POS_FIGHTING)
		prob -= (GET_LEVEL(tch) >> 3);

	/**** casting probability ends here *****/

	/* You throws the dice and you takes your chances.. 101% is total failure */
	if (number(0, 111) > prob) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || spellnum == SPELL_ELECTROSTATIC_FIELD
			|| !skill_message(0, ch, tch, spellnum))
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
/*	struct Creature *tch = NULL;
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

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &target, &spellnum,
				cmd)))
		return;

	if (!SPELL_IS_MERCENARY(spellnum)) {
		act("You don't seem to have that.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target) {
		send_to_char(ch, "I cannot find your target!\r\n");
		return;
	}

	rpoints = mag_manacost(ch, spellnum);
    resource = find_item_kit(ch);
    
    if (!resource && ch->getLevel() < LVL_AMBASSADOR) {
        send_to_char(ch, "But you don't have a resource kit!\r\n");
        return;
    }

	if ((rpoints > 0) && (GET_RPOINTS(resource) < rpoints)
		&& (ch->getLevel() < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the resources!\r\n");
		return;
	}

	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN)
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
		prob -= GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	if (FIGHTING(ch) && (FIGHTING(ch))->getPosition() == POS_FIGHTING)
		prob -= (GET_LEVEL(FIGHTING(ch)) >> 3);


	if (number(0, 111) > prob) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, spellnum)) {
            act("You fumble around in $p and come up with nothing!", FALSE,
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


struct obj_data *find_item_kit(Creature *ch)
{
/*    struct obj_data *cur_obj;

    for (int i = 0; i < NUM_WEARS; i++) {
        cur_obj = ch->equipment[i];
        if (cur_obj) {
            if ((GET_OBJ_TYPE(cur_obj == ITEM_KIT)) &&
                (ch->getLevel() > GET_OBJ_VAL(cur_obj, 0)) &&
                (ch->getLevel() < GET_OBJ_VAL(cur_obj, 1))) {
                return cur_obj;
            }
        }
    }

    for (obj_data = ch->carrying; obj_data; obj_data = obj_data->next) {
        if ((GET_OBJ_TYPE(cur_obj == ITEM_KIT)) &&
            (ch->getLevel() > GET_OBJ_VAL(cur_obj, 0)) &&
            (ch->getLevel() < GET_OBJ_VAL(cur_obj, 1))) {
            return cur_obj;
        }
    } */

    return NULL;
}

ACMD(do_alter)
{
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
    int tdir;
	int mana, spellnum, target = 0, temp = 0;

	if (IS_NPC(ch))
		return;

	if (GET_CLASS(ch) != CLASS_PHYSIC &&
		GET_REMORT_CLASS(ch) != CLASS_PHYSIC
		&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able to alter the fabric of reality.\r\n");
		return;
	}
	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky to alter reality!\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) &&
		IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
		send_to_char(ch, "You can't alter while wielding a two handed weapon!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &tdir, &target, &spellnum,
				cmd)))
		return;

	if ((ROOM_FLAGGED(ch->in_room, ROOM_NOSCIENCE)) &&
		SPELL_IS_PHYSICS(spellnum)) {
		send_to_char(ch, 
			"You are unable to alter physical reality in this space.\r\n");
		act("$n tries to solve an elaborate equation, but fails.", FALSE,
			ch, 0, 0, TO_ROOM);
		return;
	}

	if (SPELL_USES_GRAVITY(spellnum) && NOGRAV_ZONE(ch->in_room->zone) &&
		SPELL_IS_PHYSICS(spellnum)) {
		send_to_char(ch, "There is no gravity here to alter.\r\n");
		act("$n tries to solve an elaborate equation, but fails.",
			FALSE, ch, 0, 0, TO_ROOM);
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
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_PHYSICS(spellnum)) {
		act("That is not a physical alteration.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target) {
		send_to_char(ch, "Cannot find the target of your alteration!\r\n");
		return;
	}
	mana = mag_manacost(ch, spellnum);
	if ((mana > 0) && (GET_MANA(ch) < mana)
		&& (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the energy to make that alteration!\r\n");
		return;
	}

	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN)
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
		if (!tch || !skill_message(0, ch, tch, spellnum))
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
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
    int tdir;
	int mana, spellnum, target = 0, temp = 0;

    extern bool check_instrument(Creature *ch, int songnum);
    extern char *get_instrument_type(int songnum);

	if (IS_NPC(ch))
		return;

	if (GET_CLASS(ch) != CLASS_BARD &&
		GET_REMORT_CLASS(ch) != CLASS_BARD
		&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You raise your pitiful voice to the heavens.\r\n");
		return;
	}
	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your heavy equipment prevents you from breathing properly!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &tdir, &target, &spellnum,
				cmd)))
		return;

	// Drunk bastards don't sing very well, do they... -- Nothing 8/28/2004
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, 
				"You try but you can't remember the tune!\r\n");
			return;
		} else {
			send_to_char(ch, "You begin to play but you can't hold the tune!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_BARD(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

    if (tch && IS_UNDEAD(tch)) {
        send_to_char(ch, "The undead have no appreciation for music!\r\n");
        return;
    }

	if (!SPELL_IS_BARD(spellnum)) {
		act("You do not know that song!", FALSE, ch, 0, 0, TO_CHAR);
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

	if ((SECT_TYPE(ch->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN)) {
		send_to_char(ch, "You can't sing or play underwater!.\r\n");
		return;
	}

	/* You throws the dice and you takes your chances.. 101% is total failure */
	if (number(0, 101) > GET_SKILL(ch, spellnum)) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, spellnum))
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

    for (int i = 0;i < NUM_CLASSES;i++)
        spell_info[idnum].min_level[i] = 0;

    spells[idnum] = xmlGetProp(node, "name");
    for (child = node->children;child;child = child->next) {
        if (xmlMatches(child->name, "granted")) {
            char *class_str = xmlGetProp(child, "class");
            int level = xmlGetIntProp(child, "level");
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
            spell_info[idnum].mana_max = xmlGetIntProp(child, "initial");
            spell_info[idnum].mana_change = xmlGetIntProp(child, "level_dec");
            spell_info[idnum].mana_min = xmlGetIntProp(child, "minimum");
        } else if (xmlMatches(child->name, "position")) {
            char *pos_str = xmlGetProp(child, "minimum");
            
            if (!pos_str) {
                errlog("Required property minimum missing from position element");
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
            char *type_str = xmlGetProp(child, "type");
            char *scope_str = xmlGetProp(child, "scope");

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
            char *value_str = xmlGetProp(child, "value");

            if (!strcmp(value_str, "violent")) {
                spell_info[idnum].violent = true;
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
        }
    }

    if (!spell_info[idnum].targets)
        spell_info[idnum].targets = TAR_IGNORE;

    return true;
}

void
clear_spells(void)
{
	for (int spl = 1; spl <= TOP_SPELL_DEFINE; spl++) {
        for (int i = 0; i < NUM_CLASSES; i++)
            spell_info[spl].gen[i] = 0;
        spells[spl] = "!UNUSED!";
        spell_info[spl].min_level[CLASS_MAGIC_USER] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_CLERIC] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_THIEF] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_WARRIOR] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_BARB] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_PSIONIC] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_PHYSIC] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_CYBORG] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_KNIGHT] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_RANGER] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_BARD] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_MONK] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_VAMPIRE] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_MERCENARY] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_SPARE1] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_SPARE2] = LVL_GRIMP + 1;
        spell_info[spl].min_level[CLASS_SPARE3] = LVL_GRIMP + 1;
        spell_info[spl].mana_max = 0;
        spell_info[spl].mana_min = 0;
        spell_info[spl].mana_change = 0;
        spell_info[spl].min_position = 0;
        spell_info[spl].targets = 0;
        spell_info[spl].violent = 0;
        spell_info[spl].routines = 0;
    }

    // Initialize string list terminator
    spells[0] = "!RESERVED!";
    spells[TOP_SPELL_DEFINE] = "\n";
}

void
boot_spells(void)
{
    const char *path = "etc/spells.xml";
	xmlDocPtr doc;
	xmlNodePtr node;
    int num_spells = 0;

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

    for (node = node->children;node;node = node->next) {
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
