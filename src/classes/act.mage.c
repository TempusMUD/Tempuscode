//
// File: act.mage.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
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
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "house.h"
#include "char_class.h"
#include "language.h"
#include "fight.h"

#define MSHIELD_USAGE "usage: mshield <low|percent> <value>\r\n"
ACMD(do_mshield)
{
    char *arg1, *arg2;
    int i = 0;

    if (!affected_by_spell(ch, SPELL_MANA_SHIELD)) {
        send_to_char(ch,
            "You are not under the affects of a mana shield.\r\n");
        return;
    }

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (!*arg1 || !*arg2) {
        send_to_char(ch, MSHIELD_USAGE);
        return;
    }

    if (is_abbrev(arg1, "low")) {
        i = atoi(arg2);
        if (i < 0 || i > GET_MANA(ch)) {
            send_to_char(ch,
                "The low must be between 0 and your current mana.\r\n");
            return;
        }
        GET_MSHIELD_LOW(ch) = i;
        send_to_char(ch, "Manashield low set to [%d].\r\n", i);
    } else if (is_abbrev(arg1, "percent")) {
        i = atoi(arg2);
        if (i < 1 || i > 100) {
            send_to_char(ch, "The percent must be between 1 and 100.\r\n");
            return;
        }
        if (GET_CLASS(ch) != CLASS_MAGE && i > 50) {
            send_to_char(ch, "You cannot set the percent higher than 50.\r\n");
            return;
        }
        GET_MSHIELD_PCT(ch) = i;
        send_to_char(ch, "Manashield percent set to [%d].\r\n", i);
    } else {
        send_to_char(ch, MSHIELD_USAGE);
    }

}

ACMD(do_empower)
{
    struct affected_type af, af2, af3;
    int val1, val2, old_mana;

    if (!IS_MAGE(ch)
        || affected_by_spell(ch, SKILL_RECONFIGURE)
        || CHECK_SKILL(ch, SKILL_EMPOWER) < number(50, 101)) {
        send_to_char(ch, "You fail to empower yourself.\r\n");
        return;
    }
    if (GET_HIT(ch) < 15 || GET_MOVE(ch) < 15) {
        send_to_char(ch,
            "You are unable to call upon any resources of power!\r\n");
        return;
    }
    old_mana = GET_MANA(ch);
    val1 = MIN(GET_LEVEL(ch), (GET_HIT(ch) >> 2));
    val2 = MIN(GET_LEVEL(ch), (GET_MOVE(ch) >> 2));

    af.level = af2.level = af3.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);
    af.bitvector = af2.bitvector = af3.bitvector = 0;
    af.is_instant = af2.is_instant = af3.is_instant = false;

    af.type = SKILL_EMPOWER;
    af.duration = (GET_INT(ch) >> 1);
    af.location = APPLY_MANA;
    af.modifier = (val1 + val2 - 5);
    af.owner = GET_IDNUM(ch);

    af2.type = SKILL_EMPOWER;
    af2.duration = (GET_INT(ch) >> 1);
    af2.location = APPLY_HIT;
    af2.modifier = -(val1);
    af2.owner = GET_IDNUM(ch);

    af3.type = SKILL_EMPOWER;
    af3.duration = (GET_INT(ch) >> 1);
    af3.location = APPLY_MOVE;
    af3.modifier = -(val2);
    af3.owner = GET_IDNUM(ch);

    struct affected_type *cur_aff;
    int aff_power;

    for (cur_aff = ch->affected; cur_aff; cur_aff = cur_aff->next) {
        if (cur_aff->type == SKILL_EMPOWER && cur_aff->location == APPLY_MANA) {
            aff_power = cur_aff->modifier + val1 + val2 - 5;
            if (aff_power > 666) {
                send_to_char(ch, "You are already fully empowered.\r\n");
                return;
            }
        }
    }
    affect_join(ch, &af, 0, 0, 1, 0);

    GET_MANA(ch) = MIN((old_mana + val2 + val1), GET_MAX_MANA(ch));
    send_to_char(ch, "You redirect your energies!\r\n");
    affect_join(ch, &af2, 0, 0, 1, 0);
    affect_join(ch, &af3, 0, 0, 1, 0);

    if ((GET_MAX_HIT(ch) >> 1) < GET_WIMP_LEV(ch)) {
        send_to_char(ch,
            "Your wimpy level has been changed from %d to %d ... wimp!\r\n",
            GET_WIMP_LEV(ch), GET_MAX_HIT(ch) >> 1);
        GET_WIMP_LEV(ch) = GET_MAX_HIT(ch) >> 1;
    }
    act("$n concentrates deeply.", true, ch, 0, 0, TO_ROOM);
    if (GET_LEVEL(ch) < LVL_GRGOD)
        WAIT_STATE(ch, PULSE_VIOLENCE * (1 + ((val1 + val2) > 100)));
}

