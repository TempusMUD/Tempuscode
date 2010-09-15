#ifndef _OBJ_DATA_H_
#define _OBJ_DATA_H_

//
// File: obj_data.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/*#define DMALLOC 1 */

/* preamble *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "defs.h"
#include "constants.h"
#include "xml_utils.h"
#include "macros.h"

/* object-related defines ********************************************/
struct creature;
struct room_data;

/* Item types: used by struct obj_data.obj_flags.type_flag */
enum item_type {
    ITEM_LIGHT = 1,	/* Item is a light source   */
    ITEM_SCROLL = 2,	/* Item is a scroll     */
    ITEM_WAND = 3,	/* Item is a wand       */
    ITEM_STAFF = 4,	/* Item is a staff      */
    ITEM_WEAPON = 5,	/* Item is a weapon     */
    ITEM_CAMERA = 6,	/* Unimplemented        */
    ITEM_MISSILE = 7,	/* Unimplemented        */
    ITEM_TREASURE = 8,	/* Item is a treasure, not gold */
    ITEM_ARMOR = 9,	/* Item is armor        */
    ITEM_POTION = 10,	/* Item is a potion     */
    ITEM_WORN = 11,	/* Unimplemented        */
    ITEM_OTHER = 12,	/* Misc object          */
    ITEM_TRASH = 13,	/* Trash - shopkeeps won't buy  */
    ITEM_TRAP = 14,	/* Unimplemented        */
    ITEM_CONTAINER = 15,	/* Item is a container      */
    ITEM_NOTE = 16,	/* Item is note         */
    ITEM_DRINKCON = 17,	/* Item is a drink container    */
    ITEM_KEY = 18,	/* Item is a key        */
    ITEM_FOOD = 19,	/* Item is food         */
    ITEM_MONEY = 20,	/* Item is money (gold)     */
    ITEM_PEN = 21,	/* Item is a pen        */
    ITEM_BOAT = 22,	/* Item is a boat       */
    ITEM_FOUNTAIN = 23,	/* Item is a fountain       */
    ITEM_WINGS = 24,	/* Item allows flying           */
    ITEM_VR_ARCADE = 25,
/**/     ITEM_SCUBA_MASK = 26,
    ITEM_DEVICE = 27,	/*Activatable device             */
    ITEM_INTERFACE = 28,
    ITEM_HOLY_SYMB = 29,
    ITEM_VEHICLE = 30,
    ITEM_ENGINE = 31,
    ITEM_BATTERY = 32,
    ITEM_ENERGY_GUN = 33,
    ITEM_WINDOW = 34,
    ITEM_PORTAL = 35,
    ITEM_TOBACCO = 36,
    ITEM_CIGARETTE = 37,
    ITEM_METAL = 38,
    ITEM_VSTONE = 39,
    ITEM_PIPE = 40,
    ITEM_TRANSPORTER = 41,
    ITEM_SYRINGE = 42,
    ITEM_CHIT = 43,
    ITEM_SCUBA_TANK = 44,
    ITEM_TATTOO = 45,
    ITEM_TOOL = 46,
    ITEM_BOMB = 47,
    ITEM_DETONATOR = 48,
    ITEM_FUSE = 49,
    ITEM_PODIUM = 50,
    ITEM_PILL = 51,
    ITEM_ENERGY_CELL = 52,
    ITEM_V_WINDOW = 53,
    ITEM_V_DOOR = 54,
    ITEM_V_CONSOLE = 55,
    ITEM_GUN = 56,
    ITEM_BULLET = 57,
    ITEM_CLIP = 58,
    ITEM_MICROCHIP = 59,
    ITEM_COMMUNICATOR = 60,
    ITEM_SCRIPT = 61,
    ITEM_INSTRUMENT = 62,
    ITEM_BOOK = 63,
    NUM_ITEM_TYPES = 64,
};

