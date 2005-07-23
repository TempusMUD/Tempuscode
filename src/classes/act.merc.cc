//
// File: act.merc.c                     -- Part of TempusMUD
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
#include "tmpstr.h"
#include "smokes.h"
#include "player_table.h"
#include "vendor.h"

#define PISTOL(gun)  ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && !IS_TWO_HAND(gun))
#define LARGE_GUN(gun) ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && IS_TWO_HAND(gun))

int apply_soil_to_char(struct Creature *ch, struct obj_data *obj, int type,
	int pos);

ACMD(do_pistolwhip)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL, *weap = NULL;
	int percent, prob, dam;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (ch->numCombatants()) {
			vict = ch->findRandomCombat();
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You pistolwhip $p!", FALSE, ch, ovict, 0, TO_CHAR);
			return;
		} else {
			send_to_char(ch, "Pistolwhip who?\r\n");
			return;
		}
	}
	if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && PISTOL(weap)) ||
			((weap = GET_EQ(ch, WEAR_WIELD_2)) && PISTOL(weap)) ||
			((weap = GET_EQ(ch, WEAR_HANDS)) && PISTOL(weap)))) {
		send_to_char(ch, "You need to be using a pistol.\r\n");
		return;
	}
	if (vict == ch) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
		act("You slam $p into your head!", FALSE, ch, weap, 0, TO_CHAR);
		act("$n beats $mself senseless with $p!", TRUE, ch, weap, 0, TO_ROOM);
		return;
	}
	if (!ch->isOkToAttack(vict, true))
		return;

	percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);
	prob = CHECK_SKILL(ch, SKILL_PISTOLWHIP);

	if (IS_PUDDING(vict) || IS_SLIME(vict))
		prob = 0;

	cur_weap = weap;
	if (percent > prob) {
		damage(ch, vict, 0, SKILL_PISTOLWHIP, WEAR_BODY);
	} else {
		dam = dice(GET_LEVEL(ch), str_app[STRENGTH_APPLY_INDEX(ch)].todam) +
			dice(4, weap->getWeight());
		dam /= 4;
		damage(ch, vict, dam, SKILL_PISTOLWHIP, WEAR_HEAD);
		gain_skill_prof(ch, SKILL_PISTOLWHIP);
	}
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_crossface)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL, *weap = NULL, *wear = NULL;
	int str_mod, dex_mod, percent = 0, prob = 0, dam = 0;
	int retval = 0, diff = 0, wear_num;
	bool prime_merc = false;
	short prev_pos = 0;

	if (GET_CLASS(ch) == CLASS_MERCENARY)
		prime_merc = true;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (ch->numCombatants()) {
			vict = ch->findRandomCombat();
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You fiercely crossface $p!", FALSE, ch, ovict, 0, TO_CHAR);
			return;
		} else {
			send_to_char(ch, "Crossface who?\r\n");
			return;
		}
	}

	if (!((weap = GET_EQ(ch, WEAR_WIELD)) && LARGE_GUN(weap))) {
		send_to_char(ch, "You need to be using a two handed gun.\r\n");
		return;
	}

	if (vict == ch) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
			act("You fear that your death will grieve $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
		act("You slam $p into your head!", FALSE, ch, weap, 0, TO_CHAR);
		act("$n beats $mself senseless with $p!", TRUE, ch, weap, 0, TO_ROOM);
		return;
	}

	if (!ch->isOkToAttack(vict))
		return;

	if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 8);
		return;
	}

	cur_weap = weap;

	str_mod = 3;
	dex_mod = 3;
	// This beastly function brought to you by Cat, the letter F, and Nothing more
	prob =
		((GET_LEVEL(ch) + ch->getLevelBonus(prime_merc)) -
		(GET_LEVEL(vict) * 2))
		+ (CHECK_SKILL(ch, SKILL_CROSSFACE) >> 2)
		+ (dex_mod * (GET_DEX(ch) - GET_DEX(vict)))
		+ (str_mod * (GET_STR(ch) - GET_STR(vict)));
	percent = number(1, 100);
	if (PRF2_FLAGGED(ch, PRF2_DEBUG))
		send_to_char(ch, "%s[CROSSFACE] %s   roll:%d   chance:%d%s\r\n",
			CCCYN(ch, C_NRM), GET_NAME(ch), prob, percent, CCNRM(ch, C_NRM));
	if (PRF2_FLAGGED(vict, PRF2_DEBUG))
		send_to_char(vict, "%s[CROSSFACE] %s   roll:%d   chance:%d%s\r\n",
			CCCYN(vict, C_NRM), GET_NAME(ch), prob, percent,
			CCNRM(vict, C_NRM));

	// You can't crossface pudding you fool!
	if (IS_PUDDING(vict) || IS_SLIME(vict))
		prob = 0;

	if (percent >= prob) {
		damage(ch, vict, 0, SKILL_CROSSFACE, WEAR_HEAD);
	} else {

		dam = dice(GET_LEVEL(ch), str_app[STRENGTH_APPLY_INDEX(ch)].todam) +
			dice(9, weap->getWeight());

		if ((wear = GET_EQ(vict, WEAR_FACE)))
			wear_num = WEAR_FACE;
		else {
			wear = GET_EQ(vict, WEAR_HEAD);
			wear_num = WEAR_HEAD;
		}

		diff = prob - percent;

		// Wow!  vict really took one hell of a shot.  Stun that bastard!
		if (diff >= 70 && !wear) {
			prev_pos = vict->getPosition();
			retval = damage(ch, vict, dam, SKILL_CROSSFACE, wear_num);
			if (prev_pos != POS_STUNNED && !IS_SET(retval, DAM_VICT_KILLED) &&
				!IS_SET(retval, DAM_ATTACKER_KILLED)) {
				if (!IS_NPC(vict) || (IS_NPC(vict) &&
						!MOB2_FLAGGED(vict, MOB2_NOSTUN))
					&& ch->numCombatants()) {
					ch->removeCombat(vict);
					vict->removeAllCombat();
					vict->setPosition(POS_STUNNED);
					act("Your crossface has knocked $N senseless!",
						TRUE, ch, NULL, vict, TO_CHAR);
					act("$n stuns $N with a vicious crossface!",
						TRUE, ch, NULL, vict, TO_ROOM);
					act("Your jaw cracks as $n whips his gun across your face.   Your vision fades...",  TRUE, ch, NULL, vict, TO_VICT);
				}
			}
		}
		// vict just took a pretty good shot, knock him down...
		else if (diff >= 55) {
			dam = (int)(dam * 0.75);
			prev_pos = vict->getPosition();
			retval = damage(ch, vict, dam, SKILL_CROSSFACE, wear_num);
			if ((prev_pos != POS_RESTING && prev_pos != POS_STUNNED)
				&& !IS_SET(retval, DAM_VICT_KILLED) &&
				!IS_SET(retval, DAM_ATTACKER_KILLED) && ch->numCombatants()) {
				vict->setPosition(POS_RESTING);
				act("Your crossface has knocked $N on his ass!",
					TRUE, ch, NULL, vict, TO_CHAR);
				act("$n's nasty crossface just knocked $N on his ass!",
					TRUE, ch, NULL, vict, TO_ROOM);
				act("Your jaw cracks as $n whips his gun across your face.\n"
					"You stagger and fall to the ground ", 
					TRUE, ch, NULL, vict, TO_VICT);
			}
		}
		// vict pretty much caught a grazing blow, knock off some eq
		else if (diff >= 20) {
			retval = damage(ch, vict, dam >> 1, SKILL_CROSSFACE, wear_num);
			if (wear && !IS_SET(retval, DAM_VICT_KILLED) &&
				!IS_SET(retval, DAM_ATTACKER_KILLED) && ch->numCombatants()) {
				act("Your crossface has knocked $N's $p from his head!",
					TRUE, ch, wear, vict, TO_CHAR);
				act("$n's nasty crossface just knocked $p from $N's head!",
					TRUE, ch, wear, vict, TO_ROOM);
				act("Your jaw cracks as $n whips his gun across your face.\n"
					"Your $p flies from your head and lands a short distance\n"
					"away.", TRUE, ch, wear, vict, TO_VICT);

				damage_eq(vict, wear, dam >> 4);
				obj_to_room(unequip_char(vict, wear_num, MODE_EQ),
					vict->in_room);
				wear = NULL;
			}
		} else {
			retval = damage(ch, vict, dam >> 1, SKILL_CROSSFACE, wear_num);
		}

		if ( wear && 
			 !IS_SET(retval, DAM_VICT_KILLED) && 
			 !IS_SET(retval, DAM_ATTACKER_KILLED) ) {
			damage_eq(vict, wear, dam >> 4);
		}

		gain_skill_prof(ch, SKILL_CROSSFACE);
	}

	if (!IS_SET(retval, DAM_ATTACKER_KILLED))
		WAIT_STATE(ch, 3 RL_SEC);
	if (!IS_SET(retval, DAM_VICT_KILLED))
		WAIT_STATE(vict, 2 RL_SEC);
}

