//
// File: temple_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(temple_healer)
{
	Creature *self = (Creature *)me;
	struct Creature *vict;
	int found = 0;

	if (spec_mode != SPECIAL_TICK)
		return false;

	if (self->getPosition() == POS_FIGHTING && FIGHTING(self) &&
		FIGHTING(self)->in_room == self->in_room) {
		switch (number(0, 20)) {
		case 0:
			do_say(self, "Now you pay!!", 0, 0, 0);
			cast_spell(self, FIGHTING(self), NULL, SPELL_FLAME_STRIKE);
			return true;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if (!IS_AFFECTED(self, AFF_SANCTUARY)) {
				do_say(self, "Guiharia, aid me now!!", 0, 0, 0);
				call_magic(self, self, NULL, SPELL_SANCTUARY, 50, CAST_SPELL);
				return true;
			}
		case 8:
			cast_spell(self, self, NULL, SPELL_GREATER_HEAL);
			return true;
		}
		return false;
	}

	if (self->getPosition() != POS_FIGHTING && !FIGHTING(self)) {
		switch (number(0, 18)) {
		case 0:
			do_say(self, "Rest here, adventurer.  Your wounds will be tended.",
				0, 0, 0);
			return true;
		case 1:
			do_say(self, "You are in the hands of Guiharia here, traveller.", 0,
				0, 0);
			return true;
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
				found = FALSE;
				CreatureList::iterator it = self->in_room->people.begin();
				for (; it != self->in_room->people.end() && !found; ++it) {
					vict = *it;
					if (self == vict || !CAN_SEE(self, vict) || !number(0, 2))
						continue;
					if (GET_MOB_VNUM(self) == 11000 && IS_EVIL(vict)) {
						if (!number(0, 20)) {
							act("$n looks at you with distaste.", FALSE, self, 0,
								vict, TO_VICT);
							act("$n looks at $N with distaste.", FALSE, self, 0,
								vict, TO_NOTVICT);
						} else if (!number(0, 20)) {
							act("$n looks at you scornfully.", FALSE, self, 0,
								vict, TO_VICT);
							act("$n looks at $N scornfully.", FALSE, self, 0,
								vict, TO_NOTVICT);
						}
						continue;
					}
					if (IS_NPC(vict))
						continue;

					if (GET_HIT(vict) < GET_MAX_HIT(vict)) {
						act("$n touches your forehead, and your pain subsides.", TRUE, self, 0, vict, TO_VICT);
						act("$n touches $N, and heals $M.", TRUE, self, 0, vict,
							TO_NOTVICT);

						cast_spell(self, vict, 0,
							GET_LEVEL(vict) <= 10 ? SPELL_CURE_LIGHT :
							GET_LEVEL(vict) <= 20 ? SPELL_CURE_CRITIC :
							GET_LEVEL(vict) <=
							30 ? SPELL_HEAL : SPELL_GREATER_HEAL);
					} else
						continue;

					return true;
				}
				break;
			}
		case 14:
		case 15:
		case 16:{

				CreatureList::iterator it = self->in_room->people.begin();
				for (; it != self->in_room->people.end() && !found; ++it) {
					vict = *it;
					if (self == vict || IS_NPC(vict) || !CAN_SEE(self, vict)
						|| !number(0, 2))
						continue;

					if (IS_POISONED(vict) || IS_SICK(vict)) {
						act("$n sweeps $s hand over your body, and your sickness ceases.", FALSE, self, 0, vict, TO_VICT);
						act("$n sweeps $s hand over $N's body.",
							TRUE, self, 0, vict, TO_NOTVICT);
						act("You sweep your hand over $N's body.",
							FALSE, self, 0, vict, TO_CHAR);

						if (IS_POISONED(vict))
							call_magic(self, vict, 0, SPELL_REMOVE_POISON,
								GET_LEVEL(self), CAST_SPELL);
						if (IS_SICK(vict))
							call_magic(self, vict, 0, SPELL_REMOVE_SICKNESS,
								GET_LEVEL(self), CAST_SPELL);
						return true;
					}
				}
				break;
			}
		case 17:
		case 18:{
				found = FALSE;
				CreatureList::iterator it = self->in_room->people.begin();
				for (; it != self->in_room->people.end() && !found; ++it) {
					vict = *it;

					if (self == vict || IS_NPC(vict) || !CAN_SEE(self, vict)
						|| !number(0, 2))
						continue;

					if (affected_by_spell(vict, SPELL_BLINDNESS) ||
						affected_by_spell(vict, SKILL_GOUGE)) {
						act("$n touches your eyes, and your vision returns.",
							FALSE, self, 0, vict, TO_VICT);
						act("$n touches the eyes of $N.", TRUE, self, 0, vict,
							TO_NOTVICT);
						act("You touch the eyes of $N.", FALSE, self, 0, vict,
							TO_CHAR);
						call_magic(self, vict, 0, SPELL_CURE_BLIND,
							GET_LEVEL(self), CAST_SPELL);
						return true;

					}
				}
			}
		}
	}
	return false;
}
