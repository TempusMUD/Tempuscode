#ifndef _CREATURE_H_
#define _CREATURE_H_

//
// File: creature.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/* char and mob-related defines *****************************************/

enum {
    ATTR_STR,
    ATTR_INT,
    ATTR_WIS,
    ATTR_DEX,
    ATTR_CON,
    ATTR_CHA,
};

/* Rent codes */
enum rent_codes {
    RENT_UNDEF = 0,
    RENT_CRASH = 1,
    RENT_RENTED = 2,
    RENT_CRYO = 3,
    RENT_FORCED = 4,
    RENT_QUIT = 5,
    RENT_NEW_CHAR = 6,
    RENT_CREATING = 7,
    RENT_REMORTING = 8,
};

/* PC char_classes */
enum char_class {
    CLASS_HELP = -2,
    CLASS_UNDEFINED = -1,
    CLASS_NONE = -1,
    CLASS_MAGIC_USER = 0,
    CLASS_MAGE = CLASS_MAGIC_USER,
    CLASS_CLERIC = 1,
    CLASS_THIEF = 2,
    CLASS_WARRIOR = 3,
    CLASS_BARB = 4,
    CLASS_PSIONIC = 5,	/* F */
    CLASS_PHYSIC = 6,	/* F */
    CLASS_CYBORG = 7,	/* F */
    CLASS_KNIGHT = 8,
    CLASS_RANGER = 9,
    CLASS_BARD = 10,	/* N */
    CLASS_MONK = 11,
    CLASS_VAMPIRE = 12,
    CLASS_MERCENARY = 13,
    CLASS_SPARE1 = 14,
    CLASS_SPARE2 = 15,
    CLASS_SPARE3 = 16,

    NUM_CLASSES = 17,	/* This must be the number of char_classes!! */
    CLASS_NORMAL = 50,
    CLASS_BIRD = 51,
    CLASS_PREDATOR = 52,
    CLASS_SNAKE = 53,
    CLASS_HORSE = 54,
    CLASS_SMALL = 55,
    CLASS_MEDIUM = 56,
    CLASS_LARGE = 57,
    CLASS_SCIENTIST = 58,
    CLASS_SKELETON = 60,
    CLASS_GHOUL = 61,
    CLASS_SHADOW = 62,
    CLASS_WIGHT = 63,
    CLASS_WRAITH = 64,
    CLASS_MUMMY = 65,
    CLASS_SPECTRE = 66,
    CLASS_NPC_VAMPIRE = 67,
    CLASS_GHOST = 68,
    CLASS_LICH = 69,
    CLASS_ZOMBIE = 70,

    CLASS_EARTH = 81,	/* Elementals */
    CLASS_FIRE = 82,
    CLASS_WATER = 83,
    CLASS_AIR = 84,
    CLASS_LIGHTNING = 85,
    CLASS_GREEN = 91,	/* Dragons */
    CLASS_WHITE = 92,
    CLASS_BLACK = 93,
    CLASS_BLUE = 94,
    CLASS_RED = 95,
    CLASS_SILVER = 96,
    CLASS_SHADOW_D = 97,
    CLASS_DEEP = 98,
    CLASS_TURTLE = 99,
    CLASS_LEAST = 101,	/* Devils  */
    CLASS_LESSER = 102,
    CLASS_GREATER = 103,
    CLASS_DUKE = 104,
    CLASS_ARCH = 105,
    CLASS_HILL = 111,
    CLASS_STONE = 112,
    CLASS_FROST = 113,
    CLASS_FIRE_G = 114,
    CLASS_CLOUD = 115,
    CLASS_STORM = 116,
    CLASS_SLAAD_RED = 120,	/* Slaad */
    CLASS_SLAAD_BLUE = 121,
    CLASS_SLAAD_GREEN = 122,
    CLASS_SLAAD_GREY = 123,
    CLASS_SLAAD_DEATH = 124,
    CLASS_SLAAD_LORD = 125,
    CLASS_DEMON_I = 130,	/* Demons of the Abyss */
    CLASS_DEMON_II = 131,
    CLASS_DEMON_III = 132,
    CLASS_DEMON_IV = 133,
    CLASS_DEMON_V = 134,
    CLASS_DEMON_VI = 135,
    CLASS_DEMON_SEMI = 136,
    CLASS_DEMON_MINOR = 137,
    CLASS_DEMON_MAJOR = 138,
    CLASS_DEMON_LORD = 139,
    CLASS_DEMON_PRINCE = 140,
    CLASS_DEVA_ASTRAL = 150,
    CLASS_DEVA_MONADIC = 151,
    CLASS_DEVA_MOVANIC = 152,
    CLASS_MEPHIT_FIRE = 160,
    CLASS_MEPHIT_LAVA = 161,
    CLASS_MEPHIT_SMOKE = 162,
    CLASS_MEPHIT_STEAM = 163,
    CLASS_DAEMON_ARCANA = 170,	// daemons
    CLASS_DAEMON_CHARONA = 171,
    CLASS_DAEMON_DERGHO = 172,
    CLASS_DAEMON_HYDRO = 173,
    CLASS_DAEMON_PISCO = 174,
    CLASS_DAEMON_ULTRO = 175,
    CLASS_DAEMON_YAGNO = 176,
    CLASS_DAEMON_PYRO = 177,
    CLASS_GODLING = 178,
    CLASS_DIETY = 179,

    TOP_CLASS = 180,
};

// Hometown defines
enum hometown {
    HOME_UNDEFINED = -1,
    HOME_MODRIAN = 0,
    HOME_NEW_THALOS = 1,
    HOME_ELECTRO = 2,
    HOME_ELVEN_VILLAGE = 3,
    HOME_ISTAN = 4,
    HOME_MONK = 5,
    HOME_NEWBIE_TOWER = 6,
    HOME_SOLACE_COVE = 7,
    HOME_MAVERNAL = 8,
    HOME_ZUL_DANE = 9,
    HOME_ARENA = 10,
    HOME_CITY = 11,
    HOME_DOOM = 12,
    HOME_SKULLPORT = 15,
    HOME_DWARVEN_CAVERNS = 16,
    HOME_HUMAN_SQUARE = 17,
    HOME_DROW_ISLE = 18,
    HOME_ASTRAL_MANSE = 19,
    HOME_SKULLPORT_NEWBIE = 20,
    HOME_NEWBIE_SCHOOL = 21,
    HOME_KROMGUARD = 22,
    NUM_HOMETOWNS = 23,
};

/* Deity Defines                */
enum deity {
    DEITY_NONE = 0,
    DEITY_GUIHARIA = 1,
    DEITY_PAN = 2,
    DEITY_JUSTICE = 3,
    DEITY_ARES = 4,
    DEITY_KALAR = 5,
};

/* Sex */
enum sex {
    SEX_NEUTRAL = 0,
    SEX_MALE = 1,
    SEX_FEMALE = 2,
};

/* Positions */
enum position {
    BOTTOM_POS = 0,
    POS_DEAD = 0,	/* dead            */
    POS_MORTALLYW = 1,	/* mortally wounded    */
    POS_INCAP = 2,	/* incapacitated    */
    POS_STUNNED = 3,	/* stunned        */
    POS_SLEEPING = 4,	/* sleeping        */
    POS_RESTING = 5,	/* resting        */
    POS_SITTING = 6,	/* sitting        */
    POS_FIGHTING = 7,	/* fighting        */
    POS_STANDING = 8,	/* standing        */
    POS_FLYING = 9,	/* flying around        */
    POS_MOUNTED = 10,
    POS_SWIMMING = 11,
    TOP_POS = 11,
};

