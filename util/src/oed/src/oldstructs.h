/* ***********************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* preamble *************************************************************/


#include <sys/types.h>

#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)


/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define FORWARD        6
#define RIGHT          7
#define BACKWARD       8
#define LEFT           9
#define FUTURE        10
#define PAST          11
#define NUM_DIRS      12

/* Door reset-states */
#define DOOR_OPEN               (1 << 0)
#define DOOR_CLOSED             (1 << 1)
#define DOOR_LOCKED             (1 << 2)
#define DOOR_HIDDEN             (1 << 3)

/* Zone flags */
#define ZONE_AUTOSAVE           (1 << 0)
#define ZONE_RESETSAVE          (1 << 1)
#define ZONE_NOTIFYOWNER        (1 << 2)
#define ZONE_LOCKED             (1 << 3)
#define ZONE_NOMAGIC            (1 << 4)
#define ZONE_NOLAW              (1 << 5)
#define ZONE_NOWEATHER          (1 << 6)
#define ZONE_NOCRIME            (1 << 7)
#define ZONE_FROZEN             (1 << 8)
#define ZONE_ISOLATED           (1 << 9)
#define ZONE_SOUNDPROOF         (1 << 10)
#define NUM_ZONE_FLAGS          11
#define ZONE_SEARCH_APPROVED    (1 << 19)
#define ZONE_MOBS_APPROVED      (1 << 20)
#define ZONE_OBJS_APPROVED      (1 << 21)
#define ZONE_ROOMS_APPROVED     (1 << 22)
#define ZONE_ZCMDS_APPROVED     (1 << 23)
#define ZONE_SHOPS_APPROVED     (1 << 24)
#define ZONE_MOBS_MODIFIED      (1 << 25)
#define ZONE_OBJS_MODIFIED      (1 << 26)
#define ZONE_ROOMS_MODIFIED     (1 << 27)
#define ZONE_ZONE_MODIFIED      (1 << 28)
#define ZONE_TICL_APPROVED      (1 << 29)
#define ZONE_TICL_MODIFIED      <1 << 30)
#define TOT_ZONE_FLAGS          29


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(1 << 0)   /* Dark			*/
#define ROOM_DEATH		(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(1 << 8)   /* room for only 1 pers	*/
#define ROOM_NOTEL		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		(1 << 13)  /* (R) The door to a house	*/
#define ROOM_SMOKE_FILLED	(1 << 14)  /* Room is filled with smoke */
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_NOPSIONICS         (1 << 16)  /* No psionics allowed       */
#define ROOM_NOSCIENCE          (1 << 17)  /* No physical alterations   */
#define ROOM_NORECALL	        (1 << 18)  /* Recall spell fails        */
#define ROOM_CLAN_HOUSE         (1 << 19)  /* Clan members only         */
#define ROOM_ARENA		(1 << 20)  /* PK Arena (no exp transfer)*/
#define ROOM_DOCK               (1 << 21)  /* Boats may be docked here  */
#define ROOM_FLAME_FILLED       (1 << 22)  /* Sets chars on fire        */
#define ROOM_ICE_COLD           (1 << 23)  /* Freezes chars             */
#define ROOM_NULL_MAGIC         (1 << 24)  /* Nullifies magical effects */
#define ROOM_HOLYOCEAN          (1 << 25)  /* */
#define ROOM_RADIOACTIVE        (1 << 26)  /* */
#define NUM_ROOM_FLAGS          26


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/
#define EX_HEAVY_DOOR           (1 << 4)   /* Door requires high STR    */  
#define EX_HARD_PICK            (1 << 5)   
#define EX_NOMOB                (1 << 6)   /* Mobs will not pass        */
#define EX_HIDDEN               (1 << 7)   /* Door revealed by search   */
#define EX_NOSCAN               (1 << 8)   /* Direction is hard to scan */
#define EX_TECH                 (1 << 9)   /* can't be picked convention */
#define EX_ONEWAY               (1 << 10)  /* bomb blasts are blocked */
#define EX_NOPASS               (1 << 11)  /* keep players out */
#define EX_WALL_THORNS          (1 << 12)  /* can get thru */
#define EX_WALL_THORNS_NOPASS   (1 << 13)
#define EX_WALL_STONE           (1 << 14)
#define EX_WALL_ICE             (1 << 15)
#define EX_WALL_FIRE            (1 << 16)
#define EX_WALL_FIRE_NOPASS     (1 << 17)
#define EX_WALL_FLESH           (1 << 18)
#define EX_WALL_IRON            (1 << 19)
#define EX_WALL_ENERGY_F        (1 << 20)
#define EX_WALL_ENERGY_F_NOPASS (1 << 21)
#define NUM_DOORFLAGS           22

/* Room Search Commands */
#define SEARCH_COM_NONE      0
#define SEARCH_COM_DOOR      1
#define SEARCH_COM_MOBILE    2
#define SEARCH_COM_OBJECT    3
#define SEARCH_COM_REMOVE    4
#define SEARCH_COM_GIVE      5
#define SEARCH_COM_EQUIP     6
#define SEARCH_COM_TRANSPORT 7
#define SEARCH_COM_SPELL     8
#define SEARCH_COM_DAMAGE    9
#define NUM_SEARCH_COM       10

     /** room search flags **/
