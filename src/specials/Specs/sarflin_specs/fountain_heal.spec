//
// File: fountain_heal.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(fountain_heal)
{
	byte num;
	struct obj_data *fountain = (struct obj_data *)me;

	if (!CMD_IS("drink"))
		return 0;

	if ((IS_OBJ_STAT(fountain, ITEM_BLESS | ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
		(IS_OBJ_STAT(fountain, ITEM_EVIL_BLESS | ITEM_ANTI_GOOD)
			&& IS_GOOD(ch)))
		return 0;

	skip_spaces(&argument);

	if (!*argument)
		return 0;

	if (!isname(argument, fountain->name))
		return 0;

	if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
		sprintf(buf, "You drink %s from $p. It tastes oddly refreshing!",
			drinks[GET_OBJ_VAL(fountain, 2)]);
		num = dice(3, 8);
		WAIT_STATE(ch, 1 RL_SEC);
		GET_HIT(ch) = MIN(GET_HIT(ch) + num, GET_MAX_HIT(ch));
	} else {
		sprintf(buf, "You drink %s from $p.",
			drinks[GET_OBJ_VAL(fountain, 2)]);
	}
	act(buf, TRUE, ch, fountain, 0, TO_CHAR);
	sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(fountain, 2)]);
	act(buf, TRUE, ch, fountain, 0, TO_ROOM);
	return 1;
}
