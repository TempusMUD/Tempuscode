/* ************************************************************************
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

extern struct obj_data *read_object(int vnum);
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

/* #define LEARNED_LEVEL        0  % known which is considered "learned" */
/* #define MAX_PER_PRAC                1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC                2  min percent gain in skill per practice */
/* #define PRAC_TYPE                3  should it say 'spell' or 'skill'?        */

extern const int prac_params[4][NUM_CLASSES] = {
/* MG  CL  TH  WR  BR  PS  PH  CY  KN  RN  HD  MN  VP  MR  S1  S2  S3*/
    {75, 75, 70, 70, 65, 75, 75, 80, 75, 75, 75, 75, 75, 70, 70, 70, 70} ,
    {25, 20, 20, 20, 20, 25, 20, 30, 20, 25, 25, 20, 15, 25, 25, 25, 25},
    {15, 15, 10, 15, 10, 15, 15, 15, 15, 15, 15, 10, 10, 10, 10, 10, 10},
    {SPL,SPL,SKL,SKL,SKL,TRG,ALT,PRG,SPL,SPL,SKL,ZEN,SPL,SKL,SKL,SKL,SKL} 

};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 *        #define ALL                0
 *        #define GOOD                1
 *        #define NEUTRAL                2
 *        #define EVIL                3
 */

extern const int guild_info[][4] = {

    /* Modrian */
    {CLASS_MAGIC_USER,           ALL,        3023,        SCMD_UP},
    {CLASS_CLERIC,        GOOD,        3007,        SCMD_EAST},
    {CLASS_THIEF,                ALL,        3033,        SCMD_EAST},
    {CLASS_BARB,          ALL,        3040,   SCMD_UP},
    {CLASS_KNIGHT,        GOOD,        3071,   SCMD_NORTH},
    {CLASS_RANGER,        ALL,        3079,   SCMD_WEST},
    {CLASS_THIEF,         ALL,        2949,   SCMD_UP},
    {CLASS_THIEF,         EVIL,        2945,   SCMD_WEST},
    {CLASS_KNIGHT,        EVIL,        3098,   SCMD_SOUTH},
    {CLASS_CLERIC,        EVIL,   2897,   SCMD_WEST},

    /* Electro Centralis */
    {CLASS_MONK,                ALL,   30156,   SCMD_WEST},
    {CLASS_PHYSIC,        ALL,   30009,   SCMD_WEST},
    {CLASS_HOOD,                ALL,   33099,        SCMD_UP},
    {CLASS_PSIONIC,       ALL,   30081,   SCMD_UP},
    {CLASS_MERCENARY,     ALL,   30235,   SCMD_EAST},

    {CLASS_CLERIC,       GOOD,   30850,   SCMD_NORTH},
    {CLASS_CLERIC,       EVIL,   33151,   SCMD_WEST},

    {CLASS_THIEF,        ALL,   33155,   SCMD_WEST},
    {CLASS_THIEF,        ALL,   30207,   SCMD_DOWN},

    {CLASS_RANGER,       ALL,   30079,   SCMD_NORTH},
    /* sub level electro */
    {CLASS_HOOD,         EVIL,   33125,   SCMD_SOUTH},

    /* tower of guiharia */
    {CLASS_CLERIC,        GOOD,   11133,   SCMD_SOUTH},
    /* Brass Dragon */
    {-999 /* all */ ,        ALL,    5065,        SCMD_WEST},

    /* New Thalos */
    {CLASS_MAGIC_USER,        ALL,    5525,        SCMD_NORTH},
    {CLASS_THIEF,         ALL,          5532,         SCMD_SOUTH},
    {CLASS_BARB,                 ALL,          5526,         SCMD_SOUTH},
    {CLASS_CLERIC,        GOOD,          5512,         SCMD_SOUTH},
    {CLASS_KNIGHT,        GOOD,   5610,   SCMD_EAST},
    {CLASS_CLERIC,        EVIL,   5613,   SCMD_WEST},
    {CLASS_KNIGHT,        EVIL,   5616,   SCMD_WEST},

    /* Istan */
    {CLASS_BARB,                 ALL,          20405,         SCMD_EAST},
    {CLASS_THIEF,             ALL,          20447,         SCMD_EAST},
    {CLASS_RANGER,             ALL,          20457,         SCMD_SOUTH},
  
    /* High Tower of Magic */
    {CLASS_MAGIC_USER,    ALL,    2568,   SCMD_UP},
    {CLASS_MAGIC_USER,    ALL,    2598,   SCMD_UP},
    {CLASS_MAGIC_USER,    NEUTRAL, 2653,  SCMD_SOUTH},
    {CLASS_MAGIC_USER,    GOOD,   2667,   SCMD_WEST},
    {CLASS_MAGIC_USER,    EVIL,   2661,   SCMD_EAST},

    /* Monk Guild */
    {CLASS_MONK,          NEUTRAL, 21014, SCMD_NORTH},
    {CLASS_MONK,          NEUTRAL, 21012, SCMD_NORTH},

    /* Solace Cove */
    {CLASS_RANGER,       ALL,      63022,   SCMD_SOUTH},
    {CLASS_CLERIC,       GOOD,     63039,   SCMD_NORTH},
    {CLASS_KNIGHT,       GOOD,     63027,   SCMD_SOUTH},
    {CLASS_KNIGHT,       EVIL,     63027,   SCMD_SOUTH},
    {CLASS_THIEF,         ALL,     63150,   SCMD_EAST},
    {CLASS_MAGIC_USER,    ALL,     63015,   SCMD_NORTH},
    {CLASS_BARB,          ALL,     63005,   SCMD_WEST},
  
    /* Elven Village */
    {CLASS_RANGER,      GOOD,      19015,   SCMD_WEST},

    /* skullport */
    {CLASS_MAGIC_USER,   ALL,      22698,   SCMD_WEST},
    {CLASS_KNIGHT,      EVIL,      22932,   SCMD_EAST},
    {CLASS_THIEF,        ALL,      22606,   SCMD_EAST},
    {CLASS_THIEF,        ALL,      22623,   SCMD_EAST},
    {CLASS_CLERIC,      EVIL,      22930,   SCMD_WEST},
    {CLASS_BARB,         ALL,      22815,   SCMD_SOUTH},

    /* this must go last -- add new guards above! */
    {-1, -1, -1, -1}};


/* THAC0 for char_classes and levels.  (To Hit Armor Class 0) */

extern const float thaco_factor[NUM_CLASSES] = {
    0.15,  /* mage    */
    0.20,  /* cleric  */
    0.25,  /* thief   */
    0.30,  /* warrior */
    0.40,  /* barb    */
    0.20,  /* psionic */
    0.15,  /* physic  */
    0.30,  /* cyborg  */
    0.35,  /* knight  */
    0.35,  /* ranger  */
    0.35,  /* hood    */
    0.40,  /* monk    */
    0.40,  /* vampire */
    0.35,  /* merc    */
    0.30,  /* spare1  */
    0.30,  /* spare2  */
    0.30   /* spare3  */
};

void 
gain_skill_prof(struct char_data *ch, int skl)
{
    int learned;
    if (skl == SKILL_READ_SCROLLS || skl == SKILL_USE_WANDS)
        learned = 10;
    else
        learned = LEARNED(ch);
  
    if (IS_NPC(ch))
        return;
    if (GET_SKILL(ch, skl) >= (learned - 10)) 
        if ((GET_SKILL(ch, skl)-GET_LEVEL(ch)) <= 60 &&!number(0, GET_LEVEL(ch))) 
            GET_SKILL(ch, skl) += 1;
}


