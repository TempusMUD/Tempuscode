//
// File: olc_srch.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"
#include "screen.h"
#include "spells.h"
#include "materials.h"

extern struct char_data *character_list;
int clan_house_can_enter(struct char_data *ch, struct room_data *room);
const char *olc_xset_keys[] = {
	"triggers",
	"keywords",
	"to_vict",
	"to_room",
	"command",
	"value",
	"to_remote",
	"flags",
	"\n",
};
#define NUM_XSET_COMMANDS 8

#define srch_p GET_OLC_SRCH(ch)
void
do_olc_xset(struct char_data *ch, char *argument)
{

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int i, command, cur_flags, tmp_flags, flag;
	int state = 0;

	if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_SEARCH_APPROVED)) {
		send_to_char("This zone has not been approved for search editing.\r\n",
			ch);
		return;
	}
	if (!GET_OLC_SRCH(ch)) {
		send_to_char("You are not currently editing a search.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	skip_spaces(&argument);

	if (!*arg1) {
		sprintf(buf, "Valid xset commands:\r\n%s", CCYEL(ch, C_NRM));
		for (i = 0; i < NUM_XSET_COMMANDS; i++) {
			strcat(buf, olc_xset_keys[i]);
			strcat(buf, "\r\n");
		}
		strcat(buf, CCYEL(ch, C_NRM));
		page_string(ch->desc, buf, 1);
		return;
	}

	if ((command = search_block(arg1, olc_xset_keys, 0)) < 0) {
		sprintf(buf, "No such xset command '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		sprintf(buf, "Set search %s to what?\r\n", olc_xset_keys[command]);
		send_to_char(buf, ch);
		return;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	switch (command) {
	case 0:					/* triggers */
		if (srch_p->command_keys)
			free(srch_p->command_keys);
		srch_p->command_keys = strdup(argument);
		send_to_char("Search command triggers set.\r\n", ch);
		break;
	case 1:					/* keywords */
		if (srch_p->keywords)
			free(srch_p->keywords);
		if (argument[0] == '~')
			srch_p->keywords = NULL;
		else
			srch_p->keywords = strdup(argument);
		send_to_char("Search argument keywords set.\r\n", ch);
		break;
	case 2:					/* to_vict */
		delete_doubledollar(argument);
		if (srch_p->to_vict)
			free(srch_p->to_vict);
		if (argument[0] == '~')
			srch_p->to_vict = NULL;
		else
			srch_p->to_vict = strdup(argument);
		send_to_char("To_vict message set.\r\n", ch);
		break;
	case 3:					/* to_room */
		delete_doubledollar(argument);
		if (srch_p->to_room)
			free(srch_p->to_room);
		if (argument[0] == '~')
			srch_p->to_room = NULL;
		else
			srch_p->to_room = strdup(argument);
		send_to_char("To_room message set.\r\n", ch);
		break;
	case 4:					/* command */
		if ((command = search_block(arg1, search_commands, 0)) < 0) {
			sprintf(buf, "No such search command '%s'.\r\n", arg1);
			send_to_char(buf, ch);
			return;
		}
		srch_p->command = command;
		send_to_char("Search command set.\r\n", ch);
		break;
	case 5:					/* value */
		if ((i = atoi(arg1)) < 0 || i > 2) {
			send_to_char("xset val <0 | 1 | 2> <value>\r\n", ch);
			return;
		}
		if (!*arg2) {
			send_to_char("Set the value to what?\r\n", ch);
			return;
		}
		srch_p->arg[i] = atoi(arg2);
		send_to_char("Ok, value set.\r\n", ch);
		break;
	case 6:					/* to_remote */
		delete_doubledollar(argument);
		if (srch_p->to_remote)
			free(srch_p->to_remote);
		if (argument[0] == '~')
			srch_p->to_remote = NULL;
		else
			srch_p->to_remote = strdup(argument);
		send_to_char("To_remote message set.\r\n", ch);
		break;

	case 7:					/* flags */
		tmp_flags = 0;
		argument = one_argument(argument, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char("Usage: olc xset flags [+/-] [FLAG, FLAG, ...]\r\n",
				ch);
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = srch_p->flags;

		while (*arg1) {
			if ((flag = search_block(arg1, search_bits, FALSE)) == -1) {
				sprintf(buf, "Invalid flag %s, skipping...\r\n", arg1);
				send_to_char(buf, ch);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		srch_p->flags = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char("Search flags set\r\n", ch);
		} else if (tmp_flags == 0)
			send_to_char("Search flags not altered.\r\n", ch);
		else {
			send_to_char("Search flags set.\r\n", ch);
		}
		break;

	default:
		send_to_char("This option currently unavailable.\r\n", ch);
		break;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
}
struct special_search_data *
do_create_search(struct char_data *ch, char *arg)
{

	struct special_search_data *srch = NULL;
	char triggers[MAX_INPUT_LENGTH], keywords[MAX_INPUT_LENGTH];

	arg = one_argument(arg, triggers);
	one_argument(arg, keywords);

	skip_spaces(&arg);

	if (!*triggers) {
		send_to_char("USAGE: create search <trigger word> <keyword>\r\n", ch);
		return NULL;
	}

	for (srch = ch->in_room->search; srch; srch = srch->next) {
		if (isname_exact(triggers, srch->command_keys) &&
			*keywords && srch->keywords
			&& isname_exact(keywords, srch->keywords)) {
			send_to_char("There is already a search here on that trigger.\r\n",
				ch);
			return NULL;
		}
	}

	if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ROOMS_APPROVED)) {
		send_to_char("World olc is not approved for this zone.\r\n", ch);
		return NULL;
	}

	CREATE(srch, struct special_search_data, 1);

	srch->command_keys = strdup(triggers);
	if (*arg)
		srch->keywords = strdup(arg);
	else
		srch->keywords = NULL;
	srch->to_room = NULL;
	srch->to_vict = NULL;
	srch->flags = 0;
	srch->command = SEARCH_COM_NONE;
	srch->arg[0] = -1;
	srch->arg[1] = -1;
	srch->arg[2] = -1;
	srch->next = ch->in_room->search;

	ch->in_room->search = srch;

	return (srch);
}

int
do_destroy_search(struct char_data *ch, char *arg)
{
	struct special_search_data *srch = NULL, *temp = NULL;
	char triggers[MAX_INPUT_LENGTH], keywords[MAX_INPUT_LENGTH];

	arg = two_arguments(arg, triggers, keywords);

	if (!*triggers) {
		send_to_char("USAGE: destroy search <trigger word> [keyword]\r\n", ch);
		return 0;
	}

	for (srch = ch->in_room->search; srch; srch = srch->next) {
		if (isname_exact(triggers, srch->command_keys) &&
			(!keywords || (!srch->keywords
					|| isname_exact(keywords, srch->keywords)))) {
			break;
		}
	}

	if (!srch) {
		send_to_char("There is no such search here.\r\n", ch);
		return 0;
	}

	if (!OLC_EDIT_OK(ch, ch->in_room->zone, ZONE_ROOMS_APPROVED)) {
		send_to_char("World olc is not approved for this zone.\r\n", ch);
		return 0;
	}
	CharacterList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		if (GET_OLC_SRCH((*cit)) == srch)
			GET_OLC_SRCH((*cit)) = NULL;
	}
	REMOVE_FROM_LIST(srch, ch->in_room->search, next);
	if (srch->command_keys)
		free(srch->command_keys);
	if (srch->keywords)
		free(srch->keywords);
	if (srch->to_vict)
		free(srch->to_vict);
	if (srch->to_room)
		free(srch->to_room);
	free(srch);
	send_to_char("Search destroyed.\r\n", ch);
	return 1;
}

int
set_char_xedit(struct char_data *ch, char *argument)
{

	struct special_search_data *srch = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		return 0;

	if (!*arg2 && is_abbrev(arg1, "exit")) {
		send_to_char("Exiting search editor.\r\n", ch);
		GET_OLC_SRCH(ch) = srch;
		return 1;
	}

	for (srch = ch->in_room->search; srch; srch = srch->next)
		if (isname_exact(arg1, srch->command_keys) &&
			(!*arg2 || !srch->keywords
				|| isname_exact(arg2, srch->keywords))) {
			GET_OLC_SRCH(ch) = srch;
			sprintf(buf,
				"You are now editing a search that triggers on:\r\n"
				"%s (%s)\r\n", GET_OLC_SRCH(ch)->command_keys,
				GET_OLC_SRCH(ch)->keywords ?
				GET_OLC_SRCH(ch)->keywords : "NULL");
			send_to_char(buf, ch);
			return 1;
		}
	send_to_char("No such search in room.\r\n", ch);
	return 0;
}

void
print_search_data_to_buf(struct char_data *ch, struct room_data *room,
	struct special_search_data *cur_search, char *buf)
{

	struct obj_data *obj = NULL;
	struct char_data *mob = NULL;

	sprintf(buf, "%sCommand triggers:%s %s, %skeywords:%s %s\r\n",
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
		cur_search->command_keys ? cur_search->command_keys : "None.",
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
		(SRCH_FLAGGED(cur_search, SRCH_CLANPASSWD) && room &&
			!clan_house_can_enter(ch, room)) ? "*******" :
		(cur_search->keywords ? cur_search->keywords : "None."));
	sprintf(buf, "%s To_vict  : %s\r\n To_room  : %s\r\n To_remote: %s\r\n",
		buf, cur_search->to_vict ? cur_search->to_vict : "None",
		cur_search->to_room ? cur_search->to_room : "None",
		cur_search->to_remote ? cur_search->to_remote : "None");

	switch (cur_search->command) {
	case SEARCH_COM_DOOR:
		sprintf(buf, "%s DOOR  Room #: %d, Direction: %d, Mode: %d.\r\n", buf,
			cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_MOBILE:
		mob = real_mobile_proto(cur_search->arg[0]);
		sprintf(buf, "%s MOB  Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
			buf, cur_search->arg[0], CCYEL(ch, C_NRM),
			mob ? GET_NAME(mob) : "NULL", CCNRM(ch, C_NRM), cur_search->arg[1],
			cur_search->arg[2]);
		break;
	case SEARCH_COM_OBJECT:
		obj = real_object_proto(cur_search->arg[0]);
		sprintf(buf, "%s OBJECT Vnum : %d (%s%s%s), to room: %d, Max: %d.\r\n",
			buf, cur_search->arg[0],
			CCGRN(ch, C_NRM),
			obj ? obj->short_description : "NULL", CCNRM(ch, C_NRM),
			cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_REMOVE:
		obj = real_object_proto(cur_search->arg[0]);
		sprintf(buf,
			"%s REMOVE  Obj Vnum : %d (%s%s%s), Room # : %d, Val 2: %d.\r\n",
			buf, cur_search->arg[0], CCGRN(ch, C_NRM),
			obj ? obj->short_description : "NULL", CCNRM(ch, C_NRM),
			cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_EQUIP:
		obj = real_object_proto(cur_search->arg[1]);
		sprintf(buf,
			"%s EQUIP  ----- : %d, Obj Vnum : %d (%s%s%s), Pos : %d.\r\n", buf,
			cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
			obj ? obj->short_description : "NULL", CCNRM(ch, C_NRM),
			cur_search->arg[2]);
		break;
	case SEARCH_COM_GIVE:
		obj = real_object_proto(cur_search->arg[1]);
		sprintf(buf,
			"%s GIVE  ----- : %d, Obj Vnum : %d (%s%s%s), Max : %d.\r\n", buf,
			cur_search->arg[0], cur_search->arg[1], CCGRN(ch, C_NRM),
			obj ? obj->short_description : "NULL", CCNRM(ch, C_NRM),
			cur_search->arg[2]);
		break;
	case SEARCH_COM_NONE:
		sprintf(buf, "%s NONE       %5d        %5d        %5d\r\n",
			buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_TRANSPORT:
		sprintf(buf, "%s TRANS      %5d        %5d        %5d\r\n",
			buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_SPELL:
		sprintf(buf, "%s SPELL      %5d        %5d        %5d (%s)\r\n",
			buf, cur_search->arg[0],
			cur_search->arg[1], cur_search->arg[2],
			(cur_search->arg[2] > 0 && cur_search->arg[2] < TOP_NPC_SPELL) ?
			spells[cur_search->arg[2]] : "NULL");
		break;
	case SEARCH_COM_DAMAGE:
		sprintf(buf, "%s DAMAGE      %5d        %5d        %5d (%s)\r\n",
			buf, cur_search->arg[0],
			cur_search->arg[1], cur_search->arg[2],
			(cur_search->arg[2] > 0 && cur_search->arg[2] < TYPE_SUFFERING) ?
			spells[cur_search->arg[2]] : "NULL");
		break;
	case SEARCH_COM_SPAWN:
		sprintf(buf, "%s SPAWN  Spawn_rm: %5d   Targ_rm:%5d   Hunt: %5d\r\n",
			buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
		break;
	case SEARCH_COM_LOADROOM:
		sprintf(buf, "%s LOADROOM  NewLoad: %5d    MaxLevel:%5d    %5d\r\n",
			buf, cur_search->arg[0], cur_search->arg[1], cur_search->arg[2]);
		break;

	default:
		sprintf(buf, "%s ERROR (%d)  %5d        %5d        %5d\r\n",
			buf, cur_search->command, cur_search->arg[0],
			cur_search->arg[1], cur_search->arg[2]);
		break;
	}
	sprintbit(cur_search->flags, search_bits, buf2);
	sprintf(buf, "%s Flags: %s%s%s\r\n",
		buf, CCBLU_BLD(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
}

void
do_olc_xstat(struct char_data *ch)
{
	if (!GET_OLC_SRCH(ch)) {
		send_to_char("You are not currently editing a search.\r\n", ch);
		return;
	}
	print_search_data_to_buf(ch, ch->in_room, GET_OLC_SRCH(ch), buf);
	send_to_char(buf, ch);

}
