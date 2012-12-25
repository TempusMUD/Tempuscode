//
// File: act.hood.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
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
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "materials.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "strutil.h"

int check_mob_reaction(struct creature *ch, struct creature *vict);
int apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
    int pos);

ACMD(do_taunt)
{
    send_to_char(ch, "You taunt them mercilessly!\r\n");
}

struct obj_data *
find_hamstring_weapon(struct creature *ch)
{
    struct obj_data *weap = NULL;
    if ((weap = GET_EQ(ch, WEAR_WIELD)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_WIELD_2)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_HANDS)) &&
        IS_OBJ_TYPE(weap, ITEM_WEAPON) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_ARMS)) &&
        IS_OBJ_TYPE(weap, ITEM_WEAPON) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_IMPLANT(ch, WEAR_HANDS)) &&
        IS_OBJ_TYPE(weap, ITEM_WEAPON) && is_slashing_weapon(weap) &&
        GET_EQ(ch, WEAR_HANDS) == NULL) {
        return weap;
    } else if ((weap = GET_IMPLANT(ch, WEAR_ARMS)) &&
        IS_OBJ_TYPE(weap, ITEM_WEAPON) && is_slashing_weapon(weap) &&
        GET_EQ(ch, WEAR_ARMS) == NULL) {
        return weap;
    }
    return NULL;
}

