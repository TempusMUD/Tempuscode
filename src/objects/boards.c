#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "obj_data.h"
#include "prog.h"
#include "editor.h"
#include "strutil.h"

struct board_data {
    const char *name;
    const char *deny_read;
    const char *deny_post;
    const char *deny_edit;
    const char *deny_remove;
    const char *not_author;
    struct reaction *read_perms;
    struct reaction *post_perms;
    struct reaction *edit_perms;
    struct reaction *remove_perms;
};
void gen_board_write(struct board_data *board, struct creature *ch,
    char *argument);
void gen_board_delete(struct board_data *board, struct creature *ch,
    char *argument);
void gen_board_read(struct board_data *board, struct creature *ch,
    char *argument);
void gen_board_list(struct board_data *board, struct creature *ch);

void
gen_board_save(struct creature *ch, const char *board, int idnum,
    const char *subject, const char *body)
{
    if (idnum < 0)
        sql_exec
            ("insert into board_messages (board, post_time, author, name, subject, body) values ('%s', now(), %ld, '%s', '%s', '%s')",
            tmp_sqlescape(board), GET_IDNUM(ch),
            tmp_sqlescape(tmp_capitalize(GET_NAME(ch))),
            tmp_sqlescape(subject), tmp_sqlescape(body));
    else
        sql_exec
            ("update board_messages set post_time=now(), subject='%s', body='%s' where idnum=%d",
            tmp_sqlescape(subject), tmp_sqlescape(body), idnum);
}

void
gen_board_show(struct creature *ch)
{
    PGresult *res;
    int idx, count;

    res =
        sql_query
        ("select board, COUNT(*) from board_messages group by board order by count desc");
    count = PQntuples(res);
    if (count == 0) {
        send_to_char(ch, "There are no messages on any board.\r\n");
        return;
    }

    acc_string_clear();
    acc_sprintf
        ("Board                Count\r\n--------------------------\r\n");
    for (idx = 0; idx < count; idx++)
        acc_sprintf("%-20s %5s\r\n", PQgetvalue(res, idx, 0), PQgetvalue(res,
                idx, 1));

    page_string(ch->desc, acc_get_string());
}

void
gen_board_write(struct board_data *board, struct creature *ch, char *argument)
{
    struct creature *player;

    if (IS_PC(ch))
        player = ch;
    else if (ch->desc && ch->desc->original)
        player = ch->desc->original;
    else {
        send_to_char(ch, "You're a mob.  Go awei.\r\n");
        return;
    }

    if (ALLOW != react(board->post_perms, player)) {
        send_to_char(ch, "%s\r\n", board->deny_post);
        return;
    }

    if (!*argument) {
        send_to_char(ch,
            "If you wish to post a message, write <subject of message>\r\n");
        return;
    }

    act("$n starts to write on the board.", true, ch, NULL, NULL, TO_ROOM);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
    start_editing_board(ch->desc, board->name, -1, argument, NULL);
}

void
gen_board_edit(struct board_data *board, struct creature *ch, char *argument)
{
    struct creature *player;
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

    if (ALLOW != react(board->edit_perms, player)) {
        send_to_char(ch, "%s\r\n", board->deny_edit);
        return;
    }

    idx = atoi(tmp_getword(&argument)) - 1;
    if (idx < 0) {
        send_to_char(ch, "That is not a valid message.\r\n");
        return;
    }
    res =
        sql_query
        ("select idnum, subject, body from board_messages where board='%s' order by idnum limit 1 offset %d",
        tmp_sqlescape(board->name), idx);
    if (PQntuples(res) == 0) {
        send_to_char(ch, "That message does not exist on this board.\r\n");
        return;
    }

    act("$n starts to write on the board.", true, ch, NULL, NULL, TO_ROOM);
    start_editing_board(ch->desc,
        board->name,
        atoi(PQgetvalue(res, 0, 0)),
        *argument ? argument : PQgetvalue(res, 0, 1), PQgetvalue(res, 0, 2));
}