// Instrument Types
enum instrument_type {
    ITEM_PERCUSSION = 0,
    ITEM_STRING = 1,
    ITEM_WIND = 2,
};

/* Take/Wear flags: used by struct obj_data.obj_flags.wear_flags */
enum wear_flag {
    ITEM_WEAR_TAKE = (1 << 0),	/* Item can be takes      */
    ITEM_WEAR_FINGER = (1 << 1),	/* Can be worn on finger    */
    ITEM_WEAR_NECK = (1 << 2),	/* Can be worn around neck    */
    ITEM_WEAR_BODY = (1 << 3),	/* Can be worn on body    */
    ITEM_WEAR_HEAD = (1 << 4),	/* Can be worn on head    */
    ITEM_WEAR_LEGS = (1 << 5),	/* Can be worn on legs    */
    ITEM_WEAR_FEET = (1 << 6),	/* Can be worn on feet    */
    ITEM_WEAR_HANDS = (1 << 7),	/* Can be worn on hands  */
    ITEM_WEAR_ARMS = (1 << 8),	/* Can be worn on arms    */
    ITEM_WEAR_SHIELD = (1 << 9),	/* Can be used as a shield  */
    ITEM_WEAR_ABOUT = (1 << 10),	/* Can be worn about body    */
    ITEM_WEAR_WAIST = (1 << 11),	/* Can be worn around waist  */
    ITEM_WEAR_WRIST = (1 << 12),	/* Can be worn on wrist  */
    ITEM_WEAR_WIELD = (1 << 13),	/* Can be wielded        */
    ITEM_WEAR_HOLD = (1 << 14),	/* Can be held        */
    ITEM_WEAR_CROTCH = (1 << 15),	/* guess where    */
    ITEM_WEAR_EYES = (1 << 16),	/* eyes */
    ITEM_WEAR_BACK = (1 << 17),	/*Worn on back       */
    ITEM_WEAR_BELT = (1 << 18),	/* Worn on a belt(ie, pouch)   */
    ITEM_WEAR_FACE = (1 << 19),
    ITEM_WEAR_EAR = (1 << 20),
    ITEM_WEAR_ASS = (1 << 21),	/*Can be RAMMED up an asshole */
    NUM_WEAR_FLAGS = 22,
};

/* Extra object flags: used by struct obj_data.obj_flags.extra_flags */
enum obj_flag {
    ITEM_GLOW = (1 << 0),	/* Item is glowing      */
    ITEM_HUM = (1 << 1),	/* Item is humming      */
    ITEM_NORENT = (1 << 2),	/* Item cannot be rented    */
    ITEM_NODONATE = (1 << 3),	/* Item cannot be donated   */
    ITEM_NOINVIS = (1 << 4),	/* Item cannot be made invis    */
    ITEM_INVISIBLE = (1 << 5),	/* Item is invisible        */
    ITEM_MAGIC = (1 << 6),	/* Item is magical      */
    ITEM_NODROP = (1 << 7),	/* Item is cursed: can't drop   */
    ITEM_BLESS = (1 << 8),	/* Item is blessed      */
    ITEM_ANTI_GOOD = (1 << 9),	/* Not usable by good people    */
    ITEM_ANTI_EVIL = (1 << 10),	/* Not usable by evil people    */
    ITEM_ANTI_NEUTRAL = (1 << 11),	/* Not usable by neutral people */
    ITEM_ANTI_MAGIC_USER = (1 << 12),	/* Not usable by mages      */
    ITEM_ANTI_CLERIC = (1 << 13),	/* Not usable by clerics    */
    ITEM_ANTI_THIEF = (1 << 14),	/* Not usable by thieves    */
    ITEM_ANTI_WARRIOR = (1 << 15),	/* Not usable by warriors   */
    ITEM_NOSELL = (1 << 16),	/* Shopkeepers won't touch it   */
    ITEM_ANTI_BARB = (1 << 17),	/* no barb */
    ITEM_ANTI_PSYCHIC = (1 << 18),	/* no psychic */
    ITEM_ANTI_PHYSIC = (1 << 19),	/* no physic */
    ITEM_ANTI_CYBORG = (1 << 20),
    ITEM_ANTI_KNIGHT = (1 << 21),
    ITEM_ANTI_RANGER = (1 << 22),
    ITEM_ANTI_BARD = (1 << 23),
    ITEM_ANTI_MONK = (1 << 24),
    ITEM_BLURRED = (1 << 25),
    ITEM_MAGIC_NODISPEL = (1 << 26),
    ITEM_UNUSED = (1 << 27),
    ITEM_REPULSION_FIELD = (1 << 28),
    ITEM_TRANSPARENT = (1 << 29),
    ITEM_DAMNED = (1 << 30),	/* Evil equivalent to Bless */
    NUM_EXTRA_FLAGS = 31,
};

