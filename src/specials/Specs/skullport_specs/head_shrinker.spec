//
// File: head_shrinker.spec                                      -- Part of TempusMUD
//
// Editted version of corpse_griller.spec
// Editted by Daniel Carruth
// Copyright 1998 by John Watson, all rights reserved.
//

#define PROTOHEAD 23200
SPECIAL(head_shrinker)
{
	struct obj_data *corpse = NULL, *head = NULL;
	char *s = NULL;
	char arg[MAX_INPUT_LENGTH];


	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (CMD_IS("list")) {
		send_to_char(ch,
			"Better empty the corpse out first if you want the contents.\r\n"
			"Type 'buy talisman <corpse>' to have a talisman made from it's head.\r\n");
		return 1;
	}

	if (!cmd || !CMD_IS("buy"))
		return 0;

	argument = one_argument(argument, arg);

	if (!is_abbrev(arg, "talisman"))
		return 0;

	argument = one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Buy head shrinking of what corpse?\r\n");
		return 1;
	}

	if (!(corpse = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		send_to_char(ch, "You don't have %s '%s'.\r\n", AN(arg), arg);
		return 1;
	}

	if (!IS_FLESH_TYPE(corpse)) {
		send_to_char(ch,
			"I only shrink flesh heads. What's wrong with you?\r\n");
		return 1;
	}
	if (IS_MAT(corpse, MAT_MEAT_COOKED)) {
		send_to_char(ch, "That's a cooked corpse, crazy man.\r\n");
		return 1;
	}
	if (!strncmp(corpse->short_description, "the headless", 12)) {
		return 1;
	}
	if (GET_GOLD(ch) < 150) {
		send_to_char(ch,
			"It costs 150 gold coins to shrink a head, buddy.\r\n");
		return 1;
	}

	if (!(head = read_object(PROTOHEAD))) {
		mudlog(LVL_POWER, BRF, true,
			"Error: Head Shrinker cannot find proto head vnum %d.\r\n",
			PROTOHEAD);
		return 1;
	}

	GET_GOLD(ch) -= 150;

	if (!strncmp(corpse->short_description, "the severed head of", 19)) {
		sprintf(buf, "the shrunken head of%s", corpse->short_description + 19);
	} else if ((s = strstr(corpse->short_description, "corpse of"))) {
		s += 9;
		if (!*s) {
			return 1;
		}
		skip_spaces(&s);
		sprintf(buf, "the shrunken head of %s", s);
	} else {
		sprintf(buf, "the shrunken head of %s", corpse->short_description);
	}

	head->short_description = str_dup(buf);

	sprintf(buf, "talisman head shrunken %s", corpse->name);
	head->name = str_dup(buf);

	sprintf(buf, "%s is here.", head->short_description);
	head->description = str_dup(buf);

	obj_to_char(head, ch);
	extract_obj(corpse);

	act("You now have $p.", FALSE, ch, head, 0, TO_CHAR);
	act("$n now has $p.", FALSE, ch, head, 0, TO_ROOM);
	return 1;
}
