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
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "fight.h"
#include "strutil.h"
#include "mobact.h"

bool
can_psidrain(struct creature *ch, struct creature *vict, int *out_dist, bool msg)
{
    const char *to_char = NULL;
    const char *to_vict = NULL;
    int dist;

    if (CHECK_SKILL(ch, SKILL_PSIDRAIN) < 30 || !IS_PSIONIC(ch)) {
        to_char = "You have no idea how.";
        goto failure;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOPSIONICS) && GET_LEVEL(ch) < LVL_GOD) {
        to_char = "Psychic powers are useless here!";
        goto failure;
    }

    if (GET_LEVEL(vict) >= LVL_AMBASSADOR && GET_LEVEL(ch) < GET_LEVEL(vict)) {
        to_char = "You cannot locate them.";
        to_vict = "$n has just tried to psidrain you.";
        goto failure;
    }

    if (vict == ch) {
        to_char = "Ha ha... Funny!";
        goto failure;
    }

    if (is_fighting(ch) && vict->in_room != ch->in_room) {
        to_char = "You cannot focus outside the room during battle!";
        goto failure;
    }

    if (ch->in_room != vict->in_room &&
        ch->in_room->zone != vict->in_room->zone) {
        to_char = "$N is not in your zone.";
        goto failure;
    }

    if (ROOM_FLAGGED(vict->in_room, ROOM_NOPSIONICS)
        && GET_LEVEL(ch) < LVL_GOD) {
        to_char = "Psychic powers are useless where $E is!";
        goto failure;
    }

    if (!ok_to_attack(ch, vict, msg))
        return false;

    if (GET_MOVE(ch) < 20) {
        to_char = "You are too physically exhausted.";
        goto failure;
    }

    dist = find_distance(ch->in_room, vict->in_room);
    if (out_dist)
        *out_dist = dist;
    if (dist > ((GET_LEVEL(ch) / 6))) {
        to_char = "$N is out of your psychic range.";
        goto failure;
    }

    if (NULL_PSI(vict)) {
        to_char = "It is pointless to attempt this on $M.";
        goto failure;
    }

    return true;

failure:
    if (msg) {
        if (to_char)
            act(to_char, false, ch, NULL, vict, TO_CHAR);
        if (to_vict)
            act(to_vict, false, ch, NULL, vict, TO_VICT);
    }
    return false;
}

