//
// File: act.mage.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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
#include "language.h"

#define MSHIELD_USAGE "usage: mshield <low|percent> <value>\r\n"
ACMD(do_mshield)
{
    char *arg1, *arg2;
	int i = 0;

	if (!affected_by_spell(ch, SPELL_MANA_SHIELD)) {
		send_to_char(ch, "You are not under the affects of a mana shield.\r\n");
		return;
	}

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, MSHIELD_USAGE);
		return;
	}

	if (is_abbrev(arg1, "low")) {
		i = atoi(arg2);
		if (i < 0 || i > GET_MANA(ch)) {
			send_to_char(ch, 
				"The low must be between 0 and your current mana.\r\n");
			return;
		}
		GET_MSHIELD_LOW(ch) = i;
        send_to_char(ch, "Manashield low set to [%d].\r\n", i);
	} else if (is_abbrev(arg1, "percent")) {
		i = atoi(arg2);
		if (i < 1 || i > 100) {
			send_to_char(ch, "The percent must be between 1 and 100.\r\n");
			return;
		}
		if (GET_CLASS(ch) != CLASS_MAGE && i > 50) {
			send_to_char(ch, "You cannot set the percent higher than 50.\r\n");
			return;
		}
		GET_MSHIELD_PCT(ch) = i;
        send_to_char(ch, "Manashield percent set to [%d].\r\n", i);
	} else {
        send_to_char(ch, MSHIELD_USAGE);
    }

}

ACMD(do_empower)
{
	struct affected_type af, af2, af3;
	int val1, val2, old_mana, old_maxmana;

	if (!IS_MAGE(ch) || CHECK_SKILL(ch, SKILL_EMPOWER) < number(50, 101)) {
		send_to_char(ch, "You fail to empower yourself.\r\n");
		return;
	}
	if (GET_HIT(ch) < 15 || GET_MOVE(ch) < 15) {
		send_to_char(ch, "You are unable to call upon any resources of power!\r\n");
		return;
	}
	old_maxmana = GET_MAX_MANA(ch);
	old_mana = GET_MANA(ch);
	val1 = MIN(GET_LEVEL(ch), (GET_HIT(ch) >> 2));
	val2 = MIN(GET_LEVEL(ch), (GET_MOVE(ch) >> 2));

	af.level = af2.level = af3.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
	af.bitvector = af2.bitvector = af3.bitvector = 0;
	af.is_instant = af2.is_instant = af3.is_instant = false;

	af.type = SKILL_EMPOWER;
	af.duration = (GET_INT(ch) >> 1);
	af.location = APPLY_MANA;
	af.modifier = (val1 + val2 - 5);
    af.owner = ch->getIdNum();

	af2.type = SKILL_EMPOWER;
	af2.duration = (GET_INT(ch) >> 1);
	af2.location = APPLY_HIT;
	af2.modifier = -(val1);
    af2.owner = ch->getIdNum();
    
	af3.type = SKILL_EMPOWER;
	af3.duration = (GET_INT(ch) >> 1);
	af3.location = APPLY_MOVE;
	af3.modifier = -(val2);
    af3.owner = ch->getIdNum();

	struct affected_type *cur_aff;
	int aff_power;

	for (cur_aff = ch->affected;cur_aff;cur_aff = cur_aff->next) {
		if (cur_aff->type == SKILL_EMPOWER
				&& cur_aff->location == APPLY_MANA) {
			aff_power = cur_aff->modifier + val1 + val2 - 5;
			if (aff_power > 666) {
				send_to_char(ch, "You are already fully empowered.\r\n");
				return;
			}
		}
	}
	affect_join(ch, &af, 0, 0, 1, 0);

	GET_MANA(ch) = MIN((old_mana + val2 + val1), GET_MAX_MANA(ch));
	send_to_char(ch, "You redirect your energies!\r\n");
	affect_join(ch, &af2, 0, 0, 1, 0);
	affect_join(ch, &af3, 0, 0, 1, 0);

	if ((GET_MAX_HIT(ch) >> 1) < GET_WIMP_LEV(ch)) {
		send_to_char(ch,
			"Your wimpy level has been changed from %d to %d ... wimp!\r\n",
			GET_WIMP_LEV(ch), GET_MAX_HIT(ch) >> 1);
		GET_WIMP_LEV(ch) = GET_MAX_HIT(ch) >> 1;
	}
	act("$n concentrates deeply.", TRUE, ch, 0, 0, TO_ROOM);
	if (GET_LEVEL(ch) < LVL_GRGOD)
		WAIT_STATE(ch, PULSE_VIOLENCE * (1 + ((val1 + val2) > 100)));
}

