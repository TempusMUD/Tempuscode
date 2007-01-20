//
// File: act.hood.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "guns.h"
#include "bomb.h"
#include "utils.h"
#include "house.h"
#include "vendor.h"


int check_mob_reaction(Creature *ch, Creature *vict);
int apply_soil_to_char(Creature *ch, obj_data *obj, int type, int pos);
int clan_house_can_enter(Creature *ch, struct room_data *room);

ACMD(do_taunt)
{
	send_to_char(ch, "You taunt them mercilessly!\r\n");
}

obj_data*
find_hamstring_weapon( Creature *ch )
{
	obj_data* weap = NULL;
	if( (weap = GET_EQ(ch, WEAR_WIELD)) && SLASHING(weap)) {
		return weap;
	} else if( (weap = GET_EQ(ch, WEAR_WIELD_2)) && SLASHING(weap)) {
		return weap;
	} else if( (weap = GET_EQ(ch, WEAR_HANDS)) && 
			   IS_OBJ_TYPE(weap, ITEM_WEAPON) && SLASHING(weap) ) {
		return weap;
	} else if( (weap = GET_EQ(ch, WEAR_ARMS)) && 
			   IS_OBJ_TYPE(weap, ITEM_WEAPON) && SLASHING(weap) ) {
		return weap;
	} else if( (weap = GET_IMPLANT(ch, WEAR_HANDS)) && 
			   IS_OBJ_TYPE(weap, ITEM_WEAPON) && SLASHING(weap) && 
			   GET_EQ(ch, WEAR_HANDS) == NULL ) {
		return weap;
	} else if( (weap = GET_IMPLANT(ch, WEAR_ARMS)) && 
			   IS_OBJ_TYPE(weap, ITEM_WEAPON) && SLASHING(weap) && 
			   GET_EQ(ch, WEAR_ARMS) == NULL ) {
		return weap;
	}
	return NULL;
}