ACMD(do_hamstring)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;
    struct affected_type af;
    char *arg;

    init_affect(&af);

    arg = tmp_getword(&argument);
    if (CHECK_SKILL(ch, SKILL_HAMSTRING) < 50) {
        send_to_char(ch,
            "Even if you knew what that was, you wouldn't do it.\r\n");
        return;
    }

    if (IS_CLERIC(ch) && IS_GOOD(ch)) {
        send_to_char(ch, "Your deity forbids this.\r\n");
        return;
    }
    // If there's noone in the room that matches your alias
    // Then it must be an object.
    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else {
            if ((ovict = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
                act("You open a deep gash in $p's hamstring!", false, ch,
                    ovict, NULL, TO_CHAR);
                return;
            } else {
                send_to_char(ch, "Hamstring who?\r\n");
                return;
            }
        }
    }

    weap = find_hamstring_weapon(ch);
    if (weap == NULL) {
        send_to_char(ch, "You need to be using a slashing weapon.\r\n");
        return;
    }
    if (vict == ch) {
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            act("You fear that your death will grieve $N.",
                false, ch, NULL, ch->master, TO_CHAR);
            return;
        }
        send_to_char(ch,
            "Cutting off your own leg just doesn't sound like fun.\r\n");
        return;
    }
    if (!ok_to_attack(ch, vict, true))
        return;

    if (GET_POSITION(vict) == POS_SITTING) {
        send_to_char(ch, "How can you cut it when they're sitting on it!\r\n");
        return;
    }
    if (GET_POSITION(vict) == POS_RESTING) {
        send_to_char(ch, "How can you cut it when they're laying on it!\r\n");
        return;
    }
    prob = CHECK_SKILL(ch, SKILL_HAMSTRING) + GET_REMORT_GEN(ch);
    percent = number(0, 125);
    if (affected_by_spell(vict, ZEN_AWARENESS)) {
        percent += 25;
    }
    if (AFF2_FLAGGED(vict, AFF2_HASTE) && !AFF2_FLAGGED(ch, AFF2_HASTE)) {
        percent += 30;
    }

    if (GET_DEX(ch) > GET_DEX(vict)) {
        prob += 3 * (GET_DEX(ch) - GET_DEX(vict));
    } else {
        percent += 3 * (GET_DEX(vict) - GET_DEX(ch));
    }
    // If they're wearing anything usefull on thier legs make it harder to hurt em.
    if ((ovict = GET_EQ(vict, WEAR_LEGS)) && IS_OBJ_TYPE(ovict, ITEM_ARMOR)) {
        if (IS_STONE_TYPE(ovict) || IS_METAL_TYPE(ovict))
            percent += GET_OBJ_VAL(ovict, 0) * 3;
        else
            percent += GET_OBJ_VAL(ovict, 0);
    }

    if (GET_LEVEL(ch) > GET_LEVEL(vict)) {
        prob += GET_LEVEL(ch) - GET_LEVEL(vict);
    } else {
        percent += GET_LEVEL(vict) - GET_LEVEL(ch);
    }

    if (IS_PUDDING(vict) || IS_SLIME(vict)
        || NON_CORPOREAL_MOB(vict) || IS_ELEMENTAL(vict))
        prob = 0;
    if (CHECK_SKILL(ch, SKILL_HAMSTRING) < 30) {
        prob = 0;
    }

    if (percent > prob) {
        damage(ch, vict, weap, 0, SKILL_HAMSTRING, WEAR_LEGS);
        WAIT_STATE(ch, 2 RL_SEC);
        return;
    } else {
        int level = 0, gen = 0;
        level = GET_LEVEL(ch);
        gen = GET_REMORT_GEN(ch);
        // For proper behavior, the mob has to be set sitting _before_ we
        // damage it.  However, we don't want it to deal the extra damage.
        // Sitting damage is 5/3rds of fighting damage, so we multiply by
        // 3/5ths to cancel out the multiplier
        dam = dice(level, 15 + gen / 2) * 3 / 5;
        add_blood_to_room(vict->in_room, 1);
        apply_soil_to_char(vict, GET_EQ(vict, WEAR_LEGS), SOIL_BLOOD,
            WEAR_LEGS);
        apply_soil_to_char(vict, GET_EQ(vict, WEAR_FEET), SOIL_BLOOD,
            WEAR_FEET);
        if (!affected_by_spell(vict, SKILL_HAMSTRING)) {
            af.type = SKILL_HAMSTRING;
            af.bitvector = AFF3_HAMSTRUNG;
            af.aff_index = 3;
            af.level = level + gen;
            af.duration = level + gen / 10;
            af.location = APPLY_DEX;
            af.modifier = 0 - (level / 2 + dice(7, 7) + dice(gen, 5))
                * (CHECK_SKILL(ch, SKILL_HAMSTRING)) / 1000;
            af.owner = GET_IDNUM(ch);
            affect_to_char(vict, &af);
            WAIT_STATE(vict, 3 RL_SEC);
            GET_POSITION(vict) = POS_RESTING;
            damage(ch, vict, weap, dam, SKILL_HAMSTRING, WEAR_LEGS);
        } else {
            WAIT_STATE(vict, 2 RL_SEC);
            GET_POSITION(vict) = POS_SITTING;
            damage(ch, vict, weap, dam / 2, SKILL_HAMSTRING, WEAR_LEGS);
        }
        if (!is_dead(ch)) {
            gain_skill_prof(ch, SKILL_HAMSTRING);
            WAIT_STATE(ch, 5 RL_SEC);
        }
    }
}

