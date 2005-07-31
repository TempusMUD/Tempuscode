//
// File: login.c -- login sequence functions for TempusMUD
//
// Copyright 1998 John Watson, all rights reserved
//

#include "structs.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "interpreter.h"
#include "utils.h"
#include "login.h"
#include "char_class.h"
#include "creature.h"
#include "help.h"

ACMD(do_hcollect_help);

void show_race_help(struct descriptor_data *d, int race);
int parse_player_class(char *arg);

//
// show_char_class_menu is for choosing a char_class when you remort
//

int
count_ampers(const char *str)
{
	int count = 0;

	while (*str) {
		if (*str == '&')
			count++;
		str++;
	}

	return count;
}

void
show_pc_race_menu(struct descriptor_data *d)
{
	int i, cl;

	cl = GET_CLASS(d->creature) + 1;
	for (i = 0;i < NUM_PC_RACES;i++) {
		if (race_restr[i][cl] != 2)
			continue;
		switch (race_restr[i][0]) {
		case RACE_HUMAN:
			send_to_desc(d,
				"                    &gHuman&n     --  Homo Sapiens\r\n");
			break;
		case RACE_ELF:
			send_to_desc(d,
				"                    &gElf&n       --  Ancient Woodland Race\r\n");
			break;
		case RACE_DWARF:
			send_to_desc(d,
				"                    &gDwarf&n     --  Short, Bearded, Strong\r\n");
			break;
		case RACE_HALF_ORC:
			send_to_desc(d,
				"                    &gHalf Orc&n  --  Mean, Ugly Bastards\r\n");
			break;
		case RACE_HALFLING:
			send_to_desc(d,
				"                    &gHalfling&n  --  Nimble Hole-dwellers\r\n");
			break;
		case RACE_TABAXI:
			send_to_desc(d,
				"                    &gTabaxi&n    --  Lithe Cat-person\r\n");
			break;
		case RACE_DROW:
			send_to_desc(d,
				"                    &gDrow&n      --  Black-hearted dark elves\r\n");
			break;
		case RACE_MINOTAUR:
			send_to_desc(d,
				"                    &gMinotaur&n  --  Powerful Bull-man\r\n");
			break;
		case RACE_ORC:
			send_to_desc(d,
				"                    &gOrc&n       --  Full blooded monsters\r\n");
			break;
		}
	}
}

bool
valid_class_race(Creature *ch, int char_class, bool remort)
{
	int i;

	// We only have to worry about the remort menu
	if (!remort)
		return true;

	if (char_class == GET_CLASS(ch))
		return false;
    
    // restrict any invalid classes
    if( char_class > NUM_CLASSES )
        return false;
    
	for (i = 0;i < NUM_PC_RACES;i++) {
		if (race_restr[i][0] == GET_RACE(ch)) {
            return ( race_restr[i][char_class + 1] == 2 ) ||
                   ( race_restr[i][char_class + 1] == 1 && remort );
        }
    }

	return false;
}

