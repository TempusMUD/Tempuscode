/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spell_parser.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __spell_parser_c__

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "materials.h"
#include "char_class.h"
#include "fight.h"
#include "screen.h"
#include "tmpstr.h"
#include "vendor.h"

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct room_direction_data *knock_door = NULL;
char locate_buf[256];

#define SINFO spell_info[spellnum]

extern int mini_mud;

extern struct room_data *world;
int find_door(struct Creature *ch, char *type, char *dir,
	const char *cmdname);
void name_from_drinkcon(struct obj_data *obj);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, this should provide
 * ample slots for skills.
 */

int max_spell_num = 0;

const char *spells[] = {
	"!RESERVED!",				/* 0 - reserved */

	"armor",					/* 1 */
	"teleport",
	"bless",
	"blindness",
	"burning hands",
	"call lightning",
	"charm person",
	"chill touch",
	"clone",
	"color spray",				/* 10 */
	"control weather",
	"create food",
	"create water",
	"cure blind",
	"cure critic",
	"cure light",
	"curse",
	"detect alignment",
	"detect invisibility",
	"detect magic",				/* 20 */
	"detect poison",
	"dispel evil",
	"earthquake",
	"enchant weapon",
	"energy drain",
	"fireball",
	"harm",
	"heal",
	"invisibility",
	"lightning bolt",			/* 30 */
	"locate object",
	"magic missile",
	"poison",
	"protection from evil",
	"remove curse",
	"sanctuary",
	"shocking grasp",
	"drowsy",
	"strength",
	"summon",					/* 40 */
	"ventriloquate",
	"word of recall",
	"remove poison",
	"sense life",
	"animate dead",
	"dispel good",
	"group armor",
	"group heal",
	"group recall",
	"infravision",				/* 50 */
	"waterwalk",
	"mana shield",
	"identify",
	"glowlight",
	"blur",						/* 55 */
	"knock",
	"dispel magic",
	"warding sigil",
	"dimension door",
	"minor creation",			/* 60 */
	"telekinesis",
	"phantasmic sword",
	"word stun",
	"prismatic spray",
	"fire shield",				/* 65 */
	"detect scrying",
	"clairvoyance",
	"slow",
	"greater enchant",
	"enchant armor",			/* 70 */
	"minor identify",
	"fly",
	"greater heal",
	"cone of cold",
	"true seeing",				/* 75 */
	"protection from good",
	"magical protection",
	"undead protection",
	"spiritual hammer",
	"pray",						/* 80 */
	"flame strike",
	"endure cold",
	"magical vestment",
	"rejuvenate",
	"regenerate",				/* 85 */
	"command",
	"air walk",
	"divine illumination",
	"goodberry",
	"barkskin",					/* 90 */
	"invis to undead",
	"haste",
	"animal kin",
	"charm animal",
	"refresh",					/* 95 */
	"breathe water",
	"conjure elemental",
	"greater invis",
	"protection from lightning",
	"protection from fire",		/* 100 */
	"restoration",
	"word of intellect",
	"gust of wind",
	"retrieve corpse",
	"local teleport",			/* 105 */
	"displacement",
	"peer",
	"meteor storm",
	"protection from devils",
	"symbol of pain",			/* 110 */
	"icy blast",
	"astral spell",
	"vestigial rune",
	"disruption",
	"stone to flesh",			/* 115 */
	"remove sickness",
	"shroud of obscurement",
	"prismatic sphere",
	"wall of thorns",
	"wall of stone",			/* 120 */
	"wall of ice",
	"wall of fire",
	"wall of iron",
	"thorn trap",
	"fiery sheet",				/* 125 */
	"chain lightning",
	"hailstorm",
	"ice storm",
	"shield of righteousness",
	"blackmantle",				/* 130 */
	"sanctification",
	"stigmata",
	"summon legion",
	"entangle",
	"anti-magic shell",			/* 135 */
	"divine intervention",
	"sphere of desecration",
	"malefic violation",
	"righteous penetration",
	"unholy stalker",			/* 140 */
	"inferno",
	"vampiric regeneration",
	"banishment",
	"control undead",
	"stoneskin",				/* 145 */
	"sun ray",
	"taint",
	"locust regeneration",
	"divine power",
	"death knell",	/* 150 */
	"telepathy",
	"damn", 
    "calm", 
    "thorn skin", 
    "!UNUSED!",	/* 155 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 160 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 165 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 170 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 175 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 180 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 185 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 190 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 195 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 200 */
	"power",					/* Sychik triggers */
	"intellect",
	"confusion",
	"fear",
	"satiation",				/*205 */
	"quench",
	"confidence",
	"nopain",
	"dermal hardening",
	"wound closure",			/*210 */
	"antibody",
	"retina",
	"adrenaline",
	"breathing stasis",
	"vertigo",					/* 215 */
	"metabolism increase",
	"ego whip",
	"psychic crush",
	"relaxation",
	"weakness",					/* 220 */
	"fortress of iron will",
	"cell regeneration",
	"psionic shield",
	"psychic surge",
	"psychic conduit",			/* 225 */
	"psionic shatter",
	"id insinuation",
	"melatonic flood",
	"motor spasm",
	"psychic resistance",		/* 230 */
	"mass hysteria",
	"group confidence",
	"clumsiness",
	"endurance",
	"amnesia",					/* 235 */
	"nullpsi",
	"!UNUSED!",
	"distraction",
	"call rodent",
	"call bird",	/* 240 */
	"call reptile",
	"call beast",
	"call predator",
	"spirit track",
	"!UNUSED!",	/* 245 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 250 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 255 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 260 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 265 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 270 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 275 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 280 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 285 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 290 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 295 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 300 */
	"acidity",
	"attraction field",
	"nuclear wasteland",
	"fluorescence",
	"gamma ray",				/* 305 */
	"halflife",
	"microwave",
	"oxidize",
	"random coordinates",
	"repulsion field",			/* 310 */
	"transmittance",
	"spacetime imprint",
	"spacetime recall",
	"time warp",
	"tidal spacesurf",			/* 315 */
	"fission blast",
	"refraction",
	"electroshield",
	"vacuum shroud",
	"densify",					/* 320 */
	"chemical stability",
	"entropy field",
	"gravity well",
	"capacitance boost",
	"electric arc",					/* 325 */
	"!UNUSED!",
	"lattice hardening",
	"nullify",
	"!UNUSED!", "!UNUSED!",		/* 330 */
	"!UNUSED!", "!UNUSED!", 
    "temporal compression",     /* 333 */
    "temporal dilation", 
    "gauss shield",	/* 335 */
	"albedo shield",
	"!UNUSED!",
	"radioimmunity",
	"!UNUSED!",
	"area stasis",				/* 340 */
	"electrostatic field",
	"electromagnetic pulse",
	"quantum rift",
	"!UNUSED!", "!UNUSED!",		/* 345 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 350 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 355 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 360 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 365 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 370 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 375 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 380 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 385 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 390 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 395 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 400 */

	"zen of healing",
	"zen of awareness",
	"zen of oblivity",
	"zen of motion",
	"zen of translocation",	/* 405 */
	"zen of celerity",
	"!UNUSED!",
	"!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 410 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 415 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 420 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 425 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 430 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 435 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 440 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 445 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 450 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 455 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 460 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 465 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 470 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 475 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 480 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 485 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 490 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 495 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 500 */
	/* SKILLS */

	"backstab",					/* 501 */
	"bash",
	"hide",
	"kick",
	"pick lock",
	"punch",
	"rescue",
	"sneak",
	"steal",
	"track",					/* 510 */
	"piledrive",
	"sleeper",
	"elbow",
	"kneethrust",
	"berserk",					/* 515 */
	"stomp",
	"bodyslam",
	"choke",
	"clothesline",
	"tag",						/* 520 */
	"intimidate",
	"speed healing",
	"stalk",
	"hearing",
	"bearhug",					/* 525 */
	"bite",
	"double attack",
	"bandage",
	"firstaid",
	"medic",					/* 530 */
	"consider",
	"glance",
	"headbutt",
	"gouge",
	"stun",						/* 535 */
	"feign",
	"conceal",
	"plant",
	"hotwire",
	"shoot",					/* 540 */
	"behead",
	"disarm",
	"spinkick",
	"roundhouse",
	"sidekick",					/* 545 */
	"spinfist",
	"jab",
	"hook",
	"sweepkick",
	"trip",						/* 550 */
	"uppercut",
	"groinkick",
	"claw",
	"rabbitpunch",
	"impale",					/* 555 */
	"pele kick",
	"lunge punch",
	"tornado kick",
	"circle",
	"triple attack",			/* 560 */
	"palm strike",
	"throat strike",
	"whirlwind",
	"hip toss",
	"combo",					/* 565 */
	"death touch",
	"crane kick",
	"ridgehand",
	"scissor kick",
	"alpha nerve",				/* 570 */
	"beta nerve",
	"gamma nerve",
	"delta nerve",
	"epsilon nerve",
	"omega nerve",				/* 575 */
	"zeta nerve",
	"meditate",
	"kata",
	"evasion",
	"second weapon",			/* 580 */
	"scanning",
	"cure disease",
	"battle cry",
	"autopsy",
	"transmute",				/* 585 */
	"metalworking",
	"leatherworking",
	"demolitions",
	"psiblast",
	"psilocate",				/* 590 */
	"psidrain",
	"gunsmithing",
	"elusion",
	"pistolwhip",
	"crossface",				/* 595 */
	"wrench",
	"cry from beyond",
	"kia",
	"wormhole",
	"lecture",					/* 600 */
	"turn",
	"analyze",
	"evaluate",
	"holytouch",
	"night vision",				/* 605 */
	"empower",
	"swimming",
	"throwing",
	"riding",
	"pipemaking",				/* 610 */
	"charge",
	"counter attack",	 /****  evasion  ****/
	"reconfigure",
	"reboot",
	"motion sensor",			/* 615 */
	"stasis",
	"energy field",
	"reflex boost",
	"power boost",
	"!UNUSED!",					/* 620 */
	"fastboot script",
	"self destruct",
	"!UNUSED!",
	"bioscan",
	"discharge",				/* 625 */
	"self repair",
	"cyborepair",
	"overhaul",
	"damage control",
	"electronics",				/* 630 */
	"hacking",
	"cyberscan",
	"cybo surgery",
	"energy weapons",
	"projectile weapons",		/* 635 */
	"speed loading",
	"hyperscanning",
	"overdrain",
	"de-energize",
	"assimilate",				/* 640 */
	"radionegation",
	"implant weaponry",
	"advanced implant weaponry",
	"offensive posturing",
	"defensive posturing",		/* 645 */
	"melee combat tactics",
	"cogenic neural bridging",
	"retreat",
	"disguise",
	"ambush",					/* 650 */
	"flying",
	"summon",
	"feed",
	"drain",
	"beguile",					/* 655 */
	"create vampire",
	"control undead",
	"terrorize",
	"!UNUSED!",
	"!UNUSED!",					/* 660 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 665 */
	"hamstring", "snatch", "drag", "snipe", "infiltrate",	/* 670 */
	"shoulderthrow", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 675 */
	"arterial flow enhancement",
	"genetek optimmunal response",
	"shukutei adrenal maximizations",
	"energy conversion",
	"!UNUSED!",					/* 680 */
	"blunt weapons",
	"whipping weapons",
	"piercing weapons",
	"slashing weapons",
	"crushing weapons",
	"blasting weapons",
	"break door",
	"archery",
	"bowery and fletchery",
	"read scrolls",				/* 690 */
	"use wands",
	"discipline of steel",
	"strike",
	"cleave",
	"great cleave",	/* 695 */
	"appraise",
	"garotte",
	"shield mastery", "uncanny dodge", "!UNUSED!",	/* 700 */

	/* OBJECT SPELLS AND NPC SPELLS/SKILLS */

	/* "identify",        */
	"!UNUSED!",
	"fire breath",				/* 702 */
	"gas breath",
	"frost breath",
	"acid breath",				/* 705 */
	"lightning breath",
	"lava breath",
	"!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 710 */
	"earth elemental",
	"fire elemental",
	"water elemental",
	"air elemental",
	"!UNUSED!",					/* 715 */
	"!UNUSED!",
	"hell fire",
	"javelin of lightning",
	"troglodytian stench",
	"spikes of manticore",		/* 720 */
	"!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 725 */
	"!UNUSED!",
	"!UNUSED!",
	"!UNUSED!",
	"!UNUSED!",
	"catapult",					/* 730 */
	"balista",
	"boiling oil",
	"hotsands",
	"mana restore",
	"blade barrier",
	"summon minotaur",
	"stygian lightning",
	"skunk stench",
	"petrify",
	"sickness",
	"essence of evil",
	"essence of goodness",
	"shrinking",
	"hell frost",
	"alien blood",
	"surgery",
	"smoke effects",
	"hell firestorm",
	"hell froststorm",
	"shadow breath",
	"steam breath",
	"trampling",
	"gore horns",
	"tail lash",
	"boiling pitch",
	"falling",
	"holyocean",
	"freezing",
	"drowning",
	"burning",
	"quad damage",
	"vader choke",
	"acid burn",
	"Zhengis Fist of Annihilation",
	"radiation sickness",
	"flamethrower",
	"gouged his own eyes out",
	"youth",
	"\n"						/* the end */
};


struct syllable {
	char *org;
	char *new_syl;
};


struct syllable syls[] = {
	{" ", " "},
	{"ar", "abra"},
	{"ate", "i"},
	{"cau", "kada"},
	{"blind", "nose"},
	{"bur", "mosa"},
	{"cu", "judi"},
	{"de", "oculo"},
	{"dis", "mar"},
	{"ect", "kamina"},
	{"en", "uns"},
	{"gro", "cra"},
	{"light", "dies"},
	{"lo", "hi"},
	{"magi", "kari"},
	{"mon", "bar"},
	{"mor", "zak"},
	{"move", "sido"},
	{"ness", "lacri"},
	{"ning", "illa"},
	{"per", "duda"},
	{"ra", "gru"},
	{"re", "candus"},
	{"son", "sabru"},
	{"tect", "infra"},
	{"tri", "cula"},
	{"ven", "nofo"},
	{"word of", "inset"},
	{"clair", "alto'k"},
	{"ning", "gzapta"},
	{"vis", "tappa"},
	{"light", "lumo"},
	{"fly", "a lifto"},
	{"align", "iptor "},
	{"prot", "provec "},
	{"fire", "infverrn'o"},
	{"heat", "skaaldr"},
	{"divine", "hohhlguihar"},
	{"sphere", "en'kompaz"},
	{"animate", "ak'necro pa"},
	{"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"},
		{"g", "t"},
	{"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"},
		{"n", "b"},
	{"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
		{"u", "e"},
	{"v", "z"},
	{"w", "x"},
	{"x", "n"},
	{"y", "l"},
	{"z", "k"},
	{"-", "rr't"},
	{"", ""}
};

int
mag_manacost(struct Creature *ch, int spellnum)
{
	int mana, mana2;

	mana = MAX(SINFO.mana_max -
		(SINFO.mana_change *
			(GET_LEVEL(ch) -
				SINFO.min_level[(int)MIN(NUM_CLASSES - 1, GET_CLASS(ch))])),
		SINFO.mana_min);

	if (GET_REMORT_CLASS(ch) >= 0) {
		mana2 = MAX(SINFO.mana_max -
			(SINFO.mana_change *
				(GET_LEVEL(ch) -
					SINFO.min_level[(int)MIN(NUM_CLASSES - 1,
							GET_REMORT_CLASS(ch))])), SINFO.mana_min);
		mana = MIN(mana2, mana);
	}

	return mana;
}


/* say_spell erodes buf, buf1, buf2 */
void
say_spell(struct Creature *ch, int spellnum, struct Creature *tch,
	struct obj_data *tobj)
{
	char lbuf[256], xbuf[256];
	int j, ofs = 0;

	*buf = '\0';
	strcpy(lbuf, spell_to_str(spellnum));

	while (*(lbuf + ofs)) {
		for (j = 0; *(syls[j].org); j++) {
			if (!strncasecmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
				strcat(buf, syls[j].new_syl);
				ofs += strlen(syls[j].org);
			}
		}
	}

	/* Magic Casting messages... */

