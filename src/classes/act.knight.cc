//
// File: act.knight.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.knight.c
//

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "house.h"
#include "char_class.h"
#include "creature.h"
#include "fight.h"

int holytouch_after_effect(long owner, Creature * vict, int level);
void healing_holytouch(Creature * ch, Creature * vict);
void malovent_holy_touch(Creature * ch, Creature * vict);
ACMD(do_holytouch)
{
	struct Creature *vict = NULL;
	char vict_name[MAX_INPUT_LENGTH];
	one_argument(argument, vict_name);

	if (CHECK_SKILL(ch, SKILL_HOLY_TOUCH) < 40 || IS_NEUTRAL(ch)) {
		send_to_char(ch, "You are unable to call upon the powers necessary.\r\n");
		return;
	}

	if (!*vict_name)
		vict = ch;
	else if (!(vict = get_char_room_vis(ch, vict_name))) {
		send_to_char(ch, "Holytouch who?\r\n");
		return;
	}

	if (vict == NULL)
		return;

	if ((NON_CORPOREAL_MOB(vict) || NULL_PSI(vict))) {
		send_to_char(ch, "That is unlikely to work.\r\n");
		return;
	}

	if (IS_EVIL(ch) && ch != vict) {
		if (!IS_GOOD(vict))
			healing_holytouch(ch, vict);
		else
			malovent_holy_touch(ch, vict);
	} else {
		healing_holytouch(ch, vict);
	}
}

/*
    Called when holytouch "wears off"
    vict makes a saving throw modified by thier align (the higher the worse)
    if success, nothing happens.
    if failed, victim tries to gouge thier own eyes out
        - remove face and eyewear
        - falls to knees screaming
        - eyeballs appear on death
*/
int
holytouch_after_effect(long owner, Creature * vict, int level)
{
	int dam = level * 2;

	send_to_char(vict, "Visions of pure evil sear through your mind!\r\n");
	if (vict->getPosition() > POS_SITTING) {
		act("$n falls to $s knees screaming!", TRUE, vict, 0, 0, TO_ROOM);
		vict->setPosition(POS_SITTING);
	} else {
		act("$n begins to scream!", TRUE, vict, 0, 0, TO_ROOM);
	}
	WAIT_STATE(vict, 1 RL_SEC);
	if (GET_EQ(vict, WEAR_FACE))
		obj_to_char(unequip_char(vict, WEAR_FACE, MODE_EQ), vict);
	if (GET_EQ(vict, WEAR_EYES))
		obj_to_char(unequip_char(vict, WEAR_EYES, MODE_EQ), vict);

	if (damage(vict, vict, dam, TYPE_MALOVENT_HOLYTOUCH, WEAR_EYES))
		return 1;
	if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_NOBLIND)) {
        affected_type af;

        af.next = NULL;
		af.type = TYPE_MALOVENT_HOLYTOUCH;
		af.duration = level / 10;
		af.modifier = -(level / 5);
		af.location = APPLY_HITROLL;
		af.bitvector = AFF_BLIND;
        af.aff_index = 0;
		af.is_instant = 0;
        af.owner = owner;

        affect_to_char(vict, &af);
	}

	return 0;
}

/*
    Initial "bad" holytouch effect.  Happens when evil holytouch's good.
 */