void
show_char_class_menu(struct descriptor_data *d, bool remort)
{
	Creature *ch = d->creature;
	char *left_col, *right_col;
	char *left_line, *right_line;

	left_col = right_col = "";
	if (valid_class_race(ch, CLASS_MAGE, remort))
		left_col = tmp_strcat(left_col,
			"&gMage&n\r\n    Delver in Magical Arts\r\n");
	if (valid_class_race(ch, CLASS_BARB, remort))
		left_col = tmp_strcat(left_col,
			"&gBarbarian&n\r\n    Uncivilized Warrior\r\n");
	if (valid_class_race(ch, CLASS_KNIGHT, remort) &&
			GET_CLASS(ch) != CLASS_MONK)
		left_col = tmp_strcat(left_col,
			"&gKnight&n\r\n    Defender of the Faith\r\n");
	if (valid_class_race(ch, CLASS_RANGER, remort))
		left_col = tmp_strcat(left_col,
			"&gRanger&n\r\n    Roamer of Worlds\r\n");
	if (valid_class_race(ch, CLASS_CLERIC, remort) &&
			GET_CLASS(ch) != CLASS_MONK)
		left_col = tmp_strcat(left_col,
			"&gCleric&n\r\n    Servant of Deity\r\n");
	if (valid_class_race(ch, CLASS_THIEF, remort))
		left_col = tmp_strcat(left_col,
			"&gThief&n\r\n    Stealthy Rogue\r\n");
	if (valid_class_race(ch, CLASS_BARD, remort))
		left_col = tmp_strcat(left_col,
			"&gBard&n\r\n    Roguish Performer\r\n");

	// Print future classes
	if (valid_class_race(ch, CLASS_CYBORG, remort))
		right_col = tmp_strcat(right_col,
			"&gCyborg&n\r\n    The Electronically Advanced\r\n");
	if (valid_class_race(ch, CLASS_PSIONIC, remort))
		right_col = tmp_strcat(right_col,
			"&gPsionic&n\r\n    Mind Traveller\r\n");
	if (valid_class_race(ch, CLASS_MERCENARY, remort))
		right_col = tmp_strcat(right_col,
			"&gMercenary&n\r\n    Gun for Hire\r\n");
	if (valid_class_race(ch, CLASS_PHYSIC, remort))
		right_col = tmp_strcat(right_col,
			"&gPhysic&n\r\n    Alterer of Universal Laws\r\n");
	if (valid_class_race(ch, CLASS_MONK, remort) &&
			GET_CLASS(ch) != CLASS_KNIGHT && GET_CLASS(ch) != CLASS_CLERIC)
		right_col = tmp_strcat(right_col,
			"&gMonk&n\r\n    Philosophical Warrior\r\n");
	do {
		left_line = tmp_getline(&left_col);
		if (!left_line)
			left_line = "";
		right_line = tmp_getline(&right_col);
		if (!right_line)
			right_line = "";
		send_to_desc(d, " %s%s%s\r\n",
			left_line,
			tmp_pad(' ', 39 + (count_ampers(left_line) * 2) - strlen(left_line)),
			right_line);
	} while (*left_line || *right_line);

	send_to_desc(d, "\r\n");
}

int
parse_pc_race(struct descriptor_data *d, char *arg)
{
	int idx, last_match;

	skip_spaces(&arg);

	last_match = -1;
	for (idx = 0;idx < NUM_PC_RACES;idx++) {
		if (!is_abbrev(arg, player_race[(int)race_restr[idx][0]]))
			continue;
		last_match = race_restr[idx][0];
		if (race_restr[idx][GET_CLASS(d->creature)+1] != 2)
			continue;
		return race_restr[idx][0];
	}
	
	return last_match;
}

void
show_pc_race_help(descriptor_data *d, char *arg)
{
	char *race_str;

	race_str = tmp_getword(&arg);	// throw away
	race_str = tmp_getword(&arg);	// actual race
	
	if (!*race_str) {
		send_to_desc(d,
			"\r\n&gTry specifying the race you want information on.&n\r\n\r\n");
		return;
	}

	if (parse_pc_race(d, race_str) == RACE_UNDEFINED) {
		send_to_desc(d, "\r\n&rThat's not a character race!&n\r\n");
		return;
	}

	do_hcollect_help(d->creature, race_str, 0, 0, NULL);
	set_desc_state(CXN_RACE_HELP, d);
}

void
show_pc_class_help(descriptor_data *d, char *arg)
{
	char *class_str;

	class_str = tmp_getword(&arg);	// throw away
	class_str = tmp_getword(&arg);	// actual class
	
	if (!*class_str) {
		send_to_desc(d,
			"\r\n&gTry specifying the class you want information on.&n\r\n");
		return;
	}

	if (parse_player_class(class_str) == CLASS_UNDEFINED) {
		send_to_desc(d, "\r\n&rThat's not a character class!&n\r\n\r\n");
		return;
	}

	do_hcollect_help(d->creature, class_str, 0, 0, NULL);
	set_desc_state(CXN_CLASS_HELP, d);
}

