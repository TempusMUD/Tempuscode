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
#include "shop.h"
#include "guns.h"
#include "specs.h"

#include <iostream>
extern int corpse_state;
char *replace_string(char *str, char *weapon_singular, char *weapon_plural,
	const char *location);


void
appear(struct Creature *ch, struct Creature *vict)
{
	char *to_char = NULL;
	int found = 0;
	//  int could_see = CAN_SEE( vict, ch );

	if (!AFF_FLAGGED(vict, AFF_DETECT_INVIS)) {
		if (affected_by_spell(ch, SPELL_INVISIBLE)) {
			affect_from_char(ch, SPELL_INVISIBLE);
			send_to_char(ch, "Your invisibility spell has expired.\r\n");
			found = 1;
		}
		if (affected_by_spell(ch, SPELL_TRANSMITTANCE)) {
			affect_from_char(ch, SPELL_TRANSMITTANCE);
			send_to_char(ch, "Your transparency has expired.\r\n");
			found = 1;
		}
	}

	if (IS_ANIMAL(vict) && affected_by_spell(ch, SPELL_INVIS_TO_ANIMALS)) {
		affect_from_char(ch, SPELL_INVIS_TO_ANIMALS);
		send_to_char(ch, "Your animal invisibility has expired.\r\n");
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
		GET_REMORT_INVIS(ch) > GET_LEVEL(vict)) {
		GET_REMORT_INVIS(ch) = GET_LEVEL(vict);
		send_to_char(ch, "You feel a bit more visible.\n");
		found = 1;
	}

	if (found) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			act("$n suddenly appears, seemingly from nowhere.",
				TRUE, ch, 0, 0, TO_ROOM);
			if (to_char)
				send_to_char(ch, to_char);
			else
				send_to_char(ch, "You fade into visibility.\r\n");
		} else
			act("You feel a strange presence as $n appears, seemingly from nowhere.", FALSE, ch, 0, 0, TO_ROOM);
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
		sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
		perror(buf2);
		safe_exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++) {
		fight_messages[i].a_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg = 0;
	}


	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		fgets(chk, 128, fl);

	while (*chk == 'M') {
		fgets(chk, 128, fl);
		sscanf(chk, " %d\n", &type);
		for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
			(fight_messages[i].a_type); i++);
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
		fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
			fgets(chk, 128, fl);
	}

	sprintf(buf, "Top message number loaded: %d.", type);
	slog(buf);
	fclose(fl);
}

