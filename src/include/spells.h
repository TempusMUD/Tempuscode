#ifndef _SPELLS_H_
#define _SPELLS_H_

/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spells.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include "creature.h"
#include "tmpstr.h"

extern int max_spell_num;
extern const char *spells[];

inline const char *
spell_to_str(int spell)
{
	if (spell < 0 || spell > max_spell_num) {
		return tmp_sprintf("!ILLEGAL(%d)!", spell);
	}

	return spells[spell];
}

inline int
str_to_spell(const char *spell)
{
	if( spell == NULL || *spell == '\0' )
		return -1;
	for( int i = 0; i < max_spell_num; i++ ) {
		if( strcmp(spell, spells[i]) == 0 )
			return i;
	}
	return -1;
}

static const int DEFAULT_STAFF_LVL = 12;
static const int DEFAULT_WAND_LVL = 12;

static const int CAST_UNDEFINED = -1;
static const int CAST_SPELL = 0;
static const int CAST_POTION = 1;
static const int CAST_WAND = 2;
static const int CAST_STAFF = 3;
static const int CAST_SCROLL = 4;
static const int CAST_PARA = 5;
static const int CAST_PETRI = 6;
static const int CAST_ROD = 7;
static const int CAST_BREATH = 8;
static const int CAST_CHEM = 9;
static const int CAST_PSIONIC = 10;
static const int CAST_PHYSIC = 11;
static const int CAST_INTERNAL = 12;
static const int CAST_MERCENARY = 13;

static const int MAG_DAMAGE = (1 << 0);
static const int MAG_AFFECTS = (1 << 1);
static const int MAG_UNAFFECTS = (1 << 2);
static const int MAG_POINTS = (1 << 3);
static const int MAG_ALTER_OBJS = (1 << 4);
static const int MAG_GROUPS = (1 << 5);
static const int MAG_MASSES = (1 << 6);
static const int MAG_AREAS = (1 << 7);
static const int MAG_SUMMONS = (1 << 8);
static const int MAG_CREATIONS = (1 << 9);
static const int MAG_MANUAL = (1 << 10);
static const int MAG_OBJECTS = (1 << 11);
static const int MAG_TOUCH = (1 << 12);
static const int MAG_MAGIC = (1 << 13);
static const int MAG_DIVINE = (1 << 14);
static const int MAG_PHYSICS = (1 << 15);
static const int MAG_PSIONIC = (1 << 16);
static const int MAG_BIOLOGIC = (1 << 17);
static const int CYB_ACTIVATE = (1 << 18);
static const int MAG_EVIL = (1 << 19);
static const int MAG_GOOD = (1 << 20);
static const int MAG_EXITS = (1 << 21);
static const int MAG_OUTDOORS = (1 << 22);
static const int MAG_NOWATER = (1 << 23);
static const int MAG_WATERZAP = (1 << 24);
static const int MAG_NOSUN = (1 << 25);
static const int MAG_ZEN = (1 << 26);
static const int MAG_MERCENARY = (1 << 27);

#define SPELL_IS_MAGIC(splnm)   IS_SET(spell_info[splnm].routines, MAG_MAGIC)
#define SPELL_IS_DIVINE(splnm)  IS_SET(spell_info[splnm].routines, MAG_DIVINE)
#define SPELL_IS_PHYSICS(splnm) IS_SET(spell_info[splnm].routines, MAG_PHYSICS)
#define SPELL_IS_PSIONIC(splnm) IS_SET(spell_info[splnm].routines, MAG_PSIONIC)
#define SPELL_IS_BIO(splnm)    IS_SET(spell_info[splnm].routines, MAG_BIOLOGIC)
#define SPELL_IS_PROGRAM(splnm)IS_SET(spell_info[splnm].routines, CYB_ACTIVATE)
#define SPELL_IS_EVIL(splnm)    IS_SET(spell_info[splnm].routines, MAG_EVIL)
#define SPELL_IS_GOOD(splnm)    IS_SET(spell_info[splnm].routines, MAG_GOOD)
#define SPELL_IS_MERCENARY(splnm) IS_SET(spell_info[splnm].routines, MAG_MERCENARY)

#define SPELL_USES_GRAVITY(splnm) (splnm == SPELL_GRAVITY_WELL)

#define SPELL_FLAGS(splnm)     (spell_info[splnm].routines)
#define SPELL_FLAGGED(splnm, flag) (IS_SET(SPELL_FLAGS(splnm), flag))

static const int TYPE_UNDEFINED = -1;
static const int SPELL_RESERVED_DBC = 0;	/* SKILL NUMBER ZERO -- RESERVED */