/* Player flags: used by struct creature.char_specials.act */
enum plr_flag {
    PLR_HARDCORE = (1 << 0),	/* Player is a hardcore character    */
    PLR_UNUSED1 = (1 << 1),
    PLR_FROZEN = (1 << 2),	/* Player is frozen            */
    PLR_DONTSET = (1 << 3),	/* Don't EVER set (ISNPC bit)    */
    PLR_WRITING = (1 << 4),	/* Player writing (board/mail/olc)    */
    PLR_MAILING = (1 << 5),	/* Player is writing mail        */
    PLR_CRASH = (1 << 6),	/* Player needs to be crash-saved    */
    PLR_SITEOK = (1 << 7),	/* Player has been site-cleared    */
    PLR_NOSHOUT = (1 << 8),	/* Player not allowed to shout/goss    */
    PLR_NOTITLE = (1 << 9),	/* Player not allowed to set title    */
    PLR_UNUSED2 = (1 << 10),
    PLR_UNUSED3 = (1 << 11),
    PLR_NOCLANMAIL = (1 << 12),	/* Player doesn't get clanmail    */
    PLR_NODELETE = (1 << 13),	/* Player shouldn't be deleted    */
    PLR_INVSTART = (1 << 14),	/* Player should enter game wizinvis    */
    PLR_CRYO = (1 << 15),	/* Player is cryo-saved (purge prog)    */
    PLR_AFK = (1 << 16),	/* Player is away from keyboard      */
    PLR_CLAN_LEADER = (1 << 17),	/* The head of the respective clan   */
    PLR_UNUSED4 = (1 << 18),
    PLR_OLC = (1 << 19),	/* Player is descripting olc         */
    PLR_HALT = (1 << 20),	/* Player is halted                  */
    PLR_OLCGOD = (1 << 21),	/* Player can edit at will           */
    PLR_TESTER = (1 << 22),	/* Player is a tester                */
    PLR_UNUSED5 = (1 << 23),
    PLR_MORTALIZED = (1 << 24),	/* God can be killed                 */
    PLR_UNUSED6 = (1 << 25),
    PLR_UNUSED7 = (1 << 26),
    PLR_NOPOST = (1 << 27),
    PLR_LOG = (1 << 28),	/* log all cmds */
    PLR_UNUSED8 = (1 << 29),
    PLR_NOPK = (1 << 30),	/* player cannot pk */
};

// Player Flags Mark II
enum plr2_flag {
    PLR2_SOULLESS = (1 << 0),	// Signing the Unholy Compact.
    PLR2_BURIED = (1 << 1),	// Player has died way too many times.
    PLR2_IN_COMBAT = (1 << 2),
};

/* Mobile flags: used by struct creature.char_specials.act */
enum mob_flag {
    NPC_SPEC = (1 << 0),	/* Mob has a callable spec-proc    */
    NPC_SENTINEL = (1 << 1),	/* Mob should not move        */
    NPC_SCAVENGER = (1 << 2),	/* Mob picks up stuff on the ground    */
    NPC_ISNPC = (1 << 3),	/* (R) Automatically set on all Mobs    */
    NPC_AWARE = (1 << 4),	/* Mob can't be backstabbed        */
    NPC_AGGRESSIVE = (1 << 5),	/* Mob hits players in the room    */
    NPC_STAY_ZONE = (1 << 6),	/* Mob shouldn't wander out of zone    */
    NPC_WIMPY = (1 << 7),	/* Mob flees if severely injured    */
    NPC_AGGR_EVIL = (1 << 8),	/* auto attack evil PC's        */
    NPC_AGGR_GOOD = (1 << 9),	/* auto attack good PC's        */
    NPC_AGGR_NEUTRAL = (1 << 10),	/* auto attack neutral PC's        */
    NPC_MEMORY = (1 << 11),	/* remember attackers if attacked    */
    NPC_HELPER = (1 << 12),	/* attack PCs fighting other NPCs    */
    NPC_NOCHARM = (1 << 13),	/* Mob can't be charmed        */
    NPC_NOSUMMON = (1 << 14),	/* Mob can't be summoned        */
    NPC_NOSLEEP = (1 << 15),	// Mob can't be slept
    NPC_NOBASH = (1 << 16),	// Mob can't be bashed (e.g. trees)
    NPC_NOBLIND = (1 << 17),	// Mob can't be blinded
    NPC_NOTURN = (1 << 18),	// Hard to turn
    NPC_NOPETRI = (1 << 19),	// Cannot be petrified
    NPC_PET = (1 << 20),	// Mob is a conjured pet and shouldn't
										 // get nor give any xp in any way.
    NPC_SOULLESS = (1 << 21),	// Mobile is Soulless - Unholy compact.
    NPC_SPIRIT_TRACKER = (1 << 22),	// Can track through !track
    NPC_UTILITY = (1 << 23), //Can't be seen, hit, etc...
#define NUM_NPC_FLAGS             24
};

enum mob2_flag {
    NPC2_SCRIPT = (1 << 0),
    NPC2_MOUNT = (1 << 1),
    NPC2_STAY_SECT = (1 << 2),	/* Can't leave SECT_type.   */
    NPC2_ATK_MOBS = (1 << 3),	/* Aggro Mobs will attack other mobs */
    NPC2_HUNT = (1 << 4),	/* Mob will hunt attacker    */
    NPC2_LOOTER = (1 << 5),	/* Loots corpses     */
    NPC2_NOSTUN = (1 << 6),
    NPC2_SELLER = (1 << 7),	/* If shopkeeper, sells anywhere. */
    NPC2_WONT_WEAR = (1 << 8),	/* Wont wear shit it picks up (SHPKPER) */
    NPC2_SILENT_HUNTER = (1 << 9),
    NPC2_FAMILIAR = (1 << 10),	/* mages familiar */
    NPC2_NO_FLOW = (1 << 11),	/* Mob doesn't flow */
    NPC2_UNAPPROVED = (1 << 12),	/* Mobile not approved for game play */
    NPC2_RENAMED = (1 << 13),	/* Mobile renamed */
    NPC2_NOAGGRO_RACE = (1 << 14),	/* wont attack members of own race */
#define NUM_NPC2_FLAGS            15
};

/* Preference flags: used by struct creature.player_specials.pref */
enum prf_flag {
    PRF_BRIEF = (1 << 0),	/* Room descs won't normally be shown    */
    PRF_NOHAGGLE = (1 << 1),
    PRF_DEAF = (1 << 2),	/* Can't hear shouts            */
    PRF_NOTELL = (1 << 3),	/* Can't receive tells        */
    PRF_DISPHP = (1 << 4),	/* Display hit points in prompt    */
    PRF_DISPMANA = (1 << 5),	/* Display mana points in prompt    */
    PRF_DISPMOVE = (1 << 6),	/* Display move points in prompt    */
    PRF_AUTOEXIT = (1 << 7),	/* Display exits in a room        */
    PRF_NOHASSLE = (1 << 8),	/* Aggr mobs won't attack        */
    PRF_NASTY = (1 << 9),	/* Can hear nasty words on channel */
    PRF_SUMMONABLE = (1 << 10),	/* Can be summoned            */
    PRF_NOPLUG = (1 << 11),	/* No repetition of comm commands    */
    PRF_HOLYLIGHT = (1 << 12),	/* Can see in dark            */
    PRF_COLOR_1 = (1 << 13),	/* Color (low bit)            */
    PRF_COLOR_2 = (1 << 14),	/* Color (high bit)            */
    PRF_NOWIZ = (1 << 15),	/* Can't hear wizline            */
    PRF_LOG1 = (1 << 16),	/* On-line System Log (low bit)    */
    PRF_LOG2 = (1 << 17),	/* On-line System Log (high bit)    */
    PRF_NOAUCT = (1 << 18),	/* Can't hear auction channel        */
    PRF_NOGOSS = (1 << 19),	/* Can't hear gossip channel        */
    PRF_NOGRATZ = (1 << 20),	/* Can't hear grats channel        */
    PRF_ROOMFLAGS = (1 << 21),	/* Can see room flags (ROOM_x)    */
    PRF_NOSNOOP = (1 << 22),	/* Can not be snooped by immortals    */
    PRF_NOMUSIC = (1 << 23),	/* Can't hear music channel            */
    PRF_NOSPEW = (1 << 24),	/* Can't hear spews            */
    PRF_GAGMISS = (1 << 25),	/* Doesn't see misses during fight    */
    PRF_NOPROJECT = (1 << 26),	/* Cannot hear the remort channel     */
    PRF_NOPETITION = (1 << 27),
    PRF_NOCLANSAY = (1 << 28),	/* Doesnt hear clan says and such     */
    PRF_NOIDENTIFY = (1 << 29),	/* Saving throw is made when id'd     */
    PRF_NODREAM = (1 << 30),
};

