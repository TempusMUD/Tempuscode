
/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spells.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "flow_room.h"
#include "smokes.h"
#include "screen.h"
#include "vehicle.h"
#include "char_class.h"
#include "fight.h"
#include "materials.h"
#include "tokenizer.h"
#include "tmpstr.h"
#include "house.h"
#include "player_table.h"
#include "events.h"
#include "vendor.h"

extern struct obj_data *object_list;

extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern char *last_command;
extern struct apply_mod_defaults *apmd;
extern const char *instrument_types[];

void weight_change_object(struct obj_data *obj, int weight);
void add_follower(struct Creature *ch, struct Creature *leader);
void zone_weather_change(struct zone_data *zone);
int clan_house_can_enter(struct Creature *ch, struct room_data *room);


/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
	int water;

	void name_to_drinkcon(struct obj_data *obj, int type);
	void name_from_drinkcon(struct obj_data *obj);

	if (ch == NULL || obj == NULL)
		return;
	level = MAX(MIN(level, LVL_GRIMP), 1);

	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			name_from_drinkcon(obj);
			GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
			name_to_drinkcon(obj, LIQ_SLIME);
		} else {
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				GET_OBJ_VAL(obj, 2) = LIQ_WATER;
				GET_OBJ_VAL(obj, 1) += water;
				weight_change_object(obj, water);
				name_from_drinkcon(obj);
				name_to_drinkcon(obj, LIQ_WATER);
				act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
	}
}

bool
teleport_not_ok(Creature *ch, Creature *vict, int level)
{
	// Immortals may not be ported or summoned by mortals
	if (!IS_IMMORT(ch) && IS_IMMORT(vict))
		return true;

	if (vict->trusts(ch))
		return false;
	
	return mag_savingthrow(vict, level, SAVING_SPELL);
}

ASPELL(spell_recall)
{

	struct room_data *load_room = NULL, *targ_room = NULL;
	//Vstone No Affect Code
	if (obj) {
		if (!IS_OBJ_TYPE(obj, ITEM_VSTONE) || !GET_OBJ_VAL(obj, 2))
			send_to_char(ch, NOEFFECT);
		if (IS_OBJ_TYPE(obj, ITEM_VSTONE))
			act("Your divine magic has no effect on $p.", FALSE, ch, obj, 0,
				TO_CHAR);
		return;
	}
	// End Vstone No Affect Code

	if (victim == NULL || IS_NPC(victim))
		return;

	if (ROOM_FLAGGED(victim->in_room, ROOM_NORECALL)) {
		send_to_char(victim, "You fade out for a moment...\r\n"
			"You are suddenly knocked to the floor by a blinding flash!\r\n");
		act("$n is knocked to the floor by a blinding flash of light!", FALSE,
			victim, 0, 0, TO_ROOM);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
		ch->master->in_room == ch->in_room) {
		if (ch == victim) {
			act("You just can't stand the thought of leaving $N behind.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else if (victim == ch->master) {
			act("You really can't stand the though of parting with $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
	}

	if (teleport_not_ok(ch, victim, level)) {
		act("$N resists your attempt to recall $M home!",
			true, ch, 0, victim, TO_CHAR);
		act("You resist $n's attempt to recall you home!",
			true, ch, 0, victim, TO_VICT);
		return;
	}

    load_room = victim->getLoadroom();

	if (!load_room || !victim->in_room) {
		errlog("NULL load_room or victim->in_room in spell_recall.");
		return;
	}

	if (load_room->zone != victim->in_room->zone &&
		(ZONE_FLAGGED(load_room->zone, ZONE_ISOLATED) ||
			ZONE_FLAGGED(victim->in_room->zone, ZONE_ISOLATED))) {
		send_to_char(victim, "You cannot recall to that place from here.\r\n");
		return;
	}

	if (GET_PLANE(load_room) != GET_PLANE(victim->in_room) &&
		((GET_PLANE(load_room) <= MAX_PRIME_PLANE &&
				GET_PLANE(victim->in_room) > MAX_PRIME_PLANE) ||
			(GET_PLANE(load_room) > MAX_PRIME_PLANE &&
				GET_PLANE(victim->in_room) <= MAX_PRIME_PLANE)) &&
		GET_PLANE(victim->in_room) != PLANE_ASTRAL) {
		if (number(0, 120) >
			(CHECK_SKILL(ch, SPELL_WORD_OF_RECALL) + GET_INT(ch)) ||
			((IS_KNIGHT(ch) || IS_CLERIC(ch)) && IS_NEUTRAL(ch))) {
			if ((targ_room = real_room(number(41100, 41863))) != NULL) {
				if (ch != victim)
					act("You send $N hurtling into the Astral Plane!!!",
						FALSE, ch, 0, victim, TO_CHAR);
				act("You hurtle out of control into the Astral Plane!!!",
					FALSE, ch, 0, victim, TO_VICT);
				act("$N is suddenly sucked into an astral void.",
					TRUE, ch, 0, victim, TO_NOTVICT);
				char_from_room(victim);
				char_to_room(victim, targ_room);
				act("$N is suddenly pulled into the Astral Plane!",
					TRUE, ch, 0, victim, TO_NOTVICT);
				look_at_room(victim, victim->in_room, 0);
				victim->in_room->zone->enter_count++;
				return;
			}
		}
	}

	if (victim->in_room->zone != load_room->zone)
		load_room->zone->enter_count++;

	act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, load_room);
	act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, victim->in_room, 0);
}

ASPELL(spell_local_teleport)
{
	struct room_data *to_room = NULL;
	int count = 0;
	if (victim == NULL) {
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
		ch->master->in_room == ch->in_room) {
		if (ch == victim) {
			act("You just can't stand the thought of leaving $N behind.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else if (victim == ch->master) {
			act("You really can't stand the though of parting with $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
	}

	if (GET_LEVEL(victim) > LVL_AMBASSADOR
		&& GET_LEVEL(victim) > GET_LEVEL(ch)) {
		act("$N sneers at you with disgust.\r\n", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N sneers at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_NOTVICT);
		act("You sneer at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_VICT);
		return;
	}
	if (teleport_not_ok(ch, victim, level)) {
		act("$N resists your attempt to teleport $M!",
			true, ch, 0, victim, TO_CHAR);
		act("You resist $n's attempt to teleport you!",
			true, ch, 0, victim, TO_VICT);
		return;
	}

	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
		(IS_NPC(victim) && mag_savingthrow(victim, level, SAVING_SPELL))) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	if (ROOM_FLAGGED(victim->in_room, ROOM_NORECALL)) {
		send_to_char(victim, "You fade out for a moment...\r\n"
			"The magic quickly dissipates!\r\n");
		act("$n fades out for a moment but quickly flickers back into view.",
			FALSE, victim, 0, 0, TO_ROOM);
		/* Removed per Cat's request        
		   "You are caught up in an energy vortex and thrown to the ground!\r\n", victim);
		   act("$n is knocked to the ground by a blinding flash of light!", 
		   FALSE, victim, 0, 0, TO_ROOM);
		   victim->setPosition( POS_RESTING ); */
		return;
	}

	do {
		to_room = real_room(number(ch->in_room->zone->number * 100,
				ch->in_room->zone->top));
		count++;
	} while (count < 1000 &&
		(!to_room ||
			ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NOMAGIC |
				ROOM_NOTEL | ROOM_NORECALL | ROOM_DEATH) ||
			(ROOM_FLAGGED(to_room, ROOM_HOUSE) &&
				!House_can_enter(ch, to_room->number)) ||
			(ROOM_FLAGGED(to_room, ROOM_CLAN_HOUSE) &&
				!clan_house_can_enter(ch, to_room))));

	if (count >= 1000 || !to_room)
		to_room = ch->in_room;

	act("$n disappears in a whirling flash of light.",
		FALSE, victim, 0, 0, TO_ROOM);
	send_to_char(victim, "Your vision slowly fades to blackness...\r\n");
	send_to_char(victim, "A new scene unfolds before you!\r\n\r\n");
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n appears out of a whirling flash.", FALSE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, victim->in_room, 0);
}

ASPELL(spell_teleport)
{
	struct room_data *to_room = NULL;
	int count = 0;
	struct zone_data *zone = NULL;

// Start V-Stone code
	struct room_data *load_room = NULL, *was_in = NULL;
	if (obj) {
		if (!IS_OBJ_TYPE(obj, ITEM_VSTONE) ||
				(GET_OBJ_VAL(obj, 2) != -1
					&& !GET_OBJ_VAL(obj, 2))) {
			send_to_char(ch, NOEFFECT);
			return;
		}

		if (GET_OBJ_VAL(obj, 1) > 0 && GET_OBJ_VAL(obj, 1) != GET_IDNUM(ch)) {
			act("$p hums loudly and zaps you!", FALSE, ch, obj, 0, TO_CHAR);
			act("$p hums loudly and zaps $n!", FALSE, ch, obj, 0, TO_ROOM);
			return;
		}

		if (GET_OBJ_VAL(obj, 0) == 0 ||
			(load_room = real_room(GET_OBJ_VAL(obj, 0))) == NULL) {
			act("$p is not linked with a real location.",
				FALSE, ch, obj, 0, TO_CHAR);
			return;
		}

		if (load_room->zone != ch->in_room->zone &&
			(ZONE_FLAGGED(load_room->zone, ZONE_ISOLATED) ||
				ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED))) {
			send_to_char(ch, "You cannot get to there from here.\r\n");
			return;
		}

		send_to_char(ch, "You slowly fade into nothingness.\r\n");
		act("$p glows brightly as $n fades from view.", FALSE, ch, obj, 0,
			TO_ROOM);

		was_in = ch->in_room;
		char_from_room(ch);
		char_to_room(ch, load_room);
		load_room->zone->enter_count++;

		act("$n slowly fades into view, $p brightly glowing.",
			TRUE, ch, obj, 0, TO_ROOM);

		if (GET_OBJ_VAL(obj, 2) > 0)
			GET_OBJ_VAL(obj, 2)--;

		if (!House_can_enter(ch, ch->in_room->number) ||
			(ROOM_FLAGGED(ch->in_room, ROOM_GODROOM) &&
				GET_LEVEL(ch) < LVL_CREATOR) ||
			ROOM_FLAGGED(ch->in_room,
				ROOM_DEATH | ROOM_NOTEL | ROOM_NOMAGIC | ROOM_NORECALL) ||
			(ZONE_FLAGGED(was_in->zone, ZONE_ISOLATED) &&
				was_in->zone != load_room->zone)) {
			send_to_char(ch, 
				"Your gut wrenches as you are slung violently through spacetime.\r\n");
			act("$n is jerked violently back into the void!", FALSE, ch, 0, 0,
				TO_ROOM);
			char_from_room(ch,false);
			char_to_room(ch, was_in, false);

			act("$n reappears, clenching $s gut in pain.",
				FALSE, ch, 0, 0, TO_ROOM);
			GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 30);
			GET_MANA(ch) = MAX(0, GET_MANA(ch) - 30);
			return;
		}

		look_at_room(ch, ch->in_room, 0);

		if (!GET_OBJ_VAL(obj, 2)) {
			act("$p becomes dim, ceases to glow, and vanishes.",
				FALSE, ch, obj, 0, TO_CHAR);
			extract_obj(obj);
		}

		return;

	}
//End V-Stone code

	if (victim == NULL) {
		return;
	}
	if (ROOM_FLAGGED(victim->in_room, ROOM_NORECALL)) {
		send_to_char(victim, "You fade out for a moment...\r\n"
			"The magic quickly dissipates!\r\n");
		act("$n fades out for a moment but quickly flickers back into view.",
			FALSE, victim, 0, 0, TO_ROOM);
		/* Removed per Cat's request        
		   "You are caught up in an energy vortex and thrown to the ground!\r\n", victim);
		   act("$n is knocked to the ground by a blinding flash of light!", 
		   FALSE, victim, 0, 0, TO_ROOM);
		   victim->setPosition( POS_RESTING ); */
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
		ch->master->in_room == ch->in_room) {
		if (ch == victim) {
			act("You just can't stand the thought of leaving $N behind.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else if (victim == ch->master) {
			act("You really can't stand the though of parting with $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
	}

	if (ch->in_room->zone->number == 400 ||
		GET_PLANE(ch->in_room) == PLANE_DOOM ||
		ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)) {
		call_magic(ch, victim, 0, NULL, SPELL_LOCAL_TELEPORT, GET_LEVEL(ch),
			CAST_SPELL);
		return;
	}

	if (GET_LEVEL(victim) > LVL_AMBASSADOR &&
		GET_LEVEL(victim) > GET_LEVEL(ch)) {
		act("$N sneers at you with disgust.\r\n", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N sneers at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_NOTVICT);
		act("You sneer at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_VICT);
		return;
	}
	if (teleport_not_ok(ch, victim, level)) {
		act("$N resists your attempt to teleport $M!",
			true, ch, 0, victim, TO_CHAR);
		act("You resist $n's attempt to teleport you!",
			true, ch, 0, victim, TO_VICT);
		return;
	}

	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
		(IS_NPC(victim) && mag_savingthrow(victim, level, SAVING_SPELL))) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	do {
		zone = real_zone(number(0, 699));
		count++;
	} while (count < 1000 &&
		(!zone || zone->plane != ch->in_room->zone->plane ||
			zone->time_frame != ch->in_room->zone->time_frame ||
			!IS_APPR(zone) || ZONE_FLAGGED(zone, ZONE_ISOLATED)));

	count = 0;

	if (count >= 1000 || !zone)
		to_room = ch->in_room;
	else
		do {
			to_room = real_room(number(zone->number * 100, zone->top));
			count++;
		} while (count < 1000 &&
			(!to_room ||
				ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NOTEL |
					ROOM_NORECALL | ROOM_NOMAGIC | ROOM_NOPHYSIC |
					ROOM_CLAN_HOUSE | ROOM_DEATH) ||
				!can_travel_sector(ch, SECT_TYPE(to_room), 0) ||
				(to_room->zone->number == 400 &&
					to_room->zone != ch->in_room->zone) ||
				(ROOM_FLAGGED(to_room, ROOM_HOUSE) &&
					!House_can_enter(ch, to_room->number))));

	if (count >= 1000 || !to_room)
		to_room = ch->in_room;

	act("$n slowly fades out of existence and is gone.",
		FALSE, victim, 0, 0, TO_ROOM);
	send_to_char(victim, "Your vision slowly fades to blackness...\r\n");
	send_to_char(victim, "A new scene unfolds before you!\r\n\r\n");
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n slowly fades into existence.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, victim->in_room, 0);
}

ASPELL(spell_astral_spell)
{

	struct room_data *to_room = NULL;
	struct zone_data *zone = NULL;
	int count = 0;

	if (victim == NULL) {
		return;
	}
	if (ROOM_FLAGGED(victim->in_room, ROOM_NORECALL)) {
		send_to_char(victim, "You fade out for a moment...\r\n"
			"The magic quickly dissipates!\r\n");
		act("$n fades out for a moment but quickly flickers back into view.",
			FALSE, victim, 0, 0, TO_ROOM);
		/* Removed per Cat's request        
		   "You are caught up in an energy vortex and thrown to the ground!\r\n", victim);
		   act("$n is knocked to the ground by a blinding flash of light!", 
		   FALSE, victim, 0, 0, TO_ROOM);
		   victim->setPosition( POS_RESTING ); */
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
		ch->master->in_room == ch->in_room) {
		if (ch == victim) {
			act("You just can't stand the thought of leaving $N behind.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else if (victim == ch->master) {
			act("You really can't stand the though of parting with $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
	}


	if (ch->in_room->zone->number == 400 || GET_PLANE(ch->in_room) ==
		PLANE_DOOM || ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED)) {
		call_magic(ch, victim, 0, NULL, SPELL_LOCAL_TELEPORT, GET_LEVEL(ch),
			CAST_SPELL);
		return;
	}

	if (GET_LEVEL(victim) > LVL_AMBASSADOR
		&& GET_LEVEL(victim) > GET_LEVEL(ch)) {
		act("$N sneers at you with disgust.\r\n", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N sneers at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_NOTVICT);
		act("You sneer at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_VICT);
		return;
	}
    
    if (IS_PC(victim) && victim->distrusts(ch)) {
		send_to_char(ch, "They must trust you to be sent to the astral plane.\r\n");
		return;
	}
	
	if (teleport_not_ok(ch, victim, level)) {
		act("$N resists your attempt to send $m into the astral plane!",
			true, ch, 0, victim, TO_CHAR);
		act("You resist $n's attempt to send you into the astral plane!",
			true, ch, 0, victim, TO_VICT);
		return;
	}


	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
		(IS_NPC(victim) && mag_savingthrow(victim, level, SAVING_SPELL))) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	if (!(zone = real_zone(411))) {
		send_to_char(ch, "noastral.\r\n");
		return;
	}

	do {
		to_room = real_room(number(zone->number * 100, zone->top));
		count++;
	} while (count < 1000 &&
		(!to_room || ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NOTEL |
				ROOM_NORECALL | ROOM_HOUSE |
				ROOM_CLAN_HOUSE | ROOM_DEATH) ||
			!can_travel_sector(ch, SECT_TYPE(to_room), 0) ||
			(ROOM_FLAGGED(to_room, ROOM_HOUSE) &&
				!House_can_enter(ch, to_room->number))));

	if (count >= 1000 || !to_room)
		to_room = ch->in_room;

	if (ch->in_room->zone != zone)
		zone->enter_count++;

	act("$n slowly fades out of this plane of existence and is gone.",
		FALSE, victim, 0, 0, TO_ROOM);
	send_to_char(victim, "Your vision slowly fades to blackness...\r\n");
	send_to_char(victim, "A new scene unfolds before you!\r\n\r\n");
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n slowly fades into existence from another plane.",
		TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, victim->in_room, 0);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
	struct room_data *targ_room = NULL;
	int prob = 0;

	if (ch == NULL || victim == NULL || ch == victim)
		return;

	if (ch->in_room == victim->in_room) {
		send_to_char(ch, "Nothing happens.\r\n");
		return;
	}
		
	if (ROOM_FLAGGED(victim->in_room, ROOM_NORECALL)) {
		send_to_char(victim, "You fade out for a moment...\r\n"
			"The magic quickly dissipates!\r\n");
		act("$n fades out for a moment but quickly flickers back into view.",
			FALSE, victim, 0, 0, TO_ROOM);
		send_to_char(ch, SUMMON_FAIL);
		return;
	}

	if (IS_PC(victim) && victim->distrusts(ch)) {
		send_to_char(ch, "They must trust you to be summoned to this place.\r\n");
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)) {
		send_to_char(ch, "This magic cannot penetrate here!\r\n");
		return;
	}
	if (ch->in_room->people.size() >= (unsigned)ch->in_room->max_occupancy) {
		send_to_char(ch, "This room is too crowded to summon anyone!\r\n");
		return;
	}

	if (GET_LEVEL(victim) > MIN(LVL_AMBASSADOR - 1, level + GET_INT(ch))) {
		send_to_char(ch, SUMMON_FAIL);
		return;
	}

	if (teleport_not_ok(ch, victim, level)) {
		act("$N resists your attempt to summon $M!",
			true, ch, 0, victim, TO_CHAR);
		act("You resist $n's attempt to summon you!",
			true, ch, 0, victim, TO_VICT);
		slog("%s failed summoning %s to %s[%d]", GET_NAME(ch),
			GET_NAME(victim), ch->in_room->name, ch->in_room->number);
		return;
	}

	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
		(IS_NPC(victim) && mag_savingthrow(victim, level, SAVING_SPELL))) {
		send_to_char(ch, SUMMON_FAIL);
		return;
	}
	if ((ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
			GET_ZONE(ch->in_room) == 400) ||
		(ROOM_FLAGGED(victim->in_room, ROOM_ARENA) &&
			GET_ZONE(victim->in_room) == 400)) {
		send_to_char(ch, 
			"You can neither summon players out of arenas nor into arenas.\r\n");
		if (ch != victim)
			act("$n has attempted to summon you.", FALSE, ch, 0, victim,
				TO_VICT);
		return;
	}

	if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_CLAN_HOUSE) &&
		!clan_house_can_enter(victim, ch->in_room)) {
		send_to_char(ch, "You cannot summon non-members into the clan house.\r\n");
		act("$n has attempted to summon you to $s clan house!!", FALSE, ch, 0,
			victim, TO_VICT);
		mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(victim)), CMP, true,
			"%s has attempted to summon %s into %s (clan).",
			GET_NAME(ch), GET_NAME(victim), ch->in_room->name);
		return;
	}
	if (ch != victim && ROOM_FLAGGED(victim->in_room, ROOM_CLAN_HOUSE) &&
		!clan_house_can_enter(ch, victim->in_room) &&
		!PLR_FLAGGED(victim, PLR_KILLER)) {
		send_to_char(ch, 
			"You cannot summon clan members from their clan house.\r\n");
		act(tmp_sprintf("$n has attempted to summon you to %s!!\r\n"
			"$e failed because you are in your clan house.\r\n",
			ch->in_room->name), FALSE, ch, 0, victim, TO_VICT);
		mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(victim)), CMP, true,
			"%s has attempted to summon %s from %s (clan).",
			GET_NAME(ch), GET_NAME(victim), victim->in_room->name);

		return;
	}

	if (victim->in_room->zone != ch->in_room->zone &&
		(ZONE_FLAGGED(victim->in_room->zone, ZONE_ISOLATED) ||
			ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED))) {
		act("The place $E exists is completely isolated from this.",
			FALSE, ch, 0, victim, TO_CHAR);
		return;
	}
	if (GET_PLANE(victim->in_room) != GET_PLANE(ch->in_room)) {
		if (GET_PLANE(ch->in_room) == PLANE_DOOM) {
			send_to_char(ch, "You cannot summon characters into VR.\r\n");
			return;
		} else if (GET_PLANE(victim->in_room) == PLANE_DOOM) {
			send_to_char(ch, "You cannot summon characters out of VR.\r\n");
			return;
		} else if (GET_PLANE(victim->in_room) != PLANE_ASTRAL) {
			if (number(0, 120) > (CHECK_SKILL(ch, SPELL_SUMMON) + GET_INT(ch))) {
				if ((targ_room = real_room(number(41100, 41863))) != NULL) {
					act("You fail, sending $N hurtling into the Astral Plane!!!", FALSE, ch, 0, victim, TO_CHAR);
					act("$n attempts to summon you, but something goes wrong!!\r\n" "You are send hurtling into the Astral Plane!!", FALSE, ch, 0, victim, TO_VICT);
					act("$N is suddenly sucked into an astral void.",
						TRUE, ch, 0, victim, TO_NOTVICT);
					char_from_room(victim);
					char_to_room(victim, targ_room);
					act("$N is suddenly pulled into the Astral Plane!",
						TRUE, ch, 0, victim, TO_NOTVICT);
					look_at_room(victim, victim->in_room, 0);
					WAIT_STATE(ch, PULSE_VIOLENCE * 2);
					return;
				}
			}

			if (!IS_REMORT(victim) && !IS_NPC(victim)) {
				slog("%s astral-summoned %s to %d.", GET_NAME(ch),
					GET_NAME(victim), ch->in_room->number);
			}

		}
	}
	CreatureList::iterator it = victim->in_room->people.begin();
	for (; it != victim->in_room->people.end(); ++it)
		if (AFF3_FLAGGED((*it), AFF3_SHROUD_OBSCUREMENT))
			prob +=
				((*it) == victim ? GET_LEVEL((*it)) : (GET_LEVEL((*it)) >> 1));

	if (GET_PLANE(victim->in_room) != GET_PLANE(ch->in_room))
		prob += GET_LEVEL(victim) >> 1;

	if (GET_TIME_FRAME(ch->in_room) != GET_TIME_FRAME(victim->in_room))
		prob += GET_LEVEL(victim) >> 1;

	if (prob > number(10, CHECK_SKILL(ch, SPELL_SUMMON))) {
		send_to_char(ch, "You cannot discern the location of your victim.\r\n");
		return;
	}

	act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

	char_from_room(victim);
	char_to_room(victim, ch->in_room);

	act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
	act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, victim->in_room, 0);
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

	if (!IS_NPC(victim)) {
		mudlog(LVL_AMBASSADOR, BRF, true,
			"%s has%s summoned %s to %s (%d)",
			GET_NAME(ch),
			(victim->distrusts(ch)) ? " forcibly":"",
			GET_NAME(victim), ch->in_room->name, ch->in_room->number);
	}
}