static const int UNHOLY_STALKER_VNUM = 1513;
static const int ZOMBIE_VNUM = 1512;

  /* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

static const int SPELL_ARMOR = 1;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_TELEPORT = 2;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_BLESS = 3;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_BLINDNESS = 4;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_BURNING_HANDS = 5;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CALL_LIGHTNING = 6;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CHARM = 7;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CHILL_TOUCH = 8;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CLONE = 9;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_COLOR_SPRAY = 10;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CONTROL_WEATHER = 11;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CREATE_FOOD = 12;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CREATE_WATER = 13;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CURE_BLIND = 14;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CURE_CRITIC = 15;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CURE_LIGHT = 16;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_CURSE = 17;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DETECT_ALIGN = 18;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DETECT_INVIS = 19;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DETECT_MAGIC = 20;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DETECT_POISON = 21;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DISPEL_EVIL = 22;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_EARTHQUAKE = 23;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_ENCHANT_WEAPON = 24;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_ENERGY_DRAIN = 25;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_FIREBALL = 26;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_HARM = 27;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_HEAL = 28;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_INVISIBLE = 29;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_LIGHTNING_BOLT = 30;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_LOCATE_OBJECT = 31;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_MAGIC_MISSILE = 32;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_POISON = 33;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_PROT_FROM_EVIL = 34;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_REMOVE_CURSE = 35;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_SANCTUARY = 36;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_SHOCKING_GRASP = 37;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_SLEEP = 38;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_STRENGTH = 39;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_SUMMON = 40;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_VENTRILOQUATE = 41;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_WORD_OF_RECALL = 42;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_REMOVE_POISON = 43;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_SENSE_LIFE = 44;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_ANIMATE_DEAD = 45;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_DISPEL_GOOD = 46;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_GROUP_ARMOR = 47;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_GROUP_HEAL = 48;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_GROUP_RECALL = 49;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_INFRAVISION = 50;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_WATERWALK = 51;	/* Reserved Skill[] DO NOT CHANGE */
static const int SPELL_MANA_SHIELD = 52;
static const int SPELL_IDENTIFY = 53;
static const int SPELL_GLOWLIGHT = 54;
static const int SPELL_BLUR = 55;
static const int SPELL_KNOCK = 56;
static const int SPELL_DISPEL_MAGIC = 57;
static const int SPELL_WARDING_SIGIL = 58;
static const int SPELL_DIMENSION_DOOR = 59;
static const int SPELL_MINOR_CREATION = 60;
static const int SPELL_TELEKINESIS = 61;
static const int SPELL_SWORD = 62;
static const int SPELL_WORD_STUN = 63;
static const int SPELL_PRISMATIC_SPRAY = 64;
static const int SPELL_FIRE_SHIELD = 65;
static const int SPELL_DETECT_SCRYING = 66;
static const int SPELL_CLAIRVOYANCE = 67;
static const int SPELL_SLOW = 68;
static const int SPELL_GREATER_ENCHANT = 69;
static const int SPELL_ENCHANT_ARMOR = 70;
static const int SPELL_MINOR_IDENTIFY = 71;
static const int SPELL_FLY = 72;
static const int SPELL_GREATER_HEAL = 73;
static const int SPELL_CONE_COLD = 74;
static const int SPELL_TRUE_SEEING = 75;
static const int SPELL_PROT_FROM_GOOD = 76;
static const int SPELL_MAGICAL_PROT = 77;
static const int SPELL_UNDEAD_PROT = 78;
static const int SPELL_SPIRIT_HAMMER = 79;
static const int SPELL_PRAY = 80;
static const int SPELL_FLAME_STRIKE = 81;
static const int SPELL_ENDURE_COLD = 82;
static const int SPELL_MAGICAL_VESTMENT = 83;
static const int SPELL_REJUVENATE = 84;
static const int SPELL_REGENERATE = 85;
static const int SPELL_COMMAND = 86;
static const int SPELL_AIR_WALK = 87;
static const int SPELL_DIVINE_ILLUMINATION = 88;
static const int SPELL_GOODBERRY = 89;
static const int SPELL_BARKSKIN = 90;
static const int SPELL_INVIS_TO_UNDEAD = 91;
static const int SPELL_HASTE = 92;
static const int SPELL_ANIMAL_KIN = 93;
static const int SPELL_CHARM_ANIMAL = 94;
static const int SPELL_REFRESH = 95;
static const int SPELL_BREATHE_WATER = 96;
static const int SPELL_CONJURE_ELEMENTAL = 97;
static const int SPELL_GREATER_INVIS = 98;
static const int SPELL_PROT_FROM_LIGHTNING = 99;
static const int SPELL_PROT_FROM_FIRE = 100;
static const int SPELL_RESTORATION = 101;
static const int SPELL_WORD_OF_INTELLECT = 102;
static const int SPELL_GUST_OF_WIND = 103;
static const int SPELL_RETRIEVE_CORPSE = 104;
static const int SPELL_LOCAL_TELEPORT = 105;
static const int SPELL_DISPLACEMENT = 106;
static const int SPELL_PEER = 107;
static const int SPELL_METEOR_STORM = 108;
static const int SPELL_PROTECT_FROM_DEVILS = 109;
static const int SPELL_SYMBOL_OF_PAIN = 110;
static const int SPELL_ICY_BLAST = 111;
static const int SPELL_ASTRAL_SPELL = 112;
static const int SPELL_VESTIGIAL_RUNE = 113;
static const int SPELL_DISRUPTION = 114;
static const int SPELL_STONE_TO_FLESH = 115;
static const int SPELL_REMOVE_SICKNESS = 116;
static const int SPELL_SHROUD_OBSCUREMENT = 117;
static const int SPELL_PRISMATIC_SPHERE = 118;
static const int SPELL_WALL_OF_THORNS = 119;
static const int SPELL_WALL_OF_STONE = 120;
static const int SPELL_WALL_OF_ICE = 121;
static const int SPELL_WALL_OF_FIRE = 122;
static const int SPELL_WALL_OF_IRON = 123;
static const int SPELL_THORN_TRAP = 124;
static const int SPELL_FIERY_SHEET = 125;
static const int SPELL_CHAIN_LIGHTNING = 126;
static const int SPELL_HAILSTORM = 127;
static const int SPELL_ICE_STORM = 128;
static const int SPELL_SHIELD_OF_RIGHTEOUSNESS = 129;	// group protection
static const int SPELL_BLACKMANTLE = 130;	// blocks healing spells
static const int SPELL_SANCTIFICATION = 131;	// 2x dam vs. evil
static const int SPELL_STIGMATA = 132;	// causes a bleeding wound
static const int SPELL_SUMMON_LEGION = 133;	// knights summon devils
static const int SPELL_ENTANGLE = 134;	// rangers entangle in veg.
static const int SPELL_ANTI_MAGIC_SHELL = 135;
static const int SPELL_DIVINE_INTERVENTION = 136;
static const int SPELL_SPHERE_OF_DESECRATION = 137;
static const int SPELL_MALEFIC_VIOLATION = 138;	// cuts thru good sanct
static const int SPELL_RIGHTEOUS_PENETRATION = 139;	// cuts thru evil sanct
static const int SPELL_UNHOLY_STALKER = 140;	// evil cleric hunter mob
static const int SPELL_INFERNO = 141;	// evil cleric room affect
static const int SPELL_VAMPIRIC_REGENERATION = 142;	// evil cleric vamp. regen
static const int SPELL_BANISHMENT = 143;	// evil cleric sends devils away
static const int SPELL_CONTROL_UNDEAD = 144;	// evil clerics charm undead
static const int SPELL_STONESKIN = 145;	// remort rangers stone skin
static const int SPELL_SUN_RAY = 146;	// Good cleric remort, 
											 // destroys undead.