	if (SPELL_IS_PSIONIC(spellnum)) {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(buf2,
					"$n momentarily closes $s eyes and concentrates.");
				send_to_char(ch, "You close your eyes and concentrate.\r\n");
			} else {
				sprintf(buf2,
					"$n closes $s eyes and touches $N with a mental finger.");
				act("You close your eyes and touch $N with a mental finger.",
					FALSE, ch, 0, tch, TO_CHAR);
			}
		} else if (tobj != NULL && tobj->in_room == ch->in_room) {
			sprintf(buf2,
				"$n closes $s eyes and touches $p with a mental finger.");
			act("You close your eyes and touch $p with a mental finger.",
				FALSE, ch, tobj, 0, TO_CHAR);
		} else {
			sprintf(buf2,
				"$n closes $s eyes and slips into the psychic world.");
			send_to_char(ch, 
				"You close your eyes and slip into a psychic world.\r\n");
		}
	} else if (SPELL_IS_PHYSICS(spellnum)) {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(buf2,
					"$n momentarily closes $s eyes and concentrates.");
				send_to_char(ch, "You close your eyes and make a calculation.\r\n");
			} else {
				sprintf(buf2, "$n looks at $N and makes a calculation.");
				act("You look at $N and make a calculation.", FALSE, ch, 0,
					tch, TO_CHAR);
			}
		} else if (tobj != NULL && tobj->in_room == ch->in_room) {
			sprintf(buf2, "$n looks directly at $p and makes a calculation.");
			act("You look directly at $p and make a calculation.", FALSE, ch,
				tobj, 0, TO_CHAR);
		} else {
			sprintf(buf2,
				"$n closes $s eyes and slips into a deep calculation.");
			send_to_char(ch, 
				"You close your eyes and make a deep calculation.\r\n");
		}
	} else if (SPELL_IS_MERCENARY(spellnum)) {
		sprintf(buf2, "$n searches through his backpack for materials.");
		send_to_char(ch, "You search for the proper materials.\r\n");
	} else {
		if (tch != NULL && tch->in_room == ch->in_room) {
			if (tch == ch) {
				sprintf(lbuf,
					"$n closes $s eyes and utters the words, '%%s'.");
				sprintf(xbuf, "You close your eyes and utter the words, '%s'.",
					buf);
			} else {
				sprintf(lbuf, "$n stares at $N and utters, '%%s'.");
				sprintf(xbuf, "You stare at $N and utter, '%s'.", buf);
			}
		} else if (tobj != NULL &&
			((tobj->in_room == ch->in_room) || (tobj->carried_by == ch))) {
			sprintf(lbuf, "$n stares at $p and utters the words, '%%s'.");
			sprintf(xbuf, "You stare at $p and utter the words, '%s'.", buf);
		} else {
			sprintf(lbuf, "$n utters the words, '%%s'.");
			sprintf(xbuf, "You utter the words, '%s'.", buf);
		}

		act(xbuf, FALSE, ch, tobj, tch, TO_CHAR);
		sprintf(buf1, lbuf, spell_to_str(spellnum));
		sprintf(buf2, lbuf, buf);
	}

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (*it == ch || *it == tch || !(*it)->desc || !AWAKE((*it)) ||
			PLR_FLAGGED((*it), PLR_WRITING | PLR_OLC))
			continue;
		perform_act(buf2, ch, tobj, tch, (*it), 0);
	}

	if (tch != NULL && tch != ch && tch->in_room == ch->in_room) {
		if (SPELL_IS_PSIONIC(spellnum)) {
			sprintf(buf1, "$n closes $s eyes and connects with your mind.");
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		} else if (SPELL_IS_PHYSICS(spellnum)) {
			sprintf(buf1,
				"$n closes $s eyes and alters the reality around you.");
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		} else {
			sprintf(buf1, "$n stares at you and utters the words, '%s'.", buf);
			act(buf1, FALSE, ch, NULL, tch, TO_VICT);
		}
	}
}

// 06/18/99
// reversed order of args to is_abbrev
// inside while. 
int
find_skill_num(char *name)
{
	int index = 0, ok;
	char *temp, *temp2;
	char spellname[256];
	char first[256], first2[256];

	while (*spell_to_str(++index) != '\n') {
		if (is_abbrev(name, spell_to_str(index)))
			return index;

		ok = 1;
		strncpy(spellname, spell_to_str(index), 255);
		temp = any_one_arg(spellname, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			//if (!is_abbrev(first, first2))
			if (!is_abbrev(first2, first))
				ok = 0;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return index;
	}

	return -1;
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 *
 * returns 1 on success of casting, 0 on failure
 *
 * return_flags has CALL_MAGIC_VICT_KILLED bit set if cvict dies
 *
 */
int
call_magic(struct Creature *caster, struct Creature *cvict,
	struct obj_data *ovict, int spellnum, int level, int casttype,
	int *return_flags)
{

	int savetype, mana = -1;
	bool same_vict = false;
	struct affected_type *af_ptr = NULL;

	if (return_flags)
		*return_flags = 0;

	if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
		return 0;

	if ((ROOM_FLAGGED(caster->in_room, ROOM_NOMAGIC)) &&
		GET_LEVEL(caster) < LVL_TIMEGOD && (SPELL_IS_MAGIC(spellnum) ||
			SPELL_IS_DIVINE(spellnum))) {
		send_to_char(caster, "Your magic fizzles out and dies.\r\n");
		act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
		return 0;
	}
	if ((ROOM_FLAGGED(caster->in_room, ROOM_NOPSIONICS)) &&
		GET_LEVEL(caster) < LVL_TIMEGOD && SPELL_IS_PSIONIC(spellnum)) {
		send_to_char(caster, "You cannot establish a mental link.\r\n");
		act("$n appears to be psionically challenged.",
			FALSE, caster, 0, 0, TO_ROOM);
		return 0;
	}
	if ((ROOM_FLAGGED(caster->in_room, ROOM_NOSCIENCE)) &&
		SPELL_IS_PHYSICS(spellnum)) {
		send_to_char(caster, 
			"You are unable to alter physical reality in this space.\r\n");
		act("$n tries to solve an elaborate equation, but fails.", FALSE,
			caster, 0, 0, TO_ROOM);
		return 0;
	}
	if (SPELL_USES_GRAVITY(spellnum) && NOGRAV_ZONE(caster->in_room->zone) &&
		SPELL_IS_PHYSICS(spellnum)) {
		send_to_char(caster, "There is no gravity here to alter.\r\n");
		act("$n tries to solve an elaborate equation, but fails.",
			FALSE, caster, 0, 0, TO_ROOM);
		return 0;
	}

    if (ovict) {
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOMAG)) {
            if  (SPELL_IS_MAGIC(spellnum) || SPELL_IS_DIVINE(spellnum)) {
                act("$p hums and shakes for a moment "
                    "then rejects your spell!", TRUE, caster, ovict, 0, TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's spell!", TRUE, caster, ovict, 0, TO_ROOM); 
                return 0;
            }
        }
        if (IS_SET(ovict->obj_flags.extra3_flags, ITEM3_NOSCI)) {
            if (SPELL_IS_PHYSICS(spellnum)) {
                act("$p hums and shakes for a moment "
                    "then rejects your alteration!", TRUE, caster, ovict, 0, TO_CHAR);
                act("$p hums and shakes for a moment "
                    "then rejects $n's alteration!", TRUE, caster, ovict, 0, TO_ROOM); 
                return 0;
            }
        }
    }

	/* stuff to check caster vs. cvict */
	if (cvict && caster != cvict) {
		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
			!peaceful_room_ok(caster, cvict, false)) {
			if (SPELL_IS_PSIONIC(spellnum)) {
				send_to_char(caster, "The Universal Psyche descends on your mind and "
					"renders you powerless!\r\n");
				act("$n concentrates for an instant, and is suddenly thrown "
					"into mental shock!\r\n", FALSE, caster, 0, 0, TO_ROOM);
				return 0;
			}
			if (SPELL_IS_PHYSICS(spellnum)) {
				send_to_char(caster, 
					"The Supernatural Reality prevents you from twisting "
					"nature in that way!\r\n");
				act("$n attemps to violently alter reality, but is restrained "
					"by the whole of the universe.", FALSE, caster, 0, 0,
					TO_ROOM);
				return 0;
			} else {
				send_to_char(caster, 
					"A flash of white light fills the room, dispelling your "
					"violent magic!\r\n");
				act("White light from no particular source suddenly fills the room, " "then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
				return 0;
			}
		}

		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)) &&
			!ok_damage_vendor(caster, cvict))
			return 0;

		if ((SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
			check_killer(caster, cvict);
            //Try to make this a little more sane...
/*			if ((SPELL_IS_PSIONIC(spellnum) || casttype == CAST_PSIONIC) &&
				(spellnum != SPELL_PSIONIC_SHATTER ||
					mag_savingthrow(cvict, level, SAVING_PSI)) &&
				mag_savingthrow(cvict, level, SAVING_PSI) &&
				AFF3_FLAGGED(cvict, AFF3_PSISHIELD) && caster != cvict) {
				act("Your psychic attack is deflected by $N's psishield!",
					FALSE, caster, 0, cvict, TO_CHAR);
				act("$n's psychic attack is deflected by $N's psishield!",
					FALSE, caster, 0, cvict, TO_NOTVICT);
				act("$n's psychic attack is deflected by your psishield!",
					FALSE, caster, 0, cvict, TO_VICT);
				return 0;
			}*/
            if (cvict->distrusts(caster) &&
				AFF3_FLAGGED(cvict, AFF3_PSISHIELD) && 
                (SPELL_IS_PSIONIC(spellnum) || casttype == CAST_PSIONIC)) {
                bool failed = false;
                int prob, percent;
                
                if (spellnum == SPELL_PSIONIC_SHATTER && 
                    !mag_savingthrow(cvict, level, SAVING_PSI))
                    failed = true;
                
                prob = CHECK_SKILL(caster, spellnum) + GET_INT(caster);
                prob += caster->getLevelBonus(spellnum);

                percent = cvict->getLevelBonus(SPELL_PSISHIELD);
                percent += number(1, 120);

                if (mag_savingthrow(cvict, GET_LEVEL(caster), SAVING_PSI))
                    percent <<= 1;

                if (GET_INT(cvict) < GET_INT(caster))
                    percent += (GET_INT(cvict) - GET_INT(caster)) << 3;

                if (percent >= prob)
                    failed = true;

                if (failed) {
                    act("Your psychic attack is deflected by $N's psishield!",
                        FALSE, caster, 0, cvict, TO_CHAR);
                    act("$n's psychic attack is deflected by $N's psishield!",
                        FALSE, caster, 0, cvict, TO_NOTVICT);
                    act("$n's psychic attack is deflected by your psishield!",
                        FALSE, caster, 0, cvict, TO_VICT);
                    return 0; 
                }
            }
		}
		if (cvict->distrusts(caster)) {
			af_ptr = NULL;
			if (SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum))
				af_ptr = affected_by_spell(cvict, SPELL_ANTI_MAGIC_SHELL);
			if (!af_ptr && IS_EVIL(caster) && SPELL_IS_DIVINE(spellnum))
				af_ptr = affected_by_spell(cvict, SPELL_DIVINE_INTERVENTION);
			if (!af_ptr && IS_GOOD(caster) && SPELL_IS_DIVINE(spellnum))
				af_ptr = affected_by_spell(cvict, SPELL_SPHERE_OF_DESECRATION);

			if (af_ptr && number(0, af_ptr->level) > number(0, level)) {
				act(tmp_sprintf("$N's %s absorbs $n's %s!",
						spell_to_str(af_ptr->type),
						spell_to_str(spellnum)),
					false, caster, 0, cvict, TO_NOTVICT);
				act(tmp_sprintf("Your %s absorbs $n's %s!",
						spell_to_str(af_ptr->type),
						spell_to_str(spellnum)),
					false, caster, 0, cvict, TO_VICT);
				act(tmp_sprintf("$N's %s absorbs your %s!",
						spell_to_str(af_ptr->type),
						spell_to_str(spellnum)),
					false, caster, 0, cvict, TO_CHAR);
				GET_MANA(cvict) = MIN(GET_MAX_MANA(cvict),
					GET_MANA(cvict) + (level >> 1));
				if (casttype == CAST_SPELL) {
					mana = mag_manacost(caster, spellnum);
					if (mana > 0)
						GET_MANA(caster) =
							MAX(0, MIN(GET_MAX_MANA(caster),
								GET_MANA(caster) - mana));
				}
				if ((af_ptr->duration -= (level >> 2)) <= 0) {
					send_to_char(cvict, "Your %s dissolves.\r\n", spell_to_str(af_ptr->type));
					affect_remove(cvict, af_ptr);
				}
				return 0;
				
			}
		}
	}
	/* determine the type of saving throw */
	switch (casttype) {
	case CAST_STAFF:
	case CAST_SCROLL:
	case CAST_POTION:
	case CAST_WAND:
		savetype = SAVING_ROD;
		break;
	case CAST_SPELL:
		savetype = SAVING_SPELL;
		break;
	case CAST_PSIONIC:
		savetype = SAVING_PSI;
		break;
	case CAST_PHYSIC:
		savetype = SAVING_PHY;
		break;
	case CAST_CHEM:
		savetype = SAVING_CHEM;
		break;
	case CAST_PARA:
		savetype = SAVING_PARA;
		break;
	case CAST_PETRI:
		savetype = SAVING_PETRI;
		break;
	case CAST_BREATH:
		savetype = SAVING_BREATH;
		break;
	case CAST_INTERNAL:
		savetype = SAVING_NONE;
		break;
	default:
		savetype = SAVING_BREATH;
		break;
	}
	if (IS_SET(SINFO.routines, MAG_DAMAGE)) {
		if (caster == cvict)
			same_vict = true;

		int retval = mag_damage(level, caster, cvict, spellnum, savetype);

		//
		// somebody died (either caster or cvict)
		//

		if (retval) {

			if (IS_SET(retval, DAM_ATTACKER_KILLED) ||
				(IS_SET(retval, DAM_VICT_KILLED) && same_vict)) {
				if (return_flags) {
					*return_flags = retval | DAM_ATTACKER_KILLED;
				}

				return 1;
			}

			if (return_flags) {
				*return_flags = retval;
			}
			return 1;
		}
	}

	if (IS_SET(SINFO.routines, MAG_EXITS) && knock_door)
		mag_exits(level, caster, caster->in_room, spellnum);

	if (IS_SET(SINFO.routines, MAG_AFFECTS))
		mag_affects(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
		mag_unaffects(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_POINTS))
		mag_points(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
		mag_alter_objs(level, caster, ovict, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_GROUPS))
		mag_groups(level, caster, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_MASSES))
		mag_masses(level, caster, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_AREAS)) {
		int retval = mag_areas(level, caster, spellnum, savetype);
		if (retval) {
			if (IS_SET(retval, DAM_ATTACKER_KILLED)
				|| (IS_SET(retval, DAM_VICT_KILLED) && same_vict)) {
				if (return_flags) {
					*return_flags = retval | DAM_ATTACKER_KILLED;
				}
				return 1;
			}
			if (return_flags) {
				*return_flags = retval;
			}
			return 1;
		}
	}
	if (IS_SET(SINFO.routines, MAG_SUMMONS))
		mag_summons(level, caster, ovict, spellnum, savetype);

	if (IS_SET(SINFO.routines, MAG_CREATIONS))
		mag_creations(level, caster, spellnum);

	if (IS_SET(SINFO.routines, MAG_OBJECTS) && ovict)
		mag_objects(level, caster, ovict, spellnum);


	if (IS_SET(SINFO.routines, MAG_MANUAL))
		switch (spellnum) {
		case SPELL_ASTRAL_SPELL:
			MANUAL_SPELL(spell_astral_spell); break;
		case SPELL_ENCHANT_WEAPON:
			MANUAL_SPELL(spell_enchant_weapon); break;
		case SPELL_CHARM:
			MANUAL_SPELL(spell_charm); break;
		case SPELL_CHARM_ANIMAL:
			MANUAL_SPELL(spell_charm_animal); break;
		case SPELL_WORD_OF_RECALL:
			MANUAL_SPELL(spell_recall); break;
		case SPELL_IDENTIFY:
			MANUAL_SPELL(spell_identify); break;
		case SPELL_SUMMON:
			MANUAL_SPELL(spell_summon); break;
		case SPELL_LOCATE_OBJECT:
			MANUAL_SPELL(spell_locate_object); break;
		case SPELL_ENCHANT_ARMOR:
			MANUAL_SPELL(spell_enchant_armor); break;
		case SPELL_GREATER_ENCHANT:
			MANUAL_SPELL(spell_greater_enchant); break;
		case SPELL_MINOR_IDENTIFY:
			MANUAL_SPELL(spell_minor_identify); break;
		case SPELL_CLAIRVOYANCE:
			MANUAL_SPELL(spell_clairvoyance); break;
		case SPELL_MAGICAL_VESTMENT:
			MANUAL_SPELL(spell_magical_vestment); break;
		case SPELL_TELEPORT:
		case SPELL_RANDOM_COORDINATES:
			MANUAL_SPELL(spell_teleport); break;
		case SPELL_CONJURE_ELEMENTAL:
			MANUAL_SPELL(spell_conjure_elemental); break;
		case SPELL_KNOCK:
			MANUAL_SPELL(spell_knock); break;
		case SPELL_SWORD:
			MANUAL_SPELL(spell_sword); break;
		case SPELL_ANIMATE_DEAD:
			MANUAL_SPELL(spell_animate_dead); break;
		case SPELL_CONTROL_WEATHER:
			MANUAL_SPELL(spell_control_weather); break;
		case SPELL_GUST_OF_WIND:
			MANUAL_SPELL(spell_gust_of_wind); break;
		case SPELL_RETRIEVE_CORPSE:
			MANUAL_SPELL(spell_retrieve_corpse); break;
		case SPELL_LOCAL_TELEPORT:
			MANUAL_SPELL(spell_local_teleport); break;
		case SPELL_PEER:
			MANUAL_SPELL(spell_peer); break;
		case SPELL_VESTIGIAL_RUNE:
			MANUAL_SPELL(spell_vestigial_rune); break;
		case SPELL_ID_INSINUATION:
			MANUAL_SPELL(spell_id_insinuation); break;
		case SPELL_SHADOW_BREATH:
			MANUAL_SPELL(spell_shadow_breath); break;
		case SPELL_SUMMON_LEGION:
			MANUAL_SPELL(spell_summon_legion); break;
		case SPELL_NUCLEAR_WASTELAND:
			MANUAL_SPELL(spell_nuclear_wasteland); break;
		case SPELL_SPACETIME_IMPRINT:
			MANUAL_SPELL(spell_spacetime_imprint); break;
		case SPELL_SPACETIME_RECALL:
			MANUAL_SPELL(spell_spacetime_recall); break;
		case SPELL_TIME_WARP:
			MANUAL_SPELL(spell_time_warp); break;
		case SPELL_UNHOLY_STALKER:
			MANUAL_SPELL(spell_unholy_stalker); break;
		case SPELL_CONTROL_UNDEAD:
			MANUAL_SPELL(spell_control_undead); break;
		case SPELL_INFERNO:
			MANUAL_SPELL(spell_inferno); break;
		case SPELL_BANISHMENT:
			MANUAL_SPELL(spell_banishment); break;
		case SPELL_AREA_STASIS:
			MANUAL_SPELL(spell_area_stasis); break;
		case SPELL_SUN_RAY:
			MANUAL_SPELL(spell_sun_ray); break;
		case SPELL_EMP_PULSE:
			MANUAL_SPELL(spell_emp_pulse); break;
		case SPELL_QUANTUM_RIFT:
			MANUAL_SPELL(spell_quantum_rift); break;
		case SPELL_DEATH_KNELL:
			MANUAL_SPELL(spell_death_knell); break;
		case SPELL_DISPEL_MAGIC:
			MANUAL_SPELL(spell_dispel_magic); break;
		case SPELL_DISTRACTION:
			MANUAL_SPELL(spell_distraction); break;
		case SPELL_BLESS:
			MANUAL_SPELL(spell_bless); break;
		case SPELL_DAMN:
			MANUAL_SPELL(spell_damn); break;
        case SPELL_CALM:
            MANUAL_SPELL(spell_calm); break;
		case SPELL_CALL_RODENT:
			MANUAL_SPELL(spell_call_rodent); break;
		case SPELL_CALL_BIRD:
			MANUAL_SPELL(spell_call_bird); break;
		case SPELL_CALL_REPTILE:
			MANUAL_SPELL(spell_call_reptile); break;
		case SPELL_CALL_BEAST:
			MANUAL_SPELL(spell_call_beast); break;
		case SPELL_CALL_PREDATOR:
			MANUAL_SPELL(spell_call_predator); break;
        case SPELL_DISPEL_EVIL:
            MANUAL_SPELL(spell_dispel_evil); break;
        case SPELL_DISPEL_GOOD:
            MANUAL_SPELL(spell_dispel_good); break;
		}

	knock_door = NULL;
	return 1;
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.
 *
 * staff  - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * wand   - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * scroll - [0]        level        [1] spell num        [2] spell num        [3] spell num
 * potion - [0] level        [1] spell num        [2] spell num        [3] spell num
 * syringe- [0] level        [1] spell num        [2] spell num        [3] spell num
 * pill   - [0] level        [1] spell num        [2] spell num        [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 *
 * Returns 0 when victim dies, or otherwise an abort is needed.
 */

int
mag_objectmagic(struct Creature *ch, struct obj_data *obj,
	char *argument, int *return_flags)
{
	int i, k, level;
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
	int my_return_flags = 0;
	if (return_flags == NULL)
		return_flags = &my_return_flags;

	one_argument(argument, arg);

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		FIND_OBJ_EQUIP, ch, &tch, &tobj);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_STAFF:
		act("You tap $p three times on the ground.", FALSE, ch, obj, 0,
			TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, 0, TO_ROOM);
		else
			act("$n taps $p three times on the ground.", FALSE, ch, obj, 0,
				TO_ROOM);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act(NOEFFECT, FALSE, ch, obj, 0, TO_ROOM);
		} else {

			if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
				|| invalid_char_class(ch, obj)
				|| (GET_INT(ch) + CHECK_SKILL(ch,
						SKILL_USE_WANDS)) < number(20, 100)) {
				act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_CHAR);
				act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_ROOM);
				GET_OBJ_VAL(obj, 2)--;
				return 1;
			}
			gain_skill_prof(ch, SKILL_USE_WANDS);
			GET_OBJ_VAL(obj, 2)--;
			WAIT_STATE(ch, PULSE_VIOLENCE);

			level =
				(GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_USE_WANDS)) / 100;
			level = MIN(level, LVL_AMBASSADOR);


			room_data *room = ch->in_room;
			CreatureList::iterator it = room->people.begin();
			for (; it != room->people.end(); ++it) {
				if (ch == *it && spell_info[GET_OBJ_VAL(obj, 3)].violent)
					continue;
				if (level)
					call_magic(ch, (*it), NULL, GET_OBJ_VAL(obj, 3),
						level, CAST_STAFF);
				else
					call_magic(ch, (*it), NULL, GET_OBJ_VAL(obj, 3),
						DEFAULT_STAFF_LVL, CAST_STAFF);
			}
		}
		break;
	case ITEM_WAND:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
				if (obj->action_desc != NULL)
					act(obj->action_desc, FALSE, ch, obj, tch, TO_ROOM);
				else {
					act("$n points $p at $N.", TRUE, ch, obj, tch, TO_NOTVICT);
					act("$n points $p at you.", TRUE, ch, obj, tch, TO_VICT);
				}
			}
		} else if (tobj != NULL) {
			act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
			if (obj->action_desc != NULL)
				act(obj->action_desc, FALSE, ch, obj, tobj, TO_ROOM);
			else
				act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
		} else {
			act("At what should $p be pointed?", FALSE, ch, obj, NULL,
				TO_CHAR);
			return 1;
		}

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act(NOEFFECT, FALSE, ch, obj, 0, TO_ROOM);
			return 1;
		}

		if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			|| invalid_char_class(ch, obj)
			|| (GET_INT(ch) + CHECK_SKILL(ch, SKILL_USE_WANDS)) < number(20,
				100)) {
			act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_CHAR);
			act("$p flashes and shakes for a moment, but nothing else happens.", FALSE, ch, obj, 0, TO_ROOM);
			GET_OBJ_VAL(obj, 2)--;
			return 1;
		}

		level = (GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_USE_WANDS)) / 100;
		level = MIN(level, LVL_AMBASSADOR);

		gain_skill_prof(ch, SKILL_USE_WANDS);
		GET_OBJ_VAL(obj, 2)--;
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
				level ? level : DEFAULT_WAND_LVL, CAST_WAND));
		break;
	case ITEM_SCROLL:
		if (*arg) {
			if (!k) {
				act("There is nothing to here to affect with $p.", FALSE,
					ch, obj, NULL, TO_CHAR);
				return 1;
			}
		} else
			tch = ch;

		act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else if (tch && tch != ch) {
			act("$n recites $p in your direction.", FALSE, ch, obj, tch,
				TO_VICT);
			act("$n recites $p in $N's direction.", FALSE, ch, obj, tch,
				TO_NOTVICT);
		} else if (tobj) {
			act("$n looks at $P and recites $p.", FALSE, ch, obj, tobj,
				TO_ROOM);
		} else
			act("$n recites $p.", FALSE, ch, obj, 0, TO_ROOM);

		if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			|| invalid_char_class(ch, obj)
			|| (GET_INT(ch) + CHECK_SKILL(ch, SKILL_READ_SCROLLS)) < number(20,
				100)) {
			act("$p flashes and smokes for a moment, then is gone.", FALSE, ch,
				obj, 0, TO_CHAR);
			act("$p flashes and smokes for a moment before dissolving.", FALSE,
				ch, obj, 0, TO_ROOM);
			if (obj)
				extract_obj(obj);
			return 1;
		}

		level =
			(GET_OBJ_VAL(obj, 0) * CHECK_SKILL(ch, SKILL_READ_SCROLLS)) / 100;
		level = MIN(level, LVL_AMBASSADOR);

		WAIT_STATE(ch, PULSE_VIOLENCE);
		gain_skill_prof(ch, SKILL_READ_SCROLLS);
		for (i = 1; i < 4; i++) {
			call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
				level, CAST_SCROLL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
		}
		extract_obj(obj);
		break;
	case ITEM_FOOD:
		if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
			act("You feel a strange sensation as you eat $p...", FALSE,
				ch, obj, NULL, TO_CHAR);
			return (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, 2),
					GET_OBJ_VAL(obj, 1), CAST_POTION));
		}
		break;
	case ITEM_POTION:
		tch = ch;
		act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
			for (i = 1; i < 4; i++) {
				if (!ch->in_room)
					break;
				call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
					GET_OBJ_VAL(obj, 0), CAST_POTION, &my_return_flags);
				if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
					IS_SET(my_return_flags, DAM_VICT_KILLED)) {
					*return_flags = my_return_flags;
					break;
				}
			}
		}
		extract_obj(obj);
		break;
	case ITEM_SYRINGE:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				act("You jab $p into your arm and press the plunger.",
					FALSE, ch, obj, 0, TO_CHAR);
				act("$n jabs $p into $s arm and depresses the plunger.",
					FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You jab $p into $N's arm and press the plunger.",
					FALSE, ch, obj, tch, TO_CHAR);
				act("$n jabs $p into $N's arm and presses the plunger.",
					TRUE, ch, obj, tch, TO_NOTVICT);
				act("$n jabs $p into your arm and presses the plunger.",
					TRUE, ch, obj, tch, TO_VICT);
			}
		} else if (tobj) {
			slog("SYSERR: tobj passed to SYRINGE in mag_objectmagic.");
		} else {
			act("Who do you want to inject with $p?", FALSE, ch, obj, NULL,
				TO_CHAR);
			return 1;
		}

		if (obj->action_desc != NULL)
			act(obj->action_desc, FALSE, ch, obj, tch, TO_VICT);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		for (i = 1; i < 4; i++) {
			call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, i),
				GET_OBJ_VAL(obj, 0),
				IS_OBJ_STAT(obj, ITEM_MAGIC) ? CAST_SPELL :
				CAST_INTERNAL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
		}
		extract_obj(obj);
		break;

	case ITEM_PILL:
		tch = ch;
		act("You swallow $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_desc)
			act(obj->action_desc, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n swallows $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);

		for (i = 1; i < 4; i++) {
			call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
				GET_OBJ_VAL(obj, 0), CAST_INTERNAL, &my_return_flags);
			if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED) ||
				IS_SET(my_return_flags, DAM_VICT_KILLED)) {
				*return_flags = my_return_flags;
				break;
			}
		}
		extract_obj(obj);
		break;

	default:
		slog("SYSERR: Unknown object_type in mag_objectmagic");
		break;
	}
	return 1;
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

