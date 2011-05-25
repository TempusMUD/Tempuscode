#ifndef _DEFS_H_
#define _DEFS_H_

//
// File: defs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdint.h>
#include <inttypes.h>

typedef int8_t sbyte;
typedef uint8_t ubyte;
typedef int16_t sh_int;
typedef uint16_t ush_int;
typedef int8_t byte;

typedef int64_t money_t;

typedef int32_t room_num;
typedef int32_t obj_num;

struct txt_block {
	char *text;
	int aliased;
	struct txt_block *next;
};

#endif