ACMD(do_drag_char)
{
    struct creature *vict = NULL;
    struct room_data *target_room = NULL;

    int percent, prob;
    char *arg, *arg2;

    int dir = -1;

    arg = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "Who do you want to drag?\r\n");
        WAIT_STATE(ch, 3);
        return;
    }

    if (vict == ch) {
        send_to_char(ch, "You can't drag yourself!\r\n");
        return;
    }

    if (!*arg2) {
        send_to_char(ch, "Which direction do you wish to drag them?\r\n");
        WAIT_STATE(ch, 3);
        return;
    }

    if (!ok_to_attack(ch, vict, true)) {
        return;
    }
    // Find out which direction the player wants to drag in
    dir = search_block(arg2, dirs, false);
    if (dir < 0) {
        send_to_char(ch, "Sorry, that's not a valid direction.\r\n");
        return;
    }

    if (!CAN_GO(ch, dir)
        || !can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0)
        || !CAN_GO(vict, dir)) {
        send_to_char(ch, "Sorry you can't go in that direction.\r\n");
        return;
    }

    target_room = EXIT(ch, dir)->to_room;

    if ((ROOM_FLAGGED(target_room, ROOM_HOUSE)
            && !can_enter_house(ch, target_room->number))
        || (ROOM_FLAGGED(target_room, ROOM_CLAN_HOUSE)
            && !clan_house_can_enter(ch, target_room))
        || (ROOM_FLAGGED(target_room, ROOM_DEATH))
        || is_npk_combat(ch, vict)) {
        act("You are unable to drag $M there.", false, ch, NULL, vict, TO_CHAR);
        return;
    }

    percent = ((GET_LEVEL(vict)) + number(1, 101));
    percent -= (GET_WEIGHT(ch) - GET_WEIGHT(vict)) / 5;
    percent -= (GET_STR(ch));

    prob =
        MAX(0, (GET_LEVEL(ch) + (CHECK_SKILL(ch,
                    SKILL_DRAG)) - GET_STR(vict)));
    prob = MIN(prob, 100);

    if (NPC_FLAGGED(vict, NPC_SENTINEL)) {
        percent = 101;
    }

    if (CHECK_SKILL(ch, SKILL_DRAG) < 30) {
        percent = 101;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        percent = 101;
    }

    if (prob > percent) {

        act(tmp_sprintf("You drag $N to the %s.", to_dirs[dir]),
            false, ch, NULL, vict, TO_CHAR);
        act(tmp_sprintf("$n grabs you and drags you %s.", to_dirs[dir]),
            false, ch, NULL, vict, TO_VICT);
        act(tmp_sprintf("$n drags $N to the %s.", to_dirs[dir]),
            false, ch, NULL, vict, TO_NOTVICT);

        perform_move(ch, dir, MOVE_NORM, 1);
        perform_move(vict, dir, MOVE_DRAG, 1);

        WAIT_STATE(ch, (PULSE_VIOLENCE * 2));
        WAIT_STATE(vict, PULSE_VIOLENCE);

        if (IS_NPC(vict) && AWAKE(vict) && check_mob_reaction(ch, vict)) {
            hit(vict, ch, TYPE_UNDEFINED);
            WAIT_STATE(ch, 2 RL_SEC);
        }
        return;
    } else {
        act("$n grabs $N but fails to move $m.", false, ch, NULL, vict,
            TO_NOTVICT);
        act("You attempt to man-handle $N but you fail!", false, ch, NULL, vict,
            TO_CHAR);
        act("$n attempts to drag you, but you hold your ground.", false, ch, NULL,
            vict, TO_VICT);
        WAIT_STATE(ch, PULSE_VIOLENCE);

        if (IS_NPC(vict) && AWAKE(vict) && check_mob_reaction(ch, vict)) {
            hit(vict, ch, TYPE_UNDEFINED);
            WAIT_STATE(ch, 2 RL_SEC);
        }
        return;
    }
}

