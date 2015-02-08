//
// File: temple_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(temple_healer)
{
    struct creature *self = (struct creature *)me;
    struct creature *vict;
    int found = 0;

    if (spec_mode != SPECIAL_TICK) {
        return false;
    }

    if (is_fighting(self)) {
        vict = random_opponent(self);

        if (vict->in_room == self->in_room) {
            switch (number(0, 20)) {
            case 0:
                perform_say(self, "say", "Now you pay!!");
                cast_spell(self, vict, NULL, NULL, SPELL_FLAME_STRIKE);
                return true;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                if (!AFF_FLAGGED(self, AFF_SANCTUARY)) {
                    perform_say(self, "say", "Guiharia, aid me now!!");
                    call_magic(self, self, NULL, NULL, SPELL_SANCTUARY, 50,
                               CAST_SPELL);
                    return true;
                }
                break;
            case 8:
                cast_spell(self, self, NULL, NULL, SPELL_GREATER_HEAL);
                return true;
            }
            return false;
        }
    }

    if (GET_POSITION(self) != POS_FIGHTING && !is_fighting(self)) {
        switch (number(0, 18)) {
        case 0:
            perform_say(self, "say",
                        "Rest here, adventurer.  Your wounds will be tended.");
            return true;
        case 1:
            perform_say(self, "say",
                        "You are in the hands of Guiharia here, traveller.");
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
        case 13: {
            found = false;
            for (GList *it = first_living(self->in_room->people); it && !found;
                 it = next_living(it)) {
                vict = it->data;
                if (self == vict || !can_see_creature(self, vict)
                    || !number(0, 2)) {
                    continue;
                }
                if (GET_NPC_VNUM(self) == 11000 && IS_EVIL(vict)) {
                    if (!number(0, 20)) {
                        act("$n looks at you with distaste.", false, self,
                            NULL, vict, TO_VICT);
                        act("$n looks at $N with distaste.", false, self,
                            NULL, vict, TO_NOTVICT);
                    } else if (!number(0, 20)) {
                        act("$n looks at you scornfully.", false, self, NULL,
                            vict, TO_VICT);
                        act("$n looks at $N scornfully.", false, self, NULL,
                            vict, TO_NOTVICT);
                    }
                    continue;
                }
                if (IS_NPC(vict)) {
                    continue;
                }

                if (GET_HIT(vict) < GET_MAX_HIT(vict)) {
                    act("$n touches your forehead, and your pain subsides.", true, self, NULL, vict, TO_VICT);
                    act("$n touches $N, and heals $M.", true, self, NULL,
                        vict, TO_NOTVICT);

                    cast_spell(self, vict, NULL, NULL,
                               GET_LEVEL(vict) <= 10 ? SPELL_CURE_LIGHT :
                               GET_LEVEL(vict) <= 20 ? SPELL_CURE_CRITIC :
                               GET_LEVEL(vict) <=
                               30 ? SPELL_HEAL : SPELL_GREATER_HEAL);
                } else {
                    continue;
                }

                return true;
            }
            break;
        }
        case 14:
        case 15:
        case 16: {
            for (GList *it = first_living(self->in_room->people); it && !found;
                 it = next_living(it)) {
                vict = it->data;
                if (self == vict || IS_NPC(vict)
                    || !can_see_creature(self, vict)
                    || !number(0, 2)) {
                    continue;
                }

                if (IS_POISONED(vict) || IS_SICK(vict)) {
                    act("$n sweeps $s hand over your body, and your sickness ceases.", false, self, NULL, vict, TO_VICT);
                    act("$n sweeps $s hand over $N's body.",
                        true, self, NULL, vict, TO_NOTVICT);
                    act("You sweep your hand over $N's body.",
                        false, self, NULL, vict, TO_CHAR);

                    if (IS_POISONED(vict)) {
                        call_magic(self, vict, NULL, NULL,
                                   SPELL_REMOVE_POISON, GET_LEVEL(self),
                                   CAST_SPELL);
                    }
                    if (IS_SICK(vict)) {
                        call_magic(self, vict, NULL, NULL,
                                   SPELL_REMOVE_SICKNESS, GET_LEVEL(self),
                                   CAST_SPELL);
                    }
                    return true;
                }
            }
            break;
        }
        case 17:
        case 18: {
            found = false;
            for (GList *it = first_living(self->in_room->people); it && !found;
                 it = next_living(it)) {
                vict = it->data;

                if (self == vict || IS_NPC(vict)
                    || !can_see_creature(self, vict)
                    || !number(0, 2)) {
                    continue;
                }

                if (affected_by_spell(vict, SPELL_BLINDNESS) ||
                    affected_by_spell(vict, SKILL_GOUGE)) {
                    act("$n touches your eyes, and your vision returns.",
                        false, self, NULL, vict, TO_VICT);
                    act("$n touches the eyes of $N.", true, self, NULL, vict,
                        TO_NOTVICT);
                    act("You touch the eyes of $N.", false, self, NULL, vict,
                        TO_CHAR);
                    call_magic(self, vict, NULL, NULL, SPELL_CURE_BLIND,
                               GET_LEVEL(self), CAST_SPELL);
                    return true;

                }
            }
        }
        }
    }
    return false;
}