#define MAX_LOCATE_TERMS	4

ASPELL(spell_locate_object)
{
	struct obj_data *i;
	extern char locate_buf[];
	int j, k;
	char *which_str;
	struct room_data *rm = NULL;
	char buf3[MAX_STRING_LENGTH];
	void *ptr;
	char terms[MAX_LOCATE_TERMS][MAX_INPUT_LENGTH];
	int term_idx, term_count = 0;
	int found;

	j = level >> 1;
	k = level >> 2;

	*buf = *buf2 = '\0';

	// Grab the search terms
	Tokenizer tokens(locate_buf, ' ');
	while (term_count <= MAX_LOCATE_TERMS && tokens.next(terms[term_count])) {
		term_count++;
	}

	//Check to see if the character can be that precise
	if (ch) {
		int extracost;

		if (term_count > MAX(1, (ch->getLevelBonus(SPELL_LOCATE_OBJECT) / 25))) {
			send_to_char(ch, "You are not powerful enough to be so precise.\r\n");
			return;
		}

		extracost = (term_count - 1) * mag_manacost(ch, SPELL_LOCATE_OBJECT);
		if (extracost > GET_MANA(ch) && ((GET_LEVEL(ch) < LVL_AMBASSADOR) &&
				!PLR_FLAGGED(ch, PLR_MORTALIZED))) {
			send_to_char(ch, "You haven't the energy to be that precise!\r\n");
			return;
		}

		GET_MANA(ch) = GET_MANA(ch) - extracost;
	}

	for (i = object_list; i && (j > 0 || k > 0); i = i->next) {
		found = 1;
		for (term_idx = 0; term_idx < term_count && found; term_idx++)
			if (!isname(terms[term_idx], i->aliases))
				found = 0;

		if (!found)
			continue;

		if (!can_see_object(ch, i))
			continue;

		if (isname("imm", i->aliases) || IS_OBJ_TYPE(i, ITEM_SCRIPT)
			|| !OBJ_APPROVED(i)) {
			continue;
		}

		rm = where_obj(i);

		if (!rm) {
			errlog("%s is nowhere? Moving to The Void.", i->name);
			rm = real_room(0);
			SET_BIT(GET_OBJ_EXTRA2(i), ITEM2_BROKEN);
			SET_BIT(GET_OBJ_EXTRA3(i), ITEM3_HUNTED);
			obj_to_room(i, rm);
			continue;
		}
		// make sure it exists in a nearby plane/time
		if (rm->zone->plane != ch->in_room->zone->plane ||
			(rm->zone->time_frame != ch->in_room->zone->time_frame &&
				ch->in_room->zone->time_frame != TIME_TIMELESS &&
				rm->zone->time_frame != TIME_TIMELESS)) {
			if (GET_LEVEL(ch) < LVL_IMMORT)
				continue;
		}

		if (i->in_obj)
			rm = where_obj(i);
		else
			rm = i->in_room;

		if (rm && ROOM_FLAGGED(rm, ROOM_HOUSE)) {
			which_str = buf2;
			if (k-- <= 0)
				continue;
		} else {
			which_str = buf;
			if (j-- <= 0)
				continue;
		}

		if (IS_OBJ_STAT2(i, ITEM2_NOLOCATE))
			sprintf(buf3, "The location of %s is indeterminable.\r\n",
				i->name);
		else if (IS_OBJ_STAT2(i, ITEM2_HIDDEN))
			sprintf(buf3, "%s is hidden somewhere.\r\n", i->name);
		else if (i->carried_by)
			sprintf(buf3, "%s is being carried by %s.\r\n",
				i->name, PERS(i->carried_by, ch));
		else if (i->in_room != NULL && !ROOM_FLAGGED(i->in_room, ROOM_HOUSE)) {
			sprintf(buf3, "%s is in %s.\r\n", i->name,
				i->in_room->name);
		} else if (i->in_obj)
			sprintf(buf3, "%s is in %s.\r\n", i->name,
				i->in_obj->name);
		else if (i->worn_by)
			sprintf(buf3, "%s is being worn by %s.\r\n",
				i->name, PERS(i->worn_by, ch));
		else
			sprintf(buf3, "%s's location is uncertain.\r\n",
				i->name);

		// this ptr crap is to quiet a compiler warning!
		ptr = (void *)CAP(buf3);

		if (strlen(which_str) + strlen(buf3) > MAX_STRING_LENGTH - 64)
			break;

		strcat(which_str, buf3);
//        j--;
	}

	if (j == level >> 1 && k == level >> 2)
		send_to_char(ch, "You sense nothing.\r\n");

	else {
		if (*buf2 && strlen(buf) + strlen(buf2) < MAX_STRING_LENGTH - 1) {
			strcat(buf, "-----------\r\n");
			strcat(buf, buf2);
		}

		page_string(ch->desc, buf);
	}

}