enum obj2_flag {
    ITEM2_RADIOACTIVE = (1 << 0),
    ITEM2_ANTI_MERC = (1 << 1),
    ITEM2_ANTI_SPARE1 = (1 << 2),
    ITEM2_ANTI_SPARE2 = (1 << 3),
    ITEM2_ANTI_SPARE3 = (1 << 4),
    ITEM2_HIDDEN = (1 << 5),
    ITEM2_TRAPPED = (1 << 6),
    ITEM2_SINGULAR = (1 << 7),
    ITEM2_NOLOCATE = (1 << 8),
    ITEM2_NOSOIL = (1 << 9),
    ITEM2_CAST_WEAPON = (1 << 10),
    ITEM2_TWO_HANDED = (1 << 11),
    ITEM2_BODY_PART = (1 << 12),
    ITEM2_ABLAZE = (1 << 13),
    ITEM2_CURSED_PERM = (1 << 14),
    ITEM2_NOREMOVE = (1 << 15),
    ITEM2_THROWN_WEAPON = (1 << 16),
    ITEM2_GODEQ = (1 << 17),
    ITEM2_NO_MORT = (1 << 18),
    ITEM2_BROKEN = (1 << 19),
    ITEM2_IMPLANT = (1 << 20),
    ITEM2_REINFORCED = (1 << 21),
    ITEM2_ENHANCED = (1 << 22),
    ITEM2_REQ_MORT = (1 << 23),
    ITEM2_PROTECTED_HUNT = (1 << 28),
    ITEM2_RENAMED = (1 << 29),
    ITEM2_UNAPPROVED = (1 << 30),
    NUM_EXTRA2_FLAGS = 31,
};

// extra3 flags
enum obj3_flag {
    ITEM3_REQ_MAGE = (1 << 0),
    ITEM3_REQ_CLERIC = (1 << 1),
    ITEM3_REQ_THIEF = (1 << 2),
    ITEM3_REQ_WARRIOR = (1 << 3),
    ITEM3_REQ_BARB = (1 << 4),
    ITEM3_REQ_PSIONIC = (1 << 5),
    ITEM3_REQ_PHYSIC = (1 << 6),
    ITEM3_REQ_CYBORG = (1 << 7),
    ITEM3_REQ_KNIGHT = (1 << 8),
    ITEM3_REQ_RANGER = (1 << 9),
    ITEM3_REQ_BARD = (1 << 10),
    ITEM3_REQ_MONK = (1 << 11),
    ITEM3_REQ_VAMPIRE = (1 << 12),
    ITEM3_REQ_MERCENARY = (1 << 13),
    ITEM3_REQ_SPARE1 = (1 << 14),
    ITEM3_REQ_SPARE2 = (1 << 15),
    ITEM3_REQ_SPARE3 = (1 << 16),
    ITEM3_LATTICE_HARDENED = (1 << 17),
    ITEM3_STAY_ZONE = (1 << 18),
    ITEM3_HUNTED = (1 << 19),
    ITEM3_NOMAG = (1 << 20),
    ITEM3_NOSCI = (1 << 21),
    NUM_EXTRA3_FLAGS = 22,
};

