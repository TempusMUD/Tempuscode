
//
// File: mobact.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __mobact_h__
#define __mobact_h__

struct char_data * choose_opponent( struct char_data *ch,
                                    struct char_data *ignore_vict );
int mobile_battle_activity( struct char_data *ch, 
                            struct char_data *previous_vict );

int mob_fight_devil( struct char_data * ch, 
                     struct char_data *precious_vict );

#endif