// PRF 2 Flags
enum prf2_flag {
    PRF2_DEBUG = (1 << 0),	/* Sees info on fight.              */
    PRF2_NEWBIE_HELPER = (1 << 1),	/* sees newbie arrivals             */
    PRF2_AUTO_DIAGNOSE = (1 << 2),	/* automatically see condition of enemy */
    PRF2_AUTOPAGE = (1 << 3),	/* Beeps when ch receives a tell    */
    PRF2_NOAFFECTS = (1 << 4),	/* Affects are not shown in score   */
    PRF2_NOHOLLER = (1 << 5),	/* Gods only                        */
    PRF2_NOIMMCHAT = (1 << 6),	/* Gods only                        */
    PRF2_UNUSED_1 = (1 << 7),
    PRF2_CLAN_HIDE = (1 << 8),	/* don't show badge in who list */
    PRF2_UNUSED_2 = (1 << 9),
    PRF2_AUTOPROMPT = (1 << 10),	/* always draw new prompt */
    PRF2_NOWHO = (1 << 11),	/* don't show in who */
    PRF2_ANONYMOUS = (1 << 12),	/* don't show char_class, level */
    PRF2_NOTRAILERS = (1 << 13),	/* don't show trailer affects */
    PRF2_VT100 = (1 << 14),	/* Players uses VT100 inferface */
    PRF2_AUTOSPLIT = (1 << 15),
    PRF2_AUTOLOOT = (1 << 16),
    PRF2_PKILLER = (1 << 17),	// player can attack other players
    PRF2_NOGECHO = (1 << 18),	// Silly Gecho things
    PRF2_UNUSED_3 = (1 << 19),
    PRF2_DISPALIGN = (1 << 20),
    PRF2_WORLDWRITE = (1 << 21), // allows worldwrite to work
    PRF2_NOGUILDSAY = (1 << 22),
    PRF2_DISPTIME = (1 << 23), //show localtime in the prompt
    PRF2_DISP_VNUMS = (1 << 24), //show vnums after items ldesc
};

/* Affect bits: used in struct creature.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
enum affect_bit {
    AFF_BLIND = (1 << 0),	/* (R) Char is blind        */
    AFF_INVISIBLE = (1 << 1),	/* Char is invisible        */
    AFF_DETECT_ALIGN = (1 << 2),	/* Char is sensitive to align */
    AFF_DETECT_INVIS = (1 << 3),	/* Char can see invis chars  */
    AFF_DETECT_MAGIC = (1 << 4),	/* Char is sensitive to magic */
    AFF_SENSE_LIFE = (1 << 5),	/* Char can sense hidden life */
    AFF_WATERWALK = (1 << 6),	/* Char can walk on water    */
    AFF_SANCTUARY = (1 << 7),	/* Char protected by sanct.    */
    AFF_GROUP = (1 << 8),	/* (R) Char is grouped    */
    AFF_CURSE = (1 << 9),	/* Char is cursed        */
    AFF_INFRAVISION = (1 << 10),	/* Char can see in dark    */
    AFF_POISON = (1 << 11),	/* (R) Char is poisoned    */
    AFF_PROTECT_EVIL = (1 << 12),	/* Char protected from evil  */
    AFF_PROTECT_GOOD = (1 << 13),	/* Char protected from good  */
    AFF_SLEEP = (1 << 14),	/* (R) Char magically asleep    */
    AFF_NOTRACK = (1 << 15),	/* Char can't be tracked    */
    AFF_INFLIGHT = (1 << 16),	/* Room for future expansion    */
    AFF_TIME_WARP = (1 << 17),	/* Room for future expansion    */
    AFF_SNEAK = (1 << 18),	/* Char can move quietly    */
    AFF_HIDE = (1 << 19),	/* Char is hidden        */
    AFF_WATERBREATH = (1 << 20),	/* Room for future expansion    */
    AFF_CHARM = (1 << 21),	/* Char is charmed        */
    AFF_CONFUSION = (1 << 22),	/* Char is confused         */
    AFF_NOPAIN = (1 << 23),	/* Char feels no pain    */
    AFF_RETINA = (1 << 24),	/* Char's retina is stimulated */
    AFF_ADRENALINE = (1 << 25),	/* Char's adrenaline is pumping */
    AFF_CONFIDENCE = (1 << 26),	/* Char is confident           */
    AFF_REJUV = (1 << 27),	/* Char is rejuvenating */
    AFF_REGEN = (1 << 28),	/* Body is regenerating */
    AFF_GLOWLIGHT = (1 << 29),	/* Light spell is operating   */
    AFF_BLUR = (1 << 30),	/* Blurry image */
#define NUM_AFF_FLAGS         31
};

enum aff2_bit {
    AFF2_FLUORESCENT = (1 << 0),
    AFF2_TRANSPARENT = (1 << 1),
    AFF2_SLOW = (1 << 2),
    AFF2_HASTE = (1 << 3),
    AFF2_MOUNTED = (1 << 4),	/*DO NOT SET THIS IN MOB FILE */
    AFF2_FIRE_SHIELD = (1 << 5),	/* affected by Fire Shield  */
    AFF2_BERSERK = (1 << 6),
    AFF2_INTIMIDATED = (1 << 7),
    AFF2_TRUE_SEEING = (1 << 8),
    AFF2_DIVINE_ILLUMINATION = (1 << 9),
    AFF2_PROTECT_UNDEAD = (1 << 10),
    AFF2_INVIS_TO_UNDEAD = (1 << 11),
    AFF2_ANIMAL_KIN = (1 << 12),
    AFF2_ENDURE_COLD = (1 << 13),
    AFF2_PARALYZED = (1 << 14),
    AFF2_PROT_LIGHTNING = (1 << 15),
    AFF2_PROT_FIRE = (1 << 16),
    AFF2_TELEKINESIS = (1 << 17),	/* Char can carry more stuff */
    AFF2_PROT_RAD = (1 << 18),	/* Enables Autoexits ! :)    */
    AFF2_ABLAZE = (1 << 19),
    AFF2_NECK_PROTECTED = (1 << 20),	/* Can't be beheaded         */
    AFF2_DISPLACEMENT = (1 << 21),
    AFF2_PROT_DEVILS = (1 << 22),
    AFF2_MEDITATE = (1 << 23),
    AFF2_EVADE = (1 << 24),
    AFF2_BLADE_BARRIER = (1 << 25),
    AFF2_OBLIVITY = (1 << 26),
    AFF2_ENERGY_FIELD = (1 << 27),
    AFF2_UNUSED = (1 << 28),
    AFF2_VERTIGO = (1 << 29),
    AFF2_PROT_DEMONS = (1 << 30),
#define NUM_AFF2_FLAGS          31
};

enum aff3_bit {
    AFF3_ATTRACTION_FIELD = (1 << 0),
    AFF3_ENERGY_LEAK = (1 << 1),
    AFF3_POISON_2 = (1 << 2),
    AFF3_POISON_3 = (1 << 3),
    AFF3_SICKNESS = (1 << 4),
    AFF3_SELF_DESTRUCT = (1 << 5),	/* Self-destruct sequence init */
    AFF3_DAMAGE_CONTROL = (1 << 6),	/* Damage control for cyborgs  */
    AFF3_STASIS = (1 << 7),	/* Borg is in static state     */
    AFF3_PRISMATIC_SPHERE = (1 << 8),	/* Defensive */
    AFF3_RADIOACTIVE = (1 << 9),
    AFF3_DETECT_POISON = (1 << 10),
    AFF3_MANA_TAP = (1 << 11),
    AFF3_ENERGY_TAP = (1 << 12),
    AFF3_SONIC_IMAGERY = (1 << 13),
    AFF3_SHROUD_OBSCUREMENT = (1 << 14),
    AFF3_NOBREATHE = (1 << 15),
    AFF3_PROT_HEAT = (1 << 16),
    AFF3_PSISHIELD = (1 << 17),
    AFF3_PSYCHIC_CRUSH = (1 << 18),
    AFF3_DOUBLE_DAMAGE = (1 << 19),
    AFF3_ACIDITY = (1 << 20),
    AFF3_HAMSTRUNG = (1 << 21),	// Bleeding badly from the leg
    AFF3_GRAVITY_WELL = (1 << 22),	// Pissed off a phyz and got hit by gravity well
    AFF3_SYMBOL_OF_PAIN = (1 << 23),	// Char's mind is burning with pain
    AFF3_EMP_SHIELD = (1 << 24),	// EMP SHIELDING
    AFF3_INST_AFF = (1 << 25),	// Affected by an instant affect
    AFF3_TAINTED = (1 << 27),	// Knight spell, "taint"
    AFF3_INFILTRATE = (1 << 28),	// Merc skill infiltrate
    AFF3_DIVINE_POWER = (1 << 29),
    AFF3_MANA_LEAK = (1 << 30),
#define NUM_AFF3_FLAGS                31
};

enum {
    ARRAY_AFF_1 = 1,
    ARRAY_AFF_2 = 2,
    ARRAY_AFF_3 = 3,
};