ACMD(do_hamstring)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL, *weap = NULL;
	int percent, prob, dam;
	struct affected_type af;
	int retval = 0;
    char *arg;

	arg = tmp_getword(&argument);
	if (CHECK_SKILL(ch, SKILL_HAMSTRING) < 50) {
		send_to_char(ch, "Even if you knew what that was, you wouldn't do it.\r\n");
		return;
	}

	if( IS_CLERIC(ch) && IS_GOOD(ch) ) {
		send_to_char(ch, "Your diety forbids this.\r\n");
		return;
	}
	// If there's noone in the room that matches your alias
	// Then it must be an object.
	if (!(vict = get_char_room_vis(ch, arg))) {
		if (ch->numCombatants()) {
			vict = ch->findRandomCombat();
		} else {
			if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
				act("You open a deep gash in $p's hamstring!", FALSE, ch,
					ovict, 0, TO_CHAR);
				return;
			} else {
				send_to_char(ch, "Hamstring who?\r\n");
				return;
			}
		}
	}

	weap = find_hamstring_weapon(ch);
	if( weap == NULL) {
		send_to_char(ch, "You need to be using a slashing weapon.\r\n");
		return;
	}
	if (vict == ch) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
		send_to_char(ch, 
			"Cutting off your own leg just doesn't sound like fun.\r\n");
		return;
	}
	if (!ch->isOkToAttack(vict, true))
		return;

	if (vict->getPosition() == POS_SITTING) {
		send_to_char(ch, "How can you cut it when they're sitting on it!\r\n");
		return;
	}
	if (vict->getPosition() == POS_RESTING) {
		send_to_char(ch, "How can you cut it when they're laying on it!\r\n");
		return;
	}
	prob = CHECK_SKILL(ch, SKILL_HAMSTRING) + GET_REMORT_GEN(ch);
	percent = number(0, 125);
	if (affected_by_spell(vict, ZEN_AWARENESS)) {
		percent += 25;
	}
	if (AFF2_FLAGGED(vict, AFF2_HASTE) && !AFF2_FLAGGED(ch, AFF2_HASTE)) {
		percent += 30;
	}

	if (GET_DEX(ch) > GET_DEX(vict)) {
		prob += 3 * (GET_DEX(ch) - GET_DEX(vict));
	} else {
		percent += 3 * (GET_DEX(vict) - GET_DEX(ch));
	}
	// If they're wearing anything usefull on thier legs make it harder to hurt em.
	if ((ovict = GET_EQ(vict, WEAR_LEGS)) && OBJ_TYPE(ovict, ITEM_ARMOR)) {
		if (IS_STONE_TYPE(ovict) || IS_METAL_TYPE(ovict))
			percent += GET_OBJ_VAL(ovict, 0) * 3;
		else
			percent += GET_OBJ_VAL(ovict, 0);
	}

	if (GET_LEVEL(ch) > GET_LEVEL(vict)) {
		prob += GET_LEVEL(ch) - GET_LEVEL(vict);
	} else {
		percent += GET_LEVEL(vict) - GET_LEVEL(ch);
	}

	if (IS_PUDDING(vict) || IS_SLIME(vict)
		|| NON_CORPOREAL_UNDEAD(vict) || IS_ELEMENTAL(vict))
		prob = 0;
	if (CHECK_SKILL(ch, SKILL_HAMSTRING) < 30) {
		prob = 0;
	}

	cur_weap = weap;
	if (percent > prob) {
		damage(ch, vict, 0, SKILL_HAMSTRING, WEAR_LEGS);
		WAIT_STATE(ch, 2 RL_SEC);
		return;
	} else {
		int level = 0, gen = 0;
		level = GET_LEVEL(ch);
		gen = GET_REMORT_GEN(ch);
		// For proper behavior, the mob has to be set sitting _before_ we
		// damage it.  However, we don't want it to deal the extra damage.
		// Sitting damage is 5/3rds of fighting damage, so we multiply by
		// 3/5ths to cancel out the multiplier
		dam = dice(level, 15 + gen / 2) * 3 / 5;
		add_blood_to_room(vict->in_room, 1);
		apply_soil_to_char(vict, GET_EQ(vict, WEAR_LEGS), SOIL_BLOOD, WEAR_LEGS);
		apply_soil_to_char(vict, GET_EQ(vict, WEAR_FEET), SOIL_BLOOD, WEAR_FEET);
		if (!affected_by_spell(vict, SKILL_HAMSTRING)) {
			af.type = SKILL_HAMSTRING;
			af.is_instant = 0;
			af.bitvector = AFF3_HAMSTRUNG;
			af.aff_index = 3;
			af.level = level + gen;
			af.duration = level + gen / 10;
			af.location = APPLY_DEX;
			af.modifier = 0 - (level / 2 + dice(7, 7) + dice(gen, 5))
				* (CHECK_SKILL(ch, SKILL_HAMSTRING)) / 1000;
            af.owner = ch->getIdNum();
			affect_to_char(vict, &af);
			WAIT_STATE(vict, 3 RL_SEC);
			vict->setPosition(POS_RESTING);
			retval = damage(ch, vict, dam, SKILL_HAMSTRING, WEAR_LEGS);
		} else {
			WAIT_STATE(vict, 2 RL_SEC);
			vict->setPosition(POS_SITTING);
			retval = damage(ch, vict, dam / 2, SKILL_HAMSTRING, WEAR_LEGS);
		}
		if( !IS_SET(retval, DAM_ATTACKER_KILLED) ) {
			gain_skill_prof(ch, SKILL_HAMSTRING);
			WAIT_STATE(ch, 5 RL_SEC);
		}
	}
}



