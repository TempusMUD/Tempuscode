//
// File: act.merc.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif
#include <stdio.h>
#include <stdlib.h>
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
#include "libpq-fe.h"
#include "db.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "bomb.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "strutil.h"
#include "guns.h"

#define PISTOL(gun)  ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && !IS_TWO_HAND(gun))
#define LARGE_GUN(gun) ((IS_GUN(gun) || IS_ENERGY_GUN(gun)) && IS_TWO_HAND(gun))

int apply_soil_to_char(struct creature *ch, struct obj_data *obj, int type,
    int pos);

ACMD(do_pistolwhip)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int percent, prob, dam;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You pistol-whip $p!", false, ch, ovict, NULL, TO_CHAR);
            return;
        } else {
            send_to_char(ch, "Pistol-whip who?\r\n");
            return;
        }
    }
    if (!(((weap = GET_EQ(ch, WEAR_WIELD)) && PISTOL(weap)) ||
            ((weap = GET_EQ(ch, WEAR_WIELD_2)) && PISTOL(weap)) ||
            ((weap = GET_EQ(ch, WEAR_HANDS)) && PISTOL(weap)))) {
        send_to_char(ch, "You need to be using a pistol.\r\n");
        return;
    }
    if (vict == ch) {
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            act("You fear that your death will grieve $N.",
                false, ch, NULL, ch->master, TO_CHAR);
            return;
        }
        act("You slam $p into your head!", false, ch, weap, NULL, TO_CHAR);
        act("$n beats $mself senseless with $p!", true, ch, weap, NULL, TO_ROOM);
        return;
    }
    if (!ok_to_attack(ch, vict, true))
        return;

    percent = ((10 - (GET_AC(vict) / 10)) * 2) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_PISTOLWHIP);

    if (IS_PUDDING(vict) || IS_SLIME(vict)) {
        prob = 0;
    }

    if (percent > prob) {
        damage(ch, vict, weap, 0, SKILL_PISTOLWHIP, WEAR_BODY);
    } else {
        dam = dice(GET_LEVEL(ch), strength_damage_bonus(GET_STR(ch))) +
            dice(4, GET_OBJ_WEIGHT(weap));
        dam /= 4;
        damage(ch, vict, weap, dam, SKILL_PISTOLWHIP, WEAR_HEAD);
        gain_skill_prof(ch, SKILL_PISTOLWHIP);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_crossface)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL, *weap = NULL;
    int str_mod, dex_mod, percent = 0, prob = 0, dam = 0;
    int diff = 0, wear_num;
    bool prime_merc = false;
    short prev_pos = 0;
    char *arg;

    if (GET_CLASS(ch) == CLASS_MERCENARY)
        prime_merc = true;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = random_opponent(ch);
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You fiercely crossface $p!", false, ch, ovict, NULL, TO_CHAR);
            return;
        } else {
            send_to_char(ch, "Crossface who?\r\n");
            return;
        }
    }

    if (!((weap = GET_EQ(ch, WEAR_WIELD)) && LARGE_GUN(weap))) {
        send_to_char(ch, "You need to be using a two-handed gun.\r\n");
        return;
    }

    if (vict == ch) {
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            act("You fear that your death will grieve $N.",
                false, ch, NULL, ch->master, TO_CHAR);
            return;
        }
        act("You slam $p into your head!", false, ch, weap, NULL, TO_CHAR);
        act("$n beats $mself senseless with $p!", true, ch, weap, NULL, TO_ROOM);
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;

    if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
        act("$n catches the butt of your gun and smacks you silly!", true,
            ch, NULL, vict, TO_CHAR);
        act("$n gets smacked silly for trying to crossface $N!", true,
            ch, NULL, vict, TO_ROOM);
        WAIT_STATE(ch, PULSE_VIOLENCE * 8);
        return;
    }

    str_mod = 3;
    dex_mod = 3;
    // This beastly function brought to you by Cat, the letter F, and Nothing more
    prob =
        ((GET_LEVEL(ch) + level_bonus(ch, prime_merc)) - (GET_LEVEL(vict) * 2))
        + (CHECK_SKILL(ch, SKILL_CROSSFACE) / 4)
        + (dex_mod * (GET_DEX(ch) - GET_DEX(vict)))
        + (str_mod * (GET_STR(ch) - GET_STR(vict)));
    percent = number(1, 100);

    // You can't crossface pudding you fool!
    if (IS_PUDDING(vict) || IS_SLIME(vict))
        prob = 0;

    if (CHECK_SKILL(ch, SKILL_CROSSFACE) < 30)
        prob = 0;

    if (PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch, "%s[CROSSFACE] %s   roll:%d   chance:%d%s\r\n",
                     CCCYN(ch, C_NRM), GET_NAME(ch), percent, prob, CCNRM(ch, C_NRM));
    if (PRF2_FLAGGED(vict, PRF2_DEBUG))
        send_to_char(vict, "%s[CROSSFACE] %s   roll:%d   chance:%d%s\r\n",
                     CCCYN(vict, C_NRM), GET_NAME(ch), percent, prob,
                     CCNRM(vict, C_NRM));

    if (percent >= prob) {
        damage(ch, vict, weap, 0, SKILL_CROSSFACE, WEAR_HEAD);
    } else {

        dam = dice(GET_LEVEL(ch), strength_damage_bonus(GET_STR(ch))) +
            dice(9, GET_OBJ_WEIGHT(weap));

        wear_num = WEAR_FACE;
        if (!GET_EQ(vict, WEAR_FACE))
            wear_num = WEAR_HEAD;

        diff = prob - percent;

        // Wow!  vict really took one hell of a shot.  Stun that bastard!
        if (diff >= 70 && !GET_EQ(vict, wear_num)) {
            prev_pos = GET_POSITION(vict);
            if (damage(ch, vict, weap, dam, SKILL_CROSSFACE, wear_num)) {
                if (prev_pos != POS_STUNNED && !is_dead(vict) && !is_dead(ch)) {
                    if (is_fighting(ch)
                        && (!IS_NPC(vict) || !NPC2_FLAGGED(vict, NPC2_NOSTUN))) {
                        act("Your crossface has knocked $N senseless!",
                            true, ch, NULL, vict, TO_CHAR);
                        act("$n stuns $N with a vicious crossface!",
                            true, ch, NULL, vict, TO_NOTVICT);
                        act("Your jaw cracks as $n whips $s gun across your face.\r"
                            "Your vision fades...", true, ch, NULL, vict, TO_VICT);
                        remove_combat(ch, vict);
                        remove_all_combat(vict);
                        GET_POSITION(vict) = POS_STUNNED;
                    }
                }
            }
        }
        // vict just took a pretty good shot, knock him down...
        else if (diff >= 55) {
            dam = (int)(dam * 0.75);
            prev_pos = GET_POSITION(vict);
            if (damage(ch, vict, weap, dam, SKILL_CROSSFACE, wear_num)) {
                if ((prev_pos != POS_RESTING && prev_pos != POS_STUNNED)
                    && !is_dead(ch) && !is_dead(vict) && is_fighting(ch)) {
                    GET_POSITION(vict) = POS_RESTING;
                    act("Your crossface has knocked $N on $S back!",
                        true, ch, NULL, vict, TO_CHAR);
                    act("$n's nasty crossface just knocked $N on $S back!",
                        true, ch, NULL, vict, TO_NOTVICT);
                    act("Your jaw cracks as $n whips $s gun across your face.\n"
                        "You stagger and fall to the ground!",
                        true, ch, NULL, vict, TO_VICT);
                }
            }
        }
        // vict pretty much caught a grazing blow, knock off some eq
        else if (diff >= 20 && !is_arena_combat(ch, vict)) {
            struct obj_data *wear, *scraps;

            if (damage(ch, vict, weap, dam / 2, SKILL_CROSSFACE, wear_num)) {
                wear = GET_EQ(vict, wear_num);
                if (wear && !is_dead(ch) && !is_dead(vict) && is_fighting(ch)) {
                    act("Your crossface has knocked $N's $p from $S head!",
                        true, ch, wear, vict, TO_CHAR);
                    act("$n's nasty crossface just knocked $p from $N's head!",
                        true, ch, wear, vict, TO_NOTVICT);
                    act("Your jaw cracks as $n whips $s gun across your face.\n"
                        "$p flies off your head and lands a short distance away.", true, ch, wear, vict, TO_VICT);

                    scraps = damage_eq(vict, wear, dam / 16, TYPE_HIT);
                    if (scraps) {
                        // Object is destroyed
                        obj_from_char(scraps);
                        obj_to_room(scraps, vict->in_room);
                    } else if (GET_EQ(vict, wear_num)) {
                        // Object is still being worn (not broken)
                        obj_to_room(unequip_char(vict, wear_num, EQUIP_WORN),
                            vict->in_room);
                    } else {
                        // Object was broken and is in inventory
                        obj_from_char(wear);
                        obj_to_room(wear, vict->in_room);
                    }
                }
            }
        } else {
            damage(ch, vict, weap, dam / 2, SKILL_CROSSFACE, wear_num);
        }

        gain_skill_prof(ch, SKILL_CROSSFACE);
    }

    if (!is_dead(ch))
        WAIT_STATE(ch, 3 RL_SEC);
    if (!is_dead(vict))
        WAIT_STATE(vict, 2 RL_SEC);
}

