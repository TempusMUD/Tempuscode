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

extern int max_spell_num;
extern char **spells;

static inline /*@observer@*/ const char *
spell_to_str(int spell)
{
	if (spell < 0 || spell > max_spell_num) {
		return tmp_sprintf("!ILLEGAL(%d)!", spell);
	}

	return spells[spell];
}

static inline int
str_to_spell(const char *spell)
{
    int i;
	if( spell == NULL || *spell == '\0' )
		return -1;
	for(i = 0; i < max_spell_num; i++ ) {
		if( strcmp(spell, spells[i]) == 0 )
			return (int)i;
	}
	return -1;
}

enum {
    DEFAULT_STAFF_LVL = 12,
    DEFAULT_WAND_LVL = 12,
};

enum cast_flags {
    CAST_UNDEFINED = -1,
    CAST_SPELL = 0,
    CAST_POTION = 1,
    CAST_WAND = 2,
    CAST_STAFF = 3,
    CAST_SCROLL = 4,
    CAST_PARA = 5,
    CAST_PETRI = 6,
    CAST_ROD = 7,
    CAST_BREATH = 8,
    CAST_CHEM = 9,
    CAST_PSIONIC = 10,
    CAST_PHYSIC = 11,
    CAST_INTERNAL = 12,
    CAST_MERCENARY = 13,
    CAST_BARD = 14,
};

enum magic_flag {
    MAG_DAMAGE = (1 << 0),
    MAG_AFFECTS = (1 << 1),
    MAG_UNAFFECTS = (1 << 2),
    MAG_POINTS = (1 << 3),
    MAG_ALTER_OBJS = (1 << 4),
    MAG_GROUPS = (1 << 5),
    MAG_MASSES = (1 << 6),
    MAG_AREAS = (1 << 7),
    MAG_SUMMONS = (1 << 8),
    MAG_CREATIONS = (1 << 9),
    MAG_MANUAL = (1 << 10),
    MAG_OBJECTS = (1 << 11),
    MAG_TOUCH = (1 << 12),
    MAG_MAGIC = (1 << 13),
    MAG_DIVINE = (1 << 14),
    MAG_PHYSICS = (1 << 15),
    MAG_PSIONIC = (1 << 16),
    MAG_BIOLOGIC = (1 << 17),
    CYB_ACTIVATE = (1 << 18),
    MAG_EVIL = (1 << 19),
    MAG_GOOD = (1 << 20),
    MAG_EXITS = (1 << 21),
    MAG_OUTDOORS = (1 << 22),
    MAG_NOWATER = (1 << 23),
    MAG_WATERZAP = (1 << 24),
    MAG_NOSUN = (1 << 25),
    MAG_ZEN = (1 << 26),
    MAG_MERCENARY = (1 << 27),
    MAG_BARD = (1 << 28),
};

#define SPELL_IS_MAGIC(splnm)   (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_MAGIC))
#define SPELL_IS_DIVINE(splnm)  (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_DIVINE))
#define SPELL_IS_PHYSICS(splnm) (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_PHYSICS))
#define SPELL_IS_PSIONIC(splnm) (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_PSIONIC))
#define SPELL_IS_BIO(splnm)    (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_BIOLOGIC))
#define SPELL_IS_PROGRAM(splnm) (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, CYB_ACTIVATE))
#define SPELL_IS_EVIL(splnm)    (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_EVIL))
#define SPELL_IS_GOOD(splnm)    (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_GOOD))
#define SPELL_IS_MERCENARY(splnm) (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_MERCENARY))
#define SPELL_IS_BARD(splnm) (splnm <= TOP_SPELL_DEFINE && IS_SET(spell_info[splnm].routines, MAG_BARD))

#define SPELL_USES_GRAVITY(splnm) (splnm == SPELL_GRAVITY_WELL)

#define SPELL_FLAGS(splnm)     (spell_info[splnm].routines)
#define SPELL_FLAGGED(splnm, flag) (IS_SET(SPELL_FLAGS(splnm), flag))

#define SINFO spell_info[spellnum]

enum spell {
    TYPE_UNDEFINED = -1,
    SPELL_RESERVED_DBC = 0,	/* SKILL NUMBER ZERO -- RESERVED */

    UNHOLY_STALKER_VNUM = 1513,
    ZOMBIE_VNUM = 1512,

