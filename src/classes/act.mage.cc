//
// File: act.mage.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define __act_mage_c__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "house.h"
#include "char_class.h"

#define MSHIELD_USAGE "usage: mshield <low|percent> <value>\r\n"
ACMD(do_mshield)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int i = 0;

	if (!affected_by_spell(ch, SPELL_MANA_SHIELD)) {
		send_to_char("You are not under the affects of a mana shield.\r\n",
			ch);
		return;
	}

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(MSHIELD_USAGE, ch);
		return;
	}

	strcpy(buf, "Manashield ");

	if (is_abbrev(arg1, "low")) {
		i = atoi(arg2);
		if (i < 0 || i > GET_MANA(ch)) {
			send_to_char
				("The low must be between 0 and your current mana.\r\n", ch);
			return;
		}
		GET_MSHIELD_LOW(ch) = i;
		strcat(buf, "low ");
	} else if (is_abbrev(arg1, "percent")) {
		i = atoi(arg2);
		if (i < 1 || i > 100) {
			send_to_char("The percent must be between 1 and 100.\r\n", ch);
			return;
		}
		if (GET_CLASS(ch) != CLASS_MAGE && i > 50) {
			send_to_char("You cannot set the percent higher than 50.\r\n", ch);
			return;
		}
		GET_MSHIELD_PCT(ch) = i;
		strcat(buf, "percent ");
	}

	sprintf(buf, "%sset to [%d].\r\n", buf, i);
	send_to_char(buf, ch);
}

ACMD(do_empower)
{
	struct affected_type af, af2, af3;
	int val1, val2, old_mana, old_maxmana;

	if (!IS_MAGE(ch) || CHECK_SKILL(ch, SKILL_EMPOWER) < number(50, 101)) {
		send_to_char("You fail to empower yourself.\r\n", ch);
		return;
	}
	if (GET_HIT(ch) < 15 || GET_MOVE(ch) < 15) {
		send_to_char("You are unable to call upon any resources of power!\r\n",
			ch);
		return;
	}
	old_maxmana = GET_MAX_MANA(ch);
	old_mana = GET_MANA(ch);
	val1 = MIN(GET_LEVEL(ch), (GET_HIT(ch) >> 2));
	val2 = MIN(GET_LEVEL(ch), (GET_MOVE(ch) >> 2));

	af.level = af2.level = af3.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

	af.bitvector = 0;
	af2.bitvector = 0;
	af3.bitvector = 0;
	af.type = SKILL_EMPOWER;
	af2.type = SKILL_EMPOWER;
	af3.type = SKILL_EMPOWER;
	af.duration = (GET_INT(ch) >> 1);
	af.location = APPLY_MANA;
	af.modifier = (val1 + val2 - 5);
	af2.duration = (GET_INT(ch) >> 1);
	af2.location = APPLY_HIT;
	af2.modifier = -(val1);
	af3.duration = (GET_INT(ch) >> 1);
	af3.location = APPLY_MOVE;
	af3.modifier = -(val2);
	affect_join(ch, &af, 0, 0, 1, 0);

	if (GET_MAX_MANA(ch) > old_maxmana) {	// Success!
		GET_MANA(ch) = MIN((old_mana + val2 + val1), GET_MAX_MANA(ch));
		send_to_char("You redirect your energies!\r\n", ch);
		affect_join(ch, &af2, 0, 0, 1, 0);
		affect_join(ch, &af3, 0, 0, 1, 0);

		if ((GET_MAX_HIT(ch) >> 1) < GET_WIMP_LEV(ch)) {
			sprintf(buf,
				"Your wimpy level has been changed from %d to %d ... wimp!\r\n",
				GET_WIMP_LEV(ch), GET_MAX_HIT(ch) >> 1);
			send_to_char(buf, ch);
			GET_WIMP_LEV(ch) = GET_MAX_HIT(ch) >> 1;
		}
	} else {					// hrm. OOps.. Ran outta gas.
		GET_MANA(ch) = old_mana;
		send_to_char("You are already fully empowered.\r\n", ch);
	}
	act("$n concentrates deeply.", TRUE, ch, 0, 0, TO_ROOM);
	if (GET_LEVEL(ch) < LVL_GRGOD)
		WAIT_STATE(ch, PULSE_VIOLENCE * (1 + ((val1 + val2) > 100)));
}

#undef __act_mage_c__
