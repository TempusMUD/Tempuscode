//
// File: implanter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

list<long> implanter_sessions; // ids of players with implant sessions

void implanter_implant(char_data *me, char_data *ch, char *args);
void implanter_extract(char_data *me, char_data *ch, char *args);
void implanter_redeem(char_data *me, char_data *ch, char *args);
bool implanter_in_session(char_data *ch);
void implanter_end_sess(char_data *me, char_data *ch);
void implanter_show_args(char_data *me, char_data *ch);

const long TICKET_VNUM = 92277;

SPECIAL(implanter)
{
	struct char_data *self = (struct char_data *)me;

	if (spec_mode != SPECIAL_LEAVE && spec_mode != SPECIAL_CMD)
		return 0;
	if (IS_NPC(ch))
		return 0;

	if (spec_mode == SPECIAL_CMD) {
		if (CMD_IS("buy")) {
			skip_spaces(&argument);
			argument = two_arguments(argument, buf, buf2);

			if (!*buf || !*buf2)
				implanter_show_args(self, ch);
			else if (!strcasecmp(buf, "implant"))
				implanter_implant(self, ch, argument);
			else if (!strcasecmp(buf, "extract"))
				implanter_extract(self, ch, argument);
			else
				implanter_show_args(self, ch);
			return 1;
		} else if (CMD_IS("redeem")) {
			skip_spaces(&argument);
			implanter_redeem(self, ch, argument);
			return 1;
		}
		return 0;
	} else if (spec_mode == SPECIAL_LEAVE) {
		implanter_end_sess(self, ch);
		return 0;
	}

	return 0;
}


