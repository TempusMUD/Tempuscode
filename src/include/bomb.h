#ifndef _BOMB_H_
#define _BOMB_H_

//
// File: bomb.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void sound_gunshots(struct room_data *rm, int type, int power, int num);
struct obj_data *detonate_bomb(struct obj_data *bomb);

/****** Bomb Utils **********************************************/

#define BOMB_UNDEFINED            0
#define BOMB_CONCUSSION           1
#define BOMB_FRAGMENTATION        2
#define BOMB_INCENDIARY           3
#define BOMB_DISRUPTION           4	/* electrical-undead disruption */
#define BOMB_NUCLEAR              5
#define BOMB_FLASH                6	/* blinds room */
#define BOMB_SMOKE                7
#define MAX_BOMB_TYPES            8

// reserved
#define BOMB_ARTIFACT             8

#define FUSE_NONE                 0
#define FUSE_BURN                 1
#define FUSE_ELECTRONIC           2
#define FUSE_REMOTE               3
#define FUSE_CONTACT              4
#define FUSE_MOTION               5

#define BOMB_TYPE(obj)            (GET_OBJ_VAL(obj, 0))
#define BOMB_POWER(obj)           (GET_OBJ_VAL(obj, 1))
#define BOMB_IDNUM(obj)           (GET_OBJ_VAL(obj, 3))

#define FUSE_TYPE(obj)            (GET_OBJ_VAL(obj, 0))
#define FUSE_STATE(obj)           (GET_OBJ_VAL(obj, 1))
#define FUSE_TIMER(obj)           (GET_OBJ_VAL(obj, 2))

#define BOMB_IS_CONCUSSION(obj)   (BOMB_TYPE(obj) == BOMB_CONCUSSION)
#define BOMB_IS_FRAG(obj)         (BOMB_TYPE(obj) == BOMB_FRAGMENTATION)
#define BOMB_IS_INCENDIARY(obj)   (BOMB_TYPE(obj) == BOMB_INCENDIARY)
#define BOMB_IS_DISRUPTION(obj)   (BOMB_TYPE(obj) == BOMB_DISRUPTION)
#define BOMB_IS_NUCLEAR(obj)      (BOMB_TYPE(obj) == BOMB_NUCLEAR)
#define BOMB_IS_FLASH(obj)        (BOMB_TYPE(obj) == BOMB_FLASH)
#define BOMB_IS_SMOKE(obj)        (BOMB_TYPE(obj) == BOMB_SMOKE)

#define FUSE_IS_BURN(obj)         (FUSE_TYPE(obj) == FUSE_BURN)
#define FUSE_IS_ELECTRONIC(obj)   (FUSE_TYPE(obj) == FUSE_ELECTRONIC)
#define FUSE_IS_REMOTE(obj)       (FUSE_TYPE(obj) == FUSE_REMOTE)
#define FUSE_IS_CONTACT(obj)      (FUSE_TYPE(obj) == FUSE_CONTACT)
#define FUSE_IS_MOTION(obj)       (FUSE_TYPE(obj) == FUSE_MOTION)

#define IS_BOMB(obj)              (IS_OBJ_TYPE(obj, ITEM_BOMB))
#define IS_DETONATOR(obj)         (IS_OBJ_TYPE(obj, ITEM_DETONATOR))
#define IS_FUSE(obj)              (IS_OBJ_TYPE(obj, ITEM_FUSE))

extern const char *bomb_types[];
extern const char *fuse_types[];

struct bomb_radius_list {
    struct room_data *room;
	struct bomb_radius_list *next;
	uint32_t power;
};

extern struct bomb_radius_list *bomb_rooms;

void add_bomb_room(struct room_data *room, int fromdir, int p_factor)
    __attribute__ ((nonnull));
void sort_rooms();
void bomb_damage_room(struct creature *damager, int damager_id, char *bomb_name, int bomb_type, int bomb_power,
    struct room_data *room, int dir, int power,
	struct creature *precious_vict)
    __attribute__ ((nonnull (3,6)));

#endif