  /* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

    SPELL_ARMOR = 1,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_TELEPORT = 2,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_BLESS = 3,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_BLINDNESS = 4,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_BURNING_HANDS = 5,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CALL_LIGHTNING = 6,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CHARM = 7,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CHILL_TOUCH = 8,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CLONE = 9,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_COLOR_SPRAY = 10,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CONTROL_WEATHER = 11,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CREATE_FOOD = 12,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CREATE_WATER = 13,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CURE_BLIND = 14,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CURE_CRITIC = 15,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CURE_LIGHT = 16,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_CURSE = 17,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DETECT_ALIGN = 18,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DETECT_INVIS = 19,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DETECT_MAGIC = 20,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DETECT_POISON = 21,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DISPEL_EVIL = 22,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_EARTHQUAKE = 23,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_ENCHANT_WEAPON = 24,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_ENERGY_DRAIN = 25,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_FIREBALL = 26,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_HARM = 27,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_HEAL = 28,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_INVISIBLE = 29,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_LIGHTNING_BOLT = 30,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_LOCATE_OBJECT = 31,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_MAGIC_MISSILE = 32,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_POISON = 33,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_PROT_FROM_EVIL = 34,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_REMOVE_CURSE = 35,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_SANCTUARY = 36,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_SHOCKING_GRASP = 37,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_SLEEP = 38,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_STRENGTH = 39,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_SUMMON = 40,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_VENTRILOQUATE = 41,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_WORD_OF_RECALL = 42,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_REMOVE_POISON = 43,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_SENSE_LIFE = 44,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_ANIMATE_DEAD = 45,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_DISPEL_GOOD = 46,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_GROUP_ARMOR = 47,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_GROUP_HEAL = 48,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_GROUP_RECALL = 49,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_INFRAVISION = 50,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_WATERWALK = 51,	/* Reserved Skill[] DO NOT CHANGE */
    SPELL_MANA_SHIELD = 52,
    SPELL_IDENTIFY = 53,
    SPELL_GLOWLIGHT = 54,
    SPELL_BLUR = 55,
    SPELL_KNOCK = 56,
    SPELL_DISPEL_MAGIC = 57,
    SPELL_WARDING_SIGIL = 58,
    SPELL_DIMENSION_DOOR = 59,
    SPELL_MINOR_CREATION = 60,
    SPELL_TELEKINESIS = 61,
    SPELL_SWORD = 62,
    SPELL_WORD_STUN = 63,
    SPELL_PRISMATIC_SPRAY = 64,
    SPELL_FIRE_SHIELD = 65,
    SPELL_DETECT_SCRYING = 66,
    SPELL_CLAIRVOYANCE = 67,
    SPELL_SLOW = 68,
    SPELL_GREATER_ENCHANT = 69,
    SPELL_ENCHANT_ARMOR = 70,
    SPELL_MINOR_IDENTIFY = 71,
    SPELL_FLY = 72,
    SPELL_GREATER_HEAL = 73,
    SPELL_CONE_COLD = 74,
    SPELL_TRUE_SEEING = 75,
    SPELL_PROT_FROM_GOOD = 76,
    SPELL_MAGICAL_PROT = 77,
    SPELL_UNDEAD_PROT = 78,
    SPELL_SPIRIT_HAMMER = 79,
    SPELL_PRAY = 80,
    SPELL_FLAME_STRIKE = 81,
    SPELL_ENDURE_COLD = 82,
    SPELL_MAGICAL_VESTMENT = 83,
    SPELL_REJUVENATE = 84,
    SPELL_REGENERATE = 85,
    SPELL_COMMAND = 86,
    SPELL_AIR_WALK = 87,
    SPELL_DIVINE_ILLUMINATION = 88,
    SPELL_GOODBERRY = 89,
    SPELL_BARKSKIN = 90,
    SPELL_INVIS_TO_UNDEAD = 91,
    SPELL_HASTE = 92,
    SPELL_ANIMAL_KIN = 93,
    SPELL_CHARM_ANIMAL = 94,
    SPELL_REFRESH = 95,
    SPELL_BREATHE_WATER = 96,
    SPELL_CONJURE_ELEMENTAL = 97,
    SPELL_GREATER_INVIS = 98,
    SPELL_PROT_FROM_LIGHTNING = 99,
    SPELL_PROT_FROM_FIRE = 100,
    SPELL_RESTORATION = 101,
    SPELL_WORD_OF_INTELLECT = 102,
    SPELL_GUST_OF_WIND = 103,
    SPELL_RETRIEVE_CORPSE = 104,
    SPELL_LOCAL_TELEPORT = 105,
    SPELL_DISPLACEMENT = 106,
    SPELL_PEER = 107,
    SPELL_METEOR_STORM = 108,
    SPELL_PROTECT_FROM_DEVILS = 109,
    SPELL_SYMBOL_OF_PAIN = 110,
    SPELL_ICY_BLAST = 111,
    SPELL_ASTRAL_SPELL = 112,
    SPELL_VESTIGIAL_RUNE = 113,
    SPELL_DISRUPTION = 114,
    SPELL_STONE_TO_FLESH = 115,
    SPELL_REMOVE_SICKNESS = 116,
    SPELL_SHROUD_OBSCUREMENT = 117,
    SPELL_PRISMATIC_SPHERE = 118,
    SPELL_WALL_OF_THORNS = 119,
    SPELL_WALL_OF_STONE = 120,
    SPELL_WALL_OF_ICE = 121,
    SPELL_WALL_OF_FIRE = 122,
    SPELL_WALL_OF_IRON = 123,
    SPELL_THORN_TRAP = 124,
    SPELL_FIERY_SHEET = 125,
    SPELL_CHAIN_LIGHTNING = 126,
    SPELL_HAILSTORM = 127,
    SPELL_ICE_STORM = 128,
    SPELL_SHIELD_OF_RIGHTEOUSNESS = 129,	// group protection
    SPELL_BLACKMANTLE = 130,	// blocks healing spells
    SPELL_SANCTIFICATION = 131,	// 2x dam vs. evil
    SPELL_STIGMATA = 132,	// causes a bleeding wound
    SPELL_SUMMON_LEGION = 133,	// knights summon devils
    SPELL_ENTANGLE = 134,	// rangers entangle in veg.
    SPELL_ANTI_MAGIC_SHELL = 135,
    SPELL_DIVINE_INTERVENTION = 136,
    SPELL_SPHERE_OF_DESECRATION = 137,
    SPELL_MALEFIC_VIOLATION = 138,	// cuts thru good sanct
    SPELL_RIGHTEOUS_PENETRATION = 139,	// cuts thru evil sanct
    SPELL_UNHOLY_STALKER = 140,	// evil cleric hunter mob
    SPELL_INFERNO = 141,	// evil cleric room affect
    SPELL_VAMPIRIC_REGENERATION = 142,	// evil cleric vamp. regen
    SPELL_BANISHMENT = 143,	// evil cleric sends devils away
    SPELL_CONTROL_UNDEAD = 144,	// evil clerics charm undead
    SPELL_STONESKIN = 145,	// remort rangers stone skin
    SPELL_SUN_RAY = 146,	// Good cleric remort,
											 // destroys undead.
    SPELL_TAINT = 147,	// Evil knight remort spell, taint.
    SPELL_LOCUST_REGENERATION = 148,	// Mage remort skill, drains mana
    SPELL_DIVINE_POWER = 149,	// Good cleric remort skill.
    SPELL_DEATH_KNELL = 150,	// Evil cleric remort skill.
    SPELL_TELEPATHY = 151,
    SPELL_DAMN = 152,
    SPELL_CALM = 153,
    SPELL_THORN_SKIN = 154,
    SPELL_ENVENOM = 155,
    SPELL_ELEMENTAL_BRAND = 156,
    SPELL_THORN_SKIN_CASTING = 157,

