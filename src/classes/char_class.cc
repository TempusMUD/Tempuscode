
/************************************************************************
 *   File: char_class.c                                       Part of CircleMUD *
 *  Usage: Source file for char_class-specific code                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

//
// File: char_class.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

 /*
  * This file attempts to concentrate most of the code which must be changed
  * in order for new char_classes to be added.  If you're adding a new char_class,
  * you should go through this entire file from beginning to end and add
  * the appropriate new special cases for your new char_class.
  */


#define __char_class_c__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "char_class.h"
#include "comm.h"
#include "vehicle.h"
#include "handler.h"
#include "security.h"
#include "tmpstr.h"
#include "clan.h"
#include "player_table.h"

extern struct room_data *world;


/*
 * These are definitions which control the guildmasters for each char_class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the char_class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPL        0
#define SKL        1
#define TRG     2
#define ALT     3
#define PRG     4
#define ZEN     5
#define SNG     6

/* #define LEARNED_LEVEL        0  % known which is considered "learned" */
/* #define MAX_PER_PRAC                1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC                2  min percent gain in skill per practice */
/* #define PRAC_TYPE                3  should it say 'spell' or 'skill'?        */

extern const int prac_params[4][NUM_CLASSES] = {
  /* MG  CL  TH  WR  BR  PS  PH  CY  KN  RN  BD  MN  VP  MR  S1  S2  S3*/
	{75, 75, 70, 70, 65, 75, 75, 80, 75, 75, 80, 75, 75, 70, 70, 70, 70},
	{25, 20, 20, 20, 20, 25, 20, 30, 20, 25, 30, 20, 15, 25, 25, 25, 25},
	{15, 15, 10, 15, 10, 15, 15, 15, 15, 15, 15, 10, 10, 10, 10, 10, 10},
	{SPL,SPL,SKL,SKL,SKL,TRG,ALT,PRG,SPL,SPL,SNG,ZEN,SPL,SKL,SKL,SKL,SKL}

};

