//
// File: vehicle.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __vehicle_h__
#define __vehicle_h__

struct obj_data *damage_eq(struct char_data *ch, 
			   struct obj_data *obj, int eq_dam, int type=-1);
struct obj_data *detonate_bomb(struct obj_data *bomb);
struct obj_data *find_vehicle(struct obj_data *v_door);

#define DOOR_STATE(car)      	(GET_OBJ_VAL(car, 1))
#define KEY_NUMBER(car)    	(GET_OBJ_VAL(car, 0))
#define ROOM_NUMBER(car)    	(GET_OBJ_VAL(car, 0))
#define CAR_FLAGS(car)          (GET_OBJ_VAL(car, 2))
#define CAR_SPECIAL(car)        (GET_OBJ_VAL(car, 3))
#define KEY_TO_CAR(key)         (GET_OBJ_VNUM(key))

#define CAR_OPENABLE(car)       (IS_SET(DOOR_STATE(car), CONT_CLOSEABLE))
#define CAR_CLOSED(car) 	(IS_SET(DOOR_STATE(car), CONT_CLOSED))
#define CAR_LOCKED(car) 	(IS_SET(DOOR_STATE(car), CONT_LOCKED))

#define ENG_OPERATIONAL  	(1 << 0)
#define ENG_PARK		(1 << 1)
#define ENG_RUN			(1 << 2)
#define ENG_LIGHTS		(1 << 3)
#define ENG_PETROL              (1 << 4)
#define ENG_ELECTRIC            (1 << 5)
#define ENG_MAGIC               (1 << 6)
#define NUM_ENGINE_FLAGS        7

#define MAX_ENERGY(engine) 	(GET_OBJ_VAL(engine, 0))
#define CUR_ENERGY(engine) 	(GET_OBJ_VAL(engine, 1))
#define ENGINE_STATE(engine) 	(GET_OBJ_VAL(engine, 2))
#define USE_RATE(engine) 	(GET_OBJ_VAL(engine, 3))
#define LOW_ENERGY(engine) 	(CUR_ENERGY(engine) < 20)

#define IS_ENGINE_SET(engine, bit) \
                                (IS_SET(ENGINE_STATE(engine), bit))
#define IS_ENGINE(obj)          (GET_OBJ_TYPE(obj) == ITEM_ENGINE)
#define WILL_RUN(engine)  	(IS_SET(ENGINE_STATE(engine), ENG_OPERATIONAL))
#define ENGINE_ON(engine) 	(IS_SET(ENGINE_STATE(engine), ENG_RUN))
#define IS_PARKED(engine) 	(IS_SET(ENGINE_STATE(engine), ENG_PARK))
#define HEADLIGHTS_ON(engine)   (IS_SET(ENGINE_STATE(engine), ENG_LIGHTS))

#define VEHICLE_ATV		(1 << 0)
#define VEHICLE_ROADCAR		(1 << 1)
#define VEHICLE_SKYCAR		(1 << 2)
#define VEHICLE_BOAT		(1 << 3)
#define VEHICLE_SPACESHIP	(1 << 4)
#define VEHICLE_ELEVATOR        (1 << 5)
#define NUM_VEHICLE_TYPES        6

#define IS_VEHICLE(obj)         (GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
#define IS_ATV(car)		(IS_SET(CAR_FLAGS(car), VEHICLE_ATV))
#define IS_ROADCAR(car)		(IS_SET(CAR_FLAGS(car), VEHICLE_ROADCAR))
#define IS_SKYCAR(car)		(IS_SET(CAR_FLAGS(car), VEHICLE_SKYCAR))
#define IS_BOAT(car)		(IS_SET(CAR_FLAGS(car), VEHICLE_BOAT))
#define IS_SPACESHIP(car)	(IS_SET(CAR_FLAGS(car), VEHICLE_SPACESHIP))
#define IS_ELEVATOR(car)	(IS_SET(CAR_FLAGS(car), VEHICLE_ELEVATOR))

#define IS_V_WINDOW(obj)        (GET_OBJ_TYPE(obj) == ITEM_V_WINDOW)
#define IS_V_DOOR(obj)          (GET_OBJ_TYPE(obj) == ITEM_V_DOOR)
#define IS_V_CONSOLE(obj)       (GET_OBJ_TYPE(obj) == ITEM_V_CONSOLE)
#define V_CAR_VNUM(obj)         (GET_OBJ_VAL(obj, 2))
#define V_CONSOLE_IDNUM(obj)    (GET_OBJ_VAL(obj, 3))

#define IS_BATTERY(obj)         (GET_OBJ_TYPE(obj) == ITEM_BATTERY)
#define RECH_RATE(batt)         (GET_OBJ_VAL(batt, 2))
#define COST_UNIT(batt)         (GET_OBJ_VAL(batt, 3))

#define IS_TRANSPORTER(obj)     (GET_OBJ_TYPE(obj) == ITEM_TRANSPORTER)
#define TRANS_TO_ROOM(obj)      (GET_OBJ_VAL(obj, 2))
#define IS_DEVICE(obj)          (GET_OBJ_TYPE(obj) == ITEM_DEVICE)
#define IS_IMPLANT(obj)         (IS_OBJ_STAT2(obj, ITEM2_IMPLANT))
#define IS_INTERFACE(obj)       (GET_OBJ_TYPE(obj) == ITEM_INTERFACE)
#define IS_ENERGY_CELL(obj)     (GET_OBJ_TYPE(obj) == ITEM_ENERGY_CELL)

#define IS_SYRINGE(obj)  (GET_OBJ_TYPE(obj) == ITEM_SYRINGE)
#define IS_VIAL(obj)     (GET_OBJ_TYPE(obj) == ITEM_VIAL)
#define IS_POTION(obj)     (GET_OBJ_TYPE(obj) == ITEM_POTION)

#define IS_TOOL(tool)     (OBJ_TYPE(tool, ITEM_TOOL))
#define TOOL_SKILL(tool)  (GET_OBJ_VAL(tool, 0))
#define TOOL_MOD(tool)    (GET_OBJ_VAL(tool, 1))

#define IS_COMMUNICATOR(obj) (GET_OBJ_TYPE(obj) == ITEM_COMMUNICATOR)
#define COMM_CHANNEL(obj)    (GET_OBJ_VAL(obj, 3))

#define COMM_UNIT_SEND_OK(ch, vict)                            \
     (GET_LEVEL(ch) >= LVL_IMMORT ||                           \
      (!ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) &&          \
       !ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF) &&        \
       (ch->in_room->zone == vict->in_room->zone ||            \
	(!ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED) &&    \
	 !ZONE_FLAGGED(vict->in_room->zone, ZONE_ISOLATED)))))

#define ERR_NONE       0
#define ERR_NULL_DEST  1
#define ERR_CLOSED_EX  2
#define ERR_NODRIVE    3
#define ERR_HOUSE      4
#define ERR_CLAN       5

#ifndef __vehicle_c__

extern const char *engine_flags[];
extern const char *engine_flags[];
extern const char *vehicle_types[];

#endif

#endif