#define SRCH_REPEATABLE         (1 << 0)
#define SRCH_TRIPPED            (1 << 1)
#define SRCH_IGNORE             (1 << 2)
#define SRCH_CLANPASSWD         (1 << 3)
#define SRCH_NOEVIL             (1 << 10)
#define SRCH_NONEUTRAL          (1 << 11)
#define SRCH_NOGOOD             (1 << 12)
#define SRCH_NOMAGE             (1 << 13)
#define SRCH_NOCLERIC           (1 << 14)
#define SRCH_NOTHIEF            (1 << 15)
#define SRCH_NOBARB             (1 << 16)
#define SRCH_NORANGER           (1 << 17)
#define SRCH_NOKNIGHT           (1 << 18)
#define SRCH_NOMONK             (1 << 19)
#define SRCH_NOPSIONIC          (1 << 20)
#define SRCH_NOPHYSIC           (1 << 21)
#define SRCH_NOMERC             (1 << 22)
#define SRCH_NOHOOD             (1 << 23)

#define NUM_SRCH_BITS           24

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_UNDERWATER	     8		   /* Underwater		*/
#define SECT_FLYING	     9		   /* Wheee!			*/
#define SECT_NOTIME         10             /* Between Times             */
#define SECT_CLIMBING       11             /* Climbing skill required   */
#define SECT_FREESPACE      12             /* Out of the atmosphere     */
#define SECT_ROAD	    13		   /* On the road     	  	*/
#define SECT_VEHICLE        14		   /* In a car			*/
#define SECT_CORNFIELD      15		   /* In the corn               */
#define SECT_SWAMP          16		   /* Swamp			*/
#define SECT_DESERT	    17		   /* Sandy and hot		*/
#define SECT_FIRE_RIVER     18
#define SECT_JUNGLE         19
#define SECT_PITCH_PIT      20
#define SECT_PITCH_SUB      21
#define SECT_BEACH          22
#define SECT_ASTRAL         23
#define NUM_SECT_TYPES      24

#define PLANE_PRIME_1        0
#define PLANE_PRIME_2        1
#define PLANE_ASTRAL	    10
#define PLANE_HELL_1	    11
#define PLANE_HELL_2	    12
#define PLANE_HELL_3	    13
#define PLANE_HELL_4	    14
#define PLANE_HELL_5	    15
#define PLANE_HELL_6	    16
#define PLANE_HELL_7	    17
#define PLANE_HELL_8	    18
#define PLANE_HELL_9	    19
#define PLANE_GHENNA	    20
#define PLANE_ABYSS         25
#define PLANE_OLC           39
#define PLANE_OLYMPUS       40
#define PLANE_COSTAL        41
#define PLANE_WESTERN       42
#define PLANE_HEAVEN        43
#define PLANE_DOOM          50
#define PLANE_NEVERWHERE    60
#define PLANE_UNDERDARK     61
#define PLANE_ELEM_WATER    70
#define PLANE_ELEM_FIRE     71
#define PLANE_ELEM_AIR      72
#define PLANE_ELEM_EARTH    73
#define PLANE_ELEM_POS      74
#define PLANE_ELEM_NEG      75

#define TIME_TIMELESS       0
#define TIME_MODRIAN        1
#define TIME_ELECTRO        2

/* char and mob-related defines *****************************************/


/* PC classes */
#define CLASS_UNDEFINED	      -1
#define CLASS_MAGIC_USER       0
#define CLASS_CLERIC           1
#define CLASS_THIEF            2
#define CLASS_WARRIOR          3
#define CLASS_BARB	       4
#define CLASS_PSIONIC          5    /* F */
#define CLASS_PHYSIC           6    /* F */
#define CLASS_CYBORG           7    /* F */
#define CLASS_KNIGHT           8
#define CLASS_RANGER           9
#define CLASS_HOOD            10    /* F */
#define CLASS_MONK            11   
#define CLASS_VAMPIRE         12
#define CLASS_MERCENARY       13
#define CLASS_SPARE1          14
#define CLASS_SPARE2          15
#define CLASS_SPARE3          16

#define NUM_CLASSES     17   /* This must be the number of classes!! */
#define CLASS_NORMAL    50
#define CLASS_BIRD      51
#define CLASS_PREDATOR  52
#define CLASS_SNAKE     53
#define CLASS_HORSE     54
#define CLASS_SMALL     55
#define CLASS_MEDIUM    56
#define CLASS_LARGE     57
#define CLASS_SCIENTIST 58
#define CLASS_SKELETON  60
#define CLASS_GHOUL	61
#define CLASS_SHADOW    62
#define CLASS_WIGHT     63
#define CLASS_WRAITH    64
#define CLASS_MUMMY     65
#define CLASS_SPECTRE   66
#define CLASS_NPC_VAMPIRE   67
#define CLASS_GHOST     68
#define CLASS_LICH      69
#define CLASS_ZOMBIE    70

