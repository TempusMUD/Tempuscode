/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: fight.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __fight_c__
#define __combat_code__

#include <signal.h>

#include "structs.h"
#include "creature_list.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "fight.h"
#include "bomb.h"
#include "guns.h"
#include "specs.h"
#include "mobact.h"
#include "combat.h"
#include "events.h"
#include "security.h"
#include "quest.h"
#include "vendor.h"

#include <iostream>
#include <algorithm>

int corpse_state = 0;

/* The Fight related routines */
obj_data *get_random_uncovered_implant(Creature * ch, int type = -1);
int calculate_weapon_probability(struct Creature *ch, int prob,
	struct obj_data *weap);
int do_combat_fire(struct Creature *ch, struct Creature *vict);
int do_casting_weapon(Creature *ch, obj_data *weap);
int calculate_attack_probability(struct Creature *ch);

/* start one char fighting another ( yes, it is horrible, I know... )  */
void
set_fighting(struct Creature *ch, struct Creature *vict, int aggr)
{
	if (ch == vict)
		return;

	if (FIGHTING(ch)) {
		slog("SYSERR: FIGHTING(ch) != NULL in set_fighting().");
		return;
	}

	if (aggr && !IS_NPC(vict)) {
		if (IS_NPC(ch)) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master)
				&& (!MOB_FLAGGED(ch, MOB_MEMORY)
					|| !char_in_memory(vict, ch))) {
				check_killer(ch->master, vict, "charmed");
			}
		} else {
			if(!is_arena_combat(ch, vict)) {
				if (!PRF2_FLAGGED(ch, PRF2_PKILLER)) {
					send_to_char(ch, "A small dark shape flies in from the future and sticks to your tongue.\r\n");
					return;
				}
				if (!PRF2_FLAGGED(vict, PRF2_PKILLER)) {
					send_to_char(ch, 
						"A small dark shape flies in from the future and sticks to your eye.\r\n");
					return;
				}
				if (ch->isNewbie() && !is_arena_combat(ch, vict)) {
					send_to_char(ch, "You are currently under new player protection, which expires at level 41.\r\n");
					send_to_char(ch, "You cannot attack other players while under this protection.\r\n");
					return;
				}
			}
			

			check_killer(ch, vict, "normal");

			if (vict->isNewbie() &&
					GET_LEVEL(ch) < LVL_IMMORT &&
					!is_arena_combat(ch, vict)) {
				act("$N is currently under new character protection.",
					FALSE, ch, 0, vict, TO_CHAR);
				act("You are protected by the gods against $n's attack!",
					FALSE, ch, 0, vict, TO_VICT);
				slog("%s protected against %s (set_fighting) at %d",
					GET_NAME(vict), GET_NAME(ch), vict->in_room->number);
				ch->setFighting(NULL);
				ch->setPosition(POS_STANDING);
				return;
			}
		}
	}

	combatList.add(ch);

	ch->setFighting(vict);
	update_pos(ch);
}


/* 
   corrects position and removes combat related bits.
   Call ONLY from stop_fighting
*/
void
remove_fighting_affects(struct Creature *ch)
{
	ch->setFighting(NULL);

	if (ch->in_room && ch->in_room->isOpenAir()) {
		ch->setPosition(POS_FLYING);
	} else if (!IS_NPC(ch)) {
		if (ch->getPosition() >= POS_FIGHTING)
			ch->setPosition(POS_STANDING);
		else if (ch->getPosition() >= POS_RESTING)
			ch->setPosition(POS_SITTING);
	} else {
		if (IS_AFFECTED(ch, AFF_CHARM) && IS_UNDEAD(ch))
			ch->setPosition(POS_STANDING);
		else if (!HUNTING(ch))
			ch->setPosition(GET_DEFAULT_POS(ch));
		else
			ch->setPosition(POS_STANDING);
	}

	update_pos(ch);

}

/* remove a char from the list of fighting chars */
void
stop_fighting(CreatureList::iterator & cit)
{
	struct Creature *ch = *cit;
	combatList.remove(cit);
	remove_fighting_affects(ch);
}

/* remove a char from the list of fighting chars */
void
stop_fighting(struct Creature *ch)
{
	combatList.remove(ch);
	remove_fighting_affects(ch);
}


/* When ch kills victim */
void
change_alignment(struct Creature *ch, struct Creature *victim)
{
	GET_ALIGNMENT(ch) += -(GET_ALIGNMENT(victim) / 100);
	GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch));
	GET_ALIGNMENT(ch) = MIN(1000, GET_ALIGNMENT(ch));
	check_eq_align(ch);
}


void
raw_kill(struct Creature *ch, struct Creature *killer, int attacktype)
{

	if (FIGHTING(ch))
		stop_fighting(ch);

	if (attacktype != SKILL_GAROTTE)
		death_cry(ch);

	make_corpse(ch, killer, attacktype);

	while (ch->affected)
		affect_remove(ch, ch->affected);

	REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PETRIFIED);

	if (IS_NPC(ch))
		ch->mob_specials.shared->kills++;

	// Equipment dealt with in make_corpse. 
	// Do not save it here.
	if (is_arena_combat(killer, ch))
		ch->arena_die();
	else
		ch->die();
//	Event::Queue(new DeathEvent(0, ch, is_arena_combat(killer, ch)));
}


extern bool LOG_DEATHS;

void
die(struct Creature *ch, struct Creature *killer, int attacktype,
	int is_humil)
{
	if (IS_NPC(ch) && GET_MOB_SPEC(ch)) {
		if (GET_MOB_SPEC(ch) (killer, ch, 0, NULL, SPECIAL_DEATH)) {
			mudlog(LVL_CREATOR, NRM, true,
				"ERROR: Mobile special for %s run in place of standard extraction.\n",
				GET_NAME(ch));
			return;
		}
	} 

	if( LOG_DEATHS ) {
		if( IS_NPC(ch) ) {
			slog("DEATH: %s killed by %s. attacktype: %d SPEC[%p]",
				GET_NAME(ch),
				killer ? GET_NAME(killer) : "(NULL)",
				attacktype, 
				GET_MOB_SPEC(ch));
		} else {
			slog("DEATH: %s killed by %s. attacktype: %d PC",
				GET_NAME(ch),
				killer ? GET_NAME(killer) : "(NULL)",
				attacktype);
		}
	}

	if( IS_PC(ch) && !is_arena_combat(killer, ch) && killer != NULL &&
			!PLR_FLAGGED(killer, PLR_KILLER) && !ch->isNewbie() ) {
		// exp loss capped at the beginning of the level.
		int loss = GET_EXP(ch) >> 3;
		loss = MIN( loss, GET_EXP(ch) - exp_scale[GET_LEVEL(ch)] );
		gain_exp(ch, -loss);
	}

	if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF) &&
			GET_LEVEL(ch) < LVL_AMBASSADOR) {

		int loss = MAX(1, GET_SEVERITY(ch) / 2); // lose 1 level per 2, min 1
		int old_hit, old_mana, old_move;
		int lvl;

		//
		// Unaffect the character before all the stuff is subtracted. Bug was being abused
		//

		while (ch->affected)
			affect_remove(ch, ch->affected);

		GET_REMORT_GEN(ch) -= MIN(GET_REMORT_GEN(ch), loss / 50);
		if (GET_LEVEL(ch) <= (loss % 50)) {
			GET_REMORT_GEN(ch) -= 1;
			lvl = 49 - (loss % 50) + GET_LEVEL(ch);
		} else
			lvl = GET_LEVEL(ch) - (loss % 50);
		
		lvl = MIN(49, MAX(1, lvl));
		// Initialize their character to a level 1 of that gen, without
		// messing with skills/spells and such
		GET_LEVEL(ch) = 1;
		GET_EXP(ch) = 1;
		GET_MAX_HIT(ch) = 20;
		GET_MAX_MANA(ch) = 100;
		GET_MAX_MOVE(ch) = 82;
		old_hit = GET_HIT(ch);
		old_mana = GET_MANA(ch);
		old_move = GET_MOVE(ch);

		advance_level(ch, true);
		GET_MAX_MOVE(ch) += GET_CON(ch);

		// 
		while (--lvl) {
			GET_LEVEL(ch)++;
			advance_level(ch, true);
		}

		GET_HIT(ch) = MIN(old_hit, GET_MAX_HIT(ch));
		GET_MANA(ch) = MIN(old_mana, GET_MAX_MANA(ch));
		GET_MOVE(ch) = MIN(old_move, GET_MAX_MOVE(ch));

		// Remove all the skills that they shouldn't have
		for (int i = 1;i < MAX_SPELLS;i++)
			if (!ABLE_TO_LEARN(ch, i))
				SET_SKILL(ch, i, 0);

		// They're now that level, but without experience, and with extra
		// life points, pracs, and such.  Make it all sane.
		GET_EXP(ch) = exp_scale[GET_LEVEL(ch)];
		GET_LIFE_POINTS(ch) = 0;
		GET_SEVERITY(ch) = 0;
		GET_INVIS_LVL(ch) = MIN(GET_LEVEL(ch), GET_INVIS_LVL(ch));

		// And they get uglier, too!
		GET_CHA(ch) = MAX(3, GET_CHA(ch) - 2);

		mudlog(LVL_AMBASSADOR, NRM, true,
			"%s losing %d levels for outlaw flag: now gen %d, lvl %d",
				GET_NAME(ch), loss, GET_REMORT_GEN(ch), GET_LEVEL(ch));

	}

	if (!IS_NPC(ch) && (!ch->in_room)
		|| !is_arena_combat(killer, ch)) {
		if (ch != killer)
			REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);

		if (GET_LEVEL(ch) > 10 && !IS_NPC(ch)) {
			if (GET_LIFE_POINTS(ch) <= 0 && GET_MAX_HIT(ch) <= 1) {

				if (IS_EVIL(ch) || IS_NEUTRAL(ch))
					send_to_char(ch, 
						"Your soul screeches in agony as it's torn from the mortal realms... forever.\r\n");

				else if (IS_GOOD(ch))
					send_to_char(ch, 
						"The righteous rejoice as your soul departs the mortal realms... forever.\r\n");

				SET_BIT(PLR2_FLAGS(ch), PLR2_BURIED);
				mudlog(LVL_GOD, NRM, true,
					"%s died with no maxhit and no life points. Burying.",
					GET_NAME(ch));

			} else if (GET_LIFE_POINTS(ch) > 0) {
				GET_LIFE_POINTS(ch) =
					MAX(0, GET_LIFE_POINTS(ch) - number(1, (GET_LEVEL(ch) >> 3)));
			} else if (!number(0, 3)) {
				GET_CON(ch) = MAX(3, GET_CON(ch) - 1);
			} else if (GET_LEVEL(ch) > number(20, 50)) {
				GET_MAX_HIT(ch) = MAX(1, GET_MAX_HIT(ch) - dice(3, 5));
			}
		}
		if (IS_CYBORG(ch)) {
			GET_TOT_DAM(ch) = 0;
			GET_BROKE(ch) = 0;
		}
		GET_PC_DEATHS(ch)++;
	}

	REMOVE_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
	REMOVE_BIT(AFF3_FLAGS(ch), AFF3_SELF_DESTRUCT);
	raw_kill(ch, killer, attacktype);	// die
}

void perform_gain_kill_exp(struct Creature *ch, struct Creature *victim,
	float multiplier);

void
group_gain(struct Creature *ch, struct Creature *victim)
{
	struct Creature *leader;
	int total_levs = 0;
	int total_pc_mems = 0;
	float mult = 0;
	float mult_mod = 0;

	if (!(leader = ch->master))
		leader = ch;
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (AFF_FLAGGED((*it), AFF_GROUP) && ((*it) == leader
		|| leader == (*it)->master)) 
		{
			total_levs += GET_LEVEL((*it));
			if( IS_PC(*it) ) {
				total_levs += GET_REMORT_GEN((*it)) << 3;
				total_pc_mems++;
			}
		}
	}
	it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (AFF_FLAGGED((*it), AFF_GROUP) && ((*it) == leader
			|| leader == (*it)->master)) 
		{
			mult = (float)GET_LEVEL((*it));
			if( IS_PC( (*it) ) )
				mult += GET_REMORT_GEN((*it)) << 3;
			mult /= (float)total_levs;

			if (total_pc_mems) {
				mult_mod = 1 - mult;
				mult_mod *= mult;
				send_to_char(*it, "Your group gain is %d%% + bonus %d%%.\n",
					(int)((float)mult * 100), (int)((float)mult_mod * 100));
			}

			perform_gain_kill_exp((*it), victim, mult);
		}
	}
}