// 0 - class/race combination not allowed
// 1 - class/race combination allowed only for secondary class
// 2 - class/race combination allowed for primary class
extern const char race_restr[NUM_PC_RACES][NUM_CLASSES + 1] = {
	//                 MG CL TH WR BR PS PH CY KN RN BD MN VP MR S1 S2 S3
	{ RACE_HUMAN,		2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0, 0 },
	{ RACE_ELF,			2, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0, 0 },
	{ RACE_DWARF,		0, 2, 2, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ RACE_HALF_ORC,	0, 0, 2, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
	{ RACE_HALFLING,	2, 2, 2, 0, 2, 1, 1, 1, 2, 2, 2, 2, 0, 1, 0, 0, 0 },
	{ RACE_TABAXI,		2, 2, 2, 0, 2, 2, 2, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0 },
	{ RACE_DROW,		2, 2, 2, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 1, 0, 0, 0 },
	{ RACE_MINOTAUR,	2, 2, 0, 0, 2, 0, 1, 1, 0, 2, 0, 0, 0, 1, 0, 0, 0 },
	{ RACE_ORC,			0, 0, 1, 0, 2, 0, 1, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0 },
};

/* THAC0 for char_classes and levels.  (To Hit Armor Class 0) */

extern const float thaco_factor[NUM_CLASSES] = {
	0.15,						/* mage    */
	0.20,						/* cleric  */
	0.25,						/* thief   */
	0.30,						/* warrior */
	0.40,						/* barb    */
	0.20,						/* psionic */
	0.15,						/* physic  */
	0.30,						/* cyborg  */
	0.35,						/* knight  */
	0.35,						/* ranger  */
	0.30,						/* bard    */
	0.40,						/* monk    */
	0.40,						/* vampire */
	0.35,						/* merc    */
	0.30,						/* spare1  */
	0.30,						/* spare2  */
	0.30						/* spare3  */
};

void
gain_skill_prof(struct Creature *ch, int skl)
{
	int learned;
	if (skl == SKILL_READ_SCROLLS || skl == SKILL_USE_WANDS)
		learned = 10;
	else
		learned = LEARNED(ch);

	// NPCs don't learn
	if (IS_NPC(ch))
		return;
	

	// You can't gain in a skill that you don't really know
	if (GET_LEVEL(ch) < SPELL_LEVEL(skl, GET_CLASS(ch))) {
		if (!IS_REMORT(ch))
			return;
		if (GET_LEVEL(ch) < SPELL_LEVEL(skl, GET_REMORT_CLASS(ch)))
			return;
	}

	// Check for remort classes too
	if (SPELL_GEN(skl, GET_CLASS(ch)) > 0 &&
			GET_REMORT_GEN(ch) < SPELL_GEN(skl, GET_CLASS(ch)))
		return;

	if (GET_SKILL(ch, skl) >= (learned - 10))
		if ((GET_SKILL(ch, skl) - GET_LEVEL(ch)) <= 60
			&& !number(0, GET_LEVEL(ch)))
			GET_SKILL(ch, skl) += 1;
}


/* Names first */
extern const char *char_class_abbrevs[] = {
	"Mage",						/* 0 */
	"Cler",
	"Thie",
	"Warr",
	"Barb",
	"Psio",						/* 5 */
	"Phyz",
	"Borg",
	"Knig",
	"Rang",
	"Bard",						/* 10 */
	"Monk",
	"Vamp",
	"Merc",
	"Spa1",
	"Spa2",						/* 15 */
	"Spa3",
	"ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",	/*25 */
	"ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",	/*35 */
	"ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",	/*45 */
	"ERR", "ERR", "ERR", "ERR",
	"Norm",
	"Bird",
	"Pred",
	"Snak",
	"Hrse",
	"Smll",
	"Medm",
	"Lrge",
	"Scin",
	"ERR",
	"Skel",
	"Ghou",
	"Shad",
	"Wigt",
	"Wrth",
	"Mumy",
	"Spct",
	"Vamp",
	"Ghst",
	"Lich",
	"Zomb",
	"ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",
	"Eart",
	"Fire",
	"Watr",
	"Air ",
	"Lgtn",
	"ERR", "ERR", "ERR", "ERR", "ERR",
	"Gren",
	"Whte",
	"Blck",
	"Blue",
	"Red",						/* 95 */
	"Slvr",
	"Shad",
	"Deep",
	"Turt",
	"ILL",						/* 100 */
	"Lest",
	"Lssr",
	"Grtr",
	"Duke",
	"Arch",						/*105 */
	"ILL", "ILL", "ILL", "ILL", "ILL",	/*110 */
	"Hill", "Ston", "Frst", "Fire", "Clod",	/*115 */
	"Strm",
	"ILL", "ILL", "ILL",
	"Red",						/* 120 */
	"Blue",
	"Gren",
	"Grey",
	"Deth",
	"Lord",						/* 125 */
	"ILL", "ILL", "ILL", "ILL",
	"Tp I",
	"T II",
	"TIII",
	"T IV",
	"Tp V",
	"T VI",
	"Semi",
	"Minr",
	"Majr",
	"Lord",
	"Prnc",						/* 140 */
	"ILL",
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
	"Astl",						/* Devas *//* 150 */
	"Mond",
	"Movn",
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	// 162
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	// 172
	"ILL", "ILL", "ILL", "ILL", "ILL",
    "Gdlg",                   // 178  Godling and Demigods extra planes
    "Diet",
    "ILL", "ILL", "ILL",	// 182
	"\n"
};

extern const char *pc_char_class_types[] = {
	"Mage",
	"Cleric",
	"Thief",
	"Warrior",
	"Barbarian",
	"Psionic",					/* 5 */
	"Physic",
	"Cyborg",
	"Knight",
	"Ranger",
	"Bard",					/* 10 */
	"Monk",
	"Vampire",
	"Mercenary",
	"Spare1",
	"Spare2",					/* 15 */
	"Spare3",
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	/* 25 */
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	/* 35 */
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	/* 45 */
	"ILL", "ILL", "ILL", "ILL",
	"Mobile",					/* 50 */
	"Bird",
	"Predator",
	"Snake",
	"Horse",
	"Small",
	"Medium",
	"Large",
	"Scientist",
	"ILL", "Skeleton",			/* 60 */
	"Ghoul", "Shadow", "Wight", "Wraith", "Mummy",	/* 65 */
	"Spectre", "Vampire", "Ghost", "Lich", "Zombie",	/* 70 */
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",	/* 80 */
	"Earth", "Fire", "Water", "Air", "Lighting",	/* 85 */
	"ILL", "ILL", "ILL", "ILL", "ILL",	/* 90 */
	"Green",
	"White",
	"Black",
	"Blue",
	"Red",						/* 95 */
	"Silver",
	"Shadow_D",
	"Deep",
	"Turtle", "ILL",			/* 100 */
	"Least",
	"Lesser",
	"Greater",
	"Duke",
	"Arch",						/*105 */
	"ILL", "ILL", "ILL", "ILL", "ILL",	/*110 */
	"Hill", "Stone", "Frost", "Fire", "Cloud",	/*115 */
	"Storm",
	"ILL", "ILL", "ILL",
	"Red",						/* Slaad *//* 120 */
	"Blue",
	"Green",
	"Grey",
	"Death",
	"Lord",						/* 125 */
	"ILL", "ILL", "ILL", "ILL",
	"Type I",					/* Demons *//* 130 */
	"Type II",
	"Type III",
	"Type IV",
	"Type V",
	"Type VI",
	"Semi",
	"Minor",
	"Major",
	"Lord",
	"Prince",					/* 140 */
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
	"Astral",					/* Devas *//* 150 */
	"Monadic",
	"Movanic",
	"ILL", "ILL", "ILL",
	"ILL", "ILL", "ILL", "ILL",
	"Fire",						/* Mephits 160 */
	"Lava",
	"Smoke",
	"Steam",
	"ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
	"Arcana",					// daemons 170
	"Charona",
	"Dergho",
	"Hydro",
	"Pisco",
	"Ultro",
	"Yagno",
	"Pyro",						// 177
    "Godling",                  // 178 - For Extra Planes
    "Deity",
	"\n"
};

// Returns a tmpstr allocated char* containing an appropriate ANSI
// color code for the given target Creature (tch) with the given
// recipient Creature(ch)'s color settings in mind.
const char*
get_char_class_color( Creature *ch, Creature *tch, int char_class ) {
    switch( char_class ) {
        case CLASS_MAGIC_USER:
            return CCMAG(ch, C_NRM);
        case CLASS_CLERIC:
            if (IS_GOOD(tch)) {
                return CCBLU_BLD(ch, C_NRM);
            } else if (IS_EVIL(tch)) {
                return CCRED_BLD(ch, C_NRM);
            } else {
                return CCYEL(ch, C_NRM);
            }
        case CLASS_KNIGHT:
            if (IS_GOOD(tch)) {
                return CCBLU_BLD(ch, C_NRM);
            } else if (IS_EVIL(tch)) {
                return CCRED(ch, C_NRM);
            } else {
                return CCYEL(ch, C_NRM);
            }
        case CLASS_RANGER:
            return CCGRN(ch, C_NRM);
        case CLASS_BARB:
            return CCCYN(ch, C_NRM);
        case CLASS_THIEF:
            return CCNRM_BLD(ch, C_NRM);
        case CLASS_CYBORG:
            return CCCYN(ch, C_NRM);
        case CLASS_PSIONIC:
            return CCMAG(ch, C_NRM);
        case CLASS_PHYSIC:
            return CCNRM_BLD(ch, C_NRM);
        case CLASS_BARD:
            return CCYEL_BLD(ch, C_NRM);
        case CLASS_MONK:
            return CCGRN(ch, C_NRM);
        case CLASS_MERCENARY:
            return CCYEL(ch, C_NRM);
        default:
            return CCNRM(ch, C_NRM);
    }
}

// Returns a const char* containing an appropriate '&c' color code for the given target 
// Creature (tch) suitable for use with send_to_desc.
const char*
get_char_class_color( Creature *tch, int char_class ) {
    switch( char_class ) {
        case CLASS_MAGIC_USER:
            return "&m";
        case CLASS_CLERIC:
        case CLASS_KNIGHT:
            if (IS_GOOD(tch)) {
                return "&B";
            } else if (IS_EVIL(tch)) {
                return "&R";
            } else {
                return "&y";
            }
        case CLASS_RANGER:
            return "&g";
        case CLASS_BARB:
            return "&c";
        case CLASS_THIEF:
            return "&N";
        case CLASS_CYBORG:
            return "&c";
        case CLASS_PSIONIC:
            return "&m";
        case CLASS_PHYSIC:
            return "&N";
        case CLASS_BARD:
            return "&Y";
        case CLASS_MONK:
            return "&g";
        case CLASS_MERCENARY:
            return "&y";
        default:
            return "&n";
    }
}



/*
 * The code to interpret a hometown letter -- used in interpreter.c
 */

int
parse_time_frame(char *arg)
{

	skip_spaces(&arg);

	if (is_abbrev(arg, "past") || is_abbrev(arg, "modrian"))
		return HOME_MODRIAN;
	if (is_abbrev(arg, "future") || is_abbrev(arg, "electro"))
		return HOME_ELECTRO;

	return HOME_UNDEFINED;
};

/*
 * The code to interpret a char_class letter -- used in interpreter.c when a
 * new character is selecting a char_class and by 'set char_class' in act.wizard.c.
 */

int
parse_player_class(char *arg)
{
	skip_spaces(&arg);
	if (is_abbrev(arg, "magic user") || is_abbrev(arg, "mage"))
		return CLASS_MAGIC_USER;
	else if (is_abbrev(arg, "cleric"))
		return CLASS_CLERIC;
	else if (is_abbrev(arg, "barbarian"))
		return CLASS_BARB;
	else if (is_abbrev(arg, "thief"))
		return CLASS_THIEF;
	else if (is_abbrev(arg, "knight"))
		return CLASS_KNIGHT;
	else if (is_abbrev(arg, "ranger"))
		return CLASS_RANGER;
	else if (is_abbrev(arg, "monk"))
		return CLASS_MONK;
	else if (is_abbrev(arg, "cyborg") || is_abbrev(arg, "borg"))
		return CLASS_CYBORG;
	else if (is_abbrev(arg, "psionic") || is_abbrev(arg, "psychic"))
		return CLASS_PSIONIC;
	else if (is_abbrev(arg, "physic") || is_abbrev(arg, "physicist"))
		return CLASS_PHYSIC;
	else if (is_abbrev(arg, "monk"))
		return CLASS_MONK;
	else if (is_abbrev(arg, "mercenary"))
		return CLASS_MERCENARY;
    else if (is_abbrev(arg, "bard"))
        return CLASS_BARD;

	return CLASS_UNDEFINED;
}

extern const char *player_race[] = {
	"Human",
	"Elf",
	"Dwarf",
	"Half Orc",
	"Klingon",
	"Halfling",					/* 5 */
	"Tabaxi",
	"Drow",
	"ILL", "ILL",
	"Mobile",					/* 10 */
	"Undead",
	"Humanoid",
	"Animal",
	"Dragon",
	"Giant",					/* 15 */
	"Orc",
	"Goblin",
	"Hafling",
	"Minotaur",
	"Troll",					/* 20 */
	"Golem",
	"Elemental",
	"Ogre",
	"Devil",
	"Trog",
	"Manticore",
	"Bugbear",
	"Draconian",
	"Duergar",
	"Slaad",
	"Robot",
	"Demon",
	"Deva",
	"Plant",
	"Archon",
	"Pudding",
	"Alien 1",
	"Predator Alien",
	"Slime",
	"Illithid",					/* 40 */
	"Fish",
	"Beholder",
	"Gaseous",
	"Githyanki",
	"Insect",
	"Daemon",
	"Mephit",
	"Kobold",
	"Umber Hulk",
	"Wemic",
	"Rakshasa",
	"Spider",                  /* 52 */
    "Griffin",
    "Rotarian",
    "Half Elf",
    "Celestial",
    "Guardinal",
    "Olympian",
    "Yugoloth",
    "Rowlahr",
	"\n"
};

extern const int race_lifespan[] = {
	80,							/* human */
	400,						/* elf */
	160,						/* dwarf */
	55,							/* half orc */
	100,						/* klingon */
	110,						/* halfling */
	80,							/* tabaxi */
	200,						/* drow */
	0,
	0,
	0,							/* mobile */
	10000,						/* undead */
	100,						/* humanoid */
	50,							/* animal */
	10000,						/* dragon */
	500,						/* giant */
	65,							/* orc */
	50,							/* goblin */
	50,							/* halfling */
	200,						/* minotaur */
	500							/* troll */
};

int
parse_race(char *arg)
{
	int j;

	for (j = 0; j < NUM_RACES; j++)
		if (is_abbrev(arg, player_race[j]))
			return j;

	return (-1);
}

int
parse_char_class(char *arg)
{
	int j;

	for (j = 0; j < TOP_CLASS; j++)
		if (is_abbrev(arg, pc_char_class_types[j]))
			return j;

	return (-1);
}


/*
 * bitvectors (i.e., powers of two) for each char_class, mainly for use in
 * do_who and do_users.  Add new char_classes at the end so that all char_classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long
find_char_class_bitvector(char arg)
{
	arg = tolower(arg);

	switch (arg) {
	case 'm':
		return (1 << 0);
		break;
	case 'c':
		return (1 << 1);
		break;
	case 't':
		return (1 << 2);
		break;
	case 'w':
		return (1 << 3);
		break;
	case 'b':
		return (1 << 4);
		break;
	case 's':
		return (1 << 5);
		break;
	case 'p':
		return (1 << 6);
		break;
	case 'y':
		return (1 << 7);
		break;
	case 'k':
		return (1 << 8);
		break;
	case 'r':
		return (1 << 9);
		break;
	case 'h':
		return (1 << 10);
		break;
	case 'n':
		return (1 << 11);
		break;
	case 'e':
		return (1 << 13);
		break;
	default:
		return 0;
		break;
	}
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each char_class then decides
 * which priority will be given for the best to worst stats.
 */
void
roll_real_abils(struct Creature *ch)
{
	int i, j, k, str_add;
	ubyte table[6];
	// Zero out table
	for (i = 0; i < 6; i++) {
		table[i] = 12;
	}

	// Increment and decrement randomly
	for (i = 0; i < 24; i++) {
		do { j = number(0, 5); } while (table[j] == 18);
		table[j] += 1;
		do { j = number(0, 5); } while (table[j] == 3);
		table[j] -= 1;
	}

    str_add = number(0, 100);
    int remainder = str_add % 10;
    if (remainder > 4)
        str_add += (10 - remainder);
    else
        str_add -= (remainder);

	// Sort the table
	for (j = 0;j < 6;j++) {
		for (k = j; k < 6; k++) {
			if (table[k] > table[j]) {
				table[j] ^= table[k];
				table[k] ^= table[j];
				table[j] ^= table[k];
			}
		}
	}
	ch->real_abils.str_add = 0;

	switch (GET_CLASS(ch)) {
	case CLASS_MAGIC_USER:
		ch->real_abils.intel = table[0];
		ch->real_abils.wis = table[1];
		ch->real_abils.dex = table[2];
		ch->real_abils.cha = table[3];
		ch->real_abils.con = table[4];
		ch->real_abils.str = table[5];
		break;
	case CLASS_CLERIC:
		ch->real_abils.wis = table[0];
		ch->real_abils.intel = table[1];
		ch->real_abils.str = table[2];
		ch->real_abils.dex = table[3];
		ch->real_abils.con = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_THIEF:
		ch->real_abils.dex = table[0];
		ch->real_abils.str = table[1];
		ch->real_abils.con = table[2];
		ch->real_abils.intel = table[3];
		ch->real_abils.wis = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_WARRIOR:
		ch->real_abils.str = table[0];
		ch->real_abils.dex = table[1];
		ch->real_abils.con = table[2];
		ch->real_abils.wis = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = str_add;
		break;
	case CLASS_BARB:
		ch->real_abils.str = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.dex = table[2];
		ch->real_abils.wis = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = str_add;
		break;

	case CLASS_PSIONIC:
		ch->real_abils.intel = table[0];
		ch->real_abils.wis = table[1];
		ch->real_abils.dex = table[2];
		ch->real_abils.str = table[3];
		ch->real_abils.con = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_PHYSIC:
		ch->real_abils.intel = table[0];
		ch->real_abils.wis = table[1];
		ch->real_abils.dex = table[2];
		ch->real_abils.str = table[3];
		ch->real_abils.con = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_CYBORG:
		switch (GET_OLD_CLASS(ch)) {
		case BORG_MENTANT:
			ch->real_abils.intel = table[0];
			ch->real_abils.str = table[1];
			ch->real_abils.dex = table[2];
			ch->real_abils.con = table[3];
			ch->real_abils.wis = table[4];
			ch->real_abils.cha = table[5];
			break;
		case BORG_SPEED:
			ch->real_abils.dex = table[0];
			ch->real_abils.con = table[1];
			ch->real_abils.str = table[2];
			ch->real_abils.intel = table[3];
			ch->real_abils.wis = table[4];
			ch->real_abils.cha = table[5];
			break;
		default:
			ch->real_abils.str = table[0];
			ch->real_abils.con = table[1];
			ch->real_abils.dex = table[2];
			ch->real_abils.intel = table[3];
			ch->real_abils.wis = table[4];
			ch->real_abils.cha = table[5];
			break;
		}
		break;
	case CLASS_KNIGHT:
		ch->real_abils.str = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.wis = table[2];
		ch->real_abils.dex = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = str_add;
		break;
	case CLASS_RANGER:
		ch->real_abils.dex = table[0];
		ch->real_abils.wis = table[1];
		ch->real_abils.con = table[2];
		ch->real_abils.str = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = str_add;
		break;
	case CLASS_MONK:
		ch->real_abils.dex = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.str = table[2];
		ch->real_abils.wis = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_VAMPIRE:
		ch->real_abils.dex = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.str = table[2];
		ch->real_abils.intel = table[3];
		ch->real_abils.wis = table[4];
		ch->real_abils.cha = table[5];
		break;
	case CLASS_MERCENARY:
		ch->real_abils.str = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.dex = table[2];
		ch->real_abils.intel = table[3];
		ch->real_abils.wis = table[4];
		ch->real_abils.cha = table[5];
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = str_add;
		break;
    case CLASS_BARD:
		ch->real_abils.str = table[2];
		ch->real_abils.dex = table[1];
		ch->real_abils.cha = table[0];
		ch->real_abils.con = table[3];
		ch->real_abils.wis = table[4];
		ch->real_abils.intel = table[5];
		break;
	default:
		ch->real_abils.dex = table[0];
		ch->real_abils.con = table[1];
		ch->real_abils.str = table[2];
		ch->real_abils.wis = table[3];
		ch->real_abils.intel = table[4];
		ch->real_abils.cha = table[5];
		break;
	}
	switch (GET_RACE(ch)) {
	case RACE_ELF:
	case RACE_DROW:
		ch->real_abils.intel += 1;
		ch->real_abils.dex += 1;
		ch->real_abils.con -= 1;
		break;
	case RACE_HALFLING:
		if (ch->real_abils.str == 18 && ch->real_abils.str_add > 0)
			ch->real_abils.str_add -= 10;
		else
			ch->real_abils.str -= 1;
		if (ch->real_abils.str == 18 && ch->real_abils.str_add > 0)
			ch->real_abils.str_add -= 10;
		else
			ch->real_abils.str -= 1;
		ch->real_abils.dex += 2;
		break;
	case RACE_DWARF:
		ch->real_abils.con += 1;
		if (ch->real_abils.str == 18) {
			ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
		} else {
			ch->real_abils.str += 1;
		}
		ch->real_abils.cha -= 1;
		break;
	case RACE_HALF_ORC:
		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
		else
			ch->real_abils.str += 1;

		if (ch->real_abils.str == 18)
			ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
		else if (ch->real_abils.str < 18)
			ch->real_abils.str += 1;

		ch->real_abils.con += 1;
		ch->real_abils.cha -= 3;
		break;
	case RACE_TABAXI:
		ch->real_abils.dex = MIN(20, ch->real_abils.dex + 3);
		ch->real_abils.intel -= 1;
		ch->real_abils.wis -= 3;
		ch->real_abils.con += 1;
		ch->real_abils.cha -= 2;
		break;
	case RACE_MINOTAUR:
		ch->real_abils.intel -= 2;
		ch->real_abils.wis -= 3;
		ch->real_abils.con += 2;
		ch->real_abils.cha -= 2;
		ch->real_abils.str += 3;
		if (ch->real_abils.str > 18) {
			ch->real_abils.str_add += (ch->real_abils.str - 18) * 10;
			if (ch->real_abils.str_add > 100) {
				ch->real_abils.str =
					MIN(20, 18 + ((ch->real_abils.str_add - 100) / 10));
				ch->real_abils.str_add = 0;
			} else
				ch->real_abils.str = 18;
		}
		break;

	case RACE_HUMAN:
		switch (GET_CLASS(ch)) {
		case CLASS_MAGIC_USER:
			ch->real_abils.intel += 1;
			ch->real_abils.dex += 1;
			break;
		case CLASS_CLERIC:
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 1;
			break;
		case CLASS_BARB:
			if (ch->real_abils.str == 18)
				ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
			else
				ch->real_abils.str += 1;
			ch->real_abils.con += 1;
			break;
		case CLASS_RANGER:
			ch->real_abils.intel += 1;
			ch->real_abils.dex += 1;
			break;
		case CLASS_THIEF:
			ch->real_abils.intel += 1;
			ch->real_abils.dex += 1;
			break;
		case CLASS_KNIGHT:
			if (ch->real_abils.str == 18)
				ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
			else
				ch->real_abils.str += 1;
			ch->real_abils.wis += 1;
			break;
		case CLASS_PSIONIC:
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 1;
			break;
		case CLASS_PHYSIC:
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 1;
			break;
		case CLASS_CYBORG:
			switch (GET_OLD_CLASS(ch)) {
			case BORG_MENTANT:
				ch->real_abils.intel += 1;
				ch->real_abils.wis += 1;
				break;
			case BORG_SPEED:
				ch->real_abils.dex += 1;
				ch->real_abils.intel += 1;
				break;
			case BORG_POWER:
				if (ch->real_abils.str == 18)
					ch->real_abils.str_add =
						MIN(100, ch->real_abils.str_add + 10);
				else
					ch->real_abils.str += 1;
				break;
			default:
				break;
			}
			break;
		case CLASS_MONK:
			ch->real_abils.dex += 1;
			ch->real_abils.wis += 1;
			break;
		case CLASS_MERCENARY:
			if (ch->real_abils.str == 18)
				ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
			else
				ch->real_abils.str += 1;
			ch->real_abils.dex += 1;
			break;
		default:
			break;
		}
		break;
	}

	ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills
   If mode == 0, then act as though the character was entering the
   game for the first time.  Otherwise, act as though the character
   is being set to that level.
 */
void
do_start(struct Creature *ch, int mode)
{
	void advance_level(struct Creature *ch, byte keep_internal);
	byte new_player = 0;
	int i;
	obj_data *implant_save[NUM_WEARS];

	// remove implant affects
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_IMPLANT(ch, i))
			implant_save[i] = unequip_char(ch, i, true, true);
		else
			implant_save[i] = NULL;

	if (GET_EXP(ch) == 0 && !IS_REMORT(ch) && !IS_VAMPIRE(ch))
		new_player = TRUE;

	GET_LEVEL(ch) = 1;
	GET_EXP(ch) = 1;

	if (mode)
		roll_real_abils(ch);

	for (i = 1; i <= MAX_SKILLS; i++)
		SET_SKILL(ch, i, 0);

	if (IS_VAMPIRE(ch))
		GET_LIFE_POINTS(ch) = 1;
	else
		GET_LIFE_POINTS(ch) = 3 * (GET_WIS(ch) + GET_CON(ch)) / 40;

	ch->points.max_hit = 20;
	ch->points.max_mana = 100;
	ch->points.max_move = 82;

	if (IS_TABAXI(ch)) {
		SET_SKILL(ch, SKILL_CLAW, LEARNED(ch));
		SET_SKILL(ch, SKILL_BITE, LEARNED(ch));
	}
	if (IS_ELF(ch)) {
		SET_SKILL(ch, SKILL_ARCHERY, LEARNED(ch));
	}

	switch (GET_CLASS(ch)) {
	case CLASS_MAGIC_USER:
		SET_SKILL(ch, SKILL_PUNCH, 10);
		break;
	case CLASS_CLERIC:
		SET_SKILL(ch, SKILL_PUNCH, 10);
		break;
	case CLASS_THIEF:
		SET_SKILL(ch, SKILL_PUNCH, 15);
		SET_SKILL(ch, SKILL_SNEAK, 10);
		SET_SKILL(ch, SKILL_HIDE, 5);
		SET_SKILL(ch, SKILL_STEAL, 15);
		break;
	case CLASS_WARRIOR:
		SET_SKILL(ch, SKILL_PUNCH, 20);
		break;
	case CLASS_BARB:
		SET_SKILL(ch, SKILL_PUNCH, 15);
		break;
	case CLASS_PSIONIC:
		SET_SKILL(ch, SKILL_PUNCH, 10);
		break;
	case CLASS_PHYSIC:
		SET_SKILL(ch, SKILL_PUNCH, 10);
		break;
	case CLASS_CYBORG:
		SET_SKILL(ch, SKILL_PUNCH, 10);
		break;
	case CLASS_KNIGHT:
		SET_SKILL(ch, SKILL_PUNCH, 20);
		break;
	case CLASS_RANGER:
		SET_SKILL(ch, SKILL_PUNCH, 15);
		GET_MAX_MOVE(ch) += dice(4, 9);
		break;
	case CLASS_MONK:
		SET_SKILL(ch, SKILL_PUNCH, 20);
		break;
	case CLASS_VAMPIRE:
		SET_SKILL(ch, SKILL_FEED, 20);
		break;
	case CLASS_MERCENARY:
		SET_SKILL(ch, SKILL_PUNCH, 20);
    case CLASS_BARD:
        SET_SKILL(ch, SKILL_PUNCH, 25);
        SET_SKILL(ch, SKILL_ARCHERY, 25);
		break;

	}

	if (new_player) {
		if (PAST_CLASS(GET_CLASS(ch))) {
			ch->desc->account->deposit_past_bank(
				8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch));
			ch->points.gold =
				8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
		} else if (FUTURE_CLASS(GET_CLASS(ch))) {
			ch->desc->account->deposit_future_bank(
				8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch));
			ch->points.cash =
				8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
		}

		set_title(ch, "the complete newbie");
	}

	advance_level(ch, 0);

	GET_MAX_MOVE(ch) += GET_CON(ch);

	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = GET_MAX_MANA(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);

	GET_COND(ch, THIRST) = 24;
	GET_COND(ch, FULL) = 24;
	GET_COND(ch, DRUNK) = 0;

	if (new_player) {
		ch->player.time.played = 0;
		ch->player.time.logon = time(0);
	}

	for (i = 0; i < NUM_WEARS; i++)
		if (implant_save[i])
			equip_char(ch, implant_save[i], i, true);
    
    // If there are no characters >= level 45 on this account enroll
    // this character in the academey.
    if (mode == 0 &&
			!ch->account->hasCharLevel(45) &&
			!ch->account->hasCharGen(1)) {
		struct clanmember_data *member = NULL;
		struct clan_data *clan = real_clan(TEMPUS_ACADEMY);

        GET_CLAN(ch) = TEMPUS_ACADEMY;
        CREATE(member, struct clanmember_data, 1);
        member->idnum = GET_IDNUM(ch);
        member->rank = 0;
        member->next = clan->member_list;
        clan->member_list = member;
        sort_clanmembers(clan);
        sql_exec("insert into clan_members (clan, player, rank) values (%d, %ld, %d)",
			 clan->number, GET_IDNUM(ch), 0);
    }

}