int
cast_spell(struct Creature *ch, struct Creature *tch,
	struct obj_data *tobj, int spellnum, int *return_flags)
{

	if (return_flags)
		*return_flags = 0;

	//
	// subtract npc mana first, since this is the entry point for npc spells
	// do_cast, do_trigger, do_alter are the entrypoints for PC spells
	//

	if (IS_NPC(ch)) {
		if (GET_MANA(ch) < mag_manacost(ch, spellnum))
			return 0;
		else {
			GET_MANA(ch) -= mag_manacost(ch, spellnum);
			if (!ch->desc) {
				WAIT_STATE( ch, (3 RL_SEC) ); //PULSE_VIOLENCE);
			}
		}
	}
	//
	// verify correct position
	//

	if (ch->getPosition() < SINFO.min_position) {
		switch (ch->getPosition()) {
		case POS_SLEEPING:
			if (SPELL_IS_PHYSICS(spellnum))
				send_to_char(ch, "You dream about great physical powers.\r\n");
			if (SPELL_IS_PSIONIC(spellnum))
				send_to_char(ch, "You dream about great psionic powers.\r\n");
			else
				send_to_char(ch, "You dream about great magical powers.\r\n");
			break;
		case POS_RESTING:
			send_to_char(ch, "You cannot concentrate while resting.\r\n");
			break;
		case POS_SITTING:
			send_to_char(ch, "You can't do this sitting!\r\n");
			break;
		case POS_FIGHTING:
			send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
			if (IS_MOB(ch)) {
				slog("SYSERR: %s tried to cast spell %d in battle.",
					GET_NAME(ch), spellnum);
			}
			break;
		default:
			send_to_char(ch, "You can't do much of anything like this!\r\n");
			break;
		}
		return 0;
	}
	if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tch)) {
		send_to_char(ch, "You are afraid you might hurt your master!\r\n");
		return 0;
	}
	if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY) &&
		GET_LEVEL(ch) < LVL_CREATOR) {
		send_to_char(ch, "You can only %s yourself!\r\n",
			SPELL_IS_PHYSICS(spellnum) ? "alter this reality on" :
			SPELL_IS_PSIONIC(spellnum) ? "trigger this psi on" :
			SPELL_IS_MERCENARY(spellnum) ? "apply this device to" :
			"cast this spell upon");
		return 0;
	}
	if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
		send_to_char(ch, "You cannot %s yourself!\r\n",
			SPELL_IS_PHYSICS(spellnum) ? "alter this reality on" :
			SPELL_IS_PSIONIC(spellnum) ? "trigger this psi on" :
			SPELL_IS_MERCENARY(spellnum) ? "apply this device to" :
			"cast this spell upon");
		return 0;
	}
	if (IS_CONFUSED(ch) &&
		CHECK_SKILL(ch, spellnum) + GET_INT(ch) < number(90, 180)) {
		send_to_char(ch, "You are too confused to %s\r\n",
			SPELL_IS_PHYSICS(spellnum) ? "alter any reality!" :
			SPELL_IS_PSIONIC(spellnum) ? "trigger any psi!" :
			SPELL_IS_MERCENARY(spellnum) ? "construct any devices!" :
			"cast any spells!");
		return 0;
	}
	if (IS_SET(SINFO.routines, MAG_GROUPS) && !IS_AFFECTED(ch, AFF_GROUP)) {
		send_to_char(ch, "You can't do this if you're not in a group!\r\n");
		return 0;
	}
	if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER &&
		SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
		send_to_char(ch, "This spell does not function underwater.\r\n");
		return 0;
	}
	if (!OUTSIDE(ch) && SPELL_FLAGGED(spellnum, MAG_OUTDOORS)) {
		send_to_char(ch, "This spell can only be cast outdoors.\r\n");
		return 0;
	}
	if (SPELL_FLAGGED(spellnum, MAG_NOSUN) && OUTSIDE(ch)
		&& !room_is_dark(ch->in_room)) {
		send_to_char(ch, "This spell cannot be cast in sunlight.\r\n");
		return 0;
	}
	// Evil Knight remort skill. 'Taint'
	if (ch != NULL && GET_LEVEL(ch) < LVL_AMBASSADOR
		&& AFF3_FLAGGED(ch, AFF3_TAINTED)) {
		struct affected_type *af;
		int mana = mag_manacost(ch, spellnum);
		int dam = dice(mana, 14);
		bool weenie = false;

		// Grab the affect for affect level
		af = affected_by_spell(ch, SPELL_TAINT);

		// Max dam based on level and gen of caster. (getLevelBonus(taint))
		if (af != NULL)
			dam = dam * af->level / 100;

		if (damage(ch, ch, dam, TYPE_TAINT_BURN, WEAR_HEAD)) {
			return 0;
		}
		// fucking weaklings can't cast while tainted.
		int attribute = MIN(GET_CON(ch), GET_WIS(ch));
		if (number(1, attribute) < number(mana / 2, mana)) {
			weenie = true;
		}
		if (ch && PRF2_FLAGGED(ch, PRF2_DEBUG))
			send_to_char(ch,
				"%s[TAINT] %s attribute:%d   weenie:%s   mana:%d   damage:%d%s\r\n",
				CCCYN(ch, C_NRM), GET_NAME(ch), attribute,
				weenie ? "t" : "f", mana, dam, CCNRM(ch, C_NRM));
		if (tch && PRF2_FLAGGED(tch, PRF2_DEBUG))
			send_to_char(tch,
				"%s[TAINT] %s attribute:%d   weenie:%s   mana:%d   damage:%d%s\r\n",
				CCCYN(tch, C_NRM), GET_NAME(ch), attribute,
				weenie ? "t" : "f", mana, dam, CCNRM(tch, C_NRM));

		if (af != NULL) {
			af->duration -= mana;
			af->duration = MAX(af->duration, 0);
			if (af->duration == 0) {
				affect_remove(ch, af);
				act("Blood pours out of $n's forehead as the rune of taint dissolves.", TRUE, ch, 0, 0, TO_ROOM);
			}
		}
		if (weenie == true) {
			act("$n screams and clutches at the rune in $s forehead.", TRUE,
				ch, 0, 0, TO_ROOM);
			send_to_char(ch, "Your concentration fails.\r\n");
			return 0;
		}
	}

	say_spell(ch, spellnum, tch, tobj);

	if (!IS_MOB(ch) && GET_LEVEL(ch) >= LVL_AMBASSADOR &&
		GET_LEVEL(ch) < LVL_GOD && !mini_mud &&
		(!tch || GET_LEVEL(tch) < LVL_AMBASSADOR) && (ch != tch)) {
		slog("ImmCast: %s called %s on %s.", GET_NAME(ch),
			spell_to_str(spellnum), tch ? GET_NAME(tch) : tobj ?
			tobj->name : knock_door ?
			(knock_door->keyword ? fname(knock_door->keyword) :
				"door") : "NULL");
	}

	int my_return_flags = 0;

	int retval = call_magic(ch, tch, tobj, spellnum,
		GET_LEVEL(ch) + (!IS_NPC(ch) ? (GET_REMORT_GEN(ch) << 1) : 0),
		(SPELL_IS_PSIONIC(spellnum) ? CAST_PSIONIC :
			(SPELL_IS_PHYSICS(spellnum) ? CAST_PHYSIC : CAST_SPELL)),
		&my_return_flags);

	if (return_flags) {
		*return_flags = my_return_flags;
	}

	if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
		return 0;
	}

	if (IS_SET(my_return_flags, DAM_VICT_KILLED)) {
		return retval;
	}

	if (tch && ch != tch && IS_NPC(tch) &&
		ch->in_room == tch->in_room &&
		SINFO.violent && !FIGHTING(tch) && tch->getPosition() > POS_SLEEPING &&
		(!AFF_FLAGGED(tch, AFF_CHARM) || ch != tch->master)) {
		int my_return_flags = hit(tch, ch, TYPE_UNDEFINED);

		my_return_flags = SWAP_DAM_RETVAL(my_return_flags);

		if (return_flags) {
			*return_flags = my_return_flags;
		}

		if (IS_SET(my_return_flags, DAM_ATTACKER_KILLED)) {
			return 0;
		}
	}

	return (retval);
}

