//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __room_data_h__
#define __room_data_h__

/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define FUTURE         6
#define PAST           7
#define NUM_DIRS       8

/* Door reset-states */
#define DOOR_OPEN               (1 << 0)
#define DOOR_CLOSED             (1 << 1)
#define DOOR_LOCKED             (1 << 2)
#define DOOR_HIDDEN             (1 << 3)


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK                 (1 << 0)   /* Dark                        */
#define ROOM_DEATH                (1 << 1)   /* Death trap                */
#define ROOM_NOMOB                (1 << 2)   /* MOBs not allowed                */
#define ROOM_INDOORS              (1 << 3)   /* Indoors                        */
#define ROOM_PEACEFUL             (1 << 4)   /* Violence not allowed        */
#define ROOM_SOUNDPROOF           (1 << 5)   /* Shouts, gossip blocked        */
#define ROOM_NOTRACK              (1 << 6)   /* Track won't go through        */
#define ROOM_NOMAGIC              (1 << 7)   /* Magic not allowed                */
#define ROOM_TUNNEL               (1 << 8)   /* room for only 1 pers        */
#define ROOM_NOTEL                (1 << 9)   /* Can't teleport in                */
#define ROOM_GODROOM              (1 << 10)  /* LVL_GOD+ only allowed        */
#define ROOM_HOUSE                (1 << 11)  /* (R) Room is a house        */
#define ROOM_HOUSE_CRASH          (1 << 12)  /* (R) House needs saving        */
#define ROOM_ATRIUM               (1 << 13)  /* (R) The door to a house        */
#define ROOM_SMOKE_FILLED         (1 << 14)  /* Room is filled with smoke */
#define ROOM_NOFLEE               (1 << 15) // room resists fleeing
#define ROOM_NOPSIONICS           (1 << 16)  /* No psionics allowed       */
#define ROOM_NOSCIENCE            (1 << 17)  /* No physical alterations   */
#define ROOM_NOPHYSIC             ROOM_NOSCIENCE
#define ROOM_NORECALL             (1 << 18)  /* Recall spell fails        */
#define ROOM_CLAN_HOUSE           (1 << 19)  /* Clan members only         */
#define ROOM_ARENA                (1 << 20)  /* PK Arena (no exp transfer)*/
#define ROOM_DOCK                 (1 << 21)  /* Boats may be docked here  */
#define ROOM_FLAME_FILLED         (1 << 22)  /* Sets chars on fire        */
#define ROOM_ICE_COLD             (1 << 23)  /* Freezes chars             */
#define ROOM_NULL_MAGIC           (1 << 24)  /* Nullifies magical effects */
#define ROOM_HOLYOCEAN            (1 << 25)  /* */
#define ROOM_RADIOACTIVE          (1 << 26)  /* */
#define ROOM_SLEEP_GAS            (1 << 27)  /* sleepy room */
#define ROOM_EXPLOSIVE_GAS        (1 << 28)  /* explodes */
#define ROOM_POISON_GAS           (1 << 29)  /* poisons char */
#define ROOM_VACUUM               (1 << 30) // no breathable air
#define NUM_ROOM_FLAGS          32


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR               (1 << 0)   /* Exit is a door                */
#define EX_CLOSED               (1 << 1)   /* The door is closed        */
#define EX_LOCKED               (1 << 2)   /* The door is locked        */
#define EX_PICKPROOF            (1 << 3)   /* Lock can't be picked        */
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
#define EX_WALL_FORCE           (1 << 22)
#define EX_SPECIAL              (1 << 23)
#define EX_REINFORCED           (1 << 24)
#define EX_SECRET               (1 << 25)
#define NUM_DOORFLAGS           26


/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0                   /* Indoors                        */
#define SECT_CITY            1                   /* In a city                        */
#define SECT_FIELD           2                   /* In a field                */
#define SECT_FOREST          3                   /* In a forest                */
#define SECT_HILLS           4                   /* In the hills                */
#define SECT_MOUNTAIN        5                   /* On a mountain                */
#define SECT_WATER_SWIM      6                   /* Swimmable water                */
#define SECT_WATER_NOSWIM    7                   /* Water - need a boat        */
#define SECT_UNDERWATER      8                   /* Underwater                */
#define SECT_FLYING          9                   /* Wheee!                        */
#define SECT_NOTIME         10             /* Between Times             */
#define SECT_CLIMBING       11             /* Climbing skill required   */
#define SECT_FREESPACE      12             /* Out of the atmosphere     */
#define SECT_ROAD           13                   /* On the road                       */
#define SECT_VEHICLE        14                   /* In a car                        */
#define SECT_CORNFIELD      15                   /* In the corn               */
#define SECT_SWAMP          16                   /* Swamp                        */
#define SECT_DESERT         17                   /* Sandy and hot                */
#define SECT_FIRE_RIVER     18
#define SECT_JUNGLE         19
#define SECT_PITCH_PIT      20
#define SECT_PITCH_SUB      21
#define SECT_BEACH          22
#define SECT_ASTRAL         23
#define SECT_ELEMENTAL_FIRE        24
#define SECT_ELEMENTAL_EARTH       25
#define SECT_ELEMENTAL_AIR         26
#define SECT_ELEMENTAL_WATER       27
#define SECT_ELEMENTAL_POSITIVE    28
#define SECT_ELEMENTAL_NEGATIVE    29
#define SECT_ELEMENTAL_SMOKE       30
#define SECT_ELEMENTAL_ICE         31
#define SECT_ELEMENTAL_OOZE        32
#define SECT_ELEMENTAL_MAGMA       33
#define SECT_ELEMENTAL_LIGHTNING   34
#define SECT_ELEMENTAL_STEAM       35
#define SECT_ELEMENTAL_RADIANCE    36
#define SECT_ELEMENTAL_MINERALS    37
#define SECT_ELEMENTAL_VACUUM      38
#define SECT_ELEMENTAL_SALT        39
#define SECT_ELEMENTAL_ASH         40
#define SECT_ELEMENTAL_DUST        41
#define SECT_BLOOD          42
#define NUM_SECT_TYPES      43



