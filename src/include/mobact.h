
//
// File: mobact.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __mobact_h__
#define __mobact_h__

struct Creature *choose_opponent(struct Creature *ch,
	struct Creature *ignore_vict);
int mobile_battle_activity(struct Creature *ch,
	struct Creature *previous_vict);

int mob_fight_devil(struct Creature *ch, struct Creature *precious_vict);

#endif