ACMD(do_drag_char)
{
	struct Creature *vict = NULL;
	struct room_data *target_room = NULL;
	struct room_data *location = NULL;

	int percent, prob;
	int found = 0;
    char *arg, *arg2;

	int dir = -1;

	location = ch->in_room;

     arg = tmp_getword(&argument);
     arg2 = tmp_getword(&argument);

	if (!(vict = get_char_room_vis(ch, arg))) {
		send_to_char(ch, "Who do you want to drag?\r\n");
		WAIT_STATE(ch, 3);
		return;
	}

	if (vict == ch) {
		send_to_char(ch, "You can't drag yourself!\r\n");
		return;
	}

	if (!*arg2) {
		send_to_char(ch, "Which direction do you wish to drag them?\r\n");
		WAIT_STATE(ch, 3);
		return;
	}

	if (!ch->isOkToAttack(vict, true)) {
		return;
	}
// Find out which direction the player wants to drag in    
	for (dir = 0; dir < NUM_DIRS && !found; dir++) {
		if (is_abbrev(arg2, dirs[dir])) {
			found = 1;
			break;
		}
	}
	if (!found) {
		send_to_char(ch, "Sorry, that's not a valid direction.\r\n");
		return;
	}

	if (EXIT(ch, dir) && (target_room = EXIT(ch, dir)->to_room) != NULL) {
		if (CAN_GO(ch, dir) && (ROOM_FLAGGED(target_room, ROOM_HOUSE) &&
				!House_can_enter(ch, target_room->number)) ||
			(ROOM_FLAGGED(target_room, ROOM_CLAN_HOUSE)
				&& !clan_house_can_enter(ch, target_room))
			|| (ROOM_FLAGGED(target_room, ROOM_DEATH))) {
			act("You are unable to drag $M there.", FALSE, ch, 0, vict,
				TO_CHAR);
			return;
		}
	}
	if (!CAN_GO(ch, dir) ||
		!can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0) ||
		!CAN_GO(vict, dir)) {
		send_to_char(ch, "Sorry you can't go in that direction.\r\n");
		return;
	}
	percent = ((GET_LEVEL(vict)) + number(1, 101));
	percent -= (GET_WEIGHT(ch) - GET_WEIGHT(vict)) / 5;

	if (GET_STR(ch) >= 19) {
		percent -= (GET_STR(ch) * 2);
	} else {
		percent -= (GET_STR(ch));
	}


	prob =
		MAX(0, (GET_LEVEL(ch) + (CHECK_SKILL(ch,
					SKILL_DRAG)) - GET_STR(vict)));
	prob = MIN(prob, 100);

	if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
		percent = 101;
	}

	if (CHECK_SKILL(ch, SKILL_DRAG) < 30) {
		percent = 101;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
		percent = 101;
	}

	if (prob > percent) {
		
		act(tmp_sprintf("You drag $N to the %s.", to_dirs[dir]),
            FALSE, ch, 0, vict, TO_CHAR);
		act(tmp_sprintf("$n grabs you and drags you %s.", to_dirs[dir]),
            FALSE, ch, 0, vict, TO_VICT);
		act(tmp_sprintf("$n drags $N to the %s.", to_dirs[dir]),
            FALSE, ch, 0, vict, TO_NOTVICT);

		perform_move(ch, dir, MOVE_NORM, 1);
		perform_move(vict, dir, MOVE_DRAG, 1);

		WAIT_STATE(ch, (PULSE_VIOLENCE * 2));
		WAIT_STATE(vict, PULSE_VIOLENCE);

		if (IS_NPC(vict) && AWAKE(vict) && check_mob_reaction(ch, vict)) {
			hit(vict, ch, TYPE_UNDEFINED);
			WAIT_STATE(ch, 2 RL_SEC);
		}
		return;
	} else {
		act("$n grabs $N but fails to move $m.", FALSE, ch, 0, vict,
			TO_NOTVICT);
		act("You attempt to man-handle $N but you fail!", FALSE, ch, 0, vict,
			TO_CHAR);
		act("$n attempts to drag you, but you hold your ground.", FALSE, ch, 0,
			vict, TO_VICT);
		WAIT_STATE(ch, PULSE_VIOLENCE);

		if (IS_NPC(vict) && AWAKE(vict) && check_mob_reaction(ch, vict)) {
			hit(vict, ch, TYPE_UNDEFINED);
			WAIT_STATE(ch, 2 RL_SEC);
		}
		return;
	}
}