// It's kind of silly to put this here, but I couldn't think of a
// better spot
ACMD(do_teach)
{
    struct creature *target;
    char *s;
    char *skill_str, *target_str;
    const char *skill_name;
    int num;
    bool is_skill;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char(ch, "Usage: teach <skill/spell/language> <player>\r\n");
        return;
    }
    // There must be exactly 0 or exactly 2 's in the argument
    if ((s = strstr(argument, "\'"))) {
        if (!(strstr(s + 1, "\'"))) {
            send_to_char(ch, "The skill name must be completely enclosed "
                "in the symbols: '\n");
            return;
        } else {
            skill_str = tmp_getquoted(&argument);
        }
    } else {
        s = strrchr(argument, ' ');
        if (!s) {
            send_to_char(ch, "You must specify who to teach!\r\n");
            return;
        }

        *s = '\0';
        skill_str = argument;
        argument = s + 1;
    }

    target_str = tmp_getword(&argument);
    if (!*target_str) {
        send_to_char(ch, "You should specify who you want to teach.\r\n");
        return;
    }
    target = get_char_room_vis(ch, target_str);
    if (!target) {
        send_to_char(ch, "You don't see anyone here named '%s'.\r\n",
            target_str);
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char(ch, "You can't teach PCs!\r\n");
        return;
    }
    if (IS_NPC(target)) {
        send_to_char(ch, "You can't teach NPCs a thing.\r\n");
        return;
    }

    if (ch == target) {
        send_to_char(ch,
            "You teach yourself with experience, not lessons.\r\n");
        return;
    }

    if ((num = find_tongue_idx_by_name(skill_str)) != TONGUE_NONE) {
        is_skill = false;
        skill_name = tongue_name(num);
        if (CHECK_TONGUE(target, num) >= CHECK_TONGUE(ch, num) / 2) {
            act(tmp_sprintf
                ("$E already knows as much as you can teach $M of '%s'.",
                    skill_name), false, ch, 0, target, TO_CHAR);
            return;
        }
    } else if ((num = find_skill_num(skill_str)) != -1) {
        is_skill = true;
        skill_name = spell_to_str(num);
        if (!is_able_to_learn(target, num)) {
            act(tmp_sprintf("$E isn't ready to learn '%s'.", skill_name),
                false, ch, 0, target, TO_CHAR);
            return;
        }
        if (CHECK_SKILL(target, num) >= CHECK_SKILL(ch, num) / 2) {
            act(tmp_sprintf
                ("$E already knows as much as you can teach $M of '%s'.",
                    skill_name), false, ch, 0, target, TO_CHAR);
            return;
        }
    } else {
        send_to_char(ch, "You don't know of any such ability.\r\n");
        return;
    }

    WAIT_STATE(ch, 2 RL_SEC);

    if (number(0,
            100) >
        GET_WIS(ch) + GET_INT(ch) + GET_WIS(target) + GET_INT(target)) {
        // Teaching failure
        act(tmp_sprintf
            ("You try to teach '%s' to $N, but $E doesn't seem to get it.",
                skill_name), false, ch, 0, target, TO_CHAR);
        act(tmp_sprintf
            ("$n tries to teach you '%s', but you don't really understand the lesson.",
                skill_name), false, ch, 0, target, TO_VICT);
        act("$n gives a lesson to $N.", false, ch, 0, target, TO_NOTVICT);
        return;
    }
    // Teaching success
    if (is_skill) {
        SET_SKILL(target, num, MIN(CHECK_SKILL(ch, num) / 2,
                CHECK_SKILL(target, num)
                + number(1, GET_INT(target))));
    } else {
        SET_TONGUE(target, num, MIN(CHECK_TONGUE(ch, num) / 2,
                CHECK_TONGUE(target, num)
                + number(1, GET_INT(target))));
    }

    act(tmp_sprintf("You give a quick lesson to $N on '%s'.", skill_name),
        false, ch, 0, target, TO_CHAR);
    act(tmp_sprintf("$n gives you a quick lesson on '%s'.", skill_name),
        false, ch, 0, target, TO_VICT);
    act("$n gives a lesson to $N.", false, ch, 0, target, TO_NOTVICT);
}

