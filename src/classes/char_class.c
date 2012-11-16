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

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>
#include <libxml/parser.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "screen.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "obj_data.h"
#include "actions.h"
#include "char_class.h"
#include "libpq-fe.h"
#include "db.h"

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

/* #define LEARNED_LEVEL        0  % known which is considered "learned" */
/* #define MAX_PER_PRAC                1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC                2  min percent gain in skill per practice */
/* #define PRAC_TYPE                3  should it say 'spell' or 'skill'?        */

const int prac_params[4][NUM_CLASSES] = {
    /* MG  CL  TH  WR  BR  PS  PH  CY  KN  RN  BD  MN  VP  MR  S1  S2  S3 */
    {75, 75, 70, 70, 65, 75, 75, 80, 75, 75, 80, 75, 75, 70, 70, 70, 70},
    {25, 20, 20, 20, 20, 25, 20, 30, 20, 25, 30, 20, 15, 25, 25, 25, 25},
    {15, 15, 10, 15, 10, 15, 15, 15, 15, 15, 15, 10, 10, 10, 10, 10, 10},
    {SPL, SPL, SKL, SKL, SKL, TRG, ALT, PRG, SPL, SPL, SNG, ZEN, SPL, SKL, SKL,
            SKL, SKL}

};

// 0 - class/race combination not allowed
// 1 - class/race combination allowed only for secondary class
// 2 - class/race combination allowed for primary class
const char race_restr[NUM_PC_RACES][NUM_CLASSES + 1] = {
    //                 MG CL TH WR BR PS PH CY KN RN BD MN VP MR S1 S2 S3
    {RACE_HUMAN, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0, 0},
    {RACE_ELF, 2, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0, 0},
    {RACE_DWARF, 0, 2, 2, 0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0},
    {RACE_HALF_ORC, 0, 0, 2, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0},
    {RACE_HALFLING, 2, 2, 2, 0, 2, 1, 1, 1, 2, 2, 2, 2, 0, 1, 0, 0, 0},
    {RACE_TABAXI, 2, 2, 2, 0, 2, 2, 2, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0},
    {RACE_DROW, 2, 2, 2, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 1, 0, 0, 0},
    {RACE_MINOTAUR, 2, 2, 0, 0, 2, 0, 1, 1, 0, 2, 0, 0, 0, 1, 0, 0, 0},
    {RACE_ORC, 0, 0, 1, 0, 2, 0, 1, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0},
};

/* THAC0 for char_classes and levels.  (To Hit Armor Class 0) */

const float thaco_factor[NUM_CLASSES] = {
    0.15,                       /* mage    */
    0.20,                       /* cleric  */
    0.25,                       /* thief   */
    0.30,                       /* warrior */
    0.40,                       /* barb    */
    0.20,                       /* psionic */
    0.15,                       /* physic  */
    0.30,                       /* cyborg  */
    0.35,                       /* knight  */
    0.35,                       /* ranger  */
    0.30,                       /* bard    */
    0.40,                       /* monk    */
    0.40,                       /* vampire */
    0.35,                       /* merc    */
    0.30,                       /* spare1  */
    0.30,                       /* spare2  */
    0.30                        /* spare3  */
};

void
gain_skill_prof(struct creature *ch, int skl)
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
        if ((GET_SKILL(ch, skl) - GET_LEVEL(ch)) <= 66
            && !number(0, GET_LEVEL(ch)))
            GET_SKILL(ch, skl) += 1;
}

