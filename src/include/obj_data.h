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

/* object-related defines ********************************************/
struct Creature;
class room_data;


/* Item types: used by obj_data.obj_flags.type_flag */
static const int ITEM_LIGHT = 1;	/* Item is a light source   */
static const int ITEM_SCROLL = 2;	/* Item is a scroll     */
static const int ITEM_WAND = 3;	/* Item is a wand       */
static const int ITEM_STAFF = 4;	/* Item is a staff      */
static const int ITEM_WEAPON = 5;	/* Item is a weapon     */
static const int ITEM_CAMERA = 6;	/* Unimplemented        */
static const int ITEM_MISSILE = 7;	/* Unimplemented        */
static const int ITEM_TREASURE = 8;	/* Item is a treasure, not gold */
static const int ITEM_ARMOR = 9;	/* Item is armor        */
static const int ITEM_POTION = 10;	/* Item is a potion     */
static const int ITEM_WORN = 11;	/* Unimplemented        */
static const int ITEM_OTHER = 12;	/* Misc object          */
static const int ITEM_TRASH = 13;	/* Trash - shopkeeps won't buy  */
static const int ITEM_TRAP = 14;	/* Unimplemented        */
static const int ITEM_CONTAINER = 15;	/* Item is a container      */
static const int ITEM_NOTE = 16;	/* Item is note         */
static const int ITEM_DRINKCON = 17;	/* Item is a drink container    */
static const int ITEM_KEY = 18;	/* Item is a key        */
static const int ITEM_FOOD = 19;	/* Item is food         */
static const int ITEM_MONEY = 20;	/* Item is money (gold)     */
static const int ITEM_PEN = 21;	/* Item is a pen        */
static const int ITEM_BOAT = 22;	/* Item is a boat       */
static const int ITEM_FOUNTAIN = 23;	/* Item is a fountain       */
static const int ITEM_WINGS = 24;	/* Item allows flying           */
static const int ITEM_VR_ARCADE = 25;
/**/ static const int ITEM_SCUBA_MASK = 26;
static const int ITEM_DEVICE = 27;	/*Activatable device             */
static const int ITEM_INTERFACE = 28;
static const int ITEM_HOLY_SYMB = 29;
static const int ITEM_VEHICLE = 30;
static const int ITEM_ENGINE = 31;
static const int ITEM_BATTERY = 32;
static const int ITEM_ENERGY_GUN = 33;
static const int ITEM_WINDOW = 34;
static const int ITEM_PORTAL = 35;
static const int ITEM_TOBACCO = 36;
static const int ITEM_CIGARETTE = 37;
static const int ITEM_METAL = 38;
static const int ITEM_VSTONE = 39;
static const int ITEM_PIPE = 40;
static const int ITEM_TRANSPORTER = 41;
static const int ITEM_SYRINGE = 42;
static const int ITEM_CHIT = 43;
static const int ITEM_SCUBA_TANK = 44;
static const int ITEM_TATTOO = 45;
static const int ITEM_TOOL = 46;
static const int ITEM_BOMB = 47;
static const int ITEM_DETONATOR = 48;
static const int ITEM_FUSE = 49;
static const int ITEM_PODIUM = 50;
static const int ITEM_PILL = 51;
static const int ITEM_ENERGY_CELL = 52;
static const int ITEM_V_WINDOW = 53;
static const int ITEM_V_DOOR = 54;
static const int ITEM_V_CONSOLE = 55;
static const int ITEM_GUN = 56;
static const int ITEM_BULLET = 57;
static const int ITEM_CLIP = 58;
static const int ITEM_MICROCHIP = 59;
static const int ITEM_COMMUNICATOR = 60;
static const int ITEM_SCRIPT = 61;
static const int NUM_ITEM_TYPES = 62;


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
static const int ITEM_WEAR_TAKE = (1 << 0);	/* Item can be takes      */
static const int ITEM_WEAR_FINGER = (1 << 1);	/* Can be worn on finger    */
static const int ITEM_WEAR_NECK = (1 << 2);	/* Can be worn around neck    */
static const int ITEM_WEAR_BODY = (1 << 3);	/* Can be worn on body    */
static const int ITEM_WEAR_HEAD = (1 << 4);	/* Can be worn on head    */
static const int ITEM_WEAR_LEGS = (1 << 5);	/* Can be worn on legs    */
static const int ITEM_WEAR_FEET = (1 << 6);	/* Can be worn on feet    */
static const int ITEM_WEAR_HANDS = (1 << 7);	/* Can be worn on hands  */
static const int ITEM_WEAR_ARMS = (1 << 8);	/* Can be worn on arms    */
static const int ITEM_WEAR_SHIELD = (1 << 9);	/* Can be used as a shield  */
static const int ITEM_WEAR_ABOUT = (1 << 10);	/* Can be worn about body    */
static const int ITEM_WEAR_WAIST = (1 << 11);	/* Can be worn around waist  */
static const int ITEM_WEAR_WRIST = (1 << 12);	/* Can be worn on wrist  */
static const int ITEM_WEAR_WIELD = (1 << 13);	/* Can be wielded        */
static const int ITEM_WEAR_HOLD = (1 << 14);	/* Can be held        */
static const int ITEM_WEAR_CROTCH = (1 << 15);	/* guess where    */
static const int ITEM_WEAR_EYES = (1 << 16);	/* eyes */
static const int ITEM_WEAR_BACK = (1 << 17);	/*Worn on back       */
static const int ITEM_WEAR_BELT = (1 << 18);	/* Worn on a belt(ie, pouch)   */
static const int ITEM_WEAR_FACE = (1 << 19);
static const int ITEM_WEAR_EAR = (1 << 20);
static const int ITEM_WEAR_ASS = (1 << 21);	/*Can be RAMMED up an asshole */
static const int NUM_WEAR_FLAGS = 22;


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
static const int ITEM_GLOW = (1 << 0);	/* Item is glowing      */
static const int ITEM_HUM = (1 << 1);	/* Item is humming      */
static const int ITEM_NORENT = (1 << 2);	/* Item cannot be rented    */
static const int ITEM_NODONATE = (1 << 3);	/* Item cannot be donated   */
static const int ITEM_NOINVIS = (1 << 4);	/* Item cannot be made invis    */
static const int ITEM_INVISIBLE = (1 << 5);	/* Item is invisible        */
static const int ITEM_MAGIC = (1 << 6);	/* Item is magical      */
static const int ITEM_NODROP = (1 << 7);	/* Item is cursed: can't drop   */
static const int ITEM_BLESS = (1 << 8);	/* Item is blessed      */
static const int ITEM_ANTI_GOOD = (1 << 9);	/* Not usable by good people    */
static const int ITEM_ANTI_EVIL = (1 << 10);	/* Not usable by evil people    */
static const int ITEM_ANTI_NEUTRAL = (1 << 11);	/* Not usable by neutral people */
static const int ITEM_ANTI_MAGIC_USER = (1 << 12);	/* Not usable by mages      */
static const int ITEM_ANTI_CLERIC = (1 << 13);	/* Not usable by clerics    */
static const int ITEM_ANTI_THIEF = (1 << 14);	/* Not usable by thieves    */
static const int ITEM_ANTI_WARRIOR = (1 << 15);	/* Not usable by warriors   */
static const int ITEM_NOSELL = (1 << 16);	/* Shopkeepers won't touch it   */
static const int ITEM_ANTI_BARB = (1 << 17);	/* no barb */
static const int ITEM_ANTI_PSYCHIC = (1 << 18);	/* no psychic */
static const int ITEM_ANTI_PHYSIC = (1 << 19);	/* no physic */
static const int ITEM_ANTI_CYBORG = (1 << 20);
static const int ITEM_ANTI_KNIGHT = (1 << 21);
static const int ITEM_ANTI_RANGER = (1 << 22);
static const int ITEM_ANTI_HOOD = (1 << 23);
static const int ITEM_ANTI_MONK = (1 << 24);
static const int ITEM_BLURRED = (1 << 25);
static const int ITEM_MAGIC_NODISPEL = (1 << 26);
static const int ITEM_UNUSED = (1 << 27);
static const int ITEM_REPULSION_FIELD = (1 << 28);
static const int ITEM_TRANSPARENT = (1 << 29);
static const int ITEM_DAMNED = (1 << 30);	/* Evil equivalent to Bless */
static const int NUM_EXTRA_FLAGS = 31;