bool
area_attack_advisable(struct creature *ch)
{
    // Area attacks are advisable when there are more than one PC and
    // no other non-fighting NPCs
    int pc_count = 0;
    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (can_see_creature(ch, tch)
            && !(IS_NPC(tch) && tch->fighting))
            pc_count++;
    }

    return (pc_count > 1);
}

bool
group_attack_advisable(struct creature * ch)
{
    int attacker_count = 0;

    // Group attacks are advisable when more than one creature is
    // attacking
    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;
        if (!g_list_find(tch->fighting, ch))
            continue;
        if (attacker_count)
            return true;
        attacker_count++;
    }

    return false;
}

// mob ai...
// fear - how much the mob thinks it is going to die
// hate - how much the mob wants to kill
//

// return true if the attack was made, otherwise return false
bool
mage_damaging_attack(struct creature * ch, struct creature * vict)
{
    if (area_attack_advisable(ch)
        && can_cast_spell(ch, SPELL_METEOR_STORM))
        cast_spell(ch, NULL, NULL, NULL, SPELL_METEOR_STORM);
    else if (group_attack_advisable(ch)
        && can_cast_spell(ch, SPELL_CHAIN_LIGHTNING))
        cast_spell(ch, NULL, NULL, NULL, SPELL_CHAIN_LIGHTNING);
    else if (can_cast_spell(ch, SPELL_LIGHTNING_BOLT) && IS_CYBORG(vict))
        cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT);
    else if (can_cast_spell(ch, SPELL_PRISMATIC_SPRAY))
        cast_spell(ch, vict, NULL, NULL, SPELL_PRISMATIC_SPRAY);
    else if (can_cast_spell(ch, SPELL_CONE_COLD))
        cast_spell(ch, vict, NULL, NULL, SPELL_CONE_COLD);
    else if (can_cast_spell(ch, SPELL_FIREBALL))
        cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL);
    else if (can_cast_spell(ch, SPELL_ENERGY_DRAIN))
        cast_spell(ch, vict, NULL, NULL, SPELL_ENERGY_DRAIN);
    else if (can_cast_spell(ch, SPELL_COLOR_SPRAY))
        cast_spell(ch, vict, NULL, NULL, SPELL_COLOR_SPRAY);
    else if (can_cast_spell(ch, SPELL_LIGHTNING_BOLT))
        cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT);
    else if (can_cast_spell(ch, SPELL_BURNING_HANDS)
        && !CHAR_WITHSTANDS_FIRE(vict))
        cast_spell(ch, vict, NULL, NULL, SPELL_BURNING_HANDS);
    else if (can_cast_spell(ch, SPELL_SHOCKING_GRASP))
        cast_spell(ch, vict, NULL, NULL, SPELL_SHOCKING_GRASP);
    else if (can_cast_spell(ch, SPELL_CHILL_TOUCH))
        cast_spell(ch, vict, NULL, NULL, SPELL_CHILL_TOUCH);
    else if (can_cast_spell(ch, SPELL_MAGIC_MISSILE))
        cast_spell(ch, vict, NULL, NULL, SPELL_MAGIC_MISSILE);
    else
        return false;
    return true;
}

