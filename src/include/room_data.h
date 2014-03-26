#ifndef _ROOM_DATA_H_
#define _ROOM_DATA_H_

//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/* room-related defines *************************************************/

/* The cardinal directions: used as index to struct room_data.dir_option[] */

struct creature;
struct obj_list;

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

/* Room flags: used in struct room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK                 (1 << 0)	/* Dark                        */
#define ROOM_DEATH                (1 << 1)	/* Death trap                */
#define ROOM_NOMOB                (1 << 2)	/* MOBs not allowed                */
#define ROOM_INDOORS              (1 << 3)	/* Indoors                        */
#define ROOM_PEACEFUL             (1 << 4)	/* Violence not allowed        */
#define ROOM_SOUNDPROOF           (1 << 5)	/* Shouts, gossip blocked        */
#define ROOM_NOTRACK              (1 << 6)	/* Track won't go through        */
#define ROOM_NOMAGIC              (1 << 7)	/* Magic not allowed                */
#define ROOM_TUNNEL               (1 << 8)	/* room for only 1 pers        */
#define ROOM_NOTEL                (1 << 9)	/* Can't teleport in                */
#define ROOM_GODROOM              (1 << 10)	/* LVL_GOD+ only allowed        */
#define ROOM_HOUSE                (1 << 11)	/* (R) Room is a house        */
#define ROOM_HOUSE_CRASH          (1 << 12)	/* (R) House needs saving        */
#define ROOM_COMFORT              (1 << 13)	// Players refresh faster
#define ROOM_SMOKE_FILLED         (1 << 14)	/* Room is filled with smoke */
#define ROOM_NOFLEE               (1 << 15)	// room resists fleeing
#define ROOM_NOPSIONICS           (1 << 16)	/* No psionics allowed       */
#define ROOM_NOSCIENCE            (1 << 17)	/* No physical alterations   */
#define ROOM_NOPHYSIC             ROOM_NOSCIENCE
#define ROOM_NORECALL             (1 << 18)	/* Recall spell fails        */
#define ROOM_CLAN_HOUSE           (1 << 19)	/* Clan members only         */
#define ROOM_ARENA                (1 << 20)	/* PK Arena (no exp transfer) */
#define ROOM_DOCK                 (1 << 21)	/* Boats may be docked here  */
#define ROOM_FLAME_FILLED         (1 << 22)	/* Sets chars on fire        */
#define ROOM_ICE_COLD             (1 << 23)	/* Freezes chars             */
#define ROOM_NULL_MAGIC           (1 << 24)	/* Nullifies magical effects */
#define ROOM_HOLYOCEAN            (1 << 25)	/* */
#define ROOM_RADIOACTIVE          (1 << 26)	/* */
#define ROOM_SLEEP_GAS            (1 << 27)	/* sleepy room */
#define ROOM_EXPLOSIVE_GAS        (1 << 28)	/* explodes */
#define ROOM_POISON_GAS           (1 << 29)	/* poisons char */
#define ROOM_VACUUM               (1 << 30)	// no breathable air
#define NUM_ROOM_FLAGS          32

/* Exit info: used in struct room_data.dir_option.exit_info */
#define EX_ISDOOR               (1 << 0)	/* Exit is a door                */
#define EX_CLOSED               (1 << 1)	/* The door is closed        */
#define EX_LOCKED               (1 << 2)	/* The door is locked        */
#define EX_PICKPROOF            (1 << 3)	/* Lock can't be picked        */
#define EX_HEAVY_DOOR           (1 << 4)	/* Door requires high STR    */
#define EX_HARD_PICK            (1 << 5)
#define EX_NOMOB                (1 << 6)	/* Mobs will not pass        */
#define EX_HIDDEN               (1 << 7)	/* Door revealed by search   */
#define EX_NOSCAN               (1 << 8)	/* Direction is hard to scan */
#define EX_TECH                 (1 << 9)	/* can't be picked convention */
#define EX_ONEWAY               (1 << 10)	/* bomb blasts are blocked */
#define EX_NOPASS               (1 << 11)	/* keep players out */
#define EX_WALL_THORNS          (1 << 12)	/* can get thru */
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