ACMD(do_snatch)
{
    //
    //  Check to make sure they have a free hand!
    //
    struct creature *vict = NULL;
    struct obj_data *obj, *sec_weap;
    char vict_name[MAX_INPUT_LENGTH];
    int percent = -1, eq_pos = -1;
    int prob = 0;
    int position = 0;
    int dam = 0;
    int bonus, dex_mod, str_mod;
    obj = NULL;
    one_argument(argument, vict_name);

    if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char(ch, "Who do you want to snatch from?\r\n");
        return;
    } else if (vict == ch) {
        send_to_char(ch, "Come on now, that's rather stupid!\r\n");
        return;
    }

    if (CHECK_SKILL(ch, SKILL_SNATCH) < 50) {
        send_to_char(ch,
            "I don't think they are going to let you do that.\r\n");
        return;
    }

    if (is_newbie(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You cannot snatch from newbies!\r\n");
        return;
    }
    if (!IS_NPC(vict) && is_newbie(ch)) {
        send_to_char(ch,
            "You can't snatch from players. You're a newbie!\r\n");
        return;
    }

    if (!IS_NPC(vict) && !vict->desc && GET_LEVEL(ch) < LVL_ELEMENT) {
        send_to_char(ch, "You cannot snatch from linkless players!!!\r\n");
        mudlog(GET_LEVEL(ch), CMP, true,
            "%s attempted to snatch from linkless %s.",
            GET_NAME(ch), GET_NAME(vict));
        return;
    }

    check_thief(ch, vict);

    // Figure out what we're gonna snatch from them.
    // Possible targets are anything in hands or on belt,
    //         including inventory.
    // Possible locations:
    // (in order of precedence)
    //        - Hold
    //        - Light
    //        - Shield
    //        - Belt
    //        - Wield
    //        - SecondWield

    position = number(1, 100);

    if (position < 20) {        // Hold
        obj = GET_EQ(vict, WEAR_HOLD);
        eq_pos = WEAR_HOLD;
    }
    if (!obj && position < 35) {    // Shield
        obj = GET_EQ(vict, WEAR_SHIELD);
        eq_pos = WEAR_SHIELD;
    }
    if (!obj && position < 55) {    // Belt
        obj = GET_EQ(vict, WEAR_BELT);
        eq_pos = WEAR_BELT;
    }
    if (!obj && position < 70) {    // Light
        obj = GET_EQ(vict, WEAR_LIGHT);
        eq_pos = WEAR_LIGHT;
    }
    if (!obj && position < 80) {    // Wield
        obj = GET_EQ(vict, WEAR_WIELD);
        eq_pos = WEAR_WIELD;
    }
    if (!obj && position < 90) {    // SecondWield
        obj = GET_EQ(vict, WEAR_WIELD_2);
        eq_pos = WEAR_WIELD_2;
    }
    if (!obj && position > 90) {    // Damn, inventory it is.
        // Screw inventory for the time being.
    }
    // Roll the dice...
    percent = number(1, 100);

    if (GET_POSITION(vict) < POS_SLEEPING)
        percent = -150;         // ALWAYS SUCCESS

    if (is_fighting(ch))
        percent += 30;

    if (GET_POSITION(vict) < POS_FIGHTING)
        percent -= 30;
    if (GET_POSITION(vict) <= POS_SLEEPING)
        percent = -150;         // ALWAYS SUCCESS

    // NO NO With Imp's and Shopkeepers!
    if (!ok_damage_vendor(ch, vict) || (IS_NPC(vict)
            && NPC_FLAGGED(vict, NPC_UTILITY)))
        percent = 121;          // Failure

    // Mod the percentage based on position and flags
    if (obj) {                  // If hes got his hand on something,
        //lets see if he gets it.
        if (IS_OBJ_STAT(obj, ITEM_NODROP))
            percent += 10;
        if (IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM))
            percent += 10;
        if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE))
            percent += 15;
        if (GET_LEVEL(ch) > LVL_TIMEGOD && GET_LEVEL(vict) < GET_LEVEL(ch))
            percent = 0;
        // Snatch the pebble from my hand grasshopper.
        if (IS_MONK(vict) || IS_THIEF(vict))
            percent += 10;
        if (affected_by_spell(vict, ZEN_AWARENESS))
            percent += 10;
    }
    // Check the percentage against the current skill.
    // If they succeed the leftover is thier bonus.
    // If they fail thier roll, or choose an empty position
    if (percent > CHECK_SKILL(ch, SKILL_SNATCH) || !obj) {
        act("$n tries to snatch something from $N but comes away empty handed!", false, ch, NULL, vict, TO_NOTVICT);
        act("$n tries to snatch something from you but comes away empty handed!", false, ch, NULL, vict, TO_VICT);
        act("You try to snatch something from $N but come away empty handed!",
            false, ch, NULL, vict, TO_CHAR);

        // Monks are cool. They stand up when someone tries to snatch from em.
        if (GET_POSITION(vict) == POS_SITTING
            && AFF2_FLAGGED(vict, AFF2_MEDITATE)) {
            GET_POSITION(vict) = POS_STANDING;
            act("You jump to your feet, glaring at $s!", false, ch, NULL, vict,
                TO_VICT);
            act("$N jumps to $S feet, glaring at You!", false, ch, NULL, vict,
                TO_CHAR);
            act("$N jumps to $S feet, glaring at $n!", false, ch, NULL, vict,
                TO_NOTVICT);
        }

    } else {                    // They got a hand on it, and it exists.
        bonus = CHECK_SKILL(ch, SKILL_SNATCH) - percent;

        // Now that the hood has a hand on, contest to see who ends up with the item.
        // Note: percent has started from scratch here.
        // str_mod and dex_mod control how signifigant an effect differences in dex
        //    have on the probability of success.  At 7, its 5% likely from this point on
        //    that an average mort hood can wrench the hammer mjolnir from thor.
        str_mod = 7;
        dex_mod = 7;

        // This beastly function brought to you by Cat and the letter F.
        percent = (GET_LEVEL(ch) - GET_LEVEL(vict) * 2)
            + bonus + (dex_mod * (GET_DEX(ch) - GET_DEX(vict)))
            + (str_mod * (GET_STR(ch) - GET_STR(vict)));
        if (CHECK_SKILL(ch, SKILL_SNATCH) > 100)
            percent += CHECK_SKILL(ch, SKILL_SNATCH) - 100;
        prob = number(1, 100);
        if (PRF2_FLAGGED(ch, PRF2_DEBUG))
            send_to_char(ch, "%s[SNATCH] %s   chance:%d   roll:%d%s\r\n",
                CCCYN(ch, C_NRM), GET_NAME(ch), percent, prob,
                CCNRM(ch, C_NRM));
        if (PRF2_FLAGGED(vict, PRF2_DEBUG))
            send_to_char(vict, "%s[SNATCH] %s   chance:%d   roll:%d%s\r\n",
                CCCYN(vict, C_NRM), GET_NAME(ch), percent, prob,
                CCNRM(vict, C_NRM));

        //failure. hand on it and failed to take away.
        if (prob > percent) {

            // If it was close, drop the item.
            if (prob < percent + 10) {
                act("$n tries to take $p away from you, forcing you to drop it!", false, ch, obj, vict, TO_VICT);
                act("You try to snatch $p away from $N but $E drops it!",
                    false, ch, obj, vict, TO_CHAR);
                act("$n tries to snatch $p from $N but $E drops it!",
                    false, ch, obj, vict, TO_NOTVICT);

                // weapon is the 1st of 2 wielded, shift to the second weapon.
                if (obj->worn_on == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD_2)) {
                    sec_weap = unequip_char(vict, WEAR_WIELD_2, EQUIP_WORN);
                    obj_to_room(unequip_char(vict, eq_pos, EQUIP_WORN),
                        vict->in_room);
                    equip_char(vict, sec_weap, WEAR_WIELD, EQUIP_WORN);
                    act("You shift $p to your primary hand.",
                        false, ch, obj, vict, TO_VICT);

                    // Otherwise, just drop it.
                } else {
                    obj_to_room(unequip_char(vict, eq_pos, EQUIP_WORN),
                        vict->in_room);
                }

                // It wasnt close. He deffinately failed.
            } else {
                act("$n grabs your $p but you manage to hold onto it!",
                    false, ch, obj, vict, TO_VICT);
                act("You grab $S $p but can't snatch it away!",
                    false, ch, obj, vict, TO_CHAR);
                act("$n grabs $N's $p but can't snatch it away!",
                    false, ch, obj, vict, TO_NOTVICT);
            }

            // success, hand on and wrestled away.
        } else {

            // If he ends up with the shit, advertize.
            if (GET_POSITION(vict) == POS_SLEEPING) {   // He's sleepin, wake his ass up.
                GET_POSITION(vict) = POS_RESTING;
                if (eq_pos == WEAR_BELT) {
                    act("$n wakes you up snatching something off your belt!",
                        false, ch, obj, vict, TO_VICT);
                    act("You wake $N up snatching $p off $S belt!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n wakes $N up snatching $p off $S belt!",
                        false, ch, obj, vict, TO_NOTVICT);
                } else {
                    act("$n wakes you up snatching out of your hand!",
                        false, ch, obj, vict, TO_VICT);
                    act("You wake $N up snatching $p out of $S hand!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n wakes $N up snatching $p out of $S hand!",
                        false, ch, obj, vict, TO_NOTVICT);
                }
            } else if (GET_POSITION(vict) == POS_SITTING &&
                AFF2_FLAGGED(vict, AFF2_MEDITATE)) {
                GET_POSITION(vict) = POS_STANDING;
                if (eq_pos == WEAR_BELT) {
                    act("$n breaks your trance snatching $p off your belt!",
                        false, ch, obj, vict, TO_VICT);
                    act("You break $S trance snatching $p off $S belt!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n breaks $N's trance snatching $p off $S belt!",
                        false, ch, obj, vict, TO_NOTVICT);
                } else {
                    act("$n breaks your trance snatching $p out of your hand!",
                        false, ch, obj, vict, TO_VICT);
                    act("You break $N's trance snatching $p out of $S hand!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n breaks $N's trance snatching $p out of $S hand!",
                        false, ch, obj, vict, TO_NOTVICT);
                }
            } else {
                if (eq_pos == WEAR_BELT) {
                    act("$n snatches $p off your belt!",
                        false, ch, obj, vict, TO_VICT);
                    act("You snatch $p off $N's belt!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n snatches $p off $N's belt!",
                        false, ch, obj, vict, TO_NOTVICT);
                } else {
                    act("$n snatches $p out of your hand!",
                        false, ch, obj, vict, TO_VICT);
                    act("You snatch $p out of $N's hand!",
                        false, ch, obj, vict, TO_CHAR);
                    act("$n snatches $p out of $N's hand!",
                        false, ch, obj, vict, TO_NOTVICT);
                }
            }

            // Keep tabs on snatching stuff. :P
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR || !IS_NPC(vict))
                slog("%s stole %s from %s.",
                    GET_NAME(ch), obj->name, GET_NAME(vict));
            obj_to_char(unequip_char(vict, eq_pos, EQUIP_WORN), ch);

            // weapon is the 1st of 2 wielded
            if (obj->worn_on == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD_2)) {
                sec_weap = unequip_char(vict, WEAR_WIELD_2, EQUIP_WORN);
                equip_char(vict, sec_weap, WEAR_WIELD, EQUIP_WORN);
                act("You shift $p to your primary hand.",
                    false, ch, obj, vict, TO_VICT);
            }
            // Punks tend to break shit.
            dam =
                dice(strength_damage_bonus(GET_STR(ch)),
                     strength_damage_bonus(GET_STR(vict)));
            if (!is_arena_combat(ch, vict))
                damage_eq(NULL, obj, dam, TYPE_HIT);
            GET_EXP(ch) += MIN(1000, GET_OBJ_COST(obj));
            gain_skill_prof(ch, SKILL_SNATCH);
            WAIT_STATE(vict, 2 RL_SEC);
        }
    }
    if (IS_NPC(vict) && AWAKE(vict))
        hit(vict, ch, TYPE_UNDEFINED);
    WAIT_STATE(ch, 4 RL_SEC);
}