int
find_spell_targets(struct Creature *ch, char *argument,
	struct Creature **tch, struct obj_data **tobj, int *target, int *spellnm,
	int cmd)
{

	char *s, *t;
	char t3[MAX_INPUT_LENGTH], t2[MAX_INPUT_LENGTH];
	int i, spellnum;

	knock_door = NULL;
	/* get: blank, spell name, target name */
	s = strtok(argument, "'");

	if (s == NULL) {
		sprintf(buf, "%s what where?", cmd_info[cmd].command);
		act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		return 0;
	}
	s = strtok(NULL, "'");
	if (s == NULL) {
		if (CMD_IS("alter"))
			send_to_char(ch, 
				"The alteration name must be enclosed in the symbols: '\r\n");
		else if (CMD_IS("trigger"))
			send_to_char(ch, 
				"The psitrigger name must be enclosed in the symbols: '\r\n");
		else if (CMD_IS("arm"))
			send_to_char(ch, 
				"The device name must be enclosed in the symbols: '\r\n");
		else
			send_to_char(ch, 
				"Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
		return 0;
	}
	t = strtok(NULL, "\0");

	spellnum = find_skill_num(s);
	*spellnm = spellnum;

	if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
		sprintf(buf, "%s what?!?", cmd_info[cmd].command);
		act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		return 0;
	}
	if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)] &&
		(!IS_REMORT(ch) ||
			GET_LEVEL(ch) < SINFO.min_level[(int)GET_REMORT_CLASS(ch)]) &&
		CHECK_SKILL(ch, spellnum) < 30) {
		send_to_char(ch, "You do not know that %s!\r\n",
			SPELL_IS_PSIONIC(spellnum) ? "trigger" :
			SPELL_IS_PHYSICS(spellnum) ? "alteration" :
			SPELL_IS_MERCENARY(spellnum) ? "device" : "spell");
		return 0;
	}
	if (CHECK_SKILL(ch, spellnum) == 0) {
		send_to_char(ch, "You are unfamiliar with that %s.\r\n",
			SPELL_IS_PSIONIC(spellnum) ? "trigger" :
			SPELL_IS_PHYSICS(spellnum) ? "alteration" :
			SPELL_IS_MERCENARY(spellnum) ? "device" : "spell");
		return 0;
	}
	/* Find the target */

	if (t != NULL) {
		// DL - moved this here so we can handle multiple locate arguments
		strncpy(locate_buf, t, 255);
		locate_buf[255] = '\0';
		one_argument(strcpy(arg, t), t);
		skip_spaces(&t);
	}
	if (IS_SET(SINFO.targets, TAR_IGNORE)) {
		*target = TRUE;
	} else if (t != NULL && *t) {
		if (!*target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
			if ((*tch = get_char_room_vis(ch, t)) != NULL)
				*target = TRUE;
		}
		if (!*target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
			if ((*tch = get_char_vis(ch, t)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
			for (i = 0; !*target && i < NUM_WEARS; i++)
				if (GET_EQ(ch, i) && !str_cmp(t, GET_EQ(ch, i)->aliases)) {
					*tobj = GET_EQ(ch, i);
					*target = TRUE;
				}
		}
		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->in_room->contents)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
			if ((*tobj = get_obj_vis(ch, t)))
				*target = TRUE;

		if (!*target && IS_SET(SINFO.targets, TAR_DOOR)) {
			half_chop(arg, t2, t3);
			if ((i = find_door(ch, t2, t3,
						spellnum == SPELL_KNOCK ? "knock" : "cast")) >= 0) {
				knock_door = ch->in_room->dir_option[i];
				*target = TRUE;
			} else {
				return 0;
			}
		}


	} else {					/* if target string is empty */
		if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
			if (FIGHTING(ch) != NULL) {
				*tch = ch;
				*target = TRUE;
			}
		if (!*target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
			if (FIGHTING(ch) != NULL) {
				*tch = FIGHTING(ch);
				*target = TRUE;
			}
		/* if no target specified, 
		   and the spell isn't violent, i
		   default to self */
		if (!*target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
			!SINFO.violent && !IS_SET(SINFO.targets, TAR_UNPLEASANT)) {
			*tch = ch;
			*target = TRUE;
		}

		if (!*target) {
			send_to_char(ch, "Upon %s should the spell be cast?\r\n",
				IS_SET(SINFO.targets,
					TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ? "what" :
				"who");
			return 0;
		}
	}
	return 1;
}


ACMD(do_cast)
{
	Creature *tch = NULL;
	obj_data *tobj = NULL, *holy_symbol = NULL, *metal = NULL;

	int mana, spellnum, i, target = 0, prob = 0, metal_wt = 0, num_eq =
		0, temp = 0;

	if (IS_NPC(ch))
		return;

	if (!IS_MAGE(ch) && !IS_CLERIC(ch) && !IS_KNIGHT(ch) && !IS_RANGER(ch)
		&& !IS_VAMPIRE(ch) && GET_CLASS(ch) < NUM_CLASSES
		&& (GET_LEVEL(ch) < LVL_GRGOD)) {
		send_to_char(ch, "You are not learned in the ways of magic.\r\n");
		return;
	}
	if (GET_LEVEL(ch) < LVL_AMBASSADOR &&
		IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.90)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky to cast anything useful!\r\n");
		return;
	}
	if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_WIELD) &&
		IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
		send_to_char(ch, 
			"You can't cast spells while wielding a two handed weapon!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &target, &spellnum,
				cmd)))
		return;

	// Drunk bastards don't cast very well, do they... -- Nothing 1/22/2001 
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, "Your mind is too clouded to cast any spells!\r\n");
			return;
		} else {
			send_to_char(ch, "You feel your concentration slipping!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_MAGIC(spellnum) && !SPELL_IS_DIVINE(spellnum)) {
		act("That is not a spell.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target && !IS_SET(SINFO.targets, TAR_DOOR)) {
		send_to_char(ch, "Cannot find the target of your spell!\r\n");
		return;
	}

	mana = mag_manacost(ch, spellnum);

	if ((mana > 0) && (GET_MANA(ch) < mana) && GET_LEVEL(ch) < LVL_AMBASSADOR
		&& !PLR_FLAGGED(ch, PLR_MORTALIZED)) {
		send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
		return;
	}

	if (GET_LEVEL(ch) < LVL_IMMORT && (!IS_EVIL(ch) && SPELL_IS_EVIL(spellnum))
		|| (!IS_GOOD(ch) && SPELL_IS_GOOD(spellnum))) {
		send_to_char(ch, "You cannot cast that spell.\r\n");
		return;
	}

	if (GET_LEVEL(ch) > 3 && GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_NPC(ch) &&
		(IS_CLERIC(ch) || IS_KNIGHT(ch)) && SPELL_IS_DIVINE(spellnum)) {
		bool need_symbol = true;
		int gen = GET_REMORT_GEN(ch);
		if (IS_SOULLESS(ch)) {
			if (GET_CLASS(ch) == CLASS_CLERIC && gen > 4)
				need_symbol = false;
			else if (GET_CLASS(ch) == CLASS_KNIGHT && gen > 6)
				need_symbol = false;
		}
		if (need_symbol) {
			for (i = 0; i < NUM_WEARS; i++) {
				if (ch->equipment[i]) {
					if (GET_OBJ_TYPE(ch->equipment[i]) == ITEM_HOLY_SYMB) {
						holy_symbol = ch->equipment[i];
						break;
					}
				}
			}
			if (!holy_symbol) {
				send_to_char(ch, 
					"You do not even wear the symbol of your faith!\r\n");
				return;
			}
			if ((IS_GOOD(ch) && (GET_OBJ_VAL(holy_symbol, 0) == 2)) ||
				(IS_EVIL(ch) && (GET_OBJ_VAL(holy_symbol, 0) == 0))) {
				send_to_char(ch, "You are not aligned with your holy symbol!\r\n");
				return;
			}
			if (GET_CLASS(ch) != GET_OBJ_VAL(holy_symbol, 1) &&
				GET_REMORT_CLASS(ch) != GET_OBJ_VAL(holy_symbol, 1)) {
				send_to_char(ch, 
					"The holy symbol you wear is not of your faith!\r\n");
				return;
			}
			if (GET_LEVEL(ch) < GET_OBJ_VAL(holy_symbol, 2)) {
				send_to_char(ch, 
					"You are not powerful enough to utilize your holy symbol!\r\n");
				return;
			}
			if (GET_LEVEL(ch) > GET_OBJ_VAL(holy_symbol, 3)) {
				act("$p will no longer support your power!",
					FALSE, ch, holy_symbol, 0, TO_CHAR);
				return;
			}
		}
	}

	/***** casting probability calculation *****/
	if ((IS_CLERIC(ch) || IS_KNIGHT(ch)) && SPELL_IS_DIVINE(spellnum)) {
		prob -= (GET_WIS(ch) + (abs(GET_ALIGNMENT(ch) / 70)));
		if (IS_NEUTRAL(ch))
			prob += 30;
	}
	if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
		&& SPELL_IS_MAGIC(spellnum))
		prob -= (GET_INT(ch) + GET_DEX(ch));

	if (IS_SICK(ch))
		prob += 20;

	if (IS_CONFUSED(ch))
		prob += number(35, 55) - GET_INT(ch);

	if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
		prob += GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob += ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	for (i = 0, num_eq = 0, metal_wt = 0; i < NUM_WEARS; i++) {
		if (ch->equipment[i]) {
			num_eq++;
			if (i != WEAR_WIELD && GET_OBJ_TYPE(ch->equipment[i]) == ITEM_ARMOR
				&& IS_METAL_TYPE(ch->equipment[i])) {
				metal_wt += ch->equipment[i]->getWeight();
				if (!metal || !number(0, 8) ||
					(ch->equipment[i]->getWeight() > metal->getWeight() &&
						!number(0, 1)))
					metal = ch->equipment[i];
			}
			if (ch->implants[i]) {
				if (IS_METAL_TYPE(ch->equipment[i])) {
					metal_wt += ch->equipment[i]->getWeight();
					if (!metal || !number(0, 8) ||
						(ch->implants[i]->getWeight() > metal->getWeight() &&
							!number(0, 1)))
						metal = ch->implants[i];
				}
			}
		}
	}

	if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
		&& SPELL_IS_MAGIC(spellnum)
		&& GET_LEVEL(ch) < LVL_ETERNAL)
		prob += metal_wt;

	prob -= (NUM_WEARS - num_eq);

	if (FIGHTING(ch) && (FIGHTING(ch))->getPosition() == POS_FIGHTING)
		prob += (GET_LEVEL(FIGHTING(ch)) >> 3);

	/**** casting probability ends here *****/

	/* Begin fucking them over for failing and describing said fuckover */

	if ((prob + number(0, 75)) > CHECK_SKILL(ch, spellnum)) {
		/* there is a failure */

		WAIT_STATE(ch, ((3 RL_SEC) - GET_REMORT_GEN(ch)));

		if (tch && tch != ch) {
			/* victim exists */

			if ((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent) &&
				!IS_SET(SINFO.routines, MAG_TOUCH) &&
				ok_to_damage(ch, tch) &&
				(prob + number(0, 111)) > CHECK_SKILL(ch, spellnum)) {
				/* misdirect */


				CreatureList::iterator it = ch->in_room->people.begin();
				CreatureList::iterator nit = ch->in_room->people.begin();
				for (; it != ch->in_room->people.end(); ++it) {
					++nit;
					if ((*it) != ch && (*it) != tch &&
						GET_LEVEL((*it)) < LVL_AMBASSADOR &&
						ok_to_damage(ch, (*it)) && (!number(0, 4)
							|| nit != ch->in_room->people.end())) {
						if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch))
							&& metal && SPELL_IS_MAGIC(spellnum)
							&& metal_wt > number(5, 80))
							act("$p has misdirected your spell toward $N!!",
								FALSE, ch, metal, (*it), TO_CHAR);
						else
							act("Your spell has been misdirected toward $N!!",
								FALSE, ch, 0, (*it), TO_CHAR);
						cast_spell(ch, (*it), tobj, spellnum);
						if (mana > 0)
							GET_MANA(ch) =
								MAX(0, MIN(GET_MAX_MANA(ch),
									GET_MANA(ch) - (mana >> 1)));
						WAIT_STATE(ch, 2 RL_SEC);
						return;
					}
				}
			}

			if ((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent) &&
				(prob + number(0, 81)) > CHECK_SKILL(ch, spellnum)) {

				/* backfire */

				if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) && metal
					&& SPELL_IS_MAGIC(spellnum) && metal_wt > number(5, 80))
					act("$p has caused your spell to backfire!!", FALSE, ch,
						metal, 0, TO_CHAR);
				else
					send_to_char(ch, "Your spell has backfired!!\r\n");
				cast_spell(ch, ch, tobj, spellnum);
				if (mana > 0)
					GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) -
							(mana >> 1)));
				WAIT_STATE(ch, 2 RL_SEC);
				return;
			}

			/* plain dud */
			if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) && metal &&
				SPELL_IS_MAGIC(spellnum) && metal_wt > number(5, 100))
				act("$p has interfered with your spell!",
					FALSE, ch, metal, 0, TO_CHAR);
			else
				send_to_char(ch, "You lost your concentration!\r\n");
			ACMD(do_say);
			if (!skill_message(0, ch, tch, spellnum)) {
				send_to_char(ch, NOEFFECT);
			}

			if (((IS_SET(SINFO.routines, MAG_DAMAGE) || SINFO.violent)) &&
				IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_NOHASSLE) &&
				can_see_creature(tch, ch))
				hit(tch, ch, TYPE_UNDEFINED);

		} else if ((IS_MAGE(ch) || IS_RANGER(ch) || IS_VAMPIRE(ch)) &&
			SPELL_IS_MAGIC(spellnum) && metal && metal_wt > number(5, 90))
			act("$p has interfered with your spell!",
				FALSE, ch, metal, 0, TO_CHAR);
		else
			send_to_char(ch, "You lost your concentration!\r\n");

		if (mana > 0)
			GET_MANA(ch) =
				MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
		/* cast spell returns 1 on success; subtract mana & set waitstate */
		//HERE
	} else {
		if (cast_spell(ch, tch, tobj, spellnum)) {
			WAIT_STATE(ch, ((3 RL_SEC) - (GET_CLASS(ch) == CLASS_MAGIC_USER ?
						GET_REMORT_GEN(ch) : 0)));
			if (mana > 0)
				GET_MANA(ch) =
					MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			gain_skill_prof(ch, spellnum);

		}
	}
}

ACMD(do_trigger)
{
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
	int mana, spellnum, target = 0, prob = 0, temp = 0;

	if (IS_NPC(ch))
		return;

	if (!IS_PSYCHIC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able to trigger the mind.\r\n");
		return;
	}
	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky for you to concentrate!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &target, &spellnum,
				cmd)))
		return;

	// Drunk bastards don't trigger very well, do they... -- Nothing 1/22/2001
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, "Your mind is too clouded to make any triggers!\r\n");
			return;
		} else {
			send_to_char(ch, "You feel your concentration slipping!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_PSIONIC(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_PSIONIC(spellnum)) {
		act("That is not a psionic trigger.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (!target) {
		send_to_char(ch, "Cannot find the target of your trigger!\r\n");
		return;
	}
	mana = mag_manacost(ch, spellnum);
	if ((mana > 0) && (GET_MANA(ch) < mana)
		&& (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the energy to make that trigger!\r\n");
		return;
	}

	if (tch && (IS_UNDEAD(tch) || IS_SLIME(tch) || IS_PUDDING(tch) ||
			IS_ROBOT(tch) || IS_PLANT(tch))) {
		act("You cannot make a mindlink with $N!", FALSE, ch, 0, tch, TO_CHAR);
		return;
	}

	if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER &&
		SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
		send_to_char(ch, "This trigger will not function underwater.\r\n");
		return;
	}

	/***** trigger probability calculation *****/
	prob = CHECK_SKILL(ch, spellnum) + (GET_INT(ch) << 1) +
		(GET_REMORT_GEN(ch) << 2);
	if (IS_SICK(ch))
		prob -= 20;
	if (IS_CONFUSED(ch))
		prob -= number(35, 55) + GET_INT(ch);

	if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
		prob -= GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	if (FIGHTING(ch) && (FIGHTING(ch))->getPosition() == POS_FIGHTING)
		prob -= (GET_LEVEL(FIGHTING(ch)) >> 3);

	/**** casting probability ends here *****/

	/* You throws the dice and you takes your chances.. 101% is total failure */
	if (number(0, 111) > prob) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || spellnum == SPELL_ELECTROSTATIC_FIELD
			|| !skill_message(0, ch, tch, spellnum))
			send_to_char(ch, "Your concentration was disturbed!\r\n");
		if (mana > 0)
			GET_MANA(ch) =
				MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} else {
		if (cast_spell(ch, tch, tobj, spellnum)) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (mana > 0)
				GET_MANA(ch) =
					MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			gain_skill_prof(ch, spellnum);
		}
	}
}

