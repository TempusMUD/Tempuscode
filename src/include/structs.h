/* ***********************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: structs.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __structs_h__
#define __structs_h__

/*#define DMALLOC 1 */

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* preamble *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
typedef char                    byte;

typedef int room_num;
typedef int obj_num;

#include "defs.h"
#include "constants.h"
#include "obj_data.h"
#include "char_data.h"
#include "desc_data.h"
#include "room_data.h"
#include "search.h"
#include "weather.h"
#include "zone_data.h"
#include "ban.h"

#endif __structs_h__






