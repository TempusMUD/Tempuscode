//
// File: act.physic.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __physic_h__
#define __physic_h__

void add_rad_sickness(CHAR *ch, int level);
int boot_timewarp_data(void);
void show_timewarps(CHAR *ch);

typedef struct timewarp_data {
  int from;
  int to;
} timewarp_data;

#define TIMEWARP_FILE "etc/timewarps"

#include "spells.h"

ASPELL(spell_nuclear_wasteland);
ASPELL(spell_spacetime_imprint);
ASPELL(spell_spacetime_recall);
ASPELL(spell_time_warp);
ASPELL(spell_area_stasis);

//extern timewarp_data *timewarp_list;
//extern int num_timewarp_data;

#endif // #ifndef __physic_h__