#define CLASS_EARTH     81		/* Elementals */
#define CLASS_FIRE      82
#define CLASS_WATER     83
#define CLASS_AIR	84
#define CLASS_LIGHTNING 85
#define CLASS_GREEN	91	  	/* Dragons */
#define CLASS_WHITE     92
#define CLASS_BLACK     93
#define CLASS_BLUE      94
#define CLASS_RED       95
#define CLASS_SILVER    96
#define CLASS_SHADOW_D  97
#define CLASS_DEEP      98
#define CLASS_TURTLE    99
#define CLASS_LEAST    101		/* Devils  */
#define CLASS_LESSER   102		
#define CLASS_GREATER  103
#define CLASS_DUKE     104
#define CLASS_ARCH     105
#define CLASS_HILL     111
#define CLASS_STONE    112
#define CLASS_FROST    113
#define CLASS_FIRE_G   114
#define CLASS_CLOUD    115
#define CLASS_STORM    116
#define CLASS_SLAAD_RED    120		/* Slaad */
#define CLASS_SLAAD_BLUE   121
#define CLASS_SLAAD_GREEN  122
#define CLASS_SLAAD_GREY   123
#define CLASS_SLAAD_DEATH  124
#define CLASS_SLAAD_LORD   125
#define CLASS_DEMON_I	 	130	/* Demons of the Abyss */
#define CLASS_DEMON_II    	131
#define CLASS_DEMON_III    	132
#define CLASS_DEMON_IV    	133
#define CLASS_DEMON_V     	134
#define CLASS_DEMON_VI     	135
#define CLASS_DEMON_SEMI     	136
#define CLASS_DEMON_MINOR    	137
#define CLASS_DEMON_MAJOR    	138
#define CLASS_DEMON_LORD	139
#define CLASS_DEMON_PRINCE	140
#define CLASS_DEVA_ASTRAL       150
#define CLASS_DEVA_MONADIC      151
#define CLASS_DEVA_MOVANIC      152
#define TOP_CLASS               153

#define RACE_UNDEFINED  -1
#define RACE_HUMAN    	 0
#define RACE_ELF   	 1
#define RACE_DWARF       2
#define RACE_HALF_ORC    3
#define RACE_KLINGON     4
#define RACE_HAFLING     5
#define RACE_TABAXI      6
#define RACE_HELP        7
#define RACE_MOBILE      10
#define RACE_UNDEAD      11
#define RACE_HUMANOID    12
#define RACE_ANIMAL      13
#define RACE_DRAGON      14
#define RACE_GIANT       15
#define RACE_ORC	 16
#define RACE_GOBLIN	 17
#define RACE_HALFLING    18
#define RACE_MINOTAUR    19
#define RACE_TROLL       20
#define RACE_GOLEM       21
#define RACE_ELEMENTAL   22
#define RACE_OGRE        23
#define RACE_DEVIL       24
#define RACE_TROGLODYTE  25
#define RACE_MANTICORE   26
#define RACE_BUGBEAR     27
#define RACE_DRACONIAN   28
#define RACE_DUERGAR     29
#define RACE_SLAAD       30
#define RACE_ROBOT       31
#define RACE_DEMON       32
#define RACE_DEVA        33
#define RACE_PLANT       34
#define RACE_ARCHON      35
#define RACE_PUDDING     36
#define RACE_ALIEN_1     37
#define RACE_PRED_ALIEN  38
#define RACE_SLIME       39
#define RACE_ILLITHID    40
#define RACE_FISH        41
#define RACE_BEHOLDER    42
#define NUM_RACES        43

/* Hometown defines                            */
#define HOME_UNDEFINED   -1
#define HOME_MODRIAN      0        
#define HOME_NEW_THALOS   1
#define HOME_ELECTRO      2
#define HOME_ELVEN_VILLAGE 3
#define HOME_ISTAN        4
#define HOME_MONK         5
#define HOME_NEWBIE_TOWER 6
#define HOME_SOLACE_COVE  7
#define HOME_MAVERNAL     8
#define HOME_ARENA        10
#define HOME_CITY         11
#define HOME_DOOM         12
#define HOME_SKULLPORT       15
#define HOME_DWARVEN_CAVERNS 16
#define HOME_HUMAN_SQUARE    17
#define HOME_DROW_ISLE       18
#define NUM_HOMETOWNS        19

/* Deity Defines				*/
#define DEITY_NONE        0
#define DEITY_GUIHARIA    1
#define DEITY_PAN	  2
#define DEITY_JUSTICE     3
#define DEITY_ARES	  4
#define DEITY_KALAR       5


/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2