    SPELL_FIRE_BREATHING = 158,
    SPELL_FROST_BREATHING = 159,
    SPELL_FLAME_OF_FAITH = 160,
    SPELL_ABLAZE = 161, // Only here to allow an ablaze affect

  /************************** Psionic Triggers ***************/
    SPELL_POWER = 201,	/* Strength                */
    SPELL_INTELLECT = 202,
    SPELL_CONFUSION = 203,
    SPELL_FEAR = 204,
    SPELL_SATIATION = 205,	/* fills hunger */
    SPELL_QUENCH = 206,	/* fills thirst */
    SPELL_CONFIDENCE = 207,	/* sets nopain */
    SPELL_NOPAIN = 208,
    SPELL_DERMAL_HARDENING = 209,
    SPELL_WOUND_CLOSURE = 210,
    SPELL_ANTIBODY = 211,
    SPELL_RETINA = 212,
    SPELL_ADRENALINE = 213,
    SPELL_BREATHING_STASIS = 214,
    SPELL_VERTIGO = 215,
    SPELL_METABOLISM = 216,	/* Increased healing, hunger, thirst */
    SPELL_EGO_WHIP = 217,
    SPELL_PSYCHIC_CRUSH = 218,
    SPELL_RELAXATION = 219,	/* speeds mana regen, weakens char */
    SPELL_WEAKNESS = 220,	/* minus str */
    SPELL_FORTRESS_OF_WILL = 221,
    SPELL_CELL_REGEN = 222,
    SPELL_PSISHIELD = 223,
    SPELL_PSYCHIC_SURGE = 224,
    SPELL_PSYCHIC_CONDUIT = 225,
    SPELL_PSIONIC_SHATTER = 226,
    SPELL_ID_INSINUATION = 227,
    SPELL_MELATONIC_FLOOD = 228,
    SPELL_MOTOR_SPASM = 229,
    SPELL_PSYCHIC_RESISTANCE = 230,
    SPELL_MASS_HYSTERIA = 231,
    SPELL_GROUP_CONFIDENCE = 232,
    SPELL_CLUMSINESS = 233,
    SPELL_ENDURANCE = 234,
    SPELL_AMNESIA = 235,	// psi remorts
    SPELL_NULLPSI = 236,	// remove psi affects
    SPELL_PSIONIC_SHOCKWAVE = 237,
    SPELL_DISTRACTION = 238,
    SPELL_CALL_RODENT = 239,
    SPELL_CALL_BIRD = 240,
    SPELL_CALL_REPTILE = 241,
    SPELL_CALL_BEAST = 242,
    SPELL_CALL_PREDATOR = 243,
    SPELL_SPIRIT_TRACK = 244,
    SPELL_PSYCHIC_FEEDBACK = 245,