/* Names first */
const char *char_class_abbrevs[] = {
    "Mage",                     /* 0 */
    "Cler",
    "Thie",
    "Warr",
    "Barb",
    "Psio",                     /* 5 */
    "Phyz",
    "Borg",
    "Knig",
    "Rang",
    "Bard",                     /* 10 */
    "Monk",
    "Vamp",
    "Merc",
    "Spa1",
    "Spa2",                     /* 15 */
    "Spa3",
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",  /*25 */
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",   /*35 */
    "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR", "ERR",   /*45 */
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
    "Red",                      /* 95 */
    "Slvr",
    "Shad",
    "Deep",
    "Turt",
    "ILL",                      /* 100 */
    "Lest",
    "Lssr",
    "Grtr",
    "Duke",
    "Arch",                     /*105 */
    "ILL", "ILL", "ILL", "ILL", "ILL",  /*110 */
    "Hill", "Ston", "Frst", "Fire", "Clod", /*115 */
    "Strm",
    "ILL", "ILL", "ILL",
    "Red",                      /* 120 */
    "Blue",
    "Gren",
    "Grey",
    "Deth",
    "Lord",                     /* 125 */
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
    "Prnc",                     /* 140 */
    "ILL",
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
    "Astl",                     /* Devas *//* 150 */
    "Mond",
    "Movn",
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",   // 162
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",   // 172
    "ILL", "ILL", "ILL", "ILL", "ILL",
    "Gdlg",                     // 178  Godling and Demigods extra planes
    "Diet",
    "ILL", "ILL", "ILL",        // 182
    "\n"
};

const char *class_names[] = {
    "Mage",
    "Cleric",
    "Thief",
    "Warrior",
    "Barbarian",
    "Psionic",                  /* 5 */
    "Physic",
    "Cyborg",
    "Knight",
    "Ranger",
    "Bard",                     /* 10 */
    "Monk",
    "Vampire",
    "Mercenary",
    "Spare1",
    "Spare2",                   /* 15 */
    "Spare3",
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",  /* 25 */
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",   /* 35 */
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",   /* 45 */
    "ILL", "ILL", "ILL", "ILL",
    "Mobile",                   /* 50 */
    "Bird",
    "Predator",
    "Snake",
    "Horse",
    "Small",
    "Medium",
    "Large",
    "Scientist",
    "ILL", "Skeleton",          /* 60 */
    "Ghoul", "Shadow", "Wight", "Wraith", "Mummy",  /* 65 */
    "Spectre", "Vampire", "Ghost", "Lich", "Zombie",    /* 70 */
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",   /* 80 */
    "Earth", "Fire", "Water", "Air", "Lighting",    /* 85 */
    "ILL", "ILL", "ILL", "ILL", "ILL",  /* 90 */
    "Green",
    "White",
    "Black",
    "Blue",
    "Red",                      /* 95 */
    "Silver",
    "Shadow_D",
    "Deep",
    "Turtle", "ILL",            /* 100 */
    "Least",
    "Lesser",
    "Greater",
    "Duke",
    "Arch",                     /*105 */
    "ILL", "ILL", "ILL", "ILL", "ILL",  /*110 */
    "Hill", "Stone", "Frost", "Fire", "Cloud",  /*115 */
    "Storm",
    "ILL", "ILL", "ILL",
    "Red",                      /* Slaad *//* 120 */
    "Blue",
    "Green",
    "Grey",
    "Death",
    "Lord",                     /* 125 */
    "ILL", "ILL", "ILL", "ILL",
    "Type I",                   /* Demons *//* 130 */
    "Type II",
    "Type III",
    "Type IV",
    "Type V",
    "Type VI",
    "Semi",
    "Minor",
    "Major",
    "Lord",
    "Prince",                   /* 140 */
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
    "Astral",                   /* Devas *//* 150 */
    "Monadic",
    "Movanic",
    "ILL", "ILL", "ILL",
    "ILL", "ILL", "ILL", "ILL",
    "Fire",                     /* Mephits 160 */
    "Lava",
    "Smoke",
    "Steam",
    "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
    "Arcana",                   // daemons 170
    "Charona",
    "Dergho",
    "Hydro",
    "Pisco",
    "Ultro",
    "Yagno",
    "Pyro",                     // 177
    "Godling",                  // 178 - For Extra Planes
    "Deity",
    "\n"
};

