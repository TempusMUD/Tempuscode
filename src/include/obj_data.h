//
// File: obj_data.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//


#ifndef __obj_data_h__
#define __obj_data_h__

/*#define DMALLOC 1 */

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* preamble *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT       1		/* Item is a light source	*/
#define ITEM_SCROLL      2		/* Item is a scroll		*/
#define ITEM_WAND        3		/* Item is a wand		*/
#define ITEM_STAFF       4		/* Item is a staff		*/
#define ITEM_WEAPON      5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON  6		/* Unimplemented		*/
#define ITEM_MISSILE     7		/* Unimplemented		*/
#define ITEM_TREASURE    8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR       9		/* Item is armor		*/
#define ITEM_POTION      10 		/* Item is a potion		*/
#define ITEM_WORN        11		/* Unimplemented		*/
#define ITEM_OTHER       12		/* Misc object			*/
#define ITEM_TRASH       13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP        14		/* Unimplemented		*/
#define ITEM_CONTAINER   15		/* Item is a container		*/
#define ITEM_NOTE        16		/* Item is note 		*/
#define ITEM_DRINKCON    17		/* Item is a drink container	*/
#define ITEM_KEY         18		/* Item is a key		*/
#define ITEM_FOOD        19		/* Item is food			*/
#define ITEM_MONEY       20		/* Item is money (gold)		*/
#define ITEM_PEN         21		/* Item is a pen		*/
#define ITEM_BOAT        22		/* Item is a boat		*/
#define ITEM_FOUNTAIN    23		/* Item is a fountain		*/
#define ITEM_WINGS       24		/* Item allows flying           */
#define ITEM_VR_ARCADE   25		/**/
#define ITEM_SCUBA_MASK  26
#define ITEM_DEVICE      27               /*Activatable device             */
#define ITEM_INTERFACE   28               
#define ITEM_HOLY_SYMB   29               
#define ITEM_VEHICLE     30
#define ITEM_ENGINE      31
#define ITEM_BATTERY     32
#define ITEM_ENERGY_GUN  33
#define ITEM_WINDOW      34
#define ITEM_PORTAL      35
#define ITEM_TOBACCO     36
#define ITEM_CIGARETTE   37
#define ITEM_METAL       38
#define ITEM_VSTONE      39
#define ITEM_PIPE        40
#define ITEM_TRANSPORTER 41
#define ITEM_SYRINGE     42
#define ITEM_CHIT        43
#define ITEM_SCUBA_TANK  44
#define ITEM_TATTOO      45
#define ITEM_TOOL        46
#define ITEM_BOMB        47
#define ITEM_DETONATOR   48
#define ITEM_FUSE        49
#define ITEM_PODIUM      50
#define ITEM_PILL        51
#define ITEM_ENERGY_CELL 52
#define ITEM_V_WINDOW    53
#define ITEM_V_DOOR      54
#define ITEM_V_CONSOLE   55
#define ITEM_GUN         56
#define ITEM_BULLET      57
#define ITEM_CLIP        58
#define ITEM_MICROCHIP   59
#define ITEM_COMMUNICATOR 60
#define ITEM_SCRIPT      61
#define ITEM_ELEVATOR_PANEL 62
#define NUM_ITEM_TYPES   63


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_CROTCH        (1 << 15) /* guess where    */
#define ITEM_WEAR_EYES          (1 << 16) /* eyes */
#define ITEM_WEAR_BACK		(1 << 17)  /*Worn on back		*/
#define ITEM_WEAR_BELT		(1 << 18)  /* Worn on a belt(ie, pouch)   */
#define ITEM_WEAR_FACE		(1 << 19)
#define ITEM_WEAR_EAR 	        (1 << 20)
#define ITEM_WEAR_ASS           (1 << 21)  /*Can be RAMMED up an asshole */
#define NUM_WEAR_FLAGS          22


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)	/* Item is glowing		*/
#define ITEM_HUM           (1 << 1)	/* Item is humming		*/
#define ITEM_NORENT        (1 << 2)	/* Item cannot be rented	*/
#define ITEM_NODONATE      (1 << 3)	/* Item cannot be donated	*/
#define ITEM_NOINVIS	   (1 << 4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (1 << 5)	/* Item is invisible		*/
#define ITEM_MAGIC         (1 << 6)	/* Item is magical		*/
#define ITEM_NODROP        (1 << 7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)	/* Item is blessed		*/
#define ITEM_ANTI_GOOD     (1 << 9)	/* Not usable by good people	*/
#define ITEM_ANTI_EVIL     (1 << 10)	/* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL  (1 << 11)	/* Not usable by neutral people	*/
#define ITEM_ANTI_MAGIC_USER (1 << 12)	/* Not usable by mages		*/
#define ITEM_ANTI_CLERIC   (1 << 13)	/* Not usable by clerics	*/
#define ITEM_ANTI_THIEF	   (1 << 14)	/* Not usable by thieves	*/
#define ITEM_ANTI_WARRIOR  (1 << 15)	/* Not usable by warriors	*/
#define ITEM_NOSELL	   (1 << 16)	/* Shopkeepers won't touch it	*/
#define ITEM_ANTI_BARB     (1 << 17)	/* no barb */
#define ITEM_ANTI_PSYCHIC  (1 << 18)  /* no psychic */
#define ITEM_ANTI_PHYSIC   (1 << 19)   /* no physic */
#define ITEM_ANTI_CYBORG   (1 << 20)   
#define ITEM_ANTI_KNIGHT   (1 << 21)
#define ITEM_ANTI_RANGER   (1 << 22)
#define ITEM_ANTI_HOOD     (1 << 23)
#define ITEM_ANTI_MONK     (1 << 24)
#define ITEM_BLURRED       (1 << 25)
#define ITEM_MAGIC_NODISPEL (1 << 26)
#define ITEM_UNUSED         (1 << 27)
#define ITEM_REPULSION_FIELD (1 << 28)
#define ITEM_TRANSPARENT    (1 << 29)
#define ITEM_EVIL_BLESS     (1 << 30)   /* Evil equivalent to Bless */
#define NUM_EXTRA_FLAGS     31

