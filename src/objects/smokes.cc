//
// File: smokes.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define __smokes_c__


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "vehicle.h"
#include "materials.h"
#include "smokes.h"
#include "clan.h"
#include "char_class.h"
#include "bomb.h"

struct obj_data *
roll_joint(struct obj_data *tobac, struct obj_data *paper)
{
	struct obj_data *obj;
	struct extra_descr_data *new_descr;
	char buf[200];

	if (!tobac || !paper) {
		slog("SYSERR:  Attempt to roll_joint with NULL tobac or paper.");
		return NULL;
	}
	obj = create_obj();
	obj->shared = null_obj_shared;
	CREATE(new_descr, struct extra_descr_data, 1);

	sprintf(buf, "cig cigarette joint %s", fname(tobac->name));
	obj->name = str_dup(buf);
	sprintf(buf, "a %s cigarette", fname(tobac->name));
	obj->short_description = str_dup(buf);
	sprintf(buf, "%s has been dropped here.", buf);
	obj->description = str_dup(buf);
	new_descr->keyword = str_dup("cig cigarette joint");
	sprintf(buf, "It looks like a %s cigarette, waiting to be smoked.",
		fname(tobac->name));
	new_descr->description = str_dup(buf);
	new_descr->next = NULL;
	obj->ex_description = new_descr;

	GET_OBJ_TYPE(obj) = ITEM_CIGARETTE;
	GET_OBJ_VAL(obj, 0) = 3 + (tobac->getWeight() * 3);
	GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(tobac, 2);
	GET_OBJ_VAL(obj, 2) = SMOKE_TYPE(tobac);
	GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE + ITEM_WEAR_HOLD;
	GET_OBJ_COST(obj) = GET_OBJ_COST(tobac);

	return obj;
}

ACMD(do_roll)
{
	struct obj_data *roll_joint(struct obj_data *tobac,
		struct obj_data *paper);
	struct obj_data *tobac, *paper, *joint;
	struct Creature *vict;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char(ch, "You roll your eyes in disgust.\r\n");
		act("$n rolls $s eyes in disgust.", TRUE, ch, 0, 0, TO_ROOM);
	} else if (!(tobac = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		if ((vict = get_char_room_vis(ch, arg1))) {
			act("You roll your eyes at $M.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n rolls $s eyes at $N.", TRUE, ch, 0, vict, TO_NOTVICT);
			act("$n rolls $s eyes at you.", TRUE, ch, 0, vict, TO_VICT);
		} else {
			send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
		}
	} else if (GET_OBJ_TYPE(tobac) != ITEM_TOBACCO)
		send_to_char(ch, "That's not smoking material!\r\n");
	else if (!*arg2)
		act("What would you like to roll $p in?", FALSE, ch, tobac, 0,
			TO_CHAR);
	else if (!(paper = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
		send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
	} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
		send_to_char(ch, "You can't roll %s in %s!\r\n", tobac->short_description,
			paper->short_description);
	} else if (tobac->getWeight() > (paper->getWeight() + 3)) {
		act("$p is too big to fit in $P.", FALSE, ch, tobac, paper, TO_CHAR);
	} else {
		send_to_char(ch, "You roll up %s in %s.\r\n", tobac->short_description,
			paper->short_description);
		act("$n rolls a cigarette with $p.", TRUE, ch, tobac, 0, TO_ROOM);
		joint = roll_joint(tobac, paper);
		if (!joint) {
			send_to_char(ch, "JOINT ERROR!\r\n");
			return;
		}
		obj_to_char(joint, ch);
		extract_obj(tobac);
		extract_obj(paper);
	}
	return;
}