// Returns a tmpstr allocated char* containing an appropriate ANSI
// color code for the given target struct creature (tch) with the given
// recipient struct creature(ch)'s color settings in mind.
const char *
get_char_class_color_code(struct creature *ch, struct creature *tch,
    int char_class)
{
    switch (char_class) {
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
// struct creature (tch) suitable for use with send_to_desc.
const char *
get_char_class_color(struct creature *tch, int char_class)
{
    switch (char_class) {
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

int
parse_race(char *arg)
{
    struct race *race = race_by_name(arg, false);
    if (race)
        return race->idnum;

    return -1;
}

int
parse_char_class(char *arg)
{
    int j;

    for (j = 0; j < TOP_CLASS; j++)
        if (is_abbrev(arg, class_names[j]))
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
    }
    return 0;
}

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each char_class then decides
 * which priority will be given for the best to worst stats.
 */
void
roll_real_abils(struct creature *ch)
{
    int i, j, k;
    uint8_t table[6];
    // Zero out table
    for (i = 0; i < 6; i++) {
        table[i] = 12;
    }

    // Increment and decrement randomly
    for (i = 0; i < 24; i++) {
        do {
            j = number(0, 5);
        } while (table[j] == 18);
        table[j] += 1;
        do {
            j = number(0, 5);
        } while (table[j] == 3);
        table[j] -= 1;
    }

    // Sort the table
    for (j = 0; j < 6; j++) {
        for (k = j; k < 6; k++) {
            if (table[k] > table[j]) {
                table[j] ^= table[k];
                table[k] ^= table[j];
                table[j] ^= table[k];
            }
        }
    }

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
        break;
    case CLASS_BARB:
        ch->real_abils.str = table[0];
        ch->real_abils.con = table[1];
        ch->real_abils.dex = table[2];
        ch->real_abils.wis = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
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
        break;
    case CLASS_RANGER:
        ch->real_abils.dex = table[0];
        ch->real_abils.wis = table[1];
        ch->real_abils.con = table[2];
        ch->real_abils.str = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
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
        break;
    case CLASS_BARD:
        ch->real_abils.cha = table[0];
        ch->real_abils.dex = table[1];
        ch->real_abils.str = table[2];
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

    struct race *race = race_by_idnum(GET_RACE(ch));

    ch->real_abils.str += race->str_mod;
    ch->real_abils.intel += race->int_mod;
    ch->real_abils.wis += race->wis_mod;
    ch->real_abils.dex += race->dex_mod;
    ch->real_abils.con += race->con_mod;
    ch->real_abils.cha += race->cha_mod;

    if (GET_RACE(ch) == RACE_HUMAN) {
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
            ch->real_abils.str += 1;
            ch->real_abils.dex += 1;
            break;
        default:
            break;
        }
    }

    ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills
   If mode == 0, then act as though the character was entering the
   game for the first time.  Otherwise, act as though the character
   is being set to that level.
 */
void
do_start(struct creature *ch, int mode)
{
    void advance_level(struct creature *ch, int8_t keep_internal);
    int8_t new_player = 0;
    int i;
    struct obj_data *implant_save[NUM_WEARS];
    struct obj_data *tattoo_save[NUM_WEARS];

    // remove implant affects
    for (i = 0; i < NUM_WEARS; i++) {
        if (GET_IMPLANT(ch, i))
            implant_save[i] = raw_unequip_char(ch, i, EQUIP_IMPLANT);
        else
            implant_save[i] = NULL;
        if (GET_TATTOO(ch, i))
            tattoo_save[i] = raw_unequip_char(ch, i, EQUIP_TATTOO);
        else
            tattoo_save[i] = NULL;
    }

    if (GET_EXP(ch) == 0 && !IS_REMORT(ch) && !IS_VAMPIRE(ch))
        new_player = true;

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
    case CLASS_MERCENARY:
        SET_SKILL(ch, SKILL_PUNCH, 20);
    case CLASS_BARD:
        SET_SKILL(ch, SKILL_PUNCH, 25);
        SET_SKILL(ch, SKILL_ARCHERY, 25);
        break;

    }

    if (new_player) {
        if (PAST_CLASS(GET_CLASS(ch))) {
            deposit_past_bank(ch->desc->account,
                8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch));
            ch->points.gold =
                8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
        } else if (FUTURE_CLASS(GET_CLASS(ch))) {
            deposit_future_bank(ch->desc->account,
                8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch));
            ch->points.cash =
                8192 + number(256, 2048) + GET_INT(ch) + GET_WIS(ch);
        }

        // New players start with a hospital gown and items most dear to them
        struct obj_data *gown = read_object(33800);
        if (gown != NULL) {
            equip_char(ch, gown, WEAR_ABOUT, EQUIP_WORN);
        }

        // Good clerics start with a holy symbol on neck
        if ((GET_CLASS(ch) == CLASS_CLERIC) && IS_GOOD(ch)) {
            struct obj_data *talisman = read_object(1280);
            if (talisman != NULL) {
                equip_char(ch, talisman, WEAR_NECK_1, EQUIP_WORN);
            }
        }

        // Evil clerics start with a holy symbol on hold
        if ((GET_CLASS(ch) == CLASS_CLERIC) && IS_EVIL(ch)) {
            struct obj_data *symbol = read_object(1260);
            if (symbol != NULL) {
                equip_char(ch, symbol, WEAR_HOLD, EQUIP_WORN);
            }
        }

        // Good knights start with a holy symbol on finger
        if ((GET_CLASS(ch) == CLASS_KNIGHT) && IS_GOOD(ch)) {
            struct obj_data *ring = read_object(1287);
            if (ring != NULL) {
                equip_char(ch, ring, WEAR_FINGER_L, EQUIP_WORN);
            }
        }

        // Evil knights start with a holy symbol on neck
        if ((GET_CLASS(ch) == CLASS_KNIGHT) && IS_EVIL(ch)) {
            struct obj_data *pendant = read_object(1270);
            if (pendant != NULL) {
                equip_char(ch, pendant, WEAR_NECK_1, EQUIP_WORN);
            }
        }

        // Bards start with a percussion instrument held, and stringed in inventory
        if ((GET_CLASS(ch) == CLASS_BARD)) {
            struct obj_data *lute = read_object(3218);
            if (lute != NULL) {
                obj_to_char(lute, ch);
            }
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
        ch->player.time.logon = time(NULL);
    }

    for (i = 0; i < NUM_WEARS; i++) {
        if (implant_save[i])
            equip_char(ch, implant_save[i], i, EQUIP_IMPLANT);
        if (tattoo_save[i])
            equip_char(ch, tattoo_save[i], i, EQUIP_TATTOO);
    }
}

/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each char_class every time they gain a level.
 */
void
advance_level(struct creature *ch, int8_t keep_internal)
{
    int add_hp[2], add_mana[2], add_move[2], i, char_class;
    char *msg;

    add_hp[0] = add_hp[1] = constitution_hitpoint_bonus(GET_CON(ch));
    add_mana[0] = add_mana[1] = wisdom_mana_bonus(GET_WIS(ch));
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
            add_hp[i] = add_hp[i] / 3;
            add_hp[i] += number(5, 10);
            add_mana[i] += number(1, 4) + (GET_LEVEL(ch) / 10);
            add_move[i] += number(10, 18);
            break;
        case CLASS_THIEF:
            add_hp[i] /= 3;
            add_hp[i] += number(4, 10);
            add_mana[i] = add_mana[i] * 3 / 10;
            add_move[i] += number(2, 6);
            break;
        case CLASS_MERCENARY:
            add_hp[i] += number(6, 14);
            add_mana[i] = add_mana[i] * 5 / 10;
            add_mana[i] += number(1, 5) + GET_LEVEL(ch) / 10;
            add_move[i] += number(3, 9);
            break;
        case CLASS_WARRIOR:
            add_hp[i] += number(10, 15);
            add_mana[i] = add_mana[i] * 4 / 10;
            add_mana[i] += number(1, 5);
            add_move[i] += number(5, 10);
            break;
        case CLASS_BARB:
            add_hp[i] += number(13, 18);
            add_mana[i] = add_mana[i] * 3 / 10;
            add_mana[i] += number(0, 3);
            add_move[i] += number(5, 10);
            break;
        case CLASS_KNIGHT:
            add_hp[i] += number(7, 13);
            add_mana[i] = add_mana[i] * 7 / 10;
            add_mana[i] += number(1, 4) + (GET_LEVEL(ch) / 15);
            add_move[i] += number(3, 8);
            break;
        case CLASS_RANGER:
            add_hp[i] += number(4, 11);
            add_mana[i] = add_mana[i] * 6 / 10;
            add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 8);
            add_move[i] += number(6, 14);
            break;
        case CLASS_PSIONIC:
            add_hp[i] /= 3;
            add_hp[i] += number(3, 8);
            add_mana[i] = add_mana[i] * 6 / 10;
            add_mana[i] += number(1, 7) + (GET_LEVEL(ch) / 5);
            add_move[i] += number(2, 6);
            break;
        case CLASS_PHYSIC:
            add_hp[i] /= 4;
            add_hp[i] += number(4, 9);
            add_mana[i] = add_mana[i] * 6 / 10;
            add_mana[i] += number(1, 6) + (GET_LEVEL(ch) / 3);
            add_move[i] += number(2, 10);
            break;
        case CLASS_CYBORG:
            add_hp[i] /= 2;
            add_hp[i] += number(6, 14);
            add_mana[i] = add_mana[i] * 3 / 10;
            add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 15);
            add_move[i] += number(5, 8);
            break;
        case CLASS_MONK:
            add_hp[i] /= 3;
            add_hp[i] += number(6, 12);
            add_mana[i] = add_mana[i] * 3 / 10;
            add_mana[i] += number(1, 2) + (GET_LEVEL(ch) / 22);
            add_move[i] += number(6, 9);
            break;
        default:
            add_hp[i] /= 2;
            add_hp[i] += number(5, 16);
            add_mana[i] = add_mana[i] * 5 / 10;
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
    if (PLR_FLAGGED(ch, PLR_HARDCORE))
        GET_LIFE_POINTS(ch) += 1;

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
            GET_COND(ch, i) = -1;
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

    crashsave(ch);
    int rid = -1;
    if (ch->in_room != NULL)
        rid = ch->in_room->number;
    msg = tmp_sprintf("%s advanced to level %d in room %d%s",
        GET_NAME(ch), GET_LEVEL(ch), rid, is_tester(ch) ? " <TESTER>" : "");
    if (keep_internal)
        slog("%s", msg);
    else
        mudlog(GET_INVIS_LVL(ch), BRF, true, "%s", msg);
}