#define NUM_OF_DIRS        NUM_DIRS

/* room-related structures ************************************************/


struct room_direction_data {
    char        *general_description;   // When look DIR.
    char        *keyword;                // for open/close

    unsigned int exit_info;        // Exit info
    obj_num key;                        // Key's number (-1 for no key)
    struct room_data *to_room;   // Pointer to room
};

struct room_affect_data {
    char *description;
    byte level;
    int flags;
    byte type;
    byte duration;
    struct room_affect_data *next;
};

#define TRAIL_EXIT 0
#define TRAIL_ENTER 1

#define TRAIL_FLAG_BLOODPRINTS    (1 << 0)
#define TRAIL_FLAG_BLOOD_DROPS    (1 << 1)

struct room_trail_data {
    char *name;        /* namelist of trail owner, in case they're gone */
    time_t time;       /* time char passed by */
    int idnum;         /* idnum of char (negative for mobiles) */
    char from_dir;      /* direction from which character entered */
    char to_dir;        /* direction to which character left */
    char track;         /* strength of physical trail (0 - 10) */
    int flags;         /* misc flags */
    struct room_trail_data *next;
};
  
/* ================== Memory Structure for room ======================= */
class room_data {
    public: // methods
        room_data( room_num n = -1, struct zone_data *z = NULL );
        bool isOpenAir( void );
    public: // members
        room_num number;  // Rooms number (vnum)
        int sector_type;  // sector type (move/hide)

        char *name;       // You are...
        char *description;// Shown when entered
        char *sounds;     // Sounds in the room
        struct extra_descr_data *ex_description; // for examine/look
        struct room_direction_data *dir_option[NUM_OF_DIRS]; // Directions
        struct special_search_data *search; // Specials to be searched
        struct room_affect_data *affects;   // temp. room affects
        struct room_trail_data *trail; // tracking data 
        int room_flags;                // DEATH,DARK ... etc
        sh_int max_occupancy;          // Maximum Occupancy of Room
        
        unsigned char find_first_step_index;

        byte light;                  // Number of lightsources in room
        byte flow_dir;               // Direction of flow
        byte flow_speed;             // Speed of flow
        byte flow_type;              // Type of flow
        struct ticl_data *ticl_ptr;  // Pointer to TICL procedure
        SPECIAL(*func);
        struct zone_data *zone;      // zone the room is in
        struct room_data *next;

        struct obj_data *contents;   // List of items in room
    //private:
        CharacterList people;    // List of NPC / PC in room        
};
//these structs used in do_return()
extern struct room_data * r_immort_start_room;
extern struct room_data * r_frozen_start_room;
extern struct room_data * r_mortal_start_room;
extern struct room_data * r_electro_start_room;
extern struct room_data * r_new_thalos_start_room;
extern struct room_data * r_elven_start_room;
extern struct room_data * r_istan_start_room;
extern struct room_data * r_arena_start_room;
extern struct room_data * r_city_start_room;
extern struct room_data * r_doom_start_room;
extern struct room_data * r_monk_start_room;
extern struct room_data * r_tower_modrian_start_room;
extern struct room_data * r_solace_start_room;
extern struct room_data * r_mavernal_start_room;
extern struct room_data * r_dwarven_caverns_start_room;
extern struct room_data * r_human_square_start_room;
extern struct room_data * r_skullport_start_room;
extern struct room_data * r_skullport_newbie_start_room;
extern struct room_data * r_drow_isle_start_room;
extern struct room_data * r_astral_manse_start_room;
extern struct room_data * r_zul_dane_start_room;
extern struct room_data * r_zul_dane_newbie_start_room;
extern struct room_data * r_newbie_school_start_room;

#endif 
