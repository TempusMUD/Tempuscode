#ifndef _PATHS_H_
#define _PATHS_H_

//
// File: paths.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void show_path(struct Creature *ch, char *arg);
void show_pathobjs(struct Creature *ch);
int add_path_to_vehicle(struct obj_data *obj, int vnum);
void path_remove_object(thing *object);
int add_path_to_mob(struct Creature *mob, int vnum);
int add_path(char *spath, int save);
bool path_vnum_exists(int vnum);
bool path_name_exists(const char *name);
char *path_name_by_vnum(int vnum);
int path_vnum_by_name(const char *name);

#endif