/* Sector types: used in struct room_data.sector_type */
#define SECT_INSIDE          0	/* Indoors                        */
#define SECT_CITY            1	/* In a city                        */
#define SECT_FIELD           2	/* In a field                */
#define SECT_FOREST          3	/* In a forest                */
#define SECT_HILLS           4	/* In the hills                */
#define SECT_MOUNTAIN        5	/* On a mountain                */
#define SECT_WATER_SWIM      6	/* Swimmable water                */
#define SECT_WATER_NOSWIM    7	/* Water - need a boat        */
#define SECT_UNDERWATER      8	/* Underwater                */
#define SECT_FLYING          9	/* Wheee!                        */
#define SECT_NOTIME         10	/* Between Times             */
#define SECT_CLIMBING       11	/* Climbing skill required   */
#define SECT_FREESPACE      12	/* Out of the atmosphere     */
#define SECT_ROAD           13	/* On the road                       */
#define SECT_VEHICLE        14	/* In a car                        */
#define SECT_FARMLAND       15	/* In the corn               */
#define SECT_SWAMP          16	/* Swamp                        */
#define SECT_DESERT         17	/* Sandy and hot                */
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
#define SECT_ROCK                  43
#define SECT_MUDDY                 44
#define SECT_TRAIL                 45
#define SECT_TUNDRA                46
#define SECT_CATACOMBS             47
#define SECT_CRACKED_ROAD          48
#define SECT_DEEP_OCEAN            49
#define NUM_SECT_TYPES      50

#define NUM_OF_DIRS        NUM_DIRS

/* room-related structures ************************************************/

struct room_direction_data {
	char *general_description;	// When look DIR.
	char *keyword;				// for open/close

	unsigned int exit_info;		// Exit info
	obj_num key;				// Key's number (-1 for no key)
    int16_t damage;             // Damage to door (-1 for unbreakable)
    int16_t maxdam;             // Durability of door (-1 for unbreakable)
	struct room_data *to_room;	// Pointer to room
};

struct room_affect_data {
	char *description;
	int8_t level;
	int flags;
	int8_t type;
	int duration;
    int val[4];
  int owner;
    int spell_type;
	struct room_affect_data *next;
};

#define TRAIL_EXIT 0
#define TRAIL_ENTER 1

#define TRAIL_FLAG_BLOODPRINTS    (1 << 0)
#define TRAIL_FLAG_BLOOD_DROPS    (1 << 1)

struct room_trail_data {
	char *name;					/* namelist of trail owner, in case they're gone */
	char *aliases;				// aliases of trail owner
	time_t time;				/* time char passed by */
	int idnum;					/* idnum of char (negative for mobiles) */
	char from_dir;				/* direction from which character entered */
	char to_dir;				/* direction to which character left */
	char track;					/* strength of physical trail (0 - 10) */
	int flags;					/* misc flags */
	struct room_trail_data *next;
};

/* ================== Memory Structure for room ======================= */
struct room_data {
    room_num number;			// Rooms number (vnum)
	int sector_type;			// sector type (move/hide)

	char *name;					// You are...
	char *description;			// Shown when entered
	char *sounds;				// Sounds in the room
	char *prog;
    unsigned char *progobj;
    size_t progobj_len;
    int prog_marker;
    struct prog_state_data *prog_state;
	struct extra_descr_data *ex_description;	// for examine/look
	/*@dependent@*/ struct room_direction_data *dir_option[NUM_OF_DIRS];	// Directions
	struct special_search_data *search;	// Specials to be searched
	struct room_affect_data *affects;	// temp. room affects
	struct room_trail_data *trail;	// tracking data
	int room_flags;				// DEATH,DARK ... etc
	int16_t max_occupancy;		// Maximum Occupancy of Room

