/************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: constants.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

#include "utils.h"
#include "constants.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "tmpstr.h"
#include "spells.h"
#include <libxml/parser.h>
#include "obj_data.h"

const char circlemud_version[] = {
    "CircleMUD, version 3.00 beta patchlevel 8\r\n"
};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/

/* (Note: strings for char_class definitions in char_class.c instead of here) */

const char *operator_str[] = {
    "[({",
    "])}",
    "|+",
    "&*",
    "^'"
};

const char *instrument_types[] = {
    "Percussion",
    "String",
    "Wind"
};

/* Extern Constant list for printing out who we sell to */
const char *trade_letters[] = {
    "Good",                     /* First, the alignment based ones */
    "Evil",
    "Neutral",
    "Magic User",               /* Then the char_class based ones */
    "Cleric",
    "Thief",
    "Warrior",
    "Barb",
    "Knight",
    "Ranger",
    "Monk",
    "Merc",
    "Bard",
    "Physic",
    "Psionic",
    "cyborg",
    "vampire",
    "spare1",
    "spare2",
    "spare3",
    "human",
    "elf",
    "dwarf",
    "half_orc",
    "tabaxi",
    "minotaur",
    "orc",
    "\n"
};

const char *desc_modes[] = {
    "playing",
    "disconnect",
    "account-login",
    "account-pw",
    "account-prompt",
    "account-verify",
    "password-prompt",
    "password-verify",
    "ansi-prompt",
    "compact-prompt",
    "email-prompt",
    "view-policy",
    "old-password-prompt",
    "new-password-prompt",
    "new-password-verify",
    "name-prompt",
    "sex-prompt",
    "hardcore-prompt",
    "class-prompt",
    "race-prompt",
    "align-prompt",
    "statistics-roll",
    "menu",
    "wait-menu",
    "class-remort",
    "delete-prompt",
    "delete-password",
    "delete-verify",
    "afterlife",
    "remort-afterlife",
    "view-background",
    "details-prompt",
    "edit-prompt",
    "edit-description",
    "network",
    "class-help",
    "race-help",
    "\n"
};

const char *temper_str[] = {
    "puke",
    "joint",
    "spit",
    "fart",
    "toke",
    "microsoft",
    "\n"
};

const char *shop_bits[] = {
    "WILL_FIGHT",
    "USES_BANK",
    "WANDERS",
    "\n"
};

const char *search_bits[] = {
    "REPEATABLE",
    "TRIPPED",
    "IGNORE",
    "CLANPASSWD",
    "TRIG_ENTER",
    "TRIG_FALL",
    "NOTRIG_FLY",
    "NOMOB",
    "NEWBIE_ONLY",
    "NOMSG",
    "NOEVIL",
    "NONEU",
    "NOGOOD",
    "NOMAGE",
    "NOCLER",
    "NOTHI",
    "NOBARB",
    "NORANG",
    "NOKNI",
    "NOMONK",
    "NOPSI",
    "NOPHY",
    "NOMERC",
    "NOBARD",
    "NOABBREV",
    "NOAFFMOB",
    "NOPLAYER",
    "REMORT_ONLY",
    "MATCH_ALL",
    "NOLOOK",
    "FAIL_TRIP",
    "\n"
};

const char *searchflag_help[] = {
    "search can be repeated an indefinite number of times.",
    "(RESERVED) search has been tripped.  internal use.",
    "process tripping command after search.",
    "this is a clan password search, do not display.",
    "tripped when someone walks/runs/flies into a room.",
    "tripped when falling into a room (e.g. spike pit).",
    "will not be tripped if the player is flying.",
    "cannot be tripped by mobs.",
    "can only be tripped by players level 6 and below.",
    "a damage effect will not emit its normal message.",
    "NOEVIL",
    "NONEU",
    "NOGOOD",
    "NOMAGE",
    "NOCLER",
    "NOTHI",
    "NOBARB",
    "NORANG",
    "NOKNI",
    "NOMONK",
    "NOPSI",
    "NOPHY",
    "NOMERC",
    "NOBARD",
    "abbreviated keywords will not work (prevent guessing.)",
    "spell and damage do not affect mobs in the room.",
    "NOPLAYER",
    "can only be tripped by remorts.",
    "all arguments must match",
    "causes the tripper not to look at the room after being transed.",
    "causes searches to be set tripped even if the search was failed.",
    "\n"
};

const char *wear_eqpos[] = {
    "light",
    "finger_right",
    "finger_left",
    "neck_1",
    "neck_2",
    "body",      /** 5 **/
    "head",
    "legs",
    "feet",
    "hands",
    "arms",      /** 10 **/
    "shield",
    "about",
    "waist",
    "wrist_right",
    "wrist_left", /** 15 **/
    "wield",
    "hold",
    "crotch",
    "eyes",
    "back",       /** 20 **/
    "belt",
    "face",
    "ear_left",
    "ear_right",
    "wield_2",
    "ass",
    "\n"
};

const char *wear_implantpos[] = {
    "light",
    "finger_right",
    "finger_left",
    "throat",
    "neck",
    "body",      /** 5 **/
    "head",
    "legs",
    "feet",
    "hands",
    "arms",      /** 10 **/
    "shield",
    "about",
    "waist",
    "wrist_right",
    "wrist_left", /** 15 **/
    "wield",
    "hold",
    "crotch",
    "eyes",
    "back",       /** 20 **/
    "belt",
    "face",
    "ear_left",
    "ear_right",
    "wield_2",
    "rectum",
    "\n"
};

const char *wear_tattoopos[] = {
    "-",
    "-",
    "-",
    "neck",
    "-",
    "chest",         /** 5 **/
    "scalp",
    "legs",
    "feet",
    "hands",
    "arms",      /** 10 **/
    "-",
    "-",
    "waist",
    "wrist_right",
    "wrist_left", /** 15 **/
    "-",
    "-",
    "crotch",
    "-",
    "back",       /** 20 **/
    "-",
    "face",
    "ear_left",
    "ear_right",
    "-",
    "buttocks",
    "\n"
};

/* Attack types */
const char *attack_type[] = {
    "hit",                      /* 0 */
    "sting",
    "whip",
    "slash",
    "bite",
    "bludgeon",                 /* 5 */
    "crush",
    "pound",
    "claw",
    "maul",
    "thrash",                   /* 10 */
    "pierce",
    "blast",
    "punch",
    "stab",
    "zap",                      /* Raygun blasting */
    "rip",
    "chop",
    "shoot",                    /* arrowz */
    "\n"
};

/* Reset__mode text */
const char *reset_mode[] = {
    "NEVER",
    "EMPTY",
    "ALWAYS",
    "\r\n"
};

const char *zone_pk_flags[] = {
    "!PK",
    "NPK",
    "CPK",
    "\n"
};

const char *zone_pk_names[] = {
    "no_pk",
    "neutral_pk",
    "chaotic_pk",
    "\n"
};

/* Zone flags */
const char *zone_flag_names[] = {
    "autosave",
    "resetsave",
    "notifyowner",
    "locked",
    "nomagic",
    "nolaw",                    /* 5 */
    "noweather",
    "nocrime",
    "frozen",
    "isolated",
    "sndprf",
    "noidle",
    "fullcontrol",
    "paused",
    "evilambience",
    "goodambience",
    "norecalc",
    "17",
    "18",
    "search_appr",
    "mobs_appr",                /* 20 */
    "objs_appr",
    "rooms_appr",
    "zcmds_appr",
    "inplay",
    "mobs_mod",                 /* 25 */
    "objs_mod",
    "rms_mod",
    "zon_mod",
    "\n"
};

/* Zone flags */
const char *zone_flags[] = {
    "AUTOSAVE",
    "RESETSAVE",
    "NOTIFYOWNER",
    "LOCKED",
    "!MAGIC",
    "!LAW",                     /* 5 */
    "!WEATHER",
    "!CRIME",
    "FROZEN",
    "ISOLATED",
    "SNDPRF",
    "NOIDLE",
    "FULLCONTROL",
    "PAUSED",
    "EVILAMBIENCE",
    "GOODAMBIENCE",
    "NORECALC",
    "17",
    "18",
    "SEARCH_APPR",
    "MOBS_APPR",                /* 20 */
    "OBJS_APPR",
    "ROOMS_APPR",
    "ZCMDS_APPR",
    "INPLAY",
    "MOBS_MOD",                 /* 25 */
    "OBJS_MOD",
    "RMS_MOD",
    "ZON_MOD",
    "\n"
};

/* cardinal directions */
const char *dirs[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "future",
    "past",
    "\n"
};

const char *to_dirs[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "into the future",
    "into the past",
    "\n"
};

const char *from_dirs[] = {
    "the south",
    "the west",
    "the north",
    "the east",
    "below",
    "above",
    "the past",
    "the future",
    "\n"
};

const char *search_commands[] = {
    "None",
    "Door",
    "Mobile",
    "Object",
    "Remove",
    "Give",
    "Equip",
    "Transport",
    "Spell",
    "Damage",
    "Spawn",
    "Loadroom",
    "\n"
};

const char *search_cmd_short[] = {
    "NONE",
    "DOOR",
    "MOBILE",
    "OBJECT",
    "REMOVE",
    "GIVE",
    "EQUIP",
    "TRANS",
    "SPELL",
    "DAMAGE",
    "SPAWN",
    "LDROOM",
    "\n"
};

