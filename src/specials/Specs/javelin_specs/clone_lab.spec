
#define CLONE_LAB_TIME 1800		// 1800 secs == .5 hours
#define CLONE_LAB_MOB_VNUM 17199

SPECIAL(clone_lab)
{

	static time_t last_time = 0;

	if (!(CMD_IS("push") && strcasecmp(argument, "button")))
		return 0;

	time_t cur_time = time(0);

	if (cur_time - last_time < CLONE_LAB_TIME)
		return 0;

	if (GET_LEVEL(ch) < LVL_IMMORT)
		last_time = cur_time;

	struct Creature *cloned_char = read_mobile(17199);

	if (!cloned_char) {
		slog("SYSERR: cloned_char spec failed to load cloned_char mobile");
		return 0;
	}

	last_time = time(0);

	GET_ALIGNMENT(cloned_char) = GET_ALIGNMENT(ch);
	cloned_char->points.max_hit = cloned_char->points.hit = ch->points.max_hit;
	cloned_char->points.max_mana = cloned_char->points.mana =
		ch->points.max_mana;
	cloned_char->points.max_move = cloned_char->points.move =
		ch->points.max_move;

	sprintf(buf, "cloned-%s cloned %s", GET_NAME(ch), GET_NAME(ch));
	cloned_char->player.name = str_dup(buf);

	sprintf(buf, "a clone of %s", GET_NAME(ch));
	cloned_char->player.short_descr = str_dup(buf);

	sprintf(buf, "%s stands before you.\n", CAP(buf));
	cloned_char->player.long_descr = str_dup(buf);

	cloned_char->player.description = NULL;

	GET_LEVEL(cloned_char) = GET_LEVEL(ch);
	GET_CLASS(cloned_char) = GET_CLASS(ch);
	GET_RACE(cloned_char) = GET_RACE(ch);
	GET_REMORT_CLASS(cloned_char) = GET_REMORT_CLASS(ch);
	GET_HITROLL(cloned_char) = GET_HITROLL(ch);
	GET_DAMROLL(cloned_char) = GET_DAMROLL(ch);
	AFF3_FLAGS(cloned_char) = AFF3_FLAGS(ch);
	AFF2_FLAGS(cloned_char) = AFF2_FLAGS(ch);
	AFF_FLAGS(cloned_char) = AFF_FLAGS(ch);
	GET_AC(cloned_char) = GET_AC(ch);
	GET_STR(cloned_char) = GET_STR(ch);
	GET_ADD(cloned_char) = GET_ADD(ch);
	GET_DEX(cloned_char) = GET_DEX(ch);
	GET_INT(cloned_char) = GET_INT(ch);
	GET_WIS(cloned_char) = GET_WIS(ch);
	GET_CON(cloned_char) = GET_CON(ch);
	GET_CHA(cloned_char) = GET_CHA(ch);

	GET_EXP(cloned_char) = GET_EXP(ch) >> 4;
	GET_SEX(cloned_char) = GET_SEX(ch);

	char_to_room(cloned_char, ch->in_room, false);
	send_to_room
		("Your vision dims as a piercing light appears and probes you.\r\nA figure suddenly appears before you!\r\n",
		ch->in_room);

	HUNTING(cloned_char) = ch;

	return 1;
}