ACMD(do_arm)
{
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
	int mana, spellnum, target = 0, prob = 0;

	if (IS_NPC(ch))
		return;

	if (!IS_MERC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able arm devices!\r\n");
		return;
	}

	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky to arm any devices!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &target, &spellnum,
				cmd)))
		return;

	if (!SPELL_IS_MERCENARY(spellnum)) {
		act("That is not a device.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target) {
		send_to_char(ch, "Cannot find the target of your device!\r\n");
		return;
	}

	mana = mag_manacost(ch, spellnum);
	if ((mana > 0) && (GET_MANA(ch) < mana)
		&& (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the energy to construct that device!\r\n");
		return;
	}

	if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER &&
		SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
		send_to_char(ch, "This device will not function underwater.\r\n");
		return;
	}

	/***** arm device probobility calculation *****/
	prob = CHECK_SKILL(ch, spellnum) + (GET_INT(ch) << 1) +
		(GET_REMORT_GEN(ch) << 2);

	if (IS_SICK(ch))
		prob -= 20;

	if (IS_CONFUSED(ch))
		prob -= number(35, 55) + GET_INT(ch);

	if (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_EQ(ch, WEAR_SHIELD))
		prob -= GET_EQ(ch, WEAR_SHIELD)->getWeight();

	prob -= ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) << 3) / CAN_CARRY_W(ch);

	if (FIGHTING(ch) && (FIGHTING(ch))->getPosition() == POS_FIGHTING)
		prob -= (GET_LEVEL(FIGHTING(ch)) >> 3);

	/**** arm device probability ends here *****/

	if (number(0, 111) > prob) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, spellnum))
			send_to_char(ch, "Your concentration was disturbed!\r\n");

		if (mana > 0)
			GET_MANA(ch) =
				MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));

		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} else {
		if (cast_spell(ch, tch, tobj, spellnum)) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (mana > 0)
				GET_MANA(ch) =
					MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			gain_skill_prof(ch, spellnum);
		}
	}
}



ACMD(do_alter)
{
	struct Creature *tch = NULL;
	struct obj_data *tobj = NULL;
	int mana, spellnum, target = 0, temp = 0;

	if (IS_NPC(ch))
		return;

	if (GET_CLASS(ch) != CLASS_PHYSIC &&
		GET_REMORT_CLASS(ch) != CLASS_PHYSIC
		&& GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "You are not able to alter the fabric of reality.\r\n");
		return;
	}
	if (IS_WEARING_W(ch) > (CAN_CARRY_W(ch) * 0.80)) {
		send_to_char(ch, 
			"Your equipment is too heavy and bulky to alter reality!\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) &&
		IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
		send_to_char(ch, "You can't alter while wielding a two handed weapon!\r\n");
		return;
	}

	if (!(find_spell_targets(ch, argument, &tch, &tobj, &target, &spellnum,
				cmd)))
		return;

	// Drunk bastards don't cast very well, do they... -- Nothing 1/22/2001
	if ((GET_COND(ch, DRUNK) > 5) && (temp = number(1, 35)) > GET_INT(ch)) {
		if (temp < 34) {
			send_to_char(ch, 
				"Your mind is too clouded to alter the fabric of reality!\r\n");
			return;
		} else {
			send_to_char(ch, "You feel your concentration slipping!\r\n");
			WAIT_STATE(ch, 2 RL_SEC);
			spellnum = number(1, MAX_SPELLS);
			if (!SPELL_IS_PHYSICS(spellnum)) {
				send_to_char(ch, "Your concentration slips away entirely.\r\n");
				return;
			}
		}
	}

	if (!SPELL_IS_PHYSICS(spellnum)) {
		act("That is not a physical alteration.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!target) {
		send_to_char(ch, "Cannot find the target of your alteration!\r\n");
		return;
	}
	mana = mag_manacost(ch, spellnum);
	if ((mana > 0) && (GET_MANA(ch) < mana)
		&& (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
		send_to_char(ch, "You haven't the energy to make that alteration!\r\n");
		return;
	}

	if (SECT_TYPE(ch->in_room) == SECT_UNDERWATER &&
		SPELL_FLAGGED(spellnum, MAG_NOWATER)) {
		send_to_char(ch, "This alteration will not function underwater.\r\n");
		return;
	}
	if (!OUTSIDE(ch) && SPELL_FLAGGED(spellnum, MAG_OUTDOORS)) {
		send_to_char(ch, "This alteration can only be made outdoors.\r\n");
		return;
	}

	/* You throws the dice and you takes your chances.. 101% is total failure */
	if (number(0, 101) > GET_SKILL(ch, spellnum)) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, spellnum))
			send_to_char(ch, "Your concentration was disturbed!\r\n");
		if (mana > 0)
			GET_MANA(ch) =
				MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} else {
		if (cast_spell(ch, tch, tobj, spellnum)) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (mana > 0)
				GET_MANA(ch) =
					MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			gain_skill_prof(ch, spellnum);
		}
	}
}


/* Assign the spells on boot up */

void
spello(int spl, int mlev, int clev, int tlev, int wlev, int blev,
	int slev, int plev, int ylev, int klev, int rlev, int hlev,
	int alev, int vlev, int merclev, int s1lev, int s2lev, int s3lev,
	sh_int max_mana, sh_int min_mana, int mana_change,
	int minpos, int targets, int violent, int routines)
{
	int i;
	for (i = 0; i < NUM_CLASSES; i++)
		spell_info[spl].gen[i] = 0;
	spell_info[spl].min_level[CLASS_MAGIC_USER] = mlev;
	spell_info[spl].min_level[CLASS_CLERIC] = clev;
	spell_info[spl].min_level[CLASS_THIEF] = tlev;
	spell_info[spl].min_level[CLASS_WARRIOR] = wlev;
	spell_info[spl].min_level[CLASS_BARB] = blev;
	spell_info[spl].min_level[CLASS_PSIONIC] = slev;
	spell_info[spl].min_level[CLASS_PHYSIC] = plev;
	spell_info[spl].min_level[CLASS_CYBORG] = ylev;
	spell_info[spl].min_level[CLASS_KNIGHT] = klev;
	spell_info[spl].min_level[CLASS_RANGER] = rlev;
	spell_info[spl].min_level[CLASS_HOOD] = hlev;
	spell_info[spl].min_level[CLASS_MONK] = alev;
	spell_info[spl].min_level[CLASS_VAMPIRE] = vlev;
	spell_info[spl].min_level[CLASS_MERCENARY] = merclev;
	spell_info[spl].min_level[CLASS_SPARE1] = s1lev;
	spell_info[spl].min_level[CLASS_SPARE2] = s2lev;
	spell_info[spl].min_level[CLASS_SPARE3] = s3lev;
	spell_info[spl].mana_max = max_mana;
	spell_info[spl].mana_min = min_mana;
	spell_info[spl].mana_change = mana_change;
	spell_info[spl].min_position = minpos;
	spell_info[spl].targets = targets;
	spell_info[spl].violent = violent;
	spell_info[spl].routines = routines;
}

void
remort_spello(int spl, int char_class, int lev, int gen,
	sh_int max_mana, sh_int min_mana, int mana_change,
	int minpos, int targets, int violent, int routines)
{
	int i;

	for (i = 0; i < NUM_CLASSES; i++) {
		spell_info[spl].min_level[i] = LVL_AMBASSADOR;
		spell_info[spl].gen[i] = 0;
	}
	spell_info[spl].min_level[char_class] = lev;
	spell_info[spl].gen[char_class] = gen;
	spell_info[spl].mana_max = max_mana;
	spell_info[spl].mana_min = min_mana;
	spell_info[spl].mana_change = mana_change;
	spell_info[spl].min_position = minpos;
	spell_info[spl].targets = targets;
	spell_info[spl].violent = violent;
	spell_info[spl].routines = routines;
}

void
spellgen(int spl, int char_class, int lev, int gen)
{
	spell_info[spl].gen[char_class] = (byte) gen;
	spell_info[spl].min_level[char_class] = (byte) lev;

}