/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/
#define POS_FLYING     9        /* flying around        */
#define POS_MOUNTED    10
#define POS_SWIMMING   11


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_AFK         (1 << 16)  /* Player is away from keyboard      */
#define PLR_CLAN_LEADER (1 << 17)  /* The head of the respective clan   */
#define PLR_TOUGHGUY    (1 << 18)  /* Player is open to pk and psteal   */
#define PLR_OLC         (1 << 19)  /* Player is descripting olc         */
#define PLR_HALT        (1 << 20)  /* Player is halted                  */
#define PLR_OLCGOD      (1 << 21)  /* Player can edit at will           */
#define PLR_TESTER      (1 << 22)  /* Player is a tester                */
#define PLR_QUESTOR     (1 << 23)  /* Quest god                         */
#define PLR_MORTALIZED  (1 << 24)  /* God can be killed                 */
#define PLR_REMORT_TOUGHGUY (1 << 25) /* open to pk by remortz          */
#define PLR_COUNCIL     (1 << 26)
#define PLR_NOPOST      (1 << 27)

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 17) /* Mob can't be blinded		*/
#define MOB_NOTURN       (1 << 18) /* Hard to turn                      */
#define MOB_NOPETRI      (1 << 19) /* Cannot be petrified               */
#define NUM_MOB_FLAGS    20

#define MOB2_SCRIPT     (1 << 0)
#define MOB2_MOUNT      (1 << 1)
#define MOB2_STAY_SECT  (1 << 2)  /* Can't leave SECT_type.   */
#define MOB2_ATK_MOBS   (1 << 3)  /* Aggro Mobs will attack other mobs */
#define MOB2_HUNT	(1 << 4)  /* Mob will hunt attacker    */
#define MOB2_LOOTER	(1 << 5)  /* Loots corpses     */
#define MOB2_NOSTUN     (1 << 6)
#define MOB2_SELLER     (1 << 7)  /* If shopkeeper, sells anywhere. */
#define MOB2_WONT_WEAR	(1 << 8)  /* Wont wear shit it picks up (SHPKPER) */
#define MOB2_SILENT_HUNTER (1 << 9)
#define MOB2_FAMILIAR   (1 << 10)  /* mages familiar */
#define MOB2_NO_FLOW    (1 << 11) /* Mob doesnt flow */
#define MOB2_UNAPPROVED (1 << 12) /* Mobile not approved for game play */
#define MOB2_RENAMED    (1 << 13) /* Mobile renamed */
#define MOB2_NOAGGRO_RACE (1 << 14) /* wont attack members of own race */
#define NUM_MOB2_FLAGS  15

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_NOSNOOP     (1 << 22) /* Can not be snooped by immortals    */
#define PRF_NOMUSIC     (1 << 23) /* Can't hear music channel	        */
#define PRF_NOSPEW	(1 << 24) /* Can't hear spews			*/
#define PRF_GAGMISS     (1 << 25) /* Doesn't see misses during fight    */
#define PRF_NOPROJECT   (1 << 26) /* Cannot hear the remort channel     */
#define PRF_NOINTWIZ    (1 << 27)
#define PRF_NOCLANSAY   (1 << 28) /* Doesnt hear clan says and such     */
#define PRF_NOIDENTIFY  (1 << 29) /* Saving throw is made when id'd     */
#define PRF_NODREAM     (1 << 30)

#define PRF2_FIGHT_DEBUG   (1 << 0) /* Sees info on fight.              */
#define PRF2_NEWBIE_HELPER (1 << 1) /* sees newbie arrivals             */
#define PRF2_AUTO_DIAGNOSE (1 << 2) /* automatically see condition of enemy */
#define PRF2_AUTOPAGE      (1 << 3) /* Beeps when ch receives a tell    */
#define PRF2_NOAFFECTS     (1 << 4) /* Affects are not shown in score   */
#define PRF2_NOHOLLER      (1 << 5) /* Gods only                        */
#define PRF2_NOIMMCHAT     (1 << 6) /* Gods only                        */
#define PRF2_CLAN_TITLE    (1 << 7) /* auto-sets title to clan stuff */
#define PRF2_CLAN_HIDE     (1 << 8) /* don't show badge in who list */
#define PRF2_LIGHT_READ    (1 << 9) /* interrupts while d->showstr_point */
#define PRF2_AUTOPROMPT    (1 << 10) /* always draw new prompt */
#define PRF2_NOWHO         (1 << 11) /* don't show in who */
#define PRF2_ANONYMOUS     (1 << 12) /* dont show class, level */
#define PRF2_NOTRAILERS    (1 << 13) /* don't show trailer affects */
#define PRF2_VT100         (1 << 14) /* Players uses VT100 inferface */


/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_INFLIGHT	      (1 << 16)	   /* Room for future expansion	*/
#define AFF_TIME_WARP         (1 << 17)	   /* Room for future expansion	*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_WATERBREATH       (1 << 20)	   /* Room for future expansion	*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_CONFUSION         (1 << 22)    /* Char is confused     	*/
#define AFF_NOPAIN            (1 << 23)    /* Char feels no pain	*/
#define AFF_RETINA            (1 << 24)    /* Char's retina is stimulated*/
#define AFF_ADRENALINE        (1 << 25)    /* Char's adrenaline is pumping*/
#define AFF_CONFIDENCE        (1 << 26)    /* Char is confident       	*/
#define AFF_REJUV             (1 << 27)    /* Char is rejuvenating */
#define AFF_REGEN             (1 << 28)    /* Body is regenerating */
#define AFF_GLOWLIGHT         (1 << 29)    /* Light spell is operating   */
#define AFF_BLUR              (1 << 30)    /* Blurry image */
#define NUM_AFF_FLAGS         31