bool
dispel_is_advisable(struct creature *vict)
{
    int blessings = 0;
    int curses = 0;

    // Return true if magical debuffs are found
    for (struct affected_type *af = vict->affected;af;af = af->next) {
        if (SPELL_IS_MAGIC(af->type) || SPELL_IS_DIVINE(af->type)) {
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
mage_best_attack(struct creature *ch, struct creature *vict)
{
    int calculate_mob_aggression(struct creature *, struct creature *);
    int aggression = calculate_mob_aggression(ch, vict);

    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (mage_damaging_attack(ch, vict))
            return;
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (GET_POSITION(vict) > POS_SLEEPING
            && can_cast_spell(ch, SPELL_WORD_STUN)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_WORD_STUN);
            return;
        } else if (GET_POSITION(vict) > POS_SLEEPING
            && can_cast_spell(ch, SPELL_SLEEP)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_SLEEP);
            return;
        } else if (!AFF_FLAGGED(vict, AFF_BLIND)
            && can_cast_spell(ch, SPELL_BLINDNESS)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_BLINDNESS);
            return;
        } else if (!AFF_FLAGGED(vict, AFF_CURSE)
            && can_cast_spell(ch, SPELL_CURSE)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_CURSE);
            return;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (can_cast_spell(ch, SPELL_DISPEL_MAGIC)
            && dispel_is_advisable(vict))
            cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_MAGIC);
        else if (!AFF2_FLAGGED(vict, AFF2_SLOW)
            && can_cast_spell(ch, SPELL_SLOW))
            cast_spell(ch, vict, NULL, NULL, SPELL_SLOW);
        else if (mage_damaging_attack(ch, vict))
            return;
    }
    if (aggression > 5) {
        if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL);
            return;
        } else if (can_cast_spell(ch, SPELL_TELEPORT)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_TELEPORT);
            return;
        } else if (can_cast_spell(ch, SPELL_LOCAL_TELEPORT)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_LOCAL_TELEPORT);
            return;
        }
    }
    // desperation - just attack full force, as hard as possible
    if (mage_damaging_attack(ch, vict))
        return;
    else if (can_cast_spell(ch, SKILL_PUNCH))
        perform_offensive_skill(ch, vict, SKILL_PUNCH);
    else
        hit(ch, vict, TYPE_UNDEFINED);
}