static const int SPELL_TAINT = 147;	// Evil knight remort spell, taint.
static const int SPELL_LOCUST_REGENERATION = 148;	// Mage remort skill, drains mana
static const int SPELL_DIVINE_POWER = 149;	// Good cleric remort skill.
static const int SPELL_DEATH_KNELL = 150;	// Evil cleric remort skill.
static const int SPELL_TELEPATHY = 151;
static const int SPELL_DAMN = 152;

  /************************** Psionic Triggers ***************/
static const int SPELL_POWER = 201;	/* Strength                */
static const int SPELL_INTELLECT = 202;
static const int SPELL_CONFUSION = 203;
static const int SPELL_FEAR = 204;
static const int SPELL_SATIATION = 205;	/* fills hunger */
static const int SPELL_QUENCH = 206;	/* fills thirst */
static const int SPELL_CONFIDENCE = 207;	/* sets nopain */
static const int SPELL_NOPAIN = 208;
static const int SPELL_DERMAL_HARDENING = 209;
static const int SPELL_WOUND_CLOSURE = 210;
static const int SPELL_ANTIBODY = 211;
static const int SPELL_RETINA = 212;
static const int SPELL_ADRENALINE = 213;
static const int SPELL_BREATHING_STASIS = 214;
static const int SPELL_VERTIGO = 215;
static const int SPELL_METABOLISM = 216;	/* Increased healing, hunger, thirst */
static const int SPELL_EGO_WHIP = 217;
static const int SPELL_PSYCHIC_CRUSH = 218;
static const int SPELL_RELAXATION = 219;	/* speeds mana regen, weakens char */
static const int SPELL_WEAKNESS = 220;	/* minus str */
static const int SPELL_FORTRESS_OF_WILL = 221;
static const int SPELL_CELL_REGEN = 222;
static const int SPELL_PSISHIELD = 223;
static const int SPELL_PSYCHIC_SURGE = 224;
static const int SPELL_PSYCHIC_CONDUIT = 225;
static const int SPELL_PSIONIC_SHATTER = 226;
static const int SPELL_ID_INSINUATION = 227;
static const int SPELL_MELATONIC_FLOOD = 228;
static const int SPELL_MOTOR_SPASM = 229;
static const int SPELL_PSYCHIC_RESISTANCE = 230;
static const int SPELL_MASS_HYSTERIA = 231;
static const int SPELL_GROUP_CONFIDENCE = 232;
static const int SPELL_CLUMSINESS = 233;
static const int SPELL_ENDURANCE = 234;
static const int SPELL_AMNESIA = 235;	// psi remorts
static const int SPELL_NULLPSI = 236;	// remove psi affects
static const int SPELL_DISTRACTION = 238;
static const int SPELL_CALL_RODENT = 239;
static const int SPELL_CALL_BIRD = 240;
static const int SPELL_CALL_REPTILE = 241;
static const int SPELL_CALL_BEAST = 242;
static const int SPELL_CALL_PREDATOR = 243;

  /**************************  Mercenary Devices ******************/