static const int ITEM2_RADIOACTIVE = (1 << 0);
static const int ITEM2_ANTI_MERC = (1 << 1);
static const int ITEM2_ANTI_SPARE1 = (1 << 2);
static const int ITEM2_ANTI_SPARE2 = (1 << 3);
static const int ITEM2_ANTI_SPARE3 = (1 << 4);
static const int ITEM2_HIDDEN = (1 << 5);
static const int ITEM2_TRAPPED = (1 << 6);
static const int ITEM2_SINGULAR = (1 << 7);
static const int ITEM2_NOLOCATE = (1 << 8);
static const int ITEM2_NOSOIL = (1 << 9);
static const int ITEM2_CAST_WEAPON = (1 << 10);
static const int ITEM2_TWO_HANDED = (1 << 11);
static const int ITEM2_BODY_PART = (1 << 12);
static const int ITEM2_ABLAZE = (1 << 13);
static const int ITEM2_CURSED_PERM = (1 << 14);
static const int ITEM2_NOREMOVE = (1 << 15);
static const int ITEM2_THROWN_WEAPON = (1 << 16);
static const int ITEM2_GODEQ = (1 << 17);
static const int ITEM2_NO_MORT = (1 << 18);
static const int ITEM2_BROKEN = (1 << 19);
static const int ITEM2_IMPLANT = (1 << 20);
static const int ITEM2_REINFORCED = (1 << 21);
static const int ITEM2_ENHANCED = (1 << 22);
static const int ITEM2_PROTECTED_HUNT = (1 << 28);
static const int ITEM2_RENAMED = (1 << 29);
static const int ITEM2_UNAPPROVED = (1 << 30);
static const int NUM_EXTRA2_FLAGS = 31;