void
mage_activity(struct creature *ch)
{
    if (room_is_dark(ch->in_room) &&
        can_cast_spell(ch, SPELL_INFRAVISION) && !has_dark_sight(ch)) {
        cast_spell(ch, ch, 0, NULL, SPELL_INFRAVISION);
    } else if (room_is_dark(ch->in_room) &&
        can_cast_spell(ch, SPELL_GLOWLIGHT) && !has_dark_sight(ch)) {
        cast_spell(ch, ch, 0, NULL, SPELL_GLOWLIGHT);
    } else if (can_cast_spell(ch, SPELL_PRISMATIC_SPHERE)
        && !AFF3_FLAGGED(ch, AFF3_PRISMATIC_SPHERE)) {
        cast_spell(ch, ch, 0, NULL, SPELL_PRISMATIC_SPHERE);
    } else if (can_cast_spell(ch, SPELL_ANTI_MAGIC_SHELL)
        && !affected_by_spell(ch, SPELL_ANTI_MAGIC_SHELL)) {
        cast_spell(ch, ch, 0, NULL, SPELL_ANTI_MAGIC_SHELL);
    } else if (can_cast_spell(ch, SPELL_HASTE)
        && !AFF2_FLAGGED(ch, AFF2_HASTE)) {
        cast_spell(ch, ch, 0, NULL, SPELL_HASTE);
    } else if (can_cast_spell(ch, SPELL_DISPLACEMENT)
        && !AFF2_FLAGGED(ch, AFF2_DISPLACEMENT)) {
        cast_spell(ch, ch, 0, NULL, SPELL_DISPLACEMENT);
    } else if (can_cast_spell(ch, SPELL_TRUE_SEEING)
        && !AFF2_FLAGGED(ch, AFF2_TRUE_SEEING)) {
        cast_spell(ch, ch, 0, NULL, SPELL_TRUE_SEEING);
    } else if (can_cast_spell(ch, SPELL_REGENERATE)
        && !AFF_FLAGGED(ch, AFF_REGEN)) {
        cast_spell(ch, ch, 0, NULL, SPELL_REGENERATE);
    } else if (can_cast_spell(ch, SPELL_FIRE_SHIELD)
        && !AFF2_FLAGGED(ch, AFF2_FIRE_SHIELD)) {
        cast_spell(ch, ch, 0, NULL, SPELL_FIRE_SHIELD);
    } else if (can_cast_spell(ch, SPELL_STRENGTH)
        && !affected_by_spell(ch, SPELL_STRENGTH)) {
        cast_spell(ch, ch, 0, NULL, SPELL_STRENGTH);
    } else if (can_cast_spell(ch, SPELL_BLUR) && !AFF_FLAGGED(ch, AFF_BLUR)) {
        cast_spell(ch, ch, 0, NULL, SPELL_BLUR);
    } else if (can_cast_spell(ch, SPELL_ARMOR)
        && !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, 0, NULL, SPELL_ARMOR);
    }
}

bool
mage_mob_fight(struct creature *ch, struct creature *precious_vict)
{
    int calculate_mob_aggression(struct creature *ch, struct creature *vict);
    struct creature *vict = NULL;

    if (!is_fighting(ch))
        return false;

    // pick an enemy
    if (!(vict = choose_opponent(ch, precious_vict)))
        return false;

    int aggression = calculate_mob_aggression(ch, vict);

    if (aggression > 75) {
        // extremely aggressive - just attack hard
        if (mage_damaging_attack(ch, vict))
            return true;
    }
    if (aggression > 50) {
        // somewhat aggressive - balance attacking with crippling
        if (!AFF2_FLAGGED(vict, AFF2_SLOW)
            && can_cast_spell(ch, SPELL_SLOW)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_SLOW);
            return true;
        }
    }
    if (aggression > 25) {
        // not very aggressive - play more defensively
        if (can_cast_spell(ch, SPELL_FIRE_SHIELD)
            && !AFF2_FLAGGED(ch, AFF2_FIRE_SHIELD)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_FIRE_SHIELD);
            return true;
        }
        if (can_cast_spell(ch, SPELL_BLUR) && !AFF_FLAGGED(ch, AFF_BLUR)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_BLUR);
            return true;
        }
        if (can_cast_spell(ch, SPELL_ARMOR)
            && !affected_by_spell(ch, SPELL_ARMOR)) {
            cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR);
            return true;
        }
        if (mage_damaging_attack(ch, vict))
            return true;
    }
    if (aggression > 5) {
        // attempt to neutralize or get away
        if (can_cast_spell(ch, SPELL_ASTRAL_SPELL)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_ASTRAL_SPELL);
            return true;
        } else if (can_cast_spell(ch, SPELL_TELEPORT)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_TELEPORT);
            return true;
        } else if (can_cast_spell(ch, SPELL_LOCAL_TELEPORT)) {
            cast_spell(ch, vict, NULL, NULL, SPELL_LOCAL_TELEPORT);
            return true;
        }
    }

    if (mage_damaging_attack(ch, vict))
        return true;
    return false;
}
