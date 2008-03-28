#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "editor.h"
#include "boards.h"

struct voting_option {
	struct voting_option *next;
	char idx;
	char *descrip;
	int count;
};

struct voting_poll {
	int idnum;
	char *header;
	char *descrip;
	time_t creation_time;
	int count;
	bool secret;
	struct voting_poll *next;
	struct voting_option *options;
	struct memory_rec_struct *memory;
};

struct voting_poll *voting_poll_list = NULL;
struct voting_poll *voting_new_poll = NULL;
static int VOTING_CMD_VOTE;
static int VOTING_CMD_READ;
static int VOTING_CMD_LOOK;
static int VOTING_CMD_EXAMINE;
static int VOTING_CMD_WRITE;
static int VOTING_CMD_REMOVE;
static int VOTING_CMD_HIDE;
static int VOTING_CMD_SHOW;
int voting_loaded = 0;

struct voting_poll *
voting_poll_by_id(int id)
{
	struct voting_poll *cur_poll;

	for (cur_poll = voting_poll_list;cur_poll;cur_poll = cur_poll->next)
		if (cur_poll->idnum == id)
			return cur_poll;
	return NULL;
}

void
voting_booth_load(void)
{
	struct voting_poll *new_poll, *prev_poll = NULL;
	struct voting_option *new_opt, *prev_opt = NULL;
	struct memory_rec_struct *new_mem;
	PGresult *res;
	int idx, count;

	res = sql_query("select idnum, extract(epoch from creation_time), header, descrip, secret from voting_polls");
	count = PQntuples(res);
	if (!count) {
		slog("No polls in voting booth");
		return;
	}

	for (idx = 0;idx < count;idx++) {
		CREATE(new_poll, struct voting_poll, 1);
		new_poll->idnum = atoi(PQgetvalue(res, idx, 0));
		new_poll->creation_time = atol(PQgetvalue(res, idx, 1));
		new_poll->header = strdup(PQgetvalue(res, idx, 2));
		new_poll->descrip = strdup(PQgetvalue(res, idx, 3));
		new_poll->secret = PQgetvalue(res, idx, 4)[0] == 't';

		if (prev_poll)
			prev_poll->next = new_poll;
		else
			voting_poll_list = new_poll;
		prev_poll = new_poll;
	}

	res = sql_query("select poll, descrip, count from voting_options order by poll, idx");
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++) {
		new_poll = voting_poll_by_id(atoi(PQgetvalue(res, idx, 0)));
		if (!new_poll)
			continue;
		CREATE(new_opt, struct voting_option, 1);
		new_opt->descrip = strdup(PQgetvalue(res, idx, 1));
		new_opt->count = atoi(PQgetvalue(res, idx, 2));
		new_opt->next = NULL;

		new_poll->count += new_opt->count;

		if (new_poll->options) {
			prev_opt = new_poll->options;
			while (prev_opt->next)
				prev_opt = prev_opt->next;
			new_opt->idx = prev_opt->idx + 1;
			prev_opt->next = new_opt;
		} else {
			new_poll->options = new_opt;
			new_opt->idx = 'a';
		}
	}


	res = sql_query("select poll, account from voting_accounts");
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++) {
		new_poll = voting_poll_by_id(atoi(PQgetvalue(res, idx, 0)));
		if (!new_poll)
			continue;
		CREATE(new_mem, struct memory_rec_struct, 1);
		new_mem->id = atoi(PQgetvalue(res, idx, 1));
		new_mem->next = new_poll->memory;
		new_poll->memory = new_mem;
	}
}

void
voting_booth_read(Creature * ch, char *argument)
{
	struct voting_poll *poll;
	struct voting_option *opt;
	struct memory_rec_struct *memory;
	int poll_num;

	skip_spaces(&argument);

	poll_num = atoi(argument);
	if (!*argument || !poll_num) {
		send_to_char(ch, "Which poll would you like to read?\r\n");
		return;
	}

	poll = voting_poll_list;

	while (poll && --poll_num)
		poll = poll->next;

	if (!poll) {
		send_to_char(ch, "That poll does not exist.\r\n");
		return;
	}

	memory = poll->memory;
	while (memory && memory->id != ch->desc->account->get_idnum())
		memory = memory->next;

	acc_string_clear();

	acc_strcat(poll->descrip, NULL);

	for (opt = poll->options; opt; opt = opt->next) {
		if (GET_LEVEL(ch) >= LVL_POWER
				|| (memory && !poll->secret)) {
			if (opt->count != poll->count)
				acc_sprintf("%3d (%2d%%) %c) %s",
					opt->count,
					((poll->count) ? ((opt->count * 100) / poll->count) : 0),
					opt->idx, opt->descrip);
			else
				acc_sprintf("%3d (all) %c) %s",
					opt->count, opt->idx, opt->descrip);
		} else {
			acc_sprintf("      %c) %s", opt->idx, opt->descrip);
		}
	}
	if (memory) {
		acc_strcat("\r\nYou have already voted.\r\n", NULL);
		if (poll->secret && GET_LEVEL(ch) < LVL_POWER)
			acc_sprintf("You are not able to see the results because this is a secret poll.\r\n");
	}

	page_string(ch->desc, acc_get_string());
}