/* Names first */
extern const char *char_class_abbrevs[] = {
    "Mage",                        /* 0 */
    "Cler",
    "Thie",
    "Warr",
    "Barb",
    "Psio",                        /* 5 */
    "Phyz",
    "Borg",
    "Knig",
    "Rang",
    "Hood",                        /* 10 */
    "Monk",
    "Vamp", 
    "Merc", 
    "Spa1", 
    "Spa2",                /* 15 */                
    "Spa3", 
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",/*25*/
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",/*35*/
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",/*45*/
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
    "ERR","ERR","ERR","ERR","ERR",
    "Gren",
    "Whte", 
    "Blck", 
    "Blue", 
    "Red",                                                         /* 95 */
    "Slvr", 
    "Shad", 
    "Deep", 
    "Turt", 
    "ILL",  /* 100 */
    "Lest",
    "Lssr",
    "Grtr",
    "Duke",
    "Arch",                                                        /*105*/
    "ILL","ILL","ILL","ILL","ILL",                                /*110*/
    "Hill", "Ston", "Frst", "Fire", "Clod",                    /*115*/
    "Strm",              
    "ILL", "ILL", "ILL",
    "Red",                                                        /* 120 */
    "Blue",
    "Gren",
    "Grey",
    "Deth",
    "Lord",        /* 125 */
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
    "Prnc",                           /* 140 */
    "ILL",
    "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",
    "Astl",                                /* Devas */   /* 150 */
    "Mond",
    "Movn",
    "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL", "ILL", // 162
    "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL", "ILL", // 172
    "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL", "ILL", // 182
    "\n"
};

extern const char *pc_char_class_types[] = {
    "Mage",
    "Cleric",
    "Thief",
    "Warrior",
    "Barbarian",
    "Psionic",                                /* 5 */
    "Physic",
    "Cyborg",
    "Knight",
    "Ranger",
    "Hoodlum",                                /* 10 */
    "Monk",
    "Vampire",
    "Mercenary", 
    "Spare1", 
    "Spare2",              /* 15 */
    "Spare3",
    "ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL",        /* 25 */
    "ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL",        /* 35 */
    "ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL",        /* 45 */
    "ILL","ILL","ILL","ILL",
    "Mobile",                                                        /* 50 */
    "Bird",
    "Predator",
    "Snake",
    "Horse", 
    "Small",
    "Medium",
    "Large",
    "Scientist",
    "ILL", "Skeleton",                                /* 60 */
    "Ghoul", "Shadow", "Wight", "Wraith", "Mummy",                /* 65 */
    "Spectre", "Vampire", "Ghost", "Lich", "Zombie",                /* 70 */  
    "ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL","ILL",        /* 80 */
    "Earth", "Fire", "Water", "Air", "Lighting",                        /* 85 */
    "ILL","ILL","ILL","ILL","ILL",                                /* 90 */
    "Green",
    "White", 
    "Black", 
    "Blue", 
    "Red",                                                         /* 95 */
    "Silver", 
    "Shadow_D", 
    "Deep", 
    "Turtle", "ILL",  /* 100 */
    "Least",
    "Lesser",
    "Greater",
    "Duke",
    "Arch",                                                        /*105*/
    "ILL","ILL","ILL","ILL","ILL",                                /*110*/
    "Hill", "Stone", "Frost", "Fire", "Cloud",                    /*115*/
    "Storm",              
    "ILL", "ILL", "ILL",
    "Red",                                /* Slaad */        /* 120 */
    "Blue",
    "Green",
    "Grey",
    "Death",
    "Lord",                                                        /* 125 */
    "ILL", "ILL", "ILL", "ILL",
    "Type I",                                /* Demons */      /* 130 */
    "Type II",
    "Type III",
    "Type IV",
    "Type V",
    "Type VI",
    "Semi",
    "Minor",
    "Major",
    "Lord",
    "Prince",                           /* 140 */
    "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",  "ILL",
    "Astral",                                /* Devas */   /* 150 */
    "Monadic",
    "Movanic",
    "ILL", "ILL", "ILL",
    "ILL", "ILL", "ILL", "ILL",
    "Fire",                                 /* Mephits 160 */
    "Lava",
    "Smoke",
    "Steam",
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
    "Arcana",                                 // daemons 170
    "Charona",
    "Dergho",
    "Hydro",
    "Pisco",
    "Ultro",
    "Yagno",
    "Pyro", // 177
    "\n"
};


/* The menu for choosing a char_class in interpreter.c: */
extern const char *char_class_menu =
"\r\n"
"Select a char_class:\r\n"
"  [C]leric\r\n"
"  [T]hief\r\n"
"  [W]arrior\r\n"
"  [B]arbarian\r\n"
"  [M]agic-user\r\n"
"  P[s]ychic\r\n"
"  [P]hysic\r\n"
"  C[y]borg\r\n"
"  [K]night\r\n"
"  [R]anger\r\n"
"  [H]oodlum\r\n"
"  Bou[n]ty Hunter\r\n";


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
parse_char_class_past(char *arg)
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

    return CLASS_UNDEFINED;
}

int 
parse_char_class_future(char *arg)
{

    skip_spaces(&arg);
    if (is_abbrev(arg, "cyborg") || is_abbrev(arg, "borg"))
        return CLASS_CYBORG;
    else if (is_abbrev(arg, "hoodlum"))
        return CLASS_HOOD;
    else if (is_abbrev(arg, "psionic") || is_abbrev(arg, "psychic"))
        return CLASS_PSIONIC;
    else if (is_abbrev(arg, "physic") || is_abbrev(arg, "physicist"))
        return CLASS_PHYSIC;
    else if (is_abbrev(arg, "monk"))
        return CLASS_MONK;
    else if (is_abbrev(arg, "mercenary"))
        return CLASS_MERCENARY;

    return CLASS_UNDEFINED;
}


extern const char *player_race[] = {
    "Human",
    "Elf",
    "Dwarf",
    "Half Orc",
    "Klingon",
    "Halfling",                                                        /* 5 */
    "Tabaxi",
    "Drow",
    "ILL","ILL", 
    "Mobile",                                                        /* 10 */
    "Undead",
    "Humanoid",
    "Animal",
    "Dragon",
    "Giant",                                                        /* 15 */
    "Orc",
    "Goblin",
    "Halfling",
    "Minotaur",
    "Troll",                                                        /* 20 */
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
    "Illithid",      /* 40 */
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
	"Spider",
    "\n"
};

extern const int race_lifespan[] = {
    80,             /* human */
    400,           /* elf */
    160,            /* dwarf */
    55,             /* half orc */
    100,            /* klingon */
    100,            /* halfling */
    80,            /* tabaxi */
    200,              /* drow */
    0,
    0,
    0,              /* mobile */
    10000,          /* undead */
    100,            /* humanoid */
    50,             /* animal */
    10000,          /* dragon */
    500,            /* giant */
    65,             /* orc */
    50,             /* goblin */
    50,             /* halfling */
    200,            /* minotaur */
    500             /* troll */
};

int
parse_race(char *arg)
{
    int j;

    for (j = 0; j < NUM_RACES; j++)
        if (is_abbrev(arg,  player_race[j]))
            return j;

    return (-1);
}

