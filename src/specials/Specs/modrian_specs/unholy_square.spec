//
// File: unholy_square.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#define STATE_HOLY    0
#define STATE_UNHOLY  1

#define VIAL_VNUM    1594
#define FOUNT_HOLY   3035
#define FOUNT_UNHOLY 3008

#define TITLE_UNHOLY "The Unholy Square"
#define DESC_UNHOLY  "   The main junction of the city of Modrian lies under a shroud of darkness.\r\n" \
                     "The pavement under your feet is blackened and stained, and an oppressive\r\n"     \
		     "odor assails your nostrils.  The sky above seems darker and redder than\r\n"      \
		     "usual.\r\n"          \
		     "   Streets lead off to the north, east, and west.  The silvery entrance to\r\n"   \
		     "the temple stands defiantly to the south.\r\n"

void
perform_defile(struct room_data *room, int *state, char **olddesc,
	char **oldtitle)
{

	struct obj_data *fount = NULL;

	if (*state != STATE_HOLY) {
		slog("SYSERR: invalid state in perform_defile from unholy_square.");
		return;
	}

	*state = STATE_UNHOLY;

	for (fount = room->contents; fount; fount = fount->next_content)
		if (GET_OBJ_VNUM(fount) == FOUNT_HOLY) {
			extract_obj(fount);
			break;
		}

	if (!(fount = read_object(FOUNT_UNHOLY)))
		slog("SYSERR: unable to load unholy fount in unholy_square.");
	else
		obj_to_room(fount, room);

	*olddesc = room->description;
	*oldtitle = room->name;
	room->name = TITLE_UNHOLY;
	room->description = DESC_UNHOLY;

	SET_BIT(room->zone->flags, ZONE_LOCKED);

	REMOVE_BIT(room->room_flags, ROOM_PEACEFUL);
}

void
perform_resanct(struct room_data *room, int *state, char *olddesc,
	char *oldtitle)
{

	if (*state != STATE_UNHOLY) {
		slog("SYSERR: invalid state in perform_resanct from unholy_square.");
		return;
	}

	*state = STATE_HOLY;

	room->name = oldtitle;
	room->description = olddesc;

	REMOVE_BIT(room->zone->flags, ZONE_LOCKED);
	SET_BIT(room->room_flags, ROOM_PEACEFUL);

}

SPECIAL(unholy_square)
{

	static int state = STATE_HOLY;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	static int winner = 0;
	static time_t wintime = 0;
	struct obj_data *obj = NULL, *fount = NULL;
	static char *olddesc = NULL, *oldtitle = NULL;

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (!ch)
		return 0;

	two_arguments(argument, arg1, arg2);


	if (CMD_IS("pour") && state == STATE_UNHOLY && ch->in_room->number == 3013) {

		if (!*arg1 || !(obj = GET_EQ(ch, WEAR_HOLD))
			|| GET_OBJ_VNUM(obj) != VIAL_VNUM || !isname(arg1, obj->name)
			|| !*arg2)
			return 0;

		for (fount = ch->in_room->contents; fount; fount = fount->next_content)
			if (GET_OBJ_VNUM(fount) == FOUNT_UNHOLY)
				break;

		if (!fount || !isname(arg2, fount->name))
			return 0;

		act("$n pours the contents of $p into $P...", FALSE, ch, obj, fount,
			TO_ROOM);
		act("You pour the contents of $p into $P...", FALSE, ch, obj, fount,
			TO_CHAR);

		act("A strong wind suddenly begins blowing through the square from above,\r\n" "sending debris and dust flying into the adjoining streets.  Thunderclouds\r\n" "swirl overhead, and lightning flickers across the sky.\r\n\r\n" "With a blinding flash of lightning and a deafening crash of thunder,\r\n" "$P is shattered into a million pieces!", FALSE, ch, obj, fount, TO_ROOM);

		act("A strong wind suddenly begins blowing through the square from above,\r\n" "sending debris and dust flying into the adjoining streets.  Thunderclouds\r\n" "swirl overhead, and lightning flickers across the sky.\r\n\r\n" "With a blinding flash of lightning and a deafening crash of thunder,\r\n" "$P is shattered into a million pieces!", FALSE, ch, obj, fount, TO_CHAR);

		extract_obj(fount);

		if (!(fount = read_object(FOUNT_HOLY))) {
			slog("SYSERR: unable to load FOUNT_HOLY in unholy_square.\r\n");
			return 1;
		}

		obj_to_room(fount, ch->in_room);

		act("\r\nWhen the dust clears, $P is left standing in its place.",
			FALSE, ch, obj, fount, TO_ROOM);
		act("\r\nWhen the dust clears, $P is left standing in its place.",
			FALSE, ch, obj, fount, TO_CHAR);

		send_to_zone("The Holy Square has been re-sanctified.\r\n",
			ch->in_room->zone, 1);

		winner = GET_IDNUM(ch);
		wintime = time(0);

		perform_resanct(ch->in_room, &state, olddesc, oldtitle);

		extract_obj(obj);

		mudlog(LVL_AMBASSADOR, BRF, true,
			"US: %s has re-sanctified Holy Square.", GET_NAME(ch));

		return 1;
	}


	if (GET_LEVEL(ch) < LVL_GRGOD || !CMD_IS("status"))
		return 0;

	// immortal commands here

	if (!*arg1) {

		send_to_char(ch, "Holy Square is %s.\r\nstatus <begin|end|clear>.\r\n",
			state ? "UNHOLY" : "HOLY");
		if (winner)
			sprintf(buf,
				"%sWinner is [%d] %s.\r\n"
				"Won at %s.\r\n", buf, winner, playerIndex.getName(winner),
				ctime(&wintime));

		return 1;
	}

	if (!strcmp(arg1, "clear")) {
		winner = 0;
		wintime = 0;
		send_to_char(ch, "Winner cleared.\r\n");
		return 1;
	}

	if (ch->in_room->number != 3013) {
		send_to_char(ch, "Begin and end in Holy Square only.\r\n");
		return 1;
	}

	if (!strcmp(arg1, "begin")) {

		if (state != STATE_HOLY) {
			send_to_char(ch, "End first.\r\n");
			return 1;
		}

		send_to_zone
			("A large black cloud rolls across the sky from the west, enveloping\r\n"
			"the city in darkness.\r\n", ch->in_room->zone, 1);

		send_to_room
			("A whirling funnel bursts from the bottom of the black cloud with a piercing\r\n"
			"scream!  A menacing laugh rolls across the city as the funnel cloud smashes\r\n"
			"directly into the center of Holy Square!  When the air clears, a new scene\r\n"
			"is revealed...\r\n", ch->in_room);

		perform_defile(ch->in_room, &state, &olddesc, &oldtitle);

		send_to_char(ch, "Holy Square defiled.\r\n");

		mudlog(LVL_AMBASSADOR, BRF, true,
			"US: %s has defiled Holy Square.", GET_NAME(ch));

		return 1;
	}

	if (!strcmp(arg1, "end")) {

		if (state != STATE_UNHOLY) {
			send_to_char(ch, "Begin first.\r\n");
			return 1;
		}

		perform_resanct(ch->in_room, &state, olddesc, oldtitle);

		send_to_char(ch, "Holy Square resanctified.\r\n");

		mudlog(LVL_AMBASSADOR, BRF, true,
			"US: %s has ended the defilement of Holy Square.", GET_NAME(ch));

		return 1;
	}

	send_to_char(ch, "The argument '%s' is not supported.\r\n", arg1);

	return 1;
}
