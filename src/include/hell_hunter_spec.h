#ifndef _HELL_HUNTER_SPEC_H_
#define _HELL_HUNTER_SPEC_H_

// File: hell_hunter_spec.h -- Part of TempusMUD
//
// DataFile: lib/etc/hell_hunter_data
//
// Copyright 1998 by John Watson, John Rothe, all rights reserved.
// Hacked to use classes and XML John Rothe 2001
//
static const int SAFE_ROOM_BITS = ROOM_PEACEFUL | ROOM_NOMOB | ROOM_NOTRACK |
                                  ROOM_NOMAGIC | ROOM_NOTEL | ROOM_ARENA |
                                  ROOM_NORECALL | ROOM_GODROOM | ROOM_HOUSE | ROOM_DEATH;

static const int H_REGULATOR = 16907;
static const int H_SPINED = 16900;
static int freq = 80;

struct devil {
    char *name;
    int vnum;
};

struct target {
    int o_vnum;
    int level;
};

struct hunter {
    int m_vnum;
    int weapon;
    char prob;
};

struct hunt_group {
    int level;
    GList *hunters;
};

#endif