ACMD(do_snatch)
{
	//
	//  Check to make sure they have a free hand!
	//
	struct Creature *vict = NULL;
	struct obj_data *obj, *sec_weap;
	char vict_name[MAX_INPUT_LENGTH];
	int percent = -1, eq_pos = -1;
	int prob = 0;
	int position = 0;
	int dam = 0;
	int bonus, dex_mod, str_mod;
	obj = NULL;
	one_argument(argument, vict_name);

	if (!(vict = get_char_room_vis(ch, vict_name))) {
		send_to_char(ch, "Who do you want to snatch from?\r\n");
		return;
	} else if (vict == ch) {
		send_to_char(ch, "Come on now, that's rather stupid!\r\n");
		return;
	}

	if (CHECK_SKILL(ch, SKILL_SNATCH) < 50) {
		send_to_char(ch, "I don't think they are going to let you do that.\r\n");
		return;
	}

	if (vict->isNewbie() && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You cannot snatch from newbies!\r\n");
		return;
	}
	if (!IS_MOB(vict) && ch->isNewbie()) {
		send_to_char(ch, "You can't snatch from players. You're a newbie!\r\n");
		return;
	}

	if (!IS_MOB(vict) && !vict->desc && GET_LEVEL(ch) < LVL_ELEMENT) {
		send_to_char(ch, "You cannot snatch from linkless players!!!\r\n");
		mudlog(GET_LEVEL(ch), CMP, true,
			"%s attempted to snatch from linkless %s.",
			GET_NAME(ch), GET_NAME(vict));
		return;
	}

	check_thief(ch, vict, NULL);

	// Figure out what we're gonna snatch from them.
	// Possible targets are anything in hands or on belt,
	//         including inventory.
	// Possible locations:
	// (in order of precedence)
	//        - Hold
	//        - Light
	//        - Shield
	//        - Belt
	//        - Wield
	//        - SecondWield

	position = number(1, 100);

	if (position < 20) {		// Hold
		obj = GET_EQ(vict, WEAR_HOLD);
		eq_pos = WEAR_HOLD;
	}
	if (!obj && position < 35) {	// Shield
		obj = GET_EQ(vict, WEAR_SHIELD);
		eq_pos = WEAR_SHIELD;
	}
	if (!obj && position < 55) {	// Belt
		obj = GET_EQ(vict, WEAR_BELT);
		eq_pos = WEAR_BELT;
	}
	if (!obj && position < 70) {	// Light
		obj = GET_EQ(vict, WEAR_LIGHT);
		eq_pos = WEAR_LIGHT;
	}
	if (!obj && position < 80) {	// Wield
		obj = GET_EQ(vict, WEAR_WIELD);
		eq_pos = WEAR_WIELD;
	}
	if (!obj && position < 90) {	// SecondWield
		obj = GET_EQ(vict, WEAR_WIELD_2);
		eq_pos = WEAR_WIELD_2;
	}
	if (!obj && position > 90) {	// Damn, inventory it is.
		// Screw inventory for the time being.
	}
	// Roll the dice...
	percent = number(1, 100);

	if (vict->getPosition() < POS_SLEEPING)
		percent = -150;			// ALWAYS SUCCESS

	if (ch->numCombatants())
		percent += 30;

	if (vict->getPosition() < POS_FIGHTING)
		percent -= 30;
	if (vict->getPosition() <= POS_SLEEPING)
		percent = -150;			// ALWAYS SUCCESS


	// NO NO With Imp's and Shopkeepers!
	if (!ok_damage_vendor(ch, vict) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_UTILITY)))
		percent = 121;			// Failure

	// Mod the percentage based on position and flags
	if (obj) {					// If hes got his hand on something, 
		//lets see if he gets it.
		if (IS_OBJ_STAT(obj, ITEM_NODROP))
			percent += 10;
		if (IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM))
			percent += 10;
		if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE))
			percent += 15;
		if (GET_LEVEL(ch) > LVL_TIMEGOD && GET_LEVEL(vict) < GET_LEVEL(ch))
			percent = 0;
		// Snatch the pebble from my hand grasshopper.
		if (IS_MONK(vict) || IS_THIEF(vict))
			percent += 10;
		if (affected_by_spell(vict, ZEN_AWARENESS))
			percent += 10;
	}
	// Check the percentage against the current skill.
	// If they succeed the leftover is thier bonus.
	// If they fail thier roll, or choose an empty position
	if (percent > CHECK_SKILL(ch, SKILL_SNATCH) || !obj) {
		act("$n tries to snatch something from $N but comes away empty handed!", FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n tries to snatch something from you but comes away empty handed!", FALSE, ch, 0, vict, TO_VICT);
		act("You try to snatch something from $N but come away empty handed!",
			FALSE, ch, 0, vict, TO_CHAR);

		// Monks are cool. They stand up when someone tries to snatch from em.
		if (vict->getPosition() == POS_SITTING
			&& IS_AFFECTED_2(vict, AFF2_MEDITATE)) {
			vict->setPosition(POS_STANDING);
			act("You jump to your feet, glaring at $s!", FALSE, ch, 0, vict,
				TO_VICT);
			act("$N jumps to $S feet, glaring at You!", FALSE, ch, 0, vict,
				TO_CHAR);
			act("$N jumps to $S feet, glaring at $n!", FALSE, ch, 0, vict,
				TO_NOTVICT);
		}

	} else {					// They got a hand on it, and it exists.
		bonus = CHECK_SKILL(ch, SKILL_SNATCH) - percent;

		// Now that the hood has a hand on, contest to see who ends up with the item.
		// Note: percent has started from scratch here.
		// str_mod and dex_mod control how signifigant an effect differences in dex
		//    have on the probability of success.  At 7, its 5% likely from this point on
		//    that an average mort hood can wrench the hammer mjolnir from thor.
		str_mod = 7;
		dex_mod = 7;

		// This beastly function brought to you by Cat and the letter F.
		percent = (GET_LEVEL(ch) - GET_LEVEL(vict) * 2)
			+ bonus + (dex_mod * (GET_DEX(ch) - GET_DEX(vict)))
			+ (str_mod * (GET_STR(ch) - GET_STR(vict)));
		if (CHECK_SKILL(ch, SKILL_SNATCH) > 100)
			percent += CHECK_SKILL(ch, SKILL_SNATCH) - 100;
		prob = number(1, 100);
		if (PRF2_FLAGGED(ch, PRF2_DEBUG))
			send_to_char(ch, "%s[SNATCH] %s   chance:%d   roll:%d%s\r\n",
				CCCYN(ch, C_NRM), GET_NAME(ch), percent, prob,
				CCNRM(ch, C_NRM));
		if (PRF2_FLAGGED(vict, PRF2_DEBUG))
			send_to_char(vict, "%s[SNATCH] %s   chance:%d   roll:%d%s\r\n",
				CCCYN(vict, C_NRM), GET_NAME(ch), percent, prob,
				CCNRM(vict, C_NRM));

		//failure. hand on it and failed to take away.
		if (prob > percent) {

			// If it was close, drop the item.
			if (prob < percent + 10) {
				act("$n tries to take $p away from you, forcing you to drop it!", FALSE, ch, obj, vict, TO_VICT);
				act("You try to snatch $p away from $N but $E drops it!",
					FALSE, ch, obj, vict, TO_CHAR);
				act("$n tries to snatch $p from $N but $E drops it!",
					FALSE, ch, obj, vict, TO_NOTVICT);

				// weapon is the 1st of 2 wielded, shift to the second weapon.
				if (obj->worn_on == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD_2)) {
					sec_weap = unequip_char(vict, WEAR_WIELD_2, MODE_EQ);
					obj_to_room(unequip_char(vict, eq_pos, MODE_EQ),
						vict->in_room);
					equip_char(vict, sec_weap, WEAR_WIELD, MODE_EQ);
					act("You shift $p to your primary hand.",
						FALSE, ch, obj, vict, TO_VICT);

					// Otherwise, just drop it.
				} else {
					obj_to_room(unequip_char(vict, eq_pos, MODE_EQ),
						vict->in_room);
				}

				// It wasnt close. He deffinately failed.    
			} else {
				act("$n grabs your $p but you manage to hold onto it!",
					FALSE, ch, obj, vict, TO_VICT);
				act("You grab $S $p but can't snatch it away!",
					FALSE, ch, obj, vict, TO_CHAR);
				act("$n grabs $N's $p but can't snatch it away!",
					FALSE, ch, obj, vict, TO_NOTVICT);
			}

			// success, hand on and wrestled away.
		} else {

			// If he ends up with the shit, advertize.
			if (vict->getPosition() == POS_SLEEPING) {	// He's sleepin, wake his ass up.
				vict->setPosition(POS_RESTING);
				if (eq_pos == WEAR_BELT) {
					act("$n wakes you up snatching something off your belt!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You wake $N up snatching $p off $S belt!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n wakes $N up snatching $p off $S belt!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				} else {
					act("$n wakes you up snatching out of your hand!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You wake $N up snatching $p out of $S hand!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n wakes $N up snatching $p out of $S hand!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				}
			} else if (vict->getPosition() == POS_SITTING &&
				IS_AFFECTED_2(vict, AFF2_MEDITATE)) {
				vict->setPosition(POS_STANDING);
				if (eq_pos == WEAR_BELT) {
					act("$n breaks your trance snatching $p off your belt!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You break $S trance snatching $p off $S belt!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n breaks $N's trance snatching $p off $S belt!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				} else {
					act("$n breaks your trance snatching $p out of your hand!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You break $N's trance snatching $p out of $S hand!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n breaks $N's trance snatching $p out of $S hand!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				}
			} else {
				if (eq_pos == WEAR_BELT) {
					act("$n snatches $p off your belt!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You snatch $p off $N's belt!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n snatches $p off $N's belt!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				} else {
					act("$n snatches $p out of your hand!",
						FALSE, ch, obj, vict, TO_VICT);
					act("You snatch $p out of $N's hand!",
						FALSE, ch, obj, vict, TO_CHAR);
					act("$n snatches $p out of $N's hand!",
						FALSE, ch, obj, vict, TO_NOTVICT);
				}
			}

			// Keep tabs on snatching stuff. :P
			if (GET_LEVEL(ch) >= LVL_AMBASSADOR || !IS_NPC(vict))
				slog("%s stole %s from %s.",
					GET_NAME(ch), obj->name, GET_NAME(vict));
			obj_to_char(unequip_char(vict, eq_pos, MODE_EQ), ch);

			// weapon is the 1st of 2 wielded
			if (obj->worn_on == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD_2)) {
				sec_weap = unequip_char(vict, WEAR_WIELD_2, MODE_EQ);
				equip_char(vict, sec_weap, WEAR_WIELD, MODE_EQ);
				act("You shift $p to your primary hand.",
					FALSE, ch, obj, vict, TO_VICT);
			}
			// Punks tend to break shit.
			dam =
				dice(str_app[GET_STR(ch)].todam, str_app[GET_STR(vict)].todam);
			if (!is_arena_combat(ch, vict))
				damage_eq(NULL, obj, dam);
			GET_EXP(ch) += MIN(1000, GET_OBJ_COST(obj));
			gain_skill_prof(ch, SKILL_SNATCH);
			WAIT_STATE(vict, 2 RL_SEC);
		}
	}
	if (IS_NPC(vict) && AWAKE(vict))
		hit(vict, ch, TYPE_UNDEFINED);
	WAIT_STATE(ch, 4 RL_SEC);
}
