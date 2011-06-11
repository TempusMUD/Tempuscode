/*************************************************************************
*   File: combat_messages.c                             Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: combat_messages.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#define __combat_code__
#define __combat_messages__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "fight.h"
#include "bomb.h"
#include "guns.h"
#include "specs.h"

extern int corpse_state;
char *replace_string(const char *str,
    const char *weapon_singular,
    const char *weapon_plural, const char *location, const char *substance);

void
appear(struct creature *ch, struct creature *vict)
{
    char *to_char = NULL;
    int found = 0;

    // Sonic imagery and retina detects transparent creatures
    if (affected_by_spell(ch, SPELL_TRANSMITTANCE) &&
        !affected_by_spell(ch, SPELL_DIMENSIONAL_SHIFT) &&
        !(AFF3_FLAGGED(vict, AFF3_SONIC_IMAGERY) ||
            AFF_FLAGGED(vict, AFF_RETINA) ||
            affected_by_spell(vict, ZEN_AWARENESS))) {
        affect_from_char(ch, SPELL_TRANSMITTANCE);
        send_to_char(ch, "Your transparency has expired.\r\n");
        found = 1;
    }
    // True seeing and detect invisibility counteract all magical invis
    if (affected_by_spell(ch, SPELL_INVISIBLE) &&
        (AFF2_FLAGGED(vict, AFF2_TRUE_SEEING) ||
            AFF_FLAGGED(vict, AFF_DETECT_INVIS))) {
        affect_from_char(ch, SPELL_INVISIBLE);
        send_to_char(ch, "Your invisibility spell has expired.\r\n");
        found = 1;
    }

    if (IS_ANIMAL(vict) && affected_by_spell(ch, SPELL_ANIMAL_KIN)) {
        affect_from_char(ch, SPELL_ANIMAL_KIN);
        send_to_char(ch, "You no longer feel kinship with animals.\r\n");
        found = 1;
    }
    if (IS_UNDEAD(vict) && affected_by_spell(ch, SPELL_INVIS_TO_UNDEAD)) {
        affect_from_char(ch, SPELL_INVIS_TO_UNDEAD);
        send_to_char(ch, "Your invisibility to undead has expired.\r\n");
        found = 1;
    }

    if (AFF_FLAGGED(ch, AFF_HIDE)) {
        REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
        found = 1;
    }

    if (!IS_NPC(vict) && !IS_NPC(ch) &&
        GET_REMORT_GEN(ch) > GET_REMORT_GEN(vict) &&
        GET_INVIS_LVL(ch) > GET_LEVEL(vict)) {
        GET_INVIS_LVL(ch) = GET_LEVEL(vict);
        send_to_char(ch, "You feel a bit more visible.\n");
        found = 1;
    }

    if (found) {
        if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
            act("$n suddenly appears, seemingly from nowhere.",
                true, ch, 0, 0, TO_ROOM);
            if (to_char)
                send_to_char(ch, "%s", to_char);
            else
                send_to_char(ch, "You fade into visibility.\r\n");
        } else
            act("You feel a strange presence as $n appears, seemingly from nowhere.", false, ch, 0, 0, TO_ROOM);
    }
}

void
load_messages(void)
{
    FILE *fl;
    int i, type;
    struct message_type *messages;
    char chk[128];

    if (!(fl = fopen(MESS_FILE, "r"))) {
        perror(tmp_sprintf("Error opening combat message file %s", MESS_FILE));
        safe_exit(1);
    }
    for (i = 0; i < MAX_MESSAGES; i++) {
        fight_messages[i].a_type = 0;
        fight_messages[i].number_of_attacks = 0;
        fight_messages[i].msg = 0;
    }

    if (!fgets(chk, 128, fl)) {
        perror(tmp_sprintf
            ("Error reading first line from combat message file %s",
                MESS_FILE));
        safe_exit(1);
    }
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        if (!fgets(chk, 128, fl)) {
            perror(tmp_sprintf
                ("Error reading header of combat message file %s", MESS_FILE));
            safe_exit(1);
        }

    while (*chk == 'M') {
        if (!fgets(chk, 128, fl)) {
            perror(tmp_sprintf("Error reading combat message file %s",
                    MESS_FILE));
            safe_exit(1);
        }
        sscanf(chk, " %d\n", &type);
        i = 0;
        while (i < MAX_MESSAGES
            && fight_messages[i].a_type && fight_messages[i].a_type != type)
            i++;
        if (i >= MAX_MESSAGES) {
            fprintf(stderr,
                "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
            safe_exit(1);
        }
        CREATE(messages, struct message_type, 1);
        fight_messages[i].number_of_attacks++;
        fight_messages[i].a_type = type;
        messages->next = fight_messages[i].msg;
        fight_messages[i].msg = messages;

        messages->die_msg.attacker_msg = fread_action(fl, i);
        messages->die_msg.victim_msg = fread_action(fl, i);
        messages->die_msg.room_msg = fread_action(fl, i);
        messages->miss_msg.attacker_msg = fread_action(fl, i);
        messages->miss_msg.victim_msg = fread_action(fl, i);
        messages->miss_msg.room_msg = fread_action(fl, i);
        messages->hit_msg.attacker_msg = fread_action(fl, i);
        messages->hit_msg.victim_msg = fread_action(fl, i);
        messages->hit_msg.room_msg = fread_action(fl, i);
        messages->god_msg.attacker_msg = fread_action(fl, i);
        messages->god_msg.victim_msg = fread_action(fl, i);
        messages->god_msg.room_msg = fread_action(fl, i);
        if (!fgets(chk, 128, fl)) {
            perror(tmp_sprintf
                ("Error reading separator from combat message file %s",
                    MESS_FILE));
            safe_exit(1);
        }
        while (!feof(fl) && (*chk == '\n' || *chk == '*'))
            if (!fgets(chk, 128, fl)) {
                perror(tmp_sprintf
                    ("Error reading next message from combat message file %s",
                        MESS_FILE));
                safe_exit(1);
            }
    }

    slog("Top message number loaded: %d.", type);
    fclose(fl);
}

void
death_cry(struct creature *ch)
{
    struct room_data *adjoin_room = NULL;
    int door;
    struct room_data *was_in = NULL;
    int found = 0;

    if (GET_NPC_SPEC(ch) == fate)
        act("$n dissipates in a cloud of mystery, leaving you to your fate.",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_GHOUL(ch) || IS_WIGHT(ch) || IS_MUMMY(ch))
        act("$n falls lifeless to the floor with a shriek.",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_SKELETON(ch))
        act("$n clatters noisily into a lifeless heap.",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_GHOST(ch) || IS_SHADOW(ch) || IS_WRAITH(ch) || IS_SPECTRE(ch))
        act("$n vanishes into the void with a terrible shriek.",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_VAMPIRE(ch))
        act("$n screams as $e is consumed in a blazing fire!",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_LICH(ch))
        act("A roar fills your ears as $n is ripped into the void!",
            false, ch, 0, 0, TO_ROOM);
    else if (IS_UNDEAD(ch))
        act("Your skin crawls as you hear $n's final shriek.",
            false, ch, 0, 0, TO_ROOM);
    else {
        for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
            struct creature *tch = it->data;

            if (tch == ch)
                return;
            found = 0;

            if (!found && IS_BARB(tch) && !number(0, 1)) {
                found = 1;
                act("You feel a rising bloodlust as you hear $n's death cry.",
                    false, ch, 0, tch, TO_VICT);
            }

            if (!found)
                act("Your blood freezes as you hear $n's death cry.",
                    false, ch, 0, tch, TO_VICT);
        }
    }

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (ch != tch && GET_POSITION(tch) == POS_SLEEPING &&
            !PLR_FLAGGED(tch, PLR_WRITING) &&
            !AFF_FLAGGED(tch, AFF_SLEEP)) {
            GET_POSITION(tch) = POS_RESTING;
        }
    }

    was_in = ch->in_room;

    for (door = 0; door < NUM_OF_DIRS && !found; door++) {
        if (CAN_GO(ch, door) && ch->in_room != EXIT(ch, door)->to_room) {
            ch->in_room = was_in->dir_option[door]->to_room;
            act(tmp_sprintf
                ("Your blood freezes as you hear someone's death cry from %s.",
                    from_dirs[door]), false, ch, 0, 0, TO_ROOM);
            adjoin_room = ch->in_room;
            ch->in_room = was_in;
            if (adjoin_room->dir_option[rev_dir[door]] &&
                adjoin_room->dir_option[rev_dir[door]]->to_room == was_in) {

                GList *tmplist = g_list_copy(adjoin_room->people);
                for (GList *it = first_living(tmplist);it;it = next_living(it)) {
                    struct creature *tch = it->data;

                    if (found)
                        break;
                    if (IS_NPC(tch)
                        && !NPC_FLAGGED(tch, NPC_SENTINEL)
                        && !is_fighting(tch)
                        && AWAKE(tch)
                        && (NPC_FLAGGED(tch, NPC_HELPER) ||
                            tch->mob_specials.shared->func == cityguard)
                        && number(0, 40) < GET_LEVEL(tch)) {
                        if ((!ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) ||
                                CHAR_WITHSTANDS_FIRE(tch)) &&
                            (!ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD) ||
                                CHAR_WITHSTANDS_COLD(tch)) &&
                            (can_see_room(tch, ch->in_room))) {

                            int move_result =
                                do_simple_move(tch, rev_dir[door], MOVE_RUSH,
                                1);

                            if (move_result == 0) {
                                break;
                            } else if (move_result == 2) {
                                break;
                            }
                        }
                    }
                }
                g_list_free(tmplist);
            }
        }
    }
}

int pick_soilage_target(struct creature *tch, gpointer victim)
{
    return (tch != victim
            && !is_dead(tch)
            && !IS_NPC(tch)
            && !number(0, 2)
            && number(5, 30) > GET_DEX(tch))
        ? 0 : -1;
}


void
blood_spray(struct creature *ch, struct creature *victim,
            struct obj_data *weapon,
            int dam __attribute__ ((unused)), int attacktype)
{
    const char *to_char, *to_vict, *to_notvict;
    int pos, found = 0;

    // some creatures don't have blood
    if (!CHAR_HAS_BLOOD(victim))
        return;

    switch (number(0, 6)) {
    case 0:
        to_char = "Your terrible %s sends a spray of $N's blood into the air!";
        to_notvict =
            "$n's terrible %s sends a spray of $N's blood into the air!";
        to_vict = "$n's terrible %s sends a spray of your blood into the air!";
        break;
    case 1:
        to_char = "A stream of blood erupts from $N as a result of your %s!";
        to_notvict =
            "A stream of blood erupts from $N as a result of $n's %s!";
        to_vict = "A stream of your blood erupts as a result of $n's %s!";
        break;
    case 2:
        to_char =
            "Your %s opens a wound in $N which erupts in a torrent of blood!";
        to_notvict =
            "$n's %s opens a wound in $N which erupts in a torrent of blood!";
        to_vict =
            "$n's %s opens a wound in you which erupts in a torrent of blood!";
        break;
    case 3:
        to_char =
            "Your mighty %s sends a spray of $N's blood raining down all over!";
        to_notvict =
            "$n's mighty %s sends a spray of $N's blood raining down all over!";
        to_vict =
            "$n's mighty %s sends a spray of your blood raining down all over!";
        break;
    case 4:
        to_char =
            "Your devastating %s sends $N's blood spraying into the air!";
        to_notvict =
            "$n's devastating %s sends $N's blood spraying into the air!";
        to_vict =
            "$n's devastating %s sends your blood spraying into the air!";
        break;
    case 5:
        to_char =
            "$N's blood sprays all over as a result of your terrible %s!";
        to_notvict =
            "$N's blood sprays all over as a result of $n's terrible %s!";
        to_vict =
            "Your blood sprays all over as a result of $n's terrible %s!";
        break;
    default:
        to_char = "A gout of $N's blood spews forth in response to your %s!";
        to_notvict =
            "A gout of $N's blood spews forth in response to $n's %s!";
        to_vict = "A gout of your blood spews forth in response to $n's %s!";
        break;
    }

    send_to_char(ch, "%s", CCRED(ch, C_NRM));
    act(tmp_sprintf(to_char,
            attacktype >= TYPE_HIT ?
            attack_hit_text[attacktype -
                TYPE_HIT].singular : spell_to_str(attacktype)), false, ch, 0,
        victim, TO_CHAR);
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
    send_to_char(victim, "%s", CCRED(victim, C_NRM));
    act(tmp_sprintf(to_vict,
            attacktype >= TYPE_HIT ?
            attack_hit_text[attacktype -
                TYPE_HIT].singular : spell_to_str(attacktype)), false, ch, 0,
        victim, TO_VICT);
    send_to_char(victim, "%s", CCNRM(victim, C_NRM));

    act(tmp_sprintf(to_notvict,
            attacktype >= TYPE_HIT ?
            attack_hit_text[attacktype -
                TYPE_HIT].singular : spell_to_str(attacktype)), false, ch, 0,
        victim, TO_NOTVICT);

    //
    // evil clerics get a bonus for sending blood flying!
    //

    if (IS_CLERIC(ch) && IS_EVIL(ch)) {
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 20);
        GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20);
        GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20);
    }

    GList *it = g_list_find_custom(ch->in_room->people,
                                   0,
                                   (GCompareFunc) pick_soilage_target);

    if (it) {
        struct creature *tch = it->data;

        pos = apply_soil_to_char(tch, NULL, SOIL_BLOOD, WEAR_RANDOM);
        if (pos) {
            char *msg = NULL;

            if (GET_EQ(tch, pos))
                msg = tmp_sprintf("$N's blood splatters all over %s!",
                    GET_EQ(tch, pos)->name);
            else
                msg = tmp_sprintf("$N's blood splatters all over your %s!",
                    wear_description[pos]);

            act(msg, false, tch, 0, victim, TO_CHAR);
            found = 1;

            if (tch == ch && IS_CLERIC(ch) && IS_EVIL(ch)) {
                GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 20);
                GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20);
                GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20);
            }
        }
    }


    if (!found)
        add_blood_to_room(ch->in_room, 1);

    if (weapon && weapon->worn_by &&
        weapon == GET_EQ(ch, weapon->worn_on) &&
        !OBJ_SOILED(weapon, SOIL_BLOOD)) {
        apply_soil_to_char(ch, NULL, SOIL_BLOOD, weapon->worn_on);
    }
}

/* message for doing damage with a weapon */
void
dam_message(int dam, struct creature *ch, struct creature *victim,
            struct obj_data *weapon,
            int w_type, int location)
{
    char *buf;
    int msgnum;

    static struct dam_weapon_type {
        const char *to_room;
        const char *to_char;
        const char *to_victim;
    } dam_weapons[] = {

        /* use #w for singular ( i.e. "slash" ) and #W for plural ( i.e. "slashes" ) */

        {                       // 0: 0
        "$n tries to #w $N, but misses.", "You try to #w $N, but miss.", "$n tries to #w you, but misses."}, {  // 1: 1 - 4
        "$n tickles $N as $e #W $M.", "You tickle $N as you #w $M.", "$n tickles you as $e #W you."}, { // 2: 3 - 6
        "$n barely #W $N.", "You barely #w $N.", "$n barely #W you."}, {    // 3: 5 - 10
        "$n #W $N.", "You #w $N.", "$n #W you."}, { // 4: 7 - 14
        "$n #W $N hard.", "You #w $N hard.", "$n #W you hard."}, {  // 5: 11 - 19
        "$n #W $N very hard.", "You #w $N very hard.", "$n #W you very hard."}, {   // 6: 15 - 23
        "$n #W $N extremely hard.", "You #w $N extremely hard.", "$n #W you extremely hard."}, {    // 7: 19 - 27
        "$n massacres $N to small fragments with $s #w.", "You massacre $N to small fragments with your #w.", "$n massacres you to small fragments with $s #w."}, { // 8: 23 - 32
        "$n devastates $N with $s incredible #w!!", "You devastate $N with your incredible #w!!", "$n devastates you with $s incredible #w!!"}, {   // 9: 32 - 37
        "$n OBLITERATES $N with $s deadly #w!!", "You OBLITERATE $N with your deadly #w!!", "$n OBLITERATES you with $s deadly #w!!"}, {    // 10: 37 - 45
        "$n utterly DEMOLISHES $N with $s unbelievable #w!!", "You utterly DEMOLISH $N with your unbelievable #w!!", "$n utterly DEMOLISHES you with $s unbelievable #w!!"}, {  // 11: 46 - 69
        "$n PULVERIZES $N with $s vicious #w!!", "You PULVERIZE $N with your vicious #w!!", "$n PULVERIZES you with $s vicious #w!!"}, {    // 12: 70 - 99
        "$n *DECIMATES* $N with $s horrible #w!!", "You *DECIMATE* $N with your horrible #w!!", "$n *DECIMATES* you with $s horrible #w!!"}, {  // 13: 100 - 139
        "$n *LIQUIFIES* $N with $s incredibly vicious #w!!", "You **LIQUIFY** $N with your incredibly vicious #w!!", "$n *LIQUIFIES* you with $s incredibly vicious #w!!"}, {   // 14: 140 - 189
        "$n **VAPORIZES** $N with $s terrible #w!!", "You **VAPORIZE** $N with your terrible #w!!", "$n **VAPORIZES** you with $s terrible #w!!"}, {    // 15: >189
        "$n **ANNIHILATES** $N with $s ultra powerful #w!!",
                "You **ANNIHILATE** $N with your ultra powerful #w!!",
                "$n **ANNIHILATES** you with $s ultra powerful #w!!"}
    };

    static struct dam_weapon_type dam_mana_shield[] = {
        {                       // 0: 0
                "$n tries to #w $N, but misses.",
                "You try to #w $N, but miss.",
            "$n tries to #w you, but misses."},
        {                       // 1: 1 - 4
                "$n tickles $N's mana shield as $e #W $M.",
                "You tickle $N's mana shield as you #w $M.",
            "$n tickles your mana shield as $e #W you."},
        {                       // 2: 3 - 6
                "$n barely #W $N's mana shield.",
                "You barely #w $N's mana shield.",
            "$n barely #W your mana shield."},
        {                       // 3: 5 - 10
                "$n #W $N's mana shield.",
                "You #w $N's mana shield.",
            "$n #W your mana shield."},
        {                       // 4: 7 - 14
                "$n #W $N's mana shield hard.",
                "You #w $N's mana shield hard.",
            "$n #W your mana shield hard."},
        {                       // 5: 11 - 19
                "$n #W $N's mana shield very hard.",
                "You #w $N's mana shield very hard.",
            "$n #W your mana shield very hard."},
        {                       // 6: 15 - 23
                "$n #W $N's mana shield extremely hard.",
                "You #w $N's mana shield extremely hard.",
            "$n #W your mana shield extremely hard."},
        {                       // 7: 19 - 27
                "$n massacres $N's mana shield to small fragments with $s #w.",
                "You massacre $N's mana shield to small fragments with your #w.",
            "$n massacres your mana shield to small fragments with $s #w."},
        {                       // 8: 23 - 32
                "$n devastates $N's mana shield with $s incredible #w!!",
                "You devastate $N's mana shield with your incredible #w!!",
            "$n devastates your mana shield with $s incredible #w!!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N's mana shield with $s deadly #w!!",
                "You OBLITERATE $N's mana shield with your deadly #w!!",
            "$n OBLITERATES your mana shield with $s deadly #w!!"},
        {                       // 10: 37 - 45
                "$n utterly DEMOLISHES $N's mana shield with $s unbelievable #w!!",
                "You utterly DEMOLISH $N's mana shield with your unbelievable #w!!",
            "$n utterly DEMOLISHES your mana shield with $s unbelievable #w!!"},
        {                       // 11: 46 - 69
                "$n PULVERIZES $N's mana shield with $s vicious #w!!",
                "You PULVERIZE $N's mana shield with your vicious #w!!",
            "$n PULVERIZES your mana shield with $s vicious #w!!"},
        {                       // 12: 70 - 99
                "$n *DECIMATES* $N's mana shield with $s horrible #w!!",
                "You *DECIMATE* $N's mana shield with your horrible #w!!",
            "$n *DECIMATES* your mana shield with $s horrible #w!!"},
        {                       // 13: 100 - 139
                "$n *LIQUIFIES* $N's mana shield with $s incredibly vicious #w!!",
                "You **LIQUIFY** $N's mana shield with your incredibly vicious #w!!",
            "$n *LIQUIFIES* your mana shield with $s incredibly vicious #w!!"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N's mana shield with $s terrible #w!!",
                "You **VAPORIZE** $N's mana shield with your terrible #w!!",
            "$n **VAPORIZES** your mana shield with $s terrible #w!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N's mana shield with $s ultra powerful #w!!",
                "You **ANNIHILATE** $N's mana shield with your ultra powerful #w!!",
            "$n **ANNIHILATES** your mana shield with $s ultra powerful #w!!"}
    };

    /* second set of possible mssgs, chosen ramdomly. */
    static struct dam_weapon_type dam_weapons_2[] = {

        {                       // 0: 0
                "$n tries to #w $N with $p, but misses.",
                "You try to #w $N with $p, but miss.",
            "$n tries to #w you with $p, but misses."},
        {                       // 1: 1 - 4
                "$n tickles $N as $e #W $M with $p.",
                "You tickle $N as you #w $M with $p.",
            "$n tickles you as $e #W you with $p."},
        {                       // 2: 3 - 6
                "$n barely #W $N with $p.",
                "You barely #w $N with $p.",
            "$n barely #W you with $p."},
        {                       // 3: 5 - 10
                "$n #W $N with $p.",
                "You #w $N with $p.",
            "$n #W you with $p."},
        {                       // 4: 7 - 14
                "$n #W $N hard with $p.",
                "You #w $N hard with $p.",
            "$n #W you hard with $p."},
        {                       // 5: 11 - 19
                "$n #W $N very hard with $p.",
                "You #w $N very hard with $p.",
            "$n #W you very hard with $p."},
        {                       // 6: 15 - 23
                "$n #W $N extremely hard with $p.",
                "You #w $N extremely hard with $p.",
            "$n #W you extremely hard with $p."},
        {                       // 7: 19 - 27
                "$n massacres $N to small fragments with $p.",
                "You massacre $N to small fragments with $p.",
            "$n massacres you to small fragments with $p."},
        {                       // 8: 23 - 32
                "$n devastates $N with $s incredible #w!!",
                "You devastate $N with a #w from $p!!",
            "$n devastates you with $s incredible #w!!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N with $p!!",
                "You OBLITERATE $N with $p!!",
            "$n OBLITERATES you with $p!!"},
        {                       // 10: 37 - 45
                "$n deals a DEMOLISHING #w to $N with $p!!",
                "You deal a DEMOLISHING #w to $N with $p!!",
            "$n deals a DEMOLISHING #w to you with $p!!"},
        {                       // 11: 46 - 79
                "$n PULVERIZES $N with $p!!",
                "You PULVERIZE $N with $p!!",
            "$n PULVERIZES you with $p!!"},
        {                       // 12: 70 - 99
                "$n *DECIMATES* $N with $p!!",
                "You *DECIMATE* $N with $p!!",
            "$n *DECIMATES* you with $p!!"},
        {                       // 13: 100 - 139
                "$n *LIQUIFIES* $N with a #w from $p!!",
                "You **LIQUIFY** $N with a #w from $p!!",
            "$n *LIQUIFIES* you with a #w from $p"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N with $p!!",
                "You **VAPORIZE** $N with a #w from $p!!",
            "$n **VAPORIZES** you with a #w from $p!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N with $p!!",
                "You **ANNIHILATE** $N with your #w from $p!!",
            "$n **ANNIHILATES** you with $s #w from $p!!"}
    };

    /* messages for body part specifics */
    static struct dam_weapon_type dam_weapons_location[] = {

        {                       // 0: 0
                "$n tries to #w $N's #p, but misses.",
                "You try to #w $N's #p, but miss.",
            "$n tries to #w your #p, but misses."},
        {                       // 1: 1 - 4
                "$n tickles $N's #p as $e #W $M.",
                "You tickle $N's #p as you #w $M.",
            "$n tickles you as $e #W your #p."},
        {                       // 2: 3 - 6
                "$n barely #W $N's #p.",
                "You barely #w $N's #p.",
            "$n barely #W your #p."},
        {                       // 3: 5 - 10
                "$n #W $N's #p.",
                "You #w $N's #p.",
            "$n #W your #p."},
        {                       // 4: 7 - 14
                "$n #W $N's #p hard.",
                "You #w $N's #p hard.",
            "$n #W your #p hard."},
        {                       // 5: 11 - 19
                "$n #W $N's #p very hard.",
                "You #w $N's #p very hard.",
            "$n #W your #p very hard."},
        {                       // 6: 15 - 23
                "$n #W $N's #p extremely hard.",
                "You #w $N's #p extremely hard.",
            "$n #W your #p extremely hard."},
        {                       // 7: 19 - 27
                "$n massacres $N's #p to fragments with $s #w.",
                "You massacre $N's #p to small fragments with your #w.",
            "$n massacres your #p to small fragments with $s #w."},
        {                       // 8: 23 - 32
                "$n devastates $N's #p with $s incredible #w!!",
                "You devastate $N's #p with your incredible #w!!",
            "$n devastates your #p with $s incredible #w!!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N's #p with $s #w!!",
                "You OBLITERATE $N's #p with your #w!!",
            "$n OBLITERATES your #p with $s #w!!"},
        {                       // 10: 37 - 45
                "$n deals a DEMOLISHING #w to $N's #p!!",
                "You deal a DEMOLISHING #w to $N's #p!!",
            "$n deals a DEMOLISHING #w to your #p!!"},
        {                       // 11: 46 - 69
                "$n PULVERIZES $N's #p with $s vicious #w!!",
                "You PULVERIZE $N's #p with your vicious #w!!",
            "$n PULVERIZES your #p with $s vicious #w!!"},
        {                       // 12: 70 - 99
                "$n *DECIMATES* $N's #p with $s horrible #w!!",
                "You *DECIMATE* $N's #p with your horrible #w!!",
            "$n *DECIMATES* your #p with $s horrible #w!!"},
        {                       // 13: 100 - 139
                "$n *LIQUIFIES* $N's #p with $s vicious #w!!",
                "You **LIQUIFY** $N's #p with your vicious #w!!",
            "$n *LIQUIFIES* your #p with $s vicious #w!!"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N's #p with $s terrible #w!!",
                "You **VAPORIZE** $N's #p with your terrible #w!!",
            "$n **VAPORIZES** your #p with $s terrible #w!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N's #p with $s ultra #w!!",
                "You **ANNIHILATE** $N's #p with your ultra #w!!",
            "$n **ANNIHILATES** your #p with $s ultra #w!!"}
    };

    /* fourth set of possible mssgs, IF RAYGUN. */
    static struct dam_weapon_type dam_guns[] = {

        {                       // 0: 0
                "$n tries to #w $N with $p, but misses.",
                "You try to #w $N with $p, but miss.",
            "$n tries to #w you with $p, but misses."},
        {                       // 1: 1 - 4
                "$n grazes $N with a #w from $p.",
                "You graze $N as you #w at $M with $p.",
            "$n grazes you as $e #W you with $p."},
        {                       // 2: 3 - 6
                "$n barely #W $N with $p.",
                "You barely #w $N with $p.",
            "$n barely #W you with $p."},
        {                       // 3: 5 - 10
                "$n #W $N with $p.",
                "You #w $N with $p.",
            "$n #W you with $p."},
        {                       // 4: 7 - 14
                "$n #W $N hard with $p.",
                "You #w $N hard with $p.",
            "$n #W you hard with $p."},
        {                       // 5: 11 - 19
                "$n #W $N very hard with $p.",
                "You #w $N very hard with $p.",
            "$n #W you very hard with $p."},
        {                       // 6: 15 - 23
                "$n #W the hell out of $N with $p.",
                "You #w the hell out of $N with $p.",
            "$n #W the hell out of you with $p."},
        {                       // 7: 19 - 27
                "$n #W $N to small fragments with $p.",
                "You #w $N to small fragments with $p.",
            "$n #W you to small fragments with $p."},
        {                       // 8: 23 - 32
                "$n devastates $N with a #w from $p!!",
                "You devastate $N with a #W from $p!!",
            "$n devastates you with $s #w from $p!!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N with a #w from $p!!",
                "You OBLITERATE $N with a #w from $p!!",
            "$n OBLITERATES you with a #w from $p!!"},
        {                       // 10: 37 - 45
                "$n DEMOLISHES $N with a dead on #w!!",
                "You DEMOLISH $N with a dead on blast from $p!!",
            "$n DEMOLISHES you with a dead on #w from $p!!"},
        {                       // 11: 46 - 79
                "$n PULVERIZES $N with a #w from $p!!",
                "You PULVERIZE $N with a #w from $p!!",
            "$n PULVERIZES you with a #w from $p!!"},
        {                       // 12: 80 - 99
                "$n *DECIMATES* $N with a #w from $p!!",
                "You *DECIMATE* $N with a #w from $p!!",
            "$n *DECIMATES* you with a #w from $p!!"},
        {                       // 13: 100 - 139
                "$n *LIQUIFIES* $N with a #w from $p!!",
                "You **LIQUIFY** $N with a #w from $p!!",
            "$n *LIQUIFIES* you with a #w from $p"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N with $p!!",
                "You **VAPORIZE** $N with a #w from $p!!",
            "$n **VAPORIZES** you with a #w from $p!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N with $p!!",
                "You **ANNIHILATE** $N with your #w from $p!!",
            "$n **ANNIHILATES** you with $s #w from $p!!"}

    };

    /* fifth set of possible mssgs, IF ENERGY_WEAPON. */
    static struct dam_weapon_type dam_energyguns[] = {

        {                       // 0: 0
                "$n misses $N with #s.",
                "You miss $N with #s.",
            "$n misses you with #s."},
        {                       // 1: 1 - 4
                "$n grazes $N with #s.",
                "You graze $N with #s.",
            "$n grazes you with #s."},
        {                       // 2: 3 - 6
                "$n barely marks $N with $p.",
                "You barely mark $N with #s.",
            "$n barely marks you with #s."},
        {                       // 3: 5 - 10
                "$n hurts $N with #s.",
                "You hurt $N with #s.",
            "$n hurts you with #s."},
        {                       // 4: 7 - 14
                "$n hurts $N badly with #s.",
                "You hurt $N badly with #s.",
            "$n hurts you badly with #s."},
        {                       // 5: 11 - 19
                "$n hurts $N very badly with #s.",
                "You hurt $N very badly with #s.",
            "$n hurts you very badly with #s."},
        {                       // 6: 15 - 23
                "$n ravages $N with #s!!",
                "You ravage $N with #s!",
            "$n ravages you with #s!"},
        {                       // 7: 19 - 27
                "$n massacre $N with #s!",
                "You massacre $N with #s!",
            "$n massacres you with #s!"},
        {                       // 8: 23 - 32
                "$n devastates $N with #s!",
                "You devastate $N with #s!",
            "$n devastates you with #s!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N with #s!!",
                "You OBLITERATE $N with #s!!",
            "$n OBLITERATES you with #s!!"},
        {                       // 10: 37 - 45
                "$n DEMOLISHES $N with #s!!",
                "You DEMOLISH $N with #s!!",
            "$n DEMOLISHES you with #s!!"},
        {                       // 11: 46 - 79
                "$n PULVERIZES $N with #s!!",
                "You PULVERIZE $N with #s!!",
            "$n PULVERIZES you with #s!!"},
        {                       // 12: 80 - 99
                "$n *DECIMATES* $N with #s!!",
                "You *DECIMATE* $N with #s!!",
            "$n *DECIMATES* you with #s!!"},
        {                       // 13: 100 - 139
                "$n *LIQUIFIES** $N with #s!!",
                "You *LIQUIFY* $N with #s!!",
            "$n *LIQUIFIES* you with #s!!"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N with #s!!",
                "You **VAPORIZE** $N with #s!!",
            "$n **VAPORIZES** you with #s!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N with #s!!",
                "You **ANNIHILATE** $N with #s!!",
            "$n **ANNIHILATES** you with #s!!"}

    };

    /* 6th set of possible mssgs, IF ENERGY_WEAPON version 2. */
    static struct dam_weapon_type dam_energyguns2[] = {

        {                       // 0: 0
                "#s misses $N as $n #W $p.",
                "#s misses $N as you #w $p.",
            "#s misses you as $n #W $p."},
        {                       // 1: 1 - 4
                "#s grazes $N as $n #W $p.",
                "#s grazes $N as you #w $p.",
            "#s grazes you as $n #W $p."},
        {                       // 2: 3 - 6
                "#s barely marks $N as $n #W $p.",
                "#s barely marks $N as you #w $p.",
            "#s barely marks you as $n #W $p."},
        {                       // 3: 5 - 10
                "#s hurts $N as $n #W $p.",
                "#s hurts $N as you #w $p.",
            "#s hurts you as $n #W $p."},
        {                       // 4: 7 - 14
                "#s hurts $N badly as $n #W $p.",
                "#s hurts $N badly as you #w $p.",
            "#s hurts you badly as $n #W $p."},
        {                       // 5: 11 - 19
                "#s hurts $N very badly as $n #W $p.",
                "#s hurts $N very badly as you #w $p.",
            "#s hurts you very badly as $n #W $p."},
        {                       // 6: 15 - 23
                "#s ravages $N as $n #W $p.",
                "#s ravages $N as you #w $p.",
            "#s ravages you as $n #W $p."},
        {                       // 7: 19 - 27
                "#s massacres $N as $n #W $p.",
                "#s massacres $N as you #w $p.",
            "#s massacres you as $n #W $p."},
        {                       // 8: 23 - 32
                "#s devastates $N as $n #W $p!",
                "#s devastates $N as you #w $p!",
            "#s devastates you as $n #W $p!"},
        {                       // 9: 32 - 37
                "#s OBLITERATES $N as $n #W $p!",
                "#s OBLITERATES $N as you #w $p!",
            "#s OBLITERATES you as $n #W $p!"},
        {                       // 10: 37 - 45
                "#s DEMOLISHES $N as $n #W $p!",
                "#s DEMOLISHES $N as you #w $p!",
            "#s DEMOLISHES you as $n #W $p!"},
        {                       // 11: 46 - 79
                "#s PULVERIZES $N as $n #W $p!",
                "#s PULVERIZES $N as you #w $p!",
            "#s PULVERIZES you as $n #W $p!"},
        {                       // 12: 80 - 99
                "#s *DECIMATES* $N as $n #W $p!",
                "#s *DECIMATES* $N as you #w $p!",
            "#s *DECIMATES* you as $n #W $p!"},
        {                       // 13: 100 - 139
                "#s *LIQUIFIES* $N as $n #W $p!!",
                "#s *LIQUIFIES* $N as you #w $p!!",
            "#s *LIQUIFIES* you as $n #W $p!!"},
        {                       // 14: 140 - 189
                "#s **VAPORIZES** $N as $n #W $p!!",
                "#s **VAPORIZES** $N as you #w $p!!",
            "#s **VAPORIZES** you as $n #W $p!!"},
        {                       // 15: >189
                "#s **ANNIHILATES** $N as $n #W $p!!",
                "#s **ANNIHILATES** $N as you #w $p!!",
            "#s **ANNIHILATES** you as $n #W $p!!"}

    };

    /* 7th set of possible mssgs, IF ENERGY_WEAPON version 3. */
    static struct dam_weapon_type dam_energyguns3[] = {

        {                       // 0: 0
                "$n misses $N with $p's #S.",
                "You miss $N with $p's #S.",
            "$n misses you with $p's #S."},
        {                       // 1: 1 - 4
                "$n grazes $N with $p's #S.",
                "You graze $N with $p's #S.",
            "$n grazes you with $p's #S."},
        {                       // 2: 3 - 6
                "$n barely marks $N with $p's #S.",
                "You barely mark $N with $p's #S.",
            "$n barely marks you with $p's #S."},
        {                       // 3: 5 - 10
                "$n hurts $N with $p's #S.",
                "You hurt $N with $p's #S.",
            "$n hurts you with $p's #S."},
        {                       // 4: 7 - 14
                "$n hurts $N badly with $p's #S.",
                "You hurt $N badly with $p's #S.",
            "$n hurts you badly with $p's #S."},
        {                       // 5: 11 - 19
                "$n hurts $N very badly with $p's #S.",
                "You hurt $N very badly with $p's #S.",
            "$n hurts you very badly with $p's #S."},
        {                       // 6: 15 - 23
                "$n ravages $N with $p's #S.",
                "You ravage $N with $p's #S.",
            "$n ravages you with $p's #S."},
        {                       // 7: 19 - 27
                "$n massacres $N with $p's #S.",
                "You massacre $N with $p's #S.",
            "$n massacres you with $p's #S."},
        {                       // 8: 23 - 32
                "$n devastates $N with $p's #S!",
                "You devastate $N with $p's #S!",
            "$n devastates you with $p's #S!"},
        {                       // 9: 32 - 37
                "$n OBLITERATES $N with $p's #S!",
                "You OBLITERATE $N with $p's #S!",
            "$n OBLITERATES you with $p's #S!"},
        {                       // 10: 37 - 45
                "$n DEMOLISHES $N with $p's #S!",
                "You DEMOLISH $N with $p's #S!",
            "$n DEMOLISHES you with $p's #S!"},
        {                       // 11: 46 - 79
                "$n PULVERIZES $N with $p's #S!",
                "You PULVERIZE $N with $p's #S!",
            "$n PULVERIZES you with $p's #S!"},
        {                       // 12: 80 - 99
                "$n **DECIMATES** $N with $p's #S!",
                "You **DECIMATE** $N with $p's #S!",
            "$n **DECIMATES** you with $p's #S!"},
        {                       // 13: 100 - 139
                "$n **LIQUIFIES** $N with $p's #S!!",
                "You **LIQUIFY** $N with $p's #S!!",
            "$n **LIQUIFIES** you with $p's #S!!"},
        {                       // 14: 140 - 189
                "$n **VAPORIZES** $N with $p's #S!!",
                "You **VAPORIZE** $N with $p's #S!!",
            "$n **VAPORIZES** you with $p's #S!!"},
        {                       // 15: >189
                "$n **ANNIHILATES** $N with $p's #S!!",
                "You **ANNIHILATE** $N with $p's #S!!",
            "$n **ANNIHILATES** you with $p's #S!!"}

    };

    if (search_nomessage)
        return;

    if (w_type >= TYPE_EGUN_LASER && w_type <= TYPE_EGUN_TOP)
        w_type = TYPE_ENERGY_GUN;

    if (w_type == SKILL_PROJ_WEAPONS)
        w_type = TYPE_BLAST;

    if (w_type == SKILL_ARCHERY)
        return;

    if (dam == 0)
        msgnum = 0;
    else if (dam <= 4)
        msgnum = 1;
    else if (dam <= 6)
        msgnum = 2;
    else if (dam <= 10)
        msgnum = 3;
    else if (dam <= 14)
        msgnum = 4;
    else if (dam <= 19)
        msgnum = 5;
    else if (dam <= 23)
        msgnum = 6;
    else if (dam <= 27)
        msgnum = 7;             /* fragments  */
    else if (dam <= 32)
        msgnum = 8;             /* devastate  */
    else if (dam <= 37)
        msgnum = 9;             /* obliterate */
    else if (dam <= 45)
        msgnum = 10;            /* demolish   */
    else if (dam <= 69)
        msgnum = 11;            /* pulverize  */
    else if (dam <= 94)
        msgnum = 12;            /* decimate   */
    else if (dam <= 129)
        msgnum = 13;            /* liquify    */
    else if (dam <= 169)
        msgnum = 14;            /* vaporize   */
    else
        msgnum = 15;            /* annihilate */

    if ((w_type < TYPE_HIT) || (w_type > TOP_ATTACKTYPE))
        w_type = TYPE_HIT;

    w_type -= TYPE_HIT;

    /* damage message to onlookers */
    if (location == WEAR_MSHIELD)   // Mana shield hit
        buf = replace_string(dam_mana_shield[msgnum].to_room,
            attack_hit_text[w_type].singular,
            attack_hit_text[w_type].plural, NULL, NULL);
    else if (weapon && IS_ENERGY_GUN(weapon)
        && w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) {
        int guntype = GET_OBJ_VAL(weapon, 3);
        if (guntype > EGUN_TOP)
            guntype = EGUN_TOP;
        if (!number(0, 2))
            buf = replace_string(dam_energyguns[msgnum].to_room,
                gun_hit_text[guntype].singular, gun_hit_text[guntype].plural,
                NULL, gun_hit_text[guntype].substance);
        else if (!number(0, 1))
            buf = replace_string(dam_energyguns2[msgnum].to_room,
                gun_hit_text[guntype].singular, gun_hit_text[guntype].plural,
                NULL, gun_hit_text[guntype].substance);
        else
            buf = replace_string(dam_energyguns3[msgnum].to_room,
                gun_hit_text[guntype].singular, gun_hit_text[guntype].plural,
                NULL, gun_hit_text[guntype].substance);
    } else if (weapon && IS_GUN(weapon) && w_type == (TYPE_BLAST - TYPE_HIT))
        buf = replace_string(dam_guns[msgnum].to_room,
            attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
            NULL, NULL);
    else if (location >= 0 && POS_DAMAGE_OK(location) && !number(0, 2) &&
        (!weapon || weapon->worn_on == WEAR_WIELD))
        buf = replace_string(dam_weapons_location[msgnum].to_room,
            attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
            wear_keywords[wear_translator[location]], NULL);
    else if (weapon && (number(0, 2) || weapon->worn_on != WEAR_WIELD))
        buf = replace_string(dam_weapons_2[msgnum].to_room,
            attack_hit_text[w_type].singular,
            attack_hit_text[w_type].plural, NULL, NULL);
    else
        buf = replace_string(dam_weapons[msgnum].to_room,
            attack_hit_text[w_type].singular,
            attack_hit_text[w_type].plural, NULL, NULL);
    act(buf, false, ch, weapon, victim, TO_NOTVICT);
    /* damage message to damager */
    if ((msgnum || !PRF_FLAGGED(ch, PRF_GAGMISS)) && ch->desc) {
        if (location == WEAR_MSHIELD)   // Mana shield hit
            buf = replace_string(dam_mana_shield[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else if (weapon && IS_ENERGY_GUN(weapon)
            && w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) {
            int guntype = GET_OBJ_VAL(weapon, 3);
            if (guntype > EGUN_TOP)
                guntype = EGUN_TOP;
            if (!number(0, 2))
                buf = replace_string(dam_energyguns[msgnum].to_char,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
            else if (!number(0, 1))
                buf = replace_string(dam_energyguns2[msgnum].to_char,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
            else
                buf = replace_string(dam_energyguns3[msgnum].to_char,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
        } else if (weapon && IS_GUN(weapon) && w_type == (TYPE_BLAST - TYPE_HIT))
            buf = replace_string(dam_guns[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else if (location >= 0 && POS_DAMAGE_OK(location) && !number(0, 2)) {
            buf = replace_string(dam_weapons_location[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural,
                wear_keywords[wear_translator[location]], NULL);
        } else if (weapon && (number(0, 4) || weapon->worn_on != WEAR_WIELD))
            buf = replace_string(dam_weapons_2[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else
            buf = replace_string(dam_weapons[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        if (location == WEAR_MSHIELD)
            send_to_char(ch, "%s", CCMAG(ch, C_NRM));
        else
            send_to_char(ch, "%s", CCYEL(ch, C_NRM));
        act(buf, false, ch, weapon, victim, TO_CHAR | TO_SLEEP);
        send_to_char(ch, "%s", CCNRM(ch, C_NRM));
    }
    /* damage message to damagee */
    if ((msgnum || !PRF_FLAGGED(victim, PRF_GAGMISS)) && victim->desc) {
        if (location == WEAR_MSHIELD)   // Mana shield hit
            buf = replace_string(dam_mana_shield[msgnum].to_victim,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else if (weapon && IS_ENERGY_GUN(weapon)
            && w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) {
            int guntype = GET_OBJ_VAL(weapon, 3);
            if (guntype > EGUN_TOP)
                guntype = EGUN_TOP;
            if (!number(0, 2))
                buf = replace_string(dam_energyguns[msgnum].to_victim,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
            else if (!number(0, 1))
                buf = replace_string(dam_energyguns2[msgnum].to_victim,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
            else
                buf = replace_string(dam_energyguns3[msgnum].to_victim,
                    gun_hit_text[guntype].singular,
                    gun_hit_text[guntype].plural, NULL,
                    gun_hit_text[guntype].substance);
        } else if (weapon && IS_GUN(weapon) && w_type == (TYPE_BLAST - TYPE_HIT))
            buf = replace_string(dam_guns[msgnum].to_char,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else if (location >= 0 && POS_DAMAGE_OK(location) && !number(0, 2)) {
            buf = replace_string(dam_weapons_location[msgnum].to_victim,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural,
                wear_keywords[wear_translator[location]], NULL);
        } else if (weapon && (number(0, 3) || weapon->worn_on != WEAR_WIELD))
            buf = replace_string(dam_weapons_2[msgnum].to_victim,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        else
            buf = replace_string(dam_weapons[msgnum].to_victim,
                attack_hit_text[w_type].singular,
                attack_hit_text[w_type].plural, NULL, NULL);
        if (location == WEAR_MSHIELD)
            send_to_char(victim, "%s", CCCYN(victim, C_NRM));
        else
            send_to_char(victim, "%s", CCRED(victim, C_NRM));
        act(buf, false, ch, weapon, victim, TO_VICT | TO_SLEEP);
        send_to_char(victim, "%s", CCNRM(victim, C_NRM));
    }

    if (BLOODLET(victim, dam, w_type + TYPE_HIT))
        blood_spray(ch, victim, weapon, dam, w_type + TYPE_HIT);
}

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int
skill_message(int dam, struct creature *ch, struct creature *vict,
              struct obj_data *weapon, int attacktype)
{
    int i, j, nr;
    struct message_type *msg;

    if (search_nomessage)
        return 1;
    for (i = 0; i < MAX_MESSAGES; i++) {
        if (fight_messages[i].a_type == attacktype) {
            // Advance to random message for this attack
            nr = dice(1, fight_messages[i].number_of_attacks);
            for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
                msg = msg->next;

            // Log an error if no message was found
            if (!msg) {
                errlog("Message %d missing for attacktype == %d",
                    j, attacktype);
                return 1;
            }
            // occassionally behead while slashing
            if (attacktype == TYPE_SLASH && nr == 1
                && !isname("headless", vict->player.name))
                corpse_state = SKILL_BEHEAD;
            else
                corpse_state = 0;

            if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_AMBASSADOR &&
                (!PLR_FLAGGED(vict, PLR_MORTALIZED) || dam == 0)) {
                act(msg->god_msg.attacker_msg, false, ch, weapon, vict, TO_CHAR);
                act(msg->god_msg.victim_msg, false, ch, weapon, vict, TO_VICT);
                act(msg->god_msg.room_msg, false, ch, weapon, vict, TO_NOTVICT);
            } else if (dam != 0) {
                if (GET_POSITION(vict) == POS_DEAD) {
                    if (ch) {
                        act(msg->die_msg.room_msg, false, ch, weapon, vict,
                            TO_NOTVICT | TO_VICT_RM);
                        if (ch != vict &&
                            find_distance(ch->in_room, vict->in_room) < 3) {
                            send_to_char(ch, "%s", CCYEL(ch, C_NRM));
                            act(msg->die_msg.attacker_msg, false, ch, weapon,
                                vict, TO_CHAR);
                            send_to_char(ch, "%s", CCNRM(ch, C_NRM));
                        }

                    }

                    send_to_char(vict, "%s", CCRED(vict, C_NRM));
                    act(msg->die_msg.victim_msg, false, ch, weapon, vict,
                        TO_VICT | TO_SLEEP);
                    send_to_char(vict, "%s", CCNRM(vict, C_NRM));

                } else {
                    if (ch) {
                        act(msg->hit_msg.room_msg, false, ch, weapon, vict,
                            TO_NOTVICT | TO_VICT_RM);
                        if (ch != vict && ch->in_room == vict->in_room) {
                            send_to_char(ch, "%s", CCYEL(ch, C_NRM));
                            act(msg->hit_msg.attacker_msg, false, ch, weapon,
                                vict, TO_CHAR);
                            send_to_char(ch, "%s", CCNRM(ch, C_NRM));
                        }
                    }

                    send_to_char(vict, "%s", CCRED(vict, C_NRM));
                    act(msg->hit_msg.victim_msg, false, ch, weapon, vict,
                        TO_VICT | TO_SLEEP);
                    send_to_char(vict, "%s", CCNRM(vict, C_NRM));

                }
            } else if (ch != vict) {    /* Dam == 0 */
                if (ch && (!IS_WEAPON(attacktype)
                        || !PRF_FLAGGED(ch, PRF_GAGMISS))) {
                    if (ch->in_room == vict->in_room) {
                        send_to_char(ch, "%s", CCYEL(ch, C_NRM));
                        act(msg->miss_msg.attacker_msg, false, ch, weapon, vict,
                            TO_CHAR);
                        send_to_char(ch, "%s", CCNRM(ch, C_NRM));
                    }

                    act(msg->miss_msg.room_msg, false, ch, weapon, vict,
                        TO_NOTVICT | TO_VICT_RM);
                }

                if (!IS_WEAPON(attacktype)
                    || !PRF_FLAGGED(vict, PRF_GAGMISS)) {
                    send_to_char(vict, "%s", CCRED(vict, C_NRM));
                    act(msg->miss_msg.victim_msg, false, ch, weapon, vict,
                        TO_VICT | TO_SLEEP);
                    send_to_char(vict, "%s", CCNRM(vict, C_NRM));
                }
            }
            if (BLOODLET(vict, dam, attacktype))
                blood_spray(ch, vict, weapon, dam, attacktype);

            return 1;
        }
    }
    return 0;
}

#undef __combat_code__
#undef __combat_messages__