#define ITEM2_RADIOACTIVE       (1 << 0)
#define ITEM2_ANTI_MERC         (1 << 1)
#define ITEM2_ANTI_SPARE1       (1 << 2)
#define ITEM2_ANTI_SPARE2       (1 << 3)
#define ITEM2_ANTI_SPARE3       (1 << 4)
#define ITEM2_HIDDEN            (1 << 5)
#define ITEM2_TRAPPED           (1 << 6)
#define ITEM2_SINGULAR          (1 << 7)
#define ITEM2_NOLOCATE          (1 << 8)
#define ITEM2_CAST_WEAPON       (1 << 10)
#define ITEM2_TWO_HANDED	(1 << 11)
#define ITEM2_BODY_PART         (1 << 12)
#define ITEM2_ABLAZE            (1 << 13)
#define ITEM2_CURSED_PERM       (1 << 14)
#define ITEM2_NOREMOVE          (1 << 15)
#define ITEM2_THROWN_WEAPON     (1 << 16)
#define ITEM2_GODEQ             (1 << 17)
#define ITEM2_NO_MORT           (1 << 18)
#define ITEM2_BROKEN            (1 << 19)
#define ITEM2_IMPLANT           (1 << 20)
#define ITEM2_REINFORCED        (1 << 21)
#define ITEM2_ENHANCED          (1 << 22)
#define ITEM2_PROTECTED_HUNT    (1 << 28)
#define ITEM2_RENAMED           (1 << 29)
#define ITEM2_UNAPPROVED        (1 << 30)
#define NUM_EXTRA2_FLAGS         31

// extra3 flags

#define ITEM3_REQ_MAGE          (1 << 0)
#define ITEM3_REQ_CLERIC        (1 << 1)
#define ITEM3_REQ_THIEF         (1 << 2)
#define ITEM3_REQ_WARRIOR       (1 << 3)
#define ITEM3_REQ_BARB          (1 << 4)
#define ITEM3_REQ_PSIONIC       (1 << 5)
#define ITEM3_REQ_PHYSIC        (1 << 6)
#define ITEM3_REQ_CYBORG        (1 << 7)
#define ITEM3_REQ_KNIGHT        (1 << 8)
#define ITEM3_REQ_RANGER        (1 << 9)
#define ITEM3_REQ_HOOD          (1 << 10)
#define ITEM3_REQ_MONK          (1 << 11)
#define ITEM3_REQ_VAMPIRE       (1 << 12)
#define ITEM3_REQ_MERCENARY     (1 << 13)
#define ITEM3_REQ_SPARE1        (1 << 14)
#define ITEM3_REQ_SPARE2        (1 << 15)
#define ITEM3_REQ_SPARE3        (1 << 16)
#define ITEM3_LATTICE_HARDENED  (1 << 17)
#define NUM_EXTRA3_FLAGS         18