void
voting_booth_vote(Creature * ch, struct obj_data *obj, char *argument)
{
	struct voting_poll *poll;
	struct voting_option *opt;
	struct memory_rec_struct *memory, *new_memory;
	char poll_str[MAX_INPUT_LENGTH];
	char answer_str[MAX_INPUT_LENGTH];
	int poll_num, answer;

	if (PLR_FLAGGED(ch, PLR_NOPOST)) {
		send_to_char(ch, "You cannot vote.\r\n");
		return;
	}

	if (GET_LEVEL(ch) < 10) {
		send_to_char(ch, "You cannot vote yet.  Try it when you've gotten your 10th level.\r\n");
		return;
	}

	two_arguments(argument, poll_str, answer_str);

	if (!*argument || !*poll_str) {
		send_to_char(ch, "Which poll would you like to vote on?\r\n");
		return;
	}

	poll_num = atoi(poll_str);
	poll = voting_poll_list;

	while (poll && --poll_num)
		poll = poll->next;

	if (!poll) {
		send_to_char(ch, "That poll does not exist.\r\n");
		return;
	}

	memory = poll->memory;
	while (memory && memory->id != ch->desc->account->get_idnum())
		memory = memory->next;

	if (memory) {
		send_to_char(ch, "You have already voted on that issue!\r\n");
		return;
	}

	if (!*answer_str) {
		send_to_char(ch, "What would you like to answer on that poll?\r\n");
		return;
	}

	answer = *answer_str;
	if (!answer) {
		send_to_char(ch,
			"Specify your answer with the letter of your choice..\r\n");
		return;
	}

	opt = poll->options;
	while (opt && opt->idx != answer)
		opt = opt->next;

	if (!opt) {
		send_to_char(ch, "Option %s for poll '%s' does not exist.\r\n",
			answer_str, poll->header);
		return;
	}

	opt->count++;
	poll->count++;

	send_to_char(ch, "You have voted for %c) %s", opt->idx, opt->descrip);
	act("$n votes on $P.", TRUE, ch, 0, obj, TO_ROOM);

	CREATE(new_memory, struct memory_rec_struct, 1);
	new_memory->next = poll->memory;
	poll->memory = new_memory;
	new_memory->id = ch->desc->account->get_idnum();

	sql_exec("update voting_options set count=%d where poll=%d and idx=%d",
		opt->count, poll->idnum, opt->idx - 'a');
	sql_exec("insert into voting_accounts (poll, account) values (%d, %d)",
		poll->idnum, ch->desc->account->get_idnum());
}