#define NOBEHEAD_EQ(obj) \
IS_SET(obj->obj_flags.bitvector[1], AFF2_NECK_PROTECTED)

// Sniper skill for mercs...I've tried to comment all of
// my thought processes.  Don't be too hard on me, this
// has been my first modification of anything functional

// --Nothing
ACMD(do_snipe)
{
	int choose_random_limb(Creature *victim); // from combat/combat_utils.cc

	struct Creature *vict;
	struct obj_data *gun, *bullet, *armor;
	struct room_data *cur_room, *nvz_room = NULL;
	struct affected_type af;
	int retval = 0;
	int prob, percent, damage_loc, dam = 0;
	int snipe_dir = -1, distance = 0;
	char *vict_str, *dir_str, *kill_msg;

	vict_str = tmp_getword(&argument);
	dir_str = tmp_getword(&argument);

	// ch is blind?
	if (IS_AFFECTED(ch, AFF_BLIND)) {
		send_to_char(ch, "You can't snipe anyone! you're blind!\r\n");
		return;
	}

	//ch in smoky room?
	if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
		GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "The room is too smoky to see very far.\r\n");
		return;
	}
	// ch wielding a rifle?
	if ((gun = GET_EQ(ch, WEAR_WIELD))) {
		if (!IS_RIFLE(gun)) {
			if ((gun = GET_EQ(ch, WEAR_WIELD_2))) {
				if (!IS_RIFLE(gun)) {
					send_to_char(ch, "But you aren't wielding a rifle!\r\n");
					return;
				}
			} else {
				send_to_char(ch, "But you aren't wielding a rifle!\r\n");
				return;
			}
		}
	} else {
		send_to_char(ch, "You aren't wielding anything fool!\r\n");
		return;
	}
	// does ch have snipe leared at all?
	// I don't know if a skill can be less than 0
	// but I don't think it can hurt to check for it
	if (CHECK_SKILL(ch, SKILL_SNIPE) <= 0) {
		send_to_char(ch, "You have no idea how!");
	}

	// is ch's gun loaded?
	if (!GUN_LOADED(gun)) {
		send_to_char(ch, "But your gun isn't loaded!\r\n");
		return;
	}

	//in what direction is ch attempting to snipe?
	snipe_dir = search_block(dir_str, dirs, FALSE);
	if (snipe_dir < 0) {
		send_to_char(ch, "Snipe in which direction?!\r\n");
		return;
	}

	if (!EXIT(ch, snipe_dir) || !EXIT(ch, snipe_dir)->to_room ||
			EXIT(ch, snipe_dir)->to_room == ch->in_room ||
			IS_SET(EXIT(ch, snipe_dir)->exit_info, EX_CLOSED)) {
		send_to_char(ch,
			"You aren't going to be sniping anyone in that direction...\r\n");
		return;
	}
	
	// is the victim in sight in that direction?
	// line of sight stops at a DT, smoke-filled room, or closed door
	distance = 0;
	vict = NULL;
	cur_room = ch->in_room;
	while (!vict && distance < 3) {
		if (!cur_room->dir_option[snipe_dir] ||
				!cur_room->dir_option[snipe_dir]->to_room ||
				cur_room->dir_option[snipe_dir]->to_room == ch->in_room ||
				IS_SET(cur_room->dir_option[snipe_dir]->exit_info, EX_CLOSED))
			break;

		cur_room = cur_room->dir_option[snipe_dir]->to_room;
		distance++;

		if (ROOM_FLAGGED(cur_room, ROOM_DEATH) ||
				ROOM_FLAGGED(cur_room, ROOM_SMOKE_FILLED))
			break;
		
		vict = get_char_in_remote_room_vis(ch, vict_str, cur_room);
        if (!nvz_room && vict && (ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)))
            nvz_room = cur_room;

	}

	if (!vict) {
		send_to_char(ch, "You can't see your target in that direction.\r\n");
		return;
	}

	// is vict an imm?
	if ((GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
		send_to_char(ch, "Are you crazy man!?!  You'll piss off superfly!!\r\n");
		return;
	}
	//is the player trying to snipe himself?
	if (vict == ch) {
		send_to_char(ch, "Yeah...real funny.\r\n");
		return;
	}
	// if vict is fighting someone you have a 50% chance of hitting the person
	// vict is fighting
	if ((vict->numCombatants()) && (number(0, 1))) {
		vict = (vict->findRandomCombat());
	}
	// Has vict been sniped once and is vict a sentinel mob?
	if ((MOB_FLAGGED(vict, MOB_SENTINEL)) &&
			affected_by_spell(ch, SKILL_SNIPE)) {
		act("$N has taken cover!\r\n", TRUE, ch, NULL, vict, TO_CHAR);
		return;
	}
	if (!vict)
		return;

	// is ch in a peaceful room?
	if (!ch->isOkToAttack(vict, true))
		return;

	//Ok, last check...is some asshole trying to damage a shop keeper
	if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
		send_to_char(ch, "You can't seem to get a clean line-of-sight.\r\n");
		return;
	}
	// Ok, the victim is in sight, ch is not blind or in a smoky room
	// ch is wielding a rifle, and neither the victim nor ch is in a
	// peaceful room, and all other misc. checks have been passed...
	// begin the hit prob calculations
	prob = CHECK_SKILL(ch, SKILL_SNIPE) + GET_REMORT_GEN(ch);
	// start percent at 40 to make it impossible for a merc to hit
	// someone if his skill is less than 40
	percent = number(40, 125);
	if (affected_by_spell(vict, ZEN_AWARENESS) ||
		IS_AFFECTED(vict, AFF2_TRUE_SEEING)) {
		percent += 25;
	}

	if (vict->getPosition() < POS_FIGHTING) {
		percent += 15;
	}

	if (IS_AFFECTED_2(vict, AFF2_HASTE) && !IS_AFFECTED_2(ch, AFF2_HASTE)) {
		percent += 25;
	}
	// if the victim has already been sniped (wears off in 3 ticks)
	// then the victim is aware of a sniper and is assumed
	// to be taking the necessary precautions, and therefore is
	// much harder to hit
	if (affected_by_spell(vict, SKILL_SNIPE)) {
		percent += 50;
	}
	// just some level checks.  The victims level matters more
	// because it should be almost impossible to hit a high
	// level character who has been sniped once already
	prob += (GET_LEVEL(ch) >> 2) + GET_REMORT_GEN(ch);
	percent += GET_LEVEL(vict) + (GET_REMORT_GEN(vict) >> 2);
	damage_loc = choose_random_limb(vict);
	// we need to extract the bullet so we need an object pointer to
	// it.  However we musn't over look the possibility that gun->contains
	// could be a clip rather than a bullet
	bullet = gun->contains;
	if (IS_CLIP(bullet)) {
		bullet = bullet->contains;
	}
	
	if (nvz_room) {
		send_to_char(ch, "You watch in shock as your bullet stops in mid-air and drops to the ground.\r\n");
		act("$n takes careful aim, fires, and gets a shocked look on $s face.",
			false, ch, 0, 0, TO_ROOM);
		send_to_room(tmp_sprintf(
			"%s screams in from %s and harmlessly falls to the ground.",
				bullet->name, from_dirs[snipe_dir]), nvz_room);
		obj_from_obj(bullet);
		obj_to_room(bullet, nvz_room);
		return;
	} else {
		extract_obj(bullet);
	}

	// if percent is greater than prob it's a miss
	if (percent > prob) {
		// call damage with 0 dam to check for killers, TG's
		// newbie protection and other such stuff automagically ;)
		retval = damage(ch, vict, 0, SKILL_SNIPE, damage_loc);
		if (retval == DAM_ATTACK_FAILED) {
			return;
		}
		// ch and vict really shouldn't be fighting if they aren't in
		// the same room...
		ch->removeCombat(vict);
		vict->removeCombat(ch);
		send_to_char(ch, "Damn!  You missed!\r\n");
		act("$n fires $p to the %s, and a look of irritation crosses $s face.",
			TRUE, ch, gun, vict, TO_ROOM);
		act(tmp_sprintf("A bullet screams past your head from the %s!",
			from_dirs[snipe_dir]), TRUE, ch, NULL, vict, TO_VICT);
		act(tmp_sprintf("A bullet screams past $n's head from the %s!",
			from_dirs[snipe_dir]), TRUE, vict, NULL, ch, TO_ROOM);
		WAIT_STATE(ch, 3 RL_SEC);
		return;
	} else {
		// it a hit!
		// grab the damage for the gun...this way this skill is
		// scalable
		dam = dice(gun_damage[GUN_TYPE(gun)][0],
			gun_damage[GUN_TYPE(gun)][1] + BUL_DAM_MOD(bullet));
		// as you can see, armor makes a huge difference, it's hard to imagine
		// a bullet doing much more than brusing someone through a T. carapace
		// seems to crash the mud if you call damage_eq() on a location
		// that doesn't have any eq...hmmm
		if (GET_EQ(vict, damage_loc)) {
			damage_eq(vict, GET_EQ(vict, damage_loc), dam >> 1);
		}
		if ((armor = GET_EQ(vict, damage_loc))
			&& OBJ_TYPE(armor, ITEM_ARMOR)) {
			if (IS_STONE_TYPE(armor) || IS_METAL_TYPE(armor))
				dam -= GET_OBJ_VAL(armor, 0) << 4;
			else
				dam -= GET_OBJ_VAL(armor, 0) << 2;
		}

		add_blood_to_room(vict->in_room, 1);
		apply_soil_to_char(ch, GET_EQ(vict, damage_loc), SOIL_BLOOD,
			damage_loc);
		if (!affected_by_spell(vict, SKILL_SNIPE)) {
			memset(&af, 0, sizeof(af));
			af.type = SKILL_SNIPE;
			af.level = GET_LEVEL(ch);
			af.duration = 3;
            af.owner = ch->getIdNum();
			affect_to_char(vict, &af);
		}

		WAIT_STATE(vict, 2 RL_SEC);
		// double damage for a head shot...1 in 27 chance
		if (damage_loc == WEAR_HEAD) {
			dam = dam << 1;
		}
		// 1.5x damage for a neck shot...2 in 27 chance
		else if (damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
			dam += dam >> 1;
		}
		if (damage_loc == WEAR_HEAD) {
			send_to_char(ch, "HEAD SHOT!!\r\n");
		} else if (damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
			send_to_char(ch, "NECK SHOT!!\r\n");
		}
		if (!IS_NPC(vict) && GET_LEVEL(vict) <= 6) {
			send_to_char(ch, "Hmm. Your gun seems to have jammed...\r\n");
			return;
		}

		act("You smirk with satisfaction as your bullet rips into $N.",
			FALSE, ch, NULL, vict, TO_CHAR);
		act(tmp_sprintf("A bullet rips into your flesh from the %s!",
			dirs[snipe_dir]), TRUE, ch, NULL, vict, TO_VICT);
		act("A bullet rips into $n's flesh!", TRUE, vict, NULL, ch,
			TO_ROOM);
		act(tmp_sprintf("$n takes careful aim and fires $p to the %s!",
			dirs[snipe_dir]), TRUE, ch, gun, vict, TO_ROOM);
		mudlog(LVL_AMBASSADOR, NRM, true,
			"INFO: %s has sniped %s from room %d to room %d",
			GET_NAME(ch), GET_NAME(vict),
			ch->in_room->number, vict->in_room->number);

		kill_msg = tmp_sprintf("You have killed %s!", GET_NAME(vict));

		retval = damage(ch, vict, dam, SKILL_SNIPE, damage_loc);
		if (retval == DAM_ATTACK_FAILED)
			return;

		if (IS_SET(retval, DAM_VICT_KILLED)) {
			act(kill_msg, TRUE, ch, 0, 0, TO_CHAR);
			act("$n gets a look of predatory satisfaction.",
				TRUE, ch, 0, 0, TO_ROOM);
		}
		gain_skill_prof(ch, SKILL_SNIPE);
		WAIT_STATE(ch, 5 RL_SEC);
	}
}