void
perform_gain_kill_exp(struct Creature *ch, struct Creature *victim,
	float multiplier)
{

	int exp = 0;


	if (!IS_NPC(victim))
		exp = (int)MIN(max_exp_gain, (GET_EXP(victim) * multiplier) / 8);
	else
		exp = (int)MIN(max_exp_gain, (GET_EXP(victim) * multiplier) / 3);

	/* Calculate level-difference bonus */
	if (IS_NPC(ch))
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	else
		exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);

	exp = MAX(exp, 1);
	exp = MIN(max_exp_gain, exp);

	if (!IS_NPC(ch)) {
		exp = MIN(((exp_scale[(int)(GET_LEVEL(ch) + 1)] -
					exp_scale[(int)GET_LEVEL(ch)]) >> 3), exp);
		/* exp is limited to 12.5% of needed from level to ( level + 1 ) */

		if ((exp + GET_EXP(ch)) > exp_scale[GET_LEVEL(ch) + 2])
			exp =
				(((exp_scale[GET_LEVEL(ch) + 2] - exp_scale[GET_LEVEL(ch) + 1])
					>> 1) + exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch);

		if (!IS_NPC(victim)) {
			if (is_arena_combat(ch, victim))
				exp = 1;
			else
				exp >>= 8;
		}
	}

	exp = ch->getPenalizedExperience( exp, victim );

	if (IS_NPC(victim) && !IS_NPC(ch) 
	&& (GET_EXP(victim) < 0 || exp > 5000000)) 
	{
		slog("%s Killed %s(%d) for exp: %d.", GET_NAME(ch),
			GET_NAME(victim), GET_EXP(victim), exp);
	}
	if (exp > ((exp_scale[GET_LEVEL(ch) + 1] - GET_EXP(ch)) / 10)) {
		send_to_char(ch, "%s%sYou have gained much experience.%s\r\n",
			CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_SPR));
	} else if (exp > 1) {
		send_to_char(ch, "%s%sYou have gained experience.%s\r\n",
			CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_SPR));
	} else if (exp < 0) {
		send_to_char(ch, "%s%sYou have lost experience.%s\r\n",
			CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_SPR));
	} else
		send_to_char(ch, "You have gained trivial experience.\r\n");

	gain_exp(ch, exp);
	change_alignment(ch, victim);

}

void
gain_kill_exp(struct Creature *ch, struct Creature *victim)
{

	if (ch == victim)
		return;

	if (IS_NPC(victim) && MOB2_FLAGGED(victim, MOB2_UNAPPROVED)
		&& !ch->isTester())
		return;

	if ((IS_NPC(ch) && IS_PET(ch)) || IS_NPC(victim) && IS_PET(victim))
		return;

	if (IS_AFFECTED(ch, AFF_GROUP)) {
		group_gain(ch, victim);
		return;
	}

	perform_gain_kill_exp(ch, victim, 1);
}


void
eqdam_extract_obj(struct obj_data *obj)
{

	struct obj_data *inobj = NULL, *next_obj = NULL;

	for (inobj = obj->contains; inobj; inobj = next_obj) {
		next_obj = inobj->next_content;

		obj_from_obj(inobj);

		if (IS_IMPLANT(inobj))
			SET_BIT(GET_OBJ_WEAR(inobj), ITEM_WEAR_TAKE);

		if (obj->in_room)
			obj_to_room(inobj, obj->in_room);
		else if (obj->worn_by)
			obj_to_char(inobj, obj->worn_by);
		else if (obj->carried_by)
			obj_to_char(inobj, obj->carried_by);
		else if (obj->in_obj)
			obj_to_obj(inobj, obj->in_obj);
		else {
			slog("SYSERR: wierd bogus shit.");
			extract_obj(inobj);
		}
	}
	extract_obj(obj);
}

struct obj_data *
destroy_object(Creature *ch, struct obj_data *obj, int type)
{
	struct obj_data *new_obj = NULL, *inobj = NULL;
	struct room_data *room = NULL;
	struct Creature *vict = NULL;
	const char *mat_name;
	char *msg = NULL;
	int tmp;

	if (GET_OBJ_MATERIAL(obj))
		mat_name = material_names[GET_OBJ_MATERIAL(obj)];
	else
		mat_name = "material";

	new_obj = create_obj();
	new_obj->shared = null_obj_shared;
	GET_OBJ_MATERIAL(new_obj) = GET_OBJ_MATERIAL(obj);
	new_obj->setWeight(obj->getWeight());
	GET_OBJ_TYPE(new_obj) = ITEM_TRASH;
	GET_OBJ_WEAR(new_obj) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(new_obj) = ITEM_NODONATE + ITEM_NOSELL;
	GET_OBJ_VAL(new_obj, 0) = obj->shared->vnum;
	GET_OBJ_MAX_DAM(new_obj) = 100;
	GET_OBJ_DAM(new_obj) = 100;
	if (type == SPELL_OXIDIZE && IS_FERROUS(obj)) {
		msg = "$p dissolves into a pile of rust!!";
		new_obj->aliases = str_dup("pile rust");
		new_obj->name = str_dup("a pile of rust");
		strcat(CAP(buf), " is lying here.");
		new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));
		GET_OBJ_MATERIAL(new_obj) = MAT_RUST;
	} else if (type == SPELL_OXIDIZE && IS_BURNABLE_TYPE(obj)) {
		msg = "$p is incinerated!!";
		new_obj->aliases = str_dup("pile ash");
		new_obj->name = str_dup("a pile of ash");
		new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));
		GET_OBJ_MATERIAL(new_obj) = MAT_ASH;

	} else if (type == SPELL_BLESS) {
		msg = "$p glows bright blue and shatters to pieces!!";
		new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
			material_names[GET_OBJ_MATERIAL(obj)],
			obj->aliases));
		new_obj->name = str_dup(tmp_strcat(
			"shattered fragments of ", mat_name));
		new_obj->line_desc = str_dup(tmp_capitalize(
			tmp_strcat(new_obj->name, " are lying here.")));
	} else if (type == SPELL_DAMN) {
		msg = "$p glows bright red and shatters to pieces!!";
		new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
			material_names[GET_OBJ_MATERIAL(obj)],
			obj->aliases));
		new_obj->name = str_dup(tmp_strcat(
			"shattered pieces of ", mat_name));
		new_obj->line_desc = str_dup(tmp_capitalize(
			tmp_strcat(new_obj->name, " are lying here.")));
	} else if (IS_METAL_TYPE(obj)) {
		msg = "$p is reduced to a mangled pile of scrap!!";
		new_obj->aliases = str_dup(tmp_sprintf("%s %s mangled heap",
			material_names[GET_OBJ_MATERIAL(obj)],
			obj->aliases));
		new_obj->name = str_dup(tmp_strcat("a mangled heap of ", mat_name));
		new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));

	} else if (IS_STONE_TYPE(obj) || IS_GLASS_TYPE(obj)) {
		msg = "$p shatters into a thousand fragments!!";
		new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
			material_names[GET_OBJ_MATERIAL(obj)],
			obj->aliases));
		new_obj->name = str_dup(tmp_strcat(
			"shattered fragments of ", mat_name));
		new_obj->line_desc = str_dup(tmp_capitalize(
			tmp_strcat(new_obj->name, " are lying here.")));

	} else {
		msg =  "$p has been destroyed!";
		new_obj->aliases = str_dup(tmp_sprintf("%s %s mutilated heap",
			material_names[GET_OBJ_MATERIAL(obj)],
			obj->aliases));
		new_obj->name = str_dup(tmp_strcat(
			"a mutilated heap of ", mat_name));
		new_obj->line_desc = str_dup(tmp_capitalize(
			tmp_strcat(new_obj->name, " is lying here.")));

		if (IS_CORPSE(obj)) {
			GET_OBJ_TYPE(new_obj) = ITEM_CONTAINER;
			GET_OBJ_VAL(new_obj, 0) = GET_OBJ_VAL(obj, 0);
			GET_OBJ_VAL(new_obj, 1) = GET_OBJ_VAL(obj, 1);
			GET_OBJ_VAL(new_obj, 2) = GET_OBJ_VAL(obj, 2);
			GET_OBJ_VAL(new_obj, 3) = GET_OBJ_VAL(obj, 3);
			GET_OBJ_TIMER(new_obj) = GET_OBJ_TIMER(obj);
			/* TODO: Make this work
			   if ( GET_OBJ_VAL( obj, 3 ) && CORPSE_IDNUM( obj ) > 0 &&
			   CORPSE_IDNUM( obj ) <= top_of_p_table ){
			   sprintf(buf, "%s destroyed by %s.", 
			   obj->name, GET_NAME( ch ));
			   mudlog(buf, CMP, LVL_DEMI, TRUE);
			   }
			 */
		}
	}

	if (IS_OBJ_STAT2(obj, ITEM2_IMPLANT))
		SET_BIT(GET_OBJ_EXTRA2(new_obj), ITEM2_IMPLANT);

	if ((room = obj->in_room) && (vict = obj->in_room->people)) {
		act(msg, FALSE, vict, obj, 0, TO_CHAR);
		act(msg, FALSE, vict, obj, 0, TO_ROOM);
	} else if ((vict = obj->worn_by))
		act(msg, FALSE, vict, obj, 0, TO_CHAR);
	else
		act(msg, FALSE, ch, obj, 0, TO_CHAR);

	if (!obj->shared->proto) {
		eqdam_extract_obj(obj);
		extract_obj(new_obj);
		return NULL;
	}

	if ((vict = obj->worn_by) || (vict = obj->carried_by)) {
		if (obj->worn_by && (obj != GET_EQ(obj->worn_by, obj->worn_on))) {
			tmp = obj->worn_on;
			eqdam_extract_obj(obj);
			if (equip_char(vict, new_obj, tmp, MODE_IMPLANT))
				return (new_obj);
		} else {
			eqdam_extract_obj(obj);
			obj_to_char(new_obj, vict);
		}
	} else if (room) {
		eqdam_extract_obj(obj);
		obj_to_room(new_obj, room);
	} else if ((inobj = obj->in_obj)) {
		eqdam_extract_obj(obj);
		obj_to_obj(new_obj, inobj);
	} else {
		slog("SYSERR: Improper location of obj and new_obj in damage_eq.");
		eqdam_extract_obj(obj);
	}
	return (new_obj);
}


struct obj_data *
damage_eq(struct Creature *ch, struct obj_data *obj, int eq_dam, int type)
{
	struct Creature *vict = NULL;
	struct obj_data *inobj = NULL, *next_obj = NULL;

    /* test to see if item should take damage */
	if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) || GET_OBJ_DAM(obj) < 0 || 
        GET_OBJ_MAX_DAM(obj) < 0 ||
		(ch && GET_LEVEL(ch) < LVL_IMMORT && !CAN_WEAR(obj, ITEM_WEAR_TAKE)) ||
		(ch && ch->in_room && ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) ||
		(obj->in_room && ROOM_FLAGGED(obj->in_room, ROOM_ARENA)) ||
		(GET_OBJ_TYPE(obj) == ITEM_KEY) || (GET_OBJ_TYPE(obj) == ITEM_SCRIPT)
		|| obj->in_room == zone_table->world)
		return NULL;

	/** damage has destroyed object */
	if ((GET_OBJ_DAM(obj) - eq_dam) < (GET_OBJ_MAX_DAM(obj) >> 5)) {
		/* damage interior items */
		for (inobj = obj->contains; inobj; inobj = next_obj) {
			next_obj = inobj->next_content;

			damage_eq(NULL, inobj, (eq_dam >> 1), type);
		}

		return destroy_object(ch, obj, type);
	}


	/* object has reached broken state */
	if ((GET_OBJ_DAM(obj) - eq_dam) < (GET_OBJ_MAX_DAM(obj) >> 3)) {

		SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_BROKEN);
		strcpy(buf2, "$p has been severely damaged!!");

		/* object looking rough ( 25% ) */
	} else if ((GET_OBJ_DAM(obj) > (GET_OBJ_MAX_DAM(obj) >> 2)) &&
		((GET_OBJ_DAM(obj) - eq_dam) < (GET_OBJ_MAX_DAM(obj) >> 2))) {

		sprintf(buf2, "$p is starting to look pretty %s.",
			IS_METAL_TYPE(obj) ? "mangled" :
			(IS_LEATHER_TYPE(obj) || IS_CLOTH_TYPE(obj)) ? "ripped up" :
			"bad");

		/* object starting to wear ( 50% ) */
	} else if ((GET_OBJ_DAM(obj) > (GET_OBJ_MAX_DAM(obj) >> 1)) &&
		((GET_OBJ_DAM(obj) - eq_dam) < (GET_OBJ_MAX_DAM(obj) >> 1))) {

		strcpy(buf2, "$p is starting to show signs of wear.");

		/* just tally the damage and end */
	} else {
		GET_OBJ_DAM(obj) -= eq_dam;
		return NULL;
	}

	/* send out messages and unequip if needed */
	if (obj->in_room && (vict = obj->in_room->people)) {
		act(buf2, FALSE, vict, obj, 0, TO_CHAR);
		act(buf2, FALSE, vict, obj, 0, TO_ROOM);
	} else if ((vict = obj->worn_by)) {
		act(buf2, FALSE, vict, obj, 0, TO_CHAR);
		if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
			if (obj == GET_EQ(vict, obj->worn_on))
				obj_to_char(unequip_char(vict, obj->worn_on, MODE_EQ), vict);
			else {
				if (IS_DEVICE(obj))
					ENGINE_STATE(obj) = 0;
			}
		}
	} else
		act(buf2, FALSE, ch, obj, 0, TO_CHAR);

	GET_OBJ_DAM(obj) -= eq_dam;
	return NULL;
}