  /**************************  Mercenary Devices ******************/
    SPELL_DECOY = 237,
  /*************************** Physic Alterations *****************/
    SPELL_ACIDITY = 301,
    SPELL_ATTRACTION_FIELD = 302,
    SPELL_NUCLEAR_WASTELAND = 303,
    SPELL_FLUORESCE = 304,
    SPELL_GAMMA_RAY = 305,
    SPELL_HALFLIFE = 306,
    SPELL_MICROWAVE = 307,
    SPELL_OXIDIZE = 308,
    SPELL_RANDOM_COORDINATES = 309,	// random teleport
    SPELL_REPULSION_FIELD = 310,
    SPELL_TRANSMITTANCE = 311,	// transparency
    SPELL_SPACETIME_IMPRINT = 312,	// sets room as teleport spot
    SPELL_SPACETIME_RECALL = 313,	// teleports to imprint telep spot
    SPELL_TIME_WARP = 314,	// random teleport into other time
    SPELL_TIDAL_SPACEWARP = 315,	// fly
    SPELL_FISSION_BLAST = 316,	// full-room damage
    SPELL_REFRACTION = 317,	// like displacement
    SPELL_ELECTROSHIELD = 318,	// prot_lightning
    SPELL_VACUUM_SHROUD = 319,	// eliminates breathing and fire
    SPELL_DENSIFY = 320,	// increase weight of obj & char
    SPELL_CHEMICAL_STABILITY = 321,	// prevent/stop acidity
    SPELL_ENTROPY_FIELD = 322,	// drains move on victim (time effect)
    SPELL_GRAVITY_WELL = 323,	// time effect crushing damage
    SPELL_CAPACITANCE_BOOST = 324,	// increase maxmv
    SPELL_ELECTRIC_ARC = 325,	// lightning bolt
    SPELL_SONIC_BOOM = 326,	// area damage + wait state
    SPELL_LATTICE_HARDENING = 327,	// dermal hard or increase obj maxdam
    SPELL_NULLIFY = 328,	// like dispel magic
    SPELL_FORCE_WALL = 329,	// sets up an exit blocker
    SPELL_UNUSED_330 = 330,
    SPELL_PHASING = 331,	// invuln.
    SPELL_ABSORPTION_SHIELD = 332,	// works like mana shield
    SPELL_TEMPORAL_COMPRESSION = 333,	// works like haste
    SPELL_TEMPORAL_DILATION = 334,	// works like slow
    SPELL_GAUSS_SHIELD = 335,	// half damage from metal
    SPELL_ALBEDO_SHIELD = 336,	// reflects e/m attacks
    SPELL_THERMOSTATIC_FIELD = 337,	// sets prot_heat + end_cold
    SPELL_RADIOIMMUNITY = 338,	// sets prot_rad
    SPELL_TRANSDIMENSIONALITY = 339,	// randomly teleport to another plane
    SPELL_AREA_STASIS = 340,	// sets !phy room flag
    SPELL_ELECTROSTATIC_FIELD = 341,	// protective static field does damage to attackers
    SPELL_EMP_PULSE = 342,	// Shuts off devices, communicators
										// deactivats all cyborg programs
										// blocked by emp shield
    SPELL_QUANTUM_RIFT = 343,	// Shuts off devices, communicators
    SPELL_ITEM_REPULSION_FIELD = 344,
    SPELL_ITEM_ATTRACTION_FIELD = 345,
    SPELL_DIMENSIONAL_SHIFT = 498, //added at top of spells to avoid interference with future bard songs
    SPELL_DIMENSIONAL_VOID = 499, //This is the negative affect thrown by dimensional shift

// *********************** Bard songs HERE man

    SONG_INSTANT_AUDIENCE = 346, // conjures an audience, like summon elem
    SONG_WALL_OF_SOUND = 347, // seals an exit, broken by shatter
    SONG_LAMENT_OF_LONGING = 349, // creates a portal to another character
    SONG_MISDIRECTION_MELISMA = 350, // misleads a tracker
    SONG_ARIA_OF_ARMAMENT = 351, // Similar to armor, group spell
    SONG_LULLABY = 352, // puts a room to sleep
    SONG_VERSE_OF_VULNERABILITY = 353, // lowers AC of target
    SONG_EXPOSURE_OVERTURE = 354, // Area affect, causes targets to vis
    SONG_VERSE_OF_VIBRATION = 355, // motor spasm++
    SONG_REGALERS_RHAPSODY = 356, // caster and groupies get satiated
    SONG_MELODY_OF_METTLE = 357, // caster and group get con and maxhit
    SONG_LUSTRATION_MELISMA = 358, // caster and group cleansed of blindness, poison, sickness
    SONG_DEFENSE_DITTY = 359, // save spell, psi, psy based on gen
    SONG_ALRONS_ARIA = 360, // singer/group confidence
    SONG_SONG_SHIELD = 361, // self only, like psi block
    SONG_VERSE_OF_VALOR = 362, // self/group increase hitroll
    SONG_HYMN_OF_PEACE = 363, // stops fighting in room, counters req of rage
    SONG_SONG_OF_SILENCE = 364, // Area, disallow speaking, casting. singing
    SONG_DRIFTERS_DITTY = 365, // self/group increases move
    SONG_UNRAVELLING_DIAPASON = 366, //dispel magic
    SONG_RHAPSODY_OF_DEPRESSION = 367, // Area, slow all but grouped
    SONG_CHANT_OF_LIGHT = 368, // group, light and prot_cold
    SONG_ARIA_OF_ASYLUM = 369, // self/group/target up to 25 percent dam reduction
    SONG_WHITE_NOISE = 370, // single target, confusion
    SONG_RHYTHM_OF_RAGE = 371, // self only, berserk, counter = hymn of peace
    SONG_POWER_OVERTURE = 372, // self only, increase strength and hitroll
    SONG_GUIHARIAS_GLORY = 373, // self/group, + damroll
    SONG_SIRENS_SONG = 374, // single target, charm
    SONG_SONIC_DISRUPTION = 375, // area, medium damage
    SONG_MIRROR_IMAGE_MELODY = 376, // causes multiple images of the singer
    SONG_CLARIFYING_HARMONIES = 377, // identify
    SONG_UNLADEN_SWALLOW_SONG = 378, // group flight
    SONG_IRRESISTABLE_DANCE = 379, // Target, -hitroll
    SONG_RHYTHM_OF_ALARM = 380, // room affect, notifies the bard of person entering room
    SONG_RHAPSODY_OF_REMEDY = 381, // self/target, heal
    SONG_SHATTER = 382, // target; damage persons/objects, penetrate WALL O SOUND
    SONG_HOME_SWEET_HOME = 383, // recall
    SONG_WEIGHT_OF_THE_WORLD = 384, // self/group/target, like telekinesis
    SONG_PURPLE_HAZE = 385, // area, pauses fighting for short time
    SONG_WOUNDING_WHISPERS = 386, // self, like blade barrier
    SONG_DIRGE = 387, // area, high damage
    SONG_EAGLES_OVERTURE = 388, // self, group, target, increases cha
    SONG_GHOST_INSTRUMENT = 389, // causes instrument to replay next some played
    SONG_LICHS_LYRICS = 390, // area, only affects living creatures
    SONG_FORTISSIMO = 391, // increases damage of sonic attacks
    SONG_INSIDIOUS_RHYTHM = 392, // target, - int
    SONG_REQUIEM = 395, // allows bard song to affect undead at half potency

// if you add a new bard skill/song here, change the NUM_BARD_SONGS const in act.bard.cc