ACMD(do_wrench)
{
	struct Creature *vict = NULL;
	struct obj_data *ovict = NULL;
	struct obj_data *neck = NULL;
	int two_handed = 0;
	int prob, percent, dam;

	one_argument(argument, arg);

	if (!(vict = get_char_room_vis(ch, arg))) {
		if (ch->numCombatants()) {
			vict = (ch->findRandomCombat());
		} else if ((ovict =
				get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
			act("You fiercly wrench $p!", FALSE, ch, ovict, 0, TO_CHAR);
			return;
		} else {
			send_to_char(ch, "Wrench who?\r\n");
			return;
		}
	}

	if (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
		send_to_char(ch, 
			"You are using both hands to wield your weapon right now!\r\n");
		return;
	}

	if (GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_WIELD_2) ||
			GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_SHIELD))) {
		send_to_char(ch, "You need a hand free to do that!\r\n");
		return;
	}

	if (!ch->isOkToAttack(vict, true))
		return;
	//
	// give a bonus if both hands are free
	//

	if (!GET_EQ(ch, WEAR_WIELD) &&
		!(GET_EQ(ch, WEAR_WIELD_2) || GET_EQ(ch, WEAR_HOLD)
			|| GET_EQ(ch, WEAR_SHIELD))) {
		two_handed = 1;
	}

	percent = ((10 - (GET_AC(vict) / 50)) << 1) + number(1, 101);
	prob = CHECK_SKILL(ch, SKILL_WRENCH);

	if (!can_see_creature(ch, vict)) {
		prob += 10;
	}

	dam = dice(GET_LEVEL(ch), GET_STR(ch));

	if (two_handed) {
		dam += dam / 2;
	}

	if (!(ch->numCombatants()) && !(vict->numCombatants())) {
		dam += dam / 3;
	}

	if (((neck = GET_IMPLANT(vict, WEAR_NECK_1)) && NOBEHEAD_EQ(neck)) ||
		((neck = GET_IMPLANT(vict, WEAR_NECK_2)) && NOBEHEAD_EQ(neck))) {
		dam >>= 1;
		damage_eq(ch, neck, dam);
	}

	if (((neck = GET_EQ(vict, WEAR_NECK_1)) && NOBEHEAD_EQ(neck)) ||
		((neck = GET_EQ(vict, WEAR_NECK_2)) && NOBEHEAD_EQ(neck))) {
		act("$n grabs you around the neck, but you are covered by $p!", FALSE,
			ch, neck, vict, TO_VICT);
		act("$n grabs $N's neck, but $N is covered by $p!", FALSE, ch, neck,
			vict, TO_NOTVICT);
		act("You grab $N's neck, but $e is covered by $p!", FALSE, ch, neck,
			vict, TO_CHAR);
		check_killer(ch, vict);
		damage_eq(ch, neck, dam);
		WAIT_STATE(ch, 2 RL_SEC);
		return;
	}

	if (prob > percent && (CHECK_SKILL(ch, SKILL_WRENCH) >= 30)) {
		WAIT_STATE(ch, PULSE_VIOLENCE * 4);
		WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		damage(ch, vict, dam, SKILL_WRENCH, WEAR_NECK_1);
		gain_skill_prof(ch, SKILL_WRENCH);
		return;
	} else {
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		damage(ch, vict, 0, SKILL_WRENCH, WEAR_NECK_1);
	}
}