/* Character equipment positions: used as index for struct creature.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
enum wear_position {
    WEAR_LIGHT = 0,
    WEAR_FINGER_R = 1,
    WEAR_FINGER_L = 2,
    WEAR_NECK_1 = 3,
    WEAR_NECK_2 = 4,
    WEAR_BODY = 5,
    WEAR_HEAD = 6,
    WEAR_LEGS = 7,
    WEAR_FEET = 8,
    WEAR_HANDS = 9,
    WEAR_ARMS = 10,
    WEAR_SHIELD = 11,
    WEAR_ABOUT = 12,
    WEAR_WAIST = 13,
    WEAR_WRIST_R = 14,
    WEAR_WRIST_L = 15,
    WEAR_WIELD = 16,
    WEAR_HOLD = 17,
    WEAR_CROTCH = 18,
    WEAR_EYES = 19,
    WEAR_BACK = 20,
    WEAR_BELT = 21,
    WEAR_FACE = 22,
    WEAR_EAR_L = 23,
    WEAR_EAR_R = 24,
    WEAR_WIELD_2 = 25,
    WEAR_ASS = 26,
    NUM_WEARS = 27,	/* This must be the # of eq positions!! */
    WEAR_RANDOM = 28,
    WEAR_MSHIELD = 29, /* This is for mana shield messages just increase it if new wear positions are added */
};

/* Modifier constants used with obj affects ('A' fields) */
enum apply {
    APPLY_NONE = 0,	/* No effect            */
    APPLY_STR = 1,	/* Apply to strength        */
    APPLY_DEX = 2,	/* Apply to dexterity        */
    APPLY_INT = 3,	/* Apply to intellegence    */
    APPLY_WIS = 4,	/* Apply to wisdom        */
    APPLY_CON = 5,	/* Apply to constitution    */
    APPLY_CHA = 6,	/* Apply to charisma        */
    APPLY_CLASS = 7,	/* Reserved            */
    APPLY_LEVEL = 8,	/* Reserved            */
    APPLY_AGE = 9,	/* Apply to age            */
    APPLY_CHAR_WEIGHT = 10,	/* Apply to weight        */
    APPLY_CHAR_HEIGHT = 11,	/* Apply to height        */
    APPLY_MANA = 12,	/* Apply to max mana        */
    APPLY_HIT = 13,	/* Apply to max hit points    */
    APPLY_MOVE = 14,	/* Apply to max move points    */
    APPLY_GOLD = 15,	/* Reserved            */
    APPLY_EXP = 16,	/* Reserved            */
    APPLY_AC = 17,	/* Apply to Armor Class        */
    APPLY_HITROLL = 18,	/* Apply to hitroll        */
    APPLY_DAMROLL = 19,	/* Apply to damage roll        */
    APPLY_SAVING_PARA = 20,	/* Apply to save throw: paralz    */
    APPLY_SAVING_ROD = 21,	/* Apply to save throw: rods    */
    APPLY_SAVING_PETRI = 22,	/* Apply to save throw: petrif    */
    APPLY_SAVING_BREATH = 23,	/* Apply to save throw: breath    */
    APPLY_SAVING_SPELL = 24,	/* Apply to save throw: spells    */
    APPLY_SNEAK = 25,
    APPLY_HIDE = 26,
    APPLY_RACE = 27,
    APPLY_SEX = 28,
    APPLY_BACKSTAB = 29,
    APPLY_PICK_LOCK = 30,
    APPLY_PUNCH = 31,
    APPLY_SHOOT = 32,
    APPLY_KICK = 33,
    APPLY_TRACK = 34,
    APPLY_IMPALE = 35,
    APPLY_BEHEAD = 36,
    APPLY_THROWING = 37,
    APPLY_RIDING = 38,
    APPLY_TURN = 39,
    APPLY_SAVING_CHEM = 40,
    APPLY_SAVING_PSI = 41,
    APPLY_ALIGN = 42,
    APPLY_SAVING_PHY = 43,
    APPLY_CASTER = 44,	// special usage
    APPLY_WEAPONSPEED = 45,
    APPLY_DISGUISE = 46,
    APPLY_NOTHIRST = 47,
    APPLY_NOHUNGER = 48,
    APPLY_NODRUNK = 49,
    APPLY_SPEED = 50,
    NUM_APPLIES = 51,
};

/* other miscellaneous defines *******************************************/

/* Player conditions */
enum player_condition {
    DRUNK = 0,
    FULL = 1,
    THIRST = 2,
};

/* other #defined constants **********************************************/
enum immortal_level {
    LVL_GRIMP = 72,
    LVL_FISHBONE = 71,
    LVL_LUCIFER = 70,
    LVL_IMPL = 69,
    LVL_ENTITY = 68,
    LVL_ANCIENT = LVL_ENTITY,
    LVL_CREATOR = 67,
    LVL_GRGOD = 66,
    LVL_TIMEGOD = 65,
    LVL_DEITY = 64,
    LVL_GOD = 63,	/* Lesser God */
    LVL_ENERGY = 62,
    LVL_FORCE = 61,
    LVL_POWER = 60,
    LVL_BEING = 59,
    LVL_SPIRIT = 58,
    LVL_ELEMENT = 57,
    LVL_DEMI = 56,
    LVL_ETERNAL = 55,
    LVL_ETHEREAL = 54,
    LVL_LUMINARY = 53,
    LVL_BUILDER = 52,
    LVL_IMMORT = 51,
    LVL_AMBASSADOR = 50,

    LVL_FREEZE = LVL_IMMORT,
    LVL_CAN_BAN = LVL_GOD,
    LVL_VIOLENCE = LVL_POWER,
    LVL_LOGALL = LVL_CREATOR,
    LVL_CAN_RETURN = 10,
};
/* char-related structures ************************************************/

/* memory structure for characters */
struct memory_rec {
	long id;
	struct memory_rec *next;
};

/* These data contain information about a players time data */
struct time_data {
	time_t birth;//This represents the characters age
	time_t death;// when did we die
	time_t logon;// Time of the last logon (used to calculate played)
	time_t played;// This is the total accumulated time played in secs
};

enum {MAX_WEAPON_SPEC = 6};
typedef struct weapon_spec {
	int vnum;
	uint8_t level;
} weapon_spec;

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
    char *name;					/* PC / NPC s name (kill ...  )         */
	char *short_descr;			/* for NPC 'actions'                    */
	char *long_descr;			/* for 'look'                   */
	char *description;			/* Extra descriptions                   */
	char *title;				/* PC / NPC's title                     */
	int16_t char_class;			/* PC / NPC's char_class               */
	int16_t remort_char_class;	/* PC / NPC REMORT CLASS (-1 for none) */
	float weight;				/* PC / NPC's weight                    */
	int16_t height;				/* PC / NPC's height                    */
	int16_t hometown;			/* PC s Hometown (zone)                 */
	int8_t sex;					/* PC / NPC's sex                       */
	int8_t race;					/* PC / NPC's race                  */
	uint8_t level;			/* PC / NPC's level                     */
	int8_t age_adjust;			/* PC age adjust to maintain sanity     */
	struct time_data time;		/* PC's AGE in days                 */
};

/* Char's abilities. */
struct char_ability_data {
    int8_t str;                  /* 1 - 35 */
	int8_t intel;
	int8_t wis;
	int8_t dex;
	int8_t con;
	int8_t cha;
};

/* Char's points */
struct char_point_data {
    int mana;
	int max_mana;			/* Max move for PC/NPC               */
	int hit;
	int max_hit;				/* Max hit for PC/NPC                      */
	int move;
	int max_move;			/* Max move for PC/NPC                     */

	int16_t armor;				/* Internal -100..100, external -10..10 AC */
    money_t gold;					/* Money carried                           */
	money_t cash;					// cash on hand
	int exp;					/* The experience of the player            */

	int8_t hitroll;				/* Any bonus or penalty to the hit roll    */
	int8_t damroll;				/* Any bonus or penalty to the damage roll */
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 */
struct char_special_data_saved {
    int alignment;				/* +-1000 for alignments                */
	long idnum;					/* player's idnum; -1 for mobiles    */
	uint32_t act;					/* act flag for NPC's; player flag for PC's */
	uint32_t act2;
	uint32_t affected_by;			/* Bitvector for spells/skills affected by */
	uint32_t affected2_by;
	uint32_t affected3_by;
    uint8_t remort_generation;
	int16_t apply_saving_throw[10];	/* Saving throw (Bonuses)        */
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
	struct creature *defending;	/* Char defended by this char */
	struct creature *hunting;	/* Char hunted by this char        */
	struct creature *mounted;	/* creatures mounted ON this char        */

	float carry_weight;			/* Carried weight                     */
	float worn_weight;			/* Total weight equipped                */
	int timer;					/* Timer for update            */
	int meditate_timer;			/* How long has been meditating           */
	int cur_flow_pulse;			/* Keeps track of whether char has flowed */

	int8_t breath_count;			/* for breathing and falling              */
	int8_t fall_count;
	int8_t position;				/* Standing, fighting, sleeping, etc.    */
	int8_t carry_items;			/* Number of items carried        */
	int8_t weapon_proficiency;	/* Scale of learnedness of weapon prof.   */

