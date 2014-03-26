#ifndef _GUNS_H_
#define _GUNS_H_

//
// File: guns.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void show_gun_status(struct creature *ch, struct obj_data *gun)
    __attribute__ ((nonnull));

	 /*** energy gun utils ***/
#define IS_ENERGY_GUN(obj)      (IS_OBJ_TYPE(obj, ITEM_ENERGY_GUN))

#define IS_RIFLE(obj)           ((IS_OBJ_TYPE(obj, ITEM_GUN)) \
                                  && (GUN_TYPE(obj) == GUN_7mm_mag))
#define EGUN_MAX_ENERGY(obj)     ((obj->contains &&                 \
				   IS_ENERGY_CELL(obj->contains)) ?  \
				  MAX_ENERGY(obj->contains) : 0)
#define EGUN_CUR_ENERGY(obj)     ((obj->contains &&                 \
				   IS_ENERGY_CELL(obj->contains)) ?  \
				  CUR_ENERGY(obj->contains) : 0)

#define MAX_R_O_F(obj)       (GET_OBJ_VAL(obj, 0))
#define CUR_R_O_F(obj)       (GET_OBJ_VAL(obj, 1))
#define GUN_DISCHARGE(obj)   (GET_OBJ_VAL(obj, 0))

	 /** gun utils **/

#define GUN_TYPE(gun)            (GET_OBJ_VAL(gun, 3))
#define MAX_LOAD(gun)            (GET_OBJ_VAL(gun, 2))
#define BUL_DAM_MOD(gun)         (GET_OBJ_VAL(gun, 2))
#define IS_GUN(gun)              (IS_OBJ_TYPE(gun, ITEM_GUN))
#define IS_ANY_GUN(gun)          (IS_GUN(gun) || IS_ENERGY_GUN(gun))
#define IS_BULLET(gun)           (IS_OBJ_TYPE(gun, ITEM_BULLET))
#define IS_CLIP(gun)             (IS_OBJ_TYPE(gun, ITEM_CLIP))

#define GUN_LOADED(gun)          (gun->contains && \
				  (MAX_LOAD(gun) || gun->contains->contains))

#define GUN_NONE        0
#define GUN_22_cal      1
#define GUN_223_cal     2
#define GUN_25_cal      3
#define GUN_30_cal      4
#define GUN_357_cal     5
#define GUN_38_cal      6
#define GUN_40_cal      7
#define GUN_44_cal      8
#define GUN_45_cal      9
#define GUN_50_cal      10
#define GUN_555_cal     11
#define GUN_NAILGUN     12
#define GUN_410         13
#define GUN_20_gauge    14
#define GUN_16_gauge    15
#define GUN_12_gauge    16
#define GUN_10_gauge    17
#define GUN_7_mm        18
#define GUN_762_mm      19
#define GUN_9_mm        20
#define GUN_10_mm       21
#define GUN_7mm_mag     22
#define GUN_BOW         25
#define GUN_XBOW        26
#define GUN_FLAMETHROWER 30
#define NUM_GUN_TYPES   31

static const int EGUN_LASER = 0;
static const int EGUN_PLASMA = 1;
static const int EGUN_ION = 2;
static const int EGUN_PHOTON = 3;
static const int EGUN_SONIC = 4;
static const int EGUN_PARTICLE = 5;
static const int EGUN_GAMMA = 6;
static const int EGUN_LIGHTNING = 7;
static const int EGUN_TOP = 8;

#define IS_ARROW(gun)  (GUN_TYPE(gun) == GUN_BOW || GUN_TYPE(gun) == GUN_XBOW)
#define IS_FLAMETHROWER(gun)  (GUN_TYPE(gun) == GUN_FLAMETHROWER)

extern const char *gun_types[];
extern const int gun_damage[][2];

#endif