  /*********************  MONK ZENS  *******************/
    ZEN_HEALING = 401,
    ZEN_AWARENESS = 402,
    ZEN_OBLIVITY = 403,
    ZEN_MOTION = 404,
    ZEN_TRANSLOCATION = 405,
    ZEN_CELERITY = 406,
    ZEN_DISPASSION = 407,
    MAX_SPELLS = 500,

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
    SKILL_BACKSTAB = 501,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_BASH = 502,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_HIDE = 503,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_KICK = 504,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_PICK_LOCK = 505,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_PUNCH = 506,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_RESCUE = 507,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_SNEAK = 508,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_STEAL = 509,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_TRACK = 510,	/* Reserved Skill[] DO NOT CHANGE */
    SKILL_PILEDRIVE = 511,
    SKILL_SLEEPER = 512,
    SKILL_ELBOW = 513,
    SKILL_KNEE = 514,
    SKILL_BERSERK = 515,
    SKILL_STOMP = 516,
    SKILL_BODYSLAM = 517,
    SKILL_CHOKE = 518,
    SKILL_CLOTHESLINE = 519,
    SKILL_TAG = 520,
    SKILL_INTIMIDATE = 521,
    SKILL_SPEED_HEALING = 522,
    SKILL_STALK = 523,
    SKILL_HEARING = 524,
    SKILL_BEARHUG = 525,
    SKILL_BITE = 526,
    SKILL_DBL_ATTACK = 527,
    SKILL_BANDAGE = 528,
    SKILL_FIRSTAID = 529,
    SKILL_MEDIC = 530,
    SKILL_CONSIDER = 531,
    SKILL_GLANCE = 532,
    SKILL_HEADBUTT = 533,
    SKILL_GOUGE = 534,
    SKILL_STUN = 535,
    SKILL_FEIGN = 536,
    SKILL_CONCEAL = 537,
    SKILL_PLANT = 538,
    SKILL_HOTWIRE = 539,
    SKILL_SHOOT = 540,
    SKILL_BEHEAD = 541,
    SKILL_DISARM = 542,
    SKILL_SPINKICK = 543,
    SKILL_ROUNDHOUSE = 544,
    SKILL_SIDEKICK = 545,
    SKILL_SPINFIST = 546,
    SKILL_JAB = 547,
    SKILL_HOOK = 548,
    SKILL_SWEEPKICK = 549,
    SKILL_TRIP = 550,
    SKILL_UPPERCUT = 551,
    SKILL_GROINKICK = 552,
    SKILL_CLAW = 553,
    SKILL_RABBITPUNCH = 554,
    SKILL_IMPALE = 555,
    SKILL_PELE_KICK = 556,
    SKILL_LUNGE_PUNCH = 557,
    SKILL_TORNADO_KICK = 558,
    SKILL_CIRCLE = 559,
    SKILL_TRIPLE_ATTACK = 560,

  /*******************  MONK SKILLS  *******************/
    SKILL_PALM_STRIKE = 561,
    SKILL_THROAT_STRIKE = 562,
    SKILL_WHIRLWIND = 563,
    SKILL_HIP_TOSS = 564,
    SKILL_COMBO = 565,
    SKILL_DEATH_TOUCH = 566,
    SKILL_CRANE_KICK = 567,
    SKILL_RIDGEHAND = 568,
    SKILL_SCISSOR_KICK = 569,
    SKILL_PINCH_ALPHA = 570,
    SKILL_PINCH_BETA = 571,
    SKILL_PINCH_GAMMA = 572,
    SKILL_PINCH_DELTA = 573,
    SKILL_PINCH_EPSILON = 574,
    SKILL_PINCH_OMEGA = 575,
    SKILL_PINCH_ZETA = 576,
    SKILL_MEDITATE = 577,
    SKILL_KATA = 578,
    SKILL_EVASION = 579,

    SKILL_SECOND_WEAPON = 580,
    SKILL_SCANNING = 581,
    SKILL_CURE_DISEASE = 582,
    SKILL_BATTLE_CRY = 583,
    SKILL_AUTOPSY = 584,
    SKILL_TRANSMUTE = 585,	/* physic transmute objs */
    SKILL_METALWORKING = 586,
    SKILL_LEATHERWORKING = 587,
    SKILL_DEMOLITIONS = 588,
    SKILL_PSIBLAST = 589,
    SKILL_PSILOCATE = 590,
    SKILL_PSIDRAIN = 591,	/* drain mana from vict */
    SKILL_GUNSMITHING = 592,	/* repair gunz */
    SKILL_ELUSION = 593,	/* !track */
    SKILL_PISTOLWHIP = 594,
    SKILL_CROSSFACE = 595,	/* rifle whip */
    SKILL_WRENCH = 596,	/* break neck */
    SKILL_CRY_FROM_BEYOND = 597,
    SKILL_KIA = 598,
    SKILL_WORMHOLE = 599,	// physic's wormhole
    SKILL_LECTURE = 600,	// physic's boring-ass lecture

    SKILL_TURN = 601,	/* Cleric's turn               */
    SKILL_ANALYZE = 602,	/* Physic's analysis           */
    SKILL_EVALUATE = 603,	/* Physic's evaluation      */
    SKILL_HOLY_TOUCH = 604,	/* Knight's skill              */
    SKILL_NIGHT_VISION = 605,
    SKILL_EMPOWER = 606,
    SKILL_SWIMMING = 607,
    SKILL_THROWING = 608,
    SKILL_RIDING = 609,
    SKILL_PIPEMAKING = 610,	//Make a pipe!
    SKILL_CHARGE = 611,	// BANG!
    SKILL_COUNTER_ATTACK = 612,