int
SWAP_DAM_RETVAL(int val)
{
	return ((val & DAM_VICT_KILLED) ? DAM_ATTACKER_KILLED : 0) |
		((val & DAM_ATTACKER_KILLED) ? DAM_VICT_KILLED : 0);
}

//
// wrapper for damage() for damaging the attacker (swaps return values automagically)
//

inline int
damage_attacker(struct Creature *ch, struct Creature *victim, int dam,
	int attacktype, int location)
{
	int retval = damage(ch, victim, dam, attacktype, location);
	retval = SWAP_DAM_RETVAL(retval);
	return retval;
}

//#define DAM_RETURN(i,flags) { if ( return_flags ) { *return_flags = flags }; cur_weap = 0; return i; }
//#define DAM_RETURN(i) { cur_weap = 0; return i; }
#define DAM_RETURN( flags ) { cur_weap = 0; return flags; }


//
// damage( ) returns TRUE on a kill, FALSE otherwise
// damage(  ) MUST return with DAM_RETURN(  ) macro !!!
// the return value bits can be a combination of:
// DAM_VICT_KILLED
// DAM_ATTACKER_KILLED
//

int
damage(struct Creature *ch, struct Creature *victim, int dam,
	int attacktype, int location)
{
	int hard_damcap, is_humil = 0, eq_dam = 0, weap_dam = 0, i, impl_dam =
		0, mana_loss;
	struct obj_data *obj = NULL, *weap = cur_weap, *impl = NULL;
	struct room_affect_data rm_aff;
	struct affected_type *af = NULL;
	bool deflected = false;


	if (victim->getPosition() <= POS_DEAD) {
		slog("SYSERR: Attempt to damage a corpse--ch=%s,vict=%s,type=%d.",
			ch ? GET_NAME(ch) : "NULL", GET_NAME(victim), attacktype);
		DAM_RETURN(DAM_VICT_KILLED);
	}

	if (victim->in_room == NULL) {
		slog("SYSERR: Attempt to damage a char with null in_room ch=%s,vict=%s,type=%d.",
			ch ? GET_NAME(ch) : "NULL", GET_NAME(victim), attacktype);
		raise(SIGSEGV);
	}
	// No more shall anyone be damaged in the void.
	if (victim->in_room == zone_table->world) {
		return false;
	}

	if (GET_HIT(victim) < -10) {
		slog("SYSERR: Attempt to damage a char with hps %d ch=%s,vict=%s,type=%d.",
			GET_HIT(victim), ch ? GET_NAME(ch) : "NULL", GET_NAME(victim),
			attacktype);
		DAM_RETURN(DAM_VICT_KILLED);
	}

	if (ch && (PLR_FLAGGED(victim, PLR_MAILING) ||
			PLR_FLAGGED(victim, PLR_WRITING) ||
			PLR_FLAGGED(victim, PLR_OLC)) && ch != victim) {
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"%s has attacked %s while writing at %d.", GET_NAME(ch),
			GET_NAME(victim), ch->in_room->number);
		stop_fighting(ch);
		stop_fighting(victim);
		send_to_char(ch, "NO!  Do you want to be ANNIHILATED by the gods?!\r\n");
		DAM_RETURN(0);
	}

	if (ch) {
		if( ( IS_NPC(ch) || IS_NPC(victim) ) && 
			  affected_by_spell(ch, SPELL_QUAD_DAMAGE) ) {
			dam <<= 2;
		} else if (AFF3_FLAGGED(ch, AFF3_DOUBLE_DAMAGE)) {
			dam <<= 1;
		}
		if (IS_AFFECTED_3(ch, AFF3_INST_AFF)) {	// In combat instant affects
			// Charging into combat gives a damage bonus
			if ((af = affected_by_spell(ch, SKILL_CHARGE))) {
				dam += (dam * af->modifier / 10);
			}
		}
		if ((af = affected_by_spell(ch, SPELL_SANCTIFICATION))) {
			if (IS_EVIL(victim) && !IS_SOULLESS(victim))
				dam += (dam * GET_REMORT_GEN(ch)) / 20;
			else if (ch != victim && IS_GOOD(victim)) {
				send_to_char(ch, "You have been de-sanctified!\r\n");
				affect_remove(ch, af);
			}
		}
	}

	if (CANNOT_DAMAGE(ch, victim, weap, attacktype))
		dam = 0;

	/* vendor protection */
	if (!ok_damage_vendor(ch, victim)) {
		DAM_RETURN(0);
	}

	/* newbie protection and PLR_NOPK check */
	if (ch && ch != victim && !IS_NPC(ch) && !IS_NPC(victim)) 
	{
		if(!is_arena_combat(ch, victim)) {
			if (PLR_FLAGGED(ch, PLR_NOPK)) {
				send_to_char(ch, "A small dark shape flies in from the future and sticks to your eyebrow.\r\n");
				DAM_RETURN(DAM_ATTACK_FAILED);
			}
			if (PLR_FLAGGED(victim, PLR_NOPK)) {
				send_to_char(ch, "A small dark shape flies in from the future and sticks to your nose.\r\n");
				DAM_RETURN(DAM_ATTACK_FAILED);
			}
		}

		if (victim->isNewbie() &&
			!is_arena_combat(ch, victim) &&
			GET_LEVEL(ch) < LVL_IMMORT) {
			act("$N is currently under new character protection.",
				FALSE, ch, 0, victim, TO_CHAR);
			act("You are protected by the gods against $n's attack!",
				FALSE, ch, 0, victim, TO_VICT);
			slog("%s protected against %s ( damage ) at %d\n",
				GET_NAME(victim), GET_NAME(ch), victim->in_room->number);
			if (victim == FIGHTING(ch))
				stop_fighting(ch);
			if (ch == FIGHTING(victim))
				stop_fighting(victim);
			DAM_RETURN(DAM_ATTACK_FAILED);
		}

		if (ch->isNewbie() &&
			!is_arena_combat(ch, victim)) {
			send_to_char(ch, 
				"You are currently under new player protection, which expires at level 41.\r\n"
				"You cannot attack other players while under this protection.\r\n");
			DAM_RETURN(0);
		}
	}

	if (victim->getPosition() < POS_FIGHTING)
		dam += (dam * (POS_FIGHTING - victim->getPosition())) / 3;

	if (ch) {
		if (MOB2_FLAGGED(ch, MOB2_UNAPPROVED) && !victim->isTester())
			dam = 0;

		if (ch->isTester() && !IS_MOB(victim) && !victim->isTester())
			dam = 0;

		if (IS_MOB(victim) && GET_LEVEL(ch) >= LVL_AMBASSADOR &&
				GET_LEVEL(ch) < LVL_TIMEGOD && !mini_mud)
			dam = 0;

		if (victim->master == ch)
			stop_follower(victim);
		appear(ch, victim);
	}

	if (attacktype < MAX_SPELLS) {
		if (SPELL_IS_PSIONIC(attacktype) &&
			affected_by_spell(victim, SPELL_PSYCHIC_RESISTANCE))
			dam >>= 1;
		if ((SPELL_IS_MAGIC(attacktype) || SPELL_IS_DIVINE(attacktype)) &&
			affected_by_spell(victim, SPELL_MAGICAL_PROT))
			dam -= dam >> 2;
	}

	if (attacktype == SPELL_PSYCHIC_CRUSH)
		location = WEAR_HEAD;

	if (attacktype == TYPE_ACID_BURN && location == -1) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(ch, i))
				damage_eq(ch, GET_EQ(ch, i), (dam >> 1), attacktype);
			if (GET_IMPLANT(ch, i))
				damage_eq(ch, GET_IMPLANT(ch, i), (dam >> 1), attacktype);
		}
	}

	if (IS_WEAPON(attacktype)
			&& GET_EQ(victim, WEAR_SHIELD)
			&& CHECK_SKILL(victim, SKILL_SHIELD_MASTERY) > 20
			&& victim->getLevelBonus(SKILL_SHIELD_MASTERY) > number(0, 600)) {
		act("$N deflects your attack with $S shield!", true,
			ch, GET_EQ(victim, WEAR_SHIELD), victim, TO_CHAR);
		act("You deflect $n's attack with $p!", true,
			ch, GET_EQ(victim, WEAR_SHIELD), victim, TO_VICT);
		act("$N deflects $n's attack with $S shield!", true,
			ch, GET_EQ(victim, WEAR_SHIELD), victim, TO_NOTVICT);
		location = WEAR_SHIELD;
		deflected = true;
	}

	/** check for armor **/
	if (location != -1) {

		if (location == WEAR_RANDOM) {
			location = choose_random_limb(victim);
		}

		if ((location == WEAR_FINGER_R ||
				location == WEAR_FINGER_L) && GET_EQ(victim, WEAR_HANDS)) {
			location = WEAR_HANDS;
		}

		if ((location == WEAR_EAR_R ||
				location == WEAR_EAR_L) && GET_EQ(victim, WEAR_HEAD)) {
			location = WEAR_HEAD;
		}

		obj = GET_EQ(victim, location);
		impl = GET_IMPLANT(victim, location);

		// implants are protected by armor
		if (obj && impl && IS_OBJ_TYPE(obj, ITEM_ARMOR) &&
			!IS_OBJ_TYPE(impl, ITEM_ARMOR))
			impl = NULL;

		if (obj || impl) {

			if (obj && OBJ_TYPE(obj, ITEM_ARMOR)) {
				eq_dam = (GET_OBJ_VAL(obj, 0) * dam) / 100;
				if (location == WEAR_SHIELD)
					eq_dam <<= 1;
			}
			if (impl && OBJ_TYPE(impl, ITEM_ARMOR))
				impl_dam = (GET_OBJ_VAL(impl, 0) * dam) / 100;

			weap_dam = eq_dam + impl_dam;

			if (obj && !eq_dam)
				eq_dam = (dam >> 4);

			if ((!obj || !OBJ_TYPE(obj, ITEM_ARMOR)) && impl && !impl_dam)
				impl_dam = (dam >> 5);

			/* here are the damage absorbing characteristics */
			if ((attacktype == TYPE_BLUDGEON || attacktype == TYPE_HIT ||
					attacktype == TYPE_POUND || attacktype == TYPE_PUNCH)) {
				if (obj) {
					if (IS_LEATHER_TYPE(obj) || IS_CLOTH_TYPE(obj) ||
						IS_FLESH_TYPE(obj))
						eq_dam = (int)(eq_dam * 0.7);
					else if (IS_METAL_TYPE(obj))
						eq_dam = (int)(eq_dam * 1.3);
				}
				if (impl) {
					if (IS_LEATHER_TYPE(impl) || IS_CLOTH_TYPE(impl) ||
						IS_FLESH_TYPE(impl))
						eq_dam = (int)(eq_dam * 0.7);
					else if (IS_METAL_TYPE(impl))
						eq_dam = (int)(eq_dam * 1.3);
				}
			} else if ((attacktype == TYPE_SLASH || attacktype == TYPE_PIERCE
					|| attacktype == TYPE_CLAW || attacktype == TYPE_STAB
					|| attacktype == TYPE_RIP || attacktype == TYPE_CHOP)) {
				if (obj && (IS_METAL_TYPE(obj) || IS_STONE_TYPE(obj))) {
					eq_dam = (int)(eq_dam * 0.7);
					weap_dam = (int)(weap_dam * 1.3);
				}
				if (impl && (IS_METAL_TYPE(impl) || IS_STONE_TYPE(impl))) {
					impl_dam = (int)(impl_dam * 0.7);
					weap_dam = (int)(weap_dam * 1.3);
				}
			} else if (attacktype == SKILL_PROJ_WEAPONS) {
				if (obj) {
					if (IS_MAT(obj, MAT_KEVLAR))
						eq_dam = (int)(eq_dam * 1.6);
					else if (IS_METAL_TYPE(obj))
						eq_dam = (int)(eq_dam * 1.3);
				}
			}

			dam = MAX(0, (dam - eq_dam - impl_dam));

			if (obj) {
				if (IS_OBJ_STAT(obj, ITEM_MAGIC_NODISPEL))
					eq_dam >>= 1;
				if (IS_OBJ_STAT(obj,
						ITEM_MAGIC | ITEM_BLESS | ITEM_DAMNED))
					eq_dam = (int)(eq_dam * 0.8);
				if (IS_OBJ_STAT2(obj, ITEM2_GODEQ | ITEM2_CURSED_PERM))
					eq_dam = (int)(eq_dam * 0.7);
			}
			if (impl) {
				if (IS_OBJ_STAT(impl, ITEM_MAGIC_NODISPEL))
					impl_dam >>= 1;
				if (IS_OBJ_STAT(impl,
						ITEM_MAGIC | ITEM_BLESS | ITEM_DAMNED))
					impl_dam = (int)(impl_dam * 0.8);
				if (IS_OBJ_STAT2(impl, ITEM2_GODEQ | ITEM2_CURSED_PERM))
					impl_dam = (int)(impl_dam * 0.7);
			}
			//    struct obj_data *the_obj = impl ? impl : obj;

			/* here are the object oriented damage specifics */
			if ((attacktype == TYPE_SLASH || attacktype == TYPE_PIERCE ||
					attacktype == TYPE_CLAW || attacktype == TYPE_STAB ||
					attacktype == TYPE_RIP || attacktype == TYPE_CHOP)) {
				if (obj && (IS_METAL_TYPE(obj) || IS_STONE_TYPE(obj))) {
					eq_dam = (int)(eq_dam * 0.7);
					weap_dam = (int)(weap_dam * 1.3);
				}
				if (impl && (IS_METAL_TYPE(impl) || IS_STONE_TYPE(impl))) {
					impl_dam = (int)(impl_dam * 0.7);
					weap_dam = (int)(weap_dam * 1.3);
				}
			} else if (attacktype == SPELL_PSYCHIC_CRUSH)
				eq_dam <<= 7;
			// OXIDIZE Damaging equipment
			else if (attacktype == SPELL_OXIDIZE) {
				// Chemical stability stops oxidize dam on eq.
				if (affected_by_spell(victim, SPELL_CHEMICAL_STABILITY)) {
					eq_dam = 0;
				} else if ((obj && IS_FERROUS(obj))) {
					apply_soil_to_char(victim, GET_EQ(victim, location),
						SOIL_RUST, location);
					eq_dam <<= 5;
				} else if (impl && IS_FERROUS(impl)) {
					apply_soil_to_char(victim, GET_IMPLANT(victim, location),
						SOIL_RUST, location);
					eq_dam <<= 5;
				} else if ((obj && IS_BURNABLE_TYPE(obj))) {
					apply_soil_to_char(victim, GET_EQ(victim, location),
						SOIL_CHAR, location);
					eq_dam <<= 3;
				} else if (impl && IS_BURNABLE_TYPE(impl)) {
					apply_soil_to_char(victim, GET_IMPLANT(victim, location),
						SOIL_CHAR, location);
					eq_dam <<= 3;
				}
			}
			if (weap) {
				if (IS_OBJ_STAT(weap, ITEM_MAGIC_NODISPEL))
					weap_dam >>= 1;
				if (IS_OBJ_STAT(weap,
						ITEM_MAGIC | ITEM_BLESS | ITEM_DAMNED))
					weap_dam = (int)(weap_dam * 0.8);
				if (IS_OBJ_STAT2(weap, ITEM2_CAST_WEAPON | ITEM2_GODEQ |
						ITEM2_CURSED_PERM))
					weap_dam = (int)(weap_dam * 0.7);
			}
		}
	}
	if (dam_object && dam)
		check_object_killer(dam_object, victim);

	if (deflected) {
		// We have to do all the object damage for shields here, after
		// it's calculated
		if (obj && eq_dam)
			damage_eq(ch, obj, eq_dam, attacktype);
		if (impl && impl_dam)
			damage_eq(ch, impl, impl_dam, attacktype);

		if (weap && (attacktype != SKILL_PROJ_WEAPONS) &&
			attacktype != SKILL_ENERGY_WEAPONS)
			damage_eq(ch, weap, MAX(weap_dam, dam >> 6), attacktype);
		return 0;
	}

	//
	// attacker is a character
	//

	if (ch) {

		//
		// attack type is a skill type
		//

		if (!cur_weap && ch != victim && dam &&
			(attacktype < MAX_SKILLS || attacktype >= TYPE_HIT) &&
			(attacktype > MAX_SPELLS
				|| IS_SET(spell_info[attacktype].routines, MAG_TOUCH))
			&& !SPELL_IS_PSIONIC(attacktype)) {

			//
			// vict has prismatic sphere
			//

			if (IS_AFFECTED_3(victim, AFF3_PRISMATIC_SPHERE) &&
				attacktype < MAX_SKILLS &&
				(CHECK_SKILL(ch, attacktype) + GET_LEVEL(ch))
				< (GET_INT(victim) + number(70, 130 + GET_LEVEL(victim)))) {
				act("You are deflected by $N's prismatic sphere!",
					FALSE, ch, 0, victim, TO_CHAR);
				act("$n is deflected by $N's prismatic sphere!",
					FALSE, ch, 0, victim, TO_NOTVICT);
				act("$n is deflected by your prismatic sphere!",
					FALSE, ch, 0, victim, TO_VICT);
				int retval =
					damage_attacker(victim, ch, dice(30, 3) + (dam >> 2),
					SPELL_PRISMATIC_SPHERE, -1);

				if (!IS_SET(retval, DAM_VICT_KILLED)) {
					gain_skill_prof(victim, SPELL_PRISMATIC_SPHERE);
				}
				SET_BIT(retval, DAM_ATTACK_FAILED);
				DAM_RETURN(retval);
			}
			//
			// vict has electrostatic field
			//

			if ((af = affected_by_spell(victim, SPELL_ELECTROSTATIC_FIELD))) {
				if (!CHAR_WITHSTANDS_ELECTRIC(ch)
					&& !mag_savingthrow(ch, af->level, SAVING_ROD)) {
					// reduces duration if it hits
					if (af->duration > 1) {
						af->duration--;
					}
					int retval =
						damage_attacker(victim, ch, dice(3, af->level),
						SPELL_ELECTROSTATIC_FIELD, -1);

					if (!IS_SET(retval, DAM_VICT_KILLED)) {
						gain_skill_prof(victim, SPELL_ELECTROSTATIC_FIELD);
					}
					SET_BIT(retval, DAM_ATTACK_FAILED);
					DAM_RETURN(retval);
				}
			}
			//
			// attack type is a nonweapon melee type
			//

			if (attacktype < MAX_SKILLS && attacktype != SKILL_BEHEAD &&
				attacktype != SKILL_SECOND_WEAPON
				&& attacktype != SKILL_IMPALE) {

				int retval = 0;	// damage result for end of block return

				//
				// vict has prismatic sphere
				//

				if (IS_AFFECTED_3(victim, AFF3_PRISMATIC_SPHERE) &&
					!mag_savingthrow(ch, GET_LEVEL(victim), SAVING_ROD)) {

					retval =
						damage_attacker(victim, ch, dice(30,
							3) + (IS_MAGE(victim) ? (dam >> 2) : 0),
						SPELL_PRISMATIC_SPHERE, -1);

					if (!IS_SET(retval, DAM_ATTACKER_KILLED)) {
						WAIT_STATE(ch, PULSE_VIOLENCE);
					}

				}
				//
				// vict has blade barrier
				//

				else if (IS_AFFECTED_2(victim, AFF2_BLADE_BARRIER)) {
					retval = damage_attacker(victim, ch,
						GET_LEVEL(victim) + (dam >> 4),
						SPELL_BLADE_BARRIER, -1);

				}
				//
				// vict has fire shield
				//

				else if (IS_AFFECTED_2(victim, AFF2_FIRE_SHIELD) &&
					attacktype != SKILL_BACKSTAB &&
					!mag_savingthrow(ch, GET_LEVEL(victim), SAVING_BREATH) &&
					!IS_AFFECTED_2(ch, AFF2_ABLAZE) &&
					!CHAR_WITHSTANDS_FIRE(ch)) {

					retval = damage_attacker(victim, ch, dice(8, 8) +
						(IS_MAGE(victim) ? (dam >> 3) : 0),
						SPELL_FIRE_SHIELD, -1);

					if (!IS_SET(retval, DAM_ATTACKER_KILLED)) {
						SET_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
					}

				}
				//
				// vict has energy field
				//

				else if (IS_AFFECTED_2(victim, AFF2_ENERGY_FIELD)) {
					af = affected_by_spell(victim, SKILL_ENERGY_FIELD);
					if (!mag_savingthrow(ch,
							af ? af->level : GET_LEVEL(victim),
							SAVING_ROD) && !CHAR_WITHSTANDS_ELECTRIC(ch)) {
						retval = damage_attacker(victim, ch,
							af ? dice(3, MAX(10, af->level)) : dice(3, 8),
							SKILL_ENERGY_FIELD, -1);

						if (!IS_SET(retval, DAM_ATTACKER_KILLED)) {
							ch->setPosition(POS_SITTING);
							WAIT_STATE(ch, 2 RL_SEC);
						}
					}
				}
				//
				// return if anybody got killed
				//

				if (retval) {
					DAM_RETURN(retval);
				}

			}
		}
	}