static const int SPELL_DECOY = 237;
  /*************************** Physic Alterations *****************/
static const int SPELL_ACIDITY = 301;
static const int SPELL_ATTRACTION_FIELD = 302;
static const int SPELL_NUCLEAR_WASTELAND = 303;
static const int SPELL_FLUORESCE = 304;
static const int SPELL_GAMMA_RAY = 305;
static const int SPELL_HALFLIFE = 306;
static const int SPELL_MICROWAVE = 307;
static const int SPELL_OXIDIZE = 308;
static const int SPELL_RANDOM_COORDINATES = 309;	// random teleport
static const int SPELL_REPULSION_FIELD = 310;
static const int SPELL_TRANSMITTANCE = 311;	// transparency
static const int SPELL_SPACETIME_IMPRINT = 312;	// sets room as teleport spot
static const int SPELL_SPACETIME_RECALL = 313;	// teleports to imprint telep spot
static const int SPELL_TIME_WARP = 314;	// random teleport into other time
static const int SPELL_TIDAL_SPACEWARP = 315;	// fly
static const int SPELL_FISSION_BLAST = 316;	// full-room damage
static const int SPELL_REFRACTION = 317;	// like displacement
static const int SPELL_ELECTROSHIELD = 318;	// prot_lightning
static const int SPELL_VACUUM_SHROUD = 319;	// eliminates breathing and fire
static const int SPELL_DENSIFY = 320;	// increase weight of obj & char
static const int SPELL_CHEMICAL_STABILITY = 321;	// prevent/stop acidity
static const int SPELL_ENTROPY_FIELD = 322;	// drains move on victim (time effect)
static const int SPELL_GRAVITY_WELL = 323;	// time effect crushing damage
static const int SPELL_CAPACITANCE_BOOST = 324;	// increase maxmv
static const int SPELL_ELECTRIC_ARC = 325;	// lightning bolt
static const int SPELL_SONIC_BOOM = 326;	// area damage + wait state
static const int SPELL_LATTICE_HARDENING = 327;	// dermal hard or increase obj maxdam
static const int SPELL_NULLIFY = 328;	// like dispel magic
static const int SPELL_FORCE_WALL = 329;	// sets up an exit blocker
static const int SPELL_UNUSED_330 = 330;
static const int SPELL_PHASING = 331;	// invuln.
static const int SPELL_ABSORPTION_SHIELD = 332;	// works like mana shield
static const int SPELL_TEMPORAL_COMPRESSION = 333;	// works like haste
static const int SPELL_TEMPORAL_DILATION = 334;	// works like slow
static const int SPELL_GAUSS_SHIELD = 335;	// half damage from metal
static const int SPELL_ALBEDO_SHIELD = 336;	// reflects e/m attacks
static const int SPELL_THERMOSTATIC_FIELD = 337;	// sets prot_heat + end_cold
static const int SPELL_RADIOIMMUNITY = 338;	// sets prot_rad
static const int SPELL_TRANSDIMENSIONALITY = 339;	// randomly teleport to another plane
static const int SPELL_AREA_STASIS = 340;	// sets !phy room flag
static const int SPELL_ELECTROSTATIC_FIELD = 341;	// protective static field does damage to attackers
static const int SPELL_EMP_PULSE = 342;	// Shuts off devices, communicators
										// deactivats all cyborg programs
										// blocked by emp shield
static const int SPELL_QUANTUM_RIFT = 343;	// Shuts off devices, communicators
  /*********************  MONK ZENS  *******************/
static const int ZEN_HEALING = 401;
static const int ZEN_AWARENESS = 402;
static const int ZEN_OBLIVITY = 403;
static const int ZEN_MOTION = 404;
static const int ZEN_TRANSLOCATION = 405;
static const int ZEN_CELERITY = 406;

static const int MAX_SPELLS = 500;

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
static const int SKILL_BACKSTAB = 501;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_BASH = 502;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_HIDE = 503;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_KICK = 504;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_PICK_LOCK = 505;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_PUNCH = 506;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_RESCUE = 507;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_SNEAK = 508;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_STEAL = 509;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_TRACK = 510;	/* Reserved Skill[] DO NOT CHANGE */
static const int SKILL_PILEDRIVE = 511;
static const int SKILL_SLEEPER = 512;
static const int SKILL_ELBOW = 513;
static const int SKILL_KNEE = 514;
static const int SKILL_BERSERK = 515;
static const int SKILL_STOMP = 516;
static const int SKILL_BODYSLAM = 517;
static const int SKILL_CHOKE = 518;
static const int SKILL_CLOTHESLINE = 519;
static const int SKILL_TAG = 520;
static const int SKILL_INTIMIDATE = 521;
static const int SKILL_SPEED_HEALING = 522;
static const int SKILL_STALK = 523;
static const int SKILL_HEARING = 524;
static const int SKILL_BEARHUG = 525;
static const int SKILL_BITE = 526;
static const int SKILL_DBL_ATTACK = 527;
static const int SKILL_BANDAGE = 528;
static const int SKILL_FIRSTAID = 529;
static const int SKILL_MEDIC = 530;
static const int SKILL_CONSIDER = 531;
static const int SKILL_GLANCE = 532;
static const int SKILL_HEADBUTT = 533;
static const int SKILL_GOUGE = 534;
static const int SKILL_STUN = 535;
static const int SKILL_FEIGN = 536;
static const int SKILL_CONCEAL = 537;
static const int SKILL_PLANT = 538;
static const int SKILL_HOTWIRE = 539;
static const int SKILL_SHOOT = 540;
static const int SKILL_BEHEAD = 541;
static const int SKILL_DISARM = 542;
static const int SKILL_SPINKICK = 543;
static const int SKILL_ROUNDHOUSE = 544;
static const int SKILL_SIDEKICK = 545;
static const int SKILL_SPINFIST = 546;
static const int SKILL_JAB = 547;
static const int SKILL_HOOK = 548;
static const int SKILL_SWEEPKICK = 549;
static const int SKILL_TRIP = 550;
static const int SKILL_UPPERCUT = 551;
static const int SKILL_GROINKICK = 552;
static const int SKILL_CLAW = 553;
static const int SKILL_RABBITPUNCH = 554;
static const int SKILL_IMPALE = 555;
static const int SKILL_PELE_KICK = 556;
static const int SKILL_LUNGE_PUNCH = 557;
static const int SKILL_TORNADO_KICK = 558;
static const int SKILL_CIRCLE = 559;
static const int SKILL_TRIPLE_ATTACK = 560;

  /*******************  MONK SKILLS  *******************/
