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
#include "olc.h"
#include "specs.h"
#include "login.h"
#include "matrix.h"
#include "elevators.h"
#include "house.h"
#include "fight.h"

#include "xml_utils.h"
#include "remorter.h"


SPECIAL(remorter)
{
    static Quiz quiz;
    int char_class_choice, i;
    int value,level;
    struct obj_data *obj = NULL, *next_obj = NULL;

    if( spec_mode == SPECIAL_DEATH) return FALSE;
    if (!cmd)
        return 0;

    if (CMD_IS("help")) {
        if(GET_LEVEL(ch) < LVL_IMMORT ) {
            show_char_class_menu(ch->desc);
        } else {
            send_to_char("Valid Commands:\r\n",ch);
            send_to_char("Status - Shows current test state.\r\n",ch);
            send_to_char("Reload - Resets test to waiting state.\r\n",ch);
        }
        return 1;
        
    }
    if( CMD_IS("status") ) {
        quiz.sendStatus(ch);
        return 1;
    }
    if( CMD_IS("reload") && GET_LEVEL(ch) >= LVL_IMMORT ) {
        act("$n conjures a new remort test from thin air!", TRUE, (char_data*)me, 0, 0, TO_ROOM);
        quiz.reset();
        send_to_char("Remort test reset.\r\n",ch);
        return 1;
    }

    if (!CMD_IS("say") && !CMD_IS("'") && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("Use the 'say' command to take the test.\r\n", ch);
        return 1;
    }

    
    if (IS_NPC(ch) || (quiz.inProgress() && !quiz.isStudent(ch)) ||
        GET_LEVEL(ch) >= LVL_IMMORT)
                return 0;


    if (GET_EXP(ch) < exp_scale[LVL_AMBASSADOR] || 
        GET_LEVEL(ch) < (LVL_AMBASSADOR -1)) {
                send_to_char("Piss off.  Come back when you are bigger.\r\n", ch);
                return 1;
    }

    skip_spaces(&argument);

    if (!*argument) {
        if ( quiz.inProgress() ) {
            if( quiz.isStudent(ch) ) {
                send_to_char("Please speak clearly.\r\n", ch);
                quiz.sendQuestion(ch);
            } else {
                send_to_char("Can it scumbag. Someone is trying to concentrate.\r\n",ch);
            }
        } else {
            send_to_char("You must say 'remort' to begin or 'goodbye' to leave.\r\n", ch);
        }
        return 1;
    }
    // ** TEST FINISHED
    if ( quiz.isComplete() ) {
        char_class_choice = parse_char_class(argument);
        if (    char_class_choice == CLASS_UNDEFINED
            ||  char_class_choice >= CLASS_SPARE1 
            ||  char_class_choice == CLASS_WARRIOR
            ||  char_class_choice == CLASS_VAMPIRE ) {
            send_to_char("You must choose one of the following:\r\n", ch);
            show_char_class_menu(ch->desc);  
        } else if (char_class_choice == GET_CLASS(ch) || 
            (GET_CLASS(ch) == CLASS_VAMPIRE && char_class_choice == GET_OLD_CLASS(ch))) {
            send_to_char("You must pick a char_class other than your first char_class.\r\n",ch);

        } else {
            if (GET_CLASS(ch) == CLASS_VAMPIRE) {
                GET_CLASS(ch) = GET_OLD_CLASS(ch);
            }
            GET_REMORT_CLASS(ch) = char_class_choice;

            // Remove all affects
            while (ch->affected)
                affect_remove(ch, ch->affected);
            // Wipe thier skills
            for (i = 1; i <= MAX_SKILLS; i++)
                SET_SKILL(ch, i, 0);

            do_start(ch, FALSE);

            REMOVE_BIT(PRF_FLAGS(ch),   PRF_NOPROJECT | PRF_ROOMFLAGS | PRF_HOLYLIGHT |
                                        PRF_NOHASSLE | PRF_LOG1 | PRF_LOG2 | PRF_NOWIZ);
            REMOVE_BIT(PLR_FLAGS(ch),   PLR_HALT | PLR_INVSTART | PLR_QUESTOR | 
                                        PLR_MORTALIZED | PLR_OLCGOD);

            GET_INVIS_LEV(ch) = 0;
            GET_REMORT_INVIS(ch) = 0;
            GET_COND(ch, DRUNK) = 0;
            GET_COND(ch, FULL) = 0;
            GET_COND(ch, THIRST) = 0;

            // Give em another gen
            if (GET_REMORT_GEN(ch) < 10)
            GET_REMORT_GEN(ch)++;
            // Whack thier remort invis
            GET_REMORT_INVIS(ch) = 0;
            GET_WIMP_LEV(ch) =     0;// wimpy
            GET_TOT_DAM(ch) = 0;     // cyborg damage 

            // Tell everyone that they remorted
            sprintf(buf, "(RTEST) %s has remorted to gen %d as a %s/%s. Score(%d)", GET_NAME(ch), 
            GET_REMORT_GEN(ch), pc_char_class_types[(int)GET_CLASS(ch)], 
            pc_char_class_types[(int)GET_REMORT_CLASS(ch)], quiz.getScore());
            mudlog(buf, BRF, LVL_IMMORT, TRUE);
            REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);

            ch->extract( FALSE );
        }  
        return 1;
    }
    if (! quiz.inProgress() && isname_exact( argument, "goodbye" ) ) {
        room_data *dias = real_room(3001);
        if(dias == NULL) {
            send_to_char("There is nowhere to send you.\r\n",ch);
            return 1;
        } else {
            send_to_char("Very well, coward.\r\n",ch);
            act("$n disappears in a mushroom cloud.", FALSE, ch, 0, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, dias);
            act("$n arrives from a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n has transferred you!", FALSE, (char_data*)me, 0, ch, TO_VICT);
            look_at_room(ch, dias, 0);
            return 1;
       }
    } else if ( isname_exact( argument, "remort" ) ) {
        if (quiz.inProgress()) {
            send_to_char("This test is already in progress.\r\n", ch);
            quiz.sendQuestion(ch);
            return 1;
        } 
    } else if (! quiz.inProgress() ) {
        send_to_char("You must say 'remort' to begin or 'goodbye' to leave.\r\n", ch);
        return 1;
    } 
  
    value = GET_GOLD(ch);

    if (! quiz.inProgress() ) {

        level = MIN(10, 3 + GET_REMORT_GEN(ch));

        if (value < level * 5000000) {
            send_to_char("You do not have sufficient sacrifice to do this.\r\n",ch);
            sprintf(buf, "The required sacrifice must be worth %d coins.\r\n"
                    "You have only brought a %d coin value.\r\n",
                    level * 5000000, value);
            send_to_char(buf, ch);
            return 1;
        }

        value = MIN(level * 5000000, GET_GOLD(ch));
        GET_GOLD(ch) -= value;

        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            extract_obj(obj);
        }
        
        for ( i = 0; i < NUM_WEARS; i++ ) {
            if ( ( obj = GET_EQ( ch, i ) ) ) {
                extract_obj( GET_EQ( ch, i ) );
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
        send_to_char("Your sacrifice has been accepted.\r\n"
                     "You must now answer as many questions as possible.\r\n"
                     "The first word of your answer is the one that counts.\r\n"
                     "Answer me by using say <answer>\r\n"
                     "If you forget the question, type say, without an answer.\r\n"
                     , ch);
        quiz.reset(ch);
        quiz.sendQuestion(ch);
        return 1;
    }

    argument = one_argument(argument, arg1);
    if( quiz.makeGuess(arg1) ) {
        sprintf(buf,"%sThat is correct.%s\r\n",CCBLU(ch,C_NRM),CCNRM(ch,C_NRM));
    } else {
        sprintf(buf,"%sThat is incorrect.%s\r\n",CCRED(ch,C_NRM),CCNRM(ch,C_NRM));
    }
    send_to_char(buf,ch);

    if(! quiz.isComplete() ) {
        quiz.nextQuestion();
        quiz.sendStatus(ch);
        quiz.sendQuestion(ch);
        return 1;
    } else { // *******    TEST COMPLETE.  YAY.
        send_to_char("The test is over.\r\n", ch);
        // Now that the "test is over" remove all those pesky affects.
        if(affected_by_spell(ch,SKILL_EMPOWER)) {
            sprintf(buf,"(RTEST) %s empowered during remort test. Possible abuse.",GET_NAME(ch));
            mudlog(buf, NRM, LVL_DEMI, TRUE);
        }
        while (ch->affected)
            affect_remove(ch, ch->affected);

        if (! quiz.isPassing() ) {
            sprintf(buf, "Your answers were only %d percent correct.\r\n"
                    //"You must be able to answer %d percent correctly.\r\n"
                    "You are unable to remort at this time.\r\n", quiz.getScore() );
            send_to_char(buf, ch);
            sprintf(buf, "(RTEST) %s has failed (%d) remort test.", GET_NAME(ch),value);
            mudlog(buf, NRM, LVL_ELEMENT, TRUE);
            REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);

            // remove all eq and affects here, just in case
            for ( i = 0; i < NUM_WEARS; i++ ) {
                if ( ( obj = GET_EQ( ch, i ) ) ) {
                    extract_obj( GET_EQ( ch, i ) );
                }
            }
            
            while ( ch->affected )
                affect_remove( ch, ch->affected );

            save_char(ch, NULL);
            //extract_char(ch, 0);
            ch->extract( FALSE );
            quiz.reset();
            return 1;
        }
        quiz.reset();
        send_to_char("You have passed the test!\r\n", ch);
        send_to_char("You have succeeded in remortalizing!\r\n"
                     "You may now choose your second char_class for this reincarnation.\r\n"
                     "Choose by speaking the char_class which you desire.", ch);
        show_char_class_menu_past(ch->desc);
        return 1;
    }
}  
  