// prac_gain: mode==TRUE means to return a prac gain value
//            mode==FALSE means to return an average value
float
prac_gain(struct Creature *ch, int mode)
{
	float gain;
	double max, min;

	min = (double)((double)GET_WIS(ch) / 8);
	max = (double)((double)GET_WIS(ch) / 5) + 0.5;
	if (mode) {
		gain = (MIN(6, float_number(min, max)));
		return (gain);
	}

	else
		return ((max + min) / 2);
}


/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each char_class every time they gain a level.
 */
void
advance_level(struct Creature *ch, byte keep_internal)
{
	int add_hp[2], add_mana[2], add_move[2], i, char_class;
	char *msg;

	add_hp[0] = add_hp[1] = con_app[GET_CON(ch)].hitp;
	add_mana[0] = add_mana[1] = mana_bonus[GET_WIS(ch)];
	add_move[0] = add_move[1] = MAX(0, GET_CON(ch) - 15);

	for (i = 0; i < 2; i++) {

		if (i == 0)
			char_class = MIN(GET_CLASS(ch), NUM_CLASSES - 1);
		else
			char_class = MIN(GET_REMORT_CLASS(ch), NUM_CLASSES - 1);
		if (char_class < 0)
			continue;

		switch (char_class) {
		case CLASS_MAGIC_USER:
			add_hp[i] /= 5;
			add_hp[i] += number(3, 10);
			add_mana[i] += number(1, 11) + (GET_LEVEL(ch) / 3);
			add_move[i] += number(1, 3);
			break;
		case CLASS_CLERIC:
			add_hp[i] /= 2;
			add_hp[i] += number(5, 11);
			add_mana[i] += number(1, 10) + (GET_LEVEL(ch) / 5);
			add_move[i] += number(1, 4);
			break;
		case CLASS_BARD:
            add_hp[i] = (int)(add_hp[i] / 3);
			add_hp[i] += number(5, 10);
			add_mana[i] += number(1, 4) + (GET_LEVEL(ch) / 10);
			add_move[i] += number(10, 18);
			break;
		case CLASS_THIEF:
			add_hp[i] /= 3;
			add_hp[i] += number(4, 10);
			add_mana[i] = (int)(add_mana[i] * 0.3);
			add_move[i] += number(2, 6);
			break;
		case CLASS_MERCENARY:
			add_hp[i] += number(6, 14);
			add_mana[i] = (int)(add_mana[i] * 0.5);
			add_mana[i] += number(1, 5) + GET_LEVEL(ch) / 10;
			add_move[i] += number(3, 9);
			break;
		case CLASS_WARRIOR:
			add_hp[i] += number(10, 15);
			add_mana[i] = (int)(add_mana[i] * 0.4);
			add_mana[i] += number(1, 5);
			add_move[i] += number(5, 10);
			break;
		case CLASS_BARB:
			add_hp[i] += number(13, 18);
			add_mana[i] = (int)(add_mana[i] * 0.3);
			add_mana[i] += number(0, 3);
			add_move[i] += number(5, 10);
			break;
		case CLASS_KNIGHT:
			add_hp[i] += number(7, 13);
			add_mana[i] = (int)(add_mana[i] * 0.7);
			add_mana[i] += number(1, 4) + (GET_LEVEL(ch) / 15);
			add_move[i] += number(3, 8);
			break;
		case CLASS_RANGER:
			add_hp[i] += number(4, 11);
			add_mana[i] = (int)(add_mana[i] * 0.6);
			add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 8);
			add_move[i] += number(6, 14);
			break;
		case CLASS_PSIONIC:
			add_hp[i] /= 3;
			add_hp[i] += number(3, 8);
			add_mana[i] = (int)(add_mana[i] * 0.6);
			add_mana[i] += number(1, 7) + (GET_LEVEL(ch) / 5);
			add_move[i] += number(2, 6);
			break;
		case CLASS_PHYSIC:
			add_hp[i] /= 4;
			add_hp[i] += number(4, 9);
			add_mana[i] = (int)(add_mana[i] * 0.6);
			add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 3);
			add_move[i] += number(2, 10);
			break;
		case CLASS_CYBORG:
			add_hp[i] /= 2;
			add_hp[i] += number(6, 14);
			add_mana[i] = (int)(add_mana[i] * 0.3);
			add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 15);
			add_move[i] += number(5, 8);
			break;
		case CLASS_MONK:
			add_hp[i] /= 3;
			add_hp[i] += number(6, 12);
			add_mana[i] = (int)(add_mana[i] * 0.3);
			add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 22);
			add_move[i] += number(6, 9);
			break;
		default:
			add_hp[i] /= 2;
			add_hp[i] += number(5, 16);
			add_mana[i] = (int)(add_mana[i] * 0.5);
			add_mana[i] += number(1, 13) + (GET_LEVEL(ch) / 4);
			add_move[i] += number(7, 15);
			break;
		}
	}

	if (IS_RACE(ch, RACE_HALF_ORC) || IS_RACE(ch, RACE_ORC)) {
		add_move[0] <<= 1;
		add_move[1] <<= 1;
	}
	ch->points.max_hit += MAX(1, add_hp[0]);
	ch->points.max_move += MAX(1, add_move[0]);

	if (GET_LEVEL(ch) > 1)
		ch->points.max_mana += add_mana[0];

	GET_LIFE_POINTS(ch) += (GET_LEVEL(ch) * (GET_WIS(ch) + GET_CON(ch))) / 300;

	if (IS_REMORT(ch) && GET_REMORT_GEN(ch)) {

		if (add_hp[0] < 0 || add_hp[1] < 0) {
			errlog("remort level (%s) add_hp: [0]=%d,[1]=%d",
				GET_NAME(ch), add_hp[0], add_hp[1]);
		}

		ch->points.max_hit += add_hp[1] >> 2;
		ch->points.max_mana += add_mana[1] >> 1;
		ch->points.max_move += add_move[1] >> 2;

	}

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		for (i = 0; i < 3; i++)
			GET_COND(ch, i) = (char)-1;
		SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
		SET_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
	}
	if (GET_LEVEL(ch) == 10)
		SET_BIT(PRF2_FLAGS(ch), PRF2_NEWBIE_HELPER);

	// special section for improving read_scrolls and use_wands
	if (CHECK_SKILL(ch, SKILL_READ_SCROLLS) > 10)
		GET_SKILL(ch, SKILL_READ_SCROLLS) =
			MIN(100, CHECK_SKILL(ch, SKILL_READ_SCROLLS) +
			MIN(10, number(1, GET_INT(ch) >> 1)));
	if (CHECK_SKILL(ch, SKILL_USE_WANDS) > 10)
		GET_SKILL(ch, SKILL_USE_WANDS) =
			MIN(100, CHECK_SKILL(ch, SKILL_USE_WANDS) +
			MIN(10, number(1, GET_INT(ch) >> 1)));

	ch->saveToXML();
    int rid = -1;
    if( ch->in_room != NULL )
        rid = ch->in_room->number;
	msg = tmp_sprintf("%s advanced to level %d in room %d%s",
		GET_NAME(ch), GET_LEVEL(ch), rid,
		ch->isTester() ? " <TESTER>" : "");
	if (keep_internal)
		slog(msg);
	else
		mudlog(GET_INVIS_LVL(ch), BRF, true, "%s", msg);
}