/* Container flags - value[1] */
enum container_flag {
    CONT_CLOSEABLE = (1 << 0),	/* Container can be closed  */
    CONT_PICKPROOF = (1 << 1),	/* Container is pickproof   */
    CONT_CLOSED = (1 << 2),	/* Container is closed      */
    CONT_LOCKED = (1 << 3),	/* Container is locked      */
};

/* Some different kind of liquids for use in values of drink containers */
enum liquid_type {
    LIQ_WATER = 0,
    LIQ_BEER = 1,
    LIQ_WINE = 2,
    LIQ_ALE = 3,
    LIQ_DARKALE = 4,
    LIQ_WHISKY = 5,
    LIQ_LEMONADE = 6,
    LIQ_FIREBRT = 7,
    LIQ_LOCALSPC = 8,
    LIQ_SLIME = 9,
    LIQ_MILK = 10,
    LIQ_TEA = 11,
    LIQ_COFFEE = 12,
    LIQ_BLOOD = 13,
    LIQ_SALTWATER = 14,
    LIQ_CLEARWATER = 15,
    LIQ_COKE = 16,
    LIQ_FIRETALON = 17,
    LIQ_SOUP = 18,
    LIQ_MUD = 19,
    LIQ_HOLY_WATER = 20,
    LIQ_OJ = 21,
    LIQ_GOATS_MILK = 22,
    LIQ_MUCUS = 23,
    LIQ_PUS = 24,
    LIQ_SPRITE = 25,
    LIQ_DIET_COKE = 26,
    LIQ_ROOT_BEER = 27,
    LIQ_VODKA = 28,
    LIQ_CITY_BEER = 29,
    LIQ_URINE = 30,
    LIQ_STOUT = 31,
    LIQ_SOULS = 32,
    LIQ_CHAMPAGNE = 33,
    LIQ_CAPPUCINO = 34,
    LIQ_RUM = 35,
    LIQ_SAKE = 36,
    LIQ_CHOCOLATE_MILK = 37,
    LIQ_JUICE = 38,
    LIQ_MEAD = 39,
    NUM_LIQUID_TYPES = 40,
};

enum obj_creator {
    CREATED_UNKNOWN = 0,
    CREATED_IMM = 2,
    CREATED_SEARCH = 3,
    CREATED_ZONE = 4,
    CREATED_MOB = 5,
    CREATED_PROG = 6,
    CREATED_PLAYER = 7,
};

// These constants are to be passed to affectJoin()
enum affect_join_flag {
    AFF_ADD = 0,
    AFF_AVG = 1,
    AFF_NOOP = 3,
};

static inline const char *
liquid_to_str(int liquid)
{
	extern const char *drinks[];

	if (liquid < 0 || liquid > NUM_LIQUID_TYPES) {
		return tmp_sprintf("!ILLEGAL(%d)!", liquid);
	}

	return drinks[liquid];
}

/* object-related structures ******************************************/

/* object flags; used in struct obj_data */
struct obj_flag_data {
	int value[4];				/* Values of the item (see list)    */
	byte type_flag;				/* Type of item             */
	int wear_flags;				/* Where you can wear it        */
	int extra_flags;			/* If it hums, glows, etc.      */
	int extra2_flags;			/* More of the same...              */
	int extra3_flags;			// More, same, different
	int weight;					/* Weight what else                 */
	int timer;					/* Timer for object                 */
	long bitvector[3];			/* To set chars bits                */
	int material;				/* material object is made of */
	int max_dam;
	int damage;
	int sigil_idnum;			// the owner of the sigil
	byte sigil_level;			// the level of the sigil
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
	byte location;				/* Which ability to change (APPLY_XXX) */
	sbyte modifier;				/* How much it changes by              */
};