#define AFF2_FLUORESCENT	(1 << 0)
#define AFF2_TRANSPARENT	(1 << 1)
#define AFF2_SLOW               (1 << 2)
#define AFF2_HASTE		(1 << 3)
#define AFF2_MOUNTED		(1 << 4)   /*DO NOT SET THIS IN MOB FILE*/
#define AFF2_FIRE_SHIELD        (1 << 5)    /* affected by Fire Shield  */
#define AFF2_BESERK		(1 << 6)
#define AFF2_INTIMIDATED	(1 << 7)
#define AFF2_TRUE_SEEING        (1 << 8)
#define AFF2_HOLY_LIGHT		(1 << 9)
#define AFF2_PROTECT_UNDEAD	(1 << 10)
#define AFF2_INVIS_TO_UNDEAD	(1 << 11)
#define AFF2_INVIS_TO_ANIMALS   (1 << 12)
#define AFF2_ENDURE_COLD        (1 << 13)
#define AFF2_PARALYZED          (1 << 14)
#define AFF2_PROT_LIGHTNING     (1 << 15)
#define AFF2_PROT_FIRE          (1 << 16)
#define AFF2_TELEKINESIS        (1 << 17)  /* Char can carry more stuff */
#define AFF2_PROT_RAD		(1 << 18)  /* Enables Autoexits ! :)    */
#define AFF2_ABLAZE             (1 << 19)
#define AFF2_NECK_PROTECTED     (1 << 20)  /* Can't be beheaded         */
#define AFF2_DISPLACEMENT       (1 << 21)
#define AFF2_PROT_DEVILS        (1 << 22)
#define AFF2_MEDITATE           (1 << 23)
#define AFF2_EVADE              (1 << 24)
#define AFF2_BLADE_BARRIER      (1 << 25)
#define AFF2_OBLIVITY           (1 << 26)
#define AFF2_ENERGY_FIELD       (1 << 27)
#define AFF2_PETRIFIED          (1 << 28)
#define AFF2_VERTIGO            (1 << 29)
#define AFF2_PROT_DEMONS        (1 << 30)
#define NUM_AFF2_FLAGS          31

#define AFF3_TEST3		(1 << 0)
#define AFF3_FEEDING            (1 << 1)
#define AFF3_POISON_2           (1 << 2)
#define AFF3_POISON_3           (1 << 3)
#define AFF3_SICKNESS           (1 << 4)
#define AFF3_SELF_DESTRUCT      (1 << 5) /* Self-destruct sequence init */
#define AFF3_DAMAGE_CONTROL     (1 << 6) /* Damage control for cyborgs  */
#define AFF3_STASIS             (1 << 7) /* Borg is in static state     */
#define AFF3_PRISMATIC_SPHERE   (1 << 8) /* Defensive */
#define AFF3_RADIOACTIVE        (1 << 9)
#define AFF3_RAD_SICKNESS       (1 << 10)
#define AFF3_MANA_TAP           (1 << 11)
#define AFF3_ENERGY_TAP         (1 << 12)
#define AFF3_SONIC_IMAGERY      (1 << 13)
#define AFF3_SHROUD_OBSCUREMENT (1 << 14)
#define AFF3_NOBREATHE          (1 << 15)
#define AFF3_PROT_HEAT          (1 << 16)
#define AFF3_PSISHIELD          (1 << 17)
#define AFF3_PSYCHIC_CRUSH      (1 << 18)
#define NUM_AFF3_FLAGS          19