	const char *mood_str;		/* Sets mood for $a in act() */
	struct char_special_data_saved saved;	/* constants saved in plrfile    */
};

static const size_t MAX_RECENT_KILLS = 100;
static const int EXPLORE_BONUS_KILL_LIMIT = 10;
static const int EXPLORE_BONUS_PERCENT = 25;

struct kill_record {
    int vnum, times;
};

enum grievance_kind { MURDER, THEFT, ATTACK };

struct grievance {
    time_t time;
    int player_id;
    int rep;
    enum grievance_kind grievance;
};

struct player_special_data_saved {
	int8_t skills[MAX_SKILLS + 1];	/* array of skills plus skill 0        */
	weapon_spec weap_spec[MAX_WEAPON_SPEC];
	int wimp_level;				/* Below this # of hit points, flee!    */
	int8_t freeze_level;			/* Level of god who froze char, if any    */
	int16_t invis_level;			/* level of invisibility        */
	room_num load_room;			/* Which room to place char in        */
	room_num home_room;
	uint32_t pref;					/* preference flags for PC's.        */
	uint32_t pref2;					/* 2nd pref flag                        */
	int8_t conditions[3];		/* Drunk, full, thirsty            */

	uint8_t clan;
	uint8_t broken_component;
	uint8_t imm_qp;
	uint8_t qlog_level;			// what level of awareness we have to qlog
	uint8_t speed;				// percentage of speedup
	uint8_t qp_allowance;			// Quest point allowance
	char badge[MAX_BADGE_LENGTH+1];
	int deity;
	int life_points;
	int pkills;
	int akills;
	int mobkills;
	int deaths;
	int old_char_class;			/* Type of borg, or char_class before vamprism. */
	int page_length;
	int total_dam;
	int columns;
	int hold_load_room;
	int quest_id;
	uint32_t plr2_bits;
	int reputation;
	int killer_severity;
	long mana_shield_low;
	long mana_shield_pct;
};

/*
 * Specials needed only by PCs, not NPCs.
 */
enum {MAX_IMPRINT_ROOMS = 6};

struct player_special_data {
    struct player_special_data_saved saved;
	char *poofin;				/* Description on arrival of a god.     */
	char *poofout;				/* Description upon a god's exit.       */
    char *afk_reason;           /* Reason the player is AFK */
    GList *afk_notifies; /* People who have sent tells while
                                  * player is AFK */
	struct alias_data *aliases;	/* Character's aliases            */
	long last_tell_from;			/* idnum of last tell from        */
	long last_tell_to;				/* idnum of last tell to */
	int imprint_rooms[MAX_IMPRINT_ROOMS];
    GList *recently_killed;
    GList *grievances;
	unsigned int soilage[NUM_WEARS];
	struct obj_data *olc_obj;	/* which obj being edited               */
	struct creature *olc_mob;	/* which mob being edited               */
	struct shop_data *olc_shop;	/* which shop being edited              */
	struct special_search_data *olc_srch;	/* which srch being edited */
	struct room_data *was_in_room;	/* location for linkdead people         */
	struct help_item *olc_help_item;
    int thaw_time;
    int freezer_id;
	int rentcode;
	int rent_per_day;
	enum cxn_state desc_mode;
	int rent_currency;
    GHashTable *tags;           /* unordered set of strings mapped to 0x1 */
};

struct mob_shared_data {
	int vnum;
	int number;
	int attack_type;			/* The Attack Type integer for NPC's     */
	int lair;					/* room the mob always returns to */
	int leader;					// mob vnum the mob always helps and follows
	int kills;
	int loaded;
    int voice;
	int8_t default_pos;			/* Default position for NPC              */
	int8_t damnodice;				/* The number of damage dice's           */
	int8_t damsizedice;			/* The size of the damage dice's         */
	int8_t morale;
	char *move_buf;				/* custom move buf */
	struct creature *proto;	/* pointer to prototype */
	SPECIAL((*func));
	char *func_param;			/* mobile's special parameter str */
	char *load_param;			/* mobile's on_load script */
	char *prog;
    unsigned char *progobj;
    size_t progobj_len;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    struct memory_rec *memory;			/* List of attackers to remember           */
	void *func_data;			// Mobile-specific data used for specials
	struct mob_shared_data *shared;
	int wait_state;				/* Wait state for bashed mobs           */
	int8_t last_direction;		/* The last direction the monster went     */
	long mob_idnum;		/* mobile's unique idnum */
};

/* An affect structure */
struct affected_type {
	int type;				/* The type of spell that caused this      */
	int duration;			/* For how long its effects will last      */
	int modifier;			/* This is added to apropriate ability     */
	int location;			/* Tells which ability to change(APPLY_XXX) */
	uint8_t level;
	uint8_t is_instant;
	long bitvector;				/* Tells which bits to set (AFF_XXX)       */
	int aff_index;
    long owner;             /* Who placed this affect on this struct creature */
	struct affected_type *next;
};

/* Structure used for chars following other chars */
struct follow_type {
	struct creature *follower;
	struct follow_type *next;
};

struct char_language_data {
    GList *languages_heard;
    int8_t tongues[MAX_TONGUES];
    char current_language;
};

/* ================== Structure for player/non-player ===================== */
struct creature {
	struct room_data *in_room;	/* Location (real room number)      */

    GList *fighting; /* list of combats for this char */
	struct char_player_data player;	/* Normal data                   */
	struct char_ability_data real_abils;	/* Abilities without modifiers   */
	struct char_ability_data aff_abils;	/* Abils with spells/stones/etc  */
	struct char_point_data points;	/* Points                        */
	struct player_special_data *player_specials;	/* PC specials          */
	struct mob_special_data mob_specials;	/* NPC specials          */
    struct char_language_data language_data;
	struct char_special_data char_specials;	/* PC/NPC specials      */
	struct affected_type *affected;	/* affected by what spells       */
	struct obj_data *equipment[NUM_WEARS];	/* Equipment array               */
	struct obj_data *implants[NUM_WEARS];
    struct obj_data *tattoos[NUM_WEARS];
    struct prog_state_data *prog_state;

	struct obj_data *carrying;	/* Head of list                  */
	struct descriptor_data *desc;	/* NULL for mobiles              */
	struct account *account;

	struct follow_type *followers;	/* List of chars followers       */
	struct creature *master;	/* Who is char following?        */