void
perform_smoke(struct Creature *ch, int type)
{

	char *to_vict = NULL;
	struct affected_type af;
	int hp_mod = 0, mana_mod = 0, move_mod = 0, spell = 0;
	ubyte lev = 0;
	int accum_dur = 0, accum_affect = 0;

	af.type = SMOKE_EFFECTS;
	af.bitvector = 0;
	af.duration = 0;
	af.location = APPLY_NONE;
	af.level = 30;

	switch (type) {
	case SMOKE_DIRTWEED:
		to_vict = "Your head hurts.";
		af.duration = 3;
		af.modifier = -2;
		af.location = APPLY_INT;
		af.bitvector = AFF_CONFUSION;
		af.aff_index = 1;
		accum_dur = 1;
		accum_affect = 1;
		move_mod = -number(3, 10);
		mana_mod = -number(1, 2);
		break;
	case SMOKE_DESERTWEED:
		af.location = APPLY_MOVE;
		af.duration = 4;
		af.modifier = 10;
		mana_mod = dice(1, 4);
		move_mod = dice(3, 4);
		break;
	case SMOKE_INDICA:
		to_vict = "Your mind is elevated to another plane.";
		af.location = APPLY_WIS;
		af.duration = 5;
		af.modifier = number(1, 2);
		mana_mod = dice(6, 4);
		move_mod = -number(1, 3);
		break;
	case SMOKE_UNHOLY_REEFER:
		af.location = APPLY_WIS;
		af.duration = 5;
		af.modifier = number(0, 2);
		mana_mod = number(1, 4);
		move_mod = -number(1, 3);
		spell = SPELL_ESSENCE_OF_EVIL;
		lev = 20;
		break;
	case SMOKE_MARIJUANA:
		to_vict = "You feel stoned.";
		af.location = APPLY_INT;
		af.duration = 3;
		af.modifier = -1;
		mana_mod = dice(4, 4);
		break;
	case SMOKE_TOBACCO:
		af.location = APPLY_MOVE;
		af.duration = 3;
		af.modifier = -number(10, 20);
		accum_dur = 1;
		mana_mod = dice(3, 3);
		break;
	case SMOKE_HEMLOCK:
		to_vict = "The smoke burns your lungs!";
		af.location = APPLY_HIT;
		af.duration = number(3, 6);
		af.modifier = -number(30, 60);
		accum_affect = 1;
		hp_mod = -number(10, 16);
		mana_mod = -10;
		move_mod = -10;
		spell = SPELL_POISON;
		lev = LVL_AMBASSADOR;
		break;
	case SMOKE_PEYOTE:
		to_vict = "You feel a strange sensation.";
		spell = SPELL_ASTRAL_SPELL;
		lev = LVL_GRIMP;
		af.location = APPLY_MANA;
		af.modifier = number(10, 20);
		af.duration = number(1, 3);
		move_mod = number(6, 12);
		break;
	case SMOKE_HOMEGROWN:
		to_vict = "There's no place like home...";
		spell = SPELL_WORD_OF_RECALL;
		lev = number(30, 60);
		mana_mod = number(20, 40);
		move_mod = number(2, 10);
		break;

	default:
		af.type = 0;
		break;
	}

	if (to_vict)
		act(to_vict, FALSE, ch, 0, 0, TO_CHAR);

	GET_HIT(ch) = MIN(MAX(GET_HIT(ch) + hp_mod, 0), GET_MAX_HIT(ch));
	GET_MANA(ch) = MIN(MAX(GET_MANA(ch) + mana_mod, 0), GET_MAX_MANA(ch));
	GET_MOVE(ch) = MIN(MAX(GET_MOVE(ch) + move_mod, 0), GET_MAX_MOVE(ch));

	if (af.type &&
		(!affected_by_spell(ch, af.type) || accum_affect || accum_dur))
		affect_join(ch, &af, accum_dur, TRUE, accum_affect, TRUE);

	if (spell && lev)
		call_magic(ch, ch, 0, spell, (int)lev, CAST_CHEM);

	WAIT_STATE(ch, 6);

}