int 
parse_char_class(char *arg)
{
    int j;

    for (j = 0; j < TOP_CLASS; j++)
        if (is_abbrev(arg,  pc_char_class_types[j]))
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
    arg = LOWER(arg);

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
    case 'n' :
        return (1 << 11);
        break;
    case 'e' :
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
roll_real_abils(struct char_data * ch)
{
    int i, j, k, temp;
    ubyte table[6];
    ubyte rolls[4];

    for (i = 0; i < 6; i++)
        table[i] = 0;

    for (i = 0; i < 6; i++) {

        for (j = 0; j < 4; j++)
            rolls[j] = number(1, 6);

        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
            MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

        for (k = 0; k < 6; k++)
            if (table[k] < temp) {
                temp ^= table[k];
                table[k] ^= temp;
                temp ^= table[k];
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
            ch->real_abils.str_add = number(0, 100);
        break;
    case CLASS_BARB:   
        ch->real_abils.str = table[0];
        ch->real_abils.con = table[1];
        ch->real_abils.dex = table[2];
        ch->real_abils.wis = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = number(0, 100);
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
            ch->real_abils.dex   = table[0];
            ch->real_abils.con   = table[1];
            ch->real_abils.str   = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.wis   = table[4];
            ch->real_abils.cha   = table[5];
            break;
        default:
            ch->real_abils.str   = table[0];
            ch->real_abils.con   = table[1];
            ch->real_abils.dex   = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.wis   = table[4];
            ch->real_abils.cha   = table[5];
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
            ch->real_abils.str_add = number(0, 100);
        break;
    case CLASS_RANGER:
        ch->real_abils.dex = table[0];
        ch->real_abils.wis = table[1];
        ch->real_abils.con = table[2];
        ch->real_abils.str = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = number(0, 100);
        break;
    case CLASS_HOOD:  
        ch->real_abils.con = table[0];
        ch->real_abils.str = table[1];
        ch->real_abils.dex = table[2];
        ch->real_abils.intel = table[3];
        ch->real_abils.wis = table[4];
        ch->real_abils.cha = table[5];
        break;
    case CLASS_MONK:
        ch->real_abils.dex =   table[0];
        ch->real_abils.con =   table[1];
        ch->real_abils.str =   table[2];
        ch->real_abils.wis =   table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha =   table[5];
        break;
    case CLASS_VAMPIRE:
        ch->real_abils.dex =   table[0];
        ch->real_abils.con =   table[1];
        ch->real_abils.str =   table[2];
        ch->real_abils.intel = table[3];
        ch->real_abils.wis =   table[4];
        ch->real_abils.cha =   table[5];
        break;
    case CLASS_MERCENARY:
        ch->real_abils.str =   table[0];
        ch->real_abils.con =   table[1];
        ch->real_abils.dex =   table[2];
        ch->real_abils.intel = table[3];
        ch->real_abils.wis =   table[4];
        ch->real_abils.cha =   table[5];
        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = number(0, 100);
        break;
    default:
        ch->real_abils.dex =   table[0];
        ch->real_abils.con =   table[1];
        ch->real_abils.str =   table[2];
        ch->real_abils.wis =   table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha =   table[5];
        break;
    }
    switch (GET_RACE(ch)) {
    case RACE_ELF:
    case RACE_DROW:
        ch->real_abils.intel += 1;
        ch->real_abils.dex += 1;
        ch->real_abils.con -= 1;
        break;
    case RACE_DWARF:
        ch->real_abils.con += 1;
        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
        else ch->real_abils.str += 1;
        ch->real_abils.cha -= 1;
        break;
    case RACE_HALF_ORC:
        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
        else ch->real_abils.str += 1;

        if (ch->real_abils.str == 18)
            ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
        else if (ch->real_abils.str < 18)
            ch->real_abils.str += 1;

        ch->real_abils.con += 1;
        ch->real_abils.cha -= 3;
        break;
    case RACE_TABAXI:
        ch->real_abils.dex  = MIN(20, ch->real_abils.dex + 3);
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
                ch->real_abils.str = MIN(20, 18+((ch->real_abils.str_add - 100) / 10));
                ch->real_abils.str_add = 0;
            } else
                ch->real_abils.str = 18;
        }
        break;

    case RACE_HUMAN:
       switch (GET_CLASS(ch)) {
       case CLASS_MAGIC_USER:
          ch->real_abils.intel +=1;
               ch->real_abils.dex +=1;
          break;
       case CLASS_CLERIC:
          ch->real_abils.intel +=1;
               ch->real_abils.wis +=1;
          break;
       case CLASS_BARB:
          if (ch->real_abils.str == 18)
                       ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
               else 
             ch->real_abils.str += 1;
               ch->real_abils.con +=1;
          break;
       case CLASS_RANGER:
          ch->real_abils.intel +=1;
               ch->real_abils.dex +=1;
          break;
       case CLASS_THIEF:
          ch->real_abils.intel +=1;
               ch->real_abils.dex +=1;
          break;
       case CLASS_KNIGHT:
          if (ch->real_abils.str == 18)
             ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
               else 
             ch->real_abils.str += 1;
               ch->real_abils.wis +=1;
          break;
       case CLASS_PSIONIC:
          ch->real_abils.intel +=1;
          ch->real_abils.wis +=1;
          break;
       case CLASS_PHYSIC:
          ch->real_abils.intel +=1;
          ch->real_abils.wis +=1;
          break;
       case CLASS_CYBORG:
          switch (GET_OLD_CLASS(ch)) {
          case BORG_MENTANT:
             ch->real_abils.intel +=1;
             ch->real_abils.wis +=1;
             break;       
          case BORG_SPEED:
             ch->real_abils.dex +=1;
             ch->real_abils.intel +=1;
                        break;
          case BORG_POWER:
             if (ch->real_abils.str == 18)
                ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
                  else 
                ch->real_abils.str +=1;
             break;
               default:
                       break;
               }
          break;
       case CLASS_HOOD:
          ch->real_abils.dex +=1;
          ch->real_abils.str +=1;
          break;
       case CLASS_MONK:
          ch->real_abils.dex +=1;
          ch->real_abils.wis +=1;
          break;
       case CLASS_MERCENARY:   
          if (ch->real_abils.str == 18)
             ch->real_abils.str_add = MIN(100, ch->real_abils.str_add + 10);
               else 
             ch->real_abils.str += 1;
          ch->real_abils.dex +=1;
          break;
       default:
          break;
       }
      break;
    }

    ch->aff_abils = ch->real_abils;
}

const int newbie_equipment[NUM_CLASSES][NUM_NEWBIE_EQ] = 
{
    {  214,  241,  237,  201,  202,  204,  206,  208,  200,  207},     /* mage */
    {  209,  205,  234,  201,  202,  204,  206,  208,  200,  207},     /* cleric */
    {  217,  244,  237,  201,  202,  204,  206,  208,  200,  207},     /* thief  */
    {   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1},     /* warrior */
    {  212,  239,  235,  201,  202,  204,  206,  208,  200,  207},     /* barbarian */
    {  215,  225,  247,  232,  222,  233,  230,  231,  203,  229},     /* psionic */
    {  219,  224,  246,  232,  222,  233,  230,  231,  203,  229},     /* physic */
    {  218,  223,  245,  232,  222,  233,  230,  231,  203,  229},     /* cyborg */
    {  213,  240,  236,  201,  202,  204,  206,  208,  200,  207},     /* knight */
    {  216,  243,  237,  201,  202,  204,  206,  208,  200,  207},     /* ranger */
    {  220,  226,  248,  232,  222,  233,  230,  231,  203,  229},     /* hood */
    {  210,  242,  238,  201,  202,  206,  208,  200,  207,   -1},     /* monk */
    {   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1},     /* vampire */
    {  221,  228,  250,  232,  233,  230,  231,  203,  229,  222},     /* mercenary */
    {   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1},     /* padding */
    {   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1},     /* padding */
    {   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1}      /* padding */
};

void 
newbie_equip(struct char_data *ch)
{

    int i, vnum;
    struct obj_data *obj = NULL;

    if (GET_CLASS(ch) < NUM_CLASSES) {
        for (i = 0; i < NUM_NEWBIE_EQ; i++) {
            if ((vnum = newbie_equipment[(int)GET_CLASS(ch)][i]) >= 0)
                if ((obj = read_object(vnum)))
                    obj_to_char(obj, ch);
        }
    }
 
    switch (GET_CLASS(ch)) {               /** special objects **/
    case CLASS_CLERIC:
        if (IS_GOOD(ch))
            obj = read_object(1280);
        else 
            obj = read_object(1260);

        if (obj)
            obj_to_char(obj, ch);
        break;
    case CLASS_KNIGHT:
        if (IS_GOOD(ch))
            obj = read_object(1287);
        else
            obj = read_object(1270);
        if (obj)
            obj_to_char(obj, ch);
        break;
    }

    if (!IS_RACE_INFRA(ch) && (obj = read_object(3030))) /* torch */
        obj_to_char(obj, ch);

}


