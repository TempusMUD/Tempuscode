#ifndef _DEFS_H_
#define _DEFS_H_

//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

typedef int64_t money_t;

typedef int32_t room_num;
typedef int32_t obj_num;

struct txt_block {
    char *text;
    int aliased;
    struct txt_block *next;
};

#endif