/*
 * invalid_char_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular char_class, based on the ITEM_ANTI_{char_class} bitvectors.
 */

int
invalid_char_class(struct Creature *ch, struct obj_data *obj)
{
	int invalid = 0;
	int foundreq = 0;

	if( IS_PC(ch) && 
		obj->shared->owner_id != 0 && 
		obj->shared->owner_id != GET_IDNUM(ch) ) {
		invalid = 1;
	}

	if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_BARB) && IS_BARB(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_PSYCHIC) && IS_PSYCHIC(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_PHYSIC) && IS_PHYSIC(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_CYBORG) && IS_CYBORG(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_KNIGHT) && IS_KNIGHT(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch)) ||
		(IS_OBJ_STAT2(obj, ITEM2_ANTI_MERC) && IS_MERC(ch)) ||
		(IS_OBJ_STAT(obj, ITEM_ANTI_MONK) && IS_MONK(ch)) ||
		(!OBJ_APPROVED(obj) && !ch->isTester()
			&& GET_LEVEL(ch) < LVL_IMMORT))
		invalid = 1;
	if (!invalid) {
		if (IS_OBJ_STAT3(obj, ITEM3_REQ_MAGE))
			if (IS_MAGE(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_CLERIC))
			if (IS_CLERIC(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_THIEF))
			if (IS_THIEF(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_WARRIOR))
			if (IS_WARRIOR(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_BARB))
			if (IS_BARB(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_PSIONIC))
			if (IS_PSIONIC(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_PHYSIC))
			if (IS_PHYSIC(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_CYBORG))
			if (IS_CYBORG(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_KNIGHT))
			if (IS_KNIGHT(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_RANGER))
			if (IS_RANGER(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_BARD))
			if (IS_BARD(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_MONK))
			if (IS_MONK(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_VAMPIRE))
			if (IS_VAMPIRE(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_MERCENARY))
			if (IS_MERC(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE1))
			if (IS_SPARE1(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE2))
			if (IS_SPARE2(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
		if (!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE3))
			if (IS_SPARE3(ch)) {
				foundreq = 1;
				invalid = 0;
			} else
				invalid = 1;
	}
	return invalid;
}