void
gen_board_remove(struct board_data *board, struct creature *ch, char *argument)
{
    struct creature *player;
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
    res =
        sql_query
        ("select idnum, author from board_messages where board='%s' order by idnum limit 1 offset %d",
        tmp_sqlescape(board->name), idx);

    if (PQntuples(res) != 1) {
        send_to_char(ch, "That posting doesn't exist!\r\n");
        return;
    }

    if (ALLOW != react(board->read_perms, player)
        || ALLOW != react(board->post_perms, player)) {
        send_to_char(ch, "%s\r\n", board->deny_remove);
        return;
    }

    if (GET_IDNUM(player) != atol(PQgetvalue(res, 0, 1)) &&
        ALLOW != react(board->remove_perms, player)) {
        send_to_char(ch, "%s\r\n", board->not_author);
        return;
    }

    sql_exec("delete from board_messages where idnum=%s",
        PQgetvalue(res, 0, 0));

    act("$n rips a post off of the board.", true, ch, NULL, NULL, TO_ROOM);
    send_to_char(ch, "The posting was deleted.\r\n");
}

void
gen_board_read(struct board_data *board, struct creature *ch, char *argument)
{
    struct creature *player;
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

    if (ALLOW != react(board->read_perms, player)) {
        send_to_char(ch, "%s\r\n", board->deny_read);
        return;
    }

    idx = atoi(argument) - 1;
    if (idx < 0) {
        send_to_char(ch, "That is not a valid message.\r\n");
        return;
    }
    res =
        sql_query
        ("select extract(epoch from post_time), name, subject, body from board_messages where board='%s' order by idnum limit 1 offset %d",
        tmp_sqlescape(board->name), idx);
    if (PQntuples(res) == 0) {
        send_to_char(ch, "That message does not exist on this board.\r\n");
        return;
    }
    acc_string_clear();
    post_time = atol(PQgetvalue(res, 0, 0));
    strftime(time_buf, 30, "%a %b %e %Y", localtime(&post_time));
    acc_sprintf("%sMessage %s : %s %-12s :: %s%s\r\n\r\n%s\r\n",
        CCBLD(ch, C_CMP),
        argument,
        time_buf,
        tmp_sprintf("(%s)", PQgetvalue(res, 0, 1)),
        CCNRM(ch, C_CMP), PQgetvalue(res, 0, 2), PQgetvalue(res, 0, 3));

    page_string(ch->desc, acc_get_string());
}

void
gen_board_list(struct board_data *board, struct creature *ch)
{
    PGresult *res;
    char time_buf[30];
    int idx, count;
    time_t post_time;

    res =
        sql_query
        ("select extract(epoch from post_time), name, subject from board_messages where board='%s' order by idnum desc",
        tmp_sqlescape(board->name));
    count = PQntuples(res);
    if (count == 0) {
        send_to_char(ch, "This board is empty.\r\n");
        return;
    }

    acc_string_clear();
    acc_sprintf
        ("This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>\r\n%sThere %s %d message%s on the board.%s\r\n",
        CCGRN(ch, C_NRM), (count == 1) ? "is" : "are", count,
        (count == 1) ? "" : "s", CCNRM(ch, C_NRM));

    for (idx = 0; idx < count; idx++) {
        post_time = atol(PQgetvalue(res, idx, 0));
        strftime(time_buf, 30, "%b %e, %Y", localtime(&post_time));
        acc_sprintf("%s%-2d %s:%s %s %-12s :: %s\r\n",
            CCGRN(ch, C_NRM), count - idx, CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            time_buf, tmp_sprintf("(%s)", PQgetvalue(res, idx, 1)),
            PQgetvalue(res, idx, 2));
    }

    page_string(ch->desc, acc_get_string());
}