#define NOBEHEAD_EQ(obj) \
IS_SET(obj->obj_flags.bitvector[1], AFF2_NECK_PROTECTED)

// Sniper skill for mercs...I've tried to comment all of
// my thought processes.  Don't be too hard on me, this
// has been my first modification of anything functional

// --Nothing
ACMD(do_snipe)
{
    int choose_random_limb(struct creature *victim);    // from combat/combat_utils.cc

    struct creature *vict;
    struct obj_data *gun, *bullet, *armor;
    struct room_data *cur_room, *nvz_room = NULL;
    struct affected_type af;
    int prob, percent, damage_loc, dam = 0;
    int snipe_dir = -1, distance = 0;
    char *vict_str, *dir_str, *kill_msg;

    init_affect(&af);

    vict_str = tmp_getword(&argument);
    dir_str = tmp_getword(&argument);

    // ch is blind?
    if (AFF_FLAGGED(ch, AFF_BLIND)) {
        send_to_char(ch, "You can't snipe anyone! you're blind!\r\n");
        return;
    }
    //ch in smoky room?
    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "The room is too smoky to see very far.\r\n");
        return;
    }
    // ch wielding a rifle?
    if ((gun = GET_EQ(ch, WEAR_WIELD))) {
        if (!IS_RIFLE(gun)) {
            if ((gun = GET_EQ(ch, WEAR_WIELD_2))) {
                if (!IS_RIFLE(gun)) {
                    send_to_char(ch, "But you aren't wielding a rifle!\r\n");
                    return;
                }
            } else {
                send_to_char(ch, "But you aren't wielding a rifle!\r\n");
                return;
            }
        }
    } else {
        send_to_char(ch, "You aren't wielding anything fool!\r\n");
        return;
    }
    // does ch have snipe leared at all?
    // I don't know if a skill can be less than 0
    // but I don't think it can hurt to check for it
    if (CHECK_SKILL(ch, SKILL_SNIPE) <= 0) {
        send_to_char(ch, "You have no idea how!\r\n");
        return;
    }
    // is ch's gun loaded?
    if (!GUN_LOADED(gun)) {
        send_to_char(ch, "But your gun isn't loaded!\r\n");
        return;
    }
    //in what direction is ch attempting to snipe?
    snipe_dir = search_block(dir_str, dirs, false);
    if (snipe_dir < 0) {
        send_to_char(ch, "Snipe in which direction?!\r\n");
        return;
    }

    if (!EXIT(ch, snipe_dir) || !EXIT(ch, snipe_dir)->to_room ||
        EXIT(ch, snipe_dir)->to_room == ch->in_room ||
        IS_SET(EXIT(ch, snipe_dir)->exit_info, EX_CLOSED)) {
        send_to_char(ch,
            "You aren't going to be sniping anyone in that direction...\r\n");
        return;
    }
    // is the victim in sight in that direction?
    // line of sight stops at a DT, smoke-filled room, or closed door
    distance = 0;
    vict = NULL;
    cur_room = ch->in_room;
    while (!vict && distance < 3) {
        if (!cur_room->dir_option[snipe_dir] ||
            !cur_room->dir_option[snipe_dir]->to_room ||
            cur_room->dir_option[snipe_dir]->to_room == ch->in_room ||
            IS_SET(cur_room->dir_option[snipe_dir]->exit_info, EX_CLOSED))
            break;

        cur_room = cur_room->dir_option[snipe_dir]->to_room;
        distance++;

        if (ROOM_FLAGGED(cur_room, ROOM_DEATH) ||
            ROOM_FLAGGED(cur_room, ROOM_SMOKE_FILLED))
            break;

        vict = get_char_in_remote_room_vis(ch, vict_str, cur_room);
        if (!nvz_room && vict && (ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)))
            nvz_room = cur_room;

    }

    if (!vict) {
        send_to_char(ch, "You can't see your target in that direction.\r\n");
        return;
    }
    // is vict an imm?
    if ((GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
        send_to_char(ch,
            "Are you crazy man!?!  You'll piss off superfly!!\r\n");
        return;
    }
    //is the player trying to snipe himself?
    if (vict == ch) {
        send_to_char(ch, "Yeah...real funny.\r\n");
        return;
    }
    // if vict is fighting someone you have a 50% chance of hitting the person
    // vict is fighting
    if (is_fighting(ch) && number(0, 1)) {
        vict = random_opponent(vict);
    }
    // Has vict been sniped once and is vict a sentinel mob?
    if ((NPC_FLAGGED(vict, NPC_SENTINEL)) &&
        affected_by_spell(ch, SKILL_SNIPE)) {
        act("$N has taken cover!\r\n", true, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (!vict)
        return;

    // is ch in a peaceful room?
    if (!ok_to_attack(ch, vict, true))
        return;

    //Ok, last check...is some asshole trying to damage a shop keeper
    if (!ok_damage_vendor(ch, vict) && GET_LEVEL(ch) < LVL_ELEMENT) {
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        send_to_char(ch, "You can't seem to get a clean line-of-sight.\r\n");
        return;
    }
    // Ok, the victim is in sight, ch is not blind or in a smoky room
    // ch is wielding a rifle, and neither the victim nor ch is in a
    // peaceful room, and all other misc. checks have been passed...
    // begin the hit prob calculations
    prob = CHECK_SKILL(ch, SKILL_SNIPE) + GET_REMORT_GEN(ch);
    // start percent at 40 to make it impossible for a merc to hit
    // someone if his skill is less than 40
    percent = number(40, 125);
    if (affected_by_spell(vict, ZEN_AWARENESS) ||
        AFF2_FLAGGED(vict, AFF2_TRUE_SEEING)) {
        percent += 25;
    }

    if (GET_POSITION(vict) < POS_FIGHTING) {
        percent += 15;
    }

    if (AFF2_FLAGGED(vict, AFF2_HASTE) && !AFF2_FLAGGED(ch, AFF2_HASTE)) {
        percent += 25;
    }
    // if the victim has already been sniped (wears off in 3 ticks)
    // then the victim is aware of a sniper and is assumed
    // to be taking the necessary precautions, and therefore is
    // much harder to hit
    if (affected_by_spell(vict, SKILL_SNIPE)) {
        percent += 50;
    }
    // just some level checks.  The victims level matters more
    // because it should be almost impossible to hit a high
    // level character who has been sniped once already
    prob += (GET_LEVEL(ch) / 4) + GET_REMORT_GEN(ch);
    percent += GET_LEVEL(vict) + (GET_REMORT_GEN(vict) / 4);
    damage_loc = choose_random_limb(vict);
    // we need to extract the bullet so we need an object pointer to
    // it.  However we musn't over look the possibility that gun->contains
    // could be a clip rather than a bullet
    bullet = gun->contains;
    if (IS_CLIP(bullet)) {
        bullet = bullet->contains;
    }

    if (nvz_room) {
        send_to_char(ch,
            "You watch in shock as your bullet stops in mid-air and drops to the ground.\r\n");
        act("$n takes careful aim, fires, and gets a shocked look on $s face.",
            false, ch, NULL, NULL, TO_ROOM);
        send_to_room(tmp_sprintf
            ("%s screams in from %s and harmlessly falls to the ground.",
                bullet->name, from_dirs[snipe_dir]), nvz_room);
        obj_from_obj(bullet);
        obj_to_room(bullet, nvz_room);
        return;
    } else {
        extract_obj(bullet);
    }

    // if percent is greater than prob it's a miss
    if (percent > prob) {
        // call damage with 0 dam to check for killers, TG's
        // newbie protection and other such stuff automagically ;)
        damage(ch, vict, gun, 0, SKILL_SNIPE, damage_loc);
        // FIXME: check to see that vict can be damaged

        // ch and vict really shouldn't be fighting if they aren't in
        // the same room...
        remove_combat(ch, vict);
        remove_combat(vict, ch);
        send_to_char(ch, "Damn!  You missed!\r\n");
        act(tmp_sprintf("$n fires $p to %s, then a look of irritation crosses $s face.",
                        to_dirs[snipe_dir]), true, ch, gun, vict, TO_ROOM);
        act(tmp_sprintf("A bullet screams past your head from %s!",
                from_dirs[snipe_dir]), true, ch, NULL, vict, TO_VICT);
        act(tmp_sprintf("A bullet screams past $n's head from %s!",
                from_dirs[snipe_dir]), true, vict, NULL, ch, TO_NOTVICT);
        WAIT_STATE(ch, 3 RL_SEC);
        return;
    } else {
        // it a hit!
        // grab the damage for the gun...this way this skill is
        // scalable
        dam = dice(gun_damage[GUN_TYPE(gun)][0],
            gun_damage[GUN_TYPE(gun)][1] + BUL_DAM_MOD(bullet));
        // as you can see, armor makes a huge difference, it's hard to imagine
        // a bullet doing much more than brusing someone through a T. carapace
        // seems to crash the mud if you call damage_eq() on a location
        // that doesn't have any eq...hmmm
        if (GET_EQ(vict, damage_loc)) {
            damage_eq(vict, GET_EQ(vict, damage_loc), dam / 2, TYPE_HIT);
        }
        if ((armor = GET_EQ(vict, damage_loc))
            && IS_OBJ_TYPE(armor, ITEM_ARMOR)) {
            if (IS_STONE_TYPE(armor) || IS_METAL_TYPE(armor))
                dam -= GET_OBJ_VAL(armor, 0) * 16;
            else
                dam -= GET_OBJ_VAL(armor, 0) * 4;
        }

        add_blood_to_room(vict->in_room, 1);
        apply_soil_to_char(ch, GET_EQ(vict, damage_loc), SOIL_BLOOD,
            damage_loc);
        if (!affected_by_spell(vict, SKILL_SNIPE)) {
            memset(&af, 0, sizeof(af));
            af.type = SKILL_SNIPE;
            af.level = GET_LEVEL(ch);
            af.duration = 3;
            af.owner = GET_IDNUM(ch);
            affect_to_char(vict, &af);
        }

        WAIT_STATE(vict, 2 RL_SEC);
        // double damage for a head shot...1 in 27 chance
        if (damage_loc == WEAR_HEAD) {
            dam = dam * 2;
        }
        // 1.5x damage for a neck shot...2 in 27 chance
        else if (damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
            dam += dam / 2;
        }
        if (damage_loc == WEAR_HEAD) {
            send_to_char(ch, "BOOM, HEADSHOT!\r\n");
        } else if (damage_loc == WEAR_NECK_1 || damage_loc == WEAR_NECK_2) {
            act("In the distance, frothy blood flows as your bullet connects with $N's neck.\r\n", false, ch, NULL, vict, TO_CHAR);
        }

        if (!IS_NPC(vict) && GET_LEVEL(vict) <= 6) {
            send_to_char(ch, "Hmm. Your gun seems to have jammed...\r\n");
            return;
        }

        act("You smirk with satisfaction as your bullet rips into $N.",
            false, ch, NULL, vict, TO_CHAR);
        act(tmp_sprintf("A bullet rips into your flesh from %s!",
                        from_dirs[snipe_dir]), true, ch, NULL, vict, TO_VICT);
        act("A bullet rips into $n's flesh!", true, vict, NULL, ch, TO_ROOM);
        act(tmp_sprintf("$n takes careful aim and fires $p %s!",
                        to_dirs[snipe_dir]), true, ch, gun, vict, TO_ROOM);
        mudlog(LVL_AMBASSADOR, NRM, true,
            "INFO: %s has sniped %s from room %d to room %d",
            GET_NAME(ch), GET_NAME(vict),
            ch->in_room->number, vict->in_room->number);

        kill_msg = tmp_sprintf("You have killed %s!", GET_NAME(vict));

        damage(ch, vict, gun, dam, SKILL_SNIPE, damage_loc);
        // FIXME: if attack failed, return

        if (is_dead(vict)) {
            act(kill_msg, true, ch, NULL, NULL, TO_CHAR);
            act("$n gets a look of predatory satisfaction.",
                true, ch, NULL, NULL, TO_ROOM);
        }
        gain_skill_prof(ch, SKILL_SNIPE);
        WAIT_STATE(ch, 5 RL_SEC);
    }
}

ACMD(do_wrench)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    struct obj_data *neck = NULL;
    int two_handed = 0;
    int prob, percent, dam;
    char *arg;

    arg = tmp_getword(&argument);

    if (!(vict = get_char_room_vis(ch, arg))) {
        if (is_fighting(ch)) {
            vict = (random_opponent(ch));
        } else if ((ovict =
                get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You fiercely wrench $p!", false, ch, ovict, NULL, TO_CHAR);
            return;
        } else {
            send_to_char(ch, "Wrench who?\r\n");
            return;
        }
    }

    if (GET_EQ(ch, WEAR_WIELD) && IS_TWO_HAND(GET_EQ(ch, WEAR_WIELD))) {
        send_to_char(ch,
            "You are using both hands to wield your weapon right now!\r\n");
        return;
    }

    if (GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_WIELD_2) ||
            GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_SHIELD))) {
        send_to_char(ch, "You need a hand free to do that!\r\n");
        return;
    }

    if (!ok_to_attack(ch, vict, true))
        return;
    //
    // give a bonus if both hands are free
    //

    if (!GET_EQ(ch, WEAR_WIELD) &&
        !(GET_EQ(ch, WEAR_WIELD_2) || GET_EQ(ch, WEAR_HOLD)
            || GET_EQ(ch, WEAR_SHIELD))) {
        two_handed = 1;
    }

    percent = ((10 - (GET_AC(vict) / 50)) * 2) + number(1, 101);
    prob = CHECK_SKILL(ch, SKILL_WRENCH);

    if (!can_see_creature(ch, vict)) {
        prob += 10;
    }

    if (IS_PUDDING(vict) || IS_SLIME(vict)) {
        prob = 0;
    }

    dam = dice(GET_LEVEL(ch), GET_STR(ch));

    if (two_handed) {
        dam += dam / 2;
    }

    if (!(is_fighting(ch)) && !is_fighting(vict)) {
        dam += dam / 3;
    }

    if (((neck = GET_IMPLANT(vict, WEAR_NECK_1)) && NOBEHEAD_EQ(neck)) ||
        ((neck = GET_IMPLANT(vict, WEAR_NECK_2)) && NOBEHEAD_EQ(neck))) {
        dam /= 2;
        damage_eq(ch, neck, dam, TYPE_HIT);
    }

    if (((neck = GET_EQ(vict, WEAR_NECK_1)) && NOBEHEAD_EQ(neck)) ||
        ((neck = GET_EQ(vict, WEAR_NECK_2)) && NOBEHEAD_EQ(neck))) {
        act("$n grabs you around the neck, but you are covered by $p!", false,
            ch, neck, vict, TO_VICT);
        act("$n grabs $N's neck, but $N is covered by $p!", false, ch, neck,
            vict, TO_NOTVICT);
        act("You grab $N's neck, but $e is covered by $p!", false, ch, neck,
            vict, TO_CHAR);
        check_attack(ch, vict);
        damage_eq(ch, neck, dam, TYPE_HIT);
        WAIT_STATE(ch, 1 RL_SEC);
        return;
    }

    if (prob > percent && (CHECK_SKILL(ch, SKILL_WRENCH) >= 30)) {
        WAIT_STATE(ch, SEG_VIOLENCE * 4);
        WAIT_STATE(vict, SEG_VIOLENCE * 2);
        damage(ch, vict, NULL, dam, SKILL_WRENCH, WEAR_NECK_1);
        gain_skill_prof(ch, SKILL_WRENCH);
        return;
    } else {
        WAIT_STATE(ch, SEG_VIOLENCE * 2);
        damage(ch, vict, NULL, 0, SKILL_WRENCH, WEAR_NECK_1);
    }
}

