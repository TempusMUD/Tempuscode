//
// File: fountain_restore.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_drink);

SPECIAL(fountain_restore)
{
	struct obj_data *fountain = (struct obj_data *)me;

	if (spec_mode != SPECIAL_CMD || !CMD_IS("drink"))
		return 0;

	if ((IS_OBJ_STAT(fountain, ITEM_BLESS | ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(fountain, ITEM_EVIL_BLESS | ITEM_ANTI_GOOD)
			&& IS_GOOD(ch)))
		return 0;

	skip_spaces(&argument);

	if (!*argument)
		return 0;

	if (!isname(argument, fountain->aliases))
		return 0;
	
	if( GET_OBJ_VAL(fountain,1) == 0 ) {
		send_to_char(ch, "It's empty.\r\n");
		return 1;
	}

	do_drink(ch, argument, 0, SCMD_DRINK, 0);
	if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
		send_to_char(ch, "It tastes amazingly refreshing!\r\n");
		GET_HIT(ch) = GET_MAX_HIT(ch);
		slog("%s using fountain restore.", GET_NAME(ch) );
		WAIT_STATE(ch, 3 RL_SEC);
	}

	return 1;
}
