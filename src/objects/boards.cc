#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "clan.h"
#include "accstr.h"
#include "security.h"

struct board_data {
	const char *name;
	const char *deny_read;
	const char *deny_post;
	const char *deny_remove;
	const char *not_author;
	Reaction *read_perms;
	Reaction *post_perms;
	Reaction *remove_perms;
};
void gen_board_write(board_data *board, Creature *ch, char *argument);
void gen_board_delete(board_data *board, Creature *ch, char *argument);
void gen_board_read(board_data *board, Creature *ch, char *argument);
void gen_board_list(board_data *board, Creature *ch);

void
gen_board_save(int idnum, char *str)
{
	if (!*str) {
		// They decided not to go through with it
		sql_exec("delete from board_messages where idnum='%d'", idnum);
		return;
	}

	sql_exec("update board_messages set body='%s' where idnum='%d'",
		tmp_sqlescape(str), idnum);
}

void
gen_board_write(board_data *board, Creature *ch, char *argument)
{
	Creature *player;
	PGresult *res;
	mail_recipient_data *n_mail_to;
	Oid oid;
	char **tmp_char;
	int idnum;

	if (IS_PC(ch))
		player = ch;
	else if (ch->desc && ch->desc->original)
		player = ch->desc->original;
	else {
		send_to_char(ch, "You're a mob.  Go awei.\r\n");
		return;
	}

	if (ALLOW != board->post_perms->react(player)) {
		send_to_char(ch, "%s\r\n", board->deny_post);
		return;
	}

	if (!*argument) {
		send_to_char(ch, "If you wish to post a message, write <subject of message>\r\n");
		return;
	}

	oid = sql_insert("insert into board_messages (board, post_time, author, name, subject) values ('%s', now(), %ld, '%s', '%s')",
		tmp_sqlescape(board->name),
		GET_IDNUM(player),
		tmp_sqlescape(tmp_capitalize(GET_NAME(ch))),
		tmp_sqlescape(argument));
	
	if (oid == InvalidOid) {
		send_to_char(ch, "Something bad just happened...\r\n");
		mudlog(LVL_IMMORT, NRM, true,
			"Shouldn't happen at %s:%d", __FILE__, __LINE__);
		return;
	}
	
	res = sql_query("select idnum from board_messages where oid='%u'", oid);
	if (PQntuples(res) != 1) {
		send_to_char(ch, "Something bad just happened...\r\n");
		mudlog(LVL_IMMORT, NRM, true,
			"Shouldn't happen at %s:%d", __FILE__, __LINE__);
		return;
	}
	idnum = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

	CREATE(n_mail_to, struct mail_recipient_data, 1);
	n_mail_to->recpt_idnum = BOARD_MAGIC + idnum;
	n_mail_to->next = NULL;
	ch->desc->mail_to = n_mail_to;

	tmp_char = (char **)malloc(sizeof(char *));
	*(tmp_char) = NULL;
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	start_text_editor(ch->desc, tmp_char, true, MAX_MESSAGE_LENGTH - 1);
}

void
gen_board_remove(board_data *board, Creature *ch, char *argument)
{
	Creature *player;
	PGresult *res;
	int idx;

	if (IS_PC(ch))
		player = ch;
	else if (ch->desc && ch->desc->original)
		player = ch->desc->original;
	else {
		send_to_char(ch, "You're a mob.  Go awei.\r\n");
		return;
	}

	idx = atoi(argument) - 1;
	if (idx < 0) {
		send_to_char(ch, "That is not a valid message.\r\n");
		return;
	}
	// First we find the idnum of the thing we want to destroy
	res = sql_query("select idnum, author from board_messages where board='%s' order by post_time limit 1 offset %d",
		tmp_sqlescape(board->name), idx);
	
	if (PQntuples(res) != 1) {
		send_to_char(ch, "That posting doesn't exist!\r\n");
		return;
	}

	if (ALLOW != board->read_perms->react(player)
			|| ALLOW != board->post_perms->react(player)) {
		send_to_char(ch, "%s\r\n", board->deny_remove);
		return;
	}

	if (GET_IDNUM(player) != atol(PQgetvalue(res, 0, 1)) &&
			ALLOW != board->remove_perms->react(player)) {
		send_to_char(ch, "%s\r\n", board->not_author);
		return;
	}

	sql_exec("delete from board_messages where idnum=%s",
		PQgetvalue(res, 0, 0));
	PQclear(res);

	send_to_char(ch, "The posting was deleted.\r\n");
}

void
gen_board_read(board_data *board, Creature *ch, char *argument)
{
	Creature *player;
	PGresult *res;
	time_t post_time;
	char time_buf[30];
	int idx;

	if (IS_PC(ch))
		player = ch;
	else if (ch->desc && ch->desc->original)
		player = ch->desc->original;
	else {
		send_to_char(ch, "You're a mob.  Go awei.\r\n");
		return;
	}

	if (ALLOW != board->read_perms->react(player)) {
		send_to_char(ch, "%s\r\n", board->deny_read);
		return;
	}
	
	idx = atoi(argument) - 1;
	if (idx < 0) {
		send_to_char(ch, "That is not a valid message.\r\n");
		return;
	}
	res = sql_query("select extract(epoch from post_time), name, subject, body from board_messages where board='%s' order by post_time limit 1 offset %d",
		tmp_sqlescape(board->name), idx);
	if (PQntuples(res) == 0) {
		send_to_char(ch, "That message does not exist on this board.\r\n");
		PQclear(res);
		return;
	}
	acc_string_clear();
	post_time = atol(PQgetvalue(res, 0, 0));
	strftime(time_buf, 30, "%a %b %e", localtime(&post_time));
	acc_sprintf("%sMessage %s : %s %-12s :: %s%s\r\n\r\n%s\r\n",
		CCBLD(ch, C_CMP),
		argument,
		time_buf,
		tmp_sprintf("(%s)", PQgetvalue(res, 0, 1)),
		CCNRM(ch, C_CMP),
		PQgetvalue(res, 0, 2),
		PQgetvalue(res, 0, 3));
	PQclear(res);

	page_string(ch->desc, acc_get_string());
}