#define BAD_ATTACK_TYPE(attacktype) (attacktype == TYPE_BLEED || \
                                         attacktype == SPELL_POISON)

	/********* OHH SHIT!  Begin new damage reduction code --N **********/
	float dam_reduction = 0;

	if (affected_by_spell(victim, SPELL_MANA_SHIELD)
		&& !BAD_ATTACK_TYPE(attacktype)) {
		mana_loss = (dam * GET_MSHIELD_PCT(victim)) / 100;
		mana_loss = MIN(GET_MANA(victim) - GET_MSHIELD_LOW(victim), mana_loss);
		mana_loss = MAX(mana_loss, 0);
		GET_MANA(victim) -= mana_loss;

		if (GET_MANA(victim) <= GET_MSHIELD_LOW(victim)) {
			send_to_char(victim, "Your mana shield has expired.\r\n");
			affect_from_char(victim, SPELL_MANA_SHIELD);
		}

		dam = MAX(0, dam - mana_loss);
	}

	if (ch && GET_CLASS(ch) == CLASS_CLERIC && IS_EVIL(ch)) {
		// evil clerics get an alignment-based damage bonus, up to 30% on
		// new moons, %10 otherwise.  It may look like we're subtracting
		// damage, but the alignment is negative by definition.
		if (get_lunar_phase(lunar_day) == MOON_NEW)
			dam -= (int)((dam * GET_ALIGNMENT(ch)) / 3000);
		else
			dam -= (int)((dam * GET_ALIGNMENT(ch)) / 10000);
	}

	//******************* Reduction based on the attacker ********************/
	//************ Knights, Clerics, and Monks out of alignment **************/
	if (ch) {
		if (IS_NEUTRAL(ch)) {
			if (IS_KNIGHT(ch) || IS_CLERIC(ch))
				dam = dam / 4;
		} else {
			if (IS_MONK(ch))
				dam = dam / 4;
		}
	}
	// ALL previous damage reduction code that was based off of character 
	// attributes has been moved to the function below.  See structs/Creature.cc
	dam_reduction = victim->getDamReduction(ch);

	dam -= (int)(dam * dam_reduction);

	/*********************** End N's coding spree ***********************/

	if (IS_PUDDING(victim) || IS_SLIME(victim) || IS_TROLL(victim))
		dam >>= 2;

	switch (attacktype) {
	case TYPE_OVERLOAD:
	case TYPE_TAINT_BURN:
		break;
	case SPELL_POISON:
		if (IS_UNDEAD(victim))
			dam = 0;
		break;
	case SPELL_STIGMATA:
	case TYPE_HOLYOCEAN:
		if (IS_UNDEAD(victim) || IS_EVIL(victim))	// is_evil added for stigmata
			dam <<= 1;
		if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD))
			dam >>= 1;
		break;
	case SPELL_OXIDIZE:
		if (IS_CYBORG(victim))
			dam <<= 1;
		else if (affected_by_spell(victim, SPELL_CHEMICAL_STABILITY))
			dam >>= 2;
		break;
	case SPELL_DISRUPTION:
		if (IS_UNDEAD(victim) || IS_CYBORG(victim) || IS_ROBOT(victim))
			dam <<= 1;
		break;
	case SPELL_CALL_LIGHTNING:
	case SPELL_LIGHTNING_BOLT:
	case SPELL_LIGHTNING_BREATH:
	case JAVELIN_OF_LIGHTNING:
	case SKILL_ENERGY_FIELD:
	case SKILL_DISCHARGE:
		if (IS_VAMPIRE(victim))
			dam >>= 1;
		if (OUTSIDE(victim)
			&& victim->in_room->zone->weather->sky == SKY_LIGHTNING)
			dam <<= 1;
		if (IS_AFFECTED_2(victim, AFF2_PROT_LIGHTNING))
			dam >>= 1;
		if (IS_CYBORG(victim))
			dam <<= 1;
		break;
	case SPELL_HAILSTORM:
		if (OUTSIDE(victim)
			&& victim->in_room->zone->weather->sky >= SKY_RAINING)
			dam <<= 1;
		break;
	case SPELL_HELL_FIRE:
	case SPELL_TAINT:
	case SPELL_BURNING_HANDS:
	case SPELL_FIREBALL:
	case SPELL_FLAME_STRIKE:
	case SPELL_FIRE_ELEMENTAL:
	case SPELL_FIRE_BREATH:
	case TYPE_ABLAZE:
	case SPELL_FIRE_SHIELD:
	case TYPE_FLAMETHROWER:
		if (IS_VAMPIRE(victim) && OUTSIDE(victim) &&
			victim->in_room->zone->weather->sunlight == SUN_LIGHT &&
			GET_PLANE(victim->in_room) < PLANE_ASTRAL)
			dam <<= 2;

		if ((victim->in_room->sector_type == SECT_PITCH_PIT ||
				ROOM_FLAGGED(victim->in_room, ROOM_EXPLOSIVE_GAS)) &&
			!ROOM_FLAGGED(victim->in_room, ROOM_FLAME_FILLED)) {
			send_to_room("The room bursts into flames!!!\r\n",
				victim->in_room);
			if (victim->in_room->sector_type == SECT_PITCH_PIT)
				rm_aff.description =
					str_dup("   The pitch is ablaze with raging flames!\r\n");
			else
				rm_aff.description =
					str_dup("   The room is ablaze with raging flames!\r\n");

			rm_aff.level = (ch != NULL) ? GET_LEVEL(ch) : number(1, 49);
			rm_aff.duration = number(3, 8);
			rm_aff.type = RM_AFF_FLAGS;
			rm_aff.flags = ROOM_FLAME_FILLED;
			affect_to_room(victim->in_room, &rm_aff);
			sound_gunshots(victim->in_room, SPELL_FIREBALL, 8, 1);
		}

		if (CHAR_WITHSTANDS_FIRE(victim))
			dam >>= 1;
		if (IS_TROLL(victim) || IS_PUDDING(victim) || IS_SLIME(victim))
			dam <<= 2;
		break;
	case SPELL_CONE_COLD:
	case SPELL_CHILL_TOUCH:
	case SPELL_ICY_BLAST:
	case SPELL_FROST_BREATH:
	case TYPE_FREEZING:
		if (CHAR_WITHSTANDS_COLD(victim))
			dam >>= 1;
		break;
	case TYPE_BLEED:
		if (!CHAR_HAS_BLOOD(victim))
			dam = 0;
		break;
	case SPELL_STEAM_BREATH:
	case TYPE_BOILING_PITCH:
		if (AFF3_FLAGGED(victim, AFF3_PROT_HEAT))
			dam >>= 1;
	case TYPE_ACID_BURN:
	case SPELL_ACIDITY:
	case SPELL_ACID_BREATH:
		if (affected_by_spell(victim, SPELL_CHEMICAL_STABILITY))
			dam >>= 1;
		break;
	}



	// rangers' critical hit
	if (ch && IS_RANGER(ch) && dam > 10 &&
		IS_WEAPON(attacktype) && number(0, 74) <= GET_REMORT_GEN(ch)) {
		send_to_char(ch, "CRITICAL HIT!\r\n");
		act("$n has scored a CRITICAL HIT!", FALSE, ch, 0, victim,
			TO_VICT);
		dam += (dam * (GET_REMORT_GEN(ch) + 4)) / 14;
	}

	if (ch)
		hard_damcap = MAX(20 + GET_LEVEL(ch) + (GET_REMORT_GEN(ch) * 2),
			GET_LEVEL(ch) * 20 + (GET_REMORT_GEN(ch) * GET_LEVEL(ch) * 2));
	else
		hard_damcap = 7000;

	dam = MAX(MIN(dam, hard_damcap), 0);

	//
	// characters under the effects of vampiric regeneration
	//

	if ((af = affected_by_spell(victim, SPELL_VAMPIRIC_REGENERATION))) {
		// pc caster
		if (ch && !IS_NPC(ch) && GET_IDNUM(ch) == af->modifier) {
			sprintf(buf, "You drain %d hitpoints from $N!", dam);
			act(buf, FALSE, ch, 0, victim, TO_CHAR);
			GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dam);
			af->duration--;
			if (af->duration <= 0) {
				affect_remove(victim, af);
			}
		}
	}

	if ((af = affected_by_spell(victim, SPELL_LOCUST_REGENERATION))) {
		// pc caster
		if (ch && !IS_NPC(ch) && GET_IDNUM(ch) == af->modifier) {
			if (victim && (GET_MANA(victim) > 0)) {
				int manadrain = MIN((int)(dam * 0.75), GET_MANA(victim));
				sprintf(buf, "You drain %d mana from $N!", manadrain);
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - (dam >> 2));
				if (GET_MOVE(ch) > 0)
					GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + dam);
				af->duration--;
				if (af->duration <= 0) {
					affect_remove(victim, af);
				}
			}
		}
	}

	GET_HIT(victim) -= dam;
	if (!IS_NPC(victim))
		GET_TOT_DAM(victim) += dam;

	if (ch && ch != victim
		&& !(MOB2_FLAGGED(victim, MOB2_UNAPPROVED) ||
			ch->isTester() || IS_PET(ch) || IS_PET(victim))) {
		// Gaining XP for damage dealt.
		int exp = MIN(GET_LEVEL(ch) * GET_LEVEL(ch) * GET_LEVEL(ch), GET_LEVEL(victim) * dam);

		exp = ch->getPenalizedExperience( exp, victim );

		gain_exp(ch, exp);
	}
	// check for killer flags and remove sleep/etc...
	if (ch && ch != victim) {
		// remove sleep/flood on a damaging hit
		if (dam) {
			if (IS_AFFECTED(victim, AFF_SLEEP)) {
				if (affected_by_spell(victim, SPELL_SLEEP))
					affect_from_char(victim, SPELL_SLEEP);
				if (affected_by_spell(victim, SPELL_MELATONIC_FLOOD))
					affect_from_char(victim, SPELL_MELATONIC_FLOOD);
			}
		}
		// remove stasis even on a miss
		if (AFF3_FLAGGED(victim, AFF3_STASIS)) {
			send_to_char(victim, "Emergency restart of system procesess...\r\n");
			REMOVE_BIT(AFF3_FLAGS(victim), AFF3_STASIS);
			WAIT_STATE(victim, (5 - (CHECK_SKILL(victim,
							SKILL_FASTBOOT) >> 5)) RL_SEC);
		}
		// check for killer flags right here
		if (ch != FIGHTING(victim) && !IS_DEFENSE_ATTACK(attacktype)) {
			check_killer(ch, victim, "secondary in damage()");
		}
	}
	update_pos(victim);

	/*
	 * skill_message sends a message from the messages file in lib/misc.
	 * dam_message just sends a generic "You hit $n extremely hard.".
	 * skill_message is preferable to dam_message because it is more
	 * descriptive.
	 * 
	 * If we are _not_ attacking with a weapon ( i.e. a spell ), always use
	 * skill_message. If we are attacking with a weapon: If this is a miss or a
	 * death blow, send a skill_message if one exists; if not, default to a
	 * dam_message. Otherwise, always send a dam_message.
	 */

	if (!IS_WEAPON(attacktype)) {
		if (ch)
			skill_message(dam, ch, victim, attacktype);

		// some "non-weapon" attacks involve a weapon, e.g. backstab
		if (weap)
			damage_eq(ch, weap, MAX(weap_dam, dam >> 6), attacktype);

		if (obj)
			damage_eq(ch, obj, eq_dam, attacktype);

		if (impl && impl_dam)
			damage_eq(ch, impl, impl_dam, attacktype);

		// ignite the victim if applicable
		if (!IS_AFFECTED_2(victim, AFF2_ABLAZE) &&
			(attacktype == SPELL_FIREBALL ||
				attacktype == SPELL_FIRE_BREATH ||
				attacktype == SPELL_HELL_FIRE ||
				attacktype == SPELL_FLAME_STRIKE ||
				attacktype == SPELL_METEOR_STORM ||
				attacktype == TYPE_FLAMETHROWER ||
				attacktype == SPELL_FIRE_ELEMENTAL)) {
			if (!mag_savingthrow(victim, 50, SAVING_BREATH) &&
				!CHAR_WITHSTANDS_FIRE(victim)) {
				act("$n's body suddenly ignites into flame!",
					FALSE, victim, 0, 0, TO_ROOM);
				act("Your body suddenly ignites into flame!",
					FALSE, victim, 0, 0, TO_CHAR);
				SET_BIT(AFF2_FLAGS(victim), AFF2_ABLAZE);
			}
		}
		// transfer sickness if applicable
		if (ch && ch != victim &&
			((attacktype > MAX_SPELLS && attacktype < MAX_SKILLS &&
					attacktype != SKILL_BEHEAD && attacktype != SKILL_IMPALE)
				|| (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING))) {
			if ((IS_SICK(ch) || IS_ZOMBIE(ch)
					|| (ch->in_room->zone->number == 163 && !IS_NPC(victim)))
				&& !IS_SICK(victim) && !IS_UNDEAD(victim)) {
				call_magic(ch, victim, 0, SPELL_SICKNESS, GET_LEVEL(ch),
					CAST_PARA);
			}
		}
	} else if (ch) {
		// it is a weapon attack
		if (victim->getPosition() == POS_DEAD || dam == 0) {
			if (!skill_message(dam, ch, victim, attacktype))
				dam_message(dam, ch, victim, attacktype, location);
		} else {
			dam_message(dam, ch, victim, attacktype, location);
		}

		if (obj && eq_dam)
			damage_eq(ch, obj, eq_dam, attacktype);
		if (impl && impl_dam)
			damage_eq(ch, impl, impl_dam, attacktype);

		if (weap && (attacktype != SKILL_PROJ_WEAPONS) &&
			attacktype != SKILL_ENERGY_WEAPONS)
			damage_eq(ch, weap, MAX(weap_dam, dam >> 6), attacktype);

		//
		// aliens spray blood all over the room
		//

		if (IS_ALIEN_1(victim) && (attacktype == TYPE_SLASH ||
				attacktype == TYPE_RIP ||
				attacktype == TYPE_BITE ||
				attacktype == TYPE_CLAW ||
				attacktype == TYPE_PIERCE ||
				attacktype == TYPE_STAB ||
				attacktype == TYPE_CHOP ||
				attacktype == SPELL_BLADE_BARRIER)) {
			CreatureList::iterator it = victim->in_room->people.begin();
			for (; it != victim->in_room->people.end(); ++it) {
				if (*it == victim || number(0, 8))
					continue;
				bool is_char = false;
				if (*it == ch)
					is_char = true;
				int retval =
					damage(victim, *it, dice(4, GET_LEVEL(victim)),
					TYPE_ALIEN_BLOOD, -1);

				if (is_char && IS_SET(retval, DAM_VICT_KILLED)) {
					DAM_RETURN(DAM_ATTACKER_KILLED);
				}
			}
		}
	}
	// Use send_to_char -- act(  ) doesn't send message if you are DEAD.
	switch (victim->getPosition()) {

		// Mortally wounded
	case POS_MORTALLYW:
		act("$n is mortally wounded, and will die soon, if not aided.",
			TRUE, victim, 0, 0, TO_ROOM);
		send_to_char(victim, 
			"You are mortally wounded, and will die soon, if not aided.\r\n");
/*        if (!IS_NPC(ch) && IS_CLERIC(ch) && IS_EVIL(ch) && (CHECK_SKILL(ch, SPELL_DEATH_KNELL) > 90)) {
			int return_flags = 0;

			stop_fighting(victim);
			stop_fighting(ch);
            call_magic(ch, victim, 0, SPELL_DEATH_KNELL, 
                       GET_LEVEL(ch), CAST_SPELL, &return_flags);
			DAM_RETURN(return_flags);
        }
        victim->setPosition(POS_DEAD);*/

		break;

		// Incapacitated
	case POS_INCAP:
		act("$n is incapacitated and will slowly die, if not aided.", TRUE,
			victim, 0, 0, TO_ROOM);
		send_to_char(victim, 
			"You are incapacitated and will slowly die, if not aided.\r\n");
		break;

		// Stunned
	case POS_STUNNED:
		act("$n is stunned, but will probably regain consciousness again.",
			TRUE, victim, 0, 0, TO_ROOM);
		send_to_char(victim, 
			"You're stunned, but will probably regain consciousness again.\r\n");
		break;

		// Dead
	case POS_DEAD:
		act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
		send_to_char(victim, "You are dead!  Sorry...\r\n");
		break;

		// pos >= Sleeping ( Fighting, Standing, Flying, Mounted... )
	default:
		{
			if (dam > (GET_MAX_HIT(victim) >> 2))
				act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);

			if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2) &&
				GET_HIT(victim) < 200) {
				sprintf(buf2,
					"%sYou wish that your wounds would stop %sBLEEDING%s%s so much!%s\r\n",
					CCRED(victim, C_SPR), CCBLD(victim, C_SPR), CCNRM(victim,
						C_SPR), CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
				send_to_char(victim, "%s", buf2);
			}
			//
			// NPCs fleeing due to MORALE
			//

			if (ch && IS_NPC(victim) && ch != victim &&
				!KNOCKDOWN_SKILL(attacktype) &&
				victim->getPosition() > POS_SITTING
				&& !MOB_FLAGGED(victim, MOB_SENTINEL) && !IS_DRAGON(victim)
				&& !IS_UNDEAD(victim) && GET_CLASS(victim) != CLASS_ARCH
				&& GET_CLASS(victim) != CLASS_DEMON_PRINCE
				&& GET_MOB_WAIT(ch) <= 0 && !MOB_FLAGGED(ch, MOB_SENTINEL)
				&& (100 - ((GET_HIT(victim) * 100) / GET_MAX_HIT(victim))) >
				GET_MORALE(victim) + number(-5, 10 + (GET_INT(victim) >> 2))) {

				if (GET_HIT(victim) > 0) {
					int retval = 0;
					do_flee(victim, "", 0, 0, &retval);
					if (IS_SET(retval, DAM_ATTACKER_KILLED)) {
						DAM_RETURN(DAM_ATTACKER_KILLED);
					}

				} else {
					mudlog(LVL_DEMI, BRF, true,
						"ERROR: %s was at position %d with %d hit points and tried to flee.",
						GET_NAME(victim), victim->getPosition(),
						GET_HIT(victim));
				}
			}
			//
			// cyborgs sustaining internal damage and perhaps exploding
			//

			if (IS_CYBORG(victim) && !IS_NPC(victim) &&
				GET_TOT_DAM(victim) >= max_component_dam(victim)) {
				if (GET_BROKE(victim)) {
					if (!AFF3_FLAGGED(victim, AFF3_SELF_DESTRUCT)) {
						send_to_char(victim, "Your %s has been severely damaged.\r\n"
							"Systems cannot support excessive damage to this and %s.\r\n"
							"%sInitiating Self-Destruct Sequence.%s\r\n",
							component_names[number(1,
									NUM_COMPS)][GET_OLD_CLASS(victim)],
							component_names[(int)
								GET_BROKE(victim)][GET_OLD_CLASS(victim)],
							CCRED_BLD(victim, C_NRM), CCNRM(victim, C_NRM));
						act("$n has auto-initiated self destruct sequence!\r\n"
							"CLEAR THE AREA!!!!", FALSE, victim, 0, 0,
							TO_ROOM | TO_SLEEP);
						MEDITATE_TIMER(victim) = 4;
						SET_BIT(AFF3_FLAGS(victim), AFF3_SELF_DESTRUCT);
						WAIT_STATE(victim, PULSE_VIOLENCE * 7);	/* Just enough to die */
					}
				} else {
					GET_BROKE(victim) = number(1, NUM_COMPS);
					act("$n staggers and begins to smoke!", FALSE, victim, 0,
						0, TO_ROOM);
					send_to_char(victim, "Your %s has been damaged!\r\n",
						component_names[(int)
							GET_BROKE(victim)][GET_OLD_CLASS(victim)]);
					GET_TOT_DAM(victim) = 0;
				}

			}
			// this is the block that handles things that happen when someone is damaging
			// someone else.  ( not damaging self or being damaged by a null char, e.g. bomb )

			if (ch && victim != ch) {

				//
				// see if the victim flees due to WIMPY
				//

				if (!IS_NPC(victim) &&
					GET_HIT(victim) < GET_WIMP_LEV(victim)
					&& GET_HIT(victim) > 0) {
					send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
					if (KNOCKDOWN_SKILL(attacktype) && dam)
						victim->setPosition(POS_SITTING);

					int retval = 0;
					do_flee(victim, "", 0, 0, &retval);
					if (IS_SET(retval, DAM_ATTACKER_KILLED)) {
						DAM_RETURN(DAM_ATTACKER_KILLED);
					}

					break;
				}
				//
				// see if the vict flees due to FEAR
				//

				else if (affected_by_spell(victim, SPELL_FEAR) &&
					!number(0, (GET_LEVEL(victim) >> 3) + 1)
					&& GET_HIT(victim) > 0) {
					if (KNOCKDOWN_SKILL(attacktype) && dam)
						victim->setPosition(POS_SITTING);

					int retval = 0;
					do_flee(victim, "", 0, 0, &retval);
					if (IS_SET(retval, DAM_ATTACKER_KILLED)) {
						DAM_RETURN(DAM_ATTACKER_KILLED);
					}

					break;
				}
				// this block handles things that happen IF and ONLY IF the ch has
				// initiated the attack.  We assume that ch is attacking unless the
				// damage is from a DEFENSE ATTACK
				if (ch && !IS_DEFENSE_ATTACK(attacktype)) {

					// ch is initiating an attack ?
					// only if ch is not "attacking" with a fireshield/energy shield/etc...
					if (!FIGHTING(ch)) {

						// mages casting spells and shooters should be able to attack
						// without initiating melee ( on their part at least )
						if (attacktype != SKILL_ENERGY_WEAPONS &&
							attacktype != SKILL_PROJ_WEAPONS &&
							ch->in_room == victim->in_room &&
							(!IS_MAGE(ch) || attacktype > MAX_SPELLS ||
								!SPELL_IS_MAGIC(attacktype))) {
							set_fighting(ch, victim, TRUE);
						}

					}
					// add ch to victim's shitlist( s )
					if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
						if (MOB_FLAGGED(victim, MOB_MEMORY))
							remember(victim, ch);
						if (MOB2_FLAGGED(victim, MOB2_HUNT))
							HUNTING(victim) = ch;
					}
					// make the victim retailiate against the attacker
					if (!FIGHTING(victim) && ch->in_room == victim->in_room) {
						set_fighting(victim, ch, FALSE);
					}
				}
			}
			break;
		}
	}							/* end switch ( victim->getPosition() ) */


	// Victim is Linkdead
	// Send him to the void

	if (!IS_NPC(victim) && !(victim->desc)
		&& !is_arena_combat(ch, victim)) {

		act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
		GET_WAS_IN(victim) = victim->in_room;
		if (ch) {
			stop_fighting(victim);
			stop_fighting(ch);
		}
		char_from_room(victim,false);
		char_to_room(victim, zone_table->world,false);
		act("$n is carried in by divine forces.", FALSE, victim, 0, 0,
			TO_ROOM);
	}

	/* debugging message */
	if (ch && PRF2_FLAGGED(ch, PRF2_DEBUG))
		send_to_char(ch,
			"%s[DAMAGE] %s   dam:%d   wait:%d   pos:%d   reduct:%.2f%s\r\n",
			CCCYN(ch, C_NRM), GET_NAME(victim), dam,
			IS_NPC(victim) ? GET_MOB_WAIT(victim) :
				victim->desc ? victim->desc->wait : 0,
			victim->getPosition(), dam_reduction, CCNRM(ch, C_NRM));

	if (victim && PRF2_FLAGGED(victim, PRF2_DEBUG))
		send_to_char(victim,
			"%s[DAMAGE] %s   dam:%d   wait:%d   pos:%d   reduct:%.2f%s\r\n",
			CCCYN(victim, C_NRM), GET_NAME(victim), dam,
			IS_NPC(victim) ? GET_MOB_WAIT(victim) :
				victim->desc ? victim->desc->wait : 0,
			victim->getPosition(), dam_reduction, CCNRM(victim, C_NRM));
	//
	// If victim is asleep, incapacitated, etc.. stop fighting.
	//

	if (!AWAKE(victim) && FIGHTING(victim))
		stop_fighting(victim);

	//
	// Victim has been slain, handle all the implications
	// Exp, Kill counter, etc...
	//

	if (victim->getPosition() == POS_DEAD) {
		bool arena = is_arena_combat(ch, victim);

		if (ch) {
			if (attacktype != SKILL_SNIPE) {
				gain_kill_exp(ch, victim);
			}

			if (!IS_NPC(victim)) {
				// Log the death
				if (victim != ch) {
					char *room_str;

					if (victim->in_room)
						room_str = tmp_sprintf("in room #%d (%s)",
							victim->in_room->number, victim->in_room->name);
					else
						room_str = "in NULL room!";
					if (IS_NPC(ch)) {
						sprintf(buf2, "%s killed by %s %s%s",
							GET_NAME(victim), GET_NAME(ch),
							affected_by_spell(ch, SPELL_QUAD_DAMAGE) ? "(quad)":"",
							room_str);
					} else {
						sprintf(buf2, "%s(%d:%d) pkilled by %s(%d:%d) %s",
							GET_NAME(victim), GET_LEVEL(victim),
							GET_REMORT_GEN(victim),
							GET_NAME(ch), GET_LEVEL(ch), GET_REMORT_GEN(ch),
							room_str);
					}

					// If it's not arena, give em a pkill and adjust reputation
					if (!arena) {
						GET_PKILLS(ch) += 1;
						count_pkill(ch, victim);
					} else {
						// Else adjust arena kills
						GET_ARENAKILLS(ch) += 1;
					}

				} else {
					const char *attack_desc;

					if (attacktype <= TOP_NPC_SPELL)
						attack_desc = spell_to_str(attacktype);
					else {
						switch (attacktype) {
						case TYPE_TAINT_BURN:
							attack_desc = "taint burn"; break;
						case TYPE_PRESSURE:
							attack_desc = "pressurization"; break;
						case TYPE_SUFFOCATING:
							attack_desc = "suffocation"; break;
						case TYPE_ANGUISH:
							attack_desc = "soulless anguish"; break;
						case TYPE_BLEED:
							attack_desc = "hamstring bleeding"; break;
						case TYPE_OVERLOAD:
							attack_desc = "cybernetic overload"; break;
						case TYPE_SUFFERING:
							attack_desc = "blood loss"; break;
						default:
							attack_desc = NULL; break;
						}
					}
					if (attack_desc)
						sprintf(buf2, "%s died by %s in room #%d (%s}",
							GET_NAME(ch), attack_desc, ch->in_room->number,
							ch->in_room->name);
					else
						sprintf(buf2, "%s died in room #%d (%s}",
							GET_NAME(ch), ch->in_room->number,
							ch->in_room->name);
				}

				// If it's arena, log it for complete only
				// and tag it
				if (arena) {
					strcat(buf2, " [ARENA]");
					//mudlog(buf2, CMP, GET_INVIS_LVL(victim), TRUE);
					qlog(NULL, buf2, QLOG_COMP, GET_INVIS_LVL(victim), TRUE);
				} else {
					mudlog(GET_INVIS_LVL(victim), BRF, true, "%s", buf2);
				}
				if (MOB_FLAGGED(ch, MOB_MEMORY))
					forget(ch, victim);
				if (!IS_NPC(ch) && ch != victim)
					is_humil = TRUE;
			} else {
				GET_MOBKILLS(ch) += 1;
			}

			if (HUNTING(ch) && HUNTING(ch) == victim)
				HUNTING(ch) = NULL;
			die(victim, ch, attacktype, is_humil);
			DAM_RETURN(DAM_VICT_KILLED);

		}

		if (!IS_NPC(victim)) {
			mudlog(LVL_AMBASSADOR, BRF, true,
				"%s killed by NULL-char (type %d [%s]) in room #%d (%s)",
				GET_NAME(victim), attacktype,
				(attacktype > 0 && attacktype < TOP_NPC_SPELL) ?
				spell_to_str(attacktype) :
				(attacktype >= TYPE_HIT && attacktype <= TOP_ATTACKTYPE) ?
				attack_type[attacktype - TYPE_HIT] : "bunk",
				victim->in_room->number, victim->in_room->name);
		}
		die(victim, NULL, attacktype, is_humil);
		DAM_RETURN(DAM_VICT_KILLED);
	}
	//
	// end if victim->getPosition() == POS_DEAD
	//

	DAM_RETURN(0);
}