/* Some initializations for characters, including initial skills */
void 
do_start(struct char_data * ch, int mode)
{
    void advance_level(struct char_data * ch, byte keep_internal);
    byte new_player = 0;
    int i, j;

    // remove implant affects
    for (i = 0; i < NUM_WEARS; i++) {

        if (ch->implants[i] && !invalid_char_class(ch, ch->implants[i]) &&
            (!IS_DEVICE(ch->implants[i]) || ENGINE_STATE(ch->implants[i]))) {
      
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
                affect_modify(ch, ch->implants[i]->affected[j].location,
                              ch->implants[i]->affected[j].modifier, 0, 0, FALSE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[0], 1, FALSE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[1], 2, FALSE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[2], 3, FALSE);
      
            if (IS_INTERFACE(ch->implants[i]) && 
                INTERFACE_TYPE(ch->implants[i]) == INTERFACE_CHIPS &&
                ch->implants[i]->contains) {
                check_interface(ch, ch->implants[i], FALSE);
        
            }
        }
    }

    if (GET_EXP(ch) == 0 && !IS_REMORT(ch) && !IS_VAMPIRE(ch))
        new_player = TRUE;
    
    GET_LEVEL(ch) = 1;
    GET_EXP(ch) = 1;

    set_title(ch, NULL);

    if (mode)
        roll_real_abils(ch);

    for (i = 1; i <= MAX_SKILLS; i++)
        SET_SKILL(ch, i, 0);

    GET_PRACTICES(ch) = 2;

    if (IS_VAMPIRE(ch))
        GET_LIFE_POINTS(ch) = 1;
    else
        GET_LIFE_POINTS(ch) = 3*(GET_WIS(ch) + GET_CON(ch))/40;

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
    case CLASS_HOOD:
        SET_SKILL(ch, SKILL_PUNCH, 15);
        break;
    case CLASS_MONK:
        SET_SKILL(ch, SKILL_PUNCH, 20);
        break;
    case CLASS_VAMPIRE:
        SET_SKILL(ch, SKILL_FEED, 20);
        break;
    case CLASS_MERCENARY:
        SET_SKILL(ch, SKILL_PUNCH, 20);

        break;

    }

    if (new_player) {
        newbie_equip(ch);
        if (PAST_CLASS(GET_CLASS(ch))) {
            ch->points.bank_gold = 8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
            ch->points.gold = 8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
        } else if (FUTURE_CLASS(GET_CLASS(ch))) {
            ch->points.credits = 8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
            ch->points.cash = 8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
        }
    }

    advance_level(ch, 0);

    GET_MAX_MOVE(ch) += GET_CON(ch);

    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);

    GET_COND(ch, THIRST) = 24;
    GET_COND(ch, FULL) = 24;
    GET_COND(ch, DRUNK) = 0;

    if (GET_REMORT_CLASS(ch) < 0) {
        ch->player.time.played = 0;
        ch->player.time.logon = time(0);
    }

    // re-add implant affects
    for (i = 0; i < NUM_WEARS; i++) {
    
        if (ch->implants[i] && !invalid_char_class(ch, ch->implants[i]) &&
            (!IS_DEVICE(ch->implants[i]) || ENGINE_STATE(ch->implants[i]))) {
      
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
                affect_modify(ch, ch->implants[i]->affected[j].location,
                              ch->implants[i]->affected[j].modifier, 0, 0, FALSE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[0], 1, TRUE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[1], 2, TRUE);
            affect_modify(ch,0,0,ch->implants[i]->obj_flags.bitvector[2], 3, TRUE);
      
            if (IS_INTERFACE(ch->implants[i]) && 
                INTERFACE_TYPE(ch->implants[i]) == INTERFACE_CHIPS &&
                ch->implants[i]->contains) {
                check_interface(ch, ch->implants[i], FALSE);
        
            }
        }
    }
  
}

// prac_gain: mode==TRUE means to return a prac gain value
//            mode==FALSE means to return an average value
float
prac_gain(struct char_data *ch, int mode)
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
        return ((max+min) / 2);
}


/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each char_class every time they gain a level.
 */
void 
advance_level(struct char_data * ch, byte keep_internal)
{
    int add_hp[2],add_mana[2],add_move[2], i, char_class;

    add_hp[0]   = add_hp[1]   = con_app[GET_CON(ch)].hitp;
    add_mana[0] = add_mana[1] = mana_bonus[GET_WIS(ch)];
    add_move[0] = add_move[1] = MAX(0, GET_CON(ch)-15);

    for (i = 0; i < 2; i++) {

        if (i == 0)
            char_class = MIN(GET_CLASS(ch), NUM_CLASSES-1);
        else
            char_class = MIN(GET_REMORT_CLASS(ch), NUM_CLASSES-1);
        if (char_class < 0)
            continue;

        switch (char_class) {
        case CLASS_MAGIC_USER:
            add_hp[i]   /= 5;
            add_hp[i]   += number(3, 8);
            add_mana[i] += number(1, 11) + (GET_LEVEL(ch) / 3);
            add_move[i] += number(1, 3);
            break;
        case CLASS_CLERIC:
            add_hp[i]   /= 2;
            add_hp[i]   += number(5, 11);
            add_mana[i] += number(1, 10) + (GET_LEVEL(ch) / 5);
            add_move[i] += number(1, 4);
            break;
        case CLASS_THIEF:
            add_hp[i]   /= 3;
            add_hp[i]   += number(4, 10);
            add_mana[i] = (int) ( add_mana[i] * 0.3 );
            add_move[i] += number(2, 6);
            break;
        case CLASS_MERCENARY:
            add_hp[i]   += number(6, 14);
            add_mana[i] = (int) ( add_mana[i] * 0.5 );
            add_mana[i] += number(1, 5) + GET_LEVEL(ch) / 10;
            add_move[i] += number(3, 9);
            break;
        case CLASS_WARRIOR:
            add_hp[i]   += number(10, 15);
            add_mana[i] = (int) ( add_mana[i] * 0.4 );
            add_mana[i] += number(1, 5);
            add_move[i] += number(5, 10);
            break;
        case CLASS_BARB:
            add_hp[i]   += number (13, 18);
            add_mana[i] = (int) ( add_mana[i] * 0.3 );
            add_mana[i] += number(0, 3);
            add_move[i] += number (5, 10);
            break;
        case CLASS_KNIGHT:
            add_hp[i]   += number (7, 13);
            add_mana[i] = (int) ( add_mana[i] * 0.7 );
            add_mana[i] += number(1, 4) + (GET_LEVEL(ch) / 15);
            add_move[i] += number (3, 8);
            break;
        case CLASS_RANGER:
            add_hp[i]   += number (5, 13);
            add_mana[i] = (int) ( add_mana[i] * 0.6 );
            add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 8);
            add_move[i] += number (6, 14);
            break;
        case CLASS_PSIONIC:
            add_hp[i]   /= 3;
            add_hp[i]   += number   (3, 8);
            add_mana[i] = (int) ( add_mana[i] * 0.6 );
            add_mana[i] += number(1, 7) + (GET_LEVEL(ch) / 5);
            add_move[i] += number (2, 6);
            break;
        case CLASS_PHYSIC:
            add_hp[i]   /= 4;
            add_hp[i]   += number (4, 9);
            add_mana[i] = (int) ( add_mana[i] * 0.6 );
            add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 3);
            add_move[i] += number (2, 10);
            break;
        case CLASS_CYBORG:
            add_hp[i]   /= 2;
            add_hp[i]   += number (6, 14);
            add_mana[i] = (int) ( add_mana[i] * 0.3 );
            add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 15);
            add_move[i] += number (5, 8);
            break;
        case CLASS_HOOD: 
            add_hp[i]   /= 3;
            add_hp[i]   += number (6, 15);
            add_mana[i] = (int) ( add_mana[i] * 0.3 );
            add_move[i] += number (5, 10);
            break;
        case CLASS_MONK:
            add_hp[i]   /= 3;
            add_hp[i]   += number (6, 12);
            add_mana[i] = (int) ( add_mana[i] * 0.3 );
            add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 22);
            add_move[i] += number (6, 9);
            break;
        default:
            add_hp[i]   /= 2;
            add_hp[i]   += number (5, 16);
            add_mana[i] = (int) ( add_mana[i] * 0.5 );
            add_mana[i] += number(1, 13) + (GET_LEVEL(ch) / 4);
            add_move[i] += number (7, 15);
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
  
    GET_PRACTICES(ch) += (int)prac_gain(ch, 1);
  
    GET_LIFE_POINTS(ch) += (GET_LEVEL(ch) * 
                            (GET_WIS(ch)+GET_CON(ch))) / 300;
  
    if (IS_REMORT(ch) && GET_REMORT_GEN(ch)) {

        if (add_hp[0] < 0 || add_hp[1] < 0) {
            sprintf(buf, "SYSERR: remort level (%s) add_hp: [0]=%d,[1]=%d",
                    GET_NAME(ch), add_hp[0], add_hp[1]);
            slog(buf);
        }
    
        ch->points.max_hit +=  add_hp[1]   >> 2;
        ch->points.max_mana += add_mana[1] >> 1;
        ch->points.max_move += add_move[1] >> 2;
    
    }

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        for (i = 0; i < 3; i++)
            GET_COND(ch, i) = (char) -1;
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
  
    save_char(ch, NULL);

    sprintf(buf, "%s advanced to level %d in room %d %s", 
        GET_NAME(ch), GET_LEVEL(ch), ch->in_room->number,
        PLR_FLAGGED(ch, PLR_TESTER) ? "<TESTER>" : "");
    if (keep_internal)
        slog(buf);
    else
        mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
}