const char *home_town_abbrevs[] = {
    "Modrian",
    "New Thalos",
    "Electro C.",
    "Elven Vill",
    "Istan",
    "Monk",
    "Newbie Tow",
    "Solace Cov",
    "Mavernal",
    "Zul'Dane",
    "Arena",
    "City",
    "DOOM",
    "13",
    "14",
    "Skullport",
    "Dwarven C.",
    "Human Sq.",
    "Drow Isle",
    "Astral M.",
    "Newbie S",
    "\n"
};

const char *home_towns[] = {
    "Modrian",
    "New Thalos",
    "Electro Centralis",
    "Elven Village",
    "Istan",
    "Monk Guild",
    "Tower of Modrian",
    "Solace Cove",
    "Mavernal",
    "Zul'Dane",
    "the Arena",
    "City",
    "DOOM",
    "13",
    "14",
    "Skullport",
    "Dwarven Caverns",
    "Human Square",
    "Drow Isle",
    "Skullport Newbie",
    "Astral Manse",
    "Newbie School",
    "Kromguard",
    "\n"
};

/* Imm levels */
const char *level_abbrevs[] = {
    "  AMB  ",
    " IMMORT",
    "BUILDER",
    "LUMINAR",
    " ETHER ",
    "ETERNAL",
    "DEMIGOD",
    "ELEMENT",
    " SPIRIT",
    " BEING ",
    " POWER ",
    " FORCE ",
    " ENERGY",
    "  GOD  ",
    " DEITY ",
    "TIMEGOD",
    "GREATER",
    "CREATOR",
    "ANCIENT",
    "  IMP  ",
    "LUCIFER",
    "BONEMAN",
    "SUPREMO"
};

const char *deity_names[] = {
    "NONE",
    "GUIHARIA",
    "PAN",
    "JUSTICE",
    "ARES",
    "KALAR",
    "\n"
};

const char *alignments[] = {
    "Good",
    "Neutral",
    "Evil",
    "\n",
};

/* ROOM_x */
const char *room_bits[] = {
    "DRK",
    "DTH",
    "!MOB",
    "IND",
    "NV",
    "SDPF",
    "!TRK",
    "!MAG",
    "TNL",
    "!TEL",
    "GDR",
    "HAS",
    "HCR",
    "COMFORT",
    "SMOKE",
    "!FLEE",
    "!PSI",
    "!SCI",
    "!RCL",
    "CLAN",
    "ARENA",
    "DOCK",
    "BURN",
    "FREEZ",
    "NULLMAG",
    "HOLYO",
    "RAD",
    "SLEEP",
    "EXPLOD",
    "POISON",
    "VACUUM",
    "\n"
};

const char *roomflag_names[] = {
    "dark",
    "deathtrap",
    "nomob",
    "indoor",
    "noviolence",
    "soundproof",
    "notrack",
    "nomagic",
    "RES (tunnel)",
    "noteleport",
    "godroom",
    "RES (house)",
    "RES (house crash)",
    "comfortable",
    "smoke-filled",
    "noflee",
    "nopsionics",
    "noscience",
    "norecall",
    "clanroom",
    "arena",
    "dock",
    "burning",
    "freezing",
    "nullmagic",
    "holyocean",
    "radioactive",
    "sleep gas",
    "explosive gas",
    "poison gas",
    "vacuum",
    "\n"
};

/* EX_x */
const char *exit_bits[] = {
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "HEAVY",
    "HARD-PICK",
    "!MOB",
    "HIDDEN",
    "!SCAN",
    "TECH",
    "ONE-WAY",
    "NOPASS",
    "THORNS",
    "THORNS_NOPASS",
    "STONE",
    "ICE",
    "FIRE",
    "FIRE_NOPASS",
    "FLESH",
    "IRON",
    "ENERGY_F",
    "ENERGY_F_NOPASS",
    "FORCE",
    "SPECIAL",
    "REINF",
    "SECRET",
    "\n"
};

/* SECT_ */
const char *sector_types[] = {
    "Inside",
    "City",
    "Field",
    "Forest",
    "Hills",
    "Mountains",
    "Water (Swim)",
    "Water (No Swim)",
    "Underwater",
    "Open Air",
    "Notime",
    "Climbing",
    "Outer Space",
    "Road",
    "Vehicle",
    "Farmland",
    "Swamp",
    "Desert",
    "Fire River",
    "Jungle",
    "Pitch Surface",
    "Pitch Submerged",
    "Beach",
    "Astral",
    "Elemental Fire",
    "Elemental Earth",
    "Elemental Air",
    "Elemental Water",
    "Elemental Positive",
    "Elemental Negative",
    "Elemental Smoke",
    "Elemental Ice",
    "Elemental Ooze",
    "Elemental Magma",
    "Elemental Lightning",
    "Elemental Steam",
    "Elemental Radiance",
    "Elemental Minerals",
    "Elemental Vacuum",
    "Elemental Salt",
    "Elemental Ash",
    "Elemental Dust",
    "Blood",
    "Rock",
    "Muddy",
    "Trail",
    "Tundra",
    "Catacombs",
    "Cracked Road",
    "Deep Ocean",
    "\n"
};

const char *planes[] = {
    "Prime One",
    "Prime Two",
    "Neverwhere",
    "Underdark",
    "Western",
    "Morbidian",                /* 5 */
    "Prime Seven",
    "Prime Eight",
    "Prime Nine",
    "Prime Ten",
    "Astral",
    "Avernus--Hell",
    "Dis--Hell",
    "Minauros--Hell",
    "Phlegethos--Hell",
    "Stygia--Hell",
    "Malbolge--Hell",
    "Maladomini--Hell",
    "Caina--Hell",
    "Nessus--Hell",
    "Ghenna",
    "21",
    "22",
    "23",
    "24",
    "The Abyss",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    "32",
    "33",
    "34",
    "35",
    "36",
    "37",
    "38",
    "OLC",
    "Olympus",
    "Costal",
    "42",
    "Heaven",
    "Elysium",
    "45",
    "46",
    "47",
    "48",
    "49",
    "DOOM",
    "Shadow",
    "52",
    "53",
    "54",
    "55",
    "56",
    "57",
    "58",
    "59",
    "60",
    "61",
    "62",
    "63",
    "64",
    "65",
    "Odyssey",
    "Paraelemental Smoke",
    "Paraelemental Ice",
    "Paraelemental Magma",
    "Elemental Water",
    "Elemental Fire",
    "Elemental Air",
    "Elemental Earth",
    "Elemental Positive",
    "Elemental Negative",
    "Paraelemental Magma",
    "Paraelemental Ooze",
    "\n"
};

const char *time_frames[] = {
    "Timeless",
    "Modrian Era",
    "Electro Era",
    "\n"
};

/* SEX_x */
const char *genders[] = {
    "Neuter",
    "Male",
    "Female",
    "\n"
};

/* POS_x */
const char *position_types[] = {
    "Dead",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing",
    "Flying",
    "Mounted",
    "Swimming",
    "\n"
};

// PLR2_x
const char *player2_bits[] = {
    "SOULLESS",
    "BURIED",
    "IN_COMBAT",
    "\n"
};

/* PLR_x */
const char *player_bits[] = {
    "HARDCORE",
    "!UNUSED!",
    "FROZEN",
    "DONTSET",
    "WRITING",
    "MAILING",
    "CSH",
    "SITEOK",
    "!SHOUT",
    "!TITLE",
    "!UNUSED!",
    "!UNUSED!",
    "!WIZL",
    "!DEL",
    "INVST",
    "CRYO",
    "AFK",
    "C_LEADR",
    "!UNUSED!",
    "OLC",
    "HALTED",
    "OLCGOD",
    "TESTER",
    "!UNUSED!",
    "MORTALIZED",
    "!UNUSED!",
    "!UNUSED!",
    "NOPOST",
    "KRN",
    "!UNUSED!",
    "NOPK",
    "\n"
};

/* NPC_x */
const char *action_bits[] = {
    "SPEC",
    "SENTINEL",
    "SCAVENGER",
    "ISNPC",
    "AWARE",
    "AGGR",
    "STAY-ZONE",
    "WIMPY",
    "AGGR_EVIL",
    "AGGR_GOOD",
    "AGGR_NEUTRAL",
    "MEMORY",
    "HELPER",
    "!CHARM",
    "!SUMMN",
    "!SLEEP",
    "!BASH",
    "!BLIND",
    "!TURN",
    "!PETRI",
    "PET",
    "SOULLESS",
    "SPRT_HNTR",
    "UTILITY",
    "\n"
};

const char *action_bits_desc[] = {
    "Special",
    "Sentinel",
    "Scavenger",
    "NPC",
    "Aware",
    "Aggressive",
    "Stay_Zone",
    "Wimpy",
    "Aggro_Evil",
    "Aggro_Good",
    "Aggro_Neutral",
    "Memory",
    "Helper",
    "NoCharm",
    "NoSummon",
    "NoSleep",
    "NoBash",
    "NoBlind",
    "NoTurn",
    "nopetri",
    "pet",
    "soulless",
    "Spirit_Hunter",
    "Utility",
    "\n"
};

const char *action2_bits[] = {
    "SCRIPT",
    "MOUNT",
    "STAY_SECT",
    "ATK_MOBS",
    "HUNT",
    "LOOT",
    "NOSTUN",
    "SELLR",
    "!WEAR",
    "SILENT",
    "FAMILIAR",
    "NO_FLOW",
    "!APPROVE",
    "RENAMED",
    "!AGGR_RACE",
    "15", "16", "17", "18", "19",
    "\n"
};

const char *action2_bits_desc[] = {
    "script",
    "Mount",
    "Stay_Sector",
    "Attack_Mobs",
    "Hunt",
    "Loot",
    "NoStun",
    "Seller",
    "NoWear",
    "Silent_Hunter",
    "Familiar",
    "NoFlow",
    "Unnapproved",
    "Renamed",
    "Noaggro_Race",
    "mugger",
    "\n"
};