int
char_class_race_hit_bonus(struct Creature *ch, struct Creature *vict)
{
	int bonus = 0;
	bonus += (IS_DWARF(ch) && (IS_OGRE(vict) || IS_TROLL(vict) ||
			IS_GIANT(vict) || (GET_HEIGHT(vict) > 2 * GET_HEIGHT(ch))));
	bonus -= (IS_DWARF(ch) && (SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
			SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM ||
			ch->in_room->isOpenAir() ||
			SECT_TYPE(ch->in_room) == SECT_UNDERWATER ||
			SECT_TYPE(ch->in_room) == SECT_DEEP_OCEAN));
	bonus += (IS_THIEF(ch) && room_is_dark(ch->in_room));
	bonus += (IS_RANGER(ch) && (SECT_TYPE(ch->in_room) == SECT_FOREST ||
			(SECT_TYPE(ch->in_room) != SECT_CITY &&
				SECT_TYPE(ch->in_room) != SECT_INSIDE && OUTSIDE(ch))));
	bonus += (IS_TABAXI(ch) && SECT_TYPE(ch->in_room) == SECT_FOREST &&
		OUTSIDE(ch));
	return (bonus);
}

extern const int exp_scale[LVL_GRIMP + 2] = {
	0,
	1,
	2500,
	6150,
	11450,
	19150,		 /***   5  ***/
	30150,
	45650,
	67600,
	98100,
	140500,		  /***  10  ***/
	199500,
	281500,
	391500,
	541000,
	746000,		  /***  15  ***/
	1025000,
	1400000,
	1900000,
	2550000,
	3400000,	   /***  20  ***/
	4500000,
	5900000,
	7700000,
	10050000,
	12950000,		/***  25  ***/
	16550000,
	21050000,
	26650000,
	33650000,
	42350000,		/***  30  ***/
	52800000,
	65300000,
	79800000,
	96800000,
	116500000,		 /***  35  ***/
	140000000,
	167000000,
	198000000,
	233500000,
	274500000,		 /***  40  ***/
	320500000,
	371500000,
	426500000,
	486500000,
	551000000,		 /***  45  ***/
	622000000,
	699000000,
	783000000,
	869000000,
	1000000000,		  /***  50  ***/
	1100000000,
	1200000000,
	1300000000,
	1400000000,
	1500000000,		  /***  55  ***/
	1600000000,
	1700000000,
	1800000000,
	1900000000,
	2000000000,		 /***  60  ***/
	2000000001,
	2000000002,
	2000000003,
	2000000004,
	2000000005,
	2000000006,
	2000000007,
	2000000008,
	2000000009,
	2000000010,
	2000000011,		/*** 71 ***/
	2000000012,
	2000000013
};


