#ifndef __PLAYER_TABLE_H__
#define __PLAYER_TABLE_H__

struct creature;

int player_idnum_by_name(const char *name);
int player_name_by_idnum(int idnum);
int player_account_by_idnum(int idnum);

struct creature *load_player_from_xml(int idnum);
void save_player_to_xml(struct creature *ch);

#endif
