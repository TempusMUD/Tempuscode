#ifndef __PLAYERS_H__
#define __PLAYERS_H__

struct creature;

bool player_name_exists(char *name);
char *player_name_by_idnum(long idnum);
long player_account_by_name(const char *name);

bool player_idnum_exists(long idnum);
long player_idnum_by_name(const char *name);
long player_account_by_idnum(long idnum);

size_t player_count(void);
long top_player_idnum(void);

void crashsave(struct creature *ch);
struct creature *load_player_from_xml(int idnum);
void save_player_to_xml(struct creature *ch);

#endif
