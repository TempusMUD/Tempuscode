#ifndef _PATHS_H_
#define _PATHS_H_

//
// File: paths.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void load_paths(void);
void path_activity(void);
void show_path(struct creature *ch, char *arg);
void show_pathobjs(struct creature *ch);
bool add_path_to_vehicle(struct obj_data *obj, int vnum);
bool add_path_to_mob(struct creature *mob, int vnum);
int add_path(char *spath, int save);
void path_remove_object(void *object);
bool path_vnum_exists(int vnum);
bool path_name_exists(const char *name);
const char *path_name_by_vnum(int vnum);
int path_vnum_by_name(const char *name);

#endif