/*
 * invalid_char_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular char_class, based on the ITEM_ANTI_{char_class} bitvectors.
 */

int 
invalid_char_class(struct char_data *ch, struct obj_data *obj) {
    int invalid = 0;
    int foundreq = 0;
    if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC)     && IS_CLERIC(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR)    && IS_WARRIOR(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF)      && IS_THIEF(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_BARB)       && IS_BARB(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_PSYCHIC)    && IS_PSYCHIC(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_PHYSIC)     && IS_PHYSIC(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_CYBORG)     && IS_CYBORG(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_KNIGHT)     && IS_KNIGHT(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_RANGER)     && IS_RANGER(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_HOOD)       && IS_HOOD(ch)) ||
        (IS_OBJ_STAT2(obj, ITEM2_ANTI_MERC)       && IS_MERC(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_MONK)       && IS_MONK(ch)) ||
    (!OBJ_APPROVED(obj) && !PLR_FLAGGED(ch, PLR_TESTER) && GET_LEVEL(ch) < LVL_IMMORT))
        invalid = 1;
    if(!invalid) {
        if(IS_OBJ_STAT3(obj, ITEM3_REQ_MAGE))
            if(IS_MAGE(ch)) 
                {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_CLERIC))
            if(IS_CLERIC(ch)) 
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_THIEF))
            if(IS_THIEF(ch))
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_WARRIOR)) 
            if(IS_WARRIOR(ch)) 
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_BARB)) 
            if(IS_BARB(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_PSIONIC)) 
            if( IS_PSIONIC(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_PHYSIC)) 
            if(IS_PHYSIC(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_CYBORG)) 
            if(IS_CYBORG(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_KNIGHT))
            if(IS_KNIGHT(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_RANGER))
            if(IS_RANGER(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_HOOD))
            if(IS_HOOD(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_MONK))
            if(IS_MONK(ch))
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_VAMPIRE))
            if(IS_VAMPIRE(ch))
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_MERCENARY)) 
            if(IS_MERC(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE1))
            if(IS_SPARE1(ch))
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE2)) 
            if(IS_SPARE2(ch) )
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
        if(!foundreq && IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE3)) 
            if(IS_SPARE3(ch))
               {foundreq = 1; invalid = 0; }
            else 
                invalid = 1; 
    }
return invalid;
}

int 
char_class_race_hit_bonus(struct char_data *ch, struct char_data *vict)
{
    int bonus = 0;
    bonus += (IS_DWARF(ch) && (IS_OGRE(vict) || IS_TROLL(vict) || 
                               IS_GIANT(vict) || 
                               (GET_HEIGHT(vict) > 2*GET_HEIGHT(ch))));
    bonus -= (IS_DWARF(ch) && (SECT_TYPE(ch->in_room) == SECT_WATER_SWIM ||
                               SECT_TYPE(ch->in_room) == SECT_WATER_NOSWIM ||
                               ch->in_room->isOpenAir() ||
                               SECT_TYPE(ch->in_room) == SECT_UNDERWATER));
    bonus += (IS_THIEF(ch) && IS_DARK(ch->in_room));
    bonus += (IS_RANGER(ch) && (SECT_TYPE(ch->in_room) == SECT_FOREST ||
                                (SECT_TYPE(ch->in_room) != SECT_CITY && 
                                 SECT_TYPE(ch->in_room) != SECT_INSIDE &&
                                 OUTSIDE(ch))));
    bonus += (IS_TABAXI(ch) && SECT_TYPE(ch->in_room) == SECT_FOREST &&
              OUTSIDE(ch));
    return (bonus);
}

extern const int exp_scale[LVL_GRIMP+2] = {
    0,
    1,
    2500,
    6150,
    11450,
    19150,       /***   5  ***/
    30150,
    45650,
    67600,
    98100,
    140500,       /***  10  ***/
    199500,
    281500,
    391500,
    541000,
    746000,       /***  15  ***/
    1025000,
    1400000,
    1900000,
    2550000,
    3400000,       /***  20  ***/
    4500000,
    5900000,
    7700000,
    10050000,
    12950000,       /***  25  ***/
    16550000,
    21050000,
    26650000,
    33650000,
    42350000,       /***  30  ***/
    52800000,
    65300000,
    79800000,
    96800000,
    116500000,       /***  35  ***/
    140000000,
    167000000,
    198000000,
    233500000,
    274500000,       /***  40  ***/
    320500000,
    371500000,
    426500000,
    486500000,
    551000000,       /***  45  ***/
    622000000,
    699000000,
    783000000,
    869000000,
    1000000000,       /***  50  ***/
    1100000000,
    1200000000,
    1300000000,
    1400000000,
    1500000000,       /***  55  ***/
    1600000000,
    1700000000,
    1800000000,
    1900000000,
    2000000000,      /***  60  ***/
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
    2000000011,     /*** 71 ***/
    2000000012,
    2000000013
};
            

/* Names of char_class/levels and exp required for each level */