void
death_cry(struct Creature *ch)
{
	struct room_data *adjoin_room = NULL;
	int door;
	struct room_data *was_in = NULL;
	int found = 0;

	if (GET_MOB_SPEC(ch) == fate)
		act("$n dissipates in a cloud of mystery, leaving you to your fate.",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_GHOUL(ch) || IS_WIGHT(ch) || IS_MUMMY(ch))
		act("$n falls lifeless to the floor with a shriek.",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_SKELETON(ch))
		act("$n clatters noisily into a lifeless heap.",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_GHOST(ch) || IS_SHADOW(ch) || IS_WRAITH(ch) || IS_SPECTRE(ch))
		act("$n vanishes into the void with a terrible shriek.",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_VAMPIRE(ch))
		act("$n screams as $e is consumed in a blazing fire!",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_LICH(ch))
		act("A roar fills your ears as $n is ripped into the void!",
			FALSE, ch, 0, 0, TO_ROOM);
	else if (IS_UNDEAD(ch))
		act("Your skin crawls as you hear $n's final shriek.",
			FALSE, ch, 0, 0, TO_ROOM);
	else {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (*it == ch)
				continue;
			found = 0;

			if (!found && IS_BARB((*it)) && !number(0, 1)) {
				found = 1;
				act("You feel a rising bloodlust as you hear $n's death cry.",
					FALSE, ch, 0, (*it), TO_VICT);
			}

			if (!found)
				act("Your blood freezes as you hear $n's death cry.",
					FALSE, ch, 0, (*it), TO_VICT);
		}
	}
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (ch != *it && (*it)->getPosition() == POS_SLEEPING &&
			!PLR_FLAGGED((*it), PLR_OLC | PLR_WRITING) &&
			!AFF_FLAGGED((*it), AFF_SLEEP)) {
			(*it)->setPosition(POS_RESTING);
		}
	}

	was_in = ch->in_room;

	for (door = 0; door < NUM_OF_DIRS && !found; door++) {
		if (CAN_GO(ch, door) && ch->in_room != EXIT(ch, door)->to_room) {
			ch->in_room = was_in->dir_option[door]->to_room;
			sprintf(buf,
				"Your blood freezes as you hear someone's death cry from %s.",
				from_dirs[door]);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			adjoin_room = ch->in_room;
			ch->in_room = was_in;
			if (adjoin_room->dir_option[rev_dir[door]] &&
				adjoin_room->dir_option[rev_dir[door]]->to_room == was_in) {
				CreatureList::iterator it = adjoin_room->people.begin();
				for (; it != adjoin_room->people.end(); ++it) {
					if (IS_MOB((*it)) && !MOB_FLAGGED((*it), MOB_SENTINEL) &&
						!FIGHTING((*it)) && AWAKE((*it)) &&
						(MOB_FLAGGED((*it), MOB_HELPER) ||
							(*it)->mob_specials.shared->func ==
							cityguard) && number(0, 40) < GET_LEVEL((*it))) {
						if ((!ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED) ||
								CHAR_WITHSTANDS_FIRE((*it))) &&
							(!ROOM_FLAGGED(ch->in_room, ROOM_ICE_COLD) ||
								CHAR_WITHSTANDS_COLD((*it))) &&
							(!IS_DARK(ch->in_room)
								|| CAN_SEE_IN_DARK((*it)))) {

							int move_result =
								do_simple_move((*it), rev_dir[door], MOVE_RUSH,
								1);

							if (move_result == 0) {
								found = number(0, 1);
								break;
							} else if (move_result == 2) {
								found = 1;
								break;
							}
						}
					}
				}
			}
		}
	}
}

void
blood_spray(struct Creature *ch, struct Creature *victim,
	int dam, int attacktype)
{
	char *to_char, *to_vict, *to_notvict;
	int pos, found = 0;
	struct Creature *nvict;

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

	sprintf(buf,
		to_char,
		attacktype >= TYPE_HIT ?
		attack_hit_text[attacktype - TYPE_HIT].singular : spell_to_str(attacktype));
	send_to_char(ch, CCRED(ch, C_NRM));
	act(buf, FALSE, ch, 0, victim, TO_CHAR);
	send_to_char(ch, CCNRM(ch, C_NRM));

	sprintf(buf,
		to_vict,
		attacktype >= TYPE_HIT ?
		attack_hit_text[attacktype - TYPE_HIT].singular : spell_to_str(attacktype));
	send_to_char(victim, CCRED(victim, C_NRM));
	act(buf, FALSE, ch, 0, victim, TO_VICT);
	send_to_char(victim, CCNRM(victim, C_NRM));

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((*it) == ch || (*it) == victim || !(*it)->desc || !AWAKE((*it)))
			continue;
	}

	sprintf(buf,
		to_notvict,
		attacktype >= TYPE_HIT ?
		attack_hit_text[attacktype - TYPE_HIT].singular : spell_to_str(attacktype));

	act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
	it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((*it) == ch || (*it) == victim || !(*it)->desc || !AWAKE((*it)))
			continue;
	}

	//
	// evil clerics get a bonus for sending blood flying!
	//

	if (IS_CLERIC(ch) && IS_EVIL(ch)) {
		GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 20);
		GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20);
		GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20);
	}
	it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		nvict = *it;
		if (nvict == victim || IS_NPC(nvict))
			continue;

		if (number(5, 30) > GET_DEX(nvict) && (nvict != ch || !number(0, 2))) {
			if ((pos =
					apply_soil_to_char(nvict, NULL, SOIL_BLOOD,
						WEAR_RANDOM))) {
				sprintf(buf, "$N's blood splatters all over your %s!",
					GET_EQ(nvict, pos) ? GET_EQ(nvict,
						pos)->short_description : wear_description[pos]);
				act(buf, FALSE, nvict, 0, victim, TO_CHAR);
				found = 1;

				if (nvict == ch && IS_CLERIC(ch) && IS_EVIL(ch)) {
					GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 20);
					GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 20);
					GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + 20);
				}

				break;
			}
		}
	}

	if (!found)
		add_blood_to_room(ch->in_room, 1);

	if (cur_weap && cur_weap->worn_by &&
		cur_weap == GET_EQ(ch, cur_weap->worn_on) &&
		!OBJ_SOILED(cur_weap, SOIL_BLOOD)) {
		apply_soil_to_char(ch, NULL, SOIL_BLOOD, cur_weap->worn_on);
	}
}