void
gen_board_list(board_data *board, Creature *ch)
{
	PGresult *res;
	char time_buf[30];
	int idx, count;
	time_t post_time;

	res = sql_query("select extract(epoch from post_time), name, subject from board_messages where board='%s' order by post_time desc", tmp_sqlescape(board->name));
	count = PQntuples(res);
	if (count == 0) {
		send_to_char(ch, "This board is empty.\r\n");
		PQclear(res);
		return;
	}

	acc_string_clear();
	acc_sprintf("This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>\r\n%sThere %s %d message%s on the board.%s\r\n",
		CCGRN(ch, C_NRM), (count == 1) ? "is":"are", count,
		(count == 1) ? "":"s", CCNRM(ch, C_NRM));

	for (idx = 0;idx < count;idx++) {
		post_time = atol(PQgetvalue(res, idx, 0));
		strftime(time_buf, 30, "%a %b %e", localtime(&post_time));
		acc_sprintf("%s%-2d %s:%s %s %-12s :: %s\r\n",
			CCGRN(ch, C_NRM), count - idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
			time_buf, tmp_sprintf("(%s)", PQgetvalue(res, idx, 1)),
			PQgetvalue(res, idx, 2));
	}
	PQclear(res);

	page_string(ch->desc, acc_get_string());
}

char *
gen_board_load(obj_data *self, char *param, int *err_line)
{
	char *line, *param_key;
	char *err = NULL;
	board_data *board;
	int lineno = 0;

	CREATE(board, board_data, 1);
	board->name = "world";
	board->deny_read = "Try as you might, you cannot bring yourself to read this board";
	board->deny_post = "Try as you might, you cannot bring yourself to write on this board";
	board->deny_remove = "Try as you might, you cannot bring yourself to delete anything on this board.";
	board->not_author = "You can only delete your own posts on this board.";
	board->read_perms = new Reaction;
	board->post_perms = new Reaction;
	board->remove_perms = new Reaction;

	while ((line = tmp_getline(&param)) != NULL) {
		lineno++;
		if (!*line)
			continue;
		param_key = tmp_getword(&line);
		if (!strcmp(param_key, "board"))
			board->name = strdup(tmp_tolower(line));
		else if (!strcmp(param_key, "deny-read"))
			board->deny_read = strdup(line);
		else if (!strcmp(param_key, "deny-post"))
			board->deny_post = strdup(line);
		else if (!strcmp(param_key, "deny-remove"))
			board->deny_remove = strdup(line);
		else if (!strcmp(param_key, "not-author"))
			board->not_author = strdup(line);
		else if (!strcmp(param_key, "read")) {
			if (!board->read_perms->add_reaction(line)) {
				err = "invalid read permission";
				break;
			}
		} else if (!strcmp(param_key, "post")) {
			if (!board->post_perms->add_reaction(line)) {
				err = "invalid post permission";
				break;
			}
		} else if (!strcmp(param_key, "remove")) {
			if (!board->remove_perms->add_reaction(line)) {
				err = "invalid delete permission";
				break;
			}
		} else {
			err = "invalid directive";
			break;
		}
	}

	if (err) {
		delete board->read_perms;
		delete board->post_perms;
		delete board->remove_perms;
		free(board);
	} else
		self->func_data = board;

	if (err_line)
		*err_line = lineno;

	return err;
}

SPECIAL(gen_board)
{
	obj_data *self = (obj_data *)me;
	board_data *board;
	char *err;
	int err_line;

	// We only handle commands
	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	// In fact, we only handle these commands
	if (!(CMD_IS("write") || CMD_IS("read") || CMD_IS("remove") || CMD_IS("look") || CMD_IS("examine")))
		return 0;
	
	skip_spaces(&argument);

	if (!*argument)
		return 0;

	// We only handle these command if they're referring to the board
	if ((CMD_IS("read") || CMD_IS("look") || CMD_IS("examine"))
			&& !isnumber(argument)
			&& !isname(argument, self->aliases))
		return 0;
	
	// If they can't see the board, they can't use it at all
	if (!can_see_object(ch, self))
		return 0;

	// Just what kind of a board is this, anyway?
	board = (board_data *)GET_OBJ_DATA(self);
	if (!board) {
		if (!GET_OBJ_PARAM(self)) {
			send_to_char(ch, "This board has not been set up yet.\r\n");
			return 1;
		}
		err = gen_board_load(self, GET_OBJ_PARAM(self), &err_line);
		if (err) {
			if (IS_PC(ch)) {
				if (IS_IMMORT(ch))
					send_to_char(ch,
						"%s has %s in line %d of its specparam\r\n",
						self->name, err, err_line);
				else {
					mudlog(LVL_IMMORT, NRM, true,
						"ERR: Object %d has %s in line %d of specparam",
						GET_OBJ_VNUM(self), err, err_line);
					send_to_char(ch, "This board is broken.  A god has been notified.\r\n");
				}
			}
			return 1;
		}
		board = (board_data *)GET_OBJ_DATA(self);
	}
	
	if (CMD_IS("write"))
		gen_board_write(board, ch, argument);
	else if (CMD_IS("remove"))
		gen_board_remove(board, ch, argument);
	else if (isnumber(argument))
		gen_board_read(board, ch, argument);
	else
		gen_board_list(board, ch);
	
	return 1;
}