/* Names of char_class/levels and exp required for each level */

int
correct_race( int race ) {
    switch( race ) {
        case RACE_ANIMAL:
        case RACE_DRAGON:
        case RACE_GIANT:
        case RACE_UNDEAD:
        case RACE_TROLL:
        case RACE_OGRE:
        case RACE_INSECT:
        case RACE_GOBLIN:
        case RACE_ELEMENTAL:
        case RACE_DUERGAR:
        case RACE_DEVIL:
        case RACE_DEVA:
        case RACE_DEMON:
        case RACE_ARCHON:
        case RACE_ALIEN_1:
            return RACE_HUMAN;
    }
    return race;
}

int
get_max_str( Creature *ch ) {
    return MIN(GET_REMORT_GEN(ch) + 18 +
                ((IS_NPC(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR) ? 8 : 0) +
                (IS_MINOTAUR(ch) ? 2 : 0) +
                (IS_DWARF(ch) ? 1 : 0) +
                (IS_HALF_ORC(ch) ? 2 : 0) + (IS_ORC(ch) ? 1 : 0), 25);
}

int
get_max_int( Creature *ch ) {
    return (IS_NPC(ch) ? 25 :
                MIN(25,
                    18 + (IS_REMORT(ch) ? GET_REMORT_GEN(ch) : 0) +
                    ((IS_ELF(ch) || IS_DROW(ch)) ? 1 : 0) +
                    (IS_MINOTAUR(ch) ? -2 : 0) +
                    (IS_TABAXI(ch) ? -1 : 0) +
                    (IS_ORC(ch) ? -1 : 0) + (IS_HALF_ORC(ch) ? -1 : 0)));
}

int
get_max_wis( Creature *ch ) {
    return (IS_NPC(ch) ? 25 :
                MIN(25, (18 + GET_REMORT_GEN(ch)) +
                    (IS_MINOTAUR(ch) ? -2 : 0) + (IS_HALF_ORC(ch) ? -2 : 0) +
                    (IS_TABAXI(ch) ? -2 : 0)));
}

int
get_max_dex( Creature *ch ) {
    return (IS_NPC(ch) ? 25 :
                MIN(25,
                    18 + (IS_REMORT(ch) ? GET_REMORT_GEN(ch) : 0) +
                    (IS_TABAXI(ch) ? 2 : 0) +
                    ((IS_ELF(ch) || IS_DROW(ch)) ? 1 : 0)));
}

int
get_max_con( Creature *ch ) {
    return (IS_NPC(ch) ? 25 :
                MIN(25,
                    18 + (IS_REMORT(ch) ? GET_REMORT_GEN(ch) : 0) +
                    ((IS_MINOTAUR(ch) || IS_DWARF(ch)) ? 1 : 0) +
                    (IS_TABAXI(ch) ? 1 : 0) +
                    (IS_HALF_ORC(ch) ? 1 : 0) +
                    (IS_ORC(ch) ? 2 : 0) +
                    ((IS_ELF(ch) || IS_DROW(ch)) ? -1 : 0)));
}

int
get_max_cha( Creature *ch ) {
    return (IS_NPC(ch) ? 25 :
                MIN(25,
                    18 + (IS_REMORT(ch) ? GET_REMORT_GEN(ch) : 0) +
                    (IS_HALF_ORC(ch) ? -3 : 0) +
                    (IS_ORC(ch) ? -3 : 0) +
                    (IS_DWARF(ch) ? -1 : 0) + (IS_TABAXI(ch) ? -2 : 0)));
}


void
calculate_height_weight( Creature *ch ) 
{
    /* make favors for sex ... and race */ // after keep
	if (ch->player.sex == SEX_MALE) {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(110, 160) + GET_STR(ch);
			ch->player.height = number(160, 200) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_HALFLING) {
			ch->player.weight = number(70, 80) + GET_STR(ch);
			ch->player.height = number(81, 100) + (GET_WEIGHT(ch) / 16);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(120, 160) + GET_STR(ch);
			ch->player.height = number(100, 125) + (GET_WEIGHT(ch) / 16);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(140, 165) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(120, 200) + (GET_WEIGHT(ch) / 16);
		} else if (GET_RACE(ch) == RACE_MINOTAUR) {
			ch->player.weight = number(200, 360) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) / 8);
		} else {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) / 8);
		}
	} else {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(80, 120) + GET_STR(ch);
			ch->player.height = number(160, 190) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_HALFLING) {
			ch->player.weight = number(70, 80) + GET_STR(ch);
			ch->player.height = number(81, 100) + (GET_WEIGHT(ch) / 16);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(100, 140) + GET_STR(ch);
			ch->player.height = number(90, 115) + (GET_WEIGHT(ch) / 16);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(90, 130) + GET_STR(ch);
			ch->player.height = number(120, 155) + (GET_WEIGHT(ch) / 8);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(110, 170) + GET_STR(ch);
			ch->player.height = number(110, 190) + (GET_WEIGHT(ch) / 8);
		} else {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) / 8);
		}
	}
}
#undef __char_class_c__