ASPELL(spell_charm)
{
	struct affected_type af;
	int percent;

	if (victim == NULL || ch == NULL)
		return;

	if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
		!PRF_FLAGGED(ch, PRF_NOHASSLE)) {
		send_to_char(ch, "You cannot charm people in arena rooms.\r\n");
		return;
	}

    af.owner = ch->getIdNum();
	if (victim == ch)
		send_to_char(ch, "You like yourself even better!\r\n");
	else if (!IS_NPC(victim) && !(victim->desc))
		send_to_char(ch, "You cannot charm linkless players!\r\n");
    else if (ch->checkReputations(victim))
        return;
	else if (!ok_damage_vendor(ch, victim)) {
		act("$N falls down laughing at you!", FALSE, ch, 0, victim, TO_CHAR);
		act("$N peers deeply into your eyes...", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N falls down laughing at $n!", FALSE, ch, 0, victim, TO_ROOM);
		act("$N peers deeply into the eyes of $n...",
			FALSE, ch, 0, victim, TO_ROOM);
		if (ch->master)
			stop_follower(ch);
		add_follower(ch, victim);
		af.type = SPELL_CHARM;
		af.is_instant = 0;
		if (GET_INT(victim))
			af.duration = 6 * 18 / GET_INT(victim);
		else
			af.duration = 6 * 18;
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
		af.aff_index = 0;
		affect_to_char(ch, &af);
		act("Isn't $n just such a great friend?", FALSE, victim, 0, ch,
			TO_VICT);
		if (IS_NPC(ch)) {
			REMOVE_BIT(MOB_FLAGS(ch), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
		}
	} else if (IS_AFFECTED(victim, AFF_SANCTUARY))
		send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
	else if (MOB_FLAGGED(victim, MOB_NOCHARM) && GET_LEVEL(ch) < LVL_IMPL)
		send_to_char(ch, "Your victim resists!\r\n");
	else if (IS_AFFECTED(ch, AFF_CHARM))
		send_to_char(ch, "You can't have any followers of your own!\r\n");
	else if (IS_AFFECTED(victim, AFF_CHARM) ||
		(level + number(0, MAX(0, GET_CHA(ch)))) <
		(GET_LEVEL(victim) + number(0, MAX(GET_CHA(victim), 1)))) {
		send_to_char(ch, "You fail.\r\n");
		if (MOB_FLAGGED(victim, MOB_MEMORY))
			remember(victim, ch);
	} else if (IS_ANIMAL(victim))
		send_to_char(ch, 
			"You need a different sort of magic to charm animals.\r\n");
	else if (circle_follow(victim, ch))
		send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
	else if (mag_savingthrow(victim, level, SAVING_PARA) ||
			(GET_INT(victim) + number(0, GET_LEVEL(victim) +
				(AFF_FLAGGED(victim, AFF_CONFIDENCE) ? 20 : 0))) >
				GET_LEVEL(ch) + 2 * GET_CHA(ch) ||
				!can_charm_more(ch)) {
		send_to_char(ch, "Your victim resists!\r\n");
		if (MOB_FLAGGED(victim, MOB_MEMORY))
			remember(victim, ch);
	} else if ((IS_ELF(victim) || IS_DROW(victim)) &&
		(percent = (40 + GET_LEVEL(victim))) > number(1, 101) &&
		GET_LEVEL(ch) < LVL_CREATOR)
		send_to_char(ch, "Your victim is Elven, and resists!\r\n");
	else if (IS_UNDEAD(victim) && GET_LEVEL(ch) < LVL_CREATOR)
		send_to_char(ch, "You cannot charm the undead!\r\n");
	else if (GET_LEVEL(victim) > LVL_AMBASSADOR &&
		GET_LEVEL(victim) > GET_LEVEL(ch)) {
		act("$N sneers at you with disgust.\r\n", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N sneers at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_NOTVICT);
		act("You sneer at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_VICT);
		return;
	} else {
		if (victim->master)
			stop_follower(victim);

		add_follower(victim, ch);

		af.type = SPELL_CHARM;
		af.is_instant = 0;

		if (GET_INT(victim))
			af.duration = 24 * 18 / GET_INT(victim);
		else
			af.duration = 24 * 18;

		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
		af.aff_index = 0;
		affect_to_char(victim, &af);

		act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim,
			TO_VICT);
		if (IS_NPC(victim)) {
			REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
		}
	}
}


ASPELL(spell_charm_animal)
{
	struct affected_type af;
	int percent;

	if (victim == NULL || ch == NULL)
		return;

    af.owner = ch->getIdNum();
	if (victim == ch)
		send_to_char(ch, "You like yourself even better!\r\n");
	else if (IS_AFFECTED(victim, AFF_SANCTUARY))
		send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
	else if (MOB_FLAGGED(victim, MOB_NOCHARM))
		send_to_char(ch, "Your victim resists!\r\n");
	else if (IS_AFFECTED(ch, AFF_CHARM))
		send_to_char(ch, "You can't have any followers of your own!\r\n");
	else if (IS_AFFECTED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
		send_to_char(ch, "You fail.\r\n");
	else if (circle_follow(victim, ch))
		send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
	else if (mag_savingthrow(victim, level, SAVING_PARA) ||
			!can_charm_more(ch))
		send_to_char(ch, "Your victim resists!\r\n");
	else if ((!IS_NPC(victim)) && (IS_ELF(victim) || IS_DROW(victim))) {
		percent = (40 + GET_LEVEL(victim));
		if (percent > number(1, 101))
			send_to_char(ch, "Your victim is Elven, and resists!\r\n");
	} else {
		if (victim->master)
			stop_follower(victim);

		add_follower(victim, ch);

		af.type = SPELL_CHARM;
		af.is_instant = 0;

		if (GET_INT(victim))
			af.duration = 48 * 18 / GET_INT(victim);
		else
			af.duration = 48 * 18;

		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
		af.aff_index = 0;
		affect_to_char(victim, &af);

		act("Isn't $n just such a nice friend?", FALSE, ch, 0, victim,
			TO_VICT);
		if (IS_NPC(victim)) {
			REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
		}
	}
}


ASPELL(spell_identify)
{
	int i;
	int found;

	struct time_info_data age(struct Creature *ch);


	if (obj) {
		send_to_char(ch, "You feel informed:\r\n");
		send_to_char(ch, "Object '%s', Item type: ", obj->name);
		buf[0] = '\0';
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf);

		send_to_char(ch,
			"%s\r\nItem will give you following abilities:  ", buf);
		strcpy(buf, "");
		if (obj->obj_flags.bitvector[0])
			sprintbit(obj->obj_flags.bitvector[0], affected_bits, buf);
		if (obj->obj_flags.bitvector[1])
			sprintbit(obj->obj_flags.bitvector[1], affected2_bits, buf);
		if (obj->obj_flags.bitvector[2])
			sprintbit(obj->obj_flags.bitvector[2], affected3_bits, buf);
		send_to_char(ch, "%s\r\n", buf);

		send_to_char(ch, "Item is: ");
		sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
		strcat(buf, "\r\n");
		send_to_char(ch, "%s", buf);

		send_to_char(ch, "Item is also: ");
		sprintbit(GET_OBJ_EXTRA2(obj), extra2_bits, buf);
		strcat(buf, "\r\n");
		send_to_char(ch, "%s", buf);

		send_to_char(ch, "Item is also: ");
		sprintbit(GET_OBJ_EXTRA3(obj), extra3_bits, buf);
		strcat(buf, "\r\n");
		send_to_char(ch, "%s", buf);

		send_to_char(ch, "Weight: %d, Value: %d, Rent: %d\r\n",
			obj->getWeight(), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
		send_to_char(ch, "Item material is %s.\r\n",
			material_names[GET_OBJ_MATERIAL(obj)]);

		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_SCROLL:
		case ITEM_POTION:
		case ITEM_PILL:
		case ITEM_SYRINGE:
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);

			if (GET_OBJ_VAL(obj, 1) >= 1)
				send_to_char(ch, " %s", spell_to_str(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)
				send_to_char(ch, " %s", spell_to_str(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)
				send_to_char(ch, " %s", spell_to_str(GET_OBJ_VAL(obj, 3)));
			send_to_char(ch, "\r\n");
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);
			send_to_char(ch, "%s at level %d\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)), GET_OBJ_VAL(obj, 0));
			send_to_char(ch, "It has %d maximum charge%s and %d remaining.\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
				GET_OBJ_VAL(obj, 2));
			break;
		case ITEM_WEAPON:
			send_to_char(ch, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
				GET_OBJ_VAL(obj, 2));
			send_to_char(ch, " for an average per-round damage of %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj,
						1)));
			if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
				send_to_char(ch, "This weapon casts: %s\r\n",
					spell_to_str(GET_OBJ_VAL(obj, 0)));
			break;
		case ITEM_ARMOR:
			send_to_char(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
			break;
		case ITEM_HOLY_SYMB:
			send_to_char(ch,
				"Alignment: %s, Class: %s, Min Level: %d, Max Level: %d\r\n",
				alignments[(int)GET_OBJ_VAL(obj, 0)],
				char_class_abbrevs[(int)GET_OBJ_VAL(obj, 1)], GET_OBJ_VAL(obj,
					2), GET_OBJ_VAL(obj, 3));
			break;
		case ITEM_TOBACCO:
			send_to_char(ch, "Smoke type is: %s%s%s\r\n",
				CCYEL(ch, C_NRM), smoke_types[SMOKE_TYPE(obj)],
				CCNRM(ch, C_NRM));
			break;
		case ITEM_CONTAINER:
			send_to_char(ch, "This container holds a maximum of %d pounds.\r\n",
				GET_OBJ_VAL(obj, 0));
			break;

		case ITEM_TOOL:
			send_to_char(ch, "Tool works with: %s, modifier: %d\r\n",
				spell_to_str(TOOL_SKILL(obj)), TOOL_MOD(obj));
			break;
        case ITEM_INSTRUMENT:
            send_to_char(ch, "Instrument type is: %s%s%s\r\n",
                         CCCYN(ch, C_NRM), (GET_OBJ_VAL(obj, 0) < 2) ? 
                         instrument_types[GET_OBJ_VAL(obj, 0)] : "UNDEFINED",
                         CCNRM(ch, C_NRM));
            break;
		}
		found = FALSE;
		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if ((obj->affected[i].location != APPLY_NONE) &&
				(obj->affected[i].modifier != 0)) {
				if (!found) {
					send_to_char(ch, "Can affect you as :\r\n");
					found = TRUE;
				}
				sprinttype(obj->affected[i].location, apply_types, buf2);
				send_to_char(ch, "   Affects: %s By %d\r\n", buf2,
					obj->affected[i].modifier);
			}
		}
		if (GET_OBJ_SIGIL_IDNUM(obj)) {
			const char *name;

			name = playerIndex.getName(GET_OBJ_SIGIL_IDNUM(obj));

			if (name)
				send_to_char(ch,
					"You detect a warding sigil bearing the mark of %s.\r\n",
					name);
			else
				send_to_char(ch,
					"You detect an warding sigil with an unfamiliar marking.\r\n");
		}

	} else if (victim) {		/* victim */
		if (victim->distrusts(ch) &&
				mag_savingthrow(victim, level, SAVING_SPELL)) {
			act("$N resists your spell!", FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));
		if (!IS_NPC(victim)) {
			send_to_char(ch,
				"%s is %d years, %d months, %d days and %d hours old.\r\n",
				GET_NAME(victim), GET_AGE(victim), age(victim).month,
				age(victim).day, age(victim).hours);
		}
		send_to_char(ch, "Race: %s, Class: %s, Alignment: %d.\r\n",
			player_race[(int)MIN(NUM_RACES, GET_RACE(victim))],
			pc_char_class_types[(int)MIN(TOP_CLASS, GET_CLASS(victim))],
			GET_ALIGNMENT(victim));
		send_to_char(ch, "Height %d cm, Weight %d pounds\r\n",
			GET_HEIGHT(victim), GET_WEIGHT(victim));
		send_to_char(ch, "Level: %d, Hits: %d, Mana: %d\r\n",
			GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
		send_to_char(ch, "AC: %d, Thac0: %d, Hitroll: %d (%d), Damroll: %d\r\n",
			GET_AC(victim), (int)MIN(THACO(GET_CLASS(victim),
					GET_LEVEL(victim)), THACO(GET_REMORT_CLASS(victim),
					GET_LEVEL(victim))), GET_HITROLL(victim),
			GET_REAL_HITROLL(victim), GET_DAMROLL(victim));
		send_to_char(ch,
			"Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
			GET_STR(victim), GET_ADD(victim), GET_INT(victim),
			GET_WIS(victim), GET_DEX(victim), GET_CON(victim),
			GET_CHA(victim));

	}
}

ASPELL(spell_minor_identify)
{
	int i;
	int found;

	struct time_info_data age(struct Creature *ch);

	if (obj) {
		send_to_char(ch, "You feel a bit informed:\r\n");
		send_to_char(ch, "Object '%s', Item type: ", obj->name);
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
		send_to_char(ch, "%s\r\n", buf2);

		send_to_char(ch, "Item will give you following abilities:  ");
		if (obj->obj_flags.bitvector[0])
			sprintbit(obj->obj_flags.bitvector[0], affected_bits, buf);
		if (obj->obj_flags.bitvector[1])
			sprintbit(obj->obj_flags.bitvector[1], affected2_bits, buf);
		if (obj->obj_flags.bitvector[2])
			sprintbit(obj->obj_flags.bitvector[2], affected3_bits, buf);
		send_to_char(ch, "%s\r\n", buf);

		send_to_char(ch, "Item is: ");
		sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
		sprintbit(GET_OBJ_EXTRA2(obj), extra2_bits, buf);
		send_to_char(ch, "%s\r\n", buf);

		send_to_char(ch, "Weight: %d, Value: %d, Rent: %d\r\n",
			obj->getWeight(), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));

		send_to_char(ch, "Item material is %s.\r\n",
			material_names[GET_OBJ_MATERIAL(obj)]);

		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_SCROLL:
		case ITEM_POTION:
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);

			if (GET_OBJ_VAL(obj, 1) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)
				send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			send_to_char(ch, "This %s casts: ",
				item_types[(int)GET_OBJ_TYPE(obj)]);
			send_to_char(ch, "%s\r\n", spell_to_str(GET_OBJ_VAL(obj, 3)));
			send_to_char(ch, "It has %d maximum charge%s and %d remaining.\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
				GET_OBJ_VAL(obj, 2));
			break;
		case ITEM_WEAPON:
			send_to_char(ch, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
				GET_OBJ_VAL(obj, 2));
			send_to_char(ch, " for an average per-round damage of %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj,
						1)));
			if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
				send_to_char(ch, "This weapon casts an offensive spell.\r\n");
			break;
		case ITEM_ARMOR:
			send_to_char(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
			break;
		case ITEM_CONTAINER:
			send_to_char(ch, "This container holds a maximum of %d pounds.\r\n",
				GET_OBJ_VAL(obj, 0));
			break;
        case ITEM_INSTRUMENT:
            send_to_char(ch, "Instrument type is: %s%s%s\r\n",
                         CCCYN(ch, C_NRM), 
                         (GET_OBJ_VAL(obj, 0) < 2) ? instrument_types[GET_OBJ_VAL(obj, 0)] :
                         "UNDEFINED",
                         CCNRM(ch, C_NRM));
            break;
		}
		found = FALSE;
		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if ((obj->affected[i].location != APPLY_NONE) &&
				(obj->affected[i].modifier != 0)) {
				if (!found) {
					send_to_char(ch, "Can affect you as :\r\n");
					found = TRUE;
				}
				sprinttype(obj->affected[i].location, apply_types, buf2);
				send_to_char(ch, "   Affects: %s\r\n", buf2);
			}
		}
	} else if (victim) {		/* victim */
		if (victim->distrusts(ch) &&
				mag_savingthrow(victim, level, SAVING_SPELL)) {
			act("$N resists your spell!", FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));
		if (!IS_NPC(victim)) {
			send_to_char(ch,
				"%s is %d years, %d months, %d days and %d hours old.\r\n",
				GET_NAME(victim), age(victim).year, age(victim).month,
				age(victim).day, age(victim).hours);
		}
		send_to_char(ch, "Height %d cm, Weight %d pounds\r\n",
			GET_HEIGHT(victim), GET_WEIGHT(victim));
		send_to_char(ch, "Level: %d, Hits: %d, Mana: %d\r\n",
			GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
		send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n",
			GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
		send_to_char(ch,
			"Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
			GET_STR(victim), GET_ADD(victim), GET_INT(victim),
			GET_WIS(victim), GET_DEX(victim), GET_CON(victim),
			GET_CHA(victim));

	}
}

ASPELL(spell_enchant_weapon)
{
	int i, max;

	if (ch == NULL || obj == NULL)
		return;

	if (((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC_NODISPEL) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED)) ||
		GET_LEVEL(ch) > LVL_CREATOR) {

		for (i = MAX_OBJ_AFFECT - 1; i >= 0; i--) {
			if (obj->affected[i].location == APPLY_HITROLL ||
				obj->affected[i].location == APPLY_DAMROLL) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			} else if (i < MAX_OBJ_AFFECT - 2 && obj->affected[i].location) {
				obj->affected[i + 2].location = obj->affected[i].location;
				obj->affected[i + 2].modifier = obj->affected[i].modifier;
			}
		}

		max = GET_INT(ch) >> 2;
		max += number(GET_LEVEL(ch) >> 3, GET_LEVEL(ch) >> 2);
		max += CHECK_SKILL(ch, SPELL_ENCHANT_WEAPON) >> 3;
		max = MIN(4, max >> 3);
		max = MAX(1, max);

		obj->affected[0].location = APPLY_HITROLL;
		obj->affected[0].modifier = MAX(1, number(max >> 1, max)) +
			(GET_LEVEL(ch) >= 50) + (GET_LEVEL(ch) >= 56) + (GET_LEVEL(ch) >=
			60)
			+ (GET_LEVEL(ch) >= 66);
		obj->affected[1].location = APPLY_DAMROLL;
		obj->affected[1].modifier = MAX(1, number(max >> 1, max)) +
			(GET_LEVEL(ch) >= 50) + (GET_LEVEL(ch) >= 56) + (GET_LEVEL(ch) >=
			60)
			+ (GET_LEVEL(ch) >= 66);


		if (IS_GOOD(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
			act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (IS_EVIL(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
			act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
			act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
		}
		if (level > number(30, 50))
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

		gain_skill_prof(ch, SPELL_ENCHANT_WEAPON);

		if (GET_LEVEL(ch) >= LVL_AMBASSADOR && !isname("imm", obj->aliases)) {
			sprintf(buf, " imm %senchant", GET_NAME(ch));
			strcpy(buf2, obj->aliases);
			strcat(buf2, buf);
			obj->aliases = str_dup(buf2);
			mudlog(GET_LEVEL(ch), CMP, true,
				"ENCHANT: %s by %s.", obj->name,
				GET_NAME(ch));
		}
	}
}


ASPELL(spell_detect_poison)
{
	if (victim) {
		if (victim == ch) {
			if (IS_AFFECTED(victim, AFF_POISON))
				send_to_char(ch, "You can sense poison in your blood.\r\n");
			else
				send_to_char(ch, "You feel healthy.\r\n");
		} else {
			if (IS_AFFECTED(victim, AFF_POISON))
				act("You sense that $E is poisoned.", FALSE, ch, 0, victim,
					TO_CHAR);
			else
				act("You sense that $E is healthy.", FALSE, ch, 0, victim,
					TO_CHAR);
		}
	}

	if (obj) {
		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
		case ITEM_FOOD:
			if (GET_OBJ_VAL(obj, 3))
				act("You sense that $p has been contaminated.", FALSE, ch, obj,
					0, TO_CHAR);
			else
				act("You sense that $p is safe for consumption.", FALSE, ch,
					obj, 0, TO_CHAR);
			break;
		default:
			send_to_char(ch, "You sense that it should not be consumed.\r\n");
		}
	}
}