// extra3 flags

static const int ITEM3_REQ_MAGE = (1 << 0);
static const int ITEM3_REQ_CLERIC = (1 << 1);
static const int ITEM3_REQ_THIEF = (1 << 2);
static const int ITEM3_REQ_WARRIOR = (1 << 3);
static const int ITEM3_REQ_BARB = (1 << 4);
static const int ITEM3_REQ_PSIONIC = (1 << 5);
static const int ITEM3_REQ_PHYSIC = (1 << 6);
static const int ITEM3_REQ_CYBORG = (1 << 7);
static const int ITEM3_REQ_KNIGHT = (1 << 8);
static const int ITEM3_REQ_RANGER = (1 << 9);
static const int ITEM3_REQ_HOOD = (1 << 10);
static const int ITEM3_REQ_MONK = (1 << 11);
static const int ITEM3_REQ_VAMPIRE = (1 << 12);
static const int ITEM3_REQ_MERCENARY = (1 << 13);
static const int ITEM3_REQ_SPARE1 = (1 << 14);
static const int ITEM3_REQ_SPARE2 = (1 << 15);
static const int ITEM3_REQ_SPARE3 = (1 << 16);
static const int ITEM3_LATTICE_HARDENED = (1 << 17);
static const int ITEM3_STAY_ZONE = (1 << 18);
static const int ITEM3_HUNTED = (1 << 19);
static const int ITEM3_NOMAG = (1 << 20);
static const int ITEM3_NOSCI = (1 << 21);
static const int NUM_EXTRA3_FLAGS = 22;


/* Container flags - value[1] */
static const int CONT_CLOSEABLE = (1 << 0);	/* Container can be closed  */
static const int CONT_PICKPROOF = (1 << 1);	/* Container is pickproof   */
static const int CONT_CLOSED = (1 << 2);	/* Container is closed      */
static const int CONT_LOCKED = (1 << 3);	/* Container is locked      */


