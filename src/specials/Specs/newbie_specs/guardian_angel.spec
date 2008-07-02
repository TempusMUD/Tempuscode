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
	bool public_response;
	int respond_to;
	unsigned long flags;
};

list<angel_data *> angels;

struct angel_chat_data {
	int char_class;             // class restriction of response
	int chance;			        // percent chance this response will be chosen
	const char *keywords;		// keywords of response
	const char *response;		// the actual response
};

angel_chat_data angel_chat[] = {
    // Game mechanics
	{ CLASS_NONE, 100, "what hitpoints", "respond Hitpoints are a measure of how much punishment you can take before you die.  When your hitpoints hit zero, you go unconscious and are easily slain." },
	{ CLASS_MAGE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_CLERIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for casting spells." },
	{ CLASS_PHYSIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for altering the laws of reality." },
	{ CLASS_PSIONIC, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess.  You use it for psionic triggers." },
	{ CLASS_NONE, 100, "what mana", "respond Mana is a measure of the amount of psycho-spiritual energy you possess." },
	{ CLASS_NONE, 100, "what move", "respond Movepoints are a measure of the amount of physical energy you have.  Moving around and fighting uses movepoints." },
	{ CLASS_NONE, 100, "what rent", "respond If you quit, you'll loose all your stuff.  Renting allows you to keep your equipment for a fee." },
	{ CLASS_NONE, 100, "why jail", "respond You probably didn't have enough money to cover your rent, or you attacked one of the cityguards." },
	{ CLASS_NONE, 100, "out jail", "respond To get out of jail, you can use the 'return' command until level 10.  After that, you'll have to find a way out." },
	{ CLASS_NONE, 100, "why naked", "respond Everyone starts out naked here.  You should find things to wear!" },
	{ CLASS_NONE, 100, "where things wear", "respond You can buy equipment or loot clothing from corpses" },
	{ CLASS_NONE, 100, "zap", "respond The equipment you're trying to use is differently aligned." },
	{ CLASS_NONE, 100, "zapped", "respond The equipment you're trying to use is differently aligned." },
	{ CLASS_NONE, 100, "burned", "respond The equipment you're trying to use is differently aligned." },
	{ CLASS_NONE, 100, "where get eq", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 50, "where get armor", "respond You could try the leather shop in Modrian." },
	{ CLASS_NONE, 100, "where get armor", "respond You could try Blacksteel's Armoury in Modrian." },
	{ CLASS_NONE, 100, "where get clothes", "respond You could visit Erik the Clothier in Modrian." },
	{ CLASS_NONE, 100, "where get weapon", "respond You should visit the weaponsmith in Modrian." },
	{ CLASS_NONE, 100, "who someone", "respond Invisible people are referred to as 'someone.'  You can't tell who they are." },
	{ CLASS_NONE, 100, "lost", "respond You can type 'return' to return to your starting position." },
	{ CLASS_NONE, 100, "cast spells", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "cast spell", "respond Fear not!  I will cast spells on you as you need them." },
	{ CLASS_NONE, 100, "money", "respond You can get money by looting corpses, selling items, and of course, begging from other players." },
	{ CLASS_NONE, 100, "where eat", "respond You can find food at the bakery" },
	{ CLASS_NONE, 100, "hungry", "respond You need to get some food to eat." },
	{ CLASS_NONE, 100, "thirsty", "respond You need to get something to drink.  There are fountains to drink from or you can buy drinks at shops." },
	{ CLASS_NONE, 100, "what do i do", "respond Go out and explore!  Talk to people!  Increase in power and wealth!  Meet interesting and exotic creatures and kill them!" },
	{ CLASS_NONE, 100, "where do i go", "respond You can try the 'areas' command to find a likely area, and 'help cities' to figure out how to get there." },
	{ CLASS_NONE, 100, "retrieve", "respond You can retrieve your corpse for a fee at a retriever, or you can try and find your corpse yourself." },
	{ CLASS_NONE, 100, "spots blood", "respond Most creatures bleed all over the place when they are injured." },
	{ CLASS_NONE, 100, "enter area", "respond Usually you can just walk in with the directional commands.  Sometimes there's a trick, though." },
	{ CLASS_NONE, 100, "what channels", "respond Channels are public communications you can use to communicate with other players.  You can get a list with the 'toggle' command." },
    
    // Basic information about places
	{ CLASS_NONE, 100, "where get eat", "respond You can find food at various shops, like the bakery" },
	{ CLASS_NONE, 100, "where food", "respond You can find food at various shops, like the bakery" },
	{ CLASS_NONE,  50, "where drink", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where drink", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE,  50, "where water", "respond You can get some water at the aquafitter's" },
	{ CLASS_NONE, 100, "where water", "respond You could drink out of the fountain in a city square." },
	{ CLASS_NONE, 100, "where get equipment", "respond You can buy equipment or loot it from corpses" },
	{ CLASS_NONE, 100, "where get clothes", "respond Armor can be found in armory shops.  You can get clothing at Erik the Clothiers." },
	{ CLASS_NONE, 100, "where get armor", "respond You can loot it from corpses or buy it from an armory." },
	{ CLASS_NONE, 100, "where get weapon", "respond You can loot a weapon from corpses or buy it from a weapon shop." },
	{ CLASS_NONE, 100, "where kill", "respond You should try the cock fighting arena, the training grounds, or the holodeck.  You could try the halflings, but it might be dangerous." },
	{ CLASS_NONE, 100, "where fight", "respond You should try the cock fighting arena, the training grounds, or the holodeck.  You could try the halflings, but it might be dangerous." },
	{ CLASS_NONE, 100, "where buy", "respond You buy things at shops in cities." },
	{ CLASS_NONE, 100, "where level", "respond You should try the cock fighting arena, the training grounds, or the holodeck" },
	{ CLASS_NONE, 100, "where rent", "respond You can rent anywhere there's a receptionist.  There's a place in Modrian called the Soulful Bard Inn." },
	{ CLASS_NONE, 100, "where soulful inn", "directions 3056" },
	{ CLASS_NONE, 100, "where cock", "directions 3047" },
	{ CLASS_NONE, 100, "where train", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where training", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where learn skills", "respond Try \"help guild\"." },
	{ CLASS_NONE, 100, "where am i", "answer whereami" },
	{ CLASS_NONE, 100, "who am i", "answer whoami" },
	{ CLASS_NONE, 100, "which way", "respond Where are you trying to go?" },

    // Directions to places
	{ CLASS_NONE, 100, "where star plaza", "directions 30013" },
	{ CLASS_NONE, 100, "where sp", "directions 30013" },
	{ CLASS_NONE, 100, "where holodeck", "directions 2322 then enter rift and go south" },
    { CLASS_NONE, 100, "where future", "directions 2322 then enter rift" },
	{ CLASS_NONE, 100, "where bakery", "directions 3028" },
	{ CLASS_NONE, 100, "where hs", "directions 3013" },
	{ CLASS_NONE, 100, "where holy square", "directions 3013" },
	{ CLASS_NONE, 100, "where halfling", "directions 13325" },
	{ CLASS_NONE, 100, "where halflings", "directions 13325" },
	{ CLASS_NONE, 100, "where aquafitters", "directions 3015" },
	{ CLASS_NONE, 100, "where aquafitter's", "directions 3015" },
	{ CLASS_NONE, 100, "where aquafitter", "directions 3015" },
	{ CLASS_NONE, 100, "where modrian", "directions 3013" },
	{ CLASS_NONE, 100, "where doctor", "directions 3013" },
    { CLASS_NONE, 100, "where armory", "directions 3043" },
    { CLASS_NONE, 100, "where weapons shop", "directions 3065" },
    { CLASS_NONE, 100, "where erik", "directions 3077" },
    { CLASS_NONE, 100, "where clothier", "directions 3077" },

    // Commands
	{ CLASS_NONE, 100, "how stop wielding", "respond Type 'remove <item>' to stop wielding it." },
	{ CLASS_NONE, 100, "how stop wearing", "respond Type 'remove <item>' to stop wearing it." },
	{ CLASS_NONE, 100, "how take off", "respond Type 'remove <item>' to take it off." },
	{ CLASS_NONE, 100, "how strong is", "respond Type 'consider <creature> to see how hard it would be for you to kill." },

    // Chatting
	{ CLASS_NONE, 100, "why you here", "respond To guide you until you no longer need a guide." },
	{ CLASS_NONE, 100, "how long stay", "respond I will stay with you until you reach level 10." },
	{ CLASS_NONE, 100, "what use you", "respond I'm very useful!  I can give directions and hints, and answer questions!" },
	{ CLASS_NONE, 100, "love you", "respond I love you too." },
	{ CLASS_NONE, 100, "recall", "respond You should be able to type 'return' to to recall until 10th level" },
	{ CLASS_NONE, 100, "me levels", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "how are you", "respond I'm doing ok, thanks for asking!" },
	{ CLASS_NONE, 100, "what are you", "respond I'm a guardian spirit, a protector and advisor to those new to our world." },
	{ CLASS_NONE, 100, "who are you", "respond I'm a guardian spirit, a protector and advisor to those new to our world." },
	{ CLASS_NONE, 100, "what good you", "respond I can be pretty useful if you let me hang around." },
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
	{ CLASS_NONE, 100, "heya", "respond Howdy!  How's it going?" },
	{ CLASS_NONE, 100, "hey", "respond Hello, hello." },
	{ CLASS_NONE, 100, "hiya", "respond Hello!  How are you?" },
	{ CLASS_NONE, 100, "good", "respond That's good to hear." },
	{ CLASS_NONE, 100, "bad", "respond I'm sorry to to hear that." },
	{ CLASS_NONE, 100, "terrible", "respond That's awful!" },
	{ CLASS_NONE, 100, "good morning", "respond Good morning to you, too!" },
	{ CLASS_NONE, 100, "good evening", "respond Good evening to you, too!" },
	{ CLASS_NONE, 100, "whazzup", "respond Watchin the game, havin a bud.  You?" },
	{ CLASS_NONE, 100, "the game a bud", "respond True...  True..." },
	{ CLASS_NONE, 100, "wowza", "respond Zoinks!" },
	{ CLASS_NONE, 100, "drone", "respond I'm not quite a drone, I think." },
	{ CLASS_NONE, 100, "prepare to die", "dismiss" },
	{ CLASS_NONE, 100, "go away", "dismiss" },
	{ CLASS_NONE, 100, "leave", "dismiss" },
	{ CLASS_NONE, 100, "piss off", "dismiss" },
	{ CLASS_NONE, 100, "fuck off", "dismiss" },

    // Newbie zone information
	{ CLASS_NONE, 100, "tooth", "respond The shark's tooth is valuable treasure that you can sell at a shop." },
	{ CLASS_NONE, 100, "artificial endurance", "respond Artificial endurance is a pill, so you can swallow it." },
	{ CLASS_NONE, 100, "new player", "respond You're a new player?  Wonderful!  I hope you enjoy your stay." },
	{ CLASS_NONE, 100, "level me", "respond No, but I will try to protect you along the way." },
	{ CLASS_NONE, 100, "help me", "respond Sure.  How would you like me to help?" },
	{ CLASS_NONE, 100, "give me", "respond I'm not actually carrying anything on me." },
	{ CLASS_NONE, 100, "killed me", "respond Er, sorry about that.  It was purely an accident." },

    // Catch-alls
    { CLASS_NONE, 33, "kill", "respond Sorry, I'm really a pacifist.  I'm just here to help out." },
    { CLASS_NONE, 33, "kill", "respond I'm not the violent type.  I'm just here to help you." },
    { CLASS_NONE, 33, "kill", "respond I'm supposed to help you learn, not do it all for you." },
    { CLASS_NONE, 100, "where", "respond Sorry, I don't really know where that might be." },
    { CLASS_NONE, 100, "what", "respond Sorry, I don't really know." },
    { CLASS_NONE, 100, "who", "respond Sorry, I don't really know." },
    { CLASS_NONE, 100, "how", "respond Sorry, I don't really know how." },
    { CLASS_NONE, 100, "why", "respond Sorry, I don't really know why." },

	{ CLASS_NONE, 100, "", "" }
};

struct angel_spell_data {
    int spell_no;
    const char *text;    
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
		data->action = strdup("We're already here!");
		return;
	}

    acc_string_clear();
	acc_strcat("The shortest path there is:", NULL);
    dir = find_first_step(cur_room, dest, STD_TRACK);
    while (cur_room && dir >= 0 && steps < 600) {
        acc_sprintf(" %s", to_dirs[dir]);
        steps++;
        cur_room = ABS_EXIT(cur_room, dir)->to_room;
        dir = find_first_step(cur_room, dest, STD_TRACK);
    }

    if (cur_room != dest || steps >= 600)
        data->action = strdup("I don't seem to be able to find that room.");
	else
		data->action = acc_get_string();
}

void
guardian_angel_action(Creature *angel, const char *action)
{
	angel_data *data = (angel_data *)angel->mob_specials.func_data;
	free(data->action);
	data->action = strdup(action);
	data->counter = 0;
}

void
angel_do_respond(Creature *self, angel_data *data, const char *message)
{
	Creature *target = get_char_in_world_by_idnum(data->respond_to);

	if (target) {
		if (self->in_room == target->in_room && data->public_response)
			perform_say_to(self, target, message);
		else
			perform_tell(self, target, message);
	}
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
		angel_do_respond(self, data, action);
		result = 1;
	} 
    else if (!strcmp(cmd, "cast")) {
        char *spell = tmp_getword(&action);
        int spell_no = atoi(spell);

		if (GET_IDNUM(charge) != data->respond_to) {
			angel_do_respond(self, data, "Uh, not a chance.");
		} else if (spell) {
			angel_do_respond(self, data, angel_spells[spell_no].text);
            cast_spell(self, charge, NULL, NULL, angel_spells[spell_no].spell_no, &return_flags);
            result = 1;
        }
    }
    else if (!strcmp(cmd, "answer")) {
        char *question = tmp_getword(&action);
        Creature *ch = get_char_in_world_by_idnum(data->respond_to);
        if (!strcmp(question, "whoami"))
            angel_do_respond(self, data,
                             tmp_sprintf("You are %s, %s %s",
                                         GET_NAME(ch),
                                         IS_GOOD(ch) ? "a good":
                                         IS_EVIL(ch) ? "an evil":"a neutral",
                                         strlist_aref(GET_CLASS(ch),
                                                      class_names)));
        else if (!strcmp(question, "whereami"))
            angel_do_respond(self, data,
                             tmp_sprintf("You are in %s.",
                                         ch->in_room->zone->name));
    }
    else if (!strcmp(cmd, "directions")) {
        char *room_num = tmp_getword(&action);
        struct room_data *room = real_room(atoi(room_num));

		if (GET_IDNUM(charge) != data->respond_to) {
			angel_do_respond(self, data, "Find it yourself, eh?");
        } else if (room) {
            angel_find_path_to_room(self, room, data);
			angel_do_respond(self, data, data->action);
        }
        else {
			angel_do_respond(self, data, "I don't seem to be able to find that room.");
        }
		result = 1;
    }
    else if (!strcmp(cmd, "dismiss")) {
		if (charge && GET_IDNUM(charge) != data->respond_to) {
			angel_do_respond(self, data, "Hah.  Piss off.");
			result = 1;
		} else {
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
    if (GET_HIT(charge) < 15 && charge->isFighting()) {
        SET_BIT(data->flags, ANGEL_DANGER);
		perform_say(self, "yell", "Banzaiiiii!  To the rescue!");
        do_rescue(self, GET_NAME(charge), 0, 0, 0);
        return 1;
    }
    else if (GET_HIT(charge) < GET_MAX_HIT(charge) / 4 && charge->isFighting()) {
		if (!IS_SET(data->flags, ANGEL_DANGER)) {
			perform_say(self, "yell", "Flee!  Flee for your life!");
			SET_BIT(data->flags, ANGEL_DANGER);
			return 1;
		}
	} else {
		REMOVE_BIT(data->flags, ANGEL_DANGER);
	}

	// Everything below here only applies to not fighting
	if (charge->isFighting())
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
    data->action = strdup("none");

	// People relate better to others of their own sex
	GET_SEX(angel) = GET_SEX(ch);

	if (angel->in_room)
		char_from_room(angel);
    char_to_room(angel, ch->in_room, false);

    do_follow(angel, GET_NAME(ch), 0, 0, 0);

    angels.push_back(data);
}

SPECIAL(guardian_angel)
{
	ACMD(do_whisper);
	Creature *self = (Creature *)me;
	angel_data *data = (angel_data *)self->mob_specials.func_data;
	angel_chat_data *cur_chat;
	Creature *charge;
	char *arg;

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
	} else if (!data && CMD_IS("beckon")) {
        assign_angel(self, ch);
        perform_say_to(self, ch, "If you ever want me to leave, just let me know.");
        return 1;
    }

	// Below here only applies with bound angels
	if (!data)
		return 0;
	
	charge = get_char_in_world_by_idnum(data->charge_id);
	if (!charge && data->counter < 0) {
        // When the charge couldn't be found, disappear immediately
		guardian_angel_action(self, "dismiss");
		return angel_do_action(self, NULL, data);
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

	if (spec_mode != SPECIAL_CMD || IS_NPC(ch))
		return 0;
	
	if (ch == charge && CMD_IS("kill") && isname(tmp_getword(&argument), self->player.name)) {
		perform_tell(self, charge, "I see you can get along without my help.  Jerk.");
		act("$n disappears in a bright flash of light!",
			false, self, 0, 0, TO_ROOM);
		self->purge(true);
		return 1;
	}

	// Only respond to actual in-room communications
	if (cmd_info[cmd].command_pointer != do_whisper &&
			cmd_info[cmd].command_pointer != do_say)
		return 0;
	
	// Require self alias to be the first word unless they're using
	// the 'say' command and they're the only creature in the room
	arg = argument;
	if (cmd_info[cmd].command_pointer == do_whisper ||
			CMD_IS(">") || CMD_IS("sayto") ||
			ch->in_room->people.size() > 2) {
		if (!isname_exact(tmp_getword(&arg), self->player.name))
			return 0;
	}
	
	data->public_response = (cmd_info[cmd].command_pointer == do_say);
	data->respond_to = GET_IDNUM(ch);

	// Ok, they said something to us - lets pattern match
	for (cur_chat = angel_chat; *cur_chat->keywords; cur_chat++) {
		if (cur_chat->char_class != CLASS_NONE
				&& cur_chat->char_class != GET_CLASS(charge))
			continue;
		if (cur_chat->chance < 100 && number(0, 99) < cur_chat->chance)
			continue;
		if (angel_chat_match(cur_chat->keywords, arg)) {
			slog("ANGEL: \"%s\" -> %s", arg, cur_chat->response);
			guardian_angel_action(self, cur_chat->response);
			return 0;
		}
	}

	slog("ANGEL: unmatched \"%s\"", arg);
	// Nothing matched - log the question and produce a lame response
	guardian_angel_action(self, "respond I'm sorry, I don't understand.");

	return 0;
}