ASPELL(spell_enchant_armor)
{
	int i;

	if (ch == NULL || obj == NULL)
		return;

	if (((GET_OBJ_TYPE(obj) == ITEM_ARMOR) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC_NODISPEL) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS) &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED)) ||
		GET_LEVEL(ch) > LVL_GRGOD) {

		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if (obj->affected[i].location == APPLY_AC ||
				obj->affected[i].location == APPLY_SAVING_BREATH ||
				obj->affected[i].location == APPLY_SAVING_SPELL ||
				obj->affected[i].location == APPLY_SAVING_PARA) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			}
		}
        int saveMod = -(1 + (level >= 20) + (level >= 35) + 
                            (level >= 45) + (level > 69) + 
                            (level > 75) );
                            
		obj->affected[0].location = APPLY_AC;
		obj->affected[0].modifier = -((level >> 3) + 1);

		obj->affected[1].location = APPLY_SAVING_BREATH;
		obj->affected[1].modifier = saveMod;

		if (IS_IMPLANT(obj)) {
			obj->affected[0].modifier >>= 1;
			obj->affected[1].modifier >>= 1;
		}

		if (!IS_IMPLANT(obj)) {
			obj->affected[2].location = APPLY_SAVING_SPELL;
			obj->affected[2].modifier = saveMod;

			obj->affected[3].location = APPLY_SAVING_PARA;
			obj->affected[3].modifier = saveMod;
		}

		if (IS_GOOD(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
			act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (IS_EVIL(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
			act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
			act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
		}
		if (level > number(30, 50))
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

		gain_skill_prof(ch, SPELL_ENCHANT_ARMOR);

		if (GET_LEVEL(ch) >= LVL_AMBASSADOR && !isname("imm", obj->aliases)) {
			sprintf(buf, " imm %senchant", GET_NAME(ch));
			strcpy(buf2, obj->aliases);
			strcat(buf2, buf);
			obj->aliases = str_dup(buf2);
			mudlog(GET_LEVEL(ch), CMP, true,
				"ENCHANT: %s by %s.", obj->name,
				GET_NAME(ch));
		}
	} else
		send_to_char(ch, NOEFFECT);
}

ASPELL(spell_greater_enchant)
{
	int i, max;

	if (ch == NULL || obj == NULL)
		return;

	if (IS_IMPLANT(obj)) {
		send_to_char(ch, "You cannot greater enchant that.\r\n");
		return;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON &&
		((!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC) &&
				!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC_NODISPEL) &&
				!IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS) &&
				!IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED)) ||
			GET_LEVEL(ch) > LVL_GRGOD)) {

		for (i = MAX_OBJ_AFFECT - 1; i >= 0; i--) {
			if (obj->affected[i].location == APPLY_HITROLL ||
				obj->affected[i].location == APPLY_DAMROLL) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			} else if (i < MAX_OBJ_AFFECT - 2 && obj->affected[i].location) {
				obj->affected[i + 2].location = obj->affected[i].location;
				obj->affected[i + 2].modifier = obj->affected[i].modifier;
			}
		}

		max = GET_INT(ch) >> 2;
		max += number(GET_LEVEL(ch) >> 3, GET_LEVEL(ch) >> 2);
		max += CHECK_SKILL(ch, SPELL_ENCHANT_WEAPON) >> 3;
		max = MIN(4, max >> 3);
		max = MAX(1, max);

		obj->affected[0].location = APPLY_HITROLL;
		obj->affected[0].modifier = MAX(2, number(max >> 1, max)) +
                                    (GET_LEVEL(ch) >= 50) + (GET_LEVEL(ch) >= 56) + 
                                    (GET_LEVEL(ch) >= 60) + (GET_LEVEL(ch) >= 67);
		obj->affected[1].location = APPLY_DAMROLL;
		obj->affected[1].modifier = MAX(2, number(max >> 1, max)) +
                                    (GET_LEVEL(ch) >= 50) + (GET_LEVEL(ch) >= 56) + 
                                    (GET_LEVEL(ch) >= 60) + (GET_LEVEL(ch) >= 67);

		if (IS_GOOD(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
			act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (IS_EVIL(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
			act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
			act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
	if ((GET_OBJ_TYPE(obj) == ITEM_ARMOR) &&
		(!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)
			|| GET_LEVEL(ch) > LVL_CREATOR)) {

		for (i = 0; i < MAX_OBJ_AFFECT - 1; i++) {
			if (obj->affected[i].location == APPLY_AC ||
				obj->affected[i].location == APPLY_SAVING_BREATH ||
				obj->affected[i].location == APPLY_SAVING_SPELL ||
				obj->affected[i].location == APPLY_SAVING_PARA) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			}
		}

        int saveMod  = -( 2 + (level >= 40) + (level >= 50) );
        int armorMod = -( level / 7 );

		obj->affected[0].location = APPLY_AC;
		obj->affected[0].modifier = armorMod;

		obj->affected[1].location = APPLY_SAVING_PARA;
		obj->affected[1].modifier = saveMod;

		obj->affected[2].location = APPLY_SAVING_BREATH;
		obj->affected[2].modifier = saveMod;

		obj->affected[3].location = APPLY_SAVING_SPELL;
		obj->affected[3].modifier = saveMod;

		if (IS_GOOD(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
			act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (IS_EVIL(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
			act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		} else {
			act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	if (level > number(35, 52))
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);

	gain_skill_prof(ch, SPELL_GREATER_ENCHANT);

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && !isname("imm", obj->aliases)) {
		sprintf(buf, " imm %senchant", GET_NAME(ch));
		strcpy(buf2, obj->aliases);
		strcat(buf2, buf);
		obj->aliases = str_dup(buf2);
		mudlog(GET_LEVEL(ch), CMP, true,
			"ENCHANT: %s by %s.", obj->name,
			GET_NAME(ch));
	}
}

ASPELL(spell_magical_vestment)
{
	int i;
	if (ch == NULL || obj == NULL)
		return;

	if (IS_NEUTRAL(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, 
			"You cannot cast this spell, you worthless excuse for a cleric!\n");
		return;
	}

	if (!IS_CLOTH_TYPE(obj) && !IS_LEATHER_TYPE(obj)) {

		if (IS_EVIL(ch) && !IS_FLESH_TYPE(obj)) {
			send_to_char(ch, 
				"The target of this spell must be made from a type of cloth, leather, or flesh.\n");
			return;
		} else if (IS_GOOD(ch)) {
			send_to_char(ch, 
				"The target of this spell must be made from a type of cloth or leather.\n");
			return;
		}
	}

	if (CAN_WEAR(obj, ITEM_WEAR_FINGER) ||
		CAN_WEAR(obj, ITEM_WEAR_SHIELD) ||
		CAN_WEAR(obj, ITEM_WEAR_HOLD) ||
		CAN_WEAR(obj, ITEM_WEAR_ASS) ||
		CAN_WEAR(obj, ITEM_WEAR_EAR) || CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
		act("$p is not a suitable vestment due to the places where it can be worn.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
		if (GET_OBJ_VAL(obj, 0) > (GET_LEVEL(ch) / 10 + 2)) {
			act("$p is not a suitable vestment because it is already a formidable armor.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
	} else if (GET_OBJ_TYPE(obj) != ITEM_WORN) {
		act("$p is not a typical worn item, and cannot be the target of this spell.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	// silently remove the armor, save_para, save_spell affects

	for (i = 0; i < MAX_OBJ_AFFECT; i++) {
		if (obj->affected[i].location == APPLY_AC ||
			obj->affected[i].location == APPLY_SAVING_PARA ||
			obj->affected[i].location == APPLY_SAVING_SPELL) {
			obj->affected[i].location = APPLY_NONE;
			obj->affected[i].modifier = 0;
		}
	}

	float multiplier = 1;

	if (IS_OBJ_STAT(obj, ITEM_DAMNED)) {
		if (IS_EVIL(ch))
			multiplier = 1.5;
		else {
			act("$p is an evil item.  You cannot endow it with your magic.",
				FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
	} else if (IS_OBJ_STAT(obj, ITEM_BLESS)) {
		if (IS_GOOD(ch))
			multiplier = 1.5;
		else {
			act("$p is a good item.  Burn it!!!", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
	}

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	if (IS_EVIL(ch)) {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DAMNED);
		act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
	} else {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
		act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
	}

	obj->affected[0].location = APPLY_AC;
	obj->affected[0].modifier =
		-(char)(2 + ((int)(level * multiplier + GET_WIS(ch)) >> 4));

	obj->affected[1].location = APPLY_SAVING_PARA;
	obj->affected[1].modifier = -(char)(1 + ((level * multiplier) / 20));

	obj->affected[2].location = APPLY_SAVING_SPELL;
	obj->affected[2].modifier = -(char)(1 + ((level * multiplier) / 20));

	if (level > number(35, 53))
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

	gain_skill_prof(ch, SPELL_MAGICAL_VESTMENT);
}

ASPELL(spell_clairvoyance)
{

	int prob = 0;
	if (GET_LEVEL(victim) >= LVL_AMBASSADOR &&
		GET_LEVEL(ch) < GET_LEVEL(victim)) {
		send_to_char(ch, "That is impossible.\r\n");
		act("$n has just attempted to get clairvoyant on your ass.",
			FALSE, ch, 0, victim, TO_VICT);
		return;
	}
	if (victim->in_room == NULL || ROOM_FLAGGED(victim->in_room, ROOM_NOTEL)
		|| ROOM_FLAGGED(victim->in_room, ROOM_GODROOM)) {
		act("$N seems to be outside the known universe at the moment.",
			FALSE, ch, 0, victim, TO_CHAR);
		return;
	}
	CreatureList::iterator it = victim->in_room->people.begin();
	for (; it != victim->in_room->people.end(); ++it)
		if (AFF3_FLAGGED((*it), AFF3_SHROUD_OBSCUREMENT))
			prob += ((*it) == (*it) ? (GET_LEVEL((*it)) << 1) :
				(GET_LEVEL((*it))));

	if (GET_PLANE(victim->in_room) != GET_PLANE(ch->in_room))
		prob += GET_LEVEL(victim) >> 1;

	if (GET_TIME_FRAME(ch->in_room) != GET_TIME_FRAME(victim->in_room))
		prob += GET_LEVEL(victim) >> 1;

	if (prob > number(10, CHECK_SKILL(ch, SPELL_CLAIRVOYANCE))) {
		send_to_char(ch, "You cannot discern the location of your victim.\r\n");
		return;
	}

	look_at_room(ch, victim->in_room, 1);
	if ((GET_WIS(victim) + GET_INT(victim) + GET_LEVEL(victim) + number(0,
				30)) > (GET_WIS(ch) + GET_INT(ch) + GET_LEVEL(ch) + number(0,
				30))) {
		if (affected_by_spell(victim, SPELL_DETECT_SCRYING))
			act("You feel the presence of $n pass near you.",
				FALSE, ch, 0, victim, TO_VICT);
		else
			send_to_char(victim, "You feel a strange presence pass near you.\r\n");

		send_to_char(ch, "They sensed a presence...\r\n");
	} else
		gain_skill_prof(ch, SPELL_CLAIRVOYANCE);
	return;
}

ASPELL(spell_coordinates)
{
}

ASPELL(spell_conjure_elemental)
{
	struct affected_type af;
	struct Creature *elemental = NULL;
	int sect_type;
	sect_type = ch->in_room->sector_type;

	if (GET_LEVEL(ch) >= 35
			&& number(0, GET_INT(ch)) > 3
			&& sect_type != SECT_WATER_SWIM
			&& sect_type != SECT_WATER_NOSWIM
			&& sect_type != SECT_UNDERWATER
			&& sect_type != SECT_DEEP_OCEAN)
		elemental = read_mobile(1283);	/*  Air Elemental */
	else if (GET_LEVEL(ch) >= 30
			&& number(0, GET_INT(ch)) > 3
			&& (sect_type == SECT_WATER_SWIM
				|| sect_type == SECT_WATER_NOSWIM
				|| sect_type == SECT_UNDERWATER
				|| sect_type == SECT_DEEP_OCEAN))
		elemental = read_mobile(1282);	/*  Water Elemental */
	else if (GET_LEVEL(ch) >= 25
			&& number(0, GET_INT(ch)) > 3
			&& sect_type != SECT_WATER_SWIM
			&& sect_type != SECT_WATER_NOSWIM
			&& sect_type != SECT_UNDERWATER
			&& sect_type != SECT_DEEP_OCEAN
			&& !ch->in_room->isOpenAir())
		elemental = read_mobile(1281);	/*  Fire Elemental */
	else if (GET_LEVEL(ch) >= 20
			&& number(0, GET_INT(ch)) > 3
			&& sect_type != SECT_WATER_SWIM
			&& sect_type != SECT_WATER_NOSWIM
			&& sect_type != SECT_UNDERWATER
			&& sect_type != SECT_DEEP_OCEAN
			&& !ch->in_room->isOpenAir())
		elemental = read_mobile(1280);	/* Earth Elemental */
	else
		elemental = NULL;
	if (!elemental) {
		send_to_char(ch, "You are unable to make the conjuration.\r\n");
		return;
	}
    float mult = MAX(0.5,
                     (float)((ch->getLevelBonus(SPELL_CONJURE_ELEMENTAL)) * 1.5) / 100);

    // tweak them out
    GET_HITROLL(elemental) = MIN((int)(GET_HITROLL(elemental) * mult), 60);
    GET_DAMROLL(elemental) = MIN((int)(GET_DAMROLL(elemental) * mult), 75);
    GET_MAX_HIT(elemental) = MIN((int)(GET_MAX_HIT(elemental) * mult), 30000);
    GET_HIT(elemental) = GET_MAX_HIT(elemental);

	char_to_room(elemental, ch->in_room);
	act("You have conjured $N from $S home plane!",
		FALSE, ch, 0, elemental, TO_CHAR);
	act("$n has conjured $N from $S home plane!",
		FALSE, ch, 0, elemental, TO_ROOM);

	SET_BIT(MOB_FLAGS(elemental), MOB_PET);

	if ((number(0, 101) + GET_LEVEL(elemental) - GET_LEVEL(ch)
			- GET_INT(ch) - GET_WIS(ch)) > 60 ||
			!can_charm_more(ch)) {
		act("Uh, oh.  $N doesn't look happy at you!",
			false, ch, 0, elemental, TO_CHAR);
		act("$N doesn't look too happy at $n!",
			false, ch, 0, elemental, TO_ROOM);
		elemental->startHunting(ch);
		remember(elemental, ch);
		return;
	} else {
		if (elemental->master)
			stop_follower(elemental);

		add_follower(elemental, ch);

		af.type = SPELL_CHARM;
		af.is_instant = 0;

		if (GET_INT(elemental))
			af.duration = GET_LEVEL(ch) * 18 / GET_INT(elemental);
		else
			af.duration = GET_LEVEL(ch) * 18;

		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
        af.owner = ch->getIdNum();
		af.aff_index = 0;
		affect_to_char(elemental, &af);

		gain_skill_prof(ch, SPELL_CONJURE_ELEMENTAL);
		return;
	}
}

ASPELL(spell_decoy)
{
	struct Creature *decoy = NULL;
	char buf[255];

	if (number(0, GET_INT(ch)) < 4) {
		send_to_char(ch, "You are unable to construct a decoy.\r\n");
		return;
	}

	decoy = read_mobile(92007);

	if (decoy == NULL) {
		send_to_char(ch, "You are unable to construct a decoy.\r\n");
		return;
	}

	strcpy(buf, ch->player.name);
	strcat(buf, " ");
	strcat(buf, ch->player.title);
	strcat(buf, " is standing here.\r\n");
	decoy->player.long_descr = strdup(buf);

	strcpy(buf, ch->player.name);
	strcat(buf, " .");
	strcat(buf, ch->player.name);
	decoy->player.name = strdup(buf);

	strcpy(buf, ch->player.name);
	decoy->player.short_descr = strdup(buf);

	act("You have constructed a perfect decoy of yourself!",
		FALSE, ch, 0, NULL, TO_CHAR);
	act("$n have constructed a perfect decoy of $mself!",
		FALSE, ch, 0, NULL, TO_ROOM);
	char_to_room(decoy, ch->in_room,false);

	SET_BIT(MOB_FLAGS(decoy), MOB_ISNPC);
	SET_BIT(MOB_FLAGS(decoy), MOB_SENTINEL);

	gain_skill_prof(ch, SPELL_DECOY);
	return;
}

ASPELL(spell_death_knell)
{
	struct affected_type af, af2, af3;

	// Zero out structures
    memset(&af, 0x0, sizeof(affected_type));
    memset(&af2, 0x0, sizeof(affected_type));
    memset(&af3, 0x0, sizeof(affected_type));

	if (GET_HIT(victim) > -1) {
		act("$N is way too healthy for that!", TRUE, ch, NULL, victim,
			TO_CHAR);
		return;
	}
	// Set affect types
	af.type = SPELL_DEATH_KNELL;
	af2.type = SPELL_DEATH_KNELL;
	af3.type = SPELL_DEATH_KNELL;

	// Set spell level
	af.level = GET_LEVEL(ch);
	af2.level = GET_LEVEL(ch);
	af3.level = GET_LEVEL(ch);

	// Set the duration
	af.duration = 4 + (ch->getLevelBonus(SPELL_DEATH_KNELL) / 6);
	af2.duration = 4 + (ch->getLevelBonus(SPELL_DEATH_KNELL) / 6);
	af3.duration = 4 + (ch->getLevelBonus(SPELL_DEATH_KNELL) / 6);

	// Affect locations
	af.location = APPLY_STR;
	af2.location = APPLY_HIT;
	af3.location = APPLY_DAMROLL;

    // Affect owner
    af.owner = ch->getIdNum();
    af2.owner = ch->getIdNum();
    af3.owner = ch->getIdNum();

	// Modifiers
	af.modifier = 2;
	af2.modifier =
		(GET_LEVEL(victim) + (ch->getLevelBonus(SPELL_DEATH_KNELL) / 2));
	af3.modifier = 5 + (ch->getLevelBonus(SPELL_DEATH_KNELL) / 15);

	// Affect the character
	if (!affected_by_spell(ch, SPELL_DEATH_KNELL)) {
		affect_to_char(ch, &af);
		affect_to_char(ch, &af2);
		affect_to_char(ch, &af3);

		// Impose a wait state on the casting character
		WAIT_STATE(ch, PULSE_VIOLENCE);

		// Set currhit
		GET_HIT(ch) += af2.modifier;

		// Messages
		act("$N withers and crumbles to dust as you drain $s lifeforce!",
			FALSE, ch, 0, victim, TO_CHAR);
		act("$N withers and crumbles to dust as $n drains $s lifeforce!",
			FALSE, ch, 0, victim, TO_ROOM);

		// Up the chars skill proficiency
		gain_skill_prof(ch, SPELL_DEATH_KNELL);

		// Kill the affected mob, since we know he already has -1 hp or less
		// 15 points of damage should do it.
		damage(ch, victim, 15, SPELL_DEATH_KNELL, 0);
	} else
		act("Nothing seems to happen.", FALSE, ch, 0, victim, TO_CHAR);
	return;
}

ASPELL(spell_knock)
{

	extern struct room_direction_data *knock_door;
	struct room_data *toroom = NULL;
	int kdir = -1, i;
	char dname[128];

	if (!ch)
		return;

	if (!knock_door) {
		send_to_char(ch, "Nope.\r\n");
		return;
	}

	if (!knock_door->keyword)
		strcpy(dname, "door");
	else
		strcpy(dname, fname(knock_door->keyword));

	if (!IS_SET(knock_door->exit_info, EX_ISDOOR)) {
		send_to_char(ch, "That ain't knockable!\r\n");

	} else if (!IS_SET(knock_door->exit_info, EX_CLOSED)) {
		send_to_char(ch, "That's not closed.\r\n");

	} else if (!IS_SET(knock_door->exit_info, EX_LOCKED)) {
		send_to_char(ch, "It's not even locked.\r\n");

	} else if (IS_SET(knock_door->exit_info, EX_PICKPROOF)) {
		send_to_char(ch, "It resists.\r\n");

	} else if (IS_SET(knock_door->exit_info, EX_HEAVY_DOOR) &&
		number(50, 120) > CHECK_SKILL(ch, SPELL_KNOCK)) {
		send_to_char(ch, "It is too heavy.\r\n");

	} else if (IS_SET(knock_door->exit_info, EX_REINFORCED) &&
		number(50, 120) > CHECK_SKILL(ch, SPELL_KNOCK)) {
		send_to_char(ch, "It is too stout.\r\n");

	} else if (number(50, 120) > CHECK_SKILL(ch, SPELL_KNOCK)) {
		send_to_char(ch, "You fail.\r\n");

	} else {

		REMOVE_BIT(knock_door->exit_info, EX_CLOSED);
		REMOVE_BIT(knock_door->exit_info, EX_LOCKED);
		send_to_char(ch, "Opened.\r\n");
		sprintf(buf, "The %s %s flung open suddenly.", dname, ISARE(dname));
		act(buf, FALSE, ch, 0, 0, TO_ROOM);

		for (i = 0; i < NUM_DIRS; i++) {
			if (ch->in_room->dir_option[i] &&
				ch->in_room->dir_option[i] == knock_door) {
				kdir = i;
				break;
			}
		}

		if (kdir == -1)
			return;

		kdir = rev_dir[kdir];

		if ((toroom = knock_door->to_room) &&
			toroom->dir_option[kdir] &&
			ch->in_room == toroom->dir_option[kdir]->to_room) {

			if (IS_SET(toroom->dir_option[kdir]->exit_info, EX_CLOSED) &&
				IS_SET(toroom->dir_option[kdir]->exit_info, EX_ISDOOR)) {
				REMOVE_BIT(toroom->dir_option[kdir]->exit_info, EX_CLOSED);
				REMOVE_BIT(toroom->dir_option[kdir]->exit_info, EX_LOCKED);

				sprintf(buf, "The %s %s flung open from the other side.",
					dname, ISARE(dname));
				send_to_room(buf, toroom);

			}
		}

		gain_skill_prof(ch, SPELL_KNOCK);
	}

	knock_door = NULL;
	return;
}

ASPELL(spell_sword)
{
	struct affected_type af;
	struct Creature *sword = NULL;


	if ((GET_LEVEL(ch) + GET_INT(ch) + number(0, 10)) < 50 + number(0, 13)) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}

	sword = read_mobile(1285);
	if (!sword) {
		send_to_char(ch, 
			"You feel a strange sensation as you attempt to contact the"
			"ethereal plane.\r\n");
		return;
	}

	char_to_room(sword, ch->in_room, false);
	act("You have conjured $N from the ethereal plane!", FALSE, ch, 0, sword,
		TO_CHAR);
	act("$n has conjured $N from the ethereal plane!", FALSE, ch, 0, sword,
		TO_ROOM);

	if (sword->master)
		stop_follower(sword);

	add_follower(sword, ch);

	af.type = SPELL_CHARM;
	af.is_instant = 0;

	if (GET_INT(sword))
		af.duration = GET_LEVEL(ch) * 18 / GET_INT(sword);
	else
		af.duration = GET_LEVEL(ch) * 18;

	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	af.level = level;
    af.owner = ch->getIdNum();
	af.aff_index = 0;

	affect_to_char(sword, &af);
	SET_BIT(MOB_FLAGS(sword), MOB_PET);
	IS_CARRYING_N(sword) = CAN_CARRY_N(sword);

	GET_HITROLL(sword) = (GET_LEVEL(ch) >> 2) + 5;
	GET_DAMROLL(sword) = (GET_LEVEL(ch) >> 3) + 4;
	/*
	   if (number((GET_LEVEL(ch) >> 1), GET_LEVEL(ch)) > 40)
	   AFF_FLAGS(sword) = AFF_SANCTUARY;
	   else
	   AFF_FLAGS(sword) = 0;
	 */

	return;
}


ASPELL(spell_control_weather)
{
	if (!OUTSIDE(ch) && GET_LEVEL(ch) < LVL_DEMI)
		send_to_char(ch, "You cannot do this indoors!\r\n");
	else if ((number(0, GET_LEVEL(ch)) +
			number(0, GET_INT(ch)) +
			number(0, GET_SKILL(ch, SPELL_CONTROL_WEATHER))) > number(50, 121))
		zone_weather_change(ch->in_room->zone);
	else
		send_to_char(ch, "You fail to control the weather.\r\n");
}

ASPELL(spell_retrieve_corpse)
{
	struct obj_data *corpse;


	if (!victim)
		return;
	if (IS_MOB(victim)) {
		send_to_char(ch, 
			"You are unable to find the former body of this entity.\r\n");
		return;
	}
	if (ch != victim->master) {
		send_to_char(ch, 
			"They must be following you in order for this to work.\r\n");
		return;
	}

	for (corpse = object_list; corpse; corpse = corpse->next)
		if (GET_OBJ_TYPE(corpse) == ITEM_CONTAINER && GET_OBJ_VAL(corpse, 3) &&
			CORPSE_IDNUM(corpse) == GET_IDNUM(victim)) {
			if (corpse->in_room)
				obj_from_room(corpse);
			else if (corpse->in_obj)
				obj_from_obj(corpse);
			else if (corpse->carried_by) {
				act("$p disappears out of your hands!", FALSE,
					corpse->carried_by, corpse, 0, TO_CHAR);
				obj_from_char(corpse);
			} else if (corpse->worn_by) {
				act("$p disappears off of your body!", FALSE, corpse->worn_by,
					corpse, 0, TO_CHAR);
				unequip_char(corpse->worn_by, corpse->worn_on, MODE_EQ);
			} else {
				act("$S corpse has shifted out of the universe.", FALSE, ch, 0,
					victim, TO_CHAR);
				return;
			}
			obj_to_room(corpse, ch->in_room);
			act("$p appears at the center of the room!", FALSE, ch, corpse, 0,
				TO_ROOM);
			return;
		}
	act("You cannot locate $S corpse.", FALSE, ch, 0, victim, TO_CHAR);
	return;
}

ASPELL(spell_gust_of_wind)
{
	int attempt, count = 0;
	struct room_data *target_room;
	struct room_affect_data *rm_aff;
	int found = 0;

	if (obj) {

		if (!obj->in_room) {
			errlog("%s tried to gust %s at %d.",
				GET_NAME(ch), obj->name, ch->in_room->number);
			act("$p doesn't budge.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		if (GET_LEVEL(ch) + number(10, GET_DEX(ch) + 40) +
			CHECK_SKILL(ch, SPELL_GUST_OF_WIND) > obj->getWeight() &&
			CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
			attempt = number(1, NUM_OF_DIRS - 1);
			while (!CAN_GO(ch, attempt) && count < 40) {
				attempt = number(1, NUM_OF_DIRS - 1);
				count++;
			}
			if (EXIT(ch, attempt) &&
				(target_room = EXIT(ch, attempt)->to_room) != NULL) {
				if (CAN_GO(ch, attempt) &&
					(!ROOM_FLAGGED(target_room, ROOM_HOUSE) ||
						House_can_enter(ch, target_room->number)) &&
					(!ROOM_FLAGGED(target_room, ROOM_CLAN_HOUSE) ||
						clan_house_can_enter(ch, target_room))) {
					sprintf(buf,
						"A sudden gust of wind blows $p out of sight to the %s!",
						dirs[attempt]);
					act(buf, TRUE, ch, obj, 0, TO_ROOM);
					act(buf, TRUE, ch, obj, 0, TO_CHAR);
					obj_from_room(obj);
					obj_to_room(obj, target_room);
					if (obj->in_room->people) {
						sprintf(buf,
							"$p is blown in on a gust of wind from the %s!",
							from_dirs[attempt]);
						act(buf, FALSE, obj->in_room->people, obj, 0, TO_ROOM);
						found = TRUE;
					}
				}
			}
		}
		if (!found)
			act("A gust of wind buffets $p!", FALSE, ch, obj, 0, TO_ROOM);

		if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED)) {
			for (rm_aff = ch->in_room->affects; rm_aff; rm_aff = rm_aff->next)
				if (rm_aff->type == RM_AFF_FLAGS &&
					IS_SET(rm_aff->flags, ROOM_SMOKE_FILLED)) {
					affect_from_room(ch->in_room, rm_aff);
					break;
				}
		}
		return;
	}
	// if (victim) stuff here
	if (!victim)
		return;

	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
		ch->master->in_room == ch->in_room) {
		if (ch == victim) {
			act("You just can't stand the thought of leaving $N behind.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		} else if (victim == ch->master) {
			act("You really can't stand the though of parting with $N.",
				FALSE, ch, 0, ch->master, TO_CHAR);
			return;
		}
	}

	if ((GET_LEVEL(victim) + GET_DEX(victim) +
			((victim->getPosition() == POS_FLYING) ? -20 : 0)) <
		GET_LEVEL(ch) + number(10, GET_DEX(ch) + 40) &&
		!mag_savingthrow(victim, level, SAVING_BREATH) && !IS_DRAGON(victim) &&
		!affected_by_spell(victim, SPELL_ENTANGLE) &&
		!MOB_FLAGGED(victim, MOB_NOBASH) &&
		!(GET_CLASS(victim) == CLASS_EARTH) && !IS_GIANT(victim) &&
		(GET_WEIGHT(victim) + ((IS_CARRYING_W(victim) +
					IS_WEARING_W(victim)) >> 1))
		< (number(200, 500) + GET_INT(ch) + 16 * GET_LEVEL(ch))) {
		attempt = number(1, NUM_OF_DIRS - 1);
		while (!CAN_GO(victim, attempt) && count < 40) {
			attempt = number(1, NUM_OF_DIRS - 1);
			count++;
		}
		if (EXIT(victim, attempt) &&
			(target_room = EXIT(victim, attempt)->to_room) != NULL) {
			if (CAN_GO(victim, attempt)
				&& !ROOM_FLAGGED(target_room, ROOM_DEATH)
				&& (!ROOM_FLAGGED(target_room, ROOM_HOUSE)
					|| House_can_enter(victim, target_room->number))
				&& (!ROOM_FLAGGED(target_room, ROOM_CLAN_HOUSE)
					|| clan_house_can_enter(victim, target_room))
				&& (!ROOM_FLAGGED(target_room, ROOM_NOTEL)
					|| !target_room->people)) {
				if (can_travel_sector(victim, SECT_TYPE(target_room), 0)) {
					sprintf(buf,
						"A sudden gust of wind blows $N out of sight to the %s!",
						dirs[attempt]);
					act(buf, TRUE, ch, 0, victim, TO_NOTVICT);
					act(buf, TRUE, ch, 0, victim, TO_CHAR);
					send_to_char(victim, "A sudden gust of wind blows you to the %s!",
						dirs[attempt]);
					char_from_room(victim);
					char_to_room(victim, target_room);
					look_at_room(victim, victim->in_room, 0);
					sprintf(buf,
						"$n is blown in on a gust of wind from the %s!",
						from_dirs[attempt]);
					act(buf, FALSE, victim, 0, 0, TO_ROOM);
					victim->setPosition(POS_RESTING);
					found = TRUE;
				}
			}
		}
	}
	if (!found) {
		act("A gust of wind buffets $N, who stands $S ground!",
			FALSE, ch, 0, victim, TO_NOTVICT);
		if (victim != ch)
			act("A gust of wind buffets $N, who stands $S ground!",
				FALSE, ch, 0, victim, TO_CHAR);
		act("A gust of wind buffets you, but you stand your ground!",
			FALSE, ch, 0, victim, TO_VICT);
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED)) {
		for (rm_aff = ch->in_room->affects; rm_aff; rm_aff = rm_aff->next)
			if (rm_aff->type == RM_AFF_FLAGS &&
				IS_SET(rm_aff->flags, ROOM_SMOKE_FILLED)) {
				affect_from_room(ch->in_room, rm_aff);
				break;
			}
	}

	return;
}

ASPELL(spell_peer)
{
	struct room_data *target_rnum;

	if (GET_OBJ_TYPE(obj) != ITEM_PORTAL && GET_OBJ_TYPE(obj) != ITEM_VSTONE)
		send_to_char(ch, 
			"Sorry, that's either not a portal, or it is very unusual.\r\n");
	else if (!(target_rnum = real_room(GET_OBJ_VAL(obj, 0))))
		send_to_char(ch, "It leads absolutely nowhere!\r\n");
	else {
		act("You peer through $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n peers through $p.", FALSE, ch, obj, 0, TO_ROOM);
		look_at_room(ch, target_rnum, 1);
		return;
	}
}

ASPELL(spell_vestigial_rune)
{
	if (!obj) {
		send_to_char(ch, "Error.  No target object in spell_vestigial_rune.\r\n");
		return;
	}

	if (GET_OBJ_TYPE(obj) != ITEM_VSTONE)
		send_to_char(ch, "That object is not an vestigial stone.\r\n");
	else if (GET_OBJ_VAL(obj, 0))
		send_to_char(ch, "This stone is already linked to the world.\r\n");
	else if (GET_OBJ_VAL(obj, 1) && GET_IDNUM(ch) != GET_OBJ_VAL(obj, 1))
		send_to_char(ch, 
			"This stone is already linked to another sentient being.\r\n");
	else if (!House_can_enter(ch, ch->in_room->number) ||
			(ROOM_FLAGGED(ch->in_room, ROOM_GODROOM)
				&& GET_LEVEL(ch) < LVL_CREATOR)
			|| ROOM_FLAGGED(ch->in_room,
				ROOM_DEATH | ROOM_NOTEL | ROOM_NOMAGIC | ROOM_NORECALL))
		act("$p hums dully and gets cold.", FALSE, ch, obj, 0, TO_CHAR);
	else {
		GET_OBJ_VAL(obj, 0) = ch->in_room->number;
		GET_OBJ_VAL(obj, 1) = GET_IDNUM(ch);
		if (GET_OBJ_VAL(obj, 2) >= 0)
			GET_OBJ_VAL(obj, 2) = (level >> 2);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
		act("$p glows briefly with a faint light.", FALSE, ch, obj, 0,
			TO_ROOM);
		act("$p glows briefly with a faint light.", FALSE, ch, obj, 0,
			TO_CHAR);
	}
}

ASPELL(spell_id_insinuation)
{

	struct Creature *pulv, *ulv = NULL;	/* un-lucky-vict */
	int total = 0;

	if (!victim)
		return;
	if (victim->numCombatants())
		return;

	send_to_char(victim, "You feel an intense desire to KILL someone!!\r\n");
	act("$n looks around frantically!", TRUE, victim, 0, 0, TO_ROOM);

	if ((mag_savingthrow(victim, level, SAVING_PSI) ||
			number(0, GET_LEVEL(victim)) > number(0, GET_LEVEL(ch))) &&
		can_see_creature(victim, ch)) {
		act("$n attacks $N in a rage!!\r\n", TRUE, victim, 0, ch, TO_NOTVICT);
		act("$n attacks you in a rage!!\r\n", TRUE, victim, 0, ch, TO_VICT);
		act("You attack $N in a rage!!\r\n", TRUE, victim, 0, ch, TO_CHAR);
		//set_fighting(victim, ch, true);
//        slog("%s:%d Adding combat 0x%x->addCombat(0x%x, false)",
//             __FILE__, __LINE__, &(*ch), &(*victim));
        ch->addCombat(victim, true);
//        slog("%s:%d Adding combat 0x%x->addCombat(0x%x, false)",
//             __FILE__, __LINE__, &(*victim), &(*ch));
        victim->addCombat(ch, false);
		return;
	}

	ulv = NULL;
	CreatureList::iterator it = victim->in_room->people.begin();
	for (; it != victim->in_room->people.end(); ++it) {
		pulv = *it;
		if (pulv == victim || pulv == ch)
			continue;
		if (PRF_FLAGGED(pulv, PRF_NOHASSLE))
			continue;
		// prevent all reputation adjustments as a result
		// of id insinuation
		if (!ok_to_damage(pulv, victim))
			continue;

		if (!ok_to_damage(ch, victim))
			continue;
		
		if (!number(0, total))
			ulv = pulv;
		total++;
	}

	if (!ulv) {
		if (number(0, CHECK_SKILL(ch, SPELL_ID_INSINUATION)) > 50 ||
			!can_see_creature(victim, ch)) {
			send_to_char(ch, "Nothing seems to happen.\r\n");
			return;
		} else
			ulv = ch;
	}

	act("$n attacks $N in a rage!!\r\n", TRUE, victim, 0, ulv, TO_NOTVICT);
	act("$n attacks you in a rage!!\r\n", TRUE, victim, 0, ulv, TO_VICT);
	act("You attack $N in a rage!!\r\n", TRUE, victim, 0, ulv, TO_CHAR);

	//set_fighting(victim, ulv, false);
//    slog("%s:%d Adding combat 0x%x->addCombat(0x%x, false)",
//         __FILE__, __LINE__, &(*victim), &(*ulv));
    victim->addCombat(ulv, false);
//    slog("%s:%d Adding combat 0x%x->addCombat(0x%x, false)",
//         __FILE__, __LINE__, &(*ulv), &(*victim));
    ulv->addCombat(victim, false);
	gain_skill_prof(ch, SPELL_ID_INSINUATION);
}


ASPELL(spell_shadow_breath)
{
	struct room_affect_data rm_aff;
	struct Creature *vch = NULL;
	int i;

	if (!victim)
		return;

	if (!ROOM_FLAGGED(victim->in_room, ROOM_DARK)) {
		rm_aff.description =
			str_dup("This room is covered in a deep magical darkness.\r\n");
		rm_aff.duration = 1;
		rm_aff.type = RM_AFF_FLAGS;
		rm_aff.flags = ROOM_DARK;
		affect_to_room(victim->in_room, &rm_aff);
	}
	CreatureList::iterator it = victim->in_room->people.begin();
	for (; it != victim->in_room->people.end(); ++it) {
		vch = *it;
		if (PRF_FLAGGED(vch, PRF_NOHASSLE))
			continue;

		affect_from_char(vch, SPELL_FLUORESCE);
		affect_from_char(vch, SPELL_GLOWLIGHT);
		affect_from_char(vch, SPELL_DIVINE_ILLUMINATION);

		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(vch, i) && IS_OBJ_TYPE(GET_EQ(vch, i), ITEM_LIGHT) &&
				GET_OBJ_VAL(GET_EQ(vch, i), 2) > 0) {
				GET_OBJ_VAL(GET_EQ(vch, i), 2) = 0;
				vch->in_room->light--;
			}
		}
	}
}

const int legion_vnums[5] = {
	16160,						// spined
	16161,						// bearded
	16162,						// barbed
	16163,						// bone
	16164						// horned
};

ASPELL(spell_summon_legion)
{
	int i, count;
	float mult;
	struct Creature *devil = NULL;
	struct affected_type af;
	struct follow_type *k = NULL;
	/*
	   i = number(0, MAX(1, GET_REMORT_GEN(ch)-4));
	   i = MAX(((GET_REMORT_GEN(ch) - 4) / 3), i);
	   i = MIN(i, 4);
	 */

	if (number(0, 120) > CHECK_SKILL(ch, SPELL_SUMMON_LEGION)) {
		send_to_char(ch, "You fail.\r\n");
		return;
	}
	int pets = 0;
	for (follow_type * f = ch->followers; f; f = f->next) {
		if (IS_NPC(f->follower) && IS_PET(f->follower)) {
			pets++;
		}
	}

	// devil modification based on power level of leige
	mult = ch->getLevelBonus(SPELL_SUMMON_LEGION) * 1.5 / 100;

	// choose the appropriate minion
	i = (ch->getLevelBonus(SPELL_SUMMON_LEGION) * 2 / 3) + number(1, 30);
	i = i / 20;					// divide based on number of devils avaliable
	i = MAX(i, 0);
	i = MIN(i, 4);

	if (!(devil = read_mobile(legion_vnums[i]))) {
		errlog("unable to load legion, i=%d.", i);
		send_to_char(ch, "legion error.\r\n");
		return;
	}
	// get better at it.
	gain_skill_prof(ch, SPELL_SUMMON_LEGION);

	// tweak based on multiplier
	GET_HITROLL(devil) = (sbyte) MIN(GET_HITROLL(devil) * mult, 60);
	GET_DAMROLL(devil) = (sbyte) MIN(GET_DAMROLL(devil) * mult, 60);
	GET_MAX_HIT(devil) = (sh_int) MIN(GET_MAX_HIT(devil) * mult, 30000);
	GET_HIT(devil) = GET_MAX_HIT(devil);

	// Make sure noone gets xp fer these buggers.
	SET_BIT(MOB_FLAGS(devil), MOB_PET);
	// or gold
	GET_GOLD(devil) = 0;



	char_to_room(devil, ch->in_room, false);
	act("A glowing interplanar rift opens with a crack of thunder!\r\n"
		"$n steps from the mouth of the conduit, which closes with a roar!",
		FALSE, devil, 0, 0, TO_ROOM);

	if (number(0, 50 + GET_LEVEL(devil)) >
			ch->getLevelBonus(SPELL_SUMMON_LEGION) ||
			!can_charm_more(ch)) {
		act("Uh, oh.  $N doesn't look happy at you!",
			false, ch, 0, devil, TO_CHAR);
		act("$N doesn't look too happy at $n!",
			false, ch, 0, devil, TO_ROOM);
		devil->startHunting(ch);
		remember(devil, ch);
		return;
	}
	for (k = ch->followers, count = 0; k; k = k->next)
		if (IS_NPC(k->follower) && IS_DEVIL(k->follower))
			count++;

	if (count > number(1, GET_REMORT_GEN(ch)))
		return;

	// pets too scared to help.
	REMOVE_BIT(MOB_FLAGS(devil), MOB_HELPER);
	REMOVE_BIT(MOB_FLAGS(devil), MOB_AGGRESSIVE);
	// pets shouldn't snatch shit.
	REMOVE_BIT(MOB_FLAGS(devil), MOB_SCAVENGER);
	REMOVE_BIT(MOB2_FLAGS(devil), MOB2_LOOTER);
	// They shouldn't bother to stay put either.
	REMOVE_BIT(MOB_FLAGS(devil), MOB_STAY_ZONE);
	REMOVE_BIT(MOB2_FLAGS(devil), MOB2_STAY_SECT);

	add_follower(devil, ch);

	af.type = SPELL_CHARM;
	af.is_instant = 0;

	af.duration = level * 10 / (GET_INT(devil) - GET_REMORT_GEN(ch));

	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	af.level = level;
    af.owner = ch->getIdNum();
	af.aff_index = 0;
	affect_to_char(devil, &af);

	return;
}

//
// Evil cleric stuff
//

struct Creature *
load_corpse_owner(struct obj_data *obj)
{
	struct Creature *vict = NULL;

	//
	// mobile
	//

	if (CORPSE_IDNUM(obj) < 0) {
		return (real_mobile_proto(-CORPSE_IDNUM(obj)));
	}
	//
	// pc, load from file
	//

	vict = new Creature(true);

	if (vict->loadFromXML(CORPSE_IDNUM(obj)))
		return (vict);

	delete vict;

	return NULL;
}

ASPELL(spell_animate_dead)
{
	struct affected_type af;
	struct Creature *orig_char = NULL;
	struct Creature *zombie = NULL;
	struct obj_data *i = NULL;
	float mult = (float)level / 70;

	// Casting it on yourself. (potions etc)
	if (ch && victim && ch == victim) {
		if (IS_UNDEAD(ch)) {
			GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dice(4, level));
			send_to_char(ch, "You feel a renewal of your tainted form.\r\n");
		}
		return;
	}

	if (!obj) {
		send_to_char(ch, "You cannot animate that.\r\n");
		return;
	}

	if (!IS_CORPSE(obj)) {
		send_to_char(ch, "You cannot animate that.\r\n");
		return;
	}

	orig_char = load_corpse_owner(obj);

	if (!orig_char) {
		send_to_char(ch, "The dark powers are not with you, tonight.\r\n");
		errlog("unable to load an original owner for corpse, idnum %d.",
			CORPSE_IDNUM(obj));
		return;
	}
	//
	// char has to be in a room to be sent through extract_char
	//

	if (IS_UNDEAD(orig_char)) {
		act("You cannot re-animate $p.", FALSE, ch, obj, 0, TO_CHAR);
		if (IS_PC(orig_char))
			delete orig_char;
		return;
	}

	if (GET_LEVEL(orig_char) >= LVL_AMBASSADOR
		&& GET_LEVEL(orig_char) > GET_LEVEL(ch)) {
		send_to_char(ch, 
			"You find yourself unable to perform this necromantic deed.\r\n");
		if (IS_PC(orig_char))
			delete orig_char;
		return;
	}

	if (!(zombie = read_mobile(ZOMBIE_VNUM))) {
		send_to_char(ch, "The dark powers are not with you, tonight.\r\n");
		errlog("unable to load ZOMBIE_VNUM in spell_animate_dead.");
		if (IS_PC(orig_char))
			delete orig_char;
		return;
	}

//	char_to_room(orig_char, ch->in_room, false);

	//
	// strings
	//
	sprintf(buf2, "%s zombie animated", obj->aliases);
	zombie->player.name = str_dup(buf2);
	zombie->player.short_descr = str_dup(obj->name);
	strcpy(buf, obj->name);
	strcat(buf, " is standing here.");
	CAP(buf);
	zombie->player.long_descr = str_dup(buf);
	zombie->player.description = NULL;

	//
	// level, char_class
	// 

	GET_LEVEL(zombie) =
		(char)MIN(LVL_AMBASSADOR - 1, GET_LEVEL(orig_char) * mult);

	if (level > number(50, 100)) {
		if (IS_NPC(orig_char)) {
			if (GET_CLASS(orig_char) < NUM_CLASSES)
				GET_REMORT_CLASS(zombie) = GET_CLASS(orig_char);
			else if (GET_REMORT_CLASS(orig_char) < NUM_CLASSES)
				GET_REMORT_CLASS(zombie) = GET_REMORT_CLASS(orig_char);
		} else {
			GET_REMORT_CLASS(zombie) = GET_CLASS(orig_char);
		}
	}

	GET_EXP(zombie) = 0;

	//
	// stats
	//

	GET_STR(zombie) = (char)MIN(25, GET_STR(orig_char) * mult);
	GET_DEX(zombie) = (char)MIN(25, GET_DEX(orig_char) * mult);
	GET_CON(zombie) = (char)MIN(25, GET_CON(orig_char) * mult);
	GET_INT(zombie) = (char)MIN(25, GET_INT(orig_char) * mult);
	GET_WIS(zombie) = (char)MIN(25, GET_WIS(orig_char) * mult);
	GET_CHA(zombie) = (char)MIN(25, GET_CHA(orig_char) * mult);

	GET_HITROLL(zombie) = (char)MIN(50, GET_HITROLL(orig_char) * mult);
	GET_DAMROLL(zombie) = (char)MIN(50, GET_HITROLL(orig_char) * mult);

	GET_AC(zombie) = (short)MIN(100, GET_AC(orig_char) * mult);

	//
	// points, hit mana move
	//
	GET_MAX_HIT(zombie) = (short)MIN(10000, GET_MAX_HIT(orig_char) * mult);
	GET_HIT(zombie) = (short)GET_MAX_HIT(zombie);

	GET_MAX_MANA(zombie) = (short)MIN(10000, GET_MAX_MANA(orig_char) * mult);
	GET_MANA(zombie) = (short)GET_MAX_MANA(zombie);

	GET_MAX_MOVE(zombie) = (short)MIN(10000, GET_MAX_MOVE(orig_char) * mult);
	GET_MOVE(zombie) = (short)GET_MAX_MOVE(zombie);

	//
	// flags
	//

	SET_BIT(AFF_FLAGS(zombie), AFF_FLAGS(orig_char));
	REMOVE_BIT(AFF_FLAGS(zombie), AFF_GROUP);

	SET_BIT(AFF2_FLAGS(zombie), AFF2_FLAGS(orig_char));
	REMOVE_BIT(AFF2_FLAGS(zombie),
		AFF2_MEDITATE | AFF2_INTIMIDATED);

    zombie->extinguish();

	SET_BIT(AFF3_FLAGS(zombie), AFF3_FLAGS(orig_char));
	REMOVE_BIT(AFF3_FLAGS(zombie),
		AFF3_SELF_DESTRUCT | AFF3_STASIS | AFF3_PSYCHIC_CRUSH);
	// Make sure noone gets xp fer these buggers.
	SET_BIT(MOB_FLAGS(zombie), MOB_PET);

	if (isname(GET_NAME(zombie), "headless"))
		SET_BIT(AFF2_FLAGS(zombie), AFF2_NECK_PROTECTED);

	REMOVE_BIT(AFF2_FLAGS(zombie), AFF2_BERSERK);

	//
	// transfer equipment and delete the corpse
	//

	while ((i = obj->contains)) {
		obj_from_obj(i);
		obj_to_char(i, zombie);
		if (IS_IMPLANT(i))
			SET_BIT(GET_OBJ_WEAR(i), ITEM_WEAR_TAKE);
		if (GET_OBJ_DAM(i) > 0)
			GET_OBJ_DAM(i) >>= 1;
	}

	extract_obj(obj);

	if (IS_PC(orig_char))
		delete orig_char;

	char_to_room(zombie, ch->in_room, false);
	act("$n rises slowly to a standing position.", FALSE, zombie, 0, 0,
		TO_ROOM);

	//
	// make the zombie a (charmed) follower
	//

	add_follower(zombie, ch);

	af.type = SPELL_CHARM;
	af.is_instant = 0;
	af.duration = -1;
	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	af.level = level;
    af.owner = ch->getIdNum();
	af.aff_index = 0;
	affect_to_char(zombie, &af);

	gain_skill_prof(ch, SPELL_ANIMATE_DEAD);
}

ASPELL(spell_unholy_stalker)
{

	int distance = 0;
	struct Creature *stalker = NULL;
	float mult = (float)level / 70;

	if (victim->in_room != ch->in_room) {

		if (AFF_FLAGGED(victim, AFF_NOTRACK) ||
			find_first_step(ch->in_room, victim->in_room, STD_TRACK) < 0 ||
			(distance = find_distance(ch->in_room, victim->in_room)) < 0) {
			act("You cannot discern a physical path to $M.", FALSE, ch, 0,
				victim, TO_CHAR);
			return;
		}
	} else
		distance = 0;

	if (distance > level) {
		act("$N is beyond the range of your powers.", FALSE, ch, 0, victim,
			TO_CHAR);
		return;
	}

	if (IS_PC(victim)) {
		send_to_char(ch, "The dark powers cannot reach them.\r\n");
		return;
	}

	if (!(stalker = read_mobile(UNHOLY_STALKER_VNUM))) {
		send_to_char(ch, "The dark powers are not with you, tonight.\r\n");
		errlog("unable to load STALKER_VNUM in spell_unholy_stalker.");
		return;
	}

	GET_LEVEL(stalker) =
		(char)MIN(LVL_AMBASSADOR - 1, GET_LEVEL(stalker) * mult);

	GET_EXP(stalker) = 0;

	//
	// stats
	//

	GET_STR(stalker) = (char)MIN(25, GET_STR(stalker) * mult);
	GET_DEX(stalker) = (char)MIN(25, GET_DEX(stalker) * mult);
	GET_CON(stalker) = (char)MIN(25, GET_CON(stalker) * mult);
	GET_INT(stalker) = (char)MIN(25, GET_INT(stalker) * mult);
	GET_WIS(stalker) = (char)MIN(25, GET_WIS(stalker) * mult);
	GET_CHA(stalker) = (char)MIN(25, GET_CHA(stalker) * mult);

	GET_HITROLL(stalker) = (char)MIN(50, GET_HITROLL(stalker) * mult);
	GET_DAMROLL(stalker) = (char)MIN(50, GET_HITROLL(stalker) * mult);

	GET_AC(stalker) = (char)MIN(100, GET_AC(stalker) * mult);

	//
	// points, hit mana move
	//

	GET_MAX_HIT(stalker) = (short)MIN(10000, GET_MAX_HIT(stalker) * mult);
	GET_HIT(stalker) = (short)GET_MAX_HIT(stalker);

	GET_MAX_MANA(stalker) = (short)MIN(10000, GET_MAX_MANA(stalker) * mult);
	GET_MANA(stalker) = (short)GET_MAX_MANA(stalker);

	GET_MAX_MOVE(stalker) = (short)MIN(10000, GET_MAX_MOVE(stalker) * mult);
	GET_MOVE(stalker) = (short)GET_MAX_MOVE(stalker);


	if (level > number(50, 80))
		SET_BIT(AFF_FLAGS(stalker), AFF_INVISIBLE);
	if (level > number(50, 80))
		SET_BIT(AFF_FLAGS(stalker), AFF_REGEN);

	if (level > number(50, 80))
		SET_BIT(AFF2_FLAGS(stalker), AFF2_HASTE);
	// Make sure noone gets xp fer these buggers.
	SET_BIT(MOB_FLAGS(stalker), MOB_PET);

	char_to_room(stalker, ch->in_room, false);

	act("The air becomes cold as $n materializes from the negative planes.",
		FALSE, stalker, 0, 0, TO_ROOM);

	send_to_char(ch, "%s begins to stalk %s.\r\n", GET_NAME(stalker),
		GET_NAME(victim));
	CAP(buf);

	stalker->startHunting(victim);

	slog("%s has sent an unholy stalker after %s.", GET_NAME(ch),
		GET_NAME(victim));

}

ASPELL(spell_control_undead)
{
	struct affected_type af;


	if (!IS_UNDEAD(victim)) {
		act("$N is not undead!", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}

	if (!IS_NPC(victim)) {
		if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA)
			&& !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
			send_to_char(ch, 
				"You cannot control undead players in arena rooms.\r\n");
			return;
		}
		if (!victim->desc) {
			send_to_char(ch, "You cannot control the linkdead.\r\n");
			return;
		}
	}

	else if (!ok_damage_vendor(ch, victim)) {

		act("$N falls down laughing at you!", FALSE, ch, 0, victim, TO_CHAR);
		act("$N peers deeply into your eyes...", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N falls down laughing at $n!", FALSE, ch, 0, victim, TO_ROOM);
		act("$N peers deeply into the eyes of $n...",
			FALSE, ch, 0, victim, TO_ROOM);
		if (ch->master)
			stop_follower(ch);
		add_follower(ch, victim);
		af.type = SPELL_CHARM;
		af.is_instant = 0;
		if (GET_INT(victim))
			af.duration = 6 * 18 / GET_INT(victim);
		else
			af.duration = 6 * 18;
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
		af.aff_index = 0;
		affect_to_char(ch, &af);
		act("Isn't $n just such a great friend?", FALSE, victim, 0, ch,
			TO_VICT);

		if (IS_NPC(ch)) {
			REMOVE_BIT(MOB_FLAGS(ch), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
		}

	}

	else if (IS_AFFECTED(victim, AFF_SANCTUARY)) {
		send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
	}

	else if (MOB_FLAGGED(victim, MOB_NOCHARM) && GET_LEVEL(ch) < LVL_IMPL) {
		send_to_char(ch, "Your victim resists!\r\n");
	}

	else if (IS_AFFECTED(ch, AFF_CHARM)) {
		send_to_char(ch, "You can't have any followers of your own!\r\n");
	}

	else if (IS_AFFECTED(victim, AFF_CHARM)) {
		send_to_char(ch, "You can't do that.\r\n");
	}

	else if (circle_follow(victim, ch)) {
		send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
	}

	else if (mag_savingthrow(victim, level, SAVING_PARA) &&
		(GET_INT(victim) + number(0, GET_LEVEL(victim) + 20) > level)) {
		send_to_char(ch, "Your victim resists!\r\n");
		if (MOB_FLAGGED(victim, MOB_MEMORY))
			remember(victim, ch);
	}

	else if (GET_LEVEL(victim) > LVL_AMBASSADOR
		&& GET_LEVEL(victim) > GET_LEVEL(ch)) {
		act("$N sneers at you with disgust.\r\n", FALSE, ch, 0, victim,
			TO_CHAR);
		act("$N sneers at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_NOTVICT);
		act("You sneer at $n with disgust.\r\n", FALSE, ch, 0, victim,
			TO_VICT);
		return;
	}

	else {
		if (victim->master)
			stop_follower(victim);

		add_follower(victim, ch);

		af.type = SPELL_CHARM;
		af.is_instant = 0;

		if (GET_INT(victim))
			af.duration = 24 * 18 / GET_INT(victim);
		else
			af.duration = 24 * 18;

		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.level = level;
        af.owner = ch->getIdNum();
		af.aff_index = 0;
		affect_to_char(victim, &af);

		act("$n has become your unholy master.", FALSE, ch, 0, victim,
			TO_VICT);

		if (IS_NPC(victim)) {
			REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
		}
	}
}

ASPELL(spell_sun_ray)
{
	int dam = 0;

	send_to_room("A brilliant ray of sunlight bathes the area!\r\n",
		ch->in_room);

	// check for players if caster is not a pkiller
	if (!IS_NPC(ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (ch == *it)
				continue;
			if (!IS_NPC((*it)) && IS_UNDEAD((*it))) {
				act("You cannot do this, because this action might cause harm to $N,\r\n"
                    "and you have not chosen to be a Pkiller.\r\n"
                    "You can toggle this with the command 'pkiller'.", 
                    FALSE, ch, 0, *it, TO_CHAR );
				return;
			}
		}
	}
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (ch == (*it))
			continue;
		if (PRF_FLAGGED((*it), PRF_NOHASSLE))
			continue;
		if (IS_UNDEAD((*it))) {
			dam = dice(level, 18) + level;
			if( IS_EVIL((*it)) ) {
				dam += ( GET_ALIGNMENT(ch) - GET_ALIGNMENT((*it)) )/4;
			}
			if( !damage(ch, (*it), dam, TYPE_ABLAZE, -1) ) {
				if (!IS_AFFECTED(*it, AFF_BLIND) &&
					!MOB_FLAGGED(*it, MOB_NOBLIND)) {

					struct affected_type af, af2;
					af.is_instant = af2.is_instant = 0;
					af.type = af2.type = SPELL_BLINDNESS;
					af.location = APPLY_HITROLL;
					af.modifier = -4;
					af.duration = 2;
					af.bitvector = AFF_BLIND;
                    af.level = af2.level = level;
                    af.owner = ch->getIdNum();
					af2.location = APPLY_AC;
					af2.modifier = 40;
					af2.duration = 2;
					af2.bitvector = AFF_BLIND;
                    af2.owner = ch->getIdNum();
					affect_join(*it, &af, FALSE, FALSE, FALSE, FALSE);
					if (af2.bitvector || af2.location)
						affect_join((*it), &af2, FALSE, FALSE, FALSE, FALSE);

					act("$n cries out in pain, clutching $s eyes!",
						FALSE, (*it), NULL, ch, TO_ROOM);
					act("You begin to scream as the flames of light sear out your eyes!", 
                        FALSE, ch, NULL, (*it), TO_VICT);
				} else {
					act("$n screams in agony!",
						FALSE, (*it), NULL, ch, TO_ROOM);
					act("You cry out in pain as the flames of light consume your body!", 
                        FALSE, ch, NULL, (*it), TO_VICT);
				}
			}
		}
	}

}

ASPELL(spell_inferno)
{
	struct room_affect_data rm_aff;
	struct Creature *vict = NULL;

	send_to_room("A raging firestorm fills the room with a hellish inferno!\r\n",
                    ch->in_room);

	// check for players if caster is not a pkiller
	if (!IS_NPC(ch) && !PRF2_FLAGGED(ch, PRF2_PKILLER)) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if (ch == *it)
				continue;
			if (!is_arena_combat(ch, (*it)) && !IS_NPC((*it))) {
				act("You cannot do this, because this action might cause harm to $N,\r\n"
                    "and you have not chosen to be a Pkiller.\r\n"
                    "You can toggle this with the command 'pkiller'.", 
                    FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
		}
	}

	if (!ROOM_FLAGGED(ch->in_room, ROOM_FLAME_FILLED)) {
		rm_aff.description = str_dup("   The room is ablaze in a hellish inferno of flame!\r\n");
		rm_aff.level = level;
		rm_aff.type = RM_AFF_FLAGS;
		rm_aff.flags = ROOM_FLAME_FILLED;
		rm_aff.duration = level >> 3;
		affect_to_room(ch->in_room, &rm_aff);

	}
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (ch == *it)
			continue;

		if (PRF_FLAGGED((*it), PRF_NOHASSLE))
			continue;

		damage(ch, (*it), dice(level, 6) + (level << 2), TYPE_ABLAZE, -1);

	}

}

ASPELL(spell_banishment)
{

	if (!IS_NPC(victim)) {
		send_to_char(ch, "You cannot banish other players.\r\n");
		return;
	}

	if (IS_DEVIL(victim)) {

		if (ZONE_IS_HELL(victim->in_room->zone)) {
			send_to_char(ch, 
				"You cannot banish devils from their home planes.\r\n");
			return;
		}

		if (mag_savingthrow(victim, level, SAVING_SPELL)) {
			send_to_char(ch, "You fail the banishment.\r\n");
			return;
		}

		act("$n is banished to $s home plane!", FALSE, victim, 0, 0, TO_ROOM);

		victim->die();
//		Event::Queue(new DeathEvent(0, victim, false));

		gain_skill_prof(ch, SPELL_BANISHMENT);

		return;
	}

	act("You cannot banish $N.", FALSE, ch, 0, victim, TO_CHAR);

}

bool
remove_random_obj_affect(Creature *ch, obj_data *obj, int level)
{
	// aff_type : apply:      0 - (MAX_OBJ_AFFECT - 1)
	//            bitfield:   MAX_OBJ_AFFECT + field
	//            weap_spell: MAX_OBJ_AFFECT + 3
	//            sigil:      MAX_OBJ_AFFECT + 4
	int aff_type = -1;
	int bitvec = -1;
	int total_affs = 0;
	int i, spell_num;

	for (i = 0;i < MAX_OBJ_AFFECT;i++)
		if (obj->affected[i].location != APPLY_NONE) {
			if (!number(0, total_affs))
				aff_type = i;
			total_affs++;
		}
	
	for (i = 0;i < 32;i++) {
		if (IS_SET(obj->obj_flags.bitvector[0], (1 << i))) {
			if (!number(0, total_affs)) {
				aff_type = MAX_OBJ_AFFECT;
				bitvec = i;
			}
			total_affs++;
		}
		if (IS_SET(obj->obj_flags.bitvector[1], (1 << i))) {
			if (!number(0, total_affs)) {
				aff_type = MAX_OBJ_AFFECT + 1;
				bitvec = i;
			}
			total_affs++;
		}
		if (IS_SET(obj->obj_flags.bitvector[2], (1 << i))) {
			if (!number(0, total_affs)) {
				aff_type = MAX_OBJ_AFFECT + 2;
				bitvec = i;
			}
			total_affs++;
		}
	}
	
	spell_num = GET_OBJ_VAL(obj, 0);
	if (IS_OBJ_TYPE(obj, ITEM_WEAPON) &&
			IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON) &&
			spell_num > 0 && spell_num < MAX_SPELLS &&
			(SPELL_IS_MAGIC(spell_num) || SPELL_IS_DIVINE(spell_num))) {
		if (!number(0, total_affs))
			aff_type = MAX_OBJ_AFFECT + 3;
	}

	if (GET_OBJ_SIGIL_IDNUM(obj) &&
			(GET_OBJ_SIGIL_IDNUM(obj) == GET_IDNUM(ch) ||
			level > GET_OBJ_SIGIL_LEVEL(obj))) {
		if (!number(0, total_affs))
			aff_type = MAX_OBJ_AFFECT + 4;
	}

	if (!total_affs)
		return true;

	// Now that we've selected the affect we're going to remove, we'll
	// want to be removing it
	if (aff_type < MAX_OBJ_AFFECT) {
		obj->affected[aff_type].location = APPLY_NONE;
		obj->affected[aff_type].modifier = 0;
	} else if (aff_type < MAX_OBJ_AFFECT + 3) {
		REMOVE_BIT(obj->obj_flags.bitvector[aff_type - MAX_OBJ_AFFECT],
			(1 << bitvec));
	} else if (aff_type == MAX_OBJ_AFFECT + 3) {
		REMOVE_BIT(GET_OBJ_EXTRA2(obj), ITEM2_CAST_WEAPON);
	} else if (aff_type == MAX_OBJ_AFFECT + 4) {
		GET_OBJ_SIGIL_IDNUM(obj) = 0;
		GET_OBJ_SIGIL_LEVEL(obj) = 0;
	} else {
		errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}

	return (total_affs - 1 == 0);
}

ASPELL(spell_dispel_magic)
{
    int aff_to_remove;
	bool affs_all_gone;
    bool tmp_affected = false;

    if (victim) {
        // Cast on creature
        if (victim->affected) {
            affected_type *aff, *next_aff;

            for (aff = victim->affected;aff;aff = next_aff) {
                next_aff = aff->next;
                if (SPELL_IS_MAGIC(aff->type) || SPELL_IS_DIVINE(aff->type) ||
                    SPELL_IS_BARD(aff->type)) {
                    if (aff->level < number(level / 2, level * 2))
                        affect_remove(victim, aff);
                }
            }
            send_to_char(victim, "You feel your magic fading away!\r\n");
            act("The magic of $n flows out into the universe.", true,
                victim, 0, 0, TO_ROOM);
        } else {
            send_to_char(ch, "Nothing seems to happen.\r\n");
        }

        return;
    }

    // First things first, remove all temporary affects
    struct tmp_obj_affect *af;
    for (af = obj->tmp_affects; af != NULL; af = obj->tmp_affects) {
        tmp_affected = true;
        obj->removeAffect(af);
    }

    if (tmp_affected && !IS_OBJ_STAT(obj, ITEM_MAGIC)) {
        act("All the magic that $p ever had is gone.", true,
            ch, obj, 0, TO_CHAR);
        return;
    }
    // Cast on object
    if (!IS_OBJ_STAT(obj, ITEM_MAGIC)) {
        act("$p is not magical.", FALSE, ch, obj, 0, TO_CHAR);
        return;
    }

    if (!IS_IMMORT(ch) && 
            (IS_OBJ_STAT(obj, ITEM_MAGIC_NODISPEL) ||
            IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM))) {
        send_to_char(ch, "Nothing seems to happen.\r\n");
        return;
    }

    if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_DAMNED)) {
        act("$p hums for a moment this absorbs your magic!", FALSE,
            ch, obj, 0, TO_CHAR);
        return;
    }

    // removes up to ten affects
    aff_to_remove = 10 - ch->getLevelBonus(IS_MAGE(ch)) / 10;
    if (!aff_to_remove)
        aff_to_remove = 1;
    aff_to_remove += number(0, 1);

    affs_all_gone = false;
    while (aff_to_remove-- && !affs_all_gone)
        affs_all_gone = remove_random_obj_affect(ch, obj, level);

    if (affs_all_gone) {
        if (IS_OBJ_STAT(obj, ITEM_MAGIC))
            REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_MAGIC);

        act("All the magic that $p ever had is gone.", true,
            ch, obj, 0, TO_CHAR);
    } else {
        act("Your spell unravels some of the magic of $p!", true,
            ch, obj, 0, TO_CHAR);
    }
}

ASPELL(spell_distraction)
{
	memory_rec *curr, *next_curr;

	if (!IS_NPC(victim)) {
		send_to_char(ch, "This trigger cannot be used against other players.\r\n");
		return;
	}

	if (!MEMORY(victim) && !victim->isHunting()) {
		send_to_char(ch, "Nothing happens.\r\n");
		return;
	}

	for (curr = MEMORY(victim);curr;curr = next_curr) {
		next_curr = curr->next;
		free(curr);
	}
	MEMORY(victim) = NULL;
	victim->stopHunting();

	gain_skill_prof(ch, SPELL_DISTRACTION);

	act("$N suddenly looks very distracted.", false, ch, 0, victim, TO_CHAR);
	act("You suddenly feel like you're missing something...", false, ch, 0, victim, TO_VICT);
	act("$N suddenly looks very distracted.", false, ch, 0, victim, TO_NOTVICT);
}

ASPELL(spell_bless)
{
	struct affected_type af, af2;
	int i;

	if (!IS_GOOD(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "Your soul is not worthy enough to cast this spell.\n");
		return;
	}

	if (obj) {
		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED)) {
			destroy_object(ch, obj, SPELL_DAMN);
			return;
		}

		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS)
				|| IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {
			send_to_char(ch, NOEFFECT);
			return;
		}

		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if (obj->affected[i].location == APPLY_AC ||
				obj->affected[i].location == APPLY_SAVING_PARA ||
				obj->affected[i].location == APPLY_SAVING_SPELL) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			}
		}

		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
		act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);

		if (GET_LEVEL(ch) >= 37) {
			obj->affected[0].location = APPLY_AC;
			obj->affected[0].modifier =
				-(char)(2 + ((int)(level + GET_WIS(ch)) >> 4));
		}

		if (level > number(35, 53))
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
	} else {
		if (IS_EVIL(victim)) {
			send_to_char(ch, "%s is not worthy of your blessing.\r\n",
				GET_NAME(victim));
			return;
		}
		
		memset(&af, 0, sizeof(affected_type));
		memset(&af2, 0, sizeof(affected_type));
		af.type = SPELL_BLESS;
		af.location = APPLY_HITROLL;
		af.modifier = 2 + (level >> 4);
		af.duration = 6;
        af.owner = ch->getIdNum();
		af2.type = SPELL_BLESS;
		af2.location = APPLY_SAVING_SPELL;
		af2.modifier = -(1 + (level >> 5));
        af2.owner = ch->getIdNum();
		af2.duration = 6;
		affect_join(victim, &af, true, false, false, false);
		affect_join(victim, &af2, true, false, false, false);
		send_to_char(victim, "You feel righteous.\r\n");
		if (ch != victim)
			act("$N briefly glows with a bright blue light!", true,
				ch, 0, victim, TO_CHAR);
		act("You briefly glow with a bright blue light!", true,
			ch, 0, victim, TO_VICT);
		act("$N briefly glows with a bright blue light!", true,
			ch, 0, victim, TO_NOTVICT);
	}

	gain_skill_prof(ch, SPELL_BLESS);
}