#undef DAM_RETURN

// Pick the next weapon that the creature will strike with
obj_data *
get_next_weap(struct Creature *ch)
{
	obj_data *cur_weap;
	int dual_prob = 0;

	if (GET_EQ(ch, WEAR_WIELD_2) && GET_EQ(ch, WEAR_WIELD)) {
		dual_prob = (GET_EQ(ch, WEAR_WIELD)->getWeight() -
			GET_EQ(ch, WEAR_WIELD_2)->getWeight()) * 2;
	}

	// Check dual wield
	cur_weap = GET_EQ(ch, WEAR_WIELD_2);
	if (cur_weap && IS_OBJ_TYPE(cur_weap, ITEM_WEAPON)) {
		if (affected_by_spell(ch, SKILL_NEURAL_BRIDGING)) {
			if ((CHECK_SKILL(ch, SKILL_NEURAL_BRIDGING) * 2 / 3) + dual_prob > number(50, 150))
				return cur_weap;
		} else {
			if ((CHECK_SKILL(ch, SKILL_SECOND_WEAPON) * 2 / 3) + dual_prob > number(50, 150))
				return cur_weap;
		}
	}

	// Check normal wield
	cur_weap = GET_EQ(ch, WEAR_WIELD);
	if (cur_weap && !number(0, 1))
		return cur_weap;

	// Check equipment on hands
	cur_weap = GET_EQ(ch, WEAR_HANDS);
	if (cur_weap && IS_OBJ_TYPE(cur_weap, ITEM_WEAPON) && !number(0, 5))
		return cur_weap;

	// Check for implanted weapons in hands
	cur_weap = GET_IMPLANT(ch, WEAR_HANDS);
	if (cur_weap && !GET_EQ(ch, WEAR_HANDS) &&
			IS_OBJ_TYPE(cur_weap, ITEM_WEAPON) && !number(0, 2))
		return cur_weap;

	// Check for weapon on feet
	cur_weap = GET_EQ(ch, WEAR_FEET);
	if (cur_weap && IS_OBJ_TYPE(cur_weap, ITEM_WEAPON) &&
			CHECK_SKILL(ch, SKILL_KICK) > number(10, 170))
		return cur_weap;

	// Check equipment on head
	cur_weap = GET_EQ(ch, WEAR_HEAD);
	if (cur_weap && IS_OBJ_TYPE(cur_weap, ITEM_WEAPON) && !number(0, 2))
		return cur_weap;
	

	cur_weap = GET_EQ(ch, WEAR_WIELD);
	if (cur_weap)
		return cur_weap;

	return GET_EQ(ch, WEAR_HANDS);
}