/*
 * invalid_char_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular char_class, based on the ITEM_ANTI_{char_class} bitvectors.
 */

int
invalid_char_class(struct creature *ch, struct obj_data *obj)
{
    // Protected object
    if (IS_PC(ch) &&
        obj->shared->owner_id != 0 && obj->shared->owner_id != GET_IDNUM(ch)) {
        return true;
    }
    // Unapproved object
    if (!OBJ_APPROVED(obj) && !is_tester(ch) && GET_LEVEL(ch) < LVL_IMMORT)
        return true;

    // Anti class restrictions
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
        (IS_OBJ_STAT(obj, ITEM_ANTI_BARD) && IS_BARD(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_MONK) && IS_MONK(ch)) ||
        (IS_OBJ_STAT2(obj, ITEM2_ANTI_MERC) && IS_MERC(ch)))
        return true;

    // Required class restrictions - any one of them must be met
    if ((IS_OBJ_STAT3(obj, ITEM3_REQ_MAGE) && IS_MAGE(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_CLERIC) && IS_CLERIC(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_THIEF) && IS_THIEF(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_WARRIOR) && IS_WARRIOR(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_BARB) && IS_BARB(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_PSIONIC) && IS_PSIONIC(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_PHYSIC) && IS_PHYSIC(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_CYBORG) && IS_CYBORG(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_KNIGHT) && IS_KNIGHT(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_RANGER) && IS_RANGER(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_BARD) && IS_BARD(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_MONK) && IS_MONK(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_VAMPIRE) && IS_VAMPIRE(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_MERCENARY) && IS_MERC(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE1) && IS_SPARE1(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE2) && IS_SPARE2(ch))
        || (IS_OBJ_STAT3(obj, ITEM3_REQ_SPARE3) && IS_SPARE3(ch)))
        return false;

    // A required class existed and the creature didn't fulfill any
    if (IS_OBJ_STAT3(obj, ITEM3_REQ_MAGE
            | ITEM3_REQ_CLERIC
            | ITEM3_REQ_THIEF
            | ITEM3_REQ_WARRIOR
            | ITEM3_REQ_BARB
            | ITEM3_REQ_PSIONIC
            | ITEM3_REQ_PHYSIC
            | ITEM3_REQ_CYBORG
            | ITEM3_REQ_KNIGHT
            | ITEM3_REQ_RANGER
            | ITEM3_REQ_BARD
            | ITEM3_REQ_MONK
            | ITEM3_REQ_VAMPIRE
            | ITEM3_REQ_MERCENARY
            | ITEM3_REQ_SPARE1 | ITEM3_REQ_SPARE2 | ITEM3_REQ_SPARE3))
        return true;

    // Passes all tests
    return false;
}