void
perform_psidrain(struct creature *ch, struct creature *vict)
{
    int find_distance(struct room_data *tmp, struct room_data *location);
    int dist, drain, prob, percent;

    if (!can_psidrain(ch, vict, &dist, true))
        return;


    if (AFF3_FLAGGED(vict, AFF3_PSISHIELD) && creature_distrusts(vict, ch)) {
        prob = CHECK_SKILL(ch, SKILL_PSIDRAIN) + GET_INT(ch);
        prob += skill_bonus(ch, SKILL_PSIDRAIN);

        percent = skill_bonus(vict, SPELL_PSISHIELD);
        percent += number(1, 120);

        if (mag_savingthrow(vict, GET_LEVEL(ch), SAVING_PSI))
            percent *= 2;

        if (GET_INT(vict) > GET_INT(ch))
            percent += (GET_INT(vict) - GET_INT(ch)) * 8;

        if (percent >= prob) {
            act("Your attack is deflected by $N's psishield!",
                false, ch, NULL, vict, TO_CHAR);
            act("$n's psychic attack is deflected by your psishield!",
                false, ch, NULL, vict, TO_VICT);
            act("$n staggers under an unseen force.",
                true, ch, NULL, vict, TO_NOTVICT);

            return;
        }
    }

    if (GET_MANA(vict) <= 0) {
        act("$E is completely drained of psychic energy.",
            true, ch, NULL, vict, TO_CHAR);
        return;
    }

    drain = dice(GET_LEVEL(ch), GET_INT(ch) + GET_REMORT_GEN(ch)) +
        CHECK_SKILL(ch, SKILL_PSIDRAIN);
    if (dist > 0)
        drain /= dist + 1;

    drain /= 4;

    drain = MAX(0, MIN(GET_MANA(vict), drain));

    prob = CHECK_SKILL(ch, SKILL_PSIDRAIN) + GET_INT(ch) +
        (AFF3_FLAGGED(vict, AFF3_PSISHIELD) ? -20 : 0);

    if (is_fighting(vict))
        prob += 15;

    if (dist > 0)
        prob -= dist * 3;

    act("$n strains against an unseen force.", false, ch, NULL, vict, TO_ROOM);

    //
    // failure
    //

    if (number(0, 121) > prob) {
        send_to_char(ch, "You are unable to create the drainage link!\r\n");
        WAIT_STATE(ch, 2 RL_SEC);

        if (IS_NPC(vict) && !is_fighting(vict)) {

            if (ch->in_room == vict->in_room) {
                add_combat(vict, ch, false);
                add_combat(ch, vict, true);
            } else {
                remember(vict, ch);
                if (NPC2_FLAGGED(vict, NPC2_HUNT))
                    start_hunting(vict, ch);
            }
        }
    }
    //
    // success
    //
    else {

        act("A torrent of psychic energy is ripped out of $N's mind!",
            false, ch, NULL, vict, TO_CHAR);
        if (ch->in_room != vict->in_room &&
            GET_LEVEL(vict) + number(0, CHECK_SKILL(vict, SKILL_PSIDRAIN)) >
            GET_LEVEL(ch))
            act("Your psychic energy is ripped from you from afar!",
                false, ch, NULL, vict, TO_VICT);
        else
            act("Your psychic energy is ripped from you by $n!",
                false, ch, NULL, vict, TO_VICT);
        GET_MANA(vict) -= drain;
        GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + drain);
        GET_MOVE(ch) -= 20;
        WAIT_STATE(vict, 1 RL_SEC);
        WAIT_STATE(ch, 5 RL_SEC);
        gain_skill_prof(ch, SKILL_PSIDRAIN);

        if (IS_NPC(vict) && !(is_fighting(vict))) {
            if (ch->in_room == vict->in_room) {
                remember(vict, ch);
                if (NPC2_FLAGGED(vict, NPC2_HUNT))
                    start_hunting(vict, ch);
                add_combat(vict, ch, false);
                add_combat(ch, vict, true);
            }
        }
    }
}