ACMD(do_infiltrate)
{
    struct affected_type af;

    init_affect(&af);

    if (AFF3_FLAGGED(ch, AFF3_INFILTRATE)) {
        send_to_char(ch,
            "Okay, you are no longer attempting to infiltrate.\r\n");
        affect_from_char(ch, SKILL_INFILTRATE);
        affect_from_char(ch, SKILL_SNEAK);
        return;
    }

    if (CHECK_SKILL(ch, SKILL_INFILTRATE) < number(20, 70)) {
        send_to_char(ch, "You don't feel particularly sneaky...\n");
        return;
    }

    send_to_char(ch,
        "Okay, you'll try to infiltrate until further notice.\r\n");

    af.type = SKILL_SNEAK;
    af.duration = GET_LEVEL(ch);
    af.bitvector = AFF_SNEAK;
    af.aff_index = 0;
    af.level = GET_LEVEL(ch) + skill_bonus(ch, SKILL_INFILTRATE);
    af.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af);

    af.type = SKILL_INFILTRATE;
    af.aff_index = 3;
    af.duration = GET_LEVEL(ch);
    af.bitvector = AFF3_INFILTRATE;
    af.level = GET_LEVEL(ch) + skill_bonus(ch, SKILL_INFILTRATE);
    af.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af);
}