int
char_class_race_hit_bonus(struct creature *ch, struct creature *vict)
{
    int bonus = 0;
    // Height modifiers
    bonus += (IS_DWARF(ch) && (IS_OGRE(vict) || IS_TROLL(vict) ||
            IS_GIANT(vict) || (GET_HEIGHT(vict) > 2 * GET_HEIGHT(ch))));
    // Dwarven dislike of water or heights
    bonus -= (IS_DWARF(ch) && (room_is_watery(ch->in_room)
            || room_is_open_air(ch->in_room)));
    // Thieves operating in the dark
    bonus += (IS_THIEF(ch) && room_is_dark(ch->in_room));
    // Rangers like being outside
    bonus += (IS_RANGER(ch) && (SECT_TYPE(ch->in_room) == SECT_FOREST ||
            (SECT_TYPE(ch->in_room) != SECT_CITY &&
                SECT_TYPE(ch->in_room) != SECT_INSIDE && OUTSIDE(ch))));
    // Tabaxi in their native habitat
    bonus += (IS_TABAXI(ch) && SECT_TYPE(ch->in_room) == SECT_FOREST &&
        OUTSIDE(ch));
    return (bonus);
}

const int exp_scale[LVL_GRIMP + 2] = {
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
void
calculate_height_weight(struct creature *ch)
{
    struct race *race = race_by_idnum(GET_RACE(ch));
    int sex = ch->player.sex;

    if (sex == SEX_NEUTRAL)
        sex = (random_binary()) ? SEX_MALE:SEX_FEMALE;

    ch->player.weight = number(race->weight_min[sex],
                               race->weight_max[sex])
        + GET_STR(ch);
    ch->player.height = number(race->height_min[sex],
                               race->height_max[sex]);
    if (race->weight_add[sex])
        ch->player.height += ch->player.weight / race->weight_add[sex];
}

#undef __char_class_c__
