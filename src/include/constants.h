
//
// File: constants.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __constants_h__
#define __constants_h__

#ifndef __constants_cc__

extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const char *operator_str[];
extern const char *trade_letters[];
extern const char *temper_str[];
extern const char *shop_bits[];
extern const char *search_bits[];
extern const  char *wear_eqpos[];
extern const  char *wear_implantpos[];
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
extern const char *connected_types[];
extern const char *where[];
extern const char *item_types[];
extern const char *item_value_types[][4];
extern const char *smoke_types[];
extern const char *wear_bits[];
extern const  char *wear_keywords[];
extern const char *wear_description[];
extern const char *extra_bits[];
extern const char *extra_names[];
extern const char *extra2_bits[];
extern const char *extra2_names[];
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

extern const int wear_bitvectors[];
#endif // __extern constants_cc__
#endif // __extern constants_h__