// It's kind of silly to put this here, but I couldn't think of a
// better spot
ACMD(do_teach)
{
    Creature *target;
    char *s;
    char *skill_str;
    const char *skill_name;
    int num;
    bool is_skill;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char(ch, "Usage: teach <skill/spell/language> <player>\r\n");
        return;
    }

    // There must be exactly 0 or exactly 2 's in the argument
    if ((s = strstr(argument, "\'"))) {
        if (!(strstr(s + 1, "\'"))) {
            send_to_char(ch, "The skill name must be completely enclosed "
                    "in the symbols: '\n");
            return;
        } else {
            skill_str = tmp_getquoted(&argument);
        }
    } else {
        s = strrchr(argument, ' ');
        if (!s) {
            send_to_char(ch, "You must specify who to teach!\r\n");
            return;
        }

        *s = '\0';
        skill_str = argument;
        argument = s + 1;
    }

    target = get_char_room_vis(ch, tmp_getword(&argument));
    if (!target) {
        send_to_char(ch, "You don't see anyone here named '%s'.\r\n", argument);
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char(ch, "You can't teach PCs!\r\n");
        return;
    }
    if (IS_NPC(target)) {
        send_to_char(ch, "You can't teach NPCs a thing.\r\n");
        return;
    }

    if (ch == target) {
        send_to_char(ch, "You teach yourself with experience, not lessons.\r\n");
        return;
    }

    num = find_skill_num(skill_str);
    if (num != -1) {
        is_skill = true;
        skill_name = spell_to_str(num);
        if (!ABLE_TO_LEARN(target, num)) {
            act(tmp_sprintf("$E isn't ready to learn '%s'.", skill_name),
                false, ch, 0, target, TO_CHAR);
            return;
        }
        if (CHECK_SKILL(target, num) > CHECK_SKILL(ch, num) / 2) {
            act(tmp_sprintf("You don't have anything to teach about '%s' that $E doesn't already know.", skill_name),
                false, ch, 0, target, TO_CHAR);
            return;
        }

    } else if ((num = find_tongue_idx_by_name(skill_str)) != TONGUE_NONE) {
        is_skill = false;
        skill_name = tongue_name(num);
        if (CHECK_TONGUE(target, num) > CHECK_TONGUE(ch, num) / 2) {
            act(tmp_sprintf("You don't have anything to teach about '%s' that $e doesn't already know.", skill_name),
                false, ch, 0, target, TO_CHAR);
            return;
        }
    } else {
        send_to_char(ch, "You don't know of any such ability.\r\n");
        return;
    }
    
    WAIT_STATE(ch, 2 RL_SEC);

    if (number(0, 100) > GET_WIS(ch) + GET_INT(ch) + GET_WIS(target) + GET_INT(target)) {
        // Teaching failure
        act(tmp_sprintf("You try to teach '%s' to $N, but $E doesn't seem to get it.", skill_name),
            false,
            ch, 0, target, TO_CHAR);
        act(tmp_sprintf("$n tries to teach you '%s', but you don't really understand the lesson.", skill_name),
            false,
            ch, 0, target, TO_VICT);
        act("$n gives a lesson to $N.", false, ch, 0, target, TO_NOTVICT);
        return;
    }

    // Teaching success
    if (is_skill) {
        SET_SKILL(target, num, CHECK_SKILL(target, num) + number(1, GET_INT(target)));
    } else {
        SET_TONGUE(target, num, CHECK_TONGUE(target, num) + number(1, GET_INT(target)));
    }

    act(tmp_sprintf("You give a quick lesson to $N on '%s'.", skill_name),
        false, ch, 0, target, TO_CHAR);
    act(tmp_sprintf("$n gives you a quick lesson on '%s'.", skill_name),
        false, ch, 0, target, TO_VICT);
    act("$n gives a lesson to $N.", false, ch, 0, target, TO_NOTVICT);
}

#undef __act_mage_c__
