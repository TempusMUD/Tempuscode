//
// File: windy_room.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define WINDY_FILE "etc/windy_rooms"

#define DEFAULT_WINDY_MIN 0
#define DEFAULT_WINDY_MAX 25

typedef struct windy_room_data {
	int vnum;
	int min;
	int max;
} windy_room_data;

windy_room_data *windy_list = NULL;
int num_windy_rooms = 0;

int
boot_windy_rooms()
{
	FILE *fl = NULL;
	windy_room_data *newdata = NULL;
	int i, j;
	int tot_good;
	struct room_data *room = NULL;

	if (!(fl = fopen(WINDY_FILE, "r"))) {
		if (errno == ENOENT) {	// file does not exist, initialize it
			if (!(fl = fopen(WINDY_FILE, "w"))) {
				errlog("error opening WINDY_FILE for init.");
				perror("");
				return 0;
			}
			i = 0;
			fwrite(&i, sizeof(int), 1, fl);
			fclose(fl);
			if (!(fl = fopen(WINDY_FILE, "r"))) {
				errlog("error reopening WINDY_FILE in boot_windy_rooms.");
				perror("");
				return 0;
			}
			slog("windy_rooms datafile successfully initialized.");
		} else {				// file exists, something else went wrong

			errlog("error opening WINDY_FILE for read.");
			perror("");
			fclose(fl);
			return 0;
		}
	}

	if (!(fread(&num_windy_rooms, sizeof(int), 1, fl))) {
		errlog("error reading size of windy list from file.");
		fclose(fl);
		return 0;
	}

	if (num_windy_rooms <= 0) {
		fclose(fl);
		return 1;
	}

	if (windy_list)
		free(windy_list);

	windy_list = NULL;

	if (!(windy_list =
			(windy_room_data *) malloc(sizeof(windy_room_data) *
				num_windy_rooms))) {
		errlog("unable to malloc %d elements for windy list.",
			num_windy_rooms);
		fclose(fl);
		return 0;
	}

	if (!(fread(windy_list, sizeof(windy_room_data), num_windy_rooms, fl))) {
		errlog("unable to read %d windy elements from file.",
			num_windy_rooms);
		fclose(fl);
		return 0;
	}

	fclose(fl);

	// sanity check the list
	for (i = 0, tot_good = num_windy_rooms; i < num_windy_rooms; i++) {
		if (!(room = real_room(windy_list[i].vnum)) ||
			windy_room != room->func) {
			windy_list[i].vnum = -1;
			tot_good--;
			continue;
		}
		windy_list[i].min = MIN(MAX(0, windy_list[i].min), 25);
		windy_list[i].max = MIN(MAX(0, windy_list[i].max), 100);
	}

	if (tot_good < num_windy_rooms) {
		newdata =
			(windy_room_data *) malloc(sizeof(windy_room_data) * tot_good);
		for (i = 0, j = 0; i < num_windy_rooms; i++) {
			if (windy_list[i].vnum >= 0) {
				newdata[j] = windy_list[i];
				j++;
				if (j >= tot_good) {
					errlog("major error in sanity checking booted windy list.");
					free(newdata);
					free(windy_list);
					windy_list = NULL;
					num_windy_rooms = 0;
					return 0;
				}
			}
		}
		free(windy_list);
		windy_list = newdata;
	}
	slog("%d windy rooms booted from file, %d culled.", num_windy_rooms,
		num_windy_rooms - tot_good);
	return 1;
}

int
save_windy_rooms(void)
{
	FILE *fl = NULL;

	if (!(fl = fopen(WINDY_FILE, "w"))) {
		errlog("error opening WINDY_FILE for write.");
		perror("");
		fclose(fl);
		return 0;
	}

	if (!(fwrite(&num_windy_rooms, sizeof(int), 1, fl))) {
		errlog("error writing size of windy list into file.");
		fclose(fl);
		return 0;
	}

	if (!(fwrite(windy_list, sizeof(windy_room_data), num_windy_rooms, fl))) {
		errlog("error writing windy list into file.");
		fclose(fl);
		return 0;
	}

	fclose(fl);

	slog("Windy room list saved successfully.");
	return 1;
}

