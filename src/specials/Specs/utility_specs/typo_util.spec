//
// File: typo_util.spec                                         -- Part of TempusMUD
//
// Copyright 2000 by John Watson, all rights reserved.
//
void do_olc_rset(struct Creature *ch, char *argument);
int save_wld(struct Creature *ch);

/**
 *    a util for editing various descriptions.
 *
**/
const char *typo_util_cmds[] = {
	"description",				// edit a room desc
	"title",					// Room title
	"sound",					// edit a rooms sound desc
	"save",
	"\n"
};

#define TYPO_UTIL_USAGE \
"Options are:\r\n"                \
"    description\r\n"\
"    title\r\n"\
"    sound\r\n"\
"    save\r\n"

SPECIAL(typo_util)
{

	struct obj_data *obj = (struct obj_data *)me;
	char arg1[MAX_INPUT_LENGTH];
	byte tcmd;

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (!cmd || GET_LEVEL(ch) < 51 || !CMD_IS("write"))
		return 0;

	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, TYPO_UTIL_USAGE);
		return 1;
	}

	if ((tcmd = search_block(arg1, typo_util_cmds, FALSE)) < 0) {
		sprintf(buf, "$p: Invalid command '%s'.", arg1);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		send_to_char(ch, TYPO_UTIL_USAGE);
		return 1;
	}

	switch (tcmd) {
	case 0:					// Room
		sprintf(arg1, "description%s", argument);
		do_olc_rset(ch, arg1);
		break;
	case 1:					// title
		sprintf(arg1, "title%s", argument);
		do_olc_rset(ch, arg1);
		break;
	case 2:					// sound
		sprintf(arg1, "sound%s", argument);
		do_olc_rset(ch, arg1);
		break;
	case 3:					// Save
		if (save_wld(ch) == 0) {
		}
		break;
	default:
		sprintf(buf, "$p: Invalid command '%s'.", arg1);
		send_to_char(ch, TYPO_UTIL_USAGE);
		break;
	}
	return 1;
}