/* PRF_x */
const char *preference_bits[] = {
    "BRieF",
    "CMPCT",
    "DEAF",
    "!TELL",
    "D_HP",
    "D_MAN",
    "D_MV",
    "AEX",
    "!HaSS",
    "NASTY",
    "SUMN",
    "!REP",
    "HLGHT",
    "C1",
    "C2",
    "!WiZ",
    "L1",
    "L2",
    "!AuC",
    "!GoS",
    "!GTZ",
    "RmFLG",
    "!SNooP",
    "!MuS",
    "!SpW",
    "!MiS",
    "!PROJ",
    "!INTWZ",
    "!CLN-SAY",
    "!IDENT",
    "!DREAM",
    "31",
    "\n"
};

const char *preference2_bits[] = {
    "DEBUG",
    "Newbie-HLPR",
    "DIAG",
    "BEEP",
    "!AFF",
    "!HOLR",
    "!IMM",
    "C_TIT",
    "C_HID",
    "L-READ",
    "AUTOPRMPT",
    "NOWHO",
    "ANON",
    "!TRAIL",
    "VT100",
    "AUTOSPLIT",
    "AUTOLOOT",
    "PK",
    "NOGECHO",
    "!UNUSED!",
    "DSPALGN",
    "WLDWRIT",
    "\n"
};

/* AFF_x */
const char *affected_bits[] = {
    "BLiND",
    "INViS",
    "DT-ALN",
    "DT-INV",
    "DT-MAG",
    "SNSE-L",
    "WaTWLK",
    "SNCT",
    "GRP",
    "CRSE",
    "NFRa",
    "PoIS",
    "PRT-E",
    "PRT-G",
    "SLeP",
    "!TRK",
    "NFLT",
    "TiM_W",
    "SNK",
    "HiD",
    "H2O-BR",
    "CHRM",
    "CoNFu",
    "!PaIN",
    "ReTNa",
    "ADReN",
    "CoNFi",
    "ReJV",
    "ReGN",
    "GlWL",
    "BlR",
    "31",
    "\n"
};

const char *affected_bits_desc[] = {
    "Blind",
    "Invisible",
    "Det_Alignment",
    "Det_Invisible",
    "Det_Magic",
    "Sense_Life",
    "Waterwalk",
    "Sanctuary",
    "Group",
    "Curse",
    "Infravision",
    "Poison",
    "Prot_Evil",
    "Prot_Good",
    "Sleep",
    "NoTrack",
    "inflight",
    "Time_Warp",
    "Sneak",
    "Hide",
    "WaterBreath",
    "Charm",
    "Confusion",
    "NoPain",
    "Retina",
    "Adrenaline",
    "Confidence",
    "Rejuvenation",
    "Regeneration",
    "Glowlight",
    "Blur",
    "\n"
};

const char *affected2_bits[] = {
    "FLUOR",
    "TRANSP",
    "SLOW",
    "HASTE",
    "MOUNTED",
    "FIRESHLD",
    "BERSERK",
    "INTIMD",
    "TRUE_SEE",
    "DIV_ILL",
    "PROT_UND",
    "INVS_UND",
    "ANML_KIN",
    "END_COLD",
    "PARALYZ",
    "PROT_LGTN",
    "PROT_FIRE",
    "TELEK",
    "PROT_RAD",
    "BURNING!",
    "!BEHEAD",
    "DISPLACR",
    "PROT_DEVILS",
    "MEDITATE",
    "EVADE",
    "BLADE BARRIER",
    "OBLIVITY",
    "NRG_FIELD",
    "PETRI",
    "VERTIGO",
    "PROT_DEMONS",
    "\n"
};

const char *affected2_bits_desc[] = {
    "Fluorescent",
    "Transparent",
    "Slow",
    "Haste",
    "Mounted",
    "Fireshield",
    "Berserk",
    "Intimidation",
    "True_See",
    "Divine_Ill",
    "Prot_Undead",
    "Invis_Undead",
    "Invis_Animals",
    "End_Cold",
    "Paralyze",
    "Prot_Lightning",
    "Prot_Fire",
    "Telekinesis",
    "Prot_Radiation",
    "Burning",
    "No_BeHead",
    "Displacer",
    "Prot_Devils",
    "Meditate",
    "Evade",
    "Blade_Barrier",
    "Oblivity",
    "Energy_Field",
    "Petri",
    "Vertigo",
    "Prot_Demons",
    "\n"
};

const char *affected3_bits[] = {
    "ATTR-FIELD",
    "ENERGY_LEAK",
    "POISON-2",
    "POISON-3",
    "SICK",
    "S-DEST!",
    "DAMCON",
    "STASIS",
    "P-SPHERE",
    "RAD",
    "DET_POISON",
    "MANA_TAP",
    "ENERGY_TAP",
    "SONIC_IMAGERY",
    "SHR_OBSC",
    "!BREATHE",
    "PROT_HEAT",
    "PSISHIELD",
    "PSY CRUSH",
    "2xDAM",
    "ACID",
    "HAMSTRING",
    "GRV WELL",
    "SMBL PAIN",
    "EMP_SHLD",
    "IAFF",
    "!UNUSED!",
    "TAINTED",
    "INFIL",
    "DivPwR",
    "MANA_LEAK",
    "\n"
};

const char *affected3_bits_desc[] = {
    "attraction_field",
    "Energy_Leak",
    "Poison_2",
    "Poison_3",
    "Sick",
    "Self_Destruct",
    "Damage_Control",
    "Stasis",
    "Prismatic_Sphere",
    "Radioactive",
    "Detect Poison",
    "Mana_Tap",
    "Energy_Tap",
    "Sonic_Imagery",
    "Shroud_Obscurement",
    "Nobreathe",
    "Prot_Heat",
    "psishield",
    "psychic_crush",
    "double_damage",
    "acidity",
    "hamstring",
    "gravity well",
    "symbol of pain",
    "emp shielding",
    "!instant affect!",
    "Sniped",
    "Tainted",
    "Infiltrating",
    "Divine Power",
    "Mana_Leak",
    "\n"
};

/* AFF_x */
const char *affected_bits_ident[] = {
    "This item will blind you.\r\n",
    "This item will make you invisible.\r\n",
    "This item will let you see auras of good and evil.\r\n",
    "This item will let you see invisible things.\r\n",
    "This item will let you sense magical auras.\r\n",
    "This item will make you magically sensitive to living beings.\r\n",
    "This item will let you walk on water.\r\n",
    "This item will provide you sanctuary.\r\n",
    "THIS ITEM HAS A BUG - PLEASE REPORT.\r\n",
    "This item will curse you when used.\r\n",
    "This item will give you infravision.\r\n",
    "This item will poison you.\r\n",
    "This item will protect you against evil creatures.\r\n",
    "This item will protect you against good creatures.\r\n",
    "This item will put you to sleep.\r\n",
    "This item will make you impossible to track.\r\n",
    "This item will give you the power of flight.\r\n",
    "This item will create a timewarp.\r\n",
    "This item will make you sneak.\r\n",
    "This item will hide you.\r\n",
    "This item will let you breathe water.\r\n",
    "This item will make you charmed.\r\n",
    "This item will make you confused.\r\n",
    "This item will remove from you the sensation of pain.\r\n",
    "This item will make your eyes much more sensitive.\r\n",
    "This item will start your adrenaline pumping.\r\n",
    "This item will make you more confident.\r\n",
    "This item will let you recover more quickly.\r\n",
    "This item will regenerate your wounds at much faster rate.\r\n",
    "This item will create a glowing ball of light.\r\n",
    "This item will blur your image.\r\n",
    "31",
    "\n"
};

const char *affected2_bits_ident[] = {
    "FLUOR",
    "TRANSP",
    "SLOW",
    "HASTE",
    "MOUNTED",
    "FIRESHLD",
    "BERSERK",
    "INTIMD",
    "TRUE_SEE",
    "DIV_ILL",
    "PROT_UND",
    "INVS_UND",
    "INVS_ANML",
    "END_COLD",
    "PARALYZ",
    "PROT_LGTN",
    "PROT_FIRE",
    "TELEK",
    "PROT_RAD",
    "BURNING!",
    "!BEHEAD",
    "DISPLACR",
    "PROT_DEVILS",
    "MEDITATE",
    "EVADE",
    "BLADE BARRIER",
    "OBLIVITY",
    "NRG_FIELD",
    "PETRI",
    "VERTIGO",
    "PROT_DEMONS",
    "\n"
};

/* WEAR_x - for eq list */
const char *where[] = {
    "<as light>       ",
    "<on finger>      ",
    "<on finger>      ",
    "<around neck>    ",
    "<around neck>    ",
    "<on body>        ",
    "<on head>        ",
    "<on legs>        ",
    "<on feet>        ",
    "<on hands>       ",
    "<on arms>        ",
    "<as shield>      ",
    "<about body>     ",
    "<about waist>    ",
    "<around wrist>   ",
    "<around wrist>   ",
    "<wielded>        ",
    "<held>           ",
    "<on crotch>      ",
    "<on eyes>        ",
    "<on back>        ",
    "<on belt>        ",
    "<on face>        ",
    "<on left ear>    ",
    "<on right ear>   ",
    "<second wielded> ",
    "<stuck up ass>   ",
    "\n"
};

