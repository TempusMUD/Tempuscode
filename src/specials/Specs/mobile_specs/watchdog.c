//
// File: watchdog.spec                                         -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// As people stand in the room, watchdog gets more and more irritated.  When
// he sees someone ugly enough, he snaps.

SPECIAL(watchdog)
{
	struct creature *dog = (struct creature *)me;
	struct creature *vict = NULL;
	static byte indignation = 0;
	struct creatureList_iterator it;

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;

	if (cmd || !AWAKE(dog) || dog->isFighting())
		return 0;

	for (it = dog->in_room->people.begin(); it != ch->in_room->people.end();
		++it) {
		if (*it != dog && can_see_creature(dog, (*it)) && GET_LEVEL((*it)) < LVL_IMMORT) {
			vict = *it;
			break;
		}
	}

	if (!vict || vict == dog)
		return 0;

	indignation++;

	if (vict) {
		switch (number(0, 4)) {
		case 0:
			act("$n growls menacingly at $N.", false, dog, 0, vict,
				TO_NOTVICT);
			act("$n growls menacingly at you.", false, dog, 0, vict, TO_VICT);
			break;
		case 1:
			act("$n barks loudly at $N.", false, dog, 0, vict, TO_NOTVICT);
			act("$n barks loudly at you.", false, dog, 0, vict, TO_VICT);
			break;
		case 2:
			act("$n growls at $N.", false, dog, 0, vict, TO_NOTVICT);
			act("$n growls at you.", false, dog, 0, vict, TO_VICT);
			break;
		case 3:
			act("$n snarls at $N.", false, dog, 0, vict, TO_NOTVICT);
			act("$n snarls at you.", false, dog, 0, vict, TO_VICT);
			break;
		default:
			break;
		}

		if (indignation > (GET_CHA(vict) >> 2)) {
			hit(dog, vict, TYPE_UNDEFINED);
			indignation = 0;
			vict = NULL;
		}
		return 1;
	}
	return 0;
}