/* message for doing damage with a weapon */
void
dam_message(int dam, struct Creature *ch, struct Creature *victim,
	int w_type, int location)
{
	char *buf;
	int msgnum;

	struct obj_data *weap = cur_weap;

	static struct dam_weapon_type {
		char *to_room;
		char *to_char;
		char *to_victim;
	} dam_weapons[] = {

		/* use #w for singular ( i.e. "slash" ) and #W for plural ( i.e. "slashes" ) */

		{
			"$n tries to #w $N, but misses.",	/* 0: 0     */
		"You try to #w $N, but miss.",
				"$n tries to #w you, but misses."}, {
			"$n tickles $N as $e #W $M.",	/* 1: 1..4  */
		"You tickle $N as you #w $M.", "$n tickles you as $e #W you."}, {
			"$n barely #W $N.",	/* 2: 3..6  */
		"You barely #w $N.", "$n barely #W you."}, {
			"$n #W $N.",		/* 3: 5..10 */
		"You #w $N.", "$n #W you."}, {
			"$n #W $N hard.",	/* 4: 7..14  */
		"You #w $N hard.", "$n #W you hard."}, {
			"$n #W $N very hard.",	/* 5: 11..19  */
		"You #w $N very hard.", "$n #W you very hard."}, {
			"$n #W $N extremely hard.",	/* 6: 15..23  */
		"You #w $N extremely hard.", "$n #W you extremely hard."}, {
			"$n massacres $N to small fragments with $s #w.",	/* 7: 19..27 */
		"You massacre $N to small fragments with your #w.",
				"$n massacres you to small fragments with $s #w."}, {
			"$n devastates $N with $s incredible #w!!",	/* 8: 23..32 */
		"You devastate $N with your incredible #w!!",
				"$n devastates you with $s incredible #w!!"}, {
			"$n OBLITERATES $N with $s deadly #w!!",	/* 9: 32..37   */
		"You OBLITERATE $N with your deadly #w!!",
				"$n OBLITERATES you with $s deadly #w!!"}, {
			"$n utterly DEMOLISHES $N with $s unbelievable #w!!",	/* 10: 37..45 */
		"You utterly DEMOLISH $N with your unbelievable #w!!",
				"$n utterly DEMOLISHES you with $s unbelievable #w!!"}, {
			"$n PULVERIZES $N with $s viscious #w!!",	/* 11: 46..69 */
		"You PULVERIZE $N with your viscious #w!!",
				"$n PULVERIZES you with $s viscious #w!!"}, {
			"$n *DECIMATES* $N with $s horrible #w!!",	/* 12: 70..99 */
		"You *DECIMATE* $N with your horrible #w!!",
				"$n *DECIMATES* you with $s horrible #w!!"}, {
			"$n *LIQUIFIES* $N with $s incredibly viscious #w!!",	/* 13: 100-139 */
		"You **LIQUIFY** $N with your incredibly viscious #w!!",
				"$n *LIQUIFIES* you with $s incredibly viscious #w!!"}, {
			"$n **VAPORIZES** $N with $s terrible #w!!",	/* 14: 14-189 */
		"You **VAPORIZE** $N with your terrible #w!!",
				"$n **VAPORIZES** you with $s terrible #w!!"}, {
			"$n **ANNIHILATES** $N with $s ultra powerful #w!!",	/* 15: >189 */
		"You **ANNIHILATE** $N with your ultra powerful #w!!",
				"$n **ANNIHILATES** you with $s ultra powerful #w!!"}

	};

	/* second set of possible mssgs, chosen ramdomly. */
	static struct dam_weapon_type dam_weapons_2[] = {

		{
				"$n tries to #w $N with $p, but misses.",	/* 0: 0     */
				"You try to #w $N with $p, but miss.",
			"$n tries to #w you with $p, but misses."},

		{
				"$n tickles $N as $e #W $M with $p.",	/* 1: 1..4  */
				"You tickle $N as you #w $M with $p.",
			"$n tickles you as $e #W you with $p."},

		{
				"$n barely #W $N with $p.",	/* 2: 3..6  */
				"You barely #w $N with $p.",
			"$n barely #W you with $p."},

		{
				"$n #W $N with $p.",	/* 3: 5..10 */
				"You #w $N with $p.",
			"$n #W you with $p."},

		{
				"$n #W $N hard with $p.",	/* 4: 7..14  */
				"You #w $N hard with $p.",
			"$n #W you hard with $p."},

		{
				"$n #W $N very hard with $p.",	/* 5: 11..19  */
				"You #w $N very hard with $p.",
			"$n #W you very hard with $p."},

		{
				"$n #W $N extremely hard with $p.",	/* 6: 15..23  */
				"You #w $N extremely hard with $p.",
			"$n #W you extremely hard with $p."},

		{
				"$n massacres $N to small fragments with $p.",	/* 7: 19..27 */
				"You massacre $N to small fragments with $p.",
			"$n massacres you to small fragments with $p."},

		{
				"$n devastates $N with $s incredible #w!!",	/* 8: 23..32 */
				"You devastate $N with a #w from $p!!",
			"$n devastates you with $s incredible #w!!"},

		{
				"$n OBLITERATES $N with $p!!",	/* 9: 32..37   */
				"You OBLITERATE $N with $p!!",
			"$n OBLITERATES you with $p!!"},

		{
				"$n deals a DEMOLISHING #w to $N with $p!!",	/* 10: 37..45 */
				"You deal a DEMOLISHING #w to $N with $p!!",
			"$n deals a DEMOLISHING #w to you with $p!!"},

		{
				"$n PULVERIZES $N with $p!!",	/* 11: 46..79 */
				"You PULVERIZE $N with $p!!",
			"$n PULVERIZES you with $p!!"},

		{
				"$n *DECIMATES* $N with $p!!",	/* 12: 70..99 */
				"You *DECIMATE* $N with $p!!",
			"$n *DECIMATES* you with $p!!"},

		{
				"$n *LIQUIFIES* $N with a #w from $p!!",	/* 13: 100-139 */
				"You **LIQUIFY** $N with a #w from $p!!",
			"$n *LIQUIFIES* you with a #w from $p"},

		{
				"$n **VAPORIZES** $N with $p!!",	/* 14: 14-189 */
				"You **VAPORIZE** $N with a #w from $p!!",
			"$n **VAPORIZES** you with a #w from $p!!"},

		{
				"$n **ANNIHILATES** $N with $p!!",	/* 15: >189 */
				"You **ANNIHILATE** $N with your #w from $p!!",
			"$n **ANNIHILATES** you with $s #w from $p!!"}

	};

	/* messages for body part specifics */
	static struct dam_weapon_type dam_weapons_location[] = {

		{
				"$n tries to #w $N's #p, but misses.",	/* 0: 0     */
				"You try to #w $N's #p, but miss.",
			"$n tries to #w your #p, but misses."},

		{
				"$n tickles $N's #p as $e #W $M.",	/* 1: 1..4  */
				"You tickle $N's #p as you #w $M.",
			"$n tickles you as $e #W your #p."},

		{
				"$n barely #W $N's #p.",	/* 2: 3..6  */
				"You barely #w $N's #p.",
			"$n barely #W your #p."},

		{
				"$n #W $N's #p.",	/* 3: 5..10 */
				"You #w $N's #p.",
			"$n #W your #p."},

		{
				"$n #W $N's #p hard.",	/* 4: 7..14  */
				"You #w $N's #p hard.",
			"$n #W your #p hard."},

		{
				"$n #W $N's #p very hard.",	/* 5: 11..19  */
				"You #w $N's #p very hard.",
			"$n #W your #p very hard."},

		{
				"$n #W $N's #p extremely hard.",	/* 6: 15..23  */
				"You #w $N's #p extremely hard.",
			"$n #W your #p extremely hard."},

		{
				"$n massacres $N's #p to fragments with $s #w.",	/* 7: 19..27 */
				"You massacre $N's #p to small fragments with your #w.",
			"$n massacres your #p to small fragments with $s #w."},

		{
				"$n devastates $N's #p with $s incredible #w!!",	/* 8: 23..32 */
				"You devastate $N's #p with your incredible #w!!",
			"$n devastates your #p with $s incredible #w!!"},

		{
				"$n OBLITERATES $N's #p with $s #w!!",	/* 9: 32..37   */
				"You OBLITERATE $N's #p with your #w!!",
			"$n OBLITERATES your #p with $s #w!!"},

		{
				"$n deals a DEMOLISHING #w to $N's #p!!",	/* 10: 37..45 */
				"You deal a DEMOLISHING #w to $N's #p!!",
			"$n deals a DEMOLISHING #w to your #p!!"},

		{
				"$n PULVERIZES $N's #p with $s viscious #w!!",	/* 11: 46..69 */
				"You PULVERIZE $N's #p with your viscious #w!!",
			"$n PULVERIZES your #p with $s viscious #w!!"},

		{
				"$n *DECIMATES* $N's #p with $s horrible #w!!",	/* 12: 70..99 */
				"You *DECIMATE* $N's #p with your horrible #w!!",
			"$n *DECIMATES* your #p with $s horrible #w!!"},

		{
				"$n *LIQUIFIES* $N's #p with $s viscious #w!!",	/* 13: 100-139 */
				"You **LIQUIFY** $N's #p with your viscious #w!!",
			"$n *LIQUIFIES* your #p with $s viscious #w!!"},

		{
				"$n **VAPORIZES** $N's #p with $s terrible #w!!",	/* 14: 14-189 */
				"You **VAPORIZE** $N's #p with your terrible #w!!",
			"$n **VAPORIZES** your #p with $s terrible #w!!"},

		{
				"$n **ANNIHILATES** $N's #p with $s ultra #w!!",	/* 15: >189 */
				"You **ANNIHILATE** $N's #p with your ultra #w!!",
			"$n **ANNIHILATES** your #p with $s ultra #w!!"}


	};

	/* fourth set of possible mssgs, IF RAYGUN. */
	static struct dam_weapon_type dam_guns[] = {

		{
				"$n tries to #w $N with $p, but misses.",	/* 0: 0     */
				"You try to #w $N with $p, but miss.",
			"$n tries to #w you with $p, but misses."},

		{
				"$n grazes $N with a #w from $p.",	/* 1: 1..4  */
				"You graze $N as you #w at $M with $p.",
			"$n grazes you as $e #W you with $p."},

		{
				"$n barely #W $N with $p.",	/* 2: 3..6  */
				"You barely #w $N with $p.",
			"$n barely #W you with $p."},

		{
				"$n #W $N with $p.",	/* 3: 5..10 */
				"You #w $N with $p.",
			"$n #W you with $p."},

		{
				"$n #W $N hard with $p.",	/* 4: 7..14  */
				"You #w $N hard with $p.",
			"$n #W you hard with $p."},

		{
				"$n #W $N very hard with $p.",	/* 5: 11..19  */
				"You #w $N very hard with $p.",
			"$n #W you very hard with $p."},

		{
				"$n #W the hell out of $N with $p.",	/* 6: 15..23  */
				"You #w the hell out of $N with $p.",
			"$n #W the hell out of you with $p."},

		{
				"$n #W $N to small fragments with $p.",	/* 7: 19..27 */
				"You #w $N to small fragments with $p.",
			"$n #W you to small fragments with $p."},

		{
				"$n devastates $N with a #w from $p!!",	/* 8: 23..32 */
				"You devastate $N with a #W from $p!!",
			"$n devastates you with $s #w from $p!!"},

		{
				"$n OBLITERATES $N with a #w from $p!!",	/* 9: 32..37   */
				"You OBLITERATE $N with a #w from $p!!",
			"$n OBLITERATES you with a #w from $p!!"},

		{
				"$n DEMOLISHES $N with a dead on #w!!",	/* 10: 37..45 */
				"You DEMOLISH $N with a dead on blast from $p!!",
			"$n DEMOLISHES you with a dead on #w from $p!!"},

		{
				"$n PULVERIZES $N with a #w from $p!!",	/* 11: 46..79 */
				"You PULVERIZE $N with a #w from $p!!",
			"$n PULVERIZES you with a #w from $p!!"},

		{
				"$n *DECIMATES* $N with a #w from $p!!",	/* 12: 70..99 */
				"You *DECIMATE* $N with a #w from $p!!",
			"$n *DECIMATES* you with a #w from $p!!"},

		{
				"$n *LIQUIFIES* $N with a #w from $p!!",	/* 13: 100-139 */
				"You **LIQUIFY** $N with a #w from $p!!",
			"$n *LIQUIFIES* you with a #w from $p"},

		{
				"$n **VAPORIZES** $N with $p!!",	/* 14: 14-189 */
				"You **VAPORIZE** $N with a #w from $p!!",
			"$n **VAPORIZES** you with a #w from $p!!"},

		{
				"$n **ANNIHILATES** $N with $p!!",	/* 15: >189 */
				"You **ANNIHILATE** $N with your #w from $p!!",
			"$n **ANNIHILATES** you with $s #w from $p!!"}

	};

	if (search_nomessage)
		return;

	if (w_type == SKILL_ENERGY_WEAPONS)
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
		msgnum = 7;				/* fragments  */
	else if (dam <= 32)
		msgnum = 8;				/* devastate  */
	else if (dam <= 37)
		msgnum = 9;				/* obliterate */
	else if (dam <= 45)
		msgnum = 10;			/* demolish   */
	else if (dam <= 69)
		msgnum = 11;			/* pulverize  */
	else if (dam <= 94)
		msgnum = 12;			/* decimate   */
	else if (dam <= 129)
		msgnum = 13;			/* liquify    */
	else if (dam <= 169)
		msgnum = 14;			/* vaporize   */
	else
		msgnum = 15;			/* annihilate */

	w_type -= TYPE_HIT;

	/* damage message to onlookers */
	if (weap && ((IS_ENERGY_GUN(weap)
				&& w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) || (IS_GUN(weap)
				&& w_type == (TYPE_BLAST - TYPE_HIT))))
		buf =
			replace_string(dam_guns[msgnum].to_room,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
			NULL);
	else if (location >= 0 && !number(0, 2) && (!weap
			|| weap->worn_on == WEAR_WIELD))
		buf =
			replace_string(dam_weapons_location[msgnum].to_room,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
			wear_keywords[wear_translator[location]]);
	else if (weap && (number(0, 2) || weap->worn_on != WEAR_WIELD))
		buf = replace_string(dam_weapons_2[msgnum].to_room,
			attack_hit_text[w_type].singular,
			attack_hit_text[w_type].plural, NULL);
	else
		buf = replace_string(dam_weapons[msgnum].to_room,
			attack_hit_text[w_type].singular,
			attack_hit_text[w_type].plural, NULL);
	act(buf, FALSE, ch, weap, victim, TO_NOTVICT);
	/* damage message to damager */
	if ((msgnum || !PRF_FLAGGED(ch, PRF_GAGMISS)) && ch->desc) {
		if (weap && ((IS_ENERGY_GUN(weap)
					&& w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) || (IS_GUN(weap)
					&& w_type == (TYPE_BLAST - TYPE_HIT))))
			buf =
				replace_string(dam_guns[msgnum].to_char,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		else if (location >= 0 && !number(0, 2)) {
			buf = replace_string(dam_weapons_location[msgnum].to_char,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural,
				wear_keywords[wear_translator[location]]);
		} else if (weap && (number(0, 4) || weap->worn_on != WEAR_WIELD))
			buf = replace_string(dam_weapons_2[msgnum].to_char,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		else
			buf = replace_string(dam_weapons[msgnum].to_char,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		send_to_char(ch, CCYEL(ch, C_NRM));
		act(buf, FALSE, ch, weap, victim, TO_CHAR | TO_SLEEP);
		send_to_char(ch, CCNRM(ch, C_NRM));
	}
	/* damage message to damagee */
	if ((msgnum || !PRF_FLAGGED(victim, PRF_GAGMISS)) && victim->desc) {
		if (weap && ((IS_ENERGY_GUN(weap)
					&& w_type == (TYPE_ENERGY_GUN - TYPE_HIT)) || (IS_GUN(weap)
					&& w_type == (TYPE_BLAST - TYPE_HIT))))
			buf =
				replace_string(dam_guns[msgnum].to_victim,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		else if (location >= 0 && !number(0, 2)) {
			buf = replace_string(dam_weapons_location[msgnum].to_victim,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural,
				wear_keywords[wear_translator[location]]);
		} else if (weap && (number(0, 3) || weap->worn_on != WEAR_WIELD))
			buf = replace_string(dam_weapons_2[msgnum].to_victim,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		else
			buf = replace_string(dam_weapons[msgnum].to_victim,
				attack_hit_text[w_type].singular,
				attack_hit_text[w_type].plural, NULL);
		send_to_char(victim, CCRED(victim, C_NRM));
		act(buf, FALSE, ch, weap, victim, TO_VICT | TO_SLEEP);
		send_to_char(victim, CCNRM(victim, C_NRM));
	}

	if (CHAR_HAS_BLOOD(victim) && BLOODLET(victim, dam, w_type + TYPE_HIT))
		blood_spray(ch, victim, dam, w_type + TYPE_HIT);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int
skill_message(int dam, struct Creature *ch, struct Creature *vict,
	int attacktype)
{
	int i, j, nr;
	struct message_type *msg;
	struct obj_data *weap = cur_weap;

	if (search_nomessage)
		return 1;
	for (i = 0; i < MAX_MESSAGES; i++) {
		if (fight_messages[i].a_type == attacktype) {
			nr = dice(1, fight_messages[i].number_of_attacks);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;

			/*      if ( attacktype == TYPE_SLASH ) {
			   sprintf( buf, "slash [%d]\r\n", nr );
			   } */
			if (attacktype == TYPE_SLASH && nr == 1
				&& !isname("headless", vict->player.name))
				corpse_state = SKILL_BEHEAD;
			else
				corpse_state = 0;

			if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_AMBASSADOR &&
				(!PLR_FLAGGED(vict, PLR_MORTALIZED) || dam == 0)) {
				act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
				act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
				act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			} else if (dam != 0) {
				if (vict->getPosition() == POS_DEAD) {
					if (ch) {
						act(msg->die_msg.room_msg, FALSE, ch, weap, vict,
							TO_NOTVICT);
						if (ch != vict) {
							send_to_char(ch, CCYEL(ch, C_NRM));
							act(msg->die_msg.attacker_msg, FALSE, ch, weap,
								vict, TO_CHAR);
							send_to_char(ch, CCNRM(ch, C_NRM));
						}

					}

					send_to_char(vict, CCRED(vict, C_NRM));
					act(msg->die_msg.victim_msg, FALSE, ch, weap, vict,
						TO_VICT | TO_SLEEP);
					send_to_char(vict, CCNRM(vict, C_NRM));

				} else {
					if (ch) {
						act(msg->hit_msg.room_msg, FALSE, ch, weap, vict,
							TO_NOTVICT);
						if (ch != vict) {
							send_to_char(ch, CCYEL(ch, C_NRM));
							act(msg->hit_msg.attacker_msg, FALSE, ch, weap,
								vict, TO_CHAR);
							send_to_char(ch, CCNRM(ch, C_NRM));
						}
					}

					send_to_char(vict, CCRED(vict, C_NRM));
					act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict,
						TO_VICT | TO_SLEEP);
					send_to_char(vict, CCNRM(vict, C_NRM));

				}
			} else if (ch != vict) {	/* Dam == 0 */
				if (ch && !PRF_FLAGGED(ch, PRF_GAGMISS)) {
					send_to_char(ch, CCYEL(ch, C_NRM));
					act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict,
						TO_CHAR);
					send_to_char(ch, CCNRM(ch, C_NRM));

					act(msg->miss_msg.room_msg, FALSE, ch, weap, vict,
						TO_NOTVICT);
				}

				if (!PRF_FLAGGED(vict, PRF_GAGMISS)) {
					send_to_char(vict, CCRED(vict, C_NRM));
					act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict,
						TO_VICT | TO_SLEEP);
					send_to_char(vict, CCNRM(vict, C_NRM));
				}
			}
			if (BLOODLET(vict, dam, attacktype))
				blood_spray(ch, vict, dam, attacktype);

			return 1;
		}
	}
	return 0;
}

#undef __combat_code__
#undef __combat_messages__
