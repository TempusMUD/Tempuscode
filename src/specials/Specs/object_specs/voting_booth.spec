#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "boards.h"

struct voting_option
	{
	struct voting_option *next;
	char idx;
	char *descrip;
	int count;
	long weight;
	};

struct voting_poll
	{
	char *header;
	char *descrip;
	time_t creation_time;
	int count;
	long weight;
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
int voting_loaded = 0;

void
voting_booth_save(void) {
	FILE *ouf;
	struct voting_poll *poll;
	struct voting_option *opt;
	struct memory_rec_struct *mem;
	
	if (!voting_poll_list)
		return;

	ouf = fopen("etc/voting.booth","w");
	for (poll = voting_poll_list;poll;poll = poll->next) {
		fprintf(ouf, "# %ld %d %ld\n",
			poll->creation_time,
			poll->count,
			poll->weight);
		fprintf(ouf,"%s~\n",poll->header);
		fprintf(ouf, "%s~\n",poll->descrip);
		for (opt = poll->options;opt;opt = opt->next) {
			fprintf(ouf, "O %d %ld\n", opt->count, opt->weight);
			fprintf(ouf, "%s~\n",opt->descrip);
		}
		for (mem = poll->memory;mem;mem = mem->next) {
			fprintf(ouf, "M %ld\n", mem->id);
		}
	}
	fclose(ouf);
}

void
voting_booth_load(void) {
	struct voting_poll *prev_poll = NULL;
	struct voting_option *new_opt,*prev_opt = NULL;
	struct memory_rec_struct *new_mem,*prev_mem = NULL;
	FILE *inf;
	char opt_idx = 'a';

	inf = fopen("etc/voting.booth","r");
	if (!inf) {
		slog("/etc/voting.booth not found.");
		return;
	}
	while (fgets(buf, MAX_INPUT_LENGTH, inf)) {
		if ('#' == buf[0]) {
			CREATE(voting_new_poll, struct voting_poll, 1);
			sscanf(buf, "# %ld %d %ld\n",
				&voting_new_poll->creation_time,
				&voting_new_poll->count,
				&voting_new_poll->weight);
			voting_new_poll->header = fread_string(inf, buf2);
			voting_new_poll->descrip = fread_string(inf, buf2);
			prev_opt = NULL;
			prev_mem = NULL;
			opt_idx = 'a';
			if (prev_poll)
				prev_poll->next = voting_new_poll;
			else
				voting_poll_list = voting_new_poll;
			prev_poll = voting_new_poll;
		} else if ('O' == buf[0]) {
			CREATE(new_opt, struct voting_option, 1);
			sscanf(buf, "O %d %ld",
				&new_opt->count,
				&new_opt->weight);
			new_opt->descrip = fread_string(inf, buf2);
			new_opt->idx = opt_idx++;
			if (prev_opt)
				prev_opt->next = new_opt;
			else
				voting_new_poll->options = new_opt;
			prev_opt = new_opt;
		} else if ('M' == buf[0]) {
			CREATE(new_mem, struct memory_rec_struct, 1);
			sscanf(buf, "M %ld", &new_mem->id);
			if (prev_mem)
				prev_mem->next = new_mem;
			else
				voting_new_poll->memory = new_mem;
			prev_mem = new_mem;
		} else {
			slog("Invalid format for /etc/voting.booth");
			return;
		}
	}
}

void
voting_booth_read(char_data *ch, struct obj_data *obj, char *argument) {
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
	while (memory && memory->next && memory->id != GET_IDNUM(ch))
		memory = memory->next;

	for (opt = poll->options;opt;opt = opt->next) {
		if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
			if ( opt->count != poll->count)
				sprintf(buf, "%3d/%4ld (%2d%%/%2ld%%) %c) %s",
					opt->count,opt->weight,
					((poll->count) ? ((opt->count * 100)/poll->count):0),
					((poll->weight) ? ((opt->weight * 100)/poll->weight):0),
					opt->idx,opt->descrip);
			else
				sprintf(buf, "%3d/%4ld (all/all) %c) %s",
					opt->count,opt->weight,
					opt->idx,opt->descrip);
		} else if (memory && memory->id == GET_IDNUM(ch)) {
			if ( opt->count != poll->count)
				sprintf(buf, "(%2d%%) %c) %s",
					((poll->count) ? ((opt->count * 100)/poll->count):0),
					opt->idx,opt->descrip);
			else
				sprintf(buf, "(all) %c) %s",
					opt->idx,opt->descrip);
		} else
			send_to_char(ch, "      %c) %s",opt->idx,opt->descrip);
	}
}