extern const struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1] = {
    {{"the Man", "the Woman"},
     {"the Apprentice of Magic", "the Apprentice of Magic"},
     {"the Spell Student", "the Spell Student"},
     {"the Scholar of Magic", "the Scholar of Magic"},
     {"the Delver in Spells", "the Delveress in Spells"},
     {"the Medium of Magic", "the Medium of Magic"},
     {"the Scribe of Magic", "the Scribess of Magic"},
     {"the Seer", "the Seeress"},
     {"the Sage", "the Sage"},
     {"the Illusionist", "the Illusionist"},
     {"the Abjurer", "the Abjuress"},
     {"the Invoker", "the Invoker"},
     {"the Enchanter", "the Enchantress"},
     {"the Conjurer", "the Conjuress"},
     {"the Magician", "the Witch"},
     {"the Mortal Creator", "the Mortal Creator"},
     {"the Savant", "the Savant"},
     {"the Magus", "the Craftess"},
     {"the Wizard", "the Wizard"},
     {"the Warlock", "the War Witch"},
     {"the Sorcerer", "the Sorceress"},
     {"the Necromancer", "the Necromancress"},
     {"the Thaumaturge", "the Thaumaturgess"},
     {"the Student of the Occult", "the Student of the Occult"},
     {"the Disciple of the Uncanny", "the Disciple of the Uncanny"},
     {"the Minor Elemental", "the Minor Elementress"},
     {"the Greater Elemental", "the Greater Elementress"},
     {"the Crafter of Magics", "the Crafter of Magics"},
     {"the Shaman", "the Shaman"},
     {"the Keeper of Talismans", "the Keeper of Talismans"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},
     {"the Believer", "the Believer"},
     {"the Attendant", "the Attendant"},
     {"the Acolyte", "the Acolyte"},
     {"the Novice", "the Novice"},
     {"the Missionary", "the Missionary"},
     {"the Adept", "the Adept"},
     {"the Deacon", "the Deaconess"},
     {"the Vicar", "the Vicaress"},
     {"the Priest", "the Priestess"},
     {"the Minister", "the Lady Minister"},
     {"the Canon", "the Canon"},
     {"the Levite", "the Levitess"},
     {"the Curate", "the Curess"},
     {"the Monk", "the Nunne"},
     {"the Healer", "the Healess"},
     {"the Chaplain", "the Chaplain"},
     {"the Expositor", "the Expositress"},
     {"the Bishop", "the Bishop"},
     {"the Arch Bishop", "the Arch Lady of the Church"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the Patriarch", "the Matriarch"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the PowerPriest", "the PowerPriestess"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},
     {"the Pilferer", "the Pilferess"},
     {"the Footpad", "the Footpad"},
     {"the Filcher", "the Filcheress"},
     {"the Pick-Pocket", "the Pick-Pocket"},
     {"the Sneak", "the Sneak"},
     {"the Pincher", "the Pincheress"},
     {"the Cut-Purse", "the Cut-Purse"},
     {"the Snatcher", "the Snatcheress"},
     {"the Sharper", "the Sharpress"},
     {"the Rogue", "the Rogue"},
     {"the Robber", "the Robber"},
     {"the Magsman", "the Magswoman"},
     {"the Highwayman", "the Highwaywoman"},
     {"the Burglar", "the Burglaress"},
     {"the Thief", "the Thief"},
     {"the Knifer", "the Knifer"},
     {"the Quick-Blade", "the Quick-Blade"},
     {"the Killer", "the Murderess"},
     {"the Brigand", "the Brigand"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Cut-Throat", "the Cut-Throat"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Deadly", "the Deadly"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},
     {"the Swordpupil", "the Swordpupil"},
     {"the Recruit", "the Recruit"},
     {"the Sentinel", "the Sentress"},
     {"the Fighter", "the Fighter"},
     {"the Soldier", "the Soldier"},
     {"the Warrior", "the Warrior"},
     {"the Veteran", "the Veteran"},
     {"the Swordsman", "the Swordswoman"},
     {"the Fencer", "the Fenceress"},
     {"the Combatant", "the Combatess"},
     {"the Hero", "the Heroine"},
     {"the Myrmidon", "the Myrmidon"},
     {"the Swashbuckler", "the Swashbuckleress"},
     {"the Mercenary", "the Mercenaress"},
     {"the Swordmaster", "the Swordmistress"},
     {"the Lieutenant", "the Lieutenant"},
     {"the Champion", "the Lady Champion"},
     {"the Dragoon", "the Lady Dragoon"},
     {"the Cavalier", "the Cavalier"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Knight", "the Lady Knight"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlord", "the Immortal Lady of War"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},
     {"the Uncouth", "the Uncouth"},
     {"the Illiterate", "the Illiterate"},
     {"the Drooling Bastard", "the Drooling Bitch"},
     {"the Grunter", "the Gruntess"},
     {"the Spitter", "the Spittress"},
     {"the Uncultured", "the Uncultured"},
     {"the Crude", "the Crude"},
     {"the Ignorant Savage", "the Ignorant Savage"},
     {"the Primitive", "the Primitive"},
     {"the Groin Kicker", "the Groin Kicker"},
     {"the Impudent", "the Impudent"},
     {"the Vulgar", "the Vulgar"},
     {"the Complete Bastard", "the Complete Bitch"},
     {"the Dragon Slayer", "the Dragon Slayer"},
     {"the Big Fisted", "the Big Fisted"},
     {"the Gallant", "the Gallant"},
     {"the Adventurer", "the Adventuress"},
     {"the Courageous", "the Courageous"},
     {"the Butcher", "the Butcheress"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Destroyer", "the Destroyer"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Head Basher", "the Head Basher"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Barbarian", "the Immortal Lady of War"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                /* psionic */
     {"the Paranormal Youngster", "the Paranormal Youngster"},
     {"the Spoon Bender", "the Spoon Bender"},
     {"the Telephone Psychic", "the Telephone Psychic"},
     {"the Palmist", "the Palmist"},
     {"the Occultist", "the Occultist"},
     {"the Extrasensory", "the Extrasensory"},
     {"the Medium", "the Medium"},
     {"the Supernatural", "the Supernatural"},
     {"the Mind Reader", "the Mind Reader"},
     {"the Metaphysical Man", "the Metaphysical Woman"},
     {"the Supersensible", "the Supersensible"},
     {"the Clairvoyant", "the Clairvoyant"},
     {"the Hypnotist", "the Hypnotist"},
     {"the Sky Pilot", "the Sky Pilot"},
     {"the Human Oracle", "the Human Oracle"},
     {"the Prophet", "the Prophetess"},
     {"the Fortune Teller", "the Fortune Teller"},
     {"the Interpreter", "the Interpretess"},
     {"the Voodoo Priest", "the Voodoo Priestess"},
     {"the Solver of Riddles", "the Solver of Riddles"},
     {"the Brain Scanner", "the Brain Scanner"},
     {"the Diagnostic", "the Diagnostic"},
     {"the Soothsayer", "the Soothsayer"},
     {"the Disciple of Prognostication", "the Disciple of Prognostication"},
     {"the Minor Astrologer", "the Minor Astrologer"},
     {"the Greater Astrologer", "the Greater Astrologer"},
     {"the Weather Prophet", "the Weather Prophet"},
     {"the Holder of Knowledge", "the Holder of Knowledge"},
     {"the Explorer of Minds", "the Explorer of Minds"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Phantasm", "Phantasm"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Psychic", "the Immortal Enchantress"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                             /* physic */
     {"the Undergraduate Student", "the Undergraduate Student"},
     {"the Lab Assistant", "the Lab Assistant"},
     {"the Scholar of Magic", "the Scholar of Magic"},
     {"the Delver in Spells", "the Delveress in Spells"},
     {"the Medium of Magic", "the Medium of Magic"},
     {"the Scribe of Magic", "the Scribess of Magic"},
     {"the Seer", "the Seeress"},
     {"the Sage", "the Sage"},
     {"the Illusionist", "the Illusionist"},
     {"the Abjurer", "the Abjuress"},
     {"the Invoker", "the Invoker"},
     {"the Enchanter", "the Enchantress"},
     {"the Conjurer", "the Conjuress"},
     {"the Magician", "the Witch"},
     {"the Creator", "the Creator"},
     {"the Savant", "the Savant"},
     {"the Magus", "the Craftess"},
     {"the Wizard", "the Wizard"},
     {"the Warlock", "the War Witch"},
     {"the Sorcerer", "the Sorceress"},
     {"the Necromancer", "the Necromancress"},
     {"the Thaumaturge", "the Thaumaturgess"},
     {"the Student of the Occult", "the Student of the Occult"},
     {"the Disciple of the Uncanny", "the Disciple of the Uncanny"},
     {"the Minor Elemental", "the Minor Elementress"},
     {"the Greater Elemental", "the Greater Elementress"},
     {"the Crafter of Magics", "the Crafter of Magics"},
     {"the Shaman", "Shaman"},
     {"the Keeper of Talismans", "the Keeper of Talismans"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Analyzer", "the Immortal Analyzer"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man","the Woman"},                    /* cyborg */
     {"the Engineered Youth",""},
     {"the Engineered Youth",""},
     {"the Preliminary Prototype",""},
     {"the Altered",""},
     {"the Voltage Progeny",""},
     {"the Frobnicated",""},
     {"the Direct Current",""},
     {"the Surgically Enhanced",""},
     {"the Calculating",""},
     {"the Connected",""},                   /* 10 */
     {"the Wired",""},
     {"the Prototype",""},
     {"the Energy Recepticle",""},
     {"the Accessorized",""},
     {"the Electrostatic",""},
     {"the Net Master",""},
     {"the Cowboy",""},
     {"the Downloader",""},
     {"the Virus Maker",""},
     {"the Machinehead",""},                 /* 20 */
     {"the Wired",""},
     {"the Reextern constructed",""},
     {"the Advanced Prototype",""},
     {"the Advanced Prototype",""},
     {"the Implanted",""},
     {"the Implanted",""},
     {"the Bionically Enhanced",""},
     {"the Bionically Enhanced",""},
     {"the Cybernetic",""},
     {"the Cybernetic",""},                  /* 30 */
     {"the Fully Functioning Model",""},
     {"the Advanced Processing Unit",""},
     {"the Superconductive Unit",""},
     {"the Advanced Model",""},
     {"the Multitasking",""},
     {"the Parallel Processor",""},
     {"the Sentient Computer",""},
     {"the Portable Cray",""},
     {"the Machine",""},
     {"the Cyborg",""},                         /* 40 */
     {"the Cybernetic Avatar",""},                         /* 41 */
     {"the Cybernetic Avatar",""},
     {"the Cybernetic Avatar",""},
     {"the Cybernetic Avatar",""},
     {"the Cybernetic Avatar",""},
     {"the Cybernetic Entity",""},
     {"the T-800",""},
     {"the T-1000",""},
     {"the Skyhook Prototype",""},
     {"the Ambassador", "the Ambassador"},        /* 50 */
     {"the Full Conversion", "the Full Conversion"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternally Astral", "the Eternally Astral"},
     {"the Demiwizard", "the Demiwitch"},
     {"the Electroelemental", "the Electroelemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser Being", "the Lesser Being"},
     {"the Deity", "the Deity"},
     {"the Timeless Entity", "the Timeless Entity"},
     {"the Artificial Intellegence", "the Artificial Intellegence"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                /* knight */
     {"the Noble Neophyte", "the Noble Neophyte"},
     {"the Page-in-Training", "the Page-in-Training"},
     {"the Armor Polisher", "the Armor Polisher"},
     {"the Devout Defender", "the Devout Defender"},
     {"the Squire", "the Squire"},
     {"the Lancer", "the Lancer"},
     {"the Apprentice", "the Apprentice"},
     {"the Bringer of Horses", "the Bringer of Horses"},
     {"the Sword Sharpener", "the Sword Sharpener"},
     {"the Stable Cleaner", "the Stable Cleaner"},
     {"the Lackey", "the Lackey"},
     {"the Flunky", "the Flunky"},
     {"the Weapon Caddy", "the Weapon Caddy"},
     {"the Underling", "the Underling"},
     {"the Squire", "the Squiress"},
     {"the Horse Washer", "the Horse Washer"},
     {"the Gentry", "the Gentry"},
     {"the Chivalrous", "the Chivalrous"},
     {"the Gallant", "the Gallant"},
     {"the Noble", "the Nobless"},
     {"the Valiant", "the Valiant"},
     {"the Pure of Heart", "the Pure of Heart"},
     {"the Protector of the Innocent", "the Protector of the Innocent"},
     {"the Intrepid", "the Intrepid"},
     {"the Honorable Avenger", "the Honorable Avenger"},
     {"the Divine Leader", "the Divine Leader"},
     {"the Aristocrat", "the Aristocrat"},
     {"the Holy Warrior", "Holy Warrior"},
     {"the Crusader", "the Crusader"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Knight", "the Knight"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Knight", "the Immortal Knight"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                           /* ranger */
     {"the Sapling", "the Sapling"},
     {"the Cub Scout", "the Brownie"},
     {"the Boy Scout", "the Girl Scout"},
     {"the Scout", "the Scout"},
     {"the Spotter", "the Spotter"},
     {"the Tracker", "the Tracker"},
     {"the Preserver", "the Preserver"},
     {"the Defender of Nature", "the Defender of Nature"},
     {"the Forest Guardian", "the Forest Guardian"},
     {"the Forest Ranger", "the Forest Ranger"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Sentinel", "the Sentinel"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Frontiersman", "Frontierswoman"},
     {"the Frontiersman", "the Frontierswoman"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Man of the Forest", "Woman of the Forest"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Ranger", "the Immortal Ranger"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                 /* hood */
     {"the Worthless Scumbag", "the Worthless Scumbag"},
     {"the Litterbug", "the Litterbug"},
     {"the Troublemaker", "the Troublemaker"},
     {"the Social Deviant", "the Social Deviant"},
     {"the Juvenile Delinquet", "the Juvenile Delinquet"},
     {"the Juvenile Delinquet", "the Juvenile Delinquet"},
     {"the Shoplifter", "the Shoplifter"},
     {"the Kleptomaniac", "the Kleptomaniac"},
     {"the Gutter Punk", "the Gutter Punk"},
     {"the Gutter Punk", "the Gutter Punk"},        // 10
     {"the Street Punk", "the Street Punk"},
     {"the Street Punk", "the Street Punk"},        
     {"the Pothead", "the Pothead"},
     {"the Bong Maker", "the Bong Maker"},
     {"the Crackhead", "the Crackhead"},
     {"the Crackhead", "the Crackhead"},
     {"the Rehabilitated Junkie", "the Rehabilitated Junkie"},
     {"the Pimp Daddy", "the Pimp Momma"},
     {"the Pimp Daddy", "the Pimp Momma"},
     {"the Drug Pusher", "the Drug Pusher"},        // 20
     {"the Drug Pusher", "the Drug Pusher"},
     {"the Crack Dealer", "the Crack Dealer"},
     {"the Heroin Dealer", "the Heroin Dealer"},
     {"the Felon", "the Felon"},
     {"the Felon", "the Felon"},
     {"the Repeat Offender", "the Repeat Offender"},
     {"the Repeat Offender", "the Repeat Offender"},
     {"the Arsonist", "the Arsonist"},
     {"the Gang Initiate", "the Gang Initiate"},
     {"the Gangsta", "the Gangsta"},                //30
     {"the Gangsta", "the Gangsta"},
     {"the Gangsta", "the Gangsta"},
     {"the Gang Leader", "the Gang Leader"},
     {"the Gang Leader", "the Gang Leader"},
     {"the Gang Leader", "the Gang Leader"},
     {"the Gang Leader", "the Gang Leader"},
     {"the Crime Boss", "the Crime Boss"},
     {"the Crime Boss", "the Crime Boss"},
     {"the Crime Boss", "the Crime Boss"},        
     {"the Crime Boss", "the Crime Boss"},                        //40
     {"the Criminal Avatar", "the Criminal Avatar"},
     {"the Criminal Avatar", "the Criminal Avatar"},
     {"the Criminal Avatar", "the Criminal Avatar"},
     {"the Criminal Avatar", "the Criminal Avatar"},
     {"the Lord of the Underworld", "the Lord of the Underworld"}, 
     {"the Lord of the Underworld", "the Lord of the Underworld"},
     {"the Lord of the Underworld", "the Lord of the Underworld"}, 
     {"the Lord of the Underworld", "the Lord of the Underworld"},
     {"the Lord of the Underworld", "the Lord of the Underworld"},
     {"the Ambassador", "the Ambassador"},                // 50
     {"the Immortal Hood", "the Immortal Hood"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                              /* monk */
     {"the Utter Novice", "the Utter Novice"},
     {"the Inexperienced Trainee", "the Inexperienced Trainee"},
     {"the First Degree White Belt", "the First Degree White Belt"},
     {"the Praying Mantis", "the Praying Mantis"},
     {"the Praying Mantis", "the Praying Mantis"},
     {"the Yellow Belt", "the Yellow Belt"},
     {"the Flying Crane", "the Flying Crane"},
     {"the Flying Crane", "the Flying Crane"},
     {"the Flying Crane", "the Flying Crane"},
     {"the Flying Crane", "the Flying Crane"},
     {"the Green Belt", "the Green Belt"},
     {"the Wiley Mongoose", "the Wiley Mongoose"},
     {"the Wiley Mongoose", "the Wiley Mongoose"},
     {"the Wiley Mongoose", "the Wiley Mongoose"},
     {"the Wiley Mongoose", "the Wiley Mongoose"},
     {"the Blue Belt", "the Blue Belt"},
     {"the Razorback Boar", "the Razorback Boar"},
     {"the Razorback Boar", "the Razorback Boar"},
     {"the Razorback Boar", "the Razorback Boar"},
     {"the Razorback Boar", "the Razorback Boar"},
     {"the Purple Belt", "the Purple Belt"},
     {"the Deadly Asp", "the Deadly Asp"},
     {"the Deadly Asp", "the Deadly Asp"},
     {"the Deadly Asp", "the Deadly Asp"},
     {"the Deadly Asp", "the Deadly Asp"},
     {"the Orange Belt", "the Orange Belt"},
     {"the Cunning Wolf", "the Cunning Wolf"},
     {"the Cunning Wolf", "the Cunning Wolf"},
     {"the Cunning Wolf", "the Cunning Wolf"},
     {"the Cunning Wolf", "the Cunning Wolf"},
     {"the Red Belt", "the Red Belt"},
     {"the Raging Kodiak", "the Raging Kodiak"},
     {"the Raging Kodiak", "the Raging Kodiak"},
     {"the Raging Kodiak", "the Raging Kodiak"},
     {"the Raging Kodiak", "the Raging Kodiak"},
     {"the Maroon Belt", "the Maroon Belt"},
     {"the Silent Tiger", "the Silent Tiger"},
     {"the Silent Tiger", "the Silent Tiger"},
     {"the Silent Tiger", "the Silent Tiger"},
     {"the Silent Tiger", "the Silent Tiger"},
     {"the Brown Belt", "the Brown Belt"},
     {"the Dragon", "the Dragon"},
     {"the Dragon", "the Dragon"},
     {"the Dragon", "the Dragon"},
     {"the First Degree Black Belt", "the First Degree Black Belt"},
     {"the Grandmaster", "the Grandmaster"},
     {"the Grandmaster", "the Grandmaster"},
     {"the Grandmaster", "the Grandmaster"},
     {"the Ultimate Warrior", "the Ultimate Warrior"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Monk", "the Immortal Monk"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                   /* vampire */
     {"the Apprentice of Magic", "the Apprentice of Magic"},
     {"the Spell Student", "the Spell Student"},
     {"the Scholar of Magic", "the Scholar of Magic"},
     {"the Delver in Spells", "the Delveress in Spells"},
     {"the Medium of Magic", "the Medium of Magic"},
     {"the Scribe of Magic", "the Scribess of Magic"},
     {"the Seer", "the Seeress"},
     {"the Sage", "the Sage"},
     {"the Illusionist", "the Illusionist"},
     {"the Abjurer", "the Abjuress"},
     {"the Invoker", "the Invoker"},
     {"the Enchanter", "the Enchantress"},
     {"the Conjurer", "the Conjuress"},
     {"the Magician", "the Witch"},
     {"the Mortal Creator", "the Mortal Creator"},
     {"the Savant", "the Savant"},
     {"the Magus", "the Craftess"},
     {"the Wizard", "the Wizard"},
     {"the Warlock", "the War Witch"},
     {"the Sorcerer", "the Sorceress"},
     {"the Necromancer", "the Necromancress"},
     {"the Thaumaturge", "the Thaumaturgess"},
     {"the Student of the Occult", "the Student of the Occult"},
     {"the Disciple of the Uncanny", "the Disciple of the Uncanny"},
     {"the Minor Elemental", "the Minor Elementress"},
     {"the Greater Elemental", "the Greater Elementress"},
     {"the Crafter of Magics", "the Crafter of Magics"},
     {"the Shaman", "Shaman"},
     {"the Keeper of Talismans", "the Keeper of Talismans"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},        
     {"the Mortal Avatar", "the Mortal Avatar"},        
     {"the Mortal Avatar", "the Mortal Avatar"},        
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},        
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Mortal Avatar", "the Mortal Avatar"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                   /* mercenary*/
     {"the Novice", "the Novice"},
     {"the Recruit", "the Recruit"},
     {"the Reckless", "the Reckless"},
     {"the Watcher", "the Watcher"},
     {"the Eluder", "the Eluder"},
     {"the Evader", "the Evader"},
     {"the Stalker", "the Stalker"},
     {"the Combatant", "the Combatant"},
     {"the Rookie", "the Rookie"},
     {"the Commando", "the Commando"},
     {"the Brawler", "the Brawler"},
     {"the Airborn", "the Airborn"},
     {"the Master", "the Master"},
     {"the Fearless", "the Fearless"},
     {"the Winner", "the Winner"},
     {"the Lieutenant", "the Lieutenant"},
     {"the Finder", "the Finder"},
     {"the Tracker", "the Tracker"},
     {"the Bombsman", "the Bombsman"},
     {"the Gunsman", "the Gunsman"},
     {"the Planter of Bombs", "the Planter of Bombs"},
     {"the Bullet Maker", "the Bullet Maker"},
     {"the Gun Wielder", "the Gun Wielder"},
     {"the Captain", "the Captain"},
     {"the Explosives Expert", "the Explosives Expert"},
     {"the Combat Specialist", "the Combat Specialist"},
     {"the Knife Thrower", "the Knife Thrower"},
     {"the Special Forces Unit", "the Special Forces Unit"},
     {"the Trained", "the Trained"},
     {"the Rifleman", "the Rifleman"},
     {"the Killer", "the Killer"},
     {"the Merciless", "the Merciless"},
     {"the Paid", "the Paid"},
     {"the Hunter", "the Hunter"},
     {"the Relentless", "the Relentless"},
     {"The Major", "The Major"},
     {"the Seeker", "the Seeker"},
     {"the Spy Eye", "the Spy Eye"},
     {"the Survivor", "the Survivor"},
     {"the Sharp Shooter", "the Sharp Shooter"},
     {"The Colonel", "The Colonel"},
     {"the Master of Elusion", "the Master of Elusion"},
     {"the Master Shooter", "the Master Shooter"},
     {"the Master Rifleman", "the Master Rifleman"},
     {"the Veteran", "the Veteran"},
     {"the Cold-hearted", "the Cold-hearted"},
     {"the Deadly", "the Deadly"},
     {"the Ruthless", "the Ruthless"},
     {"the Sniper", "the Sniper"},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                   /* spare1 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                   /* spare2 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    },
    {{"the Man", "the Woman"},                                   /*spare3*/
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},  /* 5 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""}, /* 10 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""}, /* 20 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""}, /* 30 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""}, /* 40 */
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"", ""},
     {"the Ambassador", "the Ambassador"},
     {"the Immortal Warlock", "the Immortal Witch"},
     {"the Builder", "the Builder"},
     {"the Luminary", "the Luminary"},
     {"the Ethereal", "the Ethereal"},
     {"the Eternal One", "the Eternal One"},
     {"the Demigod", "the Demigoddess"},
     {"the Elemental", "the Elemental"},
     {"the Spirit", "the Spirit"},
     {"the Being", "the Being"},
     {"the Power", "the Power"},
     {"the Force", "the Force"},
     {"the Energy", "the Energy"},
     {"the Lesser God", "the Lesser Goddess"},
     {"the Deity", "the Deity"},
     {"the God of Time", "the Goddess of Time"},
     {"the Greater God", "the Greater Goddess"},
     {"the Creator", "the Creator"},
     {"the Entity", "the Entity"},
     {"the Implementor", "the Implementor"},
     {"the Angel of Light", "the Angel of Light"},
     {"", ""},
     {"da GRIMP", "da GRIMP"}
    }
};

extern const char *evil_knight_titles[LVL_GRIMP+1] = {
    "the Evil Knight",
    "the Unrightous Offspring", /* 1 */
    "the Defiant",
    "the Unlawful",
    "the Defiler",
    "the Lost Soul",             /* 5 */
    "the Damned",
    "the Devout Destroyer",
    "the Bringer of Pain",
    "the Squire of Evil",
    "the Sacrificial Lamb",       /* 10 */
    "the Wicked",
    "the Follower of Ares",
    "the Evil Understudy",
    "the Ambitious One",
    "the Defender of the Guilty", /* 15 */
    "the Dark Warrior",
    "the Remorseless",
    "of Pain and Suffering",
    "the Slayer of Justice",
    "the Servant of Ares",        /* 20 */
    "the Consort of Chaos",
    "the Unholy Crusader",
    "the Unholy Avenger",
    "the Harbringer of Death",
    "the Fiend of War",           /* 25 */
    "the Utterly Damned",
    "the Evil Knight",
    "the Scourge of Goodness",
    "the The Hell Spawn",
    "the Reaper of Souls",        /* 30 */
    "the Pillager of Innocence",
    "the Living Legend",
    "the Destroyer of Life",
    "the Servant of Evil",
    "the Servant of Evil",       /* 35 */
    "the Servant of Evil",
    "the Servant of Evil",
    "the Servant of Evil",
    "the Servant of Evil",
    "the Death Knight",         /* 40 */
    "the Death Knight",
    "the Death Knight",
    "the Death Knight",
    "the Death Knight",
    "the Death Knight",         /* 45 */
    "the Death Knight",
    "the Death Knight",
    "the AntiChrist",
    "the Lord of Evil",
    "the Ambassador",
    "the Immortal Death Knight", /* 51 */
    "the Builder",
    "the Luminary",
    "the Ethereal",
    "the Death Knight of Eternity",
    "the Demigod of Death",
    "the Elemental Ruler",
    "the Spirit",
    "the Being",
    "the Power",
    "the Force",
    "the Energy",
    "the Lesser Power",
    "the Evil Deity",
    "the Marauder of Time", 
    "the Greater Evil",
    "the Creator of Evil",
    "the Evil Entity",
    "the Implementor",
    "the Angel of Light",
    "",
    "da GRIMP"
};


#undef __char_class_c__
