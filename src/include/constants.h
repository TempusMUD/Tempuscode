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
	SPECIAL_RESET =  7, // zone reset
};

/* other miscellaneous defines *******************************************/

#define OPT_USEC        100000	/* 10 passes per second */
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC          * PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (4 RL_SEC)
#define PULSE_MOBILE_SPEC (2 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define SEG_VIOLENCE    (7)
#define FIRE_TICK       (3 RL_SEC)
#define PULSE_FLOWS     (1 RL_SEC)

enum {
    MAX_POOF_LENGTH =   	256,
    MAX_NAME_LENGTH =   	20,
    MAX_PWD_LENGTH =    	10,
    MAX_TITLE_LENGTH =  	60,
    MAX_BADGE_LENGTH =		 7,
    MAX_AFK_LENGTH =       20,
    HOST_LENGTH =       	63,

    SMALL_BUFSIZE =     	4096,
    LARGE_BUFSIZE =     	65536,
    GARBAGE_SPACE =     	64,
    MAX_STRING_LENGTH =    65536,
    MAX_INPUT_LENGTH =  	2048,
    MAX_RAW_INPUT_LENGTH =	32767,
    EXDSCR_LENGTH =     	240,
    MAX_MESSAGES =      		200,
    MAX_CHAR_DESC =     		1023,
    MAX_TONGUES =        		50,
    MAX_SKILLS =        		700,
    MAX_AFFECT =        		96,
    MAX_OBJ_AFFECT =    		16,

    CRIMINAL_REP =            300,	// minimum rep to be a criminal
};

struct dex_skill_type {
	int16_t p_pocket;
	int16_t p_locks;
	int16_t traps;
	int16_t sneak;
	int16_t hide;
};

struct dex_app_type {
	int16_t reaction;
	int16_t miss_att;
	int16_t defensive;
    int16_t tohit;
    int16_t todam;
};

struct str_app_type {
	int16_t tohit;				/* To Hit (THAC0) Bonus/Penalty        */
	int16_t todam;				/* Damage Bonus/Penalty                */
	int16_t carry_w;				/* Maximum weight that can be carrried */
	int16_t wield_w;				/* Maximum weight that can be wielded  */
};

struct con_app_type {
	int16_t hitp;
	int16_t shock;
};

struct weap_spec_info {
	double multiplier;
	int max;
};

extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];

extern const char circlemud_version[];
extern const char *reputation_msg[];
extern const char *operator_str[];
extern const char *trade_letters[];
extern const char *temper_str[];
extern const char *shop_bits[];
extern const char *search_bits[];
extern const char *searchflag_help[];
extern const char *wear_eqpos[];
extern const char *wear_implantpos[];
extern const char *wear_tattoopos[];
extern const char *attack_type[];
extern const char *reset_mode[];
extern const char *zone_flags[];
extern const char *dirs[];
extern const char *to_dirs[];
extern const char *from_dirs[];
extern const char *search_commands[];
extern const char *search_cmd_short[];
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
extern const char *affected_bits_ident[];
extern const char *affected2_bits[];
extern const char *affected2_bits_desc[];
extern const char *affected2_bits_ident[];
extern const char *affected3_bits[];
extern const char *affected3_bits_desc[];
extern const char *affected3_bits_ident[];
extern const char *where[];
extern const char *tattoo_pos_descs[];
extern const char *item_types[];
extern const char *item_type_descs[];
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
extern const char *item_wear_off_msg[];
extern const int rev_dir[];
extern const int8_t movement_loss[];
extern const char *weekdays[7];
extern const char *month_name[16];
extern const int daylight_mod[16];
extern const char *sun_types[];
extern const char *sky_types[];
extern const char *lunar_phases[];
extern const char *moon_sky_types[];
extern const int8_t eq_pos_order[];
extern const char implant_pos_order[];
extern const char tattoo_pos_order[];
extern const char *material_names[];
extern const int weapon_proficiencies[];
extern const char *soilage_bits[];
extern const char *grievance_kind_descs[];

extern const char *trail_flags[];
extern const char *spell_bits[];
extern const char *spell_bit_keywords[];

extern const struct weap_spec_info weap_spec_char_class[];
extern const int wear_bitvectors[];
extern const char *desc_modes[];
#endif