const char *tattoo_pos_descs[] = {
    "<INVALID>        ",
    "<INVALID>        ",
    "<INVALID>        ",
    "<on neck>        ",
    "<INVALID>        ",
    "<on chest>       ",
    "<on scalp>       ",
    "<on legs>        ",
    "<on feet>        ",
    "<on hands>       ",
    "<on arms>        ",
    "<INVALID>        ",
    "<INVALID>        ",
    "<about waist>    ",
    "<around wrist>   ",
    "<around wrist>   ",
    "<INVALID>        ",
    "<INVALID>        ",
    "<around crotch>  ",
    "<INVALID>        ",
    "<on back>        ",
    "<INVALID>        ",
    "<on face>        ",
    "<on left ear>    ",
    "<on right ear>   ",
    "<INVALID>        ",
    "<on buttocks>    ",
    "\n"
};

/* ITEM_x (ordinal object types) */
const char *item_types[] = {
    "UNDEFINED",
    "LIGHT",
    "SCROLL",
    "WAND",
    "STAFF",
    "WEAPON",
    "CAMERA",
    "MISSILE",
    "TREASURE",
    "ARMOR",
    "POTION",
    "WORN",
    "OTHER",
    "TRASH",
    "TRAP",
    "CONTAINER",
    "NOTE",
    "LIQ CONT",
    "KEY",
    "FOOD",
    "MONEY",
    "PEN",
    "BOAT",
    "FOUNTAIN",
    "WINGS",
    "VR_INTERFACE",
    "SCUBA_MASK",
    "DEVICE",
    "INTERFACE",
    "HOLY SYMBOL",
    "VEHICLE",
    "ENGINE",
    "BATTERY",
    "ENERGY_GUN",
    "WINDOW",
    "PORTAL",
    "TOBACCO",
    "CIGARETTE",
    "METAL",
    "V-STONE",
    "PIPE",
    "TRANSPORTER",
    "SYRINGE",
    "CHIT",
    "SCUBA_TANK",
    "TATTOO",
    "TOOL",
    "BOMB",
    "DETONATOR",
    "FUSE",
    "PODIUM",
    "PILL",
    "ENERGY_CELL",
    "V_WINDOW",
    "V_DOOR",
    "V_CONSOLE",
    "GUN",
    "BULLET",
    "CLIP",
    "MICROCHIP",
    "COMMUNICATOR",
    "SCRIPT",
    "INSTRUMENT",
    "BOOK",
    "\n"
};

/* ITEM_x (ordinal object types) */
const char *item_type_descs[] = {
    "an undefined... something...",
    "a light source",
    "a magical scroll",
    "a magic wand",
    "a magical staff",
    "a weapon",
    "a camera",
    "a missile",
    "treasure",
    "a piece of armor",
    "a magical potion",
    "a piece of clothing",
    "just an object",
    "a piece of trash",
    "a trap",
    "a container",
    "a note",
    "a liquid container",
    "a key",
    "some food",
    "money",
    "a pen",
    "a boat",
    "a fountain",
    "a pair of wings",
    "a virtual reality interface",
    "a scuba mask",
    "a technical device",
    "a technical interface",
    "a holy symbol",
    "a vehicle",
    "an engine",
    "a battery",
    "an energy gun",
    "a window",
    "a magical portal",
    "something to smoke",
    "a cigarette",
    "a piece of metal",
    "a v-stone",
    "a pipe",
    "a transporter",
    "a syringe",
    "a chit",
    "a scuba tank",
    "a tattoo",
    "a tool",
    "a bomb",
    "a bomb detonator",
    "a bomb fuse",
    "a podium",
    "a pill",
    "an energy cell",
    "a vehicular window",
    "a door to a vehicle",
    "a vehicle's console",
    "a gun",
    "a bullet",
    "a clip for bullets",
    "a microchip",
    "a communicator",
    "a mobile script",
    "a musical instrument",
    "a book",
    "\n"
};

