struct town_crier_data {
	char *msg_head;
	char *msg_pos;
	int counter;
	int last_msg;		// idnum of the last message shouted
};

void
town_crier_reset_timer(town_crier_data *data)
{
	// On average, the crier should shout something every
	// 10 minutes or so
	data->counter = (540 + number(0, 60)) / 4;
}

void
town_crier_shout(Creature *self, town_crier_data *data)
{
	char *str;

	str = tmp_getline(&data->msg_pos);
	if (str && *str)
		do_gen_comm(self, str, 0, SCMD_SHOUT, NULL);

	if (!*data->msg_pos) {
		free(data->msg_head);
		data->msg_head = data->msg_pos = NULL;
		town_crier_reset_timer(data);
	}
}

bool
town_crier_select(town_crier_data *data)
{
	int count;
	PGresult *res;

	// Get the number of things we could shout
	res = sql_query("select COUNT(*) from board_messages where board='town_crier' and not idnum=%d", data->last_msg);
	count = atol(PQgetvalue(res, 0, 0));
	PQclear(res);
	if (!count) {
		data->last_msg = 0;
		return false;	// no messages
	}
	
	res = sql_query("select idnum, body from board_messages where board='town_crier' and not idnum=%d limit 1 offset %d", data->last_msg, number(0, count - 1));
	if (PQntuples(res) != 1) {
		slog("SYSERR: town_crier found %d tuples from selection\n", PQntuples(res));
		PQclear(res);
		return false;
	}
	
	data->last_msg = atol(PQgetvalue(res, 0, 0));
	data->msg_head = data->msg_pos = strdup(PQgetvalue(res, 0, 1));
	PQclear(res);

	return true;
}

SPECIAL(town_crier)
{
	Creature *self = (Creature *)me;
	town_crier_data *data;
	char *str;

	data = (town_crier_data *)self->mob_specials.func_data;
	if (!data) {
		CREATE(data, town_crier_data, 1);
		self->mob_specials.func_data = data;
		data->msg_head = data->msg_pos = NULL;
		data->last_msg = 0;
		data->counter = number(0, 600) / 4;
	}

	if (spec_mode == SPECIAL_CMD && IS_IMMORT(ch)) {
		if (CMD_IS("status")) {
			send_to_char(ch, "Town crier status:\r\n"
							 "  Counter: %d\r\n"
							 "  Last message: %d\r\n",
				data->counter,
				data->last_msg);
			if (data->msg_pos) {
				str = data->msg_pos;
				send_to_char(ch, "Next shout: %s\r\n", tmp_getline(&str));
			} else {
				send_to_char(ch, "Not currently shouting.\r\n");
			}
		} else if (CMD_IS("poke") && isname(argument, self->player.name)) {
			data->counter = 0;
			return 0;
		} else
			return 0;
		return 1;
	} else if (spec_mode != SPECIAL_TICK)
		return 0;
	
	// If we're already shouting something, shout the next line
	if (data->msg_head) {
		town_crier_shout(self, data);
		return 1;
	}

	// If we're not currently shouting, decrement the counter until it's
	// time to shout something again
	if (data->counter--)
		return 0;

	// Try to select a new message.  If no message was selected, keep the
	// counter at 0 and return
	if (!town_crier_select(data)) {
		town_crier_reset_timer(data);
		return 0;
	}

	// We have a new message so we immediately start shouting it
	town_crier_shout(self, data);

	return 1;
}