static const int SKILL_PALM_STRIKE = 561;
static const int SKILL_THROAT_STRIKE = 562;
static const int SKILL_WHIRLWIND = 563;
static const int SKILL_HIP_TOSS = 564;
static const int SKILL_COMBO = 565;
static const int SKILL_DEATH_TOUCH = 566;
static const int SKILL_CRANE_KICK = 567;
static const int SKILL_RIDGEHAND = 568;
static const int SKILL_SCISSOR_KICK = 569;
static const int SKILL_PINCH_ALPHA = 570;
static const int SKILL_PINCH_BETA = 571;
static const int SKILL_PINCH_GAMMA = 572;
static const int SKILL_PINCH_DELTA = 573;
static const int SKILL_PINCH_EPSILON = 574;
static const int SKILL_PINCH_OMEGA = 575;
static const int SKILL_PINCH_ZETA = 576;
static const int SKILL_MEDITATE = 577;
static const int SKILL_KATA = 578;
static const int SKILL_EVASION = 579;

static const int SKILL_SECOND_WEAPON = 580;
static const int SKILL_SCANNING = 581;
static const int SKILL_CURE_DISEASE = 582;
static const int SKILL_BATTLE_CRY = 583;
static const int SKILL_AUTOPSY = 584;
static const int SKILL_TRANSMUTE = 585;	/* physic transmute objs */
static const int SKILL_METALWORKING = 586;
static const int SKILL_LEATHERWORKING = 587;
static const int SKILL_DEMOLITIONS = 588;
static const int SKILL_PSIBLAST = 589;
static const int SKILL_PSILOCATE = 590;
static const int SKILL_PSIDRAIN = 591;	/* drain mana from vict */
static const int SKILL_GUNSMITHING = 592;	/* repair gunz */
static const int SKILL_ELUSION = 593;	/* !track */
static const int SKILL_PISTOLWHIP = 594;
static const int SKILL_CROSSFACE = 595;	/* rifle whip */
static const int SKILL_WRENCH = 596;	/* break neck */
static const int SKILL_CRY_FROM_BEYOND = 597;
static const int SKILL_KIA = 598;
static const int SKILL_WORMHOLE = 599;	// physic's wormhole
static const int SKILL_LECTURE = 600;	// physic's boring-ass lecture

static const int SKILL_TURN = 601;	/* Cleric's turn               */
static const int SKILL_ANALYZE = 602;	/* Physic's analysis           */
static const int SKILL_EVALUATE = 603;	/* Physic's evaluation      */
static const int SKILL_HOLY_TOUCH = 604;	/* Knight's skill              */
static const int SKILL_NIGHT_VISION = 605;
static const int SKILL_EMPOWER = 606;
static const int SKILL_SWIMMING = 607;
static const int SKILL_THROWING = 608;
static const int SKILL_RIDING = 609;
static const int SKILL_PIPEMAKING = 610;	//Make a pipe!
static const int SKILL_CHARGE = 611;	// BANG!


  /*****************  CYBORG SKILLS  ********************/
