//
// File: phantasmic_sword.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(phantasmic_sword)
{
	struct affected_type af;
	struct creature *mast = NULL;
    struct creature *self = (struct creature *)me;

	if (spec_mode != SPECIAL_TICK)
		return 0;
	if (ch->fighting) {
		if (!number(0, 8)) {
			act("$n starts to emit a piercing whine!", false, ch, 0, 0,
				TO_ROOM);
			af.type = SPELL_STRENGTH;
			if (!AFF_FLAGGED(ch, AFF_ADRENALINE))
				af.bitvector = AFF_ADRENALINE;
			else if (!AFF2_FLAGGED(ch, AFF2_HASTE)) {
				af.bitvector = AFF2_HASTE;
				af.aff_index = 2;
			} else
				af.bitvector = 0;
			af.modifier = 1;
			af.location = APPLY_STR;
			af.duration = 2;
            af.owner = self->getIdNum();
			if (GET_STR(ch) > 21)
				return 1;
			affect_to_char(ch, &af);
			return 1;
		}
		return 0;
	} else {

		if ((!(mast = ch->master) || !AFF_FLAGGED(ch, AFF_CHARM))
			&& !ch->fighting) {
			if (ch->in_room == NULL)
				return 0;
			if (!number(0, 4)) {
				act("$n departs for the ethereal plane!", true, ch, 0, 0,
					TO_ROOM);
				ch->purge(true);
				return 1;
			} else
				return 0;
		}

		if (AWAKE(ch)) {
			struct creatureList_iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (*it != ch && IS_NPC((*it)) &&
					GET_MOB_VNUM(ch) == GET_MOB_VNUM((*it)) &&
					!(number(0, GET_LEVEL(mast) +
							(GET_CHA(mast) >> (mast->in_room !=
									ch->in_room))))) {
					//set_fighting(ch, *it, true);
                    ch->addCombat(*it, true);
                    (*it)->addCombat(ch, false);
					return 0;
				}
			}

		}
		if (mast->in_room != ch->in_room)
			return 0;

		switch (number(0, 70)) {
		case 0:
			act("$n urges you to join battle.", false, ch, 0, mast, TO_VICT);
			act("$n urges $N to join battle.", false, ch, 0, mast, TO_NOTVICT);
			break;
		case 1:
			act("$n thrums faintly.", false, ch, 0, 0, TO_ROOM);
			break;
		case 2:
			act("$n whispers, 'I am hungry for blood...'", false, ch, 0, 0,
				TO_ROOM);
			break;
		case 3:
			act("$n floats slowly past your throat.", true, ch, 0, mast,
				TO_VICT);
			act("$n floats slowly past $N's throat.", true, ch, 0, mast,
				TO_NOTVICT);
			break;
		case 4:
			act("$n emits a humming sound.", false, ch, 0, 0, TO_ROOM);
			break;
		case 5:
			act("$n fades out of reality for a moment.", true, ch, 0, 0,
				TO_ROOM);
			break;
		case 6:
			act("$n starts moaning eerily.", false, ch, 0, 0, TO_ROOM);
			break;
		default:
			return 0;
		}
		return 1;
	}
}