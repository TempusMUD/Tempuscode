//
// File: healing_ranger.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(healing_ranger)
{
	struct creature *vict = NULL;
	int found = 0;
	ACMD(do_bandage);
	ACMD(do_firstaid);
	ACMD(do_medic);

	if (spec_mode != SPECIAL_TICK)
		return false;

	if (cmd || ch->isFighting())
		return false;

	switch (number(0, 25)) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:{
			found = false;
			struct creatureList_iterator it = ch->in_room->people.begin();
			struct creatureList_iterator nit = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && !found; ++it) {
				++nit;
				vict = *it;
				if (ch == vict || (nit != ch->in_room->people.end()
						&& !number(0, 2))) {
					continue;
				}
				if (!IS_NPC(vict) && can_see_creature(ch, vict) &&
					(GET_HIT(vict) < GET_MAX_HIT(vict))) {
					if (GET_MOVE(ch) > 50) {
						if (GET_LEVEL(vict) <= 15)
							do_bandage(ch, GET_NAME(vict), 0, 0, 0);
						else if (GET_LEVEL(vict) <= 30)
							do_firstaid(ch, GET_NAME(vict), 0, 0, 0);
						else
							do_medic(ch, GET_NAME(vict), 0, 0, 0);

						return true;
					}
				}
			}
			break;
		}
	case 14:
	case 15:
	case 16:{
			found = false;
			struct creatureList_iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && !found; ++it) {
				if (AFF_FLAGGED((*it), AFF_POISON)) {
					if (GET_MANA(ch) > 50) {
						cast_spell(ch, *it, 0, NULL, SPELL_REMOVE_POISON);
						return true;
					}
				}
			}
			break;
		}
	case 17:
	case 18:{
			found = false;
			found = false;
			struct creatureList_iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end() && !found; ++it)
				vict = *it;
			if (!IS_NPC(vict) && (affected_by_spell(vict, SPELL_BLINDNESS) ||
					affected_by_spell(vict, SKILL_GOUGE))) {
				cast_spell(ch, vict, 0, NULL, SPELL_CURE_BLIND);
				return true;
			} else
				return false;
			break;
		}
	case 19:
	case 20:
	case 21:
	case 22:{
			found = false;
			if (ch->in_room->zone->weather->sky == SKY_LIGHTNING) {
				found = false;
				struct creatureList_iterator it = ch->in_room->people.begin();
				for (; it != ch->in_room->people.end() && !found; ++it) {
					vict = *it;
					if (!affected_by_spell(vict, SPELL_PROT_FROM_LIGHTNING) &&
						GET_MANA(ch) > 50) {
						cast_spell(ch, vict, 0, NULL, SPELL_PROT_FROM_LIGHTNING);
						return true;
					}
				}
			}
			break;
		}
	default:
		return false;
	}
	return false;
}