ACMD(do_psidrain)
{

    struct creature *vict = NULL;

    skip_spaces(&argument);

    if (!*argument && !(vict = random_opponent(ch))) {
        send_to_char(ch, "Psidrain who?\r\n");
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

    if (GET_LEVEL(vict) >= LVL_AMBASSADOR && GET_LEVEL(ch) < GET_LEVEL(vict)) {
        send_to_char(ch, "You cannot locate %s '%s'.\r\n", AN(argument),
            argument);
        act("$n has just tried to psidrain you.", false, ch, NULL, vict, TO_VICT);
        return;
    }

    perform_psidrain(ch, vict);
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
nullpsi_is_advisable(struct creature * vict)
{
    int blessings = 0;
    int curses = 0;

    // Return true if psionic debuffs are found
    for (struct affected_type *af = vict->affected;af;af = af->next) {
        if (SPELL_IS_PSIONIC(af->type)) {
            if (!SPELL_FLAGGED(af->type, MAG_DAMAGE)
                && !spell_info[af->type].violent
                && !(spell_info[af->type].targets & TAR_UNPLEASANT))
                blessings++;
            else
                curses++;
        }
    }
    return (blessings > curses * 2);
}

void
psionic_best_attack(struct creature *ch, struct creature *vict)
{
    int aggression = calculate_mob_aggression(ch, vict);

    // Psions can't really do anything when there's a psishield in place
    if (AFF3_FLAGGED(vict, AFF3_PSISHIELD)
        && can_cast_spell(ch, SPELL_PSIONIC_SHATTER)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSIONIC_SHATTER);
        return;
    }
    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (!affected_by_spell(vict, SPELL_PSYCHIC_SURGE)
            && can_cast_spell(ch, SPELL_PSYCHIC_SURGE)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_SURGE);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return;
        } else if (GET_POSITION(vict) > POS_SITTING
            && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP);
            return;
        }
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH)
            && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH);
            return;
        } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM)
            && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM);
            return;
        } else if (GET_POSITION(vict) > POS_SITTING
            && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (!IS_CONFUSED(vict)
            && can_cast_spell(ch, SPELL_CONFUSION)
            && (IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
                IS_KNIGHT(vict) || IS_PHYSIC(vict))) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CONFUSION);
            return;
        } else if (!AFF2_FLAGGED(ch, AFF2_VERTIGO)
            && can_cast_spell(ch, SPELL_VERTIGO)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_VERTIGO);
            return;
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_FEEDBACK)
            && can_cast_spell(ch, SPELL_PSYCHIC_FEEDBACK)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_FEEDBACK);
            return;
        } else if (nullpsi_is_advisable(vict)
            && can_cast_spell(ch, SPELL_NULLPSI)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_NULLPSI);
            return;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return;
        }
    }
    if (aggression > 5) {
        // attempt to neutralize or get away
        if (GET_POSITION(vict) > POS_SLEEPING
            && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD);
            return;
        } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL);
            return;
        } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA);
            return;
        } else if (can_cast_spell(ch, SPELL_FEAR)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_FEAR);
            return;
        }
    }
    // desperation - just attack full force, as hard as possible
    if (!affected_by_spell(vict, SPELL_PSYCHIC_SURGE)
        && can_cast_spell(ch, SPELL_PSYCHIC_SURGE)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_SURGE);
        return;
    } else if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH)
        && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH);
        return;
    } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM)
        && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM);
        return;
    } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
        perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
        return;
    } else if (GET_POSITION(vict) > POS_SITTING
        && can_cast_spell(ch, SPELL_EGO_WHIP)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP);
        return;
    } else if (GET_POSITION(vict) > POS_SLEEPING
        && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD);
        return;
    } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL);
        return;
    } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA);
        return;
    } else if (can_cast_spell(ch, SPELL_FEAR)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_FEAR);
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
        cast_spell(ch, ch, NULL, NULL, SPELL_RETINA);
    else if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.80) {
        if (can_cast_spell(ch, SPELL_CELL_REGEN))
            cast_spell(ch, ch, NULL, NULL, SPELL_CELL_REGEN);
        else if (can_cast_spell(ch, SPELL_WOUND_CLOSURE))
            cast_spell(ch, ch, NULL, NULL, SPELL_WOUND_CLOSURE);
    } else if (!AFF_FLAGGED(ch, AFF_NOPAIN)
        && !AFF_FLAGGED(ch, AFF_SANCTUARY)
        && can_cast_spell(ch, SPELL_NOPAIN))
        cast_spell(ch, ch, NULL, NULL, SPELL_NOPAIN);
    else if (!room_has_air(ch->in_room) &&
        !can_travel_sector(ch, ch->in_room->sector_type, 0) &&
        can_cast_spell(ch, SPELL_BREATHING_STASIS) &&
        !AFF3_FLAGGED(ch, AFF3_NOBREATHE))
        cast_spell(ch, ch, NULL, NULL, SPELL_BREATHING_STASIS);
    else if (!AFF2_FLAGGED(ch, AFF2_TELEKINESIS)
        && can_cast_spell(ch, SPELL_TELEKINESIS))
        cast_spell(ch, ch, NULL, NULL, SPELL_TELEKINESIS);
    else if (!affected_by_spell(ch, SPELL_DERMAL_HARDENING)
        && can_cast_spell(ch, SPELL_DERMAL_HARDENING))
        cast_spell(ch, ch, NULL, NULL, SPELL_DERMAL_HARDENING);
    else if (!AFF3_FLAGGED(ch, AFF3_PSISHIELD)
        && can_cast_spell(ch, SPELL_PSISHIELD))
        cast_spell(ch, ch, NULL, NULL, SPELL_PSISHIELD);
    else if (can_cast_spell(ch, SPELL_PSYCHIC_RESISTANCE) &&
        !affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
        cast_spell(ch, ch, NULL, NULL, SPELL_PSYCHIC_RESISTANCE);
    else if (can_cast_spell(ch, SPELL_POWER)
        && !affected_by_spell(ch, SPELL_POWER))
        cast_spell(ch, ch, NULL, NULL, SPELL_POWER);
    else if (!AFF_FLAGGED(ch, AFF_CONFIDENCE)
        && can_cast_spell(ch, SPELL_CONFIDENCE))
        cast_spell(ch, ch, NULL, NULL, SPELL_CONFIDENCE);
}