/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFEE     12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15
#define LIQ_COKE       16
#define LIQ_FIRETALON  17
#define LIQ_SOUP       18
#define LIQ_MUD        19
#define LIQ_HOLY_WATER 20
#define LIQ_OJ         21
#define LIQ_GOATS_MILK 22
#define LIQ_MUCUS      23
#define LIQ_PUS        24
#define LIQ_SPRITE     25
#define LIQ_DIET_COKE  26
#define LIQ_ROOT_BEER  27
#define LIQ_VODKA      28
#define LIQ_CITY_BEER  29
#define LIQ_URINE      30
#define LIQ_STOUT      31
#define LIQ_SOULS      32
#define LIQ_CHAMPAGNE  33
#define LIQ_CAPPUCINO  34
#define LIQ_RUM        35
#define LIQ_SAKE       36
#define LIQ_CHOCOLATE_MILK 37
#define LIQ_JUICE      38
#define NUM_LIQUID_TYPES 39


/* object-related structures ******************************************/

#ifdef TRACK_OBJS
#define TRACKER_STR_LEN 64
typedef struct obj_tracker {
    time_t lost_time;
    char string[TRACKER_STR_LEN];
} obj_tracker;
#endif

/* object flags; used in obj_data */
struct obj_flag_data {
    int setWeight( int new_weight );
    inline int getWeight( void ) { return weight; }
    int	value[4];	/* Values of the item (see list)    */
    byte type_flag;	/* Type of item			    */
    int	wear_flags;	/* Where you can wear it	    */
    int	extra_flags;	/* If it hums, glows, etc.	    */
    int  extra2_flags;   /* More of the same...              */
    int extra3_flags; // More, same, different
private:
    int	weight;		/* Weight what else                 */
public:
    int	timer;		/* Timer for object                 */
    long	bitvector[3];	/* To set chars bits                */
    int  material;       /* material object is made of */
    int  max_dam;
    int  damage;
    int  sigil_idnum;		// the owner of the sigil
    byte sigil_level;		// the level of the sigil
#ifdef TRACK_OBJS
    obj_tracker tracker; // temp debugging thing
#endif
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
    byte location;      /* Which ability to change (APPLY_XXX) */
    sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
    int modifyWeight( int mod_weight );
    int setWeight( int new_weight );
    inline int getWeight( void ) { return obj_flags.getWeight(); }
    struct room_data *in_room;	/* In what room -1 when conta/carr    */
    int cur_flow_pulse;          /* Keep track of flowing pulse        */

    struct obj_flag_data obj_flags;/* Object information               */
    struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

    char	*name;                    /* Title of object :get etc.        */
    char	*description;		  /* When in room                     */
    char	*short_description;       /* when worn/carry/in cont.         */
    char	*action_description;      /* What to write when used          */
    unsigned int plrtext_len;       /* If contains savable plrtext      */
    struct extra_descr_data *ex_description; /* extra descriptions     */
    struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
    struct char_data *worn_by;	  /* Worn by?			      */
    struct obj_shared_data *shared;
    sh_int worn_on;		  /* Worn where?		      */
    unsigned int soilage;

    struct obj_data *in_obj;       /* In what object NULL when none    */
    struct obj_data *contains;     /* Contains objects                 */
    struct obj_data *aux_obj;      /* for special usage                */

    struct obj_data *next_content; /* For 'contains' lists             */
    struct obj_data *next;         /* For the object list              */
};
typedef struct obj_data OBJ;
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
    obj_num item_number;
  
    char short_desc[EXDSCR_LENGTH];
    char name[EXDSCR_LENGTH];
    int	value[4];
    int	extra_flags;
    int   extra2_flags;
    int   bitvector[3];
    int	weight;
    int	timer;
    int   contains;	/* # of items this item contains */
    int   in_room_vnum;
    int   wear_flags;     /* positions which this can be worn on */
    int   damage;
    int   max_dam;
    int   material;
    unsigned int plrtext_len;
    byte  worn_on_position;
    byte  type;
    byte  sparebyte1;
    byte  sigil_level;
    int   soilage;
    int   sigil_idnum;
    int   extra3_flags;
    int   spareint4;
    struct obj_affected_type affected[MAX_OBJ_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
    int	time;
    int	rentcode;
    int	net_cost_per_diem;
    int	gold;
    int	account;
    int	nitems;
    int	credits;
    int	currency;
    int	spare2;
    int	spare3;
    int	spare4;
    int	spare5;
    int	spare6;
    int	spare7;
};

/* shared data structs */
struct obj_shared_data {
    int vnum;
    int number;
    int	cost;		/* Value when sold (gp.)            */
    int	cost_per_day;	/* Cost to keep pr. real day        */
    int house_count;
    struct obj_data *proto;     /* pointer to prototype */
    struct ticl_data *ticl_ptr; /* Pointer to TICL procedure */
    SPECIAL(*func);
};


#endif __obj_data_h__
