#ifndef _BOARDS_H_
#define _BOARDS_H_

#define MAX_MESSAGE_LENGTH 16384

void gen_board_show(struct creature *ch)
__attribute__ ((nonnull));
void gen_board_save(struct creature *ch, const char *board, int idnum, const char *subject, const char *body)
__attribute__ ((nonnull));

#endif
