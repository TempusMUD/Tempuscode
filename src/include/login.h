#ifndef _LOGIN_H_
#define _LOGIN_H_

//
// File: login.h
//
// Copyright 1998 John Watson, all rights reserved
//


void show_menu(struct descriptor_data *d);
void show_race_menu_past(struct descriptor_data *d);
void show_race_menu_future(struct descriptor_data *d);
void show_race_restrict(struct descriptor_data *d, int timeframe = 0);
void show_char_class_menu(struct descriptor_data *d, int timeframe = 0);
void show_time_menu(struct descriptor_data *d);
void show_past_home_menu(struct descriptor_data *d);
void show_future_home_menu(struct descriptor_data *d);

int parse_past_home(struct descriptor_data *d, char *arg);
int parse_pc_race(struct descriptor_data *d, char *arg, int timeframe = 0);
int parse_future_home(struct descriptor_data *d, char *arg);


#define MODE_RENT_MENU 0
#define MODE_SHOW_MENU 1
#endif