void
malovent_holy_touch(Creature * ch, Creature * vict)
{
	int chance = 0;
	struct affected_type af;
	af.type = af.duration = af.modifier = af.location = 0;
	af.level = af.is_instant = af.aff_index = 0;
	af.next = NULL;
    af.owner = 0;

	if (GET_LEVEL(vict) > LVL_AMBASSADOR) {
		send_to_char(ch, "Aren't they evil enough already?\r\n");
		return;
	}

	if (affected_by_spell(vict, SKILL_HOLY_TOUCH)
		|| affected_by_spell(vict, TYPE_MALOVENT_HOLYTOUCH)) {
		act("There is nothing more you can show $N.", FALSE, ch, 0, vict,
			TO_CHAR);
		return;
	}

	if (!ch->isOkToAttack(vict))
		return;

	chance = GET_ALIGNMENT(vict) / 10;
	if (GET_WIS(vict) > 20)
		chance -= ( GET_WIS(vict) - 20) * 5;
	if (AFF_FLAGGED(vict, AFF_PROTECT_EVIL))
		chance -= 20;
	if (affected_by_spell(vict, SPELL_BLESS))
		chance -= 10;
	if (affected_by_spell(vict, SPELL_PRAY))
		chance -= 10;

	if (IS_SOULLESS(ch))
		chance += 25;

	WAIT_STATE(ch, 2 RL_SEC);
	check_killer(ch, vict);

	int roll = random_percentage_zero_low();

	if (PRF2_FLAGGED(ch, PRF2_DEBUG))
		send_to_char(ch, "%s[HOLYTOUCH] %s   roll:%d   chance:%d%s\r\n",
			CCCYN(ch, C_NRM), GET_NAME(ch), roll, chance, CCNRM(ch, C_NRM));
	if (PRF2_FLAGGED(vict, PRF2_DEBUG))
		send_to_char(vict, "%s[HOLYTOUCH] %s   roll:%d   chance:%d%s\r\n",
			CCCYN(vict, C_NRM), GET_NAME(ch), roll, chance, CCNRM(vict, C_NRM));

	if (roll > chance) {
		damage(ch, vict, 0, SKILL_HOLY_TOUCH, 0);
		return;
	}

	af.type = SKILL_HOLY_TOUCH;
	af.is_instant = 1;
	af.level = ch->getLevelBonus(SKILL_HOLY_TOUCH);
	af.duration = number(1, 3);
	af.aff_index = 3;
	af.bitvector = AFF3_INST_AFF;
    af.owner = ch->getIdNum();

	if (IS_SOULLESS(ch))
		af.level += 20;

	affect_to_char(vict, &af);

	if (GET_LEVEL(ch) < LVL_AMBASSADOR)
		WAIT_STATE(ch, PULSE_VIOLENCE);

	gain_skill_prof(ch, SKILL_HOLY_TOUCH);
	if (damage(ch, vict, ch->getLevelBonus(SKILL_HOLY_TOUCH) * 2,
			SKILL_HOLY_TOUCH, WEAR_EYES))
		return;

}

void
healing_holytouch(Creature * ch, Creature * vict)
{
	int mod, gen;

	gen = GET_REMORT_GEN(ch);
	mod = (GET_LEVEL(ch) + gen + CHECK_SKILL(ch, SKILL_HOLY_TOUCH) / 16);

	if (GET_MOVE(ch) > mod) {
		if (GET_MANA(ch) < 5) {
			send_to_char(ch, "You are too spiritually exhausted.\r\n");
			return;
		}
		GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + mod);
		GET_MANA(ch) = MAX(0, GET_MANA(ch) - (mod / 2));
		GET_MOVE(ch) = MAX(GET_MOVE(ch) - mod, 0);
		if (ch == vict) {
			send_to_char(ch, "You cover your head with your hands and pray.\r\n");
			act("$n covers $s head with $s hands and prays.", true, ch, 0, 0,
				TO_ROOM);
		} else {
			act("$N places $S hands on your head and prays.", FALSE, vict, 0,
				ch, TO_CHAR);
			act("$n places $s hands on the head of $N.", FALSE, ch, 0, vict,
				TO_NOTVICT);
			send_to_char(ch, "You do it.\r\n");
		}
		if (GET_LEVEL(ch) < LVL_AMBASSADOR)
			WAIT_STATE(ch, PULSE_VIOLENCE);

		if (IS_GOOD(ch)) {
			if (gen > 0 && affected_by_spell(vict, TYPE_MALOVENT_HOLYTOUCH))
				affect_from_char(vict, TYPE_MALOVENT_HOLYTOUCH);
			if (gen > 1) {
				if (affected_by_spell(vict, SPELL_SICKNESS))
					affect_from_char(vict, SPELL_SICKNESS);
				REMOVE_BIT(AFF2_FLAGS(vict), AFF3_SICKNESS);
			}
			if (gen > 2 && affected_by_spell(vict, SPELL_POISON))
				affect_from_char(vict, SPELL_POISON);
			if (gen > 3 && affected_by_spell(vict, TYPE_RAD_SICKNESS))
				affect_from_char(vict, TYPE_RAD_SICKNESS);
			if (gen > 4 && affected_by_spell(vict, SPELL_BLINDNESS))
				affect_from_char(vict, SPELL_BLINDNESS);
			if (gen > 5 && affected_by_spell(vict, SKILL_GOUGE))
				affect_from_char(vict, SKILL_GOUGE);
			if (gen > 6 && affected_by_spell(vict, SKILL_HAMSTRING))
				affect_from_char(vict, SKILL_HAMSTRING);
			if (gen > 7 && affected_by_spell(vict, SPELL_CURSE))
				affect_from_char(vict, SPELL_CURSE);
			if (gen > 8 && affected_by_spell(vict, SPELL_MOTOR_SPASM))
				affect_from_char(vict, SPELL_MOTOR_SPASM);
			if (gen > 9) {
				if (affected_by_spell(vict, SPELL_PETRIFY))
					affect_from_char(vict, SPELL_PETRIFY);
				REMOVE_BIT(AFF2_FLAGS(vict), AFF2_PETRIFIED);
			}
		}
	} else {
		send_to_char(ch, "You must rest awhile before doing this again.\r\n");
	}
}

#undef __act_knight_c__