ASPELL(spell_calm)
{
    if (!victim) {
        send_to_char(ch, "Something that shouldn't have "
                         "happened just did.\r\n");
        errlog("NULL victim in spell_calm()"); 
        return;
    }

    victim->removeAllCombat();

    return;
}

ASPELL(spell_damn)
{
	struct affected_type af, af2;
	int i;

	if (!IS_EVIL(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch,
			"Your soul is not stained enough to cast this spell.\n");
		return;
	}

	if (obj) {
		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS)) {
			destroy_object(ch, obj, SPELL_DAMN);
			return;
		}

		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED)
				|| IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {
			send_to_char(ch, NOEFFECT);
			return;
		}

		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if (obj->affected[i].location == APPLY_AC ||
				obj->affected[i].location == APPLY_SAVING_PARA ||
				obj->affected[i].location == APPLY_SAVING_SPELL) {
				obj->affected[i].location = APPLY_NONE;
				obj->affected[i].modifier = 0;
			}
		}

		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DAMNED);
		act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);

		if (GET_LEVEL(ch) >= 37) {
			obj->affected[0].location = APPLY_AC;
			obj->affected[0].modifier =
				-(char)(2 + ((int)(level + GET_WIS(ch)) >> 4));
		}

		if (level > number(35, 53))
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
	} else {
		if (IS_EVIL(victim)) {
			if (victim == ch)
				send_to_char(ch, "You are already damned by your own actions.\r\n");
			else
				send_to_char(ch, "%s is already damned by their own actions.\r\n",
					GET_NAME(victim));
			return;
		}
		
		memset(&af, 0, sizeof(affected_type));
		memset(&af2, 0, sizeof(affected_type));
		af.type = SPELL_DAMN;
		af.location = APPLY_HITROLL;
		af.modifier = -(2 + (level >> 4));
		af.duration = 6;
        af.owner = ch->getIdNum();
		af2.type = SPELL_DAMN;
		af2.location = APPLY_SAVING_SPELL;
		af2.modifier = +(1 + (level >> 5));
        af2.owner = ch->getIdNum();
		af2.duration = 6;
		affect_join(victim, &af, true, false, false, false);
		affect_join(victim, &af2, true, false, false, false);
		send_to_char(victim, "You feel terrible.\r\n");

		if (ch != victim)
			act("$N briefly glows with a dark red light!", true,
				ch, 0, victim, TO_CHAR);
		act("You briefly glow with a dark red light!", true,
			ch, 0, victim, TO_VICT);
		act("$N briefly glows with a dark red light!", true,
			ch, 0, victim, TO_NOTVICT);
	}
	gain_skill_prof(ch, SPELL_DAMN);
}