#define ARRAY_AFF_1        1
#define ARRAY_AFF_2        2
#define ARRAY_AFF_3	   3

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_QHOME        17             /* Hometown Query		*/
#define CON_RACE_PAST	 18		/* Racial Query			*/
#define CON_QALIGN       19		/* Alignment Query		*/
#define CON_QCOLOR       20		/* Start with color?            */
#define CON_QTIME_FRAME  21             /* Query for overall time frame */
#define CON_AFTERLIFE    22             /* After dies, before menu      */
#define CON_QHOME_PAST   23
#define CON_QHOME_FUTURE 24
#define CON_QREROLL      25             
#define CON_RACEHELP_P   26
#define CON_CLASSHELP_P  27
#define CON_HOMEHELP_P   28
#define CON_HOMEHELP_F   29
#define CON_RACE_FUTURE  30
#define CON_RACEHELP_F   31
#define CON_CLASSHELP_F  32
#define CON_NET_MENU1    50             /* First net menu state         */
#define CON_NET_PROGMENU1 51            /* State which class of skill   */
#define CON_NET_PROG_CYB  52            /* State which class of skill   */
#define CON_NET_PROG_MNK  53            /* State which class of skill   */
#define CON_NET_PROG_HOOD 54


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_CROTCH    18
#define WEAR_EYES      19
#define WEAR_BACK      20
#define WEAR_BELT      21
#define WEAR_FACE      22
#define WEAR_EAR_L     23
#define WEAR_EAR_R     24
#define WEAR_WIELD_2   25
#define WEAR_ASS       26
#define NUM_WEARS      27	/* This must be the # of eq positions!! */
#define WEAR_RANDOM    28


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
#define ITEM_UNUSED      60
#define ITEM_SCRIPT      61
#define NUM_ITEM_TYPES   62


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
#define ITEM_ATTRACTION_FIELD (1 << 27)
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
#define ITEM2_PROTECTED_HUNT    (1 << 28)
#define ITEM2_RENAMED           (1 << 29)
#define ITEM2_UNAPPROVED        (1 << 30)
#define NUM_EXTRA2_FLAGS         31


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intellegence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_SNEAK            25
#define APPLY_HIDE             26
#define APPLY_RACE	       27
#define APPLY_SEX	       28
#define APPLY_BACKSTAB	       29
#define APPLY_PICK_LOCK        30
#define APPLY_PUNCH	       31
#define APPLY_SHOOT	       32
#define APPLY_KICK	       33
#define APPLY_TRACK            34
#define APPLY_IMPALE           35
#define APPLY_BEHEAD           36
#define APPLY_THROWING         37   
#define APPLY_RIDING           38
#define APPLY_TURN             39
#define APPLY_SAVING_CHEM      40
#define APPLY_SAVING_PSI       41
#define NUM_APPLIES            42

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
#define NUM_LIQUID_TYPES 36


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* other #defined constants **********************************************/

#define LVL_GRIMP      71
#define LVL_LUCIFER    70
#define LVL_IMPL       69
#define LVL_ENTITY     68
#define LVL_CREATOR    67
#define LVL_GRGOD      66
#define LVL_TIMEGOD    65
#define LVL_DIETY      64
#define LVL_GOD	       63	/* Lesser God */
#define LVL_ENERGY     62
#define LVL_FORCE      61
#define LVL_POWER      60
#define LVL_BEING      59
#define LVL_SPIRIT     58
#define LVL_ELEMENT    57
#define LVL_DEMI       56
#define LVL_ETERNAL    55
#define LVL_ETHEREAL   54
#define LVL_LUMINARY   53
#define LVL_BUILDER    52
#define LVL_IMMORT     51
#define LVL_AMBASSADOR 50

#define LVL_FREEZE	LVL_GOD
#define LVL_CAN_BAN     LVL_GOD
#define LVL_VIOLENCE    LVL_POWER

#define NUM_OF_DIRS	NUM_DIRS

#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (4 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define SEG_VIOLENCE    (7)
#define FIRE_TICK       (3 RL_SEC)
#define PULSE_FLOWS     (1 RL_SEC)

#define SMALL_BUFSIZE		1024
#define LARGE_BUFSIZE		(12 * 1024)
#define GARBAGE_SPACE		32

#define MAX_STRING_LENGTH	65536
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		150 
#define MAX_POOF_LENGTH         256 /*Used in char_file_u *DO*NOT*CHANGE**/
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	60  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		700 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		96  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6 /* Used in obj_file_elem *DO*NOT*CHANGE* */


/***********************************************************************
 * Structures                                                          *
 **********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
typedef char			bool;
typedef char			byte;

typedef int	room_num;
typedef int	obj_num;

typedef struct Link {
  int          type;
  int          flags;
  void         *object;
  struct Link  *prev;
  struct Link  *next;
} Link;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
  char	*keyword;                 /* Keyword in look/examine          */
  char	*description;             /* What to see                      */
  struct extra_descr_data *next; /* Next in list                      */
};

struct special_search_data {
  char *command_keys;             /* which command activates          */
  char *keywords;                 /* Keywords which activate the command */
  char *to_vict;
  char *to_room;
  char *to_remote;
  byte command;
  int flags;
  int arg[3];
  struct special_search_data *next;
};

/* object-related structures ******************************************/


/* object flags; used in obj_data */
struct obj_flag_data {
   int	value[4];	/* Values of the item (see list)    */
   byte type_flag;	/* Type of item			    */
   int	wear_flags;	/* Where you can wear it	    */
   int	extra_flags;	/* If it hums, glows, etc.	    */
   int  extra2_flags;   /* More of the same...              */
   int	weight;		/* Weight what else                 */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   long	bitvector[3];	/* To set chars bits                */
   int  material;       /* material object is made of */
   int  max_dam;
   int  damage;
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;      /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   struct room_data *in_room;	/* In what room -1 when conta/carr    */
   int cur_flow_pulse;          /* Keep track of flowing pulse        */

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   struct obj_shared_data *shared;
   sh_int worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */
   struct obj_data *aux_obj;      /* for special usage                */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
};
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
  byte  worn_on_position;
  byte  type;
  byte  sparebyte1;
  byte  sparebyte2;
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
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;   /* When look DIR.			*/
   char	*keyword;		/* for open/close			*/

   sh_int exit_info;		/* Exit info				*/
   obj_num key;			/* Key's number (-1 for no key)		*/
   struct room_data *to_room;   /* Pointer to room                    */
};