struct tmp_obj_affect {
    char level;           /* level of spell which applied this affect */
    int type;             /* type of spell which applied this effect */
    int duration;         /* How long this effect will last */
    int dam_mod;          /* Current damage modifier */
    int maxdam_mod;       /* Max damage modifier */
    char val_mod[4];      /* How much to change each value by */
    char type_mod;        /* Change the type of the obj to this value */
    char old_type;        /* Original obj type (saved for wearoff) */
    int worn_mod;         /* Add these bits to the wear positions */
    int extra_mod;        /* Add this to the extra bitvector */
    char extra_index;     /* Which bitvector to modify */
    int weight_mod;       /* Change the weight by this value */
    char affect_loc[MAX_OBJ_AFFECT];  /* (APPLY_XXX)*/
    char affect_mod[MAX_OBJ_AFFECT];; /* Change apply by this value */
    struct tmp_obj_affect *next;
};

/* shared data structs */
struct obj_shared_data {
	int vnum;
	int number;
	int cost;					/* Value when sold (gp.)            */
	int cost_per_day;			/* Cost to keep pr. real day        */
	int house_count;
	/** The player id of the owner of this (oedited) object **/
	long owner_id;
	struct obj_data *proto;		/* pointer to prototype */
	SPECIAL(*func);
	char *func_param;
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
	struct room_data *in_room;	/* In what room -1 when conta/carr    */
	int cur_flow_pulse;			/* Keep track of flowing pulse        */

	struct obj_flag_data obj_flags;	/* Object information               */
	struct obj_affected_type affected[MAX_OBJ_AFFECT];	/* affects */

	char *name;	/* when worn/carry/in cont.         */
	char *aliases;					/* Title of object :get etc.        */
	char *line_desc;			/* When in room                     */
	char *action_desc;	/* What to write when used          */
	unsigned int plrtext_len;	/* If contains savable plrtext      */
	struct extra_descr_data *ex_description;	/* extra descriptions     */
	struct creature *carried_by;	/* Carried by :NULL in room/conta   */
	struct creature *worn_by;	/* Worn by?                 */
	struct obj_shared_data *shared;
	sh_int worn_on;				/* Worn where?              */
	unsigned int soilage;
	void *func_data;
	long unique_id;
	time_t creation_time;
	int creation_method;
	long int creator;

    /* Temp obj affects! */
    struct tmp_obj_affect *tmp_affects;
	struct obj_data *in_obj;	/* In what object NULL when none    */
	struct obj_data *contains;	/* Contains objects                 */
	struct obj_data *aux_obj;	/* for special usage                */

	struct obj_data *next_content;	/* For 'contains' lists             */
	struct obj_data *next;		/* For the object list              */
};
/* ======================================================================= */

struct obj_data *make_object(void);
void free_object(struct obj_data *obj);
void save_object_to_xml(struct obj_data *obj, FILE *outf);
struct obj_data *load_object_from_xml(struct obj_data *container, struct creature *victim, struct room_data* room, xmlNodePtr node);
int count_contained_objs(struct obj_data *obj);
int weigh_contained_objs(struct obj_data *obj);
struct obj_affected_type *obj_affected_by_spell(struct obj_data *object, int spell);
int equipment_position_of(struct obj_data *obj);
int implant_position_of(struct obj_data *obj);
void remove_object_affect(struct obj_data *obj, struct tmp_obj_affect *af);
struct tmp_obj_affect *obj_has_affect(struct obj_data *obj, int spellnum);
struct room_data *find_object_room(struct obj_data *obj);
void normalize_applies(struct obj_data *obj);
int modify_object_weight(struct obj_data *obj, int mod_weight);
bool obj_is_unrentable(struct obj_data *obj);
void obj_affect_join(struct obj_data *obj, struct tmp_obj_affect *af, int dur_mode, int val_mode, int aff_mode);

#endif