void
implanter_implant(char_data *me, char_data *ch, char *args)
{
	extern const int wear_bitvectors[];
	struct obj_data *implant = NULL;
	int cost = 0, i, pos = 0;
	bool in_session;

	in_session = implanter_in_session(ch);

	if (!(implant = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
		sprintf(buf1, "You don't seem to be carrying any '%s'.", buf2);
		perform_tell(me, ch, buf1);
		return;
	}
	if (!IS_IMPLANT(implant)) {
		sprintf(buf1, "%s cannot be implanted.  Get a clue.",
			implant->short_description);
		perform_tell(me, ch, buf1);
		return;
	}
	if (*args)
		args = one_argument(args, buf2);
	else
		*buf2 = '\0';

	if (!*buf2) {
		perform_tell(me, ch, "Have it implanted in what position?");
		strcpy(buf, "Valid positions are:\r\n");
		for (i = 0; i < NUM_WEARS; i++)
			sprintf(buf, "%s  %s\r\n", buf, wear_implantpos[i]);
		page_string(ch->desc, buf, 1);
		return;
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
		return;
	}
	if (implant->getWeight() > GET_STR(ch)) {
		sprintf(buf, "That thing is too heavy to implant!");
		perform_tell(me, ch, buf);
		return;
	}


	if (!CAN_WEAR(implant, wear_bitvectors[pos])) {
		sprintf(buf, "%s cannot be implanted there.",
			implant->short_description);
		perform_tell(me, ch, buf);
		send_to_char("It can be implanted: ", ch);
		sprintbit(implant->obj_flags.wear_flags & ~ITEM_WEAR_TAKE,
			wear_bits, buf);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		return;
	}

	if ((IS_OBJ_STAT(implant, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(implant, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
		(IS_OBJ_STAT(implant, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
		act("You are unable to safely utilize $p.", FALSE, ch, implant, 0,
			TO_CHAR);
		return;
	}

	if (IS_OBJ_STAT2(implant, ITEM2_BROKEN)) {
		sprintf(buf1, "%s is broken -- are you some kind of moron?",
			implant->short_description);
		perform_tell(me, ch, buf1);
		return;
	}

	if (GET_IMPLANT(ch, pos)) {
		sprintf(buf1,
			"You are already implanted with %s in that position, dork.",
			GET_IMPLANT(ch, pos)->short_description);
		perform_tell(me, ch, buf1);
		return;
	}

	if (IS_INTERFACE(implant)
		&& INTERFACE_TYPE(implant) == INTERFACE_CHIPS) {
		for (i = 0; i < NUM_WEARS; i++) {
			if ((GET_EQ(ch, i) && IS_INTERFACE(GET_EQ(ch, i)) &&
					INTERFACE_TYPE(GET_EQ(ch, i)) == INTERFACE_CHIPS) ||
				(GET_IMPLANT(ch, i) && IS_INTERFACE(GET_IMPLANT(ch, i)) &&
					INTERFACE_TYPE(GET_IMPLANT(ch, i)) == INTERFACE_CHIPS))
			{
				perform_tell(me, ch,
					"You are already using an interface.\r\n");
				return;
			}
		}
	}

	if (IS_OBJ_STAT2(implant, ITEM2_SINGULAR)) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_IMPLANT(ch, i) != NULL &&
				GET_OBJ_VNUM(GET_IMPLANT(ch, i)) == GET_OBJ_VNUM(implant))
			{
				sprintf(buf1,
					"You'll have to get %s removed if you want that put in.",
					GET_IMPLANT(ch, i)->short_description);
				perform_tell(me, ch, buf1);
				return;
			}
		}
	}

	cost = GET_OBJ_COST(implant);
	if (!IS_CYBORG(ch))
		cost <<= 1;

	if (!in_session && GET_CASH(ch) < cost) {
		sprintf(buf1, "The cost for implanting will be %d credits...  "
			"Which you obviously do not have.", cost);
		perform_tell(me, ch, buf1);
		perform_tell(me, ch, "Take a hike, luser.");
		return;
	}

	if (!in_session && !IS_CYBORG(ch) && GET_LIFE_POINTS(ch) < 1) {
		perform_tell(me, ch,
			"Your body can't take the surgery.\r\n"
			"It will expend one life point.");
		return;
	}

	obj_from_char(implant);
	equip_char(ch, implant, pos, MODE_IMPLANT);

	sprintf(buf, "$n implants $p in your %s.", wear_implantpos[pos]);
	act(buf, FALSE, me, implant, ch, TO_VICT);
	act("$n implants $p in $N.", FALSE, me, implant, ch,
		TO_NOTVICT);

	GET_HIT(ch) = 1;
	GET_MOVE(ch) = 1;
	if (!in_session) {
		GET_CASH(ch) -= cost;
		if (!IS_CYBORG(ch))
			GET_LIFE_POINTS(ch)--;
		WAIT_STATE(ch, 10 RL_SEC);
	} else {
		WAIT_STATE(ch, 2 RL_SEC);
	}
	return;
}

void
implanter_extract(char_data *me, char_data *ch, char *args)
{
	struct obj_data *implant = NULL, *obj = NULL;
	int cost = 0, pos = 0;
	bool in_session;

	in_session = implanter_in_session(ch);

	if (strncmp(buf2, "me", 2) &&
		!(obj = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
		sprintf(buf1, "You don't seem to be carrying any '%s'.", buf2);
		perform_tell(me, ch, buf1);
		return;
	}

	if (obj && !IS_CORPSE(obj) && !OBJ_TYPE(obj, ITEM_DRINKCON) &&
		!IS_BODY_PART(obj) && !isname("head", obj->name)) {
		sprintf(buf1, "I cannot extract anything from %s.",
			obj->short_description);
		perform_tell(me, ch, buf1);
		return;
	}

	if (*args)
		args= two_arguments(args, buf, buf2);
	else {
		*buf = '\0';
		*buf2 = '\0';
	}

	if (!*buf) {
		sprintf(buf1, "Extract what implant from %s?", obj ?
			obj->short_description : "your body");
		perform_tell(me, ch, buf1);
		return;
	}

	if (obj && !(implant = get_obj_in_list_vis(ch, buf, obj->contains))) {
		sprintf(buf2, "There is no '%s' in %s.", buf,
			obj->short_description);
		perform_tell(me, ch, buf2);
		return;
	}

	if (!obj) {
		if (!*buf2) {
			perform_tell(me, ch,
				"Extract an implant from what position?");
			return;
		}
		if ((pos = search_block(buf2, wear_implantpos, 0)) < 0) {
			sprintf(buf1, "'%s' is not a valid implant position, asshole.",
				buf2);
			perform_tell(me, ch, buf1);
			return;
		}
		if (!(implant = GET_IMPLANT(ch, pos))) {
			sprintf(buf1, "You are not implanted with anything at %s.",
				wear_implantpos[pos]);
			perform_tell(me, ch, buf1);
			return;
		}
		if (!isname(buf, implant->name)) {
			sprintf(buf2, "%s is implanted at %s... not '%s'.",
				implant->short_description, wear_implantpos[pos], buf);
			perform_tell(me, ch, buf2);
			return;
		}
	}

	cost = GET_OBJ_COST(implant);
	if (!obj && !IS_CYBORG(ch))
		cost <<= 1;
	if (obj)
		cost >>= 2;

	if (!in_session && GET_CASH(ch) < cost) {
		sprintf(buf1, "The cost for extraction will be %d credits...  "
			"Which you obviously do not have.", cost);
		perform_tell(me, ch, buf1);
		perform_tell(me, ch, "Take a hike, luser.");
		return;
	}

	if (!in_session)
		GET_CASH(ch) -= cost;

	if (!obj) {
		sprintf(buf, "$n extracts $p from your %s.", wear_implantpos[pos]);
		act(buf, FALSE, me, implant, ch, TO_VICT);
		act("$n extracts $p from $N.", FALSE, me, implant, ch,
			TO_NOTVICT);

		obj_to_char((implant = unequip_char(ch, pos, MODE_IMPLANT)), ch);
		SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
		GET_HIT(ch) = 1;
		GET_MOVE(ch) = 1;
		WAIT_STATE(ch, 10 RL_SEC);
		save_char(ch, NULL);
	} else {
		act("$n extracts $p from $P.", FALSE, me, implant, obj,
			TO_ROOM);
		obj_from_obj(implant);
		SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
		obj_to_char(implant, ch);
		save_char(ch, NULL);
	}

	return;
}

void implanter_redeem(char_data *me, char_data *ch, char *args)
{
	if (implanter_in_session(ch)) {
		perform_tell(me, ch, "You've already redeemed your implanting session!");
		return;
	}

	if (!strcasecmp(args, "ticket")) {
		obj_data *obj;

		obj = ch->carrying;
		while (obj && GET_OBJ_VNUM(obj) != TICKET_VNUM)
			obj = obj->next_content;
		if (!obj) {
			perform_tell(me, ch, "You don't have any tickets to redeem!");
			return;
		}
		obj_from_char(obj);
		extract_obj(obj);
	} else if (is_abbrev(args, "qpoint")) {
		if (GET_QUEST_POINTS(ch) <= 0) {
			perform_tell(me, ch, "You don't have any quest points to redeem!");
			return;
		}

		GET_QUEST_POINTS(ch) -= 1;
	} else {
		perform_tell(me, ch, "What do ya wanna redeem?");
		return;
	}

	implanter_sessions.push_back(GET_IDNUM(ch));
	perform_tell(me, ch, "Alright.  So ya got connections.");
	perform_tell(me, ch, "Act like you're buyin' stuff so I won't get in trouble, right?");
	perform_tell(me, ch, "I'll only do this for ya until ya leave.");
}

bool
implanter_in_session(char_data *ch)
{
	if (implanter_sessions.empty())
		return false;

	list<long>::iterator it;

	it = implanter_sessions.begin();
	while (it != implanter_sessions.end())
		if (*it == GET_IDNUM(ch)) {
			return true;
		} else
			it++;
	return false;
}

void implanter_end_sess(char_data *me, char_data *ch)
{
	implanter_sessions.remove(GET_IDNUM(ch));
}

void
implanter_show_args(char_data *me, char_data *ch)
{
	perform_tell(me, ch, "buy implant <implant> <position> or");
	perform_tell(me, ch,
		"buy extract <'me' | object> <implant> [pos]");
	perform_tell(me, ch,
		"redeem < ticket | qpoint >");
	return;
}