ACMD(do_infiltrate)
{
	struct affected_type af;

	if (IS_AFFECTED_3(ch, AFF3_INFILTRATE)) {
		send_to_char(ch, "Okay, you are no longer attempting to infiltrate.\r\n");
		affect_from_char(ch, SKILL_INFILTRATE);
		affect_from_char(ch, SKILL_SNEAK);
		return;
	}

	if (CHECK_SKILL(ch, SKILL_INFILTRATE) < number(20, 70)) {
		send_to_char(ch, "You don't feel particularly sneaky...\n");
		return;
	}

	send_to_char(ch, "Okay, you'll try to infiltrate until further notice.\r\n");

	af.type = SKILL_SNEAK;
	af.is_instant = 0;
	af.duration = GET_LEVEL(ch);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_SNEAK;
	af.aff_index = 0;
	af.level = GET_LEVEL(ch) + ch->getLevelBonus(SKILL_INFILTRATE);
    af.owner = ch->getIdNum();
	affect_to_char(ch, &af);

	af.type = SKILL_INFILTRATE;
	af.aff_index = 3;
	af.is_instant = 0;
	af.duration = GET_LEVEL(ch);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF3_INFILTRATE;
	af.level = GET_LEVEL(ch) + ch->getLevelBonus(SKILL_INFILTRATE);
    af.owner = ch->getIdNum();
	affect_to_char(ch, &af);
}

