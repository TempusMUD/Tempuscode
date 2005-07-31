#ifndef _BOARDS_H_
#define _BOARDS_H_

#define MAX_MESSAGE_LENGTH 16384

void gen_board_show(Creature *ch);
void gen_board_save(Creature *ch, const char *board, const char *subject, const char *body);

#endif
