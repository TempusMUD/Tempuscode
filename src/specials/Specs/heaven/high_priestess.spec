//
// File: high_priestess.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(high_priestess)
{

	struct Creature *hpr = (struct Creature *)me, *archon = NULL, *vict = NULL;
	struct room_data *focus = real_room(11173), *quarters = real_room(43252);

	if (cmd)
		return 0;
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (focus == NULL || quarters == NULL) {
		errlog("High Priestess room is bunk. Removing spec");
		hpr->mob_specials.shared->func = NULL;
		REMOVE_BIT(MOB_FLAGS(hpr), MOB_SPEC);
		return 0;
	}

	if (hpr->numCombatants() && hpr->in_room != quarters) {
		if (GET_HIT(hpr) > 200)
			return 0;

		act("There is a blinding flash of light!!\r\n"
			"$n disappears in a thunderclap!", FALSE, hpr, 0, 0, TO_ROOM);
        hpr->removeAllCombat();

		if ((archon = read_mobile(43014))) {
			char_to_room(archon, hpr->in_room, false);
			act("$n appears at the center of the room.", FALSE, archon, 0, 0,
				TO_ROOM);
		}
		if ((archon = read_mobile(43015))) {
			char_to_room(archon, hpr->in_room, false);
			act("$n appears at the center of the room.", FALSE, archon, 0, 0,
				TO_ROOM);
			hit(archon, vict, TYPE_UNDEFINED);
		}

		char_from_room(hpr, false);
		char_to_room(hpr, quarters, false);
		act("$n appears at the center of the room.", TRUE, hpr, 0, 0, TO_ROOM);
		return 1;
	}

	if (!hpr->numCombatants() && hpr->in_room == quarters &&
		GET_HIT(hpr) == GET_MAX_HIT(hpr) &&
		GET_MANA(hpr) > (GET_MAX_MANA(hpr) * 0.75)) {
		act("$n steps onto a flaming golden chariot and disappears into the sky.", FALSE, hpr, 0, 0, TO_ROOM);
		char_from_room(hpr, false);
		char_to_room(hpr, focus, false);
		act("A dazzling shaft of light appears from the heavens, bathing the throne.\r\n$n slowly appears, seated at the focus.", FALSE, hpr, 0, 0, TO_ROOM);
		return 1;
	}

	return 0;
}