#define TYPE_RODENT		0
#define TYPE_BIRD		1
#define TYPE_REPTILE	2
#define TYPE_BEAST 		3
#define TYPE_PREDATOR	4

Creature *
load_familiar(Creature *ch, int sect_type, int type)
{
	Creature *result;
	const char *to_char = NULL, *to_room = NULL;

	switch (sect_type) {
	case SECT_CITY:
		result = read_mobile(20 + type);
		to_room = "$n emerges from a dirty alley.";
		break;
	case SECT_MOUNTAIN:
		result = read_mobile(30 + type);
		to_char = "$n hops up on a rock and looks at you expectantly.";
		to_room = "$n hops up on a rock and looks at $N expectantly.";
		break;
	case SECT_FIELD:
		result = read_mobile(40 + type);
		to_room = "$n emerges from the tall grasses.";
		break;
	case SECT_FOREST:
		result = read_mobile(50 + type);
		to_room = "$n leaves the cover of the trees.";
		break;
	case SECT_JUNGLE:
		result = read_mobile(60 + type);
		to_room = "$n emerges from the thick jungle undergrowth.";
		break;
	case SECT_SWAMP:
		result = read_mobile(70 + type);
		to_char = "$n carefully steps out of the reeds";
		to_room = "$n carefully steps out of some reeds.";
		break;
	default:
		return NULL;
	}

	if (!result)
		return NULL;

	char_to_room(result, ch->in_room);
	if (to_char)
		act(to_room, false, result, 0, ch, TO_CHAR);
	if (to_room)
		act(to_room, false, result, 0, ch, TO_ROOM);

	return result;
}

