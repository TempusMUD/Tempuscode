//
// File: communicator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MASTER_COMM_USAGE \
                    "Usage:\r\n" \
		    "         use <unit> scan\r\n" \
		    "         use <unit> monitor\r\n"
#define MAX_CHAN 100

SPECIAL(master_communicator)
{
	struct obj_data *comm = (struct obj_data *)me, *o = NULL, *tmpo = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	struct Creature *vict = NULL;
	int i, num = 0, chan[MAX_CHAN];

	if (spec_mode != SPECIAL_CMD)
		return 0;
	if (!CMD_IS("use") || GET_LEVEL(ch) < LVL_TIMEGOD)
		return 0;

	skip_spaces(&argument);
	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2) {
		send_to_char(ch, MASTER_COMM_USAGE);
		return 1;
	}

	if (!isname(arg1, comm->aliases))
		return 0;

	*buf = 0;
	if (is_abbrev(arg2, "scan")) {

		for (o = object_list; o && num < MAX_CHAN; o = o->next) {

			if (!IS_COMMUNICATOR(o) ||
				((!o->worn_by || IS_NPC(o->worn_by)) &&
					(!o->carried_by || IS_NPC(o->carried_by))))
				continue;

			for (i = 0; i < num; i++)
				if (COMM_CHANNEL(o) == chan[i])
					break;

			if (i < num)
				continue;

			chan[num] = COMM_CHANNEL(o);
			num++;

			sprintf(buf, "%sEntities monitoring channel [%d]:\r\n",
				buf, COMM_CHANNEL(o));

			for (tmpo = o, i = 0; tmpo; tmpo = tmpo->next) {

				if (!IS_COMMUNICATOR(tmpo) ||
					COMM_CHANNEL(tmpo) != chan[num - 1])
					continue;

				if (((vict = tmpo->carried_by) || (vict = tmpo->worn_by)) &&
					can_see_creature(ch, vict))

					sprintf(buf, "%s     %3d. %20s %10s %5s\r\n",
						buf, ++i, PERS(vict, ch),
						!ENGINE_STATE(tmpo) ? "[inactive]" : "",
						!COMM_UNIT_SEND_OK(vict, ch) ? "(cantsend)" : "");

			}
		}
		page_string(ch->desc, buf);
	}

	send_to_char(ch, MASTER_COMM_USAGE);
	return 1;
}