void
perform_appraise(struct creature *ch, struct obj_data *obj, int skill_lvl)
{
    int i;
    long cost;
    unsigned long eq_req_flags;

    struct time_info_data age(struct creature *ch);

    acc_string_clear();

    acc_sprintf("%s is %s.\r\n", tmp_capitalize(obj->name),
        strlist_aref(GET_OBJ_TYPE(obj), item_type_descs));

    if (skill_lvl > 30) {
        eq_req_flags = ITEM_ANTI_GOOD | ITEM_ANTI_EVIL | ITEM_ANTI_NEUTRAL |
            ITEM_ANTI_MAGIC_USER | ITEM_ANTI_CLERIC | ITEM_ANTI_THIEF |
            ITEM_ANTI_WARRIOR | ITEM_NOSELL | ITEM_ANTI_BARB |
            ITEM_ANTI_PSYCHIC | ITEM_ANTI_PHYSIC | ITEM_ANTI_CYBORG |
            ITEM_ANTI_KNIGHT | ITEM_ANTI_RANGER | ITEM_ANTI_BARD |
            ITEM_ANTI_MONK | ITEM_BLURRED | ITEM_DAMNED;
        if (GET_OBJ_EXTRA(obj) & eq_req_flags)
            acc_sprintf("Item is %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA(obj) & eq_req_flags, extra_bits));

        eq_req_flags = ITEM2_ANTI_MERC;
        if (GET_OBJ_EXTRA2(obj) & eq_req_flags)
            acc_sprintf("Item is %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA2(obj) & eq_req_flags,
                    extra2_bits));

        eq_req_flags = ITEM3_REQ_MAGE | ITEM3_REQ_CLERIC | ITEM3_REQ_THIEF |
            ITEM3_REQ_WARRIOR | ITEM3_REQ_BARB | ITEM3_REQ_PSIONIC |
            ITEM3_REQ_PHYSIC | ITEM3_REQ_CYBORG | ITEM3_REQ_KNIGHT |
            ITEM3_REQ_RANGER | ITEM3_REQ_BARD | ITEM3_REQ_MONK |
            ITEM3_REQ_VAMPIRE | ITEM3_REQ_MERCENARY;
        if (GET_OBJ_EXTRA3(obj) & eq_req_flags)
            acc_sprintf("Item is %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA3(obj) & eq_req_flags,
                    extra3_bits));
    }

    acc_sprintf("Item weighs around %s and is made of %s.\n",
                format_weight(GET_OBJ_WEIGHT(obj), USE_METRIC(ch)),
                material_names[GET_OBJ_MATERIAL(obj)]);

    if (skill_lvl > 100)
        cost = 0;
    else
        cost = (100 - skill_lvl) * GET_OBJ_COST(obj) / 100;
    cost = GET_OBJ_COST(obj) + number(0, cost) - cost / 2;

    if (cost > 0)
        acc_sprintf("Item looks to be worth about %'ld.\r\n", cost);
    else
        acc_sprintf("Item doesn't look to be worth anything.\r\n");

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
        if (skill_lvl > 80) {
            acc_sprintf("This %s casts: ", item_types[(int)GET_OBJ_TYPE(obj)]);

            if (GET_OBJ_VAL(obj, 1) >= 1)
                acc_sprintf("%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 1)));
            if (GET_OBJ_VAL(obj, 2) >= 1)
                acc_sprintf("%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 2)));
            if (GET_OBJ_VAL(obj, 3) >= 1)
                acc_sprintf("%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
        }
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        if (skill_lvl > 80) {
            acc_sprintf("This %s casts: ", item_types[(int)GET_OBJ_TYPE(obj)]);
            acc_sprintf("%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
            if (skill_lvl > 90)
                acc_sprintf("It has %d maximum charge%s and %d remaining.\r\n",
                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
                    GET_OBJ_VAL(obj, 2));
        }
        break;
    case ITEM_WEAPON:
        acc_sprintf("This weapon can deal up to %d points of damage.\r\n",
            GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 1));

        if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
            acc_sprintf("This weapon casts an offensive spell.\r\n");
        break;
    case ITEM_ARMOR:
        if (GET_OBJ_VAL(obj, 0) < 2)
            acc_sprintf("This armor provides hardly any protection.\r\n");
        else if (GET_OBJ_VAL(obj, 0) < 5)
            acc_sprintf("This armor provides a little protection.\r\n");
        else if (GET_OBJ_VAL(obj, 0) < 15)
            acc_sprintf("This armor provides some protection.\r\n");
        else if (GET_OBJ_VAL(obj, 0) < 20)
            acc_sprintf("This armor provides a lot of protection.\r\n");
        else if (GET_OBJ_VAL(obj, 0) < 25)
            acc_sprintf
                ("This armor provides a ridiculous amount of protection.\r\n");
        else
            acc_sprintf
                ("This armor provides an insane amount of protection.\r\n");
        break;
    case ITEM_CONTAINER:
        acc_sprintf("This container holds a maximum of %s.\r\n",
                    format_weight(GET_OBJ_VAL(obj, 0), USE_METRIC(ch)));
        break;
    case ITEM_FOUNTAIN:
    case ITEM_DRINKCON:
        acc_sprintf("This container holds some %s\r\n",
            drinks[GET_OBJ_VAL(obj, 2)]);
    }

    for (i = 0; i < MIN(MAX_OBJ_AFFECT, skill_lvl / 25); i++) {
        if (obj->affected[i].location != APPLY_NONE) {
            if (obj->affected[i].modifier > 0)
                acc_sprintf("Item increases %s\r\n",
                    strlist_aref(obj->affected[i].location, apply_types));
            else if (obj->affected[i].modifier < 0)
                acc_sprintf("Item decreases %s\r\n",
                    strlist_aref(obj->affected[i].location, apply_types));
        }
    }
    page_string(ch->desc, acc_get_string());
}

ACMD(do_appraise)
{
    struct obj_data *obj = NULL;    // the object that will be emptied
    int bits;
    char *arg;

    arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "You have to appraise something.\r\n");
        return;
    }

    if (CHECK_SKILL(ch, SKILL_APPRAISE) < 10) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }

    if (!(bits =
            generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, NULL, &obj))) {
        send_to_char(ch, "You can't find any %s to appraise.\r\n", arg);
        return;
    }

    perform_appraise(ch, obj, CHECK_SKILL(ch, SKILL_APPRAISE));
}