/* Some different kind of liquids for use in values of drink containers */
static const int LIQ_WATER = 0;
static const int LIQ_BEER = 1;
static const int LIQ_WINE = 2;
static const int LIQ_ALE = 3;
static const int LIQ_DARKALE = 4;
static const int LIQ_WHISKY = 5;
static const int LIQ_LEMONADE = 6;
static const int LIQ_FIREBRT = 7;
static const int LIQ_LOCALSPC = 8;
static const int LIQ_SLIME = 9;
static const int LIQ_MILK = 10;
static const int LIQ_TEA = 11;
static const int LIQ_COFFEE = 12;
static const int LIQ_BLOOD = 13;
static const int LIQ_SALTWATER = 14;
static const int LIQ_CLEARWATER = 15;
static const int LIQ_COKE = 16;
static const int LIQ_FIRETALON = 17;
static const int LIQ_SOUP = 18;
static const int LIQ_MUD = 19;
static const int LIQ_HOLY_WATER = 20;
static const int LIQ_OJ = 21;
static const int LIQ_GOATS_MILK = 22;
static const int LIQ_MUCUS = 23;
static const int LIQ_PUS = 24;
static const int LIQ_SPRITE = 25;
static const int LIQ_DIET_COKE = 26;
static const int LIQ_ROOT_BEER = 27;
static const int LIQ_VODKA = 28;
static const int LIQ_CITY_BEER = 29;
static const int LIQ_URINE = 30;
static const int LIQ_STOUT = 31;
static const int LIQ_SOULS = 32;
static const int LIQ_CHAMPAGNE = 33;
static const int LIQ_CAPPUCINO = 34;
static const int LIQ_RUM = 35;
static const int LIQ_SAKE = 36;
static const int LIQ_CHOCOLATE_MILK = 37;
static const int LIQ_JUICE = 38;
static const int NUM_LIQUID_TYPES = 39;

inline const char *
liquid_to_str(int liquid)
{
	extern const char *drinks[];

	if (liquid < 0 || liquid > NUM_LIQUID_TYPES) {
		return tmp_sprintf("!ILLEGAL(%d)!", liquid);
	}

	return drinks[liquid];
}


/* object-related structures ******************************************/

/* object flags; used in obj_data */
struct obj_flag_data {
	int setWeight(int new_weight);
	inline int getWeight(void) {
		return weight;
	} 
	int value[4];				/* Values of the item (see list)    */
	byte type_flag;				/* Type of item             */
	int wear_flags;				/* Where you can wear it        */
	int extra_flags;			/* If it hums, glows, etc.      */
	int extra2_flags;			/* More of the same...              */
	int extra3_flags;			// More, same, different
  private:
	int weight;					/* Weight what else                 */
  public:
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
/* ================== Memory Structure for Objects ================== */
struct obj_data {
	bool isUnrentable();
	int save(FILE * fl);
	int modifyWeight(int mod_weight);
	int setWeight(int new_weight);
	inline int getWeight(void) {
		return obj_flags.getWeight();
	} 
	inline int getContainedWeight(void) {
		obj_data *cur_obj;
		int result = 0;

		if (!contains)
			return 0;
		for (cur_obj = contains; cur_obj; cur_obj = cur_obj->next_content)
			result += cur_obj->getWeight();
		return result;
	}
	inline int getObjWeight(void) {
		return getWeight() - getContainedWeight();
	}
	inline int getNumContained(void) {
		obj_data *cur_obj;
		int result= 0;

		if (!contains)
			return 0;
		for (cur_obj = contains; cur_obj; cur_obj = cur_obj->next_content)
			result++;
		return result;
	}

	void clear();
	
	bool loadFromXML( obj_data *container, 
				 	  Creature *victim, 
					  room_data *room, 
					  xmlNodePtr node);

	void saveToXML( FILE* ouf );
	void display_rent(Creature *ch, const char *currency_str);

	room_data *in_room;	/* In what room -1 when conta/carr    */
	int cur_flow_pulse;			/* Keep track of flowing pulse        */

	struct obj_flag_data obj_flags;	/* Object information               */
	struct obj_affected_type affected[MAX_OBJ_AFFECT];	/* affects */

	char *name;	/* when worn/carry/in cont.         */
	char *aliases;					/* Title of object :get etc.        */
	char *line_desc;			/* When in room                     */
	char *action_desc;	/* What to write when used          */
	unsigned int plrtext_len;	/* If contains savable plrtext      */
	struct extra_descr_data *ex_description;	/* extra descriptions     */
	struct Creature *carried_by;	/* Carried by :NULL in room/conta   */
	struct Creature *worn_by;	/* Worn by?                 */
	struct obj_shared_data *shared;
	sh_int worn_on;				/* Worn where?              */
	unsigned int soilage;

	struct obj_data *in_obj;	/* In what object NULL when none    */
	struct obj_data *contains;	/* Contains objects                 */
	struct obj_data *aux_obj;	/* for special usage                */

	struct obj_data *next_content;	/* For 'contains' lists             */
	struct obj_data *next;		/* For the object list              */
};
/* ======================================================================= */

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


#endif
