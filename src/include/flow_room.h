#ifndef _FLOW_ROOM_H_
#define _FLOW_ROOM_H_

//
// File: flow_room.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define F_TYPE_NONE              0
#define F_TYPE_WIND              1
#define F_TYPE_FALLING           2
#define F_TYPE_RIVER_SURFACE     3
#define F_TYPE_WATER_VORTEX      4
#define F_TYPE_UNDERWATER        5
#define F_TYPE_CONDUIT           6
#define F_TYPE_CONVEYOR          7
#define F_TYPE_LAVA_FLOW         8
#define F_TYPE_RIVER_FIRE        9
#define F_TYPE_VOLC_UPDRAFT      10
#define F_TYPE_ROTATING_DISC     11
#define F_TYPE_ESCALATOR         12
#define F_TYPE_SINKING_SWAMP     13
#define F_TYPE_UNSEEN_FORCE      14
#define F_TYPE_ELEMENTAL_WIND    15
#define F_TYPE_QUICKSAND         16
#define F_TYPE_CROWD             17

#define NUM_FLOW_TYPES           18

#define MSG_TORM_1    0
#define MSG_TORM_2    1
#define MSG_TOCHAR    2


#define RM_AFF_FLAGS     NUM_DIRS

void affect_to_room(struct room_data *room, struct room_affect_data *aff);
void affect_from_room(struct room_data *room, struct room_affect_data *aff);
struct room_affect_data *room_affected_by(struct room_data *room, int type);
#ifndef __flow_room_c__

extern const char *flow_types[];

#endif

#endif
