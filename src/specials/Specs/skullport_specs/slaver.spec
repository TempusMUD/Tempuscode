//
// File: slaver.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define SLAVE_PIT 22626
#define PIT_LIP   22945

SPECIAL(slaver)
{

	struct Creature *slaver = (struct Creature *)me;
	struct Creature *vict = NULL;
	struct room_data *r_pit_lip = real_room(PIT_LIP),
		*r_slave_pit = real_room(SLAVE_PIT);

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || !AWAKE(slaver))
		return 0;

	if (!r_pit_lip || !r_slave_pit)
		return 0;

	if (!slaver->numCombatants()) {

		CreatureList::iterator it = slaver->in_room->people.begin();
		for (; it != slaver->in_room->people.end(); ++it) {
			vict = *it;
			if (ch != vict && !IS_NPC(vict) && can_see_creature(slaver, vict) &&
				PRF_FLAGGED(vict, PRF_NOHASSLE) &&
				GET_LEVEL(vict) < number(1, 50) && !IS_EVIL(vict)) {
				do_say(slaver, "Ha!  You're coming with me!", 0, 0, 0);

				if (slaver->in_room == r_pit_lip) {
					act("$n hurls you headfirst into the slave pit!",
						FALSE, slaver, 0, vict, TO_VICT);
					act("$n hurls $N headfirst into the slave pit!",
						FALSE, slaver, 0, vict, TO_NOTVICT);
					slaver->removeCombat(vict);
					vict->removeCombat(slaver);
					char_from_room(vict, false);
					char_to_room(vict, r_slave_pit, false);
					look_at_room(vict, vict->in_room, 1);
					act("$n is hurled into the pit from above!",
						FALSE, vict, 0, 0, TO_ROOM);
					return 1;
				} else
					return (drag_char_to_jail(slaver, vict, r_pit_lip));
			}
		}
		return 0;
	}

	if (!(vict = slaver->findRandomCombat()) ||
		PRF_FLAGGED(vict, PRF_NOHASSLE) || !can_see_creature(slaver, vict))
		return 0;

	if (GET_MOB_WAIT(slaver) <= 5 && !number(0, 1)) {

		if (slaver->in_room == r_pit_lip) {
			act("$n hurls you headfirst into the slave pit!",
				FALSE, slaver, 0, vict, TO_VICT);
			act("$n hurls $N headfirst into the slave pit!",
				FALSE, slaver, 0, vict, TO_NOTVICT);
			slaver->removeCombat(vict);
			vict->removeCombat(slaver);
			char_from_room(vict, false);
			char_to_room(vict, r_slave_pit, false);
			look_at_room(vict, vict->in_room, 1);
			act("$n is hurled into the pit from above!",
				FALSE, vict, 0, 0, TO_ROOM);
			return 1;
		} else
			return (drag_char_to_jail(slaver, vict, r_pit_lip));
	}

	return 0;
}