const char *item_value_types[][4] = {
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},
    {"Color", "Type", "Hours", "UNDEFINED"},    /* Light      */
    {"Level", "Spell1", "Spell2", "Spell3"},    /* Scroll     */
    {"Level", "Max Charg", "Cur Chrg", "Spell"},    /* wand       */
    {"Level", "Max Charg", "Cur Chrg", "Spell"},    /* staff      */
    {"Spell", "Dam dice1", "Dam dice ", "Atck type"},   /* weapon     */
    {"Targ room", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* camera */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* missile    */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* treasure   */
    {"AC-Apply", "UNDEFINED", "UNDEFINED", "UNDEFINED"},    /* armor      */
    {"Level", "Spell1", "Spell2", "Spell3"},    /* Potion     */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* worn       */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* other      */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* trash      */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* trap       */
    {"Max Capac", "Flags", "Keynum", "DONT SET"},   /* container  */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* note       */
    {"Max units", "Cur Unit", "Liq.type", "Poison"},    /* liq cont   */
    {"Keytype", "Rentflag", "UNDEFINED", "UNDEFINED"},  /* key        */
    {"Hours", "Spell lev", "Spellnum", "Poison"},   /* food       */
    {"Num Coins", "Type", "UNDEFINED", "UNDEFINED"},    /* money      */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* pen        */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* boat       */
    {"Max units", "Cur Units", "Liq.type", "Poison"},   /* fountain   */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* wings      */
    {"Startroom", "Cost", "Max Lev.", "Hometown"},  /* vr interface */
    {"State", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* scuba      */
    {"Max energ", "Cur Energ", "State", "Rate     "},   /* device     */
    {"Type", "UNDEFINED", "Max", "UNDEFINED"},  /* interface  */
    {"Align", "Class", "Min level", "Max level"},   /* holy symb  */
    {"Room/KeyNum", "Doorstate", "Flags", "Special"},   /* vehicle */
    {"Max energ", "Cur Energ", "Enginstat", "Rate     "},   /* engine     */
    {"Max charg", "Cur charg", "Rate", "Cost/unit"},    /* battery    */
    {"Drain Rate", "Dam dice1", "Dam dice2", "gun type"},   /* raygun     */
    {"Targ room", "Doorstate", "UNDEFINED", "UNDEFINED"},   /* window     */
    {"Targ room", "Doorstate", "Keynum?", "Charges"},   /* portal     */
    {"Type", "Max Drags", "UNDEFINED", "UNDEFINED"},    /*tobacco     */
    {"Drags lft", "UNDEFINED", "Tobac typ", "Lit?"},    /* joint      */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* metal      */
    {"Targ room", "Idnum", "Charges", "UNDEFINED"}, /* vstone     */
    {"Drags lft", "Max drags", "Tobac typ", "Lit?   "}, /* pipe       */
    {"Max Energy", "Cur Energ", "To-room", "tunable?"}, /* transporter */
    {"Level", "Spell1", "Spell2", "Spell3"},    /* Syringe     */
    {"Credits  ", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* chit */
    {"Max Units", "Cur Unit", "UNDEFINED", "UNDEFINED"},    /* scuba tank   */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* tattoo */
    {"skillnum", "modifier", "UNDEFINED", "UNDEFINED"}, /* tool * */
    {"Type", "Power", "UND", "Idnum"},  /* bomb * */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* detonator */
    {"Type", "State", "Timer", "UNDEFINED"},    /* fuse */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* podium */
    {"Level", "Spell1", "Spell2", "Spell3"},    /* Pill  */
    {"Max charg", "Cur charg", "Recharge rate", "UNDEF"},   /* energy cell */
    {"Room", "Doorflags", "Car Vnum", "UNDEFINED"}, /* v window    */
    {"Room", "Unused", "Car Vnum", "UNDEFINED"},    /* v door    */
    {"Room", "Unused", "Car Vnum", "Driver Idnum"}, /* v console */
    {"MAX ROF", "CUR ROF", "Max Load", "Gun Type"}, /* gun */
    {"UNDEFINED", "UNDEFINED", "Dam Mod", "Gun Type"},  /* bullet */
    {"UNDEFINED", "UNDEFINED", "Max Load", "Gun Type"}, /* clip */
    {"Type", "Data", "Max", "UNDEFINED"},   /* chip */
    {"Max Charge", "Cur Charge", "State", "Channel"},   /* Communicator  */
    {"Top Message", "Mode", "Wait Time", "Counter"},    /* Script  */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"},   /* Instrument */
    {"Level", "Spell1", "Spell2", "Spell3"},    /* Book */
    {"UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED"}
};

const char *smoke_types[] = {
    "none",
    "dirtweed",
    "desertweed",
    "indica",
    "unholy reefer",
    "tobacco",
    "hemlock",
    "peyote",
    "marijuana",
    "homegrown",
    "\n"
};

/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
    "TAKE",
    "FINGER",
    "NECK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "WIELD",
    "HOLD",
    "CROTCH",
    "EYES",
    "BACK",
    "BELT",
    "FACE",
    "EARS",
    "ASS",
    "\n"
};

const char *wear_keywords[] = {
    "!RESERVED! (light)",
    "finger",
    "!RESERVED! (finger)",
    "neck",
    "!RESERVED! (neck)",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "shield",
    "about",
    "waist",
    "wrist",
    "!RESERVED! (wrist)",
    "!RESERVED! (wield)",
    "!RESERVED! (hold)",
    "crotch",
    "eyes",
    "back",
    "belt",
    "face",
    "ear",
    "!RESERVED! (ear)",
    "WIELD 2",
    "ass",
    "\n"
};

const char *wear_description[] = {
    "!light!",
    "right finger",
    "left finger",
    "neck",
    "!neck!",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "!shield",
    "!about!",
    "waist",
    "right wrist",
    "left wrist",
    "!wield!",
    "!hold!",
    "crotch",
    "eyes",
    "back",
    "!belt!",
    "face",
    "left ear",
    "right ear",
    "!wield2!",
    "!ass!",
    "\n"
};

/* ITEM_x (extra bits) */
const char *extra_bits[] = {
    "GLOW",
    "HUM",
    "!RENT",
    "!DON",
    "!INVIS",
    "INVIS",
    "MAGIC",
    "!DROP",
    "BLESS",
    "!GOOD",
    "!EVIL",
    "!NEU",
    "!MAGE",
    "!CLE",
    "!THI",
    "!WAR",
    "!SELL",
    "!BAR",
    "!PSI",
    "!PHY",
    "!CYB",
    "!KNI",
    "!RAN",
    "!BARD",
    "!MONK",
    "BLUR",
    "!DISP_MAG",
    "unused138",
    "REP_FLD",
    "TRANSP",
    "DAMNED",
    "\n"
};

const char *extra_names[] = {
    "glow",
    "hum",
    "norent",
    "nodonate",
    "noinvis",
    "invisible",
    "magic",
    "nodrop",
    "bless",
    "nogood",
    "noevil",
    "noneutral",
    "nomage",
    "nocleric",
    "nothief",
    "nowarrior",
    "nosell",
    "nobarb",
    "nopsionic",
    "nophysic",
    "nocyborg",
    "noknight",
    "noranger",
    "nobard",
    "nomonk",
    "blur",
    "nodispel_magic",
    "unused138",
    "repulsion_field",
    "transparent",
    "evil_bless",
    "\n"
};

const char *extra2_bits[] = {
    "RADACT",
    "!MERC",
    "!SPR1",
    "!SPR2",
    "!SPR3",
    "HIDDEN",
    "TRAPPED",
    "SINGULAR",
    "!LOC",
    "!SOIL",
    "CAST_W",
    "2_HAND",
    "BODY PART",
    "ABLAZE",
    "PERM_CURSE",
    "!REMOVE",
    "THROWN",
    "GODEQ",
    "!MORT",
    "BROKEN",
    "IMPLANT",
    "REINF",
    "ENHAN",
    "MORT",
    "*",
    "*",
    "*",
    "*",
    "PROT_HUNT",
    "RENAMED",
    "_!APPROVE_",
    "\n"
};

const char *extra2_names[] = {
    "radioactive",
    "nomerc",
    "nospare1",
    "nospare2",
    "nospare3",
    "hidden",
    "trapped",
    "singular",
    "nolocate",
    "nosoilage",
    "casting_weapon",
    "two_handed",
    "body_part",
    "ablaze",
    "perm_curse",
    "noremove",
    "throwable",
    "godeq",
    "nomort",
    "broken",
    "implant",
    "reinforced",
    "enhanced",
    "mort",
    "*",
    "*",
    "*",
    "*",
    "prot_hunt",
    "renamed",
    "unapproved",
    "\n"
};

const char *extra3_bits[] = {
    "MAGE",
    "CLE",
    "THI",
    "WAR",
    "BAR",
    "PSI",
    "PHY",
    "CYB",
    "KNI",
    "RAN",
    "BARD",
    "MONK",
    "VAMP",
    "MER",
    "SPR1",
    "SPR2",
    "SPR3",
    "HARD",
    "STAY",
    "HUNT",
    "!MAG",
    "!SCI",
    "\n"
};

const char *extra3_names[] = {
    "mage",
    "cleric",
    "thief",
    "warrior",
    "barbarian",
    "psionic",
    "physic",
    "cyborg",
    "knight",
    "ranger",
    "bard",
    "monk",
    "vampire",
    "mercenary",
    "spare1",
    "spare2",
    "spare3",
    "hardened",
    "stayzone",
    "hunted",
    "nomagic",
    "noscience",
    "\n"
};

/* APPLY_x */
const char *apply_types[] = {
    "NONE",
    "STR",
    "DEX",
    "INT",
    "WIS",
    "CON",
    "CHA",
    "CLASS",
    "LEVEL",
    "AGE",
    "WEIGHT",
    "HEIGHT",
    "MAXMANA",
    "MAXHIT",
    "MAXMOVE",
    "GOLD",
    "EXP",
    "ARMOR",
    "HITROLL",
    "DAMROLL",
    "SAV_PARA",
    "SAV_ROD",
    "SAV_PETRI",
    "SAV_BREATH",
    "SAV_SPELL",
    "SNEAK",
    "HIDE",
    "RACE",
    "SEX",
    "BACKST",
    "PICK",
    "PUNCH",
    "SHOOT",
    "KICK",
    "TRACK",
    "IMPALE",
    "BEHEAD",
    "THROW",
    "RIDING",
    "TURN",
    "SAV_CHEM",
    "SAV_PSI",
    "ALIGN",
    "SAV_PHY",
    "CASTER",
    "WEAP_SPEED",
    "DISGUISE",
    "NOTHIRST",
    "NOHUNGER",
    "NODRUNK",
    "SPEED",
    "\n"
};

/* CONT_x */
const char *container_bits[] = {
    "CLOSEABLE",
    "PICKPROOF",
    "CLOSED",
    "LOCKED",
    "\n",
};

/* LIQ_x */
const char *drinks[] = {
    "water",
    "beer",
    "wine",
    "ale",
    "dark ale",
    "whiskey",
    "lemonade",
    "firebreather",
    "local specialty",
    "slime",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt water",
    "clear water",
    "coke",
    "firetalon",
    "soup",
    "mud",
    "holy water",
    "orange juice",
    "goatsmilk",
    "mucus",
    "pus",
    "sprite",                   /* 25 */
    "diet coke",
    "root beer",
    "vodka",
    "beer",                     /* city beer */
    "urine",                    /* 30 */
    "stout",
    "souls",
    "champagne",
    "cappucino",
    "rum",
    "sake",
    "chocolate milk",
    "juice",
    "mead",
    "\n"
};

/* other extern constants for liquids ******************************************/

/* one-word alias for each drink */
const char *drinknames[] = {
    "water",
    "beer",
    "wine",
    "ale",
    "ale",
    "whiskey",
    "lemonade",
    "firebreather",
    "local",
    "slime mold juice",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt water",
    "water clear",
    "coke",
    "firetalon",
    "soup",
    "mud",
    "holy water",               /* 20 */
    "orange juice",
    "goatsmilk",
    "mucus",
    "pus",
    "sprite",                   /* 25 */
    "diet",
    "root",
    "vodka",
    "beer",
    "urine",
    "stout",
    "souls",
    "champagne bubbly",
    "cappucino coffee",
    "rum",
    "sake",
    "chocolate milk",
    "juice",
    "mead",
    "\n"
};

/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
/* (DRUNK, HUNGER, THIRST)*/
const char drink_aff[][3] = {
    {0, 1, 10},                 /*water */
    {3, 2, 5},                  /* beer */
    {5, 2, 5},                  /* wine */
    {2, 2, 5},                  /* ale */
    {1, 2, 5},                  /* darkale */
    {6, 1, 4},                  /* whiskey *//* 5 */
    {0, 1, 8},                  /* lemonade */
    {10, 0, 0},                 /* firebreather */
    {3, 3, 3},                  /* local specialty */
    {0, 4, -8},                 /* slime */
    {0, 3, 6},                  /* milk *//* 10 */
    {0, 1, 6},                  /* tea */
    {0, 1, 6},                  /* coffee */
    {0, 2, -1},                 /* blood */
    {0, 1, -2},                 /* saltwater */
    {0, 0, 13},                 /* clear water *//* 15 */
    {0, 1, 10},                 /* coke */
    {10, 0, 1},                 /* fire talon */
    {0, 5, 5},                  /* soup */
    {0, 3, -2},                 /* mud */
    {0, 1, 10},                 /* holy water *//* 20 */
    {0, 2, 8},                  /* orange juice */
    {0, 3, 8},                  /* goatsmilk */
    {0, 3, -5},                 /* mucus */
    {0, 1, 0},                  /* pus */
    {0, 1, 10},                 /* sprite *//* 25 */
    {0, 1, 10},                 /* diet coke */
    {0, 1, 10},                 /* root beer */
    {12, 0, 2},                 /* vodka */
    {1, 1, 6},                  /* city beer */
    {0, 0, -2},                 /* urine *//* 30 */
    {5, 3, 6},                  /* stout  */
    {2, 1, 8},                  /* souls */
    {1, 2, 2},                  /* champagne */
    {0, 1, 6},                  /* cappucino */
    {10, 0, 2},                 /* rum */
    {12, 0, 2},                 /* sake */
    {0, 3, 6},                  // chocolate milk
    {-1, -1, -1},
};

/* color of the various drinks */
const char *color_liquid[] = {
    "clear",
    "brown",
    "deep red",
    "brown",
    "dark",
    "golden",                   /* 5 */
    "yellow",
    "green",
    "clear",
    "light green",
    "white",                    /* 10 */
    "brown",
    "black",
    "red",
    "clear",
    "crystal clear",            /* 15 */
    "dark carbonated",
    "glowing blue",
    "soupy",
    "thick, brown",
    "sparkling clear",
    "pulpy orange",
    "thick white",
    "sickly yellow",
    "pus white",
    "clear carbonated",
    "brown carbonated",         /* 25 */
    "brown carbonated",
    "clear",
    "brown",
    "stinky yellow",
    "dark black",               /* 30 */
    "translucent black",
    "sparkling pink",
    "creamy brown",
    "golden",
    "dark brown",
    "unknown",
    "\n"
};

/* level of fullness for drink containers */
const char *fullness[] = {
    "less than half ",
    "about half ",
    "more than half ",
    ""
};

/* str, int, wis, dex, con applies **************************************/

const char *reputation_msg[] = {
    "Innocent",                 // 0 reputation
    "Mostly Harmless",          // 1-99
    "Unfriendly",               // 100-199
    "Unkind",                   // 200-299
    "Cold",                     // 300-399
    "Daunting",                 // 400-499
    "Feared",                   // 500-599
    "Frightening",              // 600-699
    "Dreaded",                  // 700-799
    "Terrifying",               // 800-899
    "Monstrous",                // 900-999
    "True Killer"               // 1000 reputation
};

const char *spell_wear_off_msg[] = {
    "RESERVED DB.C",            /* 0 */
    "You feel less protected.", /* 1 */
    "!Teleport!",
    "You feel less righteous.",
    "Your eyesight returns.",
    "!Burning Hands!",          /* 5 */
    "!Call Lightning",
    "You feel more self-confident.",
    "Your limbs tingle as you feel your strength return.",
    "!Clone!",
    "!Color Spray!",            /* 10 */
    "!Control Weather!",
    "!Create Food!",
    "!Create Water!",
    "!Cure Blind!",
    "!Cure Critic!",            /* 15 */
    "!Cure Light!",
    "You feel more optimistic.",
    "You can no longer see the auras of others.",
    "Your eyes stop tingling.",
    "Your magical sensitivity disappears.", /* 20 */
    "Your sensitivity to poison is gone.",
    "!Dispel Evil!",
    "!Earthquake!",
    "!Enchant Weapon!",
    "!Energy Drain!",           /* 25 */
    "!Fireball!",
    "!Harm!",
    "!Heal!",
    "You feel yourself exposed.",
    "!Lightning Bolt!",         /* 30 */
    "!Locate object!",
    "!Magic Missile!",
    "You feel less sick.",
    "The feeling of invulnerability fades.",
    "!Remove Curse!",           /* 35 */
    "The white aura around your body fades.",
    "!Shocking Grasp!",
    "You feel less tired.",
    "You feel weaker.",
    "!Summon!",                 /* 40 */
    "!Ventriloquate!",
    "!Word of Recall!",
    "!Remove Poison!",
    "You feel less aware of your surroundings.",
    "!Animate Dead!",           /* 45 */
    "!Dispel Good!",
    "!Group Armor!",
    "!Group Heal!",
    "!Group Recall!",
    "Your night vision seems to fade.", /* 50 */
    "Your feet seem less buoyant.",
    "!UNUSED!",
    "!identify!",
    "The ghostly light around you fades.",
    "Your image is no longer blurred and shifting.",    /* 55 */
    "!knock!",
    "!dispel magic!",
    "!UNUSED!",
    "!dimension door",
    "!minor creation!",         /* 60 */
    "Your telekinesis has worn off.",
    "!sword!",
    "You shake off the stunned feeling you have had.",
    "!prismatic spray!",
    "The fire shield before you evaporates.",   /* 65 */
    "You feel less aware of scrying attempts.",
    "You are no longer clairvoyant.",
    "Your movements seem to speed back up to normal.",
    "!greater enchant!",
    "!enchant armor!",          /* 70 */
    "!minor identify!",
    "You can no longer fly.",
    "G Heal",
    "Your limbs tingle as you feel your strength return.",
    "You are no longer super-sensitive.",
    "The feeling of invulnerability fades.",
    "You are no longer protected from magical attacks.",
    "You are no longer protected from the undead.",
    "Spirit Hammer",
    "The effects of your prayer wear off.",
    "flame strike",
    "You are no longer able to endure the cold.",
    "magical vest",
    "You no longer heal so much from sleep.",
    "Your body stops regenerating.",
    "command",
    "Your feet are no longer able to walk on the air.",
    "The holy light about you fades.",
    "!goodberry!",
    "Your skin no longer feels hard like bark.",    /* 90 */
    "You feel exposed to the undead.",
    "You slow back down to normal.",
    "You no longer feel a kinship to animals.",
    "!Charm animal!",
    "!Refresh!",                /*95 */
    "You are no longer able to breathe water.",
    "!Conjure Elemental!",
    "You feel yourself exposed.",
    "You no longer feel safe from lightning.",
    "You are no longer protected from fire.",   /*100 */
    "!restoration!",
    "You feel your heightened intelligence fade.",
    "!gust of wind!",
    "!retrieve corpse!",
    "!local teleport!",         /* 105 */
    "Your image is no longer displaced.",
    "!peer!",
    "!meteor storm!",
    "You no longer feel safe from devils.",
    "You feel better.",         /* 110 */
    "!Ice Storm!",
    "!Astral Spell!",
    "!Rstone!",
    "!Disruption!",
    "!Stone to Flesh!",         /* 115 */
    "!Remove Sickness!",
    "The shroud of obscurement fades into nothingness.",
    "The prismatic sphere around you fades.",
    "!wall of thorns!",
    "!wall of stone!",          /* 120 */
    "!wall of ice!",
    "!wall of fire!",
    "!wall of iron!",
    "!thorn trap!",
    "!fiery sheet!",            /* 125 */
    "!chain lightning!",
    "!hailstorm!",
    "!ice storm!",
    "The shield of righteousness fades away.",
    "The blackmantle dissipates.",  /* 130 */
    "You are no longer sanctified.",
    "Your bleeding stigmata vanishes.",
    "!summon legion!",
    "The vegetative entanglement relaxes and releases you.",
    "Your anti-magic shell dissipates.",    /* 135 */
    "Your shield of divine intervention vanishes.",
    "Your sphere of desecration dissolves.",
    "Your malefic violation has worn off.",
    "You are no longer a righteous penetrator.",
    "!UNUSED!",                 /* 140 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "Your granite-like flesh softens.", /* 145 */
    "!UNUSED!",
    "The flow of blood obscures your vision as your rune of taint dissolves.",
    "!UNUSED!",
    "The power of Guiharia leaves you to your own devices.",
    "The effects of your death knell fade away.",   /* 150 */
    "You are no longer as telepathic.",
    "!UNUSED!",
    "!calm!",
    "The thorns protruding from your skin wither and fade away.",
    "!UNUSED!",                 /* 155 */
    "!UNUSED!", "!UNUSED!",
    "The fiery tingling in your throat has ceased.",
    "The cold tingling in your throat has ceased.",
    "!UNUSED!",                 /* 160 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 165 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 170 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 175 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 180 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 185 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 190 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 195 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 200 */
    "The strength zone of your brain relaxes.", /* 201 */
    "Your mental center of logic relaxes.",
    "You feel a sense of mental clarity return.",
    "You feel less afraid.",
    "!Satiation!",              /* 205 */
    "!Quench!",
    "You feel less confident now.",
    "Your body suddenly becomes aware of pain again.",
    "Your skin relaxes, and thins back out.",
    "!wound closure!",          /* 210 */
    "!Antibody!",
    "Your retina loses some of its sensitivity.",
    "Your adrenaline flow ceases.",
    "You begin to breathe again.",
    "You feel the vertigo fade.",   /* 215 */
    "Your metabolism slows.",
    "!ego whip!",
    "You feel the crushing psychic grip release its hold.",
    "You feel less relaxed.",
    "You feel your weakness pass.", /* 220 */
    "!iron will!",
    "Your cell regeneration slows.",
    "Your psionic shield dissipates.",
    "You feel a bit more clear-headed.",
    "!psychic conduit!",        /* 225 */
    "!psionic shatter!",
    "!id insinuation!",
    "You feel less sleepy.",
    "Your muscles stop convulsing.",
    "Your psychic resistance passes.",  /* 230 */
    "!mass hysteria!",
    "!group confidence!",
    "You feel less clumsy.",
    "Your endurance decreases.",
    "Your memories gradually return.",  /* 235 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 240 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "You no longer sense trails.",
    "You are no longer providing psychic feedback to your attackers.",  /* 245 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 250 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 255 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 260 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 265 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 270 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 275 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 280 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 285 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 290 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 295 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 300 */
    "You feel less acidic.",
    "The attraction field around you dissipates.",
    "!coordinates!",
    "The atoms around you cease to fluoresce.",
    "You feel less irradiated.",    /* 305 */
    "You feel less radioactive.",
    "!microwave!",
    "!oxidize!",
    "!random coordinates!",
    "Your repulsion field fades.",  /* 310 */
    "Your body is no longer transparent.",
    "You resume your normal pace through time.",
    "You resume your normal pace through time.",
    "!time warp!",
    "Spacetime relaxes, and gravity pulls you to the ground.",  /* 315 */
    "!UNUSED!",
    "You feel less refractive.",
    "!UNUSED!",
    "The vacuum shroud surrounding you dissipates.",
    "You feel less dense.",     /* 320 */
    "You feel less chemically inert.",
    "The rate of entropy around you returns to normal.",
    "The gravity well around you fades.",
    "Your energy level fades.", "!UNUSED!", /* 325 */
    "!UNUSED!",
    "Your molecular structure weakens.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 330 */
    "!UNUSED!", "!UNUSED!",
    "Time speeds back up to normal.",
    "Time seems to slow down as your movements return to normal.",
    "You feel less protected from metal.",  /* 335 */
    "Your albedo shield flickers and dissipates.",
    "!UNUSED!",
    "Your radio-immunity has worn off.",
    "!UNUSED!",
    "!UNUSED!",                 /* 340 */
    "Your electrostatic field dissipates.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 345 */
    "!instant audience!",
    "The wall of sound falters and fades.",
    "!UNUSED!",
    "!lament of longing!",
    "You are no longer misdirecting your pursuers.",    /* 350 */
    "The song of armament fades.",
    "!UNUSED!",
    "Your armor returns to normal harness.",
    "!exposure overture!",
    "!UNUSED!",                 /* 355 */
    "You start to feel hungry again.",
    "The effects of the melody of mettle dissipate.",
    "!lustration melisma!",
    "You feel more vulnerable to external forces.",
    "Your song bolstered confidence fades.",    /* 360 */
    "!UNUSED!",
    "The valor of fallen heroes leaves you to your own devices.",
    "!UNUSED!",
    "!UNUSED!",
    "You feel tired as the effect of the music leaves you.",    /* 365 */
    "!unravelling diapason!",
    "!UNUSED!",
    "The light leaves the air around you.  A chill sets in.",
    "Your musical asylum fades away.",
    "The white noise finally leaves your ears.",    /* 370 */
    "Your rage subsides.",
    "Your strength fails with your memory of the Power Overture.",
    "The power of the deities no longer flows in your veins.",
    "!UNUSED!",
    "!SONIC DISRUPTION!",       /* 375 */
    "Your mirror images waver and disappear.",
    "!clarifying harmonies!",
    "The swallow's wings return you gently to the ground.",
    "You no longer feel compelled to dance.",
    "!rhythm of alarm!",        /* 380 */
    "!Rhapsody of Remedy!",
    "!UNUSED!",
    "!home sweet home!",
    "The weight of the world descends upon your shoulders.",
    "!UNUSED!",                 /* 385 */
    "The slivers of music whirling around your body fade into nothingness.",
    "!DIRGE!",
    "You feel less charming.",
    "!UNUSED!",
    "Your life force is no longer fading.", /* 390 */
    "The air around you returns to normal.",
    "The rhythm invading your brain dissipates.",
    "!UNUSED!",
    "!UNUSED!",
    "!UNUSED!",                 /* 395 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 400 */
    "!zen healing!",
    "You feel your heightened awareness fade.",
    "You are no longer oblivious to pain.",
    "Your motions begin to feel less free.",
    "You become separate from the zen of translocation.",   /* 405 */
    "You lose the zen of celerity.",
    "You have lost the zen of dispassion.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 410 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 415 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 420 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 425 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 430 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 435 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 440 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 445 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 450 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 455 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 460 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 465 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 470 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 475 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 480 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 485 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 490 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 495 */

    "!UNUSED!", "!UNUSED!",
    "You are no longer shifted into a parallel dimension.", /*498 */
    "You recover from the effects of your cross-dimensional mishap.",   /*499 */
    "!UNUSED!",                 /* 500 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 505 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 510 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 515 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 520 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 525 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 530 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 535 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 540 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 545 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 550 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 555 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 560 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 565 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "You feel less inept.",     /* Pinch Alpha   *//* 570 */
    "You feel less inept.",     /* Pinch Beta    */
    "You feel your confusion pass.",    /* Pinch Gamma   */
    "You feel less clumsy.",    /* Pinch Delta   */
    "You feel your weakness fade.", /* Pinch Epsilon */
    "",                         /* Pinch Omega   *//* 575 */
    "!PINCH_ZETA!",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 580 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 585 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 590 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 595 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 600 */

    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 605 */
    "You feel less empowered.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 610 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 615 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 620 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 625 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 630 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 635 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 640 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 645 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "Your disguise has expired.",
    "!UNUSED!",                 /* 650 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 655 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 660 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 665 */
    "The wound in your leg seems to have closed.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 670 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!tumbling!", "!UNUSED!",   /* 675 */
    "!lingering song!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 680 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 685 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 690 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 695 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 700 */

    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 705 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 710 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 715 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 720 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 725 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 730 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 735 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "Your body is no longer stoned.",   /* Petrification */
    "You feel less sick.",      /* 740 */
    "!e_evil!",
    "!e_good!",
    "Your body expands to its normal size.",
    "!UNUSED!", "!UNUSED!",     /* 745 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 750 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 755 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 760 */
    "Your bright blue glow fades away.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 765 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 770 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 775 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 780 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 785 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 790 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 795 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 800 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 805 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 810 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 815 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 820 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 825 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 830 */
    "!LIGHTNING GUN SPEC!",
    "You feel less disoriented.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 835 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 840 */
    "\n"
};

const char *item_wear_off_msg[] = {
    "!RESERVED!",               /* 0 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 5 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 10 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 15 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 20 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 25 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 30 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 35 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 40 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 45 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 50 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 55 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 60 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 65 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 70 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 75 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 80 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 85 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 90 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 95 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 100 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 105 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 110 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 115 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 120 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 125 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 130 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 135 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 140 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 145 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 150 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "$p is no longer producing poison.",    /* 155 */
    "The elemental brands on $p shimmer and fade.",
    "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "The flame of faith on $p flickers and dies out.",  /* 160 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 165 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 170 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 175 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 180 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 185 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 190 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 195 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 200 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 205 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 210 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 215 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 220 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 225 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 230 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 235 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 240 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 245 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 250 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 255 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 260 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 265 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 270 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 275 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 280 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 285 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 290 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 295 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 300 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 305 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 310 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 315 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 320 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 325 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 330 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 335 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 340 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!",
    "$p ceases repelling matter.",
    "The attraction field on $p fades.",    /* 345 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 350 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 355 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 360 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 365 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 370 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 375 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 380 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 385 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 390 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 395 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 400 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 405 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 410 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 415 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 420 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 425 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 430 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 435 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 440 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 445 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 450 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 455 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 460 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 465 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 470 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 475 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 480 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 485 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 490 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 495 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 500 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 505 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 510 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 515 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 520 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 525 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 530 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 535 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 540 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 545 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 550 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 555 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 560 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 565 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 570 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 575 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 580 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 585 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 590 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 595 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 600 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 605 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 610 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 615 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 620 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 625 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 630 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 635 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 640 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 645 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 650 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 655 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 660 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 665 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 670 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 675 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 680 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 685 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 690 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 695 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 700 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 705 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 710 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 715 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 720 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 725 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 730 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 735 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 740 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 745 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 750 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 755 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 760 */
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", /* 765 */
    "\n"
};

const int rev_dir[] = {
    2,
    3,
    0,
    1,
    5,
    4,
    7,
    6
};

const int8_t movement_loss[] = {
    1,                          // Inside
    1,                          // City
    2,                          // Field
    3,                          // Forest
    4,                          // Hills
    5,                          // Mountains
    3,                          // Swimming
    3,                          // Unswimable
    4,                          // Underwater
    2,                          // Flying
    1,                          // Notime
    6,                          // Climbing
    2,                          // Free Space
    2,                          // Road
    1,                          // Vehicle
    2,                          // Cornfield
    3,                          // Swamp
    2,                          // Desert
    3,                          // Fire River
    10,                         // Jungle
    12,                         // Pitch Surface
    18,                         // Pitch Submerged
    3,                          // Beach
    1,                          // Astral
    3,                          // elemental fire
    20,                         // elemental earth
    1,                          // elemental air
    4,                          // elemental water
    0,                          // elemental positive
    50,                         // elemental negative
    3,                          // elemental smoke
    8,                          // elemental ice
    8,                          // elemental ooze
    9,                          // elemental magma
    6,                          // elemental lightning
    4,                          // elemental steam
    1,                          // elemental radiance
    12,                         // elemental minerals
    1,                          // elemental vacuum
    4,                          // elemental salt
    3,                          // elemental ash
    3,                          // elemental dust
    4,                          // blood
    1,                          // rock
    4,                          // muddy
    3,                          // trail
    3,                          // tundra
    1,                          // cracked_road
    6,                          // deep ocean
};

const char *weekdays[7] = {
    "the Day of the Moon",
    "the Day of the Bull",
    "the Day of the Deception",
    "the Day of Thunder",
    "the Day of Freedom",
    "the day of the Herb",
    "the Day of the Sun"
};

const char *month_name[16] = {
    "Month of Winter",          /* 0 */
    "Month of the Dire Wolf",
    "Month of the Frost Giant",
    "Month of the Old Forces",
    "Month of the Grand Struggle",
    "Month of Conception",
    "Month of Nature",
    "Month of Futility",
    "Month of the Dragon",
    "Month of the Sun",
    "Month of Madness",
    "Month of the Battle",
    "Month of the Dark Shades",
    "Month of Long Shadows",
    "Month of the Ancient Darkness",
    "Month of the Great Evil"
};

const int daylight_mod[16] = {
    -1, -1,
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    -1, -1
};

const char *sun_types[] = {
    "dark", "rise", "light", "set"
};

const char *sky_types[] = {
    "clear", "cloudy", "rain", "storm"
};

const char *lunar_phases[] = {
    "new",
    "waxing crescent",
    "first quarter",
    "waxing gibbous",
    "full",
    "waning gibbous",
    "last quarter",
    "waning crescent",
    "\n"
};

const char *moon_sky_types[] = {
    "not visible",
    "rising",
    "in the east",
    "directly overhead",
    "in the west",
    "setting",
    "\n"
};

const int8_t eq_pos_order[] = {
    WEAR_HEAD,
    WEAR_FACE,
    WEAR_EYES,
    WEAR_EAR_L,
    WEAR_EAR_R,
    WEAR_NECK_1,
    WEAR_NECK_2,
    WEAR_ABOUT,
    WEAR_BODY,
    WEAR_BACK,
    WEAR_ARMS,                  /* 10 */
    WEAR_SHIELD,
    WEAR_WRIST_R,
    WEAR_WRIST_L,
    WEAR_HANDS,
    WEAR_FINGER_R,
    WEAR_FINGER_L,
    WEAR_LIGHT,
    WEAR_HOLD,
    WEAR_WIELD,
    WEAR_WIELD_2,
    WEAR_WAIST,
    WEAR_BELT,
    WEAR_CROTCH,
    WEAR_LEGS,
    WEAR_FEET,
    WEAR_ASS,
    0, 0, 0, 0, 0, 0
};

const char implant_pos_order[] = {
    WEAR_HEAD,
    WEAR_FACE,
    WEAR_EYES,
    WEAR_EAR_L,
    WEAR_EAR_R,
    WEAR_NECK_1,
    WEAR_NECK_2,
    WEAR_ABOUT,
    WEAR_BODY,
    WEAR_BACK,
    WEAR_ARMS,                  /* 10 */
    WEAR_SHIELD,
    WEAR_WRIST_R,
    WEAR_WRIST_L,
    WEAR_HANDS,
    WEAR_FINGER_R,
    WEAR_FINGER_L,
    WEAR_LIGHT,
    WEAR_WAIST,
    WEAR_BELT,
    WEAR_CROTCH,
    WEAR_LEGS,
    WEAR_FEET,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char tattoo_pos_order[] = {
    WEAR_HEAD,
    WEAR_FACE,
    WEAR_EAR_L,
    WEAR_EAR_R,
    WEAR_NECK_1,
    WEAR_BODY,
    WEAR_BACK,
    WEAR_ARMS,                  /* 10 */
    WEAR_WRIST_R,
    WEAR_WRIST_L,
    WEAR_HANDS,
    WEAR_WAIST,
    WEAR_BELT,
    WEAR_CROTCH,
    WEAR_LEGS,
    WEAR_FEET,
    WEAR_ASS,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char *material_names[] = {
    "indeterminate",
    "water",
    "fire",
    "shadow",
    "gelatin",
    "light",
    "dreams",
    "*",
    "*",
    "*",
    "paper",                /** 10 **/
    "papyrus",
    "cardboard",
    "hemp",
    "parchment",
    "*",
    "*",
    "*",
    "*",
    "*",
    "cloth",             /** 20 **/
    "silk",
    "cotton",
    "polyester",
    "vinyl",
    "wool",
    "satin",
    "denim",
    "carpet",
    "velvet",
    "nylon",               /** 30 **/
    "canvas",
    "sponge",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "leather",           /** 40 **/
    "suede",
    "hard leather",
    "skin",
    "fur",
    "scales",
    "hair",
    "ivory",
    "*",
    "*",
    "flesh",             /** 50 **/
    "bone",
    "tissue",
    "cooked meat",
    "raw meat",
    "cheese",
    "egg",
    "*",
    "*",
    "*",
    "vegetable",         /** 60 **/
    "leaf",
    "grain",
    "bread",
    "fruit",
    "nut",
    "flower petal",
    "fungus",
    "slime",
    "*",
    "candy",             /** 70 **/
    "chocolate",
    "pudding",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "wood",              /** 80 **/
    "oak",
    "pine",
    "maple",
    "birch",
    "mahogany",
    "teak",
    "rattan",
    "ebony",
    "bamboo",
    "*",                /** 90 **/
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "metal",             /** 100 **/
    "iron",
    "bronze",
    "steel",
    "copper",
    "brass",
    "silver",
    "gold",
    "platinum",
    "electrum",
    "lead",              /** 110 **/
    "tin",
    "chrome",
    "aluminum",
    "silicon",
    "titanium",
    "adamantium",
    "cadmium",
    "nickel",
    "mithril",
    "pewter",                 /** 120 **/
    "plutonium",
    "uranium",
    "rust",
    "orichalcum",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",                 /** 130 **/
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "plastic",           /** 140 **/
    "kevlar",
    "rubber",
    "fiberglass",
    "asphalt",
    "concrete",
    "wax",
    "phenolic",
    "latex",
    "enamel",
    "*",                 /** 150 **/
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "glass",             /** 160 **/
    "crystal",
    "lucite",
    "porcelain",
    "ice",
    "shell",
    "earthenware",
    "pottery",
    "ceramic",
    "*",
    "*",                /** 170 **/
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "stone",             /** 180 **/
    "azurite",
    "agate",
    "moss Agate",
    "banded Agate",
    "eye Agate",
    "tiger Eye Agate",
    "quartz",
    "rose Quartz",
    "smoky Quartz",
    "quartz 2",
    "hematite",         /** 190 **/
    "lapis lazuli",
    "malachite",
    "obsidian",
    "rhodochorsite",
    "tiger eye",
    "turquoise",
    "bloodstone",
    "carnelian",
    "chalcedony",       /** 200 **/
    "chysoprase",
    "citrine",
    "jasper",
    "moonstone",
    "onyx",
    "zircon",
    "amber",
    "alexandrite",
    "amethyst",
    "oriental amethyst",/** 210 **/
    "aquamarine",
    "chrysoberyl",
    "coral",
    "garnet",
    "jade",
    "jet",
    "pearl",
    "peridot",
    "spinel",
    "topaz",            /** 220 **/
    "oriental topaz",
    "tourmaline",
    "sapphire",
    "black sapphire",
    "star sapphire",
    "ruby",
    "star ruby",
    "opal",
    "fire opal",
    "diamond",          /** 230 **/
    "sandstone",
    "marble",
    "emerald",
    "mud",
    "clay",
    "labradorite",
    "iolite",
    "spectrolite",
    "charolite",
    "basalt",
    "ash",
    "ink",
    "\n"
};

const int weapon_proficiencies[] = {
    0,                          /* TYPE_HIT */
    SKILL_PROF_WHIP,
    SKILL_PROF_WHIP,
    SKILL_PROF_SLASH,           /* TYPE_SLASH */
    0,                          /* TYPE_BITE */
    SKILL_PROF_POUND,
    SKILL_PROF_CRUSH,
    SKILL_PROF_POUND,
    SKILL_PROF_SLASH,           /* TYPE_CLAW */
    SKILL_PROF_CRUSH,           /* TYPE_MAUL */
    SKILL_PROF_WHIP,            /* TYPE_THRASH */
    SKILL_PROF_PIERCE,
    SKILL_PROF_BLAST,
    0,                          /* TYPE_PUNCH */
    SKILL_PROF_PIERCE,
    SKILL_ENERGY_WEAPONS,
    SKILL_PROF_SLASH,           /* TYPE_RIP */
    SKILL_PROF_SLASH,           /* TYPE_CHOP */
    0, 0, 0
};

const char *soilage_bits[] = {
    "wet",
    "bloody",
    "muddy",
    "covered with feces",
    "dripping with urine",
    "smeared with mucus",
    "spattered with saliva",
    "covered with acid",
    "oily",
    "sweaty",
    "greasy",
    "sooty",
    "slimy",
    "sticky",
    "covered with vomit",
    "rusty",
    "charred",
    "\n"
};

const char *trail_flags[] = {
    "BLOODPRINTS",
    "BLOOD_DROPS",
    "\n"
};

const char *spell_bits[] = {
    "DAM",
    "AFF",
    "UNAFF",
    "PNTS",
    "OBJS",
    "GRP",
    "MASS",
    "AREA",
    "SUMM",
    "CREAT",
    "SPEC",
    "OBJ",
    "TCH",
    "MAG",
    "DIV",
    "PHY",
    "PSI",
    "BIO",
    "CYB",
    "EVIL",
    "GOOD",
    "EXIT",
    "OUTD",
    "!WTR",
    "ZAP",
    "!SUN",
    "ZEN",
    "MERC",
    "SONG",
    "\n"
};

const char *spell_bit_keywords[] = {
    "damage",
    "affects",
    "unaffects",
    "points",
    "alterations",
    "groups",
    "masses",
    "areas",
    "summons",
    "creations",
    "manual",
    "objects",
    "touch",
    "magic",
    "divine",
    "physics",
    "psionic",
    "biologic",
    "cybernetic",
    "evil",
    "good",
    "exits",
    "outdoors",
    "nowater",
    "waterzap",
    "nosun",
    "zen",
    "mercenary",
    "song",
    "\n"
};

const struct weap_spec_info weap_spec_char_class[NUM_CLASSES] = {
    {
        3.5, 3},                // mage
    {
        2.5, 4},                // cleric
    {
        2.0, 5},                // thief
    {
        1.5, 6},                // warrior
    {
        1.5, 6},                // barbarian
    {
        3.5, 3},                // psionic
    {
        3.0, 3},                // physic
    {
        2.0, 5},                // cyborg
    {
        1.5, 6},                // knight
    {
        1.5, 6},                // ranger
    {
        2.0, 5},                // bard
    {
        2.5, 3},                // monk
    {
        1.5, 6},                // vampire
    {
        1.5, 6},                // merc
    {
        1.5, 6},                // spare1
    {
        1.5, 6},                // spare2
    {
        1.5, 6}                 // spare3
};

const int wear_bitvectors[] =
    { ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER,
    ITEM_WEAR_NECK, ITEM_WEAR_NECK, ITEM_WEAR_BODY,
    ITEM_WEAR_HEAD, ITEM_WEAR_LEGS, ITEM_WEAR_FEET,
    ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST,
    ITEM_WEAR_WRIST, ITEM_WEAR_WIELD, ITEM_WEAR_HOLD,
    ITEM_WEAR_CROTCH, ITEM_WEAR_EYES, ITEM_WEAR_BACK,
    ITEM_WEAR_BELT, ITEM_WEAR_FACE, ITEM_WEAR_EAR,
    ITEM_WEAR_EAR, ITEM_WEAR_WIELD, ITEM_WEAR_ASS
};

const char *grievance_kind_descs[] = { "murder", "theft", "attack", "\n" };

const char *logtypes[] = {
    "off", "brief", "normal", "complete", "\n"
};


#undef __constants_cc__