// Damage multiplier for the backstab skill.
// 7x max for levels.
// 6x max more for gens based on getLevelBonus.
static inline int BACKSTAB_MULT( Creature *ch  ) {
	int mult = 2 + ( ch->getLevel() + 1 )/10;
	int bonus = MAX(0,ch->getLevelBonus(SKILL_BACKSTAB) - 50);
	mult += ( 6 * bonus ) / 50;
	return mult;
}


//
// generic hit
//
// return values are same as damage()
//

int
hit(struct Creature *ch, struct Creature *victim, int type)
{

	int w_type = 0, victim_ac, calc_thaco, dam, tmp_dam, diceroll, skill = 0;
	int i, metal_wt;
	byte limb;
	struct obj_data *weap = NULL;
	int retval;

	if (ch->in_room != victim->in_room) {
		if (FIGHTING(ch) && FIGHTING(ch) == victim)
			stop_fighting(ch);
		return 0;
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) &&
		!(PLR_FLAGGED(ch, PLR_KILLER) && FIGHTING(victim) == ch) &&
		!PLR_FLAGGED(victim, PLR_KILLER) && GET_LEVEL(ch) < LVL_CREATOR) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
		return 0;
	}
	if (LVL_AMBASSADOR <= GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_GOD &&
		IS_NPC(victim) && !mini_mud) {
		send_to_char(ch, "You are not allowed to attack mobiles!\r\n");
		return 0;
	}
	if (IS_AFFECTED_2(ch, AFF2_PETRIFIED) && GET_LEVEL(ch) < LVL_ELEMENT) {
		if (!number(0, 2))
			act("You want to fight back against $N's attack, but cannot!",
				FALSE, ch, 0, victim, TO_CHAR | TO_SLEEP);
		else if (!number(0, 1))
			send_to_char(ch, 
				"You have been turned to stone, and cannot fight!\r\n");
		else
			send_to_char(ch, "You cannot fight back!!  You are petrified!\r\n");

		return 0;
	}

	if (victim->isNewbie() && !IS_NPC(ch) && !IS_NPC(victim) &&
		!is_arena_combat(ch, victim) &&
		GET_LEVEL(ch) < LVL_IMMORT) {
		act("$N is currently under new character protection.",
			FALSE, ch, 0, victim, TO_CHAR);
		act("You are protected by the gods against $n's attack!",
			FALSE, ch, 0, victim, TO_VICT);
		slog("%s protected against %s ( hit ) at %d\n",
			GET_NAME(victim), GET_NAME(ch), victim->in_room->number);

		if (victim == FIGHTING(ch))
			stop_fighting(ch);
		if (ch == FIGHTING(victim))
			stop_fighting(victim);
		return 0;
	}

	if (MOUNTED(ch)) {
		if (MOUNTED(ch)->in_room != ch->in_room) {
			REMOVE_BIT(AFF2_FLAGS(MOUNTED(ch)), AFF2_MOUNTED);
			MOUNTED(ch) = NULL;
		} else
			send_to_char(ch, "You had better dismount first.\r\n");
		return 0;
	}
	if (MOUNTED(victim)) {
		REMOVE_BIT(AFF2_FLAGS(MOUNTED(victim)), AFF2_MOUNTED);
		MOUNTED(victim) = NULL;
		act("You are knocked from your mount by $N's attack!",
			FALSE, victim, 0, ch, TO_CHAR);
	}
	if (IS_AFFECTED_2(victim, AFF2_MOUNTED)) {
		REMOVE_BIT(AFF2_FLAGS(victim), AFF2_MOUNTED);
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (MOUNTED((*it)) && MOUNTED((*it)) == victim) {
				act("You are knocked from your mount by $N's attack!",
					FALSE, (*it), 0, ch, TO_CHAR);
				MOUNTED((*it)) = NULL;
				(*it)->setPosition(POS_STANDING);
			}
		}
	}

	for (i = 0, metal_wt = 0; i < NUM_WEARS; i++)
		if (ch->equipment[i] &&
			GET_OBJ_TYPE(ch->equipment[i]) == ITEM_ARMOR &&
			(IS_METAL_TYPE(ch->equipment[i]) ||
				IS_STONE_TYPE(ch->equipment[i])))
			metal_wt += ch->equipment[i]->getWeight();

