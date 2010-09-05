#ifndef __PLAYERS_H__
#define __PLAYERS_H__

struct creature;

bool player_name_exists(char *name);
char *player_name_by_idnum(int idnum);
int player_account_by_name(const char *name);

bool player_idnum_exists(int idnum);
int player_idnum_by_name(const char *name);
int player_account_by_idnum(int idnum);

int player_count(void);

struct creature *load_player_from_xml(int idnum);
void save_player_to_xml(struct creature *ch);

#endif