	unsigned char find_first_step_index;

	int8_t light;					// Number of lightsources in room
	int8_t flow_dir;				// Direction of flow
	int8_t flow_speed;              // Speed of flow
	int8_t flow_type;				// Type of flow
    int8_t flow_pulse;              // Pulse to track flow speed
	SPECIAL((*func));
	char *func_param;
	struct zone_data *zone;		// zone the room is in
	struct room_data *next;
	struct obj_data *contents;	// List of items in room
	GList *people;		// List of NPC / PC in room
};

static inline int
SECT(struct room_data * room)
{
	return room->sector_type;
}

static inline bool
room_is_open_air(struct room_data *room)
{
    int sect = room->sector_type;
    return (sect == SECT_FLYING ||
            sect == SECT_ELEMENTAL_AIR ||
            sect == SECT_ELEMENTAL_RADIANCE ||
            sect == SECT_ELEMENTAL_LIGHTNING ||
            sect == SECT_ELEMENTAL_VACUUM);
}

static inline bool
room_is_watery(struct room_data *room)
{
    int sect = room->sector_type;
    return (sect == SECT_WATER_NOSWIM ||
            sect == SECT_WATER_SWIM ||
            sect == SECT_UNDERWATER ||
            sect == SECT_ELEMENTAL_WATER ||
            sect == SECT_DEEP_OCEAN);
}

static inline bool
room_is_underwater(struct room_data *room)
{
    int sect = room->sector_type;
    return (sect == SECT_UNDERWATER ||
            sect == SECT_ELEMENTAL_WATER ||
            sect == SECT_DEEP_OCEAN);
}

static inline bool
room_has_air(struct room_data *room)
{
    int sect = room->sector_type;
    if (room_is_underwater(room))
        return false;

    if (sect == SECT_PITCH_SUB
        || sect == SECT_ELEMENTAL_OOZE
        || sect == SECT_WATER_NOSWIM
        || sect == SECT_FREESPACE)
        return false;

    return true;
}

bool room_is_sunny(struct room_data *room);
bool room_is_dark(struct room_data *room);
bool room_is_light(struct room_data *room);

static inline /*@dependent@*/ struct room_direction_data*
ABS_EXIT( struct room_data *room, int dir ) {
	return room->dir_option[dir];
}

//these structs used in do_return()
extern struct room_data *r_immort_start_room;
extern struct room_data *r_frozen_start_room;
extern struct room_data *r_mortal_start_room;
extern struct room_data *r_electro_start_room;
extern struct room_data *r_new_thalos_start_room;
extern struct room_data *r_kromguard_start_room;
extern struct room_data *r_elven_start_room;
extern struct room_data *r_istan_start_room;
extern struct room_data *r_arena_start_room;
extern struct room_data *r_monk_start_room;
extern struct room_data *r_tower_modrian_start_room;
extern struct room_data *r_solace_start_room;
extern struct room_data *r_mavernal_start_room;
extern struct room_data *r_dwarven_caverns_start_room;
extern struct room_data *r_human_square_start_room;
extern struct room_data *r_skullport_start_room;
extern struct room_data *r_skullport_newbie_start_room;
extern struct room_data *r_drow_isle_start_room;
extern struct room_data *r_astral_manse_start_room;
extern struct room_data *r_zul_dane_start_room;
extern struct room_data *r_zul_dane_newbie_start_room;
extern struct room_data *r_newbie_school_start_room;

int count_room_exits(struct room_data *room);
struct room_data *make_room(struct zone_data *zone, int num);
void free_room(struct room_data *room);
void link_rooms(struct room_data *room_a, struct room_data *room_b, int dir);
bool will_fit_in_room(struct creature *ch, struct room_data *room);

#endif