//	if( type == SKILL_CLEAVE ) {
//		cur_weap = GET_EQ(ch, WEAR_WIELD);
//	} else 
	if ((type != SKILL_BACKSTAB && type != SKILL_CIRCLE &&
			type != SKILL_BEHEAD && type != SKILL_CLEAVE) || !cur_weap) {
		if (type == SKILL_IMPLANT_W || type == SKILL_ADV_IMPLANT_W)
			cur_weap = get_random_uncovered_implant(ch, ITEM_WEAPON);
		else
			cur_weap = get_next_weap(ch);
		if (cur_weap) {
			if (IS_ENERGY_GUN(cur_weap) || IS_GUN(cur_weap)) {
				w_type = TYPE_BLUDGEON;
			} else if (IS_OBJ_TYPE(cur_weap, ITEM_WEAPON))
				w_type = GET_OBJ_VAL(cur_weap, 3) + TYPE_HIT;
			else
				cur_weap = NULL;
		}
	}

	if (!cur_weap) {
		if (IS_NPC(ch) && (ch->mob_specials.shared->attack_type != 0))
			w_type = ch->mob_specials.shared->attack_type + TYPE_HIT;
		else if (IS_TABAXI(ch) && number(0, 1))
			w_type = TYPE_CLAW;
		else
			w_type = TYPE_HIT;
	}

	weap = cur_weap;
	calc_thaco = calculate_thaco(ch, victim, weap);

	victim_ac = MAX(GET_AC(victim), -(GET_LEVEL(ch) << 2)) / 10;

	diceroll = number(1, 20);

	if (PRF2_FLAGGED(ch, PRF2_DEBUG))
		send_to_char(ch, "%s[HIT] thac0:%d   roll:%d  AC:%d%s\r\n",
			CCCYN(ch, C_NRM), calc_thaco, diceroll, victim_ac,
			CCNRM(ch, C_NRM));

	/* decide whether this is a hit or a miss */
	if (((diceroll < 20) && AWAKE(victim) &&
			((diceroll == 1) || ((calc_thaco - diceroll)) > victim_ac))) {

		if (type == SKILL_BACKSTAB || type == SKILL_CIRCLE ||
			type == SKILL_SECOND_WEAPON || type == SKILL_CLEAVE )
			return (damage(ch, victim, 0, type, -1));
		else
			return (damage(ch, victim, 0, w_type, -1));
	}

	/* IT's A HIT! */

	if (CHECK_SKILL(ch, SKILL_DBL_ATTACK) > 60)
		gain_skill_prof(ch, SKILL_DBL_ATTACK);
	if (CHECK_SKILL(ch, SKILL_TRIPLE_ATTACK) > 60)
		gain_skill_prof(ch, SKILL_TRIPLE_ATTACK);
	if (affected_by_spell(ch, SKILL_NEURAL_BRIDGING)
		&& CHECK_SKILL(ch, SKILL_NEURAL_BRIDGING) > 60)
		gain_skill_prof(ch, SKILL_NEURAL_BRIDGING);

	/* okay, it's a hit.  calculate limb */

	limb = choose_random_limb(victim);

	if (type == SKILL_BACKSTAB || type == SKILL_CIRCLE)
		limb = WEAR_BACK;
	else if (type == SKILL_IMPALE || type == SKILL_LUNGE_PUNCH)
		limb = WEAR_BODY;
	else if (type == SKILL_BEHEAD)
		limb = WEAR_NECK_1;

	/* okay, we know the guy has been hit.  now calculate damage. */
	dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
	dam += GET_DAMROLL(ch);
	tmp_dam = dam;

	if (cur_weap) {
		if (IS_OBJ_TYPE(cur_weap, ITEM_WEAPON)) {
			dam += dice(GET_OBJ_VAL(cur_weap, 1), GET_OBJ_VAL(cur_weap, 2));
			if (invalid_char_class(ch, cur_weap))
				dam >>= 1;
			if (IS_NPC(ch)) {
				tmp_dam += dice(ch->mob_specials.shared->damnodice,
					ch->mob_specials.shared->damsizedice);
				dam = MAX(tmp_dam, dam);
			}
			if (GET_OBJ_VNUM(cur_weap) > 0) {
				for (i = 0; i < MAX_WEAPON_SPEC; i++)
					if (GET_WEAP_SPEC(ch, i).vnum == GET_OBJ_VNUM(cur_weap)) {
						dam += GET_WEAP_SPEC(ch, i).level;
						break;
					}
			}
			if (IS_TWO_HAND(cur_weap) && IS_BARB(ch) && 
                cur_weap->worn_on == WEAR_WIELD) 
            {
				int dam_add;
				dam_add = cur_weap->getWeight() / 4;
				if (CHECK_SKILL(ch, SKILL_DISCIPLINE_OF_STEEL) > 60) {
					int bonus = ch->getLevelBonus( SKILL_DISCIPLINE_OF_STEEL );
					int weight = cur_weap->getWeight();
					dam_add += ( bonus * weight ) / 100;
				}
				dam += dam_add;
			}
              
            if( type == SKILL_CLEAVE ) {
                dam *= 4;
				w_type = SKILL_CLEAVE;
				//if( IS_PC(ch) )
				//	fprintf(stderr, "CLEAVE %d\r\n", dam );
            } 
			//else {
			//	if( IS_PC(ch) )
			//		fprintf(stderr, "NORMAL %d\r\n", dam );
			//}
		} else if (IS_OBJ_TYPE(cur_weap, ITEM_ARMOR)) {
			dam += (GET_OBJ_VAL(cur_weap, 0) / 3);
		} else
			dam += dice(1, 4) + number(0, cur_weap->getWeight());
	} else if (IS_NPC(ch)) {
		tmp_dam += dice(ch->mob_specials.shared->damnodice,
			ch->mob_specials.shared->damsizedice);
		dam = MAX(tmp_dam, dam);
	} else {
		dam += number(0, 3);	/* Max. 3 dam with bare hands */
		if (IS_MONK(ch) || IS_VAMPIRE(ch))
			dam += number((GET_LEVEL(ch) >> 2), GET_LEVEL(ch));
		else if (IS_BARB(ch))
			dam += number(0, (GET_LEVEL(ch) >> 1));
	}

	dam = MAX(1, dam);			/* at least 1 hp damage min per hit */
	dam = MIN(dam, 30 + (GET_LEVEL(ch) << 3));	/* level limit */
  

	if (type == SKILL_BACKSTAB) {
		gain_skill_prof(ch, type);
		if (IS_THIEF(ch)) {
			dam *= BACKSTAB_MULT(ch);
			dam +=
				number(0, (CHECK_SKILL(ch,
						SKILL_BACKSTAB) - LEARNED(ch)) >> 1);
		}

		retval = damage(ch, victim, dam, SKILL_BACKSTAB, WEAR_BACK);

		if (retval) {
			return retval;
		}
	} else if (type == SKILL_CIRCLE) {
		if (IS_THIEF(ch)) {
			dam *= MAX(2, (BACKSTAB_MULT(ch) >> 1));
			dam +=
				number(0, (CHECK_SKILL(ch, SKILL_CIRCLE) - LEARNED(ch)) >> 1);
		}
		gain_skill_prof(ch, type);

		retval = damage(ch, victim, dam, SKILL_CIRCLE, WEAR_BACK);

		if (retval) {
			return retval;
		}

	} else {
		if (cur_weap && IS_OBJ_TYPE(cur_weap, ITEM_WEAPON) &&
			GET_OBJ_VAL(cur_weap, 3) >= 0 &&
			GET_OBJ_VAL(cur_weap, 3) < TOP_ATTACKTYPE - TYPE_HIT &&
			IS_MONK(ch)) {
			skill = weapon_proficiencies[GET_OBJ_VAL(cur_weap, 3)];
			if (skill)
				gain_skill_prof(ch, skill);
		}
		retval = damage(ch, victim, dam, w_type, limb);

		if (retval) {
			return retval;
		}

		if (weap && IS_FERROUS(weap) && IS_NPC(victim)
			&& GET_MOB_SPEC(victim) == rust_monster) {

			if ((!IS_OBJ_STAT(weap, ITEM_MAGIC) ||
					mag_savingthrow(ch, GET_LEVEL(victim), SAVING_ROD)) &&
				(!IS_OBJ_STAT(weap, ITEM_MAGIC_NODISPEL) ||
					mag_savingthrow(ch, GET_LEVEL(victim), SAVING_ROD))) {

				act("$p spontaneously oxidizes and crumbles into a pile of rust!", FALSE, ch, weap, 0, TO_CHAR);
				act("$p crumbles into rust on contact with $N!",
					FALSE, ch, weap, victim, TO_ROOM);

				extract_obj(weap);

				if ((weap = read_object(RUSTPILE)))
					obj_to_room(weap, ch->in_room);
				return 0;
			}
		}

		if (weap && IS_OBJ_TYPE(weap, ITEM_WEAPON) &&
			weap == GET_EQ(ch, weap->worn_on)) {
			if (GET_OBJ_SPEC(weap)) {
				GET_OBJ_SPEC(weap) (ch, weap, 0, NULL, SPECIAL_COMBAT);
			} else if (IS_OBJ_STAT2(weap, ITEM2_CAST_WEAPON))
				retval = do_casting_weapon(ch, weap);
				if (retval)
					return retval;
		}
	}
	
	if (!IS_NPC(ch) && GET_MOVE(ch) > 20) {
		GET_MOVE(ch)--;
		if (IS_DROW(ch) && OUTSIDE(ch) && PRIME_MATERIAL_ROOM(ch->in_room) &&
			ch->in_room->zone->weather->sunlight == SUN_LIGHT)
			GET_MOVE(ch)--;
		if (IS_CYBORG(ch) && GET_BROKE(ch))
			GET_MOVE(ch)--;
	}

	return 0;
}

