//
// File: act.psionic.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// File: act.psionic.c -- psionic stuff
//
// Copyright 1998 by John Watson, all rights reserved
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
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"

ACMD(do_psidrain)
{

	struct creature *vict = NULL;
	int dist, drain, prob, percent;
	int find_distance(struct room_data *tmp, struct room_data *location);

	ACMD_set_return_flags(0);

	skip_spaces(&argument);

	if (CHECK_SKILL(ch, SKILL_PSIDRAIN) < 30 || !IS_PSIONIC(ch)) {
		send_to_char(ch, "You have no idea how.\r\n");
		return;
	}

	if (!*argument && !(vict = random_opponent(ch))) {
		send_to_char(ch, "Psidrain who?\r\n");
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS) && GET_LEVEL(ch) < LVL_GOD) {
		send_to_char(ch, "Psychic powers are useless here!\r\n");
		return;
	}

	if (*argument) {
		if (!(vict = get_char_room_vis(ch, argument)) &&
			!(vict = get_char_vis(ch, argument))) {
			send_to_char(ch, "You cannot locate %s '%s'.\r\n", AN(argument),
				argument);
			return;
		}
	}
	if (GET_LEVEL(vict) >= LVL_AMBASSADOR &&
		GET_LEVEL(ch) < GET_LEVEL(vict)) {
        send_to_char(ch, "You cannot locate %s '%s'.\r\n", AN(argument),
                     argument);
		act("$n has just tried to psidrain you.",
			false, ch, 0, vict, TO_VICT);
		return;
	}
	if (!vict)
		return;

	if (vict == ch) {
		send_to_char(ch, "Ha ha... Funny!\r\n");
		return;
	}

	if (ch->fighting && vict->in_room != ch->in_room) {
		send_to_char(ch, "You cannot focus outside the room during battle!\r\n");
		return;
	}

	if (ch->in_room != vict->in_room &&
		ch->in_room->zone != vict->in_room->zone) {
		act("$N is not in your zone.", false, ch, 0, vict, TO_CHAR);
		return;
	}

	if (ROOM_FLAGGED(vict->in_room, ROOM_NOPSIONICS)
		&& GET_LEVEL(ch) < LVL_GOD) {
		act("Psychic powers are useless where $E is!", false, ch, 0, vict,
			TO_CHAR);
		return;
	}

	if (!ok_to_attack(ch, vict, true))
		return;

	if (GET_MOVE(ch) < 20) {
		send_to_char(ch, "You are too physically exhausted.\r\n");
		return;
	}

	if ((dist = find_distance(ch->in_room, vict->in_room)) >
		((GET_LEVEL(ch) / 6))) {
		act("$N is out of your psychic range.", false, ch, 0, vict, TO_CHAR);
		return;
	}

	if (NULL_PSI(vict)) {
		act("It is pointless to attempt this on $M.", false, ch, 0, vict,
			TO_CHAR);
		return;
	}

	if (AFF3_FLAGGED(vict, AFF3_PSISHIELD) && distrusts(vict, ch)) {
        prob = CHECK_SKILL(ch, SKILL_PSIDRAIN) + GET_INT(ch);
        prob += skill_bonus(ch, SKILL_PSIDRAIN);

        percent = skill_bonus(vict, SPELL_PSISHIELD);
        percent += number(1, 120);

        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_PSI))
            percent <<= 1;

        if (GET_INT(vict) > GET_INT(ch))
            percent += (GET_INT(vict) - GET_INT(ch)) << 3;

        if (percent >= prob) {
            act("Your attack is deflected by $N's psishield!",
                false, ch, 0, vict, TO_CHAR);
            act("$n's psychic attack is deflected by your psishield!",
                false, ch, 0, vict, TO_VICT);
            act("$n staggers under an unseen force.",
                true, ch, 0, vict, TO_NOTVICT);

		    return;
        }
	}

	if (GET_MANA(vict) <= 0) {
		act("$E is completely drained of psychic energy.",
			true, ch, 0, vict, TO_CHAR);
		return;
	}

	drain = dice(GET_LEVEL(ch), GET_INT(ch) + GET_REMORT_GEN(ch)) +
		CHECK_SKILL(ch, SKILL_PSIDRAIN);
	if (dist > 0)
		drain /= dist + 1;

	drain >>= 2;

	drain = MAX(0, MIN(GET_MANA(vict), drain));

	prob = CHECK_SKILL(ch, SKILL_PSIDRAIN) + GET_INT(ch) +
		(AFF3_FLAGGED(vict, AFF3_PSISHIELD) ? -20 : 0);

	if (isFighting(vict))
		prob += 15;

	if (dist > 0)
		prob -= dist * 3;

	act("$n strains against an unseen force.", false, ch, 0, vict, TO_ROOM);

	//
	// failure
	//

	if (number(0, 121) > prob) {
		send_to_char(ch, "You are unable to create the drainage link!\r\n");
		WAIT_STATE(ch, 2 RL_SEC);

		if (IS_NPC(vict) && !isFighting(vict)) {

			if (ch->in_room == vict->in_room) {
				addCombat(vict, ch, false);
                addCombat(ch, vict, true);
            }
			else {
				remember(vict, ch);
				if (MOB2_FLAGGED(vict, MOB2_HUNT))
					startHunting(vict, ch);
			}
		}
	}
	//
	// success
	//
	else {

		act("A torrent of psychic energy is ripped out of $N's mind!",
			false, ch, 0, vict, TO_CHAR);
		if (ch->in_room != vict->in_room &&
			GET_LEVEL(vict) + number(0, CHECK_SKILL(vict, SKILL_PSIDRAIN)) >
			GET_LEVEL(ch))
			act("Your psychic energy is ripped from you from afar!",
				false, ch, 0, vict, TO_VICT);
		else
			act("Your psychic energy is ripped from you by $n!",
				false, ch, 0, vict, TO_VICT);
		GET_MANA(vict) -= drain;
		GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + drain);
		GET_MOVE(ch) -= 20;
		WAIT_STATE(vict, 1 RL_SEC);
		WAIT_STATE(ch, 5 RL_SEC);
		gain_skill_prof(ch, SKILL_PSIDRAIN);

		if (IS_NPC(vict) && !(isFighting(vict))) {
			if (ch->in_room == vict->in_room) {
				remember(vict, ch);
				if (MOB2_FLAGGED(vict, MOB2_HUNT))
					startHunting(vict, ch);
				addCombat(vict, ch, false);
                addCombat(ch, vict, true);
			}
		}
	}
}