void
show_race_help(struct descriptor_data *d, int race)
{

	SEND_TO_Q("\r\n", d);
	switch (race) {
	case RACE_HUMAN:
		SEND_TO_Q("Heh, do you need help?\r\n", d);
		break;
	case RACE_ELF:
		SEND_TO_Q
			("   Elves are an agile, intellegent race which lives in close\r\n"
			"harmony with the natural world.  They are very long lived, often\r\n"
			"exceeding a thousand years in age.  Elves are especially adept\r\n"
			"with bows and swords, resistant to Charm and Sleep spells, and\r\n"
			"have natural night vision.  Elves, however, are hated by the\r\n"
			"evil humanoid races such as the orcs and goblins.\r\n"
			"Elves can be of any class except Barbarian.\r\n", d);
		break;
	case RACE_HALF_ORC:
		SEND_TO_Q
			("   Half-Orcs are the result of a cross breeding of an orc and human,\r\n"
			"the father usually being the orc.  The other races usually hate\r\n"
			"half-orcs, but tolerate them.  Half-orcs are especially strong\r\n"
			"and vicious, but have no magical abilities.  They are able to see\r\n"
			"in the dark, and are able to take a lot of abuse.\r\n"
			"Half-Orcs cannot be Clerics, Mages, Knights, Monks, or Rangers.\r\n",
			d);
		break;
	case RACE_TABAXI:
		SEND_TO_Q
			("   This is an intellegent race of feline humanoids from the far\r\n"
			"reaches of the jungles of the southern continent.  Most tabaxi are\r\n"
			"at least as tall as their human counterparts, but more lithe.\r\n"
			"Adult tabaxi are covered in light tawny fur which is striped in\r\n"
			"black, and their eyes range in color from green to yellow.  Tabaxi\r\n"
			"are exceptionally agile, and fairly strong, but their intellegence\r\n"
			"is somewhat animalistic.  Tabaxi may be of all classes except Knight.\r\n",
			d);
		break;
	case RACE_MINOTAUR:
		SEND_TO_Q
			("   Minotaurs are the result of an ancient curse which transformed\r\n"
			"men into powerful monsters with the head of a bull.  Minotaurs are\r\n"
			"very large and strong, but often lacking in mental\r\n"
			"facilities.  Minotaurs may not be knights, monks, or thieves.\r\n",
			d);
		break;

	case RACE_DROW:
		SEND_TO_Q
			("   The Drow are a race of evil creatures that live beneath the ground.\r\n"
			"The strongest concentration of drow has been pinpointed to Skullport\r\n"
			"inside Undermountain.  This is only the information that we have garnered\r\n"
			"from the few that we have captured.  The truth is far from known.\r\n"
			"     Drow are subterranean elves that have spent their entire lives\r\n"
			"underground.  Like elves they are slightly faster, and smarter than\r\n"
			"humans, and just a little less healty.  Unlike elves, the lifespan of\r\n"
			"Drow is only around 500 yrs.  This is probably due to their link with\r\n"
			"chaos.  Drow have to start in Skullport, and must be evil.\r\n"
			"Drow may be of any char_class except barbarian and monk.\r\n", d);
		break;
	case RACE_DWARF:
		SEND_TO_Q
			("   Dwarves are short, stocky humanoids which feel most at home in\r\n"
			"deep caves and mountains.  They are usually quite strong and healthy\r\n"
			"can see in the dark, and have great success fighting giant creatures,\r\n"
			"but tend be ill-tempered.  Dwarves are unable to use magical spells\r\n"
			"and have much difficulty getting magical items to work.  Dwarves\r\n"
			"tolerate elves, but hate orcs and goblins, and attack them on sight!\r\n"
			"Dwarves cannot be Mages, Monks, or Rangers.\r\n", d);
		break;
	default:
		SEND_TO_Q("There is no help on that race.\r\n", d);
		break;
	}
	SEND_TO_Q("\r\n", d);
}

const char *
get_hometown_abbrev(int hometown)
{
	int idx = 0;

	while (idx < hometown && home_town_abbrevs[idx][0] != '\n')
		idx++;
	if (home_town_abbrevs[idx][0] == '\n')
		return "INVALID";
	
	return home_town_abbrevs[idx];
}
