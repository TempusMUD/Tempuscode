#ifndef _CHAR_CLASS_H_
#define _CHAR_CLASS_H_

//
// File: char_class.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

struct room_data;

// Returns a tmpstr allocated char* containing an appropriate ANSI
// color code for the given target Creature (tch) with the given
// recipient Creature(ch)'s color settings in mind.
const char* get_char_class_color( Creature *ch, Creature *tch, int char_class );
// Returns a const char* containing an appropriate '&c' color code for the given
// target Creature (tch) suitable for use with send_to_desc.
const char* get_char_class_color( Creature *tch, int char_class );

int invalid_char_class(struct Creature *ch, struct obj_data *obj);
void gain_skill_prof(struct Creature *ch, int skl);
void calculate_height_weight( Creature *ch );
const char *get_component_name(int comp, int sub_class);

int get_max_str( Creature *ch );
int get_max_int( Creature *ch );
int get_max_wis( Creature *ch );
int get_max_dex( Creature *ch );
int get_max_con( Creature *ch );
int get_max_cha( Creature *ch );

#endif