const char *
gen_board_load(struct obj_data *self, char *param, int *err_line)
{
    char *line, *param_key;
    const char *err = NULL;
    struct board_data *board;
    int lineno = 0;

    CREATE(board, struct board_data, 1);
    board->name = "world";
    board->deny_read =
        "Try as you might, you cannot bring yourself to read this board.";
    board->deny_post =
        "Try as you might, you cannot bring yourself to write on this board.";
    board->deny_edit =
        "Try as you might, you cannot bring yourself to edit this board.";
    board->deny_remove =
        "Try as you might, you cannot bring yourself to delete anything on this board.";
    board->not_author = "You can only delete your own posts on this board.";
    CREATE(board->read_perms, struct reaction, 1);
    CREATE(board->post_perms, struct reaction, 1);
    CREATE(board->edit_perms, struct reaction, 1);
    CREATE(board->remove_perms, struct reaction, 1);

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
        else if (!strcmp(param_key, "deny-edit"))
            board->deny_edit = strdup(line);
        else if (!strcmp(param_key, "deny-remove"))
            board->deny_remove = strdup(line);
        else if (!strcmp(param_key, "not-author"))
            board->not_author = strdup(line);
        else if (!strcmp(param_key, "read")) {
            if (!add_reaction(board->read_perms, line)) {
                err = "invalid read permission";
                break;
            }
        } else if (!strcmp(param_key, "post")) {
            if (!add_reaction(board->post_perms, line)) {
                err = "invalid post permission";
                break;
            }
        } else if (!strcmp(param_key, "edit")) {
            if (!add_reaction(board->edit_perms, line)) {
                err = "invalid edit permission";
                break;
            }
        } else if (!strcmp(param_key, "remove")) {
            if (!add_reaction(board->remove_perms, line)) {
                err = "invalid delete permission";
                break;
            }
        } else {
            err = "invalid directive";
            break;
        }
    }

    if (err) {
        free_reaction(board->read_perms);
        free_reaction(board->post_perms);
        free_reaction(board->edit_perms);
        free_reaction(board->remove_perms);
        free(board);
    } else
        self->func_data = board;

    if (err_line)
        *err_line = lineno;

    return err;
}

SPECIAL(gen_board)
{
    struct obj_data *self = (struct obj_data *)me;
    struct board_data *board;
    const char *err;
    char *arg;
    int err_line;

    // We only handle commands
    if (spec_mode != SPECIAL_CMD)
        return 0;

    // In fact, we only handle these commands
    if (!(CMD_IS("write") || CMD_IS("read") || CMD_IS("remove")
            || CMD_IS("edit") || CMD_IS("look") || CMD_IS("examine")))
        return 0;

    skip_spaces(&argument);

    if (!*argument)
        return 0;

    // We only handle these command if they're referring to the board
    if ((CMD_IS("read") || CMD_IS("look") || CMD_IS("examine"))
        && !is_number(argument)
        && !isname(argument, self->aliases))
        return 0;

    // We only handle remove command if it's referring to a message
    if (CMD_IS("remove") && !is_number(argument))
        return 0;

    // We only handle edit command if it's referring to a message
    arg = argument;
    if (CMD_IS("edit") && !is_number(tmp_getword(&arg)))
        return 0;

    // If they can't see the board, they can't use it at all
    if (!can_see_object(ch, self))
        return 0;

    // Just what kind of a board is this, anyway?
    board = (struct board_data *)GET_OBJ_DATA(self);
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
                    send_to_char(ch,
                        "This board is broken.  A god has been notified.\r\n");
                }
            }
            return 1;
        }
        board = (struct board_data *)GET_OBJ_DATA(self);
    }

    if (CMD_IS("write"))
        gen_board_write(board, ch, argument);
    else if (CMD_IS("remove"))
        gen_board_remove(board, ch, argument);
    else if (CMD_IS("edit"))
        gen_board_edit(board, ch, argument);
    else if (is_number(argument))
        gen_board_read(board, ch, argument);
    else
        gen_board_list(board, ch);

    return 1;
}
