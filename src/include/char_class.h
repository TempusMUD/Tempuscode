#ifndef _CHAR_CLASS_H_
#define _CHAR_CLASS_H_

//
// File: char_class.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __char_class_c__

extern const char *player_race[];
extern const int race_lifespan[];
extern const char *class_names[];
extern const char *char_class_abbrevs[];
extern const int prac_params[4][NUM_CLASSES];
extern const float thaco_factor[NUM_CLASSES];
extern const int exp_scale[LVL_GRIMP + 2];
extern const char *evil_knight_titles[LVL_GRIMP + 1];
extern const char race_restr[NUM_PC_RACES][NUM_CLASSES + 1];

#endif

// Returns a tmpstr allocated char* containing an appropriate ANSI
// color code for the given target struct creature (tch) with the given
// recipient struct creature(ch)'s color settings in mind.
const char *get_char_class_color_code( struct creature *ch, struct creature *tch, int char_class );
// Returns a const char* containing an appropriate '&c' color code for the given
// target struct creature (tch) suitable for use with send_to_desc.
const char *get_char_class_color( struct creature *tch, int char_class);

int invalid_char_class(struct creature *ch, struct obj_data *obj);
void gain_skill_prof(struct creature *ch, int skl);
void calculate_height_weight( struct creature *ch );
const char *get_component_name(int comp, int sub_class);
void do_start(struct creature *ch, int mode);

int get_max_str( struct creature *ch );
int get_max_int( struct creature *ch );
int get_max_wis( struct creature *ch );
int get_max_dex( struct creature *ch );
int get_max_con( struct creature *ch );
int get_max_cha( struct creature *ch );

#include "actions.h"
#include "mobact.h"

#define ALL     0
#define GOOD        1
#define NEUTRAL     2
#define EVIL        3
#define NUM_NEWBIE_EQ 10
#define MORT_LEARNED(ch) \
                       (prac_params[0][MIN(NUM_CLASSES-1, \
                                                                        GET_CLASS(ch))])

#define REMORT_LEARNED(ch) \
                       (!IS_REMORT(ch) ? 0 : \
                                    prac_params[0][MIN(NUM_CLASSES-1, \
                                                                GET_REMORT_CLASS(ch))])

#define LEARNED(ch)     (MAX(MORT_LEARNED(ch), REMORT_LEARNED(ch)) + \
                     (GET_REMORT_GEN(ch) << 1))
#define MINGAIN(ch)     (GET_INT(ch) + GET_REMORT_GEN(ch))
#define MAXGAIN(ch)     ((GET_INT(ch) << 1) + GET_REMORT_GEN(ch))

#define SPLSKL(ch)  (GET_CLASS(ch) > NUM_CLASSES ? "spell" :          \
                                 prac_types[prac_params[3][(int)GET_CLASS(ch)]])

#define PAST_CLASS(i)  (i == CLASS_MAGIC_USER || \
                    i == CLASS_CLERIC || \
                    i == CLASS_RANGER || \
                    i == CLASS_KNIGHT || \
                    i == CLASS_BARB || \
                    i == CLASS_THIEF || \
                    i == CLASS_MONK || \
                    i == CLASS_BARD)

#define FUTURE_CLASS(i) (i == CLASS_MERCENARY || \
                     i == CLASS_PSIONIC || \
                     i == CLASS_PHYSIC || \
                     i == CLASS_CYBORG)

#endif

