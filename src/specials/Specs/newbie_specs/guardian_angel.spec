struct angel_data {
	angel_data *next_angel;
	Creature *angel;
	long charge_id;	// the player ID of the angel's charge
	char *charge_name;	// the name of the angel's charge
	int counter;	// a counter before angel does action
	char *action;	// action for angel to do
};

struct angel_chat_data {
	char *keywords;
	char *response;
};

angel_chat_data angel_chat[] = {
	{ "goodbye", "dismiss" },
	{ "go away", "dismiss" },
	{ "leave me", "dismiss" },
	{ "piss off", "dismiss" },
	{ "how you", "respond I'm doing ok, thanks for asking!" },
	{ "who you", "respond I'm your guardian angel.  I'm here to help you out!" },
	{ "where food", "respond You can find food at the bakery" },
	{ "where drink", "respond You can get some water at the aquafitter's" },
	{ "why i naked", "respond You should find things to wear!" },
	{ "where things wear", "respond You can buy equipment or loot it from corpses" },
	{ "", "" }
};

// returns true if all words in keywords can be found, in order, in ref
bool
chat_match(const char *key, const char *ref)
{
	const char *ref_pt, *key_pt;

	ref_pt = ref;
	key_pt = key;

	do {
		if (!*ref_pt)		// didn't get all the words in key
			return false;

		while (*ref_pt && *key_pt && isalpha(*ref_pt) && *ref_pt == *key_pt) {
			ref_pt++;
			key_pt++;
		}
		if (!isalpha(*ref_pt) && !isalpha(*key_pt)) {
			// word matched - advance key to next word
			while (*key_pt && !isalpha(*key_pt))
				key_pt++;
			key = key_pt;
		} else {
			// word didn't match - advance ref to next word, rewind key
			while (*ref_pt && isalpha(*ref_pt))
				ref_pt++;
			key_pt = key;
		}
		while (*ref_pt && !isalpha(*ref_pt))
			ref_pt++;
	} while (*key);

	return true;
}

int
angel_do_action(Creature *self, Creature *charge, angel_data *data)
{
	char *cmd;

	cmd = tmp_getword(&data->action);
	if (!strcmp(cmd, "respond")) {
		perform_tell(self, charge, data->action);
		return 1;
	} else if (!strcmp(cmd, "dismiss")) {
		act("$n shrugs $s shoulders and disappears!", false,
			self, 0, 0, TO_ROOM);
		self->purge(true);
		return 1;
	}
	return 0;
}

int
angel_check_charge(Creature *self, Creature *charge)
{
	if (!charge)
		return 0;

	if (self->in_room != charge->in_room) {
		act("$n disappears in a bright flash of light!", false,
			self, 0, 0, TO_ROOM);
		char_from_room(self);
		char_to_room(self, charge->in_room);
		act("$n appears in a bright flash of light!", false,
			self, 0, 0, TO_ROOM);
		return 1;
	}

	if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4 && FIGHTING(charge)) {
		do_say(self, "Flee!  Flee for your life!", 0, SCMD_YELL, 0);
		return 1;
	}
	return 0;
}

SPECIAL(guardian_angel)
{
	Creature *self = (Creature *)me;
	angel_data *data = (angel_data *)self->mob_specials.func_data;
	angel_chat_data *cur_chat;
	Creature *charge;
	char *arg;

	if (!data) {
		CREATE(data, angel_data, 1);
		self->mob_specials.func_data = data;
		data->next_angel = NULL;
		data->angel = self;
		data->charge_id = 0;
		data->charge_name = "NONE";
		data->counter = 0;
		data->action = "none";
	}
	
	charge = get_char_in_world_by_idnum(data->charge_id);
	if (!charge && strcmp(data->action, "dismissed")) {
		data->counter = 20;
		data->action = "dismissed";
	}

	if (spec_mode == SPECIAL_TICK) {
		data->counter--;
		if (!data->counter)
			return angel_do_action(self, charge, data);

		return angel_check_charge(self, charge);
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	if (IS_IMMORT(ch)) {
		if (CMD_IS("status")) {
			send_to_char(ch, "Angel status\r\n------------\r\n");
			send_to_char(ch, "Charge: %s [%ld]\r\n",
				data->charge_name, data->charge_id);
			send_to_char(ch, "Counter: %d\r\nAction: %s\r\n",
				data->counter, data->action);
			return 1;
		} else if (CMD_IS("beckon") && isname(argument, self->player.name)) {
			data->charge_id = GET_IDNUM(ch);
			data->charge_name = GET_NAME(ch);
		}
	}

	if (ch != charge)
		return 0;
	if (CMD_IS("kill") && isname(tmp_getword(&argument), self->player.name)) {
		perform_tell(self, charge, "I see you can get along without my help.  Jerk.");
		act("$n disappears in a bright flash of light!",
			false, self, 0, 0, TO_ROOM);
		self->purge(true);
		return 1;
	}

	if (!CMD_IS("tell")
			&& !CMD_IS("ask")
			&& !CMD_IS(">")
			&& !CMD_IS("say")
			&& !CMD_IS("'"))
		return 0;
	
	arg = tmp_getword(&argument);
	if (!isname(arg, self->player.name))
		return 0;
	
	// Ok, they said something to us - lets pattern match
	for (cur_chat = angel_chat; *cur_chat->keywords; cur_chat++) {
		if (chat_match(cur_chat->keywords, argument)) {
			data->action = cur_chat->response;
			data->counter = 1;
			return 0;
		}
	}

	return 0;
}