struct room_affect_data {
  char *description;
  byte level;
  int flags;
  byte type;
  byte duration;
  struct room_affect_data *next;
};

/* ================== Memory Structure for room ======================= */
struct room_data {
   room_num number;		/* Rooms number	(vnum)		      */
   int	sector_type;            /* sector type (move/hide)            */
   char *name;			/* You are...			      */
   char	*description;           /* Shown when entered                 */
   char *sounds;		/* Sounds in the room		      */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   struct special_search_data *search; /* Specials to be searched     */
   struct room_affect_data *affects; /* temp. room affects            */
   int room_flags;		/* DEATH,DARK ... etc                 */
   sh_int max_occupancy;        /* Maximum Occupancy of Room          */

   byte light;                  /* Number of lightsources in room     */
   byte flow_dir;               /* Direction of flow                  */
   byte flow_speed;             /* Speed of flow                      */
   byte flow_type;              /* Type of flow                       */
   struct ticl_data *ticl_ptr;  /* Pointer to TICL procedure          */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
   struct zone_data *zone;      /* zone the room is in                */
   struct room_data *next;
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   byte hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   sh_int char_class;       /* PC / NPC's class		       */
   sh_int remort_class; /* PC / NPC REMORT CLASS (-1 for none) */
   sh_int weight;      /* PC / NPC's weight                    */
   sh_int height;      /* PC / NPC's height                    */
   sh_int hometown;    /* PC s Hometown (zone)                 */
   byte sex;           /* PC / NPC's sex                       */
   byte race;          /* PC / NPC's race   		       */
   byte level;         /* PC / NPC's level                     */
   byte age_adjust;    /* PC age adjust to maintain sanity     */
   struct time_data time;  /* PC's AGE in days                 */
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;
   sbyte str_add;      /* 000 - 100 if strength 18             */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max move for PC/NPC			   */
   sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   sh_int armor;        /* Internal -100..100, external -10..10 AC */
   int	gold;           /* Money carried                           */
   int	bank_gold;	/* Gold the char has in a bank account	   */
   int  credits;	/* Internal net credits			*/
   int	exp;            /* The experience of the player            */

   sbyte hitroll;       /* Any bonus or penalty to the hit roll    */
   sbyte damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   long	act;			/* act flag for NPC's; player flag for PC's */
   long act2;

   long	affected_by;		/* Bitvector for spells/skills affected by */
   long affected2_by;
   long affected3_by;

   sh_int apply_saving_throw[10]; /* Saving throw (Bonuses)		*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
  struct char_data *fighting;	/* Opponent				*/
  struct char_data *hunting;	/* Char hunted by this char		*/
  struct char_data *mounted;	/* MOB mounted by this char		*/
  
  int  carry_weight;	      /* Carried weight		         	*/
  int  worn_weight;	      /* Total weight equipped		        */
  int  timer;		      /* Timer for update			*/
  int  meditate_timer;        /* How long has been meditating           */
  int  cur_flow_pulse;        /* Keeps track of whether char has flowed */

  byte aux_counter;            /* for breathing and falling              */
  byte position;	      /* Standing, fighting, sleeping, etc.	*/
  byte carry_items;	      /* Number of items carried		*/
  byte weapon_proficiency;    /* Scale of learnedness of weapon prof.   */
  
  struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte PADDING0;		/* used to be spells_to_learn		*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	wimp_level;		/* Below this # of hit points, flee!	*/
   byte freeze_level;		/* Level of god who froze char, if any	*/
   sh_int invis_level;		/* level of invisibility		*/
   room_num load_room;		/* Which room to place char in		*/
   long	pref;			/* preference flags for PC's.		*/
   long pref2;                  /* 2nd pref flag                        */
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte clan;
   ubyte hold_home;
   ubyte remort_invis_level;
   ubyte broken_component;
   ubyte remort_generation;
   ubyte spare5;
   int deity;
   int spells_to_learn;
   int life_points;
   int pkills;
   int mobkills;
   int deaths;
   int old_class;               /* Type of borg, or class before vamprism. */
   int page_length;
   int total_dam;
   int columns;
   int spare16;
   long	spare17;
   long	spare18;
   long	spare19;
   long	spare20;
   long	spare21;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
  struct player_special_data_saved saved;
  char	*poofin;		/* Description on arrival of a god.     */
  char	*poofout;		/* Description upon a god's exit.       */
  struct alias *aliases;	/* Character's aliases			*/
  long last_tell;		/* idnum of last tell from		*/
  struct obj_data *olc_obj;     /* which obj being edited               */
  struct char_data *olc_mob;    /* which mob being edited               */
  struct shop_data *olc_shop;   /* which shop being edited              */
  struct olc_help_r *olc_help;  /* which help record being edited       */
  struct special_search_data *olc_srch;      /* which srch being edited */
  struct ticl_data *olc_ticl;   /* which ticl being edited              */
  struct room_data *was_in_room;/* location for linkdead people         */
};