/*
 * Arguments for spello calls:
 *
 * spellnum, levels (MCTW), maxmana, minmana, manachng, minpos, targets,
 * violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL). levels  :  Minimum level (mage, cleric,
 * thief, warrior) a player must be to cast this spell.  Use 'X' for immortal
 * only. maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell). minmana :  The minimum
 * mana this spell will take, no matter how high level the caster is.
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|'). violent :  TRUE or FALSE,
 * depending on if this is considered a violent spell and should not be cast
 * in PEACEFUL rooms or on yourself. routines:  A list of magic routines
 * which are associated with this spell. Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

#define UU (LVL_GRIMP+1)
#define UNUSED UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,0,0,0,0,0,0,0

#define X LVL_AMBASSADOR
#define Y LVL_GOD
#define Z LVL_CREATOR

void
mag_assign_spells(void)
{
	int i;


	for (i = 1; i <= TOP_SPELL_DEFINE; i++)
		spello(i, UNUSED);
	/* C L A S S E S      M A N A   */
	/*                    M C T W B S P Y K R H N V Mr S1 S2 S3 */
	spello(SPELL_AIR_WALK, X, 32, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_NOWATER);

	spello(SPELL_ARMOR, 4, 1, X, X, X, X, X, X, 5, X, X, X, X, X, X, X, X,
		45, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_ASTRAL_SPELL, 43, 47, X, X, X, 45, X, X, X, X, X, X, X, X, X,
		X, X, 175, 105, 8, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_DIVINE | MAG_PSIONIC | MAG_MANUAL);

	spello(SPELL_CONTROL_UNDEAD, X, 26, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 50, 20, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_MAGIC | MAG_MANUAL | MAG_EVIL | MAG_DIVINE);

	spello(SPELL_TELEPORT, 33, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		75, 50, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV,
		FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_LOCAL_TELEPORT, 18, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 45, 30, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_BLUR, 13, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		45, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_BLESS, X, 7, X, X, X, X, X, X, 10, X, X, X, X, X, X, X, X,
		35, 15, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE,
		MAG_DIVINE | MAG_MANUAL | MAG_GOOD);

	spello(SPELL_DAMN, X, 7, X, X, X, X, X, X, 10, X, X, X, X, X, X, X, X,
		35, 15, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, true,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL);
    
	spello(SPELL_CALM, X, X, X, X, X, X, X, X, 36, X, X, X, X, X, X, X, X,
		35, 15, 1, POS_STANDING, TAR_CHAR_ROOM, false, MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_BLINDNESS, 16, 13, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 35, 25, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_BREATHE_WATER, 40, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 45, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_BURNING_HANDS, 11, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_TOUCH | MAG_DAMAGE | MAG_NOWATER);

	spello(SPELL_CALL_LIGHTNING, X, 25, X, X, X, X, X, X, X, 30, X, X, X, X, X,
		X, X, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE | MAG_NOWATER | MAG_OUTDOORS);

	spello(SPELL_CHARM, 22, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CHARM_ANIMAL, X, X, X, X, X, X, X, X, X, 23, X, X, X, X, X, X,
		X, 75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_CHILL_TOUCH, 5, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_TOUCH | MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_CLAIRVOYANCE, 35, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_WORLD, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CALL_RODENT, X, X, X, X, X, X, X, X, X, 15, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CALL_BIRD, X, X, X, X, X, X, X, X, X, 22, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CALL_REPTILE, X, X, X, X, X, X, X, X, X, 29, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CALL_BEAST, X, X, X, X, X, X, X, X, X, 35, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CALL_PREDATOR, X, X, X, X, X, X, X, X, X, 41, X, X, X, X, X, X,
		X, 90, 60, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Bar Syk Ph Cyb Kni Ran Hood Bnt Max Min Chn */
	spello(SPELL_CLONE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		80, 65, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_COLOR_SPRAY, 18, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 90, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE);

	spello(SPELL_COMMAND, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		50, 35, 5,
		POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE,
		MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_CONE_COLD, 36, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		140, 55, 4,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_MAGIC | MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_CONJURE_ELEMENTAL, 20, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, X, 175, 90, 4, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_CONTROL_WEATHER, X, 22, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 75, 25, 5, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_MAGIC | MAG_MANUAL | MAG_NOWATER | MAG_OUTDOORS);

	spello(SPELL_CREATE_FOOD, X, 5, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 5, 3, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_DIVINE | MAG_CREATIONS);

	spello(SPELL_CREATE_WATER, X, 3, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
		MAG_DIVINE | MAG_OBJECTS);

	/* Ma Cl Th Wa Ba Sy Ph Cy  Kn Rn Hd Bnt Max Min Chn */
	spello(SPELL_CURE_BLIND, X, 6, X, X, X, X, X, X, 26, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_UNAFFECTS);

	spello(SPELL_CURE_CRITIC, X, 10, X, X, X, X, X, X, 14, X, X, X, X, X, X, X,
		X, 40, 10, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_POINTS);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Ba Sy Ph Cy Kn Rn Hd Bnt Max Min Chn */
	spello(SPELL_CURE_LIGHT, X, 2, X, X, X, X, X, X, 6, X, X, X, X, X, X, X, X,
		30, 5, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_DIVINE | MAG_POINTS);

	spello(SPELL_CURSE, 19, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		80, 50, 2,
		POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, TRUE,
		MAG_MAGIC | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_DETECT_ALIGN, 24, 1, X, X, X, X, X, X, 1, 13, X, X, X, X, X,
		X, X, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_DETECT_INVIS, 14, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_DETECT_MAGIC, 7, 14, X, X, X, X, X, X, 20, 27, X, X, X, X, X,
		X, X, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	/* C L A S S E S      M A N A   */
	spello(SPELL_DETECT_POISON, 20, 4, X, X, X, X, X, X, 12, 6, X, X, X, X, X,
		X, X, 15, 5, 1, POS_STANDING,
		TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_DETECT_SCRYING, 44, 44, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 50, 30, 4, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_DIMENSION_DOOR, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 90, 60, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_DISPEL_EVIL, X, 20, X, X, X, X, X, X, X, X, X, X, X, X, X, X, 
        X, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE,
		MAG_DIVINE | MAG_MANUAL | MAG_GOOD);

	spello(SPELL_DISPEL_GOOD, X, 20, X, X, X, X, X, X, X, X, X, X, X, X, X, X, 
        X, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL);

	spello(SPELL_DISPEL_MAGIC, 39, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 90, 55, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
		FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_DISRUPTION, X, 38, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 150, 75, 7, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE);

	spello(SPELL_DISPLACEMENT, 46, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 55, 30, 4, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Bar Syk Ph Cyb Kni Ran Hood Bnt Max Min Chn */
	spello(SPELL_EARTHQUAKE, X, 30, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 40, 25, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_DIVINE | MAG_AREAS);

	spello(SPELL_ENCHANT_ARMOR, 16, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 180, 80, 3, POS_STANDING, TAR_OBJ_INV, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_ENCHANT_WEAPON, 21, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 200, 100, 3, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_ENDURE_COLD, X, 15, X, X, X, X, X, X, 16, 16, X, X, X, X, X,
		X, X, 100, 40, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_ENERGY_DRAIN, 24, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 40, 25, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE | MAG_MANUAL);

	spello(SPELL_FLY, 33, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 2,
		POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_FLAME_STRIKE, X, 33, X, X, X, X, X, X, 35, X, X, X, X, X, X,
		X, X, 60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE | MAG_NOWATER);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Bar Syk Ph Cyb Kni Ran Hood Bnt Max Min Chn */
	spello(SPELL_GOODBERRY, X, X, X, X, X, X, X, X, X, 7, X, X, X, X, X, X, X,
		30, 5, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_MAGIC | MAG_CREATIONS);

	spello(SPELL_GUST_OF_WIND, 40, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 80, 55, 4, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM, FALSE,
		MAG_MAGIC | MAG_MANUAL | MAG_NOWATER);

	spello(SPELL_BARKSKIN, X, X, X, X, X, X, X, X, X, 4, X, X, X, X, X, X, X,
		60, 25, 1,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_ICY_BLAST, X, 41, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		80, 50, 3,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE);

	spello(SPELL_INVIS_TO_UNDEAD, 30, X, X, X, X, X, X, X, X, 21, X, X, X, X,
		X, X, X, 60, 40, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_ANIMAL_KIN, X, X, X, X, X, X, X, X, X, 17, X, X, X, X,
		X, X, X, 60, 40, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_GREATER_ENCHANT, 34, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 230, 120, 5, POS_STANDING, TAR_OBJ_INV, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_GREATER_INVIS, 38, X, X, X, X, X, X, X, X, X, X, X, 44, X, X,
		X, X, 85, 65, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_GROUP_ARMOR, X, 19, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 30, 2, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_DIVINE | MAG_GROUPS);

	spello(SPELL_FIREBALL, 31, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		130, 40, 5,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE | MAG_NOWATER);

	/* C L A S S E S      M A N A   */
	spello(SPELL_FIRE_SHIELD, 17, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 70, 30, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS | MAG_NOWATER);

	spello(SPELL_GREATER_HEAL, X, 34, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 120, 90, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_POINTS | MAG_UNAFFECTS);

	spello(SPELL_GROUP_HEAL, X, 31, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 80, 60, 5, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_DIVINE | MAG_GROUPS);

	spello(SPELL_HARM, X, 29, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		95, 45, 2,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE);

	spello(SPELL_HEAL, X, 24, X, X, X, X, X, X, 28, X, X, X, X, X, X, X, X,
		60, 30, 1,
		POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_POINTS | MAG_UNAFFECTS);

	spello(SPELL_DIVINE_ILLUMINATION, X, 9, X, X, X, X, X, X, 7, X, X, X, X, X,
		X, X, X, 40, 20, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_HASTE, 44, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 1,
		POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MAGIC | MAG_AFFECTS);

	/* C L A S S E S      M A N A   */
	spello(SPELL_INFRAVISION, 3, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 25, 10, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_INVISIBLE, 12, X, X, X, X, X, X, X, X, X, X, X, 16, X, X, X,
		X, 35, 25, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
		FALSE, MAG_MAGIC | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_GLOWLIGHT, 8, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		30, 20, 1,
		POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_KNOCK, 27, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_LIGHTNING_BOLT, 15, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 50, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE | MAG_WATERZAP);

	spello(SPELL_LOCATE_OBJECT, 23, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 25, 20, 1, POS_STANDING, TAR_OBJ_WORLD, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_MAGIC_MISSILE, 1, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 20, 8, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE);

	spello(SPELL_MINOR_IDENTIFY, 15, 46, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 75, 50, 3, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	/* C L A S S E S      M A N A   */
	spello(SPELL_MAGICAL_PROT, 32, 26, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 40, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_MAGICAL_VESTMENT, X, 11, X, X, X, X, X, X, 4, X, X, X, X, X,
		X, X, X, 100, 80, 1, POS_STANDING, TAR_OBJ_INV, FALSE,
		MAG_MANUAL | MAG_DIVINE);

	spello(SPELL_METEOR_STORM, 48, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 180, 110, 25, POS_FIGHTING, TAR_IGNORE, TRUE,
		MAG_MAGIC | MAG_AREAS | MAG_NOWATER);


	spello(SPELL_CHAIN_LIGHTNING,
		/* M C T W B i h y K R H n V r 1 2 3   Max Min Chn */
		23, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, 120, 30, 5,
		POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MAGIC | MAG_MASSES | MAG_WATERZAP);

	spello(SPELL_HAILSTORM,
		/* M C T W B i h y K R  H n V r 1 2 3   Max Min Chn */
		X, X, X, X, X, X, X, X, X, 37, X, X, X, X, X, X, X, 180, 110, 10,
		POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MAGIC | MAG_MASSES | MAG_OUTDOORS);

	spello(SPELL_ICE_STORM,
		/* M C T W B i h y K R H n V r 1 2 3   Max Min Chn */
		X, 49, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, 130, 60, 15,
		POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MAGIC | MAG_MASSES);

	spello(SPELL_POISON, 20, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		50, 20, 3,
		POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
		MAG_MAGIC | MAG_OBJECTS | MAG_AFFECTS);

	spello(SPELL_PRAY, X, 27, X, X, X, X, X, X, 30, X, X, X, X, X, X, X, X,
		80, 50, 2,
		POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_PRISMATIC_SPRAY, 42, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 160, 80, 10, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_DAMAGE);

	spello(SPELL_PROTECT_FROM_DEVILS, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, X, 40, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_PROT_FROM_EVIL, X, 16, X, X, X, X, X, X, 9, X, X, X, X, X, X,
		X, X, 40, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	spello(SPELL_PROT_FROM_GOOD, X, 16, X, X, X, X, X, X, 9, X, X, X, X, X, X,
		X, X, 40, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_EVIL);

	/* Ma Cl Th Wa Ba Sy Ph Cy Kn Rn Hd Mk Vmp */
	spello(SPELL_PROT_FROM_LIGHTNING, X, X, X, X, X, X, X, X, X, 9, X, X, X, X,
		X, X, X, 40, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);
	/* Ma Cl Th Wa Ba Sy Ph Cy Kn  Rn Hd Mk Vmp */
	spello(SPELL_PROT_FROM_FIRE, X, 16, X, X, X, X, X, X, X, 19, X, X, X, X, X,
		X, X, 40, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_REMOVE_CURSE, 26, 27, X, X, X, X, X, X, 31, X, X, X, X, X, X,
		X, X, 45, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE,
		MAG_DIVINE | MAG_UNAFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_REMOVE_SICKNESS, X, 37, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 145, 85, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_UNAFFECTS);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Ba Sy Ph Cy Kn Rn Hd Mk Vmp Max Min Chn */
	spello(SPELL_REJUVENATE, X, 40, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 100, 40, 8, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_REFRESH, X, X, X, X, X, X, X, X, X, 15, X, X, X, X, X, X, X,
		100, 40, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_POINTS);

	spello(SPELL_REGENERATE, 49, 45, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 140, 100, 10, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_RETRIEVE_CORPSE, X, 48, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 125, 65, 15, POS_STANDING, TAR_CHAR_WORLD, FALSE,
		MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_SANCTUARY, X, 28, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		110, 85, 5,
		POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_DIVINE | MAG_AFFECTS);

	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Bar Syk Ph Cy Kn Rn Hood Bnt Max Min Chn */
	spello(SPELL_SHOCKING_GRASP, 9, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MAGIC | MAG_TOUCH | MAG_DAMAGE);

	spello(SPELL_SHROUD_OBSCUREMENT, 36, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, X, 145, 65, 10, MAG_MAGIC | POS_STANDING,
		TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_SLEEP, 10, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		40, 25, 5, POS_STANDING, TAR_CHAR_ROOM, TRUE, MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_SLOW, 38, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		120, 60, 6, POS_FIGHTING,
		TAR_UNPLEASANT | TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_SPIRIT_HAMMER, X, 10, X, X, X, X, X, X, 15, X, X, X, X, X, X,
		X, X, 40, 10, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE);

	spello(SPELL_STRENGTH, 6, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		65, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_SUMMON, 29, 17, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		320, 180, 7, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_SWORD, 30, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		265, 180, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_SYMBOL_OF_PAIN, X, 38, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 140, 80, 5, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE | MAG_AFFECTS | MAG_EVIL);

	spello(SPELL_STONE_TO_FLESH, X, 36, X, X, X, X, X, X, X, 44, X, X, X, X, X,
		X, X, 380, 230, 20, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_UNAFFECTS);

	spello(SPELL_TELEKINESIS, 28, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 165, 70, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_WORD_STUN, 37, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		100, 50, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_MAGIC | MAG_AFFECTS);


	spello(SPELL_TRUE_SEEING, 41, 42, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 210, 65, 15, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	/* Ma Cl Th Wa Bar Sy Ph Cy Kn Rn Hd Bnt vmp */
	spello(SPELL_WORD_OF_RECALL, X, 15, X, X, X, X, X, X, 25, X, X, X, X, X, X,
		X, X, 100, 20, 2, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV, FALSE,
		MAG_DIVINE | MAG_MANUAL);

	spello(SPELL_WORD_OF_INTELLECT, X, 35, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, X, 100, 40, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_AFFECTS);

	spello(SPELL_PEER, 40, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		100, 40, 6, POS_STANDING, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
		FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_REMOVE_POISON, X, 8, X, X, X, X, X, X, 19, 12, X, X, X, X, X,
		X, X, 40, 8, 4, POS_STANDING,
		TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
		MAG_DIVINE | MAG_UNAFFECTS);

	spello(SPELL_RESTORATION, X, 43, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 210, 80, 11, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_POINTS);


	spello(SPELL_SENSE_LIFE, 27, 23, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 120, 80, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_UNDEAD_PROT, 41, 18, X, X, X, X, X, X, 18, X, X, X, X, X, X,
		X, X, 60, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_WATERWALK, 32, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	spello(SPELL_IDENTIFY, 25, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		95, 50, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
		FALSE, MAG_MAGIC | MAG_MANUAL);

	spello(SPELL_WALL_OF_THORNS, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_EXITS);

	spello(SPELL_WALL_OF_STONE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_EXITS);

	spello(SPELL_WALL_OF_ICE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_EXITS);

	spello(SPELL_WALL_OF_FIRE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE,
		MAG_MAGIC | MAG_EXITS | MAG_NOWATER);

	spello(SPELL_WALL_OF_IRON, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_EXITS);

	spello(SPELL_THORN_TRAP, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		50, 20, 1, POS_STANDING, TAR_DOOR, FALSE, MAG_MAGIC | MAG_EXITS);

	spello(SPELL_FIERY_SHEET, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 50, 20, 1, POS_STANDING, TAR_DOOR, FALSE,
		MAG_MAGIC | MAG_EXITS | MAG_NOWATER);

	/*   Psionic TRIGGERS go here ----\/ \/ \/       */

	spello(SPELL_POWER, X, X, X, X, X, 15, X, X, X, X, X, X, X, X, X, X, X,
		80, 50, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_INTELLECT, X, X, X, X, X, 23, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS | MAG_UNAFFECTS);

	spello(SPELL_CONFUSION, X, X, X, X, X, 36, X, X, X, X, X, X, X, X, X, X, X,
		110, 80, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
		TRUE, MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_FEAR, X, X, X, X, X, 29, X, X, X, X, X, X, X, X, X, X, X,
		60, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_SATIATION, X, X, X, X, X, 3, X, X, X, X, X, X, X, X, X, X, X,
		30, 10, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_POINTS);

	spello(SPELL_QUENCH, X, X, X, X, X, 4, X, X, X, X, X, X, X, X, X, X, X,
		35, 15, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_POINTS);

	spello(SPELL_CONFIDENCE, X, X, X, X, X, 8, X, X, X, X, X, X, X, X, X, X, X,
		60, 40, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS | MAG_UNAFFECTS);

	spello(SPELL_NOPAIN, X, X, X, X, X, 30, X, X, X, X, X, X, X, X, X, X, X,
		130, 100, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_DERMAL_HARDENING, X, X, X, X, X, 12, X, X, X, X, X, X, X, X,
		X, X, X, 70, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_WOUND_CLOSURE, X, X, X, X, X, 10, X, X, X, X, X, X, X, X, X,
		X, X, 80, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_POINTS);

	spello(SPELL_ANTIBODY, X, X, X, X, X, 19, X, X, X, X, X, X, X, X, X, X, X,
		80, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_UNAFFECTS);

	spello(SPELL_RETINA, X, X, X, X, X, 13, X, X, X, X, X, X, X, X, X, X, X,
		60, 25, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_ADRENALINE, X, X, X, X, X, 27, X, X, X, X, X, X, X, X, X, X,
		X, 80, 60, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_BREATHING_STASIS, X, X, X, X, X, 21, X, X, X, X, X, X, X, X,
		X, X, X, 100, 50, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_VERTIGO, X, X, X, X, X, 31, X, X, X, X, X, X, X, X, X, X, X,
		70, 50, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_METABOLISM, X, X, X, X, X, 9, X, X, X, X, X, X, X, X, X, X, X,
		50, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_EGO_WHIP, X, X, X, X, X, 22, X, X, X, X, X, X, X, X, X, X, X,
		50, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_PSIONIC);

	spello(SPELL_PSYCHIC_CRUSH, X, X, X, X, X, 35, X, X, X, X, X, X, X, X, X,
		X, X, 130, 30, 15, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_RELAXATION, X, X, X, X, X, 7, X, X, X, X, X, X, X, X, X, X, X,
		100, 40, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS | MAG_UNAFFECTS);

	spello(SPELL_WEAKNESS, X, X, X, X, X, 16, X, X, X, X, X, X, X, X, X, X, X,
		70, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_FORTRESS_OF_WILL, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_CELL_REGEN, X, X, X, X, X, 34, X, X, X, X, X, X, X, X, X, X,
		X, 100, 50, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_POINTS | MAG_AFFECTS);

	spello(SPELL_PSISHIELD, X, X, X, X, X, 30, X, X, X, X, X, X, X, X, X, X, X,
		120, 60, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_PSYCHIC_SURGE, X, X, X, X, X, 40, X, X, X, X, X, X, X, X, X,
		X, X, 90, 60, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE,
		MAG_PSIONIC | MAG_DAMAGE);

	spello(SPELL_PSYCHIC_CONDUIT, X, X, X, X, X, 26, X, X, X, X, X, X, X, X, X,
		X, X, 70, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE,
		MAG_PSIONIC | MAG_POINTS);

	spello(SPELL_PSIONIC_SHATTER, X, X, X, X, X, 33, X, X, X, X, X, X, X, X, X,
		X, X, 80, 30, 3, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_UNAFFECTS);

	spello(SPELL_ID_INSINUATION, X, X, X, X, X, 32, X, X, X, X, X, X, X, X, X,
		X, X, 110, 80, 5, POS_STANDING,
		TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_MANUAL);

	spello(SPELL_MELATONIC_FLOOD, X, X, X, X, X, 18, X, X, X, X, X, X, X, X, X,
		X, X, 90, 30, 5, POS_STANDING, TAR_CHAR_ROOM, TRUE,
		MAG_PSIONIC | MAG_AFFECTS | MAG_UNAFFECTS);

	spello(SPELL_MOTOR_SPASM, X, X, X, X, X, 37, X, X, X, X, X, X, X, X, X, X,
		X, 100, 70, 5, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, FALSE,
		MAG_DAMAGE | MAG_AFFECTS | MAG_PSIONIC);

	spello(SPELL_PSYCHIC_RESISTANCE, X, X, X, X, X, 20, X, X, X, X, X, X, X, X,
		X, X, X, 80, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_AFFECTS | MAG_PSIONIC);

	spello(SPELL_MASS_HYSTERIA, X, X, X, X, X, 48, X, X, X, X, X, X, X, X, X,
		X, X, 100, 30, 1, POS_STANDING, TAR_IGNORE, TRUE,
		MAG_PSIONIC | MAG_AREAS);

	spello(SPELL_GROUP_CONFIDENCE, X, X, X, X, X, 14, X, X, X, X, X, X, X, X,
		X, X, X, 90, 45, 2, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_PSIONIC | MAG_GROUPS);

	spello(SPELL_CLUMSINESS, X, X, X, X, X, 28, X, X, X, X, X, X, X, X, X, X,
		X, 70, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_ENDURANCE, X, X, X, X, X, 11, X, X, X, X, X, X, X, X, X, X, X,
		90, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_AFFECTS | MAG_POINTS);

	spello(SPELL_NULLPSI, X, X, X, X, X, 17, X, X, X, X, X, X, X, X, X, X, X,
		90, 50, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PSIONIC | MAG_UNAFFECTS);

	spello(SPELL_TELEPATHY, X, X, X, X, X, 41, X, X, X, X, X, X, X, X, X, X, X,
		95, 62, 4, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
		MAG_PSIONIC | MAG_AFFECTS);

	spello(SPELL_DISTRACTION, X, X, X, X, X, 2, X, X, X, X, X, X, X, X, X, X, X,
		40, 10, 4, POS_STANDING, TAR_CHAR_ROOM, false,
		MAG_PSIONIC | MAG_MANUAL);

	spello(SKILL_PSIBLAST, X, X, X, X, X, 5, X, X, X, X, X, X, X, X, X, X, X,
		50, 50, 1, 0, 0, 0, MAG_DAMAGE | MAG_PSIONIC);

	spello(SKILL_PSILOCATE, X, X, X, X, X, 25, X, X, X, X, X, X, X, X, X, X, X,
		100, 30, 5, 0, 0, 0, MAG_PSIONIC);

	spello(SKILL_PSIDRAIN, X, X, X, X, X, 24, X, X, X, X, X, X, X, X, X, X, X,
		50, 30, 1, 0, 0, 0, MAG_PSIONIC);

	/*   Physic ALTERS go here ----\/ \/ \/       */
	/* C L A S S E S      M A N A   */
	/* Ma Cl Th Wa Bar Syk Ph Cyb Kni Ran Hood Bnt  */
	spello(SPELL_ELECTROSTATIC_FIELD, X, X, X, X, X, X, 1, X, X, X, X, X, X, X,
		X, X, X, 40, 20, 2, POS_SITTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_ENTROPY_FIELD, X, X, X, X, X, X, 27, X, X, X, X, X, X, X,
		X, X, X, 101, 60, 8, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_DAMAGE);

	spello(SPELL_ACIDITY, X, X, X, X, X, X, 2, X, X, X, X, X, X, X, X, X, X,
		40, 20, 2, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_ATTRACTION_FIELD, X, X, X, X, X, X, 8, X, X, X, X, X, X, X, X,
		X, X, 60, 40, 1, POS_SITTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM | TAR_OBJ_INV |
		TAR_OBJ_EQUIP, TRUE, MAG_PHYSICS | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_NUCLEAR_WASTELAND, X, X, X, X, X, X, 40, X, X, X, X, X, X, X,
		X, X, X, 100, 50, 10, POS_SITTING, TAR_IGNORE, TRUE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_SPACETIME_IMPRINT, X, X, X, X, X, X, 30, X, X, X, X, X, X, X,
		X, X, X, 100, 50, 10, POS_SITTING, TAR_IGNORE, TRUE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_SPACETIME_RECALL, X, X, X, X, X, X, 30, X, X, X, X, X, X, X,
		X, X, X, 100, 50, 10, POS_SITTING, TAR_IGNORE, TRUE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_FLUORESCE, X, X, X, X, X, X, 3, X, X, X, X, X, X, X, X, X, X,
		50, 10, 1, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_GAMMA_RAY, X, X, X, X, X, X, 25, X, X, X, X, X, X, X, X, X, X,
		100, 50, 2, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_GRAVITY_WELL, X, X, X, X, X, X, 38, X, X, X, X, X, X, X, X, X,
		X, 130, 70, 4, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_CAPACITANCE_BOOST, X, X, X, X, X, X, 6, X, X, X, X, X, X, X,
           X, X, X, 70, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_POINTS);

	spello(SPELL_ELECTRIC_ARC, X, X, X, X, X, X, 21, X, X, X, X, X, X, X, X,
		X, X, 90, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE | MAG_WATERZAP);

	spello(SPELL_TEMPORAL_COMPRESSION, X, X, X, X, X, X, 29, X, X, X, X, 
           X, X, X, X, X, X, 80, 50, 1,
		POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_TEMPORAL_DILATION, X, X, X, X, X, X, 36, X, X, X, X, X, 
           X, X, X, X, X, 120, 60, 6, POS_FIGHTING, TAR_UNPLEASANT | 
           TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_HALFLIFE, X, X, X, X, X, X, 30, X, X, X, X, X, X, X, X, X, X,
		120, 70, 5, POS_SITTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_EQUIP | TAR_OBJ_INV |
		TAR_OBJ_ROOM, TRUE, MAG_PHYSICS | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_MICROWAVE, X, X, X, X, X, X, 18, X, X, X, X, X, X, X, X, X, X,
		45, 20, 2, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE);

	spello(SPELL_OXIDIZE, X, X, X, X, X, X, 10, X, X, X, X, X, X, X, X, X, X,
		30, 15, 1, POS_SITTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PHYSICS | MAG_DAMAGE);

	spello(SPELL_RANDOM_COORDINATES, X, X, X, X, X, X, 20, X, X, X, X, X, X, X,
		X, X, X, 100, 20, 2, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_REPULSION_FIELD, X, X, X, X, X, X, 16, X, X, X, X, X, X, X, X,
		X, X, 30, 20, 1, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_VACUUM_SHROUD, X, X, X, X, X, X, 31, X, X, X, X, X, X, X, X,
		X, X, 60, 40, 1, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_ALBEDO_SHIELD, X, X, X, X, X, X, 34, X, X, X, X, X, X, X, X,
		X, X, 90, 50, 1, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_TRANSMITTANCE, X, X, X, X, X, X, 14, X, X, X, X, X, X, X, X,
		X, X, 60, 30, 1, POS_SITTING,
		TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_INV, FALSE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_TIME_WARP, X, X, X, X, X, X, 35, X, X, X, X, X, X, X, X, X, X,
		100, 50, 8, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_RADIOIMMUNITY, X, X, X, X, X, X, 20, X, X, X, X, X, X, X, X,
		X, X, 80, 20, 8, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_DENSIFY, X, X, X, X, X, X, 26, X, X, X, X, X, X, X, X, X, X,
		80, 20, 8, POS_SITTING,
		TAR_CHAR_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_OBJ_INV, FALSE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_LATTICE_HARDENING, X, X, X, X, X, X, 28, X, X, X, X, X, X, X,
		X, X, X, 100, 40, 4, POS_SITTING,
		TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV, FALSE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_ALTER_OBJS);

	spello(SPELL_CHEMICAL_STABILITY, X, X, X, X, X, X, 9, X, X, X, X, X, X, X,
		X, X, X, 80, 20, 8, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS | MAG_UNAFFECTS);

	spello(SPELL_REFRACTION, X, X, X, X, X, X, 33, X, X, X, X, X, X, X, X, X,
		X, 80, 20, 8, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	spello(SPELL_NULLIFY, X, X, X, X, X, X, 11, X, X, X, X, X, X, X, X, X, X,
		90, 55, 5,
		POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_PHYSICS | MAG_UNAFFECTS);

	spello(SPELL_AREA_STASIS, X, X, X, X, X, X, 40, X, X, X, X, X, X, X, X, X,
		X, 100, 50, 5, POS_SITTING, TAR_IGNORE, FALSE,
		MAG_PHYSICS | MAG_MANUAL);

	spello(SPELL_EMP_PULSE, X, X, X, X, X, X, 34, X, X, X, X, X, X, X, X, X, X,
		145, 75, 5, POS_SITTING, TAR_IGNORE, FALSE, MAG_PHYSICS | MAG_MANUAL);

	/* Ma Cl Th Wa Bar Sy Ph Cb Kn Rn Hd Mnk Max Min Chn */
	spello(SPELL_FISSION_BLAST, X, X, X, X, X, X, 43, X, X, X, X, X, X, X, X,
		X, X, 150, 70, 10, POS_FIGHTING, TAR_IGNORE, TRUE,
		MAG_PHYSICS | MAG_AREAS);

//	spello(SKILL_AMBUSH, X, X, X, X, X, X, X, X, X, 29, 12, X, X, 39, X, X, X,
	spello(SKILL_AMBUSH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, POS_STANDING, TAR_IGNORE, true, MAG_MANUAL);

	spello(ZEN_HEALING, X, X, X, X, X, X, X, X, X, X, X, 36, X, X, X, X, X, 30,
		9, 2, 0, 0, 0, MAG_ZEN);
	spello(ZEN_AWARENESS, X, X, X, X, X, X, X, X, X, X, X, 21, X, X, X, X, X,
		30, 5, 1, 0, 0, 0, MAG_ZEN);
	spello(ZEN_OBLIVITY, X, X, X, X, X, X, X, X, X, X, X, 43, X, X, X, X, X,
		40, 10, 4, 0, 0, 0, MAG_ZEN);
	spello(ZEN_MOTION, X, X, X, X, X, X, X, X, X, X, X, 28, X, X, X, X, X, 30,
		7, 1, 0, 0, 0, MAG_ZEN);


	/* C L A S S E S      M A N A   */

	spello(SPELL_TIDAL_SPACEWARP, X, X, X, X, X, X, 15, X, X, X, X, X, X, X, X,
		X, X, 100, 50, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE,
		MAG_PHYSICS | MAG_AFFECTS);

	/* ALL REMORT SKILLS HERE */
	remort_spello(SPELL_VESTIGIAL_RUNE,
		CLASS_MAGE, 45, 1,
		450, 380, 10, POS_STANDING, TAR_OBJ_ROOM | TAR_OBJ_INV, FALSE,
		MAG_MAGIC | MAG_MANUAL);

	remort_spello(SPELL_PRISMATIC_SPHERE, 
		CLASS_MAGE, 40, 1, 
		170, 30, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY,
		FALSE, MAG_MAGIC | MAG_AFFECTS);

	remort_spello(SPELL_QUANTUM_RIFT,
		CLASS_PHYSIC, 45, 5,
		400, 300, 5, POS_SITTING, TAR_IGNORE, FALSE, MAG_PHYSICS | MAG_MANUAL);
	remort_spello(SPELL_STONESKIN,
		CLASS_RANGER, 20, 4,
		60, 25, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	remort_spello(SPELL_SHIELD_OF_RIGHTEOUSNESS,
		CLASS_CLERIC, 32, 2,
		50, 20, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GROUPS | MAG_GOOD);

	remort_spello(SPELL_SUN_RAY,
		CLASS_CLERIC, 20, 3,
		100, 40, 3, POS_FIGHTING, TAR_IGNORE, TRUE,
		MAG_DIVINE | MAG_MANUAL | MAG_GOOD);

	remort_spello(SPELL_TAINT,
		CLASS_KNIGHT, 42, 9,
		250, 180, 10, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_DAMAGE | MAG_AFFECTS | MAG_EVIL);

	remort_spello(SPELL_BLACKMANTLE,
		CLASS_CLERIC, 42, 1,
		120, 80, 5, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_AFFECTS | MAG_EVIL);

	remort_spello(SPELL_SANCTIFICATION,
		CLASS_KNIGHT, 33, 1,
		100, 60, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	remort_spello(SPELL_STIGMATA,
		CLASS_CLERIC, 23, 1,
		60, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	remort_spello(SPELL_DIVINE_POWER,
		CLASS_CLERIC, 30, 5,
		150, 80, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	remort_spello(SPELL_DEATH_KNELL,
		CLASS_CLERIC, 25, 1,
		100, 30, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
		MAG_DIVINE | MAG_DAMAGE | MAG_MANUAL | MAG_EVIL);

	remort_spello(SPELL_MANA_SHIELD, CLASS_MAGE, 29, 3,
		60, 30, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);

	remort_spello(SPELL_ENTANGLE, CLASS_RANGER, 22, 1,
		70, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_OUTDOORS | MAG_MAGIC | MAG_AFFECTS);

	remort_spello(SPELL_SUMMON_LEGION,
		CLASS_KNIGHT, 27, 5,
		200, 100, 5, POS_STANDING, TAR_IGNORE, FALSE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL);

	remort_spello(SPELL_ANTI_MAGIC_SHELL, CLASS_MAGE, 21, 2,
		150, 70, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_MAGIC | MAG_AFFECTS);


	remort_spello(SPELL_WARDING_SIGIL, CLASS_MAGE, 17, 1,
		300, 250, 2, POS_STANDING, TAR_OBJ_ROOM | TAR_OBJ_INV, FALSE,
		MAG_MAGIC | MAG_ALTER_OBJS);


	remort_spello(SPELL_DIVINE_INTERVENTION, CLASS_CLERIC, 21, 4,
		120, 70, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	spellgen(SPELL_DIVINE_INTERVENTION, CLASS_KNIGHT, 23, 6);

	remort_spello(SPELL_SPHERE_OF_DESECRATION, CLASS_CLERIC, 21, 4,
		120, 70, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_EVIL);

	spellgen(SPELL_SPHERE_OF_DESECRATION, CLASS_KNIGHT, 23, 6);

	remort_spello(SPELL_AMNESIA, CLASS_PSIONIC, 25, 1,
		70, 30, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_PSIONIC | MAG_AFFECTS);

	remort_spello(ZEN_TRANSLOCATION, CLASS_MONK, 37, 1,
		40, 20, 1, 0, 0, 0, MAG_ZEN);

	remort_spello(ZEN_CELERITY, CLASS_MONK, 25, 2,
		40, 20, 1, 0, 0, 0, MAG_ZEN);

	remort_spello(SKILL_DISGUISE, CLASS_THIEF, 17, 1, 40, 20, 1, 0, 0, 0, 0);
    
	remort_spello(SKILL_UNCANNY_DODGE, CLASS_THIEF, 22, 2,
		          0, 0, 0, 0, 0, 0, 0);

	remort_spello(SKILL_DE_ENERGIZE, CLASS_CYBORG, 22, 1,
		0, 0, 0, POS_FIGHTING, 0, TRUE, 0);

	remort_spello(SKILL_ASSIMILATE, CLASS_CYBORG, 22, 3,
		0, 0, 0, POS_FIGHTING, 0, TRUE, 0);

	remort_spello(SKILL_RADIONEGATION, CLASS_CYBORG, 17, 1,
		0, 0, 0, POS_FIGHTING, 0, TRUE, CYB_ACTIVATE);

	remort_spello(SPELL_MALEFIC_VIOLATION, CLASS_CLERIC, 39, 5,
		100, 50, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_EVIL);

	remort_spello(SPELL_RIGHTEOUS_PENETRATION, CLASS_CLERIC, 39, 5,
		100, 50, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
		MAG_DIVINE | MAG_AFFECTS | MAG_GOOD);

	remort_spello(SKILL_WORMHOLE, CLASS_PHYSIC, 30, 1, 50, 20, 2, 0, 0, 0, 0);

    remort_spello(SPELL_GAUSS_SHIELD, CLASS_PHYSIC, 32, 3, 90, 70, 1,
                  POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
                  MAG_PHYSICS | MAG_AFFECTS);

	remort_spello(SPELL_UNHOLY_STALKER, CLASS_CLERIC, 25, 3,
		100, 50, 5, POS_STANDING, TAR_CHAR_WORLD, TRUE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL | MAG_NOSUN);

	remort_spello(SPELL_ANIMATE_DEAD, CLASS_CLERIC, 24, 2,
		60, 30, 1, POS_STANDING, TAR_OBJ_ROOM, TRUE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL | MAG_NOSUN);

	remort_spello(SPELL_INFERNO, CLASS_CLERIC, 35, 1,
		100, 50, 7, POS_FIGHTING, TAR_IGNORE, TRUE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL);

	remort_spello(SPELL_VAMPIRIC_REGENERATION, CLASS_CLERIC, 37, 4,
		100, 50, 5, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE,
		MAG_DIVINE | MAG_AFFECTS | MAG_EVIL);

	remort_spello(SPELL_BANISHMENT, CLASS_CLERIC, 29, 4,
		100, 50, 5, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE,
		MAG_DIVINE | MAG_MANUAL | MAG_EVIL);

	remort_spello(SKILL_DISCIPLINE_OF_STEEL, CLASS_BARB, 10, 1,
		0, 0, 0, 0, 0, 0, 0);

	remort_spello(SKILL_GREAT_CLEAVE, CLASS_BARB, 40, 2, 0, 0, 0, 0, 0, 0, 0);

	remort_spello(SPELL_LOCUST_REGENERATION, CLASS_MAGE, 34, 5,
		125, 60, 5, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT
		| TAR_NOT_SELF, TRUE, MAG_MAGIC | MAG_AFFECTS);

    remort_spello(SPELL_THORN_SKIN, CLASS_RANGER, 38, 6, 110, 80, 1, 
                  POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
                  MAG_MAGIC | MAG_AFFECTS);
/*	--- Add this back in when we want trail-based tracking
	remort_spello(SPELL_SPIRIT_TRACK, CLASS_RANGER, 38, 2,
		120, 60, 10, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY,
		false, MAG_MAGIC | MAG_AFFECTS);
*/	


/*
 * SKILLS
 * 
 * The only parameters needed for skills are only the minimum levels for each
 * char_class.  The remaining 8 fields of the structure should be filled with
 * 0's.
 */

	/* Ma Cl Th Wa Br Ps Ph Cyb Kni Rn Hd Mnk vm mr 1 2 3 */
	spello(SKILL_CLEAVE, X, X, X, X, 30, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_STRIKE, X, X, X, X, 17, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HAMSTRING, X, X, X, X, X, X, X, X, X, X, 32, X, X, 32, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_DRAG, X, X, X, X, 40, X, X, X, X, X, 30, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SNATCH, X, X, X, X, X, X, X, X, X, X, 40, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_ARCHERY, X, X, 14, 5, 24, X, X, X, X, 9, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BOW_FLETCH, X, X, X, 10, X, X, X, X, X, 17, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	/*                        M C T Wa Br ps Ph cy Kn Rn Hd M v mr 1 2 3 */
	spello(SKILL_READ_SCROLLS, 1, 2, 3, X, X, 35, X, X, 21, 21, X, 7, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_USE_WANDS, 4, 9, 10, X, X, 20, X, X, 21, 29, X, 35, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BACKSTAB, X, X, 5, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CIRCLE, X, X, 33, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HIDE, X, X, 3, X, X, X, X, X, X, 15, 7, X, X, 30, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa Br Syk Ph Cyb Kni Rn Hd Mnk vm mr 1 2 3 */
	spello(SKILL_KICK, X, X, X, 1, 2, X, X, 3, X, 3, X, X, X, 2, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BASH, X, X, X, 1, 10, X, X, X, 9, 14, X, X, X, 4, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BREAK_DOOR, X, X, X, X, 26, X, X, X, X, X, X, X, X, 1, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HEADBUTT, X, X, X, 1, 13, X, X, X, X, 16, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HOTWIRE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_GOUGE, X, X, 30, X, X, X, X, X, X, X, X, 36, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_STUN, X, X, 35, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);

	spello(SKILL_FEIGN, X, X, 25, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl  Th  Wa Bar Syk Ph Cyb Kni Ran Hood Bnt */
	spello(SKILL_CONCEAL, X, X, 15, X, X, X, X, X, X, X, 16, X, X, 30, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PLANT, X, X, 20, X, X, X, X, X, X, X, X, X, X, 35, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PICK_LOCK, X, X, 1, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/*  Ma Cl Th Wa  Br Sy Ph Cy Kn  Rn Hd  Bt Vm */
	spello(SKILL_RESCUE, X, X, X, 3, 12, X, X, X, 8, 12, X, 16, X, 26, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SNEAK, X, X, 3, X, X, X, X, X, X, 8, X, X, 7, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_STEAL, X, X, 2, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_TRACK, X, X, 8, 9, 14, X, X, 11, X, 11, X, X, 5, 14, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PUNCH, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0);

	/*  Ma Cl Th Wa Br Sy Ph Cy Kn  Rn Hd Bt Vm */
	spello(SKILL_PILEDRIVE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SHIELD_MASTERY, X, X, X, X, X, X, X, X, 22, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SLEEPER, X, X, X, 1, 25, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_ELBOW, X, X, X, 1, 8, X, X, 9, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/*  Ma Cl Th Wa  Br Sy Ph Cy Kn  Rn Hd Bt Vm */
	spello(SKILL_KNEE, X, X, X, 1, 6, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/*  Ma Cl Th Wa  Br Sy Ph  Cy Kn  Rn Hd Bt Vm */
	spello(SKILL_AUTOPSY, X, X, X, 1, X, X, X, 40, X, 36, X, X, X, 10, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BERSERK, X, X, X, 1, 22, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);

	spello(SKILL_BATTLE_CRY, X, X, X, 1, 33, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);
	/*  Ma Cl Th Wa Br Sy Ph Cy Kn Rn Hd Mn Vm */
	spello(SKILL_KIA, X, X, X, 1, X, X, X, X, X, X, X, 23, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CRY_FROM_BEYOND, X, X, X, 1, 42, X, X, X, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_STOMP, X, X, 5, 1, 4, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BODYSLAM, X, X, X, 1, 20, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CHOKE, X, X, 23, 1, 16, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CLOTHESLINE, X, X, X, 1, 18, X, X, X, X, X, X, X, X, 12, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	/* Ma  Cl  Th  Wa Bar  Cyb Kni Ran Hood Bnt */
	/* Ma  Cl  Th  Wa Bar Syk Ph Cyb Kni Ran Hood Bnt */
	spello(SKILL_TAG, X, X, X, 1, 27, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_INTIMIDATE, X, X, X, 1, 35, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa  Ba Sy  Ph Cyb Kn Rn Hd Bt */
	spello(SKILL_SPEED_HEALING, X, X, X, 10, 39, X, X, X, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_STALK, X, X, 40, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HEARING, X, X, 11, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BEARHUG, X, X, X, X, X, X, X, X, X, 25, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BITE, X, X, X, X, X, X, X, X, X, 5, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa Ba Sy Ph Cy Kn  Rn  Hd Mn  V Mr */
	spello(SKILL_DBL_ATTACK, X, X, 45, 30, 37, X, X, X, 25, 30, 42, 24, X, 25,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_NIGHT_VISION, X, X, X, X, X, X, X, X, X, 2, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_TURN, X, 1, X, X, X, X, X, X, 3, X, X, X, X, X, X, X, X,
		90, 30, 5, 0, 0, 0, 0);

	/*    Ma Cl Th Wa Ba Sy  Ph  Cy Kn Rn Hd Mn Bm Mr S1 S2 S3  */
	spello(SKILL_ANALYZE, X, X, X, X, X, X, 12, 20, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/*    Ma Cl Th Wa Ba Sy Ph Cy Kn Rn Hd Mn Bm Mr S1 S2 S3  */
	spello(SKILL_EVALUATE, X, X, X, X, X, X, 10, 20, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CURE_DISEASE, X, X, X, 1, X, X, X, X, 43, X, X, X, X, X, X, X,
		X, 30, 10, 1, 0, 0, 0, 0);

	/*   Ma C T W B S P y K R H Mn V r S S S */
	spello(SKILL_HOLY_TOUCH, X, X, X, X, X, X, X, X, 2, X, X, X, X, X, X, X, X,
		30, 10, 1, 0, 0, 0, 0);

	spello(SKILL_BANDAGE, X, X, X, 1, 34, X, X, X, X, 10, X, 17, X, 3, X, X, X,
		40, 30, 1, 0, 0, 0, 0);

	spello(SKILL_FIRSTAID, X, X, X, 1, X, X, X, X, X, 19, X, 30, X, 16, X, X,
		X, 70, 60, 1, 0, 0, 0, 0);

	spello(SKILL_MEDIC, X, X, X, 1, X, X, X, X, X, 40, X, X, X, X, X, X, X,
		90, 80, 1, 0, 0, 0, 0);

	/* Ma Cl Th Wa Ba Sy Ph Cy Kn  Rn  Hd Mnk Vam */
	spello(SKILL_LEATHERWORKING, X, X, 32, 1, 26, X, X, X, 29, 28, X, 32, X, X,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa Ba Sy Ph Cy Kn  Rn  Hd Mnk Vam */
	spello(SKILL_METALWORKING, X, X, X, 1, 41, X, X, X, 32, 36, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa Ba Sy Ph Cy Kn  Rn  Hd Mnk Vam */
	spello(SKILL_CONSIDER, X, X, 11, 1, 9, X, X, X, 13, 18, X, 13, X, 13, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_GLANCE, X, X, 4, X, X, X, X, X, X, X, X, X, 14, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SHOOT, X, X, 10, X, X, X, X, 20, X, 26, 20, X, 10, 3, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BEHEAD, X, X, X, X, X, X, X, X, 40, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_EMPOWER, 5, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_DISARM, X, X, 13, 1, 32, X, X, X, 20, 20, X, 31, X, 25, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SPINKICK, X, X, X, 1, 17, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_ROUNDHOUSE, X, X, X, 1, 19, X, X, X, X, 20, X, X, X, 5, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SIDEKICK, X, X, X, 1, 29, X, X, X, X, 35, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SPINFIST, X, X, X, 1, 7, X, X, X, 6, 7, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_JAB, X, X, 6, 1, X, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HOOK, X, X, X, 1, 23, X, X, X, 22, 23, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SWEEPKICK, X, X, X, 1, 28, X, X, X, X, 27, X, 25, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th  Wa Ba Sy Ph Cy Kn Rn Hd Mk Vm Mr 1 2 3 */
	spello(SKILL_TRIP, X, X, 18, 1, X, X, X, X, X, X, 15, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_UPPERCUT, X, X, X, 1, 11, X, X, X, 15, 13, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_GROINKICK, X, X, 12, 1, X, X, X, X, X, X, 7, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CLAW, X, X, X, 1, X, X, X, X, X, 32, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_RABBITPUNCH, X, X, 10, 1, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_IMPALE, X, X, X, 1, 43, X, X, X, 30, 45, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PELE_KICK, X, X, 41, 1, 40, X, X, X, X, 41, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_LUNGE_PUNCH, X, X, X, 1, 32, X, X, X, 35, 42, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_TORNADO_KICK, X, X, X, X, 45, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_TRIPLE_ATTACK, X, X, X, 40, 48, X, X, X, 45, 43, X, 41, X, 38,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);
    
	spello(SKILL_COUNTER_ATTACK, X, X, X, X, X, X, X, X, X, 31, X, X, X, X,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);
    
	spello(SKILL_SWIMMING, 1, 1, 1, 1, 1, 2, 1, X, 1, 1, 1, 7, X, 1, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	/* Ma Cl Th Wa Ba Sy Ph  C Kn Rn Hd Mk Vm Mr 1 2 3 */
	spello(SKILL_THROWING, X, X, 7, 5, 15, X, X, X, X, 9, 3, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_RIDING, 1, 1, 1, 1, 1, X, X, X, 1, 1, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_APPRAISE, 10, 10, 10, X, 10, 10, 10, 10, 10, 10, X, 10, X, 10, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PIPEMAKING, 40, X, X, 1, X, X, X, X, X, 15, 13, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SECOND_WEAPON, X, X, X, X, X, X, X, X, X, 14, X, X, X, 18, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);
	/* Ma Cl Th Wa Ba Sy Ph  C Kn  Rn Hd Mk Vm Mr 1 2 3 */
	spello(SKILL_SCANNING, X, X, 31, X, X, X, X, X, X, 24, X, X, X, 22, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_RETREAT, X, X, 16, 16, X, X, X, X, X, 18, 17, 33, X, 22, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);
/**********************   MERC SKILLS  ********************/
	spello(SKILL_GUNSMITHING, X, X, X, X, X, X, X, X, X, X, X, X, X, 20, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PISTOLWHIP, X, X, X, X, X, X, X, X, X, X, X, X, X, 20, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CROSSFACE, X, X, X, X, X, X, X, X, X, X, X, X, X, 26, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_WRENCH, X, X, X, X, X, X, X, X, X, X, X, X, X, 6, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_ELUSION, X, X, X, X, X, X, X, X, X, X, X, X, X, 11, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	remort_spello(SKILL_SNIPE, CLASS_MERCENARY, 37, 2, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_INFILTRATE, X, X, X, X, X, X, X, X, X, X, X, X, X, 27, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SHOULDER_THROW, X, X, X, X, X, X, X, X, X, X, X, X, X, 39, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

/*********************  BARB SKILLS ***********************/
	spello(SKILL_PROF_POUND, X, X, X, X, 3, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_PROF_WHIP, X, X, X, X, 11, X, X, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_PROF_PIERCE, X, X, X, X, 18, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_PROF_SLASH, X, X, X, X, 26, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_PROF_CRUSH, X, X, X, X, 34, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_PROF_BLAST, X, X, X, X, 42, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

/**********************   MERC DEVICES   *********************/
	/*                                                               24 */
	spello(SPELL_DECOY, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		75, 30, 7,
		POS_STANDING, TAR_IGNORE, FALSE, MAG_MERCENARY | MAG_MANUAL);

	spello(SKILL_GAROTTE, X, X, X, X, X, X, X, X, X, X, X, X, X, 21, X, X, X,
		0, 0, 0, 0, 0, true, 0);

	spello(SKILL_DEMOLITIONS, X, X, X, X, X, X, X, X, X, X, X, X, X, 19, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

/**********************   CYBORG SKILLS  *********************/
	spello(SKILL_RECONFIGURE, X, X, X, X, X, X, X, 11, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_REBOOT, X, X, X, X, X, X, X, 1, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_MOTION_SENSOR, X, X, X, X, X, X, X, 16, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_STASIS, X, X, X, X, X, X, X, 10, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_ENERGY_FIELD, X, X, X, X, X, X, X, 32, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_REFLEX_BOOST, X, X, X, X, X, X, X, 18, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_IMPLANT_W, X, X, X, X, X, X, X, 26, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	remort_spello(SKILL_ADV_IMPLANT_W, CLASS_CYBORG, 28, 5,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_OFFENSIVE_POS, X, X, X, X, X, X, X, 15, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_DEFENSIVE_POS, X, X, X, X, X, X, X, 15, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	/* Ma Cl Th Wa Ba Sy Ph  C Kn  Rn Hd Mk Vm Mr 1 2 3 */
	spello(SKILL_MELEE_COMBAT_TAC, X, X, X, X, X, X, X, 24, X, X, X, X, X, X,
		X, X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_NEURAL_BRIDGING, X, X, X, X, X, X, X, 35, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_ARTERIAL_FLOW, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_OPTIMMUNAL_RESP, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	/* M C Th Wa Ba Sy Ph C  Kn Rn Hd Mk Vm  Mr */
	spello(SKILL_ADRENAL_MAXIMIZER, X, X, X, X, X, X, X, 37, X, X, X, X, X, X,
		X, X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_POWER_BOOST, X, X, X, X, X, X, X, 18, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_FASTBOOT, X, X, X, X, X, X, X, 12, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SELF_DESTRUCT, X, X, X, X, X, X, X, 12, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_BIOSCAN, X, X, X, X, X, X, X, 12, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_DISCHARGE, X, X, X, X, X, X, X, 14, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SELFREPAIR, X, X, X, X, X, X, X, 20, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CYBOREPAIR, X, X, X, X, X, X, X, 30, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_OVERHAUL, X, X, X, X, X, X, X, 45, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_DAMAGE_CONTROL, X, X, X, X, X, X, X, 12, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

	spello(SKILL_ELECTRONICS, X, X, X, X, X, X, 13, 6, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HACKING, X, X, X, X, X, X, X, 20, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CYBERSCAN, X, X, X, X, X, X, X, 30, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CYBO_SURGERY, X, X, X, X, X, X, X, 45, X, X, X, X, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HYPERSCAN, X, X, X, X, X, X, X, 25, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, CYB_ACTIVATE);

// spellnum, levels (MCTW), maxmana, minmana, manachng, minpos, targets,
// violent?, routines.
	spello(SKILL_OVERDRAIN, X, X, X, X, X, X, X, 8, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM, FALSE, 0);

	spello(SKILL_ENERGY_WEAPONS, X, X, X, X, X, X, X, 20, X, X, 30, X, X, 7,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);
	/*     M C T W B P Ph Cy Kn Rn Hd Mn Bm Mr 1 2 3 */
	spello(SKILL_PROJ_WEAPONS, X, X, X, X, X, X, X, 20, X, X, 10, X, X, 8, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SPEED_LOADING, X, X, X, X, X, X, X, 25, X, X, 30, X, X, 15, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	/***********************  MONK  SKILLS ***********************/
	spello(SKILL_PALM_STRIKE, X, X, X, X, X, X, X, X, X, X, X, 4, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_THROAT_STRIKE, X, X, X, X, X, X, X, X, X, X, X, 10, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_WHIRLWIND, X, X, X, X, X, X, X, X, X, X, X, 40, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_HIP_TOSS, X, X, X, X, X, X, X, X, X, X, X, 27, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_COMBO, X, X, X, X, X, X, X, X, X, X, X, 33, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_DEATH_TOUCH, X, X, X, X, X, X, X, X, X, X, X, 49, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_CRANE_KICK, X, X, X, X, X, X, X, X, X, X, X, 15, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_RIDGEHAND, X, X, X, X, X, X, X, X, X, X, X, 30, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_SCISSOR_KICK, X, X, X, X, X, X, X, X, X, X, X, 20, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_PINCH_ALPHA, X, X, X, X, X, X, X, X, X, X, X, 6, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_BETA, X, X, X, X, X, X, X, X, X, X, X, 12, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_GAMMA, X, X, X, X, X, X, X, X, X, X, X, 35, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_DELTA, X, X, X, X, X, X, X, X, X, X, X, 19, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_EPSILON, X, X, X, X, X, X, X, X, X, X, X, 26, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_OMEGA, X, X, X, X, X, X, X, X, X, X, X, 39, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);
	spello(SKILL_PINCH_ZETA, X, X, X, X, X, X, X, X, X, X, X, 32, X, X, X, X,
		X, 0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);

	spello(SKILL_MEDITATE, X, X, X, X, X, X, X, X, X, X, X, 9, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_KATA, X, X, X, X, X, X, X, X, X, X, X, 5, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_EVASION, X, X, X, X, X, X, X, X, X, X, X, 22, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, MAG_BIOLOGIC);

    
	/*  ******************* VAMPIRE SKILLS ********************   */
	spello(SKILL_FLYING, X, X, X, X, X, X, X, X, X, X, X, X, 33, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_SUMMON, X, X, X, X, X, X, X, X, X, X, X, X, 9, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_FEED, X, X, X, X, X, X, X, X, X, X, X, X, 1, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_BEGUILE, X, X, X, X, X, X, X, X, X, X, X, X, 26, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_DRAIN, X, X, X, X, X, X, X, X, X, X, X, X, 36, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_CREATE_VAMPIRE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_CONTROL_UNDEAD, X, X, X, X, X, X, X, X, X, X, X, X, 23, X, X,
		X, X, 0, 0, 0, 0, 0, 0, 0);
	spello(SKILL_TERRORIZE, X, X, X, X, X, X, X, X, X, X, X, X, 29, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	// physic skills
	/*     M  C  T  W  B  P Ph Cy Kn Rn Hd Mn Bm Mr 1 2 3 */
	spello(SKILL_LECTURE, X, X, X, X, X, X, 30, X, X, X, X, X, X, X, X, X, X,
		0, 0, 0, 0, 0, 0, 0);

	spello(SKILL_ENERGY_CONVERSION, X, X, X, X, X, X, 22, X, X, X, X, X, X, X,
		X, X, X, 0, 0, 0, 0, 0, 0, 0);


	/* spello(SPELL_IDENTIFY, 0, 0, 0, 0, 0, 0, 0, 0, */
/*         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL); */

	spello(SPELL_HELL_FIRE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE);

	spello(SPELL_HELL_FROST, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_HELL_FIRE_STORM, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 15, 1, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MASSES);

	spello(SPELL_HELL_FROST_STORM, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 15, 1, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MASSES);

	spello(SPELL_TROG_STENCH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_AFFECTS);

	spello(SPELL_FROST_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE);

	spello(SPELL_FIRE_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_NOWATER);

	spello(SPELL_GAS_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_ACID_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_AFFECTS);

	spello(SPELL_LIGHTNING_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE | MAG_WATERZAP);

	spello(SPELL_MANA_RESTORE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);

	spello(SPELL_PETRIFY, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		330, 215, 21, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

	spello(SPELL_SICKNESS, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		330, 215, 21, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

	spello(SPELL_ESSENCE_OF_EVIL, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 330, 215, 21, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_POINTS | MAG_EVIL | MAG_DAMAGE);

	spello(SPELL_ESSENCE_OF_GOOD, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, X, 330, 215, 21, POS_SITTING, TAR_CHAR_ROOM, FALSE,
		MAG_POINTS | MAG_GOOD | MAG_DAMAGE);

	spello(SPELL_SHADOW_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_MANUAL | MAG_DAMAGE);

	spello(SPELL_STEAM_BREATH, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
		MAG_DAMAGE);

	spello(SPELL_QUAD_DAMAGE, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
		X, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE,
		MAG_AFFECTS);

	spello(SPELL_ZHENGIS_FIST_OF_ANNIHILATION, X, X, X, X, X, X, X, X, X, X, X,
		X, X, X, X, X, X, 30, 15, 1, POS_FIGHTING,
		TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
}

#undef __spell_parser_c__