    int prog_marker;
};

struct aff_stash {
    struct obj_data *saved_eq[NUM_WEARS];
    struct obj_data *saved_impl[NUM_WEARS];
    struct obj_data *saved_tattoo[NUM_WEARS];
    struct affected_type *saved_affs;
};

/* ====================================================================== */

extern GList *creatures;
extern GHashTable *creature_map;

/*@only@*/ struct creature *make_creature(bool pc);
void free_creature(/*@only@*/ struct creature *ch);

int level_bonus(struct creature *ch, bool primary);
int skill_bonus(struct creature *ch, int skillnum);
int calc_penalized_exp(struct creature *ch, int experience, struct creature *victim);

void dismount(struct creature *ch);
void mount(struct creature *ch, struct creature *vict);

struct room_data *player_loadroom(struct creature *ch);
bool load_player_objects(struct creature *ch);
bool save_player_objects(struct creature *ch);
bool display_unrentables(struct creature *ch);
money_t adjusted_price(struct creature *buyer, struct creature *seller, money_t base_price);
int cost_modifier(struct creature *ch, struct creature *seller);

bool creature_trusts_idnum(struct creature *ch, long idnum);
bool creature_distrusts_idnum(struct creature *ch, long idnum);
bool creature_trusts(struct creature *ch, struct creature *target);
bool creature_distrusts(struct creature *ch, struct creature *target);
float damage_reduction(struct creature *ch, struct creature *attacker);
int reputation_of(struct creature *ch);
void gain_reputation(struct creature *ch, int amt);
void ignite_creature(struct creature *ch, struct creature *igniter);
void extinguish_creature(struct creature *ch);
int creature_breath_threshold(struct creature *ch);

void start_defending(struct creature *ch, struct creature *target);
void stop_defending(struct creature *ch);

void add_combat(struct creature *ch, struct creature *target, bool initiated);
struct creature *random_opponent(struct creature *ch);
void remove_combat(struct creature *ch, struct creature *vict);
void remove_all_combat(struct creature *ch);
void remove_combat(struct creature *ch, struct creature *target);

#define NPC_HUNTING(ch) ((ch)->char_specials.hunting)
#define MOUNTED_BY(ch) ((ch)->char_specials.mounted)
#define DEFENDING(ch) ((ch)->char_specials.defending)
#define NPC_FLAGS(ch)  ((ch)->char_specials.saved.act)
#define NPC2_FLAGS(ch) ((ch)->char_specials.saved.act2)
#define PLR_FLAGS(ch)  ((ch)->char_specials.saved.act)
#define PLR2_FLAGS(ch) ((ch)->player_specials->saved.plr2_bits)

#define PRF_FLAGS(ch)  ((ch)->player_specials->saved.pref)
#define PRF2_FLAGS(ch) ((ch)->player_specials->saved.pref2)
#define AFF_FLAGS(ch)  ((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected2_by)
#define AFF3_FLAGS(ch) ((ch)->char_specials.saved.affected3_by)

#define CHAR_SOILAGE(ch, pos) (ch->player_specials->soilage[pos])
#define CHAR_SOILED(ch, pos, soil) (IS_SET(CHAR_SOILAGE(ch, pos), soil))

#define ILLEGAL_SOILPOS(pos) \
     (pos == WEAR_LIGHT || pos == WEAR_SHIELD || pos == WEAR_ABOUT || \
      pos == WEAR_WIELD || pos == WEAR_HOLD || pos == WEAR_BELT ||    \
      pos == WEAR_WIELD_2 || pos == WEAR_ASS || pos == WEAR_NECK_2 || \
      pos == WEAR_RANDOM)

#define IS_WEAR_EXTREMITY(pos) \
    (        pos == WEAR_FINGER_L || pos == WEAR_FINGER_R \
     || pos == WEAR_LEGS     || pos == WEAR_FEET     || pos == WEAR_BELT \
     || pos == WEAR_EAR_L    || pos == WEAR_EAR_R    || pos == WEAR_ARMS \
     || pos == WEAR_HANDS    || pos == WEAR_HOLD     || pos == WEAR_WIELD \
     || pos == WEAR_SHIELD   || pos == WEAR_WRIST_L  || pos == WEAR_WRIST_R \
         || pos == WEAR_WAIST         || pos == WEAR_CROTCH)

#define IS_WEAR_STRIKER(pos) \
    (        pos == WEAR_FINGER_L || pos == WEAR_FINGER_R \
     || pos == WEAR_LEGS     || pos == WEAR_FEET \
     || pos == WEAR_EAR_L    || pos == WEAR_EAR_R    || pos == WEAR_ARMS \
     || pos == WEAR_HANDS    || pos == WEAR_WRIST_L  || pos == WEAR_WRIST_R \
         || pos == WEAR_WAIST          || pos == WEAR_HEAD)

#define ILLEGAL_IMPLANTPOS(pos) \
     (pos == WEAR_LIGHT || pos == WEAR_SHIELD || pos == WEAR_ABOUT || \
      pos == WEAR_WIELD || pos == WEAR_BELT || pos == WEAR_WIELD_2 || \
	  pos == WEAR_RANDOM || pos == WEAR_HOLD)

#define ILLEGAL_TATTOOPOS(pos) \
     (pos == WEAR_LIGHT || pos == WEAR_SHIELD || pos == WEAR_ABOUT || \
      pos == WEAR_WIELD || pos == WEAR_HOLD || pos == WEAR_BELT ||    \
      pos == WEAR_WIELD_2 || pos == WEAR_ASS || pos == WEAR_NECK_2 || \
      pos == WEAR_FINGER_L || pos == WEAR_FINGER_R ||                 \
      pos == WEAR_EYES || pos == WEAR_RANDOM)

#define GET_ROWS(ch)    ((ch)->player_specials->saved.page_length)
#define GET_COLS(ch)    ((ch)->player_specials->saved.columns)

#define IS_NPC(ch)  (IS_SET(NPC_FLAGS(ch), NPC_ISNPC))
#define IS_PC(ch)   (!IS_NPC(ch))

#define NOHASS(ch)       (PRF_FLAGGED(ch, PRF_NOHASSLE))

#define GET_MORALE(ch)     (ch->mob_specials.shared->morale)
#define NPC_SHARED(ch)     (ch->mob_specials.shared)

#define IS_PET(ch)       (NPC_FLAGGED(ch, NPC_PET))
#define IS_SOULLESS(ch) (NPC_FLAGGED(ch, NPC_SOULLESS) || PLR2_FLAGGED(ch, PLR2_SOULLESS))
#define HAS_SYMBOL(ch) (IS_SOULLESS(ch) || affected_by_spell(ch, SPELL_STIGMATA) \
                        || AFF3_FLAGGED(ch, AFF3_SYMBOL_OF_PAIN) \
                        || AFF3_FLAGGED(ch, AFF3_TAINTED))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF2_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))

#define CHAR_WITHSTANDS_RAD(ch)   (PRF_FLAGGED(ch, PRF_NOHASSLE) || \
                                   AFF2_FLAGGED(ch, AFF2_PROT_RAD))

#define CHAR_WITHSTANDS_FIRE(ch)  (GET_LEVEL(ch) >= LVL_IMMORT      ||  \
                                  AFF2_FLAGGED(ch, AFF2_PROT_FIRE) ||  \
                                        GET_CLASS(ch) == CLASS_FIRE ||  \
                      (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_RED) ||  \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_4 || \
                               GET_PLANE(ch->in_room) == PLANE_HELL_6)))

#define CHAR_WITHSTANDS_COLD(ch)  (GET_LEVEL(ch) >= LVL_IMMORT        || \
                                   IS_VAMPIRE(ch) || \
                                  AFF2_FLAGGED(ch, AFF2_ENDURE_COLD) || \
                                  AFF2_FLAGGED(ch, AFF2_ABLAZE)      || \
                                   (GET_POSITION(ch) == POS_SLEEPING &&       \
                                    AFF3_FLAGGED(ch, AFF3_STASIS))    || \
                      (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_WHITE) || \
                        IS_UNDEAD(ch) || GET_CLASS(ch) == CLASS_FROST || \
                                                         IS_SLAAD(ch) || \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_5 || \
                               GET_PLANE(ch->in_room) == PLANE_HELL_8)))

#define CHAR_WITHSTANDS_HEAT(ch)  (GET_LEVEL(ch) >= LVL_IMMORT        || \
                                   AFF3_FLAGGED(ch, AFF3_PROT_HEAT)   || \
                                   (GET_POSITION(ch) == POS_SLEEPING &&       \
                                    AFF3_FLAGGED(ch, AFF3_STASIS))    || \
                        (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_RED) || \
                         IS_UNDEAD(ch) || GET_CLASS(ch) == CLASS_FIRE || \
                          IS_SLAAD(ch) || \
                           (GET_NPC_VNUM(ch) >= 16100 && \
                            GET_NPC_VNUM(ch) <= 16699) || \
             (IS_DEVIL(ch) && (GET_PLANE(ch->in_room) == PLANE_HELL_4 || \
                                   GET_PLANE(ch->in_room) == PLANE_HELL_6)))

#define CHAR_WITHSTANDS_ELECTRIC(ch)   \
                                (AFF2_FLAGGED(ch, AFF2_PROT_LIGHTNING) || \
                                 IS_VAMPIRE(ch) || \
                           (IS_DRAGON(ch) && GET_CLASS(ch) == CLASS_BLUE))

#define NEEDS_TO_BREATHE(ch) \
     (!AFF3_FLAGGED(ch, AFF3_NOBREATHE) && \
      !IS_UNDEAD(ch) && (GET_NPC_VNUM(ch) <= 16100 || \
                         GET_NPC_VNUM(ch) > 16999))

#define NULL_PSI(vict) \
     (IS_UNDEAD(vict) || IS_SLIME(vict) || IS_PUDDING(vict) || \
      IS_ROBOT(vict) || IS_PLANT(vict))

inline static bool
is_dead(struct creature *ch)
{
    return (ch->char_specials.position == POS_DEAD);
}

inline static /*@dependent@*/ /*@null@*/  GList *
first_living(GList *node)
{
    while (node != NULL && is_dead((struct creature *)node->data))
        node = node->next;
    return node;
}

inline static /*@dependent@*/ /*@null@*/ GList *
next_living(GList *node)
{
    if (!node)
        return NULL;

    node = node->next;
    return first_living(node);
}

#define IN_ROOM(ch)        ((ch)->in_room)
#define GET_WAS_IN(ch)        ((ch)->player_specials->was_in_room)
#define GET_AGE(ch)     (MAX(age(ch).year + ch->player.age_adjust, 17))

#define GET_NAME(ch)    (IS_NPC(ch) ? \
                         (ch)->player.short_descr : (ch)->player.name)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.char_class)
