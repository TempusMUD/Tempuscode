#ifndef _BOARDS_H_
#define _BOARDS_H_

#define BOARD_MAGIC	1048575		/* arbitrary number - see modify.c */
#define VOTING_MAGIC 1048574	/* equally arbitrary, but must be lower */
#define MAX_MESSAGE_LENGTH 16384

void gen_board_show(Creature *ch);
void gen_board_save(int idnum, char *body);

#endif
