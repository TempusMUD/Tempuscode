//
// File: remorter.cc                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson and John Rothe, all rights reserved.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;
#include <libxml/parser.h>
#include <libxml/tree.h>
// Undefine CHAR to avoid collisions
#undef CHAR
#include <string.h>
#include <stdlib.h>
#include <errno.h>
// Tempus Includes
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "char_class.h"
#include "screen.h"
#include "clan.h"
#include "vehicle.h"
#include "materials.h"
#include "paths.h"
#include "specs.h"
#include "login.h"
#include "matrix.h"
#include "elevators.h"
#include "house.h"
#include "fight.h"

#include "xml_utils.h"
#include "remorter.h"

void Crash_save_implants(struct Creature *ch, bool extract = true);


SPECIAL(remorter)
{
	static Quiz quiz;
	int i;
	int value, level;
	struct obj_data *obj = NULL, *next_obj = NULL;

	if (spec_mode != SPECIAL_CMD)
		return FALSE;
	if (!cmd)
		return 0;

	if (CMD_IS("help") && GET_LEVEL(ch) >= LVL_IMMORT) {
		send_to_char(ch, "Valid Commands:\r\n");
		send_to_char(ch, "Status - Shows current test state.\r\n");
		send_to_char(ch, "Reload - Resets test to waiting state.\r\n");
		return 1;
	}
	if (CMD_IS("status") && GET_LEVEL(ch) >= LVL_IMMORT) {
		quiz.sendStatus(ch);
		return 1;
	}
	if (CMD_IS("reload") && GET_LEVEL(ch) >= LVL_IMMORT) {
		act("$n conjures a new remort test from thin air!", TRUE,
			(Creature *) me, 0, 0, TO_ROOM);
		quiz.reset();
		send_to_char(ch, "Remort test reset.\r\n");
		return 1;
	}

	if (!CMD_IS("say") && !CMD_IS("'") && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "Use the 'say' command to take the test.\r\n"
                	"If you want to leave, just say 'goodbye'.\r\n");
		return 1;
	}


	if (IS_NPC(ch) || (quiz.inProgress() && !quiz.isStudent(ch)) ||
		GET_LEVEL(ch) >= LVL_IMMORT)
		return 0;


	if (GET_EXP(ch) < exp_scale[LVL_AMBASSADOR] ||
		GET_LEVEL(ch) < (LVL_AMBASSADOR - 1)) {
		send_to_char(ch, "Piss off.  Come back when you are bigger.\r\n");
		room_data *room = ch->getLoadroom();
        if( room == NULL )
            room = real_room(3061);// modrian dump
		act("$n disappears in a mushroom cloud.", FALSE, ch, 0, 0,TO_ROOM);
		char_from_room(ch,false);
		char_to_room(ch, room,false);
		act("$n arrives from a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
		act("$n has transferred you!", FALSE, (Creature *) me, 0, ch, TO_VICT);
		look_at_room(ch, room, 0);
		return 1;
	}

	skip_spaces(&argument);

	if (!*argument) {
		if (quiz.inProgress()) {
			if (quiz.isStudent(ch)) {
				send_to_char(ch, "Please speak clearly.\r\n");
				quiz.sendQuestion(ch);
			} else {
				send_to_char(ch,
					"Can it scumbag. Someone is trying to concentrate.\r\n"
                	"If you want to leave, just say 'goodbye'.\r\n");
			}
		} else {
			send_to_char(ch, "You must say 'remort' to begin or 'goodbye' to leave.\r\n");
		}
		return 1;
	}
	if (!quiz.inProgress() && isname_exact(argument, "goodbye")) {
		room_data *room = ch->getLoadroom();
        if( room == NULL )
            room = real_room(3061);// modrian dump

		if (room == NULL) {
			send_to_char(ch, "There is nowhere to send you.\r\n");
			return 1;
		} else {
			send_to_char(ch, "Very well, coward.\r\n");
			act("$n disappears in a mushroom cloud.", FALSE, ch, 0, 0,TO_ROOM);
			char_from_room(ch,false);
			char_to_room(ch, room,false);
			act("$n arrives from a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
			act("$n has transferred you!", FALSE, (Creature *) me, 0, ch, TO_VICT);
			look_at_room(ch, room, 0);
			return 1;
		}
	} else if (isname_exact(argument, "remort")) {
		if (quiz.inProgress()) {
			send_to_char(ch, "This test is already in progress.\r\n");
			quiz.sendQuestion(ch);
			return 1;
		}
	} else if (!quiz.inProgress()) {
		send_to_char(ch, 
			"You must say 'remort' to begin or 'goodbye' to leave.\r\n");
		return 1;
	}

	value = GET_GOLD(ch);

	if (!quiz.inProgress()) {

		level = MIN(10, 3 + GET_REMORT_GEN(ch));

		if (value < level * 5000000) {
			send_to_char(ch, 
				"You do not have sufficient sacrifice to do this.\r\n");
			send_to_char(ch,
				"The required sacrifice must be worth %d coins.\r\n"
				"You have only brought a %d coin value.\r\n", level * 5000000,
				value);
			return 1;
		}

		value = MIN(level * 5000000, GET_GOLD(ch));
		GET_GOLD(ch) -= value;

		for (obj = ch->carrying; obj; obj = next_obj) {
			next_obj = obj->next_content;
			extract_obj(obj);
		}

		for (i = 0; i < NUM_WEARS; i++) {
			if ((obj = GET_EQ(ch, i))) {
				extract_obj(GET_EQ(ch, i));
			}
		}

		while (ch->affected)
			affect_remove(ch, ch->affected);

		for (obj = ch->in_room->contents; obj; obj = next_obj) {
			next_obj = obj->next_content;
			extract_obj(obj);
		}

		if (GET_COND(ch, FULL) >= 0)
			GET_COND(ch, FULL) = 24;
		if (GET_COND(ch, THIRST) >= 0)
			GET_COND(ch, THIRST) = 24;

		SET_BIT(ch->in_room->room_flags, ROOM_NORECALL);
		send_to_char(ch, "Your sacrifice has been accepted.\r\n"
			"You must now answer as many questions as possible.\r\n"
			"The first word of your answer is the one that counts.\r\n"
			"Answer me by using say <answer>\r\n"
			"If you forget the question, type say, without an answer.\r\n");
		quiz.reset(ch);
		quiz.sendQuestion(ch);
		return 1;
	}

	argument = one_argument(argument, arg1);
	if (quiz.makeGuess(ch, arg1)) {
		send_to_char(ch, "%s%sThat is correct.%s\r\n",
			CCBLD(ch, C_NRM), CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
	} else {
		send_to_char(ch, "%sThat is incorrect.%s\r\n",
			CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	}

	if (!quiz.isComplete()) {
		quiz.nextQuestion();
		//quiz.sendStatus(ch);
		quiz.sendQuestion(ch);
		return 1;
	} else {					// *******    TEST COMPLETE.  YAY.
		// remove all eq and affects here, just in case
		for (i = 0; i < NUM_WEARS; i++) {
			if ((obj = GET_EQ(ch, i))) {
				extract_obj(GET_EQ(ch, i));
			}
		}

		while (ch->affected)
			affect_remove(ch, ch->affected);

		// Save the char and its implants but not its eq
		save_char(ch, NULL);

		if (!quiz.isPassing()) {
			send_to_char(ch, "The test is over.\r\n");
			send_to_char(ch, "Your answers were only %d percent correct.\r\n"
				//"You must be able to answer %d percent correctly.\r\n"
				"You are unable to remort at this time.\r\n", quiz.getScore());
			mudlog(LVL_ELEMENT, NRM, false,
				"%s has failed remort test at gen %d.", GET_NAME(ch),
				GET_REMORT_GEN(ch));
			quiz.log(buf);
			quiz.logScore();
			REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);
			quiz.reset();

            room_data *load_room = ch->getLoadroom();
            if( load_room == NULL )
                load_room = real_room(3061);//modrian dumps

            send_to_char(ch, "You have been banished from the chamber!\r\n");
            act("$n is banished from the chamber!", FALSE, ch, 0, 0, TO_ROOM);
            ch->setPosition(POS_RESTING);
            char_from_room(ch,false);
            char_to_room(ch, load_room,false);
            act("$n appears with a bright flash of light!", FALSE, ch, 0, 0, TO_ROOM);

			//ch->extract(false, false, CON_MENU);
			//ch->extract(true, true, CON_MENU);
			return 1;
		} else {
			// Wipe thier skills
			for (i = 1; i <= MAX_SKILLS; i++)
				SET_SKILL(ch, i, 0);

			do_start(ch, FALSE);

			REMOVE_BIT(PRF_FLAGS(ch),
				PRF_NOPROJECT | PRF_ROOMFLAGS | PRF_HOLYLIGHT | PRF_NOHASSLE |
				PRF_LOG1 | PRF_LOG2 | PRF_NOWIZ);
			REMOVE_BIT(PLR_FLAGS(ch),
				PLR_HALT | PLR_INVSTART | PLR_QUESTOR | PLR_MORTALIZED |
				PLR_OLCGOD);

			GET_INVIS_LVL(ch) = 0;
			GET_COND(ch, DRUNK) = 0;
			GET_COND(ch, FULL) = 0;
			GET_COND(ch, THIRST) = 0;

			// Give em another gen
			if (GET_REMORT_GEN(ch) < 10)
				GET_REMORT_GEN(ch)++;
			// Whack thier remort invis
			GET_WIMP_LEV(ch) = 0;	// wimpy
			GET_TOT_DAM(ch) = 0;	// cyborg damage 

			// Tell everyone that they remorted
			mudlog(LVL_IMMORT, BRF, false,
				"%s completed gen %d remort test with score %d",
				GET_NAME(ch), GET_REMORT_GEN(ch),
				quiz.getScore());
			quiz.log(buf);
			quiz.logScore();

			REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);
			quiz.reset();

			// Save the char and its implants but not its eq
			save_char(ch, NULL);
			Crash_crashsave(ch);
			Crash_save_implants(ch);
			ch->extract(true, false, CON_QCLASS_REMORT);

			return 1;
		}
	}
}
