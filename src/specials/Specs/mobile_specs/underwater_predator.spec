//
// File: underwater_predator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(underwater_predator)
{

	struct Creature *pred = (struct Creature *)me;
	struct Creature *vict = NULL;
	struct room_data *troom = NULL;
	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (pred->getPosition() < POS_RESTING ||
		AFF2_FLAGGED(pred, AFF2_PETRIFIED) || !AWAKE(pred))
		return 0;

	if ((vict = pred->findRandomCombat())
		&& SECT_TYPE(pred->in_room) != SECT_UNDERWATER
		&& SECT_TYPE(pred->in_room) != SECT_DEEP_OCEAN
		&& !number(0, 3)) {

		if (EXIT(pred, DOWN) &&
			((troom = EXIT(pred, DOWN)->to_room) != NULL) &&
			(SECT_TYPE(troom) == SECT_UNDERWATER
				|| SECT_TYPE(troom) == SECT_DEEP_OCEAN) &&
			!ROOM_FLAGGED(troom, ROOM_GODROOM | ROOM_DEATH | ROOM_PEACEFUL)) {

			if ((STRENGTH_APPLY_INDEX(pred) + number(1, 6) >
					STRENGTH_APPLY_INDEX(vict))) {

				act("$n drags you under!!!", FALSE, pred, 0, vict, TO_VICT);
				act("$n drags $N under!!!", FALSE, pred, 0, vict, TO_NOTVICT);
				char_from_room(pred, false);
				char_to_room(pred, troom, false);
				char_from_room(vict, false);
				char_to_room(vict, troom, false);
				look_at_room(vict, vict->in_room, 0);

				act("$N is dragged, struggling,  in from above by $n!!",
					FALSE, pred, 0, vict, TO_NOTVICT);

				WAIT_STATE(pred, PULSE_VIOLENCE);
				return 1;
			}
		}
	} else if (!pred->numCombatants() && MOB_FLAGGED(pred, MOB_AGGRESSIVE) &&
		(SECT_TYPE(pred->in_room) == SECT_UNDERWATER
			|| SECT_TYPE(pred->in_room) == SECT_DEEP_OCEAN) &&
		EXIT(pred, UP) &&
		((troom = EXIT(pred, UP)->to_room) != NULL) &&
		!ROOM_FLAGGED(troom, ROOM_NOMOB | ROOM_PEACEFUL | 
                      ROOM_DEATH | ROOM_GODROOM)) {
		CreatureList::iterator it = troom->people.begin();
		for (; it != troom->people.end(); ++it) {
			vict = *it;
			if ((IS_MOB(vict) && !MOB2_FLAGGED(pred, MOB2_ATK_MOBS)) ||
				(!IS_NPC(vict) && !vict->desc) ||
				PRF_FLAGGED(vict, PRF_NOHASSLE) || !can_see_creature(pred, vict) ||
				PLR_FLAGGED(vict, PLR_OLC | PLR_WRITING | PLR_MAILING) ||
				vict->getPosition() == POS_FLYING ||
				(vict->isMounted()
					&& (vict->isMounted())->getPosition() == POS_FLYING))
				continue;

			act("$n cruises up out of sight with deadly intention.",
				TRUE, pred, 0, 0, TO_ROOM);
			char_from_room(pred, false);

			char_to_room(pred, troom, false);
			act("$n emerges from the depths and attacks!!!",
				TRUE, pred, 0, 0, TO_ROOM);

			hit(pred, vict, TYPE_UNDEFINED);
			return 1;
		}
	}
	return 0;
}