ACMD(do_combine)
{
    struct obj_data *potion1, *potion2;
    char *arg;
    int bits;

    arg = tmp_getword(&argument);
    if (!*arg) {
        send_to_char(ch, "Usage: combine <potion1> <potion2>\r\n");
        return;
    }
    // Find the first potion
    bits = generic_find(arg, FIND_OBJ_INV, ch, NULL, &potion1);
    if (!bits) {
        send_to_char(ch, "You don't see any %s here.\r\n", arg);
        return;
    }
    if (!IS_POTION(potion1)) {
        act("$p is not a potion.", true, ch, NULL, potion1, TO_CHAR);
        return;
    }
    // Find the second potion
    arg = tmp_getword(&argument);
    bits = generic_find(arg, FIND_OBJ_INV, ch, NULL, &potion2);
    if (!bits) {
        send_to_char(ch, "You don't see any %s here.\r\n", arg);
        return;
    }
    if (!IS_POTION(potion2)) {
        act("$p is not a potion.", true, ch, NULL, potion2, TO_CHAR);
        return;
    }

    if (potion1 == potion2) {
        send_to_char(ch, "You can't combine a potion with itself.\r\n");
        return;
    }

    if (CHECK_SKILL(ch, SKILL_CHEMISTRY) <= 0) {
        send_to_char(ch, "You aren't familiar with how to combine them.\r\n");
    }

    int spell_count = 0;
    int level_count = 0;
    int idx;
    for (idx = 1; idx < 4; idx++) {
        if (GET_OBJ_VAL(potion1, idx) > 0)
            spell_count++;
        if (GET_OBJ_VAL(potion2, idx) > 0)
            spell_count++;
    }
    level_count = GET_OBJ_VAL(potion1, 0) + GET_OBJ_VAL(potion2, 0);
    // Create the combined potion
    struct obj_data *new_potion = read_object(MIXED_POTION_VNUM);

    new_potion->creation_method = CREATED_PLAYER;
    new_potion->creator = GET_IDNUM(ch);

    extract_obj(potion1);
    extract_obj(potion2);
    obj_to_char(new_potion, ch);

    // Check to see if mixture explodes :D
    if (spell_count > 3 || level_count > 49) {
        switch (number(0, 5)) {
        case 0:
            BOMB_TYPE(new_potion) = BOMB_CONCUSSION;
            break;
        case 1:
            BOMB_TYPE(new_potion) = BOMB_INCENDIARY;
            break;
        case 2:
            BOMB_TYPE(new_potion) = BOMB_FRAGMENTATION;
            break;
        case 3:
            BOMB_TYPE(new_potion) = BOMB_FLASH;
            break;
        case 4:
            BOMB_TYPE(new_potion) = BOMB_SMOKE;
            break;
        default:
            BOMB_TYPE(new_potion) = BOMB_DISRUPTION;
            break;
        }
        BOMB_POWER(new_potion) = number(20, 40);
        if (IS_PC(ch)) {
            BOMB_IDNUM(new_potion) = GET_IDNUM(ch);
        } else {
            BOMB_IDNUM(new_potion) = -NPC_IDNUM(ch);
        }

        detonate_bomb(new_potion);
        return;
    }

    if (number(0, 120) < CHECK_SKILL(ch, SKILL_CHEMISTRY)) {
        // Didn't explode - set up the level and spells
        GET_OBJ_VAL(new_potion, 0) =
            level_count * 100 / (2 * skill_bonus(ch, SKILL_CHEMISTRY));
        spell_count = 1;
        int idx;
        for (idx = 1; idx < 4; idx++) {
            if (GET_OBJ_VAL(potion1, idx) > 0)
                GET_OBJ_VAL(new_potion, spell_count++) =
                    GET_OBJ_VAL(potion1, idx);
            if (GET_OBJ_VAL(potion2, idx) > 0)
                GET_OBJ_VAL(new_potion, spell_count++) =
                    GET_OBJ_VAL(potion2, idx);
        }
    }
    // They don't know if they succeeded unless they identify it or use it
    act("You mix them together and create $p!",
        false, ch, new_potion, NULL, TO_CHAR);
    act("$n mixes two potions together and creates $p!",
        false, ch, new_potion, NULL, TO_ROOM);
}