static const int SKILL_RECONFIGURE = 613;	// Re-allocate stats
static const int SKILL_REBOOT = 614;	// Start over from scratch
static const int SKILL_MOTION_SENSOR = 615;	// Detect Entries into room
static const int SKILL_STASIS = 616;	// State of rapid healing
static const int SKILL_ENERGY_FIELD = 617;	// Protective field
static const int SKILL_REFLEX_BOOST = 618;	// Speeds up processes
static const int SKILL_POWER_BOOST = 619;	// Increases Strength
static const int SKILL_UNUSED_1 = 620;	//  
static const int SKILL_FASTBOOT = 621;	// Reboots are faster
static const int SKILL_SELF_DESTRUCT = 622;	// Effective self destructs
static const int SKILL_UNUSED_2 = 623;	//  
static const int SKILL_BIOSCAN = 624;	// Sense Life scan
static const int SKILL_DISCHARGE = 625;	// Discharge attack
static const int SKILL_SELFREPAIR = 626;	// Repair hit points
static const int SKILL_CYBOREPAIR = 627;	// Repair other borgs
static const int SKILL_OVERHAUL = 628;	// Overhaul other borgs
static const int SKILL_DAMAGE_CONTROL = 629;	// Damage Control System
static const int SKILL_ELECTRONICS = 630;	// Operation of Electronics
static const int SKILL_HACKING = 631;	// hack electronic systems 
static const int SKILL_CYBERSCAN = 632;	// scan others for implants
static const int SKILL_CYBO_SURGERY = 633;	// implant objects
static const int SKILL_ENERGY_WEAPONS = 634;	// energy weapon use
static const int SKILL_PROJ_WEAPONS = 635;	// projectile weapon use
static const int SKILL_SPEED_LOADING = 636;	// speed load weapons
static const int SKILL_HYPERSCAN = 637;	// aware of hidden objs and traps
static const int SKILL_OVERDRAIN = 638;	// overdrain batteries
static const int SKILL_DE_ENERGIZE = 639;	// drain energy from chars
static const int SKILL_ASSIMILATE = 640;	// assimilate objects
static const int SKILL_RADIONEGATION = 641;	// immunity to radiation
static const int SKILL_IMPLANT_W = 642;	// Extra attacks with implant weapons.
static const int SKILL_ADV_IMPLANT_W = 643;	// ""
static const int SKILL_OFFENSIVE_POS = 644;	// Offensive Posturing
static const int SKILL_DEFENSIVE_POS = 645;	// Defensive Posturing
static const int SKILL_MELEE_COMBAT_TAC = 646;	// Melee combat tactics
static const int SKILL_NEURAL_BRIDGING = 647;	// Cogenic Neural Bridging
										// (Ambidextarity)
// Continue around 670

static const int SKILL_RETREAT = 648;	// controlled flee
static const int SKILL_DISGUISE = 649;	// look like a mob
static const int SKILL_AMBUSH = 650;	// surprise victim

//--------------  VAMPIRE SKILLS  ------------------
static const int SKILL_FLYING = 651;
static const int SKILL_SUMMON = 652;
static const int SKILL_FEED = 653;
static const int SKILL_DRAIN = 654;
static const int SKILL_BEGUILE = 655;
static const int SKILL_CREATE_VAMPIRE = 656;
static const int SKILL_CONTROL_UNDEAD = 657;
static const int SKILL_TERRORIZE = 658;

//-------------  Hood Skillz... yeah. Just two
static const int SKILL_HAMSTRING = 666;
static const int SKILL_SNATCH = 667;
static const int SKILL_DRAG = 668;

//static const int SKILL_TAUNT = 669;

//------------  Mercenary Skills -------------------
static const int SKILL_SNIPE = 669;	// sniper skill for mercs
static const int SKILL_INFILTRATE = 670;	// merc skill, improvement on sneak
static const int SKILL_SHOULDER_THROW = 671;	// grounding skill between hiptoss
										 // and sweepkick

// Overflow Cyborg
static const int SKILL_ARTERIAL_FLOW = 676;	// Arterial Flow Enhancement
static const int SKILL_OPTIMMUNAL_RESP = 677;	// Genetek Optimmunal Nodes
static const int SKILL_ADRENAL_MAXIMIZER = 678;	// Shukutei Adrenal Maximizer

static const int SKILL_ENERGY_CONVERSION = 679;	// physic's energy conversion

  /******************  PROFICENCIES  *******************/
static const int SKILL_PROF_POUND = 681;
static const int SKILL_PROF_WHIP = 682;
static const int SKILL_PROF_PIERCE = 683;
static const int SKILL_PROF_SLASH = 684;
static const int SKILL_PROF_CRUSH = 685;
static const int SKILL_PROF_BLAST = 686;
static const int SKILL_BREAK_DOOR = 687;
static const int SKILL_ARCHERY = 688;
static const int SKILL_BOW_FLETCH = 689;
static const int SKILL_READ_SCROLLS = 690;
static const int SKILL_USE_WANDS = 691;


/* New skills may be added here up to MAX_SKILLS (700) */
static const int SKILL_DISCIPLINE_OF_STEEL = 692;
static const int SKILL_STRIKE = 693;
static const int SKILL_CLEAVE = 694;
static const int SKILL_GREAT_CLEAVE = 695;
static const int SKILL_APPRAISE = 696;
static const int SKILL_GAROTTE = 697;
static const int SKILL_SHIELD_MASTERY = 698;
/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

