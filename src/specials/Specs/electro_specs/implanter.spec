//
// File: implanter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

list < long >implanter_sessions;	// ids of players with implant sessions

void implanter_implant(Creature * me, Creature * ch, char *args);
void implanter_extract(Creature * me, Creature * ch, char *args);
void implanter_repair(Creature * me, Creature * ch, char *args);
void implanter_redeem(Creature * me, Creature * ch, char *args);
bool implanter_in_session(Creature * ch);
void implanter_end_sess(Creature * me, Creature * ch);
void implanter_show_args(Creature * me, Creature * ch);
void implanter_show_pos(Creature * me, Creature * ch, obj_data * obj);

const long TICKET_VNUM = 92277;

SPECIAL(implanter)
{
	struct Creature *self = (struct Creature *)me;

	if (spec_mode != SPECIAL_LEAVE && spec_mode != SPECIAL_CMD)
		return 0;
	if (IS_NPC(ch))
		return 0;

	if (spec_mode == SPECIAL_CMD) {
		if (CMD_IS("buy")) {
			char *buy_str;

			buy_str = tmp_getword(&argument);

			if (!*buy_str)
				implanter_show_args(self, ch);
			else if (!strcasecmp(buy_str, "implant"))
				implanter_implant(self, ch, argument);
			else if (!strcasecmp(buy_str, "extract"))
				implanter_extract(self, ch, argument);
			else if (!strcasecmp(buy_str, "repair"))
				implanter_repair(self, ch, argument);
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
implanter_implant(Creature * me, Creature * ch, char *args)
{
	extern const int wear_bitvectors[];
	struct obj_data *implant = NULL;
	char *obj_str, *pos_str, *msg;
	int cost = 0, i, pos = 0;
	bool in_session;

	in_session = implanter_in_session(ch);

	obj_str = tmp_getword(&args);
	pos_str = tmp_getword(&args);

	if (!(implant = get_obj_in_list_vis(ch, obj_str, ch->carrying))) {
		msg = tmp_sprintf("You don't seem to be carrying any '%s'.", obj_str);
		perform_tell(me, ch, msg);
		return;
	}
	if (!IS_IMPLANT(implant)) {
		msg = tmp_sprintf("%s cannot be implanted.  Get a clue.",
			implant->name);
		perform_tell(me, ch, msg);
		return;
	}

	if (!*pos_str) {
		perform_tell(me, ch, "Have it implanted in what position?");
		implanter_show_pos(me, ch, implant);
		return;
	}

	if ((pos = search_block(pos_str, wear_implantpos, 0)) < 0 ||
		(ILLEGAL_IMPLANTPOS(pos) && !IS_OBJ_TYPE(implant, ITEM_TOOL))) {
		msg = tmp_sprintf("'%s' isn't a valid position.", pos_str);
		perform_tell(me, ch, msg);
		implanter_show_pos(me, ch, implant);
		return;
	}
	if (implant->getWeight() > GET_STR(ch)) {
		perform_tell(me, ch, "That thing is too heavy to implant!");
		return;
	}


	if (!CAN_WEAR(implant, wear_bitvectors[pos])) {
		msg = tmp_sprintf("%s cannot be implanted there.",
			implant->name);
		perform_tell(me, ch, msg);
		implanter_show_pos(me, ch, implant);
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
		msg = tmp_sprintf("%s is broken -- are you some kind of moron?",
			implant->name);
		perform_tell(me, ch, msg);
		return;
	}

	if (GET_IMPLANT(ch, pos)) {
		msg =
			tmp_sprintf("You are already implanted with %s in that position.",
			GET_IMPLANT(ch, pos)->name);
		perform_tell(me, ch, msg);
		return;
	}

	if (IS_INTERFACE(implant)
		&& INTERFACE_TYPE(implant) == INTERFACE_CHIPS) {
		for (i = 0; i < NUM_WEARS; i++) {
			if ((GET_EQ(ch, i) && IS_INTERFACE(GET_EQ(ch, i)) &&
					INTERFACE_TYPE(GET_EQ(ch, i)) == INTERFACE_CHIPS) ||
				(GET_IMPLANT(ch, i) && IS_INTERFACE(GET_IMPLANT(ch, i)) &&
					INTERFACE_TYPE(GET_IMPLANT(ch, i)) == INTERFACE_CHIPS)) {
				perform_tell(me, ch,
					"You are already using an interface.\r\n");
				return;
			}
		}
	}

	if (IS_OBJ_STAT2(implant, ITEM2_SINGULAR)) {
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_IMPLANT(ch, i) != NULL &&
				GET_OBJ_VNUM(GET_IMPLANT(ch, i)) == GET_OBJ_VNUM(implant)) {
				msg =
					tmp_sprintf
					("You'll have to get %s removed if you want that put in.",
					GET_IMPLANT(ch, i)->name);
				perform_tell(me, ch, msg);
				return;
			}
		}
	}

	cost = GET_OBJ_COST(implant);
	if (!IS_CYBORG(ch))
		cost <<= 1;

	if (!in_session && GET_CASH(ch) < cost) {
		msg = tmp_sprintf("The cost for implanting will be %d credits...  "
			"Which you obviously do not have.", cost);
		perform_tell(me, ch, msg);
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

	msg = tmp_sprintf("$n implants $p in your %s.", wear_implantpos[pos]);
	act(msg, FALSE, me, implant, ch, TO_VICT);
	act("$n implants $p in $N.", FALSE, me, implant, ch, TO_NOTVICT);

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
implanter_extract(Creature * me, Creature * ch, char *args)
{
	struct obj_data *implant = NULL, *obj = NULL;
	char *targ_str, *obj_str, *pos_str, *msg;
	int cost = 0, pos = 0;
	bool in_session;

	in_session = implanter_in_session(ch);

	targ_str = tmp_getword(&args);
	obj_str = tmp_getword(&args);
	pos_str = tmp_getword(&args);

	if (strncmp(targ_str, "me", 2) &&
		!(obj = get_obj_in_list_vis(ch, targ_str, ch->carrying))) {
		msg = tmp_sprintf("You don't seem to be carrying any '%s'.", targ_str);
		perform_tell(me, ch, msg);
		return;
	}

	if (obj && !IS_CORPSE(obj) && !OBJ_TYPE(obj, ITEM_DRINKCON) &&
		!IS_BODY_PART(obj) && !isname("head", obj->aliases)) {
		msg = tmp_sprintf("I cannot extract anything from %s.",
			obj->name);
		perform_tell(me, ch, msg);
		return;
	}

	if (!*obj_str) {
		msg = tmp_sprintf("Extract what implant from %s?", obj ?
			obj->name : "your body");
		perform_tell(me, ch, msg);
		return;
	}

	if (obj && !(implant = get_obj_in_list_vis(ch, obj_str, obj->contains))) {
		msg = tmp_sprintf("There is no '%s' in %s.", obj_str,
			obj->name);
		perform_tell(me, ch, msg);
		return;
	}

	if (!obj) {
		if (!*pos_str) {
			perform_tell(me, ch, "Extract an implant from what position?");
			return;
		}
		if ((pos = search_block(pos_str, wear_implantpos, 0)) < 0) {
			msg =
				tmp_sprintf("'%s' is not a valid implant position.", pos_str);
			perform_tell(me, ch, msg);
			return;
		}
		if (!(implant = GET_IMPLANT(ch, pos))) {
			msg = tmp_sprintf("You are not implanted with anything at %s.",
				wear_implantpos[pos]);
			perform_tell(me, ch, msg);
			return;
		}
		if (!isname(obj_str, implant->aliases)) {
			msg = tmp_sprintf("%s is implanted at %s... not '%s'.",
				implant->name, wear_implantpos[pos], pos_str);
			perform_tell(me, ch, msg);
			return;
		}
	}

	cost = GET_OBJ_COST(implant);
	if (!obj && !IS_CYBORG(ch))
		cost <<= 1;
	if (obj)
		cost >>= 2;

	if (!in_session && GET_CASH(ch) < cost) {
		msg = tmp_sprintf("The cost for extraction will be %d credits...  "
			"Which you obviously do not have.", cost);
		perform_tell(me, ch, msg);
		perform_tell(me, ch, "Take a hike, luser.");
		return;
	}

	if (!in_session)
		GET_CASH(ch) -= cost;

	if (!obj) {
		msg =
			tmp_sprintf("$n extracts $p from your %s.", wear_implantpos[pos]);
		act(msg, FALSE, me, implant, ch, TO_VICT);
		act("$n extracts $p from $N.", FALSE, me, implant, ch, TO_NOTVICT);

		obj_to_char((implant = unequip_char(ch, pos, MODE_IMPLANT)), ch);
		SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
		GET_HIT(ch) = 1;
		GET_MOVE(ch) = 1;
		WAIT_STATE(ch, 10 RL_SEC);
		ch->saveToXML();
	} else {
		act("$n extracts $p from $P.", FALSE, me, implant, obj, TO_ROOM);
		obj_from_obj(implant);
		SET_BIT(GET_OBJ_WEAR(implant), ITEM_WEAR_TAKE);
		obj_to_char(implant, ch);
		ch->saveToXML();
	}

	return;
}

void
implanter_repair(Creature * me, Creature * ch, char *args)
{
	struct obj_data *implant = NULL, *proto_implant = NULL;
	char *obj_str, *pos_str, *msg;
	int cost = 0, pos = 0;
	bool in_session;

	in_session = implanter_in_session(ch);
	obj_str = tmp_getword(&args);
	pos_str = tmp_getword(&args);

	if (!*obj_str) {
		perform_tell(me, ch, "Repair which implant from your body?");
		return;
	}

	if (!*pos_str) {
		perform_tell(me, ch, "Repair an implant in what position?");
		return;
	}
	if ((pos = search_block(pos_str, wear_implantpos, 0)) < 0) {
		msg = tmp_sprintf("'%s' is not a valid implant position.", pos_str);
		perform_tell(me, ch, msg);
		return;
	}
	if (!(implant = GET_IMPLANT(ch, pos))) {
		msg = tmp_sprintf("You are not implanted with anything at %s.",
			wear_implantpos[pos]);
		perform_tell(me, ch, msg);
		return;
	}
	if (!isname(obj_str, implant->aliases)) {
		msg = tmp_sprintf("%s is implanted at %s... not '%s'.",
			implant->name, wear_implantpos[pos], pos_str);
		perform_tell(me, ch, msg);
		return;
	}

	if (!(proto_implant = real_object_proto(implant->shared->vnum))) {
		perform_tell(me, ch, "No way am I going to repair that.");
		return;
	}

	if (GET_OBJ_DAM(implant) == GET_OBJ_MAX_DAM(implant)) {
		perform_tell(me, ch,
			"Don't waste my time! It's in perfectly good condition.");
		return;
	}

	if (GET_OBJ_MAX_DAM(implant) == 0 ||
		GET_OBJ_MAX_DAM(implant) <= (GET_OBJ_MAX_DAM(proto_implant) >> 4)) {
		msg = tmp_sprintf("Sorry, %s is damaged beyond repair.",
			implant->name);
		perform_tell(me, ch, msg);
		return;
	}
	// implant repairs cost 1.5 the amount of insertion/extraction
	cost = GET_OBJ_COST(implant) + GET_OBJ_COST(implant) >> 1;
	if (!IS_CYBORG(ch))
		cost <<= 1;

	if (!in_session && GET_CASH(ch) < cost) {
		msg = tmp_sprintf("The cost for repair will be %d credits...  "
			"Which you obviously do not have.", cost);
		perform_tell(me, ch, msg);
		perform_tell(me, ch, "Take a hike, luser.");
		return;
	}

	if (!in_session)
		GET_CASH(ch) -= cost;

	act("$n repairs $p.", FALSE, me, implant, ch, TO_VICT);
	act("$n repairs $p inside $N.", FALSE, me, implant, ch, TO_NOTVICT);

	if (IS_OBJ_STAT2(implant, ITEM2_BROKEN)) {
		GET_OBJ_MAX_DAM(implant) -=
			((GET_OBJ_MAX_DAM(implant) * 15) / 100) + 1;
		REMOVE_BIT(GET_OBJ_EXTRA2(implant), ITEM2_BROKEN);
	}
	GET_OBJ_DAM(implant) = GET_OBJ_MAX_DAM(implant);

	GET_HIT(ch) = 1;
	GET_MOVE(ch) = 1;
	WAIT_STATE(ch, 10 RL_SEC);
	ch->saveToXML();

	return;
}

void
implanter_redeem(Creature * me, Creature * ch, char *args)
{
	if (implanter_in_session(ch)) {
		perform_tell(me, ch,
			"You've already redeemed your implanting session!");
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
		if (ch->account->get_quest_points() <= 0) {
			perform_tell(me, ch, "You don't have any quest points to redeem!");
			return;
		}

		ch->account->set_quest_points(ch->account->get_quest_points() - 1);
	} else {
		perform_tell(me, ch, "What do ya wanna redeem?  A ticket or a qpoint?");
		return;
	}

	implanter_sessions.push_back(GET_IDNUM(ch));
	perform_tell(me, ch, "Alright.  So ya got connections.");
	perform_tell(me, ch,
		"Act like you're buyin' stuff so I won't get in trouble, right?");
	perform_tell(me, ch, "I'll only do this for ya until ya leave.");

	mudlog(LVL_AMBASSADOR, BRF, true,
		"Implant session redeemed by %s", GET_NAME(ch));
}

bool
implanter_in_session(Creature * ch)
{
	if (implanter_sessions.empty())
		return false;

	list < long >::iterator it;

	it = implanter_sessions.begin();
	while (it != implanter_sessions.end())
		if (*it == GET_IDNUM(ch)) {
			return true;
		} else
			it++;
	return false;
}

void
implanter_end_sess(Creature * me, Creature * ch)
{
	implanter_sessions.remove(GET_IDNUM(ch));
}

void
implanter_show_args(Creature * me, Creature * ch)
{
	perform_tell(me, ch, "buy implant <implant> <position> or");
	perform_tell(me, ch, "buy extract <'me' | object> <implant> [pos] or");
	perform_tell(me, ch, "buy repair <implant> [pos] or");
	perform_tell(me, ch, "redeem < ticket | qpoint >");
	return;
}

void
implanter_show_pos(Creature * me, Creature * ch, obj_data * obj)
{
	int pos;
	bool not_first = false;

	strcpy(buf, "You can implant it in these positions: ");
	for (pos = 0; wear_implantpos[pos][0] != '\n'; pos++)
		if (!ILLEGAL_IMPLANTPOS(pos) && CAN_WEAR(obj, wear_bitvectors[pos])) {
			if (not_first)
				strcat(buf, ", ");
			else
				not_first = true;

			strcat(buf, wear_implantpos[pos]);
		}

	perform_tell(me, ch, buf);
}
