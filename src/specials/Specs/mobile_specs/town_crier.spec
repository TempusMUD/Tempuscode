struct town_crier_data {
	char *msg_head;
	char *msg_pos;
	int counter;
};

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
		// On average, the crier should shout something every
		// 5 minutes or so
		data->counter = (240 + number(0, 60)) / 4;
	}
}

SPECIAL(town_crier)
{
	Creature *self = (Creature *)me;
	town_crier_data *data;
	char *str;
	int count;
	PGresult *res;

	data = (town_crier_data *)self->mob_specials.func_data;
	if (!data) {
		CREATE(data, town_crier_data, 1);
		self->mob_specials.func_data = data;
		data->msg_head = data->msg_pos = NULL;
		data->counter = number(0, 600) / 4;
	}

	if (spec_mode == SPECIAL_CMD && IS_IMMORT(ch)) {
		if (CMD_IS("status")) {
			send_to_char(ch, "Town crier status:\r\n  Counter: %d\r\n",
				data->counter);
			if (data->msg_pos) {
				str = data->msg_pos;
				send_to_char(ch, "Next shout: %s\r\n", tmp_getline(&str));
			} else {
				send_to_char(ch, "Not currently shouting.\r\n");
			}
		} else if (CMD_IS("trigger"))
			data->counter = 0;
		else
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

	// Get the number of things we could shout
	res = sql_query("select COUNT(*) from board_messages where board='town_crier'");
	count = atol(PQgetvalue(res, 0, 0));
	PQclear(res);
	if (!count)
		return 0;	// no messages
	
	res = sql_query("select body from board_messages where board='town_crier' limit 1 offset %d", number(0, count - 1));
	if (PQntuples(res) != 1) {
		slog("SYSERR: town_crier found %d tuples from selection\n", PQntuples(res));
		PQclear(res);
		return 0;
	}
	
	data->msg_head = data->msg_pos = strdup(PQgetvalue(res, 0, 0));
	PQclear(res);

	town_crier_shout(self, data);
	return 1;
}