bool
perform_call_familiar(Creature *ch, int level, int type)
{
	struct affected_type af;
	struct Creature *pet = NULL;
	struct follow_type *cur_fol;
	int percent;

	// First check to make sure that they don't already have a familiar
	for (cur_fol = ch->followers;cur_fol;cur_fol = cur_fol->next) {
		if (MOB2_FLAGGED(cur_fol->follower, MOB2_FAMILIAR)) {
			send_to_char(ch, NOEFFECT);
			if (ch->in_room == cur_fol->follower->in_room) {
				act("$N looks up at you mournfully.", true,
					ch, 0, cur_fol->follower, TO_CHAR);
				act("You look up at $n mournfully.", true,
					ch, 0, cur_fol->follower, TO_VICT);
				act("$N looks up at $n mournfully.", true,
					ch, 0, cur_fol->follower, TO_NOTVICT);
			}
			return false;
		}
	}

	pet = load_familiar(ch, ch->in_room->sector_type, type);
	if (!pet) {
		send_to_char(ch, NOEFFECT);
		return false;
	}

	SET_BIT(MOB_FLAGS(pet), MOB_PET);
	SET_BIT(MOB2_FLAGS(pet), MOB2_FAMILIAR);

	// Scale the pet to the caster's level
	percent = 50 + ch->getLevelBonus(true) / 2;
	GET_LEVEL(pet) = GET_LEVEL(pet) * percent / 100;
	GET_EXP(pet) = 0;
	GET_MAX_HIT(pet) = GET_MAX_HIT(pet) * percent / 100;
	GET_HIT(pet) = GET_MAX_HIT(pet);
	GET_MAX_MANA(pet) = GET_MAX_MANA(pet) * percent / 100;
	GET_MANA(pet) = GET_MAX_HIT(pet);
	GET_MAX_MOVE(pet) = GET_MAX_MOVE(pet) * percent / 100;
	GET_MOVE(pet) = GET_MAX_HIT(pet);
	GET_HITROLL(pet) = GET_HITROLL(pet) * percent / 100;
	GET_DAMROLL(pet) = GET_DAMROLL(pet) * percent / 100;

	if (pet->master)
		stop_follower(pet);
	add_follower(pet, ch);
	GET_MOB_LEADER(pet) = 1;

	af.type = SPELL_CHARM;
	af.is_instant = 0;
	af.duration = -1;
	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	af.level = level;
    af.owner = ch->getIdNum();
	af.aff_index = 0;
	affect_to_char(pet, &af);

	return true;
}

ASPELL(spell_call_rodent)
{
	if (perform_call_familiar(ch, level, TYPE_RODENT))
		gain_skill_prof(ch, SPELL_CALL_RODENT);
}

ASPELL(spell_call_bird)
{
	if (perform_call_familiar(ch, level, TYPE_BIRD))
		gain_skill_prof(ch, SPELL_CALL_BIRD);
}

ASPELL(spell_call_reptile)
{
	if (perform_call_familiar(ch, level, TYPE_REPTILE))
		gain_skill_prof(ch, SPELL_CALL_REPTILE);
}

ASPELL(spell_call_beast)
{
	if (perform_call_familiar(ch, level, TYPE_BEAST))
		gain_skill_prof(ch, SPELL_CALL_BEAST);
}

ASPELL(spell_call_predator)
{
	if (perform_call_familiar(ch, level, TYPE_PREDATOR))
		gain_skill_prof(ch, SPELL_CALL_PREDATOR);
}