struct obj_shared_data {
  int vnum;
  int number;
  int house_count;
  struct obj_data *proto;     /* pointer to prototype */
  struct ticl_data *ticl_ptr; /* Pointer to TICL procedure */
  SPECIAL(*func);
};

struct mob_shared_data {
  int vnum;
  int number;
  int  attack_type;           /* The Attack Type integer for NPC's     */
  byte default_pos;           /* Default position for NPC              */
  byte damnodice;             /* The number of damage dice's	       */
  byte damsizedice;           /* The size of the damage dice's         */
  byte morale;
  struct char_data *proto;    /* pointer to prototype */
  struct ticl_data *ticl_ptr; /* Pointer to TICL procedure */
  SPECIAL(*func);
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;	    /* List of attackers to remember	       */
   struct extra_descr_data *response;  /* for response processing */
   struct mob_shared_data *shared;
   int wait_state;	    /* Wait state for bashed mobs	       */
   byte last_direction;     /* The last direction the monster went     */

};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   sh_int modifier;       /* This is added to apropriate ability     */
   sh_int location;      /* Tells which ability to change(APPLY_XXX)*/
   long	bitvector;       /* Tells which bits to set (AFF_XXX)       */
   int aff_index;

   struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
  int pfilepos;			 /* playerfile pos		  */
  struct room_data *in_room;            /* Location (real room number)	  */

  struct char_player_data player;       /* Normal data                   */
  struct char_ability_data real_abils;	 /* Abilities without modifiers   */
  struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
  struct char_point_data points;        /* Points                        */
  struct char_special_data char_specials;	/* PC/NPC specials	  */
  struct player_special_data *player_specials; /* PC specials		  */
  struct mob_special_data mob_specials;	/* NPC specials		  */
  
  struct affected_type *affected;       /* affected by what spells       */
  struct obj_data *equipment[NUM_WEARS];/* Equipment array               */
  struct obj_data *implants[NUM_WEARS];

  struct obj_data *carrying;            /* Head of list                  */
  struct descriptor_data *desc;         /* NULL for mobiles              */

  struct char_data *next_in_room;     /* For room->people - list         */
  struct char_data *next;             /* For either monster or ppl-list  */
  struct char_data *next_fighting;    /* For fighting list               */
  
  struct follow_type *followers;        /* List of chars followers       */
  struct char_data *master;             /* Who is char following?        */
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   char poofin[MAX_POOF_LENGTH];
   char poofout[MAX_POOF_LENGTH];
   sh_int char_class;
   sh_int remort_class;
   sh_int weight;
   sh_int height;
   sh_int hometown;
   byte sex;
   byte race;
   byte level;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};

struct mail_recipient_data {
  long recpt_idnum;                /* Idnum of char to recieve mail  */
  struct mail_recipient_data *next; /*pointer to next in recpt list. */
};

struct descriptor_data {
   int	descriptor;		/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   int	connected;		/* mode of 'connectedness'		*/
   int	wait;			/* wait for how many loops		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char	*showstr_head;		/* for paging through texts		*/
   char	*showstr_point;		/*		-			*/
   char	**str;			/* for the modify-str system		*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte	prompt_mode;		/* control of prompt-printing		*/
   int	max_str;		/*		-			*/
   int  repeat_cmd_count;       /* how many times has this command been */
   int  editor_mode;            /* Flag if char is in editor            */
   int  editor_cur_lnum;        /* Current line number for editor       */
   char *editor_file;           /* Original line number for editor      */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char last_argument[MAX_INPUT_LENGTH]; /* */
   char output_broken;
   char *output;		/* ptr to the current output buffer	*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   int  last_cmd;
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct mail_recipient_data *mail_to;	/* list of names for mailsystem	*/
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
  sh_int hitp;
  sh_int shock;
};


struct weather_data {
  int  pressure;	/* How is the pressure ( Mb ) */
  int  change;	        /* How fast and what way does it change. */
  byte sky;	        /* How is the sky. */
  byte sunlight;	/* And how much sun. */
  byte temp;            /* temperature */
  byte humid;           /* humidity */
};


struct title_type {
   char	*title_m;
   char	*title_f;
};


struct olc_help_r {
  char *keyword;
  long  pos;
  char *text;
};


struct ticl_data {
  int    vnum;             /* VNUM of this proc */
  long   proc_id;          /* IDNum of this proc */
  long   creator;          /* IDNum of the creator */
  time_t date_created;     /* Date proc created */
  time_t last_modified;    /* Date last modified */
  long   last_modified_by; /* IDNum of char to last modify */
  int    times_executed;   /* Number of times proc run */
  int    flags;            /* MOB/OBJ/ROOM/ZONE/!APPROVED */
  int    compiled;         /* Indicates successfull interpretation */
  char  *title;            /* Name of proc */
  char  *code;             /* TICL instructions (code) */
  struct ticl_data *next;  /* Pointer to next TICL proc */
};