int
calculate_mob_aggression(struct creature *ch, struct creature *vict)
{
    // aggression is the average of the percent health of self and the
    // percent damage of the other
    return (GET_HIT(ch) * 100 / GET_MAX_HIT(ch)
            + (100 - GET_HIT(vict) * 100 / GET_MAX_HIT(vict))) / 2;
}

bool
nullpsi_is_advisable(struct creature *vict)
{
    // Return true if psion buffs are found
    struct affected_type *af;
    for (af = vict->affected;af;af = af->next) {
        if (SPELL_IS_PSIONIC(af->type)
            && !SPELL_FLAGGED(af->type, MAG_DAMAGE)
            && !spell_info[af->type].violent
            && !spell_info[af->type].targets & TAR_UNPLEASANT)
            return true;
    }
    return false;
}

void
psionic_best_attack(struct creature *ch, struct creature *vict)
{
    int return_flags;
    int aggression = calculate_mob_aggression(ch, vict);

    // Psions can't really do anything when there's a psishield in place
    if (AFF3_FLAGGED(vict, AFF3_PSISHIELD) && can_cast_spell(ch, SPELL_PSIONIC_SHATTER)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSIONIC_SHATTER, &return_flags);
        return;
    }
    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (!affected_by_spell(vict, SPELL_PSYCHIC_SURGE) && can_cast_spell(ch, SPELL_PSYCHIC_SURGE)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_SURGE, &return_flags);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return;
        } else if (GET_POSITION(vict) > POS_SITTING && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP, &return_flags);
            return;
        }
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH) && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH, &return_flags);
            return;
        } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM) && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM, &return_flags);
            return;
        } else if (GET_POSITION(vict) > POS_SITTING && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP, &return_flags);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (!IS_CONFUSED(vict)
            && can_cast_spell(ch, SPELL_CONFUSION)
            && (IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
                IS_KNIGHT(vict) || IS_PHYSIC(vict))) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CONFUSION, &return_flags);
            return;
        } else if (!AFF2_FLAGGED(ch, AFF2_VERTIGO) && can_cast_spell(ch, SPELL_VERTIGO)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_VERTIGO, &return_flags);
            return;
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_FEEDBACK) && can_cast_spell(ch, SPELL_PSYCHIC_FEEDBACK)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_FEEDBACK, &return_flags);
            return;
        } else if (nullpsi_is_advisable(vict) && can_cast_spell(ch, SPELL_NULLPSI)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_NULLPSI, &return_flags);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return;
        }
    }
    if (aggression > 5) {
        // attempt to neutralize or get away
        if (GET_POSITION(vict) > POS_SLEEPING && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD, &return_flags);
            return;
        } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL, &return_flags);
            return;
        } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA, &return_flags);
            return;
        } else if (can_cast_spell(ch, SPELL_FEAR)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_FEAR, &return_flags);
            return;
        }
    }
    // desperation - just attack full force, as hard as possible
    if (!affected_by_spell(vict, SPELL_PSYCHIC_SURGE) && can_cast_spell(ch, SPELL_PSYCHIC_SURGE)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_SURGE, &return_flags);
        return;
    } else if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH) && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH, &return_flags);
        return;
    } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM) && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM, &return_flags);
        return;
    } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
        perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
        return;
    } else if (GET_POSITION(vict) > POS_SITTING && can_cast_spell(ch, SPELL_EGO_WHIP)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP, &return_flags);
        return;
    } else if (GET_POSITION(vict) > POS_SLEEPING && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD, &return_flags);
        return;
    } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL, &return_flags);
        return;
    } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA, &return_flags);
        return;
    } else if (can_cast_spell(ch, SPELL_FEAR)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_FEAR, &return_flags);
        return;
    } else {
        hit(ch, vict, TYPE_UNDEFINED);
    }
}

