//
// File: login.h
//
// Copyright 1998 John Watson, all rights reserved
//


void show_menu(struct descriptor_data *d);
void show_race_menu_past(struct descriptor_data *d);
void show_race_menu_future(struct descriptor_data *d);
void show_race_restrict_past(struct descriptor_data *d);
void show_race_restrict_future(struct descriptor_data *d);
void show_char_class_menu(struct descriptor_data *d);
void show_char_class_menu_past(struct descriptor_data *d);
void show_char_class_menu_future(struct descriptor_data *d);
void show_time_menu(struct descriptor_data *d);
void show_past_home_menu(struct descriptor_data *d);
void show_future_home_menu(struct descriptor_data *d);

int parse_past_home(struct descriptor_data *d, char *arg);
int parse_race_past(struct descriptor_data *d, char *arg);
int parse_future_home(struct descriptor_data *d, char *arg);
int parse_race_future(struct descriptor_data *d, char *arg);