windy_room_data *
find_windy_data(int vnum)
{
	int i;

	for (i = 0; i < num_windy_rooms; i++) {
		if (windy_list[i].vnum == vnum)
			return (windy_list + i);
	}
	return NULL;
}

windy_room_data *
add_windy_data(int vnum, int min, int max)
{
	windy_room_data *windy = NULL, *newdata = NULL;

	min = MIN(MAX(0, min), 25);
	max = MAX(MIN(MAX(0, max), 100), min);

	if ((windy = find_windy_data(vnum))) {
		windy->min = min;
		windy->max = max;
		return (windy);
	}

	num_windy_rooms++;

	if (!(newdata =
			(windy_room_data *) realloc(windy_list,
				sizeof(windy_room_data) * num_windy_rooms))) {
		errlog("error reallocating windy_rooms for room %d.", vnum);
		return NULL;
	}

	newdata[num_windy_rooms - 1].vnum = vnum;
	newdata[num_windy_rooms - 1].min = min;
	newdata[num_windy_rooms - 1].max = max;

	windy_list = newdata;

	return (windy_list + num_windy_rooms - 1);
}

void
show_windy(windy_room_data * windy, Creature *ch)
{
	send_to_char(ch, "Room [%5d]: min-max range: (%d-%d)\r\n", windy->vnum,
		windy->min, windy->max);
}

#define WINDY_USAGE "Usage: status [show|list|min|max|load|save|reset|help]\r\n"

int
immort_windy_command(Creature *ch, windy_room_data * windy, char *argument)
{

	int i, max, min;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];


	if (!CAN_EDIT_ZONE(ch, ch->in_room->zone)) {
		send_to_char(ch, "You cannot edit this zone.\r\n"
			"Goto an editable zone to list windy data.\r\n");
		return 1;
	}

	two_arguments(argument, arg1, arg2);


	if (!*arg1) {
		send_to_char(ch, WINDY_USAGE);
		return 1;
	}

	if (!strcmp(arg1, "show")) {
		send_to_char(ch, "Windy data for this room:\r\n");
		show_windy(windy, ch);
	} else if (!strcmp(arg1, "list")) {
		send_to_char(ch, "Windy room data:\r\n");
		for (i = 0; i < num_windy_rooms; i++)
			show_windy(windy_list + i, ch);
	} else if (!strcmp(arg1, "min")) {
		if (!*arg2) {
			send_to_char(ch, "Set minimum random range to what?\r\n");
		} else {
			min = atoi(arg2);
			add_windy_data(windy->vnum, min, windy->max);
			send_to_char(ch, "Minimum random range set.\r\n");
			show_windy(windy, ch);
		}
	} else if (!strcmp(arg1, "max")) {
		if (!*arg2) {
			send_to_char(ch, "Set maximum random range to what?\r\n");
		} else {
			max = atoi(arg2);
			add_windy_data(windy->vnum, windy->min, max);
			send_to_char(ch, "Maximum random range set.\r\n");
			show_windy(windy, ch);
		}
	} else if (!strcmp(arg1, "load")) {
		if (!boot_windy_rooms()) {
			slog(" === disabling spec in room %d.", ch->in_room->number);
			ch->in_room->func = NULL;	// disable in this room only
			send_to_char(ch,
				"An error occurred.  Disabling spec in this room.\r\n");
		} else
			send_to_char(ch, "Windy rooms loaded successfully.\r\n");
	} else if (!strcmp(arg1, "save")) {
		if (!save_windy_rooms())
			send_to_char(ch, "An error occurred while saving.\r\n");
		else
			send_to_char(ch, "Windy room data save successful.\r\n");
	} else if (!strcmp(arg1, "reset")) {
		send_to_char(ch, "Sorry, you cannot perform this horrible deed.\r\n");
	} else if (!strcmp(arg1, "help")) {
		send_to_char(ch,
			"The min/max values of a windy room represent the range of a random\r\n"
			"number generation.  If the random number is greater than the player's\r\n"
			"raw strength, the player will be blown in a random direction.\r\n"
			"The special is triggered every time a player enters a command in the\r\n"
			"room.  Players who are sitting/resting/sleeping have a bonus (+5) against\r\n"
			"the wind, flying players have a penalty (-10).\r\n"
			"Total weight of a player (including gear) gives a bonus point per every\r\n"
			"80 pounds\r\n"
			"If the random number, with modifiers (reverse the signs) is greater than\r\n"
			"or equal to 10, a message will be given to the player regarding the wind\r\n"
			"even if he is not actually blown out of the room.  Random numbers (with mods)\r\n"
			"less than 10 are not announced to the player.\r\n");
	} else {
		send_to_char(ch, "Unknown argument, '%s'.\r\n%s", arg1, WINDY_USAGE);
	}
	return 1;
}