void
psionic_activity(struct creature *ch)
{
    if (room_is_dark(ch->in_room)
        && !has_dark_sight(ch)
        && can_cast_spell(ch, SPELL_RETINA))
        cast_spell(ch, ch, 0, NULL, SPELL_RETINA, NULL);
    else if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (can_cast_spell(ch, SPELL_CELL_REGEN))
            cast_spell(ch, ch, 0, NULL, SPELL_CELL_REGEN, NULL);
        else if (can_cast_spell(ch, SPELL_WOUND_CLOSURE))
            cast_spell(ch, ch, 0, NULL, SPELL_WOUND_CLOSURE, NULL);
    } else if (!AFF_FLAGGED(ch, AFF_NOPAIN)
               && !AFF_FLAGGED(ch, AFF_SANCTUARY)
               && can_cast_spell(ch, SPELL_NOPAIN))
        cast_spell(ch, ch, 0, NULL, SPELL_NOPAIN, NULL);
    else if (!room_has_air(ch->in_room) &&
             !can_travel_sector(ch, ch->in_room->sector_type, 0) &&
             can_cast_spell(ch, SPELL_BREATHING_STASIS) &&
             !AFF3_FLAGGED(ch, AFF3_NOBREATHE))
        cast_spell(ch, ch, 0, NULL, SPELL_BREATHING_STASIS, NULL);
    else if (!AFF2_FLAGGED(ch, AFF2_TELEKINESIS)
             && can_cast_spell(ch, SPELL_TELEKINESIS))
        cast_spell(ch, ch, 0, NULL, SPELL_TELEKINESIS, NULL);
    else if (!affected_by_spell(ch, SPELL_DERMAL_HARDENING)
             && can_cast_spell(ch, SPELL_DERMAL_HARDENING))
        cast_spell(ch, ch, 0, NULL, SPELL_DERMAL_HARDENING, NULL);
    else if (!AFF3_FLAGGED(ch, AFF3_PSISHIELD)
             && can_cast_spell(ch, SPELL_PSISHIELD))
        cast_spell(ch, ch, 0, NULL, SPELL_PSISHIELD, NULL);
    else if (can_cast_spell(ch, SPELL_PSYCHIC_RESISTANCE) &&
             !affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
        cast_spell(ch, ch, 0, NULL, SPELL_PSYCHIC_RESISTANCE, NULL);
    else if (can_cast_spell(ch, SPELL_POWER) && !affected_by_spell(ch, SPELL_POWER))
        cast_spell(ch, ch, 0, NULL, SPELL_POWER, NULL);
    else if (!AFF_FLAGGED(ch, AFF_CONFIDENCE)
             && can_cast_spell(ch, SPELL_CONFIDENCE))
        cast_spell(ch, ch, 0, NULL, SPELL_CONFIDENCE, NULL);
}

