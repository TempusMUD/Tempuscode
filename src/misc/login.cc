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

void show_race_help(struct descriptor_data *d, int race, int timeframe);

void
show_menu(struct descriptor_data *d)
{
	if (!(d->creature)) {
		return;
	}

	SEND_TO_Q("\r\n\r\n", d);

	sprintf(buf, "%s                      You have now arrived at\r\n",
		CCCYN(d->creature, C_NRM));
	sprintf(buf, "%s                           %sTEMPUS  MUD%s     \r\n", buf,
		CCRED(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf, "%s                      %s0%s)%s Depart from Tempus.%s\r\n",
		buf, CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf,
		"%s                      %s1%s)%s Go forth into the realm.%s\r\n", buf,
		CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf, "%s                      %s2%s)%s Enter description.%s\r\n",
		buf, CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf,
		"%s                      %s3%s)%s Read the background story.%s\r\n",
		buf, CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf, "%s                      %s4%s)%s Change password.%s\r\n",
		buf, CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	sprintf(buf,
		"%s                      %s5%s)%s Delete this character.%s\r\n", buf,
		CCYEL(d->creature, C_NRM), CCRED(d->creature, C_NRM),
		CCGRN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));

	if (PLR2_FLAGGED(d->creature, PLR2_BURIED)) {
		sprintf(buf,
			"%s\r\n\r\n\r\n        %sYou have passed on, and been given a proper burial.%s",
			buf, CCYEL(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	} else if (PLR_FLAGGED(d->creature, PLR_CRYO)) {
		sprintf(buf,
			"%s\r\n                      %sYou are currently cryo rented.%s\r\n",
			buf, CCCYN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
	}


	sprintf(buf, "%s\r\n\r\n\r\n\r\n", buf);
	sprintf(buf, "%s                         Make your choice:  ", buf);

	SEND_TO_Q(buf, d);
	return;
}

//
// show_char_class_menu is for choosing a char_class when you remort
//

void
show_char_class_menu(struct descriptor_data *d, int timeframe)
{
	Creature *ch = d->creature;
	int i;

	for (i = 0;i < NUM_PC_RACES;i++)
		if (race_restr[i][0] == GET_RACE(ch))
			break;
		
	buf[0] = '\0';
	if (!timeframe || timeframe == TIME_PAST) {
		if (GET_CLASS(ch) != CLASS_MAGE && race_restr[i][CLASS_MAGE + 1])
			send_to_desc(d,
				"                &gMage&n      --  Delver in Magical Arts\r\n");
		if (GET_CLASS(ch) != CLASS_BARB && race_restr[i][CLASS_BARB + 1])
			send_to_desc(d,
				"                &gBarbarian&n --  Uncivilized Warrior\r\n");
		if (GET_CLASS(ch) != CLASS_KNIGHT && GET_CLASS(ch) != CLASS_MONK &&
				race_restr[i][CLASS_KNIGHT + 1])
			send_to_desc(d,
				"                &gKnight&n    --  Defender of the Faith\r\n");
		if (GET_CLASS(ch) != CLASS_RANGER && race_restr[i][CLASS_RANGER + 1])
			send_to_desc(d, "                &gRanger&n    --  Roamer of Worlds\r\n");
		if (GET_CLASS(ch) != CLASS_CLERIC && GET_CLASS(ch) != CLASS_MONK &&
				race_restr[i][CLASS_CLERIC + 1])
			send_to_desc(d, "                &gCleric&n    --  Servant of Diety\r\n");
		if (GET_CLASS(ch) != CLASS_THIEF && race_restr[i][CLASS_THIEF + 1])
			send_to_desc(d, "                &gThief&n     --  Stealthy Rogue\r\n");
	}

	// Print future classes
	if (!timeframe || timeframe == TIME_FUTURE) {
		if (GET_CLASS(ch) != CLASS_CYBORG && race_restr[i][CLASS_CYBORG + 1])
			send_to_desc(d,
				"                &gCyborg&n      --  The Electronically Advanced\r\n");
		if (GET_CLASS(ch) != CLASS_PSIONIC && race_restr[i][CLASS_PSIONIC + 1])
			send_to_desc(d, "                &gPsionic&n     --  Mind Traveller\r\n");
		if (GET_CLASS(ch) != CLASS_MERCENARY && race_restr[i][CLASS_MERCENARY + 1])
			send_to_desc(d, "                &gMercenary&n   --  Gun for Hire\r\n");
		if (GET_CLASS(ch) != CLASS_PHYSIC && race_restr[i][CLASS_PHYSIC + 1])
			send_to_desc(d,
				"                &gPhysic&n      --  Controller of Forces\r\n");
		if (GET_CLASS(ch) != CLASS_HOOD && race_restr[i][CLASS_HOOD + 1])
			send_to_desc(d,
				"                &gHoodlum&n     --  The Lowdown Street Punk\r\n");
	}

	// Monks are both future and past
	if (GET_CLASS(ch) != CLASS_MONK && GET_CLASS(ch) != CLASS_KNIGHT && 
		GET_CLASS(ch) != CLASS_CLERIC && race_restr[i][CLASS_MONK + 1])
		send_to_desc(d,
			"                &gMonk&n      --  Philosophical Warrior\r\n");
	send_to_desc(d, "\r\n\r\n\r\n");
}

//
// show_home_help_past
//

void
show_home_help_past(struct descriptor_data *d, int home)
{

	switch (home) {
	case (-1):
		SEND_TO_Q("There is no help on that home.\r\n", d);
		break;
	case HOME_MODRIAN:
		SEND_TO_Q
			("\r\nModrian is the center of civilized life in the realm.  Located\r\n"
			"at a focal point of the land, it enjoys relative safety from the\r\n"
			"dangers of the wilderness, under the rule of Duke Araken.  Major\r\n"
			"roads lead away from Modrian in all directions, linking it to the\r\n"
			"other major cities.  Guilds for all char_classes other than monk can be\r\n"
			"found here, including one of the primary centers of worship of the\r\n"
			"goddess Guiharia.\r\n", d);
		break;
	case HOME_ISTAN:
		SEND_TO_Q
			("\r\nLocated on the wild northern outskirts of the civilized realm, Istan\r\n"
			"is a vital city filled with tough pioneers and rangers of the land.\r\n"
			"Istan built over the banks of the River Na, which flows all the\r\n"
			"way to the Misty Sea, past Modrian.  Istan is known for being raided\r\n"
			"frequently by the evil giants which inhabit the area, and the\r\n"
			"vast subterranean realm known as Undermountain, which lies beneath\r\n"
			"the city.\r\n"
			"Istan sports guilds for thieves, barbarians, and rangers, and lies\r\n"
			"near to the guild of fighting monks.\r\n", d);
		break;
	case HOME_NEW_THALOS:
		SEND_TO_Q
			("\r\nSmitty appears and bellows, 'There is no help on New thalos!!\r\n",
			d);
		break;
	case HOME_KROMGUARD:
		SEND_TO_Q
			("\r\nSmitty appears and bellows, 'There is no help on kromguard!!\r\n",
			d);
		break;
	case HOME_NEWBIE_TOWER:
		SEND_TO_Q
			("\r\nThe Tower of Modrian is an area designed specifically to aid new\r\n"
			"players, specifically those new to mudding in general, in getting\r\n"
			"started in the game.  The tower is built atop the Temple of Guiharia\r\n"
			"in Modrian, and the city itself is readily accessible from the\r\n"
			"tower.  The tower may be used until the player reaches level 6.\r\n",
			d);
		break;
	case HOME_SKULLPORT:
		SEND_TO_Q
			("\r\nSkullport is the roughest town on Tempus.  Located deep\r\n"
			"in UnderMountain, its only accesses to the surface world are\r\n"
			"dangerous caverns and the River Sargauth.  Thieves enjoy the\r\n"
			"largest Thieves Guild in the world and mages study under the\r\n"
			"masterful hand of the Drow. The Antipaladins, Evil Clerics of\r\n"
			"Kalar, and barbarians also retain a guild in the caverns of\r\n"
			"the UnderDark.\r\n", d);

		break;
	default:
		SEND_TO_Q("No help on that place yet.\r\n", d);
		break;
	}
	SEND_TO_Q("\r\n", d);
}

//
// parse_past_home
//

int
parse_past_home(struct descriptor_data *d, char *arg)
{
	int home = -1;

	skip_spaces(&arg);
	half_chop(arg, buf, buf2);

	if (is_abbrev(buf, "help")) {
		if (!*buf2)
			SEND_TO_Q("Help on what hometown?\r\n", d);
		else {
			home = parse_past_home(d, buf2);
			show_home_help_past(d, home);
		}
		return (-2);
	}
	if (is_abbrev(buf, "new thalos"))
		return (HOME_NEW_THALOS);
	else if (is_abbrev(buf, "istan"))
		return (HOME_ISTAN);
	else if (is_abbrev(buf, "elven village"))
		return (HOME_ELVEN_VILLAGE);
	else if (is_abbrev(buf, "solace cove"))
		return (HOME_SOLACE_COVE);
	else if (is_abbrev(buf, "modrian"))
		return (HOME_MODRIAN);
	else if (is_abbrev(buf, "tower"))
		return (HOME_NEWBIE_TOWER);
	else if (is_abbrev(buf, "skullport"))
		return (HOME_SKULLPORT);
	else
		return (-1);
	return (-1);
}

//
// show_time_menu
//

int
parse_pc_race(struct descriptor_data *d, char *arg, int timeframe)
{
	int race = -1;

	skip_spaces(&arg);
	half_chop(arg, buf, buf2);

	if (!str_cmp(buf, "help")) {
		if (!*buf2)
			SEND_TO_Q("Help on what race?\r\n", d);
		else {
			race = parse_pc_race(d, buf2, timeframe);
			show_race_help(d, race, timeframe);
		}
		return (-2);
	}

	if (is_abbrev(buf, "human"))
		return RACE_HUMAN;
	else if (is_abbrev(buf, "elf") || is_abbrev(buf, "elven"))
		return RACE_ELF;
	else if (is_abbrev(buf, "half orc") || is_abbrev(buf, "half orcen"))
		return RACE_HALF_ORC;
	else if (is_abbrev(buf, "tabaxi"))
		return RACE_TABAXI;
	if (timeframe == TIME_PAST) {
		// Past only classes
		if (is_abbrev(buf, "dwarf") || is_abbrev(buf, "dwarven"))
			return RACE_DWARF;
		else if (is_abbrev(buf, "minotaur"))
			return RACE_MINOTAUR;
		else if (is_abbrev(buf, "drow"))
			return RACE_DROW;
	} else {
		// Future only races
		if (is_abbrev(buf, "orc"))
			return RACE_ORC;
	}

	return (-1);
}

void
show_race_help(struct descriptor_data *d, int race, int timeframe)
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