SPECIAL(windy_room)
{
	struct room_data *room = (struct room_data *)me;
	int dir, real_dirs[NUM_DIRS], num;
	static int booted = 0;
	windy_room_data *windy = NULL;
	int prob;

	if (!booted) {
		if (!boot_windy_rooms()) {
			slog(" === disabling spec in room %d.", room->number);
			room->func = NULL;	// disable in this room only
			return 0;
		}
		booted = 1;
	}

	if (!(windy = find_windy_data(room->number))) {
		if (!(windy =
				add_windy_data(room->number, DEFAULT_WINDY_MIN,
					DEFAULT_WINDY_MAX))) {
			errlog("unable to generate default windy room data for room %d.", room->number);
			room->func = NULL;
			return 0;
		}
		save_windy_rooms();		// the func will report any erros, just keep on truckin'
	}
	// immort editing commands
	if (GET_LEVEL(ch) > LVL_AMBASSADOR && CMD_IS("status"))
		return (immort_windy_command(ch, windy, argument));


	// character blowing commands
	if (PRF_FLAGGED(ch, PRF_NOHASSLE))
		return 0;

	prob = number(windy->min, windy->max);

	if (ch->getPosition() < POS_FIGHTING)	// lying-low bonus
		prob -= 5;
	if (ch->getPosition() >= POS_FLYING)	// flying penalty
		prob += 10;

	prob -= (GET_WEIGHT(ch) + IS_CARRYING_W(ch) + IS_WEARING_W(ch)) / 80;	// weight bonus


	if (prob > GET_STR(ch)) {

		for (dir = 0, num = 0; dir < NUM_DIRS; dir++) {
			if (CAN_GO(ch, dir)) {
				real_dirs[num] = dir;
				num++;
			}
		}
		if (!num) {
			send_to_char(ch, "no valid exits\r\n");
			return 0;
		}

		dir = real_dirs[number(0, num - 1)];

		if (!(room = ch->in_room->dir_option[dir]->to_room)) {
			errlog("null room in windy_room.");
			send_to_room("You feel your hair stand on end.\r\n", ch->in_room);
			return 1;
		}

		send_to_char(ch,
			"An icy gust of wind sends you flying through the air!\r\n");
		sprintf(buf,
			"An icy gust of wind sends $n flying %sward through the air!",
			dirs[dir]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);

		char_from_room(ch);
		char_to_room(ch, room);

		look_at_room(ch, ch->in_room, 0);

		ch->setPosition(POS_SITTING);
		sprintf(buf,
			"$n comes flying in on an icy blast of wind from %s!",
			from_dirs[dir]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);

		return 1;
	}

	if (prob > 10) {
		send_to_char(ch,
			"A %s arctic wind buffets you, but you stand your ground.\r\n",
			prob > 22 ? "hurricane force" : prob > 19 ? "gale force" : prob >
			15 ? "powerful" : "strong");
	}
	return 0;
}
