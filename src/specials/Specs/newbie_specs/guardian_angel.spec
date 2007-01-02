#include <list>
#include "accstr.h"

#define ANGEL_DUAL_WIELD		(1 << 0)
#define ANGEL_CONSUME			(1 << 1)
#define ANGEL_DANGER			(1 << 3)
#define ANGEL_LOWPOINTS			(1 << 4)
#define ANGEL_INSTRUCTIONS      (1 << 5)
#define ANGEL_POISONED			(1 << 6)
#define ANGEL_DEATHLYPOISONED	(1 << 6)

ACMD(do_follow);
ACMD(do_rescue);
bool affected_by_spell( struct char_data *ch, byte skill );
int cast_spell(struct Creature *ch, struct Creature *tch, 
               struct obj_data *tobj, int *tdir, int spellnum, int *return_flags);

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
    // Game mechanics
	{ CLASS_NONE, 100, "what hitpoints", "respond Hitpoints are a measure of how much punishment you can take before you die.  When your hitpoints hit zero, you go unconscious and are easily slain." },
	{ CLASS_MAGE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_CLERIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_PHYSIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for altering the laws of reality." },
	{ CLASS_PSIONIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for psionic triggers." },
	{ CLASS_NONE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess." },

	{ CLASS_NONE, 100, "what move", "respond Movepoints are a measure of the amount of physical energy you have.  Moving around and fighting use movepoints." },
	{ CLASS_NONE, 100, "why jail", "respond You probably didn't have enough money to cover your rent, or you attacked one of the cityguards." },
	{ CLASS_NONE, 100, "out jail", "respond To get out of jail, you can use the 'return' command until level 10.  After that, you'll have to find a way out." },
	{ CLASS_NONE, 100, "why naked", "respond Everyone starts out naked here.  You should find things to wear!" },
	{ CLASS_NONE, 100, "where things wear", "respond You can buy equipment or loot clothing from corpses" },
	{ CLASS_NONE, 100, "where get eq", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "who someone", "respond Invisible people are referred to as 'someone.'  You can't tell who they are." },
	{ CLASS_NONE, 100, "lost", "respond You can type 'return' to return to your starting position." },
	{ CLASS_NONE, 100, "cast spells", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "cast spell", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "money", "respond You can get money by looting corpses, selling items, and of course, begging from other players." },
	{ CLASS_NONE, 100, "where eat", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "hungry", "respond You need to get some food to eat." },
	{ CLASS_NONE, 100, "thirsty", "respond You need to get something to drink.  There are fountains to drink from or you can buy drinks at shops." },
	{ CLASS_NONE, 100, "what do i do", "respond Go out and explore!  Talk to people!  Increase in power and wealth!  Meet interesting and exotic creatures and kill them!" },
	{ CLASS_NONE, 100, "where do i go", "respond You can try the 'areas' command to find a likely area, and 'help map' to figure out how to get there.  Or you can ask me for a few places" },

    // Basic information about places
	{ CLASS_NONE, 100, "where get eat", "respond You can find food at various shops, like the bakery" },
	{ CLASS_NONE, 100, "where food", "respond You can find food at various shops, like the bakery" },
	{ CLASS_NONE,  50, "where drink", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where drink", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE,  50, "where water", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where water", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE, 100, "where kill", "respond You should try the cock fighting arena, the training grounds, or the holodeck.  You could try the halflings, but it might be dangerous." },
	{ CLASS_NONE, 100, "where fight", "respond You should try the cock fighting arena, the training grounds, or the holodeck.  You could try the halflings, but it might be dangerous." },
	{ CLASS_NONE, 100, "where buy", "respond You buy things at shops in cities." },
	{ CLASS_NONE, 100, "where level", "respond You should try the cock fighting arena, the training grounds, or the holodeck" },
	{ CLASS_NONE, 100, "where cock", "directions 3047" },
	{ CLASS_NONE, 100, "where train", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where training", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where learn skills", "respond Try \"help guild\"." },

    // Directions to places
	{ CLASS_NONE, 100, "where star plaza", "directions 30013" },
	{ CLASS_NONE, 100, "where holodeck", "directions 2322 then enter rift and go south" },
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
	{ CLASS_NONE, 100, "where doctor", "directions 3013" },

    // Commands
	{ CLASS_NONE, 100, "where get equipment", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "how stop wielding", "respond Type 'remove <item>' to stop wielding it." },
	{ CLASS_NONE, 100, "how stop wearing", "respond Type 'remove <item>' to stop wearing it." },
	{ CLASS_NONE, 100, "how take off", "respond Type 'remove <item>' to take it off." },

    // Chatting
	{ CLASS_NONE, 100, "why you here", "respond To guide you until you no longer need a guide." },
	{ CLASS_NONE, 100, "how long stay", "respond I will stay with you until you reach level 10." },
	{ CLASS_NONE, 100, "what use you", "respond I'm very useful!  I can give directions and hints, and answer questions!" },
	{ CLASS_NONE, 100, "love you", "respond I love you too." },
	{ CLASS_NONE, 100, "me levels", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "how are you", "respond I'm doing ok, thanks for asking!" },
	{ CLASS_NONE, 100, "what are you", "respond I'm a guardian spirit, a protector and advisor to those new to our world." },
	{ CLASS_NONE, 100, "who are you", "respond I'm a guardian spirit, a protector and advisor to those new to our world." },
	{ CLASS_NONE, 100, "help level", "respond No, but I can protect you if you need it." },
	{ CLASS_NONE, 100, "help fight", "respond I will only help you fight when you are in dire need." },
    { CLASS_NONE, 100, "do you eat", "respond No.  I feed on the surrounding spiritual energy called mana." },
	{ CLASS_NONE, 100, "do you food", "respond Sorry.  As a celestial envoy, I have no need for food." },
    { CLASS_NONE, 100, "you made", "respond Thanks for pointing it out." },
    { CLASS_NONE, 100, "your favorite color", "respond My favorite color is purple." },
    { CLASS_NONE, 33, "my name is", "respond Pleased to meet you." },
    { CLASS_NONE, 33, "my name is", "respond Glad to know you." },
    { CLASS_NONE, 100, "my name is", "respond Enchanted, I'm sure." },
	{ CLASS_NONE, 25, "thanks", "respond No problem." },
	{ CLASS_NONE, 25, "thanks", "respond Sure thing." },
	{ CLASS_NONE, 25, "thanks", "respond I'm here to help." },
	{ CLASS_NONE, 100, "thanks", "respond You're welcome." },
	{ CLASS_NONE, 25, "thank you", "respond No problem." },
	{ CLASS_NONE, 25, "thank you", "respond Sure thing." },
	{ CLASS_NONE, 25, "thank you", "respond I'm here to help." },
	{ CLASS_NONE, 100, "thank you", "respond You're welcome." },
	{ CLASS_NONE, 100, "hi", "respond Hi there!  How are you doing today?" },
	{ CLASS_NONE, 100, "hello", "respond Hi!  What's happening?" },
	{ CLASS_NONE, 100, "good morning", "respond Good morning to you, too!" },
	{ CLASS_NONE, 100, "good evening", "respond Good evening to you, too!" },
	{ CLASS_NONE, 100, "whazzup", "respond Watchin the game, havin a bud.  You?" },
	{ CLASS_NONE, 100, "the game a bud", "respond True...  True..." },
	{ CLASS_NONE, 100, "go away", "dismiss" },
	{ CLASS_NONE, 100, "leave me", "dismiss" },
	{ CLASS_NONE, 100, "piss off", "dismiss" },
	{ CLASS_NONE, 100, "fuck off", "dismiss" },

    // Newbie zone information
	{ CLASS_NONE, 100, "tooth", "respond The shark's tooth is valuable treasure that you can sell at a shop." },
	{ CLASS_NONE, 100, "artificial endurance", "respond Artificial endurance is a pill, so you can swallow it." },
	{ CLASS_NONE, 100, "new player", "respond You're a new player?  Wonderful!  I hope you enjoy your stay." },
	{ CLASS_NONE, 100, "level me", "respond No, but I will try to protect you along the way." },

    // Catch-alls
    { CLASS_NONE, 100, "where", "respond Sorry, I don't really know where that might be." },
    { CLASS_NONE, 100, "what", "respond Sorry, I don't really know." },
    { CLASS_NONE, 100, "who", "respond Sorry, I don't really know." },
    { CLASS_NONE, 100, "how", "respond Sorry, I don't really know how." },
    { CLASS_NONE, 100, "why", "respond Sorry, I don't really know why." },

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

void
angel_find_path_to_room(Creature *angel, struct room_data *dest, struct angel_data *data)
{
    int dir = -1, steps = 0;
    struct room_data *cur_room = angel->in_room;

    data->counter = 0;

	if (cur_room == dest) {
		data->action = str_dup("respond We're already here!");
		return;
	}

    acc_string_clear();
	acc_strcat("respond The shortest path there is:", NULL);
    dir = find_first_step(cur_room, dest, STD_TRACK);
    while (cur_room && dir >= 0 && steps < 600) {
        acc_sprintf(" %s", to_dirs[dir]);
        steps++;
        cur_room = ABS_EXIT(cur_room, dir)->to_room;
        dir = find_first_step(cur_room, dest, STD_TRACK);
    }

    if (cur_room != dest || steps >= 600)
        data->action = str_dup("respond I don't seem to be able to find that room.");
	else
		data->action = acc_get_string();
}

void
guardian_angel_action(Creature *angel, const char *action)
{
	angel_data *data = (angel_data *)angel->mob_specials.func_data;
	free(data->action);
	data->action = str_dup(action);
	data->counter = 0;
}

int
angel_do_action(Creature *self, Creature *charge, angel_data *data)
{
	char *cmd, *action;
    int return_flags = 0;
	int result = 0;

	action = data->action;
	cmd = tmp_getword(&action);
	if (!strcmp(cmd, "respond")) {
		perform_tell(self, charge, action);
		result = 1;
	} 
    else if (!strcmp(cmd, "cast")) {
        char *spell = tmp_getword(&action);
        int spell_no = atoi(spell);

        if (spell) {
            perform_say_to(self, charge, angel_spells[spell_no].text);
            cast_spell(self, charge, NULL, NULL, angel_spells[spell_no].spell_no, &return_flags);
            result = 1;
        }
    }
    else if (!strcmp(cmd, "directions")) {
        char *room_num = tmp_getword(&action);
        struct room_data *room = real_room(atoi(room_num));
        if (room) {
            angel_find_path_to_room(self, room, data);
            perform_tell(self, charge, data->action);
        }
        else {
            perform_tell(self, charge, "I don't seem to be able to find that room.");
        }
		result = 1;
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

	guardian_angel_action(self, "none");
	data->counter = -1;
	return result;
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
		perform_say(self, SCMD_YELL, "Banzaiiiii!  To the rescue!");
        do_rescue(self, GET_NAME(charge), 0, 0, 0);
        return 1;
    }
    else if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4 && charge->numCombatants()) {
		if (!IS_SET(data->flags, ANGEL_DANGER)) {
			perform_say(self, SCMD_YELL, "Flee!  Flee for your life!");
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
		perform_say_to(self, charge, "You regenerate hitpoints, mana, and move much faster if you aren't hungry or thirsty.");
		SET_BIT(data->flags, ANGEL_CONSUME);
		return 1;
	}

	if (GET_EQ(charge, WEAR_WIELD_2)) {
		if (!IS_SET(data->flags, ANGEL_DUAL_WIELD)) {
			perform_say_to(self, charge, "It's best not to wield two weapons at once if you don't have the double wield skill.");
			SET_BIT(data->flags, ANGEL_DUAL_WIELD);
			return 1;
		}
	}

	// Check to see if they need to rest
	if (!IS_SET(data->flags, ANGEL_LOWPOINTS)
			&& charge->getPosition() > POS_FIGHTING) {

		if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4) {
			perform_say_to(self, charge, "You're running low on hit points.  Maybe you should rest or sleep to regain them faster.");
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
			return 1;
		} else if (GET_MOVE(charge) < GET_MAX_MOVE(charge) / 4) {
			perform_say_to(self, charge, "You're running low on move points.  Maybe you should rest or sleep to regain them faster.");
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
			return 1;
		} else if (GET_MANA(charge) < GET_MAX_MANA(charge) / 4) {
			perform_say_to(self, charge, "You're running low on mana points.  Maybe you should rest or sleep to regain them faster.");
			SET_BIT(data->flags, ANGEL_LOWPOINTS);
        }
    }

	if (affected_by_spell(charge, SPELL_POISON)) {
		// Charge is poisoned
		if (GET_HIT(charge) < 11) {
			// Charge is about to die
			if (ROOM_FLAGGED(self->in_room, ROOM_NOMAGIC)) {
				perform_say_to(self, charge, "You're about to die from poisoning!  I'll save you!");
				cast_spell(self, charge, NULL, NULL, SPELL_REMOVE_POISON, &return_flags);
			} else if (!IS_SET(data->flags, ANGEL_DEATHLYPOISONED)) {
				perform_say_to(self, charge, "You're about to die from poisoning!  You need to quaff an antidote immediately!");
			}
			data->flags |= ANGEL_POISONED | ANGEL_DEATHLYPOISONED;
		} else if (!IS_SET(data->flags, ANGEL_POISONED)) {
			// Charge isn't about to die, so just warn
			perform_say_to(self, charge, "You're poisoned!  You should try to find an antidote as soon as possible!");
			data->flags |= ANGEL_POISONED;
		}
		return 1;
	} else {
		// Charge isn't poisoned anymore, so clear the flags
		data->flags &= ~(ANGEL_POISONED | ANGEL_DEATHLYPOISONED);
	}

	if (!number(0,60)) {
		// Occasionally throw them a bone
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
				 !SPELL_IS_PHYSICS(angel_spells[x].spell_no))) {
			  guardian_angel_action(self, tmp_sprintf("cast %d", x));
			}
		}
	}

	return 0;
}

