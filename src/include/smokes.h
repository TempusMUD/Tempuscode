#ifndef _SMOKES_H_
#define _SMOKES_H_

//
// File: smokes.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define SMOKE_NOTHING       0
#define SMOKE_DIRTWEED      1
#define SMOKE_DESERTWEED    2
#define SMOKE_INDICA        3
#define SMOKE_UNHOLY_REEFER 4
#define SMOKE_TOBACCO       5
#define SMOKE_HEMLOCK       6
#define SMOKE_PEYOTE        7
#define SMOKE_MARIJUANA     8
#define SMOKE_HOMEGROWN     9
#define NUM_SMOKES          10

#define IS_TOBACCO(obj)     (GET_OBJ_TYPE(obj) == ITEM_TOBACCO)
#define IS_PIPE(obj)        (GET_OBJ_TYPE(obj) == ITEM_PIPE)
#define IS_CIGARETTE(obj)   (GET_OBJ_TYPE(obj) == ITEM_CIGARETTE)
#define SMOKE_TYPE(obj)     (IS_TOBACCO(obj) ? GET_OBJ_VAL(obj, 0) : \
                             GET_OBJ_VAL(obj, 2))
#define MAX_DRAGS(obj)      (GET_OBJ_VAL(obj, 1))
#define CUR_DRAGS(obj)      (GET_OBJ_VAL(obj, 0))
#define SMOKE_LIT(obj)      (GET_OBJ_VAL(obj, 3))
#endif
