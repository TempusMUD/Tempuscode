#define ANGEL_DUAL_WIELD		(1 << 0)
#define ANGEL_CONSUME			(1 << 1)
#define ANGEL_DANGER			(1 << 3)
#define ANGEL_LOWPOINTS			(1 << 4)

struct angel_data {
	angel_data *next_angel;
	Creature *angel;
	long charge_id;	// the player ID of the angel's charge
	char *charge_name;
	int counter;	// a counter before angel does action
	char *action;	// action for angel to do
	unsigned long flags;
};

struct angel_chat_data {
	int char_class;			// class restriction of response
	int chance;			// percent chance this response will be chosen
	char *keywords;		// keywords of response
	char *response;		// the actual response
};

angel_chat_data angel_chat[] = {
	{ CLASS_NONE, 100, "hi", "respond Hi there!  How are you doing today?" },
	{ CLASS_NONE, 100, "hello", "respond Hi!  What's happening?" },
	{ CLASS_NONE, 100, "good morning", "respond Good morning to you, too!" },
	{ CLASS_NONE, 100, "good evening", "respond Good evening to you, too!" },
	{ CLASS_NONE, 100, "goodbye", "dismiss" },
	{ CLASS_NONE, 100, "go away", "dismiss" },
	{ CLASS_NONE, 100, "leave me", "dismiss" },
	{ CLASS_NONE, 100, "piss off", "dismiss" },
	{ CLASS_NONE, 100, "how you", "respond I'm doing ok, thanks for asking!" },
	{ CLASS_NONE, 100, "who you", "respond I'm your guardian angel.  I'm here to help you out!" },
	{ CLASS_NONE, 100, "where buy", "respond You buy things at shops in cities." },
	{ CLASS_NONE, 100, "where go fight", "respond You should try the cock fighting arena, the training grounds, or the holodeck" },
	{ CLASS_NONE, 100, "where cock", "directions 3283" },
	{ CLASS_NONE, 100, "where training", "directions 3283" },
	{ CLASS_NONE, 100, "where hs", "directions 3013" },
	{ CLASS_NONE, 100, "where holodeck", "directions 2322 then enter rift and go south" },
	{ CLASS_NONE, 100, "where food", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "where things eat", "respond You can find food at the bakery" },
	{ CLASS_NONE,  50, "where drink", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where drink", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE, 100, "why hungry", "respond You need to get some food to eat." },
	{ CLASS_NONE, 100, "why thirsty", "respond You need to get something to drink.  There are fountains to drink from or you can buy drinks at shops." },
	{ CLASS_NONE, 100, "why naked", "respond You should find things to wear!" },
	{ CLASS_NONE, 100, "where things wear", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "who someone", "respond Invisible people are referred to as 'someone.'  You can't tell who they are." },
	{ CLASS_NONE, 100, "what hitpoints", "respond Hitpoints are a measure of how much punishment you can take before you die.  When your hitpoints hit zero, you go unconscious and are easily slain." },

	{ CLASS_MAGE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_CLERIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_PHYSIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for altering the laws of reality." },
	{ CLASS_PSIONIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for psionic triggers." },
	{ CLASS_NONE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess." },

	{ CLASS_NONE, 100, "what move", "respond Movepoints are a measure of the amount of physical energy you have.  Moving around and fighting use movepoints." },
	{ CLASS_NONE, 100, "lost", "respond You can type 'return' to return to your starting position." },
	{ CLASS_NONE, 100, "how stop wielding", "respond Type 'remove <item>' to stop wielding it." },
	{ CLASS_NONE, 100, "how stop wearing", "respond Type 'remove <item>' to stop wearing it." },
	{ CLASS_NONE, 100, "how take off", "respond Type 'remove <item>' to take it off." },
	{ CLASS_NONE, 100, "", "" }
};

// returns true if all words in keywords can be found, in order, in ref
bool
angel_chat_match(const char *key, const char *ref)
{
	const char *ref_pt, *key_pt;

	ref_pt = ref;
	key_pt = key;

	do {
		if (!*ref_pt)		// didn't get all the words in key
			return false;

		while (*ref_pt && *key_pt
				&& isalpha(*ref_pt)
				&& *key_pt == tolower(*ref_pt)) {
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
angel_check_charge(Creature *self, Creature *charge, angel_data *data)
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

	// First check for mortal danger
	if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4 && charge->numCombatants()) {
		if (!IS_SET(data->flags, ANGEL_DANGER)) {
			do_say(self, "Flee!  Flee for your life!", 0, SCMD_YELL, 0);
			SET_BIT(data->flags, ANGEL_DANGER);
			return 1;
		}
	} else {
		REMOVE_BIT(data->flags, ANGEL_DANGER);
	}

	// Everything below here only applies to not fighting
	if (charge->numCombatants())
		return 0;
	
	if ((!GET_COND(charge, FULL) || !GET_COND(charge, THIRST))
			&& !IS_SET(data->flags, ANGEL_CONSUME)) {
		do_say(self, "You regenerate hitpoints, mana, and move much faster if you aren't hungry or thirsty.", 0, SCMD_SAY, 0);
		SET_BIT(data->flags, ANGEL_CONSUME);
		return 1;
	}

	if (GET_EQ(charge, WEAR_WIELD_2)) {
		if (!IS_SET(data->flags, ANGEL_DUAL_WIELD)) {
			do_say(self, "It's best not to wield two weapons at once if you don't have the double wield skill.", 0, SCMD_SAY, 0);
			SET_BIT(data->flags, ANGEL_DUAL_WIELD);
			return 1;
		}
	}

	// Check to see if they need to rest
	if (!IS_SET(data->flags, ANGEL_LOWPOINTS)
			&& charge->getPosition() > POS_FIGHTING) {

		if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4) {
			do_say(self, "You're running low on hit points.  Maybe you should rest or sleep to regain them faster.", 0, SCMD_SAY, 0);
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
			return 1;
		} else if (GET_MOVE(charge) < GET_MAX_MOVE(charge) / 4) {
			do_say(self, "You're running low on move points.  Maybe you should rest or sleep to regain them faster.", 0, SCMD_SAY, 0);
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
			return 1;
		} else if (GET_MANA(charge) < GET_MAX_MANA(charge) / 4) {
			do_say(self, "You're running low on move points.  Maybe you should rest or sleep to regain them faster.", 0, SCMD_SAY, 0);
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
			return 1;
		}
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
	const char *word;
	int result;

	if (!data) {
		CREATE(data, angel_data, 1);
		self->mob_specials.func_data = data;
		data->next_angel = NULL;
		data->angel = self;
		data->charge_id = 0;
		data->counter = -1;
		data->action = "none";
	}
	
	charge = get_char_in_world_by_idnum(data->charge_id);
	if (!charge && data->counter < 0) {
		data->counter = 20;
		data->action = "dismissed";
	}

	if (spec_mode == SPECIAL_TICK) {
		if (data->counter > 0)
			data->counter--;
		else if (data->counter < 0)
			return angel_check_charge(self, charge, data);
		else {
			result = angel_do_action(self, charge, data);
			data->counter = -1;
			data->action = "none";
			return result;
		}

	}

	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	if (IS_IMMORT(ch)) {
		if (CMD_IS("status")) {
			send_to_char(ch, "Angel status\r\n------------\r\n");
			send_to_char(ch, "Charge: %s [%ld]\r\n",
				playerIndex.getName(data->charge_id), data->charge_id);
			send_to_char(ch, "Counter: %d\r\nAction: %s\r\n",
				data->counter, data->action);
			return 1;
		} else if (CMD_IS("beckon") && isname(argument, self->player.name)) {
			data->charge_id = GET_IDNUM(ch);
			data->charge_name = GET_NAME(ch);
			GET_SEX(self) = GET_SEX(ch);
			perform_tell(self, ch, "I'm all yours, baby.");
			return 1;
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

	if (!CMD_IS("ask")
			&& !CMD_IS(">")
			&& !CMD_IS("say")
			&& !CMD_IS("'"))
		return 0;
	
	if (ch->in_room->people.size() > 2) {
		arg = argument;
		word = arg;
		while (*word) {
			word = tmp_getword(&arg);
			if (isname(word, self->player.name))
				break;
		}
		if (!*word)
			return 0;
	}
	
	// Ok, they said something to us - lets pattern match
	for (cur_chat = angel_chat; *cur_chat->keywords; cur_chat++) {
		if (cur_chat->char_class != CLASS_NONE
				&& cur_chat->char_class != GET_CLASS(charge))
			continue;
		if (cur_chat->chance < 100 && number(0, 99) < cur_chat->chance)
			continue;
		if (angel_chat_match(cur_chat->keywords, argument)) {
			data->action = cur_chat->response;
			data->counter = 0;
			return 0;
		}
	}

	// Nothing matched - log the question and produce a lame response
	

	return 0;
}