void
assign_angel(Creature *angel, Creature *ch)
{
    angel_data *data;

	CREATE(data, angel_data, 1);
    angel->mob_specials.func_data = data;
    data->angel = angel;
    data->charge_id = ch->getIdNum();
    data->counter = -1;
    data->action = str_dup("none");

	// People relate better to others of their own sex
	GET_SEX(angel) = GET_SEX(ch);

	if (angel->in_room)
		char_from_room(angel);
    char_to_room(angel, ch->in_room, false);

    do_follow(angel, GET_NAME(ch), 0, 0, 0);

    angels.push_back(data);
}

void
angel_to_char(Creature *ch) 
{
/*    static const int ANGEL_VNUM = 3038;
    static const int DEMON_VNUM = 3039; */

    static const int ANGEL_VNUM = 70603;
    static const int DEMON_VNUM = 70604;
    Creature *angel;
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

	assign_angel(angel, ch);
}

SPECIAL(guardian_angel)
{
	Creature *self = (Creature *)me;
	angel_data *data = (angel_data *)self->mob_specials.func_data;
	angel_chat_data *cur_chat;
	Creature *charge;
	char *arg;
	const char *word;

	if (spec_mode == SPECIAL_CMD && IS_IMMORT(ch)) {
		if (CMD_IS("status")) {
			send_to_char(ch, "Angel status\r\n------------\r\n");
			if (data) {
				send_to_char(ch, "Charge: %s [%ld]\r\n",
					playerIndex.getName(data->charge_id), data->charge_id);
				send_to_char(ch, "Counter: %d\r\nAction: %s\r\n",
					data->counter, data->action);
			} else {
				send_to_char(ch, "Charge: none\r\n");
			}
			return 1;
		} else if (CMD_IS("beckon") && isname(argument, self->player.name)) {
			assign_angel(self, ch);
			perform_tell(self, ch, "I'm all yours.");
			return 1;
		} else if (CMD_IS("order") &&
				isname(tmp_getword(&argument), self->player.name) &&
				is_abbrev(tmp_getword(&argument), "follow")) {
			charge = get_player_vis(ch, tmp_getword(&argument), false);
			if (charge) {
				assign_angel(self, charge);
				perform_tell(self, ch, "Sure thing, boss.");
			} else {
				perform_tell(self, ch, "Follow who?");
			}
			return 1;
		}
	}

	// Below here only applies with bound angels
	if (!data)
		return 0;
	
	charge = get_char_in_world_by_idnum(data->charge_id);
	if (!charge && data->counter < 0) {
        // When the charge couldn't be found, disappear immediately
		guardian_angel_action(self, "dismiss");
	}

	if (spec_mode == SPECIAL_TICK) {
		if (data->counter > 0)
			data->counter--;
		else if (data->counter < 0)
			return angel_check_charge(self, charge, data);
		else {
			return angel_do_action(self, charge, data);
		}
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	if (ch == charge && CMD_IS("kill") && isname(tmp_getword(&argument), self->player.name)) {
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
			guardian_angel_action(self, cur_chat->response);
			return 0;
		}
	}

	// Nothing matched - log the question and produce a lame response
    slog("ANGEL:  Unknown Question: \"%s\"", argument);
    guardian_angel_action(self, "respond I'm sorry, I don't understand.");

	return 0;
}

