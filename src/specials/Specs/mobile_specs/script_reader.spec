//
// File: script_reader.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_RANDOM      ( 1 << 0 )
#define MODE_ALONE       ( 1 << 1 )

#define TOP_MESSAGE    ( GET_OBJ_VAL( obj, 0 ) )
#define MESSAGE_MODE   ( GET_OBJ_VAL( obj, 1 ) )
#define WAIT_TIME      ( GET_OBJ_VAL( obj, 2 ) )
#define SCRIPT_COUNTER ( GET_OBJ_VAL( obj, 3 ) )
#define CUR_WAIT       ( GET_OBJ_DAM( obj ) )
#define LAST_SCRIPT_ACTION       ( GET_OBJ_MAX_DAM( obj ) )
#define SCRIPT_FLAGGED( flag )  ( IS_SET( MESSAGE_MODE, flag ) )

static bool
script_perform(Creature *ch, obj_data *obj, int num)
{
	char *num_str;
	char *desc = NULL;

	num_str = tmp_sprintf("%d", num);
	desc = find_exdesc(num_str, obj->ex_description, 1);
	if (!desc)
		return false;
	
	desc = tmp_gsub(desc, "\r\n", " ");
	if (*desc)
		desc[strlen(desc) - 1] = '\0';
	command_interpreter(ch, tmp_gsub(desc, "\r\n", " "));
	return true;
}

SPECIAL(mob_read_script)
{
	struct obj_data *obj = GET_IMPLANT(ch, WEAR_HOLD);
	int which = 0;
	int found = 0;

	if (spec_mode != SPECIAL_TICK)
		return false;

	if (!SCRIPT_FLAGGED(MODE_ALONE)) {
		CreatureList::iterator it = ch->in_room->people.begin();
		for (; it != ch->in_room->people.end(); ++it) {
			if ((*it)->desc && CAN_SEE(ch, (*it))) {
				found = 1;
				break;
			}
		}

		if (!found) {
			SCRIPT_COUNTER = 0;
			CUR_WAIT = 0;
			return false;
		}
	}

	if (CUR_WAIT > 0) {
		CUR_WAIT--;
		return false;
	}

	if (TOP_MESSAGE < 0)
		return false;

	CUR_WAIT = WAIT_TIME;

	if (SCRIPT_FLAGGED(MODE_RANDOM)) {
		which = number(0, TOP_MESSAGE);

		if (which == LAST_SCRIPT_ACTION)
			return false;
		else
			LAST_SCRIPT_ACTION = which;

		return script_perform(ch, obj, which);
	}

	// ordered mode

	if (SCRIPT_COUNTER > TOP_MESSAGE) {
		SCRIPT_COUNTER = 0;
		CUR_WAIT = WAIT_TIME << 1;

		return false;
	}

	return script_perform(ch, obj, SCRIPT_COUNTER++);
}