static const int SPELL_FIRE_BREATH = 702;
static const int SPELL_GAS_BREATH = 703;
static const int SPELL_FROST_BREATH = 704;
static const int SPELL_ACID_BREATH = 705;
static const int SPELL_LIGHTNING_BREATH = 706;
static const int TYPE_LAVA_BREATH = 707;
static const int SPELL_EARTH_ELEMENTAL = 711;
static const int SPELL_FIRE_ELEMENTAL = 712;
static const int SPELL_WATER_ELEMENTAL = 713;
static const int SPELL_AIR_ELEMENTAL = 714;
static const int SPELL_HELL_FIRE = 717;
static const int JAVELIN_OF_LIGHTNING = 718;
static const int SPELL_TROG_STENCH = 719;
static const int SPELL_MANTICORE_SPIKES = 720;
static const int TYPE_CATAPULT = 730;
static const int TYPE_BALISTA = 731;
static const int TYPE_BOILING_OIL = 732;
static const int TYPE_HOTSANDS = 733;
static const int SPELL_MANA_RESTORE = 734;
static const int SPELL_BLADE_BARRIER = 735;
static const int SPELL_SUMMON_MINOTAUR = 736;
static const int TYPE_STYGIAN_LIGHTNING = 737;
static const int SPELL_SKUNK_STENCH = 738;
static const int SPELL_PETRIFY = 739;
static const int SPELL_SICKNESS = 740;
static const int SPELL_ESSENCE_OF_EVIL = 741;
static const int SPELL_ESSENCE_OF_GOOD = 742;
static const int SPELL_SHRINKING = 743;
static const int SPELL_HELL_FROST = 744;
static const int TYPE_ALIEN_BLOOD = 745;
static const int TYPE_SURGERY = 746;
static const int SMOKE_EFFECTS = 747;
static const int SPELL_HELL_FIRE_STORM = 748;
static const int SPELL_HELL_FROST_STORM = 749;
static const int SPELL_SHADOW_BREATH = 750;
static const int SPELL_STEAM_BREATH = 751;
static const int TYPE_TRAMPLING = 752;
static const int TYPE_GORE_HORNS = 753;
static const int TYPE_TAIL_LASH = 754;
static const int TYPE_BOILING_PITCH = 755;
static const int TYPE_FALLING = 756;
static const int TYPE_HOLYOCEAN = 757;
static const int TYPE_FREEZING = 758;
static const int TYPE_DROWNING = 759;
static const int TYPE_ABLAZE = 760;
static const int SPELL_QUAD_DAMAGE = 761;
static const int TYPE_VADER_CHOKE = 762;
static const int TYPE_ACID_BURN = 763;
static const int SPELL_ZHENGIS_FIST_OF_ANNIHILATION = 764;
static const int TYPE_RAD_SICKNESS = 765;
static const int TYPE_FLAMETHROWER = 766;
static const int TYPE_MALOVENT_HOLYTOUCH = 767;	// When holytouch wears off.
static const int SPELL_YOUTH = 768;
static const int TYPE_SWALLOW = 769;
static const int TOP_NPC_SPELL = 770;

static const int TOP_SPELL_DEFINE = 799;
/* NEW NPC/OBJECT SPELLS can be inserted here up to 799 */


/* WEAPON ATTACK TYPES */

static const int TYPE_HIT = 800;
static const int TYPE_STING = 801;
static const int TYPE_WHIP = 802;
static const int TYPE_SLASH = 803;
static const int TYPE_BITE = 804;
static const int TYPE_BLUDGEON = 805;
static const int TYPE_CRUSH = 806;
static const int TYPE_POUND = 807;
static const int TYPE_CLAW = 808;
static const int TYPE_MAUL = 809;
static const int TYPE_THRASH = 810;
static const int TYPE_PIERCE = 811;
static const int TYPE_BLAST = 812;
static const int TYPE_PUNCH = 813;
static const int TYPE_STAB = 814;
static const int TYPE_ENERGY_GUN = 815;
static const int TYPE_RIP = 816;
static const int TYPE_CHOP = 817;
static const int TYPE_SHOOT = 818;


static const int TOP_ATTACKTYPE = 819;
/* new attack types can be added here - up to TYPE_SUFFERING */
static const int TYPE_TAINT_BURN = 893;	// casting while tainted
static const int TYPE_PRESSURE = 894;
static const int TYPE_SUFFOCATING = 895;
static const int TYPE_ANGUISH = 896;	// Soulless and good aligned. dumbass.
static const int TYPE_BLEED = 897;	// Open wound
static const int TYPE_OVERLOAD = 898;	// cyborg overloading systems.
static const int TYPE_SUFFERING = 899;

static const int SAVING_PARA = 0;
static const int SAVING_ROD = 1;
static const int SAVING_PETRI = 2;
static const int SAVING_BREATH = 3;
static const int SAVING_SPELL = 4;
static const int SAVING_CHEM = 5;
static const int SAVING_PSI = 6;
static const int SAVING_PHY = 7;
static const int SAVING_NONE = 50;

static const int TAR_IGNORE = 1;
static const int TAR_CHAR_ROOM = 2;
static const int TAR_CHAR_WORLD = 4;
static const int TAR_FIGHT_SELF = 8;
static const int TAR_FIGHT_VICT = 16;
static const int TAR_SELF_ONLY = 32;	/* Only a check, use with i.e. TAR_CHAR_ROOM */
static const int TAR_NOT_SELF = 64;	/* Only a check, use with i.e. TAR_CHAR_ROOM */
static const int TAR_OBJ_INV = 128;
static const int TAR_OBJ_ROOM = 256;
static const int TAR_OBJ_WORLD = 512;
static const int TAR_OBJ_EQUIP = 1024;
static const int TAR_DOOR = 2048;
static const int TAR_UNPLEASANT = 4096;