int
do_casting_weapon(Creature *ch, obj_data *weap)
{
	obj_data *weap2;

	if (GET_OBJ_VAL(weap, 0) < 0 || GET_OBJ_VAL(weap, 0) > MAX_SPELLS) {
		slog("Invalid spell number detected on weapon %d", GET_OBJ_VNUM(weap));
		return 0;
	}

	if ((SPELL_IS_MAGIC(GET_OBJ_VAL(weap, 0)) ||
			IS_OBJ_STAT(weap, ITEM_MAGIC)) &&
			ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
		return 0;
	if (SPELL_IS_PHYSICS(GET_OBJ_VAL(weap, 0)) &&
			ROOM_FLAGGED(ch->in_room, ROOM_NOSCIENCE))
		return 0;
	if (SPELL_IS_PSIONIC(GET_OBJ_VAL(weap, 0)) &&
			ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS))
		return 0;
	if (number(0, MAX(2, LVL_GRIMP + 28 - GET_LEVEL(ch) - GET_INT(ch) -
			(CHECK_SKILL(ch, GET_OBJ_VAL(weap, 0)) >> 3))))
		return 0;
	if (weap->action_desc)
		strcpy(buf, weap->action_desc);
	else
		sprintf(buf, "$p begins to hum and shake%s!",
			weap->worn_on == WEAR_WIELD ||
			weap->worn_on == WEAR_WIELD_2 ? " in your hand" : "");
	sprintf(buf2, "$p begins to hum and shake%s!",
		weap->worn_on == WEAR_WIELD ||
		weap->worn_on == WEAR_WIELD_2 ? " in $n's hand" : "");
	send_to_char(ch, CCCYN(ch, C_NRM));
	act(buf, FALSE, ch, weap, 0, TO_CHAR);
	send_to_char(ch, CCNRM(ch, C_NRM));
	act(buf2, TRUE, ch, weap, 0, TO_ROOM);
	if ((((!IS_DWARF(ch) && !IS_CYBORG(ch)) ||
				!IS_OBJ_STAT(weap, ITEM_MAGIC) ||
				!SPELL_IS_MAGIC(GET_OBJ_VAL(weap, 0))) &&
			number(0, GET_INT(ch) + 3)) || GET_LEVEL(ch) > LVL_GRGOD) {
		if (IS_SET(spell_info[GET_OBJ_VAL(weap, 0)].routines, MAG_DAMAGE) ||
				spell_info[GET_OBJ_VAL(weap, 0)].violent ||
				IS_SET(spell_info[GET_OBJ_VAL(weap, 0)].targets,
					TAR_UNPLEASANT))
			call_magic(ch, FIGHTING(ch), 0, GET_OBJ_VAL(weap, 0),
				GET_LEVEL(ch), CAST_WAND);
		else if (!affected_by_spell(ch, GET_OBJ_VAL(weap, 0)))
			call_magic(ch, ch, 0, GET_OBJ_VAL(weap, 0), GET_LEVEL(ch),
				CAST_WAND);
	} else {
		// drop the weapon
		if ((weap->worn_on == WEAR_WIELD ||
				weap->worn_on == WEAR_WIELD_2) &&
			GET_EQ(ch, weap->worn_on) == weap) {
			act("$p shocks you!  You are forced to drop it!",
				FALSE, ch, weap, 0, TO_CHAR);

			// weapon is the 1st of 2 wielded
			if (weap->worn_on == WEAR_WIELD &&
				GET_EQ(ch, WEAR_WIELD_2)) {
				weap2 = unequip_char(ch, WEAR_WIELD_2, MODE_EQ);
				obj_to_room(unequip_char(ch, weap->worn_on, MODE_EQ),
					ch->in_room);
				if (equip_char(ch, weap2, WEAR_WIELD, MODE_EQ))
					return DAM_ATTACKER_KILLED;
			}
			// weapon should fall to ground
			else if (number(0, 20) > GET_DEX(ch))
				obj_to_room(unequip_char(ch, weap->worn_on, MODE_EQ),
					ch->in_room);
			// weapon should drop to inventory
			else
				obj_to_char(unequip_char(ch, weap->worn_on, MODE_EQ),
					ch);

		}
		// just shock the victim
		else {
			act("$p blasts the hell out of you!  OUCH!!",
				FALSE, ch, weap, 0, TO_CHAR);
			GET_HIT(ch) -= dice(3, 4);
		}
		act("$n cries out as $p shocks $m!", FALSE, ch, weap, 0,
			TO_ROOM);
	}
	return 0;
}

/* control the fights.  Called every 0.SEG_VIOLENCE sec from comm.c. */
void
perform_violence(void)
{

	register struct Creature *ch;
	int prob, i, die_roll;

	CreatureList::iterator cit = combatList.begin();
	for (; cit != combatList.end(); ++cit) {
		ch = *cit;
		if (!ch->in_room || !FIGHTING(ch))
			continue;
		if (ch == FIGHTING(ch)) {	// intentional crash here.
			slog("SYSERR: ch == FIGHTING( ch ) in perform_violence.");
			raise(SIGSEGV);
		}

		if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room ||
			AFF2_FLAGGED(ch, AFF2_PETRIFIED) ||
			(IS_NPC(ch) && ch->in_room->zone->idle_time >= ZONE_IDLE_TIME)) {
			stop_fighting(ch);
			continue;
		}

		if (IS_NPC(ch)) {
			if (GET_MOB_WAIT(ch) > 0) {
				GET_MOB_WAIT(ch) = MAX(GET_MOB_WAIT(ch) - SEG_VIOLENCE, 0);
			} else if (GET_MOB_WAIT(ch) == 0) {
				update_pos(ch);
			}
			if (ch->getPosition() <= POS_SITTING)
				continue;
		}
		// Make sure they're fighting before they fight.
		if (ch->getPosition() == POS_STANDING
			|| ch->getPosition() == POS_FLYING) {
			ch->setPosition(POS_FIGHTING);
			continue;
		}

        // Moved all attack prob stuff to this function.
        // STOP THE INSANITY! --N
        prob = calculate_attack_probability(ch);

		die_roll = number(0, 300);

		if (PRF2_FLAGGED(ch, PRF2_DEBUG)) {
			send_to_char(ch,
				"%s[COMBAT] %s   prob:%d   roll:%d   wait:%d%s\r\n",
				CCCYN(ch, C_NRM), GET_NAME(ch), prob, die_roll,
				IS_NPC(ch) ? GET_MOB_WAIT(ch) : (CHECK_WAIT(ch) ? ch->desc->
					wait : 0),
				CCNRM(ch, C_NRM));
		}
		if (PRF2_FLAGGED(FIGHTING(ch), PRF2_DEBUG)) {
			send_to_char(FIGHTING(ch),
				"%s[COMBAT] %s   prob:%d   roll:%d   wait:%d%s\r\n",
				CCCYN(FIGHTING(ch), C_NRM), GET_NAME(ch), prob, die_roll,
				IS_NPC(ch) ? GET_MOB_WAIT(ch) : (CHECK_WAIT(ch) ? ch->desc->
					wait : 0),
				CCNRM(FIGHTING(ch), C_NRM));
		}
		//
		// it's an attack!
		//
		if (MIN(100, prob + 15) >= die_roll) {

			bool stop = false;
            int retval = -1;

			for (i = 0; i < 4; i++) {
				if (!FIGHTING(ch) || GET_LEVEL(ch) < (i << 3))
					break;
				if (ch->getPosition() < POS_FIGHTING) {
					if (CHECK_WAIT(ch) < 10)
						send_to_char(ch, "You can't fight while sitting!!\r\n");
					break;
				}

				if (prob >= number((i << 4) + (i << 3), (i << 5) + (i << 3))) {
                    // Special combat loop for mercs...
                    if (IS_MERC(ch)) {
                        // Roll the dice to see if the merc gets to shoot his gun this round
                        if (prob > number(1, 101)) {
                            retval = do_combat_fire(ch, FIGHTING(ch));
                            // Either the attacker or the victim was killed
                            if ((retval == 1) || (retval == DAM_VICT_KILLED)) {
                                stop = true;
                                break;
                            }
                            // the merc passed his test but was unable to fire for some reason.
                            // Whack them with it instead.
                            else if (retval == -1) {
                                if (hit(ch, FIGHTING(ch), TYPE_UNDEFINED)) {
                                    stop = true;
                                    break;
                                }
                            }
                        }
                        // The merc was too slow.  Can't shoot his gun this round
                        else {
                            if (hit(ch, FIGHTING(ch), TYPE_UNDEFINED)) {
                                stop = true;
                                break;
                            }
                        }
                    }
                    // Everyone elses combat loop
                    else {
                        if (hit(ch, FIGHTING(ch), TYPE_UNDEFINED)) {
                            stop = true;
                            break;
                        }
                    }
				}
			}

			if (stop)
				continue;

			if (IS_CYBORG(ch)) {
				int implant_prob;

				if (!FIGHTING(ch))
					continue;

				if (ch->getPosition() < POS_FIGHTING) {
					if (CHECK_WAIT(ch) < 10)
						send_to_char(ch, "You can't fight while sitting!!\r\n");
					continue;
				}

				if (number(1, 100) < CHECK_SKILL(ch, SKILL_ADV_IMPLANT_W)) {

					implant_prob = 25;

					if (CHECK_SKILL(ch, SKILL_ADV_IMPLANT_W) > 100) {
						implant_prob +=
							GET_REMORT_GEN(ch) + (CHECK_SKILL(ch,
								SKILL_ADV_IMPLANT_W) - 100) / 2;
					}

					if (number(0, 100) < implant_prob) {
						int retval =
							hit(ch, FIGHTING(ch), SKILL_ADV_IMPLANT_W);
						/*  No longer needed because of CombatList
						   if ( IS_SET( retval, DAM_VICT_KILLED ) ) {
						   next_combat_list = tmp_next_combat_list;
						   }
						 */
						if (retval)
							continue;
					}
				}

				if (!FIGHTING(ch))
					continue;

				if (IS_NPC(ch) && (GET_REMORT_CLASS(ch) == CLASS_UNDEFINED
						|| GET_CLASS(ch) != CLASS_CYBORG))
					continue;

				if (number(1, 100) < CHECK_SKILL(ch, SKILL_IMPLANT_W)) {

					implant_prob = 25;

					if (CHECK_SKILL(ch, SKILL_IMPLANT_W) > 100) {
						implant_prob +=
							GET_REMORT_GEN(ch) + (CHECK_SKILL(ch,
								SKILL_IMPLANT_W) - 100) / 2;
					}

					if (number(0, 100) < implant_prob) {
						int retval = hit(ch, FIGHTING(ch), SKILL_IMPLANT_W);
						/*
						   if ( IS_SET( retval, DAM_VICT_KILLED ) ) {
						   next_combat_list = tmp_next_combat_list;
						   } */

						if (retval)
							continue;
					}
				}
			}

			continue;
		}

		else if (IS_NPC(ch) && ch->in_room &&
			ch->getPosition() == POS_FIGHTING &&
			GET_MOB_WAIT(ch) <= 0 && (MIN(100, prob) >= number(0, 300))) {

			//
			// if the opponent is the next_combat_list, calling
			// mobile_battle_activity with the next_combat_list as
			// precious will result in zero activity, and we cannot
			// really trust the spec proc to do the right thing.
			// so just throw a hit instead.  this happens
			// rarely enough that nobody should notice
			//

			if (FIGHTING(ch) == next_combat_list) {
				//int retval = 
				hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
				/*
				   if ( IS_SET( retval, DAM_VICT_KILLED ) )
				   next_combat_list = tmp_next_combat_list;
				 */
				continue;
			}


			if (MOB_FLAGGED(ch, MOB_SPEC) && ch->in_room &&
				ch->mob_specials.shared->func && !number(0, 2)) {

				//
				// TODO: make spec procs handle killing chars in a safe manner
				// TODO: it is still possible for the spec_proc to kill the char
				// which next_combat_list points to.  spec_procs need an optional
				// precious_vict pointer next
				//

				(ch->mob_specials.shared->func) (ch, ch, 0, "", SPECIAL_TICK);

				continue;
			}

			if (ch->in_room && GET_MOB_WAIT(ch) <= 0 && FIGHTING(ch)) {

				//
				// call mobile_battle_activity, but don't touch next_combat_list
				//

				mobile_battle_activity(ch, next_combat_list);

			}
		}
	}
}

#undef __combat_code__