void
voting_booth_list(Creature * ch)
{
	struct voting_poll *poll;
	struct memory_rec_struct *memory;
	int poll_count = 0;
	char *secret_str, *not_voted_str;

	secret_str = tmp_sprintf(" %s(secret)%s",
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	not_voted_str = tmp_sprintf(" %s(not voted)%s",
		CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	poll = voting_poll_list;
	while (poll) {
		poll_count++;
		poll = poll->next;
	}

	send_to_char(ch,
		"This is a voting booth.  Usage: READ <poll #>, VOTE <poll #> <answer>\r\n");
	send_to_char(ch,
		"You can look at polls after voting to see current poll results.\r\n");

	poll = voting_poll_list;

	if (!poll) {
		send_to_char(ch, "%s%sThere are no polls.%s\r\n", CCRED(ch, C_NRM),
			CCBLD(ch, C_NRM), CCNRM(ch, C_NRM));
	} else {
		if (poll_count == 1)
			send_to_char(ch, "%sThere is 1 issue to vote upon.%s\r\n",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		else
			send_to_char(ch, "%sThere are %d issues to vote upon.%s\r\n",
				CCGRN(ch, C_NRM), poll_count, CCNRM(ch, C_NRM));
		poll_count = 0;
		while (poll) {
			memory = poll->memory;
			while (memory && memory->id != ch->desc->account->get_idnum())
				memory = memory->next;

			strftime(buf2, 2048, "%a %b %d", localtime(&poll->creation_time));
			send_to_char(ch, "%2d : %s (%d responses) :: %s%s%s\r\n",
				++poll_count, buf2, poll->count, poll->header,
				poll->secret ? secret_str:"",
				memory ? "":not_voted_str);
			poll = poll->next;
		}
	}
}

void
voting_booth_change_view(Creature * ch, char *argument, bool secret)
{
	struct voting_poll *cur_poll;
	int poll_num;

	poll_num = atoi(argument);
	cur_poll = voting_poll_list;

	while (cur_poll && --poll_num)
		cur_poll = cur_poll->next;

	if (!cur_poll) {
		send_to_char(ch, "That poll does not exist.\r\n");
		return;
	}

	cur_poll->secret = secret;
	sql_exec("update voting_polls set secret=%s where idnum=%d",
		secret ? "true":"false", cur_poll->idnum);
	send_to_char(ch, "Poll %s is now %s.\r\n", argument,
		secret ? "secret":"no longer secret");
}

void
voting_booth_remove(Creature * ch, char *argument)
{
	struct voting_poll *poll, *prev_poll;
	struct voting_option *opt, *next_opt;
	struct memory_rec_struct *mem, *next_mem;
	int poll_num;

	skip_spaces(&argument);

	poll_num = atoi(argument);
	if (!*argument || !poll_num) {
		send_to_char(ch, "Which poll would you like to remove?\r\n");
		return;
	}

    if (!voting_poll_list) {
		send_to_char(ch, "There are no polls to remove.\r\n");
		return;
    }

	if (1 == poll_num) {
		poll = voting_poll_list;
		voting_poll_list = voting_poll_list->next;
	} else {
		prev_poll = voting_poll_list;

		poll_num--;				// stop at the prev_poll before
		while (prev_poll && --poll_num)
			prev_poll = prev_poll->next;

		if (poll_num || !prev_poll->next) {
			send_to_char(ch, "That poll does not exist.\r\n");
			return;
		}

		poll = prev_poll->next;
		prev_poll->next = poll->next;
	}

	for (mem = poll->memory; mem; mem = next_mem) {
		next_mem = mem->next;
		free(mem);
	}

	for (opt = poll->options; opt; opt = next_opt) {
		next_opt = opt->next;
		free(opt->descrip);
		free(opt);
	}

	free(poll->descrip);

	sql_exec("delete from voting_accounts where poll=%d", poll->idnum);
	sql_exec("delete from voting_options where poll=%d", poll->idnum);
	sql_exec("delete from voting_polls where idnum=%d", poll->idnum);

	free(poll);

	send_to_char(ch, "Poll removed.\r\n");

}

void
voting_booth_write(Creature * ch, char *argument)
{
	skip_spaces(&argument);
	if (!*arg) {
		send_to_char(ch, "We must have a headline!\r\n");
		return;
	}

    start_editing_poll(ch->desc, argument);

	act("$n starts to add a poll.", TRUE, ch, 0, 0, TO_ROOM);
}

void
voting_add_poll(const char *header, const char *text)
{
	struct voting_poll *prev_poll;
	struct voting_option *prev_option, *new_option;
	char *read_pt, *line_pt;
	int reading_desc = 1;
	char opt_idx = 'a';
	char *main_buf;
	Oid poll_oid;
	PGresult *res;

	CREATE(voting_new_poll, struct voting_poll, 1);
	voting_new_poll->next = NULL;
	voting_new_poll->header = strdup(header);
	voting_new_poll->descrip = strdup(text);
	voting_new_poll->creation_time = time(NULL);
	voting_new_poll->count = 0;
	voting_new_poll->options = NULL;
	voting_new_poll->memory = NULL;
	voting_new_poll->secret = true;

	if (!*voting_new_poll->descrip) {
		free(voting_new_poll->header);
		free(voting_new_poll->descrip);
		free(voting_new_poll);
		return;
	}

	prev_option = NULL;
	main_buf = voting_new_poll->descrip;
	read_pt = voting_new_poll->descrip;
	buf[0] = '\0';
	while (*read_pt) {
		line_pt = read_pt;

		while (*read_pt && *read_pt != '\r' && *read_pt != '\n')
			read_pt++;
		if (*read_pt) {
			*read_pt++ = '\0';
			if ('*' == *line_pt) {
				line_pt++;
				skip_spaces(&line_pt);
				if (reading_desc) {
					voting_new_poll->descrip = strdup(buf);
					reading_desc = 0;
					buf[0] = '\0';
				} else {
					CREATE(new_option, struct voting_option, 1);
					new_option->idx = opt_idx++;
					new_option->descrip = strdup(buf);
					new_option->count = 0;
					new_option->next = NULL;
					if (prev_option)
						prev_option->next = new_option;
					else
						voting_new_poll->options = new_option;
					prev_option = new_option;
					buf[0] = '\0';
				}
			}

			strcat(buf, line_pt);
			strcat(buf, "\r\n");

			while (*read_pt && ('\r' == *read_pt || '\n' == *read_pt))
				read_pt++;
			line_pt = read_pt;
		}
	}

	if (buf[0]) {
		if (reading_desc) {
			voting_new_poll->descrip = strdup(buf);
		} else {
			CREATE(new_option, struct voting_option, 1);
			new_option->idx = opt_idx++;
			new_option->descrip = strdup(buf);
			new_option->count = 0;
			new_option->next = NULL;
			if (prev_option)
				prev_option->next = new_option;
			else
				voting_new_poll->options = new_option;
			prev_option = new_option;
		}
	}

	prev_poll = voting_poll_list;
	if (!prev_poll)
		voting_poll_list = voting_new_poll;
	else {
		while (prev_poll->next)
			prev_poll = prev_poll->next;
		prev_poll->next = voting_new_poll;
	}

	free(main_buf);

	// Store new poll in database
	poll_oid = sql_insert("insert into voting_polls (creation_time, header, descrip, secret) values (now(), '%s', '%s', true)",
		tmp_sqlescape(voting_new_poll->header),
		tmp_sqlescape(voting_new_poll->descrip));
	res = sql_query("select idnum from voting_polls where oid=%u", poll_oid);
	voting_new_poll->idnum = atoi(PQgetvalue(res, 0, 0));

	// Now store new options for poll
	for (new_option = voting_new_poll->options;new_option;new_option = new_option->next) {
		sql_exec("insert into voting_options (poll, descrip, idx, count) values (%d, '%s', %d, 0)",
			voting_new_poll->idnum,
			tmp_sqlescape(new_option->descrip),
			new_option->idx - 'a');

	}
}

void
voting_booth_init(void)
{
	VOTING_CMD_READ = find_command("read");
	VOTING_CMD_LOOK = find_command("look");
	VOTING_CMD_EXAMINE = find_command("examine");
	VOTING_CMD_WRITE = find_command("write");
	VOTING_CMD_REMOVE = find_command("remove");
	VOTING_CMD_VOTE = find_command("vote");
	VOTING_CMD_HIDE = find_command("hide");
	VOTING_CMD_SHOW = find_command("show");

	voting_booth_load();
}

SPECIAL(voting_booth)
{
	struct obj_data *obj = (struct obj_data *)me;

	if (!voting_loaded) {
		voting_booth_init();
		voting_loaded = 1;
	}

	if (spec_mode != SPECIAL_CMD || !ch->desc)
		return false;

	if (cmd != VOTING_CMD_VOTE && cmd != VOTING_CMD_LOOK &&
		cmd != VOTING_CMD_EXAMINE && cmd != VOTING_CMD_REMOVE &&
		cmd != VOTING_CMD_READ && cmd != VOTING_CMD_WRITE &&
		cmd != VOTING_CMD_SHOW && cmd != VOTING_CMD_HIDE)
		return 0;

	if (VOTING_CMD_VOTE == cmd)
		voting_booth_vote(ch, obj, argument);
	else if (VOTING_CMD_READ == cmd) {
		skip_spaces(&argument);
		if (!isnumber(argument))
			return 0;
		voting_booth_read(ch, argument);
	} else if (VOTING_CMD_LOOK == cmd || VOTING_CMD_EXAMINE == cmd) {
		skip_spaces(&argument);
		if (!isname(argument, obj->aliases) && !isname(argument, "voting booth"))
			return 0;
		voting_booth_list(ch);
	} else if (VOTING_CMD_REMOVE == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		skip_spaces(&argument);
		if (!isnumber(argument))
			return 0;
		voting_booth_remove(ch, argument);
	} else if (VOTING_CMD_WRITE == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR)
		voting_booth_write(ch, argument);
	else if (VOTING_CMD_SHOW == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		skip_spaces(&argument);
		if (!isnumber(argument))
			return 0;
		voting_booth_change_view(ch, argument, false);
	} else if (VOTING_CMD_HIDE == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		skip_spaces(&argument);
		if (!isnumber(argument))
			return 0;
		voting_booth_change_view(ch, argument, true);
	} else
		return 0;

	return 1;
}
