#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_


//
// File: constants.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define NOWHERE    -1			/* nil reference for room-database        */
#define NOTHING           -1	/* nil reference for objects                */
#define NOBODY           -1		/* nil reference for mobiles                */

enum special_mode {
	SPECIAL_CMD =    0,	// special command response
	SPECIAL_TICK =   1,	// special periodic action
	SPECIAL_DEATH =  2,	// special death notification
	SPECIAL_FIGHT =  3,	// special fight starting
	SPECIAL_COMBAT = 4,	// special in-combat ability
	SPECIAL_ENTER =  5,	// special upon entrance
	SPECIAL_LEAVE =  6,	// special upon exit
	SPECIAL_HELP =   7, // describe special to ch for documentation
	SPECIAL_RESET =  8, // zone reset
};

/* other miscellaneous defines *******************************************/

#define OPT_USEC        100000	/* 10 passes per second */
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC          * PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (4 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define SEG_VIOLENCE    (7)
#define SEG_QUEUE       (7)
#define FIRE_TICK       (3 RL_SEC)
#define PULSE_FLOWS     (1 RL_SEC)

const size_t MAX_POOF_LENGTH =   	256;
const size_t MAX_NAME_LENGTH =   	20;
const size_t MAX_PWD_LENGTH =    	10;
const size_t MAX_TITLE_LENGTH =  	60;
const size_t HOST_LENGTH =       	63;

const size_t SMALL_BUFSIZE =     		4096;
const size_t LARGE_BUFSIZE =     		65536;
const size_t GARBAGE_SPACE =     		64;
const size_t MAX_STRING_LENGTH = 65536;
const size_t MAX_INPUT_LENGTH =  	2048;	// Max length per *line* of input
const size_t MAX_RAW_INPUT_LENGTH =	32767;	// Max size of *raw* input
const size_t EXDSCR_LENGTH =     	240;	// char_file_u *DO*NOT*CHANGE*
const int MAX_MESSAGES =      		150;
const int MAX_CHAR_DESC =     		1023;	// char_file_u
const int MAX_TONGUE =        		3;		// char_file_u *DO*NOT*CHANGE*
const int MAX_SKILLS =        		700;	// char_file_u *DO*NOT*CHANGE*
const int MAX_AFFECT =        		96;		// char_file_u *DO*NOT*CHANGE*
const int MAX_OBJ_AFFECT =    		6;		// obj_file_elem *DO*NOT*CHANGE*

#ifndef __constants_cc__

extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const char *reputation_msg[];
extern const char *operator_str[];
extern const char *trade_letters[];
extern const char *temper_str[];
extern const char *shop_bits[];
extern const char *search_bits[];
extern const char *searchflag_help[];
extern const char *wear_eqpos[];
extern const char *wear_implantpos[];
extern const char *attack_type[];
extern const char *reset_mode[];
extern const char *zone_flags[];
extern const char *dirs[];
extern const char *to_dirs[];
extern const char *from_dirs[];
extern const char *search_commands[];
extern const char *home_town_abbrevs[];
extern const char *home_towns[];
extern const char *level_abbrevs[];
extern const char *diety_names[];
extern const char *alignments[];
extern const char *room_bits[];
extern const char *roomflag_names[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *planes[];
extern const char *time_frames[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *player2_bits[];
extern const char *action_bits[];
extern const char *action_bits_desc[];
extern const char *action2_bits[];
extern const char *action2_bits_desc[];
extern const char *preference_bits[];
extern const char *preference2_bits[];
extern const char *affected_bits[];
extern const char *affected_bits_desc[];
extern const char *affected2_bits[];
extern const char *affected2_bits_desc[];
extern const char *affected3_bits[];
extern const char *affected3_bits_desc[];
extern const char *where[];
extern const char *item_types[];
extern const char *item_value_types[][4];
extern const char *smoke_types[];
extern const char *wear_bits[];
extern const char *wear_keywords[];
extern const char *wear_description[];
extern const char *extra_bits[];
extern const char *extra_names[];
extern const char *extra2_bits[];
extern const char *extra2_names[];
extern const char *extra3_bits[];
extern const char *extra3_names[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char drink_aff[][3];
extern const char *color_liquid[];
extern const char *fullness[];
extern const int mana_bonus[26];
extern const char *spell_wear_off_msg[];
extern const int rev_dir[];
extern const char movement_loss[];
extern const char *weekdays[7];
extern const char *month_name[16];
extern const int daylight_mod[16];
extern const char *sun_types[];
extern const char *sky_types[];
extern const char *lunar_phases[];
extern const char *moon_sky_types[];
extern const char eq_pos_order[];
extern const char *material_names[];
extern const int weapon_proficiencies[];
extern const char *soilage_bits[];

extern const char *trail_flags[];
extern const char *spell_bits[];

extern const char *ctypes[];
extern const int wear_bitvectors[];
extern const char *desc_modes[];
#endif							// __extern constants_cc__
#endif