struct spell_info_type {
	char min_position;			/* Position for caster   */
	sh_int mana_min;			/* Min amount of mana used by a spell (highest lev) */
	sh_int mana_max;			/* Max amount of mana used by a spell (lowest lev) */
	char mana_change;			/* Change in mana used by spell from lev to lev */

	char min_level[NUM_CLASSES];
	char gen[NUM_CLASSES];
	int routines;
	char violent;
	sh_int targets;				/* See below for use with TAR_XXX  */
};

extern struct spell_info_type spell_info[];
/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

static const int SPELL_TYPE_SPELL = 0;
static const int SPELL_TYPE_POTION = 1;
static const int SPELL_TYPE_WAND = 2;
static const int SPELL_TYPE_STAFF = 3;
static const int SPELL_TYPE_SCROLL = 4;


/* Attacktypes with grammar */

struct attack_hit_type {
	char *singular;
	char *plural;
};


#define ASPELL(spellname) \
void	spellname(byte level, struct Creature *ch, \
		  struct Creature *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

static inline int SPELL_LEVEL( int spell, int char_class ) {
	return spell_info[spell].min_level[char_class];
}
static inline int SPELL_GEN( int spell, int char_class ) {
	    return spell_info[spell].gen[char_class];
}

#define ABLE_TO_LEARN(ch, spl) \
((GET_REMORT_GEN(ch) >= SPELL_GEN(spl, GET_CLASS(ch)) && \
  GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_CLASS(ch))) || \
 (IS_REMORT(ch) && GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_REMORT_CLASS(ch)) && \
  !SPELL_GEN(spl, GET_REMORT_CLASS(ch))))

ASPELL(spell_astral_spell);
ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_charm_animal);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_minor_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_enchant_armor);
ASPELL(spell_greater_enchant);
ASPELL(spell_clairvoyance);
ASPELL(spell_coordinates);
ASPELL(spell_detect_poison);
ASPELL(spell_magical_vestment);
ASPELL(spell_conjure_elemental);
ASPELL(spell_knock);
ASPELL(spell_sword);
ASPELL(spell_animate_dead);
ASPELL(spell_control_weather);
ASPELL(spell_gust_of_wind);
ASPELL(spell_retrieve_corpse);
ASPELL(spell_local_teleport);
ASPELL(spell_peer);
ASPELL(spell_vestigial_rune);
ASPELL(spell_id_insinuation);
ASPELL(spell_shadow_breath);
ASPELL(spell_summon_legion);
ASPELL(spell_unholy_stalker);
ASPELL(spell_control_undead);
ASPELL(spell_inferno);
ASPELL(spell_banishment);
ASPELL(spell_sun_ray);
ASPELL(spell_emp_pulse);
ASPELL(spell_quantum_rift);
ASPELL(spell_decoy);
ASPELL(spell_death_knell);
ASPELL(spell_dispel_magic);
ASPELL(spell_distraction);
ASPELL(spell_bless);
ASPELL(spell_damn);
ASPELL(spell_call_rodent);
ASPELL(spell_call_bird);
ASPELL(spell_call_reptile);
ASPELL(spell_call_beast);
ASPELL(spell_call_predator);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_damage(int level, struct Creature *ch, struct Creature *victim,
	int spellnum, int savetype);

int mag_exits(int level, struct Creature *caster, struct room_data *room,
	int spellnum);

void mag_affects(int level, struct Creature *ch, struct Creature *victim,
	int spellnum, int savetype);

void mag_group_switch(int level, struct Creature *ch, struct Creature *tch,
	int spellnum, int savetype);

void mag_groups(int level, struct Creature *ch, int spellnum, int savetype);

void mag_masses(byte level, struct Creature *ch, int spellnum, int savetype);

int mag_areas(byte level, struct Creature *ch, int spellnum, int savetype);

void mag_summons(int level, struct Creature *ch, struct obj_data *obj,
	int spellnum, int savetype);

void mag_points(int level, struct Creature *ch, struct Creature *victim,
	int spellnum, int savetype);

void mag_unaffects(int level, struct Creature *ch, struct Creature *victim,
	int spellnum, int type);

void mag_alter_objs(int level, struct Creature *ch, struct obj_data *obj,
	int spellnum, int type);

void mag_creations(int level, struct Creature *ch, int spellnum);

int call_magic(struct Creature *caster, struct Creature *cvict,
	struct obj_data *ovict, int spellnum, int level, int casttype,
	int *return_flags = 0);

int mag_objectmagic(struct Creature *ch, struct obj_data *obj,
	char *argument, int *return_flags = NULL);

void mag_objects(int level, struct Creature *ch, struct obj_data *obj,
	int spellnum);

int cast_spell(struct Creature *ch, struct Creature *tch,
	struct obj_data *tobj, int spellnum, int *return_flags = 0);

int mag_savingthrow(struct Creature *ch, int level, int type);

int mag_manacost(struct Creature *ch, int spellnum);


#endif