ACMD(do_smoke)
{
	char arg1[MAX_INPUT_LENGTH];
	struct obj_data *joint = NULL;
	struct Creature *vict;
	int type;
	one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, "Smoke blows out of your ears.\r\n");
		act("Smoke blows out of $n's ears.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (!(joint = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
		(!GET_EQ(ch, WEAR_HOLD) ||
			!(joint = get_obj_in_list_vis(ch, arg1, GET_EQ(ch, WEAR_HOLD))))) {
		if (!(vict = get_char_room_vis(ch, arg1)))
			send_to_char(ch, "Smoke who or what?\r\n");
		else if (vict == ch)
			send_to_char(ch, "Hmmm... okay.\r\n");
		else {
			act("You blow smoke in $N's face.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n blows smoke in your face.", FALSE, ch, 0, vict, TO_VICT);
			act("$n blows smoke in $N's face.", FALSE, ch, 0, vict,
				TO_NOTVICT);
		}
	} else if (GET_OBJ_TYPE(joint) == ITEM_TOBACCO)
		send_to_char(ch, "You need to roll that up first.\r\n");
	else if (GET_OBJ_TYPE(joint) == ITEM_CIGARETTE) {
		if (GET_OBJ_VAL(joint, 3)) {
			GET_OBJ_VAL(joint, 0) = MAX(0, GET_OBJ_VAL(joint, 0) - 1);
			type = SMOKE_TYPE(joint);
			if (GET_OBJ_VAL(joint, 0)) {
				act("You take a drag on $p.", FALSE, ch, joint, 0, TO_CHAR);
				act("$n takes a drag on $p.", FALSE, ch, joint, 0, TO_ROOM);
			} else {
				act("You burn your finger as you smoke the last of $p.", FALSE,
					ch, joint, 0, TO_CHAR);
				act("$n burns $s finger on $p.", FALSE, ch, joint, 0, TO_ROOM);
				extract_obj(joint);
			}
			perform_smoke(ch, type);
		} else
			send_to_char(ch, "It doesn't seem to be lit.\r\n");
	} else if (GET_OBJ_TYPE(joint) == ITEM_PIPE) {
		if (GET_OBJ_VAL(joint, 3)) {

			GET_OBJ_VAL(joint, 0) = MAX(0, GET_OBJ_VAL(joint, 0) - 1);
			if (GET_OBJ_VAL(joint, 0)) {
				act("You take a drag on $p.", FALSE, ch, joint, 0, TO_CHAR);
				act("$n takes a drag on $p.", FALSE, ch, joint, 0, TO_ROOM);
			} else {
				act("You inhale a burning ash as you finish $p.", FALSE, ch,
					joint, 0, TO_CHAR);
				act("$n coughs harshly after inhaling the comet from $p.",
					FALSE, ch, joint, 0, TO_ROOM);
				GET_OBJ_VAL(joint, 3) = 0;
			}
			perform_smoke(ch, SMOKE_TYPE(joint));
		} else
			send_to_char(ch, "It doesn't seem to be lit.\r\n");

	} else
		send_to_char(ch, "You can't smoke that!\r\n");
	return;
}


ACMD(do_convert)
{

	struct obj_data *obj = NULL, *iobj = NULL, *nobj = NULL;
	int i;

	skip_spaces(&argument);
	if (!*argument)
		send_to_char(ch, "Convert what?\r\n");
	else if (CHECK_SKILL(ch, SKILL_PIPEMAKING) < (number(20,
				70) - GET_INT(ch)))
		send_to_char(ch, "You can't seem to figure out how.\r\n");
	else if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
		!(obj = get_object_in_equip_pos(ch, argument, WEAR_HOLD))) {
		send_to_char(ch, "You can't seem to have %s '%s'.\r\n", AN(argument),
			argument);
	} else if (GET_OBJ_TYPE(obj) == ITEM_FOOD ||
		GET_OBJ_TYPE(obj) == ITEM_TOBACCO ||
		GET_OBJ_TYPE(obj) == ITEM_NOTE ||
		GET_OBJ_TYPE(obj) == ITEM_MONEY ||
		GET_OBJ_TYPE(obj) == ITEM_VEHICLE ||
		GET_OBJ_TYPE(obj) == ITEM_CIGARETTE ||
		GET_OBJ_TYPE(obj) == ITEM_METAL ||
		GET_OBJ_TYPE(obj) == ITEM_VSTONE ||
		GET_OBJ_TYPE(obj) == ITEM_POTION ||
		GET_OBJ_TYPE(obj) == ITEM_KEY ||
		IS_PAPER_TYPE(obj) ||
		IS_CLOTH_TYPE(obj) ||
		IS_VEGETABLE_TYPE(obj) ||
		IS_FLESH_TYPE(obj) ||
		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL(obj, 3)) ||
		IS_OBJ_STAT2(obj, ITEM2_BODY_PART))
		send_to_char(ch, "You cannot convert that!\r\n");
	else if (GET_OBJ_TYPE(obj) == ITEM_PIPE)
		send_to_char(ch, "It's already a pipe.\r\n");
	else {

		GET_OBJ_TYPE(obj) = ITEM_PIPE;
		REMOVE_BIT(obj->obj_flags.wear_flags, -1);
		SET_BIT(obj->obj_flags.wear_flags, ITEM_WEAR_HOLD | ITEM_WEAR_TAKE);
		REMOVE_BIT(obj->obj_flags.extra_flags, -1);
		REMOVE_BIT(obj->obj_flags.extra2_flags, -1);
		REMOVE_BIT(obj->obj_flags.extra3_flags, -1);

		obj->obj_flags.bitvector[0] = 0;
		obj->obj_flags.bitvector[1] = 0;
		obj->obj_flags.bitvector[2] = 0;

		GET_OBJ_VAL(obj, 1) = ((GET_INT(ch) +
				(CHECK_SKILL(ch, SKILL_PIPEMAKING) >> 3) +
				GET_LEVEL(ch)) >> 1);
		GET_OBJ_VAL(obj, 0) = 0;
		GET_OBJ_VAL(obj, 2) = 0;
		GET_OBJ_VAL(obj, 3) = 0;

		for (iobj = obj->contains; iobj; iobj = nobj) {
			nobj = iobj->next_content;
			extract_obj(iobj);
		}

		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			obj->affected[i].location = APPLY_NONE;
			obj->affected[i].modifier = 0;
		}

		act("You skillfully convert $p into a smoking pipe.",
			FALSE, ch, obj, 0, TO_CHAR);
		act("$n skillfully converts $p into a smoking pipe.",
			TRUE, ch, obj, 0, TO_ROOM);

		gain_skill_prof(ch, SKILL_PIPEMAKING);

		sprintf(buf, "a pipe made from %s", obj->short_description);
		obj->short_description = str_dup(buf);
		strcpy(buf, "pipe ");
		strcat(buf, obj->name);
		obj->name = str_dup(buf);

	}
}