  /*****************  CYBORG SKILLS  ********************/
    SKILL_RECONFIGURE = 613,	// Re-allocate stats
    SKILL_REBOOT = 614,	// Start over from scratch
    SKILL_MOTION_SENSOR = 615,	// Detect Entries into room
    SKILL_STASIS = 616,	// State of rapid healing
    SKILL_ENERGY_FIELD = 617,	// Protective field
    SKILL_REFLEX_BOOST = 618,	// Speeds up processes
    SKILL_POWER_BOOST = 619,	// Increases Strength
    SKILL_UNUSED_1 = 620,	//
    SKILL_FASTBOOT = 621,	// Reboots are faster
    SKILL_SELF_DESTRUCT = 622,	// Effective self destructs
    SKILL_UNUSED_2 = 623,	//
    SKILL_BIOSCAN = 624,	// Sense Life scan
    SKILL_DISCHARGE = 625,	// Discharge attack
    SKILL_SELFREPAIR = 626,	// Repair hit points
    SKILL_CYBOREPAIR = 627,	// Repair other borgs
    SKILL_OVERHAUL = 628,	// Overhaul other borgs
    SKILL_DAMAGE_CONTROL = 629,	// Damage Control System
    SKILL_ELECTRONICS = 630,	// Operation of Electronics
    SKILL_HACKING = 631,	// hack electronic systems
    SKILL_CYBERSCAN = 632,	// scan others for implants
    SKILL_CYBO_SURGERY = 633,	// implant objects
    SKILL_ENERGY_WEAPONS = 634,	// energy weapon use
    SKILL_PROJ_WEAPONS = 635,	// projectile weapon use
    SKILL_SPEED_LOADING = 636,	// speed load weapons
    SKILL_HYPERSCAN = 637,	// aware of hidden objs and traps
    SKILL_OVERDRAIN = 638,	// overdrain batteries
    SKILL_DE_ENERGIZE = 639,	// drain energy from chars
    SKILL_ASSIMILATE = 640,	// assimilate objects
    SKILL_RADIONEGATION = 641,	// immunity to radiation
    SKILL_IMPLANT_W = 642,	// Extra attacks with implant weapons.
    SKILL_ADV_IMPLANT_W = 643,	// ""
    SKILL_OFFENSIVE_POS = 644,	// Offensive Posturing
    SKILL_DEFENSIVE_POS = 645,	// Defensive Posturing
    SKILL_MELEE_COMBAT_TAC = 646,	// Melee combat tactics
    SKILL_NEURAL_BRIDGING = 647,	// Cogenic Neural Bridging
										// (Ambidextarity)
// Cyborg skills continue around 675

    SKILL_RETREAT = 648,	// controlled flee
    SKILL_DISGUISE = 649,	// look like a mob
    SKILL_AMBUSH = 650,	// surprise victim

    SKILL_CHEMISTRY = 651, // merc skill
    SKILL_ADVANCED_CYBO_SURGERY = 652,
    SKILL_SHIELD_SLAM = 653, //knight knockdown requiring shield
    SKILL_SPARE4 = 654,
    SKILL_BEGUILE = 655,
    SKILL_SPARE6 = 656,
    SKILL_SPARE7 = 657,
    SKILL_SPARE8 = 658,

//-------------  Hood Skillz... yeah. Just two
    SKILL_SNATCH = 667,
    SKILL_DRAG = 668,

//    SKILL_TAUNT = 669,

//------------  Mercenary Skills -------------------
    SKILL_HAMSTRING = 666,
    SKILL_SNIPE = 669,	// sniper skill for mercs
    SKILL_INFILTRATE = 670,	// merc skill, improvement on sneak
    SKILL_SHOULDER_THROW = 671,	// grounding skill between hiptoss
										 // and sweepkick

// Bard Skills
    SKILL_SCREAM = 672, // damage like psiblast, chance to stun
    SKILL_VENTRILOQUISM = 673, // makes objects talk
    SKILL_TUMBLING = 674, // like uncanny dodge
    SKILL_LINGERING_SONG = 676, // increases duration of song affects

// Overflow Cyborg
    SKILL_NANITE_RECONSTRUCTION = 675,	// repairs implants
//    SKILL_ARTERIAL_FLOW = 676,	// Arterial Flow Enhancements
    SKILL_OPTIMMUNAL_RESP = 677,	// Genetek Optimmunal Node
    SKILL_ADRENAL_MAXIMIZER = 678,	// Shukutei Adrenal Maximizer

    SKILL_ENERGY_CONVERSION = 679,	// physic's energy conversion

  /******************  PROFICENCIES  *******************/
    SKILL_PROF_POUND = 681,
    SKILL_PROF_WHIP = 682,
    SKILL_PROF_PIERCE = 683,
    SKILL_PROF_SLASH = 684,
    SKILL_PROF_CRUSH = 685,
    SKILL_PROF_BLAST = 686,
    SKILL_BREAK_DOOR = 687,
    SKILL_ARCHERY = 688,
    SKILL_BOW_FLETCH = 689,
    SKILL_READ_SCROLLS = 690,
    SKILL_USE_WANDS = 691,

/* New skills may be added here up to MAX_SKILLS (700) */
    SKILL_DISCIPLINE_OF_STEEL = 692,
    SKILL_STRIKE = 693,
    SKILL_CLEAVE = 694,
    SKILL_GREAT_CLEAVE = 695,
    SKILL_APPRAISE = 696,
    SKILL_GARROTE = 697,
    SKILL_SHIELD_MASTERY = 698,
    SKILL_UNCANNY_DODGE = 699,
/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

