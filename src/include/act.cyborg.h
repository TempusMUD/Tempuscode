//
// File: act.cyborg.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __act_cyborg_h__
#define __act_cyborg_h__

int max_component_dam(struct char_data *ch);
int room_count(struct char_data *ch, struct room_data *room);
int redundant_skillchip(struct obj_data *chip, struct obj_data *slot);
void engage_self_destruct(struct char_data *ch);

#define NUM_COMPS               9	/* Number of borg components */

#define BORG_POWER              0
#define BORG_SPEED              1
#define BORG_MENTANT            2

#define INTERFACE_TYPE(obj)     (GET_OBJ_VAL(obj, 0))
#define INTERFACE_MAX(obj)      (GET_OBJ_VAL(obj, 2))
#define INTERFACE_CUR(obj)      (GET_OBJ_VAL(obj, 3))

#define INTERFACE_NONE          0
#define INTERFACE_POWER         1
#define INTERFACE_CHIPS         2
#define NUM_INTERFACES          3

	 /**** microchip utils ****/

#define IS_CHIP(obj)           (GET_OBJ_TYPE(obj) == ITEM_MICROCHIP)
#define CHIP_TYPE(obj)         (GET_OBJ_VAL(obj, 0))
#define CHIP_DATA(obj)         (GET_OBJ_VAL(obj, 1))
#define CHIP_MAX(obj)          (GET_OBJ_VAL(obj, 2))

#define CHIP_NONE               0
#define CHIP_SKILL              1
#define CHIP_AFFECTS            2
#define NUM_CHIPS               3

#define SKILLCHIP(obj)         (CHIP_TYPE(obj) == CHIP_SKILL)
#define AFFCHIP(obj)           (CHIP_TYPE(obj) == CHIP_AFFECTS)


#define RECHARGABLE(obj)         (IS_VEHICLE(target) || IS_BATTERY(target) ||\
				  IS_ENERGY_CELL(target) ||  \
				  IS_TRANSPORTER(target) || \
				  IS_COMMUNICATOR(target) || \
				  IS_DEVICE(target))


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

#ifndef __act_cyborg_c__

extern const char *microchip_types[];
extern const char *interface_types[];
extern const char *borg_subchar_class_names[];
extern const char *component_names[][3];

#endif

#endif
