#include <list>

#define ANGEL_DUAL_WIELD		(1 << 0)
#define ANGEL_CONSUME			(1 << 1)
#define ANGEL_DANGER			(1 << 3)
#define ANGEL_LOWPOINTS			(1 << 4)
#define ANGEL_INSTRUCTIONS      (1 << 5)

ACMD(do_follow);
ACMD(do_rescue);
bool affected_by_spell( struct char_data *ch, byte skill );
int cast_spell(struct Creature *ch, struct Creature *tch, 
               struct obj_data *tobj, int *tdir, int spellnum, int *return_flags);
void angel_find_path_to_room(Creature *angel, struct room_data *dest, struct angel_data **data);

struct angel_data {
	Creature *angel;
	long charge_id;	// the player ID of the angel's charge
	char *charge_name;
	int counter;	// a counter before angel does action
	char *action;	// action for angel to do
	unsigned long flags;
};

list<angel_data *> angels;

struct angel_chat_data {
	int char_class;			// class restriction of response
	int chance;			// percent chance this response will be chosen
	char *keywords;		// keywords of response
	char *response;		// the actual response
};

angel_chat_data angel_chat[] = {
	{ CLASS_NONE, 100, "where get something eat", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "where get things eat", "respond You can find food at the bakery" },
	{ CLASS_NONE,  50, "where get drink", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE,  50, "where get water", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where get drink", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE, 100, "where get drink", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE, 100, "where learn spells", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where learn skills", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where star plaza", "directions 30013" },
	{ CLASS_NONE, 100, "where get equipment", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "how stop wielding", "respond Type 'remove <item>' to stop wielding it." },
	{ CLASS_NONE, 100, "how stop wearing", "respond Type 'remove <item>' to stop wearing it." },
	{ CLASS_NONE, 100, "how take off", "respond Type 'remove <item>' to take it off." },
	{ CLASS_NONE, 100, "why you here", "respond To protect your interests, and serve your needs." },
	{ CLASS_NONE, 100, "how long stay", "respond I will stay with you until level 10." },
	{ CLASS_NONE, 100, "where holodeck", "directions 2322 then enter rift and go south" },
	{ CLASS_NONE, 100, "where get food", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "where go fight", "respond You should try the cock fighting arena, the training grounds, or the holodeck.  You could try the halflings, but it might be dangerous." },
	{ CLASS_NONE, 100, "hi", "respond Hi there!  How are you doing today?" },
	{ CLASS_NONE, 100, "hello", "respond Hi!  What's happening?" },
	{ CLASS_NONE, 100, "good morning", "respond Good morning to you, too!" },
	{ CLASS_NONE, 100, "good evening", "respond Good evening to you, too!" },
	{ CLASS_NONE, 100, "go away", "dismiss" },
	{ CLASS_NONE, 100, "leave me", "dismiss" },
	{ CLASS_NONE, 100, "piss off", "dismiss" },
	{ CLASS_NONE, 100, "fuck off", "dismiss" },
	{ CLASS_NONE, 100, "how are you", "respond I'm doing ok, thanks for asking!" },
	{ CLASS_NONE, 100, "who are you", "respond I'm your guardian.  I'm here to help you out!" },
	{ CLASS_NONE, 100, "where buy", "respond You buy things at shops in cities." },
	{ CLASS_NONE, 100, "where level", "respond You should try the cock fighting arena, the training grounds, or the holodeck" },
	{ CLASS_NONE, 100, "where cock", "directions 3047" },
	{ CLASS_NONE, 100, "where train", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where training", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "cast spells", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "cast spell", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "where bakery", "directions 3028" },
	{ CLASS_NONE, 100, "where hs", "directions 3013" },
	{ CLASS_NONE, 100, "where holy square", "directions 3013" },
	{ CLASS_NONE, 100, "where sp", "directions 30013" },
	{ CLASS_NONE, 100, "where halfling", "directions 13325" },
	{ CLASS_NONE, 100, "where halflings", "directions 13325" },
	{ CLASS_NONE, 100, "where aquafitters", "directions 3015" },
	{ CLASS_NONE, 100, "where aquafitter's", "directions 3015" },
	{ CLASS_NONE, 100, "where aquafitter", "directions 3015" },
	{ CLASS_NONE, 100, "where modrian", "directions 3013" },
	{ CLASS_NONE, 100, "where eat", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "why hungry", "respond You need to get some food to eat." },
	{ CLASS_NONE, 100, "why thirsty", "respond You need to get something to drink.  There are fountains to drink from or you can buy drinks at shops." },
	{ CLASS_NONE, 100, "why naked", "respond You should find things to wear!" },
	{ CLASS_NONE, 100, "where things wear", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "where get eq", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "who someone", "respond Invisible people are referred to as 'someone.'  You can't tell who they are." },
	{ CLASS_NONE, 100, "what hitpoints", "respond Hitpoints are a measure of how much punishment you can take before you die.  When your hitpoints hit zero, you go unconscious and are easily slain." },

	{ CLASS_MAGE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_CLERIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_PHYSIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for altering the laws of reality." },
	{ CLASS_PSIONIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for psionic triggers." },
	{ CLASS_NONE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess." },

	{ CLASS_NONE, 100, "what move", "respond Movepoints are a measure of the amount of physical energy you have.  Moving around and fighting use movepoints." },
	{ CLASS_NONE, 100, "lost", "respond You can type 'return' to return to your starting position." },
	{ CLASS_NONE, 100, "level me", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "help me level", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "help me fight", "respond I will only help you fight when you are in dire need." },
	{ CLASS_NONE, 100, "love you", "respond It is fitting that you should." },
	{ CLASS_NONE, 100, "me levels", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "", "" }
};

struct angel_spell_data {
    int spell_no;
    char *text;    
};

angel_spell_data angel_spells[] = {
    { SPELL_DETECT_INVIS, "This spell will allow you to see invisible "
                          "objects and creatures." },
    { SPELL_RETINA, "This spell will allow you to see transparent objects and creatures." },
    { SPELL_ARMOR, "This spell will offer you a small amount of protection." },
    { SPELL_STRENGTH, "This spell will increase your strength." },
    { -1, "" }
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
    int return_flags = 0;

	cmd = tmp_getword(&data->action);
	if (!strcmp(cmd, "respond")) {
		perform_tell(self, charge, data->action);
		return 1;
	} 
    else if (!strcmp(cmd, "cast")) {
        char *spell = tmp_getword(&data->action);
        int spell_no = atoi(spell);

        if (spell) {
            do_say(self, angel_spells[spell_no].text, 0, SCMD_SAY, 0);
            cast_spell(self, charge, NULL, NULL, angel_spells[spell_no].spell_no, &return_flags);
            return 1;
        }
    }
    else if (!strcmp(cmd, "directions")) {
        char *room_num = tmp_getword(&data->action);
        struct room_data *room = real_room(atoi(room_num));
        if (room) {
            angel_find_path_to_room(self, room, &data);
            data->counter = -1;
            perform_tell(self, charge, data->action);
        }
        else {
            data->counter = -1;
            perform_tell(self, charge, "I don't seem to be able to find that room.");
        }
    }
    else if (!strcmp(cmd, "dismiss")) {
		act("$n shrugs $s shoulders and disappears!", false,
			self, 0, 0, TO_ROOM);

        list<angel_data *>::iterator li = angels.begin();
        for (; li != angels.end(); ++li) {
            if (data->charge_id == (*li)->charge_id) {
                // We have to do something after this because we're about to
                // fux0r this iterator.  So, since we already know there can
                // only be one matching entry, let's just end the loop.
                angels.erase(li);
                li = angels.end();
            }
        }
		self->purge(true);
		return 1;
	}
	return 0;
}

int
angel_check_charge(Creature *self, Creature *charge, angel_data *data)
{
    int return_flags = 0;

	if (!charge)
		return 0;

	if (self->in_room != charge->in_room) {
		act("$n disappears in a bright flash of light!", false,
			self, 0, 0, TO_ROOM);
		char_from_room(self);
		char_to_room(self, charge->in_room);
		act("$n appears in a bright flash of light!", false,
			self, 0, 0, TO_ROOM);
        do_follow(self, GET_NAME(charge), 0, 0, 0);
		return 1;
	}

	// First check for mortal danger
    if (GET_HIT(charge) < 15 && charge->numCombatants()) {
        SET_BIT(data->flags, ANGEL_DANGER);
		do_say(self, "Banzaiiiii!  To the rescue!", 0, SCMD_YELL, 0);
        do_rescue(self, GET_NAME(charge), 0, 0, 0);
        return 1;
    }
    else if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4 && charge->numCombatants()) {
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
			do_say(self, "You're running low on mana points.  Maybe you should rest or sleep to regain them faster.", 0, SCMD_SAY, 0);
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
        }
    }

    if (!ROOM_FLAGGED(self->in_room, ROOM_NOMAGIC) &&
		affected_by_spell(charge, SPELL_POISON)) {
	  do_say(self, "You're poisoned!  I'll take care of that for you.", 0, SCMD_SAY, 0);
	  cast_spell(self, charge, NULL, NULL, SPELL_REMOVE_POISON, &return_flags);
	  return 1;
    }

    if (!ROOM_FLAGGED(self->in_room, ROOM_NOMAGIC) &&
		affected_by_spell(charge, SPELL_SICKNESS)) {
	  do_say(self, "You're sick!  This spell should fix you up!.", 0, SCMD_SAY, 0);
	  cast_spell(self, charge, NULL, NULL, SPELL_REMOVE_SICKNESS, &return_flags);
	  return 1;
    }

    for (int x = 0; angel_spells[x].spell_no != -1; x++) {
        if (charge &&
			charge->in_room == self->in_room &&
			can_see_creature(self, charge) &&
			!affected_by_spell(charge, angel_spells[x].spell_no) &&
			(!ROOM_FLAGGED(self->in_room, ROOM_NOMAGIC) ||
			 (!SPELL_IS_MAGIC(angel_spells[x].spell_no) &&
			  !SPELL_IS_DIVINE(angel_spells[x].spell_no))) &&
			(!ROOM_FLAGGED(self->in_room, ROOM_NOPSIONICS) ||
			 !SPELL_IS_PSIONIC(angel_spells[x].spell_no)) &&
			(!ROOM_FLAGGED(self->in_room, ROOM_NOSCIENCE) ||
			 !SPELL_IS_PHYSICS(angel_spells[x].spell_no))
			) {
		  data->action = str_dup(tmp_sprintf("cast %d", x));
		  data->counter = 0;
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
		data->angel = self;
		data->charge_id = 0;
		data->counter = -1;
		data->action = str_dup("none");
	}
	
	charge = get_char_in_world_by_idnum(data->charge_id);
	if (!charge && data->counter < 0) {
        // When the charge couldn't be found, disappear immediately
		data->action = str_dup("dismiss");
        data->counter = 0;
	}

	if (spec_mode == SPECIAL_TICK) {
		if (data->counter > 0)
			data->counter--;
		else if (data->counter < 0)
			return angel_check_charge(self, charge, data);
		else {
			result = angel_do_action(self, charge, data);
			data->counter = -1;
			data->action = str_dup("none");
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

	if (!CMD_IS("ask") && !CMD_IS(">"))
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
    slog("ANGEL:  Unknown Question: \"%s\"", argument);
    data->action = str_dup("respond I'm sorry, I don't understand that question.");
    data->counter = 0;

	return 0;
}

void
angel_to_char(Creature *ch) 
{
/*    static const int ANGEL_VNUM = 3038;
    static const int DEMON_VNUM = 3039; */

    static const int ANGEL_VNUM = 70603;
    static const int DEMON_VNUM = 70604;
    Creature *angel;
    angel_data *data;
    int vnum;
    char *msg;

    list<angel_data *>::iterator li = angels.begin();

    for (; li != angels.end(); ++li) {
        angel_data *tdata = *li;
        if (tdata->charge_id == ch->getIdNum())
            return;
    }

    if (!IS_EVIL(ch)) {
        vnum = ANGEL_VNUM;
        msg = tmp_strdup("A guardian angel has been sent to protect you!\r\n");
    }
    else {
        vnum = DEMON_VNUM;
        msg = tmp_strdup("A guardian demon has been sent to protect you!\r\n");
    }

    if (!(angel = read_mobile(vnum))) {
        slog("ANGEL:  read_mobile() failed for vnum [%d]", vnum);
        return;
    }
    send_to_char(ch, msg);

	CREATE(data, angel_data, 1);
    angel->mob_specials.func_data = data;
    data->angel = angel;
    data->charge_id = ch->getIdNum();
    data->counter = -1;
    data->action = str_dup("none");

    char_to_room(angel, ch->in_room, false);

    do_follow(angel, GET_NAME(ch), 0, 0, 0);

    angels.push_back(data);
}

void
angel_find_path_to_room(Creature *angel, struct room_data *dest, struct angel_data **data)
{
    int dir = -1, steps = 0;
    struct room_data *cur_room = angel->in_room;
    char buf[1024];

    sprintf(buf, "The shortest path there is:");
    dir = find_first_step(cur_room, dest, STD_TRACK);
    while (cur_room && dir >= 0 && steps < 600) {
        strcat(buf, tmp_sprintf(" %s", to_dirs[dir]));
        steps++;
        cur_room = ABS_EXIT(cur_room, dir)->to_room;
        dir = find_first_step(cur_room, dest, STD_TRACK);
    }

    if (cur_room != dest || steps >= 600)
        sprintf(buf, "I don't seem to be able to find that room.");
    else if (cur_room == dest && steps == 0)
        sprintf(buf, "We're already there!");

    (*data)->counter = 0;
    (*data)->action = str_dup(buf);
}
