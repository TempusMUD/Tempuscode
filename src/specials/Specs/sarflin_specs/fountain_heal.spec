//
// File: fountain_heal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_drink);

SPECIAL(fountain_heal)
{
	byte num;
	struct obj_data *fountain = (struct obj_data *)me;

	if (spec_mode != SPECIAL_CMD || !CMD_IS("drink"))
		return 0;

	if ((IS_OBJ_STAT(fountain, ITEM_BLESS | ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(fountain, ITEM_DAMNED | ITEM_ANTI_GOOD)
			&& IS_GOOD(ch)))
		return 0;

	skip_spaces(&argument);

	if (!*argument)
		return 0;

	if (!isname(argument, fountain->aliases))
		return 0;

	do_drink(ch, argument, 0, SCMD_DRINK, 0);
	if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
		send_to_char(ch, "It tastes oddly refreshing!\r\n");
		num = dice(3, 8);
		WAIT_STATE(ch, 1 RL_SEC);
		GET_HIT(ch) = MIN(GET_HIT(ch) + num, GET_MAX_HIT(ch));
	}

	return 1;
}