#define GET_REMORT_CLASS(ch)  ((ch)->player.remort_char_class)
#define CHECK_REMORT_CLASS(ch)  ((ch)->player.remort_char_class)
#define GET_RACE(ch)        ((ch)->player.race)
#define GET_HOME(ch)        ((ch)->player.hometown)
#define GET_HEIGHT(ch)        ((ch)->player.height)
#define GET_WEIGHT(ch)        ((ch)->player.weight)
#define GET_SEX(ch)        ((ch)->player.sex)
#define IS_MALE(ch)     ((ch)->player.sex == SEX_MALE)
#define IS_FEMALE(ch)   ((ch)->player.sex == SEX_FEMALE)
#define IS_IMMORT(ch)	(GET_LEVEL(ch) >= LVL_AMBASSADOR)
#define IS_MORT(ch)		(!IS_REMORT(ch) && !IS_IMMORT(ch))

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)          ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)          ((ch)->points.hit)
#define GET_MAX_HIT(ch)          ((ch)->points.max_hit)
#define GET_MOVE(ch)          ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)          ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)          ((ch)->points.gold)
#define GET_CASH(ch)      ((ch)->points.cash)

#define USE_METRIC(ch)    ((ch)->desc \
                           && (ch)->desc->account \
                           && (ch)->desc->account->metric_units)
#define GET_PAGE_LENGTH(ch)     ((ch->desc) ? ch->desc->account->term_height:0)
#define GET_PAGE_WIDTH(ch)     ((ch->desc) ? ch->desc->account->term_width:0)
#define GET_PAST_BANK(ch) (((ch)->account) ? (ch)->account->bank_past:0)
#define GET_FUTURE_BANK(ch) (((ch)->account) ? (ch)->account->bank_future:0)

#define BANK_MONEY(ch) (ch->in_room->zone->time_frame == TIME_ELECTRO ? \
                        GET_FUTURE_BANK(ch) : GET_PAST_BANK(ch))
#define CASH_MONEY(ch) (ch->in_room->zone->time_frame == TIME_ELECTRO ? \
                    GET_CASH(ch) : GET_GOLD(ch))
const char *CURRENCY(struct creature * ch);

#define GET_HITROLL(ch)          ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

#define GET_IDNUM(ch)          ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define IS_WEARING_W(ch)  ((ch)->char_specials.worn_weight)
#define TOTAL_ENCUM(ch)   (IS_CARRYING_W(ch) + IS_WEARING_W(ch))
#define GET_WEAPON_PROF(ch)  ((ch)->char_specials.weapon_proficiency)
#define GET_POSITION(ch)        ((ch)->char_specials.position)
#define CHECK_WEAPON_PROF(ch)  ((ch)->char_specials.weapon_proficiency)
#define MEDITATE_TIMER(ch)  (ch->char_specials.meditate_timer)
#define GET_FALL_COUNT(ch)     ((ch)->char_specials.fall_count)
#define GET_MOOD(ch)		(ch->char_specials.mood_str)
#define BREATH_COUNT_OF(ch) (ch->char_specials.breath_count)

#define DRIVING(ch)       ((ch)->char_specials.driving)
#define GET_SAVE(ch, i)          ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COND(ch, i)                ((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)        ((ch)->player_specials->saved.load_room)
#define GET_HOMEROOM(ch)		((ch)->player_specials->saved.home_room)
#define GET_LIFE_POINTS(ch)     ((ch)->player_specials->saved.life_points)
#define GET_MOBKILLS(ch)        ((ch)->player_specials->saved.mobkills)
#define GET_MSHIELD_LOW(ch)     ((ch)->player_specials->saved.mana_shield_low)
#define GET_MSHIELD_PCT(ch)     ((ch)->player_specials->saved.mana_shield_pct)
#define GET_PKILLS(ch)          ((ch)->player_specials->saved.pkills)
#define GET_ARENAKILLS(ch)		((ch)->player_specials->saved.akills)
#define GET_PC_DEATHS(ch)       ((ch)->player_specials->saved.deaths)
#define GET_SEVERITY(ch)		((ch)->player_specials->saved.killer_severity)
#define SPEED_OF(ch)            ((ch)->player_specials->saved.speed)
#define RAW_REPUTATION_OF(ch)      ((ch)->player_specials->saved.reputation)

#define GET_INVIS_LVL(ch)        ((ch)->player_specials->saved.invis_level)
#define GET_BROKE(ch)           ((ch)->player_specials->saved.broken_component)
#define GET_OLD_CLASS(ch)       ((ch)->player_specials->saved.old_char_class)
#define GET_TOT_DAM(ch)         ((ch)->player_specials->saved.total_dam)
#define GET_WIMP_LEV(ch)        ((ch)->player_specials->saved.wimp_level)
#define GET_WEAP_SPEC(ch, i)    ((ch)->player_specials->saved.weap_spec[(i)])
#define GET_DEITY(ch)                ((ch)->player_specials->saved.deity)
#define GET_CLAN(ch)                ((ch)->player_specials->saved.clan)
#define GET_REMORT_GEN(ch) ((ch)->char_specials.saved.remort_generation)

#define GET_IMMORT_QP(ch)    ((ch)->player_specials->saved.imm_qp)
#define GET_QUEST_ALLOWANCE(ch) ((ch)->player_specials->saved.qp_allowance)
#define GET_QLOG_LEVEL(ch)     ((ch)->player_specials->saved.qlog_level)
#define GET_QUEST(ch)           ((ch)->player_specials->saved.quest_id)
#define GET_FREEZE_LEV(ch)        ((ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)                ((ch)->player_specials->saved.bad_pws)
#define POOFIN(ch)                ((ch)->player_specials->poofin)
#define POOFOUT(ch)                ((ch)->player_specials->poofout)
#define AFK_REASON(ch)                ((ch)->player_specials->afk_reason)
#define AFK_NOTIFIES(ch)                ((ch)->player_specials->afk_notifies)
#define BADGE(ch)					((ch)->player_specials->saved.badge)
#define GET_IMPRINT_ROOM(ch,i)  ((ch)->player_specials->imprint_rooms[i])
#define GET_RECENT_KILLS(ch) ((ch)->player_specials->recently_killed)
#define GET_GRIEVANCES(ch)   ((ch)->player_specials->grievances)
#define GET_LAST_OLC_TARG(ch)        ((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)        ((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)                ((ch)->player_specials->aliases)
#define GET_LAST_TELL_FROM(ch)        ((ch)->player_specials->last_tell_from)
#define GET_LAST_TELL_TO(ch)        ((ch)->player_specials->last_tell_to)
#define GET_OLC_OBJ(ch)         ((ch)->player_specials->olc_obj)
#define GET_OLC_MOB(ch)         ((ch)->player_specials->olc_mob)
#define GET_OLC_SHOP(ch)        ((ch)->player_specials->olc_shop)
#define GET_OLC_HELP(ch)        ((ch)->player_specials->olc_help_item)
#define GET_OLC_SRCH(ch)        ((ch)->player_specials->olc_srch)
#define GET_OLC_HANDLER(ch)     ((ch)->player_specials->olc_handler)

#define SET_SKILL(ch, i, pct)        \
                             {(ch)->player_specials->saved.skills[i] = pct; }
#define SET_TONGUE(ch, i, pct)        \
                             {(ch)->language_data.tongues[i] = pct; }

#define GET_EQ(ch, i)                ((ch)->equipment[i])
#define GET_IMPLANT(ch, i)      ((ch)->implants[i])
#define GET_TATTOO(ch, i)      ((ch)->tattoos[i])

#define GET_NPC_SPEC(ch) (IS_NPC(ch) ? ((ch)->mob_specials.shared->func) : NULL)
#define GET_NPC_PROG(ch) (IS_NPC(ch) ? ((ch)->mob_specials.shared->prog) : NULL)
#define GET_NPC_PROGOBJ(ch) (IS_NPC(ch) ? ((ch)->mob_specials.shared->progobj) : NULL)

#define GET_NPC_PARAM(ch) (IS_NPC(ch) ? ((ch)->mob_specials.shared->func_param) : NULL)
#define GET_LOAD_PARAM(ch) (IS_NPC(ch) ? ((ch)->mob_specials.shared->load_param) : NULL)
#define GET_NPC_VNUM(mob)        (IS_NPC(mob) ? \
                                      (mob)->mob_specials.shared->vnum : -1)

#define GET_NPC_WAIT(ch)        ((ch)->mob_specials.wait_state)
#define GET_NPC_LAIR(ch)        ((ch)->mob_specials.shared->lair)
#define GET_NPC_LEADER(ch)        ((ch)->mob_specials.shared->leader)
#define GET_DEFAULT_POS(ch)        ((ch)->mob_specials.shared->default_pos)
#define MEMORY(ch)                ((ch)->mob_specials.memory)
#define NPC_IDNUM(ch)           ((ch)->mob_specials.mob_idnum)

