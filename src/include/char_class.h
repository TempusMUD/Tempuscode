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
extern const char *pc_char_class_types[];
extern const char *char_class_abbrevs[];
extern const int prac_params[4][NUM_CLASSES];
extern const float thaco_factor[NUM_CLASSES];
extern const int exp_scale[LVL_GRIMP + 2];
extern const struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
extern const char *evil_knight_titles[LVL_GRIMP + 1];
extern const char race_restr[NUM_PC_RACES][NUM_CLASSES + 1];

#endif

int invalid_char_class(struct Creature *ch, struct obj_data *obj);
void gain_skill_prof(struct Creature *ch, int skl);

#include "actions.h"
#include "mobact.h"

#define ALL		0
#define GOOD		1
#define NEUTRAL		2
#define EVIL		3
#define NUM_NEWBIE_EQ 10
#define MORT_LEARNED(ch) \
                   (prac_params[0][(int)MIN(NUM_CLASSES-1, \
					    GET_CLASS(ch))])

#define REMORT_LEARNED(ch) \
                   (!IS_REMORT(ch) ? 0 : \
		    prac_params[0][(int)MIN(NUM_CLASSES-1, \
					    GET_REMORT_CLASS(ch))])

#define LEARNED(ch) 	(MAX(MORT_LEARNED(ch), REMORT_LEARNED(ch)) + \
			 (GET_REMORT_GEN(ch) << 1))
			 /*
			    #define MAXGAIN(ch)     (prac_params[1][(int)GET_CLASS(ch)] + (GET_REMORT_GEN(ch) << 2))

			    #define MINGAIN(ch)     (prac_params[2][(int)GET_CLASS(ch)] + \
			    GET_REMORT_GEN(ch))
			  */
#define MINGAIN(ch)     (GET_INT(ch) + GET_REMORT_GEN(ch))
#define MAXGAIN(ch)     ((GET_INT(ch) << 1) + GET_REMORT_GEN(ch))

#define SPLSKL(ch) 	(GET_CLASS(ch) > NUM_CLASSES ? "spell" :          \
                         prac_types[prac_params[3][(int)GET_CLASS(ch)]])


#define PAST_CLASS(i)  (i == CLASS_MAGIC_USER || \
			i == CLASS_CLERIC || \
			i == CLASS_RANGER || \
			i == CLASS_KNIGHT || \
			i == CLASS_BARB || \
			i == CLASS_THIEF || \
			i == CLASS_MONK)

#define FUTURE_CLASS(i) (i == CLASS_MERCENARY || \
			 i == CLASS_PSIONIC || \
			 i == CLASS_PHYSIC || \
			 i == CLASS_CYBORG || \
			 i == CLASS_HOOD)


#endif