struct obj_data *
find_hamstring_weapon(struct creature *ch)
{
    struct obj_data *weap = NULL;
    if ((weap = GET_EQ(ch, WEAR_WIELD)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_WIELD_2)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_HANDS)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_EQ(ch, WEAR_ARMS)) && is_slashing_weapon(weap)) {
        return weap;
    } else if ((weap = GET_IMPLANT(ch, WEAR_HANDS))
               && is_slashing_weapon(weap)
               && GET_EQ(ch, WEAR_HANDS) == NULL) {
        return weap;
    } else if ((weap = GET_IMPLANT(ch, WEAR_ARMS))
               && is_slashing_weapon(weap)
               && GET_EQ(ch, WEAR_ARMS) == NULL) {
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
        dam = dice(level, 15 + gen / 2);
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
            if (damage(ch, vict, weap, dam, SKILL_HAMSTRING, WEAR_LEGS))
                GET_POSITION(vict) = POS_RESTING;
        } else {
            WAIT_STATE(vict, 2 RL_SEC);
            if (damage(ch, vict, weap, dam / 2, SKILL_HAMSTRING, WEAR_LEGS))
                GET_POSITION(vict) = POS_SITTING;
        }
        if (!is_dead(ch)) {
            gain_skill_prof(ch, SKILL_HAMSTRING);
            WAIT_STATE(ch, 5 RL_SEC);
        }
    }
}