#define CAN_CARRY_W(ch) (MAX(10, RAW_CARRY_W(ch)))

#define RAW_CARRY_W(ch) (AFF2_FLAGGED(ch, AFF2_TELEKINESIS) ? \
                         (strength_carry_weight(GET_STR(ch)) * 2) : \
                         strength_carry_weight(GET_STR(ch)) + \
                         ((GET_LEVEL(ch) >= LVL_GOD) ? 100000 : 0) + \
						 ((GET_LEVEL(ch) >= LVL_IMMORT) ? 1000 : 0))
#define CAN_CARRY_N(ch) (1 + GET_DEX(ch) + \
                         ((GET_LEVEL(ch) >= LVL_GOD) ? 100000 : 0) + \
						 ((GET_LEVEL(ch) >= LVL_IMMORT) ? 1000 : 0) + \
                         (AFF2_FLAGGED(ch, AFF2_TELEKINESIS) ? \
                          (GET_LEVEL(ch) / 4) : 0))

#define AWAKE(ch) (GET_POSITION(ch) > POS_SLEEPING && !AFF2_FLAGGED(ch, AFF2_MEDITATE))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_SICK(ch)    (AFF3_FLAGGED(ch, AFF3_SICKNESS))
#define IS_HAMSTRUNG(ch)    (AFF3_FLAGGED(ch, AFF3_HAMSTRUNG))
#define HAS_POISON_1(ch) (AFF_FLAGGED(ch, AFF_POISON))
#define HAS_POISON_2(ch) (AFF3_FLAGGED(ch, AFF3_POISON_2))
#define HAS_POISON_3(ch) (AFF3_FLAGGED(ch, AFF3_POISON_3))
#define IS_POISONED(ch) (HAS_POISON_1(ch) || HAS_POISON_2(ch) || \
                         HAS_POISON_3(ch))
#define IS_CONFUSED(ch)  (AFF_FLAGGED(ch, AFF_CONFUSION) \
                          || AFF3_FLAGGED(ch, AFF3_SYMBOL_OF_PAIN))
#define THACO(char_class, level) \
     ((char_class < 0) ? 20 :                             \
      ((char_class < NUM_CLASSES) ?                       \
       (20 - (level * thaco_factor[char_class])) :        \
       (20 - (level * thaco_factor[CLASS_WARRIOR]))))

static inline bool NPC_FLAGGED(struct creature *ch, enum mob_flag flag) {
    return IS_NPC(ch) && IS_SET(NPC_FLAGS(ch), flag);
}
static inline bool NPC2_FLAGGED(struct creature *ch, enum mob2_flag flag) {
    return IS_NPC(ch) && IS_SET(NPC2_FLAGS(ch), flag);
}
static inline bool PLR_FLAGGED(struct creature *ch, enum plr_flag flag) {
    return IS_PC(ch) && IS_SET(PLR_FLAGS(ch), flag);
}
static inline bool PLR2_FLAGGED(struct creature *ch, enum plr2_flag flag) {
    return IS_PC(ch) && IS_SET(PLR2_FLAGS(ch), flag);
}
static inline bool AFF_FLAGGED(struct creature *ch, enum affect_bit flag) {
    return IS_SET(AFF_FLAGS(ch), flag);
}
static inline bool AFF2_FLAGGED(struct creature *ch, enum aff2_bit flag) {
    return IS_SET(AFF2_FLAGS(ch), flag);
}
static inline bool AFF3_FLAGGED(struct creature *ch, enum aff3_bit flag) {
    return IS_SET(AFF3_FLAGS(ch), flag);
}

static inline bool
PRF_FLAGGED( struct creature *ch, enum prf_flag flag)
{
    if( IS_NPC(ch) ) {
        if(ch->desc != NULL && ch->desc->original != NULL) {
            return IS_SET(PRF_FLAGS(ch->desc->original), flag);
        } else {
            return false;
        }
    } else {
        return IS_SET(PRF_FLAGS(ch),flag);
    }
}
static inline bool
PRF2_FLAGGED( struct creature *ch, enum prf2_flag flag )
{
    if( IS_NPC(ch) ) {
        if(ch->desc != NULL && ch->desc->original != NULL) {
            return IS_SET(PRF2_FLAGS(ch->desc->original), flag);
        } else {
            return false;
        }
    } else {
        return IS_SET(PRF2_FLAGS(ch),flag);
    }
}

int reputation_of(struct creature *ch);

static inline bool
IS_CRIMINAL(struct creature *ch)
{
	return IS_PC(ch) && reputation_of(ch) >= 300;
}

static inline int
GET_REPUTATION_RANK(struct creature *ch)
{
	if (reputation_of(ch) == 0)
		return 0;
	else if (reputation_of(ch) >= 1000)
		return 11;

	return (reputation_of(ch) / 100) + 1;
}

static inline bool
IS_REMORT( const struct creature *ch )
{
	if( ch == NULL )
		return false;
	if( GET_REMORT_CLASS(ch) == CLASS_UNDEFINED )
		return false;
	if( IS_PC(ch) && GET_REMORT_GEN(ch) == 0 )
		return false;
	return true;
}

static inline /*@dependent@*/ /*@null@*/ struct room_direction_data*
EXIT(struct creature *ch, int dir) {
	return ch->in_room->dir_option[dir];
}
static inline /*@dependent@*/ /*@null@*/ struct room_direction_data*
_2ND_EXIT(struct creature *ch, int dir) {
    struct room_direction_data *exit = EXIT(ch, dir);
    if (exit != NULL) {
        return exit->to_room->dir_option[dir];
    }
    return NULL;
}
static inline /*@dependent@*/ /*@null@*/ struct room_direction_data*
_3RD_EXIT(struct creature *ch, int dir) {
    struct room_direction_data *exit = _2ND_EXIT(ch, dir);
    if (exit != NULL) {
        return exit->to_room->dir_option[dir];
    }
    return NULL;
}

static inline bool
NPC_CAN_GO(struct creature * ch, int door)
{
    struct room_direction_data *exit = EXIT(ch, door);

	if (exit != NULL
        && exit->to_room != NULL
        && (!IS_SET(exit->exit_info, EX_CLOSED | EX_NOPASS | EX_HIDDEN)
            || GET_LEVEL(ch) >= LVL_IMMORT
            || NON_CORPOREAL_MOB(ch))) {
		return true;
	}
	return false;
}

void add_player_tag(struct creature *ch, const char *tag);
void remove_player_tag(struct creature *ch, const char *tag);
bool player_has_tag(struct creature *ch, const char *tag);

bool is_fighting(struct creature *ch)
    __attribute__ ((nonnull));
bool is_newbie(struct creature *ch)
    __attribute__ ((nonnull));

void start_hunting(struct creature *ch, struct creature *vict)
    __attribute__ ((nonnull));
void stop_hunting(struct creature *ch)
    __attribute__ ((nonnull));

bool adjust_creature_money(struct creature *ch, money_t amount)
    __attribute__ ((nonnull));

void restore_creature(struct creature *ch)
    __attribute__ ((nonnull));
int unrent(struct creature *ch)
    __attribute__ ((nonnull));
bool checkLoadCorpse(struct creature *ch)
    __attribute__ ((nonnull));
int loadCorpse(struct creature *ch)
    __attribute__ ((nonnull));
void creature_set_reputation(struct creature *ch, int amt)
    __attribute__ ((nonnull));
void check_position(struct creature *ch)
    __attribute__ ((nonnull));

bool creature_rent(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_cryo(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_quit(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_idle(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_die(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_npk_die(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_arena_die(struct creature *ch)
    __attribute__ ((nonnull));
bool creature_purge(struct creature *ch, bool destroy_obj)
    __attribute__ ((nonnull));
bool creature_remort(struct creature *ch)
    __attribute__ ((nonnull));

int max_creature_attr(struct creature *ch, int mode)
    __attribute__ ((nonnull));
int strength_damage_bonus(int str);
int strength_hit_bonus(int str);
float strength_carry_weight(int str);
float strength_wield_weight(int str);
int dexterity_defense_bonus(int dex);
int dexterity_hit_bonus(int dex);
int dexterity_damage_bonus(int dex);
int constitution_hitpoint_bonus(int con);
int constitution_shock_bonus(int con);
int wisdom_mana_bonus(int intel);

struct aff_stash *stash_creature_affects(struct creature *ch)
    __attribute__ ((nonnull));
void restore_creature_affects(struct creature *ch,
                              struct aff_stash *aff_stash)
    __attribute__ ((nonnull));

#endif