void
voting_booth_vote(char_data *ch, struct obj_data *obj, char *argument) {
	struct voting_poll *poll;
	struct voting_option *opt;
	struct memory_rec_struct *memory,*new_memory;
	char poll_str[MAX_INPUT_LENGTH];
	char answer_str[MAX_INPUT_LENGTH];
	int poll_num,answer;

	if (PLR_FLAGGED(ch, PLR_NOPOST)) {
		send_to_char(ch, "You cannot vote.\r\n");
		return;
	}

	if (GET_LEVEL(ch) < 10) {
		send_to_char(ch, "You cannot vote yet.\r\n");
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
	while (memory && memory->next && memory->id != GET_IDNUM(ch))
		memory = memory->next;

	if (memory && memory->id == GET_IDNUM(ch)) {
		send_to_char(ch, "You have already voted on that issue!\r\n");
		return;
	}

	if (!*answer_str) {
		send_to_char(ch, "What would you like to answer on that poll?\r\n");
		return;
	}

	answer = *answer_str;
	if (!answer) {
		send_to_char(ch, "Specify your answer with the letter of your choice..\r\n");
		return;
	}

	opt = poll->options;
	while (opt && opt->idx != answer)
		opt = opt->next;

	if ( !opt ) {
		send_to_char(ch, "Option %s for poll '%s' does not exist.\r\n",answer_str,
			poll->header);
		return;
	}

	opt->count++;
	opt->weight += GET_LEVEL(ch) + ((GET_LEVEL(ch) < LVL_AMBASSADOR) ? (GET_REMORT_GEN(ch) * 50):0);
	poll->count++;
	poll->weight += GET_LEVEL(ch) + ((GET_LEVEL(ch) < LVL_AMBASSADOR) ? (GET_REMORT_GEN(ch) * 50):0);
	
	send_to_char(ch, "You have voted for %c) %s",opt->idx,opt->descrip);
	act("$n votes on $P.", TRUE, ch, 0, obj, TO_ROOM);

	CREATE(new_memory, struct memory_rec_struct, 1);
	if (memory)
		memory->next = new_memory;
	else	
		poll->memory = new_memory;
	new_memory->next = NULL;
	new_memory->id = GET_IDNUM(ch);

	voting_booth_save();
}

void
voting_booth_list(char_data *ch, struct obj_data *obj) {
	struct voting_poll *poll;
	int poll_count = 0;

	poll = voting_poll_list;
	while (poll) {
		poll_count++;
		poll = poll->next;
	}

	send_to_char(ch, "This is a voting booth.  Usage: READ <poll #>, VOTE <poll #> <answer>\r\n");
	send_to_char(ch, "You can look at polls after voting to see current poll results.\r\n");

	poll = voting_poll_list;

	if (!poll) {
		send_to_char(ch, "%s%sThere are no polls.%s\r\n", CCRED(ch, C_NRM),
			CCBLD(ch, C_NRM), CCNRM(ch, C_NRM));
	} else {
		send_to_char(ch, "%sThere are %d issues to vote upon.%s\r\n",
			CCGRN(ch, C_NRM), poll_count, CCNRM(ch, C_NRM));
		poll_count = 0;
		while (poll) {
			strftime(buf2, 2048, "%a %b %d", localtime(&poll->creation_time));
			send_to_char(ch, "%2d : %s (%d responses) :: %s\r\n", ++poll_count,
				buf2, poll->count, poll->header);
			poll = poll->next;
		}
	}
}

void
voting_booth_remove(char_data *ch, struct obj_data *obj, char *argument) {
	struct voting_poll *poll,*prev_poll;
	struct voting_option *opt,*next_opt;
	struct memory_rec_struct *mem,*next_mem;
	int poll_num;

	skip_spaces(&argument);

	poll_num = atoi(argument);
	if (!*argument || !poll_num) {
		send_to_char(ch, "Which poll would you like to remove?\r\n");
		return;
	}

	if (1 == poll_num) {
		poll = voting_poll_list;
		voting_poll_list = voting_poll_list->next;
	} else {
		prev_poll = voting_poll_list;

		poll_num--;	// stop at the prev_poll before
		while (prev_poll && --poll_num)
			prev_poll = prev_poll->next;

		if (!prev_poll->next) {
			send_to_char(ch, "That poll does not exist.\r\n");
			return;
		}

		poll = prev_poll->next;
		prev_poll->next = poll->next;
	}
	
	for (mem = poll->memory;mem;mem = next_mem) {
		next_mem = mem->next;
		free(mem);
	}

	for (opt = poll->options;opt;opt = next_opt) {
		next_opt = opt->next;
		free(opt->descrip);
		free(opt);
	}

	free(poll->descrip);
	free(poll);

	voting_booth_save();
}

void
voting_booth_write(char_data *ch, struct obj_data *obj, char *argument) {
	struct mail_recipient_data *n_mail_to;

	skip_spaces(&argument);
	if (!*arg) {
		send_to_char(ch, "We must have a headline!\r\n");
		return;
	}
	
	CREATE(voting_new_poll, struct voting_poll,1);
	voting_new_poll->next = NULL;
	voting_new_poll->header = str_dup(argument);
	voting_new_poll->descrip = NULL;
	voting_new_poll->creation_time = time(NULL);
	voting_new_poll->count = 0;
	voting_new_poll->weight = 0;
	voting_new_poll->options = NULL;
	voting_new_poll->memory = NULL;

	start_text_editor(ch->desc,
		&(voting_new_poll->descrip),
		true,
		MAX_MESSAGE_LENGTH);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

	act("$n starts to add a poll.", TRUE, ch, 0, 0, TO_ROOM);
	CREATE(n_mail_to, struct mail_recipient_data, 1);
	n_mail_to->next = NULL;
	n_mail_to->recpt_idnum = VOTING_MAGIC;
	ch->desc->mail_to = n_mail_to;
}

void
voting_add_poll( void ) {
	struct voting_poll *prev_poll;
	struct voting_option *prev_option,*new_option;
	char *read_pt,*line_pt;
	int reading_desc = 1;
	char opt_idx = 'a';
	char *main_buf;

	if ( !voting_new_poll->descrip ) {
		slog( "ERROR: voting_add_poll called with NULL buffer" );
		return;
	}

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
	while ( *read_pt ) {
		line_pt = read_pt;

		while ( *read_pt && *read_pt != '\r' && *read_pt != '\n' )
			read_pt++;
		if (*read_pt) {
			*read_pt++ = '\0';
			if ('*' == *line_pt) {
				line_pt++;
				skip_spaces(&line_pt);
				if (reading_desc) {
					voting_new_poll->descrip = str_dup(buf);
					reading_desc = 0;
					buf[0] = '\0';
				} else {
					CREATE(new_option, struct voting_option, 1);
					new_option->idx = opt_idx++;
					new_option->descrip = str_dup(buf);
					new_option->count = 0;
					new_option->weight = 0;
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
			voting_new_poll->descrip = str_dup(buf);
		} else {
			CREATE(new_option, struct voting_option, 1);
			new_option->idx = opt_idx++;
			new_option->descrip = str_dup(buf);
			new_option->count = 0;
			new_option->weight = 0;
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
		while ( prev_poll->next )
			prev_poll = prev_poll->next;
		prev_poll->next = voting_new_poll;
	}

	free(main_buf);

	voting_booth_save();
}

void
voting_booth_init( void ) {
	struct voting_poll *cur_poll;
	struct voting_option *cur_opt;
	struct memory_rec_struct *cur_mem;
	int vote_check;
	long weight_check;
	int poll_idx;

	VOTING_CMD_READ = find_command("read");
	VOTING_CMD_LOOK = find_command("look");
	VOTING_CMD_EXAMINE = find_command("examine");
	VOTING_CMD_WRITE = find_command("write");
	VOTING_CMD_REMOVE = find_command("remove");
	VOTING_CMD_VOTE = find_command("vote");

	voting_booth_load();

	// integrity check
	poll_idx = 0;
	for (cur_poll = voting_poll_list;cur_poll;cur_poll = cur_poll->next) {
		poll_idx++;
		vote_check = 0;
		for (cur_mem = cur_poll->memory;cur_mem;cur_mem = cur_mem->next)
			vote_check++;
		if (vote_check != cur_poll->count) {
			sprintf(buf, "ERROR: memory mismatch for voting booth %i", poll_idx);
			slog(buf);
		}
		vote_check = 0;
		weight_check = 0;
		for (cur_opt = cur_poll->options;cur_opt;cur_opt = cur_opt->next) {
			vote_check += cur_opt->count;
			weight_check += cur_opt->weight;
		}
		if (vote_check != cur_poll->count) {
			sprintf(buf, "ERROR: count mismatch for voting booth %i", poll_idx);
			slog(buf);
		}
		if (weight_check != cur_poll->weight) {
			sprintf(buf, "ERROR: weight mismatch for voting booth %i", poll_idx);
			slog(buf);
		}
	}
}

SPECIAL(voting_booth) {
	struct obj_data *obj = (struct obj_data *) me;

	if (!voting_loaded) {
		voting_booth_init();
		voting_loaded = 1;
	}

	if (!ch->desc)
		return 0;
	
	if (cmd != VOTING_CMD_VOTE && cmd != VOTING_CMD_LOOK &&
		cmd != VOTING_CMD_EXAMINE && cmd != VOTING_CMD_REMOVE &&
		cmd != VOTING_CMD_READ && cmd != VOTING_CMD_WRITE)
		return 0;
		
		if (VOTING_CMD_VOTE == cmd)
			voting_booth_vote(ch, obj, argument);
		else if (VOTING_CMD_READ == cmd) {
			skip_spaces(&argument);
			if (!isnumber(argument))
				return 0;
			voting_booth_read(ch, obj, argument);
		} else if (VOTING_CMD_LOOK == cmd || VOTING_CMD_EXAMINE == cmd) {
			skip_spaces(&argument);
			if ( !isname(argument, obj->name) && !isname(argument,"voting booth") )
				return 0;
			voting_booth_list(ch, obj);
		} else if (VOTING_CMD_REMOVE == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR) {
			skip_spaces(&argument);
			if (!isnumber(argument))
				return 0;
			voting_booth_remove(ch, obj, argument);
		} else if (VOTING_CMD_WRITE == cmd && GET_LEVEL(ch) >= LVL_AMBASSADOR)
			voting_booth_write(ch, obj, argument);
		else
			return 0;

	return 1;
}
