//
// File: improve_stats.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_STR    0
#define MODE_INT    1
#define MODE_WIS    2
#define MODE_DEX    3
#define MODE_CON    4
#define MODE_CHA    5

char *improve_modes[7] = {
	"strength", "intelligence", "wisdom",
	"dexterity", "constitution", "charisma",
	"\n"
};

#define REAL_STAT     (mode == MODE_STR ? ch->real_abils.str :   \
		       mode == MODE_INT ? ch->real_abils.intel : \
		       mode == MODE_WIS ? ch->real_abils.wis :   \
		       mode == MODE_DEX ? ch->real_abils.dex :   \
		       mode == MODE_CON ? ch->real_abils.con :   \
		       ch->real_abils.cha)
int
do_gen_improve(struct Creature *ch, int cmd, int mode, char *argument)
{

	int gold, life_cost;
	int old_stat = REAL_STAT;
	int max_stat;

	if ((!CMD_IS("improve") && !CMD_IS("train")) || IS_NPC(ch))
		return FALSE;

	if (GET_LEVEL(ch) < 10) {
		send_to_char(ch, "You are not yet ready to improve this way.\r\n");
		send_to_char(ch, "Come back when you are level 10 or above.\r\n");
		return TRUE;
	}

	gold = REAL_STAT * GET_LEVEL(ch) * 50;
	if (mode == MODE_STR && IS_MAGE(ch))
		gold <<= 1;
	life_cost = MAX(6, (REAL_STAT << 1) - (GET_WIS(ch)));

	skip_spaces(&argument);

	switch (mode) {
        case MODE_STR:
            max_stat = get_max_str(ch);
            break;
        case MODE_DEX:
            max_stat = get_max_dex(ch);
            break;
        case MODE_INT:
            max_stat = get_max_int(ch);
            break;
        case MODE_CON:
            max_stat = get_max_con(ch);
            break;
        case MODE_CHA:
            max_stat = get_max_cha(ch);
            break;
        case MODE_WIS:
            max_stat = get_max_wis(ch);
            break;
        default:
            return FALSE;
	}

	if (!*argument) {
		if (REAL_STAT >= max_stat &&	// Thier stat is maxed
			// And make sure they cant up thier stradd
			(!(mode == MODE_STR && REAL_STAT == 18
					&& ch->real_abils.str_add < 100))) {
			send_to_char(ch, "%sYour %s cannot be improved further.%s\r\n",
				CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
			return TRUE;
		}

		send_to_char(ch,
			"It will cost you %d coins and %d life points to improve your %s.\r\n",
			gold, life_cost, improve_modes[mode]);
		sprintf(buf, "$n considers the implications of improving $s %s.",
			improve_modes[mode]);
		act(buf, TRUE, ch, 0, 0, TO_ROOM);
		if (GET_GOLD(ch) < gold)
			send_to_char(ch,
				"But you do not have enough gold on you for that.\r\n");
		else if (GET_LIFE_POINTS(ch) < life_cost)
			send_to_char(ch,
				"But you do not have enough life points for that.\r\n");

		return TRUE;
	}

	if (!is_abbrev(argument, improve_modes[mode])) {
		send_to_char(ch, "The only thing you can improve here is %s.\r\n",
			improve_modes[mode]);
		return TRUE;
	}

	if (REAL_STAT >= max_stat &&	// Thier stat is maxed
		// And make sure they cant up thier stradd
		(!(mode == MODE_STR && REAL_STAT == 18
				&& ch->real_abils.str_add < 100))) {
		send_to_char(ch, "%sYour %s cannot be improved further.%s\r\n",
			CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
		return TRUE;
	}

	if (GET_GOLD(ch) < gold) {
		send_to_char(ch, "You cannot afford it.  The cost is %d coins.\r\n",
			gold);
		return 1;
	}
	if (GET_LIFE_POINTS(ch) < life_cost) {
		sprintf(buf,
			"You have not gained sufficient life points to do this.\r\n"
			"It requires %d.\r\n", life_cost);
		send_to_char(ch, "%s", buf);
		return 1;
	}

	while (ch->affected)
		affect_remove(ch, ch->affected);

	GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - gold);
	GET_LIFE_POINTS(ch) -= life_cost;

	if (mode == MODE_STR) {
		if (REAL_STAT == 18)
			REAL_STAT += (ch->real_abils.str_add / 10);
		else if (REAL_STAT > 18)
			REAL_STAT += 10;

		REAL_STAT += 1;
		ch->real_abils.str_add = 0;

		if (REAL_STAT > 18) {
			if (REAL_STAT > 28) {
				REAL_STAT = 18 + (REAL_STAT - 28);
			} else {
				ch->real_abils.str_add = (REAL_STAT - 18) * 10;
				REAL_STAT = 18;
			}
		} else
			ch->real_abils.str_add = 0;
	} else {
		REAL_STAT += 1;
	}
	if (mode != MODE_STR)
		slog("%s improved %s from %d to %d at %d.",
			GET_NAME(ch), improve_modes[mode], old_stat, REAL_STAT,
			ch->in_room->number);
	else
		slog("%s improved %s from %d to %d/%d at %d.",
			GET_NAME(ch), improve_modes[mode], old_stat, REAL_STAT,
			ch->real_abils.str_add, ch->in_room->number);

	send_to_char(ch, "You begin your training.\r\n");
	act("$n begins to train.", FALSE, ch, 0, 0, TO_ROOM);
	WAIT_STATE(ch, REAL_STAT RL_SEC);
	ch->saveToXML();

	return TRUE;
}

SPECIAL(improve_dex)
{
	return (do_gen_improve(ch, cmd, MODE_DEX, argument));
}

SPECIAL(improve_str)
{
	return (do_gen_improve(ch, cmd, MODE_STR, argument));
}

SPECIAL(improve_int)
{
	return (do_gen_improve(ch, cmd, MODE_INT, argument));
}

SPECIAL(improve_wis)
{
	return (do_gen_improve(ch, cmd, MODE_WIS, argument));
}

SPECIAL(improve_con)
{
	return (do_gen_improve(ch, cmd, MODE_CON, argument));
}

SPECIAL(improve_cha)
{
	return (do_gen_improve(ch, cmd, MODE_CHA, argument));
}

#undef MODE_STR
#undef MODE_INT
#undef MODE_WIS
#undef MODE_DEX
#undef MODE_CON
#undef MODE_CHA
#undef REAL_STAT