void
perform_appraise(Creature *ch, obj_data *obj, int skill_lvl)
{
	int i;
	long cost;
	unsigned long eq_req_flags;

	struct time_info_data age(struct Creature *ch);

	sprinttype(GET_OBJ_TYPE(obj), item_type_descs, buf2);
	send_to_char(ch, "%s is %s.\n", tmp_capitalize(obj->name), buf2); 

	if (skill_lvl > 30) {
		eq_req_flags = ITEM_ANTI_GOOD | ITEM_ANTI_EVIL | ITEM_ANTI_NEUTRAL |
			ITEM_ANTI_MAGIC_USER | ITEM_ANTI_CLERIC | ITEM_ANTI_THIEF |
			ITEM_ANTI_WARRIOR | ITEM_NOSELL | ITEM_ANTI_BARB |
			ITEM_ANTI_PSYCHIC | ITEM_ANTI_PHYSIC | ITEM_ANTI_CYBORG |
			ITEM_ANTI_KNIGHT | ITEM_ANTI_RANGER | ITEM_ANTI_BARD |
			ITEM_ANTI_MONK | ITEM_BLURRED | ITEM_DAMNED;
		if (GET_OBJ_EXTRA(obj) & eq_req_flags) {
			sprintbit(GET_OBJ_EXTRA(obj) & eq_req_flags, extra_bits, buf);
			send_to_char(ch, "Item is: %s\r\n", buf);
		}

		eq_req_flags = ITEM2_ANTI_MERC;
		if (GET_OBJ_EXTRA2(obj) & eq_req_flags) {
			sprintbit(GET_OBJ_EXTRA2(obj) & eq_req_flags, extra2_bits, buf);
			send_to_char(ch, "Item is: %s\r\n", buf);
		}

		eq_req_flags = ITEM3_REQ_MAGE | ITEM3_REQ_CLERIC | ITEM3_REQ_THIEF |
			ITEM3_REQ_WARRIOR | ITEM3_REQ_BARB | ITEM3_REQ_PSIONIC |
			ITEM3_REQ_PHYSIC | ITEM3_REQ_CYBORG | ITEM3_REQ_KNIGHT |
			ITEM3_REQ_RANGER | ITEM3_REQ_BARD | ITEM3_REQ_MONK |
			ITEM3_REQ_VAMPIRE | ITEM3_REQ_MERCENARY;
		if (GET_OBJ_EXTRA3(obj) & eq_req_flags) {
			sprintbit(GET_OBJ_EXTRA3(obj) & eq_req_flags, extra3_bits, buf);
			send_to_char(ch, "Item is: %s\r\n", buf);
		}
	}

	send_to_char(ch, "Item weighs around %d lbs, and is made of %s.\n",
		obj->getWeight(), material_names[GET_OBJ_MATERIAL(obj)]);
	
	if (skill_lvl > 100)
		cost = 0;
	else
		cost = (100 - skill_lvl) * GET_OBJ_COST(obj) / 100;
	cost = GET_OBJ_COST(obj) + number(0, cost) - cost / 2;

	if (cost > 0)
		send_to_char(ch, "Item looks to be worth about %ld.\r\n", cost);
	else
		send_to_char(ch, "Item doesn't look to be worth anything.\r\n");

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_SCROLL:
	case ITEM_POTION:
		if (skill_lvl > 80) {
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);

			if (GET_OBJ_VAL(obj, 1) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
		}
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		if (skill_lvl > 80) {
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);
			send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
			if (skill_lvl > 90)
				send_to_char(ch, "It has %d maximum charge%s and %d remaining.\r\n",
					GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
					GET_OBJ_VAL(obj, 2));
		}
		break;
	case ITEM_WEAPON:
		send_to_char(ch, "This weapon can deal up to %d points of damage.\r\n",
			GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 1));
			
		if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
			send_to_char(ch, "This weapon casts an offensive spell.\r\n");
		break;
	case ITEM_ARMOR:
		if (GET_OBJ_VAL(obj, 0) < 2)
			send_to_char(ch, "This armor provides hardly any protection.\r\n");
		else if (GET_OBJ_VAL(obj, 0) < 5)
			send_to_char(ch, "This armor provides a little protection.\r\n");
		else if (GET_OBJ_VAL(obj, 0) < 15)
			send_to_char(ch, "This armor provides some protection.\r\n");
		else if (GET_OBJ_VAL(obj, 0) < 20)
			send_to_char(ch, "This armor provides a lot of protection.\r\n");
		else if (GET_OBJ_VAL(obj, 0) < 25)
			send_to_char(ch, "This armor provides a ridiculous amount of protection.\r\n");
		else
			send_to_char(ch, "This armor provides an insane amount of protection.\r\n");
		break;
	case ITEM_CONTAINER:
		send_to_char(ch, "This container holds a maximum of %d pounds.\r\n",
			GET_OBJ_VAL(obj, 0));
		break;
	case ITEM_FOUNTAIN:
	case ITEM_DRINKCON:
		send_to_char(ch, "This container holds some %s\r\n",
			drinks[GET_OBJ_VAL(obj, 2)]);
	}

	for (i = 0;i < MIN(MAX_OBJ_AFFECT, skill_lvl / 25); i++) {
		if (obj->affected[i].location != APPLY_NONE) {
			sprinttype(obj->affected[i].location, apply_types, buf2);
			if (obj->affected[i].modifier > 0)
				send_to_char(ch, "Item increases %s\r\n", buf2);
			else if (obj->affected[i].modifier < 0)
				send_to_char(ch, "Item decreases %s\r\n", buf2);
		}
	}
}

ACMD(do_appraise)
{
	struct obj_data *obj = NULL;	// the object that will be emptied 
	struct Creature *dummy = NULL;
	int bits;
	char *arg;

	arg = tmp_getword(&argument);

	if (!*arg) {
		send_to_char(ch, "You have to appraise something.\r\n");
		return;
	}

	if (CHECK_SKILL(ch, SKILL_APPRAISE) < 10) {
		send_to_char(ch, "You have no idea how.\r\n");
		return;
	}

	if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &obj))) {
		send_to_char(ch, "You can't find any %s to appraise\r\n.", arg);
		return;
	}

	perform_appraise(ch, obj, CHECK_SKILL(ch, SKILL_APPRAISE));
}