bool
psionic_mob_fight(struct creature *ch, struct creature *precious_vict)
{
    struct creature *vict = NULL;

    if (!is_fighting(ch))
        return false;

    // pick an enemy
    if (!(vict = choose_opponent(ch, precious_vict)))
        return false;

    int aggression = calculate_mob_aggression(ch, vict);

    // Psions can't really do anything when there's a psishield in place
    if (AFF3_FLAGGED(vict, AFF3_PSISHIELD)
        && can_cast_spell(ch, SPELL_PSIONIC_SHATTER)) {
        cast_spell(ch, vict, NULL, NULL, SPELL_PSIONIC_SHATTER);
        return true;
    }
    // Prioritize healing with aggression
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) * MIN(20, MAX(80, aggression)) / 100)
        && can_cast_spell(ch, SPELL_WOUND_CLOSURE)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_WOUND_CLOSURE);
        return true;
    }

    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return true;
        } else if (GET_POSITION(vict) > POS_SITTING
            && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP);
            return true;
        }
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (GET_MANA(ch) < GET_MAX_MANA(ch) / 2
            && can_cast_spell(ch, SKILL_PSIDRAIN)
            && can_psidrain(ch, vict, NULL, false)) {
            perform_psidrain(ch, vict);
            return true;
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_CRUSH)
            && can_cast_spell(ch, SPELL_PSYCHIC_CRUSH)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_CRUSH);
            return true;
        } else if (!affected_by_spell(vict, SPELL_MOTOR_SPASM)
            && can_cast_spell(ch, SPELL_MOTOR_SPASM)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MOTOR_SPASM);
            return true;
        } else if (!affected_by_spell(ch, SPELL_ADRENALINE)
            && can_cast_spell(ch, SPELL_ADRENALINE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_ADRENALINE);
            return true;
        } else if (GET_POSITION(vict) > POS_SITTING
            && can_cast_spell(ch, SPELL_EGO_WHIP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_EGO_WHIP);
            return true;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return true;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (IS_PSIONIC(vict)
            && !affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE)
            && can_cast_spell(ch, SPELL_PSYCHIC_RESISTANCE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_PSYCHIC_RESISTANCE);
            return true;
        } else if (!IS_CONFUSED(vict)
            && can_cast_spell(ch, SPELL_CONFUSION)
            && (IS_MAGE(vict) || IS_PSIONIC(vict) || IS_CLERIC(vict) ||
                IS_KNIGHT(vict) || IS_PHYSIC(vict))) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CONFUSION);
            return true;
        } else if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4
            && can_cast_spell(ch, SPELL_ENDURANCE)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_ENDURANCE);
            return true;
        } else if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4
            && can_cast_spell(ch, SPELL_DERMAL_HARDENING)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_DERMAL_HARDENING);
            return true;
        } else if (!AFF2_FLAGGED(ch, AFF2_VERTIGO)
            && can_cast_spell(ch, SPELL_VERTIGO)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_VERTIGO);
            return true;
        } else if (!affected_by_spell(vict, SPELL_PSYCHIC_FEEDBACK)
            && can_cast_spell(ch, SPELL_PSYCHIC_FEEDBACK)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_PSYCHIC_FEEDBACK);
            return true;
        } else if (!affected_by_spell(vict, SPELL_WEAKNESS)
            && can_cast_spell(ch, SPELL_WEAKNESS)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_WEAKNESS);
            return true;
        } else if (!affected_by_spell(vict, SPELL_CLUMSINESS)
            && can_cast_spell(ch, SPELL_CLUMSINESS)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CLUMSINESS);
            return true;
        } else if (can_cast_spell(ch, SKILL_PSIBLAST)) {
            perform_offensive_skill(ch, vict, SKILL_PSIBLAST);
            return true;
        }
    }
    if (aggression > 5) {
        // attempt to neutralize or get away
        if (GET_POSITION(vict) > POS_SLEEPING
            && can_cast_spell(ch, SPELL_MELATONIC_FLOOD)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_MELATONIC_FLOOD);
            return true;
        } else if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL);
            return true;
        } else if (can_cast_spell(ch, SPELL_AMNESIA)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_AMNESIA);
            return true;
        } else if (can_cast_spell(ch, SPELL_FEAR)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_FEAR);
            return true;
        }
    }

    return false;
}
