//
// File: darom.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#define QUEST_PATH "etc/"
#include <string.h>

struct quest_type {
	short min;
	short max;
	short number;
	char *key_wrd;
	char *quest_desc;
	char *first_clue;
};

struct quest_type quest_rec;
short num_quest = 0;

int
read_quest(FILE * f_point)
{
	char ts[100];

	fgets(ts, 100, f_point);
	if (ts[0] == '#') {
		strtok(ts, " ");		/* get the # from the start of the line */
		quest_rec.number = atoi(strtok(NULL, " "));
		quest_rec.min = atoi(strtok(NULL, " "));
		quest_rec.max = atoi(strtok(NULL, " "));
		quest_rec.key_wrd = fread_string(f_point, buf);
		quest_rec.quest_desc = fread_string(f_point, buf);
		quest_rec.first_clue = fread_string(f_point, buf);
	}
	return (0);
}

/*30*/
void
clear_quest(void)
{
	quest_rec.min = -1;
	quest_rec.max = -1;
	quest_rec.number = -1;
	if (quest_rec.key_wrd != NULL)
		free(quest_rec.key_wrd);
	if (quest_rec.quest_desc != NULL)
		free(quest_rec.quest_desc);
	if (quest_rec.first_clue != NULL)
		free(quest_rec.first_clue);
}

/*40*/
SPECIAL(darom)
{
	FILE *file_handle = NULL;
	char str_cat[80];
	short i;
	quest_rec.key_wrd = NULL;
	quest_rec.quest_desc = NULL;
	quest_rec.first_clue = NULL;
	if (spec_mode != SPECIAL_CMD)
		return 0;
	/*50 */
	if (!CMD_IS("ask"))
		return (0);
	skip_spaces(&argument);
	if (!*argument)
		return 0;

	half_chop(argument, buf, buf2);

	if (strncasecmp(buf, "darom", 5))
		return 0;

	if (!strncasecmp(buf2, "assignment", 10)) {
		sprintf(str_cat, "%squest.txt", QUEST_PATH);
		file_handle = fopen(str_cat, "rt+");
		if (file_handle == NULL) {
			slog("can not open quest file");
			return 0;
		}
		fseek(file_handle, 0, 0);
		num_quest = atoi(fgets(buf, 100, file_handle));
		for (i = 0; i < num_quest; i++) {
			read_quest(file_handle);
			if ((quest_rec.max >= GET_LEVEL(ch))
				&& (quest_rec.min <= GET_LEVEL(ch))) {
				act("Darom tells $n something.", TRUE, ch, 0, 0, TO_ROOM);
				act(quest_rec.quest_desc, TRUE, ch, 0, 0, TO_CHAR);

			}
			clear_quest();
		};
		fclose(file_handle);
		return 1;
	} else {
		if (strlen(buf2) != 0) {
			short flag = 0;
			sprintf(str_cat, "%squest.txt", QUEST_PATH);
			file_handle = fopen(str_cat, "rt+");
			if (file_handle == NULL) {
				slog("can not open quest file");
				return 0;
			}
			fseek(file_handle, 0, 0);
			num_quest = atoi(fgets(buf, 100, file_handle));
			for (i = 0; i < num_quest; i++) {
				read_quest(file_handle);
				if (!strncasecmp(quest_rec.key_wrd, buf2, strlen(buf2))) {
					act("Darom tells $n something.", TRUE, ch, 0, 0, TO_ROOM);
					act(quest_rec.first_clue, TRUE, ch, 0, 0, TO_CHAR);
					flag = 1;
				}
				clear_quest();
				if (flag)
					break;
			};
			fclose(file_handle);
			return (flag);
		}
	}
	return 0;
}