    SPELL_FIRE_BREATH = 702,
    SPELL_GAS_BREATH = 703,
    SPELL_FROST_BREATH = 704,
    SPELL_ACID_BREATH = 705,
    SPELL_LIGHTNING_BREATH = 706,
    TYPE_LAVA_BREATH = 707,
    SPELL_EARTH_ELEMENTAL = 711,
    SPELL_FIRE_ELEMENTAL = 712,
    SPELL_WATER_ELEMENTAL = 713,
    SPELL_AIR_ELEMENTAL = 714,
    SPELL_HELL_FIRE = 717,
    JAVELIN_OF_LIGHTNING = 718,
    SPELL_TROG_STENCH = 719,
    SPELL_MANTICORE_SPIKES = 720,
    TYPE_CATAPULT = 730,
    TYPE_BALISTA = 731,
    TYPE_BOILING_OIL = 732,
    TYPE_HOTSANDS = 733,
    SPELL_MANA_RESTORE = 734,
    SPELL_BLADE_BARRIER = 735,
    SPELL_SUMMON_MINOTAUR = 736,
    TYPE_STYGIAN_LIGHTNING = 737,
    SPELL_SKUNK_STENCH = 738,
    SPELL_PETRIFY = 739,
    SPELL_SICKNESS = 740,
    SPELL_ESSENCE_OF_EVIL = 741,
    SPELL_ESSENCE_OF_GOOD = 742,
    SPELL_SHRINKING = 743,
    SPELL_HELL_FROST = 744,
    TYPE_ALIEN_BLOOD = 745,
    TYPE_SURGERY = 746,
    SMOKE_EFFECTS = 747,
    SPELL_HELL_FIRE_STORM = 748,
    SPELL_HELL_FROST_STORM = 749,
    SPELL_SHADOW_BREATH = 750,
    SPELL_STEAM_BREATH = 751,
    TYPE_TRAMPLING = 752,
    TYPE_GORE_HORNS = 753,
    TYPE_TAIL_LASH = 754,
    TYPE_BOILING_PITCH = 755,
    TYPE_FALLING = 756,
    TYPE_HOLYOCEAN = 757,
    TYPE_FREEZING = 758,
    TYPE_DROWNING = 759,
    TYPE_ABLAZE = 760,
    SPELL_QUAD_DAMAGE = 761,
    TYPE_VADER_CHOKE = 762,
    TYPE_ACID_BURN = 763,
    SPELL_ZHENGIS_FIST_OF_ANNIHILATION = 764,
    TYPE_RAD_SICKNESS = 765,
    TYPE_FLAMETHROWER = 766,
    TYPE_MALOVENT_HOLYTOUCH = 767,	// When holytouch wears off.
    SPELL_YOUTH = 768,
    TYPE_SWALLOW = 769,
    TOP_NPC_SPELL = 770,

    TOP_SPELL_DEFINE = 799,
/* NEW NPC/OBJECT SPELLS can be inserted here up to 799 */
};

/* WEAPON ATTACK TYPES */
enum attack {
    TYPE_HIT = 800,
    TYPE_STING = 801,
    TYPE_WHIP = 802,
    TYPE_SLASH = 803,
    TYPE_BITE = 804,
    TYPE_BLUDGEON = 805,
    TYPE_CRUSH = 806,
    TYPE_POUND = 807,
    TYPE_CLAW = 808,
    TYPE_MAUL = 809,
    TYPE_THRASH = 810,
    TYPE_PIERCE = 811,
    TYPE_BLAST = 812,
    TYPE_PUNCH = 813,
    TYPE_STAB = 814,
    TYPE_ENERGY_GUN = 815,
    TYPE_RIP = 816,
    TYPE_CHOP = 817,
    TYPE_SHOOT = 818,

    TOP_ATTACKTYPE = 819,
/* new attack types can be added here - up to TYPE_SUFFERING */

//energy weapon attack types
    TYPE_EGUN_LASER = 820,
    TYPE_EGUN_PLASMA = 821,
    TYPE_EGUN_ION = 822,
    TYPE_EGUN_PHOTON = 823,
    TYPE_EGUN_SONIC = 824,
    TYPE_EGUN_PARTICLE = 825,
    TYPE_EGUN_GAMMA = 826,
    TYPE_EGUN_LIGHTNING = 827,
    TYPE_EGUN_TOP = 828,

//energy weapon spec types
    TYPE_EGUN_SPEC_LIGHTNING = 831,
    TYPE_EGUN_SPEC_SONIC = 832,

    TYPE_CRUSHING_DEPTH = 892,	// in deep ocean without vehicle
    TYPE_TAINT_BURN = 893,	// casting while tainted
    TYPE_PRESSURE = 894,
    TYPE_SUFFOCATING = 895,
    TYPE_ANGUISH = 896,	// Soulless and good aligned. dumbass.
    TYPE_BLEED = 897,	// Open wound
    TYPE_OVERLOAD = 898,	// cyborg overloading systems.
    TYPE_SUFFERING = 899,
    TOP_DAMAGETYPE = 900,
};

enum saving_throw {
    SAVING_PARA = 0,
    SAVING_ROD = 1,
    SAVING_PETRI = 2,
    SAVING_BREATH = 3,
    SAVING_SPELL = 4,
    SAVING_CHEM = 5,
    SAVING_PSI = 6,
    SAVING_PHY = 7,
    SAVING_NONE = 50,
};

