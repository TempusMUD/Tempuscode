//
// File: remorter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_REMORT   (1 << 0)
#define MODE_IMMORT   (1 << 1)
#define MODE_DONE     (1 << 2)

int parse_char_class(char *arg);
void do_start(struct char_data *ch, int mode); 
extern struct remort_question_data *remort_quiz[];
extern int top_of_remort_quiz;

SPECIAL(remorter)
{
    int char_class_choice, i;
    sh_int loop;
    static byte status = 0, level = 0, count = 0;
    static int idnum = 0, index = -1, correct = 0, value = 0;
    struct obj_data *obj = NULL, *next_obj = NULL;
    char arg1[MAX_INPUT_LENGTH];

    if (!cmd)
	return 0;

    if (!CMD_IS("say") && !CMD_IS("'") && GET_LEVEL(ch) < LVL_IMMORT) {
	send_to_char("Use the 'say' command to take the test.\r\n", ch);
	return 0;
    }

    if (IS_NPC(ch) || (idnum && GET_IDNUM(ch) != idnum) ||
	GET_LEVEL(ch) >= LVL_IMMORT)
		return 0;

    if (CMD_IS("help")) {
		show_char_class_menu(ch->desc);
		return 1;
    }

    if (!CMD_IS("say") && !CMD_IS("'") && !CMD_IS(">"))
		return 0;

    if (GET_EXP(ch) < exp_scale[LVL_AMBASSADOR] || 
	GET_LEVEL(ch) < (LVL_AMBASSADOR -1)) {
		send_to_char("Piss off.  Come back when you are bigger.\r\n", ch);
		return 1;
    }

    skip_spaces(&argument);

    if (!*argument) {
		send_to_char("Please speak clearly.\r\n", ch);
		if (status && index) {
			send_to_char(remort_quiz[index]->question, ch);
			send_to_char("\r\n", ch);
		}
		return 1;
    }

    if (IS_SET(status, MODE_DONE)) {
		char_class_choice = parse_char_class(argument);
		if (	char_class_choice == CLASS_UNDEFINED
			|| 	char_class_choice >= CLASS_SPARE1 
			|| 	char_class_choice == CLASS_WARRIOR
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
			do_start(ch, FALSE);
		  
			REMOVE_BIT(PRF_FLAGS(ch), PRF_NOPROJECT | PRF_ROOMFLAGS | PRF_HOLYLIGHT |
				   PRF_NOHASSLE | PRF_LOG1 | PRF_LOG2 | PRF_NOWIZ);
			REMOVE_BIT(PLR_FLAGS(ch), PLR_HALT | PLR_INVSTART | PLR_QUESTOR | 
				   PLR_MORTALIZED | PLR_OLCGOD);
		  
			GET_INVIS_LEV(ch) = 0;
			GET_REMORT_INVIS(ch) = 0;
			GET_COND(ch, DRUNK) = 0;
			GET_COND(ch, FULL) = 0;
			GET_COND(ch, THIRST) = 0;
		
			// If they are no longer a cleric or knight, remove unholy compact.
			
			if ( IS_SOULLESS( ch ) ) {
			    
			    if ( !IS_CLERIC(ch) && !IS_KNIGHT(ch) ) {
				send_to_char("You seem to have lost your soul.\r\n I'll give you a new one.\r\n",ch);
				REMOVE_BIT( PLR2_FLAGS(ch), PLR2_SOULLESS );
			    }
			}

			for (i = 1; i <= MAX_SKILLS; i++)
			SET_SKILL(ch, i, 0);
		  
			if (GET_REMORT_GEN(ch) < 10)
			GET_REMORT_GEN(ch)++;
			GET_REMORT_INVIS(ch) = 0;
			GET_WIMP_LEV(ch) =     0;
			sprintf(buf, "(RTEST) %s has remorted (%d) as a %s/%s.", GET_NAME(ch), 
				GET_REMORT_GEN(ch), pc_char_class_types[(int)GET_CLASS(ch)], 
				pc_char_class_types[(int)GET_REMORT_CLASS(ch)]);
			mudlog(buf, BRF, LVL_IMMORT, TRUE);
			REMOVE_BIT(ch->in_room->room_flags, ROOM_NORECALL);

			extract_char(ch, FALSE);
			status = 0;
			index = -1;
			idnum = 0;
		}  
		return 1;
    }
    
    if ( isname_exact( argument, "remort" ) ) {
	if (status) {
	    send_to_char("One thing at a time.\r\n", ch);
	    send_to_char(remort_quiz[index]->question, ch);
	    send_to_char("\r\n", ch);
	    return 1;
	} else {
	    SET_BIT(status, MODE_REMORT);
	    idnum = GET_IDNUM(ch);
	}
    } else if ( isname_exact(argument, "immort")) {
	if (status) {
	    send_to_char("One thing at a time.\r\n", ch);
	    send_to_char(remort_quiz[index]->question, ch);
	    send_to_char("\r\n", ch);
	    return 1;
	} else {
	    send_to_char("No new immortals are being accepted at this time.\r\n",ch);
	    return 1;
	    SET_BIT(status, MODE_IMMORT);
	    idnum = GET_IDNUM(ch);
	}
    } else if (!status) {
	send_to_char("You must say 'immort' or 'remort' to begin.\r\n", ch);
	return 1;
    }
  
    value = GET_GOLD(ch);

    if (index < 0) {
	/*
	for (i = 0; i < NUM_WEARS; i++) {
	    if ((obj = GET_EQ(ch, i)))
		value += recurs_obj_cost(obj, true, 0);
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
	    next_obj = obj->next_content;
	    value += recurs_obj_cost(obj, true, 0);
	}
	*/

	if (IS_SET(status, MODE_IMMORT))
	    level = 10;
	else
	    level = MIN(10, 3 + GET_REMORT_GEN(ch));

	if (value < level * 5000000) {
	    send_to_char("You do not have sufficient sacrifice to do this.\r\n",ch);
	    sprintf(buf, "The required sacrifice must be worth %d coins.\r\n"
		    "You have only brought a %d coin value.\r\n",
		    level * 5000000, value);
	    send_to_char(buf, ch);
	    status = 0;
	    idnum = 0;
	    return 1;
	}

	value = MIN(level * 5000000, GET_GOLD(ch));
	GET_GOLD(ch) -= value;

	for (obj = ch->carrying; obj; obj = next_obj) {
	    next_obj = obj->next_content;
//	    value += recurs_obj_cost(obj, true, 0);
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

	for (i = 0; i <= top_of_remort_quiz; i++)
	    remort_quiz[i]->asked = 0;
	count =   0;
	correct = 0;
	value =   0;
	index =   number(0, 20);
	send_to_char(remort_quiz[index]->question, ch);
	send_to_char("\r\n", ch);
	return 1;
    }

    argument = one_argument(argument, arg1);
    if ( isname_exact( arg1, remort_quiz[index]->answer ) ) {
	send_to_char("That is correct.\r\n", ch);
	correct++;
    } else
	send_to_char("That is incorrect.\r\n", ch);

    remort_quiz[index]->asked = 1;
    count++;

    loop = 0;
    while (index < 0 || remort_quiz[index]->points > level || 
	   remort_quiz[index]->asked) {
	if ((loop++) >= top_of_remort_quiz) {
	    loop = -1;
	    break;
	}
	index = number(0, top_of_remort_quiz);
    }

    if (count > (level * 6) || count > top_of_remort_quiz || loop < 0) {
	send_to_char("The test is over.\r\n", ch);
	// Now that the "test is over" remove all those pesky affects.
	if(affected_by_spell(ch,SKILL_EMPOWER)) {
		sprintf(buf,"(RTEST) %s empowered during remort test. Possible abuse.",GET_NAME(ch));
	    mudlog(buf, NRM, LVL_DEMI, TRUE);
	}
	while (ch->affected)
	    affect_remove(ch, ch->affected);
	value = correct * 100;
	value /= count;

	if (value < (50 + (level << 2))) {
	    sprintf(buf, "Your answers were only %d percent correct.\r\n"
		    "You must be able to answer %d percent correctly.\r\n"
		    "You are unable to %s at this time.\r\n", value,
		    (50 + (level << 2)), (IS_SET(status, MODE_REMORT) ?
					  "remort" : "immort"));
	    send_to_char(buf, ch);
	    sprintf(buf, "(RTEST) %s has failed (%d) test for %s status.", GET_NAME(ch),
		    value,
		    IS_SET(status, MODE_REMORT) ? "remort" : "immort");
	    mudlog(buf, NRM, LVL_ELEMENT, TRUE);
	    index = -1;
	    status = 0;
	    idnum =  0;
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
	    extract_char(ch, 0);
	    return 1;
	}

	send_to_char("You have passed the test!\r\n", ch);
	if (IS_SET(status, MODE_IMMORT)) {
	    GET_LEVEL(ch) = LVL_IMMORT;
	    sprintf(buf, "(RTEST) immort advance %s, (%d).", GET_NAME(ch), value);
	    slog(buf);
	    advance_level(ch, 0);
	    send_to_char("You are now an immortal of Tempus.\r\n"
			 "You are expected to unhold the ideals of the game with all your power.\r\n"
			 "You must first become familiar your powers and obligations.\r\n"
			 "Type 'wizhelp' to view a list of special commands which you have earned.\r\n"
			 "The first that you use should be 'handbook'.\r\n", ch);
	    index = -1;
	    status = 0;
	    idnum =  0;
	    return 1;
	} else {
	    send_to_char("You have succeeded in remortalizing!\r\n"
			 "You may now choose your second char_class for this reincarnation.\r\n"
			 "Choose by speaking the char_class which you desire.", ch);
	    show_char_class_menu_past(ch->desc);
	    SET_BIT(status, MODE_DONE);
	    return 1;
	}
    }
      
    send_to_char(remort_quiz[index]->question, ch);
    send_to_char("\r\n", ch);
    return 1;
}  
  