ACMD(do_light)
{
	ACMD(do_grab);
	struct obj_data *obj = NULL;
	char arg1[MAX_INPUT_LENGTH];

	one_argument(argument, arg1);

	if (GET_EQ(ch, WEAR_HOLD) &&
			isname(arg1, GET_EQ(ch, WEAR_HOLD)->name))
		obj = GET_EQ(ch, WEAR_HOLD);

	if (!obj)
		obj = get_obj_in_list_vis(ch, arg1, ch->carrying);

	if (!obj)
		send_to_char(ch, "Light what?\r\n");
	else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
		do_grab(ch, fname(obj->name), 0, 0, 0);
	else if (GET_OBJ_TYPE(obj) == ITEM_TOBACCO)
		send_to_char(ch, "You need to roll it up first.\r\n");
	else if (GET_OBJ_TYPE(obj) != ITEM_CIGARETTE &&
		GET_OBJ_TYPE(obj) != ITEM_PIPE && !IS_BOMB(obj))
		send_to_char(ch, "You can't light that!\r\n");
	else {
		if (IS_BOMB(obj)) {
			if (!obj->contains)
				act("$p is not fused.", FALSE, ch, obj, 0, TO_CHAR);
			else if (!IS_FUSE(obj->contains) || !FUSE_IS_BURN(obj->contains))
				act("$p is not equipped with a conventional fuse.",
					FALSE, ch, obj, 0, TO_CHAR);
			else if (FUSE_STATE(obj->contains))
				act("$p is already lit.", FALSE, ch, obj, 0, TO_CHAR);
			else {
				act("You light $p.", TRUE, ch, obj, 0, TO_CHAR);
				act("$n lights $p.", TRUE, ch, obj, 0, TO_ROOM);
				FUSE_STATE(obj->contains) = 1;
				BOMB_IDNUM(obj) = GET_IDNUM(ch);
			}
		} else if (GET_OBJ_VAL(obj, 3))
			send_to_char(ch, "That's already lit.\r\n");
		else if (GET_OBJ_TYPE(obj) == ITEM_PIPE && !GET_OBJ_VAL(obj, 0))
			send_to_char(ch, "There's nothing in it!\r\n");
		else {
			act("You light $p.", TRUE, ch, obj, 0, TO_CHAR);
			act("$n lights $p.", TRUE, ch, obj, 0, TO_ROOM);
			GET_OBJ_VAL(obj, 3) = 1;
		}
	}
	return;
}