int
psionic_mob_fight(struct creature *ch, struct creature *precious_vict)
{
	struct creature *vict = 0;
    int return_flags;

	if (!ch->fighting)
		return 0;

	// pick an enemy
	if (!(vict = choose_opponent(ch, precious_vict)))
		return 0;

    int aggression = calculate_mob_aggression(ch, vict);

    // Psions can't really do anything when there's a psishield in place
    if (AFF3_FLAGGED(vict, AFF3_PSISHIELD) && can_cast_spell(ch, SPELL_PSIONIC_SHATTER)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSIONIC_SHATTER, &return_flags);
        return 1;
    }

    // Prioritize healing with aggression
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) * MIN(20, MAX(80, aggression)) / 100)
        && can_cast_spell(ch, SPELL_WOUND_CLOSURE)) {
		cast_spell(ch, ch, NULL, NULL, SPELL_WOUND_CLOSURE, NULL);
        return 1;
    }

    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return 1;
        } else if (GET_POSITION(vict) > POS_SITTING && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP, &return_flags);
            return 1;
        }
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (GET_MANA(ch) < GET_MAX_MANA(ch) / 2 && can_cast_spell(ch, SKILL_PSIDRAIN)) {
            if (!can_see_creature(ch, vict))
                // just attack the default opponent
                do_psidrain(ch, tmp_strdup(""), 0, 0, 0);
            else
                do_psidrain(ch, GET_NAME(vict), 0, 0, 0);
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH) && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH, &return_flags);
            return 1;
        } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM) && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM, &return_flags);
            return 1;
        } else if (!affected_by_spell(ch, SPELL_ADRENALINE) && can_cast_spell(ch, SPELL_ADRENALINE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_ADRENALINE, &return_flags);
        } else if (GET_POSITION(vict) > POS_SITTING && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP, &return_flags);
            return 1;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return 1;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (IS_PSIONIC(vict)
            && !affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE)
            && can_cast_spell(ch, SPELL_PSYCHIC_RESISTANCE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_PSYCHIC_RESISTANCE, &return_flags);
        } else if (!IS_CONFUSED(vict)
                   && can_cast_spell(ch, SPELL_CONFUSION)
                   && (IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
                       IS_KNIGHT(vict) || IS_PHYSIC(vict))) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CONFUSION, &return_flags);
            return 1;
        } else if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4 && can_cast_spell(ch, SPELL_ENDURANCE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_ENDURANCE, &return_flags);
            return 1;
        } else if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4 && can_cast_spell(ch, SPELL_DERMAL_HARDENING)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_DERMAL_HARDENING, &return_flags);
            return 1;
        } else if (!AFF2_FLAGGED(ch, AFF2_VERTIGO) && can_cast_spell(ch, SPELL_VERTIGO)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_VERTIGO, &return_flags);
            return 1;
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_FEEDBACK) && can_cast_spell(ch, SPELL_PSYCHIC_FEEDBACK)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_FEEDBACK, &return_flags);
            return 1;
        } else if (!affected_by_spell(vict, SPELL_WEAKNESS) && can_cast_spell(ch, SPELL_WEAKNESS)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_WEAKNESS, &return_flags);
            return 1;
        } else if (!affected_by_spell(vict, SPELL_CLUMSINESS) && can_cast_spell(ch, SPELL_CLUMSINESS)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CLUMSINESS, &return_flags);
            return 1;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST, &return_flags);
            return 1;
        }
    }
    if (aggression > 5) {
        // attempt to neutralize or get away
        if (GET_POSITION(vict) > POS_SLEEPING && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD,
                       &return_flags);
            return 1;
        } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL, &return_flags);
            return 1;
        } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA, &return_flags);
            return 1;
        } else if (can_cast_spell(ch, SPELL_FEAR)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_FEAR, &return_flags);
            return 1;
        }
    }

	return 0;
}
