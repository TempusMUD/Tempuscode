//
// File: implanter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(implanter)
{
    extern const int wear_bitvectors[];

    struct char_data *implanter = (struct char_data *) me;
    struct obj_data *implant = NULL, *obj = NULL;
    int cost = 0, i, pos = 0;
    char buf[MAX_STRING_LENGTH], buf2[MAX_INPUT_LENGTH];

    if (!cmd)
	return 0;

    if (!CMD_IS("buy"))
	return 0;

    skip_spaces(&argument);
    argument = two_arguments(argument, buf, buf2);
  
    if (!*buf || !*buf2) {
	perform_tell(implanter, ch, "Buy implant <implant> <position> or");
	perform_tell(implanter, ch, "Buy extract <'me' | object> <implant> [pos]");
	return 1;
    }

    if (!strncmp(buf, "implant", 7)) {
	if (!(implant = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
	    sprintf(buf, "You don't seem to be carrying any '%s'.", buf2);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}
	if (!IS_IMPLANT(implant)) {
	    sprintf(buf, "%s cannot be implanted.  Get a clue.", 
		    implant->short_description);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}
	if (*argument)
	    argument = one_argument(argument, buf2);
	else
	    *buf2 = '\0';

	if (!*buf2) {
	    perform_tell(implanter, ch, "Have it implanted in what position?");
	    strcpy(buf, "Valid positions are:\r\n");
	    for (i = 0; i < NUM_WEARS; i++)
		sprintf(buf, "%s  %s\r\n", buf, wear_implantpos[i]);
	    page_string(ch->desc, buf, 1);
	    return 1;
	}

	if ((pos = search_block(buf2, wear_implantpos, 0)) < 0 ||
	    (ILLEGAL_IMPLANTPOS(pos) && !IS_OBJ_TYPE(implant, ITEM_TOOL))) {
	    sprintf(buf, "'%s' is an invalid position.\r\n", buf2);
	    strcat(buf, "Valid positions are:\r\n");
	    for (i = 0; i < NUM_WEARS; i++) {
		if (!ILLEGAL_IMPLANTPOS(pos))
		    sprintf(buf, "%s  %s\r\n", buf, wear_implantpos[i]);
	    }
	    page_string(ch->desc, buf, 1);
	    return 1;
	}
	if (implant->getWeight() > GET_STR(ch)) {
		sprintf(buf,"That thing is too heavy to implant!");
	    perform_tell(implanter, ch, buf);
		return 1;
	}
	if (!CAN_WEAR(implant, wear_bitvectors[pos])) {
	    sprintf(buf, "%s cannot be implanted there.", 
		    implant->short_description);
	    perform_tell(implanter, ch, buf);
	    send_to_char("It can be implanted: ", ch);
	    sprintbit(implant->obj_flags.wear_flags & ~ITEM_WEAR_TAKE, wear_bits, buf);
	    strcat(buf, "\r\n");
	    send_to_char(buf, ch);
	    return 1;
	}
    
	if ((IS_OBJ_STAT(implant, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
	    (IS_OBJ_STAT(implant, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
	    (IS_OBJ_STAT(implant, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
	    act("You are unable to safely utilize $p.",FALSE,ch,implant,0,TO_CHAR);
	    return 1;
	}
    
	if (IS_OBJ_STAT2(implant, ITEM2_BROKEN)) {
	    sprintf(buf, "%s is broken -- are you some kind of moron?",
		    implant->short_description);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}
      
	if (GET_IMPLANT(ch, pos)) {
	    sprintf(buf, 
		    "You are already implanted with %s in that position, dork.",
		    GET_IMPLANT(ch, pos)->short_description);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}

	if (IS_INTERFACE(implant) && INTERFACE_TYPE(implant) == INTERFACE_CHIPS) {
	    for (i = 0; i < NUM_WEARS; i++) {
		if ((GET_EQ(ch, i) && IS_INTERFACE(GET_EQ(ch, i)) &&
		     INTERFACE_TYPE(GET_EQ(ch, i)) == INTERFACE_CHIPS) ||
		    (GET_IMPLANT(ch, i) && IS_INTERFACE(GET_IMPLANT(ch, i)) &&
		     INTERFACE_TYPE(GET_IMPLANT(ch, i)) == INTERFACE_CHIPS)) {
		    send_to_char("You are already using an interface.\r\n", ch);
		    return 1;
		}
	    }
	}
	    
	cost = GET_OBJ_COST(implant);
	if (!IS_CYBORG(ch))
	    cost <<= 1;

	if (GET_CASH(ch) < cost) {
	    sprintf(buf, "The cost for implanting will be %d credits...  "
		    "Which you obviously do not have.", cost);
	    perform_tell(implanter, ch, buf);
	    perform_tell(implanter, ch, "Take a hike, luser.");
	    return 1;
	}

	if (!IS_CYBORG(ch) && GET_LIFE_POINTS(ch) < 1) {
	    perform_tell(implanter, ch,
			 "Your body can't take the surgery.\r\n"
			 "It will expend one life point.");
	    return 1;
	}
    
	obj_from_char(implant);
	equip_char(ch, implant, pos, MODE_IMPLANT);
    
	sprintf(buf, "$n implants $p in your %s.", wear_implantpos[pos]);
	act(buf, FALSE, implanter, implant, ch, TO_VICT);
	act("$n implants $p in $N.",  FALSE, implanter, implant, ch, TO_NOTVICT);

	GET_CASH(ch) -= cost;
	if (!IS_CYBORG(ch)) {
	    GET_LIFE_POINTS(ch)--;
	}
	GET_HIT(ch) = 1;
	GET_MOVE(ch) = 1;
	WAIT_STATE(ch, 10 RL_SEC);
	return 1;
    }

    if (!strncmp(buf, "extract", 7)) {
	if (strncmp(buf2, "me", 2) &&
	    !(obj = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
	    sprintf(buf, "You don't seem to be carrying any '%s'.", buf2);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}

	if (obj && !IS_CORPSE(obj) && !OBJ_TYPE(obj, ITEM_DRINKCON) &&
	    !IS_BODY_PART(obj) && !isname("head", obj->name)) {
	    sprintf(buf, "I cannot extract anything from %s.", 
		    obj->short_description);
	    perform_tell(implanter, ch, buf);
	    return 1;
	}

	if (*argument)
	    argument = two_arguments(argument, buf, buf2);
	else {
	    *buf = '\0';
	    *buf2 = '\0';
	}

	if (!*buf) {
	    sprintf(buf, "Extract what implant from %s?", obj ? 
		    obj->short_description : "your body");
	    perform_tell(implanter, ch, buf);
	    return 1;
	}

	if (obj && !(implant = get_obj_in_list_vis(ch, buf, obj->contains))) {
	    sprintf(buf2, "There is no '%s' in %s.", buf, obj->short_description);
	    perform_tell(implanter, ch, buf2);
	    return 1;
	}
    
	if (!obj) {
	    if (!*buf2) {
		perform_tell(implanter, ch, "Extract an implant from what position?");
		return 1;
	    }
	    if ((pos = search_block(buf2, wear_implantpos, 0)) < 0) {
		sprintf(buf, "'%s' is not a valid implant position, asshole.", buf2);
		perform_tell(implanter, ch, buf);
		return 1;
	    }
	    if (!(implant = GET_IMPLANT(ch, pos))) {
		sprintf(buf, "You are not implanted with anything at %s.",
			wear_implantpos[pos]);
		perform_tell(implanter, ch, buf);
		return 1;
	    }
	    if (!isname(buf, implant->name)) {
		sprintf(buf2, "%s is implanted at %s... not '%s'.",
			implant->short_description, wear_implantpos[pos], buf);
		perform_tell(implanter, ch, buf2);
		return 1;
	    }
	}

	cost = GET_OBJ_COST(implant);
	if (!obj && !IS_CYBORG(ch))
	    cost <<= 1;
	if (obj)
	    cost >>= 2;
    
	if (GET_CASH(ch) < cost) {
	    sprintf(buf, "The cost for extraction will be %d credits...  "
		    "Which you obviously do not have.", cost);
	    perform_tell(implanter, ch, buf);
	    perform_tell(implanter, ch, "Take a hike, luser.");
	    return 1;
	}

	/*    if (!obj && !IS_CYBORG(ch) && GET_LIFE_POINTS(ch) < 1) {
	      perform_tell(implanter, ch,
	      "Your body can't take the surgery.\r\n"
	      "It will expend one life point.");
	      return 1;
	      }*/
    
	GET_CASH(ch) -= cost;
	if (!obj) {
	    sprintf(buf, "$n extracts $p from your %s.", wear_implantpos[pos]);
	    act(buf, FALSE, implanter, implant, ch, TO_VICT);
	    act("$n extracts $p from $N.",FALSE,implanter, implant,ch,TO_NOTVICT);

	    /*      if (!IS_CYBORG(ch))
		    GET_LIFE_POINTS(ch)--;*/
	    obj_to_char((implant = unequip_char(ch, pos, MODE_IMPLANT)), ch);
	    SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
	    GET_HIT(ch) = 1;
	    GET_MOVE(ch) = 1;
	    WAIT_STATE(ch, 10 RL_SEC);
	    save_char(ch, NULL);
	} else {
	    act("$n extracts $p from $P.",FALSE,implanter,implant,obj,TO_ROOM);
	    obj_from_obj(implant);
	    SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
	    obj_to_char(implant, ch);
	    save_char(ch, NULL);
	}
    
	return 1;
    }
    perform_tell(implanter, ch, "Buy implant <implant> <position> or");
    perform_tell(implanter, ch, "Buy extract <me | object> <implant> [pos]");
    return 1;
}