enum target_flag {
    TAR_IGNORE = 1,
    TAR_CHAR_ROOM = 2,
    TAR_CHAR_WORLD = 4,
    TAR_FIGHT_SELF = 8,
    TAR_FIGHT_VICT = 16,
    TAR_SELF_ONLY = 32,	/* Only a check, use with i.e. TAR_CHAR_ROOM */
    TAR_NOT_SELF = 64,	/* Only a check, use with i.e. TAR_CHAR_ROOM */
    TAR_OBJ_INV = 128,
    TAR_OBJ_ROOM = 256,
    TAR_OBJ_WORLD = 512,
    TAR_OBJ_EQUIP = 1024,
    TAR_DOOR = 2048,
    TAR_UNPLEASANT = 4096,
    TAR_DIR = 8192,
};

struct spell_info_type {
	char min_position;			/* Position for caster   */
	int16_t mana_min;			/* Min amount of mana used by a spell (highest lev) */
	int16_t mana_max;			/* Max amount of mana used by a spell (lowest lev) */
	char mana_change;			/* Change in mana used by spell from lev to lev */

	char min_level[NUM_CLASSES];
	char gen[NUM_CLASSES];
	int routines;
	int16_t targets;				/* See below for use with TAR_XXX  */
	bool violent;
        bool is_weapon;
        bool defensive;
};

struct bard_song {
    char *lyrics; // The lyrics or song description
    bool instrumental; // is it an instrumental
    int type; // what type of instrument do we need?
};

extern struct spell_info_type spell_info[];
extern struct bard_song songs[];

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
enum spell_type {
    SPELL_TYPE_SPELL = 0,
    SPELL_TYPE_POTION = 1,
    SPELL_TYPE_WAND = 2,
    SPELL_TYPE_STAFF = 3,
    SPELL_TYPE_SCROLL = 4,
};

/* Attacktypes with grammar */

struct attack_hit_type {
	const char *singular;
	const char *plural;
};

struct gun_hit_type {
    const char *singular;
    const char *plural;
    const char *substance;
};

#define ASPELL(spellname) \
	void spellname(__attribute__ ((unused)) uint8_t level, \
		__attribute__ ((unused)) struct creature *ch, \
		__attribute__ ((unused))  struct creature *victim, \
		__attribute__ ((unused)) struct obj_data *obj, \
		__attribute__ ((unused)) int *dir)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict, dvict);

static inline int SPELL_LEVEL( int spell, int char_class ) {
	return (int)spell_info[spell].min_level[char_class];
}
static inline int SPELL_GEN( int spell, int char_class ) {
    return (int)spell_info[spell].gen[char_class];
}
static inline bool IS_WEAPON(int spell) {
    return spell_info[spell].is_weapon;
}

bool is_able_to_learn(struct creature *ch, int spl);

ASPELL(spell_astral_spell);
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
ASPELL(spell_death_knell);
ASPELL(spell_dispel_magic);
ASPELL(spell_distraction);
ASPELL(spell_bless);
ASPELL(spell_damn);
ASPELL(spell_calm);
ASPELL(spell_call_rodent);
ASPELL(spell_call_bird);
ASPELL(spell_call_reptile);
ASPELL(spell_call_beast);
ASPELL(spell_call_predator);
ASPELL(spell_dispel_evil);
ASPELL(spell_dispel_good);
ASPELL(song_exposure_overture);
ASPELL(song_lament_of_longing);
ASPELL(song_unravelling_diapason);
ASPELL(song_instant_audience);
ASPELL(song_rhythm_of_alarm);
ASPELL(song_wall_of_sound);
ASPELL(song_hymn_of_peace);

/* basic magic calling functions */

int find_skill_num(char *name);

bool can_cast_spell(struct creature *ch, int spellnum);

int mag_damage(int level, struct creature *ch, struct creature *victim,
	int spellnum, int savetype);

int mag_exits(int level, struct creature *caster, struct room_data *room,
	int spellnum);

void mag_affects(int level, struct creature *ch, struct creature *victim,
	int *dir, int spellnum, int savetype);

void mag_group_switch(int level, struct creature *ch, struct creature *tch,
	int spellnum, int savetype);

void mag_groups(int level, struct creature *ch, int spellnum, int savetype);

void mag_masses(int8_t level, struct creature *ch, int spellnum, int savetype);

int mag_areas(int8_t level, struct creature *ch, int spellnum, int savetype);

void mag_summons(int level, struct creature *ch, struct obj_data *obj,
	int spellnum, int savetype);

void mag_points(int level, struct creature *ch, struct creature *victim,
	int *dir, int spellnum, int savetype);

void mag_unaffects(int level, struct creature *ch, struct creature *victim,
	int spellnum, int type);

void mag_alter_objs(int level, struct creature *ch, struct obj_data *obj,
	int spellnum, int type);

void mag_creations(int level, struct creature *ch, int spellnum);

int call_magic(struct creature *caster, struct creature *cvict,
	struct obj_data *ovict, int *dvict, int spellnum, int level, int casttype);

int mag_objectmagic(struct creature *ch, struct obj_data *obj,
	char *argument);

void mag_objects(int level, struct creature *ch, struct obj_data *obj,
	int spellnum);

int cast_spell(struct creature *ch, struct creature *tch,
	struct obj_data *tobj, int *tdir, int spellnum);

bool mag_savingthrow(struct creature *ch, int level, int type);

int mag_manacost(struct creature *ch, int spellnum);

#endif