ACMD(do_extinguish)
{
	struct obj_data *ovict = NULL;
	struct Creature *vict = NULL;
	char arg1[MAX_INPUT_LENGTH];
	one_argument(argument, arg1);

	if (!*arg1 || ch == get_char_room_vis(ch, arg1)) {
		if (!IS_AFFECTED_2(ch, AFF2_ABLAZE))
			send_to_char(ch, "You're not even on fire!\r\n");
		else if (((number(1,
						80) > (GET_LEVEL(ch) + GET_WIS(ch) - GET_AC(ch) / 10))
				|| ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)
				|| (IS_VAMPIRE(ch) && OUTSIDE(ch)
					&& ch->in_room->zone->weather->sunlight == SUN_LIGHT
					&& GET_PLANE(ch->in_room) < PLANE_ASTRAL))
			&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
			send_to_char(ch, "You fail to extinguish yourself!\r\n");
			act("$n tries to put out the flames which cover $s body, but fails!", FALSE, ch, 0, 0, TO_ROOM);
		} else {
			send_to_char(ch, "You succeed in putting out the flames.  WHEW!\r\n");
			act("$n hastily extinguishes the flames which cover $s body.",
				FALSE, ch, 0, 0, TO_ROOM);
			REMOVE_BIT(AFF2_FLAGS(ch), AFF2_ABLAZE);
		}
	return;
	}
	
	if (GET_EQ(ch, WEAR_HOLD) &&
			isname(arg1, GET_EQ(ch, WEAR_HOLD)->name))
		ovict = GET_EQ(ch, WEAR_HOLD);

	if (!ovict)
		ovict = get_obj_in_list_vis(ch, arg1, ch->carrying);
	if (!ovict)
		ovict = get_obj_in_list_vis(ch, arg1, ch->in_room->contents);

	if (ovict) {
		if (!OBJ_TYPE(ovict, ITEM_PIPE) &&
			!OBJ_TYPE(ovict, ITEM_CIGARETTE) && !OBJ_TYPE(ovict, ITEM_BOMB))
			send_to_char(ch, "You can't extinguish that.\r\n");
		else if (((GET_OBJ_TYPE(ovict) == ITEM_PIPE ||
					GET_OBJ_TYPE(ovict) == ITEM_CIGARETTE) &&
				!GET_OBJ_VAL(ovict, 3)) ||
			(IS_BOMB(ovict) &&
				(!ovict->contains || !IS_FUSE(ovict->contains) ||
					!FUSE_IS_BURN(ovict->contains))))
			send_to_char(ch, "That's not ablaze!\r\n");
		else {
			act("You extinguish $p.", FALSE, ch, ovict, 0, TO_CHAR);
			act("$n extinguishes $p.", FALSE, ch, ovict, 0, TO_ROOM);
			if (IS_BOMB(ovict))
				FUSE_STATE(ovict->contains) = 0;
			else
				GET_OBJ_VAL(ovict, 3) = 0;
		}
		return;
	}

	if ((vict = get_char_room_vis(ch, arg1))) {
		if (!IS_AFFECTED_2(vict, AFF2_ABLAZE))
			act("$N's not even on fire!", FALSE, ch, 0, vict, TO_CHAR);
		else if (number(40,
				80) > (GET_LEVEL(ch) + GET_WIS(ch) - GET_AC(vict) / 10)) {
			act("You fail to extinguish $M!", FALSE, ch, 0, vict, TO_CHAR);
			act("$n tries to put out the flames on $N's body, but fails!",
				FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n tries to put out the flames on your body, but fails!",
				FALSE, ch, 0, vict, TO_VICT);
		} else {
			send_to_char(ch, "You succeed in putting out the flames.  WHEW!\r\n");
			act("$n hastily puts out the flames on your body.", FALSE, ch, 0,
				vict, TO_VICT);
			act("$n hastily extinguishes the flames on $N's body.", FALSE, ch,
				0, vict, TO_NOTVICT);
			REMOVE_BIT(AFF2_FLAGS(vict), AFF2_ABLAZE);
		}
	}
}

ACMD(do_ignite)
{
	struct Creature *vict;
	char arg1[MAX_INPUT_LENGTH];
	ACMD(do_light);
	one_argument(argument, arg1);

	if (GET_LEVEL(ch) < LVL_CREATOR) {
		do_light(ch, argument, 0, 0, 0);
		return;
	}

	if (!(vict = get_char_room_vis(ch, arg1)))
		send_to_char(ch, "No-one by that name around here.\r\n");
	else if (GET_LEVEL(vict) > GET_LEVEL(ch))
		send_to_char(ch, "That's probably a really bad idea.\r\n");
	else if (IS_AFFECTED_2(vict, AFF2_ABLAZE))
		act("$N is already ablaze!", FALSE, ch, 0, vict, TO_CHAR);
	else {
		act("$n makes an arcane gesture...", FALSE, ch, 0, 0, TO_ROOM);
		if (ch != vict)
			act("$N is suddenly consumed in burning flames!!!", FALSE, ch, 0,
				vict, TO_CHAR);
		act("$N is suddenly consumed in burning flames!!!", FALSE, ch, 0, vict,
			TO_NOTVICT);
		act("You are suddenly consumed in burning flames!!!", FALSE, ch, 0,
			vict, TO_VICT);
		SET_BIT(AFF2_FLAGS(vict), AFF2_ABLAZE);
	}
	return;
}

#undef __smokes_c__
